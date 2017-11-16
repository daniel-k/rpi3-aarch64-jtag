/*
 * Derived from code:
 * 
 * https://github.com/swarren/rpi-3-aarch64-demo
 * Copyright (C) 2012 Vikram Narayananan <vikram186@gmail.com>
 * (C) Copyright 2012-2016 Stephen Warren
 * Copyright (C) 1996-2000 Russell King
 *
 * SPDX-License-Identifier:	GPL-2.0
 *
 * https://github.com/dwelch67/raspberrypi/tree/master/armjtag
 * Copyright (c) 2012 David Welch dwelch@dwelch.com
 */

#include <stdint.h>

#define BIT(x)	(1U << (x))

#define PERI_BASE				(0x3F000000)

#define MU_BASE					(PERI_BASE + 0x00215040)
#define MU_LSR_TX_NOT_FULL		(BIT(5))
#define MU_LSR_RX_READY			(BIT(0))

#define AUX_ENABLES				(PERI_BASE + 0x00215004)
#define AUX_MU_IO_REG			(PERI_BASE + 0x00215040)
#define AUX_MU_IER_REG			(PERI_BASE + 0x00215044)
#define AUX_MU_IIR_REG			(PERI_BASE + 0x00215048)
#define AUX_MU_LCR_REG			(PERI_BASE + 0x0021504C)
#define AUX_MU_MCR_REG			(PERI_BASE + 0x00215050)
#define AUX_MU_LSR_REG			(PERI_BASE + 0x00215054)
#define AUX_MU_MSR_REG			(PERI_BASE + 0x00215058)
#define AUX_MU_SCRATCH			(PERI_BASE + 0x0021505C)
#define AUX_MU_CNTL_REG			(PERI_BASE + 0x00215060)
#define AUX_MU_STAT_REG			(PERI_BASE + 0x00215064)
#define AUX_MU_BAUD_REG			(PERI_BASE + 0x00215068)

#define GPFSEL0					(PERI_BASE + 0x00200000)
#define GPFSEL1					(PERI_BASE + 0x00200004)
#define GPFSEL2					(PERI_BASE + 0x00200008)
#define GPSET0					(PERI_BASE + 0x0020001C)
#define GPCLR0					(PERI_BASE + 0x00200028)
#define GPPUD					(PERI_BASE + 0x00200094)
#define GPPUDCLK0				(PERI_BASE + 0x00200098)
#define GPPUDCLK1				(PERI_BASE + 0x0020009C)

#define GPFSEL_PIN_MASK			(7U)//(BIT(2) | BIT(1) | BIT(0))
#define GPFSEL_ALT_4			(3U)//(BIT(1) | BIT(0))
#define GPFSEL_ALT_5			(2U)//(BIT(1))

#define __arch_getl(a)		(*(volatile unsigned int *)(a))
#define __arch_putl(v,a)	(*(volatile unsigned int *)(a) = (v))

#define dmb()				__asm__ __volatile__ ("dmb st" : : : "memory")
#define nop()				__asm__ __volatile__ ("nop")
#define __iormb()			dmb()
#define __iowmb()			dmb()

struct bcm283x_mu_regs {
	uint32_t io;
	uint32_t iir;
	uint32_t ier;
	uint32_t lcr;
	uint32_t mcr;
	uint32_t lsr;
	uint32_t msr;
	uint32_t scratch;
	uint32_t cntl;
	uint32_t stat;
	uint32_t baud;
};


static inline uint32_t readl(uint64_t addr)
{
	uint32_t value = __arch_getl(addr);
	__iormb();
	return value;
}


static inline void writel(uint64_t addr, uint32_t value)
{
	__arch_putl(value, addr);
	__iowmb();
}


static void bcm283x_mu_serial_putc(const char data)
{
	struct bcm283x_mu_regs *regs = (struct bcm283x_mu_regs *) MU_BASE;

	/* Wait until there is space in the FIFO */
	while (!(readl((uint64_t) &regs->lsr) & MU_LSR_TX_NOT_FULL));

	/* Send the character */
	writel((uint64_t) &regs->io, data);
}


static void dbg_puts(const char *s)
{
	while (*s)
		bcm283x_mu_serial_putc(*s++);
}


static void enable_af_pins(int clock, uint32_t bitmask)
{
	uint32_t clock_reg;
	int i;

	switch(clock)
	{
	case 0: clock_reg = GPPUDCLK0; break;
	case 1: clock_reg = GPPUDCLK1; break;
	default: return;
	}

	writel(GPPUD, 0);

	for(i = 0; i < 150; i++) nop();

	writel(clock_reg, bitmask);

	for(i = 0; i < 150; i++) nop();

	writel(clock_reg, 0);
}

static void enable_jtag()
{
	uint32_t gpfsel2;

	gpfsel2 = readl(GPFSEL2);
	gpfsel2 &= ~(GPFSEL_PIN_MASK	<<  6);	// Gpio22
	gpfsel2 |=  (GPFSEL_ALT_4		<<  6);	// Alt4: ARM_TRST
	gpfsel2 &= ~(GPFSEL_PIN_MASK	<<  9);	// Gpio23
	gpfsel2 |=  (GPFSEL_ALT_4		<<  9);	// Alt4: ARM_RTCK
	gpfsel2 &= ~(GPFSEL_PIN_MASK	<< 12);	// Gpio24
	gpfsel2 |=  (GPFSEL_ALT_4		<< 12);	// Alt4: ARM_TDO
	gpfsel2 &= ~(GPFSEL_PIN_MASK	<< 15);	// Gpio25
	gpfsel2 |=  (GPFSEL_ALT_4		<< 15);	// Alt4: ARM_TCK
	gpfsel2 &= ~(GPFSEL_PIN_MASK	<< 18);	// Gpio26
	gpfsel2 |=  (GPFSEL_ALT_4		<< 18);	// Alt4: ARM_TDI
	gpfsel2 &= ~(GPFSEL_PIN_MASK	<< 21);	// Gpio27
	gpfsel2 |=  (GPFSEL_ALT_4		<< 21);	// Alt4: ARM_TMS
	writel(GPFSEL2,gpfsel2);

	enable_af_pins(0, BIT(22) | BIT(23) | BIT(24) | BIT(25) | BIT(26) | BIT(27));
}


static void uart_init(void)
{
	writel(AUX_ENABLES, 1);			// enable UART peripheral
	writel(AUX_MU_IER_REG, 0);		// disable UART interrupts for config
	writel(AUX_MU_CNTL_REG, 0);		// disable UART RX/TX for config
	writel(AUX_MU_LCR_REG, 3);		// 8 bit mode (bit 1 reserved!)
	writel(AUX_MU_MCR_REG, 0);		// RTS line high
	writel(AUX_MU_IER_REG, 0);		// disable UART interrupts
	writel(AUX_MU_IIR_REG, 0xC6);	// enable and clear FIFOs
	writel(AUX_MU_BAUD_REG, 270);	// 115200 Baud

	uint32_t gpfsel1;

	gpfsel1 = readl(GPFSEL1);
	gpfsel1 &= ~(GPFSEL_PIN_MASK << 12);	// Gpio14
	gpfsel1 |= (GPFSEL_ALT_5		<< 12);	// Alt5: UART_TXD
	gpfsel1 &= ~(GPFSEL_PIN_MASK << 15); // Gpio15
	gpfsel1 |= (GPFSEL_ALT_5		<< 15);	// Alt5: UART_RXD
	writel(GPFSEL1, gpfsel1);

	enable_af_pins(0, BIT(14) | BIT(15));

	// enable UART RX/TX again
	writel(AUX_MU_CNTL_REG,3);
}


int main()
{
	uart_init();
	dbg_puts("\r\nWaiting for JTAG\r\n");
	enable_jtag();
	while (1)
	{
		bcm283x_mu_serial_putc('.');
		for(int i = 0; i < 1000000; i++) nop();
	}
}

NAME	= rpi3-jtag.elf
KERNEL	= kernel.img
CROSS	= aarch64-linux-gnu-
CFLAGS	= -ggdb3 -std=gnu99 -Wall
LDFLAGS = -Bstatic --gc-sections -nostartfiles -nostdlib

all: $(KERNEL)

%.o: %.asm
	${CROSS}as -o $@ $<

%.o: %.c
	${CROSS}gcc ${CFLAGS} -c -o $@ $<

$(NAME): main.o startup.o led.o
	${CROSS}ld $(LDFLAGS) -o $@ -T linkerscript.ld $^

$(KERNEL): $(NAME)
	${CROSS}objcopy --gap-fill=0xff -j .text -j .rodata -j .data -O binary $< $@

clean:
	rm -f $(KERNEL) $(NAME) main.o startup.o led.o

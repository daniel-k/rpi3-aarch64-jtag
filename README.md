JTAG and bare metal on Raspberry Pi 3
=====================================

## Build

You need the `aarch64-linux-gnu` toolchain.

Typing `make` will build `kernel.img`.

```bash
$ cp config.txt kernel.img /path/to/sdcard/
```

## Get OpenOCD

```bash
$ git clone https://github.com/daniel-k/openocd.git openocd-armv8
$ cd openocd-armv8
$ git checkout armv8
$ ./bootstrap
$ ./configure
$ make -j8
$ sudo make install
```

## Start OpenOCD

Replace interface with your JTAG adapter.

```bash
$ openocd -f interface/ftdi/olimex-arm-usb-ocd.cfg -f target/rpi3.cfg
```

## Prepare CPU

```bash
$ nc localhost 4444

> reset init
reset init
JTAG tap: rpi3.dap tap/device found: 0x4ba00477 (mfg: 0x23b, part: 0xba00, ver: 0x4)
rpi3.cpu: ran after reset and before halt ...
rpi3.cpu1: ran after reset and before halt ...
rpi3.cpu2: ran after reset and before halt ...
rpi3.cpu3: ran after reset and before halt ...
number of cache level 2
cache l2 present :not supported
rpi3.cpu cluster 0 core 0 multi core
target state: halted
target halted in ARM64 state due to debug-request, current mode: EL3H
cpsr: 0x800003cd pc: 0x100000504
MMU: disabled, D-Cache: disabled, I-Cache: disabled
number of cache level 2
cache l2 present :not supported
rpi3.cpu1 cluster 0 core 1 multi core
```

## Load and start program

```
> targets rpi3.cpu; halt; load_image kernel.img 0 bin; reg pc 0; resume
halt; load_image kernel.img 0 bin; reg pc 0; resume
target state: halted
target halted in ARM64 state due to debug-request, current mode: EL3H
cpsr: 0x800003cd pc: 0x3c0
MMU: disabled, D-Cache: disabled, I-Cache: disabled
1011 bytes written at address 0x00000000
downloaded 1011 bytes in 0.061059s (16.170 KiB/s)
pc (/64): 0x0000000000000000
```

## OpenOCD commands

 - `reset init`: prepare CPU for debugging
 - `targets`: list and select cores
 - `halt`: stop execution
 - `resume`: resume execution
 - `reg`: show registers
 - `reg REG VALUE`: set value of register
 - `step`: single-step
 - `bp ADDR SIZE [hw]`: set a breakpoint at ADDR (you must supply the opcode size)
 - `load_image FILENAME ADDR [TYPE]`: copy program to RAM at given address
 - `poll`: show debugging state

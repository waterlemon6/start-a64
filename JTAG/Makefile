obj-m := JTAG.o

KERNEL_DIR := /home/waterlemon/arm-linux/a64/lichee/linux-3.10

all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KERNEL_DIR) M=`pwd` modules

.PHONY: clean
clean:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KERNEL_DIR) M=`pwd` modules clean


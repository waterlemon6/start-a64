obj-m := SPISlaveDevice.o

KERNEL_DIR := /home/waterlemon/arm-linux/cqa64_linux_qt5.3.2/lichee/linux-3.10

all:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KERNEL_DIR) M=`pwd` modules

.PHONY: clean
clean:
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -C $(KERNEL_DIR) M=`pwd` modules clean


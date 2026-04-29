obj-m += deadlock_demo.o

KDIR := /home/ansar/embedded-linux-qemu-labs/kernel/linux
CROSS := arm-linux-

all:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS) modules

clean:
	make -C $(KDIR) M=$(PWD) ARCH=arm CROSS_COMPILE=$(CROSS) clean

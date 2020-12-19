# TODO: Change this to the location of your kernel source code
KERNEL_SOURCE=/project/scratch01/compiling/czihao/linux_source/linux

EXTRA_CFLAGS += -DMODULE=1 -D__KERNEL__=1

kernel_memory-objs := $(kernel_memory-y)
obj-m := mmu_module.o

PHONY: all

all:
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_SOURCE) ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- M=$(PWD) clean 

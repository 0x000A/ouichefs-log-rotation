KERNELDIR ?= /lib/modules/$(shell uname -r)/build

obj-m += largest.o oldest.o

all :
	make -C $(KERNELDIR) M=$(PWD) src=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) src=$(PWD) clean


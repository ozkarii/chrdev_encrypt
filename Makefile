CONFIG_MODULE_SIG=n
obj-m = helloworld.o
orig-dir = $(PWD)
#KVERSION = $(shell uname -r)

all:
	#make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules  <- to build if kernel source is in /usr/src/kernel
	make -C kernel-headers/6.11.8-200.fc40.x86_64 M=$(PWD) KBUILD_OUTPUT=$(orig-dir)/kernel-src/6.11.8-200.fc40.x86_64 modules

clean:
	#make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean <- to build if kernel source is in /usr/src/kernel
	make -C kernel-headers/6.11.8-200.fc40.x86_64 M=$(PWD) KBUILD_OUTPUT=$(orig-dir)/kernel-src/6.11.8-200.fc40.x86_64 clean

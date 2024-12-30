CONFIG_MODULE_SIG=n
obj-m = helloworld.o
orig-dir = $(PWD)
KVERSION = $(shell uname -r)

all:
	# to build if kernel source and headers installed
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

	# to build if kernel source placed and headers placed in project directory kernel-src/ and kernel-headers/
	# make -C kernel-headers/$(KVERSION)/build M=$(PWD) KBUILD_OUTPUT=$(orig-dir)/kernel-src/$(KVERSION) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
	#make -C kernel-headers/$(KVERSION) M=$(PWD) KBUILD_OUTPUT=$(orig-dir)/kernel-src/$(KVERSION) clean

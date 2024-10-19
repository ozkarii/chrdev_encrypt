CONFIG_MODULE_SIG=n
obj-m = helloworld.o
KVERSION = 6.10.11-200.fc40.x86_64

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean

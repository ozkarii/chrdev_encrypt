CONFIG_MODULE_SIG=n
obj-m = encryptor.o
KVERSION = $(shell uname -r)

# to build if kernel source and headers installed

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules
	gcc set_key.c -o set-key
	gcc decryptor.c -o decryptor

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
	rm -f set-key decryptor

# Encryptor character device

Operating systems course project

Report below

Developement was done on a virtual machine running Fedora 40 with the kernel version being
6.11.8-200.fc40.x86_64

To build the module, kernel headers and the kernel developement packages needed to be installed:

    $ sudo dnf install kernel-headers kernel-devel

After installing, the module should compile by simply running make:

    $ make all

The module can be loaded into the kernel with:

    $ sudo insmod encryptor.ko

To create an inode representing the character device we first need to find out the driver's major number which the kernel has allocated to it:

    $ cat /proc/devices | grep encryptor
    239 encryptor

Creating the inode is done with the mknod command, we specify the file we want to create,
set it to be of character device type, as well as give it the major number and the minor number which is the first available, so 0:

    $ sudo mknod /dev/encryptor c 239 0

Now the device should be accessable through the file /dev/encryptor.

The device uses XOR Cipher for encryption thanks to its simplicity. The encrypted message is as long as the input message.

The encryption key can be provided by using ioctl with the following struct as the only argument:

    typedef struct {
        size_t size;
        char* key;
    } EncryptionKey;

The key can be of any length, but no longer than 256 characters which has been set as the limit in the driver code.
Giving a longer key will have no effect.

We have provided a simple program which calls ioctl with the user given encryption key:

    $ sudo ./set-key <device_file> <encryption_key>

A message can be encrypted by writing it to the file representing the device:

    $ echo "<message>" | sudo tee <device_file> 

The encryption will be performed when writing, so the loaded encryption at the time of writing key will be used.
If no key has been loaded, writing will do nothing.

To get the encrypted message, it can be read from the same file normally, some bytes are not displayed as text by commands like ```cat``` so it is best to read it in hex:
    
    $ od -t x1 <device>

We also created a program to decrypt the message and print it as a test:

    $ ./decryptor <device> <key>

For debugging and testing, prints to the kernel log buffer were done with macros like ```printk()``` and ```pr_err()```. The kernel buffer was examined with:

    $ sudo dmesg -wH


## Example run
    oskar@fedora:~$ sudo ./set-key /dev/encryptor kayttojarjestelmat
    oskar@fedora:~$ echo "hello world" | sudo tee /dev/encryptor 
    hello world
    oskar@fedora:~$ ./decryptor /dev/encryptor kayttojarjestelmat
    hello world

Log

    [Jan 3 22:05] Copying key struct
    [  +0.000008] Allocating memory for the new key
    [  +0.000005] Key updated successfully to kayttojarjestelmat, length: 18
    [Jan 3 22:06] Message to be encrypted: hello world
                (12)
    [  +0.000011] Encryption key: kayttojarjestelmat
    [  +0.000005] Allocating new encrypted message
    [  +0.000004] XOR cipher
    [  +0.000002] Encrypted message: \x03\x04\x15\x18\x1bO\x1d\x0e (12)
    [  +0.000004] Encrypted message length: 12

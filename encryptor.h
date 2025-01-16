#ifndef ENCRYPTOR_H
#define ENCRYPTOR_H

#include <linux/unistd.h>
#include <linux/ioctl.h>

//Ioctl control command.
//_IOW macro declares data trasnfer from userspace to the device.
//Arguments 'k' and 1 defines unique ioctl command number and last argument
//is type of data to transfer.
#define IOCTL_SET_KEY _IOW('k', 1, char*)
#define MAX_KEY_SIZE 256

typedef struct {
    size_t size;
    char* key;
} EncryptionKey;

#endif

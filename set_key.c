/*
* Program to pass encryption key to device
* driver with ioctl() system call
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "encryptor.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        printf("Usage: %s <filename> <key>\n", argv[0]);
        return -1;
    }

    char *filename = argv[1];
    char *key = argv[2];
    size_t key_length = strlen(key);

    //A struct is passed to a driver
    EncryptionKey encryption_key = {key_length, key};

    int fd = open(filename, O_RDWR);

    if (fd < 0)
    {
        printf("Opening the file failed\n");
        return -1;
    }

    //Ioctl system call to pass encryption key to driver
    //Second argument is control command to declare the direction of data transfer
    //and data type. It is defined in "encryptor.h".
    if (ioctl(fd, IOCTL_SET_KEY, &encryption_key) < 0)
    {
        printf("Error in ioctl\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}
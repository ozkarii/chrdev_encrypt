#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "hello_ioctl.h"

int main()
{
    char word[] = "salattuteksti";
    char key[] = "avain";
    char encrypted_word[255] = "";

    int fd = open("/dev/myHelloDriver", O_RDWR);
    if (fd < 0)
    {
        printf("Opening the file failed");
        return -1;
    }
    
    if (ioctl(fd, ENCRYPT, key) < 0)
    {
        printf("Error in ioctl\n");
        close(fd);
        return -1;
    }
    
    close(fd);
    return 0;
}
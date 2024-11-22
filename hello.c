#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/string.h>

#include "hello_ioctl.h"

#define DEVICE_NAME "hello"
#define MINOR_COUNT 1

MODULE_LICENSE("GPL");

static char kernel_buffer[255];
static size_t buffer_size = 255;
static char key[255];

dev_t dev;
unsigned int first_minor = 0;
struct cdev my_cdev;

static int __init init_my_module(void);
void __exit exit_my_module(void);

static int my_open(struct inode* inode, struct file* file)
{
	return 0;
}

static ssize_t my_read(struct file* file, char __user* buffer, size_t len, loff_t* offset)
{
    int to_copy;
    if (*offset >= buffer_size)
    {
        return 0;
    }
    //only copy size that is left in the buffer
    to_copy = min(len, buffer_size-*offset);
    //copy_to_user returns amount of bytes that not were copied
    if (copy_to_user(buffer, kernel_buffer+*offset, to_copy))
    {
        printk("Error copying to user\n");
    }
    //Update offset by ammount of bytes that were copied
    *offset += to_copy;
    return to_copy;
}

static ssize_t my_write(struct file* file, const char __user* buffer, size_t len, loff_t* offset)
{
    int to_copy;
    to_copy = min(len, buffer_size-*offset);
    if (copy_from_user(kernel_buffer+*offset, buffer, to_copy))
    {
        printk("Error copying from user\n");
    }
    *offset = to_copy;
    return to_copy;
}

static long int my_ioctl(struct file* file, unsigned int cmd, unsigned long arg)
{
    switch (cmd)
    {
        //TODO: Add encryption to written word
        case ENCRYPT:
            if (copy_from_user(&key, (char*) arg, sizeof(key)))
            {
                printk("Error copying key from user\n");
            }
            else
            {
                printk("Given key is: %s\n", key);
            }
            break;
        default:
            printk("Invalid ioctl command");
            return -1;
    }
    return 0;
}

static int my_release(struct inode* inode, struct file* file)
{
	return 0;
}

static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
    .unlocked_ioctl = my_ioctl
};

static int __init init_my_module(void)
{
	printk("Init module hello\n");
	int ret;
    //Dynamically allocate device numbers. dev is output-only parameter and
    //holds the first device number
    ret = alloc_chrdev_region(&dev, first_minor, MINOR_COUNT, DEVICE_NAME);
	if (ret < 0)
	{
		printk("Error allocating major/minor numbers: %d\n", ret);
		return -1;
	}
    //Print major number and firts minor number
    printk("Hello module registered, MAJOR: %d MINOR: %d\n", MAJOR(dev), MINOR(dev));
    //Initialize cdev structure my_cdev
	cdev_init(&my_cdev, &my_fops);
    //Add my_cdev to kernel
    ret = cdev_add(&my_cdev, dev, MINOR_COUNT);
	if ( ret < 0)
	{
		printk("Error adding char device\n");
		return -1;
	}
	return 0;
}

void __exit exit_my_module(void)
{
	printk("Module hello exit\n");
    //Remove char device from the system
	cdev_del(&my_cdev);
    //Unregister device numbers
	unregister_chrdev_region(dev, MINOR_COUNT);
}

module_init(init_my_module);
module_exit(exit_my_module);

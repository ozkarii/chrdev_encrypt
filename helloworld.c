#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "helloworld"
#define MINOR_COUNT 1

MODULE_LICENSE("GPL");

dev_t major;
unsigned int minor;
struct cdev my_cdev;
const char* moi = "Moi maailma!\n";
size_t moi_len = 13;

static int __init init_my_module(void);
void __exit exit_my_module(void);

static int my_open(struct inode* inode, struct file* file)
{
	return 0;
}

static ssize_t my_read(struct file* file, char __user* buffer, size_t len, loff_t* offset)
{   
    if (*offset >= moi_len)
    {
        return 0;
    }

    if (len > (moi_len - *offset))
    {
        len = (moi_len - *offset);
    }

    if (copy_to_user(buffer, moi + *offset, len))
    {
		printk(KERN_ERR "Error copying to user\n");
        return -EFAULT;
    }
        
    *offset += len;
    return len;
}

static ssize_t my_write(struct file* file, const char __user* buffer, size_t len, loff_t* offset)
{
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
    .release = my_release
};

static int __init init_my_module(void)
{
	printk("init\n");
	int ret = alloc_chrdev_region(&major, minor, MINOR_COUNT, DEVICE_NAME);
	if (ret < 0)
	{
		printk(KERN_ERR "Error allocating major/minor numbers: %d\n", ret);
		return -1;
	}
	cdev_init(&my_cdev, &my_fops);
	if (cdev_add(&my_cdev, major, MINOR_COUNT) < 0)
	{
		printk(KERN_ERR "Error adding char device\n");
		return -1;
	}

	return 0;
}

void __exit exit_my_module(void)
{
	printk("exit\n");
	cdev_del(&my_cdev);
	unregister_chrdev_region(major, 1);
}

module_init(init_my_module);
module_exit(exit_my_module);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <crypto/skcipher.h>
#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "encryptor.h"

#define DEVICE_NAME "encryptor"
#define MINOR_COUNT 1

MODULE_LICENSE("GPL");

dev_t major;
unsigned int minor;
struct cdev encryptor_cdev;

EncryptionKey encryption_key = {0, NULL};
char* encrypted_message = NULL;
ssize_t encrypted_message_len = 0;

// Function prototypes for implemented system calls
static int __init init_encryptor_module(void);
void __exit exit_encryptor_module(void);
static int open_encryptor(struct inode* inode, struct file* file);
static ssize_t read_encrypted(struct file* file, char __user* buffer, size_t len, loff_t* offset);
static ssize_t write_message(struct file* file, const char __user* buffer, size_t len, loff_t* offset);
static long int ioctl_set_key(struct file* file, unsigned int cmd, unsigned long arg);
static int release_module(struct inode* inode, struct file* file);

// Application specific function declarations
ssize_t encrypt_message(const char* message, size_t message_size);

// File operations implemented by the module
static struct file_operations encryptor_fops = {
    .owner = THIS_MODULE,
    .open = open_encryptor,
    .read = read_encrypted,
    .write = write_message,
    .unlocked_ioctl = ioctl_set_key,
    .release = release_module
};


/**
 * @brief Initialize the encryptor module, allocate device numbers and add the cdev to the kernel
 * 
 * @return int 0 on success, negative error code on failure
 */
static int __init init_encryptor_module(void)
{
	pr_info("Init encryptor module...\n");
    //Dynamically allocate device numbers. dev is output-only parameter and
    //holds the first device number
    int ret = alloc_chrdev_region(&major, minor, MINOR_COUNT, DEVICE_NAME);
	if (ret < 0)
	{
		pr_err("Error allocating major/minor numbers: %d\n", ret);
		return -EFAULT;
	}
    // Initialize cdev structure
	cdev_init(&encryptor_cdev, &encryptor_fops);
    // Add cdev to kernel
    ret = cdev_add(&encryptor_cdev, major, MINOR_COUNT);
	if (ret < 0)
	{
		pr_err("Error adding char device\n");
		return -EFAULT;
	}

	return 0;
}

/**
 * @brief Unload the encryptor module, free the allocated memory and unregister the device.
 *        Called automatically when the module is removed from the kernel.
 */
void __exit exit_encryptor_module(void)
{
	pr_info("Exit encryptor module...\n");
	// Free dynamic strings
	kfree(encrypted_message);
    kfree(encryption_key.key);
    //Remove char device from the system
	cdev_del(&encryptor_cdev);
    //Unregister device numbers
	unregister_chrdev_region(major, MINOR_COUNT);

}

/**
 * @brief Open the encryptor device, does nothing so just returns 0 for success
 * 
 * @param inode inode of the device
 * @param file file struct of the device
 * @return int 0 on success
 */
static int open_encryptor(struct inode* inode, struct file* file)
{
	return 0;
}

/**
 * @brief Implements read system call for encryptor device, reads the encrypted message to the user
 * 
 * @param file file struct, not used
 * @param buffer user buffer to write the message to
 * @param len length of the buffer
 * @param offset offset in reading the message
 * @return ssize_t amount of bytes read, negative error code on failure
 */
static ssize_t read_encrypted(struct file* file, char __user* buffer, size_t len, loff_t* offset)
{   
    if (*offset >= encrypted_message_len)
    {
        return 0;
    }

    if (len < encrypted_message_len) {
        pr_err("User buffer too small\n");
        return -EINVAL;
    }

    //only copy size that is left in the buffer
    int copied = min(encrypted_message_len, encrypted_message_len - *offset);

    //copy_to_user returns amount of bytes that not were copied yet
    if (copy_to_user(buffer, encrypted_message + *offset, copied))
    {
		pr_err( "Error copying to user\n");
        return -EFAULT;
    }

    //Update offset by ammount of bytes that were copied
    *offset += copied;
    return copied;
}

/**
 * @brief Implements write system call for encryptor device, encrypts the message
 *        written by the user and encrypts it with the stored key.
 * 
 * @param file file struct, not used
 * @param buffer user buffer containing the message
 * @param len length of the message
 * @param offset offset in writing the message
 * @return ssize_t amount of bytes written, negative error code on failure
 */
static ssize_t write_message(struct file* file, const char __user* buffer, size_t len, loff_t* offset)
{
    // If encryption key is nullpointer, there is no key -> do nothing
    if (!encryption_key.key)
    {
        pr_err("No encryption key loaded\n");
        return -EFAULT;
    }
    char* message_buffer = kmalloc(len, GFP_KERNEL);

    int copied = min(len, len - *offset);
    if (copy_from_user(message_buffer + *offset, buffer, copied))
    {
        pr_err("Error copying from user\n");
        kfree(message_buffer);
        return -EFAULT;
    }
    if (copied == len)
    {
        encrypted_message_len = encrypt_message(message_buffer, len);
        if (encrypted_message_len < 0)
        {
            pr_err("Encryption failed\n");
            encrypted_message_len = -1;
            return -EFAULT;
        }
        else
        {
            pr_info("Encrypted message length: %zd\n", encrypted_message_len);            
            return copied;
        }
    }
    *offset = copied;
    return copied;
}

/**
 * @brief Set the key for encryption, implements ioctl system call
 * 
 * @param file file struct, not used
 * @param cmd ioctl command, should be IOCTL_SET_KEY
 * @param arg Key length and key, cast to EncryptionKey struct
 * @return long int 0 on success, negative error code on failure
 */
static long int ioctl_set_key(struct file* file, unsigned int cmd, unsigned long arg)
{
	// Temp key is used to handle errors and copy safely from user space
    EncryptionKey temp_key;

    switch (cmd) {
        case IOCTL_SET_KEY: {
            pr_info("Copying key struct\n");
            if (copy_from_user(&temp_key, (EncryptionKey*)arg, sizeof(EncryptionKey)))
            {
                pr_err("Error copying key struct\n");
                return -EFAULT;
            }

            // Validate the key length
            if (temp_key.size == 0 || temp_key.size > MAX_KEY_SIZE)
            {
                pr_err("Invalid key length: %zu\n", temp_key.size);
                return -EINVAL;
            }

            // free old key if there is one
            if (encryption_key.size > 0)
            {
                kfree(encryption_key.key);
                encryption_key.size = 0;
            }
            
            pr_info("Allocating memory for the new key\n");
            encryption_key.key = kzalloc(temp_key.size, GFP_KERNEL);
            if (!encryption_key.key)
            {
                return -ENOMEM;
            }

			pr_info("Copying key\n");
            if (copy_from_user(encryption_key.key, temp_key.key, temp_key.size))
            {
            	pr_err("Error copying key\n");
                return -EFAULT;
            }

            encryption_key.size = temp_key.size;

            pr_info("Key updated successfully to %s, length: %zu\n", encryption_key.key, encryption_key.size);
            return 0;
        }
        default:
            pr_err("Invalid ioctl cmd %d\n", cmd);
            return -EFAULT;
    }
}

/**
 * @brief Called when module unloaded, only returns success
 * 
 * @param inode inode of the character device
 * @param file file struct
 * @return int success 0
 */
static int release_module(struct inode* inode, struct file* file)
{
	return 0;
}

/**
 * @brief Encrypts the given message with XOR cipher with the global key
 * 
 * @param message message to be encrypted
 * @param message_size message length in bytes
 * @return ssize_t size of the encrypted message in bytes, negative error code on failure
 */
ssize_t encrypt_message(const char* message, size_t message_size)
{
    pr_info("Message to be encrypted: %s (%zu)\n", message, message_size);
    pr_info("Encryption key: %s\n", encryption_key.key);
    if (encrypted_message)
    {
        kfree(encrypted_message); // Free previously allocated buffer for message
        encrypted_message = NULL;
    }
    pr_info("Allocating new encrypted message\n");
    
    encrypted_message_len = message_size;
    encrypted_message = kzalloc(encrypted_message_len, GFP_KERNEL);
    if (!encrypted_message)
        return -ENOMEM;

    pr_info("XOR cipher\n");
    for (size_t i = 0; i < message_size; i++)
    {
        encrypted_message[i] = message[i] ^ encryption_key.key[i % encryption_key.size];
    }

    pr_info("Encrypted message: %s (%zu)", encrypted_message, encrypted_message_len);
    return encrypted_message_len;
}

module_init(init_encryptor_module);
module_exit(exit_encryptor_module);

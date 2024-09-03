/**************************************************************
* Class:  CSC-415-02 Summer 2024
* Name: Austin Kuykendall
* Student ID: 920222066
* GitHub ID: austinlikesfood
* Project: Assignment 6 – Device Driver
*
* File: encryption.c
*
* Description: Takes in user input and encrypts data
* 
*
**************************************************************/
#define MY_MAJOR 415
#define MY_MINOR 0
#define DEVICE_NAME "encryption"
#define BUFFER_SIZE 512
#define KEY 5
// the above defines are used for the installer.sh file
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/vmalloc.h>
#include <linux/sched.h>
#define SETKEY _IO('e', 2)
#define DECRYPT _IO('e', 1)
#define ENCRYPT _IO('e', 0)

MODULE_AUTHOR("Austin");
MODULE_DESCRIPTION("encryption program");
MODULE_LICENSE("GPL");
static int encrypt(int key);
static int decrypt(int key);
static int myOpen(struct inode * inode, struct file * fs);
static int myClose(struct inode * inode, struct file * fs);
static long myIoCtl (struct file * fs, unsigned int command, unsigned long data);
static ssize_t myRead(struct file * fs, char __user * buf, size_t hsize, loff_t * off);
static ssize_t myWrite(struct file * fs, const char __user * buf, size_t hsize, loff_t * off);
// kernel buffer
char *kbuf;
struct cdev my_cdev;
// we need this to know which state teh data is in. Encrypted or not
// so I chose 1, 0 
typedef struct encds {
    int key;
    int flag;
} encds;
// struct for our lower level operations 
struct file_operations fops = {
    .open = myOpen,
    .read = myRead,
    .read = myRead,
    .write = myWrite,
    .release = myClose,
    .unlocked_ioctl = myIoCtl,
    .owner = THIS_MODULE
};
// based on the kbuff, we know how many bytes to encrypt
// this is how we know what will be encrypted 
static ssize_t myWrite (struct file * fs, const char __user * buf, size_t hsize, loff_t * off) {
    int err;
    struct encds * ds;
    ds = (struct encds *) fs->private_data;
    err = copy_from_user(kbuf + *off, buf, hsize);

    printk(KERN_SOH "leftover value or offset: %lld", *off);
    encrypt(ds->key);

    ds->flag = 1;

    return hsize;
}

// this read funciton allows interaction with the buffer and kernel
static ssize_t myRead (struct file * fs, char __user * buf, size_t hsize, loff_t * off) {
    int bufferLength;
    int err;
    struct encds * ds;
    ds = (struct encds *) fs->private_data;

    bufferLength = strlen(kbuf);

    printk(KERN_SOH "Buffer Length: %d   value of offset: %lld", bufferLength, *off);

    if (bufferLength < hsize) 
    {
        hsize = bufferLength;
    }
    err = copy_to_user(buf, kbuf + *off, hsize);
    if (err != 0) 
    {
        printk(KERN_ERR "user copying failed: %d byte error amount\n", err);
        return -1;
    }
    return 0;
}

// saves data as the file descrtiptor
static int myOpen(struct inode * inode, struct file * fs) {
    struct encds * dataDescriptor;
    dataDescriptor = vmalloc(sizeof(struct encds));

    if (dataDescriptor == 0)
    {
        printk(KERN_ERR "Cannot vmalloc, File not opened.\n");
        return -1;
    }
    dataDescriptor->key = KEY;
    dataDescriptor->flag = 0;
    fs->private_data = dataDescriptor;
    return 0;
}

// after we open we can close to make sure the memory is free.
// this way we dont waste space
static int myClose(struct inode * inode, struct file * fs) {
    struct encds * ds;

    ds = (struct encds *) fs->private_data;
    vfree(ds);
    return 0;
}
// maniuplate the i variable to create our encrytion
// change the value 
static int encrypt(int key) {
    int i;
    int bufferLength;
    bufferLength = strlen(kbuf);
    for (i = 0; i < bufferLength - 1; i++)
    {
        kbuf[i] = ((kbuf[i] - key) % 128) + 1;
    }
    printk(KERN_INFO "The encrytion is:\n%s\n", kbuf);
    return 0;
}
// to read idt back and verify it wit hthe user, 
// we implemetn a decypher method below.
static int decrypt(int key) {
    int i; 
    int bufferLength;
    bufferLength = strlen(kbuf);
    // reads back what was originally encrytped in the above function
    for (i = 0; i < bufferLength - 1; i++)
    {
        kbuf[i] = (kbuf[i] + key - 1) % 128;
    }

    printk(KERN_INFO "Text decrytped:\n%s\n", kbuf);
    return 0;
}

// this is our manipultaion of device fiels with a swtitch
static long myIoCtl (struct file * fs, unsigned int command, unsigned long retVal) {
    struct encds * driverStatus;
    printk(KERN_INFO "inside myIoCtl");

    driverStatus = (struct encds *) fs->private_data;
    switch (command)
    {
        case ENCRYPT:
            encrypt(driverStatus->key);
            driverStatus->flag = 1;
            break;
        case DECRYPT:
            decrypt(driverStatus->key);
            driverStatus->flag = 0;
            break;
        case SETKEY:
            driverStatus->key = (int) retVal;
            break;
    }
    return 0;
}

// creates a device node in /dev, returns error if not made
int init_module(void) {
    int result;
    int registers;
    dev_t devNumber;
    devNumber = MKDEV(MY_MAJOR, MY_MINOR);

    registers = register_chrdev_region(devNumber, 1, DEVICE_NAME);
    printk(KERN_INFO "chardev processed: %d\n", registers);
    cdev_init(&my_cdev, &fops);

    kbuf = vmalloc(BUFFER_SIZE);
    if (kbuf == NULL)
    {
        printk(KERN_ERR "failed to parse k buffer\n");
        return -1;
    }

    result = cdev_add(&my_cdev, devNumber, 1);

    return result;
}

// gets rid of the module after the program is terminated 
void cleanup_module(void) {
    dev_t devno;

    devno = MKDEV(MY_MAJOR, MY_MINOR);
    unregister_chrdev_region(devno, 1);
    cdev_del(&my_cdev);
    // always need to free after malloc
    vfree(kbuf);
    // make sure its null after so we can ahrd code it 
    kbuf = NULL;

    printk(KERN_INFO "Exiting.\n");
}
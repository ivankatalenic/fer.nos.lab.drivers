/*
 * shofer.c -- module implementation
 *
 * Copyright (C) 2022 Ivan Katalenic
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 * No warranty is attached.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/kfifo.h>
#include <linux/poll.h>
#include <linux/semaphore.h>

#include "config.h"

static int message_queue_size = MESSAGE_QUEUE_SIZE; // Maximum number of messages in the queue.
static int message_size = MESSAGE_SIZE;             // Maximum size of a message in the queue.
static int max_users = MAX_USERS;                   // Maximum number of the driver users.

/* Some parameters can be given at module load time */
module_param(message_queue_size, int, S_IRUGO);
MODULE_PARM_DESC(message_queue_size, "Maximum message queue size");
module_param(message_size, int, S_IRUGO);
MODULE_PARM_DESC(message_size, "Maximum message size in bytes");
module_param(max_users, int, S_IRUGO);
MODULE_PARM_DESC(max_users, "Maximum number of users");

MODULE_AUTHOR(AUTHOR);
MODULE_LICENSE(LICENSE);

struct shofer_dev* shofer;
static dev_t dev_no = 0;

/* prototypes */
static struct shofer_dev *shofer_create(dev_t, struct file_operations *,
	int *);
static void shofer_delete(struct shofer_dev *);
static void cleanup(void);

static int shofer_open(struct inode *, struct file *);
static int shofer_release(struct inode *, struct file *);
static ssize_t shofer_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t shofer_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations shofer_fops = {
	.owner =    THIS_MODULE,
	.open =     shofer_open,
	.release =  shofer_release,
	.read =     shofer_read,
	.write =    shofer_write,
};

/* init module */
static int __init shofer_module_init(void)
{
	int retval;
	
	klog(KERN_NOTICE, "Module started initialization");

	// Get the device number
	retval = alloc_chrdev_region(&dev_no, 0, 1, DRIVER_NAME);
	if (retval < 0) {
		klog(KERN_WARNING, "Can't get the device number");
		return retval;
	}

	shofer = shofer_create(dev_no, &shofer_fops, &retval);
	if (!shofer) {
		cleanup();
		return retval;
	}

	klog(KERN_NOTICE, "Module initialized with major=%d", MAJOR(dev_no));

	return 0;
}

static void cleanup(void)
{
	struct message *msg;
	struct message *next;

	list_for_each_entry_safe(msg, next, &shofer->queue.list, list) {
		list_del(&msg->list);
		kfree(msg);
	}

	shofer_delete(shofer);

	if (dev_no)
		unregister_chrdev_region(dev_no, 1);
}

/* called when module exit */
static void __exit shofer_module_exit(void)
{
	klog(KERN_NOTICE, "Module started exit operation");
	cleanup();
	klog(KERN_NOTICE, "Module finished exit operation");
}

module_init(shofer_module_init);
module_exit(shofer_module_exit);

/* Create and initialize a single shofer_dev */
static struct shofer_dev *shofer_create(dev_t dev_no,
	struct file_operations *fops, int *retval)
{
	struct shofer_dev *shofer = kmalloc(sizeof(struct shofer_dev), GFP_KERNEL);
	if (!shofer){
		*retval = -ENOMEM;
		klog(KERN_WARNING, "kmalloc failed\n");
		return NULL;
	}
	memset(shofer, 0, sizeof(struct shofer_dev));

	shofer->dev_no = dev_no;

	shofer->users_cnt = 0;
	mutex_init(&shofer->users_lock);

	cdev_init(&shofer->cdev, fops);
	shofer->cdev.owner = THIS_MODULE;
	shofer->cdev.ops = fops;
	*retval = cdev_add(&shofer->cdev, dev_no, 1);
	if (*retval) {
		klog(KERN_WARNING, "Error (%d) when adding a device", *retval);
		kfree(shofer);
		return NULL;
	}

	INIT_LIST_HEAD(&shofer->queue.list);
	shofer->queue.size = 0;
	mutex_init(&shofer->queue.lock);

	sema_init(&shofer->rsem, 0);
	sema_init(&shofer->wsem, message_queue_size);

	*retval = 0;

	return shofer;
}
static void shofer_delete(struct shofer_dev *shofer)
{
	cdev_del(&shofer->cdev);
	kfree(shofer);
}

/* Open called when a process calls "open" on this device */
static int shofer_open(struct inode *inode, struct file *filp)
{
	struct shofer_dev *shofer; /* device information */
	shofer = container_of(inode->i_cdev, struct shofer_dev, cdev);
	filp->private_data = shofer; /* for other methods */

	if ((filp->f_flags & O_ACCMODE) != O_RDONLY && (filp->f_flags & O_ACCMODE) != O_WRONLY)
		return -EPERM;
	
	if (mutex_lock_interruptible(&shofer->users_lock))
		return -ERESTARTSYS;
	
	if (shofer->users_cnt > max_users) {
		mutex_unlock(&shofer->users_lock);
		return -EDQUOT;
	}

	shofer->users_cnt++;

	mutex_unlock(&shofer->users_lock);

	return 0;
}

/* Called when a process performs "close" operation */
static int shofer_release(struct inode *inode, struct file *filp)
{
	struct shofer_dev *shofer = filp->private_data;

	if (mutex_lock_interruptible(&shofer->users_lock))
		return -ERESTARTSYS;

	shofer->users_cnt--;
	
	mutex_unlock(&shofer->users_lock);

	return 0;
}

static ssize_t shofer_read(struct file *filp, char __user *ubuf, size_t count,
	loff_t *f_pos)
{
	ssize_t retval = 0;
	struct shofer_dev *shofer = filp->private_data;
	struct message *msg;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
		return -EPERM;
	if (count < message_size)
		// Attempted to read a partial message which is not allowed.
		return -EINVAL;
	
	if (down_interruptible(&shofer->rsem))
		return -ERESTARTSYS;
	if (mutex_lock_interruptible(&shofer->queue.lock)) {
		up(&shofer->rsem);
		return -ERESTARTSYS;
	}

	msg = list_first_entry(&shofer->queue.list, struct message, list);

	retval = copy_to_user(ubuf, msg->data, msg->size);
	if (retval) {
		mutex_unlock(&shofer->queue.lock);
		up(&shofer->rsem);
		klog(KERN_WARNING, "copy_to_user failed\n");
		return retval;
	} else
		retval = count;
	
	list_del(&msg->list);
	kfree(msg);

	mutex_unlock(&shofer->queue.lock);

	up(&shofer->wsem);

	return retval;
}

static ssize_t shofer_write(struct file *filp, const char __user *ubuf,
	size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;
	struct shofer_dev *shofer = filp->private_data;
	struct message *msg;

	if ((filp->f_flags & O_ACCMODE) == O_RDONLY)
		return -EPERM;
	if (count > message_size)
		return -EMSGSIZE;

	if (down_interruptible(&shofer->wsem))
		return -ERESTARTSYS;
	if (mutex_lock_interruptible(&shofer->queue.lock)) {
		up(&shofer->wsem);
		return -ERESTARTSYS;
	}

	msg = kmalloc(sizeof(struct message) + count, GFP_KERNEL);
	if (!msg){
		mutex_unlock(&shofer->queue.lock);
		up(&shofer->wsem);
		klog(KERN_WARNING, "kmalloc failed\n");
		return -ENOMEM;
	}
	msg->data = (char *)(msg + 1);
	msg->size = count;

	retval = copy_from_user(msg->data, ubuf, count);
	if (retval) {
		mutex_unlock(&shofer->queue.lock);
		up(&shofer->wsem);
		kfree(msg);
		klog(KERN_WARNING, "copy_from_user failed\n");
		return retval;
	} else
		retval = count;

	list_add_tail(&msg->list, &shofer->queue.list);

	mutex_unlock(&shofer->queue.lock);

	up(&shofer->rsem);

	return retval;
}

/*
 * config.h -- structures, constants, macros
 *
 * Copyright (C) 2022 Ivan Katalenic
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form.
 * No warranty is attached.
 *
 */

#pragma once

#define DRIVER_NAME 	"shofer"

#define AUTHOR		"Ivan Katalenic"
#define LICENSE		"Dual BSD/GPL"

#define MESSAGE_QUEUE_SIZE 8
#define MESSAGE_SIZE       256
#define MAX_USERS          6

struct message {
	struct list_head list;
	
	char*  data;
	size_t size;
};

struct message_queue {
	struct list_head list;
	size_t           size;
	
	struct mutex lock;
};

/* Device driver */
struct shofer_dev {
	dev_t dev_no;		/* Device number */
	struct cdev cdev;	/* Char device structure */

	struct mutex users_lock;
	size_t users_cnt;

	struct message_queue queue;

	struct semaphore rsem;
	struct semaphore wsem;
};


#define klog(LEVEL, format, ...)	\
printk ( LEVEL "[shofer] %d: " format "\n", __LINE__, ##__VA_ARGS__)

//printk ( LEVEL "[shofer]%s:%d]" format "\n", __FILE__, __LINE__, ##__VA_ARGS__)
//printk ( LEVEL "[shofer]%s:%d]" format "\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__)

//#define SHOFER_DEBUG

#ifdef SHOFER_DEBUG
#define LOG(format, ...)	klog(KERN_DEBUG, format,  ##__VA_ARGS__)
#else /* !SHOFER_DEBUG */
#warning Debug not activated
#define LOG(format, ...)
#endif /* SHOFER_DEBUG */

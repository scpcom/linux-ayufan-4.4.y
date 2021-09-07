#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/net.h>

struct stub_device {
	struct usb_interface *interface;
	struct list_head list;

	struct usbip_device ud;
	__u32 devid;

	spinlock_t priv_lock;
	struct list_head priv_init;
	struct list_head priv_tx;
	struct list_head priv_free;

	struct list_head unlink_tx;
	struct list_head unlink_free;

	wait_queue_head_t tx_waitq;
#ifdef MY_ABC_HERE
	wait_queue_head_t rx_waitq;
#endif
};

struct stub_priv {
	unsigned long seqnum;
	struct list_head list;
	struct stub_device *sdev;
	struct urb *urb;

	int unlinking;
};

struct stub_unlink {
	unsigned long seqnum;
	struct list_head list;
	__u32 status;
};

extern struct kmem_cache *stub_priv_cache;

void stub_complete(struct urb *);
void stub_tx_loop(struct usbip_task *);

extern struct usb_driver stub_driver;

void stub_rx_loop(struct usbip_task *);
void stub_enqueue_ret_unlink(struct stub_device *, __u32, __u32);
#ifdef MY_ABC_HERE
int syno_socket_check(struct usbip_device *ud);
#endif

#ifdef MY_ABC_HERE
int del_match_busid(char *busid);
#endif
int match_busid(const char *busid);
void stub_device_cleanup_urbs(struct stub_device *sdev);

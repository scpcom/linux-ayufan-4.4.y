#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __VHCI_COMMON_H
#define __VHCI_COMMON_H

#include <linux/version.h>
#include <linux/usb.h>
#include <asm/byteorder.h>
#include <net/sock.h>

#ifdef CONFIG_USB_DEBUG

#define usbip_udbg(fmt, args...)					\
	do {								\
		printk(KERN_DEBUG "%-10s:(%s,%d) %s: " fmt,		\
			(in_interrupt() ? "interrupt" : (current)->comm),\
			__FILE__, __LINE__, __func__, ##args);		\
	} while (0)

#else   

#define usbip_udbg(fmt, args...)		do { } while (0)

#endif  

enum {
	usbip_debug_xmit	= (1 << 0),
	usbip_debug_sysfs	= (1 << 1),
	usbip_debug_urb		= (1 << 2),
	usbip_debug_eh		= (1 << 3),

	usbip_debug_stub_cmp	= (1 << 8),
	usbip_debug_stub_dev	= (1 << 9),
	usbip_debug_stub_rx	= (1 << 10),
	usbip_debug_stub_tx	= (1 << 11),

	usbip_debug_vhci_rh	= (1 << 8),
	usbip_debug_vhci_hc	= (1 << 9),
	usbip_debug_vhci_rx	= (1 << 10),
	usbip_debug_vhci_tx	= (1 << 11),
	usbip_debug_vhci_sysfs  = (1 << 12)
};

#define usbip_dbg_flag_xmit	(usbip_debug_flag & usbip_debug_xmit)
#define usbip_dbg_flag_vhci_rh	(usbip_debug_flag & usbip_debug_vhci_rh)
#define usbip_dbg_flag_vhci_hc	(usbip_debug_flag & usbip_debug_vhci_hc)
#define usbip_dbg_flag_vhci_rx	(usbip_debug_flag & usbip_debug_vhci_rx)
#define usbip_dbg_flag_vhci_tx	(usbip_debug_flag & usbip_debug_vhci_tx)
#define usbip_dbg_flag_stub_rx	(usbip_debug_flag & usbip_debug_stub_rx)
#define usbip_dbg_flag_stub_tx	(usbip_debug_flag & usbip_debug_stub_tx)
#define usbip_dbg_flag_vhci_sysfs   (usbip_debug_flag & usbip_debug_vhci_sysfs)

extern unsigned long usbip_debug_flag;
extern struct device_attribute dev_attr_usbip_debug;

#define usbip_dbg_with_flag(flag, fmt, args...)		\
	do {						\
		if (flag & usbip_debug_flag)		\
			usbip_udbg(fmt , ##args);		\
	} while (0)

#define usbip_dbg_sysfs(fmt, args...)		\
	usbip_dbg_with_flag(usbip_debug_sysfs, fmt , ##args)
#define usbip_dbg_xmit(fmt, args...)		\
	usbip_dbg_with_flag(usbip_debug_xmit, fmt , ##args)
#define usbip_dbg_urb(fmt, args...)		\
	usbip_dbg_with_flag(usbip_debug_urb, fmt , ##args)
#define usbip_dbg_eh(fmt, args...)		\
	usbip_dbg_with_flag(usbip_debug_eh, fmt , ##args)

#define usbip_dbg_vhci_rh(fmt, args...)	\
	usbip_dbg_with_flag(usbip_debug_vhci_rh, fmt , ##args)
#define usbip_dbg_vhci_hc(fmt, args...)	\
	usbip_dbg_with_flag(usbip_debug_vhci_hc, fmt , ##args)
#define usbip_dbg_vhci_rx(fmt, args...)	\
	usbip_dbg_with_flag(usbip_debug_vhci_rx, fmt , ##args)
#define usbip_dbg_vhci_tx(fmt, args...)	\
	usbip_dbg_with_flag(usbip_debug_vhci_tx, fmt , ##args)
#define usbip_dbg_vhci_sysfs(fmt, args...)	\
	usbip_dbg_with_flag(usbip_debug_vhci_sysfs, fmt , ##args)

#define usbip_dbg_stub_cmp(fmt, args...)	\
	usbip_dbg_with_flag(usbip_debug_stub_cmp, fmt , ##args)
#define usbip_dbg_stub_rx(fmt, args...)	\
	usbip_dbg_with_flag(usbip_debug_stub_rx, fmt , ##args)
#define usbip_dbg_stub_tx(fmt, args...)	\
	usbip_dbg_with_flag(usbip_debug_stub_tx, fmt , ##args)

#define usbip_uerr(fmt, args...)					\
	do {								\
		printk(KERN_ERR "%-10s: ***ERROR*** (%s,%d) %s: " fmt,	\
			(in_interrupt() ? "interrupt" : (current)->comm),\
			__FILE__, __LINE__, __func__, ##args);	\
	} while (0)

#define usbip_uinfo(fmt, args...)				\
	do {							\
		printk(KERN_INFO "usbip: " fmt , ## args);	\
	} while (0)

struct usbip_header_basic {
#define USBIP_CMD_SUBMIT	0x0001
#define USBIP_CMD_UNLINK	0x0002
#define USBIP_RET_SUBMIT	0x0003
#define USBIP_RET_UNLINK	0x0004

#ifdef MY_ABC_HERE
#define USBIP_RESET_DEV     0xFFFF
#endif
	__u32 command;

	__u32 seqnum;

	__u32 devid;

#define USBIP_DIR_OUT	0
#define USBIP_DIR_IN 	1
	__u32 direction;
	__u32 ep;      
} __attribute__ ((packed));

struct usbip_header_cmd_submit {
	 
	__u32 transfer_flags;

	__s32 transfer_buffer_length;

	__s32 start_frame;

	__s32 number_of_packets;

	__s32 interval;

	unsigned char setup[8];
} __attribute__ ((packed));

struct usbip_header_ret_submit {
	__s32 status;
	__s32 actual_length;  
	__s32 start_frame;  
	__s32 number_of_packets;   
	__s32 error_count;  
} __attribute__ ((packed));

struct usbip_header_cmd_unlink {
	__u32 seqnum;  
} __attribute__ ((packed));

struct usbip_header_ret_unlink {
	__s32 status;
} __attribute__ ((packed));

struct usbip_iso_packet_descriptor {
	__u32 offset;
	__u32 length;             
	__u32 actual_length;
	__u32 status;
} __attribute__ ((packed));

struct usbip_header {
	struct usbip_header_basic base;

	union {
		struct usbip_header_cmd_submit	cmd_submit;
		struct usbip_header_ret_submit	ret_submit;
		struct usbip_header_cmd_unlink	cmd_unlink;
		struct usbip_header_ret_unlink	ret_unlink;
	} u;
} __attribute__ ((packed));

int usbip_xmit(int, struct socket *, char *, int, int);
int usbip_sendmsg(struct socket *, struct msghdr *, int);

static inline int interface_to_busnum(struct usb_interface *interface)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	return udev->bus->busnum;
}

static inline int interface_to_devnum(struct usb_interface *interface)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	return udev->devnum;
}

static inline int interface_to_infnum(struct usb_interface *interface)
{
	return interface->cur_altsetting->desc.bInterfaceNumber;
}

#if 0
int setnodelay(struct socket *);
int setquickack(struct socket *);
int setkeepalive(struct socket *socket);
void setreuse(struct socket *);
#endif

struct socket *sockfd_to_socket(unsigned int);
int set_sockaddr(struct socket *socket, struct sockaddr_storage *ss);

void usbip_dump_urb(struct urb *purb);
void usbip_dump_header(struct usbip_header *pdu);

struct usbip_device;

struct usbip_task {
	struct task_struct *thread;
	struct completion thread_done;
	char *name;
	void (*loop_ops)(struct usbip_task *);
};

enum usbip_side {
	USBIP_VHCI,
	USBIP_STUB,
};

enum usbip_status {
	 
	SDEV_ST_AVAILABLE = 0x01,
	 
	SDEV_ST_USED,
	 
	SDEV_ST_ERROR,

	VDEV_ST_NULL,
	 
	VDEV_ST_NOTASSIGNED,
	VDEV_ST_USED,
	VDEV_ST_ERROR
};

struct usbip_device {
	enum usbip_side side;

	enum usbip_status status;

	spinlock_t lock;

	struct socket *tcp_socket;
#ifdef MY_ABC_HERE
	unsigned int sockfd;
	long ideal_time;
	struct timespec get_socket_time;
	struct timer_list socket_timer;
#endif

	struct usbip_task tcp_rx;
	struct usbip_task tcp_tx;

#define USBIP_EH_SHUTDOWN	(1 << 0)
#define USBIP_EH_BYE		(1 << 1)
#define USBIP_EH_RESET		(1 << 2)
#define USBIP_EH_UNUSABLE	(1 << 3)
#ifdef MY_ABC_HERE
#define USBIP_EH_CLOSE_SOCKET	(1 << 4)
#endif

#define SDEV_EVENT_REMOVED   (USBIP_EH_SHUTDOWN | USBIP_EH_RESET | USBIP_EH_BYE)
#define	SDEV_EVENT_DOWN		(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#ifdef MY_ABC_HERE
#define	SDEV_EVENT_ERROR_TCP	(USBIP_EH_CLOSE_SOCKET)
#else
#define	SDEV_EVENT_ERROR_TCP	(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#endif
#define	SDEV_EVENT_ERROR_SUBMIT	(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#define	SDEV_EVENT_ERROR_MALLOC	(USBIP_EH_SHUTDOWN | USBIP_EH_UNUSABLE)

#define	VDEV_EVENT_REMOVED	(USBIP_EH_SHUTDOWN | USBIP_EH_BYE)
#define	VDEV_EVENT_DOWN		(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#define	VDEV_EVENT_ERROR_TCP	(USBIP_EH_SHUTDOWN | USBIP_EH_RESET)
#define	VDEV_EVENT_ERROR_MALLOC	(USBIP_EH_SHUTDOWN | USBIP_EH_UNUSABLE)

	unsigned long event;
	struct usbip_task eh;
	wait_queue_head_t eh_waitq;

	struct eh_ops {
		void (*shutdown)(struct usbip_device *);
		void (*reset)(struct usbip_device *);
		void (*unusable)(struct usbip_device *);
#ifdef MY_ABC_HERE
		void (*close_connection)(struct usbip_device *);
		void (*cleanup_urb)(struct usbip_device *);
#endif
	} eh_ops;
};

void usbip_task_init(struct usbip_task *ut, char *,
				void (*loop_ops)(struct usbip_task *));

int usbip_start_threads(struct usbip_device *ud);
void usbip_stop_threads(struct usbip_device *ud);
int usbip_thread(void *param);

void usbip_pack_pdu(struct usbip_header *pdu, struct urb *urb, int cmd,
								int pack);

void usbip_header_correct_endian(struct usbip_header *pdu, int send);
 
int usbip_recv_xbuff(struct usbip_device *ud, struct urb *urb);
 
int usbip_recv_iso(struct usbip_device *ud, struct urb *urb);
void *usbip_alloc_iso_desc_pdu(struct urb *urb, ssize_t *bufflen);
#ifdef MY_ABC_HERE
void syno_usbip_shutdown_connection(struct usbip_device *ud);
void syno_usbip_timer_timeout(unsigned long param);

#define SYNO_USBIP_CONNECTION_IDEALCHECK	5*HZ;  
#endif

int usbip_start_eh(struct usbip_device *ud);
void usbip_stop_eh(struct usbip_device *ud);
void usbip_event_add(struct usbip_device *ud, unsigned long event);
int usbip_event_happened(struct usbip_device *ud);

#ifdef MY_ABC_HERE
int syno_usbip_event_happened(struct usbip_device *ud);
#endif

#endif

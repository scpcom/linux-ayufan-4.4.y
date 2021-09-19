#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/usb.h>
#include <linux/usb/ch11.h>
#include <linux/usb/hcd.h>
#include "usb.h"

#ifdef MY_ABC_HERE
#define SYNO_CONNECT_BOUNCE 0x400
#endif  

struct usb_hub {
	struct device		*intfdev;	 
	struct usb_device	*hdev;
	struct kref		kref;
	struct urb		*urb;		 

	u8			(*buffer)[8];
	union {
		struct usb_hub_status	hub;
		struct usb_port_status	port;
	}			*status;	 
	struct mutex		status_mutex;	 

	int			error;		 
	int			nerrors;	 

	struct list_head	event_list;	 
	unsigned long		event_bits[1];	 
	unsigned long		change_bits[1];	 
	unsigned long		busy_bits[1];	 
	unsigned long		removed_bits[1];  
	unsigned long		wakeup_bits[1];	 
#if defined(CONFIG_USB_ETRON_HUB)
	unsigned long		bot_mode_bits[1];
#endif  

#if USB_MAXCHILDREN > 31  
#error event_bits[] is too short!
#endif

	struct usb_hub_descriptor *descriptor;	 
	struct usb_tt		tt;		 

	unsigned		mA_per_port;	 
#ifdef	CONFIG_PM
	unsigned		wakeup_enabled_descendants;
#endif

	unsigned		limited_power:1;
	unsigned		quiescing:1;
	unsigned		disconnected:1;

	unsigned		quirk_check_port_auto_suspend:1;

	unsigned		has_indicators:1;
	u8			indicator[USB_MAXCHILDREN];
	struct delayed_work	leds;
	struct delayed_work	init_work;
	struct usb_port		**ports;

#ifdef MY_ABC_HERE
	struct timer_list	ups_discon_flt_timer;
	int			ups_discon_flt_port;
	unsigned long		ups_discon_flt_last;  
#define SYNO_UPS_DISCON_FLT_STATUS_NONE			0
#define SYNO_UPS_DISCON_FLT_STATUS_DEFERRED		1
#define SYNO_UPS_DISCON_FLT_STATUS_TIMEOUT		2
	unsigned int		ups_discon_flt_status;
#endif  
};

struct usb_port {
	struct usb_device *child;
	struct device dev;
	struct dev_state *port_owner;
	enum usb_port_connect_type connect_type;
	u8 portnum;
	unsigned power_is_on:1;
	unsigned did_runtime_put:1;
#if defined (MY_DEF_HERE)
	unsigned int power_cycle_counter;
#endif  
#ifdef MY_DEF_HERE
#define SYNO_USB_PORT_CASTRATED_XHC 0x01
	unsigned int flag;
#endif  
#ifdef MY_DEF_HERE
	unsigned syno_vbus_gpp;
#endif  
};
#if defined (MY_DEF_HERE)
#define SYNO_POWER_CYCLE_TRIES	(3)
#endif  

#define to_usb_port(_dev) \
	container_of(_dev, struct usb_port, dev)

extern int usb_hub_create_port_device(struct usb_hub *hub,
		int port1);
extern void usb_hub_remove_port_device(struct usb_hub *hub,
		int port1);
extern int usb_hub_set_port_power(struct usb_device *hdev,
		int port1, bool set);
extern struct usb_hub *usb_hub_to_struct_hub(struct usb_device *hdev);
extern int hub_port_debounce(struct usb_hub *hub, int port1,
		bool must_be_connected);
extern int usb_clear_port_feature(struct usb_device *hdev,
		int port1, int feature);

static inline int hub_port_debounce_be_connected(struct usb_hub *hub,
		int port1)
{
	return hub_port_debounce(hub, port1, true);
}

static inline int hub_port_debounce_be_stable(struct usb_hub *hub,
		int port1)
{
	return hub_port_debounce(hub, port1, false);
}

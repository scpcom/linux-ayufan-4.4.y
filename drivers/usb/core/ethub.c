#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * USB hub driver.
 *
 * (C) Copyright 1999 Linus Torvalds
 * (C) Copyright 1999 Johannes Erdfelt
 * (C) Copyright 1999 Gregory P. Smith
 * (C) Copyright 2001 Brad Hards (bhards@bigpond.net.au)
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/completion.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/usb.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/hcd.h>
#include <linux/usb/quirks.h>
#ifdef MY_ABC_HERE
#include <linux/usb/syno_quirks.h>
#endif /* MY_ABC_HERE */
#include <linux/usb/storage.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/pm_qos.h>

#include <asm/uaccess.h>
#include <asm/byteorder.h>

#include "hub.h"

/* Protect struct usb_device->state and ->children members
 * Note: Both are also protected by ->dev.sem, except that ->state can
 * change to USB_STATE_NOTATTACHED even when the semaphore isn't held. */
static DEFINE_SPINLOCK(device_state_lock);

#ifdef MY_ABC_HERE
extern struct mutex hub_init_mutex;
#endif /* MY_ABC_HERE */

/* workqueue to process hub events */
static struct workqueue_struct *hub_wq;
static void hub_event(struct work_struct *work);
static const char ethub_name[] = "ethub";

#define HUB_DEBOUNCE_TIMEOUT	2000
#define HUB_DEBOUNCE_STEP	  25
#define HUB_DEBOUNCE_STABLE	 100

static void hub_release(struct kref *kref);
static int usb_reset_and_verify_device(struct usb_device *udev);

static inline char *portspeed(struct usb_hub *hub, int portstatus)
{
	if (hub_is_superspeed(hub->hdev))
		return "5.0 Gb/s";
	if (portstatus & USB_PORT_STAT_HIGH_SPEED)
		return "480 Mb/s";
	else if (portstatus & USB_PORT_STAT_LOW_SPEED)
		return "1.5 Mb/s";
	else
		return "12 Mb/s";
}

/* USB 2.0 spec Section 11.24.4.5 */
static int get_hub_descriptor(struct usb_device *hdev, void *data)
{
	int i, ret, size;
	unsigned dtype;

	if (hub_is_superspeed(hdev)) {
		dtype = USB_DT_SS_HUB;
		size = USB_DT_SS_HUB_SIZE;
	} else {
		dtype = USB_DT_HUB;
		size = sizeof(struct usb_hub_descriptor);
	}

	for (i = 0; i < 3; i++) {
		ret = usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN | USB_RT_HUB,
			dtype << 8, 0, data, size,
			USB_CTRL_GET_TIMEOUT);
		if (ret >= (USB_DT_HUB_NONVAR_SIZE + 2))
			return ret;
	}
	return -EINVAL;
}

/*
 * USB 2.0 spec Section 11.24.2.1
 */
static int clear_hub_feature(struct usb_device *hdev, int feature)
{
	return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
		USB_REQ_CLEAR_FEATURE, USB_RT_HUB, feature, 0, NULL, 0, 1000);
}

/*
 * USB 2.0 spec Section 11.24.2.13
 */
static int set_port_feature(struct usb_device *hdev, int port1, int feature)
{
	return usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
		USB_REQ_SET_FEATURE, USB_RT_PORT, feature, port1,
		NULL, 0, 1000);
}

static char *to_led_name(int selector)
{
	switch (selector) {
	case HUB_LED_AMBER:
		return "amber";
	case HUB_LED_GREEN:
		return "green";
	case HUB_LED_OFF:
		return "off";
	case HUB_LED_AUTO:
		return "auto";
	default:
		return "??";
	}
}

/*
 * USB 2.0 spec Section 11.24.2.7.1.10 and table 11-7
 * for info about using port indicators
 */
static void set_port_led(struct usb_hub *hub, int port1, int selector)
{
	struct usb_port *port_dev = hub->ports[port1 - 1];
	int status;

	status = set_port_feature(hub->hdev, (selector << 8) | port1,
			USB_PORT_FEAT_INDICATOR);
	dev_dbg(&port_dev->dev, "indicator %s status %d\n",
		to_led_name(selector), status);
}

#define	LED_CYCLE_PERIOD	((2*HZ)/3)

static void led_work(struct work_struct *work)
{
	struct usb_hub		*hub =
		container_of(work, struct usb_hub, leds.work);
	struct usb_device	*hdev = hub->hdev;
	unsigned		i;
	unsigned		changed = 0;
	int			cursor = -1;

	if (hdev->state != USB_STATE_CONFIGURED || hub->quiescing)
		return;

	for (i = 0; i < hdev->maxchild; i++) {
		unsigned	selector, mode;

		/* 30%-50% duty cycle */

		switch (hub->indicator[i]) {
		/* cycle marker */
		case INDICATOR_CYCLE:
			cursor = i;
			selector = HUB_LED_AUTO;
			mode = INDICATOR_AUTO;
			break;
		/* blinking green = sw attention */
		case INDICATOR_GREEN_BLINK:
			selector = HUB_LED_GREEN;
			mode = INDICATOR_GREEN_BLINK_OFF;
			break;
		case INDICATOR_GREEN_BLINK_OFF:
			selector = HUB_LED_OFF;
			mode = INDICATOR_GREEN_BLINK;
			break;
		/* blinking amber = hw attention */
		case INDICATOR_AMBER_BLINK:
			selector = HUB_LED_AMBER;
			mode = INDICATOR_AMBER_BLINK_OFF;
			break;
		case INDICATOR_AMBER_BLINK_OFF:
			selector = HUB_LED_OFF;
			mode = INDICATOR_AMBER_BLINK;
			break;
		/* blink green/amber = reserved */
		case INDICATOR_ALT_BLINK:
			selector = HUB_LED_GREEN;
			mode = INDICATOR_ALT_BLINK_OFF;
			break;
		case INDICATOR_ALT_BLINK_OFF:
			selector = HUB_LED_AMBER;
			mode = INDICATOR_ALT_BLINK;
			break;
		default:
			continue;
		}
		if (selector != HUB_LED_AUTO)
			changed = 1;
		set_port_led(hub, i + 1, selector);
		hub->indicator[i] = mode;
	}
	if (changed)
		queue_delayed_work(system_power_efficient_wq,
				&hub->leds, LED_CYCLE_PERIOD);
}

/* use a short timeout for hub/port status fetches */
#define	USB_STS_TIMEOUT		500
#define	USB_STS_RETRIES		3

/*
 * USB 2.0 spec Section 11.24.2.6
 */
static int get_hub_status(struct usb_device *hdev,
		struct usb_hub_status *data)
{
	int i, status = -ETIMEDOUT;

	for (i = 0; i < USB_STS_RETRIES &&
			(status == -ETIMEDOUT || status == -EPIPE); i++) {
		status = usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
			USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_HUB, 0, 0,
			data, sizeof(*data), USB_STS_TIMEOUT);
	}
	return status;
}

/*
 * USB 2.0 spec Section 11.24.2.7
 */
static int get_port_status(struct usb_device *hdev, int port1,
		struct usb_port_status *data)
{
	int i, status = -ETIMEDOUT;

	for (i = 0; i < USB_STS_RETRIES &&
			(status == -ETIMEDOUT || status == -EPIPE); i++) {
		status = usb_control_msg(hdev, usb_rcvctrlpipe(hdev, 0),
			USB_REQ_GET_STATUS, USB_DIR_IN | USB_RT_PORT, 0, port1,
			data, sizeof(*data), USB_STS_TIMEOUT);
	}
	return status;
}

static int hub_port_status(struct usb_hub *hub, int port1,
		u16 *status, u16 *change)
{
	int ret;

	mutex_lock(&hub->status_mutex);
	ret = get_port_status(hub->hdev, port1, &hub->status->port);
	if (ret < 4) {
		if (ret != -ENODEV)
			dev_err(hub->intfdev,
				"%s failed (err = %d)\n", __func__, ret);
		if (ret >= 0)
			ret = -EIO;
	} else {
		*status = le16_to_cpu(hub->status->port.wPortStatus);
		*change = le16_to_cpu(hub->status->port.wPortChange);

		ret = 0;
	}
	mutex_unlock(&hub->status_mutex);
	return ret;
}

static void kick_hub_wq(struct usb_hub *hub)
{
	struct usb_interface *intf;

	if (hub->disconnected || work_pending(&hub->events))
		return;

	/*
	 * Suppress autosuspend until the event is proceed.
	 *
	 * Be careful and make sure that the symmetric operation is
	 * always called. We are here only when there is no pending
	 * work for this hub. Therefore put the interface either when
	 * the new work is called or when it is canceled.
	 */
	intf = to_usb_interface(hub->intfdev);
	usb_autopm_get_interface_no_resume(intf);
	kref_get(&hub->kref);

	if (queue_work(hub_wq, &hub->events))
		return;

	/* the work has already been scheduled */
	usb_autopm_put_interface_async(intf);
	kref_put(&hub->kref, hub_release);
}

void ethub_usb_kick_hub_wq(struct usb_device *hdev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);

	if (hub)
		kick_hub_wq(hub);
}

/*
 * Let the USB core know that a USB 3.0 device has sent a Function Wake Device
 * Notification, which indicates it had initiated remote wakeup.
 *
 * USB 3.0 hubs do not report the port link state change from U3 to U0 when the
 * device initiates resume, so the USB core will not receive notice of the
 * resume through the normal hub interrupt URB.
 */
void ethub_usb_wakeup_notification(struct usb_device *hdev,
		unsigned int portnum)
{
	struct usb_hub *hub;

	if (!hdev)
		return;

	hub = usb_hub_to_struct_hub(hdev);
	if (hub) {
		set_bit(portnum, hub->wakeup_bits);
		kick_hub_wq(hub);
	}
}

void usb_run_bot_mode_notification(struct usb_device *hdev,
		unsigned int portnum)
{
	struct usb_hub *hub;

	if (!hdev)
		return;

	hub = usb_hub_to_struct_hub(hdev);
	if (hub) {
		set_bit(portnum, hub->bot_mode_bits);
		set_bit(portnum, hub->change_bits);
		kick_hub_wq(hub);
	}
}
EXPORT_SYMBOL_GPL(usb_run_bot_mode_notification);

/* completion function, fires on port status changes and various faults */
static void hub_irq(struct urb *urb)
{
	struct usb_hub *hub = urb->context;
	int status = urb->status;
	unsigned i;
	unsigned long bits;

	switch (status) {
	case -ENOENT:		/* synchronous unlink */
	case -ECONNRESET:	/* async unlink */
	case -ESHUTDOWN:	/* hardware going away */
		return;

	default:		/* presumably an error */
		/* Cause a hub reset after 10 consecutive errors */
		dev_dbg(hub->intfdev, "transfer --> %d\n", status);
		if ((++hub->nerrors < 10) || hub->error)
			goto resubmit;
		hub->error = status;
		/* FALL THROUGH */

	/* let hub_wq handle things */
	case 0:			/* we got data:  port status changed */
		bits = 0;
		for (i = 0; i < urb->actual_length; ++i)
			bits |= ((unsigned long) ((*hub->buffer)[i]))
					<< (i*8);
		hub->event_bits[0] = bits;
		break;
	}

	hub->nerrors = 0;

	/* Something happened, let hub_wq figure it out */
	kick_hub_wq(hub);

resubmit:
	if (hub->quiescing)
		return;

	status = usb_submit_urb(hub->urb, GFP_ATOMIC);
	if (status != 0 && status != -ENODEV && status != -EPERM)
		dev_err(hub->intfdev, "resubmit --> %d\n", status);
}

static void hub_power_on(struct usb_hub *hub, bool do_delay)
{
	int port1;

	/* Enable power on each port.  Some hubs have reserved values
	 * of LPSM (> 2) in their descriptors, even though they are
	 * USB 2.0 hubs.  Some hubs do not implement port-power switching
	 * but only emulate it.  In all cases, the ports won't work
	 * unless we send these messages to the hub.
	 */
	if (hub_is_port_power_switchable(hub))
		dev_dbg(hub->intfdev, "enabling power on all ports\n");
	else
		dev_dbg(hub->intfdev, "trying to enable port power on "
				"non-switchable hub\n");
	for (port1 = 1; port1 <= hub->hdev->maxchild; port1++)
		if (test_bit(port1, hub->power_bits))
			set_port_feature(hub->hdev, port1, USB_PORT_FEAT_POWER);
		else
			usb_clear_port_feature(hub->hdev, port1,
						USB_PORT_FEAT_POWER);
	if (do_delay)
		msleep(hub_power_on_good_delay(hub));
}

static int hub_hub_status(struct usb_hub *hub,
		u16 *status, u16 *change)
{
	int ret;

	mutex_lock(&hub->status_mutex);
	ret = get_hub_status(hub->hdev, &hub->status->hub);
	if (ret < 0) {
		if (ret != -ENODEV)
			dev_err(hub->intfdev,
				"%s failed (err = %d)\n", __func__, ret);
	} else {
		*status = le16_to_cpu(hub->status->hub.wHubStatus);
		*change = le16_to_cpu(hub->status->hub.wHubChange);
		ret = 0;
	}
	mutex_unlock(&hub->status_mutex);
	return ret;
}

static int hub_set_port_link_state(struct usb_hub *hub, int port1,
			unsigned int link_status)
{
	return set_port_feature(hub->hdev,
			port1 | (link_status << 3),
			USB_PORT_FEAT_LINK_STATE);
}

/*
 * If USB 3.0 ports are placed into the Disabled state, they will no longer
 * detect any device connects or disconnects.  This is generally not what the
 * USB core wants, since it expects a disabled port to produce a port status
 * change event when a new device connects.
 *
 * Instead, set the link state to Disabled, wait for the link to settle into
 * that state, clear any change bits, and then put the port into the RxDetect
 * state.
 */
static int hub_usb3_port_disable(struct usb_hub *hub, int port1)
{
	int ret;
	int total_time;
	u16 portchange, portstatus;

	if (!hub_is_superspeed(hub->hdev))
		return -EINVAL;

	ret = hub_set_port_link_state(hub, port1, USB_SS_PORT_LS_SS_DISABLED);
	if (ret) {
		dev_err(hub->intfdev, "cannot disable port %d (err = %d)\n",
				port1, ret);
		return ret;
	}

	/* Wait for the link to enter the disabled state. */
	for (total_time = 0; ; total_time += HUB_DEBOUNCE_STEP) {
		ret = hub_port_status(hub, port1, &portstatus, &portchange);
		if (ret < 0)
			return ret;

		if ((portstatus & USB_PORT_STAT_LINK_STATE) ==
				USB_SS_PORT_LS_SS_DISABLED)
			break;
		if (total_time >= HUB_DEBOUNCE_TIMEOUT)
			break;
		msleep(HUB_DEBOUNCE_STEP);
	}
	if (total_time >= HUB_DEBOUNCE_TIMEOUT)
		dev_warn(&hub->ports[port1 - 1]->dev,
				"Could not disable after %d ms\n", total_time);

	return hub_set_port_link_state(hub, port1, USB_SS_PORT_LS_RX_DETECT);
}

static int hub_port_disable(struct usb_hub *hub, int port1, int set_state)
{
	struct usb_port *port_dev = hub->ports[port1 - 1];
	struct usb_device *hdev = hub->hdev;
	int ret = 0;

	if (port_dev->child && set_state)
		usb_set_device_state(port_dev->child, USB_STATE_NOTATTACHED);
	if (!hub->error) {
		if (hub_is_superspeed(hub->hdev))
			ret = hub_usb3_port_disable(hub, port1);
		else
			ret = usb_clear_port_feature(hdev, port1,
					USB_PORT_FEAT_ENABLE);
	}
	if (ret && ret != -ENODEV)
		dev_err(&port_dev->dev, "cannot disable (err = %d)\n", ret);
	return ret;
}

/*
 * Disable a port and mark a logical connect-change event, so that some
 * time later hub_wq will disconnect() any existing usb_device on the port
 * and will re-enumerate if there actually is a device attached.
 */
static void hub_port_logical_disconnect(struct usb_hub *hub, int port1)
{
	dev_dbg(&hub->ports[port1 - 1]->dev, "logical disconnect\n");
	hub_port_disable(hub, port1, 1);

	/* FIXME let caller ask to power down the port:
	 *  - some devices won't enumerate without a VBUS power cycle
	 *  - SRP saves power that way
	 *  - ... new call, TBD ...
	 * That's easy if this hub can switch power per-port, and
	 * hub_wq reactivates the port later (timer, SRP, etc).
	 * Powerdown must be optional, because of reset/DFU.
	 */

	set_bit(port1, hub->change_bits);
	kick_hub_wq(hub);
}

/**
 * usb_remove_device - disable a device's port on its parent hub
 * @udev: device to be disabled and removed
 * Context: @udev locked, must be able to sleep.
 *
 * After @udev's port has been disabled, hub_wq is notified and it will
 * see that the device has been disconnected.  When the device is
 * physically unplugged and something is plugged in, the events will
 * be received and processed normally.
 *
 * Return: 0 if successful. A negative error code otherwise.
 */
int ethub_usb_remove_device(struct usb_device *udev)
{
	struct usb_hub *hub;
	struct usb_interface *intf;

	if (!udev->parent)	/* Can't remove a root hub */
		return -EINVAL;
	hub = usb_hub_to_struct_hub(udev->parent);
	intf = to_usb_interface(hub->intfdev);

	usb_autopm_get_interface(intf);
	set_bit(udev->portnum, hub->removed_bits);
	hub_port_logical_disconnect(hub, udev->portnum);
	usb_autopm_put_interface(intf);
	return 0;
}

enum hub_activation_type {
	HUB_INIT, HUB_INIT2, HUB_INIT3,		/* INITs must come first */
	HUB_POST_RESET, HUB_RESUME, HUB_RESET_RESUME,
};

static void hub_init_func2(struct work_struct *ws);
static void hub_init_func3(struct work_struct *ws);
static bool hub_port_warm_reset_required(struct usb_hub *hub, int port1,
		u16 portstatus);

static void hub_activate(struct usb_hub *hub, enum hub_activation_type type)
{
	struct usb_device *hdev = hub->hdev;
	struct usb_hcd *hcd;
	int ret;
	int port1;
	int status;
	bool need_debounce_delay = false;
	unsigned delay;

	/* Continue a partial initialization */
	if (type == HUB_INIT2 || type == HUB_INIT3) {
		device_lock(hub->intfdev);

		/* Was the hub disconnected while we were waiting? */
		if (hub->disconnected) {
			device_unlock(hub->intfdev);
			kref_put(&hub->kref, hub_release);
			return;
		}
		if (type == HUB_INIT2)
			goto init2;
		goto init3;
	}
	kref_get(&hub->kref);

	/* The superspeed hub except for root hub has to use Hub Depth
	 * value as an offset into the route string to locate the bits
	 * it uses to determine the downstream port number. So hub driver
	 * should send a set hub depth request to superspeed hub after
	 * the superspeed hub is set configuration in initialization or
	 * reset procedure.
	 *
	 * After a resume, port power should still be on.
	 * For any other type of activation, turn it on.
	 */
	if (type != HUB_RESUME) {
		if (hdev->parent && hub_is_superspeed(hdev)) {
			ret = usb_control_msg(hdev, usb_sndctrlpipe(hdev, 0),
					HUB_SET_DEPTH, USB_RT_HUB,
					hdev->level - 1, 0, NULL, 0,
					USB_CTRL_SET_TIMEOUT);
			if (ret < 0)
				dev_err(hub->intfdev,
						"set hub depth failed\n");
		}

		/* Speed up system boot by using a delayed_work for the
		 * hub's initial power-up delays.  This is pretty awkward
		 * and the implementation looks like a home-brewed sort of
		 * setjmp/longjmp, but it saves at least 100 ms for each
		 * root hub (assuming usbcore is compiled into the kernel
		 * rather than as a module).  It adds up.
		 *
		 * This can't be done for HUB_RESUME or HUB_RESET_RESUME
		 * because for those activation types the ports have to be
		 * operational when we return.  In theory this could be done
		 * for HUB_POST_RESET, but it's easier not to.
		 */
		if (type == HUB_INIT) {
			delay = hub_power_on_good_delay(hub);

			hub_power_on(hub, false);
			INIT_DELAYED_WORK(&hub->init_work, hub_init_func2);
			queue_delayed_work(system_power_efficient_wq,
					&hub->init_work,
					msecs_to_jiffies(delay));

			/* Suppress autosuspend until init is done */
			usb_autopm_get_interface_no_resume(
					to_usb_interface(hub->intfdev));
			return;		/* Continues at init2: below */
		} else if (type == HUB_RESET_RESUME) {
			/* The internal host controller state for the hub device
			 * may be gone after a host power loss on system resume.
			 * Update the device's info so the HW knows it's a hub.
			 */
			hcd = bus_to_hcd(hdev->bus);
			if (hcd->driver->update_hub_device) {
				ret = hcd->driver->update_hub_device(hcd, hdev,
						&hub->tt, GFP_NOIO);
				if (ret < 0) {
					dev_err(hub->intfdev, "Host not "
							"accepting hub info "
							"update.\n");
					dev_err(hub->intfdev, "LS/FS devices "
							"and hubs may not work "
							"under this hub\n.");
				}
			}
			hub_power_on(hub, true);
		} else {
			hub_power_on(hub, true);
		}
	}
 init2:

	/*
	 * Check each port and set hub->change_bits to let hub_wq know
	 * which ports need attention.
	 */
	for (port1 = 1; port1 <= hdev->maxchild; ++port1) {
		struct usb_port *port_dev = hub->ports[port1 - 1];
		struct usb_device *udev = port_dev->child;
		u16 portstatus, portchange;

		portstatus = portchange = 0;
		status = hub_port_status(hub, port1, &portstatus, &portchange);
		if (udev || (portstatus & USB_PORT_STAT_CONNECTION))
			dev_dbg(&port_dev->dev, "status %04x change %04x\n",
					portstatus, portchange);

		/*
		 * After anything other than HUB_RESUME (i.e., initialization
		 * or any sort of reset), every port should be disabled.
		 * Unconnected ports should likewise be disabled (paranoia),
		 * and so should ports for which we have no usb_device.
		 */
		if ((portstatus & USB_PORT_STAT_ENABLE) && (
				type != HUB_RESUME ||
				!(portstatus & USB_PORT_STAT_CONNECTION) ||
				!udev ||
				udev->state == USB_STATE_NOTATTACHED)) {
			/*
			 * USB3 protocol ports will automatically transition
			 * to Enabled state when detect an USB3.0 device attach.
			 * Do not disable USB3 protocol ports, just pretend
			 * power was lost
			 */
			portstatus &= ~USB_PORT_STAT_ENABLE;
			if (!hub_is_superspeed(hdev))
				usb_clear_port_feature(hdev, port1,
						   USB_PORT_FEAT_ENABLE);
		}

		/* Clear status-change flags; we'll debounce later */
		if (portchange & USB_PORT_STAT_C_CONNECTION) {
			need_debounce_delay = true;
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_CONNECTION);
		}
		if (portchange & USB_PORT_STAT_C_ENABLE) {
			need_debounce_delay = true;
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_ENABLE);
		}
		if (portchange & USB_PORT_STAT_C_RESET) {
			need_debounce_delay = true;
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_RESET);
		}
		if ((portchange & USB_PORT_STAT_C_BH_RESET) &&
				hub_is_superspeed(hub->hdev)) {
			need_debounce_delay = true;
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_BH_PORT_RESET);
		}

		if (hub_port_warm_reset_required(hub, port1, portstatus)) {
			need_debounce_delay = true;
			set_bit(port1, hub->change_bits);
		}

		/* We can forget about a "removed" device when there's a
		 * physical disconnect or the connect status changes.
		 */
		if (!(portstatus & USB_PORT_STAT_CONNECTION) ||
				(portchange & USB_PORT_STAT_C_CONNECTION))
			clear_bit(port1, hub->removed_bits);

		if (!udev || udev->state == USB_STATE_NOTATTACHED) {
			/* Tell hub_wq to disconnect the device or
			 * check for a new connection
			 */
			if (udev || (portstatus & USB_PORT_STAT_CONNECTION) ||
			    (portstatus & USB_PORT_STAT_OVERCURRENT))
				set_bit(port1, hub->change_bits);

		} else if (portstatus & USB_PORT_STAT_ENABLE) {
			bool port_resumed = (portstatus &
					USB_PORT_STAT_LINK_STATE) ==
				USB_SS_PORT_LS_U0;
			/* The power session apparently survived the resume.
			 * If there was an overcurrent or suspend change
			 * (i.e., remote wakeup request), have hub_wq
			 * take care of it.  Look at the port link state
			 * for USB 3.0 hubs, since they don't have a suspend
			 * change bit, and they don't set the port link change
			 * bit on device-initiated resume.
			 */
			if (portchange || (hub_is_superspeed(hub->hdev) &&
						port_resumed))
				set_bit(port1, hub->change_bits);

		} else if (udev->persist_enabled) {
#ifdef CONFIG_PM
			udev->reset_resume = 1;
#endif
			/* Don't set the change_bits when the device
			 * was powered off.
			 */
			if (test_bit(port1, hub->power_bits))
				set_bit(port1, hub->change_bits);

		} else {
			/* The power session is gone; tell hub_wq */
			ethub_usb_set_device_state(udev, USB_STATE_NOTATTACHED);
			set_bit(port1, hub->change_bits);
		}
	}

	/* If no port-status-change flags were set, we don't need any
	 * debouncing.  If flags were set we can try to debounce the
	 * ports all at once right now, instead of letting hub_wq do them
	 * one at a time later on.
	 *
	 * If any port-status changes do occur during this delay, hub_wq
	 * will see them later and handle them normally.
	 */
	if (need_debounce_delay) {
		delay = HUB_DEBOUNCE_STABLE;

		/* Don't do a long sleep inside a workqueue routine */
		if (type == HUB_INIT2) {
			if (hub_is_superspeed(hdev) &&
				(hdev->descriptor.idVendor == cpu_to_le16(0x1c04) ||
				hdev->descriptor.idProduct == cpu_to_le16(0x0008))) {
				delay = 5000;
			}

			INIT_DELAYED_WORK(&hub->init_work, hub_init_func3);
			queue_delayed_work(system_power_efficient_wq,
					&hub->init_work,
					msecs_to_jiffies(delay));
			device_unlock(hub->intfdev);
			return;		/* Continues at init3: below */
		} else {
			msleep(delay);
		}
	}
 init3:
	hub->quiescing = 0;

	status = usb_submit_urb(hub->urb, GFP_NOIO);
	if (status < 0)
		dev_err(hub->intfdev, "activate --> %d\n", status);

	/* Scan all ports that need attention */
	kick_hub_wq(hub);

	/* Allow autosuspend if it was suppressed */
	if (type <= HUB_INIT3)
		usb_autopm_put_interface_async(to_usb_interface(hub->intfdev));

	if (type == HUB_INIT2 || type == HUB_INIT3)
		device_unlock(hub->intfdev);

	kref_put(&hub->kref, hub_release);
}

/* Implement the continuations for the delays above */
static void hub_init_func2(struct work_struct *ws)
{
	struct usb_hub *hub = container_of(ws, struct usb_hub, init_work.work);

	hub_activate(hub, HUB_INIT2);
}

static void hub_init_func3(struct work_struct *ws)
{
	struct usb_hub *hub = container_of(ws, struct usb_hub, init_work.work);

	hub_activate(hub, HUB_INIT3);
}

enum hub_quiescing_type {
	HUB_DISCONNECT, HUB_PRE_RESET, HUB_SUSPEND
};

static void hub_quiesce(struct usb_hub *hub, enum hub_quiescing_type type)
{
	struct usb_device *hdev = hub->hdev;
	int i;

	cancel_delayed_work_sync(&hub->init_work);

	/* hub_wq and related activity won't re-trigger */
	hub->quiescing = 1;

	if (type != HUB_SUSPEND) {
		/* Disconnect all the children */
		for (i = 0; i < hdev->maxchild; ++i) {
			if (hub->ports[i]->child)
				ethub_usb_disconnect(&hub->ports[i]->child);
		}
	}

	/* Stop hub_wq and related activity */
	usb_kill_urb(hub->urb);
	if (hub->has_indicators)
		cancel_delayed_work_sync(&hub->leds);
}

static void hub_pm_barrier_for_all_ports(struct usb_hub *hub)
{
	int i;

	for (i = 0; i < hub->hdev->maxchild; ++i)
		pm_runtime_barrier(&hub->ports[i]->dev);
}

/* caller has locked the hub device */
static int hub_pre_reset(struct usb_interface *intf)
{
	struct usb_hub *hub = usb_get_intfdata(intf);

	hub_quiesce(hub, HUB_PRE_RESET);
	hub->in_reset = 1;
	hub_pm_barrier_for_all_ports(hub);
	return 0;
}

/* caller has locked the hub device */
static int hub_post_reset(struct usb_interface *intf)
{
	struct usb_hub *hub = usb_get_intfdata(intf);

	hub->in_reset = 0;
	hub_pm_barrier_for_all_ports(hub);
	hub_activate(hub, HUB_POST_RESET);
	return 0;
}

static int hub_configure(struct usb_hub *hub,
	struct usb_endpoint_descriptor *endpoint)
{
	struct usb_hcd *hcd;
	struct usb_device *hdev = hub->hdev;
	struct device *hub_dev = hub->intfdev;
	u16 hubstatus, hubchange;
	u16 wHubCharacteristics;
	unsigned int pipe;
	int maxp, ret, i;
	char *message = "out of memory";
	unsigned unit_load;
	unsigned full_load;
	unsigned maxchild;

#ifdef MY_ABC_HERE
	mutex_lock(&hub_init_mutex);
#endif /* MY_ABC_HERE */

	hub->buffer = kmalloc(sizeof(*hub->buffer), GFP_KERNEL);
	if (!hub->buffer) {
		ret = -ENOMEM;
		goto fail;
	}

	hub->status = kmalloc(sizeof(*hub->status), GFP_KERNEL);
	if (!hub->status) {
		ret = -ENOMEM;
		goto fail;
	}
	mutex_init(&hub->status_mutex);

	hub->descriptor = kmalloc(sizeof(*hub->descriptor), GFP_KERNEL);
	if (!hub->descriptor) {
		ret = -ENOMEM;
		goto fail;
	}

	/* Request the entire hub descriptor.
	 * hub->descriptor can handle USB_MAXCHILDREN ports,
	 * but the hub can/will return fewer bytes here.
	 */
	ret = get_hub_descriptor(hdev, hub->descriptor);
	if (ret < 0) {
		message = "can't read hub descriptor";
		goto fail;
	} else if (hub->descriptor->bNbrPorts > USB_MAXCHILDREN) {
		message = "hub has too many ports!";
		ret = -ENODEV;
		goto fail;
	} else if (hub->descriptor->bNbrPorts == 0) {
		message = "hub doesn't have any ports!";
		ret = -ENODEV;
		goto fail;
	}

	maxchild = hub->descriptor->bNbrPorts;
	dev_info(hub_dev, "%d port%s detected\n", maxchild,
			(maxchild == 1) ? "" : "s");

	hub->ports = kzalloc(maxchild * sizeof(struct usb_port *), GFP_KERNEL);
	if (!hub->ports) {
		ret = -ENOMEM;
		goto fail;
	}

	wHubCharacteristics = le16_to_cpu(hub->descriptor->wHubCharacteristics);
	if (hub_is_superspeed(hdev)) {
		unit_load = 150;
		full_load = 900;
	} else {
		unit_load = 100;
		full_load = 500;
	}

	/* FIXME for USB 3.0, skip for now */
	if ((wHubCharacteristics & HUB_CHAR_COMPOUND) &&
			!(hub_is_superspeed(hdev))) {
		char	portstr[USB_MAXCHILDREN + 1];

		for (i = 0; i < maxchild; i++)
			portstr[i] = hub->descriptor->u.hs.DeviceRemovable
				    [((i + 1) / 8)] & (1 << ((i + 1) % 8))
				? 'F' : 'R';
		portstr[maxchild] = 0;
		dev_dbg(hub_dev, "compound device; port removable status: %s\n", portstr);
	} else
		dev_dbg(hub_dev, "standalone hub\n");

	switch (wHubCharacteristics & HUB_CHAR_LPSM) {
	case HUB_CHAR_COMMON_LPSM:
		dev_dbg(hub_dev, "ganged power switching\n");
		break;
	case HUB_CHAR_INDV_PORT_LPSM:
		dev_dbg(hub_dev, "individual port power switching\n");
		break;
	case HUB_CHAR_NO_LPSM:
	case HUB_CHAR_LPSM:
		dev_dbg(hub_dev, "no power switching (usb 1.0)\n");
		break;
	}

	switch (wHubCharacteristics & HUB_CHAR_OCPM) {
	case HUB_CHAR_COMMON_OCPM:
		dev_dbg(hub_dev, "global over-current protection\n");
		break;
	case HUB_CHAR_INDV_PORT_OCPM:
		dev_dbg(hub_dev, "individual port over-current protection\n");
		break;
	case HUB_CHAR_NO_OCPM:
	case HUB_CHAR_OCPM:
		dev_dbg(hub_dev, "no over-current protection\n");
		break;
	}

	spin_lock_init(&hub->tt.lock);
	INIT_LIST_HEAD(&hub->tt.clear_list);
	switch (hdev->descriptor.bDeviceProtocol) {
	case USB_HUB_PR_FS:
		break;
	case USB_HUB_PR_HS_SINGLE_TT:
		dev_dbg(hub_dev, "Single TT\n");
		hub->tt.hub = hdev;
		break;
	case USB_HUB_PR_HS_MULTI_TT:
		ret = usb_set_interface(hdev, 0, 1);
		if (ret == 0) {
			dev_dbg(hub_dev, "TT per port\n");
			hub->tt.multi = 1;
		} else
			dev_err(hub_dev, "Using single TT (err %d)\n",
				ret);
		hub->tt.hub = hdev;
		break;
	case USB_HUB_PR_SS:
		/* USB 3.0 hubs don't have a TT */
		break;
	default:
		dev_dbg(hub_dev, "Unrecognized hub protocol %d\n",
			hdev->descriptor.bDeviceProtocol);
		break;
	}

	/* Note 8 FS bit times == (8 bits / 12000000 bps) ~= 666ns */
	switch (wHubCharacteristics & HUB_CHAR_TTTT) {
	case HUB_TTTT_8_BITS:
		if (hdev->descriptor.bDeviceProtocol != 0) {
			hub->tt.think_time = 666;
			dev_dbg(hub_dev, "TT requires at most %d "
					"FS bit times (%d ns)\n",
				8, hub->tt.think_time);
		}
		break;
	case HUB_TTTT_16_BITS:
		hub->tt.think_time = 666 * 2;
		dev_dbg(hub_dev, "TT requires at most %d "
				"FS bit times (%d ns)\n",
			16, hub->tt.think_time);
		break;
	case HUB_TTTT_24_BITS:
		hub->tt.think_time = 666 * 3;
		dev_dbg(hub_dev, "TT requires at most %d "
				"FS bit times (%d ns)\n",
			24, hub->tt.think_time);
		break;
	case HUB_TTTT_32_BITS:
		hub->tt.think_time = 666 * 4;
		dev_dbg(hub_dev, "TT requires at most %d "
				"FS bit times (%d ns)\n",
			32, hub->tt.think_time);
		break;
	}

	/* probe() zeroes hub->indicator[] */
	if (wHubCharacteristics & HUB_CHAR_PORTIND) {
		hub->has_indicators = 1;
		dev_dbg(hub_dev, "Port indicators are supported\n");
	}

	dev_dbg(hub_dev, "power on to power good time: %dms\n",
		hub->descriptor->bPwrOn2PwrGood * 2);

	/* power budgeting mostly matters with bus-powered hubs,
	 * and battery-powered root hubs (may provide just 8 mA).
	 */
	ret = usb_get_status(hdev, USB_RECIP_DEVICE, 0, &hubstatus);
	if (ret) {
		message = "can't get hub status";
		goto fail;
	}
	hcd = bus_to_hcd(hdev->bus);
	if (hdev == hdev->bus->root_hub) {
		if (hcd->power_budget > 0)
			hdev->bus_mA = hcd->power_budget;
		else
			hdev->bus_mA = full_load * maxchild;
		if (hdev->bus_mA >= full_load)
			hub->mA_per_port = full_load;
		else {
			hub->mA_per_port = hdev->bus_mA;
			hub->limited_power = 1;
		}
	} else if ((hubstatus & (1 << USB_DEVICE_SELF_POWERED)) == 0) {
		int remaining = hdev->bus_mA -
			hub->descriptor->bHubContrCurrent;

		dev_dbg(hub_dev, "hub controller current requirement: %dmA\n",
			hub->descriptor->bHubContrCurrent);
		hub->limited_power = 1;

		if (remaining < maxchild * unit_load)
			dev_warn(hub_dev,
					"insufficient power available "
					"to use all downstream ports\n");
		hub->mA_per_port = unit_load;	/* 7.2.1 */

	} else {	/* Self-powered external hub */
		/* FIXME: What about battery-powered external hubs that
		 * provide less current per port? */
		hub->mA_per_port = full_load;
	}
	if (hub->mA_per_port < full_load)
		dev_dbg(hub_dev, "%umA bus power budget for each child\n",
				hub->mA_per_port);

	ret = hub_hub_status(hub, &hubstatus, &hubchange);
	if (ret < 0) {
		message = "can't get hub status";
		goto fail;
	}

	/* local power status reports aren't always correct */
	if (hdev->actconfig->desc.bmAttributes & USB_CONFIG_ATT_SELFPOWER)
		dev_dbg(hub_dev, "local power source is %s\n",
			(hubstatus & HUB_STATUS_LOCAL_POWER)
			? "lost (inactive)" : "good");

	if ((wHubCharacteristics & HUB_CHAR_OCPM) == 0)
		dev_dbg(hub_dev, "%sover-current condition exists\n",
			(hubstatus & HUB_STATUS_OVERCURRENT) ? "" : "no ");

	/* set up the interrupt endpoint
	 * We use the EP's maxpacket size instead of (PORTS+1+7)/8
	 * bytes as USB2.0[11.12.3] says because some hubs are known
	 * to send more data (and thus cause overflow). For root hubs,
	 * maxpktsize is defined in hcd.c's fake endpoint descriptors
	 * to be big enough for at least USB_MAXCHILDREN ports. */
	pipe = usb_rcvintpipe(hdev, endpoint->bEndpointAddress);
	maxp = usb_maxpacket(hdev, pipe, usb_pipeout(pipe));

	if (maxp > sizeof(*hub->buffer))
		maxp = sizeof(*hub->buffer);

	hub->urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!hub->urb) {
		ret = -ENOMEM;
		goto fail;
	}

	usb_fill_int_urb(hub->urb, hdev, pipe, *hub->buffer, maxp, hub_irq,
		hub, endpoint->bInterval);

	mutex_lock(&usb_port_peer_mutex);
	for (i = 0; i < maxchild; i++) {
		ret = usb_hub_create_port_device(hub, i + 1);
		if (ret < 0) {
			dev_err(hub->intfdev,
				"couldn't create port%d device.\n", i + 1);
			break;
		}
	}
	hdev->maxchild = i;
	for (i = 0; i < hdev->maxchild; i++) {
		struct usb_port *port_dev = hub->ports[i];

		pm_runtime_put(&port_dev->dev);
	}

	mutex_unlock(&usb_port_peer_mutex);
	if (ret < 0)
		goto fail;

	/* Update the HCD's internal representation of this hub before hub_wq
	 * starts getting port status changes for devices under the hub.
	 */
	if (hcd->driver->update_hub_device) {
		ret = hcd->driver->update_hub_device(hcd, hdev,
				&hub->tt, GFP_KERNEL);
		if (ret < 0) {
			message = "can't update HCD hub info";
			goto fail;
		}
	}
#ifdef MY_ABC_HERE
	mutex_unlock(&hub_init_mutex);
#endif /* MY_ABC_HERE */

	usb_hub_adjust_deviceremovable(hdev, hub->descriptor);

	hub_activate(hub, HUB_INIT);
	return 0;

fail:
	dev_err(hub_dev, "config failed, %s (err %d)\n",
			message, ret);
	/* hub_disconnect() frees urb and descriptor */
#ifdef MY_ABC_HERE
	mutex_unlock(&hub_init_mutex);
#endif /* MY_ABC_HERE */
	return ret;
}

static void hub_release(struct kref *kref)
{
	struct usb_hub *hub = container_of(kref, struct usb_hub, kref);

	usb_put_dev(hub->hdev);
	usb_put_intf(to_usb_interface(hub->intfdev));
	kfree(hub);
}

static unsigned highspeed_hubs;

static void hub_disconnect(struct usb_interface *intf)
{
	struct usb_hub *hub = usb_get_intfdata(intf);
	struct usb_device *hdev = interface_to_usbdev(intf);
	int port1;

	/*
	 * Stop adding new hub events. We do not want to block here and thus
	 * will not try to remove any pending work item.
	 */
	hub->disconnected = 1;
#ifdef MY_ABC_HERE
	del_timer_sync(&hub->ups_discon_flt_timer);
#endif /* MY_ABC_HERE */

	/* Disconnect all children and quiesce the hub */
	hub->error = 0;
	hub_quiesce(hub, HUB_DISCONNECT);

	mutex_lock(&usb_port_peer_mutex);

	/* Avoid races with recursively_mark_NOTATTACHED() */
	spin_lock_irq(&device_state_lock);
	port1 = hdev->maxchild;
	hdev->maxchild = 0;
	usb_set_intfdata(intf, NULL);
	spin_unlock_irq(&device_state_lock);

	for (; port1 > 0; --port1)
		usb_hub_remove_port_device(hub, port1);

	mutex_unlock(&usb_port_peer_mutex);

	if (hub->hdev->speed == USB_SPEED_HIGH)
		highspeed_hubs--;

	usb_free_urb(hub->urb);
	kfree(hub->ports);
	kfree(hub->descriptor);
	kfree(hub->status);
	kfree(hub->buffer);

	pm_suspend_ignore_children(&intf->dev, false);
	kref_put(&hub->kref, hub_release);
}

#ifdef MY_ABC_HERE
static void ups_discon_flt_work(unsigned long arg)
{
	struct usb_hub *hub = (struct usb_hub *) arg;
	set_bit(hub->ups_discon_flt_port, hub->change_bits);
	hub->ups_discon_flt_status = SYNO_UPS_DISCON_FLT_STATUS_TIMEOUT;
	kick_hub_wq(hub);
}
#endif /* MY_ABC_HERE */

static int hub_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_host_interface *desc;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_device *hdev;
	struct usb_hub *hub;

	desc = intf->cur_altsetting;
	hdev = interface_to_usbdev(intf);

	if (!usb_is_etron_hcd(hdev))
		return -ENODEV;

	/*
	 * Set default autosuspend delay as 0 to speedup bus suspend,
	 * based on the below considerations:
	 *
	 * - Unlike other drivers, the hub driver does not rely on the
	 *   autosuspend delay to provide enough time to handle a wakeup
	 *   event, and the submitted status URB is just to check future
	 *   change on hub downstream ports, so it is safe to do it.
	 *
	 * - The patch might cause one or more auto supend/resume for
	 *   below very rare devices when they are plugged into hub
	 *   first time:
	 *
	 *   	devices having trouble initializing, and disconnect
	 *   	themselves from the bus and then reconnect a second
	 *   	or so later
	 *
	 *   	devices just for downloading firmware, and disconnects
	 *   	themselves after completing it
	 *
	 *   For these quite rare devices, their drivers may change the
	 *   autosuspend delay of their parent hub in the probe() to one
	 *   appropriate value to avoid the subtle problem if someone
	 *   does care it.
	 *
	 * - The patch may cause one or more auto suspend/resume on
	 *   hub during running 'lsusb', but it is probably too
	 *   infrequent to worry about.
	 *
	 * - Change autosuspend delay of hub can avoid unnecessary auto
	 *   suspend timer for hub, also may decrease power consumption
	 *   of USB bus.
	 *
	 * - If user has indicated to prevent autosuspend by passing
	 *   usbcore.autosuspend = -1 then keep autosuspend disabled.
	 */
#ifdef CONFIG_PM
	if (hdev->dev.power.autosuspend_delay >= 0)
		pm_runtime_set_autosuspend_delay(&hdev->dev, 0);
#endif

	/* Hubs have proper suspend/resume support.  USB 3.0 device suspend is
	 * different from USB 2.0/1.1 device suspend, and unfortunately we
	 * don't support it yet.  So leave autosuspend disabled for USB 3.0
	 * external hubs for now.  Enable autosuspend for USB 3.0 roothubs,
	 * since that isn't a "real" hub.
	 */
	if (!hub_is_superspeed(hdev) || !hdev->parent)
		usb_enable_autosuspend(hdev);

	if (hdev->level == MAX_TOPO_LEVEL) {
		dev_err(&intf->dev,
			"Unsupported bus topology: hub nested too deep\n");
		return -E2BIG;
	}

	/* Some hubs have a subclass of 1, which AFAICT according to the */
	/*  specs is not defined, but it works */
	if ((desc->desc.bInterfaceSubClass != 0) &&
	    (desc->desc.bInterfaceSubClass != 1)) {
descriptor_error:
		dev_err(&intf->dev, "bad descriptor, ignoring hub\n");
		return -EIO;
	}

	/* Multiple endpoints? What kind of mutant ninja-hub is this? */
	if (desc->desc.bNumEndpoints != 1)
		goto descriptor_error;

	endpoint = &desc->endpoint[0].desc;

	/* If it's not an interrupt in endpoint, we'd better punt! */
	if (!usb_endpoint_is_int_in(endpoint))
		goto descriptor_error;

	/* We found a hub */
	dev_info(&intf->dev, "USB hub found\n");

	hub = kzalloc(sizeof(*hub), GFP_KERNEL);
	if (!hub) {
		dev_dbg(&intf->dev, "couldn't kmalloc hub struct\n");
		return -ENOMEM;
	}

	kref_init(&hub->kref);
	hub->intfdev = &intf->dev;
	hub->hdev = hdev;
	INIT_DELAYED_WORK(&hub->leds, led_work);
	INIT_DELAYED_WORK(&hub->init_work, NULL);
	INIT_WORK(&hub->events, hub_event);
	usb_get_intf(intf);
	usb_get_dev(hdev);

	usb_set_intfdata(intf, hub);
	intf->needs_remote_wakeup = 1;
	pm_suspend_ignore_children(&intf->dev, true);

	if (hdev->speed == USB_SPEED_HIGH)
		highspeed_hubs++;

#ifdef MY_ABC_HERE
	init_timer(&hub->ups_discon_flt_timer);
	hub->ups_discon_flt_timer.data = (unsigned long) hub;
	hub->ups_discon_flt_timer.function = ups_discon_flt_work;
	hub->ups_discon_flt_last = jiffies - 16 * HZ;
	hub->ups_discon_flt_status = SYNO_UPS_DISCON_FLT_STATUS_NONE;
#endif /* MY_ABC_HERE */

	if (hub_configure(hub, endpoint) >= 0)
		return 0;

	hub_disconnect(intf);
	return -ENODEV;
}

static int
hub_ioctl(struct usb_interface *intf, unsigned int code, void *user_data)
{
	struct usb_device *hdev = interface_to_usbdev(intf);
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);

	/* assert ifno == 0 (part of hub spec) */
	switch (code) {
	case USBDEVFS_HUB_PORTINFO: {
		struct usbdevfs_hub_portinfo *info = user_data;
		int i;

		spin_lock_irq(&device_state_lock);
		if (hdev->devnum <= 0)
			info->nports = 0;
		else {
			info->nports = hdev->maxchild;
			for (i = 0; i < info->nports; i++) {
				if (hub->ports[i]->child == NULL)
					info->port[i] = 0;
				else
					info->port[i] =
						hub->ports[i]->child->devnum;
			}
		}
		spin_unlock_irq(&device_state_lock);

		return info->nports + 1;
		}

	default:
		return -ENOSYS;
	}
}

static void recursively_mark_NOTATTACHED(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev);
	int i;

	for (i = 0; i < udev->maxchild; ++i) {
		if (hub->ports[i]->child)
			recursively_mark_NOTATTACHED(hub->ports[i]->child);
	}
	if (udev->state == USB_STATE_SUSPENDED)
		udev->active_duration -= jiffies;
	udev->state = USB_STATE_NOTATTACHED;
}

/**
 * usb_set_device_state - change a device's current state (usbcore, hcds)
 * @udev: pointer to device whose state should be changed
 * @new_state: new state value to be stored
 *
 * udev->state is _not_ fully protected by the device lock.  Although
 * most transitions are made only while holding the lock, the state can
 * can change to USB_STATE_NOTATTACHED at almost any time.  This
 * is so that devices can be marked as disconnected as soon as possible,
 * without having to wait for any semaphores to be released.  As a result,
 * all changes to any device's state must be protected by the
 * device_state_lock spinlock.
 *
 * Once a device has been added to the device tree, all changes to its state
 * should be made using this routine.  The state should _not_ be set directly.
 *
 * If udev->state is already USB_STATE_NOTATTACHED then no change is made.
 * Otherwise udev->state is set to new_state, and if new_state is
 * USB_STATE_NOTATTACHED then all of udev's descendants' states are also set
 * to USB_STATE_NOTATTACHED.
 */
void ethub_usb_set_device_state(struct usb_device *udev,
		enum usb_device_state new_state)
{
	unsigned long flags;
	int wakeup = -1;

	spin_lock_irqsave(&device_state_lock, flags);
	if (udev->state == USB_STATE_NOTATTACHED)
		;	/* do nothing */
	else if (new_state != USB_STATE_NOTATTACHED) {

		/* root hub wakeup capabilities are managed out-of-band
		 * and may involve silicon errata ... ignore them here.
		 */
		if (udev->parent) {
			if (udev->state == USB_STATE_SUSPENDED
					|| new_state == USB_STATE_SUSPENDED)
				;	/* No change to wakeup settings */
			else if (new_state == USB_STATE_CONFIGURED)
				wakeup = (udev->quirks &
					USB_QUIRK_IGNORE_REMOTE_WAKEUP) ? 0 :
					udev->actconfig->desc.bmAttributes &
					USB_CONFIG_ATT_WAKEUP;
			else
				wakeup = 0;
		}
		if (udev->state == USB_STATE_SUSPENDED &&
			new_state != USB_STATE_SUSPENDED)
			udev->active_duration -= jiffies;
		else if (new_state == USB_STATE_SUSPENDED &&
				udev->state != USB_STATE_SUSPENDED)
			udev->active_duration += jiffies;
		udev->state = new_state;
	} else
		recursively_mark_NOTATTACHED(udev);
	spin_unlock_irqrestore(&device_state_lock, flags);
	if (wakeup >= 0)
		device_set_wakeup_capable(&udev->dev, wakeup);
}

/*
 * Choose a device number.
 *
 * Device numbers are used as filenames in usbfs.  On USB-1.1 and
 * USB-2.0 buses they are also used as device addresses, however on
 * USB-3.0 buses the address is assigned by the controller hardware
 * and it usually is not the same as the device number.
 *
 * WUSB devices are simple: they have no hubs behind, so the mapping
 * device <-> virtual port number becomes 1:1. Why? to simplify the
 * life of the device connection logic in
 * drivers/usb/wusbcore/devconnect.c. When we do the initial secret
 * handshake we need to assign a temporary address in the unauthorized
 * space. For simplicity we use the first virtual port number found to
 * be free [drivers/usb/wusbcore/devconnect.c:wusbhc_devconnect_ack()]
 * and that becomes it's address [X < 128] or its unauthorized address
 * [X | 0x80].
 *
 * We add 1 as an offset to the one-based USB-stack port number
 * (zero-based wusb virtual port index) for two reasons: (a) dev addr
 * 0 is reserved by USB for default address; (b) Linux's USB stack
 * uses always #1 for the root hub of the controller. So USB stack's
 * port #1, which is wusb virtual-port #0 has address #2.
 *
 * Devices connected under xHCI are not as simple.  The host controller
 * supports virtualization, so the hardware assigns device addresses and
 * the HCD must setup data structures before issuing a set address
 * command to the hardware.
 */
static void choose_devnum(struct usb_device *udev)
{
	int		devnum;
	struct usb_bus	*bus = udev->bus;

	/* be safe when more hub events are proceed in parallel */
	mutex_lock(&bus->devnum_next_mutex);
	if (udev->wusb) {
		devnum = udev->portnum + 1;
		BUG_ON(test_bit(devnum, bus->devmap.devicemap));
	} else {
		/* Try to allocate the next devnum beginning at
		 * bus->devnum_next. */
		devnum = find_next_zero_bit(bus->devmap.devicemap, 128,
					    bus->devnum_next);
		if (devnum >= 128)
			devnum = find_next_zero_bit(bus->devmap.devicemap,
						    128, 1);
		bus->devnum_next = (devnum >= 127 ? 1 : devnum + 1);
	}
	if (devnum < 128) {
		set_bit(devnum, bus->devmap.devicemap);
		udev->devnum = devnum;
	}
	mutex_unlock(&bus->devnum_next_mutex);
}

static void release_devnum(struct usb_device *udev)
{
	if (udev->devnum > 0) {
		clear_bit(udev->devnum, udev->bus->devmap.devicemap);
		udev->devnum = -1;
	}
}

static void update_devnum(struct usb_device *udev, int devnum)
{
	/* The address for a WUSB device is managed by wusbcore. */
	if (!udev->wusb)
		udev->devnum = devnum;
}

static void hub_free_dev(struct usb_device *udev)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	/* Root hubs aren't real devices, so don't free HCD resources */
	if (hcd->driver->free_dev && udev->parent)
		hcd->driver->free_dev(hcd, udev);
}

static void hub_disconnect_children(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev);
	int i;

	/* Free up all the children before we remove this device */
	for (i = 0; i < udev->maxchild; i++) {
		if (hub->ports[i]->child)
			ethub_usb_disconnect(&hub->ports[i]->child);
	}
}

/**
 * usb_disconnect - disconnect a device (usbcore-internal)
 * @pdev: pointer to device being disconnected
 * Context: !in_interrupt ()
 *
 * Something got disconnected. Get rid of it and all of its children.
 *
 * If *pdev is a normal device then the parent hub must already be locked.
 * If *pdev is a root hub then the caller must hold the usb_bus_list_lock,
 * which protects the set of root hubs as well as the list of buses.
 *
 * Only hub drivers (including virtual root hub drivers for host
 * controllers) should ever call this.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 */
void ethub_usb_disconnect(struct usb_device **pdev)
{
	struct usb_port *port_dev = NULL;
	struct usb_device *udev = *pdev;
	struct usb_hub *hub = NULL;
	int port1 = 1;

	/* mark the device as inactive, so any further urb submissions for
	 * this device (and any of its children) will fail immediately.
	 * this quiesces everything except pending urbs.
	 */
	ethub_usb_set_device_state(udev, USB_STATE_NOTATTACHED);
	dev_info(&udev->dev, "USB disconnect, device number %d\n",
			udev->devnum);

	usb_lock_device(udev);

	hub_disconnect_children(udev);

	/* deallocate hcd/hardware state ... nuking all pending urbs and
	 * cleaning up all state associated with the current configuration
	 * so that the hardware is now fully quiesced.
	 */
	dev_dbg(&udev->dev, "unregistering device\n");
	usb_disable_device(udev, 0);
	usb_hcd_synchronize_unlinks(udev);

	if (udev->parent) {
		port1 = udev->portnum;
		hub = usb_hub_to_struct_hub(udev->parent);
		port_dev = hub->ports[port1 - 1];

		sysfs_remove_link(&udev->dev.kobj, "port");
		sysfs_remove_link(&port_dev->dev.kobj, "device");

		/*
		 * As usb_port_runtime_resume() de-references udev, make
		 * sure no resumes occur during removal
		 */
		if (!test_and_set_bit(port1, hub->child_usage_bits))
			pm_runtime_get_sync(&port_dev->dev);
	}

	usb_remove_ep_devs(&udev->ep0);
	usb_unlock_device(udev);

	/* Unregister the device.  The device driver is responsible
	 * for de-configuring the device and invoking the remove-device
	 * notifier chain (used by usbfs and possibly others).
	 */
	device_del(&udev->dev);

	/* Free the device number and delete the parent's children[]
	 * (or root_hub) pointer.
	 */
	release_devnum(udev);

	/* Avoid races with recursively_mark_NOTATTACHED() */
	spin_lock_irq(&device_state_lock);
	*pdev = NULL;
	spin_unlock_irq(&device_state_lock);

	if (port_dev && test_and_clear_bit(port1, hub->child_usage_bits))
		pm_runtime_put(&port_dev->dev);

	hub_free_dev(udev);

	put_device(&udev->dev);
}

#ifdef CONFIG_USB_ANNOUNCE_NEW_DEVICES
static void show_string(struct usb_device *udev, char *id, char *string)
{
	if (!string)
		return;
	dev_info(&udev->dev, "%s: %s\n", id, string);
}

static void announce_device(struct usb_device *udev)
{
	dev_info(&udev->dev, "New USB device found, idVendor=%04x, idProduct=%04x\n",
		le16_to_cpu(udev->descriptor.idVendor),
		le16_to_cpu(udev->descriptor.idProduct));
	dev_info(&udev->dev,
		"New USB device strings: Mfr=%d, Product=%d, SerialNumber=%d\n",
		udev->descriptor.iManufacturer,
		udev->descriptor.iProduct,
		udev->descriptor.iSerialNumber);
	show_string(udev, "Product", udev->product);
	show_string(udev, "Manufacturer", udev->manufacturer);
	show_string(udev, "SerialNumber", udev->serial);
}
#else
static inline void announce_device(struct usb_device *udev) { }
#endif

/**
 * usb_enumerate_device - Read device configs/intfs/otg (usbcore-internal)
 * @udev: newly addressed device (in ADDRESS state)
 *
 * This is only called by usb_new_device() and usb_authorize_device()
 * and FIXME -- all comments that apply to them apply here wrt to
 * environment.
 *
 * If the device is WUSB and not authorized, we don't attempt to read
 * the string descriptors, as they will be errored out by the device
 * until it has been authorized.
 *
 * Return: 0 if successful. A negative error code otherwise.
 */
static int usb_enumerate_device(struct usb_device *udev)
{
	int err = 0;
	int retry = 2;

	if (udev->config == NULL) {
		err = usb_get_configuration(udev);
		if (err < 0) {
			if (err != -ENODEV)
				dev_err(&udev->dev, "can't read configurations, error %d\n",
						err);
			return err;
		}
	}

	/* read the standard strings and cache them if present */
	udev->product = usb_cache_string(udev, udev->descriptor.iProduct);
	udev->manufacturer = usb_cache_string(udev,
					      udev->descriptor.iManufacturer);
	do {
		udelay(500);
		udev->serial = usb_cache_string(udev, udev->descriptor.iSerialNumber);
#ifdef MY_ABC_HERE
		/* Make the backup serial pointer points to the formal udev->serial.
		 * If we are going to change its serial string, we will make syno_old_serial
		 * keep the original serial(even it is NULL).
		 */
		udev->syno_old_serial = udev->serial;
#endif /* MY_ABC_HERE */
	} while (!udev->serial && retry--);

	usb_detect_interface_quirks(udev);

	return 0;
}

#ifdef MY_ABC_HERE
/* Return 1 if found the same serial in other usb device. Otherwizs, return 0. */
static int device_serial_match(struct usb_device *dev, struct usb_device *udev_search)
{
	int child, match = 0;
	struct usb_device *childdev;

	/* look through all of the children of this device */
	usb_hub_for_each_child(dev, child, childdev) {
		if (childdev && childdev != udev_search && childdev->serial) {

			/* Can't down() here. Because when using hub, it will be lock by someone else */
			if (childdev->serial[0] && strcmp(childdev->serial, udev_search->serial) == 0) {
				match++;
			} else {
				match = device_serial_match(childdev, udev_search);
			}

			if (match) {
				break;
			}
		}
	}
	return match;
}
#endif /* MY_ABC_HERE*/

static void set_usb_port_removable(struct usb_device *udev)
{
	struct usb_device *hdev = udev->parent;
	struct usb_hub *hub;
	u8 port = udev->portnum;
	u16 wHubCharacteristics;
	bool removable = true;

	if (!hdev)
		return;

	hub = usb_hub_to_struct_hub(udev->parent);

	/*
	 * If the platform firmware has provided information about a port,
	 * use that to determine whether it's removable.
	 */
	switch (hub->ports[udev->portnum - 1]->connect_type) {
	case USB_PORT_CONNECT_TYPE_HOT_PLUG:
		udev->removable = USB_DEVICE_REMOVABLE;
		return;
	case USB_PORT_CONNECT_TYPE_HARD_WIRED:
	case USB_PORT_NOT_USED:
		udev->removable = USB_DEVICE_FIXED;
		return;
	default:
		break;
	}

	/*
	 * Otherwise, check whether the hub knows whether a port is removable
	 * or not
	 */
	wHubCharacteristics = le16_to_cpu(hub->descriptor->wHubCharacteristics);

	if (!(wHubCharacteristics & HUB_CHAR_COMPOUND))
		return;

	if (hub_is_superspeed(hdev)) {
		if (le16_to_cpu(hub->descriptor->u.ss.DeviceRemovable)
				& (1 << port))
			removable = false;
	} else {
		if (hub->descriptor->u.hs.DeviceRemovable[port / 8] & (1 << (port % 8)))
			removable = false;
	}

	if (removable)
		udev->removable = USB_DEVICE_REMOVABLE;
	else
		udev->removable = USB_DEVICE_FIXED;

}

static void usb_set_uas_device_quirks(struct usb_device *udev)
{
	struct device_driver *driver;
	struct usb_interface_cache *intfc;
	struct usb_host_interface *alts;
	int i;

#define USB_QUIRK_UAS_MODE		0x80000000
#define USB_QUIRK_BOT_MODE		0x40000000

	if (USB_SPEED_SUPER != udev->speed)
		return;
	if (!udev->config || !udev->config[0].intf_cache[0])
		return;

	driver = driver_find("etuas", &usb_bus_type);
	if (driver == NULL)
		return;

	intfc = udev->config[0].intf_cache[0];
	for (i = 0; i < intfc->num_altsetting; i++) {
		alts = &intfc->altsetting[i];
		if (alts->desc.bInterfaceClass == USB_CLASS_MASS_STORAGE &&
			alts->desc.bInterfaceSubClass == USB_SC_SCSI &&
			alts->desc.bInterfaceProtocol == USB_PR_UAS) {
			struct usb_hub *hub = usb_hub_to_struct_hub(udev->parent);

			if (!test_bit(udev->portnum, hub->bot_mode_bits)) {
				udev->quirks |= USB_QUIRK_UAS_MODE;
				dev_info(&udev->dev, "set UAS mode quirk\n");
			} else {
				udev->quirks |= USB_QUIRK_BOT_MODE;
				dev_info(&udev->dev, "set BOT mode quirk\n");
			}
			return;
		}
	}
}

/**
 * usb_new_device - perform initial device setup (usbcore-internal)
 * @udev: newly addressed device (in ADDRESS state)
 *
 * This is called with devices which have been detected but not fully
 * enumerated.  The device descriptor is available, but not descriptors
 * for any device configuration.  The caller must have locked either
 * the parent hub (if udev is a normal device) or else the
 * usb_bus_list_lock (if udev is a root hub).  The parent's pointer to
 * udev has already been installed, but udev is not yet visible through
 * sysfs or other filesystem code.
 *
 * This call is synchronous, and may not be used in an interrupt context.
 *
 * Only the hub driver or root-hub registrar should ever call this.
 *
 * Return: Whether the device is configured properly or not. Zero if the
 * interface was registered with the driver core; else a negative errno
 * value.
 *
 */
int ethub_usb_new_device(struct usb_device *udev)
{
	int err;

	if (udev->parent) {
		/* Initialize non-root-hub device wakeup to disabled;
		 * device (un)configuration controls wakeup capable
		 * sysfs power/wakeup controls wakeup enabled/disabled
		 */
		device_init_wakeup(&udev->dev, 0);
	}

	/* Tell the runtime-PM framework the device is active */
	pm_runtime_set_active(&udev->dev);
	pm_runtime_get_noresume(&udev->dev);
	pm_runtime_use_autosuspend(&udev->dev);
	pm_runtime_enable(&udev->dev);

	/* By default, forbid autosuspend for all devices.  It will be
	 * allowed for hubs during binding.
	 */
	usb_disable_autosuspend(udev);

	err = usb_enumerate_device(udev);	/* Read descriptors */
	if (err < 0)
		goto fail;
	dev_dbg(&udev->dev, "udev %d, busnum %d, minor = %d\n",
			udev->devnum, udev->bus->busnum,
			(((udev->bus->busnum-1) * 128) + (udev->devnum-1)));
	/* export the usbdev device-node for libusb */
	udev->dev.devt = MKDEV(USB_DEVICE_MAJOR,
			(((udev->bus->busnum-1) * 128) + (udev->devnum-1)));

#ifdef MY_ABC_HERE
#define SERIAL_LEN 33
	/* Make a fake serial number from product name */
	if ( NULL == udev->product ) {
		udev->product = kmalloc(16, GFP_KERNEL);
		if (NULL != udev->product) {
			snprintf(udev->product, 16, "USBDevice");
		}
	}

	if (NULL == udev->serial && NULL != udev->product) {
		int i;
		char seed = 0xb4; /* pick a random number */

		udev->serial = kmalloc(SERIAL_LEN, GFP_KERNEL);
		if (NULL != udev->serial) {
			const int cProductLen = strlen(udev->product);

			printk("Got empty serial number. "
					"Generate serial number from product.\n");
			udev->serial[0] = '\0';
			for(i = 0; (i < cProductLen) && (i < (SERIAL_LEN-1)/2); i++) {
				snprintf(udev->serial + strlen(udev->serial),
					   SERIAL_LEN - strlen(udev->serial),
					   "%02x", (seed ^= udev->product[cProductLen-i-1]));
			}
			udev->serial[SERIAL_LEN-1] = '\0';
		}
	}

	if (udev->parent && udev->serial) {
		int match, counter = 0;
		struct list_head *buslist;
		struct usb_bus *bus;

RETRY:
		match = 0;
		for (buslist = usb_bus_list.next;
			  buslist != &usb_bus_list;
			  buslist = buslist->next) {

			bus = container_of(buslist, struct usb_bus, bus_list);
			if (!bus->root_hub)
				continue;

			mutex_lock(&hub_init_mutex);
			match = device_serial_match(bus->root_hub, udev);
			mutex_unlock(&hub_init_mutex);

			if (match) {
				break;
			}
		}
		if (match) {
			int Len = strlen(udev->serial);

			if (Len == 0) {
				printk("USB serial length is 0!\n");
			} else if (counter > 9) {
				printk("There are to many same devices (%d)\n", counter);
			} else {
#if defined(MY_ABC_HERE)
				/* backup old serial for checking if firmware changed after usb reset */
				udev->syno_old_serial = kmalloc(Len, GFP_KERNEL);
				memcpy(udev->syno_old_serial, udev->serial, Len);
#endif /* defined(MY_ABC_HERE) */
				udev->serial[Len - 1] = counter + '0';
				udev->serial[Len] = '\0';
				printk("%s (%d) Same device found. Change serial to %s \n",
						__FILE__, __LINE__, udev->serial);

				counter++;
				goto RETRY;
			}
		}
	}
#endif /* MY_ABC_HERE */

	/* Tell the world! */
	announce_device(udev);

	usb_set_uas_device_quirks(udev);
	device_enable_async_suspend(&udev->dev);

	/* check whether the hub or firmware marks this port as non-removable */
	if (udev->parent)
		set_usb_port_removable(udev);

	/* Register the device.  The device driver is responsible
	 * for configuring the device and invoking the add-device
	 * notifier chain (used by usbfs and possibly others).
	 */
	err = device_add(&udev->dev);
	if (err) {
		dev_err(&udev->dev, "can't device_add, error %d\n", err);
		goto fail;
	}

	/* Create link files between child device and usb port device. */
	if (udev->parent) {
		struct usb_hub *hub = usb_hub_to_struct_hub(udev->parent);
		int port1 = udev->portnum;
		struct usb_port	*port_dev = hub->ports[port1 - 1];

		err = sysfs_create_link(&udev->dev.kobj,
				&port_dev->dev.kobj, "port");
		if (err)
			goto fail;

		err = sysfs_create_link(&port_dev->dev.kobj,
				&udev->dev.kobj, "device");
		if (err) {
			sysfs_remove_link(&udev->dev.kobj, "port");
			goto fail;
		}

		if (!test_and_set_bit(port1, hub->child_usage_bits))
			pm_runtime_get_sync(&port_dev->dev);
	}

	(void) usb_create_ep_devs(&udev->dev, &udev->ep0, udev);
	usb_mark_last_busy(udev);
	pm_runtime_put_sync_autosuspend(&udev->dev);
	return err;

fail:
	ethub_usb_set_device_state(udev, USB_STATE_NOTATTACHED);
	pm_runtime_disable(&udev->dev);
	pm_runtime_set_suspended(&udev->dev);
	return err;
}

#define PORT_RESET_TRIES	5
#define SET_ADDRESS_TRIES	2
#define GET_DESCRIPTOR_TRIES	2
#define SET_CONFIG_TRIES	2

#define HUB_ROOT_RESET_TIME	50	/* times are in msec */
#define HUB_SHORT_RESET_TIME	10
#define HUB_BH_RESET_TIME	50
#define HUB_LONG_RESET_TIME	200
#define HUB_RESET_TIMEOUT	800

#define HUB_SPEED_MORPH_TRIES	3
#define HUB_SPEED_MORPH_RESET_TRIES	3

/* Is a USB 3.0 port in the Inactive or Compliance Mode state?
 * Port worm reset is required to recover
 */
static bool hub_port_warm_reset_required(struct usb_hub *hub, int port1,
		u16 portstatus)
{
	u16 link_state;

	if (!hub_is_superspeed(hub->hdev))
		return false;

	if (test_bit(port1, hub->warm_reset_bits))
		return true;

	link_state = portstatus & USB_PORT_STAT_LINK_STATE;
	return link_state == USB_SS_PORT_LS_SS_INACTIVE
		|| link_state == USB_SS_PORT_LS_COMP_MOD;
}

static int hub_port_wait_reset(struct usb_hub *hub, int port1,
			struct usb_device *udev, unsigned int delay, bool warm)
{
	int delay_time, ret;
	u16 portstatus;
	u16 portchange;

	for (delay_time = 0;
			delay_time < HUB_RESET_TIMEOUT;
			delay_time += delay) {
		/* wait to give the device a chance to reset */
		msleep(delay);

		/* read and decode port status */
		ret = hub_port_status(hub, port1, &portstatus, &portchange);
		if (ret < 0)
			return ret;

		/* The port state is unknown until the reset completes. */
		if (!(portstatus & USB_PORT_STAT_RESET))
			break;

		/* switch to the long delay after two short delay failures */
		if (delay_time >= 2 * HUB_SHORT_RESET_TIME)
			delay = HUB_LONG_RESET_TIME;

		dev_dbg(&hub->ports[port1 - 1]->dev,
				"not %sreset yet, waiting %dms\n",
				warm ? "warm " : "", delay);
	}

	if ((portstatus & USB_PORT_STAT_RESET))
		return -EBUSY;

	msleep(1000);

	ret = hub_port_status(hub, port1, &portstatus, &portchange);
	if (ret < 0)
		return ret;

	if (hub_port_warm_reset_required(hub, port1, portstatus))
		return -ENOTCONN;

	/* Device went away? */
	if (!(portstatus & USB_PORT_STAT_CONNECTION))
		return -ENOTCONN;

	/* bomb out completely if the connection bounced.  A USB 3.0
	 * connection may bounce if multiple warm resets were issued,
	 * but the device may have successfully re-connected. Ignore it.
	 */
	if (!hub_is_superspeed(hub->hdev) &&
			(portchange & USB_PORT_STAT_C_CONNECTION))
#ifdef MY_ABC_HERE
	{
		usb_clear_port_feature(hub->hdev, port1,
				USB_PORT_FEAT_C_CONNECTION);
		if (portchange & USB_PORT_STAT_C_ENABLE)
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_ENABLE);
		return -SYNO_CONNECT_BOUNCE;
	}
#else /* MY_ABC_HERE */
		return -ENOTCONN;
#endif /* MY_ABC_HERE */

	if (!(portstatus & USB_PORT_STAT_ENABLE))
		return -EBUSY;

	if (!udev)
		return 0;

	if (hub_is_superspeed(hub->hdev))
		udev->speed = USB_SPEED_SUPER;
	else if (portstatus & USB_PORT_STAT_HIGH_SPEED)
		udev->speed = USB_SPEED_HIGH;
	else if (portstatus & USB_PORT_STAT_LOW_SPEED)
		udev->speed = USB_SPEED_LOW;
	else
		udev->speed = USB_SPEED_FULL;
	return 0;
}

/* Handle port reset and port warm(BH) reset (for USB3 protocol ports) */
static int hub_port_reset(struct usb_hub *hub, int port1,
			struct usb_device *udev, unsigned int delay, bool warm)
{
	int i, status;
	u16 portchange, portstatus;
	struct usb_port *port_dev = hub->ports[port1 - 1];

	if (!hub_is_superspeed(hub->hdev)) {
		if (warm) {
			dev_err(hub->intfdev, "only USB3 hub support "
						"warm reset\n");
			return -EINVAL;
		}
	} else if (!warm) {
		/*
		 * If the caller hasn't explicitly requested a warm reset,
		 * double check and see if one is needed.
		 */
		if (hub_port_status(hub, port1, &portstatus, &portchange) == 0)
			if (hub_port_warm_reset_required(hub, port1,
							portstatus))
				warm = true;
	}
	clear_bit(port1, hub->warm_reset_bits);

	/* Reset the port */
	for (i = 0; i < PORT_RESET_TRIES; i++) {
		status = set_port_feature(hub->hdev, port1, (warm ?
					USB_PORT_FEAT_BH_PORT_RESET :
					USB_PORT_FEAT_RESET));
		if (status == -ENODEV) {
			;	/* The hub is gone */
		} else if (status) {
			dev_err(&port_dev->dev,
					"cannot %sreset (err = %d)\n",
					warm ? "warm " : "", status);
		} else {
			status = hub_port_wait_reset(hub, port1, udev, delay,
								warm);
			if (status && status != -ENOTCONN && status != -ENODEV)
				dev_dbg(hub->intfdev,
						"port_wait_reset: err = %d\n",
						status);
		}

		/* Check for disconnect or reset */
		if ((status == 0 || status == -ENOTCONN || status == -ENODEV)
#ifdef MY_ABC_HERE
				&& -SYNO_CONNECT_BOUNCE != status
#endif /* MY_ABC_HERE */
			) {
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_RESET);

			if (!hub_is_superspeed(hub->hdev))
				goto done;

			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_BH_PORT_RESET);
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_PORT_LINK_STATE);
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_CONNECTION);

			/*
			 * If a USB 3.0 device migrates from reset to an error
			 * state, re-issue the warm reset.
			 */
			if (hub_port_status(hub, port1,
					&portstatus, &portchange) < 0)
				goto done;

			if (!hub_port_warm_reset_required(hub, port1,
					portstatus))
				goto done;

			/*
			 * If the port is in SS.Inactive or Compliance Mode, the
			 * hot or warm reset failed.  Try another warm reset.
			 */
			if (!warm) {
				dev_dbg(&port_dev->dev,
						"hot reset failed, warm reset\n");
				warm = true;
			}
		}

		dev_dbg(&port_dev->dev,
				"not enabled, trying %sreset again...\n",
				warm ? "warm " : "");
		delay = HUB_LONG_RESET_TIME;
	}

	dev_err(&port_dev->dev, "Cannot enable. Maybe the USB cable is bad?\n");

done:
	if (status == 0) {
		/* TRSTRCY = 10 ms; plus some extra */
		msleep(10 + 40);
		if (udev) {
			struct usb_hcd *hcd = bus_to_hcd(udev->bus);

			update_devnum(udev, 0);
			/* The xHC may think the device is already reset,
			 * so ignore the status.
			 */
			if (hcd->driver->reset_device)
				hcd->driver->reset_device(hcd, udev);

			usb_set_device_state(udev, USB_STATE_DEFAULT);
		}
	} else {
		if (udev)
			usb_set_device_state(udev, USB_STATE_NOTATTACHED);
	}

	return status;
}

/* Check if a port is power on */
static int port_is_power_on(struct usb_hub *hub, unsigned portstatus)
{
	int ret = 0;

	if (hub_is_superspeed(hub->hdev)) {
		if (portstatus & USB_SS_PORT_STAT_POWER)
			ret = 1;
	} else {
		if (portstatus & USB_PORT_STAT_POWER)
			ret = 1;
	}

	return ret;
}

static void usb_lock_port(struct usb_port *port_dev)
		__acquires(&port_dev->status_lock)
{
	mutex_lock(&port_dev->status_lock);
	__acquire(&port_dev->status_lock);
}

static void usb_unlock_port(struct usb_port *port_dev)
		__releases(&port_dev->status_lock)
{
	mutex_unlock(&port_dev->status_lock);
	__release(&port_dev->status_lock);
}

#ifdef	CONFIG_PM

/* Check if a port is suspended(USB2.0 port) or in U3 state(USB3.0 port) */
static int port_is_suspended(struct usb_hub *hub, unsigned portstatus)
{
	int ret = 0;

	if (hub_is_superspeed(hub->hdev)) {
		if ((portstatus & USB_PORT_STAT_LINK_STATE)
				== USB_SS_PORT_LS_U3)
			ret = 1;
	} else {
		if (portstatus & USB_PORT_STAT_SUSPEND)
			ret = 1;
	}

	return ret;
}

/* Determine whether the device on a port is ready for a normal resume,
 * is ready for a reset-resume, or should be disconnected.
 */
static int check_port_resume_type(struct usb_device *udev,
		struct usb_hub *hub, int port1,
		int status, unsigned portchange, unsigned portstatus)
{
	struct usb_port *port_dev = hub->ports[port1 - 1];

	/* Is a warm reset needed to recover the connection? */
	if (status == 0 && udev->reset_resume
		&& hub_port_warm_reset_required(hub, port1, portstatus)) {
		/* pass */;
	}
	/* Is the device still present? */
	else if (status || port_is_suspended(hub, portstatus) ||
			!port_is_power_on(hub, portstatus) ||
			!(portstatus & USB_PORT_STAT_CONNECTION)) {
		if (status >= 0)
			status = -ENODEV;
	}

	/* Can't do a normal resume if the port isn't enabled,
	 * so try a reset-resume instead.
	 */
	else if (!(portstatus & USB_PORT_STAT_ENABLE) && !udev->reset_resume) {
		if (udev->persist_enabled)
			udev->reset_resume = 1;
		else
			status = -ENODEV;
	}

	if (status) {
		dev_dbg(&port_dev->dev, "status %04x.%04x after resume, %d\n",
				portchange, portstatus, status);
	} else if (udev->reset_resume) {

		/* Late port handoff can set status-change bits */
		if (portchange & USB_PORT_STAT_C_CONNECTION)
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_CONNECTION);
		if (portchange & USB_PORT_STAT_C_ENABLE)
			usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_ENABLE);
	}

	return status;
}

/*
 * usb_enable_remote_wakeup - enable remote wakeup for a device
 * @udev: target device
 *
 * For USB-2 devices: Set the device's remote wakeup feature.
 *
 * For USB-3 devices: Assume there's only one function on the device and
 * enable remote wake for the first interface.  FIXME if the interface
 * association descriptor shows there's more than one function.
 */
static int usb_enable_remote_wakeup(struct usb_device *udev)
{
	if (udev->speed < USB_SPEED_SUPER)
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, USB_RECIP_DEVICE,
				USB_DEVICE_REMOTE_WAKEUP, 0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
	else
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_SET_FEATURE, USB_RECIP_INTERFACE,
				USB_INTRF_FUNC_SUSPEND,
				USB_INTRF_FUNC_SUSPEND_RW |
					USB_INTRF_FUNC_SUSPEND_LP,
				NULL, 0, USB_CTRL_SET_TIMEOUT);
}

/*
 * usb_disable_remote_wakeup - disable remote wakeup for a device
 * @udev: target device
 *
 * For USB-2 devices: Clear the device's remote wakeup feature.
 *
 * For USB-3 devices: Assume there's only one function on the device and
 * disable remote wake for the first interface.  FIXME if the interface
 * association descriptor shows there's more than one function.
 */
static int usb_disable_remote_wakeup(struct usb_device *udev)
{
	if (udev->speed < USB_SPEED_SUPER)
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_CLEAR_FEATURE, USB_RECIP_DEVICE,
				USB_DEVICE_REMOTE_WAKEUP, 0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
	else
		return usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
				USB_REQ_CLEAR_FEATURE, USB_RECIP_INTERFACE,
				USB_INTRF_FUNC_SUSPEND,	0, NULL, 0,
				USB_CTRL_SET_TIMEOUT);
}

/* Count of wakeup-enabled devices at or below udev */
static unsigned wakeup_enabled_descendants(struct usb_device *udev)
{
	struct usb_hub *hub = usb_hub_to_struct_hub(udev);

	return udev->do_remote_wakeup +
			(hub ? hub->wakeup_enabled_descendants : 0);
}

/*
 * usb_port_suspend - suspend a usb device's upstream port
 * @udev: device that's no longer in active use, not a root hub
 * Context: must be able to sleep; device not locked; pm locks held
 *
 * Suspends a USB device that isn't in active use, conserving power.
 * Devices may wake out of a suspend, if anything important happens,
 * using the remote wakeup mechanism.  They may also be taken out of
 * suspend by the host, using usb_port_resume().  It's also routine
 * to disconnect devices while they are suspended.
 *
 * This only affects the USB hardware for a device; its interfaces
 * (and, for hubs, child devices) must already have been suspended.
 *
 * Selective port suspend reduces power; most suspended devices draw
 * less than 500 uA.  It's also used in OTG, along with remote wakeup.
 * All devices below the suspended port are also suspended.
 *
 * Devices leave suspend state when the host wakes them up.  Some devices
 * also support "remote wakeup", where the device can activate the USB
 * tree above them to deliver data, such as a keypress or packet.  In
 * some cases, this wakes the USB host.
 *
 * Suspending OTG devices may trigger HNP, if that's been enabled
 * between a pair of dual-role devices.  That will change roles, such
 * as from A-Host to A-Peripheral or from B-Host back to B-Peripheral.
 *
 * Devices on USB hub ports have only one "suspend" state, corresponding
 * to ACPI D2, "may cause the device to lose some context".
 * State transitions include:
 *
 *   - suspend, resume ... when the VBUS power link stays live
 *   - suspend, disconnect ... VBUS lost
 *
 * Once VBUS drop breaks the circuit, the port it's using has to go through
 * normal re-enumeration procedures, starting with enabling VBUS power.
 * Other than re-initializing the hub (plug/unplug, except for root hubs),
 * Linux (2.6) currently has NO mechanisms to initiate that:  no hub_wq
 * timer, no SRP, no requests through sysfs.
 *
 * If Runtime PM isn't enabled or used, non-SuperSpeed devices may not get
 * suspended until their bus goes into global suspend (i.e., the root
 * hub is suspended).  Nevertheless, we change @udev->state to
 * USB_STATE_SUSPENDED as this is the device's "logical" state.  The actual
 * upstream port setting is stored in @udev->port_is_suspended.
 *
 * Returns 0 on success, else negative errno.
 */
int ethub_usb_port_suspend(struct usb_device *udev, pm_message_t msg)
{
	struct usb_hub	*hub = usb_hub_to_struct_hub(udev->parent);
	struct usb_port *port_dev = hub->ports[udev->portnum - 1];
	int		port1 = udev->portnum;
	int		status;
	bool		really_suspend = true;

	usb_lock_port(port_dev);

	/* enable remote wakeup when appropriate; this lets the device
	 * wake up the upstream hub (including maybe the root hub).
	 *
	 * NOTE:  OTG devices may issue remote wakeup (or SRP) even when
	 * we don't explicitly enable it here.
	 */
	if (udev->do_remote_wakeup) {
		status = usb_enable_remote_wakeup(udev);
		if (status) {
			dev_dbg(&udev->dev, "won't remote wakeup, status %d\n",
					status);
			/* bail if autosuspend is requested */
			if (PMSG_IS_AUTO(msg))
				goto err_wakeup;
		}
	}

	/* see 7.1.7.6 */
	if (hub_is_superspeed(hub->hdev))
		status = hub_set_port_link_state(hub, port1, USB_SS_PORT_LS_U3);

	/*
	 * For system suspend, we do not need to enable the suspend feature
	 * on individual USB-2 ports.  The devices will automatically go
	 * into suspend a few ms after the root hub stops sending packets.
	 * The USB 2.0 spec calls this "global suspend".
	 *
	 * However, many USB hubs have a bug: They don't relay wakeup requests
	 * from a downstream port if the port's suspend feature isn't on.
	 * Therefore we will turn on the suspend feature if udev or any of its
	 * descendants is enabled for remote wakeup.
	 */
	else if (PMSG_IS_AUTO(msg) || wakeup_enabled_descendants(udev) > 0)
		status = set_port_feature(hub->hdev, port1,
				USB_PORT_FEAT_SUSPEND);
	else {
		really_suspend = false;
		status = 0;
	}
	if (status) {
		dev_dbg(&port_dev->dev, "can't suspend, status %d\n", status);

		if (udev->do_remote_wakeup)
			(void) usb_disable_remote_wakeup(udev);
 err_wakeup:

		/* System sleep transitions should never fail */
		if (!PMSG_IS_AUTO(msg))
			status = 0;
	} else {
		dev_dbg(&udev->dev, "usb %ssuspend, wakeup %d\n",
				(PMSG_IS_AUTO(msg) ? "auto-" : ""),
				udev->do_remote_wakeup);
		if (really_suspend) {
			udev->port_is_suspended = 1;

			/* device has up to 10 msec to fully suspend */
			msleep(10);
		}
		ethub_usb_set_device_state(udev, USB_STATE_SUSPENDED);
	}

	if (status == 0 && !udev->do_remote_wakeup && udev->persist_enabled
			&& test_and_clear_bit(port1, hub->child_usage_bits))
		pm_runtime_put_sync(&port_dev->dev);

	usb_mark_last_busy(hub->hdev);

	usb_unlock_port(port_dev);
	return status;
}

/*
 * If the USB "suspend" state is in use (rather than "global suspend"),
 * many devices will be individually taken out of suspend state using
 * special "resume" signaling.  This routine kicks in shortly after
 * hardware resume signaling is finished, either because of selective
 * resume (by host) or remote wakeup (by device) ... now see what changed
 * in the tree that's rooted at this device.
 *
 * If @udev->reset_resume is set then the device is reset before the
 * status check is done.
 */
static int finish_port_resume(struct usb_device *udev)
{
	int	status = 0;
	u16	devstatus = 0;

	/* caller owns the udev device lock */
	dev_dbg(&udev->dev, "%s\n",
		udev->reset_resume ? "finish reset-resume" : "finish resume");

	/* usb ch9 identifies four variants of SUSPENDED, based on what
	 * state the device resumes to.  Linux currently won't see the
	 * first two on the host side; they'd be inside hub_port_init()
	 * during many timeouts, but hub_wq can't suspend until later.
	 */
	ethub_usb_set_device_state(udev, udev->actconfig
			? USB_STATE_CONFIGURED
			: USB_STATE_ADDRESS);

	/* 10.5.4.5 says not to reset a suspended port if the attached
	 * device is enabled for remote wakeup.  Hence the reset
	 * operation is carried out here, after the port has been
	 * resumed.
	 */
	if (udev->reset_resume) {
		/*
		 * If the device morphs or switches modes when it is reset,
		 * we don't want to perform a reset-resume.  We'll fail the
		 * resume, which will cause a logical disconnect, and then
		 * the device will be rediscovered.
		 */
 retry_reset_resume:
		if (udev->quirks & USB_QUIRK_RESET)
			status = -ENODEV;
		else
			status = usb_reset_and_verify_device(udev);
	}

	/* 10.5.4.5 says be sure devices in the tree are still there.
	 * For now let's assume the device didn't go crazy on resume,
	 * and device drivers will know about any resume quirks.
	 */
	if (status == 0) {
		devstatus = 0;
		status = usb_get_status(udev, USB_RECIP_DEVICE, 0, &devstatus);

		/* If a normal resume failed, try doing a reset-resume */
		if (status && !udev->reset_resume && udev->persist_enabled) {
			dev_dbg(&udev->dev, "retry with reset-resume\n");
			udev->reset_resume = 1;
			goto retry_reset_resume;
		}
	}

	if (status) {
		dev_dbg(&udev->dev, "gone after usb resume? status %d\n",
				status);
	/*
	 * There are a few quirky devices which violate the standard
	 * by claiming to have remote wakeup enabled after a reset,
	 * which crash if the feature is cleared, hence check for
	 * udev->reset_resume
	 */
	} else if (udev->actconfig && !udev->reset_resume) {
		if (udev->speed < USB_SPEED_SUPER) {
			if (devstatus & (1 << USB_DEVICE_REMOTE_WAKEUP))
				status = usb_disable_remote_wakeup(udev);
		} else {
			status = usb_get_status(udev, USB_RECIP_INTERFACE, 0,
					&devstatus);
			if (!status && devstatus & (USB_INTRF_STAT_FUNC_RW_CAP
					| USB_INTRF_STAT_FUNC_RW))
				status = usb_disable_remote_wakeup(udev);
		}

		if (status)
			dev_dbg(&udev->dev,
				"disable remote wakeup, status %d\n",
				status);
		status = 0;
	}
	return status;
}

/*
 * usb_port_resume - re-activate a suspended usb device's upstream port
 * @udev: device to re-activate, not a root hub
 * Context: must be able to sleep; device not locked; pm locks held
 *
 * This will re-activate the suspended device, increasing power usage
 * while letting drivers communicate again with its endpoints.
 * USB resume explicitly guarantees that the power session between
 * the host and the device is the same as it was when the device
 * suspended.
 *
 * If @udev->reset_resume is set then this routine won't check that the
 * port is still enabled.  Furthermore, finish_port_resume() above will
 * reset @udev.  The end result is that a broken power session can be
 * recovered and @udev will appear to persist across a loss of VBUS power.
 *
 * For example, if a host controller doesn't maintain VBUS suspend current
 * during a system sleep or is reset when the system wakes up, all the USB
 * power sessions below it will be broken.  This is especially troublesome
 * for mass-storage devices containing mounted filesystems, since the
 * device will appear to have disconnected and all the memory mappings
 * to it will be lost.  Using the USB_PERSIST facility, the device can be
 * made to appear as if it had not disconnected.
 *
 * This facility can be dangerous.  Although usb_reset_and_verify_device() makes
 * every effort to insure that the same device is present after the
 * reset as before, it cannot provide a 100% guarantee.  Furthermore it's
 * quite possible for a device to remain unaltered but its media to be
 * changed.  If the user replaces a flash memory card while the system is
 * asleep, he will have only himself to blame when the filesystem on the
 * new card is corrupted and the system crashes.
 *
 * Returns 0 on success, else negative errno.
 */
int ethub_usb_port_resume(struct usb_device *udev, pm_message_t msg)
{
	struct usb_hub	*hub = usb_hub_to_struct_hub(udev->parent);
	struct usb_port *port_dev = hub->ports[udev->portnum  - 1];
	int		port1 = udev->portnum;
	int		status;
	u16		portchange, portstatus;

	if (!test_and_set_bit(port1, hub->child_usage_bits)) {
		status = pm_runtime_get_sync(&port_dev->dev);
		if (status < 0) {
			dev_dbg(&udev->dev, "can't resume usb port, status %d\n",
					status);
			return status;
		}
	}

	usb_lock_port(port_dev);

	/* Skip the initial Clear-Suspend step for a remote wakeup */
	status = hub_port_status(hub, port1, &portstatus, &portchange);
	if (status == 0 && !port_is_suspended(hub, portstatus))
		goto SuspendCleared;

	/* see 7.1.7.7; affects power usage, but not budgeting */
	if (hub_is_superspeed(hub->hdev))
		status = hub_set_port_link_state(hub, port1, USB_SS_PORT_LS_U0);
	else
		status = usb_clear_port_feature(hub->hdev,
				port1, USB_PORT_FEAT_SUSPEND);
	if (status) {
		dev_dbg(&port_dev->dev, "can't resume, status %d\n", status);
	} else {
		/* drive resume for USB_RESUME_TIMEOUT msec */
		dev_dbg(&udev->dev, "usb %sresume\n",
				(PMSG_IS_AUTO(msg) ? "auto-" : ""));
		msleep(USB_RESUME_TIMEOUT);

		/* Virtual root hubs can trigger on GET_PORT_STATUS to
		 * stop resume signaling.  Then finish the resume
		 * sequence.
		 */
		status = hub_port_status(hub, port1, &portstatus, &portchange);

		/* TRSMRCY = 10 msec */
		msleep(10);
	}

 SuspendCleared:
	if (status == 0) {
		udev->port_is_suspended = 0;
		if (hub_is_superspeed(hub->hdev)) {
			if (portchange & USB_PORT_STAT_C_LINK_STATE)
				usb_clear_port_feature(hub->hdev, port1,
					USB_PORT_FEAT_C_PORT_LINK_STATE);
		} else {
			if (portchange & USB_PORT_STAT_C_SUSPEND)
				usb_clear_port_feature(hub->hdev, port1,
						USB_PORT_FEAT_C_SUSPEND);
		}
	}

	status = check_port_resume_type(udev,
			hub, port1, status, portchange, portstatus);
	if (status == 0)
		status = finish_port_resume(udev);
	if (status < 0) {
		dev_dbg(&udev->dev, "can't resume, status %d\n", status);
		hub_port_logical_disconnect(hub, port1);
	}

	usb_unlock_port(port_dev);

	return status;
}

/* Returns 1 if there was a remote wakeup and a connect status change. */
static int hub_handle_remote_wakeup(struct usb_hub *hub, unsigned int port,
		u16 portstatus, u16 portchange)
		__must_hold(&port_dev->status_lock)
{
	struct usb_port *port_dev = hub->ports[port - 1];
	struct usb_device *hdev;
	struct usb_device *udev;
	int connect_change = 0;
	int ret;

	hdev = hub->hdev;
	udev = port_dev->child;
	if (!hub_is_superspeed(hdev)) {
		if (!(portchange & USB_PORT_STAT_C_SUSPEND))
			return 0;
		usb_clear_port_feature(hdev, port, USB_PORT_FEAT_C_SUSPEND);
	} else {
		if (!udev || udev->state != USB_STATE_SUSPENDED ||
				 (portstatus & USB_PORT_STAT_LINK_STATE) !=
				 USB_SS_PORT_LS_U0)
			return 0;
	}

	if (udev) {
		/* TRSMRCY = 10 msec */
		msleep(10);

		usb_unlock_port(port_dev);
		ret = usb_remote_wakeup(udev);
		usb_lock_port(port_dev);
		if (ret < 0)
			connect_change = 1;
	} else {
		ret = -ENODEV;
		hub_port_disable(hub, port, 1);
	}
	dev_dbg(&port_dev->dev, "resume, status %d\n", ret);
	return connect_change;
}

static int hub_suspend(struct usb_interface *intf, pm_message_t msg)
{
	struct usb_hub		*hub = usb_get_intfdata(intf);
	struct usb_device	*hdev = hub->hdev;
	unsigned		port1;
	int			status;

	/*
	 * Warn if children aren't already suspended.
	 * Also, add up the number of wakeup-enabled descendants.
	 */
	hub->wakeup_enabled_descendants = 0;
	for (port1 = 1; port1 <= hdev->maxchild; port1++) {
		struct usb_port *port_dev = hub->ports[port1 - 1];
		struct usb_device *udev = port_dev->child;

		if (udev && udev->can_submit) {
			dev_warn(&port_dev->dev, "device %s not suspended yet\n",
					dev_name(&udev->dev));
			if (PMSG_IS_AUTO(msg))
				return -EBUSY;
		}
		if (udev)
			hub->wakeup_enabled_descendants +=
					wakeup_enabled_descendants(udev);
	}
	if (hub_is_superspeed(hdev) && hdev->do_remote_wakeup) {
		/* Enable hub to send remote wakeup for all ports. */
		for (port1 = 1; port1 <= hdev->maxchild; port1++) {
			status = set_port_feature(hdev,
					port1 |
					USB_PORT_FEAT_REMOTE_WAKE_CONNECT |
					USB_PORT_FEAT_REMOTE_WAKE_DISCONNECT |
					USB_PORT_FEAT_REMOTE_WAKE_OVER_CURRENT,
					USB_PORT_FEAT_REMOTE_WAKE_MASK);
		}
	}

	dev_dbg(&intf->dev, "%s\n", __func__);

	/* stop hub_wq and related activity */
	hub_quiesce(hub, HUB_SUSPEND);
	return 0;
}

static int hub_resume(struct usb_interface *intf)
{
	struct usb_hub *hub = usb_get_intfdata(intf);

	dev_dbg(&intf->dev, "%s\n", __func__);
	hub_activate(hub, HUB_RESUME);
	return 0;
}

static int hub_reset_resume(struct usb_interface *intf)
{
	struct usb_hub *hub = usb_get_intfdata(intf);

	dev_dbg(&intf->dev, "%s\n", __func__);
	hub_activate(hub, HUB_RESET_RESUME);
	return 0;
}

#else	/* CONFIG_PM */

#define hub_suspend		NULL
#define hub_resume		NULL
#define hub_reset_resume	NULL

static int hub_handle_remote_wakeup(struct usb_hub *hub, unsigned int port,
		u16 portstatus, u16 portchange)
{
	return 0;
}

#endif	/* CONFIG_PM */

#define usb_sndaddr0pipe()	(PIPE_CONTROL << 30)
#define usb_rcvaddr0pipe()	((PIPE_CONTROL << 30) | USB_DIR_IN)

static int hub_set_address(struct usb_device *udev, int devnum)
{
	int retval;
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	/*
	 * The host controller will choose the device address,
	 * instead of the core having chosen it earlier
	 */
	if (!hcd->driver->address_device && devnum <= 1)
		return -EINVAL;
	if (udev->state == USB_STATE_ADDRESS)
		return 0;
	if (udev->state != USB_STATE_DEFAULT)
		return -EINVAL;
	if (hcd->driver->address_device)
		retval = hcd->driver->address_device(hcd, udev);
	else
		retval = usb_control_msg(udev, usb_sndaddr0pipe(),
				USB_REQ_SET_ADDRESS, 0, devnum, 0,
				NULL, 0, USB_CTRL_SET_TIMEOUT);
	if (retval == 0) {
		update_devnum(udev, devnum);
		/* Device now using proper address. */
		ethub_usb_set_device_state(udev, USB_STATE_ADDRESS);
		usb_ep0_reinit(udev);
	}
	return retval;
}

static int check_superspeed(struct usb_hub *hub, struct usb_device *udev)
{
	struct usb_device *hdev = hub->hdev;
	int retval = -EINVAL;

	if (!hdev->parent &&
		!hub_is_superspeed(hdev) &&
		0x0210 <= le16_to_cpu(udev->descriptor.bcdUSB)) {
		dev_info(&udev->dev, "not running at top speed\n");
		retval = 0;
	}

	return retval;
}

/* Reset device, (re)assign address, get device descriptor.
 * Device connection must be stable, no more debouncing needed.
 * Returns device in USB_STATE_ADDRESS, except on error.
 *
 * If this is called for an already-existing device (as part of
 * usb_reset_and_verify_device), the caller must own the device lock and
 * the port lock.  For a newly detected device that is not accessible
 * through any global pointers, it's not necessary to lock the device,
 * but it is still necessary to lock the port.
 */
static int
hub_port_init(struct usb_hub *hub, struct usb_device *udev, int port1,
		int retry_counter)
{
	struct usb_device	*hdev = hub->hdev;
	struct usb_hcd		*hcd = bus_to_hcd(hdev->bus);
	int			i, j, retval;
	unsigned		delay = HUB_SHORT_RESET_TIME;
	enum usb_device_speed	oldspeed = udev->speed;
	const char		*speed;
	int			devnum = udev->devnum;
	int reset_retry = 1;
	int speed_morph_count = 0;

	/* root hub ports have a slightly longer reset period
	 * (from USB 2.0 spec, section 7.1.7.5)
	 */
	if (!hdev->parent) {
		delay = HUB_ROOT_RESET_TIME;
		if (port1 == hdev->bus->otg_port)
			hdev->bus->b_hnp_enable = 0;
	}

	/* Some low speed devices have problems with the quick delay, so */
	/*  be a bit pessimistic with those devices. RHbug #23670 */
	if (oldspeed == USB_SPEED_LOW)
		delay = HUB_LONG_RESET_TIME;

	mutex_lock(hcd->address0_mutex);

port_speed_morph:
	do {
		/* Reset the device; full speed may morph to high speed */
		/* FIXME a USB 2.0 device may morph into SuperSpeed on reset. */
		retval = hub_port_reset(hub, port1, udev, delay, false);
		if (retval < 0)
			break;
	} while (--reset_retry);

	if (retval < 0)		/* error or disconnect */
		goto fail;
	/* success, speed is known */

	retval = -ENODEV;

	if (oldspeed != USB_SPEED_UNKNOWN && oldspeed != udev->speed) {
		dev_dbg(&udev->dev, "device reset changed speed!\n");
		goto fail;
	}
	oldspeed = udev->speed;

	/* USB 2.0 section 5.5.3 talks about ep0 maxpacket ...
	 * it's fixed size except for full speed devices.
	 * For Wireless USB devices, ep0 max packet is always 512 (tho
	 * reported as 0xff in the device descriptor). WUSB1.0[4.8.1].
	 */
	switch (udev->speed) {
	case USB_SPEED_SUPER:
	case USB_SPEED_WIRELESS:	/* fixed at 512 */
		udev->ep0.desc.wMaxPacketSize = cpu_to_le16(512);
		break;
	case USB_SPEED_HIGH:		/* fixed at 64 */
		udev->ep0.desc.wMaxPacketSize = cpu_to_le16(64);
		break;
	case USB_SPEED_FULL:		/* 8, 16, 32, or 64 */
		/* to determine the ep0 maxpacket size, try to read
		 * the device descriptor to get bMaxPacketSize0 and
		 * then correct our initial guess.
		 */
		udev->ep0.desc.wMaxPacketSize = cpu_to_le16(64);
		break;
	case USB_SPEED_LOW:		/* fixed at 8 */
		udev->ep0.desc.wMaxPacketSize = cpu_to_le16(8);
		break;
	default:
		goto fail;
	}

	if (udev->speed == USB_SPEED_WIRELESS)
		speed = "variable speed Wireless";
	else
		speed = usb_speed_string(udev->speed);

	if (udev->speed != USB_SPEED_SUPER)
		dev_info(&udev->dev,
				"%s %s USB device number %d using %s\n",
				(udev->config) ? "reset" : "new", speed,
				devnum, udev->bus->controller->driver->name);

	/* Set up TT records, if needed  */
	if (hdev->tt) {
		udev->tt = hdev->tt;
		udev->ttport = hdev->ttport;
	} else if (udev->speed != USB_SPEED_HIGH
			&& hdev->speed == USB_SPEED_HIGH) {
		if (!hub->tt.hub) {
			dev_err(&udev->dev, "parent hub has no TT\n");
			retval = -EINVAL;
			goto fail;
		}
		udev->tt = &hub->tt;
		udev->ttport = port1;
	}

	/* Why interleave GET_DESCRIPTOR and SET_ADDRESS this way?
	 * Because device hardware and firmware is sometimes buggy in
	 * this area, and this is how Linux has done it for ages.
	 * Change it cautiously.
	 *
	 * NOTE:  If USE_NEW_SCHEME() is true we will start by issuing
	 * a 64-byte GET_DESCRIPTOR request.  This is what Windows does,
	 * so it may help with some non-standards-compliant devices.
	 * Otherwise we start with SET_ADDRESS and then try to read the
	 * first 8 bytes of the device descriptor to get the ep0 maxpacket
	 * value.
	 */
	for (i = 0; i < GET_DESCRIPTOR_TRIES; (++i, msleep(100))) {
		/*
		 * If device is WUSB, we already assigned an
		 * unauthorized address in the Connect Ack sequence;
		 * authorization will assign the final address.
		 */
		if (udev->wusb == 0) {
			for (j = 0; j < SET_ADDRESS_TRIES; ++j) {
				retval = hub_set_address(udev, devnum);
				if (retval >= 0)
					break;
				msleep(200);
			}
			if (retval < 0) {
				if (retval != -ENODEV)
					dev_err(&udev->dev, "device not accepting address %d, error %d\n",
							devnum, retval);
				goto fail;
			}
			if (udev->speed == USB_SPEED_SUPER) {
				devnum = udev->devnum;
				dev_info(&udev->dev,
						"%s SuperSpeed USB device number %d using %s\n",
						(udev->config) ? "reset" : "new",
						devnum, udev->bus->controller->driver->name);
			}

			/* cope with hardware quirkiness:
			 *  - let SET_ADDRESS settle, some device hardware wants it
			 *  - read ep0 maxpacket even for high and low speed,
			 */
			msleep(10);
		}

		retval = usb_get_device_descriptor(udev, 8);
		if (retval < 8) {
			if (retval != -ENODEV)
				dev_err(&udev->dev,
					"device descriptor read/8, error %d\n",
					retval);
			if (retval >= 0)
				retval = -EMSGSIZE;
		} else {
			retval = 0;
			break;
		}
	}
	if (retval)
		goto fail;

	/*
	 * Some superspeed devices have finished the link training process
	 * and attached to a superspeed hub port, but the device descriptor
	 * got from those devices show they aren't superspeed devices. Warm
	 * reset the port attached by the devices can fix them.
	 */
	if ((udev->speed == USB_SPEED_SUPER) &&
			(le16_to_cpu(udev->descriptor.bcdUSB) < 0x0300)) {
		dev_err(&udev->dev, "got a wrong device descriptor, "
				"warm reset device\n");
		hub_port_reset(hub, port1, udev,
				HUB_BH_RESET_TIME, true);
		retval = -EINVAL;
		goto fail;
	}

	if (udev->descriptor.bMaxPacketSize0 == 0xff ||
			udev->speed == USB_SPEED_SUPER)
		i = 512;
	else
		i = udev->descriptor.bMaxPacketSize0;
	if (usb_endpoint_maxp(&udev->ep0.desc) != i) {
		if (udev->speed == USB_SPEED_LOW ||
				!(i == 8 || i == 16 || i == 32 || i == 64)) {
			dev_err(&udev->dev, "Invalid ep0 maxpacket: %d\n", i);
			retval = -EMSGSIZE;
			goto fail;
		}
		if (udev->speed == USB_SPEED_FULL)
			dev_dbg(&udev->dev, "ep0 maxpacket = %d\n", i);
		else
			dev_warn(&udev->dev, "Using ep0 maxpacket: %d\n", i);
		udev->ep0.desc.wMaxPacketSize = cpu_to_le16(i);
		usb_ep0_reinit(udev);
	}

	retval = usb_get_device_descriptor(udev, USB_DT_DEVICE_SIZE);
	if (retval < (signed)sizeof(udev->descriptor)) {
		if (retval != -ENODEV)
			dev_err(&udev->dev, "device descriptor read/all, error %d\n",
					retval);
		if (retval >= 0)
			retval = -ENOMSG;
		goto fail;
	}

	retval = check_superspeed(hub, udev);
	if (!retval && speed_morph_count < HUB_SPEED_MORPH_TRIES) {
		speed_morph_count++;
		reset_retry = HUB_SPEED_MORPH_RESET_TRIES;
		hub_port_disable(hub, port1, 0);
		oldspeed = USB_SPEED_UNKNOWN;
		goto port_speed_morph;
	}

	usb_detect_quirks(udev);

	if (udev->wusb == 0 && le16_to_cpu(udev->descriptor.bcdUSB) >= 0x0201)
		usb_get_bos_descriptor(udev);

	retval = 0;
	/* notify HCD that we have a device connected and addressed */
	if (hcd->driver->update_device)
		hcd->driver->update_device(hcd, udev);
fail:
	if (retval) {
		hub_port_disable(hub, port1, 0);
		update_devnum(udev, devnum);	/* for disconnect processing */
	}
	mutex_unlock(hcd->address0_mutex);
	return retval;
}

static void
check_highspeed(struct usb_hub *hub, struct usb_device *udev, int port1)
{
	struct usb_qualifier_descriptor	*qual;
	int				status;

	if (udev->quirks & USB_QUIRK_DEVICE_QUALIFIER)
		return;

	qual = kmalloc(sizeof *qual, GFP_KERNEL);
	if (qual == NULL)
		return;

	status = usb_get_descriptor(udev, USB_DT_DEVICE_QUALIFIER, 0,
			qual, sizeof *qual);
	if (status == sizeof *qual) {
		dev_info(&udev->dev, "not running at top speed; "
			"connect to a high speed hub\n");
		/* hub LEDs are probably harder to miss than syslog */
		if (hub->has_indicators) {
			hub->indicator[port1-1] = INDICATOR_GREEN_BLINK;
			queue_delayed_work(system_power_efficient_wq,
					&hub->leds, 0);
		}
	}
	kfree(qual);
}

static unsigned
hub_power_remaining(struct usb_hub *hub)
{
	struct usb_device *hdev = hub->hdev;
	int remaining;
	int port1;

	if (!hub->limited_power)
		return 0;

	remaining = hdev->bus_mA - hub->descriptor->bHubContrCurrent;
	for (port1 = 1; port1 <= hdev->maxchild; ++port1) {
		struct usb_port *port_dev = hub->ports[port1 - 1];
		struct usb_device *udev = port_dev->child;
		unsigned unit_load;
		int delta;

		if (!udev)
			continue;
		if (hub_is_superspeed(udev))
			unit_load = 150;
		else
			unit_load = 100;

		/*
		 * Unconfigured devices may not use more than one unit load,
		 * or 8mA for OTG ports
		 */
		if (udev->actconfig)
			delta = usb_get_max_power(udev, udev->actconfig);
		else if (port1 != udev->bus->otg_port || hdev->parent)
			delta = unit_load;
		else
			delta = 8;
		if (delta > hub->mA_per_port)
			dev_warn(&port_dev->dev, "%dmA is over %umA budget!\n",
					delta, hub->mA_per_port);
		remaining -= delta;
	}
	if (remaining < 0) {
		dev_warn(hub->intfdev, "%dmA over power budget!\n",
			-remaining);
		remaining = 0;
	}
	return remaining;
}

static void hub_port_connect(struct usb_hub *hub, int port1, u16 portstatus,
		u16 portchange)
{
	int status, i;
	unsigned unit_load;
	struct usb_device *hdev = hub->hdev;
	struct usb_hcd *hcd = bus_to_hcd(hdev->bus);
	struct usb_port *port_dev = hub->ports[port1 - 1];
	struct usb_device *udev = port_dev->child;

	/* Disconnect any existing devices under this port */
	if (udev) {
#ifdef MY_ABC_HERE
		/* Defer disconnect for UPS devices */
		if (unlikely(udev->syno_quirks &
					SYNO_USB_QUIRK_UPS_DISCONNECT_FILTER)) {
			if (portstatus & USB_PORT_STAT_CONNECTION) {
				/* for the case that the device is connected again after
				 * disconnected, cancel the disconnection and reset it.
				 */
				int ret;
				del_timer_sync(&hub->ups_discon_flt_timer);
				hub->ups_discon_flt_status =
					SYNO_UPS_DISCON_FLT_STATUS_NONE;
				hub->ups_discon_flt_last = jiffies;
				hub_port_debounce_be_stable(hub, port1);
				usb_lock_device(udev);
				ret = usb_reset_device(udev);
				usb_unlock_device(udev);
				if (0 > ret) {
					hub_port_status(hub, port1, &portstatus, &portchange);
					dev_err(&port_dev->dev, "deferred disconnect failed, "
							"on port %d reset (ret: %d), status: %04x, "
							"change: %04x\n", port1, ret, portstatus,
							portchange);
				} else
					return;
			} else if (portchange & USB_PORT_STAT_C_CONNECTION) {
				/* for the case that the device is disconnected, defer it. */
				if (unlikely((udev->syno_quirks &
								SYNO_USB_QUIRK_LIMITED_UPS_DISCONNECT_FILTERING)
							 && time_after(hub->ups_discon_flt_last + 15 * HZ,
								 jiffies)))
					/* For the buggy UPS which disconnects very frequently
					 * before a UPS driver (usually in userspace) links the UPS.
					 * Because the condition that the buggy UPS isn't stable
					 * initally and UPS driver can't also link it before the
					 * driver stops trying, so we should actually disconnect and
					 * re-connect to notify and restart the UPS driver.
					 */
					 ;
				else {
					if (!timer_pending(&hub->ups_discon_flt_timer)) {
						hub->ups_discon_flt_status =
							SYNO_UPS_DISCON_FLT_STATUS_DEFERRED;
						hub->ups_discon_flt_port = port1;
						hub->ups_discon_flt_timer.expires = jiffies + 3 * HZ;
						add_timer(&hub->ups_discon_flt_timer);
					}
					return;
				}
			} else {
				/* for the case that defer disconnection timeout, disconnect it
				 * actually.
				 */
				del_timer_sync(&hub->ups_discon_flt_timer);
				hub->ups_discon_flt_status =
					SYNO_UPS_DISCON_FLT_STATUS_NONE;
			}
		}
#endif /* MY_ABC_HERE */
		ethub_usb_disconnect(&port_dev->child);
	}

	/* We can forget about a "removed" device when there's a physical
	 * disconnect or the connect status changes.
	 */
	if (!(portstatus & USB_PORT_STAT_CONNECTION) ||
			(portchange & USB_PORT_STAT_C_CONNECTION)) {
		clear_bit(port1, hub->removed_bits);
		clear_bit(port1, hub->bot_mode_bits);
	}

	if (portchange & (USB_PORT_STAT_C_CONNECTION |
				USB_PORT_STAT_C_ENABLE)) {
		status = hub_port_debounce_be_stable(hub, port1);
		if (status < 0) {
			if (status != -ENODEV && printk_ratelimit())
				dev_err(&port_dev->dev,
						"connect-debounce failed\n");
			portstatus &= ~USB_PORT_STAT_CONNECTION;
		} else {
			portstatus = status;
		}
	}

	/* Return now if debouncing failed or nothing is connected or
	 * the device was "removed".
	 */
	if (!(portstatus & USB_PORT_STAT_CONNECTION) ||
			test_bit(port1, hub->removed_bits)) {

		/*
		 * maybe switch power back on (e.g. root hub was reset)
		 * but only if the port isn't owned by someone else.
		 */
		if (hub_is_port_power_switchable(hub)
				&& !port_is_power_on(hub, portstatus)
				&& !port_dev->port_owner)
			set_port_feature(hdev, port1, USB_PORT_FEAT_POWER);

		if (portstatus & USB_PORT_STAT_ENABLE)
			goto done;
		return;
	}
	if (hub_is_superspeed(hub->hdev))
		unit_load = 150;
	else
		unit_load = 100;

	status = 0;
	for (i = 0; i < SET_CONFIG_TRIES; i++) {

		/* reallocate for each attempt, since references
		 * to the previous one can escape in various ways
		 */
		udev = usb_alloc_dev(hdev, hdev->bus, port1);
		if (!udev) {
			dev_err(&port_dev->dev,
					"couldn't allocate usb_device\n");
			goto done;
		}

		ethub_usb_set_device_state(udev, USB_STATE_POWERED);
		udev->bus_mA = hub->mA_per_port;
		udev->level = hdev->level + 1;
		udev->wusb = 0;

		/* Only USB 3.0 devices are connected to SuperSpeed hubs. */
		if (hub_is_superspeed(hub->hdev))
			udev->speed = USB_SPEED_SUPER;
		else
			udev->speed = USB_SPEED_UNKNOWN;

		choose_devnum(udev);
		if (udev->devnum <= 0) {
			status = -ENOTCONN;	/* Don't retry */
			goto loop;
		}

		/* reset (non-USB 3.0 devices) and get descriptor */
		usb_lock_port(port_dev);
		status = hub_port_init(hub, udev, port1, i);
		usb_unlock_port(port_dev);
		if (status < 0)
			goto loop;

		if (udev->quirks & USB_QUIRK_DELAY_INIT)
			msleep(1000);

		/* consecutive bus-powered hubs aren't reliable; they can
		 * violate the voltage drop budget.  if the new child has
		 * a "powered" LED, users should notice we didn't enable it
		 * (without reading syslog), even without per-port LEDs
		 * on the parent.
		 */
		if (udev->descriptor.bDeviceClass == USB_CLASS_HUB
				&& udev->bus_mA <= unit_load) {
			u16	devstat;

			status = usb_get_status(udev, USB_RECIP_DEVICE, 0,
					&devstat);
			if (status) {
				dev_dbg(&udev->dev, "get status %d ?\n", status);
				goto loop_disable;
			}
			if ((devstat & (1 << USB_DEVICE_SELF_POWERED)) == 0) {
				dev_err(&udev->dev,
					"can't connect bus-powered hub "
					"to this port\n");
				if (hub->has_indicators) {
					hub->indicator[port1-1] =
						INDICATOR_AMBER_BLINK;
					queue_delayed_work(
						system_power_efficient_wq,
						&hub->leds, 0);
				}
				status = -ENOTCONN;	/* Don't retry */
				goto loop_disable;
			}
		}

		/* check for devices running slower than they could */
		if (le16_to_cpu(udev->descriptor.bcdUSB) >= 0x0200
				&& udev->speed == USB_SPEED_FULL
				&& highspeed_hubs != 0)
			check_highspeed(hub, udev, port1);

		/* Store the parent's children[] pointer.  At this point
		 * udev becomes globally accessible, although presumably
		 * no one will look at it until hdev is unlocked.
		 */
		status = 0;

		mutex_lock(&usb_port_peer_mutex);

		/* We mustn't add new devices if the parent hub has
		 * been disconnected; we would race with the
		 * recursively_mark_NOTATTACHED() routine.
		 */
		spin_lock_irq(&device_state_lock);
		if (hdev->state == USB_STATE_NOTATTACHED)
			status = -ENOTCONN;
		else
			port_dev->child = udev;
		spin_unlock_irq(&device_state_lock);
		mutex_unlock(&usb_port_peer_mutex);

		/* Run it through the hoops (find a driver, etc) */
		if (!status) {
			status = ethub_usb_new_device(udev);
			if (status) {
				mutex_lock(&usb_port_peer_mutex);
				spin_lock_irq(&device_state_lock);
				port_dev->child = NULL;
				spin_unlock_irq(&device_state_lock);
				mutex_unlock(&usb_port_peer_mutex);
			}
		}

		if (status)
			goto loop_disable;

		status = hub_power_remaining(hub);
		if (status)
			dev_dbg(hub->intfdev, "%dmA power budget left\n", status);

		return;

loop_disable:
		hub_port_disable(hub, port1, 1);
loop:
		usb_ep0_reinit(udev);
		release_devnum(udev);
		hub_free_dev(udev);
		usb_put_dev(udev);
		if ((status == -ENOTCONN) || (status == -ENOTSUPP))
			break;
	}
	if (hub->hdev->parent ||
			!hcd->driver->port_handed_over ||
			!(hcd->driver->port_handed_over)(hcd, port1)) {
		if (status != -ENOTCONN && status != -ENODEV)
			dev_err(&port_dev->dev,
					"unable to enumerate USB device\n");
	}

done:
	hub_port_disable(hub, port1, 1);
	if (hcd->driver->relinquish_port && !hub->hdev->parent)
		hcd->driver->relinquish_port(hcd, port1);

}

/* Handle physical or logical connection change events.
 * This routine is called when:
 *	a port connection-change occurs;
 *	a port enable-change occurs (often caused by EMI);
 *	usb_reset_and_verify_device() encounters changed descriptors (as from
 *		a firmware download)
 * caller already locked the hub
 */
static void hub_port_connect_change(struct usb_hub *hub, int port1,
					u16 portstatus, u16 portchange)
		__must_hold(&port_dev->status_lock)
{
	struct usb_port *port_dev = hub->ports[port1 - 1];
	struct usb_device *udev = port_dev->child;
	int status = -ENODEV;

	dev_dbg(&port_dev->dev, "status %04x, change %04x, %s\n", portstatus,
			portchange, portspeed(hub, portstatus));

	if (hub->has_indicators) {
		set_port_led(hub, port1, HUB_LED_AUTO);
		hub->indicator[port1-1] = INDICATOR_AUTO;
	}

	/* Try to resuscitate an existing device */
	if ((portstatus & USB_PORT_STAT_CONNECTION) && udev &&
			udev->state != USB_STATE_NOTATTACHED) {
		if ((portstatus & USB_PORT_STAT_ENABLE)
			&& !(portchange & USB_PORT_STAT_C_CONNECTION)) {
			status = 0;		/* Nothing to do */
#ifdef MY_ABC_HERE
			if (unlikely((udev->syno_quirks &
							SYNO_USB_QUIRK_UPS_DISCONNECT_FILTER) &&
						 (hub->ups_discon_flt_status !=
							SYNO_UPS_DISCON_FLT_STATUS_NONE))) {
				dev_warn(&port_dev->dev, "UPS disconnect filter: re-connected but "
						"port %d still enabled, status: %04x, change: %04x\n",
						port1, portstatus, portchange);
			}
#endif /* MY_ABC_HERE */
#ifdef CONFIG_PM
		} else if (udev->state == USB_STATE_SUSPENDED &&
				udev->persist_enabled) {
			/* For a suspended device, treat this as a
			 * remote wakeup event.
			 */
			usb_unlock_port(port_dev);
			status = usb_remote_wakeup(udev);
			usb_lock_port(port_dev);
#endif
		} else {
			/* Don't resuscitate */;
		}
	}
	clear_bit(port1, hub->change_bits);

	/* successfully revalidated the connection */
	if (status == 0)
		return;

	usb_unlock_port(port_dev);
	hub_port_connect(hub, port1, portstatus, portchange);
	usb_lock_port(port_dev);
}

static void port_event(struct usb_hub *hub, int port1)
		__must_hold(&port_dev->status_lock)
{
	int connect_change;
	struct usb_port *port_dev = hub->ports[port1 - 1];
	struct usb_device *udev = port_dev->child;
	struct usb_device *hdev = hub->hdev;
	u16 portstatus, portchange;

	connect_change = test_bit(port1, hub->change_bits);
	clear_bit(port1, hub->event_bits);
	clear_bit(port1, hub->wakeup_bits);

	if (hub_port_status(hub, port1, &portstatus, &portchange) < 0)
		return;

	if (portchange & USB_PORT_STAT_C_CONNECTION) {
		usb_clear_port_feature(hdev, port1, USB_PORT_FEAT_C_CONNECTION);
		connect_change = 1;
	}

	if (portchange & USB_PORT_STAT_C_ENABLE) {
		if (!connect_change)
			dev_dbg(&port_dev->dev, "enable change, status %08x\n",
					portstatus);
		usb_clear_port_feature(hdev, port1, USB_PORT_FEAT_C_ENABLE);

		/*
		 * EM interference sometimes causes badly shielded USB devices
		 * to be shutdown by the hub, this hack enables them again.
		 * Works at least with mouse driver.
		 */
		if (!(portstatus & USB_PORT_STAT_ENABLE)
		    && !connect_change && udev) {
#ifdef MY_ABC_HERE
			int ret = 1;
#endif /* MY_ABC_HERE */
			dev_err(&port_dev->dev, "disabled by hub (EMI?), re-enabling...\n");
#ifdef MY_ABC_HERE
			/* re-enable for UPS's EMI without disconnect */
			if (udev->syno_quirks &
					SYNO_USB_QUIRK_UPS_DISCONNECT_FILTER) {
				usb_unlock_port(port_dev);
				usb_lock_device(udev);
				ret = usb_reset_device(udev);
				usb_unlock_device(udev);
				usb_lock_port(port_dev);
			}

			if (ret)
#endif /* MY_ABC_HERE */
			connect_change = 1;
		}
	}

	if (portchange & USB_PORT_STAT_C_OVERCURRENT) {
		u16 status = 0, unused;

		dev_dbg(&port_dev->dev, "over-current change\n");
		usb_clear_port_feature(hdev, port1,
				USB_PORT_FEAT_C_OVER_CURRENT);
		msleep(100);	/* Cool down */
		hub_power_on(hub, true);
		hub_port_status(hub, port1, &status, &unused);
		if (status & USB_PORT_STAT_OVERCURRENT)
			dev_err(&port_dev->dev, "over-current condition\n");
	}

	if (portchange & USB_PORT_STAT_C_RESET) {
		dev_dbg(&port_dev->dev, "reset change\n");
		usb_clear_port_feature(hdev, port1, USB_PORT_FEAT_C_RESET);
	}
	if ((portchange & USB_PORT_STAT_C_BH_RESET)
	    && hub_is_superspeed(hdev)) {
		dev_dbg(&port_dev->dev, "warm reset change\n");
		usb_clear_port_feature(hdev, port1,
				USB_PORT_FEAT_C_BH_PORT_RESET);
	}
	if (portchange & USB_PORT_STAT_C_LINK_STATE) {
		dev_dbg(&port_dev->dev, "link state change\n");
		usb_clear_port_feature(hdev, port1,
				USB_PORT_FEAT_C_PORT_LINK_STATE);
	}
	if (portchange & USB_PORT_STAT_C_CONFIG_ERROR) {
		dev_warn(&port_dev->dev, "config error\n");
		usb_clear_port_feature(hdev, port1,
				USB_PORT_FEAT_C_PORT_CONFIG_ERROR);
	}

	/* skip port actions that require the port to be powered on */
	if (!pm_runtime_active(&port_dev->dev))
		return;

	if (hub_handle_remote_wakeup(hub, port1, portstatus, portchange))
		connect_change = 1;

	/*
	 * Warm reset a USB3 protocol port if it's in
	 * SS.Inactive state.
	 */
	if (hub_port_warm_reset_required(hub, port1, portstatus)) {
		dev_dbg(&port_dev->dev, "do warm reset\n");
		if (!udev || !(portstatus & USB_PORT_STAT_CONNECTION)
				|| udev->state == USB_STATE_NOTATTACHED) {
			if (hub_port_reset(hub, port1, NULL,
					HUB_BH_RESET_TIME, true) < 0)
				hub_port_disable(hub, port1, 1);
			else {
				connect_change = 0;
				if (!hub_port_status(hub, port1,
						&portstatus, &portchange)) {
					if (!udev &&
							(portstatus & USB_PORT_STAT_CONNECTION))
						connect_change = 1;
					if (udev &&
							!(portstatus & USB_PORT_STAT_CONNECTION))
						connect_change = 1;
				}
			}
		} else {
			usb_unlock_port(port_dev);
			usb_lock_device(udev);
			ethub_usb_reset_device(udev);
			usb_unlock_device(udev);
			usb_lock_port(port_dev);
			connect_change = 0;
		}
	}

	if (connect_change)
		hub_port_connect_change(hub, port1, portstatus, portchange);
}

static void hub_event(struct work_struct *work)
{
	struct usb_device *hdev;
	struct usb_interface *intf;
	struct usb_hub *hub;
	struct device *hub_dev;
	u16 hubstatus;
	u16 hubchange;
	int i, ret;
#ifdef MY_ABC_HERE
	u16 portstatus;
	u16 portchange;
	int ups_discon_flt;
	int connect_change;
#endif /* MY_ABC_HERE */

	hub = container_of(work, struct usb_hub, events);
	hdev = hub->hdev;
	hub_dev = hub->intfdev;
	intf = to_usb_interface(hub_dev);

	dev_dbg(hub_dev, "state %d ports %d chg %04x evt %04x\n",
			hdev->state, hdev->maxchild,
			/* NOTE: expects max 15 ports... */
			(u16) hub->change_bits[0],
			(u16) hub->event_bits[0]);

	/* Lock the device, then check to see if we were
	 * disconnected while waiting for the lock to succeed. */
	usb_lock_device(hdev);
	if (unlikely(hub->disconnected))
		goto out_hdev_lock;

	/* If the hub has died, clean up after it */
	if (hdev->state == USB_STATE_NOTATTACHED) {
		hub->error = -ENODEV;
		hub_quiesce(hub, HUB_DISCONNECT);
		goto out_hdev_lock;
	}

	/* Autoresume */
	ret = usb_autopm_get_interface(intf);
	if (ret) {
		dev_dbg(hub_dev, "Can't autoresume: %d\n", ret);
		goto out_hdev_lock;
	}

	/* If this is an inactive hub, do nothing */
	if (hub->quiescing)
		goto out_autopm;

	if (hub->error) {
		dev_dbg(hub_dev, "resetting for error %d\n", hub->error);

		ret = ethub_usb_reset_device(hdev);
		if (ret) {
			dev_dbg(hub_dev, "error resetting hub: %d\n", ret);
			goto out_autopm;
		}

		hub->nerrors = 0;
		hub->error = 0;
	}

#ifdef MY_ABC_HERE
	ups_discon_flt = 0;

	/* Serialize hub events among a deferred disconnection of a UPS and
	 * others within the same instance of hub, by deferring hub events to
	 * behind the deferred disconnection of a UPS.
	 */
	if (SYNO_UPS_DISCON_FLT_STATUS_NONE != hub->ups_discon_flt_status) {
		connect_change = 0;
		i = hub->ups_discon_flt_port;
		/* for the case of ups_discon_flt_timer timeout */
		connect_change = test_bit(i, hub->change_bits);

		ret = hub_port_status(hub, i,
				&portstatus, &portchange);

		/* for the case of re-connection of a UPS */
		if (0 >= ret &&
			portchange & USB_PORT_STAT_C_CONNECTION)
			connect_change = 1;

		if (connect_change)
			/* we must ensure that the target port of UPS-disconnect filter
			 * (i.e. ups_discon_flt_port) can be dealt with before others by
			 * the following for-loop.
			 */
			ups_discon_flt = 1;
		else
			/* all hub events coming from the ports that are not the target
			 * of UPS-disconnect filter (i.e. ups_discon_flt_port) will be
			 * discarded (or pending, actually) if there is a pending UPS-
			 * disconnection within the same instance of hub. Because a
			 * pending UPS-disconnection must generate a hub event, so the
			 * discarded ones will be also dealt with by the following for-
			 * loop later.
			 */
			goto out_hdev_lock;
	}
#endif /* MY_ABC_HERE */

	/* deal with port status changes */
#ifdef MY_ABC_HERE
	for (i = ups_discon_flt? hub->ups_discon_flt_port: 1;
		 i <= hdev->maxchild;
		 i = ups_discon_flt? 1: i + 1, ups_discon_flt = 0) {
#else
	for (i = 1; i <= hdev->maxchild; i++) {
#endif /* MY_ABC_HERE */
		struct usb_port *port_dev = hub->ports[i - 1];

		if (test_bit(i, hub->event_bits)
				|| test_bit(i, hub->change_bits)
				|| test_bit(i, hub->wakeup_bits)) {
			/*
			 * The get_noresume and barrier ensure that if
			 * the port was in the process of resuming, we
			 * flush that work and keep the port active for
			 * the duration of the port_event().  However,
			 * if the port is runtime pm suspended
			 * (powered-off), we leave it in that state, run
			 * an abbreviated port_event(), and move on.
			 */
			pm_runtime_get_noresume(&port_dev->dev);
			pm_runtime_barrier(&port_dev->dev);
			usb_lock_port(port_dev);
			port_event(hub, i);
			usb_unlock_port(port_dev);
			pm_runtime_put_sync(&port_dev->dev);
		}
	}

	/* deal with hub status changes */
	if (test_and_clear_bit(0, hub->event_bits) == 0)
		;	/* do nothing */
	else if (hub_hub_status(hub, &hubstatus, &hubchange) < 0)
		dev_err(hub_dev, "get_hub_status failed\n");
	else {
		if (hubchange & HUB_CHANGE_LOCAL_POWER) {
			dev_dbg(hub_dev, "power change\n");
			clear_hub_feature(hdev, C_HUB_LOCAL_POWER);
			if (hubstatus & HUB_STATUS_LOCAL_POWER)
				/* FIXME: Is this always true? */
				hub->limited_power = 1;
			else
				hub->limited_power = 0;
		}
		if (hubchange & HUB_CHANGE_OVERCURRENT) {
			u16 status = 0;
			u16 unused;

			dev_dbg(hub_dev, "over-current change\n");
			clear_hub_feature(hdev, C_HUB_OVER_CURRENT);
			msleep(500);	/* Cool down */
			hub_power_on(hub, true);
			hub_hub_status(hub, &status, &unused);
			if (status & HUB_STATUS_OVERCURRENT)
				dev_err(hub_dev, "over-current condition\n");
		}
	}

out_autopm:
	/* Balance the usb_autopm_get_interface() above */
	usb_autopm_put_interface_no_suspend(intf);
out_hdev_lock:
	usb_unlock_device(hdev);

	/* Balance the stuff in kick_hub_wq() and allow autosuspend */
	usb_autopm_put_interface(intf);
	kref_put(&hub->kref, hub_release);
}

static const struct usb_device_id hub_id_table[] = {
    { .match_flags = USB_DEVICE_ID_MATCH_DEV_CLASS,
      .bDeviceClass = USB_CLASS_HUB},
    { .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS,
      .bInterfaceClass = USB_CLASS_HUB},
    { }						/* Terminating entry */
};

static struct usb_driver hub_driver = {
	.name =		"ethub",
	.probe =	hub_probe,
	.disconnect =	hub_disconnect,
	.suspend =	hub_suspend,
	.resume =	hub_resume,
	.reset_resume =	hub_reset_resume,
	.pre_reset =	hub_pre_reset,
	.post_reset =	hub_post_reset,
	.unlocked_ioctl = hub_ioctl,
	.id_table =	hub_id_table,
	.supports_autosuspend =	1,
};

int ethub_init(void)
{
	if (usb_register(&hub_driver) < 0) {
		printk(KERN_ERR "%s: can't register ethub driver\n",
			ethub_name);
		return -1;
	}

	/*
	 * The workqueue needs to be freezable to avoid interfering with
	 * USB-PERSIST port handover. Otherwise it might see that a full-speed
	 * device was gone before the EHCI controller had handed its port
	 * over to the companion full-speed controller.
	 */
	hub_wq = alloc_workqueue("usb_ethub_wq", WQ_FREEZABLE, 0);
	if (hub_wq)
		return 0;

	/* Fall through if kernel_thread failed */
	usb_deregister(&hub_driver);
	pr_err("%s: can't allocate workqueue\n", ethub_name);

	return -1;
}

void ethub_cleanup(void)
{
	destroy_workqueue(hub_wq);

	/*
	 * Hub resources are freed for us by usb_deregister. It calls
	 * usb_driver_purge on every device which in turn calls that
	 * devices disconnect function if it is using this driver.
	 * The hub_disconnect function takes care of releasing the
	 * individual hub resources. -greg
	 */
	usb_deregister(&hub_driver);
} /* usb_hub_cleanup() */

static int descriptors_changed(struct usb_device *udev,
		struct usb_device_descriptor *old_device_descriptor,
		struct usb_host_bos *old_bos)
{
	int		changed = 0;
	unsigned	index;
	unsigned	serial_len = 0;
	unsigned	len;
	unsigned	old_length;
	int		length;
	char		*buf;

	if (memcmp(&udev->descriptor, old_device_descriptor,
			sizeof(*old_device_descriptor)) != 0)
		return 1;

	if ((old_bos && !udev->bos) || (!old_bos && udev->bos))
		return 1;
	if (udev->bos) {
		len = le16_to_cpu(udev->bos->desc->wTotalLength);
		if (len != le16_to_cpu(old_bos->desc->wTotalLength))
			return 1;
		if (memcmp(udev->bos->desc, old_bos->desc, len))
			return 1;
	}

	/* Since the idVendor, idProduct, and bcdDevice values in the
	 * device descriptor haven't changed, we will assume the
	 * Manufacturer and Product strings haven't changed either.
	 * But the SerialNumber string could be different (e.g., a
	 * different flash card of the same brand).
	 */
#if defined(MY_ABC_HERE)
	/* For bug <Linux Kernel Porting - linux-4.4.x> #295:
	 * when calling usb_reset_device, compare the length of serial number
	 * with the original one. If we didn't change the serial, the
	 * udev->syno_old_serial will still point to udev->serial.
	 */
	if (udev->syno_old_serial)
		serial_len = strlen(udev->syno_old_serial) + 1;
#else
	if (udev->serial)
		serial_len = strlen(udev->serial) + 1;
#endif /* defined(MY_ABC_HERE) */

	len = serial_len;
	for (index = 0; index < udev->descriptor.bNumConfigurations; index++) {
		old_length = le16_to_cpu(udev->config[index].desc.wTotalLength);
		len = max(len, old_length);
	}

	buf = kmalloc(len, GFP_NOIO);
	if (buf == NULL) {
		dev_err(&udev->dev, "no mem to re-read configs after reset\n");
		/* assume the worst */
		return 1;
	}
	for (index = 0; index < udev->descriptor.bNumConfigurations; index++) {
		old_length = le16_to_cpu(udev->config[index].desc.wTotalLength);
		length = usb_get_descriptor(udev, USB_DT_CONFIG, index, buf,
				old_length);
		if (length != old_length) {
			dev_dbg(&udev->dev, "config index %d, error %d\n",
					index, length);
			changed = 1;
			break;
		}
		if (memcmp(buf, udev->rawdescriptors[index], old_length)
				!= 0) {
			dev_dbg(&udev->dev, "config index %d changed (#%d)\n",
				index,
				((struct usb_config_descriptor *) buf)->
					bConfigurationValue);
			changed = 1;
			break;
		}
	}

	if (!changed && serial_len) {
		length = usb_string(udev, udev->descriptor.iSerialNumber,
				buf, serial_len);
		if (length + 1 != serial_len) {
			dev_dbg(&udev->dev, "serial string error %d\n",
					length);
			changed = 1;
#if defined(MY_ABC_HERE)
		/* For bug <Linux Kernel Porting - linux-4.4.x> #295 , when calling usb_reset_device,
		 * compare the serial number with the original one.
		 * If we had changed its serial, the syno_old_serial will still keep the original one;
		 * Otherwise it will point to whatever udev->serial points.
		 */
		} else if (memcmp(buf, udev->syno_old_serial, length) != 0) {
#else
		} else if (memcmp(buf, udev->serial, length) != 0) {
#endif /* defined(MY_ABC_HERE) */
			dev_dbg(&udev->dev, "serial string changed\n");
			changed = 1;
		}
	}

	kfree(buf);
	return changed;
}

static void usb_stop_device(struct usb_device *udev)
{
	int i;
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);
	struct usb_host_endpoint *ep;

	for (i = 1; i < 16; i++) {
		ep = udev->ep_out[i];
		if (ep) {
			ep->enabled = 0;
			if (hcd->driver->stop_endpoint)
				hcd->driver->stop_endpoint(hcd, udev, ep);
		}

		ep = udev->ep_in[i];
		if (ep) {
			ep->enabled = 0;
			if (hcd->driver->stop_endpoint)
				hcd->driver->stop_endpoint(hcd, udev, ep);
		}
	}
}

/**
 * usb_reset_and_verify_device - perform a USB port reset to reinitialize a device
 * @udev: device to reset (not in SUSPENDED or NOTATTACHED state)
 *
 * WARNING - don't use this routine to reset a composite device
 * (one with multiple interfaces owned by separate drivers)!
 * Use usb_reset_device() instead.
 *
 * Do a port reset, reassign the device's address, and establish its
 * former operating configuration.  If the reset fails, or the device's
 * descriptors change from their values before the reset, or the original
 * configuration and altsettings cannot be restored, a flag will be set
 * telling hub_wq to pretend the device has been disconnected and then
 * re-connected.  All drivers will be unbound, and the device will be
 * re-enumerated and probed all over again.
 *
 * Return: 0 if the reset succeeded, -ENODEV if the device has been
 * flagged for logical disconnection, or some other negative error code
 * if the reset wasn't even attempted.
 *
 * Note:
 * The caller must own the device lock and the port lock, the latter is
 * taken by usb_reset_device().  For example, it's safe to use
 * usb_reset_device() from a driver probe() routine after downloading
 * new firmware.  For calls that might not occur during probe(), drivers
 * should lock the device using usb_lock_device_for_reset().
 *
 * Locking exception: This routine may also be called from within an
 * autoresume handler.  Such usage won't conflict with other tasks
 * holding the device lock because these tasks should always call
 * usb_autopm_resume_device(), thereby preventing any unwanted
 * autoresume.  The autoresume handler is expected to have already
 * acquired the port lock before calling this routine.
 */
static int usb_reset_and_verify_device(struct usb_device *udev)
{
	struct usb_device		*parent_hdev = udev->parent;
	struct usb_hub			*parent_hub;
	struct usb_hcd			*hcd = bus_to_hcd(udev->bus);
	struct usb_device_descriptor	descriptor = udev->descriptor;
	struct usb_host_bos		*bos;
	int				i, j, ret = 0;
	int				port1 = udev->portnum;

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED) {
		dev_dbg(&udev->dev, "device reset not allowed in state %d\n",
				udev->state);
		return -EINVAL;
	}

	if (!parent_hdev)
		return -EISDIR;

	parent_hub = usb_hub_to_struct_hub(parent_hdev);

	bos = udev->bos;
	udev->bos = NULL;

	usb_stop_device(udev);

	for (i = 0; i < SET_CONFIG_TRIES; ++i) {

		/* ep0 maxpacket size may change; let the HCD know about it.
		 * Other endpoints will be handled by re-enumeration. */
		usb_ep0_reinit(udev);
		ret = hub_port_init(parent_hub, udev, port1, i);
		if (ret >= 0 || ret == -ENOTCONN || ret == -ENODEV)
			break;
	}

	if (ret < 0)
		goto re_enumerate;

	/* Device might have changed firmware (DFU or similar) */
	if (descriptors_changed(udev, &descriptor, bos)) {
		dev_info(&udev->dev, "device firmware changed\n");
		udev->descriptor = descriptor;	/* for disconnect() calls */
		goto re_enumerate;
	}

	/* Restore the device's previous configuration */
	if (!udev->actconfig)
		goto done;

	mutex_lock(hcd->bandwidth_mutex);
	ret = usb_hcd_alloc_bandwidth(udev, udev->actconfig, NULL, NULL);
	if (ret < 0) {
		dev_warn(&udev->dev,
				"Busted HC?  Not enough HCD resources for "
				"old configuration.\n");
		mutex_unlock(hcd->bandwidth_mutex);
		goto re_enumerate;
	}
	ret = usb_control_msg(udev, usb_sndctrlpipe(udev, 0),
			USB_REQ_SET_CONFIGURATION, 0,
			udev->actconfig->desc.bConfigurationValue, 0,
			NULL, 0, USB_CTRL_SET_TIMEOUT);
	if (ret < 0) {
		dev_err(&udev->dev,
			"can't restore configuration #%d (error=%d)\n",
			udev->actconfig->desc.bConfigurationValue, ret);
		mutex_unlock(hcd->bandwidth_mutex);
		goto re_enumerate;
	}
	mutex_unlock(hcd->bandwidth_mutex);
	ethub_usb_set_device_state(udev, USB_STATE_CONFIGURED);

	/* Put interfaces back into the same altsettings as before.
	 * Don't bother to send the Set-Interface request for interfaces
	 * that were already in altsetting 0; besides being unnecessary,
	 * many devices can't handle it.  Instead just reset the host-side
	 * endpoint state.
	 */
	for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
		struct usb_host_config *config = udev->actconfig;
		struct usb_interface *intf = config->interface[i];
		struct usb_interface_descriptor *desc;

		desc = &intf->cur_altsetting->desc;
		if (desc->bAlternateSetting == 0) {
			usb_disable_interface(udev, intf, true);
			usb_enable_interface(udev, intf, true);
			ret = 0;
		} else {
			/* Let the bandwidth allocation function know that this
			 * device has been reset, and it will have to use
			 * alternate setting 0 as the current alternate setting.
			 */
			intf->resetting_device = 1;
			ret = usb_set_interface(udev, desc->bInterfaceNumber,
					desc->bAlternateSetting);
			intf->resetting_device = 0;
		}
		if (ret < 0) {
			dev_err(&udev->dev, "failed to restore interface %d "
				"altsetting %d (error=%d)\n",
				desc->bInterfaceNumber,
				desc->bAlternateSetting,
				ret);
			goto re_enumerate;
		}
		/* Resetting also frees any allocated streams */
		for (j = 0; j < intf->cur_altsetting->desc.bNumEndpoints; j++)
			intf->cur_altsetting->endpoint[j].streams = 0;
	}

done:
	usb_release_bos_descriptor(udev);
	udev->bos = bos;
	return 0;

re_enumerate:
	hub_port_logical_disconnect(parent_hub, port1);
	usb_release_bos_descriptor(udev);
	udev->bos = bos;
	return -ENODEV;
}

/**
 * usb_reset_device - warn interface drivers and perform a USB port reset
 * @udev: device to reset (not in SUSPENDED or NOTATTACHED state)
 *
 * Warns all drivers bound to registered interfaces (using their pre_reset
 * method), performs the port reset, and then lets the drivers know that
 * the reset is over (using their post_reset method).
 *
 * Return: The same as for usb_reset_and_verify_device().
 *
 * Note:
 * The caller must own the device lock.  For example, it's safe to use
 * this from a driver probe() routine after downloading new firmware.
 * For calls that might not occur during probe(), drivers should lock
 * the device using usb_lock_device_for_reset().
 *
 * If an interface is currently being probed or disconnected, we assume
 * its driver knows how to handle resets.  For all other interfaces,
 * if the driver doesn't have pre_reset and post_reset methods then
 * we attempt to unbind it and rebind afterward.
 */
int ethub_usb_reset_device(struct usb_device *udev)
{
	int ret;
	int i;
	unsigned int noio_flag;
	struct usb_port *port_dev;
	struct usb_host_config *config = udev->actconfig;
	struct usb_hub *hub = usb_hub_to_struct_hub(udev->parent);

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED) {
		dev_dbg(&udev->dev, "device reset not allowed in state %d\n",
				udev->state);
		return -EINVAL;
	}

	if (!udev->parent) {
		/* this requires hcd-specific logic; see ohci_restart() */
		dev_dbg(&udev->dev, "%s for root hub!\n", __func__);
		return -EISDIR;
	}

	port_dev = hub->ports[udev->portnum - 1];

	/*
	 * Don't allocate memory with GFP_KERNEL in current
	 * context to avoid possible deadlock if usb mass
	 * storage interface or usbnet interface(iSCSI case)
	 * is included in current configuration. The easist
	 * approach is to do it for every device reset,
	 * because the device 'memalloc_noio' flag may have
	 * not been set before reseting the usb device.
	 */
	noio_flag = memalloc_noio_save();

	/* Prevent autosuspend during the reset */
	usb_autoresume_device(udev);

	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			struct usb_interface *cintf = config->interface[i];
			struct usb_driver *drv;
			int unbind = 0;

			if (cintf->dev.driver) {
				drv = to_usb_driver(cintf->dev.driver);
				if (drv->pre_reset && drv->post_reset)
					unbind = (drv->pre_reset)(cintf);
				else if (cintf->condition ==
						USB_INTERFACE_BOUND)
					unbind = 1;
				if (unbind)
					usb_forced_unbind_intf(cintf);
			}
		}
	}

	usb_lock_port(port_dev);
	ret = usb_reset_and_verify_device(udev);
	usb_unlock_port(port_dev);

	if (config) {
		for (i = config->desc.bNumInterfaces - 1; i >= 0; --i) {
			struct usb_interface *cintf = config->interface[i];
			struct usb_driver *drv;
			int rebind = cintf->needs_binding;

			if (!rebind && cintf->dev.driver) {
				drv = to_usb_driver(cintf->dev.driver);
				if (drv->post_reset)
					rebind = (drv->post_reset)(cintf);
				else if (cintf->condition ==
						USB_INTERFACE_BOUND)
					rebind = 1;
				if (rebind)
					cintf->needs_binding = 1;
			}
		}
		usb_unbind_and_rebind_marked_interfaces(udev);
	}

	usb_autosuspend_device(udev);
	memalloc_noio_restore(noio_flag);
	return ret;
}

int usb_is_etron_hcd(struct usb_device *udev)
{
	struct usb_hcd *hcd = bus_to_hcd(udev->bus);

	if (hcd->chip_id == HCD_CHIP_ID_ETRON_EJ168 ||
		hcd->chip_id == HCD_CHIP_ID_ETRON_EJ188)
		return 1;

	return 0;
}
EXPORT_SYMBOL_GPL(usb_is_etron_hcd);

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/slab.h>
#include <linux/pm_qos.h>
#include <linux/gpio.h>

#include "hub.h"

static const struct attribute_group *port_dev_group[];
extern inline int hub_is_superspeed(struct usb_device *hdev);

#ifdef MY_ABC_HERE
extern u32 syno_pch_lpc_gpio_pin(int pin, int *pValue, int isWrite);
#endif  
static ssize_t show_port_connect_type(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct usb_port *port_dev = to_usb_port(dev);
	char *result;

	switch (port_dev->connect_type) {
	case USB_PORT_CONNECT_TYPE_HOT_PLUG:
		result = "hotplug";
		break;
	case USB_PORT_CONNECT_TYPE_HARD_WIRED:
		result = "hardwired";
		break;
	case USB_PORT_NOT_USED:
		result = "not used";
		break;
	default:
		result = "unknown";
		break;
	}

	return sprintf(buf, "%s\n", result);
}
static DEVICE_ATTR(connect_type, S_IRUGO, show_port_connect_type,
		NULL);

static struct attribute *port_dev_attrs[] = {
	&dev_attr_connect_type.attr,
	NULL,
};

static struct attribute_group port_dev_attr_grp = {
	.attrs = port_dev_attrs,
};

static const struct attribute_group *port_dev_group[] = {
	&port_dev_attr_grp,
	NULL,
};

static void usb_port_device_release(struct device *dev)
{
	struct usb_port *port_dev = to_usb_port(dev);

	kfree(port_dev);
}

#ifdef CONFIG_PM_RUNTIME
static int usb_port_runtime_resume(struct device *dev)
{
	struct usb_port *port_dev = to_usb_port(dev);
	struct usb_device *hdev = to_usb_device(dev->parent->parent);
	struct usb_interface *intf = to_usb_interface(dev->parent);
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);
	int port1 = port_dev->portnum;
	int retval;

	if (!hub)
		return -EINVAL;

	usb_autopm_get_interface(intf);
	set_bit(port1, hub->busy_bits);

	retval = usb_hub_set_port_power(hdev, port1, true);
	if (port_dev->child && !retval) {
		 
		retval = hub_port_debounce_be_connected(hub, port1);
		if (retval < 0)
			dev_dbg(&port_dev->dev, "can't get reconnection after setting port  power on, status %d\n",
					retval);
		usb_clear_port_feature(hdev, port1, USB_PORT_FEAT_C_ENABLE);
		retval = 0;
	}

	clear_bit(port1, hub->busy_bits);
	usb_autopm_put_interface(intf);
	return retval;
}

static int usb_port_runtime_suspend(struct device *dev)
{
	struct usb_port *port_dev = to_usb_port(dev);
	struct usb_device *hdev = to_usb_device(dev->parent->parent);
	struct usb_interface *intf = to_usb_interface(dev->parent);
	struct usb_hub *hub = usb_hub_to_struct_hub(hdev);
	int port1 = port_dev->portnum;
	int retval;

	if (!hub)
		return -EINVAL;

	if (dev_pm_qos_flags(&port_dev->dev, PM_QOS_FLAG_NO_POWER_OFF)
			== PM_QOS_FLAGS_ALL)
		return -EAGAIN;

	usb_autopm_get_interface(intf);
	set_bit(port1, hub->busy_bits);
	retval = usb_hub_set_port_power(hdev, port1, false);
	usb_clear_port_feature(hdev, port1, USB_PORT_FEAT_C_CONNECTION);
	usb_clear_port_feature(hdev, port1,	USB_PORT_FEAT_C_ENABLE);
	clear_bit(port1, hub->busy_bits);
	usb_autopm_put_interface(intf);
	return retval;
}
#endif

static const struct dev_pm_ops usb_port_pm_ops = {
#ifdef CONFIG_PM_RUNTIME
	.runtime_suspend =	usb_port_runtime_suspend,
	.runtime_resume =	usb_port_runtime_resume,
	.runtime_idle =		pm_generic_runtime_idle,
#endif
};

struct device_type usb_port_device_type = {
	.name =		"usb_port",
	.release =	usb_port_device_release,
	.pm =		&usb_port_pm_ops,
};

int usb_hub_create_port_device(struct usb_hub *hub, int port1)
{
	struct usb_port *port_dev = NULL;
	int retval;
#if defined(MY_DEF_HERE) ||\
	defined(MY_DEF_HERE)
	struct usb_device *hdev = hub->hdev;
	int i = 0;
#endif  
#ifdef MY_DEF_HERE
	extern char gSynoCastratedXhcAddr[CONFIG_SYNO_NUM_CASTRATED_XHC][13];
	extern unsigned gSynoCastratedXhcPortBitmap[CONFIG_SYNO_NUM_CASTRATED_XHC];
#endif  
#ifdef MY_DEF_HERE
	extern char gSynoUsbVbusHostAddr[CONFIG_SYNO_USB_VBUS_NUM_GPIO][13];
	extern int gSynoUsbVbusPort[CONFIG_SYNO_USB_VBUS_NUM_GPIO];
	extern unsigned gSynoUsbVbusGpp[CONFIG_SYNO_USB_VBUS_NUM_GPIO];
	int value = 0;
#endif  

	port_dev = kzalloc(sizeof(*port_dev), GFP_KERNEL);
	if (!port_dev) {
		retval = -ENOMEM;
		goto exit;
	}

	hub->ports[port1 - 1] = port_dev;
	port_dev->portnum = port1;
	port_dev->power_is_on = true;
	port_dev->dev.parent = hub->intfdev;
	port_dev->dev.groups = port_dev_group;
	port_dev->dev.type = &usb_port_device_type;
	dev_set_name(&port_dev->dev, "port%d", port1);
#if defined (MY_DEF_HERE)
	port_dev->power_cycle_counter = SYNO_POWER_CYCLE_TRIES;
#endif  

#ifdef MY_DEF_HERE
	if (hdev && hdev->serial) {
		for (i = 0; i < CONFIG_SYNO_NUM_CASTRATED_XHC; i++) {
			if (0 == strcmp(gSynoCastratedXhcAddr[i], hdev->serial) &&
				gSynoCastratedXhcPortBitmap[i] & (0x01 << (port1 - 1))) {
				 
				port_dev->flag |= SYNO_USB_PORT_CASTRATED_XHC;
				if (hub_is_superspeed(hdev))
					dev_info (&port_dev->dev, "is a castrated xHC-port\n");
			}
		}
	}
#endif  

#ifdef MY_DEF_HERE
	if (hdev && hdev->serial) {
		for (i = 0; i < CONFIG_SYNO_USB_VBUS_NUM_GPIO; i++) {
			if (0 == strcmp(gSynoUsbVbusHostAddr[i], hdev->serial)) {
#ifdef MY_ABC_HERE
				value = 0;
				if (0 == syno_pch_lpc_gpio_pin(gSynoUsbVbusGpp[i], &value, 0) && 0 == value) {
					value = 1;
					if (0 == syno_pch_lpc_gpio_pin(gSynoUsbVbusGpp[i], &value, 1)) {
						printk(KERN_INFO " port%d is going to power up Vbus by "
								"GPIO#%d\n", port1, gSynoUsbVbusGpp[i]);
						mdelay(100);
					}
				}
#else  
				if (0 == gpio_get_value(gSynoUsbVbusGpp[i])) {
					gpio_set_value(gSynoUsbVbusGpp[i], 1);
					printk(KERN_INFO " port%d is going to power up Vbus by "
							"GPIO#%d\n", port1, gSynoUsbVbusGpp[i]);
					mdelay(100);
				}
#endif  
				if (port1 == gSynoUsbVbusPort[i])
					port_dev->syno_vbus_gpp = gSynoUsbVbusGpp[i];
			}
		}
	}
#endif  
	retval = device_register(&port_dev->dev);
	if (retval)
		goto error_register;

	pm_runtime_set_active(&port_dev->dev);

	if (!dev_pm_qos_expose_flags(&port_dev->dev,
			PM_QOS_FLAG_NO_POWER_OFF))
		pm_runtime_enable(&port_dev->dev);

	device_enable_async_suspend(&port_dev->dev);
	return 0;

error_register:
	put_device(&port_dev->dev);
exit:
	return retval;
}

void usb_hub_remove_port_device(struct usb_hub *hub,
				       int port1)
{
	device_unregister(&hub->ports[port1 - 1]->dev);
}

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include "usb.h"

static const struct usb_device_id usb_quirk_list[] = {
	 
	{ USB_DEVICE(0x0204, 0x6025), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x03f0, 0x0701), .driver_info =
			USB_QUIRK_STRING_FETCH_255 },

	{ USB_DEVICE(0x041e, 0x3020), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x045e, 0x0770), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x046d, 0x08c1), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x046d, 0x08c2), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x046d, 0x08c3), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x046d, 0x08c5), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x046d, 0x08c6), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x046d, 0x08c7), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x046d, 0xc122), .driver_info = USB_QUIRK_DELAY_INIT },

	{ USB_DEVICE(0x0471, 0x0155), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x04b4, 0x0526), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	{ USB_DEVICE(0x04d8, 0x000c), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	{ USB_DEVICE(0x04e8, 0x6601), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	{ USB_DEVICE(0x0582, 0x0007), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x0582, 0x0027), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x05ac, 0x021a), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x0638, 0x0a13), .driver_info =
	  USB_QUIRK_STRING_FETCH_255 },

	{ USB_DEVICE(0x06a3, 0x0006), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	{ USB_DEVICE(0x06f8, 0x0804), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x06f8, 0x3005), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x0763, 0x0192), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x08ec, 0x1000), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x0926, 0x3333), .driver_info =
			USB_QUIRK_CONFIG_INTF_STRINGS },

	{ USB_DEVICE(0x0971, 0x2000), .driver_info = USB_QUIRK_NO_SET_INTF },

	{ USB_DEVICE(0x0a5c, 0x2021), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x10d6, 0x2200), .driver_info =
			USB_QUIRK_STRING_FETCH_255 },

	{ USB_DEVICE(0x1516, 0x8628), .driver_info = USB_QUIRK_RESET_RESUME },

	{ USB_DEVICE(0x1908, 0x1315), .driver_info =
			USB_QUIRK_HONOR_BNUMINTERFACES },

	{ USB_DEVICE(0x8086, 0xf1a5), .driver_info = USB_QUIRK_RESET_RESUME },

	{ }   
};

static const struct usb_device_id usb_interface_quirk_list[] = {
	 
	{ USB_VENDOR_AND_INTERFACE_INFO(0x046d, USB_CLASS_VIDEO, 1, 0),
	  .driver_info = USB_QUIRK_RESET_RESUME },

	{ }   
};

static bool usb_match_any_interface(struct usb_device *udev,
				    const struct usb_device_id *id)
{
	unsigned int i;

	for (i = 0; i < udev->descriptor.bNumConfigurations; ++i) {
		struct usb_host_config *cfg = &udev->config[i];
		unsigned int j;

		for (j = 0; j < cfg->desc.bNumInterfaces; ++j) {
			struct usb_interface_cache *cache;
			struct usb_host_interface *intf;

			cache = cfg->intf_cache[j];
			if (cache->num_altsetting == 0)
				continue;

			intf = &cache->altsetting[0];
			if (usb_match_one_id_intf(udev, intf, id))
				return true;
		}
	}

	return false;
}

static u32 __usb_detect_quirks(struct usb_device *udev,
			       const struct usb_device_id *id)
{
	u32 quirks = 0;

	for (; id->match_flags; id++) {
		if (!usb_match_device(udev, id))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_INFO) &&
		    !usb_match_any_interface(udev, id))
			continue;

		quirks |= (u32)(id->driver_info);
	}

	return quirks;
}

void usb_detect_quirks(struct usb_device *udev)
{
	udev->quirks = __usb_detect_quirks(udev, usb_quirk_list);
	if (udev->quirks)
		dev_dbg(&udev->dev, "USB quirks for this device: %x\n",
			udev->quirks);

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	 
#ifdef CONFIG_PM
	udev->quirks = USB_QUIRK_RESET_RESUME;
#endif
#endif

#if 0		 
	 
	if (udev->descriptor.bDeviceClass == USB_CLASS_HUB)
		udev->persist_enabled = 1;

#else
	 
	if (!(udev->quirks & USB_QUIRK_RESET_MORPHS))
		udev->persist_enabled = 1;
#endif	 
}

void usb_detect_interface_quirks(struct usb_device *udev)
{
	u32 quirks;

	quirks = __usb_detect_quirks(udev, usb_interface_quirk_list);
	if (quirks == 0)
		return;

	dev_dbg(&udev->dev, "USB interface quirks for this device: %x\n",
		quirks);
	udev->quirks |= quirks;
}

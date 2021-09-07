#include <linux/slab.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/usb/quirks.h>
#include <linux/usb.h>

#include "usb.h"
#include "hcd.h"


/**
 * usb_hcd_alloc_bandwidth - check whether a new bandwidth setting exceeds
 *				the bus bandwidth
 * @udev: target &usb_device
 * @new_config: new configuration to install
 * @cur_alt: the current alternate interface setting
 * @new_alt: alternate interface setting that is being installed
 *
 * To change configurations, pass in the new configuration in new_config,
 * and pass NULL for cur_alt and new_alt.
 *
 * To reset a device's configuration (put the device in the ADDRESSED state),
 * pass in NULL for new_config, cur_alt, and new_alt.
 *
 * To change alternate interface settings, pass in NULL for new_config,
 * pass in the current alternate interface setting in cur_alt,
 * and pass in the new alternate interface setting in new_alt.
 *
 * Returns an error if the requested bandwidth change exceeds the
 * bus bandwidth or host controller internal resources.
 */
int etapi_usb_hcd_alloc_bandwidth(struct usb_device *udev,
		struct usb_host_config *new_config,
		struct usb_host_interface *cur_alt,
		struct usb_host_interface *new_alt)
{
	int num_intfs, i, j;
	struct usb_host_interface *alt = NULL;
	int ret = 0;
	struct usb_hcd *hcd;
	struct usb_host_endpoint *ep;

	hcd = bus_to_hcd(udev->bus);
	if (!hcd->driver->check_bandwidth)
		return 0;

	/* Configuration is being removed - set configuration 0 */
	if (!new_config && !cur_alt) {
		for (i = 1; i < 16; ++i) {
			ep = udev->ep_out[i];
			if (ep)
				hcd->driver->drop_endpoint(hcd, udev, ep);
			ep = udev->ep_in[i];
			if (ep)
				hcd->driver->drop_endpoint(hcd, udev, ep);
		}
		hcd->driver->check_bandwidth(hcd, udev);
		return 0;
	}
	/* Check if the HCD says there's enough bandwidth.  Enable all endpoints
	 * each interface's alt setting 0 and ask the HCD to check the bandwidth
	 * of the bus.  There will always be bandwidth for endpoint 0, so it's
	 * ok to exclude it.
	 */
	if (new_config) {
		num_intfs = new_config->desc.bNumInterfaces;
		/* Remove endpoints (except endpoint 0, which is always on the
		 * schedule) from the old config from the schedule
		 */
		for (i = 1; i < 16; ++i) {
			ep = udev->ep_out[i];
			if (ep) {
				ret = hcd->driver->drop_endpoint(hcd, udev, ep);
				if (ret < 0)
					goto reset;
			}
			ep = udev->ep_in[i];
			if (ep) {
				ret = hcd->driver->drop_endpoint(hcd, udev, ep);
				if (ret < 0)
					goto reset;
			}
		}
		for (i = 0; i < num_intfs; ++i) {
			struct usb_host_interface *first_alt;
			int iface_num;

			first_alt = &new_config->intf_cache[i]->altsetting[0];
			iface_num = first_alt->desc.bInterfaceNumber;
			/* Set up endpoints for alternate interface setting 0 */
			alt = usb_find_alt_setting(new_config, iface_num, 0);
			if (!alt)
				/* No alt setting 0? Pick the first setting. */
				alt = first_alt;

			for (j = 0; j < alt->desc.bNumEndpoints; j++) {
				ret = hcd->driver->add_endpoint(hcd, udev, &alt->endpoint[j]);
				if (ret < 0)
					goto reset;
			}
		}
	}
	if (cur_alt && new_alt) {
		struct usb_interface *iface = usb_ifnum_to_if(udev,
				cur_alt->desc.bInterfaceNumber);

		if (iface->resetting_device) {
			/*
			 * The USB core just reset the device, so the xHCI host
			 * and the device will think alt setting 0 is installed.
			 * However, the USB core will pass in the alternate
			 * setting installed before the reset as cur_alt.  Dig
			 * out the alternate setting 0 structure, or the first
			 * alternate setting if a broken device doesn't have alt
			 * setting 0.
			 */
			cur_alt = usb_altnum_to_altsetting(iface, 0);
			if (!cur_alt)
				cur_alt = &iface->altsetting[0];
		}

		/* Drop all the endpoints in the current alt setting */
		for (i = 0; i < cur_alt->desc.bNumEndpoints; i++) {
			ret = hcd->driver->drop_endpoint(hcd, udev,
					&cur_alt->endpoint[i]);
			if (ret < 0)
				goto reset;
		}
		/* Add all the endpoints in the new alt setting */
		for (i = 0; i < new_alt->desc.bNumEndpoints; i++) {
			ret = hcd->driver->add_endpoint(hcd, udev,
					&new_alt->endpoint[i]);
			if (ret < 0)
				goto reset;
		}
	}
	ret = hcd->driver->check_bandwidth(hcd, udev);
reset:
	if (ret < 0)
		hcd->driver->reset_bandwidth(hcd, udev);
	return ret;
}

/**
 * usb_alloc_streams - allocate bulk endpoint stream IDs.
 * @interface:		alternate setting that includes all endpoints.
 * @eps:		array of endpoints that need streams.
 * @num_eps:		number of endpoints in the array.
 * @num_streams:	number of streams to allocate.
 * @mem_flags:		flags hcd should use to allocate memory.
 *
 * Sets up a group of bulk endpoints to have num_streams stream IDs available.
 * Drivers may queue multiple transfers to different stream IDs, which may
 * complete in a different order than they were queued.
 */
int usb_alloc_streams(struct usb_interface *interface,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		unsigned int num_streams, gfp_t mem_flags)
{
	struct usb_hcd *hcd;
	struct usb_device *dev;
	int i;

	dev = interface_to_usbdev(interface);
	hcd = bus_to_hcd(dev->bus);
	if (!hcd->driver->alloc_streams || !hcd->driver->free_streams)
		return -EINVAL;
	if (dev->speed != USB_SPEED_SUPER)
		return -EINVAL;

	/* Streams only apply to bulk endpoints. */
	for (i = 0; i < num_eps; i++)
		if (!usb_endpoint_xfer_bulk(&eps[i]->desc))
			return -EINVAL;

	return hcd->driver->alloc_streams(hcd, dev, eps, num_eps,
			num_streams, mem_flags);
}
EXPORT_SYMBOL_GPL(usb_alloc_streams);

/**
 * usb_free_streams - free bulk endpoint stream IDs.
 * @interface:	alternate setting that includes all endpoints.
 * @eps:	array of endpoints to remove streams from.
 * @num_eps:	number of endpoints in the array.
 * @mem_flags:	flags hcd should use to allocate memory.
 *
 * Reverts a group of bulk endpoints back to not using stream IDs.
 * Can fail if we are given bad arguments, or HCD is broken.
 */
void usb_free_streams(struct usb_interface *interface,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		gfp_t mem_flags)
{
	struct usb_hcd *hcd;
	struct usb_device *dev;
	int i;

	dev = interface_to_usbdev(interface);
	hcd = bus_to_hcd(dev->bus);
	if (dev->speed != USB_SPEED_SUPER)
		return;

	/* Streams only apply to bulk endpoints. */
	for (i = 0; i < num_eps; i++)
		if (!eps[i] || !usb_endpoint_xfer_bulk(&eps[i]->desc))
			return;

	hcd->driver->free_streams(hcd, dev, eps, num_eps, mem_flags);
}
EXPORT_SYMBOL_GPL(usb_free_streams);

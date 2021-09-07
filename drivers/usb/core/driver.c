#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/device.h>
#include <linux/usb.h>
#include <linux/usb/quirks.h>
#include <linux/workqueue.h>
#include "hcd.h"
#include "usb.h"

#ifdef MY_ABC_HERE
#include <linux/synobios.h>
#endif

#ifdef MY_ABC_HERE
extern int blIsUSBDeviceAtFrontPort(struct usb_device *usbdev);
#endif

#ifdef MY_ABC_HERE
extern int (*funcSYNOGetHwCapability)(CAPABILITY *);
extern int blIsCardReader(struct usb_device *usbdev);

static unsigned char has_cardreader(void)
{
	CAPABILITY Capability;
	unsigned char ret = 0;

	Capability.id = CAPABILITY_CARDREADER;
	Capability.support = 0;

	if (funcSYNOGetHwCapability) {
		if (funcSYNOGetHwCapability(&Capability)){
			goto END;
		}
	}

	ret = Capability.support;
END:
	return ret;
}
#endif

#ifdef CONFIG_HOTPLUG

ssize_t usb_store_new_id(struct usb_dynids *dynids,
			 struct device_driver *driver,
			 const char *buf, size_t count)
{
	struct usb_dynid *dynid;
	u32 idVendor = 0;
	u32 idProduct = 0;
	int fields = 0;
	int retval = 0;

	fields = sscanf(buf, "%x %x", &idVendor, &idProduct);
	if (fields < 2)
		return -EINVAL;

	dynid = kzalloc(sizeof(*dynid), GFP_KERNEL);
	if (!dynid)
		return -ENOMEM;

	INIT_LIST_HEAD(&dynid->node);
	dynid->id.idVendor = idVendor;
	dynid->id.idProduct = idProduct;
	dynid->id.match_flags = USB_DEVICE_ID_MATCH_DEVICE;

	spin_lock(&dynids->lock);
	list_add_tail(&dynid->node, &dynids->list);
	spin_unlock(&dynids->lock);

	if (get_driver(driver)) {
		retval = driver_attach(driver);
		put_driver(driver);
	}

	if (retval)
		return retval;
	return count;
}
EXPORT_SYMBOL_GPL(usb_store_new_id);

static ssize_t store_new_id(struct device_driver *driver,
			    const char *buf, size_t count)
{
	struct usb_driver *usb_drv = to_usb_driver(driver);

	return usb_store_new_id(&usb_drv->dynids, driver, buf, count);
}
static DRIVER_ATTR(new_id, S_IWUSR, NULL, store_new_id);

static ssize_t
store_remove_id(struct device_driver *driver, const char *buf, size_t count)
{
	struct usb_dynid *dynid, *n;
	struct usb_driver *usb_driver = to_usb_driver(driver);
	u32 idVendor = 0;
	u32 idProduct = 0;
	int fields = 0;
	int retval = 0;

	fields = sscanf(buf, "%x %x", &idVendor, &idProduct);
	if (fields < 2)
		return -EINVAL;

	spin_lock(&usb_driver->dynids.lock);
	list_for_each_entry_safe(dynid, n, &usb_driver->dynids.list, node) {
		struct usb_device_id *id = &dynid->id;
		if ((id->idVendor == idVendor) &&
		    (id->idProduct == idProduct)) {
			list_del(&dynid->node);
			kfree(dynid);
			retval = 0;
			break;
		}
	}
	spin_unlock(&usb_driver->dynids.lock);

	if (retval)
		return retval;
	return count;
}
static DRIVER_ATTR(remove_id, S_IWUSR, NULL, store_remove_id);

static int usb_create_newid_file(struct usb_driver *usb_drv)
{
	int error = 0;

	if (usb_drv->no_dynamic_id)
		goto exit;

	if (usb_drv->probe != NULL)
		error = driver_create_file(&usb_drv->drvwrap.driver,
					   &driver_attr_new_id);
exit:
	return error;
}

static void usb_remove_newid_file(struct usb_driver *usb_drv)
{
	if (usb_drv->no_dynamic_id)
		return;

	if (usb_drv->probe != NULL)
		driver_remove_file(&usb_drv->drvwrap.driver,
				   &driver_attr_new_id);
}

static int
usb_create_removeid_file(struct usb_driver *drv)
{
	int error = 0;
	if (drv->probe != NULL)
		error = driver_create_file(&drv->drvwrap.driver,
				&driver_attr_remove_id);
	return error;
}

static void usb_remove_removeid_file(struct usb_driver *drv)
{
	driver_remove_file(&drv->drvwrap.driver, &driver_attr_remove_id);
}

static void usb_free_dynids(struct usb_driver *usb_drv)
{
	struct usb_dynid *dynid, *n;

	spin_lock(&usb_drv->dynids.lock);
	list_for_each_entry_safe(dynid, n, &usb_drv->dynids.list, node) {
		list_del(&dynid->node);
		kfree(dynid);
	}
	spin_unlock(&usb_drv->dynids.lock);
}
#else
static inline int usb_create_newid_file(struct usb_driver *usb_drv)
{
	return 0;
}

static void usb_remove_newid_file(struct usb_driver *usb_drv)
{
}

static int
usb_create_removeid_file(struct usb_driver *drv)
{
	return 0;
}

static void usb_remove_removeid_file(struct usb_driver *drv)
{
}

static inline void usb_free_dynids(struct usb_driver *usb_drv)
{
}
#endif

static const struct usb_device_id *usb_match_dynamic_id(struct usb_interface *intf,
							struct usb_driver *drv)
{
	struct usb_dynid *dynid;

	spin_lock(&drv->dynids.lock);
	list_for_each_entry(dynid, &drv->dynids.list, node) {
		if (usb_match_one_id(intf, &dynid->id)) {
			spin_unlock(&drv->dynids.lock);
			return &dynid->id;
		}
	}
	spin_unlock(&drv->dynids.lock);
	return NULL;
}

static int usb_probe_device(struct device *dev)
{
	struct usb_device_driver *udriver = to_usb_device_driver(dev->driver);
	struct usb_device *udev = to_usb_device(dev);
	int error = -ENODEV;

	dev_dbg(dev, "%s\n", __func__);

	udev->pm_usage_cnt = !(udriver->supports_autosuspend);

	error = udriver->probe(udev);
	return error;
}

static int usb_unbind_device(struct device *dev)
{
	struct usb_device_driver *udriver = to_usb_device_driver(dev->driver);

	udriver->disconnect(to_usb_device(dev));
	return 0;
}

static void usb_cancel_queued_reset(struct usb_interface *iface)
{
	if (iface->reset_running == 0)
		cancel_work_sync(&iface->reset_ws);
}

static int usb_probe_interface(struct device *dev)
{
	struct usb_driver *driver = to_usb_driver(dev->driver);
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_device *udev = interface_to_usbdev(intf);
	const struct usb_device_id *id;
	int error = -ENODEV;

	dev_dbg(dev, "%s\n", __func__);

	intf->needs_binding = 0;

	if (usb_device_is_owned(udev))
		return -ENODEV;

	if (udev->authorized == 0) {
		dev_err(&intf->dev, "Device is not authorized for usage\n");
		return -ENODEV;
	}

	id = usb_match_id(intf, driver->id_table);
	if (!id)
		id = usb_match_dynamic_id(intf, driver);
	if (id) {
		dev_dbg(dev, "%s - got id\n", __func__);

		error = usb_autoresume_device(udev);
		if (error)
			return error;

		mark_active(intf);
		intf->condition = USB_INTERFACE_BINDING;

		atomic_set(&intf->pm_usage_cnt, !driver->supports_autosuspend);

		if (intf->needs_altsetting0) {
			error = usb_set_interface(udev, intf->altsetting[0].
					desc.bInterfaceNumber, 0);
			if (error < 0)
				goto err;

			intf->needs_altsetting0 = 0;
		}

		error = driver->probe(intf, id);
		if (error)
			goto err;

		intf->condition = USB_INTERFACE_BOUND;
		usb_autosuspend_device(udev);
	}

	return error;

err:
	mark_quiesced(intf);
	intf->needs_remote_wakeup = 0;
	intf->condition = USB_INTERFACE_UNBOUND;
	usb_cancel_queued_reset(intf);
	usb_autosuspend_device(udev);
	return error;
}

static int usb_unbind_interface(struct device *dev)
{
	struct usb_driver *driver = to_usb_driver(dev->driver);
	struct usb_interface *intf = to_usb_interface(dev);
	struct usb_device *udev;
	int error, r;

	intf->condition = USB_INTERFACE_UNBINDING;

	udev = interface_to_usbdev(intf);
	error = usb_autoresume_device(udev);

	if (!driver->soft_unbind)
		usb_disable_interface(udev, intf, false);

	driver->disconnect(intf);
	usb_cancel_queued_reset(intf);

	if (intf->cur_altsetting->desc.bAlternateSetting == 0) {
		 
		usb_enable_interface(udev, intf, false);
	} else if (!error && intf->dev.power.status == DPM_ON) {
		r = usb_set_interface(udev, intf->altsetting[0].
				desc.bInterfaceNumber, 0);
		if (r < 0)
			intf->needs_altsetting0 = 1;
	} else {
		intf->needs_altsetting0 = 1;
	}
	usb_set_intfdata(intf, NULL);

	intf->condition = USB_INTERFACE_UNBOUND;
	mark_quiesced(intf);
	intf->needs_remote_wakeup = 0;

	if (!error)
		usb_autosuspend_device(udev);

	return 0;
}

int usb_driver_claim_interface(struct usb_driver *driver,
				struct usb_interface *iface, void *priv)
{
	struct device *dev = &iface->dev;
	struct usb_device *udev = interface_to_usbdev(iface);
	int retval = 0;

	if (dev->driver)
		return -EBUSY;

	dev->driver = &driver->drvwrap.driver;
	usb_set_intfdata(iface, priv);
	iface->needs_binding = 0;

	usb_pm_lock(udev);
	iface->condition = USB_INTERFACE_BOUND;
	mark_active(iface);
	atomic_set(&iface->pm_usage_cnt, !driver->supports_autosuspend);
	usb_pm_unlock(udev);

	if (device_is_registered(dev))
		retval = device_bind_driver(dev);

	return retval;
}
EXPORT_SYMBOL_GPL(usb_driver_claim_interface);

void usb_driver_release_interface(struct usb_driver *driver,
					struct usb_interface *iface)
{
	struct device *dev = &iface->dev;

	if (!dev->driver || dev->driver != &driver->drvwrap.driver)
		return;

	if (iface->condition != USB_INTERFACE_BOUND)
		return;
	iface->condition = USB_INTERFACE_UNBINDING;

	if (device_is_registered(dev)) {
		device_release_driver(dev);
	} else {
		down(&dev->sem);
		usb_unbind_interface(dev);
		dev->driver = NULL;
		up(&dev->sem);
	}
}
EXPORT_SYMBOL_GPL(usb_driver_release_interface);

int usb_match_device(struct usb_device *dev, const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
	    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
	    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
	    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
	    (id->bDeviceClass != dev->descriptor.bDeviceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
	    (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
	    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
		return 0;

	return 1;
}

int usb_match_one_id(struct usb_interface *interface,
		     const struct usb_device_id *id)
{
	struct usb_host_interface *intf;
	struct usb_device *dev;

	if (id == NULL)
		return 0;

	intf = interface->cur_altsetting;
	dev = interface_to_usbdev(interface);

	if (!usb_match_device(dev, id))
		return 0;

	if (dev->descriptor.bDeviceClass == USB_CLASS_VENDOR_SPEC &&
			!(id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
			(id->match_flags & (USB_DEVICE_ID_MATCH_INT_CLASS |
				USB_DEVICE_ID_MATCH_INT_SUBCLASS |
				USB_DEVICE_ID_MATCH_INT_PROTOCOL |
				USB_DEVICE_ID_MATCH_INT_NUMBER)))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
	    (id->bInterfaceClass != intf->desc.bInterfaceClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
	    (id->bInterfaceSubClass != intf->desc.bInterfaceSubClass))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
	    (id->bInterfaceProtocol != intf->desc.bInterfaceProtocol))
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_NUMBER) &&
	    (id->bInterfaceNumber != intf->desc.bInterfaceNumber))
		return 0;

	return 1;
}
EXPORT_SYMBOL_GPL(usb_match_one_id);

const struct usb_device_id *usb_match_id(struct usb_interface *interface,
					 const struct usb_device_id *id)
{
	 
	if (id == NULL)
		return NULL;

	for (; id->idVendor || id->idProduct || id->bDeviceClass ||
	       id->bInterfaceClass || id->driver_info; id++) {
		if (usb_match_one_id(interface, id))
			return id;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(usb_match_id);

static int usb_device_match(struct device *dev, struct device_driver *drv)
{
	 
	if (is_usb_device(dev)) {

		if (!is_usb_device_driver(drv))
			return 0;

		return 1;

	} else if (is_usb_interface(dev)) {
		struct usb_interface *intf;
		struct usb_driver *usb_drv;
		const struct usb_device_id *id;

		if (is_usb_device_driver(drv))
			return 0;

		intf = to_usb_interface(dev);
		usb_drv = to_usb_driver(drv);

		id = usb_match_id(intf, usb_drv->id_table);
		if (id)
			return 1;

		id = usb_match_dynamic_id(intf, usb_drv);
		if (id)
			return 1;
	}

	return 0;
}

#ifdef	CONFIG_HOTPLUG
static int usb_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct usb_device *usb_dev;

	if (is_usb_device(dev)) {
		usb_dev = to_usb_device(dev);
	} else if (is_usb_interface(dev)) {
		struct usb_interface *intf = to_usb_interface(dev);

		usb_dev = interface_to_usbdev(intf);
	} else {
		return 0;
	}

	if (usb_dev->devnum < 0) {
		 
		pr_debug("usb %s: already deleted?\n", dev_name(dev));
		return -ENODEV;
	}
	if (!usb_dev->bus) {
		pr_debug("usb %s: bus removed?\n", dev_name(dev));
		return -ENODEV;
	}

#ifdef	CONFIG_USB_DEVICEFS
	 
	if (add_uevent_var(env, "DEVICE=/proc/bus/usb/%03d/%03d",
			   usb_dev->bus->busnum, usb_dev->devnum))
		return -ENOMEM;
#endif

	if (add_uevent_var(env, "PRODUCT=%x/%x/%x",
			   le16_to_cpu(usb_dev->descriptor.idVendor),
			   le16_to_cpu(usb_dev->descriptor.idProduct),
			   le16_to_cpu(usb_dev->descriptor.bcdDevice)))
		return -ENOMEM;

	if (add_uevent_var(env, "TYPE=%d/%d/%d",
			   usb_dev->descriptor.bDeviceClass,
			   usb_dev->descriptor.bDeviceSubClass,
			   usb_dev->descriptor.bDeviceProtocol))
		return -ENOMEM;

#ifdef MY_ABC_HERE
	if (blIsUSBDeviceAtFrontPort(usb_dev)) {
		if (add_uevent_var(env, "FRONTPORT=1"))
			return -ENOMEM;
	}
#endif

#ifdef MY_ABC_HERE
	if(has_cardreader()) {
		if (blIsCardReader(usb_dev)) {
			if (add_uevent_var(env, "CARDREADER=1"))
				return -ENOMEM;
		}
	}
#endif

	return 0;
}

#else

static int usb_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return -ENODEV;
}
#endif	 

int usb_register_device_driver(struct usb_device_driver *new_udriver,
		struct module *owner)
{
	int retval = 0;

	if (usb_disabled())
		return -ENODEV;

	new_udriver->drvwrap.for_devices = 1;
	new_udriver->drvwrap.driver.name = (char *) new_udriver->name;
	new_udriver->drvwrap.driver.bus = &usb_bus_type;
	new_udriver->drvwrap.driver.probe = usb_probe_device;
	new_udriver->drvwrap.driver.remove = usb_unbind_device;
	new_udriver->drvwrap.driver.owner = owner;

	retval = driver_register(&new_udriver->drvwrap.driver);

	if (!retval) {
		pr_info("%s: registered new device driver %s\n",
			usbcore_name, new_udriver->name);
		usbfs_update_special();
	} else {
		printk(KERN_ERR "%s: error %d registering device "
			"	driver %s\n",
			usbcore_name, retval, new_udriver->name);
	}

	return retval;
}
EXPORT_SYMBOL_GPL(usb_register_device_driver);

void usb_deregister_device_driver(struct usb_device_driver *udriver)
{
	pr_info("%s: deregistering device driver %s\n",
			usbcore_name, udriver->name);

	driver_unregister(&udriver->drvwrap.driver);
	usbfs_update_special();
}
EXPORT_SYMBOL_GPL(usb_deregister_device_driver);

int usb_register_driver(struct usb_driver *new_driver, struct module *owner,
			const char *mod_name)
{
	int retval = 0;

	if (usb_disabled())
		return -ENODEV;

	new_driver->drvwrap.for_devices = 0;
	new_driver->drvwrap.driver.name = (char *) new_driver->name;
	new_driver->drvwrap.driver.bus = &usb_bus_type;
	new_driver->drvwrap.driver.probe = usb_probe_interface;
	new_driver->drvwrap.driver.remove = usb_unbind_interface;
	new_driver->drvwrap.driver.owner = owner;
	new_driver->drvwrap.driver.mod_name = mod_name;
	spin_lock_init(&new_driver->dynids.lock);
	INIT_LIST_HEAD(&new_driver->dynids.list);

	retval = driver_register(&new_driver->drvwrap.driver);
	if (retval)
		goto out;

	usbfs_update_special();

	retval = usb_create_newid_file(new_driver);
	if (retval)
		goto out_newid;

	retval = usb_create_removeid_file(new_driver);
	if (retval)
		goto out_removeid;

	pr_info("%s: registered new interface driver %s\n",
			usbcore_name, new_driver->name);

out:
	return retval;

out_removeid:
	usb_remove_newid_file(new_driver);
out_newid:
	driver_unregister(&new_driver->drvwrap.driver);

	printk(KERN_ERR "%s: error %d registering interface "
			"	driver %s\n",
			usbcore_name, retval, new_driver->name);
	goto out;
}
EXPORT_SYMBOL_GPL(usb_register_driver);

void usb_deregister(struct usb_driver *driver)
{
	pr_info("%s: deregistering interface driver %s\n",
			usbcore_name, driver->name);

	usb_remove_removeid_file(driver);
	usb_remove_newid_file(driver);
	usb_free_dynids(driver);
	driver_unregister(&driver->drvwrap.driver);

	usbfs_update_special();
}
EXPORT_SYMBOL_GPL(usb_deregister);

void usb_forced_unbind_intf(struct usb_interface *intf)
{
	struct usb_driver *driver = to_usb_driver(intf->dev.driver);

	dev_dbg(&intf->dev, "forced unbind\n");
	usb_driver_release_interface(driver, intf);

	intf->needs_binding = 1;
}

void usb_rebind_intf(struct usb_interface *intf)
{
	int rc;

	if (intf->dev.driver) {
		struct usb_driver *driver =
				to_usb_driver(intf->dev.driver);

		dev_dbg(&intf->dev, "forced unbind\n");
		usb_driver_release_interface(driver, intf);
	}

	if (intf->dev.power.status == DPM_ON) {
		intf->needs_binding = 0;
		rc = device_attach(&intf->dev);
		if (rc < 0)
			dev_warn(&intf->dev, "rebind failed: %d\n", rc);
	}
}

#ifdef CONFIG_PM

#define DO_UNBIND	0
#define DO_REBIND	1

static void do_unbind_rebind(struct usb_device *udev, int action)
{
	struct usb_host_config	*config;
	int			i;
	struct usb_interface	*intf;
	struct usb_driver	*drv;

	config = udev->actconfig;
	if (config) {
		for (i = 0; i < config->desc.bNumInterfaces; ++i) {
			intf = config->interface[i];
			switch (action) {
			case DO_UNBIND:
				if (intf->dev.driver) {
					drv = to_usb_driver(intf->dev.driver);
					if (!drv->suspend || !drv->resume)
						usb_forced_unbind_intf(intf);
				}
				break;
			case DO_REBIND:
				if (intf->needs_binding)
					usb_rebind_intf(intf);
				break;
			}
		}
	}
}

static int usb_suspend_device(struct usb_device *udev, pm_message_t msg)
{
	struct usb_device_driver	*udriver;
	int				status = 0;

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED)
		goto done;

	if (udev->dev.driver)
		udriver = to_usb_device_driver(udev->dev.driver);
	else {
		udev->do_remote_wakeup = 0;
		udriver = &usb_generic_driver;
	}
	status = udriver->suspend(udev, msg);

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	return status;
}

static int usb_resume_device(struct usb_device *udev, pm_message_t msg)
{
	struct usb_device_driver	*udriver;
	int				status = 0;

	if (udev->state == USB_STATE_NOTATTACHED)
		goto done;

	if (udev->dev.driver == NULL) {
		status = -ENOTCONN;
		goto done;
	}

	if (udev->quirks & USB_QUIRK_RESET_RESUME)
		udev->reset_resume = 1;

	udriver = to_usb_device_driver(udev->dev.driver);
	status = udriver->resume(udev, msg);

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	return status;
}

static int usb_suspend_interface(struct usb_device *udev,
		struct usb_interface *intf, pm_message_t msg)
{
	struct usb_driver	*driver;
	int			status = 0;

	if (udev->state == USB_STATE_NOTATTACHED || !is_active(intf))
		goto done;

	if (intf->condition == USB_INTERFACE_UNBOUND)
		goto done;
	driver = to_usb_driver(intf->dev.driver);

	if (driver->suspend) {
		status = driver->suspend(intf, msg);
		if (status == 0)
			mark_quiesced(intf);
		else if (!(msg.event & PM_EVENT_AUTO))
			dev_err(&intf->dev, "%s error %d\n",
					"suspend", status);
	} else {
		 
		intf->needs_binding = 1;
		dev_warn(&intf->dev, "no %s for driver %s?\n",
				"suspend", driver->name);
		mark_quiesced(intf);
	}

 done:
	dev_vdbg(&intf->dev, "%s: status %d\n", __func__, status);
	return status;
}

static int usb_resume_interface(struct usb_device *udev,
		struct usb_interface *intf, pm_message_t msg, int reset_resume)
{
	struct usb_driver	*driver;
	int			status = 0;

	if (udev->state == USB_STATE_NOTATTACHED || is_active(intf))
		goto done;

	if (intf->condition == USB_INTERFACE_UNBINDING)
		goto done;

	if (intf->condition == USB_INTERFACE_UNBOUND) {

		if (intf->needs_altsetting0 &&
				intf->dev.power.status == DPM_ON) {
			usb_set_interface(udev, intf->altsetting[0].
					desc.bInterfaceNumber, 0);
			intf->needs_altsetting0 = 0;
		}
		goto done;
	}

	if (intf->needs_binding)
		goto done;
	driver = to_usb_driver(intf->dev.driver);

	if (reset_resume) {
		if (driver->reset_resume) {
			status = driver->reset_resume(intf);
			if (status)
				dev_err(&intf->dev, "%s error %d\n",
						"reset_resume", status);
		} else {
			intf->needs_binding = 1;
			dev_warn(&intf->dev, "no %s for driver %s?\n",
					"reset_resume", driver->name);
		}
	} else {
		if (driver->resume) {
			status = driver->resume(intf);
			if (status)
				dev_err(&intf->dev, "%s error %d\n",
						"resume", status);
		} else {
			intf->needs_binding = 1;
			dev_warn(&intf->dev, "no %s for driver %s?\n",
					"resume", driver->name);
		}
	}

done:
	dev_vdbg(&intf->dev, "%s: status %d\n", __func__, status);
	if (status == 0 && intf->condition == USB_INTERFACE_BOUND)
		mark_active(intf);

	return status;
}

#ifdef	CONFIG_USB_SUSPEND

static int autosuspend_check(struct usb_device *udev, int reschedule)
{
	int			i;
	struct usb_interface	*intf;
	unsigned long		suspend_time, j;

	if (udev->pm_usage_cnt > 0)
		return -EBUSY;
	if (udev->autosuspend_delay < 0 || udev->autosuspend_disabled)
		return -EPERM;

	suspend_time = udev->last_busy + udev->autosuspend_delay;
	if (udev->actconfig) {
		for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
			intf = udev->actconfig->interface[i];
			if (!is_active(intf))
				continue;
			if (atomic_read(&intf->pm_usage_cnt) > 0)
				return -EBUSY;
			if (intf->needs_remote_wakeup &&
					!udev->do_remote_wakeup) {
				dev_dbg(&udev->dev, "remote wakeup needed "
						"for autosuspend\n");
				return -EOPNOTSUPP;
			}

			if (udev->quirks & USB_QUIRK_RESET_RESUME) {
				struct usb_driver *driver;

				driver = to_usb_driver(intf->dev.driver);
				if (!driver->reset_resume ||
				    intf->needs_remote_wakeup)
					return -EOPNOTSUPP;
			}
		}
	}

	j = jiffies;
	if (time_before(j, suspend_time))
		reschedule = 1;
	else
		suspend_time = j + HZ;
	if (reschedule) {
		if (!timer_pending(&udev->autosuspend.timer)) {
			queue_delayed_work(ksuspend_usb_wq, &udev->autosuspend,
				round_jiffies_up_relative(suspend_time - j));
		}
		return -EAGAIN;
	}
	return 0;
}

#else

static inline int autosuspend_check(struct usb_device *udev, int reschedule)
{
	return 0;
}

#endif	 

static int usb_suspend_both(struct usb_device *udev, pm_message_t msg)
{
	int			status = 0;
	int			i = 0;
	struct usb_interface	*intf;
	struct usb_device	*parent = udev->parent;

	if (udev->state == USB_STATE_NOTATTACHED ||
			udev->state == USB_STATE_SUSPENDED)
		goto done;

	if (msg.event & PM_EVENT_AUTO) {
		udev->do_remote_wakeup = device_may_wakeup(&udev->dev);
		status = autosuspend_check(udev, 0);
		if (status < 0)
			goto done;
	}

	if (udev->actconfig) {
		for (; i < udev->actconfig->desc.bNumInterfaces; i++) {
			intf = udev->actconfig->interface[i];
			status = usb_suspend_interface(udev, intf, msg);

			if (!(msg.event & PM_EVENT_AUTO))
				status = 0;
			if (status != 0)
				break;
		}
	}
	if (status == 0) {
		status = usb_suspend_device(udev, msg);

		if (!(msg.event & PM_EVENT_AUTO))
			status = 0;
	}

	if (status != 0) {
		pm_message_t msg2;

		msg2.event = msg.event ^ (PM_EVENT_SUSPEND | PM_EVENT_RESUME);
		while (--i >= 0) {
			intf = udev->actconfig->interface[i];
			usb_resume_interface(udev, intf, msg2, 0);
		}

		if (msg.event & PM_EVENT_AUTO)
			autosuspend_check(udev, status == -EBUSY);

	} else {
		cancel_delayed_work(&udev->autosuspend);
		udev->can_submit = 0;
		for (i = 0; i < 16; ++i) {
			usb_hcd_flush_endpoint(udev, udev->ep_out[i]);
			usb_hcd_flush_endpoint(udev, udev->ep_in[i]);
		}

		if (parent && udev->state == USB_STATE_SUSPENDED)
			usb_autosuspend_device(parent);
	}

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	return status;
}

static int usb_resume_both(struct usb_device *udev, pm_message_t msg)
{
	int			status = 0;
	int			i;
	struct usb_interface	*intf;
	struct usb_device	*parent = udev->parent;

	cancel_delayed_work(&udev->autosuspend);
	if (udev->state == USB_STATE_NOTATTACHED) {
		status = -ENODEV;
		goto done;
	}
	udev->can_submit = 1;

	if (udev->state == USB_STATE_SUSPENDED) {
		if (parent) {
			status = usb_autoresume_device(parent);
			if (status == 0) {
				status = usb_resume_device(udev, msg);
				if (status || udev->state ==
						USB_STATE_NOTATTACHED) {
					usb_autosuspend_device(parent);

					if (udev->state ==
							USB_STATE_NOTATTACHED)
						udev->discon_suspended = 1;
				}
			}
		} else {

			status = usb_resume_device(udev, msg);
		}
	} else if (udev->reset_resume)
		status = usb_resume_device(udev, msg);

	if (status == 0 && udev->actconfig) {
		for (i = 0; i < udev->actconfig->desc.bNumInterfaces; i++) {
			intf = udev->actconfig->interface[i];
			usb_resume_interface(udev, intf, msg,
					udev->reset_resume);
		}
	}

 done:
	dev_vdbg(&udev->dev, "%s: status %d\n", __func__, status);
	if (!status)
		udev->reset_resume = 0;
	return status;
}

#ifdef CONFIG_USB_SUSPEND

static int usb_autopm_do_device(struct usb_device *udev, int inc_usage_cnt)
{
	int	status = 0;

	usb_pm_lock(udev);
	udev->auto_pm = 1;
	udev->pm_usage_cnt += inc_usage_cnt;
	WARN_ON(udev->pm_usage_cnt < 0);
	if (inc_usage_cnt)
		udev->last_busy = jiffies;
	if (inc_usage_cnt >= 0 && udev->pm_usage_cnt > 0) {
		if (udev->state == USB_STATE_SUSPENDED)
			status = usb_resume_both(udev, PMSG_AUTO_RESUME);
		if (status != 0)
			udev->pm_usage_cnt -= inc_usage_cnt;
		else if (inc_usage_cnt)
			udev->last_busy = jiffies;
	} else if (inc_usage_cnt <= 0 && udev->pm_usage_cnt <= 0) {
		status = usb_suspend_both(udev, PMSG_AUTO_SUSPEND);
	}
	usb_pm_unlock(udev);
	return status;
}

void usb_autosuspend_work(struct work_struct *work)
{
	struct usb_device *udev =
		container_of(work, struct usb_device, autosuspend.work);

	usb_autopm_do_device(udev, 0);
}

void usb_autoresume_work(struct work_struct *work)
{
	struct usb_device *udev =
		container_of(work, struct usb_device, autoresume);

	if (usb_autopm_do_device(udev, 1) == 0)
		usb_autopm_do_device(udev, -1);
}

void usb_autosuspend_device(struct usb_device *udev)
{
	int	status;

	status = usb_autopm_do_device(udev, -1);
	dev_vdbg(&udev->dev, "%s: cnt %d\n",
			__func__, udev->pm_usage_cnt);
}

void usb_try_autosuspend_device(struct usb_device *udev)
{
	usb_autopm_do_device(udev, 0);
	dev_vdbg(&udev->dev, "%s: cnt %d\n",
			__func__, udev->pm_usage_cnt);
}

int usb_autoresume_device(struct usb_device *udev)
{
	int	status;

	status = usb_autopm_do_device(udev, 1);
	dev_vdbg(&udev->dev, "%s: status %d cnt %d\n",
			__func__, status, udev->pm_usage_cnt);
	return status;
}

static int usb_autopm_do_interface(struct usb_interface *intf,
		int inc_usage_cnt)
{
	struct usb_device	*udev = interface_to_usbdev(intf);
	int			status = 0;

	usb_pm_lock(udev);
	if (intf->condition == USB_INTERFACE_UNBOUND)
		status = -ENODEV;
	else {
		udev->auto_pm = 1;
		atomic_add(inc_usage_cnt, &intf->pm_usage_cnt);
		udev->last_busy = jiffies;
		if (inc_usage_cnt >= 0 &&
				atomic_read(&intf->pm_usage_cnt) > 0) {
			if (udev->state == USB_STATE_SUSPENDED)
				status = usb_resume_both(udev,
						PMSG_AUTO_RESUME);
			if (status != 0)
				atomic_sub(inc_usage_cnt, &intf->pm_usage_cnt);
			else
				udev->last_busy = jiffies;
		} else if (inc_usage_cnt <= 0 &&
				atomic_read(&intf->pm_usage_cnt) <= 0) {
			status = usb_suspend_both(udev, PMSG_AUTO_SUSPEND);
		}
	}
	usb_pm_unlock(udev);
	return status;
}

void usb_autopm_put_interface(struct usb_interface *intf)
{
	int	status;

	status = usb_autopm_do_interface(intf, -1);
	dev_vdbg(&intf->dev, "%s: status %d cnt %d\n",
			__func__, status, atomic_read(&intf->pm_usage_cnt));
}
EXPORT_SYMBOL_GPL(usb_autopm_put_interface);

void usb_autopm_put_interface_async(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);
	int			status = 0;

	if (intf->condition == USB_INTERFACE_UNBOUND) {
		status = -ENODEV;
	} else {
		udev->last_busy = jiffies;
		atomic_dec(&intf->pm_usage_cnt);
		if (udev->autosuspend_disabled || udev->autosuspend_delay < 0)
			status = -EPERM;
		else if (atomic_read(&intf->pm_usage_cnt) <= 0 &&
				!timer_pending(&udev->autosuspend.timer)) {
			queue_delayed_work(ksuspend_usb_wq, &udev->autosuspend,
					round_jiffies_up_relative(
						udev->autosuspend_delay));
		}
	}
	dev_vdbg(&intf->dev, "%s: status %d cnt %d\n",
			__func__, status, atomic_read(&intf->pm_usage_cnt));
}
EXPORT_SYMBOL_GPL(usb_autopm_put_interface_async);

int usb_autopm_get_interface(struct usb_interface *intf)
{
	int	status;

	status = usb_autopm_do_interface(intf, 1);
	dev_vdbg(&intf->dev, "%s: status %d cnt %d\n",
			__func__, status, atomic_read(&intf->pm_usage_cnt));
	return status;
}
EXPORT_SYMBOL_GPL(usb_autopm_get_interface);

int usb_autopm_get_interface_async(struct usb_interface *intf)
{
	struct usb_device	*udev = interface_to_usbdev(intf);
	int			status = 0;

	if (intf->condition == USB_INTERFACE_UNBOUND)
		status = -ENODEV;
	else {
		atomic_inc(&intf->pm_usage_cnt);
		if (atomic_read(&intf->pm_usage_cnt) > 0 &&
				udev->state == USB_STATE_SUSPENDED)
			queue_work(ksuspend_usb_wq, &udev->autoresume);
	}
	dev_vdbg(&intf->dev, "%s: status %d cnt %d\n",
			__func__, status, atomic_read(&intf->pm_usage_cnt));
	return status;
}
EXPORT_SYMBOL_GPL(usb_autopm_get_interface_async);

#else

void usb_autosuspend_work(struct work_struct *work)
{}

void usb_autoresume_work(struct work_struct *work)
{}

#endif  

int usb_external_suspend_device(struct usb_device *udev, pm_message_t msg)
{
	int	status;

	do_unbind_rebind(udev, DO_UNBIND);
	usb_pm_lock(udev);
	udev->auto_pm = 0;
	status = usb_suspend_both(udev, msg);
	usb_pm_unlock(udev);
	return status;
}

int usb_external_resume_device(struct usb_device *udev, pm_message_t msg)
{
	int	status;

	usb_pm_lock(udev);
	udev->auto_pm = 0;
	status = usb_resume_both(udev, msg);
	udev->last_busy = jiffies;
	usb_pm_unlock(udev);
	if (status == 0)
		do_unbind_rebind(udev, DO_REBIND);

	if (status == 0)
		usb_try_autosuspend_device(udev);
	return status;
}

static void choose_wakeup(struct usb_device *udev, pm_message_t msg)
{
	 
	if (msg.event == PM_EVENT_FREEZE || msg.event == PM_EVENT_QUIESCE) {
		udev->do_remote_wakeup = 0;
		return;
	}

	udev->do_remote_wakeup = device_may_wakeup(&udev->dev);
}

int usb_suspend(struct device *dev, pm_message_t msg)
{
	struct usb_device	*udev;

	udev = to_usb_device(dev);

	if (udev->state == USB_STATE_SUSPENDED) {
		if (udev->parent || udev->speed != USB_SPEED_HIGH)
			udev->skip_sys_resume = 1;
		return 0;
	}

	udev->skip_sys_resume = 0;
	choose_wakeup(udev, msg);
	return usb_external_suspend_device(udev, msg);
}

int usb_resume(struct device *dev, pm_message_t msg)
{
	struct usb_device	*udev;
	int			status;

	udev = to_usb_device(dev);

	if (udev->skip_sys_resume)
		return 0;
	status = usb_external_resume_device(udev, msg);

	if (status == -ENODEV)
		return 0;
	return status;
}

#endif  

struct bus_type usb_bus_type = {
	.name =		"usb",
	.match =	usb_device_match,
	.uevent =	usb_uevent,
};

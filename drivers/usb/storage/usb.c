#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/freezer.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/mutex.h>
#include <linux/utsname.h>

#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>

#include "usb.h"
#include "scsiglue.h"
#include "transport.h"
#include "protocol.h"
#include "debug.h"
#include "initializers.h"

#include "sierra_ms.h"
#include "option_ms.h"

MODULE_AUTHOR("Matthew Dharm <mdharm-usb@one-eyed-alien.net>");
MODULE_DESCRIPTION("USB Mass Storage driver for Linux");
MODULE_LICENSE("GPL");

static unsigned int delay_use = 5;
module_param(delay_use, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(delay_use, "seconds to delay before using a new device");

static char quirks[128];
module_param_string(quirks, quirks, sizeof(quirks), S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(quirks, "supplemental list of device IDs and their quirks");

#if defined(CONFIG_USB_UAS) || defined(CONFIG_USB_UAS_MODULE)
static unsigned int uas_check = 1;
module_param(uas_check, uint, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(uas_check, "check whether new device supports UAS protocol");
#endif

#define UNUSUAL_DEV(idVendor, idProduct, bcdDeviceMin, bcdDeviceMax, \
		    vendor_name, product_name, use_protocol, use_transport, \
		    init_function, Flags) \
{ \
	.vendorName = vendor_name,	\
	.productName = product_name,	\
	.useProtocol = use_protocol,	\
	.useTransport = use_transport,	\
	.initFunction = init_function,	\
}

#define COMPLIANT_DEV	UNUSUAL_DEV

#define USUAL_DEV(use_protocol, use_transport, use_type) \
{ \
	.useProtocol = use_protocol,	\
	.useTransport = use_transport,	\
}

#define UNUSUAL_VENDOR_INTF(idVendor, cl, sc, pr, \
		vendor_name, product_name, use_protocol, use_transport, \
		init_function, Flags) \
{ \
	.vendorName = vendor_name,	\
	.productName = product_name,	\
	.useProtocol = use_protocol,	\
	.useTransport = use_transport,	\
	.initFunction = init_function,	\
}

static struct us_unusual_dev us_unusual_dev_list[] = {
#	include "unusual_devs.h" 
	{ }		 
};

#undef UNUSUAL_DEV
#undef COMPLIANT_DEV
#undef USUAL_DEV
#undef UNUSUAL_VENDOR_INTF

static unsigned int usb_stor_sg_tablesize(struct usb_interface *intf)
{
	struct usb_device *usb_dev = interface_to_usbdev(intf);

	if (usb_dev->bus->sg_tablesize) {
		return usb_dev->bus->sg_tablesize;
	}
	return SG_ALL;
}

#ifdef CONFIG_PM	 

int usb_stor_suspend(struct usb_interface *iface, pm_message_t message)
{
	struct us_data *us = usb_get_intfdata(iface);

	mutex_lock(&us->dev_mutex);

	US_DEBUGP("%s\n", __func__);
	if (us->suspend_resume_hook)
		(us->suspend_resume_hook)(us, US_SUSPEND);

	mutex_unlock(&us->dev_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(usb_stor_suspend);

int usb_stor_resume(struct usb_interface *iface)
{
	struct us_data *us = usb_get_intfdata(iface);

	mutex_lock(&us->dev_mutex);

	US_DEBUGP("%s\n", __func__);
	if (us->suspend_resume_hook)
		(us->suspend_resume_hook)(us, US_RESUME);

	mutex_unlock(&us->dev_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(usb_stor_resume);

int usb_stor_reset_resume(struct usb_interface *iface)
{
	struct us_data *us = usb_get_intfdata(iface);

	US_DEBUGP("%s\n", __func__);

	usb_stor_report_bus_reset(us);

	return 0;
}
EXPORT_SYMBOL_GPL(usb_stor_reset_resume);

#endif  

int usb_stor_pre_reset(struct usb_interface *iface)
{
	struct us_data *us = usb_get_intfdata(iface);

	US_DEBUGP("%s\n", __func__);

	mutex_lock(&us->dev_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(usb_stor_pre_reset);

int usb_stor_post_reset(struct usb_interface *iface)
{
	struct us_data *us = usb_get_intfdata(iface);

	US_DEBUGP("%s\n", __func__);

	usb_stor_report_bus_reset(us);

	mutex_unlock(&us->dev_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(usb_stor_post_reset);

void fill_inquiry_response(struct us_data *us, unsigned char *data,
		unsigned int data_len)
{
	if (data_len<36)  
		return;

	memset(data+8, ' ', 28);
	if(data[0]&0x20) {  
	} else {
		u16 bcdDevice = le16_to_cpu(us->pusb_dev->descriptor.bcdDevice);
		int n;

		n = strlen(us->unusual_dev->vendorName);
		memcpy(data+8, us->unusual_dev->vendorName, min(8, n));
		n = strlen(us->unusual_dev->productName);
		memcpy(data+16, us->unusual_dev->productName, min(16, n));

		data[32] = 0x30 + ((bcdDevice>>12) & 0x0F);
		data[33] = 0x30 + ((bcdDevice>>8) & 0x0F);
		data[34] = 0x30 + ((bcdDevice>>4) & 0x0F);
		data[35] = 0x30 + ((bcdDevice) & 0x0F);
	}

	usb_stor_set_xfer_buf(data, data_len, us->srb);
}
EXPORT_SYMBOL_GPL(fill_inquiry_response);

static int usb_stor_no_test_unit_ready(struct us_data *us)
{
	struct usb_device *udev = us->pusb_dev;

	if (us->srb->cmnd[0] != TEST_UNIT_READY)
		return -EINVAL;

	if (udev->descriptor.idVendor != cpu_to_le16(0x1759) ||
		udev->descriptor.idProduct != cpu_to_le16(0x5002))
		return -EINVAL;

	return 0;
}

static int usb_stor_control_thread(void * __us)
{
	struct us_data *us = (struct us_data *)__us;
	struct Scsi_Host *host = us_to_host(us);

	for(;;) {
		US_DEBUGP("*** thread sleeping.\n");
		if (wait_for_completion_interruptible(&us->cmnd_ready))
			break;

		US_DEBUGP("*** thread awakened.\n");

		mutex_lock(&(us->dev_mutex));

		scsi_lock(host);

		if (us->srb == NULL) {
			scsi_unlock(host);
			mutex_unlock(&us->dev_mutex);
			US_DEBUGP("-- exiting\n");
			break;
		}

		if (test_bit(US_FLIDX_TIMED_OUT, &us->dflags)) {
			us->srb->result = DID_ABORT << 16;
			goto SkipForAbort;
		}

		scsi_unlock(host);

		if (us->srb->sc_data_direction == DMA_BIDIRECTIONAL) {
			US_DEBUGP("UNKNOWN data direction\n");
			us->srb->result = DID_ERROR << 16;
		}

		else if (us->srb->device->id && 
				!(us->fflags & US_FL_SCM_MULT_TARG)) {
			US_DEBUGP("Bad target number (%d:%d)\n",
				  us->srb->device->id, us->srb->device->lun);
			us->srb->result = DID_BAD_TARGET << 16;
		}

		else if (us->srb->device->lun > us->max_lun) {
			US_DEBUGP("Bad LUN (%d:%d)\n",
				  us->srb->device->id, us->srb->device->lun);
			us->srb->result = DID_BAD_TARGET << 16;
		}

		else if ((us->srb->cmnd[0] == INQUIRY) &&
			    (us->fflags & US_FL_FIX_INQUIRY)) {
			unsigned char data_ptr[36] = {
			    0x00, 0x80, 0x02, 0x02,
			    0x1F, 0x00, 0x00, 0x00};

			US_DEBUGP("Faking INQUIRY command\n");
			fill_inquiry_response(us, data_ptr, 36);
			us->srb->result = SAM_STAT_GOOD;
		}

		else if (!usb_stor_no_test_unit_ready(us)) {
			US_DEBUGP("Ignoring TEST_UNIT_READY command\n");
			us->srb->result = SAM_STAT_GOOD;
		}

		else {
			US_DEBUG(usb_stor_show_command(us->srb));
			us->proto_handler(us->srb, us);
		}

		scsi_lock(host);

		if (us->srb->result != DID_ABORT << 16) {
			US_DEBUGP("scsi cmd done, result=0x%x\n", 
				   us->srb->result);
			us->srb->scsi_done(us->srb);
		} else {
SkipForAbort:
			US_DEBUGP("scsi command aborted\n");
		}

		if (test_bit(US_FLIDX_TIMED_OUT, &us->dflags)) {
			complete(&(us->notify));

			clear_bit(US_FLIDX_ABORTING, &us->dflags);
			clear_bit(US_FLIDX_TIMED_OUT, &us->dflags);
		}

		us->srb = NULL;
		scsi_unlock(host);

		mutex_unlock(&us->dev_mutex);
	}  

	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}	

static int associate_dev(struct us_data *us, struct usb_interface *intf)
{
	US_DEBUGP("-- %s\n", __func__);

	us->pusb_dev = interface_to_usbdev(intf);
	us->pusb_intf = intf;
	us->ifnum = intf->cur_altsetting->desc.bInterfaceNumber;
	US_DEBUGP("Vendor: 0x%04x, Product: 0x%04x, Revision: 0x%04x\n",
			le16_to_cpu(us->pusb_dev->descriptor.idVendor),
			le16_to_cpu(us->pusb_dev->descriptor.idProduct),
			le16_to_cpu(us->pusb_dev->descriptor.bcdDevice));
	US_DEBUGP("Interface Subclass: 0x%02x, Protocol: 0x%02x\n",
			intf->cur_altsetting->desc.bInterfaceSubClass,
			intf->cur_altsetting->desc.bInterfaceProtocol);

	usb_set_intfdata(intf, us);

	us->cr = usb_buffer_alloc(us->pusb_dev, sizeof(*us->cr),
			GFP_KERNEL, &us->cr_dma);
	if (!us->cr) {
		US_DEBUGP("usb_ctrlrequest allocation failed\n");
		return -ENOMEM;
	}

	us->iobuf = usb_buffer_alloc(us->pusb_dev, US_IOBUF_SIZE,
			GFP_KERNEL, &us->iobuf_dma);
	if (!us->iobuf) {
		US_DEBUGP("I/O buffer allocation failed\n");
		return -ENOMEM;
	}
	return 0;
}

#define TOLOWER(x) ((x) | 0x20)

static void adjust_quirks(struct us_data *us)
{
	char *p;
	u16 vid = le16_to_cpu(us->pusb_dev->descriptor.idVendor);
	u16 pid = le16_to_cpu(us->pusb_dev->descriptor.idProduct);
	unsigned f = 0;
	unsigned int mask = (US_FL_SANE_SENSE | US_FL_BAD_SENSE |
			US_FL_FIX_CAPACITY |
			US_FL_CAPACITY_HEURISTICS | US_FL_IGNORE_DEVICE |
			US_FL_NOT_LOCKABLE | US_FL_MAX_SECTORS_64 |
			US_FL_CAPACITY_OK | US_FL_IGNORE_RESIDUE |
			US_FL_SINGLE_LUN | US_FL_NO_WP_DETECT);

	p = quirks;
	while (*p) {
		 
		if (vid == simple_strtoul(p, &p, 16) &&
				*p == ':' &&
				pid == simple_strtoul(p+1, &p, 16) &&
				*p == ':')
			break;

		while (*p) {
			if (*p++ == ',')
				break;
		}
	}
	if (!*p)	 
		return;

	while (*++p && *p != ',') {
		switch (TOLOWER(*p)) {
		case 'a':
			f |= US_FL_SANE_SENSE;
			break;
		case 'b':
			f |= US_FL_BAD_SENSE;
			break;
		case 'c':
			f |= US_FL_FIX_CAPACITY;
			break;
		case 'h':
			f |= US_FL_CAPACITY_HEURISTICS;
			break;
		case 'i':
			f |= US_FL_IGNORE_DEVICE;
			break;
		case 'l':
			f |= US_FL_NOT_LOCKABLE;
			break;
		case 'm':
			f |= US_FL_MAX_SECTORS_64;
			break;
		case 'o':
			f |= US_FL_CAPACITY_OK;
			break;
		case 'r':
			f |= US_FL_IGNORE_RESIDUE;
			break;
		case 's':
			f |= US_FL_SINGLE_LUN;
			break;
		case 'w':
			f |= US_FL_NO_WP_DETECT;
			break;
		 
		}
	}
	us->fflags = (us->fflags & ~mask) | f;
	dev_info(&us->pusb_intf->dev, "Quirks match for "
			"vid %04x pid %04x: %x\n",
			vid, pid, f);
}

static int get_device_info(struct us_data *us, const struct usb_device_id *id,
		struct us_unusual_dev *unusual_dev)
{
	struct usb_device *dev = us->pusb_dev;
	struct usb_interface_descriptor *idesc =
		&us->pusb_intf->cur_altsetting->desc;

	us->unusual_dev = unusual_dev;
	us->subclass = (unusual_dev->useProtocol == USB_SC_DEVICE) ?
			idesc->bInterfaceSubClass :
			unusual_dev->useProtocol;
	us->protocol = (unusual_dev->useTransport == USB_PR_DEVICE) ?
			idesc->bInterfaceProtocol :
			unusual_dev->useTransport;
	us->fflags = USB_US_ORIG_FLAGS(id->driver_info);
	adjust_quirks(us);

	if (us->fflags & US_FL_IGNORE_DEVICE) {
		printk(KERN_INFO USB_STORAGE "device ignored\n");
		return -ENODEV;
	}

	if (dev->speed != USB_SPEED_HIGH)
		us->fflags &= ~US_FL_GO_SLOW;

	if (id->idVendor || id->idProduct) {
		static const char *msgs[3] = {
			"an unneeded SubClass entry",
			"an unneeded Protocol entry",
			"unneeded SubClass and Protocol entries"};
		struct usb_device_descriptor *ddesc = &dev->descriptor;
		int msg = -1;

		if (unusual_dev->useProtocol != USB_SC_DEVICE &&
			us->subclass == idesc->bInterfaceSubClass)
			msg += 1;
		if (unusual_dev->useTransport != USB_PR_DEVICE &&
			us->protocol == idesc->bInterfaceProtocol)
			msg += 2;
		if (msg >= 0 && !(us->fflags & US_FL_NEED_OVERRIDE))
			printk(KERN_NOTICE USB_STORAGE "This device "
				"(%04x,%04x,%04x S %02x P %02x)"
				" has %s in unusual_devs.h (kernel"
				" %s)\n"
				"   Please send a copy of this message to "
				"<linux-usb@vger.kernel.org> and "
				"<usb-storage@lists.one-eyed-alien.net>\n",
				le16_to_cpu(ddesc->idVendor),
				le16_to_cpu(ddesc->idProduct),
				le16_to_cpu(ddesc->bcdDevice),
				idesc->bInterfaceSubClass,
				idesc->bInterfaceProtocol,
				msgs[msg],
				utsname()->release);
	}

	return 0;
}

static void get_transport(struct us_data *us)
{
	switch (us->protocol) {
	case USB_PR_CB:
		us->transport_name = "Control/Bulk";
		us->transport = usb_stor_CB_transport;
		us->transport_reset = usb_stor_CB_reset;
		us->max_lun = 7;
		break;

	case USB_PR_CBI:
		us->transport_name = "Control/Bulk/Interrupt";
		us->transport = usb_stor_CB_transport;
		us->transport_reset = usb_stor_CB_reset;
		us->max_lun = 7;
		break;

	case USB_PR_BULK:
		us->transport_name = "Bulk";
		us->transport = usb_stor_Bulk_transport;
		us->transport_reset = usb_stor_Bulk_reset;
		break;
	}
}

static void get_protocol(struct us_data *us)
{
	switch (us->subclass) {
	case USB_SC_RBC:
		us->protocol_name = "Reduced Block Commands (RBC)";
		us->proto_handler = usb_stor_transparent_scsi_command;
		break;

	case USB_SC_8020:
		us->protocol_name = "8020i";
		us->proto_handler = usb_stor_pad12_command;
		us->max_lun = 0;
		break;

	case USB_SC_QIC:
		us->protocol_name = "QIC-157";
		us->proto_handler = usb_stor_pad12_command;
		us->max_lun = 0;
		break;

	case USB_SC_8070:
		us->protocol_name = "8070i";
		us->proto_handler = usb_stor_pad12_command;
		us->max_lun = 0;
		break;

	case USB_SC_SCSI:
		us->protocol_name = "Transparent SCSI";
		us->proto_handler = usb_stor_transparent_scsi_command;
		break;

	case USB_SC_UFI:
		us->protocol_name = "Uniform Floppy Interface (UFI)";
		us->proto_handler = usb_stor_ufi_command;
		break;
	}
}

static int get_pipes(struct us_data *us)
{
	struct usb_host_interface *altsetting =
		us->pusb_intf->cur_altsetting;
	int i;
	struct usb_endpoint_descriptor *ep;
	struct usb_endpoint_descriptor *ep_in = NULL;
	struct usb_endpoint_descriptor *ep_out = NULL;
	struct usb_endpoint_descriptor *ep_int = NULL;

	for (i = 0; i < altsetting->desc.bNumEndpoints; i++) {
		ep = &altsetting->endpoint[i].desc;

		if (usb_endpoint_xfer_bulk(ep)) {
			if (usb_endpoint_dir_in(ep)) {
				if (!ep_in)
					ep_in = ep;
			} else {
				if (!ep_out)
					ep_out = ep;
			}
		}

		else if (usb_endpoint_is_int_in(ep)) {
			if (!ep_int)
				ep_int = ep;
		}
	}

	if (!ep_in || !ep_out || (us->protocol == USB_PR_CBI && !ep_int)) {
		US_DEBUGP("Endpoint sanity check failed! Rejecting dev.\n");
		return -EIO;
	}

	us->send_ctrl_pipe = usb_sndctrlpipe(us->pusb_dev, 0);
	us->recv_ctrl_pipe = usb_rcvctrlpipe(us->pusb_dev, 0);
	us->send_bulk_pipe = usb_sndbulkpipe(us->pusb_dev,
		usb_endpoint_num(ep_out));
	us->recv_bulk_pipe = usb_rcvbulkpipe(us->pusb_dev, 
		usb_endpoint_num(ep_in));
	if (ep_int) {
		us->recv_intr_pipe = usb_rcvintpipe(us->pusb_dev,
			usb_endpoint_num(ep_int));
		us->ep_bInterval = ep_int->bInterval;
	}
	return 0;
}

static int usb_stor_acquire_resources(struct us_data *us)
{
	int p;
	struct task_struct *th;

	us->current_urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!us->current_urb) {
		US_DEBUGP("URB allocation failed\n");
		return -ENOMEM;
	}

	if (us->unusual_dev->initFunction) {
		p = us->unusual_dev->initFunction(us);
		if (p)
			return p;
	}

	th = kthread_run(usb_stor_control_thread, us, "usb-storage");
	if (IS_ERR(th)) {
		printk(KERN_WARNING USB_STORAGE 
		       "Unable to start control thread\n");
		return PTR_ERR(th);
	}
	us->ctl_thread = th;

	return 0;
}

static void usb_stor_release_resources(struct us_data *us)
{
	US_DEBUGP("-- %s\n", __func__);

	US_DEBUGP("-- sending exit command to thread\n");
	complete(&us->cmnd_ready);
	if (us->ctl_thread)
		kthread_stop(us->ctl_thread);

	if (us->extra_destructor) {
		US_DEBUGP("-- calling extra_destructor()\n");
		us->extra_destructor(us->extra);
	}

	kfree(us->extra);
	usb_free_urb(us->current_urb);
}

static void dissociate_dev(struct us_data *us)
{
	US_DEBUGP("-- %s\n", __func__);

	if (us->cr)
		usb_buffer_free(us->pusb_dev, sizeof(*us->cr), us->cr,
				us->cr_dma);
	if (us->iobuf)
		usb_buffer_free(us->pusb_dev, US_IOBUF_SIZE, us->iobuf,
				us->iobuf_dma);

	usb_set_intfdata(us->pusb_intf, NULL);
}

static void quiesce_and_remove_host(struct us_data *us)
{
	struct Scsi_Host *host = us_to_host(us);

	if (us->pusb_dev->state == USB_STATE_NOTATTACHED)
		set_bit(US_FLIDX_DISCONNECTING, &us->dflags);

	set_bit(US_FLIDX_DONT_SCAN, &us->dflags);
	wake_up(&us->delay_wait);
	wait_for_completion(&us->scanning_done);

	scsi_remove_host(host);

	scsi_lock(host);
	set_bit(US_FLIDX_DISCONNECTING, &us->dflags);
	scsi_unlock(host);
	wake_up(&us->delay_wait);
}

static void release_everything(struct us_data *us)
{
	usb_stor_release_resources(us);
	dissociate_dev(us);

	scsi_host_put(us_to_host(us));
}

static int usb_stor_scan_thread(void * __us)
{
	struct us_data *us = (struct us_data *)__us;

	printk(KERN_DEBUG
		"usb-storage: device found at %d\n", us->pusb_dev->devnum);

	set_freezable();
	 
	if (delay_use > 0) {
		printk(KERN_DEBUG "usb-storage: waiting for device "
				"to settle before scanning\n");
		wait_event_freezable_timeout(us->delay_wait,
				test_bit(US_FLIDX_DONT_SCAN, &us->dflags),
				delay_use * HZ);
	}

	if (!test_bit(US_FLIDX_DONT_SCAN, &us->dflags)) {

		if (us->protocol == USB_PR_BULK &&
				!(us->fflags & US_FL_SINGLE_LUN)) {
			mutex_lock(&us->dev_mutex);
			us->max_lun = usb_stor_Bulk_max_lun(us);
			mutex_unlock(&us->dev_mutex);
		}
		scsi_scan_host(us_to_host(us));
		printk(KERN_DEBUG "usb-storage: device scan complete\n");

	}

	complete_and_exit(&us->scanning_done, 0);
}

#ifdef MY_ABC_HERE
extern int (*SynoUSBDeviceIsSATA)(void *);

int SynoIsSATADisk(void *hostdata)
{
       return ((struct us_data *)hostdata)->is_ata_disk;
}
#endif

int usb_stor_probe1(struct us_data **pus,
		struct usb_interface *intf,
		const struct usb_device_id *id,
		struct us_unusual_dev *unusual_dev)
{
	struct Scsi_Host *host;
	struct us_data *us;
	int result;

	US_DEBUGP("USB Mass Storage device detected\n");

	host = scsi_host_alloc(&usb_stor_host_template, sizeof(*us));
	if (!host) {
		printk(KERN_WARNING USB_STORAGE
			"Unable to allocate the scsi host\n");
		return -ENOMEM;
	}

	host->max_cmd_len = 16;
	host->sg_tablesize = usb_stor_sg_tablesize(intf);
	*pus = us = host_to_us(host);
	memset(us, 0, sizeof(struct us_data));
	mutex_init(&(us->dev_mutex));
	init_completion(&us->cmnd_ready);
	init_completion(&(us->notify));
	init_waitqueue_head(&us->delay_wait);
	init_completion(&us->scanning_done);

	result = associate_dev(us, intf);
	if (result)
		goto BadDevice;

	result = get_device_info(us, id, unusual_dev);
	if (result)
		goto BadDevice;

	get_transport(us);
	get_protocol(us);

	return 0;

BadDevice:
	US_DEBUGP("storage_probe() failed\n");
	release_everything(us);
	return result;
}
EXPORT_SYMBOL_GPL(usb_stor_probe1);

int usb_stor_probe2(struct us_data *us)
{
	struct task_struct *th;
	int result;

	if (!us->transport || !us->proto_handler) {
		result = -ENXIO;
		goto BadDevice;
	}
	US_DEBUGP("Transport: %s\n", us->transport_name);
	US_DEBUGP("Protocol: %s\n", us->protocol_name);

	if (us->fflags & US_FL_SINGLE_LUN)
		us->max_lun = 0;

	result = get_pipes(us);
	if (result)
		goto BadDevice;

	result = usb_stor_acquire_resources(us);
	if (result)
		goto BadDevice;
	result = scsi_add_host(us_to_host(us), &us->pusb_intf->dev);
	if (result) {
		printk(KERN_WARNING USB_STORAGE
			"Unable to add the scsi host\n");
		goto BadDevice;
	}

#ifdef MY_ABC_HERE
	 
	us->is_ata_disk = 0;
	if(us->pusb_dev &&
		us->pusb_dev->product &&
		NULL != strstr(us->pusb_dev->product, "Serial-ATA")) {
		us->is_ata_disk = 1;
	}
#endif
	 
	th = kthread_create(usb_stor_scan_thread, us, "usb-stor-scan");
	if (IS_ERR(th)) {
		printk(KERN_WARNING USB_STORAGE 
		       "Unable to start the device-scanning thread\n");
		complete(&us->scanning_done);
		quiesce_and_remove_host(us);
		result = PTR_ERR(th);
		goto BadDevice;
	}

	wake_up_process(th);

	return 0;

BadDevice:
	US_DEBUGP("storage_probe() failed\n");
	release_everything(us);
	return result;
}
EXPORT_SYMBOL_GPL(usb_stor_probe2);

void usb_stor_disconnect(struct usb_interface *intf)
{
	struct us_data *us = usb_get_intfdata(intf);

	US_DEBUGP("storage_disconnect() called\n");
	quiesce_and_remove_host(us);
	release_everything(us);
}
EXPORT_SYMBOL_GPL(usb_stor_disconnect);

#if defined(CONFIG_USB_UAS) || defined(CONFIG_USB_UAS_MODULE)
static int is_uas_device(struct usb_interface *intf)
{
	int i;

	for (i = 0; i < intf->num_altsetting; i++) {
		struct usb_host_interface *alt = &intf->altsetting[i];

		if (alt->desc.bInterfaceClass == USB_CLASS_MASS_STORAGE &&
			alt->desc.bInterfaceSubClass == USB_SC_SCSI &&
			alt->desc.bInterfaceProtocol == USB_PR_UAS)
			return 0;
	}

	return -ENODEV;
}
#endif

static int storage_probe(struct usb_interface *intf,
			 const struct usb_device_id *id)
{
	struct us_data *us;
	int result;

#if defined(CONFIG_USB_UAS) || defined(CONFIG_USB_UAS_MODULE)
	if (uas_check && !is_uas_device(intf))
		return -ENODEV;
#endif

	if (usb_usual_check_type(id, USB_US_TYPE_STOR) ||
			usb_usual_ignore_device(intf))
		return -ENXIO;

	result = usb_stor_probe1(&us, intf, id,
			(id - usb_storage_usb_ids) + us_unusual_dev_list);
	if (result)
		return result;

	result = usb_stor_probe2(us);
	return result;
}

static struct usb_driver usb_storage_driver = {
	.name =		"usb-storage",
	.probe =	storage_probe,
	.disconnect =	usb_stor_disconnect,
	.suspend =	usb_stor_suspend,
	.resume =	usb_stor_resume,
	.reset_resume =	usb_stor_reset_resume,
	.pre_reset =	usb_stor_pre_reset,
	.post_reset =	usb_stor_post_reset,
	.id_table =	usb_storage_usb_ids,
	.soft_unbind =	1,
};

static int __init usb_stor_init(void)
{
	int retval;

	printk(KERN_INFO "Initializing USB Mass Storage driver...\n");

	retval = usb_register(&usb_storage_driver);
	if (retval == 0) {
		printk(KERN_INFO "USB Mass Storage support registered.\n");
		usb_usual_set_present(USB_US_TYPE_STOR);
	}

#ifdef MY_ABC_HERE
       SynoUSBDeviceIsSATA = SynoIsSATADisk;
#endif

	return retval;
}

static void __exit usb_stor_exit(void)
{
	US_DEBUGP("usb_stor_exit() called\n");

	US_DEBUGP("-- calling usb_deregister()\n");
	usb_deregister(&usb_storage_driver) ;

	usb_usual_clear_present(USB_US_TYPE_STOR);

#ifdef MY_ABC_HERE
       SynoUSBDeviceIsSATA = NULL;
#endif
}

module_init(usb_stor_init);
module_exit(usb_stor_exit);

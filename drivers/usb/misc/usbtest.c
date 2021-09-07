#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/scatterlist.h>
#include <linux/mutex.h>

#include <linux/usb.h>





struct usbtest_param {
	
	unsigned		test_num;	
	unsigned		iterations;
	unsigned		length;
	unsigned		vary;
	unsigned		sglen;

	
	struct timeval		duration;
};
#define USBTEST_REQUEST	_IOWR('U', 100, struct usbtest_param)



#define	GENERIC		







struct usbtest_info {
	const char		*name;
	u8			ep_in;		
	u8			ep_out;		
	unsigned		autoconf : 1;
	unsigned		ctrl_out : 1;
	unsigned		iso : 1;	
	int			alt;
};


struct usbtest_dev {
	struct usb_interface	*intf;
	struct usbtest_info	*info;
	int			in_pipe;
	int			out_pipe;
#ifdef CONFIG_MV_INCLUDE_USB
	struct usb_endpoint_descriptor	*in_desc, *out_desc;
#endif
	int			in_iso_pipe;
	int			out_iso_pipe;
	struct usb_endpoint_descriptor	*iso_in, *iso_out;
	struct mutex		lock;

#define TBUF_SIZE	256
	u8			*buf;
};

static struct usb_device *testdev_to_usbdev (struct usbtest_dev *test)
{
	return interface_to_usbdev (test->intf);
}


#ifdef CONFIG_MV_INCLUDE_USB
#define UNLINK_RATE     1   
#else
#define	INTERRUPT_RATE		1	
#endif

#define ERROR(tdev, fmt, args...) \
	dev_err(&(tdev)->intf->dev , fmt , ## args)
#define WARNING(tdev, fmt, args...) \
	dev_warn(&(tdev)->intf->dev , fmt , ## args)



static int
get_endpoints (struct usbtest_dev *dev, struct usb_interface *intf)
{
	int				tmp;
	struct usb_host_interface	*alt;
	struct usb_host_endpoint	*in, *out;
	struct usb_host_endpoint	*iso_in, *iso_out;
	struct usb_device		*udev;

	for (tmp = 0; tmp < intf->num_altsetting; tmp++) {
		unsigned	ep;

		in = out = NULL;
		iso_in = iso_out = NULL;
		alt = intf->altsetting + tmp;

		
		for (ep = 0; ep < alt->desc.bNumEndpoints; ep++) {
			struct usb_host_endpoint	*e;

			e = alt->endpoint + ep;
			switch (e->desc.bmAttributes) {
			case USB_ENDPOINT_XFER_BULK:
#ifdef CONFIG_MV_INCLUDE_USB
			case USB_ENDPOINT_XFER_INT:
#endif
				break;
			case USB_ENDPOINT_XFER_ISOC:
				if (dev->info->iso)
					goto try_iso;
				
			default:
				continue;
			}
			if (usb_endpoint_dir_in(&e->desc)) {
				if (!in)
					in = e;
			} else {
				if (!out)
					out = e;
			}
			continue;
try_iso:
			if (usb_endpoint_dir_in(&e->desc)) {
				if (!iso_in)
					iso_in = e;
			} else {
				if (!iso_out)
					iso_out = e;
			}
		}
		if ((in && out)  ||  (iso_in && iso_out))
			goto found;
	}
	return -EINVAL;

found:
	udev = testdev_to_usbdev (dev);
	if (alt->desc.bAlternateSetting != 0) {
		tmp = usb_set_interface (udev,
				alt->desc.bInterfaceNumber,
				alt->desc.bAlternateSetting);
		if (tmp < 0)
			return tmp;
	}

	if (in) {
#ifdef CONFIG_MV_INCLUDE_USB
		if(in->desc.bmAttributes == USB_ENDPOINT_XFER_INT) {
			dev->in_pipe = usb_rcvintpipe (udev,
			in->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
			if(out)
				dev->out_pipe = usb_sndintpipe (udev,
				out->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
		}
		else {
			dev->in_pipe = usb_rcvbulkpipe (udev,
				in->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
			if(out)
				dev->out_pipe = usb_sndbulkpipe (udev,
					out->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
		}
		dev->in_desc = &in->desc;
		dev->out_desc = &out->desc;
#else
		dev->in_pipe = usb_rcvbulkpipe (udev,
			in->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
		dev->out_pipe = usb_sndbulkpipe (udev,
			out->desc.bEndpointAddress & USB_ENDPOINT_NUMBER_MASK);
#endif
	}
	if (iso_in) {
		dev->iso_in = &iso_in->desc;
		dev->in_iso_pipe = usb_rcvisocpipe (udev,
				iso_in->desc.bEndpointAddress
					& USB_ENDPOINT_NUMBER_MASK);
		dev->iso_out = &iso_out->desc;
		dev->out_iso_pipe = usb_sndisocpipe (udev,
				iso_out->desc.bEndpointAddress
					& USB_ENDPOINT_NUMBER_MASK);
	}
	return 0;
}





static void simple_callback (struct urb *urb)
{
	complete(urb->context);
}

static struct urb *simple_alloc_urb (
	struct usb_device	*udev,
	int			pipe,
#ifdef CONFIG_MV_INCLUDE_USB
	struct usb_endpoint_descriptor *desc,
#endif
	unsigned long		bytes
)
{
	struct urb		*urb;

	urb = usb_alloc_urb (0, GFP_KERNEL);
	if (!urb)
		return urb;
	usb_fill_bulk_urb (urb, udev, pipe, NULL, bytes, simple_callback, NULL);
#ifdef CONFIG_MV_INCLUDE_USB
	if(desc != NULL) {
		if( (udev->speed == USB_SPEED_HIGH) ||
			((desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) )
			urb->interval = (1 << (desc->bInterval - 1));
	else
		urb->interval = desc->bInterval;
	}
#else
	urb->interval = (udev->speed == USB_SPEED_HIGH)
			? (INTERRUPT_RATE << 3)
			: INTERRUPT_RATE;
#endif
	urb->transfer_flags = URB_NO_TRANSFER_DMA_MAP;
	if (usb_pipein (pipe))
		urb->transfer_flags |= URB_SHORT_NOT_OK;
	urb->transfer_buffer = usb_buffer_alloc (udev, bytes, GFP_KERNEL,
			&urb->transfer_dma);
	if (!urb->transfer_buffer) {
		usb_free_urb (urb);
		urb = NULL;
	} else
		memset (urb->transfer_buffer, 0, bytes);
	return urb;
}

static unsigned pattern = 0;
module_param (pattern, uint, S_IRUGO);
MODULE_PARM_DESC(pattern, "i/o pattern (0 == zeroes)");

static inline void simple_fill_buf (struct urb *urb)
{
	unsigned	i;
	u8		*buf = urb->transfer_buffer;
	unsigned	len = urb->transfer_buffer_length;

	switch (pattern) {
	default:
		
	case 0:
		memset (buf, 0, len);
		break;
	case 1:			
		for (i = 0; i < len; i++)
			*buf++ = (u8) (i % 63);
		break;
	}
}

static inline int simple_check_buf(struct usbtest_dev *tdev, struct urb *urb)
{
	unsigned	i;
	u8		expected;
	u8		*buf = urb->transfer_buffer;
	unsigned	len = urb->actual_length;

	for (i = 0; i < len; i++, buf++) {
		switch (pattern) {
		
		case 0:
			expected = 0;
			break;
#ifdef CONFIG_MV_INCLUDE_USB
		
		case 1:			
#else
		
		case 1:			
#endif
			expected = i % 63;
			break;
		
		default:
			expected = !*buf;
			break;
		}
		if (*buf == expected)
			continue;
		ERROR(tdev, "buf[%d] = %d (not %d)\n", i, *buf, expected);
		return -EINVAL;
	}
	return 0;
}

static void simple_free_urb (struct urb *urb)
{
#ifdef CONFIG_MV_INCLUDE_USB
	if(urb != NULL)
	{
		if((urb->transfer_buffer != NULL) && (urb->transfer_buffer_length > 0))
			usb_buffer_free (urb->dev, urb->transfer_buffer_length, urb->transfer_buffer, urb->transfer_dma);
		usb_free_urb (urb);
	}
#else
	usb_buffer_free (urb->dev, urb->transfer_buffer_length,
			urb->transfer_buffer, urb->transfer_dma);
	usb_free_urb (urb);
#endif
}

static int simple_io (
	struct usbtest_dev	*tdev,
	struct urb		*urb,
	int			iterations,
	int			vary,
	int			expected,
	const char		*label
)
{
	struct usb_device	*udev = urb->dev;
	int			max = urb->transfer_buffer_length;
	struct completion	completion;
	int			retval = 0;

	urb->context = &completion;
	while (retval == 0 && iterations-- > 0) {
		init_completion (&completion);
		if (usb_pipeout (urb->pipe))
			simple_fill_buf (urb);
		if ((retval = usb_submit_urb (urb, GFP_KERNEL)) != 0)
			break;

		
		wait_for_completion (&completion);
		retval = urb->status;
		urb->dev = udev;
		if (retval == 0 && usb_pipein (urb->pipe))
			retval = simple_check_buf(tdev, urb);

		if (vary) {
			int	len = urb->transfer_buffer_length;

			len += vary;
			len %= max;
			if (len == 0)
				len = (vary < max) ? vary : max;
			urb->transfer_buffer_length = len;
		}

		
	}
	urb->transfer_buffer_length = max;

	if (expected != retval)
		dev_err(&udev->dev,
			"%s failed, iterations left %d, status %d (not %d)\n",
				label, iterations, retval, expected);
	return retval;
}





static void free_sglist (struct scatterlist *sg, int nents)
{
	unsigned		i;

	if (!sg)
		return;
	for (i = 0; i < nents; i++) {
		if (!sg_page(&sg[i]))
			continue;
		kfree (sg_virt(&sg[i]));
	}
	kfree (sg);
}

static struct scatterlist *
alloc_sglist (int nents, int max, int vary)
{
	struct scatterlist	*sg;
	unsigned		i;
	unsigned		size = max;

	sg = kmalloc (nents * sizeof *sg, GFP_KERNEL);
	if (!sg)
		return NULL;
#ifdef CONFIG_MV_INCLUDE_USB
	memset(sg, 0, nents * sizeof *sg);
#endif
	sg_init_table(sg, nents);

	for (i = 0; i < nents; i++) {
		char		*buf;
		unsigned	j;

		buf = kzalloc (size, GFP_KERNEL);
		if (!buf) {
			free_sglist (sg, i);
			return NULL;
		}

		
		sg_set_buf(&sg[i], buf, size);

		switch (pattern) {
		case 0:
			
			break;
		case 1:
			for (j = 0; j < size; j++)
				*buf++ = (u8) (j % 63);
			break;
		}

		if (vary) {
			size += vary;
			size %= max;
			if (size == 0)
				size = (vary < max) ? vary : max;
		}
	}

	return sg;
}

static int perform_sglist (
	struct usbtest_dev	*tdev,
	unsigned		iterations,
	int			pipe,
#ifdef CONFIG_MV_INCLUDE_USB
	struct usb_endpoint_descriptor *desc,
#endif
	struct usb_sg_request	*req,
	struct scatterlist	*sg,
	int			nents
)
{
	struct usb_device	*udev = testdev_to_usbdev(tdev);
	int			retval = 0;
#ifdef CONFIG_MV_INCLUDE_USB
	int         interval = 0;
#endif

	while (retval == 0 && iterations-- > 0) {
#ifdef CONFIG_MV_INCLUDE_USB
		if(desc != NULL) {
			if( (udev->speed == USB_SPEED_HIGH) ||
				( (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) )
				interval = (1 << (desc->bInterval - 1));
			else
				interval = desc->bInterval;
		}

		retval = usb_sg_init (req, udev, pipe, interval,
					sg, nents, 0, GFP_KERNEL);
#else
		retval = usb_sg_init (req, udev, pipe,
				(udev->speed == USB_SPEED_HIGH)
					? (INTERRUPT_RATE << 3)
					: INTERRUPT_RATE,
				sg, nents, 0, GFP_KERNEL);

#endif
		if (retval)
			break;
		usb_sg_wait (req);
		retval = req->status;

		

		
	}

	
	

	if (retval)
		ERROR(tdev, "perform_sglist failed, "
				"iterations left %d, status %d\n",
				iterations, retval);
	return retval;
}





static unsigned realworld = 1;
module_param (realworld, uint, 0);
MODULE_PARM_DESC (realworld, "clear to demand stricter spec compliance");

static int get_altsetting (struct usbtest_dev *dev)
{
	struct usb_interface	*iface = dev->intf;
	struct usb_device	*udev = interface_to_usbdev (iface);
	int			retval;

	retval = usb_control_msg (udev, usb_rcvctrlpipe (udev, 0),
			USB_REQ_GET_INTERFACE, USB_DIR_IN|USB_RECIP_INTERFACE,
			0, iface->altsetting [0].desc.bInterfaceNumber,
			dev->buf, 1, USB_CTRL_GET_TIMEOUT);
	switch (retval) {
	case 1:
		return dev->buf [0];
	case 0:
		retval = -ERANGE;
		
	default:
		return retval;
	}
}

static int set_altsetting (struct usbtest_dev *dev, int alternate)
{
	struct usb_interface		*iface = dev->intf;
	struct usb_device		*udev;

	if (alternate < 0 || alternate >= 256)
		return -EINVAL;

	udev = interface_to_usbdev (iface);
	return usb_set_interface (udev,
			iface->altsetting [0].desc.bInterfaceNumber,
			alternate);
}

static int is_good_config(struct usbtest_dev *tdev, int len)
{
	struct usb_config_descriptor	*config;

	if (len < sizeof *config)
		return 0;
	config = (struct usb_config_descriptor *) tdev->buf;

	switch (config->bDescriptorType) {
	case USB_DT_CONFIG:
	case USB_DT_OTHER_SPEED_CONFIG:
		if (config->bLength != 9) {
			ERROR(tdev, "bogus config descriptor length\n");
			return 0;
		}
		
		if (!realworld && !(config->bmAttributes & 0x80)) {
			ERROR(tdev, "high bit of config attributes not set\n");
			return 0;
		}
		if (config->bmAttributes & 0x1f) {	
			ERROR(tdev, "reserved config bits set\n");
			return 0;
		}
		break;
	default:
		return 0;
	}

	if (le16_to_cpu(config->wTotalLength) == len)		
		return 1;
	if (le16_to_cpu(config->wTotalLength) >= TBUF_SIZE)		
		return 1;
	ERROR(tdev, "bogus config descriptor read size\n");
	return 0;
}


static int ch9_postconfig (struct usbtest_dev *dev)
{
	struct usb_interface	*iface = dev->intf;
	struct usb_device	*udev = interface_to_usbdev (iface);
	int			i, alt, retval;

	
	for (i = 0; i < iface->num_altsetting; i++) {

		
		alt = iface->altsetting [i].desc.bAlternateSetting;
		if (alt < 0 || alt >= iface->num_altsetting) {
			dev_err(&iface->dev,
					"invalid alt [%d].bAltSetting = %d\n",
					i, alt);
		}

		
		if (realworld && iface->num_altsetting == 1)
			continue;

		
		retval = set_altsetting (dev, alt);
		if (retval) {
			dev_err(&iface->dev, "can't set_interface = %d, %d\n",
					alt, retval);
			return retval;
		}

		
		retval = get_altsetting (dev);
		if (retval != alt) {
			dev_err(&iface->dev, "get alt should be %d, was %d\n",
					alt, retval);
			return (retval < 0) ? retval : -EDOM;
		}

	}

	
	if (!realworld || udev->descriptor.bNumConfigurations != 1) {
		int	expected = udev->actconfig->desc.bConfigurationValue;

		
		retval = usb_control_msg (udev, usb_rcvctrlpipe (udev, 0),
				USB_REQ_GET_CONFIGURATION,
				USB_DIR_IN | USB_RECIP_DEVICE,
				0, 0, dev->buf, 1, USB_CTRL_GET_TIMEOUT);
		if (retval != 1 || dev->buf [0] != expected) {
			dev_err(&iface->dev, "get config --> %d %d (1 %d)\n",
				retval, dev->buf[0], expected);
			return (retval < 0) ? retval : -EDOM;
		}
	}

	
	retval = usb_get_descriptor (udev, USB_DT_DEVICE, 0,
			dev->buf, sizeof udev->descriptor);
	if (retval != sizeof udev->descriptor) {
		dev_err(&iface->dev, "dev descriptor --> %d\n", retval);
		return (retval < 0) ? retval : -EDOM;
	}

	
	for (i = 0; i < udev->descriptor.bNumConfigurations; i++) {
		retval = usb_get_descriptor (udev, USB_DT_CONFIG, i,
				dev->buf, TBUF_SIZE);
		if (!is_good_config(dev, retval)) {
			dev_err(&iface->dev,
					"config [%d] descriptor --> %d\n",
					i, retval);
			return (retval < 0) ? retval : -EDOM;
		}

		
		
	}

	
	if (le16_to_cpu(udev->descriptor.bcdUSB) == 0x0200) {
		struct usb_qualifier_descriptor		*d = NULL;

		
		retval = usb_get_descriptor (udev,
				USB_DT_DEVICE_QUALIFIER, 0, dev->buf,
				sizeof (struct usb_qualifier_descriptor));
		if (retval == -EPIPE) {
			if (udev->speed == USB_SPEED_HIGH) {
				dev_err(&iface->dev,
						"hs dev qualifier --> %d\n",
						retval);
				return (retval < 0) ? retval : -EDOM;
			}
			
		} else if (retval != sizeof (struct usb_qualifier_descriptor)) {
			dev_err(&iface->dev, "dev qualifier --> %d\n", retval);
			return (retval < 0) ? retval : -EDOM;
		} else
			d = (struct usb_qualifier_descriptor *) dev->buf;

		
		if (d) {
			unsigned max = d->bNumConfigurations;
			for (i = 0; i < max; i++) {
				retval = usb_get_descriptor (udev,
					USB_DT_OTHER_SPEED_CONFIG, i,
					dev->buf, TBUF_SIZE);
				if (!is_good_config(dev, retval)) {
					dev_err(&iface->dev,
						"other speed config --> %d\n",
						retval);
					return (retval < 0) ? retval : -EDOM;
				}
			}
		}
	}
	

	
	retval = usb_get_status (udev, USB_RECIP_DEVICE, 0, dev->buf);
	if (retval != 2) {
		dev_err(&iface->dev, "get dev status --> %d\n", retval);
		return (retval < 0) ? retval : -EDOM;
	}

	
	

	retval = usb_get_status (udev, USB_RECIP_INTERFACE,
			iface->altsetting [0].desc.bInterfaceNumber, dev->buf);
	if (retval != 2) {
		dev_err(&iface->dev, "get interface status --> %d\n", retval);
		return (retval < 0) ? retval : -EDOM;
	}
	

	return 0;
}





struct ctrl_ctx {
	spinlock_t		lock;
	struct usbtest_dev	*dev;
	struct completion	complete;
	unsigned		count;
	unsigned		pending;
	int			status;
	struct urb		**urb;
	struct usbtest_param	*param;
	int			last;
};

#define NUM_SUBCASES	15		

struct subcase {
	struct usb_ctrlrequest	setup;
	int			number;
	int			expected;
};

static void ctrl_complete (struct urb *urb)
{
	struct ctrl_ctx		*ctx = urb->context;
	struct usb_ctrlrequest	*reqp;
	struct subcase		*subcase;
	int			status = urb->status;

	reqp = (struct usb_ctrlrequest *)urb->setup_packet;
	subcase = container_of (reqp, struct subcase, setup);

	spin_lock (&ctx->lock);
	ctx->count--;
	ctx->pending--;

	
	if (subcase->number > 0) {
		if ((subcase->number - ctx->last) != 1) {
			ERROR(ctx->dev,
				"subcase %d completed out of order, last %d\n",
				subcase->number, ctx->last);
			status = -EDOM;
			ctx->last = subcase->number;
			goto error;
		}
	}
	ctx->last = subcase->number;

	
	if (status == subcase->expected)
		status = 0;

	
	else if (status != -ECONNRESET) {

		
		if (subcase->expected > 0 && (
			  ((status == -subcase->expected	
			   || status == 0))))			
			status = 0;
		
		else if (subcase->number == 12 && status == -EPIPE)
			status = 0;
		else
			ERROR(ctx->dev, "subtest %d error, status %d\n",
					subcase->number, status);
	}

	
	if (status) {
error:
		if (ctx->status == 0) {
			int		i;

			ctx->status = status;
			ERROR(ctx->dev, "control queue %02x.%02x, err %d, "
					"%d left, subcase %d, len %d/%d\n",
					reqp->bRequestType, reqp->bRequest,
					status, ctx->count, subcase->number,
					urb->actual_length,
					urb->transfer_buffer_length);

			

			
			for (i = 1; i < ctx->param->sglen; i++) {
				struct urb	*u = ctx->urb [
						(i + subcase->number)
						% ctx->param->sglen];

				if (u == urb || !u->dev)
					continue;
				spin_unlock(&ctx->lock);
				status = usb_unlink_urb (u);
				spin_lock(&ctx->lock);
				switch (status) {
				case -EINPROGRESS:
				case -EBUSY:
				case -EIDRM:
					continue;
				default:
					ERROR(ctx->dev, "urb unlink --> %d\n",
							status);
				}
			}
			status = ctx->status;
		}
	}

	
	if ((status == 0) && (ctx->pending < ctx->count)) {
		if ((status = usb_submit_urb (urb, GFP_ATOMIC)) != 0) {
			ERROR(ctx->dev,
				"can't resubmit ctrl %02x.%02x, err %d\n",
				reqp->bRequestType, reqp->bRequest, status);
			urb->dev = NULL;
		} else
			ctx->pending++;
	} else
		urb->dev = NULL;

	
	if (ctx->pending == 0)
		complete (&ctx->complete);
	spin_unlock (&ctx->lock);
}

static int
test_ctrl_queue (struct usbtest_dev *dev, struct usbtest_param *param)
{
	struct usb_device	*udev = testdev_to_usbdev (dev);
	struct urb		**urb;
	struct ctrl_ctx		context;
	int			i;

	spin_lock_init (&context.lock);
	context.dev = dev;
	init_completion (&context.complete);
	context.count = param->sglen * param->iterations;
	context.pending = 0;
	context.status = -ENOMEM;
	context.param = param;
	context.last = -1;

	
	urb = kcalloc(param->sglen, sizeof(struct urb *), GFP_KERNEL);
	if (!urb)
		return -ENOMEM;
	for (i = 0; i < param->sglen; i++) {
		int			pipe = usb_rcvctrlpipe (udev, 0);
		unsigned		len;
		struct urb		*u;
		struct usb_ctrlrequest	req;
		struct subcase		*reqp;

		
		int			expected = 0;

		
		memset (&req, 0, sizeof req);
		req.bRequest = USB_REQ_GET_DESCRIPTOR;
		req.bRequestType = USB_DIR_IN|USB_RECIP_DEVICE;

		switch (i % NUM_SUBCASES) {
		case 0:		
			req.wValue = cpu_to_le16 (USB_DT_DEVICE << 8);
			len = sizeof (struct usb_device_descriptor);
			break;
		case 1:		
			req.wValue = cpu_to_le16 ((USB_DT_CONFIG << 8) | 0);
			len = sizeof (struct usb_config_descriptor);
			break;
		case 2:		
			req.bRequest = USB_REQ_GET_INTERFACE;
			req.bRequestType = USB_DIR_IN|USB_RECIP_INTERFACE;
			
			len = 1;
			expected = EPIPE;
			break;
		case 3:		
			req.bRequest = USB_REQ_GET_STATUS;
			req.bRequestType = USB_DIR_IN|USB_RECIP_INTERFACE;
			
			len = 2;
			break;
		case 4:		
			req.bRequest = USB_REQ_GET_STATUS;
			req.bRequestType = USB_DIR_IN|USB_RECIP_DEVICE;
			len = 2;
			break;
		case 5:		
			req.wValue = cpu_to_le16 (USB_DT_DEVICE_QUALIFIER << 8);
			len = sizeof (struct usb_qualifier_descriptor);
			if (udev->speed != USB_SPEED_HIGH)
				expected = EPIPE;
			break;
		case 6:		
			req.wValue = cpu_to_le16 ((USB_DT_CONFIG << 8) | 0);
			len = sizeof (struct usb_config_descriptor);
			len += sizeof (struct usb_interface_descriptor);
			break;
		case 7:		
			req.wValue = cpu_to_le16 (USB_DT_INTERFACE << 8);
			
			len = sizeof (struct usb_interface_descriptor);
			expected = -EPIPE;
			break;
		
		
		case 8:		
			req.bRequest = USB_REQ_CLEAR_FEATURE;
			req.bRequestType = USB_RECIP_ENDPOINT;
			
			
			len = 0;
			pipe = usb_sndctrlpipe (udev, 0);
			expected = EPIPE;
			break;
		case 9:		
			req.bRequest = USB_REQ_GET_STATUS;
			req.bRequestType = USB_DIR_IN|USB_RECIP_ENDPOINT;
			
			len = 2;
			break;
		case 10:	
			req.wValue = cpu_to_le16 ((USB_DT_CONFIG << 8) | 0);
			len = 1024;
			expected = -EREMOTEIO;
			break;
		
		case 11:	
			req.wValue = cpu_to_le16 (USB_DT_ENDPOINT << 8);
			
			len = sizeof (struct usb_interface_descriptor);
			expected = EPIPE;
			break;
		
		case 12:	
			req.wValue = cpu_to_le16 (USB_DT_STRING << 8);
			
			len = sizeof (struct usb_interface_descriptor);
			
			expected = EREMOTEIO;	
			break;
		case 13:	
			req.wValue = cpu_to_le16 ((USB_DT_CONFIG << 8) | 0);
			
			len = 1024 - udev->descriptor.bMaxPacketSize0;
			expected = -EREMOTEIO;
			break;
		case 14:	
			req.wValue = cpu_to_le16 ((USB_DT_DEVICE << 8) | 0);
			
			len = udev->descriptor.bMaxPacketSize0;
			switch (len) {
			case 8:		len = 24; break;
			case 16:	len = 32; break;
			}
			expected = -EREMOTEIO;
			break;
		default:
			ERROR(dev, "bogus number of ctrl queue testcases!\n");
			context.status = -EINVAL;
			goto cleanup;
		}
		req.wLength = cpu_to_le16 (len);
#ifdef CONFIG_MV_INCLUDE_USB
		urb [i] = u = simple_alloc_urb (udev, pipe, NULL, len);
#else
		urb [i] = u = simple_alloc_urb (udev, pipe, len);
#endif
		if (!u)
			goto cleanup;

		reqp = usb_buffer_alloc (udev, sizeof *reqp, GFP_KERNEL,
				&u->setup_dma);
		if (!reqp)
			goto cleanup;
		reqp->setup = req;
		reqp->number = i % NUM_SUBCASES;
		reqp->expected = expected;
		u->setup_packet = (char *) &reqp->setup;
		u->transfer_flags |= URB_NO_SETUP_DMA_MAP;

		u->context = &context;
		u->complete = ctrl_complete;
#ifdef CONFIG_SYNO_PLX_PORTING
		u->transfer_flags |= URB_NO_SETUP_DMA_MAP;
#endif
	}

	
	context.urb = urb;
	spin_lock_irq (&context.lock);
	for (i = 0; i < param->sglen; i++) {
		context.status = usb_submit_urb (urb [i], GFP_ATOMIC);
		if (context.status != 0) {
			ERROR(dev, "can't submit urb[%d], status %d\n",
					i, context.status);
			context.count = context.pending;
			break;
		}
		context.pending++;
	}
	spin_unlock_irq (&context.lock);

	

	
	if (context.pending > 0)
		wait_for_completion (&context.complete);

cleanup:
	for (i = 0; i < param->sglen; i++) {
		if (!urb [i])
			continue;
		urb [i]->dev = udev;
		if (urb [i]->setup_packet)
			usb_buffer_free (udev, sizeof (struct usb_ctrlrequest),
					urb [i]->setup_packet,
					urb [i]->setup_dma);
		simple_free_urb (urb [i]);
	}
	kfree (urb);
	return context.status;
}
#undef NUM_SUBCASES



static void unlink1_callback (struct urb *urb)
{
	int	status = urb->status;

	
	if (!status)
		status = usb_submit_urb (urb, GFP_ATOMIC);
	if (status) {
		urb->status = status;
		complete(urb->context);
	}
}

#ifdef CONFIG_MV_INCLUDE_USB
static int unlink1 (struct usbtest_dev *dev, int pipe,
                    struct usb_endpoint_descriptor *desc, int size, int async)
#else
static int unlink1 (struct usbtest_dev *dev, int pipe, int size, int async)
#endif
{
	struct urb		*urb;
	struct completion	completion;
	int			retval = 0;

	init_completion (&completion);
#ifdef CONFIG_MV_INCLUDE_USB
	urb = simple_alloc_urb (testdev_to_usbdev (dev), pipe, desc, size);
#else
	urb = simple_alloc_urb (testdev_to_usbdev (dev), pipe, size);
#endif
	if (!urb)
		return -ENOMEM;
	urb->context = &completion;
	urb->complete = unlink1_callback;

	
	if ((retval = usb_submit_urb (urb, GFP_KERNEL)) != 0) {
		dev_err(&dev->intf->dev, "submit fail %d\n", retval);
		return retval;
	}

	
#ifdef CONFIG_MV_INCLUDE_USB
	msleep (jiffies % (2 * UNLINK_RATE));
#else
	msleep (jiffies % (2 * INTERRUPT_RATE));
#endif
	if (async) {
		while (!completion_done(&completion)) {
			retval = usb_unlink_urb(urb);

			switch (retval) {
			case -EBUSY:
			case -EIDRM:
				
				ERROR(dev, "unlink retry\n");
				continue;
			case 0:
			case -EINPROGRESS:
				break;

			default:
				dev_err(&dev->intf->dev,
					"unlink fail %d\n", retval);
				return retval;
			}

			break;
		}
	} else
		usb_kill_urb (urb);

	wait_for_completion (&completion);
	retval = urb->status;
	simple_free_urb (urb);

	if (async)
		return (retval == -ECONNRESET) ? 0 : retval - 1000;
	else
		return (retval == -ENOENT || retval == -EPERM) ?
				0 : retval - 2000;
}

#ifdef CONFIG_MV_INCLUDE_USB
static int unlink_simple (struct usbtest_dev *dev, int pipe,
                          struct usb_endpoint_descriptor *desc, int len)
#else
static int unlink_simple (struct usbtest_dev *dev, int pipe, int len)
#endif
{
	int			retval = 0;

	
#ifdef CONFIG_MV_INCLUDE_USB
	retval = unlink1 (dev, pipe, desc, len, 1);
#else
	retval = unlink1 (dev, pipe, len, 1);
#endif
	if (!retval)
#ifdef CONFIG_MV_INCLUDE_USB
		retval = unlink1 (dev, pipe, desc, len, 0);
#else
		retval = unlink1 (dev, pipe, len, 0);
#endif
	return retval;
}



static int verify_not_halted(struct usbtest_dev *tdev, int ep, struct urb *urb)
{
	int	retval;
	u16	status;

	
	retval = usb_get_status (urb->dev, USB_RECIP_ENDPOINT, ep, &status);
	if (retval < 0) {
		ERROR(tdev, "ep %02x couldn't get no-halt status, %d\n",
				ep, retval);
		return retval;
	}
	if (status != 0) {
		ERROR(tdev, "ep %02x bogus status: %04x != 0\n", ep, status);
		return -EINVAL;
	}
	retval = simple_io(tdev, urb, 1, 0, 0, __func__);
	if (retval != 0)
		return -EINVAL;
	return 0;
}

static int verify_halted(struct usbtest_dev *tdev, int ep, struct urb *urb)
{
	int	retval;
	u16	status;

	
	retval = usb_get_status (urb->dev, USB_RECIP_ENDPOINT, ep, &status);
	if (retval < 0) {
		ERROR(tdev, "ep %02x couldn't get halt status, %d\n",
				ep, retval);
		return retval;
	}
	le16_to_cpus(&status);
	if (status != 1) {
		ERROR(tdev, "ep %02x bogus status: %04x != 1\n", ep, status);
		return -EINVAL;
	}
	retval = simple_io(tdev, urb, 1, 0, -EPIPE, __func__);
	if (retval != -EPIPE)
		return -EINVAL;
	retval = simple_io(tdev, urb, 1, 0, -EPIPE, "verify_still_halted");
	if (retval != -EPIPE)
		return -EINVAL;
	return 0;
}

static int test_halt(struct usbtest_dev *tdev, int ep, struct urb *urb)
{
	int	retval;

	
	retval = verify_not_halted(tdev, ep, urb);
	if (retval < 0)
		return retval;

	
	retval = usb_control_msg (urb->dev, usb_sndctrlpipe (urb->dev, 0),
			USB_REQ_SET_FEATURE, USB_RECIP_ENDPOINT,
			USB_ENDPOINT_HALT, ep,
			NULL, 0, USB_CTRL_SET_TIMEOUT);
	if (retval < 0) {
		ERROR(tdev, "ep %02x couldn't set halt, %d\n", ep, retval);
		return retval;
	}
	retval = verify_halted(tdev, ep, urb);
	if (retval < 0)
		return retval;

	
	retval = usb_clear_halt (urb->dev, urb->pipe);
	if (retval < 0) {
		ERROR(tdev, "ep %02x couldn't clear halt, %d\n", ep, retval);
		return retval;
	}
	retval = verify_not_halted(tdev, ep, urb);
	if (retval < 0)
		return retval;

	

	return 0;
}

static int halt_simple (struct usbtest_dev *dev)
{
	int		ep;
	int		retval = 0;
	struct urb	*urb;

#ifdef CONFIG_MV_INCLUDE_USB
	urb = simple_alloc_urb (testdev_to_usbdev (dev), 0, NULL, 512);
#else
#ifdef CONFIG_SYNO_PLX_PORTING
 	urb = simple_alloc_urb (testdev_to_usbdev (dev), 0, 256);
#else
	urb = simple_alloc_urb (testdev_to_usbdev (dev), 0, 512);
#endif
#endif
	if (urb == NULL)
		return -ENOMEM;

	if (dev->in_pipe) {
		ep = usb_pipeendpoint (dev->in_pipe) | USB_DIR_IN;
		urb->pipe = dev->in_pipe;
		retval = test_halt(dev, ep, urb);
		if (retval < 0)
			goto done;
	}

	if (dev->out_pipe) {
		ep = usb_pipeendpoint (dev->out_pipe);
		urb->pipe = dev->out_pipe;
		retval = test_halt(dev, ep, urb);
	}
done:
	simple_free_urb (urb);
	return retval;
}




static int ctrl_out (struct usbtest_dev *dev,
		unsigned count, unsigned length, unsigned vary)
{
	unsigned		i, j, len;
	int			retval;
	u8			*buf;
	char			*what = "?";
	struct usb_device	*udev;

	if (length < 1 || length > 0xffff || vary >= length)
		return -EINVAL;

	buf = kmalloc(length, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	udev = testdev_to_usbdev (dev);
	len = length;
	retval = 0;

	
	for (i = 0; i < count; i++) {
		
		for (j = 0; j < len; j++)
			buf [j] = i + j;
		retval = usb_control_msg (udev, usb_sndctrlpipe (udev,0),
				0x5b, USB_DIR_OUT|USB_TYPE_VENDOR,
				0, 0, buf, len, USB_CTRL_SET_TIMEOUT);
		if (retval != len) {
			what = "write";
			if (retval >= 0) {
				ERROR(dev, "ctrl_out, wlen %d (expected %d)\n",
						retval, len);
				retval = -EBADMSG;
			}
			break;
		}

		
		retval = usb_control_msg (udev, usb_rcvctrlpipe (udev,0),
				0x5c, USB_DIR_IN|USB_TYPE_VENDOR,
				0, 0, buf, len, USB_CTRL_GET_TIMEOUT);
		if (retval != len) {
			what = "read";
			if (retval >= 0) {
				ERROR(dev, "ctrl_out, rlen %d (expected %d)\n",
						retval, len);
				retval = -EBADMSG;
			}
			break;
		}

		
		for (j = 0; j < len; j++) {
			if (buf [j] != (u8) (i + j)) {
				ERROR(dev, "ctrl_out, byte %d is %d not %d\n",
					j, buf [j], (u8) i + j);
				retval = -EBADMSG;
				break;
			}
		}
		if (retval < 0) {
			what = "verify";
			break;
		}

		len += vary;

		
		if (len > length)
			len = realworld ? 1 : 0;
	}

	if (retval < 0)
		ERROR (dev, "ctrl_out %s failed, code %d, count %d\n",
			what, retval, i);

	kfree (buf);
	return retval;
}





struct iso_context {
	unsigned		count;
	unsigned		pending;
	spinlock_t		lock;
	struct completion	done;
	int			submit_error;
	unsigned long		errors;
	unsigned long		packet_count;
	struct usbtest_dev	*dev;
};

static void iso_callback (struct urb *urb)
{
	struct iso_context	*ctx = urb->context;

	spin_lock(&ctx->lock);
	ctx->count--;

	ctx->packet_count += urb->number_of_packets;
	if (urb->error_count > 0)
		ctx->errors += urb->error_count;
	else if (urb->status != 0)
		ctx->errors += urb->number_of_packets;

	if (urb->status == 0 && ctx->count > (ctx->pending - 1)
			&& !ctx->submit_error) {
		int status = usb_submit_urb (urb, GFP_ATOMIC);
		switch (status) {
		case 0:
			goto done;
		default:
			dev_err(&ctx->dev->intf->dev,
					"iso resubmit err %d\n",
					status);
			
		case -ENODEV:			
		case -ESHUTDOWN:		
			ctx->submit_error = 1;
			break;
		}
	}
#ifdef CONFIG_MV_INCLUDE_USB
	
#else
	simple_free_urb (urb);
#endif

	ctx->pending--;
	if (ctx->pending == 0) {
		if (ctx->errors)
			dev_err(&ctx->dev->intf->dev,
				"iso test, %lu errors out of %lu\n",
				ctx->errors, ctx->packet_count);
		complete (&ctx->done);
	}
done:
	spin_unlock(&ctx->lock);
}

static struct urb *iso_alloc_urb (
	struct usb_device	*udev,
	int			pipe,
	struct usb_endpoint_descriptor	*desc,
	long			bytes
)
{
	struct urb		*urb;
	unsigned		i, maxp, packets;

	if (bytes < 0 || !desc)
		return NULL;
	maxp = 0x7ff & le16_to_cpu(desc->wMaxPacketSize);
	maxp *= 1 + (0x3 & (le16_to_cpu(desc->wMaxPacketSize) >> 11));
	packets = DIV_ROUND_UP(bytes, maxp);

	urb = usb_alloc_urb (packets, GFP_KERNEL);
	if (!urb)
		return urb;
	urb->dev = udev;
	urb->pipe = pipe;

	urb->number_of_packets = packets;
	urb->transfer_buffer_length = bytes;
	urb->transfer_buffer = usb_buffer_alloc (udev, bytes, GFP_KERNEL,
			&urb->transfer_dma);
	if (!urb->transfer_buffer) {
		usb_free_urb (urb);
		return NULL;
	}
	memset (urb->transfer_buffer, 0, bytes);
	for (i = 0; i < packets; i++) {
		
		urb->iso_frame_desc[i].length = min ((unsigned) bytes, maxp);
		bytes -= urb->iso_frame_desc[i].length;

		urb->iso_frame_desc[i].offset = maxp * i;
	}

	urb->complete = iso_callback;
	
	urb->interval = 1 << (desc->bInterval - 1);
	urb->transfer_flags = URB_ISO_ASAP | URB_NO_TRANSFER_DMA_MAP;
	return urb;
}

static int
test_iso_queue (struct usbtest_dev *dev, struct usbtest_param *param,
		int pipe, struct usb_endpoint_descriptor *desc)
{
	struct iso_context	context;
	struct usb_device	*udev;
	unsigned		i;
	unsigned long		packets = 0;
	int			status = 0;
#ifdef CONFIG_MV_INCLUDE_USB
	struct urb		*urbs[50];	
#else
	struct urb		*urbs[10];	
#endif

#ifdef CONFIG_MV_INCLUDE_USB
	if (param->sglen > 50)
#else
	if (param->sglen > 10)
#endif
		return -EDOM;

	memset(&context, 0, sizeof context);
	context.count = param->iterations * param->sglen;
	context.dev = dev;
	init_completion (&context.done);
	spin_lock_init (&context.lock);

	memset (urbs, 0, sizeof urbs);
	udev = testdev_to_usbdev (dev);
	dev_info(&dev->intf->dev,
		"... iso period %d %sframes, wMaxPacket %04x\n",
		1 << (desc->bInterval - 1),
		(udev->speed == USB_SPEED_HIGH) ? "micro" : "",
		le16_to_cpu(desc->wMaxPacketSize));

	for (i = 0; i < param->sglen; i++) {
		urbs [i] = iso_alloc_urb (udev, pipe, desc,
				param->length);
		if (!urbs [i]) {
			status = -ENOMEM;
			goto fail;
		}
		packets += urbs[i]->number_of_packets;
		urbs [i]->context = &context;
	}
	packets *= param->iterations;
	dev_info(&dev->intf->dev,
		"... total %lu msec (%lu packets)\n",
		(packets * (1 << (desc->bInterval - 1)))
			/ ((udev->speed == USB_SPEED_HIGH) ? 8 : 1),
		packets);

	spin_lock_irq (&context.lock);
	for (i = 0; i < param->sglen; i++) {
#ifdef CONFIG_MV_INCLUDE_USB
		
#endif
		++context.pending;
		status = usb_submit_urb (urbs [i], GFP_ATOMIC);
		if (status < 0) {
			ERROR (dev, "submit iso[%d], error %d\n", i, status);
			if (i == 0) {
				spin_unlock_irq (&context.lock);
				goto fail;
			}

#ifdef CONFIG_MV_INCLUDE_USB
			
#else
			simple_free_urb (urbs [i]);
#endif
			context.pending--;
			context.submit_error = 1;
			break;
		}
	}
	spin_unlock_irq (&context.lock);

	wait_for_completion (&context.done);

	
	if (status != 0)
		;
	else if (context.submit_error)
		status = -EACCES;
	else if (context.errors > context.packet_count / 10)
		status = -EIO;
#ifndef CONFIG_MV_INCLUDE_USB
	return status;
#endif

fail:
	for (i = 0; i < param->sglen; i++) {
		if (urbs [i])
			simple_free_urb (urbs [i]);
	}
	return status;
}





static int
usbtest_ioctl (struct usb_interface *intf, unsigned int code, void *buf)
{
	struct usbtest_dev	*dev = usb_get_intfdata (intf);
	struct usb_device	*udev = testdev_to_usbdev (dev);
	struct usbtest_param	*param = buf;
	int			retval = -EOPNOTSUPP;
	struct urb		*urb;
	struct scatterlist	*sg;
	struct usb_sg_request	req;
	struct timeval		start;
	unsigned		i;

	

	if (code != USBTEST_REQUEST)
		return -EOPNOTSUPP;

	if (param->iterations <= 0)
		return -EINVAL;

	if (mutex_lock_interruptible(&dev->lock))
		return -ERESTARTSYS;

	
	if (!intf->is_active) {
		mutex_unlock(&dev->lock);
		return -EHOSTUNREACH;
	}

	
	if (dev->info->alt >= 0) {
		int	res;

		if (intf->altsetting->desc.bInterfaceNumber) {
			mutex_unlock(&dev->lock);
			return -ENODEV;
		}
		res = set_altsetting (dev, dev->info->alt);
		if (res) {
			dev_err (&intf->dev,
					"set altsetting to %d failed, %d\n",
					dev->info->alt, res);
			mutex_unlock(&dev->lock);
			return res;
		}
	}

	
	do_gettimeofday (&start);
	switch (param->test_num) {

	case 0:
		dev_info(&intf->dev, "TEST 0:  NOP\n");
		retval = 0;
		break;

	
	case 1:
		if (dev->out_pipe == 0)
			break;
		dev_info(&intf->dev,
				"TEST 1:  write %d bytes %u times\n",
				param->length, param->iterations);
#ifdef CONFIG_MV_INCLUDE_USB
		urb = simple_alloc_urb (udev, dev->out_pipe, dev->out_desc, param->length);
#else
		urb = simple_alloc_urb (udev, dev->out_pipe, param->length);
#endif
		if (!urb) {
			retval = -ENOMEM;
			break;
		}
		
		retval = simple_io(dev, urb, param->iterations, 0, 0, "test1");
		simple_free_urb (urb);
		break;
	case 2:
		if (dev->in_pipe == 0)
			break;
		dev_info(&intf->dev,
				"TEST 2:  read %d bytes %u times\n",
				param->length, param->iterations);
#ifdef CONFIG_MV_INCLUDE_USB
		urb = simple_alloc_urb (udev, dev->out_pipe, dev->out_desc, param->length);
#else
		urb = simple_alloc_urb (udev, dev->in_pipe, param->length);
#endif
		if (!urb) {
			retval = -ENOMEM;
			break;
		}
		
		retval = simple_io(dev, urb, param->iterations, 0, 0, "test2");
		simple_free_urb (urb);
		break;
	case 3:
		if (dev->out_pipe == 0 || param->vary == 0)
			break;
		dev_info(&intf->dev,
				"TEST 3:  write/%d 0..%d bytes %u times\n",
				param->vary, param->length, param->iterations);
#ifdef CONFIG_MV_INCLUDE_USB
		urb = simple_alloc_urb (udev, dev->out_pipe, dev->out_desc, param->length);
#else
		urb = simple_alloc_urb (udev, dev->out_pipe, param->length);
#endif
		if (!urb) {
			retval = -ENOMEM;
			break;
		}
		
		retval = simple_io(dev, urb, param->iterations, param->vary,
					0, "test3");
		simple_free_urb (urb);
		break;
	case 4:
		if (dev->in_pipe == 0 || param->vary == 0)
			break;
		dev_info(&intf->dev,
				"TEST 4:  read/%d 0..%d bytes %u times\n",
				param->vary, param->length, param->iterations);
#ifdef CONFIG_MV_INCLUDE_USB
		urb = simple_alloc_urb (udev, dev->in_pipe, dev->in_desc, param->length);
#else
		urb = simple_alloc_urb (udev, dev->in_pipe, param->length);
#endif
		if (!urb) {
			retval = -ENOMEM;
			break;
		}
		
		retval = simple_io(dev, urb, param->iterations, param->vary,
					0, "test4");
		simple_free_urb (urb);
		break;

	
	case 5:
		if (dev->out_pipe == 0 || param->sglen == 0)
			break;
		dev_info(&intf->dev,
			"TEST 5:  write %d sglists %d entries of %d bytes\n",
				param->iterations,
				param->sglen, param->length);
		sg = alloc_sglist (param->sglen, param->length, 0);
		if (!sg) {
			retval = -ENOMEM;
			break;
		}
		
		retval = perform_sglist(dev, param->iterations, dev->out_pipe,
#ifdef CONFIG_MV_INCLUDE_USB
				dev->out_desc, &req, sg, param->sglen);
#else
				&req, sg, param->sglen);
#endif
		free_sglist (sg, param->sglen);
		break;

	case 6:
		if (dev->in_pipe == 0 || param->sglen == 0)
			break;
		dev_info(&intf->dev,
			"TEST 6:  read %d sglists %d entries of %d bytes\n",
				param->iterations,
				param->sglen, param->length);
		sg = alloc_sglist (param->sglen, param->length, 0);
		if (!sg) {
			retval = -ENOMEM;
			break;
		}
		
		retval = perform_sglist(dev, param->iterations, dev->in_pipe,
#ifdef CONFIG_MV_INCLUDE_USB
				dev->in_desc, &req, sg, param->sglen);
#else
				&req, sg, param->sglen);
#endif
		free_sglist (sg, param->sglen);
		break;
	case 7:
		if (dev->out_pipe == 0 || param->sglen == 0 || param->vary == 0)
			break;
		dev_info(&intf->dev,
			"TEST 7:  write/%d %d sglists %d entries 0..%d bytes\n",
				param->vary, param->iterations,
				param->sglen, param->length);
		sg = alloc_sglist (param->sglen, param->length, param->vary);
		if (!sg) {
			retval = -ENOMEM;
			break;
		}
		
		retval = perform_sglist(dev, param->iterations, dev->out_pipe,
#ifdef CONFIG_MV_INCLUDE_USB
				dev->out_desc, &req, sg, param->sglen);
#else
				&req, sg, param->sglen);
#endif
		free_sglist (sg, param->sglen);
		break;
	case 8:
		if (dev->in_pipe == 0 || param->sglen == 0 || param->vary == 0)
			break;
		dev_info(&intf->dev,
			"TEST 8:  read/%d %d sglists %d entries 0..%d bytes\n",
				param->vary, param->iterations,
				param->sglen, param->length);
		sg = alloc_sglist (param->sglen, param->length, param->vary);
		if (!sg) {
			retval = -ENOMEM;
			break;
		}
		
		retval = perform_sglist(dev, param->iterations, dev->in_pipe,
#ifdef CONFIG_MV_INCLUDE_USB
				dev->in_desc, &req, sg, param->sglen);
#else
				&req, sg, param->sglen);
#endif
		free_sglist (sg, param->sglen);
		break;

	
	case 9:
		retval = 0;
		dev_info(&intf->dev,
			"TEST 9:  ch9 (subset) control tests, %d times\n",
				param->iterations);
		for (i = param->iterations; retval == 0 && i--; )
			retval = ch9_postconfig (dev);
		if (retval)
			dev_err(&intf->dev, "ch9 subset failed, "
					"iterations left %d\n", i);
		break;

	
	case 10:
		if (param->sglen == 0)
			break;
		retval = 0;
		dev_info(&intf->dev,
				"TEST 10:  queue %d control calls, %d times\n",
				param->sglen,
				param->iterations);
		retval = test_ctrl_queue (dev, param);
		break;

	
	case 11:
		if (dev->in_pipe == 0 || !param->length)
			break;
		retval = 0;
		dev_info(&intf->dev, "TEST 11:  unlink %d reads of %d\n",
				param->iterations, param->length);
		for (i = param->iterations; retval == 0 && i--; )
#ifdef CONFIG_MV_INCLUDE_USB
			retval = unlink_simple (dev, dev->in_pipe, dev->in_desc,
#else
			retval = unlink_simple (dev, dev->in_pipe,
#endif
						param->length);
		if (retval)
			dev_err(&intf->dev, "unlink reads failed %d, "
				"iterations left %d\n", retval, i);
		break;
	case 12:
		if (dev->out_pipe == 0 || !param->length)
			break;
		retval = 0;
		dev_info(&intf->dev, "TEST 12:  unlink %d writes of %d\n",
				param->iterations, param->length);
		for (i = param->iterations; retval == 0 && i--; )
#ifdef CONFIG_MV_INCLUDE_USB
			retval = unlink_simple (dev, dev->out_pipe, dev->out_desc,
#else
			retval = unlink_simple (dev, dev->out_pipe,
#endif
						param->length);
		if (retval)
			dev_err(&intf->dev, "unlink writes failed %d, "
				"iterations left %d\n", retval, i);
		break;

	
	case 13:
		if (dev->out_pipe == 0 && dev->in_pipe == 0)
			break;
		retval = 0;
		dev_info(&intf->dev, "TEST 13:  set/clear %d halts\n",
				param->iterations);
		for (i = param->iterations; retval == 0 && i--; )
			retval = halt_simple (dev);

		if (retval)
			ERROR(dev, "halts failed, iterations left %d\n", i);
		break;

	
	case 14:
		if (!dev->info->ctrl_out)
			break;
		dev_info(&intf->dev, "TEST 14:  %d ep0out, %d..%d vary %d\n",
				param->iterations,
				realworld ? 1 : 0, param->length,
				param->vary);
		retval = ctrl_out(dev, param->iterations,
				param->length, param->vary);
		break;

	
	case 15:
		if (dev->out_iso_pipe == 0 || param->sglen == 0)
			break;
		dev_info(&intf->dev,
			"TEST 15:  write %d iso, %d entries of %d bytes\n",
				param->iterations,
				param->sglen, param->length);
		
		retval = test_iso_queue (dev, param,
				dev->out_iso_pipe, dev->iso_out);
		break;

	
	case 16:
		if (dev->in_iso_pipe == 0 || param->sglen == 0)
			break;
		dev_info(&intf->dev,
			"TEST 16:  read %d iso, %d entries of %d bytes\n",
				param->iterations,
				param->sglen, param->length);
		
		retval = test_iso_queue (dev, param,
				dev->in_iso_pipe, dev->iso_in);
		break;

	

	

	}
	do_gettimeofday (&param->duration);
	param->duration.tv_sec -= start.tv_sec;
	param->duration.tv_usec -= start.tv_usec;
	if (param->duration.tv_usec < 0) {
		param->duration.tv_usec += 1000 * 1000;
		param->duration.tv_sec -= 1;
	}
	mutex_unlock(&dev->lock);
	return retval;
}



static unsigned force_interrupt = 0;
module_param (force_interrupt, uint, 0);
MODULE_PARM_DESC (force_interrupt, "0 = test default; else interrupt");

#ifdef	GENERIC
static unsigned short vendor;
module_param(vendor, ushort, 0);
MODULE_PARM_DESC (vendor, "vendor code (from usb-if)");

static unsigned short product;
module_param(product, ushort, 0);
MODULE_PARM_DESC (product, "product code (from vendor)");
#endif

static int
usbtest_probe (struct usb_interface *intf, const struct usb_device_id *id)
{
	struct usb_device	*udev;
	struct usbtest_dev	*dev;
	struct usbtest_info	*info;
	char			*rtest, *wtest;
	char			*irtest, *iwtest;

	udev = interface_to_usbdev (intf);

#ifdef	GENERIC
	
	if (id->match_flags == 0) {
		
		if (!vendor || le16_to_cpu(udev->descriptor.idVendor) != (u16)vendor)
			return -ENODEV;
		if (product && le16_to_cpu(udev->descriptor.idProduct) != (u16)product)
			return -ENODEV;
		dev_info(&intf->dev, "matched module params, "
					"vend=0x%04x prod=0x%04x\n",
				le16_to_cpu(udev->descriptor.idVendor),
				le16_to_cpu(udev->descriptor.idProduct));
	}
#endif

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;
	info = (struct usbtest_info *) id->driver_info;
	dev->info = info;
	mutex_init(&dev->lock);

	dev->intf = intf;

	
	if ((dev->buf = kmalloc (TBUF_SIZE, GFP_KERNEL)) == NULL) {
		kfree (dev);
		return -ENOMEM;
	}

	
	rtest = wtest = "";
	irtest = iwtest = "";
	if (force_interrupt || udev->speed == USB_SPEED_LOW) {
		if (info->ep_in) {
			dev->in_pipe = usb_rcvintpipe (udev, info->ep_in);
			rtest = " intr-in";
		}
		if (info->ep_out) {
			dev->out_pipe = usb_sndintpipe (udev, info->ep_out);
			wtest = " intr-out";
		}
	} else {
		if (info->autoconf) {
			int status;

			status = get_endpoints (dev, intf);
			if (status < 0) {
				WARNING(dev, "couldn't get endpoints, %d\n",
						status);
				return status;
			}
			
		} else {
			if (info->ep_in)
				dev->in_pipe = usb_rcvbulkpipe (udev,
							info->ep_in);
			if (info->ep_out)
				dev->out_pipe = usb_sndbulkpipe (udev,
							info->ep_out);
		}
#ifdef CONFIG_MV_INCLUDE_USB
		if (dev->in_pipe) {
			if(usb_pipeint(dev->in_pipe)) {
				rtest = " intr-in";
			}
			else {
				rtest = " bulk-in";
			}
		}
		if (dev->out_pipe){
			if(usb_pipeint(dev->out_pipe)) {
				wtest = " intr-out";
			}
			else {
				wtest = " bulk-out";
			}
		}
#else
		if (dev->in_pipe)
			rtest = " bulk-in";
		if (dev->out_pipe)
			wtest = " bulk-out";
#endif
		if (dev->in_iso_pipe)
			irtest = " iso-in";
		if (dev->out_iso_pipe)
			iwtest = " iso-out";
	}

	usb_set_intfdata (intf, dev);
	dev_info (&intf->dev, "%s\n", info->name);
	dev_info (&intf->dev, "%s speed {control%s%s%s%s%s} tests%s\n",
			({ char *tmp;
			switch (udev->speed) {
			case USB_SPEED_LOW: tmp = "low"; break;
			case USB_SPEED_FULL: tmp = "full"; break;
			case USB_SPEED_HIGH: tmp = "high"; break;
			default: tmp = "unknown"; break;
			}; tmp; }),
			info->ctrl_out ? " in/out" : "",
			rtest, wtest,
			irtest, iwtest,
			info->alt >= 0 ? " (+alt)" : "");
	return 0;
}

static int usbtest_suspend (struct usb_interface *intf, pm_message_t message)
{
	return 0;
}

static int usbtest_resume (struct usb_interface *intf)
{
	return 0;
}

static void usbtest_disconnect (struct usb_interface *intf)
{
	struct usbtest_dev	*dev = usb_get_intfdata (intf);

	usb_set_intfdata (intf, NULL);
	dev_dbg (&intf->dev, "disconnect\n");
	kfree (dev);
}




static struct usbtest_info ez1_info = {
	.name		= "EZ-USB device",
	.ep_in		= 2,
	.ep_out		= 2,
	.alt		= 1,
};


static struct usbtest_info ez2_info = {
	.name		= "FX2 device",
	.ep_in		= 6,
	.ep_out		= 2,
	.alt		= 1,
};


static struct usbtest_info fw_info = {
	.name		= "usb test device",
	.ep_in		= 2,
	.ep_out		= 2,
	.alt		= 1,
	.autoconf	= 1,		
	.ctrl_out	= 1,
	.iso		= 1,		
};


static struct usbtest_info gz_info = {
	.name		= "Linux gadget zero",
	.autoconf	= 1,
	.ctrl_out	= 1,
	.alt		= 0,
};

static struct usbtest_info um_info = {
	.name		= "Linux user mode test driver",
	.autoconf	= 1,
	.alt		= -1,
};

static struct usbtest_info um2_info = {
	.name		= "Linux user mode ISO test driver",
	.autoconf	= 1,
	.iso		= 1,
	.alt		= -1,
};

#ifdef IBOT2

static struct usbtest_info ibot2_info = {
	.name		= "iBOT2 webcam",
	.ep_in		= 2,
	.alt		= -1,
};
#endif

#ifdef GENERIC

static struct usbtest_info generic_info = {
	.name		= "Generic USB device",
	.alt		= -1,
};
#endif

static struct usb_device_id id_table [] = {

	

	
#ifdef CONFIG_SYNO_PLX_PORTING
	
	{ USB_DEVICE (0x0547, 0x2131),
		.driver_info = (unsigned long) &ez1_info,
		},
#endif

	
	{ USB_DEVICE (0x0547, 0x2235),
		.driver_info = (unsigned long) &ez1_info,
		},

	
	{ USB_DEVICE (0x0547, 0x0080),
		.driver_info = (unsigned long) &ez1_info,
		},

	
	{ USB_DEVICE (0x04b4, 0x8613),
		.driver_info = (unsigned long) &ez2_info,
		},

	
	{ USB_DEVICE (0xfff0, 0xfff0),
		.driver_info = (unsigned long) &fw_info,
		},

	
	{ USB_DEVICE (0x0525, 0xa4a0),
		.driver_info = (unsigned long) &gz_info,
		},

	
	{ USB_DEVICE (0x0525, 0xa4a4),
		.driver_info = (unsigned long) &um_info,
		},

	
	{ USB_DEVICE (0x0525, 0xa4a3),
		.driver_info = (unsigned long) &um2_info,
		},

#ifdef KEYSPAN_19Qi
	
	
	{ USB_DEVICE (0x06cd, 0x010b),
		.driver_info = (unsigned long) &ez1_info,
		},
#endif

	

#ifdef IBOT2
	
	
	{ USB_DEVICE (0x0b62, 0x0059),
		.driver_info = (unsigned long) &ibot2_info,
		},
#endif

	

#ifdef GENERIC
	
	{ .driver_info = (unsigned long) &generic_info, },
#endif

	

	{ }
};
MODULE_DEVICE_TABLE (usb, id_table);

static struct usb_driver usbtest_driver = {
	.name =		"usbtest",
	.id_table =	id_table,
	.probe =	usbtest_probe,
	.ioctl =	usbtest_ioctl,
	.disconnect =	usbtest_disconnect,
	.suspend =	usbtest_suspend,
	.resume =	usbtest_resume,
};



static int __init usbtest_init (void)
{
#ifdef GENERIC
	if (vendor)
		pr_debug("params: vend=0x%04x prod=0x%04x\n", vendor, product);
#endif
	return usb_register (&usbtest_driver);
}
module_init (usbtest_init);

static void __exit usbtest_exit (void)
{
	usb_deregister (&usbtest_driver);
}
module_exit (usbtest_exit);

MODULE_DESCRIPTION ("USB Core/HCD Testing Driver");
MODULE_LICENSE ("GPL");

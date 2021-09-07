#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "usbip_common.h"
#include "stub.h"
#include "../../usb/core/hcd.h"

static int is_clear_halt_cmd(struct urb *urb)
{
	struct usb_ctrlrequest *req;

	req = (struct usb_ctrlrequest *) urb->setup_packet;

	 return (req->bRequest == USB_REQ_CLEAR_FEATURE) &&
		 (req->bRequestType == USB_RECIP_ENDPOINT) &&
		 (req->wValue == USB_ENDPOINT_HALT);
}

static int is_set_interface_cmd(struct urb *urb)
{
	struct usb_ctrlrequest *req;

	req = (struct usb_ctrlrequest *) urb->setup_packet;

	return (req->bRequest == USB_REQ_SET_INTERFACE) &&
		   (req->bRequestType == USB_RECIP_INTERFACE);
}

static int is_set_configuration_cmd(struct urb *urb)
{
	struct usb_ctrlrequest *req;

	req = (struct usb_ctrlrequest *) urb->setup_packet;

	return (req->bRequest == USB_REQ_SET_CONFIGURATION) &&
		   (req->bRequestType == USB_RECIP_DEVICE);
}

static int is_reset_device_cmd(struct urb *urb)
{
	struct usb_ctrlrequest *req;
	__u16 value;
	__u16 index;

	req = (struct usb_ctrlrequest *) urb->setup_packet;
	value = le16_to_cpu(req->wValue);
	index = le16_to_cpu(req->wIndex);

	if ((req->bRequest == USB_REQ_SET_FEATURE) &&
			(req->bRequestType == USB_RT_PORT) &&
			(value == USB_PORT_FEAT_RESET)) {
		usbip_dbg_stub_rx("reset_device_cmd, port %u\n", index);
		return 1;
	} else
		return 0;
}

static int tweak_clear_halt_cmd(struct urb *urb)
{
	struct usb_ctrlrequest *req;
	int target_endp;
	int target_dir;
	int target_pipe;
	int ret;

	req = (struct usb_ctrlrequest *) urb->setup_packet;

	target_endp = le16_to_cpu(req->wIndex) & 0x000f;

	target_dir = le16_to_cpu(req->wIndex) & 0x0080;

	if (target_dir)
		target_pipe = usb_rcvctrlpipe(urb->dev, target_endp);
	else
		target_pipe = usb_sndctrlpipe(urb->dev, target_endp);

	ret = usb_clear_halt(urb->dev, target_pipe);
	if (ret < 0)
		usbip_uinfo("clear_halt error: devnum %d endp %d, %d\n",
					urb->dev->devnum, target_endp, ret);
	else
		usbip_uinfo("clear_halt done: devnum %d endp %d\n",
					urb->dev->devnum, target_endp);

	return ret;
}

static int tweak_set_interface_cmd(struct urb *urb)
{
	struct usb_ctrlrequest *req;
	__u16 alternate;
	__u16 interface;
	int ret;

	req = (struct usb_ctrlrequest *) urb->setup_packet;
	alternate = le16_to_cpu(req->wValue);
	interface = le16_to_cpu(req->wIndex);

	usbip_dbg_stub_rx("set_interface: inf %u alt %u\n", interface,
								alternate);

	ret = usb_set_interface(urb->dev, interface, alternate);
	if (ret < 0)
		usbip_uinfo("set_interface error: inf %u alt %u, %d\n",
					interface, alternate, ret);
	else
		usbip_uinfo("set_interface done: inf %u alt %u\n",
							interface,
							alternate);

	return ret;
}

static int tweak_set_configuration_cmd(struct urb *urb)
{
	struct usb_ctrlrequest *req;
	__u16 config;

	req = (struct usb_ctrlrequest *) urb->setup_packet;
	config = le16_to_cpu(req->wValue);

	usbip_uinfo("set_configuration (%d) to %s\n", config,
						dev_name(&urb->dev->dev));
	usbip_uinfo("but, skip!\n");

	return 0;
	 
}

static int tweak_reset_device_cmd(struct urb *urb)
{
	struct usb_ctrlrequest *req;
	__u16 value;
	__u16 index;
	int ret;

	req = (struct usb_ctrlrequest *) urb->setup_packet;
	value = le16_to_cpu(req->wValue);
	index = le16_to_cpu(req->wIndex);

	usbip_uinfo("reset_device (port %d) to %s\n", index,
						dev_name(&urb->dev->dev));

	ret = usb_lock_device_for_reset(urb->dev, NULL);
	if (ret < 0) {
		dev_err(&urb->dev->dev, "lock for reset\n");
		return ret;
	}

	ret = usb_reset_device(urb->dev);
	if (ret < 0)
		dev_err(&urb->dev->dev, "device reset\n");

	usb_unlock_device(urb->dev);

	return ret;
}

static void tweak_special_requests(struct urb *urb)
{	
	if (!urb || !urb->setup_packet)
		return;
	
	if (usb_pipetype(urb->pipe) != PIPE_CONTROL)
		return;	

	if (is_clear_halt_cmd(urb)) 
		 
		 tweak_clear_halt_cmd(urb);		 

	else if (is_set_interface_cmd(urb)) 
		 
		tweak_set_interface_cmd(urb);		

	else if (is_set_configuration_cmd(urb))
		 
		tweak_set_configuration_cmd(urb);		

	else if (is_reset_device_cmd(urb)) 
		tweak_reset_device_cmd(urb);		
	else 
		usbip_dbg_stub_rx("no need to tweak\n"); 			
}

static int stub_recv_cmd_unlink(struct stub_device *sdev,
						struct usbip_header *pdu)
{
	unsigned long flags;

	struct stub_priv *priv;

	spin_lock_irqsave(&sdev->priv_lock, flags);

	list_for_each_entry(priv, &sdev->priv_init, list) {
		if (priv->seqnum == pdu->u.cmd_unlink.seqnum) {
			int ret;

			dev_info(&priv->urb->dev->dev, "unlink urb %p\n",
				 priv->urb);

			priv->unlinking = 1;

			priv->seqnum = pdu->base.seqnum;

			spin_unlock_irqrestore(&sdev->priv_lock, flags);

			ret = usb_unlink_urb(priv->urb);
			if (ret != -EINPROGRESS)
				dev_err(&priv->urb->dev->dev,
					"failed to unlink a urb %p, ret %d\n",
					priv->urb, ret);
			return 0;
		}
	}

	usbip_dbg_stub_rx("seqnum %d is not pending\n",
						pdu->u.cmd_unlink.seqnum);

	stub_enqueue_ret_unlink(sdev, pdu->base.seqnum, 0);

	spin_unlock_irqrestore(&sdev->priv_lock, flags);

	return 0;
}

static int valid_request(struct stub_device *sdev, struct usbip_header *pdu)
{
	struct usbip_device *ud = &sdev->ud;

	if (pdu->base.devid == sdev->devid) {
		spin_lock(&ud->lock);
		if (ud->status == SDEV_ST_USED) {
			 
			spin_unlock(&ud->lock);
			return 1;
		}
		spin_unlock(&ud->lock);
	}

	return 0;
}

static struct stub_priv *stub_priv_alloc(struct stub_device *sdev,
					 struct usbip_header *pdu)
{
	struct stub_priv *priv;
	struct usbip_device *ud = &sdev->ud;
	unsigned long flags;

	spin_lock_irqsave(&sdev->priv_lock, flags);

	priv = kmem_cache_zalloc(stub_priv_cache, GFP_ATOMIC);
	if (!priv) {
		dev_err(&sdev->interface->dev, "alloc stub_priv\n");
		spin_unlock_irqrestore(&sdev->priv_lock, flags);
		usbip_event_add(ud, SDEV_EVENT_ERROR_MALLOC);
		return NULL;
	}

	priv->seqnum = pdu->base.seqnum;
	priv->sdev = sdev;

	list_add_tail(&priv->list, &sdev->priv_init);

	spin_unlock_irqrestore(&sdev->priv_lock, flags);

	return priv;
}

static struct usb_host_endpoint *get_ep_from_epnum(struct usb_device *udev,
		int epnum0)
{
	struct usb_host_config *config;
	int i = 0, j = 0;
	struct usb_host_endpoint *ep = NULL;
	int epnum;
	int found = 0;

	if (epnum0 == 0)
		return &udev->ep0;

	config = udev->actconfig;
	if (!config)
		return NULL;

	for (i = 0; i < config->desc.bNumInterfaces; i++) {		
		struct usb_host_interface *setting;

		setting = config->interface[i]->cur_altsetting;

		for (j = 0; j < setting->desc.bNumEndpoints; j++) {
			ep = &setting->endpoint[j];
			epnum = (ep->desc.bEndpointAddress & 0x7f);			

			if (epnum == epnum0) {
                 
				printk("found ep[%d]=epnum %d\n", j, epnum0);				
				found = 1;
				break;
			}
		}
	}

	if (found)
		return ep;
	else
		return NULL;
}

static int get_pipe(struct stub_device *sdev, int epnum, int dir)
{
	struct usb_device *udev = interface_to_usbdev(sdev->interface);
	struct usb_host_endpoint *ep;
	struct usb_endpoint_descriptor *epd = NULL;
#ifdef MY_ABC_HERE
	if (dir == USBIP_DIR_IN)
		ep = udev->ep_in[epnum & 0x7f];
	else
		ep = udev->ep_out[epnum & 0x7f];
#else
	ep = get_ep_from_epnum(udev, epnum);    
#endif

	if (!ep) {
		dev_err(&sdev->interface->dev, "no such endpoint?, %d\n",
			epnum);
		BUG();
	}

	epd = &ep->desc;

#if 0
	 
	if (epnum == 0) {
		if (dir == USBIP_DIR_OUT)
			return usb_sndctrlpipe(udev, 0);
		else
			return usb_rcvctrlpipe(udev, 0);
	}
#endif

	if (usb_endpoint_xfer_control(epd)) {		
		if (dir == USBIP_DIR_OUT)
			return usb_sndctrlpipe(udev, epnum);
		else
			return usb_rcvctrlpipe(udev, epnum);
	}

	if (usb_endpoint_xfer_bulk(epd)) {		
		if (dir == USBIP_DIR_OUT)
			return usb_sndbulkpipe(udev, epnum);
		else
			return usb_rcvbulkpipe(udev, epnum);
	}

	if (usb_endpoint_xfer_int(epd)) {		
		if (dir == USBIP_DIR_OUT)
			return usb_sndintpipe(udev, epnum);
		else
			return usb_rcvintpipe(udev, epnum);
	}

	if (usb_endpoint_xfer_isoc(epd)) {		
		if (dir == USBIP_DIR_OUT)
			return usb_sndisocpipe(udev, epnum);
		else
			return usb_rcvisocpipe(udev, epnum);
	}
	
	dev_err(&sdev->interface->dev, "get pipe, epnum %d\n", epnum);
	return 0;
}

#ifdef MY_ABC_HERE
static void masking_bogus_flags(struct urb *urb)
{
       int                             xfertype;
       struct usb_device               *dev;
       struct usb_host_endpoint        *ep;
       int                             is_out;
       unsigned int    allowed;

       if (!urb || urb->hcpriv || !urb->complete)
               return;
       dev = urb->dev;
       if ((!dev) || (dev->state < USB_STATE_UNAUTHENTICATED))
               return;

       ep = (usb_pipein(urb->pipe) ? dev->ep_in : dev->ep_out)
                       [usb_pipeendpoint(urb->pipe)];
       if (!ep)
               return;

       xfertype = usb_endpoint_type(&ep->desc);
       if (xfertype == USB_ENDPOINT_XFER_CONTROL) {
               struct usb_ctrlrequest *setup =
                               (struct usb_ctrlrequest *) urb->setup_packet;

               if (!setup)
                       return;
               is_out = !(setup->bRequestType & USB_DIR_IN) ||
                               !setup->wLength;
       } else {
               is_out = usb_endpoint_dir_out(&ep->desc);
       }
        
       allowed = (URB_NO_TRANSFER_DMA_MAP | URB_NO_INTERRUPT |
                  URB_DIR_MASK | URB_FREE_BUFFER);
       switch (xfertype) {
       case USB_ENDPOINT_XFER_BULK:
               if (is_out) {
	 				usbip_udbg("## USB_ENDPOINT_XFER_BULK: adding allow URB_ZERO_PACKET\n");
                       allowed |= URB_ZERO_PACKET;
			   }
                
	   case USB_ENDPOINT_XFER_CONTROL:
				usbip_udbg("## USB_ENDPOINT_XFER_CONTROL: adding allow URB_NO_FSBR\n");
               allowed |= URB_NO_FSBR;  
                
       default:                         
               if (!is_out) {
				   usbip_udbg("## default: adding allow URB_SHORT_NOT_OK\n");
                   allowed |= URB_SHORT_NOT_OK;
			   }
               break;
	   case USB_ENDPOINT_XFER_ISOC:
				usbip_udbg("## USB_ENDPOINT_XFER_ISOC: adding allow URB_ISO_ASAP\n");
               allowed |= URB_ISO_ASAP;
               break;
       }
       urb->transfer_flags &= allowed;
}
#endif

static void stub_recv_cmd_submit(struct stub_device *sdev,
				 struct usbip_header *pdu)
{
	int ret;
	struct stub_priv *priv;
	struct usbip_device *ud = &sdev->ud;
	struct usb_device *udev = interface_to_usbdev(sdev->interface);
	int pipe = get_pipe(sdev, pdu->base.ep, pdu->base.direction);		
	
	priv = stub_priv_alloc(sdev, pdu);
	if (!priv)
		return;

	if (usb_pipeisoc(pipe))
		priv->urb = usb_alloc_urb(pdu->u.cmd_submit.number_of_packets,
								GFP_KERNEL);
	else
		priv->urb = usb_alloc_urb(0, GFP_KERNEL);

	if (!priv->urb) {
		dev_err(&sdev->interface->dev, "malloc urb\n");
		usbip_event_add(ud, SDEV_EVENT_ERROR_MALLOC);
		return;
	}

	if (pdu->u.cmd_submit.transfer_buffer_length > 0) {
		priv->urb->transfer_buffer =
			kzalloc(pdu->u.cmd_submit.transfer_buffer_length,
								GFP_KERNEL);
		if (!priv->urb->transfer_buffer) {
			dev_err(&sdev->interface->dev, "malloc x_buff\n");
			usbip_event_add(ud, SDEV_EVENT_ERROR_MALLOC);
			return;
		}
	}

	priv->urb->setup_packet = kzalloc(8, GFP_KERNEL);
	if (!priv->urb->setup_packet) {
		dev_err(&sdev->interface->dev, "allocate setup_packet\n");
		usbip_event_add(ud, SDEV_EVENT_ERROR_MALLOC);
		return;
	}
	memcpy(priv->urb->setup_packet, &pdu->u.cmd_submit.setup, 8);

	priv->urb->context                = (void *) priv;
	priv->urb->dev                    = udev;
	priv->urb->pipe                   = pipe;
	priv->urb->complete               = stub_complete;

	usbip_pack_pdu(pdu, priv->urb, USBIP_CMD_SUBMIT, 0);

	if (usbip_recv_xbuff(ud, priv->urb) < 0)
		return;

	if (usbip_recv_iso(ud, priv->urb) < 0)
		return;

	tweak_special_requests(priv->urb);

#ifdef MY_ABC_HERE
	masking_bogus_flags(priv->urb);	
#endif
	 
	ret = usb_submit_urb(priv->urb, GFP_KERNEL);			

	if (ret == 0)
		usbip_dbg_stub_rx("submit urb ok, seqnum %u\n",
							pdu->base.seqnum);
	else {		
		dev_err(&sdev->interface->dev, "submit_urb error, %d\n", ret);
		usbip_dump_header(pdu);
		usbip_dump_urb(priv->urb);

#ifndef MY_ABC_HERE
		 
		usbip_event_add(ud, SDEV_EVENT_ERROR_SUBMIT);
#endif
	}

	usbip_dbg_stub_rx("Leave\n");
	return;
}

static void stub_rx_pdu(struct usbip_device *ud)
{
	int ret;
	struct usbip_header pdu;
	struct stub_device *sdev = container_of(ud, struct stub_device, ud);
	struct device *dev = &sdev->interface->dev;

	usbip_dbg_stub_rx("Enter\n");

	memset(&pdu, 0, sizeof(pdu));

	ret = usbip_xmit(0, ud->tcp_socket, (char *) &pdu, sizeof(pdu), 0);
	if (ret != sizeof(pdu)) {
		dev_err(dev, "recv a header, %d\n", ret);
		usbip_event_add(ud, SDEV_EVENT_ERROR_TCP);
		return;
	}

#ifdef MY_ABC_HERE	
	ud->get_socket_time = current_kernel_time();	
#endif	

	usbip_header_correct_endian(&pdu, 0);

	if (usbip_dbg_flag_stub_rx)
		usbip_dump_header(&pdu);

	if (!valid_request(sdev, &pdu)) {
		dev_err(dev, "recv invalid request\n");
		usbip_event_add(ud, SDEV_EVENT_ERROR_TCP);
		return;
	}

	switch (pdu.base.command) {
	case USBIP_CMD_UNLINK:		
		stub_recv_cmd_unlink(sdev, &pdu);
		break;

	case USBIP_CMD_SUBMIT:
#ifdef MY_ABC_HERE	
	case USBIP_RESET_DEV:				
		if(pdu.base.command == USBIP_RESET_DEV) {
			printk("reset device\n");
		}
#endif
		stub_recv_cmd_submit(sdev, &pdu);
		break;

	default:
		 
		dev_err(dev, "unknown pdu\n");
		usbip_event_add(ud, SDEV_EVENT_ERROR_TCP);
		return;
	}

}

#ifdef MY_ABC_HERE
int syno_socket_check(struct usbip_device *ud)
{
	if (-1 == ud->sockfd || NULL == ud->tcp_socket) {
		err("syno_socket_check() stop");
		return 0;
	}
	return 1;
}
#endif

void stub_rx_loop(struct usbip_task *ut)
{
	struct usbip_device *ud = container_of(ut, struct usbip_device, tcp_rx);
#ifdef MY_ABC_HERE
	struct stub_device *sdev = container_of(ud, struct stub_device, ud);
#endif

	while (1) {
		if (signal_pending(current)) {
			usbip_dbg_stub_rx("signal caught!\n");
			break;
		}

#ifdef MY_ABC_HERE
		if (syno_usbip_event_happened(ud))
#else
		if (usbip_event_happened(ud))
#endif
			break;
#ifdef MY_ABC_HERE
		wait_event_interruptible(sdev->rx_waitq, syno_socket_check(ud));
#endif
		stub_rx_pdu(ud);
	}
}

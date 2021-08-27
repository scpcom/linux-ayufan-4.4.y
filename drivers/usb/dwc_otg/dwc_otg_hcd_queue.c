#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif

#ifndef DWC_DEVICE_ONLY

#include "dwc_otg_hcd.h"
#include "dwc_otg_regs.h"

void dwc_otg_hcd_qh_free(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh)
{
	dwc_otg_qtd_t *qtd, *qtd_tmp;
#if defined(MY_ABC_HERE)
	dwc_irqflags_t flags;
#endif

#if !defined(MY_ABC_HERE)
	DWC_SPINLOCK(hcd->lock);
#endif
	DWC_CIRCLEQ_FOREACH_SAFE(qtd, qtd_tmp, &qh->qtd_list, qtd_list_entry) {
		DWC_CIRCLEQ_REMOVE(&qh->qtd_list, qtd, qtd_list_entry);
		dwc_otg_hcd_qtd_free(qtd);
	}

#if defined(MY_ABC_HERE)
	DWC_SPINLOCK_IRQSAVE(hcd->lock, &flags);
#endif
	if (hcd->core_if->dma_desc_enable) {
		dwc_otg_hcd_qh_free_ddma(hcd, qh);
#if defined(MY_ABC_HERE)
		DWC_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
#endif
	} else if (qh->dw_align_buf) {
#if defined(MY_ABC_HERE)
		DWC_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
#endif
		uint32_t buf_size;
		if (qh->ep_type == UE_ISOCHRONOUS) {
			buf_size = 4096;
		} else {
#if defined(MY_ABC_HERE)
			DWC_SPINLOCK_IRQSAVE(hcd->lock, &flags);
#endif
			buf_size = hcd->core_if->core_params->max_transfer_size;
#if defined(MY_ABC_HERE)
			DWC_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
#endif
		}
		DWC_DMA_FREE(buf_size, qh->dw_align_buf, qh->dw_align_buf_dma);
	}
#if defined(MY_ABC_HERE)
	else {
		DWC_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);
	}
#endif

	DWC_FREE(qh);
#if !defined(MY_ABC_HERE)
	DWC_SPINUNLOCK(hcd->lock);
#endif
	return;
}

#define BitStuffTime(bytecount)  ((8 * 7* bytecount) / 6)
#define HS_HOST_DELAY		5	 
#define FS_LS_HOST_DELAY	1000	 
#define HUB_LS_SETUP		333	 
#define NS_TO_US(ns)		((ns + 500) / 1000)
				 
static uint32_t calc_bus_time(int speed, int is_in, int is_isoc, int bytecount)
{
	unsigned long retval;

	switch (speed) {
	case USB_SPEED_HIGH:
		if (is_isoc) {
			retval =
			    ((38 * 8 * 2083) +
			     (2083 * (3 + BitStuffTime(bytecount)))) / 1000 +
			    HS_HOST_DELAY;
		} else {
			retval =
			    ((55 * 8 * 2083) +
			     (2083 * (3 + BitStuffTime(bytecount)))) / 1000 +
			    HS_HOST_DELAY;
		}
		break;
	case USB_SPEED_FULL:
		if (is_isoc) {
			retval =
			    (8354 * (31 + 10 * BitStuffTime(bytecount))) / 1000;
			if (is_in) {
				retval = 7268 + FS_LS_HOST_DELAY + retval;
			} else {
				retval = 6265 + FS_LS_HOST_DELAY + retval;
			}
		} else {
			retval =
			    (8354 * (31 + 10 * BitStuffTime(bytecount))) / 1000;
			retval = 9107 + FS_LS_HOST_DELAY + retval;
		}
		break;
	case USB_SPEED_LOW:
		if (is_in) {
			retval =
			    (67667 * (31 + 10 * BitStuffTime(bytecount))) /
			    1000;
			retval =
			    64060 + (2 * HUB_LS_SETUP) + FS_LS_HOST_DELAY +
			    retval;
		} else {
			retval =
			    (66700 * (31 + 10 * BitStuffTime(bytecount))) /
			    1000;
			retval =
			    64107 + (2 * HUB_LS_SETUP) + FS_LS_HOST_DELAY +
			    retval;
		}
		break;
	default:
		DWC_WARN("Unknown device speed\n");
		retval = -1;
	}

	return NS_TO_US(retval);
}

#define SCHEDULE_SLOP 10
void qh_init(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh, dwc_otg_hcd_urb_t * urb)
{
	char *speed, *type;
	int dev_speed;
	uint32_t hub_addr, hub_port;

	dwc_memset(qh, 0, sizeof(dwc_otg_qh_t));

	qh->ep_type = dwc_otg_hcd_get_pipe_type(&urb->pipe_info);
	qh->ep_is_in = dwc_otg_hcd_is_pipe_in(&urb->pipe_info) ? 1 : 0;

	qh->data_toggle = DWC_OTG_HC_PID_DATA0;
	qh->maxp = dwc_otg_hcd_get_mps(&urb->pipe_info);
	DWC_CIRCLEQ_INIT(&qh->qtd_list);
	DWC_LIST_INIT(&qh->qh_list_entry);
	qh->channel = NULL;

	dev_speed = hcd->fops->speed(hcd, urb->priv);

	hcd->fops->hub_info(hcd, urb->priv, &hub_addr, &hub_port);
	qh->do_split = 0;

	if (((dev_speed == USB_SPEED_LOW) ||
	     (dev_speed == USB_SPEED_FULL)) &&
	    (hub_addr != 0 && hub_addr != 1)) {
		DWC_DEBUGPL(DBG_HCD,
			    "QH init: EP %d: TT found at hub addr %d, for port %d\n",
			    dwc_otg_hcd_get_ep_num(&urb->pipe_info), hub_addr,
			    hub_port);
		qh->do_split = 1;
	}

	if (qh->ep_type == UE_INTERRUPT || qh->ep_type == UE_ISOCHRONOUS) {
		 
		hprt0_data_t hprt;

		int bytecount =
		    dwc_hb_mult(qh->maxp) * dwc_max_packet(qh->maxp);

		qh->usecs =
		    calc_bus_time((qh->do_split ? USB_SPEED_HIGH : dev_speed),
				  qh->ep_is_in, (qh->ep_type == UE_ISOCHRONOUS),
				  bytecount);
		 
#if defined(MY_ABC_HERE)
		hcd->frame_number = dwc_otg_hcd_get_frame_number(hcd);
#endif
		qh->sched_frame = dwc_frame_num_inc(hcd->frame_number,
						    SCHEDULE_SLOP);
		qh->interval = urb->interval;

#if 0
		 
		if (qh->ep_type == UE_INTERRUPT) {
			qh->interval = 8;
		}
#endif
		hprt.d32 = DWC_READ_REG32(hcd->core_if->host_if->hprt0);
		if ((hprt.b.prtspd == DWC_HPRT0_PRTSPD_HIGH_SPEED) &&
		    ((dev_speed == USB_SPEED_LOW) ||
		     (dev_speed == USB_SPEED_FULL))) {
			qh->interval *= 8;
			qh->sched_frame |= 0x7;
#if defined(MY_ABC_HERE)
			if ((qh->ep_type == UE_INTERRUPT) && qh->ep_is_in && qh->do_split) {
				qh->sched_frame = dwc_frame_num_inc(qh->sched_frame, 2);
			}
#endif
			qh->start_split_frame = qh->sched_frame;
		}

	}

	DWC_DEBUGPL(DBG_HCD, "DWC OTG HCD QH Initialized\n");
	DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD QH  - qh = %p\n", qh);
	DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD QH  - Device Address = %d\n",
		    dwc_otg_hcd_get_dev_addr(&urb->pipe_info));
	DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD QH  - Endpoint %d, %s\n",
		    dwc_otg_hcd_get_ep_num(&urb->pipe_info),
		    dwc_otg_hcd_is_pipe_in(&urb->pipe_info) ? "IN" : "OUT");
	switch (dev_speed) {
	case USB_SPEED_LOW:
		qh->dev_speed = DWC_OTG_EP_SPEED_LOW;
		speed = "low";
		break;
	case USB_SPEED_FULL:
		qh->dev_speed = DWC_OTG_EP_SPEED_FULL;
		speed = "full";
		break;
	case USB_SPEED_HIGH:
		qh->dev_speed = DWC_OTG_EP_SPEED_HIGH;
		speed = "high";
		break;
	default:
		speed = "?";
		break;
	}
	DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD QH  - Speed = %s\n", speed);

	switch (qh->ep_type) {
	case UE_ISOCHRONOUS:
		type = "isochronous";
		break;
	case UE_INTERRUPT:
		type = "interrupt";
		break;
	case UE_CONTROL:
		type = "control";
		break;
	case UE_BULK:
		type = "bulk";
		break;
	default:
		type = "?";
		break;
	}

	DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD QH  - Type = %s\n", type);

#ifdef DEBUG
	if (qh->ep_type == UE_INTERRUPT) {
		DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD QH - usecs = %d\n",
			    qh->usecs);
		DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD QH - interval = %d\n",
			    qh->interval);
	}
#endif

}

dwc_otg_qh_t *dwc_otg_hcd_qh_create(dwc_otg_hcd_t * hcd,
				    dwc_otg_hcd_urb_t * urb, int atomic_alloc)
{
	dwc_otg_qh_t *qh;

	qh = dwc_otg_hcd_qh_alloc(atomic_alloc);
	if (qh == NULL) {
		DWC_ERROR("qh allocation failed");
		return NULL;
	}

	qh_init(hcd, qh, urb);

	if (hcd->core_if->dma_desc_enable
	    && (dwc_otg_hcd_qh_init_ddma(hcd, qh) < 0)) {
		dwc_otg_hcd_qh_free(hcd, qh);
		return NULL;
	}

	return qh;
}

static int periodic_channel_available(dwc_otg_hcd_t * hcd)
{
	 
	int status;
	int num_channels;

	num_channels = hcd->core_if->core_params->host_channels;
	if ((hcd->periodic_channels + hcd->non_periodic_channels < num_channels)
	    && (hcd->periodic_channels < num_channels - 1)) {
		status = 0;
	} else {
		DWC_INFO("%s: Total channels: %d, Periodic: %d, Non-periodic: %d\n",
			__func__, num_channels, hcd->periodic_channels, hcd->non_periodic_channels);	 
		status = -DWC_E_NO_SPACE;
	}

	return status;
}

static int check_periodic_bandwidth(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh)
{
	int status;
	int16_t max_claimed_usecs;

	status = 0;

	if ((qh->dev_speed == DWC_OTG_EP_SPEED_HIGH) || qh->do_split) {
		 
		max_claimed_usecs = 100 - qh->usecs;
	} else {
		 
		max_claimed_usecs = 900 - qh->usecs;
	}

	if (hcd->periodic_usecs > max_claimed_usecs) {
		DWC_INFO("%s: already claimed usecs %d, required usecs %d\n", __func__, hcd->periodic_usecs, qh->usecs);	 
		status = -DWC_E_NO_SPACE;
	}

	return status;
}

static int check_max_xfer_size(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh)
{
	int status;
	uint32_t max_xfer_size;
	uint32_t max_channel_xfer_size;

	status = 0;

	max_xfer_size = dwc_max_packet(qh->maxp) * dwc_hb_mult(qh->maxp);
	max_channel_xfer_size = hcd->core_if->core_params->max_transfer_size;

	if (max_xfer_size > max_channel_xfer_size) {
		DWC_INFO("%s: Periodic xfer length %d > " "max xfer length for channel %d\n",
				__func__, max_xfer_size, max_channel_xfer_size);	 
		status = -DWC_E_NO_SPACE;
	}

	return status;
}

static int schedule_periodic(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh)
{
	int status = 0;

	status = periodic_channel_available(hcd);
	if (status) {
		DWC_INFO("%s: No host channel available for periodic " "transfer.\n", __func__);	 
		return status;
	}

	status = check_periodic_bandwidth(hcd, qh);
	if (status) {
		DWC_INFO("%s: Insufficient periodic bandwidth for " "periodic transfer.\n", __func__);	 
		return status;
	}

	status = check_max_xfer_size(hcd, qh);
	if (status) {
		DWC_INFO("%s: Channel max transfer size too small " "for periodic transfer.\n", __func__);	 
		return status;
	}

	if (hcd->core_if->dma_desc_enable) {
		 
		DWC_LIST_INSERT_TAIL(&hcd->periodic_sched_ready, &qh->qh_list_entry);
	}
	else {
	 
	DWC_LIST_INSERT_TAIL(&hcd->periodic_sched_inactive, &qh->qh_list_entry);
	}

	hcd->periodic_channels++;

	hcd->periodic_usecs += qh->usecs;

	return status;
}

int dwc_otg_hcd_qh_add(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh)
{
	int status = 0;
	gintmsk_data_t intr_mask = {.d32 = 0 };

	if (!DWC_LIST_EMPTY(&qh->qh_list_entry)) {
		 
		return status;
	}

	if (dwc_qh_is_non_per(qh)) {
		 
		DWC_LIST_INSERT_TAIL(&hcd->non_periodic_sched_inactive,
				     &qh->qh_list_entry);
	} else {
		status = schedule_periodic(hcd, qh);
		if ( !hcd->periodic_qh_count ) {
			intr_mask.b.sofintr = 1;
			DWC_MODIFY_REG32(&hcd->core_if->core_global_regs->gintmsk,
								intr_mask.d32, intr_mask.d32);
		}
		hcd->periodic_qh_count++;
	}

	return status;
}

static void deschedule_periodic(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh)
{
	DWC_LIST_REMOVE_INIT(&qh->qh_list_entry);

	hcd->periodic_channels--;

	hcd->periodic_usecs -= qh->usecs;
}

void dwc_otg_hcd_qh_remove(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh)
{
	gintmsk_data_t intr_mask = {.d32 = 0 };

	if (DWC_LIST_EMPTY(&qh->qh_list_entry)) {
		 
		return;
	}

	if (dwc_qh_is_non_per(qh)) {
		if (hcd->non_periodic_qh_ptr == &qh->qh_list_entry) {
			hcd->non_periodic_qh_ptr =
			    hcd->non_periodic_qh_ptr->next;
		}
		DWC_LIST_REMOVE_INIT(&qh->qh_list_entry);
	} else {
		deschedule_periodic(hcd, qh);
		hcd->periodic_qh_count--;
		if( !hcd->periodic_qh_count ) {
			intr_mask.b.sofintr = 1;
				DWC_MODIFY_REG32(&hcd->core_if->core_global_regs->gintmsk,
									intr_mask.d32, 0);
		}
	}
}

void dwc_otg_hcd_qh_deactivate(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh,
			       int sched_next_periodic_split)
{	
	if (dwc_qh_is_non_per(qh)) {
		dwc_otg_hcd_qh_remove(hcd, qh);
		if (!DWC_CIRCLEQ_EMPTY(&qh->qtd_list)) {
			 
			dwc_otg_hcd_qh_add(hcd, qh);
		}
	} else {
		uint16_t frame_number = dwc_otg_hcd_get_frame_number(hcd);

		if (qh->do_split) {
			 
			if (sched_next_periodic_split) {

				qh->sched_frame = frame_number;
				if (dwc_frame_num_le(frame_number,
						     dwc_frame_num_inc
						     (qh->start_split_frame,
						      1))) {
					 
					if ((qh->ep_type != UE_ISOCHRONOUS) ||
					    (qh->ep_is_in != 0)) {
						qh->sched_frame =
						    dwc_frame_num_inc(qh->sched_frame, 1);
					}
				}
			} else {
				qh->sched_frame =
				    dwc_frame_num_inc(qh->start_split_frame,
						      qh->interval);
				if (dwc_frame_num_le
				    (qh->sched_frame, frame_number)) {
					qh->sched_frame = frame_number;
				}
				qh->sched_frame |= 0x7;
#if defined(MY_ABC_HERE)
				if ((qh->ep_type == UE_INTERRUPT) && qh->ep_is_in && qh->do_split) {
					qh->sched_frame = dwc_frame_num_inc(qh->sched_frame, 2);
				}
#endif
				qh->start_split_frame = qh->sched_frame;
			}
		} else {
			qh->sched_frame =
			    dwc_frame_num_inc(qh->sched_frame, qh->interval);
			if (dwc_frame_num_le(qh->sched_frame, frame_number)) {
				qh->sched_frame = frame_number;
			}
		}

		if (DWC_CIRCLEQ_EMPTY(&qh->qtd_list)) {
			dwc_otg_hcd_qh_remove(hcd, qh);
		} else {
			 
			if (qh->sched_frame == frame_number) {
				DWC_LIST_MOVE_HEAD(&hcd->periodic_sched_ready,
						   &qh->qh_list_entry);
			} else {
				DWC_LIST_MOVE_HEAD
				    (&hcd->periodic_sched_inactive,
				     &qh->qh_list_entry);
			}
		}
	}
}

dwc_otg_qtd_t *dwc_otg_hcd_qtd_create(dwc_otg_hcd_urb_t * urb, int atomic_alloc)
{
	dwc_otg_qtd_t *qtd;

	qtd = dwc_otg_hcd_qtd_alloc(atomic_alloc);
	if (qtd == NULL) {
		return NULL;
	}

	dwc_otg_hcd_qtd_init(qtd, urb);
	return qtd;
}

void dwc_otg_hcd_qtd_init(dwc_otg_qtd_t * qtd, dwc_otg_hcd_urb_t * urb)
{
	dwc_memset(qtd, 0, sizeof(dwc_otg_qtd_t));
	qtd->urb = urb;
	if (dwc_otg_hcd_get_pipe_type(&urb->pipe_info) == UE_CONTROL) {
		 
		qtd->data_toggle = DWC_OTG_HC_PID_DATA1;
		qtd->control_phase = DWC_OTG_CONTROL_SETUP;
	}

	qtd->complete_split = 0;
	qtd->isoc_split_pos = DWC_HCSPLIT_XACTPOS_ALL;
	qtd->isoc_split_offset = 0;
	qtd->in_process = 0;

	urb->qtd = qtd;
	return;
}

int dwc_otg_hcd_qtd_add(dwc_otg_qtd_t * qtd,
			dwc_otg_hcd_t * hcd, dwc_otg_qh_t ** qh, int atomic_alloc)
{
	int retval = 0;
	dwc_irqflags_t flags;

	dwc_otg_hcd_urb_t *urb = qtd->urb;

	if (*qh == NULL) {
		*qh = dwc_otg_hcd_qh_create(hcd, urb, atomic_alloc);
		if (*qh == NULL) {
			retval = -1;
			goto done;
		}
	}
	DWC_SPINLOCK_IRQSAVE(hcd->lock, &flags);
	retval = dwc_otg_hcd_qh_add(hcd, *qh);
	if (retval == 0) {
		DWC_CIRCLEQ_INSERT_TAIL(&((*qh)->qtd_list), qtd,
					qtd_list_entry);
	}
	DWC_SPINUNLOCK_IRQRESTORE(hcd->lock, flags);

done:

	return retval;
}

#endif  

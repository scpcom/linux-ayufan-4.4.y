#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef DWC_DEVICE_ONLY

#include "dwc_otg_hcd.h"
#include "dwc_otg_regs.h"

int32_t dwc_otg_hcd_handle_intr(dwc_otg_hcd_t * dwc_otg_hcd)
{
	int retval = 0;

	dwc_otg_core_if_t *core_if = dwc_otg_hcd->core_if;
	gintsts_data_t gintsts;
#ifdef DEBUG
	dwc_otg_core_global_regs_t *global_regs = core_if->core_global_regs;
#endif

	if (core_if->hibernation_suspend == 1) {
		return retval;
	}
	DWC_SPINLOCK(dwc_otg_hcd->lock);
	 
	if (dwc_otg_is_host_mode(core_if)) {
		gintsts.d32 = dwc_otg_read_core_intr(core_if);
		if (!gintsts.d32) {
			DWC_SPINUNLOCK(dwc_otg_hcd->lock);
			return 0;
		}
#ifdef DEBUG
		 
#ifndef DEBUG_SOF
		if (gintsts.d32 != DWC_SOF_INTR_MASK)
#endif
			DWC_DEBUGPL(DBG_HCD, "\n");
#endif

#ifdef DEBUG
#ifndef DEBUG_SOF
		if (gintsts.d32 != DWC_SOF_INTR_MASK)
#endif
			DWC_DEBUGPL(DBG_HCD,
				    "DWC OTG HCD Interrupt Detected gintsts&gintmsk=0x%08x\n",
				    gintsts.d32);
#endif

		if (gintsts.b.sofintr) {
			retval |= dwc_otg_hcd_handle_sof_intr(dwc_otg_hcd);
		}
		if (gintsts.b.rxstsqlvl) {
			retval |=
			    dwc_otg_hcd_handle_rx_status_q_level_intr
			    (dwc_otg_hcd);
		}
		if (gintsts.b.nptxfempty) {
			retval |=
			    dwc_otg_hcd_handle_np_tx_fifo_empty_intr
			    (dwc_otg_hcd);
		}
		if (gintsts.b.i2cintr) {
			 
		}
		if (gintsts.b.portintr) {
			retval |= dwc_otg_hcd_handle_port_intr(dwc_otg_hcd);
		}
		if (gintsts.b.hcintr) {
			retval |= dwc_otg_hcd_handle_hc_intr(dwc_otg_hcd);
		}
		if (gintsts.b.ptxfempty) {
			retval |=
			    dwc_otg_hcd_handle_perio_tx_fifo_empty_intr
			    (dwc_otg_hcd);
		}
#ifdef DEBUG
#ifndef DEBUG_SOF
		if (gintsts.d32 != DWC_SOF_INTR_MASK)
#endif
		{
			DWC_DEBUGPL(DBG_HCD,
				    "DWC OTG HCD Finished Servicing Interrupts\n");
			DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD gintsts=0x%08x\n",
				    DWC_READ_REG32(&global_regs->gintsts));
			DWC_DEBUGPL(DBG_HCDV, "DWC OTG HCD gintmsk=0x%08x\n",
				    DWC_READ_REG32(&global_regs->gintmsk));
		}
#endif

#ifdef DEBUG
#ifndef DEBUG_SOF
		if (gintsts.d32 != DWC_SOF_INTR_MASK)
#endif
			DWC_DEBUGPL(DBG_HCD, "\n");
#endif

	}
	DWC_SPINUNLOCK(dwc_otg_hcd->lock);
	return retval;
}

#ifdef DWC_TRACK_MISSED_SOFS
#warning Compiling code to track missed SOFs
#define FRAME_NUM_ARRAY_SIZE 1000
 
static inline void track_missed_sofs(uint16_t curr_frame_number)
{
	static uint16_t frame_num_array[FRAME_NUM_ARRAY_SIZE];
	static uint16_t last_frame_num_array[FRAME_NUM_ARRAY_SIZE];
	static int frame_num_idx = 0;
	static uint16_t last_frame_num = DWC_HFNUM_MAX_FRNUM;
	static int dumped_frame_num_array = 0;

	if (frame_num_idx < FRAME_NUM_ARRAY_SIZE) {
		if (((last_frame_num + 1) & DWC_HFNUM_MAX_FRNUM) !=
		    curr_frame_number) {
			frame_num_array[frame_num_idx] = curr_frame_number;
			last_frame_num_array[frame_num_idx++] = last_frame_num;
		}
	} else if (!dumped_frame_num_array) {
		int i;
		DWC_PRINTF("Frame     Last Frame\n");
		DWC_PRINTF("-----     ----------\n");
		for (i = 0; i < FRAME_NUM_ARRAY_SIZE; i++) {
			DWC_PRINTF("0x%04x    0x%04x\n",
				   frame_num_array[i], last_frame_num_array[i]);
		}
		dumped_frame_num_array = 1;
	}
	last_frame_num = curr_frame_number;
}
#endif

int32_t dwc_otg_hcd_handle_sof_intr(dwc_otg_hcd_t * hcd)
{
	hfnum_data_t hfnum;
	dwc_list_link_t *qh_entry;
	dwc_otg_qh_t *qh;
	dwc_otg_transaction_type_e tr_type;
	gintsts_data_t gintsts = {.d32 = 0 };

	hfnum.d32 =
	    DWC_READ_REG32(&hcd->core_if->host_if->host_global_regs->hfnum);

#ifdef DEBUG_SOF
	DWC_DEBUGPL(DBG_HCD, "--Start of Frame Interrupt--\n");
#endif
	hcd->frame_number = hfnum.b.frnum;

#ifdef DEBUG
	hcd->frrem_accum += hfnum.b.frrem;
	hcd->frrem_samples++;
#endif

#ifdef DWC_TRACK_MISSED_SOFS
	track_missed_sofs(hcd->frame_number);
#endif
	 
	qh_entry = DWC_LIST_FIRST(&hcd->periodic_sched_inactive);
	while (qh_entry != &hcd->periodic_sched_inactive) {
		qh = DWC_LIST_ENTRY(qh_entry, dwc_otg_qh_t, qh_list_entry);
		qh_entry = qh_entry->next;
		if (dwc_frame_num_le(qh->sched_frame, hcd->frame_number)) {
			 
			DWC_LIST_MOVE_HEAD(&hcd->periodic_sched_ready,
					   &qh->qh_list_entry);
		}
	}
	tr_type = dwc_otg_hcd_select_transactions(hcd);
	if (tr_type != DWC_OTG_TRANSACTION_NONE) {
		dwc_otg_hcd_queue_transactions(hcd, tr_type);
	}

	gintsts.b.sofintr = 1;
	DWC_WRITE_REG32(&hcd->core_if->core_global_regs->gintsts, gintsts.d32);

	return 1;
}

int32_t dwc_otg_hcd_handle_rx_status_q_level_intr(dwc_otg_hcd_t * dwc_otg_hcd)
{
	host_grxsts_data_t grxsts;
	dwc_hc_t *hc = NULL;

	DWC_DEBUGPL(DBG_HCD, "--RxStsQ Level Interrupt--\n");

	grxsts.d32 =
	    DWC_READ_REG32(&dwc_otg_hcd->core_if->core_global_regs->grxstsp);

	hc = dwc_otg_hcd->hc_ptr_array[grxsts.b.chnum];
	if (!hc) {
		DWC_ERROR("Unable to get corresponding channel\n");
		return 0;
	}

	DWC_DEBUGPL(DBG_HCDV, "    Ch num = %d\n", grxsts.b.chnum);
	DWC_DEBUGPL(DBG_HCDV, "    Count = %d\n", grxsts.b.bcnt);
	DWC_DEBUGPL(DBG_HCDV, "    DPID = %d, hc.dpid = %d\n", grxsts.b.dpid,
		    hc->data_pid_start);
	DWC_DEBUGPL(DBG_HCDV, "    PStatus = %d\n", grxsts.b.pktsts);

	switch (grxsts.b.pktsts) {
	case DWC_GRXSTS_PKTSTS_IN:
		 
		if (grxsts.b.bcnt > 0) {
			dwc_otg_read_packet(dwc_otg_hcd->core_if,
					    hc->xfer_buff, grxsts.b.bcnt);

			hc->xfer_count += grxsts.b.bcnt;
			hc->xfer_buff += grxsts.b.bcnt;
		}

	case DWC_GRXSTS_PKTSTS_IN_XFER_COMP:
	case DWC_GRXSTS_PKTSTS_DATA_TOGGLE_ERR:
	case DWC_GRXSTS_PKTSTS_CH_HALTED:
		 
		break;
	default:
		DWC_ERROR("RX_STS_Q Interrupt: Unknown status %d\n",
			  grxsts.b.pktsts);
		break;
	}

	return 1;
}

int32_t dwc_otg_hcd_handle_np_tx_fifo_empty_intr(dwc_otg_hcd_t * dwc_otg_hcd)
{
	DWC_DEBUGPL(DBG_HCD, "--Non-Periodic TxFIFO Empty Interrupt--\n");
	dwc_otg_hcd_queue_transactions(dwc_otg_hcd,
				       DWC_OTG_TRANSACTION_NON_PERIODIC);
	return 1;
}

int32_t dwc_otg_hcd_handle_perio_tx_fifo_empty_intr(dwc_otg_hcd_t * dwc_otg_hcd)
{
	DWC_DEBUGPL(DBG_HCD, "--Periodic TxFIFO Empty Interrupt--\n");
	dwc_otg_hcd_queue_transactions(dwc_otg_hcd,
				       DWC_OTG_TRANSACTION_PERIODIC);
	return 1;
}

int32_t dwc_otg_hcd_handle_port_intr(dwc_otg_hcd_t * dwc_otg_hcd)
{
	int retval = 0;
	hprt0_data_t hprt0;
	hprt0_data_t hprt0_modify;

	hprt0.d32 = DWC_READ_REG32(dwc_otg_hcd->core_if->host_if->hprt0);
	hprt0_modify.d32 = DWC_READ_REG32(dwc_otg_hcd->core_if->host_if->hprt0);

	hprt0_modify.b.prtena = 0;
	hprt0_modify.b.prtconndet = 0;
	hprt0_modify.b.prtenchng = 0;
	hprt0_modify.b.prtovrcurrchng = 0;

	if (dwc_otg_hcd->core_if->hibernation_suspend == 1) {
		 
		hprt0_modify.b.prtconndet = 1;
		hprt0_modify.b.prtenchng = 1;
		DWC_WRITE_REG32(dwc_otg_hcd->core_if->host_if->hprt0, hprt0_modify.d32);
		hprt0.d32 = DWC_READ_REG32(dwc_otg_hcd->core_if->host_if->hprt0);
		return retval;
	}

	if (hprt0.b.prtconndet) {
		 
		if (dwc_otg_hcd->core_if->adp_enable &&
				dwc_otg_hcd->core_if->adp.vbuson_timer_started == 1) {
			DWC_PRINTF("PORT CONNECT DETECTED ----------------\n");
			DWC_TIMER_CANCEL(dwc_otg_hcd->core_if->adp.vbuson_timer);
			dwc_otg_hcd->core_if->adp.vbuson_timer_started = 0;
			 
		} else {

			DWC_DEBUGPL(DBG_HCD, "--Port Interrupt HPRT0=0x%08x "
				    "Port Connect Detected--\n", hprt0.d32);
			dwc_otg_hcd->flags.b.port_connect_status_change = 1;
			dwc_otg_hcd->flags.b.port_connect_status = 1;
			hprt0_modify.b.prtconndet = 1;

			DWC_TIMER_CANCEL(dwc_otg_hcd->conn_timer);
		}
		 
		retval |= 1;
	}

	if (hprt0.b.prtenchng) {
		DWC_DEBUGPL(DBG_HCD, "  --Port Interrupt HPRT0=0x%08x "
			    "Port Enable Changed--\n", hprt0.d32);
		hprt0_modify.b.prtenchng = 1;
		if (hprt0.b.prtena == 1) {
			hfir_data_t hfir;
			int do_reset = 0;
			dwc_otg_core_params_t *params =
			    dwc_otg_hcd->core_if->core_params;
			dwc_otg_core_global_regs_t *global_regs =
			    dwc_otg_hcd->core_if->core_global_regs;
			dwc_otg_host_if_t *host_if =
			    dwc_otg_hcd->core_if->host_if;

			hfir.d32 = DWC_READ_REG32(&host_if->host_global_regs->hfir);
			hfir.b.frint = calc_frame_interval(dwc_otg_hcd->core_if);
			DWC_WRITE_REG32(&host_if->host_global_regs->hfir, hfir.d32);

			if (params->host_support_fs_ls_low_power) {
				gusbcfg_data_t usbcfg;

				usbcfg.d32 =
				    DWC_READ_REG32(&global_regs->gusbcfg);

				if (hprt0.b.prtspd == DWC_HPRT0_PRTSPD_LOW_SPEED
				    || hprt0.b.prtspd ==
				    DWC_HPRT0_PRTSPD_FULL_SPEED) {
					 
					hcfg_data_t hcfg;
					if (usbcfg.b.phylpwrclksel == 0) {
						 
						usbcfg.b.phylpwrclksel = 1;
						DWC_WRITE_REG32
						    (&global_regs->gusbcfg,
						     usbcfg.d32);
						do_reset = 1;
					}

					hcfg.d32 =
					    DWC_READ_REG32
					    (&host_if->host_global_regs->hcfg);

					if (hprt0.b.prtspd ==
					    DWC_HPRT0_PRTSPD_LOW_SPEED
					    && params->host_ls_low_power_phy_clk
					    ==
					    DWC_HOST_LS_LOW_POWER_PHY_CLK_PARAM_6MHZ)
					{
						 
						DWC_DEBUGPL(DBG_CIL,
							    "FS_PHY programming HCFG to 6 MHz (Low Power)\n");
						if (hcfg.b.fslspclksel !=
						    DWC_HCFG_6_MHZ) {
							hcfg.b.fslspclksel =
							    DWC_HCFG_6_MHZ;
							DWC_WRITE_REG32
							    (&host_if->host_global_regs->hcfg,
							     hcfg.d32);
							do_reset = 1;
						}
					} else {
						 
						DWC_DEBUGPL(DBG_CIL,
							    "FS_PHY programming HCFG to 48 MHz ()\n");
						if (hcfg.b.fslspclksel !=
						    DWC_HCFG_48_MHZ) {
							hcfg.b.fslspclksel =
							    DWC_HCFG_48_MHZ;
							DWC_WRITE_REG32
							    (&host_if->host_global_regs->hcfg,
							     hcfg.d32);
							do_reset = 1;
						}
					}
				} else {
					 
					if (usbcfg.b.phylpwrclksel == 1) {
						usbcfg.b.phylpwrclksel = 0;
						DWC_WRITE_REG32
						    (&global_regs->gusbcfg,
						     usbcfg.d32);
						do_reset = 1;
					}
				}

				if (do_reset) {
					DWC_TASK_SCHEDULE(dwc_otg_hcd->reset_tasklet);
				}
			}

			if (!do_reset) {
				 
				dwc_otg_hcd->flags.b.port_reset_change = 1;
			}
		} else {
			dwc_otg_hcd->flags.b.port_enable_change = 1;
		}
		retval |= 1;
	}

	if (hprt0.b.prtovrcurrchng) {
		DWC_DEBUGPL(DBG_HCD, "  --Port Interrupt HPRT0=0x%08x "
			    "Port Overcurrent Changed--\n", hprt0.d32);
		dwc_otg_hcd->flags.b.port_over_current_change = 1;
		hprt0_modify.b.prtovrcurrchng = 1;
		retval |= 1;
	}

	DWC_WRITE_REG32(dwc_otg_hcd->core_if->host_if->hprt0, hprt0_modify.d32);

	return retval;
}

int32_t dwc_otg_hcd_handle_hc_intr(dwc_otg_hcd_t * dwc_otg_hcd)
{
	int i;
	int retval = 0;
	haint_data_t haint;

	haint.d32 = dwc_otg_read_host_all_channels_intr(dwc_otg_hcd->core_if);

	for (i = 0; i < dwc_otg_hcd->core_if->core_params->host_channels; i++) {
		if (haint.b2.chint & (1 << i)) {
			retval |= dwc_otg_hcd_handle_hc_n_intr(dwc_otg_hcd, i);
		}
	}

	return retval;
}

static uint32_t get_actual_xfer_length(dwc_hc_t * hc,
				       dwc_otg_hc_regs_t * hc_regs,
				       dwc_otg_qtd_t * qtd,
				       dwc_otg_halt_status_e halt_status,
				       int *short_read)
{
	hctsiz_data_t hctsiz;
	uint32_t length;

	if (short_read != NULL) {
		*short_read = 0;
	}
	hctsiz.d32 = DWC_READ_REG32(&hc_regs->hctsiz);

	if (halt_status == DWC_OTG_HC_XFER_COMPLETE) {
		if (hc->ep_is_in) {
			length = hc->xfer_len - hctsiz.b.xfersize;
			if (short_read != NULL) {
				*short_read = (hctsiz.b.xfersize != 0);
			}
		} else if (hc->qh->do_split) {
			length = qtd->ssplit_out_xfer_count;
		} else {
			length = hc->xfer_len;
		}
	} else {
		 
		length =
		    (hc->start_pkt_count - hctsiz.b.pktcnt) * hc->max_packet;
	}

	return length;
}

static int update_urb_state_xfer_comp(dwc_hc_t * hc,
				      dwc_otg_hc_regs_t * hc_regs,
				      dwc_otg_hcd_urb_t * urb,
				      dwc_otg_qtd_t * qtd)
{
	int xfer_done = 0;
	int short_read = 0;

	int xfer_length;

	xfer_length = get_actual_xfer_length(hc, hc_regs, qtd,
					     DWC_OTG_HC_XFER_COMPLETE,
					     &short_read);

	if (hc->align_buff && xfer_length && hc->ep_is_in) {
		dwc_memcpy(urb->buf + urb->actual_length, hc->qh->dw_align_buf,
			   xfer_length);
	}

	urb->actual_length += xfer_length;

	if (xfer_length && (hc->ep_type == DWC_OTG_EP_TYPE_BULK) &&
	    (urb->flags & URB_SEND_ZERO_PACKET)
	    && (urb->actual_length == urb->length)
	    && !(urb->length % hc->max_packet)) {
		xfer_done = 0;
	} else if (short_read || urb->actual_length == urb->length) {
		xfer_done = 1;
		urb->status = 0;
	}

#ifdef DEBUG
	{
		hctsiz_data_t hctsiz;
		hctsiz.d32 = DWC_READ_REG32(&hc_regs->hctsiz);
		DWC_DEBUGPL(DBG_HCDV, "DWC_otg: %s: %s, channel %d\n",
			    __func__, (hc->ep_is_in ? "IN" : "OUT"),
			    hc->hc_num);
		DWC_DEBUGPL(DBG_HCDV, "  hc->xfer_len %d\n", hc->xfer_len);
		DWC_DEBUGPL(DBG_HCDV, "  hctsiz.xfersize %d\n",
			    hctsiz.b.xfersize);
		DWC_DEBUGPL(DBG_HCDV, "  urb->transfer_buffer_length %d\n",
			    urb->length);
		DWC_DEBUGPL(DBG_HCDV, "  urb->actual_length %d\n",
			    urb->actual_length);
		DWC_DEBUGPL(DBG_HCDV, "  short_read %d, xfer_done %d\n",
			    short_read, xfer_done);
	}
#endif

	return xfer_done;
}

void dwc_otg_hcd_save_data_toggle(dwc_hc_t * hc,
			     dwc_otg_hc_regs_t * hc_regs, dwc_otg_qtd_t * qtd)
{
	hctsiz_data_t hctsiz;
	hctsiz.d32 = DWC_READ_REG32(&hc_regs->hctsiz);

	if (hc->ep_type != DWC_OTG_EP_TYPE_CONTROL) {
		dwc_otg_qh_t *qh = hc->qh;
		if (hctsiz.b.pid == DWC_HCTSIZ_DATA0) {
			qh->data_toggle = DWC_OTG_HC_PID_DATA0;
		} else {
			qh->data_toggle = DWC_OTG_HC_PID_DATA1;
		}
	} else {
		if (hctsiz.b.pid == DWC_HCTSIZ_DATA0) {
			qtd->data_toggle = DWC_OTG_HC_PID_DATA0;
		} else {
			qtd->data_toggle = DWC_OTG_HC_PID_DATA1;
		}
	}
}

static dwc_otg_halt_status_e
update_isoc_urb_state(dwc_otg_hcd_t * hcd,
		      dwc_hc_t * hc,
		      dwc_otg_hc_regs_t * hc_regs,
		      dwc_otg_qtd_t * qtd, dwc_otg_halt_status_e halt_status)
{
	dwc_otg_hcd_urb_t *urb = qtd->urb;
	dwc_otg_halt_status_e ret_val = halt_status;
	struct dwc_otg_hcd_iso_packet_desc *frame_desc;

	frame_desc = &urb->iso_descs[qtd->isoc_frame_index];
	switch (halt_status) {
	case DWC_OTG_HC_XFER_COMPLETE:
		frame_desc->status = 0;
		frame_desc->actual_length =
		    get_actual_xfer_length(hc, hc_regs, qtd, halt_status, NULL);

		if (hc->align_buff && frame_desc->actual_length && hc->ep_is_in) {
			dwc_memcpy(urb->buf + frame_desc->offset + qtd->isoc_split_offset,
				   hc->qh->dw_align_buf, frame_desc->actual_length);
		}

		break;
	case DWC_OTG_HC_XFER_FRAME_OVERRUN:
		urb->error_count++;
		if (hc->ep_is_in) {
			frame_desc->status = -DWC_E_NO_STREAM_RES;
		} else {
			frame_desc->status = -DWC_E_COMMUNICATION;
		}
		frame_desc->actual_length = 0;
		break;
	case DWC_OTG_HC_XFER_BABBLE_ERR:
		urb->error_count++;
		frame_desc->status = -DWC_E_OVERFLOW;
		 
		break;
	case DWC_OTG_HC_XFER_XACT_ERR:
		urb->error_count++;
		frame_desc->status = -DWC_E_PROTOCOL;
		frame_desc->actual_length =
		    get_actual_xfer_length(hc, hc_regs, qtd, halt_status, NULL);

		if (hc->align_buff && frame_desc->actual_length && hc->ep_is_in) {
			dwc_memcpy(urb->buf + frame_desc->offset + qtd->isoc_split_offset,
				   hc->qh->dw_align_buf, frame_desc->actual_length);
		}
		 
		if (hc->qh->do_split && (hc->ep_type == DWC_OTG_EP_TYPE_ISOC) &&
		    hc->ep_is_in && hcd->core_if->dma_enable) {
			qtd->complete_split = 0;
			qtd->isoc_split_offset = 0;
		}

		break;
	default:
		DWC_ASSERT(1, "Unhandled _halt_status (%d)\n", halt_status);
		break;
	}
	if (++qtd->isoc_frame_index == urb->packet_count) {
		 
		hcd->fops->complete(hcd, urb->priv, urb, 0);
		ret_val = DWC_OTG_HC_XFER_URB_COMPLETE;
	} else {
		ret_val = DWC_OTG_HC_XFER_COMPLETE;
	}
	return ret_val;
}

static void deactivate_qh(dwc_otg_hcd_t * hcd, dwc_otg_qh_t * qh, int free_qtd)
{
	int continue_split = 0;
	dwc_otg_qtd_t *qtd;

	DWC_DEBUGPL(DBG_HCDV, "  %s(%p,%p,%d)\n", __func__, hcd, qh, free_qtd);

	qtd = DWC_CIRCLEQ_FIRST(&qh->qtd_list);

	if (qtd->complete_split) {
		continue_split = 1;
	} else if (qtd->isoc_split_pos == DWC_HCSPLIT_XACTPOS_MID ||
		   qtd->isoc_split_pos == DWC_HCSPLIT_XACTPOS_END) {
		continue_split = 1;
	}

	if (free_qtd) {
		dwc_otg_hcd_qtd_remove_and_free(hcd, qtd, qh);
		continue_split = 0;
	}

	qh->channel = NULL;
	dwc_otg_hcd_qh_deactivate(hcd, qh, continue_split);
}

static void release_channel(dwc_otg_hcd_t * hcd,
			    dwc_hc_t * hc,
			    dwc_otg_qtd_t * qtd,
			    dwc_otg_halt_status_e halt_status)
{
	dwc_otg_transaction_type_e tr_type;
	int free_qtd;
    gintmsk_data_t intr_mask = {.d32 = 0 };

	DWC_DEBUGPL(DBG_HCDV, "  %s: channel %d, halt_status %d\n",
		    __func__, hc->hc_num, halt_status);

	switch (halt_status) {
	case DWC_OTG_HC_XFER_URB_COMPLETE:
		free_qtd = 1;
		break;
	case DWC_OTG_HC_XFER_AHB_ERR:
	case DWC_OTG_HC_XFER_STALL:
	case DWC_OTG_HC_XFER_BABBLE_ERR:
		free_qtd = 1;
		break;
	case DWC_OTG_HC_XFER_XACT_ERR:
		if (qtd->error_count >= 3) {
			DWC_DEBUGPL(DBG_HCDV,
				    "  Complete URB with transaction error\n");
			free_qtd = 1;
			qtd->urb->status = -DWC_E_PROTOCOL;
			hcd->fops->complete(hcd, qtd->urb->priv,
					    qtd->urb, -DWC_E_PROTOCOL);
		} else {
			free_qtd = 0;
		}
		break;
	case DWC_OTG_HC_XFER_URB_DEQUEUE:
		 
		goto cleanup;
	case DWC_OTG_HC_XFER_NO_HALT_STATUS:
		free_qtd = 0;
		break;
	case DWC_OTG_HC_XFER_PERIODIC_INCOMPLETE:
		DWC_DEBUGPL(DBG_HCDV,
			"  Complete URB with I/O error\n");
		free_qtd = 1;
		qtd->urb->status = -DWC_E_IO;
		hcd->fops->complete(hcd, qtd->urb->priv,
			qtd->urb, -DWC_E_IO);
		break;
	default:
		free_qtd = 0;
		break;
	}

	deactivate_qh(hcd, hc->qh, free_qtd);

cleanup:
	 
	dwc_otg_hc_cleanup(hcd->core_if, hc);
	DWC_CIRCLEQ_INSERT_TAIL(&hcd->free_hc_list, hc, hc_list_entry);

	switch (hc->ep_type) {
	case DWC_OTG_EP_TYPE_CONTROL:
	case DWC_OTG_EP_TYPE_BULK:
		hcd->non_periodic_channels--;
		break;

	default:
		 
		break;
	}

	
    intr_mask.d32 = DWC_READ_REG32(&hcd->core_if->core_global_regs->gintmsk);
#if defined(CONFIG_SYNO_C2K_DAC_TERRIBLE_SOUND)
    if (!intr_mask.b.sofintr || (intr_mask.b.sofintr && (hc->ep_type != DWC_OTG_EP_TYPE_BULK))) {
#else
    if (!intr_mask.b.sofintr) {
#endif
        tr_type = dwc_otg_hcd_select_transactions(hcd);
        if (tr_type != DWC_OTG_TRANSACTION_NONE) {
            dwc_otg_hcd_queue_transactions(hcd, tr_type);
        }
    }
}

static void halt_channel(dwc_otg_hcd_t * hcd,
			 dwc_hc_t * hc,
			 dwc_otg_qtd_t * qtd, dwc_otg_halt_status_e halt_status)
{
	if (hcd->core_if->dma_enable) {
		release_channel(hcd, hc, qtd, halt_status);
		return;
	}

	dwc_otg_hc_halt(hcd->core_if, hc, halt_status);

	if (hc->halt_on_queue) {
		gintmsk_data_t gintmsk = {.d32 = 0 };
		dwc_otg_core_global_regs_t *global_regs;
		global_regs = hcd->core_if->core_global_regs;

		if (hc->ep_type == DWC_OTG_EP_TYPE_CONTROL ||
		    hc->ep_type == DWC_OTG_EP_TYPE_BULK) {
			 
			gintmsk.b.nptxfempty = 1;
			DWC_MODIFY_REG32(&global_regs->gintmsk, 0, gintmsk.d32);
		} else {
			 
			DWC_LIST_MOVE_HEAD(&hcd->periodic_sched_assigned,
					   &hc->qh->qh_list_entry);

			gintmsk.b.ptxfempty = 1;
			DWC_MODIFY_REG32(&global_regs->gintmsk, 0, gintmsk.d32);
		}
	}
}

static void complete_non_periodic_xfer(dwc_otg_hcd_t * hcd,
				       dwc_hc_t * hc,
				       dwc_otg_hc_regs_t * hc_regs,
				       dwc_otg_qtd_t * qtd,
				       dwc_otg_halt_status_e halt_status)
{
	hcint_data_t hcint;

	qtd->error_count = 0;

	hcint.d32 = DWC_READ_REG32(&hc_regs->hcint);
	if (hcint.b.nyet) {
		 
		hc->qh->ping_state = 1;
		clear_hc_int(hc_regs, nyet);
	}

	if (hc->ep_is_in) {
		 
		halt_channel(hcd, hc, qtd, halt_status);
	} else {
		 
		release_channel(hcd, hc, qtd, halt_status);
	}
}

static void complete_periodic_xfer(dwc_otg_hcd_t * hcd,
				   dwc_hc_t * hc,
				   dwc_otg_hc_regs_t * hc_regs,
				   dwc_otg_qtd_t * qtd,
				   dwc_otg_halt_status_e halt_status)
{
	hctsiz_data_t hctsiz;
	qtd->error_count = 0;

	hctsiz.d32 = DWC_READ_REG32(&hc_regs->hctsiz);
	if (!hc->ep_is_in || hctsiz.b.pktcnt == 0) {
		 
		release_channel(hcd, hc, qtd, halt_status);
	} else {
		 
		halt_channel(hcd, hc, qtd, halt_status);
	}
}

static int32_t handle_xfercomp_isoc_split_in(dwc_otg_hcd_t * hcd,
					     dwc_hc_t * hc,
					     dwc_otg_hc_regs_t * hc_regs,
					     dwc_otg_qtd_t * qtd)
{
	uint32_t len;
	struct dwc_otg_hcd_iso_packet_desc *frame_desc;
	frame_desc = &qtd->urb->iso_descs[qtd->isoc_frame_index];

	len = get_actual_xfer_length(hc, hc_regs, qtd,
				     DWC_OTG_HC_XFER_COMPLETE, NULL);

	if (!len) {
		qtd->complete_split = 0;
		qtd->isoc_split_offset = 0;
		return 0;
	}
	frame_desc->actual_length += len;

	if (hc->align_buff && len)
		dwc_memcpy(qtd->urb->buf + frame_desc->offset +
			   qtd->isoc_split_offset, hc->qh->dw_align_buf, len);
	qtd->isoc_split_offset += len;

	if (frame_desc->length == frame_desc->actual_length) {
		frame_desc->status = 0;
		qtd->isoc_frame_index++;
		qtd->complete_split = 0;
		qtd->isoc_split_offset = 0;
	}

	if (qtd->isoc_frame_index == qtd->urb->packet_count) {
		hcd->fops->complete(hcd, qtd->urb->priv, qtd->urb, 0);
		release_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_URB_COMPLETE);
	} else {
		release_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_NO_HALT_STATUS);
	}

	return 1;		 
}

static int32_t handle_hc_xfercomp_intr(dwc_otg_hcd_t * hcd,
				       dwc_hc_t * hc,
				       dwc_otg_hc_regs_t * hc_regs,
				       dwc_otg_qtd_t * qtd)
{
	int urb_xfer_done;
	dwc_otg_halt_status_e halt_status = DWC_OTG_HC_XFER_COMPLETE;
	dwc_otg_hcd_urb_t *urb = qtd->urb;
	int pipe_type = dwc_otg_hcd_get_pipe_type(&urb->pipe_info);

	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "Transfer Complete--\n", hc->hc_num);

	if (hcd->core_if->dma_desc_enable) {
		dwc_otg_hcd_complete_xfer_ddma(hcd, hc, hc_regs, halt_status);
		if (pipe_type == UE_ISOCHRONOUS) {
			 
			clear_hc_int(hc_regs, xfercomp);
			return 1;
		}
		goto handle_xfercomp_done;
	}

	if (hc->qh->do_split) {
		if ((hc->ep_type == DWC_OTG_EP_TYPE_ISOC) && hc->ep_is_in
		    && hcd->core_if->dma_enable) {
			if (qtd->complete_split
			    && handle_xfercomp_isoc_split_in(hcd, hc, hc_regs,
							     qtd))
				goto handle_xfercomp_done;
		} else {
			qtd->complete_split = 0;
		}
	}

	switch (pipe_type) {
	case UE_CONTROL:
		switch (qtd->control_phase) {
		case DWC_OTG_CONTROL_SETUP:
			if (urb->length > 0) {
				qtd->control_phase = DWC_OTG_CONTROL_DATA;
			} else {
				qtd->control_phase = DWC_OTG_CONTROL_STATUS;
			}
			DWC_DEBUGPL(DBG_HCDV,
				    "  Control setup transaction done\n");
			halt_status = DWC_OTG_HC_XFER_COMPLETE;
			break;
		case DWC_OTG_CONTROL_DATA:{
				urb_xfer_done =
				    update_urb_state_xfer_comp(hc, hc_regs, urb,
							       qtd);
				if (urb_xfer_done) {
					qtd->control_phase =
					    DWC_OTG_CONTROL_STATUS;
					DWC_DEBUGPL(DBG_HCDV,
						    "  Control data transfer done\n");
				} else {
					dwc_otg_hcd_save_data_toggle(hc, hc_regs, qtd);
				}
				halt_status = DWC_OTG_HC_XFER_COMPLETE;
				break;
			}
		case DWC_OTG_CONTROL_STATUS:
			DWC_DEBUGPL(DBG_HCDV, "  Control transfer complete\n");
			if (urb->status == -DWC_E_IN_PROGRESS) {
				urb->status = 0;
			}
			hcd->fops->complete(hcd, urb->priv, urb, urb->status);
			halt_status = DWC_OTG_HC_XFER_URB_COMPLETE;
			break;
		}

		complete_non_periodic_xfer(hcd, hc, hc_regs, qtd, halt_status);
		break;
	case UE_BULK:
		DWC_DEBUGPL(DBG_HCDV, "  Bulk transfer complete\n");
		urb_xfer_done =
		    update_urb_state_xfer_comp(hc, hc_regs, urb, qtd);
		if (urb_xfer_done) {
			hcd->fops->complete(hcd, urb->priv, urb, urb->status);
			halt_status = DWC_OTG_HC_XFER_URB_COMPLETE;
		} else {
			halt_status = DWC_OTG_HC_XFER_COMPLETE;
		}

		dwc_otg_hcd_save_data_toggle(hc, hc_regs, qtd);
		complete_non_periodic_xfer(hcd, hc, hc_regs, qtd, halt_status);
		break;
	case UE_INTERRUPT:
		DWC_DEBUGPL(DBG_HCDV, "  Interrupt transfer complete\n");
		urb_xfer_done =
			update_urb_state_xfer_comp(hc, hc_regs, urb, qtd);

		if (urb_xfer_done) {
				hcd->fops->complete(hcd, urb->priv, urb, urb->status);
				halt_status = DWC_OTG_HC_XFER_URB_COMPLETE;
		} else {
				halt_status = DWC_OTG_HC_XFER_COMPLETE;
		}

		dwc_otg_hcd_save_data_toggle(hc, hc_regs, qtd);
		complete_periodic_xfer(hcd, hc, hc_regs, qtd, halt_status);
		break;
	case UE_ISOCHRONOUS:
		DWC_DEBUGPL(DBG_HCDV, "  Isochronous transfer complete\n");
		if (qtd->isoc_split_pos == DWC_HCSPLIT_XACTPOS_ALL) {
			halt_status =
			    update_isoc_urb_state(hcd, hc, hc_regs, qtd,
						  DWC_OTG_HC_XFER_COMPLETE);
		}
		complete_periodic_xfer(hcd, hc, hc_regs, qtd, halt_status);
		break;
	}

handle_xfercomp_done:
	disable_hc_int(hc_regs, xfercompl);

	return 1;
}

static int32_t handle_hc_stall_intr(dwc_otg_hcd_t * hcd,
				    dwc_hc_t * hc,
				    dwc_otg_hc_regs_t * hc_regs,
				    dwc_otg_qtd_t * qtd)
{
	dwc_otg_hcd_urb_t *urb = qtd->urb;
	int pipe_type = dwc_otg_hcd_get_pipe_type(&urb->pipe_info);

	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "STALL Received--\n", hc->hc_num);

	if (hcd->core_if->dma_desc_enable) {
		dwc_otg_hcd_complete_xfer_ddma(hcd, hc, hc_regs, DWC_OTG_HC_XFER_STALL);
		goto handle_stall_done;
	}

	if (pipe_type == UE_CONTROL) {
		hcd->fops->complete(hcd, urb->priv, urb, -DWC_E_PIPE);
	}

	if (pipe_type == UE_BULK || pipe_type == UE_INTERRUPT) {
		hcd->fops->complete(hcd, urb->priv, urb, -DWC_E_PIPE);
		 
		hc->qh->data_toggle = 0;
	}

	halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_STALL);

handle_stall_done:
	disable_hc_int(hc_regs, stall);

	return 1;
}

static void update_urb_state_xfer_intr(dwc_hc_t * hc,
				       dwc_otg_hc_regs_t * hc_regs,
				       dwc_otg_hcd_urb_t * urb,
				       dwc_otg_qtd_t * qtd,
				       dwc_otg_halt_status_e halt_status)
{
	uint32_t bytes_transferred = get_actual_xfer_length(hc, hc_regs, qtd,
							    halt_status, NULL);
	 
	if (hc->align_buff && bytes_transferred && hc->ep_is_in) {
		dwc_memcpy(urb->buf + urb->actual_length, hc->qh->dw_align_buf,
			   bytes_transferred);
	}

	urb->actual_length += bytes_transferred;

#ifdef DEBUG
	{
		hctsiz_data_t hctsiz;
		hctsiz.d32 = DWC_READ_REG32(&hc_regs->hctsiz);
		DWC_DEBUGPL(DBG_HCDV, "DWC_otg: %s: %s, channel %d\n",
			    __func__, (hc->ep_is_in ? "IN" : "OUT"),
			    hc->hc_num);
		DWC_DEBUGPL(DBG_HCDV, "  hc->start_pkt_count %d\n",
			    hc->start_pkt_count);
		DWC_DEBUGPL(DBG_HCDV, "  hctsiz.pktcnt %d\n", hctsiz.b.pktcnt);
		DWC_DEBUGPL(DBG_HCDV, "  hc->max_packet %d\n", hc->max_packet);
		DWC_DEBUGPL(DBG_HCDV, "  bytes_transferred %d\n",
			    bytes_transferred);
		DWC_DEBUGPL(DBG_HCDV, "  urb->actual_length %d\n",
			    urb->actual_length);
		DWC_DEBUGPL(DBG_HCDV, "  urb->transfer_buffer_length %d\n",
			    urb->length);
	}
#endif
}

static int32_t handle_hc_nak_intr(dwc_otg_hcd_t * hcd,
				  dwc_hc_t * hc,
				  dwc_otg_hc_regs_t * hc_regs,
				  dwc_otg_qtd_t * qtd)
{
	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "NAK Received--\n", hc->hc_num);

	if (hc->do_split) {
		if (hc->complete_split) {
			qtd->error_count = 0;
		}
		qtd->complete_split = 0;
		halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_NAK);
		goto handle_nak_done;
	}

	switch (dwc_otg_hcd_get_pipe_type(&qtd->urb->pipe_info)) {
	case UE_CONTROL:
	case UE_BULK:
		if (hcd->core_if->dma_enable && hc->ep_is_in) {
			 
			qtd->error_count = 0;
			goto handle_nak_done;
		}

		qtd->error_count = 0;

		if (!hc->qh->ping_state) {
			update_urb_state_xfer_intr(hc, hc_regs,
						   qtd->urb, qtd,
						   DWC_OTG_HC_XFER_NAK);
			dwc_otg_hcd_save_data_toggle(hc, hc_regs, qtd);

			if (hc->speed == DWC_OTG_EP_SPEED_HIGH)
				hc->qh->ping_state = 1;
		}

		halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_NAK);
		break;
	case UE_INTERRUPT:
		qtd->error_count = 0;
		halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_NAK);
		break;
	case UE_ISOCHRONOUS:
		 
		DWC_ASSERT(1, "NACK interrupt for ISOC transfer\n");
		break;
	}

handle_nak_done:
	disable_hc_int(hc_regs, nak);

	return 1;
}

static int32_t handle_hc_ack_intr(dwc_otg_hcd_t * hcd,
				  dwc_hc_t * hc,
				  dwc_otg_hc_regs_t * hc_regs,
				  dwc_otg_qtd_t * qtd)
{
	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "ACK Received--\n", hc->hc_num);

	if (hc->do_split) {
		 
		if (!hc->ep_is_in && hc->data_pid_start != DWC_OTG_HC_PID_SETUP) {
			qtd->ssplit_out_xfer_count = hc->xfer_len;
		}
		if (!(hc->ep_type == DWC_OTG_EP_TYPE_ISOC && !hc->ep_is_in)) {
			 
			qtd->complete_split = 1;
		}

		if (hc->ep_type == DWC_OTG_EP_TYPE_ISOC && !hc->ep_is_in) {
			switch (hc->xact_pos) {
			case DWC_HCSPLIT_XACTPOS_ALL:
				break;
			case DWC_HCSPLIT_XACTPOS_END:
				qtd->isoc_split_pos = DWC_HCSPLIT_XACTPOS_ALL;
				qtd->isoc_split_offset = 0;
				break;
			case DWC_HCSPLIT_XACTPOS_BEGIN:
			case DWC_HCSPLIT_XACTPOS_MID:
				 
				{
					struct dwc_otg_hcd_iso_packet_desc
					*frame_desc;

					frame_desc =
					    &qtd->urb->
					    iso_descs[qtd->isoc_frame_index];
					qtd->isoc_split_offset += 188;

					if ((frame_desc->length -
					     qtd->isoc_split_offset) <= 188) {
						qtd->isoc_split_pos =
						    DWC_HCSPLIT_XACTPOS_END;
					} else {
						qtd->isoc_split_pos =
						    DWC_HCSPLIT_XACTPOS_MID;
					}

				}
				break;
			}
		} else {
			halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_ACK);
		}
	} else {
		qtd->error_count = 0;

		if (hc->qh->ping_state) {
			hc->qh->ping_state = 0;
			 
			halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_ACK);
		}
	}

	disable_hc_int(hc_regs, ack);

	return 1;
}

static int32_t handle_hc_nyet_intr(dwc_otg_hcd_t * hcd,
				   dwc_hc_t * hc,
				   dwc_otg_hc_regs_t * hc_regs,
				   dwc_otg_qtd_t * qtd)
{
	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "NYET Received--\n", hc->hc_num);

	if (hc->do_split && hc->complete_split) {
		if (hc->ep_is_in && (hc->ep_type == DWC_OTG_EP_TYPE_ISOC)
		    && hcd->core_if->dma_enable) {
			qtd->complete_split = 0;
			qtd->isoc_split_offset = 0;
			if (++qtd->isoc_frame_index == qtd->urb->packet_count) {
				hcd->fops->complete(hcd, qtd->urb->priv, qtd->urb, 0);
				release_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_URB_COMPLETE);
			}
			else
				release_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_NO_HALT_STATUS);
			goto handle_nyet_done;
		}

		if (hc->ep_type == DWC_OTG_EP_TYPE_INTR ||
		    hc->ep_type == DWC_OTG_EP_TYPE_ISOC) {
			int frnum = dwc_otg_hcd_get_frame_number(hcd);

			if (dwc_full_frame_num(frnum) !=
			    dwc_full_frame_num(hc->qh->sched_frame)) {
				 
#if 0
				 
				qtd->error_count++;
#endif
				qtd->complete_split = 0;
				halt_channel(hcd, hc, qtd,
					     DWC_OTG_HC_XFER_XACT_ERR);
				 
				goto handle_nyet_done;
			}
		}

		halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_NYET);
		goto handle_nyet_done;
	}

	hc->qh->ping_state = 1;
	qtd->error_count = 0;

	update_urb_state_xfer_intr(hc, hc_regs, qtd->urb, qtd,
				   DWC_OTG_HC_XFER_NYET);
	dwc_otg_hcd_save_data_toggle(hc, hc_regs, qtd);

	halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_NYET);

handle_nyet_done:
	disable_hc_int(hc_regs, nyet);
	return 1;
}

static int32_t handle_hc_babble_intr(dwc_otg_hcd_t * hcd,
				     dwc_hc_t * hc,
				     dwc_otg_hc_regs_t * hc_regs,
				     dwc_otg_qtd_t * qtd)
{
	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "Babble Error--\n", hc->hc_num);

	if (hcd->core_if->dma_desc_enable) {
		dwc_otg_hcd_complete_xfer_ddma(hcd, hc, hc_regs,
					       DWC_OTG_HC_XFER_BABBLE_ERR);
		goto handle_babble_done;
	}

	if (hc->ep_type != DWC_OTG_EP_TYPE_ISOC) {
		hcd->fops->complete(hcd, qtd->urb->priv,
				    qtd->urb, -DWC_E_OVERFLOW);
		halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_BABBLE_ERR);
	} else {
		dwc_otg_halt_status_e halt_status;
		halt_status = update_isoc_urb_state(hcd, hc, hc_regs, qtd,
						    DWC_OTG_HC_XFER_BABBLE_ERR);
		halt_channel(hcd, hc, qtd, halt_status);
	}

handle_babble_done:
	disable_hc_int(hc_regs, bblerr);
	return 1;
}

static int32_t handle_hc_ahberr_intr(dwc_otg_hcd_t * hcd,
				     dwc_hc_t * hc,
				     dwc_otg_hc_regs_t * hc_regs,
				     dwc_otg_qtd_t * qtd)
{
	hcchar_data_t hcchar;
	hcsplt_data_t hcsplt;
	hctsiz_data_t hctsiz;
	uint32_t hcdma;
	char *pipetype, *speed;

	dwc_otg_hcd_urb_t *urb = qtd->urb;

	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "AHB Error--\n", hc->hc_num);

	hcchar.d32 = DWC_READ_REG32(&hc_regs->hcchar);
	hcsplt.d32 = DWC_READ_REG32(&hc_regs->hcsplt);
	hctsiz.d32 = DWC_READ_REG32(&hc_regs->hctsiz);
	hcdma = DWC_READ_REG32(&hc_regs->hcdma);

	DWC_ERROR("AHB ERROR, Channel %d\n", hc->hc_num);
	DWC_ERROR("  hcchar 0x%08x, hcsplt 0x%08x\n", hcchar.d32, hcsplt.d32);
	DWC_ERROR("  hctsiz 0x%08x, hcdma 0x%08x\n", hctsiz.d32, hcdma);
	DWC_DEBUGPL(DBG_HCD, "DWC OTG HCD URB Enqueue\n");
	DWC_ERROR("  Device address: %d\n",
		  dwc_otg_hcd_get_dev_addr(&urb->pipe_info));
	DWC_ERROR("  Endpoint: %d, %s\n",
		  dwc_otg_hcd_get_ep_num(&urb->pipe_info),
		  (dwc_otg_hcd_is_pipe_in(&urb->pipe_info) ? "IN" : "OUT"));

	switch (dwc_otg_hcd_get_pipe_type(&urb->pipe_info)) {
	case UE_CONTROL:
		pipetype = "CONTROL";
		break;
	case UE_BULK:
		pipetype = "BULK";
		break;
	case UE_INTERRUPT:
		pipetype = "INTERRUPT";
		break;
	case UE_ISOCHRONOUS:
		pipetype = "ISOCHRONOUS";
		break;
	default:
		pipetype = "UNKNOWN";
		break;
	}

	DWC_ERROR("  Endpoint type: %s\n", pipetype);

	switch (hc->speed) {
	case DWC_OTG_EP_SPEED_HIGH:
		speed = "HIGH";
		break;
	case DWC_OTG_EP_SPEED_FULL:
		speed = "FULL";
		break;
	case DWC_OTG_EP_SPEED_LOW:
		speed = "LOW";
		break;
	default:
		speed = "UNKNOWN";
		break;
	};

	DWC_ERROR("  Speed: %s\n", speed);

	DWC_ERROR("  Max packet size: %d\n",
		  dwc_otg_hcd_get_mps(&urb->pipe_info));
	DWC_ERROR("  Data buffer length: %d\n", urb->length);
	DWC_ERROR("  Transfer buffer: %p, Transfer DMA: %p\n",
		  urb->buf, (void *)urb->dma);
	DWC_ERROR("  Setup buffer: %p, Setup DMA: %p\n",
		  urb->setup_packet, (void *)urb->setup_dma);
	DWC_ERROR("  Interval: %d\n", urb->interval);

	if (hcd->core_if->dma_desc_enable) {
		dwc_otg_hcd_complete_xfer_ddma(hcd, hc, hc_regs,
					       DWC_OTG_HC_XFER_AHB_ERR);
		goto handle_ahberr_done;
	}

	hcd->fops->complete(hcd, urb->priv, urb, -DWC_E_IO);

	dwc_otg_hc_halt(hcd->core_if, hc, DWC_OTG_HC_XFER_AHB_ERR);
handle_ahberr_done:
	disable_hc_int(hc_regs, ahberr);
	return 1;
}

static int32_t handle_hc_xacterr_intr(dwc_otg_hcd_t * hcd,
				      dwc_hc_t * hc,
				      dwc_otg_hc_regs_t * hc_regs,
				      dwc_otg_qtd_t * qtd)
{
	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "Transaction Error--\n", hc->hc_num);

	if (hcd->core_if->dma_desc_enable) {
		dwc_otg_hcd_complete_xfer_ddma(hcd, hc, hc_regs,
					       DWC_OTG_HC_XFER_XACT_ERR);
		goto handle_xacterr_done;
	}

	if (qtd == NULL)
		goto handle_xacterr_done;

	if(qtd->urb == NULL)
		goto handle_xacterr_done;

	if (&qtd->urb->pipe_info == NULL)
		goto handle_xacterr_done;

	switch (dwc_otg_hcd_get_pipe_type(&qtd->urb->pipe_info)) {
	case UE_CONTROL:
	case UE_BULK:
		qtd->error_count++;
		if (!hc->qh->ping_state) {

			update_urb_state_xfer_intr(hc, hc_regs,
						   qtd->urb, qtd,
						   DWC_OTG_HC_XFER_XACT_ERR);
			dwc_otg_hcd_save_data_toggle(hc, hc_regs, qtd);
			if (!hc->ep_is_in && hc->speed == DWC_OTG_EP_SPEED_HIGH) {
				hc->qh->ping_state = 1;
			}
		}

		halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_XACT_ERR);
		break;
	case UE_INTERRUPT:
		qtd->error_count++;
		if (hc->do_split && hc->complete_split) {
			qtd->complete_split = 0;
		}
		halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_XACT_ERR);
		break;
	case UE_ISOCHRONOUS:
		{
			dwc_otg_halt_status_e halt_status;
			halt_status =
			    update_isoc_urb_state(hcd, hc, hc_regs, qtd,
						  DWC_OTG_HC_XFER_XACT_ERR);

			halt_channel(hcd, hc, qtd, halt_status);
		}
		break;
	}
handle_xacterr_done:
	disable_hc_int(hc_regs, xacterr);

	return 1;
}

static int32_t handle_hc_frmovrun_intr(dwc_otg_hcd_t * hcd,
				       dwc_hc_t * hc,
				       dwc_otg_hc_regs_t * hc_regs,
				       dwc_otg_qtd_t * qtd)
{
	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "Frame Overrun--\n", hc->hc_num);

	switch (dwc_otg_hcd_get_pipe_type(&qtd->urb->pipe_info)) {
	case UE_CONTROL:
	case UE_BULK:
		break;
	case UE_INTERRUPT:
		halt_channel(hcd, hc, qtd, DWC_OTG_HC_XFER_FRAME_OVERRUN);
		break;
	case UE_ISOCHRONOUS:
		{
			dwc_otg_halt_status_e halt_status;
			halt_status =
			    update_isoc_urb_state(hcd, hc, hc_regs, qtd,
						  DWC_OTG_HC_XFER_FRAME_OVERRUN);

			halt_channel(hcd, hc, qtd, halt_status);
		}
		break;
	}

	disable_hc_int(hc_regs, frmovrun);

	return 1;
}

static int32_t handle_hc_datatglerr_intr(dwc_otg_hcd_t * hcd,
					 dwc_hc_t * hc,
					 dwc_otg_hc_regs_t * hc_regs,
					 dwc_otg_qtd_t * qtd)
{
	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "Data Toggle Error--\n", hc->hc_num);

	if (hc->ep_is_in) {
		qtd->error_count = 0;
	} else {
		DWC_ERROR("Data Toggle Error on OUT transfer,"
			  "channel %d\n", hc->hc_num);
	}

	disable_hc_int(hc_regs, datatglerr);

	return 1;
}

#ifdef DEBUG
 
static inline int halt_status_ok(dwc_otg_hcd_t * hcd,
				 dwc_hc_t * hc,
				 dwc_otg_hc_regs_t * hc_regs,
				 dwc_otg_qtd_t * qtd)
{
	hcchar_data_t hcchar;
	hctsiz_data_t hctsiz;
	hcint_data_t hcint;
	hcintmsk_data_t hcintmsk;
	hcsplt_data_t hcsplt;

	if (hc->halt_status == DWC_OTG_HC_XFER_NO_HALT_STATUS) {
		 
		hcchar.d32 = DWC_READ_REG32(&hc_regs->hcchar);
		hctsiz.d32 = DWC_READ_REG32(&hc_regs->hctsiz);
		hcint.d32 = DWC_READ_REG32(&hc_regs->hcint);
		hcintmsk.d32 = DWC_READ_REG32(&hc_regs->hcintmsk);
		hcsplt.d32 = DWC_READ_REG32(&hc_regs->hcsplt);
		DWC_WARN
		    ("%s: hc->halt_status == DWC_OTG_HC_XFER_NO_HALT_STATUS, "
		     "channel %d, hcchar 0x%08x, hctsiz 0x%08x, "
		     "hcint 0x%08x, hcintmsk 0x%08x, "
		     "hcsplt 0x%08x, qtd->complete_split %d\n", __func__,
		     hc->hc_num, hcchar.d32, hctsiz.d32, hcint.d32,
		     hcintmsk.d32, hcsplt.d32, qtd->complete_split);

		DWC_WARN("%s: no halt status, channel %d, ignoring interrupt\n",
			 __func__, hc->hc_num);
		DWC_WARN("\n");
		clear_hc_int(hc_regs, chhltd);
		return 0;
	}

	hcchar.d32 = DWC_READ_REG32(&hc_regs->hcchar);
	if (hcchar.b.chdis) {
		DWC_WARN("%s: hcchar.chdis set unexpectedly, "
			 "hcchar 0x%08x, trying to halt again\n",
			 __func__, hcchar.d32);
		clear_hc_int(hc_regs, chhltd);
		hc->halt_pending = 0;
		halt_channel(hcd, hc, qtd, hc->halt_status);
		return 0;
	}

	return 1;
}
#endif

static void handle_hc_chhltd_intr_dma(dwc_otg_hcd_t * hcd,
				      dwc_hc_t * hc,
				      dwc_otg_hc_regs_t * hc_regs,
				      dwc_otg_qtd_t * qtd)
{
	hcint_data_t hcint;
	hcintmsk_data_t hcintmsk;
	int out_nak_enh = 0;

    clear_hc_int(hc_regs, chhltd);

	if (hcd->core_if->snpsid >= OTG_CORE_REV_2_71a) {
		if (hc->speed == DWC_OTG_EP_SPEED_HIGH && !hc->ep_is_in &&
		    (hc->ep_type == DWC_OTG_EP_TYPE_CONTROL ||
		     hc->ep_type == DWC_OTG_EP_TYPE_BULK)) {
			out_nak_enh = 1;
		}
	}

	if (hc->halt_status == DWC_OTG_HC_XFER_URB_DEQUEUE ||
	    (hc->halt_status == DWC_OTG_HC_XFER_AHB_ERR
	     && !hcd->core_if->dma_desc_enable)) {
		 
		if (hcd->core_if->dma_desc_enable)
			dwc_otg_hcd_complete_xfer_ddma(hcd, hc, hc_regs,
						       hc->halt_status);
		else
			release_channel(hcd, hc, qtd, hc->halt_status);
		return;
	}

	hcint.d32 = DWC_READ_REG32(&hc_regs->hcint);
	hcintmsk.d32 = DWC_READ_REG32(&hc_regs->hcintmsk);

	if (hcint.b.xfercomp) {
		 
		if (hc->ep_type == DWC_OTG_EP_TYPE_ISOC && !hc->ep_is_in) {
			handle_hc_ack_intr(hcd, hc, hc_regs, qtd);
		}
		handle_hc_xfercomp_intr(hcd, hc, hc_regs, qtd);
	} else if (hcint.b.stall) {
		handle_hc_stall_intr(hcd, hc, hc_regs, qtd);
	} else if (hcint.b.xacterr && !hcd->core_if->dma_desc_enable) {
		if (out_nak_enh) {
			if (hcint.b.nyet || hcint.b.nak || hcint.b.ack) {
				DWC_DEBUG("XactErr with NYET/NAK/ACK\n");
				qtd->error_count = 0;
			} else {
				DWC_DEBUG("XactErr without NYET/NAK/ACK\n");
			}
		}

		handle_hc_xacterr_intr(hcd, hc, hc_regs, qtd);
	} else if (hcint.b.xcs_xact && hcd->core_if->dma_desc_enable) {
		handle_hc_xacterr_intr(hcd, hc, hc_regs, qtd);
	} else if (hcint.b.ahberr && hcd->core_if->dma_desc_enable) {
		handle_hc_ahberr_intr(hcd, hc, hc_regs, qtd);
	} else if (hcint.b.bblerr) {
		handle_hc_babble_intr(hcd, hc, hc_regs, qtd);
	} else if (hcint.b.frmovrun) {
		handle_hc_frmovrun_intr(hcd, hc, hc_regs, qtd);
	} else if (!out_nak_enh) {
		if (hcint.b.nyet) {
			 
			handle_hc_nyet_intr(hcd, hc, hc_regs, qtd);
		} else if (hcint.b.nak && !hcintmsk.b.nak) {
			 
			handle_hc_nak_intr(hcd, hc, hc_regs, qtd);
		} else if (hcint.b.ack && !hcintmsk.b.ack) {
			 
			handle_hc_ack_intr(hcd, hc, hc_regs, qtd);
		} else {
			if (hc->ep_type == DWC_OTG_EP_TYPE_INTR ||
			    hc->ep_type == DWC_OTG_EP_TYPE_ISOC) {
				 
#ifdef DEBUG
				DWC_PRINTF
				    ("%s: Halt channel %d (assume incomplete periodic transfer)\n",
				     __func__, hc->hc_num);
#endif
				halt_channel(hcd, hc, qtd,
					     DWC_OTG_HC_XFER_PERIODIC_INCOMPLETE);
			} else {
				DWC_ERROR
				    ("%s: Channel %d, DMA Mode -- ChHltd set, but reason "
				     "for halting is unknown, hcint 0x%08x, intsts 0x%08x\n",
				     __func__, hc->hc_num, hcint.d32,
				     DWC_READ_REG32(&hcd->
						    core_if->core_global_regs->
						    gintsts));
				halt_channel(hcd, hc, qtd,
					     DWC_OTG_HC_XFER_PERIODIC_INCOMPLETE);
			}

		}
	} else {
		DWC_PRINTF("NYET/NAK/ACK/other in non-error case, 0x%08x\n",
			   hcint.d32);
	}
}

static int32_t handle_hc_chhltd_intr(dwc_otg_hcd_t * hcd,
				     dwc_hc_t * hc,
				     dwc_otg_hc_regs_t * hc_regs,
				     dwc_otg_qtd_t * qtd)
{
	DWC_DEBUGPL(DBG_HCD, "--Host Channel %d Interrupt: "
		    "Channel Halted--\n", hc->hc_num);

	if (hcd->core_if->dma_enable) {
		handle_hc_chhltd_intr_dma(hcd, hc, hc_regs, qtd);
	} else {
#ifdef DEBUG
		if (!halt_status_ok(hcd, hc, hc_regs, qtd)) {
			return 1;
		}
#endif
		release_channel(hcd, hc, qtd, hc->halt_status);
	}

	return 1;
}

int32_t dwc_otg_hcd_handle_hc_n_intr(dwc_otg_hcd_t * dwc_otg_hcd, uint32_t num)
{
	int retval = 0;
	hcint_data_t hcint;
	hcintmsk_data_t hcintmsk;
	dwc_hc_t *hc;
	dwc_otg_hc_regs_t *hc_regs;
	dwc_otg_qtd_t *qtd;

	DWC_DEBUGPL(DBG_HCDV, "--Host Channel Interrupt--, Channel %d\n", num);

	hc = dwc_otg_hcd->hc_ptr_array[num];
	hc_regs = dwc_otg_hcd->core_if->host_if->hc_regs[num];
	qtd = DWC_CIRCLEQ_FIRST(&hc->qh->qtd_list);

	hcint.d32 = DWC_READ_REG32(&hc_regs->hcint);
	hcintmsk.d32 = DWC_READ_REG32(&hc_regs->hcintmsk);
	DWC_DEBUGPL(DBG_HCDV,
		    "  hcint 0x%08x, hcintmsk 0x%08x, hcint&hcintmsk 0x%08x\n",
		    hcint.d32, hcintmsk.d32, (hcint.d32 & hcintmsk.d32));
	hcint.d32 = hcint.d32 & hcintmsk.d32;

	if (!dwc_otg_hcd->core_if->dma_enable) {
		if (hcint.b.chhltd && hcint.d32 != 0x2) {
			hcint.b.chhltd = 0;
		}
	}

	if (hcint.b.xfercomp) {
		retval |=
		    handle_hc_xfercomp_intr(dwc_otg_hcd, hc, hc_regs, qtd);
		 
		hcint.b.nyet = 0;
	}
	if (hcint.b.chhltd) {
		retval |= handle_hc_chhltd_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.ahberr) {
		retval |= handle_hc_ahberr_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.stall) {
		retval |= handle_hc_stall_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.nak) {
		retval |= handle_hc_nak_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.ack) {
		retval |= handle_hc_ack_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.nyet) {
		retval |= handle_hc_nyet_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.xacterr) {
		retval |= handle_hc_xacterr_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.bblerr) {
		retval |= handle_hc_babble_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.frmovrun) {
		retval |=
		    handle_hc_frmovrun_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}
	if (hcint.b.datatglerr) {
		retval |=
		    handle_hc_datatglerr_intr(dwc_otg_hcd, hc, hc_regs, qtd);
	}

	return retval;
}

#endif  

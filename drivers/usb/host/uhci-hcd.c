#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/pm.h>
#include <linux/dmapool.h>
#include <linux/dma-mapping.h>
#include <linux/usb.h>
#include <linux/bitops.h>
#include <linux/dmi.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>

#include "../core/hcd.h"
#include "uhci-hcd.h"
#include "pci-quirks.h"

#define DRIVER_AUTHOR "Linus 'Frodo Rabbit' Torvalds, Johannes Erdfelt, \
Randy Dunlap, Georg Acher, Deti Fliegl, Thomas Sailer, Roman Weissgaerber, \
Alan Stern"
#define DRIVER_DESC "USB Universal Host Controller Interface driver"

static int ignore_oc;
module_param(ignore_oc, bool, S_IRUGO);
MODULE_PARM_DESC(ignore_oc, "ignore hardware overcurrent indications");

#ifdef DEBUG
#define DEBUG_CONFIGURED	1
static int debug = 1;
module_param(debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug level");

#else
#define DEBUG_CONFIGURED	0
#define debug			0
#endif

static char *errbuf;
#define ERRBUF_LEN    (32 * 1024)

static struct kmem_cache *uhci_up_cachep;	 

static void suspend_rh(struct uhci_hcd *uhci, enum uhci_rh_state new_state);
static void wakeup_rh(struct uhci_hcd *uhci);
static void uhci_get_current_frame_number(struct uhci_hcd *uhci);

static __le32 uhci_frame_skel_link(struct uhci_hcd *uhci, int frame)
{
	int skelnum;

	skelnum = 8 - (int) __ffs(frame | UHCI_NUMFRAMES);
	if (skelnum <= 1)
		skelnum = 9;
	return LINK_TO_QH(uhci->skelqh[skelnum]);
}

#include "uhci-debug.c"
#include "uhci-q.c"
#include "uhci-hub.c"

static void finish_reset(struct uhci_hcd *uhci)
{
	int port;

	for (port = 0; port < uhci->rh_numports; ++port)
		outw(0, uhci->io_addr + USBPORTSC1 + (port * 2));

	uhci->port_c_suspend = uhci->resuming_ports = 0;
	uhci->rh_state = UHCI_RH_RESET;
	uhci->is_stopped = UHCI_IS_STOPPED;
	uhci_to_hcd(uhci)->state = HC_STATE_HALT;
	uhci_to_hcd(uhci)->poll_rh = 0;

	uhci->dead = 0;		 
}

static void uhci_hc_died(struct uhci_hcd *uhci)
{
	uhci_get_current_frame_number(uhci);
	uhci_reset_hc(to_pci_dev(uhci_dev(uhci)), uhci->io_addr);
	finish_reset(uhci);
	uhci->dead = 1;

	++uhci->frame_number;
}

static void check_and_reset_hc(struct uhci_hcd *uhci)
{
	if (uhci_check_and_reset_hc(to_pci_dev(uhci_dev(uhci)), uhci->io_addr))
		finish_reset(uhci);
}

static void configure_hc(struct uhci_hcd *uhci)
{
	 
	outb(USBSOF_DEFAULT, uhci->io_addr + USBSOF);

	outl(uhci->frame_dma_handle, uhci->io_addr + USBFLBASEADD);

	outw(uhci->frame_number & UHCI_MAX_SOF_NUMBER,
			uhci->io_addr + USBFRNUM);

	uhci_to_hcd(uhci)->state = HC_STATE_SUSPENDED;
	mb();

	pci_write_config_word(to_pci_dev(uhci_dev(uhci)), USBLEGSUP,
			USBLEGSUP_DEFAULT);
}

static int resume_detect_interrupts_are_broken(struct uhci_hcd *uhci)
{
	int port;

	if (ignore_oc)
		return 1;

	switch (to_pci_dev(uhci_dev(uhci))->vendor) {
	    default:
		break;

	    case PCI_VENDOR_ID_GENESYS:
		 
		return 1;

	    case PCI_VENDOR_ID_INTEL:
		 
		for (port = 0; port < uhci->rh_numports; ++port) {
			if (inw(uhci->io_addr + USBPORTSC1 + port * 2) &
					USBPORTSC_OC)
				return 1;
		}
		break;
	}
	return 0;
}

static int global_suspend_mode_is_broken(struct uhci_hcd *uhci)
{
	int port;
	const char *sys_info;
	static char bad_Asus_board[] = "A7V8X";

	sys_info = dmi_get_system_info(DMI_BOARD_NAME);
	if (sys_info && !strcmp(sys_info, bad_Asus_board)) {
		for (port = 0; port < uhci->rh_numports; ++port) {
			if (inw(uhci->io_addr + USBPORTSC1 + port * 2) &
					USBPORTSC_CCS)
				return 1;
		}
	}

	return 0;
}

static void suspend_rh(struct uhci_hcd *uhci, enum uhci_rh_state new_state)
__releases(uhci->lock)
__acquires(uhci->lock)
{
	int auto_stop;
	int int_enable, egsm_enable, wakeup_enable;
	struct usb_device *rhdev = uhci_to_hcd(uhci)->self.root_hub;

	auto_stop = (new_state == UHCI_RH_AUTO_STOPPED);
	dev_dbg(&rhdev->dev, "%s%s\n", __func__,
			(auto_stop ? " (auto-stop)" : ""));

	egsm_enable = USBCMD_EGSM;
	uhci->RD_enable = 1;
	int_enable = USBINTR_RESUME;
	wakeup_enable = 1;

	if (auto_stop) {
		if (!device_may_wakeup(&rhdev->dev))
			int_enable = 0;

	} else {
#ifdef CONFIG_PM
		if (!rhdev->do_remote_wakeup)
			wakeup_enable = 0;
#endif
	}

	if (!wakeup_enable || global_suspend_mode_is_broken(uhci))
		egsm_enable = 0;

	if (!wakeup_enable || resume_detect_interrupts_are_broken(uhci) ||
			!int_enable)
		uhci->RD_enable = int_enable = 0;

	outw(int_enable, uhci->io_addr + USBINTR);
	outw(egsm_enable | USBCMD_CF, uhci->io_addr + USBCMD);
	mb();
	udelay(5);

	if (!auto_stop && !(inw(uhci->io_addr + USBSTS) & USBSTS_HCH)) {
		uhci->rh_state = UHCI_RH_SUSPENDING;
		spin_unlock_irq(&uhci->lock);
		msleep(1);
		spin_lock_irq(&uhci->lock);
		if (uhci->dead)
			return;
	}
	if (!(inw(uhci->io_addr + USBSTS) & USBSTS_HCH))
		dev_warn(uhci_dev(uhci), "Controller not stopped yet!\n");

	uhci_get_current_frame_number(uhci);

	uhci->rh_state = new_state;
	uhci->is_stopped = UHCI_IS_STOPPED;

	uhci_to_hcd(uhci)->poll_rh = (!int_enable && wakeup_enable);

	uhci_scan_schedule(uhci);
	uhci_fsbr_off(uhci);
}

static void start_rh(struct uhci_hcd *uhci)
{
	uhci_to_hcd(uhci)->state = HC_STATE_RUNNING;
	uhci->is_stopped = 0;

	outw(USBCMD_RS | USBCMD_CF | USBCMD_MAXP, uhci->io_addr + USBCMD);
	outw(USBINTR_TIMEOUT | USBINTR_RESUME | USBINTR_IOC | USBINTR_SP,
			uhci->io_addr + USBINTR);
	mb();
	uhci->rh_state = UHCI_RH_RUNNING;
	uhci_to_hcd(uhci)->poll_rh = 1;
}

static void wakeup_rh(struct uhci_hcd *uhci)
__releases(uhci->lock)
__acquires(uhci->lock)
{
	dev_dbg(&uhci_to_hcd(uhci)->self.root_hub->dev,
			"%s%s\n", __func__,
			uhci->rh_state == UHCI_RH_AUTO_STOPPED ?
				" (auto-start)" : "");

	if (uhci->rh_state == UHCI_RH_SUSPENDED) {
		unsigned egsm;

		egsm = inw(uhci->io_addr + USBCMD) & USBCMD_EGSM;
		uhci->rh_state = UHCI_RH_RESUMING;
		outw(USBCMD_FGR | USBCMD_CF | egsm, uhci->io_addr + USBCMD);
		spin_unlock_irq(&uhci->lock);
		msleep(20);
		spin_lock_irq(&uhci->lock);
		if (uhci->dead)
			return;

		outw(USBCMD_CF, uhci->io_addr + USBCMD);
		mb();
		udelay(4);
		if (inw(uhci->io_addr + USBCMD) & USBCMD_FGR)
			dev_warn(uhci_dev(uhci), "FGR not stopped yet!\n");
	}

	start_rh(uhci);

	mod_timer(&uhci_to_hcd(uhci)->rh_timer, jiffies);
}

static irqreturn_t uhci_irq(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	unsigned short status;

	status = inw(uhci->io_addr + USBSTS);
	if (!(status & ~USBSTS_HCH))	 
		return IRQ_NONE;
	outw(status, uhci->io_addr + USBSTS);		 

	if (status & ~(USBSTS_USBINT | USBSTS_ERROR | USBSTS_RD)) {
		if (status & USBSTS_HSE)
			dev_err(uhci_dev(uhci), "host system error, "
					"PCI problems?\n");
		if (status & USBSTS_HCPE)
			dev_err(uhci_dev(uhci), "host controller process "
					"error, something bad happened!\n");
		if (status & USBSTS_HCH) {
			spin_lock(&uhci->lock);
			if (uhci->rh_state >= UHCI_RH_RUNNING) {
				dev_err(uhci_dev(uhci),
					"host controller halted, "
					"very bad!\n");
				if (debug > 1 && errbuf) {
					 
					uhci_sprint_schedule(uhci,
							errbuf, ERRBUF_LEN);
					lprintk(errbuf);
				}
				uhci_hc_died(uhci);

				mod_timer(&hcd->rh_timer, jiffies);
			}
			spin_unlock(&uhci->lock);
		}
	}

	if (status & USBSTS_RD)
		usb_hcd_poll_rh_status(hcd);
	else {
		spin_lock(&uhci->lock);
		uhci_scan_schedule(uhci);
		spin_unlock(&uhci->lock);
	}

	return IRQ_HANDLED;
}

static void uhci_get_current_frame_number(struct uhci_hcd *uhci)
{
	if (!uhci->is_stopped) {
		unsigned delta;

		delta = (inw(uhci->io_addr + USBFRNUM) - uhci->frame_number) &
				(UHCI_NUMFRAMES - 1);
		uhci->frame_number += delta;
	}
}

static void release_uhci(struct uhci_hcd *uhci)
{
	int i;

	if (DEBUG_CONFIGURED) {
		spin_lock_irq(&uhci->lock);
		uhci->is_initialized = 0;
		spin_unlock_irq(&uhci->lock);

		debugfs_remove(uhci->dentry);
	}

	for (i = 0; i < UHCI_NUM_SKELQH; i++)
		uhci_free_qh(uhci, uhci->skelqh[i]);

	uhci_free_td(uhci, uhci->term_td);

	dma_pool_destroy(uhci->qh_pool);

	dma_pool_destroy(uhci->td_pool);

	kfree(uhci->frame_cpu);

	dma_free_coherent(uhci_dev(uhci),
			UHCI_NUMFRAMES * sizeof(*uhci->frame),
			uhci->frame, uhci->frame_dma_handle);
}

static int uhci_init(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	unsigned io_size = (unsigned) hcd->rsrc_len;
	int port;

	uhci->io_addr = (unsigned long) hcd->rsrc_start;

	for (port = 0; port < (io_size - USBPORTSC1) / 2; port++) {
		unsigned int portstatus;

		portstatus = inw(uhci->io_addr + USBPORTSC1 + (port * 2));
		if (!(portstatus & 0x0080) || portstatus == 0xffff)
			break;
	}
	if (debug)
		dev_info(uhci_dev(uhci), "detected %d ports\n", port);

	if (port > UHCI_RH_MAXCHILD) {
		dev_info(uhci_dev(uhci), "port count misdetected? "
				"forcing to 2 ports\n");
		port = 2;
	}
	uhci->rh_numports = port;

	check_and_reset_hc(uhci);
	return 0;
}

static void uhci_shutdown(struct pci_dev *pdev)
{
	struct usb_hcd *hcd = (struct usb_hcd *) pci_get_drvdata(pdev);

#ifdef MY_ABC_HERE
	if (hcd->irq >= 0) {
		 
		free_irq(hcd->irq, hcd);
		hcd->irq = -1;
	}
#endif
	uhci_hc_died(hcd_to_uhci(hcd));
}

static int uhci_start(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	int retval = -EBUSY;
	int i;
	struct dentry *dentry;

	hcd->uses_new_polling = 1;

	spin_lock_init(&uhci->lock);
	setup_timer(&uhci->fsbr_timer, uhci_fsbr_timeout,
			(unsigned long) uhci);
	INIT_LIST_HEAD(&uhci->idle_qh_list);
	init_waitqueue_head(&uhci->waitqh);

	if (DEBUG_CONFIGURED) {
		dentry = debugfs_create_file(hcd->self.bus_name,
				S_IFREG|S_IRUGO|S_IWUSR, uhci_debugfs_root,
				uhci, &uhci_debug_operations);
		if (!dentry) {
			dev_err(uhci_dev(uhci), "couldn't create uhci "
					"debugfs entry\n");
			retval = -ENOMEM;
			goto err_create_debug_entry;
		}
		uhci->dentry = dentry;
	}

	uhci->frame = dma_alloc_coherent(uhci_dev(uhci),
			UHCI_NUMFRAMES * sizeof(*uhci->frame),
			&uhci->frame_dma_handle, 0);
	if (!uhci->frame) {
		dev_err(uhci_dev(uhci), "unable to allocate "
				"consistent memory for frame list\n");
		goto err_alloc_frame;
	}
	memset(uhci->frame, 0, UHCI_NUMFRAMES * sizeof(*uhci->frame));

	uhci->frame_cpu = kcalloc(UHCI_NUMFRAMES, sizeof(*uhci->frame_cpu),
			GFP_KERNEL);
	if (!uhci->frame_cpu) {
		dev_err(uhci_dev(uhci), "unable to allocate "
				"memory for frame pointers\n");
		goto err_alloc_frame_cpu;
	}

	uhci->td_pool = dma_pool_create("uhci_td", uhci_dev(uhci),
			sizeof(struct uhci_td), 16, 0);
	if (!uhci->td_pool) {
		dev_err(uhci_dev(uhci), "unable to create td dma_pool\n");
		goto err_create_td_pool;
	}

	uhci->qh_pool = dma_pool_create("uhci_qh", uhci_dev(uhci),
			sizeof(struct uhci_qh), 16, 0);
	if (!uhci->qh_pool) {
		dev_err(uhci_dev(uhci), "unable to create qh dma_pool\n");
		goto err_create_qh_pool;
	}

	uhci->term_td = uhci_alloc_td(uhci);
	if (!uhci->term_td) {
		dev_err(uhci_dev(uhci), "unable to allocate terminating TD\n");
		goto err_alloc_term_td;
	}

	for (i = 0; i < UHCI_NUM_SKELQH; i++) {
		uhci->skelqh[i] = uhci_alloc_qh(uhci, NULL, NULL);
		if (!uhci->skelqh[i]) {
			dev_err(uhci_dev(uhci), "unable to allocate QH\n");
			goto err_alloc_skelqh;
		}
	}

	for (i = SKEL_ISO + 1; i < SKEL_ASYNC; ++i)
		uhci->skelqh[i]->link = LINK_TO_QH(uhci->skel_async_qh);
	uhci->skel_async_qh->link = UHCI_PTR_TERM;
	uhci->skel_term_qh->link = LINK_TO_QH(uhci->skel_term_qh);

	uhci_fill_td(uhci->term_td, 0, uhci_explen(0) |
			(0x7f << TD_TOKEN_DEVADDR_SHIFT) | USB_PID_IN, 0);
	uhci->term_td->link = UHCI_PTR_TERM;
	uhci->skel_async_qh->element = uhci->skel_term_qh->element =
			LINK_TO_TD(uhci->term_td);

	for (i = 0; i < UHCI_NUMFRAMES; i++) {

		uhci->frame[i] = uhci_frame_skel_link(uhci, i);
	}

	mb();

	configure_hc(uhci);
	uhci->is_initialized = 1;
	start_rh(uhci);
	return 0;

err_alloc_skelqh:
	for (i = 0; i < UHCI_NUM_SKELQH; i++) {
		if (uhci->skelqh[i])
			uhci_free_qh(uhci, uhci->skelqh[i]);
	}

	uhci_free_td(uhci, uhci->term_td);

err_alloc_term_td:
	dma_pool_destroy(uhci->qh_pool);

err_create_qh_pool:
	dma_pool_destroy(uhci->td_pool);

err_create_td_pool:
	kfree(uhci->frame_cpu);

err_alloc_frame_cpu:
	dma_free_coherent(uhci_dev(uhci),
			UHCI_NUMFRAMES * sizeof(*uhci->frame),
			uhci->frame, uhci->frame_dma_handle);

err_alloc_frame:
	debugfs_remove(uhci->dentry);

err_create_debug_entry:
	return retval;
}

static void uhci_stop(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);

	spin_lock_irq(&uhci->lock);
	if (test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags) && !uhci->dead)
		uhci_hc_died(uhci);
	uhci_scan_schedule(uhci);
	spin_unlock_irq(&uhci->lock);
	synchronize_irq(hcd->irq);

	del_timer_sync(&uhci->fsbr_timer);
	release_uhci(uhci);
}

#ifdef CONFIG_PM
static int uhci_rh_suspend(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	int rc = 0;

	spin_lock_irq(&uhci->lock);
	if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags))
		rc = -ESHUTDOWN;
	else if (uhci->dead)
		;		 

	else if (hcd->self.root_hub->do_remote_wakeup &&
			uhci->resuming_ports) {
		dev_dbg(uhci_dev(uhci), "suspend failed because a port "
				"is resuming\n");
		rc = -EBUSY;
	} else
		suspend_rh(uhci, UHCI_RH_SUSPENDED);
	spin_unlock_irq(&uhci->lock);
	return rc;
}

static int uhci_rh_resume(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	int rc = 0;

	spin_lock_irq(&uhci->lock);
	if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags))
		rc = -ESHUTDOWN;
	else if (!uhci->dead)
		wakeup_rh(uhci);
	spin_unlock_irq(&uhci->lock);
	return rc;
}

static int uhci_pci_suspend(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	int rc = 0;

	dev_dbg(uhci_dev(uhci), "%s\n", __func__);

	spin_lock_irq(&uhci->lock);
	if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags) || uhci->dead)
		goto done_okay;		 

	if (uhci->rh_state > UHCI_RH_SUSPENDED) {
		dev_warn(uhci_dev(uhci), "Root hub isn't suspended!\n");
		rc = -EBUSY;
		goto done;
	};

	pci_write_config_word(to_pci_dev(uhci_dev(uhci)), USBLEGSUP, 0);
	mb();
	hcd->poll_rh = 0;

done_okay:
	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
done:
	spin_unlock_irq(&uhci->lock);
	return rc;
}

static int uhci_pci_resume(struct usb_hcd *hcd, bool hibernated)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);

	dev_dbg(uhci_dev(uhci), "%s\n", __func__);

	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	mb();

	spin_lock_irq(&uhci->lock);

	if (hibernated)
		uhci_hc_died(uhci);

	check_and_reset_hc(uhci);

	configure_hc(uhci);

	if (uhci->rh_state == UHCI_RH_RESET) {

		usb_root_hub_lost_power(hcd->self.root_hub);
		suspend_rh(uhci, UHCI_RH_SUSPENDED);
	}

	spin_unlock_irq(&uhci->lock);

	if (!uhci->RD_enable && hcd->self.root_hub->do_remote_wakeup) {
		hcd->poll_rh = 1;
		usb_hcd_poll_rh_status(hcd);
	}
	return 0;
}
#endif

static void uhci_hcd_endpoint_disable(struct usb_hcd *hcd,
		struct usb_host_endpoint *hep)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	struct uhci_qh *qh;

	spin_lock_irq(&uhci->lock);
	qh = (struct uhci_qh *) hep->hcpriv;
	if (qh == NULL)
		goto done;

	while (qh->state != QH_STATE_IDLE) {
		++uhci->num_waiting;
		spin_unlock_irq(&uhci->lock);
		wait_event_interruptible(uhci->waitqh,
				qh->state == QH_STATE_IDLE);
		spin_lock_irq(&uhci->lock);
		--uhci->num_waiting;
	}

	uhci_free_qh(uhci, qh);
done:
	spin_unlock_irq(&uhci->lock);
}

static int uhci_hcd_get_frame_number(struct usb_hcd *hcd)
{
	struct uhci_hcd *uhci = hcd_to_uhci(hcd);
	unsigned frame_number;
	unsigned delta;

	frame_number = uhci->frame_number;
	barrier();
	delta = (inw(uhci->io_addr + USBFRNUM) - frame_number) &
			(UHCI_NUMFRAMES - 1);
	return frame_number + delta;
}

static const char hcd_name[] = "uhci_hcd";

static const struct hc_driver uhci_driver = {
	.description =		hcd_name,
	.product_desc =		"UHCI Host Controller",
	.hcd_priv_size =	sizeof(struct uhci_hcd),

	.irq =			uhci_irq,
	.flags =		HCD_USB11,

	.reset =		uhci_init,
	.start =		uhci_start,
#ifdef CONFIG_PM
	.pci_suspend =		uhci_pci_suspend,
	.pci_resume =		uhci_pci_resume,
	.bus_suspend =		uhci_rh_suspend,
	.bus_resume =		uhci_rh_resume,
#endif
	.stop =			uhci_stop,

	.urb_enqueue =		uhci_urb_enqueue,
	.urb_dequeue =		uhci_urb_dequeue,

	.endpoint_disable =	uhci_hcd_endpoint_disable,
	.get_frame_number =	uhci_hcd_get_frame_number,

	.hub_status_data =	uhci_hub_status_data,
	.hub_control =		uhci_hub_control,
};

static const struct pci_device_id uhci_pci_ids[] = { {
	 
	PCI_DEVICE_CLASS(PCI_CLASS_SERIAL_USB_UHCI, ~0),
	.driver_data =	(unsigned long) &uhci_driver,
	}, {   }
};

MODULE_DEVICE_TABLE(pci, uhci_pci_ids);

static struct pci_driver uhci_pci_driver = {
	.name =		(char *)hcd_name,
	.id_table =	uhci_pci_ids,

	.probe =	usb_hcd_pci_probe,
	.remove =	usb_hcd_pci_remove,
	.shutdown =	uhci_shutdown,

#ifdef CONFIG_PM_SLEEP
	.driver =	{
		.pm =	&usb_hcd_pci_pm_ops
	},
#endif
};
 
static int __init uhci_hcd_init(void)
{
	int retval = -ENOMEM;

	if (usb_disabled())
		return -ENODEV;

	printk(KERN_INFO "uhci_hcd: " DRIVER_DESC "%s\n",
			ignore_oc ? ", overcurrent ignored" : "");
	set_bit(USB_UHCI_LOADED, &usb_hcds_loaded);

	if (DEBUG_CONFIGURED) {
		errbuf = kmalloc(ERRBUF_LEN, GFP_KERNEL);
		if (!errbuf)
			goto errbuf_failed;
		uhci_debugfs_root = debugfs_create_dir("uhci", usb_debug_root);
		if (!uhci_debugfs_root)
			goto debug_failed;
	}

	uhci_up_cachep = kmem_cache_create("uhci_urb_priv",
		sizeof(struct urb_priv), 0, 0, NULL);
	if (!uhci_up_cachep)
		goto up_failed;

	retval = pci_register_driver(&uhci_pci_driver);
	if (retval)
		goto init_failed;

	return 0;

init_failed:
	kmem_cache_destroy(uhci_up_cachep);

up_failed:
	debugfs_remove(uhci_debugfs_root);

debug_failed:
	kfree(errbuf);

errbuf_failed:

	clear_bit(USB_UHCI_LOADED, &usb_hcds_loaded);
	return retval;
}

static void __exit uhci_hcd_cleanup(void) 
{
	pci_unregister_driver(&uhci_pci_driver);
	kmem_cache_destroy(uhci_up_cachep);
	debugfs_remove(uhci_debugfs_root);
	kfree(errbuf);
	clear_bit(USB_UHCI_LOADED, &usb_hcds_loaded);
}

module_init(uhci_hcd_init);
module_exit(uhci_hcd_cleanup);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

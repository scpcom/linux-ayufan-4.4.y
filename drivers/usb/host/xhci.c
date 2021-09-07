#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/irq.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>

#include "xhci.h"

#define DRIVER_AUTHOR "Sarah Sharp"
#define DRIVER_DESC "'eXtensible' Host Controller (xHC) Driver"

static int link_quirk;
module_param(link_quirk, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(link_quirk, "Don't clear the chain bit on a link TRB");

#ifdef MY_ABC_HERE
#include <linux/kthread.h>
#include <linux/pci.h>

static struct task_struct *xhci_task = NULL;
static struct usb_hcd *xhci_task_hcd = NULL;  

#define BOUNCE_TIME 10  

#ifdef MY_ABC_HERE
extern enum XHCI_SPECIAL_RESET_MODE xhci_special_reset;  
extern unsigned short xhci_vendor;

#define SKIP_SPECIAL_RESET_BEFORE 60  

#define XHCI_PORT_NUM 4  
 
static bool inline IS_USB3(unsigned int x)
{
	if ((xhci_vendor == PCI_VENDOR_ID_NEC) &&
		(x == 1 || x == 2)) {
		return true;
	} else if ((xhci_vendor == PCI_VENDOR_ID_ETRON) &&
		(x == 3 || x == 4)) {
		return true;
	} else {
		return false;
	}
}

static bool inline IS_USB2(unsigned int x)
{
	return !IS_USB3(x);
}

#endif

enum STATUS_MONITOR {
	STATUS_MON_TEST = 0x1,
	STATUS_MON_RESET = 0x2,
	STATUS_MON_BH_RESET = 0x4,
	STATUS_MON_CONNECTION = 0x8,
	STATUS_MON_SS_DISABLED = 0x10,
	STATUS_MON_CEC = 0x20,
	STATUS_MON_CC_RESET_BH = 0x40,
	STATUS_MON_POWER = 0x80,
	STATUS_MON_ENABLE = 0x100,
	STATUS_MON_CC_RESET_BH_EN = 0x200,
	STATUS_MON_POLLING = 0x400,
};  

enum PORT_MONITOR {
	PORT_USB2_3 = 0,  
	PORT_MONITOR_MAX = 1,
};

static int xhci_thread(void *__unused)
{
	uint count = 0;
	const uint count_check = BOUNCE_TIME/2-1;
	u32 status = 0;
	uint i = 0, j = 0, k = 0;
	struct usb_device *hdev[PORT_MONITOR_MAX];  
	struct usb_hcd *hcd[PORT_MONITOR_MAX];
	uint port_num[PORT_MONITOR_MAX];
	u16 bounce[PORT_MONITOR_MAX][4];  

	for (j = 0; j < PORT_MONITOR_MAX; j++) {
		hdev[j] = NULL;
		hcd[j] = NULL;
		port_num[j] = 0;
	}
	memset(&bounce, 0, sizeof(bounce));

	if (xhci_task_hcd) {
		hcd[PORT_USB2_3] = xhci_task_hcd;
		hdev[PORT_USB2_3] = xhci_task_hcd->self.root_hub;
		port_num[PORT_USB2_3] = XHCI_PORT_NUM;
	}

#ifdef MY_ABC_HERE
	for (k = 0; k < SKIP_SPECIAL_RESET_BEFORE; k++) {
		if (kthread_should_stop()) {
			printk(KERN_INFO "xhci_thread. exit!\n");
			return 0;
		}
		msleep(1000);
	}
#endif

	while(1) {
		if (kthread_should_stop()) {
			break;
		}

		if (count >= count_check) {

			if(xhci_task_hcd) {

				for (j = 0; j< PORT_MONITOR_MAX; j++) {
					usb_lock_device(hdev[j]);
					for (i = 1; i <= port_num[j]; i++) {
						status = 0;
						xhci_hub_control(hcd[j], GetPortStatus, 0, i, (void*) &status, 0);
						status = le32_to_cpu(status);
						dbg("xhci_thread. usb%d: 0x%x.\n", i-1, status);

						if (status & USB_PORT_STAT_TEST_MODE) {
							printk(KERN_DEBUG "xhci_thread. usb%d test mode.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_TEST) {
								printk(KERN_DEBUG "xhci_thread. usb%d test mode. reset!\n", i-1);
								xhci_hub_control(hcd[j], SetPortFeature, USB_PORT_FEAT_RESET, i, 0, 0);
								 
								xhci_hub_control(hcd[j], SetPortFeature, USB_PORT_FEAT_RESET, (i+2-1)%XHCI_PORT_NUM+1, 0, 0);
								 
								bounce[j][i-1] &= ~STATUS_MON_TEST;  
							} else {
								bounce[j][i-1] |= STATUS_MON_TEST;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_TEST;  
						}

						if ((status & USB_PORT_STAT_C_RESET << 16) && 
											!(status & USB_PORT_STAT_CONNECTION)) {
							printk(KERN_DEBUG "xhci_thread. usb%d reset change.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_RESET) {
								printk(KERN_DEBUG "xhci_thread. usb%d reset change. clear!\n", i-1);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_C_RESET, i, 0, 0);
								bounce[j][i-1] &= ~STATUS_MON_RESET;  
							} else {
								bounce[j][i-1] |= STATUS_MON_RESET;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_RESET;  
						}

						if ((status & (USB_PORT_STAT_C_BH_RESET << 16)) && 
											!(status & USB_PORT_STAT_CONNECTION)) {
							printk(KERN_DEBUG "xhci_thread. usb%d bh reset change.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_BH_RESET) {
								printk(KERN_DEBUG "xhci_thread. usb%d bh reset change. clear.\n", i-1);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_C_BH_PORT_RESET, i, 0, 0);
								bounce[j][i-1] &= ~STATUS_MON_BH_RESET;  
							} else {
								bounce[j][i-1] |= STATUS_MON_BH_RESET;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_BH_RESET;  
						}

						if ((status & (USB_PORT_STAT_C_CONNECTION << 16)) && 
											!(status & USB_PORT_STAT_CONNECTION)) {
							printk(KERN_DEBUG "xhci_thread. usb%d connect change.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_CONNECTION) {
								printk(KERN_DEBUG "xhci_thread. usb%d connect change. clear.\n", i-1);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_C_CONNECTION, i, 0, 0);
								bounce[j][i-1] &= ~STATUS_MON_CONNECTION;  
							} else {
								bounce[j][i-1] |= STATUS_MON_CONNECTION;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_CONNECTION;  
						}

						if (status == USB_SS_PORT_LS_SS_DISABLED ||
								(IS_USB2(i) && status == (USB_SS_PORT_LS_SS_DISABLED | USB_PORT_STAT_POWER)) ||
								(IS_USB3(i) && status == (USB_SS_PORT_LS_SS_DISABLED | USB_SS_PORT_STAT_POWER))) {
							printk(KERN_DEBUG "xhci_thread. usb%d ss disabled.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_SS_DISABLED) {
								printk(KERN_DEBUG "xhci_thread. usb%d ss disabled. set power.\n", i-1);
								xhci_hub_control(hcd[j], SetPortFeature, USB_PORT_FEAT_POWER, i, 0, 0);
								bounce[j][i-1] &= ~STATUS_MON_SS_DISABLED;  
							} else {
								bounce[j][i-1] |= STATUS_MON_SS_DISABLED;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_SS_DISABLED;  
						}

						if ((status & (PORT_PLC | PORT_CEC)) &&
								!(status & USB_PORT_STAT_CONNECTION)) { 
							printk(KERN_DEBUG "xhci_thread. usb%d plc/cec error.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_CEC) {
								printk(KERN_DEBUG "xhci_thread. usb%d plc/cec error. clear.\n", i-1);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_C_CONNECTION, i, 0, 0);
								bounce[j][i-1] &= ~STATUS_MON_CEC;  
							} else {
								bounce[j][i-1] |= STATUS_MON_CEC;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_CEC;  
						}

#ifdef MY_ABC_HERE
						 
						if ((status & USB_PORT_STAT_C_RESET << 16) && 
								(status & USB_PORT_STAT_C_BH_RESET << 16) && 
								(status & USB_PORT_STAT_C_CONNECTION << 16) && 
								(status & USB_PORT_STAT_CONNECTION)) {
							printk(KERN_DEBUG "xhci_thread. usb%d cc/reset/bh error.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_CC_RESET_BH) {
								printk(KERN_DEBUG "xhci_thread. usb%d cc/reset/bh. re-power.\n", i-1);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_C_BH_PORT_RESET, i, 0, 0);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_C_RESET, i, 0, 0);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_C_CONNECTION, i, 0, 0);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_POWER, i, 0, 0);
								 
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_POWER, (i+2-1)%XHCI_PORT_NUM+1, 0, 0);
								 
								bounce[j][i-1] &= ~STATUS_MON_CC_RESET_BH;  
							} else {
								bounce[j][i-1] |= STATUS_MON_CC_RESET_BH;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_CC_RESET_BH;  
						}

						if ((IS_USB2(i) && !(status & USB_PORT_STAT_POWER)) ||
								(IS_USB3(i) && !(status & USB_SS_PORT_STAT_POWER))) {
							printk(KERN_DEBUG "xhci_thread. usb%d power error.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_POWER) {
								printk(KERN_DEBUG "xhci_thread. usb%d power error. set.\n", i-1);
								xhci_hub_control(hcd[j], SetPortFeature, USB_PORT_FEAT_POWER, i, 0, 0);
								bounce[j][i-1] &= ~STATUS_MON_POWER;  
							} else {
								bounce[j][i-1] |= STATUS_MON_POWER;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_POWER;  
						}

						if ((IS_USB2(i) && (status == (USB_PORT_STAT_POWER | USB_PORT_STAT_CONNECTION))) ||
								(IS_USB3(i) && (status == (USB_SS_PORT_STAT_POWER | USB_PORT_STAT_CONNECTION)))) {
							printk(KERN_DEBUG "xhci_thread. usb%d enable error.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_ENABLE) {
								printk(KERN_DEBUG "xhci_thread. usb%d enable error. set.\n", i-1);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_POWER, i, 0, 0);
								 
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_POWER, (i+2-1)%XHCI_PORT_NUM+1, 0, 0);
								 
								bounce[j][i-1] &= ~STATUS_MON_ENABLE;  
							} else {
								bounce[j][i-1] |= STATUS_MON_ENABLE;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_ENABLE;  
						}

						if ((IS_USB2(i) && (status == (USB_PORT_STAT_POWER | USB_SS_PORT_LS_POLLING))) ||
								(IS_USB3(i) && (status == (USB_SS_PORT_STAT_POWER | USB_SS_PORT_LS_POLLING)))) {
							printk(KERN_DEBUG "xhci_thread. usb%d polling error.\n", i-1);
							if (bounce[j][i-1] & STATUS_MON_POLLING) {
								printk(KERN_DEBUG "xhci_thread. usb%d polling error. reset.\n", i-1);
								xhci_hub_control(hcd[j], ClearPortFeature, USB_PORT_FEAT_POWER, i, 0, 0);
								bounce[j][i-1] &= ~STATUS_MON_POLLING;  
							} else {
								bounce[j][i-1] |= STATUS_MON_POLLING;  
							}
						} else {
							bounce[j][i-1] &= ~STATUS_MON_POLLING;  
						}

#endif

					}
					usb_unlock_device(hdev[j]);

				}

			}
			count = 0;

		}
		if (kthread_should_stop()) {
			break;
		}
		count++;
		k++;
		msleep(1000);
	}

	printk(KERN_INFO "xhci_thread. exit!\n");

	return 0;
}
#endif

static int handshake(struct xhci_hcd *xhci, void __iomem *ptr,
		      u32 mask, u32 done, int usec)
{
	u32	result;

	do {
		result = xhci_readl(xhci, ptr);
		if (result == ~(u32)0)		 
			return -ENODEV;
		result &= mask;
		if (result == done)
			return 0;
		udelay(1);
		usec--;
	} while (usec > 0);
	return -ETIMEDOUT;
}

void xhci_quiesce(struct xhci_hcd *xhci)
{
	u32 halted;
	u32 cmd;
	u32 mask;

	mask = ~(XHCI_IRQS);
	halted = xhci_readl(xhci, &xhci->op_regs->status) & STS_HALT;
	if (!halted)
		mask &= ~CMD_RUN;

	cmd = xhci_readl(xhci, &xhci->op_regs->command);
	cmd &= mask;
	xhci_writel(xhci, cmd, &xhci->op_regs->command);
}

int xhci_halt(struct xhci_hcd *xhci)
{
	xhci_dbg(xhci, "// Halt the HC\n");
	xhci_quiesce(xhci);

	return handshake(xhci, &xhci->op_regs->status,
			STS_HALT, STS_HALT, XHCI_MAX_HALT_USEC);
}

int xhci_reset(struct xhci_hcd *xhci)
{
	u32 command;
	u32 state;

	state = xhci_readl(xhci, &xhci->op_regs->status);
	if ((state & STS_HALT) == 0) {
#ifdef MY_ABC_HERE
		xhci_warn(xhci, "Host controller not halted, try halt again.\n");
		msleep(1000);
		xhci_halt(xhci);
		msleep(1000);
		state = xhci_readl(xhci, &xhci->op_regs->status);
		if ((state & STS_HALT) == 0) {
			xhci_warn(xhci, "Host controller not halted, aborting reset!\n");
			return 0;
		}
#else
		xhci_warn(xhci, "Host controller not halted, aborting reset.\n");
		return 0;
#endif
	}

	xhci_dbg(xhci, "// Reset the HC\n");
	command = xhci_readl(xhci, &xhci->op_regs->command);
	command |= CMD_RESET;
	xhci_writel(xhci, command, &xhci->op_regs->command);
	 
	xhci_to_hcd(xhci)->state = HC_STATE_HALT;

#ifdef MY_ABC_HERE
	return handshake(xhci, &xhci->op_regs->command, CMD_RESET, 0, 1000 * 1000);
#else
	return handshake(xhci, &xhci->op_regs->command, CMD_RESET, 0, 250 * 1000);
#endif
}

#if 0
 
static int xhci_setup_msix(struct xhci_hcd *xhci)
{
	int ret;
	struct pci_dev *pdev = to_pci_dev(xhci_to_hcd(xhci)->self.controller);

	xhci->msix_count = 0;
	 
	xhci->msix_entries = kmalloc(sizeof(struct msix_entry), GFP_KERNEL);
	if (!xhci->msix_entries) {
		xhci_err(xhci, "Failed to allocate MSI-X entries\n");
		return -ENOMEM;
	}
	xhci->msix_entries[0].entry = 0;

	ret = pci_enable_msix(pdev, xhci->msix_entries, xhci->msix_count);
	if (ret) {
		xhci_err(xhci, "Failed to enable MSI-X\n");
		goto free_entries;
	}

	ret = request_irq(xhci->msix_entries[0].vector, &xhci_irq, 0,
			"xHCI", xhci_to_hcd(xhci));
	if (ret) {
		xhci_err(xhci, "Failed to allocate MSI-X interrupt\n");
		goto disable_msix;
	}
	xhci_dbg(xhci, "Finished setting up MSI-X\n");
	return 0;

disable_msix:
	pci_disable_msix(pdev);
free_entries:
	kfree(xhci->msix_entries);
	xhci->msix_entries = NULL;
	return ret;
}

static void xhci_cleanup_msix(struct xhci_hcd *xhci)
{
	struct pci_dev *pdev = to_pci_dev(xhci_to_hcd(xhci)->self.controller);
	if (!xhci->msix_entries)
		return;

	free_irq(xhci->msix_entries[0].vector, xhci);
	pci_disable_msix(pdev);
	kfree(xhci->msix_entries);
	xhci->msix_entries = NULL;
	xhci_dbg(xhci, "Finished cleaning up MSI-X\n");
}
#endif

int xhci_init(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	int retval = 0;

	xhci_dbg(xhci, "xhci_init\n");
	spin_lock_init(&xhci->lock);
	if (link_quirk) {
		xhci_dbg(xhci, "QUIRK: Not clearing Link TRB chain bits.\n");
		xhci->quirks |= XHCI_LINK_TRB_QUIRK;
	} else {
		xhci_dbg(xhci, "xHCI doesn't need link TRB QUIRK\n");
	}
	retval = xhci_mem_init(xhci, GFP_KERNEL);
	xhci_dbg(xhci, "Finished xhci_init\n");

	return retval;
}

static void xhci_work(struct xhci_hcd *xhci)
{
	u32 temp;
	u64 temp_64;

	temp = xhci_readl(xhci, &xhci->op_regs->status);
	temp |= STS_EINT;
	xhci_writel(xhci, temp, &xhci->op_regs->status);
	 
	temp = xhci_readl(xhci, &xhci->ir_set->irq_pending);
	temp |= 0x3;
	xhci_writel(xhci, temp, &xhci->ir_set->irq_pending);
	 
	xhci_readl(xhci, &xhci->ir_set->irq_pending);

	if (xhci->xhc_state & XHCI_STATE_DYING)
		xhci_dbg(xhci, "xHCI dying, ignoring interrupt. "
				"Shouldn't IRQs be disabled?\n");
	else
		 
		xhci_handle_event(xhci);

	temp_64 = xhci_read_64(xhci, &xhci->ir_set->erst_dequeue);
	xhci_write_64(xhci, temp_64 | ERST_EHB, &xhci->ir_set->erst_dequeue);
	 
	xhci_readl(xhci, &xhci->ir_set->irq_pending);
}

#ifdef CONFIG_USB_XHCI_HCD_DEBUGGING
void xhci_event_ring_work(unsigned long arg)
{
	unsigned long flags;
	int temp;
	u64 temp_64;
	struct xhci_hcd *xhci = (struct xhci_hcd *) arg;
	int i, j;

	xhci_dbg(xhci, "Poll event ring: %lu\n", jiffies);

	spin_lock_irqsave(&xhci->lock, flags);
	temp = xhci_readl(xhci, &xhci->op_regs->status);
	xhci_dbg(xhci, "op reg status = 0x%x\n", temp);
	if (temp == 0xffffffff || (xhci->xhc_state & XHCI_STATE_DYING)) {
		xhci_dbg(xhci, "HW died, polling stopped.\n");
		spin_unlock_irqrestore(&xhci->lock, flags);
		return;
	}

	temp = xhci_readl(xhci, &xhci->ir_set->irq_pending);
	xhci_dbg(xhci, "ir_set 0 pending = 0x%x\n", temp);
	xhci_dbg(xhci, "No-op commands handled = %d\n", xhci->noops_handled);
	xhci_dbg(xhci, "HC error bitmask = 0x%x\n", xhci->error_bitmask);
	xhci->error_bitmask = 0;
	xhci_dbg(xhci, "Event ring:\n");
	xhci_debug_segment(xhci, xhci->event_ring->deq_seg);
	xhci_dbg_ring_ptrs(xhci, xhci->event_ring);
	temp_64 = xhci_read_64(xhci, &xhci->ir_set->erst_dequeue);
	temp_64 &= ~ERST_PTR_MASK;
	xhci_dbg(xhci, "ERST deq = 64'h%0lx\n", (long unsigned int) temp_64);
	xhci_dbg(xhci, "Command ring:\n");
	xhci_debug_segment(xhci, xhci->cmd_ring->deq_seg);
	xhci_dbg_ring_ptrs(xhci, xhci->cmd_ring);
	xhci_dbg_cmd_ptrs(xhci);
	for (i = 0; i < MAX_HC_SLOTS; ++i) {
		if (!xhci->devs[i])
			continue;
		for (j = 0; j < 31; ++j) {
			xhci_dbg_ep_rings(xhci, i, j, &xhci->devs[i]->eps[j]);
		}
	}

	if (xhci->noops_submitted != NUM_TEST_NOOPS)
		if (xhci_setup_one_noop(xhci))
			xhci_ring_cmd_db(xhci);
	spin_unlock_irqrestore(&xhci->lock, flags);

	if (!xhci->zombie)
		mod_timer(&xhci->event_ring_timer, jiffies + POLL_TIMEOUT * HZ);
	else
		xhci_dbg(xhci, "Quit polling the event ring.\n");
}
#endif

#ifdef MY_ABC_HERE
extern int gSynoFactoryUSBFastReset;
extern unsigned int blk_timeout_factory;  
#endif

#ifdef MY_ABC_HERE
extern int gSynoFactoryUSB3Disable;
#endif

int xhci_run(struct usb_hcd *hcd)
{
	u32 temp;
	u64 temp_64;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	void (*doorbell)(struct xhci_hcd *) = NULL;

	hcd->uses_new_polling = 1;
	hcd->poll_rh = 0;

	xhci_dbg(xhci, "xhci_run\n");
#if 0	 
	 
	ret = xhci_setup_msix(xhci);
	if (!ret)
		return ret;

	return -ENOSYS;
#endif
#ifdef CONFIG_USB_XHCI_HCD_DEBUGGING
	init_timer(&xhci->event_ring_timer);
	xhci->event_ring_timer.data = (unsigned long) xhci;
	xhci->event_ring_timer.function = xhci_event_ring_work;
	 
	xhci->event_ring_timer.expires = jiffies + POLL_TIMEOUT * HZ;
	xhci->zombie = 0;
	xhci_dbg(xhci, "Setting event ring polling timer\n");
	add_timer(&xhci->event_ring_timer);
#endif

	xhci_dbg(xhci, "Command ring memory map follows:\n");
	xhci_debug_ring(xhci, xhci->cmd_ring);
	xhci_dbg_ring_ptrs(xhci, xhci->cmd_ring);
	xhci_dbg_cmd_ptrs(xhci);

	xhci_dbg(xhci, "ERST memory map follows:\n");
	xhci_dbg_erst(xhci, &xhci->erst);
	xhci_dbg(xhci, "Event ring:\n");
	xhci_debug_ring(xhci, xhci->event_ring);
	xhci_dbg_ring_ptrs(xhci, xhci->event_ring);
	temp_64 = xhci_read_64(xhci, &xhci->ir_set->erst_dequeue);
	temp_64 &= ~ERST_PTR_MASK;
	xhci_dbg(xhci, "ERST deq = 64'h%0lx\n", (long unsigned int) temp_64);

	xhci_dbg(xhci, "// Set the interrupt modulation register\n");
	temp = xhci_readl(xhci, &xhci->ir_set->irq_control);
	temp &= ~ER_IRQ_INTERVAL_MASK;
	temp |= (u32) 160;
	xhci_writel(xhci, temp, &xhci->ir_set->irq_control);

	hcd->state = HC_STATE_RUNNING;
	temp = xhci_readl(xhci, &xhci->op_regs->command);
	temp |= (CMD_EIE);
	xhci_dbg(xhci, "// Enable interrupts, cmd = 0x%x.\n",
			temp);
	xhci_writel(xhci, temp, &xhci->op_regs->command);

	temp = xhci_readl(xhci, &xhci->ir_set->irq_pending);
	xhci_dbg(xhci, "// Enabling event ring interrupter %p by writing 0x%x to irq_pending\n",
			xhci->ir_set, (unsigned int) ER_IRQ_ENABLE(temp));
	xhci_writel(xhci, ER_IRQ_ENABLE(temp),
			&xhci->ir_set->irq_pending);
	xhci_print_ir_set(xhci, xhci->ir_set, 0);

	if (NUM_TEST_NOOPS > 0)
		doorbell = xhci_setup_one_noop(xhci);
	if (xhci->quirks & XHCI_NEC_HOST)
		xhci_queue_vendor_command(xhci, 0, 0, 0,
				TRB_TYPE(TRB_NEC_GET_FW));

	temp = xhci_readl(xhci, &xhci->op_regs->command);
	temp |= (CMD_RUN);
	xhci_dbg(xhci, "// Turn on HC, cmd = 0x%x.\n",
			temp);
	xhci_writel(xhci, temp, &xhci->op_regs->command);
	 
	temp = xhci_readl(xhci, &xhci->op_regs->command);
	xhci_dbg(xhci, "// @%p = 0x%x\n", &xhci->op_regs->command, temp);
	if (doorbell)
		(*doorbell)(xhci);
	if (xhci->quirks & XHCI_NEC_HOST)
		xhci_ring_cmd_db(xhci);

	xhci_dbg(xhci, "Finished xhci_run\n");

#ifdef MY_ABC_HERE
	if (1 == gSynoFactoryUSB3Disable) {
		printk("xhci USB3 ports are disabled!\n");
	}
#endif

#ifdef MY_ABC_HERE
	if (1 == gSynoFactoryUSBFastReset) {
		printk("USB_FAST_RESET enabled!\n");
		blk_timeout_factory = 1;
	}
#endif

#ifdef MY_ABC_HERE
#ifdef MY_ABC_HERE
	xhci_task_hcd = hcd;
	if(1 == gSynoFactoryUSB3Disable) {
		xhci_special_reset = XHCI_SPECIAL_RESET_DISABLE;
	} else {  
		xhci_special_reset = XHCI_SPECIAL_RESET_PAUSE;
		if(PCI_VENDOR_ID_ETRON != xhci_vendor) {
			xhci_task = kthread_run(xhci_thread, NULL, "xhci_thread");
		}
	}
#else
	xhci_task = kthread_run(xhci_thread, NULL, "xhci_thread");
#endif
#endif

	return 0;
}

void xhci_stop(struct usb_hcd *hcd)
{
	u32 temp;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

#ifdef MY_ABC_HERE
	if (xhci_task) {
		kthread_stop(xhci_task);
		xhci_task = NULL;
	}
	xhci_task_hcd = NULL;
#endif

	spin_lock_irq(&xhci->lock);
	xhci_halt(xhci);
	xhci_reset(xhci);
	spin_unlock_irq(&xhci->lock);

#if 0	 
	xhci_cleanup_msix(xhci);
#endif
#ifdef CONFIG_USB_XHCI_HCD_DEBUGGING
	 
	xhci->zombie = 1;
	del_timer_sync(&xhci->event_ring_timer);
#endif

	xhci_dbg(xhci, "// Disabling event ring interrupts\n");
	temp = xhci_readl(xhci, &xhci->op_regs->status);
	xhci_writel(xhci, temp & ~STS_EINT, &xhci->op_regs->status);
	temp = xhci_readl(xhci, &xhci->ir_set->irq_pending);
	xhci_writel(xhci, ER_IRQ_DISABLE(temp),
			&xhci->ir_set->irq_pending);
	xhci_print_ir_set(xhci, xhci->ir_set, 0);

	xhci_dbg(xhci, "cleaning up memory\n");
	xhci_mem_cleanup(xhci);
	xhci_dbg(xhci, "xhci_stop completed - status = %x\n",
		    xhci_readl(xhci, &xhci->op_regs->status));

#ifdef MY_ABC_HERE
	if (1 == gSynoFactoryUSBFastReset) {
		printk("USB_FAST_RESET disabled!\n");
		blk_timeout_factory = 0;
	}
#endif

}

void xhci_shutdown(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

#ifdef MY_ABC_HERE
	if (xhci_task) {
		kthread_stop(xhci_task);
		xhci_task = NULL;
	}
#endif

	spin_lock_irq(&xhci->lock);
	xhci_halt(xhci);
	spin_unlock_irq(&xhci->lock);

#if 0
	xhci_cleanup_msix(xhci);
#endif

	xhci_dbg(xhci, "xhci_shutdown completed - status = %x\n",
		    xhci_readl(xhci, &xhci->op_regs->status));
}

unsigned int xhci_get_endpoint_index(struct usb_endpoint_descriptor *desc)
{
	unsigned int index;
	if (usb_endpoint_xfer_control(desc))
		index = (unsigned int) (usb_endpoint_num(desc)*2);
	else
		index = (unsigned int) (usb_endpoint_num(desc)*2) +
			(usb_endpoint_dir_in(desc) ? 1 : 0) - 1;
	return index;
}

unsigned int xhci_get_endpoint_flag(struct usb_endpoint_descriptor *desc)
{
	return 1 << (xhci_get_endpoint_index(desc) + 1);
}

unsigned int xhci_get_endpoint_flag_from_index(unsigned int ep_index)
{
	return 1 << (ep_index + 1);
}

unsigned int xhci_last_valid_endpoint(u32 added_ctxs)
{
	return fls(added_ctxs) - 1;
}

int xhci_check_args(struct usb_hcd *hcd, struct usb_device *udev,
 							struct usb_host_endpoint *ep, int check_ep, bool check_virt_dev,
 							const char *func) {
 			struct xhci_hcd *xhci;
 			struct xhci_virt_device *virt_dev;

	if (!hcd || (check_ep && !ep) || !udev) {
		printk(KERN_DEBUG "xHCI %s called with invalid args\n",
				func);
		return -EINVAL;
	}
	if (!udev->parent) {
		printk(KERN_DEBUG "xHCI %s called for root hub\n",
				func);
		return 0;
	}

	if (check_virt_dev) {
					xhci = hcd_to_xhci(hcd);
					if (!udev->slot_id || !xhci->devs
									|| !xhci->devs[udev->slot_id]) {
									printk(KERN_DEBUG "xHCI %s called with unaddressed "
																					"device\n", func);
									return -EINVAL;
					}

					virt_dev = xhci->devs[udev->slot_id];
					if (virt_dev->udev != udev) {
									printk(KERN_DEBUG "xHCI %s called with udev and "
																		"virt_dev does not match\n", func);
									return -EINVAL;
					}

	}
	return 1;
}

static int xhci_configure_endpoint(struct xhci_hcd *xhci,
		struct usb_device *udev, struct xhci_command *command,
		bool ctx_change, bool must_succeed);

static int xhci_check_maxpacket(struct xhci_hcd *xhci, unsigned int slot_id,
		unsigned int ep_index, struct urb *urb)
{
	struct xhci_container_ctx *in_ctx;
	struct xhci_container_ctx *out_ctx;
	struct xhci_input_control_ctx *ctrl_ctx;
	struct xhci_ep_ctx *ep_ctx;
	int max_packet_size;
	int hw_max_packet_size;
	int ret = 0;

	out_ctx = xhci->devs[slot_id]->out_ctx;
	ep_ctx = xhci_get_ep_ctx(xhci, out_ctx, ep_index);
	hw_max_packet_size = MAX_PACKET_DECODED(le32_to_cpu(ep_ctx->ep_info2));
	max_packet_size = le16_to_cpu(urb->dev->ep0.desc.wMaxPacketSize);
	if (hw_max_packet_size != max_packet_size) {
		xhci_dbg(xhci, "Max Packet Size for ep 0 changed.\n");
		xhci_dbg(xhci, "Max packet size in usb_device = %d\n",
				max_packet_size);
		xhci_dbg(xhci, "Max packet size in xHCI HW = %d\n",
				hw_max_packet_size);
		xhci_dbg(xhci, "Issuing evaluate context command.\n");

		xhci_endpoint_copy(xhci, xhci->devs[slot_id]->in_ctx,
				xhci->devs[slot_id]->out_ctx, ep_index);
		in_ctx = xhci->devs[slot_id]->in_ctx;
		ep_ctx = xhci_get_ep_ctx(xhci, in_ctx, ep_index);
		ep_ctx->ep_info2 &= cpu_to_le32(~MAX_PACKET_MASK);
		ep_ctx->ep_info2 |= cpu_to_le32(MAX_PACKET(max_packet_size));

		ctrl_ctx = xhci_get_input_control_ctx(xhci, in_ctx);
		ctrl_ctx->add_flags = cpu_to_le32(EP0_FLAG);
		ctrl_ctx->drop_flags = 0;

		xhci_dbg(xhci, "Slot %d input context\n", slot_id);
		xhci_dbg_ctx(xhci, in_ctx, ep_index);
		xhci_dbg(xhci, "Slot %d output context\n", slot_id);
		xhci_dbg_ctx(xhci, out_ctx, ep_index);

		ret = xhci_configure_endpoint(xhci, urb->dev, NULL,
				true, false);

		ctrl_ctx->add_flags = cpu_to_le32(SLOT_FLAG);
	}
	return ret;
}

int xhci_urb_enqueue(struct usb_hcd *hcd, struct urb *urb, gfp_t mem_flags)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	unsigned long flags;
	int ret = 0;
	unsigned int slot_id, ep_index;
	struct urb_priv	*urb_priv;
	int size, i;

 			if (!urb || xhci_check_args(hcd, urb->dev, urb->ep,
 																			true, true, __func__) <= 0)

		return -EINVAL;

	slot_id = urb->dev->slot_id;
	ep_index = xhci_get_endpoint_index(&urb->ep->desc);

	if (!test_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags)) {
		if (!in_interrupt())
			xhci_dbg(xhci, "urb submitted during PCI suspend\n");
		ret = -ESHUTDOWN;
		goto exit;
	}

	if (usb_endpoint_xfer_isoc(&urb->ep->desc))
		size = urb->number_of_packets;
	else
		size = 1;

	urb_priv = kzalloc(sizeof(struct urb_priv) +
				  size * sizeof(struct xhci_td *), mem_flags);
	if (!urb_priv)
		return -ENOMEM;

	for (i = 0; i < size; i++) {
		urb_priv->td[i] = kzalloc(sizeof(struct xhci_td), mem_flags);
		if (!urb_priv->td[i]) {
			urb_priv->length = i;
			xhci_urb_free_priv(xhci, urb_priv);
			return -ENOMEM;
		}
	}

	urb_priv->length = size;
	urb_priv->td_cnt = 0;
	urb->hcpriv = urb_priv;

	if (usb_endpoint_xfer_control(&urb->ep->desc)) {
		 
		if (urb->dev->speed == USB_SPEED_FULL) {
			ret = xhci_check_maxpacket(xhci, slot_id,
					ep_index, urb);
			if (ret < 0) {
				xhci_urb_free_priv(xhci, urb_priv);
				urb->hcpriv = NULL;
				return ret;
			}
		}

		spin_lock_irqsave(&xhci->lock, flags);
		if (xhci->xhc_state & XHCI_STATE_DYING)
			goto dying;
		ret = xhci_queue_ctrl_tx(xhci, GFP_ATOMIC, urb,
				slot_id, ep_index);
		if (ret)
			goto free_priv;
		spin_unlock_irqrestore(&xhci->lock, flags);
	} else if (usb_endpoint_xfer_bulk(&urb->ep->desc)) {
		spin_lock_irqsave(&xhci->lock, flags);
		if (xhci->xhc_state & XHCI_STATE_DYING)
			goto dying;
		if (xhci->devs[slot_id]->eps[ep_index].ep_state &
				EP_GETTING_STREAMS) {
			xhci_warn(xhci, "WARN: Can't enqueue URB while bulk ep "
					"is transitioning to using streams.\n");
			ret = -EINVAL;
		} else if (xhci->devs[slot_id]->eps[ep_index].ep_state &
				EP_GETTING_NO_STREAMS) {
			xhci_warn(xhci, "WARN: Can't enqueue URB while bulk ep "
					"is transitioning to "
					"not having streams.\n");
			ret = -EINVAL;
		} else {
			ret = xhci_queue_bulk_tx(xhci, GFP_ATOMIC, urb,
					slot_id, ep_index);
		}
		if (ret)
			goto free_priv;
		spin_unlock_irqrestore(&xhci->lock, flags);
	} else if (usb_endpoint_xfer_int(&urb->ep->desc)) {
		spin_lock_irqsave(&xhci->lock, flags);
		if (xhci->xhc_state & XHCI_STATE_DYING)
			goto dying;
		ret = xhci_queue_intr_tx(xhci, GFP_ATOMIC, urb,
				slot_id, ep_index);
		if (ret)
			goto free_priv;
		spin_unlock_irqrestore(&xhci->lock, flags);
	} else {
		spin_lock_irqsave(&xhci->lock, flags);
		if (xhci->xhc_state & XHCI_STATE_DYING)
			goto dying;
		ret = xhci_queue_isoc_tx_prepare(xhci, GFP_ATOMIC, urb,
				slot_id, ep_index);
		if (ret)
			goto free_priv;
		spin_unlock_irqrestore(&xhci->lock, flags);
	}
exit:
	return ret;
dying:
	xhci_dbg(xhci, "Ep 0x%x: URB %p submitted for "
			"non-responsive xHCI host.\n",
			urb->ep->desc.bEndpointAddress, urb);
	ret = -ESHUTDOWN;
free_priv:
	xhci_urb_free_priv(xhci, urb_priv);
	urb->hcpriv = NULL;
	spin_unlock_irqrestore(&xhci->lock, flags);
	return ret;
}

static struct xhci_ring *xhci_urb_to_transfer_ring(struct xhci_hcd *xhci,
		struct urb *urb)
{
	unsigned int slot_id;
	unsigned int ep_index;
	unsigned int stream_id;
	struct xhci_virt_ep *ep;

	slot_id = urb->dev->slot_id;
	ep_index = xhci_get_endpoint_index(&urb->ep->desc);
	stream_id = urb->stream_id;
	ep = &xhci->devs[slot_id]->eps[ep_index];
	 
	if (!(ep->ep_state & EP_HAS_STREAMS))
		return ep->ring;

	if (stream_id == 0) {
		xhci_warn(xhci,
				"WARN: Slot ID %u, ep index %u has streams, "
				"but URB has no stream ID.\n",
				slot_id, ep_index);
		return NULL;
	}

	if (stream_id < ep->stream_info->num_streams)
		return ep->stream_info->stream_rings[stream_id];

	xhci_warn(xhci,
			"WARN: Slot ID %u, ep index %u has "
			"stream IDs 1 to %u allocated, "
			"but stream ID %u is requested.\n",
			slot_id, ep_index,
			ep->stream_info->num_streams - 1,
			stream_id);
	return NULL;
}

int xhci_urb_dequeue(struct usb_hcd *hcd, struct urb *urb, int status)
{
	unsigned long flags;
	int ret, i;
	u32 temp;
	struct xhci_hcd *xhci;
	struct urb_priv	*urb_priv;
	struct xhci_td *td;
	unsigned int ep_index;
	struct xhci_ring *ep_ring;
	struct xhci_virt_ep *ep;

	xhci = hcd_to_xhci(hcd);
	spin_lock_irqsave(&xhci->lock, flags);
	 
	ret = usb_hcd_check_unlink_urb(hcd, urb, status);
	if (ret || !urb->hcpriv)
		goto done;
	temp = xhci_readl(xhci, &xhci->op_regs->status);
	if (temp == 0xffffffff) {
		xhci_dbg(xhci, "HW died, freeing TD.\n");
		urb_priv = urb->hcpriv;

		usb_hcd_unlink_urb_from_ep(hcd, urb);
		spin_unlock_irqrestore(&xhci->lock, flags);
		usb_hcd_giveback_urb(hcd, urb, -ESHUTDOWN);
		xhci_urb_free_priv(xhci, urb_priv);
		return ret;
	}
	if (xhci->xhc_state & XHCI_STATE_DYING) {
		xhci_dbg(xhci, "Ep 0x%x: URB %p to be canceled on "
				"non-responsive xHCI host.\n",
				urb->ep->desc.bEndpointAddress, urb);
		 
		goto done;
	}

	xhci_dbg(xhci, "Cancel URB %p\n", urb);
	xhci_dbg(xhci, "Event ring:\n");
	xhci_debug_ring(xhci, xhci->event_ring);
	ep_index = xhci_get_endpoint_index(&urb->ep->desc);
	ep = &xhci->devs[urb->dev->slot_id]->eps[ep_index];
	ep_ring = xhci_urb_to_transfer_ring(xhci, urb);
	if (!ep_ring) {
		ret = -EINVAL;
		goto done;
	}

	xhci_dbg(xhci, "Endpoint ring:\n");
	xhci_debug_ring(xhci, ep_ring);

	urb_priv = urb->hcpriv;

	for (i = urb_priv->td_cnt; i < urb_priv->length; i++) {
		td = urb_priv->td[i];
		list_add_tail(&td->cancelled_td_list, &ep->cancelled_td_list);
	}

	if (!(ep->ep_state & EP_HALT_PENDING)) {
		ep->ep_state |= EP_HALT_PENDING;
		ep->stop_cmds_pending++;
		ep->stop_cmd_timer.expires = jiffies +
			XHCI_STOP_EP_CMD_TIMEOUT * HZ;
		add_timer(&ep->stop_cmd_timer);
		xhci_queue_stop_endpoint(xhci, urb->dev->slot_id, ep_index);
		xhci_ring_cmd_db(xhci);
	}
done:
	spin_unlock_irqrestore(&xhci->lock, flags);
	return ret;
}

int xhci_drop_endpoint(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep)
{
	struct xhci_hcd *xhci;
	struct xhci_container_ctx *in_ctx, *out_ctx;
	struct xhci_input_control_ctx *ctrl_ctx;
	struct xhci_slot_ctx *slot_ctx;
	unsigned int last_ctx;
	unsigned int ep_index;
	struct xhci_ep_ctx *ep_ctx;
	u32 drop_flag;
	u32 new_add_flags, new_drop_flags, new_slot_info;
	int ret;

	ret = xhci_check_args(hcd, udev, ep, 1, true, __func__);
	if (ret <= 0)
		return ret;
	xhci = hcd_to_xhci(hcd);
	xhci_dbg(xhci, "%s called for udev %p\n", __func__, udev);

	drop_flag = xhci_get_endpoint_flag(&ep->desc);
	if (drop_flag == SLOT_FLAG || drop_flag == EP0_FLAG) {
		xhci_dbg(xhci, "xHCI %s - can't drop slot or ep 0 %#x\n",
				__func__, drop_flag);
		return 0;
	}

	in_ctx = xhci->devs[udev->slot_id]->in_ctx;
	out_ctx = xhci->devs[udev->slot_id]->out_ctx;
	ctrl_ctx = xhci_get_input_control_ctx(xhci, in_ctx);
	ep_index = xhci_get_endpoint_index(&ep->desc);
	ep_ctx = xhci_get_ep_ctx(xhci, out_ctx, ep_index);
	 
	if (((ep_ctx->ep_info & cpu_to_le32(EP_STATE_MASK)) ==
			cpu_to_le32(EP_STATE_DISABLED)) ||
			le32_to_cpu(ctrl_ctx->drop_flags) &
			xhci_get_endpoint_flag(&ep->desc)) {
		xhci_warn(xhci, "xHCI %s called with disabled ep %p\n",
				__func__, ep);
		return 0;
	}

	ctrl_ctx->drop_flags |= cpu_to_le32(drop_flag);
	new_drop_flags = le32_to_cpu(ctrl_ctx->drop_flags);

	ctrl_ctx->add_flags &= cpu_to_le32(~drop_flag);
	new_add_flags = le32_to_cpu(ctrl_ctx->add_flags);

	last_ctx = xhci_last_valid_endpoint(le32_to_cpu(ctrl_ctx->add_flags));
	slot_ctx = xhci_get_slot_ctx(xhci, in_ctx);
	 
	if ((le32_to_cpu(slot_ctx->dev_info) & LAST_CTX_MASK) >
		LAST_CTX(last_ctx)) {
		slot_ctx->dev_info &= cpu_to_le32(~LAST_CTX_MASK);
		slot_ctx->dev_info |= cpu_to_le32(LAST_CTX(last_ctx));
	}
	new_slot_info = le32_to_cpu(slot_ctx->dev_info);

	xhci_endpoint_zero(xhci, xhci->devs[udev->slot_id], ep);

	xhci_dbg(xhci, "drop ep 0x%x, slot id %d, new drop flags = %#x, new add flags = %#x, new slot info = %#x\n",
			(unsigned int) ep->desc.bEndpointAddress,
			udev->slot_id,
			(unsigned int) new_drop_flags,
			(unsigned int) new_add_flags,
			(unsigned int) new_slot_info);
	return 0;
}

int xhci_add_endpoint(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint *ep)
{
	struct xhci_hcd *xhci;
	struct xhci_container_ctx *in_ctx, *out_ctx;
	unsigned int ep_index;
	struct xhci_ep_ctx *ep_ctx;
	struct xhci_slot_ctx *slot_ctx;
	struct xhci_input_control_ctx *ctrl_ctx;
	u32 added_ctxs;
	unsigned int last_ctx;
	u32 new_add_flags, new_drop_flags, new_slot_info;
	int ret = 0;

	ret = xhci_check_args(hcd, udev, ep, 1, true, __func__);
	if (ret <= 0) {
		 
		ep->hcpriv = NULL;
		return ret;
	}
	xhci = hcd_to_xhci(hcd);

	added_ctxs = xhci_get_endpoint_flag(&ep->desc);
	last_ctx = xhci_last_valid_endpoint(added_ctxs);
	if (added_ctxs == SLOT_FLAG || added_ctxs == EP0_FLAG) {
		 
		xhci_dbg(xhci, "xHCI %s - can't add slot or ep 0 %#x\n",
				__func__, added_ctxs);
		return 0;
	}

	in_ctx = xhci->devs[udev->slot_id]->in_ctx;
	out_ctx = xhci->devs[udev->slot_id]->out_ctx;
	ctrl_ctx = xhci_get_input_control_ctx(xhci, in_ctx);
	ep_index = xhci_get_endpoint_index(&ep->desc);
	ep_ctx = xhci_get_ep_ctx(xhci, out_ctx, ep_index);
	 
	if (le32_to_cpu(ctrl_ctx->add_flags) & xhci_get_endpoint_flag(&ep->desc)) {
		xhci_warn(xhci, "xHCI %s called with enabled ep %p\n",
				__func__, ep);
		return 0;
	}

	if (xhci_endpoint_init(xhci, xhci->devs[udev->slot_id],
				udev, ep, GFP_NOIO) < 0) {
		dev_dbg(&udev->dev, "%s - could not initialize ep %#x\n",
				__func__, ep->desc.bEndpointAddress);
		return -ENOMEM;
	}

	ctrl_ctx->add_flags |= cpu_to_le32(added_ctxs);
	new_add_flags = le32_to_cpu(ctrl_ctx->add_flags);

	new_drop_flags = le32_to_cpu(ctrl_ctx->drop_flags);

	slot_ctx = xhci_get_slot_ctx(xhci, in_ctx);
	 
	if ((le32_to_cpu(slot_ctx->dev_info) & LAST_CTX_MASK) <
		LAST_CTX(last_ctx)) {
		slot_ctx->dev_info &= cpu_to_le32(~LAST_CTX_MASK);
		slot_ctx->dev_info |= cpu_to_le32(LAST_CTX(last_ctx));
	}
	new_slot_info = le32_to_cpu(slot_ctx->dev_info);

	ep->hcpriv = udev;

	xhci_dbg(xhci, "add ep 0x%x, slot id %d, new drop flags = %#x, new add flags = %#x, new slot info = %#x\n",
			(unsigned int) ep->desc.bEndpointAddress,
			udev->slot_id,
			(unsigned int) new_drop_flags,
			(unsigned int) new_add_flags,
			(unsigned int) new_slot_info);
	return 0;
}

static void xhci_zero_in_ctx(struct xhci_hcd *xhci, struct xhci_virt_device *virt_dev)
{
	struct xhci_input_control_ctx *ctrl_ctx;
	struct xhci_ep_ctx *ep_ctx;
	struct xhci_slot_ctx *slot_ctx;
	int i;

	ctrl_ctx = xhci_get_input_control_ctx(xhci, virt_dev->in_ctx);
	ctrl_ctx->drop_flags = 0;
	ctrl_ctx->add_flags = 0;
	slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->in_ctx);
	slot_ctx->dev_info &= cpu_to_le32(~LAST_CTX_MASK);
	 
	slot_ctx->dev_info |= cpu_to_le32(LAST_CTX(1));
	for (i = 1; i < 31; ++i) {
		ep_ctx = xhci_get_ep_ctx(xhci, virt_dev->in_ctx, i);
		ep_ctx->ep_info = 0;
		ep_ctx->ep_info2 = 0;
		ep_ctx->deq = 0;
		ep_ctx->tx_info = 0;
	}
}

static int xhci_configure_endpoint_result(struct xhci_hcd *xhci,
		struct usb_device *udev, int *cmd_status)
{
	int ret;

	switch (*cmd_status) {
	case COMP_ENOMEM:
		dev_warn(&udev->dev, "Not enough host controller resources "
				"for new device state.\n");
		ret = -ENOMEM;
		 
		break;
	case COMP_BW_ERR:
		dev_warn(&udev->dev, "Not enough bandwidth "
				"for new device state.\n");
		ret = -ENOSPC;
		 
		break;
	case COMP_TRB_ERR:
		 
		dev_warn(&udev->dev, "ERROR: Endpoint drop flag = 0, "
				"add flag = 1, "
				"and endpoint is not disabled.\n");
		ret = -EINVAL;
		break;
	case COMP_SUCCESS:
		dev_dbg(&udev->dev, "Successful Endpoint Configure command\n");
		ret = 0;
		break;
	default:
		xhci_err(xhci, "ERROR: unexpected command completion "
				"code 0x%x.\n", *cmd_status);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int xhci_evaluate_context_result(struct xhci_hcd *xhci,
		struct usb_device *udev, int *cmd_status)
{
	int ret;
	struct xhci_virt_device *virt_dev = xhci->devs[udev->slot_id];

	switch (*cmd_status) {
	case COMP_EINVAL:
		dev_warn(&udev->dev, "WARN: xHCI driver setup invalid evaluate "
				"context command.\n");
		ret = -EINVAL;
		break;
	case COMP_EBADSLT:
		dev_warn(&udev->dev, "WARN: slot not enabled for"
				"evaluate context command.\n");
	case COMP_CTX_STATE:
		dev_warn(&udev->dev, "WARN: invalid context state for "
				"evaluate context command.\n");
		xhci_dbg_ctx(xhci, virt_dev->out_ctx, 1);
		ret = -EINVAL;
		break;
	case COMP_SUCCESS:
		dev_dbg(&udev->dev, "Successful evaluate context command\n");
		ret = 0;
		break;
	default:
		xhci_err(xhci, "ERROR: unexpected command completion "
				"code 0x%x.\n", *cmd_status);
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int xhci_configure_endpoint(struct xhci_hcd *xhci,
		struct usb_device *udev,
		struct xhci_command *command,
		bool ctx_change, bool must_succeed)
{
	int ret;
	int timeleft;
	unsigned long flags;
	struct xhci_container_ctx *in_ctx;
	struct completion *cmd_completion;
	u32 *cmd_status;
	struct xhci_virt_device *virt_dev;

	spin_lock_irqsave(&xhci->lock, flags);
	virt_dev = xhci->devs[udev->slot_id];
	if (command) {
		in_ctx = command->in_ctx;
		cmd_completion = command->completion;
		cmd_status = &command->status;
		command->command_trb = xhci->cmd_ring->enqueue;

 							if (TRB_TYPE_LINK_LE32(command->command_trb->link.control))
 											command->command_trb =
 															xhci->cmd_ring->enq_seg->next->trbs;

		list_add_tail(&command->cmd_list, &virt_dev->cmd_list);
	} else {
		in_ctx = virt_dev->in_ctx;
		cmd_completion = &virt_dev->cmd_completion;
		cmd_status = &virt_dev->cmd_status;
	}
	init_completion(cmd_completion);

	if (!ctx_change)
		ret = xhci_queue_configure_endpoint(xhci, in_ctx->dma,
				udev->slot_id, must_succeed);
	else
		ret = xhci_queue_evaluate_context(xhci, in_ctx->dma,
				udev->slot_id);
	if (ret < 0) {
		if (command)
			list_del(&command->cmd_list);
		spin_unlock_irqrestore(&xhci->lock, flags);
		xhci_dbg(xhci, "FIXME allocate a new ring segment\n");
		return -ENOMEM;
	}
	xhci_ring_cmd_db(xhci);
	spin_unlock_irqrestore(&xhci->lock, flags);

	timeleft = wait_for_completion_interruptible_timeout(
			cmd_completion,
#ifdef MY_ABC_HERE
			USB_CTRL_SET_TIMEOUT/5);
#else
			USB_CTRL_SET_TIMEOUT);
#endif
	if (timeleft <= 0) {
		xhci_warn(xhci, "%s while waiting for %s command\n",
				timeleft == 0 ? "Timeout" : "Signal",
				ctx_change == 0 ?
					"configure endpoint" :
					"evaluate context");
		 
		return -ETIME;
	}

	if (!ctx_change)
		return xhci_configure_endpoint_result(xhci, udev, cmd_status);
	return xhci_evaluate_context_result(xhci, udev, cmd_status);
}

int xhci_check_bandwidth(struct usb_hcd *hcd, struct usb_device *udev)
{
	int i;
	int ret = 0;
	struct xhci_hcd *xhci;
	struct xhci_virt_device	*virt_dev;
	struct xhci_input_control_ctx *ctrl_ctx;
	struct xhci_slot_ctx *slot_ctx;

	ret = xhci_check_args(hcd, udev, NULL, 0, true, __func__);
	if (ret <= 0)
		return ret;
	xhci = hcd_to_xhci(hcd);

	xhci_dbg(xhci, "%s called for udev %p\n", __func__, udev);
	virt_dev = xhci->devs[udev->slot_id];

	ctrl_ctx = xhci_get_input_control_ctx(xhci, virt_dev->in_ctx);
	ctrl_ctx->add_flags |= cpu_to_le32(SLOT_FLAG);
	ctrl_ctx->add_flags &= cpu_to_le32(~EP0_FLAG);
	ctrl_ctx->drop_flags &= cpu_to_le32(~(SLOT_FLAG | EP0_FLAG));
	xhci_dbg(xhci, "New Input Control Context:\n");
	slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->in_ctx);
	xhci_dbg_ctx(xhci, virt_dev->in_ctx,
			LAST_CTX_TO_EP_NUM(le32_to_cpu(slot_ctx->dev_info)));

	ret = xhci_configure_endpoint(xhci, udev, NULL,
			false, false);
	if (ret) {
		 
		return ret;
	}

	xhci_dbg(xhci, "Output context after successful config ep cmd:\n");
	xhci_dbg_ctx(xhci, virt_dev->out_ctx,
			LAST_CTX_TO_EP_NUM(le32_to_cpu(slot_ctx->dev_info)));

	for (i = 1; i < 31; ++i) {
		if ((le32_to_cpu(ctrl_ctx->drop_flags) & (1 << (i + 1))) &&
			!(le32_to_cpu(ctrl_ctx->add_flags) & (1 << (i + 1))))
			xhci_free_or_cache_endpoint_ring(xhci, virt_dev, i);
	}

	xhci_zero_in_ctx(xhci, virt_dev);
	 
	for (i = 1; i < 31; ++i) {
		if (!virt_dev->eps[i].new_ring)
			continue;
		 
		if (virt_dev->eps[i].ring) {
			xhci_free_or_cache_endpoint_ring(xhci, virt_dev, i);
		}
		virt_dev->eps[i].ring = virt_dev->eps[i].new_ring;
		virt_dev->eps[i].new_ring = NULL;
	}

	return ret;
}

void xhci_reset_bandwidth(struct usb_hcd *hcd, struct usb_device *udev)
{
	struct xhci_hcd *xhci;
	struct xhci_virt_device	*virt_dev;
	int i, ret;

	ret = xhci_check_args(hcd, udev, NULL, 0, true, __func__);
	if (ret <= 0)
		return;
	xhci = hcd_to_xhci(hcd);

	xhci_dbg(xhci, "%s called for udev %p\n", __func__, udev);
	virt_dev = xhci->devs[udev->slot_id];
	 
	for (i = 0; i < 31; ++i) {
		if (virt_dev->eps[i].new_ring) {
			xhci_ring_free(xhci, virt_dev->eps[i].new_ring);
			virt_dev->eps[i].new_ring = NULL;
		}
	}
	xhci_zero_in_ctx(xhci, virt_dev);
}

static void xhci_setup_input_ctx_for_config_ep(struct xhci_hcd *xhci,
		struct xhci_container_ctx *in_ctx,
		struct xhci_container_ctx *out_ctx,
		u32 add_flags, u32 drop_flags)
{
	struct xhci_input_control_ctx *ctrl_ctx;
	ctrl_ctx = xhci_get_input_control_ctx(xhci, in_ctx);
	ctrl_ctx->add_flags = cpu_to_le32(add_flags);
	ctrl_ctx->drop_flags = cpu_to_le32(drop_flags);
	xhci_slot_copy(xhci, in_ctx, out_ctx);
	ctrl_ctx->add_flags |= cpu_to_le32(SLOT_FLAG);

	xhci_dbg(xhci, "Input Context:\n");
	xhci_dbg_ctx(xhci, in_ctx, xhci_last_valid_endpoint(add_flags));
}

void xhci_setup_input_ctx_for_quirk(struct xhci_hcd *xhci,
		unsigned int slot_id, unsigned int ep_index,
		struct xhci_dequeue_state *deq_state)
{
	struct xhci_container_ctx *in_ctx;
	struct xhci_ep_ctx *ep_ctx;
	u32 added_ctxs;
	dma_addr_t addr;

	xhci_endpoint_copy(xhci, xhci->devs[slot_id]->in_ctx,
			xhci->devs[slot_id]->out_ctx, ep_index);
	in_ctx = xhci->devs[slot_id]->in_ctx;
	ep_ctx = xhci_get_ep_ctx(xhci, in_ctx, ep_index);
	addr = xhci_trb_virt_to_dma(deq_state->new_deq_seg,
			deq_state->new_deq_ptr);
	if (addr == 0) {
		xhci_warn(xhci, "WARN Cannot submit config ep after "
				"reset ep command\n");
		xhci_warn(xhci, "WARN deq seg = %p, deq ptr = %p\n",
				deq_state->new_deq_seg,
				deq_state->new_deq_ptr);
		return;
	}
	ep_ctx->deq = cpu_to_le64(addr | deq_state->new_cycle_state);

	added_ctxs = xhci_get_endpoint_flag_from_index(ep_index);
	xhci_setup_input_ctx_for_config_ep(xhci, xhci->devs[slot_id]->in_ctx,
			xhci->devs[slot_id]->out_ctx, added_ctxs, added_ctxs);
}

void xhci_cleanup_stalled_ring(struct xhci_hcd *xhci,
		struct usb_device *udev, unsigned int ep_index)
{
	struct xhci_dequeue_state deq_state;
	struct xhci_virt_ep *ep;

	xhci_dbg(xhci, "Cleaning up stalled endpoint ring\n");
	ep = &xhci->devs[udev->slot_id]->eps[ep_index];
	 
	xhci_find_new_dequeue_state(xhci, udev->slot_id,
			ep_index, ep->stopped_stream, ep->stopped_td,
			&deq_state);

	if (!(xhci->quirks & XHCI_RESET_EP_QUIRK)) {
		xhci_dbg(xhci, "Queueing new dequeue state\n");
		xhci_queue_new_dequeue_state(xhci, udev->slot_id,
				ep_index, ep->stopped_stream, &deq_state);
	} else {
		 
		xhci_dbg(xhci, "Setting up input context for "
				"configure endpoint command\n");
		xhci_setup_input_ctx_for_quirk(xhci, udev->slot_id,
				ep_index, &deq_state);
	}
}

void xhci_endpoint_reset(struct usb_hcd *hcd,
		struct usb_host_endpoint *ep)
{
	struct xhci_hcd *xhci;
	struct usb_device *udev;
	unsigned int ep_index;
	unsigned long flags;
	int ret;
	struct xhci_virt_ep *virt_ep;

	xhci = hcd_to_xhci(hcd);
	udev = (struct usb_device *) ep->hcpriv;
	 
	if (!ep->hcpriv)
		return;

	ep_index = xhci_get_endpoint_index(&ep->desc);
	virt_ep = &xhci->devs[udev->slot_id]->eps[ep_index];
	if (!virt_ep->stopped_td) {
		xhci_dbg(xhci, "Endpoint 0x%x not halted, refusing to reset.\n",
				ep->desc.bEndpointAddress);
		return;
	}
	if (usb_endpoint_xfer_control(&ep->desc)) {
		xhci_dbg(xhci, "Control endpoint stall already handled.\n");
		return;
	}

#ifdef MY_ABC_HERE
			 
			msleep(3000);
#endif

	xhci_dbg(xhci, "Queueing reset endpoint command\n");
	spin_lock_irqsave(&xhci->lock, flags);
	ret = xhci_queue_reset_ep(xhci, udev->slot_id, ep_index);
	 
	if (!ret) {
		xhci_cleanup_stalled_ring(xhci, udev, ep_index);
		kfree(virt_ep->stopped_td);
		xhci_ring_cmd_db(xhci);
	}
	virt_ep->stopped_td = NULL;
	virt_ep->stopped_trb = NULL;
	virt_ep->stopped_stream = 0;
	spin_unlock_irqrestore(&xhci->lock, flags);

	if (ret)
		xhci_warn(xhci, "FIXME allocate a new ring segment\n");

}

static int xhci_check_streams_endpoint(struct xhci_hcd *xhci,
		struct usb_device *udev, struct usb_host_endpoint *ep,
		unsigned int slot_id)
{
	int ret;
	unsigned int ep_index;
	unsigned int ep_state;

	if (!ep)
		return -EINVAL;
	ret = xhci_check_args(xhci_to_hcd(xhci), udev, ep, 1, true, __func__);
	if (ret <= 0)
		return -EINVAL;
	if (ep->ss_ep_comp.bmAttributes == 0) {
		xhci_warn(xhci, "WARN: SuperSpeed Endpoint Companion"
				" descriptor for ep 0x%x does not support streams\n",
				ep->desc.bEndpointAddress);
		return -EINVAL;
	}

	ep_index = xhci_get_endpoint_index(&ep->desc);
	ep_state = xhci->devs[slot_id]->eps[ep_index].ep_state;
	if (ep_state & EP_HAS_STREAMS ||
			ep_state & EP_GETTING_STREAMS) {
		xhci_warn(xhci, "WARN: SuperSpeed bulk endpoint 0x%x "
				"already has streams set up.\n",
				ep->desc.bEndpointAddress);
		xhci_warn(xhci, "Send email to xHCI maintainer and ask for "
				"dynamic stream context array reallocation.\n");
		return -EINVAL;
	}
	if (!list_empty(&xhci->devs[slot_id]->eps[ep_index].ring->td_list)) {
		xhci_warn(xhci, "Cannot setup streams for SuperSpeed bulk "
				"endpoint 0x%x; URBs are pending.\n",
				ep->desc.bEndpointAddress);
		return -EINVAL;
	}
	return 0;
}

static void xhci_calculate_streams_entries(struct xhci_hcd *xhci,
		unsigned int *num_streams, unsigned int *num_stream_ctxs)
{
	unsigned int max_streams;

	*num_stream_ctxs = roundup_pow_of_two(*num_streams);
	 
	max_streams = HCC_MAX_PSA(xhci->hcc_params);
	if (*num_stream_ctxs > max_streams) {
		xhci_dbg(xhci, "xHCI HW only supports %u stream ctx entries.\n",
				max_streams);
		*num_stream_ctxs = max_streams;
		*num_streams = max_streams;
	}
}

static int xhci_calculate_streams_and_bitmask(struct xhci_hcd *xhci,
		struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		unsigned int *num_streams, u32 *changed_ep_bitmask)
{
	unsigned int max_streams;
	unsigned int endpoint_flag;
	int i;
	int ret;

	for (i = 0; i < num_eps; i++) {
		ret = xhci_check_streams_endpoint(xhci, udev,
				eps[i], udev->slot_id);
		if (ret < 0)
			return ret;

		max_streams = USB_SS_MAX_STREAMS(eps[i]->ss_ep_comp.bmAttributes);
		if (max_streams < (*num_streams - 1)) {
			xhci_dbg(xhci, "Ep 0x%x only supports %u stream IDs.\n",
					eps[i]->desc.bEndpointAddress,
					max_streams);
			*num_streams = max_streams+1;
		}

		endpoint_flag = xhci_get_endpoint_flag(&eps[i]->desc);
		if (*changed_ep_bitmask & endpoint_flag)
			return -EINVAL;
		*changed_ep_bitmask |= endpoint_flag;
	}
	return 0;
}

static u32 xhci_calculate_no_streams_bitmask(struct xhci_hcd *xhci,
		struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps)
{
	u32 changed_ep_bitmask = 0;
	unsigned int slot_id;
	unsigned int ep_index;
	unsigned int ep_state;
	int i;

	slot_id = udev->slot_id;
	if (!xhci->devs[slot_id])
		return 0;

	for (i = 0; i < num_eps; i++) {
		ep_index = xhci_get_endpoint_index(&eps[i]->desc);
		ep_state = xhci->devs[slot_id]->eps[ep_index].ep_state;
		 
		if (ep_state & EP_GETTING_NO_STREAMS) {
			xhci_warn(xhci, "WARN Can't disable streams for "
					"endpoint 0x%x\n, "
					"streams are being disabled already.",
					eps[i]->desc.bEndpointAddress);
			return 0;
		}
		 
		if (!(ep_state & EP_HAS_STREAMS) &&
				!(ep_state & EP_GETTING_STREAMS)) {
			xhci_warn(xhci, "WARN Can't disable streams for "
					"endpoint 0x%x\n, "
					"streams are already disabled!",
					eps[i]->desc.bEndpointAddress);
			xhci_warn(xhci, "WARN xhci_free_streams() called "
					"with non-streams endpoint\n");
			return 0;
		}
		changed_ep_bitmask |= xhci_get_endpoint_flag(&eps[i]->desc);
	}
	return changed_ep_bitmask;
}

int xhci_alloc_streams(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		unsigned int num_streams, gfp_t mem_flags)
{
	int i, ret;
	struct xhci_hcd *xhci;
	struct xhci_virt_device *vdev;
	struct xhci_command *config_cmd;
	unsigned int ep_index;
	unsigned int num_stream_ctxs;
	unsigned long flags;
	u32 changed_ep_bitmask = 0;

	if (!eps)
		return -EINVAL;

	num_streams += 1;
	xhci = hcd_to_xhci(hcd);
	xhci_dbg(xhci, "Driver wants %u stream IDs (including stream 0).\n",
			num_streams);

	config_cmd = xhci_alloc_command(xhci, true, true, mem_flags);
	if (!config_cmd) {
		xhci_dbg(xhci, "Could not allocate xHCI command structure.\n");
		return -ENOMEM;
	}

	spin_lock_irqsave(&xhci->lock, flags);
	ret = xhci_calculate_streams_and_bitmask(xhci, udev, eps,
			num_eps, &num_streams, &changed_ep_bitmask);
	if (ret < 0) {
		xhci_free_command(xhci, config_cmd);
		spin_unlock_irqrestore(&xhci->lock, flags);
		return ret;
	}
	if (num_streams <= 1) {
		xhci_warn(xhci, "WARN: endpoints can't handle "
				"more than one stream.\n");
		xhci_free_command(xhci, config_cmd);
		spin_unlock_irqrestore(&xhci->lock, flags);
		return -EINVAL;
	}
	vdev = xhci->devs[udev->slot_id];
	 
	for (i = 0; i < num_eps; i++) {
		ep_index = xhci_get_endpoint_index(&eps[i]->desc);
		vdev->eps[ep_index].ep_state |= EP_GETTING_STREAMS;
	}
	spin_unlock_irqrestore(&xhci->lock, flags);

	xhci_calculate_streams_entries(xhci, &num_streams, &num_stream_ctxs);
	xhci_dbg(xhci, "Need %u stream ctx entries for %u stream IDs.\n",
			num_stream_ctxs, num_streams);

	for (i = 0; i < num_eps; i++) {
		ep_index = xhci_get_endpoint_index(&eps[i]->desc);
		vdev->eps[ep_index].stream_info = xhci_alloc_stream_info(xhci,
				num_stream_ctxs,
				num_streams, mem_flags);
		if (!vdev->eps[ep_index].stream_info)
			goto cleanup;
		 
	}

	for (i = 0; i < num_eps; i++) {
		struct xhci_ep_ctx *ep_ctx;

		ep_index = xhci_get_endpoint_index(&eps[i]->desc);
		ep_ctx = xhci_get_ep_ctx(xhci, config_cmd->in_ctx, ep_index);

		xhci_endpoint_copy(xhci, config_cmd->in_ctx,
				vdev->out_ctx, ep_index);
		xhci_setup_streams_ep_input_ctx(xhci, ep_ctx,
				vdev->eps[ep_index].stream_info);
	}
	 
	xhci_setup_input_ctx_for_config_ep(xhci, config_cmd->in_ctx,
			vdev->out_ctx, changed_ep_bitmask, changed_ep_bitmask);

	ret = xhci_configure_endpoint(xhci, udev, config_cmd,
			false, false);

	if (ret < 0)
		goto cleanup;

	spin_lock_irqsave(&xhci->lock, flags);
	for (i = 0; i < num_eps; i++) {
		ep_index = xhci_get_endpoint_index(&eps[i]->desc);
		vdev->eps[ep_index].ep_state &= ~EP_GETTING_STREAMS;
		xhci_dbg(xhci, "Slot %u ep ctx %u now has streams.\n",
			 udev->slot_id, ep_index);
		vdev->eps[ep_index].ep_state |= EP_HAS_STREAMS;
	}
	xhci_free_command(xhci, config_cmd);
	spin_unlock_irqrestore(&xhci->lock, flags);

	return num_streams - 1;

cleanup:
	 
	for (i = 0; i < num_eps; i++) {
		ep_index = xhci_get_endpoint_index(&eps[i]->desc);
		xhci_free_stream_info(xhci, vdev->eps[ep_index].stream_info);
		 
		vdev->eps[ep_index].ep_state &= ~EP_GETTING_STREAMS;
		vdev->eps[ep_index].ep_state &= ~EP_HAS_STREAMS;
		xhci_endpoint_zero(xhci, vdev, eps[i]);
	}
	xhci_free_command(xhci, config_cmd);
	return -ENOMEM;
}

int xhci_free_streams(struct usb_hcd *hcd, struct usb_device *udev,
		struct usb_host_endpoint **eps, unsigned int num_eps,
		gfp_t mem_flags)
{
	int i, ret;
	struct xhci_hcd *xhci;
	struct xhci_virt_device *vdev;
	struct xhci_command *command;
	unsigned int ep_index;
	unsigned long flags;
	u32 changed_ep_bitmask;

	xhci = hcd_to_xhci(hcd);
	vdev = xhci->devs[udev->slot_id];

	spin_lock_irqsave(&xhci->lock, flags);
	changed_ep_bitmask = xhci_calculate_no_streams_bitmask(xhci,
			udev, eps, num_eps);
	if (changed_ep_bitmask == 0) {
		spin_unlock_irqrestore(&xhci->lock, flags);
		return -EINVAL;
	}

	ep_index = xhci_get_endpoint_index(&eps[0]->desc);
	command = vdev->eps[ep_index].stream_info->free_streams_command;
	for (i = 0; i < num_eps; i++) {
		struct xhci_ep_ctx *ep_ctx;

		ep_index = xhci_get_endpoint_index(&eps[i]->desc);
		ep_ctx = xhci_get_ep_ctx(xhci, command->in_ctx, ep_index);
		xhci->devs[udev->slot_id]->eps[ep_index].ep_state |=
			EP_GETTING_NO_STREAMS;

		xhci_endpoint_copy(xhci, command->in_ctx,
				vdev->out_ctx, ep_index);
		xhci_setup_no_streams_ep_input_ctx(xhci, ep_ctx,
				&vdev->eps[ep_index]);
	}
	xhci_setup_input_ctx_for_config_ep(xhci, command->in_ctx,
			vdev->out_ctx, changed_ep_bitmask, changed_ep_bitmask);
	spin_unlock_irqrestore(&xhci->lock, flags);

	ret = xhci_configure_endpoint(xhci, udev, command,
			false, true);

	if (ret < 0)
		return ret;

	spin_lock_irqsave(&xhci->lock, flags);
	for (i = 0; i < num_eps; i++) {
		ep_index = xhci_get_endpoint_index(&eps[i]->desc);
		xhci_free_stream_info(xhci, vdev->eps[ep_index].stream_info);
		 
		vdev->eps[ep_index].ep_state &= ~EP_GETTING_NO_STREAMS;
		vdev->eps[ep_index].ep_state &= ~EP_HAS_STREAMS;
	}
	spin_unlock_irqrestore(&xhci->lock, flags);

	return 0;
}

int xhci_discover_or_reset_device(struct usb_hcd *hcd, struct usb_device *udev)
{
	int ret, i;
	unsigned long flags;
	struct xhci_hcd *xhci;
	unsigned int slot_id;
	struct xhci_virt_device *virt_dev;
	struct xhci_command *reset_device_cmd;
	int timeleft;
	int last_freed_endpoint;
	struct xhci_slot_ctx *slot_ctx;

	ret = xhci_check_args(hcd, udev, NULL, 0, false, __func__);
	if (ret <= 0)
		return ret;
	xhci = hcd_to_xhci(hcd);
	slot_id = udev->slot_id;
	virt_dev = xhci->devs[slot_id];

 			if (!virt_dev) {
 							xhci_dbg(xhci, "The device to be reset with slot ID %u does "
 															"not exist. Re-allocate the device\n", slot_id);
 							ret = xhci_alloc_dev(hcd, udev);
 							if (ret == 1)
 											return 0;
 							else
 											return -EINVAL;
 			}

 			if (virt_dev->udev != udev) {
 							 
 							xhci_dbg(xhci, "The device to be reset with slot ID %u does "
 															"not match the udev. Re-allocate the device\n",
 															slot_id);
 							ret = xhci_alloc_dev(hcd, udev);
 							if (ret == 1)
 											return 0;
 							else
 											return -EINVAL;
 			}

 			slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->out_ctx);
 			if (GET_SLOT_STATE(le32_to_cpu(slot_ctx->dev_state)) ==
 																							SLOT_STATE_DISABLED)
 							return 0;

	xhci_dbg(xhci, "Resetting device with slot ID %u\n", slot_id);
	 
	reset_device_cmd = xhci_alloc_command(xhci, false, true, GFP_NOIO);
	if (!reset_device_cmd) {
		xhci_dbg(xhci, "Couldn't allocate command structure.\n");
		return -ENOMEM;
	}

	spin_lock_irqsave(&xhci->lock, flags);
	reset_device_cmd->command_trb = xhci->cmd_ring->enqueue;

 			if (TRB_TYPE_LINK_LE32(reset_device_cmd->command_trb->link.control))
 							reset_device_cmd->command_trb =
 											xhci->cmd_ring->enq_seg->next->trbs;

	list_add_tail(&reset_device_cmd->cmd_list, &virt_dev->cmd_list);
	ret = xhci_queue_reset_device(xhci, slot_id);
	if (ret) {
		xhci_dbg(xhci, "FIXME: allocate a command ring segment\n");
		list_del(&reset_device_cmd->cmd_list);
		spin_unlock_irqrestore(&xhci->lock, flags);
		goto command_cleanup;
	}
	xhci_ring_cmd_db(xhci);
	spin_unlock_irqrestore(&xhci->lock, flags);

	timeleft = wait_for_completion_interruptible_timeout(
			reset_device_cmd->completion,
#ifdef MY_ABC_HERE
			USB_CTRL_SET_TIMEOUT/5);
#else
			USB_CTRL_SET_TIMEOUT);
#endif
	if (timeleft <= 0) {
		xhci_warn(xhci, "%s while waiting for reset device command\n",
				timeleft == 0 ? "Timeout" : "Signal");
		spin_lock_irqsave(&xhci->lock, flags);
		 
		if (reset_device_cmd->cmd_list.next != LIST_POISON1)
			list_del(&reset_device_cmd->cmd_list);
		spin_unlock_irqrestore(&xhci->lock, flags);
		ret = -ETIME;
		goto command_cleanup;
	}

	ret = reset_device_cmd->status;
	switch (ret) {
	case COMP_EBADSLT:  
	case COMP_CTX_STATE:  
		xhci_info(xhci, "Can't reset device (slot ID %u) in %s state\n",
				slot_id,
				xhci_get_slot_state(xhci, virt_dev->out_ctx));
		xhci_info(xhci, "Not freeing device rings.\n");
		 
		ret = 0;
		goto command_cleanup;
	case COMP_SUCCESS:
		xhci_dbg(xhci, "Successful reset device command.\n");
		break;
	default:
		if (xhci_is_vendor_info_code(xhci, ret))
			break;
		xhci_warn(xhci, "Unknown completion code %u for "
				"reset device command.\n", ret);
		ret = -EINVAL;
		goto command_cleanup;
	}

	last_freed_endpoint = 1;
	for (i = 1; i < 31; ++i) {
		if (!virt_dev->eps[i].ring)
			continue;
		xhci_free_or_cache_endpoint_ring(xhci, virt_dev, i);
		last_freed_endpoint = i;
	}
	xhci_dbg(xhci, "Output context after successful reset device cmd:\n");
	xhci_dbg_ctx(xhci, virt_dev->out_ctx, last_freed_endpoint);
	ret = 0;

command_cleanup:
	xhci_free_command(xhci, reset_device_cmd);
	return ret;
}

void xhci_free_dev(struct usb_hcd *hcd, struct usb_device *udev)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	struct xhci_virt_device *virt_dev;
	unsigned long flags;
	u32 state;
	int i, ret;

	ret = xhci_check_args(hcd, udev, NULL, 0, true, __func__);
	if (ret <= 0)
		return;
	virt_dev = xhci->devs[udev->slot_id];

	for (i = 0; i < 31; ++i) {
		virt_dev->eps[i].ep_state &= ~EP_HALT_PENDING;
		del_timer_sync(&virt_dev->eps[i].stop_cmd_timer);
	}

	spin_lock_irqsave(&xhci->lock, flags);
	 
	state = xhci_readl(xhci, &xhci->op_regs->status);
	if (state == 0xffffffff || (xhci->xhc_state & XHCI_STATE_DYING)) {
		xhci_free_virt_device(xhci, udev->slot_id);
		spin_unlock_irqrestore(&xhci->lock, flags);
		return;
	}

	if (xhci_queue_slot_control(xhci, TRB_DISABLE_SLOT, udev->slot_id)) {
		spin_unlock_irqrestore(&xhci->lock, flags);
		xhci_dbg(xhci, "FIXME: allocate a command ring segment\n");
		return;
	}
	xhci_ring_cmd_db(xhci);
	spin_unlock_irqrestore(&xhci->lock, flags);
	 
}

int xhci_alloc_dev(struct usb_hcd *hcd, struct usb_device *udev)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	unsigned long flags;
	int timeleft;
	int ret;

#ifdef MY_ABC_HERE
	msleep(1000);  
#endif

	spin_lock_irqsave(&xhci->lock, flags);
	ret = xhci_queue_slot_control(xhci, TRB_ENABLE_SLOT, 0);
	if (ret) {
		spin_unlock_irqrestore(&xhci->lock, flags);
		xhci_dbg(xhci, "FIXME: allocate a command ring segment\n");
		return 0;
	}
	xhci_ring_cmd_db(xhci);
	spin_unlock_irqrestore(&xhci->lock, flags);

	timeleft = wait_for_completion_interruptible_timeout(&xhci->addr_dev,
#ifdef MY_ABC_HERE
			USB_CTRL_SET_TIMEOUT/5);
#else
			USB_CTRL_SET_TIMEOUT);
#endif
	if (timeleft <= 0) {
		xhci_warn(xhci, "%s while waiting for a slot\n",
				timeleft == 0 ? "Timeout" : "Signal");
		 
		return 0;
	}

	if (!xhci->slot_id) {
		xhci_err(xhci, "Error while assigning device slot ID\n");
		return 0;
	}
	 
	if (!xhci_alloc_virt_device(xhci, xhci->slot_id, udev, GFP_NOIO)) {

		xhci_warn(xhci, "Could not allocate xHCI USB device data structures\n");
		spin_lock_irqsave(&xhci->lock, flags);
		if (!xhci_queue_slot_control(xhci, TRB_DISABLE_SLOT, udev->slot_id))
			xhci_ring_cmd_db(xhci);
		spin_unlock_irqrestore(&xhci->lock, flags);
		return 0;
	}
	udev->slot_id = xhci->slot_id;
	 
	return 1;
}

int xhci_address_device(struct usb_hcd *hcd, struct usb_device *udev)
{
	unsigned long flags;
	int timeleft;
	struct xhci_virt_device *virt_dev;
	int ret = 0;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	struct xhci_slot_ctx *slot_ctx;
	struct xhci_input_control_ctx *ctrl_ctx;
	u64 temp_64;

	if (!udev->slot_id) {
		xhci_dbg(xhci, "Bad Slot ID %d\n", udev->slot_id);
		return -EINVAL;
	}

	virt_dev = xhci->devs[udev->slot_id];

	slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->in_ctx);
	 
	if (!slot_ctx->dev_info)
		xhci_setup_addressable_virt_dev(xhci, udev);
	 
	else
		xhci_copy_ep0_dequeue_into_input_ctx(xhci, udev);

	xhci_dbg(xhci, "Slot ID %d Input Context:\n", udev->slot_id);
	xhci_dbg_ctx(xhci, virt_dev->in_ctx, 2);

	spin_lock_irqsave(&xhci->lock, flags);
	ret = xhci_queue_address_device(xhci, virt_dev->in_ctx->dma,
					udev->slot_id);
	if (ret) {
		spin_unlock_irqrestore(&xhci->lock, flags);
		xhci_dbg(xhci, "FIXME: allocate a command ring segment\n");
		return ret;
	}
	xhci_ring_cmd_db(xhci);
	spin_unlock_irqrestore(&xhci->lock, flags);

	timeleft = wait_for_completion_interruptible_timeout(&xhci->addr_dev,
#ifdef MY_ABC_HERE
			USB_CTRL_SET_TIMEOUT/5);
#else
			USB_CTRL_SET_TIMEOUT);
#endif

	if (timeleft <= 0) {
		xhci_warn(xhci, "%s while waiting for a slot\n",
				timeleft == 0 ? "Timeout" : "Signal");
		 
		return -ETIME;
	}

	switch (virt_dev->cmd_status) {
	case COMP_CTX_STATE:
	case COMP_EBADSLT:
		xhci_err(xhci, "Setup ERROR: address device command for slot %d.\n",
				udev->slot_id);
		ret = -EINVAL;
		break;
	case COMP_TX_ERR:
		dev_warn(&udev->dev, "Device not responding to set address.\n");
		ret = -EPROTO;
		break;
	case COMP_SUCCESS:
		xhci_dbg(xhci, "Successful Address Device command\n");
		break;
	default:
		xhci_err(xhci, "ERROR: unexpected command completion "
				"code 0x%x.\n", virt_dev->cmd_status);
		xhci_dbg(xhci, "Slot ID %d Output Context:\n", udev->slot_id);
		xhci_dbg_ctx(xhci, virt_dev->out_ctx, 2);
		ret = -EINVAL;
		break;
	}
	if (ret) {
		return ret;
	}
	temp_64 = xhci_read_64(xhci, &xhci->op_regs->dcbaa_ptr);
	xhci_dbg(xhci, "Op regs DCBAA ptr = %#016llx\n", temp_64);
	xhci_dbg(xhci, "Slot ID %d dcbaa entry @%p = %#016llx\n",
			udev->slot_id,
			&xhci->dcbaa->dev_context_ptrs[udev->slot_id],
			(unsigned long long)
				le64_to_cpu(xhci->dcbaa->dev_context_ptrs[udev->slot_id]));
	xhci_dbg(xhci, "Output Context DMA address = %#08llx\n",
			(unsigned long long)virt_dev->out_ctx->dma);
	xhci_dbg(xhci, "Slot ID %d Input Context:\n", udev->slot_id);
	xhci_dbg_ctx(xhci, virt_dev->in_ctx, 2);
	xhci_dbg(xhci, "Slot ID %d Output Context:\n", udev->slot_id);
	xhci_dbg_ctx(xhci, virt_dev->out_ctx, 2);
	 
	slot_ctx = xhci_get_slot_ctx(xhci, virt_dev->out_ctx);
	 
	virt_dev->address = (le32_to_cpu(slot_ctx->dev_state) & DEV_ADDR_MASK) + 1;
	 
	ctrl_ctx = xhci_get_input_control_ctx(xhci, virt_dev->in_ctx);
	ctrl_ctx->add_flags = 0;
	ctrl_ctx->drop_flags = 0;

	xhci_dbg(xhci, "Internal device address = %d\n", virt_dev->address);

	return 0;
}

int xhci_update_hub_device(struct usb_hcd *hcd, struct usb_device *hdev,
			struct usb_tt *tt, gfp_t mem_flags)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	struct xhci_virt_device *vdev;
	struct xhci_command *config_cmd;
	struct xhci_input_control_ctx *ctrl_ctx;
	struct xhci_slot_ctx *slot_ctx;
	unsigned long flags;
	unsigned think_time;
	int ret;

	if (!hdev->parent)
		return 0;

	vdev = xhci->devs[hdev->slot_id];
	if (!vdev) {
		xhci_warn(xhci, "Cannot update hub desc for unknown device.\n");
		return -EINVAL;
	}
	config_cmd = xhci_alloc_command(xhci, true, true, mem_flags);
	if (!config_cmd) {
		xhci_dbg(xhci, "Could not allocate xHCI command structure.\n");
		return -ENOMEM;
	}

	spin_lock_irqsave(&xhci->lock, flags);
	xhci_slot_copy(xhci, config_cmd->in_ctx, vdev->out_ctx);
	ctrl_ctx = xhci_get_input_control_ctx(xhci, config_cmd->in_ctx);
	ctrl_ctx->add_flags |= cpu_to_le32(SLOT_FLAG);
	slot_ctx = xhci_get_slot_ctx(xhci, config_cmd->in_ctx);
	slot_ctx->dev_info |= cpu_to_le32(DEV_HUB);
	if (tt->multi)
		slot_ctx->dev_info |= cpu_to_le32(DEV_MTT);
	if (xhci->hci_version > 0x95) {
		xhci_dbg(xhci, "xHCI version %x needs hub "
				"TT think time and number of ports\n",
				(unsigned int) xhci->hci_version);
		slot_ctx->dev_info2 |= cpu_to_le32(XHCI_MAX_PORTS(hdev->maxchild));
		 
		think_time = tt->think_time;
		if (think_time != 0)
			think_time = (think_time / 666) - 1;
		slot_ctx->tt_info |= cpu_to_le32(TT_THINK_TIME(think_time));
	} else {
		xhci_dbg(xhci, "xHCI version %x doesn't need hub "
				"TT think time or number of ports\n",
				(unsigned int) xhci->hci_version);
	}
	slot_ctx->dev_state = 0;
	spin_unlock_irqrestore(&xhci->lock, flags);

	xhci_dbg(xhci, "Set up %s for hub device.\n",
			(xhci->hci_version > 0x95) ?
			"configure endpoint" : "evaluate context");
	xhci_dbg(xhci, "Slot %u Input Context:\n", hdev->slot_id);
	xhci_dbg_ctx(xhci, config_cmd->in_ctx, 0);

	if (xhci->hci_version > 0x95)
		ret = xhci_configure_endpoint(xhci, hdev, config_cmd,
				false, false);
	else
		ret = xhci_configure_endpoint(xhci, hdev, config_cmd,
				true, false);

	xhci_dbg(xhci, "Slot %u Output Context:\n", hdev->slot_id);
	xhci_dbg_ctx(xhci, vdev->out_ctx, 0);

	xhci_free_command(xhci, config_cmd);
	return ret;
}

int xhci_get_frame(struct usb_hcd *hcd)
{
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);
	 
	return xhci_readl(xhci, &xhci->run_regs->microframe_index) >> 3;
}

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_LICENSE("GPL");

static int __init xhci_hcd_init(void)
{
#ifdef CONFIG_PCI
	int retval = 0;

	retval = xhci_register_pci();

	if (retval < 0) {
		printk(KERN_DEBUG "Problem registering PCI driver.");
		return retval;
	}
#endif
	 
	BUILD_BUG_ON(sizeof(struct xhci_doorbell_array) != 256*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_slot_ctx) != 8*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_ep_ctx) != 8*32/8);
	 
	BUILD_BUG_ON(sizeof(struct xhci_stream_ctx) != 4*32/8);
	BUILD_BUG_ON(sizeof(union xhci_trb) != 4*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_erst_entry) != 4*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_cap_regs) != 7*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_intr_reg) != 8*32/8);
	 
	BUILD_BUG_ON(sizeof(struct xhci_run_regs) != (8+8*128)*32/8);
	BUILD_BUG_ON(sizeof(struct xhci_doorbell_array) != 256*32/8);
	return 0;
}
module_init(xhci_hcd_init);

static void __exit xhci_hcd_cleanup(void)
{
#ifdef CONFIG_PCI
	xhci_unregister_pci();
#endif
}
module_exit(xhci_hcd_cleanup);

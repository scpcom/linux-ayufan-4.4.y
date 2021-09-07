/**************************************************************************
 *
 *  Copyright (c) 2007,2009 Oxford Semiconductor Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  Module Name:
 *      ox810sata.c
 *
 *  Abstract:
 *      A driver to interface the OX810 SATA core
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/memory.h>
#include <mach/ox810sata.h>
#include <mach/sata_helper.h>
#include <linux/libata.h>

#include "libata.h"

#define DRIVER_AUTHOR   "Oxford Semiconductor Ltd."
#define DRIVER_DESC     "810 SATA core controler"
#define DRIVER_NAME     "ox810sata"

MODULE_LICENSE("GPL");
MODULE_VERSION(1.0);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

#if defined(CONFIG_ARCH_OXNAS_FPGA) && defined(CONFIG_ARCH_OX820)
#warning "Limiting SATA link to 1.5Gb/s"
#define LIMIT_TO_1pt5Gbs
#endif // CONFIG_ARCH_OX820
//#define SATA_TF_DUMP
//#define ERROR_INJECTION
//#define CRAZY_DUMP_DEBUG

#undef OXNAS_SATA_DEBUG

#ifdef OXNAS_SATA_DEBUG
#define OPRINTK(fmt, args...) printk(KERN_ERR "%s: " fmt, __func__, ## args)
#else	// OXNAS_SATA_DEBUG
#define OPRINTK(fmt, args...)
#endif	// OXNAS_SATA_DEBUG

#define SATA_ABORT_WAIT_MS 5000
#define SATA_SRST_WAIT_MS  5000

static spinlock_t access_lock = SPIN_LOCK_UNLOCKED;
static int core_locked = 0;
static int reentrant_port_no = -1;
static DECLARE_WAIT_QUEUE_HEAD(sata_wait_queue);

static volatile ox810sata_isr_callback_t ox810sata_isr_callback = 0;
static volatile unsigned long ox810sata_isr_arg = 0;

/* To track ODRB SG list ownership */
static odrb_sg_list_t * volatile odrb_sg = 0;

/*
 * Forward declarations for ata_port_operations functions
 */
static int ox810sata_qc_defer(struct ata_queued_cmd *qc);
static void ox810sata_qc_prep(struct ata_queued_cmd *qc);
static unsigned int ox810sata_qc_issue(struct ata_queued_cmd *qc);
static bool ox810sata_qc_fill_rtf(struct ata_queued_cmd *qc);

static int ox810sata_qc_new(struct ata_port *ap);
static void ox810sata_qc_free(struct ata_queued_cmd *qc);

static void ox810sata_freeze(struct ata_port *ap);
static void ox810sata_thaw(struct ata_port *ap);
static int ox810sata_softreset(struct ata_link *link, unsigned int *classes, unsigned long deadline);
static int ox810sata_hardreset(struct ata_link *link, unsigned int *class, unsigned long deadline);
static int ox810sata_prereset(struct ata_link *link, unsigned long deadline);
static void ox810sata_postreset(struct ata_link *link, unsigned int *classes);

static void ox810sata_error_handler(struct ata_port *ap);
static void ox810sata_post_internal_cmd(struct ata_queued_cmd *qc);
static void ox810sata_dev_config(struct ata_device *);

static int ox810sata_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val);
static int ox810sata_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val);

static int ox810sata_port_start(struct ata_port *ap);
static void ox810sata_port_stop(struct ata_port *ap);
static void ox810sata_host_stop(struct ata_host *host_set);

static int acquire_hw(int port_no, int may_sleep, int timeout_jiffies);

typedef struct ox810sata_port_private_data {
    u32 ErrorsWithNoCommamnd;
    u32 int_status;
    u32 in_cleanup;
	u32 *port_base;
} ox810sata_port_private_data_t;

typedef struct ox810sata_host_private_data {
    u32 error_inject;
    u32 hw_raid_active;
    int active_port;
} ox810sata_host_private_data_t;

static struct scsi_host_template ox810sata_sht = 
{
	ATA_BASE_SHT(DRV_NAME),
    .unchecked_isa_dma  = 0,
    .sg_tablesize       = CONFIG_ARCH_OXNAS_MAX_SATA_SG_ENTRIES,
    .dma_boundary       = ~0UL, // NAS has no DMA boundary restrictions
};

typedef struct ox810sata_driver {
    struct kobject kobj;
    struct platform_driver driver;
    struct ata_host* host;
} ox810sata_driver_t;

static int ox810sata_probe(struct platform_device *);
static int ox810sata_remove(struct platform_device *);

static ox810sata_driver_t ox810sata_driver = 
{
    .driver = {
        .driver.name = DRIVER_NAME,
        .driver.bus = &platform_bus_type,
        .probe = ox810sata_probe, 
        .remove = ox810sata_remove,
    },
    .host = 0,
};

static struct ata_port_operations ox810sata_port_ops =
{
	.inherits				= &sata_port_ops,

	.qc_defer				= ox810sata_qc_defer,
	.qc_prep				= ox810sata_qc_prep,
	.qc_issue				= ox810sata_qc_issue,
	.qc_fill_rtf			= ox810sata_qc_fill_rtf,

	.qc_new					= ox810sata_qc_new,
	.qc_free				= ox810sata_qc_free,

	.freeze					= ox810sata_freeze,
	.thaw					= ox810sata_thaw,
	.softreset				= ox810sata_softreset,
	.hardreset				= ox810sata_hardreset,
	.prereset				= ox810sata_prereset,
	.postreset				= ox810sata_postreset,

    .error_handler			= ox810sata_error_handler,
    .post_internal_cmd		= ox810sata_post_internal_cmd,
    .dev_config				= ox810sata_dev_config,

    .scr_read				= ox810sata_scr_read,
    .scr_write				= ox810sata_scr_write,

    .port_start				= ox810sata_port_start,
    .port_stop				= ox810sata_port_stop,
	.host_stop				= ox810sata_host_stop,

	.acquire_hw				= acquire_hw,
};

static const struct ata_port_info ox810sata_port_info = {
    .flags = ATA_FLAG_SATA |
			 ATA_FLAG_SATA_RESET |
			 ATA_FLAG_NO_LEGACY |
			 ATA_FLAG_NO_ATAPI |
			 ATA_FLAG_PIO_DMA,
    .pio_mask   = 0x1f, /* pio modes 0..4*/
    .mwdma_mask = 0x07, /* mwdma0-2 */
    .udma_mask  = 0x7f, /* udma0-5 */
    .port_ops   = &ox810sata_port_ops,
};

/** 
 * Outputs all the registers in the SATA core for diagnosis of faults.
 *
 * @param ap Hardware with the registers in
 */
#ifndef CRAZY_DUMP_DEBUG
void CrazyDumpDebug(void) {}
#else // CRAZY_DUMP_DEBUG
void CrazyDumpDebug(void)
{
    u32 offset;
    u32 result;
    u32 patience;
    volatile u32* ioaddr;

    printk("Port 0 High level registers\n");
    ioaddr = (u32*)SATA0_REGS_BASE;
    for (offset = 0; offset < 48; offset++) {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    printk("Port 0 link layer registers\n");
    for (offset = 0; offset < 16; ++offset) {
        *(ioaddr + OX810SATA_LINK_RD_ADDR) = (offset*4);
        wmb();
    
        for (patience = 0x100000; patience > 0; --patience) {
            if (*(ioaddr + OX810SATA_LINK_CONTROL) & 0x00000001)
                break;
        }
    
        result = *(ioaddr + OX810SATA_LINK_DATA);
        printk("[%02x] %08x\n", offset*4, result);
    }

    printk("Port 1 High level registers\n");
    ioaddr = (u32*)SATA1_REGS_BASE;
    for (offset = 0; offset < 48; offset++) {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    printk("Port 1 link layer registers\n");
    for (offset = 0; offset < 16; ++offset) {
        *(ioaddr + OX810SATA_LINK_RD_ADDR) = (offset*4);
        wmb();

        for (patience = 0x100000; patience > 0; --patience) {
            if (*(ioaddr + OX810SATA_LINK_CONTROL) & 0x00000001)
                break;
        }

        result = *(ioaddr + OX810SATA_LINK_DATA);
        printk("[%02x] %08x\n", offset*4, result);
    }

    printk("RAID registers\n");
    ioaddr = (u32*)SATARAID_REGS_BASE;
    for (offset = 0; offset < 48; offset++) {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    printk("CORE registers\n");
    ioaddr = (u32*)SATACORE_REGS_BASE;
    for (offset = 0; offset < 48; offset++) {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    printk("DMA registers\n");
    ioaddr = (u32*)SATADMA_REGS_BASE;
    for (offset = 0; offset < 48; offset++) {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    printk("SG-DMA registers\n");
    ioaddr = (u32*)SATASGDMA_REGS_BASE;
    for (offset = 0; offset < 48; offset++) {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }
}
#endif // CRAZY_DUMP_DEBUG
EXPORT_SYMBOL(CrazyDumpDebug);

static void ox810sata_reset_core(void)
{
DPRINTK("ENTER\n");
    // Assert reset to controller, link and PHY
    writel( (1UL << SYS_CTRL_RSTEN_SATA_BIT)      |
            (1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT) |
            (1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT), SYS_CTRL_RSTEN_SET_CTRL);
    wmb();
    udelay(50);

    // Deassert reset to controller
    writel(1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
    wmb();
    udelay(50);

    // Deassert reset to link and PHY
    writel( (1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT) |
            (1UL << SYS_CTRL_RSTEN_SATA_BIT), SYS_CTRL_RSTEN_CLR_CTRL);
    wmb();
    udelay(50);
}

/**
 * Gets the base address of the ata core from the ata_port structure. The value
 * returned will remain the same when hardware raid is active.
 *
 * @param ap pointer to the appropriate ata_port structure
 * @return the base address of the SATA core
 */
static inline u32* ox810sata_get_io_base(struct ata_port* ap)
{
    return ((ox810sata_port_private_data_t*)(ap->private_data))->port_base;
}

/**
 * Gets the base of the ox810 port associated with the ata-port as known
 * by lib-ata, The value returned changes to the single RAID port when
 * hardware RAID commands are active.
 * 
 * @param ap pointer to the appropriate ata_port structure
 * @return the base address of the SATA core
 */
static inline u32* ox810sata_get_tfio_base(struct ata_port* ap)
{
	ox810sata_host_private_data_t *pd = ox810sata_driver.host->private_data;

    if ((pd->hw_raid_active) && (pd->active_port == ap->port_no)) {
        return (u32*)SATARAID_REGS_BASE;
    } else {
        return ox810sata_get_io_base(ap);
    }
}

static void ox810sata_post_reset_init(struct ata_port* ap) 
{
    u32  patience;
    u32* ioaddr = ox810sata_get_io_base(ap);
DPRINTK("port_no %d\n", ap->port_no);

    /* Enable phy error detection */ 
    writel(0x30003, ioaddr + OX810SATA_LINK_DATA );
    wmb();
    writel(0x0C, ioaddr + OX810SATA_LINK_WR_ADDR );
    wmb();
    for (patience = 0x100000; patience > 0;--patience) {
        if (readl(ioaddr + OX810SATA_LINK_CONTROL) & 0x00000001) {
            break;
		}
    }
	if (!patience) {
		printk(KERN_WARNING "ox810sata_post_reset_init() Timed-out of wait for link async write to complete\n");
	}

    /* Enable interrupts for ports  */
VPRINTK("Enable interrupts\n");
    writel(~0, OX810SATA_CORE_IEC);  
    writel(OX810SATA_NORMAL_INTS_WANTED, OX810SATA_CORE_IES);  
}

static void ox810sata_cleanup(struct ata_port *ap)
{
    u32 reg;
    u32 patience;
DPRINTK("port_no %d\n", ap->port_no);

    /* Clear error bits in both ports */
    reg = readl((u32*)SATA0_REGS_BASE + OX810SATA_SATA_CONTROL);
    reg |= OX810SATA_SCTL_CLR_ERR ;
    writel(reg, (u32*)SATA0_REGS_BASE + OX810SATA_SATA_CONTROL);
    reg = readl((u32*)SATA1_REGS_BASE + OX810SATA_SATA_CONTROL);
    reg |= OX810SATA_SCTL_CLR_ERR ;
    writel(reg, (u32*)SATA1_REGS_BASE + OX810SATA_SATA_CONTROL);
    reg = readl((u32* )SATARAID_REGS_BASE + OX810SATA_SATA_CONTROL);
    reg |= OX810SATA_SCTL_CLR_ERR ;
    writel(reg, (u32* )SATARAID_REGS_BASE + OX810SATA_SATA_CONTROL);

    /* Test SATA core idle state */
DPRINTK("Before idle status 0x%08x\n", readl(OX810SATA_IDLE_STATUS));
    if (!(~readl(OX810SATA_IDLE_STATUS) & OX810SATA_IDLE_CORES)) {
DPRINTK("Both ports are idle, status 0x%08x\n", readl(OX810SATA_IDLE_STATUS));
        return;
	}

    /* Initiate SATA core abort */
DPRINTK("Aborting transfer at SATA core\n");
    reg = readl(OX810SATA_DEVICE_CONTROL);
    writel(reg | OX810SATA_DEVICE_CONTROL_ABORT, OX810SATA_DEVICE_CONTROL);
DPRINTK("After abort sent status 0x%08x\n", readl(OX810SATA_IDLE_STATUS));

    /* Wait for the abort to take effect */
    patience = 1000;
    do {
        if (!(~readl(OX810SATA_IDLE_STATUS) & OX810SATA_IDLE_CORES)) {
            break;
        }
        udelay(50);
    } while (--patience);

    /* Terminate the SATA core abort */
	writel(reg & ~OX810SATA_DEVICE_CONTROL_ABORT, OX810SATA_DEVICE_CONTROL);
	if (patience) {
DPRINTK("Both ports now idle, status 0x%08x\n", readl(OX810SATA_IDLE_STATUS));
		return;
	} else {
		printk(KERN_WARNING "ox810sata_cleanup() Timed-out of wait for SATA abort to complete\n");
	}

    /* Send a sync escape to port 0 */
DPRINTK("Sending sync escape to port 0\n");
    reg = readl((u32*)SATA0_REGS_BASE + OX810SATA_SATA_COMMAND);
    reg &= ~SATA_OPCODE_MASK;
    reg |= CMD_SYNC_ESCAPE;
    writel(reg, (u32*)SATA0_REGS_BASE + OX810SATA_SATA_COMMAND);

    /* Wait for the sync escape to take effect */
    patience = 1000;
    do {
        if (!(~readl(OX810SATA_IDLE_STATUS) & OX810SATA_IDLE_CORES)) {
DPRINTK("Both ports now idle, status 0x%08x\n", readl(OX810SATA_IDLE_STATUS));
            return;
        }
        udelay(50);
    } while (--patience);
	if (!patience) {
		printk(KERN_WARNING "ox810sata_cleanup() Timed-out of wait for sync escape on port 0 to take effect\n");
	}

    /* Send a sync escape to port 1 */
DPRINTK("Sending sync escape to port 1\n");
    reg = readl((u32*)SATA1_REGS_BASE + OX810SATA_SATA_COMMAND);
    reg &= ~SATA_OPCODE_MASK;
    reg |= CMD_SYNC_ESCAPE;
    writel(reg, (u32*)SATA1_REGS_BASE + OX810SATA_SATA_COMMAND);

    /* Wait for the sync escape to take effect */
    patience = 1000;
    do {
        if (!(~readl(OX810SATA_IDLE_STATUS) & OX810SATA_IDLE_CORES)) {
DPRINTK("Both ports now idle, status 0x%08x\n", readl(OX810SATA_IDLE_STATUS));
            return;
        }
        udelay(50);
    } while (--patience);
	if (!patience) {
		printk(KERN_WARNING "ox810sata_cleanup() Timed-out of wait for sync escape on port 1 to take effect\n");
	}

    /* SATA core did not go idle, so cause a SATA core reset from the RPS */
	CrazyDumpDebug();
    ox810sata_reset_core();

    /* Read SATA core idle state */
    if (~readl(OX810SATA_IDLE_STATUS) & OX810SATA_IDLE_CORES) {
        printk(KERN_WARNING "ox810sata_cleanup() All attempts to clear SATA core problems failed\n");
        CrazyDumpDebug();
    }

	/* Seems fairly pointless at this point, but attempt to get the SATA core
	   started on the road back to operation */
    ox810sata_post_reset_init(ox810sata_driver.host->ports[0]);
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
    ox810sata_post_reset_init(ox810sata_driver.host->ports[1]);
#endif // CONFIG_SATA_OXNAS_SINGLE_SATA
}

/** 
 * irq_handler is the interrupt handling routine registered with the system,
 * by libata.
 */
static irqreturn_t ox810sata_irq_handler(
	int   irq,
	void *dev_instance)
{
	struct ata_host               *host = (struct ata_host *)dev_instance;
	int                            port_num = 0;
	struct ata_port               *ap;
	ox810sata_port_private_data_t *port_pd;
	ox810sata_host_private_data_t *host_pd;
	u32                            int_status;
	u32                           *ioaddr;

DPRINTK("irq %d\n", irq);
	if (ox810sata_isr_callback) {
		/* Invoke the interrupt hook routine */
		return (*ox810sata_isr_callback)(irq, ox810sata_isr_arg);
	}

#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
	for (; port_num < 2; port_num++) {
#endif // CONFIG_SATA_OXNAS_SINGLE_SATA
		ap = host->ports[port_num];
		ioaddr = ox810sata_get_tfio_base(ap);
		port_pd = (ox810sata_port_private_data_t*)ap->private_data;
		host_pd = (ox810sata_host_private_data_t*)ap->host->private_data;

		/* check the ISR for the port to see if it created the interrupt */
		int_status = readl(ioaddr + OX810SATA_INT_STATUS);
		if (int_status & OX810SATA_INT_WANT) {
			const int MAX_TIMEOUT_LOOPS = 20;
			int count;
			struct ata_queued_cmd* qc;
			int sata_status;
DPRINTK("IRQ was for port %d, status 0x%08x\n", port_num, int_status);

			/* Clear and mask asserted interrupts */
			writel(int_status, ioaddr + OX810SATA_INT_CLEAR);
			writel(int_status, ioaddr + OX810SATA_INT_DISABLE);

			port_pd->int_status = int_status;

			/* Get the ATA queued command that initiated the SATA transfer */
			qc = ata_qc_from_tag(ap, ap->link.active_tag);
			if (qc) {
				/* Wait a little while for the DMA to finish after the SATA has
				   finished. For writes there should be no delay between SATA
				   interrupt and DMA finishing, whereas for reads there should only
				   be a delay while the final SATA core buffer full of data is DMAed
				   to memory, i.e. a short time */
				count = 0;
				while (odrb_dma_isactive(qc->n_elem > 1) &&
					   (count++ < MAX_TIMEOUT_LOOPS)) {
					udelay(1);
				}
				if (count > MAX_TIMEOUT_LOOPS) {
					printk(KERN_WARNING "ox810sata_irq_handler() Wait for DMA to finish timed-out\n");
				}

				/* Force DMA to finish if it hasn't already */
				odrb_dma_abort(qc->n_elem > 1);

				/* Housekeeping after completion of the DMA transfer */
				odrb_dma_postop_housekeeping(qc->n_elem > 1);
			}

			/* Extract any error conditions from the SATA core */
			sata_status = sata_transfer_complete(ioaddr);
			if (sata_status < 0) {
				// Record that the transfer failed, so the block(s) can be disgarded
printk("ox810sata_irq_handler() Transfer failed, status %d\n", sata_status);
				if (qc) {
printk("ox810sata_irq_handler() Marking qc->err_mask as AC_ERR_DEV\n");
					qc->err_mask = AC_ERR_DEV;
				}
			}

			/* Clear any problem state from the SATA core */
			ox810sata_cleanup(ap);

#ifdef CONFIG_SATA_OXNAS_DISK_LIGHT            
			/* disk light off */
			writel(1 << CONFIG_OX810SATA_DISK_LIGHT_GPIO_LINE, GPIO_A_OUTPUT_CLEAR);
#endif  /* CONFIG_SATA_OXNAS_DISK_LIGHT */

DPRINTK("qc %p, tag %d\n", qc, ap->link.active_tag);
			if (qc) {
				ata_qc_complete(qc);
			}
		}
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
	}
#endif // CONFIG_SATA_OXNAS_SINGLE_SATA

    return IRQ_HANDLED;
}

/** 
 * The driver probe function.
 * Registered with the amba bus driver as a parameter of ox810sata_driver.bus
 * it will register the ata device with kernel first performing any 
 * initialisation required (if the correct device is present).
 * @param pdev Pointer to the 810 SATA device structure 
 * @return 0 if no errors
 */
static int ox810sata_probe(struct platform_device* pdev)
{
    const struct ata_port_info    *port_info[2] =
		{ &ox810sata_port_info, &ox810sata_port_info };
    struct ata_host               *host;
    struct resource               *memres;
    void __iomem                  *port0_iomem;
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
    void __iomem                  *port1_iomem;
#endif // !CONFIG_SATA_OXNAS_SINGLE_SATA
    int                            irq;
    u32                            version;
#ifdef CONFIG_SATA_OXNAS_DISK_LIGHT
    unsigned long                  reg;
#endif // CONFIG_SATA_OXNAS_DISK_LIGHT
	ox810sata_host_private_data_t *private_data;
DPRINTK("ENTER\n");

	/* Ensure port 0 memory has been allocated */
    memres = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (memres == NULL) {
        return -ENOMEM;
    }
    port0_iomem = (void __iomem*)memres->start;

#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
	/* Ensure port 1 memory has been allocated */
    memres = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (memres == NULL) {
        return -ENOMEM;
    }
    port1_iomem = (void __iomem*)memres->start;
#endif // !CONFIG_SATA_OXNAS_SINGLE_SATA

    /* Ensure the SATA interrupt line has been allocated */
    irq = platform_get_irq(pdev, 0);
    if (irq < 0) {
        return -ENOMEM;
    }

    /* Enable the clock to the SATA block */
    writel(1UL << SYS_CTRL_CKEN_SATA_BIT, SYS_CTRL_CKEN_SET_CTRL);
	wmb();

	/* Ensure SATA is in a sensible state */
    ox810sata_reset_core();

    /* Check for an 810 SATA core */
    version = readl(((u32*)port0_iomem) + OX810SATA_VERSION);
    switch (version) {
        case OX810SATA_CORE_VERSION:
            printk(KERN_INFO "OX810 SATA host found\n");   
            break;
        case OX820SATA_CORE_VERSION:
            printk(KERN_INFO "OX820 SATA host found\n");   
            break;
        default:
            printk(KERN_ERR "Unknown sata core (version 0x%08x)\n", version);     
            return -ENODEV;
            break;
    }

#ifdef CONFIG_SATA_OXNAS_DISK_LIGHT
    /* setup path */
    reg = ~(1 << CONFIG_OX810SATA_DISK_LIGHT_GPIO_LINE) & readl(SYS_CTRL_GPIO_PRIMSEL_CTRL_0);
    writel(reg, SYS_CTRL_GPIO_PRIMSEL_CTRL_0);
    reg = ~(1 << CONFIG_OX810SATA_DISK_LIGHT_GPIO_LINE) & readl(SYS_CTRL_GPIO_SECSEL_CTRL_0);
    writel(reg, SYS_CTRL_GPIO_SECSEL_CTRL_0);
    reg = ~(1 << CONFIG_OX810SATA_DISK_LIGHT_GPIO_LINE) & readl(SYS_CTRL_GPIO_TERTSEL_CTRL_0);
    writel(reg, SYS_CTRL_GPIO_TERTSEL_CTRL_0);

    /* enable output */
    writel(1 << CONFIG_OX810SATA_DISK_LIGHT_GPIO_LINE, GPIO_A_OUTPUT_ENABLE);

    /* disk light off */
    writel(1 << CONFIG_OX810SATA_DISK_LIGHT_GPIO_LINE, GPIO_A_OUTPUT_CLEAR);
#endif  /* CONFIG_SATA_OXNAS_DISK_LIGHT */

	/* Allocate and zeroise host private data */
	private_data = kzalloc(sizeof(ox810sata_host_private_data_t), GFP_KERNEL);
	if (!private_data) {
		printk(KERN_WARNING "ox810sata_probe() Failed to allocate space for host private data\n");
		return -ENOMEM;
	}

	/* Allocate ATA host */
#ifdef CONFIG_SATA_OXNAS_SINGLE_SATA
    host = ata_host_alloc_pinfo(&(pdev->dev), port_info, 1);
#else // CONFIG_SATA_OXNAS_SINGLE_SATA
    host = ata_host_alloc_pinfo(&(pdev->dev), port_info, 2);
#endif // CONFIG_SATA_OXNAS_SINGLE_SATA
	if (!host) {
        printk(KERN_ERR DRIVER_NAME " Couldn't create an ata host.\n");
		kfree(private_data);
		return -ENOMEM;
    }
	host->private_data = private_data;

	/* Record the base address with each port so port_start can find it later */
	host->ports[0]->private_data = port0_iomem;
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
	host->ports[1]->private_data = port1_iomem;
#endif // !CONFIG_SATA_OXNAS_SINGLE_SATA

	/* Need to be able to get at host and both ports via the driver structure
	   on occasions where the SATA h/w doesn't match the SCSI/SATA stack's idea
	   of everything being referenced through a port */
	ox810sata_driver.host = host;

    /* call ata_device_add and begin probing for drives*/
    ata_host_activate(host, irq, ox810sata_irq_handler, 0, &ox810sata_sht);

    return 0;
}

/** 
 * Called when the amba bus tells this device to remove itself.
 * @param pdev pointer to the device that needs to be shutdown
 */
static int ox810sata_remove(struct platform_device* pdev)
{
    struct ata_host *host = dev_get_drvdata(&(pdev->dev));
	ox810sata_host_private_data_t *host_pd =
		(ox810sata_host_private_data_t*)host->private_data;
    unsigned int i;
DPRINTK("pdev %p\n", pdev);

    for (i=0; i < host->n_ports; i++)  {
        struct ata_port *ap = host->ports[i];
        scsi_remove_host(ap->scsi_host);
    }

	kfree(host_pd);

    // Disable the clock to the SATA block
    writel(1UL << SYS_CTRL_CKEN_SATA_BIT, SYS_CTRL_CKEN_CLR_CTRL);

    return 0;
}

/**
 * A record of which drives have accumulated raid faults. A set bit indicates
 * a fault has occured on that drive */
static u32 ox810sata_accumulated_RAID_faults = 0;

/** 
 * Called after IDENTIFY [PACKET] DEVICE is issued to each device found.
 * Typically used to apply device-specific fixups prior to issue of
 * SET FEATURES - XFER MODE, and prior to operation.
 * @param port The port to configure
 * @param pdev The hardware associated with controlling the port
 */
static void ox810sata_dev_config(struct ata_device* pdev)
{
    u32 reg;
	struct ata_port *ap = pdev->link->ap;
    u32 *ioaddr = ox810sata_get_io_base(ap);
DPRINTK("devno %d, port_no %d\n", pdev->devno, ap->port_no);

    /* Set the bits to put the interface into 28 or 48-bit node */
    reg = readl(ioaddr + OX810SATA_DRIVE_CONTROL);

    /* mask out the pair of bits associaed with each port */
    reg &= ~(3 << (ap->port_no * 2));

    /* set the mode pair associated with each port */
    reg |= ((pdev->flags & ATA_DFLAG_LBA48) ? OX810SATA_DR_CON_48 :
                                              OX810SATA_DR_CON_28) << (ap->port_no * 2);
    writel(reg, ioaddr + OX810SATA_DRIVE_CONTROL);

    /* if this is an ATA-6 disk, put the port into ATA-5 auto translate mode */
    if (pdev->flags & ATA_DFLAG_LBA48) {
        reg = readl(ioaddr + OX810SATA_PORT_CONTROL);
        reg |= 2;
        writel(reg, ioaddr + OX810SATA_PORT_CONTROL);
    }
}

/** 
 * Output the taskfile for diagnostic reasons, it will always appear in the 
 * debug output as if it's a task file being written.
 * @param tf The taskfile to output
 */
#ifndef SATA_TF_DUMP
static inline void tfdump(const struct ata_taskfile* tf) {}
#else // SATA_TF_DUMP
static inline void tfdump(const struct ata_taskfile* tf)
{
    if (tf->flags & ATA_TFLAG_LBA48) {
		printk("Cmd %x Ft %x%x, LBA-48 %02x%02x%02x%02x%02x%02x, nsect %02x%02x, ctl %02x, dev %x\n",
			tf->command, tf->hob_feature, tf->feature, tf->hob_lbah, tf->hob_lbam,
			tf->hob_lbal, tf->lbah, tf->lbam, tf->lbal, tf->hob_nsect, tf->nsect,
			tf->ctl, tf->device);
    } else {
		printk("Cmd %x Ft %x, LBA-28 %01x%02x%02x%02x, nsect %02x, ctl %02x, dev %x\n",
			tf->command, tf->feature, tf->device & 0x0f, tf->lbah, tf->lbam,
			tf->lbal, tf->nsect, tf->ctl, tf->device);
    }
}
#endif // SATA_TF_DUMP

/** 
 * called to write a taskfile into the ORB registers
 * @param ap hardware with the registers in
 * @param tf taskfile to write to the registers
 */
static void ox810sata_tf_load(struct ata_port *ap, const struct ata_taskfile *tf)
{
	u32 count = 0;
	u32 orb1 = 0; 
	u32 orb2 = 0; 
	u32 orb3 = 0;
	u32 orb4 = 0;
	u32 command_reg;
	u32 *ioaddr = ox810sata_get_tfio_base(ap);
DPRINTK("Entered: port_no %d\n", ap->port_no);

	/* Wait a maximum of 10ms for the core to be idle */
	do {
		command_reg = readl(ioaddr + OX810SATA_SATA_COMMAND);
		if (!(command_reg & CMD_CORE_BUSY)) {
			break;
		}
		count++;
		udelay(50);
	} while (count < 200);
	if (unlikely(count == 200)) {
		printk(KERN_WARNING "ox810sata_tf_load() SATA core did not go idle\n");
	}

	/* Only write the control value if it has changed */
	if (tf->ctl != ap->last_ctl) {
DPRINTK("Writing new command 0x%x\n", tf->ctl);
		orb4 |= (tf->ctl) << 24;
		writel(orb4, ioaddr + OX810SATA_ORB4);

		ap->last_ctl = tf->ctl;

		/** @todo find a more elegant way to do this */
		/* if the new control value is a soft reset, command the core to send a
		control FIS */
		if (tf->ctl & ATA_SRST) {
DPRINTK("Send soft reset FIS as a no-command transfer\n");
			command_reg = readl(ioaddr + OX810SATA_SATA_COMMAND);
			command_reg &= ~SATA_OPCODE_MASK;
			command_reg |= CMD_WRITE_TO_ORB_REGS_NO_COMMAND;
			writel(command_reg, ioaddr + OX810SATA_SATA_COMMAND);
		}
	}

	/* Should we write the nsect and lba values? */
	if (tf->flags & ATA_TFLAG_ISADDR) {
DPRINTK("Writing nsect and lba\n");
		/* Set LBA bit as it's an address */
		orb1 |= (tf->device & ATA_LBA) << 24;

		if (tf->flags & ATA_TFLAG_LBA48) {
DPRINTK("LBA48 load\n");
			orb1 |= ATA_LBA << 24;

			orb2 |= (tf->hob_nsect) << 8;

			orb3 |= (tf->hob_lbal) << 24;

			orb4 |= (tf->hob_lbam)    << 0;
			orb4 |= (tf->hob_lbah)    << 8;
			orb4 |= (tf->hob_feature) << 16;
		} else {
			orb3 |= (tf->device & 0xf) << 24;
		}

		/* Write 28-bit lba */
		orb2 |= (tf->nsect)   << 0;
		orb2 |= (tf->feature) << 16;

		orb3 |= (tf->lbal) << 0;
		orb3 |= (tf->lbam) << 8;
		orb3 |= (tf->lbah) << 16;

		orb4 |= (tf->ctl) << 24;

		/* Write values to ORB regs, except ORB2 as there will be the command
		   still to be added to the new contents of ORB2 */
		writel(orb1, ioaddr + OX810SATA_ORB1);
		writel(orb3, ioaddr + OX810SATA_ORB3);
		writel(orb4, ioaddr + OX810SATA_ORB4);
	}

	/* Always write in the command, which shares an ORB register with nsect
	   which is only written if ATA_TFLAG_ISADDR is set in the taskfile */
DPRINTK("Writing command 0x%x\n", tf->command);
	orb2 |= (tf->command) << 24;
	writel(orb2, ioaddr + OX810SATA_ORB2);

	if (tf->flags & ATA_TFLAG_DEVICE) {
DPRINTK("Writing device 0x%x\n", tf->device);
		orb1 |= (tf->device) << 24;

		/* write value to register */
		writel(orb1, ioaddr + OX810SATA_ORB1);
	}

	tfdump(tf);
DPRINTK("Exit:\n");
}

static inline u32 __ox810sata_scr_read(u32* core_addr, unsigned int sc_reg) 
{
    u32 result;
    u32 patience;

    /* we've got 8 other registers in before the start of the standard ones */    
    writel(sc_reg, core_addr + OX810SATA_LINK_RD_ADDR);

    for (patience = 0x100000; patience > 0; --patience) {
        if (readl(core_addr + OX810SATA_LINK_CONTROL) & 0x00000001) {
            break;
		}
    }

    result = readl(core_addr + OX810SATA_LINK_DATA);

#ifdef LIMIT_TO_1pt5Gbs    
    /* 820 FPGA limited to 1.5Gbs */
    if (sc_reg == 0x28) {
        VPRINTK("Reporting a 1.5Gb speed limit\n");
        result |= 0x00000010 ;
    }
#endif    

DPRINTK("core_addr %p, sc_req 0x%08x -> 0x%08x\n", core_addr, sc_reg, result);
    return result;
}

static int ox810sata_scr_read_port(struct ata_port *ap, unsigned int sc_reg, u32 *val)
{
    u32* ioaddr = ox810sata_get_io_base(ap);
	*val = __ox810sata_scr_read(ioaddr, 0x20 + (sc_reg*4));
    return 0;
}

static int ox810sata_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val)
{
	return ox810sata_scr_read_port(link->ap, sc_reg, val);
}

/** 
 * Reads the Status ATA shadow register from hardware. Due to a fault with PIO
 * transfers, it it sometimes necessary to mask out the DRQ bit
 * @param ap hardware with the registers in
 * @return The status register
 */
static u8 ox810sata_check_status(struct ata_port *ap)
{
    u32 Reg = 0;
    u8 status;
    u32 *ioaddr = ox810sata_get_tfio_base(ap);
	ox810sata_host_private_data_t *host_pd =
		(ox810sata_host_private_data_t*)ap->host->private_data;
DPRINTK("port_no %d\n", ap->port_no);

    /* read byte 3 of orb2 register */
    status = readl(ioaddr + OX810SATA_ORB2) >> 24;
DPRINTK("status 0x%08x\n", status);

    /* check for the drive going missing indicated by SCR status bits 0-3 = 0 */
    if (host_pd->hw_raid_active) {
        u32 Temp;
        ox810sata_scr_read_port(ap->host->ports[0], SCR_STATUS, &Temp );
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
        ox810sata_scr_read_port(ap->host->ports[1], SCR_STATUS, &Reg);
#endif // CONFIG_SATA_OXNAS_SINGLE_SATA
        Reg |= Temp;
    } else {
        ox810sata_scr_read_port(ap, SCR_STATUS, &Reg );
    }

DPRINTK("Reg 0x%08x\n", Reg);
    if (!(Reg & 0x1)) { 
        status |= ATA_DF;
        status |= ATA_ERR;
    }

DPRINTK("Exit: status 0x%08x\n", status);
    return status;
}

/** 
 * Called to read the hardware registers / DMA buffers, to
 * obtain the current set of taskfile register values.
 * @param ap hardware with the registers in
 * @param tf taskfile to read the registers into
 */
static void ox810sata_tf_read(struct ata_port *ap, struct ata_taskfile *tf)
{
    u32 *ioaddr = ox810sata_get_tfio_base(ap);

    /* read the orb registers */
    u32 orb1 = readl(ioaddr + OX810SATA_ORB1); 
    u32 orb2 = readl(ioaddr + OX810SATA_ORB2); 
    u32 orb3 = readl(ioaddr + OX810SATA_ORB3);
    u32 orb4 = readl(ioaddr + OX810SATA_ORB4);
DPRINTK("port_no %d\n", ap->port_no);

    /* read common 28/48 bit tf parameters */
    tf->device  = (orb1 >> 24);
    tf->nsect   = (orb2 >> 0);
    tf->feature = (orb2 >> 16);
    tf->command = ox810sata_check_status(ap);

    /* read 48 or 28 bit tf parameters */
    if (tf->flags & ATA_TFLAG_LBA48) {
DPRINTK("48 bit\n");
        tf->hob_nsect = (orb2 >> 8) ;

        tf->lbal      = (orb3 >> 0) ;
        tf->lbam      = (orb3 >> 8) ;
        tf->lbah      = (orb3 >> 16) ;
        tf->hob_lbal  = (orb3 >> 24) ;

        tf->hob_lbam  = (orb4 >> 0) ;
        tf->hob_lbah  = (orb4 >> 8) ;
        /* feature ext and control are write only */
    } else {
        /* read 28-bit lba */
DPRINTK("28 bit\n");
        tf->lbal      = (orb3 >> 0) ;
        tf->lbam      = (orb3 >> 8) ;
        tf->lbah      = (orb3 >> 16) ;
    }

    tfdump(tf);
}

static void __ox810sata_scr_write(u32* core_addr, unsigned int sc_reg, u32 val)
{
    u32 patience;
DPRINTK("core_addr %p, sc_req 0x%08x, val 0x%08x\n", core_addr, sc_reg, val);

    writel(val, core_addr + OX810SATA_LINK_DATA );
    wmb();
    writel(sc_reg , core_addr + OX810SATA_LINK_WR_ADDR );
    wmb();

    for (patience = 0x100000; patience > 0;--patience) {
        if (readl(core_addr + OX810SATA_LINK_CONTROL) & 0x00000001) {
            break;
		}
    }
}

static int ox810sata_scr_write_port(struct ata_port *ap, unsigned int sc_reg, u32 val)
{
    u32 *ioaddr = ox810sata_get_io_base(ap);
    __ox810sata_scr_write(ioaddr, 0x20 + (sc_reg * 4), val);
	return 0;
}

static int ox810sata_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val)
{
	return ox810sata_scr_write_port(link->ap, sc_reg, val);
}

/*
 * To be used by non-SCSI/SATA stack SATA core users to acquire the SATA core
 */
static int __acquire_sata_core(
	int                      port_no,
	ox810sata_isr_callback_t callback,
	unsigned long            arg,
	int                      may_sleep,
	int                      timeout_jiffies,
	int                      allow_reentrant)
{
	unsigned long end = jiffies + timeout_jiffies;
	int           acquired = 0;
	unsigned long flags;
	DECLARE_WAITQUEUE(wait, current);

OPRINTK("Entered\n");
	spin_lock_irqsave(&access_lock, flags);
	while (1) {
		if (!core_locked ||
			(allow_reentrant && (port_no == reentrant_port_no))) {
			if (allow_reentrant) {
OPRINTK("Allowing SCSI/SATA %s access for port %d\n", core_locked ? "re-entrant" : "first", port_no);
				reentrant_port_no = port_no;
			} else { 
OPRINTK("Allowing bypass access for port %d\n", port_no);
			}

			core_locked = 1;
			ox810sata_isr_callback = callback;
			ox810sata_isr_arg = arg;
			acquired = 1;
			break;
		}

		if (!may_sleep) {
OPRINTK("Failing for port %d as cannot sleep\n", port_no);
			break;
		}

		add_wait_queue(&sata_wait_queue, &wait);

		while (core_locked &&
			  !(allow_reentrant && (port_no == reentrant_port_no))) {
OPRINTK("Wait for port %d\n", port_no);
			spin_unlock_irqrestore(&access_lock, flags);
			if (!schedule_timeout_interruptible(HZ)) {
//				printk(KERN_WARNING "__acquire_sata_core() Timed-out of schedule(), checking overall timeout\n");
			}
			spin_lock_irqsave(&access_lock, flags);
			if (time_after(jiffies, end)) {
				printk(KERN_INFO "__acquire_sata_core() Failing for port %d timed out\n", port_no);
				remove_wait_queue(&sata_wait_queue, &wait);
				goto out;
			}
OPRINTK("Awoken from wait for port %d\n", port_no);
		}

		remove_wait_queue(&sata_wait_queue, &wait);
	}

out:
	spin_unlock_irqrestore(&access_lock, flags);
OPRINTK("Leaving with acquired = %d\n", acquired);
	return acquired;
}

int acquire_sata_core_may_sleep(
	ox810sata_isr_callback_t callback,
	unsigned long            arg,
	int                      timeout_jiffies)
{
OPRINTK("Entered\n");
	return __acquire_sata_core(0, callback, arg, 1, timeout_jiffies, 0);
}
EXPORT_SYMBOL(acquire_sata_core_may_sleep);

/*
 * To be used by non-SCSI/SATA stack SATA core users to release the SATA core
 */
void release_sata_core(void)
{
	unsigned long flags;

	/*
	 * Check that none of the SCSI/SATA stack SATA core acquire/release
	 * mucking about has occured while the SATA core should have been owned
	 * by someone other than the SCSI/SATA stack
	 */
OPRINTK("Entered\n");
	spin_lock_irqsave(&access_lock, flags);
	BUG_ON(reentrant_port_no != -1);
	BUG_ON(!core_locked);
	core_locked = 0;
	wake_up(&sata_wait_queue);
	spin_unlock_irqrestore(&access_lock, flags);
OPRINTK("Leaving\n");
}
EXPORT_SYMBOL(release_sata_core);

int sata_core_has_waiters(void)
{
	return !list_empty(&sata_wait_queue.task_list);
}
EXPORT_SYMBOL(sata_core_has_waiters);

/*
 * ata_port operation to gain ownership of the SATA hardware prior to issuing
 * a command against a SATA host. Allows any number of users of the port against
 * which the lock was first acquired, thus enforcing that only one SATA core
 * port may be operated on at once.
 */
static int acquire_hw(
	int port_no,
	int may_sleep,
	int timeout_jiffies)
{
OPRINTK("Entered, port %d, may_sleep %d\n", port_no, may_sleep);
	return __acquire_sata_core(port_no, NULL, 0, may_sleep, timeout_jiffies, 1);
}

/*
 * ata_port operation to release ownership of the SATA hardware
 */
void release_hw(struct ata_port *ap)
{
	unsigned long flags;

	spin_lock_irqsave(&access_lock, flags);
OPRINTK("Entered, core locked = %d, reentrant_port_no = %d, port_no = %d\n", core_locked, reentrant_port_no, ap->port_no);

	/* This function should only have been called by the SCSI/SATA stack */
	BUG_ON(!core_locked && (reentrant_port_no != -1));

	/* Only unlock for the port currently locked by SCSI/SATA stack */
	if (core_locked && (reentrant_port_no == ap->port_no)) {
		/*
		 * If an ODRB DMA SG list had been allocated to accompany the SATA
		 * transfer then release it now
		 */
		if (odrb_sg) {
OPRINTK("Releasing ORDB SG list for port %d\n", ap->port_no);
			odrb_free_sata_sg_list(odrb_sg);
			odrb_sg = 0;
		}

		reentrant_port_no = -1;
		core_locked = 0;
		wake_up(&sata_wait_queue);
	} else {
OPRINTK("Attempt to unlock from wrong port = %d\n", ap->port_no);
	}
OPRINTK("Leaving\n");
	spin_unlock_irqrestore(&access_lock, flags);
}

static int ox810sata_qc_defer(struct ata_queued_cmd *qc)
{
	int defer;

DPRINTK("Entered\n");
	defer = ata_std_qc_defer(qc);
	if (!defer && !acquire_hw(qc->ap->port_no, 0, 0)) {
OPRINTK("Deferring for port %d\n", qc->ap->port_no);
		defer = ATA_DEFER_LINK;
	}

	return defer;
}

static void ox810sata_qc_prep(struct ata_queued_cmd *qc)
{
	struct scatterlist *sg;
	unsigned int si;
	unsigned int total_len = 0;
	odrb_sg_entry_t *sg_entry = 0;
DPRINTK("Entered, tag %d, dma_dir %d, qc->tf.protocol %d, qc->tf.flags 0x%lx\n", qc->tag, qc->dma_dir, qc->tf.protocol, qc->tf.flags);

	BUG_ON(odrb_sg);
	BUG_ON(ata_is_atapi(qc->tf.protocol));
	BUG_ON(ata_is_ncq(qc->tf.protocol));

	if (qc->n_elem > 1) {
DPRINTK("Assemble SG list, n_elem %d\n", qc->n_elem);
		/* Convert the scatterlist entries into ODRB SG entries */
		for_each_sg(qc->sg, sg, qc->n_elem, si) {
			if (!odrb_sg) {
				/* Get hold of a pointer to an array of SG entries */
OPRINTK("Allocate SG list...\n");
				if (odrb_alloc_sata_sg_list(&odrb_sg, 1)) {
					/*
					 * Failed to get SG list, so set command error mask to
					 * indicate that there's a problem with the command
					 */
					printk(KERN_WARNING "ox810sata_qc_prep() Failed to acquire SG list, failing command prep for port %d\n", qc->ap->port_no);
					qc->err_mask = -1;
					break;
				}
OPRINTK("...SG list %p\n", odrb_sg);
				sg_entry = odrb_sg->sg_entries;
			} else {
				/* Acquire the next SG element and link the just completed SG
				   element's next_ field to its physical address */
				odrb_sg_entry_t *prev_odrb_sg_entry = sg_entry;
				prev_odrb_sg_entry->next_ = (dma_addr_t)descriptors_virt_to_phys(((u32)++sg_entry));
			}

			// Fill the SG entry with the DMAable address and length
			sg_entry->addr_ = sg_dma_address(sg);
			sg_entry->length_ = sg_dma_len(sg);

			total_len += sg_entry->length_;
		}
	}

	if (sg_entry) {
DPRINTK("SG DMA total_len %d\n", total_len);
		/* Terminate the SG list */
		sg_entry->next_ = 0;

		/* Setup the ODRB DMA transfer */
		BUG_ON(qc->dma_dir == DMA_NONE);
		odrb_dma_sata_sq(
			qc->dma_dir == DMA_FROM_DEVICE ? OXNAS_DMA_FROM_DEVICE :
											 OXNAS_DMA_TO_DEVICE,
			total_len >> SECTOR_SHIFT, odrb_sg->phys, 1);
	} else if (qc->n_elem == 1) {
DPRINTK("Single DMA\n");
DPRINTK("\tlen %d\n", sg_dma_len(qc->sg));
DPRINTK("\tadr 0x%p\n", (void*)sg_dma_address(qc->sg));
		/* Don't use SGDMA controller for transfers with a single element */
		odrb_dma_sata_single(qc->dma_dir == DMA_FROM_DEVICE ?
			OXNAS_DMA_FROM_DEVICE : OXNAS_DMA_TO_DEVICE,
			sg_dma_address(qc->sg), sg_dma_len(qc->sg));
	} else {
DPRINTK("No SG entries!\n");
	}
DPRINTK("Leaving\n");
}

static unsigned int ox810sata_qc_issue(struct ata_queued_cmd *qc)
{
	u32 *port_base = ox810sata_get_tfio_base(qc->ap);
	u32 reg;
DPRINTK("Entered, tag %d, dma_dir %d, qc->tf.protocol %d, qc->tf.flags 0x%lx\n", qc->tag, qc->dma_dir, qc->tf.protocol, qc->tf.flags);

    /* Only allow one command to be in progress for the entire SATA core */
    if (qc->err_mask) {
DPRINTK("qc has non-zero err_mask, rejecting for port %d\n", qc->ap->port_no);
        return -1;
	}

#ifdef CONFIG_SATA_OXNAS_DISK_LIGHT
    /* disk light on */
    writel(1 << CONFIG_OX810SATA_DISK_LIGHT_GPIO_LINE, GPIO_A_OUTPUT_SET);
#endif  // CONFIG_SATA_OXNAS_DISK_LIGHT
#ifdef CONFIG_WDC_LEDS_TRIGGER_SATA_DISK
    wdc_ledtrig_sata_activity();
#endif // CONFIG_WDC_LEDS_TRIGGER_SATA_DISK

	/* Clear phy/link errors */
	ox810sata_scr_write_port(ox810sata_driver.host->ports[0], SCR_ERROR, ~0);
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
	ox810sata_scr_write_port(ox810sata_driver.host->ports[1], SCR_ERROR, ~0);
#endif // CONFIG_SATA_OXNAS_SINGLE_SATA

	/* Disable all interrupts for ports and RAID controller */
	writel(~0, (u32*)SATA0_REGS_BASE + OX810SATA_INT_DISABLE);
	writel(~0, (u32*)SATA1_REGS_BASE + OX810SATA_INT_DISABLE);
	writel(~0, (u32*)SATARAID_REGS_BASE + OX810SATA_INT_DISABLE);

	/* Disable all interrupts for core */
	writel(~0, OX810SATA_CORE_IEC);

	/* Clear any interrupts for ports and RAID controller */
	writel(~0, (u32*)SATA0_REGS_BASE + OX810SATA_INT_CLEAR);
	writel(~0, (u32*)SATA1_REGS_BASE + OX810SATA_INT_CLEAR);
	writel(~0, (u32*)SATARAID_REGS_BASE + OX810SATA_INT_CLEAR);

	/* Clear errors on both hosts */
	reg = readl((u32* )SATA0_REGS_BASE + OX810SATA_SATA_CONTROL);
	reg |= OX810SATA_SCTL_CLR_ERR ;
	writel(reg, (u32* )SATA0_REGS_BASE + OX810SATA_SATA_CONTROL);
	reg = readl((u32* )SATA1_REGS_BASE + OX810SATA_SATA_CONTROL);
	reg |= OX810SATA_SCTL_CLR_ERR ;
	writel(reg, (u32* )SATA1_REGS_BASE + OX810SATA_SATA_CONTROL);
	reg = readl((u32* )SATARAID_REGS_BASE + OX810SATA_SATA_CONTROL);
	reg |= OX810SATA_SCTL_CLR_ERR ;
	writel(reg, (u32* )SATARAID_REGS_BASE + OX810SATA_SATA_CONTROL);

	/* Set the RAID controller hardware to idle */
	writel(OX810SATA_NOTRAID, OX810SATA_RAID_CONTROL);

	/* Load the ORB registers with the ATA taskfile contents associated with
	   the command */
	ox810sata_tf_load(qc->ap, &qc->tf);

	/* Enable interrupts from core */
	writel(OX810SATA_NORMAL_INTS_WANTED, OX810SATA_CORE_IES);  

	/* Enable interrupts from port */
	writel(OX810SATA_INT_WANT, port_base + OX810SATA_INT_ENABLE);

#ifdef ERROR_INJECTION
{
	ox810sata_host_private_data_t *host_pd =
		(ox810sata_host_private_data_t *)qc->ap->host->private_data;
    static u32 prand = 10;
    if (host_pd->error_inject) {
        u32 *portaddr = ox810sata_get_io_base(qc->ap);
        prand = prand ? prand - 1 : 100;
        if (prand < host_pd->error_inject) {
            DPRINTK("ox810sata_exec_command: error injection on\n");
            __ox810sata_scr_write( portaddr, 0x14 , 0xd );
        } else {
            __ox810sata_scr_write( portaddr, 0x14 , 0x1 );
            DPRINTK("ox810sata_exec_command: error injection off\n");
        }
    }
}
#endif

	reg = readl(port_base + OX810SATA_SATA_COMMAND);
DPRINTK("Before command reg 0x%08x\n", reg);
	/* Cause the SATA core to generate the FIS corresponding to the taskfile
	   contents */
	reg &= ~SATA_OPCODE_MASK;
	reg |= CMD_WRITE_TO_ORB_REGS;
DPRINTK("Poking SATA to write to ORB regs, writing 0x%08x to command reg\n", reg);
	writel(reg, port_base + OX810SATA_SATA_COMMAND);
DPRINTK("After command reg 0x%08x\n", readl(port_base + OX810SATA_SATA_COMMAND));

	return 0;
}

static bool ox810sata_qc_fill_rtf(struct ata_queued_cmd *qc)
{
DPRINTK("tag %d\n", qc->tag);

	/* Read the most recently received FIS from the SATA core ORB registers
	   and convert to an ATA taskfile */
	ox810sata_tf_read(qc->ap, &qc->result_tf);
	return true;
}

static int ox810sata_qc_new(struct ata_port *ap)
{
DPRINTK("Entered for port %d\n", ap->port_no);
	return 0;
}

static void ox810sata_qc_free(struct ata_queued_cmd *qc)
{
DPRINTK("Entered\n");
	release_hw(qc->ap);
}

/** 
 * port_start() is called just after the data structures for each port are
 * initialized. Typically this is used to alloc per-port DMA buffers, tables
 * rings, enable DMA engines and similar tasks.
 *
 * @return 0 = success
 * @param ap hardware with the registers in
 */
static int ox810sata_port_start(struct ata_port *ap)
{
    ox810sata_port_private_data_t *pd;

	/* Extract the port base address stored when host was allocated */
	u32 *port_base = ap->private_data;
DPRINTK("port_no %d\n", ap->port_no);

	/* Allocate and zeroise port private data */
	ap->private_data = kzalloc(sizeof(ox810sata_port_private_data_t), GFP_KERNEL);
	if (!ap->private_data) {
		printk(KERN_WARNING "ox810sata_port_start() Failed to allocate space for private data\n");
		return -ENOMEM;
	}
	pd = (ox810sata_port_private_data_t*)ap->private_data;

	/* Save the port base address */
	pd->port_base = port_base;

	/* Allocate space for DMA descriptors */
	ap->prd = kmalloc(ATA_PRD_TBL_SZ, GFP_KERNEL);
    if (!ap->prd) {
		printk(KERN_WARNING "ox810sata_port_start() Failed to allocate space for DMA descriptors\n");
        return -ENOMEM;
    }

    DPRINTK("ap = %p, pd = %p\n",ap,ap->private_data);

    /* post reset init needs to be called for both ports as there's one reset
    for both ports*/
	ox810sata_post_reset_init(ap);

    return 0;
}

/** 
 * port_stop() is called after ->host_stop(). It's sole function is to 
 * release DMA/memory resources, now that they are no longer actively being
 * used.
 */
static void ox810sata_port_stop(struct ata_port *ap)
{
    ox810sata_port_private_data_t* pd = (ox810sata_port_private_data_t* )ap->private_data;
DPRINTK("port_no %d\n", ap->port_no);
    kfree(pd);
}

/** 
 * host_stop() is called when the rmmod or hot unplug process begins. The
 * hook must stop all hardware interrupts, DMA engines, etc.
 *
 * @param ap hardware with the registers in
 */
static void ox810sata_host_stop(struct ata_host *host_set)
{
DPRINTK("Enter\n");
}

static void ox810sata_post_internal_cmd(struct ata_queued_cmd *qc)
{
    struct ata_port *ap = qc->ap;
DPRINTK("qc %p, tag %d, ap %p, port_no %d\n", qc, qc->tag, qc->ap, qc->ap->port_no);

    /* Perform core cleanups and resets */
    ox810sata_cleanup(ap);
}

#define ERROR_HW_ACQUIRE_TIMEOUT_JIFFIES (HZ)
static void ox810sata_error_handler(struct ata_port *ap)
{
DPRINTK("Enter port_no %d\n", ap->port_no);

    /*
	 * Only allow commands to be in progress on one port at a time, but error
	 * error handling must be allowed through when a SATA command has failed
	 * to complete within SCSI/SATA stack timeout period in order to clean up
	 * the port
	 */
	if (!acquire_hw(ap->port_no, 1, ERROR_HW_ACQUIRE_TIMEOUT_JIFFIES)) return;

	/* Will probably need to do some pre- and post- processing around the
	   standard ata error handling routine */
	ata_std_error_handler(ap);

	/*
	 * The error handling will have gained the SATA core lock either normally,
	 * or by breaking the lock obtained via qc_issue() presumably because the
	 * command has failed and timed-out. In either case we don't expect someone
	 * else to have release the SATA core lock from under us
	 */
DPRINTK("Releasing SATA core lock, port_no %d\n", ap->port_no);
	release_hw(ap);
}

static int ox810sata_prereset(
	struct ata_link *link,
	unsigned long    deadline)
{
DPRINTK("port_no %d\n", link->ap->port_no);

    /* Perform core cleanups and resets */
    ox810sata_cleanup(link->ap);

	return ata_std_prereset(link, deadline);
}

static int ox810sata_hardreset(
	struct ata_link *link,
	unsigned int    *class,
	unsigned long    deadline)
{
DPRINTK("port_no %d\n", link->ap->port_no);
	return sata_std_hardreset(link, class, deadline);
}

static int ox810sata_softreset(
	struct ata_link *link,
	unsigned int    *classes,
	unsigned long    deadline)
{
	struct ata_taskfile tf;

DPRINTK("ENTER, port_no %d\n", link->ap->port_no);

	/* Issue a reset */

	/* Get the result of the reset */
	ox810sata_tf_read(link->ap, &tf);
	classes[0] = ata_dev_classify(&tf);

DPRINTK("EXIT, class=%u\n", classes[0]);
	return 0;
}

static void ox810sata_postreset(
	struct ata_link *link,
	unsigned int    *classes)
{
DPRINTK("port_no %d\n", link->ap->port_no);
	ata_std_postreset(link, classes);

    /* Turn on phy error detection */ 
    __ox810sata_scr_write((u32* )SATA0_REGS_BASE , 0x0c, 0x30003 );
    __ox810sata_scr_write((u32* )SATA1_REGS_BASE , 0x0c, 0x30003 );
}

static void ox810sata_freeze(struct ata_port *ap)
{
DPRINTK("Entered\n");
}

static void ox810sata_thaw(struct ata_port *ap)
{
DPRINTK("Entered\n");
}

/**
 * Returns accumulated RAID faults and then clears the accumulation
 * @return accumulated RAID faults indicated by set bits
 */
int oxnassata_RAID_faults(void)
{
    int temp = ox810sata_accumulated_RAID_faults;
    ox810sata_accumulated_RAID_faults = 0;
    return temp;
}

/**
 * Returns ox810 port number the request queue is serviced by.
 *
 * @param queue The queue under investigation.
 * @return The ox810 sata port number servicing the queue or -1 if not found.
 */
int oxnassata_get_port_no(struct request_queue* q)
{
    struct ata_port* ap = 0;
    struct scsi_device* sdev = 0;
    
    /* check port 0 */
    ap = ox810sata_driver.host->ports[0];
    if (ap) {
        shost_for_each_device(sdev, ap->scsi_host) {
            if (sdev->request_queue == q) {
                DPRINTK("Queue %p on port 0\n", q);
                return 0;
            }
        }
	}

#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
    /* check port 1 */
    ap = ox810sata_driver.host->ports[1];
    if (ap) {
        shost_for_each_device(sdev, ap->scsi_host) {
            if (sdev->request_queue == q) {
                DPRINTK("Queue %p on port 1\n", q);
                return 1;
            }
        }
	}
#endif // CONFIG_SATA_OXNAS_SINGLE_SATA

    /* not found */
    return -1;  
}

/**
 * @return true if all the drives attached to the internal SATA ports use the
 * same LBA size.
 */
int oxnassata_LBA_schemes_compatible(void)
{
    unsigned long flags0 ;
    unsigned long flags1 ;
    struct ata_port* ap ;
    
    /* check port 0 */
    ap = ox810sata_driver.host->ports[0];
    if (ap)
        flags0 = ap->link.device[0].flags & ATA_DFLAG_LBA48 ;
    else
        return 0;
    
    /* check port 1 */
    ap = ox810sata_driver.host->ports[1];
    if (ap)
        flags1 = ap->link.device[0].flags & ATA_DFLAG_LBA48 ;
    else
        return 0;

    /* compare */
    return (flags0 == flags1);  
}

EXPORT_SYMBOL( oxnassata_RAID_faults );
EXPORT_SYMBOL( oxnassata_get_port_no );
EXPORT_SYMBOL( oxnassata_LBA_schemes_compatible );

#ifdef ERROR_INJECTION
/**
 * @param kobj Not Used
 * @param attr Used to determine which file is being accessed
 * @param buffer Space to put the file contents
 * @return The number of bytes transferred or an error
 */
static int ox810sata_error_inject_show(
	char   *page,
	char  **start,
	off_t   off,
	int     count,
	int    *eof,
	void   *data)
{
    if (page) {
		ox810sata_host_private_data_t *host_pd =
			(ox810sata_host_private_data_t*)ox810sata_driver.host->private_data;

        if (host_pd->error_inject) {
            page[0] = host_pd->error_inject + '0';
            page[1] = '\n';
            page[2] = 0;
            return 3;
        } else {
            strcpy(page, "off\n" );
            return 5;
        }
    }

    /* if we get here, there's been an error */
    return -EIO;
}

static int ox810sata_error_inject_store(
	struct file       *file,
	const char __user *buffer,
	unsigned long      count,
	void              *data)
{
    if (count) {
		ox810sata_host_private_data_t *host_pd =
			(ox810sata_host_private_data_t*)ox810sata_driver.host->private_data;

        if ((buffer[0] >= '0') &&
            (buffer[0] <= '9')) {
            host_pd->error_inject = buffer[0] - '0';
        }
        return count;
    }

    /* if we get here, there's been an error */
    return -EIO;
}
#endif // ERROR_INJECTION

static int __init ox810sata_init(void)
{
#ifdef ERROR_INJECTION
    struct proc_dir_entry *res =
		create_proc_entry("ox810sata_errorinject",0,NULL);

	if (res) {
		res->read_proc=ox810sata_error_inject_show;
        res->write_proc=ox810sata_error_inject_store;
		res->data=NULL;
	}
#endif

    return platform_driver_register(&ox810sata_driver.driver);
}

static void __exit ox810sata_exit(void)
{
    platform_driver_unregister(&ox810sata_driver.driver);
}

module_init(ox810sata_init);
module_exit(ox810sata_exit);

/**
 * Describes the identity of the SATA core and the resources it requires
 */ 
static struct resource ox810sata_resources[] = {
	{
        .name       = "sata_port_0_registers",
		.start		= SATA0_REGS_BASE,
		.end		= SATA0_REGS_BASE + 0xff,
		.flags		= IORESOURCE_MEM,
	},
	{
        .name       = "sata_port_1_registers",
		.start		= SATA1_REGS_BASE,
		.end		= SATA1_REGS_BASE + 0xff,
		.flags		= IORESOURCE_MEM,
	},
    {
        .name       = "sata_irq",
        .start      = SATA_INTERRUPT,
		.flags		= IORESOURCE_IRQ,
    },
};

static struct platform_device ox810sata_dev = 
{
    .name = DRIVER_NAME,
    .id = 0,
    .num_resources = 3,
	.resource  = ox810sata_resources,
    .dev.coherent_dma_mask = 0xffffffff,
}; 

static int __init ox810sata_device_init(void)
{
DPRINTK("Enter\n");
	// register the ata device for the driver to find
    return platform_device_register(&ox810sata_dev);
}

static void __exit ox810sata_device_exit(void)
{
DPRINTK("Enter\n");
    platform_device_unregister(&ox810sata_dev);
}

module_init(ox810sata_device_init);
module_exit(ox810sata_device_exit);

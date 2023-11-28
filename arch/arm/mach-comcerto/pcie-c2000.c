/*
 * linux/arch/arm/mach-comcerto/pcie-c2000.c
 *
 * Copyright (C) 2004,2005 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/sched.h>
#if defined(CONFIG_PCI_MSI)
#include <linux/msi.h>
#endif

#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/sizes.h>
#include <asm/mach/pci.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <asm/mach/irq.h>
#include <mach/pcie-c2000.h>
#include <mach/serdes-c2000.h>
#include <mach/reset.h>
#include <mach/hardware.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
#include <linux/irqchip/chained_irq.h>
#endif
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <mach/comcerto-2000/pm.h>
//#define COMCERTO_PCIE_DEBUG
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 10, 0)
#include <asm/system.h>
#else
#include <asm/system_info.h>
#endif

#define pcie_readl(r) readl((void*)(r))
#define pcie_writel(v,r) writel(v, (void*)(r))

#define pcie_readl_relaxed(r) readl_relaxed((void*)(r))
#define pcie_writel_relaxed(v,r) writel_relaxed(v, (void*)(r))
#define pcie_writew_relaxed(v,r) writew_relaxed(v, (void*)(r))
#define pcie_writeb_relaxed(v,r) writeb_relaxed(v, (void*)(r))

#ifdef CONFIG_PCI_MSI
static DECLARE_BITMAP(msi_irq_in_use[NUM_PCIE_PORTS], PCIE_NUM_MSI_IRQS);
static int comcerto_msi_init(struct pcie_port *pp);
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
extern void irq_init_desc(unsigned int irq);
#endif

int pcie_external_clk = 0;
static int __init get_pcie_clk_mode(char *str)
{
        if (!strcmp(str, "yes"))
                pcie_external_clk = 1;

        return 1;
}

__setup("pcie_external_clk=", get_pcie_clk_mode);

int pcie_gen1_only = 0;
static int __init get_pcie_gen_mode(char *str)
{
        if (!strcmp(str, "yes"))
                pcie_gen1_only = 1;

        return 1;
}

__setup("pcie_gen1_only=", get_pcie_gen_mode);


/* DWC PCIEe configuration register offsets on APB */
struct pcie_app_reg app_regs[MAX_PCIE_PORTS] = {
	/* PCIe0 */
	{
		0x00000000,
		0x00000004,
		0x00000008,
		0x0000000C,
		0x00000010,
		0x00000014,
		0x00000018,
		0x00000040,
		0x00000044,
		0x00000048,
		0x00000058,
		0x00000080,
		0x00000084,
		0x000000C0,
		0x000000C4,
		0x00000100,
		0x00000104,
		0x00000108,
		0x0000010C
	},
	/* PCIe1 */
	{
		0x00000020,
		0x00000024,
		0x00000028,
		0x0000002C,
		0x00000030,
		0x00000034,
		0x00000038,
		0x0000004C,
		0x00000050,
		0x00000054,
		0x0000005C,
		0x00000088,
		0x0000008C,
		0x000000C8,
		0x000000CC,
		0x00000110,
		0x00000114,
		0x00000118,
		0x0000011C
	}
};

/* Keeping all DDR area of 512MB accesible for inbound transaction */
#define INBOUND_ADDR_MASK	0x1FFFFFFF


#define PCIE_SETUP_iATU_IB_ENTRY( _pp, _view_port, _base, _limit, _ctl1, _ctl2, _target ) \
{\
	comcerto_dbi_write_reg(_pp, PCIE_iATU_VIEW_PORT, 4, (u32)(_view_port|iATU_VIEW_PORT_IN_BOUND)); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_CTRL2, 4, 0); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_SRC_LOW, 4, (u32)_base); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_SRC_HIGH, 4, 0); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_LIMIT, 4, (u32)((_base)+(_limit))); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_TRGT_LOW, 4, (u32)_target); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_TRGT_HIGH, 4, (u32)0); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_CTRL1, 4, (u32)_ctl1); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_CTRL2, 4, (u32)(_ctl2 |iATU_CTRL2_ID_EN) ); \
}

#define PCIE_SETUP_iATU_OB_ENTRY( _pp, _view_port, _base, _limit, _ctl1, _ctl2, _target ) \
{\
	comcerto_dbi_write_reg(_pp, PCIE_iATU_VIEW_PORT, 4, (u32)_view_port); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_CTRL2, 4, 0); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_SRC_LOW, 4, (u32)_base); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_SRC_HIGH, 4, (u32)0); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_LIMIT, 4, ((u32)((_base)+(_limit)))); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_TRGT_LOW, 4, (u32)_target); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_TRGT_HIGH, 4, (u32)0); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_CTRL1, 4, (u32)_ctl1); \
	comcerto_dbi_write_reg(_pp, PCIE_iATU_CTRL2, 4, (u32)(_ctl2 |iATU_CTRL2_ID_EN) ); \
}

#define MAX_LINK_UP_WAIT_JIFFIES	HZ /* 1 Second */

static unsigned long pcie_cnf_base_addr[MAX_PCIE_PORTS] =
		{ COMCERTO_AXI_PCIe0_BASE, COMCERTO_AXI_PCIe1_BASE };
static unsigned long pcie_remote_base_addr[MAX_PCIE_PORTS] =
		{ COMCERTO_AXI_PCIe0_SLAVE_BASE, COMCERTO_AXI_PCIe1_SLAVE_BASE };
static int pcie_msi_base[MAX_PCIE_PORTS] =
		{ PCIE0_MSI_INT_BASE, PCIE1_MSI_INT_BASE };
static int pcie_intx_base[MAX_PCIE_PORTS] =
		{ PCIE0_INTX_BASE, PCIE1_INTX_BASE };
static int pcie_irqs[MAX_PCIE_PORTS] =
		{ IRQ_PCIe0, IRQ_PCIe1 };


static struct pcie_port pcie_port[MAX_PCIE_PORTS];

static int pcie_port_is_host( int nr  )
{
	struct pcie_port *pp= &pcie_port[nr];

	return ( pp->port_mode == PCIE_PORT_MODE_RC ) ? 1 : 0;
}

/**
 * This function sets PCIe port mode.
 */
static void pcie_port_set_mode( int nr, int mode  )
{
	struct pcie_port *pp= &pcie_port[nr];

	pcie_writel( (pcie_readl(pp->va_app_base + pp->app_regs->cfg0) & ~0xf) | mode, pp->va_app_base + pp->app_regs->cfg0);
}

/**
 * This function returns the given port mode.
 * @param nr	PCIe Port number.
 */
static int pcie_port_get_mode( int nr  )
{
	struct pcie_port *pp= &pcie_port[nr];

	return ( pcie_readl(pp->va_app_base + pp->app_regs->cfg0) &
			DWC_CFG0_DEV_TYPE_MASK );
}

/**
 * This function checks whether link is up or not.
 * Returns true if link is up otherwise returns false.
 * @param pp	Pointer to PCIe Port control block.
 */
static int comcerto_pcie_link_up( struct pcie_port *pp  )
{
	unsigned long deadline = jiffies + MAX_LINK_UP_WAIT_JIFFIES;

	do {
		if (pcie_readl( pp->va_app_base + pp->app_regs->sts0 ) & STS0_RDLH_LINK_UP) {
			return 1;
		}

		cond_resched();
	} while (!time_after_eq(jiffies, deadline));

	return 0;
}

/**
 * bus_to_port().
 *
 */
static struct pcie_port *bus_to_port(int bus)
{
	int i;

	for (i = NUM_PCIE_PORTS - 1; i >= 0; i--) {
		int rbus = pcie_port[i].root_bus_nr;
		if ( !pcie_port_is_host(i) )
			continue;
		if (rbus != -1 && rbus <= bus)
			break;
	}

	return i >= 0 ? pcie_port + i : NULL;
}

/**
 * This function is used to read DBI registers.
 */

static void comcerto_dbi_read_reg(struct pcie_port *pp, int where, int size,
		u32 *val)
{
	u32 va_address;

	va_address = (u32)pp->va_dbi_base + (where & ~0x3);

	*val = pcie_readl_relaxed(va_address);

	if (size == 1)
		*val = (*val >> (8 * (where & 3))) & 0xff;
	else if (size == 2)
		*val = (*val >> (8 * (where & 3))) & 0xffff;
}

/**
 * This function is used to write into DBI registers.
 */
static void comcerto_dbi_write_reg(struct pcie_port *pp, int where, int size,
		u32 val)
{
	u32 va_address;
	int pos, val1, mask = 0;

	va_address = (u32)pp->va_dbi_base + (where & ~0x3);

	pos = (where & 0x3) << 3;

	if (size == 4)
		val1 = val;
	else
	{
		if (size == 2)
			mask = 0xffff;
		else if (size == 1)
			mask = 0xff;

		val1 = pcie_readl_relaxed(va_address);
		val1 = ( val1 & ~( mask  << pos ) ) | ( (val & mask) << pos );
	}

	pcie_writel_relaxed(val1, va_address);
}

static inline void nop_delay(void)
{
        int k;
        for(k = 0 ; k < 1000; k++)
                nop();
}

static int comcerto_pcie_rd_conf(struct pcie_port *pp, int bus_nr,
		u32 devfn, int where, int size, u32 *val)
{
	u32 address;
	u32 target_address = (u32)(bus_nr << 24) | (PCI_SLOT(devfn) << 19) | (PCI_FUNC(devfn) << 16);

	/* Initialize iATU */
	if (bus_nr != pp->root_bus_nr) {

		if (pp->cfg1_prev_taddr != target_address) {
			/* Type1 configuration request */
			PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_CNF1, (u32)iATU_GET_CFG1_BASE(pp->remote_mem_baseaddr),
				iATU_CFG1_SIZE - 1, (AXI_OP_TYPE_CONFIG_RDRW_TYPE1 & iATU_CTRL1_TYPE_MASK),
				0, target_address );
			pp->cfg1_prev_taddr = target_address;
		}

		address = (u32)pp->va_cfg1_base |(where & 0xFFFC);
	} else {
		if (pp->cfg0_prev_taddr != target_address) {
			/* Type0 configuration request */
			PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_CNF0, (u32)iATU_GET_CFG0_BASE(pp->remote_mem_baseaddr),
				iATU_CFG0_SIZE - 1, (AXI_OP_TYPE_CONFIG_RDRW_TYPE0 & iATU_CTRL1_TYPE_MASK),
				0, target_address );
			pp->cfg0_prev_taddr = target_address;
		}

		address = (u32)pp->va_cfg0_base |(where & 0xFFFC);
	}


	*val = pcie_readl_relaxed(address);

	/* Because of the imprecise external abort the processor is not able to get the exact instruction 
           which caused the abort and hence when the abort handler tries to restore the PC to the next 
           instruction to resume it is often wrong and it results in skipping few instruction after the 
           pcie_readl_relaxed which has caused abort. So nop instructions are added after pcie_readl so that even 
           if the some instructions are missed out it will miss the nop instruction only.
	*/
	nop_delay();

	if (size == 1)
		*val = (*val >> (8 * (where & 3))) & 0xff;
	else if (size == 2)
		*val = (*val >> (8 * (where & 3))) & 0xffff;

#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s: bus:%d dev:%d where:%d, size:%d addr : %x value:%x\n", __func__, bus_nr, devfn, where, size, address, *val);
#endif
	return PCIBIOS_SUCCESSFUL;
}

static int pcie_read_conf(struct pci_bus *bus, u32 devfn, int where, int size, u32 *val)
{
	struct pcie_port *pp = bus_to_port(bus->number);
	unsigned long flags;
	int ret;


#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s: bus:%d dev:%d where:%d, size:%d\n", __func__, bus->number, devfn, where, size);
#endif


	/* Make sure that link is up.
	 * Filter device numbers, unless it's a type1 access
	 */
	if ( (!pp->link_state)||
			((bus->number == pp->root_bus_nr) && (PCI_SLOT(devfn) > 0)) ) {
		*val = 0xffffffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	BUG_ON (((where & 0x3) + size) > 4);

	/* Enter critical section. */
	spin_lock_irqsave(&pp->conf_lock, flags);
	ret = comcerto_pcie_rd_conf(pp, bus->number, devfn, where, size, val);
	/* Exit critical section. */
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	return ret;
}

static int comcerto_pcie_wr_conf(struct pcie_port *pp, int bus_nr,
		u32 devfn, int where, int size, u32 val)
{
	int ret = PCIBIOS_SUCCESSFUL;
	u32 address;
	u32 target_address = (u32)(bus_nr << 24) | (PCI_SLOT(devfn) << 19) | (PCI_FUNC(devfn) << 16);

	/* Initialize iATU */
	if (bus_nr != pp->root_bus_nr) {
		if (pp->cfg1_prev_taddr != target_address) {
			/* Type1 configuration request */
			PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_CNF1, (u32)iATU_GET_CFG1_BASE(pp->remote_mem_baseaddr),
				iATU_CFG1_SIZE - 1, (AXI_OP_TYPE_CONFIG_RDRW_TYPE1 & iATU_CTRL1_TYPE_MASK),
				0, target_address );
			pp->cfg1_prev_taddr = target_address;
		}

		address = (u32)pp->va_cfg1_base |(where & 0xFFFC);
	} else {
		if (pp->cfg0_prev_taddr != target_address) {
			/* Type0 configuration request */
			PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_CNF0, (u32)iATU_GET_CFG0_BASE(pp->remote_mem_baseaddr),
				iATU_CFG0_SIZE - 1, (AXI_OP_TYPE_CONFIG_RDRW_TYPE0 & iATU_CTRL1_TYPE_MASK),
				0, target_address );
			pp->cfg0_prev_taddr = target_address;
		}

		address = (u32)pp->va_cfg0_base |(where & 0xFFFC);
	}


#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s: bus:%d dev:%d where:%d, size:%d addr : %x value:%x\n", __func__, bus_nr, devfn, where, size, address, val);
#endif
	if (size == 4)
		pcie_writel_relaxed(val, address);
	else if (size == 2)
		pcie_writew_relaxed(val, address + (where & 2));
	else if (size == 1)
		pcie_writeb_relaxed(val, address + (where & 3));
	else
		ret = PCIBIOS_BAD_REGISTER_NUMBER;

	return ret;
}

static int pcie_write_conf(struct pci_bus *bus, u32 devfn, int where, int size, u32 val)
{
	struct pcie_port *pp = bus_to_port(bus->number);
	unsigned long flags;
	int ret;

#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s: bus:%d dev:%d where:%d, size:%d val:%d\n", __func__, bus->number, devfn, where, size, val);
#endif

	/* Make sure that link is up.
	 * Filter device numbers, unless it's a type1 access
	 */
	if ( (!pp->link_state)||
			((bus->number == pp->root_bus_nr) && (PCI_SLOT(devfn) > 0)) ) {
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	BUG_ON (((where & 0x3) + size) > 4);

	/* Enter critical section. */
	spin_lock_irqsave(&pp->conf_lock, flags);
	ret = comcerto_pcie_wr_conf(pp, bus->number, devfn, where, size, val);
	if (where == PCI_COMMAND)
		pp->cmd_reg_val = val & 0xffff;
	/* Exit critical section. */
	spin_unlock_irqrestore(&pp->conf_lock, flags);

	return ret;
}



static u8 __init comcerto_pcie_swizzle(struct pci_dev *dev, u8 *pin)
{
	return PCI_SLOT(dev->devfn);
}

int comcerto_pcie_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	struct pcie_port *pp = bus_to_port(dev->bus->number);
	int irq = (PCIE0_INTX_BASE + pp->port * PCIE_NUM_INTX_IRQS + pin - 1);

	return irq;
}

EXPORT_SYMBOL(comcerto_pcie_map_irq);

static int __init comcerto_pcie_setup(int nr, struct pci_sys_data *sys)
{
	struct pcie_port *pp;
	struct pcie_app_reg *app_reg;
	u32 val;

	if ((nr >= NUM_PCIE_PORTS) || !pcie_port_is_host(nr))
		return 0;

#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s:%d nr:%d\n", __func__, __LINE__, nr);
#endif


	pp = &pcie_port[nr];

	if (!pp->link_state)
		return 0;

	pp->root_bus_nr = sys->busnr;
	app_reg = pp->app_regs;

	/* Allocate device memory mapped and IO mapped regions */
	snprintf(pp->mem_space_name, sizeof(pp->mem_space_name),
			"PCIe %d MEM", pp->port);
	pp->mem_space_name[sizeof(pp->mem_space_name) - 1] = 0;
	pp->res[0].name = pp->mem_space_name;
	pp->res[0].start = iATU_GET_MEM_BASE(pp->remote_mem_baseaddr);
	pp->res[0].end = pp->res[0].start + iATU_MEM_SIZE - 1;
	pp->res[0].flags = IORESOURCE_MEM;

	if (request_resource(&iomem_resource, &pp->res[0]))
	{
		printk(KERN_ERR "%s:can't allocate PCIe Memory space", __func__);
		return 0;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	pci_add_resource_offset(&sys->resources, &pp->res[0], sys->mem_offset);
#endif

	snprintf(pp->io_space_name, sizeof(pp->io_space_name),
			"PCIe %d I/O", pp->port);
	pp->io_space_name[sizeof(pp->io_space_name) - 1] = 0;
	pp->res[1].name = pp->io_space_name;
	pp->res[1].start = iATU_GET_IO_BASE(pp->remote_mem_baseaddr);
	pp->res[1].end = pp->res[1].start + iATU_IO_SIZE - 1;
	pp->res[1].flags = IORESOURCE_IO;

	if (request_resource(&iomem_resource, &pp->res[1]))
	{
		printk(KERN_ERR "%s:can't allocate PCIe IO space", __func__);
		return 0;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 0)
	pci_add_resource_offset(&sys->resources, &pp->res[1], sys->io_offset);
#endif

	/* Generic PCIe unit setup.*/

	/* Enable own BME. It is necessary to enable own BME to do a
	 * memory transaction on a downstream device
	 */
	comcerto_dbi_read_reg(pp, PCI_COMMAND, 2, &val);
	val |= (PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER
			| PCI_COMMAND_PARITY | PCI_COMMAND_SERR);
	comcerto_dbi_write_reg(pp, PCI_COMMAND, 2, val);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 4, 0)
	/* Need to come back here*/

	sys->resource[0] = &pp->res[0];
	sys->resource[1] = &pp->res[1];
	sys->resource[2] = NULL;
#endif

	pp->cfg0_prev_taddr = 0xffffffff;
	pp->cfg1_prev_taddr = 0xffffffff;

	return 1;
}

static struct pci_ops pcie_ops = {
	.read = pcie_read_conf,
	.write = pcie_write_conf,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 8, 0)
static struct pci_bus *__init comcerto_pcie_scan_bus(int nr, struct pci_sys_data *sys)
{
	struct pci_bus *bus;

	if ((nr < NUM_PCIE_PORTS) && (pcie_port_is_host(nr))) {

		bus = pci_scan_bus(sys->busnr, &pcie_ops, sys);
	} else {
		bus = NULL;
		BUG();
	}

	return bus;
}
#endif

static struct hw_pci comcerto_pcie __initdata = {
	.nr_controllers	= NUM_PCIE_PORTS,
	.swizzle = comcerto_pcie_swizzle,
	.map_irq = comcerto_pcie_map_irq,
	.setup = comcerto_pcie_setup,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 8, 0)
	.ops		= &pcie_ops,
#else
	.scan = comcerto_pcie_scan_bus,
#endif
};



#ifdef CONFIG_PCI_MSI
/* MSI int handler
 */
static void handle_msi(struct pcie_port *pp)
{
	unsigned long val, mask;
	unsigned int pos, mask0;


	val = pcie_readl_relaxed(pp->va_dbi_base + PCIE_MSI_INTR0_STATUS);

continue_handle:

	pos = 0;

	while (val) {
		mask0 = 1 << pos;

		if (val & mask0) {
			/* FIXME : WA for bz69520
			 * To avoid race condition during avk the interrupt disabling interrupt before
			 * Ack and enabling after Ack.
			 */
			spin_lock(&pp->intr_lock);
			mask = pcie_readl_relaxed(pp->va_dbi_base + PCIE_MSI_INTR0_ENABLE);
			pcie_writel_relaxed(mask & ~mask0, pp->va_dbi_base + PCIE_MSI_INTR0_ENABLE);
			pcie_writel_relaxed(mask0, pp->va_dbi_base + PCIE_MSI_INTR0_STATUS);
			pcie_writel_relaxed(mask, pp->va_dbi_base + PCIE_MSI_INTR0_ENABLE);
			spin_unlock(&pp->intr_lock);
			generic_handle_irq(pp->msi_base	+ pos);
			val = val & ~mask0;
		}
		pos++;
	}

	val = pcie_readl_relaxed(pp->va_dbi_base + PCIE_MSI_INTR0_STATUS);
	if(val)
		goto continue_handle;

#if 0
	for (i = 0; i < (PCIE_NUM_MSI_IRQS >> 5); i++) {
		val = pcie_readl_relaxed(pp->va_dbi_base + PCIE_MSI_INTR0_STATUS + (i * 12));
		if (val) {
			pos = 0;
			while ((pos = find_next_bit(&val, 32, pos)) != 32) {
				/* FIXME : WA for bz69520
				 * To avoid race condition during avk the interrupt disabling interrupt before
				 * Ack and enabling after Ack.
				 */
				spin_lock(&pp->intr_lock);
				mask = pcie_readl_relaxed(pp->va_dbi_base + PCIE_MSI_INTR0_ENABLE + (i * 12));
				pcie_writel_relaxed(mask & ~(1 << pos), pp->va_dbi_base + PCIE_MSI_INTR0_ENABLE + (i * 12));
				pcie_writel_relaxed((1 << pos), pp->va_dbi_base + PCIE_MSI_INTR0_STATUS + (i * 12));
				pcie_writel_relaxed(mask & (1 << pos), pp->va_dbi_base + PCIE_MSI_INTR0_ENABLE + (i * 12));
				spin_unlock(&pp->intr_lock);
				generic_handle_irq(pp->msi_base	+ (i * 32) + pos);
				pos++;
			}
		}
	}
#endif
}
#else
static void handle_msi(struct pcie_port *pp)
{
}
#endif

static void comcerto_pcie_int_handler(unsigned int irq, struct irq_desc *desc)
{
	struct pcie_port *pp = irq_get_handler_data(irq);
	struct pcie_app_reg *app_reg = pp->app_regs;
	struct irq_chip *chip;
	unsigned int status;
	int pos, ii;

	chip = irq_desc_get_chip(desc);
	chained_irq_enter(chip, desc);
	status = pcie_readl_relaxed(pp->va_app_base + app_reg->intr_sts);

	if (status & INTR_CTRL_MSI) {
		pcie_writel_relaxed(INTR_CTRL_MSI, (pp->va_app_base + app_reg->intr_sts));
		status &= ~(INTR_CTRL_MSI);
		handle_msi(pp);
	}

	for (ii = 0; (ii < PCIE_NUM_INTX_IRQS) && status; ii++) {
		pos = ii << 1;

		/* Handle INTx Assert */
		if (status & (INTR_CTRL_INTA_ASSERT << pos)) {
			status &= ~(INTR_CTRL_INTA_ASSERT << pos);
			pcie_writel_relaxed((INTR_CTRL_INTA_ASSERT << pos), (pp->va_app_base + app_reg->intr_sts));
			generic_handle_irq(pp->intx_base + ii);
		}

		/*FIXME : No need to handle DEASSERT, simply clear the interrupt */
		status &= ~(INTR_CTRL_INTA_DEASSERT << pos);
#if 0
		/* Handle INTx Deasert */
		if (status & (INTR_CTRL_INTA_DEASSERT << pos)) {
			status &= ~(INTR_CTRL_INTA_DEASSERT << pos);
			pcie_writel_relaxed((INTR_CTRL_INTA_DEASSERT << pos), (pp->va_app_base + app_reg->intr_sts));
		}
#endif
	}

	if (status) {
		printk(KERN_INFO "%s:Unhandled interrupt %x\n", __func__, status);
		/* FIXME: HP, AER, PME interrupts need to be handled */
		pcie_writel_relaxed(status, (pp->va_app_base + app_reg->intr_sts));
	}

	chained_irq_exit(chip, desc);
}


static void pcie_nop_intx_irq(struct irq_data *data )
{
	return;
}

static void pcie_unmask_intx_irq( struct irq_data *data )
{
	struct pcie_port *pp = data->chip_data;

	spin_lock(&pp->conf_lock);
	comcerto_pcie_wr_conf(pp, pp->root_bus_nr, 0, PCI_COMMAND, 2, (pp->cmd_reg_val & ~PCI_COMMAND_INTX_DISABLE));
	spin_unlock(&pp->conf_lock);
}

static void pcie_enable_intx_irq( struct irq_data *data )
{
	int irq = data->irq;
	struct pcie_port *pp = data->chip_data;
	int irq_offset = irq - pp->intx_base;
	struct pcie_app_reg *app_reg = (struct pcie_app_reg *)pp->app_regs;
	unsigned long flags;

	if( irq_offset >= PCIE_NUM_INTX_IRQS )
		return;

	/*
	 * In interrupt sts/mask register each interrupt has two
	 * consecutive bits, one for assert and another for dessert.
	 */
	irq_offset = irq_offset << 1;

	spin_lock_irqsave(&pp->intr_lock, flags);
	pcie_writel_relaxed(pcie_readl_relaxed(pp->va_app_base + app_reg->intr_en) | ( INTR_CTRL_INTA_ASSERT << irq_offset),
			(pp->va_app_base + app_reg->intr_en) );
	spin_unlock_irqrestore(&pp->intr_lock, flags);
}

static void pcie_mask_intx_irq( struct irq_data *data )
{
	struct pcie_port *pp = data->chip_data;

	spin_lock(&pp->conf_lock);
	comcerto_pcie_wr_conf(pp, pp->root_bus_nr, 0, PCI_COMMAND, 2, (pp->cmd_reg_val | PCI_COMMAND_INTX_DISABLE));
	spin_unlock(&pp->conf_lock);
}

static void pcie_disable_intx_irq( struct irq_data *data )
{
	int irq = data->irq;
	struct pcie_port *pp = data->chip_data;
	int irq_offset = irq - pp->intx_base;
	struct pcie_app_reg *app_reg = (struct pcie_app_reg *)pp->app_regs;
	unsigned long flags;

	if( irq_offset >= PCIE_NUM_INTX_IRQS )
		return;

	/*
	 * In interrupt sts/mask register each interrupt has two
	 * consecutive bits, one for assert and another for dessert.
	 */
	irq_offset = irq_offset << 1;

	spin_lock_irqsave(&pp->intr_lock, flags);
	pcie_writel_relaxed( pcie_readl_relaxed(pp->va_app_base + app_reg->intr_en) & ~( INTR_CTRL_INTA_ASSERT << irq_offset),
			(pp->va_app_base + app_reg->intr_en) );
	spin_unlock_irqrestore(&pp->intr_lock, flags);
}

static struct irq_chip pcie_intx_chip = {
	.name = "PCIe INTx",
	.irq_ack = pcie_nop_intx_irq,
	.irq_enable = pcie_enable_intx_irq,
	.irq_disable = pcie_disable_intx_irq,
	.irq_mask = pcie_mask_intx_irq,
	.irq_unmask = pcie_unmask_intx_irq,
};

static int comcerto_pcie_intx_init(struct pcie_port *pp)
{
	int i, irq;
	struct pcie_app_reg *app_reg;

	/* Disable INTX interrupt*/
	app_reg = pp->app_regs;

	pcie_writel(pcie_readl(pp->va_app_base + app_reg->intr_en) &
			~(INTR_CTRL_INTA_ASSERT |
				INTR_CTRL_INTB_ASSERT |
				INTR_CTRL_INTC_ASSERT |
				INTR_CTRL_INTD_ASSERT),
			pp->va_app_base + app_reg->intr_en );

	/* initilize INTX chip here only. MSI chip will be
	 * initilized dynamically.*/
	irq = pp->intx_base;
	for (i = 0; i < PCIE_NUM_INTX_IRQS; i++) {
		irq_set_chip_data(irq + i, pp);
		irq_set_chip_and_handler(irq + i, &pcie_intx_chip,
				handle_level_irq);
		set_irq_flags(irq + i, IRQF_VALID);
	}

	irq_set_handler_data(pp->irq, pp);
	irq_set_chained_handler(pp->irq, comcerto_pcie_int_handler);

	/* FIXME Added for debuging */
	pcie_writel(pcie_readl(pp->va_app_base + app_reg->intr_en) &
			~(INTR_CTRL_AER |  INTR_CTRL_PME |
 			 INTR_CTRL_HP  |  INTR_CTRL_LINK_AUTO_BW ),
			pp->va_app_base + app_reg->intr_en );


	return 0;
}

static int comcerto_pcie_rc_int_init( struct pcie_port *pp )
{

	printk(KERN_INFO "%s\n",__func__);

	comcerto_pcie_intx_init(pp);

#ifdef CONFIG_PCI_MSI
	comcerto_msi_init(pp);
#endif

	return 0;
}

#ifdef CONFIG_PCI_MSI
static int find_valid_pos0(int port, int nvec, int pos, int *pos0)
{
	int flag = 1;
	do {
		pos = find_next_zero_bit(msi_irq_in_use[port],
				PCIE_NUM_MSI_IRQS, pos);
		/*if you have reached to the end then get out from here.*/
		if (pos == PCIE_NUM_MSI_IRQS)
			return -ENOSPC;
		/* Check if this position is at correct offset.nvec is always a
		 * power of two. pos0 must be nvec bit alligned.
		 */
		if (pos % nvec)
			pos += nvec - (pos % nvec);
		else
			flag = 0;
	} while (flag);

	*pos0 = pos;
	return 0;
}

#define GET_MSI_INT_REG_POS(_irq) ((_irq >> 5) * 12)
#define GET_MSI_INT_OFST(_irq) (_irq & 0x1F)

static void comcerto_msi_nop(struct irq_data *data)
{
	return;
}

static void comcerto_msi_unmask(struct irq_data *data)
{
	struct pcie_port *pp = data->chip_data;
	int irq = data->irq - pp->msi_base;
	int ofst, pos;
	unsigned int val;
	unsigned long flags;

	if( irq >= PCIE_NUM_MSI_IRQS )
		return;

	pos = GET_MSI_INT_REG_POS(irq);
	ofst = GET_MSI_INT_OFST(irq);
#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s: %x:%x\n",__func__, pos, ofst);
#endif

	spin_lock_irqsave(&pp->intr_lock, flags);
	comcerto_dbi_read_reg(pp, PCIE_MSI_INTR0_MASK + pos, 4, &val);
	val &= ~(1 << ofst);
	comcerto_dbi_write_reg(pp, PCIE_MSI_INTR0_MASK + pos, 4, val);
	spin_unlock_irqrestore(&pp->intr_lock, flags);

	return;
}

static void comcerto_msi_mask(struct irq_data *data)
{
	struct pcie_port *pp = data->chip_data;
	int irq = data->irq - pp->msi_base;
	int ofst, pos;
	unsigned int val;
	unsigned long flags;

	if( irq >= PCIE_NUM_MSI_IRQS )
		return;

	pos = GET_MSI_INT_REG_POS(irq);
	ofst = GET_MSI_INT_OFST(irq);
#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s: %x:%x\n",__func__, pos, ofst);
#endif

	spin_lock_irqsave(&pp->intr_lock, flags);
	comcerto_dbi_read_reg(pp, PCIE_MSI_INTR0_MASK + pos, 4, &val);
	val |= (1 << ofst);
	comcerto_dbi_write_reg(pp, PCIE_MSI_INTR0_MASK + pos, 4, val);
	spin_unlock_irqrestore(&pp->intr_lock, flags);

	return;
}


static void comcerto_msi_enable(struct irq_data *data)
{
	struct pcie_port *pp = data->chip_data;
	int irq = data->irq - pp->msi_base;
	int ofst, pos;
	unsigned int val;
	unsigned long flags;

	if( irq >= PCIE_NUM_MSI_IRQS )
		return;

	pos = GET_MSI_INT_REG_POS(irq);
	ofst = GET_MSI_INT_OFST(irq);

#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s: %x:%x\n",__func__, pos, ofst);
#endif

	spin_lock_irqsave(&pp->intr_lock, flags);
	comcerto_dbi_read_reg(pp, PCIE_MSI_INTR0_ENABLE + pos, 4, &val);
	val |= (1 << ofst);
	comcerto_dbi_write_reg(pp, PCIE_MSI_INTR0_ENABLE + pos, 4, val);
	spin_unlock_irqrestore(&pp->intr_lock, flags);

	return;
}


static void comcerto_msi_disable(struct irq_data *data)
{
	struct pcie_port *pp = data->chip_data;
	int irq = data->irq - pp->msi_base;
	int ofst, pos;
	unsigned int val;
	unsigned long flags;

	if( irq >= PCIE_NUM_MSI_IRQS )
		return;

	pos = GET_MSI_INT_REG_POS(irq);
	ofst = GET_MSI_INT_OFST(irq);
#ifdef COMCERTO_PCIE_DEBUG
	printk(KERN_DEBUG "%s: %x:%x\n",__func__, pos, ofst);
#endif

	spin_lock_irqsave(&pp->intr_lock, flags);
	comcerto_dbi_read_reg(pp, PCIE_MSI_INTR0_ENABLE + pos, 4, &val);
	val &= ~(1 << ofst);
	comcerto_dbi_write_reg(pp, PCIE_MSI_INTR0_ENABLE + pos, 4, val);
	spin_unlock_irqrestore(&pp->intr_lock, flags);

	return;
}

static struct irq_chip comcerto_msi_chip = {
	.name = "PCI-MSI",
	.irq_ack = comcerto_msi_nop,
	.irq_enable = comcerto_msi_enable,
	.irq_disable = comcerto_msi_disable,
	.irq_mask = comcerto_msi_mask,
	.irq_unmask = comcerto_msi_unmask,
};

/*
 * Dynamic irq allocate and deallocation
 */
static int get_irq(int nvec, struct msi_desc *desc, int *pos)
{
	int irq, pos0, pos1, i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 19, 0)
	struct pci_dev *pdev = container_of(desc->dev, struct pci_dev, dev);
#else
	struct pci_dev *pdev = desc->dev;
#endif
	struct pcie_port *pp = bus_to_port(pdev->bus->number);

	pos0 = find_first_zero_bit(msi_irq_in_use[pp->port],
			PCIE_NUM_MSI_IRQS);
	if (pos0 % nvec) {
		if (find_valid_pos0(pp->port, nvec, pos0, &pos0))
			goto no_valid_irq;
	}
	if (nvec > 1) {
		pos1 = find_next_bit(msi_irq_in_use[pp->port],
				PCIE_NUM_MSI_IRQS, pos0);
		/* there must be nvec number of consecutive free bits */
		while ((pos1 - pos0) < nvec) {
			if (find_valid_pos0(pp->port, nvec, pos1, &pos0))
				goto no_valid_irq;
			pos1 = find_next_bit(msi_irq_in_use[pp->port],
					PCIE_NUM_MSI_IRQS, pos0);
		}
	}

	irq = pp->msi_base + pos0;

	if ((irq + nvec) > (pp->msi_base + PCIE_NUM_MSI_IRQS))
		goto no_valid_irq;

	i = 0;
	while (i < nvec) {
		set_bit(pos0 + i, msi_irq_in_use[pp->port]);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
		irq_init_desc(irq + i);
#else
		dynamic_irq_init(irq + i);
#endif
		irq_set_chip_data(irq + i, pp);
		irq_set_chip_and_handler(irq + i, &comcerto_msi_chip,
				handle_simple_irq);
		set_irq_flags(irq + i, IRQF_VALID);
		i++;
	}

	irq_set_msi_desc(irq, desc);

	*pos = pos0;
	return irq;
no_valid_irq:
	printk(KERN_ERR "%s : MSI interrupt allocate failed\n", __func__);
	*pos = pos0;
	return -ENOSPC;
}

static void clean_irq(unsigned int irq)
{
	int res, bit, val, pos;
	struct irq_data *data = irq_get_irq_data(irq);
	struct pcie_port *pp = data->chip_data;
	unsigned long flags;

	if( !pp )
		return;

	pos = irq - pp->msi_base;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 16, 0)
	irq_init_desc(irq);
#else
	dynamic_irq_init(irq);
#endif

	spin_lock_irqsave(&pp->msi_map_lock, flags);
	clear_bit(pos, msi_irq_in_use[pp->port]);

	/* Disable corresponding interrupt on MSI interrupt
	 * controller.
	 */
	res = (pos / 32) * 12;
	bit = pos % 32;
	comcerto_dbi_read_reg(pp, PCIE_MSI_INTR0_ENABLE + res, 4, &val);
	val &= ~(1 << bit);
	comcerto_dbi_write_reg(pp, PCIE_MSI_INTR0_ENABLE + res, 4, val);
	spin_unlock_irqrestore(&pp->msi_map_lock, flags);

}

int arch_setup_msi_irq(struct pci_dev *pdev, struct msi_desc *desc)
{
	int irq, pos;
	struct msi_msg msg;
	struct pcie_port *pp = bus_to_port(pdev->bus->number);
	unsigned long flags;

	spin_lock_irqsave(&pp->msi_map_lock, flags);
	irq = get_irq(1, desc, &pos);
	spin_unlock_irqrestore(&pp->msi_map_lock, flags);


	if (irq < 0)
		return irq;

	desc->msi_attrib.multiple = 0;

	/* An EP will modify lower 8 bits(max) of msi data while
	 * sending any msi interrupt
	 */
	msg.address_hi = 0x0;
	msg.address_lo = pp->msi_mbox_handle;
	msg.data = pos;
	write_msi_msg(irq, &msg);

	return 0;
}

void arch_teardown_msi_irq(unsigned int irq)
{
	clean_irq(irq);
}

static int comcerto_msi_init(struct pcie_port *pp)
{
	struct pcie_app_reg *app_reg = (struct pcie_app_reg *)pp->app_regs;

	pp->msi_mbox_baseaddr = dma_alloc_coherent(NULL, sizeof(u32), &pp->msi_mbox_handle, GFP_KERNEL);
	if (!pp->msi_mbox_baseaddr) {
		printk(KERN_ERR "PCIe(%d): failed to allocate msi mailbox coherent memory\n", pp->port);
		goto err;
	}

	comcerto_dbi_write_reg(pp, PCIE_MSI_ADDR_LO, 4, pp->msi_mbox_handle);
	comcerto_dbi_write_reg(pp, PCIE_MSI_ADDR_HI, 4, 0);
	/* Enbale MSI interrupt*/
	pcie_writel(pcie_readl(pp->va_app_base + app_reg->intr_en) | INTR_CTRL_MSI,
			pp->va_app_base + app_reg->intr_en);
	return 0;

err:
	return -1;
}
#endif

static void comcerto_pcie_rc_init(struct pcie_port *pp)
{
	struct pcie_app_reg *app_reg = pp->app_regs;
	unsigned int val;


	//FIXME : Bit:27 definition is not clear from document
	//	  This register setting is copied from simulation code.
	pcie_writel(pcie_readl(pp->va_app_base + app_reg->cfg0) | 0x08007FF0,
			pp->va_app_base + app_reg->cfg0);

	comcerto_dbi_read_reg(pp, PCIE_AFL0L1_REG, 4, &val);
	val &= ~(0x00FFFF00);
	val |= 0x00F1F100;
	comcerto_dbi_write_reg(pp, PCIE_AFL0L1_REG, 4, val);

	if(pcie_gen1_only)
	{
		comcerto_dbi_write_reg(pp, PCIE_LCNT2_REG, 4, 0x1);
		comcerto_dbi_write_reg(pp, PCIE_LCAP_REG, 4, 0x1);
	}
	else
	{
		comcerto_dbi_read_reg(pp, PCIE_G2CTRL_REG, 4, &val);
		val &= ~(0xFF);
		val |= 0xF1;
		comcerto_dbi_write_reg(pp, PCIE_G2CTRL_REG, 4, val);

		// instruct pcie to switch to gen2 after init
		comcerto_dbi_read_reg(pp, PCIE_G2CTRL_REG, 4, &val);
		val |= (1 << 17);
		comcerto_dbi_write_reg(pp, PCIE_G2CTRL_REG, 4, val);
	}

	/*setup iATU for outbound translation */
	PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_MEM, iATU_GET_MEM_BASE(pp->remote_mem_baseaddr),
			iATU_MEM_SIZE - 1, 0, 0, pp->remote_mem_baseaddr );
	PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_IO, iATU_GET_IO_BASE(pp->remote_mem_baseaddr),
			iATU_IO_SIZE - 1, (AXI_OP_TYPE_IO_RDRW & iATU_CTRL1_TYPE_MASK),
			0, iATU_GET_IO_BASE(pp->remote_mem_baseaddr) );
	PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_MSG, iATU_GET_MSG_BASE(pp->remote_mem_baseaddr),
			iATU_MSG_SIZE - 1, (AXI_OP_TYPE_MSG_REQ & iATU_CTRL1_TYPE_MASK),
			0, iATU_GET_MSG_BASE(pp->remote_mem_baseaddr) );
	PCIE_SETUP_iATU_IB_ENTRY( pp, 0, 0,
			INBOUND_ADDR_MASK, 0, 0, COMCERTO_AXI_DDR_BASE);

}

static int pcie_app_init(struct pcie_port *pp, int nr, int mode)
{

	if (nr > MAX_PCIE_PORTS) {
		printk(KERN_ERR "%s: Invalid port\n", __func__);
		goto err0;
	}

	if (mode != CFG0_DEV_TYPE_RC) {
		printk(KERN_ERR "%s: Unsupported mode selected mode: %d. \n", __func__, mode);
		goto err0;
	}

        memset(pp, 0, sizeof(struct pcie_port));

	pp->port = nr;
        pp->app_regs = &app_regs[nr];
        pp->root_bus_nr = nr;
        pp->base = pcie_cnf_base_addr[nr];
        pp->app_base = (unsigned long)COMCERTO_APB_PCI_SATA_USB_CTRL_BASE;
	pp->va_app_base = (void __iomem *)APB_VADDR(COMCERTO_APB_PCI_SATA_USB_CTRL_BASE);
        pp->remote_mem_baseaddr = pcie_remote_base_addr[nr];

	if (!nr)
		pp->va_dbi_base = (void __iomem *) COMCERTO_AXI_PCIe0_VADDR_BASE;
	else
		pp->va_dbi_base = (void __iomem *) COMCERTO_AXI_PCIe1_VADDR_BASE;

	pp->va_cfg0_base = (void __iomem *)
		ioremap( iATU_GET_CFG0_BASE(pp->remote_mem_baseaddr), iATU_CFG0_SIZE);
	if (!pp->va_cfg0_base) {
		pr_err("error with ioremap in function %s\n", __func__);
		goto err0;
	}
	pp->va_cfg1_base = (void __iomem *)
		ioremap( iATU_GET_CFG1_BASE(pp->remote_mem_baseaddr) , iATU_CFG1_SIZE);
	if (!pp->va_cfg1_base) {
		pr_err("error with ioremap in function %s\n", __func__);
		goto err1;
	}

	pp->intx_base = pcie_intx_base[nr];
	pp->msi_base = pcie_msi_base[nr];
	pp->irq = pcie_irqs[nr];
	spin_lock_init(&pp->conf_lock);
	spin_lock_init(&pp->intr_lock);
	spin_lock_init(&pp->msi_map_lock);
	memset(pp->res, 0, sizeof(pp->res));

	/* Get the reference clock to PCIe port*/
	switch (nr) {
		case 0:
			pp->ref_clock = clk_get(NULL, "pcie0");
			break;
		case 1:
			pp->ref_clock = clk_get(NULL, "pcie1");
			break;
		default:
			printk(KERN_ERR "%s: Invalid port\n", __func__);
			goto err2;
	}
	if (IS_ERR(pp->ref_clock)) {
		pr_err("%s: Unable to obtain pcie%d clock: %ld\n", __func__, nr, PTR_ERR(pcie_port[nr].ref_clock));
		goto err2;
	}

	/* Enable the PCIE_OCC clock */
#if defined(CONFIG_COMCERTO_PCIE_OCC_CLOCK)
	pp->occ_clock = clk_get(NULL,"pcie_occ");
	if (IS_ERR(pp->occ_clock)) {
		pr_err("%s: Unable to obtain pcie_occ clock: %ld\n", __func__, PTR_ERR(pp->occ_clock));
		goto err_occ_clock;
	}
#endif
	pcie_port_set_mode(nr, mode);
	
	return 0;

#if defined(CONFIG_COMCERTO_PCIE_OCC_CLOCK)
err_occ_clock:
	clk_put(pp->occ_clock);
#endif

err2:
	iounmap(pp->va_cfg1_base);
err1:
	iounmap(pp->va_cfg0_base);
err0:
	return -1;

}

#if defined(CONFIG_C2K_EVM) || defined(CONFIG_C2K_ASIC) || defined(CONFIG_SEQUOIA)
#define PCIE_DEV_EXT_RESET_DEASSERT(_id) \
	pcie_writel(pcie_readl(COMCERTO_GPIO_OUTPUT_REG) & ~(GPIO_PIN_27), COMCERTO_GPIO_OUTPUT_REG);

#define PCIE_DEV_EXT_RESET_ASSERT(_id) \
	pcie_writel(pcie_readl(COMCERTO_GPIO_OUTPUT_REG) | (GPIO_PIN_27), COMCERTO_GPIO_OUTPUT_REG);
#else
/* Board specific */
#define PCIE_DEV_EXT_RESET_DEASSERT(_id)
#define PCIE_DEV_EXT_RESET_ASSERT(_id)
#endif

static int comcerto_pcie_device_reset(struct pcie_port *pp)
{
	int ii;
	struct pcie_port *l_pp = pp;
	struct pci_dev *pci_dev = NULL;

	printk(KERN_INFO "ENTER: Bringing PCIe%d device reset\n", pp->port);
	if (!pp->link_state)
		return -1;

	/* On C2KEVM and ASIC same reset is connected to PCIe0/1.
         * So, device might have kept in reset, using other PCIe host.
	 */
	if (!comcerto_pcie_link_up(pp)) {
		printk(KERN_INFO "%s : Device is already link down state\n", __func__);
		return 0;
	}


	/*FIXME : Below code might be required if we want to reset pcie device, without
         *        invoking pcie device supend. If we invoke pcie device suspend it will
	 *	  take care of saving pcie config space.
         */
#if 0
	/* Now save the PCIe device configuration space.*/
	while((pci_dev = pci_get_subsys(PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, pci_dev))) {
		if (pp->root_bus_nr == pci_dev->bus->number) {
			pci_save_state(pci_dev);
		}
	}

	/* On C2KEVM/ASIC, since same GPIO27 is used to reset PCIe0/PCIe1 devices,
	 * save configuration of devices on other PCIe also.
	 * This may not be applicable for other cutomer boards.
	 */
#if defined(CONFIG_C2K_EVM) || defined(CONFIG_C2K_ASIC) || defined(CONFIG_SEQUOIA)
	l_pp = &pcie_port[!pp->port];
	pci_dev = NULL;

	if (l_pp->link_state) {
		while((pci_dev = pci_get_subsys(PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, pci_dev))) {
			if (l_pp->root_bus_nr == pci_dev->bus->number) {
				pci_save_state(pci_dev);
			}
		}
	}
#endif
#endif

	/************** Ready to issue rest *****************/
	/* De-assert external reset (GPIO-27) */
	PCIE_DEV_EXT_RESET_DEASSERT(pp->port);
	printk(KERN_INFO "EXIT: Bringing PCIe%d device reset\n", pp->port);

	return 0;

}


static int comcerto_pcie_device_reset_exit(struct pcie_port *pp)
{
	unsigned int val;
	struct pci_dev *pci_dev;

	printk(KERN_INFO "ENTER: Bringing PCIe%d device out-of-reset\n", pp->port);

	if (!pp->link_state && pp->reset)
		return -1;

	/* Pull up external reset */
	/* assert external reset (GPIO-27) */
	PCIE_DEV_EXT_RESET_ASSERT(pp->port);

	udelay(1000);
	udelay(1000);
	udelay(1000);
	udelay(1000);
	udelay(1000);
	udelay(1000);
	udelay(1000);

	/* Restore the RC configuration */
	comcerto_dbi_read_reg(pp, PCIE_AFL0L1_REG, 4, &val);
	val &= ~(0x00FFFF00);
	val |= 0x00F1F100;
	comcerto_dbi_write_reg(pp, PCIE_AFL0L1_REG, 4, val);

	if(pcie_gen1_only)
	{
		comcerto_dbi_write_reg(pp, PCIE_LCNT2_REG, 4, 0x1);
		comcerto_dbi_write_reg(pp, PCIE_LCAP_REG, 4, 0x1);
	}
	else
	{
		comcerto_dbi_read_reg(pp, PCIE_G2CTRL_REG, 4, &val);
		val &= ~(0xFF);
		val |= 0xF1;
		comcerto_dbi_write_reg(pp, PCIE_G2CTRL_REG, 4, val);
	}

	// instruct pcie to switch to gen2 after init
	comcerto_dbi_read_reg(pp, PCIE_G2CTRL_REG, 4, &val);
	val |= (1 << 17);
	comcerto_dbi_write_reg(pp, PCIE_G2CTRL_REG, 4, val);

	/*setup iATU for outbound translation */
	PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_MEM, iATU_GET_MEM_BASE(pp->remote_mem_baseaddr),
			iATU_MEM_SIZE - 1, 0, 0, pp->remote_mem_baseaddr );
	PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_IO, iATU_GET_IO_BASE(pp->remote_mem_baseaddr),
			iATU_IO_SIZE - 1, (AXI_OP_TYPE_IO_RDRW & iATU_CTRL1_TYPE_MASK),
			0, iATU_GET_IO_BASE(pp->remote_mem_baseaddr) );
	PCIE_SETUP_iATU_OB_ENTRY( pp, iATU_ENTRY_MSG, iATU_GET_MSG_BASE(pp->remote_mem_baseaddr),
			iATU_MSG_SIZE - 1, (AXI_OP_TYPE_MSG_REQ & iATU_CTRL1_TYPE_MASK),
			0, iATU_GET_MSG_BASE(pp->remote_mem_baseaddr) );
	PCIE_SETUP_iATU_IB_ENTRY( pp, 0, 0,
			INBOUND_ADDR_MASK, 0, 0, COMCERTO_AXI_DDR_BASE);

	comcerto_dbi_write_reg(pp, PCIE_MSI_ADDR_LO, 4, pp->msi_mbox_handle);
	comcerto_dbi_write_reg(pp, PCIE_MSI_ADDR_HI, 4, 0);

	pcie_writel_relaxed(0x7, pp->va_app_base + pp->app_regs->cfg5);

	/* Generic PCIe unit setup.*/

	/* Enable own BME. It is necessary to enable own BME to do a
	 * memory transaction on a downstream device
	 */
	comcerto_dbi_read_reg(pp, PCI_COMMAND, 2, &val);
	val |= (PCI_COMMAND_IO | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER
			| PCI_COMMAND_PARITY | PCI_COMMAND_SERR);
	comcerto_dbi_write_reg(pp, PCI_COMMAND, 2, val);

	pp->cfg0_prev_taddr = 0xffffffff;
	pp->cfg1_prev_taddr = 0xffffffff;

	//udelay(1000);

	if(comcerto_pcie_link_up(pp)) {
	printk(KERN_INFO " Bringing PCIe%d device out-of-reset : Link Up\n", pp->port);
	/*FIXME : Below code might be required if we want to bring pcie device out-of-reset,
         *	  without invoking pcie device supend. If we invoke pcie device resume it will
	 *	  take care of restoring, saved pcie config space.
         */
#if 0
		pci_dev = NULL;
		while((pci_dev = pci_get_subsys(PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, pci_dev))) {
			if (pp->root_bus_nr == pci_dev->bus->number) {
				pci_restore_state(pci_dev);
			}
		}
#endif
	}

	printk(KERN_INFO "EXIT: Bringing PCIe%d device out-of-reset\n", pp->port);
	return 0;

}

static ssize_t comcerto_pcie_show_reset(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *pdev = to_platform_device(dev);
        struct pcie_port *pp = &pcie_port[pdev->id];

	return sprintf(buf, "%d\n", pp->reset);
}

static ssize_t comcerto_pcie_set_reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
        struct pcie_port *pp = &pcie_port[pdev->id];
	int reset = 0, rc;

	if (!pcie_port_is_host(pdev->id) ||  !(pp->link_state))
			return count;

	sscanf(buf, "%d", &reset);

	reset = reset ? 1:0;

	if (pp->reset == reset) {
		printk(KERN_INFO "%s: Already in same state\n", __func__);
		return count;
	}


	if (reset) {
		printk(KERN_INFO "ENTER : Putting PCIe%d device into reset\n", pdev->id);

		if (!comcerto_pcie_device_reset(pp)) {
			int ii = 10;

			/* Wait for link_req_rst_not, to be de-asseted */
			while (ii--) {

				if (!(pcie_readl( pp->va_app_base + pp->app_regs->sts0 ) & STS0_LINK_REQ_RST_NOT)) {
					printk(KERN_INFO "%s : (PCIe%d) link_req_rst_not is de-asseted\n", __func__, pp->port);
					break;
				}

				udelay(1000);
			}

			if (ii == 10) 
				printk(KERN_WARNING "%s : (PCIe%d) link_req_rst_not is not de-asseted \n", __func__, pp->port);

			pp->reset = 1;
			/* Disable LTSSM and initiate linkdown reset */
			pcie_writel((pcie_readl(pp->va_app_base + pp->app_regs->cfg5) &
						~(CFG5_LTSSM_ENABLE)) | CFG5_LINK_DOWN_RST,
						 pp->va_app_base + pp->app_regs->cfg5);
			udelay(1000);
		}

		printk(KERN_INFO "Disabling PCIe%d Controler Clock\n", pdev->id);

		if (pcie_port[pdev->id].port_mode != PCIE_PORT_MODE_NONE)
			clk_disable(pcie_port[pdev->id].ref_clock);

		printk(KERN_INFO "EXIT : Putting PCIe%d device into reset\n", pdev->id);
	}
	else {

		printk(KERN_INFO "ENTER: Bringing PCIe%d device outof reset\n", pdev->id);

		printk(KERN_INFO "Enabling PCIe%d Controler Clock\n", pdev->id);

		if(pcie_port[pdev->id].port_mode != PCIE_PORT_MODE_NONE) {
			rc = clk_enable(pcie_port[pdev->id].ref_clock);
			if (rc)
				pr_err("%s: PCIe%d clock enable failed\n", __func__, pdev->id);
		}

		if (!comcerto_pcie_device_reset_exit(pp)) {
			pp->reset = 0;
		}
	}

	return count;
}

static DEVICE_ATTR(device_reset, 0644, comcerto_pcie_show_reset, comcerto_pcie_set_reset);


static ssize_t comcerto_pcie_serdes_pd(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct platform_device *pdev = to_platform_device(dev);
        struct pcie_port *pp = &pcie_port[pdev->id];
	int reset = 0, rc;

	sscanf(buf, "%d", &reset);

	reset = reset ? 1:0;

	if (reset) {
		printk(KERN_INFO "%s: Putting Serdes to Low Power and CMU Power Off\n", __func__);

		if (pdev->id)
			pcie_writel(pcie_readl(USBPHY_SERDES_STAT_BASE+0x44) | ((0x3 << 2)|(0x1 << 7)) , USBPHY_SERDES_STAT_BASE+0x44);
		else
			pcie_writel(pcie_readl(USBPHY_SERDES_STAT_BASE+0x34) | ((0x3 << 2)|(0x1 << 7)) , USBPHY_SERDES_STAT_BASE+0x34);
	} else {
		printk(KERN_INFO "%s: Getting Serdes out of Low Power and CMU Power On\n", __func__);

		if (pdev->id)
			pcie_writel(pcie_readl(USBPHY_SERDES_STAT_BASE+0x44) & ~((0x3 << 2)|(0x1 << 7)) , USBPHY_SERDES_STAT_BASE+0x44);
		else
			pcie_writel(pcie_readl(USBPHY_SERDES_STAT_BASE+0x34) & ~((0x3 << 2)|(0x1 << 7)) , USBPHY_SERDES_STAT_BASE+0x34);
	}

	return count;
}

static DEVICE_ATTR(serdes_pd, 0644, comcerto_pcie_show_reset, comcerto_pcie_serdes_pd);


#ifdef CONFIG_PM
static int no_irq_resume ;

static int comcerto_pcie_suspend(struct device *dev)
{
	unsigned int val, i;
	struct platform_device *pdev = to_platform_device(dev);

	printk(KERN_INFO "%s: pcie device %p (id = %d):\n",
			 __func__, pdev, pdev->id);

	/* Check for the BitMask bit for PCIe, if it is enabled
	 * then we are not going suspend the PCIe device , as by
	 * this device , we will wake from System Resume.
	 */
	if ( !(host_utilpe_shared_pmu_bitmask & PCIe0_IRQ) || !(host_utilpe_shared_pmu_bitmask & PCIe1_IRQ) ){
 		/* We will Just return from here */
		return 0;
	}
	if (pcie_port[pdev->id].port_mode != PCIE_PORT_MODE_NONE) {
		if (comcerto_pcie_link_up(&pcie_port[pdev->id])){
			/* Enable PME to root Port */
			comcerto_dbi_read_reg(&pcie_port[pdev->id], (PCI_CAP_PM + PCI_PM_CTRL), 4, &val);
			comcerto_dbi_write_reg(&pcie_port[pdev->id], (PCI_CAP_PM + PCI_PM_CTRL), 4, val | PCI_PM_CTRL_STATE_MASK);
			/* Required PM Delay */
			for (i = 0 ; i < 40 ; i++)
				udelay(500);
		}
	}
	no_irq_resume =0 ;
	return 0;
}

static int comcerto_pcie_resume(struct device *dev)
{
	unsigned int val, i;
	struct platform_device *pdev = to_platform_device(dev);

	printk(KERN_INFO "%s: pcie device %p (id = %d)\n", 
		__func__, pdev, pdev->id);
 	/* Check for the Bit_Mask bit for PCIe, if it is enabled
 	 * then we are not going suspend the PCIe device , as by
	 * this device , we will wake from System Resume.
 	*/
	if ( !(host_utilpe_shared_pmu_bitmask & PCIe0_IRQ) || !(host_utilpe_shared_pmu_bitmask & PCIe1_IRQ) ){

 		/* We will Just return
 		*/
		return 0;
	}
	if( no_irq_resume == 0)
       	{
		if(pcie_port[pdev->id].port_mode != PCIE_PORT_MODE_NONE) {
			if (comcerto_pcie_link_up(&pcie_port[pdev->id])){
		    		/* Put In D0 State */
		    		comcerto_dbi_read_reg(&pcie_port[pdev->id], (PCI_CAP_PM + PCI_PM_CTRL), 4, &val);
		    		comcerto_dbi_write_reg(&pcie_port[pdev->id], (PCI_CAP_PM + PCI_PM_CTRL), 4, val & (~PCI_PM_CTRL_STATE_MASK));

				/* Required PM Delay */
		    		for (i = 0 ; i < 40 ; i++)
			    		udelay(500);
	    		}	
		}	
	}
	return 0;
}

static int comcerto_pcie_noirq_resume(struct device *dev)
{
	int val,i;
	struct platform_device *pdev = to_platform_device(dev);

	printk(KERN_INFO "%s: pcie device %p (id = %d)\n",
			 __func__, pdev, pdev->id);

	/* Check for the Bit_Mask bit for PCIe, if it is enabled
	 * then we are not going suspend the PCIe device , as by
	 * this device , we will wake from System Resume.
	 */
	if ( !(host_utilpe_shared_pmu_bitmask & PCIe0_IRQ) || !(host_utilpe_shared_pmu_bitmask & PCIe1_IRQ) ){
               /* We will Just return
                */
		return 0;
	}
	if(pcie_port[pdev->id].port_mode != PCIE_PORT_MODE_NONE) {
		if (comcerto_pcie_link_up(&pcie_port[pdev->id])){
			/* Put In D0 State */
			comcerto_dbi_read_reg(&pcie_port[pdev->id], (PCI_CAP_PM + PCI_PM_CTRL), 4, &val);
			comcerto_dbi_write_reg(&pcie_port[pdev->id], (PCI_CAP_PM + PCI_PM_CTRL), 4, val & (~PCI_PM_CTRL_STATE_MASK));
			/* Required PM Delay */
			for (i = 0 ; i < 40 ; i++)
				udelay(500);
	    	}
	}	
	return 0;
}

static const struct dev_pm_ops pcie_platform_pm_ops = {
	.suspend = comcerto_pcie_suspend,
	.resume = comcerto_pcie_resume,
	.resume_noirq = comcerto_pcie_noirq_resume,
};


static struct platform_driver comcerto_pcie_driver = {
	.driver = {
		.name = "pcie",
		.pm   = &pcie_platform_pm_ops,
		.owner = THIS_MODULE,
	},
};

static struct platform_device pcie_pwr0 = {
	.name = "pcie",
	.id = 0,
};

static struct platform_device pcie_pwr1 = {
	.name = "pcie",
	.id = 1,
};
#endif

static void comcerto_serdes_set_polarity(struct serdes_regs_s *p_pcie_phy_reg_file, int current_polarity)
{
	switch(current_polarity)
	{
		case 0:
			p_pcie_phy_reg_file[0x73].val = 0x0;
			p_pcie_phy_reg_file[0x75].val = 0x0;
			break;
		case 1: 
			p_pcie_phy_reg_file[0x73].val = 0x8;
			p_pcie_phy_reg_file[0x75].val = 0x2;
			break;
		case 2:
			p_pcie_phy_reg_file[0x73].val = 0x0;
			p_pcie_phy_reg_file[0x75].val = 0x2;
			break;
		case 3:
			p_pcie_phy_reg_file[0x73].val = 0x8;
			p_pcie_phy_reg_file[0x75].val = 0x0;
			break;
	}

}

static int comcerto_pcie_bsp_link_init(struct pcie_port *pp, int nr, struct serdes_regs_s *p_pcie_phy_reg_file, int serdes_reg_size, int current_polarity)
{
	int axi_pcie_component;
	int pcie_component;
	int serdes_component;
	int rc;
	int if_err = 1;

	comcerto_serdes_set_polarity(p_pcie_phy_reg_file, current_polarity);

	if(nr == 0)
	{
		axi_pcie_component = COMPONENT_AXI_PCIE0;
		pcie_component = COMPONENT_SERDES_PCIE0;	
		serdes_component = COMPONENT_SERDES0;
	}
	else
	{
		axi_pcie_component = COMPONENT_AXI_PCIE1;
		pcie_component = COMPONENT_SERDES_PCIE1;	
		serdes_component = COMPONENT_SERDES1;
	}

	//Bring serdes out of reset
	c2000_block_reset(axi_pcie_component,0);
	c2000_block_reset(serdes_component,0);

	/* SW select for ck_soc_div_i SOC clock */
	pcie_writel(0xFF3C, COMCERTO_SERDES_DWC_CFG_REG( nr, SD_PHY_CTRL3_REG_OFST ));
	pcie_writel(pcie_readl(COMCERTO_SERDES_DWC_CFG_REG( nr, SD_PHY_CTRL2_REG_OFST )) & ~0x3,
			COMCERTO_SERDES_DWC_CFG_REG( nr, SD_PHY_CTRL2_REG_OFST ));

	rc = clk_enable(pp->ref_clock);
	if (rc){
		pr_err("%s: PCIe%d clock enable failed\n", __func__, nr);
		goto err1;
	}
	
	/* Enable the PCIE_OCC clock */	
#if defined(CONFIG_COMCERTO_PCIE_OCC_CLOCK)
	rc =  clk_enable(pp->occ_clock);
	if (rc){
		pr_err("%s: PCIe_occ clock enable failed\n", __func__);
		goto err_occ_clock;
	}
#endif

	/* Serdes Initialization. */
	if( serdes_phy_init(nr,  p_pcie_phy_reg_file,
				serdes_reg_size/ sizeof(serdes_regs_t),
				SD_DEV_TYPE_PCIE) )
	{
		pp->port_mode = PCIE_PORT_MODE_NONE;
		pr_err("%s: Failed to initialize serdes (%d)\n", __func__, nr );
		goto err_phy_link;
	}

	mdelay(1); //After CMU locks wait for sometime

#if defined(CONFIG_C2K_MFCN_EVM)
	if(nr == 0){
		GPIO_reset_external_device(COMPONENT_PCIE0,0);
	}else{
		GPIO_reset_external_device(COMPONENT_PCIE1,0);
	}

	mdelay(1);
#endif
	//Bring PCIe out of reset
	c2000_block_reset(pcie_component,0);

	//Hold the LTSSM in detect state
	pcie_writel(pcie_readl(pp->va_app_base + pp->app_regs->cfg5) & ~CFG5_LTSSM_ENABLE,
			pp->va_app_base + pp->app_regs->cfg5);

	comcerto_pcie_rc_init(pp);

	//Enable LTSSM to start link initialization
	pcie_writel(pcie_readl(pp->va_app_base + pp->app_regs->cfg5) | (CFG5_APP_INIT_RST | CFG5_LTSSM_ENABLE),
			pp->va_app_base + pp->app_regs->cfg5);

	pp->link_state = comcerto_pcie_link_up( &pcie_port[nr] );

	if(!pp->link_state)
	{
		if_err = 0;
		goto err_phy_link;
	}

	return 0;

err_phy_link:
	clk_disable(pp->ref_clock);
	clk_put(pp->ref_clock);
#if defined(CONFIG_COMCERTO_PCIE_OCC_CLOCK)
err_occ_clock:
	clk_disable(pp->occ_clock);
	clk_put(pp->occ_clock);
#endif

err1:
	//Put all to reset
	c2000_block_reset(axi_pcie_component,1);
	c2000_block_reset(serdes_component,1);
	c2000_block_reset(pcie_component,1);

	if(if_err)
		return -1;
	else
		return 0;

}

static int comcerto_pcie_bsp_init(struct pcie_port *pp, int nr)
{
	struct serdes_regs_s *p_pcie_phy_reg_file;
	int serdes_regs_size;
	int polarity_max = 4;
	int polarity;
	int ret;

	if (nr >= NUM_PCIE_PORTS) {
		printk("%s : Invalid PCIe port number\n", __func__);
		goto err0;
	}

	ret = pcie_app_init(pp, nr, CFG0_DEV_TYPE_RC);
	if(ret == -1)
		goto err0;;

	pp->port_mode = pcie_port_get_mode(nr);

	if(pcie_external_clk)
	{
		p_pcie_phy_reg_file = &pcie_phy_reg_file_100[0];
		serdes_regs_size = sizeof(pcie_phy_reg_file_100);
	}
	else
	{
		if(HAL_get_ref_clk() == REF_CLK_24MHZ)
		{
                        p_pcie_phy_reg_file = &pcie_phy_reg_file_24[0];
			serdes_regs_size = sizeof(pcie_phy_reg_file_24);
			printk(KERN_INFO "PCIe: Ref clk 24Mhz\n");
		}
                else
		{
                        p_pcie_phy_reg_file = &pcie_phy_reg_file_48[0];
			serdes_regs_size = sizeof(pcie_phy_reg_file_48);
			printk(KERN_INFO "PCIe: Ref clk 48Mhz\n");
		}
        }

	if (system_rev == 1) {
		printk(KERN_INFO "PCIe: Detected C2K RevA1 device serdes clk devider old:new=%x:%x\n",
				p_pcie_phy_reg_file[0x61].val, 0x06);
		p_pcie_phy_reg_file[0x61].val = 0x6;
	}

	for(polarity = 0 ; polarity < polarity_max; polarity++)
	{

		ret = comcerto_pcie_bsp_link_init(pp, nr, p_pcie_phy_reg_file, serdes_regs_size, polarity);
		if(ret == -1)
			goto err1;;

		if (pp->link_state)
			goto linkup;

		if(!pcie_port_is_host(nr))
			return 0; //Endpoint, so no need to change polarity

	}
	
	printk(KERN_INFO "PCIe%d: Link Up Failed \n",nr);

err1: 	
	iounmap(pp->va_cfg0_base);
	iounmap(pp->va_cfg1_base);
	pp->port_mode = PCIE_PORT_MODE_NONE;
err0:
	return -1;

linkup:
	printk(KERN_INFO "PCIe%d: Link Up Success \n",nr);
	printk(KERN_INFO "PCIe%d: Polarity: %d Gen1 mode: %d External Clk: %d\n",nr, polarity, pcie_gen1_only,pcie_external_clk);
	if (pcie_port_is_host(nr))
		comcerto_pcie_rc_int_init(pp);	
	return 0;

}

static int comcerto_pcie_abort_handler(unsigned long addr, unsigned int fsr,
                                      struct pt_regs *regs)
{
        if (fsr & (1 << 10))
                regs->ARM_pc += 4;
        return 0;
}


static int __init comcerto_pcie_init(void)
{
        struct pcie_port *pp;
	int i, rc;
	int num_pcie_port = 1;
	struct pci_dev *pdev = NULL;

        pp = &pcie_port[0];
	comcerto_pcie_bsp_init(pp, 0);

	if ( (NUM_PCIE_PORTS == 2)  &&
                        !(pcie_readl(COMCERTO_GPIO_SYSTEM_CONFIG) & BOOT_SERDES1_CNF_SATA0) )
        {
                num_pcie_port = 2;
		pp = &pcie_port[1];
		comcerto_pcie_bsp_init(pp, 1);
        }

	comcerto_pcie.nr_controllers = num_pcie_port;

	pcibios_min_io = iATU_GET_IO_BASE(COMCERTO_AXI_PCIe0_SLAVE_BASE);
	pcibios_min_mem = COMCERTO_AXI_PCIe0_SLAVE_BASE;
	pci_add_flags(PCI_REASSIGN_ALL_RSRC);

	hook_fault_code(16 + 6, comcerto_pcie_abort_handler, SIGBUS, 0, "imprecise external abort");

	pci_common_init(&comcerto_pcie);

	for ( i = 0; i < num_pcie_port; i++ )
	{

		if (!pcie_port_is_host(i) ||  !(pcie_port[i].link_state))
			continue;

		while((pdev = pci_get_subsys(PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, PCI_ANY_ID, pdev))) {
			if (pcie_port[i].root_bus_nr == pdev->bus->number) {
				if( (rc = pcie_get_readrq(pdev)) > 512 ) {
					printk(KERN_WARNING "PCIe%d Device rdreq size (%d) is more than supported\n", i, rc);
					pcie_set_readrq(pdev, 512);
				}
			}
		}
	}


#ifdef CONFIG_PM
	platform_device_register(&pcie_pwr0);

	if (device_create_file(&pcie_pwr0.dev, &dev_attr_device_reset))
		printk(KERN_ERR "%s: Unable to create pcie0 reset sysfs entry\n", __func__);

	if (device_create_file(&pcie_pwr0.dev, &dev_attr_serdes_pd))
		printk(KERN_ERR "%s: Unable to create pcie0 serdes_pd sysfs entry\n", __func__);

	if(num_pcie_port > 1) {
		platform_device_register(&pcie_pwr1);

		if (device_create_file(&pcie_pwr1.dev, &dev_attr_device_reset))
			printk(KERN_ERR "%s: Unable to create pcie1 reset sysfs entry\n", __func__);

		if (device_create_file(&pcie_pwr1.dev, &dev_attr_serdes_pd))
			printk(KERN_ERR "%s: Unable to create pcie1 serdes_pd sysfs entry\n", __func__);
	}

	platform_driver_register(&comcerto_pcie_driver);
#endif

	return 0;
}
subsys_initcall(comcerto_pcie_init);



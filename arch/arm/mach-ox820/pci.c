 
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/mach/pci.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/system.h>

#define VERSION_ID_MAGIC 0x082510b5

static int link_up[2];
static DEFINE_SPINLOCK(pciea_lock);
static DEFINE_SPINLOCK(pcieb_lock);

void __iomem * ox820_ioremap(
	unsigned long phys_addr,
	size_t        size,
	unsigned int  mtype)
{
	 
	if (!is_pci_phys_addr(phys_addr)) {
 
		return __arm_ioremap(phys_addr, size, mtype);
	}

	return (void*)pci_phys_to_virt(phys_addr);
}
EXPORT_SYMBOL(ox820_ioremap);

void ox820_iounmap(void __iomem *virt_addr)
{
	 
	if (!is_pci_virt_addr((u32)virt_addr)) {
 
		__iounmap(virt_addr);
	}
 
}
EXPORT_SYMBOL(ox820_iounmap);

static inline int controller_index_from_phys(u32 phys)
{
	return phys >= PCIEB_CLIENT_BASE_PA;
}

static inline int controller_index_from_virt(u32 virt)
{
	return virt >= PCIEB_CLIENT_BASE;
}

unsigned long __ox820_inl(u32 phys)
{
	unsigned long           value;

	if (!link_up[controller_index_from_phys(phys)]) {
 
		value = ~0UL;
	} else {
		u32 virt = pci_phys_to_virt(phys);

		value = __raw_readl(virt);
 
	}

	return value;
}

unsigned short __ox820_inw(u32 phys)
{
	 
	unsigned short value;

	value = (unsigned short)(inl(phys & ~3) >> (phys | 3));
 
	return value;
}

unsigned char __ox820_inb(u32 phys)
{
	 
	unsigned char value;

	value = (unsigned char)(inl(phys & ~3) >> (phys | 3));
 
	return value;
}

static void inline set_out_lanes(
	u32           virt,
	unsigned char lanes)
{
	 
	unsigned long slave_reg_adr =
		controller_index_from_virt(virt) ? SYS_CTRL_PCIEB_AHB_SLAVE_CTRL :
										   SYS_CTRL_PCIEA_AHB_SLAVE_CTRL;

	unsigned long slave_reg_val = readl(slave_reg_adr);
	slave_reg_val &= ~(0xf << SYS_CTRL_PCIE_SLAVE_BE_BIT);
	slave_reg_val |= (lanes << SYS_CTRL_PCIE_SLAVE_BE_BIT);
	writel(slave_reg_val, slave_reg_adr);
}

static void out_lanes(
	u32           value,
	u32           phys,
	unsigned char lanes)
{
 
	if (!link_up[controller_index_from_phys(phys)]) {
 
	} else {
		unsigned long flags;
		u32           virt = pci_phys_to_virt(phys);

		if (lanes != 0xf) {
			 
			spin_lock_irqsave(
				controller_index_from_phys(phys) ? &pcieb_lock : &pciea_lock, flags);

			set_out_lanes(virt, lanes);
		}

		__raw_writel(value, virt);
 
		if (lanes != 0xf) {
			 
			set_out_lanes(virt, 0xf);

			spin_unlock_irqrestore(
				controller_index_from_phys(phys) ? &pcieb_lock : &pciea_lock, flags);
		}
	}
}

void __ox820_outl(
	unsigned long value,
	u32           phys)
{
 
	out_lanes(value, phys, 0xf);
}

void __ox820_outw(
	unsigned short value,
	u32            phys)
{
	u32 quad_val = (u32)value << (8 * (phys & 3));
 
	out_lanes(quad_val, phys & ~3, 3 << (phys & 3));
}

void __ox820_outb(
	unsigned char value,
	u32           phys)
{
	u32 quad_val = (unsigned long)value << (8 * (phys & 3));
 
	out_lanes(quad_val, phys & ~3, 1 << (phys & 3));
}

void __ox820_outsb(u32 p, unsigned char  * from, u32 len)	{ while (len--) { __ox820_outb((*from++),(p) ); } }
void __ox820_outsw(u32 p, unsigned short * from, u32 len)	{ while (len--) { __ox820_outw((*from++),(p) ); } }
void __ox820_outsl(u32 p, unsigned long  * from, u32 len)	{ while (len--) { __ox820_outl((*from++),(p) ); } }
                                  
void __ox820_insb(u32 p, unsigned char  * to, u32 len)	{ while (len--) { *to++ = __ox820_inb(p); } }
void __ox820_insw(u32 p, unsigned short * to, u32 len)	{ while (len--) { *to++ = __ox820_inw(p); } }
void __ox820_insl(u32 p, unsigned long  * to, u32 len)	{ while (len--) { *to++ = __ox820_inl(p); } }

EXPORT_SYMBOL(__ox820_inb);
EXPORT_SYMBOL(__ox820_inw);
EXPORT_SYMBOL(__ox820_inl);

EXPORT_SYMBOL(__ox820_outb);
EXPORT_SYMBOL(__ox820_outw);
EXPORT_SYMBOL(__ox820_outl);

EXPORT_SYMBOL(__ox820_insb);
EXPORT_SYMBOL(__ox820_insw);
EXPORT_SYMBOL(__ox820_insl);

EXPORT_SYMBOL(__ox820_outsb);
EXPORT_SYMBOL(__ox820_outsw);
EXPORT_SYMBOL(__ox820_outsl);

#ifdef CONFIG_PCI

#define LINK_UP_TIMEOUT_SECONDS 3

#define TOTAL_WINDOW_SIZE	(64*1024*1024)

#define NON_PREFETCHABLE_WINDOW_SIZE	(32*1024*1024)
#define PREFETCHABLE_WINDOW_SIZE		(30*1024*1024)
#define IO_WINDOW_SIZE					(1*1024*1024)

#if ((NON_PREFETCHABLE_WINDOW_SIZE + PREFETCHABLE_WINDOW_SIZE + IO_WINDOW_SIZE) >= TOTAL_WINDOW_SIZE)
#error "PCIe windows sizes incorrect"
#endif

#define NON_PREFETCHABLE_WINDOW_OFFSET	0 
#define PREFETCHABLE_WINDOW_OFFSET		(NON_PREFETCHABLE_WINDOW_SIZE)
#define IO_WINDOW_OFFSET				(NON_PREFETCHABLE_WINDOW_SIZE + PREFETCHABLE_WINDOW_SIZE)
#define CONFIG_WINDOW_OFFSET			(NON_PREFETCHABLE_WINDOW_SIZE + PREFETCHABLE_WINDOW_SIZE + IO_WINDOW_SIZE)

static int __init ox820_map_irq(
	struct pci_dev *dev,
	u8              slot,
	u8              pin)
{
 
	return dev->bus->number ? PCIEB_INTERRUPT : PCIEA_INTERRUPT;
}

static struct resource pciea_non_mem = {
	.name	= "PCIeA non-prefetchable",
	.start	= PCIEA_CLIENT_BASE_PA + NON_PREFETCHABLE_WINDOW_OFFSET,
	.end	= PCIEA_CLIENT_BASE_PA + PREFETCHABLE_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_MEM,
};

static struct resource pciea_pre_mem = {
	.name	= "PCIeA prefetchable",
	.start	= PCIEA_CLIENT_BASE_PA + PREFETCHABLE_WINDOW_OFFSET,
	.end	= PCIEA_CLIENT_BASE_PA + IO_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_MEM | IORESOURCE_PREFETCH,
};

static struct resource pciea_io_mem = {
	.name	= "PCIeA I/O space",
	.start	= PCIEA_CLIENT_BASE_PA + IO_WINDOW_OFFSET,
	.end	= PCIEA_CLIENT_BASE_PA + CONFIG_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_IO,
};

static int __init ox820_pciea_setup_resources(struct resource **resource)
{
	int ret = 0;

	ret = request_resource(&iomem_resource, &pciea_io_mem);
	if (ret) {
		printk(KERN_ERR "PCIeA: unable to allocate I/O memory region (%d)\n", ret);
		goto out;
	}
	ret = request_resource(&iomem_resource, &pciea_non_mem);
	if (ret) {
		printk(KERN_ERR "PCIeA: unable to allocate non-prefetchable memory region (%d)\n", ret);
		goto release_io_mem;
	}
	ret = request_resource(&iomem_resource, &pciea_pre_mem);
	if (ret) {
		printk(KERN_ERR "PCIeA: unable to allocate prefetchable memory region (%d)\n", ret);
		goto release_non_mem;
	}

	resource[0] = &pciea_io_mem;
	resource[1] = &pciea_non_mem;
	resource[2] = &pciea_pre_mem;

	goto out;

release_non_mem:
	release_resource(&pciea_non_mem);
release_io_mem:
	release_resource(&pciea_io_mem);
out:
	return ret;
}

static struct resource pcieb_non_mem = {
	.name	= "PCIeB non-prefetchable",
	.start	= PCIEB_CLIENT_BASE_PA + NON_PREFETCHABLE_WINDOW_OFFSET,
	.end	= PCIEB_CLIENT_BASE_PA + PREFETCHABLE_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_MEM,
};

static struct resource pcieb_pre_mem = {
	.name	= "PCIeB prefetchable",
	.start	= PCIEB_CLIENT_BASE_PA + PREFETCHABLE_WINDOW_OFFSET,
	.end	= PCIEB_CLIENT_BASE_PA + IO_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_MEM | IORESOURCE_PREFETCH,
};

static struct resource pcieb_io_mem = {
	.name	= "PCIeB I/O space",
	.start	= PCIEB_CLIENT_BASE_PA + IO_WINDOW_OFFSET,
	.end	= PCIEB_CLIENT_BASE_PA + CONFIG_WINDOW_OFFSET - 1,
	.flags	= IORESOURCE_IO,
};

static int __init ox820_pcieb_setup_resources(struct resource **resource)
{
	int ret = 0;

	ret = request_resource(&iomem_resource, &pcieb_io_mem);
	if (ret) {
		printk(KERN_ERR "PCIeB: unable to allocate I/O memory region (%d)\n", ret);
		goto out;
	}
	ret = request_resource(&iomem_resource, &pcieb_non_mem);
	if (ret) {
		printk(KERN_ERR "PCIeB: unable to allocate non-prefetchable memory region (%d)\n", ret);
		goto release_io_mem;
	}
	ret = request_resource(&iomem_resource, &pcieb_pre_mem);
	if (ret) {
		printk(KERN_ERR "PCIeB: unable to allocate prefetchable memory region (%d)\n", ret);
		goto release_non_mem;
	}

	resource[0] = &pcieb_io_mem;
	resource[1] = &pcieb_non_mem;
	resource[2] = &pcieb_pre_mem;

	goto out;

release_non_mem:
	release_resource(&pcieb_non_mem);
release_io_mem:
	release_resource(&pcieb_io_mem);
out:
	return ret;
}

int __init ox820_pci_setup(
	int                  nr,
	struct pci_sys_data *sys)
{
	int ret = 0;
 
	if (nr == 0) {
		sys->mem_offset = 0;
		ret = ox820_pciea_setup_resources(sys->resource);
		if (ret < 0) {
			printk(KERN_ERR "ox820_pci_setup: Failed to setup PCIeA resources\n");
			goto out;
		}
	} else if (nr == 1) {
		sys->mem_offset = 0;
		ret = ox820_pcieb_setup_resources(sys->resource);
		if (ret < 0) {
			printk(KERN_ERR "ox820_pci_setup: Failed to setup PCIeA resources\n");
			goto out;
		}
	}
	return 1;

out:
	return ret;
}

static int controller_index_from_bus(int primary_bus_number)
{
	return !!primary_bus_number;
}

static unsigned long pci_addr(
	int           primary_bus_number,
	unsigned char bus_number,
	unsigned int  devfn,
	int           where)
{
	unsigned int  slot = PCI_SLOT(devfn);
	unsigned int  function = PCI_FUNC(devfn);
	int           controller;
	unsigned char modified_bus_number;
	unsigned long addr;

	BUG_ON(slot != 0);
	
	controller = controller_index_from_bus(primary_bus_number);

	modified_bus_number = bus_number - primary_bus_number;

	addr = controller ? (PCIEB_CLIENT_BASE + CONFIG_WINDOW_OFFSET) :
					    (PCIEA_CLIENT_BASE + CONFIG_WINDOW_OFFSET);

	addr += ((modified_bus_number << 20) | (slot << 15) | (function << 12) | (where & ~3));

	return addr;
}

static int ox820_read_config(
	struct pci_bus *bus,
	unsigned int    devfn,
	int             where,
	int             size,
	u32            *val)
{
	struct pci_sys_data *sys = bus->sysdata;
	unsigned int         slot = PCI_SLOT(devfn);
	u32                  v;

	if (!link_up[controller_index_from_bus(sys->busnr)] || slot > 0) {
 
		*val = ~0UL;
	} else {
		 
		unsigned long addr = pci_addr(sys->busnr, bus->number, devfn, where);

		v = __raw_readl(addr);
 
		switch (size) {
			case 1:
				if (where & 2) v >>= 16;
				if (where & 1) v >>= 8;
				v &= 0xff;
				break;
			case 2:
				if (where & 2) v >>= 16;
				v &= 0xffff;
				break;
			case 4:
				break;
			default:
				BUG();
		}

		*val = v;
 
	}

	return PCIBIOS_SUCCESSFUL;
}

static int ox820_write_config(
	struct pci_bus *bus,
	unsigned int    devfn,
	int             where,
	int             size,
	u32             val)
{
	struct pci_sys_data *sys = bus->sysdata;
	unsigned int         slot = PCI_SLOT(devfn);

	if (!link_up[controller_index_from_bus(sys->busnr)] || slot > 0) {
 
	} else {
		 
		u32 virt = pci_addr(sys->busnr, bus->number, devfn, where);

		if (size == 4) {
 
			__raw_writel(val, virt);
		} else {
			 
			unsigned long flags;
			u32           quad_val = val << (8 * (virt & 3));

			spin_lock_irqsave(
				controller_index_from_virt(virt) ? &pcieb_lock : &pciea_lock, flags);

			set_out_lanes(virt, (3 >> (2-size)) << (virt & 3));
			__raw_writel(quad_val, virt & ~3);
			set_out_lanes(virt, 0xf);

			spin_unlock_irqrestore(
				controller_index_from_virt(virt) ? &pcieb_lock : &pciea_lock, flags);
		}
	}

	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops ox820_pci_ops = {
	.read	= ox820_read_config,
	.write	= ox820_write_config,
};

struct pci_bus *ox820_pci_scan_bus(
	int                  nr,
	struct pci_sys_data *sys)
{
 
	return pci_scan_bus(sys->busnr, &ox820_pci_ops, sys);
 
}

#if (CONFIG_OXNAS_PCIE_RESET_GPIO < SYS_CTRL_NUM_PINS)
#define	PCIE_OUTPUT_SET	GPIO_A_OUTPUT_SET
#define	PCIE_OUTPUT_CLEAR	GPIO_A_OUTPUT_CLEAR
#define	PCIE_OUTPUT_ENABLE_SET	GPIO_A_OUTPUT_ENABLE_SET
#define	PCIE_OUTPUT_ENABLE_CLEAR	GPIO_A_OUTPUT_ENABLE_CLEAR
#else
#define PCIE_RESET_PIN          (CONFIG_OXNAS_PCIE_RESET_GPIO - SYS_CTRL_NUM_PINS)
#define	PCIE_OUTPUT_SET	GPIO_B_OUTPUT_SET
#define	PCIE_OUTPUT_CLEAR	GPIO_B_OUTPUT_CLEAR
#define	PCIE_OUTPUT_ENABLE_SET	GPIO_B_OUTPUT_ENABLE_SET
#define	PCIE_OUTPUT_ENABLE_CLEAR	GPIO_B_OUTPUT_ENABLE_CLEAR
#endif

void __init ox820_pci_preinit(void)
{
	unsigned long end;
	unsigned long version_id;
	unsigned long pin = ( 1 << PCIE_RESET_PIN);

    writel(1<<SYS_CTRL_RSTEN_PLLB_BIT, SYS_CTRL_RSTEN_CLR_CTRL);  
     
    writel(pin, PCIE_OUTPUT_ENABLE_SET);
    writel(pin, PCIE_OUTPUT_CLEAR);
    wmb();
    mdelay(500);	 
    writel(pin, PCIE_OUTPUT_ENABLE_CLEAR);	 
    wmb();

    writel(0x218, SEC_CTRL_PLLB_CTRL0);  
	
    writel(0x0F, SYS_CTRL_HCSL_CTRL);  

    writel(1UL << SYS_CTRL_RSTEN_PCIEPHY_BIT, SYS_CTRL_RSTEN_SET_CTRL);
    wmb();
    writel(1UL << SYS_CTRL_RSTEN_PCIEPHY_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
    wmb();

    writel(1UL << SYS_CTRL_RSTEN_PCIEA_BIT, SYS_CTRL_RSTEN_SET_CTRL);
    writel(1UL << SYS_CTRL_RSTEN_PCIEB_BIT, SYS_CTRL_RSTEN_SET_CTRL);
    wmb();
    writel(1UL << SYS_CTRL_RSTEN_PCIEA_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
    writel(1UL << SYS_CTRL_RSTEN_PCIEB_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
    wmb();

    writel(1UL << SYS_CTRL_CKEN_PCIEA_BIT, SYS_CTRL_CKEN_SET_CTRL);
    writel(1UL << SYS_CTRL_CKEN_PCIEB_BIT, SYS_CTRL_CKEN_SET_CTRL);
     
    writel(1UL << SYS_CTRL_PCIE_READY_ENTR_L23_BIT, SYS_CTRL_PCIEA_CTRL);
    writel(1UL << SYS_CTRL_PCIE_READY_ENTR_L23_BIT, SYS_CTRL_PCIEB_CTRL);
    wmb();

	link_up[0] = link_up[1] = 1;
#ifdef CONFIG_SYNO_PLX_PORTING
	if (1 == CONFIG_SYNO_OXNAS_PEX_NUM) {
		link_up[0] = 1;
		link_up[1] = 0;
	} else {
		link_up[0] = link_up[1] = 0;
	}
#endif

	version_id = readl(PCIEA_DBI_BASE + PCI_CONFIG_VERSION_DEVICEID_REG_OFFSET);
	printk(KERN_INFO "PCIeA version/deviceID %p\n", (void*)version_id);
	if (version_id != VERSION_ID_MAGIC) {
		printk(KERN_INFO "PCIeA controller not found (version_id %p vs "
			"expected %p)\n", (void*)version_id, (void*)VERSION_ID_MAGIC);
		link_up[0] = 0;
	}

	version_id = readl(PCIEB_DBI_BASE + PCI_CONFIG_VERSION_DEVICEID_REG_OFFSET);
	printk(KERN_INFO "PCIeB version/deviceID %p\n", (void*)version_id);
	if (version_id != VERSION_ID_MAGIC) {
		printk(KERN_INFO "PCIeB controller not found (version_id %p vs "
			"expected %p)\n", (void*)version_id, (void*)VERSION_ID_MAGIC);
		link_up[1] = 0;
	}

	if (link_up[0]) {
		 
		writel(SYS_CTRL_PCIE_DEVICE_TYPE_ROOT << SYS_CTRL_PCIE_DEVICE_TYPE_BIT, SYS_CTRL_PCIEA_CTRL);
		wmb();

		writel(1UL << SYS_CTRL_RSTEN_PCIEA_BIT, SYS_CTRL_RSTEN_SET_CTRL);
		wmb();
		writel(1UL << SYS_CTRL_RSTEN_PCIEA_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
		wmb();

		writel(0 << ENABLE_IN_ADDR_TRANS_BIT, IB_ADDR_XLATE_ENABLE);
		wmb();

		writel(pciea_non_mem.start,	SYS_CTRL_PCIEA_IN0_MEM_ADDR);
		writel(pciea_non_mem.end,	SYS_CTRL_PCIEA_IN0_MEM_LIMIT);
		writel(pciea_non_mem.start,	SYS_CTRL_PCIEA_POM0_MEM_ADDR);

		writel(pciea_pre_mem.start,	SYS_CTRL_PCIEA_IN1_MEM_ADDR);
		writel(pciea_pre_mem.end,	SYS_CTRL_PCIEA_IN1_MEM_LIMIT);
		writel(pciea_pre_mem.start,	SYS_CTRL_PCIEA_POM1_MEM_ADDR);

		writel(pciea_io_mem.start,	SYS_CTRL_PCIEA_IN_IO_ADDR);
		writel(pciea_io_mem.end,	SYS_CTRL_PCIEA_IN_IO_LIMIT);

		writel(pciea_io_mem.end + 1,   SYS_CTRL_PCIEA_IN_CFG0_ADDR);
		writel(PCIEA_CLIENT_BASE_PA + TOTAL_WINDOW_SIZE - 1, SYS_CTRL_PCIEA_IN_CFG0_LIMIT);
		wmb();

		writel(readl(SYS_CTRL_PCIEA_CTRL) | (1UL << SYS_CTRL_PCIE_OBTRANS_BIT), SYS_CTRL_PCIEA_CTRL);
		wmb();

		writel(7, PCIEA_DBI_BASE + PCI_CONFIG_COMMAND_STATUS_REG_OFFSET);
		wmb();

		writel(readl(SYS_CTRL_PCIEA_CTRL) | (1UL << SYS_CTRL_PCIE_LTSSM_BIT), SYS_CTRL_PCIEA_CTRL);
		wmb();

		end = jiffies + (LINK_UP_TIMEOUT_SECONDS * HZ);
		while (!(readl(SYS_CTRL_PCIEA_CTRL) & (1UL << SYS_CTRL_PCIE_LINK_UP_BIT))) {
			if (time_after(jiffies, end)) {
				link_up[0] = 0;
				printk(KERN_WARNING "ox820_pci_preinit() PCIEA link up timeout (%p)\n",
					(void*)readl(SYS_CTRL_PCIEA_CTRL));
				break;
			}
		}
	}

	if (link_up[1]) {
		 
		writel(SYS_CTRL_PCIE_DEVICE_TYPE_ROOT << SYS_CTRL_PCIE_DEVICE_TYPE_BIT, SYS_CTRL_PCIEB_CTRL);
		wmb();

		writel(1UL << SYS_CTRL_RSTEN_PCIEB_BIT, SYS_CTRL_RSTEN_SET_CTRL);
		wmb();
		writel(1UL << SYS_CTRL_RSTEN_PCIEB_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
		wmb();

		writel(0 << ENABLE_IN_ADDR_TRANS_BIT, IB_ADDR_XLATE_ENABLE);
		wmb();

		writel(pcieb_non_mem.start,	SYS_CTRL_PCIEB_IN0_MEM_ADDR);
		writel(pcieb_non_mem.end,	SYS_CTRL_PCIEB_IN0_MEM_LIMIT);
		writel(pcieb_non_mem.start,	SYS_CTRL_PCIEB_POM0_MEM_ADDR);

		writel(pcieb_pre_mem.start,	SYS_CTRL_PCIEB_IN1_MEM_ADDR);
		writel(pcieb_pre_mem.end,	SYS_CTRL_PCIEB_IN1_MEM_LIMIT);
		writel(pcieb_pre_mem.start,	SYS_CTRL_PCIEB_POM1_MEM_ADDR);

		writel(pcieb_io_mem.start,	SYS_CTRL_PCIEB_IN_IO_ADDR);
		writel(pcieb_io_mem.end,	SYS_CTRL_PCIEB_IN_IO_LIMIT);

		writel(pcieb_io_mem.end + 1,   SYS_CTRL_PCIEB_IN_CFG0_ADDR);
		writel(PCIEB_CLIENT_BASE_PA + TOTAL_WINDOW_SIZE - 1, SYS_CTRL_PCIEB_IN_CFG0_LIMIT);
		wmb();

		writel(readl(SYS_CTRL_PCIEB_CTRL) | (1UL << SYS_CTRL_PCIE_OBTRANS_BIT), SYS_CTRL_PCIEB_CTRL);
		wmb();

		writel(7, PCIEB_DBI_BASE + PCI_CONFIG_COMMAND_STATUS_REG_OFFSET);
		wmb();

		writel(readl(SYS_CTRL_PCIEB_CTRL) | (1UL << SYS_CTRL_PCIE_LTSSM_BIT), SYS_CTRL_PCIEB_CTRL);

		end = jiffies + (LINK_UP_TIMEOUT_SECONDS * HZ);
		while (!(readl(SYS_CTRL_PCIEB_CTRL) & (1UL << SYS_CTRL_PCIE_LINK_UP_BIT))) {
			if (time_after(jiffies, end)) {
				link_up[1] = 0;
				printk(KERN_WARNING "ox820_pci_preinit() PCIEB link up timeout (%p)\n",
					(void*)readl(SYS_CTRL_PCIEA_CTRL));
				break;
			}
		}
	}
}

static struct hw_pci ox820_pci __initdata = {
	.swizzle        = NULL,
	.map_irq        = ox820_map_irq,
	.setup          = ox820_pci_setup,
	.nr_controllers = 2,
	.scan           = ox820_pci_scan_bus,
	.preinit        = ox820_pci_preinit,
};

static int __init ox820_pci_init(void)
{
 
    pci_common_init(&ox820_pci);
	return 0;
}

static void __exit ox820_pci_exit(void)
{
 
}

subsys_initcall(ox820_pci_init);
module_exit(ox820_pci_exit);

#endif

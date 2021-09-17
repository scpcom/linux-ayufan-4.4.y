#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/init.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/platform.h>
#include <mach/irqs.h>

#include <asm/signal.h>
#include <asm/mach/pci.h>
#include <asm/irq_regs.h>

#include <asm/hardware/pci_v3.h>

#define v3_writeb(o,v) __raw_writeb(v, PCI_V3_VADDR + (unsigned int)(o))
#define v3_readb(o)    (__raw_readb(PCI_V3_VADDR + (unsigned int)(o)))

#define v3_writew(o,v) __raw_writew(v, PCI_V3_VADDR + (unsigned int)(o))
#define v3_readw(o)    (__raw_readw(PCI_V3_VADDR + (unsigned int)(o)))

#define v3_writel(o,v) __raw_writel(v, PCI_V3_VADDR + (unsigned int)(o))
#define v3_readl(o)    (__raw_readl(PCI_V3_VADDR + (unsigned int)(o)))

static DEFINE_RAW_SPINLOCK(v3_lock);

#define PCI_BUS_NONMEM_START	0x00000000
#define PCI_BUS_NONMEM_SIZE	SZ_256M

#define PCI_BUS_PREMEM_START	PCI_BUS_NONMEM_START + PCI_BUS_NONMEM_SIZE
#define PCI_BUS_PREMEM_SIZE	SZ_256M

#if PCI_BUS_NONMEM_START & 0x000fffff
#error PCI_BUS_NONMEM_START must be megabyte aligned
#endif
#if PCI_BUS_PREMEM_START & 0x000fffff
#error PCI_BUS_PREMEM_START must be megabyte aligned
#endif

#undef V3_LB_BASE_PREFETCH
#define V3_LB_BASE_PREFETCH 0

static void __iomem *v3_open_config_window(struct pci_bus *bus,
					   unsigned int devfn, int offset)
{
	unsigned int address, mapaddress, busnr;

	busnr = bus->number;

	BUG_ON(offset > 255);
	BUG_ON(busnr > 255);
	BUG_ON(devfn > 255);

	if (busnr == 0) {
		int slot = PCI_SLOT(devfn);

		address = PCI_FUNC(devfn) << 8;
		mapaddress = V3_LB_MAP_TYPE_CONFIG;

		if (slot > 12)
			 
			mapaddress |= 1 << (slot - 5);
		else
			 
			address |= 1 << (slot + 11);
	} else {
        	 
		mapaddress = V3_LB_MAP_TYPE_CONFIG | V3_LB_MAP_AD_LOW_EN;
		address = (busnr << 16) | (devfn << 8);
	}

	v3_writel(V3_LB_BASE0, v3_addr_to_lb_base(PHYS_PCI_MEM_BASE) |
			V3_LB_BASE_ADR_SIZE_512MB | V3_LB_BASE_ENABLE);

	v3_writel(V3_LB_BASE1, v3_addr_to_lb_base(PHYS_PCI_CONFIG_BASE) |
			V3_LB_BASE_ADR_SIZE_16MB | V3_LB_BASE_ENABLE);
	v3_writew(V3_LB_MAP1, mapaddress);

	return PCI_CONFIG_VADDR + address + offset;
}

static void v3_close_config_window(void)
{
	 
	v3_writel(V3_LB_BASE1, v3_addr_to_lb_base(PHYS_PCI_MEM_BASE + SZ_256M) |
			V3_LB_BASE_ADR_SIZE_256MB | V3_LB_BASE_PREFETCH |
			V3_LB_BASE_ENABLE);
	v3_writew(V3_LB_MAP1, v3_addr_to_lb_map(PCI_BUS_PREMEM_START) |
			V3_LB_MAP_TYPE_MEM_MULTIPLE);

	v3_writel(V3_LB_BASE0, v3_addr_to_lb_base(PHYS_PCI_MEM_BASE) |
			V3_LB_BASE_ADR_SIZE_256MB | V3_LB_BASE_ENABLE);
}

static int v3_read_config(struct pci_bus *bus, unsigned int devfn, int where,
			  int size, u32 *val)
{
	void __iomem *addr;
	unsigned long flags;
	u32 v;

	raw_spin_lock_irqsave(&v3_lock, flags);
	addr = v3_open_config_window(bus, devfn, where);

	switch (size) {
	case 1:
		v = __raw_readb(addr);
		break;

	case 2:
		v = __raw_readw(addr);
		break;

	default:
		v = __raw_readl(addr);
		break;
	}

	v3_close_config_window();
	raw_spin_unlock_irqrestore(&v3_lock, flags);

	*val = v;
	return PCIBIOS_SUCCESSFUL;
}

static int v3_write_config(struct pci_bus *bus, unsigned int devfn, int where,
			   int size, u32 val)
{
	void __iomem *addr;
	unsigned long flags;

	raw_spin_lock_irqsave(&v3_lock, flags);
	addr = v3_open_config_window(bus, devfn, where);

	switch (size) {
	case 1:
		__raw_writeb((u8)val, addr);
		__raw_readb(addr);
		break;

	case 2:
		__raw_writew((u16)val, addr);
		__raw_readw(addr);
		break;

	case 4:
		__raw_writel(val, addr);
		__raw_readl(addr);
		break;
	}

	v3_close_config_window();
	raw_spin_unlock_irqrestore(&v3_lock, flags);

	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops pci_v3_ops = {
	.read	= v3_read_config,
	.write	= v3_write_config,
};

static struct resource non_mem = {
	.name	= "PCI non-prefetchable",
	.start	= PHYS_PCI_MEM_BASE + PCI_BUS_NONMEM_START,
	.end	= PHYS_PCI_MEM_BASE + PCI_BUS_NONMEM_START + PCI_BUS_NONMEM_SIZE - 1,
	.flags	= IORESOURCE_MEM,
};

static struct resource pre_mem = {
	.name	= "PCI prefetchable",
	.start	= PHYS_PCI_MEM_BASE + PCI_BUS_PREMEM_START,
	.end	= PHYS_PCI_MEM_BASE + PCI_BUS_PREMEM_START + PCI_BUS_PREMEM_SIZE - 1,
	.flags	= IORESOURCE_MEM | IORESOURCE_PREFETCH,
};

static int __init pci_v3_setup_resources(struct pci_sys_data *sys)
{
	if (request_resource(&iomem_resource, &non_mem)) {
		printk(KERN_ERR "PCI: unable to allocate non-prefetchable "
		       "memory region\n");
		return -EBUSY;
	}
	if (request_resource(&iomem_resource, &pre_mem)) {
		release_resource(&non_mem);
		printk(KERN_ERR "PCI: unable to allocate prefetchable "
		       "memory region\n");
		return -EBUSY;
	}

	pci_add_resource_offset(&sys->resources, &non_mem, sys->mem_offset);
	pci_add_resource_offset(&sys->resources, &pre_mem, sys->mem_offset);

	return 1;
}

static void __iomem *ap_syscon_base;
#define INTEGRATOR_SC_PCIENABLE_OFFSET	0x18
#define INTEGRATOR_SC_LBFADDR_OFFSET	0x20
#define INTEGRATOR_SC_LBFCODE_OFFSET	0x24

static int
v3_pci_fault(unsigned long addr, unsigned int fsr, struct pt_regs *regs)
{
	unsigned long pc = instruction_pointer(regs);
	unsigned long instr = *(unsigned long *)pc;
#if 0
	char buf[128];

	sprintf(buf, "V3 fault: addr 0x%08lx, FSR 0x%03x, PC 0x%08lx [%08lx] LBFADDR=%08x LBFCODE=%02x ISTAT=%02x\n",
		addr, fsr, pc, instr, __raw_readl(ap_syscon_base + INTEGRATOR_SC_LBFADDR_OFFSET), __raw_readl(ap_syscon_base + INTEGRATOR_SC_LBFCODE_OFFSET) & 255,
		v3_readb(V3_LB_ISTAT));
	printk(KERN_DEBUG "%s", buf);
#endif

	v3_writeb(V3_LB_ISTAT, 0);
	__raw_writel(3, ap_syscon_base + INTEGRATOR_SC_PCIENABLE_OFFSET);

	if ((instr & 0x0c100000) == 0x04100000) {
		int reg = (instr >> 12) & 15;
		unsigned long val;

		if (instr & 0x00400000)
			val = 255;
		else
			val = -1;

		regs->uregs[reg] = val;
		regs->ARM_pc += 4;
		return 0;
	}

	if ((instr & 0x0e100090) == 0x00100090) {
		int reg = (instr >> 12) & 15;

		regs->uregs[reg] = -1;
		regs->ARM_pc += 4;
		return 0;
	}

	return 1;
}

static irqreturn_t v3_irq(int dummy, void *devid)
{
#ifdef CONFIG_DEBUG_LL
	struct pt_regs *regs = get_irq_regs();
	unsigned long pc = instruction_pointer(regs);
	unsigned long instr = *(unsigned long *)pc;
	char buf[128];
	extern void printascii(const char *);

	sprintf(buf, "V3 int %d: pc=0x%08lx [%08lx] LBFADDR=%08x LBFCODE=%02x "
		"ISTAT=%02x\n", IRQ_AP_V3INT, pc, instr,
		__raw_readl(ap_syscon_base + INTEGRATOR_SC_LBFADDR_OFFSET),
		__raw_readl(ap_syscon_base + INTEGRATOR_SC_LBFCODE_OFFSET) & 255,
		v3_readb(V3_LB_ISTAT));
	printascii(buf);
#endif

	v3_writew(V3_PCI_STAT, 0xf000);
	v3_writeb(V3_LB_ISTAT, 0);
	__raw_writel(3, ap_syscon_base + INTEGRATOR_SC_PCIENABLE_OFFSET);

#ifdef CONFIG_DEBUG_LL
	 
	if ((instr & 0x0c100000) == 0x04100000) {
		int reg = (instr >> 16) & 15;
		sprintf(buf, "   reg%d = %08lx\n", reg, regs->uregs[reg]);
		printascii(buf);
	}
#endif
	return IRQ_HANDLED;
}

int __init pci_v3_setup(int nr, struct pci_sys_data *sys)
{
	int ret = 0;

	if (!ap_syscon_base)
		return -EINVAL;

	if (nr == 0) {
		sys->mem_offset = PHYS_PCI_MEM_BASE;
		ret = pci_v3_setup_resources(sys);
	}

	return ret;
}

void __init pci_v3_preinit(void)
{
	unsigned long flags;
	unsigned int temp;
	int ret;

	ap_syscon_base = ioremap(INTEGRATOR_SC_BASE, 0x100);
	if (!ap_syscon_base) {
		pr_err("unable to remap the AP syscon for PCIv3\n");
		return;
	}

	pcibios_min_mem = 0x00100000;

	hook_fault_code(4, v3_pci_fault, SIGBUS, 0, "external abort on linefetch");
	hook_fault_code(6, v3_pci_fault, SIGBUS, 0, "external abort on linefetch");
	hook_fault_code(8, v3_pci_fault, SIGBUS, 0, "external abort on non-linefetch");
	hook_fault_code(10, v3_pci_fault, SIGBUS, 0, "external abort on non-linefetch");

	raw_spin_lock_irqsave(&v3_lock, flags);

	if (v3_readw(V3_SYSTEM) & V3_SYSTEM_M_LOCK)
		v3_writew(V3_SYSTEM, 0xa05f);

	v3_writel(V3_LB_BASE0, v3_addr_to_lb_base(PHYS_PCI_MEM_BASE) |
			V3_LB_BASE_ADR_SIZE_256MB | V3_LB_BASE_ENABLE);
	v3_writew(V3_LB_MAP0, v3_addr_to_lb_map(PCI_BUS_NONMEM_START) |
			V3_LB_MAP_TYPE_MEM);

	v3_writel(V3_LB_BASE1, v3_addr_to_lb_base(PHYS_PCI_MEM_BASE + SZ_256M) |
			V3_LB_BASE_ADR_SIZE_256MB | V3_LB_BASE_PREFETCH |
			V3_LB_BASE_ENABLE);
	v3_writew(V3_LB_MAP1, v3_addr_to_lb_map(PCI_BUS_PREMEM_START) |
			V3_LB_MAP_TYPE_MEM_MULTIPLE);

	v3_writel(V3_LB_BASE2, v3_addr_to_lb_base2(PHYS_PCI_IO_BASE) |
			V3_LB_BASE_ENABLE);
	v3_writew(V3_LB_MAP2, v3_addr_to_lb_map2(0));

	temp = v3_readw(V3_PCI_CFG) & ~V3_PCI_CFG_M_I2O_EN;
	temp |= V3_PCI_CFG_M_IO_REG_DIS | V3_PCI_CFG_M_IO_DIS;
	v3_writew(V3_PCI_CFG, temp);

	printk(KERN_DEBUG "FIFO_CFG: %04x  FIFO_PRIO: %04x\n",
		v3_readw(V3_FIFO_CFG), v3_readw(V3_FIFO_PRIORITY));

	v3_writew(V3_FIFO_PRIORITY, 0x0a0a);

	temp = v3_readw(V3_SYSTEM) | V3_SYSTEM_M_LOCK;
	v3_writew(V3_SYSTEM, temp);

	v3_writeb(V3_LB_ISTAT, 0);
	v3_writew(V3_LB_CFG, v3_readw(V3_LB_CFG) | (1 << 10));
	v3_writeb(V3_LB_IMASK, 0x28);
	__raw_writel(3, ap_syscon_base + INTEGRATOR_SC_PCIENABLE_OFFSET);

	ret = request_irq(IRQ_AP_V3INT, v3_irq, 0, "V3", NULL);
	if (ret)
		printk(KERN_ERR "PCI: unable to grab PCI error "
		       "interrupt: %d\n", ret);

	raw_spin_unlock_irqrestore(&v3_lock, flags);
}

void __init pci_v3_postinit(void)
{
	unsigned int pci_cmd;

	pci_cmd = PCI_COMMAND_MEMORY |
		  PCI_COMMAND_MASTER | PCI_COMMAND_INVALIDATE;

	v3_writew(V3_PCI_CMD, pci_cmd);

	v3_writeb(V3_LB_ISTAT, ~0x40);
	v3_writeb(V3_LB_IMASK, 0x68);

#if 0
	ret = request_irq(IRQ_AP_LBUSTIMEOUT, lb_timeout, 0, "bus timeout", NULL);
	if (ret)
		printk(KERN_ERR "PCI: unable to grab local bus timeout "
		       "interrupt: %d\n", ret);
#endif

	register_isa_ports(PHYS_PCI_MEM_BASE, PHYS_PCI_IO_BASE, 0);
}

#if defined(MY_ABC_HERE)
 
static u8 __init pci_v3_swizzle(struct pci_dev *dev, u8 *pinp)
{
	if (*pinp == 0)
		*pinp = 1;

	return pci_common_swizzle(dev, pinp);
}

static int irq_tab[4] __initdata = {
	IRQ_AP_PCIINT0,	IRQ_AP_PCIINT1,	IRQ_AP_PCIINT2,	IRQ_AP_PCIINT3
};

static int __init pci_v3_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	int intnr = ((slot - 9) + (pin - 1)) & 3;

	return irq_tab[intnr];
}

static struct hw_pci pci_v3 __initdata = {
	.swizzle		= pci_v3_swizzle,
	.setup			= pci_v3_setup,
	.nr_controllers		= 1,
	.ops			= &pci_v3_ops,
	.preinit		= pci_v3_preinit,
	.postinit		= pci_v3_postinit,
};

#ifdef CONFIG_OF

static int __init pci_v3_map_irq_dt(const struct pci_dev *dev, u8 slot, u8 pin)
{
	struct of_phandle_args oirq;
	int ret;

	ret = of_irq_parse_pci(dev, &oirq);
	if (ret) {
		dev_err(&dev->dev, "of_irq_parse_pci() %d\n", ret);
		 
		return 0;
	}

	return irq_create_of_mapping(&oirq);
}

static int __init pci_v3_dtprobe(struct platform_device *pdev,
				struct device_node *np)
{
	struct of_pci_range_parser parser;
	struct of_pci_range range;
	struct resource *res;
	int irq, ret;

	if (of_pci_range_parser_init(&parser, np))
		return -EINVAL;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "unable to obtain PCIv3 base\n");
		return -ENODEV;
	}
	pci_v3_base = devm_ioremap(&pdev->dev, res->start,
				   resource_size(res));
	if (!pci_v3_base) {
		dev_err(&pdev->dev, "unable to remap PCIv3 base\n");
		return -ENODEV;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq <= 0) {
		dev_err(&pdev->dev, "unable to obtain PCIv3 error IRQ\n");
		return -ENODEV;
	}
	ret = devm_request_irq(&pdev->dev, irq, v3_irq, 0,
			"PCIv3 error", NULL);
	if (ret < 0) {
		dev_err(&pdev->dev, "unable to request PCIv3 error IRQ %d (%d)\n", irq, ret);
		return ret;
	}

	for_each_of_pci_range(&parser, &range) {
		if (!range.flags) {
			of_pci_range_to_resource(&range, np, &conf_mem);
			conf_mem.name = "PCIv3 config";
		}
		if (range.flags & IORESOURCE_IO) {
			of_pci_range_to_resource(&range, np, &io_mem);
			io_mem.name = "PCIv3 I/O";
		}
		if ((range.flags & IORESOURCE_MEM) &&
			!(range.flags & IORESOURCE_PREFETCH)) {
			non_mem_pci = range.pci_addr;
			non_mem_pci_sz = range.size;
			of_pci_range_to_resource(&range, np, &non_mem);
			non_mem.name = "PCIv3 non-prefetched mem";
		}
		if ((range.flags & IORESOURCE_MEM) &&
			(range.flags & IORESOURCE_PREFETCH)) {
			pre_mem_pci = range.pci_addr;
			pre_mem_pci_sz = range.size;
			of_pci_range_to_resource(&range, np, &pre_mem);
			pre_mem.name = "PCIv3 prefetched mem";
		}
	}

	if (!conf_mem.start || !io_mem.start ||
	    !non_mem.start || !pre_mem.start) {
		dev_err(&pdev->dev, "missing ranges in device node\n");
		return -EINVAL;
	}

	pci_v3.map_irq = pci_v3_map_irq_dt;
	pci_common_init_dev(&pdev->dev, &pci_v3);

	return 0;
}

#else

static inline int pci_v3_dtprobe(struct platform_device *pdev,
				  struct device_node *np)
{
	return -EINVAL;
}

#endif

static int __init pci_v3_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret;

	ap_syscon_base = ioremap(INTEGRATOR_SC_BASE, 0x100);
	if (!ap_syscon_base) {
		dev_err(&pdev->dev, "unable to remap the AP syscon for PCIv3\n");
		return -ENODEV;
	}

	if (np)
		return pci_v3_dtprobe(pdev, np);

	pci_v3_base = devm_ioremap(&pdev->dev, PHYS_PCI_V3_BASE, SZ_64K);
	if (!pci_v3_base) {
		dev_err(&pdev->dev, "unable to remap PCIv3 base\n");
		return -ENODEV;
	}

	ret = devm_request_irq(&pdev->dev, IRQ_AP_V3INT, v3_irq, 0, "V3", NULL);
	if (ret) {
		dev_err(&pdev->dev, "unable to grab PCI error interrupt: %d\n",
			ret);
		return -ENODEV;
	}

	conf_mem.name = "PCIv3 config";
	conf_mem.start = PHYS_PCI_CONFIG_BASE;
	conf_mem.end = PHYS_PCI_CONFIG_BASE + SZ_16M - 1;
	conf_mem.flags = IORESOURCE_MEM;

	io_mem.name = "PCIv3 I/O";
	io_mem.start = PHYS_PCI_IO_BASE;
	io_mem.end = PHYS_PCI_IO_BASE + SZ_16M - 1;
	io_mem.flags = IORESOURCE_MEM;

	non_mem_pci = 0x00000000;
	non_mem_pci_sz = SZ_256M;
	non_mem.name = "PCIv3 non-prefetched mem";
	non_mem.start = PHYS_PCI_MEM_BASE;
	non_mem.end = PHYS_PCI_MEM_BASE + SZ_256M - 1;
	non_mem.flags = IORESOURCE_MEM;

	pre_mem_pci = 0x10000000;
	pre_mem_pci_sz = SZ_256M;
	pre_mem.name = "PCIv3 prefetched mem";
	pre_mem.start = PHYS_PCI_PRE_BASE + SZ_256M;
	pre_mem.end = PHYS_PCI_PRE_BASE + SZ_256M - 1;
	pre_mem.flags = IORESOURCE_MEM | IORESOURCE_PREFETCH;

	pci_v3.map_irq = pci_v3_map_irq;

	pci_common_init_dev(&pdev->dev, &pci_v3);

	return 0;
}

static const struct of_device_id pci_ids[] = {
	{ .compatible = "v3,v360epc-pci", },
	{},
};

static struct platform_driver pci_v3_driver = {
	.driver = {
		.name = "pci-v3",
		.of_match_table = pci_ids,
	},
};

static int __init pci_v3_init(void)
{
	return platform_driver_probe(&pci_v3_driver, pci_v3_probe);
}

subsys_initcall(pci_v3_init);

static struct map_desc pci_v3_io_desc[] __initdata __maybe_unused = {
	{
		.virtual	= (unsigned long)PCI_MEMORY_VADDR,
		.pfn		= __phys_to_pfn(PHYS_PCI_MEM_BASE),
		.length		= SZ_16M,
		.type		= MT_DEVICE
	}, {
		.virtual	= (unsigned long)PCI_CONFIG_VADDR,
		.pfn		= __phys_to_pfn(PHYS_PCI_CONFIG_BASE),
		.length		= SZ_16M,
		.type		= MT_DEVICE
	}
};

int __init pci_v3_early_init(void)
{
	iotable_init(pci_v3_io_desc, ARRAY_SIZE(pci_v3_io_desc));
	vga_base = (unsigned long)PCI_MEMORY_VADDR;
	pci_map_io_early(__phys_to_pfn(PHYS_PCI_IO_BASE));
	return 0;
}
#endif  

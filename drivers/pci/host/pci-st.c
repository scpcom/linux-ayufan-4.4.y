#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/delay.h>
#include <linux/pci.h>
#include <linux/signal.h>
#include <linux/reset.h>
#include <linux/phy/phy.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>

#define TRANSLATION_CONTROL		0x900
 
#define EP_TRANSLATION_ENABLE		BIT(0)
 
#define RC_PASS_ADDR_RANGE		BIT(1)

#define PIM0_MEM_ADDR_START		0x910
#define PIM1_MEM_ADDR_START		0x914
#define PIM2_MEM_ADDR_START		0x918

#define CFG_BASE_ADDRESS		0x92c

#define CFG_REGION_SIZE			65536

#define CFG_PCIE_CAP			0x70

#define FUNC0_BDF_NUM			0x930

#define FUNC_BDF_NUM(x)			(0x930 + (((x) / 2) * 4))

#define POM0_MEM_ADDR_START		0x960
 
#define IN0_MEM_ADDR_START		0x964
 
#define IN0_MEM_ADDR_LIMIT		0x968

#define POM1_MEM_ADDR_START		0x970
 
#define IN1_MEM_ADDR_START		0x974
 
#define IN1_MEM_ADDR_LIMIT		0x978

#define MSI_ADDRESS			0x820
#define MSI_UPPER_ADDRESS		0x824
#define MSI_OFFSET_REG(n)		((n) * 0xc)
#define MSI_INTERRUPT_ENABLE(n)		(0x828 + MSI_OFFSET_REG(n))
#define MSI_INTERRUPT_MASK(n)		(0x82c + MSI_OFFSET_REG(n))
#define MSI_INTERRUPT_STATUS(n)		(0x830 + MSI_OFFSET_REG(n))
#define MSI_GPIO_REG			0x888
#define MSI_NUM_ENDPOINTS		8
#define INT_PCI_MSI_NR			(MSI_NUM_ENDPOINTS * 32)

#define PORT_LOGIC_DEBUG_REG_0		0x728

#define DEBUG_REG_0_LTSSM_MASK		0x1f
#define S_DETECT_QUIET			0x00
#define S_DETECT_ACT			0x01
#define S_POLL_ACTIVE			0x02
#define S_POLL_COMPLIANCE		0x03
#define S_POLL_CONFIG			0x04
#define S_PRE_DETECT_QUIET		0x05
#define S_DETECT_WAIT			0x06
#define S_CFG_LINKWD_START		0x07
#define S_CFG_LINKWD_ACEPT		0x08
#define S_CFG_LANENUM_WAIT		0x09
#define S_CFG_LANENUM_ACEPT		0x0A
#define S_CFG_COMPLETE			0x0B
#define S_CFG_IDLE			0x0C
#define S_RCVRY_LOCK			0x0D
#define S_RCVRY_SPEED			0x0E
#define S_RCVRY_RCVRCFG			0x0F
#define S_RCVRY_IDLE			0x10
#define S_L0				0x11
#define S_L0S				0x12
#define S_L123_SEND_EIDLE		0x13
#define S_L1_IDLE			0x14
#define S_L2_IDLE			0x15
#define S_L2_WAKE			0x16
#define S_DISABLED_ENTRY		0x17
#define S_DISABLED_IDLE			0x18
#define S_DISABLED			0x19
#define S_LPBK_ENTRY			0x1A
#define S_LPBK_ACTIVE			0x1B
#define S_LPBK_EXIT			0x1C
#define S_LPBK_EXIT_TIMEOUT		0x1D
#define S_HOT_RESET_ENTRY		0x1E
#define S_HOT_RESET			0x1F

#define PCIE_SYS_INT			BIT(5)
#define PCIE_APP_REQ_RETRY_EN		BIT(3)
#define PCIE_APP_LTSSM_ENABLE		BIT(2)
#define PCIE_APP_INIT_RST		BIT(1)
#define PCIE_DEVICE_TYPE		BIT(0)
#define PCIE_DEFAULT_VAL		PCIE_DEVICE_TYPE

struct st_msi {
	struct msi_chip chip;
	DECLARE_BITMAP(used, INT_PCI_MSI_NR);
	void __iomem *regs;
	struct irq_domain *domain;
	int mux_irq;
	spinlock_t reg_lock;
	struct mutex lock;
};

static inline struct st_msi *to_st_msi(struct msi_chip *chip)
{
	return container_of(chip, struct st_msi, chip);
}

struct st_pcie;

struct st_pcie_ops {
	int (*init)(struct st_pcie *st_pcie);
	int (*enable_ltssm)(struct st_pcie *st_pcie);
	int (*disable_ltssm)(struct st_pcie *st_pcie);
	bool phy_auto;
};

struct st_pcie {
	struct device *dev;
#ifdef MY_DEF_HERE
	u8 root_bus_nr;
#endif  

	void __iomem *cntrl;
	void __iomem *ahb;
	int syscfg0;
	int syscfg1;
	u32 ahb_val;

	struct st_msi *msi;
	struct phy *phy;

	void __iomem *config_area;
	struct resource io;
	struct resource mem;
	struct resource prefetch;
	struct resource busn;
	struct resource *lmi;
	spinlock_t abort_lock;

	const struct st_pcie_ops *data;
	struct regmap *regmap;
	struct reset_control *pwr;
	struct reset_control *rst;
	int irq[4];
	int irq_lines;
	int reset_gpio;
	phys_addr_t config_window_start;
};

static inline struct st_pcie *sys_to_pcie(struct pci_sys_data *sys)
{
	return sys->private_data;
}

static inline u32 shift_data_read(int where, int size, u32 data)
{
	data >>= (8 * (where & 0x3));

	switch (size) {
	case 1:
		data &= 0xff;
		break;
	case 2:
		BUG_ON(where & 1);
		data &= 0xffff;
		break;
	case 4:
		BUG_ON(where & 3);
		break;
	default:
		BUG();
	}

	return data;
}

static u32 dbi_read(struct st_pcie *priv, int where, int size)
{
	u32 data;

	data = readl_relaxed(priv->cntrl + (where & ~0x3));

	return shift_data_read(where, size, data);
}

static inline u8 dbi_readb(struct st_pcie *priv, unsigned addr)
{
	return (u8)dbi_read(priv, addr, 1);
}

static inline u16 dbi_readw(struct st_pcie *priv, unsigned addr)
{
	return (u16)dbi_read(priv, addr, 2);
}

static inline u32 dbi_readl(struct st_pcie *priv, unsigned addr)
{
	return dbi_read(priv, addr, 4);
}

static inline u32 shift_data_write(int where, int size, u32 val, u32 data)
{
	int shift_bits = (where & 0x3) * 8;

	switch (size) {
	case 1:
		data &= ~(0xff << shift_bits);
		data |= ((val & 0xff) << shift_bits);
		break;
	case 2:
		data &= ~(0xffff << shift_bits);
		data |= ((val & 0xffff) << shift_bits);
		BUG_ON(where & 1);
		break;
	case 4:
		data = val;
		BUG_ON(where & 3);
		break;
	default:
		BUG();
	}

	return data;
}

static void dbi_write(struct st_pcie *priv,
		      u32 val, int where, int size)
{
	u32 uninitialized_var(data);
	int aligned_addr = where & ~0x3;

	if (size != 4)
		data = readl_relaxed(priv->cntrl + aligned_addr);

	data = shift_data_write(where, size, val, data);

	writel_relaxed(data, priv->cntrl + aligned_addr);
}

static inline void dbi_writeb(struct st_pcie *priv,
			      u8 val, unsigned addr)
{
	dbi_write(priv, (u32) val, addr, 1);
}

static inline void dbi_writew(struct st_pcie *priv,
			      u16 val, unsigned addr)
{
	dbi_write(priv, (u32) val, addr, 2);
}

static inline void dbi_writel(struct st_pcie *priv,
			      u32 val, unsigned addr)
{
	dbi_write(priv, (u32) val, addr, 4);
}

static inline void pcie_cap_writew(struct st_pcie *priv,
				   u16 val, unsigned cap)
{
	dbi_writew(priv, val, CFG_PCIE_CAP + cap);
}

static inline u16 pcie_cap_readw(struct st_pcie *priv,
				 unsigned cap)
{
	return dbi_readw(priv, CFG_PCIE_CAP + cap);
}

#define LINK_LOOP_DELAY_MS 1
 
#define LINK_WAIT_MS 120
#define LINK_LOOP_COUNT (LINK_WAIT_MS / LINK_LOOP_DELAY_MS)

static int link_up(struct st_pcie *priv)
{
	u32 status;
	int link_up;
	int count = 0;

	do {
		 
		status = dbi_readl(priv, PORT_LOGIC_DEBUG_REG_0);
		status &= DEBUG_REG_0_LTSSM_MASK;

		link_up = (status == S_L0) || (status == S_L0S) ||
			  (status == S_L1_IDLE);

		if (!link_up)
			mdelay(LINK_LOOP_DELAY_MS);

	} while (!link_up  && ++count < LINK_LOOP_COUNT);

	return link_up;
}

static atomic_t abort_flag;

static unsigned long abort_start, abort_end;

static int st_pcie_abort(unsigned long addr, unsigned int fsr,
			  struct pt_regs *regs)
{
	unsigned long pc = regs->ARM_pc;

	if (pc < abort_start || pc >= abort_end)
		return 1;

	if (!((fsr & (1 << 10)) && ((fsr & 0xf) == 0x6)))
		return 1;

	atomic_set(&abort_flag, 1);

	mb();

	return 0;
}

static inline u32 bdf_num(int bus, int devfn, int is_root_bus)
{
	return ((bus << 8) | devfn) << (is_root_bus ? 0 : 16);
}

static inline unsigned config_addr(int where, int is_root_bus)
{
	return (where & ~3) | (!is_root_bus << 12);
}
#ifdef MY_DEF_HERE
static int st_pcie_valid_config(struct st_pcie *pp,
				struct pci_bus *bus, int dev)
{
	 
	if (bus->number != pp->root_bus_nr) {
		if (!link_up(pp))
			return 0;
	}

	if (bus->number == pp->root_bus_nr && dev > 0)
		return 0;

	if (bus->primary == pp->root_bus_nr && dev > 0)
		return 0;

	return 1;
}
#endif  

static int st_pcie_config_read(struct pci_bus *bus, unsigned int devfn,
				int where, int size, u32 *val)
{
	u32 bdf;
	u32 data;
#ifdef MY_DEF_HERE
#else  
	int slot = PCI_SLOT(devfn);
#endif  
	unsigned long flags;
	struct st_pcie *priv = sys_to_pcie(bus->sysdata);
	int is_root_bus = pci_is_root_bus(bus);
	int retry_count = 0;
	int ret;
#ifdef MY_DEF_HERE
	if (st_pcie_valid_config(priv, bus, PCI_SLOT(devfn)) == 0) {
		*val = 0xffffffff;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

	if (is_root_bus) {
		 
		data = dbi_readl(priv, (where & ~0x3));
		*val = shift_data_read(where, size, data);
		return PCIBIOS_SUCCESSFUL;
	}

#else  
#ifdef CONFIG_STM_PCIE_TRACKER_BUG
	void __iomem *raw_config_area = priv->config_area;
	raw_config_area = __stm_unfrob(priv->config_area);
#endif
	 
	if (!priv || (is_root_bus && slot != 1) || !link_up(priv)) {
		*val = ~0;
		return PCIBIOS_DEVICE_NOT_FOUND;
	}

#endif  
	bdf = bdf_num(bus->number, devfn, is_root_bus);

retry:
	 
	spin_lock_irqsave(&priv->abort_lock, flags);

	dbi_writel(priv, bdf, FUNC0_BDF_NUM);
	 
	dbi_readl(priv, FUNC0_BDF_NUM);

	atomic_set(&abort_flag, 0);
	ret = PCIBIOS_SUCCESSFUL;

#ifdef MY_DEF_HERE
#else  
#ifdef CONFIG_STM_PCIE_TRACKER_BUG
	 
	spin_lock(&stm_pcie_io_spinlock);
#endif
#endif  

	abort_start = (unsigned long)&&config_read_start;
	abort_end = (unsigned long)&&config_read_end;

config_read_start:
#ifdef MY_DEF_HERE
	 
	data = readl(priv->config_area + config_addr(where,
		     bus->parent->number == priv->root_bus_nr));
#else  
#ifndef CONFIG_STM_PCIE_TRACKER_BUG
	 
	data = readl(priv->config_area + config_addr(where, is_root_bus));
#else  

	data = __raw_readl(raw_config_area +
			   config_addr(where, is_root_bus));

#endif  
#endif  

	mb();  

config_read_end:

#ifdef MY_DEF_HERE
#else  
#ifdef CONFIG_STM_PCIE_TRACKER_BUG
	spin_unlock(&stm_pcie_io_spinlock);
#endif
#endif  
	 
	if (atomic_read(&abort_flag)) {
		ret = PCIBIOS_DEVICE_NOT_FOUND;
		data = ~0;
	}

	spin_unlock_irqrestore(&priv->abort_lock, flags);

	if (((where&~3) == 0) && devfn == 0 && (data == 0 || data == ~0)) {
		if (retry_count++ < 1000) {
			mdelay(1);
			goto retry;
		} else {
			*val = ~0;
			return PCIBIOS_DEVICE_NOT_FOUND;
		}
	}

	*val = shift_data_read(where, size, data);

	return ret;
}

static int st_pcie_config_write(struct pci_bus *bus, unsigned int devfn,
				 int where, int size, u32 val)
{
	unsigned long flags;
	u32 bdf;
	u32 data = 0;
#ifdef MY_DEF_HERE
#else  
	int slot = PCI_SLOT(devfn);
#endif  
	struct st_pcie *priv = sys_to_pcie(bus->sysdata);
	int is_root_bus = pci_is_root_bus(bus);
	int ret;
#ifdef MY_DEF_HERE
#else  
#ifdef CONFIG_STM_PCIE_TRACKER_BUG
	void __iomem *raw_config_area = priv->config_area;
	raw_config_area = __stm_unfrob(priv->config_area);
#endif
#endif  

#ifdef MY_DEF_HERE
	if (st_pcie_valid_config(priv, bus, PCI_SLOT(devfn)) == 0)
#else  
	if (!priv || (is_root_bus && slot != 1) || !link_up(priv))
#endif  
		return PCIBIOS_DEVICE_NOT_FOUND;
#ifdef MY_DEF_HERE
	if (is_root_bus) {
		 
		data = dbi_readl(priv, (where & ~0x3));
		data = shift_data_write(where, size, val, data);
		dbi_writel(priv, data, (where & ~0x3));
		return PCIBIOS_SUCCESSFUL;
	}
#endif  

	bdf = bdf_num(bus->number, devfn, is_root_bus);

	spin_lock_irqsave(&priv->abort_lock, flags);

	dbi_writel(priv, bdf, FUNC0_BDF_NUM);
	dbi_readl(priv, FUNC0_BDF_NUM);

	atomic_set(&abort_flag, 0);

#ifdef MY_DEF_HERE
#else  
#ifdef CONFIG_STM_PCIE_TRACKER_BUG
	 
	spin_lock(&stm_pcie_io_spinlock);
#endif
#endif  

	abort_start = (unsigned long)&&config_write_start;
	abort_end = (unsigned long)&&config_write_end;

config_write_start:

#ifdef MY_DEF_HERE
	 
	if (size != 4)
		data = readl(priv->config_area + config_addr(where,
			     bus->parent->number == priv->root_bus_nr));
#else  
#ifndef CONFIG_STM_PCIE_TRACKER_BUG
	 
	if (size != 4)
		data = readl(priv->config_area +
			     config_addr(where, is_root_bus));
#else
	 
	if (size != 4)
		data = __raw_readl(raw_config_area +
			     config_addr(where, is_root_bus));
#endif
#endif  
	mb();

	data = shift_data_write(where, size, val, data);

#ifdef MY_DEF_HERE
	writel(data, priv->config_area + config_addr(where,
	       bus->parent->number == priv->root_bus_nr));
#else  
#ifndef CONFIG_STM_PCIE_TRACKER_BUG
	writel(data, priv->config_area + config_addr(where, is_root_bus));
#else
	__raw_writel(data, raw_config_area + config_addr(where, is_root_bus));
#endif
#endif  

	mb();

config_write_end:

#ifdef MY_DEF_HERE
#else  
#ifdef CONFIG_STM_PCIE_TRACKER_BUG
	spin_unlock(&stm_pcie_io_spinlock);
#endif
#endif  
	ret = atomic_read(&abort_flag) ? PCIBIOS_DEVICE_NOT_FOUND
				       : PCIBIOS_SUCCESSFUL;

	spin_unlock_irqrestore(&priv->abort_lock, flags);

	return ret;
}

static struct pci_ops st_pcie_config_ops = {
	.read = st_pcie_config_read,
	.write = st_pcie_config_write,
};

static void st_pcie_board_reset(struct st_pcie *pcie)
{
	if (!gpio_is_valid(pcie->reset_gpio))
		return;

	if (gpio_direction_output(pcie->reset_gpio, 0)) {
		dev_err(pcie->dev, "Cannot set PERST# (gpio %u) to output\n",
			pcie->reset_gpio);
		return;
	}

	usleep_range(1000, 2000);
	gpio_direction_output(pcie->reset_gpio, 1);

	usleep_range(100000, 150000);
}

static int st_pcie_hw_setup(struct st_pcie *priv,
				       phys_addr_t lmi_window_start,
				       unsigned long lmi_window_size,
				       phys_addr_t config_window_start)
{
	int err;

	err = priv->data->disable_ltssm(priv);
	if (err)
		return err;

	dbi_writew(priv, PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER, PCI_COMMAND);

	dbi_writel(priv, config_window_start, CFG_BASE_ADDRESS);

	dbi_writel(priv, lmi_window_start, IN0_MEM_ADDR_START);
	dbi_writel(priv, lmi_window_start + lmi_window_size - 1,
		   IN0_MEM_ADDR_LIMIT);

	dbi_writel(priv, ~0, IN1_MEM_ADDR_START);
	dbi_writel(priv, 0, IN1_MEM_ADDR_LIMIT);

	dbi_writel(priv, RC_PASS_ADDR_RANGE, TRANSLATION_CONTROL);

	if (priv->ahb) {
		 
		writel(priv->ahb_val, priv->ahb + 4);
		 
		writel(0x2, priv->ahb);
	}

	st_pcie_board_reset(priv);

#ifdef MY_DEF_HERE
	err = priv->data->enable_ltssm(priv);
	if (err)
		return err;

	dbi_writew(priv, PCI_CLASS_BRIDGE_PCI, PCI_CLASS_DEVICE);

	return err;
#else  
	return priv->data->enable_ltssm(priv);
#endif  
}

static int remap_named_resource(struct platform_device *pdev,
					  const char *name,
					  void __iomem **io_ptr)
{
	struct resource *res;
	resource_size_t size;
	void __iomem *p;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (!res)
		return -ENXIO;

	size = resource_size(res);

	if (!devm_request_mem_region(&pdev->dev,
				     res->start, size, name))
		return -EBUSY;

	p = devm_ioremap_nocache(&pdev->dev, res->start, size);
	if (!p)
		return -ENOMEM;

	*io_ptr = p;

	return 0;
}

static irqreturn_t st_pcie_sys_err(int irq, void *dev_data)
{
	panic("PCI express serious error raised\n");
	return IRQ_HANDLED;
}

static int st_pcie_map_irq(const struct pci_dev *dev, u8 slot, u8 pin)
{
	struct st_pcie *pcie = sys_to_pcie(dev->bus->sysdata);
#ifdef MY_DEF_HERE
	int i = (slot + pin - 1) % 4;
#else  
	int i = (slot - 1 + pin - 1) % 4;
#endif  

	if (i >= pcie->irq_lines)
		i = 0;

	dev_info(pcie->dev,
		"PCIe map irq: %04d:%02x:%02x.%02x slot %d, pin %d, irq: %d\n",
		pci_domain_nr(dev->bus), dev->bus->number, PCI_SLOT(dev->devfn),
		PCI_FUNC(dev->devfn), slot, pin, pcie->irq[i]);

	return pcie->irq[i];
}

static void st_pcie_add_bus(struct pci_bus *bus)
{
	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		struct st_pcie *pcie = sys_to_pcie(bus->sysdata);

		bus->msi = &pcie->msi->chip;
	}
}

static int st_pcie_setup(int nr, struct pci_sys_data *sys)
{
	struct st_pcie *pcie = sys_to_pcie(sys);

	if (pcie->io.start)
		pci_add_resource(&sys->resources, &pcie->io);

	pci_add_resource(&sys->resources, &pcie->mem);
	pci_add_resource(&sys->resources, &pcie->prefetch);
	pci_add_resource(&sys->resources, &pcie->busn);

	return 1;
}

static struct pci_bus *st_pcie_scan(int nr, struct pci_sys_data *sys)
{
	struct st_pcie *pcie = sys_to_pcie(sys);

#ifdef MY_DEF_HERE
	pcie->root_bus_nr = sys->busnr;
#endif  
	return pci_scan_root_bus(pcie->dev, sys->busnr, &st_pcie_config_ops,
				 sys, &sys->resources);
}

static irqreturn_t st_msi_irq_demux(int mux_irq, void *data)
{
	struct st_pcie *pcie = data;
	struct st_msi *msi = pcie->msi;
	int ep, processed = 0;
	unsigned long status;
	u32 mask;
	u32 offset;
	unsigned int index;
	unsigned int irq;

	for (ep = 0; ep < MSI_NUM_ENDPOINTS; ep++) {
		status = readl_relaxed(msi->regs + MSI_INTERRUPT_STATUS(ep));
		mask = readl_relaxed(msi->regs + MSI_INTERRUPT_MASK(ep));
		status &= ~mask;

		while (status) {
			offset = find_first_bit(&status, 32);
			index = ep * 32 + offset;

			writel_relaxed(1 << offset,
					msi->regs + MSI_INTERRUPT_STATUS(ep));
			readl_relaxed(msi->regs + MSI_INTERRUPT_STATUS(ep));

			irq = irq_find_mapping(msi->domain, index);
			if (irq) {
				if (test_bit(index, msi->used))
					generic_handle_irq(irq);
				else
					dev_info(pcie->dev, "unhandled MSI\n");
			} else {
				 
				dev_info(pcie->dev, "unexpected MSI\n");
			}

			status = readl_relaxed(
					msi->regs + MSI_INTERRUPT_STATUS(ep));
			mask = readl_relaxed(
					msi->regs + MSI_INTERRUPT_MASK(ep));
			status &= ~mask;

			processed++;
		};
	}

	return processed > 0 ? IRQ_HANDLED : IRQ_NONE;
}

static inline void set_msi_bit(struct irq_data *data, unsigned int reg_base)
{
	struct st_msi *msi = irq_data_get_irq_chip_data(data);
	int ep = data->hwirq / 32;
	unsigned int offset = data->hwirq & 0x1F;
	int val;
	unsigned long flags;

	spin_lock_irqsave(&msi->reg_lock, flags);

	val = readl_relaxed(msi->regs + reg_base + MSI_OFFSET_REG(ep));
	val |= 1 << offset;
	writel_relaxed(val, msi->regs + reg_base + MSI_OFFSET_REG(ep));

	readl_relaxed(msi->regs + reg_base + MSI_OFFSET_REG(ep));

	spin_unlock_irqrestore(&msi->reg_lock, flags);
}

static inline void clear_msi_bit(struct irq_data *data, unsigned int reg_base)
{
	struct st_msi *msi = irq_data_get_irq_chip_data(data);
	int ep = data->hwirq / 32;
	unsigned int offset = data->hwirq & 0x1F;
	int val;
	unsigned long flags;

	spin_lock_irqsave(&msi->reg_lock, flags);

	val = readl_relaxed(msi->regs + reg_base + MSI_OFFSET_REG(ep));
	val &= ~(1 << offset);
	writel_relaxed(val, msi->regs + reg_base + MSI_OFFSET_REG(ep));

	readl_relaxed(msi->regs + reg_base + MSI_OFFSET_REG(ep));

	spin_unlock_irqrestore(&msi->reg_lock, flags);
}

static void st_enable_msi_irq(struct irq_data *data)
{
	set_msi_bit(data, MSI_INTERRUPT_ENABLE(0));
	 
	unmask_msi_irq(data);
}

static void st_disable_msi_irq(struct irq_data *data)
{
	 
	mask_msi_irq(data);

	clear_msi_bit(data, MSI_INTERRUPT_ENABLE(0));
}

static void st_mask_msi_irq(struct irq_data *data)
{
	set_msi_bit(data, MSI_INTERRUPT_MASK(0));
}

static void st_unmask_msi_irq(struct irq_data *data)
{
	clear_msi_bit(data, MSI_INTERRUPT_MASK(0));
}

static struct irq_chip st_msi_irq_chip = {
	.name = "ST PCIe MSI",
	.irq_enable = st_enable_msi_irq,
	.irq_disable = st_disable_msi_irq,
	.irq_mask = st_mask_msi_irq,
	.irq_unmask = st_unmask_msi_irq,
};

static void st_msi_init_one(struct st_msi *msi)
{
	int ep;

	writel(0, msi->regs + MSI_UPPER_ADDRESS);
	writel(virt_to_phys(msi), msi->regs + MSI_ADDRESS);

	for (ep = 0; ep < MSI_NUM_ENDPOINTS; ep++) {
		writel(0, msi->regs + MSI_INTERRUPT_ENABLE(ep));
		writel(0, msi->regs + MSI_INTERRUPT_MASK(ep));
		writel(~0, msi->regs + MSI_INTERRUPT_STATUS(ep));
	}
}

static int st_msi_map(struct irq_domain *domain, unsigned int irq,
			 irq_hw_number_t hwirq)
{
	irq_set_chip_and_handler(irq, &st_msi_irq_chip, handle_level_irq);
	irq_set_chip_data(irq, domain->host_data);
	set_irq_flags(irq, IRQF_VALID);

	return 0;
}

static const struct irq_domain_ops msi_domain_ops = {
	.map = st_msi_map,
};

static int st_msi_alloc(struct st_msi *chip)
{
	int msi;

	mutex_lock(&chip->lock);

	msi = find_first_zero_bit(chip->used, INT_PCI_MSI_NR);
	if (msi < INT_PCI_MSI_NR)
		set_bit(msi, chip->used);
	else
		msi = -ENOSPC;

	mutex_unlock(&chip->lock);

	return msi;
}

static void st_msi_free(struct st_msi *chip, unsigned long irq)
{
	struct device *dev = chip->chip.dev;

	mutex_lock(&chip->lock);

	if (!test_bit(irq, chip->used))
		dev_err(dev, "trying to free unused MSI#%lu\n", irq);
	else
		clear_bit(irq, chip->used);

	mutex_unlock(&chip->lock);
}

static int st_setup_msi_irq(struct msi_chip *chip, struct pci_dev *pdev,
			       struct msi_desc *desc)
{
	struct st_msi *msi = to_st_msi(chip);
	struct msi_msg msg;
	unsigned int irq;
	int hwirq;

	hwirq = st_msi_alloc(msi);
	if (hwirq < 0)
		return hwirq;

	irq = irq_create_mapping(msi->domain, hwirq);
	if (!irq)
		return -EINVAL;

	irq_set_msi_desc(irq, desc);

	msg.data = hwirq;
	msg.address_hi = 0;
	msg.address_lo = virt_to_phys(msi);

	write_msi_msg(irq, &msg);

	return 0;
}

static void st_teardown_msi_irq(struct msi_chip *chip, unsigned int irq)
{
	struct st_msi *msi = to_st_msi(chip);
	struct irq_data *d = irq_get_irq_data(irq);

	st_msi_free(msi, d->hwirq);
}

static int st_pcie_enable_msi(struct st_pcie *pcie)
{
	int err;
	struct platform_device *pdev = to_platform_device(pcie->dev);
	struct st_msi *msi;

	msi = devm_kzalloc(&pdev->dev, sizeof(*msi), GFP_KERNEL);
	if (!msi)
		return -ENOMEM;

	pcie->msi = msi;

	spin_lock_init(&msi->reg_lock);
	mutex_init(&msi->lock);

	msi->regs = pcie->cntrl;

	msi->chip.dev = pcie->dev;
	msi->chip.setup_irq = st_setup_msi_irq;
	msi->chip.teardown_irq = st_teardown_msi_irq;

	msi->domain = irq_domain_add_linear(pcie->dev->of_node, INT_PCI_MSI_NR,
					    &msi_domain_ops, msi);
	if (!msi->domain) {
		dev_err(&pdev->dev, "failed to create IRQ domain\n");
		return -ENOMEM;
	}

	err = platform_get_irq_byname(pdev, "msi");
	if (err < 0) {
		dev_err(&pdev->dev, "failed to get IRQ: %d\n", err);
		goto err;
	}

	msi->mux_irq = err;

	st_msi_init_one(msi);

	err = request_irq(msi->mux_irq, st_msi_irq_demux, 0,
			  st_msi_irq_chip.name, pcie);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to request IRQ: %d\n", err);
		goto err;
	}

	return 0;

err:
	irq_domain_remove(msi->domain);
	return err;
}

static int st_pcie_disable_msi(struct st_pcie *pcie)
{
	int i, irq;
	struct st_msi *msi = pcie->msi;

	st_msi_init_one(msi);

	if (msi->mux_irq > 0)
		free_irq(msi->mux_irq, pcie);

	for (i = 0; i < INT_PCI_MSI_NR; i++) {
		irq = irq_find_mapping(msi->domain, i);
		if (irq > 0)
			irq_dispose_mapping(irq);
	}

	irq_domain_remove(msi->domain);

	return 0;
}

static int st_pcie_parse_dt(struct st_pcie *pcie)
{
	struct device_node *np = pcie->dev->of_node;
	struct of_pci_range_parser parser;
	struct of_pci_range range;
	struct resource res;
	int err;

	if (of_pci_range_parser_init(&parser, np)) {
		dev_err(pcie->dev, "missing \"ranges\" property\n");
		return -EINVAL;
	}

	for_each_of_pci_range(&parser, &range) {
		of_pci_range_to_resource(&range, np, &res);

		dev_dbg(pcie->dev, "start 0x%08x end 0x%08x flags 0x%08lx\n",
				res.start, res.end, res.flags);

		switch (res.flags & IORESOURCE_TYPE_BITS) {
		case IORESOURCE_IO:
			memcpy(&pcie->io, &res, sizeof(res));
			pcie->io.name = "I/O";
			break;

		case IORESOURCE_MEM:
			if (res.flags & IORESOURCE_PREFETCH) {
				memcpy(&pcie->prefetch, &res, sizeof(res));
				pcie->prefetch.name = "PREFETCH";
			} else {
				memcpy(&pcie->mem, &res, sizeof(res));
				pcie->mem.name = "MEM";
			}
			break;
		}
	}

	err = of_pci_parse_bus_range(np, &pcie->busn);
	if (err < 0) {
		dev_err(pcie->dev, "failed to parse ranges property: %d\n",
			err);
		pcie->busn.name = np->name;
		pcie->busn.start = 0;
		pcie->busn.end = 0xff;
		pcie->busn.flags = IORESOURCE_BUS;
	}

	return 0;
}

static int st_pcie_enable(struct st_pcie *pcie)
{
	struct hw_pci hw;

	memset(&hw, 0, sizeof(hw));

	hw.nr_controllers = 1;
	hw.private_data = (void **)&pcie;
	hw.setup = st_pcie_setup;
	hw.map_irq = st_pcie_map_irq;
	hw.add_bus = st_pcie_add_bus;
	hw.scan = st_pcie_scan;
	hw.ops = &st_pcie_config_ops;

	pci_common_init_dev(pcie->dev, &hw);

	return 0;
}

static int st_pcie_init(struct st_pcie *pcie)
{
	int ret;

	ret = reset_control_deassert(pcie->pwr);
	if (ret) {
		dev_err(pcie->dev, "unable to bring out of powerdown\n");
		return ret;
	}

	ret = reset_control_deassert(pcie->rst);
	if (ret) {
		dev_err(pcie->dev, "unable to bring out of softreset\n");
		return ret;
	}

	ret = regmap_write(pcie->regmap, pcie->syscfg0, PCIE_DEVICE_TYPE);
	if (ret < 0) {
		dev_err(pcie->dev, "unable to set device type\n");
		return ret;
	}

	usleep_range(1000, 2000);
	return ret;
}

static int stih407_pcie_enable_ltssm(struct st_pcie *pcie)
{
	if (!pcie->syscfg1)
		return -EINVAL;

	return regmap_update_bits(pcie->regmap, pcie->syscfg1,
			PCIE_APP_LTSSM_ENABLE,	PCIE_APP_LTSSM_ENABLE);
}

static int stih407_pcie_disable_ltssm(struct st_pcie *pcie)
{
	if (!pcie->syscfg1)
		return -EINVAL;

	return regmap_update_bits(pcie->regmap, pcie->syscfg1,
			PCIE_APP_LTSSM_ENABLE, 0);
}

static struct st_pcie_ops stih407_pcie_ops = {
	.init = st_pcie_init,
	.enable_ltssm = stih407_pcie_enable_ltssm,
	.disable_ltssm = stih407_pcie_disable_ltssm,
};

static int st_pcie_enable_ltssm(struct st_pcie *pcie)
{
	return regmap_write(pcie->regmap, pcie->syscfg0,
			PCIE_DEFAULT_VAL | PCIE_APP_LTSSM_ENABLE);
}

static int st_pcie_disable_ltssm(struct st_pcie *pcie)
{
	return regmap_write(pcie->regmap, pcie->syscfg0, PCIE_DEFAULT_VAL);
}

static struct st_pcie_ops st_pcie_ops = {
	.init = st_pcie_init,
	.enable_ltssm = st_pcie_enable_ltssm,
	.disable_ltssm = st_pcie_disable_ltssm,
};

static struct st_pcie_ops stid127_pcie_ops = {
	.init = st_pcie_init,
	.enable_ltssm = st_pcie_enable_ltssm,
	.disable_ltssm = st_pcie_disable_ltssm,
	.phy_auto = true,
};

static const struct of_device_id st_pcie_of_match[] = {
	{ .compatible = "st,stih407-pcie", .data = (void *)&stih407_pcie_ops},
	{ .compatible = "st,stih416-pcie", .data = (void *)&st_pcie_ops},
	{ .compatible = "st,stid127-pcie", .data = (void *)&stid127_pcie_ops},
	{ },
};

#ifdef CONFIG_PM
static int st_pcie_suspend(struct device *pcie_dev)
{
	struct st_pcie *pcie = dev_get_drvdata(pcie_dev);

	if (!pcie->data->phy_auto)
		phy_exit(pcie->phy);

	return 0;
}

static int st_pcie_resume(struct device *pcie_dev)
{
	struct st_pcie *pcie = dev_get_drvdata(pcie_dev);
	int ret;

	ret = pcie->data->init(pcie);
	if (ret) {
		pr_err("%s: Error on pcie init\n", __func__);
		return ret;
	}

	ret = pcie->data->disable_ltssm(pcie);
	if (ret) {
		pr_err("%s: Error on pcie disable_ltssm\n", __func__);
		return ret;
	}

	if (!pcie->data->phy_auto)
		phy_init(pcie->phy);

	st_pcie_hw_setup(pcie,
		pcie->lmi->start,
		resource_size(pcie->lmi),
		pcie->config_window_start);

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		 
		st_msi_init_one(pcie->msi);
	}

	return 0;
}

const struct dev_pm_ops st_pcie_pm_ops = {
	.suspend_noirq = st_pcie_suspend,
	.resume_noirq = st_pcie_resume,
};
#define ST_PCIE_PM	(&st_pcie_pm_ops)
#else
#define ST_PCIE_PM	NULL
#endif

static int __init st_pcie_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct st_pcie *pcie;
	struct device_node *np = pdev->dev.of_node;
	int err, serr_irq;

	pcie = devm_kzalloc(&pdev->dev, sizeof(*pcie), GFP_KERNEL);
	if (!pcie)
		return -ENOMEM;

	memset(pcie, 0, sizeof(*pcie));

	pcie->dev = &pdev->dev;

	spin_lock_init(&pcie->abort_lock);

	err = st_pcie_parse_dt(pcie);
	if (err < 0)
		return err;

	pcie->regmap = syscon_regmap_lookup_by_phandle(np, "st,syscfg");
	if (IS_ERR(pcie->regmap)) {
		dev_err(pcie->dev, "No syscfg phandle specified\n");
		return PTR_ERR(pcie->regmap);
	}

	err = remap_named_resource(pdev, "pcie cntrl", &pcie->cntrl);
	if (err)
		return err;

	err = remap_named_resource(pdev, "pcie ahb", &pcie->ahb);
	if (!err) {
		if (of_property_read_u32(np, "st,ahb-fixup", &pcie->ahb_val)) {
			dev_err(pcie->dev, "missing ahb-fixup property\n");
			return -EINVAL;
		}
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pcie cs");
	if (!res)
		return -ENXIO;

	if ((resource_size(res) != CFG_REGION_SIZE) ||
	    (res->start & (CFG_REGION_SIZE - 1))) {
		dev_err(pcie->dev, "Invalid config space properties\n");
		return -EINVAL;
	}
	pcie->config_window_start = res->start;

	err = remap_named_resource(pdev, "pcie cs", &pcie->config_area);
	if (err)
		return err;

	pcie->lmi = platform_get_resource_byname(pdev, IORESOURCE_MEM,
			"mem-window");
	if (!pcie->lmi)
		return -ENXIO;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "syscfg0");
	if (!res)
		return -ENXIO;
	pcie->syscfg0 = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "syscfg1");
	if (res)
		pcie->syscfg1 = res->start;

	serr_irq = platform_get_irq_byname(pdev, "pcie syserr");
	if (serr_irq < 0) {
		dev_err(&pdev->dev, "failed to get syserr IRQ: %d\n",
				serr_irq);
		return serr_irq;
	}

	err = devm_request_irq(&pdev->dev, serr_irq, st_pcie_sys_err,
			      IRQF_DISABLED, "pcie syserr", pcie);
	if (err) {
		dev_err(&pdev->dev, "failed to register syserr IRQ: %d\n",
				err);
		return err;
	}

	err = platform_get_irq_byname(pdev, "pcie inta");
	if (err < 0) {
		dev_err(&pdev->dev, "failed to get inta IRQ: %d\n", err);
		return err;
	}

	pcie->irq[pcie->irq_lines++] = err;

	err = platform_get_irq_byname(pdev, "pcie intb");
	if (err > 0)
		pcie->irq[pcie->irq_lines++] = err;

	err = platform_get_irq_byname(pdev, "pcie intc");
	if (err > 0)
		pcie->irq[pcie->irq_lines++] = err;

	err = platform_get_irq_byname(pdev, "pcie intd");
	if (err > 0)
		pcie->irq[pcie->irq_lines++] = err;

	pcie->data = of_match_node(st_pcie_of_match, np)->data;
	if (!pcie->data) {
		dev_err(&pdev->dev, "compatible data not provided\n");
		return -EINVAL;
	}

	pcie->pwr = devm_reset_control_get(&pdev->dev, "power");
	if (IS_ERR(pcie->pwr)) {
		dev_err(&pdev->dev, "power reset control not defined\n");
		return -EINVAL;
	}

	pcie->rst = devm_reset_control_get(&pdev->dev, "softreset");
	if (IS_ERR(pcie->rst)) {
		dev_err(&pdev->dev, "Soft reset control not defined\n");
		return -EINVAL;
	}

	err = pcie->data->init(pcie);
	if (err)
		return err;

	err = pcie->data->disable_ltssm(pcie);
	if (err) {
		dev_err(&pdev->dev, "failed to disable ltssm\n");
		return err;
	}

	if (!pcie->data->phy_auto) {

		pcie->phy = devm_phy_get(&pdev->dev, "pcie_phy");
		if (IS_ERR(pcie->phy)) {
			dev_err(&pdev->dev, "no PHY configured\n");
			return PTR_ERR(pcie->phy);
		}

		err = phy_init(pcie->phy);
		if (err < 0) {
			dev_err(&pdev->dev, "Cannot init PHY: %d\n", err);
			return err;
		}
	}

	pcie->reset_gpio = of_get_named_gpio(np, "reset-gpio", 0);
	if (!gpio_is_valid(pcie->reset_gpio))
		dev_dbg(&pdev->dev, "No reset-gpio configured\n");
	else {
		err = devm_gpio_request(&pdev->dev,
				pcie->reset_gpio,
				"PCIe reset");
		if (err) {
			dev_err(&pdev->dev, "Cannot request reset-gpio %d\n",
				pcie->reset_gpio);
			return err;
		}
	}

	err = st_pcie_hw_setup(pcie, pcie->lmi->start,
				resource_size(pcie->lmi),
				pcie->config_window_start);
	if (err) {
		dev_err(&pdev->dev, "Error while doing hw setup: %d\n", err);
		goto disable_phy;
	}

	if (IS_ENABLED(CONFIG_PCI_MSI)) {
		 
		err = st_pcie_enable_msi(pcie);
		if (err)
			goto disable_link;
	}

	if (IS_ENABLED(CONFIG_ARM)) {
		 
		hook_fault_code(16+6, st_pcie_abort, SIGBUS, 0,
			"imprecise external abort");
	}

	err = st_pcie_enable(pcie);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to enable PCIe ports: %d\n", err);
		goto disable_msi;
	}

	platform_set_drvdata(pdev, pcie);

	dev_info(&pdev->dev, "Initialized\n");
	return 0;

disable_msi:
	if (IS_ENABLED(CONFIG_PCI_MSI))
		st_pcie_disable_msi(pcie);
disable_link:
	pcie->data->disable_ltssm(pcie);
disable_phy:
	if (!pcie->data->phy_auto)
		phy_exit(pcie->phy);

	return err;
}

MODULE_DEVICE_TABLE(of, st_pcie_of_match);

static struct platform_driver st_pcie_driver = {
	.driver = {
		.name = "st-pcie",
		.owner = THIS_MODULE,
		.of_match_table = st_pcie_of_match,
		.pm = ST_PCIE_PM,
	},
};

static int __init pcie_init(void)
{
	return platform_driver_probe(&st_pcie_driver, st_pcie_probe);
}
device_initcall(pcie_init);

MODULE_AUTHOR("Fabrice Gasnier <fabrice.gasnier@st.com>");
MODULE_DESCRIPTION("PCI express Driver for ST SoCs");
MODULE_LICENSE("GPLv2");

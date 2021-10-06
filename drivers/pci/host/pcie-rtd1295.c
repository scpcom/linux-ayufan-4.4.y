#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/of_address.h>
#include <linux/of_pci.h>
#include <linux/of_platform.h>
#include <linux/of_irq.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/resource.h>
#include <linux/signal.h>
#include <linux/types.h>
#include <linux/reset-helper.h>
#include <linux/reset.h>
#include <linux/suspend.h>
#include <linux/kthread.h>

#include <soc/realtek/rtd129x_cpu.h>
#include "pcie-rtd1295.h"

static void __iomem	*PCIE_CTRL_BASE;
static void __iomem	*PCIE_CFG_BASE;
static void __iomem	*SYSTEM_BASE1;
static void __iomem	*SYSTEM_BASE2;
static void __iomem	*EMMC_MUXPAD;

static u32 pcie0_gpio_reset = 0;
static u32 pcie0_gpio_iso = 0;

static bool cfg_direct_access = false;

static int rtd129x_cpu_id;
static int rtd129x_cpu_revision;

static struct clk *pcie0_clk;
static struct reset_control *rstn_pcie0_stitch;
static struct reset_control *rstn_pcie0;
static struct reset_control *rstn_pcie0_core;
static struct reset_control *rstn_pcie0_power;
static struct reset_control *rstn_pcie0_nonstich;
static struct reset_control *rstn_pcie0_phy;
static struct reset_control *rstn_pcie0_phy_mdio;

static struct pci_bus *bus;

static void rtk_pci_ctrl_write(unsigned long addr, unsigned int val)
{
	writel(val, addr + PCIE_CTRL_BASE);
}

static unsigned int rtk_pci_ctrl_read(unsigned long addr)
{
	unsigned int val = readl(addr + PCIE_CTRL_BASE);
	return val;
}

static void rtk_pci_direct_cfg_write(unsigned long addr, unsigned int val)
{
	writel(val, addr + PCIE_CFG_BASE);
}

static void rtk_pci_direct_cfg_write_word(unsigned long addr, u16 val)
{
	writew(val, addr + PCIE_CFG_BASE);
}

static void rtk_pci_direct_cfg_write_byte(unsigned long addr, u8 val)
{
	writeb(val, addr + PCIE_CFG_BASE);
}

static unsigned int rtk_pci_direct_cfg_read(unsigned long addr)
{
	unsigned int val = readl(addr + PCIE_CFG_BASE);
	return val;
}

static u16 rtk_pci_direct_cfg_read_word(unsigned long addr)
{
	u16 val = readw(addr + PCIE_CFG_BASE);
	return val;
}

static u8 rtk_pci_direct_cfg_read_byte(unsigned long addr)
{
	u8 val = readb(addr + PCIE_CFG_BASE);
	return val;
}

static int _indirect_cfg_write(unsigned long addr, unsigned long data, unsigned char size)
{
	unsigned long status;
	unsigned char mask;
	int try_count = 1000;

//	printk(KERN_ALERT "_indirect_cfg_write: addr(%x): %x size %d\n", addr, data, size);

	if (ADDR_TO_DEVICE_NO(addr) != 0)
		return PCIBIOS_DEVICE_NOT_FOUND;

	mask = _pci_byte_mask(addr, size);

	if (!mask)
		return PCIBIOS_SET_FAILED;

	data = (data << _pci_bit_shift(addr)) & _pci_bit_mask(mask);

	rtk_pci_ctrl_write(PCIE_INDIR_CTR, 0x12);
	rtk_pci_ctrl_write(PCIE_CFG_ST, CFG_ST_ERROR|CFG_ST_DONE);
	rtk_pci_ctrl_write(PCIE_CFG_ADDR, addr);
	rtk_pci_ctrl_write(PCIE_CFG_WDATA, data);

	if (size == 4)
		rtk_pci_ctrl_write(PCIE_CFG_EN, 0x1);
	else
		rtk_pci_ctrl_write(PCIE_CFG_EN, BYTE_CNT(mask) | BYTE_EN | WRRD_EN(1));

	rtk_pci_ctrl_write(PCIE_CFG_CT, GO_CT);

	do {
		status = rtk_pci_ctrl_read(PCIE_CFG_ST);
		udelay(50);
	} while (!(status & CFG_ST_DONE) && try_count--);

	if (try_count < 0) {
		PCI_CFG_WARNING("Write config data (%p) failed - timeout\n",
				(void *) addr);
		goto error_occur;
	}

	if (rtk_pci_ctrl_read(PCIE_CFG_ST) & CFG_ST_ERROR) {
		if (status & CFG_ST_DETEC_PAR_ERROR)
			PCI_CFG_WARNING("Write config data failed - PAR error detected\n");
		if (status & CFG_ST_SIGNAL_SYS_ERROR)
			PCI_CFG_WARNING("Write config data failed - system error\n");
		if (status & CFG_ST_REC_MASTER_ABORT)
			PCI_CFG_WARNING("Write config data failed - master abort\n");
		if (status & CFG_ST_REC_TARGET_ABORT)
			PCI_CFG_WARNING("Write config data failed - target abort\n");
		if (status & CFG_ST_SIG_TAR_ABORT)
			PCI_CFG_WARNING("Write config data failed - tar abort\n");

		goto error_occur;
	}

	rtk_pci_ctrl_write(PCIE_CFG_ST, CFG_ST_ERROR|CFG_ST_DONE);

	return PCIBIOS_SUCCESSFUL;

error_occur:

	rtk_pci_ctrl_write(PCIE_CFG_ST, CFG_ST_ERROR|CFG_ST_DONE);

	return PCIBIOS_SET_FAILED;
}

static int _indirect_cfg_read(unsigned long addr, u32 *pdata, unsigned char size)
{
	unsigned long status;
	unsigned char mask;
	int try_count = 20000;

	if (ADDR_TO_DEVICE_NO(addr) != 0)
		return PCIBIOS_DEVICE_NOT_FOUND;

	mask = _pci_byte_mask(addr, size);

	if (!mask)
		return PCIBIOS_SET_FAILED;

	rtk_pci_ctrl_write(PCIE_INDIR_CTR, 0x10);
	rtk_pci_ctrl_write(PCIE_CFG_ST, 0x3);
	rtk_pci_ctrl_write(PCIE_CFG_ADDR, (addr & ~0x3));
	rtk_pci_ctrl_write(PCIE_CFG_EN, BYTE_CNT(mask) | BYTE_EN | WRRD_EN(0));
	rtk_pci_ctrl_write(PCIE_CFG_CT, GO_CT);

	do {
		status = rtk_pci_ctrl_read(PCIE_CFG_ST);
		udelay(50);
	} while (!(status & CFG_ST_DONE) && try_count--);

	if (try_count < 0) {
		PCI_CFG_WARNING("Read config data (%p) failed - timeout\n", (void *) addr);
		goto error_occur;
	}

	if (rtk_pci_ctrl_read(PCIE_CFG_ST) & CFG_ST_ERROR) {
		if (status & CFG_ST_DETEC_PAR_ERROR)
			PCI_CFG_WARNING("Read config data failed - PAR error detected\n");
		if (status & CFG_ST_SIGNAL_SYS_ERROR)
			PCI_CFG_WARNING("Read config data failed - system error\n");
		if (status & CFG_ST_REC_MASTER_ABORT)
			PCI_CFG_WARNING("Read config data failed - master abort\n");
		if (status & CFG_ST_REC_TARGET_ABORT)
			PCI_CFG_WARNING("Read config data failed - target abort\n");
		if (status & CFG_ST_SIG_TAR_ABORT)
			PCI_CFG_WARNING("Read config data failed - tar abort\n");
		goto error_occur;
	}

	rtk_pci_ctrl_write(PCIE_CFG_ST, 0x3);

	*pdata = (rtk_pci_ctrl_read(PCIE_CFG_RDATA) & _pci_bit_mask(mask)) >> _pci_bit_shift(addr);
	return PCIBIOS_SUCCESSFUL;

error_occur:
	rtk_pci_ctrl_write(PCIE_CFG_ST, 0x3);
	return PCIBIOS_SET_FAILED;
}

static int rtk_pcie_rd_conf(struct pci_bus *bus, unsigned int devfn,
			    int reg, int size, u32 *pval)
{
	unsigned long address;
	int ret = PCIBIOS_DEVICE_NOT_FOUND;
	u32 val = 0;
	u8 retry = 5;

	if (rtd129x_cpu_revision == RTD129x_CHIP_REVISION_A00)
		udelay(200);

again:
	if (bus->number == 0 && PCI_SLOT(devfn) == 0 && PCI_FUNC(devfn) == 0) {
		if (cfg_direct_access) {
			rtk_pci_ctrl_write(0xC00, 0x40012);

			if (size == 1)
				*pval = rtk_pci_direct_cfg_read_byte(reg);
			else if (size == 2)
				*pval = rtk_pci_direct_cfg_read_word(reg);
			else if (size == 4)
				*pval = rtk_pci_direct_cfg_read(reg);

			rtk_pci_ctrl_write(0xC00, 0x1E0002);
			ret = PCIBIOS_SUCCESSFUL;

		} else {
			address = _pci_address_conversion(bus, devfn, reg);
			ret = _indirect_cfg_read(address, pval, size);
		}
	}

	val = rtk_pci_ctrl_read(0xC7C);
	if ((val & 0x1f) && retry) {
		rtk_pci_ctrl_write(0xC7C, val);
		retry--;
		dev_err(&bus->dev, "pcie1 dllp error occur = 0x%x\n", val);
		goto again;
	}

	//dev_info(&bus->dev, "rtk_pcie_rd_conf devfn = 0x%x, reg = 0x%x, *pval = 0x%x\n", devfn, reg, *pval);

	return ret;
}

static int rtk_pcie_wr_conf(struct pci_bus *bus, unsigned int devfn,
			    int reg, int size, u32 val)
{
	unsigned long address;
	int ret = PCIBIOS_DEVICE_NOT_FOUND;

	if (rtd129x_cpu_revision == RTD129x_CHIP_REVISION_A00)
		udelay(200);

	//dev_info(&bus->dev, "rtk_pcie_wr_conf devfn = 0x%x, reg = 0x%x, val = 0x%x\n", devfn, reg, val);

	if (bus->number == 0 && PCI_SLOT(devfn) == 0 && PCI_FUNC(devfn) == 0) {
		if ((reg == 0x10) || (reg == 0x18) || (reg == 0x20) || (reg == 0x24)) {
			if ((val & 0xc0000000) == 0xc0000000)
				rtk_pci_ctrl_write(0xD04, (val&0xfffffff0));
		}

		if (cfg_direct_access) {
			rtk_pci_ctrl_write(0xC00, 0x40012);

			if (size == 1)
				rtk_pci_direct_cfg_write_byte(reg, val);
			else if (size == 2)
				rtk_pci_direct_cfg_write_word(reg, val);
			else if (size == 4)
				rtk_pci_direct_cfg_write(reg, val);

			rtk_pci_ctrl_write(0xC00, 0x1E0002);
			ret = PCIBIOS_SUCCESSFUL;
		} else {
			address = _pci_address_conversion(bus, devfn, reg);
			ret = _indirect_cfg_write(address, val, size);
		}
	}

	return ret;
}

static struct pci_ops rtk_pcie_ops = {
	.read = rtk_pcie_rd_conf,
	.write = rtk_pcie_wr_conf,
};

static int rtk_pcie_mdio_chk(void){
#if 0
	int ret = 1;
	unsigned long timeout = 0;

	timeout = jiffies + msecs_to_jiffies(200);
	while(time_before(jiffies, timeout)){
		if((rtk_pci_ctrl_read(0xC1C) & 0x80) == 0x00000000){
			ret = 0;
			break;
		}
	}

	return ret;
#else

	msleep(1);
	return 0;
#endif

}

static int rtk_pcie_hw_initial(struct device *dev)
{
	bool pci_link_detected;
	int timeout = 0;
	int ret = 0;
	u32 val;

	if (rtd129x_cpu_revision == RTD129x_CHIP_REVISION_A00 ||
	   (rtd129x_cpu_id == RTK1296_CPU_ID && rtd129x_cpu_revision != RTD129x_CHIP_REVISION_B00)) {
		/* 0x9801C614[2:0] = 1 */
		val = readl(SYSTEM_BASE1 + 0x14);
		val &= (~0x7);
		val |= 0x1;
		writel(val, (SYSTEM_BASE1 + 0x14));

		/* 0x9801C600[3:0] = 0 --PCIE */
		val = readl(SYSTEM_BASE1);
		val &= (~0xf);
		writel(val, SYSTEM_BASE1);

		/* 0x9801C608[7:0] = 8’b01010000 */
		writeb(0x51, SYSTEM_BASE1 + 0x8);
	}

	reset_control_deassert(rstn_pcie0_stitch);
	reset_control_deassert(rstn_pcie0);
	reset_control_deassert(rstn_pcie0_core);
	reset_control_deassert(rstn_pcie0_power);
	reset_control_deassert(rstn_pcie0_nonstich);
	reset_control_deassert(rstn_pcie0_phy);
	reset_control_deassert(rstn_pcie0_phy_mdio);
	ret = clk_prepare_enable(pcie0_clk);
	if (ret) {
		dev_err(dev, "unable to enable pcie0_clk clock\n");
		clk_disable_unprepare(pcie0_clk);
		return -EINVAL;
	}

	/* #bit17 null for mdio rstN */
	rtk_pci_ctrl_write(0xC00, 0x00140010);

	/* #Write soft reset */
	rtk_pci_ctrl_write(0xC1C, 0x00000003);
	if(rtk_pcie_mdio_chk())
		return -1;

	rtk_pci_ctrl_write(0xC1C, 0x31810801);
	if(rtk_pcie_mdio_chk())
		return -1;

	rtk_pci_ctrl_write(0xC1C, 0xB2001101);
	if(rtk_pcie_mdio_chk())
		return -1;

	rtk_pci_ctrl_write(0xC1C, 0x00100001);
	if(rtk_pcie_mdio_chk())
		return -1;

	if (gpio_is_valid(pcie0_gpio_iso)) {
		ret = gpio_direction_output(pcie0_gpio_iso, 1);
		mdelay(100);
	}

	//Reset PCIE0 device
	if (gpio_is_valid(pcie0_gpio_reset))
		gpio_direction_output(pcie0_gpio_reset, 0);

	mdelay(100);

	if (gpio_is_valid(pcie0_gpio_reset))
		gpio_direction_output(pcie0_gpio_reset, 1);

	if (cfg_direct_access)
		rtk_pci_ctrl_write(0xC00, 0x00040012);
	else
		rtk_pci_ctrl_write(0xC00, 0x001E0022);

	mdelay(50);

	/* #Link initial setting */
	rtk_pci_ctrl_write(0x710, 0x00010120);

	do {
		pci_link_detected = (rtk_pci_ctrl_read(0xCB4) & 0x800);
		if (!pci_link_detected) {
			mdelay(TIMEOUT_RESOLUTION);
			timeout += TIMEOUT_RESOLUTION;
		}
	} while (!pci_link_detected && timeout < PCIE_CONNECT_TIMEOUT);

	if (pci_link_detected) {
		dev_err(dev, "PCIE device has link up in slot 1\n");
	} else {
		reset_control_assert(rstn_pcie0_stitch);
		reset_control_assert(rstn_pcie0);
		reset_control_assert(rstn_pcie0_core);
		reset_control_assert(rstn_pcie0_power);
		reset_control_assert(rstn_pcie0_nonstich);
		reset_control_assert(rstn_pcie0_phy);
		reset_control_assert(rstn_pcie0_phy_mdio);
		clk_disable_unprepare(pcie0_clk);

		if (gpio_is_valid(pcie0_gpio_reset))
			gpio_free(pcie0_gpio_reset);

		if (gpio_is_valid(pcie0_gpio_iso))
			gpio_free(pcie0_gpio_iso);
		dev_err(dev, "PCIE device has link down in slot 1\n");
		return -ENODEV;
	}

	writel(readl(EMMC_MUXPAD + 0x61C) | 0x00000004, EMMC_MUXPAD + 0x61C);

	/* #make sure DBI is working */
	rtk_pci_ctrl_write(0x04, 0x00000007);

	/* #Base */
	rtk_pci_ctrl_write(0xCFC, 0x9804f000);

	/* #Mask */
	rtk_pci_ctrl_write(0xD00, 0xFFFFF000);

	/* #translate for CFG R/W */
	rtk_pci_ctrl_write(0xD04, 0x40000000);

	/* prevent pcie hang if dllp error occur*/
	rtk_pci_ctrl_write(0xC78, 0x200001);

	/* set limit and base register */
	rtk_pci_ctrl_write(0x20, 0x0000FFF0);
	rtk_pci_ctrl_write(0x24, 0x0000FFF0);

	return 0;
}

static int rtk_pcie_probe(struct platform_device *pdev)
{
	int ret = 0;
	resource_size_t iobase = 0;
	LIST_HEAD(res);

	dev_info(&pdev->dev, "PCIE host driver initial begin.\n");

	PCIE_CTRL_BASE = of_iomap(pdev->dev.of_node, 0);
	if (!PCIE_CTRL_BASE) {
		dev_err(&pdev->dev, "can't request 'ctrl' address\n");
		return -EINVAL;
	}

	PCIE_CFG_BASE = of_iomap(pdev->dev.of_node, 1);
	if (!PCIE_CFG_BASE) {
		dev_err(&pdev->dev, "can't request 'cfg' address\n");
		return -EINVAL;
	}

	SYSTEM_BASE1 = of_iomap(pdev->dev.of_node, 2);
	if (!SYSTEM_BASE1) {
		dev_err(&pdev->dev, "can't request 'base1' address\n");
		return -EINVAL;
	}

	SYSTEM_BASE2 = of_iomap(pdev->dev.of_node, 3);
	if (!SYSTEM_BASE2) {
		dev_err(&pdev->dev, "can't request 'base2' address\n");
		return -EINVAL;
	}

	EMMC_MUXPAD = of_iomap(pdev->dev.of_node, 4);
	if (!EMMC_MUXPAD) {
		dev_err(&pdev->dev, "can't request 'EMMC_MUXPAD' address\n");
		return -EINVAL;
	}

	pcie0_gpio_reset = of_get_gpio_flags(pdev->dev.of_node, 0, NULL);
	if (gpio_is_valid(pcie0_gpio_reset)) {
		ret = gpio_request(pcie0_gpio_reset, "pcie0_gpio_reset");
		if (ret < 0)
			printk(KERN_ERR "%s: can't request gpio %d\n", __func__, pcie0_gpio_reset);
	} else
		printk(KERN_ERR "%s: gpio %d is not valid\n", __func__, pcie0_gpio_reset);

	pcie0_gpio_iso = of_get_named_gpio_flags(pdev->dev.of_node, "iso-gpios", 0, NULL);
	if (pcie0_gpio_iso) {
		if (gpio_is_valid(pcie0_gpio_iso)) {
				ret = gpio_request(pcie0_gpio_iso, "pcie0_gpio_iso");
				if (ret < 0)
					printk(KERN_ERR "%s: can't request gpio %d\n", __func__, pcie0_gpio_iso);
			} else
				printk(KERN_ERR "%s: gpio %d is not valid\n", __func__, pcie0_gpio_iso);
	}

	pcie0_clk = devm_clk_get(&pdev->dev, "clk_en_pcie0");
	if (IS_ERR(pcie0_clk)) {
		dev_err(&pdev->dev, "clk_en_pcie0 source missing or invalid\n");
		return PTR_ERR(pcie0_clk);
	}

	rstn_pcie0_stitch = rstc_get("rstn_pcie0_stitch");
	if(rstn_pcie0_stitch == NULL){
		dev_err(&pdev->dev, "rstn_pcie0_stitch source missing or invalid\n");
		return -EINVAL;
	}

	rstn_pcie0 = rstc_get("rstn_pcie0");
	if(rstn_pcie0 == NULL){
		dev_err(&pdev->dev, "rstn_pcie0 source missing or invalid\n");
		return -EINVAL;
	}

	rstn_pcie0_core = rstc_get("rstn_pcie0_core");
	if(rstn_pcie0_core == NULL){
		dev_err(&pdev->dev, "rstn_pcie0_core source missing or invalid\n");
		return -EINVAL;
	}

	rstn_pcie0_power = rstc_get("rstn_pcie0_power");
	if(rstn_pcie0_power == NULL){
		dev_err(&pdev->dev, "rstn_pcie0_power source missing or invalid\n");
		return -EINVAL;
	}

	rstn_pcie0_nonstich = rstc_get("rstn_pcie0_nonstich");
	if(rstn_pcie0_nonstich == NULL){
		dev_err(&pdev->dev, "rstn_pcie0_nonstich source missing or invalid\n");
		return -EINVAL;
	}

	rstn_pcie0_phy = rstc_get("rstn_pcie0_phy");
	if(rstn_pcie0_phy == NULL){
		dev_err(&pdev->dev, "rstn_pcie0_phy source missing or invalid\n");
		return -EINVAL;
	}

	rstn_pcie0_phy_mdio = rstc_get("rstn_pcie0_phy_mdio");
	if(rstn_pcie0_phy_mdio == NULL){
		dev_err(&pdev->dev, "rstn_pcie0_phy_mdio source missing or invalid\n");
		return -EINVAL;
	}

	reset_control_assert(rstn_pcie0_stitch);
	reset_control_assert(rstn_pcie0);
	reset_control_assert(rstn_pcie0_core);
	reset_control_assert(rstn_pcie0_power);
	reset_control_assert(rstn_pcie0_nonstich);
	reset_control_assert(rstn_pcie0_phy);
	reset_control_assert(rstn_pcie0_phy_mdio);

	rtd129x_cpu_id = get_rtd129x_cpu_id();
	rtd129x_cpu_revision = get_rtd129x_cpu_revision();

	if (rtk_pcie_hw_initial(&pdev->dev) < 0) {
		dev_err(&pdev->dev, "rtk_pcie_hw_initial fail\n");
		return -EINVAL;
	}

	/*-------------------------------------------
	 * Register PCI-E host
	 *-------------------------------------------*/
	ret = of_pci_get_host_bridge_resources(pdev->dev.of_node, 0x0, 0xff, &res, &iobase);
	if (ret)
		return ret;

	bus = pci_create_root_bus(&pdev->dev, 0x0, &rtk_pcie_ops, NULL, &res);

	if (!bus)
		return -ENOMEM;

	pci_scan_child_bus(bus);
	pci_assign_unassigned_bus_resources(bus);
	pci_bus_add_devices(bus);

	dev_info(&pdev->dev, "PCIE host driver initial done.\n");

	return ret;
}

static int rtk_pcie_shutdown(struct platform_device *pdev)
{
	dev_info(&pdev->dev, "shutdown enter ...\n");

	if (gpio_is_valid(pcie0_gpio_reset))
		gpio_direction_output(pcie0_gpio_reset, 0);

	msleep(100);

	if (gpio_is_valid(pcie0_gpio_iso))
		gpio_direction_output(pcie0_gpio_iso, 0);

	dev_info(&pdev->dev, "shutdown exit ...\n");
}

#ifdef CONFIG_SUSPEND
static int rtk_pcie1_suspend(struct device *dev)
{
	dev_info(dev, "suspend enter ...\n");

	if (gpio_is_valid(pcie0_gpio_reset))
		gpio_direction_output(pcie0_gpio_reset, 0);

	msleep(100);

	if (gpio_is_valid(pcie0_gpio_iso))
		gpio_direction_output(pcie0_gpio_iso, 0);

	if(RTK_PM_STATE == PM_SUSPEND_STANDBY){
		rtk_pci_ctrl_write(0x178, 0xA3FF0001);
		rtk_pci_ctrl_write(0x098, 0x400);
		rtk_pci_ctrl_write(0xC6C, 0x00000031);
		dev_info(dev, "Idle mode\n");
	}else{
		dev_info(dev, "Suspend mode\n");
		reset_control_assert(rstn_pcie0_stitch);
		reset_control_assert(rstn_pcie0);
		reset_control_assert(rstn_pcie0_core);
		reset_control_assert(rstn_pcie0_power);
		reset_control_assert(rstn_pcie0_nonstich);
		reset_control_assert(rstn_pcie0_phy);
		reset_control_assert(rstn_pcie0_phy_mdio);
		clk_disable_unprepare(pcie0_clk);
	}
	dev_info(dev, "suspend exit ...\n");

	return 0;
}

static int rtk_pcie1_resume(struct device *dev)
{
	int ret = 0;

	dev_info(dev, "resume enter ...\n");

	if(RTK_PM_STATE == PM_SUSPEND_STANDBY){
		rtk_pci_ctrl_write(0xC6C, 0x00000032);
		dev_info(dev, "Idle mode\n");
	}else{
		dev_info(dev, "Suspend mode\n");

		if (gpio_is_valid(pcie0_gpio_reset)) {
			ret = gpio_request(pcie0_gpio_reset, "pcie0_gpio_reset");
			if (ret < 0)
				printk(KERN_ERR "%s: can't request gpio %d\n", __func__, pcie0_gpio_reset);
		} else
			printk(KERN_ERR "%s: gpio %d is not valid\n", __func__, pcie0_gpio_reset);

		if (gpio_is_valid(pcie0_gpio_iso)) {
			ret = gpio_request(pcie0_gpio_iso, "pcie0_gpio_iso");
			if (ret < 0)
				printk(KERN_ERR "%s: can't request gpio %d\n", __func__, pcie0_gpio_iso);
		} else
			printk(KERN_ERR "%s: gpio %d is not valid\n", __func__, pcie0_gpio_iso);

		reset_control_deassert(rstn_pcie0_stitch);
		reset_control_deassert(rstn_pcie0);
		reset_control_deassert(rstn_pcie0_core);
		reset_control_deassert(rstn_pcie0_power);
		reset_control_deassert(rstn_pcie0_nonstich);
		reset_control_deassert(rstn_pcie0_phy);
		reset_control_deassert(rstn_pcie0_phy_mdio);
		ret = clk_prepare_enable(pcie0_clk);
		if (ret) {
			dev_err(dev, "unable to enable pcie0_clk clock\n");
			clk_disable_unprepare(pcie0_clk);
			return -EINVAL;
		}

		if (rtk_pcie_hw_initial(dev) < 0) {
			dev_err(dev, "rtk_pcie_hw_initial fail\n");
			return -EINVAL;
		}
	}

	dev_info(dev, "resume exit ...\n");

	return ret;
}

static struct dev_pm_ops rtk_pcie1_pm_ops = {
	.suspend_noirq = rtk_pcie1_suspend,
	.resume_noirq = rtk_pcie1_resume,
};
#endif //CONFIG_SUSPEND

static const struct of_device_id rtk_pcie_match_table[] = {
	{.compatible = "realtek,rtd1295-pcie-slot1",},
	{},
};

static struct platform_driver rtk_pcie_driver = {
	.driver = {
		   .name = "[RTD129x PCIE Slot1]",
		   .of_match_table = of_match_ptr(rtk_pcie_match_table),
#ifdef CONFIG_SUSPEND
		   .pm = &rtk_pcie1_pm_ops,
#endif
	},
	.probe = rtk_pcie_probe,
	.shutdown = rtk_pcie_shutdown,
};
module_platform_driver(rtk_pcie_driver);

MODULE_AUTHOR("James Tai <james.tai@realtek.com>");
MODULE_DESCRIPTION("Realtek PCIe slot1 host controller driver");

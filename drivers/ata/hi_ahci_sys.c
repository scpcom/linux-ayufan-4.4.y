#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <mach/hardware.h>

#ifdef CONFIG_ARCH_HI3536
#include "hi_ahci_sys_hi3536_defconfig.h"
#include "hi_ahci_sys_hi3536_defconfig.c"
#endif/*CONFIG_ARCH_HI3536*/

#ifdef CONFIG_ARCH_HI3521A
#include "hi_ahci_sys_hi3521a_defconfig.h"
#include "hi_ahci_sys_hi3521a_defconfig.c"
#endif/*CONFIG_ARCH_HI3521A*/

#ifdef CONFIG_ARCH_HI3531A
#include "hi_ahci_sys_hi3531a_defconfig.h"
#include "hi_ahci_sys_hi3531a_defconfig.c"
#endif/*CONFIG_ARCH_HI3531A*/

static int n_ports = CONFIG_HI_SATA_PORTS;
static int phy_mode = CONFIG_HI_SATA_MODE;

#ifdef MODULE
module_param(phy_config, uint, 0600);
MODULE_PARM_DESC(phy_config, "sata phy config (default:0x0e180000)");
module_param(n_ports, uint, 0600);
MODULE_PARM_DESC(n_ports, "sata port number (default:2)");
module_param(mode_3g, uint, 0600);
MODULE_PARM_DESC(phy_mode, "sata phy mode (0:1.5G;1:3G(default);2:6G)");
#endif

int hi_sata_init(struct device *dev, void __iomem *mmio)
{
	int port_num;

	port_num = n_ports;

#ifdef CONFIG_ARCH_HI3536
	if (hi_sata_use_esata() == 1)
		port_num += 1;
	else if (hi_sata_use_esata() == 0)
		port_num += 2;
#endif
#ifdef CONFIG_ARCH_HI3531A
	port_num = hi_sata_port_nr();
	if ((port_num < 1) || (port_num > 4)) {
		pr_err("sata ports number:%d WRONG!!!\n", n_ports);
		return -EINVAL;
	}
#endif

	hi_sata_poweron();
	msleep(20);
	hi_sata_clk_enable();
	hi_sata_phy_clk_sel();
	hi_sata_unreset();
	msleep(20);
	hi_sata_phy_unreset();
	msleep(20);
	hi_sata_phy_reset();
	msleep(20);
	hi_sata_phy_unreset();
	msleep(20);
	hi_sata_clk_unreset();
	msleep(20);
	hisata_phy_init(mmio, phy_mode, port_num);

	return 0;
}
EXPORT_SYMBOL(hi_sata_init);

void hi_sata_exit(struct device *dev)
{
	hi_sata_phy_reset();
	msleep(20);
	hi_sata_reset();
	msleep(20);
	hi_sata_clk_reset();
	msleep(20);
	hi_sata_clk_disable();
	hi_sata_poweroff();
	msleep(20);
}
EXPORT_SYMBOL(hi_sata_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hisilicon osdrv group");

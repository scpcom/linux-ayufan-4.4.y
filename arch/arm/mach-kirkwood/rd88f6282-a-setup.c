/*
 * arch/arm/mach-kirkwood/rd88f6282-a-setup.c
 *
 * Marvell RD-88F6282-A Reference Board Setup
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mtd/partitions.h>
#include <linux/ata_platform.h>
#include <linux/mv643xx_eth.h>
#include <net/dsa.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include "common.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

static struct mtd_partition rd88f6282_a_nand_parts[] = {
	{
		.name = "u-boot",
		.offset = 0,
		.size = SZ_1M
	}, {
		.name = "uImage",
		.offset = MTDPART_OFS_NXTBLK,
		.size = SZ_2M
	}, {
		.name = "root",
		.offset = MTDPART_OFS_NXTBLK,
		.size = MTDPART_SIZ_FULL
	},
};

static struct dsa_chip_data rd88f6282_a_switch_chip_data = {
	.sw_addr	= 16,
	.port_names[0]	= "lan1",
	.port_names[1]	= "lan2",
	.port_names[2]	= "lan3",
	.port_names[3]	= "lan4",
	.port_names[5]	= "cpu",
};

static struct dsa_platform_data rd88f6282_a_switch_plat_data = {
	.nr_chips	= 1,
	.chip		= &rd88f6282_a_switch_chip_data,
};

static struct mv643xx_eth_platform_data rd88f6282_a_ge00_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR(0),
};

static struct mv643xx_eth_platform_data rd88f6282_a_ge01_data = {
	.phy_addr	= MV643XX_ETH_PHY_ADDR(10),
};

static struct mv_sata_platform_data rd88f6282_a_sata_data = {
	.n_ports	= 2,
};

static void __init rd88f6282_a_init(void)
{
	/*
	 * Init board id (Marvell-internal) and MPPs
	 */
	mvBoardIdSet(RD_88F6282A_ID);
	mvBoardEnvInit();
	mvCtrlEnvInit();

	/*
	 * Basic setup. Needs to be called early.
	 */
	kirkwood_init();

	kirkwood_nand_init(ARRAY_AND_SIZE(rd88f6282_a_nand_parts), 25);
	kirkwood_ehci_init();
	kirkwood_hwmon_init();

	kirkwood_ge00_init(&rd88f6282_a_ge00_data);
	kirkwood_ge01_init(&rd88f6282_a_ge01_data);
	kirkwood_ge01_switch_init(&rd88f6282_a_switch_plat_data, NO_IRQ);

	kirkwood_sata_init(&rd88f6282_a_sata_data);
	kirkwood_i2c_init();
	kirkwood_uart1_init();
}

static int __init rd88f6282_a_pci_init(void)
{
	if (machine_is_rd88f6282_a())
		kirkwood_pcie_init(KW_PCIE1 | KW_PCIE0);

	return 0;
}
subsys_initcall(rd88f6282_a_pci_init);

MACHINE_START(RD88F6282_A, "Marvell RD-88F6282-A Reference Board")
	/* Maintainer: Kosta Zertsekel <konszert@marvell.com> */
	.atag_offset	= 0x100,
	.init_machine	= rd88f6282_a_init,
	.map_io		= kirkwood_map_io,
	.init_early	= kirkwood_init_early,
	.init_irq	= kirkwood_init_irq,
	.timer		= &kirkwood_timer,
MACHINE_END

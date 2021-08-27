/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mtd/partitions.h>
#include <linux/ata_platform.h>
#include <linux/mv643xx_eth.h>
#include <linux/spi/flash.h>
#include <linux/spi/spi.h>
#include <linux/spi/orion_spi.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/kirkwood.h>
#include <plat/mvsdio.h>
#include "common.h"
#include "mpp.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

static struct mtd_partition db88f6281_nand_parts[] = {
	{
		.name = "u-boot",
		.offset = 0,
		.size = SZ_1M
	}, {
		.name = "uImage",
		.offset = MTDPART_OFS_NXTBLK,
		.size = SZ_4M
	}, {
		.name = "root",
		.offset = MTDPART_OFS_NXTBLK,
		.size = MTDPART_SIZ_FULL
	},
};

static void __init xcat98dx_init(void)
{
	/*
	 * Init board id (Marvell-internal) and MPPs
	 */
	mvBoardIdSet(XCAT98DX_ID);
#if 0
	mvBoardEnvInit();
	mvCtrlEnvInit();
#endif

	/*
	 * Basic setup. Needs to be called early.
	 */
	kirkwood_init();

	kirkwood_xcat_clock_init();

	kirkwood_nand_init(ARRAY_AND_SIZE(db88f6281_nand_parts), 25);
	kirkwood_i2c_init();
	kirkwood_uart0_init();
}

static int __init xcat98dx_pci_init(void)
{
	if (machine_is_xcat98dx())
		kirkwood_pcie_init(KW_PCIE0);

	return 0;
}
subsys_initcall(xcat98dx_pci_init);

MACHINE_START(XCAT98DX, "Marvell xCat98DX Development Board")
	/* Maintainer: Kosta Zertsekel <konszert@marvell.com> */
	.atag_offset	= 0x100,
	.init_machine	= xcat98dx_init,
	.map_io		= kirkwood_map_io,
	.init_early	= kirkwood_init_early,
	.init_irq	= kirkwood_init_irq,
	.timer		= &kirkwood_timer,
MACHINE_END

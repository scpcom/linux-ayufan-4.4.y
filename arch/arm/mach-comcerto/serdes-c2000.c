
/*
 * Copyright (C) Mindspeed Technologies, Inc. 2011. All rights reserved.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * @file serdes.c
 * @brief this C file will contain all required functions to program
 *        Snowbush SerDes PHY interface.
 * @date 10/02/2011
 */

#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/types.h>
#include <asm/io.h>
#include <mach/comcerto-2000.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/sizes.h>
#include <mach/serdes-c2000.h>

#define MAX_LANE_OK_WAIT_JIFFIES	(200 * HZ) / 1000    /* 200ms */
#define MAX_CMU_OK_WAIT_JIFFIES		(2000 * HZ) / 1000   /* 2 Seconds */

/**
 * This function Wait for the 'Lane OK' to be signaled by the 
 * Snowbush Serdes PHY.
 * @param sbphy_num	SerDes PHY intefrace number.
 */
static int wait_lane_ok(u32 sbphy_num)
{
	u32 rd_data = 0, masked_data = 0;
	u32 lane_ok_dtctd_mask = 0x00001000;
	unsigned long deadline = jiffies + MAX_LANE_OK_WAIT_JIFFIES;

	/* Keep looping until you see the lane_ok_o of Serdes */
	do
	{
		rd_data = readl(COMCERTO_SERDES_DWC_CFG_REG( sbphy_num, SD_PHY_STS_REG_OFST));

		/* Mask lane_ok Status */
		masked_data = rd_data & lane_ok_dtctd_mask;

		if(masked_data == lane_ok_dtctd_mask) {
			/* Lane OK Detected on Serdes Port */
			printk(KERN_INFO "Serdes%d: Lane OK Passed\n",sbphy_num);
			return 1;
		}

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	printk(KERN_INFO "Serdes%d: Lane OK Failed\n",sbphy_num);
	return 0;
}


/**
 * This function wait for the 'CMU OK' to be signaled by the
 * Snowbush Serdes PHY.
 * @param sbphy_num	SerDes PHY intefrace number.
 */
static int wait_cmu_ok(u32 sbphy_num)
{
	u32 rd_data = 0, masked_data = 0;
	u32 cmu_ok_dtctd_mask = 0x00004000;
	int CMU_Offset;
	unsigned long deadline = jiffies + MAX_CMU_OK_WAIT_JIFFIES;

	CMU_Offset = COMCERTO_SERDES_DWC_CFG_REG( sbphy_num, SD_PHY_STS_REG_OFST );


	/* Keep looping until you see the cmu_ok_o of Serdes */
	do
	{
		rd_data = readl(CMU_Offset);

		/* Mask cmu_ok Status */
		masked_data = rd_data & cmu_ok_dtctd_mask;

		if(masked_data == cmu_ok_dtctd_mask) {
			/* CMU OK Detected on Serdes Port */
			//printk(KERN_INFO "Serdes%d: CMU OK Passed\n",sbphy_num);
			return 1;
		}

		cond_resched();

	} while (!time_after_eq(jiffies, deadline));

	printk(KERN_INFO "Serdes%d: CMU OK Failed\n",sbphy_num);

	return 0;
}


/**
 * This function wait for the specified configured Snowbush PHY
 * (Serdes) to issue it's CMU-OK, and it's Lane to become Ready
 * after releasing the CMU & Lane resets.
 * @param sbphy_num	SerDes PHY intefrace number.
 */
static int wait_sb_cmu_lane_rdy(u32 sbphy_num, u32 type)
{
	u32 sd_ctl2_reg_offset;
	u32 cmu_rst_mask = 0x00010000;
	u32 lane_rst_mask = 0x00000040;
	u32 tmp = 0;

	sd_ctl2_reg_offset = COMCERTO_SERDES_DWC_CFG_REG( sbphy_num, SD_PHY_CTRL2_REG_OFST );

	/* Releasing the CMU Reset */
	tmp = readl(sd_ctl2_reg_offset);
	tmp = tmp & (~cmu_rst_mask);
	tmp = tmp | cmu_rst_mask;

	writel(tmp, sd_ctl2_reg_offset );

	/* Waiting for CMU OK */
	if( !wait_cmu_ok(sbphy_num) )
		return -1;

	if ( type == SD_DEV_TYPE_PCIE )
		writel(0xC3, COMCERTO_SERDES_REG(sbphy_num, (SD_COMMON_LANE << 2)));
	else
		writel(0x03, COMCERTO_SERDES_REG(sbphy_num, (SD_COMMON_LANE << 2)));

	/* Releasing the Lane Reset */
	tmp = readl(sd_ctl2_reg_offset);
	tmp = tmp & (~lane_rst_mask);
	tmp = tmp | lane_rst_mask;

	writel(tmp, sd_ctl2_reg_offset);

	/* Waiting for the Lane Ready */
	if (type != SD_DEV_TYPE_PCIE) {
		if( !wait_lane_ok(sbphy_num) )
			return -1;
	}

	return 0;
}


/**
 * This function initialize the Snowbush PHY (Serdes) for operation
 * with the one of the PCIE,SATA or SGMII IP blocks, and then waiting
 * until it issue it's CMU-OK, and it's  Lane to become Ready after
 * releasing the CMU & Lane Resets.
 * @param phy_num	SerDes PHY intefrace number.
 * @param *regs		Register file (Array of registers and coresponding
 *                      values to be programmed).
 * @param size		Number of registers to be programmed.
 */
int serdes_phy_init(int phy_num, struct serdes_regs_s *regs, int size, int type)
{
	int ii;

	/* Initilize serdes phy registers */
	for( ii = 0; ii < size; ii++ )
		writel(regs[ii].val, COMCERTO_SERDES_REG(phy_num, regs[ii].ofst));

	/* Wait for the initialization of Serdes-1 Port/Lane to become Ready */
	return wait_sb_cmu_lane_rdy(phy_num, type);
}

EXPORT_SYMBOL(serdes_phy_init);


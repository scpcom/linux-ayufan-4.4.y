/*
 * arch/arm/plat-oxnas/sata_helper.c
 *
 * Copyright (C) 2010 Oxford Semiconductor Ltd
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

#include <mach/sata_helper.h>

int sata_transfer_complete(u32 *port_base)
{
	u32 status;
	int failure = 0;
	int complete = 0;

	// Is the transfer complete?
	status = readl(port_base + OX810SATA_SATA_COMMAND);

	complete = !(status & CMD_CORE_BUSY);
	if (complete) {
		// Did the transfer fail?
		status = readl(port_base + OX810SATA_ORB2);

		failure = status & (1 << 24);
		if (failure) {
			// Extract the failure status
			status >>= 24;
			status &= 0xFF;
		}
	}

	return failure ? -(int)status : complete;
}

void direct_sata_transfer(
	int 	  action, /* C_WRITE_DMA_EXT or C_READ_DMA_EXT */ 
	int		  port,
	sector_t  lba,
	long long sectors)
{
	u32 *sata_port_base;
    u32  orb[6];

	BUG_ON((port < 0) || (port > 1));
	BUG_ON(!sectors);
	BUG_ON(sectors & 0xFFFFFFFF00000000ULL);
    BUG_ON(lba & 0xFFFF000000000000ULL);

//printk(KERN_INFO "Sata transfer - action - %d port - %d sector - %lld num_of_sectors - %lld \n", action, port, lba, sectors);

	/* Get base address of SATA registers for the port */
	sata_port_base = (u32*)(port ? SATA1_REGS_BASE : SATA0_REGS_BASE);

	/* Form the SATA command - only 48 bit disks supported */
    orb[1] = (ATA_LBA << 24);
    orb[2] = (action << 24) | (sectors & 0xFFFF);
    orb[3] = (u32)lba;
    orb[4] = (lba & 0xFFFF00000000ULL) >> 32;           
    orb[5] = (sectors & 0xFFFF0000) >> 16;

	// Disable all interrupts for ports and RAID controller
	writel(~0, (u32*)SATA0_REGS_BASE + OX810SATA_INT_DISABLE);
	writel(~0, (u32*)SATA1_REGS_BASE + OX810SATA_INT_DISABLE);
	writel(~0, (u32*)SATARAID_REGS_BASE + OX810SATA_INT_DISABLE);

	// Disable all interrupts for core
	writel(~0, OX810SATA_CORE_IEC);

	// Clear any interrupts for ports and RAID controller
	writel(~0, (u32*)SATA0_REGS_BASE + OX810SATA_INT_CLEAR);
	writel(~0, (u32*)SATA1_REGS_BASE + OX810SATA_INT_CLEAR);
	writel(~0, (u32*)SATARAID_REGS_BASE + OX810SATA_INT_CLEAR);

	/* Write command registers */
	writel(orb[1], sata_port_base + OX810SATA_ORB1);
	writel(orb[2], sata_port_base + OX810SATA_ORB2);
	writel(orb[3], sata_port_base + OX810SATA_ORB3);
	writel(orb[4], sata_port_base + OX810SATA_ORB4);
	writel(orb[5], sata_port_base + OX810SATA_ORB5);

	/* Enable interrupts from core */
	writel(OX810SATA_NORMAL_INTS_WANTED, OX810SATA_CORE_IES);  

	/* Enable interrupts from port */
	writel(OX810SATA_INT_WANT, sata_port_base + OX810SATA_INT_ENABLE);

	/* Issue command */
	writel(CMD_WRITE_TO_ORB_REGS, sata_port_base + OX810SATA_SATA_COMMAND);
}

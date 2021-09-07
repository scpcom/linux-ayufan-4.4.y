/*
 * arch/arm/plat-oxnas/include/mach/sata_helper.h
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

#ifndef SATA_HELPER_H_
#define SATA_HELPER_H_

#include <linux/ata.h>
#include <mach/ox810sata.h>

#define SECTOR_SHIFT 9
#define SECTOR_SIZE	(1 << SECTOR_SHIFT)

#define C_WRITE_DMA_EXT	0x35
#define C_READ_DMA_EXT	0x25

extern int sata_transfer_complete(u32 *port_base);
extern void direct_sata_transfer(int action, int port, sector_t lba, long long sect);

#endif /* SATA_HELPER_H_ */

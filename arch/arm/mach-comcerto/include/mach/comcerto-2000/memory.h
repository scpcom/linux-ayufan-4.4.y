/*
 *  arch/arm/arch-comcerto/include/mach/comcerto-2000/memory.h
 *
 *  Copyright (C) 2011 Mindspeed Technologies, Inc.
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


#ifndef __MEMORY_H__
#define __MEMORY_H__

	/* Physical addresses of memories */
	#define IRAM_MEMORY_SIZE			SZ_64K

/*
 * Virtual view <-> DMA view memory address translations
 * virt_to_bus: Used to translate the virtual address to an
 *              address suitable to be passed to set_dma_addr
 * bus_to_virt: Used to convert an address for DMA operations
 *              to an address that the kernel can use.
 */

#define aram_to_virt(p)		(void*)(((unsigned long)p - COMCERTO_AXI_IRAM_BASE) + IRAM_MEMORY_VADDR)
#define virt_to_aram(v)		(((unsigned long)v - IRAM_MEMORY_VADDR) + COMCERTO_AXI_IRAM_BASE)
#endif

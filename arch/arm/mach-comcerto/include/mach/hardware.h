/*
 *  arch/arm/mach-comcerto/include/mach/hardware.h
 *
 *  Copyright (C) 2012 Mindspeed Technologies, Inc.
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

#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <linux/types.h>

	/***** Device *****/
	#if defined(CONFIG_ARCH_M86XXX)
		#include <mach/comcerto-2000.h>
	#else
		#error "mach/hardware.h :  Unknown architecture" 
	#endif
	

	/***** Board *****/
	#if defined(CONFIG_C2K_ASIC)
		#include <mach/board-c2kasic.h>
	#elif defined(CONFIG_C2K_EVM)
		#include <mach/board-c2kevm.h>
	#elif defined(CONFIG_C2K_MFCN_EVM)
		#include <mach/board-c2kmfcnevm.h>
	#elif defined(CONFIG_RTSM_C2K)
		#include <mach/board-c2krtsm.h>
	#elif defined(CONFIG_SEQUOIA)
		#include <mach/board-sequoia.h>
	
	#else
		#error "mach/board_XXX.h :  Unknown board"
	#endif

	#include <mach/comcerto-common.h>

#endif

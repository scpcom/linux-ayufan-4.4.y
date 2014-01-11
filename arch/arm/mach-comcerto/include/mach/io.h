/*
 *  linux/include/asm-arm/arch-comcerto/io.h
 *
 *  Copyright (C) 2004,2008 Mindspeed Technologies, Inc.
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
#ifndef __ASM_ARCH_IO_H
#define __ASM_ARCH_IO_H


#if defined(CONFIG_ARCH_M86XXX)
	#include <mach/comcerto-2000/io.h>
#else
	#error "Unsupported CPU"
#endif

#endif /* __ASM_ARCH_IO_H */

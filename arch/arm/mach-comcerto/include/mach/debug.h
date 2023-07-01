/*
 *  linux/include/asm-arm/arch-comcerto/debug.h
 *
 *  Copyright (C) 2004,2005 Mindspeed Technologies, Inc.
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

#ifndef _DEBUG_H
#define _DEBUG_H

#include <linux/kernel.h>

/* debug messages: to disable comment the next line */
#define DEBUG_MSG

/* several debug levels: 1 enables, 0 disables */

#define DEBUG_ALWAYS 		1

/* prints general information */
#define DEBUG_INFO			1

/* prints timing information */
#define DEBUG_TIMING		0

#define DEBUG_DISPLAY 		1

#define VED_INIT_FUNC		1
#define VED_RX_FUNC		0
#define VED_TX_FUNC		0
#define VED_STATE		0
#define SMI_PART		0

#define SKB_POOL_ERR		1
#define SKB_POOL_INIT		0
#define SKB_POOL_FUNC		0

#define MSP_ERR			1
#define MSP_INIT		0
#define MSP_FUNC		0


/* add other debug messages types here */

/* the debug macro */
#ifdef DEBUG_MSG
#define info(fmt, args...) printk(KERN_INFO __FILE__ ": " fmt "\n" , ## args)
#define PDEBUG(type, fmt, args...) do {if(type) info ("%d: " fmt, __LINE__ , ## args);} while(0)
#else
#define PDEBUG(type, fmt, args...) do{} while(0)
#endif

#endif	/* _DEBUG_H */

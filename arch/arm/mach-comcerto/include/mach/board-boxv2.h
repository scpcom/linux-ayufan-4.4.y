/*
 * arch/arm/mach-comcerto/include/mach/board-boxv2.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BOARD_BOXV2_H__
#define __BOARD_BOXV2_H__

#include <mach/hardware.h>
	
	/***********************************
	 * Expansion bus configuration
	 ***********************************/
	 
	#define COMCERTO_EXPCLK		42000000	/* 42MHz */

	/***********************************
	 * GPIO
	 ***********************************/
	#define COMCERTO_OUTPUT_GPIO		(COMCERTO_NAND_CE)
	#define COMCERTO_IRQ_RISING_EDGE_GPIO	0 // [FIXME]
	#define COMCERTO_IRQ_FALLING_EDGE_GPIO	(GPIO_2 | GPIO_0) // [FIXME]
	#define COMCERTO_IRQ_LEVEL_GPIO 	GPIO_2 // [FIXME]
	/*Are pins used either as GPIO or as pins for others IP blocks*/
	#define COMCERTO_GPIO_PIN_USAGE		(SPI_BUS) // [FIXME]

	/***********************************
	 * NAND
	 ***********************************/
	#define COMCERTO_EXP_CS4_SEG_SZ		1

	#define COMCERTO_NAND_FIO_ADDR		EXP_CS4_AXI_BASEADDR
	#define COMCERTO_NAND_BR		0x20000000 /* BR is on GPIO_29 */
	#define COMCERTO_NAND_CE		0x10000000 /* CE is on GPIO_28 */
	#define COMCERTO_NAND_IO_SZ		((COMCERTO_EXP_CS4_SEG_SZ << 12) +0x1000)
#endif

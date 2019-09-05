/*
 * arch/arm/mach-comcerto/include/mach/board-c2krtsm.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BOARD_C2KRTSM_H__
#define __BOARD_C2KRTSM_H__

#include <mach/hardware.h>
	
	/***********************************
	 * Expansion bus configuration
	 ***********************************/
	 
	#define COMCERTO_EXPCLK		50000000	/* 50MHz */

	/***********************************
	 * GPIO
	 ***********************************/
	#define COMCERTO_OUTPUT_GPIO 		(COMCERTO_NAND_CE | COMCERTO_NAND_ALE | COMCERTO_NAND_CLE) // [FIXME]
	#define COMCERTO_IRQ_RISING_EDGE_GPIO	0 // [FIXME]
	#define COMCERTO_IRQ_FALLING_EDGE_GPIO	(GPIO_2 | GPIO_0) // [FIXME]
	#define COMCERTO_IRQ_LEVEL_GPIO 	GPIO_2 // [FIXME]
	/*Are pins used either as GPIO or as pins for others IP blocks*/
	#define COMCERTO_GPIO_PIN_USAGE		(SPI_BUS) // [FIXME]

	/***********************************
	 * EEPROM
	 ***********************************/
	
	/***********************************
	 * NOR
	 ***********************************/
	#define NORFLASH_MEMORY_PHY1		EXP_CS0_AXI_BASEADDR

	/***********************************
	 * NAND
	 ***********************************/
	#define COMCERTO_EXP_CS4_SEG_SZ		1
	
	#define COMCERTO_NAND_FIO_ADDR		COMCERTO_EXP_CS4_BASE
	#define COMCERTO_NAND_BR		GPIO_6
	#define COMCERTO_NAND_CE		GPIO_29
	#define COMCERTO_NAND_ALE		GPIO_30
	#define COMCERTO_NAND_CLE		GPIO_31
	#define COMCERTO_NAND_IO_SZ		((COMCERTO_EXP_CS4_SEG_SZ << 12) +0x1000)

	/***********************************
	 * SLIC
	 ***********************************/
	#define COMCERTO_SLIC_GPIO_IRQ		IRQ_G0

#endif

/*
 * arch/arm/mach-comcerto/include/mach/board-optimus.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BOARD_OPTIMUS_H__
#define __BOARD_OPTIMUS_H__

#include <mach/hardware.h>

	/***********************************
	 * Expansion bus configuration
	 ***********************************/

	#define COMCERTO_EXPCLK		50000000	/* 50MHz */
	#define MOCA_RESET_GPIO_PIN	GPIO_PIN_11
	#define PCIE_ADDITIONAL_RESET_PIN	GPIO_PIN_62

	/***********************************
	 * GPIO
	 ***********************************/
	#define COMCERTO_OUTPUT_GPIO	(COMCERTO_NAND_CE|MOCA_RESET_GPIO_PIN)
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

	#define COMCERTO_NAND_FIO_ADDR		EXP_CS4_AXI_BASEADDR
	#define COMCERTO_NAND_BR		0x20000000 /* BR is on GPIO_29 */
	#define COMCERTO_NAND_CE		0x10000000 /* CE is on GPIO_28 */
	#define COMCERTO_NAND_IO_SZ		((COMCERTO_EXP_CS4_SEG_SZ << 12) +0x1000)

	/***********************************
	 * SLIC
	 ***********************************/
	#define COMCERTO_SLIC_GPIO_IRQ		IRQ_G2

	/*****************************************
	 * Spacecast board specific configuration
	 *****************************************/
	#if defined(CONFIG_GOOGLE_SPACECAST)
		#undef COMCERTO_OUTPUT_GPIO
		#undef MOCA_RESET_GPIO_PIN
		#define TUNER_RESET_GPIO_PIN	GPIO_PIN_11
		#define USB_BRG_RESET_GPIO_PIN	GPIO_PIN_9
		#define COMCERTO_OUTPUT_GPIO	(USB_BRG_RESET_GPIO_PIN|TUNER_RESET_GPIO_PIN)
	#endif

#endif

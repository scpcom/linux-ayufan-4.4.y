#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * include/asm-arm/arch-aurora/dove.h
 *
 * Generic definitions for Marvell Dove MV88F6781 SoC
 *
 * Author: Tzachi Perelstein <tzachi@marvell.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
 
#ifndef __ASM_ARCH_AURORA_H
#define __ASM_ARCH_AURORA_H

#include <mach/vmalloc.h>

/****************************************************************/
/******************* System Address Mapping *********************/
/****************************************************************/

/* The base address of memory that should be reserved for IO windows.
** The reserved end address is 0xFFFFFFFF.
*/
#define CONFIG_DRAM_IO_RESERVE_BASE	0xF0000000ll

/*
 * Armada-XP address maps.
 * Virtual Addresses should be between 0xFA800000 and 0xFEFFFFFF (72MB)
 * Physical Addresses should be between 0xF0000000 and 0xFFFFFFFF (256MB)
 * If a larger physical address space is needed, then the amount of DRAM
 * available for Linux will be smaller.
 *
 * phys		virt		size
 * e0000000	@runtime	128M	PCIe-0 Memory space
 * e8000000	@runtime	128M	PCIe-1 Memory space
 * f0000000	fab00000	16M	SPI-CS0 (Flash)
 * f1000000	fbb00000	1M	Internal Registers
 * f1100000	fbc00000	1M	PCIe-0 I/O space
 * f1200000	fbd00000	1M	PCIe-1 I/O space
 * f1300000	fbe00000	1M	PCIe-2 I/O space
 * f1400000	fbf00000	1M	PCIe-3 I/O space
 * f1500000	fc000000	1M	PCIe-4 I/O space
 * f1600000	fc100000	1M	PCIe-5 I/O space
 * f1700000	fc200000	1M	PCIe-6 I/O space
 * f1800000	fc300000	1M	PCIe-7 I/O space
 * f1900000	fc400000	1M	PCIe-8 I/O space
 * f1a00000	fc500000	1M	PCIe-9 I/O space
 * f1b00000	fc600000	1M	DMA based UART
 * f4000000	fe700000	1M	Device-CS0
 * f2000000	fc700000	32M	Boot-Device CS (NOR Flash)
 * f4100000	fe800000	1M	Device-CS1 (NOR Flash)
 * f4200000	fe900000	1M	Device-CS2 (NOR Flash)
 * f4300000	fea00000	1M	Device-CS3 (NOR Flash)
 * f4400000	feb00000	1M	CESA SRAM (2 units)
 * f4500000	fec00000	1M	NETA-BM (PNC)
 * fff00000	fed00000	1M	BootROM
 * f4700000	fee00800	1M	PMU Scratch pad
 * f4800000	fef00000	1M	Legacy Nand Flash
 */

/*
 * SDRAM Address decoding
 * These values are dummy. Uboot configures these values.
 */
#define SDRAM_CS0_BASE  		0x00000000
#define SDRAM_CS0_SIZE  		_256M
#define SDRAM_CS1_BASE  		0x10000000
#define SDRAM_CS1_SIZE  		_256M
#define SDRAM_CS2_BASE  		0x20000000
#define SDRAM_CS2_SIZE  		_256M
#define SDRAM_CS3_BASE  		0x30000000
#define SDRAM_CS3_SIZE  		_256M

/* First, we place the "static" windows (that has less chance to have a size
 * change), then all the "dynamic" windows (the ones that might need different
 * sizes depending on the system configuration - SPI flash size, PEX cards...).
 */

/*
 * Internal registers.
 * PHYS: 0xF1000000 to 0xF1100000
 * VIRT: 0xFBC00000 to 0xFBC00000
 * INTER_REGS_VIRT_BASE Must be 2MB aligned in order to support CONFIG_DEBUG_LL.
 * Before paging_init, the UART port is mapped by a section entry (2MB in LPAE).
 */
#define INTER_REGS_PHYS_BASE	0xF1000000
#define INTER_REGS_VIRT_BASE	0xFBC00000
#define INTER_REGS_BASE			INTER_REGS_VIRT_BASE /* For compatibility */

/*
 * Uart registers.
 * PHYS: 0xF1200000 - 0xF1300000
 * VIRT: 0xFBE00000
 */
#define UART_REGS_BASE			0xF1200000
#define UART_VIRT_BASE			0xFBE00000
#define UART_SIZE			_1M

/*
 * BM / PNC window
 * PHYS: 0xF1300000 - 0xF1400000
 * VIRT: @runtime
 */
#define PNC_BM_PHYS_BASE		0xF1300000
#define PNC_BM_SIZE			_1M

/*
 * Crypto Engine(s)
 * PHYS: 0xF1400000 - 0xF1500000
 * VIRT: 0xFC000000
 */
#define CRYPT_ENG_PHYS_BASE(chan)	(0xF1400000 + (chan * 0x10000))
#define CRYPT_ENG_VIRT_BASE(chan)	(0xFC000000 + (chan * 0x10000))
#define CRYPT_ENG_SIZE			_64K

/*
 * PMU Scratch PAD
 * PHYS: 0xF1600000 - 0xF1700000
 * VIRT: 0xFC200000
 */
#define PMU_SCRATCH_PHYS_BASE		0xF1500000
#define PMU_SCRATCH_VIRT_BASE		0xFC100000
#define PMU_SCRATCH_SIZE		_1M

/*
 * PHYS: 0xF1700000 to 0xF100000
 * VIRT: 0xFC300000 - 0xFCC00000
 * PEX IO Address Decoding
 * Allocate 1MB for each IO windowi, total of 10MB.
 */
#define PEX0_IO_PHYS_BASE		0xF1600000
#define PEX0_IO_VIRT_BASE		0xFC200000
#define PEX0_IO_SIZE			_1M
#define PEX1_IO_PHYS_BASE		0xF1700000
#define PEX1_IO_VIRT_BASE		0xFC300000
#define PEX1_IO_SIZE			_1M
#define PEX2_IO_PHYS_BASE		0xF1800000
#define PEX2_IO_VIRT_BASE		0xFC400000
#define PEX2_IO_SIZE			_1M
#define PEX3_IO_PHYS_BASE		0xF1900000
#define PEX3_IO_VIRT_BASE		0xFC500000
#define PEX3_IO_SIZE			_1M
#define PEX4_IO_PHYS_BASE		0xF1A00000
#define PEX4_IO_VIRT_BASE		0xFC600000
#define PEX4_IO_SIZE			_1M
#define PEX5_IO_PHYS_BASE		0xF1B00000
#define PEX5_IO_VIRT_BASE		0xFC700000
#define PEX5_IO_SIZE			_1M
#define PEX6_IO_PHYS_BASE		0xF1C00000
#define PEX6_IO_VIRT_BASE		0xFC800000
#define PEX6_IO_SIZE			_1M
#define PEX7_IO_PHYS_BASE		0xF1D00000
#define PEX7_IO_VIRT_BASE		0xFC900000
#define PEX7_IO_SIZE			_1M
#define PEX8_IO_PHYS_BASE		0xF1E00000
#define PEX8_IO_VIRT_BASE		0xFCA00000
#define PEX8_IO_SIZE			_1M
#define PEX9_IO_PHYS_BASE		0xF1F00000
#define PEX9_IO_VIRT_BASE		0xFCB00000
#define PEX9_IO_SIZE			_1M

/*
 * Below are the IOs that might require size change depending on the
 * system peripherals.
 */

/*
 * PHYS: 0xF8000000 to 0xFFEFFFFF
 * VIRT: @runtime
 * PEX Memory Address Decoding
 * Virtual address not specified - remapped @runtime
 * Reserve 127MB for PEX MEM space, so that in case an interface needs more
 * than 2MB, it will be possible to relocate it within the reserved 127MB.
 */
#define PEX0_MEM_PHYS_BASE		0xF2000000
#define PEX0_MEM_SIZE			_2M
#define PEX1_MEM_PHYS_BASE		0xF2200000
#define PEX1_MEM_SIZE			_2M
#define PEX2_MEM_PHYS_BASE		0xF2400000
#define PEX2_MEM_SIZE			_2M
#define PEX3_MEM_PHYS_BASE		0xF2600000
#define PEX3_MEM_SIZE			_2M
#define PEX4_MEM_PHYS_BASE		0xF2800000
#define PEX4_MEM_SIZE			_2M
#define PEX5_MEM_PHYS_BASE		0xF2A00000
#define PEX5_MEM_SIZE			_2M
#define PEX6_MEM_PHYS_BASE		0xF2C00000
#define PEX6_MEM_SIZE			_2M
#define PEX7_MEM_PHYS_BASE		0xF2E00000
#define PEX7_MEM_SIZE			_2M
#define PEX8_MEM_PHYS_BASE		0xF3000000
#define PEX8_MEM_SIZE			_2M
#define PEX9_MEM_PHYS_BASE		0xF3200000
#define PEX9_MEM_SIZE			_2M

/*
 * Device Bus address decode windows.
 * PHYS: 0xF3300000 - 0xF3400000
 * VIRT: @runtime
 */
#define DEVICE_CS0_PHYS_BASE		0xF3300000
#define DEVICE_CS0_SIZE			_1M
#define DEVICE_CS1_PHYS_BASE		0xF3400000
#define DEVICE_CS1_SIZE			_1M
#define DEVICE_CS2_PHYS_BASE		0xF3500000
#define DEVICE_CS2_SIZE			_1M
#define DEVICE_CS3_PHYS_BASE		0xF3600000
#define DEVICE_CS3_SIZE			_1M

/*
 * Device Boot-CS Window
 * PHYS: 0xF4000000 - 0xF6000000
 * VIRT: @runtime
 */
#define DEVICE_BOOTCS_PHYS_BASE		0xF4000000
#define DEVICE_BOOTCS_SIZE		_32M

/*
 * SPI Flash window.
 * PHYS: 0xF6000000 - 0xF7000000
 * VIRT: @runtime
 * This is a 16MB window, if a larger flash exists, then the
 * window needs to be enlarged.
 */
#ifdef CONFIG_SYNO_ARMADA_ARCH
#define SPI_CS0_PHYS_BASE		0xF0000000
#else
#define SPI_CS0_PHYS_BASE		0xF6000000
#endif
#define SPI_CS0_SIZE			_16M

/*
 * Free area from 0xF7000000 to 0xFFF00000 (143MB).
 * This can be used to map a larger window for PCI / SPI / Device-CS....
 */

/*
 * PHYS: 0xFFF00000 - 0xFFFFFFFF
 * VIRT: 0xFF000000 - 0xFF0FFFFF
 * Bootrom window
 */
#define BOOTROM_PHYS_BASE		0xFFF00000
#define BOOTROM_VIRT_BASE		0xFF000000
#define BOOTROM_SIZE			_1M

/*
 * Linux native definitiotns
 */
#define XOR0_PHYS_BASE			(INTER_REGS_PHYS_BASE | 0x60900)
#define XOR1_PHYS_BASE			(INTER_REGS_PHYS_BASE | 0xF0900)
#define XOR0_HIGH_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0x60B00)
#define XOR1_HIGH_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0xF0B00)

#define	LCD_PHYS_BASE			(INTER_REGS_PHYS_BASE | 0xE0000)

#define AXP_NFC_PHYS_BASE	(INTER_REGS_PHYS_BASE | 0xD0000)

#define SDRAM_OPERATION_REG		(INTER_REGS_BASE | 0x1418)
#define SDRAM_CONFIG_REG		(INTER_REGS_BASE | 0x1400)
#define SDRAM_DLB_EVICT_REG		(INTER_REGS_BASE | 0x170C)

#define AXP_UART_PHYS_BASE(port)	(INTER_REGS_PHYS_BASE | 0x12000 + (port * 0x100))
#define DDR_VIRT_BASE			(INTER_REGS_BASE | 0x00000)
#define AXP_BRIDGE_VIRT_BASE		(INTER_REGS_BASE | 0x20000)
#define AXP_BRIDGE_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0x20000)
#define DDR_WINDOW_CPU_BASE		(DDR_VIRT_BASE | 0x1500)
#define AXP_SW_TRIG_IRQ			(AXP_BRIDGE_VIRT_BASE | 0x0A04)
#define AXP_SW_TRIG_IRQ_PHYS		(AXP_BRIDGE_PHYS_BASE | 0x0A04)
#define AXP_SW_TRIG_IRQ_CPU_TARGET_OFFS	8
#define AXP_SW_TRIG_IRQ_INITID_MASK	0x1F
#define AXP_PER_CPU_BASE		(AXP_BRIDGE_VIRT_BASE | 0x1000)
#define AXP_IRQ_VIRT_BASE		(AXP_PER_CPU_BASE)
#define AXP_CPU_INTACK			0xB4
#define AXP_IRQ_SEL_CAUSE_OFF		0xA0
#define AXP_IN_DOORBELL_CAUSE		0x78
#define AXP_IN_DRBEL_CAUSE			(AXP_PER_CPU_BASE | 0x78)
#define AXP_IN_DRBEL_MSK			(AXP_PER_CPU_BASE | 0x7c)

#ifdef CONFIG_MACH_ARMADA_XP_FPGA
#define AXP_CPU_RESUME_ADDR_REG(cpu)	(AXP_BRIDGE_VIRT_BASE | 0x984)
#else
#define AXP_CPU_RESUME_ADDR_REG(cpu)	(AXP_BRIDGE_VIRT_BASE | (0x2124+(cpu)*0x100))
#endif
#define AXP_CPU_RESUME_CTRL_REG		(AXP_BRIDGE_VIRT_BASE | 0x988)
#define AXP_CPU_RESET_REG(cpu)		(AXP_BRIDGE_VIRT_BASE | (0x800+(cpu)*8))
#define AXP_CPU_RESET_OFFS		0

#define AXP_L2_CLEAN_WAY_REG		(INTER_REGS_BASE | 0x87BC) 
#define AXP_L2_MNTNC_STAT_REG		(INTER_REGS_BASE | 0x8704)
#define AXP_SNOOP_FILTER_PHYS_REG	(INTER_REGS_PHYS_BASE | 0x21020)
#define AXP_REVISION_ID_PHYS_REG	(INTER_REGS_PHYS_BASE | 0x40008)
#define AXP_REVISION_ID_VIRT_REG	(INTER_REGS_BASE | 0x40008)
#endif

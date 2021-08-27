/*
 * Generic definitions for Marvell Armada MV88F6710 SoC
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

/*
 * Armada-XP address maps.
 *
 * phys		virt		size
 * e0000000	@runtime	128M	PCIe-0 Memory space
 * e8000000	@runtime	128M	PCIe-1 Memory space
 * f0000000	fab00000	16M	SPI-CS0 (Flash)
 * d0000000	fbb00000	1M	Internal Registers
 * f1100000	fbc00000	1M	PCIe-0 I/O space
 * f1200000	fbd00000	1M	PCIe-1 I/O space
 * f1b00000	fc600000	1M	DMA based UART
 * f2000000	fc700000	32M	Device-CS0 (NOR Flash)
 * f4000000	fe700000	1M	Boot-Device CS
 * f4100000	fe800000	1M	Device-CS1 (NOR Flash)
 * f4200000	fe900000	1M	Device-CS2 (NOR Flash)
 * f4300000	fea00000	1M	Device-CS3 (NOR Flash)
 * f4400000	feb00000	1M	CESA SRAM
 * f4600000	fed00000	1M	BootROM
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

/*
 * PEX Address Decoding
 * Virtual address not specified - remapped @runtime
 */
#define PEX0_MEM_PHYS_BASE		0xE0000000
#define PEX0_MEM_SIZE			_32M
#define PEX1_MEM_PHYS_BASE		0xE2000000
#define PEX1_MEM_SIZE			_32M

#define SPI_CS0_PHYS_BASE		0xF0000000
#define SPI_CS0_VIRT_BASE		0xFAB00000
#define SPI_CS0_SIZE			_16M

#define INTER_REGS_PHYS_BASE		0xF1000000
#define INTER_REGS_BASE			0xFBB00000

#define PEX0_IO_PHYS_BASE		0xF1100000
#define PEX0_IO_VIRT_BASE		0xFBC00000
#define PEX0_IO_SIZE			_1M
#define PEX1_IO_PHYS_BASE		0xF1200000
#define PEX1_IO_VIRT_BASE		0xFBD00000
#define PEX1_IO_SIZE			_1M

#define UART_REGS_BASE			0xF1B00000
#define UART_VIRT_BASE			0xFC600000
#define UART_SIZE			_1M

#define DEVICE_CS0_PHYS_BASE		0xF2000000
#define DEVICE_CS0_VIRT_BASE		0xFC700000
#define DEVICE_CS0_SIZE			_32M
#define DEVICE_BOOTCS_PHYS_BASE		0xF5000000
#define DEVICE_BOOTCS_VIRT_BASE		0xF5000000
#define DEVICE_BOOTCS_SIZE		_16M
#define DEVICE_CS1_PHYS_BASE		0xF4100000
#define DEVICE_CS1_VIRT_BASE		0xFE800000
#define DEVICE_CS1_SIZE			_1M
#define DEVICE_CS2_PHYS_BASE		0xF4200000
#define DEVICE_CS2_VIRT_BASE		0xFE900000
#define DEVICE_CS2_SIZE			_1M
#define DEVICE_CS3_PHYS_BASE		0xF4300000
#define DEVICE_CS3_VIRT_BASE		0xFEA00000
#define DEVICE_CS3_SIZE			_1M

#define CRYPT_ENG_PHYS_BASE(chan)	0xC8010000
#define CRYPT_ENG_VIRT_BASE(chan)	0xFEB00000
#define CRYPT_ENG_SIZE			_64K

#define XOR0_PHYS_BASE			(INTER_REGS_PHYS_BASE | 0x60800)
#define XOR1_PHYS_BASE			(INTER_REGS_PHYS_BASE | 0x60900)
#define XOR0_HIGH_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0x60A00)
#define XOR1_HIGH_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0x60B00)

#define BOOTROM_PHYS_BASE		0xFFF00000
#define BOOTROM_VIRT_BASE		0xFE700000
#define BOOTROM_SIZE			_1M

#define PMU_SCRATCH_PHYS_BASE		0xF4700000
#define PMU_SCRATCH_VIRT_BASE		0xFEE00000
#define PMU_SCRATCH_SIZE		_1M

#define LEGACY_NAND_PHYS_BASE		0xF4800000
#define LEGACY_NAND_VIRT_BASE		0xFEF00000
#define LEGACY_NAND_SIZE		_1M

#define AXP_NFC_PHYS_BASE	(INTER_REGS_PHYS_BASE | 0xD0000)

/*
 * Linux native definitiotns
 */
#define SDRAM_OPERATION_REG		(INTER_REGS_BASE | 0x1418)
#define SDRAM_CONFIG_REG		(INTER_REGS_BASE | 0x1400)
#define SDRAM_DLB_EVICT_REG		(INTER_REGS_BASE | 0x170C)

#define AXP_UART0_PHYS_BASE		(INTER_REGS_PHYS_BASE | 0x12000)
#define DDR_VIRT_BASE			(INTER_REGS_BASE | 0x00000)
#define DDR_WINDOW_CPU_BASE		(DDR_VIRT_BASE | 0x20000)
#define AXP_BRIDGE_VIRT_BASE		(INTER_REGS_BASE | 0x20000)
#define AXP_SW_TRIG_IRQ			(AXP_BRIDGE_VIRT_BASE | 0x0A04)
#define AXP_PER_CPU_BASE		(AXP_BRIDGE_VIRT_BASE | 0x1000)
#define AXP_IRQ_VIRT_BASE		(AXP_PER_CPU_BASE)
#define AXP_IRQ_SEL_CAUSE_OFF		0xA0
#define AXP_IN_DOORBELL_CAUSE		0x78
#define AXP_IN_DRBEL_MSK		(AXP_PER_CPU_BASE | 0x7c)
#define AXP_CPU_RESUME_CTRL_REG		(AXP_BRIDGE_VIRT_BASE | 0x988)
#define AXP_CPU_RESUME_ADDR_REG(cpu)	(AXP_BRIDGE_VIRT_BASE | (0x2124+(cpu)*0x100))
#define AXP_CPU_RESET_REG(cpu)		(AXP_BRIDGE_VIRT_BASE | (0x800+(cpu)*8))
#define AXP_L2_CLEAN_WAY_REG		(INTER_REGS_BASE | 0x87BC)
#define AXP_L2_MNTNC_STAT_REG		(INTER_REGS_BASE | 0x8704)
#define AXP_ASM_GPP_IRQ_CAUSE_REG	(INTER_REGS_BASE + 0x18110) 	/* level interrupts for gpp cause */
#define AXP_ASM_GPP_IRQ_MID_CAUSE_REG	(INTER_REGS_BASE + 0x18150) 	/* level interrupts for gpp mid cause */
#define AXP_ASM_GPP_IRQ_HIGH_CAUSE_REG	(INTER_REGS_BASE + 0x18190) 	/* level interrupts for gpp high cause */
#define AXP_ASM_GPP_IRQ_MASK_REG        (INTER_REGS_BASE + 0x1811c) 	/* level low mask */
#define AXP_ASM_GPP_IRQ_MID_MASK_REG	(INTER_REGS_BASE + 0x1815c)	/* level mid mask */
#define AXP_ASM_GPP_IRQ_HIGH_MASK_REG	(INTER_REGS_BASE + 0x1819c)	/* level high mask */
#define AXP_ASM_SOC_MAIN_ERR_CAUSE_REG	(INTER_REGS_BASE + 0x20A20)	/* SoC main error cause */
#define AXP_ASM_SOC_MAIN_ERR_MASK_REG	(INTER_REGS_BASE + 0x218C0)	/* SoC main error mask */

#endif

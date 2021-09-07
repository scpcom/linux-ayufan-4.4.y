#ifdef CONFIG_SYNO_QORIQ
/**
 * MPC85xx Internal Memory Map
 *
 * Authors: Jiang Yutang <b14898@freescale.com>
 *
 * Copyright 2010 Freescale Semiconductor, Inc
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This header file defines structures for various 85xx SOC devices that are
 * used by multiple source files.
 */

#ifndef __ASM_POWERPC_IMMAP_85XX_H__
#define __ASM_POWERPC_IMMAP_85XX_H__
#ifdef __KERNEL__

/* Global Utility Registers */
struct ccsr_guts {
	__be32	porpllsr;	/* 0x.0000 - POR PLL Ratio Status Register */
	__be32	porbmsr;	/* 0x.0004 - POR Boot Mode Status Register */
	u8	res1[0xc - 0x8];
	__be32	pordevsr;	/* 0x.000c - POR Device Status Register */
	__be32	pordbgmsr;	/* 0x.0010 - POR Debug Mode Status Register */
	__be32  pordevsr2;	/* 0x.0014 - POR Device Status Register 2 */
	u8	res2[0x20 - 0x18];
	__be32	gpporcr;	/* 0x.0020 - general-purpose POR Configuration
				   Register */
	u8	res3[0x60 - 0x24];
	__be32	pmuxcr;		/* 0x.0060 - Alternate Function Signal
				   Multiplex Control */
#define CCSR_GUTS_PMUXCR_SDT_S		0xa
#define CCSR_GUTS_PMUXCR_SDT_M		0x3
#define CCSR_GUTS_PMUXCR_SDT_SSI	0x0
#define CCSR_GUTS_PMUXCR_SDT_DMA	0x1
#define CCSR_GUTS_PMUXCR_SDT_TDM	0x2
#define CCSR_GUTS_PMUXCR_SDT_GPIO	0x3
	__be32  pmuxcr2;	/* 0x.0064 - Alternate Function Signal
				   Multiplex Control */
	__be32  dmuxcr;		/* 0x.0068 - DMA Mux Control Register */
#define CCSR_GUTS_DMUXCR_D1C0_S	0x1e
#define CCSR_GUTS_DMUXCR_D1C1_S	0x1c
#define CCSR_GUTS_DMUXCR_D1C2_S	0x1a
#define CCSR_GUTS_DMUXCR_D1C3_S	0x18
#define CCSR_GUTS_DMUXCR_D2C0_S	0x16
#define CCSR_GUTS_DMUXCR_D2C1_S	0x14
#define CCSR_GUTS_DMUXCR_D2C2_S	0x12
#define CCSR_GUTS_DMUXCR_D2C3_S	0x10
#define CCSR_GUTS_DMUXCR_DC_M	0x3
#define CCSR_GUTS_DMUXCR_DC_NC0	0x0
#define CCSR_GUTS_DMUXCR_DC_SSI	0x1
#define CCSR_GUTS_DMUXCR_DC_PAD	0x1
#define CCSR_GUTS_DMUXCR_DC_NC1	0x3
	u8	res4[0x70 - 0x6c];
	__be32	devdisr;	/* 0x.0070 - Device Disable Control */
	u8	res5[0x7c - 0x74];
	__be32  pmjcr;		/* 0x.007c - Power Management Jog Control
				   Register */
	__be32	powmgtcsr;	/* 0x.0080 - Power Management Status and
				   Control Register */
	__be32  pmrccr;		/* 0x.0084 - Power Management Reset Counter
				   Configuration Register */
	__be32  pmpdccr;	/* 0x.0088 - Power Management Power Down
				   Counter Configuration Register */
	__be32  pmcdr;		/* 0x.008c - Power Management Clock Disable
				   Register */
	__be32	mcpsumr;	/* 0x.0090 - Machine Check Summary Register */
	__be32	rstrscr;	/* 0x.0094 - Reset Request Status and Control
				   Register */
	__be32  ectrstcr;	/* 0x.0098 - Exception Reset Control Register*/
	__be32  autorstsr;	/* 0x.009c - Automatic Reset Status Register */
	__be32	pvr;		/* 0x.00a0 - Processor Version Register */
	__be32	svr;		/* 0x.00a4 - System Version Register */
	u8	res6[0xB0 - 0xA8];
	__be32	rstcr;		/* 0x.00b0 - Reset Control Register */
	u8	res7[0xC0 - 0xB4];
	__be32	iovselsr;	/* 0x.00c0 - IO Voltage Select Status
				   Register */
	u8	res8[0x224 - 0xC4];
	__be32	iodelay1;	/* 0x.0224 - IO Delay Control Register 1 */
	__be32  iodelay2;	/* 0x.0228 - IO Delay Control Register 2 */
	u8	res9[0x800 - 0x22c];
	__be32  clkdvdr;	/* 0x.0800 - Clock Divide Register */
	u8	res10[0xb28 - 0x804];
	__be32	ddrclkdr;	/* 0x.0b28 - DDR Clock Disable Register */
	u8	res11[0xc00 - 0xB2c];
	__be32	esr;		/* 0x.0c00 - Error Summary Register */
	u8	res12[0xe00 - 0xc04];
	__be32	clkocr;		/* 0x.0e00 - Clock Out Select Register */
	u8	res13[0xe20 - 0xe04];
	__be32	ecmcr;		/* 0x.0e20 - ECM control register */
	__be32	cpfor;		/* 0x.0e24 - L2 Charge Pump Fuse Override
				   Register */
	u8	res14[0xf2c - 0xE28];
	__be32	itcr;		/* 0x.0f2c - Internal Transaction Control
				   Register */
	u8	res15[0x1000 - 0xf30];
} __attribute__ ((packed));

#endif /* __ASM_POWERPC_IMMAP_85XX_H__ */
#endif /* __KERNEL__ */
#endif /* CONFIG_SYNO_QORIQ */

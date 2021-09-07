#ifdef CONFIG_SYNO_QORIQ
/*
 * Freescale board control FPGA.
 *
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Mingkai Hu <Mingkai.hu@freescale.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#ifndef __PPC_FSL_PIXIS_H
#define __PPC_FSL_PIXIS_H

#include <linux/types.h>

/* pixis command */
#define PX_CMD_START	0x1
#define PX_CMD_STOP		0x0
#define PX_CMD_SLEEP	0x2

/* data format */
#define VOLT_FMT		0x00
#define CURR_FMT		0x01
#define TEMP_FMT		0x02

/* status bit */
#define PXOC_MSG		(0x01 << 0)
#define PXMA_ERR		(0x01 << 1)
#define PXMA_ACK		(0x01 << 0)

/* micro define */
#define DATA_ADDR		0x80	/* data address in OCM SRAM */
#define DIVIDE_FACTOR   1000    /* support the calculation */
#define REC_NUM			9		/* record number to collect */

/* OCM Message Codes. */
#define OM_END			0x00
#define OM_SETDLY		0x01
#define OM_RST0			0x02
#define OM_RST1			0x03
#define OM_CHKDLY		0x04
#define OM_PWR			0x05
#define OM_WAKE			0x07
#define OM_GETMEM		0x08
#define OM_SETMEM		0x09
#define OM_SCLR			0x10
#define OM_START		0x11
#define OM_STOP			0x12
#define OM_GET			0x13
#define OM_ENABLE		0x14
#define OM_TIMER		0x15
#define OM_SETV			0x30
#define OM_INFO			0x31

struct pixis_reg {
	u8	id;			/* 0x00 - System ID register */
	u8	arch;		/* 0x01 - System architecture version register */
	u8	scver;		/* 0x02 - ngPIXIS version register */
	u8	ctl;		/* 0x03 - General control register */
	u8	rst;		/* 0x04 - Reset control register */
	u8	stat;		/* 0x05 - General status register */
	u8	aux;		/* 0x06 - Auxiliary register */
	u8	spd;		/* 0x07 - Speed register */
	u8	cfg0;		/* 0x08 - Board Configuration register 0 */
	u8	cfg1;		/* 0x09 - Board configuration register 1 */
	u8	addr;		/* 0x0A - SRAM address register */
	u8	res1[2];	/* 0x0B - 0x0C reserved */
	u8	data;		/* 0x0D - SRAM data register */
	u8	led;		/* 0x0E - LED Data Register */
	u8	tagd;		/* 0x0F - Tag register */
	u8	vctl;		/* 0x10 - VELA control register */
	u8	vstat;		/* 0x11 - VELA status register */
	u8	vcfgen0;	/* 0x12 - VELA control enable register 0 */
	u8	rrsn;		/* 0x13 - Rstrsn register */
	u8	ocmd;		/* 0x14 - OCMCSR register */
	u8	omsg;		/* 0x15 - OCMMSG register */
	u8	gmdbg;		/* 0x16 - GMDBG register */
	u8	gmdd;		/* 0x17 - GMDD register */
	u8	mack;		/* 0x18 - OCM ack register */
	u8	sclk0;		/* 0x19 - SCLK0 register */
	u8	sclk1;		/* 0x1A - SCLK1 register */
	u8	sclk2;		/* 0x1B - SCLK2 register */
	u8	dclk0;		/* 0x1C - DCLK0 register */
	u8	dclk1;		/* 0x1D - DCLK1 register */
	u8	dclk2;		/* 0x1E - DCLK2 register */
	u8	watch;		/* 0x1F - WATCH register */
	u8	sw1;		/* 0x20 - SW1 register */
	u8	en1;		/* 0x21 - EN1 register */
	u8	sw2;
	u8	en2;
	u8	sw3;
	u8	en4;
	u8	sw5;
	u8	en5;
	u8	sw6;
	u8	en6;
	u8	sw7;
	u8	en7;
	u8	sw8;
	u8	en8;
} __attribute__ ((packed));

struct crecord {
	u16	curr;		/* current value */
	u16	max;		/* max value */
	u16	qty1;		/* sample number  */
	u8	qty2;
	u32	acc;		/* accumulate values */
} __attribute__((packed));

struct fsl_pixis {
	struct pixis_reg __iomem *base;
	struct crecord rec[REC_NUM];
	u32	pm_cmd;
	char mode[10];
};

#ifdef CONFIG_FSL_PIXIS
int pixis_start_pm_sleep(void);
int pixis_stop_pm_sleep(void);
int pmbus_2volt(int);
int pmbus_2cur(int);
#else
static inline int pixis_start_pm_sleep(void) { return 0; }
static inline int pixis_stop_pm_sleep(void) { return 0; }
static inline int pmbus_2volt(int a) { return 0; }
static inline int pmbus_2cur(int a) { return 0; }
#endif

#endif
#endif /* CONFIG_SYNO_QORIQ */

/*
 * Copyright (C) 2005-2009 Freescale Semiconductor, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef PCI_AGENT_LIB_H
#define PCI_AGENT_LIB_H

#define PCI_VENDOR_ID_FREESCALE	0x1957
#define PCI_VENDOR_ID_MOTOROLA 0x1057
#define	PCI_DEVICE_ID_MPC8568E	0x0020
#define	PCI_DEVICE_ID_MPC8536E	0x0050

#define PPC85XX_NETDRV_NAME	"Boardnet: PPC85xx PCI Agent Demo Driver"
#define DRV_VERSION		"2.0"
#define PFX PPC85XX_NETDRV_NAME ": "

/* For inbound window */
#define PIWAR_EN		0x80000000
#define PIWAR_TRGT_MEM		0x00f00000
#define PIWAR_RTT_SNOOP		0x00050000
#define PIWAR_WTT_SNOOP		0x00005000

/* These are the flags in the message register */
/* tx, rx and device flags */
#define	AGENT_SENT		0x00000001
#define	AGENT_GET		0x00000000
#define	HOST_SENT		0x00000001
#define HOST_GET		0x00000000
#define	DEV_TBUSY		0x00000001

#define SHARE_FLAG		0xA367CF5A

#define AGENT_UP		0x2
#define AGENT_DOWN		0x4
#define HOST_UP			0x8
#define HOST_DOWN		0x10

/*Define one page for share memory*/
#define NEED_LOCAL_PAGE		0

/*The agent defined 4K mem for sharing*/
#define	AGENT_MEM_SIZE	((NEED_LOCAL_PAGE + 1) * PAGE_SIZE)

#endif

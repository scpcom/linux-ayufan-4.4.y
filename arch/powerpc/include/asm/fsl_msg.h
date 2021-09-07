#ifdef CONFIG_SYNO_QORIQ
/*
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * Author: Jason Jin <Jason.jin@freescale.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */

#ifndef _ASM_FSL_MSG_H
#define _ASM_FSL_MSG_H

#include <linux/types.h>

struct fsl_msg_unit {
	unsigned int irq;
	unsigned int msg_num;

	struct fsl_msg *fsl_msg;
	bool requested;
	u32 msg_group_addr_offset;

	u32 __iomem *msg_addr;
	u32 __iomem *mer;
	u32 __iomem *msr;
};

extern struct fsl_msg_unit *fsl_get_msg_unit(void);
extern void fsl_release_msg_unit(struct fsl_msg_unit *msg);
extern void fsl_clear_msg(struct fsl_msg_unit *msg);
extern void fsl_enable_msg(struct fsl_msg_unit *msg);
extern void fsl_msg_route_int_to_irqout(struct fsl_msg_unit *msg);
extern void fsl_send_msg(struct fsl_msg_unit *msg, u32 message);
extern void fsl_read_msg(struct fsl_msg_unit *msg, u32 *message);

#define FSL_NUM_MPIC_MSGS 4

#endif /* _ASM_FSL_MSG_H */
#endif /* CONFIG_SYNO_QORIQ */

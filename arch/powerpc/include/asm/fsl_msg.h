#ifdef CONFIG_SYNO_QORIQ
 
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

#endif  
#endif  

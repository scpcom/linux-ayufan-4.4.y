/*******************************************************************************
   Copyright (C) Marvell International Ltd. and its affiliates

   This software file (the "File") is owned and distributed by Marvell
   International Ltd. and/or its affiliates ("Marvell") under the following
   alternative licensing terms.  Once you have made an election to distribute the
   File under one of the following license alternatives, please (i) delete this
   introductory statement regarding license alternatives, (ii) delete the two
   license alternatives that you have not elected to use and (iii) preserve the
   Marvell copyright notice above.

********************************************************************************
   Marvell GPL License Option

   If you received this File from Marvell, you may opt to use, redistribute and/or
   modify this File in accordance with the terms and conditions of the General
   Public License Version 2, June 1991 (the "GPL License"), a copy of which is
   available along with the File in the license.txt file or by writing to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
   on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

   THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
   WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
   DISCLAIMED.  The GPL License provides additional details about this warranty
   disclaimer.
*******************************************************************************/

#include "mvTypes.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "mvIpc.h"
#include "mvOs.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <asm/irq_regs.h>

#include "mvDebug.h"
#include "mvCommon.h"
#include "mvIpc.h"
#include "mv_ipc_poll.h"
#include "mv_ipc_common.h"
#include "mv_ipc_node.h"
#include "include/mach/smp.h"

/*#define IPC_DRV_DEBUG*/
#ifdef IPC_DRV_DEBUG
#define ipc_debug       printk
#else
#define ipc_debug(x ...)
#endif

static MV_U8 **enabledChannels;
struct timer_list poll_timer;
static void do_ipc_rx_poll(unsigned long);

/***********************************************************************************
 * mvIpcPollInit
 *
 * DESCRIPTION:
 *              Init the structures
 *
 * INPUT:
 *		None
 * OUTPUT:
 *       None
 * RETURN:
 *		None
 *
 ************************************************************************************/
MV_VOID mvIpcPollInit(MV_U32 link)
{
	MV_U32 firstRunFlag = 0;

	if (NULL == enabledChannels) {
		enabledChannels = mvOsMalloc(sizeof(MV_U8 *) * mvIpcGetNumOfLinks());
		firstRunFlag = 1;
	}
	enabledChannels[link] = mvOsMalloc(sizeof(MV_U8) * mvIpcChnNumMax());
	mvOsMemset(enabledChannels[link], '\0', mvIpcChnNumMax());

	/*If passive polling mode, do not start timer event*/
	if (mvIpcGetHwLayerId(link) == MV_IPC_HW_LAYER_POLLING_PASSIVE)
		return;

	/*If timer event started before, do not start timer event*/
	if (firstRunFlag == 0)
		return;

	/*Start timer event*/
	init_timer(&poll_timer);
	poll_timer.function = do_ipc_rx_poll;
	poll_timer.expires = jiffies + MV_IPC_POLL_PERIOD;
	add_timer(&poll_timer);

	/*TODO this timer mechanism working in 10ms resolution,
	may be not good for real application*/

	return;
}

/***********************************************************************************
 * mvIpcSendTriggerPoll
 *
 * DESCRIPTION:
 *              Trigger placeholder for polling mode, do nothing
 *
 * INPUT:
 *		cpuId - the id of the target CPU
 *		chnId - The channel ID
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_VOID mvIpcSendTriggerPoll(MV_U32 linkId, MV_U32 chnId)
{
	return;
}

/***********************************************************************************
 * mvIpcEnableChnRx
 *
 * DESCRIPTION:
 *		Unmasks the doorbell for the given channel
 *
 * INPUT:
 *		irq - number of irq/doorbell to unmask
 * OUTPUT:
 *       None
 * RETURN:
 *		MV_OK or MV_ERROR
 *
 ************************************************************************************/
MV_VOID registerChnInISRPoll(MV_U32 linkId, MV_U32 chnId, MV_BOOL enable)
{
	if (MV_TRUE == enable)
		enabledChannels[linkId][chnId] = 1;
	else
		enabledChannels[linkId][chnId] = 0;
}

/*******************************************************************************
 * do_ipc_rx_poll()                                                            *
 *  rx polling service routine                                                 *
 ******************************************************************************/
void do_ipc_rx_poll(unsigned long data)
{
	MV_U32 linkId, chnId;
	int read_msgs = IPC_RX_MAX_MSGS_PER_ISR;

	ipc_debug(KERN_INFO "IPC: RX polling");

	/*NOTE:
		This ISR may be customised by user application requerements to make it more efficient*/

	/* Scan all rx flags*/
	for (linkId = 0; linkId < mvIpcGetNumOfLinks(); linkId++) {
		for (chnId = 0; chnId < mvIpcChnNum(linkId); chnId++) {
			/*Check if RX flag raised*/
			if ((enabledChannels[linkId][chnId] == 1) &&
			    (mvIpcRxMsgFlagCheck(linkId, chnId) == MV_TRUE)) {
				/*If ready ti RX, start get the messages*/
				ipc_debug(KERN_INFO "Got message in channel %d\n", chnId);
				while (read_msgs) {
					if (mvIpcRxMsg(linkId, chnId) != MV_OK)
						break;
					read_msgs--;
				}
			}
		}
	}

	if (read_msgs == IPC_RX_MAX_MSGS_PER_ISR)
		ipc_debug(KERN_WARNING "IPC: Polling loop with no messages\n");

	poll_timer.expires = jiffies + MV_IPC_POLL_PERIOD;
	add_timer(&poll_timer);
}

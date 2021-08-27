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

#ifndef __MV_IPC_H__
#define __MV_IPC_H__

#define ipc_attach_chn(chnId, cpu, ret) 	mvIpcAttachChannel(chnId, cpu, ret)
#define ipc_dettach_chn(chnId) 				mvIpcDettachChannel(chnId)
#define ipc_close_chn(chnId)		 		mvIpcCloseChannel(chnId)
#define ipc_tx_msg(chnId, msg)		 		mvIpcTxMsg(chnId, msg)
#define ipc_tx_ready(chnId)		 			mvIpcIsTxReady(chnId)
#define ipc_release_msg(chnId, msg)			mvIpcReleaseMsg(chnId, msg)

typedef int (*IPC_RX_CLBK)(MV_IPC_MSG *msg);

typedef struct __ipc_channel_info
{
	IPC_RX_CLBK  rxCallback;

} MV_IPC_CHN;

void* ipc_sh_malloc(unsigned int size);
void* ipc_virt_to_phys(void *virt_addr);
void* ipc_phys_to_virt(void *phys_addr);
int ipc_open_chn(int chnId, IPC_RX_CLBK rx_clbk);

#endif /* __MV_IPC_H__ */

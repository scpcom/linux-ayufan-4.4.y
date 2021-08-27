/*******************************************************************************
   Copyright (C) Marvell MV_U32ernational Ltd. and its affiliates

   This software file (the "File") is owned and distributed by Marvell
   MV_U32ernational Ltd. and/or its affiliates ("Marvell") under the following
   alternative licensing terms.  Once you have made an election to distribute the
   File under one of the following license alternatives, please (i) delete this
   MV_U32roductory statement regarding license alternatives, (ii) delete the two
   license alternatives that you have not elected to use and (iii) preserve the
   Marvell copyright notice above.

********************************************************************************
   Marvell Commercial License Option

   If you received this File from Marvell and you have entered MV_U32o a commercial
   license agreement (a "Commercial License") with Marvell, the File is licensed
   to you under the terms of the applicable Commercial License.

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
********************************************************************************
   Marvell BSD License Option

   If you received this File from Marvell, you may opt to use, redistribute and/or
   modify this File under the following licensing terms.
   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

*   Redistributions of source code must retain the above copyright notice,
		this list of conditions and the following disclaimer.

*   Redistributions in binary form must reproduce the above copyright
		notice, this list of conditions and the following disclaimer in the
		documentation and/or other materials provided with the distribution.

*   Neither the name of Marvell nor the names of its contributors may be
		used to endorse or promote products derived from this software without
		specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
   ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS MV_U32ERRUPTION) HOWEVER CAUSED AND ON
   ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
#ifndef __mv_ipc_common_h
#define __mv_ipc_common_h

#include "mv_ipc_node.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "mvIpc.h"
#include "mvOs.h"
#include "mv_ipc_doorbell.h"
#include "mv_ipc_os.h"

typedef struct __ipc_link_info_struct {
	MV_U32 numOfChannels;
	MV_BOOL isMaster;
	MV_VOID *shmemVirtAddr;
	MV_U32 shmemAddr;
	MV_U32 shmemSize;
} MV_IPC_LINK_INFO;

typedef struct __ipc_chn_info_struct {
	MV_U32 queueSize;
} MV_IPC_CHN_INFO;

/*Interrupt/Polling modes*/
enum {
	MV_IPC_HW_LAYER_POLLING_ACTIVE  = 0,
	MV_IPC_HW_LAYER_INTERRUPT_ISR,
	MV_IPC_HW_LAYER_POLLING_PASSIVE,
};

MV_U32 mvIpcWhoAmI(void);
MV_U32 mvIpcGetNumOfLinks(void);
MV_U32 mvIpcChnNum(MV_U32 link);
MV_U32 mvIpcChnNumMax(void);
MV_BOOL mvIpcGetlinkMaster(MV_U32 link);
MV_U32 mvIpcGetlinkRemoteNodeId(MV_U32 link);
MV_VOID *mvIpcGetShmemAddr(MV_U32 link);
MV_U32 mvIpcGetShmemBaseAddr(MV_U32 link);
MV_U32 mvIpcGetShmemSize(MV_U32 link);
MV_U32 mvIpcGetChnQueueSize(MV_U32 link, MV_U32 chn);
MV_U32 mvIpcGetHwLayerId(MV_U32 link);
MV_IPC_SEND_TRIGGER mvIpcGetChnTxHwPtr(MV_U32 link);
MV_IPC_RX_CHANNEL_REGISTER mvIpcGetChnRxHwPtr(MV_U32 link);
MV_U32 mvIpcGetFreeMemMasterPercent(MV_U32 link);
MV_STATUS mvIpcCommonInit(void);

#endif /*__mv_ipc_common_h*/

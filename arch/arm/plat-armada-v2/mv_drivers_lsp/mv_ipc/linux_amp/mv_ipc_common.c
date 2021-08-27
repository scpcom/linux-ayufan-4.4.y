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
#include "mvOs.h"

#include "mv_ipc_common.h"
#include "mv_ipc_node.h"
#include "mv_ipc_os.h"
#include "mvIpc.h"

MV_IPC_LINK_INFO mv_ipc_link_info_array[MV_IPC_LINKS_NUM] = MV_IPC_LINK_INFO_TABLE;
MV_IPC_CHN_INFO mv_ipc_chn_info_array[MV_IPC_LINKS_NUM][MV_IPC_CHN_NUM_MAX] =
	MV_IPC_CHN_INFO_TABLE;
MV_VOID *hwLayerAPI[MV_IPC_HW_LAYERS_NUM][2] = MV_IPC_HW_LAYER_API_TABLE;

MV_U32 mv_ipc_master_free_reg_percent[MV_IPC_LINKS_NUM] =
	MV_IPC_MASTER_FREE_REGION_PERCENT_TBL;

/*#define IPC_DRV_DEBUG*/

/*Return local node ID*/
MV_U32 mvIpcWhoAmI(void)
{
	if (CONFIG_MV_DRAM_BASE == 0x0)
		return 0;
	else
		return 1;
}

/*Return number of link in local node*/
MV_U32 mvIpcGetNumOfLinks(void)
{
	return MV_IPC_LINKS_NUM;
}

/*Return number of channels for the link*/
MV_U32 mvIpcChnNum(MV_U32 link)
{
	MV_IPC_LINK_INFO *str = &mv_ipc_link_info_array[link];

	return str->numOfChannels;
}

/*Return max number of channels for all links*/
MV_U32 mvIpcChnNumMax(void)
{
	return MV_IPC_CHN_NUM_MAX;
}

/*Return true if local node is a master for the link*/
MV_BOOL mvIpcGetlinkMaster(MV_U32 link)
{
/*	MV_IPC_LINK_INFO* str = &mv_ipc_link_info_array[link];
	return str->isMaster;*/

	if (CONFIG_MV_DRAM_BASE == 0x0)
		return MV_TRUE;
	else
		return MV_FALSE;
}

/*Return remote node ID for the link*/
MV_U32 mvIpcGetlinkRemoteNodeId(MV_U32 link)
{
/*	MV_IPC_LINK_INFO* str = &mv_ipc_link_info_array[link];
	return str->isMaster;*/

	if (CONFIG_MV_DRAM_BASE == 0x0)
		return 1;
	else
		return 0;
}

/*Return Shared memory base address*/
MV_VOID *mvIpcGetShmemAddr(MV_U32 link)
{
	return mvIpcOsGetVirtBase(link);
}

/*Return Shared memory Phys base size*/
MV_U32 mvIpcGetShmemBaseAddr(MV_U32 link)
{
	MV_IPC_LINK_INFO *str = &mv_ipc_link_info_array[link];

	return str->shmemAddr;
}

/*Return Shared memory Virtual base size*/
MV_U32 mvIpcGetShmemSize(MV_U32 link)
{
	MV_IPC_LINK_INFO *str = &mv_ipc_link_info_array[link];

	return str->shmemSize;
}

/*Return queue size*/
MV_U32 mvIpcGetChnQueueSize(MV_U32 link, MV_U32 chn)
{
	MV_IPC_CHN_INFO *str = &mv_ipc_chn_info_array[link][chn];

	return str->queueSize;
}

/*Return HW Layer ID */
MV_U32 mvIpcGetHwLayerId(MV_U32 link)
{
	return MV_IPC_HW_LAYER_ACTUAL;
}

/*Return pointer to Send Trigger function*/
MV_IPC_SEND_TRIGGER mvIpcGetChnTxHwPtr(MV_U32 link)
{
	return hwLayerAPI[mvIpcGetHwLayerId(link)][0];
}

/*Return pointer to Register channel function*/
MV_IPC_RX_CHANNEL_REGISTER mvIpcGetChnRxHwPtr(MV_U32 link)
{
	return hwLayerAPI[mvIpcGetHwLayerId(link)][1];
}

/*Return percent of free memory division between master and slave*/
MV_U32 mvIpcGetFreeMemMasterPercent(MV_U32 link)
{
	return mv_ipc_master_free_reg_percent[link];
}

/*Do init sequence*/
MV_STATUS mvIpcCommonInit(void)
{
	MV_U32 link;
	MV_STATUS status;

#ifdef IPC_DRV_DEBUG
	if (CONFIG_MV_DRAM_BASE == 0x0) {
		printk(KERN_INFO "---------------------Delay to async boot sequence----------\n");
		mvOsDelay(1000);
	}
#endif

	/* Initialize shared memory*/
	for (link = 0; link < mvIpcGetNumOfLinks(); link++)
		mvIpcOsSharedStack(link, mvIpcGetShmemBaseAddr(link), mvIpcGetShmemSize(link));

	for (link = 0; link < mvIpcGetNumOfLinks(); link++) {
		mvIpcDoorbellInit(link);
		mvIpcPollInit(link);
	}

	for (link = 0; link < mvIpcGetNumOfLinks(); link++) {
		status = mvIpcLinkStart(link);
		if (status != MV_OK) {
			printk(KERN_ERR "IPC: IPC HAL %d initialization failed\n", 0);
			return status;
		}
	}

	return MV_OK;
}

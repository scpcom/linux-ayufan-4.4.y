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

#ifndef __mv_ipc_node_h
#define __mv_ipc_node_h

#include "mvTypes.h"
#include "mv_ipc_doorbell.h"
#include "mv_ipc_poll.h"

/*NOTE:
	This files defines node configuration, the data used by mv_ipc_common
	to implement API for OS/HW/HAL layers.
	For AMP node part of data may by irrelevant,
	because single mv_ipc_node.h used, so configuration will differ in runtime*/

/*Unique ID of the IPC node, all nodes should be numbered sequentially from 0*/
#define MV_IPC_NODE_ID 0

/*Number of links established with other nodes*/
#define MV_IPC_LINKS_NUM 2

#define MV_IPC_CHN_NUM_MAX 8
/*Sizes of TX/RX queues for each channel (slave node doesnt know the sizes)*/
/*For slave the value will be ignored*/
#define MV_IPC_CHN_INFO_TABLE  { \
		{ { 16 }, { 32 }, { 16 }, { 32 } }, \
		{ { 16 }, { 32 }, { 16 }, { 32 }, { 16 }, { 32 }, { 16 }, { 32 } } \
}

/*Set HW Layer mode*/
/*#define MV_IPC_HW_LAYER_ACTUAL        MV_IPC_HW_LAYER_POLLING_ACTIVE*/
#define MV_IPC_HW_LAYER_ACTUAL  MV_IPC_HW_LAYER_INTERRUPT_ISR

#define MV_IPC_HW_LAYERS_NUM    2
/*HW layers function, send trigger is TX done signal,
	register channel is interrupt and channel enable function*/
#define MV_IPC_HW_LAYER_API_TABLE  { \
		{ mvIpcSendTriggerPoll, registerChnInISRPoll },         /*MV_IPC_HW_LAYER_POLLING_ACTIVE*/ \
		{ mvIpcSendDoorbell, registerChnInISRDoorbell },        /*MV_IPC_HW_LAYER_INTERRUPT_ISR*/ \
}

/*Link info array, set number of channels,
   master/slave. shmem phys adrress and size*/
/*numOfChn		isMaster, shmemAddr,		shmemSize*/
#define MV_IPC_LINK_INFO_TABLE { \
		{ 4,             MV_TRUE,        NULL,           0x60000000,             0x100000 }, \
		{ 8,             MV_TRUE,        NULL,           0x60100000,             0x100000 }, \
}

/*Division of free malloc area between master and slave*/
#define MV_IPC_MASTER_FREE_REGION_PERCENT_TBL {	\
		50, 50		\
}

#endif /*__mv_ipc_node_h*/

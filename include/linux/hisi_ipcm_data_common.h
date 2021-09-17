#ifndef __HISI_IPCM_DATA_COMMON_H__
#define __HISI_IPCM_DATA_COMMON_H__

#include "hisi_ipcm_proto.h"
#include "hios_type.h"

/*for debug*/
#define IPCM_DBG_LEVEL   (0x3)
#define IPCM_INFO_LEVEL  (IPCM_DBG_LEVEL)
#define IPCM_ERR_LEVEL   (IPCM_DBG_LEVEL + 1)

#define mcc_trace(level, s, params...) do {\
	if (level >= IPCM_ERR_LEVEL)\
		pr_info("[%s, %d]: " s "\n",\
			__func__, __LINE__, ## params);\
} while (0)

/*for byte align*/
#define DIGHT_NUM (4)
#define LEN_DIGHT(len) ((len + DIGHT_NUM - 1) /\
			DIGHT_NUM * DIGHT_NUM)

/*message head size*/
#define _IPCM_HEAD_SIZE  ((sizeof(struct hisi_ipcm_transfer_head))\
				/ DIGHT_NUM*DIGHT_NUM)

/* 1.*rp_addr 2.*wp_addr 3.*flag(irq or thread)*/
#define HISI_IPCM_SHARED_USEED_SIZE  (3 * sizeof(unsigned int)\
					+ _IPCM_HEAD_SIZE)

/*APIs*/
int hisi_ipcm_send_irq_data(const void *buf,
				unsigned int len,
				struct hisi_ipcm_transfer_head *head);
int hisi_ipcm_send_thread_data(struct hisi_ipc_transfer_handle *handle,
				const void *buf,
				unsigned int len,
				struct hisi_ipcm_transfer_head *head);
#endif

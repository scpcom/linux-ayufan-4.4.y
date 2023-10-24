/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULE_EXPT_H_
#define _MODULE_EXPT_H_

#include "types.h"
#include "channels.h"
#include "modules.h"
#include "module_Rx.h"
#include "module_qm.h"

#define SMI_EXPT0_BASE		0x0A000420
#define SMI_EXPT1_BASE		0x0A000440

#define FPP_SMI_CTRL		0x00
#define FPP_SMI_RXBASE		0x04
#define FPP_SMI_TXBASE		0x08
#define FPP_SMI_TXEXTBASE	0x0C

#define EXPT_BUF_HEADROOM	64

#define EXPT_DEFAULT_TXRATE	(100000 / 1000)
#define EXPT_TX_QMAX		64

#define EXPT_Q0 0
#define EXPT_NUM_QUEUES 4
#define EXPT_MAX_QUEUE  ((EXPT_NUM_QUEUES) - 1)

#define DSCP_MAX_VALUE 63

#define EXPT_WEIGTH 8
#define EXPT_BUDGET 16

#define EXPT_RX_EXT_DESC_MASK	 (\
	RX_STA_L4_CKSUM |\
	RX_STA_L4_GOOD |\
	RX_STA_L3_CKSUM |\
	RX_STA_L3_GOOD |\
	RX_STA_TCP	 |\
	RX_STA_UDP	 |\
	RX_STA_IPV6	 |\
	RX_STA_IPV4	 |\
	RX_STA_PPPOE	 |\
	0)


typedef struct tEXPT_context {
	U32 SMI_baseaddr;			// Virtual registers
	U32 Rx_Q_PTR;				// descriptors base address
	U32 Tx_Q_PTR;				// descriptors base address
	U32 Tx_Q_ExtPTR;				// extented descriptors (Ipsec) base address
	U32 rx_irqm;
	U32 tx_irqm;

	U32 Txtosend;
	U32 RxtoClean;
	int tx_nqueued;

	struct tDataQueue pktqueue[EXPT_NUM_QUEUES];
} EXPT_context, *PEXPT_context;

typedef struct tEXPT_globals {
	U32 LastTimer;
	int TxRateLimit;
	int TxBucket;
} EXPT_globals;

extern EXPT_globals gExptGlobals;


typedef struct _tExptQueueDSCPCommand {
	unsigned short queue ;
	unsigned short num_dscp;
	unsigned char dscp[64];
}ExptQueueDSCPCommand, *PExptQueueDSCPCommand;

typedef struct _tExptQueueCTRLCommand {
	unsigned short queue;
	unsigned short reserved;
}ExptQueueCTRLCommand, *PExptQueueCTRLCommand;

extern PEXPT_context gExptpCtx[MAX_PORTS];
extern volatile U8 Expt_tx_go[MAX_PORTS];
extern struct tEXPT_context gExptCtx[MAX_PORTS];

BOOL M_expt_init(PModuleDesc pModule);
void M_EXPT_process_packet(struct tMetadata *mtd);
void M_EXPT_rx_process_packet(struct tMetadata *mtd);
U16 M_expt_cmdproc(U16 cmd_code, U16 cmd_len, U16 *pcmd);
int M_expt_updateL2(struct tMetadata *mtd, U16 family, U8* itf_flags);
int M_expt_queue_reset(void);
void pfe_expt_queue_init(void);

void M_expt_tx_enable(U8 portid);
void M_expt_tx_disable(U8 portid);
void M_expt_rx_enable(U8 portid);
void M_expt_rx_disable(U8 portid);


#if defined(COMCERTO_2000_CLASS)
/** Setup an appropriate (TMU) queue value for packets going to the host.
 *
 * @param mtd pointer to packet metadata.
 */
static inline void M_EXPT_compute_queue(struct tMetadata *mtd)
{
	mtd->queue = get_tmu3_queue(mtd->input_port, mtd->queue > 0);
}

/** Prepare an HIF packet header.
 * MUST be called AFTER M_EXPT_compute_queue, so that mtd->queue is set properly.
 * @param mtd                          pointer to metadata of packet to send
 */
static inline u16 M_EXPT_prepare_hif_header(struct tMetadata *mtd)
{
	u32 client_id = mtd->input_port;
	u32 ctrl = 0;
	u16 hdr_len = 0;
	struct hif_ipsec_hdr *hif_ipsec;

	if (mtd->flags & MTD_RX_CHECKSUMMED)
		ctrl = HIF_CTRL_RX_CHECKSUMMED;

#ifdef WIFI_ENABLE
	if ( IS_WIFI_PORT(mtd->input_port) && (mtd->input_port == mtd->output_port))
	{
		ctrl |= HIF_CTRL_RX_WIFI_EXPT;
		ctrl |= __cpu_to_le32((mtd->input_port - WIFI0_PORT) << HIF_CTRL_VAPID_OFST);
		client_id = WIFI0_PORT;
	}
	else if (IS_WIFI_PORT(mtd->output_port))
	{
		ctrl |= __cpu_to_le32((mtd->output_port - WIFI0_PORT) << HIF_CTRL_VAPID_OFST);
		client_id = WIFI0_PORT;
	}
	//PRINTF("WiFi input:%d output:%d ctrl:%x\n", mtd->input_port, mtd->output_port, ctrl);
#endif

	if(mtd->flags & MTD_IPSEC_INBOUND_MASK)
	{
		ctrl |= HIF_CTRL_RX_IPSEC_IN;
		ctrl |= __cpu_to_le32(sizeof(struct hif_ipsec_hdr) << 24);

		hdr_len = sizeof(struct hif_ipsec_hdr);
		
		hif_ipsec = (struct hif_ipsec_hdr *)(mtd->data + mtd->offset - hdr_len);
		hif_ipsec->sa_handle[0] = mtd->priv_sa_handle[0];
		hif_ipsec->sa_handle[1] = mtd->priv_sa_handle[1];
	}

	hdr_len += hif_hdr_add(mtd->data + mtd->offset - hdr_len, client_id, get_hif_queue(mtd->queue, 0), ctrl);

	return hdr_len;
}
#endif



#endif /* _MODULE_EXPT_H_ */

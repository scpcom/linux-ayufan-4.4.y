/*
 *  Copyright (c) 2011 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _UTIL_MTD_H_
#define _UTIL_MTD_H_

#include "types.h"
#include "hal.h"
#include "util_rx.h"


#define NUM_MTDS		(32)
#define NUM_FRAG_MTDS		(NUM_FRAG4_Q*2*2)


typedef struct tMetadata {
	struct tMetadata *next;
	u16	length;
	u16	offset;
	u8	*data;

	void 	*pkt_start;
	void	*rx_dmem_end;

	u16	flags;
	u8	input_port;

	union {
		// FIXME -- is this right????  problem with c2k???
	  	struct  { U32 pad_double; U64 dword;} __attribute__((packed));
		U32	word[MTD_PRIV];
		struct socket_info {
			U16 id;
			U32 l4cksum;
			U32 l4offset;
		} socket;

		struct frag_info {
			U16 offset;
			U16 L4_offset;
			U32 l3_offset; //received from Class
			U16 end;
			U8 baseoff;
		} frag;

		 struct sec_path {
			U8  l2_len[SA_MAX_OP];
			U8  queue;
			U8  output_port;
                        U8  sec_outitf;
                        U8  sec_L4off;
                        U8  sec_L4_proto;
                        S8  sec_sa_op;
                        U16 sec_sa_h[SA_MAX_OP];
                } sec;
	
	} priv;

	//SAEntry	dmem_sa;
} Metadata, *PMetadata;

#define baseoff		priv.frag.baseoff
#define l3_offset	priv.frag.l3_offset

#define priv_sa_outitf	priv.sec.sec_outitf
#define priv_l2len	priv.sec.l2_len
#define priv_queue	priv.sec.queue
#define priv_outport	priv.sec.output_port
#define priv_sa_op	priv.sec.sec_sa_op
#define priv_sa_handle	priv.sec.sec_sa_h
#define priv_L4_offset	priv.sec.sec_L4off
#define priv_L4_proto	priv.sec.sec_L4_proto

extern Metadata g_util_mtd_table[];
extern Metadata g_util_frag_mtd_table[];
extern PMetadata gmtd_head;
extern PMetadata g_frag_mtd_head;

static __inline void mtd_init()
{
        int i;
        gmtd_head = NULL;
        for (i = 0; i < NUM_MTDS; i++)
        {
                g_util_mtd_table[i].next = gmtd_head;
                gmtd_head = &g_util_mtd_table[i];
        }
	/*TODO Is it a good idea to move to FPART model?? */
	g_frag_mtd_head = NULL;
	for(i = 0; i < NUM_FRAG_MTDS; i++)
	{
		g_util_frag_mtd_table[i].next = g_frag_mtd_head;
		g_frag_mtd_head = &g_util_frag_mtd_table[i];
	}
}

static __inline PMetadata mtd_alloc()
{
	PMetadata mtd = NULL;

	if (gmtd_head)
	{
		mtd = gmtd_head;
		gmtd_head = gmtd_head->next;
		mtd->next =NULL;
	}

//	mtd->pkt_start = NULL;
//	mtd->data = NULL;

	return (mtd);
}

static __inline PMetadata frag_mtd_alloc()
{
	PMetadata mtd = NULL;

	if (g_frag_mtd_head)
	{
		mtd = g_frag_mtd_head;
		g_frag_mtd_head = g_frag_mtd_head->next;
		mtd->next =NULL;
	}

	return (mtd);
}

static __inline int frag_mtd_free(PMetadata pMetaData)
{
	pMetaData->next = g_frag_mtd_head;
	g_frag_mtd_head = pMetaData;

	return 0;
}

static __inline int mtd_free(PMetadata pMetaData)
{
	if(pMetaData >= g_util_frag_mtd_table) {
		frag_mtd_free(pMetaData);
	}else {
		pMetaData->next = gmtd_head;
		gmtd_head = pMetaData;
	}

	return 0;
}

static __inline void free_packet(struct tMetadata *mtd)
{
	/* freeing jumbo frames buffers from ddr */
	/*fp_free_buffers(i);
	FIXME for jumbo frames with more than one DDR buffer, do we have to free them all ? */
#ifndef REVA_WA
	util_efet_wait(0);
#endif
	if (mtd->pkt_start)
	{
		efet_writel((u32)mtd->pkt_start & ~(DDR_BUF_SIZE - 1), BMU2_BASE_ADDR + BMU_FREE_CTRL);
		mtd->pkt_start = NULL;
	}

	mtd_free(mtd);

	PESTATUS_INCR_DROP();
}

#endif /* UTIL_MTD_H_ */

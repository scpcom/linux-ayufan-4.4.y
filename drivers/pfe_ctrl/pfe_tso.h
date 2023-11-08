#ifndef __PFE_TSO_
#define __PFE_TSO_

#define  PFE_TSO_STATS

struct tso_cb_s
{
	unsigned int tso_len_counters[32];
	/* This array is used to store the dma mapping for skb fragments */
	struct hif_frag_info_s 	dma_map_array[EMAC_TXQ_CNT][EMAC_TXQ_DEPTH];
};

unsigned int pfe_common_tso_to_desc(struct sk_buff *skb);
int pfe_common_tso( struct sk_buff *skb, struct hif_client_s *client, struct tso_cb_s *tso, int queuenum, u32 ctrl);
void pfe_common_skb_unmap( struct sk_buff *skb);
#endif

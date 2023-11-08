#ifndef _PFE_VWD_H_
#define _PFE_VWD_H_

#include <linux/cdev.h>
#include <linux/interrupt.h>
#include "pfe_tso.h"


#define PFE_VWD_LRO_STATS
#define PFE_VWD_NAPI_STATS
#define VWD_DEBUG_STATS
#define VWD_TXQ_CNT	16
#define VWD_TXQ_DEPTH	(HIF_TX_DESC_NT >> 1)
#define VWD_RXQ_CNT	3
#define VWD_RXQ_DEPTH	HIF_RX_DESC_NT /* make sure clients can receive a full burst of packets */


#define VWD_MINOR               0
#define VWD_MINOR_COUNT         1
#define VWD_DRV_NAME            "vwd"
#define VWD_DEV_COUNT           1
#define VWD_RX_POLL_WEIGHT	HIF_RX_POLL_WEIGHT - 16

struct vap_desc_s {
	struct	kobject *vap_kobj;
	struct net_device *dev;
	struct pfe_vwd_priv_s *priv;
	unsigned int   ifindex;
	struct hif_client_s client;
	struct net_device	dummy_dev;
	struct napi_struct	low_napi;
	struct napi_struct	high_napi;
	struct napi_struct	lro_napi;
	spinlock_t tx_lock[VWD_TXQ_CNT];
	struct sk_buff *skb_inflight[VWD_RXQ_CNT + 6];
	int cpu_id;
	unsigned char  ifname[12];
	unsigned char  macaddr[6];
	unsigned short vapid;
        unsigned short programmed;
        unsigned short bridged;
	unsigned short  direct_rx_path;          /* Direct path support from offload device=>VWD */
	unsigned short  direct_tx_path;          /* Direct path support from offload VWD=>device */
	unsigned short  custom_skb;   	      /* Customized skb model from VWD=>offload_device */
};



struct vap_cmd_s {
	int 		action;
#define 	ADD 	0
#define 	REMOVE	1
#define 	UPDATE	2
#define 	RESET	3
	int     	ifindex;
	unsigned short	vapid;
	unsigned short  direct_rx_path;
	unsigned char	ifname[12];
	unsigned char	macaddr[6];
};



struct pfe_vwd_priv_s {

	struct pfe 		*pfe;
	unsigned char name[12];

	struct cdev char_dev;
	int char_devno;

	struct vap_desc_s vaps[MAX_VAP_SUPPORT];
	int vap_count;
	int vap_programmed_count;
	int vap_bridged_count;
	spinlock_t vaplock;
	struct timer_list tx_timer;
	struct tso_cb_s 	tso;

#ifdef PFE_VWD_LRO_STATS
	unsigned int lro_len_counters[LRO_LEN_COUNT_MAX];
	unsigned int lro_nb_counters[LRO_NB_COUNT_MAX]; //TODO change to exact max number when RX scatter done
#endif
#ifdef PFE_VWD_NAPI_STATS
	unsigned int napi_counters[NAPI_MAX_COUNT];
#endif
	int fast_path_enable;
	int fast_bridging_enable;
	int fast_routing_enable;
	int tso_hook_enable;

#ifdef VWD_DEBUG_STATS
	u32 pkts_local_tx_sgs;
	u32 pkts_total_local_tx;
	u32 pkts_local_tx_csum;
	u32 pkts_transmitted;
	u32 pkts_slow_forwarded[VWD_RXQ_CNT];
	u32 pkts_tx_dropped;
	u32 pkts_rx_fast_forwarded[VWD_RXQ_CNT];
	u32 rx_skb_alloc_fail;
	u32 rx_csum_correct;
#endif

	u32 msg_enable;

};


int pfe_vwd_init(struct pfe *pfe);
void pfe_vwd_exit(struct pfe *pfe);

#endif /* _PFE_VWD_H_ */

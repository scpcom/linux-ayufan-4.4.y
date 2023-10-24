#ifndef _PFE_VWD_H_
#define _PFE_VWD_H_

#include <linux/cdev.h>
#include <linux/interrupt.h>


#define MAX_VAP_SUPPORT 2

#define VWD_TXQ_CNT	16
#define VWD_TXQ_DEPTH	(HIF_TX_DESC_NT >> 1)
#define VWD_RXQ_CNT	2
#define VWD_RXQ_DEPTH	HIF_RX_DESC_NT /* make sure clients can receive a full burst of packets */


#define VWD_MINOR               0
#define VWD_MINOR_COUNT         1
#define VWD_DRV_NAME            "vwd"
#define VWD_DEV_COUNT           1
#define VWD_RX_POLL_WEIGHT	HIF_RX_POLL_WEIGHT - 16

struct vap_desc_s {
	struct net_device *dev;
	unsigned int   ifindex;
	unsigned char  ifname[12];
	unsigned char  macaddr[6];
	unsigned short vapid;
        unsigned short programmed;
        unsigned short bridged;
};



struct vap_cmd_s {
	int 		action;
#define 	ADD 	0
#define 	REMOVE	1
#define 	UPDATE	2
	int     	ifindex;
	unsigned int	vapid;
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
	spinlock_t vwd_tx_lock[VWD_TXQ_CNT];
	struct hif_client_s client;
	struct net_device	dummy_dev;
	struct napi_struct	low_napi;
	struct napi_struct	high_napi;

	int fast_path_enable;
	int fast_bridging_enable;
	int fast_routing_enable;

	u32 pkts_transmitted;
	u32 pkts_slow_forwarded[VWD_RXQ_CNT];
	u32 pkts_tx_dropped;
	u32 pkts_rx_fast_forwarded[VWD_RXQ_CNT];
	u32 rx_skb_alloc_fail;

	u32 msg_enable;

};


int pfe_vwd_init(struct pfe *pfe);
void pfe_vwd_exit(struct pfe *pfe);

#endif /* _PFE_VWD_H_ */

#ifndef _PFE_PCAP_H_
#define _PFE_PCAP_H_



#define  COMCERTO_CAP_RX_DESC_NT       (1024)
#define  COMCERTO_CAP_DFL_RATELIMIT     10000  //pps
#define  COMCERTO_CAP_MIN_RATELIMIT     1000 //pps
#define  COMCERTO_CAP_MAX_RATELIMIT     800000 //pps
#define  COMCERTO_CAP_DFL_BUDGET        32  //packets processed in tasklet
#define  COMCERTO_CAP_MAX_BUDGET        64
#define  COMCERTO_CAP_POLL_MS           100

int pfe_pcap_init(struct pfe *pfe);
void pfe_pcap_exit(struct pfe *pfe);

#define PCAP_RXQ_CNT	1
#define PCAP_TXQ_CNT	1

#define PCAP_RXQ_DEPTH	1024
#define PCAP_TXQ_DEPTH	1

#define PCAP_RX_POLL_WEIGHT (HIF_RX_POLL_WEIGHT - 16)


typedef struct pcap_priv_s {
	struct pfe* 		pfe;
	unsigned char		name[12];

	struct net_device_stats stats[NUM_GEMAC_SUPPORT];
	struct net_device       *dev[NUM_GEMAC_SUPPORT];
	struct hif_client_s 	client;
	u32                     rate_limit;
	u32                     pkts_per_msec;
	struct net_device       dummy_dev;
	struct napi_struct      low_napi;
}pcap_priv_t;


#endif /* _PFE_PCAP_H_ */

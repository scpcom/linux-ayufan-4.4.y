#ifndef _PFE_CTRL_H_
#define _PFE_CTRL_H_

#include <linux/dmapool.h>

#include "config.h"
#include "pfe/pfe.h"

#define DMA_BUF_SIZE_256	0x100	/* enough for 2 conntracks, 1 bridge entry or 1 multicast entry */
#define DMA_BUF_SIZE_512	0x200	/* 512bytes dma allocated buffers used by rtp relay feature */
#define DMA_BUF_MIN_ALIGNMENT	8
#define DMA_BUF_BOUNDARY	(4 * 1024) /* bursts can not cross 4k boundary */

#define CMD_TX_ENABLE	0x0501
#define CMD_TX_DISABLE	0x0502

#define CMD_RX_LRO		0x0011
#define CMD_PKTCAP_ENABLE       0x0d01
#define CMD_QM_EXPT_RATE	0x020c

#define EXPT_TYPE_PCAP		0x3

struct pfe_ctrl {
	struct mutex mutex;
	spinlock_t lock;

	void *dma_pool;
	void *dma_pool_512;

	struct device *dev;

	void *hash_array_baseaddr;		/** Virtual base address of the conntrack hash array */
	unsigned long hash_array_phys_baseaddr; /** Physical base address of the conntrack hash array */

	struct task_struct *timer_thread;
	struct list_head timer_list;
	unsigned long timer_period;

	int (*event_cb)(u16, u16, u16*);

	unsigned long sync_mailbox_baseaddr[MAX_PE]; /* Sync mailbox PFE internal address, initialized when parsing elf images */
	unsigned long msg_mailbox_baseaddr[MAX_PE]; /* Msg mailbox PFE internal address, initialized when parsing elf images */

	unsigned long class_dmem_sh;
	unsigned long class_pe_lmem_sh;
	unsigned long tmu_dmem_sh;
	unsigned long util_dmem_sh;
	unsigned long util_ddr_sh;
	struct clk *clk_axi;
	unsigned int sys_clk;			// AXI clock value, in KHz
	void *ipsec_lmem_baseaddr;
	unsigned long ipsec_lmem_phys_baseaddr;
	
	/* used for asynchronous message transfer to PFE */
	struct list_head msg_list;
	struct work_struct work;	
};

int pfe_ctrl_init(struct pfe *pfe);
void pfe_ctrl_exit(struct pfe *pfe);

int pe_sync_stop(struct pfe_ctrl *ctrl, int pe_mask);
void pe_start(struct pfe_ctrl *ctrl, int pe_mask);
int pe_request(struct pfe_ctrl *ctrl, int id,unsigned short cmd_type, unsigned long dst, unsigned long src, int len);
int pe_read(struct pfe_ctrl *ctrl, int id, u32 *dst, unsigned long src, int len, int clear_flag);
int tmu_pe_request(struct pfe_ctrl *ctrl, int id, unsigned int tmu_cmd_bitmask);

int pfe_ctrl_set_eth_state(int id, unsigned int state, unsigned char *mac_addr);
int pfe_ctrl_set_lro(char enable);
#ifdef CFG_PCAP
int pfe_ctrl_set_pcap(char enable);
int pfe_ctrl_set_pcap_ratelimit(u32 pkts_per_msec);
#endif

int relax(unsigned long end);

/* used for asynchronous message transfer to PFE */
#define FPP_MAX_MSG_LENGTH	256 /* expressed in U8 -> 256 bytes*/
struct fpp_msg {
        struct list_head list;
        void (*callback)(unsigned long, int, u16, u16 *);
        unsigned long data;
        u16 fcode;
        u16 length;
        u16 *payload;
};

#endif /* _PFE_CTRL_H_ */

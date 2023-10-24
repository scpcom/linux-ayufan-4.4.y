#ifndef _PFE_CTRL_H_
#define _PFE_CTRL_H_

#include <linux/dmapool.h>

#include "pfe/pfe.h"

#define DMA_BUF_SIZE_256	0x100	/* enough for 2 conntracks, 1 bridge entry or 1 multicast entry */
#define DMA_BUF_SIZE_512	0x200	/* 512bytes dma allocated buffers used by rtp relay feature */
#define DMA_BUF_MIN_ALIGNMENT	8
#define DMA_BUF_BOUNDARY	(4 * 1024) /* bursts can not cross 4k boundary */

#define CMD_TX_ENABLE	0x0501
#define CMD_TX_DISABLE	0x0502

#define CMD_RX_LRO		0x0011

struct pfe_ctrl {
	struct mutex mutex;

	struct dma_pool *dma_pool;
	struct dma_pool *dma_pool_512;

	struct device *dev;

	void *null_ct;
	dma_addr_t null_ct_dma_handle;

	void *hash_array_baseaddr;		/** Virtual base address of the conntrack hash array */
	unsigned long hash_array_phys_baseaddr; /** Physical base address of the conntrack hash array */

	struct task_struct *timer_thread;
	struct list_head timer_list;
	unsigned long timer_period;

	void (*event_cb)(u16, u16, u16*);

	unsigned long sync_mailbox_baseaddr[MAX_PE]; /* Sync mailbox PFE internal address, initialized when parsing elf images */
	unsigned long msg_mailbox_baseaddr[MAX_PE]; /* Msg mailbox PFE internal address, initialized when parsing elf images */

	unsigned long class_dmem_sh;
	unsigned long class_pe_lmem_sh;
	unsigned long tmu_dmem_sh;
	unsigned long util_dmem_sh;
	unsigned long util_ddr_sh;
	struct clk *clk_axi;
	unsigned int sys_clk;			// AXI clock value, in KHz
	unsigned long ipsec_lmem_baseaddr;
	unsigned long ipsec_lmem_phys_baseaddr;
};

int pfe_ctrl_init(struct pfe *pfe);
void pfe_ctrl_exit(struct pfe *pfe);

int pe_sync_stop(struct pfe_ctrl *ctrl, int pe_mask);
void pe_start(struct pfe_ctrl *ctrl, int pe_mask);
int pe_request(struct pfe_ctrl *ctrl, int id,unsigned short cmd_type, unsigned long dst, unsigned long src, int len);
int tmu_pe_request(struct pfe_ctrl *ctrl, int id, unsigned int tmu_cmd_bitmask);

int pfe_ctrl_set_eth_state(int id, unsigned int state, unsigned char *mac_addr);
int pfe_ctrl_set_lro(char enable);

#endif /* _PFE_CTRL_H_ */

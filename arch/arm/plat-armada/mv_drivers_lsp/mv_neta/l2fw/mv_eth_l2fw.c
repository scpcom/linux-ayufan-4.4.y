/* mv_eth_l2fw.c */
#include <linux/ctype.h>

#include "xor/mvXor.h"
#include "xor/mvXorRegs.h"
#include "mv_hal_if/mvSysXorApi.h"

#include "mvOs.h"
#include "mv_eth_l2fw.h"
#include "mv_neta/net_dev/mv_netdev.h"
#include "gbe/mvNeta.h"
#include "gbe/mvNetaRegs.h"

#include "mv_eth_l2fw.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#ifdef CONFIG_MV_ETH_L2SEC
extern int cesa_init(void);
extern MV_STATUS handleEsp(struct eth_pbuf *pkt, struct neta_rx_desc *rx_desc,
							struct eth_port  *new_pp, int inPort);
#endif

int espEnabled = 0;

struct eth_pbuf *mv_eth_pool_get(struct bm_pool *pool);

static int mv_eth_ports_l2fw_num;

static L2FW_RULE **l2fw_hash = NULL;

#define	L2FW_HASH_MASK   (L2FW_HASH_SIZE - 1)

static MV_U32 l2fw_jhash_iv;

static int numHashEntries;

struct eth_port_l2fw **mv_eth_ports_l2fw;
static inline int       mv_eth_l2fw_rx(struct eth_port *pp, int rx_todo, int rxq);
static inline MV_STATUS mv_eth_l2fw_tx(struct eth_pbuf *pkt, struct eth_port *pp,
					   int withXor, struct neta_rx_desc *rx_desc);


void printBufVirtPtr(MV_BUF_INFO *pBuf)
{
	int i;
	if (pBuf->bufVirtPtr == NULL) {
		printk(KERN_INFO "pBuf->bufVirtPt==NULL in %s\n", __func__);
		return;
	}
	for (i = 0; i < 40; i++) {
		printk(KERN_INFO "KERN_INFO [%d]=%x ", i, pBuf->bufVirtPtr[i]);
		if (!(i%10) && i > 1)
			printk(KERN_INFO "\n");
	}
	printk(KERN_INFO "\n****************** %s\n", __func__);

}
void printBufInfo(MV_BUF_INFO *pbuf)
{
	printk(KERN_INFO "bufSize=%d\n"      , pbuf->bufSize);
	printk(KERN_INFO "dataSize=%d\n"     , pbuf->dataSize);
	printk(KERN_INFO "memHandle=%d\n"    , pbuf->memHandle);
	printk(KERN_INFO "bufAddrShift=%d\n" , pbuf->bufAddrShift);
	printk(KERN_INFO "*****************************************\n\n");

}


static s32 atoi(char *psz_buf)
{
	char *pch = psz_buf;
	s32 base = 0;
	unsigned long res;
	int ret_val;

	while (isspace(*pch))
			pch++;

	if (*pch == '-' || *pch == '+') {
			base = 10;
			pch++;
	} else if (*pch && tolower(pch[strlen(pch) - 1]) == 'h') {
			base = 16;
	}

	ret_val = strict_strtoul(pch, base, &res);

	return ret_val ? : res;
}



static L2FW_RULE *l2fw_lookup(MV_U32 srcIP, MV_U32 dstIP)
{
	MV_U32 hash;
	L2FW_RULE *rule;

	hash = mv_jhash_3words(srcIP, dstIP, (MV_U32) 0, l2fw_jhash_iv);
	hash &= L2FW_HASH_MASK;
	rule = l2fw_hash[hash];

#ifdef CONFIG_MV_ETH_L2FW_DEBUG
	if (rule)
		printk(KERN_INFO "rule is not NULL in %s\n", __func__);
	else
		printk(KERN_INFO "rule is NULL in %s\n", __func__);
#endif
	while (rule) {
		if ((rule->srcIP == srcIP) && (rule->dstIP == dstIP))
			return rule;

		rule = rule->next;
	}

	return NULL;
}

void l2fw_show_numHashEntries(void)
{
	mvOsPrintf("number of Hash Entries is %d \n", numHashEntries);

}

void l2fw_flush(void)
{
	MV_U32 i = 0;
	mvOsPrintf("\nFlushing L2fw Rule Database: \n");
	mvOsPrintf("*******************************\n");
	for (i = 0; i < L2FW_HASH_SIZE; i++)
		l2fw_hash[i] = NULL;
	numHashEntries = 0;
}


void l2fw_dump(void)
{
	MV_U32 i = 0;
	L2FW_RULE *currRule;
	MV_U8	  *srcIP, *dstIP;

	mvOsPrintf("\nPrinting L2fw Rule Database: \n");
	mvOsPrintf("*******************************\n");

	for (i = 0; i < L2FW_HASH_SIZE; i++) {
		currRule = l2fw_hash[i];
		srcIP = (MV_U8 *)&(currRule->srcIP);
		dstIP = (MV_U8 *)&(currRule->dstIP);

		while (currRule != NULL) {
			mvOsPrintf("%u.%u.%u.%u->%u.%u.%u.%u     out port=%d (hash=%x)\n",
				MV_IPQUAD(srcIP), MV_IPQUAD(dstIP),
				currRule->port, i);
			currRule = currRule->next;
		}
	}

}


MV_STATUS l2fw_add(MV_U32 srcIP, MV_U32 dstIP, int port)
{
	L2FW_RULE *l2fw_rule;
	MV_U8	  *srcIPchr, *dstIPchr;

	MV_U32 hash = mv_jhash_3words(srcIP, dstIP, (MV_U32) 0, l2fw_jhash_iv);
	hash &= L2FW_HASH_MASK;
	if (numHashEntries == L2FW_HASH_SIZE) {
		printk(KERN_INFO "cannot add entry, hash table is full, there are %d entires \n", L2FW_HASH_SIZE);
		return MV_ERROR;
	}

	srcIPchr = (MV_U8 *)&(srcIP);
	dstIPchr = (MV_U8 *)&(dstIP);

#ifdef CONFIG_MV_ETH_L2FW_DEBUG
	mvOsPrintf("srcIP=%x dstIP=%x in %s\n", srcIP, dstIP, __func__);
	mvOsPrintf("srcIp = %u.%u.%u.%u in %s\n", MV_IPQUAD(srcIPchr), __func__);
	mvOsPrintf("dstIp = %u.%u.%u.%u in %s\n", MV_IPQUAD(dstIPchr), __func__);
#endif

	l2fw_rule = l2fw_lookup(srcIP, dstIP);
	if (l2fw_rule)
		return MV_OK;

	l2fw_rule = (L2FW_RULE *)mvOsMalloc(sizeof(L2FW_RULE));
	if (!l2fw_rule) {
		mvOsPrintf("%s: OOM\n", __func__);
		return MV_FAIL;
	}
#ifdef CONFIG_MV_ETH_L2FW_DEBUG
	mvOsPrintf("adding a rule to l2fw hash in %s\n", __func__);
#endif
	l2fw_rule->srcIP = srcIP;
	l2fw_rule->dstIP = dstIP;
		l2fw_rule->port = port;

	l2fw_rule->next = l2fw_hash[hash];
	l2fw_hash[hash] = l2fw_rule;
	numHashEntries++;
    return MV_OK;
}

MV_STATUS l2fw_add_ip(const char *buf)
{
	char *addr1, *addr2;
	L2FW_RULE *l2fw_rule;
	MV_U32 srcIP;
	MV_U32 dstIP;
	MV_U8	  *srcIPchr, *dstIPchr;
	char dest1[15];
	char dest2[15];
	char *portStr;
	int offset1, offset2, port;
	MV_U32 hash    = 0;
	if (numHashEntries == L2FW_HASH_SIZE) {
		printk(KERN_INFO "cannot add entry, hash table is full, there are %d entires \n", L2FW_HASH_SIZE);
		return MV_ERROR;
	}

	memset(dest1,   0, sizeof(dest1));
	memset(dest2,   0, sizeof(dest2));

	addr1 = strchr(buf, ',');
	addr2 =	strchr(addr1+1, ',');
	offset1 = addr1-buf;
	offset2 = addr2-addr1;
	if (!addr1) {
			printk(KERN_INFO "first separating comma (',') missing in input in %s\n", __func__);
			return MV_FAIL;
	}
	if (!addr2) {
			printk(KERN_INFO "second separating comma (',') missing in input in %s\n", __func__);
			return MV_FAIL;
	}

	strncpy(dest1, buf, addr1-buf);
	srcIP = in_aton(dest1);
	strncpy(dest2, buf+offset1+1, addr2-addr1-1);
	dstIP = in_aton(dest2);
	srcIPchr = (MV_U8 *)&(srcIP);
	dstIPchr = (MV_U8 *)&(dstIP);
	portStr = addr2+1;
	if (*portStr == 'D') {
		L2FW_RULE *l2fw_rule_to_del, *prev;
		hash = mv_jhash_3words(srcIP, dstIP, (MV_U32) 0, l2fw_jhash_iv);
		hash &= L2FW_HASH_MASK;
		l2fw_rule_to_del = l2fw_hash[hash];
		prev = NULL;

		while (l2fw_rule_to_del) {
		if ((l2fw_rule_to_del->srcIP == srcIP) &&
			(l2fw_rule_to_del->dstIP == dstIP)) {
			if (prev)
				prev->next = l2fw_rule_to_del->next;
			else
				l2fw_hash[hash] = l2fw_rule_to_del->next;
			mvOsPrintf("%u.%u.%u.%u->%u.%u.%u.%u deleted\n", MV_IPQUAD(srcIPchr), MV_IPQUAD(dstIPchr));
			mvOsFree(l2fw_rule_to_del);
			numHashEntries--;
			return MV_OK;
		}

		prev = l2fw_rule_to_del;
		l2fw_rule_to_del = l2fw_rule_to_del->next;
	}
		mvOsPrintf("%u.%u.%u.%u->%u.%u.%u.%u : entry not found\n", MV_IPQUAD(srcIPchr), MV_IPQUAD(dstIPchr));
		return MV_NOT_FOUND;
	}

	port = atoi(portStr);
	hash = mv_jhash_3words(srcIP, dstIP, (MV_U32) 0, l2fw_jhash_iv);
	hash &= L2FW_HASH_MASK;

	l2fw_rule = l2fw_lookup(srcIP, dstIP);
	if (l2fw_rule) {
		mvOsPrintf("%u.%u.%u.%u->%u.%u.%u.%u : entry already exist\n",
				MV_IPQUAD(srcIPchr), MV_IPQUAD(dstIPchr));
		return MV_OK;
	}

	l2fw_rule = (L2FW_RULE *)mvOsMalloc(sizeof(L2FW_RULE));
	if (!l2fw_rule) {
		mvOsPrintf("%s: OOM\n", __func__);
		return MV_FAIL;
	}
#ifdef CONFIG_MV_ETH_L2FW_DEBUG
	mvOsPrintf("adding a rule to l2fw hash in %s\n", __func__);
#endif
	l2fw_rule->srcIP = srcIP;
	l2fw_rule->dstIP = dstIP;
	l2fw_rule->port = port;

	l2fw_rule->next = l2fw_hash[hash];
	l2fw_hash[hash] = l2fw_rule;
	numHashEntries++;
    return MV_OK;

}

void l2fw_esp_show(void)
{
	if (espEnabled)
		printk(KERN_INFO "ESP is enabled in %s\n", __func__);
	else
		printk(KERN_INFO "ESP is not enabled in %s\n", __func__);
}

#ifdef CONFIG_MV_INCLUDE_XOR
static void dump_xor(void)
{
	mvOsPrintf(" CHANNEL_ARBITER_REG %08x\n",
		MV_REG_READ(XOR_CHANNEL_ARBITER_REG(1)));
	mvOsPrintf(" CONFIG_REG          %08x\n",
		MV_REG_READ(XOR_CONFIG_REG(1, XOR_CHAN(0))));
	mvOsPrintf(" ACTIVATION_REG      %08x\n",
		MV_REG_READ(XOR_ACTIVATION_REG(1, XOR_CHAN(0))));
	mvOsPrintf(" CAUSE_REG           %08x\n",
		MV_REG_READ(XOR_CAUSE_REG(1)));
	mvOsPrintf(" MASK_REG            %08x\n",
		MV_REG_READ(XOR_MASK_REG(1)));
	mvOsPrintf(" ERROR_CAUSE_REG     %08x\n",
		MV_REG_READ(XOR_ERROR_CAUSE_REG(1)));
	mvOsPrintf(" ERROR_ADDR_REG      %08x\n",
		MV_REG_READ(XOR_ERROR_ADDR_REG(1)));
	mvOsPrintf(" NEXT_DESC_PTR_REG   %08x\n",
		MV_REG_READ(XOR_NEXT_DESC_PTR_REG(1, XOR_CHAN(0))));
	mvOsPrintf(" CURR_DESC_PTR_REG   %08x\n",
		MV_REG_READ(XOR_CURR_DESC_PTR_REG(1, XOR_CHAN(0))));
	mvOsPrintf(" BYTE_COUNT_REG      %08x\n\n",
		MV_REG_READ(XOR_BYTE_COUNT_REG(1, XOR_CHAN(0))));
	mvOsPrintf("  %08x\n\n", XOR_WINDOW_CTRL_REG(1, XOR_CHAN(0))) ;
		mvOsPrintf(" XOR_WINDOW_CTRL_REG      %08x\n\n",
		MV_REG_READ(XOR_WINDOW_CTRL_REG(1, XOR_CHAN(0)))) ;
}
#endif


/* L2fw defines */
#define L2FW_DISABLE				0
#define TX_AS_IS					1
#define SWAP_MAC					2
#define COPY_AND_SWAP		        3

#define XOR_CAUSE_DONE_MASK(chan) ((BIT0|BIT1) << (chan * 16))

static int         l2fw_xor_threshold = 200;
static MV_XOR_DESC *eth_xor_desc = NULL;
static MV_LONG      eth_xor_desc_phys_addr;


static int mv_eth_poll_l2fw(struct napi_struct *napi, int budget)
{
	int rx_done = 0;
	MV_U32 causeRxTx;
	struct eth_port *pp = MV_ETH_PRIV(napi->dev);
	read_lock(&pp->rwlock);

	STAT_INFO(pp->stats.poll[smp_processor_id()]++);

	/* Read cause register */
	causeRxTx = MV_REG_READ(NETA_INTR_NEW_CAUSE_REG(pp->port)) &
	    (MV_ETH_MISC_SUM_INTR_MASK | MV_ETH_TXDONE_INTR_MASK |
		 MV_ETH_RX_INTR_MASK);

	if (causeRxTx & MV_ETH_MISC_SUM_INTR_MASK) {
		MV_U32 causeMisc;

		/* Process MISC events - Link, etc ??? */
		causeRxTx &= ~MV_ETH_MISC_SUM_INTR_MASK;
		causeMisc = MV_REG_READ(NETA_INTR_MISC_CAUSE_REG(pp->port));

		if (causeMisc & NETA_CAUSE_LINK_CHANGE_MASK)
			mv_eth_link_event(pp, 1);
		MV_REG_WRITE(NETA_INTR_MISC_CAUSE_REG(pp->port), 0);
	}

	causeRxTx |= pp->causeRxTx[smp_processor_id()];
#ifdef CONFIG_MV_ETH_TXDONE_ISR
	if (causeRxTx & MV_ETH_TXDONE_INTR_MASK) {
		/* TX_DONE process */

		mv_eth_tx_done_gbe(pp,
				(causeRxTx & MV_ETH_TXDONE_INTR_MASK));

		causeRxTx &= ~MV_ETH_TXDONE_INTR_MASK;
	}
#endif /* CONFIG_MV_ETH_TXDONE_ISR */

#if (CONFIG_MV_ETH_RXQ > 1)
	while ((causeRxTx != 0) && (budget > 0)) {
		int count, rx_queue;

		rx_queue = mv_eth_rx_policy(causeRxTx);
		if (rx_queue == -1)
			break;

		count = mv_eth_l2fw_rx(pp, budget, rx_queue);
		rx_done += count;
		budget -= count;
		if (budget > 0)
			causeRxTx &=
			 ~((1 << rx_queue) << NETA_CAUSE_RXQ_OCCUP_DESC_OFFS);
	}
#else
	rx_done = mv_eth_l2fw_rx(pp, budget, CONFIG_MV_ETH_RXQ_DEF);
	budget -= rx_done;
#endif /* (CONFIG_MV_ETH_RXQ > 1) */

	if (budget > 0) {
		unsigned long flags;
		causeRxTx = 0;

		napi_complete(napi);
		STAT_INFO(pp->stats.poll_exit[smp_processor_id()]++);

		local_irq_save(flags);
		MV_REG_WRITE(NETA_INTR_NEW_MASK_REG(pp->port),
			(MV_ETH_MISC_SUM_INTR_MASK | MV_ETH_TXDONE_INTR_MASK |
				  MV_ETH_RX_INTR_MASK));

		local_irq_restore(flags);
	}
	pp->causeRxTx[smp_processor_id()] = causeRxTx;

	read_unlock(&pp->rwlock);

	return rx_done;
}


void mv_eth_set_l2fw(int cmd, int rx_port, int out_tx_port)
{
	struct eth_port *pp;
	struct net_device *dev;
	int group;

	pp = mv_eth_ports[rx_port];
	if (!pp) {
		mvOsPrintf("pp is NULL in setting L2FW (%s)\n", __func__);
		return;
	}

	dev = pp->dev;
	if (dev == NULL) {
		mvOsPrintf("device is NULL in in setting L2FW (%s)\n", __func__);
		return;
	}
	if (!test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags))) {
		mvOsPrintf("Device is down for port=%d ; MV_ETH_F_STARTED_BIT is not set in %s\n", rx_port, __func__);
		mvOsPrintf("Cannot set to L2FW mode in %s\n", __func__);
		return;
	}

	/* when disabling l2fw, and then ifdown/up, we should
	   enable MV_ETH_F_CONNECT_LINUX_BIT bit so that the port will be started ok.
	   TBD: remember last state */

	if (cmd == L2FW_DISABLE)
		set_bit(MV_ETH_F_CONNECT_LINUX_BIT, &(pp->flags));
	else
		clear_bit(MV_ETH_F_CONNECT_LINUX_BIT, &(pp->flags));

	for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++) {
		if (cmd == L2FW_DISABLE) {
		if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags)))
			napi_disable(pp->napiGroup[group]);
		netif_napi_del(pp->napiGroup[group]);
			netif_napi_add(dev, pp->napiGroup[group], mv_eth_poll,
				pp->weight);
			if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags)))
				napi_enable(pp->napiGroup[group]);
		} else {
			if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags)))
				napi_disable(pp->napiGroup[group]);
			netif_napi_del(pp->napiGroup[group]);
			printk(KERN_INFO "pp->weight=%d in %s\n", pp->weight, __func__);
			netif_napi_add(dev, pp->napiGroup[group], mv_eth_poll_l2fw,
				pp->weight);
		if (test_bit(MV_ETH_F_STARTED_BIT, &(pp->flags)))
			napi_enable(pp->napiGroup[group]);
	}
}
}


static inline struct eth_pbuf *l2fw_swap_mac(struct eth_pbuf *pRxPktInfo)
{
	MV_U16 *pSrc;
	int i;
	MV_U16 swap;
	pSrc = (MV_U16 *)(pRxPktInfo->pBuf + pRxPktInfo->offset + MV_ETH_MH_SIZE);

	for (i = 0; i < 3; i++) {
		swap = pSrc[i];
		pSrc[i] = pSrc[i+3];
		pSrc[i+3] = swap;
		}

	return  pRxPktInfo;
}

static inline void l2fw_copy_mac(struct eth_pbuf *pRxPktInfo,
					 struct eth_pbuf *pTxPktInfo)
	{
	/* copy 30 bytes (start after MH header) */
	/* 12 for SA + DA */
	/* 18 for the rest */
	MV_U16 *pSrc;
	MV_U16 *pDst;
	int i;
	pSrc = (MV_U16 *)(pRxPktInfo->pBuf + pRxPktInfo->offset + MV_ETH_MH_SIZE);
	pDst = (MV_U16 *)(pTxPktInfo->pBuf + pTxPktInfo->offset + MV_ETH_MH_SIZE);

	/* swap mac SA and DA */
	for (i = 0; i < 3; i++) {
		pDst[i]   = pSrc[i+3];
		pDst[i+3] = pSrc[i];
		}
	for (i = 6; i < 15; i++)
		pDst[i] = pSrc[i];
	}

static inline void l2fw_copy_and_swap_mac(struct eth_pbuf *pRxPktInfo, struct eth_pbuf *pTxPktInfo)
{
	MV_U16 *pSrc;
	MV_U16 *pDst;
	int i;

	pSrc = (MV_U16 *)(pRxPktInfo->pBuf +  pRxPktInfo->offset + MV_ETH_MH_SIZE);
	pDst = (MV_U16 *)(pTxPktInfo->pBuf +  pTxPktInfo->offset + MV_ETH_MH_SIZE);
	for (i = 0; i < 3; i++) {
		pDst[i]   = pSrc[i+3];
		pDst[i+3] = pSrc[i];
		}
}

static inline
struct eth_pbuf *eth_l2fw_copy_packet_withoutXor(struct eth_pbuf *pRxPktInfo)
{
	MV_U8 *pSrc;
	MV_U8 *pDst;
	struct bm_pool *pool;
	struct eth_pbuf *pTxPktInfo;

	mvOsCacheInvalidate(NULL, pRxPktInfo->pBuf + pRxPktInfo->offset,
						pRxPktInfo->bytes);

	pool = &mv_eth_pool[pRxPktInfo->pool];
	pTxPktInfo = mv_eth_pool_get(pool);
	if (pTxPktInfo == NULL) {
		mvOsPrintf("pTxPktInfo == NULL in %s\n", __func__);
		return NULL;
		}
	pSrc = pRxPktInfo->pBuf +  pRxPktInfo->offset + MV_ETH_MH_SIZE;
	pDst = pTxPktInfo->pBuf +  pTxPktInfo->offset + MV_ETH_MH_SIZE;

	memcpy(pDst+12, pSrc+12, pRxPktInfo->bytes-12);
	l2fw_copy_and_swap_mac(pRxPktInfo, pTxPktInfo);
	pTxPktInfo->bytes = pRxPktInfo->bytes;
	mvOsCacheFlush(NULL, pTxPktInfo->pBuf + pTxPktInfo->offset, pTxPktInfo->bytes);

	return pTxPktInfo;
}

static inline
struct eth_pbuf *eth_l2fw_copy_packet_withXor(struct eth_pbuf *pRxPktInfo)
{
	struct bm_pool *pool;
	struct eth_pbuf *pTxPktInfo;

	pool = &mv_eth_pool[pRxPktInfo->pool];
	pTxPktInfo = mv_eth_pool_get(pool);
	if (pTxPktInfo == NULL) {
		mvOsPrintf("pTxPktInfo == NULL in %s\n", __func__);
		return NULL;
		}

	/* sync between giga and XOR to avoid errors (like checksum errors in TX)
	   when working with IOCC */

	mvOsCacheIoSync();

	eth_xor_desc->srcAdd0    = pRxPktInfo->physAddr + pRxPktInfo->offset + MV_ETH_MH_SIZE + 30;
	eth_xor_desc->phyDestAdd = pTxPktInfo->physAddr + pTxPktInfo->offset + MV_ETH_MH_SIZE + 30;

	eth_xor_desc->byteCnt    = pRxPktInfo->bytes - 30;

	eth_xor_desc->phyNextDescPtr = 0;
	eth_xor_desc->status         = BIT31;
	/* we had changed only the first part of eth_xor_desc, so flush only one
	 line of cache */
	mvOsCacheLineFlush(NULL, eth_xor_desc);
	MV_REG_WRITE(XOR_NEXT_DESC_PTR_REG(1, XOR_CHAN(0)), eth_xor_desc_phys_addr);

	MV_REG_WRITE(XOR_ACTIVATION_REG(1, XOR_CHAN(0)), XEXACTR_XESTART_MASK);

	mvOsCacheLineInv(NULL, pRxPktInfo->pBuf + pRxPktInfo->offset);
	l2fw_copy_mac(pRxPktInfo, pTxPktInfo);
	mvOsCacheLineFlush(NULL, pTxPktInfo->pBuf + pTxPktInfo->offset);

	/* Update TxPktInfo */
	pTxPktInfo->bytes = pRxPktInfo->bytes;
	return pTxPktInfo;
}

#ifdef CONFIG_MV_INCLUDE_XOR
void setXorDesc(void)
{
	unsigned int mode;
	eth_xor_desc = mvOsMalloc(sizeof(MV_XOR_DESC) + XEXDPR_DST_PTR_DMA_MASK + 32);
	eth_xor_desc = (MV_XOR_DESC *)MV_ALIGN_UP((MV_U32)eth_xor_desc, XEXDPR_DST_PTR_DMA_MASK+1);
	eth_xor_desc_phys_addr = mvOsIoVirtToPhys(NULL, eth_xor_desc);
	mvSysXorInit();

	mode = MV_REG_READ(XOR_CONFIG_REG(1, XOR_CHAN(0)));
	mode &= ~XEXCR_OPERATION_MODE_MASK;
	mode |= XEXCR_OPERATION_MODE_DMA;
	MV_REG_WRITE(XOR_CONFIG_REG(1, XOR_CHAN(0)), mode);

    MV_REG_WRITE(XOR_NEXT_DESC_PTR_REG(1, XOR_CHAN(0)), eth_xor_desc_phys_addr);
	dump_xor();
}
#endif

static inline int xorReady(void)
{
	int timeout = 0;

	while (!(MV_REG_READ(XOR_CAUSE_REG(1)) & XOR_CAUSE_DONE_MASK(XOR_CHAN(0)))) {
		if (timeout > 0x100000) {
			mvOsPrintf("XOR timeout\n");
			return 0;
			}
		timeout++;
	}

	/* Clear int */
	MV_REG_WRITE(XOR_CAUSE_REG(1), ~(XOR_CAUSE_DONE_MASK(XOR_CHAN(0))));

	return 1;
}


void l2fw(int cmd, int rx_port, int tx_port)
{
	struct eth_port_l2fw *ppl2fw;

	mv_eth_ports_l2fw_num = mvCtrlEthMaxPortGet();
	ppl2fw = mv_eth_ports_l2fw[rx_port];
	mvOsPrintf("cmd=%d rx_port=%d tx_port=%d in %s \n",
				cmd, rx_port, tx_port, __func__);
	ppl2fw->txPort = tx_port;
	ppl2fw->cmd	= cmd;
	mv_eth_set_l2fw(cmd, rx_port, tx_port);
}

void l2fw_xor(int threshold)
{
	mvOsPrintf("setting threshold to %d in %s\n", threshold, __func__);
	l2fw_xor_threshold = threshold;
}


static inline MV_STATUS mv_eth_l2fw_tx(struct eth_pbuf *pkt, struct eth_port *pp, int withXor,
									   struct neta_rx_desc *rx_desc)
{
	struct neta_tx_desc *tx_desc;
	u32 tx_cmd = 0;
	struct tx_queue *txq_ctrl;
	/* assigning different txq for each rx port , to avoid waiting on the
	same txq lock when traffic on several rx ports are destined to the same
	outgoing interface */
	int txq = pp->txq[smp_processor_id()];
	read_lock(&pp->rwlock);
	txq_ctrl = &pp->txq_ctrl[pp->txp * CONFIG_MV_ETH_TXQ + txq];

	mv_eth_lock(txq_ctrl, pp);

	if (txq_ctrl->txq_count >= mv_ctrl_txdone)
		mv_eth_txq_done(pp, txq_ctrl);
	/* Get next descriptor for tx, single buffer, so FIRST & LAST */
	tx_desc = mv_eth_tx_desc_get(txq_ctrl, 1);
	if (tx_desc == NULL) {

		mv_eth_unlock(txq_ctrl, pp);

		read_unlock(&pp->rwlock);
		/* No resources: Drop */
		pp->dev->stats.tx_dropped++;
		if (withXor)
			xorReady();
		return MV_DROPPED;
	}
	txq_ctrl->txq_count++;

	tx_cmd |= NETA_TX_BM_ENABLE_MASK | NETA_TX_BM_POOL_ID_MASK(pkt->pool);
	txq_ctrl->shadow_txq[txq_ctrl->shadow_txq_put_i] = (u32) NULL;
	mv_eth_shadow_inc_put(txq_ctrl);

	tx_desc->command = tx_cmd | NETA_TX_L4_CSUM_NOT |
		NETA_TX_FLZ_DESC_MASK | NETA_TX_F_DESC_MASK
		| NETA_TX_L_DESC_MASK |
		NETA_TX_PKT_OFFSET_MASK(pkt->offset + MV_ETH_MH_SIZE);

	tx_desc->dataSize    = pkt->bytes;
	tx_desc->bufPhysAddr = pkt->physAddr;

	mv_eth_tx_desc_flush(tx_desc);

	if (withXor) {
		if (!xorReady()) {
			mvOsPrintf("MV_DROPPED in %s\n", __func__);

			mv_eth_unlock(txq_ctrl, pp);

			read_unlock(&pp->rwlock);
			return MV_DROPPED;
		}
	}
	mvNetaTxqPendDescAdd(pp->port, pp->txp, txq, 1);

	mv_eth_unlock(txq_ctrl, pp);

	read_unlock(&pp->rwlock);

	return MV_OK;
}

static inline int mv_eth_l2fw_rx(struct eth_port *pp, int rx_todo, int rxq)
{
	struct eth_port  *new_pp;
	L2FW_RULE *l2fw_rule;
	MV_NETA_RXQ_CTRL *rx_ctrl = pp->rxq_ctrl[rxq].q;
	int rx_done, rx_filled;
	struct neta_rx_desc *rx_desc;
	u32 rx_status = MV_OK;
	struct eth_pbuf *pkt;
	struct eth_pbuf *newpkt = NULL;
	struct bm_pool *pool;
	MV_STATUS status = MV_OK;
	struct eth_port_l2fw *ppl2fw = mv_eth_ports_l2fw[pp->port];
	MV_IP_HEADER *pIph = NULL;
	MV_U8 *pData;
	int	ipOffset;

	rx_done = mvNetaRxqBusyDescNumGet(pp->port, rxq);
	mvOsCacheIoSync();
	if (rx_todo > rx_done)
		rx_todo = rx_done;
	rx_done = 0;
	rx_filled = 0;

	/* Fairness NAPI loop */
	while (rx_done < rx_todo) {
#ifdef CONFIG_MV_ETH_RX_DESC_PREFETCH
		rx_desc = mv_eth_rx_prefetch(pp, rx_ctrl, rx_done, rx_todo);
		if (!rx_desc)
			printk(KERN_INFO "rx_desc is NULL in %s\n", __func__);
#else
		rx_desc = mvNetaRxqNextDescGet(rx_ctrl);
		mvOsCacheLineInv(NULL, rx_desc);
		prefetch(rx_desc);
#endif /* CONFIG_MV_ETH_RX_DESC_PREFETCH */

		rx_done++;
		rx_filled++;

		pkt = (struct eth_pbuf *)rx_desc->bufCookie;
		if (!pkt) {
			printk(KERN_INFO "pkt is NULL in ; rx_done=%d %s\n", rx_done, __func__);
			return rx_done;
		}

		pool = &mv_eth_pool[pkt->pool];
		rx_status = rx_desc->status;
		if (((rx_status & NETA_RX_FL_DESC_MASK) != NETA_RX_FL_DESC_MASK) ||
			(rx_status & NETA_RX_ES_MASK)) {
			STAT_ERR(pp->stats.rx_error++);

			if (pp->dev)
				pp->dev->stats.rx_errors++;

			mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);
			continue;
		}

		pkt->bytes = rx_desc->dataSize - (MV_ETH_CRC_SIZE + MV_ETH_MH_SIZE);

		pData = pkt->pBuf + pkt->offset;

#ifdef CONFIG_MV_ETH_PNC
		ipOffset = NETA_RX_GET_IPHDR_OFFSET(rx_desc);
#else
		if ((rx_desc->status & ETH_RX_VLAN_TAGGED_FRAME_MASK))
			ipOffset = MV_ETH_MH_SIZE + sizeof(MV_802_3_HEADER) + MV_VLAN_HLEN;
		else
			ipOffset = MV_ETH_MH_SIZE + sizeof(MV_802_3_HEADER);
#endif

		pIph = (MV_IP_HEADER *)(pData + ipOffset);
		if (pIph == NULL) {
			printk(KERN_INFO "pIph==NULL in %s\n", __func__);
			continue;
		}
#ifdef CONFIG_MV_ETH_L2FW_DEBUG
		if (pIph) {
			MV_U8 *srcIP, *dstIP;
			srcIP = (MV_U8 *)&(pIph->srcIP);
			dstIP = (MV_U8 *)&(pIph->dstIP);
			printk(KERN_INFO "%u.%u.%u.%u->%u.%u.%u.%u in %s\n", MV_IPQUAD(srcIP), MV_IPQUAD(dstIP), __func__);
		} else
			printk(KERN_INFO "pIph is NULL in %s\n", __func__);
#endif
		if (espEnabled)
			new_pp  = mv_eth_ports[ppl2fw->txPort];
		else {
			l2fw_rule = l2fw_lookup(pIph->srcIP, pIph->dstIP);

			if (!l2fw_rule) {
#ifdef CONFIG_MV_ETH_L2FW_DEBUG
				printk(KERN_INFO "l2fw_lookup() failed in %s\n", __func__);
#endif
				mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);
				continue;
			 }

#ifdef CONFIG_MV_ETH_L2FW_DEBUG
				printk(KERN_INFO "l2fw_lookup() is ok l2fw_rule->port=%d in %s\n", l2fw_rule->port, __func__);
#endif
				new_pp  = mv_eth_ports[l2fw_rule->port];
			}

		switch (ppl2fw->cmd) {
		case TX_AS_IS:
#ifdef CONFIG_MV_ETH_L2SEC
					if (espEnabled) {
						status = handleEsp(pkt, rx_desc, new_pp, pp->port);
					}
				else
#endif
				status = mv_eth_l2fw_tx(pkt, new_pp, 0, rx_desc);
				break;

		case SWAP_MAC:
				mvOsCacheLineInv(NULL, pkt->pBuf + pkt->offset);
				l2fw_swap_mac(pkt);
				mvOsCacheLineFlush(NULL, pkt->pBuf+pkt->offset);
				status = mv_eth_l2fw_tx(pkt, new_pp, 0, rx_desc);
				break;

		case COPY_AND_SWAP:
				if (pkt->bytes >= l2fw_xor_threshold) {
					newpkt = eth_l2fw_copy_packet_withXor(pkt);
					if (newpkt)
						status = mv_eth_l2fw_tx(newpkt, new_pp, 1, rx_desc);
					else
						status = MV_ERROR;
				} else {
						newpkt = eth_l2fw_copy_packet_withoutXor(pkt);
						if (newpkt)
							status = mv_eth_l2fw_tx(newpkt, new_pp, 0, rx_desc);
						else
							status = MV_ERROR;
				}
		}
		if (status == MV_OK) {
				mvOsCacheLineInv(NULL, rx_desc);
			/* we do not need the pkt , we do not do anything with it*/
			if  ((ppl2fw->cmd	== COPY_AND_SWAP) && !(espEnabled))
				mv_eth_pool_put(pool, pkt);
			continue;
		} else if (status == MV_DROPPED) {
			mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);
			if ((ppl2fw->cmd	== COPY_AND_SWAP) && !(espEnabled))
				mv_eth_pool_put(pool, newpkt);

			continue;
		} else if (status == MV_ERROR) {
			printk(KERN_INFO "MV_ERROR in %s\n", __func__);
			mv_eth_rxq_refill(pp, rxq, pkt, pool, rx_desc);
		}

	} /* of while */

	/* Update RxQ management counters */
	mvOsCacheIoSync();

	mvNetaRxqDescNumUpdate(pp->port, rxq, rx_done, rx_filled);

	return rx_done;
}

#ifdef CONFIG_MV_ETH_L2FW
int __devinit mv_l2fw_init(void)
{
	int size, port;
	MV_U32 bytes;
	MV_U32 regVal;
	mv_eth_ports_l2fw_num = mvCtrlEthMaxPortGet();
	mvOsPrintf("in %s: mv_eth_ports_l2fw_num=%d\n", __func__, mv_eth_ports_l2fw_num);

	size = mv_eth_ports_l2fw_num * sizeof(struct eth_port_l2fw *);
	mv_eth_ports_l2fw = mvOsMalloc(size);
	if (!mv_eth_ports_l2fw)
		goto oom;
	memset(mv_eth_ports_l2fw, 0, size);
	for (port = 0; port < mv_eth_ports_l2fw_num; port++) {
		mv_eth_ports_l2fw[port] =
			mvOsMalloc(sizeof(struct eth_port_l2fw));
		if (!mv_eth_ports_l2fw[port])
			goto oom1;
		mv_eth_ports_l2fw[port]->cmd    = L2FW_DISABLE;
		mv_eth_ports_l2fw[port]->txPort = -1;
	}

	bytes = sizeof(L2FW_RULE *) * L2FW_HASH_SIZE;
	l2fw_jhash_iv = mvOsRand();

	l2fw_hash = (L2FW_RULE **)mvOsMalloc(bytes);
	if (l2fw_hash == NULL) {
		mvOsPrintf("l2fw hash: not enough memory\n");
		return MV_NO_RESOURCE;
	}

	mvOsMemset(l2fw_hash, 0, bytes);

	mvOsPrintf("L2FW hash init %d entries, %d bytes\n", L2FW_HASH_SIZE, bytes);
	regVal = 0;
#ifdef CONFIG_MV_ETH_L2SEC
	cesa_init();
#endif

#ifdef CONFIG_MV_INCLUDE_XOR
	setXorDesc();
#endif
	return 0;
oom:
	mvOsPrintf("%s: out of memory in L2FW initialization\n", __func__);
oom1:
	mvOsFree(mv_eth_ports_l2fw);
	return -ENOMEM;

}
#endif

module_init(mv_l2fw_init);

MODULE_AUTHOR("Rami Rosen");
MODULE_DESCRIPTION("l2fw module");
MODULE_LICENSE("GPL");

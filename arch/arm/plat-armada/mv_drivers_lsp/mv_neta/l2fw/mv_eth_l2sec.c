/* mv_eth_l2sec.c */

#include "mv_eth_l2sec.h"

static inline MV_STATUS mv_eth_cesa_l2fw_tx(struct eth_pbuf *pkt, struct eth_port *pp)
{
	struct neta_tx_desc *tx_desc;
	u32 tx_cmd = 0;
	struct tx_queue *txq_ctrl;

	/* assigning different txq for each rx port , to avoid waiting on the
	same txq lock when traffic on several rx ports are destined to the same
	outgoing interface */
	int txq = 0;
	txq_ctrl = &pp->txq_ctrl[pp->txp * CONFIG_MV_ETH_TXQ + txq];

	mv_eth_lock(txq_ctrl, pp);

	if (txq_ctrl->txq_count >= mv_ctrl_txdone)
		mv_eth_txq_done(pp, txq_ctrl);
	/* Get next descriptor for tx, single buffer, so FIRST & LAST */
	tx_desc = mv_eth_tx_desc_get(txq_ctrl, 1);
	if (tx_desc == NULL) {
		/* printk("tx_desc == NULL pp->port=%d in %s\n", pp->port, ,__func__); */

		mv_eth_unlock(txq_ctrl, pp);

		/* No resources: Drop */
		pp->dev->stats.tx_dropped++;
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
	mvNetaTxqPendDescAdd(pp->port, pp->txp, 0, 1);

	mv_eth_unlock(txq_ctrl, pp);

	return MV_OK;
}

static inline void nfp_sec_complete_out(unsigned long data)

{
	MV_NFP_SEC_CESA_PRIV_L2FW *nfp_sec_cesa_priv = (MV_NFP_SEC_CESA_PRIV_L2FW *)data;		MV_U32            ifout;
	MV_PKT_INFO       *pkt;
	MV_BUF_INFO       *pBuf;
	struct eth_port   *pp;
	struct eth_pbuf   *pPkt;
	int oldOfsset;
	MV_STATUS status = MV_FAIL;
	static int counterOfFailed = 0;
	if (!nfp_sec_cesa_priv) {
		printk(KERN_INFO "nfp_sec_cesa_priv is NULL in %s\n", __func__);
		return;
	}
	ifout = nfp_sec_cesa_priv->ifout;

	pkt = nfp_sec_cesa_priv->pPktInfo;
	if (!pkt) {
		printk(KERN_INFO "pPktInfo is NULL in %s\n", __func__);
		return;
	}
	pBuf = pkt->pFrags;
	if (!pBuf) {
		printk(KERN_INFO "pBuf is NULL in %s\n", __func__);
		return;
	}
	pPkt = nfp_sec_cesa_priv->pPkt;
	if (!pPkt) {
		printk(KERN_INFO "!pPkt) in %s\n", __func__);
		return;
	}
	pPkt->bytes    = pBuf->dataSize;
	pPkt->bytes += MV_NFP_SEC_ESP_OFFSET;
	oldOfsset      = pPkt->offset;
	pPkt->offset   = pPkt->offset - (sizeof(MV_ESP_HEADER) + sizeof(MV_IP_HEADER) + MV_CESA_AES_BLOCK_SIZE);

	pp     = mv_eth_ports[ifout];

	status = 	mv_eth_cesa_l2fw_tx(pPkt, pp);
	if (status == MV_DROPPED)
		counterOfFailed++;
	 else
		pPkt->offset = oldOfsset;
}

int l2fw_set_cesa_chan(int port, int cesaChan)
{
//	struct eth_port *pp;
	printk(KERN_INFO "setting cesaChan to %d for port=%d \n", cesaChan, port);
	if ((cesaChan != CESA_0) && (cesaChan != CESA_1))  {
		printk(KERN_INFO "non permitted value for CESA channel \n");
		return -EINVAL;
	}
//	pp = mv_eth_ports[port];
//	if (pp)
//		pp->cesaChan = cesaChan;
	cesaChanPort[port] = cesaChan;
	return 0;
}

MV_STATUS my_mvSysCesaInit(int numOfSession, int queueDepth, void *osHandle)
{

	MV_CESA_HAL_DATA halData;
	MV_UNIT_WIN_INFO addrWinMap[MAX_TARGETS + 1];
	MV_STATUS status;
	MV_U8 chan;

	status = mvCtrlAddrWinMapBuild(addrWinMap, MAX_TARGETS + 1);

	if (status == MV_OK) {
		for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
			status = mvCesaTdmaWinInit(chan, addrWinMap);
			if (status != MV_OK) {
				mvOsPrintf("Error, unable to initialize CESA windows for channel(%d)\n", chan);
				break;
			}
			halData.sramPhysBase[chan] = (MV_ULONG)mv_crypto_virt_base_get(chan);
			halData.sramVirtBase[chan] = (MV_U8 *)mv_crypto_virt_base_get(chan);
			halData.sramOffset[chan] = 0;
		}

		if (status == MV_OK) {
		halData.ctrlModel = mvCtrlModelGet();
		halData.ctrlRev = mvCtrlRevGet();
			status = mvCesaHalInit(numOfSession, queueDepth,
					osHandle, &halData);
		}
	}

	return status;
}

void cesaStart(void)
{
	int bufNum, bufSize;
	int i, j, idx;
	MV_CESA_MBUF *pMbufSrc_0, *pMbufDst_0;
	MV_BUF_INFO *pFragsSrc_0, *pFragsDst_0;
	char *pBuf_0;

	MV_CESA_MBUF *pMbufSrc_1, *pMbufDst_1;
	MV_BUF_INFO *pFragsSrc_1, *pFragsDst_1;
	char *pBuf_1;

	printk(KERN_INFO "in %s\n", __func__);

	cesaCmdArray_0 = 	mvOsMalloc(sizeof(MV_CESA_COMMAND) * CESA_DEF_REQ_SIZE);

	if (cesaCmdArray_0 == NULL) {
		mvOsPrintf("Can't allocate %d bytes of memory\n",
			   (int)(sizeof(MV_CESA_COMMAND) * CESA_DEF_REQ_SIZE));
		return;
	}
	memset(cesaCmdArray_0, 0, sizeof(MV_CESA_COMMAND) * CESA_DEF_REQ_SIZE);
	/* CESA_DEF_BUF_NUM */
	bufNum    =  1;
	/* CESA_DEF_BUF_SIZE */
	bufSize   = 1500;

	pMbufSrc_0  = mvOsMalloc(sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	pFragsSrc_0 = mvOsMalloc(sizeof(MV_BUF_INFO) * bufNum * CESA_DEF_REQ_SIZE);

	pMbufDst_0  = mvOsMalloc(sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	pFragsDst_0 = mvOsMalloc(sizeof(MV_BUF_INFO) * bufNum * CESA_DEF_REQ_SIZE);

	if ((pMbufSrc_0 == NULL) || (pFragsSrc_0 == NULL) ||
		(pMbufDst_0 == NULL) || (pFragsDst_0 == NULL)) {
		mvOsPrintf(" Can't malloc Src and Dst pMbuf and pFrags structures.\n");
		return;
	}

	memset(pMbufSrc_0,  0, sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	memset(pFragsSrc_0, 0, sizeof(MV_BUF_INFO) * bufNum * CESA_DEF_REQ_SIZE);

	memset(pMbufDst_0,  0, sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	memset(pFragsDst_0, 0, sizeof(MV_BUF_INFO) * bufNum * CESA_DEF_REQ_SIZE);

	idx = 0;
		for (i = 0; i < CESA_DEF_REQ_SIZE; i++) {
		pBuf_0 = mvOsIoCachedMalloc(cesaOSHandle, bufSize * bufNum * 2,
					  &cesaBufs_0[i].bufPhysAddr, &cesaBufs_0[i].memHandle);
		if (pBuf_0 == NULL) {
			mvOsPrintf("testStart: Can't malloc %d bytes for pBuf\n", bufSize * bufNum * 2);
				return;
			}

		memset(pBuf_0, 0, bufSize * bufNum * 2);
		mvOsCacheFlush(cesaOSHandle, pBuf_0, bufSize * bufNum * 2);
		if (pBuf_0 == NULL) {
				mvOsPrintf("Can't allocate %d bytes for req_%d buffers\n",
				   bufSize * bufNum * 2, i);
				return;
			}

		cesaBufs_0[i].bufVirtPtr = (MV_U8 *) pBuf_0;
		cesaBufs_0[i].bufSize = bufSize * bufNum * 2;

		cesaCmdArray_0[i].pSrc = &pMbufSrc_0[i];
		cesaCmdArray_0[i].pSrc->pFrags = &pFragsSrc_0[idx];
		cesaCmdArray_0[i].pSrc->numFrags = bufNum;
		cesaCmdArray_0[i].pSrc->mbufSize = 0;

		cesaCmdArray_0[i].pDst = &pMbufDst_0[i];
		cesaCmdArray_0[i].pDst->pFrags = &pFragsDst_0[idx];
		cesaCmdArray_0[i].pDst->numFrags = bufNum;
		cesaCmdArray_0[i].pDst->mbufSize = 0;

		for (j = 0; j < bufNum; j++) {
			cesaCmdArray_0[i].pSrc->pFrags[j].bufVirtPtr = (MV_U8 *) pBuf_0;
			cesaCmdArray_0[i].pSrc->pFrags[j].bufSize = bufSize;
			pBuf_0 += bufSize;
			cesaCmdArray_0[i].pDst->pFrags[j].bufVirtPtr = (MV_U8 *) pBuf_0;

			cesaCmdArray_0[i].pDst->pFrags[j].bufSize = bufSize;
			pBuf_0 += bufSize;
		}
		idx += bufNum;
	}

	cesaMbufArray_0 = mvOsMalloc(sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	if (cesaMbufArray_0 == NULL) {
		mvOsPrintf("Can't allocate %d bytes of memory\n",
			   (int)(sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE));
		return;
	}
	memset(cesaMbufArray_0, 0, sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);

	cesaPrivArray_0 = mvOsMalloc(sizeof(MV_NFP_SEC_CESA_PRIV_L2FW) * (CESA_DEF_REQ_SIZE + MV_NFP_SEC_REQ_Q_SIZE));
	memset(cesaPrivArray_0, 0, sizeof(MV_NFP_SEC_CESA_PRIV_L2FW) * (CESA_DEF_REQ_SIZE + MV_NFP_SEC_REQ_Q_SIZE));

	/* second engine */
	cesaCmdArray_1 = 	mvOsMalloc(sizeof(MV_CESA_COMMAND) * CESA_DEF_REQ_SIZE);

	if (cesaCmdArray_1 == NULL) {
		mvOsPrintf("Can't allocate %d bytes of memory\n",
			   (int)(sizeof(MV_CESA_COMMAND) * CESA_DEF_REQ_SIZE));
		return;
			}
	memset(cesaCmdArray_1, 0, sizeof(MV_CESA_COMMAND) * CESA_DEF_REQ_SIZE);

	/* CESA_DEF_BUF_NUM */
	bufNum    =  1;
	/* CESA_DEF_BUF_SIZE */
	bufSize   = 1500;

	pMbufSrc_1  = mvOsMalloc(sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	pFragsSrc_1 = mvOsMalloc(sizeof(MV_BUF_INFO) * bufNum * CESA_DEF_REQ_SIZE);

	pMbufDst_1  = mvOsMalloc(sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	pFragsDst_1 = mvOsMalloc(sizeof(MV_BUF_INFO) * bufNum * CESA_DEF_REQ_SIZE);

	if ((pMbufSrc_1 == NULL) || (pFragsSrc_1 == NULL) || (pMbufDst_1 == NULL)
		|| (pFragsDst_1 == NULL)) {
		mvOsPrintf(" Can't malloc Src and Dst pMbuf and pFrags structures.\n");
		return;
		}

	memset(pMbufSrc_1,  0, sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	memset(pFragsSrc_1, 0, sizeof(MV_BUF_INFO) * bufNum * CESA_DEF_REQ_SIZE);

	memset(pMbufDst_1,  0, sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	memset(pFragsDst_1, 0, sizeof(MV_BUF_INFO) * bufNum * CESA_DEF_REQ_SIZE);

	idx = 0;
	for (i = 0; i < CESA_DEF_REQ_SIZE; i++) {
		pBuf_1 = mvOsIoCachedMalloc(cesaOSHandle, bufSize * bufNum * 2,
					  &cesaBufs_1[i].bufPhysAddr, &cesaBufs_1[i].memHandle);
		if (pBuf_1 == NULL) {
			mvOsPrintf("testStart: Can't malloc %d bytes for pBuf\n", bufSize * bufNum * 2);
			return;
		}

		memset(pBuf_1, 0, bufSize * bufNum * 2);
		mvOsCacheFlush(cesaOSHandle, pBuf_1, bufSize * bufNum * 2);
		if (pBuf_1 == NULL) {
			mvOsPrintf("Can't allocate %d bytes for req_%d buffers\n",
				   bufSize * bufNum * 2, i);
			return;
}

		cesaBufs_1[i].bufVirtPtr = (MV_U8 *) pBuf_1;
		cesaBufs_1[i].bufSize = bufSize * bufNum * 2;

		cesaCmdArray_1[i].pSrc = &pMbufSrc_1[i];
		cesaCmdArray_1[i].pSrc->pFrags = &pFragsSrc_1[idx];
		cesaCmdArray_1[i].pSrc->numFrags = bufNum;
		cesaCmdArray_1[i].pSrc->mbufSize = 0;

		cesaCmdArray_1[i].pDst = &pMbufDst_1[i];
		cesaCmdArray_1[i].pDst->pFrags = &pFragsDst_1[idx];
		cesaCmdArray_1[i].pDst->numFrags = bufNum;
		cesaCmdArray_1[i].pDst->mbufSize = 0;

		for (j = 0; j < bufNum; j++) {
			cesaCmdArray_1[i].pSrc->pFrags[j].bufVirtPtr = (MV_U8 *) pBuf_1;
			cesaCmdArray_1[i].pSrc->pFrags[j].bufSize = bufSize;
			pBuf_1 += bufSize;
			cesaCmdArray_1[i].pDst->pFrags[j].bufVirtPtr = (MV_U8 *) pBuf_1;

			cesaCmdArray_1[i].pDst->pFrags[j].bufSize = bufSize;
			pBuf_1 += bufSize;
		}
		idx += bufNum;
	}

	cesaMbufArray_1 = mvOsMalloc(sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);
	if (cesaMbufArray_1 == NULL) {
		mvOsPrintf("Can't allocate %d bytes of memory\n",
			   (int)(sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE));
		return;
	}
	memset(cesaMbufArray_1, 0, sizeof(MV_CESA_MBUF) * CESA_DEF_REQ_SIZE);

	cesaPrivArray_1 = mvOsMalloc(sizeof(MV_NFP_SEC_CESA_PRIV_L2FW) * (CESA_DEF_REQ_SIZE + MV_NFP_SEC_REQ_Q_SIZE));
	memset(cesaPrivArray_1, 0, sizeof(MV_NFP_SEC_CESA_PRIV_L2FW) * (CESA_DEF_REQ_SIZE + MV_NFP_SEC_REQ_Q_SIZE));

	pPktInfoNewArray_0 = mvOsMalloc(sizeof(MV_PKT_INFO) * MV_NFP_SEC_REQ_Q_SIZE);

	if (!pPktInfoNewArray_0) {
		printk(KERN_INFO "mvOsMalloc() failed in %s\n", __func__);
		return;
	}

	pBufInfoArray_0 = mvOsMalloc(sizeof(MV_BUF_INFO) * MV_NFP_SEC_REQ_Q_SIZE);
	if (!pBufInfoArray_0) {
		printk(KERN_INFO "could not allocate MV_BUF_INFO in %s\n", __func__);
		return;
	}

	pPktInfoNewArray_1 = mvOsMalloc(sizeof(MV_PKT_INFO) * MV_NFP_SEC_REQ_Q_SIZE);

	if (!pPktInfoNewArray_1) {
		printk(KERN_INFO "mvOsMalloc() failed in %s\n", __func__);
		return;
	}
	pBufInfoArray_1 = mvOsMalloc(sizeof(MV_BUF_INFO) * MV_NFP_SEC_REQ_Q_SIZE);
	if (!pBufInfoArray_0) {
		printk(KERN_INFO "could not allocate MV_BUF_INFO in %s\n", __func__);
		return;
	}
	printk(KERN_INFO "start finished in %s\n", __func__);
}

static irqreturn_t nfp_sec_interrupt_handler_0(int irq, void *arg)
{
	MV_CESA_RESULT  	result;
	MV_STATUS               status;
	MV_U8 chan = 0;

	MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);

	while (1) {
	/* Get Ready requests */

	status = mvCesaReadyGet(chan, &result);
	if (status != MV_OK)
			break;

	nfp_sec_complete_out((unsigned long)((MV_NFP_SEC_CESA_PRIV_L2FW *)result.pReqPrv));
	}
	return IRQ_HANDLED;
}

static irqreturn_t nfp_sec_interrupt_handler_1(int irq, void *arg)
{
	MV_CESA_RESULT  	result;
	MV_STATUS           status;
	MV_U8 chan = 1;
    MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);
	while (1) {
	/* Get Ready requests */
	status = mvCesaReadyGet(chan, &result);
	if (status != MV_OK)
		break;

	nfp_sec_complete_out((unsigned long)((MV_NFP_SEC_CESA_PRIV_L2FW *)result.pReqPrv));
	}

	return IRQ_HANDLED;
}

void openCesaSession(void)
{
	unsigned char sha1Key[]  = {0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
								0x24, 0x68, 0xac, 0xe0, 0x24, 0x68, 0xac, 0xe0,
								0x13, 0x57, 0x9b, 0xdf};
	/* sizeof(cryptoKey) should be 128 for AES-128 */
	unsigned char cryptoKey[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
									0x02, 0x46, 0x8a, 0xce, 0x13, 0x57, 0x9b, 0xdf};

	int i;
	MV_NFP_SEC_SA_ENTRY sa;
	MV_CESA_OPEN_SESSION os;
	unsigned short digest_size = 0;
	memset(&sa, 0, sizeof(MV_NFP_SEC_SA_ENTRY));
	memset(&os, 0, sizeof(MV_CESA_OPEN_SESSION));

	os.operation 		= MV_CESA_MAC_THEN_CRYPTO;
	os.cryptoAlgorithm  = MV_CESA_CRYPTO_AES;
	os.macMode  		= MV_CESA_MAC_HMAC_SHA1;
	digest_size 		= MV_CESA_SHA1_DIGEST_SIZE;
	os.cryptoMode 		= MV_CESA_CRYPTO_ECB;
	for (i = 0; i < sizeof(cryptoKey); i++)
		os.cryptoKey[i] = cryptoKey[i];

	os.cryptoKeyLength = sizeof(cryptoKey);

	for (i = 0; i < sizeof(sha1Key); i++)
		os.macKey[i] = sha1Key[i];
	os.macKeyLength = sizeof(sha1Key);
	os.digestSize = digest_size;

	if (mvCesaSessionOpen(&os, (short *)&(sa.sid)))
		printk(KERN_INFO "mvCesaSessionOpen failed in %s\n", __func__);
}

void l2fw_esp_set(int enableEsp)
{
	if (enableEsp) {
		openCesaSession();
		printk(KERN_INFO "calling cesaStart() in %s\n", __func__);
		cesaStart();
	} else
		printk(KERN_INFO "enableEsp=%d disabling ESP in %s\n", enableEsp, __func__);
	espEnabled = enableEsp;
}

int cesa_init(void)
{
	u8 chan = 0;
	int i;
	const char *irq_str[] = {"cesa0", "cesa1"};
	printk(KERN_INFO "in %s\n", __func__);
	for (i = 0; i < 2; i++)
		spin_lock_init(&cesa_lock[i]);
	if (mvCtrlPwrClckGet(CESA_UNIT_ID, 0) == MV_FALSE)
		return 0;
	if (MV_OK != my_mvSysCesaInit(1, 256, NULL)) {
		printk(KERN_INFO "%s,%d: mvCesaInit Failed. \n", __FILE__, __LINE__);
		return EINVAL;
	}

	/* clear and unmask Int */
		MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);
	MV_REG_WRITE(MV_CESA_ISR_MASK_REG(chan), MV_CESA_CAUSE_ACC_DMA_MASK);
	if (request_irq(CESA_IRQ(0), nfp_sec_interrupt_handler_0,
							(IRQF_DISABLED) , irq_str[chan], NULL)) {
				printk(KERN_INFO "%s,%d: cannot assign irq %x\n", __FILE__, __LINE__, CESA_IRQ(chan));
			return EINVAL;
		}

	chan = 1;
	MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);
	MV_REG_WRITE(MV_CESA_ISR_MASK_REG(chan), MV_CESA_CAUSE_ACC_DMA_MASK);

	if (request_irq(CESA_IRQ(1), nfp_sec_interrupt_handler_1,
							(IRQF_DISABLED) , irq_str[chan], NULL)) {
				printk(KERN_INFO "%s,%d: cannot assign irq %x\n", __FILE__, __LINE__, CESA_IRQ(chan));
		return EINVAL;
}

	atomic_set(&req_count[0], 0);
	atomic_set(&req_count[1], 0);
	mvOsPrintf("MV_CESA_TDMA_CTRL_REG address 0 %08x\n\n", MV_CESA_TDMA_CTRL_REG(0));
	mvOsPrintf("MV_CESA_TDMA_CTRL_REG address 1 %08x\n\n", MV_CESA_TDMA_CTRL_REG(1));
	mvOsPrintf("MV_CESA_TDMA_CTRL_REG(0)  %08x\n",
		MV_REG_READ(MV_CESA_TDMA_CTRL_REG(0)));
	mvOsPrintf("MV_CESA_TDMA_CTRL_REG(1)  %08x\n",
		MV_REG_READ(MV_CESA_TDMA_CTRL_REG(1)));

	memset(&sa, 0, sizeof(MV_NFP_SEC_SA_ENTRY));
	sa.digestSize = MV_CESA_SHA1_DIGEST_SIZE;
	sa.ivSize = MV_CESA_AES_BLOCK_SIZE;
	sa.spi = 3;

	sa.tunProt = MV_NFP_SEC_TUNNEL;
	sa.encap   = MV_NFP_SEC_ESP;
	sa.seqNum  = 4;
	sa.tunnelHdr.sIp = 0x6400A8C0;
	sa.tunnelHdr.dIp = 0x6401A8C0;
	sa.tunnelHdr.outIfIndex = 0;
	sa.lifeTime = 0;

	sa.secOp = MV_NFP_SEC_ENCRYPT;
	strcpy(sa.tunnelHdr.dstMac, "aabbccddeeff");
	strcpy(sa.tunnelHdr.srcMac, "abacadaeafaa");

	return 0;
}

/* l2sec/mv_eth_l2sec.h */

#ifndef L2SEC_MV_ETH_L2SEC_H
#define L2SEC_MV_ETH_L2SEC_H

#include "mvOs.h"
#include "cesa/mvCesa.h"

#include "mv_neta/l2fw/mv_eth_l2fw.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "mv_neta/net_dev/mv_netdev.h"

/* Taken from mvNfpSec.h */
/* IPSec defines */
#define MV_NFP_SEC_MAX_PACKET		1540
#define MV_NFP_SEC_ENC_BLOCK_SIZE	16

#define MV_NFP_SEC_ESP_OFFSET		34

/* IPSec Enumerators */
typedef enum {
	MV_NFP_SEC_TUNNEL = 0,
	MV_NFP_SEC_TRANSPORT,
} MV_NFP_SEC_PROT;

typedef enum {
	MV_NFP_SEC_ESP = 0,
	MV_NFP_SEC_AH,
} MV_NFP_SEC_ENCAP;


typedef enum {
	MV_NFP_SEC_ENCRYPT = 0,
	MV_NFP_SEC_DECRYPT,
} MV_NFP_SEC_OP;

typedef struct _mv_nfp_sa_stats {
	MV_U32 encrypt;
	MV_U32 decrypt;
	MV_U32 rejected;	/* slow path */
	MV_U32 dropped;		/* packet drop */
	MV_U32 bytes;
} MV_NFP_SA_STATS;

/* IPSec Structures */
typedef struct _mv_nfp_sec_tunnel_hdr {
	MV_U32 sIp;			/* BE */
	MV_U32 dIp;			/* BE */
	/* dstMac should be 2 byte aligned */
	MV_U8 dstMac[MV_MAC_ADDR_SIZE];	/* BE */
	MV_U8 srcMac[MV_MAC_ADDR_SIZE];	/* BE */
	MV_U8 outIfIndex;
} MV_NFP_SEC_TUNNEL_HDR;

typedef struct _mv_nfp_sec_sa_entry {
	MV_U32 spi;			/* BE */
	MV_NFP_SEC_PROT tunProt;
	MV_NFP_SEC_ENCAP encap;
	MV_U16 sid;
	MV_U32 seqNum;			/* LE  */
	MV_NFP_SEC_TUNNEL_HDR tunnelHdr;
	MV_U32 lifeTime;
	MV_U8 ivSize;
	MV_U8 cipherBlockSize;
	MV_U8 digestSize;
	MV_NFP_SEC_OP secOp;
	MV_NFP_SA_STATS stats;
} MV_NFP_SEC_SA_ENTRY;

typedef struct _mv_nfp_sec_cesa_priv {
	MV_NFP_SEC_SA_ENTRY *pSaEntry;
	MV_PKT_INFO *pPktInfo;
	MV_U8 orgDigest[MV_CESA_MAX_DIGEST_SIZE];
	MV_CESA_COMMAND *pCesaCmd;
} MV_NFP_SEC_CESA_PRIV;

int cesaChanPort[CONFIG_MV_ETH_PORTS_NUM];

#define CESA_0    0
#define CESA_1    1
/* for future - handle by CPU */
#define CESA_NONE 2

#define MV_NFP_SEC_REQ_Q_SIZE 1000
#define CESA_DEF_REQ_SIZE       (256*4)
int counterNoResources[4]  = {0, 0, 0, 0};
spinlock_t cesa_lock[2];

extern u32 mv_crypto_virt_base_get(u8 chan);
static MV_PKT_INFO *pPktInfoNewArray_0;
static MV_PKT_INFO *pPktInfoNewArray_1;
static MV_BUF_INFO *pBufInfoArray_0;
static MV_BUF_INFO *pBufInfoArray_1;

MV_BUF_INFO cesaBufs_0[CESA_DEF_REQ_SIZE];
MV_BUF_INFO cesaBufs_1[CESA_DEF_REQ_SIZE];

static int cesaPrivIndx_0 = 0;
static int cesaPrivIndx_1 = 0;

static int cesaCmdIndx_0 = 0;
static int cesaCmdIndx_1 = 0;

typedef struct _mv_nfp_sec_cesa_priv_l2fw {
	MV_NFP_SEC_SA_ENTRY *pSaEntry;
	MV_PKT_INFO *pPktInfo;
	MV_U8 orgDigest[MV_CESA_MAX_DIGEST_SIZE];
	MV_CESA_COMMAND *pCesaCmd;
	struct eth_pbuf *pPkt;
	int ifout;
	int ownerId;
	int inPort;
} MV_NFP_SEC_CESA_PRIV_L2FW;

MV_NFP_SEC_CESA_PRIV_L2FW *cesaPrivArray_0;
MV_NFP_SEC_CESA_PRIV_L2FW *cesaPrivArray_1;

void *cesaOSHandle = NULL;
static MV_CESA_MBUF *cesaMbufArray_0;
static MV_CESA_MBUF *cesaMbufArray_1;

static MV_CESA_COMMAND *cesaCmdArray_0;
static MV_CESA_COMMAND *cesaCmdArray_1;


static MV_NFP_SEC_SA_ENTRY sa;
atomic_t req_count[2];
int l2fw_set_cesa_chan(int port, int cesaChan);
int cesa_init(void);


/* from mv_hal/eth/gbe/mvEthRegs.h */

/* Tx descriptor bits */
#define ETH_TX_ERROR_CODE_OFFSET            1
#define ETH_TX_ERROR_CODE_MASK              (3<<ETH_TX_ERROR_CODE_OFFSET)
#define ETH_TX_LATE_COLLISION_ERROR         (0<<ETH_TX_ERROR_CODE_OFFSET)
#define ETH_TX_UNDERRUN_ERROR               (1<<ETH_TX_ERROR_CODE_OFFSET)
#define ETH_TX_EXCESSIVE_COLLISION_ERROR    (2<<ETH_TX_ERROR_CODE_OFFSET)

#define ETH_TX_LLC_SNAP_FORMAT_BIT          9
#define ETH_TX_LLC_SNAP_FORMAT_MASK         (1<<ETH_TX_LLC_SNAP_FORMAT_BIT)

#define ETH_TX_IP_FRAG_BIT                  10
#define ETH_TX_IP_FRAG_MASK                 (1<<ETH_TX_IP_FRAG_BIT)
#define ETH_TX_IP_FRAG                      (0<<ETH_TX_IP_FRAG_BIT)
#define ETH_TX_IP_NO_FRAG                   (1<<ETH_TX_IP_FRAG_BIT)

#define ETH_TX_IP_HEADER_LEN_OFFSET         11
#define ETH_TX_IP_HEADER_LEN_ALL_MASK       (0xF<<ETH_TX_IP_HEADER_LEN_OFFSET)
#define ETH_TX_IP_HEADER_LEN_MASK(len)      ((len)<<ETH_TX_IP_HEADER_LEN_OFFSET)

#define ETH_TX_VLAN_TAGGED_FRAME_BIT        15
#define ETH_TX_VLAN_TAGGED_FRAME_MASK       (1<<ETH_TX_VLAN_TAGGED_FRAME_BIT)

#define ETH_TX_L4_TYPE_BIT                  16
#define ETH_TX_L4_TCP_TYPE                  (0<<ETH_TX_L4_TYPE_BIT)
#define ETH_TX_L4_UDP_TYPE                  (1<<ETH_TX_L4_TYPE_BIT)

#define ETH_TX_GENERATE_L4_CHKSUM_BIT       17
#define ETH_TX_GENERATE_L4_CHKSUM_MASK      (1<<ETH_TX_GENERATE_L4_CHKSUM_BIT)

#define ETH_TX_GENERATE_IP_CHKSUM_BIT       18
#define ETH_TX_GENERATE_IP_CHKSUM_MASK      (1<<ETH_TX_GENERATE_IP_CHKSUM_BIT)

#define ETH_TX_ZERO_PADDING_BIT             19
#define ETH_TX_ZERO_PADDING_MASK            (1<<ETH_TX_ZERO_PADDING_BIT)

#define ETH_TX_LAST_DESC_BIT                20
#define ETH_TX_LAST_DESC_MASK               (1<<ETH_TX_LAST_DESC_BIT)

#define ETH_TX_FIRST_DESC_BIT               21
#define ETH_TX_FIRST_DESC_MASK              (1<<ETH_TX_FIRST_DESC_BIT)

#define ETH_TX_GENERATE_CRC_BIT             22
#define ETH_TX_GENERATE_CRC_MASK            (1<<ETH_TX_GENERATE_CRC_BIT)

#define ETH_TX_ENABLE_INTERRUPT_BIT         23
#define ETH_TX_ENABLE_INTERRUPT_MASK        (1<<ETH_TX_ENABLE_INTERRUPT_BIT)

#define ETH_TX_AUTO_MODE_BIT                30
#define ETH_TX_AUTO_MODE_MASK               (1<<ETH_TX_AUTO_MODE_BIT)


inline MV_VOID mvNfpSecBuildIPTunnel(MV_PKT_INFO *pPktInfo, MV_NFP_SEC_SA_ENTRY *pSAEntry)
{
	MV_IP_HEADER *pIpHdr, *pIntIpHdr;
	MV_U16 newIpTotalLength;

	newIpTotalLength = pPktInfo->pFrags[0].dataSize - sizeof(MV_802_3_HEADER);

	pIpHdr = (MV_IP_HEADER *) (pPktInfo->pFrags[0].bufVirtPtr + sizeof(MV_802_3_HEADER));
	pIntIpHdr = (MV_IP_HEADER *) ((MV_U8 *) (pIpHdr) + sizeof(MV_IP_HEADER) + sizeof(MV_ESP_HEADER) +
				      pSAEntry->ivSize);

	/* TBD - review below settings in RFC */
	pIpHdr->version = 0x45;
	pIpHdr->tos = 0;
	pIpHdr->checksum = 0;
	pIpHdr->totalLength = MV_16BIT_BE(newIpTotalLength);
	pIpHdr->identifier = 0;
	pIpHdr->fragmentCtrl = 0;
	pIpHdr->ttl = pIntIpHdr->ttl - 1;
	pIpHdr->protocol = MV_IP_PROTO_ESP;
	pIpHdr->srcIP = pSAEntry->tunnelHdr.sIp;
	pIpHdr->dstIP = pSAEntry->tunnelHdr.dIp;

	pPktInfo->status = ETH_TX_IP_NO_FRAG | ETH_TX_GENERATE_IP_CHKSUM_MASK | (0x5 << ETH_TX_IP_HEADER_LEN_OFFSET);

	return;
}


/* Append sequence number and spi, save some space for IV */
inline MV_VOID mvNfpSecBuildEspHdr(MV_PKT_INFO *pPktInfo, MV_NFP_SEC_SA_ENTRY *pSAEntry)
{
	MV_ESP_HEADER *pEspHdr;

	pEspHdr = (MV_ESP_HEADER *) (pPktInfo->pFrags[0].bufVirtPtr + sizeof(MV_802_3_HEADER) + sizeof(MV_IP_HEADER));
	pEspHdr->spi = pSAEntry->spi;
	pSAEntry->seqNum = (pSAEntry->seqNum++);
	pEspHdr->seqNum = MV_32BIT_BE(pSAEntry->seqNum);
}

inline MV_VOID mvNfpSecBuildMac(MV_PKT_INFO *pPktInfo, MV_NFP_SEC_SA_ENTRY *pSAEntry)
{
	MV_802_3_HEADER *pMacHdr;

	pMacHdr = (MV_802_3_HEADER *) ((MV_U8 *) (pPktInfo->pFrags[0].bufVirtPtr));
	memcpy(pMacHdr, &pSAEntry->tunnelHdr.dstMac, 12);
	pMacHdr->typeOrLen = 0x08;	/* stands for IP protocol code 16bit swapped */
	return;
}


inline MV_STATUS mvSecEspProcess_0(struct eth_pbuf *pPkt, MV_PKT_INFO *pPktInfo,
							MV_NFP_SEC_SA_ENTRY *pSAEntry, struct eth_port *newpp,
							MV_U8 channel, int inPort)
{
	MV_CESA_COMMAND	*pCesaCmd;
	MV_CESA_MBUF *pCesaMbuf;
	MV_NFP_SEC_CESA_PRIV_L2FW *pCesaPriv;
	MV_STATUS status;
	MV_IP_HEADER *pIpHdr;
	MV_BUF_INFO  *pBuf;

	pCesaCmd  = &cesaCmdArray_0[cesaCmdIndx_0];
	pCesaMbuf = &cesaMbufArray_0[cesaCmdIndx_0];
	cesaCmdIndx_0++;

	cesaCmdIndx_0 %= CESA_DEF_REQ_SIZE;
	pCesaPriv = &cesaPrivArray_0[cesaPrivIndx_0++];

	cesaPrivIndx_0 = cesaPrivIndx_0%(CESA_DEF_REQ_SIZE + MV_NFP_SEC_REQ_Q_SIZE);

	pCesaPriv->pPktInfo = pPktInfo;
	pCesaPriv->pSaEntry = pSAEntry;
	pCesaPriv->pCesaCmd = pCesaCmd;

	pCesaPriv->pPkt   = pPkt;
	pCesaPriv->ifout  = newpp->port;
	pCesaPriv->inPort = inPort;
	/*
	 *  Fix, encrypt/decrypt the IP payload only, --BK 20091027
	 */
	pBuf = pPktInfo->pFrags;
	pIpHdr = (MV_IP_HEADER *)(pBuf->bufVirtPtr + sizeof(MV_802_3_HEADER));
	pBuf->dataSize = MV_16BIT_BE(pIpHdr->totalLength) + sizeof(MV_802_3_HEADER);
	/* after next command, pBuf->bufVirtPtr will point to ESP */
	pBuf->bufVirtPtr += MV_NFP_SEC_ESP_OFFSET;
	pBuf->bufPhysAddr += MV_NFP_SEC_ESP_OFFSET;
	pBuf->dataSize -= MV_NFP_SEC_ESP_OFFSET;

	pBuf->bufAddrShift -= MV_NFP_SEC_ESP_OFFSET;
	pCesaMbuf->pFrags = pPktInfo->pFrags;
	pCesaMbuf->numFrags = 1;
	pCesaMbuf->mbufSize = pBuf->dataSize;

	pCesaMbuf->pFrags->bufSize = pBuf->dataSize;

	pCesaCmd->pReqPrv = (MV_VOID *)pCesaPriv;
	pCesaCmd->sessionId = pSAEntry->sid;
	pCesaCmd->pSrc = pCesaMbuf;
	pCesaCmd->pDst = pCesaMbuf;
	pCesaCmd->skipFlush = MV_TRUE;

	/* Assume ESP */
	pCesaCmd->cryptoOffset = sizeof(MV_ESP_HEADER) + pSAEntry->ivSize;
	pCesaCmd->cryptoLength =  pBuf->dataSize - (sizeof(MV_ESP_HEADER)
				  + pSAEntry->ivSize + pSAEntry->digestSize);
	pCesaCmd->ivFromUser = 0; /* relevant for encode only */
	pCesaCmd->ivOffset = sizeof(MV_ESP_HEADER);
	pCesaCmd->macOffset = 0;
	pCesaCmd->macLength = pBuf->dataSize - pSAEntry->digestSize;
	if ((pCesaCmd->digestOffset != 0) && ((pCesaCmd->digestOffset%4)))  {
		printk(KERN_INFO "pBuf->dataSize=%d pSAEntry->digestSize=%d in %s\n",
			pBuf->dataSize, pSAEntry->digestSize, __func__);
		printk(KERN_INFO "pCesaCmd->digestOffset=%d in %s\n",
			pCesaCmd->digestOffset, __func__);
	}
	pCesaCmd->digestOffset = pBuf->dataSize - pSAEntry->digestSize ;

	disable_irq(CESA_IRQ(channel));

	status = mvCesaAction(channel, pCesaCmd);
	enable_irq(CESA_IRQ(channel));
	if (status != MV_OK) {
		pSAEntry->stats.rejected++;
		mvOsPrintf("%s: mvCesaAction failed %d\n", __func__, status);
	}
	return status;
}

inline MV_STATUS mvSecEspProcess_1(struct eth_pbuf *pPkt, MV_PKT_INFO *pPktInfo,
						  MV_NFP_SEC_SA_ENTRY *pSAEntry, struct eth_port *newpp,
						  MV_U8 channel, int inPort)

{
	MV_CESA_COMMAND	*pCesaCmd;
	MV_CESA_MBUF *pCesaMbuf;
	MV_NFP_SEC_CESA_PRIV_L2FW *pCesaPriv;
	MV_STATUS status;
	MV_IP_HEADER *pIpHdr;
	MV_BUF_INFO  *pBuf;
	pCesaCmd  = &cesaCmdArray_1[cesaCmdIndx_1];
	pCesaMbuf = &cesaMbufArray_1[cesaCmdIndx_1];
	cesaCmdIndx_1++;
	cesaCmdIndx_1 %= CESA_DEF_REQ_SIZE;
	pCesaPriv = &cesaPrivArray_1[cesaPrivIndx_1++];
	cesaPrivIndx_1 = cesaPrivIndx_1%(CESA_DEF_REQ_SIZE + MV_NFP_SEC_REQ_Q_SIZE);

	pCesaPriv->pPktInfo = pPktInfo;
	pCesaPriv->pSaEntry = pSAEntry;
	pCesaPriv->pCesaCmd = pCesaCmd;

	pCesaPriv->pPkt   = pPkt;
	pCesaPriv->ifout  = newpp->port;
	pCesaPriv->inPort = inPort;
	/*
	 *  Fix, encrypt/decrypt the IP payload only, --BK 20091027
	 */
	pBuf = pPktInfo->pFrags;
	pIpHdr = (MV_IP_HEADER *)(pBuf->bufVirtPtr + sizeof(MV_802_3_HEADER));
	pBuf->dataSize = MV_16BIT_BE(pIpHdr->totalLength) + sizeof(MV_802_3_HEADER);
	/* after next command, pBuf->bufVirtPtr will point to ESP */
	pBuf->bufVirtPtr += MV_NFP_SEC_ESP_OFFSET;
	pBuf->bufPhysAddr += MV_NFP_SEC_ESP_OFFSET;
	pBuf->dataSize -= MV_NFP_SEC_ESP_OFFSET;
	pBuf->bufAddrShift -= MV_NFP_SEC_ESP_OFFSET;
	pCesaMbuf->pFrags = pPktInfo->pFrags;
	pCesaMbuf->numFrags = 1;
	pCesaMbuf->mbufSize = pBuf->dataSize;
	pCesaMbuf->pFrags->bufSize = pBuf->dataSize;

	pCesaCmd->pReqPrv = (MV_VOID *)pCesaPriv;
	pCesaCmd->sessionId = pSAEntry->sid;
	pCesaCmd->pSrc = pCesaMbuf;
	pCesaCmd->pDst = pCesaMbuf;
	pCesaCmd->skipFlush = MV_TRUE;

	/* Assume ESP */
	pCesaCmd->cryptoOffset = sizeof(MV_ESP_HEADER) + pSAEntry->ivSize;
	pCesaCmd->cryptoLength =  pBuf->dataSize - (sizeof(MV_ESP_HEADER)
				  + pSAEntry->ivSize + pSAEntry->digestSize);
	pCesaCmd->ivFromUser = 0; /* relevant for encode only */
	pCesaCmd->ivOffset = sizeof(MV_ESP_HEADER);
	pCesaCmd->macOffset = 0;
	pCesaCmd->macLength = pBuf->dataSize - pSAEntry->digestSize;
	if ((pCesaCmd->digestOffset != 0) && ((pCesaCmd->digestOffset%4)))  {
		printk(KERN_INFO "pBuf->dataSize=%d pSAEntry->digestSize=%d in %s\n",
			pBuf->dataSize, pSAEntry->digestSize, __func__);
		printk(KERN_INFO "pCesaCmd->digestOffset=%d in %s\n",
			pCesaCmd->digestOffset, __func__);
	}
	pCesaCmd->digestOffset = pBuf->dataSize - pSAEntry->digestSize ;

	disable_irq(CESA_IRQ(channel));

	status = mvCesaAction(channel, pCesaCmd);
	enable_irq(CESA_IRQ(channel));
	if (status != MV_OK) {
		pSAEntry->stats.rejected++;
		mvOsPrintf("%s: mvCesaAction failed %d\n", __func__, status);
	}

	return status;
}

inline MV_STATUS mvSecOutgoing(struct eth_pbuf *pkt, MV_PKT_INFO *pPktInfo,
						MV_NFP_SEC_SA_ENTRY *pSAEntry, struct eth_port *new_pp,
						int inPort, MV_U8 chan)
{
	MV_U8 *pTmp;
	MV_U32 cryptoSize, encBlockMod, dSize;
	MV_BUF_INFO *pBuf = pPktInfo->pFrags;
	/* CESA Q is full drop. */
	if (cesaReqResources[chan] <= 1) {
		counterNoResources[inPort]++;
		return MV_DROPPED;
	}
	cryptoSize = pBuf->dataSize - sizeof(MV_802_3_HEADER);

	/* Align buffer address to beginning of new packet - TBD handle VLAN tag, LLC */
	dSize = pSAEntry->ivSize + sizeof(MV_ESP_HEADER) + sizeof(MV_IP_HEADER);
	pBuf->bufVirtPtr -= dSize;
	pBuf->bufPhysAddr -= dSize;
	pBuf->dataSize += dSize;
	pBuf->bufAddrShift += dSize;

	encBlockMod = (cryptoSize % MV_NFP_SEC_ENC_BLOCK_SIZE);
	/* leave space for padLen + Protocol */
	if (encBlockMod > 14) {
		encBlockMod =  MV_NFP_SEC_ENC_BLOCK_SIZE - encBlockMod;
		encBlockMod += MV_NFP_SEC_ENC_BLOCK_SIZE;
	} else
		encBlockMod =  MV_NFP_SEC_ENC_BLOCK_SIZE - encBlockMod;
	/* expected frame size */
	dSize = pBuf->dataSize + encBlockMod + pSAEntry->digestSize;

	pBuf->dataSize += encBlockMod;
	pTmp = pBuf->bufVirtPtr + pBuf->dataSize;
	memset(pTmp - encBlockMod, 0, encBlockMod - 2);
	*((MV_U8 *)(pTmp-2)) = (MV_U8)(encBlockMod-2);
	*((MV_U8 *)(pTmp-1)) = (MV_U8)4;

	pBuf->dataSize += pSAEntry->digestSize;

	mvNfpSecBuildEspHdr(pPktInfo, pSAEntry);
	mvNfpSecBuildIPTunnel(pPktInfo, pSAEntry);
	mvNfpSecBuildMac(pPktInfo, pSAEntry);

	/* flush & invalidate new MAC, IP, & ESP headers + old ip*/
	dSize = pBuf->bufAddrShift + sizeof(MV_IP_HEADER) + sizeof(MV_802_3_HEADER);

	if (chan == 0)
	  return mvSecEspProcess_0(pkt, pPktInfo, pSAEntry, new_pp, chan, inPort);
	else
	  return mvSecEspProcess_1(pkt, pPktInfo, pSAEntry, new_pp, chan, inPort);
}

inline MV_STATUS handleEsp(struct eth_pbuf *pkt, struct neta_rx_desc *rx_desc,
							struct eth_port  *new_pp, int inPort)
{
	MV_STATUS res;
	int chan = 	cesaChanPort[inPort];

	spin_lock(&cesa_lock[chan]);

	if (chan == 0) {
		pBufInfoArray_0[cesaCmdIndx_0].bufAddrShift = 0;
		pBufInfoArray_0[cesaCmdIndx_0].dataSize    = pkt->bytes;

		pBufInfoArray_0[cesaCmdIndx_0].bufSize     = pkt->bytes;
		pBufInfoArray_0[cesaCmdIndx_0].bufVirtPtr  = pkt->pBuf + pkt->offset + MV_ETH_MH_SIZE;

		pBufInfoArray_0[cesaCmdIndx_0].bufPhysAddr = mvOsIoVirtToPhy(NULL, pkt->pBuf + pkt->offset + MV_ETH_MH_SIZE);
		pBufInfoArray_0[cesaCmdIndx_0].memHandle   = 0;

		pPktInfoNewArray_0[cesaCmdIndx_0].pFrags = &pBufInfoArray_0[cesaCmdIndx_0];
		pPktInfoNewArray_0[cesaCmdIndx_0].numFrags = 1;
	} else {
		pBufInfoArray_1[cesaCmdIndx_1].bufAddrShift = 0;
		pBufInfoArray_1[cesaCmdIndx_1].dataSize    = pkt->bytes;

		pBufInfoArray_1[cesaCmdIndx_1].bufSize     = pkt->bytes;
		pBufInfoArray_1[cesaCmdIndx_1].bufVirtPtr  = pkt->pBuf + pkt->offset + MV_ETH_MH_SIZE;

		pBufInfoArray_1[cesaCmdIndx_1].bufPhysAddr = mvOsIoVirtToPhy(NULL, pkt->pBuf + pkt->offset + MV_ETH_MH_SIZE);
		pBufInfoArray_1[cesaCmdIndx_1].memHandle   = 0;

		pPktInfoNewArray_1[cesaCmdIndx_1].pFrags = &pBufInfoArray_1[cesaCmdIndx_1];
		pPktInfoNewArray_1[cesaCmdIndx_1].numFrags = 1;
	}

	if (chan == 0)
		res = mvSecOutgoing(pkt, &pPktInfoNewArray_0[cesaCmdIndx_0], &sa, new_pp, inPort, chan);
	else
		res = mvSecOutgoing(pkt, &pPktInfoNewArray_1[cesaCmdIndx_1], &sa, new_pp, inPort, chan);

	spin_unlock(&cesa_lock[chan]);
	return res;
}

void l2fw_stats(void)
{
	int i;
	for (i = 0; i < 4; i++) {
		mvOsPrintf("number of Cesa No Resources error is port[%d]=%d \n", i, counterNoResources[i]);
		counterNoResources[i] = 0;
	}
}

#endif

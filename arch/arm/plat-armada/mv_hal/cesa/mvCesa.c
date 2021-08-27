#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSysCesaConfig.h"
#include "mvCesaRegs.h"
#include "mvCesa.h"
#include "AES/mvAes.h"
#include "mvMD5.h"
#include "mvSHA1.h"
#include "mvSHA256.h"

#undef CESA_DEBUG

MV_CESA_STATS cesaStats;
MV_16 cesaLastSid[MV_CESA_CHANNELS];
MV_CESA_SA **pCesaSAD = NULL;
MV_U32 cesaMaxSA = 0;
MV_CESA_REQ *pCesaReqFirst[MV_CESA_CHANNELS];
MV_CESA_REQ *pCesaReqLast[MV_CESA_CHANNELS];
MV_CESA_REQ *pCesaReqEmpty[MV_CESA_CHANNELS];
MV_CESA_REQ *pCesaReqProcess[MV_CESA_CHANNELS];
#ifdef MV_CESA_INT_COALESCING_SUPPORT
MV_CESA_REQ *pCesaReqStartNext[MV_CESA_CHANNELS];
MV_CESA_REQ *pCesaReqProcessCurr[MV_CESA_CHANNELS];
#endif

int cesaQueueDepth[MV_CESA_CHANNELS];
int cesaReqResources[MV_CESA_CHANNELS];

MV_CESA_SRAM_MAP *cesaSramVirtPtr[MV_CESA_CHANNELS];
void *cesaOsHandle = NULL;
MV_U16 ctrlModel;
MV_U8 ctrlRev;
MV_U32 sha2CmdVal;

#ifdef MV_CESA_CHAIN_MODE

MV_U32 cesaChainLength[MV_CESA_CHANNELS];
int chainReqNum[MV_CESA_CHANNELS];
MV_U32 chainIndex[MV_CESA_CHANNELS];
MV_CESA_REQ *pNextActiveChain[MV_CESA_CHANNELS];
MV_CESA_REQ *pEndCurrChain[MV_CESA_CHANNELS];
MV_BOOL isFirstReq[MV_CESA_CHANNELS];

#endif  

static MV_CESA_HAL_DATA cesaHalData;

static INLINE MV_U8 *mvCesaSramAddrGet(MV_U8 chan)
{
	return (MV_U8 *) cesaHalData.sramPhysBase[chan];
}

static INLINE MV_ULONG mvCesaSramVirtToPhys(MV_U8 chan, void *pDev, MV_U8 *pSramVirt)
{
	return (MV_ULONG) (pSramVirt - cesaHalData.sramVirtBase[chan]) + cesaHalData.sramPhysBase[chan];
}

static INLINE void mvCesaSramDescrBuild(MV_U8 chan, MV_U32 config, int frag,
					int cryptoOffset, int ivOffset, int cryptoLength,
					int macOffset, int digestOffset, int macLength, int macTotalLen,
					MV_CESA_REQ *pCesaReq, MV_DMA_DESC *pDmaDesc);

static INLINE void mvCesaSramSaUpdate(MV_U8 chan, short sid, MV_DMA_DESC *pDmaDesc);

static INLINE int mvCesaDmaCopyPrepare(MV_U8 chan, MV_CESA_MBUF *pMbuf, MV_U8 *pSramBuf,
				       MV_DMA_DESC *pDmaDesc, MV_BOOL isToMbuf,
				       int offset, int copySize, MV_BOOL skipFlush);

static void mvCesaHmacIvGet(MV_CESA_MAC_MODE macMode, unsigned char key[], int keyLength,
			    unsigned char innerIV[], unsigned char outerIV[]);

static MV_STATUS mvCesaFragAuthComplete(MV_U8 chan, MV_CESA_REQ *pReq, MV_CESA_SA *pSA, int macDataSize);

static MV_CESA_COMMAND *mvCesaCtrModeInit(void);

static MV_STATUS mvCesaCtrModePrepare(MV_CESA_COMMAND *pCtrModeCmd, MV_CESA_COMMAND *pCmd);
static MV_STATUS mvCesaCtrModeComplete(MV_CESA_COMMAND *pOrgCmd, MV_CESA_COMMAND *pCmd);
static void mvCesaCtrModeFinish(MV_CESA_COMMAND *pCmd);

static INLINE MV_STATUS mvCesaReqProcess(MV_U8 chan, MV_CESA_REQ *pReq);
static MV_STATUS mvCesaFragReqProcess(MV_U8 chan, MV_CESA_REQ *pReq, MV_U8 frag);

static INLINE MV_STATUS mvCesaParamCheck(MV_CESA_SA *pSA, MV_CESA_COMMAND *pCmd, MV_U8 *pFixOffset);
static INLINE MV_STATUS mvCesaFragParamCheck(MV_U8 chan, MV_CESA_SA *pSA, MV_CESA_COMMAND *pCmd);

static INLINE void mvCesaFragSizeFind(MV_CESA_SA *pSA, MV_CESA_REQ *pReq,
				      int cryptoOffset, int macOffset,
				      int *pCopySize, int *pCryptoDataSize, int *pMacDataSize);
static MV_STATUS mvCesaMbufCacheUnmap(MV_CESA_MBUF *pMbuf, int offset, int size);
static MV_STATUS mvCesaUpdateSADSize(MV_U32 size);

static INLINE MV_CESA_REQ *MV_CESA_REQ_NEXT_PTR(MV_U8 chan, MV_CESA_REQ *pReq)
{
	if (pReq == pCesaReqLast[chan])
		return pCesaReqFirst[chan];

	return (pReq + 1);
}

static INLINE MV_CESA_REQ *MV_CESA_REQ_PREV_PTR(MV_U8 chan, MV_CESA_REQ *pReq)
{
	if (pReq == pCesaReqFirst[chan])
		return pCesaReqLast[chan];

	return (pReq - 1);
}

static INLINE void mvCesaReqProcessStart(MV_U8 chan, MV_CESA_REQ *pReq)
{
	MV_32 frag;

#ifdef MV_CESA_CHAIN_MODE
	pReq->state = MV_CESA_CHAIN;
#else
	pReq->state = MV_CESA_PROCESS;
#endif

	cesaStats.startCount++;
	(pReq->use)++;

	if (pReq->fragMode == MV_CESA_FRAG_NONE) {
		frag = 0;
	} else {
		frag = pReq->frags.nextFrag;
		pReq->frags.nextFrag++;
	}

	MV_REG_WRITE(MV_CESA_TDMA_CURR_DESC_PTR_REG(chan), 0);
	MV_REG_WRITE(MV_CESA_TDMA_NEXT_DESC_PTR_REG(chan),
		     (MV_U32) mvCesaVirtToPhys(&pReq->dmaDescBuf, pReq->dma[frag].pDmaFirst));

#if defined(MV_BRIDGE_SYNC_REORDER)
	mvOsBridgeReorderWA();
#endif

	MV_REG_BIT_SET(MV_CESA_CMD_REG(chan), (MV_CESA_CMD_CHAN_ENABLE_MASK | sha2CmdVal));
}

MV_STATUS mvCesaHalInit(int numOfSession, int queueDepth, void *osHandle, MV_CESA_HAL_DATA *halData)
{
	int i, req;
	MV_U32 descOffsetReg, configReg;
	MV_U8 chan;

	cesaOsHandle = osHandle;
	sha2CmdVal = 0;

	mvOsPrintf("mvCesaInit: channels=%d, session=%d, queue=%d\n", MV_CESA_CHANNELS, numOfSession, queueDepth);

	pCesaSAD = mvOsMalloc(sizeof(MV_CESA_SA *) * numOfSession);
	if (pCesaSAD == NULL) {
		mvOsPrintf("mvCesaInit: Can't allocate %u bytes for %d SAs\n",
			   sizeof(MV_CESA_SA *) * numOfSession, numOfSession);
		mvCesaFinish();
		return MV_NO_RESOURCE;
	}
	memset(pCesaSAD, 0, sizeof(MV_CESA_SA *) * numOfSession);
	cesaMaxSA = numOfSession;

	ctrlModel = halData->ctrlModel;
	ctrlRev = halData->ctrlRev;

	for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {

		cesaSramVirtPtr[chan] = (MV_CESA_SRAM_MAP *) (halData->sramVirtBase[chan] + halData->sramOffset[chan]);

		pCesaReqFirst[chan] = mvOsMalloc(sizeof(MV_CESA_REQ) * queueDepth);
		if (pCesaReqFirst[chan] == NULL) {
			mvOsPrintf("mvCesaInit: Can't allocate %u bytes for %d requests\n",
				   sizeof(MV_CESA_REQ) * queueDepth, queueDepth);
			mvCesaFinish();
			return MV_NO_RESOURCE;
		}
		memset(pCesaReqFirst[chan], 0, sizeof(MV_CESA_REQ) * queueDepth);
		pCesaReqEmpty[chan] = pCesaReqFirst[chan];
		pCesaReqLast[chan] = pCesaReqFirst[chan] + (queueDepth - 1);
		pCesaReqProcess[chan] = pCesaReqEmpty[chan];
#ifdef MV_CESA_INT_COALESCING_SUPPORT
		pCesaReqStartNext[chan] = pCesaReqFirst[chan];
		pCesaReqProcessCurr[chan] = NULL;
#endif
		cesaQueueDepth[chan] = queueDepth;
		cesaReqResources[chan] = queueDepth;
		cesaLastSid[chan] = -1;
#ifdef MV_CESA_CHAIN_MODE
		cesaChainLength[chan] = MAX_CESA_CHAIN_LENGTH;
		chainReqNum[chan] = 0;
		chainIndex[chan] = 0;
		pNextActiveChain[chan] = NULL;
		pEndCurrChain[chan] = NULL;
		isFirstReq[chan] = MV_TRUE;
#endif

		if (MV_IS_NOT_ALIGN((MV_ULONG) cesaSramVirtPtr[chan], 8)) {
			mvOsPrintf("mvCesaInit: pSramBase (%p) must be 8 byte aligned\n", cesaSramVirtPtr[chan]);
			mvCesaFinish();
			return MV_NOT_ALIGNED;
		}

		MV_REG_WRITE(MV_CESA_CFG_REG(chan), 0);
		MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);
		MV_REG_WRITE(MV_CESA_ISR_MASK_REG(chan), 0);

		descOffsetReg = configReg = 0;
		for (req = 0; req < queueDepth; req++) {
			int frag;
			MV_CESA_REQ *pReq;
			MV_DMA_DESC *pDmaDesc;

			pReq = &pCesaReqFirst[chan][req];

			pReq->cesaDescBuf.bufSize = sizeof(MV_CESA_DESC) * MV_CESA_MAX_REQ_FRAGS + CPU_D_CACHE_LINE_SIZE;

			pReq->cesaDescBuf.bufVirtPtr = mvOsIoCachedMalloc(osHandle, pReq->cesaDescBuf.bufSize,
							&pReq->cesaDescBuf.bufPhysAddr,
							&pReq->cesaDescBuf.memHandle);
			if (pReq->cesaDescBuf.bufVirtPtr == NULL) {
				mvOsPrintf("mvCesaInit: req=%d, Can't allocate %d bytes for CESA descriptors\n",
						req, pReq->cesaDescBuf.bufSize);
				mvCesaFinish();
				return MV_NO_RESOURCE;
			}
			memset(pReq->cesaDescBuf.bufVirtPtr, 0, pReq->cesaDescBuf.bufSize);

			pReq->pCesaDesc = (MV_CESA_DESC *) MV_ALIGN_UP((MV_ULONG) pReq->cesaDescBuf.bufVirtPtr,
							CPU_D_CACHE_LINE_SIZE);

			pReq->dmaDescBuf.bufSize = sizeof(MV_DMA_DESC) * MV_CESA_MAX_DMA_DESC * MV_CESA_MAX_REQ_FRAGS +
							CPU_D_CACHE_LINE_SIZE;

			pReq->dmaDescBuf.bufVirtPtr = mvOsIoCachedMalloc(osHandle, pReq->dmaDescBuf.bufSize,
							&pReq->dmaDescBuf.bufPhysAddr, &pReq->dmaDescBuf.memHandle);

			if (pReq->dmaDescBuf.bufVirtPtr == NULL) {
				mvOsPrintf("mvCesaInit: req=%d, Can't allocate %d bytes for DMA descriptor list\n",
					req, pReq->dmaDescBuf.bufSize);
				mvCesaFinish();
				return MV_NO_RESOURCE;
			}
			memset(pReq->dmaDescBuf.bufVirtPtr, 0, pReq->dmaDescBuf.bufSize);
			pDmaDesc = (MV_DMA_DESC *) MV_ALIGN_UP((MV_ULONG) pReq->dmaDescBuf.bufVirtPtr,
						CPU_D_CACHE_LINE_SIZE);

			for (frag = 0; frag < MV_CESA_MAX_REQ_FRAGS; frag++) {
				MV_CESA_DMA *pDma = &pReq->dma[frag];

				pDma->pDmaFirst = pDmaDesc;
				pDma->pDmaLast = NULL;

				for (i = 0; i < MV_CESA_MAX_DMA_DESC - 1; i++) {
					 
					pDma->pDmaFirst[i].phyNextDescPtr =
						MV_32BIT_LE(mvCesaVirtToPhys(&pReq->dmaDescBuf, &pDmaDesc[i + 1]));
				}
				pDma->pDmaFirst[i].phyNextDescPtr = 0;
				mvOsCacheFlush(NULL, &pDma->pDmaFirst[0], MV_CESA_MAX_DMA_DESC * sizeof(MV_DMA_DESC));

				pDmaDesc += MV_CESA_MAX_DMA_DESC;
			}
		}

		descOffsetReg = (MV_U16)((MV_U8 *)&cesaSramVirtPtr[chan]->desc - mvCesaSramAddrGet(chan));
		MV_REG_WRITE(MV_CESA_CHAN_DESC_OFFSET_REG(chan), descOffsetReg);

		configReg |= (MV_CESA_CFG_WAIT_DMA_MASK | MV_CESA_CFG_ACT_DMA_MASK);

#ifdef MV_CESA_CHAIN_MODE
		configReg |= MV_CESA_CFG_CHAIN_MODE_MASK;
#endif

		MV_REG_WRITE(MV_CESA_TDMA_CTRL_REG(chan), MV_CESA_TDMA_CTRL_VALUE);
		MV_REG_WRITE(MV_CESA_TDMA_BYTE_COUNT_REG(chan), 0);
		MV_REG_WRITE(MV_CESA_TDMA_CURR_DESC_PTR_REG(chan), 0);

		switch ((MV_U16)(ctrlModel & 0xff00)) {
		case 0x6500:  
			if (ctrlRev < 2) {
				 
				configReg |= MV_CESA_CFG_ENC_AUTH_PARALLEL_MODE_MASK;
				sha2CmdVal = BIT31;
			}
			break;
		case 0x6700:  
			 
			MV_REG_BIT_SET(MV_CESA_TDMA_CTRL_REG(chan), MV_CESA_TDMA_OUTSTAND_NEW_MODE_BIT);
			break;
		case 0x7800:  
			if (ctrlRev < 1) {  
#ifdef AURORA_IO_CACHE_COHERENCY
				 
				MV_REG_BIT_RESET(MV_CESA_TDMA_CTRL_REG(chan), MV_CESA_TDMA_OUTSTAND_READ_EN_MASK);
#endif
				 
				configReg |= MV_CESA_CFG_ENC_AUTH_PARALLEL_MODE_MASK;
			} else {  
				 
				MV_REG_BIT_SET(MV_CESA_TDMA_CTRL_REG(chan), MV_CESA_TDMA_OUTSTAND_OUT_OF_ORDER_3TRANS_BIT);
			}
			sha2CmdVal = BIT31;
			break;
		default:
			mvOsPrintf("Error, chip revision(%d) no supported\n", halData->ctrlRev);
			break;
		}

#ifdef MV_CESA_INT_COALESCING_SUPPORT
		configReg |= MV_CESA_CFG_CHAIN_MODE_MASK;
		 
		MV_REG_WRITE(MV_CESA_INT_COAL_TH_REG(chan), MV_CESA_INT_COAL_THRESHOLD);
		MV_REG_WRITE(MV_CESA_INT_TIME_TH_REG(chan), MV_CESA_INT_COAL_TIME_THRESHOLD);
#endif

		MV_REG_WRITE(MV_CESA_CFG_REG(chan), configReg);
	}

	mvCesaDebugStatsClear();
	mvOsMemcpy(&cesaHalData, halData, sizeof(MV_CESA_HAL_DATA));

	return MV_OK;
}

MV_STATUS mvCesaFinish(void)
{
	int req, sid;
	MV_CESA_REQ *pReq;
	MV_U8 chan;

	mvOsPrintf("mvCesaFinish: \n");

	for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {

		cesaSramVirtPtr[chan] = NULL;

		MV_REG_WRITE(MV_CESA_CFG_REG(chan), 0);
		MV_REG_WRITE(MV_CESA_ISR_CAUSE_REG(chan), 0);
		MV_REG_WRITE(MV_CESA_ISR_MASK_REG(chan), 0);

		for (req = 0; req < cesaQueueDepth[chan]; req++) {
			pReq = &pCesaReqFirst[chan][req];
			if (pReq->dmaDescBuf.bufVirtPtr != NULL) {
				mvOsIoCachedFree(cesaOsHandle, pReq->dmaDescBuf.bufSize,
					pReq->dmaDescBuf.bufPhysAddr,
					pReq->dmaDescBuf.bufVirtPtr, pReq->dmaDescBuf.memHandle);
			}
			if (pReq->cesaDescBuf.bufVirtPtr != NULL) {
				mvOsIoCachedFree(cesaOsHandle, pReq->cesaDescBuf.bufSize,
					pReq->cesaDescBuf.bufPhysAddr,
					pReq->cesaDescBuf.bufVirtPtr, pReq->cesaDescBuf.memHandle);
			}

			if (pCesaReqFirst[chan] != NULL) {
				mvOsFree(pCesaReqFirst[chan]);
				pCesaReqFirst[chan] = pCesaReqLast[chan] = NULL;
				pCesaReqEmpty[chan] = pCesaReqProcess[chan] = NULL;
				cesaQueueDepth[chan] = cesaReqResources[chan] = 0;
			}
		}
	}

	if (pCesaSAD != NULL) {
		for (sid = 0; sid < cesaMaxSA; sid++) {
#ifdef MY_ABC_HERE
			if (pCesaSAD[sid] != NULL) {
#endif
				 
				mvOsIoCachedFree(cesaOsHandle, pCesaSAD[sid]->sramSABuffSize,
						 pCesaSAD[sid]->sramSAPhysAddr,
						 pCesaSAD[sid]->sramSABuff, pCesaSAD[sid]->memHandle);
				 
				mvOsFree(pCesaSAD[sid]);
				pCesaSAD[sid] = NULL;	
#ifdef MY_ABC_HERE
			}
#endif
		}
		cesaMaxSA = 0;
	}

	return MV_OK;
}

MV_STATUS mvCesaCryptoIvSet(MV_U8 chan, MV_U8 *pIV, int ivSize)
{
	MV_U8 *pSramIV;

	pSramIV = cesaSramVirtPtr[chan]->cryptoIV;
	if (ivSize > MV_CESA_MAX_IV_LENGTH) {
		mvOsPrintf("mvCesaCryptoIvSet: ivSize (%d) is too large\n", ivSize);
		ivSize = MV_CESA_MAX_IV_LENGTH;
	}
	if (pIV != NULL) {
		memcpy(pSramIV, pIV, ivSize);
		ivSize = MV_CESA_MAX_IV_LENGTH - ivSize;
		pSramIV += ivSize;
	}

	while (ivSize > 0) {
		int size, mv_random = mvOsRand();

		size = MV_MIN(ivSize, sizeof(mv_random));
		memcpy(pSramIV, (void *)&mv_random, size);

		pSramIV += size;
		ivSize -= size;
	}
 
	return MV_OK;
}

MV_STATUS mvCesaSessionOpen(MV_CESA_OPEN_SESSION *pSession, short *pSid)
{
	short sid;
	MV_U32 config = 0;
	int digestSize;
	MV_BUF_INFO cesaSramSaBuf;

	cesaStats.openedCount++;

	for (sid = 0; sid < cesaMaxSA; sid++)
		if (pCesaSAD[sid] == NULL)
			break;

	if (sid == cesaMaxSA) {
		if (MV_FAIL == mvCesaUpdateSADSize(cesaMaxSA * 2)) {
			mvOsPrintf("mvCesaSessionOpen: SA Database is FULL\n");
			return MV_FULL;
		}
	}

	pCesaSAD[sid] = mvOsMalloc(sizeof(MV_CESA_SA));
	if (pCesaSAD[sid] == NULL) {
		mvOsPrintf("mvCesaSessionOpen: Can't allocate %d bytes for SA structures\n", sizeof(MV_CESA_SA));
		return MV_FULL;
	}
	memset(pCesaSAD[sid], 0, sizeof(MV_CESA_SA));

	cesaSramSaBuf.bufSize = sizeof(MV_CESA_SRAM_SA) + CPU_D_CACHE_LINE_SIZE;

	cesaSramSaBuf.bufVirtPtr = mvOsIoCachedMalloc(cesaOsHandle, cesaSramSaBuf.bufSize,
						      &cesaSramSaBuf.bufPhysAddr, &cesaSramSaBuf.memHandle);

	if (cesaSramSaBuf.bufVirtPtr == NULL) {
		mvOsPrintf("mvCesaSessionOpen: Can't allocate %d bytes for sramSA structures\n", cesaSramSaBuf.bufSize);
		return MV_FULL;
	}
	memset(cesaSramSaBuf.bufVirtPtr, 0, cesaSramSaBuf.bufSize);

	pCesaSAD[sid]->sramSABuff = cesaSramSaBuf.bufVirtPtr;
	pCesaSAD[sid]->sramSABuffSize = cesaSramSaBuf.bufSize;
	pCesaSAD[sid]->memHandle = cesaSramSaBuf.memHandle;
	pCesaSAD[sid]->pSramSA = (MV_CESA_SRAM_SA *) MV_ALIGN_UP((MV_ULONG) cesaSramSaBuf.bufVirtPtr,
								 CPU_D_CACHE_LINE_SIZE);

	pCesaSAD[sid]->sramSAPhysAddr = MV_32BIT_LE(mvCesaVirtToPhys(&cesaSramSaBuf, pCesaSAD[sid]->pSramSA));

	if (pSession->operation >= MV_CESA_MAX_OPERATION) {
		mvOsPrintf("mvCesaSessionOpen: Unexpected operation %d\n", pSession->operation);
		return MV_BAD_PARAM;
	}
	config |= (pSession->operation << MV_CESA_OPERATION_OFFSET);

	if ((pSession->direction != MV_CESA_DIR_ENCODE) && (pSession->direction != MV_CESA_DIR_DECODE)) {
		mvOsPrintf("mvCesaSessionOpen: Unexpected direction %d\n", pSession->direction);
		return MV_BAD_PARAM;
	}
	config |= (pSession->direction << MV_CESA_DIRECTION_BIT);
	 
	if (pSession->operation != MV_CESA_CRYPTO_ONLY) {
		 
		if ((pSession->macMode == MV_CESA_MAC_HMAC_MD5) || (pSession->macMode == MV_CESA_MAC_HMAC_SHA1) ||
					(pSession->macMode == MV_CESA_MAC_HMAC_SHA2)) {
			if (pSession->macKeyLength > MV_CESA_MAX_MAC_KEY_LENGTH) {
				mvOsPrintf("mvCesaSessionOpen: macKeyLength %d is too large\n", pSession->macKeyLength);
				return MV_BAD_PARAM;
			}
			mvCesaHmacIvGet(pSession->macMode, pSession->macKey, pSession->macKeyLength,
					pCesaSAD[sid]->pSramSA->macInnerIV, pCesaSAD[sid]->pSramSA->macOuterIV);
			pCesaSAD[sid]->macKeyLength = pSession->macKeyLength;
		}
		switch (pSession->macMode) {
		case MV_CESA_MAC_MD5:
		case MV_CESA_MAC_HMAC_MD5:
			digestSize = MV_CESA_MD5_DIGEST_SIZE;
			break;

		case MV_CESA_MAC_SHA1:
		case MV_CESA_MAC_HMAC_SHA1:
			digestSize = MV_CESA_SHA1_DIGEST_SIZE;
			break;

		case MV_CESA_MAC_SHA2:
		case MV_CESA_MAC_HMAC_SHA2:
			digestSize = MV_CESA_SHA2_DIGEST_SIZE;
			break;

		default:
			mvOsPrintf("mvCesaSessionOpen: Unexpected macMode %d\n", pSession->macMode);
			return MV_BAD_PARAM;
		}
		config |= (pSession->macMode << MV_CESA_MAC_MODE_OFFSET);

		if ((pSession->digestSize != digestSize) && (pSession->digestSize != 12)) {
			mvOsPrintf("mvCesaSessionOpen: Unexpected digest size %d\n", pSession->digestSize);
			mvOsPrintf("\t Valid values [bytes]: MD5-16, SHA1-20, SHA2-32, All-12\n");
			return MV_BAD_PARAM;
		}
		pCesaSAD[sid]->digestSize = pSession->digestSize;

		if (pCesaSAD[sid]->digestSize == 12) {
			 
			config |= (MV_CESA_MAC_DIGEST_96B << MV_CESA_MAC_DIGEST_SIZE_BIT);
		}
	}

	if (pSession->operation != MV_CESA_MAC_ONLY) {
		switch (pSession->cryptoAlgorithm) {
		case MV_CESA_CRYPTO_DES:
			pCesaSAD[sid]->cryptoKeyLength = MV_CESA_DES_KEY_LENGTH;
			pCesaSAD[sid]->cryptoBlockSize = MV_CESA_DES_BLOCK_SIZE;
			break;

		case MV_CESA_CRYPTO_3DES:
			pCesaSAD[sid]->cryptoKeyLength = MV_CESA_3DES_KEY_LENGTH;
			pCesaSAD[sid]->cryptoBlockSize = MV_CESA_DES_BLOCK_SIZE;
			 
			config |= (MV_CESA_CRYPTO_3DES_EDE << MV_CESA_CRYPTO_3DES_MODE_BIT);
			break;

		case MV_CESA_CRYPTO_AES:
			switch (pSession->cryptoKeyLength) {
			case 16:
				pCesaSAD[sid]->cryptoKeyLength = MV_CESA_AES_128_KEY_LENGTH;
				config |= (MV_CESA_CRYPTO_AES_KEY_128 << MV_CESA_CRYPTO_AES_KEY_LEN_OFFSET);
				break;

			case 24:
				pCesaSAD[sid]->cryptoKeyLength = MV_CESA_AES_192_KEY_LENGTH;
				config |= (MV_CESA_CRYPTO_AES_KEY_192 << MV_CESA_CRYPTO_AES_KEY_LEN_OFFSET);
				break;

			case 32:
			default:
				pCesaSAD[sid]->cryptoKeyLength = MV_CESA_AES_256_KEY_LENGTH;
				config |= (MV_CESA_CRYPTO_AES_KEY_256 << MV_CESA_CRYPTO_AES_KEY_LEN_OFFSET);
				break;
			}
			pCesaSAD[sid]->cryptoBlockSize = MV_CESA_AES_BLOCK_SIZE;
			break;

		default:
			mvOsPrintf("mvCesaSessionOpen: Unexpected cryptoAlgorithm %d\n", pSession->cryptoAlgorithm);
			return MV_BAD_PARAM;
		}
		config |= (pSession->cryptoAlgorithm << MV_CESA_CRYPTO_ALG_OFFSET);

		if (pSession->cryptoKeyLength != pCesaSAD[sid]->cryptoKeyLength) {
			mvOsPrintf("cesaSessionOpen: Wrong CryptoKeySize %d != %d\n",
				   pSession->cryptoKeyLength, pCesaSAD[sid]->cryptoKeyLength);
			return MV_BAD_PARAM;
		}

		if ((pSession->cryptoAlgorithm == MV_CESA_CRYPTO_AES) && (pSession->direction == MV_CESA_DIR_DECODE)) {
			 
			aesMakeKey(pCesaSAD[sid]->pSramSA->cryptoKey, pSession->cryptoKey,
				   pSession->cryptoKeyLength * 8, MV_CESA_AES_BLOCK_SIZE * 8);
		} else {
			 
			memcpy(pCesaSAD[sid]->pSramSA->cryptoKey, pSession->cryptoKey, pCesaSAD[sid]->cryptoKeyLength);

		}

		switch (pSession->cryptoMode) {
		case MV_CESA_CRYPTO_ECB:
			pCesaSAD[sid]->cryptoIvSize = 0;
			break;

		case MV_CESA_CRYPTO_CBC:
			pCesaSAD[sid]->cryptoIvSize = pCesaSAD[sid]->cryptoBlockSize;
			break;

		case MV_CESA_CRYPTO_CTR:
			 
			if (pSession->cryptoAlgorithm != MV_CESA_CRYPTO_AES) {
				mvOsPrintf("mvCesaSessionOpen: CRYPTO CTR mode supported for AES only\n");
				return MV_BAD_PARAM;
			}
			pCesaSAD[sid]->cryptoIvSize = 0;
			pCesaSAD[sid]->ctrMode = 1;
			 
			pSession->cryptoMode = MV_CESA_CRYPTO_ECB;
			break;

		default:
			mvOsPrintf("mvCesaSessionOpen: Unexpected cryptoMode %d\n", pSession->cryptoMode);
			return MV_BAD_PARAM;
		}

		config |= (pSession->cryptoMode << MV_CESA_CRYPTO_MODE_BIT);
	}
	pCesaSAD[sid]->config = config;

	mvOsCacheFlush(NULL, pCesaSAD[sid]->pSramSA, sizeof(MV_CESA_SRAM_SA));
	if (pSid != NULL)
		*pSid = sid;

	return MV_OK;
}

MV_STATUS mvCesaSessionClose(short sid)
{
	MV_U8 chan;

	cesaStats.closedCount++;

	if (sid >= cesaMaxSA) {
		mvOsPrintf("CESA Error: sid (%d) is too big\n", sid);
		return MV_BAD_PARAM;
	}

	if (pCesaSAD[sid] == NULL) {
		mvOsPrintf("CESA Warning: Session (sid=%d) is invalid\n", sid);
		return MV_NOT_FOUND;
	}

	for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
		if (cesaLastSid[chan] == sid)
			cesaLastSid[chan] = -1;
	}

	mvOsIoCachedFree(cesaOsHandle, pCesaSAD[sid]->sramSABuffSize,
			 pCesaSAD[sid]->sramSAPhysAddr, pCesaSAD[sid]->sramSABuff, pCesaSAD[sid]->memHandle);
	mvOsFree(pCesaSAD[sid]);

	pCesaSAD[sid] = NULL;

	return MV_OK;
}

MV_STATUS mvCesaAction(MV_U8 chan, MV_CESA_COMMAND *pCmd)
{
	MV_STATUS status;
	MV_CESA_REQ *pReq = pCesaReqEmpty[chan];
	int sid = pCmd->sessionId;
	MV_CESA_SA *pSA = pCesaSAD[sid];
#ifdef MV_CESA_CHAIN_MODE
	MV_CESA_REQ *pFromReq;
	MV_CESA_REQ *pToReq;
#endif
	cesaStats.reqCount++;

	if (cesaReqResources[chan] == 0)
		return MV_NO_RESOURCE;

	if ((sid >= cesaMaxSA) || (pSA == NULL)) {
		mvOsPrintf("CESA Action Error: Session sid=%d is INVALID\n", sid);
		return MV_BAD_PARAM;
	}
	pSA->count++;

	if (pSA->ctrMode) {
		 
		if ((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET)) {
			mvOsPrintf("mvCesaAction : CRYPTO CTR mode can't be mixed with AUTH\n");
			return MV_NOT_ALLOWED;
		}
		 
		pReq->pOrgCmd = pCmd;
		 
		pCmd = mvCesaCtrModeInit();
		if (pCmd == NULL)
			return MV_OUT_OF_CPU_MEM;

		mvCesaCtrModePrepare(pCmd, pReq->pOrgCmd);
		pReq->fixOffset = 0;
	} else {
		 
		status = mvCesaParamCheck(pSA, pCmd, &pReq->fixOffset);
		if (status != MV_OK)
			return status;
	}
	pReq->pCmd = pCmd;

	if (pCmd->pSrc->mbufSize <= sizeof(cesaSramVirtPtr[chan]->buf)) {
		 
		pReq->fragMode = MV_CESA_FRAG_NONE;

		status = mvCesaReqProcess(chan, pReq);
		if (status != MV_OK)
			mvOsPrintf("CesaReady: ReqProcess error: pReq=%p, status=0x%x\n", pReq, status);
#ifdef MV_CESA_CHAIN_MODE
		pReq->frags.numFrag = 1;
#endif
	} else {
		MV_U8 frag = 0;

		status = mvCesaFragParamCheck(chan, pSA, pCmd);
		if (status != MV_OK)
			return status;

		pReq->fragMode = MV_CESA_FRAG_FIRST;
		pReq->frags.nextFrag = 0;

		while (pReq->fragMode != MV_CESA_FRAG_LAST) {
			if (frag >= MV_CESA_MAX_REQ_FRAGS) {
				mvOsPrintf("mvCesaAction Error: Too large request frag=%d\n", frag);
				return MV_OUT_OF_CPU_MEM;
			}
			status = mvCesaFragReqProcess(chan, pReq, frag);
			if (status == MV_OK) {
#if defined(MV_CESA_CHAIN_MODE) || defined(MV_CESA_INT_COALESCING_SUPPORT)
				if (frag) {
					pReq->dma[frag - 1].pDmaLast->phyNextDescPtr =
					    MV_32BIT_LE(mvCesaVirtToPhys(&pReq->dmaDescBuf, pReq->dma[frag].pDmaFirst));
					mvOsCacheFlush(NULL, pReq->dma[frag - 1].pDmaLast, sizeof(MV_DMA_DESC));
				}
#endif  
				frag++;
			}
		}
		pReq->frags.numFrag = frag;

#ifdef MV_CESA_CHAIN_MODE
		if (chainReqNum[chan]) {
			chainReqNum[chan] += pReq->frags.numFrag;
			if (chainReqNum[chan] >= MAX_CESA_CHAIN_LENGTH)
				chainReqNum[chan] = MAX_CESA_CHAIN_LENGTH;
		}
#endif  
	}

	pReq->state = MV_CESA_PENDING;

	pCesaReqEmpty[chan] = MV_CESA_REQ_NEXT_PTR(chan, pCesaReqEmpty[chan]);
	cesaReqResources[chan] -= 1;

	if ((cesaQueueDepth[chan] - cesaReqResources[chan]) > cesaStats.maxReqCount)
		cesaStats.maxReqCount = (cesaQueueDepth[chan] - cesaReqResources[chan]);
 
	cesaLastSid[chan] = sid;

#ifdef MV_CESA_CHAIN_MODE

	if ((chainReqNum[chan] > 0) && (chainReqNum[chan] < MAX_CESA_CHAIN_LENGTH)) {
		if (chainIndex[chan]) {
			pFromReq = MV_CESA_REQ_PREV_PTR(chan, pReq);
			pToReq = pReq;
			pReq->state = MV_CESA_CHAIN;

			pFromReq->dma[pFromReq->frags.numFrag - 1].pDmaLast->phyNextDescPtr =
			    MV_32BIT_LE(mvCesaVirtToPhys(&pToReq->dmaDescBuf, pToReq->dma[0].pDmaFirst));
			mvOsCacheFlush(NULL, pFromReq->dma[pFromReq->frags.numFrag - 1].pDmaLast, sizeof(MV_DMA_DESC));

			if (pNextActiveChain[chan]->state != MV_CESA_PENDING)
				pEndCurrChain[chan] = pNextActiveChain[chan] = MV_CESA_REQ_NEXT_PTR(chan, pReq);
		} else {	 
			chainReqNum[chan] = 0;
			chainIndex[chan]++;
			 
			if (pNextActiveChain[chan]->state != MV_CESA_PENDING)
				pEndCurrChain[chan] = pNextActiveChain[chan] = pReq;
		}
	} else {
		 
		if (chainReqNum[chan] == MAX_CESA_CHAIN_LENGTH) {
			chainIndex[chan]++;
			if (pNextActiveChain[chan]->state != MV_CESA_PENDING)
				pEndCurrChain[chan] = pNextActiveChain[chan] = pReq;
			chainReqNum[chan] = 0;
		}

		pReq = pCesaReqProcess[chan];
		if (pReq->state == MV_CESA_PENDING) {
			pNextActiveChain[chan] = pReq;
			pEndCurrChain[chan] = MV_CESA_REQ_NEXT_PTR(chan, pReq);
			 
			mvCesaReqProcessStart(chan, pReq);
		}
	}

	chainReqNum[chan]++;

	if ((chainIndex[chan] < MAX_CESA_CHAIN_LENGTH) && (chainReqNum[chan] > cesaStats.maxChainUsage))
		cesaStats.maxChainUsage = chainReqNum[chan];

#elif defined(MV_CESA_INT_COALESCING_SUPPORT)

	if (!(MV_REG_READ(MV_CESA_STATUS_REG(chan)) & MV_CESA_STATUS_ACTIVE_MASK)) {
		if (pCesaReqStartNext[chan]->state == MV_CESA_PENDING) {
			mvCesaReqProcessStart(chan, pCesaReqStartNext[chan]);
			pCesaReqProcessCurr[chan] = pCesaReqStartNext[chan];
			pCesaReqStartNext[chan] = MV_CESA_REQ_NEXT_PTR(chan, pCesaReqStartNext[chan]);
		}
	}

#else  

	pReq = pCesaReqProcess[chan];
	if (pReq->state == MV_CESA_PENDING) {
		 
		mvCesaReqProcessStart(chan, pReq);
	}
#endif  

	if (cesaReqResources[chan] == 0)
		return MV_NO_MORE;

	return MV_OK;

}

MV_STATUS mvCesaReadyGet(MV_U8 chan, MV_CESA_RESULT *pResult)
{
	MV_STATUS status, readyStatus = MV_NOT_READY;
	MV_U32 statusReg;
	MV_CESA_REQ *pReq;
	MV_CESA_SA *pSA;

#ifdef MV_CESA_CHAIN_MODE
	if (isFirstReq[chan] == MV_TRUE) {

		if (chainIndex[chan] == 0)
			chainReqNum[chan] = 0;

		isFirstReq[chan] = MV_FALSE;

		if (pNextActiveChain[chan]->state == MV_CESA_PENDING) {

			mvCesaReqProcessStart(chan, pNextActiveChain[chan]);
			pEndCurrChain[chan] = pNextActiveChain[chan];
			if (chainIndex[chan] > 0)
				chainIndex[chan]--;
			 
			while (pNextActiveChain[chan]->state == MV_CESA_CHAIN)
				pNextActiveChain[chan] = MV_CESA_REQ_NEXT_PTR(chan, pNextActiveChain[chan]);
		}

	}

	if (pCesaReqProcess[chan] == pEndCurrChain[chan]) {

		isFirstReq[chan] = MV_TRUE;
		pEndCurrChain[chan] = pNextActiveChain[chan];
#else
	if (pCesaReqProcess[chan]->state != MV_CESA_PROCESS) {
#endif  

		return MV_EMPTY;
	}

#ifdef MV_CESA_INT_COALESCING_SUPPORT
	statusReg = MV_REG_READ(MV_CESA_STATUS_REG(chan));
	if ((statusReg & MV_CESA_STATUS_ACTIVE_MASK) &&
		(pCesaReqProcessCurr[chan] == pCesaReqProcess[chan])) {
		cesaStats.notReadyCount++;
		return MV_NOT_READY;
	}
#endif  

	cesaStats.readyCount++;

	pReq = pCesaReqProcess[chan];
	pSA = pCesaSAD[pReq->pCmd->sessionId];

	pResult->retCode = MV_OK;
	if (pReq->fragMode != MV_CESA_FRAG_NONE) {
		MV_U8 *pNewDigest;
		int frag;

#if defined(MV_CESA_CHAIN_MODE) || defined(MV_CESA_INT_COALESCING_SUPPORT)
		pReq->frags.nextFrag = 1;
		while (pReq->frags.nextFrag <= pReq->frags.numFrag) {
#endif

			frag = (pReq->frags.nextFrag - 1);

			pReq->dma[frag].pDmaLast->phyNextDescPtr =
			    MV_32BIT_LE(mvCesaVirtToPhys(&pReq->dmaDescBuf, &pReq->dma[frag].pDmaLast[1]));
			pReq->dma[frag].pDmaLast = NULL;

			if (pReq->frags.nextFrag >= pReq->frags.numFrag) {
				mvCesaMbufCacheUnmap(pReq->pCmd->pDst, 0, pReq->pCmd->pDst->mbufSize);

				if ((pSA->config & MV_CESA_OPERATION_MASK) !=
				    (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET)) {
					int macDataSize = pReq->pCmd->macLength - pReq->frags.macSize;

					if (macDataSize != 0) {
						 
						mvCesaFragAuthComplete(chan, pReq, pSA, macDataSize);
					}

					pNewDigest = cesaSramVirtPtr[chan]->buf + pReq->frags.newDigestOffset;
					status = mvCesaCopyToMbuf(pNewDigest, pReq->pCmd->pDst,
								  pReq->pCmd->digestOffset, pSA->digestSize);

					if ((pSA->config & MV_CESA_DIRECTION_MASK) ==
					    (MV_CESA_DIR_DECODE << MV_CESA_DIRECTION_BIT)) {
						if (memcmp(pNewDigest, pReq->frags.orgDigest, pSA->digestSize) != 0) {
 
							pResult->retCode = MV_FAIL;
						}
					}
				}
				readyStatus = MV_OK;
			}
#if defined(MV_CESA_CHAIN_MODE) || defined(MV_CESA_INT_COALESCING_SUPPORT)
			pReq->frags.nextFrag++;
		}
#endif
	} else {
		mvCesaMbufCacheUnmap(pReq->pCmd->pDst, 0, pReq->pCmd->pDst->mbufSize);

		pReq->dma[0].pDmaLast->phyNextDescPtr =
		    MV_32BIT_LE(mvCesaVirtToPhys(&pReq->dmaDescBuf, &pReq->dma[0].pDmaLast[1]));
		pReq->dma[0].pDmaLast = NULL;
		if (((pSA->config & MV_CESA_OPERATION_MASK) !=
		     (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET)) &&
		    ((pSA->config & MV_CESA_DIRECTION_MASK) == (MV_CESA_DIR_DECODE << MV_CESA_DIRECTION_BIT))) {
			 
			statusReg = MV_REG_READ(MV_CESA_STATUS_REG(chan));
			if (statusReg & MV_CESA_STATUS_DIGEST_ERR_MASK) {
 
				pResult->retCode = MV_FAIL;
			}
		}
		readyStatus = MV_OK;
	}

	if (readyStatus == MV_OK) {
		 
		pResult->pReqPrv = pReq->pCmd->pReqPrv;
		pResult->sessionId = pReq->pCmd->sessionId;
		pResult->mbufSize = pReq->pCmd->pSrc->mbufSize;
		pResult->reqId = pReq->pCmd->reqId;
		pReq->state = MV_CESA_IDLE;
		pCesaReqProcess[chan] = MV_CESA_REQ_NEXT_PTR(chan, pCesaReqProcess[chan]);
		cesaReqResources[chan]++;

		if (pSA->ctrMode) {
			 
			mvCesaCtrModeComplete(pReq->pOrgCmd, pReq->pCmd);
			mvCesaCtrModeFinish(pReq->pCmd);
			pReq->pOrgCmd = NULL;
		}
	}

#ifdef MV_CESA_INT_PER_PACKET

	if (pCesaReqProcess[chan]->state == MV_CESA_PENDING)
		mvCesaReqProcessStart(chan, pCesaReqProcess[chan]);

#elif defined(MV_CESA_INT_COALESCING_SUPPORT)
	statusReg = MV_REG_READ(MV_CESA_STATUS_REG(chan));
	if (!(statusReg & MV_CESA_STATUS_ACTIVE_MASK)) {
		if (pCesaReqStartNext[chan]->state == MV_CESA_PENDING) {
			mvCesaReqProcessStart(chan, pCesaReqStartNext[chan]);
			pCesaReqProcessCurr[chan] = pCesaReqStartNext[chan];
			pCesaReqStartNext[chan] = MV_CESA_REQ_NEXT_PTR(chan, pCesaReqStartNext[chan]);
		}
	}
#endif
	return readyStatus;
}

int mvCesaMbufOffset(MV_CESA_MBUF *pMbuf, int offset, int *pBufOffset)
{
	int frag = 0;

	while (offset > 0) {
		if (frag >= pMbuf->numFrags) {
			mvOsPrintf("mvCesaMbufOffset: Error: frag (%d) > numFrags (%d)\n", frag, pMbuf->numFrags);
			return MV_INVALID;
		}
		if (offset < pMbuf->pFrags[frag].bufSize)
			break;

		offset -= pMbuf->pFrags[frag].bufSize;
		frag++;
	}
	if (pBufOffset != NULL)
		*pBufOffset = offset;

	return frag;
}

MV_STATUS mvCesaCopyFromMbuf(MV_U8 *pDstBuf, MV_CESA_MBUF *pSrcMbuf, int offset, int size)
{
	int frag, fragOffset, bufSize;
	MV_U8 *pBuf;

	if (size == 0)
		return MV_OK;

	frag = mvCesaMbufOffset(pSrcMbuf, offset, &fragOffset);
	if (frag == MV_INVALID) {
		mvOsPrintf("CESA Mbuf Error: offset (%d) out of range\n", offset);
		return MV_OUT_OF_RANGE;
	}

	bufSize = pSrcMbuf->pFrags[frag].bufSize - fragOffset;
	pBuf = pSrcMbuf->pFrags[frag].bufVirtPtr + fragOffset;
	while (MV_TRUE) {
		if (size <= bufSize) {
			memcpy(pDstBuf, pBuf, size);
			return MV_OK;
		}
		memcpy(pDstBuf, pBuf, bufSize);
		size -= bufSize;
		frag++;
		pDstBuf += bufSize;
		if (frag >= pSrcMbuf->numFrags)
			break;

		bufSize = pSrcMbuf->pFrags[frag].bufSize;
		pBuf = pSrcMbuf->pFrags[frag].bufVirtPtr;
	}
	mvOsPrintf("mvCesaCopyFromMbuf: Mbuf is EMPTY - %d bytes isn't copied\n", size);
	return MV_EMPTY;
}

MV_STATUS mvCesaCopyToMbuf(MV_U8 *pSrcBuf, MV_CESA_MBUF *pDstMbuf, int offset, int size)
{
	int frag, fragOffset, bufSize;
	MV_U8 *pBuf;

	if (size == 0)
		return MV_OK;

	frag = mvCesaMbufOffset(pDstMbuf, offset, &fragOffset);
	if (frag == MV_INVALID) {
		mvOsPrintf("CESA Mbuf Error: offset (%d) out of range\n", offset);
		return MV_OUT_OF_RANGE;
	}

	bufSize = pDstMbuf->pFrags[frag].bufSize - fragOffset;
	pBuf = pDstMbuf->pFrags[frag].bufVirtPtr + fragOffset;
	while (MV_TRUE) {
		if (size <= bufSize) {
			memcpy(pBuf, pSrcBuf, size);
			return MV_OK;
		}
		memcpy(pBuf, pSrcBuf, bufSize);
		size -= bufSize;
		frag++;
		pSrcBuf += bufSize;
		if (frag >= pDstMbuf->numFrags)
			break;

		bufSize = pDstMbuf->pFrags[frag].bufSize;
		pBuf = pDstMbuf->pFrags[frag].bufVirtPtr;
	}
	mvOsPrintf("mvCesaCopyToMbuf: Mbuf is FULL - %d bytes isn't copied\n", size);
	return MV_FULL;
}

MV_STATUS mvCesaMbufCopy(MV_CESA_MBUF *pMbufDst, int dstMbufOffset,
			 MV_CESA_MBUF *pMbufSrc, int srcMbufOffset, int size)
{
	int srcFrag, dstFrag, srcSize, dstSize, srcOffset, dstOffset;
	int copySize;
	MV_U8 *pSrc, *pDst;

	if (size == 0)
		return MV_OK;

	srcFrag = mvCesaMbufOffset(pMbufSrc, srcMbufOffset, &srcOffset);
	if (srcFrag == MV_INVALID) {
		mvOsPrintf("CESA srcMbuf Error: offset (%d) out of range\n", srcMbufOffset);
		return MV_OUT_OF_RANGE;
	}
	pSrc = pMbufSrc->pFrags[srcFrag].bufVirtPtr + srcOffset;
	srcSize = pMbufSrc->pFrags[srcFrag].bufSize - srcOffset;

	dstFrag = mvCesaMbufOffset(pMbufDst, dstMbufOffset, &dstOffset);
	if (dstFrag == MV_INVALID) {
		mvOsPrintf("CESA dstMbuf Error: offset (%d) out of range\n", dstMbufOffset);
		return MV_OUT_OF_RANGE;
	}
	pDst = pMbufDst->pFrags[dstFrag].bufVirtPtr + dstOffset;
	dstSize = pMbufDst->pFrags[dstFrag].bufSize - dstOffset;

	while (size > 0) {
		copySize = MV_MIN(srcSize, dstSize);
		if (size <= copySize) {
			memcpy(pDst, pSrc, size);
			return MV_OK;
		}
		memcpy(pDst, pSrc, copySize);
		size -= copySize;
		srcSize -= copySize;
		dstSize -= copySize;

		if (srcSize == 0) {
			srcFrag++;
			if (srcFrag >= pMbufSrc->numFrags)
				break;

			pSrc = pMbufSrc->pFrags[srcFrag].bufVirtPtr;
			srcSize = pMbufSrc->pFrags[srcFrag].bufSize;
		}

		if (dstSize == 0) {
			dstFrag++;
			if (dstFrag >= pMbufDst->numFrags)
				break;

			pDst = pMbufDst->pFrags[dstFrag].bufVirtPtr;
			dstSize = pMbufDst->pFrags[dstFrag].bufSize;
		}
	}
	mvOsPrintf("mvCesaMbufCopy: BAD size - %d bytes isn't copied\n", size);

	return MV_BAD_SIZE;
}

MV_STATUS mvCesaUpdateSADSize(MV_U32 size)
{
	MV_CESA_SA **pNewCesaSAD = NULL;

	pNewCesaSAD = mvOsMalloc(sizeof(MV_CESA_SA *) * size);
	if (pNewCesaSAD == NULL) {
		mvOsPrintf("mvCesaUpdateSADSize: Can't allocate %d bytes for new SAD buffer\n", size);
		return MV_FAIL;
	}
	memset(pNewCesaSAD, 0, (sizeof(MV_CESA_SA *) * size));
	mvOsMemcpy(pNewCesaSAD, pCesaSAD, (sizeof(MV_CESA_SA *) * cesaMaxSA));
	mvOsFree(pCesaSAD);
	pCesaSAD = pNewCesaSAD;
	cesaMaxSA = size;

	return MV_OK;
}

static MV_STATUS mvCesaMbufCacheUnmap(MV_CESA_MBUF *pMbuf, int offset, int size)
{
	int frag, fragOffset, bufSize;
	MV_U8 *pBuf;

	if (size == 0)
		return MV_OK;

	frag = mvCesaMbufOffset(pMbuf, offset, &fragOffset);
	if (frag == MV_INVALID) {
		mvOsPrintf("CESA Mbuf Error: offset (%d) out of range\n", offset);
		return MV_OUT_OF_RANGE;
	}

	bufSize = pMbuf->pFrags[frag].bufSize - fragOffset;
	pBuf = pMbuf->pFrags[frag].bufVirtPtr + fragOffset;
	while (MV_TRUE) {
		if (size <= bufSize) {
			mvOsCacheUnmap(NULL, mvOsIoVirtToPhy(NULL, pBuf), size);
			return MV_OK;
		}

		mvOsCacheUnmap(NULL, mvOsIoVirtToPhy(NULL, pBuf), bufSize);
		size -= bufSize;
		frag++;
		if (frag >= pMbuf->numFrags)
			break;

		bufSize = pMbuf->pFrags[frag].bufSize;
		pBuf = pMbuf->pFrags[frag].bufVirtPtr;
	}
	mvOsPrintf("%s: Mbuf is FULL - %d bytes isn't Unmapped\n", __func__, size);
	return MV_FULL;
}

static MV_STATUS mvCesaFragReqProcess(MV_U8 chan, MV_CESA_REQ *pReq, MV_U8 frag)
{
	int i, copySize, cryptoDataSize, macDataSize, sid;
	int cryptoIvOffset, digestOffset;
	MV_U32 config;
	MV_CESA_COMMAND *pCmd = pReq->pCmd;
	MV_CESA_SA *pSA;
	MV_CESA_MBUF *pMbuf;
	MV_DMA_DESC *pDmaDesc = pReq->dma[frag].pDmaFirst;
	MV_U8 *pSramBuf = cesaSramVirtPtr[chan]->buf;
	int macTotalLen = 0;
	int fixOffset, cryptoOffset, macOffset;

	cesaStats.fragCount++;

	sid = pReq->pCmd->sessionId;

	pSA = pCesaSAD[sid];

	cryptoIvOffset = digestOffset = 0;
	i = macDataSize = 0;
	cryptoDataSize = 0;

	if (pReq->fragMode == MV_CESA_FRAG_FIRST) {
		 
		pReq->frags.bufOffset = 0;
		pReq->frags.cryptoSize = 0;
		pReq->frags.macSize = 0;

		config = pSA->config | (MV_CESA_FRAG_FIRST << MV_CESA_FRAG_MODE_OFFSET);

		fixOffset = pReq->fixOffset;
		 
		cryptoOffset = pCmd->cryptoOffset;
		macOffset = pCmd->macOffset;

		copySize = sizeof(cesaSramVirtPtr[chan]->buf) - pReq->fixOffset;

		mvCesaFragSizeFind(pSA, pReq, cryptoOffset, macOffset, &copySize, &cryptoDataSize, &macDataSize);

		if ((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_MAC_ONLY << MV_CESA_OPERATION_OFFSET)) {
			 
			if ((pSA->config & MV_CESA_CRYPTO_MODE_MASK) == (MV_CESA_CRYPTO_CBC << MV_CESA_CRYPTO_MODE_BIT)) {
				 
				if ((pCmd->ivFromUser) &&
				    ((pSA->config & MV_CESA_DIRECTION_MASK) ==
				     (MV_CESA_DIR_ENCODE << MV_CESA_DIRECTION_BIT))) {

					i += mvCesaDmaCopyPrepare(chan, pCmd->pSrc, cesaSramVirtPtr[chan]->cryptoIV,
									&pDmaDesc[i], MV_FALSE, pCmd->ivOffset,
									pSA->cryptoIvSize, pCmd->skipFlush);
				}

				if (pCmd->ivOffset > (copySize - pSA->cryptoIvSize)) {
					 
					cryptoIvOffset = cesaSramVirtPtr[chan]->tempCryptoIV - mvCesaSramAddrGet(chan);

					if ((pSA->config & MV_CESA_DIRECTION_MASK) ==
					    (MV_CESA_DIR_DECODE << MV_CESA_DIRECTION_BIT)) {
						i += mvCesaDmaCopyPrepare(chan, pCmd->pSrc,
										cesaSramVirtPtr[chan]->tempCryptoIV,
										&pDmaDesc[i], MV_FALSE, pCmd->ivOffset,
										pSA->cryptoIvSize, pCmd->skipFlush);
					} else {
						 
						if (pCmd->ivFromUser == 0) {
							 
							i += mvCesaDmaCopyPrepare(chan, pCmd->pSrc,
								cesaSramVirtPtr[chan]->cryptoIV, &pDmaDesc[i],
								MV_TRUE, pCmd->ivOffset, pSA->cryptoIvSize, pCmd->skipFlush);
						}
					}
				} else {
					cryptoIvOffset = pCmd->ivOffset;
				}
			}
		}

		if ((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET)) {
			 
			if ((pSA->config & MV_CESA_DIRECTION_MASK) == (MV_CESA_DIR_DECODE << MV_CESA_DIRECTION_BIT)) {
				 
				mvCesaCopyFromMbuf(pReq->frags.orgDigest,
						   pCmd->pSrc, pCmd->digestOffset, pSA->digestSize);

				if (pCmd->digestOffset > (copySize - pSA->digestSize)) {
					MV_U8 digestZero[MV_CESA_MAX_DIGEST_SIZE];

					memset(digestZero, 0, MV_CESA_MAX_DIGEST_SIZE);
					mvCesaCopyToMbuf(digestZero, pCmd->pSrc, pCmd->digestOffset, pSA->digestSize);

					digestOffset = cesaSramVirtPtr[chan]->tempDigest - mvCesaSramAddrGet(chan);
				} else {
					digestOffset = pCmd->digestOffset;
				}
			}
		}
		 
		if (cesaLastSid[chan] != sid) {
			mvCesaSramSaUpdate(chan, sid, &pDmaDesc[i]);
			i++;
		}

		pReq->fragMode = MV_CESA_FRAG_MIDDLE;
	} else {
		 
		fixOffset = 0;
		cryptoOffset = 0;
		macOffset = 0;
		if ((pCmd->pSrc->mbufSize - pReq->frags.bufOffset) <= sizeof(cesaSramVirtPtr[chan]->buf)) {
			 
			config = pSA->config | (MV_CESA_FRAG_LAST << MV_CESA_FRAG_MODE_OFFSET);
			pReq->fragMode = MV_CESA_FRAG_LAST;
			copySize = pCmd->pSrc->mbufSize - pReq->frags.bufOffset;

			if ((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET)) {
				macDataSize = pCmd->macLength - pReq->frags.macSize;

				if (pCmd->digestOffset < pReq->frags.bufOffset) {
					 
					digestOffset = cesaSramVirtPtr[chan]->tempDigest - mvCesaSramAddrGet(chan);
				} else {
					digestOffset = pCmd->digestOffset - pReq->frags.bufOffset;
				}
				pReq->frags.newDigestOffset = digestOffset;
				macTotalLen = pCmd->macLength;
			}

			if ((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_MAC_ONLY << MV_CESA_OPERATION_OFFSET))
				cryptoDataSize = pCmd->cryptoLength - pReq->frags.cryptoSize;

		} else {
			 
			config = pSA->config | (MV_CESA_FRAG_MIDDLE << MV_CESA_FRAG_MODE_OFFSET);
			copySize = sizeof(cesaSramVirtPtr[chan]->buf);
			 
			mvCesaFragSizeFind(pSA, pReq, cryptoOffset, macOffset,
					   &copySize, &cryptoDataSize, &macDataSize);
		}
	}

	pMbuf = pCmd->pSrc;
	i += mvCesaDmaCopyPrepare(chan, pMbuf, pSramBuf + fixOffset, &pDmaDesc[i],
				  MV_FALSE, pReq->frags.bufOffset, copySize, pCmd->skipFlush);

	mvCesaSramDescrBuild(chan, config, frag,
			     cryptoOffset + fixOffset, cryptoIvOffset + fixOffset,
			     cryptoDataSize, macOffset + fixOffset,
			     digestOffset + fixOffset, macDataSize, macTotalLen, pReq, &pDmaDesc[i]);
	i++;

	pDmaDesc[i].byteCnt = 0;
	pDmaDesc[i].phySrcAdd = 0;
	pDmaDesc[i].phyDestAdd = 0;
	i++;

	pMbuf = pCmd->pDst;
	i += mvCesaDmaCopyPrepare(chan, pMbuf, pSramBuf + fixOffset, &pDmaDesc[i],
				  MV_TRUE, pReq->frags.bufOffset, copySize, pCmd->skipFlush);

	pDmaDesc[i - 1].phyNextDescPtr = 0;
	pReq->dma[frag].pDmaLast = &pDmaDesc[i - 1];
	mvOsCacheFlush(NULL, pReq->dma[frag].pDmaFirst, i * sizeof(MV_DMA_DESC));

	pReq->frags.bufOffset += copySize;
	pReq->frags.cryptoSize += cryptoDataSize;
	pReq->frags.macSize += macDataSize;

	return MV_OK;
}

static MV_STATUS mvCesaReqProcess(MV_U8 chan, MV_CESA_REQ *pReq)
{
	MV_CESA_MBUF *pMbuf;
	MV_DMA_DESC *pDmaDesc;
	MV_U8 *pSramBuf;
	int sid, i, fixOffset;
	MV_CESA_SA *pSA;
	MV_CESA_COMMAND *pCmd = pReq->pCmd;

	cesaStats.procCount++;

	sid = pCmd->sessionId;
	pSA = pCesaSAD[sid];
	pDmaDesc = pReq->dma[0].pDmaFirst;
	pSramBuf = cesaSramVirtPtr[chan]->buf;
	fixOffset = pReq->fixOffset;

	i = 0;

	if (((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_MAC_ONLY << MV_CESA_OPERATION_OFFSET)) &&
	    ((pSA->config & MV_CESA_CRYPTO_MODE_MASK) == (MV_CESA_CRYPTO_CBC << MV_CESA_CRYPTO_MODE_BIT)) &&
	    ((pSA->config & MV_CESA_DIRECTION_MASK) == (MV_CESA_DIR_ENCODE << MV_CESA_DIRECTION_BIT)) &&
	    (pCmd->ivFromUser)) {
		 
		i += mvCesaDmaCopyPrepare(chan, pCmd->pSrc, cesaSramVirtPtr[chan]->cryptoIV, &pDmaDesc[i],
					  MV_FALSE, pCmd->ivOffset, pSA->cryptoIvSize, pCmd->skipFlush);
	}

	if (cesaLastSid[chan] != sid) {
		mvCesaSramSaUpdate(chan, sid, &pDmaDesc[i]);
		i++;
	}

	pMbuf = pCmd->pSrc;
	i += mvCesaDmaCopyPrepare(chan, pMbuf, pSramBuf + fixOffset, &pDmaDesc[i],
				  MV_FALSE, 0, pMbuf->mbufSize, pCmd->skipFlush);

	mvCesaSramDescrBuild(chan, pSA->config, 0, pCmd->cryptoOffset + fixOffset,
			     pCmd->ivOffset + fixOffset, pCmd->cryptoLength,
			     pCmd->macOffset + fixOffset, pCmd->digestOffset + fixOffset,
			     pCmd->macLength, pCmd->macLength, pReq, &pDmaDesc[i]);
	i++;

	pDmaDesc[i].byteCnt = 0;
	pDmaDesc[i].phySrcAdd = 0;
	pDmaDesc[i].phyDestAdd = 0;
	i++;

	pMbuf = pCmd->pDst;
	i += mvCesaDmaCopyPrepare(chan, pMbuf, pSramBuf + fixOffset, &pDmaDesc[i],
				  MV_TRUE, 0, pMbuf->mbufSize, pCmd->skipFlush);

	pDmaDesc[i - 1].phyNextDescPtr = 0;
	pReq->dma[0].pDmaLast = &pDmaDesc[i - 1];
	mvOsCacheFlush(NULL, pReq->dma[0].pDmaFirst, i * sizeof(MV_DMA_DESC));

	return MV_OK;
}

static void mvCesaSramDescrBuild(MV_U8 chan, MV_U32 config, int frag,
				 int cryptoOffset, int ivOffset, int cryptoLength,
				 int macOffset, int digestOffset, int macLength,
				 int macTotalLen, MV_CESA_REQ *pReq, MV_DMA_DESC *pDmaDesc)
{
	MV_CESA_DESC *pCesaDesc = &pReq->pCesaDesc[frag];
	MV_CESA_DESC *pSramDesc = &cesaSramVirtPtr[chan]->desc;
	MV_U16 sramBufOffset = (MV_U16)((MV_U8 *)cesaSramVirtPtr[chan]->buf - mvCesaSramAddrGet(chan));

	pCesaDesc->config = MV_32BIT_LE(config);

	if ((config & MV_CESA_OPERATION_MASK) != (MV_CESA_MAC_ONLY << MV_CESA_OPERATION_OFFSET)) {
		 
		pCesaDesc->cryptoSrcOffset = MV_16BIT_LE(sramBufOffset + cryptoOffset);
		pCesaDesc->cryptoDstOffset = MV_16BIT_LE(sramBufOffset + cryptoOffset);
		 
		pCesaDesc->cryptoDataLen = MV_16BIT_LE(cryptoLength);
		 
		pCesaDesc->cryptoKeyOffset = MV_16BIT_LE((MV_U16) (cesaSramVirtPtr[chan]->sramSA.cryptoKey -
								   mvCesaSramAddrGet(chan)));
		 
		pCesaDesc->cryptoIvOffset = MV_16BIT_LE((MV_U16) (cesaSramVirtPtr[chan]->cryptoIV - mvCesaSramAddrGet(chan)));
		pCesaDesc->cryptoIvBufOffset = MV_16BIT_LE(sramBufOffset + ivOffset);
	}

	if ((config & MV_CESA_OPERATION_MASK) != (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET)) {
		 
		pCesaDesc->macSrcOffset = MV_16BIT_LE(sramBufOffset + macOffset);
		pCesaDesc->macTotalLen = MV_16BIT_LE(macTotalLen);

		pCesaDesc->macDigestOffset = MV_16BIT_LE(sramBufOffset + digestOffset);
		pCesaDesc->macDataLen = MV_16BIT_LE(macLength);

		pCesaDesc->macInnerIvOffset = MV_16BIT_LE((MV_U16) (cesaSramVirtPtr[chan]->sramSA.macInnerIV -
								    mvCesaSramAddrGet(chan)));
		pCesaDesc->macOuterIvOffset = MV_16BIT_LE((MV_U16) (cesaSramVirtPtr[chan]->sramSA.macOuterIV -
								    mvCesaSramAddrGet(chan)));
	}
	 
	pDmaDesc->phySrcAdd = MV_32BIT_LE(mvCesaVirtToPhys(&pReq->cesaDescBuf, pCesaDesc));
	pDmaDesc->phyDestAdd = MV_32BIT_LE(mvCesaSramVirtToPhys(chan, NULL, (MV_U8 *) pSramDesc));
	pDmaDesc->byteCnt = MV_32BIT_LE(sizeof(MV_CESA_DESC) | BIT31);

	mvOsCacheFlush(NULL, pCesaDesc, sizeof(MV_CESA_DESC));
}

static INLINE void mvCesaSramSaUpdate(MV_U8 chan, short sid, MV_DMA_DESC *pDmaDesc)
{
	MV_CESA_SA *pSA = pCesaSAD[sid];

	pDmaDesc->byteCnt = MV_32BIT_LE(sizeof(MV_CESA_SRAM_SA) | BIT31);
	pDmaDesc->phySrcAdd = pSA->sramSAPhysAddr;
	pDmaDesc->phyDestAdd = MV_32BIT_LE(mvCesaSramVirtToPhys(chan, NULL, (MV_U8 *)&cesaSramVirtPtr[chan]->sramSA));

}

#ifndef MV_NETBSD
static INLINE int mvCesaDmaCopyPrepare(MV_U8 chan, MV_CESA_MBUF *pMbuf, MV_U8 *pSramBuf,
				       MV_DMA_DESC *pDmaDesc, MV_BOOL isToMbuf,
				       int offset, int copySize, MV_BOOL skipFlush)
{
	int bufOffset, bufSize, size, frag, i;
	MV_U8 *pBuf;

	i = 0;

	frag = mvCesaMbufOffset(pMbuf, offset, &bufOffset);
	bufSize = pMbuf->pFrags[frag].bufSize - bufOffset;
	pBuf = pMbuf->pFrags[frag].bufVirtPtr + bufOffset;

	size = 0;

	while (size < copySize) {
		 
		bufSize = MV_MIN(bufSize, (copySize - size));
		pDmaDesc[i].byteCnt = MV_32BIT_LE(bufSize | BIT31);
		if (isToMbuf) {
			pDmaDesc[i].phyDestAdd = MV_32BIT_LE(mvOsIoVirtToPhy(NULL, pBuf));
			pDmaDesc[i].phySrcAdd = MV_32BIT_LE(mvCesaSramVirtToPhys(chan, NULL, (pSramBuf + size)));
			 
			if (skipFlush == MV_FALSE)
				mvOsCacheInvalidate(NULL, pBuf, bufSize);
		} else {
			pDmaDesc[i].phySrcAdd = MV_32BIT_LE(mvOsIoVirtToPhy(NULL, pBuf));
			pDmaDesc[i].phyDestAdd = MV_32BIT_LE(mvCesaSramVirtToPhys(chan, NULL, (pSramBuf + size)));
			 
			if (skipFlush == MV_FALSE)
				mvOsCacheFlush(NULL, pBuf, bufSize);
		}

		i++;
		size += bufSize;

		frag++;
		pBuf = pMbuf->pFrags[frag].bufVirtPtr;
		bufSize = pMbuf->pFrags[frag].bufSize;
	}
	return i;
}
#else  
static int mvCesaDmaCopyPrepare(MV_U8 chan, MV_CESA_MBUF *pMbuf, MV_U8 *pSramBuf,
				MV_DMA_DESC *pDmaDesc, MV_BOOL isToMbuf, int offset, int copySize, MV_BOOL skipFlush)
{
	int bufOffset, bufSize, thisSize, size, frag, i;
	MV_ULONG bufPhys, sramPhys;
	MV_U8 *pBuf;

	frag = mvCesaMbufOffset(pMbuf, offset, &bufOffset);

	sramPhys = mvCesaSramVirtToPhys(chan, NULL, pSramBuf);

	size = i = 0;

	while (size < copySize) {
		 
		bufSize = pMbuf->pFrags[frag].bufSize - bufOffset;
		pBuf = pMbuf->pFrags[frag].bufVirtPtr + bufOffset;
		bufOffset = 0;	 
		frag++;

		while (bufSize > 0) {
			 
			thisSize = PAGE_SIZE - (((MV_ULONG) pBuf) & (PAGE_SIZE - 1));
			thisSize = MV_MIN(bufSize, thisSize);
			 
			if (thisSize > (copySize - size)) {
				thisSize = copySize - size;
				bufSize = 0;
			}

			bufPhys = MV_32BIT_LE(mvOsIoVirtToPhy(NULL, pBuf));

			pDmaDesc[i].byteCnt = MV_32BIT_LE(thisSize | BIT31);
			if (isToMbuf) {
				pDmaDesc[i].phyDestAdd = bufPhys;
				pDmaDesc[i].phySrcAdd = MV_32BIT_LE(sramPhys);
				 
				if (skipFlush == MV_FALSE)
					mvOsCacheInvalidate(NULL, pBuf, thisSize);
			} else {
				pDmaDesc[i].phySrcAdd = bufPhys;
				pDmaDesc[i].phyDestAdd = MV_32BIT_LE(sramPhys);
				 
				if (skipFlush == MV_FALSE)
					mvOsCacheFlush(NULL, pBuf, thisSize);
			}

			pDmaDesc[i].phyNextDescPtr = MV_32BIT_LE(mvOsIoVirtToPhy(NULL, (&pDmaDesc[i + 1])));

			mvOsCacheFlush(NULL, &pDmaDesc[i], sizeof(MV_DMA_DESC));

			bufSize -= thisSize;
			sramPhys += thisSize;
			pBuf += thisSize;
			size += thisSize;
			i++;
		}
	}

	return i;
}
#endif  
 
static void mvCesaHmacIvGet(MV_CESA_MAC_MODE macMode, unsigned char key[], int keyLength,
			    unsigned char innerIV[], unsigned char outerIV[])
{
	unsigned char inner[MV_CESA_MAX_MAC_KEY_LENGTH];
	unsigned char outer[MV_CESA_MAX_MAC_KEY_LENGTH];
	int i, digestSize = 0;
#if defined(MV_CPU_LE) || defined(MV_PPC)
	MV_U32 swapped32, val32, *pVal32;
#endif
	for (i = 0; i < keyLength; i++) {
		inner[i] = 0x36 ^ key[i];
		outer[i] = 0x5c ^ key[i];
	}

	for (i = keyLength; i < MV_CESA_MAX_MAC_KEY_LENGTH; i++) {
		inner[i] = 0x36;
		outer[i] = 0x5c;
	}
	if (macMode == MV_CESA_MAC_HMAC_MD5) {
		MV_MD5_CONTEXT ctx;

		mvMD5Init(&ctx);
		mvMD5Update(&ctx, inner, MV_CESA_MAX_MAC_KEY_LENGTH);

		memcpy(innerIV, ctx.buf, MV_CESA_MD5_DIGEST_SIZE);
		memset(&ctx, 0, sizeof(ctx));

		mvMD5Init(&ctx);
		mvMD5Update(&ctx, outer, MV_CESA_MAX_MAC_KEY_LENGTH);
		memcpy(outerIV, ctx.buf, MV_CESA_MD5_DIGEST_SIZE);
		memset(&ctx, 0, sizeof(ctx));
		digestSize = MV_CESA_MD5_DIGEST_SIZE;
	} else if (macMode == MV_CESA_MAC_HMAC_SHA1) {
		MV_SHA1_CTX ctx;

		mvSHA1Init(&ctx);
		mvSHA1Update(&ctx, inner, MV_CESA_MAX_MAC_KEY_LENGTH);
		memcpy(innerIV, ctx.state, MV_CESA_SHA1_DIGEST_SIZE);
		memset(&ctx, 0, sizeof(ctx));

		mvSHA1Init(&ctx);
		mvSHA1Update(&ctx, outer, MV_CESA_MAX_MAC_KEY_LENGTH);
		memcpy(outerIV, ctx.state, MV_CESA_SHA1_DIGEST_SIZE);
		memset(&ctx, 0, sizeof(ctx));
		digestSize = MV_CESA_SHA1_DIGEST_SIZE;
	} else if (macMode == MV_CESA_MAC_HMAC_SHA2) {
		sha256_context ctx;

		mvSHA256Init(&ctx);
		mvSHA256Update(&ctx, inner, MV_CESA_MAX_MAC_KEY_LENGTH);
		memcpy(innerIV, ctx.state, MV_CESA_SHA2_DIGEST_SIZE);
		memset(&ctx, 0, sizeof(ctx));

		mvSHA256Init(&ctx);
		mvSHA256Update(&ctx, outer, MV_CESA_MAX_MAC_KEY_LENGTH);
		memcpy(outerIV, ctx.state, MV_CESA_SHA2_DIGEST_SIZE);
		memset(&ctx, 0, sizeof(ctx));
		digestSize = MV_CESA_SHA2_DIGEST_SIZE;
	} else {
		mvOsPrintf("hmacGetIV: Unexpected macMode %d\n", macMode);
	}
#if defined(MV_CPU_LE) || defined(MV_PPC)
	 
	pVal32 = (MV_U32 *) innerIV;
	for (i = 0; i < digestSize / 4; i++) {
		val32 = *pVal32;
		swapped32 = MV_BYTE_SWAP_32BIT(val32);
		*pVal32 = swapped32;
		pVal32++;
	}
	pVal32 = (MV_U32 *) outerIV;
	for (i = 0; i < digestSize / 4; i++) {
		val32 = *pVal32;
		swapped32 = MV_BYTE_SWAP_32BIT(val32);
		*pVal32 = swapped32;
		pVal32++;
	}
#endif  
}

static void mvCesaFragSha1Complete(MV_U8 chan, MV_CESA_MBUF *pMbuf, int offset,
				   MV_U8 *pOuterIV, int macLeftSize, int macTotalSize, MV_U8 *pDigest)
{
	MV_SHA1_CTX ctx;
	MV_U8 *pData;
	int i, frag, fragOffset, size;

	for (i = 0; i < MV_CESA_SHA1_DIGEST_SIZE / 4; i++)
		ctx.state[i] = MV_REG_READ(MV_CESA_AUTH_INIT_VAL_DIGEST_REG(chan, i));

	memset(ctx.buffer, 0, 64);
	 
	ctx.count[0] = ((macTotalSize - macLeftSize) * 8);
	ctx.count[1] = 0;

	if (pOuterIV != NULL)
		ctx.count[0] += (64 * 8);

	frag = mvCesaMbufOffset(pMbuf, offset, &fragOffset);
	if (frag == MV_INVALID) {
		mvOsPrintf("CESA Mbuf Error: offset (%d) out of range\n", offset);
		return;
	}

	pData = pMbuf->pFrags[frag].bufVirtPtr + fragOffset;
	size = pMbuf->pFrags[frag].bufSize - fragOffset;

	while (macLeftSize > 0) {
		if (macLeftSize <= size) {
			mvSHA1Update(&ctx, pData, macLeftSize);
			break;
		}
		mvSHA1Update(&ctx, pData, size);
		macLeftSize -= size;
		frag++;
		pData = pMbuf->pFrags[frag].bufVirtPtr;
		size = pMbuf->pFrags[frag].bufSize;
	}
	mvSHA1Final(pDigest, &ctx);
 
	if (pOuterIV != NULL) {
		 
		for (i = 0; i < MV_CESA_SHA1_DIGEST_SIZE / 4; i++) {
#if defined(MV_CPU_LE) || defined(MV_ARM)
			ctx.state[i] = MV_BYTE_SWAP_32BIT(((MV_U32 *) pOuterIV)[i]);
#else
			ctx.state[i] = ((MV_U32 *) pOuterIV)[i];
#endif
		}
		memset(ctx.buffer, 0, 64);

		ctx.count[0] = 64 * 8;
		ctx.count[1] = 0;
		mvSHA1Update(&ctx, pDigest, MV_CESA_SHA1_DIGEST_SIZE);
		mvSHA1Final(pDigest, &ctx);
	}
}

static void mvCesaFragMd5Complete(MV_U8 chan, MV_CESA_MBUF *pMbuf, int offset,
				  MV_U8 *pOuterIV, int macLeftSize, int macTotalSize, MV_U8 *pDigest)
{
	MV_MD5_CONTEXT ctx;
	MV_U8 *pData;
	int i, frag, fragOffset, size;

	for (i = 0; i < MV_CESA_MD5_DIGEST_SIZE / 4; i++)
		ctx.buf[i] = MV_REG_READ(MV_CESA_AUTH_INIT_VAL_DIGEST_REG(chan, i));

	memset(ctx.in, 0, 64);

	ctx.bits[0] = ((macTotalSize - macLeftSize) * 8);
	ctx.bits[1] = 0;

	if (pOuterIV != NULL)
		ctx.bits[0] += (64 * 8);

	frag = mvCesaMbufOffset(pMbuf, offset, &fragOffset);
	if (frag == MV_INVALID) {
		mvOsPrintf("CESA Mbuf Error: offset (%d) out of range\n", offset);
		return;
	}

	pData = pMbuf->pFrags[frag].bufVirtPtr + fragOffset;
	size = pMbuf->pFrags[frag].bufSize - fragOffset;

	while (macLeftSize > 0) {
		if (macLeftSize <= size) {
			mvMD5Update(&ctx, pData, macLeftSize);
			break;
		}
		mvMD5Update(&ctx, pData, size);
		macLeftSize -= size;
		frag++;
		pData = pMbuf->pFrags[frag].bufVirtPtr;
		size = pMbuf->pFrags[frag].bufSize;
	}
	mvMD5Final(pDigest, &ctx);

	if (pOuterIV != NULL) {
		 
		for (i = 0; i < MV_CESA_MD5_DIGEST_SIZE / 4; i++) {
#if defined(MV_CPU_LE) || defined(MV_ARM)
			ctx.buf[i] = MV_BYTE_SWAP_32BIT(((MV_U32 *) pOuterIV)[i]);
#else
			ctx.buf[i] = ((MV_U32 *) pOuterIV)[i];
#endif
		}
		memset(ctx.in, 0, 64);

		ctx.bits[0] = 64 * 8;
		ctx.bits[1] = 0;
		mvMD5Update(&ctx, pDigest, MV_CESA_MD5_DIGEST_SIZE);
		mvMD5Final(pDigest, &ctx);
	}
}

static void mvCesaFragSha2Complete(MV_U8 chan, MV_CESA_MBUF *pMbuf, int offset,
				   MV_U8 *pOuterIV, int macLeftSize, int macTotalSize, MV_U8 *pDigest)
{
	sha256_context ctx;
	MV_U8 *pData;
	int i, frag, fragOffset, size;

	for (i = 0; i < MV_CESA_SHA2_DIGEST_SIZE / 4; i++)
		ctx.state[i] = MV_REG_READ(MV_CESA_AUTH_INIT_VAL_DIGEST_REG(chan, i));

	memset(ctx.buffer, 0, 64);
	 
	ctx.total[0] = ((macTotalSize - macLeftSize) * 8);
	ctx.total[1] = 0;

	if (pOuterIV != NULL)
		ctx.total[0] += (64 * 8);

	frag = mvCesaMbufOffset(pMbuf, offset, &fragOffset);
	if (frag == MV_INVALID) {
		mvOsPrintf("CESA Mbuf Error: offset (%d) out of range\n", offset);
		return;
	}

	pData = pMbuf->pFrags[frag].bufVirtPtr + fragOffset;
	size = pMbuf->pFrags[frag].bufSize - fragOffset;

	while (macLeftSize > 0) {
		if (macLeftSize <= size) {
			mvSHA256Update(&ctx, pData, macLeftSize);
			break;
		}
		mvSHA256Update(&ctx, pData, size);
		macLeftSize -= size;
		frag++;
		pData = pMbuf->pFrags[frag].bufVirtPtr;
		size = pMbuf->pFrags[frag].bufSize;
	}
	mvSHA256Finish(&ctx, pDigest);
 
	if (pOuterIV != NULL) {
		 
		for (i = 0; i < MV_CESA_SHA2_DIGEST_SIZE / 4; i++) {
#if defined(MV_CPU_LE) || defined(MV_ARM)
			ctx.state[i] = MV_BYTE_SWAP_32BIT(((MV_U32 *) pOuterIV)[i]);
#else
			ctx.state[i] = ((MV_U32 *) pOuterIV)[i];
#endif
		}
		memset(ctx.buffer, 0, 64);

		ctx.total[0] = 64 * 8;
		ctx.total[1] = 0;
		mvSHA256Update(&ctx, pDigest, MV_CESA_SHA2_DIGEST_SIZE);
		mvSHA256Finish(&ctx, pDigest);
	}
}

static MV_STATUS mvCesaFragAuthComplete(MV_U8 chan, MV_CESA_REQ *pReq, MV_CESA_SA *pSA, int macDataSize)
{
	MV_CESA_COMMAND *pCmd = pReq->pCmd;
	MV_U8 *pDigest;
	MV_CESA_MAC_MODE macMode;
	MV_U8 *pOuterIV = NULL;

	if (pCmd->pSrc != pCmd->pDst)
		mvCesaMbufCopy(pCmd->pDst, pReq->frags.bufOffset, pCmd->pSrc, pReq->frags.bufOffset, macDataSize);

	pDigest = (mvCesaSramAddrGet(chan) + pReq->frags.newDigestOffset);

	macMode = (pSA->config & MV_CESA_MAC_MODE_MASK) >> MV_CESA_MAC_MODE_OFFSET;
 
	switch (macMode) {
	case MV_CESA_MAC_HMAC_MD5:
		pOuterIV = pSA->pSramSA->macOuterIV;
		 
	case MV_CESA_MAC_MD5:
		mvCesaFragMd5Complete(chan, pCmd->pDst, pReq->frags.bufOffset, pOuterIV,
				      macDataSize, pCmd->macLength, pDigest);
		break;

	case MV_CESA_MAC_HMAC_SHA1:
		pOuterIV = pSA->pSramSA->macOuterIV;
		 
	case MV_CESA_MAC_SHA1:
		mvCesaFragSha1Complete(chan, pCmd->pDst, pReq->frags.bufOffset, pOuterIV,
				       macDataSize, pCmd->macLength, pDigest);
		break;

	case MV_CESA_MAC_HMAC_SHA2:
		pOuterIV = pSA->pSramSA->macOuterIV;
		 
	case MV_CESA_MAC_SHA2:
		mvCesaFragSha2Complete(chan, pCmd->pDst, pReq->frags.bufOffset, pOuterIV,
				       macDataSize, pCmd->macLength, pDigest);
		break;

	default:
		mvOsPrintf("mvCesaFragAuthComplete: Unexpected macMode %d\n", macMode);
		return MV_BAD_PARAM;
	}
	return MV_OK;
}

static MV_CESA_COMMAND *mvCesaCtrModeInit(void)
{
	MV_CESA_MBUF *pMbuf;
	MV_U8 *pBuf;
	MV_CESA_COMMAND *pCmd;

	pBuf = mvOsMalloc(sizeof(MV_CESA_COMMAND) + sizeof(MV_CESA_MBUF) + sizeof(MV_BUF_INFO) + 100);
	if (pBuf == NULL) {
		mvOsPrintf("mvCesaCtrModeInit: Can't allocate %u bytes for CTR Mode\n",
			   sizeof(MV_CESA_COMMAND) + sizeof(MV_CESA_MBUF) + sizeof(MV_BUF_INFO));
		return NULL;
	}
	pCmd = (MV_CESA_COMMAND *)pBuf;
	pBuf += sizeof(MV_CESA_COMMAND);

	pMbuf = (MV_CESA_MBUF *)pBuf;
	pBuf += sizeof(MV_CESA_MBUF);

	pMbuf->pFrags = (MV_BUF_INFO *)pBuf;

	pMbuf->numFrags = 1;
	pCmd->pSrc = pMbuf;
	pCmd->pDst = pMbuf;
 
	return pCmd;
}

static MV_STATUS mvCesaCtrModePrepare(MV_CESA_COMMAND *pCtrModeCmd, MV_CESA_COMMAND *pCmd)
{
	MV_CESA_MBUF *pMbuf;
	MV_U8 *pBuf, *pIV;
	MV_U32 counter, *pCounter;
	int cryptoSize = MV_ALIGN_UP(pCmd->cryptoLength, MV_CESA_AES_BLOCK_SIZE);
 
	pMbuf = pCtrModeCmd->pSrc;

	pBuf = mvOsIoCachedMalloc(cesaOsHandle, cryptoSize, &pMbuf->pFrags[0].bufPhysAddr, &pMbuf->pFrags[0].memHandle);
	if (pBuf == NULL) {
		mvOsPrintf("mvCesaCtrModePrepare: Can't allocate %d bytes\n", cryptoSize);
		return MV_OUT_OF_CPU_MEM;
	}
	memset(pBuf, 0, cryptoSize);
	mvOsCacheFlush(NULL, pBuf, cryptoSize);

	pMbuf->pFrags[0].bufVirtPtr = pBuf;
	pMbuf->mbufSize = cryptoSize;
	pMbuf->pFrags[0].bufSize = cryptoSize;

	pCtrModeCmd->pReqPrv = pCmd->pReqPrv;
	pCtrModeCmd->sessionId = pCmd->sessionId;

	pCtrModeCmd->cryptoOffset = 0;
	pCtrModeCmd->cryptoLength = cryptoSize;

	mvCesaCopyFromMbuf(pBuf, pCmd->pSrc, pCmd->ivOffset, MV_CESA_AES_BLOCK_SIZE);
	pCounter = (MV_U32 *)(pBuf + (MV_CESA_AES_BLOCK_SIZE - sizeof(counter)));
	counter = *pCounter;
	counter = MV_32BIT_BE(counter);
	pIV = pBuf;
	cryptoSize -= MV_CESA_AES_BLOCK_SIZE;

	while (cryptoSize > 0) {
		pBuf += MV_CESA_AES_BLOCK_SIZE;
		memcpy(pBuf, pIV, MV_CESA_AES_BLOCK_SIZE - sizeof(counter));
		pCounter = (MV_U32 *)(pBuf + (MV_CESA_AES_BLOCK_SIZE - sizeof(counter)));
		counter++;
		*pCounter = MV_32BIT_BE(counter);
		cryptoSize -= MV_CESA_AES_BLOCK_SIZE;
	}

	return MV_OK;
}

static MV_STATUS mvCesaCtrModeComplete(MV_CESA_COMMAND *pOrgCmd, MV_CESA_COMMAND *pCmd)
{
	int srcFrag, dstFrag, srcOffset, dstOffset, keyOffset, srcSize, dstSize;
	int cryptoSize = pCmd->cryptoLength;
	MV_U8 *pSrc, *pDst, *pKey;
	MV_STATUS status = MV_OK;
 
	pKey = pCmd->pDst->pFrags[0].bufVirtPtr;
	keyOffset = 0;

	if ((pOrgCmd->pSrc != pOrgCmd->pDst) && (pOrgCmd->cryptoOffset > 0)) {
		 
		status = mvCesaMbufCopy(pOrgCmd->pDst, 0, pOrgCmd->pSrc, 0, pOrgCmd->cryptoOffset);
 
	}

	srcFrag = mvCesaMbufOffset(pOrgCmd->pSrc, pOrgCmd->cryptoOffset, &srcOffset);
	pSrc = pOrgCmd->pSrc->pFrags[srcFrag].bufVirtPtr;
	srcSize = pOrgCmd->pSrc->pFrags[srcFrag].bufSize;

	dstFrag = mvCesaMbufOffset(pOrgCmd->pDst, pOrgCmd->cryptoOffset, &dstOffset);
	pDst = pOrgCmd->pDst->pFrags[dstFrag].bufVirtPtr;
	dstSize = pOrgCmd->pDst->pFrags[dstFrag].bufSize;

	while (cryptoSize > 0) {
		pDst[dstOffset] = (pSrc[srcOffset] ^ pKey[keyOffset]);

		cryptoSize--;
		dstOffset++;
		srcOffset++;
		keyOffset++;

		if (srcOffset >= srcSize) {
			srcFrag++;
			srcOffset = 0;
			pSrc = pOrgCmd->pSrc->pFrags[srcFrag].bufVirtPtr;
			srcSize = pOrgCmd->pSrc->pFrags[srcFrag].bufSize;
		}

		if (dstOffset >= dstSize) {
			dstFrag++;
			dstOffset = 0;
			pDst = pOrgCmd->pDst->pFrags[dstFrag].bufVirtPtr;
			dstSize = pOrgCmd->pDst->pFrags[dstFrag].bufSize;
		}
	}

	if (pOrgCmd->pSrc != pOrgCmd->pDst) {
		 
		srcOffset = pOrgCmd->cryptoOffset + pOrgCmd->cryptoLength;

		if ((pOrgCmd->pDst->mbufSize - srcOffset) > 0) {
			status = mvCesaMbufCopy(pOrgCmd->pDst, srcOffset,
						pOrgCmd->pSrc, srcOffset, pOrgCmd->pDst->mbufSize - srcOffset);
		}

	}

	mvOsIoCachedFree(cesaOsHandle, pCmd->pDst->pFrags[0].bufSize,
			 pCmd->pDst->pFrags[0].bufPhysAddr,
			 pCmd->pDst->pFrags[0].bufVirtPtr, pCmd->pDst->pFrags[0].memHandle);

	return MV_OK;
}

static void mvCesaCtrModeFinish(MV_CESA_COMMAND *pCmd)
{
	mvOsFree(pCmd);
}

static MV_STATUS mvCesaParamCheck(MV_CESA_SA *pSA, MV_CESA_COMMAND *pCmd, MV_U8 *pFixOffset)
{
	MV_U8 fixOffset = 0xFF;

	if (((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET))) {
		 
		if (MV_IS_NOT_ALIGN(pCmd->macOffset, 4)) {
			mvOsPrintf("mvCesaAction: macOffset %d must be 4 byte aligned\n", pCmd->macOffset);
			return MV_BAD_PARAM;
		}
		 
		if (MV_IS_NOT_ALIGN(pCmd->digestOffset, 4)) {
			mvOsPrintf("mvCesaAction: digestOffset %d must be 4 byte aligned\n", pCmd->digestOffset);
			return MV_BAD_PARAM;
		}
		 
		if (fixOffset == 0xFF) {
			fixOffset = (pCmd->macOffset % 8);
		} else {
			if ((pCmd->macOffset % 8) != fixOffset) {
				mvOsPrintf("mvCesaAction: macOffset %d mod 8 must be equal %d\n",
					   pCmd->macOffset, fixOffset);
				return MV_BAD_PARAM;
			}
		}
		if ((pCmd->digestOffset % 8) != fixOffset) {
			mvOsPrintf("mvCesaAction: digestOffset %d mod 8 must be equal %d\n",
				   pCmd->digestOffset, fixOffset);
			return MV_BAD_PARAM;
		}
	}
	 
	if (((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_MAC_ONLY << MV_CESA_OPERATION_OFFSET))) {
		 
		if (MV_IS_NOT_ALIGN(pCmd->cryptoOffset, 4)) {
			mvOsPrintf("CesaAction: cryptoOffset=%d must be 4 byte aligned\n", pCmd->cryptoOffset);
			return MV_BAD_PARAM;
		}
		 
		if (MV_IS_NOT_ALIGN(pCmd->cryptoLength, pSA->cryptoBlockSize)) {
			mvOsPrintf("mvCesaAction: cryptoLength=%d must be %d byte aligned\n",
				   pCmd->cryptoLength, pSA->cryptoBlockSize);
			return MV_BAD_PARAM;
		}
		if (fixOffset == 0xFF) {
			fixOffset = (pCmd->cryptoOffset % 8);
		} else {
			 
			if ((pCmd->cryptoOffset % 8) != fixOffset) {
				mvOsPrintf("mvCesaAction: cryptoOffset %d mod 8 must be equal %d \n",
					   pCmd->cryptoOffset, fixOffset);
				return MV_BAD_PARAM;
			}
		}

		if (pSA->cryptoIvSize > 0) {
			 
			if (((pCmd->ivOffset + pSA->cryptoIvSize) > pCmd->cryptoOffset) &&
			    (pCmd->ivOffset < (pCmd->cryptoOffset + pCmd->cryptoLength))) {
				mvOsPrintf
				    ("mvCesaFragParamCheck: cryptoIvOffset (%d) is part of cryptoLength (%d+%d)\n",
				     pCmd->ivOffset, pCmd->macOffset, pCmd->macLength);
				return MV_BAD_PARAM;
			}

			if (MV_IS_NOT_ALIGN(pCmd->ivOffset, 4)) {
				mvOsPrintf("CesaAction: ivOffset=%d must be 4 byte aligned\n", pCmd->ivOffset);
				return MV_BAD_PARAM;
			}
			 
			if ((pCmd->ivOffset % 8) != fixOffset) {
				mvOsPrintf("mvCesaAction: ivOffset %d mod 8 must be %d\n", pCmd->ivOffset, fixOffset);
				return MV_BAD_PARAM;
			}
		}
	}
	return MV_OK;
}

static MV_STATUS mvCesaFragParamCheck(MV_U8 chan, MV_CESA_SA *pSA, MV_CESA_COMMAND *pCmd)
{
	int offset;

	if (((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET))) {
		 
		if (pCmd->macOffset > (sizeof(cesaSramVirtPtr[chan]->buf) - MV_CESA_AUTH_BLOCK_SIZE)) {
			mvOsPrintf("mvCesaFragParamCheck: macOffset is too large (%d)\n", pCmd->macOffset);
			return MV_BAD_PARAM;
		}
		 
		if (((pCmd->macOffset + pCmd->macLength) > pCmd->pSrc->mbufSize) ||
		    ((pCmd->pSrc->mbufSize - (pCmd->macOffset + pCmd->macLength)) >= sizeof(cesaSramVirtPtr[chan]->buf))) {
			mvOsPrintf("mvCesaFragParamCheck: macLength is too large (%d), mbufSize=%d\n",
				   pCmd->macLength, pCmd->pSrc->mbufSize);
			return MV_BAD_PARAM;
		}
	}

	if (((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_MAC_ONLY << MV_CESA_OPERATION_OFFSET))) {
		 
		if ((pCmd->cryptoOffset + 4) > (sizeof(cesaSramVirtPtr[chan]->buf) - pSA->cryptoBlockSize)) {
			mvOsPrintf("mvCesaFragParamCheck: cryptoOffset is too large (%d)\n", pCmd->cryptoOffset);
			return MV_BAD_PARAM;
		}

		if (((pCmd->cryptoOffset + pCmd->cryptoLength) > pCmd->pSrc->mbufSize) ||
		    ((pCmd->pSrc->mbufSize - (pCmd->cryptoOffset + pCmd->cryptoLength)) >=
		     (sizeof(cesaSramVirtPtr[chan]->buf) - pSA->cryptoBlockSize))) {
			mvOsPrintf("mvCesaFragParamCheck: cryptoLength is too large (%d), mbufSize=%d\n",
				   pCmd->cryptoLength, pCmd->pSrc->mbufSize);
			return MV_BAD_PARAM;
		}
	}

	if (((pSA->config & MV_CESA_OPERATION_MASK) ==
	     (MV_CESA_MAC_THEN_CRYPTO << MV_CESA_OPERATION_OFFSET)) ||
	    ((pSA->config & MV_CESA_OPERATION_MASK) == (MV_CESA_CRYPTO_THEN_MAC << MV_CESA_OPERATION_OFFSET))) {

		if (pCmd->cryptoOffset > pCmd->macOffset)
			offset = pCmd->cryptoOffset - pCmd->macOffset;
		else
			offset = pCmd->macOffset - pCmd->cryptoOffset;

		if (MV_IS_NOT_ALIGN(offset, pSA->cryptoBlockSize)) {
 
			return MV_NOT_ALLOWED;
		}
		 
		if (((pCmd->digestOffset + pSA->digestSize) > pCmd->cryptoOffset) &&
		    (pCmd->digestOffset < (pCmd->cryptoOffset + pCmd->cryptoLength))) {
 
			return MV_NOT_ALLOWED;
		}
	}
	return MV_OK;
}

static void mvCesaFragSizeFind(MV_CESA_SA *pSA, MV_CESA_REQ *pReq,
			       int cryptoOffset, int macOffset, int *pCopySize, int *pCryptoDataSize, int *pMacDataSize)
{
	MV_CESA_COMMAND *pCmd = pReq->pCmd;
	int cryptoDataSize, macDataSize, copySize;

	cryptoDataSize = macDataSize = 0;
	copySize = *pCopySize;

	if ((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_MAC_ONLY << MV_CESA_OPERATION_OFFSET)) {
		cryptoDataSize = MV_MIN((copySize - cryptoOffset), (pCmd->cryptoLength - (pReq->frags.cryptoSize + 1)));

		if (MV_IS_NOT_ALIGN(cryptoDataSize, pSA->cryptoBlockSize)) {
			cryptoDataSize = MV_ALIGN_DOWN(cryptoDataSize, pSA->cryptoBlockSize);
			copySize = cryptoOffset + cryptoDataSize;
		}
	}
	if ((pSA->config & MV_CESA_OPERATION_MASK) != (MV_CESA_CRYPTO_ONLY << MV_CESA_OPERATION_OFFSET)) {
		macDataSize = MV_MIN((copySize - macOffset), (pCmd->macLength - (pReq->frags.macSize + 1)));

		if (MV_IS_NOT_ALIGN(macDataSize, MV_CESA_AUTH_BLOCK_SIZE)) {
			macDataSize = MV_ALIGN_DOWN(macDataSize, MV_CESA_AUTH_BLOCK_SIZE);
			copySize = macOffset + macDataSize;
		}
		cryptoDataSize = copySize - cryptoOffset;
	}
	*pCopySize = copySize;

	if (pCryptoDataSize != NULL)
		*pCryptoDataSize = cryptoDataSize;

	if (pMacDataSize != NULL)
		*pMacDataSize = macDataSize;
}

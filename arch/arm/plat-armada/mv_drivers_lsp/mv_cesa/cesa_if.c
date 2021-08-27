#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"
#include "mvOs.h"
#include "cesa_if.h"
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/module.h>

#define MV_CESA_IF_MAX_WEIGHT	0xFFFFFFFF

static MV_CESA_RESULT **pResQ;
static MV_CESA_RESULT *resQ;
static MV_CESA_RESULT *pEmptyResult;
static MV_CESA_FLOW_TYPE flowType[MV_CESA_CHANNELS];
static MV_U32 chanWeight[MV_CESA_CHANNELS];
static MV_STATUS isReady[MV_CESA_CHANNELS];
static MV_CESA_POLICY cesaPolicy;
static MV_U8 splitChanId;
static MV_U32 resQueueDepth;
static MV_U32 reqId;
static MV_U32 resId;
static spinlock_t chanLock[MV_CESA_CHANNELS];
static DEFINE_SPINLOCK(cesaIfLock);
static DEFINE_SPINLOCK(cesaIsrLock);

MV_STATUS mvCesaIfInit(int numOfSession, int queueDepth, void *osHandle, MV_CESA_HAL_DATA *halData)
{
	MV_U8 chan = 0;

	reqId = 0;
	resId = 0;
	resQueueDepth = (MV_CESA_CHANNELS * queueDepth);

#if (CONFIG_MV_CESA_CHANNELS == 1)
	cesaPolicy = CESA_SINGLE_CHAN_POLICY;
#else
	cesaPolicy = CESA_DUAL_CHAN_BALANCED_POLICY;
#endif

	resQ = (MV_CESA_RESULT *)mvOsMalloc(resQueueDepth * sizeof(MV_CESA_RESULT));
	if (resQ == NULL) {
		mvOsPrintf("%s: Error, resQ malloc failed\n", __func__);
		return MV_ERROR;
	}
	pEmptyResult = &resQ[0];

	pResQ = (MV_CESA_RESULT **)mvOsMalloc(resQueueDepth * sizeof(MV_CESA_RESULT*));
	if (pResQ == NULL) {
		mvOsPrintf("%s: Error, pResQ malloc failed\n", __func__);
		return MV_ERROR;
	}

	spin_lock_init(&cesaIfLock);
	spin_lock_init(&cesaIsrLock);

	for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
		spin_lock_init(&chanLock[chan]);
		chanWeight[chan] = 0;
		flowType[chan] = 0;
		isReady[chan] = MV_TRUE;
	}

	memset(pResQ, 0, (resQueueDepth * sizeof(MV_CESA_RESULT *)));
	memset(resQ, 0, (resQueueDepth * sizeof(MV_CESA_RESULT)));

	return mvCesaHalInit(numOfSession, queueDepth, osHandle, halData);
}

MV_STATUS mvCesaIfAction(MV_CESA_COMMAND *pCmd)
{
	MV_U8 chan = 0, chanId = 0xff;
	MV_U32 min = MV_CESA_IF_MAX_WEIGHT;  
	MV_STATUS status;
	MV_ULONG flags = 0;

	switch (cesaPolicy) {
	case CESA_WEIGHTED_CHAN_POLICY:
	case CESA_NULL_POLICY:
		for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
			if ((cesaReqResources[chan] > 0) && (chanWeight[chan] < min)) {
				min = chanWeight[chan];
				chanId = chan;
			}
		}

		if (cesaReqResources[chanId] <= 1)
			return MV_NO_RESOURCE;

		spin_lock_irqsave(&chanLock[chanId], flags);
		chanWeight[chanId] += pCmd->pSrc->mbufSize;
		spin_unlock_irqrestore(&chanLock[chanId], flags);
		break;

	case CESA_DUAL_CHAN_BALANCED_POLICY:
		spin_lock(&cesaIfLock);
		chanId = (reqId % 2);
		spin_unlock(&cesaIfLock);

		if (cesaReqResources[chanId] <= 1)
			return MV_NO_RESOURCE;

		break;

	case CESA_FLOW_ASSOC_CHAN_POLICY:
		for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
			if (flowType[chan] == pCmd->flowType) {
				chanId = chan;
				break;
			}	
		}

		if(chanId == 0xff) {
			mvOsPrintf("%s: Error, policy was not set correctly\n", __func__);
			return MV_ERROR;
		}

		break;

	case CESA_SINGLE_CHAN_POLICY:
		spin_lock(&cesaIfLock);
		chanId = 0;
		spin_unlock(&cesaIfLock);

		if (cesaReqResources[chanId] <= 1)
			return MV_NO_RESOURCE;

		break;

	default:
		mvOsPrintf("%s: Error, policy not supported\n", __func__);
		return MV_ERROR;
	}

	if (pCmd->split != MV_CESA_SPLIT_NONE) {
		if (pCmd->split == MV_CESA_SPLIT_FIRST) {
			spin_lock(&cesaIfLock);
			splitChanId = chanId;
			spin_unlock(&cesaIfLock);
		} else	 
			chanId = splitChanId;
	}

	spin_lock(&cesaIfLock);
	pCmd->reqId = reqId;
	spin_unlock(&cesaIfLock);

	spin_lock_irqsave(&chanLock[chanId], flags);
	status = mvCesaAction(chanId, pCmd);

	if ((status == MV_OK) || (status == MV_NO_MORE))
		reqId = ((reqId + 1) % resQueueDepth);

	spin_unlock_irqrestore(&chanLock[chanId], flags);

	return status;
}
#if defined(CONFIG_SYNO_ARMADA_ARCH)
EXPORT_SYMBOL(mvCesaIfAction);
#endif

MV_STATUS mvCesaIfReadyGet(MV_U8 chan, MV_CESA_RESULT *pResult)
{
	MV_STATUS status;
	MV_CESA_RESULT *pCurrResult;
	MV_ULONG flags;

	if (chan >= MV_CESA_CHANNELS) {
		printk("%s: Error, bad channel index(%d)\n", __func__, chan);
		return MV_ERROR;
	}

	spin_lock_irqsave(&chanLock[chan], flags);

	if (isReady[chan] == MV_FALSE)
		goto out;

	while (1) {

		spin_lock(&cesaIsrLock);
		pCurrResult = pEmptyResult;
		spin_unlock(&cesaIsrLock);

		status = mvCesaReadyGet(chan, pCurrResult);

		if (status != MV_OK)
			break;

		spin_lock(&cesaIsrLock);
		pEmptyResult = ((pEmptyResult != &resQ[resQueueDepth-1]) ? (pEmptyResult + 1) : &resQ[0]);
		spin_unlock(&cesaIsrLock);

		switch (cesaPolicy) {
		case CESA_WEIGHTED_CHAN_POLICY:
		case CESA_NULL_POLICY:
			chanWeight[chan] -= pCurrResult->mbufSize;
			break;

		case CESA_FLOW_ASSOC_CHAN_POLICY:
			 
			break;

		case CESA_DUAL_CHAN_BALANCED_POLICY:
		case CESA_SINGLE_CHAN_POLICY:
			break;

		default:
			mvOsPrintf("%s: Error, policy not supported\n", __func__);
			return MV_ERROR;
		}

		if (pResQ[pCurrResult->reqId] != NULL)
			mvOsPrintf("%s: Warning, result entry not empty(reqId=%d, chan=%d, resId=%d)\n", __func__, pCurrResult->reqId, chan, resId);

		spin_lock(&cesaIsrLock);
		pResQ[pCurrResult->reqId] = pCurrResult;
		spin_unlock(&cesaIsrLock);

#ifdef CONFIG_MV_CESA_INT_PER_PACKET
		break;
#endif
	}

out:
	spin_lock(&cesaIsrLock);

	if (pResQ[resId] == NULL) {
		isReady[chan] = MV_TRUE;
		status = MV_NOT_READY;
	} else {
		 
		isReady[chan] = MV_FALSE;
		 
		pResult->retCode = pResQ[resId]->retCode;
		pResult->pReqPrv = pResQ[resId]->pReqPrv;
		pResult->sessionId = pResQ[resId]->sessionId;
		pResult->mbufSize = pResQ[resId]->mbufSize;
		pResult->reqId = pResQ[resId]->reqId;
		pResQ[resId] = NULL;
		resId = ((resId + 1) % resQueueDepth);
		status = MV_OK;
	}

	spin_unlock(&cesaIsrLock);

	
	spin_unlock_irqrestore(&chanLock[chan], flags);

	return status;
}
#if defined(CONFIG_SYNO_ARMADA_ARCH)
EXPORT_SYMBOL(mvCesaIfReadyGet);
#endif

MV_STATUS mvCesaIfPolicySet(MV_CESA_POLICY policy, MV_CESA_FLOW_TYPE flow)
{
	MV_U8 chan = 0;

	spin_lock(&cesaIfLock);

	if (cesaPolicy == CESA_NULL_POLICY) {
		cesaPolicy = policy;
	} else {
		 
		if (cesaPolicy != policy) {
			spin_unlock(&cesaIfLock);
			mvOsPrintf("%s: Error, can not support multiple policies\n", __func__);
			return MV_ERROR;
		}
	}

	if (policy == CESA_FLOW_ASSOC_CHAN_POLICY) {

		if (flow == CESA_NULL_FLOW_TYPE) {
			spin_unlock(&cesaIfLock);
			mvOsPrintf("%s: Error, bad policy configuration\n", __func__);
			return MV_ERROR;
		}

		for (chan = 0; chan < MV_CESA_CHANNELS; chan++) {
			if (flowType[chan] == CESA_NULL_FLOW_TYPE)
				flowType[chan] = flow;
		}

		if (chan == MV_CESA_CHANNELS) {
			spin_unlock(&cesaIfLock);
			mvOsPrintf("%s: Error, no empty entry is available\n", __func__);
			return MV_ERROR;
		}

	}

	spin_unlock(&cesaIfLock);

	return MV_OK;
}

MV_STATUS mvCesaIfPolicyGet(MV_CESA_POLICY *pCesaPolicy)
{
	*pCesaPolicy = cesaPolicy;

	return MV_OK;
}

MV_STATUS mvCesaIfTdmaWinInit(MV_U8 chan, MV_UNIT_WIN_INFO *addrWinMap)
{
	return mvCesaTdmaWinInit(chan, addrWinMap);
}

MV_STATUS mvCesaIfFinish(void)
{
	 
	mvOsFree(pResQ);
	mvOsFree(resQ);

	return mvCesaFinish();
}
#if defined(CONFIG_SYNO_ARMADA_ARCH)
EXPORT_SYMBOL(mvCesaIfFinish);
#endif

MV_STATUS mvCesaIfSessionOpen(MV_CESA_OPEN_SESSION *pSession, short *pSid)
{
	return mvCesaSessionOpen(pSession, pSid);
}
#if defined(CONFIG_SYNO_ARMADA_ARCH)
EXPORT_SYMBOL(mvCesaIfSessionOpen);
#endif

MV_STATUS mvCesaIfSessionClose(short sid)
{
	return mvCesaSessionClose(sid);
}
#if defined(CONFIG_SYNO_ARMADA_ARCH)
EXPORT_SYMBOL(mvCesaIfSessionClose);
#endif

MV_VOID mvCesaIfDebugMbuf(const char *str, MV_CESA_MBUF *pMbuf, int offset, int size)
{
	return mvCesaDebugMbuf(str, pMbuf, offset, size);
}

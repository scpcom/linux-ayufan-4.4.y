
/*
 * t23xrmRequest.c
 *
 * t23x resource manager inbound request processing module
 *
 * Copyright (c) 2007-2009 Freescale Semiconductor, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither the name of Freescale Semiconductor nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * 2.1.0   2009_05_04 sec - add SNOW-3G support
 *
 */




/** @file
 * Processes inbound requests for the resource manager, and also
 * handles the cancellation of a queued request
 */

#include <linux/mm.h>
#include "t23xrmInternal.h"



/**
 * Starts processing an array of descriptors from an exec message.
 * The array must be less than the fetch FIFO depth in this
 * implementation (unless event timers are implemented to ensure
 * feeding of more)
 *
 * @param ins - crypto core instance
 * @param *rq - exec message to initiate
 * @param ch  - channel to use
 */
void rmStartDesc(T2CoreInstance *t2blk, RMexecMessage *rq, int32_t ch)
{
    int32_t    dpdidx;
    T2DPD     *physdesc;

    t2blk->channelActvMsg[ch] = rq;

    /* get physical pointer to head of list */
    physdesc = (T2DPD *)virt_to_phys(rq->descHead);

    /* Feed each one to the fetch FIFO. List MUST be smaller than FIFO*/
    for (dpdidx = 0; dpdidx < rq->descCount; dpdidx++)
    {
        t2blk->regs->chn[ch].fetchFIFO = (uint32_t)physdesc;
        physdesc++;
        t2blk->chnDescCt[ch]++;
    }
}



/**
 * Queue a new request message
 *
 * @param *t2blk
 * @param *intfc
 * @param *execMsg
 * @param *entryID
 */
RMstatus xwcRMqueueRequest(RMinterfaceCtx *intfc,
                           RMexecMessage  *execMsg,
                           uint32_t       *entryID)
{
    int32_t         i, channel;
    T2DPD          *listHead, *thisDPD;
    uint32_t        priEU, secEU, descType;
    unsigned long   irqflags;
    T2CoreInstance *t2blk;

#ifdef RM_DBG_DPDVIEW
    int32_t j, k;
    linkEntry *thisEnt;
#endif

    t2blk = (T2CoreInstance *)intfc->devctx;

#ifdef RM_DBG_APITRACE
    printk("xwcRMqueueRequest(intfc=0x%08x, execMsg=0x%08x, entryID=0x%08x)\n",
          (uint32_t)intfc, (uint32_t)execMsg, (uint32_t)entryID);
#endif

    /* If bad IFctx, flag an error */
    if (intfc == NULL)
        return RM_BAD_INTFC_CTX;

    /* If null request, just return and act dumb */
    if (execMsg == NULL)
        return RM_OK;

    /* If FW version incompatible, quit */
    if (execMsg->frameworkID > 2)
        return RM_UNSUPPORTED_FW;

    /* Now check the vital stuff in the execution message */
    if ((execMsg->descHead == NULL) ||
            (execMsg->descCount == 0) ||
            (execMsg->messageReleaseHandler == NULL))
        return RM_BAD_EXEC_MESSAGE;

#ifdef RM_DBG_DPDVIEW

    printk("t23xrm:xwcRMqueueRequest() - new request received, "
           "%d deep\n", execMsg->descCount);

    listHead = (T2DPD *)execMsg->descHead;

    for (i = 0; i < execMsg->descCount; i++)
    {
        thisDPD = &listHead[i];

        printk("first desc at:   0x%08x (0x%08lx physical)\n", (u32)thisDPD, virt_to_phys(thisDPD));

        printk("desc %2d header:  0x%08x\n",
                i,
                thisDPD->hdr);

        printk("(pri:");

        switch((thisDPD->hdr >> EU_SHIFT_PRIMARY) & EU_SEL_MASK)
        {
            case EU_ARC4: printk("RC4"); break;
            case EU_DES:  printk("DES"); break;
            case EU_RND:  printk("RND"); break;
            case EU_PK:   printk("PK "); break;
            case EU_AES:  printk("AES"); break;
            case EU_KEA:  printk("KEA"); break;
            case EU_CRC:  printk("CRC"); break;
            case EU_SNOW: printk("SNO"); break;

            case EU_MD:
            case EU_MDPRIME:
                printk("MD "); break;

            case EU_NONE:
            default:
                printk("nul"); break;

        printk("/0x%02x sec:", (thisDPD->hdr >> EU_SHIFT_PRIMARY) & EU_MODE_MASK);

        switch((thisDPD->hdr >> EU_SHIFT_SECONDARY) & EU_SEL_MASK)
        {
            case EU_ARC4: printk("RC4"); break;
            case EU_DES:  printk("DES"); break;
            case EU_RND:  printk("RND"); break;
            case EU_PK:   printk("PK "); break;
            case EU_AES:  printk("AES"); break;
            case EU_KEA:  printk("KEA"); break;
            case EU_CRC:  printk("CRC"); break;
            case EU_SNOW: printk("SNO"); break;

            case EU_MD:
            case EU_MDPRIME:
                printk("MD "); break;

            case EU_NONE:
            default:
                printk("nul"); break;
        }
        printk("/0x%02x - ", (thisDPD->hdr >> EU_SHIFT_SECONDARY) & EU_MODE_MASK);

        switch(thisDPD->hdr & DESCTYPE_MASK)
        {
            case DESCTYPE_AES_CTR:         printk("aes-ctrmode  :"); break;
            case DESCTYPE_IPSEC_ESP:       printk("ipsec-ESP    :"); break;
            case DESCTYPE_COMMON:          printk("cipher-common:"); break;
            case DESCTYPE_AES_CCMP:        printk("aes-CCMP     :"); break;
            case DESCTYPE_HMAC:            printk("hmac         :"); break;
            case DESCTYPE_SRTP:            printk("srtp         :"); break;
            case DESCTYPE_PK_ECC_ASM:      printk("ecc-assemble :"); break;
            case DESCTYPE_PK_ECC_PTMULT:   printk("ecc-pt-mult  :"); break;
            case DESCTYPE_ARC4:            printk("arc4-stream  :"); break;
            case DESCTYPE_PK_ECC_PTADD_D:  printk("ecc-pt-add   :"); break;
            case DESCTYPE_PK_MONTY:        printk("pk-montgomery:"); break;
            case DESCTYPE_TLS_BLOCK:       printk("tls blkcphr  :"); break;
            case DESCTYPE_TLS_STREAM:      printk("TLS stmcphr  :"); break;
            case DESCTYPE_RAIDXOR:         printk("raid XOR     :"); break;
            case DESCTYPE_IPSEC_AES_GCM:   printk("ipsec aes/gcm:"); break;
            case DESCTYPE_AES_HMAC:        printk("aes-HMAC     :"); break;
            case DESCTYPE_DBLCRC:          printk("double-crc   :"); break;
            default:                       printk("unknown      :"); break;
        }

        if (thisDPD->hdr & HDR_INBOUND)
            printk("inbound:");
        else
            printk("outbound:");

        if (thisDPD->hdr & HDR_DONE)
            printk("done");
        else
            printk("none");

        printk(")\n");

        for (j = 0; j < TOTAL_PAIRS; j++)
        {
            if ((thisDPD->pair[j].size) ||
                (thisDPD->pair[j].ptr))
            {
                printk("desc %2d pair %1d: 0x%1x%08x: %5d (ext:%3d)",
                        i, j,
                        thisDPD->pair[j].eptr & EPTR_MASK,
                        (uint32_t)thisDPD->pair[j].ptr,
                        thisDPD->pair[j].size,
                        thisDPD->pair[j].extent & EXTENT_MASK);
                if (thisDPD->pair[j].extent & JUMPTABLE)
                {
                    printk(", scattered\n");
                    k = 0;
                    thisEnt = (linkEntry *)phys_to_virt((unsigned long)thisDPD->pair[j].ptr);
                    do
                    {
                        printk("          frag: 0x%1x%08x: %5d ",
                               thisEnt[k].extAddr & EPTR_MASK,
                               (uint32_t)thisEnt[k].segAddr,
                               thisEnt[k].segLen);
                        if (thisEnt[k].chainCtrl & NEXT_ENTRY)
                        {
                            printk("->");
                            thisEnt = (linkEntry *)phys_to_virt((unsigned long)thisEnt[k].segAddr);
                            k = 0;
                        }
                        printk("\n");
                    } while (!(thisEnt[k++].chainCtrl & LAST_ENTRY));
                }
                else
                    printk(", direct\n");
            }
        }
    }

#endif

    /* Now verify descriptors for supported EUs and modes */
    listHead = (T2DPD *)execMsg->descHead;
    for (i = 0; i < execMsg->descCount; i++)
    {
        thisDPD = &listHead[i];

        priEU    = ((thisDPD->hdr >> EU_SHIFT_PRIMARY) & EU_SEL_MASK) >> EU_SEL_SHIFT;
        secEU    = ((thisDPD->hdr >> EU_SHIFT_SECONDARY) & EU_SEL_MASK) >> EU_SEL_SHIFT;
        descType = (thisDPD->hdr & DESCTYPE_MASK) & DESCTYPE_SHIFT;


        /* Check primary and secondary EUs against those in the capability mask */
        if (!((1 << priEU) & t2blk->euPresent))
            return RM_NO_CAPABILITY;

        if (!((1 << secEU) & t2blk->euPresent))
            return RM_NO_CAPABILITY;

        /* Check against supported modes */
        if (!((1 << descType) & t2blk->validTypes))
            return RM_NO_CAPABILITY;
    }

    /* OK, if we got this far, we must have a good RQ message */
    /* Log the owning interface in this message */
    execMsg->owningIF = intfc;

    /* Count in in the statistics */
    t2blk->processedRQs++;

    /*
     * Lock the global request queue while we either choose a
     * channel if one is available, or queue the request if
     * all are busy.
     *
     * With this implementation, the exec queue lock is protecting
     * both the channel pool and the global request queue
     */

    spin_lock_irqsave(&t2blk->execQlock, irqflags);

    /* Now see if a channel is available */
    if (t2blk->freeChannels)
    {
        for (channel = 0; channel < t2blk->totalChannels; channel++)
            if (t2blk->channelState[channel] == CHstateFree)
            {
                t2blk->freeChannels--;
                t2blk->channelState[channel] = CHstateBusy;
                rmStartDesc(t2blk, execMsg, channel);
                break;
            }
    }
    else
    {
        /* Did we overrun the global request queue? */
        if ((t2blk->execQlevel + 1) >= EXEC_QUEUE_DEPTH)
        {
            spin_unlock_irqrestore(&t2blk->execQlock, irqflags);
            return RM_EXEC_QUEUE_FULL;
        }

        /* no, so push it on */
        t2blk->execQ[t2blk->execQtail] = execMsg;
        t2blk->execQtail = (t2blk->execQtail + 1) % EXEC_QUEUE_DEPTH;
        t2blk->execQlevel++;
        if (t2blk->execQlevel > t2blk->execQpeak)
            t2blk->execQpeak = t2blk->execQlevel;

    }

    spin_unlock_irqrestore(&t2blk->execQlock, irqflags);

    return RM_OK;
}




/**
 * Cancel a queued request
 * @param ins
 * @param *intfc
 * @param entryID
 * @return
 */
RMstatus xwcRMcancelRequest(RMinterfaceCtx *intfc,
                            uint32_t        entryID)
{
#ifdef RM_DBG_APITRACE
    T2CoreInstance *t2blk = (T2CoreInstance *)intfc->devctx;

    printk("xwcRMcancelRequest(inst=0x%08x,intfc=0x%08x,entryID=0x%08x)\n",
            (uint32_t)t2blk, (uint32_t)intfc, entryID);
#endif

    return RM_UNIMPLEMENTED;
}

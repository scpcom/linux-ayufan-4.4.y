
/*
 * t23xrmResponse.c
 *
 * t23x resource manager response processing module. This module
 * handles post-request processing on behalf of an interface
 *
 * Copyright (c) 2007-2010 Freescale Semiconductor, Inc.
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
 * 1.0.0   2008_01_29  sec - simpler queue locking implemented
 * 2.1.0   2009_05_04  sec - add SNOW-3G support
 */




/** @file
 * Handles post-request processing for the resource manager, including
 * the core of deferred-service processing to follow an interrupt (the
 * actual tasklet declaration is in t23xrmISR.c)
 */


#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include "t23xrmInternal.h"

#define free_channel(i) \
    spin_lock_irqsave(&thisDev->execQlock, irqflags);\
if (thisDev->channelState[i] == CHstateBusy) { \
    thisDev->channelActvMsg[i] = NULL; \
    thisDev->channelState[i] = CHstateFree; \
    thisDev->freeChannels++; } \
    spin_unlock_irqrestore(&thisDev->execQlock, irqflags);


extern void rmStartDesc(T2CoreInstance *t2blk, RMexecMessage *rq, int32_t ch);
extern RMinterfaceCtx ifCtx[];


/**
 * Deferred service responder to process messages off the ISR
 * queue
 * If expanded to multiple instances, "ins" will be the index
 * of the device instance state block
 * @param ins
 */
void t23RMdeferredInterruptResponder(int inst)
{
    int32_t         i;
    T2ISRCtx       *thisEvent;
    T2CoreInstance *thisDev;
    RMexecMessage  *thisMsg, *nextMsg;
    u32             errorDesc;
    int32_t         channel, burstsize, burst;
    unsigned long   irqflags;
#ifdef RM_DBG_EXTENDED_ERROR
    int32_t         j;
    int16_t         errbits;
#endif

    thisDev = ifCtx[inst].devctx;


    /* Spin through "do" for as many events on the ISR event queue */
    /* This means that the tasklet might get invoked with nothing to */
    /* do on rare occasions, so be it... */

    do
    {
        /* Mask interrupts, and pull ISR contexts off the ISR queue */
        /* If level == 0, nothing to do */
        disable_irq(thisDev->doneIRQid);
        if (!thisDev->isrQlevel)
        {
            enable_irq(thisDev->doneIRQid);
            return;
        }

        /* Grab an IRQ event context, decrement queue level, and unmask */
        thisEvent = &thisDev->t2isrQ[thisDev->isrQhead];
        thisDev->isrQhead = (thisDev->isrQhead + 1) % ISRMSG_QUEUE_DEPTH;
        thisDev->isrQlevel--;
        enable_irq(thisDev->doneIRQid);

#ifdef RM_DBG_EXTENDED_ERROR
        printk("t23xrm:t23RMdeferredInterruptResponder() - new ISR event - done 0x%04x - error 0x%04x\n",
                thisEvent->channelsDone, thisEvent->channelsInError);

        if (thisEvent->channelsDone)
            printk("channels done: ");
        for (j = 0; j < thisDev->totalChannels; j++)
            if (thisEvent->channelsDone & (INT_ACTIVE_CH0 << j))
                printk("%d ", j + 1);
        if (thisEvent->channelsDone)
            printk("\n");

        if (thisEvent->channelsInError)
            printk("channels in error: ");
        for (j = 0; j < thisDev->totalChannels; j++)
            if (thisEvent->channelsInError & (INT_ACTIVE_CH0 << j))
                printk("%d ", j + 1);
        if (thisEvent->channelsInError)
            printk("\n");
#endif

        /* For any "done" interrupt, go through all interrupting channels */
        /* and invoke handlers as spec'ed in each message, then free the  */
        /* channel if there is no static reservation                      */
        if (thisEvent->channelsDone)
            for (i = 0; i < thisDev->totalChannels; i++)
                if (thisEvent->channelsDone & (INT_ACTIVE_CH0 << i))
                {
                    thisMsg = thisDev->channelActvMsg[i];
                    /* error information is zeroed */
                    thisMsg->errState[0] = 0;
                    thisMsg->errState[1] = 0;
                    thisMsg->errState[2] = 0;
                    thisMsg->errState[3] = 0;
                    thisMsg->errDesc     = NULL;

                    if (thisMsg->descriptorDoneHandler != NULL)
                        thisMsg->descriptorDoneHandler(0);

                    thisMsg->messageReleaseHandler(thisMsg->releaseArgument);
                    if (thisMsg->waitingTask != NULL)
                        wake_up(thisMsg->waitingTask);

                    free_channel(i);
                }

        /* For any "error" interrupt, go through all interrupting channels     */
        /* capture the error status and a pointer to the error-ing descriptor, */
        /* then go invoke handlers if they exist, and free the channel         */
        if (thisEvent->channelsInError)
            for (i = 0; i < thisDev->totalChannels; i++)
                if (thisEvent->channelsInError & (INT_ACTIVE_CH0 << i))
                {
                    thisMsg = thisDev->channelActvMsg[i];
                    /* write the CPSR/errdesc to exec message status */
                    thisMsg->errState[0] = thisEvent->channelState[i];
                    thisMsg->errState[1] = 0;
                    thisMsg->errState[2] = thisEvent->priEUstate[i];
                    thisMsg->errState[3] = thisEvent->secEUstate[i];

                    errorDesc = (thisEvent->currentDesc[i] & 0x00000000ffffffffull);
                    thisMsg->errDesc = (T2DPD *)errorDesc;

                    if (thisMsg->descriptorErrorHandler != NULL)
                        thisMsg->descriptorErrorHandler(0);

                    thisMsg->messageReleaseHandler(thisMsg->releaseArgument);
                    if (thisMsg->waitingTask != NULL)
                        wake_up(thisMsg->waitingTask);

                    free_channel(i);

#ifdef RM_DBG_EXTENDED_ERROR
                    printk("Extended error report for channel %d:\n", i + 1);

                    printk("CPSR ");
                    printk("get:0x%02llx ", (thisMsg->errState[0] & T3_CPSR_GET_STATE_MASK) >> T3_CPSR_GET_STATE_SHIFT);
                    printk("put:0x%02llx ", (thisMsg->errState[0] & T3_CPSR_PUT_STATE_MASK) >> T3_CPSR_PUT_STATE_SHIFT);
                    printk("main:0x%03llx ", (thisMsg->errState[0] & T3_CPSR_MAIN_STATE_MASK) >> T3_CPSR_MAIN_STATE_SHIFT);
                    printk("ff_lvl:0x%02llx ", (thisMsg->errState[0] & T3_CPSR_FF_LEVEL_MASK) >> T3_CPSR_FF_LEVEL_SHIFT);
                    if (thisMsg->errState[0] & T3_CPSR_PRD) printk("prd ");
                    if (thisMsg->errState[0] & T3_CPSR_SRD) printk("srd ");
                    if (thisMsg->errState[0] & T3_CPSR_PD)  printk("pd ");
                    if (thisMsg->errState[0] & T3_CPSR_SD)  printk("sd ");
                    printk("\n");

                    printk("CPSR error bits: ");
                    errbits = thisMsg->errState[0] & T2_CPSR_ERROR_MASK;
                    if (errbits & T2_CHN_ERROR_DOF)            printk("DOF ");
                    if (errbits & T2_CHN_ERROR_SOF)            printk("SOF ");
                    if (errbits & T2_CHN_ERROR_MDTE)           printk("MDTE ");
                    if (errbits & T2_CHN_ERROR_SG_ZERO_LEN)    printk("SGZERO ");
                    if (errbits & T2_CHN_ERROR_FP_ZERO)        printk("FPZERO ");
                    if (errbits & T2_CHN_ERROR_ILLEGAL_HEADER) printk("ILLHDR ");
                    if (errbits & T2_CHN_ERROR_INVALID_EU)     printk("INVEU ");
                    if (errbits & T2_CHN_ERROR_EU_ERROR)       printk("EUERR ");
                    if (errbits & T2_CHN_ERROR_G_BOUNDARY)     printk("GBDRY ");
                    if (errbits & T2_CHN_ERROR_G_LENGTH)       printk("GLEN ");
                    if (errbits & T2_CHN_ERROR_S_BOUNDARY)     printk("SBDRY ");
                    if (errbits & T2_CHN_ERROR_S_LENGTH)       printk("SLEN ");
                    printk("\n");

                    printk("fetched descriptor as read into channel:\n");
                    printk("header: 0x%08x\n", thisEvent->fetchedDesc[i].dpd.hdr);
                    for (j = 0; j < TOTAL_PAIRS; j++)
                    {
                        if (thisEvent->fetchedDesc[i].dpd.pair[j].eptr ||
                            thisEvent->fetchedDesc[i].dpd.pair[j].ptr  ||
                            thisEvent->fetchedDesc[i].dpd.pair[j].size ||
                            thisEvent->fetchedDesc[i].dpd.pair[j].extent)
                        {
                            printk("pair %d: 0x%02x%08x: %5d (ext:%3d)",
                                   j,
                                   thisEvent->fetchedDesc[i].dpd.pair[j].eptr,
                                   (u32)thisEvent->fetchedDesc[i].dpd.pair[j].ptr,
                                   thisEvent->fetchedDesc[i].dpd.pair[j].size,
                                   (thisEvent->fetchedDesc[i].dpd.pair[j].extent & EXTENT_MASK));
                            if (thisEvent->fetchedDesc[i].dpd.pair[j].extent & JUMPTABLE)
                                printk(", scattered\n");
                            else
                                printk("\n");
                        }
                    }

                    for (j = 0; j < 4; j++)
                    {
                        if (thisEvent->fetchedDesc[i].glt[j].segLen    ||
                            thisEvent->fetchedDesc[i].glt[j].chainCtrl ||
                            thisEvent->fetchedDesc[i].glt[j].extAddr   ||
                            thisEvent->fetchedDesc[i].glt[j].segAddr)
                        {
                            printk("gthr %d: 0x%02x%08x: %5d ",
                            j,
                            thisEvent->fetchedDesc[i].glt[j].extAddr,
                            (u32)thisEvent->fetchedDesc[i].glt[j].segAddr,
                            thisEvent->fetchedDesc[i].glt[j].segLen);

                            if (thisEvent->fetchedDesc[i].glt[j].chainCtrl & LAST_ENTRY)
                                printk("end ");
                            if (thisEvent->fetchedDesc[i].glt[j].chainCtrl & NEXT_ENTRY)
                                printk("next ");

                            printk("\n");
                        }
                    }

                    for (j = 0; j < 4; j++)
                    {
                        if (thisEvent->fetchedDesc[i].slt[j].segLen    ||
                            thisEvent->fetchedDesc[i].slt[j].chainCtrl ||
                            thisEvent->fetchedDesc[i].slt[j].extAddr   ||
                            thisEvent->fetchedDesc[i].slt[j].segAddr)
                        {
                            printk("sctr %d: 0x%02x%08x: %5d ",
                            j,
                            thisEvent->fetchedDesc[i].slt[j].extAddr,
                            (u32)thisEvent->fetchedDesc[i].slt[j].segAddr,
                            thisEvent->fetchedDesc[i].slt[j].segLen);

                            if (thisEvent->fetchedDesc[i].slt[j].chainCtrl & LAST_ENTRY)
                                printk("end ");
                            if (thisEvent->fetchedDesc[i].slt[j].chainCtrl & NEXT_ENTRY)
                                printk("next ");

                            printk("\n");
                        }
                    }

                    printk("channel %d pri EU intstatus = 0x%016llx\n",
                           i,
                           thisEvent->priEUstate[i]);

                    printk("channel %d sec EU intstatus = 0x%016llx\n",
                           i,
                           thisEvent->secEUstate[i]);
#endif
                }


        /* Once we get down here, we should have some free channels */
        /* If there's stuff on the exec queue, start as many        */
        /* entries off the queue as possible                        */
        spin_lock_irqsave(&thisDev->execQlock, irqflags);
        if ((thisDev->execQlevel) &&
            (thisDev->freeChannels))
        {

            /* OK, we have at least one in the request queue, and */
            /* at least one free channel. Channel allocation and  */
            /* the request queue are both locked. Based on this,  */
            /* how many can we launch?                            */
            if (thisDev->execQlevel < thisDev->freeChannels)
                burstsize = thisDev->execQlevel;
            else
                burstsize = thisDev->freeChannels;

            burst = burstsize;

            /* loop through each possible message in this launch burst */
            while (burstsize)
            {
                /* Don't know which channels are free, so find one */
                for (channel = 0; channel < thisDev->totalChannels; channel++)
                    if (thisDev->channelState[channel] == CHstateFree)
                    {
                        /* This one is free, reserve it */
                        thisDev->freeChannels--;
                        thisDev->channelState[channel] = CHstateBusy;
                        break;
                    }

                /* Got an allocated channel. Pop one entry */
                nextMsg = thisDev->execQ[thisDev->execQhead];
                thisDev->execQhead =
                    (thisDev->execQhead + 1) % EXEC_QUEUE_DEPTH;
                thisDev->execQlevel--;

                /* launch it */
                if ((uint32_t)nextMsg <= 0x00001000)
                    panic("t23xrm:t23RMdeferredInterruptResponder() new msg 0x%08x channel %d execQlevel %d head %d tail %d, isrQ %d, burst %d, pending 0x%04x\n",
                          (uint32_t)nextMsg, channel, thisDev->execQlevel, thisDev->execQhead, thisDev->execQtail, thisDev->isrQlevel, burst, thisEvent->channelsDone);
                rmStartDesc(thisDev, nextMsg, channel);

                burstsize--;
            }
        }
        spin_unlock_irqrestore(&thisDev->execQlock, irqflags);

    } while (thisDev->isrQlevel);
}

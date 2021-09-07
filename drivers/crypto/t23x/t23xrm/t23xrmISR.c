
/*
 * t23xrmISR
 *
 * t23x interrupt service module
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
 * 2.1.0   2009_05_04 sec - remove overflow handler
 */



/** @file
 * Handles interrupt processing for the Talitos 2/3 resource manager,
 * including definition of the deferred-service tasklet, and it's
 * initiation.
 */

#include <linux/interrupt.h>

#include "../common/t23.h"
#include "t23xrmInternal.h"



/**
 * deferred Interrupt routine
 * @param unused
 */
void t23RMdeferredInterruptResponder(unsigned long unused);



DECLARE_TASKLET(t23xrmPostInt0,
                t23RMdeferredInterruptResponder,
		0);


/* Extract EU status registeres from a selector derived from a */
/* descriptor header                                           */

static u64 euGetState(u32              selector,
                      T2CORE          *dev)
{
    switch (selector)
    {
        case EU_ARC4:
            return dev->euARC4.interruptStatus;

        case EU_DES:
            return dev->euDES.interruptStatus;

        case EU_MD:
        case EU_MDPRIME:
            return dev->euMD.interruptStatus;

        case EU_RND:
            return dev->euRND.interruptStatus;

        case EU_PK:
            return dev->euPK.interruptStatus;

        case EU_AES:
            return dev->euAES.interruptStatus;

        case EU_KEA:
            return dev->euKEA.interruptStatus;

        case EU_CRC:
            return dev->euCRC.interruptStatus;

        default:
        case EU_NONE:
            return 0;
    }
}


/**
 * Interrupt Handler
 * @param irq
 * @param devInstBlock
 * @param *regs
 * @return
 */
irqreturn_t t23RMintDoneHandler(int32_t         irq,
                                void           *devInstBlock)
{
    T2CoreInstance *t2blk;
    u64             intBits, tmp;
    T2ISRCtx       *thisEntry;
    int32_t         i;
    u32             priSel, secSel;

    t2blk = devInstBlock;

    /* Get a copy of the current pending interrupt bits */
    intBits = t2blk->regs->ctrl.intStatus;

    /* None set, then we didn't cause it, say so in the return */
    if (!intBits)
    {
        panic("t23xrm: no interrupt status detected\n");
        return IRQ_NONE;
    }

    /* We've caused something, don't know what yet. Grab an ISR queue */
    /* entry to store the context in.                                 */

    if ((t2blk->isrQlevel + 1) >= ISRMSG_QUEUE_DEPTH)
        panic("t23xrm: ISR message queue overflow\n");

    thisEntry = &t2blk->t2isrQ[t2blk->isrQtail];
    t2blk->isrQtail = (t2blk->isrQtail + 1) % ISRMSG_QUEUE_DEPTH;
    t2blk->isrQlevel++;
    if (t2blk->isrQlevel > t2blk->isrQpeak)
        t2blk->isrQpeak = t2blk->isrQlevel;

    /* clear done/error data before OR of done/error pending bits */
    thisEntry->channelsDone    = 0;
    thisEntry->channelsInError = 0;

    /* Capture CPSR/CPDR, and done/error state in queue entry */
    for (i = 0; i < t2blk->totalChannels; i++)
    {
        thisEntry->channelState[i] = t2blk->regs->chn[i].pointerStatus;
        thisEntry->currentDesc[i]  = t2blk->regs->chn[i].currentDesc;

        if (intBits & (T2_IMR_DONE_CH0 << (i * T2_IMR_DONE_STEP)))
            thisEntry->channelsDone |= (INT_ACTIVE_CH0 << i);

        if (intBits & (T2_IMR_ERROR_CH0 << (i * T2_IMR_ERROR_STEP)))
        {
            /* Build a mask of the channels with an error pending */
            thisEntry->channelsInError |= (INT_ACTIVE_CH0 << i);

            /* Snapshot EU status for later analysis */
            /* First, extract EU selection from the failed descriptor */
            priSel = (((T2DPD *)&t2blk->regs->chn[i].descBuffer)->hdr
                     >> EU_SHIFT_PRIMARY) & EU_SEL_MASK;
            secSel = (((T2DPD *)&t2blk->regs->chn[i].descBuffer)->hdr
                     >> EU_SHIFT_SECONDARY) & EU_SEL_MASK;

            /* Second, translate/extract relevant EU status registers */
            thisEntry->priEUstate[i] = euGetState(priSel, t2blk->regs);

            if (secSel)
                thisEntry->secEUstate[i] = euGetState(secSel, t2blk->regs);
            else
                thisEntry->secEUstate[i] = 0;

#ifdef RM_DBG_EXTENDED_ERROR

            /* just a blind copy of the descriptor buffer, and the */
            /* SG link tables, all in one big copy                 */
            memcpy(&thisEntry->fetchedDesc[i],
                   &t2blk->regs->chn[i].descBuffer,
                   sizeof(struct internal_descinfo));
#endif

            /* Now that we've identified an error-ing channel,       */
            /* and have captured it's state, reset it, let the reset */
            /* latch, and restore it's saved configuration           */
            t2blk->regs->chn[i].config = T2_CCR_RESET;
            tmp = t2blk->regs->chn[i].config; /* allow reset to cycle */
            t2blk->regs->chn[i].config = t2blk->channelConfig[i];
        }

    }


    tasklet_schedule(&t23xrmPostInt0); /* kick off deferred service  */

    /* Must make sure interrupt clear register is   */
    /* written once identified                      */
    t2blk->regs->ctrl.intClear = intBits;

    return IRQ_HANDLED;
}

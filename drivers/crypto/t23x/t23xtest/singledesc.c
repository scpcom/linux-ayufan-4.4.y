/*
 * singledesc.c
 *
 * t23x RM testing module - single descriptor cases
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
 * 2.1.0   2009_05_04 sec - updated from simplified registration
 */




/** @file
 *  Test using single descriptor
 */


#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/interrupt.h>

#include "../common/t23.h"
#include "../common/xwcRMinterface.h"
#include "rmtest.h"

/* This is the data set we can use for a simple SHA1 request */
static const uint8_t sha1padplaintext1[] = "abc";
static const uint8_t sha1paddigest1[] =
{
    0xa9, 0x99, 0x3e, 0x36, 0x47,
    0x06, 0x81, 0x6a, 0xba, 0x3e,
    0x25, 0x71, 0x78, 0x50, 0xc2,
    0x6c, 0x9c, 0xd0, 0xd8, 0x9d
};



static void intRelease(struct semaphore *lock)
{
    up(lock);
}



int32_t testOneDPD_withInterruptSHA(RMinterfaceCtx *ifCtx)
{
    T2DPD           *thisdpd;
    RMexecMessage    execMsg;
    uint8_t         *plaintext, *digest;
    RMstatus         rmstatus;
    uint32_t         entryID;
    struct semaphore intlock;

    memset(&execMsg, 0, sizeof(execMsg));
    init_MUTEX_LOCKED(&intlock);

    thisdpd   = getDPD(1);
    plaintext = getInBuffer((uint8_t *)sha1padplaintext1, 3);
    digest    = getOutBuffer(20);

    execMsg.frameworkID           = XWC_FRAMEWORK_CURRENT_VERSION;
    execMsg.descHead              = thisdpd;
    execMsg.descCount             = 1;
    execMsg.messageReleaseHandler = (void (*)(void *))intRelease;
    execMsg.releaseArgument       = (void *)&intlock;

    if ((thisdpd == NULL) ||
        (plaintext == NULL) ||
        (digest == NULL))
    {
        freeBuf(thisdpd);
        freeBuf(plaintext);
        freeBuf(digest);
        dbgmsg("testOneDPD_withInterruptSHA(): no memory\n");
        return -1;
    }

    thisdpd->hdr = ((EU_MD | MD_SHA1 | MD_INIT | MD_PD) << EU_SHIFT_PRIMARY) |
                    DESCTYPE_COMMON | HDR_OUTBOUND | HDR_DONE;
    setDPDfield(thisdpd, 3, plaintext, 3, 0, 0);
    setDPDfield(thisdpd, 5, digest,   20, 0, 0);


    /*
     * Step #2 - Pass the DPD to the RM for handling
     */

    rmstatus = xwcRMqueueRequest(ifCtx, &execMsg, &entryID);

    if (rmstatus != RM_OK)
    {
        dbgmsg("testOneDPD_withInterruptSHA(): error queueing request, error %d\n", rmstatus);
        freeBuf(thisdpd);
        freeBuf(plaintext);
        freeBuf(digest);
        return -1;
    }

    if (down_interruptible(&intlock))
    {
        dbgmsg("testOneDPD_withInterruptSHA(): interrupted\n");
        freeBuf(thisdpd);
        freeBuf(plaintext);
        freeBuf(digest);
        return -1;
    }


    /*
     * Step #3 - Verify the result
     */

    if (!memcmp(digest, sha1paddigest1, 20))
        dbgmsg("testOneDPD_withInterruptSHA(): OK\n");
    else
    {
        dbgmsg("testOneDPD_withInterruptSHA(): processed digest is INCORRECT\n");
        bufferDump(digest, 20);
    }


    freeBuf(thisdpd);
    freeBuf(plaintext);
    freeBuf(digest);
    return 0;
}


#define BITSINABYTE (8)

int32_t rndbitwt(uint8_t *buf, int32_t size)
{
    uint32_t  totalbits, actualbits;
    uint32_t  i, j;

    totalbits = size * BITSINABYTE;
    actualbits = 0;

    for (i = 0; i < size; i++)
        for (j = 0; j < BITSINABYTE; j++)
            if ((buf[i] >> j) & 0x01)
                actualbits++;
    return((actualbits * 100) / totalbits);
}

#define RNDSIZE 2048

int32_t testOneDPD_withInterruptRND(RMinterfaceCtx *ifCtx)
{
    T2DPD           *thisdpd;
    RMexecMessage    execMsg;
    uint8_t         *databuf;
    RMstatus         rmstatus;
    uint32_t         entryID;
    int32_t          bitwt;
    linkEntry       *sglist;
    struct semaphore intlock;

    memset(&execMsg, 0, sizeof(execMsg));
    init_MUTEX_LOCKED(&intlock);

    thisdpd   = getDPD(1);
    sglist    = getSGlist(1);
    databuf = (uint8_t *)kmalloc(RNDSIZE, GFP_DMA | GFP_KERNEL);

    execMsg.frameworkID           = XWC_FRAMEWORK_CURRENT_VERSION;
    execMsg.descHead              = thisdpd;
    execMsg.descCount             = 1;
    execMsg.messageReleaseHandler = (void (*)(void *))intRelease;
    execMsg.releaseArgument       = (void *)&intlock;

    if ((thisdpd == NULL) ||
        (databuf == NULL) ||
        (sglist == NULL))
    {
        freeBuf(thisdpd);
        freeBuf(sglist);
        kfree(databuf);
        dbgmsg("testOneDPD_withInterruptRND(): no memory\n");
        return -1;
    }

    thisdpd->hdr      = (EU_RND << EU_SHIFT_PRIMARY) | DESCTYPE_COMMON | HDR_DONE;
    sglist->segLen    = RNDSIZE;
    sglist->chainCtrl = LAST_ENTRY;
    sglist->segAddr   = (uint8_t *)__pa(databuf);

    setDPDfield(thisdpd, 4, (uint8_t *)sglist, RNDSIZE, 0, 1);


    rmstatus = xwcRMqueueRequest(ifCtx, &execMsg, &entryID);

    if (rmstatus != RM_OK)
    {
        if (rmstatus == RM_NO_CAPABILITY)
            dbgmsg("testOneDPD_withInterruptRND(): random generator not present\n");
        else
            dbgmsg("testOneDPD_withInterruptRND(): error queueing request, error %d\n", rmstatus);

        freeBuf(thisdpd);
        freeBuf(sglist);
        kfree(databuf);
        return -1;
    }

    if (down_interruptible(&intlock))
    {
        dbgmsg("testOneDPD_withInterruptRND(): interrupted\n");
        freeBuf(thisdpd);
        kfree(databuf);
        freeBuf(sglist);
        return -1;
    }

    bitwt = rndbitwt(databuf, RNDSIZE);

    if ((bitwt < 46) || (bitwt > 54))
    {
        dbgmsg("testOneDPD_withInterruptRND(): failed - bit weight = %d%\n", bitwt);
        bufferDump(databuf, 64);
    }
    else
        dbgmsg("testOneDPD_withInterruptRND(): OK\n");


    freeBuf(thisdpd);
    kfree(databuf);
    freeBuf(sglist);
    return 0;
}

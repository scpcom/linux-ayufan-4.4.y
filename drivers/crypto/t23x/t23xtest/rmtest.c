/*
 * rmtest.c
 *
 * t23x RM test - base functionality
 *
 * This module runs a short series of crypto operations against
 * most "any" hardware in the SEC 2.x/3.x family. It serves
 * as a quick checkout of the RM, and acts as a very primitive
 * registered interface, useful as a limited example.
 *
 * Copyright (c) 2007, 2008 Freescale Semiconductor, Inc.
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
 */




/** @file
 * Test module meant to exercise a resource manager's functionality
 */



#include <linux/slab.h>

#include "../common/xwcRMinterface.h"
#include "../common/t23.h"
#include "../t23xrm/t23xrmInternal.h"
#include "rmtest.h"




int (*dbgmsg)(const char *fmt, ...);
RMinterfaceCtx *ifctx;


int32_t execTestSeries(void)
{
    RMstatus        rmstatus;
    int32_t         teststatus;


    dbgmsg = printk;

    dbgmsg("Extensible Crypto Driver - t23x RM testing module - pkg rev %s\n", T23X_PACKAGE_VERSION);


    /*
     * Step #1 - Register thyself as an interface
     */

    rmstatus = xwcRMregisterInterface("t23xtest", &ifctx);
    if (rmstatus != RM_OK)
    {
        dbgmsg("t23xtest: can't register interface, error %d\n", rmstatus);
        return -1;
    }

    /*
     * Step #2 - walk through individual tests
     */
    teststatus = 0;

    if (testOneDPD_withInterruptSHA(ifctx))
        teststatus++;

    if (testOneDPD_withInterruptRND(ifctx))
        teststatus++;

    if (testMultiDPD_withInterrupt(ifctx))
        teststatus++;


    /*
     * Step #3 - Deregister and exit
     */
    rmstatus = xwcRMderegisterInterface(ifctx);
    if (rmstatus != RM_OK)
    {
        dbgmsg("t23xtest: can't deregister interface, error %d\n", rmstatus);
        return -1;
    }

    return 0;
}


/* Helper functions for all tests to use */

/* Allocate 1-n DPDs */
T2DPD *getDPD(int32_t count)
{
    T2DPD *desc;

    desc = (T2DPD *)kmalloc((sizeof(T2DPD) * count), GFP_DMA | GFP_KERNEL);
    if (desc == NULL)
        return (T2DPD *)NULL;

    memset(desc, 0, sizeof(T2DPD) * count);

    return desc;
}

/* Free a buffer */
void freeBuf(void *stuff)
{
    kfree(stuff);
}

/* Get an input buffer, copy data to it */
uint8_t *getInBuffer(uint8_t *content, int32_t size)
{
    uint8_t *buf;

    buf = (uint8_t *)kmalloc(size, GFP_DMA | GFP_KERNEL);
    if (buf == NULL)
        return (uint8_t *)NULL;

    memcpy(buf, content, size);

    return (uint8_t *)buf;
}

/* Get an output buffer */
uint8_t *getOutBuffer(int32_t size)
{
    uint8_t *buf;

    buf = (uint8_t *)kmalloc(size, GFP_DMA | GFP_KERNEL);
    memset(buf, 0, size);

    return (uint8_t *)buf;
}


linkEntry *getSGlist(int32_t entries)
{
    linkEntry *list;

    list = (linkEntry *)kmalloc(sizeof(linkEntry) * entries,
            GFP_DMA | GFP_KERNEL);
    memset(list, 0, sizeof(linkEntry) * entries);
    return list;
}



/* Write a DPD "pointer field" with content */
void setDPDfield(   T2DPD           *desc,
                    int32_t          field,
                    uint8_t         *ptr,
                    uint16_t         size,
                    uint8_t          extent,
                    uint8_t          jump)
{
    desc->pair[field].size   = size;
    desc->pair[field].extent = extent & EXTENT_MASK;
    desc->pair[field].ptr    = (void *)__pa(ptr);
    desc->pair[field].eptr   = 0; /* not used here */
    if (jump)
        desc->pair[field].extent |= JUMPTABLE;
}


/* Get a physical address from a buffer */
void *physAddr(void *vaddr)
{
    return (void *)__pa(vaddr);
}


#define DUMP_LINE_SIZE (8)

void bufferDump(uint8_t *buf, int32_t size)
{
    uint8_t *next;
    int32_t  remain, linesz, i;
    uint8_t  ascout[DUMP_LINE_SIZE + 1];

    next = buf;

    remain = size;

    while(remain)
    {
        memset(ascout, 0, DUMP_LINE_SIZE + 1); /* +1 for eol terminator */

        dbgmsg("0x%08x: ", (uint32_t)next);

        if (remain <= DUMP_LINE_SIZE)
            linesz = remain;
        else
            linesz = DUMP_LINE_SIZE;

        for (i = 0; i < linesz; i++)
        {
            dbgmsg("%02x ", *next);

            if ((*next >= 32) && (*next <= 126))
                ascout[i] = *next;
            else
                ascout[i] = '.';

            next++;
        }

        if (linesz < DUMP_LINE_SIZE)
            for (i = DUMP_LINE_SIZE - linesz; i; i--)
                dbgmsg("   ");

        dbgmsg("%s\n", ascout);

        remain -= linesz;

    }




}

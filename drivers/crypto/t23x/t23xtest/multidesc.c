/*
 * multidesc.c
 *
 * t23x RM testing module - multi-descriptor requests
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
 *  Test using multiple descriptors
 */

#include <linux/mm.h>
#include <linux/vmalloc.h>

#include "../common/t23.h"
#include "../common/xwcRMinterface.h"
#include "rmtest.h"

/* SHA1 data */
static const uint8_t sha1plaintext[] =
"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";

static const uint8_t sha1digest[] =
{
    0x84, 0x98, 0x3e, 0x44, 0x1c,
    0x3b, 0xd2, 0x6e, 0xba, 0xae,
    0x4a, 0xa1, 0xf9, 0x51, 0x29,
    0xe5, 0xe5, 0x46, 0x70, 0xf1
};

/** AES ECB Data */
static const uint8_t AESkey[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87
};

static const uint8_t AESdata[] =
{"Now is the time for all good men"}; /* 32 bytes */

static const uint8_t AEScipher[] = {
    0xf0, 0xd1, 0xad, 0x6f, 0x90, 0x1f, 0xff, 0xae,
    0x55, 0x72, 0xa6, 0x92, 0x8d, 0xab, 0x52, 0xb0,
    0x64, 0xb2, 0x5c, 0x79, 0xf8, 0x76, 0x73, 0x03,
    0x21, 0xe3, 0x6d, 0xc0, 0x10, 0x11, 0xac, 0xce
};

/* TDES Data */
static const uint8_t DESdata[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01,
    0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12
};

static const uint8_t DESkey[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
};

static const uint8_t DEScipher[] = {
    0xf6, 0x53, 0xf7, 0x0a, 0x75, 0xfe, 0x67, 0x8a,
    0xbf, 0xbf, 0x24, 0x6b, 0x4a, 0xdc, 0xf2, 0xf8,
    0x16, 0x92, 0x9c, 0x44, 0xf0, 0x26, 0xe1, 0x12,
    0x81, 0xf0, 0x5f, 0x4a, 0x56, 0x8e, 0xcc, 0x48
};


static void testMultiIntRelease(struct semaphore *lock)
{
    up(lock);
}



int32_t testMultiDPD_withInterrupt(RMinterfaceCtx *ifCtx)
{
    int32_t           testfail;
    T2DPD            *dpdlist;
    RMexecMessage     execMsg;
    uint8_t          *hashplain, *digest;
    uint8_t          *desplain, *descipher, *deskey;
    uint8_t          *aesplain, *aescipher, *aeskey;
    RMstatus          rmstatus;
    uint32_t          entryID;
    struct semaphore  irqlock;

    memset(&execMsg, 0, sizeof(execMsg));
    init_MUTEX_LOCKED(&irqlock);

    dpdlist = getDPD(3);
    if (dpdlist == NULL)
    {
        dbgmsg("testMultiDPD_withInterrupt(): no memory\n");
        return -1;
    }

    execMsg.frameworkID           = XWC_FRAMEWORK_CURRENT_VERSION;
    execMsg.descHead              = dpdlist;
    execMsg.descCount             = 3;
    execMsg.messageReleaseHandler = (void (*)(void *))testMultiIntRelease;
    execMsg.releaseArgument       = (void *)&irqlock;

    /* Set up the hash request */
    hashplain = getInBuffer((uint8_t *)sha1plaintext, 56);
    digest    = getOutBuffer(20);

    if ((hashplain == NULL) ||
        (digest == NULL))
    {
        freeBuf(dpdlist);
        freeBuf(hashplain);
        freeBuf(digest);
        dbgmsg("testMultiDPD_withInterrupt(): no memory\n");
        return -1;
    }

    (&dpdlist[0])->hdr = ((EU_MD | MD_SHA1 | MD_INIT | MD_PD)
                          << EU_SHIFT_PRIMARY) |
                          DESCTYPE_COMMON | HDR_OUTBOUND; /* no DONE */
    setDPDfield(&dpdlist[0], 3, hashplain, 56, 0, 0);
    setDPDfield(&dpdlist[0], 5, digest,    20, 0, 0);


    /* Set up DES request */
    desplain  = getInBuffer((uint8_t *)DESdata, 32);
    deskey    = getInBuffer((uint8_t *)DESkey,  24);
    descipher = getOutBuffer(32);

    if ((desplain  == NULL) ||
        (deskey    == NULL) ||
        (descipher == NULL))
    {
        freeBuf(dpdlist);
        freeBuf(hashplain);
        freeBuf(digest);
        freeBuf(desplain);
        freeBuf(deskey);
        freeBuf(descipher);
        dbgmsg("testMultiDPD_withInterrupt(): no memory\n");
        return -1;
    }

    /* TDES ECB Encrypt = 0x20700010 */
    /* Key in [2], input in [3], output in [4] */
    (&dpdlist[1])->hdr = ((EU_DES | DES_ENCRYPT | DES_ECB | DES_TRIPLE)
                          << EU_SHIFT_PRIMARY) |
                          DESCTYPE_COMMON | HDR_OUTBOUND; /* no DONE */
    setDPDfield(&dpdlist[1], 2, deskey,    24, 0, 0);
    setDPDfield(&dpdlist[1], 3, desplain,  32, 0, 0);
    setDPDfield(&dpdlist[1], 4, descipher, 32, 0, 0);

    /* AES ECB Encrypt = 0x60100010 */
    /* inIV [1], key [2], input [3], output [4], outIV [5] */
    aesplain  = getInBuffer((uint8_t *)AESdata, 32);
    aeskey    = getInBuffer((uint8_t *)AESkey,   16);
    aescipher = getOutBuffer(32);

    if ((aesplain  == NULL) ||
        (aeskey    == NULL) ||
        (aescipher == NULL))
    {
        freeBuf(dpdlist);
        freeBuf(hashplain);
        freeBuf(digest);
        freeBuf(desplain);
        freeBuf(deskey);
        freeBuf(descipher);
        freeBuf(aesplain);
        freeBuf(aeskey);
        freeBuf(aescipher);
        dbgmsg("testMultiDPD_withInterrupt(): no memory\n");
        return -1;
    }

    (&dpdlist[2])->hdr = ((EU_AES | AES_ENCRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
                          DESCTYPE_COMMON | HDR_OUTBOUND | HDR_DONE;
    setDPDfield(&dpdlist[2], 2, aeskey,    16, 0, 0);
    setDPDfield(&dpdlist[2], 3, aesplain,  32, 0, 0);
    setDPDfield(&dpdlist[2], 4, aescipher, 32, 0, 0);


    /*
     * Step #2 - Pass the DPD to the RM for handling
     */

    rmstatus = xwcRMqueueRequest(ifCtx, &execMsg, &entryID);
    if (rmstatus != RM_OK)
    {
        dbgmsg("testMultiDPD_withInterrupt(): error queueing request, error %d\n", rmstatus);
        freeBuf(dpdlist);
        freeBuf(hashplain);
        freeBuf(digest);
        freeBuf(desplain);
        freeBuf(deskey);
        freeBuf(descipher);
        freeBuf(aesplain);
        freeBuf(aeskey);
        freeBuf(aescipher);
        return -1;
    }

    if (down_interruptible(&irqlock))
    {
        dbgmsg("testMultiDPD_withInterrupt(): interrupted\n");
        freeBuf(dpdlist);
        freeBuf(hashplain);
        freeBuf(digest);
        freeBuf(desplain);
        freeBuf(deskey);
        freeBuf(descipher);
        freeBuf(aesplain);
        freeBuf(aeskey);
        freeBuf(aescipher);
        return -1;
    }



    /*
     * Step #3 - Verify the result
     */
    testfail = 0;

    if (memcmp(digest, sha1digest, 20))
    {
        dbgmsg("testMultiDPD_withInterrupt(): hash digest is INCORRECT\n");
        bufferDump(digest, 20);
        testfail++;
    }

    if (memcmp(descipher, DEScipher, 32))
    {
        dbgmsg("testMultiDPD_withInterrupt(): DES ciphertext is INCORRECT\n");
        bufferDump(descipher, 32);
        testfail++;
    }

    if (memcmp(aescipher, AEScipher, 32))
    {
        dbgmsg("testMultiDPD_withInterrupt(): AES ciphertext is INCORRECT\n");
        bufferDump(aescipher, 32);
        testfail++;
    }

    if (testfail == 0)
        dbgmsg("testMultiDPD_withInterrupt(): OK\n");

    freeBuf(dpdlist);
    freeBuf(hashplain);
    freeBuf(digest);
    freeBuf(desplain);
    freeBuf(deskey);
    freeBuf(descipher);
    freeBuf(aesplain);
    freeBuf(aeskey);
    freeBuf(aescipher);

    return 0;
}

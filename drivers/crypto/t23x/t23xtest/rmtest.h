/*
 * rmtest.h
 *
 * t23x RM testing module - common definitions
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




#ifndef RMTEST_H
#define RMTEST_H


/** @file
 * Helper functions for all tests to use
 */


#ifdef _cplusplus
extern "C" {
#endif


    extern int (*dbgmsg)(const char *fmt, ...);
    extern RMinterfaceCtx *ifctx;


    /* Helper functions for all tests to use */
    T2DPD *getDPD(int32_t);
    void freeBuf(void *);
    uint8_t *getInBuffer(uint8_t *content, int32_t size);
    uint8_t *getOutBuffer(int32_t size);
    linkEntry *getSGlist(int32_t entries);
    void setDPDfield(   T2DPD   *desc,
                        int32_t  field,
                        uint8_t *ptr,
                        uint16_t size,
                        uint8_t  extent,
                        uint8_t  jump);
    void bufferDump(uint8_t *buf, int32_t size);
    void *physAddr(void *vaddr);

    /* Individual test prototypes */

    int32_t testOneDPD_withInterruptSHA(RMinterfaceCtx *);
    int32_t testOneDPD_withInterruptRND(RMinterfaceCtx *);
    int32_t testMultiDPD_withInterrupt(RMinterfaceCtx *);

#ifdef _cplusplus
}
#endif

#endif /* RMTEST_H */

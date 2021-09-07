/*
 * Sec2local.h
 *
 * Interenal definitions for the SEC2.x legacy interface for the
 * t23x extensible crypto driver subsystem
 *
 * Copyright (c) 2007, Freescale Semiconductor, Inc.
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
 */



#ifndef SEC2LOCAL_H
#define SEC2LOCAL_H


/** @file
 * Descriptor construction definitions for the SEC2.x
 *   - security processing core family
 */


#ifdef _cplusplus
extern "C" {
#endif

#include "../common/t23.h"
#include "../common/xwcRMinterface.h"
#include "Sec2.h"


#define SEC2X_DEVMAJOR   (0)
#define SEC2X_DEVNAME    "sec2x"



    /**
     * Types of requests. These are saved off in the dynamic request
     * queue, or in the channel state block so that the deferred
     * service knows how to handle the request completion notification
     */
    typedef enum rq_type
    {
        RQ_KERNEL_ASYNC, /**< asynchronous,
                                can directly invoke handlers */
        RQ_USER_ASYNC,   /**< User-mode asynchronous,
                                will save and send signals */
        RQ_USER_BLOCK    /**< User-mode synchronous,
                                will waitqueue the request */
    } RQ_TYPE;


    /**
     * Specifies the memory translation needed when putting addresses
     * in a descriptor under construction
     */
    typedef enum ptrtype
    {
        PTR_PHYSICAL,       /**< physical address,
                                no translation needed                 */
        PTR_LOGICAL,        /**< logical address,
                                simple translation only               */
        PTR_USER_VIRTUAL,   /**< points to a user memory buffer       */
        PTR_KERNEL_VIRTUAL, /**< points to a kernel memory buffer     */
    } PTRTYPE;




    /*
     * Internal prototypes
     */

    /* t2dpd.c */
    int constructDPDlist(GENERIC_REQ *rqList, RMexecMessage *exMsg, PTRTYPE memtyp);
    int releaseDPDlist(RMexecMessage *exMsg, PTRTYPE memtyp);


#ifdef _cplusplus
}
#endif

#endif /* SEC2LOCAL_H */

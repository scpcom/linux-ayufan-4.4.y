
/*
 * xwcRMinterface.h
 *
 * Defines module interaction between layers for drivers that
 * conform to the Extensible Crypto Driver Framework (XCDF).
 * This header is meant to describe the Registered Interface/
 * Resource Manager component boundary. It contains no hardware
 * dependencies.
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
 * 1.0.0   2008_01_29 sec - add FW version support
 * 2.1.0   2009_05_04 sec - remove redundant registration info
 */


#ifndef XWCRMINTFC_H
#define XWCRMINTFC_H


/** @file
 * xwcRMinterface.h defines the inteface to the extensible driver
 * resource manager independent of the accelerator architecture.
 */


#ifdef __KERNEL__
#include <linux/wait.h>
#include <linux/string.h>
#endif

#include "../common/t23.h"


/* Use this as our "tag" for determining request compatibility, */
/* should it become a consideration                             */
#define XWC_FRAMEWORK_CURRENT_VERSION_STRING "2"
#define XWC_FRAMEWORK_CURRENT_VERSION         2

/**
 * Error status reported to interfaces for all RM requests
 */

typedef uint32_t RMstatus;

/**< No error, nothing to see here */
#define RM_OK                    (0)

/**< The context block referenced by an inbound request was
     misspecified  */
#define RM_BAD_INTFC_CTX         (-1)

/**< No more interfaces can be registered at this time */
#define RM_MAX_INTFC_REGISTERED  (-2)

/**< Registration request misspecified something */
#define RM_BAD_REGISTRATION_RQ   (-3)

/**< Deregistration request misspecified something */
#define RM_BAD_DEREGISTRATION_RQ (-4)

/**< A requested feature is not yet implemented in this version */
#define RM_UNIMPLEMENTED         (-5)

/**< Something in an exec message was corrupted on a queue request
     or a request cancellation */
#define RM_BAD_EXEC_MESSAGE      (-6)

/**< Some element in a passed descriptor chain is corrupt, or
     references some inaccessible element */
#define RM_CORRUPT_DESCRIPTOR    (-7)

/**< No more entries left in the exec queue, try again later */
#define RM_EXEC_QUEUE_FULL       (-8)

/**< An algorithm or processing mode requested is not present
     in the hardware being used */
#define RM_NO_CAPABILITY         (-9)

/**< The RM needs system heap for something, and can't get it
     at the present time. */
#define RM_NO_MEMORY             (-10)

/**< IF tried to cancel request that was already "in process"
     with a descriptor launched, so, can't cancel */
#define RM_CANNOT_CANCEL         (-11)

/**< Framework version not supported by this RM */
#define RM_UNSUPPORTED_FW        (-12)




/*
 * The basic context for a registered interface
 * The RM owns these, they are filled out and referenced to
 * the interface being registered.
 */


typedef struct __RMinterfaceCtx {

     /* interface state registration removed in v2+ due to disuse */

#ifdef __KERNEL__
    struct device *dev;     /* Assigned dev */
    void          *devctx;  /* Device-private data */
#endif

} RMinterfaceCtx;








/**
 * Basic RM request execution message
 *  @section request_reading Basic RM Request
 *
 *  Resource Manager - Execution Request Message
 *
 *  This is the basis of the request mechanism between any Interface
 *  and the Resource Manager. It describes everything that the IF
 *  needs the RM to do with a string of composed descriptors.
 * *
 *  When the RM receives this message, it will place this message
 *  on a queue for execution as soon as resources are available to
 *  execute it. The IF *must* leave this message, and it's associated
 *  descriptors, alone until the completion handler is called.
 *
 *  The request will return an entry ID that the IF can maintain
 *  if it needs to cancel a request.
 */


typedef struct _RMexecMessage {
    /* If framework evolves, this is a version compatibility tag */
    uint32_t           frameworkID;
    uint32_t           reserved;

    /*
     * Part #1: Everything about the set of descriptors that defines
     * this request. This includes a pointer to the head of the list,
     * any mapping information that must "live" with the request
     * throughout it's lifecycle, and a count of the total descriptors
     * in the list
     */
    void              *descHead;      /**< Points to DMA-able descriptor
                                           list, readable by hardware */
    void              *descBufferMap; /**< Auxiliary mapping info
                                           for desccriptor list, the actual
                                           content is hardware-dependent */
    uint32_t           descCount;     /**< Total descriptors in this set */

    /*
     * Part #2: Status/error information. Although an Interface can't touch
     * registers directly, it can interpret them. Therefore, this will hold
     * error information for interpretation by the interface, into whatever
     * form is useful.
     */
    u64                errState[4]; /**< untranslated error content from RM */

    void              *errDesc;     /**< physical address of the descriptor
                                         triggering the error, if implemented
                                         by the hardware in use             */


    /*
     * Part #3: Exec message release handler specification.
     * Since the Interface might not have a way of knowing when a request
     * has completed processing, it can specify a handler that knows how to
     * release all resources associated with the request. This specification
     * takes the form of a handler function, and a pointer to an Interface-
     * defined argument that identifies the individual request. For the
     * cirrent release, this function is required.
     */
    void             (*messageReleaseHandler)(void *);
    void              *releaseArgument;

    /*
     * Part #4: Reserved queue handlers. Used for allocating a queue
     * to a dedicated application, and invoking specialty handlers either
     * at interrupt time, or deferred service time.
     *
     * Unimplemented in the current release.
     */
    uint8_t            reservedQueue;
    void             (*queueISRHandler)(uint32_t); /* ISR level handler */
    void             (*queueAuxHandler)(uint32_t); /* deferred handler */

    /*
     * Part #5: Request completion identity.
     * At the completion of a request, the Resource Manager can take
     * optional actions to notify the Interface, or a user of the interface,
     * that the request is complete. There are two classes of such actions,
     * one for synchronous style requests, and another for asynchronous
     * requests.
     */
    /* Synchronous set */
    wait_queue_head_t *waitingTask; /**< saved waitqueue for waiting task */

    /* Asynchronous set */
    void             (*descriptorDoneHandler)(uint32_t);
    void             (*descriptorErrorHandler)(uint32_t);
    void              *initialRQ; /**< IF-defined pointer for deferred service */
    pid_t              rqID;      /**< PID of the requesting process */
    int                sigval[4]; /**< saved signals for user process */
    uint8_t            buftype; /**< memory type for buffer releases */



    /*
     * Part #6: Miscellaneous tracking information
     */
    RMinterfaceCtx    *owningIF; /**< Request belongs to this Interface */
} RMexecMessage;




/**
 * General prototypes for the RM's functionality exposed to an IF
 */

/** Queue a new request message */
RMstatus xwcRMqueueRequest(RMinterfaceCtx *intfc,
                           RMexecMessage  *execMsg,
                           uint32_t       *entryID);



/** Cancel a queued request */
RMstatus xwcRMcancelRequest(RMinterfaceCtx *intfc,
                            uint32_t        entryID);


/** Registration prototypes for any resource manager */
RMstatus xwcRMregisterInterface(uint8_t         *intname,
                                RMinterfaceCtx **regdesc);

RMstatus xwcRMderegisterInterface(RMinterfaceCtx *regdesc);


/** Memory buffer mapping/management functions for use by all IFs */

RMstatus xwcMemTranslateLogical(uint32_t    ent,
                                void       *pd,
                                void       *pdmap);

RMstatus xwcMemTranslateUserVirtual(uint32_t    ent,
                                    void       *pd,
                                    void       *pdmap);

RMstatus xwcMemTranslateKernelVirtual(uint32_t    ent,
                                      void       *pd,
                                      void       *pdmap);

RMstatus xwcMemReleaseLogical(uint32_t    ent,
                              void       *pd,
                              void       *pdmap);

RMstatus xwcMemReleaseUserVirtual(uint32_t    ent,
                                  void       *pd,
                                  void       *pdmap);

RMstatus xwcMemReleaseKernelVirtual(uint32_t    ent,
                                    void       *pd,
                                    void       *pdmap);


#endif /* XWCRMINTFC_H */

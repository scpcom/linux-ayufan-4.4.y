
/*
 * t23xrmInternal.h
 *
 * t23x extensible driver subsystem Inter-component definitions, specific
 * to the hardware architecture and the specific implementation
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
 * 2.1.0   2009_05_04 sec - simplify registration
 *
 */



/** @file
 * Defines internal data structures to be shared between modules of
 * the T2/3 resource manager. Does not expose any "public" interfaces
 */


#include <linux/version.h>

#include "../common/t23.h"
#include "../common/xwcRMinterface.h"

#ifndef T23XRMINTERNAL_H
#define T23XRMINTERNAL_H


#define T23X_PACKAGE_VERSION "2.1.0"


/* Limit of registrable interfaces */
#define MAX_INTERFACES (32)

/* Maximum entries in an execution queue */
#define EXEC_QUEUE_DEPTH (8192)

/* Maximum entries in the interrupt message queue */
#define ISRMSG_QUEUE_DEPTH (32)



/* Switch for drivers that don't have a device tree, or those that do */
#define NO_FDT  0
#define HAS_FDT 1



/**
 * Current state of a channel
 */
typedef enum
{
    CHstateFree,
    CHstateBusy,
    CHstateReserved,
} T2chState;

/**
 * Current processing state of a queue entry
 */
typedef enum
{
    RQstateEmpty,        /**< Empty request                     */
    RQstatePending,      /**< Waiting for resources             */
    RQstateProcessing,   /**< Running, descriptors queued       */
    RQstateDone,         /**< Processing done,
                                executing followup actions      */
    RQstateCanceling,    /**< Cancellation request received     */
} T2rqState;


/**
 * Queue entry for all requests. This is what the RM uses to track
 * queued requests
 */
typedef struct _T2RMqueueEntry
{
    RMexecMessage  *rqMsg;
    RMinterfaceCtx *ownerIF;
    T2rqState       rqState;
    /* prev/next */
} T2RMqueueEntry;



/**
 * Descriptor page mapping info used to track resources used in
 * scatter-gather mapping for any T2/3 descriptor. This gets used
 * with descBufferMap in the exec message
 */

typedef struct _PAIR_MAP
{
    struct page        **pages;
    int                  pageCt; /**< Count of mapped pages
                                        for this pair */
} T2PTR_PAIR_MAP;

typedef struct _DPD_AUXMAP
{
    T2PTR_PAIR_MAP pair[TOTAL_PAIRS];
    /* anything for whole of DPD? */
} T2DESC_AUXMAP;



/*
 * Interrupt event context - comprises the ISR queue
 */

#define INT_ACTIVE_CH0 (0x001)
#define INT_ACTIVE_CH1 (0x002)
#define INT_ACTIVE_CH2 (0x004)
#define INT_ACTIVE_CH3 (0x008)
#define INT_ACTIVE_CH4 (0x010)
#define INT_ACTIVE_CH5 (0x020)
#define INT_ACTIVE_CH6 (0x040)
#define INT_ACTIVE_CH7 (0x080)
#define INT_ACTIVE_CH8 (0x100)
#define INT_ACTIVE_CH9 (0x200)

struct internal_descinfo
{
    T2DPD     dpd;
    linkEntry glt[4];
    linkEntry slt[4];
};


typedef struct _T2ISRCtx
{
    u16 channelsDone;                    /**< bitmask of done channels */
    u16 channelsInError;                 /**< bitmask of errored channels */
    u64 channelState[T3_MAX_CHANNELS];   /**< CPSR of channel */
    u64 currentDesc[T3_MAX_CHANNELS];    /**< CDPR of channel */

    /* These save the EU state associated with a channel so that */
    /* more in-depth information can be analyzed                */
    u64 priEUstate[T3_MAX_CHANNELS]; /* Primary EU error status */
    u64 secEUstate[T3_MAX_CHANNELS]; /* Secondary EU error status */

#ifdef RM_DBG_EXTENDED_ERROR
    /* Captures current descriptor and SG tables upon error interrupt */
    /* This takes a lot of space, but it's the only practical way to  */
    /* "save" the information for out-of-ISR analysis. This is copied */
    /* as one chunk, so it has to mirror the actual registers         */
    struct internal_descinfo fetchedDesc[T3_MAX_CHANNELS];

#endif

} T2ISRCtx;



/**
 * Instance state block for each Talitos device in a system
 * Normally is only one, but if we keep this together, we can
 * handle multiples
 */

#define MAX_T2_INSTANCES (1)

typedef struct _T2CoreInstance
{
    /* Hardware information */
    T2CORE        *regs;            /**< base of Talitos register space */
    u32            doneIRQid;       /**< main interrupt ID */
    u32            ovflIRQid;       /**< overflow interrupt, nonzero if exists */

    /* Geometry of this core, derived from ID register or device tree */
    u8             totalChannels;  /**< Total usable channels        */
    u8             fifoDepth;      /**< Depth of fetchFIFOs          */
    u32            euPresent;      /**< header bits of installed EUs */
    u32            validTypes;     /**< valid descriptor types       */

    /* Translated stuff from device ID register */
    const u8      *devName;        /**< printable version name */
    u8             devMajor;       /**< 2 or 3     */
    u8             devMinor;       /**< ex: 2.n    */
    u8             devRev;         /**< ex: 2.1.n  */

    /* Current state information */
    /* will want channel reservation and states for each... */
    u8             freeChannels;
    T2chState      channelState[T3_MAX_CHANNELS];
    u64            channelConfig[T3_MAX_CHANNELS];
    RMexecMessage *channelActvMsg[T3_MAX_CHANNELS];

    /* ISR message queue for this instance */
    T2ISRCtx       t2isrQ[ISRMSG_QUEUE_DEPTH];
    u32            isrQhead;
    u32            isrQtail;
    u32            isrQlevel;

    /* Request feeder queue  */
    RMexecMessage  *execQ[EXEC_QUEUE_DEPTH];
    u32             execQhead;
    u32             execQtail;
    u32             execQlevel;
    spinlock_t      execQlock;

    /* Statistics collection - may not be permanent */
    u32             execQpeak;    /* Peak depth of global request queue */
    u32             isrQpeak;     /* Peak depth of ISR event queue */
    u64             processedRQs; /* Total exec messages processed */
    u64             chnDescCt[T3_MAX_CHANNELS]; /* total descriptors/channel */
} T2CoreInstance;





/**
 * Portable driver initialization - not exposed
 *
 */
int32_t t23RMdevInit(T2CoreInstance *t2blk,
                     uint32_t channels,
                     uint32_t fifo_depth,
                     uint32_t eu_present,
                     uint32_t desc_types);


/**
 * Portable driver shutdown/removal function - not exposed
 *
 */
RMstatus t23RMdevRemove(T2CoreInstance *t2blk);




#endif /* T23XRMINTERNAL_H */

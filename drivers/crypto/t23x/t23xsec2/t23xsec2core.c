/*
 * t23xsec2core.c
 *
 * t23x extensible driver subsystem - SEC2.x legacy interface
 * Module registration and request dispatch code
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
 * 2.1.0   2009_05_04 sec - simplified registration, compatibility
 *
 */



/** @file
 * Handles SEC2 legacy interface module registration and ioctl()
 * system call dispatch for the Talitos 2/3 xwc driver subsystem.
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>


/* Includes for SEC2 legacy interfaces */
#include "Sec2.h"
#include "Sec2local.h"

/* For the framework */
#include "../common/xwcRMinterface.h"

/* For the local rm */
#include "../t23xrm/t23xrmInternal.h"

#define BLOCK_RQ_PENDING (0x7fffffff)
#define BLOCK_RQ_DONE    (0x00000001)

#define EXECMSG_POOL_DEPTH 32



/* Globals for this module alone */

RMinterfaceCtx *ifctx;

RMexecMessage   excMsgPool[EXECMSG_POOL_DEPTH];
spinlock_t      excPoolLock;

BOOLEAN         msgInUse[EXECMSG_POOL_DEPTH];
T2DESC_AUXMAP   descPageMap[EXECMSG_POOL_DEPTH];

uint32_t        msgID;


/* Translate a CPSR into a SEC2 driver error                        */
/* Notice that this does nothing with the error descriptor pointer, */
/* this interface lacks a graceful means of handling it             */
static int sec2xTranslateError(RMexecMessage *xmsg)
{
    int status = SEC2_UNKNOWN_ERROR;
    u64 err;

    /* Only the first word of errState is meaningful for Talitos */
    /* All it contains is a saved copy of a CPSR, and we'll look */
    /* at the error field alone for error information.           */

    err = xmsg->errState[0] & T2_CPSR_ERROR_MASK;

#ifdef SEC2_DBG_EXTENDED_ERROR
    if (err)
    {
        printk("t23xsec2: error translation CPSR:0x%016llx\n", xmsg->errState[0]);
        printk("                          pri EU:0x%016llx\n", xmsg->errState[2]);
        printk("                          sec EU:0x%016llx\n", xmsg->errState[3]);
    }
#endif

    /* if no value, OK, so return and skip translation */
    if (!err)
        return SEC2_SUCCESS;

    if ((err & T2_CHN_ERROR_EU_ERROR) ||
        (err & T2_CHN_ERROR_INVALID_EU) ||
        (err & T2_CHN_ERROR_ILLEGAL_HEADER))
        status = SEC2_CHA_ERROR;

    if (err & T2_CHN_ERROR_FP_ZERO)
        status = SEC2_INVALID_OPERATION_ID;

    if (err & T2_CHN_ERROR_FP_ZERO)
        status = SEC2_INCOMPLETE_POINTER;

    if ((err & T2_CHN_ERROR_G_BOUNDARY) ||
        (err & T2_CHN_ERROR_G_LENGTH) ||
        (err & T2_CHN_ERROR_S_BOUNDARY) ||
        (err & T2_CHN_ERROR_S_LENGTH) ||
        (err & T2_CHN_ERROR_SG_ZERO_LEN))
        status = SEC2_SCATTER_LIST_ERROR;

    if (err & T2_CHN_ERROR_MDTE)
        status = SEC2_BUS_MASTER_ERROR;

    if ((err & T2_CHN_ERROR_DOF) ||
        (err & T2_CHN_ERROR_SOF))
        status = SEC2_FETCH_FIFO_OVERFLOW;

    return status;
}




static RMexecMessage *getExecMsg(void)
{
    int i;

    spin_lock(&excPoolLock);
    for (i = 0; i < EXECMSG_POOL_DEPTH; i++)
        if (msgInUse[i] == FALSE)
        {
            msgInUse[i] = TRUE;
            spin_unlock(&excPoolLock);
            return &excMsgPool[i];
        }

    spin_unlock(&excPoolLock);
    return NULL;
}

static void freeExecMsg(RMexecMessage *execMsg)
{
    int i;

    spin_lock(&excPoolLock);
    for (i = 0; i < EXECMSG_POOL_DEPTH; i++)
        if (execMsg == &excMsgPool[i])
        {
            msgInUse[i] = FALSE;
            spin_unlock(&excPoolLock);
            return;
        }
    spin_unlock(&excPoolLock);

    return;
}


/* Release handler (callback) for a blocking request       */
/* Invoked by the RM by reference out of a request message */
static void sec2xBlockReleaseHandler(void *rqarg)
{
    volatile int *wasHere = rqarg;

    *wasHere = BLOCK_RQ_DONE;
}

extern struct task_struct *find_task_by_vpid(pid_t nr);
/* Release handler for completion of a non-blocking request */
/* All notifiers and deallocations have to happen from here */
static void sec2xAsyncReleaseHandler(void *msg)
{
    RMexecMessage      *thisMsg = msg;
    GENERIC_REQ        *originalRQ;
    int                 status;
    struct task_struct *usertask;

    releaseDPDlist(thisMsg, thisMsg->buftype);

    originalRQ = (GENERIC_REQ *)thisMsg->initialRQ;

    /* Translate error registers into something local */
    status = sec2xTranslateError(thisMsg);

    /* Do notifiers. If rqID has a PID, send signals */
    if (status == SEC2_SUCCESS)
    {
        if (thisMsg->rqID)
        {
            usertask = find_task_by_vpid(thisMsg->rqID);
            send_sig(thisMsg->sigval[0], usertask, 1);
        }
        else
            if (originalRQ->notify != NULL)
                originalRQ->notify(originalRQ->pNotifyCtx);
    }
    else
    {
        if (thisMsg->rqID)
        {
            usertask = find_task_by_vpid(thisMsg->rqID);
            send_sig(thisMsg->sigval[1], usertask, 1);
        }
        else
            if (originalRQ->notify_on_error != NULL)
                originalRQ->notify_on_error(NULL);
    }

    /* free up resources */
    freeExecMsg(thisMsg);
}





/**
 * Initialize the legacy interface and connect it to the
 * current resource manager
 */
int SEC2xDrvInit(void)
{
    RMstatus rmstatus;
    int i, j;

    /* Set up the exec message pool for this module */
    for (i = 0; i < EXECMSG_POOL_DEPTH; i++)
    {
        excMsgPool[i].frameworkID            = XWC_FRAMEWORK_CURRENT_VERSION;
        excMsgPool[i].descCount              = 0;
        excMsgPool[i].errState[0]            = 0;
        excMsgPool[i].errState[1]            = 0;
        excMsgPool[i].errState[2]            = 0;
        excMsgPool[i].errState[3]            = 0;
        excMsgPool[i].errDesc                = NULL;
        excMsgPool[i].messageReleaseHandler  = NULL;
        excMsgPool[i].releaseArgument        = NULL;
        excMsgPool[i].waitingTask            = NULL;
        excMsgPool[i].queueISRHandler        = NULL;
        excMsgPool[i].queueAuxHandler        = NULL;
        excMsgPool[i].reservedQueue          = 0;
        excMsgPool[i].descriptorDoneHandler  = NULL;
        excMsgPool[i].descriptorErrorHandler = NULL;
        excMsgPool[i].buftype                = 0;
        excMsgPool[i].initialRQ              = NULL;
        excMsgPool[i].rqID                   = 0;
        excMsgPool[i].sigval[0]              = 0;
        excMsgPool[i].sigval[1]              = 0;
        excMsgPool[i].sigval[2]              = 0;
        excMsgPool[i].sigval[3]              = 0;
        excMsgPool[i].owningIF               = NULL;


        excMsgPool[i].descHead      = kmalloc(sizeof(T2DPD) * T2_CHANNEL_FIFO_DEPTH, GFP_KERNEL | GFP_DMA);
        excMsgPool[i].descBufferMap = &descPageMap[i];

        if (excMsgPool[i].descHead == NULL)
        {
            printk("t23xsec2: can't get descriptor buffers\n");
            for (j = i - 1; j > -1; j--)
                kfree(excMsgPool[i].descHead);

            return -1;
        }
    }
    spin_lock_init(&excPoolLock);

    /* register with the RM */
    rmstatus = xwcRMregisterInterface("t23xsec2", &ifctx);
    if (rmstatus != RM_OK)
    {
        printk("t23xsec2: can't register interface, error %d\n", rmstatus);
        return -1;
    }

#ifdef SEC2_DBG_INFO
    printk("Extensible Crypto Driver - SEC2.x Legacy Interface - pkg rev %s\n", T23X_PACKAGE_VERSION);
#endif

    return 0;
}




/**
 * Disconnect this interface
 */

int SEC2xShutdown(void)
{
    RMstatus rmstatus;
    int      i;
    /* Cancel any pending requests!!!! */

    /* Release local resources */
    for (i = 0; i < EXECMSG_POOL_DEPTH; i++)
        kfree(excMsgPool[i].descHead);

    /* Deregister from the RM */
    rmstatus = xwcRMderegisterInterface(ifctx);
    if (rmstatus != RM_OK)
    {
        printk("t2xsec2: can't deregister this interface, error %d\n", rmstatus);
        return -1;
    }

    return 0;
}



/**
 * Open a new path and make it known to this IF
 */
int SEC2xOpen(struct inode *nd, struct file *fil)
{


    return SEC2_SUCCESS;
}


/**
 * Close an open path
 */
int SEC2xClose(struct inode *nd, struct file *fil)
{

    /* nothing needed here... */
    return SEC2_SUCCESS;
}




/**
 * ioctl() entry point from the I/O manager
 *
 */
int SEC2xIoctl(struct inode  *nd,
               struct file   *fil,
               unsigned int   code,
               unsigned long  param)
{
    int               status;
    RQ_TYPE           rqType;
    RMstatus          rmstat;
    RMexecMessage    *localMsg;
    wait_queue_head_t wq;
    wait_queue_t      wqent;
    volatile int      blockst;
    int               i;
    MALLOC_REQ       *mem;
    KBUF_MULTI       *kbm;
    PTRTYPE           memtype;


    status = SEC2_SUCCESS;

    switch (code)
    {

        /* standard request type */
        case IOCTL_PROC_REQ:
        case IOCTL_PROC_REQ_VIRTUAL:

            /* Check the param block */
            if (param == (int)NULL)
            {
                status = SEC2_INVALID_ADDRESS;
                break;
            }

            /* Figure out the memory type we have to deal with */
            /* If virtual specified, we have to figure out which type */
            if (code == IOCTL_PROC_REQ_VIRTUAL)
                if (fil == NULL)
                    memtype = PTR_KERNEL_VIRTUAL;
                else
                    memtype = PTR_USER_VIRTUAL;
            else
                memtype = PTR_LOGICAL;

            /* Allocate a request message from the "pool" */
            localMsg = getExecMsg();
            if (localMsg == NULL)
                return SEC2_INSUFFICIENT_REQS;

            /* Construct a list of descriptors from the input */
            status = constructDPDlist((GENERIC_REQ *)param, localMsg, memtype);
            if (status != SEC2_SUCCESS)
                return status;

            /* Set up completion handlers here. For a non-blocking */
            /* request like this, we have to do all releasing from */
            /* inside the handler itself, because this function    */
            /* may exit before the request completes               */
            localMsg->messageReleaseHandler = sec2xAsyncReleaseHandler;
            localMsg->releaseArgument       = localMsg;
            localMsg->buftype               = (uint8_t)memtype;

            /* Save off parameter block, process ID, signal values */
            localMsg->initialRQ             = (void *)param;
            if (fil != NULL)
            {
                localMsg->rqID                  = current->pid;
                localMsg->sigval[0]             = (int)(((GENERIC_REQ *)param)->notify);
                localMsg->sigval[1]             = (int)(((GENERIC_REQ *)param)->notify_on_error);
            }
            else
                localMsg->rqID = 0;

            /* Set the RM to processing our request */
            rmstat = xwcRMqueueRequest(ifctx, localMsg, &msgID);

            /* report error if the RM no can do... */
            if (rmstat)
            {
                printk("t23xsec2:ioctl() - error 0x%08x from RM, request not initiated\n", rmstat);
                status = SEC2_UNKNOWN_ERROR;
                releaseDPDlist(localMsg, memtype);
                freeExecMsg(localMsg);
            }

            /* Return status to the user and go home. If queueRequest()   */
            /* worked OK, now it's processing, and it's up to the release */
            /* handler to free resources and translate/report errors      */
            return status;
            break;


        /* blocking request types, should ONLY come from usermode */
        case IOCTL_PROC_REQ_BLOCK:
        case IOCTL_PROC_REQ_BLOCK_VIRTUAL:
            /* check the presence of a param block */
            if (param == (int)NULL)
            {
                status = SEC2_INVALID_ADDRESS;
                break;
            }

            if (fil == NULL) /* only valid from usermode */
            {
                status = SEC2_INVALID_REQUEST_MODE;
                break;
            }
            else
                rqType = RQ_USER_BLOCK;

            if (code == IOCTL_PROC_REQ_BLOCK_VIRTUAL)
                memtype = PTR_USER_VIRTUAL;
            else
                memtype = PTR_LOGICAL;

            /* Allocate a request message from the "pool" */
            localMsg = getExecMsg();
            if (localMsg == NULL)
                return SEC2_INSUFFICIENT_REQS;

            /* Construct a list of descriptors from the input */
            status = constructDPDlist((GENERIC_REQ *)param, localMsg, memtype);
            if (status != SEC2_SUCCESS)
                return status;

            /* Set up completion action & waitqueue entry for this request */
            blockst                         = BLOCK_RQ_PENDING;
            localMsg->messageReleaseHandler = sec2xBlockReleaseHandler;
            localMsg->releaseArgument       = (void *)&blockst;

            init_waitqueue_head(&wq);
            init_waitqueue_entry(&wqent, current);
            add_wait_queue(&wq, &wqent);
            localMsg->waitingTask = &wq;
            set_current_state(TASK_INTERRUPTIBLE);

            /* Pass constructed request off to the RM for processing */
            rmstat = xwcRMqueueRequest(ifctx, localMsg, &msgID);

            /* report error, else spin on the waitqueue */
            if (rmstat)
            {
                status = SEC2_UNKNOWN_ERROR; /* worst case error */
                if (rmstat == RM_NO_CAPABILITY)     /* maybe no such CHA? */
                    status = SEC2_INVALID_CHA_TYPE;
                set_current_state(TASK_RUNNING);
            }
            else
            {
                while(1)
                {
                    set_current_state(TASK_INTERRUPTIBLE);
                    if (blockst == BLOCK_RQ_PENDING)
                        schedule();
                    else
                        break;
                }
                set_current_state(TASK_RUNNING);
            }

            /* Release the DPD list. */
            releaseDPDlist(localMsg, memtype);

            /* If error from the waitqueue hold, return it */
            if (status)
                return status;

            /* If no error from the waitqueue hold, then check our exec */
            /* message for error registers, and translate */
            if (!status)
                status = sec2xTranslateError(localMsg);

            /* all done with this exec msg */
            freeExecMsg(localMsg);
            return status;

            break;

        case IOCTL_GET_STATUS:
            status = SEC2_UNIMPLEMENTED;
            break;

        case IOCTL_RESERVE_CHANNEL_STATIC:
#ifdef UNIMPLEMENTED
            status = ReserveChannelStatic((unsigned char *)param,
                    (int)taskIdSelf());
#endif
            status = SEC2_UNIMPLEMENTED;
            break;

        case IOCTL_RELEASE_CHANNEL:
#ifdef UNIMPLEMENTED
            status = ReleaseChannel(*(unsigned char *)param, (int)taskIdSelf(), FALSE);
#endif
            status = SEC2_UNIMPLEMENTED;
            break;


        case IOCTL_MALLOC:
            if ((((MALLOC_REQ *)param)->ptr =
                        kmalloc(((MALLOC_REQ *)param)->sz, GFP_KERNEL | GFP_DMA)) == 0)
            {
                status = SEC2_MALLOC_FAILED;
                break;
            }
            memset(((MALLOC_REQ *)param)->ptr, 0, ((MALLOC_REQ *)param)->sz);
            status = SEC2_SUCCESS;
            break;

        case IOCTL_COPYFROM:
            mem = (MALLOC_REQ *)param;
            mem->pid = current->pid;
            copy_from_user(mem->to, mem->from, mem->sz);
            status = SEC2_SUCCESS;
            break;

        case IOCTL_COPYTO:
            mem = (MALLOC_REQ *)param;
            mem->pid = current->pid;
            copy_to_user(mem->to, mem->from, mem->sz);
            status = SEC2_SUCCESS;
            break;

        case IOCTL_FREE:
            kfree((void *)param);
            break;

        case IOCTL_KBUF_MULTI_PUSH:
            kbm = (KBUF_MULTI *)param;
            for (i = 0; i < MAX_PAIRS; i++)
            {
                if ((kbm->pair[i].local != NULL) &&
                        (kbm->pair[i].kbuf != NULL) &&
                        (kbm->pair[i].size > 0))
                    copy_from_user(kbm->pair[i].kbuf,   /* destination */
                            kbm->pair[i].local,  /* source      */
                            kbm->pair[i].size);
            }
            break;

        case IOCTL_KBUF_MULTI_PULL:
            kbm = (KBUF_MULTI *)param;
            for (i = 0; i < MAX_PAIRS; i++)
            {
                if ((kbm->pair[i].local != NULL) &&
                        (kbm->pair[i].kbuf != NULL) &&
                        (kbm->pair[i].size > 0))
                    copy_to_user(kbm->pair[i].local,   /* destination */
                            kbm->pair[i].kbuf,    /* source      */
                            kbm->pair[i].size);
            }
            break;

        case IOCTL_KBUF_MULTI_ALLOC:
            kbm = (KBUF_MULTI *)param;
            for (i = 0; i < MAX_PAIRS; i++)
            {
                /* If size spec'ed nonzero, allocate buffer */
                if (kbm->pair[i].size)
                {
                    kbm->pair[i].kbuf = kmalloc(kbm->pair[i].size, GFP_KERNEL | GFP_DMA);
                    /* If allocate error, unwind any other allocs and exit */
                    if (kbm->pair[i].kbuf == NULL)
                    {
                        while (i >= 0)
                        {
                            if (kbm->pair[i].kbuf != NULL)
                                kfree(kbm->pair[i].kbuf);
                            i--;
                        }
                        status = SEC2_MALLOC_FAILED;
                        break;
                    } /* end allocation error */
                } /* end if (nonzero size) */
            }
            status = SEC2_SUCCESS;
            break;


        case IOCTL_KBUF_MULTI_FREE:
            kbm = (KBUF_MULTI *)param;
            for (i = 0; i < MAX_PAIRS; i++)
                if (kbm->pair[i].kbuf != NULL)
                    kfree(kbm->pair[i].kbuf);
            break;



        case IOCTL_INSTALL_AUX_HANDLER:
#ifdef UNIMPLEMENTED
            chan = ((AUX_HANDLER_SPEC *)param)->channel;
            /* see if requested channel is valid */
            if ((chan <= 0) || (chan > TotalChannels))
            {
                status = SEC2_INVALID_CHANNEL;
                break;
            }

            /* channel is valid, is it reserved (and not busy)? */
            if (ChannelAssignments[chan - 1].assignment != CHANNEL_STATIC_ASSIGNED)
            {
                status = SEC2_CHANNEL_NOT_AVAILABLE;
                break;
            }


            /* Channel spec is in range, and is reserved for use. Notice that */
            /* we really don't have any good means to identify the requestor  */
            /* for validity (could be the kernel itself), so will assume that */
            /* channel ownership is not an issue. Now register/remove the     */
            /* handler                                                        */

            ChannelAssignments[chan - 1].auxHandler = ((AUX_HANDLER_SPEC *)param)->auxHandler;
#endif
            status = SEC2_UNIMPLEMENTED;
            break;

    } /* switch (code) */


    return status;
}




/* Equivalence symbol to mimic the entry point of the old driver */
int SEC2_ioctl(struct inode  *nd,
               struct file   *fil,
               unsigned int   code,
               unsigned long  param)
{
    return(SEC2xIoctl(nd, fil, code, param));
}

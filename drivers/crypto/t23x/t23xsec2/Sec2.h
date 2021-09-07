
/*
 * Sec2.h
 *
 * Public-level interfaces for the SEC2.x legacy crypto interface
 * Contains changes to interwork with the t23x extensible subsystem
 * for Talitos 2/3
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
 * 2.1.0   2009_05_04 sec - Add SNOW-3G support
 *
 */




#ifndef SEC2_H
#define SEC2_H



/** @file
 * Top-level driver include file for SEC2 security processor
 *
 *        - XWC prototype version
 */


#ifdef _cplusplus
extern "C" {
#endif




    /* General types for the driver, if they don't exist elsewhere */

#ifndef BOOLEAN
#define BOOLEAN int
#endif

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#ifndef offsetof
#define offsetof(s,m)   (size_t)&(((s *)0)->m)
#endif



    /* IOCTL function code offsets */
    /* Each defines an IOCTL request to be added to a system-defined offset */
#define SEC2_PROC_REQ                (1)  /**< Process request        */
#define SEC2_GET_STATUS              (2)  /**< Read delayed status    */
#define SEC2_RESERVE_CHANNEL_STATIC  (4)  /**< Reserve any channel
                                                for use               */
#define SEC2_RELEASE_CHANNEL         (8)  /**< Release channel
                                                from reserved use     */
#define SEC2_MALLOC                  (13) /**< Allocate driver
                                                storage block         */
#define SEC2_FREE                    (14) /**< Free a driver
                                                storage block         */
#define SEC2_COPYFROM                (15) /**< Driver buffer
                                                copy function         */
#define SEC2_COPYTO                  (16) /**< Opposing buffer
                                                copy function         */
#define SEC2_INSTALL_AUX_HANDLER     (20) /**< Install aux handler
                                                for channel           */
#define SEC2_KBUF_MULTI_PUSH         (21) /**< Push multiple buffers
                                                to kernel             */
#define SEC2_KBUF_MULTI_PULL         (22) /**< Pull multiple buffers
                                                from kernel           */
#define SEC2_KBUF_MULTI_ALLOC        (23) /**< Allocate multiple
                                                kernel buffers        */
#define SEC2_KBUF_MULTI_FREE         (24) /**< Free multiple
                                                kernel buffers        */
#define SEC2_PROC_REQ_BLOCK          (25) /**< PROC_REQ with block          */

/* 26-29 reserved for SEC2 request expansion */

#define SEC2_PROC_REQ_VIRTUAL        (30) /**< PROC_REQ using virtual
                                               buffers */

#define SEC2_PROC_REQ_BLOCK_VIRTUAL  (31) /**< PROC_REQ_BLOCK using
                                               virtual buffers */


    /* Base IOCTL function index,                 */
    /* assume 0 to 0x7ff reserved for the host OS */
#define SEC2_IOCTL_INDEX             (0x800)

    /* Computed IOCTL constants for use */
#define IOCTL_PROC_REQ               (SEC2_IOCTL_INDEX + SEC2_PROC_REQ)
#define IOCTL_GET_STATUS             (SEC2_IOCTL_INDEX + SEC2_GET_STATUS)
#define IOCTL_RESERVE_CHANNEL_STATIC (SEC2_IOCTL_INDEX + SEC2_RESERVE_CHANNEL_STATIC)
#define IOCTL_RELEASE_CHANNEL        (SEC2_IOCTL_INDEX + SEC2_RELEASE_CHANNEL)
#define IOCTL_MALLOC                 (SEC2_IOCTL_INDEX + SEC2_MALLOC)
#define IOCTL_FREE                   (SEC2_IOCTL_INDEX + SEC2_FREE)
#define IOCTL_COPYFROM               (SEC2_IOCTL_INDEX + SEC2_COPYFROM)
#define IOCTL_COPYTO                 (SEC2_IOCTL_INDEX + SEC2_COPYTO)
#define IOCTL_INSTALL_AUX_HANDLER    (SEC2_IOCTL_INDEX + SEC2_INSTALL_AUX_HANDLER)
#define IOCTL_KBUF_MULTI_PUSH        (SEC2_IOCTL_INDEX + SEC2_KBUF_MULTI_PUSH)
#define IOCTL_KBUF_MULTI_PULL        (SEC2_IOCTL_INDEX + SEC2_KBUF_MULTI_PULL)
#define IOCTL_KBUF_MULTI_ALLOC       (SEC2_IOCTL_INDEX + SEC2_KBUF_MULTI_ALLOC)
#define IOCTL_KBUF_MULTI_FREE        (SEC2_IOCTL_INDEX + SEC2_KBUF_MULTI_FREE)
#define IOCTL_PROC_REQ_BLOCK         (SEC2_IOCTL_INDEX + SEC2_PROC_REQ_BLOCK)

#define IOCTL_PROC_REQ_VIRTUAL       (SEC2_IOCTL_INDEX + SEC2_PROC_REQ_VIRTUAL)
#define IOCTL_PROC_REQ_BLOCK_VIRTUAL (SEC2_IOCTL_INDEX + SEC2_PROC_REQ_BLOCK_VIRTUAL)




    /* General error/status codes for ioctl() function returns,       */
    /* and for the status field of the general-purpose part of        */
    /* the request struct                                             */

    /* all OK                                               */
#define SEC2_SUCCESS                          (0x00000000)

    /* Driver memory allocation problem                     */
#define SEC2_MEMORY_ALLOCATION                (0xE004FFFF)

    /* Channel spec out-of-range.                           */
#define SEC2_INVALID_CHANNEL                  (0xE004FFFE)

    /* Requested cipher type not supported on               */
    /* this hardware.                                       */
#define SEC2_INVALID_CHA_TYPE                 (0xE004FFFD)

    /* Operation type specified in the request              */
    /* block does not exist                                 */
#define SEC2_INVALID_OPERATION_ID             (0xE004FFFC)

    /* Requested channel not available for use              */
#define SEC2_CHANNEL_NOT_AVAILABLE            (0xE004FFFB)

    /* Requested cipher type already in use by              */
    /* some other request                                   */
#define SEC2_CHA_NOT_AVAILABLE                (0xE004FFFA)

    /* At least one data length in the request is invalid   */
    /* This is likely an alignment issue                    */
#define SEC2_INVALID_LENGTH                   (0xE004FFF9)

    /* An address in the request struct is misaligned       */
#define SEC2_OUTPUT_BUFFER_ALIGNMENT          (0xE004FFF8)

    /* Driver has internal address translation err          */
#define SEC2_ADDRESS_PROBLEM                  (0xE004FFF6)

    /* Driver has run out of request entries                */
#define SEC2_INSUFFICIENT_REQS                (0xE004FFF5)

    /* An accelerator is in error state after a request     */
#define SEC2_CHA_ERROR                        (0xE004FFF2)

    /* Specified request was NULL                           */
#define SEC2_NULL_REQUEST                     (0xE004FFF1)

    /* Timeout during request execution                     */
#define SEC2_REQUEST_TIMED_OUT                (0xE004FFF0)

    /* Driver ran out of memory in the course of processing */
#define SEC2_MALLOC_FAILED                    (0xE004FFEF)

    /* Driver failed to free a block while processing       */
#define SEC2_FREE_FAILED                      (0xE004FFEE)

    /* Driver encountered a parity error in the input data  */
#define SEC2_PARITY_SYSTEM_ERROR              (0xE004FFED)

#define SEC2_INCOMPLETE_POINTER               (0xE004FFEC)
#define SEC2_TEA_ERROR                        (0xE004FFEB)

    /* While trying to build a scatter-gather fragment list */
    /* for a specified request, the driver ran out of       */
    /* entries to store into                                */
#define SEC2_FRAGMENT_POOL_EXHAUSTED          (0xE004FFEA)

    /* Driver attempted to stuff too many DPDs into the     */
    /* fetch FIFO. This indicates an internal driver error  */
#define SEC2_FETCH_FIFO_OVERFLOW              (0xE004FFE9)

    /* Hardware failed while trying to acquire mastership   */
    /* of the bus                                           */
#define SEC2_BUS_MASTER_ERROR                 (0xE004FFE8)

    /* While composing a scatter descriptor, the driver     */
    /* encountered an error, most likely a bad pointer in   */
    /* the specification list                               */
#define SEC2_SCATTER_LIST_ERROR               (0xE004FFE7)

    /* Driver encountered an unidentfiable condition        */
#define SEC2_UNKNOWN_ERROR                    (0xE004FFE6)

    /* Request invalid for usage mode                       */
    /* Returned for blocking request from out of usermode   */
#define SEC2_INVALID_REQUEST_MODE             (0xE004FFE5)

    /* Unrecognized IOCTL code passed on request */
#define SEC2_UNKNOWN_IOCTL_CODE               (0xE004FFE4)

    /* Unimplemented request */
#define SEC2_UNIMPLEMENTED                    (0xE004FFE3)



    /* Security device not found at the specified location  */
    /* This is a legacy error left over from earlier revs   */
#define SEC2_IO_CARD_NOT_FOUND                     (-1000)

    /* Driver can't allocate a buffer                       */
#define SEC2_IO_MEMORY_ALLOCATE_ERROR              (-1001)

    /* Driver has a problem with register I/O               */
#define SEC2_IO_IO_ERROR                           (-1002)

    /* System can't attach driver to the OS dispatch table  */
#define SEC2_IO_VXWORKS_DRIVER_TABLE_ADD_ERROR     (-1003)

    /* Driver can't use a needed interrupt ID               */
#define SEC2_IO_INTERRUPT_ALLOCATE_ERROR           (-1004)

    /* Driver can't use peripheral base region              */
#define SEC2_CANNOT_SETUP_BAR0_ERROR               (-1008)

    /* Driver can't initialize message queue                */
#define SEC2_VXWORKS_CANNOT_CREATE_QUEUE           (-1009)

    /* Request was cancelled by external means              */
#define SEC2_CANCELLED_REQUEST                     (-1010)

    /* User passed a null request pointer to IOCTL_PROC_REQ */
#define SEC2_INVALID_ADDRESS                       (-1011)






    /* Limit of geometry, for allocators */
#define MAX_CHANNELS     (4)
#define MAX_CHAS         (7)
#define NUM_DPD_FLDS     (7)


    /*
     * STATUS_REQ
     * structure used to indicate the state of the SEC2 as well as the
     * river.  Returned as a pointer by GetStatus() and embedded in all
     * requests.
     */
    typedef struct
    {
        unsigned long ChaAssignmentStatusRegister[2];
        unsigned long InterruptControlRegister[2];
        unsigned long InterruptStatusRegister[2];
        unsigned long IdRegister;
        unsigned long ChannelStatusRegister[MAX_CHANNELS][2];
        unsigned long ChannelConfigurationRegister[MAX_CHANNELS][2];
        unsigned long CHAInterruptStatusRegister[MAX_CHAS][2];
        unsigned long QueueEntryDepth;
        unsigned long FreeChannels;
        unsigned long FreeRngas;
        unsigned long FreeAfhas;
        unsigned long FreeDesas;
        unsigned long FreeMdhas;
        unsigned long FreePkhas;
        unsigned long FreeAesas;
        unsigned long FreeKeas;
        unsigned long BlockSize;
    } STATUS_REQ;

    typedef STATUS_REQ SEC2_STATUS;

    typedef void* PSEC2_NOTIFY_CTX;

    /*
     * SEC2_NOTIFY_ON_ERROR_CTX
     * structure used in all requests to indicate the nature of errors.
     */
    typedef struct
    {
        unsigned long errorcode; /**< Error the request generated     */
        void *request;           /**< Pointer to original request block */
        STATUS_REQ driverstatus; /**< Detailed device state block     */
    } SEC2_NOTIFY_ON_ERROR_CTX, *PSEC2_NOTIFY_ON_ERROR_CTX;


    typedef void (*PSEC2_NOTIFY_ROUTINE) (PSEC2_NOTIFY_CTX pCtx);
    typedef void (*PSEC2_NOTIFY_ON_ERROR_ROUTINE) (PSEC2_NOTIFY_ON_ERROR_CTX pErrCtx);




    /**
     * AUX_HANDLER_SPEC
     *
     * Passed as the argument to SEC2_INSTALL_AUX_HANDLER, and is used
     * to specify an auxiliary handler to be called via deferred service
     * for a channel completion interrupt. Such a handler is called
     * before the standard channel service
     *
     * It has two parts, a pointer to the handler requested (or NULL if
     * an existing handler is to be disabled), and the channel that the
     * handler is to be associated with (this channel must be reserved
     * static to be used for this).
     *
     * Note that for Linux targets, this feature can not be used from
     * a user-mode application.

     */
    typedef struct _aux_handler_spec
    {
        int  (*auxHandler)(int, void *);    /**< points to handler
                                                routine specified   */
        char   channel;                     /**< channel previously
                                                reserved for use    */
    } AUX_HANDLER_SPEC;


    /**
     * MALLOC_REQ
     * structure used on allocation requests to the driver
     *
     *  Used by KernelMalloc()
     *
     *  Used also by the Dispatch function that calls
     *  KernelMalloc() and KernelFree()
     */
    typedef struct
    {
        unsigned long   sz;     /**< Number of bytes to allocate
                                 * Zero means to use the default.
                                 * A value of zero can be used to
                                 * avoid fragmentation.               */
        void            *ptr;   /**< Pointer to the adress that is to
                                 * be returned by a call to
                                 * KernelMalloc() or a pointer to
                                 * an address that is to
                                 * be freed when calling KernelFree() */
        char            *to;    /**< copy to                          */
        char            *from;  /**< copy from                        */
        int             pid;    /**< pid                              */
    } MALLOC_REQ;



    /**
     * KBUF_MULTI
     *
     * Used to push/pull a set of data items from local (virtual) memory to
     * contiguous kernel buffers. It contains 7 source/destination
     * buffer pairs, one for each possible pair represented in a
     * packet descriptor
     *
     * Each pair consists of:
     *   local - address of local data buffer
     *   kbuf  - address of kernel (contiguous) data buffer
     *   size  - size of data for allocation or movement
     *
     * This block works with all multibuffer driver service calls:
     *
     * - IOCTL_KBUF_MULTI_PUSH
     *   Move buffers from local memory to kernel buffers.
     *   "local" is source, "kbuf" is destination, "size" in bytes
     *   Any zero size, or pair with one null pointer is skipped
     *
     * - IOCTL_KBUF_MULTI_PULL
     *   Move from kernel buffers to local memory
     *   "local" is destination, "kbuf" is source, "size" in bytes
     *   Any zero size, or pair with one null pointer is skipped
     *
     * - IOCTL_KBUF_MULTI_ALLOC
     *   Alloc kernel buffers, "kbuf" gets filled with address of "size"
     *   so long as size is nonzero. If any buffer is unavailable, will
     *   return error (and no buffers are allocated)
     *
     * - IOCTL_KBUF_MULTI_FREE,  "kbuf" gets freed if non-NULL
     *
     */

    typedef struct
    {
        unsigned char *local;  /**< Local ptr
                                    (src for push, dest for pull)     */
        unsigned char *kbuf;   /**< Kernel ptr
                                    (dest for push, src for pull)     */
        unsigned int   size;   /**< Size of buffer in bytes           */
    } KBUF_ITEM;

#define MAX_PAIRS (NUM_DPD_FLDS)

    typedef struct
    {
        KBUF_ITEM pair[MAX_PAIRS];
    } KBUF_MULTI;



    /**
     * Basic scatter/gather linked list form for general-purpose use
     * This is used by the requestor to identify scatter buffer
     * fragments to the driver for any pointer in a request. It's
     * constructed in common NULL-terminated linked list form
     */

    typedef struct _sec2scatterFragment
    {
        void          *next;                  /**< next fragment in list
                                                    NULL if none      */
        void          *fragment;              /**< physical location
                                                    of fragment       */
        unsigned short size;                  /**< size of this
                                                    fragment          */
    } EXT_SCATTER_ELEMENT;




/* Direct call into the driver's scatter/gather buffer marking function */
int MarkScatterBuffer(void *request, void *buffer);



#define RNG_GROUP       (0x1000)          /**< RNG Group              */
#define DES_GROUP       (0x2000)          /**< Des Groups             */
#define ARC4_GROUP      (0x3000)          /**< RC4 Groups             */
#define MD_GROUP        (0x4000)          /**< MD Groups              */
#define PK_GROUP        (0x5000)          /**< PK Groups              */
#define AES_GROUP       (0x6000)          /**< AES Groups             */
#define IPSEC_DES_GROUP (0x7000)          /**< ipsec Des + MD Groups  */
#define IPSEC_AES_GROUP (0x8000)          /**< ipsec aes + md groups  */
#define TLS_GROUP       (0x9000)          /**< SSL/TLS                */
#define KEA_GROUP       (0xa000)          /**< Kasumi                 */
#define CRC_GROUP       (0xb000)          /**< CRC Group              */
#define SNOW_GROUP      (0xc000)          /**< SNOW-3G Group          */

/* Extracts the request type from an OpId */
#define DESC_TYPE_MASK (0xFF00)
/* Extracts the request group from an OpId */
#define DESC_NUM_MASK (0x00FF)




/**
 * Common request block for all request headers
 * This macro contains the common portion of all request blocks
 * It's composed as follows:
 *   opId -            Type of operation requested
 *   scatterBufs -     Map of scattered buffers, returned from MarkScatterBuffer()
 *   notifyFlags -     if bits set, notify pointers are signal values to be
 *                     sent upon request completion
 *   channel -         Zero if auto-allocated, >= 1 if using static channel
 *   notify -          Handler for normal completion. Signal value to use
 *                     for completion if in usermode
 *   pNotifyCtx -      Context for handler. Nothing for usermode.
 *   notify_on_error - Handler for error completion. Signal value to use
 *                     for error indication if in usermode
 *   ctxNotifyOnErr -  returned device context if error. Nothing for usermode.
 *   status -          Driver completion status
 *   nextReq -         Next request in list, if any
 */

#define COMMON_REQ_PREAMBLE \
    unsigned long                 opId; \
    unsigned char                 scatterBufs; \
    unsigned char                 notifyFlags; \
    unsigned char                 reserved; \
    unsigned char                 channel; \
    PSEC2_NOTIFY_ROUTINE          notify; \
    PSEC2_NOTIFY_CTX              pNotifyCtx; \
    PSEC2_NOTIFY_ON_ERROR_ROUTINE notify_on_error; \
    SEC2_NOTIFY_ON_ERROR_CTX      ctxNotifyOnErr; \
    int                           status; \
    void                         *nextReq;



/**
 * This gets used by driver functions that need access to
 * just the request header without knowing the remaining
 * request-type-specific content
 */
typedef struct { COMMON_REQ_PREAMBLE } GENERIC_REQ;


/* Bits for use with notifyFlags in the request header                        */
/* NOTIFY_IS_PID and NOTIFY_ERROR_IS_PID are no longer used in 2.0 and after, */
/* NOTIFY_SIGNAL is used to tell an RTOS to use a signal if needed            */
#define NOTIFY_SIGNAL       (0x04) /* notify/notify_on_error contain user-defined */
                                   /* signal values for completion notification   */



    /**
     * RNG_REQ - Random-Number generator request block
     * rngBytes - number of bytes to return
     * rngData  - pointer to user's random data buffer
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  rngBytes;
        unsigned char *rngData;
    } RNG_REQ;

#define NUM_RNGA_DESC (1)
#define DPD_RNG_GROUP (RNG_GROUP | 0x0000)

    /* Get random data from the RNG */
#define DPD_RNG_GETRN (DPD_RNG_GROUP + 0)






    /**
     * DES_CBC_CRYPT_REQ - DES request with context load
     * These are basically CBC (cipher-block-chain) operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  inIvBytes;   /**< 0 or 8 bytes                 */
        unsigned char *inIvData;
        unsigned long  keyBytes;    /**< 8, 16, or 24 bytes           */
        unsigned char *keyData;
        unsigned long  inBytes;     /**< multiple of 8 bytes          */
        unsigned char *inData;
        unsigned char *outData;     /**< output length = input length */
        unsigned long  outIvBytes;  /**< 0 or 8 bytes                 */
        unsigned char *outIvData;
    } DES_CBC_CRYPT_REQ;

    /* typedef the old name for legacy compatibility */
    typedef DES_CBC_CRYPT_REQ DES_LOADCTX_CRYPT_REQ;

#define NUM_DES_CBC_DESC         (12)
#define DPD_DES_CBC_GROUP        (DES_GROUP | 0x0500)

#define DPD_SDES_CBC_CTX_ENCRYPT    (DPD_DES_CBC_GROUP + 0)
#define DPD_SDES_CBC_CTX_DECRYPT    (DPD_DES_CBC_GROUP + 1)
#define DPD_TDES_CBC_CTX_ENCRYPT    (DPD_DES_CBC_GROUP + 2)
#define DPD_TDES_CBC_CTX_DECRYPT    (DPD_DES_CBC_GROUP + 3)
#define DPD_SDES_CFB_64_CTX_ENCRYPT (DPD_DES_CBC_GROUP + 4)
#define DPD_SDES_CFB_64_CTX_DECRYPT (DPD_DES_CBC_GROUP + 5)
#define DPD_TDES_CFB_64_CTX_ENCRYPT (DPD_DES_CBC_GROUP + 6)
#define DPD_TDES_CFB_64_CTX_DECRYPT (DPD_DES_CBC_GROUP + 7)
#define DPD_SDES_OFB_64_CTX_ENCRYPT (DPD_DES_CBC_GROUP + 8)
#define DPD_SDES_OFB_64_CTX_DECRYPT (DPD_DES_CBC_GROUP + 9)
#define DPD_TDES_OFB_64_CTX_ENCRYPT (DPD_DES_CBC_GROUP + 10)
#define DPD_TDES_OFB_64_CTX_DECRYPT (DPD_DES_CBC_GROUP + 11)




    /**
     * DES_CRYPT_REQ - non-context DES cipher operation
     * These are ECB-only operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  keyBytes;  /* 8, 16, or 24 bytes */
        unsigned char *keyData;
        unsigned long  inBytes;  /* multiple of 8 bytes */
        unsigned char *inData;
        unsigned char *outData;  /* output length = input length */
    } DES_CRYPT_REQ;

#define NUM_DES_DESC         (4)
#define DPD_DES_ECB_GROUP    (DES_GROUP | 0x0600)

#define DPD_SDES_ECB_ENCRYPT (DPD_DES_ECB_GROUP + 0)
#define DPD_SDES_ECB_DECRYPT (DPD_DES_ECB_GROUP + 1)
#define DPD_TDES_ECB_ENCRYPT (DPD_DES_ECB_GROUP + 2)
#define DPD_TDES_ECB_DECRYPT (DPD_DES_ECB_GROUP + 3)




    /*
     * AESA_CRYPT_REQ - AES ciphering operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  keyBytes;     /**< 8, 16, or 24 bytes          */
        unsigned char *keyData;
        unsigned long  inIvBytes;    /**< 0 or 16 bytes               */
        unsigned char *inIvData;
        unsigned long  inBytes;      /**< multiple of 8 bytes         */
        unsigned char *inData;
        unsigned char *outData;      /**< output length = input length*/
        unsigned long  outCtxBytes;  /**< 0 or 16 bytes               */
        unsigned char *outCtxData;
    } AESA_CRYPT_REQ;

#define NUM_AESA_CRYPT_DESC            (23)
#define DPD_AESA_CRYPT_GROUP           (AES_GROUP | 0x0000)

#define DPD_AESA_CBC_ENCRYPT_CRYPT     (DPD_AESA_CRYPT_GROUP + 0)
#define DPD_AESA_CBC_DECRYPT_CRYPT     (DPD_AESA_CRYPT_GROUP + 1)
#define DPD_AESA_CBC_DECRYPT_CRYPT_RDK (DPD_AESA_CRYPT_GROUP + 2)
#define DPD_AESA_ECB_ENCRYPT_CRYPT     (DPD_AESA_CRYPT_GROUP + 3)
#define DPD_AESA_ECB_DECRYPT_CRYPT     (DPD_AESA_CRYPT_GROUP + 4)
#define DPD_AESA_ECB_DECRYPT_CRYPT_RDK (DPD_AESA_CRYPT_GROUP + 5)
#define DPD_AESA_CTR_CRYPT             (DPD_AESA_CRYPT_GROUP + 6)
#define DPD_AESA_CTR_HMAC              (DPD_AESA_CRYPT_GROUP + 7)
#define DPD_AESA_CBC_RBP_ENCRYPT       (DPD_AESA_CRYPT_GROUP + 8)
#define DPD_AESA_CBC_RBP_DECRYPT       (DPD_AESA_CRYPT_GROUP + 9)
#define DPD_AESA_OFB_ENCRYPT           (DPD_AESA_CRYPT_GROUP + 10)
#define DPD_AESA_OFB_DECRYPT           (DPD_AESA_CRYPT_GROUP + 11)
#define DPD_AESA_CFB128_ENCRYPT        (DPD_AESA_CRYPT_GROUP + 12)
#define DPD_AESA_CFB128_DECRYPT        (DPD_AESA_CRYPT_GROUP + 13)
#define DPD_AESA_CCM_ENCRYPT           (DPD_AESA_CRYPT_GROUP + 14)
#define DPD_AESA_CCM_DECRYPT           (DPD_AESA_CRYPT_GROUP + 15)
#define DPD_AESA_CCM_FINAL_ENCRYPT     (DPD_AESA_CRYPT_GROUP + 16)
#define DPD_AESA_CCM_FINAL_DECRYPT     (DPD_AESA_CRYPT_GROUP + 17)
#define DPD_AESA_CCM_FINAL_DECRYPT_CMP (DPD_AESA_CRYPT_GROUP + 18)
#define DPD_AESA_LRW_ENCRYPT           (DPD_AESA_CRYPT_GROUP + 19)
#define DPD_AESA_LRW_NO_TWEAK_ENCRYPT  (DPD_AESA_CRYPT_GROUP + 20)
#define DPD_AESA_LRW_DECRYPT           (DPD_AESA_CRYPT_GROUP + 21)
#define DPD_AESA_LRW_NO_TWEAK_DECRYPT  (DPD_AESA_CRYPT_GROUP + 22)




    /**
     * RAID_XOR_REQ - specialty AESA XOR operations
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned char *inDataA;  /**< input block A */
        unsigned char *inDataB;  /**< input block B */
        unsigned char *inDataC;  /**< input block C */
        unsigned char *inDataD;  /**< input block D */
        unsigned char *inDataE;  /**< input block E */
        unsigned char *inDataF;  /**< input block F */
        unsigned char *outData;  /**< output buffer */
        unsigned long  opSize;   /**< size to process */
    } RAID_XOR_REQ;

#define NUM_RAID_XOR_DESC              (1)
#define DPD_RAID_XOR_GROUP             (AES_GROUP | 0x0200)

#define DPD_RAID_XOR                   (DPD_RAID_XOR_GROUP + 0)




/*
 * AESA_MAC_REQ - AES authenticity operations
 */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  keyBytes;
        unsigned char *keyData;
        unsigned long  inCtxBytes;
        unsigned char *inCtxData;
        unsigned long  outCtxBytes;
        unsigned char *outCtxData;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned long  cmpBytes;
        unsigned char *cmpIn;        /* ICV compare */
        unsigned char *cmpOut;       /* Digest out */
    } AESA_MAC_REQ;

#define NUM_AESA_MAC_DESC              (4)
#define DPD_AESA_MAC_GROUP             (AES_GROUP | 0x0100)

#define DPD_AESA_CMAC                  (DPD_AESA_MAC_GROUP + 0)
#define DPD_AESA_CMAC_CMP              (DPD_AESA_MAC_GROUP + 1)
#define DPD_AESA_XCBCMAC               (DPD_AESA_MAC_GROUP + 2)
#define DPD_AESA_XCBCMAC_CMP           (DPD_AESA_MAC_GROUP + 3)




/*
 * AESA_GCM_REQ
 */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  gcmCtxBytes;
        unsigned char *gcmCtxData;
        unsigned long  AADBytes;
        unsigned char *AAD;
        unsigned char  ivBytes;
        unsigned char *ivIn;
        unsigned char *ivOut;
        unsigned long  keyBytes;
        unsigned long  saltBytes;
        unsigned char *keyData;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned long  cmpBytes;
        unsigned long  outBytes;
        unsigned char *outData;
        unsigned long  crcBytes;
    } AESA_GCM_REQ;

#define NUM_AESA_GCM_DESC              (5)
#define DPD_AESA_GCM_GROUP             (AES_GROUP | 0x0300)

#define DPD_AESA_GCM_ENCRYPT           (DPD_AESA_GCM_GROUP + 0)
#define DPD_AESA_GCM_DECRYPT           (DPD_AESA_GCM_GROUP + 1)
#define DPD_AESA_GCM_FINAL_ENCRYPT     (DPD_AESA_GCM_GROUP + 2)
#define DPD_AESA_GCM_FINAL_DECRYPT     (DPD_AESA_GCM_GROUP + 3)
#define DPD_AESA_GCM_FINAL_DECRYPT_CMP (DPD_AESA_GCM_GROUP + 4)





    /**
     * KEA_CRYPT_REQ - Kasumi cipher operations
     * Not present in 2.0
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  ivBytes;
        unsigned char *ivData;
        unsigned long  keyBytes;
        unsigned char *keyData;
        unsigned long  inBytes;    /**< multiple of 8 bytes          */
        unsigned char *inData;
        unsigned long  outBytes;   /**< must be zero if f9 only      */
        unsigned char *outData;    /**< output length = input length */
        /** if f9_CMP, this becomes compare value */
        unsigned long  outIvBytes;
        unsigned char *outIvData;
        unsigned long  ctxBytes;
        unsigned char *ctxData;    /**< f9 integrity digest/context  */
    } KEA_CRYPT_REQ;

#define NUM_KEA_CRYPT_DESC                     (10)
#define DPD_KEA_CRYPT_GROUP                    (KEA_GROUP | 0x0000)

    /* Kasumi request types */
#define DPD_KEA_f8_CIPHER_INIT                 (DPD_KEA_CRYPT_GROUP + 0)
#define DPD_KEA_f8_CIPHER                      (DPD_KEA_CRYPT_GROUP + 1)
#define DPD_KEA_f9_CIPHER_INIT                 (DPD_KEA_CRYPT_GROUP + 2)
#define DPD_KEA_f9_CIPHER                      (DPD_KEA_CRYPT_GROUP + 3)
#define DPD_KEA_f9_CIPHER_FINAL                (DPD_KEA_CRYPT_GROUP + 4)
#define DPD_KEA_f9_CIPHER_INIT_FINAL           (DPD_KEA_CRYPT_GROUP + 5)
#define DPD_KEA_GSM_A53_CIPHER                 (DPD_KEA_CRYPT_GROUP + 6)
#define DPD_KEA_EDGE_A53_CIPHER                (DPD_KEA_CRYPT_GROUP + 7)
#define DPD_KEA_f9_CIPHER_FINAL_CMP            (DPD_KEA_CRYPT_GROUP + 8)
#define DPD_KEA_f9_CIPHER_INIT_FINAL_CMP       (DPD_KEA_CRYPT_GROUP + 9)



    /**
     * SNOW3G_CRYPT_REQ - SNOW for 3G cipher operations
     * Present >= 3.1
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  ivBytes;    /**< Combination bearer/count/dir */
        unsigned char *ivData;
        unsigned long  keyBytes;   /**< always 16 for UEA2/UIA2 */
        unsigned char *keyData;
        unsigned long  inBytes;    /**< multiple of 8 bytes          */
        unsigned char *inData;
        unsigned long  outBytes;   /**< zero for UIA2 only      */
        unsigned char *outData;    /**< output length = input length */
        unsigned long  outIvBytes;
        unsigned char *outIvData;
        unsigned long  ctxBytes;   /**< = 8 for UIA2, lower 4 bytes */
        unsigned char *ctxData;    /**< UIA2 integrity digest/context  */
    } SNOW3G_CRYPT_REQ;

#define NUM_SNOW3G_CRYPT_DESC                  (8)
#define DPD_SNOW3G_CRYPT_GROUP                 (SNOW_GROUP | 0x0000)

#define DPD_SNOW3G_UEA2_INIT                   (DPD_SNOW3G_CRYPT_GROUP + 0)
#define DPD_SNOW3G_UEA2                        (DPD_SNOW3G_CRYPT_GROUP + 1)
#define DPD_SNOW3G_UIA2_INIT                   (DPD_SNOW3G_CRYPT_GROUP + 2)
#define DPD_SNOW3G_UIA2                        (DPD_SNOW3G_CRYPT_GROUP + 3)
#define DPD_SNOW3G_UIA2_FINAL                  (DPD_SNOW3G_CRYPT_GROUP + 4)
#define DPD_SNOW3G_UIA2_INIT_FINAL             (DPD_SNOW3G_CRYPT_GROUP + 5)
#define DPD_SNOW3G_UIA2_FINAL_CMP              (DPD_SNOW3G_CRYPT_GROUP + 8)
#define DPD_SNOW3G_UIA2_INIT_FINAL_CMP         (DPD_SNOW3G_CRYPT_GROUP + 9)



    /**
     * ARC4_LOADCTX_CRYPT_REQ - ARC4 cipher operation with context load
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  inCtxBytes;  /**< 257 bytes */
        unsigned char *inCtxData;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned char *outData;         /**< output length = input length */
        unsigned long  outCtxBytes;     /**< 257 bytes */
        unsigned char *outCtxData;
    } ARC4_LOADCTX_CRYPT_REQ;

#define NUM_RC4_LOADCTX_UNLOADCTX_DESC  (1)
#define DPD_RC4_LDCTX_CRYPT_ULCTX_GROUP (ARC4_GROUP | 0x0400)

#define DPD_RC4_LDCTX_CRYPT_ULCTX       (DPD_RC4_LDCTX_CRYPT_ULCTX_GROUP + 0)




    /**
     * ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ - ARC4 operation with key load
     *                                    and context save
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  keyBytes;
        unsigned char *keyData;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned char *outData;  /* output length = input length */
        unsigned long  outCtxBytes;  /* 257 bytes */
        unsigned char *outCtxData;
    } ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ;

#define NUM_RC4_LOADKEY_UNLOADCTX_DESC  (1)
#define DPD_RC4_LDKEY_CRYPT_ULCTX_GROUP (ARC4_GROUP | 0x0500)

#define DPD_RC4_LDKEY_CRYPT_ULCTX       (DPD_RC4_LDKEY_CRYPT_ULCTX_GROUP + 0)




    /**
     * HASH_REQ - combined SHA/MD5 hash operations with context loads
     *            and saves
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  ctxBytes;
        unsigned char *ctxData;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned long  outBytes;  /**< length is fixed by algorithm   */
        unsigned char *outData;
        unsigned char *cmpData;   /**< digest compare value,
                                        same len as outData           */
    } HASH_REQ;


#define NUM_MDHA_DESC                        (30)
#define DPD_HASH_LDCTX_HASH_ULCTX_GROUP      (MD_GROUP | 0x0400)

#define DPD_SHA256_LDCTX_HASH_ULCTX          (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 0)
#define DPD_MD5_LDCTX_HASH_ULCTX             (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 1)
#define DPD_SHA_LDCTX_HASH_ULCTX             (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 2)
#define DPD_SHA256_LDCTX_IDGS_HASH_ULCTX     (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 3)
#define DPD_MD5_LDCTX_IDGS_HASH_ULCTX        (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 4)
#define DPD_SHA_LDCTX_IDGS_HASH_ULCTX        (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 5)
#define DPD_SHA256_CONT_HASH_ULCTX           (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 6)
#define DPD_MD5_CONT_HASH_ULCTX              (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 7)
#define DPD_SHA_CONT_HASH_ULCTX              (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 8)
#define DPD_SHA224_LDCTX_HASH_ULCTX          (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 9)
#define DPD_SHA224_LDCTX_IDGS_HASH_ULCTX     (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 10)
#define DPD_SHA224_CONT_HASH_ULCTX           (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 12)
#define DPD_SHA256_LDCTX_HASH_ULCTX_CMP      (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 13)
#define DPD_MD5_LDCTX_HASH_ULCTX_CMP         (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 14)
#define DPD_SHA_LDCTX_HASH_ULCTX_CMP         (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 15)
#define DPD_SHA256_LDCTX_IDGS_HASH_ULCTX_CMP (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 16)
#define DPD_MD5_LDCTX_IDGS_HASH_ULCTX_CMP    (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 17)
#define DPD_SHA_LDCTX_IDGS_HASH_ULCTX_CMP    (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 18)
#define DPD_SHA224_LDCTX_HASH_ULCTX_CMP      (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 19)
#define DPD_SHA224_LDCTX_IDGS_HASH_ULCTX_CMP (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 20)
#define DPD_SHA384_LDCTX_HASH_ULCTX          (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 21)
#define DPD_SHA384_LDCTX_IDGS_HASH_ULCTX     (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 22)
#define DPD_SHA384_CONT_HASH_ULCTX           (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 23)
#define DPD_SHA384_LDCTX_HASH_ULCTX_CMP      (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 24)
#define DPD_SHA384_LDCTX_IDGS_HASH_ULCTX_CMP (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 25)
#define DPD_SHA512_LDCTX_HASH_ULCTX          (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 26)
#define DPD_SHA512_LDCTX_IDGS_HASH_ULCTX     (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 27)
#define DPD_SHA512_CONT_HASH_ULCTX           (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 28)
#define DPD_SHA512_LDCTX_HASH_ULCTX_CMP      (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 29)
#define DPD_SHA512_LDCTX_IDGS_HASH_ULCTX_CMP (DPD_HASH_LDCTX_HASH_ULCTX_GROUP + 30)




#define NUM_MDHA_PAD_DESC                        (24)
#define DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP      (MD_GROUP | 0x0500)

#define DPD_SHA256_LDCTX_HASH_PAD_ULCTX          (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 0)
#define DPD_MD5_LDCTX_HASH_PAD_ULCTX             (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 1)
#define DPD_SHA_LDCTX_HASH_PAD_ULCTX             (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 2)
#define DPD_SHA256_LDCTX_IDGS_HASH_PAD_ULCTX     (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 3)
#define DPD_MD5_LDCTX_IDGS_HASH_PAD_ULCTX        (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 4)
#define DPD_SHA_LDCTX_IDGS_HASH_PAD_ULCTX        (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 5)
#define DPD_SHA224_LDCTX_HASH_PAD_ULCTX          (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 6)
#define DPD_SHA224_LDCTX_IDGS_HASH_PAD_ULCTX     (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 7)
#define DPD_SHA256_LDCTX_HASH_PAD_ULCTX_CMP      (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 8)
#define DPD_MD5_LDCTX_HASH_PAD_ULCTX_CMP         (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 9)
#define DPD_SHA_LDCTX_HASH_PAD_ULCTX_CMP         (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 10)
#define DPD_SHA256_LDCTX_IDGS_HASH_PAD_ULCTX_CMP (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 11)
#define DPD_MD5_LDCTX_IDGS_HASH_PAD_ULCTX_CMP    (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 12)
#define DPD_SHA_LDCTX_IDGS_HASH_PAD_ULCTX_CMP    (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 13)
#define DPD_SHA224_LDCTX_HASH_PAD_ULCTX_CMP      (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 14)
#define DPD_SHA224_LDCTX_IDGS_HASH_PAD_ULCTX_CMP (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 15)
#define DPD_SHA384_LDCTX_HASH_PAD_ULCTX          (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 16)
#define DPD_SHA384_LDCTX_IDGS_HASH_PAD_ULCTX     (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 17)
#define DPD_SHA384_LDCTX_HASH_PAD_ULCTX_CMP      (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 18)
#define DPD_SHA384_LDCTX_IDGS_HASH_PAD_ULCTX_CMP (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 19)
#define DPD_SHA512_LDCTX_HASH_PAD_ULCTX          (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 20)
#define DPD_SHA512_LDCTX_IDGS_HASH_PAD_ULCTX     (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 21)
#define DPD_SHA512_LDCTX_HASH_PAD_ULCTX_CMP      (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 22)
#define DPD_SHA512_LDCTX_IDGS_HASH_PAD_ULCTX_CMP (DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP + 23)



    /**
     * HMAC_PAD_REQ - Hash-message authentication code
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  keyBytes;
        unsigned char *keyData;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned long  outBytes;  /**< length is fixed by algorithm   */
        unsigned char *outData;
        unsigned char *cmpData;   /**< digest compare value,
                                        same len as outData           */
    } HMAC_PAD_REQ;

#define NUM_HMAC_PAD_DESC                   (24)
#define DPD_HASH_LDCTX_HMAC_ULCTX_GROUP     (MD_GROUP | 0x0A00)

#define DPD_SHA256_LDCTX_HMAC_ULCTX         (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 0)
#define DPD_MD5_LDCTX_HMAC_ULCTX            (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 1)
#define DPD_SHA_LDCTX_HMAC_ULCTX            (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 2)
#define DPD_SHA256_LDCTX_HMAC_PAD_ULCTX     (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 3)
#define DPD_MD5_LDCTX_HMAC_PAD_ULCTX        (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 4)
#define DPD_SHA_LDCTX_HMAC_PAD_ULCTX        (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 5)
#define DPD_SHA224_LDCTX_HMAC_ULCTX         (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 6)
#define DPD_SHA224_LDCTX_HMAC_PAD_ULCTX     (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 7)
#define DPD_SHA256_LDCTX_HMAC_ULCTX_CMP     (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 8)
#define DPD_MD5_LDCTX_HMAC_ULCTX_CMP        (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 9)
#define DPD_SHA_LDCTX_HMAC_ULCTX_CMP        (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 10)
#define DPD_SHA256_LDCTX_HMAC_PAD_ULCTX_CMP (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 11)
#define DPD_MD5_LDCTX_HMAC_PAD_ULCTX_CMP    (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 12)
#define DPD_SHA_LDCTX_HMAC_PAD_ULCTX_CMP    (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 13)
#define DPD_SHA224_LDCTX_HMAC_ULCTX_CMP     (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 14)
#define DPD_SHA224_LDCTX_HMAC_PAD_ULCTX_CMP (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 15)
#define DPD_SHA384_LDCTX_HMAC_ULCTX         (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 16)
#define DPD_SHA384_LDCTX_HMAC_PAD_ULCTX     (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 17)
#define DPD_SHA384_LDCTX_HMAC_ULCTX_CMP     (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 18)
#define DPD_SHA384_LDCTX_HMAC_PAD_ULCTX_CMP (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 19)
#define DPD_SHA512_LDCTX_HMAC_ULCTX         (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 20)
#define DPD_SHA512_LDCTX_HMAC_PAD_ULCTX     (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 21)
#define DPD_SHA512_LDCTX_HMAC_ULCTX_CMP     (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 22)
#define DPD_SHA512_LDCTX_HMAC_PAD_ULCTX_CMP (DPD_HASH_LDCTX_HMAC_ULCTX_GROUP + 23)



    /**
     * MOD_EXP_REQ - integer public key (modular expo) operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  aDataBytes;
        unsigned char *aData;
        unsigned long  expBytes;
        unsigned char *expData;
        unsigned long  modBytes;
        unsigned char *modData;
        unsigned long  outBytes;
        unsigned char *outData;
    } MOD_EXP_REQ;

#define NUM_MM_EXP_DESC                  (2)
#define DPD_MM_LDCTX_EXP_ULCTX_GROUP     (PK_GROUP | 0x0100)

#define DPD_MM_LDCTX_EXP_ULCTX           (DPD_MM_LDCTX_EXP_ULCTX_GROUP + 0)
#define DPD_MM_LDCTX_EXP_TEQ_ULCTX       (DPD_MM_LDCTX_EXP_ULCTX_GROUP + 1)



    /**
     * MOD_SS_EXP_REQ - single-stage RSA operations
     *
     */
    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  expBytes;
        unsigned char *expData;
        unsigned long  modBytes;
        unsigned char *modData;
        unsigned long  aDataBytes;
        unsigned char *aData;
        unsigned long  bDataBytes;
        unsigned char *bData;
    } MOD_SS_EXP_REQ;

#define NUM_MM_SS_EXP_DESC      (2)
#define DPD_MM_SS_EXP_GROUP     (PK_GROUP | 0x0b00)

#define DPD_MM_SS_RSA_EXP       (DPD_MM_SS_EXP_GROUP + 0)
#define DPD_MM_SS_RSA_TEQ_EXP   (DPD_MM_SS_EXP_GROUP + 1)



    /**
     * MOD_R2MODN_REQ - Modular R2 operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  modBytes;
        unsigned char *modData;
        unsigned long  outBytes;
        unsigned char *outData;
    } MOD_R2MODN_REQ;

#define NUM_MOD_R2MODN_DESC             (2)
#define DPD_MM_LDCTX_R2MODN_ULCTX_GROUP (PK_GROUP | 0x0200)

#define DPD_MM_LDCTX_R2MODN_ULCTX       (DPD_MM_LDCTX_R2MODN_ULCTX_GROUP + 0)
#define DPD_F2M_R2                      (DPD_MM_LDCTX_R2MODN_ULCTX_GROUP + 1)




    /**
     * MOD_RRMODP_REQ - montgomery precomputation operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  nBytes;
        unsigned long  pBytes;
        unsigned char *pData;
        unsigned long  outBytes;
        unsigned char *outData;
    } MOD_RRMODP_REQ;

#define NUM_MM_RRMODP_DESC              (1)
#define DPD_MM_LDCTX_RRMODP_ULCTX_GROUP (PK_GROUP | 0x0300)

#define DPD_MM_LDCTX_RRMODP_ULCTX       (DPD_MM_LDCTX_RRMODP_ULCTX_GROUP + 0)




    /**
     * MOD_INV_REQ - Modular Inversion
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  nBytes;
        unsigned char *nData;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned long  outBytes;
        unsigned char *outData;
    } MOD_INV_REQ;

#define NUM_MM_MOD_INV_DESC        (1)
#define DPD_MM_MOD_INV_ULCTX_GROUP (PK_GROUP | 0x0500)

#define DPD_MM_MOD_INV_ULCTX       (DPD_MM_MOD_INV_ULCTX_GROUP + 0)



    /**
     * MOD_2OP_REQ
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  bDataBytes;
        unsigned char *bData;
        unsigned long  aDataBytes;
        unsigned char *aData;
        unsigned long  modBytes;
        unsigned char *modData;
        unsigned long  outBytes;
        unsigned char *outData;
    } MOD_2OP_REQ;

#define NUM_MOD_2OP_DESC              (52)
#define DPD_MOD_LDCTX_2OP_ULCTX_GROUP (PK_GROUP | 0x0400)

#define DPD_MOD_LDCTX_MUL1_ULCTX      (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 0)
#define DPD_MOD_LDCTX_MUL2_ULCTX      (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 1)
#define DPD_MOD_LDCTX_ADD_ULCTX       (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 2)
#define DPD_MOD_LDCTX_SUB_ULCTX       (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 3)

    /* equivalents for obsolete names */
#define DPD_MM_LDCTX_MUL1_ULCTX DPD_MOD_LDCTX_MUL1_ULCTX
#define DPD_MM_LDCTX_MUL2_ULCTX DPD_MOD_LDCTX_MUL2_ULCTX
#define DPD_MM_LDCTX_ADD_ULCTX  DPD_MOD_LDCTX_ADD_ULCTX
#define DPD_MM_LDCTX_SUB_ULCTX  DPD_MOD_LDCTX_SUB_ULCTX

    /* These constants will likely be removed in a future release in favor */
    /* of a different internal register allocation scheme                  */
#define DPD_POLY_LDCTX_A0_B0_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 4)
#define DPD_POLY_LDCTX_A0_B0_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 5)
#define DPD_POLY_LDCTX_A0_B0_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 6)
#define DPD_POLY_LDCTX_A1_B0_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 7)
#define DPD_POLY_LDCTX_A1_B0_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 8)
#define DPD_POLY_LDCTX_A1_B0_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 9)
#define DPD_POLY_LDCTX_A2_B0_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 10)
#define DPD_POLY_LDCTX_A2_B0_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 11)
#define DPD_POLY_LDCTX_A2_B0_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 12)
#define DPD_POLY_LDCTX_A3_B0_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 13)
#define DPD_POLY_LDCTX_A3_B0_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 14)
#define DPD_POLY_LDCTX_A3_B0_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 15)
#define DPD_POLY_LDCTX_A0_B1_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 16)
#define DPD_POLY_LDCTX_A0_B1_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 17)
#define DPD_POLY_LDCTX_A0_B1_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 18)
#define DPD_POLY_LDCTX_A1_B1_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 19)
#define DPD_POLY_LDCTX_A1_B1_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 20)
#define DPD_POLY_LDCTX_A1_B1_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 21)
#define DPD_POLY_LDCTX_A2_B1_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 22)
#define DPD_POLY_LDCTX_A2_B1_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 23)
#define DPD_POLY_LDCTX_A2_B1_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 24)
#define DPD_POLY_LDCTX_A3_B1_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 25)
#define DPD_POLY_LDCTX_A3_B1_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 26)
#define DPD_POLY_LDCTX_A3_B1_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 27)
#define DPD_POLY_LDCTX_A0_B2_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 28)
#define DPD_POLY_LDCTX_A0_B2_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 29)
#define DPD_POLY_LDCTX_A0_B2_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 30)
#define DPD_POLY_LDCTX_A1_B2_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 31)
#define DPD_POLY_LDCTX_A1_B2_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 32)
#define DPD_POLY_LDCTX_A1_B2_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 33)
#define DPD_POLY_LDCTX_A2_B2_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 34)
#define DPD_POLY_LDCTX_A2_B2_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 35)
#define DPD_POLY_LDCTX_A2_B2_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 36)
#define DPD_POLY_LDCTX_A3_B2_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 37)
#define DPD_POLY_LDCTX_A3_B2_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 38)
#define DPD_POLY_LDCTX_A3_B2_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 39)
#define DPD_POLY_LDCTX_A0_B3_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 40)
#define DPD_POLY_LDCTX_A0_B3_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 41)
#define DPD_POLY_LDCTX_A0_B3_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 42)
#define DPD_POLY_LDCTX_A1_B3_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 43)
#define DPD_POLY_LDCTX_A1_B3_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 44)
#define DPD_POLY_LDCTX_A1_B3_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 45)
#define DPD_POLY_LDCTX_A2_B3_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 46)
#define DPD_POLY_LDCTX_A2_B3_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 47)
#define DPD_POLY_LDCTX_A2_B3_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 48)
#define DPD_POLY_LDCTX_A3_B3_MUL1_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 49)
#define DPD_POLY_LDCTX_A3_B3_MUL2_ULCTX (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 50)
#define DPD_POLY_LDCTX_A3_B3_ADD_ULCTX  (DPD_MOD_LDCTX_2OP_ULCTX_GROUP + 51)





    /**
     * ECC_POINT_REQ - elliptic curve point processing
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  nDataBytes;
        unsigned char *nData;
        unsigned long  eDataBytes;
        unsigned char *eData;
        unsigned long  buildDataBytes;
        unsigned char *buildData;
        unsigned long  b1DataBytes;
        unsigned char *b1Data;
        unsigned long  b2DataBytes;
        unsigned char *b2Data;
        unsigned long  b3DataBytes;
        unsigned char *b3Data;
    } ECC_POINT_REQ;

#define NUM_EC_POINT_DESC              (8)
#define DPD_EC_LDCTX_kP_ULCTX_GROUP    (PK_GROUP | 0x0800)

#define DPD_EC_FP_AFF_PT_MULT          (DPD_EC_LDCTX_kP_ULCTX_GROUP + 0)
#define DPD_EC_FP_PROJ_PT_MULT         (DPD_EC_LDCTX_kP_ULCTX_GROUP + 1)
#define DPD_EC_F2M_AFF_PT_MULT         (DPD_EC_LDCTX_kP_ULCTX_GROUP + 2)
#define DPD_EC_F2M_PROJ_PT_MULT        (DPD_EC_LDCTX_kP_ULCTX_GROUP + 3)
#define DPD_EC_FP_LDCTX_ADD_ULCTX      (DPD_EC_LDCTX_kP_ULCTX_GROUP + 4)
#define DPD_EC_FP_LDCTX_DOUBLE_ULCTX   (DPD_EC_LDCTX_kP_ULCTX_GROUP + 5)
#define DPD_EC_F2M_LDCTX_ADD_ULCTX     (DPD_EC_LDCTX_kP_ULCTX_GROUP + 6)
#define DPD_EC_F2M_LDCTX_DOUBLE_ULCTX  (DPD_EC_LDCTX_kP_ULCTX_GROUP + 7)

    /* legacy name definitions */
#define DPD_EC_FP_AFF_LDCTX_kP_ULCTX   DPD_EC_FP_AFF_PT_MULT
#define DPD_EC_FP_PROJ_LDCTX_kP_ULCTX  DPD_EC_FP_PROJ_PT_MULT
#define DPD_EC_F2M_AFF_LDCTX_kP_ULCTX  DPD_EC_F2M_AFF_PT_MULT
#define DPD_EC_F2M_PROJ_LDCTX_kP_ULCTX DPD_EC_F2M_PROJ_PT_MULT



    /**
     * F2M_INV_REQ - Binary Inversion
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  nBytes;
        unsigned char *nData;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned long  outBytes;
        unsigned char *outData;
    } F2M_INV_REQ;

#define NUM_F2M_INV_DESC        (1)
#define DPD_F2M_INV_ULCTX_GROUP (PK_GROUP | 0x0600)

#define DPD_F2M_INV_ULCTX       (DPD_F2M_INV_ULCTX_GROUP + 0)




    /**
     * ECC_2OP_REQ - F2M elliptic curve operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  bDataBytes;
        unsigned char *bData;
        unsigned long  aDataBytes;
        unsigned char *aData;
        unsigned long  modBytes;
        unsigned char *modData;
        unsigned long  outBytes;
        unsigned char *outData;
    } ECC_2OP_REQ;

#define NUM_EC_2OP_DESC             (3)
#define DPD_EC_2OP_GROUP            (PK_GROUP | 0x0900)

#define DPD_F2M_LDCTX_MUL1_ULCTX    (DPD_EC_2OP_GROUP + 0)
#define DPD_F2M_LDCTX_MUL2_ULCTX    (DPD_EC_2OP_GROUP + 1)
#define DPD_F2M_LDCTX_ADD_ULCTX     (DPD_EC_2OP_GROUP + 2)



    /**
     * ECC_SPKBUILD_REQ - elliptic curve request builder
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  a0DataBytes;
        unsigned char *a0Data;
        unsigned long  a1DataBytes;
        unsigned char *a1Data;
        unsigned long  a2DataBytes;
        unsigned char *a2Data;
        unsigned long  a3DataBytes;
        unsigned char *a3Data;
        unsigned long  b0DataBytes;
        unsigned char *b0Data;
        unsigned long  b1DataBytes;
        unsigned char *b1Data;
        unsigned long  buildDataBytes;
        unsigned char *buildData;
    } ECC_SPKBUILD_REQ;

#define NUM_EC_SPKBUILD_DESC  (1)
#define DPD_EC_SPKBUILD_GROUP (PK_GROUP | 0x0c00)

#define DPD_EC_SPKBUILD       (DPD_EC_SPKBUILD_GROUP + 0)




    /**
     * ECC_PTADD_DBL_REQ - elliptic curve point add
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  modBytes;
        unsigned char *modData;
        unsigned long  buildDataBytes;
        unsigned char *buildData;
        unsigned long  b2InDataBytes;
        unsigned char *b2InData;
        unsigned long  b3InDataBytes;
        unsigned char *b3InData;
        unsigned long  b1DataBytes;
        unsigned char *b1Data;
        unsigned long  b2DataBytes;
        unsigned char *b2Data;
        unsigned long  b3DataBytes;
        unsigned char *b3Data;
    } ECC_PTADD_DBL_REQ;

#define NUM_EC_PTADD_DBL_DESC  (4)
#define DPD_EC_PTADD_DBL_GROUP (PK_GROUP | 0x0d00)

#define DPD_EC_FPADD           (DPD_EC_PTADD_DBL_GROUP + 0)
#define DPD_EC_FPDBL           (DPD_EC_PTADD_DBL_GROUP + 1)
#define DPD_EC_F2MADD          (DPD_EC_PTADD_DBL_GROUP + 2)
#define DPD_EC_F2MDBL          (DPD_EC_PTADD_DBL_GROUP + 3)






    /**
     * IPSEC_CBC_REQ - common IPSec/DES-CBC compound request operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char *hashKeyData;
        unsigned long  cryptKeyBytes;
        unsigned char *cryptKeyData;
        unsigned long  cryptCtxInBytes;
        unsigned char *cryptCtxInData;
        unsigned long  hashInDataBytes;
        unsigned char *hashInData;
        unsigned long  inDataBytes;
        unsigned char *inData;
        unsigned char *cryptDataOut;
        unsigned long  hashDataOutBytes;
        unsigned char *hashDataOut;
    } IPSEC_CBC_REQ;

#define NUM_IPSEC_CBC_DESC                    (24)
#define DPD_IPSEC_CBC_GROUP                   (IPSEC_DES_GROUP | 0x0000)

#define DPD_IPSEC_CBC_SDES_ENCRYPT_MD5_PAD    (DPD_IPSEC_CBC_GROUP + 0)
#define DPD_IPSEC_CBC_SDES_ENCRYPT_SHA_PAD    (DPD_IPSEC_CBC_GROUP + 1)
#define DPD_IPSEC_CBC_SDES_ENCRYPT_SHA256_PAD (DPD_IPSEC_CBC_GROUP + 2)
#define DPD_IPSEC_CBC_SDES_DECRYPT_MD5_PAD    (DPD_IPSEC_CBC_GROUP + 3)
#define DPD_IPSEC_CBC_SDES_DECRYPT_SHA_PAD    (DPD_IPSEC_CBC_GROUP + 4)
#define DPD_IPSEC_CBC_SDES_DECRYPT_SHA256_PAD (DPD_IPSEC_CBC_GROUP + 5)
#define DPD_IPSEC_CBC_TDES_ENCRYPT_MD5_PAD    (DPD_IPSEC_CBC_GROUP + 6)
#define DPD_IPSEC_CBC_TDES_ENCRYPT_SHA_PAD    (DPD_IPSEC_CBC_GROUP + 7)
#define DPD_IPSEC_CBC_TDES_ENCRYPT_SHA256_PAD (DPD_IPSEC_CBC_GROUP + 8)
#define DPD_IPSEC_CBC_TDES_DECRYPT_MD5_PAD    (DPD_IPSEC_CBC_GROUP + 9)
#define DPD_IPSEC_CBC_TDES_DECRYPT_SHA_PAD    (DPD_IPSEC_CBC_GROUP + 10)
#define DPD_IPSEC_CBC_TDES_DECRYPT_SHA256_PAD (DPD_IPSEC_CBC_GROUP + 11)

    /* All "standard" legacy request types will autopad, no reason */
    /* to do otherwise                                             */
#define DPD_IPSEC_CBC_SDES_ENCRYPT_MD5    DPD_IPSEC_CBC_SDES_ENCRYPT_MD5_PAD
#define DPD_IPSEC_CBC_SDES_ENCRYPT_SHA    DPD_IPSEC_CBC_SDES_ENCRYPT_SHA_PAD
#define DPD_IPSEC_CBC_SDES_ENCRYPT_SHA256 DPD_IPSEC_CBC_SDES_ENCRYPT_SHA256_PAD
#define DPD_IPSEC_CBC_SDES_DECRYPT_MD5    DPD_IPSEC_CBC_SDES_DECRYPT_MD5_PAD
#define DPD_IPSEC_CBC_SDES_DECRYPT_SHA    DPD_IPSEC_CBC_SDES_DECRYPT_SHA_PAD
#define DPD_IPSEC_CBC_SDES_DECRYPT_SHA256 DPD_IPSEC_CBC_SDES_DECRYPT_SHA256_PAD
#define DPD_IPSEC_CBC_TDES_ENCRYPT_MD5    DPD_IPSEC_CBC_TDES_ENCRYPT_MD5_PAD
#define DPD_IPSEC_CBC_TDES_ENCRYPT_SHA    DPD_IPSEC_CBC_TDES_ENCRYPT_SHA_PAD
#define DPD_IPSEC_CBC_TDES_ENCRYPT_SHA256 DPD_IPSEC_CBC_TDES_ENCRYPT_SHA256_PAD
#define DPD_IPSEC_CBC_TDES_DECRYPT_MD5    DPD_IPSEC_CBC_TDES_DECRYPT_MD5_PAD
#define DPD_IPSEC_CBC_TDES_DECRYPT_SHA    DPD_IPSEC_CBC_TDES_DECRYPT_SHA_PAD
#define DPD_IPSEC_CBC_TDES_DECRYPT_SHA256 DPD_IPSEC_CBC_TDES_DECRYPT_SHA256_PAD



    /**
     * IPSEC_ECB_REQ - common IPSec/DES-ECB compound request operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char *hashKeyData;
        unsigned long  cryptKeyBytes;
        unsigned char *cryptKeyData;
        unsigned long  hashInDataBytes;
        unsigned char *hashInData;
        unsigned long  inDataBytes;
        unsigned char *inData;
        unsigned long  hashDataOutBytes;
        unsigned char *hashDataOut;
        unsigned char *cryptDataOut;
    } IPSEC_ECB_REQ;

#define NUM_IPSEC_ECB_DESC                    (24)
#define DPD_IPSEC_ECB_GROUP                   (IPSEC_DES_GROUP | 0x0100)

#define DPD_IPSEC_ECB_SDES_ENCRYPT_MD5_PAD    (DPD_IPSEC_ECB_GROUP + 0)
#define DPD_IPSEC_ECB_SDES_ENCRYPT_SHA_PAD    (DPD_IPSEC_ECB_GROUP + 1)
#define DPD_IPSEC_ECB_SDES_ENCRYPT_SHA256_PAD (DPD_IPSEC_ECB_GROUP + 2)
#define DPD_IPSEC_ECB_SDES_DECRYPT_MD5_PAD    (DPD_IPSEC_ECB_GROUP + 3)
#define DPD_IPSEC_ECB_SDES_DECRYPT_SHA_PAD    (DPD_IPSEC_ECB_GROUP + 4)
#define DPD_IPSEC_ECB_SDES_DECRYPT_SHA256_PAD (DPD_IPSEC_ECB_GROUP + 5)
#define DPD_IPSEC_ECB_TDES_ENCRYPT_MD5_PAD    (DPD_IPSEC_ECB_GROUP + 6)
#define DPD_IPSEC_ECB_TDES_ENCRYPT_SHA_PAD    (DPD_IPSEC_ECB_GROUP + 7)
#define DPD_IPSEC_ECB_TDES_ENCRYPT_SHA256_PAD (DPD_IPSEC_ECB_GROUP + 8)
#define DPD_IPSEC_ECB_TDES_DECRYPT_MD5_PAD    (DPD_IPSEC_ECB_GROUP + 9)
#define DPD_IPSEC_ECB_TDES_DECRYPT_SHA_PAD    (DPD_IPSEC_ECB_GROUP + 10)
#define DPD_IPSEC_ECB_TDES_DECRYPT_SHA256_PAD (DPD_IPSEC_ECB_GROUP + 11)

#define DPD_IPSEC_ECB_SDES_ENCRYPT_MD5    DPD_IPSEC_ECB_SDES_ENCRYPT_MD5_PAD
#define DPD_IPSEC_ECB_SDES_ENCRYPT_SHA    DPD_IPSEC_ECB_SDES_ENCRYPT_SHA_PAD
#define DPD_IPSEC_ECB_SDES_ENCRYPT_SHA256 DPD_IPSEC_ECB_SDES_ENCRYPT_SHA256_PAD
#define DPD_IPSEC_ECB_SDES_DECRYPT_MD5    DPD_IPSEC_ECB_SDES_DECRYPT_MD5_PAD
#define DPD_IPSEC_ECB_SDES_DECRYPT_SHA    DPD_IPSEC_ECB_SDES_DECRYPT_SHA_PAD
#define DPD_IPSEC_ECB_SDES_DECRYPT_SHA256 DPD_IPSEC_ECB_SDES_DECRYPT_SHA256_PAD
#define DPD_IPSEC_ECB_TDES_ENCRYPT_MD5    DPD_IPSEC_ECB_TDES_ENCRYPT_MD5_PAD
#define DPD_IPSEC_ECB_TDES_ENCRYPT_SHA    DPD_IPSEC_ECB_TDES_ENCRYPT_SHA_PAD
#define DPD_IPSEC_ECB_TDES_ENCRYPT_SHA256 DPD_IPSEC_ECB_TDES_ENCRYPT_SHA256_PAD
#define DPD_IPSEC_ECB_TDES_DECRYPT_MD5    DPD_IPSEC_ECB_TDES_DECRYPT_MD5_PAD
#define DPD_IPSEC_ECB_TDES_DECRYPT_SHA    DPD_IPSEC_ECB_TDES_DECRYPT_SHA_PAD
#define DPD_IPSEC_ECB_TDES_DECRYPT_SHA256 DPD_IPSEC_ECB_TDES_DECRYPT_SHA256_PAD



    /**
     * IPSEC_AES_CBC_REQ - common IPSec/AES-CBC compound request operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char *hashKeyData;
        unsigned long  cryptKeyBytes;
        unsigned char *cryptKeyData;
        unsigned long  cryptCtxInBytes;
        unsigned char *cryptCtxInData;
        unsigned long  hashInDataBytes;
        unsigned char *hashInData;
        unsigned long  inDataBytes;
        unsigned char *inData;
        unsigned char *cryptDataOut;
        unsigned long  hashDataOutBytes;
        unsigned char *hashDataOut;
    } IPSEC_AES_CBC_REQ;

#define NUM_IPSEC_AES_CBC_DESC                      (12)
#define DPD_IPSEC_AES_CBC_GROUP                     (IPSEC_AES_GROUP | 0x0000)

#define DPD_IPSEC_AES_CBC_ENCRYPT_MD5_APAD          (DPD_IPSEC_AES_CBC_GROUP + 0)
#define DPD_IPSEC_AES_CBC_ENCRYPT_SHA_APAD          (DPD_IPSEC_AES_CBC_GROUP + 1)
#define DPD_IPSEC_AES_CBC_ENCRYPT_SHA256_APAD       (DPD_IPSEC_AES_CBC_GROUP + 2)
#define DPD_IPSEC_AES_CBC_ENCRYPT_MD5               (DPD_IPSEC_AES_CBC_GROUP + 3)
#define DPD_IPSEC_AES_CBC_ENCRYPT_SHA               (DPD_IPSEC_AES_CBC_GROUP + 4)
#define DPD_IPSEC_AES_CBC_ENCRYPT_SHA256            (DPD_IPSEC_AES_CBC_GROUP + 5)
#define DPD_IPSEC_AES_CBC_DECRYPT_MD5_APAD          (DPD_IPSEC_AES_CBC_GROUP + 6)
#define DPD_IPSEC_AES_CBC_DECRYPT_SHA_APAD          (DPD_IPSEC_AES_CBC_GROUP + 7)
#define DPD_IPSEC_AES_CBC_DECRYPT_SHA256_APAD       (DPD_IPSEC_AES_CBC_GROUP + 8)
#define DPD_IPSEC_AES_CBC_DECRYPT_MD5               (DPD_IPSEC_AES_CBC_GROUP + 9)
#define DPD_IPSEC_AES_CBC_DECRYPT_SHA               (DPD_IPSEC_AES_CBC_GROUP + 10)
#define DPD_IPSEC_AES_CBC_DECRYPT_SHA256            (DPD_IPSEC_AES_CBC_GROUP + 11)



    /**
     * IPSEC_AES_ECB_REQ - common IPSec/AES-ECB compound request operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char *hashKeyData;
        unsigned long  cryptKeyBytes;
        unsigned char *cryptKeyData;
        unsigned long  hashInDataBytes;
        unsigned char *hashInData;
        unsigned long  inDataBytes;
        unsigned char *inData;
        unsigned char *cryptDataOut;
        unsigned long  hashDataOutBytes;
        unsigned char *hashDataOut;
    } IPSEC_AES_ECB_REQ;

#define NUM_IPSEC_AES_ECB_DESC                      (12)
#define DPD_IPSEC_AES_ECB_GROUP                     (IPSEC_AES_GROUP | 0x0100)

#define DPD_IPSEC_AES_ECB_ENCRYPT_MD5_APAD          (DPD_IPSEC_AES_ECB_GROUP + 0)
#define DPD_IPSEC_AES_ECB_ENCRYPT_SHA_APAD          (DPD_IPSEC_AES_ECB_GROUP + 1)
#define DPD_IPSEC_AES_ECB_ENCRYPT_SHA256_APAD       (DPD_IPSEC_AES_ECB_GROUP + 2)
#define DPD_IPSEC_AES_ECB_ENCRYPT_MD5               (DPD_IPSEC_AES_ECB_GROUP + 3)
#define DPD_IPSEC_AES_ECB_ENCRYPT_SHA               (DPD_IPSEC_AES_ECB_GROUP + 4)
#define DPD_IPSEC_AES_ECB_ENCRYPT_SHA256            (DPD_IPSEC_AES_ECB_GROUP + 5)
#define DPD_IPSEC_AES_ECB_DECRYPT_MD5_APAD          (DPD_IPSEC_AES_ECB_GROUP + 6)
#define DPD_IPSEC_AES_ECB_DECRYPT_SHA_APAD          (DPD_IPSEC_AES_ECB_GROUP + 7)
#define DPD_IPSEC_AES_ECB_DECRYPT_SHA256_APAD       (DPD_IPSEC_AES_ECB_GROUP + 8)
#define DPD_IPSEC_AES_ECB_DECRYPT_MD5               (DPD_IPSEC_AES_ECB_GROUP + 9)
#define DPD_IPSEC_AES_ECB_DECRYPT_SHA               (DPD_IPSEC_AES_ECB_GROUP + 10)
#define DPD_IPSEC_AES_ECB_DECRYPT_SHA256            (DPD_IPSEC_AES_ECB_GROUP + 11)




    /**
     * IPSEC_ESP_REQ - IPSec Encapsulated system payload compound operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char *hashKeyData;
        unsigned long  cryptKeyBytes;
        unsigned char *cryptKeyData;
        unsigned long  cryptCtxInBytes;
        unsigned char *cryptCtxInData;
        unsigned long  hashInDataBytes;
        unsigned char *hashInData;
        unsigned long  inDataBytes;
        unsigned char *inData;
        unsigned char *cryptDataOut;
        unsigned long  hashDataOutBytes; /* out ICV is always at the end of cryptDataOut */
        unsigned long  cryptCtxOutBytes;
        unsigned char *cryptCtxOutData;
    } IPSEC_ESP_REQ;

#define NUM_IPSEC_ESP_DESC                                (36)
#define DPD_IPSEC_ESP_GROUP                               (IPSEC_DES_GROUP | 0x0500)

#define DPD_IPSEC_ESP_OUT_SDES_ECB_CRPT_MD5_PAD           (DPD_IPSEC_ESP_GROUP + 0)
#define DPD_IPSEC_ESP_OUT_SDES_ECB_CRPT_SHA_PAD           (DPD_IPSEC_ESP_GROUP + 1)
#define DPD_IPSEC_ESP_OUT_SDES_ECB_CRPT_SHA256_PAD        (DPD_IPSEC_ESP_GROUP + 2)
#define DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_MD5_PAD           (DPD_IPSEC_ESP_GROUP + 3)
#define DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_SHA_PAD           (DPD_IPSEC_ESP_GROUP + 4)
#define DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_SHA256_PAD        (DPD_IPSEC_ESP_GROUP + 5)
#define DPD_IPSEC_ESP_OUT_SDES_CBC_CRPT_MD5_PAD           (DPD_IPSEC_ESP_GROUP + 6)
#define DPD_IPSEC_ESP_OUT_SDES_CBC_CRPT_SHA_PAD           (DPD_IPSEC_ESP_GROUP + 7)
#define DPD_IPSEC_ESP_OUT_SDES_CBC_CRPT_SHA256_PAD        (DPD_IPSEC_ESP_GROUP + 8)
#define DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_MD5_PAD           (DPD_IPSEC_ESP_GROUP + 9)
#define DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_SHA_PAD           (DPD_IPSEC_ESP_GROUP + 10)
#define DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_SHA256_PAD        (DPD_IPSEC_ESP_GROUP + 11)
#define DPD_IPSEC_ESP_OUT_TDES_CBC_CRPT_MD5_PAD           (DPD_IPSEC_ESP_GROUP + 12)
#define DPD_IPSEC_ESP_OUT_TDES_CBC_CRPT_SHA_PAD           (DPD_IPSEC_ESP_GROUP + 13)
#define DPD_IPSEC_ESP_OUT_TDES_CBC_CRPT_SHA256_PAD        (DPD_IPSEC_ESP_GROUP + 14)
#define DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_MD5_PAD           (DPD_IPSEC_ESP_GROUP + 15)
#define DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_SHA_PAD           (DPD_IPSEC_ESP_GROUP + 16)
#define DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_SHA256_PAD        (DPD_IPSEC_ESP_GROUP + 17)
#define DPD_IPSEC_ESP_OUT_TDES_ECB_CRPT_MD5_PAD           (DPD_IPSEC_ESP_GROUP + 18)
#define DPD_IPSEC_ESP_OUT_TDES_ECB_CRPT_SHA_PAD           (DPD_IPSEC_ESP_GROUP + 19)
#define DPD_IPSEC_ESP_OUT_TDES_ECB_CRPT_SHA256_PAD        (DPD_IPSEC_ESP_GROUP + 20)
#define DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_MD5_PAD           (DPD_IPSEC_ESP_GROUP + 21)
#define DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_SHA_PAD           (DPD_IPSEC_ESP_GROUP + 22)
#define DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_SHA256_PAD        (DPD_IPSEC_ESP_GROUP + 23)
#define DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_MD5_PAD_CMP       (DPD_IPSEC_ESP_GROUP + 24)
#define DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_SHA_PAD_CMP       (DPD_IPSEC_ESP_GROUP + 25)
#define DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_SHA256_PAD_CMP    (DPD_IPSEC_ESP_GROUP + 26)
#define DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_MD5_PAD_CMP       (DPD_IPSEC_ESP_GROUP + 27)
#define DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_SHA_PAD_CMP       (DPD_IPSEC_ESP_GROUP + 28)
#define DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_SHA256_PAD_CMP    (DPD_IPSEC_ESP_GROUP + 29)
#define DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_MD5_PAD_CMP       (DPD_IPSEC_ESP_GROUP + 30)
#define DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_SHA_PAD_CMP       (DPD_IPSEC_ESP_GROUP + 31)
#define DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_SHA256_PAD_CMP    (DPD_IPSEC_ESP_GROUP + 32)
#define DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_MD5_PAD_CMP       (DPD_IPSEC_ESP_GROUP + 33)
#define DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_SHA_PAD_CMP       (DPD_IPSEC_ESP_GROUP + 34)
#define DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_SHA256_PAD_CMP    (DPD_IPSEC_ESP_GROUP + 35)





    /**
     * CCMP_REQ 802.11 counter mode with CBC MAC protocol operation
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  keyBytes;
        unsigned char *keyData;
        unsigned long  ctxBytes;
        unsigned char *context;
        unsigned long  FrameDataBytes;
        unsigned char *FrameData;
        unsigned long  AADBytes;
        unsigned char *AADData;
        unsigned long  cryptDataBytes;
        unsigned char *cryptDataOut;
        unsigned long  MICBytes;
        unsigned char *MICData;
    } CCMP_REQ;

#define NUM_CCMP_DESC                         (3)
#define DPD_CCMP_GROUP                        (AES_GROUP | 0x0500)

#define DPD_802_11_CCMP_OUTBOUND              (DPD_CCMP_GROUP + 0)
#define DPD_802_11_CCMP_INBOUND               (DPD_CCMP_GROUP + 1)
#define DPD_802_11_CCMP_INBOUND_CMP           (DPD_CCMP_GROUP + 2)



    /**
     * SRTP_REQ - secure RTP compound operations
     *
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char *hashKeyData;
        unsigned long  keyBytes;
        unsigned char *keyData;
        unsigned long  ivBytes;
        unsigned char *ivData;
        unsigned long  HeaderBytes;
        unsigned long  inBytes;
        unsigned char *inData;
        unsigned long  ROCBytes;
        unsigned long  cryptDataBytes;
        unsigned char *cryptDataOut;
        unsigned long  digestBytes;
        unsigned char *digestData;
        unsigned long  outIvBytes;
        unsigned char *outIvData;
    } SRTP_REQ;

#define NUM_SRTP_DESC                                     (3)
#define DPD_SRTP_GROUP                                    (IPSEC_AES_GROUP | 0x0500)

#define DPD_SRTP_OUTBOUND                                 (DPD_SRTP_GROUP + 0)
#define DPD_SRTP_INBOUND                                  (DPD_SRTP_GROUP + 1)
#define DPD_SRTP_INBOUND_CMP                              (DPD_SRTP_GROUP + 2)





    /*
     * TLS request types
     *
     * Note that this differs from many similar types of requests
     * on account that there are significant differences is the needed
     * format for a DPD for each of the different types of TLS packets
     *
     * Since DPD's are built according to a translation table that's
     * determined on a group basis (not on individual requests), and
     * since the DPDs have to be constructed differently for each type
     * of TLS packet/payload, there are individual request structs for
     * each
     */

    /**
     * TLS_BLOCK_INBOUND_REQ - SSL/TLS block cipher inbound operation
     * This is a 2.1 request type only
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char* hashKeyData;
        unsigned long  hashOnlyBytes;
        unsigned char* hashOnlyData;
        unsigned long  ivBytes;
        unsigned char* ivData;
        unsigned long  cipherKeyBytes;
        unsigned char* cipherKeyData;
        unsigned long  inBytes;
        unsigned char* inData;
        unsigned long  MACcmpBytes;
        unsigned long  outBytes;
        unsigned char* outData;
        unsigned long  MACoutBytes;
        unsigned long  ivOutBytes;
        unsigned char* ivOutData;
    } TLS_BLOCK_INBOUND_REQ;

#define NUM_TLS_BLOCK_INBOUND_DESC                (30)
#define DPD_TLS_BLOCK_INBOUND_GROUP               (TLS_GROUP | 0x0000)

#define DPD_TLS_BLOCK_INBOUND_SDES_MD5            (DPD_TLS_BLOCK_INBOUND_GROUP)
#define DPD_TLS_BLOCK_INBOUND_SDES_MD5_CMP        (DPD_TLS_BLOCK_INBOUND_GROUP + 1)
#define DPD_TLS_BLOCK_INBOUND_SDES_SHA1           (DPD_TLS_BLOCK_INBOUND_GROUP + 2)
#define DPD_TLS_BLOCK_INBOUND_SDES_SHA1_CMP       (DPD_TLS_BLOCK_INBOUND_GROUP + 3)
#define DPD_TLS_BLOCK_INBOUND_SDES_SHA256         (DPD_TLS_BLOCK_INBOUND_GROUP + 4)
#define DPD_TLS_BLOCK_INBOUND_SDES_SHA256_CMP     (DPD_TLS_BLOCK_INBOUND_GROUP + 5)
#define DPD_TLS_BLOCK_INBOUND_TDES_MD5            (DPD_TLS_BLOCK_INBOUND_GROUP + 6)
#define DPD_TLS_BLOCK_INBOUND_TDES_MD5_CMP        (DPD_TLS_BLOCK_INBOUND_GROUP + 7)
#define DPD_TLS_BLOCK_INBOUND_TDES_SHA1           (DPD_TLS_BLOCK_INBOUND_GROUP + 8)
#define DPD_TLS_BLOCK_INBOUND_TDES_SHA1_CMP       (DPD_TLS_BLOCK_INBOUND_GROUP + 9)
#define DPD_TLS_BLOCK_INBOUND_TDES_SHA256         (DPD_TLS_BLOCK_INBOUND_GROUP + 10)
#define DPD_TLS_BLOCK_INBOUND_TDES_SHA256_CMP     (DPD_TLS_BLOCK_INBOUND_GROUP + 11)
#define DPD_TLS_BLOCK_INBOUND_SDES_MD5_SMAC       (DPD_TLS_BLOCK_INBOUND_GROUP + 12)
#define DPD_TLS_BLOCK_INBOUND_SDES_MD5_SMAC_CMP   (DPD_TLS_BLOCK_INBOUND_GROUP + 13)
#define DPD_TLS_BLOCK_INBOUND_SDES_SHA1_SMAC      (DPD_TLS_BLOCK_INBOUND_GROUP + 14)
#define DPD_TLS_BLOCK_INBOUND_SDES_SHA1_SMAC_CMP  (DPD_TLS_BLOCK_INBOUND_GROUP + 15)
#define DPD_TLS_BLOCK_INBOUND_TDES_MD5_SMAC       (DPD_TLS_BLOCK_INBOUND_GROUP + 16)
#define DPD_TLS_BLOCK_INBOUND_TDES_MD5_SMAC_CMP   (DPD_TLS_BLOCK_INBOUND_GROUP + 17)
#define DPD_TLS_BLOCK_INBOUND_TDES_SHA1_SMAC      (DPD_TLS_BLOCK_INBOUND_GROUP + 18)
#define DPD_TLS_BLOCK_INBOUND_TDES_SHA1_SMAC_CMP  (DPD_TLS_BLOCK_INBOUND_GROUP + 19)
#define DPD_TLS_BLOCK_INBOUND_AES_MD5             (DPD_TLS_BLOCK_INBOUND_GROUP + 20)
#define DPD_TLS_BLOCK_INBOUND_AES_MD5_CMP         (DPD_TLS_BLOCK_INBOUND_GROUP + 21)
#define DPD_TLS_BLOCK_INBOUND_AES_SHA1            (DPD_TLS_BLOCK_INBOUND_GROUP + 22)
#define DPD_TLS_BLOCK_INBOUND_AES_SHA1_CMP        (DPD_TLS_BLOCK_INBOUND_GROUP + 23)
#define DPD_TLS_BLOCK_INBOUND_AES_SHA256          (DPD_TLS_BLOCK_INBOUND_GROUP + 24)
#define DPD_TLS_BLOCK_INBOUND_AES_SHA256_CMP      (DPD_TLS_BLOCK_INBOUND_GROUP + 25)
#define DPD_TLS_BLOCK_INBOUND_AES_MD5_SMAC        (DPD_TLS_BLOCK_INBOUND_GROUP + 26)
#define DPD_TLS_BLOCK_INBOUND_AES_MD5_SMAC_CMP    (DPD_TLS_BLOCK_INBOUND_GROUP + 27)
#define DPD_TLS_BLOCK_INBOUND_AES_SHA1_SMAC       (DPD_TLS_BLOCK_INBOUND_GROUP + 28)
#define DPD_TLS_BLOCK_INBOUND_AES_SHA1_SMAC_CMP   (DPD_TLS_BLOCK_INBOUND_GROUP + 29)



    /**
     * TLS_BLOCK_OUTBOUND_REQ - SSL/TLS block cipher outbound operation
     * This is a 2.1 request type only
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char* hashKeyData;
        unsigned long  ivBytes;
        unsigned char* ivData;
        unsigned long  cipherKeyBytes;
        unsigned char* cipherKeyData;
        unsigned long  hashOnlyBytes;
        unsigned char* hashOnlyData;
        unsigned long  mainDataBytes;
        unsigned long  outBytes;
        unsigned char* outData;
        unsigned long  MACbytes;
        unsigned long  cipherOnlyBytes;
        unsigned char* cipherOnlyData;
        unsigned long  ivOutBytes;
        unsigned char* ivOutData;
    } TLS_BLOCK_OUTBOUND_REQ;

#define NUM_TLS_BLOCK_OUTBOUND_DESC             (9)
#define DPD_TLS_BLOCK_OUTBOUND_GROUP            (TLS_GROUP | 0x0100)

#define DPD_TLS_BLOCK_OUTBOUND_SDES_MD5         (DPD_TLS_BLOCK_OUTBOUND_GROUP + 0)
#define DPD_TLS_BLOCK_OUTBOUND_SDES_SHA1        (DPD_TLS_BLOCK_OUTBOUND_GROUP + 1)
#define DPD_TLS_BLOCK_OUTBOUND_SDES_SHA256      (DPD_TLS_BLOCK_OUTBOUND_GROUP + 2)
#define DPD_TLS_BLOCK_OUTBOUND_TDES_MD5         (DPD_TLS_BLOCK_OUTBOUND_GROUP + 3)
#define DPD_TLS_BLOCK_OUTBOUND_TDES_SHA1        (DPD_TLS_BLOCK_OUTBOUND_GROUP + 4)
#define DPD_TLS_BLOCK_OUTBOUND_TDES_SHA256      (DPD_TLS_BLOCK_OUTBOUND_GROUP + 5)
#define DPD_TLS_BLOCK_OUTBOUND_AES_MD5          (DPD_TLS_BLOCK_OUTBOUND_GROUP + 6)
#define DPD_TLS_BLOCK_OUTBOUND_AES_SHA1         (DPD_TLS_BLOCK_OUTBOUND_GROUP + 7)
#define DPD_TLS_BLOCK_OUTBOUND_AES_SHA256       (DPD_TLS_BLOCK_OUTBOUND_GROUP + 8)


    /**
     * TLS_STREAM_INBOUND_REQ - SSL/TLS stream cipher inbound operation
     * This is a 2.1 request type only
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char* hashKeyData;
        unsigned long  hashOnlyBytes;
        unsigned char* hashOnlyData;
        unsigned long  ivBytes;
        unsigned char* ivData;
        unsigned long  cipherKeyBytes;
        unsigned char* cipherKeyData;
        unsigned long  inBytes;
        unsigned char* inData;
        unsigned long  MACcmpBytes;
        unsigned long  outBytes;
        unsigned char* outData;
        unsigned long  MACoutBytes;
        unsigned long  ivOutBytes;
        unsigned char* ivOutData;
        unsigned char* cmpData;
    } TLS_STREAM_INBOUND_REQ;

#define NUM_TLS_STREAM_INBOUND_DESC               (12)
#define DPD_TLS_STREAM_INBOUND_GROUP              (TLS_GROUP | 0x0200)

#define DPD_TLS_STREAM_INBOUND
#define DPD_TLS_STREAM_INBOUND_MD5                (DPD_TLS_STREAM_INBOUND_GROUP)
#define DPD_TLS_STREAM_INBOUND_CTX_MD5            (DPD_TLS_STREAM_INBOUND_GROUP + 1)
#define DPD_TLS_STREAM_INBOUND_SHA1               (DPD_TLS_STREAM_INBOUND_GROUP + 2)
#define DPD_TLS_STREAM_INBOUND_CTX_SHA1           (DPD_TLS_STREAM_INBOUND_GROUP + 3)
#define DPD_TLS_STREAM_INBOUND_SHA256             (DPD_TLS_STREAM_INBOUND_GROUP + 4)
#define DPD_TLS_STREAM_INBOUND_CTX_SHA256         (DPD_TLS_STREAM_INBOUND_GROUP + 5)
#define DPD_TLS_STREAM_INBOUND_MD5_CMP            (DPD_TLS_STREAM_INBOUND_GROUP + 6)
#define DPD_TLS_STREAM_INBOUND_CTX_MD5_CMP        (DPD_TLS_STREAM_INBOUND_GROUP + 7)
#define DPD_TLS_STREAM_INBOUND_SHA1_CMP           (DPD_TLS_STREAM_INBOUND_GROUP + 8)
#define DPD_TLS_STREAM_INBOUND_CTX_SHA1_CMP       (DPD_TLS_STREAM_INBOUND_GROUP + 9)
#define DPD_TLS_STREAM_INBOUND_SHA256_CMP         (DPD_TLS_STREAM_INBOUND_GROUP + 10)
#define DPD_TLS_STREAM_INBOUND_CTX_SHA256_CMP     (DPD_TLS_STREAM_INBOUND_GROUP + 11)




    /**
     * TLS_STREAM_OUTBOUND_REQ - SSL/TLS stream cipher outbound
     * operation This is a 2.1 request type only
     */

    typedef struct
    {
        COMMON_REQ_PREAMBLE
        unsigned long  hashKeyBytes;
        unsigned char* hashKeyData;
        unsigned long  ivBytes;
        unsigned char* ivData;
        unsigned long  cipherKeyBytes;
        unsigned char* cipherKeyData;
        unsigned long  hashOnlyBytes;
        unsigned char* hashOnlyData;
        unsigned long  mainDataBytes;
        unsigned long  outBytes;
        unsigned char* outData;
        unsigned long  MACbytes;
        unsigned long  ivOutBytes;
        unsigned char* ivOutData;
        unsigned char* cmpData;
    } TLS_STREAM_OUTBOUND_REQ;

#define NUM_TLS_STREAM_OUTBOUND_DESC             (6)
#define DPD_TLS_STREAM_OUTBOUND_GROUP            (TLS_GROUP | 0x0300)

#define DPD_TLS_STREAM_OUTBOUND_MD5              (DPD_TLS_STREAM_OUTBOUND_GROUP + 0)
#define DPD_TLS_STREAM_OUTBOUND_SHA1             (DPD_TLS_STREAM_OUTBOUND_GROUP + 1)
#define DPD_TLS_STREAM_OUTBOUND_SHA256           (DPD_TLS_STREAM_OUTBOUND_GROUP + 2)
#define DPD_TLS_STREAM_OUTBOUND_CTX_MD5          (DPD_TLS_STREAM_OUTBOUND_GROUP + 3)
#define DPD_TLS_STREAM_OUTBOUND_CTX_SHA1         (DPD_TLS_STREAM_OUTBOUND_GROUP + 4)
#define DPD_TLS_STREAM_OUTBOUND_CTX_SHA256       (DPD_TLS_STREAM_OUTBOUND_GROUP + 5)






#ifdef _cplusplus
}
#endif

#endif  /* SEC2_H */

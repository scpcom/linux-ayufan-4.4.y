/*
 * t2dpd.c
 *
 * t23x SEC2.x legacy interface
 * REQ-to-descriptor translation interface
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
 * 2.1.0   2009_05_04 sec - add SNOW-3G support
 *
 */



/** @file
 * Constructs packet descriptors for the legacy driver interface
 * for Talitos 2/3
 */


#include <linux/kernel.h>
#include <linux/types.h>

#include <asm/page.h>

#include "../common/xwcRMinterface.h"
#include "../t23xrm/t23xrmInternal.h"
#include "Sec2.h"
#include "Sec2local.h"


/**
 * Header constants for AESA_CRYPT_REQ / DPD_AESA_CRYPT_GROUP
 * Note that CCM and SRT modes are not handled by this request type,
 * they are deployed in the SRTP and CCMP request types.
 */

const unsigned long AesaDesc[1*NUM_AESA_CRYPT_DESC] = {
    /* DPD_AESA_CBC_ENCRYPT_CRYPT     OpId 0x6000 */
    /* CM = 1 (cbc), encrypt on                   */
    /* desc type = 2 (common, nosnoop)            */
    /* 0x60300010,                                */
    ((EU_AES | AES_ENCRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_CBC_DECRYPT_CRYPT     OpId 0x6001 */
    /* CM = 1 (cbc), encrypt off                  */
    /* desc type = 2 (common, nosnoop)            */
    /* 0x60200010,                                */
    ((EU_AES | AES_DECRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_CBC_DECRYPT_CRYPT_RDK OpId 0x6002 */
    /* RDK = noexpand, CM = 1 (cbc), encrypt off  */
    /* desc type = 2 (common, nosnoop)            */
    /* 0x60a00010,                                */
    ((EU_AES | AES_DECRYPT | AES_CBC | AES_RDK) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_ECB_ENCRYPT_CRYPT     OpId 0x6003 */
    /* CM = 0 (ecb), encrypt on                   */
    /* desc type = 2 (common, nosnoop)            */
    /* 0x60100010,                                */
    ((EU_AES | AES_ENCRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_ECB_DECRYPT_CRYPT     OpId 0x6004 */
    /* CM = 0 (ecb), encrypt off                  */
    /* desc type = 2 (common, nosnoop)            */
    /* 0x60000010,                                */
    ((EU_AES | AES_DECRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_ECB_DECRYPT_CRYPT_RDK OpId 0x6005 */
    /* RDK = noexpand, CM = 1 (ecb), encrypt off  */
    /* desc type = 2 (common, nosnoop)            */
    /* 0x60800010,                                */
    ((EU_AES | AES_DECRYPT | AES_ECB | AES_RDK) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_CTR_CRYPT             OpId 0x6006 */
    /* CM = 2 (ctr), encrypt off                  */
    /* desc type = 0 (aesa_ctr, nosnoop)          */
    /* 0x60600000,                                */
    ((EU_AES | AES_CTR) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_CTR | HDR_OUTBOUND,

    /* DPD_AESA_CTR_HMAC              OpId 0x6007 */
    /* CM = 2 (ctr), encrypt off                  */
    /* desc type = 24 (aesa_ctr_HMAC-snoop)       */
    /* 0x606000c0,                                */
    ((EU_AES | AES_CTR) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_CBC_RBP_ENCRYPT       OpId 0x6008 */
    ((EU_AES | AES_CBC_RBP | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_CBC_RBP_DECRYPT       OpId 0x6009 */
    ((EU_AES | AES_CBC_RBP | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_OFB_ENCRYPT           OpId 0x600a */
    ((EU_AES | AES_CBC_OFB | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_OFB_DECRYPT           OpId 0x600b */
    ((EU_AES | AES_CBC_OFB | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_CFB128_ENCRYPT        OpId 0x600c */
    ((EU_AES | AES_CFB128 | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_CFB128_DECRYPT        OpId 0x600d */
    ((EU_AES | AES_CFB128 | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_CCM_ENCRYPT           OpId 0x600e */
    ((EU_AES | AES_CCM | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_CCM_DECRYPT           OpId 0x600f */
    ((EU_AES | AES_CCM | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_CCM_FINAL_ENCRYPT     OpId 0x6010 */
    ((EU_AES | AES_CCM_FMAC | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_CCM_FINAL_DECRYPT     OpId 0x6011 */
    ((EU_AES | AES_CCM_FMAC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_CCM_FINAL_DECRYPT_CMP OpId 0x6012 */
    ((EU_AES | AES_CCM_FMAC | AES_RDK | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_LRW_ENCRYPT           OpId 0x6013 */
    ((EU_AES | AES_LRW | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_LRW_NO_TWEAK_ENCRYPT  OpId 0x6014 */
    ((EU_AES | AES_LRW_NO_TWEAK | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_LRW_DECRYPT           OpId 0x6015 */
    ((EU_AES | AES_LRW | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

    /* DPD_AESA_LRW_NO_TWEAK_DECRYPT  OpId 0x6016 */
    ((EU_AES | AES_LRW_NO_TWEAK | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_AES_HMAC | HDR_OUTBOUND,

};



const unsigned long AesaMACDesc[1*NUM_AESA_MAC_DESC] = {
    /* DPD_AESA_CMAC                  OpId 0x6100 */
    ((EU_AES | AES_CMAC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_CMAC_CMP              OpId 0x6101 */
    ((EU_AES | AES_CMAC_ICV | AES_RDK) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_XCBCMAC               OpId 0x6102 */
    ((EU_AES | AES_XCBC_MAC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_AESA_XCBCMAC_CMP           OpId 0x6103 */
    ((EU_AES | AES_XCBC_MAC_ICV | AES_RDK) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_OUTBOUND
};


const unsigned long AesaGCMDesc[1*NUM_AESA_GCM_DESC] = {
    /* DPD_AESA_GCM_ENCRYPT           OpId 0x6300 */
    ((EU_AES | AES_GCM | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_IPSEC_AES_GCM | HDR_OUTBOUND,

    /* DPD_AESA_GCM_DECRYPT           OpId 0x6301 */
    ((EU_AES | AES_GCM | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_IPSEC_AES_GCM | HDR_OUTBOUND,

    /* DPD_AESA_GCM_FINAL_ENCRYPT     OpId 0x6302 */
    ((EU_AES | AES_GCM_FMAC | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_IPSEC_AES_GCM | HDR_OUTBOUND,

    /* DPD_AESA_GCM_FINAL_DECRYPT     OpId 0x6303 */
    ((EU_AES | AES_GCM_FMAC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_IPSEC_AES_GCM | HDR_OUTBOUND,

    /* DPD_AESA_GCM_FINAL_DECRYPT_CMP OpId 0x6304 */
    ((EU_AES | AES_GCM_FMAC | AES_RDK | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_IPSEC_AES_GCM | HDR_OUTBOUND,
};



/**
 * Header constants for RNG_REQ / DPD_RNG_GROUP
 */

const unsigned long RngDesc[1*NUM_RNGA_DESC] = {
    /* DPD_RNG_GETRN Req OpId 0x1000 */
    /* 0x40000010                    */
    (EU_RND << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON
};



/**
 * Header constants for DES_CBC_CRYPT_REQ / DPD_DES_CBC_GROUP
 */

const unsigned long DesCbcReq[1 * NUM_DES_CBC_DESC] = {
    /* DPD_SDES_CBC_CTX_ENCRYPT              OpId 0x2500 */
    ((EU_DES | DES_SINGLE | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SDES_CBC_CTX_DECRYPT              OpId 0x2501 */
    ((EU_DES | DES_SINGLE | DES_CBC | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_TDES_CBC_CTX_ENCRYPT              OpId 0x2502 */
    ((EU_DES | DES_TRIPLE | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_TDES_CBC_CTX_DECRYPT              OpId 0x2503 */
    ((EU_DES | DES_TRIPLE | DES_CBC | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SDES_CFB_64_CTX_ENCRYPT           OpId 0x2508 */
    ((EU_DES | DES_SINGLE | DES_CFB_64 | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SDES_CFB_64_CTX_DECRYPT           OpId 0x2509 */
    ((EU_DES | DES_SINGLE | DES_CFB_64 | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_TDES_CFB_64_CTX_ENCRYPT           OpId 0x250a */
    ((EU_DES | DES_TRIPLE | DES_CFB_64 | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_TDES_CFB_64_CTX_DECRYPT           OpId 0x250b */
    ((EU_DES | DES_TRIPLE | DES_CFB_64 | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SDES_OFB_64_CTX_ENCRYPT           OpId 0x250c */
    ((EU_DES | DES_SINGLE | DES_OFB_64 | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SDES_OFB_64_CTX_DECRYPT           OpId 0x250d */
    ((EU_DES | DES_SINGLE | DES_OFB_64 | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_TDES_OFB_64_CTX_ENCRYPT           OpId 0x250e */
    ((EU_DES | DES_TRIPLE | DES_OFB_64 | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_TDES_OFB_64_CTX_DECRYPT           OpId 0x250f */
    ((EU_DES | DES_TRIPLE | DES_OFB_64 | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

};




/**
 * Header constants for DES_CRYPT_REQ / DPD_DES_ECB_GROUP
 */

const unsigned long DesReq[1*NUM_DES_DESC] = {
    /* DPD_SDES_ECB_ENCRYPT Req OpId 0x2600 */
    /* 0x20100010,                          */
    ((EU_DES | DES_SINGLE | DES_ECB | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SDES_ECB_DECRYPT Req OpId 0x2601 */
    /* 0x20000010,                          */
    ((EU_DES | DES_SINGLE | DES_ECB | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_TDES_ECB_ENCRYPT Req OpId 0x2602 */
    /* 0x20300010,                          */
    ((EU_DES | DES_TRIPLE | DES_ECB | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_TDES_ECB_DECRYPT Req OpId 0x2603 */
    /* 0x20200010                           */
    ((EU_DES | DES_TRIPLE | DES_ECB | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,
};




/**
 * Header constants for
 * ARC4_LOADCTX_CRYPT_REQ / DPD_RC4_LDCTX_CRYPT_ULCTX_GROUP
 */

const unsigned long Rc4LoadCtxUnloadCtxReq[1*NUM_RC4_LOADCTX_UNLOADCTX_DESC] = {
    /* DPD_RC4_LDCTX_CRYPT_ULCTX_GROUP OpId 0x3400 */
    /* 0x10700050                                  */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
        DESCTYPE_ARC4
};



/**
 * Header constants for
 * ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ / DPD_RC4_LDKEY_CRYPT_ULCTX_GROUP
 */
const unsigned long Rc4LoadKeyUnloadCtxReq[1*NUM_RC4_LOADKEY_UNLOADCTX_DESC] = {
    /* DPD_RC4_LDKEY_CRYPT_ULCTX Req OpId 0x3500 */
    /* 0x10200050                                */
    ((EU_ARC4 | ARC4_DUMP_CONTEXT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_ARC4
};



/**
 * Header constants for HASH_REQ / DPD_HASH_LDCTX_HASH_ULCTX_GROUP
 */
const unsigned long MdhaReq[1 * NUM_MDHA_DESC] = {
    /* DPD_SHA256_LDCTX_HASH_ULCTX          OpId 0x4400 */
    /* SHA-256                                          */
    /* 0x30100010,                                      */
    ((EU_MD | MD_SHA256) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_MD5_LDCTX_HASH_ULCTX             OpId 0x4401 */
    /* MD-5                                             */
    /* 0x30200010,                                      */
    ((EU_MD | MD_MD5) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA_LDCTX_HASH_ULCTX             OpId 0x4402 */
    /* SHA-1                                            */
    /* 0x30000010,                                      */
    ((EU_MD | MD_SHA1) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA256_LDCTX_IDGS_HASH_ULCTX     OpId 0x4403 */
    /* SHA-256, CONT, INIT                              */
    /* 0x39100010,                                      */
    ((EU_MD | MD_SHA256 | MD_CONT | MD_INIT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_MD5_LDCTX_IDGS_HASH_ULCTX        OpId 0x4404 */
    /* MD-5, CONT, INIT                                 */
    /* 0x39200010,                                      */
    ((EU_MD | MD_MD5 | MD_CONT | MD_INIT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA_LDCTX_IDGS_HASH_ULCTX        OpId 0x4405 */
    /* SHA-1, CONT, INIT                                */
    /* 0x39000010,                                      */
    ((EU_MD | MD_SHA1 | MD_CONT | MD_INIT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA256_CONT_HASH_ULCTX           OpId 0x4406 */
    /* SHA256, CONT                                     */
    /* 0x38100010,                                      */
    ((EU_MD | MD_SHA256 | MD_CONT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_MD5_CONT_HASH_ULCTX              OpId 0x4407 */
    /* MD-5, CONT                                       */
    /* 0x38200010,                                      */
    ((EU_MD | MD_MD5 | MD_CONT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA_CONT_HASH_ULCTX              OpId 0x4408 */
    /* SHA-1, CONT                                      */
    /* 0x38000010,                                      */
    ((EU_MD | MD_SHA1 | MD_CONT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA224_LDCTX_HASH_ULCTX          OpId 0x4409 */
    /* SHA-224                                          */
    /* 0x30300010,                                      */
    ((EU_MD | MD_SHA224) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA224_LDCTX_IDGS_HASH_ULCTX     OpId 0x440a */
    /* SHA224, CONT, INIT                               */
    /* 0x39300010,                                      */
    ((EU_MD | MD_SHA224 | MD_CONT | MD_INIT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA224_CONT_HASH_ULCTX           OpId 0x440b */
    /* SHA-224, CONT                                    */
    /* 0x38300010,                                      */
    ((EU_MD | MD_SHA224 | MD_CONT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA256_LDCTX_HASH_ULCTX_CMP      OpId 0x440c */
    /* SHA-256, CICV                                    */
    /* 0x34100012,                                      */
    ((EU_MD | MD_SHA256 | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_MD5_LDCTX_HASH_ULCTX_CMP         OpId 0x440d */
    /* MD-5, CICV                                       */
    /* 0x34200012,                                      */
    ((EU_MD | MD_MD5 | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA_LDCTX_HASH_ULCTX_CMP         OpId 0x440e */
    /* SHA-1, CICV                                      */
    /* 0x34000012,                                      */
    ((EU_MD | MD_SHA1 | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA256_LDCTX_IDGS_HASH_ULCTX_CMP OpId 0x440f */
    /* SHA-256, CONT, INIT, CICV                        */
    /* 0x3d100012,                                      */
    ((EU_MD | MD_SHA256 | MD_CONT | MD_INIT | MD_CICV) << EU_SHIFT_PRIMARY)
        | DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_MD5_LDCTX_IDGS_HASH_ULCTX_CMP    OpId 0x4410 */
    /* MD-5, CONT, INIT, CICV                           */
    /* 0x3d200012,                                      */
    ((EU_MD | MD_MD5 | MD_CONT | MD_INIT | MD_CICV) << EU_SHIFT_PRIMARY)
        | DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA_LDCTX_IDGS_HASH_ULCTX_CMP    OpId 0x4411 */
    /* SHA-1, CONT, INIT, CICV                          */
    /* 0x3d000012,                                      */
    ((EU_MD | MD_SHA1 | MD_CONT | MD_INIT | MD_CICV) << EU_SHIFT_PRIMARY)
        | DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA224_LDCTX_HASH_ULCTX_CMP      OpId 0x4412 */
    /* SHA-224, CICV                                    */
    /* 0x34300012,                                      */
    ((EU_MD | MD_SHA224 | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA224_LDCTX_IDGS_HASH_ULCTX_CMP OpId 0x4413 */
    /* SHA224, CONT, INIT, CICV                         */
    /* 0x3d300012,                                      */
    ((EU_MD | MD_SHA224 | MD_CONT | MD_INIT | MD_CICV) << EU_SHIFT_PRIMARY)
        | DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA384_LDCTX_HASH_ULCTX          OpId 0x4414 */
    ((EU_MDPRIME | MD_SHA384) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA384_LDCTX_IDGS_HASH_ULCTX     OpId 0x4415 */
    ((EU_MDPRIME | MD_SHA384 | MD_CONT | MD_INIT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA384_CONT_HASH_ULCTX           OpId 0x4416 */
    ((EU_MDPRIME | MD_SHA384 | MD_CONT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA384_LDCTX_HASH_ULCTX_CMP      OpId 0x4417 */
    ((EU_MDPRIME | MD_SHA384 | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA384_LDCTX_IDGS_HASH_ULCTX_CMP OpId 0x4418 */
    ((EU_MDPRIME | MD_SHA384 | MD_CONT | MD_INIT | MD_CICV) << EU_SHIFT_PRIMARY)
        | DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA512_LDCTX_HASH_ULCTX          OpId 0x4419 */
    ((EU_MDPRIME | MD_SHA512) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA512_LDCTX_IDGS_HASH_ULCTX     OpId 0x441a */
    ((EU_MDPRIME | MD_SHA512 | MD_CONT | MD_INIT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA512_CONT_HASH_ULCTX           OpId 0x441b */
    ((EU_MDPRIME | MD_SHA512 | MD_CONT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA512_LDCTX_HASH_ULCTX_CMP      OpId 0x441c */
    ((EU_MDPRIME | MD_SHA512 | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA512_LDCTX_IDGS_HASH_ULCTX_CMP OpId 0x441d */
    ((EU_MDPRIME | MD_SHA512 | MD_CONT | MD_INIT | MD_CICV) << EU_SHIFT_PRIMARY)
        | DESCTYPE_COMMON | HDR_INBOUND,

};


/**
 * Header constants for HASH_REQ / DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP
 */
const unsigned long MdhaPadReq[1 * NUM_MDHA_PAD_DESC] = {
    /* DPD_SHA256_LDCTX_HASH_PAD_ULCTX          OpId 0x4500 */
    /* SHA-256, PD                                          */
    /* 0x30500010,                                          */
    ((EU_MD | MD_SHA256 | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_MD5_LDCTX_HASH_PAD_ULCTX             OpId 0x4501 */
    /* MD-5, PD                                             */
    /* 0x30600010,                                          */
    ((EU_MD | MD_MD5 | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA_LDCTX_HASH_PAD_ULCTX             OpId 0x4502 */
    /* SHA-1, PD                                            */
    /* 0x30400010,                                          */
    ((EU_MD | MD_SHA1 | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA256_LDCTX_IDGS_HASH_PAD_ULCTX     OpId 0x4503 */
    /* SHA-256, PD, INIT                                    */
    /* 0x31500010,                                          */
    ((EU_MD | MD_SHA256 | MD_INIT | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_MD5_LDCTX_IDGS_HASH_PAD_ULCTX        OpId 0x4504 */
    /* MD-5, PD, INIT                                       */
    /* 0x31600010,                                          */
    ((EU_MD | MD_MD5 | MD_INIT  | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA_LDCTX_IDGS_HASH_PAD_ULCTX        OpId 0x4505 */
    /* SHA-1, PD, INIT                                      */
    /* 0x31400010,                                          */
    ((EU_MD | MD_SHA1 | MD_INIT  | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA224_LDCTX_HASH_PAD_ULCTX          OpId 0x4506 */
    /* SHA-224, PD                                          */
    /* 0x30700010,                                          */
    ((EU_MD | MD_SHA224 | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA224_LDCTX_IDGS_HASH_PAD_ULCTX     OpId 0x4507 */
    /* SHA-224, PD, INIT                                    */
    /* 0x31700010,                                          */
    ((EU_MD | MD_SHA224 | MD_INIT  | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA256_LDCTX_HASH_PAD_ULCTX_CMP      OpId 0x4508 */
    /* SHA-256, PD, CICV                                    */
    /* 0x34500012,                                          */
    ((EU_MD | MD_SHA256 | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_MD5_LDCTX_HASH_PAD_ULCTX_CMP         OpId 0x4509 */
    /* MD-5, PD, CICV                                       */
    /* 0x34600012,                                          */
    ((EU_MD | MD_MD5 | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA_LDCTX_HASH_PAD_ULCTX_CMP         OpId 0x450a */
    /* SHA-1, PD, CICV                                      */
    /* 0x34400012,                                          */
    ((EU_MD | MD_SHA1 | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA256_LDCTX_IDGS_HASH_PAD_ULCTX_CMP OpId 0x450b */
    /* SHA-256, PD, INIT, CICV                              */
    /* 0x35500012,                                          */
    ((EU_MD | MD_SHA256 | MD_INIT | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_MD5_LDCTX_IDGS_HASH_PAD_ULCTX_CMP    OpId 0x450c */
    /* MD-5, PD, INIT, CICV                                 */
    /* 0x35600012,                                          */
    ((EU_MD | MD_MD5 | MD_INIT | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA_LDCTX_IDGS_HASH_PAD_ULCTX_CMP    OpId 0x450d */
    /* SHA-1, PD, INIT, CICV                                */
    /* 0x35400012,                                          */
    ((EU_MD | MD_SHA1 | MD_INIT | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA224_LDCTX_HASH_PAD_ULCTX_CMP      OpId 0x450e */
    /* SHA-224, PD, CICV                                    */
    /* 0x34700012,                                          */
    ((EU_MD | MD_SHA224 | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA224_LDCTX_IDGS_HASH_PAD_ULCTX_CMP OpId 0x450f */
    /* SHA-224, PD, INIT                                    */
    /* 0x35700012,                                          */
    ((EU_MD | MD_SHA224 | MD_INIT | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA384_LDCTX_HASH_PAD_ULCTX          OpId 0x4510 */
    ((EU_MDPRIME | MD_SHA384 | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA384_LDCTX_IDGS_HASH_PAD_ULCTX     OpId 0x4510 */
    ((EU_MDPRIME | MD_SHA384 | MD_INIT | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA384_LDCTX_HASH_PAD_ULCTX_CMP      OpId 0x4510 */
    ((EU_MDPRIME | MD_SHA384 | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA384_LDCTX_IDGS_HASH_PAD_ULCTX_CMP OpId 0x4510 */
    ((EU_MDPRIME | MD_SHA384 | MD_INIT | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA512_LDCTX_HASH_PAD_ULCTX          OpId 0x4510 */
    ((EU_MDPRIME | MD_SHA512 | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA512_LDCTX_IDGS_HASH_PAD_ULCTX     OpId 0x4510 */
    ((EU_MDPRIME | MD_SHA512 | MD_INIT | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA512_LDCTX_HASH_PAD_ULCTX_CMP      OpId 0x4510 */
    ((EU_MDPRIME | MD_SHA512 | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA512_LDCTX_IDGS_HASH_PAD_ULCTX_CMP OpId 0x4510 */
    ((EU_MDPRIME | MD_SHA512 | MD_INIT | MD_CICV | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,
};



/**
 * Header constants for HMAC_PAD_REQ / DPD_HASH_LDCTX_HMAC_ULCTX_GROUP
 */

const unsigned long HmacPadReq[1 * NUM_HMAC_PAD_DESC] = {
    /* DPD_SHA256_LDCTX_HMAC_ULCTX             OpId 0x4a00 */
    /* SHA-256, INIT, HMAC                                 */
    /* 0x31900010,                                         */
    ((EU_MD | MD_SHA256 | MD_INIT | MD_HMAC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_MD5_LDCTX_HMAC_ULCTX                OpId 0x4a01 */
    /* MD-5, INIT, HMAC                                    */
    /* 0x31A00010,                                         */
    ((EU_MD | MD_MD5 | MD_INIT | MD_HMAC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA_LDCTX_HMAC_ULCTX                OpId 0x4a02 */
    /* SHA-1, INIT, HMAC                                   */
    /* 0x31800010,                                         */
    ((EU_MD | MD_SHA1 | MD_INIT | MD_HMAC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA256_LDCTX_HMAC_PAD_ULCTX         OpId 0x4a03 */
    /* SHA-256, INIT, HMAC, PD                             */
    /* 0x31D00010,                                         */
    ((EU_MD | MD_SHA256 | MD_INIT | MD_HMAC | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_MD5_LDCTX_HMAC_PAD_ULCTX            OpId 0x4a04 */
    /* MD-5, INIT, HMAC, PD                                */
    /* 0x31E00010,                                         */
    ((EU_MD | MD_MD5 | MD_INIT | MD_HMAC | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA_LDCTX_HMAC_PAD_ULCTX            OpId 0x4a05 */
    /* SHA-1, INIT, HMAC, PD                               */
    /* 0x31C00010,                                         */
    ((EU_MD | MD_SHA1 | MD_INIT | MD_HMAC | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA224_LDCTX_HMAC_ULCTX             OpId 0x4a06 */
    /* SHA-224, INIT, HMAC                                 */
    /* 0x31b00010,                                         */
    ((EU_MD | MD_SHA224 | MD_INIT | MD_HMAC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA224_LDCTX_HMAC_PAD_ULCTX         OpId 0x4a07 */
    /* SHA-224, INIT, HMAC, PD                             */
    /* 0x31f00010,                                         */
    ((EU_MD | MD_SHA224 | MD_INIT | MD_HMAC | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA256_LDCTX_HMAC_ULCTX_CMP         OpId 0x4a08 */
    /* SHA-256, INIT, HMAC, CICV                           */
    /* 0x35900012,                                         */
    ((EU_MD | MD_SHA256 | MD_INIT | MD_HMAC | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_MD5_LDCTX_HMAC_ULCTX_CMP            OpId 0x4a09 */
    /* MD-5, INIT, HMAC, CICV                              */
    /* 0x35A00012,                                         */
    ((EU_MD | MD_MD5 | MD_INIT | MD_HMAC | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA_LDCTX_HMAC_ULCTX_CMP            OpId 0x4a0a */
    /* SHA-1, INIT, HMAC, CICV                             */
    /* 0x35800012,                                         */
    ((EU_MD | MD_SHA1 | MD_INIT | MD_HMAC | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA256_LDCTX_HMAC_PAD_ULCTX_CMP     OpId 0x4a0b */
    /* SHA-256, INIT, HMAC, PD, CICV                       */
    /* 0x35D00012,                                         */
    ((EU_MD | MD_SHA256 | MD_INIT | MD_HMAC | MD_PD | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_MD5_LDCTX_HMAC_PAD_ULCTX_CMP        OpId 0x4a0c */
    /* MD-5, INIT, HMAC, PD, CICV                          */
    /* 0x35E00012,                                         */
    ((EU_MD | MD_MD5 | MD_INIT | MD_HMAC | MD_PD | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA_LDCTX_HMAC_PAD_ULCTX_CMP        OpId 0x4a0d */
    /* SHA-1, INIT, HMAC, PD, CICV                         */
    /* 0x35C00012,                                         */
    ((EU_MD | MD_SHA1 | MD_INIT | MD_HMAC | MD_PD | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA224_LDCTX_HMAC_ULCTX_CMP         OpId 0x4a0e */
    /* SHA-224, INIT, HMAC, CICV                           */
    /* 0x35b00012,                                         */
    ((EU_MD | MD_SHA224 | MD_INIT | MD_HMAC | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA224_LDCTX_HMAC_PAD_ULCTX_CMP     OpId 0x4a0f */
    /* SHA-224, INIT, HMAC, PD, CICV                       */
    /* 0x35f00012,                                         */
    ((EU_MD | MD_SHA224 | MD_INIT | MD_HMAC | MD_PD | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA384_LDCTX_HMAC_ULCTX             OpId 0x4a10 */
    ((EU_MDPRIME | MD_SHA384 | MD_INIT | MD_HMAC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA384_LDCTX_HMAC_PAD_ULCTX         OpId 0x4a11 */
    ((EU_MDPRIME | MD_SHA384 | MD_INIT | MD_HMAC | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA384_LDCTX_HMAC_ULCTX_CMP         OpId 0x4a12 */
    ((EU_MDPRIME | MD_SHA384 | MD_INIT | MD_HMAC | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA384_LDCTX_HMAC_PAD_ULCTX_CMP     OpId 0x4a13 */
    ((EU_MDPRIME | MD_SHA384 | MD_INIT | MD_HMAC | MD_PD | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA512_LDCTX_HMAC_ULCTX             OpId 0x4a14 */
    ((EU_MDPRIME | MD_SHA512 | MD_INIT | MD_HMAC) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA512_LDCTX_HMAC_PAD_ULCTX         OpId 0x4a15 */
    ((EU_MDPRIME | MD_SHA512 | MD_INIT | MD_HMAC | MD_PD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON,

    /* DPD_SHA512_LDCTX_HMAC_ULCTX_CMP         OpId 0x4a16 */
    ((EU_MDPRIME | MD_SHA512 | MD_INIT | MD_HMAC | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_SHA512_LDCTX_HMAC_PAD_ULCTX_CMP     OpId 0x4a17 */
    ((EU_MDPRIME | MD_SHA512 | MD_INIT | MD_HMAC | MD_PD | MD_CICV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_COMMON | HDR_INBOUND,

};



/**
 * Header constants for MOD_EXP_REQ  DPD_MM_LDCTX_EXP_ULCTX_GROUP
 */
const unsigned long PkhaMmExpReq[1*NUM_MM_EXP_DESC] = {
    /* DPD_MM_LDCTX_EXP_ULCTX                  OpId 0x5100 */
    ((EU_PK | PK_MOD_EXP) << EU_SHIFT_PRIMARY)     | DESCTYPE_PK_MONTY,

    /* DPD_MM_LDCTX_EXP_TEQ_ULCTX              OpId 0x5101 */
    ((EU_PK | PK_MOD_EXP_TEQ) << EU_SHIFT_PRIMARY) | DESCTYPE_PK_MONTY
};


/**
 * Header constants for MOD_SS_EXP_REQ / DPD_MM_SS_RSA_EXP
 */
const unsigned long PkhaMmSsExpReq[1*NUM_MM_SS_EXP_DESC] = {
    /* DPD_MM_SS_RSA_EXP                       OpId 0x5b00 */
    ((EU_PK | PK_RSA_SS) << EU_SHIFT_PRIMARY)     | DESCTYPE_PK_MONTY,

    /* DPD_MM_SS_RSA_EXP_TEQ                   OpId 0x5b01 */
    ((EU_PK | PK_RSA_SS_TEQ) << EU_SHIFT_PRIMARY) | DESCTYPE_PK_MONTY,
};


/**
 * Header constants for MOD_R2MODN_REQ / DPD_MM_LDCTX_R2MODN_ULCTX_GROUP
 */
const unsigned long PkhaModR2modnReq[1 * NUM_MOD_R2MODN_DESC] = {
    /*  DPD_MM_LDCTX_R2MODN_ULCTX Req OpId 0x5200 */
    /* 0x50300080,                                */
    ((EU_PK | PK_MOD_R2MODN) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY,

    /*  DPD_F2M_R2 Req OpId 0x5201 */
    /* 0x50D00080                                 */
    ((EU_PK | PK_F2M_R2) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY
};

/**
 * Header constants for MOD_RRMODP_REQ / DPD_MM_LDCTX_RRMODP_ULCTX_GROUP
 */
const unsigned long PkhaMmRrmodpReq[1*NUM_MM_RRMODP_DESC] = {
    /* DPD_MM_LDCTX_RRMODP_ULCTX Req OpId 0x5300 */
    /* 0x50400080                                */
    ((EU_PK | PK_MOD_RRMODP) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY
};

/**
 * Header constants for MOD_INV_REQ / DPD_MM_MOD_INV_ULCTX_GROUP
 */
const unsigned long PkhaMmModInvReq[1*NUM_MM_MOD_INV_DESC] = {
    /* DPD_MM_MOD_INV_ULCTX Req OpId 0x5500 */
    /* 0x50f00080                           */
    ((EU_PK | PK_FP_MODINV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY
};

/**
 * Header constant for F2M_INV_REQ / DPD_F2M_INV_ULCTX_GROUP
 */
const unsigned long PkhaF2MInvReq[1*NUM_F2M_INV_DESC] = {
    /* DPD_F2M_INV_ULCTX Req OpId 0x5600 */
    /* 0x50e00080                        */
    ((EU_PK | PK_F2M_INV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY
};

/**
 * Header constants for MOD_2OP_REQ / DPD_MM_LDCTX_2OP_ULCTX_GROUP
 * These are carryovers from SEC1, and need to be reviewed...
 */
const unsigned long PkhaMod2OpReq[1 * NUM_MOD_2OP_DESC] = {
    /* DPD_MOD_LDCTX_MUL1_ULCTX        Req OpId 0x5400 */
    /* 0x53000080,                                     */
    ((EU_PK | PK_FP_MULT_MONT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_MOD_LDCTX_MUL2_ULCTX        Req OpId 0x5401 */
    /* 0x54000080,                                     */
    ((EU_PK | PK_FP_MULT_DECONV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_MOD_LDCTX_ADD_ULCTX         Req OpId 0x5402 */
    /* 0x51000080,                                     */
    ((EU_PK | PK_FP_MODADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_MOD_LDCTX_SUB_ULCTX         Req OpId 0x5403 */
    /* 0x52000080,                                     */
    ((EU_PK | PK_FP_MODSUB) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B0_MUL1_ULCTX Req OpId 0x5404 */
    /* 0x56000080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B0_MUL2_ULCTX Req OpId 0x5405 */
    /* 0x57000080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B0_ADD_ULCTX  Req OpId 0x5406 */
    /* 0x55000080,                                     */
    ((EU_PK | PK_F2M_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B0_MUL1_ULCTX Req OpId 0x5407 */
    /* 0x56400080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_MOD_RRMODP) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B0_MUL2_ULCTX Req OpId 0x5408 */
    /* 0x57400080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_MOD_RRMODP) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B0_ADD_ULCTX  Req OpId 0x5409 */
    /* 0x55400080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_MOD_RRMODP) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B0_MUL1_ULCTX Req OpId 0x540a */
    /* 0x56800080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_EC_F2M_PROJ_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B0_MUL2_ULCTX Req OpId 0x540b */
    /* 0x57800080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_EC_F2M_PROJ_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B0_ADD_ULCTX  Req OpId 0x540c */
    /* 0x55800080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_EC_F2M_PROJ_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B0_MUL1_ULCTX Req OpId 0x540d */
    /* 0x56c00080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_F2M_MULT_DECONV | PK_RSA_SS) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B0_MUL2_ULCTX Req OpId 0x540e */
    /* 0x57c00080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_F2M_MULT_DECONV | PK_RSA_SS) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B0_ADD_ULCTX  Req OpId 0x540f */
    /* 0x55c00080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_F2M_MULT_DECONV | PK_RSA_SS) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B1_MUL1_ULCTX Req OpId 0x5410 */
    /* 0x56100080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_CLEARMEM) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B1_MUL2_ULCTX Req OpId 0x5411 */
    /* 0x57100080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_CLEARMEM) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B1_ADD_ULCTX  Req OpId 0x5412 */
    /* 0x55100080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_CLEARMEM) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B1_MUL1_ULCTX Req OpId 0x5413 */
    /* 0x56500080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_EC_FP_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B1_MUL2_ULCTX Req OpId 0x5414 */
    /* 0x57500080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_EC_FP_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B1_ADD_ULCTX  Req OpId 0x5415 */
    /* 0x55500080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_EC_FP_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B1_MUL1_ULCTX Req OpId 0x5416 */
    /* 0x56900080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_EC_FP_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B1_MUL2_ULCTX Req OpId 0x5417 */
    /* 0x57900080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_EC_FP_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B1_ADD_ULCTX  Req OpId 0x5418 */
    /* 0x55900080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_EC_FP_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B1_MUL1_ULCTX Req OpId 0x5419 */
    /* 0x56d00080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_F2M_R2) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B1_MUL2_ULCTX Req OpId 0x541a */
    /* 0x57d00080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_F2M_R2) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B1_ADD_ULCTX  Req OpId 0x541b */
    /* 0x55d00080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_F2M_R2) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B2_MUL1_ULCTX Req OpId 0x541c */
    /* 0x56200080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_MOD_EXP) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B2_MUL2_ULCTX Req OpId 0x541d */
    /* 0x57200080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_MOD_EXP) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B2_ADD_ULCTX  Req OpId 0x541e */
    /* 0x55200080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_MOD_EXP) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B2_MUL1_ULCTX Req OpId 0x541f */
    /* 0x56600080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_EC_F2M_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B2_MUL2_ULCTX Req OpId 0x5420 */
    /* 0x57600080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_EC_F2M_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B2_ADD_ULCTX  Req OpId 0x5421 */
    /* 0x55600080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_EC_F2M_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B2_MUL1_ULCTX Req OpId 0x5422 */
    /* 0x56a00080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_EC_FP_DOUBLE) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B2_MUL2_ULCTX Req OpId 0x5423 */
    /* 0x57a00080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_EC_FP_DOUBLE) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B2_ADD_ULCTX  Req OpId 0x5424 */
    /* 0x55a00080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_EC_FP_DOUBLE) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B2_MUL1_ULCTX Req OpId 0x5425 */
    /* 0x56e00080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_F2M_INV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B2_MUL2_ULCTX Req OpId 0x5426 */
    /* 0x57e00080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_F2M_INV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B2_ADD_ULCTX  Req OpId 0x5427 */
    /* 0x55e00080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_F2M_INV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B3_MUL1_ULCTX Req OpId 0x5428 */
    /* 0x56300080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_MOD_R2MODN) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B3_MUL2_ULCTX Req OpId 0x5429 */
    /* 0x57300080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_MOD_R2MODN) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A0_B3_ADD_ULCTX  Req OpId 0x542a */
    /* 0x55300080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_MOD_R2MODN) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B3_MUL1_ULCTX Req OpId 0x542b */
    /* 0x56700080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_EC_FP_PROJ_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B3_MUL2_ULCTX Req OpId 0x542c */
    /* 0x57700080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_EC_FP_PROJ_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A1_B3_ADD_ULCTX  Req OpId 0x542d */
    /* 0x55700080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_EC_FP_PROJ_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B3_MUL1_ULCTX Req OpId 0x542e */
    /* 0x56b00080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_EC_F2M_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B3_MUL2_ULCTX Req OpId 0x542f */
    /* 0x57b00080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_EC_F2M_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A2_B3_ADD_ULCTX  Req OpId 0x5430 */
    /* 0x55b00080,                                     */
    ((EU_PK | PK_F2M_ADD | PK_EC_F2M_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B3_MUL1_ULCTX Req OpId 0x5431 */
    /* 0x56f00080,                                     */
    ((EU_PK | PK_F2M_MULT_MONT | PK_FP_MODINV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B3_MUL2_ULCTX Req OpId 0x5432 */
    /* 0x57f00080,                                     */
    ((EU_PK | PK_F2M_MULT_DECONV | PK_FP_MODINV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,
    /* DPD_POLY_LDCTX_A3_B3_ADD_ULCTX  Req OpId 0x5433 */
    /* 0x55f00080                                     */
    ((EU_PK | PK_F2M_ADD | PK_FP_MODINV) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND
};



/**
 * Header constants for ECC_POINT_REQ / DPD_EC_LDCTX_kP_ULCTX_GROUP
 * These descriptors use the "packed" format
 */
const unsigned long PkhaEccPointReq[1*NUM_EC_POINT_DESC] = {
    /* DPD_EC_FP_AFF_PT_MULT          OpId 0x5800 */
    /* 0x50500048,                                */
    ((EU_PK | PK_EC_FP_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_PTMULT | HDR_OUTBOUND,

    /* DPD_EC_FP_PROJ_PT_MULT         OpId 0x5801 */
    /* 0x50700048,                                */
    ((EU_PK | PK_EC_FP_PROJ_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_PTMULT | HDR_OUTBOUND,

    /* DPD_EC_F2M_AFF_PT_MULT         OpId 0x5802 */
    /* 0x50600048,                                */
    ((EU_PK | PK_EC_F2M_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_PTMULT | HDR_OUTBOUND,

    /* DPD_EC_F2M_PROJ_PT_MULT        OpId 0x5803 */
    /* 0x50800048,                                */
    ((EU_PK | PK_EC_F2M_PROJ_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_PTMULT | HDR_OUTBOUND,

    /* DPD_EC_FP_LDCTX_ADD_ULCTX      OpId 0x5804 */
    /* 0x50900048,                                */
    ((EU_PK | PK_EC_FP_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_PTMULT | HDR_OUTBOUND,

    /* DPD_EC_FP_LDCTX_DOUBLE_ULCTX   OpId 0x5805 */
    /* 0x50a00048,                                */
    ((EU_PK | PK_EC_FP_DOUBLE) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_PTMULT | HDR_OUTBOUND,

    /* DPD_EC_F2M_LDCTX_ADD_ULCTX     OpId 0x5806 */
    /* 0x50b00048,                                */
    ((EU_PK | PK_EC_F2M_ADD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_PTMULT | HDR_OUTBOUND,

    /* DPD_EC_F2M_LDCTX_DOUBLE_ULCTX  OpId 0x5807 */
    /* 0x50c00048                                 */
    ((EU_PK | PK_EC_F2M_DOUBLE) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_PTMULT | HDR_OUTBOUND
};



/**
 * Header constants for ECC_2OP_REQ / DPD_EC_2OP_GROUP
 */
const unsigned long PkhaEcc2OpReq[1*NUM_EC_2OP_DESC] = {
    /* DPD_EC_F2M_LDCTX_MUL1_ULCTX Req OpId 0x5900 */
    /* 0x56000080,                                 */
    ((EU_PK | PK_EC_F2M_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,

    /* DPD_EC_F2M_LDCTX_MUL2_ULCTX Req OpId 0x5901 */
    /* 0x57000080,                                 */
    ((EU_PK | PK_EC_F2M_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND,

    /* DPD_EC_F2M_LDCTX_ADD_ULCTX  Req OpId 0x5902 */
    /* 0x55000080                                  */
    ((EU_PK | PK_EC_F2M_AFF_PTMULT) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_MONTY | HDR_OUTBOUND
};


/**
 * Header constants for ECC_SPKBUILD_REQ / DPD_EC_SPKBUILD_GROUP
 */
const unsigned long PkhaEccSpkbuildReq[1*NUM_EC_SPKBUILD_DESC] =
{
    /* DPD_EC_SPKBUILD Req OpId 0x5c00 */
    /* 0x5ff00038                      */
    ((EU_PK | PK_PKBUILD) << EU_SHIFT_PRIMARY) |
        DESCTYPE_PK_ECC_ASM
};


/**
 * Header constants for ECC_PTADD_DBL
 */
const unsigned long PkhaEccPtaddDblReq[1 * NUM_EC_PTADD_DBL_DESC] =
{
    /* DPD_EC_FPADD      Req OpId 0x5d00 */
    /* 0x50900058                        */
    ((EU_PK | PK_EC_FP_ADD) << EU_SHIFT_PRIMARY) |
     DESCTYPE_PK_ECC_PTADD_D,

    /* DPD_EC_FPDBL      Req OpId 0x5d01 */
    /* 0x50a00058                        */
    ((EU_PK | PK_EC_FP_DOUBLE) << EU_SHIFT_PRIMARY) |
     DESCTYPE_PK_ECC_PTADD_D,

    /* DPD_EC_F2MADD     Req OpId 0x5d02 */
    /* 0x50b00058                        */
    ((EU_PK | PK_EC_F2M_ADD) << EU_SHIFT_PRIMARY) |
     DESCTYPE_PK_ECC_PTADD_D,

    /* DPD_EC_F2MDBL     Req OpId 0x5d03 */
    /* 0x50c00058                        */
    ((EU_PK | PK_EC_F2M_DOUBLE) << EU_SHIFT_PRIMARY) |
     DESCTYPE_PK_ECC_PTADD_D
};


/**
 * Header constants for IPSEC_CBC_REQ / DPD_IPSEC_CBC_GROUP
 */
const unsigned long IpsecCbcReq[1*NUM_IPSEC_CBC_DESC] = {
    /* DPD_IPSEC_CBC_SDES_ENCRYPT_MD5_PAD    OpId 0x7000 */
    /* 0x20531e20,                                       */
    ((EU_DES | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC,

    /* DPD_IPSEC_CBC_SDES_ENCRYPT_SHA_PAD    OpId 0x7001 */
    /* 0x20531c20,                                       */
    ((EU_DES | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC,

    /* DPD_IPSEC_CBC_SDES_ENCRYPT_SHA256_PAD OpId 0x7002 */
    /* 0x20531d20,                                       */
    ((EU_DES | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC,

    /* DPD_IPSEC_CBC_SDES_DECRYPT_MD5_PAD    OpId 0x7003 */
    /* 0x20431e22,                                       */
    ((EU_DES | DES_CBC | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_CBC_SDES_DECRYPT_SHA_PAD    OpId 0x7004 */
    /* 0x20431c22,                                       */
    ((EU_DES | DES_CBC | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_CBC_SDES_DECRYPT_SHA256_PAD OpId 0x7005 */
    /* 0x20431d22,                                       */
    ((EU_DES | DES_CBC | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_CBC_TDES_ENCRYPT_MD5_PAD    OpId 0x7006 */
    /* 0x20731e20,                                       */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC,

    /* DPD_IPSEC_CBC_TDES_ENCRYPT_SHA_PAD    OpId 0x7007 */
    /* 0x20731c20,                                       */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC,

    /* DPD_IPSEC_CBC_TDES_ENCRYPT_SHA256_PAD OpId 0x7008 */
    /* 0x20731d20,                                       */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC,

    /* DPD_IPSEC_CBC_TDES_DECRYPT_MD5_PAD    OpId 0x7009 */
    /* 0x20631e22,                                       */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_CBC_TDES_DECRYPT_SHA_PAD    OpId 0x700a */
    /* 0x20631c22,                                       */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_CBC_TDES_DECRYPT_SHA256_PAD OpId 0x700b */
    /* 0x20631d22                                        */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND
};




/**
 * Header constants for IPSEC_ECB_REQ / DPD_IPSEC_ECB_GROUP
 */
const unsigned long IpsecEcbReq[1*NUM_IPSEC_ECB_DESC] = {
    /* DPD_IPSEC_ECB_SDES_ENCRYPT_MD5_PAD    OpId 0x7100 */
    /* 0x20131e20,                                       */
    ((EU_DES | DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /* DPD_IPSEC_ECB_SDES_ENCRYPT_SHA_PAD    OpId 0x7101 */
    /* 0x20131c20,                                       */
    ((EU_DES | DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /* DPD_IPSEC_ECB_SDES_ENCRYPT_SHA256_PAD OpId 0x7102 */
    /* 0x20131d20,                                       */
    ((EU_DES | DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /* DPD_IPSEC_ECB_SDES_DECRYPT_MD5_PAD    OpId 0x7103 */
    /* 0x20031e22,                                       */
    ((EU_DES | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_ECB_SDES_DECRYPT_SHA_PAD    OpId 0x7104 */
    /* 0x20031c22,                                       */
    ((EU_DES | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_ECB_SDES_DECRYPT_SHA256_PAD OpId 0x7105 */
    /* 0x20031d22,                                       */
    ((EU_DES | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_ECB_TDES_ENCRYPT_MD5_PAD    OpId 0x7106 */
    /* 0x20331e20,                                       */
    ((EU_DES | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /* DPD_IPSEC_ECB_TDES_ENCRYPT_SHA_PAD    OpId 0x7107 */
    /* 0x20331c20,                                       */
    ((EU_DES | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /* DPD_IPSEC_ECB_TDES_ENCRYPT_SHA256_PAD OpId 0x7108 */
    /* 0x20331d20,                                       */
    ((EU_DES | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /* DPD_IPSEC_ECB_TDES_DECRYPT_MD5_PAD    OpId 0x7109 */
    /* 0x20231e22,                                       */
    ((EU_DES | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_ECB_TDES_DECRYPT_SHA_PAD    OpId 0x710a */
    /* 0x20231c22,                                       */
    ((EU_DES | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /* DPD_IPSEC_ECB_TDES_DECRYPT_SHA256_PAD OpId 0x710b */
    /* 0x20231d22                                        */
    ((EU_DES | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,
};



/**
 * Header constants for IPSEC_AES_CBC_REQ / DPD_IPSEC_AES_CBC_GROUP
 */

const unsigned long IpsecAesCbcReq[1*NUM_IPSEC_AES_CBC_DESC] = {
    /*  DPD_IPSEC_AES_CBC_ENCRYPT_MD5_APAD           OpId 0x8000 */
    /* 0x60331E20,                                               */
    ((EU_AES | AES_ENCRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_CBC_ENCRYPT_SHA_APAD           OpId 0x8001 */
    /* 0x60331C20,                                               */
    ((EU_AES | AES_ENCRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_CBC_ENCRYPT_SHA256_APAD        OpId 0x8002 */
    /* 0x60331D20,                                               */
    ((EU_AES | AES_ENCRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_CBC_ENCRYPT_MD5                OpId 0x8003 */
    /* 0x60331a20,                                               */
    ((EU_AES | AES_ENCRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_CBC_ENCRYPT_SHA                OpId 0x8004 */
    /* 0x60331820,                                               */
    ((EU_AES | AES_ENCRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_CBC_ENCRYPT_SHA256             OpId 0x8005 */
    /* 0x60331920,                                               */
    ((EU_AES | AES_ENCRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_CBC_DECRYPT_MD5_APAD           OpId 0x8006 */
    /* 0x60231e22,                                               */
    ((EU_AES | AES_DECRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_CBC_DECRYPT_SHA_APAD           OpId 0x8007 */
    /* 0x60231c22,                                               */
    ((EU_AES | AES_DECRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_CBC_DECRYPT_SHA256_APAD        OpId 0x8008 */
    /* 0x60231d22,                                               */
    ((EU_AES | AES_DECRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_CBC_DECRYPT_MD5                OpId 0x8009 */
    /* 0x60231a22,                                               */
    ((EU_AES | AES_DECRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_CBC_DECRYPT_SHA                OpId 0x800a */
    /* 0x60231822,                                               */
    ((EU_AES | AES_DECRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_CBC_DECRYPT_SHA256             OpId 0x800b */
    /* 0x60231922                                               */
    ((EU_AES | AES_DECRYPT | AES_CBC) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND
};




/**
 * Header constants for IPSEC_AES_ECB_REQ / DPD_IPSEC_AES_ECB_GROUP
 */

const unsigned long IpsecAesEcbReq[1*NUM_IPSEC_AES_ECB_DESC] = {
    /*  DPD_IPSEC_AES_ECB_ENCRYPT_MD5_APAD           OpId 0x8100 */
    /* 0x60131E20,                                               */
    ((EU_AES | AES_ENCRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_ECB_ENCRYPT_SHA_APAD           OpId 0x8101 */
    /* 0x60131C20,                                               */
    ((EU_AES | AES_ENCRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_ECB_ENCRYPT_SHA256_APAD        OpId 0x8102 */
    /* 0x60131D20,                                               */
    ((EU_AES | AES_ENCRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_ECB_ENCRYPT_MD5                OpId 0x8103 */
    /* 0x60131a20,                                               */
    ((EU_AES | AES_ENCRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC  | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_ECB_ENCRYPT_SHA                OpId 0x8104 */
    /* 0x60131820,                                               */
    ((EU_AES | AES_ENCRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC  | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_ECB_ENCRYPT_SHA256             OpId 0x8105 */
    /* 0x60131920,                                               */
    ((EU_AES | AES_ENCRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC  | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_OUTBOUND,

    /*  DPD_IPSEC_AES_ECB_DECRYPT_MD5_APAD           OpId 0x8106 */
    /* 0x60031e22,                                               */
    ((EU_AES | AES_DECRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_ECB_DECRYPT_SHA_APAD           OpId 0x8107 */
    /* 0x60031c22,                                               */
    ((EU_AES | AES_DECRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_ECB_DECRYPT_SHA256_APAD        OpId 0x8108 */
    /* 0x60031d22,                                               */
    ((EU_AES | AES_DECRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_ECB_DECRYPT_MD5                OpId 0x8109 */
    /* 0x60031a22,                                               */
    ((EU_AES | AES_DECRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC  | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_ECB_DECRYPT_SHA                OpId 0x810a */
    /* 0x60031822,                                               */
    ((EU_AES | AES_DECRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC  | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,

    /*  DPD_IPSEC_AES_ECB_DECRYPT_SHA256             OpId 0x810b */
    /* 0x60031922                                                */
    ((EU_AES | AES_DECRYPT | AES_ECB) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC  | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_HMAC | HDR_INBOUND,
};



/**
 * Header constants for IPSEC_ESP_REQ / DPD_IPSEC_ESP_GROUP
 */

const unsigned long IpsecEspReq[1 * NUM_IPSEC_ESP_DESC] = {
    /*  DPD_IPSEC_ESP_OUT_SDES_ECB_CRPT_MD5_PAD          OpId 0x7500 */
    /* 0x20131e08,                                                   */
    ((EU_DES | DES_ECB |DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_OUT_SDES_ECB_CRPT_SHA_PAD          OpId 0x7501 */
    /* 0x20131c08,                                                   */
    ((EU_DES | DES_ECB |DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_OUT_SDES_ECB_CRPT_SHA256_PAD       OpId 0x7502 */
    /* 0x20131d08,                                                   */
    ((EU_DES | DES_ECB |DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_MD5_PAD          OpId 0x7503 */
    /* 0x20031e0a,                                                   */
    ((EU_DES | DES_ECB |DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_SHA_PAD          OpId 0x7504 */
    /* 0x20031c0a,                                                   */
    ((EU_DES | DES_ECB |DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_SHA256_PAD       OpId 0x7505 */
    /* 0x20031d0a,                                                   */
    ((EU_DES | DES_ECB |DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_OUT_SDES_CBC_CRPT_MD5_PAD          OpId 0x7506 */
    /* 0x20531e08,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_OUT_SDES_CBC_CRPT_SHA_PAD          OpId 0x7507 */
    /* 0x20531c08,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_OUT_SDES_CBC_CRPT_SHA256_PAD       OpId 0x7508 */
    /* 0x20531d08,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_MD5_PAD          OpId 0x7509 */
    /* 0x20431e0a,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_SHA_PAD          OpId 0x750a */
    /* 0x20431c0a,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_SHA256_PAD       OpId 0x750b */
    /* 0x20431d0a,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_OUT_TDES_CBC_CRPT_MD5_PAD          OpId 0x750c */
    /* 0x20731e08,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_OUT_TDES_CBC_CRPT_SHA_PAD          OpId 0x750d */
    /* 0x20731c08,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_OUT_TDES_CBC_CRPT_SHA256_PAD       OpId 0x750e */
    /* 0x20731d08,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_MD5_PAD          OpId 0x750f */
    /* 0x20631e0a,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_SHA_PAD          OpId 0x7510 */
    /* 0x20631c0a,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_SHA256_PAD       OpId 0x7511 */
    /* 0x20631d0a,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_OUT_TDES_ECB_CRPT_MD5_PAD          OpId 0x7512 */
    /* 0x20331e08,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_OUT_TDES_ECB_CRPT_SHA_PAD          OpId 0x7513 */
    /* 0x20331c08,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_OUT_TDES_ECB_CRPT_SHA256_PAD       OpId 0x7514 */
    /* 0x20331d08,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_OUTBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_MD5_PAD          OpId 0x7515 */
    /* 0x20231e0a,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_SHA_PAD          OpId 0x7516 */
    /* 0x20231c0a,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_SHA256_PAD       OpId 0x7517 */
    /* 0x20231d0a,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_MD5_PAD_CMP      OpId 0x7518 */
    /* 0x20035e0a,                                                   */
    ((EU_DES | DES_ECB | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_SHA_PAD_CMP      OpId 0x7519 */
    /* 0x20035c0a,                                                   */
    ((EU_DES | DES_ECB | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_ECB_DCRPT_SHA256_PAD_CMP   OpId 0x751a */
    /* 0x20035d0a,                                                   */
    ((EU_DES | DES_ECB | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_MD5_PAD_CMP      OpId 0x751b */
    /* 0x20435e0a,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_SHA_PAD_CMP      OpId 0x751c */
    /* 0x20435c0a,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_SDES_CBC_DCRPT_SHA256_PAD_CMP   OpId 0x751d */
    /* 0x20435d0a,                                                   */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_MD5_PAD_CMP      OpId 0x751e */
    /* 0x20635e0a,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_SHA_PAD_CMP      OpId 0x751f */
    /* 0x20635c0a,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_CBC_DCRPT_SHA256_PAD_CMP   OpId 0x7520 */
    /* 0x20635d0a,                                                   */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_MD5_PAD_CMP      OpId 0x7521 */
    /* 0x20235e0a,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_SHA_PAD_CMP      OpId 0x7522 */
    /* 0x20235c0a,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND,

    /*  DPD_IPSEC_ESP_IN_TDES_ECB_DCRPT_SHA256_PAD_CMP   OpId 0x7523 */
    /* 0x20235d0a,                                                   */
    ((EU_DES | DES_ECB | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_CICV | MD_INIT | MD_HMAC | MD_PD | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_IPSEC_ESP | HDR_INBOUND
};




/**
 * Header constants for CCMP_REQ / DPD_CCMP_GROUP
 */

const unsigned long CcmpReq[1 * NUM_CCMP_DESC] = {
    /*  DPD_802_11_CCMP_OUTBOUND                 OpId 0x6500 */
    /* 0x6b100018,                                           */
    /* AES mode ECM = 2, FM, CM = 0, ED                      */
    ((EU_AES | AES_CCM | AES_FINALMAC | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    DESCTYPE_AES_CCMP | HDR_OUTBOUND,

    /*  DPD_802_11_CCMP_INBOUND                  OpId 0x6501 */
    /* 0x6b00001a,                                           */
    /* AES mode ECM = 2, CM = 0, FM                          */
    ((EU_AES | AES_CCM | AES_FINALMAC) << EU_SHIFT_PRIMARY) |
    DESCTYPE_AES_CCMP | HDR_INBOUND,

    /*  DPD_802_11_CCMP_INBOUND_CMP              0pId 0x6502 */
    /* 0x6f00001a,                                           */
    /* AES mode ECM = 2, CM = 0, FM, IM                      */
    ((EU_AES | AES_CCM | AES_FINALMAC | AES_INITMAC) << EU_SHIFT_PRIMARY) |
    DESCTYPE_AES_CCMP | HDR_INBOUND
};




/**
 * Header constants for SRTP_REQ / DPD_SRTP_GROUP
 */

const unsigned long SrtpReq[1 * NUM_SRTP_DESC] = {
    /* DPD_SRTP_OUTBOUND                         OpId 0x8500  */
    /* Primary EU:   AES     Mode: SRT, CBC-encrypt           */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, PD, SHA1       */
    /* Type:         SRTP, Outbound                           */
    /* 0x64731c28,                                            */
    ((EU_AES | AES_SRTP | AES_CBC | AES_ENCRYPT) << EU_SHIFT_PRIMARY) ||
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) ||
    DESCTYPE_SRTP | HDR_OUTBOUND,

    /* DPD_SRTP_INBOUND                          OpId 0x8501  */
    /* Primary EU:   AES     Mode: SRT, CBC-decrypt           */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, PD, SHA1       */
    /* Type:         SRTP, Inbound                            */
    /* 0x64631c2a,                                            */
    ((EU_AES | AES_SRTP | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) ||
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) ||
    DESCTYPE_SRTP | HDR_OUTBOUND,

    /* DPD_SRTP_INBOUND_CMP                     OpId 0x8502   */
    /* Primary EU:   AES     Mode: SRT, CBC-decrypt           */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, PD, CICV, SHA1 */
    /* Type:         SRTP, Inbound                            */
    /* 0x64635c2a                                             */
    ((EU_AES | AES_SRTP | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) ||
    ((EU_MD | MD_INIT | MD_HMAC | MD_PD | MD_SHA1) << EU_SHIFT_SECONDARY) ||
    DESCTYPE_SRTP | HDR_OUTBOUND,
};



/**
 * Header constants for KEA_CRYPT_REQ / DPD_KEA_CRYPT_GROUP
 */

const unsigned long KeaReq[1 * NUM_KEA_CRYPT_DESC] = {
    /* DPD_KEA_f8_CIPHER_INIT           (DPD_KEA_CRYPT_GROUP + 0) (OpId 0xa000) */
    /* INT, f8                                                                  */
    /* 0x70800010,                                                              */
    ((EU_KEA | KEA_F8 | KEA_INIT) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f8_CIPHER                (DPD_KEA_CRYPT_GROUP + 1) (OpId 0xa001) */
    /* f8                                                                       */
    /* 0x70000010,                                                              */
    ((EU_KEA | KEA_F8) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_CIPHER_INIT           (DPD_KEA_CRYPT_GROUP + 2) (OpId 0xa002) */
    /* INT, f9                                                                  */
    /* 0x70a00010,                                                              */
    ((EU_KEA | KEA_F9 | KEA_INIT) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_CIPHER                (DPD_KEA_CRYPT_GROUP + 3) (OpId 0xa003) */
    /* f9                                                                       */
    /* 0x70200010,                                                              */
    ((EU_KEA | KEA_F9) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_CIPHER_FINAL          (DPD_KEA_CRYPT_GROUP + 4) (OpId 0xa004) */
    /* PE, f9                                                                   */
    /* 0x71200010,                                                              */
    ((EU_KEA | KEA_F9 | KEA_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_CIPHER_INIT_FINAL     (DPD_KEA_CRYPT_GROUP + 5) (OpId 0xa005) */
    /* INT, PE, f9                                                              */
    /* 0x71a00010,                                                              */
    ((EU_KEA | KEA_F9 | KEA_INIT | KEA_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_GSM_A53_CIPHER           (DPD_KEA_CRYPT_GROUP + 6) (OpId 0xa006) */
    /* GSM, PE, INT, f8                                                         */
    /* 0x79800010,                                                              */
    ((EU_KEA | KEA_F8 | KEA_GSM | KEA_EOM | KEA_INIT) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_EDGE_A53_CIPHER          (DPD_KEA_CRYPT_GROUP + 7) (OpId 0xa007) */
    /* EDGE, PE, INT, f8                                                        */
    /* 0x73800010,                                                              */
    ((EU_KEA | KEA_F8 | KEA_EDGE | KEA_INIT | KEA_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_CIPHER_FINAL_CMP      (DPD_KEA_CRYPT_GROUP + 8) (OpId 0xa008) */
    /* PE, f9, CICV                                                             */
    /* 0x75200012,                                                              */
    ((EU_KEA | KEA_F9 | KEA_ICV | KEA_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_KEA_f9_CIPHER_INIT_FINAL_CMP (DPD_KEA_CRYPT_GROUP + 9) (OpId 0xa009) */
    /* INT, PE, f9, CICV                                                        */
    /* 0x75a00012,                                                              */
    ((EU_KEA | KEA_F9 | KEA_INIT | KEA_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_INBOUND
};



/**
 * Header constants for SNOW3G_CRYPT_REQ / DPD_SNOW3G_CRYPT_GROUP
 */

const unsigned long SnoReq[1 * NUM_SNOW3G_CRYPT_DESC] = {
    /* DPD_SNOW3G_f8_INIT        (DPD_SNOW3G_CRYPT_GROUP + 0) (OpId 0xc000) */
    /* INT, f8                                                              */
    ((EU_SNOW | SNOW_F8 | SNOW_INIT) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f8                (DPD_SNOW3G_CRYPT_GROUP + 1) (OpId 0xa001) */
    /* f8                                                                   */
    ((EU_SNOW | SNOW_F8) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_INIT           (DPD_SNOW3G_CRYPT_GROUP + 2) (OpId 0xc002) */
    /* INT, f9                                                              */
    ((EU_SNOW | SNOW_F9 | SNOW_INIT) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9                (DPD_SNOW3G_CRYPT_GROUP + 3) (OpId 0xc003) */
    /* f9                                                                   */
    ((EU_SNOW | SNOW_F9) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_FINAL          (DPD_SNOW3G_CRYPT_GROUP + 4) (OpId 0xc004) */
    /* PE, f9                                                               */
    ((EU_SNOW | SNOW_F9 | SNOW_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_INIT_FINAL     (DPD_SNOW3G_CRYPT_GROUP + 5) (OpId 0xc005) */
    /* INT, PE, f9                                                          */
    ((EU_SNOW | SNOW_F9 | SNOW_INIT | SNOW_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_OUTBOUND,

    /* DPD_KEA_f9_FINAL_CMP      (DPD_SNOW3G_CRYPT_GROUP + 8) (OpId 0xc008) */
    /* PE, f9, CICV                                                         */
    ((EU_SNOW | SNOW_F9 | SNOW_ICV | SNOW_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_INBOUND,

    /* DPD_KEA_f9_INIT_FINAL_CMP (DPD_SNOW3G_CRYPT_GROUP + 9) (OpId 0xc009) */
    /* INT, PE, f9, CICV                                                    */
    ((EU_SNOW | SNOW_F9 | SNOW_INIT | SNOW_EOM) << EU_SHIFT_PRIMARY) |
    DESCTYPE_COMMON | HDR_INBOUND
};


/**
 * Header constants for RAID_XOR_REQ / DPD_RAID_XOR_REQ_GROUP
 */

const unsigned long RaidXorReq[1 * NUM_RAID_XOR_DESC] = {
    /* DPD_RAID_XOR                              OpId 0x6200 */
    ((EU_AES | AES_XOR) << EU_SHIFT_PRIMARY) |
    DESCTYPE_RAIDXOR,
};



/**
 * Header constants for TLS_BLOCK_INBOUND_REQ / DPD_TLS_BLOCK_INBOUND_GROUP
 */
const unsigned long TlsBlockInboundReq[1 * NUM_TLS_BLOCK_INBOUND_DESC] = {
    /* DPD_TLS_BLOCK_INBOUND_SDES_MD5            OpId 0x9000 */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x20431a8a,                                           */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_MD5_CMP        OpId 0x9001 */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, MD5     */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x20435a8a,                                           */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_SHA1           OpId 0x9002 */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2043188a,                                           */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_SHA1_CMP       OpId 0x9003 */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA1    */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2043588a,                                           */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_SHA256         OpId 0x9004 */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2043198a,                                           */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_SHA256_CMP     OpId 0x9005 */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA256  */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2043598a,                                           */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_MD5            OpId 0x9006 */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x20631a8a,                                           */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_MD5_CMP        OpId 0x9007 */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, MD5     */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x20635a8a,                                           */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_MD5) << EU_SHIFT_SECONDARY ) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_SHA1           OpId 0x9008 */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2063188a,                                           */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_SHA1_CMP       OpId 0x9009 */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA1    */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2063588a,                                           */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_SHA256         OpId 0x900a */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2063198a,                                           */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_SHA256_CMP     OpId 0x900b */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA256  */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2063598a,                                           */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_MD5_SMAC       OpId 0x900c */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, MD5           */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2043328a,                                           */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_MD5_SMAC_CMP   OpId 0x900d */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, CICV, MD5     */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2043728a,                                           */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_CICV | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_SHA1_SMAC      OpId 0x900e */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, SHA1          */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2043308a, */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_SDES_SHA1_SMAC_CMP  OpId 0x900f */
    /* Primary EU:   DES     Mode: Single, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, CICV, SHA1    */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2043708a, */
    ((EU_DES | DES_CBC | DES_SINGLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_CICV | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_MD5_SMAC       OpId 0x9010 */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, MD5           */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2063328a, */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_MD5_SMAC_CMP   OpId 0x9011 */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, CICV, MD5     */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2063728a, */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_CICV | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_SHA1_SMAC      OpId 0x9012 */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, SHA1          */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2063308a, */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_TDES_SHA1_SMAC_CMP  OpId 0x9013 */
    /* Primary EU:   DES     Mode: Triple, CBC-decrypt       */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, CICV, SHA1    */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x2063708a, */
    ((EU_DES | DES_CBC | DES_TRIPLE | DES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_CICV | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_MD5             OpId 0x9014 */
    /* Primary EU:   AES     Mode: CBC-decrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x60231a8a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_MD5_CMP         OpId 0x9015 */
    /* Primary EU:   AES     Mode: CBC-decrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, MD5     */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x60235a8a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_SHA1            OpId 0x9016 */
    /* Primary EU:   AES     Mode: CBC-decrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x6023188a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_SHA1_CMP        OpId 0x9017 */
    /* Primary EU:   AES     Mode: CBC-decrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA1    */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x6023588a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_SHA256          OpId 0x9018 */
    /* Primary EU:   AES     Mode: CBC-decrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x6023198a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_SHA256_CMP      OpId 0x9019 */
    /* Primary EU:   AES     Mode: CBC-decrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA256  */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x6023598a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_MD5_SMAC        OpId 0x901a */
    /* Primary EU:   AES     CBC-decrypt                     */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, MD5           */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x6023328a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_MD5_SMAC_CMP    OpId 0x901b */
    /* Primary EU:   AES     CBC-decrypt                     */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, CICV, MD5     */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x6023728a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_CICV | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_SHA1_SMAC       OpId 0x901c */
    /* Primary EU:   AES     CBC-decrypt                     */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, SHA1          */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x6023308a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,

    /* DPD_TLS_BLOCK_INBOUND_AES_SHA1_SMAC_CMP   OpId 0x901d */
    /* Primary EU:   AES     CBC-decrypt                     */
    /* Secondary EU: MDEU    Mode: INIT, SMAC, CICV, SHA1    */
    /* Type:         TLS/SSL Block, Inbound                  */
    /* 0x6023708a, */
    ((EU_AES | AES_CBC | AES_DECRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_SMAC | MD_CICV | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_INBOUND,
};





/**
 * Header constants for TLS_BLOCK_OUTBOUND_REQ / DPD_TLS_BLOCK_OUTBOUND_GROUP
 */
const unsigned long TlsBlockOutboundReq[1 * NUM_TLS_BLOCK_OUTBOUND_DESC] = {
    /* DPD_TLS_BLOCK_OUTBOUND_SDES_MD5           OpId 0x9100 */
    /* Primary EU:   DES     Mode: Single, CBC-encrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x20531a88, */
    ((EU_DES | DES_SINGLE | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

    /* DPD_TLS_BLOCK_OUTBOUND_SDES_SHA1          OpId 0x9101 */
    /* Primary EU:   DES     Mode: Single, CBC-encrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x20531888, */
    ((EU_DES | DES_SINGLE | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

    /* DPD_TLS_BLOCK_OUTBOUND_SDES_SHA256        OpId 0x9102 */
    /* Primary EU:   DES     Mode: Single, CBC-encrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x20531988, */
    ((EU_DES | DES_SINGLE | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

    /* DPD_TLS_BLOCK_OUTBOUND_TDES_MD5           OpId 0x9103 */
    /* Primary EU:   DES     Mode: Triple, CBC-encrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x20731a88, */
    ((EU_DES | DES_TRIPLE | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

    /* DPD_TLS_BLOCK_OUTBOUND_TDES_SHA1          OpId 0x9104 */
    /* Primary EU:   DES     Mode: Triple, CBC-encrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x20731888, */
    ((EU_DES | DES_TRIPLE | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

    /* DPD_TLS_BLOCK_OUTBOUND_TDES_SHA256        OpId 0x9105 */
    /* Primary EU:   DES     Mode: Triple, CBC-encrypt       */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x20731988, */
    ((EU_DES | DES_TRIPLE | DES_CBC | DES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

    /* DPD_TLS_BLOCK_OUTBOUND_AES_MD5            OpId 0x9106 */
    /* Primary EU:   AES     Mode: CBC-encrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x60331a88, */
    ((EU_AES | AES_CBC | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

    /* DPD_TLS_BLOCK_OUTBOUND_AES_SHA1           OpId 0x9107 */
    /* Primary EU:   AES     Mode: CBC-encrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x60331888, */
    ((EU_AES | AES_CBC | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

    /* DPD_TLS_BLOCK_OUTBOUND_AES_SHA256         OpId 0x9108 */
    /* Primary EU:   AES     Mode: CBC-encrypt               */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Block, Outbound                 */
    /* 0x60331988, */
    ((EU_AES | AES_CBC | AES_ENCRYPT) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_BLOCK | HDR_OUTBOUND,

};





/**
 * Header constants for TLS_STREAM_INBOUND_REQ / DPD_TLS_STREAM_INBOUND_GROUP
 */
const unsigned long TlsStreamInboundReq[1 * NUM_TLS_STREAM_INBOUND_DESC] = {
    /* DPD_TLS_STREAM_INBOUND_MD5                OpId 0x9200 */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x10031a9a, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_CTX_MD5            OpId 0x9201 */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x10731a9a, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_SHA1               OpId 0x9202 */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x1003189a, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_CTX_SHA1           OpId 0x9203 */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x1073189a, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_SHA256             OpId 0x9204 */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x1003199a, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_CTX_SHA256         OpId 0x9205 */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x1073199a, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_MD5_CMP            OpId 0x9206 */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, MD5     */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x10035a9a, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_CTX_MD5_CMP        OpId 0x9207 */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, MD5     */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x10735a9a, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_SHA1_CMP           OpId 0x9208 */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA1    */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x1003589a, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_CTX_SHA1_CMP       OpId 0x9209 */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA1    */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x1073589a, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_SHA256_CMP         OpId 0x920a */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA256  */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x1003599a, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,

    /* DPD_TLS_STREAM_INBOUND_CTX_SHA256_CMP     OpId 0x920b */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, CICV, SHA256  */
    /* Type:         TLS/SSL Stream, Inbound                 */
    /* 0x1073599a, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_CICV | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_INBOUND,
};




/**
 * Header constants for TLS_STREAM_OUTBOUND_REQ / DPD_TLS_STREAM_OUTBOUND_GROUP
 */
const unsigned long TlsStreamOutboundReq[1 * NUM_TLS_STREAM_OUTBOUND_DESC] = {
    /* DPD_TLS_STREAM_OUTBOUND_MD5               OpId 0x9300 */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Stream, Outbound                */
    /* 0x10031a98, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_OUTBOUND,

    /* DPD_TLS_STREAM_OUTBOUND_SHA1              OpId 0x9301 */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Stream, Outbound                */
    /* 0x10031898, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_OUTBOUND,

    /* DPD_TLS_STREAM_OUTBOUND_SHA256            OpId 0x9302 */
    /* Primary EU:   AFEU    Mode: (none)                    */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA256        */
    /* Type:         TLS/SSL Stream, Outbound                */
    /* 0x10031998, */
    ((EU_ARC4) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_OUTBOUND,

    /* DPD_TLS_STREAM_OUTBOUND_CTX_MD5           OpId 0x9303 */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, MD5           */
    /* Type:         TLS/SSL Stream, Outbound                */
    /* 0x10731a98, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_MD5) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_OUTBOUND,

    /* DPD_TLS_STREAM_OUTBOUND_CTX_SHA1          OpId 0x9304 */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA1          */
    /* Type:         TLS/SSL Stream, Outbound                */
    /* 0x10731898, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA1) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_OUTBOUND,

    /* DPD_TLS_STREAM_OUTBOUND_CTX_SHA256        OpId 0x9305 */
    /* Primary EU:   AFEU    Mode: CS, DC, PP                */
    /* Secondary EU: MDEU    Mode: INIT, HMAC, SHA156        */
    /* Type:         TLS/SSL Stream, Outbound                */
    /* 0x10731998, */
    ((EU_ARC4 | ARC4_PERMUTE_INHIBIT | ARC4_DUMP_CONTEXT | ARC4_FETCH_CTX) << EU_SHIFT_PRIMARY) |
    ((EU_MD | MD_INIT | MD_HMAC | MD_SHA256) << EU_SHIFT_SECONDARY) |
    DESCTYPE_TLS_STREAM | HDR_OUTBOUND,
};


#define DES_STD_BLOCKSIZE      (8)
#define ARC4_STD_MIN_KEYBYTES  (1)
#define ARC4_STD_MAX_KEYBYTES  (16)
#define ARC4_STD_CONTEXTBYTES  (259)



#define STD_OFFSETS(s,l1,l2,p1,p2)  offsetof(s,l1), offsetof(s,l2),\
    offsetof(s,p1), 0
#define EXT_OFFSETS(s,l1,l2,p1,p2)  offsetof(s,l1), offsetof(s,l2),\
    offsetof(s,p1), offsetof(s,p2)
#define NULL_PTR_OFFSETS(s,l1,l2)   offsetof(s,l1), offsetof(s,l2), 0, 0
#define ZERO_LEN_OFFSETS(s,p1,p2)   0, 0, offsetof(s,p1), offsetof(s,p2)
#define ALL_ZERO_OFFSETS            0, 0, 0, 0

static char NIL[] = {"NIL"};



/*! \enum FLD_TYPE
  \brief A field can either be for reading from or writing to
 */
typedef enum {
    Read,
    Write,
    Extent
} FLD_TYPE;


/**
 * DPD_FLD_DETAILS_ENTRY
 *    Describes where and how a field in a request goes to a field in a DPD
 */
typedef struct
{
    char                    *txt;               /**< Description of the
                                                 * field within the
                                                 * request a NULL
                                                 * indicates the end
                                                 * of field entries   */
    unsigned int            lenOffset1st;       /**< Offset into request
                                                 * pointer for the
                                                 * initial length field
                                                 */
    unsigned int            lenOffsetNxt;       /**< Offset into request
                                                 * pointer for the next
                                                 * length field. Used
                                                 * when input points to
                                                 * output of the
                                                 * previous request   */
    unsigned int            ptrOffset1st;       /**< Offset into request
                                                 * pointer for the
                                                 * initial data area  */
    unsigned int            extOffset;
    FLD_TYPE                dataType;           /**< Data type either:
                                                 * a "Read" or "Write"
                                                 * area               */
    BOOLEAN                 (*pFncSize)(unsigned long len);
    /**< Pointer to function that checks
     * whether the length is
     * consistent with the request */
} DPD_FLD_DETAILS_ENTRY;

/**
 * DPD_DETAILS_ENTRY
 * Describes how a request is broken into a single DPD or a set of chained
 * DPDs
 */
typedef struct DPD_DETAILS_ENTRY
{
    unsigned long            opId;               /**< Operation ID for
                                                  * entry             */
    char                    *txt;                /**< Description of
                                                  * request a NULL
                                                  * indicates the end
                                                  * of the table      */
    unsigned long            sz;                 /**< Size of request */
    const unsigned long     *hdrDesc;            /**< Descriptor Header
                                                  */
    unsigned int             lenOffsetBlockLen;  /**< Offset into
                                                  * request pointer
                                                  * for total length of
                                                  * data              */
    DPD_FLD_DETAILS_ENTRY    fld[NUM_DPD_FLDS];
} DPD_DETAILS_ENTRY;        /* Each request is enumerated here */


BOOLEAN ChkDesIvLen(unsigned long len);
BOOLEAN ChkDesKeyLen(unsigned long len);
BOOLEAN ChkDesStaticDataLen(unsigned long len);
BOOLEAN ChkDesDataLen(unsigned long len);
BOOLEAN ChkDesCryptLen(unsigned long len);
BOOLEAN ChkDesCtxLen(unsigned long len);
BOOLEAN ChkArcKeyLen(unsigned long len);
BOOLEAN ChkArcCtxLen(unsigned long len);
BOOLEAN ChkOptionalArcCtxLen(unsigned long len);
BOOLEAN ChkEccLen(unsigned long len);
BOOLEAN ChkAesIvLen(unsigned long len);
BOOLEAN ChkAesKeyLen(unsigned long len);
BOOLEAN ChkCcmpKeyLen(unsigned long len);


DPD_DETAILS_ENTRY DpdDetails[] =
{


    /*
     * DPD_RNG_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  (none)                (none)             (none)
     * p2  (none)                (none)             (none)
     * p3  (none)                (none)             (none)
     * p4  rngData               rngBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_RNG_GROUP,
        "DPD_RNG_GROUP",
        sizeof(RNG_REQ),
        RngDesc,
        offsetof(RNG_REQ, rngBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                           Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                           Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                           Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                           Read,  NULL},
            {"rngData",     STD_OFFSETS(RNG_REQ, rngBytes, rngBytes, rngData, rngData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                           Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                           Read,  NULL}
        },
    },


    /*
     * DPD_DES_CBC_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  inIvData              inIvBytes          (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            (none)
     * p4  outData               outBytes           (none)
     * p5  outIvData             outIvBytes         (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_DES_CBC_GROUP,
        "DPD_DES_CBC_GROUP",
        sizeof(DES_CBC_CRYPT_REQ),
        DesCbcReq,
        offsetof(DES_CBC_CRYPT_REQ, inBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                             Read,  NULL},
            {"inIvData",    STD_OFFSETS(DES_CBC_CRYPT_REQ, inIvBytes,  inIvBytes,  inIvData,  outIvData), Read,  ChkDesIvLen},
            {"keyData",     STD_OFFSETS(DES_CBC_CRYPT_REQ, keyBytes,   keyBytes,   keyData,   keyData),   Read,  ChkDesKeyLen},
            {"inData",      STD_OFFSETS(DES_CBC_CRYPT_REQ, inBytes,    inBytes,    inData,    inData),    Read,  ChkDesDataLen},
            {"outData",     STD_OFFSETS(DES_CBC_CRYPT_REQ, inBytes,    inBytes,    outData,   outData),   Write, NULL},
            {"outIvData",   STD_OFFSETS(DES_CBC_CRYPT_REQ, outIvBytes, outIvBytes, outIvData, outIvData), Write, ChkDesIvLen},
            {NIL,           ALL_ZERO_OFFSETS,                                                             Read,  NULL}
        },
    },




    /*
     * DPD_DES_ECB_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  (none)                (none)             (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            (none)
     * p4  outData               inBytes            (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_DES_ECB_GROUP,
        "DPD_DES_ECB_GROUP",
        sizeof(DES_CRYPT_REQ),
        DesReq,
        offsetof(DES_CRYPT_REQ, inBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                 Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                 Read,  NULL},
            {"keyData",     STD_OFFSETS(DES_CRYPT_REQ, keyBytes, keyBytes, keyData, keyData), Read,  ChkDesKeyLen},
            {"inData",      STD_OFFSETS(DES_CRYPT_REQ, inBytes,  inBytes,  inData,  inData),  Read,  ChkDesDataLen},
            {"outData",     STD_OFFSETS(DES_CRYPT_REQ, inBytes,  inBytes,  outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                 Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                 Read,  NULL}
        },
    },




    /*
     * DPD_RC4_LDCTX_CRYPT_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  inCtxData             inCtxBytes         (none)
     * p2  (none)                (none)             (none)
     * p3  inData                inBytes            (none)
     * p4  outData               inBytes            (none)
     * p5  outCtxData            outCtxBytes        (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_RC4_LDCTX_CRYPT_ULCTX_GROUP,
        "DPD_RC4_LDCTX_CRYPT_ULCTX_GROUP",
        sizeof(ARC4_LOADCTX_CRYPT_REQ),
        Rc4LoadCtxUnloadCtxReq,
        offsetof(ARC4_LOADCTX_CRYPT_REQ, inBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                                      Read,  NULL},
            {"inCtxData",   STD_OFFSETS(ARC4_LOADCTX_CRYPT_REQ, inCtxBytes,  inCtxBytes,  inCtxData,  outCtxData), Read,  ChkArcCtxLen},
            {NIL,           ALL_ZERO_OFFSETS,                                                                      Read,  NULL},
            {"inData",      STD_OFFSETS(ARC4_LOADCTX_CRYPT_REQ, inBytes,     inBytes,     inData,     inData),     Read,  NULL},
            {"outData",     STD_OFFSETS(ARC4_LOADCTX_CRYPT_REQ, inBytes,     inBytes,     outData,    outData),    Write, NULL},
            {"outCtxData",  STD_OFFSETS(ARC4_LOADCTX_CRYPT_REQ, outCtxBytes, outCtxBytes, outCtxData, outCtxData), Write, ChkArcCtxLen},
            {NIL,           ALL_ZERO_OFFSETS,                                                                      Read,  NULL}
        },
    },




    /*
     * DPD_RC4_LDKEY_CRYPT_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  (none)                (none)             (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            (none)
     * p4  outData               inBytes            (none)
     * p5  outCtxData            outCtxBytes        (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_RC4_LDKEY_CRYPT_ULCTX_GROUP,
        "DPD_RC4_LDKEY_CRYPT_ULCTX_GROUP",
        sizeof(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ),
        Rc4LoadKeyUnloadCtxReq,
        offsetof(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, inBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                                                Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                                                Read,  NULL},
            {"keyData",     STD_OFFSETS(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, keyBytes,    keyBytes,    keyData,    keyData),    Read,  ChkArcKeyLen},
            {"inData",      STD_OFFSETS(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, inBytes,     inBytes,     inData,     inData),     Read,  NULL},
            {"outData",     STD_OFFSETS(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, inBytes,     inBytes,     outData,    outData),    Write, NULL},
            {"outCtxData",  STD_OFFSETS(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, outCtxBytes, outCtxBytes, outCtxData, outCtxData), Write, ChkArcCtxLen},
            {NIL,           ALL_ZERO_OFFSETS,                                                                                Read,  NULL}
        },
    },




    /*
     * DPD_HASH_LDCTX_HASH_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  ctxData               ctxBytes           (none)
     * p2  (none)                (none)             (none)
     * p3  inData                inBytes            (none)
     * p4  cmpData               outBytes           (none)
     * p5  outData               outBytes           (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_HASH_LDCTX_HASH_ULCTX_GROUP,
        "DPD_HASH_LDCTX_HASH_ULCTX_GROUP",
        sizeof(HASH_REQ),
        MdhaReq,
        offsetof(HASH_REQ, inBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL},
            {"ctxData",     STD_OFFSETS(HASH_REQ, ctxBytes, outBytes, ctxData, outData), Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL},
            {"inData",      STD_OFFSETS(HASH_REQ, inBytes,  inBytes,  inData,  inData),  Read,  NULL},
            {"cmpData",     STD_OFFSETS(HASH_REQ, outBytes, outBytes, cmpData, cmpData), Read,  NULL},
            {"outData",     STD_OFFSETS(HASH_REQ, outBytes, outBytes, outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL}
        },
    },




    /*
     * DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  ctxData               ctxBytes           (none)
     * p2  (none)                (none)             (none)
     * p3  inData                inBytes            (none)
     * p4  cmpData               outBytes           (none)
     * p5  outData               outBytes           (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP,
        "DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP",
        sizeof(HASH_REQ),
        MdhaPadReq,
        offsetof(HASH_REQ, inBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL},
            {"ctxData",     STD_OFFSETS(HASH_REQ, ctxBytes, ctxBytes, ctxData, ctxData), Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL},
            {"inData",      STD_OFFSETS(HASH_REQ, inBytes,  inBytes,  inData,  inData),  Read,  NULL},
            {"cmpData",     STD_OFFSETS(HASH_REQ, outBytes, outBytes, cmpData, cmpData), Read,  NULL},
            {"outData",     STD_OFFSETS(HASH_REQ, outBytes, outBytes, outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL}
        },
    },




    /*
     * DPD_HASH_LDCTX_HMAC_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  (none)                (none)             (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            (none)
     * p4  cmpData               outBytes           (none)
     * p5  outData               outBytes           (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_HASH_LDCTX_HMAC_ULCTX_GROUP,
        "DPD_HASH_LDCTX_HMAC_ULCTX_GROUP",
        sizeof(HMAC_PAD_REQ),
        HmacPadReq,
        offsetof(HMAC_PAD_REQ, inBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                Read,  NULL},
            {"keyData",     STD_OFFSETS(HMAC_PAD_REQ, keyBytes, keyBytes, keyData, keyData), Read,  NULL},
            {"inData",      STD_OFFSETS(HMAC_PAD_REQ, inBytes,  inBytes,  inData,  inData),  Read,  NULL},
            {"cmpData",     STD_OFFSETS(HMAC_PAD_REQ, outBytes, outBytes, cmpData, cmpData), Read,  NULL},
            {"outData",     STD_OFFSETS(HMAC_PAD_REQ, outBytes, outBytes, outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                Read,  NULL}
        },
    },




    /*
     * DPD_MM_LDCTX_EXP_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  modData               modBytes           (none)
     * p1  (none)                (none)             (none)
     * p2  aData                 aDataBytes         (none)
     * p3  expData               expBytes           (none)
     * p4  outData               outBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_MM_LDCTX_EXP_ULCTX_GROUP,
        "DPD_MM_LDCTX_EXP_ULCTX_GROUP",
        sizeof(MOD_EXP_REQ),
        PkhaMmExpReq,
        offsetof(MOD_EXP_REQ, outBytes),
        {
            {"modData",     STD_OFFSETS(MOD_EXP_REQ, modBytes,   modBytes,   modData, modData), Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL},
            {"aData",       STD_OFFSETS(MOD_EXP_REQ, aDataBytes, aDataBytes, aData,   aData),   Read,  NULL},
            {"expData",     STD_OFFSETS(MOD_EXP_REQ, expBytes,   expBytes,   expData, expData), Read,  NULL},
            {"outData",     STD_OFFSETS(MOD_EXP_REQ, outBytes,   outBytes,   outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL}
        },
    },




    /*
     * DPD_MM_SS_RSA_EXP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  modData               modBytes           (none)
     * p1  (none)                (none)             (none)
     * p2  aData                 aDataBytes         (none)
     * p3  expData               expBytes           (none)
     * p4  bData                 bDataBytes         (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_MM_SS_RSA_EXP,
        "DPD_MM_SS_RSA_EXP",
        sizeof(MOD_SS_EXP_REQ),
        PkhaMmSsExpReq,
        0,
        {
            {"modData",     STD_OFFSETS(MOD_SS_EXP_REQ, modBytes,   modBytes,   modData, modData), Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                      Read,  NULL},
            {"aData",       STD_OFFSETS(MOD_SS_EXP_REQ, aDataBytes, aDataBytes, aData,   aData),   Read,  NULL},
            {"expData",     STD_OFFSETS(MOD_SS_EXP_REQ, expBytes,   expBytes,   expData, expData), Read,  NULL},
            {"bData",       STD_OFFSETS(MOD_SS_EXP_REQ, bDataBytes, bDataBytes, bData,   bData),   Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                      Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                      Read,  NULL}
        },
    },




    /*
     * DPD_MM_LDCTX_R2MODN_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  modData               modBytes           (none)
     * p1  (none)                (none)             (none)
     * p2  (none)                (none)             (none)
     * p3  (none)                (none)             (none)
     * p4  outData               outBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_MM_LDCTX_R2MODN_ULCTX_GROUP,
        "DPD_MM_LDCTX_R2MODN_ULCTX_GROUP",
        sizeof(MOD_R2MODN_REQ),
        PkhaModR2modnReq,
        offsetof(MOD_R2MODN_REQ, outBytes),
        {
            {"modData",     STD_OFFSETS(MOD_R2MODN_REQ, modBytes, modBytes, modData, modData), Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {"outData",     STD_OFFSETS(MOD_R2MODN_REQ, outBytes, outBytes, outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL}
        },
    },




    /*
     * DPD_MM_LDCTX_RRMODP_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  pData                 pBytes             (none)
     * p1  (none)                (none)             (none)
     * p2  (none)                (none)             (none)
     * p3  NULL                  nBytes             (none)
     * p4  outData               outBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_MM_LDCTX_RRMODP_ULCTX_GROUP,
        "DPD_MM_LDCTX_RRMODP_ULCTX_GROUP",
        sizeof(MOD_RRMODP_REQ),
        PkhaMmRrmodpReq,
        offsetof(MOD_RRMODP_REQ, outBytes),
        {
            {"pData",       STD_OFFSETS(MOD_RRMODP_REQ,      pBytes,   pBytes,   pData,   pData),   Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                       Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                       Read,  NULL},
            {"nBytes",      NULL_PTR_OFFSETS(MOD_RRMODP_REQ, nBytes,   nBytes),                     Read,  NULL},
            {"outData",     STD_OFFSETS(MOD_RRMODP_REQ,      outBytes, outBytes, outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                       Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                       Read,  NULL}
        },
    },




    /*
     * DPD_MM_MOD_INV_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  nData                 nBytes             (none)
     * p1  (none)                (none)             (none)
     * p2  inData                inBytes            (none)
     * p3  (none)                (none)             (none)
     * p4  outData               outBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_MM_MOD_INV_ULCTX_GROUP,
        "DPD_MM_MOD_INV_ULCTX_GROUP",
        sizeof(MOD_INV_REQ),
        PkhaMmModInvReq,
        offsetof(MOD_INV_REQ, outBytes),
        {
            {"nData",       STD_OFFSETS(MOD_INV_REQ,      nBytes,   nBytes,   nData,   nData), Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {"inData",      STD_OFFSETS(MOD_INV_REQ,      inBytes, inBytes, inData, inData),   Read, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {"outData",     STD_OFFSETS(MOD_INV_REQ,      outBytes, outBytes, outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL}
        },
    },


    /*
     * DPD_F2M_INV_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  nData                 nBytes             (none)
     * p1  (none)                (none)             (none)
     * p2  inData                inBytes            (none)
     * p3  (none)                (none)             (none)
     * p4  outData               outBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */

    {
        DPD_F2M_INV_ULCTX_GROUP,
        "DPD_F2M_INV_ULCTX_GROUP",
        sizeof(F2M_INV_REQ),
        PkhaF2MInvReq,
        offsetof(F2M_INV_REQ, outBytes),
        {
            {"nData",       STD_OFFSETS(MOD_INV_REQ,      nBytes,   nBytes,   nData,   nData), Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {"inData",      STD_OFFSETS(MOD_INV_REQ,      inBytes, inBytes, inData, inData),   Read, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {"outData",     STD_OFFSETS(MOD_INV_REQ,      outBytes, outBytes, outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                  Read,  NULL}
        },
    },





    /*
     * DPD_MM_LDCTX_2OP_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  modData               modBytes           (none)
     * p1  bData                 bDataBytes         (none)
     * p2  aData                 aDataBytes         (none)
     * p3  (none)                (none)             (none)
     * p4  outData               outBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_MOD_LDCTX_2OP_ULCTX_GROUP,
        "DPD_MOD_LDCTX_2OP_ULCTX_GROUP",
        sizeof(MOD_2OP_REQ),
        PkhaMod2OpReq,
        offsetof(MOD_2OP_REQ, outBytes),
        {
            {"modData",     STD_OFFSETS(MOD_2OP_REQ, modBytes,   modBytes,   modData, modData), Read,  NULL},
            {"bData",       STD_OFFSETS(MOD_2OP_REQ, bDataBytes, bDataBytes, bData,   bData),   Read,  NULL},
            {"aData",       STD_OFFSETS(MOD_2OP_REQ, aDataBytes, aDataBytes, aData,   aData),   Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL},
            {"outData",     STD_OFFSETS(MOD_2OP_REQ, outBytes,   outBytes,   outData, outData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL}
        },
    },




    /*
     * DPD_EC_LDCTX_kP_ULCTX_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  nData                 nDataBytes         (none)
     * p1  eData                 eDataBytes         (none)
     * p2  buildData             buildDataBytes     (none)
     * p3  b1Data                b1DataBytes        (none)
     * p4  b2Data                b2DataBytes        (none)
     * p5  b3Data                b3DataBytes        (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_EC_LDCTX_kP_ULCTX_GROUP,
        "DPD_EC_LDCTX_kP_ULCTX_GROUP",
        sizeof(ECC_POINT_REQ),
        PkhaEccPointReq,
        offsetof(ECC_POINT_REQ, b3DataBytes),
        {
            {"nData",     STD_OFFSETS(ECC_POINT_REQ, nDataBytes,     nDataBytes,     nData,     nData),     Read,  NULL},
            {"eData",     STD_OFFSETS(ECC_POINT_REQ, eDataBytes,     eDataBytes,     eData,     eData),     Read,  NULL},
            {"buildData", STD_OFFSETS(ECC_POINT_REQ, buildDataBytes, buildDataBytes, buildData, buildData), Read,  NULL},
            {"b1Data",    STD_OFFSETS(ECC_POINT_REQ, b1DataBytes,    b1DataBytes,    b1Data,    b1Data),    Write, NULL},
            {"b2Data",    STD_OFFSETS(ECC_POINT_REQ, b2DataBytes,    b2DataBytes,    b2Data,    b2Data),    Write, NULL},
            {"b3Data",    STD_OFFSETS(ECC_POINT_REQ, b3DataBytes,    b3DataBytes,    b3Data,    b3Data),    Write, NULL},
            {NIL,         ALL_ZERO_OFFSETS,                                                                 Read,  NULL}
        },
    },




    /*
     * DPD_EC_2OP_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  bData                 bDataBytes         (none)
     * p1  aData                 aDataBytes         (none)
     * p2  modData               modBytes           (none)
     * p3  outData               outBytes           (none)
     * p4  (none)                (none)             (none)
     * p5  (none)                (none)             (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_EC_2OP_GROUP,
        "DPD_EC_2OP_GROUP",
        sizeof(ECC_2OP_REQ),
        PkhaEcc2OpReq,
        offsetof(ECC_2OP_REQ, outBytes),
        {
            {"modData",     STD_OFFSETS(ECC_2OP_REQ, modBytes,   modBytes,   modData, modData), Read,  ChkEccLen},
            {"bData",       STD_OFFSETS(ECC_2OP_REQ, bDataBytes, bDataBytes, bData,   bData),   Read,  ChkEccLen},
            {"aData",       STD_OFFSETS(ECC_2OP_REQ, aDataBytes, aDataBytes, aData,   aData),   Read,  ChkEccLen},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL},
            {"outData",     STD_OFFSETS(ECC_2OP_REQ, outBytes,   outBytes,   outData, outData), Write, ChkEccLen},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                   Read,  NULL}
        },
    },




    /*
     * DPD_EC_SPKBUILD_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  a0Data                a0DataBytes        (none)
     * p1  a1Data                a1DataBytes        (none)
     * p2  a2Data                a2DataBytes        (none)
     * p3  a3Data                a3DataBytes        (none)
     * p4  b0Data                b0DataBytes        (none)
     * p5  b1Data                b1DataBytes        (none)
     * p6  buildData             buildDataBytes     (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_EC_SPKBUILD_GROUP,
        "DPD_EC_SPKBUILD_GROUP",
        sizeof(ECC_SPKBUILD_REQ),
        PkhaEccSpkbuildReq,
        offsetof(ECC_SPKBUILD_REQ, buildDataBytes),
        {
            {"a0Data",    STD_OFFSETS(ECC_SPKBUILD_REQ, a0DataBytes,    a0DataBytes,    a0Data,    a0Data),    Read,  NULL},
            {"a1Data",    STD_OFFSETS(ECC_SPKBUILD_REQ, a1DataBytes,    a1DataBytes,    a1Data,    a1Data),    Read,  NULL},
            {"a2Data",    STD_OFFSETS(ECC_SPKBUILD_REQ, a2DataBytes,    a2DataBytes,    a2Data,    a2Data),    Read,  NULL},
            {"a3Data",    STD_OFFSETS(ECC_SPKBUILD_REQ, a3DataBytes,    a3DataBytes,    a3Data,    a3Data),    Read,  NULL},
            {"b0Data",    STD_OFFSETS(ECC_SPKBUILD_REQ, b0DataBytes,    b0DataBytes,    b0Data,    b0Data),    Read,  NULL},
            {"b1Data",    STD_OFFSETS(ECC_SPKBUILD_REQ, b1DataBytes,    b1DataBytes,    b1Data,    b1Data),    Read,  NULL},
            {"buildData", STD_OFFSETS(ECC_SPKBUILD_REQ, buildDataBytes, buildDataBytes, buildData, buildData), Write, NULL},
        },
    },




    /*
     * DPD_EC_PTADD_DBL_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  modData               modBytes           (none)
     * p1  buildData             buildDataBytes     (none)
     * p2  b2InData              b2InDataBytes      (none)
     * p3  b3InData              b3InDataBytes      (none)
     * p4  b1Data                b1DataBytes        (none)
     * p5  b2Data                b2DataBytes        (none)
     * p6  b3Data                b3DataBytes        (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_EC_PTADD_DBL_GROUP,
        "DPD_EC_PTADD_DBL_GROUP",
        sizeof(ECC_PTADD_DBL_REQ),
        PkhaEccPtaddDblReq,
        offsetof(ECC_PTADD_DBL_REQ, b3DataBytes),
        {
            {"modData",   STD_OFFSETS(ECC_PTADD_DBL_REQ, modBytes,       modBytes,       modData,   modData),   Read,  NULL},
            {"buildData", STD_OFFSETS(ECC_PTADD_DBL_REQ, buildDataBytes, buildDataBytes, buildData, buildData), Read,  NULL},
            {"b2InData",  STD_OFFSETS(ECC_PTADD_DBL_REQ, b2InDataBytes,  b2InDataBytes,  b2InData,  b2InData),  Read,  NULL},
            {"b3InData",  STD_OFFSETS(ECC_PTADD_DBL_REQ, b3InDataBytes,  b3InDataBytes,  b3InData,  b3InData),  Read,  NULL},
            {"b1Data",    STD_OFFSETS(ECC_PTADD_DBL_REQ, b1DataBytes,    b1DataBytes,    b1Data,    b1Data),    Write, NULL},
            {"b2Data",    STD_OFFSETS(ECC_PTADD_DBL_REQ, b2DataBytes,    b2DataBytes,    b2Data,    b2Data),    Write, NULL},
            {"b3Data",    STD_OFFSETS(ECC_PTADD_DBL_REQ, b3DataBytes,    b3DataBytes,    b3Data,    b3Data),    Write, NULL},
        },
    },





    /*
     * DPD_IPSEC_CBC_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes       (none)
     * p1  hashInData            hashInDataBytes    (none)
     * p2  cryptKeyData          cryptKeyBytes      (none)
     * p3  cryptCtxInData        cryptCtxInBytes    (none)
     * p4  inData                inDataBytes        (none)
     * p5  cryptDataOut          inDataBytes        (none)
     * p6  hashDataOut           hashDataOutBytes   (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_IPSEC_CBC_GROUP,
        "DPD_IPSEC_CBC_GROUP",
        sizeof(IPSEC_CBC_REQ),
        IpsecCbcReq,
        offsetof(IPSEC_CBC_REQ, inDataBytes),
        {
            {"hashKeyData",    STD_OFFSETS(IPSEC_CBC_REQ, hashKeyBytes,     hashKeyBytes,     hashKeyData,    hashKeyData),    Read,  NULL},
            {"hashInData",     STD_OFFSETS(IPSEC_CBC_REQ, hashInDataBytes,  hashInDataBytes,  hashInData,     hashInData),     Read,  NULL},
            {"cryptKeyData",   STD_OFFSETS(IPSEC_CBC_REQ, cryptKeyBytes,    cryptKeyBytes,    cryptKeyData,   cryptKeyData),   Read,  ChkDesKeyLen},
            {"cryptCtxInData", STD_OFFSETS(IPSEC_CBC_REQ, cryptCtxInBytes,  cryptCtxInBytes,  cryptCtxInData, cryptCtxInData), Read,  ChkDesCtxLen},
            {"inData",         STD_OFFSETS(IPSEC_CBC_REQ, inDataBytes,      inDataBytes,      inData,         inData),         Read,  ChkDesStaticDataLen},
            {"cryptDataOut",   STD_OFFSETS(IPSEC_CBC_REQ, inDataBytes,      inDataBytes,      cryptDataOut,   cryptDataOut),   Write, NULL},
            {"hashDataOut",    STD_OFFSETS(IPSEC_CBC_REQ, hashDataOutBytes, hashDataOutBytes, hashDataOut,    hashDataOut),    Write, NULL}
        },
    },





    /*
     * DPD_IPSEC_ECB_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes       (none)
     * p1  hashInData            hashInDataBytes    (none)
     * p2  cryptKeyData          cryptKeyBytes      (none)
     * p3  (none)                (none)             (none)
     * p4  inData                inDataBytes        (none)
     * p5  cryptDataOut          inDataBytes        (none)
     * p6  hashDataOut           hashDataOutBytes   (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_IPSEC_ECB_GROUP,
        "DPD_IPSEC_ECB_GROUP",
        sizeof(IPSEC_ECB_REQ),
        IpsecEcbReq,
        offsetof(IPSEC_ECB_REQ, inDataBytes),
        {
            {"hashKeyData",  STD_OFFSETS(IPSEC_ECB_REQ, hashKeyBytes,     hashKeyBytes,     hashKeyData,  hashKeyData),  Read,  NULL},
            {"hashInData",   STD_OFFSETS(IPSEC_ECB_REQ, hashInDataBytes,  hashInDataBytes,  hashInData,   hashInData),   Read,  NULL},
            {"cryptKeyData", STD_OFFSETS(IPSEC_ECB_REQ, cryptKeyBytes,    cryptKeyBytes,    cryptKeyData, cryptKeyData), Read,  ChkDesKeyLen},
            {NIL,            ALL_ZERO_OFFSETS,                                                                           Read,  NULL},
            {"inData",       STD_OFFSETS(IPSEC_ECB_REQ, inDataBytes,      inDataBytes,      inData,       inData),       Read,  ChkDesStaticDataLen},
            {"cryptDataOut", STD_OFFSETS(IPSEC_ECB_REQ, inDataBytes,      inDataBytes,      cryptDataOut, cryptDataOut), Write, NULL},
            {"hashDataOut",  STD_OFFSETS(IPSEC_ECB_REQ, hashDataOutBytes, hashDataOutBytes, hashDataOut,  hashDataOut),  Write, NULL}
        },
    },




    /*
     * DPD_IPSEC_ESP_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes       (none)
     * p1  hashInData            hashInDataBytes    (none)
     * p2  cryptCtxInData        cryptCtxInBytes    (none)
     * p3  cryptKeyData          cryptKeyBytes      (none)
     * p4  inData                inDataBytes        (none)
     * p5  cryptDataOut          inDataBytes        hashOutDataBytes
     * p6  cryptCtxOutData       cryptCtxOutBytes   (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_IPSEC_ESP_GROUP,
        "DPD_IPSEC_ESP_GROUP",
        sizeof(IPSEC_ESP_REQ),
        IpsecEspReq,
        offsetof(IPSEC_ESP_REQ, inDataBytes),
        {
            {"hashKeyData",     STD_OFFSETS(IPSEC_ESP_REQ, hashKeyBytes,     hashKeyBytes,     hashKeyData,     hashKeyData),      Read,  NULL},
            {"hashInData",      STD_OFFSETS(IPSEC_ESP_REQ, hashInDataBytes,  hashInDataBytes,  hashKeyData,     hashKeyData),      Read,  NULL},
            {"cryptCtxInData",  STD_OFFSETS(IPSEC_ESP_REQ, cryptCtxInBytes,  cryptCtxInBytes,  cryptCtxInData,  cryptCtxInData),   Read,  ChkDesCtxLen},
            {"cryptKeyData",    STD_OFFSETS(IPSEC_ESP_REQ, cryptKeyBytes,    cryptKeyBytes,    cryptKeyData,    cryptKeyData),     Read,  ChkDesKeyLen},
            {"inData",          STD_OFFSETS(IPSEC_ESP_REQ, inDataBytes,      inDataBytes,      inData,          inData),           Read,  ChkDesStaticDataLen},
            {"cryptDataOut",    EXT_OFFSETS(IPSEC_ESP_REQ, inDataBytes,      inDataBytes,      cryptDataOut,    hashDataOutBytes), Write, NULL},
            {"cryptCtxOutData", STD_OFFSETS(IPSEC_ESP_REQ, cryptCtxOutBytes, cryptCtxOutBytes, cryptCtxOutData, cryptCtxOutData),  Write, ChkDesCtxLen}
        },
    },




    /*
     * DPD_IPSEC_AES_CBC_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes       (none)
     * p1  hashInData            hashInDataBytes    (none)
     * p2  cryptKeyData          cryptKeyBytes      (none)
     * p3  cryptCtxInData        cryptCtxInBytes    (none)
     * p4  inData                inDataBytes        (none)
     * p5  cryptDataOut          inDataBytes        (none)
     * p6  hashDataOut           hashDataOutBytes   (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_IPSEC_AES_CBC_GROUP,
        "DPD_IPSEC_AES_CBC_GROUP",
        sizeof(IPSEC_AES_CBC_REQ),
        IpsecAesCbcReq,
        offsetof(IPSEC_AES_CBC_REQ, inDataBytes),
        {
            {"hashKeyData",    STD_OFFSETS(IPSEC_CBC_REQ, hashKeyBytes,     hashKeyBytes,     hashKeyData,    hashKeyData),    Read,  NULL},
            {"hashInData",     STD_OFFSETS(IPSEC_CBC_REQ, hashInDataBytes,  hashInDataBytes,  hashInData,     hashInData),     Read,  NULL},
            {"cryptKeyData",   STD_OFFSETS(IPSEC_CBC_REQ, cryptKeyBytes,    cryptKeyBytes,    cryptKeyData,   cryptKeyData),   Read,  ChkAesKeyLen},
            {"cryptCtxInData", STD_OFFSETS(IPSEC_CBC_REQ, cryptCtxInBytes,  cryptCtxInBytes,  cryptCtxInData, cryptCtxInData), Read,  ChkAesIvLen},
            {"inData",         STD_OFFSETS(IPSEC_CBC_REQ, inDataBytes,      inDataBytes,      inData,         inData),         Read,  ChkDesStaticDataLen},
            {"cryptDataOut",   STD_OFFSETS(IPSEC_CBC_REQ, inDataBytes,      inDataBytes,      cryptDataOut,   cryptDataOut),   Write, NULL},
            {"hashDataOut",    STD_OFFSETS(IPSEC_CBC_REQ, hashDataOutBytes, hashDataOutBytes, hashDataOut,    hashDataOut),    Write, NULL}
        },
    },




    /*
     * DPD_IPSEC_AES_ECB_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes       (none)
     * p1  hashInData            hashInDataBytes    (none)
     * p2  cryptKeyData          cryptKeyBytes      (none)
     * p3  (none)                (none)             (none)
     * p4  inData                inDataBytes        (none)
     * p5  cryptDataOut          inDataBytes        (none)
     * p6  hashDataOut           hashDataOutBytes   (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_IPSEC_AES_ECB_GROUP,
        "DPD_IPSEC_AES_ECB_GROUP",
        sizeof(IPSEC_AES_ECB_REQ),
        IpsecAesEcbReq,
        offsetof(IPSEC_AES_ECB_REQ, inDataBytes),
        {
            {"hashKeyData",  STD_OFFSETS(IPSEC_AES_ECB_REQ, hashKeyBytes,     hashKeyBytes,     hashKeyData,  hashKeyData),  Read,  NULL},
            {"hashInData",   STD_OFFSETS(IPSEC_AES_ECB_REQ, hashInDataBytes,  hashInDataBytes,  hashInData,   hashInData),   Read,  NULL},
            {"cryptKeyData", STD_OFFSETS(IPSEC_AES_ECB_REQ, cryptKeyBytes,    cryptKeyBytes,    cryptKeyData, cryptKeyData), Read,  ChkAesKeyLen},
            {NIL,            ALL_ZERO_OFFSETS,                                                                               Read,  NULL},
            {"inData",       STD_OFFSETS(IPSEC_AES_ECB_REQ, inDataBytes,      inDataBytes,      inData,       inData),       Read,  ChkDesStaticDataLen},
            {"cryptDataOut", STD_OFFSETS(IPSEC_AES_ECB_REQ, inDataBytes,      inDataBytes,      cryptDataOut, cryptDataOut), Write, NULL},
            {"hashDataOut",  STD_OFFSETS(IPSEC_AES_ECB_REQ, hashDataOutBytes, hashDataOutBytes, hashDataOut,  hashDataOut),  Write, NULL}
        },
    },





    /*
     * DPD_AESA_CRYPT_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  inIvData              inIvBytes          (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            (none)
     * p4  outData               inBytes            (none)
     * p5  outCtxData            outCtxBytes        (none)
     * p6  (none)                (none)             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_AESA_CRYPT_GROUP,
        "DPD_AESA_CRYPT_GROUP",
        sizeof(AESA_CRYPT_REQ),
        AesaDesc,
        offsetof(AESA_CRYPT_REQ, inBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                              Read,  NULL},
            {"inIvData",    STD_OFFSETS(AESA_CRYPT_REQ, inIvBytes,   inIvBytes,   inIvData,   inIvData),   Read,  ChkAesIvLen},
            {"keyData",     STD_OFFSETS(AESA_CRYPT_REQ, keyBytes,    keyBytes,    keyData,    keyData),    Read,  ChkAesKeyLen},
            {"inData",      STD_OFFSETS(AESA_CRYPT_REQ, inBytes,     inBytes,     inData,     inData),     Read,  0},
            {"outData",     STD_OFFSETS(AESA_CRYPT_REQ, inBytes,     inBytes,     outData,    outData),    Write, NULL},
            {"outCtxData",  STD_OFFSETS(AESA_CRYPT_REQ, outCtxBytes, outCtxBytes, outCtxData, outCtxData), Write, NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                                              Read,  NULL}
        },
    },


    /*
     * DPD_AESA_MAC_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  inCtxData             inCtxBytes         (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            (none)
     * p4  cmpIn                 cmpBytes           (none)
     * p5  outCtxData            outCtxBytes        (none)
     * p6  cmpOut                cmpBytes           (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_AESA_MAC_GROUP,
        "DPD_AESA_MAC_GROUP",
        sizeof(AESA_MAC_REQ),
        AesaMACDesc,
        offsetof(AESA_MAC_REQ, keyBytes),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                              Read,  NULL},
            {"inCtxData",   STD_OFFSETS(AESA_MAC_REQ, inCtxBytes,  inCtxBytes,   inCtxData,  inCtxData),   Read,  ChkAesIvLen},
            {"keyData",     STD_OFFSETS(AESA_MAC_REQ, keyBytes,    keyBytes,     keyData,    keyData),     Read,  ChkAesKeyLen},
            {"inData",      STD_OFFSETS(AESA_MAC_REQ, inBytes,     inBytes,      inData,     inData),      Read,  0},
            {"cmpIn",       STD_OFFSETS(AESA_MAC_REQ, cmpBytes,    cmpBytes,     cmpIn,      cmpIn),       Read,  NULL},
            {"outCtxData",  STD_OFFSETS(AESA_MAC_REQ, outCtxBytes, outCtxBytes,  outCtxData, outCtxData),  Write, NULL},
            {"cmpOut",      STD_OFFSETS(AESA_MAC_REQ, cmpBytes,    cmpBytes,     cmpOut,     cmpOut),      Write, NULL}
        },
    },



    /*
     * DPD_AESA_GCM_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  gcmCtxData            gcmCtxBytes        (none)
     * p1  AAD                   AADBytes           (none)
     * p2  ivIn                  ivBytes            (none)
     * p3  keyData               keyBytes           saltBytes
     * p4  inData                inBytes            (none)
     * p5  outData               outBytes           cmpBytes
     * p6  ivOut                 ivBytes            crcBytes
     * ----------------------------------------------------------------
     */
    {
        DPD_AESA_GCM_GROUP,
        "DPD_AESA_GCM_GROUP",
        sizeof(AESA_GCM_REQ),
        AesaGCMDesc,
        offsetof(AESA_GCM_REQ, gcmCtxBytes),
        {
            {"gcmCtxData",  STD_OFFSETS(AESA_GCM_REQ,   gcmCtxBytes, gcmCtxBytes, gcmCtxData, gcmCtxData), Read,  NULL},
            {"AAD",         STD_OFFSETS(AESA_GCM_REQ,   AADBytes,    AADBytes,    AAD,        AAD),        Read,  0},
            {"IVin",        STD_OFFSETS(AESA_GCM_REQ,   ivBytes,     ivBytes,     ivIn,       ivIn),     Read,  0},
            {"key",         EXT_OFFSETS(AESA_GCM_REQ,   keyBytes,    keyBytes,    keyData,    saltBytes),  Read,  0},
            {"inData",      STD_OFFSETS(AESA_GCM_REQ,   inBytes,     inBytes,     inData,     inData),     Read,  0},
            {"outData",     EXT_OFFSETS(AESA_GCM_REQ,   outBytes,    outBytes,    outData,    cmpBytes),   Write, NULL},
            {"IVout",       EXT_OFFSETS(AESA_GCM_REQ,   ivBytes,     ivBytes,     ivOut,      crcBytes),   Write, NULL},
        },
    },




    /*
     * DPD_CCMP_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  context               ctxBytes           (none)
     * p2  keyData               keyBytes           (none)
     * p3  AADData               AADBytes           (none)
     * p4  FrameData             FrameDataBytes     (none)
     * p5  cryptDataOut          cryptDataBytes     (none)
     * p6  MICData               MICBytes           (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_CCMP_GROUP,
        "DPD_CCMP_GROUP",
        sizeof(CCMP_REQ),
        CcmpReq,
        offsetof(CCMP_REQ, MICData),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                                  Read,  NULL},
            {"context",     STD_OFFSETS(CCMP_REQ, ctxBytes,       ctxBytes,       context,      context),      Read,  NULL},
            {"keyData",     STD_OFFSETS(CCMP_REQ, keyBytes,       keyBytes,       keyData,      keyData),      Read,  ChkCcmpKeyLen},
            {"AADData",     STD_OFFSETS(CCMP_REQ, AADBytes,       AADBytes,       AADData,      AADData),      Read,  0},
            {"FrameData",   STD_OFFSETS(CCMP_REQ, FrameDataBytes, FrameDataBytes, FrameData,    FrameData),    Read,  0},
            {"cryptDataOut",STD_OFFSETS(CCMP_REQ, cryptDataBytes, cryptDataBytes, cryptDataOut, cryptDataOut), Write, NULL},
            {"MICData",     STD_OFFSETS(CCMP_REQ, MICBytes,       MICBytes,       MICData,      MICData),      Write, NULL}
        },
    },




    /*
     * DPD_SRTP_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes       (none)
     * p1  ivData                ivBytes            (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            HeaderBytes
     * p4  cryptDataOut          cryptDataBytes     ROCBytes
     * p5  digestData            digestBytes        (none)
     * p6  outIvData             outIvBytes         (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_SRTP_GROUP,
        "DPD_SRTP_GROUP",
        sizeof(SRTP_REQ),
        SrtpReq,
        offsetof(SRTP_REQ, outIvData),
        {
            {"hashKeyData", STD_OFFSETS(SRTP_REQ, hashKeyBytes,   hashKeyBytes,   hashKeyData,  hashKeyData), Read,  NULL},
            {"ivData",      STD_OFFSETS(SRTP_REQ, ivBytes,        ivBytes,        ivData,       ivData),      Read,  NULL},
            {"keyData",     STD_OFFSETS(SRTP_REQ, keyBytes,       keyBytes,       keyData,      keyData),     Read,  ChkAesKeyLen},
            {"inData",      EXT_OFFSETS(SRTP_REQ, inBytes,        inBytes,        inData,       HeaderBytes), Read,  0},
            {"cryptDataOut",EXT_OFFSETS(SRTP_REQ, cryptDataBytes, cryptDataBytes, cryptDataOut, ROCBytes),    Write, NULL},
            {"digestData",  STD_OFFSETS(SRTP_REQ, digestBytes,    digestBytes,    digestData,   digestData),  Read,  0},
            {"outIvData",   STD_OFFSETS(SRTP_REQ, outIvBytes,     outIvBytes,     outIvData,    outIvData),   Write, NULL}
        },
    },





    /*
     * DPD_KEA_CRYPT_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  ivData                ivBytes            (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            (none)
     * p4  outData               outBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  ctxData               ctxBytes           (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_KEA_CRYPT_GROUP,
        "DPD_KEA_CRYPT_GROUP",
        sizeof(KEA_CRYPT_REQ),
        KeaReq,
        offsetof(KEA_CRYPT_REQ, ctxData),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                         Read,  NULL},
            {"ivData",      STD_OFFSETS(KEA_CRYPT_REQ, ivBytes,    ivBytes,    ivData,    ivData),    Read,  NULL},
            {"keyData",     STD_OFFSETS(KEA_CRYPT_REQ, keyBytes,   keyBytes,   keyData,   keyData),   Read,  NULL},
            {"inData",      STD_OFFSETS(KEA_CRYPT_REQ, inBytes,    inBytes,    inData,    inData),    Read,  NULL},
            {"outData",     STD_OFFSETS(KEA_CRYPT_REQ, outBytes,   outBytes,   outData,   outData),   Write, NULL},
            {"outIvData",   STD_OFFSETS(KEA_CRYPT_REQ, outIvBytes, outIvBytes, outIvData, outIvData), Write, NULL},
            {"ctxData",     STD_OFFSETS(KEA_CRYPT_REQ, ctxBytes,   ctxBytes,   ctxData,   ctxData),   Write, NULL}
        },
    },

    /*
     * DPD_SNOW3G_CRYPT_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  ivData                ivBytes            (none)
     * p2  keyData               keyBytes           (none)
     * p3  inData                inBytes            (none)
     * p4  outData               outBytes           (none)
     * p5  (none)                (none)             (none)
     * p6  ctxData               ctxBytes           (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_SNOW3G_CRYPT_GROUP,
        "DPD_SNOW3G_CRYPT_GROUP",
        sizeof(SNOW3G_CRYPT_REQ),
        SnoReq,
        offsetof(SNOW3G_CRYPT_REQ, ctxData),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                                            Read,  NULL},
            {"ivData",      STD_OFFSETS(SNOW3G_CRYPT_REQ, ivBytes,    ivBytes,    ivData,    ivData),    Read,  NULL},
            {"keyData",     STD_OFFSETS(SNOW3G_CRYPT_REQ, keyBytes,   keyBytes,   keyData,   keyData),   Read,  NULL},
            {"inData",      STD_OFFSETS(SNOW3G_CRYPT_REQ, inBytes,    inBytes,    inData,    inData),    Read,  NULL},
            {"outData",     STD_OFFSETS(SNOW3G_CRYPT_REQ, outBytes,   outBytes,   outData,   outData),   Write, NULL},
            {"outIvData",   STD_OFFSETS(SNOW3G_CRYPT_REQ, outIvBytes, outIvBytes, outIvData, outIvData), Write, NULL},
            {"ctxData",     STD_OFFSETS(SNOW3G_CRYPT_REQ, ctxBytes,   ctxBytes,   ctxData,   ctxData),   Write, NULL}
        },
    },


    /*
     * DPD_RAID_XOR_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  (none)                (none)             (none)
     * p1  (none)                (none)             (none)
     * p2  (none)                (none)             (none)
     * p3  inDataA               opSize             (none)
     * p4  inDataB               opSize             (none)
     * p5  inDataC               opSize             (none)
     * p6  outData               opSize             (none)
     * ----------------------------------------------------------------
     */
    {
        DPD_RAID_XOR_GROUP,
        "DPD_RAID_XOR_GROUP",
        sizeof(RAID_XOR_REQ),
        RaidXorReq,
        offsetof(RAID_XOR_REQ, outData),
        {
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL},
            {NIL,           ALL_ZERO_OFFSETS,                                            Read,  NULL},
            {"inDataA",     STD_OFFSETS(RAID_XOR_REQ, opSize, opSize, inDataA, inDataA), Read,  NULL},
            {"inDataB",     STD_OFFSETS(RAID_XOR_REQ, opSize, opSize, inDataB, inDataB), Read,  NULL},
            {"inDataC",     STD_OFFSETS(RAID_XOR_REQ, opSize, opSize, inDataC, inDataC), Read,  NULL},
            {"outData",     STD_OFFSETS(RAID_XOR_REQ, opSize, opSize, outData, outData), Write, NULL},
        },
    },


    /*
     * DPD_TLS_BLOCK_INBOUND_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes
     * p1  hashOnlyData                             hashOnlyBytes
     * p2  ivData                ivBytes
     * p3  cipherKeyData         cipherKeyBytes
     * p4  inData                inBytes            MACcmpBytes
     * p5  outData               outBytes           MACoutBytes
     * p6  ivOutData             ivOutBytes
     * ----------------------------------------------------------------
     */
    {
        DPD_TLS_BLOCK_INBOUND_GROUP,
        "DPD_TLS_BLOCK_INBOUND_GROUP",
        sizeof(TLS_BLOCK_INBOUND_REQ),
        TlsBlockInboundReq,
        offsetof(TLS_BLOCK_INBOUND_REQ, ivOutData),
        {
            {"hashKeyData",   STD_OFFSETS(TLS_BLOCK_INBOUND_REQ, hashKeyBytes,   hashKeyBytes,   hashKeyData,   hashKeyData),   Read,  NULL},
            {"hashOnlyData",  EXT_OFFSETS(TLS_BLOCK_INBOUND_REQ, hashOnlyBytes,  hashOnlyBytes,  hashOnlyData,  hashOnlyBytes), Read,  NULL},
            {"ivData",        STD_OFFSETS(TLS_BLOCK_INBOUND_REQ, ivBytes,        ivBytes,        ivData,        ivData),        Read,  NULL},
            {"cipherKeyData", STD_OFFSETS(TLS_BLOCK_INBOUND_REQ, cipherKeyBytes, cipherKeyBytes, cipherKeyData, cipherKeyData), Read,  NULL},
            {"inData",        EXT_OFFSETS(TLS_BLOCK_INBOUND_REQ, inBytes,        inBytes,        inData,        MACcmpBytes),   Read,  NULL},
            {"outData",       EXT_OFFSETS(TLS_BLOCK_INBOUND_REQ, outBytes,       outBytes,       outData,       MACoutBytes),   Write, NULL},
            {"ivOutData",     STD_OFFSETS(TLS_BLOCK_INBOUND_REQ, ivOutBytes,     ivOutBytes,     ivOutData,     ivOutData),     Write, NULL}
        },
    },



    /*
     * DPD_TLS_BLOCK_OUTBOUND_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes
     * p1  ivData                ivBytes
     * p2  cipherKeyData         cipherKeyBytes
     * p3  hashOnlyData          mainDataBytes      hashOnlyBytes
     * p4  cipherOnlyData        cipherOnlyBytes    MACbytes
     * p5  outData               outBytes
     * p6  ivOutData             ivOutBytes
     * ----------------------------------------------------------------
     */
    {
        DPD_TLS_BLOCK_OUTBOUND_GROUP,
        "DPD_TLS_BLOCK_OUTBOUND_GROUP",
        sizeof(TLS_BLOCK_OUTBOUND_REQ),
        TlsBlockOutboundReq,
        offsetof(TLS_BLOCK_OUTBOUND_REQ, ivOutData),
        {
            {"hashKeyData",    STD_OFFSETS(TLS_BLOCK_OUTBOUND_REQ, hashKeyBytes,    hashKeyBytes,    hashKeyData,    hashKeyData),    Read,  NULL},
            {"ivData",         STD_OFFSETS(TLS_BLOCK_OUTBOUND_REQ, ivBytes,         ivBytes,         ivData,         ivData),         Read,  NULL},
            {"cipherKeyData",  STD_OFFSETS(TLS_BLOCK_OUTBOUND_REQ, cipherKeyBytes,  cipherKeyBytes,  cipherKeyData,  cipherKeyData),  Read,  NULL},
            {"hashOnlyData",   EXT_OFFSETS(TLS_BLOCK_OUTBOUND_REQ, mainDataBytes,   mainDataBytes,   hashOnlyData,   hashOnlyBytes),  Read,  NULL},
            {"cipherOnlyData", EXT_OFFSETS(TLS_BLOCK_OUTBOUND_REQ, cipherOnlyBytes, cipherOnlyBytes, cipherOnlyData, MACbytes),       Write, NULL},
            {"outData",        STD_OFFSETS(TLS_BLOCK_OUTBOUND_REQ, outBytes,        outBytes,        outData,        outData),        Read,  NULL},
            {"ivOutData",      STD_OFFSETS(TLS_BLOCK_OUTBOUND_REQ, ivOutBytes,      ivOutBytes,      ivOutData,      ivOutData),      Write, NULL}
        },
    },





    /*
     * DPD_TLS_STREAM_INBOUND_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes
     * p1  hashOnlyData                             hashOnlyBytes
     * p2  ivData                ivBytes
     * p3  cipherKeyData         cipherKeyBytes
     * p4  inData                inBytes            MACcmpBytes
     * p5  outData               outBytes           MACoutBytes
     * p6  ivOutData             ivOutBytes
     * ----------------------------------------------------------------
     */
    {
        DPD_TLS_STREAM_INBOUND_GROUP,
        "DPD_TLS_STREAM_INBOUND_GROUP",
        sizeof(TLS_STREAM_INBOUND_REQ),
        TlsStreamInboundReq,
        offsetof(TLS_STREAM_INBOUND_REQ, ivOutBytes),
        {
            {"hashKeyData",   STD_OFFSETS(TLS_STREAM_INBOUND_REQ, hashKeyBytes,   hashKeyBytes,   hashKeyData,   hashKeyData),   Read,  NULL},
            {"hashOnlyData",  EXT_OFFSETS(TLS_STREAM_INBOUND_REQ, hashOnlyBytes,  hashOnlyBytes,  hashOnlyData,  hashOnlyBytes), Read,  NULL},
            {"ivData",        STD_OFFSETS(TLS_STREAM_INBOUND_REQ, ivBytes,        ivBytes,        ivData,        ivData),        Read,  NULL},
            {"cipherKeyData", STD_OFFSETS(TLS_STREAM_INBOUND_REQ, cipherKeyBytes, cipherKeyBytes, cipherKeyData, cipherKeyData), Read,  NULL},
            {"inData",        EXT_OFFSETS(TLS_STREAM_INBOUND_REQ, inBytes,        inBytes,        inData,        MACcmpBytes),   Read,  NULL},
            {"outData",       EXT_OFFSETS(TLS_STREAM_INBOUND_REQ, outBytes,       outBytes,       outData,       MACoutBytes),   Write, NULL},
            {"ivOutData",     STD_OFFSETS(TLS_STREAM_INBOUND_REQ, ivOutBytes,     ivOutBytes,     ivOutData,     ivOutData),     Write, NULL}
        },
    },






    /*
     * DPD_TLS_STREAM_OUTBOUND_GROUP
     *     pointer               length             extent
     * ----------------------------------------------------------------
     * p0  hashKeyData           hashKeyBytes
     * p1  ivData                ivBytes
     * p2  cipherKeyData         cipherKeyBytes
     * p3  hashOnlyData          mainDataBytes      hashOnlyBytes
     * p4  outData               outBytes           MACbytes
     * p5
     * p6  ivOutData             ivOutBytes
     * ----------------------------------------------------------------
     */
    {
        DPD_TLS_STREAM_OUTBOUND_GROUP, "DPD_TLS_STREAM_OUTBOUND_GROUP",
        sizeof(TLS_STREAM_OUTBOUND_REQ), TlsStreamOutboundReq,
        offsetof(TLS_STREAM_OUTBOUND_REQ, ivOutBytes),
        {
            {"hashKeyData",    STD_OFFSETS(TLS_STREAM_OUTBOUND_REQ, hashKeyBytes,   hashKeyBytes,   hashKeyData,   hashKeyData),   Read,  NULL},
            {"ivData",         STD_OFFSETS(TLS_STREAM_OUTBOUND_REQ, ivBytes,        ivBytes,        ivData,        ivData),        Read,  NULL},
            {"cipherKeyData",  STD_OFFSETS(TLS_STREAM_OUTBOUND_REQ, cipherKeyBytes, cipherKeyBytes, cipherKeyData, cipherKeyData), Read,  NULL},
            {"hashOnlyData",   EXT_OFFSETS(TLS_STREAM_OUTBOUND_REQ, mainDataBytes,  mainDataBytes,  hashOnlyData,  hashOnlyBytes), Read,  NULL},
            {"outData",        EXT_OFFSETS(TLS_STREAM_OUTBOUND_REQ, outBytes,       outBytes,       outData,       MACbytes),      Write, NULL},
            {NIL,              ALL_ZERO_OFFSETS,                                                                                  Read,  NULL},
            {"ivOutData",      STD_OFFSETS(TLS_STREAM_OUTBOUND_REQ, ivOutBytes,     ivOutBytes,     ivOutData,     ivOutData),     Write, NULL}
        },
    },

    /* Always terminates list, keep this line at the end when adding functions */
    {
        0, NULL,
        0, NULL,
        0,
        {
            {NULL, ALL_ZERO_OFFSETS, Read, NULL},
        },
    },

};


int VerifyRequest(register void *rq, register DPD_DETAILS_ENTRY *dpdmap)
{
    register int           i;
    register unsigned long len;

    for (i = 0; i < NUM_DPD_FLDS && dpdmap->fld[i].txt != NULL; i++)
    {

        if (dpdmap->fld[i].pFncSize != NULL)
        {
            len = *(unsigned int *)((unsigned int)rq + dpdmap->fld[i].lenOffset1st);

            if (!dpdmap->fld[i].pFncSize(len))
                return SEC2_INVALID_LENGTH;
        }

        if (dpdmap->fld[i].extOffset != 0)
        {
            len = *(unsigned int *)((unsigned int)rq + dpdmap->fld[i].extOffset);
            if (len >= 128)
                return SEC2_INVALID_LENGTH;
        }
    }

    return SEC2_SUCCESS;
}





/**
 * Scan the DPDDetails table for a particular opId value.
 * Return a pointer to the entry if it is found, NULL otherwise
 */
DPD_DETAILS_ENTRY *GetRequestDescEntry(unsigned long opId)
{
    int i = 0;

    opId &= DESC_TYPE_MASK;
    while (DpdDetails[i].txt != NULL && DpdDetails[i].opId != opId) i++;

    return(DpdDetails[i].opId == 0 ? NULL : &DpdDetails[i]);
}



/**
 * This needs to get the core info from the RM */
int CheckCapability(unsigned long opID)
{
    /*
       switch (opID & 0xf000)
       {
       case RNG_GROUP:
       if (DevCapability & CHA_RNGA_BITMASK)
       return(SEC2_SUCCESS);

       case DES_GROUP:
       if (DevCapability & CHA_DESA_BITMASK)
       return(SEC2_SUCCESS);

       case ARC4_GROUP:
       if (DevCapability & CHA_AFHA_BITMASK)
       return(SEC2_SUCCESS);

       case MD_GROUP:
       if (DevCapability & CHA_MDHA_BITMASK)
       return(SEC2_SUCCESS);

       case PK_GROUP:
       if (DevCapability & CHA_PKHA_BITMASK)
       return(SEC2_SUCCESS);

       case AES_GROUP:
       if (DevCapability & CHA_AESA_BITMASK)
       return(SEC2_SUCCESS);

       case IPSEC_DES_GROUP:
       case TLS_GROUP:
       if ((DevCapability & CHA_MDHA_BITMASK) &&
       (DevCapability & CHA_DESA_BITMASK))
       return(SEC2_SUCCESS);

       case IPSEC_AES_GROUP:
       if ((DevCapability & CHA_MDHA_BITMASK) &&
       (DevCapability & CHA_AESA_BITMASK))
       return(SEC2_SUCCESS);

       case KEA_GROUP:
       if (DevCapability & CHA_KEA_BITMASK)
       return(SEC2_SUCCESS);

       default:
       return (SEC2_INVALID_CHA_TYPE);
       }

       return (SEC2_INVALID_CHA_TYPE);
     */
    return SEC2_SUCCESS;
}


BOOLEAN ChkDesIvLen(unsigned long len)
{
    return len != 0 && len != DES_STD_BLOCKSIZE ? FALSE : TRUE;
}



BOOLEAN ChkDesKeyLen(unsigned long len)
{
    return len != DES_STD_BLOCKSIZE && len != (2 * DES_STD_BLOCKSIZE) && len != (3 * DES_STD_BLOCKSIZE) ? FALSE : TRUE;
}



BOOLEAN ChkDesStaticDataLen(unsigned long len)
{
    return (len & 0x7) != 0 ? FALSE : TRUE;
}



BOOLEAN ChkDesDataLen(unsigned long len)
{
    return (len & 0x7) != 0 ? FALSE : TRUE;
}



BOOLEAN ChkDesCtxLen(unsigned long len)
{
    return len != 0 && len != DES_STD_BLOCKSIZE ? FALSE : TRUE;
}



BOOLEAN ChkAesKeyLen(unsigned long len)
{
    return len != 16 && len != 24 && len != 32 ? FALSE : TRUE;
}



BOOLEAN ChkAesIvLen(unsigned long len)
{
    return len != 0 && len != 16 ? FALSE : TRUE;
}



BOOLEAN ChkArcKeyLen(unsigned long len)
{
    return len < ARC4_STD_MIN_KEYBYTES || len > ARC4_STD_MAX_KEYBYTES ? FALSE : TRUE;
}



BOOLEAN ChkArcCtxLen(unsigned long len)
{
    return len != ARC4_STD_CONTEXTBYTES ? FALSE : TRUE;
}



BOOLEAN ChkOptionalArcCtxLen(unsigned long len)
{
    return len != ARC4_STD_CONTEXTBYTES && len != 0 ? FALSE : TRUE;
}



BOOLEAN ChkEccLen(unsigned long len)
{
    return len > 64 ? FALSE : TRUE;
}

#if 0

BOOLEAN FitInBlock(unsigned long len)
{
    return len > BlockSize ? FALSE : TRUE;
}
#endif



BOOLEAN ChkCcmpKeyLen(unsigned long len)
{
    return len != 0 && len != 16 ? FALSE : TRUE;
}





int constructDPDlist(GENERIC_REQ   *rqList,
                     RMexecMessage *execMsg,
                     PTRTYPE        memtype)
{
    DPD_DETAILS_ENTRY *dpdmap;
    T2DPD             *thisDPD;
    T2DESC_AUXMAP     *thisDPDmap;
    int                i, status, dpdcount;
    GENERIC_REQ       *thisRQ;

    if (rqList == NULL)
        return SEC2_NULL_REQUEST;

    dpdcount   = 0;
    thisRQ     = rqList;
    thisDPD    = execMsg->descHead;
    thisDPDmap = execMsg->descBufferMap;

    do
    {
        dpdmap = GetRequestDescEntry(thisRQ->opId);
        if (dpdmap == NULL)
            return SEC2_INVALID_OPERATION_ID;

        status = CheckCapability(thisRQ->opId);
        if (status != SEC2_SUCCESS)
            return status;

        status = VerifyRequest(thisRQ, dpdmap);
        if (status != SEC2_SUCCESS)
            return status;


        /* Get the descriptor header */
        thisDPD->hdr = dpdmap->hdrDesc[1 * (thisRQ->opId & DESC_NUM_MASK)];

        /* Now process each pair */
        for (i = 0;
             (i < TOTAL_PAIRS) && (dpdmap->fld[i].txt != NULL);
             i++)
        {
            if (*dpdmap->fld[i].txt == *"NIL")
            {
                thisDPD->pair[i].ptr    = NULL;
                thisDPD->pair[i].eptr   = 0;
                thisDPD->pair[i].size   = 0;
                thisDPD->pair[i].extent = 0;
            }
            else /* is non NIL pair, and has value */
            {
                if ((dpdmap->fld[i].ptrOffset1st != 0) &&
                        *(unsigned int *)
                        ((unsigned int)thisRQ + dpdmap->fld[i].ptrOffset1st))
                {
                    /* Extract the pointer using the dpdmap offset */
                    thisDPD->pair[i].ptr =
                        (void *)(*(unsigned int *)
                                 ((unsigned int)thisRQ + dpdmap->fld[i].ptrOffset1st));

                    /* 36-bit addressing not in use for legacy, zero it */
                    thisDPD->pair[i].eptr = 0;

                    /* Extract the length using the dpdmap offset */
                    thisDPD->pair[i].size =
                        *(unsigned int *)
                        ((unsigned int)thisRQ + dpdmap->fld[i].lenOffset1st);

                    /* Extract the (optional) extent using the dpdmap offset */
                    if (dpdmap->fld[i].extOffset)
                        thisDPD->pair[i].extent =
                            (*(unsigned int *)
                             ((unsigned int)thisRQ + dpdmap->fld[i].extOffset) &
                             EXTENT_MASK);
                    else
                        thisDPD->pair[i].extent = 0;

                    switch(memtype)
                    {
                        /* If we know it's a phyiscal pointer, don't have to */
                        /* do anything                                       */
                        case PTR_PHYSICAL:
                            break;


                            /* If logical, then we assume that the buffer is     */
                            /* contiguous, and we only have to substitute a      */
                            /* physical address for the pointer specified        */
                        case PTR_LOGICAL:
                            status = xwcMemTranslateLogical(i, thisDPD, thisDPDmap);
                            if (status)
                            {
                                printk("t23xsec2: can't translate logical address\n");
                                return SEC2_ADDRESS_PROBLEM;
                            }
                            break;


                            /* If user virtual, we have look at the page map for */
                            /* this buffer, create a scatterlist, and lock in    */
                            /* pages in question                                 */
                        case PTR_USER_VIRTUAL:
                            status = xwcMemTranslateUserVirtual(i, thisDPD, thisDPDmap);
                            if (status)
                            {
                                printk("t23xsec2: can't translate user virtual address\n");
                                return SEC2_ADDRESS_PROBLEM;
                            }
                            break;


                            /* If kernel virtual, we have to build a scatterlist */
                            /* Not supported at this time - if ever...           */
                        case PTR_KERNEL_VIRTUAL:
                            status = xwcMemTranslateKernelVirtual(i, thisDPD, thisDPDmap);
                            if (status)
                            {
                                printk("t23xsec2: can't translate kernel virtual address\n");
                                return SEC2_ADDRESS_PROBLEM;
                            }
                            break;


                            /* NO default, if no specifed type, this is a bug... */
                        default:
                            return SEC2_ADDRESS_PROBLEM;

                    } /* switch (memtype) */
                } /* if (a real pointer) */
                else /* valid field, but no data needed, so clear it */
                {
                    thisDPD->pair[i].ptr    = NULL;
                    thisDPD->pair[i].eptr   = 0;
                    thisDPD->pair[i].size   = 0;
                    thisDPD->pair[i].extent = 0;
                }
            } /* non-NULL pair */
        } /* for (each pair) */

        /* If this is the last RQ in the list, then mark this DPD */
        /* to generate an interrupt. Not sure it's appropriate to */
        /* stay this way...                                      */

        if (thisRQ->nextReq == NULL)
            thisDPD->hdr |= HDR_DONE;

        /* Done with this DPD, go to next */
        thisRQ = thisRQ->nextReq;
        thisDPD++;
        thisDPDmap++;

        /* This should be dynamic...*/
        if (++dpdcount > T2_CHANNEL_FIFO_DEPTH)
            return SEC2_INSUFFICIENT_REQS;

    } while (thisRQ != NULL);

    execMsg->descCount = dpdcount;

    return SEC2_SUCCESS;
}


/**
 * After done with a DPD list, free any resources it may have used */
int releaseDPDlist(RMexecMessage *execMsg,
                   PTRTYPE        memtype)
{
    int            i, status, totalDPDs;
    T2DPD         *thisDPD;
    T2DESC_AUXMAP *thisDPDmap;


    thisDPD    = execMsg->descHead;
    thisDPDmap = execMsg->descBufferMap;
    totalDPDs  = execMsg->descCount;

    do
    {
        for (i = 0; (i < TOTAL_PAIRS); i++)
        {
            if (thisDPD->pair[i].ptr != NULL)
                switch (memtype)
                {
                    case PTR_PHYSICAL:
                    case PTR_LOGICAL:
                        break;

                    case PTR_USER_VIRTUAL:
                        status = xwcMemReleaseUserVirtual(i, thisDPD, thisDPDmap);
                        if (status)
                        {
                            printk("t2xsec2: can't release user virtual buffer\n");
                            return SEC2_UNKNOWN_ERROR;
                        }
                        break;

                    case PTR_KERNEL_VIRTUAL:
                        status = xwcMemReleaseKernelVirtual(i, thisDPD, thisDPDmap);
                        if (status)
                        {
                            printk("t2xsec2: can't release kernel virtual address\n");
                            return SEC2_UNKNOWN_ERROR;
                        }
                        break;

                    default:
                        return SEC2_ADDRESS_PROBLEM;

                } /* switch (memtype) */
            /* if (valid pair) */
        } /* for (each pair) */

        thisDPD++;
        thisDPDmap++;

    } while (--totalDPDs);

    return SEC2_SUCCESS;
}

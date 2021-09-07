
/*
 * t23xrmID.c
 *
 * Contains core version detection code for the t23x resource manager
 * module
 *
 * As of the present rev, core characteristice are provided by the
 * device tree, this module effectively determines the core
 * major/minor/rev from the ID registers. It can be used to detect
 * core capability for operating systems that don't support an fdt
 * construct in the future.
 *
 * Copyright (c) 2007-2010 Freescale Semiconductor, Inc.
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
 * 2.1.0   2009_05_04 sec - add SNOW-3G CHA support
 */


/** @file
 * Handles device version detection for Talitos 2/3 devices, and
 * sets up "capability information" for an initialized device
 */

#include <linux/types.h>

#include "t23xrmInternal.h"

extern T2CoreInstance t2blk[];




static const uint8_t devname_unknown[] = { "unknown" };
static const uint8_t devname_2_0_0[]   = { "2.0.0" }; /* 8541/8555 */
static const uint8_t devname_2_0_1[]   = { "2.0.1" }; /* 8349/8347/8343/8360/8538 */
static const uint8_t devname_2_1_0[]   = { "2.1.0" }; /* 8548/8547/8545/8543 */
static const uint8_t devname_2_1_1[]   = { "2.1.1" };
static const uint8_t devname_2_1_2[]   = { "2.1.2" }; /* 8548/8547/8545/8543 */
static const uint8_t devname_2_1_3[]   = { "2.1.3" };
static const uint8_t devname_2_1_4[]   = { "2.1.4" }; /* 8548/8547/8545/8543 */
static const uint8_t devname_2_1_5[]   = { "2.1.5" }; /* 8548/8547/8545/8543 */
static const uint8_t devname_2_2_0[]   = { "2.2.0" }; /* 8323/8321/8311/8313 */
static const uint8_t devname_2_2_1[]   = { "2.2.1" }; /* 8323/8321/8311/8313 */
static const uint8_t devname_2_2_2[]   = { "2.2.2" }; /* 8323/8321/8311/8313 */
static const uint8_t devname_2_4_0[]   = { "2.4.0" }; /* 8349/8347/8343/8360/8538 */
static const uint8_t devname_2_4_1[]   = { "2.4.1" }; /* 8349/8347/8343/8360/8538 */
static const uint8_t devname_2_4_2[]   = { "2.4.2" }; /* 8349/8347/8343/8360/8538 */

static const uint8_t devname_3_0_0[]   = { "3.0.0" }; /* 8572/8571/8379/8378/8377 */
static const uint8_t devname_3_0_1[]   = { "3.0.1" }; /* 8536 */
static const uint8_t devname_3_1_0[]   = { "3.1.0" }; /* 8569/8526 */
static const uint8_t devname_3_3_0[]   = { "3.3.0" }; /* 8315 */
static const uint8_t devname_3_3_1[]   = { "3.3.1" }; /* 8315 */
static const uint8_t devname_3_3_2[]   = { "3.3.2" }; /* p1021/p1022 */

/*
 * Device capability mode bits for both Talitos 2 and 3
 * This is the same convention used for the device tree, and is used to build
 * a "capability mask" for the hardware detected in the system.
 *
 * On late linux-powerpc versions, this comes directly from the OS. For all
 * others, NO_FDT should be specified to t23xrmCoreID(), and the proper
 * values detected from ID registers and passed in.
 *
 *
 *       EU selection bits
 *
 *       0 (reserved - none selected)
 *       1 AFEU
 *       2 DEU
 *       3 MDEU-A  (T2 and 3)
 *       4 RNG
 *       5 PKEU
 *       6 AESU
 *       7 KEU
 *       8 CRCU
 *       9 STEU
 *      10 (reserved)
 *      11 MDEU-B  (T3 only)
 *      12 (reserved)
 *      13 (reserved)
 *      14 (reserved)
 *      15 (reserved, header writeback)
 *
 *      Descriptor processing mode selection bits
 *
 *       0 aesu_ctr_nonsnoop      AESU CTR mode. nonsnoop
 *       1 ipsec_esp              IPSEC ESP conventional (non-GCM)
 *       2 common_nonsnoop        Common non-PK or ARC4
 *       3 802.11i_AES_ccmp       CCMP cipher/hash compliant with 802.11i
 *       4 hmac_snoop_no_afeu     HMAC snoop, non-AFEU
 *       5 srtp descriptor        Cipher/hash compliant with SRTP
 *       6 non_hmac_snoop_no_afeu
 *       7 pkeu_assemble          PK request build for ECC
 *       8 aesu_key_expand_output
 *       9 pkeu_ptmul             PK point multiply
 *      10 common_nonsnoop_afeu   Common ARC4
 *      11 pkeu_ptadd_dbl         PK point add double
 *      12 (reserved)
 *      13 (reserved)
 *      14 (reserved)
 *      15 (reserved)
 *      16 pkeu_mm                PKEU Montgomery Multiplication
 *      17 tls_ssl_block          TLS/SSL generic blockcipher
 *      18 (reserved)
 *      19 tls_ssl_stream         TLS/SSL generic streamcipher
 *      20 (reserved)
 *      21 raid_xor               XOR 2-6 sources
 *      22 (reserved)
 *      23 ipsec_aes_gcm          IPSEC ESP with GCM cipher/hash
 *      24 hmac_snoop_aesu_ctr    AESU CTR hmac snooping
 *      25 dbl_crc                Double CRC operations
 *      26 (reserved)
 *      27 (reserved)
 *      28 (reserved)
 *      29 (reserved)
 *      30 (reserved)
 *      31 (reserved)
 */

#define HAS_NULL   0x00000001
#define HAS_AFEU   0x00000002
#define HAS_DEU    0x00000004
#define HAS_MDEU   0x00000008
#define HAS_RNG    0x00000010
#define HAS_PKEU   0x00000020
#define HAS_AESU   0x00000040
#define HAS_KEU    0x00000080
#define HAS_CRCU   0x00000100
#define HAS_STEU   0x00000200
#define HAS_MDEU_B 0x00000800

#define HAS_AESU_CTR_NONSNOOP      0x00000001
#define HAS_IPSEC_ESP              0x00000002
#define HAS_COMMON_NONSNOOP        0x00000004
#define HAS_COMMON_80211_AES_CCMP  0x00000008
#define HAS_HMAC_SNOOP_NO_AFEU     0x00000010
#define HAS_SRTP                   0x00000020
#define HAS_NON_HMAC_SNOOP_NO_AFEU 0x00000040
#define HAS_PKEU_ASSEMBLE          0x00000080
#define HAS_AESU_KEY_EXPAND_OUT    0x00000100
#define HAS_PKEU_PTMUL             0x00000200
#define HAS_COMMON_NONSNOOP_AFEU   0x00000400
#define HAS_PKEU_PTADD_DBL         0x00000800
#define HAS_PKEU_MM                0x00010000
#define HAS_TLS_SSL_BLOCK          0x00020000
#define HAS_TLS_SSL_STREAM         0x00080000
#define HAS_RAID_XOR               0x00200000
#define HAS_IPSEC_AES_GCM          0x00800000
#define HAS_HMAC_SNOOP_AESU_CTR    0x01000000
#define HAS_DBL_CRC                0x02000000


/**
 * Core ID and capabilty map builder.
 * This needs to be made table-driven
 *
 * Passes instance and NO_FDT if the device tree is not present,
 * or HAS_FDT if it exists
 */

void t23xrmCoreID(T2CoreInstance *inst, uint8_t fdt)
{
    T2CoreInstance  *thisInst;

    thisInst = inst;

    /* If a device has no known ID, don't let it do much */
    thisInst->devMajor      = 0;
    thisInst->devMinor      = 0;
    thisInst->devRev        = 0;
    thisInst->devName       = devname_unknown;
    if (fdt == NO_FDT)
    {
        thisInst->totalChannels = 0;
        thisInst->fifoDepth     = 24;
        thisInst->euPresent     = HAS_NULL;
        thisInst->validTypes    = 0;
    }


    /* 2.0.0 - 8541/8555 */
    if (thisInst->regs->ctrl.ID == 0x0000000000000040ull)
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 0;
        thisInst->devRev   = 0;
        thisInst->devName  = devname_2_0_0;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.0.1 - 8349/8347/8343/8360/8538 */
    if (thisInst->regs->ctrl.ID == 0x0000000000000041ull)
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 0;
        thisInst->devRev   = 1;
        thisInst->devName  = devname_2_0_1;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.1.0 - 8548/8547/8545/8543 */
    if ((thisInst->regs->ctrl.ID == 0x0000000000000040ull) &&
        (thisInst->regs->ipID.id == 0x0030000000000000ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 1;
        thisInst->devRev   = 0;
        thisInst->devName  = devname_2_1_0;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_KEU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK |
                                      HAS_TLS_SSL_STREAM | HAS_RAID_XOR |
                                      HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.1.2 - 8548/8547/8545/8543 */
    if ((thisInst->regs->ctrl.ID == 0x0030010200000000ull) &&
        (thisInst->regs->ipID.id == 0x0030010200000000ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 1;
        thisInst->devRev   = 2;
        thisInst->devName  = devname_2_1_2;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_KEU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK |
                                      HAS_TLS_SSL_STREAM | HAS_RAID_XOR |
                                      HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.1.4 - 8548/8547/8545/8543 */
    if ((thisInst->regs->ctrl.ID == 0x0030010400000000ull) &&
        (thisInst->regs->ipID.id == 0x0030010400000000ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 1;
        thisInst->devRev   = 4;
        thisInst->devName  = devname_2_1_4;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_KEU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK |
                                      HAS_TLS_SSL_STREAM | HAS_RAID_XOR |
                                      HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.1.5 - 8548/8547/8545/8543 */
    if ((thisInst->regs->ctrl.ID == 0x0030010500000000ull) &&
        (thisInst->regs->ipID.id == 0x0030010500000000ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 1;
        thisInst->devRev   = 5;
        thisInst->devName  = devname_2_1_5;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_KEU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK |
                                      HAS_TLS_SSL_STREAM | HAS_RAID_XOR |
                                      HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.2.0 - 8323/8321/8311/8313 */
    if ((thisInst->regs->ctrl.ID == 0x00000000000000a0ull) &&
        (thisInst->regs->ipID.id == 0x00000000000000a0ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 2;
        thisInst->devRev   = 0;
        thisInst->devName  = devname_2_2_0;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 1;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_DEU | HAS_MDEU | HAS_AESU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_AESU_KEY_EXPAND_OUT |
                                      HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_IPSEC_AES_GCM | HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.2.1 - 8323/8321/8311/8313 */
    if ((thisInst->regs->ctrl.ID == 0x00000000000100a0ull) &&
        (thisInst->regs->ipID.id == 0x00000000000100a0ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 2;
        thisInst->devRev   = 1;
        thisInst->devName  = devname_2_2_1;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 1;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_DEU | HAS_MDEU | HAS_AESU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_AESU_KEY_EXPAND_OUT |
                                      HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_IPSEC_AES_GCM | HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.2.2 - 8323/8321/8311/8313 */
    if ((thisInst->regs->ctrl.ID == 0x00000000000200a0ull) &&
        (thisInst->regs->ipID.id == 0x00000000000200a0ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 2;
        thisInst->devRev   = 2;
        thisInst->devName  = devname_2_2_2;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 1;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_DEU | HAS_MDEU | HAS_AESU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_AESU_KEY_EXPAND_OUT |
                                      HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_IPSEC_AES_GCM | HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.4.0 - 8349/8347/8343/8360/8538 */
    if ((thisInst->regs->ctrl.ID == 0x0030000000000003ull) &&
        (thisInst->regs->ipID.id == 0x0030000000000003ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 4;
        thisInst->devRev   = 0;
        thisInst->devName  = devname_2_4_0;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.4.1 - 8349/8347/8343/8360/8538 */
    if ((thisInst->regs->ctrl.ID == 0x0030000100000003ull) &&
        (thisInst->regs->ipID.id == 0x0030000100000003ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 4;
        thisInst->devRev   = 1;
        thisInst->devName  = devname_2_4_1;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 2.4.2 - 8349/8347/8343/8360/8538 */
    if ((thisInst->regs->ctrl.ID == 0x0030000200000003ull) &&
        (thisInst->regs->ipID.id == 0x0030000200000003ull))
    {
        thisInst->devMajor = 2;
        thisInst->devMinor = 4;
        thisInst->devRev   = 2;
        thisInst->devName  = devname_2_4_2;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_HMAC_SNOOP_AESU_CTR;
        }
    }

    /* 3.0.0 - 8572/8571/8379/8378/8377 */
    if ((thisInst->regs->ctrl.ID == 0x0030030000000000ull) &&
        (thisInst->regs->ipID.id == 0x0030030000000000ull))
    {
        thisInst->devMajor = 3;
        thisInst->devMinor = 0;
        thisInst->devRev   = 0;
        thisInst->devName  = devname_3_0_0;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_KEU | HAS_CRCU | HAS_MDEU_B;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_RAID_XOR | HAS_IPSEC_AES_GCM |
                                      HAS_HMAC_SNOOP_AESU_CTR | HAS_DBL_CRC;
        }
    }

    /* 3.0.1 - 8536 */
    if ((thisInst->regs->ctrl.ID == 0x0030030000010000ull) &&
        (thisInst->regs->ipID.id == 0x0030030000010000ull))
    {
        thisInst->devMajor = 3;
        thisInst->devMinor = 0;
        thisInst->devRev   = 1;
        thisInst->devName  = devname_3_0_1;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_KEU | HAS_CRCU | HAS_MDEU_B;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_RAID_XOR | HAS_IPSEC_AES_GCM |
                                      HAS_HMAC_SNOOP_AESU_CTR | HAS_DBL_CRC;
        }
    }

    /* 3.1.0 - 8569/8526 */
    if ((thisInst->regs->ctrl.ID == 0x0030030100000000ull) &&
        (thisInst->regs->ipID.id == 0x0030030100000000ull))
    {
        thisInst->devMajor = 3;
        thisInst->devMinor = 1;
        thisInst->devRev   = 0;
        thisInst->devName  = devname_3_1_0;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_AFEU | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_KEU | HAS_CRCU | HAS_STEU | HAS_MDEU_B;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_COMMON_NONSNOOP_AFEU | HAS_PKEU_PTADD_DBL |
                                      HAS_PKEU_MM | HAS_TLS_SSL_BLOCK | HAS_TLS_SSL_STREAM |
                                      HAS_RAID_XOR | HAS_IPSEC_AES_GCM |
                                      HAS_HMAC_SNOOP_AESU_CTR | HAS_DBL_CRC;
        }
    }

    /* 3.3.0 - 8315 */
    if ((thisInst->regs->ctrl.ID == 0x0030030300000000ull) &&
        (thisInst->regs->ipID.id == 0x0030030300000000ull))
    {
        thisInst->devMajor = 3;
        thisInst->devMinor = 3;
        thisInst->devRev   = 0;
        thisInst->devName  = devname_3_3_0;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_CRCU | HAS_MDEU_B;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_PKEU_PTADD_DBL | HAS_PKEU_MM | HAS_TLS_SSL_BLOCK |
                                      HAS_TLS_SSL_STREAM | HAS_RAID_XOR | HAS_IPSEC_AES_GCM |
                                      HAS_HMAC_SNOOP_AESU_CTR | HAS_DBL_CRC;
        }
    }

    /* 3.3.1 - 8315 */
    if ((thisInst->regs->ctrl.ID == 0x0030030300010000ull) &&
        (thisInst->regs->ipID.id == 0x0030030300010000ull))
    {
        thisInst->devMajor = 3;
        thisInst->devMinor = 3;
        thisInst->devRev   = 1;
        thisInst->devName  = devname_3_3_1;
        if (fdt == NO_FDT)
        {
            thisInst->totalChannels = 4;
            thisInst->fifoDepth     = 24;
            thisInst->euPresent     = HAS_NULL | HAS_DEU | HAS_MDEU | HAS_RNG |
                                      HAS_PKEU | HAS_AESU | HAS_CRCU | HAS_MDEU_B;
            thisInst->validTypes    = HAS_AESU_CTR_NONSNOOP | HAS_IPSEC_ESP |
                                      HAS_COMMON_NONSNOOP | HAS_COMMON_80211_AES_CCMP |
                                      HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
                                      HAS_NON_HMAC_SNOOP_NO_AFEU | HAS_PKEU_ASSEMBLE |
                                      HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
                                      HAS_PKEU_PTADD_DBL | HAS_PKEU_MM | HAS_TLS_SSL_BLOCK |
                                      HAS_TLS_SSL_STREAM | HAS_RAID_XOR | HAS_IPSEC_AES_GCM |
                                      HAS_HMAC_SNOOP_AESU_CTR | HAS_DBL_CRC;
        }
    }


	/* 3.3.2 - p1021, p1022 */
	if ((thisInst->regs->ctrl.ID == 0x0030030300020000ull) &&
		(thisInst->regs->ipID.id == 0x0030030300020000ull)) {
		thisInst->devMajor = 3;
		thisInst->devMinor = 3;
		thisInst->devRev   = 2;
		thisInst->devName  = devname_3_3_2;
		if (fdt == NO_FDT) {
			thisInst->totalChannels = 4;
			thisInst->fifoDepth = 24;
			thisInst->euPresent = HAS_NULL | HAS_DEU | HAS_MDEU |
				HAS_RNG | HAS_PKEU | HAS_AESU | HAS_CRCU |
				HAS_MDEU_B;
			thisInst->validTypes = HAS_AESU_CTR_NONSNOOP |
				HAS_IPSEC_ESP | HAS_COMMON_NONSNOOP |
				HAS_COMMON_80211_AES_CCMP |
				HAS_HMAC_SNOOP_NO_AFEU | HAS_SRTP |
				HAS_NON_HMAC_SNOOP_NO_AFEU |
				HAS_AESU_KEY_EXPAND_OUT | HAS_PKEU_PTMUL |
				HAS_PKEU_PTADD_DBL | HAS_PKEU_MM |
				HAS_TLS_SSL_BLOCK | HAS_RAID_XOR |
				HAS_IPSEC_AES_GCM | HAS_DBL_CRC;
		}
	}
	/* Last, init the number of free channels from the total */
	thisInst->freeChannels = thisInst->totalChannels;
}


/*
 * t23.h
 *
 * Basic hardware definitions that apply to both the
 * Talitos (SEC) 2.x and 3.x family of embeddable cryptographic
 * acceleration cores native to PowerQUICC processors
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
 * 2.1.0   2009_05_04 sec - add SNOW-3G CHA support
 */


#ifndef T23_H
#define T23_H


/** @file
 * Basic hardware definitions for the Talitos (SEC) 2.x and 3.x
 * family of embeddable cryptographic acceleration cores.
 */


#include <linux/types.h>



#ifdef _cplusplus
extern "C" {
#endif



/*
 * Basic header construction definitions
 */

#define EU_SEL_MASK           (0x00000f00)
#define EU_MODE_MASK          (0x000000ff)
#define EU_SEL_SHIFT          (8)

/*
 * Header bits for EU selection:  0 -  3 (primary)
 *                               12 - 15 (secondary)
 */
#define EU_NONE               (0x00 << EU_SEL_SHIFT) /**< No EU selected          */
#define EU_ARC4               (0x01 << EU_SEL_SHIFT) /**< RC-4                    */
#define EU_DES                (0x02 << EU_SEL_SHIFT) /**< DES                     */
#define EU_MD                 (0x03 << EU_SEL_SHIFT) /**< SHA160/225/256/MD5      */
#define EU_RND                (0x04 << EU_SEL_SHIFT) /**< Randomizer              */
#define EU_PK                 (0x05 << EU_SEL_SHIFT) /**< Public Key              */
#define EU_AES                (0x06 << EU_SEL_SHIFT) /**< AES                     */
#define EU_KEA                (0x07 << EU_SEL_SHIFT) /**< Kasumi                  */
#define EU_CRC                (0x08 << EU_SEL_SHIFT) /**< CRC unit                */
#define EU_SNOW               (0x09 << EU_SEL_SHIFT) /**< SNOW-3G unit            */
#define EU_MDPRIME            (0x0b << EU_SEL_SHIFT) /**< SHA256/384/512 -t3 only */


/* MDHA Mode Bits                                            */
/* |  56  |  57  |  58  |  59  |  60  |  61  |  62  |  63  | */
/* | CONT | CICV | SMAC | INIT | HMAC |  PD  |     ALG     | */
#define MD_CONT               (0x80)
#define MD_CICV               (0x40)
#define MD_SMAC               (0x20)
#define MD_INIT               (0x10)
#define MD_HMAC               (0x08)
#define MD_PD                 (0x04)
#define MD_SHA1               (0x00)
#define MD_SHA256             (0x01)
#define MD_MD5                (0x02)
#define MD_SHA224             (0x03)
#define MD_SHA384             (0x00)  /* in MDEU-B mode */
#define MD_SHA512             (0x02)  /* in MDEU-B mode */


/* DES Mode Bits                                             */
/* |  56  |  57  |  58  |  59  |  60  |  61  |  62  |  63  | */
/*                             |     CM      |  TS  |  ED  | */
#define DES_ENCRYPT           (0x01)
#define DES_DECRYPT           (0x00)
#define DES_TRIPLE            (0x02)
#define DES_SINGLE            (0x00)
#define DES_CBC               (0x04)
#define DES_ECB               (0x00)
#define DES_CFB_64            (0x08)
#define DES_OFB_64            (0x0c)


/* AES Mode Bits                                                     */
/* |  56  |  57  |  58  |  59  |  60  |  61  |  62  |  63  |         */
/* |     ECM     | AUX2 | AUX1 | AUX0 |     CM      |  ED  | T3 view */
/* |     ECM     |  FM  |  IM  | RDK  |     CM      |  ED  | T2 view */
#define AES_ENCRYPT           (0x01)
#define AES_DECRYPT           (0x00)
#define AES_RDK               (0x08)
#define AES_INITMAC           (0x10)
#define AES_FINALMAC          (0x20)
#define AES_AUX0              (0x08)
#define AES_AUX1              (0x10)
#define AES_AUX2              (0x20)

#define AES_ECB               ((0x00 << 6) | (0x00 << 1))
#define AES_CBC               ((0x00 << 6) | (0x01 << 1))
#define AES_CBC_RBP           ((0x00 << 6) | (0x01 << 1) | AES_FINALMAC)
#define AES_CBC_OFB           ((0x00 << 6) | (0x02 << 1))
#define AES_CTR               ((0x00 << 6) | (0x03 << 1))
#define AES_LRW_NO_TWEAK      ((0x01 << 6) | (0x00 << 1) | AES_FINALMAC)
#define AES_LRW               ((0x01 << 6) | (0x00 << 1))
#define AES_CMAC              ((0x01 << 6) | (0x02 << 1))
#define AES_CMAC_ICV          ((0x01 << 6) | (0x02 << 1) | AES_FINALMAC)
#define AES_SRTP              ((0x01 << 6) | (0x03 << 1))
#define AES_CCM               ((0x02 << 6) | (0x00 << 1))
#define AES_CCM_FMAC          ((0x02 << 6) | (0x00 << 1) | AES_FINALMAC)
#define AES_GCM               ((0x02 << 6) | (0x01 << 1))
#define AES_GCM_FMAC          ((0x02 << 6) | (0x01 << 1) | AES_FINALMAC)
#define AES_XCBC_MAC          ((0x02 << 6) | (0x02 << 1))
#define AES_XCBC_MAC_ICV      ((0x02 << 6) | (0x02 << 1) | AES_FINALMAC)
#define AES_CFB128            ((0x02 << 6) | (0x03 << 1))
#define AES_CCM               ((0x02 << 6) | (0x00 << 1))
#define AES_CCM_ICV           ((0x03 << 6) | (0x00 << 1))
#define AES_GCM_ICV           ((0x03 << 6) | (0x01 << 1))
#define AES_XOR               ((0x03 << 6) | (0x03 << 1))


/* PKHA Mode Bits                                             */
/* |  56  |  57  |  58  |  59  |  60  |  61  |  62  |  63  |  */
/* |                     ROUTINE                              */
#define PK_CLEARMEM           (0x01) /**< Clear PKHA memory           */
#define PK_MOD_EXP            (0x02) /**< Expo mod N,
                                            convert from Monty        */
#define PK_MOD_R2MODN         (0x03) /**< Compute Monty converter     */
#define PK_MOD_RRMODP         (0x04) /**< Compute Monty converter
                                            for CRT                   */
#define PK_EC_FP_AFF_PTMULT   (0x05) /**< FP scalar * point in
                                            affine system             */
#define PK_EC_F2M_AFF_PTMULT  (0x06) /**< F2M scalar * point in
                                            affine system             */
#define PK_EC_FP_PROJ_PTMULT  (0x07) /**< FP scalar
                                            * point projective        */
#define PK_EC_F2M_PROJ_PTMULT (0x08) /**< F2M scalar
                                            * point projective        */
#define PK_EC_FP_ADD          (0x09) /**< FP Add two points           */
#define PK_EC_FP_DOUBLE       (0x0a) /**< FP double a point           */
#define PK_EC_F2M_ADD         (0x0b) /**< F2M Add two points          */
#define PK_EC_F2M_DOUBLE      (0x0c) /**< F2M double a point          */
#define PK_F2M_R2             (0x0d) /**< F2M R2 mod N montgomery
                                            converter                 */
#define PK_F2M_INV            (0x0e) /**< F2M modular inversion       */
#define PK_FP_MODINV          (0x0f) /**< FP modular inversion        */
#define PK_FP_MODADD          (0x10) /**< FP modular addition         */
#define PK_FP_MODSUB          (0x20) /**< FP modular subtraction      */
#define PK_FP_MULT_MONT       (0x30) /**< FP multiply in montgomery   */
#define PK_FP_MULT_DECONV     (0x40) /**< FP multiply and deconvert
                                            from monty                */
#define PK_F2M_ADD            (0x50) /**< F2M modular addition        */
#define PK_F2M_MULT_MONT      (0x60) /**< F2M multiply in montgomery  */
#define PK_F2M_MULT_DECONV    (0x70) /**< F2M multiply and deconvert
                                            from monty                */
#define PK_RSA_SS             (0x80) /**< Single-Step RSA expo        */
#define PK_MOD_EXP_TEQ        (0x1d) /**< Expo ModN with timing EQ    */
#define PK_RSA_SS_TEQ         (0x1e) /**< SingleStep RSA w/timing EQ  */
#define PK_PKBUILD            (0xff) /**< Build the ECC intermediate
                                            structure                 */

/* ARC4 mode bits                                            */
/* |  56  |  57  |  58  |  59  |  60  |  61  |  62  |  63  | */
/*                                    |  CS  |  DC  |  PP  | */
#define ARC4_PERMUTE_INHIBIT  (0x01) /**< don't init Sbox if true     */
#define ARC4_DUMP_CONTEXT     (0x02) /**< write context when done     */
#define ARC4_FETCH_CTX        (0x04) /**< read context
                                            before starting           */

/* Kasumi mode bits                                          */
/* |  56  |  57  |  58  |  59  |  60  |  61  |  62  |  63  | */
/* | GSM  | CICV | EDGE |  PE  | INT  |      |     ALG     | */
#define KEA_F8                (0x00) /**< f8 function                 */
#define KEA_F9                (0x02) /**< f9 function                 */
#define KEA_INIT              (0x08) /**< setup for new message       */
#define KEA_EOM               (0x10) /**< end of message              */
#define KEA_EDGE              (0x20) /**< EDGE-compatible reads       */
#define KEA_ICV               (0x40) /**< inline integrity check      */
#define KEA_GSM               (0x80) /**< GSM-compatible reads        */


/* CRC mode bits                                              */
/* |  56  |  57  |  58  |  59  |  60  |  61  |  62  |  63  |  */
/* | RAW  | CICV |                           |     ALG     |  */
#define CRC_RAW               (0x80) /**< do not perform result
                                            manupulation              */
#define CRC_ICV               (0x40) /**< perform inline integrity
                                            check                     */
#define CRC_802               (0x00) /**< Use 802.x
                                            polynomial/residue        */
#define CRC_ISCSI             (0x01) /**< Use iSCSI
                                            polynomial/residue        */
#define CRC_STATIC_CUSTOM     (0x02) /**< Use Control Register
                                            for remainder             */
#define CRC_DYNAMIC_CUSTOM    (0x03) /**< Use Key register
                                            for remainder             */

/* SNOW-3G mode bits                                          */
/* |  56  |  57  |  58  |  59  |  60  |  61  |  62  |  63  |  */
/* |      | CICV |      |  PE  | INT  |         ALG        |  */
#define SNOW_F8               (0x01) /**< F8 cipher                  */
#define SNOW_F9               (0x02) /**< F9 integrity check         */
#define SNOW_INIT             (0x08) /**< Initialization (new msg)   */
#define SNOW_EOM              (0x10) /**< End of message (finalize)  */
#define SNOW_ICV              (0x40) /**< inline integrity check     */


/* Remaining header stuff, shift/mask for EU selection and descriptor type */

#define EU_SHIFT_PRIMARY    (20)
#define EU_SHIFT_SECONDARY  (8)

#define DESCTYPE_SHIFT      (3)
#define DESCTYPE_MASK       (0x0000001f << DESCTYPE_SHIFT)


/*
 * AES counter-mode nonsnooping       0  b0000_0
 * aesu_ctr_nosnoop
 * p0 - (reserved)
 * p1 - Cipher Context In
 * p2 - Cipher Key
 * p3 - Main Data In
 * p4 - Data Out
 * p5 - Cipher Context Out
 * p6 - (reserved)
 */
#define DESCTYPE_AES_CTR        (0 << DESCTYPE_SHIFT)


/*
 * IPSec ESP singlepass               1  b0000_1
 * ipsec_esp
 * p0 - HMAC Key
 * p1 - Hash-only Header
 * p2 - Cipher IV In
 * p3 - Cipher Key
 * p4 - Main Data In          ext: ICV In
 * p5 - Data Out              ext: ICV Out
 * p6 - Cipher IV Out
 */
#define DESCTYPE_IPSEC_ESP      (1 << DESCTYPE_SHIFT)


/*
 * Common nonsnooping no RC4          2  b0001_0
 * common_nosnoop DES, Kasumi f8, RNG, AES-CCM
 * p0 - (reserved)
 * p1 - Context In
 * p2 - Key
 * p3 - Main Data In
 * p4 - Data Out
 * p5 - Context Out
 * p6 - (reserved)
 *
 * common_nosnoop MDEU
 * p0 - (reserved)
 * p1 - Context In
 * p2 - Key
 * p3 - Main Data In
 * p4 - ICV In
 * p5 - Context Out
 * p6 - (reserved)
 *
 * common_nosnoop Kasumi f9, AES-XCBC, AES_CMAC, CRCA
 * p0 - (reserved)
 * p1 - Context In
 * p2 - Key
 * p3 - Main Data In
 * p4 - (reserved)            ext: ICV in
 * p5 - Context Out
 * p6 - ICV Out
 */
#define DESCTYPE_COMMON         (2 << DESCTYPE_SHIFT)


/*
 * AES CCMP singlepass 802.11i        3  b0001_1
 * 802.11i_AES_ccmp
 * p0 - CRC-only Header       ext: CRC In/Out (FCS)
 * p1 - AES Context In
 * p2 - AES Key
 * p3 - Hash-Only Header
 * p4 - Main Data In          ext: MIC In
 * p5 - Data Out              ext: MIC Out
 * p6 - AES Context Out
 */
#define DESCTYPE_AES_CCMP       (3 << DESCTYPE_SHIFT)


/*
 * HMAC snooping no RC4               4  b0010_0
 * hmac_snoop_no_afeu
 * p0 - Hash Key
 * p1 - Hash-Only Header
 * p2 - Cipher Key
 * p3 - Cipher Context In
 * p4 - Main Data In
 * p5 - Data Out
 * p6 - ICV Out
 */
#define DESCTYPE_HMAC           (4 << DESCTYPE_SHIFT)


/*
 * SRTP single-pass                   5  b010_1
 * srtp_with_ICV_check
 * p0 - HMAC Key
 * p1 - AES Context in
 * p2 - AES Key
 * p3 - Main Data In          ext: Hash-Only Header
 * p4 - Data Out              ext: Hash-Only Trailer
 * p5 - HMAC Out
 * p6 - AES Context Out
 *
 * srtp_without_ICV_check
 * p0 - HMAC Key
 * p1 - AES Context in
 * p2 - AES Key
 * p3 - Main Data In          ext: Hash-Only Header
 * p4 - HMAC In               ext: Hash-Only Trailer
 * p5 - Data Out              ext: HMAC Out
 * p6 - AES Context Out
 */
#define DESCTYPE_SRTP           (5 << DESCTYPE_SHIFT)


/*
 * (reserved)                         6  b0011_0
 */


/*
 * ECC data assemble                  7  b0011_1
 * pkeu_build
 * p0 - A0 In
 * p1 - A1 In
 * p2 - A2 In
 * p3 - A3 In
 * p4 - B0 In
 * p5 - B1 In
 * p6 - Build Out
 */
#define DESCTYPE_PK_ECC_ASM     (7 << DESCTYPE_SHIFT)


/*
 * (reserved)                         8  b0100_0
 */


/*
 * ECC point multiply                 9  b0100_1
 * pkeu_ptmul
 * p0 - N In
 * p1 - E In
 * p2 - Build In
 * p3 - B1 Out
 * p4 - B2 Out
 * p5 - B3 out
 * p6 - (reserved)
 */
#define DESCTYPE_PK_ECC_PTMULT  (9 << DESCTYPE_SHIFT)


/*
 * Common nonsnooping RC4            10  b0101_0
 * common_nosnoop_afeu
 * p0 - (reserved)
 * p1 - Context In (via in FIFO)
 * p2 - Cipher Key
 * p3 - Main Data In
 * p4 - Data Out
 * p5 - Context Out (via out FIFO)
 * p6 - (reserved)
 */
#define DESCTYPE_ARC4           (10 << DESCTYPE_SHIFT)


/*
 * ECC point add-double              11  b0101_1
 * pkeu_ptadd_dbl
 * p0 - N In
 * p1 - Build In
 * p2 - B2 In
 * p3 - B3 In
 * p4 - B1 Out
 * p5 - B2 Out
 * p6 - B3 Out
 */
#define DESCTYPE_PK_ECC_PTADD_D (11 << DESCTYPE_SHIFT)


/*
 * (reserved)                        12-15
 */


/*
 * Montgomery multiply               16  b1000_0
 * pkeu_mm
 * p0 - N In
 * p1 - B In
 * p2 - A In
 * p3 - E In
 * p4 - B Out
 * p5 - (reserved)
 * p6 - (reserved)
 */
#define DESCTYPE_PK_MONTY       (16 << DESCTYPE_SHIFT)


/*
 * TLS singlepass blockcipher        17  b1000_1
 * tls_ssl_block (outbound)
 * p0 - MAC Key
 * p1 - Cipher IV In
 * p2 - Cipher Key
 * p3 - Main Data In          ext: Hash-Only Header
 * p4 - Cipher-Only Trailer   ext: ICV Out
 * p5 - Data Out
 * p6 - Cipher IV Out
 *
 * tls_ssl_block (inbound)
 * p0 - MAC Key
 * p1 - Cipher IV In
 * p2 - Cipher Key
 * p3 - (reserved)            ext: Hash-Only Header
 * p4 - Main Data In          ext: ICV In
 * p5 - Main Data Out         ext: ICV Out
 * p6 - Cipher IV Out
 */
#define DESCTYPE_TLS_BLOCK      (17 << DESCTYPE_SHIFT)


/*
 * (reserved)                        18  b1001_0
 */


/*
 * TLS singlepass streamcipher       19  b1001_1
 * tls_ssl_stream (outbound)
 * p0 - MAC Key
 * p1 - Cipher IV In
 * p2 - Cipher Key
 * p3 - Main Data In          ext: Hash-Only Header
 * p4 - (reserved)            ext: ICV Out
 * p5 - Data Out
 * p6 - Cipher IV Out
 *
 * tls_ssl_stream (inbound)
 * p0 - MAC Key
 * p1 - Cipher IV In
 * p2 - Cipher Key
 * p3 - (reserved)            ext: Hash-Only Header
 * p4 - Main Data In          ext: ICV In
 * p5 - Data Out              ext: ICV Out
 * p6 - Cipher IV Out
 */
#define DESCTYPE_TLS_STREAM     (19 << DESCTYPE_SHIFT)


/*
 * (reserved)                        20  b1010_0
 */


/*
 * RAID-XOR multiple sources         21  b1010_1
 * raid_xor
 * p0 - Source F Data In
 * p1 - Source E Data In
 * p2 - Source D Data In
 * p3 - Source C Data In
 * p4 - Source B Data In
 * p5 - Source A Data In
 * p6 - Data Out
 */
#define DESCTYPE_RAIDXOR        (21 << DESCTYPE_SHIFT)


/*
 * (reserved)                        22  b1011_0
 */


/*
 * IPSec ESP with AES-ECM            23  b1011_1
 * ipsec_aes_gcm
 * p0 - AES Context In
 * p1 - AAD In
 * p2 - Nonce Part 2 In
 * p3 - AES Key In            ext: Nonce Part 1 In
 * p4 - Main Data In          ext: AES ICV In
 * p5 - Data Out              ext: AES ICV Out
 * p6 - Cipher Context Out    ext: CRC ICV In/Out
 */
#define DESCTYPE_IPSEC_AES_GCM  (23 << DESCTYPE_SHIFT)


/*
 * AES CTR snoop (-HMAC)             24  b1100_0
 * hmac_snoop_aesu_ctr
 * p0 - Hash Key
 * p1 - Hash-Only Header
 * p2 - AES Key
 * p3 - AES Context In
 * p4 - Main Data In
 * p5 - Data Out
 * p6 - ICV Out
 */
#define DESCTYPE_AES_HMAC       (24 << DESCTYPE_SHIFT)


/*
 * Double CRC operation              25  b1100_1
 * dbl_digest
 * p0 - Header In             ext: Header ICV
 * p1 - Payload In            ext: Payload ICV
 * p2 - (reserved)            ext: Header ICV Out
 * p3 - (reserved)            ext: Payload ICV Out
 * p4 - (reserved)
 * p5 - (reserved)
 * p6 - (reserved)
 */
#define DESCTYPE_DBLCRC         (25 << DESCTYPE_SHIFT)

/*
 * (reserved)                        26-31
 */


/* Remaining Header Bits */
#define HDR_OUTBOUND        (0x00000000)
#define HDR_INBOUND         (0x00000002)
#define HDR_DONE            (0x00000001)





/*
 * General register structure of a SEC2 or 3 core
 */

/** Register location offset relative to IMMR base or equivalent */
#define T2_BASEADDR_OFFSET            (0x00030000)

/** Size of register bank space requested from memory manager */
#define T2_REGISTER_BANK_SIZE         (0x00010000)


/** fixed depth of a T2 fetch FIFO */
#define T2_CHANNEL_FIFO_DEPTH         (24)


#ifdef OBSOLETE
#define T2_TOTAL_CHANNELS             (4)
#define T2_TOTAL_EUS                  (7)
#endif

#define T3_MAX_CHANNELS               (10) /* theoretical max, based on */
                                           /* register map              */
#define T3_MAX_EUS                    (9)  /* includes MDEU-B */

    typedef u8 _resvd;




    /*
     * Content of the device controller block
     * This controls all aspects of the core's interaction with
     * the remainder of the system (interrupts, bus priorities, etc.
     *
     */

    /* EU assignment status register - 0x31028 ro */
#define T2_EUASR_AFEU_MASK                 0x0f00000000000000ull
#define T2_EUASR_AFEU_SHIFT                56
#define T2_EUASR_MDEU_MASK                 0x000f000000000000ull
#define T2_EUASR_MDEU_SHIFT                48
#define T2_EUASR_AESU_MASK                 0x00000f0000000000ull
#define T2_EUASR_AESU_SHIFT                40
#define T2_EUASR_DEU_MASK                  0x0000000f00000000ull
#define T2_EUASR_DEU_SHIFT                 32
#define T2_EUASR_KEU_MASK                  0x00000000000f0000ull
#define T2_EUASR_KEU_SHIFT                 16
#define T2_EUASR_PKEU_MASK                 0x0000000000000f00ull
#define T2_EUASR_PKEU_SHIFT                8
#define T2_EUASR_RNG_MASK                  0x000000000000000full
#define T2_EUASR_RNG_SHIFT                 0
#define T2_EUASR_STEU_MASK                 0x000000000000f000ull
#define T2_EUASR_STEU_SHIFT                12


    /* Interrupt Mask/Status/Clear (IMR/ISR/ICR) defs */
    /* IMR - 0x31008 rw */
    /* ISR - 0x31010 ro */
    /* ICR - 0x31018 wo */
#define T2_IMR_INTERNAL_TIMEOUT            0x0001000000000000ull

#define T2_IMR_DONE_OVERFLOW_CH0           0x0000010000000000ull
#define T2_IMR_DONE_OVERFLOW_CH1           0x0000020000000000ull
#define T2_IMR_DONE_OVERFLOW_CH2           0x0000040000000000ull
#define T2_IMR_DONE_OVERFLOW_CH3           0x0000080000000000ull

#define T2_IMR_ERROR_CH0                   0x0000000200000000ull
#define T2_IMR_ERROR_CH1                   0x0000000800000000ull
#define T2_IMR_ERROR_CH2                   0x0000002000000000ull
#define T2_IMR_ERROR_CH3                   0x0000008000000000ull
#define T2_IMR_ERROR_ALL_CHANNELS          (T2_IMR_ERROR_CH0 | \
        T2_IMR_ERROR_CH0 | \
        T2_IMR_ERROR_CH0 | \
        T2_IMR_ERROR_CH0)
#define T2_IMR_ERROR_STEP                  2

#define T2_IMR_DONE_CH0                    0x0000000100000000ull
#define T2_IMR_DONE_CH1                    0x0000000400000000ull
#define T2_IMR_DONE_CH2                    0x0000001000000000ull
#define T2_IMR_DONE_CH3                    0x0000004000000000ull
#define T2_IMR_DONE_ALL_CHANNELS           (T2_IMR_DONE_CH0 | \
        T2_IMR_DONE_CH1 | \
        T2_IMR_DONE_CH2 | \
        T2_IMR_DONE_CH3)
#define T2_IMR_DONE_STEP                   2

#define T2_IMR_DONE_DEU                    0x0000000000000001ull
#define T2_IMR_ERROR_DEU                   0x0000000000000002ull
#define T2_IMR_DONE_AESU                   0x0000000000000010ull
#define T2_IMR_ERROR_AESU                  0x0000000000000020ull
#define T2_IMR_DONE_MDEU                   0x0000000000000100ull
#define T2_IMR_ERROR_MDEU                  0x0000000000000200ull
#define T2_IMR_DONE_AFEU                   0x0000000000001000ull
#define T2_IMR_ERROR_AFEU                  0x0000000000002000ull
#define T2_IMR_DONE_RNG                    0x0000000000010000ull
#define T2_IMR_ERROR_RNG                   0x0000000000020000ull
#define T2_IMR_DONE_PKEU                   0x0000000000100000ull
#define T2_IMR_ERROR_PKEU                  0x0000000000200000ull
#define T2_IMR_DONE_KEU                    0x0000000001000000ull
#define T2_IMR_ERROR_KEU                   0x0000000002000000ull
#define T2_IMR_DONE_STEU                   0x0000000000400000ull
#define T2_IMR_ERROR_STEU                  0x0000000000800000ull


/* Master Control Register (MCR) defs - 0x31030 rw */

#define T2_MCR_BUSPRIORITY_LOW             0x0000000000000000ull
#define T2_MCR_BUSPRIORITY_MED_LOW         0x0000010000000000ull
#define T2_MCR_BUSPRIORITY_MED_HIGH        0x0000020000000000ull
#define T2_MCR_BUSPRIORITY_HIGH            0x0000030000000000ull

/* GI exists only in the PQ2pro variants */
#define T2_MCR_GLOBAL_INHIBIT              0x0000000200000000ull

#define T2_MCR_SOFTWARE_RESET              0x0000000100000000ull



/** Register-level view of a controller block, 256 bytes */
typedef struct _T2controller {
    _resvd       _reserved1[8];                  /**< 0x00 - 0x07 */
    volatile u64 intMask;                        /**< 0x08 - 0x0f */
    volatile u64 intStatus;                      /**< 0x10 - 0x17 */
    volatile u64 intClear;                       /**< 0x18 - 0x1f */
    volatile u64 ID;                             /**< 0x20 - 0x27 */
    volatile u64 euStatus;                       /**< 0x28 - 0x2f */
    volatile u64 masterControl;                  /**< 0x30 - 0x37 */
    _resvd       _reserved2[200];                /**< 0x38 - 0xff */
} T2CONTROLLER;



    /*
     * Content of a channel control block
     * Provides for a DMA-like interface between cryptographic services
     * and host memory
     */

    /* Channel Control Register (CCR) defs - 0x31x08 rw*/
#define T2_CCR_CONTINUE                    0x0000000200000000ull
#define T2_CCR_RESET                       0x0000000100000000ull

#define T2_CCR_BURSTSIZE_128               0x0000000000000100ull
#define T2_CCR_BURSTSIZE_64                0x0000000000000000ull
#define T2_CCR_ICV_STATUS_WRITEBACK        0x0000000000000080ull
#define T2_CCR_ADDRESS_32                  0x0000000000000000ull
#define T2_CCR_ADDRESS_36                  0x0000000000000020ull
#define T2_CCR_CHANNEL_DONE_WRITEBACK      0x0000000000000010ull
#define T2_CCR_STATUS_WRITEBACK            0x0000000000000008ull
#define T2_CCR_SELECTIVE_NOTIFY            0x0000000000000004ull
#define T2_CCR_CHANNEL_DONE_INTERRUPT      0x0000000000000002ull

/* Channel Pointer Status Register (CPSR) defs - 0x31x10 ro */
/* Note these are mildly different between T2/3, but the    */
/* error bits are basically the same                        */
#define T2_CPSR_FF_COUNTER_MASK            0x1f00000000000000ull
#define T2_CPSR_FF_COUNTER_SHIFT           56
#define T2_CPSR_G_STATE_MASK               0x000f000000000000ull
#define T2_CPSR_G_STATE_SHIFT              48
#define T2_CPSR_S_STATE_MASK               0x00000f0000000000ull
#define T2_CPSR_S_STATE_SHIFT              40
#define T2_CPSR_CHN_STATE_MASK             0x000000ff00000000ull
#define T2_CPSR_CHN_STATE_SHIFT            32

#define T3_CPSR_GET_STATE_MASK             0x7f00000000000000ull
#define T3_CPSR_GET_STATE_SHIFT            56
#define T3_CPSR_PUT_STATE_MASK             0x007f000000000000ull
#define T3_CPSR_PUT_STATE_SHIFT            48
#define T3_CPSR_MAIN_STATE_MASK            0x000001ff00000000ull
#define T3_CPSR_MAIN_STATE_SHIFT           32

#define T2_CPSR_MI                         0x0000000002000000ull
#define T2_CPSR_MO                         000000000001000000ull
#define T2_CPSR_PR                         0x0000000000800000ull
#define T2_CPSR_SR                         0x0000000000400000ull
#define T2_CPSR_PG                         0x0000000000200000ull
#define T2_CPSR_SG                         0x0000000000100000ull
#define T2_CPSR_PRD                        0x0000000000080000ull
#define T2_CPSR_SRD                        0x0000000000040000ull
#define T2_CPSR_PD                         0x0000000000020000ull
#define T2_CPSR_SD                         0x0000000000010000ull
#define T2_CPSR_ERROR_MASK                 0x000000000000fff0ull
#define T2_CPSR_ERROR_SHIFT                4
#define T2_CPSR_PAIR_PTR_MASK              0x000000000000000full

#define T3_CPSR_FF_LEVEL_MASK              0x000000001f000000ull
#define T3_CPSR_FF_LEVEL_SHIFT             24
#define T3_CPSR_PRD                        0x0000000000080000ull
#define T3_CPSR_SRD                        0x0000000000040000ull
#define T3_CPSR_PD                         0x0000000000020000ull
#define T3_CPSR_SD                         0x0000000000010000ull

#define T2_CHN_ERROR_DOF                   0x0000000000008000ull
#define T2_CHN_ERROR_SOF                   0x0000000000004000ull
#define T2_CHN_ERROR_MDTE                  0x0000000000002000ull
#define T2_CHN_ERROR_SG_ZERO_LEN           0x0000000000001000ull
#define T2_CHN_ERROR_FP_ZERO               0x0000000000000800ull
#define T2_CHN_ERROR_ILLEGAL_HEADER        0x0000000000000400ull
#define T2_CHN_ERROR_INVALID_EU            0x0000000000000200ull
#define T2_CHN_ERROR_EU_ERROR              0x0000000000000100ull
#define T2_CHN_ERROR_G_BOUNDARY            0x0000000000000080ull
#define T2_CHN_ERROR_G_LENGTH              0x0000000000000040ull
#define T2_CHN_ERROR_S_BOUNDARY            0x0000000000000020ull
#define T2_CHN_ERROR_S_LENGTH              0x0000000000000010ull



/** Register view of a channel block - 256 bytes */
typedef struct _T2channel {
    _resvd       _reserved1[8];                  /**< 0x00 - 0x07 */
    volatile u64 config;                         /**< 0x08 - 0x0f */
    volatile u64 pointerStatus;                  /**< 0x10 - 0x17 */
    _resvd       _reserved2[40];                 /**< 0x18 - 0x3f */
    volatile u64 currentDesc;                    /**< 0x40 - 0x47 */
    volatile u64 fetchFIFO;                      /**< 0x48 - 0x4f */
    _resvd       _reserved3[48];                 /**< 0x50 - 0x7f */
    volatile u64 descBuffer[8];                  /**< 0x80 - 0xbf */
    volatile u64 gatherLinkTbl[4];               /**< 0xc0 - 0xdf */
    volatile u64 scatterLinkTbl[4];              /**< 0xe0 - 0xff */
} T2CHANNEL;


/** Register view of the T3 polychannel status - padded to 256 bytes */
typedef struct _T2polychn {
    volatile u64 fetchFIFOenQ;                   /**< 0x00 - 0x07 */
    volatile u64 descFinished;                   /**< 0x08 - 0x0f */
    volatile u64 dataBytesIn;                    /**< 0x10 - 0x17 */
    volatile u64 dataBytesOut;                   /**< 0x18 - 0x1f */
    _resvd       _reserved[224];                 /**< 0x20 - 0xff */
} T3POLYCHN;

/** Register view of the IP block ID region - 256 bytes */
typedef struct _T2IPID {
    _resvd         _reserved0[248];              /**< 0x00 - 0xf7 */
    volatile u64 id;                             /**< 0xf8 = 0xff */
} T2IPID;


/** DES (DEU) block register view 0x---32000 */
typedef struct _T2EU_DES {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f RW */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    _resvd         _reserved[8];                 /**< 0x020 - 0x027 -- */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RO */
    volatile u64   interruptControl;             /**< 0x038 - 0x03f RW */
    _resvd         _reserved1[16];               /**< 0x040 - 0x04f -- */
    volatile u64   EUgo;                         /**< 0x050 - 0x057 WO */
    _resvd         _reserved2[168];              /**< 0x058 - 0x0ff -- */
    volatile u8    IV[8];                        /**< 0x100 - 0x107 RW */
    _resvd         _reserved3[760];              /**< 0x108 - 0x3ff -- */
    volatile u8    key1[8];                      /**< 0x400 - 0x407 WO */
    volatile u8    key2[8];                      /**< 0x408 - 0x40f WO */
    volatile u8    key3[8];                      /**< 0x410 - 0x417 WO */
    _resvd         _reserved4[1000];             /**< 0x418 - 0x7ff -- */
    volatile u8    FIFO[2048];                   /**< 0x800 - 0xfff RW */
} T2EU_DES;


/** AES (AESU) block register view 0x---34000 */
typedef struct _T2EU_AES {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f RW */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    _resvd         _reserved[8];                 /**< 0x020 - 0x027 -- */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RO */
    volatile u64   interruptControl;             /**< 0x038 - 0x03f RW */
    _resvd         _reserved1[16];               /**< 0x040 - 0x04f -- */
    volatile u64   EUgo;                         /**< 0x050 - 0x057 WO */
    _resvd         _reserved2[168];              /**< 0x058 - 0x0ff -- */
    volatile u8    context[8];                   /**< 0x100 - 0x107 RW */
    _resvd         _reserved3[760];              /**< 0x108 - 0x3ff -- */
    volatile u8    key[8];                       /**< 0x400 - 0x407 RW */
    _resvd         _reserved4[1016];             /**< 0x408 - 0x7ff -- */
    volatile u8    FIFO[2048];                   /**< 0x800 - 0xfff RW */
} T2EU_AES;


/** MD (MDEU) block register view 0x---36000 */
typedef struct _T2EU_MD {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f RW */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    _resvd         _reserved[8];                 /**< 0x020 - 0x027 -- */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RO */
    volatile u64   interruptControl;             /**< 0x038 - 0x03f RW */
    volatile u64   ICVsize;                      /**< 0x040 - 0x047 RW */
    _resvd         _reserved1[8];                /**< 0x048 - 0x04f -- */
    volatile u64   EUgo;                         /**< 0x050 - 0x057 WO */
    _resvd         _reserved2[168];              /**< 0x058 - 0x0ff -- */
    volatile u8    context[33];                  /**< 0x100 - 0x120 RW */
    _resvd         _reserved3[735];              /**< 0x121 - 0x3ff -- */
    volatile u8    key[128];                     /**< 0x400 - 0x47f WO */
    _resvd         _reserved4[896];              /**< 0x480 - 0x7ff -- */
    volatile u8    FIFO[2048];                   /**< 0x800 - 0xfff WO */
} T2EU_MD;


/** ARC (AFEU) block register view 0x---38000 */
typedef struct _T2EU_ARC4 {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f RW */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    _resvd         _reserved[8];                 /**< 0x020 - 0x027 -- */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RO */
    volatile u64   interruptControl;             /**< 0x038 - 0x03f RW */
    _resvd         _reserved1[16];               /**< 0x040 - 0x04f -- */
    volatile u64   EUgo;                         /**< 0x050 - 0x057 WO */
    _resvd         _reserved2[168];              /**< 0x058 - 0x0ff -- */
    volatile u8    contextMem[256];              /**< 0x100 - 0x1ff RW */
    volatile u8    contextPtr[512];              /**< 0x200 - 0x3ff RW */
    volatile u8    key0[128];                    /**< 0x400 - 0x47f WO */
    volatile u8    key1[128];                    /**< 0x480 - 0x4ff WO */
    _resvd         _reserved3[768];              /**< 0x500 - 0x7ff -- */
    volatile u8    FIFO[2048];                   /**< 0x800 - 0xfff RW */
} T2EU_ARC4;


/** Random Generator (RNG) block register view 0x---3a000 */
typedef struct _T2EU_RND {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f RW */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    _resvd         _reserved[8];                 /**< 0x020 - 0x027 -- */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RO */
    volatile u64   interruptControl;             /**< 0x038 - 0x03f RW */
    _resvd         _reserved1[16];               /**< 0x040 - 0x04f -- */
    volatile u64   EUgo;                         /**< 0x050 - 0x057 WO */
    _resvd         _reserved2[1960];             /**< 0x058 - 0x7ff -- */
    volatile u8    FIFO[2048];                   /**< 0x800 - 0xfff RW */
} T2EU_RND;


/** PKHA block register view 0x---3c000 */
typedef struct _T2EU_PKHA {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f RW */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    _resvd         _reserved[8];                 /**< 0x020 - 0x027 -- */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RO */
    volatile u64   interruptControl;             /**< 0x038 - 0x03f RW */
    volatile u64   ABsize;                       /**< 0x040 - 0x047 RW */
    _resvd         _reserved1[8];                /**< 0x048 - 0x04f -- */
    volatile u64   EUgo;                         /**< 0x050 - 0x057 WO */
    _resvd         _reserved2[424];              /**< 0x058 - 0x1ff -- */
    volatile u8    paramA0[64];                  /**< 0x200 - 0x23f RW */
    volatile u8    paramA1[64];                  /**< 0x240 - 0x27f RW */
    volatile u8    paramA2[64];                  /**< 0x280 - 0x2bf RW */
    volatile u8    paramA3[64];                  /**< 0x2c0 - 0x2ff RW */
    volatile u8    paramB0[64];                  /**< 0x300 - 0x33f RW */
    volatile u8    paramB1[64];                  /**< 0x340 - 0x37f RW */
    volatile u8    paramB2[64];                  /**< 0x380 - 0x3bf RW */
    volatile u8    paramB3[64];                  /**< 0x3c0 - 0x3ff RW */
    volatile u8    paramE[256];                  /**< 0x400 - 0x4ff WO */
    _resvd         _reserved3[768];              /**< 0x500 - 0x7ff -- */
    volatile u8    paramN[256];                  /**< 0x800 - 0x8ff RW */
    _resvd         _reserved4[1792];             /**< 0x900 - 0xfff -- */
} T2EU_PKHA;


/** Kasumi (KEU) block register view 0x---3e000 */
typedef struct _T2EU_KEA {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f RW */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    _resvd         _reserved[8];                 /**< 0x020 - 0x027 -- */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RO */
    volatile u64   interruptControl;             /**< 0x038 - 0x03f RW */
    _resvd         _reserved1[8];                /**< 0x040 - 0x047 -- */
    volatile u64   F9MAC;                        /**< 0x048 - 0x04f R0 */
    volatile u64   EUgo;                         /**< 0x050 - 0x057 WO */
    _resvd         _reserved2[168];              /**< 0x058 - 0x0ff -- */
    volatile u64   IV1;                          /**< 0x100 - 0x107 RW */
    volatile u64   ICVin;                        /**< 0x108 - 0x10f RW */
    volatile u64   IV2;                          /**< 0x110 - 0x117 RW */
    volatile u64   context1;                     /**< 0x118 - 0x11f RW */
    volatile u64   context2;                     /**< 0x120 - 0x127 RW */
    volatile u64   context3;                     /**< 0x128 - 0x12f RW */
    volatile u64   context4;                     /**< 0x130 - 0x137 RW */
    volatile u64   context5;                     /**< 0x138 - 0x13f RW */
    volatile u64   context6;                     /**< 0x140 - 0x147 RW */
    _resvd         _reserved3[696];              /**< 0x148 - 0x3ff -- */
    volatile u64   key1;                         /**< 0x400 - 0x407 RW */
    volatile u64   key2;                         /**< 0x408 - 0x40f RW */
    volatile u64   key3;                         /**< 0x410 - 0x417 RW */
    volatile u64   key4;                         /**< 0x418 - 0x41f RW */
    _resvd         _reserved4[992];              /**< 0x420 - 0x7ff -- */
    volatile u8    FIFO[2048];                   /**< 0x800 - 0xfff RW */
} T2EU_KEA;

/** SNOW-3G (STEU) block register view 0x---3d000 - T3.1+ only */
typedef struct _T3EU_SNOW3G {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f -- */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    _resvd         _reserved[8];                 /**< 0x020 - 0x027 -- */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RO */
    volatile u64   interruptControl;             /**< 0x038 - 0x03f RW */
    _resvd         _reserved1[8];                /**< 0x040 - 0x047 -- */
    volatile u64   F9MAC;                        /**< 0x048 - 0x04f R0 */
    volatile u64   eom;                          /**< 0x050 - 0x057 WO */
    _resvd         _reserved2[168];              /**< 0x058 - 0x0ff -- */
    volatile u64   IV1;                          /**< 0x100 - 0x107 RW */
    volatile u64   ICVin;                        /**< 0x108 - 0x10f RW */
    volatile u64   IV2;                          /**< 0x110 - 0x117 RW */
    volatile u64   context1;                     /**< 0x118 - 0x11f RW */
    volatile u64   context2;                     /**< 0x120 - 0x127 RW */
    volatile u64   context3;                     /**< 0x128 - 0x12f RW */
    volatile u64   context4;                     /**< 0x130 - 0x137 RW */
    volatile u64   LFSRstate;                    /**< 0x138 - 0x13f RW */
    _resvd         _reserved3[56];               /**< 0x140 - 0x177 -- */
    volatile u64   FSMstate[2];                  /**< 0x178 - 0x187 RW */
    _resvd         _reserved4[632];              /**< 0x188 - 0x3ff -- */
    volatile u64   keyData[2];                   /**< 0x400 - 0x40f RW */
    _resvd         _reserved5[1008];             /**< 0x410 - 0x7ff -- */
    volatile u8    FIFO[2048];                   /**< 0x800 - 0xfff RW */
} T3EU_SNOW3G;


/** CRC (CRCU) block register view 0x---3f000 - T3 only */
typedef struct _T3EU_CRC {
    volatile u64   mode;                         /**< 0x000 - 0x007 RW */
    volatile u64   keySize;                      /**< 0x008 - 0x00f RW */
    volatile u64   dataSize;                     /**< 0x010 - 0x017 RW */
    volatile u64   resetControl;                 /**< 0x018 - 0x01f RW */
    volatile u64   control;                      /**< 0x020 - 0x027 RW */
    volatile u64   status;                       /**< 0x028 - 0x02f RO */
    volatile u64   interruptStatus;              /**< 0x030 - 0x037 RW */
    volatile u64   interruptMask;                /**< 0x038 - 0x03f RW */
    volatile u64   ICVsize[2];                   /**< 0x040 - 0x04f -- */
    volatile u64   EUgo;                         /**< 0x050 - 0x057 WO */
    _resvd         _reserved[176];               /**< 0x058 - 0x107 -- */
    volatile u32   context;                      /**< 0x108 - 0x10b RW */
    _resvd         _reserved1[756];              /**< 0x10c - 0x3ff -- */
    volatile u64   key;                          /**< 0x400 - 0x407 RW */
    _resvd         _reserved2[1016];             /**< 0x408 - 0x7ff -- */
    volatile u8    FIFO[2048];                   /**< 0x800 - 0xfff WO */
} T3EU_CRC;



/*
 * Top-level view of Talitos, all blocks aggregated into the full core
 * This is typically:
 *
 * 0x0000 - 0x00ff - (reserved)
 * 0x0100 - 0x01ff - Alternate channel 0 - T3 only
 * 0x0200 - 0x02ff - Alternate channel 1 - T3 only
 * 0x0300 - 0x03ff - Alternate channel 2 - T3 only
 * 0x0400 - 0x04ff - Alternate channel 3 - T3 only
 * 0x0500 - 0x05ff - Theoretical alternate channel 4 - T3 only
 * 0x0600 - 0x06ff - Theoretical alternate channel 5 - T3 only
 * 0x0700 - 0x07ff - Theoretical alternate channel 6 - T3 only
 * 0x0800 - 0x08ff - Theoretical alternate channel 7 - T3 only
 * 0x0900 - 0x09ff - Theoretical alternate channel 8 - T3 only
 * 0x0a00 - 0x0aff - Theoretical alternate channel 9 - T3 only
 * 0x1000 - 0x10ff - Controller
 * 0x1100 - 0x11ff - Channel 0
 * 0x1200 - 0x12ff - Channel 1
 * 0x1300 - 0x13ff - Channel 2
 * 0x1400 - 0x14ff - Channel 3
 * 0x1500 - 0x15ff - T3 Polychannel
 * 0x1600 - 0x16ff - (reserved - theoretical channel 5)
 * 0x1700 - 0x17ff - (reserved - theoretical channel 6)
 * 0x1800 - 0x18ff - (reserved - theoretical channel 7)
 * 0x1900 - 0x19ff - (reserved - theoretical channel 8)
 * 0x1a00 - 0x1aff - (reserved - theoretical channel 9)
 * 0x1b00 - 0x1bf7 - (reserved)
 * 0x1bf8 - 0x1bff - IP block revision register
 * 0x1c00 - 0x1fff - (reserved)
 * 0x2000 - 0x2fff - DEU
 * 0x3000 - 0x3fff - (reserved)
 * 0x4000 - 0x4fff - AESU
 * 0x5000 - 0x5fff - (reserved)
 * 0x6000 - 0x6fff - MDEU
 * 0x7000 - 0x7fff - (reserved)
 * 0x8000 - 0x8fff - AFEU
 * 0x9000 - 0x9fff - (reserved)
 * 0xa000 - 0xafff - RNG
 * 0xb000 - 0xbfff - (reserved)
 * 0xc000 - 0xcfff - PKEU
 * 0xd000 - 0xdfff - SNOW3G
 * 0xe000 - 0xefff - KEU
 * 0xf000 - 0xffff - CRCU - T3 only
 */

typedef struct _T2CORE {
    _resvd       _reserved0[256];
    T2CHANNEL    altchn[T3_MAX_CHANNELS]; /**< T3 only, 10 channels only
                                                    theoretical */
    _resvd       _reserved1[1280];
    T2CONTROLLER ctrl;
    T2CHANNEL    chn[4];
    T3POLYCHN    polyCh;
    _resvd       _reserved2[1280];
    T2IPID       ipID;
    _resvd       _reserved3[1024];
    T2EU_DES     euDES;
    _resvd       _reserved4[4096];
    T2EU_AES     euAES;
    _resvd       _reserved5[4096];
    T2EU_MD      euMD;
    _resvd       _reserved6[4096];
    T2EU_ARC4    euARC4;
    _resvd       _reserved7[4096];
    T2EU_RND     euRND;
    _resvd       _reserved8[4096];
    T2EU_PKHA    euPK;
    T3EU_SNOW3G  euSNOW;
    T2EU_KEA     euKEA;
    T3EU_CRC     euCRC;
} T2CORE;



/*
 * Packet descriptor format for any T2 variant
 */

#define TOTAL_PAIRS (7)

#define LAST_ENTRY  (0x02)  /**< linkEntry.chainCtrl
                                    - end of linktable */
#define NEXT_ENTRY  (0x01)  /**< linkEntry.chainCtrl
                                    - point to next table */


/** Describes a single fragment entry in a linktable
    These are referenced by a pointer pair if a virtual buffer is
    "dispersed" (scattered) in memory                             */

typedef struct _linkEntry {
    u16     segLen;     /**< segment length, <= 64k */
    u8      chainCtrl;  /**< chain                  */
    u8      extAddr;    /**< extended address bits  */
    void   *segAddr;    /**< segment address        */
} linkEntry;


/* Describes a pointer/size pair in a packet descriptor */
/* There are 7 of these in a single packet descriptor   */

#define JUMPTABLE   (0x80)  /**< ptrPair[].extent if linktable in use */
#define EXTENT_MASK (0x7f)  /**< ptrPair[].extent without J bit       */
#define EPTR_MASK   (0x0f)  /**< ptrPair[].eptr useful bits           */

typedef struct _ptrPair {
    u16   size;     /**< 64K buffer size                          */
    u8    extent;   /**< J bit (jumptable) plus 128-byte extentq  */
    u8    eptr;     /**< 4-bit pointer extension
                        (36-bit addressing                        */
    void *ptr;
} ptrPair;



/** Basic form of a single packet descriptor */
typedef struct _T2DPD {
    u32       hdr;
    u32       fdbk;
    ptrPair   pair[TOTAL_PAIRS];
} T2DPD;





#ifdef _cplusplus
}
#endif

#endif /* T23_H */

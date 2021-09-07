
/*
 * sec2_sctrMap.c - scatter buffer pointer marker
 *
 * Instantiated into the t23x SEC2.x legacy interface module,.
 * This code is OBSOLETE, and is provided for legacy application
 * transition. New applications should use the _VIRTUAL functions
 * wherever possible
 *
 * Copyright (c) 2004-2007 Freescale Semiconductor, Inc.
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

#include "Sec2.h"
#include "Sec2local.h"

typedef struct _scatter_assoc
{
    unsigned long offset;
    unsigned long pos;
} SCATTER_ASSOC;





static const SCATTER_ASSOC SCTMAP_RNG[] =
{
    {offsetof(RNG_REQ, rngData), 0x10},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_DES_CBC_CTX[] =
{
    {offsetof(DES_CBC_CRYPT_REQ, inIvData),  0x02},
    {offsetof(DES_CBC_CRYPT_REQ, keyData),   0x04},
    {offsetof(DES_CBC_CRYPT_REQ, inData),    0x08},
    {offsetof(DES_CBC_CRYPT_REQ, outData),   0x10},
    {offsetof(DES_CBC_CRYPT_REQ, outIvData), 0x20},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_DES_ECB[] =
{
    {offsetof(DES_CRYPT_REQ, keyData), 0x04},
    {offsetof(DES_CRYPT_REQ, inData),  0x08},
    {offsetof(DES_CRYPT_REQ, outData), 0x10},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_RC4_LDCTX_CRYPT_ULCTX[] =
{
    {offsetof(ARC4_LOADCTX_CRYPT_REQ, inCtxData),   0x02},
    {offsetof(ARC4_LOADCTX_CRYPT_REQ, inData),      0x08},
    {offsetof(ARC4_LOADCTX_CRYPT_REQ, outData),     0x10},
    {offsetof(ARC4_LOADCTX_CRYPT_REQ, outCtxData),  0x20},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_RC4_LDKEY_CRYPT_ULCTX[] =
{
    {offsetof(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, keyData),     0x04},
    {offsetof(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, inData),      0x08},
    {offsetof(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, outData),     0x10},
    {offsetof(ARC4_LOADKEY_CRYPT_UNLOADCTX_REQ, outCtxData),  0x20},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_HASH_LDCTX_HASH_ULCTX[] =
{
    {offsetof(HASH_REQ, ctxData),  0x02},
    {offsetof(HASH_REQ, inData),   0x08},
    {offsetof(HASH_REQ, cmpData),  0x10},
    {offsetof(HASH_REQ, outData),  0x20},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_HASH_LDCTX_HASH_PAD_ULCTX[] =
{
    {offsetof(HASH_REQ, ctxData),  0x02},
    {offsetof(HASH_REQ, inData),   0x08},
    {offsetof(HASH_REQ, cmpData),  0x10},
    {offsetof(HASH_REQ, outData),  0x20},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_HASH_LDCTX_HMAC_ULCTX[] =
{
    {offsetof(HMAC_PAD_REQ, keyData), 0x04},
    {offsetof(HMAC_PAD_REQ, inData),  0x08},
    {offsetof(HMAC_PAD_REQ, cmpData), 0x10},
    {offsetof(HMAC_PAD_REQ, outData), 0x20},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_MM_LDCTX_EXP_ULCTX[] =
{
    {offsetof(MOD_EXP_REQ, aData),   0x02},
    {offsetof(MOD_EXP_REQ, expData), 0x04},
    {offsetof(MOD_EXP_REQ, modData), 0x08},
    {offsetof(MOD_EXP_REQ, outData), 0x10},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_MM_SS_RSA_EXP[] =
{
    {offsetof(MOD_SS_EXP_REQ, modData), 0x01},
    {offsetof(MOD_SS_EXP_REQ, aData),   0x04},
    {offsetof(MOD_SS_EXP_REQ, expData), 0x08},
    {offsetof(MOD_SS_EXP_REQ, bData),   0x10},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_MM_LDCTX_R2MODN_ULCTX[] =
{
    {offsetof(MOD_R2MODN_REQ, modData), 0x08},
    {offsetof(MOD_R2MODN_REQ, outData), 0x10},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_MM_LDCTX_RRMODP_ULCTX[] =
{
    {offsetof(MOD_RRMODP_REQ, pData),   0x01},
    {offsetof(MOD_RRMODP_REQ, nBytes),  0x08},
    {offsetof(MOD_RRMODP_REQ, outData), 0x10},
    {0, 0}
};


static const SCATTER_ASSOC SCTMAP_MM_MOD_INV_ULCTX[] =
{
    {offsetof(MOD_INV_REQ, nData),   0x01},
    {offsetof(MOD_INV_REQ, inData),  0x04},
    {offsetof(MOD_INV_REQ, outData), 0x10},
    {0, 0}
};


static const SCATTER_ASSOC SCTMAP_MOD_LDCTX_2OP_ULCTX[] =
{
    {offsetof(MOD_2OP_REQ, modData), 0x01},
    {offsetof(MOD_2OP_REQ, bData),   0x02},
    {offsetof(MOD_2OP_REQ, aData),   0x04},
    {offsetof(MOD_2OP_REQ, outData), 0x10},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_EC_LDCTX_kP_ULCTX[] =
{
    {offsetof(ECC_POINT_REQ, nData),     0x01},
    {offsetof(ECC_POINT_REQ, eData),     0x02},
    {offsetof(ECC_POINT_REQ, buildData), 0x04},
    {offsetof(ECC_POINT_REQ, b1Data),    0x08},
    {offsetof(ECC_POINT_REQ, b2Data),    0x10},
    {offsetof(ECC_POINT_REQ, b3Data),    0x20},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_EC_2OP[] =
{
    {offsetof(ECC_2OP_REQ, bData),   0x01},
    {offsetof(ECC_2OP_REQ, aData),   0x02},
    {offsetof(ECC_2OP_REQ, modData), 0x04},
    {offsetof(ECC_2OP_REQ, outData), 0x08},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_EC_SPKBUILD_ULCTX[] =
{
    {offsetof(ECC_SPKBUILD_REQ, a0Data),    0x01},
    {offsetof(ECC_SPKBUILD_REQ, a1Data),    0x02},
    {offsetof(ECC_SPKBUILD_REQ, a2Data),    0x04},
    {offsetof(ECC_SPKBUILD_REQ, a3Data),    0x08},
    {offsetof(ECC_SPKBUILD_REQ, b0Data),    0x10},
    {offsetof(ECC_SPKBUILD_REQ, b1Data),    0x20},
    {offsetof(ECC_SPKBUILD_REQ, buildData), 0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_ECC_PTADD_DBL[] =
{
    {offsetof(ECC_PTADD_DBL_REQ, modData),    0x01},
    {offsetof(ECC_PTADD_DBL_REQ, buildData),  0x02},
    {offsetof(ECC_PTADD_DBL_REQ, b2InData),   0x04},
    {offsetof(ECC_PTADD_DBL_REQ, b3InData),   0x08},
    {offsetof(ECC_PTADD_DBL_REQ, b1Data),     0x10},
    {offsetof(ECC_PTADD_DBL_REQ, b2Data),     0x20},
    {offsetof(ECC_PTADD_DBL_REQ, b3Data),     0x40},
};



static const SCATTER_ASSOC SCTMAP_IPSEC_CBC[] =
{
    {offsetof(IPSEC_CBC_REQ, hashKeyData),    0x01},
    {offsetof(IPSEC_CBC_REQ, hashInData),     0x02},
    {offsetof(IPSEC_CBC_REQ, cryptKeyData),   0x04},
    {offsetof(IPSEC_CBC_REQ, cryptCtxInData), 0x08},
    {offsetof(IPSEC_CBC_REQ, inData),         0x10},
    {offsetof(IPSEC_CBC_REQ, cryptDataOut),   0x20},
    {offsetof(IPSEC_CBC_REQ, hashDataOut),    0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_IPSEC_ESP[] =
{
    {offsetof(IPSEC_ESP_REQ, hashKeyData),     0x01},
    {offsetof(IPSEC_ESP_REQ, hashInData),      0x02},
    {offsetof(IPSEC_ESP_REQ, cryptCtxInData),  0x04},
    {offsetof(IPSEC_ESP_REQ, cryptKeyData),    0x08},
    {offsetof(IPSEC_ESP_REQ, inData),          0x10},
    {offsetof(IPSEC_ESP_REQ, cryptDataOut),    0x20},
    {offsetof(IPSEC_ESP_REQ, cryptCtxOutData), 0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_IPSEC_STATIC_CBC[] =
{
    {offsetof(IPSEC_CBC_REQ, hashKeyData),    0x01},
    {offsetof(IPSEC_CBC_REQ, hashInData),     0x02},
    {offsetof(IPSEC_CBC_REQ, cryptKeyData),   0x04},
    {offsetof(IPSEC_CBC_REQ, cryptCtxInData), 0x08},
    {offsetof(IPSEC_CBC_REQ, inData),         0x10},
    {offsetof(IPSEC_CBC_REQ, cryptDataOut),   0x20},
    {offsetof(IPSEC_CBC_REQ, hashDataOut),    0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_IPSEC_ECB[] =
{
    {offsetof(IPSEC_ECB_REQ, hashKeyData),  0x01},
    {offsetof(IPSEC_ECB_REQ, hashInData),   0x02},
    {offsetof(IPSEC_ECB_REQ, cryptKeyData), 0x04},
    {offsetof(IPSEC_ECB_REQ, inData),       0x10},
    {offsetof(IPSEC_ECB_REQ, cryptDataOut), 0x20},
    {offsetof(IPSEC_ECB_REQ, hashDataOut),  0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_IPSEC_AES_CBC[] =
{
    {offsetof(IPSEC_AES_CBC_REQ, hashKeyData),    0x01},
    {offsetof(IPSEC_AES_CBC_REQ, hashInData),     0x02},
    {offsetof(IPSEC_AES_CBC_REQ, cryptKeyData),   0x04},
    {offsetof(IPSEC_AES_CBC_REQ, cryptCtxInData), 0x08},
    {offsetof(IPSEC_AES_CBC_REQ, inData),         0x10},
    {offsetof(IPSEC_AES_CBC_REQ, cryptDataOut),   0x20},
    {offsetof(IPSEC_AES_CBC_REQ, hashDataOut),    0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_IPSEC_AES_ECB[] =
{
    {offsetof(IPSEC_AES_ECB_REQ, hashKeyData),  0x01},
    {offsetof(IPSEC_AES_ECB_REQ, hashInData),   0x02},
    {offsetof(IPSEC_AES_ECB_REQ, cryptKeyData), 0x04},
    {offsetof(IPSEC_AES_ECB_REQ, inData),       0x10},
    {offsetof(IPSEC_AES_ECB_REQ, cryptDataOut), 0x20},
    {offsetof(IPSEC_AES_ECB_REQ, hashDataOut),  0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_AESA_CRYPT[] =
{
    {offsetof(AESA_CRYPT_REQ, inIvData),   0x02},
    {offsetof(AESA_CRYPT_REQ, keyData),    0x04},
    {offsetof(AESA_CRYPT_REQ, inData),     0x08},
    {offsetof(AESA_CRYPT_REQ, outData),    0x10},
    {offsetof(AESA_CRYPT_REQ, outCtxData), 0x20},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_CCMP[] =
{
    {offsetof(CCMP_REQ, context),      0x02},
    {offsetof(CCMP_REQ, keyData),      0x04},
    {offsetof(CCMP_REQ, AADData),      0x08},
    {offsetof(CCMP_REQ, FrameData),    0x10},
    {offsetof(CCMP_REQ, cryptDataOut), 0x20},
    {offsetof(CCMP_REQ, MICData),      0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_SRTP[] =
{
    {offsetof(SRTP_REQ, hashKeyData),  0x01},
    {offsetof(SRTP_REQ, ivData),       0x02},
    {offsetof(SRTP_REQ, keyData),      0x04},
    {offsetof(SRTP_REQ, inData),       0x08},
    {offsetof(SRTP_REQ, cryptDataOut), 0x10},
    {offsetof(SRTP_REQ, digestData),   0x20},
    {offsetof(SRTP_REQ, outIvData),    0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_KEA_CRYPT[] =
{
    {offsetof(KEA_CRYPT_REQ, ivData),   0x02},
    {offsetof(KEA_CRYPT_REQ, keyData),  0x04},
    {offsetof(KEA_CRYPT_REQ, inData),   0x08},
    {offsetof(KEA_CRYPT_REQ, outData),  0x10},
    {offsetof(KEA_CRYPT_REQ, ctxData),  0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_RAID_XOR[] =
{
    {offsetof(RAID_XOR_REQ, inDataA),  0x08},
    {offsetof(RAID_XOR_REQ, inDataB),  0x10},
    {offsetof(RAID_XOR_REQ, inDataC),  0x20},
    {offsetof(RAID_XOR_REQ, outData),  0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_TLS_BLOCK_INBOUND[] =
{
    {offsetof(TLS_BLOCK_INBOUND_REQ, hashKeyData),   0x01},
    {offsetof(TLS_BLOCK_INBOUND_REQ, hashOnlyData),  0x02},
    {offsetof(TLS_BLOCK_INBOUND_REQ, ivData),        0x04},
    {offsetof(TLS_BLOCK_INBOUND_REQ, cipherKeyData), 0x08},
    {offsetof(TLS_BLOCK_INBOUND_REQ, inData),        0x10},
    {offsetof(TLS_BLOCK_INBOUND_REQ, outData),       0x20},
    {offsetof(TLS_BLOCK_INBOUND_REQ, ivOutData),     0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_TLS_BLOCK_OUTBOUND[] =
{
    {offsetof(TLS_BLOCK_OUTBOUND_REQ, hashKeyData),    0x01},
    {offsetof(TLS_BLOCK_OUTBOUND_REQ, ivData),         0x02},
    {offsetof(TLS_BLOCK_OUTBOUND_REQ, cipherKeyData),  0x04},
    {offsetof(TLS_BLOCK_OUTBOUND_REQ, hashOnlyData),   0x08},
    {offsetof(TLS_BLOCK_OUTBOUND_REQ, cipherOnlyData), 0x10},
    {offsetof(TLS_BLOCK_OUTBOUND_REQ, outData),        0x20},
    {offsetof(TLS_BLOCK_OUTBOUND_REQ, ivOutData),      0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_TLS_STREAM_INBOUND[] =
{
    {offsetof(TLS_STREAM_INBOUND_REQ, hashKeyData),   0x01},
    {offsetof(TLS_STREAM_INBOUND_REQ, hashOnlyData),  0x02},
    {offsetof(TLS_STREAM_INBOUND_REQ, ivData),        0x04},
    {offsetof(TLS_STREAM_INBOUND_REQ, cipherKeyData), 0x08},
    {offsetof(TLS_STREAM_INBOUND_REQ, inData),        0x10},
    {offsetof(TLS_STREAM_INBOUND_REQ, outData),       0x20},
    {offsetof(TLS_STREAM_INBOUND_REQ, ivOutData),     0x40},
    {0, 0}
};



static const SCATTER_ASSOC SCTMAP_TLS_STREAM_OUTBOUND[] =
{
    {offsetof(TLS_STREAM_OUTBOUND_REQ, hashKeyData),   0x01},
    {offsetof(TLS_STREAM_OUTBOUND_REQ, ivData),        0x02},
    {offsetof(TLS_STREAM_OUTBOUND_REQ, cipherKeyData), 0x04},
    {offsetof(TLS_STREAM_OUTBOUND_REQ, hashOnlyData),  0x08},
    {offsetof(TLS_STREAM_OUTBOUND_REQ, outData),       0x10},
    {offsetof(TLS_STREAM_OUTBOUND_REQ, ivOutData),     0x40},
    {0, 0}
};




static unsigned char locate(SCATTER_ASSOC *assoc,
                            unsigned long offset)
{
  int i = 0;

  while(assoc[i].offset)
    if (assoc[i].offset == offset)
      return(assoc[i].pos);
    else
      i++;

  return(0);
}

/* Locate a pointer field within a request struct, look it up, and mark the */
/* scatter buffer mask if it is such */

int MarkScatterBuffer(void *request, void *buffer)
{
  GENERIC_REQ *rq;
  unsigned long offs;

  rq = (GENERIC_REQ *)request;
  offs = (unsigned long)buffer - (unsigned long)request;

  switch (rq->opId & DESC_TYPE_MASK)
  {
    case DPD_RNG_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_RNG, offs);
      break;

    case DPD_DES_CBC_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_DES_CBC_CTX, offs);
      break;

    case DPD_DES_ECB_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_DES_ECB, offs);
      break;

    case DPD_RC4_LDCTX_CRYPT_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_RC4_LDCTX_CRYPT_ULCTX, offs);
      break;

    case DPD_RC4_LDKEY_CRYPT_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_RC4_LDKEY_CRYPT_ULCTX, offs);
      break;

    case DPD_HASH_LDCTX_HASH_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_HASH_LDCTX_HASH_ULCTX, offs);
      break;

    case DPD_HASH_LDCTX_HASH_PAD_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_HASH_LDCTX_HASH_PAD_ULCTX, offs);
      break;

    case DPD_HASH_LDCTX_HMAC_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_HASH_LDCTX_HMAC_ULCTX, offs);
      break;

    case DPD_MM_LDCTX_EXP_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_MM_LDCTX_EXP_ULCTX, offs);
      break;

    case DPD_MM_SS_EXP_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_MM_SS_RSA_EXP, offs);
      break;

    case DPD_MM_LDCTX_R2MODN_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_MM_LDCTX_R2MODN_ULCTX, offs);
      break;

    case DPD_MM_LDCTX_RRMODP_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_MM_LDCTX_RRMODP_ULCTX, offs);
      break;

    case DPD_MM_MOD_INV_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_MM_MOD_INV_ULCTX, offs);
      break;

    case DPD_MOD_LDCTX_2OP_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_MOD_LDCTX_2OP_ULCTX, offs);
      break;

    case DPD_EC_LDCTX_kP_ULCTX_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_EC_LDCTX_kP_ULCTX, offs);
      break;

    case DPD_EC_2OP_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_EC_2OP, offs);
      break;

    case DPD_IPSEC_CBC_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_IPSEC_CBC, offs);
      break;

    case DPD_IPSEC_ECB_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_IPSEC_ECB, offs);
      break;

    case DPD_IPSEC_AES_CBC_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_IPSEC_AES_CBC, offs);
      break;

    case DPD_IPSEC_AES_ECB_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_IPSEC_AES_ECB, offs);
      break;

    case DPD_AESA_CRYPT_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_AESA_CRYPT, offs);
      break;

    case DPD_IPSEC_ESP_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_IPSEC_ESP, offs);
      break;

    case DPD_CCMP_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_CCMP, offs);
      break;

    case DPD_SRTP_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_SRTP, offs);
      break;

    case DPD_RAID_XOR_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_RAID_XOR, offs);
      break;

    case DPD_KEA_CRYPT_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_KEA_CRYPT, offs);
      break;

    case DPD_EC_SPKBUILD_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_EC_SPKBUILD_ULCTX, offs);
      break;

    case DPD_EC_PTADD_DBL_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_ECC_PTADD_DBL, offs);
      break;

    case DPD_TLS_BLOCK_INBOUND_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_TLS_BLOCK_INBOUND, offs);
      break;

    case DPD_TLS_BLOCK_OUTBOUND_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_TLS_BLOCK_OUTBOUND, offs);
      break;

    case DPD_TLS_STREAM_INBOUND_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_TLS_STREAM_INBOUND, offs);
      break;

    case DPD_TLS_STREAM_OUTBOUND_GROUP:
      rq->scatterBufs |= locate((SCATTER_ASSOC *)SCTMAP_TLS_STREAM_OUTBOUND, offs);
      break;

    default:
      return -1;
  }

  return 0;
}

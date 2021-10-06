/*
 * Cipher algorithms supported by the CESA: DES, 3DES and AES.
 *
 * Author: Boris Brezillon <boris.brezillon@free-electrons.com>
 * Author: Arnaud Ebalard <arno@natisbad.org>
 *
 * This work is based on an initial version written by
 * Sebastian Andrzej Siewior < sebastian at breakpoint dot cc >
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <crypto/aes.h>
#include <crypto/des.h>

#include "cesa.h"

struct mv_cesa_des_ctx {
	struct mv_cesa_ctx base;
	u8 key[DES_KEY_SIZE];
};

struct mv_cesa_des3_ctx {
	struct mv_cesa_ctx base;
	u8 key[DES3_EDE_KEY_SIZE];
};

struct mv_cesa_aes_ctx {
	struct mv_cesa_ctx base;
	struct crypto_aes_ctx aes;
};

struct mv_cesa_ablkcipher_dma_iter {
	struct mv_cesa_dma_iter base;
	struct mv_cesa_sg_dma_iter src;
	struct mv_cesa_sg_dma_iter dst;
};

static inline void
mv_cesa_ablkcipher_req_iter_init(struct mv_cesa_ablkcipher_dma_iter *iter,
				 struct ablkcipher_request *req)
{
	mv_cesa_req_dma_iter_init(&iter->base, req->nbytes);
	mv_cesa_sg_dma_iter_init(&iter->src, req->src, DMA_TO_DEVICE);
	mv_cesa_sg_dma_iter_init(&iter->dst, req->dst, DMA_FROM_DEVICE);
}

static inline bool
mv_cesa_ablkcipher_req_iter_next_op(struct mv_cesa_ablkcipher_dma_iter *iter)
{
	iter->src.op_offset = 0;
	iter->dst.op_offset = 0;

	return mv_cesa_req_dma_iter_next_op(&iter->base);
}

static inline void
mv_cesa_ablkcipher_dma_cleanup(struct ablkcipher_request *req)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);

	if (req->dst != req->src) {
		dma_unmap_sg(cesa_dev->dev, req->dst, creq->dst_nents,
			     DMA_FROM_DEVICE);
		dma_unmap_sg(cesa_dev->dev, req->src, creq->src_nents,
			     DMA_TO_DEVICE);
	} else {
		dma_unmap_sg(cesa_dev->dev, req->src, creq->src_nents,
			     DMA_BIDIRECTIONAL);
	}
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	mv_cesa_dma_cleanup(&creq->base);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	mv_cesa_dma_cleanup(&creq->req.dma);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

static inline void mv_cesa_ablkcipher_cleanup(struct ablkcipher_request *req)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	if (mv_cesa_req_get_type(&creq->base) == CESA_DMA_REQ)
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (creq->req.base.type == CESA_DMA_REQ)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		mv_cesa_ablkcipher_dma_cleanup(req);
}

static void mv_cesa_ablkcipher_std_step(struct ablkcipher_request *req)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->std;
	struct mv_cesa_engine *engine = creq->base.engine;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->req.std;
	struct mv_cesa_engine *engine = sreq->base.engine;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	size_t  len = min_t(size_t, req->nbytes - sreq->offset,
			    CESA_SA_SRAM_PAYLOAD_SIZE);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	mv_cesa_adjust_op(engine, &sreq->op);
	memcpy_toio(engine->sram, &sreq->op, sizeof(sreq->op));

#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	len = sg_pcopy_to_buffer(req->src, creq->src_nents,
				 engine->sram + CESA_SA_DATA_SRAM_OFFSET,
				 len, sreq->offset);

	sreq->size = len;
	mv_cesa_set_crypt_op_len(&sreq->op, len);

	/* FIXME: only update enc_len field */
	if (!sreq->skip_ctx) {
		memcpy_toio(engine->sram, &sreq->op, sizeof(sreq->op));
		sreq->skip_ctx = true;
	} else {
		memcpy_toio(engine->sram, &sreq->op, sizeof(sreq->op.desc));
	}

	mv_cesa_set_int_mask(engine, CESA_SA_INT_ACCEL0_DONE);
	writel_relaxed(CESA_SA_CFG_PARA_DIS, engine->regs + CESA_SA_CFG);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	BUG_ON(readl(engine->regs + CESA_SA_CMD) &
	       CESA_SA_CMD_EN_CESA_SA_ACCL0);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	writel(CESA_SA_CMD_EN_CESA_SA_ACCL0, engine->regs + CESA_SA_CMD);
}

static int mv_cesa_ablkcipher_std_process(struct ablkcipher_request *req,
					  u32 status)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->std;
	struct mv_cesa_engine *engine = creq->base.engine;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->req.std;
	struct mv_cesa_engine *engine = sreq->base.engine;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	size_t len;

	len = sg_pcopy_from_buffer(req->dst, creq->dst_nents,
				   engine->sram + CESA_SA_DATA_SRAM_OFFSET,
				   sreq->size, sreq->offset);

	sreq->offset += len;
	if (sreq->offset < req->nbytes)
		return -EINPROGRESS;

	return 0;
}

static int mv_cesa_ablkcipher_process(struct crypto_async_request *req,
				      u32 status)
{
	struct ablkcipher_request *ablkreq = ablkcipher_request_cast(req);
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(ablkreq);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct mv_cesa_req *basereq = &creq->base;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->req.std;
	struct mv_cesa_engine *engine = sreq->base.engine;
	int ret;

	if (creq->req.base.type == CESA_DMA_REQ)
		ret = mv_cesa_dma_process(&creq->req.dma, status);
	else
		ret = mv_cesa_ablkcipher_std_process(ablkreq, status);

	if (ret)
		return ret;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	if (mv_cesa_req_get_type(basereq) == CESA_STD_REQ)
		return mv_cesa_ablkcipher_std_process(ablkreq, status);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	memcpy_fromio(ablkreq->info,
		      engine->sram + CESA_SA_CRYPT_IV_SRAM_OFFSET,
		      crypto_ablkcipher_ivsize(crypto_ablkcipher_reqtfm(ablkreq)));
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	return mv_cesa_dma_process(basereq, status);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	return 0;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

static void mv_cesa_ablkcipher_step(struct crypto_async_request *req)
{
	struct ablkcipher_request *ablkreq = ablkcipher_request_cast(req);
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(ablkreq);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	if (mv_cesa_req_get_type(&creq->base) == CESA_DMA_REQ)
		mv_cesa_dma_step(&creq->base);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (creq->req.base.type == CESA_DMA_REQ)
		mv_cesa_dma_step(&creq->req.dma);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	else
		mv_cesa_ablkcipher_std_step(ablkreq);
}

static inline void
mv_cesa_ablkcipher_dma_prepare(struct ablkcipher_request *req)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct mv_cesa_req *basereq = &creq->base;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_tdma_req *dreq = &creq->req.dma;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	mv_cesa_dma_prepare(basereq, basereq->engine);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	mv_cesa_dma_prepare(dreq, dreq->base.engine);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

static inline void
mv_cesa_ablkcipher_std_prepare(struct ablkcipher_request *req)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->std;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->req.std;
	struct mv_cesa_engine *engine = sreq->base.engine;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	sreq->size = 0;
	sreq->offset = 0;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	mv_cesa_adjust_op(engine, &sreq->op);
	memcpy_toio(engine->sram, &sreq->op, sizeof(sreq->op));
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

static inline void mv_cesa_ablkcipher_prepare(struct crypto_async_request *req,
					      struct mv_cesa_engine *engine)
{
	struct ablkcipher_request *ablkreq = ablkcipher_request_cast(req);
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(ablkreq);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	creq->base.engine = engine;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	creq->req.base.engine = engine;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	if (mv_cesa_req_get_type(&creq->base) == CESA_DMA_REQ)
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (creq->req.base.type == CESA_DMA_REQ)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		mv_cesa_ablkcipher_dma_prepare(ablkreq);
	else
		mv_cesa_ablkcipher_std_prepare(ablkreq);
}

static inline void
mv_cesa_ablkcipher_req_cleanup(struct crypto_async_request *req)
{
	struct ablkcipher_request *ablkreq = ablkcipher_request_cast(req);

	mv_cesa_ablkcipher_cleanup(ablkreq);
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
static void
mv_cesa_ablkcipher_complete(struct crypto_async_request *req)
{
	struct ablkcipher_request *ablkreq = ablkcipher_request_cast(req);
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(ablkreq);
	struct mv_cesa_engine *engine = creq->base.engine;
	unsigned int ivsize;

	atomic_sub(ablkreq->nbytes, &engine->load);
	ivsize = crypto_ablkcipher_ivsize(crypto_ablkcipher_reqtfm(ablkreq));

	if (mv_cesa_req_get_type(&creq->base) == CESA_DMA_REQ) {
		struct mv_cesa_req *basereq;

		basereq = &creq->base;
		memcpy(ablkreq->info, basereq->chain.last->op->ctx.blkcipher.iv,
		       ivsize);
	} else {
		memcpy_fromio(ablkreq->info,
			      engine->sram + CESA_SA_CRYPT_IV_SRAM_OFFSET,
			      ivsize);
	}
}

#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static const struct mv_cesa_req_ops mv_cesa_ablkcipher_req_ops = {
	.step = mv_cesa_ablkcipher_step,
	.process = mv_cesa_ablkcipher_process,
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	.prepare = mv_cesa_ablkcipher_prepare,
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	.cleanup = mv_cesa_ablkcipher_req_cleanup,
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	.complete = mv_cesa_ablkcipher_complete,
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
};

static int mv_cesa_ablkcipher_cra_init(struct crypto_tfm *tfm)
{
	struct mv_cesa_aes_ctx *ctx = crypto_tfm_ctx(tfm);

	ctx->base.ops = &mv_cesa_ablkcipher_req_ops;

	tfm->crt_ablkcipher.reqsize = sizeof(struct mv_cesa_ablkcipher_req);

	return 0;
}

static int mv_cesa_aes_setkey(struct crypto_ablkcipher *cipher, const u8 *key,
			      unsigned int len)
{
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(cipher);
	struct mv_cesa_aes_ctx *ctx = crypto_tfm_ctx(tfm);
	int remaining;
	int offset;
	int ret;
	int i;

	ret = crypto_aes_expand_key(&ctx->aes, key, len);
	if (ret) {
		crypto_ablkcipher_set_flags(cipher, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return ret;
	}

	remaining = (ctx->aes.key_length - 16) / 4;
	offset = ctx->aes.key_length + 24 - remaining;
	for (i = 0; i < remaining; i++)
		ctx->aes.key_dec[4 + i] =
			cpu_to_le32(ctx->aes.key_enc[offset + i]);

	return 0;
}

static int mv_cesa_des_setkey(struct crypto_ablkcipher *cipher, const u8 *key,
			      unsigned int len)
{
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(cipher);
	struct mv_cesa_des_ctx *ctx = crypto_tfm_ctx(tfm);
	u32 tmp[DES_EXPKEY_WORDS];
	int ret;

	if (len != DES_KEY_SIZE) {
		crypto_ablkcipher_set_flags(cipher, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	ret = des_ekey(tmp, key);
	if (!ret && (tfm->crt_flags & CRYPTO_TFM_REQ_WEAK_KEY)) {
		tfm->crt_flags |= CRYPTO_TFM_RES_WEAK_KEY;
		return -EINVAL;
	}

	memcpy(ctx->key, key, DES_KEY_SIZE);

	return 0;
}

static int mv_cesa_des3_ede_setkey(struct crypto_ablkcipher *cipher,
				   const u8 *key, unsigned int len)
{
	struct crypto_tfm *tfm = crypto_ablkcipher_tfm(cipher);
	struct mv_cesa_des_ctx *ctx = crypto_tfm_ctx(tfm);

	if (len != DES3_EDE_KEY_SIZE) {
		crypto_ablkcipher_set_flags(cipher, CRYPTO_TFM_RES_BAD_KEY_LEN);
		return -EINVAL;
	}

	memcpy(ctx->key, key, DES3_EDE_KEY_SIZE);

	return 0;
}

static int mv_cesa_ablkcipher_dma_req_init(struct ablkcipher_request *req,
				const struct mv_cesa_op_ctx *op_templ)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);
	gfp_t flags = (req->base.flags & CRYPTO_TFM_REQ_MAY_SLEEP) ?
		      GFP_KERNEL : GFP_ATOMIC;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct mv_cesa_req *basereq = &creq->base;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_tdma_req *dreq = &creq->req.dma;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_ablkcipher_dma_iter iter;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_tdma_chain chain;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	bool skip_ctx = false;
	int ret;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	unsigned int ivsize;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	basereq->chain.first = NULL;
	basereq->chain.last = NULL;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	dreq->base.type = CESA_DMA_REQ;
	dreq->chain.first = NULL;
	dreq->chain.last = NULL;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	if (req->src != req->dst) {
		ret = dma_map_sg(cesa_dev->dev, req->src, creq->src_nents,
				 DMA_TO_DEVICE);
		if (!ret)
			return -ENOMEM;

		ret = dma_map_sg(cesa_dev->dev, req->dst, creq->dst_nents,
				 DMA_FROM_DEVICE);
		if (!ret) {
			ret = -ENOMEM;
			goto err_unmap_src;
		}
	} else {
		ret = dma_map_sg(cesa_dev->dev, req->src, creq->src_nents,
				 DMA_BIDIRECTIONAL);
		if (!ret)
			return -ENOMEM;
	}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	mv_cesa_tdma_desc_iter_init(&basereq->chain);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	mv_cesa_tdma_desc_iter_init(&chain);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	mv_cesa_ablkcipher_req_iter_init(&iter, req);

	do {
		struct mv_cesa_op_ctx *op;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		op = mv_cesa_dma_add_op(&basereq->chain, op_templ, skip_ctx, flags);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		op = mv_cesa_dma_add_op(&chain, op_templ, skip_ctx, flags);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		if (IS_ERR(op)) {
			ret = PTR_ERR(op);
			goto err_free_tdma;
		}
		skip_ctx = true;

		mv_cesa_set_crypt_op_len(op, iter.base.op_len);

		/* Add input transfers */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		ret = mv_cesa_dma_add_op_transfers(&basereq->chain, &iter.base,
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		ret = mv_cesa_dma_add_op_transfers(&chain, &iter.base,
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
						   &iter.src, flags);
		if (ret)
			goto err_free_tdma;

		/* Add dummy desc to launch the crypto operation */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		ret = mv_cesa_dma_add_dummy_launch(&basereq->chain, flags);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		ret = mv_cesa_dma_add_dummy_launch(&chain, flags);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		if (ret)
			goto err_free_tdma;

		/* Add output transfers */
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
		ret = mv_cesa_dma_add_op_transfers(&basereq->chain, &iter.base,
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
		ret = mv_cesa_dma_add_op_transfers(&chain, &iter.base,
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
						   &iter.dst, flags);
		if (ret)
			goto err_free_tdma;

	} while (mv_cesa_ablkcipher_req_iter_next_op(&iter));

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	/* Add output data for IV */
	ivsize = crypto_ablkcipher_ivsize(crypto_ablkcipher_reqtfm(req));
	ret = mv_cesa_dma_add_result_op(&basereq->chain, CESA_SA_CFG_SRAM_OFFSET,
				    CESA_SA_DATA_SRAM_OFFSET,
				    CESA_TDMA_SRC_IN_SRAM, flags);

	if (ret)
		goto err_free_tdma;

	basereq->chain.last->flags |= CESA_TDMA_END_OF_REQ;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	dreq->chain = chain;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	return 0;

err_free_tdma:
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	mv_cesa_dma_cleanup(basereq);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	mv_cesa_dma_cleanup(dreq);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (req->dst != req->src)
		dma_unmap_sg(cesa_dev->dev, req->dst, creq->dst_nents,
			     DMA_FROM_DEVICE);

err_unmap_src:
	dma_unmap_sg(cesa_dev->dev, req->src, creq->src_nents,
		     req->dst != req->src ? DMA_TO_DEVICE : DMA_BIDIRECTIONAL);

	return ret;
}

static inline int
mv_cesa_ablkcipher_std_req_init(struct ablkcipher_request *req,
				const struct mv_cesa_op_ctx *op_templ)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->std;
	struct mv_cesa_req *basereq = &creq->base;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_ablkcipher_std_req *sreq = &creq->req.std;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	sreq->base.type = CESA_STD_REQ;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	sreq->op = *op_templ;
	sreq->skip_ctx = false;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	basereq->chain.first = NULL;
	basereq->chain.last = NULL;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	return 0;
}

static int mv_cesa_ablkcipher_req_init(struct ablkcipher_request *req,
				       struct mv_cesa_op_ctx *tmpl)
{
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);
	struct crypto_ablkcipher *tfm = crypto_ablkcipher_reqtfm(req);
	unsigned int blksize = crypto_ablkcipher_blocksize(tfm);
	int ret;

	if (!IS_ALIGNED(req->nbytes, blksize))
		return -EINVAL;

	creq->src_nents = sg_nents_for_len(req->src, req->nbytes);
#if defined(CONFIG_SYNO_LSP_ARMADA_16_12)
	if (creq->src_nents < 0) {
		dev_err(cesa_dev->dev, "Invalid number of src SG");
		return creq->src_nents;
	}
#endif /* CONFIG_SYNO_LSP_ARMADA_16_12 */
	creq->dst_nents = sg_nents_for_len(req->dst, req->nbytes);
#if defined(CONFIG_SYNO_LSP_ARMADA_16_12)
	if (creq->dst_nents < 0) {
		dev_err(cesa_dev->dev, "Invalid number of dst SG");
		return creq->dst_nents;
	}
#endif /* CONFIG_SYNO_LSP_ARMADA_16_12 */

	mv_cesa_update_op_cfg(tmpl, CESA_SA_DESC_CFG_OP_CRYPT_ONLY,
			      CESA_SA_DESC_CFG_OP_MSK);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	/* TODO: add a threshold for DMA usage */
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (cesa_dev->caps->has_tdma)
		ret = mv_cesa_ablkcipher_dma_req_init(req, tmpl);
	else
		ret = mv_cesa_ablkcipher_std_req_init(req, tmpl);

	return ret;
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
static int mv_cesa_ablkcipher_queue_req(struct ablkcipher_request *req,
					struct mv_cesa_op_ctx *tmpl)
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static int mv_cesa_des_op(struct ablkcipher_request *req,
			  struct mv_cesa_op_ctx *tmpl)
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
{
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	struct mv_cesa_des_ctx *ctx = crypto_tfm_ctx(req->base.tfm);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	int ret;
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	struct mv_cesa_ablkcipher_req *creq = ablkcipher_request_ctx(req);
	struct mv_cesa_engine *engine;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	mv_cesa_update_op_cfg(tmpl, CESA_SA_DESC_CFG_CRYPTM_DES,
			      CESA_SA_DESC_CFG_CRYPTM_MSK);

	memcpy(tmpl->ctx.blkcipher.key, ctx->key, DES_KEY_SIZE);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	ret = mv_cesa_ablkcipher_req_init(req, tmpl);
	if (ret)
		return ret;

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	engine = mv_cesa_select_engine(req->nbytes);
	mv_cesa_ablkcipher_prepare(&req->base, engine);

	ret = mv_cesa_queue_req(&req->base, &creq->base);

#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	ret = mv_cesa_queue_req(&req->base);
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	if (mv_cesa_req_needs_cleanup(&req->base, ret))
		mv_cesa_ablkcipher_cleanup(req);

	return ret;
}

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
static int mv_cesa_des_op(struct ablkcipher_request *req,
			  struct mv_cesa_op_ctx *tmpl)
{
	struct mv_cesa_des_ctx *ctx = crypto_tfm_ctx(req->base.tfm);

	mv_cesa_update_op_cfg(tmpl, CESA_SA_DESC_CFG_CRYPTM_DES,
			      CESA_SA_DESC_CFG_CRYPTM_MSK);

	memcpy(tmpl->ctx.blkcipher.key, ctx->key, DES_KEY_SIZE);

	return mv_cesa_ablkcipher_queue_req(req, tmpl);
}

#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
static int mv_cesa_ecb_des_encrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl,
			   CESA_SA_DESC_CFG_CRYPTCM_ECB |
			   CESA_SA_DESC_CFG_DIR_ENC);

	return mv_cesa_des_op(req, &tmpl);
}

static int mv_cesa_ecb_des_decrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl,
			   CESA_SA_DESC_CFG_CRYPTCM_ECB |
			   CESA_SA_DESC_CFG_DIR_DEC);

	return mv_cesa_des_op(req, &tmpl);
}

struct crypto_alg mv_cesa_ecb_des_alg = {
	.cra_name = "ecb(des)",
	.cra_driver_name = "mv-ecb-des",
	.cra_priority = 300,
	.cra_flags = CRYPTO_ALG_TYPE_ABLKCIPHER |
		     CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_ASYNC,
	.cra_blocksize = DES_BLOCK_SIZE,
	.cra_ctxsize = sizeof(struct mv_cesa_des_ctx),
	.cra_alignmask = 0,
	.cra_type = &crypto_ablkcipher_type,
	.cra_module = THIS_MODULE,
	.cra_init = mv_cesa_ablkcipher_cra_init,
	.cra_u = {
		.ablkcipher = {
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.setkey = mv_cesa_des_setkey,
			.encrypt = mv_cesa_ecb_des_encrypt,
			.decrypt = mv_cesa_ecb_des_decrypt,
		},
	},
};

static int mv_cesa_cbc_des_op(struct ablkcipher_request *req,
			      struct mv_cesa_op_ctx *tmpl)
{
	mv_cesa_update_op_cfg(tmpl, CESA_SA_DESC_CFG_CRYPTCM_CBC,
			      CESA_SA_DESC_CFG_CRYPTCM_MSK);

	memcpy(tmpl->ctx.blkcipher.iv, req->info, DES_BLOCK_SIZE);

	return mv_cesa_des_op(req, tmpl);
}

static int mv_cesa_cbc_des_encrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl, CESA_SA_DESC_CFG_DIR_ENC);

	return mv_cesa_cbc_des_op(req, &tmpl);
}

static int mv_cesa_cbc_des_decrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl, CESA_SA_DESC_CFG_DIR_DEC);

	return mv_cesa_cbc_des_op(req, &tmpl);
}

struct crypto_alg mv_cesa_cbc_des_alg = {
	.cra_name = "cbc(des)",
	.cra_driver_name = "mv-cbc-des",
	.cra_priority = 300,
	.cra_flags = CRYPTO_ALG_TYPE_ABLKCIPHER |
		     CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_ASYNC,
	.cra_blocksize = DES_BLOCK_SIZE,
	.cra_ctxsize = sizeof(struct mv_cesa_des_ctx),
	.cra_alignmask = 0,
	.cra_type = &crypto_ablkcipher_type,
	.cra_module = THIS_MODULE,
	.cra_init = mv_cesa_ablkcipher_cra_init,
	.cra_u = {
		.ablkcipher = {
			.min_keysize = DES_KEY_SIZE,
			.max_keysize = DES_KEY_SIZE,
			.ivsize	     = DES_BLOCK_SIZE,
			.setkey = mv_cesa_des_setkey,
			.encrypt = mv_cesa_cbc_des_encrypt,
			.decrypt = mv_cesa_cbc_des_decrypt,
		},
	},
};

static int mv_cesa_des3_op(struct ablkcipher_request *req,
			   struct mv_cesa_op_ctx *tmpl)
{
	struct mv_cesa_des3_ctx *ctx = crypto_tfm_ctx(req->base.tfm);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
//do nothing
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	int ret;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */

	mv_cesa_update_op_cfg(tmpl, CESA_SA_DESC_CFG_CRYPTM_3DES,
			      CESA_SA_DESC_CFG_CRYPTM_MSK);

	memcpy(tmpl->ctx.blkcipher.key, ctx->key, DES3_EDE_KEY_SIZE);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	return mv_cesa_ablkcipher_queue_req(req, tmpl);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	ret = mv_cesa_ablkcipher_req_init(req, tmpl);
	if (ret)
		return ret;

	ret = mv_cesa_queue_req(&req->base);
	if (mv_cesa_req_needs_cleanup(&req->base, ret))
		mv_cesa_ablkcipher_cleanup(req);

	return ret;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

static int mv_cesa_ecb_des3_ede_encrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl,
			   CESA_SA_DESC_CFG_CRYPTCM_ECB |
			   CESA_SA_DESC_CFG_3DES_EDE |
			   CESA_SA_DESC_CFG_DIR_ENC);

	return mv_cesa_des3_op(req, &tmpl);
}

static int mv_cesa_ecb_des3_ede_decrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl,
			   CESA_SA_DESC_CFG_CRYPTCM_ECB |
			   CESA_SA_DESC_CFG_3DES_EDE |
			   CESA_SA_DESC_CFG_DIR_DEC);

	return mv_cesa_des3_op(req, &tmpl);
}

struct crypto_alg mv_cesa_ecb_des3_ede_alg = {
	.cra_name = "ecb(des3_ede)",
	.cra_driver_name = "mv-ecb-des3-ede",
	.cra_priority = 300,
	.cra_flags = CRYPTO_ALG_TYPE_ABLKCIPHER |
		     CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_ASYNC,
	.cra_blocksize = DES3_EDE_BLOCK_SIZE,
	.cra_ctxsize = sizeof(struct mv_cesa_des3_ctx),
	.cra_alignmask = 0,
	.cra_type = &crypto_ablkcipher_type,
	.cra_module = THIS_MODULE,
	.cra_init = mv_cesa_ablkcipher_cra_init,
	.cra_u = {
		.ablkcipher = {
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.ivsize	     = DES3_EDE_BLOCK_SIZE,
			.setkey = mv_cesa_des3_ede_setkey,
			.encrypt = mv_cesa_ecb_des3_ede_encrypt,
			.decrypt = mv_cesa_ecb_des3_ede_decrypt,
		},
	},
};

static int mv_cesa_cbc_des3_op(struct ablkcipher_request *req,
			       struct mv_cesa_op_ctx *tmpl)
{
	memcpy(tmpl->ctx.blkcipher.iv, req->info, DES3_EDE_BLOCK_SIZE);

	return mv_cesa_des3_op(req, tmpl);
}

static int mv_cesa_cbc_des3_ede_encrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl,
			   CESA_SA_DESC_CFG_CRYPTCM_CBC |
			   CESA_SA_DESC_CFG_3DES_EDE |
			   CESA_SA_DESC_CFG_DIR_ENC);

	return mv_cesa_cbc_des3_op(req, &tmpl);
}

static int mv_cesa_cbc_des3_ede_decrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl,
			   CESA_SA_DESC_CFG_CRYPTCM_CBC |
			   CESA_SA_DESC_CFG_3DES_EDE |
			   CESA_SA_DESC_CFG_DIR_DEC);

	return mv_cesa_cbc_des3_op(req, &tmpl);
}

struct crypto_alg mv_cesa_cbc_des3_ede_alg = {
	.cra_name = "cbc(des3_ede)",
	.cra_driver_name = "mv-cbc-des3-ede",
	.cra_priority = 300,
	.cra_flags = CRYPTO_ALG_TYPE_ABLKCIPHER |
		     CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_ASYNC,
	.cra_blocksize = DES3_EDE_BLOCK_SIZE,
	.cra_ctxsize = sizeof(struct mv_cesa_des3_ctx),
	.cra_alignmask = 0,
	.cra_type = &crypto_ablkcipher_type,
	.cra_module = THIS_MODULE,
	.cra_init = mv_cesa_ablkcipher_cra_init,
	.cra_u = {
		.ablkcipher = {
			.min_keysize = DES3_EDE_KEY_SIZE,
			.max_keysize = DES3_EDE_KEY_SIZE,
			.ivsize	     = DES3_EDE_BLOCK_SIZE,
			.setkey = mv_cesa_des3_ede_setkey,
			.encrypt = mv_cesa_cbc_des3_ede_encrypt,
			.decrypt = mv_cesa_cbc_des3_ede_decrypt,
		},
	},
};

static int mv_cesa_aes_op(struct ablkcipher_request *req,
			  struct mv_cesa_op_ctx *tmpl)
{
	struct mv_cesa_aes_ctx *ctx = crypto_tfm_ctx(req->base.tfm);
#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	int i;
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	int ret, i;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	u32 *key;
	u32 cfg;

	cfg = CESA_SA_DESC_CFG_CRYPTM_AES;

	if (mv_cesa_get_op_cfg(tmpl) & CESA_SA_DESC_CFG_DIR_DEC)
		key = ctx->aes.key_dec;
	else
		key = ctx->aes.key_enc;

	for (i = 0; i < ctx->aes.key_length / sizeof(u32); i++)
		tmpl->ctx.blkcipher.key[i] = cpu_to_le32(key[i]);

	if (ctx->aes.key_length == 24)
		cfg |= CESA_SA_DESC_CFG_AES_LEN_192;
	else if (ctx->aes.key_length == 32)
		cfg |= CESA_SA_DESC_CFG_AES_LEN_256;

	mv_cesa_update_op_cfg(tmpl, cfg,
			      CESA_SA_DESC_CFG_CRYPTM_MSK |
			      CESA_SA_DESC_CFG_AES_LEN_MSK);

#if defined(CONFIG_SYNO_LSP_ARMADA_17_04_02)
	return mv_cesa_ablkcipher_queue_req(req, tmpl);
#else /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
	ret = mv_cesa_ablkcipher_req_init(req, tmpl);
	if (ret)
		return ret;

	ret = mv_cesa_queue_req(&req->base);
	if (mv_cesa_req_needs_cleanup(&req->base, ret))
		mv_cesa_ablkcipher_cleanup(req);

	return ret;
#endif /* CONFIG_SYNO_LSP_ARMADA_17_04_02 */
}

static int mv_cesa_ecb_aes_encrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl,
			   CESA_SA_DESC_CFG_CRYPTCM_ECB |
			   CESA_SA_DESC_CFG_DIR_ENC);

	return mv_cesa_aes_op(req, &tmpl);
}

static int mv_cesa_ecb_aes_decrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl,
			   CESA_SA_DESC_CFG_CRYPTCM_ECB |
			   CESA_SA_DESC_CFG_DIR_DEC);

	return mv_cesa_aes_op(req, &tmpl);
}

struct crypto_alg mv_cesa_ecb_aes_alg = {
	.cra_name = "ecb(aes)",
	.cra_driver_name = "mv-ecb-aes",
	.cra_priority = 300,
	.cra_flags = CRYPTO_ALG_TYPE_ABLKCIPHER |
		     CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_ASYNC,
	.cra_blocksize = AES_BLOCK_SIZE,
	.cra_ctxsize = sizeof(struct mv_cesa_aes_ctx),
	.cra_alignmask = 0,
	.cra_type = &crypto_ablkcipher_type,
	.cra_module = THIS_MODULE,
	.cra_init = mv_cesa_ablkcipher_cra_init,
	.cra_u = {
		.ablkcipher = {
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.setkey = mv_cesa_aes_setkey,
			.encrypt = mv_cesa_ecb_aes_encrypt,
			.decrypt = mv_cesa_ecb_aes_decrypt,
		},
	},
};

static int mv_cesa_cbc_aes_op(struct ablkcipher_request *req,
			      struct mv_cesa_op_ctx *tmpl)
{
	mv_cesa_update_op_cfg(tmpl, CESA_SA_DESC_CFG_CRYPTCM_CBC,
			      CESA_SA_DESC_CFG_CRYPTCM_MSK);
	memcpy(tmpl->ctx.blkcipher.iv, req->info, AES_BLOCK_SIZE);

	return mv_cesa_aes_op(req, tmpl);
}

static int mv_cesa_cbc_aes_encrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl, CESA_SA_DESC_CFG_DIR_ENC);

	return mv_cesa_cbc_aes_op(req, &tmpl);
}

static int mv_cesa_cbc_aes_decrypt(struct ablkcipher_request *req)
{
	struct mv_cesa_op_ctx tmpl;

	mv_cesa_set_op_cfg(&tmpl, CESA_SA_DESC_CFG_DIR_DEC);

	return mv_cesa_cbc_aes_op(req, &tmpl);
}

struct crypto_alg mv_cesa_cbc_aes_alg = {
	.cra_name = "cbc(aes)",
	.cra_driver_name = "mv-cbc-aes",
	.cra_priority = 300,
	.cra_flags = CRYPTO_ALG_TYPE_ABLKCIPHER |
		     CRYPTO_ALG_KERN_DRIVER_ONLY | CRYPTO_ALG_ASYNC,
	.cra_blocksize = AES_BLOCK_SIZE,
	.cra_ctxsize = sizeof(struct mv_cesa_aes_ctx),
	.cra_alignmask = 0,
	.cra_type = &crypto_ablkcipher_type,
	.cra_module = THIS_MODULE,
	.cra_init = mv_cesa_ablkcipher_cra_init,
	.cra_u = {
		.ablkcipher = {
			.min_keysize = AES_MIN_KEY_SIZE,
			.max_keysize = AES_MAX_KEY_SIZE,
			.ivsize = AES_BLOCK_SIZE,
			.setkey = mv_cesa_aes_setkey,
			.encrypt = mv_cesa_cbc_aes_encrypt,
			.decrypt = mv_cesa_cbc_aes_decrypt,
		},
	},
};

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * Driver for Marvell NETA network controller Buffer Manager.
 *
 * Copyright (C) 2015 Marvell
 *
 * Marcin Wojtas <mw@semihalf.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clk.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/mbus.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/of.h>
#if defined(MY_DEF_HERE)
#include <linux/of_platform.h>
#endif /* MY_DEF_HERE */
#include <linux/platform_device.h>
#if defined(MY_DEF_HERE)
#include <linux/netdevice.h>
#endif /* MY_DEF_HERE */
#include <linux/skbuff.h>
#include <net/hwbm.h>
#include "mvneta_bm.h"

#define MVNETA_BM_DRIVER_NAME "mvneta_bm"
#define MVNETA_BM_DRIVER_VERSION "1.0"

static void mvneta_bm_write(struct mvneta_bm *priv, u32 offset, u32 data)
{
	writel(data, priv->reg_base + offset);
}

static u32 mvneta_bm_read(struct mvneta_bm *priv, u32 offset)
{
	return readl(priv->reg_base + offset);
}

static void mvneta_bm_pool_enable(struct mvneta_bm *priv, int pool_id)
{
	u32 val;

	val = mvneta_bm_read(priv, MVNETA_BM_POOL_BASE_REG(pool_id));
	val |= MVNETA_BM_POOL_ENABLE_MASK;
	mvneta_bm_write(priv, MVNETA_BM_POOL_BASE_REG(pool_id), val);

	/* Clear BM cause register */
	mvneta_bm_write(priv, MVNETA_BM_INTR_CAUSE_REG, 0);
}

static void mvneta_bm_pool_disable(struct mvneta_bm *priv, int pool_id)
{
	u32 val;

	val = mvneta_bm_read(priv, MVNETA_BM_POOL_BASE_REG(pool_id));
	val &= ~MVNETA_BM_POOL_ENABLE_MASK;
	mvneta_bm_write(priv, MVNETA_BM_POOL_BASE_REG(pool_id), val);
}

static inline void mvneta_bm_config_set(struct mvneta_bm *priv, u32 mask)
{
	u32 val;

	val = mvneta_bm_read(priv, MVNETA_BM_CONFIG_REG);
	val |= mask;
	mvneta_bm_write(priv, MVNETA_BM_CONFIG_REG, val);
}

static inline void mvneta_bm_config_clear(struct mvneta_bm *priv, u32 mask)
{
	u32 val;

	val = mvneta_bm_read(priv, MVNETA_BM_CONFIG_REG);
	val &= ~mask;
	mvneta_bm_write(priv, MVNETA_BM_CONFIG_REG, val);
}

static void mvneta_bm_pool_target_set(struct mvneta_bm *priv, int pool_id,
				      u8 target_id, u8 attr)
{
	u32 val;

	val = mvneta_bm_read(priv, MVNETA_BM_XBAR_POOL_REG(pool_id));
	val &= ~MVNETA_BM_TARGET_ID_MASK(pool_id);
	val &= ~MVNETA_BM_XBAR_ATTR_MASK(pool_id);
	val |= MVNETA_BM_TARGET_ID_VAL(pool_id, target_id);
	val |= MVNETA_BM_XBAR_ATTR_VAL(pool_id, attr);

	mvneta_bm_write(priv, MVNETA_BM_XBAR_POOL_REG(pool_id), val);
}

int mvneta_bm_construct(struct hwbm_pool *hwbm_pool, void *buf)
{
	struct mvneta_bm_pool *bm_pool =
		(struct mvneta_bm_pool *)hwbm_pool->priv;
	struct mvneta_bm *priv = bm_pool->priv;
	dma_addr_t phys_addr;

	/* In order to update buf_cookie field of RX descriptor properly,
	 * BM hardware expects buf virtual address to be placed in the
	 * first four bytes of corrected mapped buffer.
	 */
	u8 *tmp = (u8 *)buf + priv->rx_offset_correction;
	*(u32 *)tmp = (u32)((uintptr_t)buf & 0xffffffff);

	phys_addr = dma_map_single(&priv->pdev->dev, buf, bm_pool->buf_size,
				   DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(&priv->pdev->dev, phys_addr)))
		return -ENOMEM;

	phys_addr += priv->rx_offset_correction;

	mvneta_bm_pool_put_bp(priv, bm_pool, phys_addr);
	return 0;
}
EXPORT_SYMBOL_GPL(mvneta_bm_construct);

/* Create pool */
static int mvneta_bm_pool_create(struct mvneta_bm *priv,
				 struct mvneta_bm_pool *bm_pool)
{
	const struct mbus_dram_target_info *dram_target_info;
	struct platform_device *pdev = priv->pdev;
	u8 target_id, attr;
	int size_bytes, i;

	size_bytes = sizeof(u32) * bm_pool->hwbm_pool.size;
	bm_pool->virt_addr = dma_alloc_coherent(&pdev->dev, size_bytes,
						&bm_pool->phys_addr,
						GFP_KERNEL);
	if (!bm_pool->virt_addr)
		return -ENOMEM;

	if (!IS_ALIGNED((uintptr_t)bm_pool->virt_addr, MVNETA_BM_POOL_PTR_ALIGN)) {
		dma_free_coherent(&pdev->dev, size_bytes, bm_pool->virt_addr,
				  bm_pool->phys_addr);
		dev_err(&pdev->dev, "BM pool %d is not %d bytes aligned\n",
			bm_pool->id, MVNETA_BM_POOL_PTR_ALIGN);
		return -ENOMEM;
	}

	dram_target_info = mv_mbus_dram_info();
	if (dram_target_info) {
		target_id = dram_target_info->mbus_dram_target_id;
		attr = 0;
		/* Try to find matching DRAM window for buffer phyaddr */
		for (i = 0; i < dram_target_info->num_cs; i++) {
			const struct mbus_dram_window *cs = dram_target_info->cs + i;

			if ((cs->base <= bm_pool->phys_addr) &&
			    (bm_pool->phys_addr <= (cs->base + cs->size - 1))) {
				attr = cs->mbus_attr;
				break;
			}
		}
		mvneta_bm_pool_target_set(priv, bm_pool->id, target_id, attr);
	}

	/* Set pool address */
	mvneta_bm_write(priv, MVNETA_BM_POOL_BASE_REG(bm_pool->id), bm_pool->phys_addr);

	mvneta_bm_pool_enable(priv, bm_pool->id);

	return 0;
}

#if defined(MY_DEF_HERE)
static void mvneta_bm_skb_free(struct sk_buff *skb)
{
	dev_kfree_skb_any(skb);
}

static struct sk_buff *mvneta_bm_skb_alloc(struct mvneta_bm_pool *bm_pool,
					   dma_addr_t *phys_addr, gfp_t gfp_mask)
{
	struct sk_buff *skb;
	struct mvneta_bm *priv = bm_pool->priv;
	u8 *data;
	dma_addr_t paddr;

	skb = __dev_alloc_skb(bm_pool->pkt_size, GFP_DMA | gfp_mask);
	if (!skb)
		return NULL;

	data = skb->head + priv->rx_offset_correction;

	/* Save skb as first 4 bytes in the buffer, then skb can be get through rx_desc->bufCookie */
	*((u32 *)data) = (u32)((uintptr_t)skb & 0xffffffff);

	paddr = dma_map_single(&priv->pdev->dev, skb->head, bm_pool->buf_size, DMA_TO_DEVICE);
	if (unlikely(dma_mapping_error(&priv->pdev->dev, paddr))) {
		dev_kfree_skb_any(skb);
		return NULL;
	}
	if (phys_addr)
		*phys_addr = paddr + priv->rx_offset_correction;

	return skb;
}

#endif /* MY_DEF_HERE */
/* Notify the driver that BM pool is being used as specific type and return the
 * pool pointer on success
 */
struct mvneta_bm_pool *mvneta_bm_pool_use(struct mvneta_bm *priv, u8 pool_id,
					  enum mvneta_bm_type type, u8 port_id,
					  int pkt_size)
{
	struct mvneta_bm_pool *new_pool = &priv->bm_pools[pool_id];
	int num, err;

	if (new_pool->type == MVNETA_BM_LONG &&
	    new_pool->port_map != 1 << port_id) {
		dev_err(&priv->pdev->dev,
			"long pool cannot be shared by the ports\n");
		return NULL;
	}

	if (new_pool->type == MVNETA_BM_SHORT && new_pool->type != type) {
		dev_err(&priv->pdev->dev,
			"mixing pools' types between the ports is forbidden\n");
		return NULL;
	}

#if defined(MY_DEF_HERE)
	if (type == MVNETA_BM_SHORT)
#else /* MY_DEF_HERE */
	if (new_pool->pkt_size == 0 || type != MVNETA_BM_SHORT)
#endif /* MY_DEF_HERE */
		new_pool->pkt_size = pkt_size;

	/* Allocate buffers in case BM pool hasn't been used yet */
	if (new_pool->type == MVNETA_BM_FREE) {
		struct hwbm_pool *hwbm_pool = &new_pool->hwbm_pool;

		new_pool->priv = priv;
		new_pool->type = type;
		new_pool->buf_size = MVNETA_RX_BUF_SIZE(new_pool->pkt_size);
		hwbm_pool->frag_size =
			SKB_DATA_ALIGN(MVNETA_RX_BUF_SIZE(new_pool->pkt_size)) +
			SKB_DATA_ALIGN(sizeof(struct skb_shared_info));
		hwbm_pool->construct = mvneta_bm_construct;
		hwbm_pool->priv = new_pool;

		/* Create new pool */
		err = mvneta_bm_pool_create(priv, new_pool);
		if (err) {
			dev_err(&priv->pdev->dev, "fail to create pool %d\n",
				new_pool->id);
			return NULL;
		}

#if defined(MY_DEF_HERE)
#ifdef CONFIG_64BIT
		{
			struct sk_buff *skb;
			dma_addr_t paddr;

			/* In Neta HW only 32 bits data is supported, so in order to
			 * obtain whole 64 bits address from RX descriptor, we store the
			* upper 32 bits when allocating buffer, and put it back
			* when using buffer cookie for accessing packet in memory.
			* Frags should be allocated from single 'memory' region, hence
			* common upper address half should be sufficient.
			*/
			skb = mvneta_bm_skb_alloc(new_pool, &paddr, GFP_KERNEL);
			if (skb) {
				new_pool->data_high = (u64)skb & 0xffffffff00000000;
				dev_kfree_skb_any(skb);
			}
		}
#endif /* CONFIG_64BIT */

#endif /* MY_DEF_HERE */
		/* Allocate buffers for this pool */
#if defined(MY_DEF_HERE)
		num = mvneta_bm_bufs_add(new_pool, hwbm_pool->size);
#else /* MY_DEF_HERE */
		num = hwbm_pool_add(hwbm_pool, hwbm_pool->size, GFP_ATOMIC);
#endif /* MY_DEF_HERE */
		if (num != hwbm_pool->size) {
			WARN(1, "pool %d: %d of %d allocated\n",
			     new_pool->id, num, hwbm_pool->size);
			return NULL;
		}
	}

	return new_pool;
}
EXPORT_SYMBOL_GPL(mvneta_bm_pool_use);

#if defined(MY_DEF_HERE)
int mvneta_bm_refill(struct mvneta_bm_pool *bm_pool, gfp_t gfp_mask)
{
	dma_addr_t paddr;
	struct mvneta_bm *priv = bm_pool->priv;
	struct sk_buff *skb;

	skb = mvneta_bm_skb_alloc(bm_pool, &paddr, gfp_mask | __GFP_NOWARN);
	if (!skb)
		return -ENOMEM;

	/* Sanity check: data_high must be the same for all allocated SKBs */
#ifdef CONFIG_64BIT
	if (unlikely(bm_pool->data_high != ((u64)skb & 0xffffffff00000000))) {
		pr_err("data_high = 0x%llx is not match allocated skb = %p\n",
		       bm_pool->data_high, skb);
		mvneta_bm_skb_free(skb);
		return -EINVAL;
	}
	/* paddr must be in 32bit range */
	if (paddr & 0xffffffff00000000) {
		pr_err("pool %d: paddr must be in 32b range. paddr = 0x%llx\n",
		       bm_pool->id, paddr);
		mvneta_bm_skb_free(skb);
		return -EINVAL;
	}
#endif

	mvneta_bm_pool_put_bp(priv, bm_pool, paddr);

	return 0;
}

/* Allocate and add number of buffers to the BM pool */
int mvneta_bm_bufs_add(struct mvneta_bm_pool *bm_pool, int nof_bufs)
{
	int i, err;
	unsigned long flags;

	spin_lock_irqsave(&bm_pool->hwbm_pool.lock, flags);

	if ((bm_pool->hwbm_pool.buf_num + nof_bufs) > bm_pool->hwbm_pool.size) {
		nof_bufs = bm_pool->hwbm_pool.size - bm_pool->hwbm_pool.buf_num;
		pr_warn("BM pool #%d: Can't add %d buffers, buf_num = %d, size = %d\n",
			bm_pool->id, nof_bufs,
			bm_pool->hwbm_pool.buf_num, bm_pool->hwbm_pool.size);
	}

	for (i = 0; i < nof_bufs; i++) {
		err = mvneta_bm_refill(bm_pool, GFP_KERNEL);
		if (err < 0)
			break;
	}

	/* Update BM driver with number of buffers added to pool */
	bm_pool->hwbm_pool.buf_num += i;

	spin_unlock_irqrestore(&bm_pool->hwbm_pool.lock, flags);

	pr_debug("BM pool #%d: %d of %d buffers added\n",
		 bm_pool->id, i, nof_bufs);

	return i;
}
EXPORT_SYMBOL_GPL(mvneta_bm_bufs_add);

#endif /* MY_DEF_HERE */
/* Free all buffers from the pool */
void mvneta_bm_bufs_free(struct mvneta_bm *priv, struct mvneta_bm_pool *bm_pool,
			 u8 port_map)
{
	int i;

	bm_pool->port_map &= ~port_map;
	if (bm_pool->port_map)
		return;

	mvneta_bm_config_set(priv, MVNETA_BM_EMPTY_LIMIT_MASK);

	for (i = 0; i < bm_pool->hwbm_pool.buf_num; i++) {
		dma_addr_t buf_phys_addr;
		u32 *vaddr;
#if defined(MY_DEF_HERE)
		struct sk_buff *skb;
#endif /* MY_DEF_HERE */

		/* Get buffer physical address (indirect access) */
		buf_phys_addr = mvneta_bm_pool_get_bp(priv, bm_pool);

		/* Work-around to the problems when destroying the pool,
		 * when it occurs that a read access to BPPI returns 0.
		 */
		if (buf_phys_addr == 0)
#if defined(MY_DEF_HERE)
			break;

		dma_unmap_single(&priv->pdev->dev, buf_phys_addr - priv->rx_offset_correction,
				 bm_pool->buf_size, DMA_FROM_DEVICE);
#else /* MY_DEF_HERE */
			continue;
#endif /* MY_DEF_HERE */

		vaddr = phys_to_virt(buf_phys_addr);
		if (!vaddr)
			break;

#if defined(MY_DEF_HERE)
#ifdef CONFIG_64BIT
		skb = (struct sk_buff *)((u64)(*(u32 *)vaddr) | bm_pool->data_high);
#else
		skb = (struct sk_buff *)(*(u32 *)vaddr);
#endif
		if (!skb)
			break;

		mvneta_bm_skb_free(skb);
#else /* MY_DEF_HERE */
		dma_unmap_single(&priv->pdev->dev, buf_phys_addr,
				 bm_pool->buf_size, DMA_FROM_DEVICE);
		hwbm_buf_free(&bm_pool->hwbm_pool, vaddr);
#endif /* MY_DEF_HERE */
	}

	mvneta_bm_config_clear(priv, MVNETA_BM_EMPTY_LIMIT_MASK);

#if defined(MY_DEF_HERE)
	pr_info("BM pool #%d: %d of %d buffers are freed\n",
		bm_pool->id, i, bm_pool->hwbm_pool.buf_num);

#endif /* MY_DEF_HERE */
	/* Update BM driver with number of buffers removed from pool */
	bm_pool->hwbm_pool.buf_num -= i;
}
EXPORT_SYMBOL_GPL(mvneta_bm_bufs_free);

/* Cleanup pool */
void mvneta_bm_pool_destroy(struct mvneta_bm *priv,
			    struct mvneta_bm_pool *bm_pool, u8 port_map)
{
	struct hwbm_pool *hwbm_pool = &bm_pool->hwbm_pool;
	bm_pool->port_map &= ~port_map;
	if (bm_pool->port_map)
		return;

	bm_pool->type = MVNETA_BM_FREE;

	mvneta_bm_bufs_free(priv, bm_pool, port_map);
	if (hwbm_pool->buf_num)
		WARN(1, "cannot free %d buffers in pool %d\n", hwbm_pool->buf_num, bm_pool->id);

	if (bm_pool->virt_addr) {
		dma_free_coherent(&priv->pdev->dev,
				  sizeof(u32) * hwbm_pool->size,
				  bm_pool->virt_addr, bm_pool->phys_addr);
		bm_pool->virt_addr = NULL;
	}

	mvneta_bm_pool_disable(priv, bm_pool->id);
}
EXPORT_SYMBOL_GPL(mvneta_bm_pool_destroy);

static void mvneta_bm_pools_init(struct mvneta_bm *priv)
{
	struct device_node *dn = priv->pdev->dev.of_node;
	struct mvneta_bm_pool *bm_pool;
	char prop[15];
	u32 size;
	int i;

	/* Activate BM unit */
	mvneta_bm_write(priv, MVNETA_BM_COMMAND_REG, MVNETA_BM_START_MASK);

	/* Create all pools with maximum size */
	for (i = 0; i < MVNETA_BM_POOLS_NUM; i++) {
		bm_pool = &priv->bm_pools[i];
		bm_pool->id = i;
		bm_pool->type = MVNETA_BM_FREE;

		/* Reset read pointer */
		mvneta_bm_write(priv, MVNETA_BM_POOL_READ_PTR_REG(i), 0);

		/* Reset write pointer */
		mvneta_bm_write(priv, MVNETA_BM_POOL_WRITE_PTR_REG(i), 0);

		/* Configure pool size according to DT or use default value */
		sprintf(prop, "pool%d,capacity", i);
		if (of_property_read_u32(dn, prop, &size)) {
			size = MVNETA_BM_POOL_CAP_DEF;
		} else if (size > MVNETA_BM_POOL_CAP_MAX) {
			dev_warn(&priv->pdev->dev,
				 "Illegal pool %d capacity %d, set to %d\n",
				 i, size, MVNETA_BM_POOL_CAP_MAX);
			size = MVNETA_BM_POOL_CAP_MAX;
		} else if (size < MVNETA_BM_POOL_CAP_MIN) {
			dev_warn(&priv->pdev->dev,
				 "Illegal pool %d capacity %d, set to %d\n",
				 i, size, MVNETA_BM_POOL_CAP_MIN);
			size = MVNETA_BM_POOL_CAP_MIN;
		} else if (!IS_ALIGNED(size, MVNETA_BM_POOL_CAP_ALIGN)) {
			dev_warn(&priv->pdev->dev,
				 "Illegal pool %d capacity %d, round to %d\n",
				 i, size, ALIGN(size,
				 MVNETA_BM_POOL_CAP_ALIGN));
			size = ALIGN(size, MVNETA_BM_POOL_CAP_ALIGN);
		}
		bm_pool->hwbm_pool.size = size;

		mvneta_bm_write(priv, MVNETA_BM_POOL_SIZE_REG(i),
				bm_pool->hwbm_pool.size);

		/* Obtain custom pkt_size from DT */
		sprintf(prop, "pool%d,pkt-size", i);
		if (of_property_read_u32(dn, prop, &bm_pool->pkt_size))
#if defined(MY_DEF_HERE)
			/* if not specified by DT, set default buffer size to support Jumbo */
			bm_pool->pkt_size = MVNETA_BM_LONG_PKT_SIZE;
#else /* MY_DEF_HERE */
			bm_pool->pkt_size = 0;
#endif /* MY_DEF_HERE */
	}
}

static void mvneta_bm_default_set(struct mvneta_bm *priv)
{
	u32 val;

	/* Mask BM all interrupts */
	mvneta_bm_write(priv, MVNETA_BM_INTR_MASK_REG, 0);

	/* Clear BM cause register */
	mvneta_bm_write(priv, MVNETA_BM_INTR_CAUSE_REG, 0);

	/* Set BM configuration register */
	val = mvneta_bm_read(priv, MVNETA_BM_CONFIG_REG);

	/* Reduce MaxInBurstSize from 32 BPs to 16 BPs */
	val &= ~MVNETA_BM_MAX_IN_BURST_SIZE_MASK;
	val |= MVNETA_BM_MAX_IN_BURST_SIZE_16BP;
	mvneta_bm_write(priv, MVNETA_BM_CONFIG_REG, val);
}

static int mvneta_bm_init(struct mvneta_bm *priv)
{
	mvneta_bm_default_set(priv);

	/* Allocate and initialize BM pools structures */
	priv->bm_pools = devm_kcalloc(&priv->pdev->dev, MVNETA_BM_POOLS_NUM,
				      sizeof(struct mvneta_bm_pool),
				      GFP_KERNEL);
	if (!priv->bm_pools)
		return -ENOMEM;

	mvneta_bm_pools_init(priv);

	return 0;
}

static int mvneta_bm_get_sram(struct device_node *dn,
			      struct mvneta_bm *priv)
{
#if defined(MY_DEF_HERE)
	struct platform_device *pdev;
	struct device_node *np_pool;
	struct resource *res;

	np_pool = of_parse_phandle(dn, "internal-mem", 0);
	if (!np_pool)
		return -1;
	pdev = of_find_device_by_node(np_pool);
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("%s:: found no memory resource\n", __func__);
		return -EINVAL;
	}
	/* get size of internal SRAM access region */
	priv->bppi_size = resource_size(res);

#endif /* MY_DEF_HERE */
	priv->bppi_pool = of_gen_pool_get(dn, "internal-mem", 0);
	if (!priv->bppi_pool) {
		pr_err("%s:: no internal-mem node found\n", __func__);
		return -ENOMEM;
	}

	priv->bppi_virt_addr = gen_pool_dma_alloc(priv->bppi_pool,
#if defined(MY_DEF_HERE)
						  priv->bppi_size,
#else /* MY_DEF_HERE */
						  MVNETA_BM_BPPI_SIZE,
#endif /* MY_DEF_HERE */
						  &priv->bppi_phys_addr);
	if (!priv->bppi_virt_addr)
		return -ENOMEM;

	return 0;
}

static void mvneta_bm_put_sram(struct mvneta_bm *priv)
{
#if defined(MY_DEF_HERE)
	gen_pool_free(priv->bppi_pool, priv->bppi_phys_addr, priv->bppi_size);
#else /* MY_DEF_HERE */
	gen_pool_free(priv->bppi_pool, priv->bppi_phys_addr,
		      MVNETA_BM_BPPI_SIZE);
#endif /* MY_DEF_HERE */
}

static int mvneta_bm_probe(struct platform_device *pdev)
{
	struct device_node *dn = pdev->dev.of_node;
	struct mvneta_bm *priv;
	struct resource *res;
	int err;

	priv = devm_kzalloc(&pdev->dev, sizeof(struct mvneta_bm), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	priv->reg_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(priv->reg_base))
		return PTR_ERR(priv->reg_base);

	priv->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);
	err = clk_prepare_enable(priv->clk);
	if (err < 0)
		return err;

	err = mvneta_bm_get_sram(dn, priv);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to allocate internal memory\n");
		goto err_clk;
	}

	priv->pdev = pdev;

	/* Initialize buffer manager internals */
	err = mvneta_bm_init(priv);
	if (err < 0) {
		dev_err(&pdev->dev, "failed to initialize controller\n");
		goto err_sram;
	}

	/* HW support packet headroom up to 127 bytes.
	 * MVNETA_RX_PKT_OFFSET_CORRECTION (64) defines maximum headroom size supported by driver.
	 * rx_offset_correction calculated to meet requirement above.
	 */
	priv->rx_offset_correction = max(0, NET_SKB_PAD - MVNETA_RX_PKT_OFFSET_CORRECTION);

	dn->data = priv;
	platform_set_drvdata(pdev, priv);

	dev_info(&pdev->dev, "Buffer Manager for network controller enabled\n");

	return 0;

err_sram:
	mvneta_bm_put_sram(priv);
err_clk:
	clk_disable_unprepare(priv->clk);
	return err;
}

static int mvneta_bm_remove(struct platform_device *pdev)
{
	struct mvneta_bm *priv = platform_get_drvdata(pdev);
	u8 all_ports_map = 0xff;
	int i = 0;

	for (i = 0; i < MVNETA_BM_POOLS_NUM; i++) {
		struct mvneta_bm_pool *bm_pool = &priv->bm_pools[i];

		mvneta_bm_pool_destroy(priv, bm_pool, all_ports_map);
	}

	mvneta_bm_put_sram(priv);

	/* Dectivate BM unit */
	mvneta_bm_write(priv, MVNETA_BM_COMMAND_REG, MVNETA_BM_STOP_MASK);

	clk_disable_unprepare(priv->clk);

	return 0;
}

static const struct of_device_id mvneta_bm_match[] = {
	{ .compatible = "marvell,armada-380-neta-bm" },
	{ }
};
MODULE_DEVICE_TABLE(of, mvneta_bm_match);

static struct platform_driver mvneta_bm_driver = {
	.probe = mvneta_bm_probe,
	.remove = mvneta_bm_remove,
	.driver = {
		.name = MVNETA_BM_DRIVER_NAME,
		.of_match_table = mvneta_bm_match,
	},
};

module_platform_driver(mvneta_bm_driver);

MODULE_DESCRIPTION("Marvell NETA Buffer Manager Driver - www.marvell.com");
MODULE_AUTHOR("Marcin Wojtas <mw@semihalf.com>");
MODULE_LICENSE("GPL v2");

// SPDX-License-Identifier: GPL-2.0-only
/*
 * linux/drivers/remoteproc/sunxi_rproc.c
 *
 * Copyright © 2020-2025, wujiayi
 *              Author: wujiayi <wujiayi@allwinnertech.com>
 *
 * This file is provided under a dual BSD/GPL license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/arm-smccc.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/remoteproc.h>
#include <linux/io.h>
#include <linux/mailbox_client.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_reserved_mem.h>
#include <linux/pm_wakeirq.h>
#include <linux/regmap.h>
#include <linux/remoteproc.h>
#include <linux/reset.h>
#include <linux/slab.h>
#include <linux/pinctrl/consumer.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/clk.h>
#include <linux/skbuff.h>
#include "remoteproc_internal.h"

#define SUNXI_RPROC_VERSION "V1.0.0"

/*
 * Register define
 */
#define DSP_ALT_RESET_VEC_REG   (0x0000) /* DSP Reset Control Register */
#define DSP_CTRL_REG0           (0x0004) /* DSP Control Register0 */
#define DSP_PRID_REG            (0x000c) /* DSP PRID Register */
#define DSP_STAT_REG            (0x0010) /* DSP STAT Register */
#define DSP_BIST_CTRL_REG       (0x0014) /* DSP BIST CTRL Register */
#define DSP_JTRST_REG           (0x001c) /* DSP JTAG CONFIG RESET Register */
#define DSP_VER_REG             (0x0020) /* DSP Version Register */

/*
 * DSP Control Register0
 */
#define BIT_RUN_STALL           (0)
#define BIT_START_VEC_SEL       (1)
#define BIT_DSP_CLKEN           (2)

#define DSP_BOOT_SRAM_REMAP_REG   (0x8)
#define BIT_SRAM_REMAP_ENABLE  (0)

#define MBOX_NB_VQ		2
#define MBOX_NB_MBX		2

#define SUNXI_MBX_VQ0		"arm-mbox-tx"
#define SUNXI_MBX_VQ0_ID	0
#define SUNXI_MBX_VQ1		"arm-mbox-rx"
#define SUNXI_MBX_VQ1_ID	1

struct sunxi_mbox {
	const unsigned char name[30];
	struct mbox_chan *chan;
	struct mbox_client client;
	struct work_struct vq_work;
	spinlock_t tx_skbs_lock;
	struct sk_buff_head tx_skbs;
	int vq_id;
};

struct sunxi_rproc_memory_mapping {
	u64 pa; /* Address seen on the cpu side */
	u64 da; /* Address seen on the remote processor side */
	u64 len;
};

struct sunxi_rproc {
	struct sunxi_rproc_memory_mapping *mem_maps;
	int mem_maps_cnt;

	struct clk *pclk; /* pll clock */
	struct clk *mclk; /* mod clock */
	struct clk *bclk; /* bus clock */
	struct clk *aclk; /* ahbs clock */
	struct reset_control *rst; /* rst control */
	struct reset_control *cfg; /* cfg control */
	struct reset_control *dbg; /* dbg control */

	u32 pc_entry;
	u32 mclk_freq; /* mclk freq */
	void __iomem *sram_remap;
	void __iomem *dsp_cfg;
	int irq;
	struct sunxi_mbox mb[MBOX_NB_MBX];
	struct workqueue_struct *workqueue;
};

static int xplorer_debug;
module_param(xplorer_debug, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(xplorer_debug, "Debug for Xplorer");

static int sunxi_rproc_pa_to_da(struct rproc *rproc, phys_addr_t pa, u64 *da)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *ddata = rproc->priv;
	struct sunxi_rproc_memory_mapping *map;
	int i;

	/*
	 * Maybe there are multiple DAs corresponding to one PA.
	 * Here we only return the first matching one in the map table.
	 */
	for (i = 0; i < ddata->mem_maps_cnt; i++) {
		map = &ddata->mem_maps[i];
		if (pa < map->pa || pa >= map->pa + map->len)
			continue;
		*da = pa - map->pa + map->da;
		dev_dbg(dev, "translate pa %pa to da 0x%llx\n", &pa, *da);
		return 0;
	}

	dev_err(dev, "Failed to translate pa %pa to da\n", &pa);
	return -EINVAL;
}

static int sunxi_rproc_da_to_pa(struct rproc *rproc, u64 da, phys_addr_t *pa)
{
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *ddata = rproc->priv;
	struct sunxi_rproc_memory_mapping *map;
	int i;

	for (i = 0; i < ddata->mem_maps_cnt; i++) {
		map = &ddata->mem_maps[i];
		if (da < map->da || da >= map->da + map->len)
			continue;
		*pa = da - map->da + map->pa;
		dev_dbg(dev, "translate da 0x%llx to pa %pa\n", da, pa);
		return 0;
	}

	dev_err(dev, "Failed to translate da 0x%llx to pa\n", da);
	return -EINVAL;
}

static int sunxi_rproc_mem_alloc(struct rproc *rproc,
				 struct rproc_mem_entry *mem)
{
	struct device *dev = rproc->dev.parent;
	void *va;

	dev_dbg(dev, "map memory: %pad+%x\n", &mem->dma, mem->len);
	va = ioremap_wc(mem->dma, mem->len);
	if (IS_ERR_OR_NULL(va)) {
		dev_err(dev, "Unable to map memory region: %pad+%x\n",
			&mem->dma, mem->len);
		return -ENOMEM;
	}

	/* Update memory entry va */
	mem->va = va;

	return 0;
}

static int sunxi_rproc_mem_release(struct rproc *rproc,
				   struct rproc_mem_entry *mem)
{
	dev_dbg(rproc->dev.parent, "unmap memory: %pad\n", &mem->dma);
	iounmap(mem->va);

	return 0;
}

static void *sunxi_rproc_da_to_va(struct rproc *rproc, u64 da, int len)
{
	struct device *dev = rproc->dev.parent;
	struct rproc_mem_entry *carveout;
	void *ptr = NULL;
	phys_addr_t pa;
	int ret;

	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);

	/* first step: translate da to pa */
	ret = sunxi_rproc_da_to_pa(rproc, da, &pa);
	if (ret < 0) {
		dev_err(dev, "invalid da 0x%llx\n", da);
		return NULL;
	}

	/* second step: get va from carveouts via pa */
	list_for_each_entry(carveout, &rproc->carveouts, node) {
		if ((pa >= carveout->dma) && (pa < carveout->dma + carveout->len)) {
			ptr = carveout->va + (pa - carveout->dma);
			return ptr;
		}
	}
	return NULL;
}

static void sunxi_rproc_mb_vq_work(struct work_struct *work)
{
	struct sunxi_mbox *mb = container_of(work, struct sunxi_mbox, vq_work);
	struct rproc *rproc = dev_get_drvdata(mb->client.dev);

	if (rproc_vq_interrupt(rproc, mb->vq_id) == IRQ_NONE)
		dev_dbg(&rproc->dev, "no message found in vq%d\n", mb->vq_id);
}

static void sunxi_rproc_mb_rx_callback(struct mbox_client *cl, void *data)
{
	struct rproc *rproc = dev_get_drvdata(cl->dev);
	struct sunxi_mbox *mb = container_of(cl, struct sunxi_mbox, client);
	struct sunxi_rproc *ddata = rproc->priv;

	queue_work(ddata->workqueue, &mb->vq_work);
}

static void sunxi_rproc_mb_tx_done(struct mbox_client *cl, void *msg, int r)
{
	struct sunxi_mbox *mb = container_of(cl, struct sunxi_mbox, client);
	struct sk_buff *skb;

	spin_lock(&mb->tx_skbs_lock);
	skb = skb_dequeue(&mb->tx_skbs);
	spin_unlock(&mb->tx_skbs_lock);

	kfree_skb(skb);
}

static const struct sunxi_mbox sunxi_rproc_mbox[MBOX_NB_MBX] = {
	{
		.name = SUNXI_MBX_VQ0,
		.vq_id = SUNXI_MBX_VQ0_ID,
		.client = {
			.rx_callback = sunxi_rproc_mb_rx_callback,
			.tx_block = false,
			.tx_done = sunxi_rproc_mb_tx_done,
		},
	},
	{
		.name = SUNXI_MBX_VQ1,
		.vq_id = SUNXI_MBX_VQ1_ID,
		.client = {
			.rx_callback = sunxi_rproc_mb_rx_callback,
			.tx_block = false,
			.tx_done = sunxi_rproc_mb_tx_done,
		},
	},

};

static int sunxi_rproc_request_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *ddata = rproc->priv;
	struct device *dev = &rproc->dev;
	unsigned int i;
	int j;
	const unsigned char *name;
	struct mbox_client *cl;

	/* Initialise mailbox structure table */
	memcpy(ddata->mb, sunxi_rproc_mbox, sizeof(sunxi_rproc_mbox));

	for (i = 0; i < MBOX_NB_MBX; i++) {
		name = ddata->mb[i].name;

		cl = &ddata->mb[i].client;
		cl->dev = dev->parent;

		ddata->mb[i].chan = mbox_request_channel_byname(cl, name);
		if (IS_ERR(ddata->mb[i].chan)) {
			if (PTR_ERR(ddata->mb[i].chan) == -EPROBE_DEFER)
				goto err_probe;
			dev_warn(dev, "cannot get %s mbox\n", name);
			ddata->mb[i].chan = NULL;
		}
		if (ddata->mb[i].vq_id >= 0) {
			INIT_WORK(&ddata->mb[i].vq_work,
				  sunxi_rproc_mb_vq_work);
			spin_lock_init(&ddata->mb[i].tx_skbs_lock);
			skb_queue_head_init(&ddata->mb[i].tx_skbs);
		}
	}

	return 0;

err_probe:
	for (j = i - 1; j >= 0; j--)
		if (ddata->mb[j].chan)
			mbox_free_channel(ddata->mb[j].chan);
	return -EPROBE_DEFER;
}

static void sunxi_rproc_free_mbox(struct rproc *rproc)
{
	struct sunxi_rproc *ddata = rproc->priv;
	unsigned int i;
	struct sk_buff *skb;

	for (i = 0; i < MBOX_NB_MBX; i++) {
		if (ddata->mb[i].chan) {
			mbox_free_channel(ddata->mb[i].chan);

			while (!skb_queue_empty(&ddata->mb[i].tx_skbs)) {
				skb = skb_dequeue(&ddata->mb[i].tx_skbs);
				kfree_skb(skb);
			}
		}
		ddata->mb[i].chan = NULL;
	}
}

static int sunxi_rproc_assert(struct rproc *rproc)
{
	struct sunxi_rproc *ddata = rproc->priv;
	int ret = 0;

	/* close dsp bus clk */
	/*
	clk_disable_unprepare(ddata->mclk);
	clk_disable_unprepare(data->bclk);
	*/

	ret = reset_control_assert(ddata->rst);
	if (ret != 0)
		return -ENXIO;

	ret = reset_control_assert(ddata->cfg);
	if (ret != 0)
		return -ENXIO;

	ret = reset_control_assert(ddata->dbg);
	if (ret != 0)
		return -ENXIO;

	return ret;
}

static void sunxi_rproc_set_runstall(void __iomem *base_reg, u32 value)
{
	u32 reg_val;

	reg_val = readl(base_reg + DSP_CTRL_REG0);
	reg_val &= ~(1 << BIT_RUN_STALL);
	reg_val |= (value << BIT_RUN_STALL);
	writel(reg_val, (base_reg + DSP_CTRL_REG0));
}

static void sunxi_rproc_set_localram(void __iomem *base_reg, u32 value)
{
	u32 reg_val;

	reg_val = readl(base_reg);
	reg_val &= ~(1 << BIT_SRAM_REMAP_ENABLE);
	reg_val |= (value << BIT_SRAM_REMAP_ENABLE);
	writel(reg_val, (base_reg));
}

static int sunxi_rproc_start(struct rproc *rproc)
{
	int ret = 0;
	int rate = 0;
	u32 reg_val = 0;
	struct device *dev = rproc->dev.parent;
	struct sunxi_rproc *ddata = rproc->priv;

	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);

	if (xplorer_debug) {
		dev_dbg(dev, "(%s:%d) dsp does not need to reset clk\n",
				__func__, __LINE__);
		return 0;
	}

	/* set dts of resets to assert  */
	ret = sunxi_rproc_assert(rproc);
	if (ret < 0) {
		dev_err(dev, "rproc assert err\n");
		return ret;
	}

	/* set pclk */
	ret = clk_prepare_enable(ddata->pclk);
	if (ret < 0) {
		dev_err(dev, "pll clk enable err\n");
		return ret;
	}

	ret = clk_set_parent(ddata->mclk, ddata->pclk);
	if (ret < 0) {
		dev_err(dev, "set mod clk parent to pclk err\n");
		return ret;
	}

	/* set mclk_freq */
	rate = clk_round_rate(ddata->mclk, ddata->mclk_freq);
	ret = clk_set_rate(ddata->mclk, rate);
	if (ret < 0) {
		dev_err(dev, "set mod clk freq err\n");
		return ret;
	}

	/* set mclk */
	ret = clk_prepare_enable(ddata->mclk);
	if (ret < 0) {
		dev_err(dev, "mod clk enable err\n");
		return ret;
	}

	/* set bus */
	ret = clk_prepare_enable(ddata->bclk);
	if (ret < 0) {
		dev_err(dev, "bus clk enable err\n");
		return ret;
	}

	/* set ahbs */
	ret = clk_prepare_enable(ddata->aclk);
	if (ret < 0) {
		dev_err(dev, "ahbs clk enable err\n");
		return ret;
	}

	/* set cfg to deassert  */
	ret = reset_control_deassert(ddata->cfg);
	if (ret < 0) {
		dev_err(dev, "set cfg to deassert err\n");
		return ret;
	}

	/* set dbg to deassert  */
	ret = reset_control_deassert(ddata->dbg);
	if (ret < 0) {
		dev_err(dev, "set dbg to deassert err\n");
		return ret;
	}

	/* set vector */
	writel(ddata->pc_entry, (ddata->dsp_cfg + DSP_ALT_RESET_VEC_REG));

	/* set statVactorSel */
	reg_val = readl(ddata->dsp_cfg + DSP_CTRL_REG0);
	reg_val |= 1 << BIT_START_VEC_SEL;
	writel(reg_val, (ddata->dsp_cfg + DSP_CTRL_REG0));

	/* set runstall */
	sunxi_rproc_set_runstall(ddata->dsp_cfg, 1);

	/* set dsp clken */
	reg_val = readl(ddata->dsp_cfg + DSP_CTRL_REG0);
	reg_val |= 1 << BIT_DSP_CLKEN;
	writel(reg_val, (ddata->dsp_cfg + DSP_CTRL_REG0));


	/* set rst to deassert  */
	ret = reset_control_deassert(ddata->rst);
	if (ret < 0) {
		dev_err(dev, "set rst to deassert err\n");
		return ret;
	}

	/* dsp can use local ram */
	sunxi_rproc_set_localram(ddata->sram_remap, 0);

	/* dsp run */
	sunxi_rproc_set_runstall(ddata->dsp_cfg, 0);

	return ret;
}

static int sunxi_rproc_stop(struct rproc *rproc)
{
	int ret = 0;
	struct device *dev = rproc->dev.parent;

	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);

	if (xplorer_debug) {
		dev_dbg(dev, "(%s:%d) dsp does not need to close clk\n",
				__func__, __LINE__);
		return 0;
	}

	ret = sunxi_rproc_assert(rproc);
	if (ret < 0) {
		dev_err(dev, "rproc assert err\n");
		return ret;
	}

	return 0;
}

static int sunxi_rproc_parse_fw(struct rproc *rproc, const struct firmware *fw)
{
	int ret = 0;
	struct sunxi_rproc *ddata = rproc->priv;
	struct elf32_hdr *ehdr  = (struct elf32_hdr *)fw->data;
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;
	struct of_phandle_iterator it;
	struct rproc_mem_entry *mem;
	struct reserved_mem *rmem;
	int index = 0;
	u64 da;

	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);

	ret = of_phandle_iterator_init(&it, np, "memory-region", NULL, 0);
	if (ret < 0) {
		dev_err(dev, "memory-region iterator init fail %d\n", ret);
		return -ENODEV;
	}

	while (of_phandle_iterator_next(&it) == 0) {
		rmem = of_reserved_mem_lookup(it.node);
		if (!rmem) {
			dev_err(dev, "unable to acquire memory-region\n");
			return -EINVAL;
		}

		ret = sunxi_rproc_pa_to_da(rproc, rmem->base, &da);
		if (ret < 0) {
			dev_err(dev, "memory region not valid: %pa\n", &rmem->base);
			return -EINVAL;
		}

		/* No need to map vdev buffer */
		if (0 == strcmp(it.node->name, "vdev0buffer")) {
			mem = rproc_of_resm_mem_entry_init(dev, index,
							   rmem->size,
							   da,
							   it.node->name);
			/*
			 * The rproc_of_resm_mem_entry_init didn't save the
			 * physical address. Here we save it manually.
			 */
			if (mem)
				mem->dma = (dma_addr_t)rmem->base;
		} else {
			mem = rproc_mem_entry_init(dev, NULL,
						   (dma_addr_t)rmem->base,
						   rmem->size, da,
						   sunxi_rproc_mem_alloc,
						   sunxi_rproc_mem_release,
						   it.node->name);
			if (mem)
				rproc_coredump_add_segment(rproc, da,
							   rmem->size);
		}

		if (!mem)
			return -ENOMEM;

		rproc_add_carveout(rproc, mem);
		index++;
	}

	ddata->pc_entry = ehdr->e_entry;

	/* check segment name, such as .resource_table */
	ret = rproc_elf_load_rsc_table(rproc, fw);
	if (ret != 0) {
		rproc->cached_table = NULL;
		rproc->table_ptr = NULL;
		rproc->table_sz = 0;
		dev_warn(&rproc->dev, "no resource table found for this firmware\n");
	}

	return ret;
}

static void sunxi_rproc_kick(struct rproc *rproc, int vqid)
{
	struct sunxi_rproc *ddata = rproc->priv;
	unsigned int i;
	int err;

	dev_dbg(&rproc->dev, "(%s:%d)\n", __func__, __LINE__);

	if (WARN_ON(vqid >= MBOX_NB_VQ))
		return;

	for (i = 0; i < MBOX_NB_MBX; i++) {
		struct sunxi_mbox *mb = &ddata->mb[i];
		struct sk_buff *skb;
		void *p_mb_msg;
		/*
		 * Because of the implementation of sunxi mailbox controller,
		 * the type of mailbox message should be u32.
		 */
		u32 msg = vqid;
		int msg_len = sizeof(u32);

		if (vqid != mb->vq_id)
			continue;
		if (!mb->chan)
			return;

		/*
		 * Since we need to send the address of message to mailbox
		 * controller, and mailbox will store the address in a internal
		 * buffer, then perhaps use it asynchronously. In case the
		 * address has become invalid when it is used, we allocate a
		 * socket buffer to store the message.
		 *
		 * Remember to free the socket buffer in mailbox tx_done callback.
		 */
		skb = alloc_skb(msg_len, GFP_ATOMIC);
		if (!skb)
			return;
		p_mb_msg = skb_put_data(skb, (void *)&msg, msg_len);
		spin_lock(&mb->tx_skbs_lock);
		skb_queue_tail(&mb->tx_skbs, skb);
		spin_unlock(&mb->tx_skbs_lock);
		err = mbox_send_message(ddata->mb[i].chan, p_mb_msg);
		if (err < 0) {
			dev_err(&rproc->dev, "(%s:%d) kick failed (%s, err:%d)\n",
				__func__, __LINE__, ddata->mb[i].name, err);
			spin_lock(&mb->tx_skbs_lock);
			skb_dequeue_tail(&mb->tx_skbs);
			spin_unlock(&mb->tx_skbs_lock);
			kfree_skb(skb);
		}
		return;
	}
}

static int sunxi_rproc_elf_find_segments(struct rproc *rproc,
					 const struct firmware *fw,
					 const char *find_name,
					 struct elf32_shdr **find_shdr,
					 struct elf32_phdr **find_phdr)
{
	struct device *dev = &rproc->dev;
	struct elf32_hdr *ehdr;
	struct elf32_shdr *shdr;
	struct elf32_phdr *phdr;
	const char *name_table;
	const u8 *elf_data = fw->data;
	u32 i, j, size;

	ehdr = (struct elf32_hdr *)elf_data;
	shdr = (struct elf32_shdr *)(elf_data + ehdr->e_shoff);
	phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);

	name_table = elf_data + shdr[ehdr->e_shstrndx].sh_offset;
	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		size = shdr->sh_size;

		if (strcmp(name_table + shdr->sh_name, find_name))
			continue;

		*find_shdr = shdr;
		dev_dbg(dev, "(%s:%d) %s addr 0x%x, size 0x%x\n",
			__func__, __LINE__, find_name, shdr->sh_addr, size);

		for (j = 0; j < ehdr->e_phnum; j++, phdr++) {
			if (shdr->sh_addr == phdr->p_paddr) {
				*find_phdr = phdr;
				dev_dbg(dev, "(%s:%d) find %s phdr\n",
					__func__, __LINE__, find_name);
				return 0;
			}
		}
	}

	return -EINVAL;

}

static int sunxi_rproc_elf_load_segments(struct rproc *rproc, const struct firmware *fw)
{
	struct device *dev = &rproc->dev;
	struct sunxi_rproc *ddata = rproc->priv;
	struct elf32_hdr *ehdr;
	struct elf32_phdr *phdr;
	struct elf32_shdr *shdr;
	int i, ret = 0;
	const u8 *elf_data = fw->data;
	u32 offset, da, memsz, filesz;
	void *ptr;

	/* we must copy .resource_table, when use xplorer to debug */
	if (xplorer_debug) {
		dev_dbg(dev, "(%s:%d) only load .resource_table data\n",
				__func__, __LINE__);

		ret = sunxi_rproc_elf_find_segments(rproc, fw, ".resource_table", &shdr, &phdr);
		if (ret < 0)
			dev_err(dev, "(%s:%d) find segments err\n", __func__, __LINE__);

		da = phdr->p_paddr;
		memsz = phdr->p_memsz;
		filesz = phdr->p_filesz;
		ptr = rproc_da_to_va(rproc, da, memsz);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%x mem 0x%x\n", da, memsz);
			return -EINVAL;
		}
		if (phdr->p_filesz)
			memcpy(ptr, elf_data + phdr->p_offset, filesz);

		if (memsz > filesz)
			memset(ptr + filesz, 0, memsz - filesz);

		return 0;
	}

	/* get ehdr & phdr addr */
	ehdr = (struct elf32_hdr *)elf_data;
	phdr = (struct elf32_phdr *)(elf_data + ehdr->e_phoff);

	/* arm can write/read local ram */
	sunxi_rproc_set_localram(ddata->sram_remap, 1);

	dev_dbg(dev, "(%s:%d)\n", __func__, __LINE__);

	/* go through the available ELF segments */
	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		da = phdr->p_paddr;
		memsz = phdr->p_memsz;
		filesz = phdr->p_filesz;
		offset = phdr->p_offset;

		if (phdr->p_type != PT_LOAD)
			continue;

		dev_dbg(dev, "phdr: type %d da 0x%x memsz 0x%x filesz 0x%x\n",
			phdr->p_type, da, memsz, filesz);

		if ((memsz == 0) || (filesz == 0))
			continue;

		if (filesz > memsz) {
			dev_err(dev, "bad phdr filesz 0x%x memsz 0x%x\n",
				filesz, memsz);
			ret = -EINVAL;
			break;
		}

		if (offset + filesz > fw->size) {
			dev_err(dev, "truncated fw: need 0x%x avail 0x%zx\n",
				offset + filesz, fw->size);
			ret = -EINVAL;
			break;
		}

		/* grab the kernel address for this device address */
		ptr = rproc_da_to_va(rproc, da, memsz);
		if (!ptr) {
			dev_err(dev, "bad phdr da 0x%x mem 0x%x\n", da, memsz);
			ret = -EINVAL;
			break;
		}

		/* put the segment where the remote processor expects it */
		if (phdr->p_filesz)
			memcpy(ptr, elf_data + phdr->p_offset, filesz);

		/*
		 * Zero out remaining memory for this segment.
		 *
		 * This isn't strictly required since dma_alloc_coherent already
		 * did this for us. albeit harmless, we may consider removing
		 * this.
		 */
		if (memsz > filesz)
			memset(ptr + filesz, 0, memsz - filesz);
	}

	return ret;
}

static struct resource_table *sunxi_rproc_rsc_table(struct rproc *rproc,
					     const struct firmware *fw)
{
	dev_dbg(&rproc->dev, "(%s:%d)\n", __func__, __LINE__);
	return rproc_elf_find_loaded_rsc_table(rproc, fw);
}

static u32 suxni_rproc_elf_get_boot_addr(struct rproc *rproc, const struct firmware *fw)
{
	u32 data = 0;
	data = rproc_elf_get_boot_addr(rproc, fw);
	dev_dbg(&rproc->dev, "(%s:%d) elf boot addr = 0x%x\n", __func__, __LINE__, data);
	return data;
}

static struct rproc_ops sunxi_rproc_ops = {
	.start		= sunxi_rproc_start,
	.stop		= sunxi_rproc_stop,
	.da_to_va	= sunxi_rproc_da_to_va,
	.kick		= sunxi_rproc_kick,
	.parse_fw	= sunxi_rproc_parse_fw,
	.find_loaded_rsc_table = sunxi_rproc_rsc_table,
	.load		= sunxi_rproc_elf_load_segments,
	.get_boot_addr	= suxni_rproc_elf_get_boot_addr,
};

static const struct of_device_id sunxi_rproc_match[] = {
	{ .compatible = "allwinner,sun8iw20p1-dsp-rproc" },
	{},
};

static int sunxi_rproc_resource_get(struct rproc *rproc, struct platform_device *pdev)
{
	struct device *dev = rproc->dev.parent;
	struct device_node *np = dev->of_node;
	struct sunxi_rproc *ddata = rproc->priv;
	struct resource *res;
	u32 *map_array;
	int ret = 0;
	int i;

	ddata->pclk = devm_clk_get(dev, "pll");
	if (IS_ERR_OR_NULL(ddata->pclk)) {
		dev_err(dev, "no find pll in dts\n");
		return -ENXIO;
	}

	ddata->mclk = devm_clk_get(dev, "mod");
	if (IS_ERR_OR_NULL(ddata->mclk)) {
		dev_err(dev, "no find mod in dts\n");
		return -ENXIO;
	}

	ddata->bclk = devm_clk_get(dev, "bus");
	if (IS_ERR_OR_NULL(ddata->bclk)) {
		dev_err(dev, "no find bus in dts\n");
		return -ENXIO;
	}

	ddata->aclk = devm_clk_get(dev, "ahbs");
	if (IS_ERR_OR_NULL(ddata->aclk)) {
		dev_err(dev, "no find ahbs in dts\n");
		return -ENXIO;
	}

	ddata->rst = devm_reset_control_get(dev, "rst");
	if (IS_ERR_OR_NULL(ddata->rst)) {
		dev_err(dev, "no find rst in dts\n");
		return -ENXIO;
	}

	ddata->cfg = devm_reset_control_get(dev, "cfg");
	if (IS_ERR_OR_NULL(ddata->cfg)) {
		dev_err(dev, "no find cfg in dts\n");
		return -ENXIO;
	}

	ddata->dbg = devm_reset_control_get(dev, "dbg");
	if (IS_ERR_OR_NULL(ddata->dbg)) {
		dev_err(dev, "no find dbg in dts\n");
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "sram-remap");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find sram-remap in dts\n");
		return -ENXIO;
	}

	ddata->sram_remap = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(ddata->sram_remap)) {
		dev_err(dev, "fail to ioremap sram-remap\n");
		return -ENXIO;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "dsp-cfg");
	if (IS_ERR_OR_NULL(res)) {
		dev_err(dev, "no find dsp-cfg in dts\n");
		return -ENXIO;
	}

	ddata->dsp_cfg = devm_ioremap_resource(dev, res);
	if (IS_ERR_OR_NULL(ddata->dsp_cfg)) {
		dev_err(dev, "fail to ioremap dsp-cfg\n");
		return -ENXIO;
	}

	ret = of_property_read_u32(np, "clock-frequency", &ddata->mclk_freq);
	if (ret < 0) {
		dev_err(dev, "fail to get clock-frequency\n");
		return -ENXIO;
	}

	dev_dbg(dev, "(%s:%d) ddata->mclk_freq = %d\n", __func__, __LINE__, ddata->mclk_freq);

	ret = of_property_count_elems_of_size(np, "memory-mappings", sizeof(u32) * 3);
	if (ret <= 0) {
		dev_err(dev, "fail to get memory-mappings\n");
		return -ENXIO;
	}
	ddata->mem_maps_cnt = ret;
	ddata->mem_maps = devm_kcalloc(dev, ddata->mem_maps_cnt,
				       sizeof(struct sunxi_rproc_memory_mapping),
				       GFP_KERNEL);
	if (!ddata->mem_maps)
		return -ENOMEM;

	map_array = devm_kcalloc(dev, ddata->mem_maps_cnt * 3, sizeof(u32), GFP_KERNEL);
	if (!map_array)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, "memory-mappings", map_array,
					 ddata->mem_maps_cnt * 3);
	if (ret < 0) {
		dev_err(dev, "fail to read memory-mappings\n");
		return -ENXIO;
	}

	for (i = 0; i < ddata->mem_maps_cnt; i++) {
		ddata->mem_maps[i].da = map_array[i * 3];
		ddata->mem_maps[i].len = map_array[i * 3 + 1];
		ddata->mem_maps[i].pa = map_array[i * 3 + 2];
		dev_dbg(dev, "memory-mappings[%d]: da: 0x%llx, len: 0x%llx, pa: 0x%llx\n",
			i, ddata->mem_maps[i].da, ddata->mem_maps[i].len,
			ddata->mem_maps[i].pa);
	}

	devm_kfree(dev, map_array);

	return 0;
}

static int sunxi_rproc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct sunxi_rproc *ddata;
	struct device_node *np = dev->of_node;
	struct rproc *rproc;
	int ret;

	dev_info(dev, "sunxi rproc driver %s\n", SUNXI_RPROC_VERSION);

	of_id = of_match_device(sunxi_rproc_match, dev);
	if (!of_id) {
		dev_err(dev, "No device of_id found\n");
		ret = -EINVAL;
		goto err_out;
	}

	rproc = rproc_alloc(dev, np->name, &sunxi_rproc_ops, NULL, sizeof(*ddata));
	if (!rproc) {
		ret = -ENOMEM;
		goto err_out;
	}

	ret = sunxi_rproc_resource_get(rproc, pdev);
	if (ret < 0) {
		dev_err(dev, "Failed to get resource\n");
		goto free_rproc;
	}

	rproc->has_iommu = false;
	rproc->auto_boot = false;
	ddata = rproc->priv;

	ddata->workqueue = create_workqueue(dev_name(dev));
	if (!ddata->workqueue) {
		dev_err(dev, "Cannot create workqueue\n");
		ret = -ENOMEM;
		goto free_rproc;
	}

	platform_set_drvdata(pdev, rproc);

	ret = sunxi_rproc_request_mbox(rproc);
	if (ret < 0) {
		dev_err(dev, "Request mbox failed\n");
		goto destroy_workqueue;
	}

	ret = rproc_add(rproc);
	if (ret < 0) {
		dev_err(dev, "Failed to register rproc\n");
		goto free_mbox;
	}

	dev_info(dev, "sunxi rproc driver init ok\n");

	return ret;

free_mbox:
	sunxi_rproc_free_mbox(rproc);
destroy_workqueue:
	destroy_workqueue(ddata->workqueue);
free_rproc:
	rproc_free(rproc);
err_out:
	return ret;
}

static int sunxi_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);
	struct sunxi_rproc *ddata = rproc->priv;

	if (atomic_read(&rproc->power) > 0)
		rproc_shutdown(rproc);

	rproc_del(rproc);

	sunxi_rproc_free_mbox(rproc);

	destroy_workqueue(ddata->workqueue);

	rproc_free(rproc);

	return 0;
}

static struct platform_driver sunxi_rproc_driver = {
	.probe = sunxi_rproc_probe,
	.remove = sunxi_rproc_remove,
	.driver = {
		.name = "sunxi-rproc", /* dev name */
		.of_match_table = of_match_ptr(sunxi_rproc_match),
	},
};
module_platform_driver(sunxi_rproc_driver);

MODULE_DESCRIPTION("Allwinnertech Remote Processor Control Driver");
MODULE_AUTHOR("wujiayi <wujiayi@allwinnertech.com>");
MODULE_LICENSE("GPL v2");

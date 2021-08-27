 
#ifndef AL_DMA_H
#define AL_DMA_H

#include <linux/dmaengine.h>
#include <linux/init.h>
#include <linux/dmapool.h>
#include <linux/cache.h>
#include <linux/circ_buf.h>
#include <linux/pci_ids.h>
#include <linux/pci.h>
#include <linux/interrupt.h>

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
#include "al_hal_ssm_raid.h"
#else
#include "al_hal_raid.h"
#endif

#define AL_DMA_VERSION  "0.01"

#define AL_DMA_IRQNAME_SIZE		40

#define AL_DMA_MAX_SIZE_SHIFT_MEMCPY	16	 
#define AL_DMA_MAX_SIZE_SHIFT_MEMSET	16	 
#define AL_DMA_MAX_SIZE_SHIFT_XOR	14	 
#define AL_DMA_MAX_SIZE_SHIFT_XOR_VAL	14	 
#define AL_DMA_MAX_SIZE_SHIFT_PQ	13	 
#define AL_DMA_MAX_SIZE_SHIFT_PQ_VAL	13	 

#define AL_DMA_ALIGN_SHIFT		0	 

#ifndef CONFIG_ALPINE_VP_WA
#define AL_DMA_RAID_TX_CDESC_SIZE	8
#define AL_DMA_RAID_RX_CDESC_SIZE	8
#else
 
#define AL_DMA_RAID_TX_CDESC_SIZE	16
#define AL_DMA_RAID_RX_CDESC_SIZE	16
#endif

#define AL_DMA_MAX_SIZE_MEMCPY		(1 << AL_DMA_MAX_SIZE_SHIFT_MEMCPY)
#define AL_DMA_MAX_SIZE_MEMSET		(1 << AL_DMA_MAX_SIZE_SHIFT_MEMSET)
#define AL_DMA_MAX_SIZE_XOR		(1 << AL_DMA_MAX_SIZE_SHIFT_XOR)
#define AL_DMA_MAX_SIZE_XOR_VAL		(1 << AL_DMA_MAX_SIZE_SHIFT_XOR_VAL)
#define AL_DMA_MAX_SIZE_PQ		(1 << AL_DMA_MAX_SIZE_SHIFT_PQ)
#define AL_DMA_MAX_SIZE_PQ_VAL		(1 << AL_DMA_MAX_SIZE_SHIFT_PQ_VAL)

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
#define AL_DMA_MAX_XOR			AL_SSM_MAX_SRC_DESCS
#else
#define AL_DMA_MAX_XOR			AL_RAID_MAX_SRC_DESCS
#endif

#define AL_DMA_OP_MAX_BLOCKS		(AL_DMA_MAX_XOR * 2)

#define AL_DMA_MAX_CHANNELS		4

#define AL_DMA_SW_RING_MIN_ORDER	4
#define AL_DMA_SW_RING_MAX_ORDER	16

#define AL_DMA_ISSUE_PNDNG_UPON_SUBMIT	1

#ifdef CONFIG_AL_DMA_STATS
#define AL_DMA_STATS_INC(var, incval)	{ (var) += (incval); }

#define AL_DMA_STATS_UPDATE(chan, num, cnt, size, size_inc)		\
{									\
	AL_DMA_STATS_INC((num), (cnt));					\
									\
	if (size_inc)							\
		AL_DMA_STATS_INC((size), (size_inc));			\
									\
	AL_DMA_STATS_INC(						\
		(chan)->stats_prep.matching_cpu,			\
		(cnt) * (((chan)->idx == smp_processor_id())));		\
									\
	AL_DMA_STATS_INC(						\
		(chan)->stats_prep.mismatching_cpu,			\
		(cnt) * (!((chan)->idx == smp_processor_id())));	\
}
#else
#define AL_DMA_STATS_INC(var, incval)
#define AL_DMA_STATS_UPDATE(chan, num, cnt, size, size_inc)
#endif

enum al_unmap_type {
	AL_UNMAP_SINGLE,
	AL_UNMAP_PAGE,
};

struct al_dma_unmap_info_ent {
	dma_addr_t	handle;
	size_t		size;
	int		dir;
	enum al_unmap_type type;
};

struct al_dma_sw_desc {
	struct al_raid_transaction hal_xaction;
	struct al_block blocks[AL_DMA_OP_MAX_BLOCKS];
	struct al_buf bufs[AL_DMA_OP_MAX_BLOCKS];

	size_t len;
	struct dma_async_tx_descriptor txd;
	#ifdef DEBUG
	int id;
	#endif

	int last_is_pq_val;
	enum sum_check_flags *pq_val_res;

	int last_is_xor_val;
	enum sum_check_flags *xor_val_res;

	int last_is_memcpy;

#ifdef AL_DMA_MEMCPY_VALIDATION
	void *memcpy_dest;
	void *memcpy_src;
	size_t memcpy_len;
#endif

	int last_is_xor;

#ifdef AL_DMA_XOR_VALIDATION
	void *xor_dest;
	int xor_src_cnt;
	void *xor_src[AL_DMA_OP_MAX_BLOCKS];
	size_t xor_len;
#endif

	struct al_dma_unmap_info_ent unmap_info[AL_DMA_OP_MAX_BLOCKS];
	int umap_ent_cnt;
};
#define to_al_dma_device(dev) container_of(dev, struct al_dma_device, common)
#define to_dev(al_dma_chan) (&(al_dma_chan)->device->pdev->dev)

#ifdef CONFIG_AL_DMA_STATS
 
struct al_dma_chan_stats_prep {
	uint64_t int_num;
	uint64_t memcpy_num;
	uint64_t memcpy_size;
	uint64_t sg_memcpy_num;
	uint64_t sg_memcpy_size;
	uint64_t memset_num;
	uint64_t memset_size;
	uint64_t xor_num;
	uint64_t xor_size;
	uint64_t pq_num;
	uint64_t pq_size;
	uint64_t pq_val_num;
	uint64_t pq_val_size;
	uint64_t xor_val_num;
	uint64_t xor_val_size;
	uint64_t matching_cpu;
	uint64_t mismatching_cpu;
};

struct al_dma_chan_stats_comp {
	uint64_t redundant_int_cnt;
	uint64_t matching_cpu;
	uint64_t mismatching_cpu;
};
#endif

struct al_dma_irq {
	char name[AL_DMA_IRQNAME_SIZE];
};

struct al_dma_device {
	struct pci_dev			*pdev;
	u16				dev_id;
	u8				rev_id;

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	struct al_ssm_dma_params	ssm_dma_params;
#else
	struct al_raid_dma_params	raid_dma_params;
#endif
	void __iomem			*udma_regs_base;
	void __iomem			*app_regs_base;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	struct al_ssm_dma		hal_raid;
#else
	struct al_raid_dma		hal_raid;
#endif

	struct dma_device		common;

	struct msix_entry		msix_entries[AL_DMA_MAX_CHANNELS];
	struct al_dma_irq		irq_tbl[AL_DMA_MAX_CHANNELS];
	struct al_dma_chan		*channels[AL_DMA_MAX_CHANNELS];
	int				max_channels;

	struct kmem_cache		*cache;
};

struct al_dma_chan {
	 
	struct dma_chan common		____cacheline_aligned;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	struct al_ssm_dma *hal_raid;
#else
	struct al_raid_dma *hal_raid;
#endif
	int	idx;
	struct al_dma_device *device;
	cpumask_t affinity_mask;

	struct al_dma_sw_desc **sw_ring;

	int tx_descs_num;  
	void *tx_dma_desc_virt;  
	dma_addr_t tx_dma_desc;

	int rx_descs_num;  
	void *rx_dma_desc_virt;  
	dma_addr_t rx_dma_desc;
	void *rx_dma_cdesc_virt;  
	dma_addr_t rx_dma_cdesc;

	struct kobject kobj;

	u16 alloc_order;

	spinlock_t prep_lock		____cacheline_aligned;
	u16 head;
	int sw_desc_num_locked;
	uint32_t tx_desc_produced;
#ifdef CONFIG_AL_DMA_STATS
	struct al_dma_chan_stats_prep stats_prep;
#endif

	spinlock_t cleanup_lock		____cacheline_aligned_in_smp;
	struct tasklet_struct cleanup_task;
	dma_cookie_t completed_cookie;
	u16 tail;
#ifdef CONFIG_AL_DMA_STATS
	struct al_dma_chan_stats_comp stats_comp;
#endif
};

static inline u16 al_dma_ring_size(struct al_dma_chan *chan)
{
	return 1 << chan->alloc_order;
}

static inline u16 al_dma_ring_active(struct al_dma_chan *chan)
{
	return CIRC_CNT(chan->head, chan->tail, al_dma_ring_size(chan));
}

static inline u16 al_dma_ring_space(struct al_dma_chan *chan)
{
	return CIRC_SPACE(chan->head, chan->tail, al_dma_ring_size(chan));
}

static inline struct al_dma_sw_desc  *
al_dma_get_ring_ent(struct al_dma_chan *chan, u16 idx)
{
	return chan->sw_ring[idx & (al_dma_ring_size(chan) - 1)];
}

struct al_dma_sysfs_entry {
	struct attribute attr;
	ssize_t (*show)(struct dma_chan *, char *);
};

static inline struct al_dma_chan *to_al_dma_chan(struct dma_chan *c)
{
	return container_of(c, struct al_dma_chan, common);
}

#ifdef DEBUG
#define set_desc_id(desc, i) ((desc)->id = (i))
#define desc_id(desc) ((desc)->id)
#else
#define set_desc_id(desc, i)
#define desc_id(desc) (0)
#endif

static inline struct al_dma_chan *
al_dma_chan_by_index(struct al_dma_device *device, int index)
{
	return device->channels[index];
}

static inline u32 al_dma_chansts(struct al_dma_chan *chan)
{
	u32 status = 0;

	return status;
}

static inline void al_dma_unmap_info_ent_set(
	struct al_dma_unmap_info_ent	*ent,
	dma_addr_t			handle,
	size_t				size,
	int				dir,
	enum al_unmap_type		type)
{
	ent->handle = handle;
	ent->size = size;
	ent->dir = dir;
	ent->type = type;
}

int al_dma_get_sw_desc_lock(
	struct al_dma_chan	*chan,
	int			num);

int al_dma_core_init(
	struct al_dma_device	*device,
	void __iomem		*iobase_udma,
	void __iomem		*iobase_app);

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
int al_dma_fast_init(
	struct al_dma_device	*device,
	void __iomem		*iobase_udma);

int al_dma_fast_terminate(
	struct al_dma_device	*device);
#endif

int al_dma_core_terminate(
	struct al_dma_device	*device);

int al_dma_cleanup_fn(
	struct al_dma_chan	*chan,
	int			from_tasklet);

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
int udma_fast_memcpy(int len, al_phys_addr_t src, al_phys_addr_t dst);
#endif

void al_dma_tx_submit_sw_cond_unlock(
	struct al_dma_chan		*chan,
	struct dma_async_tx_descriptor	*tx);

void al_dma_kobject_add(struct al_dma_device *device, struct kobj_type *type);
void al_dma_kobject_del(struct al_dma_device *device);
extern const struct sysfs_ops al_dma_sysfs_ops;
extern struct al_dma_sysfs_entry al_dma_version_attr;
extern struct al_dma_sysfs_entry al_dma_cap_attr;

#endif  

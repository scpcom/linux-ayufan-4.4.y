#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/err.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <asm/cacheflush.h>
#include <asm/hardware/cache-l2x0.h>
#include "cache-aurora-l2.h"

#define CACHE_LINE_SIZE		32

static void __iomem *l2x0_base;
static DEFINE_RAW_SPINLOCK(l2x0_lock);
static u32 l2x0_way_mask;	 
static u32 l2x0_size;
#if defined(CONFIG_SYNO_LSP_HI3536)
static u32 l2x0_cache_id;
static unsigned int l2x0_sets;
static unsigned int l2x0_ways;
#endif  
static unsigned long sync_reg_offset = L2X0_CACHE_SYNC;

static u32  cache_id_part_number_from_dt;

struct l2x0_regs l2x0_saved_regs;

struct l2x0_of_data {
	void (*setup)(const struct device_node *, u32 *, u32 *);
	void (*save)(void);
	struct outer_cache_fns outer_cache;
};

static bool of_init = false;

#if defined(CONFIG_SYNO_LSP_HI3536)
static inline bool is_pl310_rev(int rev)
{
	return (l2x0_cache_id &
		(L2X0_CACHE_ID_PART_MASK | L2X0_CACHE_ID_REV_MASK)) ==
			(L2X0_CACHE_ID_PART_L310 | rev);
}
#endif  

static inline void cache_wait_way(void __iomem *reg, unsigned long mask)
{
	 
	while (readl_relaxed(reg) & mask)
		cpu_relax();
}

#ifdef CONFIG_CACHE_PL310
static inline void cache_wait(void __iomem *reg, unsigned long mask)
{
	 
}
#else
#define cache_wait	cache_wait_way
#endif

static inline void cache_sync(void)
{
	void __iomem *base = l2x0_base;

	writel_relaxed(0, base + sync_reg_offset);
	cache_wait(base + L2X0_CACHE_SYNC, 1);
}

static inline void l2x0_clean_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
}

static inline void l2x0_inv_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}

#if defined(CONFIG_PL310_ERRATA_588369) || defined(CONFIG_PL310_ERRATA_727915)
static inline void debug_writel(unsigned long val)
{
	if (outer_cache.set_debug)
		outer_cache.set_debug(val);
}

static void pl310_set_debug(unsigned long val)
{
	writel_relaxed(val, l2x0_base + L2X0_DEBUG_CTRL);
}
#else
 
static inline void debug_writel(unsigned long val)
{
}

#define pl310_set_debug	NULL
#endif

#ifdef CONFIG_PL310_ERRATA_588369
static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;

	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_LINE_PA);
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_INV_LINE_PA);
}
#else

static inline void l2x0_flush_line(unsigned long addr)
{
	void __iomem *base = l2x0_base;
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	writel_relaxed(addr, base + L2X0_CLEAN_INV_LINE_PA);
}
#endif

static void l2x0_cache_sync(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

#if defined(CONFIG_SYNO_LSP_HI3536)
#ifdef CONFIG_PL310_ERRATA_727915
static void l2x0_for_each_set_way(void __iomem *reg)
{
	int set;
	int way;
	unsigned long flags;

	for (way = 0; way < l2x0_ways; way++) {
		raw_spin_lock_irqsave(&l2x0_lock, flags);
		for (set = 0; set < l2x0_sets; set++)
			writel_relaxed((way << 28) | (set << 5), reg);
		cache_sync();
		raw_spin_unlock_irqrestore(&l2x0_lock, flags);
	}
}
#endif
#endif  

static void __l2x0_flush_all(void)
{
	debug_writel(0x03);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_INV_WAY, l2x0_way_mask);
	cache_sync();
	debug_writel(0x00);
}

static void l2x0_flush_all(void)
{
	unsigned long flags;

#if defined(CONFIG_SYNO_LSP_HI3536)
#ifdef CONFIG_PL310_ERRATA_727915
	if (is_pl310_rev(REV_PL310_R2P0)) {
		l2x0_for_each_set_way(l2x0_base + L2X0_CLEAN_INV_LINE_IDX);
		return;
	}
#endif
#endif  

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	__l2x0_flush_all();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_clean_all(void)
{
	unsigned long flags;

#if defined(CONFIG_SYNO_LSP_HI3536)
#ifdef CONFIG_PL310_ERRATA_727915
	if (is_pl310_rev(REV_PL310_R2P0)) {
		l2x0_for_each_set_way(l2x0_base + L2X0_CLEAN_LINE_IDX);
		return;
	}
#endif
#endif  

	raw_spin_lock_irqsave(&l2x0_lock, flags);
#if defined(CONFIG_SYNO_LSP_HI3536)
	debug_writel(0x03);
#endif  
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_CLEAN_WAY);
	cache_wait_way(l2x0_base + L2X0_CLEAN_WAY, l2x0_way_mask);
	cache_sync();
#if defined(CONFIG_SYNO_LSP_HI3536)
	debug_writel(0x00);
#endif  
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_inv_all(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	 
	BUG_ON(readl(l2x0_base + L2X0_CTRL) & L2X0_CTRL_EN);
	writel_relaxed(l2x0_way_mask, l2x0_base + L2X0_INV_WAY);
	cache_wait_way(l2x0_base + L2X0_INV_WAY, l2x0_way_mask);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_inv_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	if (start & (CACHE_LINE_SIZE - 1)) {
		start &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(start);
		debug_writel(0x00);
		start += CACHE_LINE_SIZE;
	}

	if (end & (CACHE_LINE_SIZE - 1)) {
		end &= ~(CACHE_LINE_SIZE - 1);
		debug_writel(0x03);
		l2x0_flush_line(end);
		debug_writel(0x00);
	}

	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_inv_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_INV_LINE_PA, 1);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_clean_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;

	if ((end - start) >= l2x0_size) {
		l2x0_clean_all();
		return;
	}

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		while (start < blk_end) {
			l2x0_clean_line(start);
			start += CACHE_LINE_SIZE;
		}

		if (blk_end < end) {
			raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_CLEAN_LINE_PA, 1);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_flush_range(unsigned long start, unsigned long end)
{
	void __iomem *base = l2x0_base;
	unsigned long flags;

	if ((end - start) >= l2x0_size) {
		l2x0_flush_all();
		return;
	}

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	start &= ~(CACHE_LINE_SIZE - 1);
	while (start < end) {
		unsigned long blk_end = start + min(end - start, 4096UL);

		debug_writel(0x03);
		while (start < blk_end) {
			l2x0_flush_line(start);
			start += CACHE_LINE_SIZE;
		}
		debug_writel(0x00);

		if (blk_end < end) {
			raw_spin_unlock_irqrestore(&l2x0_lock, flags);
			raw_spin_lock_irqsave(&l2x0_lock, flags);
		}
	}
	cache_wait(base + L2X0_CLEAN_INV_LINE_PA, 1);
	cache_sync();
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_disable(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	__l2x0_flush_all();
	writel_relaxed(0, l2x0_base + L2X0_CTRL);
#if defined (MY_DEF_HERE)
	dsb(st);
#else  
	dsb();
#endif  
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);
}

static void l2x0_unlock(u32 cache_id)
{
	int lockregs;
	int i;
#if defined (MY_DEF_HERE)
	u32 way_mask = 0;
#endif  

	switch (cache_id & L2X0_CACHE_ID_PART_MASK) {
	case L2X0_CACHE_ID_PART_L310:
		lockregs = 8;
		break;
	case AURORA_CACHE_ID:
		lockregs = 4;
		break;
	default:
		 
		lockregs = 1;
		break;
	}

#if defined (MY_DEF_HERE)
	 
	if (of_machine_is_compatible("st,stih301"))
		way_mask = 0x0F;

	for (i = 0; i < lockregs; i++) {
		writel_relaxed(way_mask, l2x0_base + L2X0_LOCKDOWN_WAY_D_BASE +
			       i * L2X0_LOCKDOWN_STRIDE);
		writel_relaxed(way_mask, l2x0_base + L2X0_LOCKDOWN_WAY_I_BASE +
			       i * L2X0_LOCKDOWN_STRIDE);
	}
#else  
	for (i = 0; i < lockregs; i++) {
		writel_relaxed(0x0, l2x0_base + L2X0_LOCKDOWN_WAY_D_BASE +
			       i * L2X0_LOCKDOWN_STRIDE);
		writel_relaxed(0x0, l2x0_base + L2X0_LOCKDOWN_WAY_I_BASE +
			       i * L2X0_LOCKDOWN_STRIDE);
	}
#endif  
}

void __init l2x0_init(void __iomem *base, u32 aux_val, u32 aux_mask)
{
	u32 aux;
#if defined(CONFIG_SYNO_LSP_HI3536)
	u32 way_size = 0;
#else  
	u32 cache_id;
	u32 way_size = 0;
	int ways;
#endif  
	int way_size_shift = L2X0_WAY_SIZE_SHIFT;
	const char *type;
#if defined (MY_DEF_HERE)
#if defined (MY_DEF_HERE)
#else  
	u32 pf;
#endif  
#endif   

	l2x0_base = base;
#if defined(CONFIG_SYNO_LSP_HI3536)
	if (cache_id_part_number_from_dt)
		l2x0_cache_id = cache_id_part_number_from_dt;
	else
		l2x0_cache_id = readl_relaxed(l2x0_base + L2X0_CACHE_ID);
#else  
	if (cache_id_part_number_from_dt)
		cache_id = cache_id_part_number_from_dt;
	else
		cache_id = readl_relaxed(l2x0_base + L2X0_CACHE_ID);
#endif  
	aux = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);

	aux &= aux_mask;
	aux |= aux_val;

#if defined(CONFIG_SYNO_LSP_HI3536)
	switch (l2x0_cache_id & L2X0_CACHE_ID_PART_MASK) {
#else  
	switch (cache_id & L2X0_CACHE_ID_PART_MASK) {
#endif  
	case L2X0_CACHE_ID_PART_L310:
#if defined(CONFIG_SYNO_LSP_HI3536)
		if (aux & (1 << 16))
			l2x0_ways = 16;
		else
			l2x0_ways = 8;
#else  
		if (aux & (1 << 16))
			ways = 16;
		else
			ways = 8;
#endif  
		type = "L310";
#ifdef CONFIG_PL310_ERRATA_753970
		 
		sync_reg_offset = L2X0_DUMMY_REG;
#endif
#if defined(CONFIG_SYNO_LSP_HI3536)
		if ((l2x0_cache_id & L2X0_CACHE_ID_RTL_MASK) <= L2X0_CACHE_ID_RTL_R3P0)
#else  
		if ((cache_id & L2X0_CACHE_ID_RTL_MASK) <= L2X0_CACHE_ID_RTL_R3P0)
#endif  
			outer_cache.set_debug = pl310_set_debug;
		break;
	case L2X0_CACHE_ID_PART_L210:
#if defined(CONFIG_SYNO_LSP_HI3536)
		l2x0_ways = (aux >> 13) & 0xf;
#else  
		ways = (aux >> 13) & 0xf;
#endif  
		type = "L210";
		break;

	case AURORA_CACHE_ID:
		sync_reg_offset = AURORA_SYNC_REG;
#if defined(CONFIG_SYNO_LSP_HI3536)
		l2x0_ways = (aux >> 13) & 0xf;
		l2x0_ways = 2 << ((l2x0_ways + 1) >> 2);
#else  
		ways = (aux >> 13) & 0xf;
		ways = 2 << ((ways + 1) >> 2);
#endif  
		way_size_shift = AURORA_WAY_SIZE_SHIFT;
		type = "Aurora";
		break;
	default:
		 
#if defined(CONFIG_SYNO_LSP_HI3536)
		l2x0_ways = 8;
#else  
		ways = 8;
#endif  
		type = "L2x0 series";
		break;
	}

#if defined(CONFIG_SYNO_LSP_HI3536)
	l2x0_way_mask = (1 << l2x0_ways) - 1;
#else  
	l2x0_way_mask = (1 << ways) - 1;
#endif  

	way_size = (aux & L2X0_AUX_CTRL_WAY_SIZE_MASK) >> 17;
#if defined(CONFIG_SYNO_LSP_HI3536)
	way_size = SZ_1K << (way_size + way_size_shift);

	l2x0_size = l2x0_ways * way_size;
	l2x0_sets = way_size / CACHE_LINE_SIZE;
#else  
	way_size = 1 << (way_size + way_size_shift);

	l2x0_size = ways * way_size * SZ_1K;
#endif  

#if defined (MY_DEF_HERE)
	 
	if (of_machine_is_compatible("st,stih301"))
		l2x0_size = 512 * SZ_1K;
#endif  

#if defined (MY_DEF_HERE)
	 
	writel(0x3, l2x0_base + L2X0_POWER_CTRL);
#endif  
	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & L2X0_CTRL_EN)) {
		 
#if defined(CONFIG_SYNO_LSP_HI3536)
		l2x0_unlock(l2x0_cache_id);
#else  
		l2x0_unlock(cache_id);
#endif  

		writel_relaxed(aux, l2x0_base + L2X0_AUX_CTRL);

		l2x0_inv_all();

		writel_relaxed(L2X0_CTRL_EN, l2x0_base + L2X0_CTRL);
	}

#if defined (MY_DEF_HERE)
#if defined (MY_DEF_HERE)
	 
	writel_relaxed(0x58800000, l2x0_base + L2X0_PREFETCH_CTRL);
#else  
	pf = readl_relaxed(l2x0_base + L2X0_PREFETCH_CTRL);
	pf |= 0xf | BIT(24) | BIT(27) | BIT(30);
	writel_relaxed(pf, l2x0_base + L2X0_PREFETCH_CTRL);
	pf = readl_relaxed(l2x0_base + L2X0_PREFETCH_CTRL);
#endif  
#endif   

	aux = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);

	l2x0_saved_regs.aux_ctrl = aux;
#if defined (MY_DEF_HERE)
	l2x0_saved_regs.pwr_ctrl = readl_relaxed(l2x0_base + L2X0_POWER_CTRL);
#endif  

	if (!of_init) {
		outer_cache.inv_range = l2x0_inv_range;
		outer_cache.clean_range = l2x0_clean_range;
		outer_cache.flush_range = l2x0_flush_range;
		outer_cache.sync = l2x0_cache_sync;
		outer_cache.flush_all = l2x0_flush_all;
		outer_cache.inv_all = l2x0_inv_all;
		outer_cache.disable = l2x0_disable;
	}

	printk(KERN_INFO "%s cache controller enabled\n", type);
	printk(KERN_INFO "l2x0: %d ways, CACHE_ID 0x%08x, AUX_CTRL 0x%08x, Cache size: %d B\n",
#if defined(CONFIG_SYNO_LSP_HI3536)
			l2x0_ways, l2x0_cache_id, aux, l2x0_size);
#else  
			ways, cache_id, aux, l2x0_size);
#endif  
}

#ifdef CONFIG_OF
static int l2_wt_override;

static unsigned long calc_range_end(unsigned long start, unsigned long end)
{
	 
	if (end > start + MAX_RANGE_SIZE)
		end = start + MAX_RANGE_SIZE;

	if (end > PAGE_ALIGN(start+1))
		end = PAGE_ALIGN(start+1);

	return end;
}

static void aurora_pa_range(unsigned long start, unsigned long end,
			unsigned long offset)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&l2x0_lock, flags);
	writel_relaxed(start, l2x0_base + AURORA_RANGE_BASE_ADDR_REG);
	writel_relaxed(end, l2x0_base + offset);
	raw_spin_unlock_irqrestore(&l2x0_lock, flags);

	cache_sync();
}

static void aurora_inv_range(unsigned long start, unsigned long end)
{
	 
	start &= ~(CACHE_LINE_SIZE - 1);
	end = ALIGN(end, CACHE_LINE_SIZE);

	while (start < end) {
		unsigned long range_end = calc_range_end(start, end);
		aurora_pa_range(start, range_end - CACHE_LINE_SIZE,
				AURORA_INVAL_RANGE_REG);
		start = range_end;
	}
}

static void aurora_clean_range(unsigned long start, unsigned long end)
{
	 
	if (!l2_wt_override) {
		start &= ~(CACHE_LINE_SIZE - 1);
		end = ALIGN(end, CACHE_LINE_SIZE);
		while (start != end) {
			unsigned long range_end = calc_range_end(start, end);
			aurora_pa_range(start, range_end - CACHE_LINE_SIZE,
					AURORA_CLEAN_RANGE_REG);
			start = range_end;
		}
	}
}

static void aurora_flush_range(unsigned long start, unsigned long end)
{
	start &= ~(CACHE_LINE_SIZE - 1);
	end = ALIGN(end, CACHE_LINE_SIZE);
	while (start != end) {
		unsigned long range_end = calc_range_end(start, end);
		 
		if (l2_wt_override)
			aurora_pa_range(start, range_end - CACHE_LINE_SIZE,
							AURORA_INVAL_RANGE_REG);
		else
			aurora_pa_range(start, range_end - CACHE_LINE_SIZE,
							AURORA_FLUSH_RANGE_REG);
		start = range_end;
	}
}

static void __init l2x0_of_setup(const struct device_node *np,
				 u32 *aux_val, u32 *aux_mask)
{
	u32 data[2] = { 0, 0 };
	u32 tag = 0;
	u32 dirty = 0;
	u32 val = 0, mask = 0;

	of_property_read_u32(np, "arm,tag-latency", &tag);
	if (tag) {
		mask |= L2X0_AUX_CTRL_TAG_LATENCY_MASK;
		val |= (tag - 1) << L2X0_AUX_CTRL_TAG_LATENCY_SHIFT;
	}

	of_property_read_u32_array(np, "arm,data-latency",
				   data, ARRAY_SIZE(data));
	if (data[0] && data[1]) {
		mask |= L2X0_AUX_CTRL_DATA_RD_LATENCY_MASK |
			L2X0_AUX_CTRL_DATA_WR_LATENCY_MASK;
		val |= ((data[0] - 1) << L2X0_AUX_CTRL_DATA_RD_LATENCY_SHIFT) |
		       ((data[1] - 1) << L2X0_AUX_CTRL_DATA_WR_LATENCY_SHIFT);
	}

	of_property_read_u32(np, "arm,dirty-latency", &dirty);
	if (dirty) {
		mask |= L2X0_AUX_CTRL_DIRTY_LATENCY_MASK;
		val |= (dirty - 1) << L2X0_AUX_CTRL_DIRTY_LATENCY_SHIFT;
	}

	*aux_val &= ~mask;
	*aux_val |= val;
	*aux_mask &= ~mask;
}

static void __init pl310_of_setup(const struct device_node *np,
				  u32 *aux_val, u32 *aux_mask)
{
	u32 data[3] = { 0, 0, 0 };
	u32 tag[3] = { 0, 0, 0 };
	u32 filter[2] = { 0, 0 };

	of_property_read_u32_array(np, "arm,tag-latency", tag, ARRAY_SIZE(tag));
	if (tag[0] && tag[1] && tag[2])
		writel_relaxed(
			((tag[0] - 1) << L2X0_LATENCY_CTRL_RD_SHIFT) |
			((tag[1] - 1) << L2X0_LATENCY_CTRL_WR_SHIFT) |
			((tag[2] - 1) << L2X0_LATENCY_CTRL_SETUP_SHIFT),
			l2x0_base + L2X0_TAG_LATENCY_CTRL);

	of_property_read_u32_array(np, "arm,data-latency",
				   data, ARRAY_SIZE(data));
	if (data[0] && data[1] && data[2])
		writel_relaxed(
			((data[0] - 1) << L2X0_LATENCY_CTRL_RD_SHIFT) |
			((data[1] - 1) << L2X0_LATENCY_CTRL_WR_SHIFT) |
			((data[2] - 1) << L2X0_LATENCY_CTRL_SETUP_SHIFT),
			l2x0_base + L2X0_DATA_LATENCY_CTRL);

	of_property_read_u32_array(np, "arm,filter-ranges",
				   filter, ARRAY_SIZE(filter));
	if (filter[1]) {
		writel_relaxed(ALIGN(filter[0] + filter[1], SZ_1M),
			       l2x0_base + L2X0_ADDR_FILTER_END);
		writel_relaxed((filter[0] & ~(SZ_1M - 1)) | L2X0_ADDR_FILTER_EN,
			       l2x0_base + L2X0_ADDR_FILTER_START);
	}
}

static void __init pl310_save(void)
{
	u32 l2x0_revision = readl_relaxed(l2x0_base + L2X0_CACHE_ID) &
		L2X0_CACHE_ID_RTL_MASK;

	l2x0_saved_regs.tag_latency = readl_relaxed(l2x0_base +
		L2X0_TAG_LATENCY_CTRL);
	l2x0_saved_regs.data_latency = readl_relaxed(l2x0_base +
		L2X0_DATA_LATENCY_CTRL);
	l2x0_saved_regs.filter_end = readl_relaxed(l2x0_base +
		L2X0_ADDR_FILTER_END);
	l2x0_saved_regs.filter_start = readl_relaxed(l2x0_base +
		L2X0_ADDR_FILTER_START);

	if (l2x0_revision >= L2X0_CACHE_ID_RTL_R2P0) {
		 
		l2x0_saved_regs.prefetch_ctrl = readl_relaxed(l2x0_base +
			L2X0_PREFETCH_CTRL);
		 
		if (l2x0_revision >= L2X0_CACHE_ID_RTL_R3P0)
			l2x0_saved_regs.pwr_ctrl = readl_relaxed(l2x0_base +
				L2X0_POWER_CTRL);
	}
}

static void aurora_save(void)
{
	l2x0_saved_regs.ctrl = readl_relaxed(l2x0_base + L2X0_CTRL);
	l2x0_saved_regs.aux_ctrl = readl_relaxed(l2x0_base + L2X0_AUX_CTRL);
}

static void l2x0_resume(void)
{
	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & L2X0_CTRL_EN)) {
		 
		l2x0_unlock(readl_relaxed(l2x0_base + L2X0_CACHE_ID));

		writel_relaxed(l2x0_saved_regs.aux_ctrl, l2x0_base +
			L2X0_AUX_CTRL);

		l2x0_inv_all();

		writel_relaxed(L2X0_CTRL_EN, l2x0_base + L2X0_CTRL);
	}
}

static void pl310_resume(void)
{
	u32 l2x0_revision;

	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & L2X0_CTRL_EN)) {
		 
		writel_relaxed(l2x0_saved_regs.tag_latency,
			l2x0_base + L2X0_TAG_LATENCY_CTRL);
		writel_relaxed(l2x0_saved_regs.data_latency,
			l2x0_base + L2X0_DATA_LATENCY_CTRL);
		writel_relaxed(l2x0_saved_regs.filter_end,
			l2x0_base + L2X0_ADDR_FILTER_END);
		writel_relaxed(l2x0_saved_regs.filter_start,
			l2x0_base + L2X0_ADDR_FILTER_START);

		l2x0_revision = readl_relaxed(l2x0_base + L2X0_CACHE_ID) &
			L2X0_CACHE_ID_RTL_MASK;

		if (l2x0_revision >= L2X0_CACHE_ID_RTL_R2P0) {
			writel_relaxed(l2x0_saved_regs.prefetch_ctrl,
				l2x0_base + L2X0_PREFETCH_CTRL);
			if (l2x0_revision >= L2X0_CACHE_ID_RTL_R3P0)
				writel_relaxed(l2x0_saved_regs.pwr_ctrl,
					l2x0_base + L2X0_POWER_CTRL);
		}
	}

	l2x0_resume();
}

static void aurora_resume(void)
{
	if (!(readl(l2x0_base + L2X0_CTRL) & L2X0_CTRL_EN)) {
		writel_relaxed(l2x0_saved_regs.aux_ctrl,
				l2x0_base + L2X0_AUX_CTRL);
		writel_relaxed(l2x0_saved_regs.ctrl, l2x0_base + L2X0_CTRL);
	}
}

static void __init aurora_broadcast_l2_commands(void)
{
	__u32 u;
	 
	__asm__ __volatile__("mrc p15, 1, %0, c15, c2, 0" : "=r"(u));
	u |= AURORA_CTRL_FW;		 
	__asm__ __volatile__("mcr p15, 1, %0, c15, c2, 0\n" : : "r"(u));
	isb();
}

static void __init aurora_of_setup(const struct device_node *np,
				u32 *aux_val, u32 *aux_mask)
{
	u32 val = AURORA_ACR_REPLACEMENT_TYPE_SEMIPLRU;
	u32 mask =  AURORA_ACR_REPLACEMENT_MASK;

	of_property_read_u32(np, "cache-id-part",
			&cache_id_part_number_from_dt);

	l2_wt_override = of_property_read_bool(np, "wt-override");

	if (l2_wt_override) {
		val |= AURORA_ACR_FORCE_WRITE_THRO_POLICY;
		mask |= AURORA_ACR_FORCE_WRITE_POLICY_MASK;
	}

	*aux_val &= ~mask;
	*aux_val |= val;
	*aux_mask &= ~mask;
}

static const struct l2x0_of_data pl310_data = {
	.setup = pl310_of_setup,
	.save  = pl310_save,
	.outer_cache = {
		.resume      = pl310_resume,
		.inv_range   = l2x0_inv_range,
		.clean_range = l2x0_clean_range,
		.flush_range = l2x0_flush_range,
		.sync        = l2x0_cache_sync,
		.flush_all   = l2x0_flush_all,
		.inv_all     = l2x0_inv_all,
		.disable     = l2x0_disable,
	},
};

static const struct l2x0_of_data l2x0_data = {
	.setup = l2x0_of_setup,
	.save  = NULL,
	.outer_cache = {
		.resume      = l2x0_resume,
		.inv_range   = l2x0_inv_range,
		.clean_range = l2x0_clean_range,
		.flush_range = l2x0_flush_range,
		.sync        = l2x0_cache_sync,
		.flush_all   = l2x0_flush_all,
		.inv_all     = l2x0_inv_all,
		.disable     = l2x0_disable,
	},
};

static const struct l2x0_of_data aurora_with_outer_data = {
	.setup = aurora_of_setup,
	.save  = aurora_save,
	.outer_cache = {
		.resume      = aurora_resume,
		.inv_range   = aurora_inv_range,
		.clean_range = aurora_clean_range,
		.flush_range = aurora_flush_range,
		.sync        = l2x0_cache_sync,
		.flush_all   = l2x0_flush_all,
		.inv_all     = l2x0_inv_all,
		.disable     = l2x0_disable,
	},
};

static const struct l2x0_of_data aurora_no_outer_data = {
	.setup = aurora_of_setup,
	.save  = aurora_save,
	.outer_cache = {
		.resume      = aurora_resume,
	},
};

static const struct of_device_id l2x0_ids[] __initconst = {
	{ .compatible = "arm,pl310-cache", .data = (void *)&pl310_data },
	{ .compatible = "arm,l220-cache", .data = (void *)&l2x0_data },
	{ .compatible = "arm,l210-cache", .data = (void *)&l2x0_data },
	{ .compatible = "marvell,aurora-system-cache",
	  .data = (void *)&aurora_no_outer_data},
	{ .compatible = "marvell,aurora-outer-cache",
	  .data = (void *)&aurora_with_outer_data},
	{}
};

#if defined(MY_ABC_HERE)
int __init l2x0_of_init_common(u32 aux_val, u32 aux_mask, bool is_coherent)
#else  
int __init l2x0_of_init(u32 aux_val, u32 aux_mask)
#endif  
{
	struct device_node *np;
	const struct l2x0_of_data *data;
	struct resource res;

	np = of_find_matching_node(NULL, l2x0_ids);
	if (!np)
		return -ENODEV;

	if (of_address_to_resource(np, 0, &res))
		return -ENODEV;

	l2x0_base = ioremap(res.start, resource_size(&res));
	if (!l2x0_base)
		return -ENOMEM;

	l2x0_saved_regs.phy_base = res.start;

	data = of_match_node(l2x0_ids, np)->data;

	if (!(readl_relaxed(l2x0_base + L2X0_CTRL) & L2X0_CTRL_EN)) {
		if (data->setup)
			data->setup(np, &aux_val, &aux_mask);

		if (data == &aurora_no_outer_data)
			aurora_broadcast_l2_commands();
	}

	if (data->save)
		data->save();

	of_init = true;
	memcpy(&outer_cache, &data->outer_cache, sizeof(outer_cache));

#if defined(MY_ABC_HERE)
	 
	if (of_device_is_compatible(np, "arm,pl310-cache")) {
		u32 l2x0_revision = readl_relaxed(l2x0_base + L2X0_CACHE_ID) &
			L2X0_CACHE_ID_RTL_MASK;
		if (l2x0_revision >= L2X0_CACHE_ID_RTL_R3P2 && is_coherent)
			outer_cache.sync = NULL;
	}
#endif  

	l2x0_init(l2x0_base, aux_val, aux_mask);

	return 0;
}

#if defined(MY_ABC_HERE)
int __init l2x0_of_init(u32 aux_val, u32 aux_mask)
{
	return l2x0_of_init_common(aux_val, aux_mask, false);
}

int __init l2x0_of_init_coherent(u32 aux_val, u32 aux_mask)
{
	return l2x0_of_init_common(aux_val, aux_mask, true);
}
#endif  

#endif

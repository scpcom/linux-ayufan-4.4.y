#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#if defined(MY_ABC_HERE)
 
#else  
 
#endif  
 
#if defined(MY_ABC_HERE)
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#endif  

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mbus.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/debugfs.h>
#if defined(MY_ABC_HERE)
#include <linux/syscore_ops.h>
#include <linux/log2.h>

#ifdef MBUS_DEBUG
#define dprintk(a...) printk(a)
#else
#define dprintk(a...)
#endif
#endif  

#define TARGET_DDR		0

#define WIN_CTRL_OFF		0x0000
#define   WIN_CTRL_ENABLE       BIT(0)
#define   WIN_CTRL_TGT_MASK     0xf0
#define   WIN_CTRL_TGT_SHIFT    4
#define   WIN_CTRL_ATTR_MASK    0xff00
#define   WIN_CTRL_ATTR_SHIFT   8
#define   WIN_CTRL_SIZE_MASK    0xffff0000
#define   WIN_CTRL_SIZE_SHIFT   16
#define WIN_BASE_OFF		0x0004
#define   WIN_BASE_LOW          0xffff0000
#define   WIN_BASE_HIGH         0xf
#define WIN_REMAP_LO_OFF	0x0008
#define   WIN_REMAP_LOW         0xffff0000
#define WIN_REMAP_HI_OFF	0x000c

#define ATTR_HW_COHERENCY	(0x1 << 4)

#define DDR_BASE_CS_OFF(n)	(0x0000 + ((n) << 3))
#define  DDR_BASE_CS_HIGH_MASK  0xf
#define  DDR_BASE_CS_LOW_MASK   0xff000000
#define DDR_SIZE_CS_OFF(n)	(0x0004 + ((n) << 3))
#define  DDR_SIZE_ENABLED       BIT(0)
#define  DDR_SIZE_CS_MASK       0x1c
#define  DDR_SIZE_CS_SHIFT      2
#define  DDR_SIZE_MASK          0xff000000

#define DOVE_DDR_BASE_CS_OFF(n) ((n) << 4)

#if defined(MY_ABC_HERE)
 
#define MBUS_BRIDGE_CTRL_OFF	0x0
#define  MBUS_BRIDGE_SIZE_MASK  0xffff0000
#define MBUS_BRIDGE_BASE_OFF	0x4
#define  MBUS_BRIDGE_BASE_MASK  0xffff0000

#define MBUS_WINS_MAX		20
#else  
struct mvebu_mbus_mapping {
	const char *name;
	u8 target;
	u8 attr;
	u8 attrmask;
};

#define MAPDEF_NOMASK       0xff
#define MAPDEF_PCIMASK      0xf7
#define MAPDEF_ORIONPCIMASK 0xd7

#define MAPDEF(__n, __t, __a, __m) \
	{ .name = __n, .target = __t, .attr = __a, .attrmask = __m }
#endif  

struct mvebu_mbus_state;

#if defined(MY_ABC_HERE)
struct mvebu_mbus_soc_data {
	unsigned int num_wins;
	unsigned int (*win_cfg_offset)(const int win);
	unsigned int (*win_remap_offset)(const int win);
	bool has_mbus_bridge;
	void (*setup_cpu_target)(struct mvebu_mbus_state *s);
	int (*save_cpu_target)(struct mvebu_mbus_state *s,
			       u32 *store_addr);
	int (*show_cpu_target)(struct mvebu_mbus_state *s,
			       struct seq_file *seq, void *v);
};
 
struct mvebu_mbus_win_data {
	u32 ctrl;
	u32 base;
	u32 remap_lo;
	u32 remap_hi;
};
#else  
struct mvebu_mbus_soc_data {
	unsigned int num_wins;
	unsigned int num_remappable_wins;
	unsigned int (*win_cfg_offset)(const int win);
	void (*setup_cpu_target)(struct mvebu_mbus_state *s);
	int (*show_cpu_target)(struct mvebu_mbus_state *s,
			   struct seq_file *seq, void *v);
	const struct mvebu_mbus_mapping *map;
};
#endif  

struct mvebu_mbus_state {
	void __iomem *mbuswins_base;
	void __iomem *sdramwins_base;
#if defined(MY_ABC_HERE)
	void __iomem *mbusbridge_base;
	phys_addr_t sdramwins_phys_base;
#endif  
	struct dentry *debugfs_root;
	struct dentry *debugfs_sdram;
	struct dentry *debugfs_devs;
#if defined(MY_ABC_HERE)
	struct resource pcie_mem_aperture;
	struct resource pcie_io_aperture;
#endif  
	const struct mvebu_mbus_soc_data *soc;
	int hw_io_coherency;

#if defined(MY_ABC_HERE)
	 
	u32 mbus_bridge_ctrl;
	u32 mbus_bridge_base;
	struct mvebu_mbus_win_data wins[MBUS_WINS_MAX];
#endif  
};

static struct mvebu_mbus_state mbus_state;

static struct mbus_dram_target_info mvebu_mbus_dram_info;
const struct mbus_dram_target_info *mv_mbus_dram_info(void)
{
	return &mvebu_mbus_dram_info;
}
EXPORT_SYMBOL_GPL(mv_mbus_dram_info);

#if defined(MY_ABC_HERE)
 
static bool mvebu_mbus_window_is_remappable(struct mvebu_mbus_state *mbus,
					    const int win)
{
	unsigned int offset = mbus->soc->win_remap_offset(win);

	return offset != MVEBU_MBUS_NO_REMAP;
}
#endif  

static void mvebu_mbus_read_window(struct mvebu_mbus_state *mbus,
				   int win, int *enabled, u64 *base,
				   u32 *size, u8 *target, u8 *attr,
				   u64 *remap)
{
	void __iomem *addr = mbus->mbuswins_base +
		mbus->soc->win_cfg_offset(win);
	u32 basereg = readl(addr + WIN_BASE_OFF);
	u32 ctrlreg = readl(addr + WIN_CTRL_OFF);

	if (!(ctrlreg & WIN_CTRL_ENABLE)) {
		*enabled = 0;
		return;
	}

	*enabled = 1;
	*base = ((u64)basereg & WIN_BASE_HIGH) << 32;
	*base |= (basereg & WIN_BASE_LOW);
	*size = (ctrlreg | ~WIN_CTRL_SIZE_MASK) + 1;

	if (target)
		*target = (ctrlreg & WIN_CTRL_TGT_MASK) >> WIN_CTRL_TGT_SHIFT;

	if (attr)
		*attr = (ctrlreg & WIN_CTRL_ATTR_MASK) >> WIN_CTRL_ATTR_SHIFT;

	if (remap) {
#if defined(MY_ABC_HERE)
		if (mvebu_mbus_window_is_remappable(mbus, win)) {
			u32 remap_low, remap_hi;
			addr = mbus->mbuswins_base +
					mbus->soc->win_remap_offset(win);
			remap_low = readl(addr + WIN_REMAP_LO_OFF);
			remap_hi  = readl(addr + WIN_REMAP_HI_OFF);
#else  
		if (win < mbus->soc->num_remappable_wins) {
			u32 remap_low = readl(addr + WIN_REMAP_LO_OFF);
			u32 remap_hi  = readl(addr + WIN_REMAP_HI_OFF);
#endif  
			*remap = ((u64)remap_hi << 32) | remap_low;
		} else
			*remap = 0;
	}
}

static void mvebu_mbus_disable_window(struct mvebu_mbus_state *mbus,
				      int win)
{
	void __iomem *addr;

	addr = mbus->mbuswins_base + mbus->soc->win_cfg_offset(win);

	writel(0, addr + WIN_BASE_OFF);
	writel(0, addr + WIN_CTRL_OFF);

#if defined(MY_ABC_HERE)
	if (mvebu_mbus_window_is_remappable(mbus, win)) {
		addr = mbus->mbuswins_base + mbus->soc->win_remap_offset(win);
#else  
	if (win < mbus->soc->num_remappable_wins) {
#endif  
		writel(0, addr + WIN_REMAP_LO_OFF);
		writel(0, addr + WIN_REMAP_HI_OFF);
	}
}

static int mvebu_mbus_window_is_free(struct mvebu_mbus_state *mbus,
				     const int win)
{
	void __iomem *addr = mbus->mbuswins_base +
		mbus->soc->win_cfg_offset(win);
	u32 ctrl = readl(addr + WIN_CTRL_OFF);

	if (win == 13)
		return false;

	return !(ctrl & WIN_CTRL_ENABLE);
}

static int mvebu_mbus_window_conflicts(struct mvebu_mbus_state *mbus,
				       phys_addr_t base, size_t size,
				       u8 target, u8 attr)
{
	u64 end = (u64)base + size;
	int win;

	for (win = 0; win < mbus->soc->num_wins; win++) {
		u64 wbase, wend;
		u32 wsize;
		u8 wtarget, wattr;
		int enabled;

		mvebu_mbus_read_window(mbus, win,
				       &enabled, &wbase, &wsize,
				       &wtarget, &wattr, NULL);

		if (!enabled)
			continue;

		wend = wbase + wsize;

		if ((u64)base < wend && end > wbase)
			return 0;
	}

	return 1;
}

static int mvebu_mbus_find_window(struct mvebu_mbus_state *mbus,
				  phys_addr_t base, size_t size)
{
	int win;

	for (win = 0; win < mbus->soc->num_wins; win++) {
		u64 wbase;
		u32 wsize;
		int enabled;

		mvebu_mbus_read_window(mbus, win,
				       &enabled, &wbase, &wsize,
				       NULL, NULL, NULL);

		if (!enabled)
			continue;

		if (base == wbase && size == wsize)
			return win;
	}

	return -ENODEV;
}

static int mvebu_mbus_setup_window(struct mvebu_mbus_state *mbus,
				   int win, phys_addr_t base, size_t size,
				   phys_addr_t remap, u8 target,
				   u8 attr)
{
	void __iomem *addr = mbus->mbuswins_base +
		mbus->soc->win_cfg_offset(win);
#if defined(MY_ABC_HERE)
	void __iomem *addr_rmp = mbus->mbuswins_base +
		mbus->soc->win_remap_offset(win);
#endif  
	u32 ctrl, remap_addr;

#if defined(MY_ABC_HERE)
	if (!is_power_of_2(size)) {
		WARN(true, "Invalid MBus window size: 0x%zx\n", size);
		return -EINVAL;
	}

	if ((base & (phys_addr_t)(size - 1)) != 0) {
		WARN(true, "Invalid MBus base/size: %pa len 0x%zx\n", &base,
		     size);
		return -EINVAL;
	}
#endif  

	ctrl = ((size - 1) & WIN_CTRL_SIZE_MASK) |
		(attr << WIN_CTRL_ATTR_SHIFT)    |
		(target << WIN_CTRL_TGT_SHIFT)   |
		WIN_CTRL_ENABLE;

	writel(base & WIN_BASE_LOW, addr + WIN_BASE_OFF);
	writel(ctrl, addr + WIN_CTRL_OFF);

#if defined(MY_ABC_HERE)
	if (mvebu_mbus_window_is_remappable(mbus, win)) {
#else  
	if (win < mbus->soc->num_remappable_wins) {
#endif  
		if (remap == MVEBU_MBUS_NO_REMAP)
			remap_addr = base;
		else
			remap_addr = remap;
#if defined(MY_ABC_HERE)
		writel(remap_addr & WIN_REMAP_LOW, addr_rmp + WIN_REMAP_LO_OFF);
		writel(0, addr_rmp + WIN_REMAP_HI_OFF);
#else  
		writel(remap_addr & WIN_REMAP_LOW, addr + WIN_REMAP_LO_OFF);
		writel(0, addr + WIN_REMAP_HI_OFF);
#endif  
	}

#if defined(MY_ABC_HERE)
	dprintk("== %s: decoding window ==\n", __func__);
	dprintk("base_phys: 0x%x, base_low 0x%x, ctrl 0x%x, remap_addr 0x%x\n",
	    base, base & WIN_BASE_LOW, ctrl, remap_addr);

	dprintk("base_addr: %p, ctrl_addr: %p\n",
	    addr + WIN_BASE_OFF, addr + WIN_CTRL_OFF);
#endif  

	return 0;
}

static int mvebu_mbus_alloc_window(struct mvebu_mbus_state *mbus,
				   phys_addr_t base, size_t size,
				   phys_addr_t remap, u8 target,
				   u8 attr)
{
	int win;

	if (remap == MVEBU_MBUS_NO_REMAP) {
#if defined(MY_ABC_HERE)
		for (win = 0; win < mbus->soc->num_wins; win++) {
			if (mvebu_mbus_window_is_remappable(mbus, win))
				continue;
#else  
		for (win = mbus->soc->num_remappable_wins;
		     win < mbus->soc->num_wins; win++)
#endif  

			if (mvebu_mbus_window_is_free(mbus, win))
				return mvebu_mbus_setup_window(mbus, win, base,
							       size, remap,
							       target, attr);
#if defined(MY_ABC_HERE)
		}
#endif  
	}

#if defined(MY_ABC_HERE)
	for (win = 0; win < mbus->soc->num_wins; win++) {
		 
		if ((remap != MVEBU_MBUS_NO_REMAP) &&
		    (!mvebu_mbus_window_is_remappable(mbus, win)))
			continue;
#else  
	for (win = 0; win < mbus->soc->num_wins; win++)
#endif  
		if (mvebu_mbus_window_is_free(mbus, win))
			return mvebu_mbus_setup_window(mbus, win, base, size,
						       remap, target, attr);
#if defined(MY_ABC_HERE)
	}
#endif  

	return -ENOMEM;
}

static int mvebu_sdram_debug_show_orion(struct mvebu_mbus_state *mbus,
					struct seq_file *seq, void *v)
{
	int i;

	for (i = 0; i < 4; i++) {
		u32 basereg = readl(mbus->sdramwins_base + DDR_BASE_CS_OFF(i));
		u32 sizereg = readl(mbus->sdramwins_base + DDR_SIZE_CS_OFF(i));
		u64 base;
		u32 size;

		if (!(sizereg & DDR_SIZE_ENABLED)) {
			seq_printf(seq, "[%d] disabled\n", i);
			continue;
		}

		base = ((u64)basereg & DDR_BASE_CS_HIGH_MASK) << 32;
		base |= basereg & DDR_BASE_CS_LOW_MASK;
		size = (sizereg | ~DDR_SIZE_MASK);

		seq_printf(seq, "[%d] %016llx - %016llx : cs%d\n",
			   i, (unsigned long long)base,
			   (unsigned long long)base + size + 1,
			   (sizereg & DDR_SIZE_CS_MASK) >> DDR_SIZE_CS_SHIFT);
	}

	return 0;
}

static int mvebu_sdram_debug_show_dove(struct mvebu_mbus_state *mbus,
				       struct seq_file *seq, void *v)
{
	int i;

	for (i = 0; i < 2; i++) {
		u32 map = readl(mbus->sdramwins_base + DOVE_DDR_BASE_CS_OFF(i));
		u64 base;
		u32 size;

		if (!(map & 1)) {
			seq_printf(seq, "[%d] disabled\n", i);
			continue;
		}

		base = map & 0xff800000;
		size = 0x100000 << (((map & 0x000f0000) >> 16) - 4);

		seq_printf(seq, "[%d] %016llx - %016llx : cs%d\n",
			   i, (unsigned long long)base,
			   (unsigned long long)base + size, i);
	}

	return 0;
}

static int mvebu_sdram_debug_show(struct seq_file *seq, void *v)
{
	struct mvebu_mbus_state *mbus = &mbus_state;
	return mbus->soc->show_cpu_target(mbus, seq, v);
}

static int mvebu_sdram_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mvebu_sdram_debug_show, inode->i_private);
}

static const struct file_operations mvebu_sdram_debug_fops = {
	.open = mvebu_sdram_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int mvebu_devs_debug_show(struct seq_file *seq, void *v)
{
	struct mvebu_mbus_state *mbus = &mbus_state;
	int win;

	for (win = 0; win < mbus->soc->num_wins; win++) {
		u64 wbase, wremap;
		u32 wsize;
		u8 wtarget, wattr;
#if defined(MY_ABC_HERE)
		int enabled;
#else  
		int enabled, i;
		const char *name;
#endif  

		mvebu_mbus_read_window(mbus, win,
				       &enabled, &wbase, &wsize,
				       &wtarget, &wattr, &wremap);

		if (!enabled) {
			seq_printf(seq, "[%02d] disabled\n", win);
			continue;
		}

#if defined(MY_ABC_HERE)
		seq_printf(seq, "[%02d] %016llx - %016llx : %04x:%04x",
			   win, (unsigned long long)wbase,
			   (unsigned long long)(wbase + wsize), wtarget, wattr);

		if (!is_power_of_2(wsize) ||
		    ((wbase & (u64)(wsize - 1)) != 0))
			seq_puts(seq, " (Invalid base/size!!)");

		if (mvebu_mbus_window_is_remappable(mbus, win)) {
#else  
		for (i = 0; mbus->soc->map[i].name; i++)
			if (mbus->soc->map[i].target == wtarget &&
			    mbus->soc->map[i].attr ==
			    (wattr & mbus->soc->map[i].attrmask))
				break;

		name = mbus->soc->map[i].name ?: "unknown";

		seq_printf(seq, "[%02d] %016llx - %016llx : %s",
			   win, (unsigned long long)wbase,
			   (unsigned long long)(wbase + wsize), name);

		if (win < mbus->soc->num_remappable_wins) {
#endif  
			seq_printf(seq, " (remap %016llx)\n",
				   (unsigned long long)wremap);
		} else
			seq_printf(seq, "\n");
	}

	return 0;
}

static int mvebu_devs_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, mvebu_devs_debug_show, inode->i_private);
}

static const struct file_operations mvebu_devs_debug_fops = {
	.open = mvebu_devs_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#if defined(MY_ABC_HERE)
static unsigned int generic_mbus_win_cfg_offset(int win)
#else  
static unsigned int orion_mbus_win_offset(int win)
#endif  
{
	return win << 4;
}

#if defined(MY_ABC_HERE)
static unsigned int generic_mbus_win_remap_2_offset(int win)
{
	if (win < 2)
		return generic_mbus_win_cfg_offset(win);
	else
		return MVEBU_MBUS_NO_REMAP;
}

static unsigned int generic_mbus_win_remap_4_offset(int win)
{
	if (win < 4)
		return generic_mbus_win_cfg_offset(win);
	else
		return MVEBU_MBUS_NO_REMAP;
}

static unsigned int generic_mbus_win_remap_8_offset(int win)
{
	if (win < 8)
		return generic_mbus_win_cfg_offset(win);
	else
		return MVEBU_MBUS_NO_REMAP;
}

static unsigned int armada_xp_mbus_win_remap_offset(int win)
{
	if (win < 8)
		return win << 4;
	else if (win == 13)
		return 0xF0 - WIN_REMAP_LO_OFF;
	else
		return MVEBU_MBUS_NO_REMAP;
}
#endif  

#if defined(MY_ABC_HERE)
static unsigned int armada_370_xp_mbus_win_cfg_offset(int win)
#else  
static unsigned int armada_370_xp_mbus_win_offset(int win)
#endif  
{
	 
	if (win < 8)
		return win << 4;
	else
		return 0x90 + ((win - 8) << 3);
}

#if defined(MY_ABC_HERE)
static unsigned int mv78xx0_mbus_win_cfg_offset(int win)
#else  
static unsigned int mv78xx0_mbus_win_offset(int win)
#endif  
{
	if (win < 8)
		return win << 4;
	else
		return 0x900 + ((win - 8) << 4);
}

static void __init
mvebu_mbus_default_setup_cpu_target(struct mvebu_mbus_state *mbus)
{
	int i;
	int cs;
#if defined(MY_ABC_HERE)
	struct mvebu_mbus_state *s = &mbus_state;
	u32 mbus_bridge_base = 0, mbus_bridge_size = 0;
	u64 mbus_bridge_end = 0;

	if (s->mbusbridge_base) {
		mbus_bridge_base =
			(readl(s->mbusbridge_base + MBUS_BRIDGE_BASE_OFF) &
			 MBUS_BRIDGE_BASE_MASK);
		mbus_bridge_size =
			(readl(s->mbusbridge_base + MBUS_BRIDGE_CTRL_OFF) |
			 ~MBUS_BRIDGE_SIZE_MASK) + 1;
		mbus_bridge_end = (u64)mbus_bridge_base + mbus_bridge_size;
	}
#endif  

	mvebu_mbus_dram_info.mbus_dram_target_id = TARGET_DDR;

	for (i = 0, cs = 0; i < 4; i++) {
#if defined(MY_ABC_HERE)
		u64 base = readl(mbus->sdramwins_base + DDR_BASE_CS_OFF(i));
		u64 size = readl(mbus->sdramwins_base + DDR_SIZE_CS_OFF(i));
		u64 end;
		struct mbus_dram_window *w;

		if (!(size & DDR_SIZE_ENABLED))
			continue;

		if (base & DDR_BASE_CS_HIGH_MASK)
			continue;

		base = base & DDR_BASE_CS_LOW_MASK;
		size = (size | ~DDR_SIZE_MASK) + 1;
		end = base + size;

		if (s->mbusbridge_base) {
			 
			if (base >= mbus_bridge_base && end <= mbus_bridge_end)
				continue;

			if (base >= mbus_bridge_base && end > mbus_bridge_end) {
				pr_info(" ==> 1\n");
				size -= mbus_bridge_end - base;
				base = mbus_bridge_end;
			}

			if (base < mbus_bridge_base && end > mbus_bridge_base)
				size -= end - mbus_bridge_base;
		}

		w = &mvebu_mbus_dram_info.cs[cs++];
		w->cs_index = i;
		w->mbus_attr = 0xf & ~(1 << i);
		if (mbus->hw_io_coherency)
			w->mbus_attr |= ATTR_HW_COHERENCY;
		w->base = base;
		w->size = size;
	}
	mvebu_mbus_dram_info.num_cs = cs;
#else  
		u32 base = readl(mbus->sdramwins_base + DDR_BASE_CS_OFF(i));
		u32 size = readl(mbus->sdramwins_base + DDR_SIZE_CS_OFF(i));

		if ((size & DDR_SIZE_ENABLED) &&
		    !(base & DDR_BASE_CS_HIGH_MASK)) {
			struct mbus_dram_window *w;

			w = &mvebu_mbus_dram_info.cs[cs++];
			w->cs_index = i;
			w->mbus_attr = 0xf & ~(1 << i);
			if (mbus->hw_io_coherency)
				w->mbus_attr |= ATTR_HW_COHERENCY;
			w->base = base & DDR_BASE_CS_LOW_MASK;
			w->size = (size | ~DDR_SIZE_MASK) + 1;
		}
	}
	mvebu_mbus_dram_info.num_cs = cs;
#endif  
}

#if defined(MY_ABC_HERE)
static int
mvebu_mbus_default_save_cpu_target(struct mvebu_mbus_state *mbus,
				   u32 *store_addr)
{
	int i;

	for (i = 0; i < 4; i++) {
		u32 base = readl(mbus->sdramwins_base + DDR_BASE_CS_OFF(i));
		u32 size = readl(mbus->sdramwins_base + DDR_SIZE_CS_OFF(i));

		writel(mbus->sdramwins_phys_base + DDR_BASE_CS_OFF(i),
		       store_addr++);
		writel(base, store_addr++);
		writel(mbus->sdramwins_phys_base + DDR_SIZE_CS_OFF(i),
		       store_addr++);
		writel(size, store_addr++);
	}

	return 16;
}
#endif  

static void __init
mvebu_mbus_dove_setup_cpu_target(struct mvebu_mbus_state *mbus)
{
	int i;
	int cs;

	mvebu_mbus_dram_info.mbus_dram_target_id = TARGET_DDR;

	for (i = 0, cs = 0; i < 2; i++) {
		u32 map = readl(mbus->sdramwins_base + DOVE_DDR_BASE_CS_OFF(i));

		if (map & 1) {
			struct mbus_dram_window *w;

			w = &mvebu_mbus_dram_info.cs[cs++];
			w->cs_index = i;
			w->mbus_attr = 0;  
					   
			w->base = map & 0xff800000;
			w->size = 0x100000 << (((map & 0x000f0000) >> 16) - 4);
		}
	}

	mvebu_mbus_dram_info.num_cs = cs;
}

#if defined(MY_ABC_HERE)
int mvebu_mbus_save_cpu_target(u32 *store_addr)
{
	return mbus_state.soc->save_cpu_target(&mbus_state, store_addr);
}
#else  
static const struct mvebu_mbus_mapping armada_370_map[] = {
	MAPDEF("bootrom",     1, 0xe0, MAPDEF_NOMASK),
	MAPDEF("devbus-boot", 1, 0x2f, MAPDEF_NOMASK),
	MAPDEF("devbus-cs0",  1, 0x3e, MAPDEF_NOMASK),
	MAPDEF("devbus-cs1",  1, 0x3d, MAPDEF_NOMASK),
	MAPDEF("devbus-cs2",  1, 0x3b, MAPDEF_NOMASK),
	MAPDEF("devbus-cs3",  1, 0x37, MAPDEF_NOMASK),
	MAPDEF("pcie0.0",     4, 0xe0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.0",     8, 0xe0, MAPDEF_PCIMASK),
	{},
};
#endif  

static const struct mvebu_mbus_soc_data armada_370_mbus_data = {
	.num_wins            = 20,
#if defined(MY_ABC_HERE)
	.win_cfg_offset      = armada_370_xp_mbus_win_cfg_offset,
	.win_remap_offset    = generic_mbus_win_remap_8_offset,
	.save_cpu_target     = mvebu_mbus_default_save_cpu_target,
#else  
	.num_remappable_wins = 8,
	.win_cfg_offset      = armada_370_xp_mbus_win_offset,
#endif  
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
	.show_cpu_target     = mvebu_sdram_debug_show_orion,
#if defined(MY_ABC_HERE)
	 
#else  
	.map                 = armada_370_map,
#endif  
};

#if defined(MY_ABC_HERE)
 
#else  
static const struct mvebu_mbus_mapping armada_xp_map[] = {
	MAPDEF("bootrom",     1, 0x1d, MAPDEF_NOMASK),
	MAPDEF("devbus-boot", 1, 0x2f, MAPDEF_NOMASK),
	MAPDEF("devbus-cs0",  1, 0x3e, MAPDEF_NOMASK),
	MAPDEF("devbus-cs1",  1, 0x3d, MAPDEF_NOMASK),
	MAPDEF("devbus-cs2",  1, 0x3b, MAPDEF_NOMASK),
	MAPDEF("devbus-cs3",  1, 0x37, MAPDEF_NOMASK),
	MAPDEF("pcie0.0",     4, 0xe0, MAPDEF_PCIMASK),
	MAPDEF("pcie0.1",     4, 0xd0, MAPDEF_PCIMASK),
	MAPDEF("pcie0.2",     4, 0xb0, MAPDEF_PCIMASK),
	MAPDEF("pcie0.3",     4, 0x70, MAPDEF_PCIMASK),
	MAPDEF("pcie1.0",     8, 0xe0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.1",     8, 0xd0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.2",     8, 0xb0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.3",     8, 0x70, MAPDEF_PCIMASK),
	MAPDEF("pcie2.0",     4, 0xf0, MAPDEF_PCIMASK),
	MAPDEF("pcie3.0",     8, 0xf0, MAPDEF_PCIMASK),
	{},
};
#endif  

static const struct mvebu_mbus_soc_data armada_xp_mbus_data = {
	.num_wins            = 20,
#if defined(MY_ABC_HERE)
	.has_mbus_bridge     = true,
	.win_cfg_offset      = armada_370_xp_mbus_win_cfg_offset,
	.win_remap_offset    = armada_xp_mbus_win_remap_offset,
	.save_cpu_target     = mvebu_mbus_default_save_cpu_target,
#else  
	.num_remappable_wins = 8,
	.win_cfg_offset      = armada_370_xp_mbus_win_offset,
#endif  
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
	.show_cpu_target     = mvebu_sdram_debug_show_orion,
#if defined(MY_ABC_HERE)
	 
#else  
	.map                 = armada_xp_map,
#endif  
};

#if defined(MY_ABC_HERE)
 
#else  
static const struct mvebu_mbus_mapping kirkwood_map[] = {
	MAPDEF("pcie0.0", 4, 0xe0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.0", 4, 0xd0, MAPDEF_PCIMASK),
	MAPDEF("sram",    3, 0x01, MAPDEF_NOMASK),
	MAPDEF("nand",    1, 0x2f, MAPDEF_NOMASK),
	{},
};
#endif  

static const struct mvebu_mbus_soc_data kirkwood_mbus_data = {
	.num_wins            = 8,
#if defined(MY_ABC_HERE)
	.win_cfg_offset      = generic_mbus_win_cfg_offset,
	.win_remap_offset    = generic_mbus_win_remap_4_offset,
#else  
	.num_remappable_wins = 4,
	.win_cfg_offset      = orion_mbus_win_offset,
#endif  
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
	.show_cpu_target     = mvebu_sdram_debug_show_orion,
#if defined(MY_ABC_HERE)
	 
#else  
	.map                 = kirkwood_map,
#endif  
};

#if defined(MY_ABC_HERE)
 
#else  
static const struct mvebu_mbus_mapping dove_map[] = {
	MAPDEF("pcie0.0",    0x4, 0xe0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.0",    0x8, 0xe0, MAPDEF_PCIMASK),
	MAPDEF("cesa",       0x3, 0x01, MAPDEF_NOMASK),
	MAPDEF("bootrom",    0x1, 0xfd, MAPDEF_NOMASK),
	MAPDEF("scratchpad", 0xd, 0x0, MAPDEF_NOMASK),
	{},
};
#endif  

static const struct mvebu_mbus_soc_data dove_mbus_data = {
	.num_wins            = 8,
#if defined(MY_ABC_HERE)
	.win_cfg_offset      = generic_mbus_win_cfg_offset,
	.win_remap_offset    = generic_mbus_win_remap_4_offset,
#else  
	.num_remappable_wins = 4,
	.win_cfg_offset      = orion_mbus_win_offset,
#endif  
	.setup_cpu_target    = mvebu_mbus_dove_setup_cpu_target,
	.show_cpu_target     = mvebu_sdram_debug_show_dove,
#if defined(MY_ABC_HERE)
	 
#else  
	.map                 = dove_map,
#endif  
};

#if defined(MY_ABC_HERE)
 
#else  
static const struct mvebu_mbus_mapping orion5x_map[] = {
	MAPDEF("pcie0.0",     4, 0x51, MAPDEF_ORIONPCIMASK),
	MAPDEF("pci0.0",      3, 0x51, MAPDEF_ORIONPCIMASK),
	MAPDEF("devbus-boot", 1, 0x0f, MAPDEF_NOMASK),
	MAPDEF("devbus-cs0",  1, 0x1e, MAPDEF_NOMASK),
	MAPDEF("devbus-cs1",  1, 0x1d, MAPDEF_NOMASK),
	MAPDEF("devbus-cs2",  1, 0x1b, MAPDEF_NOMASK),
	MAPDEF("sram",        0, 0x00, MAPDEF_NOMASK),
	{},
};
#endif  

static const struct mvebu_mbus_soc_data orion5x_4win_mbus_data = {
	.num_wins            = 8,
#if defined(MY_ABC_HERE)
	.win_cfg_offset      = generic_mbus_win_cfg_offset,
	.win_remap_offset    = generic_mbus_win_remap_4_offset,
#else  
	.num_remappable_wins = 4,
	.win_cfg_offset      = orion_mbus_win_offset,
#endif  
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
	.show_cpu_target     = mvebu_sdram_debug_show_orion,
#if defined(MY_ABC_HERE)
	 
#else  
	.map                 = orion5x_map,
#endif  
};

static const struct mvebu_mbus_soc_data orion5x_2win_mbus_data = {
	.num_wins            = 8,
#if defined(MY_ABC_HERE)
	.win_cfg_offset      = generic_mbus_win_cfg_offset,
	.win_remap_offset    = generic_mbus_win_remap_2_offset,
#else  
	.num_remappable_wins = 2,
	.win_cfg_offset      = orion_mbus_win_offset,
#endif  
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
	.show_cpu_target     = mvebu_sdram_debug_show_orion,
#if defined(MY_ABC_HERE)
	 
#else  
	.map                 = orion5x_map,
#endif  
};

#if defined(MY_ABC_HERE)
 
#else  
static const struct mvebu_mbus_mapping mv78xx0_map[] = {
	MAPDEF("pcie0.0", 4, 0xe0, MAPDEF_PCIMASK),
	MAPDEF("pcie0.1", 4, 0xd0, MAPDEF_PCIMASK),
	MAPDEF("pcie0.2", 4, 0xb0, MAPDEF_PCIMASK),
	MAPDEF("pcie0.3", 4, 0x70, MAPDEF_PCIMASK),
	MAPDEF("pcie1.0", 8, 0xe0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.1", 8, 0xd0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.2", 8, 0xb0, MAPDEF_PCIMASK),
	MAPDEF("pcie1.3", 8, 0x70, MAPDEF_PCIMASK),
	MAPDEF("pcie2.0", 4, 0xf0, MAPDEF_PCIMASK),
	MAPDEF("pcie3.0", 8, 0xf0, MAPDEF_PCIMASK),
	{},
};
#endif  

static const struct mvebu_mbus_soc_data mv78xx0_mbus_data = {
	.num_wins            = 14,
#if defined(MY_ABC_HERE)
	.win_cfg_offset      = mv78xx0_mbus_win_cfg_offset,
	.win_remap_offset    = generic_mbus_win_remap_8_offset,
#else  
	.num_remappable_wins = 8,
	.win_cfg_offset      = mv78xx0_mbus_win_offset,
#endif  
	.setup_cpu_target    = mvebu_mbus_default_setup_cpu_target,
	.show_cpu_target     = mvebu_sdram_debug_show_orion,
#if defined(MY_ABC_HERE)
	 
#else  
	.map                 = mv78xx0_map,
#endif  
};

static const struct of_device_id of_mvebu_mbus_ids[] = {
	{ .compatible = "marvell,armada370-mbus",
	  .data = &armada_370_mbus_data, },
#if defined(MY_ABC_HERE)
	{ .compatible = "marvell,armada375-mbus",
	  .data = &armada_xp_mbus_data, },
	{ .compatible = "marvell,armada380-mbus",
	  .data = &armada_xp_mbus_data, },
#endif  
	{ .compatible = "marvell,armadaxp-mbus",
	  .data = &armada_xp_mbus_data, },
	{ .compatible = "marvell,kirkwood-mbus",
	  .data = &kirkwood_mbus_data, },
	{ .compatible = "marvell,dove-mbus",
	  .data = &dove_mbus_data, },
	{ .compatible = "marvell,orion5x-88f5281-mbus",
	  .data = &orion5x_4win_mbus_data, },
	{ .compatible = "marvell,orion5x-88f5182-mbus",
	  .data = &orion5x_2win_mbus_data, },
	{ .compatible = "marvell,orion5x-88f5181-mbus",
	  .data = &orion5x_2win_mbus_data, },
	{ .compatible = "marvell,orion5x-88f6183-mbus",
	  .data = &orion5x_4win_mbus_data, },
	{ .compatible = "marvell,mv78xx0-mbus",
	  .data = &mv78xx0_mbus_data, },
	{ },
};

#if defined(MY_ABC_HERE)
int mvebu_mbus_add_window_remap_by_id(unsigned int target,
				      unsigned int attribute,
				      phys_addr_t base, size_t size,
				      phys_addr_t remap)
{
	struct mvebu_mbus_state *s = &mbus_state;

	if (!mvebu_mbus_window_conflicts(s, base, size, target, attribute)) {
		pr_err("cannot add window '%x:%x', conflicts with another window\n",
		       target, attribute);
		return -EINVAL;
	}

	return mvebu_mbus_alloc_window(s, base, size, remap, target, attribute);
}
#else  
int mvebu_mbus_add_window_remap_flags(const char *devname, phys_addr_t base,
				      size_t size, phys_addr_t remap,
				      unsigned int flags)
{
	struct mvebu_mbus_state *s = &mbus_state;
	u8 target, attr;
	int i;

	if (!s->soc->map)
		return -ENODEV;

	for (i = 0; s->soc->map[i].name; i++)
		if (!strcmp(s->soc->map[i].name, devname))
			break;

	if (!s->soc->map[i].name) {
		pr_err("mvebu-mbus: unknown device '%s'\n", devname);
		return -ENODEV;
	}

	target = s->soc->map[i].target;
	attr   = s->soc->map[i].attr;

	if (flags == MVEBU_MBUS_PCI_MEM)
		attr |= 0x8;
	else if (flags == MVEBU_MBUS_PCI_WA)
		attr |= 0x28;

	if (!mvebu_mbus_window_conflicts(s, base, size, target, attr)) {
		pr_err("mvebu-mbus: cannot add window '%s', conflicts with another window\n",
		       devname);
		return -EINVAL;
	}

	return mvebu_mbus_alloc_window(s, base, size, remap, target, attr);

}
#endif  

#if defined(MY_ABC_HERE)
int mvebu_mbus_add_window_by_id(unsigned int target, unsigned int attribute,
				phys_addr_t base, size_t size)
{
	return mvebu_mbus_add_window_remap_by_id(target, attribute, base,
						 size, MVEBU_MBUS_NO_REMAP);
}
#else  
int mvebu_mbus_add_window(const char *devname, phys_addr_t base, size_t size)
{
	return mvebu_mbus_add_window_remap_flags(devname, base, size,
						 MVEBU_MBUS_NO_REMAP, 0);
}
#endif  

int mvebu_mbus_del_window(phys_addr_t base, size_t size)
{
	int win;

	win = mvebu_mbus_find_window(&mbus_state, base, size);
	if (win < 0)
		return win;

	mvebu_mbus_disable_window(&mbus_state, win);
	return 0;
}

#if defined(MY_ABC_HERE)
void mvebu_mbus_get_pcie_mem_aperture(struct resource *res)
{
	if (!res)
		return;
	*res = mbus_state.pcie_mem_aperture;
}

void mvebu_mbus_get_pcie_io_aperture(struct resource *res)
{
	if (!res)
		return;
	*res = mbus_state.pcie_io_aperture;
}

int mvebu_mbus_get_addr_win_info(phys_addr_t phyaddr, u8 *trg_id, u8 *attr)
{
	const struct mbus_dram_target_info *dram;
	int i;

	if (NULL == trg_id || NULL == attr) {
		pr_err("%s: Invalid parameter\n", __func__);
		return -EINVAL;
	}
	 
	dram = mv_mbus_dram_info();
	if (!dram) {
		pr_err("%s: No DRAM information\n", __func__);
		return -ENODEV;
	}
	 
	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;
		if (cs->base <= phyaddr && phyaddr <= (cs->base + cs->size)) {
			*trg_id = dram->mbus_dram_target_id;
			*attr = cs->mbus_attr;
			break;
		}
	}
	if (i == dram->num_cs) {
		pr_err("%s: Invalid dram address 0x%x\n", __func__, phyaddr);
		return -EINVAL;
	}

	return 0;
}
#if defined(MY_ABC_HERE)
EXPORT_SYMBOL(mvebu_mbus_get_addr_win_info);
#endif  
#endif  

static __init int mvebu_mbus_debugfs_init(void)
{
	struct mvebu_mbus_state *s = &mbus_state;

	if (!s->mbuswins_base)
		return 0;

	s->debugfs_root = debugfs_create_dir("mvebu-mbus", NULL);
	if (s->debugfs_root) {
		s->debugfs_sdram = debugfs_create_file("sdram", S_IRUGO,
						       s->debugfs_root, NULL,
						       &mvebu_sdram_debug_fops);
		s->debugfs_devs = debugfs_create_file("devices", S_IRUGO,
						      s->debugfs_root, NULL,
						      &mvebu_devs_debug_fops);
	}

	return 0;
}
fs_initcall(mvebu_mbus_debugfs_init);

#if defined(MY_ABC_HERE)
static int mvebu_mbus_suspend(void)
{
	struct mvebu_mbus_state *s = &mbus_state;
	int win;

	if (!s->mbusbridge_base)
		return -ENODEV;

	for (win = 0; win < s->soc->num_wins; win++) {
		void __iomem *addr = s->mbuswins_base +
			s->soc->win_cfg_offset(win);

		s->wins[win].base = readl(addr + WIN_BASE_OFF);
		s->wins[win].ctrl = readl(addr + WIN_CTRL_OFF);

		if (mvebu_mbus_window_is_remappable(s, win)) {
			s->wins[win].remap_lo = readl(addr + WIN_REMAP_LO_OFF);
			s->wins[win].remap_hi = readl(addr + WIN_REMAP_HI_OFF);
		}
	}

	s->mbus_bridge_ctrl = readl(s->mbusbridge_base +
				    MBUS_BRIDGE_CTRL_OFF);
	s->mbus_bridge_base = readl(s->mbusbridge_base +
				    MBUS_BRIDGE_BASE_OFF);

	return 0;
}

static void mvebu_mbus_resume(void)
{
	struct mvebu_mbus_state *s = &mbus_state;
	int win;

	writel(s->mbus_bridge_ctrl,
	       s->mbusbridge_base + MBUS_BRIDGE_CTRL_OFF);
	writel(s->mbus_bridge_base,
	       s->mbusbridge_base + MBUS_BRIDGE_BASE_OFF);

	for (win = 0; win < s->soc->num_wins; win++) {
		void __iomem *addr = s->mbuswins_base +
			s->soc->win_cfg_offset(win);

		writel(s->wins[win].base, addr + WIN_BASE_OFF);
		writel(s->wins[win].ctrl, addr + WIN_CTRL_OFF);

		if (mvebu_mbus_window_is_remappable(s, win)) {
			writel(s->wins[win].remap_lo, addr + WIN_REMAP_LO_OFF);
			writel(s->wins[win].remap_hi, addr + WIN_REMAP_HI_OFF);
		}
	}
}

struct syscore_ops mvebu_mbus_syscore_ops = {
	.suspend	= mvebu_mbus_suspend,
	.resume		= mvebu_mbus_resume,
};

static int __init mvebu_mbus_common_init(struct mvebu_mbus_state *mbus,
					 phys_addr_t mbuswins_phys_base,
					 size_t mbuswins_size,
					 phys_addr_t sdramwins_phys_base,
					 size_t sdramwins_size,
					 phys_addr_t mbusbridge_phys_base,
					 size_t mbusbridge_size)
{
	int win;

	mbus->mbuswins_base = ioremap(mbuswins_phys_base, mbuswins_size);
	if (!mbus->mbuswins_base)
		return -ENOMEM;

	mbus->sdramwins_base = ioremap(sdramwins_phys_base, sdramwins_size);
	if (!mbus->sdramwins_base) {
		iounmap(mbus_state.mbuswins_base);
		return -ENOMEM;
	}

	if (of_find_compatible_node(NULL, NULL, "marvell,coherency-fabric"))
		mbus->hw_io_coherency = 1;

	for (win = 0; win < mbus->soc->num_wins; win++)
		mvebu_mbus_disable_window(mbus, win);

	mbus->soc->setup_cpu_target(mbus);

	register_syscore_ops(&mvebu_mbus_syscore_ops);

	return 0;
}

int __init mvebu_mbus_init(const char *soc, phys_addr_t mbuswins_phys_base,
			   size_t mbuswins_size,
			   phys_addr_t sdramwins_phys_base,
			   size_t sdramwins_size, int is_coherent)
{
	const struct of_device_id *of_id;

	for (of_id = of_mvebu_mbus_ids; of_id->compatible; of_id++)
		if (!strcmp(of_id->compatible, soc))
			break;

	if (!of_id->compatible) {
		pr_err("could not find a matching SoC family\n");
		return -ENODEV;
	}

	mbus_state.soc = of_id->data;

	return mvebu_mbus_common_init(&mbus_state,
			mbuswins_phys_base,
			mbuswins_size,
			sdramwins_phys_base,
			sdramwins_size, 0, 0);
}
#else  
int __init mvebu_mbus_init(const char *soc, phys_addr_t mbuswins_phys_base,
			   size_t mbuswins_size,
			   phys_addr_t sdramwins_phys_base,
			   size_t sdramwins_size, int is_coherent)
{
	struct mvebu_mbus_state *mbus = &mbus_state;
	const struct of_device_id *of_id;
	int win;

	for (of_id = of_mvebu_mbus_ids; of_id->compatible; of_id++)
		if (!strcmp(of_id->compatible, soc))
			break;

	if (!of_id->compatible) {
		pr_err("mvebu-mbus: could not find a matching SoC family\n");
		return -ENODEV;
	}

	mbus->soc = of_id->data;

	mbus->mbuswins_base = ioremap(mbuswins_phys_base, mbuswins_size);
	if (!mbus->mbuswins_base)
		return -ENOMEM;

	mbus->sdramwins_base = ioremap(sdramwins_phys_base, sdramwins_size);
	if (!mbus->sdramwins_base) {
		iounmap(mbus_state.mbuswins_base);
		return -ENOMEM;
	}

	mbus->hw_io_coherency = is_coherent;

	for (win = 0; win < mbus->soc->num_wins; win++)
		mvebu_mbus_disable_window(mbus, win);

	mbus->soc->setup_cpu_target(mbus);

	return 0;
}
#endif  

#if defined(MY_ABC_HERE)
#ifdef CONFIG_OF
 
#define CUSTOM(id) (((id) & 0xF0000000) >> 24)
#define TARGET(id) (((id) & 0x0F000000) >> 24)
#define ATTR(id)   (((id) & 0x00FF0000) >> 16)

static int __init mbus_dt_setup_win(struct mvebu_mbus_state *mbus,
				    u32 base, u32 size,
				    u8 target, u8 attr)
{
	if (!mvebu_mbus_window_conflicts(mbus, base, size, target, attr)) {
		pr_err("cannot add window '%04x:%04x', conflicts with another window\n",
		       target, attr);
		return -EBUSY;
	}

	if (mvebu_mbus_alloc_window(mbus, base, size, MVEBU_MBUS_NO_REMAP,
				    target, attr)) {
		pr_err("cannot add window '%04x:%04x', too many windows\n",
		       target, attr);
		return -ENOMEM;
	}
	return 0;
}

static int
mbus_parse_ranges(struct device_node *node,
		  int *addr_cells, int *c_addr_cells, int *c_size_cells,
		  int *cell_count, const __be32 **ranges_start,
		  const __be32 **ranges_end)
{
	const __be32 *prop;
	int ranges_len, tuple_len;

	*ranges_start = of_get_property(node, "ranges", &ranges_len);
	if (*ranges_start == NULL) {
		*addr_cells = *c_addr_cells = *c_size_cells = *cell_count = 0;
		*ranges_start = *ranges_end = NULL;
		return 0;
	}
	*ranges_end = *ranges_start + ranges_len / sizeof(__be32);

	*addr_cells = of_n_addr_cells(node);

	prop = of_get_property(node, "#address-cells", NULL);
	*c_addr_cells = be32_to_cpup(prop);

	prop = of_get_property(node, "#size-cells", NULL);
	*c_size_cells = be32_to_cpup(prop);

	*cell_count = *addr_cells + *c_addr_cells + *c_size_cells;
	tuple_len = (*cell_count) * sizeof(__be32);

	if (ranges_len % tuple_len) {
		pr_warn("malformed ranges entry '%s'\n", node->name);
		return -EINVAL;
	}
	return 0;
}

static int __init mbus_dt_setup(struct mvebu_mbus_state *mbus,
				struct device_node *np)
{
	int addr_cells, c_addr_cells, c_size_cells;
	int i, ret, cell_count;
	const __be32 *r, *ranges_start, *ranges_end;

	ret = mbus_parse_ranges(np, &addr_cells, &c_addr_cells,
				&c_size_cells, &cell_count,
				&ranges_start, &ranges_end);
	if (ret < 0)
		return ret;

	for (i = 0, r = ranges_start; r < ranges_end; r += cell_count, i++) {
		u32 windowid, base, size;
		u8 target, attr;

		windowid = of_read_number(r, 1);
		if (CUSTOM(windowid))
			continue;

		target = TARGET(windowid);
		attr = ATTR(windowid);

		base = of_read_number(r + c_addr_cells, addr_cells);
		size = of_read_number(r + c_addr_cells + addr_cells,
				      c_size_cells);
		ret = mbus_dt_setup_win(mbus, base, size, target, attr);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static void __init mvebu_mbus_get_pcie_resources(struct device_node *np,
						 struct resource *mem,
						 struct resource *io)
{
	u32 reg[2];
	int ret;

	memset(mem, 0, sizeof(struct resource));
	memset(io, 0, sizeof(struct resource));

	ret = of_property_read_u32_array(np, "pcie-mem-aperture", reg, ARRAY_SIZE(reg));
	if (!ret) {
		mem->start = reg[0];
		mem->end = mem->start + reg[1] - 1;
		mem->flags = IORESOURCE_MEM;
	}

	ret = of_property_read_u32_array(np, "pcie-io-aperture", reg, ARRAY_SIZE(reg));
	if (!ret) {
		io->start = reg[0];
		io->end = io->start + reg[1] - 1;
		io->flags = IORESOURCE_IO;
	}
}

int __init mvebu_mbus_dt_init(bool is_coherent)
{
	struct resource mbuswins_res, sdramwins_res, mbusbridge_res;
	struct device_node *np, *controller;
	const struct of_device_id *of_id;
	const __be32 *prop;
	int ret;

	np = of_find_matching_node(NULL, of_mvebu_mbus_ids);
	if (!np) {
		pr_err("could not find a matching SoC family\n");
		return -ENODEV;
	}

	of_id = of_match_node(of_mvebu_mbus_ids, np);
	mbus_state.soc = of_id->data;

	prop = of_get_property(np, "controller", NULL);
	if (!prop) {
		pr_err("required 'controller' property missing\n");
		return -EINVAL;
	}

	controller = of_find_node_by_phandle(be32_to_cpup(prop));
	if (!controller) {
		pr_err("could not find an 'mbus-controller' node\n");
		return -ENODEV;
	}

	if (of_address_to_resource(controller, 0, &mbuswins_res)) {
		pr_err("cannot get MBUS register address\n");
		return -EINVAL;
	}

	if (of_address_to_resource(controller, 1, &sdramwins_res)) {
		pr_err("cannot get SDRAM register address\n");
		return -EINVAL;
	}

	memset(&mbusbridge_res, 0, sizeof(mbusbridge_res));

	if (mbus_state.soc->has_mbus_bridge) {
		if (of_address_to_resource(controller, 2, &mbusbridge_res))
			pr_warn(FW_WARN "deprecated mbus-mvebu Device Tree, suspend/resume will not work\n");
	}

	mbus_state.hw_io_coherency = is_coherent;

	mvebu_mbus_get_pcie_resources(np, &mbus_state.pcie_mem_aperture,
					  &mbus_state.pcie_io_aperture);

	ret = mvebu_mbus_common_init(&mbus_state,
				     mbuswins_res.start,
				     resource_size(&mbuswins_res),
				     sdramwins_res.start,
				     resource_size(&sdramwins_res),
				     mbusbridge_res.start,
				     resource_size(&mbusbridge_res));
	if (ret)
		return ret;

	return mbus_dt_setup(&mbus_state, np);
}

int mvebu_mbus_win_addr_get(u8 target_id, u8 attribute, u32 *phy_base, u32 *size)
{
	int addr_cells, c_addr_cells, c_size_cells;
	int i, ret, cell_count;
	const __be32 *r, *ranges_start, *ranges_end;
	struct device_node *np;

	np = of_find_matching_node(NULL, of_mvebu_mbus_ids);
	if (!np) {
		pr_err("could not find a matching SoC family\n");
		return -ENODEV;
	}

	ret = mbus_parse_ranges(np, &addr_cells, &c_addr_cells,
				&c_size_cells, &cell_count,
				&ranges_start, &ranges_end);
	if (ret < 0)
		return ret;

	*phy_base = 0;
	*size = 0;
	for (i = 0, r = ranges_start; r < ranges_end; r += cell_count, i++) {
		u32 windowid;
		u8 target, attr;

		windowid = of_read_number(r, 1);
		if (CUSTOM(windowid))
			continue;

		target = TARGET(windowid);
		attr = ATTR(windowid);
		if (target_id != target || attr != attribute)
			continue;

		*phy_base = of_read_number(r + c_addr_cells, addr_cells);
		*size = of_read_number(r + c_addr_cells + addr_cells,
				      c_size_cells);
		break;
	}
	return 0;
}
#if defined(MY_ABC_HERE)
EXPORT_SYMBOL(mvebu_mbus_win_addr_get);
#endif  

#ifdef MBUS_DEBUG
void mbus_debug_window()
{
	void __iomem *win_addr = mbus_state.mbuswins_base;
	int i, j;

	pr_info("----------- mbus window -----------\n");

	for (i = 0; i <= 7; i++) {

		pr_info("WIN%d\n", i);
		for (j = 0; j < 4; j++) {

			pr_info("\twin(%d,%d): %p\t 0x%x\n", i, j,
			    win_addr, readl(win_addr));

			win_addr += 4;
		}
	}

	pr_info("\nINTERREGS_WIN(%d): %p\t 0x%x\n", i++, win_addr,
							      readl(win_addr));
	win_addr += 4;
	pr_info("\nSYNC_BARIER_WIN(%d): %p\t 0x%x\n\n", i++, win_addr,
							      readl(win_addr));
	win_addr += 4;

	win_addr += 8;

	for (i = 8; i <= 19; i++) {

		pr_info("WIN%d\n", i);
		for (j = 0; j < 2; j++) {

			pr_info("\twin(%d,%d): %p\t 0x%x\n", i, j,
			    win_addr, readl(win_addr));

			win_addr += 4;
		}
	}

	pr_info("\n");

}
#endif  
#endif  
#endif  

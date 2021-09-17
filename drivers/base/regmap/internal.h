#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _REGMAP_INTERNAL_H
#define _REGMAP_INTERNAL_H

#include <linux/regmap.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/wait.h>

struct regmap;
struct regcache_ops;

struct regmap_debugfs_off_cache {
	struct list_head list;
	off_t min;
	off_t max;
	unsigned int base_reg;
	unsigned int max_reg;
};

struct regmap_format {
	size_t buf_size;
	size_t reg_bytes;
	size_t pad_bytes;
	size_t val_bytes;
	void (*format_write)(struct regmap *map,
			     unsigned int reg, unsigned int val);
	void (*format_reg)(void *buf, unsigned int reg, unsigned int shift);
	void (*format_val)(void *buf, unsigned int val, unsigned int shift);
	unsigned int (*parse_val)(const void *buf);
	void (*parse_inplace)(void *buf);
};

struct regmap_async {
	struct list_head list;
	struct work_struct cleanup;
	struct regmap *map;
	void *work_buf;
};

struct regmap {
	struct mutex mutex;
	spinlock_t spinlock;
	regmap_lock lock;
	regmap_unlock unlock;
	void *lock_arg;  

	struct device *dev;  
	void *work_buf;      
	struct regmap_format format;   
	const struct regmap_bus *bus;
	void *bus_context;
	const char *name;

	spinlock_t async_lock;
	wait_queue_head_t async_waitq;
	struct list_head async_list;
	int async_ret;

#ifdef CONFIG_DEBUG_FS
	struct dentry *debugfs;
	const char *debugfs_name;

	unsigned int debugfs_reg_len;
	unsigned int debugfs_val_len;
	unsigned int debugfs_tot_len;

	struct list_head debugfs_off_cache;
	struct mutex cache_lock;
#endif

	unsigned int max_register;
	bool (*writeable_reg)(struct device *dev, unsigned int reg);
	bool (*readable_reg)(struct device *dev, unsigned int reg);
	bool (*volatile_reg)(struct device *dev, unsigned int reg);
	bool (*precious_reg)(struct device *dev, unsigned int reg);
	const struct regmap_access_table *wr_table;
	const struct regmap_access_table *rd_table;
	const struct regmap_access_table *volatile_table;
	const struct regmap_access_table *precious_table;

	int (*reg_read)(void *context, unsigned int reg, unsigned int *val);
	int (*reg_write)(void *context, unsigned int reg, unsigned int val);

	bool defer_caching;

	u8 read_flag_mask;
	u8 write_flag_mask;

	int reg_shift;
	int reg_stride;

	const struct regcache_ops *cache_ops;
	enum regcache_type cache_type;

	unsigned int cache_size_raw;
	 
	unsigned int cache_word_size;
	 
	unsigned int num_reg_defaults;
	 
	unsigned int num_reg_defaults_raw;

	u32 cache_only;
	 
	u32 cache_bypass;
	 
	bool cache_free;

	struct reg_default *reg_defaults;
	const void *reg_defaults_raw;
	void *cache;
	u32 cache_dirty;

	unsigned long *cache_present;
	unsigned int cache_present_nbits;

	struct reg_default *patch;
	int patch_regs;

	bool use_single_rw;

	struct rb_root range_tree;
	void *selector_work_buf;	 
};

struct regcache_ops {
	const char *name;
	enum regcache_type type;
	int (*init)(struct regmap *map);
	int (*exit)(struct regmap *map);
	int (*read)(struct regmap *map, unsigned int reg, unsigned int *value);
	int (*write)(struct regmap *map, unsigned int reg, unsigned int value);
	int (*sync)(struct regmap *map, unsigned int min, unsigned int max);
};

bool regmap_writeable(struct regmap *map, unsigned int reg);
bool regmap_readable(struct regmap *map, unsigned int reg);
bool regmap_volatile(struct regmap *map, unsigned int reg);
bool regmap_precious(struct regmap *map, unsigned int reg);

int _regmap_write(struct regmap *map, unsigned int reg,
		  unsigned int val);

struct regmap_range_node {
	struct rb_node node;
	const char *name;
	struct regmap *map;

	unsigned int range_min;
	unsigned int range_max;

	unsigned int selector_reg;
	unsigned int selector_mask;
	int selector_shift;

	unsigned int window_start;
	unsigned int window_len;
};

#if defined (MY_DEF_HERE)
struct regmap_field {
	struct regmap *regmap;
	unsigned int mask;
	 
	unsigned int shift;
	unsigned int reg;
};
#endif  
#ifdef CONFIG_DEBUG_FS
extern void regmap_debugfs_initcall(void);
extern void regmap_debugfs_init(struct regmap *map, const char *name);
extern void regmap_debugfs_exit(struct regmap *map);
#else
static inline void regmap_debugfs_initcall(void) { }
static inline void regmap_debugfs_init(struct regmap *map, const char *name) { }
static inline void regmap_debugfs_exit(struct regmap *map) { }
#endif

int regcache_init(struct regmap *map, const struct regmap_config *config);
void regcache_exit(struct regmap *map);
int regcache_read(struct regmap *map,
		       unsigned int reg, unsigned int *value);
int regcache_write(struct regmap *map,
			unsigned int reg, unsigned int value);
int regcache_sync(struct regmap *map);
int regcache_sync_block(struct regmap *map, void *block,
			unsigned int block_base, unsigned int start,
			unsigned int end);

static inline const void *regcache_get_val_addr(struct regmap *map,
						const void *base,
						unsigned int idx)
{
	return base + (map->cache_word_size * idx);
}

unsigned int regcache_get_val(struct regmap *map, const void *base,
			      unsigned int idx);
bool regcache_set_val(struct regmap *map, void *base, unsigned int idx,
		      unsigned int val);
int regcache_lookup_reg(struct regmap *map, unsigned int reg);
int regcache_set_reg_present(struct regmap *map, unsigned int reg);

static inline bool regcache_reg_present(struct regmap *map, unsigned int reg)
{
	if (!map->cache_present)
		return true;
	if (reg > map->cache_present_nbits)
		return false;
	return map->cache_present[BIT_WORD(reg)] & BIT_MASK(reg);
}

int _regmap_raw_write(struct regmap *map, unsigned int reg,
		      const void *val, size_t val_len, bool async);

void regmap_async_complete_cb(struct regmap_async *async, int ret);

extern struct regcache_ops regcache_rbtree_ops;
extern struct regcache_ops regcache_lzo_ops;
extern struct regcache_ops regcache_flat_ops;

#endif

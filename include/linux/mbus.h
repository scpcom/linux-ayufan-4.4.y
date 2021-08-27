#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LINUX_MBUS_H
#define __LINUX_MBUS_H

struct mbus_dram_target_info
{
	 
	u8		mbus_dram_target_id;

	int		num_cs;
	struct mbus_dram_window {
		u8	cs_index;
		u8	mbus_attr;
		u32	base;
		u32	size;
	} cs[4];
};

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
 
#if defined(CONFIG_PLAT_ORION) || defined(CONFIG_PLAT_ARMADA)
extern const struct mbus_dram_target_info *mv_mbus_dram_info(void);
#else
static inline const struct mbus_dram_target_info *mv_mbus_dram_info(void)
{
	return NULL;
}
#endif
#endif
#endif

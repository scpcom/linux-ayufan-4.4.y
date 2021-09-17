#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LINUX_MBUS_H
#define __LINUX_MBUS_H

#if defined(MY_ABC_HERE)
struct resource;
#endif  

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

#define MVEBU_MBUS_PCI_IO  0x1
#define MVEBU_MBUS_PCI_MEM 0x2
#define MVEBU_MBUS_PCI_WA  0x3

#define MVEBU_MBUS_NO_REMAP (0xffffffff)

#define MVEBU_MBUS_MAX_WINNAME_SZ 32

#ifdef CONFIG_PLAT_ORION
extern const struct mbus_dram_target_info *mv_mbus_dram_info(void);
#else
static inline const struct mbus_dram_target_info *mv_mbus_dram_info(void)
{
	return NULL;
}
#endif

#if defined(MY_ABC_HERE)
int mvebu_mbus_save_cpu_target(u32 *store_addr);
void mvebu_mbus_get_pcie_mem_aperture(struct resource *res);
void mvebu_mbus_get_pcie_io_aperture(struct resource *res);
int mvebu_mbus_add_window_remap_by_id(unsigned int target,
				      unsigned int attribute,
				      phys_addr_t base, size_t size,
				      phys_addr_t remap);
int mvebu_mbus_add_window_by_id(unsigned int target, unsigned int attribute,
				phys_addr_t base, size_t size);
#else  
int mvebu_mbus_add_window_remap_flags(const char *devname, phys_addr_t base,
				      size_t size, phys_addr_t remap,
				      unsigned int flags);
int mvebu_mbus_add_window(const char *devname, phys_addr_t base,
			  size_t size);
#endif  
int mvebu_mbus_del_window(phys_addr_t base, size_t size);
int mvebu_mbus_init(const char *soc, phys_addr_t mbus_phys_base,
		    size_t mbus_size, phys_addr_t sdram_phys_base,
		    size_t sdram_size, int is_coherent);
#if defined(MY_ABC_HERE)
int mvebu_mbus_dt_init(bool is_coherent);
int mvebu_mbus_get_addr_win_info(phys_addr_t phyaddr, u8 *trg_id, u8 *attr);
int mvebu_mbus_win_addr_get(u8 target_id, u8 attribute, u32 *phy_base, u32 *size);
#endif  

#endif  

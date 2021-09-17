#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#include <linux/of.h>	 
#ifndef _POWERPC_PROM_H
#define _POWERPC_PROM_H
#ifdef __KERNEL__

#include <linux/types.h>
#include <asm/irq.h>
#include <linux/atomic.h>

#define HAVE_ARCH_DEVTREE_FIXUPS

extern u64 of_translate_dma_address(struct device_node *dev,
				    const __be32 *in_addr);

#ifdef CONFIG_PCI
extern unsigned long pci_address_to_pio(phys_addr_t address);
#define pci_address_to_pio pci_address_to_pio
#endif	 

void of_parse_dma_window(struct device_node *dn, const void *dma_window_prop,
		unsigned long *busno, unsigned long *phys, unsigned long *size);

extern void kdump_move_device_tree(void);

#if defined(MY_DEF_HERE) || defined(MY_ABC_HERE)
 
#else  
 
struct device_node *of_get_cpu_node(int cpu, unsigned int *thread);
#endif  

struct device_node *of_find_next_cache_node(struct device_node *np);

#ifdef CONFIG_NUMA
extern int of_node_to_nid(struct device_node *device);
#else
static inline int of_node_to_nid(struct device_node *device) { return 0; }
#endif
#define of_node_to_nid of_node_to_nid

extern void of_instantiate_rtc(void);

struct of_drconf_cell {
	u64	base_addr;
	u32	drc_index;
	u32	reserved;
	u32	aa_index;
	u32	flags;
};

#define DRCONF_MEM_ASSIGNED	0x00000008
#define DRCONF_MEM_AI_INVALID	0x00000040
#define DRCONF_MEM_RESERVED	0x00000080

#define OV_IGNORE		0x80	 
#define OV_CESSATION_POLICY	0x40	 

#define OV1_PPC_2_00		0x80	 
#define OV1_PPC_2_01		0x40	 
#define OV1_PPC_2_02		0x20	 
#define OV1_PPC_2_03		0x10	 
#define OV1_PPC_2_04		0x08	 
#define OV1_PPC_2_05		0x04	 
#define OV1_PPC_2_06		0x02	 
#define OV1_PPC_2_07		0x01	 

#define OV2_REAL_MODE		0x20	 

#define OV3_FP			0x80	 
#define OV3_VMX			0x40	 
#define OV3_DFP			0x20	 

#define OV4_MIN_ENT_CAP		0x01	 

#define OV5_FEAT(x)	((x) & 0xff)
#define OV5_INDX(x)	((x) >> 8)
#define OV5_LPAR		0x0280	 
#define OV5_SPLPAR		0x0240	 
 
#define OV5_DRCONF_MEMORY	0x0220
#define OV5_LARGE_PAGES		0x0210	 
#define OV5_DONATE_DEDICATE_CPU	0x0202	 
#define OV5_MSI			0x0201	 
#define OV5_CMO			0x0480	 
#define OV5_XCMO		0x0440	 
#define OV5_TYPE1_AFFINITY	0x0580	 
#define OV5_PRRN		0x0540	 
#define OV5_PFO_HW_RNG		0x0E80	 
#define OV5_PFO_HW_842		0x0E40	 
#define OV5_PFO_HW_ENCR		0x0E20	 
#define OV5_SUB_PROCESSORS	0x0F01	 

#define OV6_LINUX		0x02	 

extern unsigned char ibm_architecture_vec[];

#include <linux/of_fdt.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>

#endif  
#endif  

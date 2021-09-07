#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef MY_DEF_HERE
 
#ifndef __ASM_POWERPC_FSL_85XX_CACHE_SRAM_H__
#define __ASM_POWERPC_FSL_85XX_CACHE_SRAM_H__

#include <asm/rheap.h>
#include <linux/spinlock.h>

struct mpc85xx_cache_sram {
	phys_addr_t base_phys;
	void *base_virt;
	unsigned int size;
	rh_info_t *rh;
	spinlock_t lock;
};

extern void mpc85xx_cache_sram_free(void *ptr);
extern void *mpc85xx_cache_sram_alloc(unsigned int size,
				  phys_addr_t *phys, unsigned int align);

#endif  
#endif  

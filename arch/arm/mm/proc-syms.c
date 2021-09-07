 
#include <linux/module.h>
#include <linux/mm.h>

#include <asm/cacheflush.h>
#include <asm/proc-fns.h>
#include <asm/tlbflush.h>
#include <asm/page.h>

#ifndef MULTI_CPU
EXPORT_SYMBOL(cpu_dcache_clean_area);
EXPORT_SYMBOL(cpu_set_pte_ext);
#else
EXPORT_SYMBOL(processor);
#endif

#ifndef MULTI_CACHE
EXPORT_SYMBOL(__cpuc_flush_kern_all);
EXPORT_SYMBOL(__cpuc_flush_user_all);
EXPORT_SYMBOL(__cpuc_flush_user_range);
EXPORT_SYMBOL(__cpuc_coherent_kern_range);
#ifdef CONFIG_SYNO_PLX_PORTING
EXPORT_SYMBOL(__cpuc_flush_dcache_area);
#else
EXPORT_SYMBOL(__cpuc_flush_dcache_page);
#endif
EXPORT_SYMBOL(dmac_inv_range);   
#else
EXPORT_SYMBOL(cpu_cache);
#endif

#ifdef CONFIG_MMU
#ifndef MULTI_USER
EXPORT_SYMBOL(__cpu_clear_user_highpage);
EXPORT_SYMBOL(__cpu_copy_user_highpage);
#else
EXPORT_SYMBOL(cpu_user);
#endif
#endif

#ifdef MULTI_TLB
EXPORT_SYMBOL(cpu_tlb);
#endif

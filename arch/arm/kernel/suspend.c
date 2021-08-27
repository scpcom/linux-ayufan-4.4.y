#include <linux/init.h>

#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/memory.h>
#include <asm/suspend.h>
#include <asm/tlbflush.h>

static pgd_t *suspend_pgd;

extern int __cpu_suspend(unsigned long, int (*)(unsigned long));
extern void cpu_resume_mmu(void);

void __cpu_suspend_save(u32 *ptr, u32 ptrsz, u32 sp, u32 *save_ptr)
{
#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
	u32 *ctx = ptr;
#else	
	*save_ptr = virt_to_phys(ptr);
#endif
	 
	*ptr++ = virt_to_phys(suspend_pgd);
	*ptr++ = sp;
	*ptr++ = virt_to_phys(cpu_do_resume);

	cpu_do_suspend(ptr);

#ifdef CONFIG_SYNO_ARMADA_ARCH_V2
flush_cache_louis();

	__cpuc_flush_dcache_area(ctx, ptrsz);
	__cpuc_flush_dcache_area(save_ptr, sizeof(*save_ptr));
#else
	flush_cache_all();
#endif  
	outer_clean_range(*save_ptr, *save_ptr + ptrsz);
	outer_clean_range(virt_to_phys(save_ptr),
			  virt_to_phys(save_ptr) + sizeof(*save_ptr));
}

int cpu_suspend(unsigned long arg, int (*fn)(unsigned long))
{
	struct mm_struct *mm = current->active_mm;
	int ret;

	if (!suspend_pgd)
		return -EINVAL;

	ret = __cpu_suspend(arg, fn);
	if (ret == 0) {
		cpu_switch_mm(mm->pgd, mm);
		local_flush_tlb_all();
	}

	return ret;
}

static int __init cpu_suspend_init(void)
{
	suspend_pgd = pgd_alloc(&init_mm);
	if (suspend_pgd) {
		unsigned long addr = virt_to_phys(cpu_resume_mmu);
		identity_mapping_add(suspend_pgd, addr, addr + SECTION_SIZE);
	}
	return suspend_pgd ? 0 : -ENOMEM;
}
core_initcall(cpu_suspend_init);

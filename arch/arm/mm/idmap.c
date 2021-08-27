#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt
#endif

#include <linux/kernel.h>

#include <asm/cputype.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>

#if (defined(MY_DEF_HERE) || defined(MY_DEF_HERE) || defined(MY_DEF_HERE)) && defined(CONFIG_ARM_LPAE)
static void idmap_add_pmd(pud_t *pud, unsigned long addr, unsigned long end,
	unsigned long prot)
{
	pmd_t *pmd;
	unsigned long next;

	if (pud_none_or_clear_bad(pud) || (pud_val(*pud) & L_PGD_SWAPPER)) {
#ifdef MY_DEF_HERE
		pmd = pmd_alloc_one(&init_mm, addr);
#else
		pmd = pmd_alloc_one(NULL, addr);
#endif
		if (!pmd) {
			pr_warning("Failed to allocate identity pmd.\n");
			return;
		}
#ifdef MY_DEF_HERE
		pud_populate(&init_mm, pud, pmd);
#else
		pud_populate(NULL, pud, pmd);
#endif
		pmd += pmd_index(addr);
	} else
		pmd = pmd_offset(pud, addr);

	do {
		next = pmd_addr_end(addr, end);
		*pmd = __pmd((addr & PMD_MASK) | prot);
		flush_pmd_entry(pmd);
	} while (pmd++, addr = next, addr != end);
}
#else	 
static void idmap_add_pmd(pud_t *pud, unsigned long addr, unsigned long end,
	unsigned long prot)
{
	pmd_t *pmd = pmd_offset(pud, addr);

	addr = (addr & PMD_MASK) | prot;
	pmd[0] = __pmd(addr);
	addr += SECTION_SIZE;
	pmd[1] = __pmd(addr);
	flush_pmd_entry(pmd);
}
#endif	 

static void idmap_add_pud(pgd_t *pgd, unsigned long addr, unsigned long end,
	unsigned long prot)
{
	pud_t *pud = pud_offset(pgd, addr);
	unsigned long next;

	do {
		next = pud_addr_end(addr, end);
		idmap_add_pmd(pud, addr, next, prot);
	} while (pud++, addr = next, addr != end);
}

void identity_mapping_add(pgd_t *pgd, unsigned long addr, unsigned long end)
{
	unsigned long prot, next;

#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	prot = PMD_TYPE_SECT | PMD_SECT_AP_WRITE | PMD_SECT_AF;
#else
	prot = PMD_TYPE_SECT | PMD_SECT_AP_WRITE;
#endif
	if (cpu_architecture() <= CPU_ARCH_ARMv5TEJ && !cpu_is_xscale())
		prot |= PMD_BIT4;

	pgd += pgd_index(addr);
	do {
		next = pgd_addr_end(addr, end);
		idmap_add_pud(pgd, addr, next, prot);
	} while (pgd++, addr = next, addr != end);
}

#ifdef CONFIG_SMP
static void idmap_del_pmd(pud_t *pud, unsigned long addr, unsigned long end)
{
#if defined(MY_DEF_HERE) || defined(MY_DEF_HERE)
	pmd_t *pmd;

	if (pud_none_or_clear_bad(pud))
		return;
	pmd = pmd_offset(pud, addr);
#else
	pmd_t *pmd = pmd_offset(pud, addr);
#endif
	pmd_clear(pmd);
}

static void idmap_del_pud(pgd_t *pgd, unsigned long addr, unsigned long end)
{
	pud_t *pud = pud_offset(pgd, addr);
	unsigned long next;

	do {
		next = pud_addr_end(addr, end);
		idmap_del_pmd(pud, addr, next);
	} while (pud++, addr = next, addr != end);
}

void identity_mapping_del(pgd_t *pgd, unsigned long addr, unsigned long end)
{
	unsigned long next;

	pgd += pgd_index(addr);
	do {
		next = pgd_addr_end(addr, end);
		idmap_del_pud(pgd, addr, next);
	} while (pgd++, addr = next, addr != end);
}
#endif

void setup_mm_for_reboot(char mode)
{
	 
	identity_mapping_add(current->active_mm->pgd, 0, TASK_SIZE);
	local_flush_tlb_all();
}

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __ARM_KVM_MMU_H__
#define __ARM_KVM_MMU_H__

#include <asm/memory.h>
#include <asm/page.h>

#define HYP_PAGE_OFFSET_MASK	UL(~0)
#define HYP_PAGE_OFFSET		PAGE_OFFSET
#define KERN_TO_HYP(kva)	(kva)

#define TRAMPOLINE_VA		UL(CONFIG_VECTORS_BASE)

#ifndef __ASSEMBLY__

#include <asm/cacheflush.h>
#include <asm/pgalloc.h>

int create_hyp_mappings(void *from, void *to);
int create_hyp_io_mappings(void *from, void *to, phys_addr_t);
void free_boot_hyp_pgd(void);
void free_hyp_pgds(void);

int kvm_alloc_stage2_pgd(struct kvm *kvm);
void kvm_free_stage2_pgd(struct kvm *kvm);
int kvm_phys_addr_ioremap(struct kvm *kvm, phys_addr_t guest_ipa,
			  phys_addr_t pa, unsigned long size);

int kvm_handle_guest_abort(struct kvm_vcpu *vcpu, struct kvm_run *run);

void kvm_mmu_free_memory_caches(struct kvm_vcpu *vcpu);

phys_addr_t kvm_mmu_get_httbr(void);
phys_addr_t kvm_mmu_get_boot_httbr(void);
phys_addr_t kvm_get_idmap_vector(void);
int kvm_mmu_init(void);
void kvm_clear_hyp_idmap(void);

static inline void kvm_set_pte(pte_t *pte, pte_t new_pte)
{
#if defined(MY_DEF_HERE)
	pte_val(*pte) = pte_val(new_pte);
#else  
	pte_val(*pte) = new_pte;
#endif  
	 
	flush_pmd_entry(pte);
}

static inline bool kvm_is_write_fault(unsigned long hsr)
{
	unsigned long hsr_ec = hsr >> HSR_EC_SHIFT;
	if (hsr_ec == HSR_EC_IABT)
		return false;
	else if ((hsr & HSR_ISV) && !(hsr & HSR_WNR))
		return false;
	else
		return true;
}

static inline void kvm_clean_pgd(pgd_t *pgd)
{
	clean_dcache_area(pgd, PTRS_PER_S2_PGD * sizeof(pgd_t));
}

static inline void kvm_clean_pmd_entry(pmd_t *pmd)
{
	clean_pmd_entry(pmd);
}

static inline void kvm_clean_pte(pte_t *pte)
{
	clean_pte_table(pte);
}

static inline void kvm_set_s2pte_writable(pte_t *pte)
{
	pte_val(*pte) |= L_PTE_S2_RDWR;
}

struct kvm;

static inline void coherent_icache_guest_page(struct kvm *kvm, gfn_t gfn)
{
	 
	if (icache_is_pipt()) {
		unsigned long hva = gfn_to_hva(kvm, gfn);
		__cpuc_coherent_user_range(hva, hva + PAGE_SIZE);
	} else if (!icache_is_vivt_asid_tagged()) {
		 
		__flush_icache_all();
	}
}

#define kvm_flush_dcache_to_poc(a,l)	__cpuc_flush_dcache_area((a), (l))

#endif	 

#endif  

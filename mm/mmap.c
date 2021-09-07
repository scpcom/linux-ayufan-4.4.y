#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/slab.h>
#include <linux/backing-dev.h>
#include <linux/mm.h>
#include <linux/shm.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/syscalls.h>
#include <linux/capability.h>
#include <linux/init.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/personality.h>
#include <linux/security.h>
#include <linux/ima.h>
#include <linux/hugetlb.h>
#include <linux/profile.h>
#include <linux/module.h>
#include <linux/mount.h>
#include <linux/mempolicy.h>
#include <linux/rmap.h>
#include <linux/mmu_notifier.h>
#include <linux/perf_event.h>

#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/tlb.h>
#include <asm/mmu_context.h>

#include "internal.h"

#ifndef arch_mmap_check
#define arch_mmap_check(addr, len, flags)	(0)
#endif

#ifndef arch_rebalance_pgtables
#define arch_rebalance_pgtables(addr, len)		(addr)
#endif

static void unmap_region(struct mm_struct *mm,
		struct vm_area_struct *vma, struct vm_area_struct *prev,
		unsigned long start, unsigned long end);

#undef DEBUG_MM_RB

pgprot_t protection_map[16] = {
	__P000, __P001, __P010, __P011, __P100, __P101, __P110, __P111,
	__S000, __S001, __S010, __S011, __S100, __S101, __S110, __S111
};

pgprot_t vm_get_page_prot(unsigned long vm_flags)
{
	return __pgprot(pgprot_val(protection_map[vm_flags &
				(VM_READ|VM_WRITE|VM_EXEC|VM_SHARED)]) |
			pgprot_val(arch_vm_get_page_prot(vm_flags)));
}
EXPORT_SYMBOL(vm_get_page_prot);

#ifdef MY_ABC_HERE
int sysctl_overcommit_memory = OVERCOMMIT_ALWAYS;
#else
int sysctl_overcommit_memory = OVERCOMMIT_GUESS;   
#endif
int sysctl_overcommit_ratio = 50;	 
int sysctl_max_map_count __read_mostly = DEFAULT_MAX_MAP_COUNT;
struct percpu_counter vm_committed_as;

int __vm_enough_memory(struct mm_struct *mm, long pages, int cap_sys_admin)
{
	unsigned long free, allowed;

	vm_acct_memory(pages);

	if (sysctl_overcommit_memory == OVERCOMMIT_ALWAYS)
		return 0;

	if (sysctl_overcommit_memory == OVERCOMMIT_GUESS) {
		unsigned long n;

		free = global_page_state(NR_FILE_PAGES);
		free += nr_swap_pages;

		free += global_page_state(NR_SLAB_RECLAIMABLE);

		if (!cap_sys_admin)
			free -= free / 32;

		if (free > pages)
			return 0;

		n = nr_free_pages();

		if (n <= totalreserve_pages)
			goto error;
		else
			n -= totalreserve_pages;

		if (!cap_sys_admin)
			n -= n / 32;
		free += n;

		if (free > pages)
			return 0;

		goto error;
	}

	allowed = (totalram_pages - hugetlb_total_pages())
	       	* sysctl_overcommit_ratio / 100;
	 
	if (!cap_sys_admin)
		allowed -= allowed / 32;
	allowed += total_swap_pages;

	if (mm)
		allowed -= mm->total_vm / 32;

	if (percpu_counter_read_positive(&vm_committed_as) < allowed)
		return 0;
error:
	vm_unacct_memory(pages);

	return -ENOMEM;
}

static void __remove_shared_vm_struct(struct vm_area_struct *vma,
		struct file *file, struct address_space *mapping)
{
	if (vma->vm_flags & VM_DENYWRITE)
		atomic_inc(&file->f_path.dentry->d_inode->i_writecount);
	if (vma->vm_flags & VM_SHARED)
		mapping->i_mmap_writable--;

	flush_dcache_mmap_lock(mapping);
	if (unlikely(vma->vm_flags & VM_NONLINEAR))
		list_del_init(&vma->shared.vm_set.list);
	else
		vma_prio_tree_remove(vma, &mapping->i_mmap);
	flush_dcache_mmap_unlock(mapping);
}

void unlink_file_vma(struct vm_area_struct *vma)
{
	struct file *file = vma->vm_file;

	if (file) {
		struct address_space *mapping = file->f_mapping;
#ifdef SYNO_FORCE_UNMOUNT
		if (!blSynostate(O_UNMOUNT_OK, file)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
			printk("%s: force unmount hit\n", __FUNCTION__);
#endif
			return;
		}
#endif
		spin_lock(&mapping->i_mmap_lock);
		__remove_shared_vm_struct(vma, file, mapping);
		spin_unlock(&mapping->i_mmap_lock);
	}
}

static struct vm_area_struct *remove_vma(struct vm_area_struct *vma)
{
	struct vm_area_struct *next = vma->vm_next;

	might_sleep();
	if (vma->vm_ops && vma->vm_ops->close)
		vma->vm_ops->close(vma);
	if (vma->vm_file) {
		fput(vma->vm_file);
		if (vma->vm_flags & VM_EXECUTABLE)
			removed_exe_file_vma(vma->vm_mm);
	}
	mpol_put(vma_policy(vma));
	kmem_cache_free(vm_area_cachep, vma);
	return next;
}

SYSCALL_DEFINE1(brk, unsigned long, brk)
{
	unsigned long rlim, retval;
	unsigned long newbrk, oldbrk;
	struct mm_struct *mm = current->mm;
	unsigned long min_brk;

	down_write(&mm->mmap_sem);

#ifdef CONFIG_COMPAT_BRK
	min_brk = mm->end_code;
#else
	min_brk = mm->start_brk;
#endif
	if (brk < min_brk)
		goto out;

	rlim = current->signal->rlim[RLIMIT_DATA].rlim_cur;
	if (rlim < RLIM_INFINITY && (brk - mm->start_brk) +
			(mm->end_data - mm->start_data) > rlim)
		goto out;

	newbrk = PAGE_ALIGN(brk);
	oldbrk = PAGE_ALIGN(mm->brk);
	if (oldbrk == newbrk)
		goto set_brk;

	if (brk <= mm->brk) {
		if (!do_munmap(mm, newbrk, oldbrk-newbrk))
			goto set_brk;
		goto out;
	}

	if (find_vma_intersection(mm, oldbrk, newbrk+PAGE_SIZE))
		goto out;

	if (do_brk(oldbrk, newbrk-oldbrk) != oldbrk)
		goto out;
set_brk:
	mm->brk = brk;
out:
	retval = mm->brk;
	up_write(&mm->mmap_sem);
	return retval;
}

#ifdef DEBUG_MM_RB
static int browse_rb(struct rb_root *root)
{
	int i = 0, j;
	struct rb_node *nd, *pn = NULL;
	unsigned long prev = 0, pend = 0;

	for (nd = rb_first(root); nd; nd = rb_next(nd)) {
		struct vm_area_struct *vma;
		vma = rb_entry(nd, struct vm_area_struct, vm_rb);
		if (vma->vm_start < prev)
			printk("vm_start %lx prev %lx\n", vma->vm_start, prev), i = -1;
		if (vma->vm_start < pend)
			printk("vm_start %lx pend %lx\n", vma->vm_start, pend);
		if (vma->vm_start > vma->vm_end)
			printk("vm_end %lx < vm_start %lx\n", vma->vm_end, vma->vm_start);
		i++;
		pn = nd;
		prev = vma->vm_start;
		pend = vma->vm_end;
	}
	j = 0;
	for (nd = pn; nd; nd = rb_prev(nd)) {
		j++;
	}
	if (i != j)
		printk("backwards %d, forwards %d\n", j, i), i = 0;
	return i;
}

void validate_mm(struct mm_struct *mm)
{
	int bug = 0;
	int i = 0;
	struct vm_area_struct *tmp = mm->mmap;
	while (tmp) {
		tmp = tmp->vm_next;
		i++;
	}
	if (i != mm->map_count)
		printk("map_count %d vm_next %d\n", mm->map_count, i), bug = 1;
	i = browse_rb(&mm->mm_rb);
	if (i != mm->map_count)
		printk("map_count %d rb %d\n", mm->map_count, i), bug = 1;
	BUG_ON(bug);
}
#else
#define validate_mm(mm) do { } while (0)
#endif

static struct vm_area_struct *
find_vma_prepare(struct mm_struct *mm, unsigned long addr,
		struct vm_area_struct **pprev, struct rb_node ***rb_link,
		struct rb_node ** rb_parent)
{
	struct vm_area_struct * vma;
	struct rb_node ** __rb_link, * __rb_parent, * rb_prev;

	__rb_link = &mm->mm_rb.rb_node;
	rb_prev = __rb_parent = NULL;
	vma = NULL;

	while (*__rb_link) {
		struct vm_area_struct *vma_tmp;

		__rb_parent = *__rb_link;
		vma_tmp = rb_entry(__rb_parent, struct vm_area_struct, vm_rb);

		if (vma_tmp->vm_end > addr) {
			vma = vma_tmp;
			if (vma_tmp->vm_start <= addr)
				break;
			__rb_link = &__rb_parent->rb_left;
		} else {
			rb_prev = __rb_parent;
			__rb_link = &__rb_parent->rb_right;
		}
	}

	*pprev = NULL;
	if (rb_prev)
		*pprev = rb_entry(rb_prev, struct vm_area_struct, vm_rb);
	*rb_link = __rb_link;
	*rb_parent = __rb_parent;
	return vma;
}

static inline void
__vma_link_list(struct mm_struct *mm, struct vm_area_struct *vma,
		struct vm_area_struct *prev, struct rb_node *rb_parent)
{
	if (prev) {
		vma->vm_next = prev->vm_next;
		prev->vm_next = vma;
	} else {
		mm->mmap = vma;
		if (rb_parent)
			vma->vm_next = rb_entry(rb_parent,
					struct vm_area_struct, vm_rb);
		else
			vma->vm_next = NULL;
	}
}

void __vma_link_rb(struct mm_struct *mm, struct vm_area_struct *vma,
		struct rb_node **rb_link, struct rb_node *rb_parent)
{
	rb_link_node(&vma->vm_rb, rb_parent, rb_link);
	rb_insert_color(&vma->vm_rb, &mm->mm_rb);
}

static void __vma_link_file(struct vm_area_struct *vma)
{
	struct file *file;

	file = vma->vm_file;
	if (file) {
		struct address_space *mapping = file->f_mapping;

#ifdef SYNO_FORCE_UNMOUNT
		if (!blSynostate(O_UNMOUNT_OK, file)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
			printk("%s: force unmount hit\n", __FUNCTION__);
#endif
			return;
		}
#endif
		if (vma->vm_flags & VM_DENYWRITE)
			atomic_dec(&file->f_path.dentry->d_inode->i_writecount);
		if (vma->vm_flags & VM_SHARED)
			mapping->i_mmap_writable++;

		flush_dcache_mmap_lock(mapping);
		if (unlikely(vma->vm_flags & VM_NONLINEAR))
			vma_nonlinear_insert(vma, &mapping->i_mmap_nonlinear);
		else
			vma_prio_tree_insert(vma, &mapping->i_mmap);
		flush_dcache_mmap_unlock(mapping);
	}
}

static void
__vma_link(struct mm_struct *mm, struct vm_area_struct *vma,
	struct vm_area_struct *prev, struct rb_node **rb_link,
	struct rb_node *rb_parent)
{
	__vma_link_list(mm, vma, prev, rb_parent);
	__vma_link_rb(mm, vma, rb_link, rb_parent);
	__anon_vma_link(vma);
}

static void vma_link(struct mm_struct *mm, struct vm_area_struct *vma,
			struct vm_area_struct *prev, struct rb_node **rb_link,
			struct rb_node *rb_parent)
{
	struct address_space *mapping = NULL;

#ifdef SYNO_FORCE_UNMOUNT
	if (vma->vm_file && !blSynostate(O_UNMOUNT_OK, vma->vm_file)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
		printk("%s: force unmount hit\n", __FUNCTION__);
#endif
		return;
	}
#endif
	if (vma->vm_file)
		mapping = vma->vm_file->f_mapping;

	if (mapping) {
		spin_lock(&mapping->i_mmap_lock);
		vma->vm_truncate_count = mapping->truncate_count;
	}
	anon_vma_lock(vma);

	__vma_link(mm, vma, prev, rb_link, rb_parent);
	__vma_link_file(vma);

	anon_vma_unlock(vma);
	if (mapping)
		spin_unlock(&mapping->i_mmap_lock);

	mm->map_count++;
	validate_mm(mm);
}

static void __insert_vm_struct(struct mm_struct *mm, struct vm_area_struct *vma)
{
	struct vm_area_struct *__vma, *prev;
	struct rb_node **rb_link, *rb_parent;

	__vma = find_vma_prepare(mm, vma->vm_start,&prev, &rb_link, &rb_parent);
	BUG_ON(__vma && __vma->vm_start < vma->vm_end);
	__vma_link(mm, vma, prev, rb_link, rb_parent);
	mm->map_count++;
}

static inline void
__vma_unlink(struct mm_struct *mm, struct vm_area_struct *vma,
		struct vm_area_struct *prev)
{
	prev->vm_next = vma->vm_next;
	rb_erase(&vma->vm_rb, &mm->mm_rb);
	if (mm->mmap_cache == vma)
		mm->mmap_cache = prev;
}

void vma_adjust(struct vm_area_struct *vma, unsigned long start,
	unsigned long end, pgoff_t pgoff, struct vm_area_struct *insert)
{
	struct mm_struct *mm = vma->vm_mm;
	struct vm_area_struct *next = vma->vm_next;
	struct vm_area_struct *importer = NULL;
	struct address_space *mapping = NULL;
	struct prio_tree_root *root = NULL;
	struct file *file = vma->vm_file;
	struct anon_vma *anon_vma = NULL;
	long adjust_next = 0;
	int remove_next = 0;

	if (next && !insert) {
		if (end >= next->vm_end) {
			 
again:			remove_next = 1 + (end > next->vm_end);
			end = next->vm_end;
			anon_vma = next->anon_vma;
			importer = vma;
		} else if (end > next->vm_start) {
			 
			adjust_next = (end - next->vm_start) >> PAGE_SHIFT;
			anon_vma = next->anon_vma;
			importer = vma;
		} else if (end < vma->vm_end) {
			 
			adjust_next = - ((vma->vm_end - end) >> PAGE_SHIFT);
			anon_vma = next->anon_vma;
			importer = next;
		}
	}

	if (file) {
		mapping = file->f_mapping;
		if (!(vma->vm_flags & VM_NONLINEAR))
			root = &mapping->i_mmap;
		spin_lock(&mapping->i_mmap_lock);
		if (importer &&
		    vma->vm_truncate_count != next->vm_truncate_count) {
			 
			importer->vm_truncate_count = 0;
		}
		if (insert) {
			insert->vm_truncate_count = vma->vm_truncate_count;
			 
			__vma_link_file(insert);
		}
	}

	if (vma->anon_vma && (insert || importer || start != vma->vm_start))
		anon_vma = vma->anon_vma;
	if (anon_vma) {
		spin_lock(&anon_vma->lock);
		 
		if (importer && !importer->anon_vma) {
			importer->anon_vma = anon_vma;
			__anon_vma_link(importer);
		}
	}

	if (root) {
		flush_dcache_mmap_lock(mapping);
		vma_prio_tree_remove(vma, root);
		if (adjust_next)
			vma_prio_tree_remove(next, root);
	}

	vma->vm_start = start;
	vma->vm_end = end;
	vma->vm_pgoff = pgoff;
	if (adjust_next) {
		next->vm_start += adjust_next << PAGE_SHIFT;
		next->vm_pgoff += adjust_next;
	}

	if (root) {
		if (adjust_next)
			vma_prio_tree_insert(next, root);
		vma_prio_tree_insert(vma, root);
		flush_dcache_mmap_unlock(mapping);
	}

	if (remove_next) {
		 
		__vma_unlink(mm, next, vma);
		if (file)
			__remove_shared_vm_struct(next, file, mapping);
		if (next->anon_vma)
			__anon_vma_merge(vma, next);
	} else if (insert) {
		 
		__insert_vm_struct(mm, insert);
	}

	if (anon_vma)
		spin_unlock(&anon_vma->lock);
	if (mapping)
		spin_unlock(&mapping->i_mmap_lock);

	if (remove_next) {
		if (file) {
			fput(file);
			if (next->vm_flags & VM_EXECUTABLE)
				removed_exe_file_vma(mm);
		}
		mm->map_count--;
		mpol_put(vma_policy(next));
		kmem_cache_free(vm_area_cachep, next);
		 
		if (remove_next == 2) {
			next = vma->vm_next;
			goto again;
		}
	}

	validate_mm(mm);
}

static inline int is_mergeable_vma(struct vm_area_struct *vma,
			struct file *file, unsigned long vm_flags)
{
	 
	if ((vma->vm_flags ^ vm_flags) & ~VM_CAN_NONLINEAR)
		return 0;
	if (vma->vm_file != file)
		return 0;
	if (vma->vm_ops && vma->vm_ops->close)
		return 0;
	return 1;
}

static inline int is_mergeable_anon_vma(struct anon_vma *anon_vma1,
					struct anon_vma *anon_vma2)
{
	return !anon_vma1 || !anon_vma2 || (anon_vma1 == anon_vma2);
}

static int
can_vma_merge_before(struct vm_area_struct *vma, unsigned long vm_flags,
	struct anon_vma *anon_vma, struct file *file, pgoff_t vm_pgoff)
{
	if (is_mergeable_vma(vma, file, vm_flags) &&
	    is_mergeable_anon_vma(anon_vma, vma->anon_vma)) {
		if (vma->vm_pgoff == vm_pgoff)
			return 1;
	}
	return 0;
}

static int
can_vma_merge_after(struct vm_area_struct *vma, unsigned long vm_flags,
	struct anon_vma *anon_vma, struct file *file, pgoff_t vm_pgoff)
{
	if (is_mergeable_vma(vma, file, vm_flags) &&
	    is_mergeable_anon_vma(anon_vma, vma->anon_vma)) {
		pgoff_t vm_pglen;
		vm_pglen = (vma->vm_end - vma->vm_start) >> PAGE_SHIFT;
		if (vma->vm_pgoff + vm_pglen == vm_pgoff)
			return 1;
	}
	return 0;
}

struct vm_area_struct *vma_merge(struct mm_struct *mm,
			struct vm_area_struct *prev, unsigned long addr,
			unsigned long end, unsigned long vm_flags,
		     	struct anon_vma *anon_vma, struct file *file,
			pgoff_t pgoff, struct mempolicy *policy)
{
	pgoff_t pglen = (end - addr) >> PAGE_SHIFT;
	struct vm_area_struct *area, *next;

	if (vm_flags & VM_SPECIAL)
		return NULL;

	if (prev)
		next = prev->vm_next;
	else
		next = mm->mmap;
	area = next;
	if (next && next->vm_end == end)		 
		next = next->vm_next;

	if (prev && prev->vm_end == addr &&
  			mpol_equal(vma_policy(prev), policy) &&
			can_vma_merge_after(prev, vm_flags,
						anon_vma, file, pgoff)) {
		 
		if (next && end == next->vm_start &&
				mpol_equal(policy, vma_policy(next)) &&
				can_vma_merge_before(next, vm_flags,
					anon_vma, file, pgoff+pglen) &&
				is_mergeable_anon_vma(prev->anon_vma,
						      next->anon_vma)) {
							 
			vma_adjust(prev, prev->vm_start,
				next->vm_end, prev->vm_pgoff, NULL);
		} else					 
			vma_adjust(prev, prev->vm_start,
				end, prev->vm_pgoff, NULL);
		return prev;
	}

	if (next && end == next->vm_start &&
 			mpol_equal(policy, vma_policy(next)) &&
			can_vma_merge_before(next, vm_flags,
					anon_vma, file, pgoff+pglen)) {
		if (prev && addr < prev->vm_end)	 
			vma_adjust(prev, prev->vm_start,
				addr, prev->vm_pgoff, NULL);
		else					 
			vma_adjust(area, addr, next->vm_end,
				next->vm_pgoff - pglen, NULL);
		return area;
	}

	return NULL;
}

struct anon_vma *find_mergeable_anon_vma(struct vm_area_struct *vma)
{
	struct vm_area_struct *near;
	unsigned long vm_flags;

	near = vma->vm_next;
	if (!near)
		goto try_prev;

	vm_flags = vma->vm_flags & ~(VM_READ|VM_WRITE|VM_EXEC);
	vm_flags |= near->vm_flags & (VM_READ|VM_WRITE|VM_EXEC);

	if (near->anon_vma && vma->vm_end == near->vm_start &&
 			mpol_equal(vma_policy(vma), vma_policy(near)) &&
			can_vma_merge_before(near, vm_flags,
				NULL, vma->vm_file, vma->vm_pgoff +
				((vma->vm_end - vma->vm_start) >> PAGE_SHIFT)))
		return near->anon_vma;
try_prev:
	 
	BUG_ON(find_vma_prev(vma->vm_mm, vma->vm_start, &near) != vma);
	if (!near)
		goto none;

	vm_flags = vma->vm_flags & ~(VM_READ|VM_WRITE|VM_EXEC);
	vm_flags |= near->vm_flags & (VM_READ|VM_WRITE|VM_EXEC);

	if (near->anon_vma && near->vm_end == vma->vm_start &&
  			mpol_equal(vma_policy(near), vma_policy(vma)) &&
			can_vma_merge_after(near, vm_flags,
				NULL, vma->vm_file, vma->vm_pgoff))
		return near->anon_vma;
none:
	 
	return NULL;
}

#ifdef CONFIG_PROC_FS
void vm_stat_account(struct mm_struct *mm, unsigned long flags,
						struct file *file, long pages)
{
	const unsigned long stack_flags
		= VM_STACK_FLAGS & (VM_GROWSUP|VM_GROWSDOWN);

	if (file) {
		mm->shared_vm += pages;
		if ((flags & (VM_EXEC|VM_WRITE)) == VM_EXEC)
			mm->exec_vm += pages;
	} else if (flags & stack_flags)
		mm->stack_vm += pages;
	if (flags & (VM_RESERVED|VM_IO))
		mm->reserved_vm += pages;
}
#endif  

unsigned long do_mmap_pgoff(struct file *file, unsigned long addr,
			unsigned long len, unsigned long prot,
			unsigned long flags, unsigned long pgoff)
{
	struct mm_struct * mm = current->mm;
	struct inode *inode;
	unsigned int vm_flags;
	int error;
	unsigned long reqprot = prot;

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	if (file && unlikely(file->inode) && unlikely(file->inode->filemap_info.map)) {
printk("do_mmap_pgoff() File %p denying due to FAST mode\n", file);
		return -EPERM;
	}
#endif  

#ifdef SYNO_FORCE_UNMOUNT
	if (file) {
		if (!blSynostate(O_UNMOUNT_OK, file)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
			printk("%s: force unmount hit\n", __FUNCTION__);
#endif
			return -EINVAL;
		}
	}
#endif
	 
	if ((prot & PROT_READ) && (current->personality & READ_IMPLIES_EXEC))
		if (!(file && (file->f_path.mnt->mnt_flags & MNT_NOEXEC)))
			prot |= PROT_EXEC;

	if (!len)
		return -EINVAL;

	if (!(flags & MAP_FIXED))
		addr = round_hint_to_min(addr);

	len = PAGE_ALIGN(len);
	if (!len)
		return -ENOMEM;

	if ((pgoff + (len >> PAGE_SHIFT)) < pgoff)
               return -EOVERFLOW;

	if (mm->map_count > sysctl_max_map_count)
		return -ENOMEM;

	addr = get_unmapped_area(file, addr, len, pgoff, flags);
	if (addr & ~PAGE_MASK)
		return addr;

	vm_flags = calc_vm_prot_bits(prot) | calc_vm_flag_bits(flags) |
			mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;

	if (flags & MAP_LOCKED)
		if (!can_do_mlock())
			return -EPERM;

	if (vm_flags & VM_LOCKED) {
		unsigned long locked, lock_limit;
		locked = len >> PAGE_SHIFT;
		locked += mm->locked_vm;
		lock_limit = current->signal->rlim[RLIMIT_MEMLOCK].rlim_cur;
		lock_limit >>= PAGE_SHIFT;
		if (locked > lock_limit && !capable(CAP_IPC_LOCK))
			return -EAGAIN;
	}

	inode = file ? file->f_path.dentry->d_inode : NULL;

	if (file) {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			if ((prot&PROT_WRITE) && !(file->f_mode&FMODE_WRITE))
				return -EACCES;

			if (IS_APPEND(inode) && (file->f_mode & FMODE_WRITE))
				return -EACCES;

			if (locks_verify_locked(inode))
				return -EAGAIN;

			vm_flags |= VM_SHARED | VM_MAYSHARE;
			if (!(file->f_mode & FMODE_WRITE))
				vm_flags &= ~(VM_MAYWRITE | VM_SHARED);

		case MAP_PRIVATE:
			if (!(file->f_mode & FMODE_READ))
				return -EACCES;
			if (file->f_path.mnt->mnt_flags & MNT_NOEXEC) {
				if (vm_flags & VM_EXEC)
					return -EPERM;
				vm_flags &= ~VM_MAYEXEC;
			}

			if (!file->f_op || !file->f_op->mmap)
				return -ENODEV;
			break;

		default:
			return -EINVAL;
		}
	} else {
		switch (flags & MAP_TYPE) {
		case MAP_SHARED:
			 
			pgoff = 0;
			vm_flags |= VM_SHARED | VM_MAYSHARE;
			break;
		case MAP_PRIVATE:
			 
			pgoff = addr >> PAGE_SHIFT;
			break;
		default:
			return -EINVAL;
		}
	}

	error = security_file_mmap(file, reqprot, prot, flags, addr, 0);
	if (error)
		return error;
	error = ima_file_mmap(file, prot);
	if (error)
		return error;

	return mmap_region(file, addr, len, flags, vm_flags, pgoff);
}
EXPORT_SYMBOL(do_mmap_pgoff);

int vma_wants_writenotify(struct vm_area_struct *vma)
{
	unsigned int vm_flags = vma->vm_flags;

	if ((vm_flags & (VM_WRITE|VM_SHARED)) != ((VM_WRITE|VM_SHARED)))
		return 0;

	if (vma->vm_ops && vma->vm_ops->page_mkwrite)
		return 1;

	if (pgprot_val(vma->vm_page_prot) !=
	    pgprot_val(vm_get_page_prot(vm_flags)))
		return 0;

	if (vm_flags & (VM_PFNMAP|VM_INSERTPAGE))
		return 0;

	return vma->vm_file && vma->vm_file->f_mapping &&
		mapping_cap_account_dirty(vma->vm_file->f_mapping);
}

static inline int accountable_mapping(struct file *file, unsigned int vm_flags)
{
	 
	if (file && is_file_hugepages(file))
		return 0;

	return (vm_flags & (VM_NORESERVE | VM_SHARED | VM_WRITE)) == VM_WRITE;
}

unsigned long mmap_region(struct file *file, unsigned long addr,
			  unsigned long len, unsigned long flags,
			  unsigned int vm_flags, unsigned long pgoff)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma, *prev;
	int correct_wcount = 0;
	int error;
	struct rb_node **rb_link, *rb_parent;
	unsigned long charged = 0;
	struct inode *inode =  file ? file->f_path.dentry->d_inode : NULL;

	error = -ENOMEM;
munmap_back:
	vma = find_vma_prepare(mm, addr, &prev, &rb_link, &rb_parent);
	if (vma && vma->vm_start < addr + len) {
		if (do_munmap(mm, addr, len))
			return -ENOMEM;
		goto munmap_back;
	}

	if (!may_expand_vm(mm, len >> PAGE_SHIFT))
		return -ENOMEM;

	if ((flags & MAP_NORESERVE)) {
		 
		if (sysctl_overcommit_memory != OVERCOMMIT_NEVER)
			vm_flags |= VM_NORESERVE;

		if (file && is_file_hugepages(file))
			vm_flags |= VM_NORESERVE;
	}

	if (accountable_mapping(file, vm_flags)) {
		charged = len >> PAGE_SHIFT;
		if (security_vm_enough_memory(charged))
			return -ENOMEM;
		vm_flags |= VM_ACCOUNT;
	}

	vma = vma_merge(mm, prev, addr, addr + len, vm_flags, NULL, file, pgoff, NULL);
	if (vma)
		goto out;

	vma = kmem_cache_zalloc(vm_area_cachep, GFP_KERNEL);
	if (!vma) {
		error = -ENOMEM;
		goto unacct_error;
	}

	vma->vm_mm = mm;
	vma->vm_start = addr;
	vma->vm_end = addr + len;
	vma->vm_flags = vm_flags;
	vma->vm_page_prot = vm_get_page_prot(vm_flags);
	vma->vm_pgoff = pgoff;

	if (file) {
		error = -EINVAL;
#ifdef SYNO_FORCE_UNMOUNT
		if (!blSynostate(O_UNMOUNT_OK, file)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
			printk("%s: force unmount hit\n", __FUNCTION__);
#endif
			goto free_vma;
		}
#endif
		if (vm_flags & (VM_GROWSDOWN|VM_GROWSUP))
			goto free_vma;
		if (vm_flags & VM_DENYWRITE) {
			error = deny_write_access(file);
			if (error)
				goto free_vma;
			correct_wcount = 1;
		}
		vma->vm_file = file;
		get_file(file);
		error = file->f_op->mmap(file, vma);
		if (error)
			goto unmap_and_free_vma;
		if (vm_flags & VM_EXECUTABLE)
			added_exe_file_vma(mm);

		addr = vma->vm_start;
		pgoff = vma->vm_pgoff;
		vm_flags = vma->vm_flags;
	} else if (vm_flags & VM_SHARED) {
		error = shmem_zero_setup(vma);
		if (error)
			goto free_vma;
	}

	if (vma_wants_writenotify(vma))
		vma->vm_page_prot = vm_get_page_prot(vm_flags & ~VM_SHARED);

	vma_link(mm, vma, prev, rb_link, rb_parent);
#ifdef SYNO_FORCE_UNMOUNT
		if (vma->vm_file && !blSynostate(O_UNMOUNT_OK, vma->vm_file)) {
#ifdef SYNO_DEBUG_FORCE_UNMOUNT
			printk("%s: force unmount hit\n", __FUNCTION__);
#endif
			if (file) {
				goto unmap_and_free_vma;
			} else {
				goto free_vma;
			}
		}
#endif
	file = vma->vm_file;

	if (correct_wcount)
		atomic_inc(&inode->i_writecount);
out:
	perf_event_mmap(vma);

	mm->total_vm += len >> PAGE_SHIFT;
	vm_stat_account(mm, vm_flags, file, len >> PAGE_SHIFT);
	if (vm_flags & VM_LOCKED) {
		 
		long nr_pages = mlock_vma_pages_range(vma, addr, addr + len);
		if (nr_pages < 0)
			return nr_pages;	 
		mm->locked_vm += (len >> PAGE_SHIFT) - nr_pages;
	} else if ((flags & MAP_POPULATE) && !(flags & MAP_NONBLOCK))
		make_pages_present(addr, addr + len);
	return addr;

unmap_and_free_vma:
	if (correct_wcount)
		atomic_inc(&inode->i_writecount);
	vma->vm_file = NULL;
	fput(file);

	unmap_region(mm, vma, prev, vma->vm_start, vma->vm_end);
	charged = 0;
free_vma:
	kmem_cache_free(vm_area_cachep, vma);
unacct_error:
	if (charged)
		vm_unacct_memory(charged);
	return error;
}

#ifndef HAVE_ARCH_UNMAPPED_AREA
unsigned long
arch_get_unmapped_area(struct file *filp, unsigned long addr,
		unsigned long len, unsigned long pgoff, unsigned long flags)
{
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	unsigned long start_addr;

	if (len > TASK_SIZE)
		return -ENOMEM;

	if (flags & MAP_FIXED)
		return addr;

	if (addr) {
		addr = PAGE_ALIGN(addr);
		vma = find_vma(mm, addr);
		if (TASK_SIZE - len >= addr &&
		    (!vma || addr + len <= vma->vm_start))
			return addr;
	}
	if (len > mm->cached_hole_size) {
	        start_addr = addr = mm->free_area_cache;
	} else {
	        start_addr = addr = TASK_UNMAPPED_BASE;
	        mm->cached_hole_size = 0;
	}

full_search:
	for (vma = find_vma(mm, addr); ; vma = vma->vm_next) {
		 
		if (TASK_SIZE - len < addr) {
			 
			if (start_addr != TASK_UNMAPPED_BASE) {
				addr = TASK_UNMAPPED_BASE;
			        start_addr = addr;
				mm->cached_hole_size = 0;
				goto full_search;
			}
			return -ENOMEM;
		}
		if (!vma || addr + len <= vma->vm_start) {
			 
			mm->free_area_cache = addr + len;
			return addr;
		}
		if (addr + mm->cached_hole_size < vma->vm_start)
		        mm->cached_hole_size = vma->vm_start - addr;
		addr = vma->vm_end;
	}
}
#endif	

void arch_unmap_area(struct mm_struct *mm, unsigned long addr)
{
	 
	if (addr >= TASK_UNMAPPED_BASE && addr < mm->free_area_cache) {
		mm->free_area_cache = addr;
		mm->cached_hole_size = ~0UL;
	}
}

#ifndef HAVE_ARCH_UNMAPPED_AREA_TOPDOWN
unsigned long
arch_get_unmapped_area_topdown(struct file *filp, const unsigned long addr0,
			  const unsigned long len, const unsigned long pgoff,
			  const unsigned long flags)
{
	struct vm_area_struct *vma;
	struct mm_struct *mm = current->mm;
	unsigned long addr = addr0;

	if (len > TASK_SIZE)
		return -ENOMEM;

	if (flags & MAP_FIXED)
		return addr;

	if (addr) {
		addr = PAGE_ALIGN(addr);
		vma = find_vma(mm, addr);
		if (TASK_SIZE - len >= addr &&
				(!vma || addr + len <= vma->vm_start))
			return addr;
	}

	if (len <= mm->cached_hole_size) {
 	        mm->cached_hole_size = 0;
 		mm->free_area_cache = mm->mmap_base;
 	}

	addr = mm->free_area_cache;

	if (addr > len) {
		vma = find_vma(mm, addr-len);
		if (!vma || addr <= vma->vm_start)
			 
			return (mm->free_area_cache = addr-len);
	}

	if (mm->mmap_base < len)
		goto bottomup;

	addr = mm->mmap_base-len;

	do {
		 
		vma = find_vma(mm, addr);
		if (!vma || addr+len <= vma->vm_start)
			 
			return (mm->free_area_cache = addr);

 		if (addr + mm->cached_hole_size < vma->vm_start)
 		        mm->cached_hole_size = vma->vm_start - addr;

		addr = vma->vm_start-len;
	} while (len < vma->vm_start);

bottomup:
	 
	mm->cached_hole_size = ~0UL;
  	mm->free_area_cache = TASK_UNMAPPED_BASE;
	addr = arch_get_unmapped_area(filp, addr0, len, pgoff, flags);
	 
	mm->free_area_cache = mm->mmap_base;
	mm->cached_hole_size = ~0UL;

	return addr;
}
#endif

void arch_unmap_area_topdown(struct mm_struct *mm, unsigned long addr)
{
	 
	if (addr > mm->free_area_cache)
		mm->free_area_cache = addr;

	if (mm->free_area_cache > mm->mmap_base)
		mm->free_area_cache = mm->mmap_base;
}

unsigned long
get_unmapped_area(struct file *file, unsigned long addr, unsigned long len,
		unsigned long pgoff, unsigned long flags)
{
	unsigned long (*get_area)(struct file *, unsigned long,
				  unsigned long, unsigned long, unsigned long);

	unsigned long error = arch_mmap_check(addr, len, flags);
	if (error)
		return error;

	if (len > TASK_SIZE)
		return -ENOMEM;

	get_area = current->mm->get_unmapped_area;
	if (file && file->f_op && file->f_op->get_unmapped_area)
		get_area = file->f_op->get_unmapped_area;
	addr = get_area(file, addr, len, pgoff, flags);
	if (IS_ERR_VALUE(addr))
		return addr;

	if (addr > TASK_SIZE - len)
		return -ENOMEM;
	if (addr & ~PAGE_MASK)
		return -EINVAL;

	return arch_rebalance_pgtables(addr, len);
}

EXPORT_SYMBOL(get_unmapped_area);

struct vm_area_struct *find_vma(struct mm_struct *mm, unsigned long addr)
{
	struct vm_area_struct *vma = NULL;

	if (mm) {
		 
		vma = mm->mmap_cache;
		if (!(vma && vma->vm_end > addr && vma->vm_start <= addr)) {
			struct rb_node * rb_node;

			rb_node = mm->mm_rb.rb_node;
			vma = NULL;

			while (rb_node) {
				struct vm_area_struct * vma_tmp;

				vma_tmp = rb_entry(rb_node,
						struct vm_area_struct, vm_rb);

				if (vma_tmp->vm_end > addr) {
					vma = vma_tmp;
					if (vma_tmp->vm_start <= addr)
						break;
					rb_node = rb_node->rb_left;
				} else
					rb_node = rb_node->rb_right;
			}
			if (vma)
				mm->mmap_cache = vma;
		}
	}
	return vma;
}

EXPORT_SYMBOL(find_vma);

struct vm_area_struct *
find_vma_prev(struct mm_struct *mm, unsigned long addr,
			struct vm_area_struct **pprev)
{
	struct vm_area_struct *vma = NULL, *prev = NULL;
	struct rb_node *rb_node;
	if (!mm)
		goto out;

	vma = mm->mmap;

	rb_node = mm->mm_rb.rb_node;

	while (rb_node) {
		struct vm_area_struct *vma_tmp;
		vma_tmp = rb_entry(rb_node, struct vm_area_struct, vm_rb);

		if (addr < vma_tmp->vm_end) {
			rb_node = rb_node->rb_left;
		} else {
			prev = vma_tmp;
			if (!prev->vm_next || (addr < prev->vm_next->vm_end))
				break;
			rb_node = rb_node->rb_right;
		}
	}

out:
	*pprev = prev;
	return prev ? prev->vm_next : vma;
}

static int acct_stack_growth(struct vm_area_struct *vma, unsigned long size, unsigned long grow)
{
	struct mm_struct *mm = vma->vm_mm;
	struct rlimit *rlim = current->signal->rlim;
	unsigned long new_start;

	if (!may_expand_vm(mm, grow))
		return -ENOMEM;

	if (size > rlim[RLIMIT_STACK].rlim_cur)
		return -ENOMEM;

	if (vma->vm_flags & VM_LOCKED) {
		unsigned long locked;
		unsigned long limit;
		locked = mm->locked_vm + grow;
		limit = rlim[RLIMIT_MEMLOCK].rlim_cur >> PAGE_SHIFT;
		if (locked > limit && !capable(CAP_IPC_LOCK))
			return -ENOMEM;
	}

	new_start = (vma->vm_flags & VM_GROWSUP) ? vma->vm_start :
			vma->vm_end - size;
	if (is_hugepage_only_range(vma->vm_mm, new_start, size))
		return -EFAULT;

	if (security_vm_enough_memory_mm(mm, grow))
		return -ENOMEM;

	mm->total_vm += grow;
	if (vma->vm_flags & VM_LOCKED)
		mm->locked_vm += grow;
	vm_stat_account(mm, vma->vm_flags, vma->vm_file, grow);
	return 0;
}

#if defined(CONFIG_STACK_GROWSUP) || defined(CONFIG_IA64)
 
#ifndef CONFIG_IA64
static
#endif
int expand_upwards(struct vm_area_struct *vma, unsigned long address)
{
	int error;

	if (!(vma->vm_flags & VM_GROWSUP))
		return -EFAULT;

	if (unlikely(anon_vma_prepare(vma)))
		return -ENOMEM;
	anon_vma_lock(vma);

	if (address < PAGE_ALIGN(address+4))
		address = PAGE_ALIGN(address+4);
	else {
		anon_vma_unlock(vma);
		return -ENOMEM;
	}
	error = 0;

	if (address > vma->vm_end) {
		unsigned long size, grow;

		size = address - vma->vm_start;
		grow = (address - vma->vm_end) >> PAGE_SHIFT;

		error = acct_stack_growth(vma, size, grow);
		if (!error)
			vma->vm_end = address;
	}
	anon_vma_unlock(vma);
	return error;
}
#endif  

static int expand_downwards(struct vm_area_struct *vma,
				   unsigned long address)
{
	int error;

	if (unlikely(anon_vma_prepare(vma)))
		return -ENOMEM;

	address &= PAGE_MASK;
	error = security_file_mmap(NULL, 0, 0, 0, address, 1);
	if (error)
		return error;

	anon_vma_lock(vma);

	if (address < vma->vm_start) {
		unsigned long size, grow;

		size = vma->vm_end - address;
		grow = (vma->vm_start - address) >> PAGE_SHIFT;

		error = acct_stack_growth(vma, size, grow);
		if (!error) {
			vma->vm_start = address;
			vma->vm_pgoff -= grow;
		}
	}
	anon_vma_unlock(vma);
	return error;
}

int expand_stack_downwards(struct vm_area_struct *vma, unsigned long address)
{
	return expand_downwards(vma, address);
}

#ifdef CONFIG_STACK_GROWSUP
int expand_stack(struct vm_area_struct *vma, unsigned long address)
{
	return expand_upwards(vma, address);
}

struct vm_area_struct *
find_extend_vma(struct mm_struct *mm, unsigned long addr)
{
	struct vm_area_struct *vma, *prev;

	addr &= PAGE_MASK;
	vma = find_vma_prev(mm, addr, &prev);
	if (vma && (vma->vm_start <= addr))
		return vma;
	if (!prev || expand_stack(prev, addr))
		return NULL;
	if (prev->vm_flags & VM_LOCKED) {
		if (mlock_vma_pages_range(prev, addr, prev->vm_end) < 0)
			return NULL;	 
	}
	return prev;
}
#else
int expand_stack(struct vm_area_struct *vma, unsigned long address)
{
	return expand_downwards(vma, address);
}

struct vm_area_struct *
find_extend_vma(struct mm_struct * mm, unsigned long addr)
{
	struct vm_area_struct * vma;
	unsigned long start;

	addr &= PAGE_MASK;
	vma = find_vma(mm,addr);
	if (!vma)
		return NULL;
	if (vma->vm_start <= addr)
		return vma;
	if (!(vma->vm_flags & VM_GROWSDOWN))
		return NULL;
	start = vma->vm_start;
	if (expand_stack(vma, addr))
		return NULL;
	if (vma->vm_flags & VM_LOCKED) {
		if (mlock_vma_pages_range(vma, addr, start) < 0)
			return NULL;	 
	}
	return vma;
}
#endif

static void remove_vma_list(struct mm_struct *mm, struct vm_area_struct *vma)
{
	 
	update_hiwater_vm(mm);
	do {
		long nrpages = vma_pages(vma);

		mm->total_vm -= nrpages;
		vm_stat_account(mm, vma->vm_flags, vma->vm_file, -nrpages);
		vma = remove_vma(vma);
	} while (vma);
	validate_mm(mm);
}

static void unmap_region(struct mm_struct *mm,
		struct vm_area_struct *vma, struct vm_area_struct *prev,
		unsigned long start, unsigned long end)
{
	struct vm_area_struct *next = prev? prev->vm_next: mm->mmap;
	struct mmu_gather *tlb;
	unsigned long nr_accounted = 0;

	lru_add_drain();
	tlb = tlb_gather_mmu(mm, 0);
	update_hiwater_rss(mm);
	unmap_vmas(&tlb, vma, start, end, &nr_accounted, NULL);
	vm_unacct_memory(nr_accounted);
	free_pgtables(tlb, vma, prev? prev->vm_end: FIRST_USER_ADDRESS,
				 next? next->vm_start: 0);
	tlb_finish_mmu(tlb, start, end);
}

static void
detach_vmas_to_be_unmapped(struct mm_struct *mm, struct vm_area_struct *vma,
	struct vm_area_struct *prev, unsigned long end)
{
	struct vm_area_struct **insertion_point;
	struct vm_area_struct *tail_vma = NULL;
	unsigned long addr;

	insertion_point = (prev ? &prev->vm_next : &mm->mmap);
	do {
		rb_erase(&vma->vm_rb, &mm->mm_rb);
		mm->map_count--;
		tail_vma = vma;
		vma = vma->vm_next;
	} while (vma && vma->vm_start < end);
	*insertion_point = vma;
	tail_vma->vm_next = NULL;
	if (mm->unmap_area == arch_unmap_area)
		addr = prev ? prev->vm_end : mm->mmap_base;
	else
		addr = vma ?  vma->vm_start : mm->mmap_base;
	mm->unmap_area(mm, addr);
	mm->mmap_cache = NULL;		 
}

int split_vma(struct mm_struct * mm, struct vm_area_struct * vma,
	      unsigned long addr, int new_below)
{
	struct mempolicy *pol;
	struct vm_area_struct *new;

	if (is_vm_hugetlb_page(vma) && (addr &
					~(huge_page_mask(hstate_vma(vma)))))
		return -EINVAL;

	if (mm->map_count >= sysctl_max_map_count)
		return -ENOMEM;

	new = kmem_cache_alloc(vm_area_cachep, GFP_KERNEL);
	if (!new)
		return -ENOMEM;

	*new = *vma;

	if (new_below)
		new->vm_end = addr;
	else {
		new->vm_start = addr;
		new->vm_pgoff += ((addr - vma->vm_start) >> PAGE_SHIFT);
	}

	pol = mpol_dup(vma_policy(vma));
	if (IS_ERR(pol)) {
		kmem_cache_free(vm_area_cachep, new);
		return PTR_ERR(pol);
	}
	vma_set_policy(new, pol);

	if (new->vm_file) {
		get_file(new->vm_file);
		if (vma->vm_flags & VM_EXECUTABLE)
			added_exe_file_vma(mm);
	}

	if (new->vm_ops && new->vm_ops->open)
		new->vm_ops->open(new);

	if (new_below)
		vma_adjust(vma, addr, vma->vm_end, vma->vm_pgoff +
			((addr - new->vm_start) >> PAGE_SHIFT), new);
	else
		vma_adjust(vma, vma->vm_start, addr, vma->vm_pgoff, new);

	return 0;
}

int do_munmap(struct mm_struct *mm, unsigned long start, size_t len)
{
	unsigned long end;
	struct vm_area_struct *vma, *prev, *last;

	if ((start & ~PAGE_MASK) || start > TASK_SIZE || len > TASK_SIZE-start)
		return -EINVAL;

	if ((len = PAGE_ALIGN(len)) == 0)
		return -EINVAL;

	vma = find_vma_prev(mm, start, &prev);
	if (!vma)
		return 0;
	 
	end = start + len;
	if (vma->vm_start >= end)
		return 0;

	if (start > vma->vm_start) {
		int error = split_vma(mm, vma, start, 0);
		if (error)
			return error;
		prev = vma;
	}

	last = find_vma(mm, end);
	if (last && end > last->vm_start) {
		int error = split_vma(mm, last, end, 1);
		if (error)
			return error;
	}
	vma = prev? prev->vm_next: mm->mmap;

	if (mm->locked_vm) {
		struct vm_area_struct *tmp = vma;
		while (tmp && tmp->vm_start < end) {
			if (tmp->vm_flags & VM_LOCKED) {
				mm->locked_vm -= vma_pages(tmp);
				munlock_vma_pages_all(tmp);
			}
			tmp = tmp->vm_next;
		}
	}

	detach_vmas_to_be_unmapped(mm, vma, prev, end);
	unmap_region(mm, vma, prev, start, end);

	remove_vma_list(mm, vma);

	return 0;
}

EXPORT_SYMBOL(do_munmap);

SYSCALL_DEFINE2(munmap, unsigned long, addr, size_t, len)
{
	int ret;
	struct mm_struct *mm = current->mm;

	profile_munmap(addr);

	down_write(&mm->mmap_sem);
	ret = do_munmap(mm, addr, len);
	up_write(&mm->mmap_sem);
	return ret;
}

static inline void verify_mm_writelocked(struct mm_struct *mm)
{
#ifdef CONFIG_DEBUG_VM
	if (unlikely(down_read_trylock(&mm->mmap_sem))) {
		WARN_ON(1);
		up_read(&mm->mmap_sem);
	}
#endif
}

unsigned long do_brk(unsigned long addr, unsigned long len)
{
	struct mm_struct * mm = current->mm;
	struct vm_area_struct * vma, * prev;
	unsigned long flags;
	struct rb_node ** rb_link, * rb_parent;
	pgoff_t pgoff = addr >> PAGE_SHIFT;
	int error;

	len = PAGE_ALIGN(len);
	if (!len)
		return addr;

	error = security_file_mmap(NULL, 0, 0, 0, addr, 1);
	if (error)
		return error;

	flags = VM_DATA_DEFAULT_FLAGS | VM_ACCOUNT | mm->def_flags;

	error = get_unmapped_area(NULL, addr, len, 0, MAP_FIXED);
	if (error & ~PAGE_MASK)
		return error;

	if (mm->def_flags & VM_LOCKED) {
		unsigned long locked, lock_limit;
		locked = len >> PAGE_SHIFT;
		locked += mm->locked_vm;
		lock_limit = current->signal->rlim[RLIMIT_MEMLOCK].rlim_cur;
		lock_limit >>= PAGE_SHIFT;
		if (locked > lock_limit && !capable(CAP_IPC_LOCK))
			return -EAGAIN;
	}

	verify_mm_writelocked(mm);

 munmap_back:
	vma = find_vma_prepare(mm, addr, &prev, &rb_link, &rb_parent);
	if (vma && vma->vm_start < addr + len) {
		if (do_munmap(mm, addr, len))
			return -ENOMEM;
		goto munmap_back;
	}

	if (!may_expand_vm(mm, len >> PAGE_SHIFT))
		return -ENOMEM;

	if (mm->map_count > sysctl_max_map_count)
		return -ENOMEM;

	if (security_vm_enough_memory(len >> PAGE_SHIFT))
		return -ENOMEM;

	vma = vma_merge(mm, prev, addr, addr + len, flags,
					NULL, NULL, pgoff, NULL);
	if (vma)
		goto out;

	vma = kmem_cache_zalloc(vm_area_cachep, GFP_KERNEL);
	if (!vma) {
		vm_unacct_memory(len >> PAGE_SHIFT);
		return -ENOMEM;
	}

	vma->vm_mm = mm;
	vma->vm_start = addr;
	vma->vm_end = addr + len;
	vma->vm_pgoff = pgoff;
	vma->vm_flags = flags;
	vma->vm_page_prot = vm_get_page_prot(flags);
	vma_link(mm, vma, prev, rb_link, rb_parent);
out:
	mm->total_vm += len >> PAGE_SHIFT;
	if (flags & VM_LOCKED) {
		if (!mlock_vma_pages_range(vma, addr, addr + len))
			mm->locked_vm += (len >> PAGE_SHIFT);
	}
	return addr;
}

EXPORT_SYMBOL(do_brk);

void exit_mmap(struct mm_struct *mm)
{
	struct mmu_gather *tlb;
	struct vm_area_struct *vma;
	unsigned long nr_accounted = 0;
	unsigned long end;

	mmu_notifier_release(mm);

	if (mm->locked_vm) {
		vma = mm->mmap;
		while (vma) {
			if (vma->vm_flags & VM_LOCKED)
				munlock_vma_pages_all(vma);
			vma = vma->vm_next;
		}
	}

	arch_exit_mmap(mm);

	vma = mm->mmap;
	if (!vma)	 
		return;

	lru_add_drain();
	flush_cache_mm(mm);
	tlb = tlb_gather_mmu(mm, 1);
	 
	end = unmap_vmas(&tlb, vma, 0, -1, &nr_accounted, NULL);
	vm_unacct_memory(nr_accounted);

	free_pgtables(tlb, vma, FIRST_USER_ADDRESS, 0);
	tlb_finish_mmu(tlb, 0, end);

	while (vma)
		vma = remove_vma(vma);

	BUG_ON(mm->nr_ptes > (FIRST_USER_ADDRESS+PMD_SIZE-1)>>PMD_SHIFT);
}

int insert_vm_struct(struct mm_struct * mm, struct vm_area_struct * vma)
{
	struct vm_area_struct * __vma, * prev;
	struct rb_node ** rb_link, * rb_parent;

	if (!vma->vm_file) {
		BUG_ON(vma->anon_vma);
		vma->vm_pgoff = vma->vm_start >> PAGE_SHIFT;
	}
	__vma = find_vma_prepare(mm,vma->vm_start,&prev,&rb_link,&rb_parent);
	if (__vma && __vma->vm_start < vma->vm_end)
		return -ENOMEM;
	if ((vma->vm_flags & VM_ACCOUNT) &&
	     security_vm_enough_memory_mm(mm, vma_pages(vma)))
		return -ENOMEM;
	vma_link(mm, vma, prev, rb_link, rb_parent);
	return 0;
}

struct vm_area_struct *copy_vma(struct vm_area_struct **vmap,
	unsigned long addr, unsigned long len, pgoff_t pgoff)
{
	struct vm_area_struct *vma = *vmap;
	unsigned long vma_start = vma->vm_start;
	struct mm_struct *mm = vma->vm_mm;
	struct vm_area_struct *new_vma, *prev;
	struct rb_node **rb_link, *rb_parent;
	struct mempolicy *pol;

	if (!vma->vm_file && !vma->anon_vma)
		pgoff = addr >> PAGE_SHIFT;

	find_vma_prepare(mm, addr, &prev, &rb_link, &rb_parent);
	new_vma = vma_merge(mm, prev, addr, addr + len, vma->vm_flags,
			vma->anon_vma, vma->vm_file, pgoff, vma_policy(vma));
	if (new_vma) {
		 
		if (vma_start >= new_vma->vm_start &&
		    vma_start < new_vma->vm_end)
			*vmap = new_vma;
	} else {
		new_vma = kmem_cache_alloc(vm_area_cachep, GFP_KERNEL);
		if (new_vma) {
			*new_vma = *vma;
			pol = mpol_dup(vma_policy(vma));
			if (IS_ERR(pol)) {
				kmem_cache_free(vm_area_cachep, new_vma);
				return NULL;
			}
			vma_set_policy(new_vma, pol);
			new_vma->vm_start = addr;
			new_vma->vm_end = addr + len;
			new_vma->vm_pgoff = pgoff;
			if (new_vma->vm_file) {
				get_file(new_vma->vm_file);
				if (vma->vm_flags & VM_EXECUTABLE)
					added_exe_file_vma(mm);
			}
			if (new_vma->vm_ops && new_vma->vm_ops->open)
				new_vma->vm_ops->open(new_vma);
			vma_link(mm, new_vma, prev, rb_link, rb_parent);
		}
	}
	return new_vma;
}

int may_expand_vm(struct mm_struct *mm, unsigned long npages)
{
	unsigned long cur = mm->total_vm;	 
	unsigned long lim;

	lim = current->signal->rlim[RLIMIT_AS].rlim_cur >> PAGE_SHIFT;

	if (cur + npages > lim)
		return 0;
	return 1;
}

static int special_mapping_fault(struct vm_area_struct *vma,
				struct vm_fault *vmf)
{
	pgoff_t pgoff;
	struct page **pages;

	pgoff = vmf->pgoff - vma->vm_pgoff;

	for (pages = vma->vm_private_data; pgoff && *pages; ++pages)
		pgoff--;

	if (*pages) {
		struct page *page = *pages;
		get_page(page);
		vmf->page = page;
		return 0;
	}

	return VM_FAULT_SIGBUS;
}

static void special_mapping_close(struct vm_area_struct *vma)
{
}

static const struct vm_operations_struct special_mapping_vmops = {
	.close = special_mapping_close,
	.fault = special_mapping_fault,
};

int install_special_mapping(struct mm_struct *mm,
			    unsigned long addr, unsigned long len,
			    unsigned long vm_flags, struct page **pages)
{
	struct vm_area_struct *vma;

	vma = kmem_cache_zalloc(vm_area_cachep, GFP_KERNEL);
	if (unlikely(vma == NULL))
		return -ENOMEM;

	vma->vm_mm = mm;
	vma->vm_start = addr;
	vma->vm_end = addr + len;

	vma->vm_flags = vm_flags | mm->def_flags | VM_DONTEXPAND;
	vma->vm_page_prot = vm_get_page_prot(vma->vm_flags);

	vma->vm_ops = &special_mapping_vmops;
	vma->vm_private_data = pages;

	if (unlikely(insert_vm_struct(mm, vma))) {
		kmem_cache_free(vm_area_cachep, vma);
		return -ENOMEM;
	}

	mm->total_vm += len >> PAGE_SHIFT;

	perf_event_mmap(vma);

	return 0;
}

static DEFINE_MUTEX(mm_all_locks_mutex);

static void vm_lock_anon_vma(struct mm_struct *mm, struct anon_vma *anon_vma)
{
	if (!test_bit(0, (unsigned long *) &anon_vma->head.next)) {
		 
		spin_lock_nest_lock(&anon_vma->lock, &mm->mmap_sem);
		 
		if (__test_and_set_bit(0, (unsigned long *)
				       &anon_vma->head.next))
			BUG();
	}
}

static void vm_lock_mapping(struct mm_struct *mm, struct address_space *mapping)
{
	if (!test_bit(AS_MM_ALL_LOCKS, &mapping->flags)) {
		 
		if (test_and_set_bit(AS_MM_ALL_LOCKS, &mapping->flags))
			BUG();
		spin_lock_nest_lock(&mapping->i_mmap_lock, &mm->mmap_sem);
	}
}

int mm_take_all_locks(struct mm_struct *mm)
{
	struct vm_area_struct *vma;
	int ret = -EINTR;

	BUG_ON(down_read_trylock(&mm->mmap_sem));

	mutex_lock(&mm_all_locks_mutex);

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (signal_pending(current))
			goto out_unlock;
		if (vma->vm_file && vma->vm_file->f_mapping)
			vm_lock_mapping(mm, vma->vm_file->f_mapping);
	}

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (signal_pending(current))
			goto out_unlock;
		if (vma->anon_vma)
			vm_lock_anon_vma(mm, vma->anon_vma);
	}

	ret = 0;

out_unlock:
	if (ret)
		mm_drop_all_locks(mm);

	return ret;
}

static void vm_unlock_anon_vma(struct anon_vma *anon_vma)
{
	if (test_bit(0, (unsigned long *) &anon_vma->head.next)) {
		 
		if (!__test_and_clear_bit(0, (unsigned long *)
					  &anon_vma->head.next))
			BUG();
		spin_unlock(&anon_vma->lock);
	}
}

static void vm_unlock_mapping(struct address_space *mapping)
{
	if (test_bit(AS_MM_ALL_LOCKS, &mapping->flags)) {
		 
		spin_unlock(&mapping->i_mmap_lock);
		if (!test_and_clear_bit(AS_MM_ALL_LOCKS,
					&mapping->flags))
			BUG();
	}
}

void mm_drop_all_locks(struct mm_struct *mm)
{
	struct vm_area_struct *vma;

	BUG_ON(down_read_trylock(&mm->mmap_sem));
	BUG_ON(!mutex_is_locked(&mm_all_locks_mutex));

	for (vma = mm->mmap; vma; vma = vma->vm_next) {
		if (vma->anon_vma)
			vm_unlock_anon_vma(vma->anon_vma);
		if (vma->vm_file && vma->vm_file->f_mapping)
			vm_unlock_mapping(vma->vm_file->f_mapping);
	}

	mutex_unlock(&mm_all_locks_mutex);
}

void __init mmap_init(void)
{
	int ret;

	ret = percpu_counter_init(&vm_committed_as, 0);
	VM_BUG_ON(ret);
}

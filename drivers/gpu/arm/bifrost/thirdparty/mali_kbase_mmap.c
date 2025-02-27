/*
 *
 * (C) COPYRIGHT ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 *//*
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained
 * from Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include "linux/mman.h"
#include "../mali_kbase.h"

/* mali_kbase_mmap.c
 *
 * This file contains Linux specific implementation of
 * kbase_get_unmapped_area() interface.
 */


/**
 * align_and_check() - Align the specified pointer to the provided alignment and
 *                     check that it is still in range.
 * @gap_end:        Highest possible start address for allocation (end of gap in
 *                  address space)
 * @gap_start:      Start address of current memory area / gap in address space
 * @info:           vm_unmapped_area_info structure passed to caller, containing
 *                  alignment, length and limits for the allocation
 * @is_shader_code: True if the allocation is for shader code (which has
 *                  additional alignment requirements)
 *
 * Return: true if gap_end is now aligned correctly and is still in range,
 *         false otherwise
 */
static bool align_and_check(unsigned long *gap_end, unsigned long gap_start,
		struct vm_unmapped_area_info *info, bool is_shader_code)
{
	/* Compute highest gap address at the desired alignment */
	(*gap_end) -= info->length;
	(*gap_end) -= (*gap_end - info->align_offset) & info->align_mask;

	if (is_shader_code) {
		/* Check for 4GB boundary */
		if (0 == (*gap_end & BASE_MEM_MASK_4GB))
			(*gap_end) -= (info->align_offset ? info->align_offset :
					info->length);
		if (0 == ((*gap_end + info->length) & BASE_MEM_MASK_4GB))
			(*gap_end) -= (info->align_offset ? info->align_offset :
					info->length);

		if (!(*gap_end & BASE_MEM_MASK_4GB) || !((*gap_end +
				info->length) & BASE_MEM_MASK_4GB))
			return false;
	}


	if ((*gap_end < info->low_limit) || (*gap_end < gap_start))
		return false;


	return true;
}

/**
 * kbase_unmapped_area_topdown() - allocates new areas top-down from
 *                                 below the stack limit.
 * @info:              Information about the memory area to allocate.
 * @is_shader_code:    Boolean which denotes whether the allocated area is
 *                      intended for the use by shader core in which case a
 *                      special alignment requirements apply.
 *
 * The unmapped_area_topdown() function in the Linux kernel is not exported
 * using EXPORT_SYMBOL_GPL macro. To allow us to call this function from a
 * module and also make use of the fact that some of the requirements for
 * the unmapped area are known in advance, we implemented an extended version
 * of this function and prefixed it with 'kbase_'.
 *
 * The difference in the call parameter list comes from the fact that
 * kbase_unmapped_area_topdown() is called with additional parameter which
 * is provided to denote whether the allocation is for a shader core memory
 * or not. This is significant since the executable shader core memory has
 * additional alignment requirements.
 *
 * The modification of the original Linux function lies in how the computation
 * of the highest gap address at the desired alignment is performed once the
 * gap with desirable properties is found. For this purpose a special function
 * is introduced (@ref align_and_check()) which beside computing the gap end
 * at the desired alignment also performs additional alignment check for the
 * case when the memory is executable shader core memory. For such case, it is
 * ensured that the gap does not end on a 4GB boundary.
 *
 * Return: address of the found gap end (high limit) if area is found;
 *         -ENOMEM if search is unsuccessful
*/

static unsigned long kbase_unmapped_area_topdown(struct vm_unmapped_area_info
		*info, bool is_shader_code)
{
#if (KERNEL_VERSION(6, 1, 0) > LINUX_VERSION_CODE)
	struct mm_struct *mm = current->mm;
	struct vm_area_struct *vma;
	unsigned long length, low_limit, high_limit, gap_start, gap_end;

	/* Adjust search length to account for worst case alignment overhead */
	length = info->length + info->align_mask;
	if (length < info->length)
		return -ENOMEM;

	/*
	 * Adjust search limits by the desired length.
	 * See implementation comment at top of unmapped_area().
	 */
	gap_end = info->high_limit;
	if (gap_end < length)
		return -ENOMEM;
	high_limit = gap_end - length;

	if (info->low_limit > high_limit)
		return -ENOMEM;
	low_limit = info->low_limit + length;

	/* Check highest gap, which does not precede any rbtree node */
	gap_start = mm->highest_vm_end;
	if (gap_start <= high_limit) {
		if (align_and_check(&gap_end, gap_start, info, is_shader_code))
			return gap_end;
	}

	/* Check if rbtree root looks promising */
	if (RB_EMPTY_ROOT(&mm->mm_rb))
		return -ENOMEM;
	vma = rb_entry(mm->mm_rb.rb_node, struct vm_area_struct, vm_rb);
	if (vma->rb_subtree_gap < length)
		return -ENOMEM;

	while (true) {
		/* Visit right subtree if it looks promising */
		gap_start = vma->vm_prev ? vma->vm_prev->vm_end : 0;
		if (gap_start <= high_limit && vma->vm_rb.rb_right) {
			struct vm_area_struct *right =
				rb_entry(vma->vm_rb.rb_right,
					 struct vm_area_struct, vm_rb);
			if (right->rb_subtree_gap >= length) {
				vma = right;
				continue;
			}
		}

check_current:
		/* Check if current node has a suitable gap */
		gap_end = vma->vm_start;
		if (gap_end < low_limit)
			return -ENOMEM;
		if (gap_start <= high_limit && gap_end - gap_start >= length) {
			/* We found a suitable gap. Clip it with the original
			 * high_limit. */
			if (gap_end > info->high_limit)
				gap_end = info->high_limit;

			if (align_and_check(&gap_end, gap_start, info,
					is_shader_code))
				return gap_end;
		}

		/* Visit left subtree if it looks promising */
		if (vma->vm_rb.rb_left) {
			struct vm_area_struct *left =
				rb_entry(vma->vm_rb.rb_left,
					 struct vm_area_struct, vm_rb);
			if (left->rb_subtree_gap >= length) {
				vma = left;
				continue;
			}
		}

		/* Go back up the rbtree to find next candidate node */
		while (true) {
			struct rb_node *prev = &vma->vm_rb;

			if (!rb_parent(prev))
				return -ENOMEM;
			vma = rb_entry(rb_parent(prev),
				       struct vm_area_struct, vm_rb);
			if (prev == vma->vm_rb.rb_right) {
				gap_start = vma->vm_prev ?
					vma->vm_prev->vm_end : 0;
				goto check_current;
			}
		}
	}
#else
	unsigned long length, high_limit, gap_start, gap_end;

	MA_STATE(mas, &current->mm->mm_mt, 0, 0);
	/* Adjust search length to account for worst case alignment overhead */
	length = info->length + info->align_mask;
	if (length < info->length)
		return -ENOMEM;

	/*
	 * Adjust search limits by the desired length.
	 * See implementation comment at top of unmapped_area().
	 */
	gap_end = info->high_limit;
	if (gap_end < length)
		return -ENOMEM;
	high_limit = gap_end - length;

	if (info->low_limit > high_limit)
		return -ENOMEM;

	while (true) {
		if (mas_empty_area_rev(&mas, info->low_limit, info->high_limit - 1, length))
			return -ENOMEM;
		gap_end = mas.last + 1;
		gap_start = mas.min;

		if (align_and_check(&gap_end, gap_start, info, is_shader_code))
			return gap_end;
	}
#endif
	return -ENOMEM;
}


/* This function is based on Linux kernel's arch_get_unmapped_area, but
 * simplified slightly. Modifications come from the fact that some values
 * about the memory area are known in advance.
 */
unsigned long kbase_get_unmapped_area(struct file *filp,
		const unsigned long addr, const unsigned long len,
		const unsigned long pgoff, const unsigned long flags)
{
	struct kbase_context *kctx = filp->private_data;
	struct mm_struct *mm = current->mm;
	struct vm_unmapped_area_info info;
	unsigned long align_offset = 0;
	unsigned long align_mask = 0;
	unsigned long high_limit = mm->mmap_base;
	unsigned long low_limit = PAGE_SIZE;
	int cpu_va_bits = BITS_PER_LONG;
	int gpu_pc_bits =
	      kctx->kbdev->gpu_props.props.core_props.log2_program_counter_size;
	bool is_shader_code = false;
	unsigned long ret;

	/* err on fixed address */
	if ((flags & MAP_FIXED) || addr)
		return -EINVAL;

#ifdef CONFIG_64BIT
	/* too big? */
	if (len > TASK_SIZE - SZ_2M)
		return -ENOMEM;

	if (!kbase_ctx_flag(kctx, KCTX_COMPAT)) {

		high_limit = min_t(unsigned long, mm->mmap_base,
				(kctx->same_va_end << PAGE_SHIFT));

		/* If there's enough (> 33 bits) of GPU VA space, align
		 * to 2MB boundaries.
		 */
		if (kctx->kbdev->gpu_props.mmu.va_bits > 33) {
			if (len >= SZ_2M) {
				align_offset = SZ_2M;
				align_mask = SZ_2M - 1;
			}
		}

		low_limit = SZ_2M;
	} else {
		cpu_va_bits = 32;
	}
#endif /* CONFIG_64BIT */
	if ((PFN_DOWN(BASE_MEM_COOKIE_BASE) <= pgoff) &&
		(PFN_DOWN(BASE_MEM_FIRST_FREE_ADDRESS) > pgoff)) {
			int cookie = pgoff - PFN_DOWN(BASE_MEM_COOKIE_BASE);
			struct kbase_va_region *reg =
					kctx->pending_regions[cookie];

			if (!reg)
				return -EINVAL;

			if (!(reg->flags & KBASE_REG_GPU_NX)) {
				if (cpu_va_bits > gpu_pc_bits) {
					align_offset = 1ULL << gpu_pc_bits;
					align_mask = align_offset - 1;
					is_shader_code = true;
				}
			} else if (reg->flags & KBASE_REG_TILER_ALIGN_TOP) {
				unsigned long extent_bytes =
				     (unsigned long)(reg->extent << PAGE_SHIFT);
				/* kbase_check_alloc_sizes() already satisfies
				 * these checks, but they're here to avoid
				 * maintenance hazards due to the assumptions
				 * involved */
				WARN_ON(reg->extent > (ULONG_MAX >> PAGE_SHIFT));
				WARN_ON(reg->initial_commit > (ULONG_MAX >> PAGE_SHIFT));
				WARN_ON(!is_power_of_2(extent_bytes));
				align_mask = extent_bytes - 1;
				align_offset =
				      extent_bytes - (reg->initial_commit << PAGE_SHIFT);
			}
#ifndef CONFIG_64BIT
	} else {
		return current->mm->get_unmapped_area(filp, addr, len, pgoff,
						      flags);
#endif
	}

	info.flags = 0;
	info.length = len;
	info.low_limit = low_limit;
	info.high_limit = high_limit;
	info.align_offset = align_offset;
	info.align_mask = align_mask;

	ret = kbase_unmapped_area_topdown(&info, is_shader_code);

	if (IS_ERR_VALUE(ret) && high_limit == mm->mmap_base &&
			high_limit < (kctx->same_va_end << PAGE_SHIFT)) {
		/* Retry above mmap_base */
		info.low_limit = mm->mmap_base;
		info.high_limit = min_t(u64, TASK_SIZE,
					(kctx->same_va_end << PAGE_SHIFT));

		ret = kbase_unmapped_area_topdown(&info, is_shader_code);
	}

	return ret;
}

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * lib/prio_tree.c - priority search tree
 *
 * Copyright (C) 2004, Rajesh Venkatasubramanian <vrajesh@umich.edu>
 *
 * This file is released under the GPL v2.
 *
 * Based on the radix priority search tree proposed by Edward M. McCreight
 * SIAM Journal of Computing, vol. 14, no.2, pages 257-276, May 1985
 *
 * 02Feb2004	Initial version
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/prio_tree.h>

#ifdef CONFIG_SYNO_ALPINE
#ifdef CONFIG_LFS_ON_32CPU
#define PRIO_TREE_0   0ULL
#define PRIO_TREE_1   1ULL
#define PRIO_TREE_BITS_PER_KEY                64
#else
#define PRIO_TREE_0   0UL
#define PRIO_TREE_1   1UL
#define PRIO_TREE_BITS_PER_KEY                BITS_PER_LONG
#endif
#endif
/*
 * A clever mix of heap and radix trees forms a radix priority search tree (PST)
 * which is useful for storing intervals, e.g, we can consider a vma as a closed
 * interval of file pages [offset_begin, offset_end], and store all vmas that
 * map a file in a PST. Then, using the PST, we can answer a stabbing query,
 * i.e., selecting a set of stored intervals (vmas) that overlap with (map) a
 * given input interval X (a set of consecutive file pages), in "O(log n + m)"
 * time where 'log n' is the height of the PST, and 'm' is the number of stored
 * intervals (vmas) that overlap (map) with the input interval X (the set of
 * consecutive file pages).
 *
 * In our implementation, we store closed intervals of the form [radix_index,
 * heap_index]. We assume that always radix_index <= heap_index. McCreight's PST
 * is designed for storing intervals with unique radix indices, i.e., each
 * interval have different radix_index. However, this limitation can be easily
 * overcome by using the size, i.e., heap_index - radix_index, as part of the
 * index, so we index the tree using [(radix_index,size), heap_index].
 *
 * When the above-mentioned indexing scheme is used, theoretically, in a 32 bit
 * machine, the maximum height of a PST can be 64. We can use a balanced version
 * of the priority search tree to optimize the tree height, but the balanced
 * tree proposed by McCreight is too complex and memory-hungry for our purpose.
 */

/*
 * The following macros are used for implementing prio_tree for i_mmap
 */

#define RADIX_INDEX(vma)  ((vma)->vm_pgoff)
#define VMA_SIZE(vma)	  (((vma)->vm_end - (vma)->vm_start) >> PAGE_SHIFT)
/* avoid overflow */
#define HEAP_INDEX(vma)	  ((vma)->vm_pgoff + (VMA_SIZE(vma) - 1))

#ifdef CONFIG_SYNO_ALPINE
static void get_index(const struct prio_tree_root *root,
    const struct prio_tree_node *node,
    prio_tree_t *radix, prio_tree_t *heap)
#else
static void get_index(const struct prio_tree_root *root,
    const struct prio_tree_node *node,
    unsigned long *radix, unsigned long *heap)
#endif
{
	if (root->raw) {
		struct vm_area_struct *vma = prio_tree_entry(
		    node, struct vm_area_struct, shared.prio_tree_node);

		*radix = RADIX_INDEX(vma);
		*heap = HEAP_INDEX(vma);
	}
	else {
		*radix = node->start;
		*heap = node->last;
	}
}

#ifdef CONFIG_SYNO_ALPINE
static prio_tree_t index_bits_to_maxindex[PRIO_TREE_BITS_PER_KEY];
#else
static unsigned long index_bits_to_maxindex[BITS_PER_LONG];
#endif

void __init prio_tree_init(void)
{
	unsigned int i;

#ifdef CONFIG_SYNO_ALPINE
	for (i = 0; i < ARRAY_SIZE(index_bits_to_maxindex) - 1; i++)
		index_bits_to_maxindex[i] = (PRIO_TREE_1 << (i + 1)) - 1;
	index_bits_to_maxindex[ARRAY_SIZE(index_bits_to_maxindex) - 1] = ~PRIO_TREE_0;
#else
	for (i = 0; i < ARRAY_SIZE(index_bits_to_maxindex) - 1; i++)
		index_bits_to_maxindex[i] = (1UL << (i + 1)) - 1;
	index_bits_to_maxindex[ARRAY_SIZE(index_bits_to_maxindex) - 1] = ~0UL;
#endif
}

/*
 * Maximum heap_index that can be stored in a PST with index_bits bits
 */
#ifdef CONFIG_SYNO_ALPINE
static inline prio_tree_t prio_tree_maxindex(unsigned int bits)
#else
static inline unsigned long prio_tree_maxindex(unsigned int bits)
#endif
{
	return index_bits_to_maxindex[bits - 1];
}

/*
 * Extend a priority search tree so that it can store a node with heap_index
 * max_heap_index. In the worst case, this algorithm takes O((log n)^2).
 * However, this function is used rarely and the common case performance is
 * not bad.
 */
#ifdef CONFIG_SYNO_ALPINE
static struct prio_tree_node *prio_tree_expand(struct prio_tree_root *root,
		struct prio_tree_node *node, prio_tree_t max_heap_index)
#else
static struct prio_tree_node *prio_tree_expand(struct prio_tree_root *root,
		struct prio_tree_node *node, unsigned long max_heap_index)
#endif
{
	struct prio_tree_node *first = NULL, *prev, *last = NULL;

	if (max_heap_index > prio_tree_maxindex(root->index_bits))
		root->index_bits++;

	while (max_heap_index > prio_tree_maxindex(root->index_bits)) {
		root->index_bits++;

		if (prio_tree_empty(root))
			continue;

		if (first == NULL) {
			first = root->prio_tree_node;
			prio_tree_remove(root, root->prio_tree_node);
			INIT_PRIO_TREE_NODE(first);
			last = first;
		} else {
			prev = last;
			last = root->prio_tree_node;
			prio_tree_remove(root, root->prio_tree_node);
			INIT_PRIO_TREE_NODE(last);
			prev->left = last;
			last->parent = prev;
		}
	}

	INIT_PRIO_TREE_NODE(node);

	if (first) {
		node->left = first;
		first->parent = node;
	} else
		last = node;

	if (!prio_tree_empty(root)) {
		last->left = root->prio_tree_node;
		last->left->parent = last;
	}

	root->prio_tree_node = node;
	return node;
}

/*
 * Replace a prio_tree_node with a new node and return the old node
 */
struct prio_tree_node *prio_tree_replace(struct prio_tree_root *root,
		struct prio_tree_node *old, struct prio_tree_node *node)
{
	INIT_PRIO_TREE_NODE(node);

	if (prio_tree_root(old)) {
		BUG_ON(root->prio_tree_node != old);
		/*
		 * We can reduce root->index_bits here. However, it is complex
		 * and does not help much to improve performance (IMO).
		 */
		node->parent = node;
		root->prio_tree_node = node;
	} else {
		node->parent = old->parent;
		if (old->parent->left == old)
			old->parent->left = node;
		else
			old->parent->right = node;
	}

	if (!prio_tree_left_empty(old)) {
		node->left = old->left;
		old->left->parent = node;
	}

	if (!prio_tree_right_empty(old)) {
		node->right = old->right;
		old->right->parent = node;
	}

	return old;
}

/*
 * Insert a prio_tree_node @node into a radix priority search tree @root. The
 * algorithm typically takes O(log n) time where 'log n' is the number of bits
 * required to represent the maximum heap_index. In the worst case, the algo
 * can take O((log n)^2) - check prio_tree_expand.
 *
 * If a prior node with same radix_index and heap_index is already found in
 * the tree, then returns the address of the prior node. Otherwise, inserts
 * @node into the tree and returns @node.
 */
struct prio_tree_node *prio_tree_insert(struct prio_tree_root *root,
		struct prio_tree_node *node)
{
	struct prio_tree_node *cur, *res = node;
#ifdef CONFIG_SYNO_ALPINE
	prio_tree_t radix_index, heap_index;
	prio_tree_t r_index, h_index, index, mask;
#else
	unsigned long radix_index, heap_index;
	unsigned long r_index, h_index, index, mask;
#endif
	int size_flag = 0;

	get_index(root, node, &radix_index, &heap_index);

	if (prio_tree_empty(root) ||
			heap_index > prio_tree_maxindex(root->index_bits))
		return prio_tree_expand(root, node, heap_index);

	cur = root->prio_tree_node;
#ifdef CONFIG_SYNO_ALPINE
	mask = PRIO_TREE_1 << (root->index_bits - 1);
#else
	mask = 1UL << (root->index_bits - 1);
#endif

	while (mask) {
		get_index(root, cur, &r_index, &h_index);

		if (r_index == radix_index && h_index == heap_index)
			return cur;

                if (h_index < heap_index ||
		    (h_index == heap_index && r_index > radix_index)) {
			struct prio_tree_node *tmp = node;
			node = prio_tree_replace(root, cur, node);
			cur = tmp;
			/* swap indices */
			index = r_index;
			r_index = radix_index;
			radix_index = index;
			index = h_index;
			h_index = heap_index;
			heap_index = index;
		}

		if (size_flag)
			index = heap_index - radix_index;
		else
			index = radix_index;

		if (index & mask) {
			if (prio_tree_right_empty(cur)) {
				INIT_PRIO_TREE_NODE(node);
				cur->right = node;
				node->parent = cur;
				return res;
			} else
				cur = cur->right;
		} else {
			if (prio_tree_left_empty(cur)) {
				INIT_PRIO_TREE_NODE(node);
				cur->left = node;
				node->parent = cur;
				return res;
			} else
				cur = cur->left;
		}

		mask >>= 1;

		if (!mask) {
#ifdef CONFIG_SYNO_ALPINE
			mask = PRIO_TREE_1 << (PRIO_TREE_BITS_PER_KEY - 1);
#else
			mask = 1UL << (BITS_PER_LONG - 1);
#endif
			size_flag = 1;
		}
	}
	/* Should not reach here */
	BUG();
	return NULL;
}

/*
 * Remove a prio_tree_node @node from a radix priority search tree @root. The
 * algorithm takes O(log n) time where 'log n' is the number of bits required
 * to represent the maximum heap_index.
 */
void prio_tree_remove(struct prio_tree_root *root, struct prio_tree_node *node)
{
	struct prio_tree_node *cur;
#ifdef CONFIG_SYNO_ALPINE
	prio_tree_t r_index, h_index_right, h_index_left;
#else
	unsigned long r_index, h_index_right, h_index_left;
#endif

	cur = node;

	while (!prio_tree_left_empty(cur) || !prio_tree_right_empty(cur)) {
		if (!prio_tree_left_empty(cur))
			get_index(root, cur->left, &r_index, &h_index_left);
		else {
			cur = cur->right;
			continue;
		}

		if (!prio_tree_right_empty(cur))
			get_index(root, cur->right, &r_index, &h_index_right);
		else {
			cur = cur->left;
			continue;
		}

		/* both h_index_left and h_index_right cannot be 0 */
		if (h_index_left >= h_index_right)
			cur = cur->left;
		else
			cur = cur->right;
	}

	if (prio_tree_root(cur)) {
		BUG_ON(root->prio_tree_node != cur);
		__INIT_PRIO_TREE_ROOT(root, root->raw);
		return;
	}

	if (cur->parent->right == cur)
		cur->parent->right = cur->parent;
	else
		cur->parent->left = cur->parent;

	while (cur != node)
		cur = prio_tree_replace(root, cur->parent, cur);
}

/*
 * Following functions help to enumerate all prio_tree_nodes in the tree that
 * overlap with the input interval X [radix_index, heap_index]. The enumeration
 * takes O(log n + m) time where 'log n' is the height of the tree (which is
 * proportional to # of bits required to represent the maximum heap_index) and
 * 'm' is the number of prio_tree_nodes that overlap the interval X.
 */

#ifdef CONFIG_SYNO_ALPINE
static struct prio_tree_node *prio_tree_left(struct prio_tree_iter *iter,
		prio_tree_t *r_index, prio_tree_t *h_index)
#else
static struct prio_tree_node *prio_tree_left(struct prio_tree_iter *iter,
		unsigned long *r_index, unsigned long *h_index)
#endif
{
	if (prio_tree_left_empty(iter->cur))
		return NULL;

	get_index(iter->root, iter->cur->left, r_index, h_index);

	if (iter->r_index <= *h_index) {
		iter->cur = iter->cur->left;
		iter->mask >>= 1;
		if (iter->mask) {
			if (iter->size_level)
				iter->size_level++;
		} else {
			if (iter->size_level) {
				BUG_ON(!prio_tree_left_empty(iter->cur));
				BUG_ON(!prio_tree_right_empty(iter->cur));
				iter->size_level++;
#ifdef CONFIG_SYNO_ALPINE
				iter->mask = PRIO_TREE_KEY_MAX_VALUE;
#else
				iter->mask = ULONG_MAX;
#endif
			} else {
				iter->size_level = 1;
#ifdef CONFIG_SYNO_ALPINE
				iter->mask = PRIO_TREE_1 << (PRIO_TREE_BITS_PER_KEY - 1);
#else
				iter->mask = 1UL << (BITS_PER_LONG - 1);
#endif
			}
		}
		return iter->cur;
	}

	return NULL;
}

#ifdef CONFIG_SYNO_ALPINE
static struct prio_tree_node *prio_tree_right(struct prio_tree_iter *iter,
		prio_tree_t *r_index, prio_tree_t *h_index)
#else
static struct prio_tree_node *prio_tree_right(struct prio_tree_iter *iter,
		unsigned long *r_index, unsigned long *h_index)
#endif
{
#ifdef CONFIG_SYNO_ALPINE
	prio_tree_t value;
#else
	unsigned long value;
#endif

	if (prio_tree_right_empty(iter->cur))
		return NULL;

	if (iter->size_level)
		value = iter->value;
	else
		value = iter->value | iter->mask;

	if (iter->h_index < value)
		return NULL;

	get_index(iter->root, iter->cur->right, r_index, h_index);

	if (iter->r_index <= *h_index) {
		iter->cur = iter->cur->right;
		iter->mask >>= 1;
		iter->value = value;
		if (iter->mask) {
			if (iter->size_level)
				iter->size_level++;
		} else {
			if (iter->size_level) {
				BUG_ON(!prio_tree_left_empty(iter->cur));
				BUG_ON(!prio_tree_right_empty(iter->cur));
				iter->size_level++;
#ifdef CONFIG_SYNO_ALPINE
				iter->mask = PRIO_TREE_KEY_MAX_VALUE;
#else
				iter->mask = ULONG_MAX;
#endif
			} else {
				iter->size_level = 1;
#ifdef CONFIG_SYNO_ALPINE
				iter->mask = PRIO_TREE_1 << (PRIO_TREE_BITS_PER_KEY - 1);
#else
				iter->mask = 1UL << (BITS_PER_LONG - 1);
#endif
			}
		}
		return iter->cur;
	}

	return NULL;
}

static struct prio_tree_node *prio_tree_parent(struct prio_tree_iter *iter)
{
	iter->cur = iter->cur->parent;
#ifdef CONFIG_SYNO_ALPINE
	if (iter->mask == PRIO_TREE_KEY_MAX_VALUE)
		iter->mask = PRIO_TREE_1;
#else
	if (iter->mask == ULONG_MAX)
		iter->mask = 1UL;
#endif
	else if (iter->size_level == 1)
#ifdef CONFIG_SYNO_ALPINE
		iter->mask = PRIO_TREE_1;
#else
		iter->mask = 1UL;
#endif
	else
		iter->mask <<= 1;
	if (iter->size_level)
		iter->size_level--;
	if (!iter->size_level && (iter->value & iter->mask))
		iter->value ^= iter->mask;
	return iter->cur;
}

#ifdef CONFIG_SYNO_ALPINE
static inline int overlap(struct prio_tree_iter *iter,
		prio_tree_t r_index, prio_tree_t h_index)
#else
static inline int overlap(struct prio_tree_iter *iter,
		unsigned long r_index, unsigned long h_index)
#endif
{
	return iter->h_index >= r_index && iter->r_index <= h_index;
}

/*
 * prio_tree_first:
 *
 * Get the first prio_tree_node that overlaps with the interval [radix_index,
 * heap_index]. Note that always radix_index <= heap_index. We do a pre-order
 * traversal of the tree.
 */
static struct prio_tree_node *prio_tree_first(struct prio_tree_iter *iter)
{
	struct prio_tree_root *root;
#ifdef CONFIG_SYNO_ALPINE
	prio_tree_t r_index, h_index;
#else
	unsigned long r_index, h_index;
#endif

	INIT_PRIO_TREE_ITER(iter);

	root = iter->root;
	if (prio_tree_empty(root))
		return NULL;

	get_index(root, root->prio_tree_node, &r_index, &h_index);

	if (iter->r_index > h_index)
		return NULL;

#ifdef CONFIG_SYNO_ALPINE
	iter->mask = PRIO_TREE_1 << (root->index_bits - 1);
#else
	iter->mask = 1UL << (root->index_bits - 1);
#endif
	iter->cur = root->prio_tree_node;

	while (1) {
		if (overlap(iter, r_index, h_index))
			return iter->cur;

		if (prio_tree_left(iter, &r_index, &h_index))
			continue;

		if (prio_tree_right(iter, &r_index, &h_index))
			continue;

		break;
	}
	return NULL;
}

/*
 * prio_tree_next:
 *
 * Get the next prio_tree_node that overlaps with the input interval in iter
 */
struct prio_tree_node *prio_tree_next(struct prio_tree_iter *iter)
{
#ifdef CONFIG_SYNO_ALPINE
	prio_tree_t r_index, h_index;
#else
	unsigned long r_index, h_index;
#endif

	if (iter->cur == NULL)
		return prio_tree_first(iter);

repeat:
	while (prio_tree_left(iter, &r_index, &h_index))
		if (overlap(iter, r_index, h_index))
			return iter->cur;

	while (!prio_tree_right(iter, &r_index, &h_index)) {
	    	while (!prio_tree_root(iter->cur) &&
				iter->cur->parent->right == iter->cur)
			prio_tree_parent(iter);

		if (prio_tree_root(iter->cur))
			return NULL;

		prio_tree_parent(iter);
	}

	if (overlap(iter, r_index, h_index))
		return iter->cur;

	goto repeat;
}

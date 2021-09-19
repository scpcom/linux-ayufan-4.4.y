#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/vmalloc.h>
#include "ctree.h"
#include "disk-io.h"
#include "backref.h"
#include "ulist.h"
#include "transaction.h"
#include "delayed-ref.h"
#include "locking.h"

#define BACKREF_FOUND_SHARED 6
#ifdef MY_ABC_HERE
#define BACKREF_NEXT_ITEM 253
#define BACKREF_FOUND_SHARED_ROOT 254
#endif  

#ifdef MY_ABC_HERE
enum btrfs_backref_mode {
	 
	BTRFS_BACKREF_NORMAL,
#ifdef MY_ABC_HERE
	 
	BTRFS_BACKREF_FIND_SHARED_ROOT,
#endif  
};
#endif  

struct extent_inode_elem {
	u64 inum;
	u64 offset;
	struct extent_inode_elem *next;
};

static int check_extent_in_eb(struct btrfs_key *key, struct extent_buffer *eb,
				struct btrfs_file_extent_item *fi,
				u64 extent_item_pos,
				struct extent_inode_elem **eie)
{
	u64 offset = 0;
	struct extent_inode_elem *e;

	if (!btrfs_file_extent_compression(eb, fi) &&
	    !btrfs_file_extent_encryption(eb, fi) &&
	    !btrfs_file_extent_other_encoding(eb, fi)) {
		u64 data_offset;
		u64 data_len;

		data_offset = btrfs_file_extent_offset(eb, fi);
		data_len = btrfs_file_extent_num_bytes(eb, fi);

		if (extent_item_pos < data_offset ||
		    extent_item_pos >= data_offset + data_len)
			return 1;
		offset = extent_item_pos - data_offset;
	}

	e = kmalloc(sizeof(*e), GFP_NOFS);
	if (!e)
		return -ENOMEM;

	e->next = *eie;
	e->inum = key->objectid;
	e->offset = key->offset + offset;
	*eie = e;

	return 0;
}

static void free_inode_elem_list(struct extent_inode_elem *eie)
{
	struct extent_inode_elem *eie_next;

	for (; eie; eie = eie_next) {
		eie_next = eie->next;
		kfree(eie);
	}
}

static int find_extent_in_eb(struct extent_buffer *eb, u64 wanted_disk_byte,
				u64 extent_item_pos,
				struct extent_inode_elem **eie)
{
	u64 disk_byte;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	int slot;
	int nritems;
	int extent_type;
	int ret;

	nritems = btrfs_header_nritems(eb);
	for (slot = 0; slot < nritems; ++slot) {
		btrfs_item_key_to_cpu(eb, &key, slot);
		if (key.type != BTRFS_EXTENT_DATA_KEY)
			continue;
		fi = btrfs_item_ptr(eb, slot, struct btrfs_file_extent_item);
		extent_type = btrfs_file_extent_type(eb, fi);
		if (extent_type == BTRFS_FILE_EXTENT_INLINE)
			continue;
		 
		disk_byte = btrfs_file_extent_disk_bytenr(eb, fi);
		if (disk_byte != wanted_disk_byte)
			continue;

		ret = check_extent_in_eb(&key, eb, fi, extent_item_pos, eie);
		if (ret < 0)
			return ret;
	}

	return 0;
}

struct __prelim_ref {
	struct list_head list;
	u64 root_id;
	struct btrfs_key key_for_search;
	int level;
	int count;
	struct extent_inode_elem *inode_list;
	u64 parent;
	u64 wanted_disk_byte;
};

static struct kmem_cache *btrfs_prelim_ref_cache;

int __init btrfs_prelim_ref_init(void)
{
	btrfs_prelim_ref_cache = kmem_cache_create("btrfs_prelim_ref",
					sizeof(struct __prelim_ref),
					0,
					SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD,
					NULL);
	if (!btrfs_prelim_ref_cache)
		return -ENOMEM;
	return 0;
}

void btrfs_prelim_ref_exit(void)
{
	if (btrfs_prelim_ref_cache)
		kmem_cache_destroy(btrfs_prelim_ref_cache);
}

static int __add_prelim_ref(struct list_head *head, u64 root_id,
			    struct btrfs_key *key, int level,
			    u64 parent, u64 wanted_disk_byte, int count,
#ifdef MY_ABC_HERE
			    enum btrfs_backref_mode mode,
#endif  
			    gfp_t gfp_mask)
{
	struct __prelim_ref *ref;

	if (root_id == BTRFS_DATA_RELOC_TREE_OBJECTID)
		return 0;

	ref = kmem_cache_alloc(btrfs_prelim_ref_cache, gfp_mask);
	if (!ref)
		return -ENOMEM;

	ref->root_id = root_id;
	if (key) {
		ref->key_for_search = *key;
		 
#ifdef MY_ABC_HERE
		 
		if (mode == BTRFS_BACKREF_NORMAL)
#endif  
		if (ref->key_for_search.type == BTRFS_EXTENT_DATA_KEY &&
		    ref->key_for_search.offset >= LLONG_MAX)
			ref->key_for_search.offset = 0;
	} else {
		memset(&ref->key_for_search, 0, sizeof(ref->key_for_search));
	}

	ref->inode_list = NULL;
	ref->level = level;
	ref->count = count;
	ref->parent = parent;
	ref->wanted_disk_byte = wanted_disk_byte;
	list_add_tail(&ref->list, head);

	return 0;
}

static int add_all_parents(struct btrfs_root *root, struct btrfs_path *path,
			   struct ulist *parents, struct __prelim_ref *ref,
			   int level, u64 time_seq, const u64 *extent_item_pos,
#ifdef MY_ABC_HERE
			   enum btrfs_backref_mode mode, u64 num_bytes,
#endif  
#ifdef MY_ABC_HERE
			   u64 file_offset, int check_first_ref,
#endif  
			   u64 total_refs)
{
	int ret = 0;
	int slot;
	struct extent_buffer *eb;
	struct btrfs_key key;
	struct btrfs_key *key_for_search = &ref->key_for_search;
	struct btrfs_file_extent_item *fi;
	struct extent_inode_elem *eie = NULL, *old = NULL;
	u64 disk_byte;
	u64 wanted_disk_byte = ref->wanted_disk_byte;
	u64 count = 0;
#ifdef MY_ABC_HERE
	u64 total_count;
	u64 datao;

	if (mode == BTRFS_BACKREF_NORMAL || key_for_search->type != BTRFS_EXTENT_DATA_KEY)
		total_count = total_refs;
	else
		total_count = ref->count + total_refs;
#endif  

	if (level != 0) {
		eb = path->nodes[level];
		ret = ulist_add(parents, eb->start, 0, GFP_NOFS);
		if (ret < 0)
			return ret;
		return 0;
	}

	if (path->slots[0] >= btrfs_header_nritems(path->nodes[0]))
		ret = btrfs_next_old_leaf(root, path, time_seq);

#ifdef MY_ABC_HERE
	while (!ret && count < total_count) {
#else
	while (!ret && count < total_refs) {
#endif  
		eb = path->nodes[0];
		slot = path->slots[0];

		btrfs_item_key_to_cpu(eb, &key, slot);

		if (key.objectid != key_for_search->objectid ||
		    key.type != BTRFS_EXTENT_DATA_KEY)
			break;

		fi = btrfs_item_ptr(eb, slot, struct btrfs_file_extent_item);
		disk_byte = btrfs_file_extent_disk_bytenr(eb, fi);

#ifdef MY_ABC_HERE
		if (mode != BTRFS_BACKREF_NORMAL &&
		    key_for_search->type == BTRFS_EXTENT_DATA_KEY &&
		    key.offset >= key_for_search->offset + num_bytes)
			break;
#endif  
		if (disk_byte == wanted_disk_byte) {
			eie = NULL;
			old = NULL;
#ifdef MY_ABC_HERE
			if (mode != BTRFS_BACKREF_NORMAL) {
				datao = key.offset - btrfs_file_extent_offset(eb, fi);
				if (datao != key_for_search->offset)
					goto next;
#ifdef MY_ABC_HERE
				if (mode == BTRFS_BACKREF_FIND_SHARED_ROOT && check_first_ref &&
					key.offset < file_offset) {
					 
					return BACKREF_NEXT_ITEM;
				}
#endif  
			}
#endif  
			count++;
			if (extent_item_pos) {
				ret = check_extent_in_eb(&key, eb, fi,
						*extent_item_pos,
						&eie);
				if (ret < 0)
					break;
			}
			if (ret > 0)
				goto next;
			ret = ulist_add_merge(parents, eb->start,
					      (uintptr_t)eie,
					      (u64 *)&old, GFP_NOFS);
			if (ret < 0)
				break;
			if (!ret && extent_item_pos) {
				while (old->next)
					old = old->next;
				old->next = eie;
			}
			eie = NULL;
		}
next:
		ret = btrfs_next_old_item(root, path, time_seq);
	}

	if (ret > 0)
		ret = 0;
	else if (ret < 0)
		free_inode_elem_list(eie);
	return ret;
}

static int __resolve_indirect_ref(struct btrfs_fs_info *fs_info,
				  struct btrfs_path *path, u64 time_seq,
				  struct __prelim_ref *ref,
				  struct ulist *parents,
#ifdef MY_ABC_HERE
				  const u64 *extent_item_pos, enum btrfs_backref_mode mode,
				  u64 num_bytes,
#ifdef MY_ABC_HERE
				  int check_first_ref, u64 file_offset,
#endif  
				  u64 total_refs)
#else
				  const u64 *extent_item_pos, u64 total_refs)
#endif  
{
	struct btrfs_root *root;
	struct btrfs_key root_key;
	struct extent_buffer *eb;
	int ret = 0;
	int root_level;
	int level = ref->level;
	int index;
#ifdef MY_ABC_HERE
	u64 origin_offset = ref->key_for_search.offset;

	if (ref->key_for_search.offset >= LLONG_MAX)
		ref->key_for_search.offset = 0;
#endif  

	root_key.objectid = ref->root_id;
	root_key.type = BTRFS_ROOT_ITEM_KEY;
	root_key.offset = (u64)-1;

	index = srcu_read_lock(&fs_info->subvol_srcu);

	root = btrfs_get_fs_root(fs_info, &root_key, false);
	if (IS_ERR(root)) {
		srcu_read_unlock(&fs_info->subvol_srcu, index);
		ret = PTR_ERR(root);
		goto out;
	}

	if (path->search_commit_root)
		root_level = btrfs_header_level(root->commit_root);
	else
		root_level = btrfs_old_root_level(root, time_seq);

	if (root_level + 1 == level) {
		srcu_read_unlock(&fs_info->subvol_srcu, index);
		goto out;
	}

	path->lowest_level = level;
	ret = btrfs_search_old_slot(root, &ref->key_for_search, path, time_seq);

	srcu_read_unlock(&fs_info->subvol_srcu, index);

	pr_debug("search slot in root %llu (level %d, ref count %d) returned "
		 "%d for key (%llu %u %llu)\n",
		 ref->root_id, level, ref->count, ret,
		 ref->key_for_search.objectid, ref->key_for_search.type,
		 ref->key_for_search.offset);
	if (ret < 0)
		goto out;

	eb = path->nodes[level];
	while (!eb) {
		if (WARN_ON(!level)) {
			ret = 1;
			goto out;
		}
		level--;
		eb = path->nodes[level];
	}

#ifdef MY_ABC_HERE
	 
	ref->key_for_search.offset = origin_offset;
#ifdef MY_ABC_HERE
	ret = add_all_parents(root, path, parents, ref, level, time_seq,
			      extent_item_pos, mode, num_bytes, file_offset,
			      check_first_ref, total_refs);
#else
	ret = add_all_parents(root, path, parents, ref, level, time_seq,
			      extent_item_pos, mode, num_bytes, total_refs);
#endif  
#else
	ret = add_all_parents(root, path, parents, ref, level, time_seq,
			      extent_item_pos, total_refs);
#endif  
out:
	path->lowest_level = 0;
	btrfs_release_path(path);
	return ret;
}

static int __resolve_indirect_refs(struct btrfs_fs_info *fs_info,
				   struct btrfs_path *path, u64 time_seq,
				   struct list_head *head,
				   const u64 *extent_item_pos, u64 total_refs,
#ifdef MY_ABC_HERE
				   u64 root_objectid,
#ifdef MY_ABC_HERE
				   u64 inum, u64 file_offset, u64 datao,
#endif  
				   enum btrfs_backref_mode mode, u64 num_bytes)
#else
				   u64 root_objectid)
#endif  
{
	int err;
	int ret = 0;
	struct __prelim_ref *ref;
	struct __prelim_ref *ref_safe;
	struct __prelim_ref *new_ref;
	struct ulist *parents;
	struct ulist_node *node;
	struct ulist_iterator uiter;

	parents = ulist_alloc(GFP_NOFS);
	if (!parents)
		return -ENOMEM;

	list_for_each_entry_safe(ref, ref_safe, head, list) {
#ifdef MY_ABC_HERE
		int check_first_ref = 0;
#endif  
		if (ref->parent)	 
			continue;
		if (ref->count == 0)
			continue;
#ifdef MY_ABC_HERE
		if (mode == BTRFS_BACKREF_NORMAL)
#endif  
		if (root_objectid && ref->root_id != root_objectid) {
			ret = BACKREF_FOUND_SHARED;
			goto out;
		}
#ifdef MY_ABC_HERE
		if (mode == BTRFS_BACKREF_FIND_SHARED_ROOT) {
			if (ref->level == 0 && ref->root_id == root_objectid &&
				ref->key_for_search.objectid == inum &&
				ref->key_for_search.offset == file_offset - datao) {
				WARN_ON(root_objectid == 0);
				check_first_ref = 1;
			}
		}
#endif  
		err = __resolve_indirect_ref(fs_info, path, time_seq, ref,
					     parents, extent_item_pos,
#ifdef MY_ABC_HERE
					     mode, num_bytes,
#endif  
#ifdef MY_ABC_HERE
					     check_first_ref, file_offset,
#endif  
					     total_refs);
		 
		if (err == -ENOENT) {
			continue;
		} else if (err) {
			ret = err;
			goto out;
		}

		ULIST_ITER_INIT(&uiter);
		node = ulist_next(parents, &uiter);
		ref->parent = node ? node->val : 0;
		ref->inode_list = node ?
			(struct extent_inode_elem *)(uintptr_t)node->aux : NULL;

		while ((node = ulist_next(parents, &uiter))) {
			new_ref = kmem_cache_alloc(btrfs_prelim_ref_cache,
						   GFP_NOFS);
			if (!new_ref) {
				ret = -ENOMEM;
				goto out;
			}
			memcpy(new_ref, ref, sizeof(*ref));
			new_ref->parent = node->val;
			new_ref->inode_list = (struct extent_inode_elem *)
							(uintptr_t)node->aux;
			list_add(&new_ref->list, &ref->list);
		}
		ulist_reinit(parents);
	}
out:
	ulist_free(parents);
	return ret;
}

static inline int ref_for_same_block(struct __prelim_ref *ref1,
				     struct __prelim_ref *ref2)
{
	if (ref1->level != ref2->level)
		return 0;
	if (ref1->root_id != ref2->root_id)
		return 0;
	if (ref1->key_for_search.type != ref2->key_for_search.type)
		return 0;
	if (ref1->key_for_search.objectid != ref2->key_for_search.objectid)
		return 0;
	if (ref1->key_for_search.offset != ref2->key_for_search.offset)
		return 0;
	if (ref1->parent != ref2->parent)
		return 0;

	return 1;
}

static int __add_missing_keys(struct btrfs_fs_info *fs_info,
			      struct list_head *head)
{
	struct list_head *pos;
	struct extent_buffer *eb;

	list_for_each(pos, head) {
		struct __prelim_ref *ref;
		ref = list_entry(pos, struct __prelim_ref, list);

		if (ref->parent)
			continue;
		if (ref->key_for_search.type)
			continue;
		BUG_ON(!ref->wanted_disk_byte);
		eb = read_tree_block(fs_info->tree_root, ref->wanted_disk_byte,
				     fs_info->tree_root->leafsize, 0);
		if (!eb || !extent_buffer_uptodate(eb)) {
			free_extent_buffer(eb);
			return -EIO;
		}
		btrfs_tree_read_lock(eb);
		if (btrfs_header_level(eb) == 0)
			btrfs_item_key_to_cpu(eb, &ref->key_for_search, 0);
		else
			btrfs_node_key_to_cpu(eb, &ref->key_for_search, 0);
		btrfs_tree_read_unlock(eb);
		free_extent_buffer(eb);
	}
	return 0;
}

static void __merge_refs(struct list_head *head, int mode)
{
	struct list_head *pos1;

	list_for_each(pos1, head) {
		struct list_head *n2;
		struct list_head *pos2;
		struct __prelim_ref *ref1;

		ref1 = list_entry(pos1, struct __prelim_ref, list);

		for (pos2 = pos1->next, n2 = pos2->next; pos2 != head;
		     pos2 = n2, n2 = pos2->next) {
			struct __prelim_ref *ref2;
			struct __prelim_ref *xchg;
			struct extent_inode_elem *eie;

			ref2 = list_entry(pos2, struct __prelim_ref, list);

			if (mode == 1) {
				if (!ref_for_same_block(ref1, ref2))
					continue;
				if (!ref1->parent && ref2->parent) {
					xchg = ref1;
					ref1 = ref2;
					ref2 = xchg;
				}
			} else {
				if (ref1->parent != ref2->parent)
					continue;
			}

			eie = ref1->inode_list;
			while (eie && eie->next)
				eie = eie->next;
			if (eie)
				eie->next = ref2->inode_list;
			else
				ref1->inode_list = ref2->inode_list;
			ref1->count += ref2->count;

			list_del(&ref2->list);
			kmem_cache_free(btrfs_prelim_ref_cache, ref2);
		}

	}
}

static int __add_delayed_refs(struct btrfs_delayed_ref_head *head, u64 seq,
			      struct list_head *prefs, u64 *total_refs,
#ifdef MY_ABC_HERE
			      u64 root_objectid, u64 inum, enum btrfs_backref_mode mode)
#else
			      u64 inum)
#endif  
{
	struct btrfs_delayed_extent_op *extent_op = head->extent_op;
	struct rb_node *n = &head->node.rb_node;
	struct btrfs_key key;
	struct btrfs_key op_key = {0};
	int sgn;
	int ret = 0;

	if (extent_op && extent_op->update_key)
		btrfs_disk_key_to_cpu(&op_key, &extent_op->key);

	spin_lock(&head->lock);
	n = rb_first(&head->ref_root);
	while (n) {
		struct btrfs_delayed_ref_node *node;
		node = rb_entry(n, struct btrfs_delayed_ref_node,
				rb_node);
		n = rb_next(n);
		if (node->seq > seq)
			continue;

		switch (node->action) {
		case BTRFS_ADD_DELAYED_EXTENT:
		case BTRFS_UPDATE_DELAYED_HEAD:
			WARN_ON(1);
			continue;
		case BTRFS_ADD_DELAYED_REF:
			sgn = 1;
			break;
		case BTRFS_DROP_DELAYED_REF:
			sgn = -1;
			break;
		default:
			BUG_ON(1);
		}
#ifdef MY_ABC_HERE
		if (mode == BTRFS_BACKREF_NORMAL || node->type != BTRFS_EXTENT_DATA_REF_KEY)
#endif  
		*total_refs += (node->ref_mod * sgn);
		switch (node->type) {
		case BTRFS_TREE_BLOCK_REF_KEY: {
			struct btrfs_delayed_tree_ref *ref;

			ref = btrfs_delayed_node_to_tree_ref(node);
			ret = __add_prelim_ref(prefs, ref->root, &op_key,
					       ref->level + 1, 0, node->bytenr,
#ifdef MY_ABC_HERE
					       node->ref_mod * sgn, 0, GFP_ATOMIC);
#else
					       node->ref_mod * sgn, GFP_ATOMIC);
#endif  
			break;
		}
		case BTRFS_SHARED_BLOCK_REF_KEY: {
			struct btrfs_delayed_tree_ref *ref;

			ref = btrfs_delayed_node_to_tree_ref(node);
			ret = __add_prelim_ref(prefs, 0, NULL,
					       ref->level + 1, ref->parent,
					       node->bytenr,
#ifdef MY_ABC_HERE
					       node->ref_mod * sgn, 0, GFP_ATOMIC);
#else
					       node->ref_mod * sgn, GFP_ATOMIC);
#endif  
			break;
		}
		case BTRFS_EXTENT_DATA_REF_KEY: {
			struct btrfs_delayed_data_ref *ref;
			ref = btrfs_delayed_node_to_data_ref(node);

			key.objectid = ref->objectid;
			key.type = BTRFS_EXTENT_DATA_KEY;
			key.offset = ref->offset;

			if (inum && ref->objectid != inum) {
				ret = BACKREF_FOUND_SHARED;
				break;
			}

			ret = __add_prelim_ref(prefs, ref->root, &key, 0, 0,
					       node->bytenr,
#ifdef MY_ABC_HERE
					       node->ref_mod * sgn, mode, GFP_ATOMIC);
#else
					       node->ref_mod * sgn, GFP_ATOMIC);
#endif  
			break;
		}
		case BTRFS_SHARED_DATA_REF_KEY: {
			struct btrfs_delayed_data_ref *ref;

			ref = btrfs_delayed_node_to_data_ref(node);
#ifdef MY_ABC_HERE
			if (mode != BTRFS_BACKREF_NORMAL)
				*total_refs += (node->ref_mod * sgn);
#endif  
			ret = __add_prelim_ref(prefs, 0, NULL, 0,
					       ref->parent, node->bytenr,
#ifdef MY_ABC_HERE
					       node->ref_mod * sgn, 0, GFP_ATOMIC);
#else
					       node->ref_mod * sgn, GFP_ATOMIC);
#endif  
			break;
		}
		default:
			WARN_ON(1);
		}
		if (ret)
			break;
	}
	spin_unlock(&head->lock);
	return ret;
}

static int __add_inline_refs(struct btrfs_fs_info *fs_info,
			     struct btrfs_path *path, u64 bytenr,
			     int *info_level, struct list_head *prefs,
#ifdef MY_ABC_HERE
			     struct ulist *roots, u64 *lowest_full_backref,
			     u64 *lowest_rootid, u64 *lowest_inum, u64 *lowest_offset,
#endif  
#ifdef MY_ABC_HERE
			     u64 *total_refs, u64 root_objectid, u64 inum,
			     enum btrfs_backref_mode mode)
#else
			     u64 *total_refs, u64 inum)
#endif  
{
	int ret = 0;
	int slot;
	struct extent_buffer *leaf;
	struct btrfs_key key;
	struct btrfs_key found_key;
	unsigned long ptr;
	unsigned long end;
	struct btrfs_extent_item *ei;
	u64 flags;
	u64 item_size;

	leaf = path->nodes[0];
	slot = path->slots[0];

	item_size = btrfs_item_size_nr(leaf, slot);
	BUG_ON(item_size < sizeof(*ei));

	ei = btrfs_item_ptr(leaf, slot, struct btrfs_extent_item);
	flags = btrfs_extent_flags(leaf, ei);
#ifdef MY_ABC_HERE
	if (mode == BTRFS_BACKREF_NORMAL || !(flags & BTRFS_EXTENT_FLAG_DATA))
#endif  
	*total_refs += btrfs_extent_refs(leaf, ei);
	btrfs_item_key_to_cpu(leaf, &found_key, slot);

	ptr = (unsigned long)(ei + 1);
	end = (unsigned long)ei + item_size;

	if (found_key.type == BTRFS_EXTENT_ITEM_KEY &&
	    flags & BTRFS_EXTENT_FLAG_TREE_BLOCK) {
		struct btrfs_tree_block_info *info;

		info = (struct btrfs_tree_block_info *)ptr;
		*info_level = btrfs_tree_block_level(leaf, info);
		ptr += sizeof(struct btrfs_tree_block_info);
		BUG_ON(ptr > end);
	} else if (found_key.type == BTRFS_METADATA_ITEM_KEY) {
		*info_level = found_key.offset;
	} else {
		BUG_ON(!(flags & BTRFS_EXTENT_FLAG_DATA));
	}

	while (ptr < end) {
		struct btrfs_extent_inline_ref *iref;
		u64 offset;
		int type;

		iref = (struct btrfs_extent_inline_ref *)ptr;
		type = btrfs_extent_inline_ref_type(leaf, iref);
		offset = btrfs_extent_inline_ref_offset(leaf, iref);

		switch (type) {
		case BTRFS_SHARED_BLOCK_REF_KEY:
			ret = __add_prelim_ref(prefs, 0, NULL,
						*info_level + 1, offset,
#ifdef MY_ABC_HERE
						bytenr, 1, 0, GFP_NOFS);
#else
						bytenr, 1, GFP_NOFS);
#endif  
			break;
		case BTRFS_SHARED_DATA_REF_KEY: {
			struct btrfs_shared_data_ref *sdref;
			int count;

#ifdef MY_ABC_HERE
			if (mode == BTRFS_BACKREF_FIND_SHARED_ROOT &&
			    *lowest_full_backref > offset)
				*lowest_full_backref = offset;
#endif  
			sdref = (struct btrfs_shared_data_ref *)(iref + 1);
			count = btrfs_shared_data_ref_count(leaf, sdref);
#ifdef MY_ABC_HERE
			if (mode != BTRFS_BACKREF_NORMAL)
				*total_refs += count;
			ret = __add_prelim_ref(prefs, 0, NULL, 0, offset,
					       bytenr, count, 0, GFP_NOFS);
#else
			ret = __add_prelim_ref(prefs, 0, NULL, 0, offset,
					       bytenr, count, GFP_NOFS);
#endif  
			break;
		}
		case BTRFS_TREE_BLOCK_REF_KEY:
#ifdef MY_ABC_HERE
			if (mode == BTRFS_BACKREF_FIND_SHARED_ROOT &&
				!ulist_search(roots, offset)) {
				ret = BACKREF_FOUND_SHARED_ROOT;
				break;
			}
#endif  
			ret = __add_prelim_ref(prefs, offset, NULL,
					       *info_level + 1, 0,
#ifdef MY_ABC_HERE
					       bytenr, 1, 0, GFP_NOFS);
#else
					       bytenr, 1, GFP_NOFS);
#endif  
			break;
		case BTRFS_EXTENT_DATA_REF_KEY: {
			struct btrfs_extent_data_ref *dref;
			int count;
			u64 root;

			dref = (struct btrfs_extent_data_ref *)(&iref->offset);
			count = btrfs_extent_data_ref_count(leaf, dref);
			key.objectid = btrfs_extent_data_ref_objectid(leaf,
								      dref);
			key.type = BTRFS_EXTENT_DATA_KEY;
			key.offset = btrfs_extent_data_ref_offset(leaf, dref);

#ifdef MY_ABC_HERE
			if (mode == BTRFS_BACKREF_NORMAL)
#endif  
			if (inum && key.objectid != inum) {
				ret = BACKREF_FOUND_SHARED;
				break;
			}

			root = btrfs_extent_data_ref_root(leaf, dref);
#ifdef MY_ABC_HERE
			if (mode == BTRFS_BACKREF_FIND_SHARED_ROOT) {
				WARN_ON(!root_objectid || !inum);
				if (!ulist_search(roots, root)) {
					ret = BACKREF_FOUND_SHARED_ROOT;
					break;
				}
				if (*lowest_rootid > root ||
					(*lowest_rootid == root && *lowest_inum > key.objectid) ||
					(*lowest_rootid == root && *lowest_inum == key.objectid &&
					 *lowest_offset > key.offset)) {
					*lowest_rootid = root;
					*lowest_inum = key.objectid;
					*lowest_offset = key.offset;
				}
			}
#endif  
#ifdef MY_ABC_HERE
			ret = __add_prelim_ref(prefs, root, &key, 0, 0,
					       bytenr, count, mode, GFP_NOFS);
#else
			ret = __add_prelim_ref(prefs, root, &key, 0, 0,
					       bytenr, count, GFP_NOFS);
#endif  
			break;
		}
		default:
			WARN_ON(1);
		}
		if (ret)
			return ret;
		ptr += btrfs_extent_inline_ref_size(type);
	}

	return 0;
}

static int __add_keyed_refs(struct btrfs_fs_info *fs_info,
			    struct btrfs_path *path, u64 bytenr,
#ifdef MY_ABC_HERE
			    struct ulist *roots, u64 *lowest_full_backref,
			    u64 *lowest_rootid, u64 *lowest_inum, u64 *lowest_offset,
#endif  
#ifdef MY_ABC_HERE
			    int info_level, struct list_head *prefs,
			    u64 *total_refs, u64 root_objectid,
			    u64 inum, enum btrfs_backref_mode mode)
#else
			    int info_level, struct list_head *prefs, u64 inum)
#endif  
{
	struct btrfs_root *extent_root = fs_info->extent_root;
	int ret;
	int slot;
	struct extent_buffer *leaf;
	struct btrfs_key key;

	while (1) {
		ret = btrfs_next_item(extent_root, path);
		if (ret < 0)
			break;
		if (ret) {
			ret = 0;
			break;
		}

		slot = path->slots[0];
		leaf = path->nodes[0];
		btrfs_item_key_to_cpu(leaf, &key, slot);

		if (key.objectid != bytenr)
			break;
		if (key.type < BTRFS_TREE_BLOCK_REF_KEY)
			continue;
		if (key.type > BTRFS_SHARED_DATA_REF_KEY)
			break;

		switch (key.type) {
		case BTRFS_SHARED_BLOCK_REF_KEY:
			ret = __add_prelim_ref(prefs, 0, NULL,
						info_level + 1, key.offset,
#ifdef MY_ABC_HERE
						bytenr, 1, 0, GFP_NOFS);
#else
						bytenr, 1, GFP_NOFS);
#endif  
			break;
		case BTRFS_SHARED_DATA_REF_KEY: {
			struct btrfs_shared_data_ref *sdref;
			int count;

#ifdef MY_ABC_HERE
			if (mode == BTRFS_BACKREF_FIND_SHARED_ROOT &&
			    *lowest_full_backref > key.offset)
				*lowest_full_backref = key.offset;
#endif  
			sdref = btrfs_item_ptr(leaf, slot,
					      struct btrfs_shared_data_ref);
			count = btrfs_shared_data_ref_count(leaf, sdref);
#ifdef MY_ABC_HERE
			if (mode != BTRFS_BACKREF_NORMAL)
				*total_refs += count;
			ret = __add_prelim_ref(prefs, 0, NULL, 0, key.offset,
						bytenr, count, 0, GFP_NOFS);
#else
			ret = __add_prelim_ref(prefs, 0, NULL, 0, key.offset,
						bytenr, count, GFP_NOFS);
#endif  
			break;
		}
		case BTRFS_TREE_BLOCK_REF_KEY:
#ifdef MY_ABC_HERE
			if (mode == BTRFS_BACKREF_FIND_SHARED_ROOT &&
				!ulist_search(roots, key.offset)) {
				ret = BACKREF_FOUND_SHARED_ROOT;
				break;
			}
#endif  
			ret = __add_prelim_ref(prefs, key.offset, NULL,
					       info_level + 1, 0,
#ifdef MY_ABC_HERE
					       bytenr, 1, 0, GFP_NOFS);
#else
					       bytenr, 1, GFP_NOFS);
#endif  
			break;
		case BTRFS_EXTENT_DATA_REF_KEY: {
			struct btrfs_extent_data_ref *dref;
			int count;
			u64 root;

			dref = btrfs_item_ptr(leaf, slot,
					      struct btrfs_extent_data_ref);
			count = btrfs_extent_data_ref_count(leaf, dref);
			key.objectid = btrfs_extent_data_ref_objectid(leaf,
								      dref);
			key.type = BTRFS_EXTENT_DATA_KEY;
			key.offset = btrfs_extent_data_ref_offset(leaf, dref);

#ifdef MY_ABC_HERE
			if (mode == BTRFS_BACKREF_NORMAL)
#endif  
			if (inum && key.objectid != inum) {
				ret = BACKREF_FOUND_SHARED;
				break;
			}

			root = btrfs_extent_data_ref_root(leaf, dref);
#ifdef MY_ABC_HERE
			if (mode == BTRFS_BACKREF_FIND_SHARED_ROOT) {
				WARN_ON(!root_objectid || !inum);
				if (!ulist_search(roots, root)) {
					ret = BACKREF_FOUND_SHARED_ROOT;
					break;
				}
				if (*lowest_rootid > root ||
					(*lowest_rootid == root && *lowest_inum > key.objectid) ||
					(*lowest_rootid == root && *lowest_inum == key.objectid &&
					 *lowest_offset == key.offset)) {
					*lowest_rootid = root;
					*lowest_inum = key.objectid;
					*lowest_offset = key.offset;
				}
			}
#endif  
#ifdef MY_ABC_HERE
			ret = __add_prelim_ref(prefs, root, &key, 0, 0,
					       bytenr, count, mode, GFP_NOFS);
#else
			ret = __add_prelim_ref(prefs, root, &key, 0, 0,
					       bytenr, count, GFP_NOFS);
#endif  
			break;
		}
		default:
			WARN_ON(1);
		}
		if (ret)
			return ret;

	}

	return ret;
}

#ifdef MY_ABC_HERE
static int check_first_ref(struct extent_buffer *eb, u64 bytenr,
		  u64 inum, u64 file_offset)
{
	u64 disk_byte;
	struct btrfs_key key;
	struct btrfs_file_extent_item *fi;
	int slot;
	int nritems;
	int extent_type;

	nritems = btrfs_header_nritems(eb);
	for (slot = 0; slot < nritems; ++slot) {
		btrfs_item_key_to_cpu(eb, &key, slot);
		if (key.type != BTRFS_EXTENT_DATA_KEY)
			continue;
		fi = btrfs_item_ptr(eb, slot, struct btrfs_file_extent_item);
		extent_type = btrfs_file_extent_type(eb, fi);
		if (extent_type == BTRFS_FILE_EXTENT_INLINE)
			continue;
		 
		disk_byte = btrfs_file_extent_disk_bytenr(eb, fi);
		if (disk_byte != bytenr)
			continue;

		if (key.objectid < inum ||
		    (key.objectid == inum && key.offset < file_offset))
			return 0;
	}

	return 1;
}

static int find_parent_nodes_shared_root(struct btrfs_fs_info *fs_info,
			     u64 bytenr, u64 leaf_bytenr, u64 datao,
			     struct ulist *refs, struct ulist *roots,
			     u64 root_objectid, u64 inum, u64 offset)
{
	struct btrfs_key key;
	struct btrfs_path *path;
	int info_level = 0;
	int ret;
	struct list_head prefs;
	struct __prelim_ref *ref;
	u64 total_refs = 0;
	u64 num_bytes = 0x10000000;  
	u64 lowest_full_backref = (u64)-1;
	u64 lowest_rootid = (u64)-1;
	u64 lowest_inum = (u64)-1;
	u64 lowest_offset = (u64)-1;
	enum btrfs_backref_mode mode = BTRFS_BACKREF_FIND_SHARED_ROOT;

	INIT_LIST_HEAD(&prefs);

	key.objectid = bytenr;
	key.offset = (u64)-1;
	if (btrfs_fs_incompat(fs_info, SKINNY_METADATA))
		key.type = BTRFS_METADATA_ITEM_KEY;
	else
		key.type = BTRFS_EXTENT_ITEM_KEY;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	path->search_commit_root = 1;
	path->skip_locking = 1;

	ret = btrfs_search_slot(NULL, fs_info->extent_root, &key, path, 0, 0);
	if (ret < 0)
		goto out;
	BUG_ON(ret == 0);

	if (path->slots[0]) {
		struct extent_buffer *leaf;
		int slot;

		path->slots[0]--;
		leaf = path->nodes[0];
		slot = path->slots[0];
		btrfs_item_key_to_cpu(leaf, &key, slot);
		if (key.objectid == bytenr &&
		    (key.type == BTRFS_EXTENT_ITEM_KEY ||
		     key.type == BTRFS_METADATA_ITEM_KEY)) {
			num_bytes = key.offset;
			ret = __add_inline_refs(fs_info, path, bytenr,
						&info_level, &prefs,
						roots, &lowest_full_backref,
						&lowest_rootid, &lowest_inum, &lowest_offset,
						&total_refs, root_objectid,
						inum, mode);
			if (ret)
				goto out;
			ret = __add_keyed_refs(fs_info, path, bytenr,
					       roots, &lowest_full_backref,
					       &lowest_rootid, &lowest_inum, &lowest_offset,
					       info_level, &prefs,
					       &total_refs, root_objectid,
					       inum, mode);
			if (ret)
				goto out;
			if (key.type == BTRFS_EXTENT_ITEM_KEY) {
				if (lowest_full_backref != (u64)-1) {
					if (leaf_bytenr != lowest_full_backref) {
						ret = BACKREF_NEXT_ITEM;
						goto out;
					}
				} else if (lowest_rootid != (u64)-1) {
					if (lowest_rootid != root_objectid || lowest_inum != inum ||
						lowest_offset != offset - datao) {
						ret = BACKREF_NEXT_ITEM;
						goto out;
					}
				}
			}
		}
	}
	btrfs_release_path(path);

	ret = __add_missing_keys(fs_info, &prefs);
	if (ret)
		goto out;

	__merge_refs(&prefs, 1);

	WARN_ON(!path->search_commit_root);
	 
	ret = __resolve_indirect_refs(fs_info, path, 0, &prefs,
				      NULL, total_refs,
				      lowest_full_backref == (u64) -1 ? root_objectid : 0,
				      inum, offset, datao, mode, num_bytes);
	if (ret)
		goto out;

	__merge_refs(&prefs, 2);

	while (!list_empty(&prefs)) {
		ref = list_first_entry(&prefs, struct __prelim_ref, list);
		WARN_ON(ref->count < 0);
		if (roots && ref->count && ref->root_id && ref->parent == 0) {
			if (!ulist_search(roots, ref->root_id)) {
				ret = BACKREF_FOUND_SHARED_ROOT;
				goto out;
			}
		}
		if (ref->count && ref->parent) {
			if (ref->level == 0 &&
			    ref->key_for_search.type == 0) {
				struct extent_buffer *eb;
				eb = read_tree_block(fs_info->extent_root,
						    ref->parent, fs_info->extent_root->leafsize, 0);
				if (!eb || !extent_buffer_uptodate(eb)) {
					free_extent_buffer(eb);
					ret = -EIO;
					goto out;
				}
				btrfs_tree_read_lock(eb);
				btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
				ret = check_first_ref(eb, bytenr, inum, offset);
				btrfs_tree_read_unlock_blocking(eb);
				free_extent_buffer(eb);
				if (!ret) {
					ret = BACKREF_NEXT_ITEM;
					goto out;
				}
			}
			ret = ulist_add(refs, ref->parent, 0, GFP_NOFS);
			if (ret < 0)
				goto out;
		}
		list_del(&ref->list);
		kmem_cache_free(btrfs_prelim_ref_cache, ref);
	}

out:
	btrfs_free_path(path);
	while (!list_empty(&prefs)) {
		ref = list_first_entry(&prefs, struct __prelim_ref, list);
		list_del(&ref->list);
		kmem_cache_free(btrfs_prelim_ref_cache, ref);
	}
	return ret;
}
#endif  

static int find_parent_nodes(struct btrfs_trans_handle *trans,
			     struct btrfs_fs_info *fs_info, u64 bytenr,
			     u64 time_seq, struct ulist *refs,
			     struct ulist *roots, const u64 *extent_item_pos,
#ifdef MY_ABC_HERE
			     u64 root_objectid, u64 inum, enum btrfs_backref_mode mode,
			     int in_run_delayed)
#else
			     u64 root_objectid, u64 inum)
#endif  
{
	struct btrfs_key key;
	struct btrfs_path *path;
	struct btrfs_delayed_ref_root *delayed_refs = NULL;
	struct btrfs_delayed_ref_head *head;
	int info_level = 0;
	int ret;
	struct list_head prefs_delayed;
	struct list_head prefs;
	struct __prelim_ref *ref;
	struct extent_inode_elem *eie = NULL;
	u64 total_refs = 0;
#ifdef MY_ABC_HERE
	u64 num_bytes = 0x10000000;  
#endif  

	INIT_LIST_HEAD(&prefs);
	INIT_LIST_HEAD(&prefs_delayed);

	key.objectid = bytenr;
	key.offset = (u64)-1;
	if (btrfs_fs_incompat(fs_info, SKINNY_METADATA))
		key.type = BTRFS_METADATA_ITEM_KEY;
	else
		key.type = BTRFS_EXTENT_ITEM_KEY;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	if (!trans) {
		path->search_commit_root = 1;
		path->skip_locking = 1;
	}

again:
	head = NULL;

	ret = btrfs_search_slot(trans, fs_info->extent_root, &key, path, 0, 0);
	if (ret < 0)
		goto out;
	BUG_ON(ret == 0);

#ifdef CONFIG_BTRFS_FS_RUN_SANITY_TESTS
	if (trans && likely(trans->type != __TRANS_DUMMY)) {
#else
	if (trans) {
#endif
		 
		delayed_refs = &trans->transaction->delayed_refs;
		spin_lock(&delayed_refs->lock);
		head = btrfs_find_delayed_ref_head(trans, bytenr);
		if (head) {
#ifdef MY_ABC_HERE
			if (in_run_delayed) {
				 
			} else
#endif  
			if (!mutex_trylock(&head->mutex)) {
				atomic_inc(&head->node.refs);
				spin_unlock(&delayed_refs->lock);

				btrfs_release_path(path);

				mutex_lock(&head->mutex);
				mutex_unlock(&head->mutex);
				btrfs_put_delayed_ref(&head->node);
				goto again;
			}
			spin_unlock(&delayed_refs->lock);
			ret = __add_delayed_refs(head, time_seq,
						 &prefs_delayed, &total_refs,
#ifdef MY_ABC_HERE
						 root_objectid, inum, mode);
			if (!in_run_delayed)
#else
						 inum);
#endif  
			mutex_unlock(&head->mutex);
			if (ret)
				goto out;
		} else {
			spin_unlock(&delayed_refs->lock);
		}
	}

	if (path->slots[0]) {
		struct extent_buffer *leaf;
		int slot;

		path->slots[0]--;
		leaf = path->nodes[0];
		slot = path->slots[0];
		btrfs_item_key_to_cpu(leaf, &key, slot);
		if (key.objectid == bytenr &&
		    (key.type == BTRFS_EXTENT_ITEM_KEY ||
		     key.type == BTRFS_METADATA_ITEM_KEY)) {
#ifdef MY_ABC_HERE
			num_bytes = key.offset;
#endif  
			ret = __add_inline_refs(fs_info, path, bytenr,
						&info_level, &prefs,
#ifdef MY_ABC_HERE
						NULL, NULL, NULL, NULL, NULL,
#endif  
#ifdef MY_ABC_HERE
						&total_refs, root_objectid,
						inum, mode);
#else
						&total_refs, inum);
#endif  
			if (ret)
				goto out;
			ret = __add_keyed_refs(fs_info, path, bytenr,
#ifdef MY_ABC_HERE
					       NULL, NULL, NULL, NULL, NULL,
#endif  
#ifdef MY_ABC_HERE
					       info_level, &prefs,
					       &total_refs, root_objectid,
					       inum, mode);
#else
					       info_level, &prefs, inum);
#endif  
			if (ret)
				goto out;
		}
	}
	btrfs_release_path(path);

	list_splice_init(&prefs_delayed, &prefs);

	ret = __add_missing_keys(fs_info, &prefs);
	if (ret)
		goto out;

	__merge_refs(&prefs, 1);

	ret = __resolve_indirect_refs(fs_info, path, time_seq, &prefs,
				      extent_item_pos, total_refs,
#ifdef MY_ABC_HERE
				      root_objectid,
#ifdef MY_ABC_HERE
				      0, 0, 0,
#endif  
				      mode, num_bytes);
#else
				      root_objectid);
#endif  
	if (ret)
		goto out;

	__merge_refs(&prefs, 2);

	while (!list_empty(&prefs)) {
		ref = list_first_entry(&prefs, struct __prelim_ref, list);
		WARN_ON(ref->count < 0);
		if (roots && ref->count && ref->root_id && ref->parent == 0) {
			if (root_objectid && ref->root_id != root_objectid) {
				ret = BACKREF_FOUND_SHARED;
				goto out;
			}

			ret = ulist_add(roots, ref->root_id, 0, GFP_NOFS);
			if (ret < 0)
				goto out;
		}
		if (ref->count && ref->parent) {
			if (extent_item_pos && !ref->inode_list &&
			    ref->level == 0) {
				u32 bsz;
				struct extent_buffer *eb;
				bsz = btrfs_level_size(fs_info->extent_root,
							ref->level);
				eb = read_tree_block(fs_info->extent_root,
							   ref->parent, bsz, 0);
				if (!eb || !extent_buffer_uptodate(eb)) {
					free_extent_buffer(eb);
					ret = -EIO;
					goto out;
				}
				btrfs_tree_read_lock(eb);
				btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
				ret = find_extent_in_eb(eb, bytenr,
							*extent_item_pos, &eie);
				btrfs_tree_read_unlock_blocking(eb);
				free_extent_buffer(eb);
				if (ret < 0)
					goto out;
				ref->inode_list = eie;
			}
			ret = ulist_add_merge(refs, ref->parent,
					      (uintptr_t)ref->inode_list,
					      (u64 *)&eie, GFP_NOFS);
			if (ret < 0)
				goto out;
			if (!ret && extent_item_pos) {
				 
				BUG_ON(!eie);
				while (eie->next)
					eie = eie->next;
				eie->next = ref->inode_list;
			}
			eie = NULL;
		}
		list_del(&ref->list);
		kmem_cache_free(btrfs_prelim_ref_cache, ref);
	}

out:
	btrfs_free_path(path);
	while (!list_empty(&prefs)) {
		ref = list_first_entry(&prefs, struct __prelim_ref, list);
		list_del(&ref->list);
		kmem_cache_free(btrfs_prelim_ref_cache, ref);
	}
	while (!list_empty(&prefs_delayed)) {
		ref = list_first_entry(&prefs_delayed, struct __prelim_ref,
				       list);
		list_del(&ref->list);
		kmem_cache_free(btrfs_prelim_ref_cache, ref);
	}
	if (ret < 0)
		free_inode_elem_list(eie);
	return ret;
}

static void free_leaf_list(struct ulist *blocks)
{
	struct ulist_node *node = NULL;
	struct extent_inode_elem *eie;
	struct ulist_iterator uiter;

	ULIST_ITER_INIT(&uiter);
	while ((node = ulist_next(blocks, &uiter))) {
		if (!node->aux)
			continue;
		eie = (struct extent_inode_elem *)(uintptr_t)node->aux;
		free_inode_elem_list(eie);
		node->aux = 0;
	}

	ulist_free(blocks);
}

static int btrfs_find_all_leafs(struct btrfs_trans_handle *trans,
				struct btrfs_fs_info *fs_info, u64 bytenr,
				u64 time_seq, struct ulist **leafs,
				const u64 *extent_item_pos)
{
	int ret;

	*leafs = ulist_alloc(GFP_NOFS);
	if (!*leafs)
		return -ENOMEM;

	ret = find_parent_nodes(trans, fs_info, bytenr,
#ifdef MY_ABC_HERE
				time_seq, *leafs, NULL, extent_item_pos, 0, 0, 0, 0);
#else
				time_seq, *leafs, NULL, extent_item_pos, 0, 0);
#endif  
	if (ret < 0 && ret != -ENOENT) {
		free_leaf_list(*leafs);
		return ret;
	}

	return 0;
}

static int __btrfs_find_all_roots(struct btrfs_trans_handle *trans,
				  struct btrfs_fs_info *fs_info, u64 bytenr,
#ifdef MY_ABC_HERE
				  u64 time_seq, struct ulist **roots,
				  u64 root_objectid, enum btrfs_backref_mode mode)
#else
				  u64 time_seq, struct ulist **roots)
#endif  
{
	struct ulist *tmp;
	struct ulist_node *node = NULL;
	struct ulist_iterator uiter;
	int ret;

	tmp = ulist_alloc(GFP_NOFS);
	if (!tmp)
		return -ENOMEM;
	*roots = ulist_alloc(GFP_NOFS);
	if (!*roots) {
		ulist_free(tmp);
		return -ENOMEM;
	}

	ULIST_ITER_INIT(&uiter);
	while (1) {
		ret = find_parent_nodes(trans, fs_info, bytenr,
#ifdef MY_ABC_HERE
					time_seq, tmp, *roots, NULL,
					0, 0, mode, 0);
#else
					time_seq, tmp, *roots, NULL, 0, 0);
#endif  
		if (ret < 0 && ret != -ENOENT) {
			ulist_free(tmp);
			ulist_free(*roots);
			return ret;
		}
		node = ulist_next(tmp, &uiter);
		if (!node)
			break;
		bytenr = node->val;
		cond_resched();
	}

	ulist_free(tmp);
	return 0;
}

#ifdef MY_ABC_HERE
 
static int __btrfs_find_all_roots_shared(struct btrfs_fs_info *fs_info,
				  u64 bytenr, u64 leaf_bytenr, u64 datao, struct ulist *roots,
				  u64 root_id, u64 inum, u64 file_offset)
{
	struct ulist *tmp;
	struct ulist_node *node = NULL;
	struct ulist_iterator uiter;
	int ret = 0;

	tmp = ulist_alloc(GFP_NOFS);
	if (!tmp)
		return -ENOMEM;

	ULIST_ITER_INIT(&uiter);
	while (1) {
		ret = find_parent_nodes_shared_root(fs_info, bytenr, leaf_bytenr, datao,
					tmp, roots, root_id, inum, file_offset);
		if (ret == BACKREF_NEXT_ITEM || ret == BACKREF_FOUND_SHARED_ROOT) {
			ulist_free(tmp);
			return ret;
		}
		if (ret < 0 && ret != -ENOENT) {
			ulist_free(tmp);
			return ret;
		}

		node = ulist_next(tmp, &uiter);
		if (!node)
			break;
		bytenr = node->val;
		cond_resched();
	}

	ulist_free(tmp);
	return 0;
}

int btrfs_find_shared_root(struct btrfs_fs_info *fs_info, u64 bytenr, u64 datao,
			 struct ulist *root_list, struct btrfs_snapshot_size_entry *entry,
			 struct btrfs_snapshot_size_ctx *ctx)
{
	int ret;
	u64 leaf_bytenr = 0;

	if (entry->level == 0)
		leaf_bytenr = entry->path->nodes[0]->start;

	down_read(&fs_info->commit_root_sem);
	ret = __btrfs_find_all_roots_shared(fs_info, bytenr, leaf_bytenr, datao,
						root_list, entry->root_id, entry->key.objectid,
						entry->key.offset);
	up_read(&fs_info->commit_root_sem);

	WARN_ON(ret > 0 && ret != BACKREF_NEXT_ITEM && ret != BACKREF_FOUND_SHARED_ROOT);

	return ret;
}

#endif  

int btrfs_find_all_roots(struct btrfs_trans_handle *trans,
			 struct btrfs_fs_info *fs_info, u64 bytenr,
			 u64 time_seq, struct ulist **roots)
{
	int ret;

	if (!trans)
		down_read(&fs_info->commit_root_sem);
#ifdef MY_ABC_HERE
	ret = __btrfs_find_all_roots(trans, fs_info, bytenr, time_seq, roots, 0, 0);
#else
	ret = __btrfs_find_all_roots(trans, fs_info, bytenr, time_seq, roots);
#endif  
	if (!trans)
		up_read(&fs_info->commit_root_sem);
	return ret;
}

int btrfs_check_shared(struct btrfs_trans_handle *trans,
		       struct btrfs_fs_info *fs_info, u64 root_objectid,
		       u64 inum, u64 bytenr)
{
	struct ulist *tmp = NULL;
	struct ulist *roots = NULL;
	struct ulist_iterator uiter;
	struct ulist_node *node;
	struct seq_list elem = {};
	int ret = 0;

	tmp = ulist_alloc(GFP_NOFS);
	roots = ulist_alloc(GFP_NOFS);
	if (!tmp || !roots) {
		ulist_free(tmp);
		ulist_free(roots);
		return -ENOMEM;
	}

	if (trans)
		btrfs_get_tree_mod_seq(fs_info, &elem);
	else
		down_read(&fs_info->commit_root_sem);
	ULIST_ITER_INIT(&uiter);
	while (1) {
		ret = find_parent_nodes(trans, fs_info, bytenr, elem.seq, tmp,
#ifdef MY_ABC_HERE
					roots, NULL, root_objectid, inum, 0, 0);
#else
					roots, NULL, root_objectid, inum);
#endif  
		if (ret == BACKREF_FOUND_SHARED) {
			 
			ret = 1;
			break;
		}
		if (ret < 0 && ret != -ENOENT)
			break;
		ret = 0;
		node = ulist_next(tmp, &uiter);
		if (!node)
			break;
		bytenr = node->val;
		cond_resched();
	}
	if (trans)
		btrfs_put_tree_mod_seq(fs_info, &elem);
	else
		up_read(&fs_info->commit_root_sem);
	ulist_free(tmp);
	ulist_free(roots);
	return ret;
}

int inode_item_info(u64 inum, u64 ioff, struct btrfs_root *fs_root,
			struct btrfs_path *path)
{
	struct btrfs_key key;
	return btrfs_find_item(fs_root, path, inum, ioff,
			BTRFS_INODE_ITEM_KEY, &key);
}

static int inode_ref_info(u64 inum, u64 ioff, struct btrfs_root *fs_root,
				struct btrfs_path *path,
				struct btrfs_key *found_key)
{
	return btrfs_find_item(fs_root, path, inum, ioff,
			BTRFS_INODE_REF_KEY, found_key);
}

int btrfs_find_one_extref(struct btrfs_root *root, u64 inode_objectid,
			  u64 start_off, struct btrfs_path *path,
			  struct btrfs_inode_extref **ret_extref,
			  u64 *found_off)
{
	int ret, slot;
	struct btrfs_key key;
	struct btrfs_key found_key;
	struct btrfs_inode_extref *extref;
	struct extent_buffer *leaf;
	unsigned long ptr;

	key.objectid = inode_objectid;
	btrfs_set_key_type(&key, BTRFS_INODE_EXTREF_KEY);
	key.offset = start_off;

	ret = btrfs_search_slot(NULL, root, &key, path, 0, 0);
	if (ret < 0)
		return ret;

	while (1) {
		leaf = path->nodes[0];
		slot = path->slots[0];
		if (slot >= btrfs_header_nritems(leaf)) {
			 
			ret = btrfs_next_leaf(root, path);
			if (ret) {
				if (ret >= 1)
					ret = -ENOENT;
				break;
			}
			continue;
		}

		btrfs_item_key_to_cpu(leaf, &found_key, slot);

		ret = -ENOENT;
		if (found_key.objectid != inode_objectid)
			break;
		if (btrfs_key_type(&found_key) != BTRFS_INODE_EXTREF_KEY)
			break;

		ret = 0;
		ptr = btrfs_item_ptr_offset(leaf, path->slots[0]);
		extref = (struct btrfs_inode_extref *)ptr;
		*ret_extref = extref;
		if (found_off)
			*found_off = found_key.offset;
		break;
	}

	return ret;
}

char *btrfs_ref_to_path(struct btrfs_root *fs_root, struct btrfs_path *path,
			u32 name_len, unsigned long name_off,
			struct extent_buffer *eb_in, u64 parent,
			char *dest, u32 size)
{
	int slot;
	u64 next_inum;
	int ret;
	s64 bytes_left = ((s64)size) - 1;
	struct extent_buffer *eb = eb_in;
	struct btrfs_key found_key;
	int leave_spinning = path->leave_spinning;
	struct btrfs_inode_ref *iref;

	if (bytes_left >= 0)
		dest[bytes_left] = '\0';

	path->leave_spinning = 1;
	while (1) {
		bytes_left -= name_len;
		if (bytes_left >= 0)
			read_extent_buffer(eb, dest + bytes_left,
					   name_off, name_len);
		if (eb != eb_in) {
			if (!path->skip_locking)
				btrfs_tree_read_unlock_blocking(eb);
			free_extent_buffer(eb);
		}
		ret = inode_ref_info(parent, 0, fs_root, path, &found_key);
		if (ret > 0)
			ret = -ENOENT;
		if (ret)
			break;

		next_inum = found_key.offset;

		if (parent == next_inum)
			break;

		slot = path->slots[0];
		eb = path->nodes[0];
		 
		if (eb != eb_in) {
			if (!path->skip_locking)
				btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
			path->nodes[0] = NULL;
			path->locks[0] = 0;
		}
		btrfs_release_path(path);
		iref = btrfs_item_ptr(eb, slot, struct btrfs_inode_ref);

		name_len = btrfs_inode_ref_name_len(eb, iref);
		name_off = (unsigned long)(iref + 1);

		parent = next_inum;
		--bytes_left;
		if (bytes_left >= 0)
			dest[bytes_left] = '/';
	}

	btrfs_release_path(path);
	path->leave_spinning = leave_spinning;

	if (ret)
		return ERR_PTR(ret);

	return dest + bytes_left;
}

int extent_from_logical(struct btrfs_fs_info *fs_info, u64 logical,
			struct btrfs_path *path, struct btrfs_key *found_key,
			u64 *flags_ret)
{
	int ret;
	u64 flags;
	u64 size = 0;
	u32 item_size;
	struct extent_buffer *eb;
	struct btrfs_extent_item *ei;
	struct btrfs_key key;

	if (btrfs_fs_incompat(fs_info, SKINNY_METADATA))
		key.type = BTRFS_METADATA_ITEM_KEY;
	else
		key.type = BTRFS_EXTENT_ITEM_KEY;
	key.objectid = logical;
	key.offset = (u64)-1;

	ret = btrfs_search_slot(NULL, fs_info->extent_root, &key, path, 0, 0);
	if (ret < 0)
		return ret;

	ret = btrfs_previous_extent_item(fs_info->extent_root, path, 0);
	if (ret) {
		if (ret > 0)
			ret = -ENOENT;
		return ret;
	}
	btrfs_item_key_to_cpu(path->nodes[0], found_key, path->slots[0]);
	if (found_key->type == BTRFS_METADATA_ITEM_KEY)
		size = fs_info->extent_root->leafsize;
	else if (found_key->type == BTRFS_EXTENT_ITEM_KEY)
		size = found_key->offset;

	if (found_key->objectid > logical ||
	    found_key->objectid + size <= logical) {
		pr_debug("logical %llu is not within any extent\n", logical);
		return -ENOENT;
	}

	eb = path->nodes[0];
	item_size = btrfs_item_size_nr(eb, path->slots[0]);
	BUG_ON(item_size < sizeof(*ei));

	ei = btrfs_item_ptr(eb, path->slots[0], struct btrfs_extent_item);
	flags = btrfs_extent_flags(eb, ei);

	pr_debug("logical %llu is at position %llu within the extent (%llu "
		 "EXTENT_ITEM %llu) flags %#llx size %u\n",
		 logical, logical - found_key->objectid, found_key->objectid,
		 found_key->offset, flags, item_size);

	WARN_ON(!flags_ret);
	if (flags_ret) {
		if (flags & BTRFS_EXTENT_FLAG_TREE_BLOCK)
			*flags_ret = BTRFS_EXTENT_FLAG_TREE_BLOCK;
		else if (flags & BTRFS_EXTENT_FLAG_DATA)
			*flags_ret = BTRFS_EXTENT_FLAG_DATA;
		else
			BUG_ON(1);
		return 0;
	}

	return -EIO;
}

static int __get_extent_inline_ref(unsigned long *ptr, struct extent_buffer *eb,
				   struct btrfs_key *key,
				   struct btrfs_extent_item *ei, u32 item_size,
				   struct btrfs_extent_inline_ref **out_eiref,
				   int *out_type)
{
	unsigned long end;
	u64 flags;
	struct btrfs_tree_block_info *info;

	if (!*ptr) {
		 
		flags = btrfs_extent_flags(eb, ei);
		if (flags & BTRFS_EXTENT_FLAG_TREE_BLOCK) {
			if (key->type == BTRFS_METADATA_ITEM_KEY) {
				 
				*out_eiref =
				     (struct btrfs_extent_inline_ref *)(ei + 1);
			} else {
				WARN_ON(key->type != BTRFS_EXTENT_ITEM_KEY);
				info = (struct btrfs_tree_block_info *)(ei + 1);
				*out_eiref =
				   (struct btrfs_extent_inline_ref *)(info + 1);
			}
		} else {
			*out_eiref = (struct btrfs_extent_inline_ref *)(ei + 1);
		}
		*ptr = (unsigned long)*out_eiref;
		if ((unsigned long)(*ptr) >= (unsigned long)ei + item_size)
			return -ENOENT;
	}

	end = (unsigned long)ei + item_size;
	*out_eiref = (struct btrfs_extent_inline_ref *)(*ptr);
	*out_type = btrfs_extent_inline_ref_type(eb, *out_eiref);

	*ptr += btrfs_extent_inline_ref_size(*out_type);
	WARN_ON(*ptr > end);
	if (*ptr == end)
		return 1;  

	return 0;
}

int tree_backref_for_extent(unsigned long *ptr, struct extent_buffer *eb,
			    struct btrfs_key *key, struct btrfs_extent_item *ei,
			    u32 item_size, u64 *out_root, u8 *out_level)
{
	int ret;
	int type;
	struct btrfs_extent_inline_ref *eiref;

	if (*ptr == (unsigned long)-1)
		return 1;

	while (1) {
		ret = __get_extent_inline_ref(ptr, eb, key, ei, item_size,
					      &eiref, &type);
		if (ret < 0)
			return ret;

		if (type == BTRFS_TREE_BLOCK_REF_KEY ||
		    type == BTRFS_SHARED_BLOCK_REF_KEY)
			break;

		if (ret == 1)
			return 1;
	}

	*out_root = btrfs_extent_inline_ref_offset(eb, eiref);

	if (key->type == BTRFS_EXTENT_ITEM_KEY) {
		struct btrfs_tree_block_info *info;

		info = (struct btrfs_tree_block_info *)(ei + 1);
		*out_level = btrfs_tree_block_level(eb, info);
	} else {
		ASSERT(key->type == BTRFS_METADATA_ITEM_KEY);
		*out_level = (u8)key->offset;
	}

	if (ret == 1)
		*ptr = (unsigned long)-1;

	return 0;
}

static int iterate_leaf_refs(struct extent_inode_elem *inode_list,
				u64 root, u64 extent_item_objectid,
				iterate_extent_inodes_t *iterate, void *ctx)
{
	struct extent_inode_elem *eie;
	int ret = 0;

	for (eie = inode_list; eie; eie = eie->next) {
		pr_debug("ref for %llu resolved, key (%llu EXTEND_DATA %llu), "
			 "root %llu\n", extent_item_objectid,
			 eie->inum, eie->offset, root);
		ret = iterate(eie->inum, eie->offset, root, ctx);
		if (ret) {
			pr_debug("stopping iteration for %llu due to ret=%d\n",
				 extent_item_objectid, ret);
			break;
		}
	}

	return ret;
}

int iterate_extent_inodes(struct btrfs_fs_info *fs_info,
				u64 extent_item_objectid, u64 extent_item_pos,
				int search_commit_root,
				iterate_extent_inodes_t *iterate, void *ctx)
{
	int ret;
	struct btrfs_trans_handle *trans = NULL;
	struct ulist *refs = NULL;
	struct ulist *roots = NULL;
	struct ulist_node *ref_node = NULL;
	struct ulist_node *root_node = NULL;
	struct seq_list tree_mod_seq_elem = {};
	struct ulist_iterator ref_uiter;
	struct ulist_iterator root_uiter;

	pr_debug("resolving all inodes for extent %llu\n",
			extent_item_objectid);

	if (!search_commit_root) {
		trans = btrfs_join_transaction(fs_info->extent_root);
		if (IS_ERR(trans))
			return PTR_ERR(trans);
		btrfs_get_tree_mod_seq(fs_info, &tree_mod_seq_elem);
	} else {
		down_read(&fs_info->commit_root_sem);
	}

	ret = btrfs_find_all_leafs(trans, fs_info, extent_item_objectid,
				   tree_mod_seq_elem.seq, &refs,
				   &extent_item_pos);
	if (ret)
		goto out;

	ULIST_ITER_INIT(&ref_uiter);
	while (!ret && (ref_node = ulist_next(refs, &ref_uiter))) {
		ret = __btrfs_find_all_roots(trans, fs_info, ref_node->val,
#ifdef MY_ABC_HERE
					     tree_mod_seq_elem.seq, &roots, 0, 0);
#else
					     tree_mod_seq_elem.seq, &roots);
#endif  
		if (ret)
			break;
		ULIST_ITER_INIT(&root_uiter);
		while (!ret && (root_node = ulist_next(roots, &root_uiter))) {
			pr_debug("root %llu references leaf %llu, data list "
				 "%#llx\n", root_node->val, ref_node->val,
				 ref_node->aux);
			ret = iterate_leaf_refs((struct extent_inode_elem *)
						(uintptr_t)ref_node->aux,
						root_node->val,
						extent_item_objectid,
						iterate, ctx);
		}
		ulist_free(roots);
	}

	free_leaf_list(refs);
out:
	if (!search_commit_root) {
		btrfs_put_tree_mod_seq(fs_info, &tree_mod_seq_elem);
		btrfs_end_transaction(trans, fs_info->extent_root);
	} else {
		up_read(&fs_info->commit_root_sem);
	}

	return ret;
}

int iterate_inodes_from_logical(u64 logical, struct btrfs_fs_info *fs_info,
				struct btrfs_path *path,
				iterate_extent_inodes_t *iterate, void *ctx)
{
	int ret;
	u64 extent_item_pos;
	u64 flags = 0;
	struct btrfs_key found_key;
	int search_commit_root = path->search_commit_root;

	ret = extent_from_logical(fs_info, logical, path, &found_key, &flags);
	btrfs_release_path(path);
	if (ret < 0)
		return ret;
	if (flags & BTRFS_EXTENT_FLAG_TREE_BLOCK)
		return -EINVAL;

	extent_item_pos = logical - found_key.objectid;
	ret = iterate_extent_inodes(fs_info, found_key.objectid,
					extent_item_pos, search_commit_root,
					iterate, ctx);

	return ret;
}

typedef int (iterate_irefs_t)(u64 parent, u32 name_len, unsigned long name_off,
			      struct extent_buffer *eb, void *ctx);

static int iterate_inode_refs(u64 inum, struct btrfs_root *fs_root,
			      struct btrfs_path *path,
			      iterate_irefs_t *iterate, void *ctx)
{
	int ret = 0;
	int slot;
	u32 cur;
	u32 len;
	u32 name_len;
	u64 parent = 0;
	int found = 0;
	struct extent_buffer *eb;
	struct btrfs_item *item;
	struct btrfs_inode_ref *iref;
	struct btrfs_key found_key;

	while (!ret) {
		ret = inode_ref_info(inum, parent ? parent+1 : 0, fs_root, path,
				     &found_key);
		if (ret < 0)
			break;
		if (ret) {
			ret = found ? 0 : -ENOENT;
			break;
		}
		++found;

		parent = found_key.offset;
		slot = path->slots[0];
		eb = btrfs_clone_extent_buffer(path->nodes[0]);
		if (!eb) {
			ret = -ENOMEM;
			break;
		}
		extent_buffer_get(eb);
		btrfs_tree_read_lock(eb);
		btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
		btrfs_release_path(path);

		item = btrfs_item_nr(slot);
		iref = btrfs_item_ptr(eb, slot, struct btrfs_inode_ref);

		for (cur = 0; cur < btrfs_item_size(eb, item); cur += len) {
			name_len = btrfs_inode_ref_name_len(eb, iref);
			 
			pr_debug("following ref at offset %u for inode %llu in "
				 "tree %llu\n", cur, found_key.objectid,
				 fs_root->objectid);
			ret = iterate(parent, name_len,
				      (unsigned long)(iref + 1), eb, ctx);
			if (ret)
				break;
			len = sizeof(*iref) + name_len;
			iref = (struct btrfs_inode_ref *)((char *)iref + len);
		}
		btrfs_tree_read_unlock_blocking(eb);
		free_extent_buffer(eb);
	}

	btrfs_release_path(path);

	return ret;
}

static int iterate_inode_extrefs(u64 inum, struct btrfs_root *fs_root,
				 struct btrfs_path *path,
				 iterate_irefs_t *iterate, void *ctx)
{
	int ret;
	int slot;
	u64 offset = 0;
	u64 parent;
	int found = 0;
	struct extent_buffer *eb;
	struct btrfs_inode_extref *extref;
	struct extent_buffer *leaf;
	u32 item_size;
	u32 cur_offset;
	unsigned long ptr;

	while (1) {
		ret = btrfs_find_one_extref(fs_root, inum, offset, path, &extref,
					    &offset);
		if (ret < 0)
			break;
		if (ret) {
			ret = found ? 0 : -ENOENT;
			break;
		}
		++found;

		slot = path->slots[0];
		eb = btrfs_clone_extent_buffer(path->nodes[0]);
		if (!eb) {
			ret = -ENOMEM;
			break;
		}
		extent_buffer_get(eb);

		btrfs_tree_read_lock(eb);
		btrfs_set_lock_blocking_rw(eb, BTRFS_READ_LOCK);
		btrfs_release_path(path);

		leaf = path->nodes[0];
		item_size = btrfs_item_size_nr(leaf, slot);
		ptr = btrfs_item_ptr_offset(leaf, slot);
		cur_offset = 0;

		while (cur_offset < item_size) {
			u32 name_len;

			extref = (struct btrfs_inode_extref *)(ptr + cur_offset);
			parent = btrfs_inode_extref_parent(eb, extref);
			name_len = btrfs_inode_extref_name_len(eb, extref);
			ret = iterate(parent, name_len,
				      (unsigned long)&extref->name, eb, ctx);
			if (ret)
				break;

			cur_offset += btrfs_inode_extref_name_len(leaf, extref);
			cur_offset += sizeof(*extref);
		}
		btrfs_tree_read_unlock_blocking(eb);
		free_extent_buffer(eb);

		offset++;
	}

	btrfs_release_path(path);

	return ret;
}

static int iterate_irefs(u64 inum, struct btrfs_root *fs_root,
			 struct btrfs_path *path, iterate_irefs_t *iterate,
			 void *ctx)
{
	int ret;
	int found_refs = 0;

	ret = iterate_inode_refs(inum, fs_root, path, iterate, ctx);
	if (!ret)
		++found_refs;
	else if (ret != -ENOENT)
		return ret;

	ret = iterate_inode_extrefs(inum, fs_root, path, iterate, ctx);
	if (ret == -ENOENT && found_refs)
		return 0;

	return ret;
}

static int inode_to_path(u64 inum, u32 name_len, unsigned long name_off,
			 struct extent_buffer *eb, void *ctx)
{
	struct inode_fs_paths *ipath = ctx;
	char *fspath;
	char *fspath_min;
	int i = ipath->fspath->elem_cnt;
	const int s_ptr = sizeof(char *);
	u32 bytes_left;

	bytes_left = ipath->fspath->bytes_left > s_ptr ?
					ipath->fspath->bytes_left - s_ptr : 0;

	fspath_min = (char *)ipath->fspath->val + (i + 1) * s_ptr;
	fspath = btrfs_ref_to_path(ipath->fs_root, ipath->btrfs_path, name_len,
				   name_off, eb, inum, fspath_min, bytes_left);
	if (IS_ERR(fspath))
		return PTR_ERR(fspath);

	if (fspath > fspath_min) {
		ipath->fspath->val[i] = (u64)(unsigned long)fspath;
		++ipath->fspath->elem_cnt;
		ipath->fspath->bytes_left = fspath - fspath_min;
	} else {
		++ipath->fspath->elem_missed;
		ipath->fspath->bytes_missing += fspath_min - fspath;
		ipath->fspath->bytes_left = 0;
	}

	return 0;
}

int paths_from_inode(u64 inum, struct inode_fs_paths *ipath)
{
	return iterate_irefs(inum, ipath->fs_root, ipath->btrfs_path,
			     inode_to_path, ipath);
}

struct btrfs_data_container *init_data_container(u32 total_bytes)
{
	struct btrfs_data_container *data;
	size_t alloc_bytes;

	alloc_bytes = max_t(size_t, total_bytes, sizeof(*data));
	data = vmalloc(alloc_bytes);
	if (!data)
		return ERR_PTR(-ENOMEM);

	if (total_bytes >= sizeof(*data)) {
		data->bytes_left = total_bytes - sizeof(*data);
		data->bytes_missing = 0;
	} else {
		data->bytes_missing = sizeof(*data) - total_bytes;
		data->bytes_left = 0;
	}

	data->elem_cnt = 0;
	data->elem_missed = 0;

	return data;
}

struct inode_fs_paths *init_ipath(s32 total_bytes, struct btrfs_root *fs_root,
					struct btrfs_path *path)
{
	struct inode_fs_paths *ifp;
	struct btrfs_data_container *fspath;

	fspath = init_data_container(total_bytes);
	if (IS_ERR(fspath))
		return (void *)fspath;

	ifp = kmalloc(sizeof(*ifp), GFP_NOFS);
	if (!ifp) {
		kfree(fspath);
		return ERR_PTR(-ENOMEM);
	}

	ifp->btrfs_path = path;
	ifp->fspath = fspath;
	ifp->fs_root = fs_root;

	return ifp;
}

void free_ipath(struct inode_fs_paths *ipath)
{
	if (!ipath)
		return;
	vfree(ipath->fspath);
	kfree(ipath);
}

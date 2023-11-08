/*
 * Copyright (C) 2006, 2010  Novell, Inc.
 * Written by Andreas Gruenbacher <agruen@kernel.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any
 * later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/richacl.h>
#include <linux/posix_acl.h>

/**
 * struct richacl_alloc  -  remember how many entries are actually allocated
 * @acl:	acl with a_count <= @count
 * @count:	the actual number of entries allocated in @acl
 *
 * We pass around this structure while modifying an acl, so that we do
 * not have to reallocate when we remove existing entries followed by
 * adding new entries.
 */
struct richacl_alloc {
	struct richacl *acl;
	unsigned int count;
};

/**
 * richacl_delete_entry  -  delete an entry in an acl
 * @x:		acl and number of allocated entries
 * @ace:	an entry in @x->acl
 *
 * Updates @ace so that it points to the entry before the deleted entry
 * on return. (When deleting the first entry, @ace will point to the
 * (non-existant) entry before the first entry). This behavior is the
 * expected behavior when deleting entries while forward iterating over
 * an acl.
 */
static void
richacl_delete_entry(struct richacl_alloc *x, struct richace **ace)
{
	void *end = x->acl->a_entries + x->acl->a_count;

	memmove(*ace, *ace + 1, end - (void *)(*ace + 1));
	(*ace)--;
	x->acl->a_count--;
}

/**
 * richacl_insert_entry  -  insert an entry in an acl
 * @x:		acl and number of allocated entries
 * @ace:	entry before which the new entry shall be inserted
 *
 * Insert a new entry in @x->acl at position @ace, and zero-initialize
 * it.  This may require reallocating @x->acl.
 */
static int
richacl_insert_entry(struct richacl_alloc *x, struct richace **ace)
{
	if (x->count == x->acl->a_count) {
		int n = *ace - x->acl->a_entries;
		struct richacl *acl2;

		acl2 = richacl_alloc(x->acl->a_count + 1);
		if (!acl2)
			return -1;
		acl2->a_flags = x->acl->a_flags;
		acl2->a_owner_mask = x->acl->a_owner_mask;
		acl2->a_group_mask = x->acl->a_group_mask;
		acl2->a_other_mask = x->acl->a_other_mask;
		memcpy(acl2->a_entries, x->acl->a_entries,
		       n * sizeof(struct richace));
		memcpy(acl2->a_entries + n + 1, *ace,
		       (x->acl->a_count - n) * sizeof(struct richace));
		if (x->acl) {
			kfree(x->acl);
			x->acl = NULL;
		}
		x->acl = acl2;
		x->count = acl2->a_count;
		*ace = acl2->a_entries + n;
	} else {
		void *end = x->acl->a_entries + x->acl->a_count;

		memmove(*ace + 1, *ace, end - (void *)*ace);
		x->acl->a_count++;
	}
	memset(*ace, 0, sizeof(struct richace));
	return 0;
}

/**
 * richace_change_mask  -  set the mask of @ace to @mask
 * @x:		acl and number of allocated entries
 * @ace:	entry to modify
 * @mask:	new mask for @ace
 *
 * If @ace is inheritable, a inherit-only ace is inserted before @ace which
 * includes the inheritable permissions of @ace, and the inheritance flags of
 * @ace are cleared before changing the mask.
 *
 * If @mode is 0, the original ace is turned into an inherit-only entry if
 * there are any inheritable permissions, and removed otherwise.
 *
 * The returned @ace points to the modified or inserted effective-only acl
 * entry if that entry exists, to the entry that has become inheritable-only,
 * or else to the previous entry in the acl.
 */
static int
richace_change_mask(struct richacl_alloc *x, struct richace **ace,
			   unsigned int mask)
{
	if (mask && (*ace)->e_mask == mask)
		return 0;
	if (mask & ~ACE4_POSIX_ALWAYS_ALLOWED) {
		if (richace_is_inheritable(*ace)) {
			if (richacl_insert_entry(x, ace))
				return -1;
			memcpy(*ace, *ace + 1, sizeof(struct richace));
			(*ace)->e_flags |= ACE4_INHERIT_ONLY_ACE;
			(*ace)++;
			richace_clear_inheritance_flags(*ace);
		}
		(*ace)->e_mask = mask;
	} else {
		if (richace_is_inheritable(*ace))
			(*ace)->e_flags |= ACE4_INHERIT_ONLY_ACE;
		else
			richacl_delete_entry(x, ace);
	}
	return 0;
}

/**
 * richacl_move_everyone_aces_down  -  move everyone@ aces to the end of the acl
 * @x:		acl and number of allocated entries
 *
 * Move all everyone aces to the end of the acl so that only a single everyone@
 * allow ace remains at the end, and update the mask fields of all aces on the
 * way.  The last ace of the resulting acl will be an everyone@ allow ace only
 * if @acl grants any permissions to @everyone.
 *
 * Having at most one everyone@ allow ace at the end of the acl helps us in the
 * following algorithms.
 *
 * This transformation does not alter the permissions that the acl grants.
 */
static int
richacl_move_everyone_aces_down(struct richacl_alloc *x)
{
	struct richace *ace;
	unsigned int allowed = 0, denied = 0;

	richacl_for_each_entry(ace, x->acl) {
		if (richace_is_inherit_only(ace))
			continue;
		if (richace_is_everyone(ace)) {
			if (richace_is_allow(ace))
				allowed |= (ace->e_mask & ~denied);
			else if (richace_is_deny(ace))
				denied |= (ace->e_mask & ~allowed);
			else
				continue;
			if (richace_change_mask(x, &ace, 0))
				return -1;
		} else {
			if (richace_is_allow(ace)) {
				if (richace_change_mask(x, &ace, allowed |
						(ace->e_mask & ~denied)))
					return -1;
			} else if (richace_is_deny(ace)) {
				if (richace_change_mask(x, &ace, denied |
						(ace->e_mask & ~allowed)))
					return -1;
			}
		}
	}
	if (allowed & ~ACE4_POSIX_ALWAYS_ALLOWED) {
		struct richace *last_ace = ace - 1;

		if (x->acl->a_entries &&
		    richace_is_everyone(last_ace) &&
		    richace_is_allow(last_ace) &&
		    richace_is_inherit_only(last_ace) &&
		    last_ace->e_mask == allowed)
			last_ace->e_flags &= ~ACE4_INHERIT_ONLY_ACE;
		else {
			if (richacl_insert_entry(x, &ace))
				return -1;
			ace->e_type = ACE4_ACCESS_ALLOWED_ACE_TYPE;
			ace->e_flags = ACE4_SPECIAL_WHO;
			ace->e_mask = allowed;
			ace->e_id = ACE_EVERYONE_ID;
		}
	}
	return 0;
}

/**
 * __richacl_propagate_everyone  -  propagate everyone@ permissions up for @who
 * @x:		acl and number of allocated entries
 * @who:	identifier to propagate permissions for
 * @allow:	permissions to propagate up
 *
 * Propagate the permissions in @allow up from the end of the acl to the start
 * for the specified principal @who.
 *
 * The simplest possible approach to achieve this would be to insert a
 * "<who>:<allow>::allow" ace before the final everyone@ allow ace.  Since this
 * would often result in aces which are not needed or which could be merged
 * with an existing ace, we make the following optimizations:
 *
 *   - We go through the acl and determine which permissions are already
 *     allowed or denied to @who, and we remove those permissions from
 *     @allow.
 *
 *   - If the acl contains an allow ace for @who and no aces after this entry
 *     deny permissions in @allow, we add the permissions in @allow to this
 *     ace.  (Propagating permissions across a deny ace which can match the
 *     process can elevate permissions.)
 *
 * This transformation does not alter the permissions that the acl grants.
 */
static int
__richacl_propagate_everyone(struct richacl_alloc *x, struct richace *who,
			  unsigned int allow)
{
	struct richace *allow_last = NULL, *ace;

	/*
	 * Remove the permissions from allow that are already determined for
	 * this who value, and figure out if there is an ALLOW entry for
	 * this who value that is "reachable" from the trailing EVERYONE@
	 * ALLOW ACE.
	 */
	richacl_for_each_entry(ace, x->acl) {
		if (richace_is_inherit_only(ace))
			continue;
		if (richace_is_allow(ace)) {
			if (richace_is_same_identifier(ace, who)) {
				allow &= ~ace->e_mask;
				allow_last = ace;
			}
		} else if (richace_is_deny(ace)) {
			if (richace_is_same_identifier(ace, who))
				allow &= ~ace->e_mask;
			else if (allow & ace->e_mask)
				allow_last = NULL;
		}
	}
	if (allow) {
		if (allow_last)
			return richace_change_mask(x, &allow_last,
						   allow_last->e_mask | allow);
		else {
			struct richace who_copy;

			ace = x->acl->a_entries + x->acl->a_count - 1;
			memcpy(&who_copy, who, sizeof(struct richace));
			if (richacl_insert_entry(x, &ace))
				return -1;
			memcpy(ace, &who_copy, sizeof(struct richace));
			ace->e_type = ACE4_ACCESS_ALLOWED_ACE_TYPE;
			richace_clear_inheritance_flags(ace);
			ace->e_mask = allow;
		}
	}
	return 0;
}

/**
 * richacl_propagate_everyone  -  propagate everyone@ permissions up the acl
 * @x:		acl and number of allocated entries
 *
 * Make sure that owner@, group@, and all other users and groups mentioned in
 * the acl will not lose any permissions when finally applying the other mask
 * to the everyone@ allow ace at the end of the acl.  If one of those
 * principals is not granted some of those permissions, insert an additional
 * allow ace for that principal into the acl before the final everyone@ allow
 * ace.
 *
 * For example, the following acl implicitly grants everyone rwx access:
 *
 *    joe:r::allow
 *    everyone@:rwx::allow
 *
 * When applying mode 0660 to this acl, owner@ and group@ would lose rw access,
 * and joe would lose w access even though the mode does not exclude those
 * permissions.  To fix this problem, we insert additional allow aces into the
 * acl.  The result after applying mode 0660 is:
 *
 *    joe:rw::allow
 *    owner@:rw::allow
 *    group@:rw::allow
 *
 * Deny aces complicate the matter.  For example, the following acl grants
 * everyone but joe write access:
 *
 *    joe:w::deny
 *    everyone@:rwx::allow
 *
 * When applying mode 0660 to this acl, owner@ and group@ would lose rw access,
 * and joe would lose r access.  The reuslt after inserting additional allow
 * aces and applying mode 0660 is:
 *
 *    joe:w::deny
 *    owner@:rw::allow
 *    group@:rw::allow
 *    joe:r::allow
 *
 * Inserting the additional aces does not alter the permissions that the acl
 * grants.
 */
static int
richacl_propagate_everyone(struct richacl_alloc *x)
{
	struct richace who = { .e_flags = ACE4_SPECIAL_WHO };
	struct richacl *acl = x->acl;
	struct richace *ace;
	unsigned int owner_allow, group_allow;

	/*
	 * If the owner mask contains permissions which are not in
	 * the group mask, the group mask contains permissions which
	 * are not in the other mask, or the owner class contains
	 * permissions which are not in the other mask,	we may need
	 * to propagate permissions up from the everyone@ allow ace.
	 * The third condition is implied by the first two.
	 */
	if (!((acl->a_owner_mask & ~acl->a_group_mask) ||
	      (acl->a_group_mask & ~acl->a_other_mask)))
		return 0;
	if (!acl->a_count)
		return 0;
	ace = acl->a_entries + acl->a_count - 1;
	if (richace_is_inherit_only(ace) || !richace_is_everyone(ace))
		return 0;
	if (!(ace->e_mask & ~(acl->a_group_mask & acl->a_other_mask)))
		/*
		 * None of the allowed permissions will get masked.
		 */
		return 0;
	owner_allow = ace->e_mask & acl->a_owner_mask;
	group_allow = ace->e_mask & acl->a_group_mask;

	/* Propagate everyone@ permissions through to owner@. */
	if (owner_allow & ~(acl->a_group_mask & acl->a_other_mask)) {
		who.e_id = ACE_OWNER_ID;
		if (__richacl_propagate_everyone(x, &who, owner_allow))
			return -1;
		acl = x->acl;
	}

	if (group_allow & ~acl->a_other_mask) {
		int n;

		/* Propagate everyone@ permissions through to group@. */
		who.e_id = ACE_GROUP_ID;
		if (__richacl_propagate_everyone(x, &who, group_allow))
			return -1;
		acl = x->acl;

		/*
		 * Start from the entry before the trailing EVERYONE@ ALLOW
		 * entry. We will not hit EVERYONE@ entries in the loop.
		 */
		for (n = acl->a_count - 2; n != -1; n--) {
			ace = acl->a_entries + n;

			if (richace_is_inherit_only(ace) ||
			    richace_is_owner(ace) ||
			    richace_is_group(ace))
				continue;
			if (richace_is_allow(ace) || richace_is_deny(ace)) {
				/*
				 * Any inserted entry will end up below the
				 * current entry
				 */
				if (__richacl_propagate_everyone(x, ace,
								 group_allow))
					return -1;
			}
		}
	}
	return 0;
}

/**
 * richacl_max_allowed  -  maximum permissions that anybody is allowed
 */
static unsigned int
richacl_max_allowed(struct richacl *acl)
{
	struct richace *ace;
	unsigned int allowed = 0;

	richacl_for_each_entry_reverse(ace, acl) {
		if (richace_is_inherit_only(ace))
			continue;
		if (richace_is_allow(ace))
			allowed |= ace->e_mask;
		else if (richace_is_deny(ace)) {
			if (richace_is_everyone(ace))
				allowed &= ~ace->e_mask;
		}
	}
	return allowed;
}

/**
 * richacl_isolate_owner_class  -  limit the owner class to the owner file mask
 * @x:		acl and number of allocated entries
 *
 * POSIX requires that after a chmod, the owner class is granted no more
 * permissions than the owner file permission bits.  Mapped to richacls, this
 * means that the owner class must not be granted any permissions that the
 * owner mask does not include.
 *
 * When we apply file masks to an acl which grant more permissions to the group
 * or other class than to the owner class, we may end up in a situation where
 * the owner is granted additional permission from other aces.  For example,
 * given this acl:
 *
 *    everyone:rwx::allow
 *
 * when file masks corresponding to mode 0466 are applied, after
 * richacl_propagate_everyone() and __richacl_apply_masks(), we end up with:
 *
 *    owner@:r::allow
 *    everyone@:rw::allow
 *
 * This acl still grants the owner rw access through the everyone@ allow ace.
 * To fix this, we must deny w access to the owner:
 *
 *    owner@:w::deny
 *    owner@:r::allow
 *    everyone@:rw::allow
 */
static int
richacl_isolate_owner_class(struct richacl_alloc *x)
{
	struct richace *ace;
	unsigned int allowed = 0;

	allowed = richacl_max_allowed(x->acl);
	if (allowed & ~x->acl->a_owner_mask) {
		/*
		 * Figure out if we can update an existig OWNER@ DENY entry.
		 */
		richacl_for_each_entry(ace, x->acl) {
			if (richace_is_inherit_only(ace))
				continue;
			if (richace_is_deny(ace)) {
				if (richace_is_owner(ace))
					break;
			} else if (richace_is_allow(ace)) {
				ace = x->acl->a_entries + x->acl->a_count;
				break;
			}
		}
		if (ace != x->acl->a_entries + x->acl->a_count) {
			if (richace_change_mask(x, &ace, ace->e_mask |
					(allowed & ~x->acl->a_owner_mask)))
				return -1;
		} else {
			/* Insert an owner@ deny entry at the front. */
			ace = x->acl->a_entries;
			if (richacl_insert_entry(x, &ace))
				return -1;
			ace->e_type = ACE4_ACCESS_DENIED_ACE_TYPE;
			ace->e_flags = ACE4_SPECIAL_WHO;
			ace->e_mask = allowed & ~x->acl->a_owner_mask;
			ace->e_id = ACE_OWNER_ID;
		}
	}
	return 0;
}

/**
 * __richacl_isolate_who  -  isolate entry from EVERYONE@ ALLOW entry
 * @x:		acl and number of allocated entries
 * @who:	identifier to isolate
 * @deny:	permissions this identifier should not be allowed
 *
 * See richacl_isolate_group_class().
 */
static int
__richacl_isolate_who(struct richacl_alloc *x, struct richace *who,
		      unsigned int deny)
{
	struct richace *ace;
	unsigned int n;
	/*
	 * Compute the permissions already denied to @who.
	 */
	richacl_for_each_entry(ace, x->acl) {
		if (richace_is_inherit_only(ace))
			continue;
		if (richace_is_same_identifier(ace, who) &&
		    richace_is_deny(ace))
			deny &= ~ace->e_mask;
	}
	if (!deny)
		return 0;

	/*
	 * Figure out if we can update an existig DENY entry.  Start from the
	 * entry before the trailing EVERYONE@ ALLOW entry. We will not hit
	 * EVERYONE@ entries in the loop.
	 */
	for (n = x->acl->a_count - 2; n != -1; n--) {
		ace = x->acl->a_entries + n;
		if (richace_is_inherit_only(ace))
			continue;
		if (richace_is_deny(ace)) {
			if (richace_is_same_identifier(ace, who))
				break;
		} else if (richace_is_allow(ace) &&
			   (ace->e_mask & deny)) {
			n = -1;
			break;
		}
	}
	if (n != -1) {
		if (richace_change_mask(x, &ace, ace->e_mask | deny))
			return -1;
	} else {
		/*
		 * Insert a new entry before the trailing EVERYONE@ DENY entry.
		 */
		struct richace who_copy;

		ace = x->acl->a_entries + x->acl->a_count - 1;
		memcpy(&who_copy, who, sizeof(struct richace));
		if (richacl_insert_entry(x, &ace))
			return -1;
		memcpy(ace, &who_copy, sizeof(struct richace));
		ace->e_type = ACE4_ACCESS_DENIED_ACE_TYPE;
		richace_clear_inheritance_flags(ace);
		ace->e_mask = deny;
	}
	return 0;
}

/**
 * richacl_isolate_group_class  -  limit the group class to the group file mask
 * @x:		acl and number of allocated entries
 *
 * POSIX requires that after a chmod, the group class is granted no more
 * permissions than the group file permission bits.  Mapped to richacls, this
 * means that the group class must not be granted any permissions that the
 * group mask does not include.
 *
 * When we apply file masks to an acl which grant more permissions to the other
 * class than to the group class, we may end up in a situation where processes
 * in the group class are granted additional permission from other aces.  For
 * example, given this acl:
 *
 *    joe:rwx::allow
 *    everyone:rwx::allow
 *
 * when file masks corresponding to mode 0646 are applied, after
 * richacl_propagate_everyone() and __richacl_apply_masks(), we end up with:
 *
 *    joe:r::allow
 *    owner@:rw::allow
 *    group@:r::allow
 *    everyone@:rw::allow
 *
 * This acl still grants joe and group@ rw access through the everyone@ allow
 * ace.  To fix this, we must deny w access to group class aces before the
 * everyone@ allow ace at the end of the acl:
 *
 *    joe:r::allow
 *    owner@:rw::allow
 *    group@:r::allow
 *    joe:w::deny
 *    group@:w::deny
 *    everyone@:rw::allow
 */
static int
richacl_isolate_group_class(struct richacl_alloc *x)
{
	struct richace who = {
		.e_flags = ACE4_SPECIAL_WHO,
		.e_id = ACE_GROUP_ID,
	};
	struct richace *ace;
	unsigned int deny;

	if (!x->acl->a_count)
		return 0;
	ace = x->acl->a_entries + x->acl->a_count - 1;
	if (richace_is_inherit_only(ace) || !richace_is_everyone(ace))
		return 0;
	deny = ace->e_mask & ~x->acl->a_group_mask;

	if (deny) {
		unsigned int n;

		if (__richacl_isolate_who(x, &who, deny))
			return -1;
		/*
		 * Start from the entry before the trailing EVERYONE@ ALLOW
		 * entry. We will not hit EVERYONE@ entries in the loop.
		 */
		for (n = x->acl->a_count - 2; n != -1; n--) {
			ace = x->acl->a_entries + n;

			if (richace_is_inherit_only(ace) ||
			    richace_is_owner(ace) ||
			    richace_is_group(ace))
				continue;
			if (__richacl_isolate_who(x, ace, deny))
				return -1;
		}
	}
	return 0;
}

/**
 * __richacl_apply_masks  -  apply the file masks to all aces
 * @x:		acl and number of allocated entries
 *
 * Apply the owner mask to owner@ aces, the other mask to
 * everyone@ aces, and the group mask to all other aces.
 *
 * The previous transformations have brought the acl into a
 * form in which applying the masks will not lead to the
 * accidental loss of permissions anymore.
 */
static int
__richacl_apply_masks(struct richacl_alloc *x)
{
	struct richace *ace;

	richacl_for_each_entry(ace, x->acl) {
		unsigned int mask;

		if (richace_is_inherit_only(ace) || !richace_is_allow(ace))
			continue;
		if (richace_is_owner(ace))
			mask = x->acl->a_owner_mask;
		else if (richace_is_everyone(ace))
			mask = x->acl->a_other_mask;
		else
			mask = x->acl->a_group_mask;
		if (richace_change_mask(x, &ace, ace->e_mask & mask))
			return -1;
	}
	return 0;
}

/**
 * richacl_apply_masks  -  apply the masks to the acl
 *
 * Transform @acl so that the standard NFSv4 permission check algorithm (which
 * is not aware of file masks) will compute the same access decisions as the
 * richacl permission check algorithm (which looks at the acl and the file
 * masks).
 *
 * This algorithm is split into several steps:
 *
 *   - Move everyone@ aces to the end of the acl.  This simplifies the other
 *     transformations, and allows the everyone@ allow ace at the end of the
 *     acl to eventually allow permissions to the other class only.
 *
 *   - Propagate everyone@ permissions up the acl.  This transformation makes
 *     sure that the owner and group class aces won't lose any permissions when
 *     we apply the other mask to the everyone@ allow ace at the end of the acl.
 *
 *   - Apply the file masks to all aces.
 *
 *   - Make sure that the owner is not granted any permissions beyond the owner
 *     mask from group class aces or from everyone@.
 *
 *   - Make sure that the group class is not granted any permissions from
 *     everyone@.
 *
 * The algorithm is exact except for richacls which cannot be represented as an
 * acl alone: for example, given this acl:
 *
 *    group@:rw::allow
 *
 * when file masks corresponding to mode 0600 are applied, the owner would only
 * get rw access if he is a member of the owning group.  This algorithm would
 * produce an empty acl in this case.  We fix this case by modifying
 * richacl_permission() so that the group mask is always applied to group class
 * aces.  With this fix, the owner would not have any access (beyond the
 * implicit permissions always granted to owners).
 *
 * NOTE: Depending on the acl and file masks, this algorithm can increase the
 * number of aces by almost a factor of three in the worst case. This may make
 * the acl too large for some purposes.
 */
int
richacl_apply_masks(struct richacl **acl)
{
	int retval = 0;

	if ((*acl)->a_flags & ACL4_MASKED) {
		struct richacl_alloc x = {
			.acl = *acl,
			.count = (*acl)->a_count,
		};

		if (richacl_move_everyone_aces_down(&x) ||
		    richacl_propagate_everyone(&x) ||
		    __richacl_apply_masks(&x) ||
		    richacl_isolate_owner_class(&x) ||
		    richacl_isolate_group_class(&x))
			retval = -ENOMEM;

		x.acl->a_flags &= ~ACL4_MASKED;
		*acl = x.acl;
	}

	return retval;
}
EXPORT_SYMBOL_GPL(richacl_apply_masks);

/**
 * richacl_from_mode  -  create an acl which corresponds to @mode
 * @mode:	file mode including the file type
 */
struct richacl *
richacl_from_mode(mode_t mode)
{
	struct richacl *acl;
	struct richace *ace;

	acl = richacl_alloc(1);
	if (!acl)
		return NULL;
	acl->a_flags = ACL4_MASKED;
	acl->a_owner_mask = richacl_mode_to_mask(mode >> 6) |
			    ACE4_POSIX_OWNER_ALLOWED;
	acl->a_group_mask = richacl_mode_to_mask(mode >> 3);
	acl->a_other_mask = richacl_mode_to_mask(mode);

	ace = acl->a_entries;
	ace->e_type  = ACE4_ACCESS_ALLOWED_ACE_TYPE;
	ace->e_flags = ACE4_SPECIAL_WHO;
	ace->e_mask = ACE4_POSIX_ALWAYS_ALLOWED |
		      ACE4_POSIX_MODE_ALL |
		      ACE4_POSIX_OWNER_ALLOWED;
	/* ACE4_DELETE_CHILD is meaningless for non-directories. */
	if (!S_ISDIR(mode))
		ace->e_mask &= ~ACE4_DELETE_CHILD;
	ace->e_id = ACE_EVERYONE_ID;
	ace->e_flags |= ACE4_SPECIAL_WHO; 

	return acl;

}
EXPORT_SYMBOL_GPL(richacl_from_mode);

/**
 * richacl_append_entry  -  insert an entry in an acl towards the end
 * @x:	acl and number of allocated entries
 *
 * Insert a new entry in @x->acl at end position and zero-initialize
 * it.  This may require reallocating @x->acl.
 */
static struct richace *richacl_append_entry(struct richacl_alloc *x)
{
	struct richace *ace;
	int n;

	if (x->count != x->acl->a_count)
		n = x->acl->a_count++;
	else {
		struct richacl *acl2;

		n = x->acl->a_count;
		acl2 = richacl_alloc(n + 1);
		if (!acl2)
			return ERR_PTR(-ENOMEM);;
		acl2->a_flags = x->acl->a_flags;
		acl2->a_owner_mask = x->acl->a_owner_mask;
		acl2->a_group_mask = x->acl->a_group_mask;
		acl2->a_other_mask = x->acl->a_other_mask;
		memcpy(acl2->a_entries, x->acl->a_entries,
		       n * sizeof(struct richace));
		if (x->acl) {
			kfree(x->acl);
			x->acl = NULL;
		}
		x->acl = acl2;
		x->count = n + 1;
	}
	ace = x->acl->a_entries + n;
	memset(ace, 0, sizeof(struct richace));
	return ace;
}

static int posix_to_richacl(struct posix_acl *pacl, int type,
			mode_t mode, struct richacl_alloc *x)
{
	int eflags;
	struct richace *ace;
	struct posix_acl_entry *pa, *pe;

	if (type == ACL_TYPE_DEFAULT)
		eflags =  ACE4_FILE_INHERIT_ACE |
			ACE4_DIRECTORY_INHERIT_ACE | ACE4_INHERIT_ONLY_ACE;
	else
		eflags = 0;

	BUG_ON(pacl->a_count < 3);
	FOREACH_ACL_ENTRY(pa, pacl, pe) {

		if (pa->e_tag == ACL_MASK)
			/*
			 * We can ignore ACL_MASK values. We derive the
			 * respective values from the inode mode values
			 */
			continue;

		/* get a slot for new ace */
		ace = richacl_append_entry(x);
		if (IS_ERR(ace))
			return PTR_ERR(ace);

		/* Add allow ACEs for each POSIX ACL entry */
		ace->e_type = ACE4_ACCESS_ALLOWED_ACE_TYPE;
		ace->e_flags = eflags;
		ace->e_mask = richacl_mode_to_mask(pa->e_perm);

		switch (pa->e_tag) {
		case ACL_USER_OBJ:
		{
			ace->e_flags |= ACE4_SPECIAL_WHO;
			ace->e_id = ACE_OWNER_ID;
			ace->e_mask |= (ACE4_POSIX_OWNER_ALLOWED 
					| ACE4_POSIX_ALWAYS_ALLOWED);
			break;
		}
		case ACL_USER:
		{
			ace->e_id = pa->e_id;
			ace->e_mask |= ACE4_POSIX_ALWAYS_ALLOWED;
			break;
		}
		case ACL_GROUP_OBJ:
		{
			ace->e_flags |= ACE4_SPECIAL_WHO;
			ace->e_id = ACE_GROUP_ID;
			ace->e_mask |= ACE4_POSIX_ALWAYS_ALLOWED;
			break;
		}
		case ACL_GROUP:
		{
			ace->e_flags |= ACE4_IDENTIFIER_GROUP;
			ace->e_id = pa->e_id;
			ace->e_mask |= ACE4_POSIX_ALWAYS_ALLOWED;
			break;
		}
		case ACL_OTHER:
		{
			ace->e_flags |= ACE4_SPECIAL_WHO;
			ace->e_id = ACE_EVERYONE_ID;
			ace->e_mask |= ACE4_POSIX_ALWAYS_ALLOWED;
			break;
		}
		} /* switch (pa->e_tag) */
	}
	/* set acl mask values */
	x->acl->a_owner_mask = richacl_mode_to_mask(mode >> 6) |
				ACE4_POSIX_ALWAYS_ALLOWED | ACE4_POSIX_OWNER_ALLOWED;
	x->acl->a_group_mask = richacl_mode_to_mask(mode >> 3) |
				ACE4_POSIX_ALWAYS_ALLOWED;
	x->acl->a_other_mask = richacl_mode_to_mask(mode) | 
				ACE4_POSIX_ALWAYS_ALLOWED;
	/*
	 * Mark that the acl as mapped from posix
	 * This gives user space the chance to verify
	 * whether the mapping was correct
	 */
	x->acl->a_flags = ACL4_POSIX_MAPPED;
	return 0;
}

struct richacl *map_posix_to_richacl(struct inode *inode,
				struct posix_acl *pacl,
				struct posix_acl *dpacl)
{
	int retval;
	int size = 0;
	struct richacl *acl = NULL;
	struct richacl_alloc x;
	struct richace *ace, *ace1, *ace2;
	int num_discarded_aces = 0;


	if (pacl) {
		if (posix_acl_valid(pacl) < 0)
			return ERR_PTR(-EINVAL);
		size += pacl->a_count;
	}

	if (dpacl) {
		if (posix_acl_valid(dpacl) < 0)
			return ERR_PTR(-EINVAL);
		size += dpacl->a_count;
	}

	/* Allocate the worst case one deny, allow pair each */
	acl = richacl_alloc(size);
	if (acl == NULL)
		return ERR_PTR(-ENOMEM);

	x.acl = acl;
	x.count = acl->a_count;
	acl->a_count = 0;

	if (pacl) {
		retval = posix_to_richacl(pacl, ACL_TYPE_ACCESS,
					  inode->i_mode, &x);
		if (retval)
			goto err_out;

 
		if (__richacl_apply_masks(&x) ||
			richacl_isolate_owner_class(&x) ||
			richacl_isolate_group_class(&x))
			retval = -ENOMEM;

		if (retval)
			goto err_out;
	}

	if (dpacl) {
		retval = posix_to_richacl(dpacl, ACL_TYPE_DEFAULT,
					  inode->i_mode, &x);
		if (retval)
			goto err_out;
	}

	/*
	 * FIXME!! Remove duplicate entries and clear in the
	 * ACE4_INHERIT_ONLY_ACE flag
	 */
	if (pacl && dpacl) {
		/* Merge the duplicate aces
 		 * for example:
 		 * Before merging 
 		 *         user06:r--x--a-R-c--S:------:allow 
 		 *         user06:r--x--a-R-c--S:fd-i--:allow 
 		 * After merging
 		 *         user06:r--x--a-R-c--S:fd----:allow
 		 */
		richacl_for_each_entry(ace1, x.acl){
			richacl_for_each_entry(ace2, x.acl){
				if (richace_is_same_identifier(ace1, ace2) &&
					(ace1->e_type == ace2->e_type) &&
					(richace_is_inherit_only(ace1) != richace_is_inherit_only(ace2)) &&
					(richace_is_inherited(ace1) == richace_is_inherited(ace2)) &&
					(ace1->e_mask == ace2->e_mask)) {
					ace1->e_flags |= ace2->e_flags;
					ace1->e_flags &= ~ACE4_INHERIT_ONLY_ACE;
					ace2->e_type = ACE4_DISCARDED_ACE_TYPE;
					num_discarded_aces++;
				}
			}
		}
		/* Remove the ACE4_DISCARDED_ACE_TYPE aces*/
		while (num_discarded_aces > 0) {
			richacl_for_each_entry(ace, x.acl) {
				if (ace->e_type == ACE4_DISCARDED_ACE_TYPE) {
					richacl_delete_entry(&x, &ace);
					num_discarded_aces--;
					break;
				}
			}
		}
	}

	return x.acl;
err_out:
	if (x.acl) {
		kfree(x.acl);
		x.acl = NULL;
	}
	return ERR_PTR(retval);
}
EXPORT_SYMBOL(map_posix_to_richacl);

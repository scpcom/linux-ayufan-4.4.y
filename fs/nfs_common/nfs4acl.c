/*
 * Copyright IBM Corporation, 2010
 * Author Aneesh Kumar K.V <aneesh.kumar@linux.vnet.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/nfs4_acl.h>
#include <linux/richacl.h>

static struct {
	char *string;
	int   stringlen;
	int type;
} s2t_map[] = {
	{
		.string    = "OWNER@",
		.stringlen = sizeof("OWNER@") - 1,
		.type      = NFS4_ACL_WHO_OWNER,
	},
	{
		.string    = "GROUP@",
		.stringlen = sizeof("GROUP@") - 1,
		.type      = NFS4_ACL_WHO_GROUP,
	},
	{
		.string    = "EVERYONE@",
		.stringlen = sizeof("EVERYONE@") - 1,
		.type      = NFS4_ACL_WHO_EVERYONE,
	},
};

int
nfs4_acl_get_whotype(char *p, u32 len)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(s2t_map); i++) {
		if (s2t_map[i].stringlen == len &&
				0 == memcmp(s2t_map[i].string, p, len))
			return s2t_map[i].type;
	}
	return NFS4_ACL_WHO_NAMED;
}
EXPORT_SYMBOL(nfs4_acl_get_whotype);

int
nfs4_acl_write_who(int who, char *p)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(s2t_map); i++) {
		if (s2t_map[i].type == who) {
			memcpy(p, s2t_map[i].string, s2t_map[i].stringlen);
			return s2t_map[i].stringlen;
		}
	}
	BUG();
	return -1;
}
EXPORT_SYMBOL(nfs4_acl_write_who);

#ifdef CONFIG_FS_RICHACL
/*
 * @racl should have file mask applied using
 * richacl_apply_masks
 */
struct nfs4_acl *
nfs4_acl_richacl_to_nfsv4(struct richacl *racl)
{
	struct nfs4_acl *acl;
	struct richace *race;
	struct nfs4_ace *ace;

	acl = nfs4_acl_new(racl->a_count);
	if (acl == NULL)
		return ERR_PTR(-ENOMEM);

	ace = acl->aces;
	richacl_for_each_entry(race, racl) {
		ace->type = race->e_type;
		ace->access_mask = race->e_mask;
		/*
		 * FIXME!! when we support dacl remove the
		 * clearing of ACE4_INHERITED_ACE
		 */
		ace->flag = race->e_flags &
			    ~(ACE4_SPECIAL_WHO | ACE4_INHERITED_ACE);
		if (richace_is_owner(race))
			ace->whotype = NFS4_ACL_WHO_OWNER;
		else if (richace_is_group(race))
			ace->whotype = NFS4_ACL_WHO_GROUP;
		else if (richace_is_everyone(race))
			ace->whotype = NFS4_ACL_WHO_EVERYONE;
		else {
			ace->whotype = NFS4_ACL_WHO_NAMED;
			if(race->e_flags & ACE4_IDENTIFIER_GROUP){
				ace->who = race->e_id;
				ace->flag |= NFS4_ACE_IDENTIFIER_GROUP;
			}
			else{
				ace->whotype = NFS4_ACL_WHO_NAMED;
				ace->who = race->e_id;
			}

		}
		ace++;
		acl->naces++;
	}
	return acl;
}

struct richacl *
nfs4_acl_nfsv4_to_richacl(struct nfs4_acl *acl)
{
	struct richacl *racl;
	struct richace *race;
	struct nfs4_ace *ace;

	racl = richacl_alloc(acl->naces);
	if (racl == NULL)
		return ERR_PTR(-ENOMEM);
	race = racl->a_entries;
	for (ace = acl->aces; ace < acl->aces + acl->naces; ace++) {
		race->e_type  = ace->type;
		race->e_flags = ace->flag;
		race->e_mask  = ace->access_mask;
		switch (ace->whotype) {
		case NFS4_ACL_WHO_OWNER:
			richace_set_who(race, richace_owner_who);
			break;
		case NFS4_ACL_WHO_GROUP:
			richace_set_who(race, richace_group_who);
			break;
		case NFS4_ACL_WHO_EVERYONE:
			richace_set_who(race, richace_everyone_who);
			break;
		case NFS4_ACL_WHO_NAMED:
			if(ace->flag & NFS4_ACE_IDENTIFIER_GROUP) {
				race->e_flags |= ACE4_IDENTIFIER_GROUP;
				race->e_id = ace->who;
			} else {
				race->e_id = ace->who;
			}
			break;
		default:
			richacl_put(racl);
			return ERR_PTR(-EINVAL);
		}
		race++;
	}
	/*
	 * NFSv4 acl don't have file mask.
	 * Derive max mask out of ACE values
	 */
	richacl_compute_max_masks(racl);
	return racl;
}
#else
struct nfs4_acl *
nfs4_acl_richacl_to_nfsv4(struct richacl *racl)
{
	return ERR_PTR(-EINVAL);
}

struct richacl *
nfs4_acl_nfsv4_to_richacl(struct nfs4_acl *acl)
{
	return ERR_PTR(-EINVAL);
}
#endif
EXPORT_SYMBOL(nfs4_acl_richacl_to_nfsv4);
EXPORT_SYMBOL(nfs4_acl_nfsv4_to_richacl);
MODULE_LICENSE("GPL");

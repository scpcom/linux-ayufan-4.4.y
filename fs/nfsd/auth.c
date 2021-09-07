#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
/*
 * linux/fs/nfsd/auth.c
 *
 * Copyright (C) 1995, 1996 Olaf Kirch <okir@monad.swb.de>
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/sunrpc/svc.h>
#include <linux/sunrpc/svcauth.h>
#include <linux/nfsd/nfsd.h>
#include <linux/nfsd/export.h>
#include "auth.h"

int nfsexp_flags(struct svc_rqst *rqstp, struct svc_export *exp)
{
	struct exp_flavor_info *f;
	struct exp_flavor_info *end = exp->ex_flavors + exp->ex_nflavors;

	for (f = exp->ex_flavors; f < end; f++) {
		if (f->pseudoflavor == rqstp->rq_flavor)
			return f->flags;
	}
	return exp->ex_flags;

}

int nfsd_setuser(struct svc_rqst *rqstp, struct svc_export *exp)
{
	struct group_info *rqgi;
	struct group_info *gi;
	struct cred *new;
	int i;
	int flags = nfsexp_flags(rqstp, exp);
	int ret;

	validate_process_creds();

	/* discard any old override before preparing the new set */
	revert_creds(get_cred(current->real_cred));
	new = prepare_creds();
	if (!new)
		return -ENOMEM;

	new->fsuid = rqstp->rq_cred.cr_uid;
	new->fsgid = rqstp->rq_cred.cr_gid;

	rqgi = rqstp->rq_cred.cr_group_info;

	if (flags & NFSEXP_ALLSQUASH) {
		new->fsuid = exp->ex_anon_uid;
		new->fsgid = exp->ex_anon_gid;
#ifdef MY_ABC_HERE
		/*
		 * When squash root/all to admin, the ex_anon_uid and ex_anon_gid are 1024/100 (admin/users).
		 * However, the rw permission of shared folder is only for administrators group. So the
		 * administrators group id (101) should be added to the cred for the share permission.
		 * Since there is no easy way in kernel to know which groups a user belongs to, here we simply
		 * assume that the admin is always in administrators group.
		 *
		 * Note: directly squash to 1024/101 may cause the new created file has owner group
		 * "administrators". We don't want this behavior.
		 */
		if (exp->ex_anon_uid == (uid_t) 1024) {
			gi = groups_alloc(1);
			if (!gi)
				goto oom;
			GROUP_AT(gi, 0) = (gid_t) 101;
		} else {
			gi = groups_alloc(0);
			if (!gi)
				goto oom;
		}
#else
		gi = groups_alloc(0);
		if (!gi)
			goto oom;
#endif /* MY_ABC_HERE */
	} else if (flags & NFSEXP_ROOTSQUASH) {
		if (!new->fsuid)
			new->fsuid = exp->ex_anon_uid;
		if (!new->fsgid)
			new->fsgid = exp->ex_anon_gid;

#ifdef MY_ABC_HERE
		if (exp->ex_anon_uid == (uid_t) 1024) {
			gi = groups_alloc(rqgi->ngroups+1);
			if (!gi)
				goto oom;
			GROUP_AT(gi, rqgi->ngroups) = (gid_t) 101;
		} else {
			gi = groups_alloc(rqgi->ngroups);
			if (!gi)
				goto oom;
		}
#else
		gi = groups_alloc(rqgi->ngroups);
		if (!gi)
			goto oom;
#endif /* MY_ABC_HERE */

		for (i = 0; i < rqgi->ngroups; i++) {
			if (!GROUP_AT(rqgi, i))
				GROUP_AT(gi, i) = exp->ex_anon_gid;
			else
				GROUP_AT(gi, i) = GROUP_AT(rqgi, i);
		}
	} else {
		gi = get_group_info(rqgi);
	}

	if (new->fsuid == (uid_t) -1)
		new->fsuid = exp->ex_anon_uid;
	if (new->fsgid == (gid_t) -1)
		new->fsgid = exp->ex_anon_gid;

	ret = set_groups(new, gi);
	put_group_info(gi);
	if (ret < 0)
		goto error;

	if (new->fsuid)
		new->cap_effective = cap_drop_nfsd_set(new->cap_effective);
	else
		new->cap_effective = cap_raise_nfsd_set(new->cap_effective,
							new->cap_permitted);
	validate_process_creds();
	put_cred(override_creds(new));
	put_cred(new);
	validate_process_creds();
	return 0;

oom:
	ret = -ENOMEM;
error:
	abort_creds(new);
	return ret;
}

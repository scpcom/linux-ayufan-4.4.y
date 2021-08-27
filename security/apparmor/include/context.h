#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __AA_CONTEXT_H
#define __AA_CONTEXT_H

#include <linux/cred.h>
#include <linux/slab.h>
#include <linux/sched.h>

#include "policy.h"

#define cred_cxt(X) (X)->security
#define current_cxt() cred_cxt(current_cred())

struct aa_file_cxt {
	u16 allow;
};

static inline struct aa_file_cxt *aa_alloc_file_context(gfp_t gfp)
{
	return kzalloc(sizeof(struct aa_file_cxt), gfp);
}

static inline void aa_free_file_context(struct aa_file_cxt *cxt)
{
	if (cxt)
		kzfree(cxt);
}

struct aa_task_cxt {
	struct aa_profile *profile;
	struct aa_profile *onexec;
	struct aa_profile *previous;
	u64 token;
};

struct aa_task_cxt *aa_alloc_task_context(gfp_t flags);
void aa_free_task_context(struct aa_task_cxt *cxt);
void aa_dup_task_context(struct aa_task_cxt *new,
			 const struct aa_task_cxt *old);
int aa_replace_current_profile(struct aa_profile *profile);
int aa_set_current_onexec(struct aa_profile *profile);
int aa_set_current_hat(struct aa_profile *profile, u64 token);
int aa_restore_previous_profile(u64 cookie);
struct aa_profile *aa_get_task_profile(struct task_struct *task);

static inline struct aa_profile *aa_cred_profile(const struct cred *cred)
{
	struct aa_task_cxt *cxt = cred_cxt(cred);
	BUG_ON(!cxt || !cxt->profile);
	return cxt->profile;
}

#ifdef MY_ABC_HERE
 
static inline struct aa_profile *aa_get_newest_cred_profile(const struct cred *cred)
{
	return aa_get_newest_profile(aa_cred_profile(cred));
}
#endif

static inline struct aa_profile *__aa_task_profile(struct task_struct *task)
{
	return aa_cred_profile(__task_cred(task));
}

static inline bool __aa_task_is_confined(struct task_struct *task)
{
	return !unconfined(__aa_task_profile(task));
}

static inline struct aa_profile *__aa_current_profile(void)
{
	return aa_cred_profile(current_cred());
}

#ifdef MY_ABC_HERE
 
static inline struct aa_profile *__aa_get_current_profile(void)
{
	struct aa_profile *p = __aa_current_profile();

	if (PROFILE_INVALID(p))
		p = aa_get_newest_profile(p);
	return p;
}

static inline void __aa_put_current_profile(struct aa_profile *profile)
{
	if (profile != __aa_current_profile())
		aa_put_profile(profile);
}
#endif  

static inline struct aa_profile *aa_current_profile(void)
{
	const struct aa_task_cxt *cxt = current_cxt();
	struct aa_profile *profile;
	BUG_ON(!cxt || !cxt->profile);

	if (PROFILE_INVALID(cxt->profile)) {
		profile = aa_get_newest_profile(cxt->profile);
		aa_replace_current_profile(profile);
		aa_put_profile(profile);
		cxt = current_cxt();
	}

	return cxt->profile;
}

static inline void aa_clear_task_cxt_trans(struct aa_task_cxt *cxt)
{
	aa_put_profile(cxt->previous);
	aa_put_profile(cxt->onexec);
	cxt->previous = NULL;
	cxt->onexec = NULL;
	cxt->token = 0;
}

#endif  

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __AA_POLICY_H
#define __AA_POLICY_H

#include <linux/capability.h>
#include <linux/cred.h>
#include <linux/kref.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/socket.h>

#include "apparmor.h"
#include "audit.h"
#include "capability.h"
#include "domain.h"
#include "file.h"
#ifdef MY_ABC_HERE
#include "net.h"
#endif  
#include "resource.h"

extern const char *const aa_profile_mode_names[];
#define APPARMOR_MODE_NAMES_MAX_INDEX 4

#define PROFILE_MODE(_profile, _mode)		\
	((aa_g_profile_mode == (_mode)) ||	\
	 ((_profile)->mode == (_mode)))

#define COMPLAIN_MODE(_profile)	PROFILE_MODE((_profile), APPARMOR_COMPLAIN)

#define KILL_MODE(_profile) PROFILE_MODE((_profile), APPARMOR_KILL)

#define PROFILE_IS_HAT(_profile) ((_profile)->flags & PFLAG_HAT)

#define PROFILE_INVALID(_profile) ((_profile)->flags & PFLAG_INVALID)

#define on_list_rcu(X) (!list_empty(X) && (X)->prev != LIST_POISON2)

enum profile_mode {
	APPARMOR_ENFORCE,	 
	APPARMOR_COMPLAIN,	 
	APPARMOR_KILL,		 
	APPARMOR_UNCONFINED,	 
};

enum profile_flags {
	PFLAG_HAT = 1,			 
	PFLAG_NULL = 4,			 
	PFLAG_IX_ON_NAME_ERROR = 8,	 
	PFLAG_IMMUTABLE = 0x10,		 
	PFLAG_USER_DEFINED = 0x20,	 
	PFLAG_NO_LIST_REF = 0x40,	 
	PFLAG_OLD_NULL_TRANS = 0x100,	 
	PFLAG_INVALID = 0x200,		 
	PFLAG_NS_COUNT = 0x400,		 

	PFLAG_MEDIATE_DELETED = 0x10000,  
};

struct aa_profile;

struct aa_policy {
	char *name;
	char *hname;
	struct list_head list;
	struct list_head profiles;
};

struct aa_ns_acct {
	int max_size;
	int max_count;
	int size;
	int count;
};

struct aa_namespace {
	struct aa_policy base;
	struct aa_namespace *parent;
	struct mutex lock;
	struct aa_ns_acct acct;
	struct aa_profile *unconfined;
	struct list_head sub_ns;
	atomic_t uniq_null;
	long uniq_id;

	struct dentry *dents[AAFS_NS_SIZEOF];
};

struct aa_policydb {
	 
	struct aa_dfa *dfa;
	unsigned int start[AA_CLASS_LAST + 1];

};

struct aa_replacedby {
	struct kref count;
	struct aa_profile __rcu *profile;
};

struct aa_profile {
	struct aa_policy base;
	struct kref count;
	struct rcu_head rcu;
	struct aa_profile __rcu *parent;

	struct aa_namespace *ns;
	struct aa_replacedby *replacedby;
	const char *rename;

	const char *attach;
	struct aa_dfa *xmatch;
	int xmatch_len;
	enum audit_mode audit;
	long mode;
	long flags;
	u32 path_flags;
	int size;

	struct aa_policydb policy;
	struct aa_file_rules file;
	struct aa_caps caps;
#ifdef MY_ABC_HERE
	struct aa_net net;
#endif  
	struct aa_rlimit rlimits;

	char *dirname;
	struct dentry *dents[AAFS_PROF_SIZEOF];
};

extern struct aa_namespace *root_ns;
extern enum profile_mode aa_g_profile_mode;

void aa_add_profile(struct aa_policy *common, struct aa_profile *profile);

bool aa_ns_visible(struct aa_namespace *curr, struct aa_namespace *view);
const char *aa_ns_name(struct aa_namespace *parent, struct aa_namespace *child);
int aa_alloc_root_ns(void);
void aa_free_root_ns(void);
void aa_free_namespace_kref(struct kref *kref);

struct aa_namespace *aa_find_namespace(struct aa_namespace *root,
				       const char *name);

void aa_free_replacedby_kref(struct kref *kref);
struct aa_profile *aa_alloc_profile(const char *name);
struct aa_profile *aa_new_null_profile(struct aa_profile *parent, int hat);
void aa_free_profile(struct aa_profile *profile);
void aa_free_profile_kref(struct kref *kref);
struct aa_profile *aa_find_child(struct aa_profile *parent, const char *name);
struct aa_profile *aa_lookup_profile(struct aa_namespace *ns, const char *name);
struct aa_profile *aa_match_profile(struct aa_namespace *ns, const char *name);

ssize_t aa_replace_profiles(void *udata, size_t size, bool noreplace);
ssize_t aa_remove_profiles(char *name, size_t size);

#define PROF_ADD 1
#define PROF_REPLACE 0

#define unconfined(X) ((X)->mode == APPARMOR_UNCONFINED)

static inline struct aa_profile *aa_deref_parent(struct aa_profile *p)
{
	return rcu_dereference_protected(p->parent,
					 mutex_is_locked(&p->ns->lock));
}

static inline struct aa_profile *aa_get_profile(struct aa_profile *p)
{
	if (p)
		kref_get(&(p->count));

	return p;
}

static inline struct aa_profile *aa_get_profile_not0(struct aa_profile *p)
{
	if (p && kref_get_not0(&p->count))
		return p;

	return NULL;
}

static inline struct aa_profile *aa_get_profile_rcu(struct aa_profile __rcu **p)
{
	struct aa_profile *c;

	rcu_read_lock();
	do {
		c = rcu_dereference(*p);
	} while (c && !kref_get_not0(&c->count));
	rcu_read_unlock();

	return c;
}

static inline struct aa_profile *aa_get_newest_profile(struct aa_profile *p)
{
	if (!p)
		return NULL;

	if (PROFILE_INVALID(p))
		return aa_get_profile_rcu(&p->replacedby->profile);

	return aa_get_profile(p);
}

static inline void aa_put_profile(struct aa_profile *p)
{
	if (p)
		kref_put(&p->count, aa_free_profile_kref);
}

static inline struct aa_replacedby *aa_get_replacedby(struct aa_replacedby *p)
{
	if (p)
		kref_get(&(p->count));

	return p;
}

static inline void aa_put_replacedby(struct aa_replacedby *p)
{
	if (p)
		kref_put(&p->count, aa_free_replacedby_kref);
}

static inline void __aa_update_replacedby(struct aa_profile *orig,
					  struct aa_profile *new)
{
	struct aa_profile *tmp = rcu_dereference(orig->replacedby->profile);
	rcu_assign_pointer(orig->replacedby->profile, aa_get_profile(new));
	orig->flags |= PFLAG_INVALID;
	aa_put_profile(tmp);
}

static inline struct aa_namespace *aa_get_namespace(struct aa_namespace *ns)
{
	if (ns)
		aa_get_profile(ns->unconfined);

	return ns;
}

static inline void aa_put_namespace(struct aa_namespace *ns)
{
	if (ns)
		aa_put_profile(ns->unconfined);
}

static inline int AUDIT_MODE(struct aa_profile *profile)
{
	if (aa_g_audit != AUDIT_NORMAL)
		return aa_g_audit;

	return profile->audit;
}

bool aa_may_manage_policy(int op);

#endif  

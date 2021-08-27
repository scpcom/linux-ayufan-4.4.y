#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __AA_NET_H
#define __AA_NET_H

#include <net/sock.h>

#ifdef MY_ABC_HERE
#include "apparmorfs.h"
#endif

struct aa_net {
	u16 allow[AF_MAX];
	u16 audit[AF_MAX];
	u16 quiet[AF_MAX];
};

#ifdef MY_ABC_HERE
extern struct aa_fs_entry aa_fs_entry_network[];
#endif

extern int aa_net_perm(int op, struct aa_profile *profile, u16 family,
		       int type, int protocol, struct sock *sk);
extern int aa_revalidate_sk(int op, struct sock *sk);

static inline void aa_free_net_rules(struct aa_net *new)
{
	 
}

#endif  

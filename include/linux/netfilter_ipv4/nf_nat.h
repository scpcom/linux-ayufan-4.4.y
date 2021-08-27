#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_NF_NAT_H
#define _LINUX_NF_NAT_H

#include <linux/types.h>

#define IP_NAT_RANGE_MAP_IPS 1
#define IP_NAT_RANGE_PROTO_SPECIFIED 2
#define IP_NAT_RANGE_PROTO_RANDOM 4
#define IP_NAT_RANGE_PERSISTENT 8
#if defined(MY_ABC_HERE)
#define IP_NAT_RANGE_4RD_NAPT 16
#endif

union nf_conntrack_man_proto {
	 
	__be16 all;

	struct {
		__be16 port;
	} tcp;
	struct {
		__be16 port;
	} udp;
	struct {
		__be16 id;
	} icmp;
	struct {
		__be16 port;
	} dccp;
	struct {
		__be16 port;
	} sctp;
	struct {
		__be16 key;	 
	} gre;
};

struct nf_nat_range {
	 
	unsigned int flags;

	__be32 min_ip, max_ip;

	union nf_conntrack_man_proto min, max;
};

struct nf_nat_multi_range_compat {
	unsigned int rangesize;  

	struct nf_nat_range range[1];
};

#define nf_nat_multi_range nf_nat_multi_range_compat

#endif

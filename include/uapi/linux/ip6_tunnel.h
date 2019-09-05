#ifndef _IP6_TUNNEL_H
#define _IP6_TUNNEL_H

#include <linux/types.h>

#define IPV6_TLV_TNL_ENCAP_LIMIT 4
#define IPV6_DEFAULT_TNL_ENCAP_LIMIT 4

/* don't add encapsulation limit if one isn't present in inner packet */
#define IP6_TNL_F_IGN_ENCAP_LIMIT 0x1
/* copy the traffic class field from the inner packet */
#define IP6_TNL_F_USE_ORIG_TCLASS 0x2
/* copy the flowlabel from the inner packet */
#define IP6_TNL_F_USE_ORIG_FLOWLABEL 0x4
/* being used for Mobile IPv6 */
#define IP6_TNL_F_MIP6_DEV 0x8
/* copy DSCP from the outer packet */
#define IP6_TNL_F_RCV_DSCP_COPY 0x10
/* copy fwmark from inner packet */
#define IP6_TNL_F_USE_ORIG_FWMARK 0x20

struct ip6_tnl_parm {
	char name[IFNAMSIZ];	/* name of tunnel device */
	int link;		/* ifindex of underlying L2 interface */
	__u8 proto;		/* tunnel protocol */
	__u8 encap_limit;	/* encapsulation limit for tunnel */
	__u8 hop_limit;		/* hop limit for tunnel */
	__be32 flowinfo;	/* traffic class and flowlabel for tunnel */
	__u32 flags;		/* tunnel flags */
	struct in6_addr laddr;	/* local tunnel end-point address */
	struct in6_addr raddr;	/* remote tunnel end-point address */
};

struct ip6_tnl_parm2 {
	char name[IFNAMSIZ];	/* name of tunnel device */
	int link;		/* ifindex of underlying L2 interface */
	__u8 proto;		/* tunnel protocol */
	__u8 encap_limit;	/* encapsulation limit for tunnel */
	__u8 hop_limit;		/* hop limit for tunnel */
	__be32 flowinfo;	/* traffic class and flowlabel for tunnel */
	__u32 flags;		/* tunnel flags */
	struct in6_addr laddr;	/* local tunnel end-point address */
	struct in6_addr raddr;	/* remote tunnel end-point address */

	__be16			i_flags;
	__be16			o_flags;
	__be32			i_key;
	__be32			o_key;
};

 
struct ip6_4rd_map_msg {
       __u32 reset;
       __u32 ifindex;
       __be32 prefix;
       __u16 prefixlen;
       struct in6_addr relay_prefix;
       struct in6_addr relay_suffix;
       __u16 relay_prefixlen;
       __u16 relay_suffixlen;
       __u16 psid_offsetlen;
       __u16 eabit_len;
       __u16 entry_num;
};

struct ip6_tnl_4rd_map_rule {
       __be32 prefix;
       __u16 prefixlen;
       struct in6_addr relay_prefix;
       struct in6_addr relay_suffix;
       __u16 relay_prefixlen;
       __u16 relay_suffixlen;
       __u16 psid_offsetlen;
       __u16 eabit_len;
       __u16 entry_num;
       struct list_head mr_list;
};

#ifdef __KERNEL__
struct ip6_tnl_4rd_parm {
       __be32 prefix;
       struct in6_addr relay_prefix;
       struct in6_addr relay_suffix;
       __u16 prefixlen;
       __u16 relay_prefixlen;
       __u16 relay_suffixlen;
       __be32 laddr4;
       __u16 port_set_id;
       __u16 port_set_id_len;
       __u16 psid_offsetlen;
       __u16 eabit_len;

       struct list_head map_list;
       rwlock_t map_lock;
};
#endif

#endif

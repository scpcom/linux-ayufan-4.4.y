#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _TSO_H
#define _TSO_H

#include <net/ip.h>

#if defined(MY_DEF_HERE)
#define TSO_HEADER_SIZE		128

#endif /* MY_DEF_HERE */
struct tso_t {
	int next_frag_idx;
	void *data;
	size_t size;
	u16 ip_id;
	bool ipv6;
	u32 tcp_seq;
};

int tso_count_descs(struct sk_buff *skb);
void tso_build_hdr(struct sk_buff *skb, char *hdr, struct tso_t *tso,
		   int size, bool is_last);
void tso_build_data(struct sk_buff *skb, struct tso_t *tso, int size);
void tso_start(struct sk_buff *skb, struct tso_t *tso);

#endif	/* _TSO_H */

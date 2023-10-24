/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2006 Netfilter Core Team <coreteam@netfilter.org>
 * (C) 2008 Patrick McHardy <kaber@trash.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/random.h>
#include <linux/ip.h>

#include <linux/netfilter.h>
#include <linux/export.h>
#include <net/secure_seq.h>
#include <net/netfilter/nf_nat.h>
#include <net/netfilter/nf_nat_core.h>
#include <net/netfilter/nf_nat_rule.h>
#include <net/netfilter/nf_nat_protocol.h>

bool nf_nat_proto_in_range(const struct nf_conntrack_tuple *tuple,
			   enum nf_nat_manip_type maniptype,
			   const union nf_conntrack_man_proto *min,
			   const union nf_conntrack_man_proto *max)
{
	__be16 port;

	if (maniptype == IP_NAT_MANIP_SRC)
		port = tuple->src.u.all;
	else
		port = tuple->dst.u.all;

	return ntohs(port) >= ntohs(min->all) &&
	       ntohs(port) <= ntohs(max->all);
}
EXPORT_SYMBOL_GPL(nf_nat_proto_in_range);

void nf_nat_proto_unique_tuple(struct nf_conntrack_tuple *tuple,
			       const struct nf_nat_range *range,
			       enum nf_nat_manip_type maniptype,
			       const struct nf_conn *ct,
			       u_int16_t *rover)
{
	unsigned int range_size, min, i;
	__be16 *portptr;
	u_int16_t off;

	if (maniptype == IP_NAT_MANIP_SRC)
		portptr = &tuple->src.u.all;
	else
		portptr = &tuple->dst.u.all;

	/* If no range specified... */
	if (!(range->flags & IP_NAT_RANGE_PROTO_SPECIFIED)) {
		/* If it's dst rewrite, can't change port */
		if (maniptype == IP_NAT_MANIP_DST)
			return;

		if (ntohs(*portptr) < 1024) {
			/* Loose convention: >> 512 is credential passing */
			if (ntohs(*portptr) < 512) {
				min = 1;
				range_size = 511 - min + 1;
			} else {
				min = 600;
				range_size = 1023 - min + 1;
			}
		} else {
			min = 1024;
			range_size = 65535 - 1024 + 1;
		}
	} else {
		min = ntohs(range->min.all);
		range_size = ntohs(range->max.all) - min + 1;
	}

	if (range->flags & IP_NAT_RANGE_PROTO_RANDOM)
		off = secure_ipv4_port_ephemeral(tuple->src.u3.ip, tuple->dst.u3.ip,
						 maniptype == IP_NAT_MANIP_SRC
						 ? tuple->dst.u.all
						 : tuple->src.u.all);
	else
		off = *rover;

       /* 4rd specific range NAPT - start */
       if ((range->flags & IP_NAT_RANGE_4RD_NAPT) && (maniptype == IP_NAT_MANIP_SRC)){
               __be16 fix_port;
               __be16 port_min, port_max;
               u_int16_t port_set_id;
               u_int16_t mbitlen   = 0 ;
               u_int16_t offsetlen = 0 ;
               u_int16_t psidlen   = 0 ;
               u_int16_t range_total_size = 0 ;
               u_int16_t offset_4rd;
               u_int16_t o_state;
               u_int16_t m_state;

               u_int16_t offset_min;
               u_int16_t offset_max;
               u_int16_t offset_cnt;
               u_int16_t mbit_cnt;

               # define MAXBITLEN 16
               port_min = ntohs(range->min.all);
               port_max = ntohs(range->max.all);

               for(i = 0; i < MAXBITLEN; i++){
                       if( ((port_min >> i) & 0x0001) == ((port_max >> i) & 0x0001) ){
                               psidlen++;
                               i++;
                               break;
                       }
                       else{
                               mbitlen++;
                       }
               }

               for( ; i < MAXBITLEN; i++){
                       if( ((port_min >> i) & 0x0001) == ((port_max >> i) & 0x0001) ){
                               psidlen++;
                       }
                       else{
                               break;
                       }
               }

               offsetlen = MAXBITLEN - psidlen - mbitlen;

               if((psidlen == 0) || (mbitlen == 0)){
                       /* ERROR */
                       printk(KERN_INFO "4rd parameter INVALID");
               }

               port_set_id = (port_min & port_max) >> mbitlen;

               if( offsetlen ){
                       if(port_min < 4096){
                               if(offsetlen > 4){


                                       offset_min = 0x1000 >> (psidlen + mbitlen);
                                       offset_max = (1 << offsetlen) - 1;
                                       offset_cnt = offset_max - offset_min + 1;
                                       mbit_cnt = 1 << mbitlen;
                                       range_total_size = offset_cnt * mbit_cnt;
                               }
                               else{
                                       offset_min = 0x0001;
                                       offset_cnt = (1 << offsetlen) - 1;
                                       mbit_cnt = 1 << mbitlen;
                                       range_total_size = offset_cnt * mbit_cnt;
                               }
                       }
                       else{
                               offset_min = 0x0000;
                               offset_cnt = 1 << offsetlen;
                               mbit_cnt = 1 << mbitlen;
                               range_total_size = offset_cnt * mbit_cnt;
                       }
               }
               else{
                       offset_min = 0;
                       range_total_size = ( 1 << mbitlen ) ;
               }

               for (i = 0; ; ++off) {

                       offset_4rd = off % range_total_size ;

                       if ( mbitlen != 0 ){
                               o_state = offset_4rd / ( 1 << mbitlen ) ;
                               m_state = offset_4rd % ( 1 << mbitlen ) ;
                       }else{
                               o_state = offset_4rd / ( 1 << mbitlen ) ;
                               m_state = 0 ;
                       }

                       fix_port = (( offset_min + o_state ) << ( mbitlen + psidlen ));
                       fix_port |= ( port_set_id << mbitlen );
                       fix_port |= m_state;

                       *portptr = htons(fix_port);

                       if ((++i != range_total_size) && nf_nat_used_tuple(tuple, ct))
                               continue;

                       //printk(KERN_INFO "fix_port = %d ",fix_port);
                       return ;
               }
       } /* 4rd specific range NAPT - end */
       else{

	       for (i = 0; ; ++off) {
		       *portptr = htons(min + off % range_size);
		       if (++i != range_size && nf_nat_used_tuple(tuple, ct))
			       continue;
		       if (!(range->flags & IP_NAT_RANGE_PROTO_RANDOM))
			       *rover = off;
		       return;
	       }
       }
	return;
}
EXPORT_SYMBOL_GPL(nf_nat_proto_unique_tuple);

#if defined(CONFIG_NF_CT_NETLINK) || defined(CONFIG_NF_CT_NETLINK_MODULE)
int nf_nat_proto_range_to_nlattr(struct sk_buff *skb,
				 const struct nf_nat_range *range)
{
	NLA_PUT_BE16(skb, CTA_PROTONAT_PORT_MIN, range->min.all);
	NLA_PUT_BE16(skb, CTA_PROTONAT_PORT_MAX, range->max.all);
	return 0;

nla_put_failure:
	return -1;
}
EXPORT_SYMBOL_GPL(nf_nat_proto_nlattr_to_range);

int nf_nat_proto_nlattr_to_range(struct nlattr *tb[],
				 struct nf_nat_range *range)
{
	if (tb[CTA_PROTONAT_PORT_MIN]) {
		range->min.all = nla_get_be16(tb[CTA_PROTONAT_PORT_MIN]);
		range->max.all = range->min.tcp.port;
		range->flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
	}
	if (tb[CTA_PROTONAT_PORT_MAX]) {
		range->max.all = nla_get_be16(tb[CTA_PROTONAT_PORT_MAX]);
		range->flags |= IP_NAT_RANGE_PROTO_SPECIFIED;
	}
	return 0;
}
EXPORT_SYMBOL_GPL(nf_nat_proto_range_to_nlattr);
#endif

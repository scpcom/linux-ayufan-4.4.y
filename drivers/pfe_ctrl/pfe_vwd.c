#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/if_ether.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv6.h>
#include <linux/netfilter_bridge.h>
#include <linux/irqnr.h>
#include <linux/ppp_defs.h>

#include <linux/rculist.h>
#include <../../../net/bridge/br_private.h>


#include "pfe_mod.h"
#include "pfe_vwd.h"

#ifdef WIFI_ENABLE

//#define VWD_DEBUG
#define VWD_DEBUG_STATS

static void pfe_vwd_sysfs_exit(void);
static unsigned int pfe_vwd_nf_route_hook_fn( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *));
static unsigned int pfe_vwd_nf_bridge_hook_fn( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *));
static int pfe_vwd_event_handler(void *data, int event, int qno);

extern unsigned int page_mode;

/* IPV4 route hook , recieve the packet and forward to VWD driver*/
static struct nf_hook_ops vwd_hook = {
	.hook = pfe_vwd_nf_route_hook_fn,
	.pf = PF_INET,
	.hooknum = NF_INET_PRE_ROUTING,
	.priority = NF_IP_PRI_FIRST,
};

/* IPV6 route hook , recieve the packet and forward to VWD driver*/
static struct nf_hook_ops vwd_hook_ipv6 = {
	.hook = pfe_vwd_nf_route_hook_fn,
	.pf = PF_INET6,
	.hooknum = NF_INET_PRE_ROUTING,
	.priority = NF_IP6_PRI_FIRST,
};

/* Bridge hook , recieve the packet and forward to VWD driver*/
static struct nf_hook_ops vwd_hook_bridge = {
	.hook = pfe_vwd_nf_bridge_hook_fn,
	.pf = PF_BRIDGE,
	.hooknum = NF_BR_PRE_ROUTING,
	.priority = NF_BR_PRI_FIRST,
};

struct pfe_vwd_priv_s glbl_pfe_vwd_priv;

#ifdef VWD_DEBUG
static void pfe_vwd_dump_skb( struct sk_buff *skb )
{
	int i;

	for (i = 0; i < skb->len; i++)
	{
		if (!(i % 16))
			printk("\n");

		printk(" %02x", skb->data[i]);
	}
}
#endif

/** pfe_vwd_show_dump_stats
 *
 */
static ssize_t pfe_vwd_show_dump_stats(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t len = 0;
	struct pfe_vwd_priv_s *priv = &pfe->vwd;

	len += sprintf(buf, "\n%s: %s Stopped : %s \nTx pkts: %d\nQ0 Slow-path pkts:%d\nQ1 Slow-path pkts:%d\nTx dropped:%d\nQ0 Rx fast-path packets:%d\n Q1 Rx fast-path packets:%d\nSKB Allocation failures:%d\n\n", __func__,
			priv->name, priv->fast_path_enable ? "no":"yes",
			priv->pkts_transmitted, priv->pkts_slow_forwarded[0],
			priv->pkts_slow_forwarded[1], priv->pkts_tx_dropped, 
			priv->pkts_rx_fast_forwarded[0], priv->pkts_rx_fast_forwarded[1], 
			priv->rx_skb_alloc_fail );

	return len;
}


/** pfe_vwd_show_fast_path_enable
 *
 */
static ssize_t pfe_vwd_show_fast_path_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int idx;

	idx = sprintf(buf, "\n%d\n", priv->fast_path_enable);

	return idx;
}

/** pfe_vwd_set_fast_path_enable
 *
 */
static ssize_t pfe_vwd_set_fast_path_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s  *priv = &pfe->vwd;
        unsigned int fast_path = 0;

        sscanf(buf, "%d", &fast_path);

	printk("%s: Wifi fast path %d\n", __func__, fast_path);

        if (fast_path && !priv->fast_path_enable)
        {
                printk("%s: Wifi fast path enabled \n", __func__);

                priv->fast_path_enable = 1;

        }
        else if (!fast_path && priv->fast_path_enable)
        {
                printk("%s: Wifi fast path disabled \n", __func__);

                priv->fast_path_enable = 0;

        }

	return count;
}

/** pfe_vwd_show_route_hook_enable
 *
 */
static ssize_t pfe_vwd_show_route_hook_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int idx;

	idx = sprintf(buf, "\n%d\n", priv->fast_routing_enable);

	return idx;
}

/** pfe_vwd_set_route_hook_enable
 *
 */
static ssize_t pfe_vwd_set_route_hook_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
        unsigned int user_val = 0;

        sscanf(buf, "%d", &user_val);

        if (user_val && !priv->fast_routing_enable)
        {
                printk("%s: Wifi fast routing enabled \n", __func__);
                priv->fast_routing_enable = 1;

                if (priv->fast_bridging_enable)
                {
                        nf_unregister_hook(&vwd_hook_bridge);                                                                                         priv->fast_bridging_enable = 0;
                }

                nf_register_hook(&vwd_hook);
                nf_register_hook(&vwd_hook_ipv6);


        }
        else if (!user_val && priv->fast_routing_enable)
        {
                printk("%s: Wifi fast routing disabled \n", __func__);
                priv->fast_routing_enable = 0;

                nf_unregister_hook(&vwd_hook);
                nf_unregister_hook(&vwd_hook_ipv6);

        }

	return count;
}

/** pfe_vwd_show_bridge_hook_enable
 *
 */
static int pfe_vwd_show_bridge_hook_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int idx;

	idx = sprintf(buf, "%d", priv->fast_bridging_enable);
	return idx;
}

/** pfe_vwd_set_bridge_hook_enable
 *
 */
static int pfe_vwd_set_bridge_hook_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	unsigned int user_val = 0;

        sscanf(buf, "%d", &user_val);

	if ( user_val && !priv->fast_bridging_enable )
        {
                printk("%s: Wifi fast bridging enabled \n", __func__);
                priv->fast_bridging_enable = 1;

                if(priv->fast_routing_enable)
                {
                        nf_unregister_hook(&vwd_hook);
                        nf_unregister_hook(&vwd_hook_ipv6);
                        priv->fast_routing_enable = 0;
                }

                nf_register_hook(&vwd_hook_bridge);
        }
        else if ( !user_val && priv->fast_bridging_enable )
        {
                printk("%s: Wifi fast bridging disabled \n", __func__);
                priv->fast_bridging_enable = 0;

                nf_unregister_hook(&vwd_hook_bridge);
        }

	return count;
}


/** pfe_vwd_classify_packet
 *
 */
static int pfe_vwd_classify_packet( struct pfe_vwd_priv_s *priv, struct sk_buff *skb,
							int bridge_hook, int route_hook, int *vapid, int *own_mac)
{
	unsigned short type;
	struct vap_desc_s *vap;
	int rc = 1, ii, length;
	unsigned char *data_ptr;
	struct ethhdr *hdr;
#if defined (CONFIG_COMCERTO_VWD_MULTI_MAC)
	struct net_bridge_fdb_entry *dst = NULL;
	struct net_bridge_port *p = NULL;
	const unsigned char *dest = eth_hdr(skb)->h_dest;
#endif
	*own_mac = 0;

	/* Move to packet network header */
	data_ptr = skb_mac_header(skb);
	length = skb->len + (skb->data - data_ptr);

	spin_lock_bh(&priv->vaplock);
	/* Broadcasts and MC are handled by stack */
	if( (eth_hdr(skb)->h_dest[0] & 0x1) ||
			( length <= ETH_HLEN ) )
	{
		rc = 1;
		goto done;
	}

	/* FIXME: This packet is VWD slow path packet, and already seen by VWD */

	if (*(unsigned long *)skb->head == 0xdead)
	{
		//printk(KERN_INFO "%s:This is dead packet....\n", __func__);
		*(unsigned long *)skb->head = 0x0;
		rc = 1;
		goto done;
	}

#ifdef VWD_DEBUG
	printk(KERN_INFO "%s: skb cur len:%d skb orig len:%d\n", __func__, skb->len, length );
#endif

	/* FIXME: We need to check the route table for the route entry. If route
	 *  entry found for the current packet, send the packet to PFE. Otherwise
	 *  REJECT the packet.
	 */
	for ( ii = 0; ii < MAX_VAP_SUPPORT; ii++ )
	{
		vap =   &priv->vaps[ii];
#ifdef VWD_DEBUG
		printk(KERN_INFO "%s:%d: iif:%d skb iif:%d\n", __func__,ii, vap->ifindex, skb->skb_iif );
#endif
		if( ( vap->ifindex ) && ( vap->ifindex == skb->skb_iif) )
		{
			hdr = (struct ethhdr *)data_ptr;
			type = htons(hdr->h_proto);
			data_ptr += ETH_HLEN;
			length -= ETH_HLEN;
			rc = 0;

			*vapid = vap->vapid;

			/* FIXME send only IPV4 and IPV6 packets to PFE */
			//Determain final protocol type
			//FIXME : This multi level parsing is not required for
			//        Bridged packets.
			if( type == ETH_P_8021Q )
			{
				struct vlan_hdr *vhdr = (struct vlan_hdr *)data_ptr;

				data_ptr += VLAN_HLEN;
				length -= VLAN_HLEN;
				type = htons(vhdr->h_vlan_encapsulated_proto);
			}

			if( type == ETH_P_PPP_SES )
			{
				struct pppoe_hdr *phdr = (struct pppoe_hdr *)data_ptr;

				data_ptr += PPPOE_SES_HLEN;
				length -= PPPOE_SES_HLEN;

				if (htons(*(u16 *)(phdr+1)) == PPP_IP)
					type = ETH_P_IP;
				else if (htons(*(u16 *)(phdr+1)) == PPP_IPV6)
					type = ETH_P_IPV6;
			}


			if (bridge_hook)
			{
#if defined (CONFIG_COMCERTO_VWD_MULTI_MAC)
				/* check if destination MAC matches one of the interfaces attached to the bridge */
				if((p = rcu_dereference(skb->dev->br_port)) != NULL)
					dst = __br_fdb_get(p->br, dest);

				if (skb->pkt_type == PACKET_HOST || (dst && dst->is_local))
#else
					if (skb->pkt_type == PACKET_HOST)
#endif
					{
						*own_mac = 1;

						if ((type != ETH_P_IP) && (type != ETH_P_IPV6))
						{
							rc = 1;
							goto done;
						}
					}
					else if (!memcmp(vap->macaddr, eth_hdr(skb)->h_dest, ETH_ALEN))
					{
						//WiFi management packets received with dst address
						//as bssid
						rc = 1;
						goto done;
					}
			}
			else
				*own_mac = 1;

			break;
		}

	}

done:
	spin_unlock_bh(&priv->vaplock);
	return rc;

}

/** pfe_vwd_flush_txQ
 *
 */
static void pfe_vwd_flush_txQ(struct pfe_vwd_priv_s *priv, int txQ_num, int n_desc)
{
	struct sk_buff *skb;
	int count = max(TX_FREE_MAX_COUNT, n_desc);
	unsigned int flags;

	//printk(KERN_INFO "%s\n", __func__);


	while (count && (skb = hif_lib_tx_get_next_complete(&priv->client, txQ_num, &flags, count))) {

		/* FIXME : Invalid data can be skipped in hif_lib itself */
		if (flags & HIF_DATA_VALID) {
			dev_kfree_skb_any(skb);
		}
		count--;
	}


}

/** pfe_vwd_send_packet
 *
 */
static void pfe_vwd_send_packet( struct sk_buff *skb, struct  pfe_vwd_priv_s *priv, int queuenum, int vapid, int own_mac)
{
	u32 ctrl = 0;
	int count;

	//printk(KERN_INFO "%s\n", __func__);

	spin_lock_bh(&priv->vwd_tx_lock[queuenum]);

	if (skb_headroom(skb) < (PFE_PKT_HEADER_SZ + sizeof(unsigned long))) {

		printk(KERN_INFO "%s: copying skb\n", __func__);

		if (pskb_expand_head(skb, (PFE_PKT_HEADER_SZ + sizeof(unsigned long)), 0, GFP_ATOMIC)) {
			kfree_skb(skb);
#ifdef VWD_DEBUG_STATS
		priv->pkts_tx_dropped += 1;
#endif
			goto out;
		}
	}


	/* Send vap_id to PFE */
	if (own_mac)
		ctrl |= ((vapid << HIF_CTRL_VAPID_OFST) | HIF_CTRL_TX_OWN_MAC);
	else
		ctrl |= (vapid << HIF_CTRL_VAPID_OFST);

	if (hif_lib_xmit_pkt(&priv->client, queuenum, skb->data, skb->len, ctrl, skb)) {
		// HIF Lib unable send packet
		printk(KERN_ERR "%s: hif_lib_xmit_pkt() failed\n", __func__);
		kfree_skb(skb);

#ifdef VWD_DEBUG_STATS
		priv->pkts_tx_dropped += 1;
#endif
		goto out;
	}

#ifdef VWD_DEBUG_STATS
		priv->pkts_transmitted += 1;
#endif

	//printk(KERN_INFO "%s: pkt sent successfully skb:%p len:%d\n", __func__, skb, skb->len);

out:
	// Recycle buffers if a socket's send buffer becomes half full or if the HIF client queue starts filling up
	if (((count = (hif_lib_tx_pending(&priv->client, queuenum) - HIF_CL_TX_FLUSH_MARK)) > 0)
		|| (skb && skb->sk && ((sk_wmem_alloc_get(skb->sk) << 1) > skb->sk->sk_sndbuf)))
		pfe_vwd_flush_txQ(priv, queuenum, count);


	spin_unlock_bh(&priv->vwd_tx_lock[queuenum]);

	return;
}

/** vwd_nf_bridge_hook_fn
 *
 */
static unsigned int pfe_vwd_nf_bridge_hook_fn( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int vapid = -1;
	int own_mac = 0;

#ifdef VWD_DEBUG
	printk("%s: protocol : 0x%04x\n", __func__, htons(skb->protocol));
#endif

	if( !priv->fast_path_enable )
		goto done;

	if( !pfe_vwd_classify_packet(priv, skb, 1, 0, &vapid, &own_mac) )
	{
#ifdef VWD_DEBUG
		printk("%s: Accepted\n", __func__);
	//	pfe_vwd_dump_skb( skb );
#endif
		skb_push(skb, ETH_HLEN);
		pfe_vwd_send_packet( skb, priv, 0, vapid, own_mac);
		return NF_STOLEN;
	}

done:

	return NF_ACCEPT;

}

/** vwd_nf_route_hook_fn
 *
 */
static unsigned int pfe_vwd_nf_route_hook_fn( unsigned int hook, struct sk_buff *skb,
		const struct net_device *in, const struct net_device *out,
		int (*okfn)(struct sk_buff *))
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;
	int vapid = -1;
	int own_mac = 0;

#ifdef VWD_DEBUG
	printk("%s: protocol : 0x%04x\n", __func__, htons(skb->protocol));
#endif

	if (!priv->fast_path_enable)
		goto done;

	if (!pfe_vwd_classify_packet(priv, skb, 0, 1, &vapid, &own_mac))
	{
#ifdef VWD_DEBUG
		printk("%s: Accepted\n", __func__);
//		pfe_vwd_dump_skb( skb );
#endif
		skb_push(skb, ETH_HLEN);
		pfe_vwd_send_packet( skb, priv, 0, vapid, own_mac);
		return NF_STOLEN;
	}

done:
	return NF_ACCEPT;

}

/** vwd_stop
 *
 */
void pfe_vwd_stop( struct pfe_vwd_priv_s *priv )
{
	printk (KERN_INFO "%s: %s\n", __func__, priv->name);

}

static DEVICE_ATTR(vwd_debug_stats, 0444, pfe_vwd_show_dump_stats, NULL);
static DEVICE_ATTR(vwd_fast_path_enable, 0644, pfe_vwd_show_fast_path_enable, pfe_vwd_set_fast_path_enable);
static DEVICE_ATTR(vwd_route_hook_enable, 0644, pfe_vwd_show_route_hook_enable, pfe_vwd_set_route_hook_enable);
static DEVICE_ATTR(vwd_bridge_hook_enable, 0644, pfe_vwd_show_bridge_hook_enable, pfe_vwd_set_bridge_hook_enable);

/** pfe_vwd_sysfs_init
 *
 */
static int pfe_vwd_sysfs_init( struct pfe_vwd_priv_s *priv )
{
	struct pfe *pfe = priv->pfe;

	if (device_create_file(pfe->dev, &dev_attr_vwd_debug_stats))
		goto err_dbg_sts;

	if (device_create_file(pfe->dev, &dev_attr_vwd_fast_path_enable))
		goto err_fp_en;

	if (device_create_file(pfe->dev, &dev_attr_vwd_route_hook_enable))
		goto err_rt;

	if (device_create_file(pfe->dev, &dev_attr_vwd_bridge_hook_enable))
		goto err_br;

	return 0;

err_br:
	device_remove_file(pfe->dev, &dev_attr_vwd_route_hook_enable);
err_rt:
	device_remove_file(pfe->dev, &dev_attr_vwd_fast_path_enable);
err_fp_en:
	device_remove_file(pfe->dev, &dev_attr_vwd_debug_stats);
err_dbg_sts:
	return -1;
}


/** pfe_vwd_sysfs_exit
 *
 */
static void pfe_vwd_sysfs_exit(void)
{
	device_remove_file(pfe->dev, &dev_attr_vwd_bridge_hook_enable);
	device_remove_file(pfe->dev, &dev_attr_vwd_route_hook_enable);
	device_remove_file(pfe->dev, &dev_attr_vwd_fast_path_enable);
	device_remove_file(pfe->dev, &dev_attr_vwd_debug_stats);
}


/** pfe_vwd_up
 *
 */
static int pfe_vwd_up(struct pfe_vwd_priv_s *priv )
{
	struct hif_client_s *client;
	int rc = 0, ii;

	printk("%s: start\n", __func__);

	nf_register_hook(&vwd_hook);
	nf_register_hook(&vwd_hook_ipv6);

	/* Some stats */
	priv->pkts_transmitted = 0;
	priv->pkts_tx_dropped = 0;
	priv->rx_skb_alloc_fail = 0;
	priv->pkts_rx_fast_forwarded[0] = 0;
	priv->pkts_rx_fast_forwarded[1] = 0;
	priv->pkts_slow_forwarded[0] = 0;
	priv->pkts_slow_forwarded[1] = 0;


	/* Register VWD Client driver with HIF */
	client = &priv->client;
	memset(client, 0, sizeof(*client));
	client->id = PFE_CL_VWD;
	client->tx_qn = VWD_TXQ_CNT;
	client->rx_qn = VWD_RXQ_CNT;
	client->priv    = priv;
	client->pfe     = priv->pfe;
	client->event_handler   = pfe_vwd_event_handler;

	/* FIXME : For now hif lib sets all tx and rx queues to same size */
	client->tx_qsize = EMAC_TXQ_DEPTH;
	client->rx_qsize = EMAC_RXQ_DEPTH;

	if ((rc = hif_lib_client_register(client))) {
		printk(KERN_ERR"%s: hif_lib_client_register(%d) failed\n", __func__, client->id);
		goto err0;
	}

	for (ii = 0; ii < VWD_TXQ_CNT; ii++)
		spin_lock_init(&priv->vwd_tx_lock[ii]);

	if (pfe_vwd_sysfs_init(priv))
		goto err1;


	priv->fast_path_enable = 0;
	priv->fast_bridging_enable = 0;
	priv->fast_routing_enable = 1;
	return rc;
err1:
	hif_lib_client_unregister(client);

err0:
	pfe_vwd_stop(priv);

	return -1;
}

/** pfe_vwd_down
 *
 */
static int pfe_vwd_down( struct pfe_vwd_priv_s *priv )
{
	printk(KERN_INFO "%s: %s\n", priv->name, __func__);


	if( priv->fast_bridging_enable )
	{
		nf_unregister_hook(&vwd_hook_bridge);
	}

	if( priv->fast_routing_enable )
	{
		nf_unregister_hook(&vwd_hook);
		nf_unregister_hook(&vwd_hook_ipv6);
	}

	pfe_vwd_sysfs_exit();
	pfe_vwd_stop(priv);
	hif_lib_client_unregister(&priv->client);

	return 0;
}

/** pfe_vwd_rx_page
 *
 */
static struct sk_buff *pfe_vwd_rx_page(struct pfe_vwd_priv_s *priv, int qno, int *expt, int *id)
{
	struct page *p;
	void *buf_addr, *temp;
	unsigned int rx_ctrl;
	unsigned int desc_ctrl = 0;
	struct sk_buff *skb;
	int length = 0, offset;

	buf_addr = hif_lib_receive_pkt(&priv->client, qno, &length, &offset, &rx_ctrl, &desc_ctrl, &temp);

	if (!buf_addr)
		goto out;

	p = virt_to_page(buf_addr);

	skb = dev_alloc_skb(MAX_HDR_SIZE + PFE_PKT_HEADROOM + 2);

	if (unlikely(!skb)) {
#ifdef VWD_DEBUG_STATS
		priv->rx_skb_alloc_fail += 1;
#endif
		goto pkt_drop;
	}

	skb_reserve(skb, PFE_PKT_HEADROOM + 2);

	/* We don't need the fragment if the whole packet */
	/* has been copied in the first linear skb        */
	if (length <= MAX_HDR_SIZE) {
		__memcpy(skb->data, buf_addr + offset, length);
		skb_put(skb, length);
		free_page((unsigned long)buf_addr);
	} else {
		__memcpy(skb->data, buf_addr + offset, MAX_HDR_SIZE);
		skb_put(skb, MAX_HDR_SIZE);
		skb_add_rx_frag(skb, 0, p, offset + MAX_HDR_SIZE, length - MAX_HDR_SIZE);
	}

	*id = (rx_ctrl >> HIF_CTRL_VAPID_OFST) & 0xff;

	if (rx_ctrl & HIF_CTRL_RX_WIFI_EXPT) {
		//printk(KERN_ERR "%s: This is dead packet....\n", __func__);
		*expt = 1;
		*(unsigned long *)skb->head = 0xdead;
	}

	return skb;

pkt_drop:

	if (skb) {
		kfree_skb(skb);
	} else {
		free_page((unsigned long)buf_addr);
	}

out:
	return NULL;
}

/** pfe_vwd_rx_skb
 *
 */
static struct sk_buff *pfe_vwd_rx_skb(struct pfe_vwd_priv_s *priv, int qno, int *expt, int *id)
{
	void *buf_addr, *temp;
	unsigned int rx_ctrl;
	unsigned int desc_ctrl = 0;
	struct sk_buff *skb = NULL;
	int length = 0, offset;

	buf_addr = hif_lib_receive_pkt(&priv->client, qno, &length, &offset, &rx_ctrl, &desc_ctrl, &temp);
	if (!buf_addr)
		goto out;


	skb = alloc_skb_header(PFE_BUF_SIZE, buf_addr, GFP_ATOMIC);
	if (unlikely(!skb)) {
#ifdef VWD_DEBUG_STATS
		priv->rx_skb_alloc_fail += 1;
#endif
		goto pkt_drop;
	}

	skb_reserve(skb, offset);
	skb_put(skb, length);

	*id = (rx_ctrl >>  HIF_CTRL_VAPID_OFST) & 0xff;

	if (rx_ctrl & HIF_CTRL_RX_WIFI_EXPT) {
		//printk(KERN_ERR "%s: This is dead packet....\n", __func__);
		*expt = 1;
		*(unsigned long *)skb->head = 0xdead;
	}

	return skb;

pkt_drop:
	if (skb) {
		kfree_skb(skb);
	} else {
		kfree(buf_addr);
	}

out:
	return NULL;
}

/** pfe_vwd_rx_poll
 *
 */
static int pfe_vwd_rx_poll( struct pfe_vwd_priv_s *priv, struct napi_struct *napi, int qno, int budget)
{
	struct sk_buff *skb;
	int work_done = 0;
	//unsigned int len;
	struct net_device *dev;

	//printk(KERN_INFO"%s\n", __func__);

	do {
		int expt = 0, id = MAX_VAP_SUPPORT;

		if (page_mode)
			skb = pfe_vwd_rx_page(priv, qno, &expt, &id);
		else
			skb = pfe_vwd_rx_skb(priv, qno, &expt, &id);

		if (!skb)
			break;

		if (id >= MAX_VAP_SUPPORT) {
			printk(KERN_ERR "%s : Unsuppoted VAP id : %d\n", __func__, id);
			dev_kfree_skb_any(skb);
			break;
		}

		spin_lock_bh(&priv->vaplock);
		if ((dev = dev_get_by_index(&init_net, priv->vaps[id].ifindex)) == NULL ) {
			spin_unlock_bh(&priv->vaplock);
			dev_kfree_skb_any(skb);
			break;
		}
		spin_unlock_bh(&priv->vaplock);

		skb->dev = dev;
		dev->last_rx = jiffies;


		/*FIXME: Need to handle WiFi to WiFi fast path */
		if (expt) {
			skb->protocol = eth_type_trans(skb, dev);
#ifdef VWD_DEBUG_STATS
                        priv->pkts_slow_forwarded[qno] += 1;
#endif
			netif_receive_skb(skb);
		}
		else {
			struct ethhdr *hdr;

                        hdr = (struct ethhdr *)skb->data;
                        skb->protocol = hdr->h_proto;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
                        skb->mac.raw = skb->data;
                        skb->nh.raw = skb->data + sizeof(struct ethhdr);
#else
                        skb_reset_mac_header(skb);
                        skb_set_network_header(skb, sizeof(struct ethhdr));
#endif

#ifdef VWD_DEBUG_STATS
                        priv->pkts_rx_fast_forwarded[qno] += 1;
#endif
                        //skb->protocol = __constant_htons(ETH_P_802_3);
                        skb->priority = 0;


			dev_queue_xmit(skb);
		}

		dev_put(dev);

		work_done++;
	} while (work_done < budget);

	/* If no Rx receive nor cleanup work was done, exit polling mode.
	 * No more netif_running(dev) check is required here , as this is checked in
	 * net/core/dev.c ( 2.6.33.5 kernel specific).
	 */
	if (work_done < budget) {
		napi_complete(napi);

		hif_lib_event_handler_start(&priv->client, EVENT_RX_PKT_IND, qno);
	}

	return work_done;
}

/** pfe_eth_low_poll
 */
static int pfe_vwd_rx_high_poll(struct napi_struct *napi, int budget)
{
	struct pfe_vwd_priv_s *priv = container_of(napi, struct pfe_vwd_priv_s, high_napi);

	return pfe_vwd_rx_poll(priv, napi, 1, budget);
}

/** pfe_eth_high_poll
 */
static int pfe_vwd_rx_low_poll(struct napi_struct *napi, int budget )
{
	struct pfe_vwd_priv_s *priv = container_of(napi, struct pfe_vwd_priv_s, low_napi);


	return pfe_vwd_rx_poll(priv, napi, 0, budget);
}
/** pfe_vwd_event_handler
 */
static int pfe_vwd_event_handler(void *data, int event, int qno)
{
	struct pfe_vwd_priv_s *priv = data;

	//printk(KERN_INFO "%s: %d\n", __func__, __LINE__);

	switch (event) {
		case EVENT_RX_PKT_IND:
			if (qno == 0) {
				if (napi_schedule_prep(&priv->low_napi)) {
					//printk(KERN_INFO "%s: schedule high prio poll\n", __func__);

					__napi_schedule(&priv->low_napi);
				}
			}
			else if (qno == 1) {
				if (napi_schedule_prep(&priv->high_napi)) {
					//printk(KERN_INFO "%s: schedule high prio poll\n", __func__);

					__napi_schedule(&priv->high_napi);
				}
			}
			break;

		case EVENT_TXDONE_IND:
		case EVENT_HIGH_RX_WM:
		default:
			break;
	}

	return 0;
}



/** pfe_vwd_driver_init
 *
 *	 PFE wifi offload:
 *	 - uses HIF functions to receive/send packets
 */

static int pfe_vwd_driver_init( struct pfe_vwd_priv_s *priv )
{
	printk("%s: start\n", __func__);

	strcpy(priv->name, "vwd");

	/* Initilize NAPI for Rx processing */
	init_dummy_netdev(&priv->dummy_dev);
	netif_napi_add(&priv->dummy_dev, &priv->low_napi, pfe_vwd_rx_low_poll, VWD_RX_POLL_WEIGHT);
	netif_napi_add(&priv->dummy_dev, &priv->high_napi, pfe_vwd_rx_high_poll, VWD_RX_POLL_WEIGHT);
	napi_enable(&priv->high_napi);
	napi_enable(&priv->low_napi);

	spin_lock_init(&priv->vaplock);
	priv->pfe = pfe;

	pfe_vwd_up(priv);
	printk("%s: end\n", __func__);
	return 0;
}


/** vwd_driver_remove
 *
 */
static int pfe_vwd_driver_remove(void)
{
	struct pfe_vwd_priv_s *priv = &pfe->vwd;

	pfe_vwd_down(priv);

	return 0;
}

/** pfe_vwd_handle_vap
 *
 */
static int pfe_vwd_handle_vap( struct pfe_vwd_priv_s *vwd, struct vap_cmd_s *cmd )
{
	struct vap_desc_s *vap;
	int rc = 0;
	unsigned long flags;
	struct net_device *dev;


	printk("%s function called %d: %s\n", __func__, cmd->action, cmd->ifname);

	dev = dev_get_by_name(&init_net, cmd->ifname);

	if((!dev) && (cmd->action != REMOVE ) )
		return -EINVAL;

	spin_lock_irqsave(&vwd->vaplock, flags);

	switch( cmd->action )
	{
		case ADD:
			/* Find free VAP */

			if( cmd->vapid >= MAX_VAP_SUPPORT )
			{
				rc = -EINVAL;
				goto done;
			}

			vap = &vwd->vaps[cmd->vapid];

			if( vap->ifindex )
			{
				rc = -EINVAL;
				break;
			}

			vap->vapid = cmd->vapid;
			vap->ifindex = cmd->ifindex;
			memcpy(vap->ifname, cmd->ifname, 12);
			memcpy(vap->macaddr, cmd->macaddr, 6);

			vap->dev = dev;


			printk("%s:ADD: name:%s, vapid:%d ifindex:%d mac:%x:%x:%x:%x:%x:%x\n", __func__,
					vap->ifname, vap->vapid, vap->ifindex,
					vap->macaddr[0], vap->macaddr[1],
					vap->macaddr[2], vap->macaddr[3],
					vap->macaddr[4], vap->macaddr[5] );

			vwd->vap_count++;
			break;
		case REMOVE:
			/* Find  VAP to be removed*/
			if( cmd->vapid >= MAX_VAP_SUPPORT )
			{
				rc = -EINVAL;
				goto done;
			}

			vap = &vwd->vaps[cmd->vapid];

			if( !vap->ifindex )
			{
				rc = 0;
				goto done;
			}

			printk("%s:REMOVE: name:%s, vapid:%d ifindex:%d mac:%x:%x:%x:%x:%x:%x\n", __func__,
					vap->ifname, vap->vapid, vap->ifindex,
					vap->macaddr[0], vap->macaddr[1],
					vap->macaddr[2], vap->macaddr[3],
					vap->macaddr[4], vap->macaddr[5] );
			vap->ifindex = 0;
			vap->dev = NULL; //FIXME : dev field is unused need to be removed from vap

			vwd->vap_count--;
			break;

		case UPDATE:
			/* Find VAP to be updated */

			if( cmd->vapid >= MAX_VAP_SUPPORT )
			{
				rc = -EINVAL;
				goto done;
			}

			vap = &vwd->vaps[cmd->vapid];

			if ( !vap->ifindex )
			{
				rc = -EINVAL;
				goto done;
			}

			printk("%s:UPDATE: old mac:%x:%x:%x:%x:%x:%x\n", __func__,
					vap->macaddr[0], vap->macaddr[1],
					vap->macaddr[2], vap->macaddr[3],
					vap->macaddr[4], vap->macaddr[5] );

			/* Not yet implemented */
			memcpy(vap->macaddr, cmd->macaddr, 6);

			printk("%s:UPDATE: name:%s, vapid:%d ifindex:%d mac:%x:%x:%x:%x:%x:%x\n", __func__,
					vap->ifname, vap->vapid, vap->ifindex,
					vap->macaddr[0], vap->macaddr[1],
					vap->macaddr[2], vap->macaddr[3],
					vap->macaddr[4], vap->macaddr[5] );
			break;

		default:
			rc =  -EINVAL;

	}
done:
	spin_unlock_irqrestore(&vwd->vaplock, flags);

	if(dev)
		dev_put(dev);


	return rc;

}

#define SIOCVAPUPDATE  ( 0x6401 )

/** pfe_vwd_ioctl
 *
 */
	static long
pfe_vwd_ioctl(struct file * file, unsigned int cmd, unsigned long arg)
{
	struct vap_cmd_s vap_cmd;
	void __user *argp = (void __user *)arg;
	struct pfe_vwd_priv_s *priv = (struct pfe_vwd_priv_s *)file->private_data;

	printk("%s: start\n", __func__);
	switch(cmd) {
		case SIOCVAPUPDATE:
			if (copy_from_user(&vap_cmd, argp, sizeof(struct vap_cmd_s)))
				return -EFAULT;

			return pfe_vwd_handle_vap(priv, &vap_cmd);
	}
	printk("%s: end\n", __func__);

	return -EOPNOTSUPP;
}


/** vwd_open
 *
 */
	static int
pfe_vwd_open(struct inode *inode, struct file *file)
{
	int result = 0;
	unsigned int dev_minor = MINOR(inode->i_rdev);

#if defined (CONFIG_COMCERTO_VWD_MULTI_MAC)
	printk( "%s :  Multi MAC mode enabled\n", __func__);
#endif
	printk( "%s :  minor device -> %d\n", __func__, dev_minor);
	if (dev_minor != 0)
	{
		printk(KERN_ERR ": trying to access unknown minor device -> %d\n", dev_minor);
		result = -ENODEV;
		goto out;
	}

	file->private_data = &pfe->vwd;

out:
	return result;
}

/** vwd_close
 *
 */
	static int
pfe_vwd_close(struct inode * inode, struct file * file)
{
	printk( "%s \n", __func__);

	return 0;
}

struct file_operations vwd_fops = {
unlocked_ioctl:	pfe_vwd_ioctl,
		open:		pfe_vwd_open,
		release:	pfe_vwd_close,
};


/** pfe_vwd_init
 *
 */
int pfe_vwd_init(struct pfe *pfe)
{
	struct pfe_vwd_priv_s	*priv ;
	int rc = 0;

	printk(KERN_INFO "%s\n", __func__);
	priv  = &pfe->vwd;
	memset(priv, 0, sizeof(*priv));


	rc = alloc_chrdev_region(&priv->char_devno,VWD_MINOR, VWD_MINOR_COUNT, VWD_DRV_NAME);
	if (rc < 0) {
		printk(KERN_ERR "%s: alloc_chrdev_region() failed\n", __func__);
		goto err0;
	}

	cdev_init(&priv->char_dev, &vwd_fops);
	priv->char_dev.owner = THIS_MODULE;

	rc = cdev_add (&priv->char_dev, priv->char_devno, VWD_DEV_COUNT);
	if (rc < 0) {
		printk(KERN_ERR "%s: cdev_add() failed\n", __func__);
		goto err1;
	}

	printk(KERN_INFO "%s: created vwd device(%d, %d)\n", __func__, MAJOR(priv->char_devno),
			MINOR(priv->char_devno));

	priv->pfe = pfe;

	if( pfe_vwd_driver_init( priv ) )
		goto err1;

	return 0;

err1:

	unregister_chrdev_region(priv->char_devno, VWD_MINOR_COUNT);

err0:
	return rc;
}

/** pfe_vwd_exit
 *
 */
void pfe_vwd_exit(struct pfe *pfe)
{
	struct pfe_vwd_priv_s	*priv = &pfe->vwd;

	printk(KERN_INFO "%s\n", __func__);

	pfe_vwd_driver_remove();
	cdev_del(&priv->char_dev);
	unregister_chrdev_region(priv->char_devno, VWD_MINOR_COUNT);
}
#else
/** pfe_vwd_init
 *
 */
int pfe_vwd_init(struct pfe *pfe)
{
	printk(KERN_INFO "%s\n", __func__);
	return 0;
}

/** pfe_vwd_exit
 *
 */
void pfe_vwd_exit(struct pfe *pfe)
{
	printk(KERN_INFO "%s\n", __func__);
}
#endif

MODULE_LICENSE("GPL");


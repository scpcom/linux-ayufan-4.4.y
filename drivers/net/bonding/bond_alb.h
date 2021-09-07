#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __BOND_ALB_H__
#define __BOND_ALB_H__

#include <linux/if_ether.h>

struct bonding;
struct slave;

#define BOND_ALB_INFO(bond)   ((bond)->alb_info)
#define SLAVE_TLB_INFO(slave) ((slave)->tlb_info)

struct tlb_client_info {
	struct slave *tx_slave;	 
	u32 tx_bytes;		 
	u32 load_history;	 
	u32 next;		 
	u32 prev;		 
};

struct rlb_client_info {
	__be32 ip_src;		 
	__be32 ip_dst;		 
	u8  mac_dst[ETH_ALEN];	 
	u32 next;		 
	u32 prev;		 
	u8  assigned;		 
	u8  ntt;		 
	struct slave *slave;	 
	u8 tag;			 
	unsigned short vlan_id;	 
};

struct tlb_slave_info {
	u32 head;	 
	u32 load;	 
};

struct alb_bond_info {
	struct timer_list	alb_timer;
	struct tlb_client_info	*tx_hashtbl;  
	spinlock_t		tx_hashtbl_lock;
	u32			unbalanced_load;
	int			tx_rebalance_counter;
	int			lp_counter;
	 
	int rlb_enabled;
	struct packet_type	rlb_pkt_type;
	struct rlb_client_info	*rx_hashtbl;	 
	spinlock_t		rx_hashtbl_lock;
	u32			rx_hashtbl_head;
	u8			rx_ntt;	 
	struct slave		*next_rx_slave; 
	u32			rlb_interval_counter;
	u8			primary_is_promisc;	    
	u32			rlb_promisc_timeout_counter; 
	u32			rlb_update_delay_counter;
	u32			rlb_update_retry_counter; 
	u8			rlb_rebalance;	 
	struct vlan_entry	*current_alb_vlan;
};

int bond_alb_initialize(struct bonding *bond, int rlb_enabled);
void bond_alb_deinitialize(struct bonding *bond);
int bond_alb_init_slave(struct bonding *bond, struct slave *slave);
void bond_alb_deinit_slave(struct bonding *bond, struct slave *slave);
void bond_alb_handle_link_change(struct bonding *bond, struct slave *slave, char link);
void bond_alb_handle_active_change(struct bonding *bond, struct slave *new_slave);
int bond_alb_xmit(struct sk_buff *skb, struct net_device *bond_dev);
void bond_alb_monitor(struct work_struct *);
int bond_alb_set_mac_address(struct net_device *bond_dev, void *addr);
void bond_alb_clear_vlan(struct bonding *bond, unsigned short vlan_id);
#ifdef	MY_ABC_HERE
void bond_alb_info_show(struct seq_file *seq);
#endif
#endif  

/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#include "module_bridge.h"
#include "fpp_globals.h"
#include "modules.h"
#include "events.h"
#include "module_Rx.h"
#include "gemac.h"
#include "fpool.h"
#include "fpp.h"
#include "layer2.h"
#include "module_hidrv.h"
#include "module_timer.h"
#include "module_expt.h"
#include "module_ethernet.h"
#include "module_ipv4.h"
#include "module_ipv6.h"
#include "module_stat.h"
#include "system.h"
#include "fe.h"
#include "channels.h"
#include "module_qm.h"
#include "module_wifi.h"

#if defined(COMCERTO_2000)
#include "control_bridge.h"
#include "control_common.h"
PVOID CLASS_DMEM_SH2 (bridge_cache)[NUM_BT_ENTRIES] __attribute__((aligned(32)));
PVOID CLASS_PE_LMEM_SH2 (bridge_l3_cache)[NUM_BT_L3_ENTRIES]  __attribute__((aligned(32)));
struct _tbridge_enable CLASS_DMEM_SH2(L2bridge_enabled)[MAX_PHY_PORTS] __attribute__((aligned(32)));

struct dlist_head hw_bridge_active_list[NUM_BT_ENTRIES];
struct dlist_head hw_bridge_l3_active_list[NUM_BT_L3_ENTRIES];
struct dlist_head hw_bridge_removal_list;

struct dma_pool *bridge_dma_pool;

extern TIMER_ENTRY bridge_delayed_removal_timer;

U32 L2Bridge_mode;
U32 L2Bridge_entries;
U32 L2Bridge_bin_number;
U32 L2Bridge_l3_bin_number;
U32 L2Bridge_timeout;
#endif

int rx_Get_Next_Hash_BridgeEntry(PL2BridgeQueryEntryResponse pBridgeCmd , int reset_action);
int rx_Get_Next_Hash_L2FlowEntry(PL2BridgeL2FlowEntryCommand pL2FlowCmd, int reset_action);

extern TIMER_ENTRY bridge_timer;


#if !defined(COMCERTO_2000)
static PVOID l2bridge_alloc(void)
{
	return Heap_Alloc_ARAM(sizeof(L2Bridge_entry));
}

static void l2bridge_free(PL2Bridge_entry pL2Bridge)
{
	Heap_Free((PVOID) pL2Bridge);
}

static int l2bridge_add(PL2Bridge_entry pL2Bridge, U32 hash)
{
	/* Add software entry to local hash */
	slist_add(&bridge_cache[hash], &pL2Bridge->list);

	L2Bridge_entries++;
			
	return 0;
}

static void l2bridge_remove(PL2Bridge_entry pL2Bridge, U32 hash)
{
	slist_remove(&bridge_cache[hash], &pL2Bridge->list);

	l2bridge_free(pL2Bridge);

	L2Bridge_entries--;
}

static void l2bridge_enable(U16 port_id, U16 flag)
{
	L2bridge_enabled[port_id].enabled = flag;
}

#else	// defined(COMCERTO_2000)

/** Update PE's internal memory bridge cache tables with the HW entry's DDR address
*
* @entry_ddr		DDR physical address of the hw socket entry
*
*/
static void hw_l2bridge_add_to_class(U32 host_addr, U32 hash)
{
	U32	*pe_addr = (U32*)&class_bridge_cache[hash];
	int id;

	for(id = CLASS0_ID; id <= CLASS_MAX_ID; id++)
		pe_dmem_writel(id, host_addr, virt_to_class_dmem(pe_addr));
}


static void hw_l2bridge_schedule_remove(struct _thw_L2Bridge_entry *hw_bridge)
{
	hw_bridge->removal_time = jiffies + 2;
	dlist_add(&hw_bridge_removal_list, &hw_bridge->list);
}

/** Processes hardware socket delayed removal list.
* Free all hardware socket in the removal list that have reached their removal time.
*
*
*/
static void hw_l2bridge_delayed_remove(void)
{
	struct dlist_head *entry;
	struct _thw_L2Bridge_entry *hw_bridge;

	dlist_for_each_safe(hw_bridge, entry, &hw_bridge_removal_list, list)
	{
		if (!time_after(jiffies, hw_bridge->removal_time))
			continue;

		dlist_remove(&hw_bridge->list);

		dma_pool_free(bridge_dma_pool, hw_bridge, be32_to_cpu(hw_bridge->dma_addr));
	}
}

static PVOID l2bridge_alloc(void)
{
	return kzalloc(sizeof(L2Bridge_entry), GFP_KERNEL);
}

static void l2bridge_free(PL2Bridge_entry pL2Bridge)
{
	kfree((PVOID)pL2Bridge);
}

static int l2bridge_add(PL2Bridge_entry pL2Bridge, U32 hash)
{
	struct _thw_L2Bridge_entry *hw_l2bridge;
	struct _thw_L2Bridge_entry *hw_l2bridge_first;
	dma_addr_t dma_addr;
	int rc = NO_ERR;
	
	/* Allocate hardware entry */
	hw_l2bridge = dma_pool_alloc(bridge_dma_pool, GFP_ATOMIC, &dma_addr);
	if (!hw_l2bridge)
	{
		printk(KERN_ERR "%s: dma alloc failed\n", __func__);
		rc = ERR_NOT_ENOUGH_MEMORY;
		goto err;
	}

	memset(hw_l2bridge, 0, sizeof(*hw_l2bridge));
	hw_l2bridge->dma_addr = cpu_to_be32(dma_addr);

	/* Link software conntrack to hardware conntrack */
	pL2Bridge->hw_l2bridge = hw_l2bridge;
	hw_l2bridge->sw_l2bridge = pL2Bridge;

	pL2Bridge->hash = hash;

	/* add hw entry to active list and update next pointer */
	if(!dlist_empty(&hw_bridge_active_list[pL2Bridge->hash]))
	{
		/* list is not empty, and we'll be added at head, so current first will become our next pointer */
		hw_l2bridge_first = container_of(dlist_first(&hw_bridge_active_list[pL2Bridge->hash]), typeof(struct _thw_L2Bridge_entry), list);
		hw_entry_set_field(&hw_l2bridge->next, hw_entry_get_field(&hw_l2bridge_first->dma_addr));
	}
	else 
	{
		/* entry is empty, so we'll be the first and only one entry */
		hw_entry_set_field(&hw_l2bridge->next, 0);
	}

	dlist_add(&hw_bridge_active_list[pL2Bridge->hash], &hw_l2bridge->list);

	/* reflect changes to hardware flow */
	hw_l2bridge->valid = pL2Bridge->valid;
	hw_l2bridge->hash = pL2Bridge->hash;

	memcpy(hw_l2bridge->da, pL2Bridge->da, 6);
	memcpy(hw_l2bridge->sa, pL2Bridge->sa, 6);

	hw_l2bridge->ethertype = pL2Bridge->ethertype; /* already set in BE by upper function*/
	hw_l2bridge->input_vlan = pL2Bridge->input_vlan;
	hw_l2bridge->output_vlan = pL2Bridge->output_vlan;
	hw_l2bridge->session_id = pL2Bridge->session_id;
	hw_l2bridge->pkt_priority = pL2Bridge->pkt_priority;
	hw_l2bridge->vlan_priority = pL2Bridge->vlan_priority;
	hw_l2bridge->vlan_priority_mark = pL2Bridge->vlan_priority_mark;
	hw_l2bridge->sa_wc = pL2Bridge->sa_wc;
	hw_l2bridge->qmod = pL2Bridge->qmod;
	hw_l2bridge->input_interface = pL2Bridge->input_interface;

	hw_l2bridge->output_itf = (void *)cpu_to_be32(virt_to_class_dmem(pL2Bridge->output_itf));

	/* Update PE's internal memory bridge cache tables with the HW entry's DDR address */
	hw_l2bridge_add_to_class(hw_l2bridge->dma_addr, hash);

	/* Add software entry to local hash */
	slist_add(&bridge_cache[hash], &pL2Bridge->list);

	L2Bridge_entries++;

	return NO_ERR;

err:
	return rc;

}

static void l2bridge_remove(PL2Bridge_entry pL2Bridge, U32 hash)
{
	struct _thw_L2Bridge_entry *hw_l2bridge;
	struct _thw_L2Bridge_entry *hw_l2bridge_prev;

	/* Check if there is a hardware flow */
	if ((hw_l2bridge = pL2Bridge->hw_l2bridge))
	{
		/* Detach from software socket */
		pL2Bridge->hw_l2bridge = NULL;

		/* if the removed entry is first in hash slot then only PE dmem hash need to be updated */
		if (&hw_l2bridge->list == dlist_first(&hw_bridge_active_list[hash])) 
		{
			hw_l2bridge_add_to_class(hw_entry_get_field(&hw_l2bridge->next), hash);
		}
		else
		{
			hw_l2bridge_prev = container_of(hw_l2bridge->list.prev, typeof(struct _thw_L2Bridge_entry), list);
			hw_entry_set_field(&hw_l2bridge_prev->next, hw_entry_get_field(&hw_l2bridge->next));
		}

		dlist_remove(&hw_l2bridge->list);

		hw_l2bridge_schedule_remove(hw_l2bridge);
	}

	slist_remove(&bridge_cache[hash], &pL2Bridge->list);

	l2bridge_free(pL2Bridge);

	L2Bridge_entries--;
}

static void l2bridge_enable(U16 port_id, U16 flag)
{
	int id;

	L2bridge_enabled[port_id].enabled = flag;

	/* update the DMEM in class-pe */
	for (id = CLASS0_ID; id <= CLASS_MAX_ID; id++)
	{
		pe_dmem_writeb(id, (U32)flag, virt_to_class_dmem((void*)&class_L2bridge_enabled[port_id].enabled));
	}
}


#endif	// defined(COMCERTO_2000)

static PL2Bridge_entry l2bridge_find_entry(U32 hash, U8 *destaddr, U16 ethertype, U16 vlan_id, U16 session_id)
{
	PL2Bridge_entry pEntry;
	PL2Bridge_entry pL2Bridge = NULL;
	struct slist_entry *entry;

	slist_for_each(pEntry, entry, &bridge_cache[hash], list)
	{
		if (TESTEQ_L2ENTRY(pEntry, (void*)destaddr, ethertype, vlan_id, session_id))
			pL2Bridge = pEntry;
	}

	return pL2Bridge;
}

int l2bridge_manual_reset(void)
{
	PL2Bridge_entry pEntry;
	struct slist_entry *entry;
	int hash;

	for(hash = 0; hash < NUM_BT_ENTRIES; hash++)
	{
		slist_for_each_safe(pEntry, entry, &bridge_cache[hash], list)
		{
			l2bridge_remove(pEntry, hash);
		}
	}

	return NO_ERR;
}

//FIXME: Auto Bridge support
#if 0
void M_bridge_remove_l2_flow_entry(PL2Flow_entry l2flow_entry)
{
	PL2Flow_entry last_Entry;
	struct L2Flow_tmp  l2flow_tmp;
	int hash;

	l2flow_tmp.da_sa = l2flow_entry->l2flow.da;
	l2flow_tmp.ethertype = l2flow_entry->l2flow.ethertype;
	l2flow_tmp.vlan_tag =  l2flow_entry->l2flow.vlan_tag;
	l2flow_tmp.session_id =  l2flow_entry->l2flow.session_id;

	hash = HASH_L2FLOW(&l2flow_tmp);


	if(l2flow_entry == (PL2Flow_entry)bridge_cache[hash])
		bridge_cache[hash] = l2flow_entry->next;
	else{
		last_Entry = (PL2Flow_entry) bridge_cache[hash];
		while(last_Entry != NULL)
		{
			if(last_Entry->next == l2flow_entry){
				last_Entry->next = l2flow_entry->next;

				break;
			}
			last_Entry = last_Entry->next;
		}
	}
	Heap_Free((PVOID) l2flow_entry);
	L2Bridge_entries--;
}


static int M_bridge_expire_l2_flow_entry(PL2Flow_entry l2flow_entry)
{
	L2BridgeL2FlowEntryCommand *message;
	HostMessage *pmsg;

	// Send indication message
	pmsg = msg_alloc();
	if (!pmsg)
	{
		l2flow_entry->status |= L2_BRIDGE_TIMED_OUT;
		return 1;
	}

	message = (L2BridgeL2FlowEntryCommand *)pmsg->data;

	// Prepare indication message
	memset(message, 0 , sizeof(*message));
	message->action =  ACTION_REMOVED;
	SFL_memcpy(message->destaddr, l2flow_entry->l2flow.da, 2 * ETHER_ADDR_LEN);
	message->ethertype = l2flow_entry->l2flow.ethertype;
	message->vlan_tag = l2flow_entry->l2flow.vlan_tag;
	message->session_id = l2flow_entry->l2flow.session_id;

	pmsg->code = CMD_RX_L2BRIDGE_FLOW_ENTRY;
	pmsg->length = sizeof(*message);

	msg_send(pmsg);

	M_bridge_remove_l2_flow_entry(l2flow_entry);

	return 0;
}


static void M_bridge_timer_l2(unsigned int xstart)
{
	PL2Flow_entry l2flow_entry;
	U32 i;
	U32 timer;

	for (i = xstart; i < xstart + L2_BRIDGE_TIMER_BINSIZE; i++){
restart_loop:
		l2flow_entry = (PL2Flow_entry) bridge_cache[i];
		if (l2flow_entry == NULL)
			continue;

		while(l2flow_entry){
			timer = ct_timer - l2flow_entry->last_l2flow_timer;

			if ((!l2flow_entry->nb_l3_ref) && ((timer > L2Bridge_timeout) || (l2flow_entry->status & L2_BRIDGE_TIMED_OUT))){
				if (!M_bridge_expire_l2_flow_entry(l2flow_entry))
					goto restart_loop;
			}
			l2flow_entry = l2flow_entry->next;
		}
	}
}


static void M_bridge_remove_l3_flow_entry(PL3Flow_entry l3flow_entry)
{
	PL3Flow_entry last_Entry;

	int hash;
	hash = HASH_L3FLOW(&l3flow_entry->l3flow);


	if(l3flow_entry == (PL3Flow_entry)bridge_l3_cache[hash])
		bridge_l3_cache[hash] = l3flow_entry->next;
	else{
		last_Entry = (PL3Flow_entry) bridge_l3_cache[hash];
		while(last_Entry != NULL)
		{
			if(last_Entry->next == l3flow_entry){
				last_Entry->next = l3flow_entry->next;
				break;
			}
			last_Entry = last_Entry->next;
		}
	}
	Heap_Free((PVOID) l3flow_entry);
}


static int M_bridge_expire_l3_flow_entry(PL3Flow_entry l3flow_entry)
{

	L2BridgeL2FlowEntryCommand *message;
	PL2Flow_entry l2flow_entry;
	HostMessage *pmsg;

	// Send indication message
	pmsg = msg_alloc();
	if (!pmsg)
	{
		l3flow_entry->status |= L2_BRIDGE_TIMED_OUT;
		return 1;
	}

	l2flow_entry = l3flow_entry->l2flow_entry;

	message = (L2BridgeL2FlowEntryCommand *)pmsg->data;
	
	/* Prepare indication message */
	memset(message, 0 , sizeof(*message));
	message->action = ACTION_REMOVED;

	/* L2 info */
	SFL_memcpy(message->destaddr, l2flow_entry->l2flow.da, 2 * ETHER_ADDR_LEN);
	message->ethertype = l2flow_entry->l2flow.ethertype;
	message->vlan_tag = l2flow_entry->l2flow.vlan_tag;
	message->session_id = l2flow_entry->l2flow.session_id;

	/* L3-4 info */
	SFL_memcpy(message->saddr, l3flow_entry->l3flow.saddr, IPV6_ADDRESS_LENGTH);
	SFL_memcpy(message->daddr, l3flow_entry->l3flow.daddr, IPV6_ADDRESS_LENGTH);
	message->proto = l3flow_entry->l3flow.proto;
	message->sport = l3flow_entry->l3flow.sport;
	message->dport = l3flow_entry->l3flow.dport;

	pmsg->code = CMD_RX_L2BRIDGE_FLOW_ENTRY;
	pmsg->length = sizeof(*message);

	msg_send(pmsg);

	M_bridge_remove_l3_flow_entry(l3flow_entry);

	if((--l2flow_entry->nb_l3_ref) == 0)
		M_bridge_remove_l2_flow_entry(l2flow_entry);

	return 0;
}


static void M_bridge_timer_l3(unsigned int xstart)
{
	PL3Flow_entry l3flow_entry;
	U32 i;
	U32 timer;

	for (i = xstart; i < xstart + L2_BRIDGE_L3_TIMER_BINSIZE; i++){
restart_loop:
		l3flow_entry = (PL3Flow_entry) bridge_l3_cache[i];
		if (l3flow_entry == NULL)
			continue;

		while(l3flow_entry){
			timer = ct_timer - l3flow_entry->last_l3flow_timer;

			if ((timer > L2Bridge_timeout) || (l3flow_entry->status & L2_BRIDGE_TIMED_OUT)){
				if (!M_bridge_expire_l3_flow_entry(l3flow_entry))
					goto restart_loop;
			}
			l3flow_entry = l3flow_entry->next;
		}
	}
}

static void M_bridge_timer(void)
{
	unsigned int bin;

	bin = L2Bridge_bin_number;
	L2Bridge_bin_number = (L2Bridge_bin_number + 1) & (L2_BRIDGE_TIMER_NUMBINS-1);
	M_bridge_timer_l2(bin * L2_BRIDGE_TIMER_BINSIZE);

	bin = L2Bridge_l3_bin_number;
	L2Bridge_l3_bin_number = (L2Bridge_l3_bin_number + 1) & (L2_BRIDGE_L3_TIMER_NUMBINS-1);
	M_bridge_timer_l3(bin * L2_BRIDGE_L3_TIMER_BINSIZE);

}

static int M_bridge_handle_l2flow(U16 *p, U16 Length)
{
	U16 ackstatus = CMD_OK;
	char has_l3 = 0;
	char new_l2 = 0;
	POnifDesc pOnif;
	POnifDesc pInif;
	U32 hash, hash_l3 = 0;
	struct L2Flow_tmp l2flow_tmp;
	struct L3Flow l3flow_tmp;
	PL2Flow_entry l2flow_entry;
	PL3Flow_entry l3flow_entry = NULL;
	char reset_action = 0;
	PL2BridgeL2FlowEntryCommand pcmd = (PL2BridgeL2FlowEntryCommand)p;

	memset(&l2flow_tmp, 0, sizeof(struct L2Flow));
	memset(&l3flow_tmp, 0, sizeof(struct L3Flow));

	if(Length != sizeof(L2BridgeL2FlowEntryCommand))
		return ERR_WRONG_COMMAND_SIZE;

	//SFL_memcpy(l2flow_tmp.da, pcmd->destaddr, 2 * ETHER_ADDR_LEN);
	l2flow_tmp.da_sa = pcmd->destaddr;
	l2flow_tmp.ethertype = pcmd->ethertype;
	l2flow_tmp.session_id = pcmd->session_id;
	l2flow_tmp.vlan_tag = pcmd->vlan_tag;

	SFL_memcpy(l3flow_tmp.saddr, pcmd->saddr, IPV6_ADDRESS_LENGTH);
	SFL_memcpy(l3flow_tmp.daddr, pcmd->daddr, IPV6_ADDRESS_LENGTH);
	l3flow_tmp.proto = pcmd->proto;
	l3flow_tmp.sport = pcmd->sport;
	l3flow_tmp.dport = pcmd->dport;

	hash = HASH_L2FLOW(&l2flow_tmp);
	if((l2flow_entry = M_bridge_find_l2_flow_entry(NULL, hash, &l2flow_tmp)) == NULL)
		new_l2 = 1;

	if(l3flow_tmp.proto){
		has_l3 = 1;
		hash_l3 = HASH_L3FLOW(&l3flow_tmp);
		l3flow_entry = M_bridge_find_l3_flow_entry(NULL, hash_l3, &l3flow_tmp);
	}

	switch(pcmd->action){
		case ACTION_REGISTER:
			if (l3flow_entry || (!has_l3 && l2flow_entry)) {
				ackstatus = ERR_BRIDGE_ENTRY_ALREADY_EXISTS;
				break;
			}
			if((pOnif = get_onif_by_name(pcmd->output_name)) == NULL) {
				ackstatus = ERR_UNKNOWN_INTERFACE;
				break;
			}
			if((pInif = get_onif_by_name(pcmd->input_name)) == NULL) {
				ackstatus = ERR_UNKNOWN_INTERFACE;
				break;
			}
			if(new_l2)
				if ((l2flow_entry = (PL2Flow_entry)Heap_Alloc_ARAM(sizeof(L2Flow_entry))) == NULL){
					ackstatus = ERR_NOT_ENOUGH_MEMORY;
					break;
				}

			if(has_l3)
				if ((l3flow_entry = (PL3Flow_entry)Heap_Alloc_ARAM(sizeof(L3Flow_entry))) == NULL){
					ackstatus = ERR_NOT_ENOUGH_MEMORY;
					if(new_l2)
						Heap_Free((PVOID)l2flow_entry);
					break;
				}
			if(new_l2){
				memset(l2flow_entry, 0, sizeof(L2Flow_entry));
				l2flow_entry->next = (PL2Flow_entry) bridge_cache[hash];
				bridge_cache[hash] = l2flow_entry;
				SFL_memcpy(l2flow_entry->l2flow.da, l2flow_tmp.da_sa, 2 * ETHER_ADDR_LEN);
				l2flow_entry->l2flow.ethertype = l2flow_tmp.ethertype;
				l2flow_entry->l2flow.session_id = l2flow_tmp.session_id;
				l2flow_entry->l2flow.vlan_tag = l2flow_tmp.vlan_tag;
				l2flow_entry->input_if = get_onif_index(pInif);
				l2flow_entry->output_if = get_onif_index(pOnif);
				l2flow_entry->output_if_type = pOnif->itf->type;
				l2flow_entry->output_if_h = pOnif->itf;
				l2flow_entry->last_l2flow_timer = ct_timer;
				l2flow_entry->mark = pcmd->mark;
				L2Bridge_entries++;
			}

			if(has_l3){
				memset(l3flow_entry, 0, sizeof(L3Flow_entry));
				l3flow_entry->next = bridge_l3_cache[hash_l3];
				bridge_l3_cache[hash_l3] = l3flow_entry;
				SFL_memcpy(&l3flow_entry->l3flow, &l3flow_tmp, sizeof(struct L3Flow));
				l3flow_entry->l2flow_entry = l2flow_entry;
				l3flow_entry->last_l3flow_timer = ct_timer;
				l3flow_entry->mark = pcmd->mark;
				l2flow_entry->nb_l3_ref++;
			}
			break;

		case ACTION_UPDATE:
			if (!l2flow_entry) {
				ackstatus = ERR_BRIDGE_ENTRY_NOT_FOUND;
				break;
			}
			if((pOnif = get_onif_by_name(pcmd->output_name)) == NULL) {
				ackstatus = ERR_UNKNOWN_INTERFACE;
				break;
			}
			if((pInif = get_onif_by_name(pcmd->input_name)) == NULL) {
				ackstatus = ERR_UNKNOWN_INTERFACE;
				break;
			}
			l2flow_entry->input_if = get_onif_index(pInif);
			l2flow_entry->output_if = get_onif_index(pOnif);
			l2flow_entry->output_if_type = pOnif->itf->type;
			l2flow_entry->output_if_h = pOnif->itf;

			break;

		case ACTION_DEREGISTER:
			if (!l2flow_entry  || (has_l3 && !l3flow_entry)){
				ackstatus = ERR_BRIDGE_ENTRY_NOT_FOUND;
				break;
			}
			if(has_l3){
				M_bridge_remove_l3_flow_entry(l3flow_entry);
				l2flow_entry->nb_l3_ref--;
			}
			if(!l2flow_entry->nb_l3_ref)
				M_bridge_remove_l2_flow_entry(l2flow_entry);

			break;

		case ACTION_QUERY:
				reset_action = 1;
			/* fallthrough */
		case ACTION_QUERY_CONT:
				ackstatus = rx_Get_Next_Hash_L2FlowEntry(pcmd, reset_action);
				break;
		default:
				ackstatus = ERR_UNKNOWN_ACTION;
			break;
	}//End switch

	return ackstatus;
}


static int M_bridge_handle_control(U16 code, U16 *p, U16 Length)
{
	U16 ackstatus = CMD_OK;
	PL2BridgeControlCommand prsp = (PL2BridgeControlCommand)p;

	if(code == CMD_RX_L2BRIDGE_FLOW_TIMEOUT){
		L2Bridge_timeout = prsp->mode_timeout * L2_BRIDGE_TICKS_PER_SECOND;
	}
	else if (code == CMD_RX_L2BRIDGE_MODE){
			if(prsp->mode_timeout > L2_BRIDGE_MODE_AUTO){
				ackstatus = ERR_WRONG_COMMAND_PARAM;
			}
			L2Bridge_mode = prsp->mode_timeout;

			if(L2Bridge_mode == L2_BRIDGE_MODE_AUTO){
				timer_add(&bridge_timer, L2_BRIDGE_TIMER_INTERVAL);
			}
			else
				timer_del(&bridge_timer);
	}
	else{
		ackstatus = ERR_UNKNOWN_COMMAND;
	}
	return ackstatus;
}


static int  M_bridge_handle_reset(void)
{
	U16 ackstatus = CMD_OK;
	PL2Flow_entry l2flow_last_entry;
	PL3Flow_entry l3flow_last_entry;
	int i;

	for(i = 0; i < NUM_BT_ENTRIES; i++)
	{
		while(bridge_cache[i] != NULL)
		{
			l2flow_last_entry = bridge_cache[i];
			bridge_cache[i] = l2flow_last_entry->next;
			Heap_Free((PVOID)l2flow_last_entry);
			L2Bridge_entries--;
		}
	}
	for(i = 0; i < NUM_BT_L3_ENTRIES; i++)
	{
		while(bridge_l3_cache[i] != NULL)
		{
			l3flow_last_entry = bridge_l3_cache[i];
			bridge_l3_cache[i] = l3flow_last_entry->next;
			Heap_Free((PVOID)l3flow_last_entry);
		}
	}
	return ackstatus;
}
#endif

U16 M_bridge_cmdproc(U16 cmd_code, U16 cmd_len, U16 *p)
{
	U16 acklen;
	U16 ackstatus;

	acklen = 2;
	ackstatus = CMD_OK;

	/* For legacy support of manual bridging, TO BE REMOVED */
	switch (cmd_code)
	{
		case CMD_RX_L2BRIDGE_ENABLE:
		case CMD_RX_L2BRIDGE_ADD:
		case CMD_RX_L2BRIDGE_REMOVE:	
		case CMD_RX_L2BRIDGE_QUERY_STATUS:		
		case CMD_RX_L2BRIDGE_QUERY_ENTRY:
			if(L2Bridge_mode == L2_BRIDGE_MODE_AUTO){
				ackstatus = ERR_BRIDGE_WRONG_MODE;
				goto exit;
			}
			break;
		case CMD_RX_L2BRIDGE_FLOW_ENTRY:
		case CMD_RX_L2BRIDGE_FLOW_TIMEOUT:
		case CMD_RX_L2BRIDGE_FLOW_RESET:
			if(L2Bridge_mode == L2_BRIDGE_MODE_MANUAL){
				ackstatus = ERR_BRIDGE_WRONG_MODE;
				goto exit;
			}
			break;
		default:
			break;
	}

	switch (cmd_code)
	{
		case CMD_RX_L2BRIDGE_ENABLE: {
			PL2BridgeEnableCommand pcmd = (PL2BridgeEnableCommand)p;
			U16 phy_port_id = pcmd->interface;

			if ( pcmd->interface > 1 )
			{
				POnifDesc pOnif;

				if((pOnif = get_onif_by_name(pcmd->input_name)) == NULL) {
					ackstatus = ERR_UNKNOWN_INTERFACE;	break;
				}

				phy_port_id = itf_get_phys_port(pOnif->itf);

			/*FIXME : Below code might not required by c1000 also, need to be removed*/
#if defined(WIFI_ENABLE) && defined(COMCERTO_1000)
				if( !onif_is_wifi( pOnif ) ) {
					ackstatus = ERR_UNKNOWN_INTERFACE; break;
				}
#endif
			}

			if ( (pcmd->interface != 0xffff) && (phy_port_id < MAX_PHY_PORTS) && (L2bridge_enabled[phy_port_id].used) )
				l2bridge_enable(phy_port_id, pcmd->enable_flag);

			break;
		}
		case CMD_RX_L2BRIDGE_ADD: {
			POnifDesc pOnif;
			U32 hash;
			U16 ethertype_bigendian;
			U16 vlanid_bigendian;
			U16 session_id;
			U16 input_interface;
			struct itf *output_itf;
			PL2Bridge_entry pEntry;
			PL2BridgeAddEntryCommand pcmd = (PL2BridgeAddEntryCommand)p;
			ethertype_bigendian = htons(pcmd->ethertype); // NULL if wildcard entry
			vlanid_bigendian = htons(pcmd->input_vlan);
			session_id = htons(pcmd->session_id);

			if(pcmd->input_interface > 1) {
				if((pOnif = get_onif_by_name(pcmd->input_name)) == NULL) {
					ackstatus = ERR_UNKNOWN_INTERFACE;
					break;
				}
				input_interface = itf_get_phys_port(pOnif->itf);
			}
			else
				input_interface = pcmd->input_interface;

			if(pcmd->output_interface > 1) {
				if((pOnif = get_onif_by_name(pcmd->output_name)) == NULL) {
					ackstatus = ERR_UNKNOWN_INTERFACE;	break;
				}

				if (pcmd->output_interface == 0xffff) {
					if (!(pOnif->itf->type & IF_TYPE_TUNNEL)) {
						ackstatus = ERR_UNKNOWN_INTERFACE;	break;
					}
				}
				else {
					if (!(pOnif->itf->type & IF_TYPE_PHYSICAL)) {
						ackstatus = ERR_UNKNOWN_INTERFACE;	break;
					}
				}

				output_itf = pOnif->itf;
			}
			else
				output_itf = &phy_port[pcmd->output_interface].itf;

			if (TESTEQ_NULL_MACADDR(pcmd->srcaddr) || (pcmd->ethertype == 0)) {
				hash = HASH_L2BRIDGE_WC(pcmd->destaddr, input_interface);
			}
			else {
				hash = HASH_L2BRIDGE(pcmd->destaddr, ethertype_bigendian, input_interface);
			}

			if((pEntry = l2bridge_find_entry(hash, pcmd->destaddr, ethertype_bigendian, vlanid_bigendian, session_id)))
			{
				ackstatus = ERR_BRIDGE_ENTRY_ALREADY_EXISTS;
				break;
			}

			if ((pEntry = (PL2Bridge_entry)l2bridge_alloc()) == NULL)
			{
				ackstatus = ERR_NOT_ENOUGH_MEMORY;
				break;
			}
			memset(pEntry, 0, sizeof(L2Bridge_entry));

			SFL_memcpy(pEntry->da, pcmd->destaddr, 2 * ETHER_ADDR_LEN);
			pEntry->ethertype = ethertype_bigendian;
			pEntry->session_id = session_id;
			pEntry->pkt_priority = pcmd->pkt_priority == 0x8000 ? 0xFF : pcmd->pkt_priority;
			if (pcmd->vlan_priority == 0x8000)
				pEntry->vlan_priority_mark = 1;
			else 
				pEntry->vlan_priority = pcmd->vlan_priority;

			pEntry->input_interface = input_interface;
			pEntry->input_vlan= vlanid_bigendian;
			pEntry->output_itf = output_itf;
			pEntry->output_vlan = htons(pcmd->output_vlan);
			pEntry->qmod = pcmd->qmod;
			if (TESTEQ_NULL_MACADDR(pcmd->srcaddr))
				pEntry->sa_wc = TRUE;

			l2bridge_add(pEntry, hash);

			break;
		}
		case CMD_RX_L2BRIDGE_REMOVE: {
			U32 hash;
			U16 ethertype_bigendian;
			U16 vlanid_bigendian;
			U16 session_id;
			PL2Bridge_entry pEntry;
			PL2BridgeRemoveEntryCommand pcmd = (PL2BridgeRemoveEntryCommand)p;
			ethertype_bigendian = htons(pcmd->ethertype);
			vlanid_bigendian = htons(pcmd->input_vlan);
			session_id = htons(pcmd->session_id);

			//This can be served for WiFi and tunnel interfaces
			if( pcmd->input_interface > 1 )
			{
				POnifDesc pOnif;

				if((pOnif = get_onif_by_name(pcmd->input_name)) != NULL) {

					pcmd->input_interface = itf_get_phys_port(pOnif->itf);
				}
			}

			if (TESTEQ_NULL_MACADDR(pcmd->srcaddr) || (pcmd->ethertype == 0)) {
				hash = HASH_L2BRIDGE_WC(pcmd->destaddr, pcmd->input_interface);
			}
			else {
				hash = HASH_L2BRIDGE(pcmd->destaddr, ethertype_bigendian, pcmd->input_interface);
			}

			if((pEntry = l2bridge_find_entry(hash, pcmd->destaddr, ethertype_bigendian, vlanid_bigendian, session_id)) == NULL)
			{
				ackstatus = CMD_ERR;
				break;
			}

			l2bridge_remove(pEntry, hash);

			break;
		}
		case CMD_RX_L2BRIDGE_QUERY_STATUS: {

			static int bridge_interface_index = 0;
			PL2BridgeQueryStatusResponse prsp = (PL2BridgeQueryStatusResponse)p;
			
			while( bridge_interface_index < MAX_PHY_PORTS )
			{
				if( L2bridge_enabled[bridge_interface_index].used )
					break;

				bridge_interface_index++;
			}				
			
			if(bridge_interface_index < MAX_PHY_PORTS)
			{
				prsp->status = L2bridge_enabled[bridge_interface_index].enabled;
				SFL_memcpy(prsp->ifname,  get_onif_name(L2bridge_enabled[bridge_interface_index].onif_index), INTERFACE_NAME_LENGTH);
				bridge_interface_index++;
				prsp->eof = 0;
			}
			else /*if (bridge_interface_index == MAX_PHY_PORTS)*/
			{
				prsp->eof = 1;
				bridge_interface_index = 0;
			}

			acklen = sizeof(L2BridgeQueryStatusResponse);
			
		
			/* This function just initializes static variables for next queries*/
			rx_Get_Next_Hash_BridgeEntry((PL2BridgeQueryEntryResponse)p, 1);
			break;
		}
		case CMD_RX_L2BRIDGE_QUERY_ENTRY: 
		{
			
			int rc;
			PL2BridgeQueryEntryResponse prsp = (PL2BridgeQueryEntryResponse)p;
			rc = rx_Get_Next_Hash_BridgeEntry (prsp , 0);
			if (rc != NO_ERR)
				prsp->eof = 1;
			else
				prsp->eof = 0;
			acklen = sizeof(L2BridgeQueryEntryResponse);
			break;
		}
#if 0		
		case CMD_RX_L2BRIDGE_FLOW_ENTRY:
			action = *p;
			ackstatus = M_bridge_handle_l2flow(p, cmd_len);
			if(ackstatus == NO_ERR && ((action == ACTION_QUERY) || (action == ACTION_QUERY_CONT)))
				acklen += sizeof(L2BridgeL2FlowEntryCommand);
			break;
		
		case CMD_RX_L2BRIDGE_FLOW_TIMEOUT:
		case CMD_RX_L2BRIDGE_MODE: 
			ackstatus = M_bridge_handle_control(cmd_code, p, cmd_len);
			break;

		case CMD_RX_L2BRIDGE_FLOW_RESET:
			ackstatus = M_bridge_handle_reset();
			break;
#endif
		default:
			ackstatus = ERR_UNKNOWN_COMMAND;
			break;
	}
exit:
	*p = ackstatus;
	return acklen;
}


#if !defined (COMCERTO_2000)
int bridge_init(void)
{
	set_event_handler(EVENT_BRIDGE, M_bridge_entry);

	set_cmd_handler(EVENT_BRIDGE, M_bridge_cmdproc);

	timer_init(&bridge_timer, M_bridge_timer);

	L2Bridge_mode = L2_BRIDGE_MODE_MANUAL;
	L2Bridge_timeout = L2_BRIDGE_DEFAULT_TIMEOUT * L2_BRIDGE_TICKS_PER_SECOND;
	L2Bridge_bin_number = 0;

	L2Bridge_l3_bin_number = 0;

	return 0;
}

void bridge_exit(void)
{
}


#else

int bridge_interface_deregister( U16 phy_port_id )
{
	if( (phy_port_id >= MAX_PHY_PORTS) || !L2bridge_enabled[phy_port_id].used )
		return -1;

	L2bridge_enabled[phy_port_id].enabled = 0;
	L2bridge_enabled[phy_port_id].used = 0;

	return 0;
}

int bridge_interface_register( U8 *name, U16 phy_port_id )
{
	POnifDesc pOnif;

	if( (phy_port_id >= MAX_PHY_PORTS) || L2bridge_enabled[phy_port_id].used )
		return -1;

	pOnif = get_onif_by_name(name);
	if (!pOnif)
		return -1;

	L2bridge_enabled[phy_port_id].onif_index = (U16)get_onif_index(pOnif);

	L2bridge_enabled[phy_port_id].enabled = 0;
	L2bridge_enabled[phy_port_id].used = 1;

	return 0;
}


int bridge_init(void)
{
	int i;
	struct pfe_ctrl *ctrl = &pfe->ctrl;

	bridge_dma_pool = ctrl->dma_pool_512;

	set_cmd_handler(EVENT_BRIDGE, M_bridge_cmdproc);

	for (i = 0; i < NUM_BT_ENTRIES; i++)
	{
		slist_head_init(&bridge_cache[i]);
		dlist_head_init(&hw_bridge_active_list[i]);
	}

	dlist_head_init(&hw_bridge_removal_list);

	timer_init(&bridge_delayed_removal_timer, hw_l2bridge_delayed_remove);
	timer_add(&bridge_delayed_removal_timer, CT_TIMER_INTERVAL);

	//FIXME: Auto Bridge support
	//timer_init(&bridge_timer, M_bridge_timer);

	L2Bridge_mode = L2_BRIDGE_MODE_MANUAL;
	L2Bridge_timeout = L2_BRIDGE_DEFAULT_TIMEOUT * L2_BRIDGE_TICKS_PER_SECOND;
	L2Bridge_bin_number = 0;
	L2Bridge_l3_bin_number = 0;

	return 0;
}


void bridge_exit(void)
{
	struct dlist_head *entry;
	struct _thw_L2Bridge_entry *hw_l2bridge;

	timer_del(&bridge_delayed_removal_timer);
	//FIXME: Auto Bridge support
	//timer_del(&bridge_timer);

	l2bridge_manual_reset();

	dlist_for_each_safe(hw_l2bridge, entry, &hw_bridge_removal_list, list)
	{
		dlist_remove(&hw_l2bridge->list);
		dma_pool_free(bridge_dma_pool, hw_l2bridge, be32_to_cpu(hw_l2bridge->dma_addr));
	}
}
#endif




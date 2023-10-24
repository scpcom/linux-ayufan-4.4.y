#include "module_mc4.h"
#include "module_mc6.h"
#include "layer2.h"
#include "fe.h"


/* This function returns total multicast entries 
   configured in a given hash index */
static int MC4_Get_Hash_Entries(int mc4_hash_index)
{
	
	int tot_mc4_entries = 0;
	struct slist_entry *entry;
	
	slist_for_each_entry(entry, &mc4_table_memory[mc4_hash_index])
	{
	    tot_mc4_entries++;
	 }
		
		
	return tot_mc4_entries;

}


/* This function fills the snapshot of MC4 entries in a given hash index */
static int MC4_Get_Hash_Snapshot(int mc4_hash_index, int mc4_tot_entries, PMC4Command pMC4Snapshot)
{
	
	int tot_mc4_entries = 0, i;
	PMC4Entry  pMC4Entry;
	struct slist_entry *entry;

	slist_for_each(pMC4Entry, entry, &mc4_table_memory[mc4_hash_index], list)
	{
		memset(pMC4Snapshot, sizeof(MC4Command), 0);
		pMC4Snapshot->src_addr  	= pMC4Entry->src_addr;
		pMC4Snapshot->src_addr_mask	= pMC4Entry->mcdest.src_mask_len;
		pMC4Snapshot->dst_addr 		= pMC4Entry->dst_addr;
		pMC4Snapshot->queue = pMC4Entry->mcdest.queue_base;
		pMC4Snapshot->num_output = pMC4Entry->mcdest.num_listeners;
		for(i = 0; i < pMC4Entry->mcdest.num_listeners; i++)
		{
			unsigned short output_index;
			POnifDesc pOnif;
			output_index = pMC4Entry->mcdest.listeners[i].output_index;
			pOnif = get_onif_by_index(output_index);
			strcpy((char *)pMC4Snapshot->output_list[i].output_device_str, (char *)pOnif->name);	
			pMC4Snapshot->output_list[i].timer = pMC4Entry->mcdest.listeners[i].timer;
		}
		if ((pMC4Entry->mcdest.flags & MC_ACP_LISTENER) && i < MC4_MAX_LISTENERS_PER_GROUP)
		{
			strcpy((char *)pMC4Snapshot->output_list[i].output_device_str, "ACP");
			pMC4Snapshot->output_list[i].timer = pMC4Entry->mcdest.wifi_listener_timer;
		}
		pMC4Snapshot++;
		tot_mc4_entries++;
		mc4_tot_entries--;
		if (mc4_tot_entries == 0)
			break;
	}
		
	return tot_mc4_entries;

}


/* This function creates the snapshot memory and returns the 
   next MC4 entry from the snapshot of the MC4 entries of a
   single hash to the caller  */
   
int MC4_Get_Next_Hash_Entry(PMC4Command pMC4Cmd, int reset_action)
{
	int mc4_hash_entries;
	PMC4Command pMC4;
	static PMC4Command pMC4Snapshot = NULL;
	static int mc4_hash_index = 0, mc4_snapshot_entries =0, mc4_snapshot_index=0, mc4_snapshot_buf_entries = 0;
	
	if(reset_action)
	{
		mc4_hash_index = 0;
		mc4_snapshot_entries =0;
		mc4_snapshot_index=0;
		if(pMC4Snapshot)
		{
			Heap_Free(pMC4Snapshot);
			pMC4Snapshot = NULL;
		}
		mc4_snapshot_buf_entries = 0;
	}
	
	if (mc4_snapshot_index == 0)
	{
		
		while( mc4_hash_index <  MC4_NUM_HASH_ENTRIES)
		{
		
			mc4_hash_entries = MC4_Get_Hash_Entries(mc4_hash_index);
			if(mc4_hash_entries == 0)
			{
				mc4_hash_index++;
				continue;
			}
		   	
		   	if(mc4_hash_entries > mc4_snapshot_buf_entries)
		   	{
		   		if(pMC4Snapshot)
		   			Heap_Free(pMC4Snapshot);
				pMC4Snapshot = Heap_Alloc(mc4_hash_entries * sizeof(MC4Command));
			
				if (!pMC4Snapshot)
				{
			    	mc4_hash_index = 0;
			    	mc4_snapshot_buf_entries = 0;
					return ERR_NOT_ENOUGH_MEMORY;
				}
				mc4_snapshot_buf_entries = mc4_hash_entries;
		   	}
				
			mc4_snapshot_entries = MC4_Get_Hash_Snapshot(mc4_hash_index ,mc4_hash_entries, pMC4Snapshot);
			
			break;
		}
		
		if (mc4_hash_index >= MC4_NUM_HASH_ENTRIES)
		{
			mc4_hash_index = 0;
			if(pMC4Snapshot)
			{
				Heap_Free(pMC4Snapshot);
				pMC4Snapshot = NULL;
			}
			mc4_snapshot_buf_entries = 0;
			return ERR_MC_ENTRY_NOT_FOUND;
		}
		   
	}
	
	pMC4 = &pMC4Snapshot[mc4_snapshot_index++];
	SFL_memcpy(pMC4Cmd, pMC4, sizeof(MC4Command));
	if (mc4_snapshot_index == mc4_snapshot_entries)
	{
		mc4_snapshot_index = 0;
		mc4_hash_index ++;
	}
		
	
	return NO_ERR;	
		
}


/* This function returns total multicastv6 entries 
   configured in a given hash index */
static int MC6_Get_Hash_Entries(int mc6_hash_index)
{
	
	int tot_mc6_entries = 0;
	struct slist_entry *entry;
	
	
	slist_for_each_entry(entry, &mc6_table_memory[mc6_hash_index])
	{
	    tot_mc6_entries++;
	 }
		
	return tot_mc6_entries;

}


/* This function fills the snapshot of MC6 entries in a given hash index */
static int MC6_Get_Hash_Snapshot(int mc6_hash_index, int mc6_tot_entries, PMC6Command pMC6Snapshot)
{
	
	int tot_mc6_entries = 0,i;
	PMC6Entry  pMC6Entry;
	struct slist_entry *entry;

	slist_for_each(pMC6Entry, entry, &mc6_table_memory[mc6_hash_index], list)
	{
		memset(pMC6Snapshot, sizeof(MC6Command), 0);
		SFL_memcpy(pMC6Snapshot->src_addr, pMC6Entry->src_addr, IPV6_ADDRESS_LENGTH);
		pMC6Snapshot->src_mask_len	= pMC6Entry->mcdest.src_mask_len;
		SFL_memcpy(pMC6Snapshot->dst_addr, pMC6Entry->dst_addr, IPV6_ADDRESS_LENGTH);
		pMC6Snapshot->queue = pMC6Entry->mcdest.queue_base;
		pMC6Snapshot->num_output = pMC6Entry->mcdest.num_listeners;
		for(i = 0; i < pMC6Entry->mcdest.num_listeners; i++)
		{
			unsigned short output_index;
			POnifDesc pOnif;
			output_index = pMC6Entry->mcdest.listeners[i].output_index;
			pOnif = get_onif_by_index(output_index);
			strcpy((char *)pMC6Snapshot->output_list[i].output_device_str, (char *)pOnif->name);	
			pMC6Snapshot->output_list[i].timer = pMC6Entry->mcdest.listeners[i].timer;
		}
		if ((pMC6Entry->mcdest.flags & MC_ACP_LISTENER) && i < MC6_MAX_LISTENERS_PER_GROUP)
		{
			strcpy((char *)pMC6Snapshot->output_list[i].output_device_str, "ACP");
			pMC6Snapshot->output_list[i].timer = pMC6Entry->mcdest.wifi_listener_timer;
		}
		pMC6Snapshot++;
		tot_mc6_entries++;
		mc6_tot_entries--;
		if (mc6_tot_entries == 0)
			break;
	}

	return tot_mc6_entries;

}


/* This function creates the snapshot memory and returns the 
   next MC6 entry from the snapshot of the MC4 entries of a
   single hash to the caller  */
   
int MC6_Get_Next_Hash_Entry(PMC6Command pMC6Cmd, int reset_action)
{
	int mc6_hash_entries;
	PMC6Command pMC6;
	static PMC6Command pMC6Snapshot = NULL;
	static int mc6_hash_index = 0, mc6_snapshot_entries =0, mc6_snapshot_index=0, mc6_snapshot_buf_entries = 0;
	
	if(reset_action)
	{
		mc6_hash_index = 0;
		mc6_snapshot_entries =0;
		mc6_snapshot_index=0;
		if(pMC6Snapshot)
		{
			Heap_Free(pMC6Snapshot);
			pMC6Snapshot = NULL;	
		}
		mc6_snapshot_buf_entries = 0;
	}
	
	if (mc6_snapshot_index == 0)
	{
		while( mc6_hash_index <  MC6_NUM_HASH_ENTRIES)
		{
		
			mc6_hash_entries = MC6_Get_Hash_Entries(mc6_hash_index);
			if(mc6_hash_entries == 0)
			{
				mc6_hash_index++;
				continue;
			}
		   	
		   	if(mc6_hash_entries > mc6_snapshot_buf_entries)
		   	{
		   		if(pMC6Snapshot)
		   			Heap_Free(pMC6Snapshot);
				pMC6Snapshot = Heap_Alloc(mc6_hash_entries * sizeof(MC6Command));
			
				if (!pMC6Snapshot)
				{
			    	mc6_hash_index = 0;
			    	mc6_snapshot_buf_entries = 0;
					return ERR_NOT_ENOUGH_MEMORY;
				}
				mc6_snapshot_buf_entries = mc6_hash_entries;
		   	}
		
			mc6_snapshot_entries = MC6_Get_Hash_Snapshot(mc6_hash_index , mc6_hash_entries, pMC6Snapshot);
				
			break;
		}
		
		if (mc6_hash_index >= MC6_NUM_HASH_ENTRIES)
		{
			mc6_hash_index = 0;
			if(pMC6Snapshot)
			{
				Heap_Free(pMC6Snapshot);
				pMC6Snapshot = NULL;	
			}
			mc6_snapshot_buf_entries =  0;
			return ERR_MC_ENTRY_NOT_FOUND;
		}
		   
	}
	
	pMC6 = &pMC6Snapshot[mc6_snapshot_index++];
	SFL_memcpy(pMC6Cmd, pMC6, sizeof(MC6Command));
	if (mc6_snapshot_index == mc6_snapshot_entries)
	{
		mc6_snapshot_index = 0;
		mc6_hash_index ++;
	}
		
	
	return NO_ERR;	
		
}

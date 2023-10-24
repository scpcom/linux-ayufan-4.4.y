#include "module_tunnel.h"

static int Tnl_Get_Hash_Entries(void)
{

        int tot_tnls=0, i;
        PTnlEntry pTnlEntry;

	for (i = 0; i < TNL_MAX_TUNNEL; i++)
	{
		pTnlEntry = &gTNLCtx.tunnel_table[i];
		if (pTnlEntry->state != TNL_STATE_FREE)
			tot_tnls++;
	}

        return tot_tnls;
}


/* This function fills in the snapshot of all Vlan entries of a VLAN cache */

static int Tnl_Get_Hash_Snapshot(int tnl_entries, PTNLCommand_query pTnlSnapshot)
{
        int tot_tnls=0, i;
        PTnlEntry pTnlEntry;

        for (i = 0; i < TNL_MAX_TUNNEL; i++)
      	{
        	pTnlEntry = &gTNLCtx.tunnel_table[i];
		if (pTnlEntry->state == TNL_STATE_FREE)
			continue;

		memset(pTnlSnapshot , 0, sizeof(TNLCommand_query));
		pTnlSnapshot->mode = pTnlEntry->mode;
		pTnlSnapshot->secure = pTnlEntry->secure;
		SFL_memcpy(pTnlSnapshot->name, get_onif_name(pTnlEntry->itf.index), 16);
		SFL_memcpy(pTnlSnapshot->local, pTnlEntry->local, IPV6_ADDRESS_LENGTH);
		SFL_memcpy(pTnlSnapshot->remote, pTnlEntry->remote, IPV6_ADDRESS_LENGTH);
		pTnlSnapshot->fl=pTnlEntry->fl;
		pTnlSnapshot->frag_off = pTnlEntry->frag_off;
		pTnlSnapshot->enabled = pTnlEntry->state;
		pTnlSnapshot->elim = pTnlEntry->elim;
		pTnlSnapshot->hlim = pTnlEntry->hlim;
		
              pTnlSnapshot++;
              tot_tnls++;
              tnl_entries--;
		if (tnl_entries == 0)
			break;
        }

        return tot_tnls;
}

U16 Tnl_Get_Next_Hash_Entry(PTNLCommand_query pTnlCmd, int reset_action)
{
	int total_tnl_entries;
       PTNLCommand_query pTnl;
       static PTNLCommand_query pTnlSnapshot= NULL;
       static int tnl_snapshot_entries =0, tnl_snapshot_index=0;

        if(reset_action)
        {
                tnl_snapshot_entries =0;
                tnl_snapshot_index=0;
                if(pTnlSnapshot)
                {
                        Heap_Free(pTnlSnapshot);
                        pTnlSnapshot = NULL;
                }
                
        }

    	if (tnl_snapshot_index == 0)
    	{
		total_tnl_entries = Tnl_Get_Hash_Entries();
		if (total_tnl_entries == 0)
			return ERR_TNL_ENTRY_NOT_FOUND;
            
               
		if(pTnlSnapshot)
		{
			Heap_Free(pTnlSnapshot);
			pTnlSnapshot = NULL;
		}

 		pTnlSnapshot = Heap_Alloc(total_tnl_entries * sizeof(TNLCommand_query));
		if (!pTnlSnapshot)
                         return ERR_NOT_ENOUGH_MEMORY;


		 tnl_snapshot_entries = Tnl_Get_Hash_Snapshot(total_tnl_entries,pTnlSnapshot);

	}

             
        if (tnl_snapshot_index == tnl_snapshot_entries)
        {
        	if(pTnlSnapshot)
      		{
			Heap_Free(pTnlSnapshot);
			pTnlSnapshot = NULL;
      		}
			 
 		tnl_snapshot_index = 0;
            	return ERR_TNL_ENTRY_NOT_FOUND;
        }
		
	 pTnl = &pTnlSnapshot[tnl_snapshot_index++];

        SFL_memcpy(pTnlCmd, pTnl, sizeof(TNLCommand_query));

        return NO_ERR;
}

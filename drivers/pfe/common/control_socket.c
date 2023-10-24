/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#include "module_socket.h"
#ifdef COMCERTO_1000
#include "module_msp.h"
#endif

#if defined(COMCERTO_2000)
#include "control_common.h"

PVOID CLASS_PE_LMEM_SH2(sock4_cache)[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));
PVOID CLASS_PE_LMEM_SH2(sock6_cache)[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));
PVOID CLASS_PE_LMEM_SH2(sockid_cache)[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));

PVOID UTIL_DMEM_SH2(sock4_cache)[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));
PVOID UTIL_DMEM_SH2(sock6_cache)[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));
PVOID UTIL_DMEM_SH2(sockid_cache)[NUM_SOCK_ENTRIES] __attribute__((aligned(32)));

struct dma_pool *socket_dma_pool;

extern TIMER_ENTRY socket_timer;
struct dlist_head hw_sock_removal_list;						/* common list to maintain hw socket under delayed removal state */
struct dlist_head hw_sock_active_list[NUM_SOCK_ENTRIES];	/* list to maintain active hw sockets */
struct dlist_head hw_sock_active_list_id[NUM_SOCK_ENTRIES];	/* list to maintain active hw sockets by socket's ID */
#endif

static void SOCKET4_delete_route(PSockEntry pSocket)
{
	L2_route_put(pSocket->pRtEntry);

	pSocket->pRtEntry = NULL;
}

static void SOCKET6_delete_route(PSock6Entry pSocket)
{
	L2_route_put(pSocket->pRtEntry);

	pSocket->pRtEntry = NULL;
}

BOOL SOCKET4_check_route(PSockEntry pSocket)
{
	PRouteEntry pRtEntry;

	pRtEntry = L2_route_get(pSocket->route_id);
	if (pRtEntry == NULL)
		return FALSE;

	pSocket->pRtEntry = pRtEntry;

	return TRUE;
}

BOOL SOCKET6_check_route(PSock6Entry pSocket)
{
	PRouteEntry pRtEntry;

	pRtEntry = L2_route_get(pSocket->route_id);
	if (pRtEntry == NULL)
		return FALSE;

	pSocket->pRtEntry = pRtEntry;

	return TRUE;
}



#if defined(COMCERTO_2000)

/** Update PE's internal memory socket cache tables with the HW entry's DDR address
*
* @entry_ddr		DDR physical address of the hw socket entry
*
*/
static void socket_add_to_class(U32 host_addr, U32 *pe_addr)
{
	class_bus_write(host_addr, virt_to_class_pe_lmem(pe_addr), 4);
}


static void socket_add_to_util(U32 host_addr, U32 *pe_addr)
{
	pe_dmem_write(UTIL_ID, host_addr, virt_to_util_dmem(pe_addr), 4);
}


static void socket_update_pe_sockid_cache(U32 host_addr, U32 hash)
{
	U32 *pe_addr;

	pe_addr = (U32*)&class_sockid_cache[hash];
	socket_add_to_class(host_addr, pe_addr);

	pe_addr = (U32*)&util_sockid_cache[hash];
	socket_add_to_util(host_addr, pe_addr);
}

static void socket_update_pe_sock4_cache(U32 host_addr, U32 hash)
{
	U32 *pe_addr;

	pe_addr = (U32*)&class_sock4_cache[hash];
	socket_add_to_class(host_addr, pe_addr);

	pe_addr = (U32*)&util_sock4_cache[hash];
	socket_add_to_util(host_addr, pe_addr);
}

static void socket_update_pe_sock6_cache(U32 host_addr, U32 hash)
{
	U32 *pe_addr;

	pe_addr = (U32*)&class_sock6_cache[hash];
	socket_add_to_class(host_addr, pe_addr);

	pe_addr = (U32*)&util_sock6_cache[hash];
	socket_add_to_util(host_addr, pe_addr);
}

void socket4_update(PSockEntry pSocket)
{
	struct _thw_sock4 *hw_sock = pSocket->hw_sock;

	if(!pSocket->hw_sock)
		return;

	if(!pSocket->pRtEntry)
		SOCKET4_check_route(pSocket);

	hw_entry_set_flags(&hw_sock->flags, SOCK_UPDATING | SOCK_VALID);
	
	hw_sock->queue = cpu_to_be16(pSocket->queue);
	hw_sock->dscp = cpu_to_be16(pSocket->dscp);
	hw_sock->SocketType = pSocket->SocketType;
	hw_sock->SocketFamily = pSocket->SocketFamily;
	hw_sock->SocketID = cpu_to_be16(pSocket->SocketID);
	hw_sock->owner_type = cpu_to_be16(pSocket->owner_type);
	hw_sock->Sport = pSocket->Sport;
	hw_sock->Dport = pSocket->Dport;
	hw_sock->proto = pSocket->proto;
	hw_sock->connected = pSocket->connected;
	hw_sock->Saddr = pSocket->Saddr;
	hw_sock->Daddr = pSocket->Daddr;
	hw_sock->initial_takeover_done = pSocket->initial_takeover_done;
	hw_sock->hash = cpu_to_be16(pSocket->hash);
	hw_sock->hash_by_id = cpu_to_be16(pSocket->hash_by_id);

	if(pSocket->pRtEntry)
	{
		hw_sock->route.itf = cpu_to_be32(virt_to_class_dmem(pSocket->pRtEntry->itf));
		memcpy(hw_sock->route.dstmac, pSocket->pRtEntry->dstmac, ETHER_ADDR_LEN);
		hw_sock->route.mtu = cpu_to_be16(pSocket->pRtEntry->mtu);
		hw_sock->route.dAddr = pSocket->pRtEntry->dAddr;
	}
	else
	{
		/* mark route as not valid */
		hw_sock->route.itf = 0xffffffff;
	}

	hw_entry_set_flags(&hw_sock->flags, SOCK_VALID);
}


void socket6_update(PSock6Entry pSocket)
{
	struct _thw_sock6 *hw_sock = pSocket->hw_sock;
	
	if(!pSocket->hw_sock)
		return;
	
	hw_entry_set_flags(&hw_sock->flags, SOCK_UPDATING | SOCK_VALID);

	hw_sock->queue = cpu_to_be16(pSocket->queue);
	hw_sock->dscp = cpu_to_be16(pSocket->dscp);
	hw_sock->SocketType = pSocket->SocketType;
	hw_sock->SocketFamily = pSocket->SocketFamily;
	hw_sock->SocketID = cpu_to_be16(pSocket->SocketID);
	hw_sock->owner_type = cpu_to_be16(pSocket->owner_type);
	hw_sock->Sport = pSocket->Sport;
	hw_sock->Dport = pSocket->Dport;
	hw_sock->proto = pSocket->proto;
	hw_sock->connected = pSocket->connected;
	hw_sock->initial_takeover_done = pSocket->initial_takeover_done;
	hw_sock->hash = cpu_to_be16(pSocket->hash);
	hw_sock->hash_by_id = cpu_to_be16(pSocket->hash_by_id);

	SFL_memcpy(hw_sock->Saddr, pSocket->Saddr, IPV6_ADDRESS_LENGTH);
	SFL_memcpy(hw_sock->Daddr, pSocket->Daddr, IPV6_ADDRESS_LENGTH);

	if(pSocket->pRtEntry)
	{
		hw_sock->route.itf = cpu_to_be32(virt_to_class_dmem(pSocket->pRtEntry->itf));
		memcpy(hw_sock->route.dstmac, pSocket->pRtEntry->dstmac, ETHER_ADDR_LEN);
		hw_sock->route.mtu = cpu_to_be16(pSocket->pRtEntry->mtu);
		hw_sock->route.dAddr = pSocket->pRtEntry->dAddr;
	}
	else
	{
		/* mark route as not valid */
		hw_sock->route.itf = 0xffffffff;
	}

	hw_entry_set_flags(&hw_sock->flags, SOCK_VALID);
}

/** Schedules an hardware socket entry for removal.
* The entry is added to a removal list and it will be free later from a timer.
* The removal time must be bigger than the worst case PE processing time for tens of packets.
*
* @param hw_sock		pointer to the hardware socket entry
*
*/
static void hw_socket_schedule_remove(struct _thw_sock4 *hw_sock)
{
	hw_sock->removal_time = jiffies + 2;
	dlist_add(&hw_sock_removal_list, &hw_sock->list);
}


/** Processes hardware socket delayed removal list.
* Free all hardware socket in the removal list that have reached their removal time.
*
*
*/
static void hw_socket_delayed_remove(void)
{
	struct dlist_head *entry;
	struct _thw_sock4 *hw_sock;

	dlist_for_each_safe(hw_sock, entry, &hw_sock_removal_list, list)
	{
		if (!time_after(jiffies, hw_sock->removal_time))
			continue;

		dlist_remove(&hw_sock->list);

		dma_pool_free(socket_dma_pool, hw_sock, be32_to_cpu(hw_sock->dma_addr));
	}
}

static PSockEntry socket4_alloc(void)
{
	return kzalloc(sizeof(SockEntry), GFP_KERNEL);
}

static void socket4_free(PSockEntry pSocket)
{
	kfree(pSocket);
}

/** Adds a software socket entry  to the local hash and hardware hash (making it visible to PFE).
*
* @param pEntry		pointer to the software socket
* @param hash		hash index where to add the socket
*
* @return		NO_ERR in case of success, ERR_xxx in case of error
*/
static int socket4_add(PSockEntry pSocket)
{
	struct _thw_sock4 *hw_sock;
	struct _thw_sock4 *hw_sock_first;
	int rc;
	dma_addr_t dma_addr;

	/* Allocate hardware entry */
	hw_sock = dma_pool_alloc(socket_dma_pool, GFP_ATOMIC, &dma_addr);
	if (!hw_sock)
	{
		rc = ERR_NOT_ENOUGH_MEMORY;
		goto err;
	}

	hw_sock->dma_addr = cpu_to_be32(dma_addr);
	memset(hw_sock->SocketStats, 0, SOCKET_STATS_SIZE);

	/* Link software conntrack to hardware conntrack */
	pSocket->hw_sock = hw_sock;
	hw_sock->sw_sock = pSocket;

	/* add hw entry to active list and update next pointer */
	if(!dlist_empty(&hw_sock_active_list[pSocket->hash]))
	{
		/* list is not empty, and we'll be added at head, so current first will become our next pointer */
		hw_sock_first = container_of(dlist_first(&hw_sock_active_list[pSocket->hash]), typeof(struct _thw_sock4), list);
		hw_entry_set_field(&hw_sock->next, hw_entry_get_field(&hw_sock_first->dma_addr));
	}
	else 
	{
        /* entry is empty, so we'll be the first and only one entry */
		hw_entry_set_field(&hw_sock->next, 0);
	}
	dlist_add(&hw_sock_active_list[pSocket->hash], &hw_sock->list);

	/* idem for the by id list */
	if(!dlist_empty(&hw_sock_active_list_id[pSocket->hash_by_id]))
	{
		hw_sock_first = container_of(dlist_first(&hw_sock_active_list_id[pSocket->hash_by_id]), typeof(struct _thw_sock4), list_id);
		hw_entry_set_field(&hw_sock->nextid, hw_entry_get_field(&hw_sock_first->dma_addr));
	}
	else
	{
		hw_entry_set_field(&hw_sock->nextid, 0);
	}
	dlist_add(&hw_sock_active_list_id[pSocket->hash_by_id], &hw_sock->list_id);

	/* fill in the hw entry */
	socket4_update(pSocket);

	/* Update PE's internal memory socket cache tables with the HW entry's DDR address */
	socket_update_pe_sock4_cache(hw_sock->dma_addr, pSocket->hash);
	socket_update_pe_sockid_cache(hw_sock->dma_addr, pSocket->hash_by_id);

	/* Add software entry to local hash */
	slist_add(&sock4_cache[pSocket->hash], &pSocket->list);
	slist_add(&sockid_cache[pSocket->hash_by_id], &pSocket->list_id);

	return NO_ERR;

err:
	socket4_free(pSocket);
	
	return rc;
}


/** Removes a software socket entry from the local hash and hardware hash
* The hardware socket is marked invalid/in use and scheduled to delayed removal.
* The software socket is removed immediately from the local hash.
*
* @param pEntry		pointer to the software socket
* @param hash		hash index where to remove the socket
*
*/
static void socket4_remove(PSockEntry pSocket, U32 hash, U32 hash_by_id)
{
	struct _thw_sock4 *hw_sock;
	struct _thw_sock4 *hw_sock_prev;

	/* Check if there is a hardware socket */
	if ((hw_sock = pSocket->hw_sock))
	{
		/* detach from software socket */
		pSocket->hw_sock = NULL;

		/* if the removed entry is first in hash slot then only PE dmem hash need to be updated */
		if (&hw_sock->list == dlist_first(&hw_sock_active_list[hash])) 
		{	
			socket_update_pe_sock4_cache(hw_entry_get_field(&hw_sock->next), hash);
		}
		else
		{
			hw_sock_prev = container_of(hw_sock->list.prev, typeof(struct _thw_sock4), list);
			hw_entry_set_field(&hw_sock_prev->next, hw_entry_get_field(&hw_sock->next));
		}
		dlist_remove(&hw_sock->list);

		if (&hw_sock->list_id == dlist_first(&hw_sock_active_list_id[hash_by_id])) 	
		{
			socket_update_pe_sockid_cache(hw_entry_get_field(&hw_sock->nextid), hash_by_id);
		}
		else
		{
			hw_sock_prev = container_of(hw_sock->list_id.prev, typeof(struct _thw_sock4), list_id);
			hw_entry_set_field(&hw_sock_prev->nextid, hw_entry_get_field(&hw_sock->nextid));
		}
		dlist_remove(&hw_sock->list_id);

		/* now switching hw entry from active to delayed removal list */
		hw_socket_schedule_remove(hw_sock);
	}

	/* destroy sw socket entry */
	SOCKET4_delete_route(pSocket);

	/* Unlink from software list */
	slist_remove(&sock4_cache[hash], &pSocket->list);
	slist_remove(&sockid_cache[hash_by_id], &pSocket->list_id);

	socket4_free(pSocket);
}

static PSock6Entry socket6_alloc(void)
{
	return kzalloc(sizeof(Sock6Entry), GFP_KERNEL);
}

static void socket6_free(PSock6Entry pSocket)
{
	kfree(pSocket);
}

static int socket6_add(PSock6Entry pSocket)
{
	struct _thw_sock6 *hw_sock;
	struct _thw_sock6 *hw_sock_first;
	int rc;
	dma_addr_t dma_addr;

	/* Allocate hardware entry */
	hw_sock = dma_pool_alloc(socket_dma_pool, GFP_ATOMIC, &dma_addr);
	if (!hw_sock)
	{
		rc = ERR_NOT_ENOUGH_MEMORY;
		goto err;
	}

	hw_sock->dma_addr = cpu_to_be32(dma_addr);
	memset(hw_sock->SocketStats, 0, SOCKET_STATS_SIZE);

	/* Link software conntrack to hardware conntrack */
	pSocket->hw_sock = hw_sock;
	hw_sock->sw_sock = pSocket;

	/* add hw entry to active list and update next pointer */
	if(!dlist_empty(&hw_sock_active_list[pSocket->hash]))
	{
		/* list is not empty, and we'll be added at head, so current first will become our next pointer */
		hw_sock_first = container_of(dlist_first(&hw_sock_active_list[pSocket->hash]), typeof(struct _thw_sock6), list);
		hw_entry_set_field(&hw_sock->next, hw_entry_get_field(&hw_sock_first->dma_addr));
	}
	else 
	{
		/* entry is empty, so we'll be the first and only one entry */
		hw_entry_set_field(&hw_sock->next, 0);
	}
	dlist_add(&hw_sock_active_list[pSocket->hash], &hw_sock->list);

	/* idem for the by id list */
	if(!dlist_empty(&hw_sock_active_list_id[pSocket->hash_by_id]))
	{
		hw_sock_first = container_of(dlist_first(&hw_sock_active_list_id[pSocket->hash_by_id]), typeof(struct _thw_sock6), list_id);
		hw_entry_set_field(&hw_sock->nextid, hw_entry_get_field(&hw_sock_first->dma_addr));
	}
	else
	{
		hw_entry_set_field(&hw_sock->nextid, 0);
	}
	dlist_add(&hw_sock_active_list_id[pSocket->hash_by_id], &hw_sock->list_id);

	/* fill in the hw entry */
	socket6_update(pSocket);

	/* Update PE's internal memory socket cache tables with the HW entry's DDR address */
	socket_update_pe_sock6_cache(hw_sock->dma_addr, pSocket->hash);
	socket_update_pe_sockid_cache(hw_sock->dma_addr, pSocket->hash_by_id);

	slist_add(&sock6_cache[pSocket->hash], &pSocket->list);
	slist_add(&sockid_cache[pSocket->hash_by_id], &pSocket->list_id);

	return NO_ERR;

err:
	socket6_free(pSocket);

	return rc;
}




/** Removes a software socket entry from the local hash and hardware hash
* The hardware socket is marked invalid/in use and scheduled to delayed removal.
* The software socket is removed immediately from the local hash.
*
* @param pEntry		pointer to the software socket
* @param hash		hash index where to remove the socket
*
*/
static void socket6_remove(PSock6Entry pSocket, U32 hash, U32 hash_by_id)
{
	struct _thw_sock6 *hw_sock; 
	struct _thw_sock6 *hw_sock_prev;

	/* Check if there is a hardware socket */
	if ((hw_sock = pSocket->hw_sock))
	{
		 /* detach from software socket */
		 pSocket->hw_sock = NULL;

		/* if the removed entry is first in hash slot then only PE dmem hash need to be updated */
		if (&hw_sock->list == dlist_first(&hw_sock_active_list[hash])) 
		{
			socket_update_pe_sock6_cache(hw_entry_get_field(&hw_sock->next), hash);
		}	
		else
		{
			hw_sock_prev = container_of(hw_sock->list.prev, typeof(struct _thw_sock6), list);
			hw_entry_set_field(&hw_sock_prev->next, hw_entry_get_field(&hw_sock->next));
		}

		if (&hw_sock->list_id == dlist_first(&hw_sock_active_list_id[hash_by_id])) 	
		{
			socket_update_pe_sockid_cache(hw_entry_get_field(&hw_sock->nextid), hash_by_id);
		}		
		else
		{
			hw_sock_prev = container_of(hw_sock->list_id.prev, typeof(struct _thw_sock6), list_id);
			hw_entry_set_field(&hw_sock_prev->nextid, hw_entry_get_field(&hw_sock->nextid));
		}

		/* Unlink from hardware lists */
		dlist_remove(&hw_sock->list);
		dlist_remove(&hw_sock->list_id);
		
		/* now switching hw entry from active to delayed removal list */
		hw_socket_schedule_remove((struct _thw_sock4 *)hw_sock);
	}

	SOCKET6_delete_route(pSocket);

	/* Unlink from software list */
	slist_remove(&sock6_cache[hash], &pSocket->list);
	slist_remove(&sockid_cache[hash_by_id], &pSocket->list_id);

	/* Free local entry */
	socket6_free(pSocket);
}

#else

static PSockEntry socket4_alloc(void)
{
	return Heap_Alloc(sizeof(SockEntry));
}

static void socket4_free(PSockEntry pSocket)
{
	Heap_Free((PVOID)pSocket);
}

static int socket4_add(PSockEntry pSocket)
{
	slist_add(&sock4_cache[pSocket->hash], &pSocket->list);
	slist_add(&sockid_cache[pSocket->hash_by_id], &pSocket->list_id);

	return NO_ERR;
}

static void socket4_remove(PSockEntry pSocket, U32 hash, U32 hash_by_id)
{
	SOCKET4_delete_route(pSocket);

	slist_remove(&sock4_cache[hash], &pSocket->list);
	slist_remove(&sockid_cache[hash_by_id], &pSocket->list_id);

	socket4_free(pSocket);
}

static void socket4_update(PSockEntry pSocket)
{

}


static PSock6Entry socket6_alloc(void)
{
	return Heap_Alloc(sizeof(Sock6Entry));
}

static void socket6_free(PSock6Entry pSocket)
{
	Heap_Free((PVOID)pSocket);
}

static int socket6_add(PSock6Entry pSocket)
{
	slist_add(&sock6_cache[pSocket->hash], &pSocket->list);
	slist_add(&sockid_cache[pSocket->hash_by_id], &pSocket->list_id);

	return NO_ERR;
}

static void socket6_remove(PSock6Entry pSocket, U32 hash, U32 hash_by_id)
{
	SOCKET6_delete_route(pSocket);

	slist_remove(&sock6_cache[hash], &pSocket->list);
	slist_remove(&sockid_cache[hash_by_id], &pSocket->list_id);

	socket6_free(pSocket);
}

static void socket6_update(PSock6Entry pSocket)
{

}

#endif

PSockEntry SOCKET_bind(U16 socketID, PVOID owner, U16 owner_type)
{
 	PSockEntry pSocket = SOCKET_find_entry_by_id(socketID);

	if (pSocket) {
		pSocket->owner = owner;
		pSocket->owner_type = owner_type;

		/* update hardware socket */
		if(pSocket->SocketFamily == PROTO_IPV6)
			socket6_update((PSock6Entry)pSocket);
		else
			socket4_update(pSocket);
	}

	return pSocket;
}

PSockEntry SOCKET_unbind(U16 socketID)
{
	PSockEntry pSocket = SOCKET_find_entry_by_id(socketID);

	if (pSocket) {
		pSocket->owner = NULL;
		pSocket->owner_type = SOCK_OWNER_NONE;

		/* update hardware socket */
		if(pSocket->SocketFamily == PROTO_IPV6)
			socket6_update((PSock6Entry)pSocket);
		else
			socket4_update(pSocket);
	}
	
	return pSocket;
}

PSockEntry SOCKET_find_entry_by_id(U16 socketID)
{
	PSockEntry pEntry;
	PSockEntry pSocket = NULL;
	struct slist_entry *entry;
	int hash;

	hash = HASH_SOCKID(socketID);

	slist_for_each(pEntry, entry, &sockid_cache[hash], list_id)
	{
		if (pEntry->SocketID == socketID)
			pSocket = pEntry;
	}

	return pSocket;
}

PSockEntry SOCKET4_find_entry(U32 saddr, U16 sport, U32 daddr, U16 dport, U16 proto)
{
	PSockEntry pEntry;
	PSockEntry pSock3 = NULL;
	struct slist_entry *entry;
	U32 hash = HASH_SOCK(daddr, dport, proto);

	slist_for_each(pEntry, entry, &sock4_cache[hash], list)
	{
		if (pEntry->Daddr == daddr && pEntry->Dport == dport && pEntry->proto == proto) {
			if (pEntry->connected) {
				// check 5-tuples for connected sockets
				if (pEntry->Saddr == saddr && pEntry->Sport == sport) {
					return pEntry;
				}	
			}
			else // remind last 3 tuples match (should be unique)
				pSock3 = pEntry;
		}
	}

	return pSock3;
}


PSock6Entry SOCKET6_find_entry(U32 *saddr, U16 sport, U32 *daddr, U16 dport, U16 proto)
{
	PSock6Entry pEntry;
	PSock6Entry pSock3 = NULL;
	struct slist_entry *entry;
	U32 hash;
	U32 daddr_lo;

	daddr_lo = READ_UNALIGNED_INT(daddr[IP6_LO_ADDR]);
	hash = HASH_SOCK6(daddr_lo, dport, proto);
	
	slist_for_each(pEntry, entry, &sock6_cache[hash], list)
	{
		if (!IPV6_CMP(pEntry->Daddr, daddr) && (pEntry->Dport == dport) && (pEntry->proto == proto))
		{
			// 3-tuples match
			if (pEntry->connected) {
				// check 5-tuples for connected sockets
				if (!IPV6_CMP(pEntry->Saddr, saddr) && (pEntry->Sport == sport)) 
						return pEntry;	
			}
			else // remind last 3 tuples match (should be unique)
			 	pSock3 = pEntry;
		}
	}
	
	return pSock3;
}

/* free IPv4 sockets entries */
void SOCKET4_free_entries(void)
{
	int i;
	U32 hash_by_id;
	PSockEntry pSock;

	for(i = 0; i < NUM_SOCK_ENTRIES; i++)
	{
		struct slist_entry *entry;

		slist_for_each_safe(pSock, entry, &sock4_cache[i], list)
		{
			hash_by_id = HASH_SOCKID(pSock->SocketID);
			socket4_remove(pSock, i, hash_by_id);
		}
	}
}


int SOCKET4_HandleIP_Socket_Open (U16 *p, U16 Length)
{
	SockOpenCommand SocketCmd;
	PSockEntry pEntry;

	// Check length
	if (Length != sizeof(SockOpenCommand))
		return ERR_WRONG_COMMAND_SIZE;
	// Ensure alignment
	SFL_memcpy((U8*)&SocketCmd, (U8*)p, sizeof(SockOpenCommand));

	if (!SocketCmd.SockID)
		return ERR_WRONG_SOCKID;

	pEntry = SOCKET4_find_entry(SocketCmd.Saddr, SocketCmd.Sport, SocketCmd.Daddr, SocketCmd.Dport, SocketCmd.proto);
	if ((pEntry) && (pEntry->connected == SocketCmd.mode)) {
		if (pEntry->SocketID != SocketCmd.SockID)
			return ERR_SOCK_ALREADY_OPENED_WITH_OTHER_ID;
		else
			return ERR_SOCK_ALREADY_OPEN;
	}

	if (SOCKET_find_entry_by_id(SocketCmd.SockID) != NULL)
		return ERR_SOCKID_ALREADY_USED;

#if defined(COMCERTO_2000)
	/* FIXME, if MSP support was not compiled in we should return error */
#elif defined(COMCERTO_1000)
	if ((SocketCmd.SockType == SOCKET_TYPE_MSP) && (!mspItfInfo))
		return ERR_MSP_NOT_READY;
#elif defined(COMCERTO_100)
	if (SocketCmd.SockType == SOCKET_TYPE_MSP)
		return ERR_WRONG_SOCK_TYPE;
#endif

	if ((pEntry = (struct _tSockEntry*)socket4_alloc()) == NULL)
	  	return ERR_NOT_ENOUGH_MEMORY;

	memset(pEntry, 0, sizeof (SockEntry));
	pEntry->SocketFamily = PROTO_IPV4;
	pEntry->Daddr = SocketCmd.Daddr;
	pEntry->Saddr = SocketCmd.Saddr;
	pEntry->Dport = SocketCmd.Dport;
	pEntry->Sport = SocketCmd.Sport;
	pEntry->proto = SocketCmd.proto;
	pEntry->SocketID = SocketCmd.SockID;
	pEntry->queue = SocketCmd.queue;
	pEntry->dscp = SocketCmd.dscp;
	pEntry->SocketType = SocketCmd.SockType;	
	pEntry->connected = SocketCmd.mode;
	pEntry->route_id = SocketCmd.route_id;
	pEntry->initial_takeover_done = FALSE;
	pEntry->hash = HASH_SOCK(pEntry->Daddr, pEntry->Dport, pEntry->proto);
	pEntry->hash_by_id = HASH_SOCKID(pEntry->SocketID);

	/* Add software and hardware entry to local and packet engine hash */
	return socket4_add(pEntry);
}





int SOCKET4_HandleIP_Socket_Update (U16 *p, U16 Length)
{
	SockUpdateCommand SocketCmd;
	PSockEntry pEntry;
	int rc = NO_ERR;

	// Check length
	if (Length != sizeof(SockUpdateCommand))
		return ERR_WRONG_COMMAND_SIZE;
	// Ensure alignment
	SFL_memcpy((U8*)&SocketCmd, (U8*)p, sizeof(SockUpdateCommand));

	pEntry = SOCKET_find_entry_by_id(SocketCmd.SockID);

	if (pEntry == NULL)
		return ERR_SOCKID_UNKNOWN;

	if (pEntry->SocketFamily != PROTO_IPV4)
		return ERR_WRONG_SOCK_FAMILY;

	if (pEntry->connected &&
	    (((SocketCmd.Saddr != 0xFFFFFFFF) && (SocketCmd.Saddr != pEntry->Saddr)) ||
	    ((SocketCmd.Sport != 0xffff) && (SocketCmd.Sport != pEntry->Sport))))
		return ERR_SOCK_ALREADY_OPEN;


	if ((SocketCmd.Saddr != 0xFFFFFFFF) && (SocketCmd.Saddr != pEntry->Saddr)) {
		SOCKET4_delete_route(pEntry);
		pEntry->Saddr = SocketCmd.Saddr;
	}

	if (pEntry->route_id != SocketCmd.route_id) {
		SOCKET4_delete_route(pEntry);
		pEntry->route_id = SocketCmd.route_id;
	}

	if (SocketCmd.Sport != 0xffff)	
		pEntry->Sport = SocketCmd.Sport;

	if (SocketCmd.queue != 0xff)
		pEntry->queue = SocketCmd.queue;

	if (SocketCmd.dscp != 0xffff)
		pEntry->dscp = SocketCmd.dscp;

	socket4_update(pEntry);

	return rc;
}


int SOCKET4_HandleIP_Socket_Close (U16 *p, U16 Length)
{
	SockCloseCommand SocketCmd;
	PSockEntry pEntry;
	U32 hash, hash_by_id;

	// Check length
	if (Length != sizeof(SockCloseCommand))
		return ERR_WRONG_COMMAND_SIZE;

	// Ensure alignment
	SFL_memcpy((U8*)&SocketCmd, (U8*)p, sizeof(SockCloseCommand));

	pEntry = SOCKET_find_entry_by_id(SocketCmd.SockID);

	if (pEntry == NULL)
		return ERR_SOCKID_UNKNOWN;

	hash = HASH_SOCK(pEntry->Daddr, pEntry->Dport, pEntry->proto);
	hash_by_id = HASH_SOCKID(pEntry->SocketID);

	/* destroy hw socket entry */
	socket4_remove(pEntry, hash, hash_by_id);

	return NO_ERR;
}


/* free IPv6 sockets entries */
void SOCKET6_free_entries(void)
{
	int i;
	U32 hash_by_id;
	PSock6Entry pSock;

	for (i = 0; i < NUM_SOCK_ENTRIES; i++)
	{
		struct slist_entry *entry;

		slist_for_each_safe(pSock, entry, &sock6_cache[i], list)
		{
			hash_by_id = HASH_SOCKID(pSock->SocketID);
			socket6_remove(pSock, i, hash_by_id);
		}
	}
}


int SOCKET6_HandleIP_Socket_Open(U16 *p, U16 Length)
{
	Sock6OpenCommand SocketCmd;
	PSock6Entry pEntry;

	// Check length
	if (Length != sizeof(Sock6OpenCommand))
		return ERR_WRONG_COMMAND_SIZE;

	// Ensure alignment
	SFL_memcpy((U8*)&SocketCmd, (U8*)p, sizeof(Sock6OpenCommand));

	if (!SocketCmd.SockID)
		return ERR_WRONG_SOCKID;

	pEntry = SOCKET6_find_entry(SocketCmd.Saddr, SocketCmd.Sport, SocketCmd.Daddr, SocketCmd.Dport, SocketCmd.proto);
	if ((pEntry) && (pEntry->connected == SocketCmd.mode)) {
		if (pEntry->SocketID != SocketCmd.SockID)
			return ERR_SOCK_ALREADY_OPENED_WITH_OTHER_ID;
		else
			return ERR_SOCK_ALREADY_OPEN;
	}

	if (SOCKET_find_entry_by_id(SocketCmd.SockID) != NULL)
		return ERR_SOCKID_ALREADY_USED;

	if(SocketCmd.SockType == SOCKET_TYPE_MSP)
		return ERR_WRONG_SOCK_TYPE;
	
	if ((pEntry = (struct _tSock6Entry*)socket6_alloc()) == NULL)
		return ERR_NOT_ENOUGH_MEMORY;

	memset(pEntry, 0, sizeof (Sock6Entry));
	pEntry->SocketFamily = PROTO_IPV6;
	SFL_memcpy(pEntry->Daddr, SocketCmd.Daddr, IPV6_ADDRESS_LENGTH);
	SFL_memcpy(pEntry->Saddr, SocketCmd.Saddr, IPV6_ADDRESS_LENGTH);
	pEntry->Dport = SocketCmd.Dport;
	pEntry->Sport = SocketCmd.Sport;
	pEntry->proto = SocketCmd.proto;
	pEntry->SocketID = SocketCmd.SockID;
	pEntry->queue = SocketCmd.queue;
	pEntry->dscp = SocketCmd.dscp;
	pEntry->connected = SocketCmd.mode;
	pEntry->SocketType = SocketCmd.SockType;	
	pEntry->route_id = SocketCmd.route_id;
	pEntry->hash = HASH_SOCK6(pEntry->Daddr[IP6_LO_ADDR], pEntry->Dport, pEntry->proto);
	pEntry->hash_by_id = HASH_SOCKID(pEntry->SocketID);

	return socket6_add(pEntry);
}


int SOCKET6_HandleIP_Socket_Update(U16 *p, U16 Length)
{
	Sock6UpdateCommand SocketCmd;
	PSock6Entry pEntry;
	int rc = NO_ERR;
	U32 nulladdr[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};

	// Check length
	if (Length != sizeof(Sock6UpdateCommand))
		return ERR_WRONG_COMMAND_SIZE;
	// Ensure alignment
	SFL_memcpy((U8*)&SocketCmd, (U8*)p, sizeof(Sock6UpdateCommand));

	pEntry = (PSock6Entry)SOCKET_find_entry_by_id(SocketCmd.SockID);

	if (pEntry == NULL)
		return ERR_SOCKID_UNKNOWN;

	if (pEntry->SocketFamily != PROTO_IPV6)
		return ERR_WRONG_SOCK_FAMILY;

	/* For connected sockets don't allow 5-tuple to change */
	if (pEntry->connected &&
	    ((IPV6_CMP(SocketCmd.Saddr, nulladdr) && IPV6_CMP(SocketCmd.Saddr, pEntry->Saddr)) ||
	    ((SocketCmd.Sport != 0xffff) && (SocketCmd.Sport != pEntry->Sport))))
		return ERR_SOCK_ALREADY_OPEN;

	if (IPV6_CMP(SocketCmd.Saddr, nulladdr) && IPV6_CMP(SocketCmd.Saddr, pEntry->Saddr))
	{
		// Route has changed -> delete it
		SOCKET6_delete_route(pEntry);
		SFL_memcpy(pEntry->Saddr, SocketCmd.Saddr, IPV6_ADDRESS_LENGTH);
	}

	if (pEntry->route_id != SocketCmd.route_id)
	{
		// Route has changed -> delete it
		SOCKET6_delete_route(pEntry);
		pEntry->route_id = SocketCmd.route_id;
	}

	if (SocketCmd.Sport != 0xffff)
		pEntry->Sport = SocketCmd.Sport;

	if (SocketCmd.queue != 0xff)
		pEntry->queue = SocketCmd.queue;

	if (SocketCmd.dscp != 0xffff)
		pEntry->dscp = SocketCmd.dscp;
	
	socket6_update(pEntry);

	return rc;
}


int SOCKET6_HandleIP_Socket_Close(U16 *p, U16 Length)
{
	Sock6CloseCommand SocketCmd;
	PSock6Entry pEntry;
	U32 hash, hash_by_id;

	// Check length
	if (Length != sizeof(Sock6CloseCommand))
		return ERR_WRONG_COMMAND_SIZE;

	// Ensure alignment
	SFL_memcpy((U8*)&SocketCmd, (U8*)p, sizeof(Sock6CloseCommand));

	pEntry = (PSock6Entry)SOCKET_find_entry_by_id(SocketCmd.SockID);

	if (pEntry == NULL)
		return ERR_SOCKID_UNKNOWN;

	hash = HASH_SOCK6(pEntry->Daddr[IP6_LO_ADDR], pEntry->Dport, pEntry->proto);
	hash_by_id = HASH_SOCKID(pEntry->SocketID);

	socket6_remove(pEntry, hash, hash_by_id);

	return NO_ERR;
}


#if defined (COMCERTO_2000)
BOOL socket_init(void)
{
	int i;
	struct pfe_ctrl *ctrl = &pfe->ctrl;

	socket_dma_pool = ctrl->dma_pool_512;

	for (i = 0; i < NUM_SOCK_ENTRIES; i++)
	{
		slist_head_init(&sock4_cache[i]);
		slist_head_init(&sock6_cache[i]);
		slist_head_init(&sockid_cache[i]);
		dlist_head_init(&hw_sock_active_list[i]);
		dlist_head_init(&hw_sock_active_list_id[i]);
	}

	dlist_head_init(&hw_sock_removal_list);

	timer_init(&socket_timer, hw_socket_delayed_remove);
	timer_add(&socket_timer, CT_TIMER_INTERVAL);

	return 0;
}

void socket_exit(void)
{
	struct dlist_head *entry;
	struct _thw_sock4 *hw_sock;

	timer_del(&socket_timer);

	SOCKET4_free_entries();
	SOCKET6_free_entries();

	dlist_for_each_safe(hw_sock, entry, &hw_sock_removal_list, list)
	{
		dlist_remove(&hw_sock->list);
		dma_pool_free(socket_dma_pool, hw_sock, be32_to_cpu(hw_sock->dma_addr));
	}
}

#endif

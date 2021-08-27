/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>

#include "include/mach/smp.h"
#include "mvTypes.h"
#include "mvOs.h"
#include "mvDebug.h"
#include "mvCommon.h"
#include "mvStack.h"
#include "mvIpc.h"
#include "mv_ipc/linux_amp/mv_ipc_os.h"
#include "cpu/mvCpu.h"

//#define IPC_NET_DEBUG
#ifdef IPC_NET_DEBUG
#define ipcnet_dbg	printk
#else
#define ipcnet_dbg(x...)
#endif

#define ipc_attach_chn(linkId, chnId, cpu, ret)		mvIpcAttachChannel(linkId, chnId, cpu, ret)
#define ipc_dettach_chn(linkId, chnId)				mvIpcDettachChannel(linkId, chnId)
#define ipc_close_chn(linkId, chnId)				mvIpcCloseChannel(linkId, chnId)
#define ipc_tx_msg(linkId, chnId, msg)				mvIpcTxMsg(linkId, chnId, msg)
#define ipc_tx_ready(linkId, chnId)					mvIpcIsTxReady(linkId, chnId)
#define ipc_release_msg(linkId, chnId, msg)			mvIpcReleaseMsg(linkId, chnId, msg)
#define ipc_sh_malloc(linkId, size)					mvIpcShmemMalloc(linkId, size)
#define ipc_open_chn(linkId, chnId, rx_clbk)		mvIpcOpenChannel(linkId, chnId, rx_clbk)
#define ipc_enable_chn_rx(linkId, chnId)			mvIpcEnableChnRx(linkId, chnId)
#define ipc_virt_to_phys(linkId, virt_addr)			mvIpcOsVirt2Phys(linkId, virt_addr)
#define ipc_phys_to_virt(linkId, phys_addr)			mvIpcOsPhys2Virt(linkId, phys_addr)

#define IPC_NET_WRAP                (4 + ETH_HLEN + 4)
#define IPC_NET_RX_BUF_SIZE(mtu) 	MV_ALIGN_UP(((mtu) + IPC_NET_WRAP), CPU_D_CACHE_LINE_SIZE)
#define IPC_NET_MAX_TX_DESC			32
#define IPC_NET_MTU_SIZE			1500
#define IPC_NET_LINK_ID				0
#define IPC_NET_CHANNEL_ID			0
#define IPC_NET_BUFF_USED(ptr)      (ptr)
#define IPC_NET_ALLOC_TIME(ptr)     (ptr + 1)
#define IPC_NET_DATA_PTR(ptr)       (ptr + 2)

static void ipc_net_tx_timeout( struct net_device *dev);
int ipc_net_start(struct net_device *dev);
int ipc_net_stop( struct net_device *dev);
static int ipc_net_xmit(struct sk_buff *skb, struct net_device *dev);
static struct net_device_stats *ipc_net_get_stats(struct net_device *dev);
static int ipc_net_set_address(struct net_device *dev, void *p);

typedef enum
{
	IPC_NET_SHARED_BUF = 0,
	IPC_NET_PRIVATE_BUF,
	IPC_NET_RELEASE_MSG

}ipc_net_msg_type;

static struct ipc_net_device *ipc_net_dev = NULL;
struct ipc_net_device
{
	struct net_device   *net_dev;
	struct delayed_work ipc_net_link;
	struct timer_list 	watchdog_timer;
	u32 				watchdog_timeo;
	struct net_device_stats 	stats;
	u32					*sh_buff_fifo[IPC_NET_MAX_TX_DESC];
	int  				next_sh_buff;
	spinlock_t         	lock;
	u32					target_cpu;
} ipc_net_device;

static const struct net_device_ops mv_ipc_netdev_ops = {
	.ndo_open = ipc_net_start,
	.ndo_stop = ipc_net_stop,
	.ndo_start_xmit = ipc_net_xmit,
	.ndo_set_mac_address = ipc_net_set_address,
	.ndo_change_mtu = NULL,
	.ndo_tx_timeout = ipc_net_tx_timeout,
	.ndo_get_stats  = ipc_net_get_stats,
};

/****************************************************************************************
 * ipc_net_tx_timeout()                                 				        		*
 *   transmit timeout function function (dummy)											*
 ***************************************************************************************/
static void ipc_net_tx_timeout( struct net_device *dev )
{
    printk(KERN_INFO "%s: tx timeout\n", dev->name);
}

/****************************************************************************************
 * ipc_net_watchdog()                                 				        		*
 *   xmit watchdog function: restart the blocked TX queue 								*
 ***************************************************************************************/
static void ipc_net_watchdog(unsigned long data)
{
    struct net_device   *dev = (struct net_device *)data;
    struct ipc_net_device* priv = netdev_priv(dev);

    ipcnet_dbg("IPC NET: Entering Watchdog\n");

    spin_lock(&priv->lock);

	if (ipc_tx_ready(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID) == MV_OK) {
		if (netif_queue_stopped(dev) && (dev->flags & IFF_UP))
			netif_wake_queue(dev);
	}
	else{
		mod_timer(&priv->watchdog_timer, jiffies + priv->watchdog_timeo);
	}

    spin_unlock(&priv->lock);

    ipcnet_dbg("IPC NET: Leaving Watchdog\n");
}

/****************************************************************************************
 * ipc_net_set_address()                                 				        	*
 *   set new MAC address 																*
 ***************************************************************************************/
static int ipc_net_set_address(struct net_device *dev, void *p)
{
	struct sockaddr *sa = p;

	if (!is_valid_ether_addr(sa->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);
	return 0;
}
/****************************************************************************************
 * ipc_net_get_stats()	                                 				        	*
 *   return network interface statistics 												*
 ***************************************************************************************/
static struct net_device_stats *ipc_net_get_stats(struct net_device *dev)
{
	struct ipc_net_device* priv = netdev_priv(dev);
	return &priv->stats;
}

/****************************************************************************************
 * ipc_net_stop()                                 				        			*
 *   stop IPC network interface 														*
 ***************************************************************************************/
int ipc_net_stop( struct net_device *dev )
{

	/* stop upper layer */
	netif_carrier_off(dev);
	netif_stop_queue(dev);

	del_timer_sync(&dev->watchdog_timer);

	if (mvIpcDettachChannel(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID) != MV_OK)
		printk("IPC NET: Failed to detach channel %d %d", IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID);

	printk(KERN_NOTICE "%s: stopped\n", dev->name);

	return 0;
}

/****************************************************************************************
 * ipc_net_get_buff()                                 				        		*
 *   Allocate a shared buffer from buffer pool
 ***************************************************************************************/
static INLINE u32 *ipc_net_get_buff(struct ipc_net_device *priv)
{
	int cnt;
	u32 used, *buff;

	for(cnt = 0; cnt < IPC_NET_MAX_TX_DESC; cnt++) {
		buff = priv->sh_buff_fifo[priv->next_sh_buff];
		used = *(IPC_NET_BUFF_USED(buff));

		if(!used)
			break;
		else {
			//TODO - add code here that releases a buffer based on time diff since allocation

			ipcnet_dbg(KERN_INFO "IPC NET: Buffer %d ptr 0x%8x allocated at %u still used now in %u\n",
				   priv->next_sh_buff, buff ,*(IPC_NET_ALLOC_TIME(buff)), (u32)jiffies );
			priv->next_sh_buff++;
		}
	}

	/* Check if buffer was allocated */
	if(cnt == IPC_NET_MAX_TX_DESC)
		return 0;

	/* Mark the buffer as used and store the allocation time */
	*(IPC_NET_BUFF_USED(buff))  = 1;
	*(IPC_NET_ALLOC_TIME(buff)) = (u32)jiffies;

	priv->next_sh_buff++;
	if(priv->next_sh_buff == IPC_NET_MAX_TX_DESC)
		priv->next_sh_buff = 0;

	return buff;
}

/****************************************************************************************
 * ipc_net_xmit()	                                 				        		*
 *   net xmit function: transmit buffer through IPC interface						    *
 ***************************************************************************************/
static int ipc_net_xmit(struct sk_buff *skb, struct net_device *dev)
{
	struct ipc_net_device* priv = netdev_priv(dev);
	int tx_in_interrupt	        = in_interrupt();
	MV_STATUS status;
	MV_IPC_MSG msg;
	u32 *sh_buf;
	unsigned long flags = 0;

	ipcnet_dbg(KERN_INFO "IPC NET: TX: Sending skb of size %d\n", skb->len);

	if (skb_shinfo(skb)->nr_frags != 0) {
		printk(KERN_ERR"%s: can't transmit fragmented skb.\n", dev->name);
		return 1;
	}

	if (netif_queue_stopped(dev)) {
		priv->stats.tx_errors++;
		priv->stats.tx_carrier_errors++;
		printk(KERN_ERR"%s: transmitting while stopped.\n", dev->name);
		return 1;
	}
	if (!tx_in_interrupt)
		local_irq_save(flags);

	if (!spin_trylock(&priv->lock)) {
		/* Collision - tell upper layer to re-queue */
		if (!tx_in_interrupt)
			local_irq_restore(flags);
		priv->stats.tx_dropped++;
		return NETDEV_TX_LOCKED;
	}

	sh_buf  = ipc_net_get_buff(priv);
	if(sh_buf == 0) {
		printk(KERN_ERR "IPC NET: Cannot allocate shared buffer for transmit\n");
		priv->stats.tx_dropped++;
		return NETDEV_TX_BUSY;
	}

	memcpy(IPC_NET_DATA_PTR(sh_buf), skb->data, skb->len);

	msg.type  = IPC_NET_SHARED_BUF;
	msg.ptr   = ipc_virt_to_phys(0, (void *)sh_buf);
	msg.size  = skb->len;
	msg.value = (MV_U32)sh_buf;

	status = ipc_tx_msg(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID, &msg);

	priv->stats.tx_bytes += skb->len;
	priv->stats.tx_packets++;

	if (status == MV_ERROR) {
		netif_stop_queue(dev);
		mod_timer(&priv->watchdog_timer, jiffies + priv->watchdog_timeo);
		*(IPC_NET_BUFF_USED(sh_buf)) = 0;
		printk(KERN_INFO "IPC NET: TX: TX queue busy\n");
	}

	if (!tx_in_interrupt)
		spin_unlock_irqrestore(&priv->lock, flags);
	else
		spin_unlock(&priv->lock);

	if (unlikely(status == MV_ERROR)) {
		priv->stats.tx_dropped++;
		return NETDEV_TX_BUSY;
	}
	else {
		dev_kfree_skb_any(skb);
		return NETDEV_TX_OK;
	}
}

/****************************************************************************************
 * ipc_net_rx()	                                 				        				*
 *   net receive function: called from IPC driver on msg arrival						*
 ***************************************************************************************/
int ipc_net_rx(MV_IPC_MSG *msg)
{
	struct ipc_net_device *priv = (struct ipc_net_device *)ipc_net_dev;
	struct net_device *dev = priv->net_dev;
	struct sk_buff *skb;
	u32 *ptr_virt;
	u32 size;

	ipcnet_dbg("Recieved msg %d, %d, 0x%08x, 0x%08x\n", msg->type, msg->size, msg->ptr, msg->value);

	if(msg->type == IPC_NET_SHARED_BUF)
	{
		skb = dev_alloc_skb(IPC_NET_RX_BUF_SIZE(dev->mtu));
		if (unlikely(!skb)) {
			printk(KERN_ERR "%s: skb alloc failure!\n", dev->name);
			ipc_release_msg(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID, msg);
			return MV_ERROR;
		}

		ptr_virt = (u32 *)ipc_phys_to_virt(0, msg->ptr);
		if(ptr_virt == 0) {
			printk(KERN_ERR "IPC NET: Unable to map shared buf ptr 0x%08x\n", (u32)msg->ptr);

		}

		size = msg->size;

		skb_reserve(skb, NET_IP_ALIGN);
		skb_put(skb, size);

		/* Copy the buffer, release the buffer and the message */
		memcpy(skb->data, IPC_NET_DATA_PTR(ptr_virt), size);

		*(IPC_NET_BUFF_USED(ptr_virt)) = 0;
		ipc_release_msg(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID, msg);
	}
	else {
		ipcnet_dbg("IPC NET: Received unknown msg type %d\n", msg->type);
		ipc_release_msg(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID, msg);
		return -1;
	}

	skb->dev       = dev;
	skb->csum      = 0;
	skb->ip_summed = CHECKSUM_UNNECESSARY;
	skb->protocol  = eth_type_trans(skb, dev);

	ipcnet_dbg("IPC NET: Passing skb 0x%08x to stack\n", skb);
	if (likely(!netif_rx(skb))) {
		ipcnet_dbg("IPC NET: Processed skb 0x%08x by stack\n", skb);
		priv->stats.rx_packets++;
		priv->stats.rx_bytes += size;
		return 0;
	}
	else {
		priv->stats.rx_errors++;
		printk("netif_receive_skb err\n");
		return -1;
	}
}

/****************************************************************************************
 * ipc_net_link_worker()                                 				        		*
 *   worker thread: wait for iPC link to establish										*
 ***************************************************************************************/
static void ipc_net_link_worker(struct work_struct *dummy)
{
	MV_STATUS ret;
	int attached;

	ret = ipc_attach_chn(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID, ipc_net_dev->target_cpu, &attached);
	if(ret != MV_OK){
		printk(KERN_ERR "IPC NET: IPC attach returned error for target CPU %d\n", ipc_net_dev->target_cpu);
		return;
	}

	if (attached) {
		printk(KERN_INFO "%s link up\n", ipc_net_dev->net_dev->name);
		netif_carrier_on(ipc_net_dev->net_dev);
		netif_wake_queue(ipc_net_dev->net_dev);
	}
	else {
		schedule_delayed_work(&ipc_net_dev->ipc_net_link, HZ);
	}
}

/****************************************************************************************
 * ipc_net_start()                                 				        			*
 *   start IPC network interface 														*
 ***************************************************************************************/
int ipc_net_start(struct net_device *dev)
{
    struct ipc_net_device *priv = netdev_priv(dev);

    ipcnet_dbg("IPC NET: Starting %s interface\n", dev->name);

    /* in default link is down */
    netif_carrier_off(dev);

    /* Stop the TX queue - it will be enabled upon PHY status change after link-up interrupt/timer */
    netif_stop_queue(dev);

    /* If you want to request irq - do it here */

    /* CLear statistics */
    memset(&priv->stats, 0, sizeof(priv->stats));

    /* Init watchdog mechanism */
	priv->watchdog_timeo = 10;
	priv->watchdog_timer.function = ipc_net_watchdog;
	priv->watchdog_timer.data = (unsigned long) dev;
	init_timer(&priv->watchdog_timer);
	spin_lock_init(&priv->lock);

	INIT_DELAYED_WORK(&priv->ipc_net_link, ipc_net_link_worker);
	schedule_delayed_work(&priv->ipc_net_link, HZ);

	printk(KERN_NOTICE "IPC NET: %s started\n", dev->name);

	return 0;
}

/****************************************************************************************
 * ipc_net_init_buff_pool()                                 				       		*
 *   Initialize IPC network buffer pool 												*
 ***************************************************************************************/
static INLINE int ipc_net_init_buff_pool(struct ipc_net_device *priv, int mtu)
{
	int i;
	u32 *sh_mem_ptr;

	for(i = 0; i < IPC_NET_MAX_TX_DESC; i++) {
		sh_mem_ptr = (u32 *)ipc_sh_malloc(0, IPC_NET_RX_BUF_SIZE(mtu));
		if(!sh_mem_ptr){
			ipcnet_dbg(KERN_ERR "IPC NET: Failed to allocate shared mem of size %d\n", mtu);
			return 0;
		}

		*(IPC_NET_BUFF_USED(sh_mem_ptr)) = 0;
		priv->sh_buff_fifo[i] = sh_mem_ptr;
	}

	priv->next_sh_buff = 0;

	return 1;
}

/****************************************************************************************
 * ipc_net_init()                                 				        		*
 *   Initialize IPC network interface 										*
 ***************************************************************************************/
#define MV_IPC_SLAVE_TO_MASTER_WAIT_TIME	1
#define MV_IPC_SLAVE_TO_MASTER_WAIT_LOOPS	20
static int ipc_net_init(void *platform_data)
{
	struct net_device     *dev = NULL;
	struct ipc_net_device *priv;
	MV_STATUS status;
	int success, i;
	int target_cpu, min_cpu, max_cpu;
	int counter;

	if(ipc_net_dev){
		printk(KERN_ERR "%s: ipc0 already initialized\n", __FUNCTION__);
		return -1;
	}

	target_cpu = *((int*)platform_data);
	min_cpu    = min((int)0, (int)(master_cpu_id + mv_cpu_count));
	max_cpu    = max((int)(NR_CPUS - 1), (int)master_cpu_id);

	if((target_cpu < min_cpu) || (target_cpu > max_cpu)){
		printk(KERN_ERR "IPC NET: Target CPU %d out of range [%d, %d]\n", target_cpu, min_cpu, max_cpu);
		return -1;
	}

	/* Allocate a network device */
	dev = alloc_netdev(sizeof(struct ipc_net_device),"ipc%d", ether_setup);
	if (!dev) {
		printk(KERN_ERR"%s: Out of memory\n", __FUNCTION__);
		return -ENOMEM;
	}

	/* Initialize the private device structure */
	priv = netdev_priv(dev);
	memset(priv, 0, sizeof(struct ipc_net_device));
	priv->net_dev = dev;

	/* Initialize the device structure. */
	dev->irq 			= 0;
	dev->tx_queue_len   = IPC_NET_MAX_TX_DESC;
	dev->watchdog_timeo = 5*HZ;
	dev->mtu 	    = IPC_NET_MTU_SIZE;
	dev->netdev_ops     = &mv_ipc_netdev_ops;
	dev->flags    	   &= ~IFF_MULTICAST;
	dev->features       = NETIF_F_SG | NETIF_F_LLTX;

	/* Set a static MAC address*/
	dev->dev_addr[0] = 0;
	dev->dev_addr[1] = 0;
	for(i = 2; i < ETH_ALEN; i++) {
		dev->dev_addr[i] = i + whoAmI();
	}

	priv->target_cpu = target_cpu;

	/* Initialize IPC driver */
	printk(KERN_ERR "IPC NET:Going open chn\n");
	counter = 0;
	do {
		status = ipc_open_chn(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID, ipc_net_rx);
		printk(KERN_ERR "IPC NET: open chn status %d\n", status);
		if (status == MV_NOT_STARTED) {
			/*if master still not started, increase the limit or delay time*/
			if (counter >= MV_IPC_SLAVE_TO_MASTER_WAIT_LOOPS)
				goto open_fail;
			counter++;
			udelay(MV_IPC_SLAVE_TO_MASTER_WAIT_TIME);
			continue;
		}

		if (status != MV_OK) {
			printk(KERN_ERR "IPC NET: Failed to open IPC channel %d %d",
					IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID);
			goto open_fail;
		}
	} while (status != MV_OK);

	/* Enable channel for RX */
	ipc_enable_chn_rx(IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID);

	success = ipc_net_init_buff_pool(priv, dev->mtu);
	if(!success)
	{
		printk(KERN_ERR "failed to allocate buffer pool for %s\n", dev->name);
		goto open_fail;
	}

	if (register_netdev(dev)) {
		printk(KERN_ERR "failed to register %s\n", dev->name);
		goto open_fail;
	} else {
		printk(KERN_INFO "IPC NET: Registered %s, ifindex = %d, Channel = %d %d, cpu = %d",
				dev->name, dev->ifindex, IPC_NET_LINK_ID, IPC_NET_CHANNEL_ID, priv->target_cpu);
	}

	ipc_net_dev = priv;

	return 0;

open_fail:
	free_netdev(dev);
	return -1;

}

/****************************************************************************************
 * ipc_net_cleanup_module()                                 				        	*
 *   free IPC network interface 														*
 ***************************************************************************************/
static void __exit ipc_net_cleanup_module(void)
{

}

/****************************************************************************************
 * ipc_net_probe()                                 				        		*
 *   probe IPC network interface 												*
 ***************************************************************************************/
static int ipc_net_probe(struct platform_device *pdev)
{
	return ipc_net_init(pdev->dev.platform_data);
}

/****************************************************************************************
 * ipc_net_shutdown()                                 				        		*
 *   probe IPC network interface 												*
 ***************************************************************************************/
static void ipc_net_shutdown(struct platform_device *pdev)
{
    printk(KERN_INFO "Shutting Down Marvell IPC Net Driver\n");
}

/****************************************************************************************
 * ipc_net_remove()                                 				        		*
 *   probe IPC network interface 												*
 ***************************************************************************************/
static int ipc_net_remove(struct platform_device *pdev)
{
    printk(KERN_INFO "Removing Marvell IPC Net Driver\n");
    return 0;
}

static struct platform_driver ipc_net_driver = {
	.probe    = ipc_net_probe,
	.remove   = ipc_net_remove,
	.shutdown = ipc_net_shutdown,
#ifdef CONFIG_CPU_IDLE
//	.suspend = mv_eth_suspend, //TODO
//	.resume  = mv_eth_resume, // TODO
#endif /* CONFIG_CPU_IDLE */
	.driver = {
		.name = "mv_ipc_net",
	},
};

/****************************************************************************************
 * ipc_net_init_module()                                 				        		*
 *   register IPC network interface 										*
 ***************************************************************************************/
static int __init ipc_net_init_module(void)
{
	return platform_driver_register(&ipc_net_driver);
}

module_init(ipc_net_init_module);
module_exit(ipc_net_cleanup_module);
MODULE_DESCRIPTION("Marvell Inter-Processor Pseudo-NIC Driver");
MODULE_AUTHOR("Yehuda Yitschak <yehuday@marvell.com>");
MODULE_LICENSE("GPL");

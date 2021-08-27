/*******************************************************************************

  Intel(R) UDMA Network Device Model sample code

  Copyright(c) 2012 Intel Corporation.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU General Public License along with
  this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

*******************************************************************************/

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/vmalloc.h>
#include <linux/pagemap.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/tcp.h>
#include <linux/ipv6.h>
#include <linux/slab.h>
#include <net/checksum.h>
#include <net/ip6_checksum.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/if_vlan.h>
#include <linux/cpu.h>
#include <linux/smp.h>
#include <linux/pm_qos.h>
#include <linux/pm_runtime.h>
#include <linux/aer.h>
#include <linux/crc32.h>
#include <asm/io.h>

#include <linux/udma_api.h>

#define DRV_VERSION "0.1"

#define MAX_TRY_TIMES           	10

/* Performance tuning parameter */
#define PCKT_NUM_GIVEN_ONE_TIME		0

/* Get MAC Adress like the same way as e1000 */
#define CONFIG_RAM_BASE         0x60000
#define GBE_CONFIG_OFFSET       0x0
#define NODE_ADDRESS_SIZE       6

#define GBE_CONFIG_RAM_BASE \
        ((unsigned int)(CONFIG_RAM_BASE + GBE_CONFIG_OFFSET))

#define GBE_CONFIG_BASE_VIRT \
        ((void __iomem *)phys_to_virt(GBE_CONFIG_RAM_BASE))

#define GBE_CONFIG_FLASH_READ(base, offset, count, data) \
        (ioread16_rep(base + (offset << 1), data, count))

static void inline __udma_net_kfree(void *buffer) 
{	
	kfree(buffer);
}
static void inline __udma_net_kfree_skb(struct sk_buff *skb) 
{	
	dev_kfree_skb_any(skb);
}

//efine UDMA_LDML_DEBUG 1
#ifdef UDMA_LDML_DEBUG

#define udma_net_dbg(fmt, args...)  do \
				     { \
				           printk(KERN_INFO "\n%s " fmt "\n","udma_net",##args); \
				     } while(0)
#else
#define udma_net_dbg(fmt, arg...)  do { } while (0)
#endif
#define udma_net_info(fmt, args...)  do \
				     { \
				           printk(KERN_INFO "\n%s " fmt "\n","udma_net",##args); \
				     } while(0)

#define udma_net_err(fmt, args...)  do \
				     { \
				           printk(KERN_ERR "\n%s " fmt "\n","udma_net",##args); \
				     } while(0)

struct udma_adapter{
	unsigned char udma_port;
	u8 mac_addr[NODE_ADDRESS_SIZE];
};
static struct net_device *udma_net_dev[UDMA_PORT_NUM_TOTAL];

/* Set MAC Address */
static void udma_set_mac_addr(unsigned char udma_port, struct net_device *netdev, struct udma_adapter *adapter){
       u16 eeprom_data, i, offset;

      for (i = 0; i < NODE_ADDRESS_SIZE; i += 2) {
               offset = i >> 1;
               GBE_CONFIG_FLASH_READ(GBE_CONFIG_BASE_VIRT, offset, 1,&eeprom_data);
               adapter->mac_addr[i] = (u8) (eeprom_data & 0x00FF);
               adapter->mac_addr[i + 1] = (u8) (eeprom_data >> 8);
       }
       adapter->mac_addr[NODE_ADDRESS_SIZE - 1] += udma_port + 1;

       memcpy(netdev->dev_addr, adapter->mac_addr, netdev->addr_len);
       memcpy(netdev->perm_addr, adapter->mac_addr, netdev->addr_len);
}

/**
 * udma_net_give_buffer - Function used to create and give free skb and buffer_desc buffer to the UDMA Low level Driver. 
 * budget - 0, means feed the UDMA until it's full; positive, feed the UDMA with the number of free buffers
**/
static int udma_net_give_buffer(struct udma_adapter *adapter, int budget)
{
	udma_buffer_desc_t *buffer_desc;
	struct sk_buff *skb;
	udma_result_t ret = UDMA_OK;
	int done = 0;
	int tried = 0;

	while(1){
		if (((budget) && (done > budget)) ||(tried > MAX_TRY_TIMES))
			break;
		buffer_desc = kzalloc(sizeof(udma_buffer_desc_t), GFP_ATOMIC);
		if(!buffer_desc) {
			printk(KERN_ERR "LDML buffer descriptor memory allocation failure\n");	
			continue;
		}
		skb = netdev_alloc_skb_ip_align(udma_net_dev[adapter->udma_port], UDMA_MIN_RX_BUFFER_SIZE);
		if(!skb){
			__udma_net_kfree(buffer_desc);
			printk("allocate new skb failed in %s function\n", __func__);
			continue;
		} else if (skb_is_nonlinear(skb)) {
			__udma_net_kfree(buffer_desc);
			printk("non linear skb is not allowed to UDMA %s function\n", __func__);
			continue;		
		}
		buffer_desc->buf = skb->data;
		buffer_desc->len = UDMA_MIN_RX_BUFFER_SIZE;
		buffer_desc->private = skb;
		buffer_desc->data_size = 0;
		buffer_desc->state = UDMA_BUFFER_INITIAL;
		ret = udma_give_free_buffer(adapter->udma_port, buffer_desc);
		if (UDMA_OK == ret) {
			done ++;
			continue;
		}
		__udma_net_kfree(buffer_desc);
		__udma_net_kfree_skb(skb);
		if (likely(ret == UDMA_FULL)) {
			udma_net_dbg("Rx Warnning: UDMA is full \n");
			break;		
		} else if (ret == UDMA_BUSY) {
			udma_net_dbg("Rx Warnning UDMA busy	\n");
			tried ++ ;
			continue;			
		} else if (ret == UDMA_INVALID_PARAM) {
			udma_net_dbg("Rx WARNNING:Input buffer parameter is incorrect, size 0x%x, ret 0x%x \n",buffer_desc->len,ret);
			tried ++ ;
			continue;				
		} else if (unlikely(ret == UDMA_NO_PERM)) {
			udma_net_dbg("Rx Warnning: UDMA is stopped  \n");
			break;
		} else if (unlikely(ret == UDMA_ERR)) {
			udma_net_dbg("Rx UDMA ERR!!  \n");
			break;	
		} else if (unlikely(ret == UDMA_UNINITIALIZED)) {
			udma_net_dbg("Rx Warnning UDMA NOT INIT!!	\n");
			break;							
		} else {
			udma_net_dbg("Rx ERROR Unrecognized UDMA error %d\n",ret);
			break;
		}
	}
	return done;
}

/**
 * udma_net_tx_callback - Function being called by UDMA Low level Driver when a packet is successfully transmitted
**/
static void udma_net_tx_callback(uint8_t port, udma_buffer_desc_t *buffer_desc)
{

	if((port >= UDMA_PORT_NUM_TOTAL) || (!buffer_desc) || (buffer_desc->state == UDMA_BUFFER_RX_DONE)){		
		udma_net_dbg("error:: NULL buffer desciptors, desc 0x%x, state 0x%x\n",buffer_desc,buffer_desc->state);
		return;
	} 

	//udma_net_dbg("In %s function buffer desc 0x%x, skb 0x%x\n", __func__,buffer_desc,buffer_desc->private); 
	if(buffer_desc->state == UDMA_BUFFER_TX_DONE) {
		if((buffer_desc->pos == UDMA_PACKT_START_END) || (buffer_desc->pos == UDMA_PACKT_END)) {
		__udma_net_kfree_skb((struct sk_buff *)buffer_desc->private);			
		}
	}
	else {
		__udma_net_kfree_skb((struct sk_buff *)buffer_desc->private);
	}

	buffer_desc->private = NULL;
	__udma_net_kfree(buffer_desc);
	if(netif_queue_stopped(udma_net_dev[port]))
		netif_wake_queue(udma_net_dev[port]);	
	return;
}

/**
 * udma_net_rx_callback - Function being called by UDMA Low level Driver when a packet is received
**/
static void udma_net_rx_callback(uint8_t port, udma_buffer_desc_t *buffer_desc)
{
	struct sk_buff *skb;
	struct udma_adapter *adapter = netdev_priv(udma_net_dev[port]);
	udma_net_dbg("In %s function 0x%x\n", __func__, buffer_desc);

	if((!buffer_desc) || (port >= UDMA_PORT_NUM_TOTAL) || (buffer_desc->state == UDMA_BUFFER_TX_DONE)){
		return;
	}

	skb = (struct sk_buff *)(buffer_desc->private);
	udma_net_dbg("In %s function buffer desc 0x%x, skb 0x%x\n", __func__,buffer_desc,buffer_desc->private); 

	if ((buffer_desc->state == UDMA_BUFFER_RX_DONE) && (buffer_desc->data_size != 0)) {
		udma_net_dbg("2In %s function % line \n", __func__,__LINE__);
		if (skb_is_nonlinear(skb)) {
			udma_net_dbg("This should never happen, Jombo packets are refused \n");		
			__udma_net_kfree_skb(skb);
			buffer_desc->private = NULL;
			__udma_net_kfree(buffer_desc);
			udma_net_give_buffer(adapter, PCKT_NUM_GIVEN_ONE_TIME); 	
			return;
		}
		buffer_desc->data_size -= ETH_FCS_LEN;
		
		skb_put(skb, buffer_desc->data_size);
		skb->dev = udma_net_dev[port];
		skb->protocol = eth_type_trans(skb, udma_net_dev[port]);
		udma_net_dev[port]->stats.rx_packets++;
		udma_net_dev[port]->stats.rx_bytes += skb->len;
		//udma_net_dbg("CRC input 0x%x, calc 0x%x\n",*(u32 *)(buffer_desc->buf + buffer_desc->data_size),ether_crc_le(buffer_desc->data_size,buffer_desc->buf));
		netif_receive_skb(skb);
		__udma_net_kfree(buffer_desc);
		/* Given one free buffer when received one */
		udma_net_give_buffer(adapter, PCKT_NUM_GIVEN_ONE_TIME);		
	} else {
		__udma_net_kfree_skb(skb);
		buffer_desc->private = NULL;
		__udma_net_kfree(buffer_desc);		
	}

	return;
}

static netdev_tx_t udma_net_xmit(struct sk_buff *skb,
				    struct net_device *netdev)
{
	struct udma_adapter *adapter = netdev_priv(netdev);
	udma_buffer_desc_t *buffer_desc;
	unsigned char port;
	unsigned int  nr_frags;
	unsigned int len = skb_headlen(skb);
	udma_result_t ret = UDMA_OK;
	u32 crc = 0;

	if (unlikely((!skb) || (skb->len <= 0))) {
		__udma_net_kfree_skb(skb);
		return NETDEV_TX_OK;
	}

	port = adapter->udma_port;
	nr_frags = skb_shinfo(skb)->nr_frags;

	if(nr_frags == 0) {
		buffer_desc = kzalloc(sizeof(udma_buffer_desc_t), GFP_ATOMIC);
		if(!buffer_desc) {
			__udma_net_kfree_skb(skb);
			netif_stop_queue(netdev);
			return NETDEV_TX_BUSY;
		}

		len = skb->len;
		/* Apend CRC to end of the frame per IEEE802.3 requriment */
	//	crc = ether_crc_le(len,skb->data);	

		/* Padding 0 if size is less than 64 bytes */
		if (skb->len < ETH_ZLEN) {
			WARN_ON(skb_pad(skb,ETH_ZLEN + ETH_FCS_LEN - skb->len));				
			//	goto free_desc_buffer;
			len += ETH_ZLEN + ETH_FCS_LEN - skb->len;
		}	
		else {
			WARN_ON(skb_pad(skb,ETH_FCS_LEN));
				//goto free_desc_buffer;				
			len += ETH_FCS_LEN;			
		}
	//	*(u32 *)(skb->data + skb->len)= crc;

		buffer_desc->buf = skb->data;
		buffer_desc->len = len;
		buffer_desc->pos = UDMA_PACKT_START_END;
		buffer_desc->private = skb;

		udma_net_dbg("In %s function buffer desc 0x%x, skb 0x%x len %x\n", __func__,buffer_desc,buffer_desc->private,skb->len);	

		ret = udma_send_packet(port, buffer_desc);
		if (UDMA_OK == ret) {			
			netdev->stats.tx_bytes += len;
			netdev->stats.tx_packets++;
		}
		else {
			if (likely(ret == UDMA_FULL)) {
				udma_net_dbg("Tx Warnning: UDMA is full \n");
			} else if (ret == UDMA_BUSY) {			
				netif_stop_queue(netdev);
				udma_net_dbg("Tx Warnning UDMA busy	\n");	
			} else if (ret == UDMA_INVALID_PARAM) {
				udma_net_dbg("Tx WARNNING:Input buffer parameter is incorrect, size 0x%x, ret 0x%x \n",buffer_desc->len,ret);			
			} else if (unlikely(ret == UDMA_NO_PERM)) {
				udma_net_dbg("Tx Warnning: UDMA is stopped  \n");
			} else if (unlikely(ret == UDMA_ERR)) {
				udma_net_dbg("Tx UDMA ERR!!	\n");
			} else if (unlikely(ret == UDMA_UNINITIALIZED)) {
				udma_net_dbg("Tx Warnning UDMA NOT INIT!!	\n");
			} else {
				udma_net_dbg("Tx ERROR Unrecognized UDMA error %d\n",ret);
			}			
			__udma_net_kfree(buffer_desc); 

			return NETDEV_TX_BUSY ;		
		}
	} else { 
		__udma_net_kfree_skb(skb);
		return NETDEV_TX_OK;		
		#if 0 //Fragment is not implemented
		buffer_desc = kzalloc(sizeof(udma_buffer_desc_t), GFP_ATOMIC);
		if(!buffer_desc){
			__udma_net_kfree_skb(skb);
			return NETDEV_TX_BUSY;
		}
		buffer_desc->buf = skb->data;
		buffer_desc->len = skb_headlen(skb);
		buffer_desc->pos = UDMA_PACKT_START;
		buffer_desc->private = skb;
		if(udma_send_packet(port, buffer_desc)){
			udma_net_dbg("Send packet failed in %s function, line %d\n", __func__, __LINE__);
			__udma_net_kfree_skb(skb);
			kfree(buffer_desc);
			return NETDEV_TX_BUSY;
		}
		netdev->stats.tx_bytes += skb_headlen(skb);
		for(i = 0;i < nr_frags;i++){
			struct skb_frag_struct *frag;

			buffer_desc = kzalloc(sizeof(udma_buffer_desc_t), GFP_ATOMIC);
			if(!buffer_desc){
				__udma_net_kfree_skb(skb);
				return NETDEV_TX_BUSY;
			} 
			frag = &skb_shinfo(skb)->frags[i];
			buffer_desc->buf = page_address(frag->page) + frag->page_offset;
			buffer_desc->len = frag->size;
			if(i < (nr_frags - 1)){
				buffer_desc->pos = UDMA_PACKT_CONTINUE;
			}
			else{
				buffer_desc->pos = UDMA_PACKT_END;
			}
			buffer_desc->private = skb;
			/* Try several times fail happens when udma send the fragment data*/
			for(j = 0;j < MAX_TRY_TIMES;j++){
				if(udma_send_packet(port, buffer_desc) == 0){
					netdev->stats.tx_bytes += frag->size;
					break;
				}
			}
			if(j == MAX_TRY_TIMES){
				printk(KERN_ERR "Failed to send the fragment data in %s function\n", __func__);
				return NETDEV_TX_BUSY;
			}
		}
#endif
	}
	netdev->stats.tx_packets++;
	return NETDEV_TX_OK;

}

/**
 * udma_net_open - Called when a network interface is made active
 * @netdev: network interface device structure
 *
 * Returns 0 on success, negative value on failure
 *
 * The open entry point is called when a network interface is made
 * active by the system (IFF_UP).  At this point all resources needed
 * for transmit and receive operations are allocated, the interrupt
 * handler is registered with the OS, the watchdog timer is started,
 * and the stack is notified that the interface is ready.
 **/
static int __udma_net_open(struct net_device *netdev)
{
	struct udma_adapter *adapter = netdev_priv(netdev);
	int ret = 0;
	unsigned char port = 0;

	port = adapter->udma_port;
	ret = udma_register_handler(port, &udma_net_tx_callback, &udma_net_rx_callback);
	if(ret)
		return ret;
	else {
		udma_net_give_buffer(adapter, 0);
		udma_net_give_buffer(adapter, 0);	
	}
	return 0;
}
static int udma_net_open(struct net_device *netdev)
{	
	int ret = 0;
	ret = __udma_net_open(netdev);
	if (ret)
		return ret;
	else 
		netif_start_queue(netdev);
	return 0;
}

/**
 * udma_close - Disables a network interface
 * @netdev: network interface device structure
 *
 * Returns 0, this is not allowed to fail
 *
 **/

static int udma_net_close(struct net_device *netdev)
{
	struct udma_adapter *adapter = netdev_priv(netdev);
	
	netif_stop_queue(netdev);
	udma_flush(adapter->udma_port);
	return 0;
}

/* Netdevice get statistics request */

/*static struct rtnl_link_stats64 *
udma_net_get_stats64(struct net_device *dev, struct rtnl_link_stats64 *stats)
{
	return 0;
}*/

static const struct net_device_ops udma_netdev_ops = {
	.ndo_open				= udma_net_open,
	.ndo_stop				= udma_net_close,
	.ndo_start_xmit			= udma_net_xmit,
	.ndo_change_mtu			= eth_change_mtu,
	//.ndo_get_stats64	= udma_net_get_stats64,
};

/**
 * udma_net_init - Driver Registration Routine
 *
 * udma_net_init is the first routine called when the driver is
 * loaded. 
 **/
 #define UDMA_NET "eth_udma"
static int __init udma_net_init(void)
{
	struct udma_adapter *adapter = NULL;
	struct net_device *netdev = NULL;
	int i,err;

	for (i = 0; i < UDMA_PORT_NUM_TOTAL; i++) {
		netdev = alloc_etherdev(sizeof(struct udma_adapter));
		if (!netdev){
			return -ENOMEM;
		}
		
		udma_net_dev[i] = netdev;
		adapter = netdev_priv(netdev);
		adapter->udma_port = i;
		
		netdev->netdev_ops = &udma_netdev_ops;
		//netdev->features |= NETIF_F_HW_CSUM;
		////NETIF_F_SG;
		sprintf(netdev->name, "%s%d", UDMA_NET, i);
		err = register_netdev(netdev);
		if (err){
			free_netdev(netdev);
			return err;
		}
		
		/* Currently the udma mac address is generated base on the GBE Mac address */
		udma_set_mac_addr(i, netdev, adapter);
		
	}
	
	printk(KERN_INFO "UDMA Network Device Driver init \n");
	return 0;
}

/**
 * udma_net_exit - Driver Exit Cleanup Routine
 *
 **/
static void __exit udma_net_exit(void)
{
	int i;
	struct udma_adapter *adapter;
	for (i = 0; i < UDMA_PORT_NUM_TOTAL; i++) {
		adapter = netdev_priv(udma_net_dev[i]);
		unregister_netdev(udma_net_dev[i]);
		free_netdev(udma_net_dev[i]);
		udma_net_dev[i] = NULL;
	}
	printk(KERN_INFO "UDMA Network Device Driver exit \n");		
}
module_init(udma_net_init);
module_exit(udma_net_exit);

MODULE_AUTHOR("Intel Corporation");
MODULE_DESCRIPTION("Intel(R) UDMA Network Device Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

/* udma_net.c */

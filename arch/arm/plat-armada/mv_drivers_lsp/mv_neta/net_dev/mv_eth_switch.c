/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
*******************************************************************************/

#include "mvCommon.h"  /* Should be included before mvSysHwConfig */
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/pci.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/version.h>
#include <net/ip.h>
#include <net/xfrm.h>

#include "mvOs.h"
#include "dbg-trace.h"
#include "mvSysHwConfig.h"
#include "boardEnv/mvBoardEnvLib.h"
#ifdef MV_INCLUDE_ETH_COMPLEX
#include "ctrlEnv/mvCtrlEthCompLib.h"
#endif /* MV_INCLUDE_ETH_COMPLEX */
#include "gbe/mvNeta.h"

#include "mv_switch.h"
#include "mv_netdev.h"

extern int mv_net_devs_num;

/* Example: "mv_net_config=4,(00:99:88:88:99:77,0)(00:55:44:55:66:77,1:2:3:4)(00:11:22:33:44:55,),mtu=1500" */
static char			*net_config_str[CONFIG_MV_ETH_PORTS_NUM] = {NULL};
struct mv_eth_switch_config     switch_net_config[CONFIG_MV_ETH_PORTS_NUM];

static int                      mv_eth_switch_started = 0;
unsigned int                    switch_enabled_ports = 0;

/* Required to get the configuration string from the Kernel Command Line */
int mv_eth0_cmdline_config(char *s);
__setup("mv_net_config=", mv_eth0_cmdline_config);

int mv_eth0_cmdline_config(char *s)
{
	net_config_str[MV_ETH_PORT_0] = s;
	return 1;
}

int mv_eth1_cmdline_config(char *s);
__setup("mv_net_config1=", mv_eth1_cmdline_config);

int mv_eth1_cmdline_config(char *s)
{
	net_config_str[MV_ETH_PORT_1] = s;
	return 1;
}

/* Local function prototypes */
static int mv_eth_check_open_bracket(char **p_net_config)
{
	if (**p_net_config == '(') {
		(*p_net_config)++;
		return 0;
	}
	printk(KERN_ERR "Syntax error: could not find opening bracket\n");
	return -EINVAL;
}

static int mv_eth_check_closing_bracket(char **p_net_config)
{
	if (**p_net_config == ')') {
		(*p_net_config)++;
		return 0;
	}
	printk(KERN_ERR "Syntax error: could not find closing bracket\n");
	return -EINVAL;
}

static int mv_eth_check_comma(char **p_net_config)
{
	if (**p_net_config == ',') {
		(*p_net_config)++;
		return 0;
	}
	printk(KERN_ERR "Syntax error: could not find comma\n");
	return -EINVAL;
}

static int mv_eth_netconfig_mac_addr_get(char **p_net_config, int idx, int port)
{
	int     num;
	char *config_str = *p_net_config;
	MV_U32  mac[MV_MAC_ADDR_SIZE];

	/* the MAC address should look like: 00:99:88:88:99:77 */
	/* that is, 6 two-digit numbers, separated by :        */
	num = sscanf(config_str, "%2x:%2x:%2x:%2x:%2x:%2x",
		&mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	if (num == MV_MAC_ADDR_SIZE) {
		while (--num >= 0)
			switch_net_config[port].mac_addr[idx][num] = (mac[num] & 0xFF);

		(*p_net_config) = config_str + 17;
		return 0;
	}
	printk(KERN_ERR "Syntax error while parsing MAC address from command line\n");
	return -EINVAL;
}

static int mv_eth_netconfig_ports_get(char **p_net_config, int idx, int port)
{
	char ch;
	char *config_str = *p_net_config;
	int  sw_port, mask = 0, status = -EINVAL;

	/* the switch port list should look like this: */
	/* example 0: )         - no ports */
	/* example 1: 0)        - single port 0 */
	/* example 2: 1:2:3:4)  - multiple ports */

	while (1) {
		ch = *config_str++;

		if (ch == ')') {
			/* Finished */
			status = 0;
			break;
		}
		sw_port = mvCharToDigit(ch);
		if (sw_port < 0)
			break;

		/* TBD - Check sw_port validity */
		mask |= (1 << sw_port);

		if (*config_str == ':')
			config_str++;
	}
	*p_net_config = config_str;

	if (status == 0) {
		switch_net_config[port].board_port_map[idx] = mask;
		return 0;
	}
	printk(KERN_ERR "Syntax error while parsing switch port mask from command line\n");
	return -EINVAL;
}

/* the mtu value is constructed as follows: */
/* mtu=value                                */
static int  mv_eth_netconfig_mtu_get(char **p_net_config, int port)
{
	unsigned int mtu;

	if (strncmp(*p_net_config, "mtu=", 4) == 0) {
		*p_net_config += 4;
		mtu = strtol(*p_net_config, p_net_config, 10);
		if (mtu > 0) {
			switch_net_config[port].mtu = mtu;
			printk(KERN_ERR "      o MTU set to %d.\n", mtu);
			return 0;
		}
		printk(KERN_ERR "Syntax error while parsing mtu value from command line\n");
		return -EINVAL;
	}

	switch_net_config[port].mtu = 1500;
	printk(KERN_ERR "      o Using default MTU %d\n", switch_net_config[port].mtu);
	return 0;
}

static int mv_eth_netconfig_max_get(char **p_net_config, int port)
{
	char num = **p_net_config;
	int netdev_num;

	netdev_num = mvCharToDigit(num);
	if (netdev_num >= 0) {
		switch_net_config[port].netdev_max = netdev_num;
		(*p_net_config) += 1;
		return 0;
	}
	printk(KERN_ERR "Syntax error while parsing number of netdevs from command line\n");
	return -EINVAL;
}

int mv_eth_switch_config_get(int use_existing_config, int port)
{
	char *p_net_config;
	int i = 0;

	if ((port != MV_ETH_PORT_0) && (port != MV_ETH_PORT_1))	{
		printk(KERN_ERR "%s: invalid port number %d\n", __func__, port);
		return -EINVAL;
	}

	if (!use_existing_config) {
		memset(&(switch_net_config[port]), 0, sizeof((switch_net_config[port])));

		if (net_config_str[port] != NULL) {
			printk(KERN_ERR "      o Using UBoot netconfig string for port %d\n", port);
		} else {
			printk(KERN_ERR "      o Using default netconfig string from Kconfig for port %d\n", port);
			if (port == MV_ETH_PORT_0)
				net_config_str[port] = CONFIG_MV_ETH_SWITCH_NETCONFIG_0;
			else if (port == MV_ETH_PORT_1)
				net_config_str[port] = CONFIG_MV_ETH_SWITCH_NETCONFIG_1;
		}
		printk(KERN_ERR "        net_config_str[%d]: %s\n", port, net_config_str[port]);

		p_net_config = net_config_str[port];
		if (mv_eth_netconfig_max_get(&p_net_config, port))
			return -EINVAL;

		/* check restriction: at least one of the configuration strings must be 0 */
		if ((net_config_str[MV_ETH_PORT_0] != NULL) &&
		    (net_config_str[MV_ETH_PORT_1] != NULL) &&
		    (switch_net_config[MV_ETH_PORT_0].netdev_max != 0) &&
		    (switch_net_config[MV_ETH_PORT_1].netdev_max != 0)) {
			printk(KERN_ERR "%s: cannot have both GbE ports using the Gateway driver, change mv_net_config\n", __func__);
			return -EINVAL;
		}

		if (switch_net_config[port].netdev_max == 0)
			return 1;

		if (switch_net_config[port].netdev_max > CONFIG_MV_ETH_SWITCH_NETDEV_NUM) {
			printk(KERN_ERR "Too large number of netdevs (%d) in command line: cut to %d\n",
				switch_net_config[port].netdev_max, CONFIG_MV_ETH_SWITCH_NETDEV_NUM);
			switch_net_config[port].netdev_max = CONFIG_MV_ETH_SWITCH_NETDEV_NUM;
		}

		if (mv_eth_check_comma(&p_net_config))
			return -EINVAL;

		for (i = 0; (i < CONFIG_MV_ETH_SWITCH_NETDEV_NUM) && (*p_net_config != '\0'); i++) {
			if (mv_eth_check_open_bracket(&p_net_config))
				return -EINVAL;

			if (mv_eth_netconfig_mac_addr_get(&p_net_config, i, port))
				return -EINVAL;

			if (mv_eth_check_comma(&p_net_config))
				return -EINVAL;

			if (mv_eth_netconfig_ports_get(&p_net_config, i, port))
				return -EINVAL;

			switch_net_config[port].netdev_cfg++;

			/* If we have a comma after the closing bracket, then interface */
			/* definition is done.                                          */
			if (*p_net_config == ',') {
				p_net_config++;
				break;
			}
		}

		/* there is a chance the previous loop did not end because a comma was found but because	*/
		/* the maximum number of interfaces was reached, so check for the comma now.		*/
		if (i == CONFIG_MV_ETH_SWITCH_NETDEV_NUM)
			if (mv_eth_check_comma(&p_net_config))
				return -EINVAL;

		if (*p_net_config != '\0') {
			if (mv_eth_netconfig_mtu_get(&p_net_config, port))
				return -EINVAL;
		} else {
			switch_net_config[port].mtu = 1500;
			printk(KERN_ERR "      o Using default MTU %d\n", switch_net_config[port].mtu);
		}

		/* at this point, we have parsed up to CONFIG_MV_ETH_SWITCH_NETDEV_NUM, and the mtu value */
		/* if the net_config string is not finished yet, then its format is invalid */
		if (*p_net_config != '\0') {
			printk(KERN_ERR "Switch netconfig string is too long: %s\n", p_net_config);
			return -EINVAL;
		}
	} else {
		/* leave most of the configuration as-is, but update MAC addresses */
		/* MTU is saved in mv_eth_switch_change_mtu */

		/* Note: at this point, since this is a re-init, mv_eth_switch_netdev_first */
		/* and mv_eth_switch_netdev_last, as well as mv_net_devs[], are valid.      */
		for (i = mv_eth_switch_netdev_first; i <= mv_eth_switch_netdev_last; i++) {
			if (mv_net_devs[i] != NULL)
				memcpy(switch_net_config[port].mac_addr[i - mv_eth_switch_netdev_first],
					mv_net_devs[i]->dev_addr, MV_MAC_ADDR_SIZE);
		}

		if (switch_net_config[port].netdev_max == 0)
			return 1;
	}

	return 0;
}

int    mv_eth_switch_set_mac_addr(struct net_device *dev, void *mac)
{
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);
	struct sockaddr *addr = (struct sockaddr *)mac;

	if (!is_valid_ether_addr(addr->sa_data))
		return -EADDRNOTAVAIL;

	/* remove old mac addr from VLAN DB */
	mv_switch_mac_addr_set(dev->dev_addr, MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id), (1 << dev_priv->cpu_port), 0);

	memcpy(dev->dev_addr, addr->sa_data, MV_MAC_ADDR_SIZE);

	/* add new mac addr to VLAN DB */
	mv_switch_mac_addr_set(dev->dev_addr, MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id), (1 << dev_priv->cpu_port), 1);

	printk(KERN_ERR "mv_eth_switch: %s change mac address to %02x:%02x:%02x:%02x:%02x:%02x\n",
		dev->name, *(dev->dev_addr), *(dev->dev_addr+1), *(dev->dev_addr+2),
		*(dev->dev_addr+3), *(dev->dev_addr+4), *(dev->dev_addr+5));

	return 0;
}

void    mv_eth_switch_set_multicast_list(struct net_device *dev)
{
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);

	if (dev->flags & IFF_PROMISC) {
		/* promiscuous mode - connect the CPU port to the VLAN (port based + 802.1q) */
		/* printk(KERN_ERR "mv_eth_switch: setting promiscuous mode\n"); */
		if (mv_switch_promisc_set(dev_priv->vlan_grp_id, dev_priv->port_map, dev_priv->cpu_port, 1))
			printk(KERN_ERR "mv_switch_promisc_set to 1 failed\n");
	} else {
		/* not in promiscuous mode - disconnect the CPU port to the VLAN (port based + 802.1q) */
		if (mv_switch_promisc_set(dev_priv->vlan_grp_id, dev_priv->port_map, dev_priv->cpu_port, 0))
			printk(KERN_ERR "mv_switch_promisc_set to 0 failed\n");

		if (dev->flags & IFF_ALLMULTI) {
			/* allmulticast - not supported. There is no way to tell the Switch to accept only	*/
			/* the multicast addresses but not Unicast addresses, so the alternatives are:	*/
			/* 1) Don't support multicast and do nothing					*/
			/* 2) Support multicast with same implementation as promiscuous			*/
			/* 3) Don't rely on Switch for MAC filtering, but use PnC			*/
			/* Currently option 1 is selected						*/
			printk(KERN_ERR "mv_eth_switch: setting all-multicast mode is not supported\n");
		}

		/* Add or delete specific multicast addresses:						*/
		/* Linux provides a list of the current multicast addresses for the device.		*/
		/* First, we delete all the multicast addresses in the ATU.				*/
		/* Then we add the specific multicast addresses Linux provides.				*/
		if (mv_switch_all_multicasts_del(MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id)))
			printk(KERN_ERR "mv_eth_switch: mv_switch_all_multicasts_del failed\n");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 34)
		if (!netdev_mc_empty(dev)) {
			struct netdev_hw_addr *ha;

			netdev_for_each_mc_addr(ha, dev) {
				mv_switch_mac_addr_set(ha->addr,
					MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id),
					(dev_priv->port_map | (1 << dev_priv->cpu_port)), 1);
			}
		}
#else
		{
			int i;
			struct dev_mc_list *curr_addr = dev->mc_list;

			/* accept specific multicasts */
			for (i = 0; i < dev->mc_count; i++, curr_addr = curr_addr->next) {
				if (!curr_addr)
					break;

				/*
				printk(KERN_ERR "Setting MC = %02X:%02X:%02X:%02X:%02X:%02X\n",
				curr_addr->dmi_addr[0], curr_addr->dmi_addr[1], curr_addr->dmi_addr[2],
				curr_addr->dmi_addr[3], curr_addr->dmi_addr[4], curr_addr->dmi_addr[5]);
				*/
				mv_switch_mac_addr_set(curr_addr->dmi_addr,
					MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id),
					(dev_priv->port_map | (1 << dev_priv->cpu_port)), 1);
			}
		}
#endif /* KERNEL_VERSION >= 2.6.34 */
	}
}

int     mv_eth_switch_change_mtu(struct net_device *dev, int mtu)
{
	int i;
	struct eth_port *priv = MV_ETH_PRIV(dev);

	/* All gateway interfaces must be down before changing MTU */
	for (i = mv_eth_switch_netdev_first; i <= mv_eth_switch_netdev_last; i++) {
		if ((mv_net_devs[i] != NULL) && (netif_running(mv_net_devs[i]))) {
			printk(KERN_ERR "All gateway interfaces must be down before changing MTU. %s is not down\n", mv_net_devs[i]->name);
			return -EPERM;
		}
	}

	if (dev->mtu != mtu) {
		int old_mtu = dev->mtu;

		mtu = mv_eth_check_mtu_valid(dev, mtu);
		if (mtu < 0)
			return -EPERM;

		if (mv_eth_change_mtu_internals(dev, mtu))
			return -EPERM;

		printk(KERN_NOTICE "%s: change mtu %d (pkt-size %d) to %d (pkt-size %d)\n",
			dev->name, old_mtu, RX_PKT_SIZE(old_mtu),
			dev->mtu, RX_PKT_SIZE(dev->mtu));
	}

	if (switch_net_config[priv->port].mtu != mtu) {
		mv_switch_jumbo_mode_set(RX_PKT_SIZE(mtu));
		switch_net_config[priv->port].mtu = mtu;
	}

	return 0;
}

int    mv_eth_switch_start(struct net_device *dev)
{
	struct eth_port	*priv = MV_ETH_PRIV(dev);
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);
	unsigned char broadcast[MV_MAC_ADDR_SIZE] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	int i;
	int group;

	/* Check that MTU value is identical for all gateway interfaces */
	for (i = mv_eth_switch_netdev_first; i <= mv_eth_switch_netdev_last; i++) {
		if ((mv_net_devs[i] != NULL) && (mv_net_devs[i]->mtu != dev->mtu)) {
			printk(KERN_ERR "All gateway devices must have same MTU\n");
			return -EPERM;
		}
	}

	/* in default link is down */
	netif_carrier_off(dev);

	/* Stop the TX queue - it will be enabled upon PHY status change after link-up interrupt/timer */
	netif_tx_stop_all_queues(dev);

	/* start upper layer accordingly with ports_map */
#ifdef CONFIG_MV_ETH_SWITCH_LINK
	dev_priv->link_map = 0;
	mv_switch_link_update_event(dev_priv->port_map, 1);
#else
	dev_priv->link_map = dev_priv->port_map;
#endif /* CONFIG_MV_ETH_SWITCH_LINK */

	if (mv_eth_switch_started == 0)	{
		/* fill rx buffers, start rx/tx activity, set coalescing */
		if (mv_eth_start_internals(priv, dev->mtu) != 0) {
			printk(KERN_ERR "%s: start internals failed\n", dev->name);
			goto error;
		}

		/* enable polling on the port, must be used after netif_poll_disable */
		for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++)
			napi_enable(priv->napiGroup[group]);

		/* connect to port interrupt line */
		if (request_irq(dev->irq, mv_eth_isr, IRQF_DISABLED|IRQF_SAMPLE_RANDOM, "mv_eth", priv)) {
			printk(KERN_ERR "cannot request irq %d for %s port %d\n",
				dev->irq, dev->name, priv->port);
			for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++)
				napi_disable(priv->napiGroup[group]);
			goto error;
		}

		/* unmask interrupts */
		mv_eth_interrupts_unmask(priv);
		smp_call_function_many(cpu_online_mask, (smp_call_func_t)mv_eth_interrupts_unmask, (void *)priv, 1);
	}

	mv_eth_switch_started++;

	/* Add our MAC addr to the VLAN DB at switch level to forward packets with this DA   */
	/* to CPU port by using the tunneling feature. The device is always in promisc mode. */
	mv_switch_mac_addr_set(dev->dev_addr, MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id), (1 << dev_priv->cpu_port), 1);

	/* We also need to allow L2 broadcasts comming up for this interface */
	mv_switch_mac_addr_set(broadcast, MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id),
			(dev_priv->port_map | (1 << dev_priv->cpu_port)), 1);

	printk(KERN_ERR "%s: started (on switch)\n", dev->name);
	return 0;

error:
	printk(KERN_ERR "%s: start failed\n", dev->name);
	return -1;
}

int     mv_eth_switch_stop(struct net_device *dev)
{
	struct eth_port *priv = MV_ETH_PRIV(dev);
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);
	struct cpu_ctrl *cpuCtrl;
	int group, cpu;

	/* stop upper layer */
	netif_carrier_off(dev);
	netif_tx_stop_all_queues(dev);

	/* stop switch from forwarding packets from this VLAN toward CPU port */
	mv_switch_atu_db_flush(MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id));

	/* It is possible that the interface is in promiscuous mode */
	/* If so, the CPU port is connected with port based VLAN to the other ports, and */
	/* we must disconnect it now to stop the Switch from forwarding packets to the CPU */
	/* when the interface is down. */
	/* mv_eth_switch_set_multicast_list will be called anyway by Linux when we do ifconfig up */
	/* and will re-set the promiscuous feature if needed */
	if (dev->flags & IFF_PROMISC) {
		if (mv_switch_promisc_set(dev_priv->vlan_grp_id, dev_priv->port_map, dev_priv->cpu_port, 0))
			printk(KERN_ERR "mv_switch_promisc_set to 0 failed\n");
	}
	mv_eth_switch_started--;

	if (mv_eth_switch_started == 0)	{
		/* first make sure that the port finished its Rx polling - see tg3 */
		/* otherwise it may cause issue in SMP, one CPU is here and the other is doing the polling */
		/* and both of it are messing with the descriptors rings!! */
		for (group = 0; group < CONFIG_MV_ETH_NAPI_GROUPS; group++)
			napi_disable(priv->napiGroup[group]);

		/* stop tx/rx activity, mask all interrupts, relese skb in rings,*/
		mv_eth_stop_internals(priv);
		for_each_possible_cpu(cpu) {
			cpuCtrl = priv->cpu_config[cpu];
			del_timer(&cpuCtrl->tx_done_timer);
			clear_bit(MV_ETH_F_TX_DONE_TIMER_BIT, &(cpuCtrl->flags));
			del_timer(&cpuCtrl->cleanup_timer);
			clear_bit(MV_ETH_F_CLEANUP_TIMER_BIT, &(cpuCtrl->flags));
		}
		if (dev->irq != 0)
			free_irq(dev->irq, priv);
	}
	printk(KERN_NOTICE "%s: stopped\n", dev->name);

	return 0;
}

#ifdef CONFIG_MV_ETH_SWITCH_LINK

void mv_eth_switch_interrupt_unmask(int qsgmii_module, int gephy_on_port)
{
#ifdef MV_INCLUDE_ETH_COMPLEX
	MV_U32 reg;

	reg = MV_REG_READ(MV_ETHCOMP_INT_MAIN_MASK_REG);

	if (qsgmii_module) {
		reg |= (MV_ETHCOMP_PCS0_LINK_INT_MASK |
			MV_ETHCOMP_PCS1_LINK_INT_MASK |
			MV_ETHCOMP_PCS2_LINK_INT_MASK |
			MV_ETHCOMP_PCS3_LINK_INT_MASK);
	}

	if (gephy_on_port >= 0)
		reg |= MV_ETHCOMP_GEPHY_INT_MASK;

	reg |= MV_ETHCOMP_SWITCH_INT_MASK;

	MV_REG_WRITE(MV_ETHCOMP_INT_MAIN_MASK_REG, reg);
#endif /* MV_INCLUDE_ETH_COMPLEX */
}

void mv_eth_switch_interrupt_clear(int qsgmii_module, int gephy_on_port)
{
#ifdef MV_INCLUDE_ETH_COMPLEX
	MV_U32 reg;

	reg = MV_REG_READ(MV_ETHCOMP_INT_MAIN_CAUSE_REG);

	if (qsgmii_module) {
		reg &= ~(MV_ETHCOMP_PCS0_LINK_INT_MASK |
			 MV_ETHCOMP_PCS1_LINK_INT_MASK |
			 MV_ETHCOMP_PCS2_LINK_INT_MASK |
			 MV_ETHCOMP_PCS3_LINK_INT_MASK);
	}

	if (gephy_on_port >= 0)
		reg &= ~MV_ETHCOMP_GEPHY_INT_MASK;

	reg &= ~MV_ETHCOMP_SWITCH_INT_MASK;

	MV_REG_WRITE(MV_ETHCOMP_INT_MAIN_CAUSE_REG, reg);
#endif /* MV_INCLUDE_ETH_COMPLEX */
}

void mv_eth_switch_update_link(unsigned int p, unsigned int link_up)
{
	struct eth_netdev *dev_priv = NULL;
	struct eth_port *priv = NULL;
	int i = 0;
	unsigned int prev_ports_link = 0;

	for (i = 0; i < mv_net_devs_num; i++) {

		if (mv_net_devs[i] == NULL)
			break;

		priv = MV_ETH_PRIV(mv_net_devs[i]);
		if (priv == NULL)
			break;

		if (!(priv->flags & (MV_ETH_F_SWITCH | MV_ETH_F_EXT_SWITCH)))
			continue;

		dev_priv = MV_DEV_PRIV(mv_net_devs[i]);
		if (dev_priv == NULL)
			break;

		if ((dev_priv->port_map & (1 << p)) == 0)
			continue;

		prev_ports_link = dev_priv->link_map;

		if (link_up)
			dev_priv->link_map |= (1 << p);
		else
			dev_priv->link_map &= ~(1 << p);

		if ((prev_ports_link != 0) && (dev_priv->link_map == 0) && netif_running(mv_net_devs[i])) {
			/* link down */
			netif_carrier_off(mv_net_devs[i]);
			netif_tx_stop_all_queues(mv_net_devs[i]);
			printk(KERN_ERR "%s: link down\n", mv_net_devs[i]->name);
		} else if ((prev_ports_link == 0) && (dev_priv->link_map != 0) && netif_running(mv_net_devs[i])) {
			/* link up */
			if (mv_eth_ctrl_is_tx_enabled(priv) == 1) {
				netif_carrier_on(mv_net_devs[i]);
				netif_tx_wake_all_queues(mv_net_devs[i]);
				printk(KERN_ERR "%s: link up\n", mv_net_devs[i]->name);
			}
		}
	}
}

#endif /* CONFIG_MV_ETH_SWITCH_LINK */

int     mv_eth_switch_port_add(struct net_device *dev, int port)
{
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);
	struct eth_port	*priv = MV_ETH_PRIV(dev);
	int i, switch_port, err = 0;

	if (dev_priv == NULL) {
		printk(KERN_ERR "%s is not connected to the switch\n", dev->name);
		return 1;
	}

	if (netif_running(dev)) {
		printk(KERN_ERR "%s must be down to change switch ports map\n", dev->name);
		return 1;
	}

	switch_port = mvBoardSwitchPortGet(0, port);

	if (switch_port < 0) {
		printk(KERN_ERR "Switch port %d can't be added\n", port);
		return 1;
	}

	if (MV_BIT_CHECK(switch_enabled_ports, switch_port)) {
		printk(KERN_ERR "Switch port %d is already enabled\n", port);
		return 0;
	}

	/* Update data base */
	dev_priv->port_map |= (1 << switch_port);
	for (i = mv_eth_switch_netdev_first; i <= mv_eth_switch_netdev_last; i++) {
		if ((mv_net_devs[i] != NULL) && (mv_net_devs[i] == dev))
			switch_net_config[priv->port].board_port_map[i - mv_eth_switch_netdev_first] |= (1 << switch_port);
	}
	switch_enabled_ports |= (1 << switch_port);
	dev_priv->tx_vlan_mh = cpu_to_be16((MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id) << 12) | dev_priv->port_map);

	err = mv_switch_port_add(switch_port, dev_priv->vlan_grp_id, dev_priv->port_map);
	if (!err)
		printk(KERN_ERR "%s: Switch port #%d mapped\n", dev->name, port);

	return err;
}

int     mv_eth_switch_port_del(struct net_device *dev, int port)
{
	struct eth_netdev *dev_priv = MV_DEV_PRIV(dev);
	struct eth_port	*priv = MV_ETH_PRIV(dev);
	int i, switch_port, err = 0;

	if (dev_priv == NULL) {
		printk(KERN_ERR "%s is not connected to the switch\n", dev->name);
		return 1;
	}

	if (netif_running(dev)) {
		printk(KERN_ERR "%s must be down to change switch ports map\n", dev->name);
		return 1;
	}

	switch_port = mvBoardSwitchPortGet(0, port);

	if (switch_port < 0) {
		printk(KERN_ERR "Switch port %d can't be added\n", port);
		return 1;
	}

	if (!MV_BIT_CHECK(switch_enabled_ports, switch_port)) {
		printk(KERN_ERR "Switch port %d is already disabled\n", port);
		return 0;
	}

	if (!MV_BIT_CHECK(dev_priv->port_map, switch_port)) {
		printk(KERN_ERR "Switch port %d is not mapped on %s\n", port, dev->name);
		return 0;
	}

	/* Update data base */
	dev_priv->port_map &= ~(1 << switch_port);
	for (i = mv_eth_switch_netdev_first; i <= mv_eth_switch_netdev_last; i++) {
		if ((mv_net_devs[i] != NULL) && (mv_net_devs[i] == dev))
			switch_net_config[priv->port].board_port_map[i - mv_eth_switch_netdev_first] &= ~(1 << switch_port);
	}
	dev_priv->link_map &= ~(1 << switch_port);
	switch_enabled_ports &= ~(1 << switch_port);
	dev_priv->tx_vlan_mh = cpu_to_be16((MV_SWITCH_VLAN_TO_GROUP(dev_priv->vlan_grp_id) << 12) | dev_priv->port_map);

	err = mv_switch_port_del(switch_port, dev_priv->vlan_grp_id, dev_priv->port_map);
	if (!err)
		printk(KERN_ERR "%s: Switch port #%d unmapped\n", dev->name, port);

	return err;
}

void    mv_eth_switch_status_print(int port)
{
	int i;
	struct eth_port *pp = mv_eth_port_by_id(port);
	struct net_device *dev;

	if (pp->flags & MV_ETH_F_SWITCH) {
		printk(KERN_ERR "ethPort=%d: mv_eth_switch status - pp=%p, flags=0x%lx\n", port, pp, pp->flags);

		printk(KERN_ERR "mtu=%d, netdev_max=%d, netdev_cfg=%d, first=%d, last=%d\n",
			switch_net_config[port].mtu, switch_net_config[port].netdev_max, switch_net_config[port].netdev_cfg,
			mv_eth_switch_netdev_first, mv_eth_switch_netdev_last);

		for (i = 0; i < switch_net_config[port].netdev_cfg; i++) {
			printk(KERN_ERR "MAC="MV_MACQUAD_FMT", board_port_map=0x%x\n",
				MV_MACQUAD(switch_net_config[port].mac_addr[i]), switch_net_config[port].board_port_map[i]);
		}
		for (i = mv_eth_switch_netdev_first; i <= mv_eth_switch_netdev_last; i++) {
			dev = mv_eth_netdev_by_id(i);
			if (dev)
				mv_eth_netdev_print(dev);
		}
	} else
		printk(KERN_ERR "ethPort=%d: switch is not connected - pp=%p, flags=0x%lx\n", port, pp, pp->flags);
}

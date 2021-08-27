#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/mii.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 24)
#include <asm/arch/system.h>
#else
#include <asm/system.h>
#endif

#include "mvOs.h"
#include "mvDebug.h"
#include "mvCommon.h"
#include "mvEthPhy.h"
#include "gmac/mvEthGmacApi.h"

#include "gbe/mvPp2Gbe.h"
#include "bm/mvBm.h"

#include "mv_netdev.h"

#include "prs/mvPp2Prs.h"

#include "wol/mvPp2Wol.h"

#define MV_ETH_TOOL_AN_TIMEOUT	5000

struct mv_pp2_tool_stats {
	char stat_string[ETH_GSTRING_LEN];
	int stat_offset;
};

#define MV_ETH_TOOL_STAT(m)	offsetof(struct eth_port, m)

static const struct mv_pp2_tool_stats mv_pp2_tool_global_strings_stats[] = {
#ifdef CONFIG_MV_PP2_STAT_ERR
	{"rx_error", MV_ETH_TOOL_STAT(stats.rx_error)},
	{"tx_timeout", MV_ETH_TOOL_STAT(stats.tx_timeout)},
	{"ext_stack_empty", MV_ETH_TOOL_STAT(stats.ext_stack_empty)},
	{"ext_stack_full", MV_ETH_TOOL_STAT(stats.ext_stack_full)},
	{"state_err", MV_ETH_TOOL_STAT(stats.state_err)},
#endif
#ifdef CONFIG_MV_PP2_STAT_INF
	{"tx_done", MV_ETH_TOOL_STAT(stats.tx_done)},
	{"link", MV_ETH_TOOL_STAT(stats.link)},
	{"netdev_stop", MV_ETH_TOOL_STAT(stats.netdev_stop)},
	{"rx_buf_hdr", MV_ETH_TOOL_STAT(stats.rx_buf_hdr)},
#ifdef CONFIG_MV_PP2_RX_SPECIAL
	{"rx_special", MV_ETH_TOOL_STAT(stats.rx_special)},
#endif
#ifdef CONFIG_MV_PP2_TX_SPECIAL
	{"tx_special", MV_ETH_TOOL_STAT(stats.tx_special)},
#endif
#endif
#ifdef CONFIG_MV_PP2_STAT_DBG
	{"rx_tagged", MV_ETH_TOOL_STAT(stats.rx_tagged)},
	{"rx_netif", MV_ETH_TOOL_STAT(stats.rx_netif)},
	{"rx_gro", MV_ETH_TOOL_STAT(stats.rx_gro)},
	{"rx_gro_bytes", MV_ETH_TOOL_STAT(stats.rx_gro_bytes)},
	{"rx_drop_sw", MV_ETH_TOOL_STAT(stats.rx_drop_sw)},
	{"rx_csum_hw", MV_ETH_TOOL_STAT(stats.rx_csum_hw)},
	{"rx_csum_sw", MV_ETH_TOOL_STAT(stats.rx_csum_sw)},
	{"tx_csum_hw", MV_ETH_TOOL_STAT(stats.tx_csum_hw)},
	{"tx_csum_sw", MV_ETH_TOOL_STAT(stats.tx_csum_sw)},
	{"tx_skb_free", MV_ETH_TOOL_STAT(stats.tx_skb_free)},
	{"tx_sg", MV_ETH_TOOL_STAT(stats.tx_sg)},
	{"tx_tso", MV_ETH_TOOL_STAT(stats.tx_tso)},
	{"tx_tso_no_resource", MV_ETH_TOOL_STAT(stats.tx_tso_no_resource)},
	{"tx_tso_bytes", MV_ETH_TOOL_STAT(stats.tx_tso_bytes)},
	{"ext_stack_put", MV_ETH_TOOL_STAT(stats.ext_stack_put)},
	{"ext_stack_get", MV_ETH_TOOL_STAT(stats.ext_stack_get)},
#endif
	{"rate_current", MV_ETH_TOOL_STAT(rate_current)},
};

static const struct mv_pp2_tool_stats mv_pp2_tool_cpu_strings_stats[] = {
#ifdef CONFIG_MV_ETH_STATS_DEBUG
	{"irq", MV_ETH_TOOL_STAT(stats.irq)},
	{"irq_err", MV_ETH_TOOL_STAT(stats.irq_err)},
	{"poll", MV_ETH_TOOL_STAT(stats.poll)},
	{"poll_exit", MV_ETH_TOOL_STAT(stats.poll_exit)},
	{"tx_done_timer_event", MV_ETH_TOOL_STAT(stats.tx_done_timer_event)},
	{"tx_done_timer_add", MV_ETH_TOOL_STAT(stats.tx_done_timer_add)},
#endif  
};

static const struct mv_pp2_tool_stats mv_pp2_tool_rx_queue_strings_stats[] = {
#ifdef CONFIG_MV_PP2_STAT_DBG
	{"rxq", MV_ETH_TOOL_STAT(stats.rxq)},
#endif  
};

static const struct mv_pp2_tool_stats mv_pp2_tool_tx_queue_strings_stats[] = {
};

#define MV_ETH_TOOL_CPU_STATS_LEN	\
	(sizeof(mv_pp2_tool_cpu_strings_stats) / sizeof(struct mv_pp2_tool_stats))

#define MV_ETH_TOOL_RX_QUEUE_STATS_LEN	\
	(sizeof(mv_pp2_tool_rx_queue_strings_stats) / sizeof(struct mv_pp2_tool_stats))

#define MV_ETH_TOOL_TX_QUEUE_STATS_LEN	\
	(sizeof(mv_pp2_tool_tx_queue_strings_stats) / sizeof(struct mv_pp2_tool_stats))

#define MV_ETH_TOOL_QUEUE_STATS_LEN	\
	((MV_ETH_TOOL_RX_QUEUE_STATS_LEN * CONFIG_MV_PP2_RXQ) + \
	(MV_ETH_TOOL_TX_QUEUE_STATS_LEN * CONFIG_MV_PP2_TXQ))

#define MV_ETH_TOOL_GLOBAL_STATS_LEN	\
	(sizeof(mv_pp2_tool_global_strings_stats) / sizeof(struct mv_pp2_tool_stats))

#define MV_ETH_TOOL_STATS_LEN		\
	(MV_ETH_TOOL_GLOBAL_STATS_LEN + MV_ETH_TOOL_CPU_STATS_LEN + MV_ETH_TOOL_QUEUE_STATS_LEN)

#ifdef MY_ABC_HERE
extern spinlock_t          mii_lock;
#endif

#ifdef MY_ABC_HERE

MV_U32 syno_wol_support(struct eth_port *pp)
{
	if (MV_PHY_ID_151X == pp->phy_chip) {
		return WAKE_MAGIC;
	}

	return 0;
}

static void syno_get_wol(struct net_device *dev, struct ethtool_wolinfo *wol)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);

	wol->supported = syno_wol_support(pp);
	wol->wolopts = pp->wol;
}

static int syno_set_wol(struct net_device *dev, struct ethtool_wolinfo *wol)
{
	struct eth_port *pp = MV_ETH_PRIV(dev);

	if ((wol->wolopts & ~syno_wol_support(pp))) {
		return -EOPNOTSUPP;
	}

	pp->wol = wol->wolopts;
	return 0;
}

#define MV_ETH_TOOL_PHY_PAGE_ADDR_REG	22
int mv_eth_tool_read_phy_reg(int phy_addr, u16 page, u16 reg, u16 *val)
{
	unsigned long 	flags;
	MV_STATUS 	status = 0;

	spin_lock_irqsave(&mii_lock, flags);
	 
	if (!mvEthPhyRegWrite(phy_addr, MV_ETH_TOOL_PHY_PAGE_ADDR_REG, page)) {
		status = mvEthPhyRegRead(phy_addr, reg, val);
	}
	spin_unlock_irqrestore(&mii_lock, flags);

	return status;
}

int mv_eth_tool_write_phy_reg(int phy_addr, u16 page, u16 reg, u16 data)
{
	unsigned long   flags;
	MV_STATUS 	status = 0;

	spin_lock_irqsave(&mii_lock, flags);
	 
	if (!mvEthPhyRegWrite(phy_addr, MV_ETH_TOOL_PHY_PAGE_ADDR_REG,
						(unsigned int)page)) {
		status = mvEthPhyRegWrite(phy_addr, reg, data);
	}
	spin_unlock_irqrestore(&mii_lock, flags);

	return status;
}
#endif
 
int mv_pp2_eth_tool_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	u16			lp_ad, stat1000;
	MV_U32			phy_addr;
	MV_ETH_PORT_SPEED 	speed;
	MV_ETH_PORT_DUPLEX 	duplex;
	MV_ETH_PORT_STATUS      status;

	if ((priv == NULL) || (MV_PP2_IS_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return -EOPNOTSUPP;
	}

	cmd->supported = (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full | SUPPORTED_100baseT_Half
			| SUPPORTED_100baseT_Full | SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII
			| SUPPORTED_1000baseT_Full);

	phy_addr = priv->plat_data->phy_addr;

	mvGmacLinkStatus(priv->port, &status);

	if (status.linkup != MV_TRUE) {
		 
		cmd->speed  = priv->speed_cfg;
		cmd->duplex = priv->duplex_cfg;
	} else {
		switch (status.speed) {
		case MV_ETH_SPEED_1000:
			cmd->speed = SPEED_1000;
			break;
		case MV_ETH_SPEED_100:
			cmd->speed = SPEED_100;
			break;
		case MV_ETH_SPEED_10:
			cmd->speed = SPEED_10;
			break;
		default:
			return -EINVAL;
		}
		if (status.duplex == MV_ETH_DUPLEX_FULL)
			cmd->duplex = 1;
		else
			cmd->duplex = 0;
	}

	cmd->port = PORT_MII;
	cmd->phy_address = phy_addr;
	cmd->transceiver = XCVR_INTERNAL;
	 
	mvGmacSpeedDuplexGet(priv->port, &speed, &duplex);
	if (speed == MV_ETH_SPEED_AN && duplex == MV_ETH_DUPLEX_AN) {
		cmd->lp_advertising = cmd->advertising = 0;
		cmd->autoneg = AUTONEG_ENABLE;
		mvEthPhyAdvertiseGet(phy_addr, (MV_U16 *)&(cmd->advertising));

		mvEthPhyRegRead(phy_addr, MII_LPA, &lp_ad);
		if (lp_ad & LPA_LPACK)
			cmd->lp_advertising |= ADVERTISED_Autoneg;
		if (lp_ad & ADVERTISE_10HALF)
			cmd->lp_advertising |= ADVERTISED_10baseT_Half;
		if (lp_ad & ADVERTISE_10FULL)
			cmd->lp_advertising |= ADVERTISED_10baseT_Full;
		if (lp_ad & ADVERTISE_100HALF)
			cmd->lp_advertising |= ADVERTISED_100baseT_Half;
		if (lp_ad & ADVERTISE_100FULL)
			cmd->lp_advertising |= ADVERTISED_100baseT_Full;

		mvEthPhyRegRead(phy_addr, MII_STAT1000, &stat1000);
		if (stat1000 & LPA_1000HALF)
			cmd->lp_advertising |= ADVERTISED_1000baseT_Half;
		if (stat1000 & LPA_1000FULL)
			cmd->lp_advertising |= ADVERTISED_1000baseT_Full;
	} else
		cmd->autoneg = AUTONEG_DISABLE;

	return 0;
}

int mv_pp2_eth_tool_restore_settings(struct net_device *netdev)
{
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	int			phy_speed, phy_duplex;
	MV_U32			phy_addr = priv->plat_data->phy_addr;
	MV_ETH_PORT_SPEED	mac_speed;
	MV_ETH_PORT_DUPLEX	mac_duplex;
	int			err = -EINVAL;

	 if (priv == NULL)
		 return -EOPNOTSUPP;

	switch (priv->speed_cfg) {
	case SPEED_10:
		phy_speed  = 0;
		mac_speed = MV_ETH_SPEED_10;
		break;
	case SPEED_100:
		phy_speed  = 1;
		mac_speed = MV_ETH_SPEED_100;
		break;
	case SPEED_1000:
		phy_speed  = 2;
		mac_speed = MV_ETH_SPEED_1000;
		break;
	default:
		return -EINVAL;
	}

	switch (priv->duplex_cfg) {
	case DUPLEX_HALF:
		phy_duplex = 0;
		mac_duplex = MV_ETH_DUPLEX_HALF;
		break;
	case DUPLEX_FULL:
		phy_duplex = 1;
		mac_duplex = MV_ETH_DUPLEX_FULL;
		break;
	default:
		return -EINVAL;
	}

	if (priv->autoneg_cfg == AUTONEG_ENABLE) {
		err = mvGmacSpeedDuplexSet(priv->port, MV_ETH_SPEED_AN, MV_ETH_DUPLEX_AN);
		if (!err)
			err = mvEthPhyAdvertiseSet(phy_addr, priv->advertise_cfg);
		 
		if (!err) {
			err = mvEthPhyRestartAN(phy_addr, MV_ETH_TOOL_AN_TIMEOUT);
			if (err == MV_TIMEOUT) {
				MV_ETH_PORT_STATUS ps;

				mvGmacLinkStatus(priv->port, &ps);

				if (!ps.linkup)
					err = 0;
			}
		}
	} else if (priv->autoneg_cfg == AUTONEG_DISABLE) {
		err = mvEthPhyDisableAN(phy_addr, phy_speed, phy_duplex);
		if (!err)
			err = mvGmacSpeedDuplexSet(priv->port, mac_speed, mac_duplex);
	} else {
		err = -EINVAL;
	}

	return err;
}

int mv_pp2_eth_tool_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct eth_port *priv = MV_ETH_PRIV(dev);
	int _speed, _duplex, _autoneg, _advertise, err;

	if ((priv == NULL) || (MV_PP2_IS_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, dev->name);
		return -EOPNOTSUPP;
	}

	_duplex  = priv->duplex_cfg;
	_speed   = priv->speed_cfg;
	_autoneg = priv->autoneg_cfg;
	_advertise = priv->advertise_cfg;

	priv->duplex_cfg = cmd->duplex;
	priv->speed_cfg = cmd->speed;
	priv->autoneg_cfg = cmd->autoneg;
	priv->advertise_cfg = cmd->advertising;
	err = mv_pp2_eth_tool_restore_settings(dev);

	if (err) {
		priv->duplex_cfg = _duplex;
		priv->speed_cfg = _speed;
		priv->autoneg_cfg = _autoneg;
		priv->advertise_cfg = _advertise;
	}
	return err;
}

int mv_pp2_eth_tool_get_regs_len(struct net_device *netdev)
{
#define MV_ETH_TOOL_REGS_LEN 42

	return (MV_ETH_TOOL_REGS_LEN * sizeof(uint32_t));
}

void mv_pp2_eth_tool_get_wol(struct net_device *netdev,
			 struct ethtool_wolinfo *wolinfo)
{
	struct eth_port	*priv = MV_ETH_PRIV(netdev);

	if (priv == NULL) {
		pr_err("%s is not supported on %s\n", __func__, netdev->name);
		return;
	}

	wolinfo->supported = WAKE_ARP | WAKE_UCAST | WAKE_MAGIC;
	wolinfo->wolopts = 0;

	if (priv->wol & (MV_PP2_WOL_ARP_IP_MASK(0) | MV_PP2_WOL_ARP_IP_MASK(1)))
		wolinfo->wolopts |= WAKE_ARP;

	if (priv->wol & MV_PP2_WOL_UCAST_MASK)
		wolinfo->wolopts |= WAKE_UCAST;
	if (priv->wol & MV_PP2_WOL_MAGIC_PTRN_MASK)
		wolinfo->wolopts |= WAKE_MAGIC;
}

int mv_pp2_eth_tool_set_wol(struct net_device *netdev,
			 struct ethtool_wolinfo *wolinfo)
{
	int ret;
	struct eth_port	*priv = MV_ETH_PRIV(netdev);

	if (priv == NULL) {
		pr_err("%s is not supported on %s\n", __func__, netdev->name);
		return -EOPNOTSUPP;
	}

	if (wolinfo->wolopts & (WAKE_PHY | WAKE_MCAST | WAKE_BCAST | WAKE_MAGICSECURE))
		return -EOPNOTSUPP;

	priv->wol = 0;
	 
	ret = mvPp2WolWakeup();
	if (ret)
		return ret;

	if (wolinfo->wolopts & WAKE_UCAST) {
		priv->wol |= MV_PP2_WOL_UCAST_MASK;
		 
		ret = mvPp2WolUcastEventSet(WOL_EVENT_EN);
		if (ret)
			return ret;
	}

	if (wolinfo->wolopts & WAKE_ARP) {
		 
		priv->wol |= MV_PP2_WOL_ARP_IP_MASK((priv->port) % MV_PP2_WOL_ARP_IP_NUM);
		 
		ret = mvPp2WolArpEventSet((priv->port) % MV_PP2_WOL_ARP_IP_NUM, WOL_EVENT_EN);
		if (ret)
			return ret;
	}

	if (wolinfo->wolopts & WAKE_MAGIC) {
		priv->wol |= MV_PP2_WOL_MAGIC_PTRN_MASK;
		 
		ret = mvPp2WolMagicDaSet(netdev->dev_addr);
		if (ret)
			return ret;
		 
		ret = mvPp2WolMagicEventSet(WOL_EVENT_EN);
		if (ret)
			return ret;
	}

	return 0;
}

void mv_pp2_eth_tool_get_drvinfo(struct net_device *netdev,
			     struct ethtool_drvinfo *info)
{
	strcpy(info->driver, "mv_eth");
	strcpy(info->fw_version, "N/A");
	strcpy(info->bus_info, "Mbus");
	info->n_stats = MV_ETH_TOOL_STATS_LEN;
	info->testinfo_len = 0;
	info->regdump_len = mv_pp2_eth_tool_get_regs_len(netdev);
	info->eedump_len = 0;
}

void mv_pp2_eth_tool_get_regs(struct net_device *netdev,
			  struct ethtool_regs *regs, void *p)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);
	uint32_t	*regs_buff = p;

	if ((priv == NULL) || MV_PP2_IS_PON_PORT(priv->port)) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return;
	}

	memset(p, 0, MV_ETH_TOOL_REGS_LEN * sizeof(uint32_t));

	regs->version = priv->plat_data->ctrl_rev;

	regs_buff[0]  = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_CTRL_0_REG(priv->port)));
	regs_buff[1]  = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_CTRL_1_REG(priv->port)));
	regs_buff[2]  = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_CTRL_2_REG(priv->port)));
	regs_buff[3]  = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_AN_CTRL_REG(priv->port)));
	regs_buff[4]  = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_STATUS_REG(priv->port)));
	regs_buff[6]  = MV_32BIT_BE(MV_REG_READ(GMAC_PORT_FIFO_CFG_0_REG(priv->port)));
	regs_buff[7]  = MV_32BIT_BE(MV_REG_READ(GMAC_PORT_FIFO_CFG_1_REG(priv->port)));
	regs_buff[8]  = MV_32BIT_BE(MV_REG_READ(ETH_PORT_ISR_CAUSE_REG(priv->port)));
	regs_buff[9]  = MV_32BIT_BE(MV_REG_READ(ETH_PORT_ISR_MASK_REG(priv->port)));
	regs_buff[17] = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_MIB_CTRL_REG(priv->port)));
	regs_buff[18] = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_CTRL_3_REG(priv->port)));
	regs_buff[22] = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_SPEED_TIMER_REG(priv->port)));
	regs_buff[36] = MV_32BIT_BE(MV_REG_READ(ETH_GMAC_CTRL_4_REG(priv->port)));
	regs_buff[40] = MV_32BIT_BE(MV_REG_READ(ETH_PORT_ISR_SUM_CAUSE_REG(priv->port)));
	regs_buff[41] = MV_32BIT_BE(MV_REG_READ(ETH_PORT_ISR_SUM_MASK_REG(priv->port)));
}

int mv_pp2_eth_tool_nway_reset(struct net_device *netdev)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);
	MV_U32	        phy_addr;

	if ((priv == NULL) || (MV_PP2_IS_PON_PORT(priv->port))) {
		printk(KERN_ERR "interface %s is not supported\n", netdev->name);
		return -EOPNOTSUPP;
	}

	phy_addr = priv->plat_data->phy_addr;
	if (mvEthPhyRestartAN(phy_addr, MV_ETH_TOOL_AN_TIMEOUT) != MV_OK)
		return -EINVAL;

	return 0;
}

u32 mv_pp2_eth_tool_get_link(struct net_device *netdev)
{
	struct eth_port     *pp = MV_ETH_PRIV(netdev);

	if (pp == NULL) {
		printk(KERN_ERR "interface %s is not supported\n", netdev->name);
		return -EOPNOTSUPP;
	}

#ifdef CONFIG_MV_INCLUDE_PON
	if (MV_PP2_IS_PON_PORT(pp->port))
		return mv_pon_link_status(NULL);
#endif  

	return mvGmacPortIsLinkUp(pp->port);
}

int mv_pp2_eth_tool_get_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd)
{
	struct eth_port *pp = MV_ETH_PRIV(netdev);
	 
	cmd->rx_coalesce_usecs = pp->rx_time_coal_cfg;
	cmd->rx_max_coalesced_frames = pp->rx_pkts_coal_cfg;
#ifdef CONFIG_MV_PP2_TXDONE_ISR
	cmd->tx_max_coalesced_frames = pp->tx_pkts_coal_cfg;
#endif

	cmd->rx_coalesce_usecs_low = pp->rx_time_low_coal_cfg;
	cmd->rx_coalesce_usecs_high = pp->rx_time_high_coal_cfg;
	cmd->pkt_rate_low = pp->pkt_rate_low_cfg;
	cmd->pkt_rate_high = pp->pkt_rate_high_cfg;
	cmd->rate_sample_interval = pp->rate_sample_cfg;
	cmd->use_adaptive_rx_coalesce = pp->rx_adaptive_coal_cfg;
	cmd->rx_max_coalesced_frames_low = pp->rx_pkts_low_coal_cfg;
	cmd->rx_max_coalesced_frames_high = pp->rx_pkts_high_coal_cfg;

	return 0;
}

int mv_pp2_eth_tool_set_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd)
{
	struct eth_port *pp = MV_ETH_PRIV(netdev);
	int rxq;

	if (!cmd->rx_coalesce_usecs && !cmd->rx_max_coalesced_frames)
		return -EPERM;
#ifdef CONFIG_MV_PP2_TXDONE_ISR
	if (!cmd->tx_max_coalesced_frames)
		return -EPERM;
#endif

	if (!cmd->use_adaptive_rx_coalesce)
		for (rxq = 0; rxq < CONFIG_MV_PP2_RXQ; rxq++) {
			mv_pp2_rx_ptks_coal_set(pp->port, rxq, cmd->rx_max_coalesced_frames);
			mv_pp2_rx_time_coal_set(pp->port, rxq, cmd->rx_coalesce_usecs);
		}

	pp->rx_time_coal_cfg = cmd->rx_coalesce_usecs;
	pp->rx_pkts_coal_cfg = cmd->rx_max_coalesced_frames;
#ifdef CONFIG_MV_PP2_TXDONE_ISR
	{
		int txp, txq;

		for (txp = 0; txp < pp->txp_num; txp++)
			for (txq = 0; txq < CONFIG_MV_PP2_TXQ; txq++)
				mv_pp2_tx_done_ptks_coal_set(pp->port, txp, txq, cmd->tx_max_coalesced_frames);
	}
#endif
	pp->tx_pkts_coal_cfg = cmd->tx_max_coalesced_frames;

	pp->rx_time_low_coal_cfg = cmd->rx_coalesce_usecs_low;
	pp->rx_time_high_coal_cfg = cmd->rx_coalesce_usecs_high;
	pp->rx_pkts_low_coal_cfg = cmd->rx_max_coalesced_frames_low;
	pp->rx_pkts_high_coal_cfg = cmd->rx_max_coalesced_frames_high;
	pp->pkt_rate_low_cfg = cmd->pkt_rate_low;
	pp->pkt_rate_high_cfg = cmd->pkt_rate_high;

	if (cmd->rate_sample_interval > 0)
		pp->rate_sample_cfg = cmd->rate_sample_interval;

	if (!pp->rx_adaptive_coal_cfg && cmd->use_adaptive_rx_coalesce) {
		pp->rx_timestamp = jiffies;
		pp->rx_rate_pkts = 0;
	}
	pp->rx_adaptive_coal_cfg = cmd->use_adaptive_rx_coalesce;

	return 0;
}

void mv_pp2_eth_tool_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);

	memset(ring, 0, sizeof(struct ethtool_ringparam));
	ring->rx_pending = priv->rxq_ctrl[0].rxq_size;
	ring->tx_pending = priv->txq_ctrl[0].txq_size;
}

int mv_pp2_eth_tool_set_ringparam(struct net_device *netdev,
				 struct ethtool_ringparam *ring)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);
	int rxq, txp, txq, rxq_size, txq_size, swf_size, hwf_size, netdev_running = 0;

	if (ring->rx_jumbo_pending || ring->rx_mini_pending)
		return -EINVAL;

	rxq_size = MV_ALIGN_UP(ring->rx_pending, 16);

	txq_size = MV_ALIGN_UP(ring->tx_pending, 32);
	 
	if (netif_running(netdev))
		netdev_running = 1;

	if (netdev_running)
		mv_pp2_eth_stop(netdev);

	if (rxq_size != priv->rxq_ctrl[0].rxq_size)
		for (rxq = 0; rxq < priv->rxq_num; rxq++)
			mv_pp2_ctrl_rxq_size_set(priv->port, rxq, rxq_size);

#ifdef CONFIG_MV_ETH_PP2_1
	hwf_size = txq_size - (nr_cpu_ids * priv->txq_ctrl[0].rsvd_chunk);
#else
	hwf_size = txq_size/2;
#endif
	 
	swf_size = hwf_size - (nr_cpu_ids * priv->txq_ctrl[0].rsvd_chunk);

	if (txq_size != priv->txq_ctrl[0].txq_size)
		for (txp = 0; txp < priv->txp_num; txp++)
			for (txq = 0; txq < CONFIG_MV_PP2_TXQ; txq++) {
				mv_pp2_ctrl_txq_size_set(priv->port, txp, txq, txq_size);
				 
				mv_pp2_ctrl_txq_limits_set(priv->port, txp, txq, hwf_size, swf_size);
			}

	if (netdev_running)
		mv_pp2_eth_open(netdev);

	return 0;
}

void mv_pp2_eth_tool_get_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct eth_port      *priv = MV_ETH_PRIV(netdev);
	int                  port = priv->port;
	MV_ETH_PORT_STATUS   portStatus;
	MV_ETH_PORT_FC       flowCtrl;

	if ((priv == NULL) || (MV_PP2_IS_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return;
	}

	mvGmacFlowCtrlGet(port, &flowCtrl);
	if ((flowCtrl == MV_ETH_FC_AN_NO) || (flowCtrl == MV_ETH_FC_AN_SYM) || (flowCtrl == MV_ETH_FC_AN_ASYM))
		pause->autoneg = AUTONEG_ENABLE;
	else
		pause->autoneg = AUTONEG_DISABLE;

	mvGmacLinkStatus(port, &portStatus);
	if (portStatus.rxFc == MV_ETH_FC_DISABLE)
		pause->rx_pause = 0;
	else
		pause->rx_pause = 1;

	if (portStatus.txFc == MV_ETH_FC_DISABLE)
		pause->tx_pause = 0;
	else
		pause->tx_pause = 1;
}

int mv_pp2_eth_tool_set_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);
	int				port = priv->port;
	MV_U32			phy_addr;
	MV_STATUS		status = MV_FAIL;

	if ((priv == NULL) || (MV_PP2_IS_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return -EOPNOTSUPP;
	}

	if (pause->rx_pause && pause->tx_pause) {  
		if (pause->autoneg) {  
			status = mvGmacFlowCtrlSet(port, MV_ETH_FC_AN_SYM);
		} else {  
			status = mvGmacFlowCtrlSet(port, MV_ETH_FC_ENABLE);
		}
	} else if (!pause->rx_pause && !pause->tx_pause) {  
		if (pause->autoneg) {  
			status = mvGmacFlowCtrlSet(port, MV_ETH_FC_AN_NO);
		} else {  
			status = mvGmacFlowCtrlSet(port, MV_ETH_FC_DISABLE);
		}
	}
	 
	if (status == MV_OK) {
		phy_addr = priv->plat_data->phy_addr;
		status = mvEthPhyRestartAN(phy_addr, MV_ETH_TOOL_AN_TIMEOUT);
	}
	if (status != MV_OK)
		return -EINVAL;

	return 0;
}

void mv_pp2_eth_tool_get_strings(struct net_device *netdev,
			     uint32_t stringset, uint8_t *data)
{
	uint8_t *p = data;
	int i, q;
	char qnum[8][4] = {" Q0", " Q1", " Q2", " Q3", " Q4", " Q5", " Q6", " Q7"};

	switch (stringset) {
	case ETH_SS_TEST:
		 
		break;
	case ETH_SS_STATS:
		for (i = 0; i < MV_ETH_TOOL_GLOBAL_STATS_LEN; i++) {
			memcpy(p, mv_pp2_tool_global_strings_stats[i].stat_string,
			       ETH_GSTRING_LEN);
			p += ETH_GSTRING_LEN;
		}
		for (q = 0; q < CONFIG_MV_PP2_RXQ; q++) {
			for (i = 0; i < MV_ETH_TOOL_RX_QUEUE_STATS_LEN; i++) {
				const char *str = mv_pp2_tool_rx_queue_strings_stats[i].stat_string;
				memcpy(p, str, ETH_GSTRING_LEN);
				strcat(p, qnum[q]);
				p += ETH_GSTRING_LEN;
			}
		}
		for (q = 0; q < CONFIG_MV_PP2_TXQ; q++) {
			for (i = 0; i < MV_ETH_TOOL_TX_QUEUE_STATS_LEN; i++) {
				const char *str = mv_pp2_tool_tx_queue_strings_stats[i].stat_string;
				memcpy(p, str, ETH_GSTRING_LEN);
				strcat(p, qnum[q]);
				p += ETH_GSTRING_LEN;
			}
		}
		break;
	}
}

int mv_pp2_eth_tool_get_stats_count(struct net_device *netdev)
{
	return 0;
}

static int mv_pp2_eth_tool_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *info,
									 u32 *rules)
{
	if (info->cmd == ETHTOOL_GRXRINGS) {
		struct eth_port *pp = MV_ETH_PRIV(dev);
		if (pp)
			info->data = ARRAY_SIZE(pp->rx_indir_table);
	}
	return 0;
}

void mv_pp2_eth_tool_get_ethtool_stats(struct net_device *netdev,
				   struct ethtool_stats *stats, uint64_t *data)
{
	struct eth_port	*priv = MV_ETH_PRIV(netdev);
	uint64_t	*pdest = data;
	int		i, q;
	int		cpu = smp_processor_id();

	for (i = 0; i < MV_ETH_TOOL_GLOBAL_STATS_LEN; i++) {
		char *p = (char *)priv +
			mv_pp2_tool_global_strings_stats[i].stat_offset;
		pdest[i] =  *(uint32_t *)p;
	}
	pdest += MV_ETH_TOOL_GLOBAL_STATS_LEN;

	for (i = 0; i < MV_ETH_TOOL_CPU_STATS_LEN; i++) {
		char *p = (char *)priv +
			mv_pp2_tool_cpu_strings_stats[i].stat_offset;
		pdest[i] =  *((uint32_t *)p + cpu);
	}
	pdest += MV_ETH_TOOL_CPU_STATS_LEN;

	for (q = 0; q < CONFIG_MV_PP2_RXQ; q++) {
		for (i = 0; i < MV_ETH_TOOL_RX_QUEUE_STATS_LEN; i++) {
			char *p = (char *)priv +
				mv_pp2_tool_rx_queue_strings_stats[i].stat_offset;
			pdest[i] =  *((uint32_t *)p + q);
		}
		pdest += MV_ETH_TOOL_RX_QUEUE_STATS_LEN;
	}

	for (q = 0; q < CONFIG_MV_PP2_TXQ; q++) {
		for (i = 0; i < MV_ETH_TOOL_TX_QUEUE_STATS_LEN; i++) {
			char *p = (char *)priv +
				mv_pp2_tool_tx_queue_strings_stats[i].stat_offset;
			pdest[i] =  *((uint32_t *)p + q);
		}
		pdest += MV_ETH_TOOL_TX_QUEUE_STATS_LEN;
	}
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
 
static int mv_pp2_eth_tool_set_phys_id(struct net_device *netdev,
			     enum ethtool_phys_id_state state)
{
	 
	switch (state) {
	case ETHTOOL_ID_ACTIVE:
		return 2;

	case ETHTOOL_ID_ON:
		break;

	case ETHTOOL_ID_OFF:
		return -EOPNOTSUPP;

	case ETHTOOL_ID_INACTIVE:
		return -EOPNOTSUPP;
	}

	return 0;
}
#else
 
static int mv_pp2_eth_tool_phys_id(struct net_device *netdev,
			     uint32_t data)
{
	 
	return -EOPNOTSUPP;
}

#endif

static int mv_pp2_eth_tool_get_sset_count(struct net_device *netdev, int sset)
{
	switch (sset) {
	case ETH_SS_STATS:
		return MV_ETH_TOOL_STATS_LEN;
	default:
		return -EOPNOTSUPP;
	}
}

const struct ethtool_ops mv_pp2_eth_tool_ops = {
	.get_settings				= mv_pp2_eth_tool_get_settings,
	.set_settings				= mv_pp2_eth_tool_set_settings,
	.get_drvinfo				= mv_pp2_eth_tool_get_drvinfo,
	.get_regs_len				= mv_pp2_eth_tool_get_regs_len,
	.get_regs				= mv_pp2_eth_tool_get_regs,
	.get_wol				= mv_pp2_eth_tool_get_wol,
	.set_wol				= mv_pp2_eth_tool_set_wol,
	.nway_reset				= mv_pp2_eth_tool_nway_reset,
	.get_link				= mv_pp2_eth_tool_get_link,
	.get_coalesce				= mv_pp2_eth_tool_get_coalesce,
	.set_coalesce				= mv_pp2_eth_tool_set_coalesce,
	.get_ringparam				= mv_pp2_eth_tool_get_ringparam,
	.set_ringparam				= mv_pp2_eth_tool_set_ringparam,
	.get_pauseparam				= mv_pp2_eth_tool_get_pauseparam,
	.set_pauseparam				= mv_pp2_eth_tool_set_pauseparam,
	.get_strings				= mv_pp2_eth_tool_get_strings,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 32)
	.get_stats_count			= mv_pp2_eth_tool_get_stats_count, 
#endif
	.get_ethtool_stats			= mv_pp2_eth_tool_get_ethtool_stats, 
	 
	.get_rxnfc				= mv_pp2_eth_tool_get_rxnfc, 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0)
	.set_phys_id				= mv_pp2_eth_tool_set_phys_id,
#else
	.phys_id				= mv_pp2_eth_tool_phys_id,
#endif
	.get_sset_count				= mv_pp2_eth_tool_get_sset_count,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 5, 0)
	.get_ts_info				= ethtool_op_get_ts_info,
#endif
#ifdef MY_ABC_HERE
	.get_wol	= syno_get_wol,
	.set_wol	= syno_set_wol,
#endif
};

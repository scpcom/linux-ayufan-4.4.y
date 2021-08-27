#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <linux/mii.h>

#include "mvOs.h"
#include "mvDebug.h"
#include "mvEthPhy.h"

#include "gbe/mvNeta.h"
#include "bm/mvBm.h"

#include "mv_switch.h"
#include "mv_netdev.h"

#include "mvOs.h"

#ifdef CONFIG_MV_ETH_PNC
#include "pnc/mvPnc.h"
#endif  

#ifdef MY_ABC_HERE
extern spinlock_t          mii_lock;
#endif

#define MV_ETH_TOOL_AN_TIMEOUT	5000

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

static int isSwitch(struct eth_port *priv)
{
	return priv->tagged;
}

int mv_eth_tool_restore_settings(struct net_device *netdev)
{
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	int			phy_speed, phy_duplex;
	MV_U32			phy_addr = priv->plat_data->phy_addr;
	MV_ETH_PORT_SPEED	mac_speed;
	MV_ETH_PORT_DUPLEX	mac_duplex;
	int			err = -EINVAL;

	 if ((priv == NULL) || (isSwitch(priv)))
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
		err = mvNetaSpeedDuplexSet(priv->port, MV_ETH_SPEED_AN, MV_ETH_DUPLEX_AN);
		if (!err)
			err = mvEthPhyAdvertiseSet(phy_addr, priv->advertise_cfg);
		 
		if (!err) {
			err = mvNetaFlowCtrlSet(priv->port, MV_ETH_FC_AN_SYM);
			if (!err) {
				err = mvEthPhyRestartAN(phy_addr, MV_ETH_TOOL_AN_TIMEOUT);
				if (err == MV_TIMEOUT) {
					MV_ETH_PORT_STATUS ps;

					mvNetaLinkStatus(priv->port, &ps);

					if (!ps.linkup)
						err = 0;
				}
			}
		}
	} else if (priv->autoneg_cfg == AUTONEG_DISABLE) {
		err = mvEthPhyDisableAN(phy_addr, phy_speed, phy_duplex);
		if (!err)
			err = mvNetaFlowCtrlSet(priv->port, MV_ETH_FC_ENABLE);
		if (!err)
			err = mvNetaSpeedDuplexSet(priv->port, mac_speed, mac_duplex);
	} else
		err = -EINVAL;

	return err;
}

int mv_eth_tool_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	u16			lp_ad, stat1000;
	MV_U32			phy_addr;
	MV_ETH_PORT_SPEED 	speed;
	MV_ETH_PORT_DUPLEX 	duplex;
	MV_ETH_PORT_STATUS      status;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return -EOPNOTSUPP;
	}

	cmd->supported = (SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full | SUPPORTED_100baseT_Half
			| SUPPORTED_100baseT_Full | SUPPORTED_Autoneg | SUPPORTED_TP | SUPPORTED_MII
			| SUPPORTED_1000baseT_Full);

	phy_addr = priv->plat_data->phy_addr;

	mvNetaLinkStatus(priv->port, &status);

	if (status.linkup != MV_TRUE) {
		 
		cmd->speed  = -1;
		cmd->duplex = -1;
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
	 
	mvNetaSpeedDuplexGet(priv->port, &speed, &duplex);
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

int mv_eth_tool_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct eth_port *priv = MV_ETH_PRIV(dev);
	int _speed, _duplex, _autoneg, _advertise, err;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
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
	err = mv_eth_tool_restore_settings(dev);

	if (err) {
		priv->duplex_cfg = _duplex;
		priv->speed_cfg = _speed;
		priv->autoneg_cfg = _autoneg;
		priv->advertise_cfg = _advertise;
	}
	return err;
}

int mv_eth_tool_get_regs_len(struct net_device *netdev)
{
#define MV_ETH_TOOL_REGS_LEN 32

	return (MV_ETH_TOOL_REGS_LEN * sizeof(uint32_t));
}

void mv_eth_tool_get_drvinfo(struct net_device *netdev,
			     struct ethtool_drvinfo *info)
{
	strcpy(info->driver, "mv_eth");
	 
	strcpy(info->fw_version, "N/A");
	strcpy(info->bus_info, "Mbus");
 
	info->testinfo_len = 0;
	info->regdump_len = mv_eth_tool_get_regs_len(netdev);
	info->eedump_len = 0;
}

void mv_eth_tool_get_regs(struct net_device *netdev,
			  struct ethtool_regs *regs, void *p)
{
	struct eth_port	*priv = MV_ETH_PRIV(netdev);
	uint32_t 	*regs_buff = p;

	if ((priv == NULL) || MV_PON_PORT(priv->port)) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return;
	}

	memset(p, 0, MV_ETH_TOOL_REGS_LEN * sizeof(uint32_t));

	regs->version = priv->plat_data->ctrl_rev;

	regs_buff[0]  = MV_REG_READ(ETH_PORT_STATUS_REG(priv->port));
	regs_buff[1]  = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(priv->port));
	regs_buff[2]  = MV_REG_READ(ETH_PORT_CONFIG_REG(priv->port));
	regs_buff[3]  = MV_REG_READ(ETH_PORT_CONFIG_EXTEND_REG(priv->port));
	regs_buff[4]  = MV_REG_READ(ETH_SDMA_CONFIG_REG(priv->port));
 
	regs_buff[6]  = MV_REG_READ(ETH_RX_QUEUE_COMMAND_REG(priv->port));
	 
	regs_buff[8]  = MV_REG_READ(ETH_INTR_CAUSE_REG(priv->port));
	regs_buff[9]  = MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(priv->port));
	regs_buff[10] = MV_REG_READ(ETH_INTR_MASK_REG(priv->port));
	regs_buff[11] = MV_REG_READ(ETH_INTR_MASK_EXT_REG(priv->port));
	 
	regs_buff[16] = MV_REG_READ(ETH_PHY_ADDR_REG(priv->port));
	regs_buff[17] = MV_REG_READ(ETH_UNIT_INTR_CAUSE_REG(priv->port));
	regs_buff[18] = MV_REG_READ(ETH_UNIT_INTR_MASK_REG(priv->port));
	regs_buff[19] = MV_REG_READ(ETH_UNIT_ERROR_ADDR_REG(priv->port));
	regs_buff[20] = MV_REG_READ(ETH_UNIT_INT_ADDR_ERROR_REG(priv->port));

}

int mv_eth_tool_nway_reset(struct net_device *netdev)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);
	MV_U32	        phy_addr;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "interface %s is not supported\n", netdev->name);
		return -EOPNOTSUPP;
	}

	phy_addr = priv->plat_data->phy_addr;
	if (mvEthPhyRestartAN(phy_addr, MV_ETH_TOOL_AN_TIMEOUT) != MV_OK)
		return -EINVAL;

	return 0;
}

u32 mv_eth_tool_get_link(struct net_device *netdev)
{
	struct eth_port     *pp = MV_ETH_PRIV(netdev);

	if (pp == NULL) {
		printk(KERN_ERR "interface %s is not supported\n", netdev->name);
		return -EOPNOTSUPP;
	}

#ifdef CONFIG_MV_PON
	if (MV_PON_PORT(pp->port))
		return mv_pon_link_status();
#endif  

	return mvNetaLinkIsUp(pp->port);
}
 
int mv_eth_tool_get_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd)
{
	struct eth_port *pp = MV_ETH_PRIV(netdev);
	 
	cmd->rx_coalesce_usecs = pp->rx_time_coal_cfg;
	cmd->rx_max_coalesced_frames = pp->rx_pkts_coal_cfg;
	cmd->tx_max_coalesced_frames = pp->tx_pkts_coal_cfg;

	cmd->rx_coalesce_usecs_low = pp->rx_time_low_coal_cfg;
	cmd->rx_coalesce_usecs_high = pp->rx_time_high_coal_cfg;
	cmd->rx_max_coalesced_frames_low = pp->rx_pkts_low_coal_cfg;
	cmd->rx_max_coalesced_frames_high = pp->rx_pkts_high_coal_cfg;
	cmd->pkt_rate_low = pp->pkt_rate_low_cfg;
	cmd->pkt_rate_high = pp->pkt_rate_high_cfg;
	cmd->rate_sample_interval = pp->rate_sample_cfg;
	cmd->use_adaptive_rx_coalesce = pp->rx_adaptive_coal_cfg;

	return 0;
}

int mv_eth_tool_set_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd)
{
	struct eth_port *pp = MV_ETH_PRIV(netdev);
	int rxq, txp, txq;

	if ((!cmd->rx_coalesce_usecs && !cmd->rx_max_coalesced_frames) || (!cmd->tx_max_coalesced_frames))
		return -EPERM;

	if (!cmd->use_adaptive_rx_coalesce) {
		for (rxq = 0; rxq < CONFIG_MV_ETH_RXQ; rxq++) {
			mv_eth_rx_pkts_coal_set(pp->port, rxq, cmd->rx_max_coalesced_frames);
			mv_eth_rx_time_coal_set(pp->port, rxq, cmd->rx_coalesce_usecs);
		}
	}

	pp->rx_time_coal_cfg = cmd->rx_coalesce_usecs;
	pp->rx_pkts_coal_cfg = cmd->rx_max_coalesced_frames;
	for (txp = 0; txp < pp->txp_num; txp++)
		for (txq = 0; txq < CONFIG_MV_ETH_TXQ; txq++)
			mv_eth_tx_done_pkts_coal_set(pp->port, txp, txq, cmd->tx_max_coalesced_frames);
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
	pp->rate_current = 0;  

	return 0;
}

void mv_eth_tool_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
{
 
}

void mv_eth_tool_get_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct eth_port      *priv = MV_ETH_PRIV(netdev);
	int                  port = priv->port;
	MV_ETH_PORT_STATUS   portStatus;
	MV_ETH_PORT_FC       flowCtrl;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return;
	}

	mvNetaFlowCtrlGet(port, &flowCtrl);
	if ((flowCtrl == MV_ETH_FC_AN_NO) || (flowCtrl == MV_ETH_FC_AN_SYM) || (flowCtrl == MV_ETH_FC_AN_ASYM))
		pause->autoneg = AUTONEG_ENABLE;
	else
		pause->autoneg = AUTONEG_DISABLE;

	mvNetaLinkStatus(port, &portStatus);
	if (portStatus.rxFc == MV_ETH_FC_DISABLE)
		pause->rx_pause = 0;
	else
		pause->rx_pause = 1;

	if (portStatus.txFc == MV_ETH_FC_DISABLE)
		pause->tx_pause = 0;
	else
		pause->tx_pause = 1;
}

int mv_eth_tool_set_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct eth_port *priv = MV_ETH_PRIV(netdev);
	int				port = priv->port;
	MV_U32			phy_addr;
	MV_STATUS		status = MV_FAIL;

	if ((priv == NULL) || (isSwitch(priv)) || (MV_PON_PORT(priv->port))) {
		printk(KERN_ERR "%s is not supported on %s\n", __func__, netdev->name);
		return -EOPNOTSUPP;
	}

	if (pause->rx_pause && pause->tx_pause) {  
		if (pause->autoneg) {  
			status = mvNetaFlowCtrlSet(port, MV_ETH_FC_AN_SYM);
		} else {  
			status = mvNetaFlowCtrlSet(port, MV_ETH_FC_ENABLE);
		}
	} else if (!pause->rx_pause && !pause->tx_pause) {  
		if (pause->autoneg) {  
			status = mvNetaFlowCtrlSet(port, MV_ETH_FC_AN_NO);
		} else {  
			status = mvNetaFlowCtrlSet(port, MV_ETH_FC_DISABLE);
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

void mv_eth_tool_get_strings(struct net_device *netdev,
			     uint32_t stringset, uint8_t *data)
{
}

int mv_eth_tool_get_stats_count(struct net_device *netdev)
{
	return 0;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
static int mv_eth_tool_get_rxfh_indir(struct net_device *netdev,
					struct ethtool_rxfh_indir *indir)
{
#if defined(MV_ETH_PNC_LB) && defined(CONFIG_MV_ETH_PNC)
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	size_t copy_size =
		min_t(size_t, indir->size, ARRAY_SIZE(priv->rx_indir_table));

	indir->size = ARRAY_SIZE(priv->rx_indir_table);

	memcpy(indir->ring_index, priv->rx_indir_table,
	       copy_size * sizeof(indir->ring_index[0]));
	return 0;
#else
	return -EOPNOTSUPP;
#endif
}

static int mv_eth_tool_set_rxfh_indir(struct net_device *netdev,
				   const struct ethtool_rxfh_indir *indir)
{
#if defined(MV_ETH_PNC_LB) && defined(CONFIG_MV_ETH_PNC)
	int i;
	struct eth_port 	*priv = MV_ETH_PRIV(netdev);
	for (i = 0; i < indir->size; i++) {
		priv->rx_indir_table[i] = indir->ring_index[i];
		mvPncLbRxqSet(i, priv->rx_indir_table[i]);
	}
	return 0;
#else
	return -EOPNOTSUPP;
#endif
}
#endif

static int mv_eth_tool_get_rxnfc(struct net_device *dev, struct ethtool_rxnfc *info,
									u32 *rules)
{
	if (info->cmd == ETHTOOL_GRXRINGS) {
		struct eth_port *pp = MV_ETH_PRIV(dev);
		if (pp)
			info->data = ARRAY_SIZE(pp->rx_indir_table);
	}
	return 0;
}

#if ((LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 33)))
 
static int mv_eth_tool_set_rx_ntuple(struct net_device *dev, struct ethtool_rx_ntuple *ntuple)
{
#ifdef CONFIG_MV_ETH_PNC_L3_FLOW
	unsigned int sip, dip, ports, sport, dport, proto;
	struct eth_port *pp;

	if ((ntuple->fs.flow_type != TCP_V4_FLOW) && (ntuple->fs.flow_type != UDP_V4_FLOW))
		return -EOPNOTSUPP;

	if ((ntuple->fs.action >= CONFIG_MV_ETH_RXQ) || (ntuple->fs.action < ETHTOOL_RXNTUPLE_ACTION_CLEAR))
		return -EINVAL;

	if (ntuple->fs.flow_type == TCP_V4_FLOW)
		proto = 6;  
	else
		proto = 17;  

	sip = ntuple->fs.h_u.tcp_ip4_spec.ip4src;
	dip = ntuple->fs.h_u.tcp_ip4_spec.ip4dst;
	sport = ntuple->fs.h_u.tcp_ip4_spec.psrc;
	dport = ntuple->fs.h_u.tcp_ip4_spec.pdst;
	if (!sip || !dip)
		return -EINVAL;

	pp = MV_ETH_PRIV(dev);
	if (!sport || !dport) {  
		pnc_ip4_2tuple_rxq(pp->port, sip, dip, ntuple->fs.action);
	} else {
		ports = (dport << 16) | ((sport << 16) >> 16);
		pnc_ip4_5tuple_rxq(pp->port, sip, dip, ports, proto, ntuple->fs.action);
	}

	return 0;
#else
	return 1;
#endif  
}
#endif

void mv_eth_tool_get_ethtool_stats(struct net_device *netdev,
				   struct ethtool_stats *stats, uint64_t *data)
{

}

const struct ethtool_ops mv_eth_tool_ops = {
	.get_settings				= mv_eth_tool_get_settings,
	.set_settings				= mv_eth_tool_set_settings,
	.get_drvinfo				= mv_eth_tool_get_drvinfo,
	.get_regs_len				= mv_eth_tool_get_regs_len,
	.get_regs				= mv_eth_tool_get_regs,
	.nway_reset				= mv_eth_tool_nway_reset,
	.get_link				= mv_eth_tool_get_link,
	.get_coalesce				= mv_eth_tool_get_coalesce,
	.set_coalesce				= mv_eth_tool_set_coalesce,
	.get_ringparam  			= mv_eth_tool_get_ringparam,
	.get_pauseparam				= mv_eth_tool_get_pauseparam,
	.set_pauseparam				= mv_eth_tool_set_pauseparam,
	.get_strings				= mv_eth_tool_get_strings, 
	.get_ethtool_stats			= mv_eth_tool_get_ethtool_stats,

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 32)
	.get_stats_count			= mv_eth_tool_get_stats_count,
#endif

#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 35)
	.get_rxfh_indir				= mv_eth_tool_get_rxfh_indir,
	.set_rxfh_indir				= mv_eth_tool_set_rxfh_indir,
	.get_rxnfc                  = mv_eth_tool_get_rxnfc,
	.set_rx_ntuple				= mv_eth_tool_set_rx_ntuple,
#endif
	.get_rxnfc				= mv_eth_tool_get_rxnfc,
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(3, 3, 0)) && (LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 33)))
	.set_rx_ntuple				= mv_eth_tool_set_rx_ntuple,
#endif

#ifdef MY_ABC_HERE
	.get_wol	= syno_get_wol,
	.set_wol	= syno_set_wol,
#endif
};

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"   
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/pci.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/tcp.h>
#include <linux/string.h>
#include <net/ip.h>
#include <net/xfrm.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
#include <asm/arch/system.h>
#else
#include <mach/system.h>
#endif

#include "mvOs.h"
#include "dbg-trace.h"
#include "mvSysHwConfig.h"
#include "eth/mvEth.h"
#include "eth-phy/mvEthPhy.h"
#include "ctrlEnv/sys/mvSysGbe.h"
#include "ctrlEnv/mvCtrlEnvLib.h"

#include "mv_netdev.h"
#include "mv_eth_tool.h"

extern spinlock_t          mii_lock;

#ifdef MY_ABC_HERE
static void syno_get_wol(struct net_device *dev, struct ethtool_wolinfo *wol);
static int syno_set_wol(struct net_device *dev, struct ethtool_wolinfo *wol);
#endif

const struct ethtool_ops mv_eth_tool_ops = {
	.get_settings		= mv_eth_tool_get_settings,
	.set_settings		= mv_eth_tool_set_settings,
	.get_drvinfo		= mv_eth_tool_get_drvinfo,
	.get_regs_len		= mv_eth_tool_get_regs_len,
	.get_regs		= mv_eth_tool_get_regs,
	.nway_reset		= mv_eth_tool_nway_reset,
	.get_link		= mv_eth_tool_get_link,
	.get_coalesce		= mv_eth_tool_get_coalesce,
	.set_coalesce		= mv_eth_tool_set_coalesce,
	.get_ringparam          = mv_eth_tool_get_ringparam,
	.get_pauseparam		= mv_eth_tool_get_pauseparam,
	.set_pauseparam		= mv_eth_tool_set_pauseparam,
	.get_rx_csum		= mv_eth_tool_get_rx_csum,
	.set_rx_csum		= mv_eth_tool_set_rx_csum,
	.get_tx_csum		= ethtool_op_get_tx_csum,
	.set_tx_csum		= mv_eth_tool_set_tx_csum,
	.get_sg			= ethtool_op_get_sg,
	.set_sg			= ethtool_op_set_sg,
	.get_tso		= ethtool_op_get_tso,
	.set_tso		= mv_eth_tool_set_tso,
	.get_ufo		= ethtool_op_get_ufo,
	.set_ufo		= mv_eth_tool_set_ufo,
	.get_strings		= mv_eth_tool_get_strings,
	.phys_id		= mv_eth_tool_phys_id,
	.get_stats_count	= mv_eth_tool_get_stats_count,
	.get_ethtool_stats	= mv_eth_tool_get_ethtool_stats,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
	.get_perm_addr		= ethtool_op_get_perm_addr,
#endif
#ifdef MY_ABC_HERE
	.get_wol	= syno_get_wol,
	.set_wol	= syno_set_wol,
#endif
};

struct mv_eth_tool_stats {
	char stat_string[ETH_GSTRING_LEN];
	int stat_offset;
};

#define MV_ETH_TOOL_STAT(m)	offsetof(mv_eth_priv, m)

static const struct mv_eth_tool_stats mv_eth_tool_global_strings_stats[] = {
	{"skb_alloc_fail", MV_ETH_TOOL_STAT(eth_stat.skb_alloc_fail)},
	{"tx_timeout", MV_ETH_TOOL_STAT(eth_stat.tx_timeout)},
	{"tx_netif_stop", MV_ETH_TOOL_STAT(eth_stat.tx_netif_stop)},
	{"tx_done_netif_wake", MV_ETH_TOOL_STAT(eth_stat.tx_done_netif_wake)},
	{"tx_skb_no_headroom", MV_ETH_TOOL_STAT(eth_stat.tx_skb_no_headroom)},
#ifdef CONFIG_MV_ETH_STATS_INFO
	{"irq_total", MV_ETH_TOOL_STAT(eth_stat.irq_total)},
	{"irq_none_events", MV_ETH_TOOL_STAT(eth_stat.irq_none)},
	{"irq_while_polling", MV_ETH_TOOL_STAT(eth_stat.irq_while_polling)},
	{"picr is", MV_ETH_TOOL_STAT(picr)},
	{"picer is", MV_ETH_TOOL_STAT(picer)},
	{"poll_events", MV_ETH_TOOL_STAT(eth_stat.poll_events)},
	{"poll_complete", MV_ETH_TOOL_STAT(eth_stat.poll_complete)},
	{"tx_events", MV_ETH_TOOL_STAT(eth_stat.tx_events)},
	{"tx_done_events", MV_ETH_TOOL_STAT(eth_stat.tx_done_events)},
	{"timer_events", MV_ETH_TOOL_STAT(eth_stat.timer_events)},
#if defined(CONFIG_MV_ETH_NFP) || defined(CONFIG_MV_SKB_REUSE)
	{"rx_pool_empty", MV_ETH_TOOL_STAT(eth_stat.rx_pool_empty)},
#endif  
#endif  

#ifdef CONFIG_MV_ETH_STATS_DEBUG
	{"skb_alloc_ok", MV_ETH_TOOL_STAT(eth_stat.skb_alloc_ok)},
	{"skb_free_ok", MV_ETH_TOOL_STAT(eth_stat.skb_free_ok)},
#ifdef CONFIG_MV_SKB_REUSE
	{"skb_reuse_rx", MV_ETH_TOOL_STAT(eth_stat.skb_reuse_rx)},
	{"skb_reuse_tx", MV_ETH_TOOL_STAT(eth_stat.skb_reuse_tx)},
	{"skb_reuse_alloc", MV_ETH_TOOL_STAT(eth_stat.skb_reuse_alloc)},
#endif  
	{"tx_csum_hw", MV_ETH_TOOL_STAT(eth_stat.tx_csum_hw)},
	{"tx_csum_sw", MV_ETH_TOOL_STAT(eth_stat.tx_csum_sw)},
	{"rx_netif_drop", MV_ETH_TOOL_STAT(eth_stat.rx_netif_drop)},
	{"rx_csum_hw", MV_ETH_TOOL_STAT(eth_stat.rx_csum_hw)},
	{"rx_csum_hw_frags", MV_ETH_TOOL_STAT(eth_stat.rx_csum_hw_frags)},
	{"rx_csum_sw", MV_ETH_TOOL_STAT(eth_stat.rx_csum_sw)},
#ifdef ETH_INCLUDE_LRO
	{"rx_lro_aggregated", MV_ETH_TOOL_STAT(lro_mgr.stats.aggregated)},
	{"rx_lro_flushed", MV_ETH_TOOL_STAT(lro_mgr.stats.flushed)},
	{"rx_lro_defragmented", MV_ETH_TOOL_STAT(lro_mgr.stats.defragmented)},
	{"rx_lro_no_resources", MV_ETH_TOOL_STAT(lro_mgr.stats.no_desc)},
#endif  
#ifdef ETH_MV_TX_EN
	{"tx_en_done", MV_ETH_TOOL_STAT(eth_stat.tx_en_done)},
	{"tx_en_busy", MV_ETH_TOOL_STAT(eth_stat.tx_en_busy)},
	{"tx_en_wait", MV_ETH_TOOL_STAT(eth_stat.tx_en_wait)},
	{"tx_en_wait_count", MV_ETH_TOOL_STAT(eth_stat.tx_en_wait_count)},
#endif  
#endif  
};

static const struct mv_eth_tool_stats mv_eth_tool_rx_queue_strings_stats[] = {
#ifdef CONFIG_MV_ETH_STATS_DEBUG
	{"rx_hal_ok", MV_ETH_TOOL_STAT(eth_stat.rx_hal_ok)},
	{"rx_fill_ok", MV_ETH_TOOL_STAT(eth_stat.rx_fill_ok)},
#endif  
};

static const struct mv_eth_tool_stats mv_eth_tool_tx_queue_strings_stats[] = {
#ifdef CONFIG_MV_ETH_STATS_DEBUG
	{"tx_count", MV_ETH_TOOL_STAT(tx_count)},
	{"tx_hal_ok", MV_ETH_TOOL_STAT(eth_stat.tx_hal_ok)},
	{"tx_hal_no_resource", MV_ETH_TOOL_STAT(eth_stat.tx_hal_no_resource)},
	{"tx_done_hal_ok", MV_ETH_TOOL_STAT(eth_stat.tx_done_hal_ok)},
#endif  
};

#define MV_ETH_TOOL_RX_QUEUE_STATS_LEN  \
	sizeof(mv_eth_tool_rx_queue_strings_stats) / sizeof(struct mv_eth_tool_stats)
	
#define MV_ETH_TOOL_TX_QUEUE_STATS_LEN  \
	sizeof(mv_eth_tool_tx_queue_strings_stats) / sizeof(struct mv_eth_tool_stats)
	
#define MV_ETH_TOOL_QUEUE_STATS_LEN 	\
	((MV_ETH_TOOL_RX_QUEUE_STATS_LEN * MV_ETH_RX_Q_NUM) + \
	(MV_ETH_TOOL_TX_QUEUE_STATS_LEN * MV_ETH_TX_Q_NUM))

#define MV_ETH_TOOL_GLOBAL_STATS_LEN	\
	sizeof(mv_eth_tool_global_strings_stats) / sizeof(struct mv_eth_tool_stats)
	
#define MV_ETH_TOOL_STATS_LEN 		\
	(MV_ETH_TOOL_GLOBAL_STATS_LEN + MV_ETH_TOOL_QUEUE_STATS_LEN)

int mv_eth_tool_read_mdio(struct net_device *netdev, int addr, int reg)
{
	unsigned long 	flags;
	unsigned short 	value;
	MV_STATUS 	status;
	
	spin_lock_irqsave(&mii_lock, flags);
	status = mvEthPhyRegRead(addr, reg, &value);
	spin_unlock_irqrestore(&mii_lock, flags);

	if (status == MV_OK)
		return value;

	return -1;
}

void mv_eth_tool_write_mdio(struct net_device *netdev, int addr, int reg, int data)
{
	unsigned long   flags;
	unsigned short  tmp   = (unsigned short)data;

	spin_lock_irqsave(&mii_lock, flags);
	mvEthPhyRegWrite(addr, reg, tmp);
	spin_unlock_irqrestore(&mii_lock, flags);
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

int mv_eth_tool_get_settings(struct net_device *netdev, struct ethtool_cmd *cmd)
{
	mv_eth_priv 		*priv = MV_ETH_PRIV(netdev);
	struct mii_if_info 	mii;
	int			retval;
	MV_ETH_PORT_STATUS 	status;
	
#ifdef CONFIG_MII	
	mii.dev			= netdev;
	mii.phy_id_mask 	= 0x1F;
	mii.reg_num_mask 	= 0x1F;
	mii.mdio_read 		= mv_eth_tool_read_mdio;
	mii.mdio_write 		= mv_eth_tool_write_mdio;
	mii.phy_id 		= priv->phy_id;
	mii.supports_gmii 	= 1;

	retval = mii_ethtool_gset(&mii, cmd);
	if (retval)
		return retval;
#endif

	mvEthStatusGet(priv->hal_priv, &status);
	
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

	cmd->port = PORT_MII;
	cmd->phy_address = priv->phy_id;

	return 0;
}

int mv_eth_tool_restore_settings(struct net_device *netdev)
{
	mv_eth_priv 		*priv = MV_ETH_PRIV(netdev);
	int 			mv_phy_speed, mv_phy_duplex;
	MV_U32			mv_phy_addr = priv->phy_id;
	MV_ETH_PORT_SPEED	mv_mac_speed;
	MV_ETH_PORT_DUPLEX	mv_mac_duplex;
	int			err = -EINVAL;

	switch (priv->speed_cfg) {
		case SPEED_10:
			mv_phy_speed  = 0;
			mv_mac_speed = MV_ETH_SPEED_10;
			break;
		case SPEED_100:
			mv_phy_speed  = 1;
			mv_mac_speed = MV_ETH_SPEED_100;
			break;
		case SPEED_1000:
			mv_phy_speed  = 2;
			mv_mac_speed = MV_ETH_SPEED_1000;
			break;
		default:
			return -EINVAL;
	}

	switch (priv->duplex_cfg) {
		case DUPLEX_HALF:
			mv_phy_duplex = 0;
			mv_mac_duplex = MV_ETH_DUPLEX_HALF;
			break;
		case DUPLEX_FULL:
			mv_phy_duplex = 1;
			mv_mac_duplex = MV_ETH_DUPLEX_FULL;
			break;
		default:
			return -EINVAL;
	}
	
	if (priv->autoneg_cfg == AUTONEG_ENABLE) {
		err = mvEthSpeedDuplexSet(priv->hal_priv,
					  MV_ETH_SPEED_AN, MV_ETH_DUPLEX_AN);

		if (!err) {

			err = mvEthPhyRestartAN(mv_phy_addr, MV_ETH_TOOL_AN_TIMEOUT);
			if (err == MV_TIMEOUT) {
				MV_ETH_PORT_STATUS ps;
				mvEthStatusGet(priv->hal_priv, &ps);
				if (!ps.isLinkUp)
					err = 0;
			}
		}
	} else if (priv->autoneg_cfg == AUTONEG_DISABLE) {
		err = mvEthPhyDisableAN(mv_phy_addr, mv_phy_speed, mv_phy_duplex);
		if (!err) {
			err = mvEthSpeedDuplexSet(priv->hal_priv,
					mv_mac_speed, mv_mac_duplex);
		}
	} else {
		err = -EINVAL;
	}

	return err;
}

int mv_eth_tool_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	mv_eth_priv *priv = MV_ETH_PRIV(dev);
	int _speed, _duplex, _autoneg, err;

#ifdef CONFIG_MV_GATEWAY
	if (priv->isGtw)
		return -EPERM;
#endif  

	_duplex = priv->duplex_cfg;
	_speed = priv->speed_cfg;
	_autoneg = priv->autoneg_cfg;

	priv->duplex_cfg = cmd->duplex;
	priv->speed_cfg = cmd->speed;
	priv->autoneg_cfg = cmd->autoneg;

	err = mv_eth_tool_restore_settings(dev);

	if (err) {
		priv->duplex_cfg = _duplex;
		priv->speed_cfg = _speed;
		priv->autoneg_cfg = _autoneg;
	}
	return err;
}

void mv_eth_tool_get_drvinfo(struct net_device *netdev,
			     struct ethtool_drvinfo *info)
{
	strcpy(info->driver, "mv_eth");
	strcpy(info->version, LSP_VERSION);
	strcpy(info->fw_version, "N/A");
	strcpy(info->bus_info, "Mbus");
	info->n_stats = MV_ETH_TOOL_STATS_LEN;
	info->testinfo_len = 0;
	info->regdump_len = mv_eth_tool_get_regs_len(netdev);
	info->eedump_len = 0;
}

int mv_eth_tool_get_regs_len(struct net_device *netdev)
{
#define MV_ETH_TOOL_REGS_LEN 32
	return MV_ETH_TOOL_REGS_LEN * sizeof(uint32_t);
}

void mv_eth_tool_get_regs(struct net_device *netdev,
			  struct ethtool_regs *regs, void *p)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(netdev);
	uint32_t 	*regs_buff = p;

	memset(p, 0, MV_ETH_TOOL_REGS_LEN * sizeof(uint32_t));

	regs->version = mvCtrlModelRevGet(); 

	regs_buff[0]  = MV_REG_READ(ETH_PORT_STATUS_REG(priv->port));
	regs_buff[1]  = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(priv->port));
	regs_buff[2]  = MV_REG_READ(ETH_PORT_CONFIG_REG(priv->port));
	regs_buff[3]  = MV_REG_READ(ETH_PORT_CONFIG_EXTEND_REG(priv->port));
	regs_buff[4]  = MV_REG_READ(ETH_SDMA_CONFIG_REG(priv->port));
	regs_buff[5]  = MV_REG_READ(ETH_TX_FIFO_URGENT_THRESH_REG(priv->port));
	regs_buff[6]  = MV_REG_READ(ETH_RX_QUEUE_COMMAND_REG(priv->port));
	regs_buff[7]  = MV_REG_READ(ETH_TX_QUEUE_COMMAND_REG(priv->port));
	regs_buff[8]  = MV_REG_READ(ETH_INTR_CAUSE_REG(priv->port));
	regs_buff[9]  = MV_REG_READ(ETH_INTR_CAUSE_EXT_REG(priv->port));
	regs_buff[10] = MV_REG_READ(ETH_INTR_MASK_REG(priv->port));
	regs_buff[11] = MV_REG_READ(ETH_INTR_MASK_EXT_REG(priv->port));
	regs_buff[12] = MV_REG_READ(ETH_RX_DESCR_STAT_CMD_REG(priv->port, 0));
	regs_buff[13] = MV_REG_READ(ETH_RX_BYTE_COUNT_REG(priv->port, 0));
	regs_buff[14] = MV_REG_READ(ETH_RX_BUF_PTR_REG(priv->port, 0));
	regs_buff[15] = MV_REG_READ(ETH_RX_CUR_DESC_PTR_REG(priv->port, 0));
	 
	regs_buff[16] = MV_REG_READ(ETH_PHY_ADDR_REG(priv->port));
	regs_buff[17] = MV_REG_READ(ETH_UNIT_INTR_CAUSE_REG(priv->port));
	regs_buff[18] = MV_REG_READ(ETH_UNIT_INTR_MASK_REG(priv->port));
	regs_buff[19] = MV_REG_READ(ETH_UNIT_ERROR_ADDR_REG(priv->port));
	regs_buff[20] = MV_REG_READ(ETH_UNIT_INT_ADDR_ERROR_REG(priv->port));
	
}

int mv_eth_tool_nway_reset(struct net_device *netdev)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(netdev);
	MV_U32		mv_phy_addr = (MV_U32)(priv->phy_id);

	if (mvEthPhyRestartAN(mv_phy_addr, MV_ETH_TOOL_AN_TIMEOUT) != MV_OK)
		return -EINVAL;

	return 0;
}

u32 mv_eth_tool_get_link(struct net_device *netdev)
{
	mv_eth_priv 		*priv = MV_ETH_PRIV(netdev);
	MV_ETH_PORT_STATUS	status;

	mvEthStatusGet(priv->hal_priv, &status);
	if (status.isLinkUp == MV_TRUE)
		return 1;
	
	return 0;
}

int mv_eth_tool_get_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(netdev);

	if (mvEthCoalGet(priv->hal_priv, &cmd->rx_coalesce_usecs,
	    &cmd->tx_coalesce_usecs) != MV_OK)
		return -EINVAL;
	
	return 0;
}

int mv_eth_tool_set_coalesce(struct net_device *netdev,
			     struct ethtool_coalesce *cmd)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(netdev);

	if ((cmd->rx_coalesce_usecs == 0) ||
		(cmd->tx_coalesce_usecs == 0)) {
		 
		return -EPERM;
	}

	if ((cmd->rx_coalesce_usecs * 166 / 64 > 0x3FFF) ||
		(cmd->tx_coalesce_usecs * 166 / 64 > 0x3FFF))
		return -EINVAL;
	
	mv_eth_rxcoal_set(priv->port, cmd->rx_coalesce_usecs);
	mv_eth_txcoal_set(priv->port, cmd->tx_coalesce_usecs);
	return 0;
}

void mv_eth_tool_get_ringparam( struct net_device *netdev,
				struct ethtool_ringparam *ring)
{
	ring->rx_max_pending = 4096;
	ring->tx_max_pending = 4096;
	ring->rx_mini_max_pending = 0;
	ring->rx_jumbo_max_pending = 0;
	ring->rx_pending = CONFIG_MV_ETH_NUM_OF_RX_DESCR;
	ring->tx_pending = CONFIG_MV_ETH_NUM_OF_TX_DESCR;
	ring->rx_mini_pending = 0;
	ring->rx_jumbo_pending = 0;
}

void mv_eth_tool_get_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	mv_eth_priv 		*priv = MV_ETH_PRIV(netdev);
	ETH_PORT_CTRL		*pPortCtrl = (ETH_PORT_CTRL*)(priv->hal_priv);
	MV_U32			reg;

	reg = MV_REG_READ(ETH_PORT_SERIAL_CTRL_REG(pPortCtrl->portNo));
	
	pause->rx_pause = 0;
	pause->tx_pause = 0;

	if (reg & ETH_DISABLE_FC_AUTO_NEG_MASK) {  
		pause->autoneg = AUTONEG_DISABLE;
		if (reg & ETH_SET_FLOW_CTRL_MASK)
		{
			pause->rx_pause = 1;
			pause->tx_pause = 1;
		}
	} else {  
		pause->autoneg = AUTONEG_ENABLE;
		if (reg & ETH_ADVERTISE_SYM_FC_MASK)
		{
			pause->rx_pause = 1;
			pause->tx_pause = 1;
		}
	}
}

int mv_eth_tool_set_pauseparam( struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	mv_eth_priv 		*priv = MV_ETH_PRIV(netdev);
	
	if (pause->rx_pause && pause->tx_pause) {  
		if (pause->autoneg) {  
			return mvEthFlowCtrlSet(priv->hal_priv, MV_ETH_FC_AN_ADV_SYM);
		} else {  
			return mvEthFlowCtrlSet(priv->hal_priv, MV_ETH_FC_ENABLE);
		}
	} else if (!pause->rx_pause && !pause->tx_pause) {  
		if (pause->autoneg) {  
			return mvEthFlowCtrlSet(priv->hal_priv, MV_ETH_FC_AN_ADV_DIS);
		} else {  
			return mvEthFlowCtrlSet(priv->hal_priv, MV_ETH_FC_DISABLE);
		}
	}

	return -EINVAL;
}

u32 mv_eth_tool_get_rx_csum(struct net_device *netdev)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(netdev);
#ifdef RX_CSUM_OFFLOAD
	return (priv->rx_csum_offload != 0);
#else
	return 0;
#endif
}

int mv_eth_tool_set_rx_csum(struct net_device *netdev, uint32_t data)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(netdev);
#ifdef RX_CSUM_OFFLOAD
	priv->rx_csum_offload = data;
	return 0;
#else
	return data ? -EINVAL : 0;
#endif
}

int mv_eth_tool_set_tx_csum(struct net_device *netdev, uint32_t data)
{
#ifdef TX_CSUM_OFFLOAD
	if (data) {
		netdev->features |= NETIF_F_IP_CSUM;
	} else {
		netdev->features &= ~NETIF_F_IP_CSUM;
	}
	return 0;
#else
	return data ? -EINVAL : 0;
#endif  
}

int mv_eth_tool_set_tso(struct net_device *netdev, uint32_t data)
{
#ifdef ETH_INCLUDE_TSO
	if (data) {
		netdev->features |= NETIF_F_TSO;
	} else {
		netdev->features &= ~NETIF_F_TSO;
	}
	return 0;
#else
	return data ? -EINVAL : 0;
#endif  
}

int mv_eth_tool_set_ufo(struct net_device *netdev, uint32_t data)
{
#ifdef ETH_INCLUDE_UFO
	if (data) {
		netdev->features |= NETIF_F_UFO;
	} else {
		netdev->features &= ~NETIF_F_UFO;
	}
	return 0;
#else
	return data ? -EINVAL : 0;
#endif  
}

void mv_eth_tool_get_strings(struct net_device *netdev,
			     uint32_t stringset, uint8_t *data)
{
	uint8_t *p = data;
	int i, q;
	char qnum[8][4] = {" Q0"," Q1"," Q2"," Q3"," Q4"," Q5"," Q6"," Q7"};

	switch (stringset) {
		case ETH_SS_TEST:
			 
			break;
		case ETH_SS_STATS:
			for (i = 0; i < MV_ETH_TOOL_GLOBAL_STATS_LEN; i++) {
				memcpy(p, mv_eth_tool_global_strings_stats[i].stat_string,
				       ETH_GSTRING_LEN);
				p += ETH_GSTRING_LEN;
			}
			for (q = 0; q < MV_ETH_RX_Q_NUM; q++) {
				for (i = 0; i < MV_ETH_TOOL_RX_QUEUE_STATS_LEN; i++) {
					const char *str = mv_eth_tool_rx_queue_strings_stats[i].stat_string;
					memcpy(p, str, ETH_GSTRING_LEN);
					strcat(p, qnum[q]);
					p += ETH_GSTRING_LEN;
				}
			}
			for (q = 0; q < MV_ETH_TX_Q_NUM; q++) {
				for (i = 0; i < MV_ETH_TOOL_TX_QUEUE_STATS_LEN; i++) {
					const char *str = mv_eth_tool_tx_queue_strings_stats[i].stat_string;
					memcpy(p, str, ETH_GSTRING_LEN);
					strcat(p, qnum[q]);
					p += ETH_GSTRING_LEN;
				}
			}
			break;
	}
}

#define ETH_TOOL_PHY_LED_CTRL_PAGE	3
#define ETH_TOOL_PHY_LED_CTRL_REG	16

int mv_eth_tool_phys_id(struct net_device *netdev, u32 data)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(netdev);
	u16		old_led_state;

	if(!data || data > (u32)(MAX_SCHEDULE_TIMEOUT / HZ))
		data = (u32)(MAX_SCHEDULE_TIMEOUT / HZ);
	
	mv_eth_tool_read_phy_reg(priv->phy_id, ETH_TOOL_PHY_LED_CTRL_PAGE,
				ETH_TOOL_PHY_LED_CTRL_REG, &old_led_state);
	 
	mv_eth_tool_write_phy_reg(priv->phy_id, ETH_TOOL_PHY_LED_CTRL_PAGE,
				ETH_TOOL_PHY_LED_CTRL_REG, 0x0BBB);
	msleep_interruptible(data * 1000);
	mv_eth_tool_write_phy_reg(priv->phy_id, ETH_TOOL_PHY_LED_CTRL_PAGE,
				  ETH_TOOL_PHY_LED_CTRL_REG, old_led_state);
	return 0;
}

int mv_eth_tool_get_stats_count(struct net_device *netdev)
{
	return MV_ETH_TOOL_STATS_LEN;
}

void mv_eth_tool_get_ethtool_stats(struct net_device *netdev,
				   struct ethtool_stats *stats, uint64_t *data)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(netdev);
	uint64_t	*pdest = data;
	int 		i, q;

	for (i = 0; i < MV_ETH_TOOL_GLOBAL_STATS_LEN; i++) {
		char *p = (char *)priv +
			mv_eth_tool_global_strings_stats[i].stat_offset;
		pdest[i] =  *(uint32_t *)p;
	}
	pdest += MV_ETH_TOOL_GLOBAL_STATS_LEN;
	
	for (q = 0; q < MV_ETH_RX_Q_NUM; q++) {
		for (i = 0; i < MV_ETH_TOOL_RX_QUEUE_STATS_LEN; i++) {
			char *p = (char *)priv +
				mv_eth_tool_rx_queue_strings_stats[i].stat_offset;
			pdest[i] =  *((uint32_t *)p + q);
		}
		pdest += MV_ETH_TOOL_RX_QUEUE_STATS_LEN;
	}

	for (q = 0; q < MV_ETH_TX_Q_NUM; q++) {
		for (i = 0; i < MV_ETH_TOOL_TX_QUEUE_STATS_LEN; i++) {
			char *p = (char *)priv +
				mv_eth_tool_tx_queue_strings_stats[i].stat_offset;
			pdest[i] =  *((uint32_t *)p + q);
		}
		pdest += MV_ETH_TOOL_TX_QUEUE_STATS_LEN;
	}
}

#ifdef MY_ABC_HERE
MV_U32 syno_wol_support(mv_eth_priv *priv)
{
	if (MV_PHY_ID_131X == priv->phy_chip) {
		return WAKE_MAGIC;
	}

	return 0;
}

static void syno_get_wol(struct net_device *dev, struct ethtool_wolinfo *wol)
{
	mv_eth_priv 	*priv = MV_ETH_PRIV(dev);

	wol->supported = syno_wol_support(priv);
	wol->wolopts = priv->wol;
}

static int syno_set_wol(struct net_device *dev, struct ethtool_wolinfo *wol)
{
	mv_eth_priv     *priv = MV_ETH_PRIV(dev);

	if ((wol->wolopts & ~syno_wol_support(priv))) {
		return -EOPNOTSUPP;
	}

	priv->wol = wol->wolopts;
	return 0;
}
#endif


/** @pfe_eth.c.
 *  Ethernet driver for to handle exception path for PFE.
 *  - uses HIF functions to send/receive packets.
 *  - uses ctrl function to start/stop interfaces.
 *  - uses direct register accesses to control phy operation.
 */
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>

#include <net/ip.h>
#include <net/sock.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>

#if defined(CONFIG_NF_CONNTRACK_MARK)
#include <net/netfilter/nf_conntrack.h>
#endif

#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
#include <net/xfrm.h>
#endif

#include "pfe_mod.h"
#include "pfe_eth.h"

const char comcerto_eth_driver_version[]="1.0";
static void *cbus_emac_base[3];
static void *cbus_gpi_base[3];

static void pfe_eth_exit_one(struct pfe_eth_priv_s *priv);
static void pfe_eth_flush_tx(struct pfe_eth_priv_s *priv, int force);
static void pfe_eth_flush_txQ(struct pfe_eth_priv_s *priv, int txQ_num, int from_tx, int n_desc);

#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
extern struct xfrm_state *xfrm_state_lookup_byhandle(struct net *net, u16 handle);
#endif

unsigned int gemac_regs[] = {
	0x0000,	/* Network control */
	0x0004,	/* Network configuration */
	0x0008, /* Network status */
	0x0010, /* DMA configuration */
	0x0014, /* Transmit status */
	0x0020, /* Receive status */
	0x0024, /* Interrupt status */
	0x0030, /* Interrupt mask */
	0x0038, /* Received pause quantum */
	0x003c, /* Transmit pause quantum */
	0x0080, /* Hash register bottom [31:0] */
	0x0084, /* Hash register bottom [63:32] */
	0x0088, /* Specific address 1 bottom [31:0] */
	0x008c, /* Specific address 1 top [47:32] */
	0x0090, /* Specific address 2 bottom [31:0] */
	0x0094, /* Specific address 2 top [47:32] */
	0x0098, /* Specific address 3 bottom [31:0] */
	0x009c, /* Specific address 3 top [47:32] */
	0x00a0, /* Specific address 4 bottom [31:0] */
	0x00a4, /* Specific address 4 top [47:32] */
	0x00a8, /* Type ID Match 1 */
	0x00ac, /* Type ID Match 2 */
	0x00b0, /* Type ID Match 3 */
	0x00b4, /* Type ID Match 4 */
	0x00b8, /* Wake Up ON LAN  */
	0x00bc, /* IPG stretch register */
	0x00c0, /* Stacked VLAN Register */
	0x00fc, /* Module ID */
	0x07a0  /* EMAC Control register */
};

/********************************************************************/
/*                   SYSFS INTERFACE				    */
/********************************************************************/
#ifdef PFE_ETH_TSO_STATS
/** pfe_eth_show_tso_stats
 *
 */
static ssize_t pfe_eth_show_tso_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	ssize_t len = 0;
	int i;

	for (i = 0; i < 32; i++)
		len += sprintf(buf + len, "TSO packets > %dKBytes = %d\n", i * 2, priv->tso_len_counters[i]);

	return len;
}

/** pfe_eth_set_tso_stats
 *
 */
static ssize_t pfe_eth_set_tso_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));

	memset(priv->tso_len_counters, 0, sizeof(priv->tso_len_counters));

	return count;
}
#endif

#ifdef PFE_ETH_LRO_STATS
/*
 * pfe_eth_show_lro_nb_stats
 */
static ssize_t pfe_eth_show_lro_nb_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	ssize_t len = 0;
	int i;

	for (i = 0; i < LRO_NB_COUNT_MAX; i++)
		len += sprintf(buf + len, "%d fragments packets = %d\n", i, priv->lro_nb_counters[i]);

	return len;
}

/*
 * pfe_eth_set_lro_nb_stats
 */
static ssize_t pfe_eth_set_lro_nb_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));

	memset(priv->lro_nb_counters, 0, sizeof(priv->lro_nb_counters));

	return count;
}

/*
 * pfe_eth_show_lro_len_stats
 */
static ssize_t pfe_eth_show_lro_len_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	ssize_t len = 0;
	int i;

	for (i = 0; i < LRO_LEN_COUNT_MAX; i++)
		len += sprintf(buf + len, "RX packets > %dKBytes = %d\n", i * 2, priv->lro_len_counters[i]);

	return len;
}

/*
 * pfe_eth_set_lro_len_stats
 */
static ssize_t pfe_eth_set_lro_len_stats(struct device *dev, 
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));

	memset(priv->lro_len_counters, 0, sizeof(priv->lro_len_counters));

	return count;
}
#endif

#ifdef PFE_ETH_NAPI_STATS
/*
 * pfe_eth_show_napi_stats
 */
static ssize_t pfe_eth_show_napi_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	ssize_t len = 0;

	len += sprintf(buf + len, "sched:  %d\n", priv->napi_counters[NAPI_SCHED_COUNT]);
	len += sprintf(buf + len, "poll:   %d\n", priv->napi_counters[NAPI_POLL_COUNT]);
	len += sprintf(buf + len, "packet: %d\n", priv->napi_counters[NAPI_PACKET_COUNT]);
	len += sprintf(buf + len, "budget: %d\n", priv->napi_counters[NAPI_FULL_BUDGET_COUNT]);
	len += sprintf(buf + len, "desc:   %d\n", priv->napi_counters[NAPI_DESC_COUNT]);

	return len;
}

/*
 * pfe_eth_set_napi_stats
 */
static ssize_t pfe_eth_set_napi_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));

	memset(priv->napi_counters, 0, sizeof(priv->napi_counters));

	return count;
}
#endif
#ifdef PFE_ETH_TX_STATS
/** pfe_eth_show_tx_stats
 *
 */
static ssize_t pfe_eth_show_tx_stats(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	ssize_t len = 0;
	int i;

	len += sprintf(buf + len, "TX queues stats:\n");

	for (i = 0; i < EMAC_TXQ_CNT; i++) {
		struct netdev_queue *tx_queue = netdev_get_tx_queue(priv->dev, i); 

		len += sprintf(buf + len, "\n");
		__netif_tx_lock_bh(tx_queue);

		hif_tx_lock(&pfe->hif);
		len += sprintf(buf + len, "Queue %2d :  credits               = %10d\n", i, hif_lib_tx_credit_avail(pfe, priv->id, i));
		len += sprintf(buf + len, "            tx packets            = %10d\n",  pfe->tmu_credit.tx_packets[priv->id][i]);
		hif_tx_unlock(&pfe->hif);

		/* Don't output additionnal stats if queue never used */
		if (!pfe->tmu_credit.tx_packets[priv->id][i])
			goto skip;

		len += sprintf(buf + len, "            clean_fail            = %10d\n", priv->clean_fail[i]);
		len += sprintf(buf + len, "            stop_queue            = %10d\n", priv->stop_queue_total[i]);
		len += sprintf(buf + len, "            stop_queue_hif        = %10d\n", priv->stop_queue_hif[i]);
		len += sprintf(buf + len, "            stop_queue_hif_client = %10d\n", priv->stop_queue_hif_client[i]);
		len += sprintf(buf + len, "            stop_queue_credit     = %10d\n", priv->stop_queue_credit[i]);
skip:
		__netif_tx_unlock_bh(tx_queue);
	}
	return len;
}

/** pfe_eth_set_tx_stats
 *
 */
static ssize_t pfe_eth_set_tx_stats(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	int i;

	for (i = 0; i < EMAC_TXQ_CNT; i++) {
		struct netdev_queue *tx_queue = netdev_get_tx_queue(priv->dev, i); 

		__netif_tx_lock_bh(tx_queue);
		priv->clean_fail[i] = 0;
		priv->stop_queue_total[i] = 0;
		priv->stop_queue_hif[i] = 0;
		priv->stop_queue_hif_client[i]= 0;
		priv->stop_queue_credit[i] = 0;
		__netif_tx_unlock_bh(tx_queue);
	}

	return count;
}
#endif
/** pfe_eth_show_txavail
 *
 */
static ssize_t pfe_eth_show_txavail(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	ssize_t len = 0;
	int i;

	for (i = 0; i < EMAC_TXQ_CNT; i++) {
		struct netdev_queue *tx_queue = netdev_get_tx_queue(priv->dev, i); 

		__netif_tx_lock_bh(tx_queue);

		len += sprintf(buf + len, "%d", hif_lib_tx_avail(&priv->client, i));

		__netif_tx_unlock_bh(tx_queue);

		if (i == (EMAC_TXQ_CNT - 1))
			len += sprintf(buf + len, "\n");
		else
			len += sprintf(buf + len, " ");
	}

	return len;
}


/** pfe_eth_show_default_priority
 *
 */ 
static ssize_t pfe_eth_show_default_priority(struct device *dev,
		struct device_attribute *attr,
		char *buf)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	unsigned long flags;
	int rc;

	spin_lock_irqsave(&priv->lock, flags);
	rc = sprintf(buf, "%d\n", priv->default_priority);
	spin_unlock_irqrestore(&priv->lock, flags);

	return rc;
}

/** pfe_eth_set_default_priority
 *
 */

static ssize_t pfe_eth_set_default_priority(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct pfe_eth_priv_s *priv = netdev_priv(to_net_dev(dev));
	unsigned long flags;

	spin_lock_irqsave(&priv->lock, flags);
	priv->default_priority = simple_strtoul(buf, NULL, 0);
	spin_unlock_irqrestore(&priv->lock, flags);

	return count;
}

static DEVICE_ATTR(txavail, 0444, pfe_eth_show_txavail, NULL);
static DEVICE_ATTR(default_priority, 0644, pfe_eth_show_default_priority, pfe_eth_set_default_priority);

#ifdef PFE_ETH_NAPI_STATS
static DEVICE_ATTR(napi_stats, 0644, pfe_eth_show_napi_stats, pfe_eth_set_napi_stats);
#endif

#ifdef PFE_ETH_TX_STATS
static DEVICE_ATTR(tx_stats, 0644, pfe_eth_show_tx_stats, pfe_eth_set_tx_stats);
#endif

#ifdef PFE_ETH_TSO_STATS
static DEVICE_ATTR(tso_stats, 0644, pfe_eth_show_tso_stats, pfe_eth_set_tso_stats);
#endif

#ifdef PFE_ETH_LRO_STATS
static DEVICE_ATTR(lro_nb_stats, 0644, pfe_eth_show_lro_nb_stats, pfe_eth_set_lro_nb_stats);
static DEVICE_ATTR(lro_len_stats, 0644, pfe_eth_show_lro_len_stats, pfe_eth_set_lro_len_stats);
#endif

/** pfe_eth_sysfs_init
 *
 */
static int pfe_eth_sysfs_init(struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	int err;

	/* Initialize the default values */
	priv->default_priority = 0;

	/* Create our sysfs files */
	err = device_create_file(&dev->dev, &dev_attr_default_priority);
	if (err) {
		netdev_err(dev, "failed to create default_priority sysfs files\n");
		goto err_priority;
	}

	err = device_create_file(&dev->dev, &dev_attr_txavail);
	if (err) {
		netdev_err(dev, "failed to create default_priority sysfs files\n");
		goto err_txavail;
	}

#ifdef PFE_ETH_NAPI_STATS
	err = device_create_file(&dev->dev, &dev_attr_napi_stats);
	if (err) {
		netdev_err(dev, "failed to create napi stats sysfs files\n");
		goto err_napi;
	}
#endif

#ifdef PFE_ETH_TX_STATS
	err = device_create_file(&dev->dev, &dev_attr_tx_stats);
	if (err) {
		netdev_err(dev, "failed to create tx stats sysfs files\n");
		goto err_tx;
	}
#endif

#ifdef PFE_ETH_TSO_STATS
	err = device_create_file(&dev->dev, &dev_attr_tso_stats);
	if (err) {
                netdev_err(dev, "failed to create tso stats sysfs files\n");
		goto err_tso;
        }
#endif

#ifdef PFE_ETH_LRO_STATS
	err = device_create_file(&dev->dev, &dev_attr_lro_nb_stats);
	if (err) {
		netdev_err(dev, "failed to create lro nb stats sysfs files\n");
                goto err_lro_nb;
        }

	err = device_create_file(&dev->dev, &dev_attr_lro_len_stats);
	if (err) {
		netdev_err(dev, "failed to create lro len stats sysfs files\n");
		goto err_lro_len;
	}
#endif

	return 0;

#ifdef PFE_ETH_LRO_STATS
err_lro_len:
	device_remove_file(&dev->dev, &dev_attr_lro_nb_stats);

err_lro_nb:
#endif

#ifdef PFE_ETH_TSO_STATS
	device_remove_file(&dev->dev, &dev_attr_tso_stats);

err_tso:
#endif
#ifdef PFE_ETH_TX_STATS
	device_remove_file(&dev->dev, &dev_attr_tx_stats);

err_tx:
#endif
#ifdef PFE_ETH_NAPI_STATS
	device_remove_file(&dev->dev, &dev_attr_napi_stats);

err_napi:
#endif
	device_remove_file(&dev->dev, &dev_attr_txavail);

err_txavail:
	device_remove_file(&dev->dev, &dev_attr_default_priority);

err_priority:
	return -1;
}

/** pfe_eth_sysfs_exit
 *
 */
void pfe_eth_sysfs_exit(struct net_device *dev)
{
#ifdef PFE_ETH_LRO_STATS
	device_remove_file(&dev->dev, &dev_attr_lro_nb_stats);
	device_remove_file(&dev->dev, &dev_attr_lro_len_stats);
#endif

#ifdef PFE_ETH_TSO_STATS
	device_remove_file(&dev->dev, &dev_attr_tso_stats);
#endif

#ifdef PFE_ETH_TX_STATS
	device_remove_file(&dev->dev, &dev_attr_tx_stats);
#endif

#ifdef PFE_ETH_NAPI_STATS
	device_remove_file(&dev->dev, &dev_attr_napi_stats);
#endif
	device_remove_file(&dev->dev, &dev_attr_txavail);
	device_remove_file(&dev->dev, &dev_attr_default_priority);
}

/*************************************************************************/
/*		ETHTOOL INTERCAE					 */
/*************************************************************************/
static char stat_gstrings[][ETH_GSTRING_LEN] = {
	"tx- octets",
	"tx- packets",
	"tx- broadcast",
	"tx- multicast",
	"tx- pause",
	"tx- 64 bytes packets",
	"tx- 64 - 127 bytes packets",
	"tx- 128 - 255 bytes packets",
	"tx- 256 - 511 bytes packets",
	"tx- 512 - 1023 bytes packets",
	"tx- 1024 - 1518 bytes packets",
	"tx- > 1518 bytes packets",
	"tx- underruns  - errors",
	"tx- single collision",
	"tx- multi collision",
	"tx- exces. collision  - errors",
	"tx- late collision  - errors",
	"tx- deferred",
	"tx- carrier sense - errors",
	"rx- octets",
	"rx- packets",
	"rx- broadcast",
	"rx- multicast",
	"rx- pause",
	"rx- 64 bytes packets",
	"rx- 64 - 127 bytes packets",
	"rx- 128 - 255 bytes packets",
	"rx- 256 - 511 bytes packets",
	"rx- 512 - 1023 bytes packets",
	"rx- 1024 - 1518 bytes packets",
	"rx- > 1518 bytes packets",
	"rx- undersize -errors",
	"rx- oversize  - errors ",
	"rx- jabbers - errors",
	"rx- fcs - errors",
	"rx- length - errors",
	"rx- symbol - errors",
	"rx- align - errors",
	"rx- ressource - errors",
	"rx- overrun - errors",
	"rx- IP cksum - errors",
	"rx- TCP cksum - errors",
	"rx- UDP cksum - errors"
};


/**
 * pfe_eth_gstrings - Fill in a buffer with the strings which correspond to
 *                    the stats.
 *
 */
static void pfe_eth_gstrings(struct net_device *dev, u32 stringset, u8 * buf)
{
	switch (stringset) {
		case ETH_SS_STATS:
			memcpy(buf, stat_gstrings, (EMAC_RMON_LEN - 2) * ETH_GSTRING_LEN);
			break;

		default:
			WARN_ON(1);
			break;
	}
}

/** 
 * pfe_eth_fill_stats - Fill in an array of 64-bit statistics from 
 *			various sources. This array will be appended 
 *			to the end of the ethtool_stats* structure, and 
 *			returned to user space
 */
static void pfe_eth_fill_stats(struct net_device *dev, struct ethtool_stats *dummy, u64 * buf)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	int i;
	for (i=0;i<EMAC_RMON_LEN;i++, buf++) {
		*buf = readl(priv->EMAC_baseaddr + EMAC_RMON_BASE_OFST + (i << 2));	
		if ( ( i == EMAC_RMON_TXBYTES_POS ) || ( i == EMAC_RMON_RXBYTES_POS ) ){
			i++;
			*buf |= (u64)readl(priv->EMAC_baseaddr + EMAC_RMON_BASE_OFST + (i << 2)) << 32;
		}
	}

}

/**
 * pfe_eth_stats_count - Returns the number of stats (and their corresponding strings) 
 *
 */
static int pfe_eth_stats_count(struct net_device *dev, int sset)
{
	switch (sset) {
		case ETH_SS_STATS:
			return EMAC_RMON_LEN - 2;
		default:
			return -EOPNOTSUPP;
	}
}

/**
 * pfe_eth_get_drvinfo -  Fills in the drvinfo structure with some basic info 
 *
 */
static void pfe_eth_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *drvinfo)
{
	strncpy(drvinfo->driver, DRV_NAME, COMCERTO_INFOSTR_LEN);
	strncpy(drvinfo->version, comcerto_eth_driver_version, COMCERTO_INFOSTR_LEN);
	strncpy(drvinfo->fw_version, "N/A", COMCERTO_INFOSTR_LEN);
	strncpy(drvinfo->bus_info, "N/A", COMCERTO_INFOSTR_LEN);
	drvinfo->testinfo_len = 0;
	drvinfo->regdump_len = 0;
	drvinfo->eedump_len = 0;
}

/**
 * pfe_eth_set_settings - Used to send commands to PHY. 
 *
 */

static int pfe_eth_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	struct phy_device *phydev = priv->phydev;

	if (NULL == phydev)
		return -ENODEV;

	return phy_ethtool_sset(phydev, cmd);
}


/**
 * pfe_eth_getsettings - Return the current settings in the ethtool_cmd structure.
 *
 */
static int pfe_eth_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	struct phy_device *phydev = priv->phydev;

	if (NULL == phydev)
		return -ENODEV;

	return phy_ethtool_gset(phydev, cmd);
}

/**
 * pfe_eth_gemac_reglen - Return the length of the register structure.
 *
 */
static int pfe_eth_gemac_reglen(struct net_device *dev)
{
	return (sizeof (gemac_regs)/ sizeof(u32)) + (( MAX_UC_SPEC_ADDR_REG - 3 ) * 2);
}

/**
 * pfe_eth_gemac_get_regs - Return the gemac register structure.
 *
 */
static void  pfe_eth_gemac_get_regs(struct net_device *dev, struct ethtool_regs *regs, void *regbuf)
{
	int i,j;
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	u32 *buf = (u32 *) regbuf;

	for (i = 0; i < sizeof (gemac_regs) / sizeof (u32); i++)
		buf[i] = readl( priv->EMAC_baseaddr + gemac_regs[i] );

	for (j = 0; j < (( MAX_UC_SPEC_ADDR_REG - 3 ) * 2); j++,i++)
		buf[i] = readl( priv->EMAC_baseaddr + EMAC_SPEC5_ADD_BOT + (j<<2) );

}

/**
 * pfe_eth_get_msglevel - Gets the debug message mask.
 *
 */
static uint32_t pfe_eth_get_msglevel(struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);

	return priv->msg_enable;
}

/**
 * pfe_eth_set_msglevel - Sets the debug message mask.
 *
 */
static void pfe_eth_set_msglevel(struct net_device *dev, uint32_t data)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);

	priv->msg_enable = data;
}

struct ethtool_ops pfe_ethtool_ops = {
	.get_settings = pfe_eth_get_settings,
	.set_settings = pfe_eth_set_settings,
	.get_drvinfo = pfe_eth_get_drvinfo,
	.get_regs_len = pfe_eth_gemac_reglen,
	.get_regs = pfe_eth_gemac_get_regs,
	.get_link = ethtool_op_get_link,
	.get_strings = pfe_eth_gstrings,
	.get_sset_count = pfe_eth_stats_count,
	.get_ethtool_stats = pfe_eth_fill_stats,
	.get_msglevel = pfe_eth_get_msglevel,
	.set_msglevel = pfe_eth_set_msglevel,
};



/** pfe_eth_mdio_reset
 */
static int pfe_eth_mdio_reset(struct mii_bus *bus)
{
	struct pfe_eth_priv_s *priv = (struct pfe_eth_priv_s *)bus->priv;

	netif_info(priv, drv, priv->dev, "%s\n", __func__);

#if !defined(CONFIG_PLATFORM_EMULATION)
	mutex_lock(&bus->mdio_lock);

	/* Setup the MII Mgmt clock speed */
	gemac_set_mdc_div(priv->EMAC_baseaddr, priv->mdc_div);

	/* Reset the management interface */
	__raw_writel(__raw_readl(priv->EMAC_baseaddr + EMAC_NETWORK_CONTROL) | EMAC_MDIO_EN, 
			priv->EMAC_baseaddr + EMAC_NETWORK_CONTROL);

	/* Wait until the bus is free */
	while(!(__raw_readl(priv->EMAC_baseaddr + EMAC_NETWORK_STATUS) & EMAC_PHY_IDLE));

	mutex_unlock(&bus->mdio_lock);
#endif

	return 0;
}


/** pfe_eth_gemac_phy_timeout
 *
 */
static int pfe_eth_gemac_phy_timeout(struct pfe_eth_priv_s *priv, int timeout)
{
	while(!(__raw_readl(priv->EMAC_baseaddr + EMAC_NETWORK_STATUS) & EMAC_PHY_IDLE)) {

		if (timeout-- <= 0) {
			return -1;
		}

		udelay(10);
	}

	return 0;
}


/** pfe_eth_mdio_write
 */
static int pfe_eth_mdio_write(struct mii_bus *bus, int mii_id, int regnum, u16 value)
{
	struct pfe_eth_priv_s *priv = (struct pfe_eth_priv_s *)bus->priv;
	u32 write_data;

#if !defined(CONFIG_PLATFORM_EMULATION)

	netif_info(priv, hw, priv->dev, "%s: phy %d\n", __func__, mii_id);

//	netif_info(priv, hw, priv->dev, "%s %d %d %x\n", bus->id, mii_id, regnum, value);

	write_data = 0x50020000;
	write_data |= ((mii_id << 23) | (regnum << 18) | value);
	__raw_writel(write_data, priv->EMAC_baseaddr + EMAC_PHY_MANAGEMENT);

	if (pfe_eth_gemac_phy_timeout(priv, EMAC_MDIO_TIMEOUT)){
		netdev_err(priv->dev, "%s: phy MDIO write timeout\n", __func__);
		return -1;
	}

#endif

	return 0;
}


/** pfe_eth_mdio_read
 */
static int pfe_eth_mdio_read(struct mii_bus *bus, int mii_id, int regnum)
{
	struct pfe_eth_priv_s *priv = (struct pfe_eth_priv_s *)bus->priv;
	u16 value = 0;
	u32 write_data;

#if !defined(CONFIG_PLATFORM_EMULATION)
	netif_info(priv, hw, priv->dev, "%s: phy %d\n", __func__, mii_id);

	write_data = 0x60020000;
	write_data |= ((mii_id << 23) | (regnum << 18));

	__raw_writel(write_data, priv->EMAC_baseaddr + EMAC_PHY_MANAGEMENT);

	if (pfe_eth_gemac_phy_timeout( priv, EMAC_MDIO_TIMEOUT))	{
		netdev_err(priv->dev, "%s: phy MDIO read timeout\n", __func__);
		return -1;	
	}

	value = __raw_readl(priv->EMAC_baseaddr + EMAC_PHY_MANAGEMENT) & 0xFFFF;
#endif

//	netif_info(priv, hw, priv->dev, "%s %d %d %x\n", bus->id, mii_id, regnum, value);

	return value;
}


/** pfe_eth_mdio_init
 */
static int pfe_eth_mdio_init(struct pfe_eth_priv_s *priv, struct comcerto_mdio_platform_data *minfo)
{
	struct mii_bus *bus;
	int rc;

	netif_info(priv, drv, priv->dev, "%s\n", __func__);

#if !defined(CONFIG_PLATFORM_EMULATION) 
	bus = mdiobus_alloc();
	if (!bus) {
		netdev_err(priv->dev, "mdiobus_alloc() failed\n");
		rc = -ENOMEM;
		goto err0;
	}

	bus->name = "Comcerto MDIO Bus";
	bus->read = &pfe_eth_mdio_read;
	bus->write = &pfe_eth_mdio_write;
	bus->reset = &pfe_eth_mdio_reset;
	snprintf(bus->id, MII_BUS_ID_SIZE, "comcerto-%x", priv->id);
	bus->priv = priv;

	bus->phy_mask = minfo->phy_mask;
	priv->mdc_div = minfo->mdc_div;

	if (!priv->mdc_div)
		priv->mdc_div = 64;

	bus->irq = minfo->irq;

	bus->parent = priv->pfe->dev;

	netif_info(priv, drv, priv->dev, "%s: mdc_div: %d, phy_mask: %x \n", __func__, priv->mdc_div, bus->phy_mask);

	rc = mdiobus_register(bus);
	if (rc) {
		netdev_err(priv->dev, "mdiobus_register(%s) failed\n", bus->name);
		goto err1;
	}

	priv->mii_bus = bus;

	return 0;

err1:
	mdiobus_free(bus);
err0:
	return rc;
#else
	return 0;
#endif

}

/** pfe_eth_mdio_exit
 */
static void pfe_eth_mdio_exit(struct mii_bus *bus)
{
	if (!bus)
		return;

	netif_info((struct pfe_eth_priv_s *)bus->priv, drv, ((struct pfe_eth_priv_s *)(bus->priv))->dev, "%s\n", __func__);

	mdiobus_unregister(bus);
	mdiobus_free(bus);
}

/** pfe_get_interface
 */
static phy_interface_t pfe_get_interface(struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	u32 mii_mode = priv->einfo->mii_config;

	netif_info(priv, drv, dev, "%s\n", __func__);

	if (priv->einfo->gemac_mode & (GEMAC_SW_CONF)) {
		switch (mii_mode) {
			case CONFIG_COMCERTO_USE_GMII:
				return PHY_INTERFACE_MODE_GMII;
				break;
			case CONFIG_COMCERTO_USE_RGMII:
				return PHY_INTERFACE_MODE_RGMII;
				break;
			case CONFIG_COMCERTO_USE_RMII:
				return PHY_INTERFACE_MODE_RMII;
				break;
			case CONFIG_COMCERTO_USE_SGMII:
				return PHY_INTERFACE_MODE_SGMII;
				break;

			default :
			case CONFIG_COMCERTO_USE_MII:
				return PHY_INTERFACE_MODE_MII;
				break;

		}
	} else {
		// Bootstrap config read from controller
		BUG();
		return 0;
	}
}

/** pfe_get_phydev_speed
 */
static int pfe_get_phydev_speed(struct phy_device *phydev)
{
	switch (phydev->speed) {
		case 10:
			return SPEED_10M;
		case 100:
			return SPEED_100M;
		case 1000:
		default:
			return SPEED_1000M;
	}

}

/** pfe_get_phydev_duplex
 */
static int pfe_get_phydev_duplex(struct phy_device *phydev)
{
	return ( phydev->duplex == DUPLEX_HALF ) ? DUP_HALF:DUP_FULL ;
}

/** pfe_eth_adjust_link
 */
static void pfe_eth_adjust_link(struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	unsigned long flags;
	struct phy_device *phydev = priv->phydev;
	int new_state = 0;

	netif_info(priv, drv, dev, "%s\n", __func__);

	spin_lock_irqsave(&priv->lock, flags);
	if (phydev->link) {
		/* Now we make sure that we can be in full duplex mode.
		 * If not, we operate in half-duplex mode. */
		if (phydev->duplex != priv->oldduplex) {
			new_state = 1;
			gemac_set_duplex(priv->EMAC_baseaddr, pfe_get_phydev_duplex(phydev));
			priv->oldduplex = phydev->duplex;
		}

		if (phydev->speed != priv->oldspeed) {
			new_state = 1;
			gemac_set_speed(priv->EMAC_baseaddr, pfe_get_phydev_speed(phydev));
			priv->oldspeed = phydev->speed;
		}

		if (!priv->oldlink) {
			new_state = 1;
			priv->oldlink = 1;
		}

	} else if (priv->oldlink) {
		new_state = 1;
		priv->oldlink = 0;
		priv->oldspeed = 0;
		priv->oldduplex = -1;
	}

	if (new_state && netif_msg_link(priv))
		phy_print_status(phydev);

	spin_unlock_irqrestore(&priv->lock, flags);
}


/** pfe_phy_exit
 */
static void pfe_phy_exit(struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);

	netif_info(priv, drv, dev, "%s\n", __func__);

	phy_disconnect(priv->phydev);
	priv->phydev = NULL;
}

/** pfe_eth_stop
 */
static void pfe_eth_stop( struct net_device *dev )
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);

	netif_info(priv, drv, dev, "%s\n", __func__);

	gemac_disable(priv->EMAC_baseaddr);
	gpi_disable(priv->GPI_baseaddr);

	/* Stop HIF GEMAC TMU queues */
	hif_lib_tmu_queue_stop(&priv->client, priv->low_tmuQ);
	hif_lib_tmu_queue_stop(&priv->client, priv->high_tmuQ);

	if (priv->phydev)
		phy_stop(priv->phydev);
}

/** pfe_eth_start
 */
static int pfe_eth_start( struct pfe_eth_priv_s *priv )
{
	netif_info(priv, drv, priv->dev, "%s\n", __func__);

	if (priv->phydev)
		phy_start(priv->phydev);

	/* Start HIF GEMAC TMU queues */
	hif_lib_tmu_queue_start(&priv->client, priv->low_tmuQ);
	hif_lib_tmu_queue_start(&priv->client, priv->high_tmuQ);
	gpi_enable(priv->GPI_baseaddr);
	gemac_enable(priv->EMAC_baseaddr);

	return 0;
}

/** pfe_phy_init
 *
 */
static int pfe_phy_init(struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	struct phy_device *phydev;
	char phy_id[MII_BUS_ID_SIZE + 3];
	char bus_id[MII_BUS_ID_SIZE];
	phy_interface_t interface;

	priv->oldlink = 0;
	priv->oldspeed = 0;
	priv->oldduplex = -1;

	sprintf(bus_id, "comcerto-%d", priv->einfo->bus_id);
	snprintf(phy_id, MII_BUS_ID_SIZE + 3, PHY_ID_FMT, bus_id, priv->einfo->phy_id);

	netif_info(priv, drv, dev, "%s: %s\n", __func__, phy_id);

	interface = pfe_get_interface(dev);

	priv->oldlink = 0;
	priv->oldspeed = 0;
	priv->oldduplex = -1;

	phydev = phy_connect(dev, phy_id, &pfe_eth_adjust_link, 0, interface);

	if (IS_ERR(phydev)) {
		netdev_err(dev, "phy_connect() failed\n");
		return PTR_ERR(phydev);
	}

	priv->phydev = phydev;

	return 0;
}

/** pfe_gemac_init
 */
static int pfe_gemac_init(struct pfe_eth_priv_s *priv)
{
	GEMAC_CFG cfg;

	netif_info(priv, ifup, priv->dev, "%s\n", __func__);

	/* software config */
	/* MII interface mode selection */ 
	switch (priv->einfo->mii_config) {
		case CONFIG_COMCERTO_USE_GMII:
			cfg.mode = GMII;
			break;

		case CONFIG_COMCERTO_USE_MII:
			cfg.mode = MII;
			break;

		case CONFIG_COMCERTO_USE_RGMII:
			cfg.mode = RGMII;
			break;

		case CONFIG_COMCERTO_USE_RMII:
			cfg.mode = RMII;
			break;

		case CONFIG_COMCERTO_USE_SGMII:
			cfg.mode = SGMII;
			break;

		default:
			cfg.mode = RGMII;
	}

	/* Speed selection */
	switch (priv->einfo->gemac_mode & GEMAC_SW_SPEED_1G ) {
		case GEMAC_SW_SPEED_1G:
			cfg.speed = SPEED_1000M;
			break;

		case GEMAC_SW_SPEED_100M:
			cfg.speed = SPEED_100M;
			break;

		case GEMAC_SW_SPEED_10M:
			cfg.speed = SPEED_10M;
			break;

		default:
			cfg.speed = SPEED_1000M;
	}

	/* Duplex selection */
	cfg.duplex =  ( priv->einfo->gemac_mode & GEMAC_SW_FULL_DUPLEX ) ? DUPLEX_FULL : DUPLEX_HALF;

	gemac_set_config( priv->EMAC_baseaddr, &cfg);
	gemac_allow_broadcast( priv->EMAC_baseaddr );
	gemac_disable_unicast( priv->EMAC_baseaddr );
	gemac_disable_multicast( priv->EMAC_baseaddr );
	gemac_disable_fcs_rx( priv->EMAC_baseaddr );
	gemac_enable_1536_rx( priv->EMAC_baseaddr );
	gemac_set_bus_width(priv->EMAC_baseaddr, 64);

	/*GEM will perform checksum verifications*/
	if (priv->dev->features & NETIF_F_RXCSUM)
		gemac_enable_rx_checksum_offload(priv->EMAC_baseaddr);
	else
		gemac_disable_rx_checksum_offload(priv->EMAC_baseaddr);	

	return 0;
}

/** pfe_eth_event_handler
 */
static int pfe_eth_event_handler(void *data, int event, int qno)
{
	struct pfe_eth_priv_s *priv = data;

	switch (event) {
	case EVENT_RX_PKT_IND:

		if (qno == 0) {
			if (napi_schedule_prep(&priv->high_napi)) {
				netif_info(priv, intr, priv->dev, "%s: schedule high prio poll\n", __func__);

#ifdef PFE_ETH_NAPI_STATS
				priv->napi_counters[NAPI_SCHED_COUNT]++;
#endif

				__napi_schedule(&priv->high_napi);
			}
		}
		else if (qno == 1) {
			if (napi_schedule_prep(&priv->low_napi)) {
				netif_info(priv, intr, priv->dev, "%s: schedule low prio poll\n", __func__);

#ifdef PFE_ETH_NAPI_STATS
				priv->napi_counters[NAPI_SCHED_COUNT]++;
#endif
				__napi_schedule(&priv->low_napi);
			}
		}
		else if (qno == 2) {
			if (napi_schedule_prep(&priv->lro_napi)) {
				netif_info(priv, intr, priv->dev, "%s: schedule lro prio poll\n", __func__);

#ifdef PFE_ETH_NAPI_STATS
				priv->napi_counters[NAPI_SCHED_COUNT]++;
#endif
				__napi_schedule(&priv->lro_napi);
			}
		}

		break;

	case EVENT_TXDONE_IND:
	case EVENT_HIGH_RX_WM:
	default:
		break;
	}

	return 0;
}

/** pfe_eth_open
 */
static int pfe_eth_open( struct net_device *dev )
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	struct hif_client_s *client;
	int rc;

	netif_info(priv, ifup, dev, "%s\n", __func__);

	/* Register client driver with HIF */
	client = &priv->client;
	memset(client, 0, sizeof(*client));
	client->id = PFE_CL_GEM0 + priv->id;
	client->tx_qn = EMAC_TXQ_CNT;
	client->rx_qn = EMAC_RXQ_CNT;
	client->priv    = priv;
	client->pfe     = priv->pfe;
	client->event_handler   = pfe_eth_event_handler;

	/* FIXME : For now hif lib sets all tx and rx queues to same size */
	client->tx_qsize = EMAC_TXQ_DEPTH;
	client->rx_qsize = EMAC_RXQ_DEPTH;

    if (alloc_on_init) {
        /* NB: tx & rx buffers allocated when device is initialized in pfe_eth_init_one() */
        client->tx_qbase = priv->tx_buf;
        client->rx_qbase = priv->rx_buf;
    }

    if ((rc = hif_lib_client_register(client))) {
		netdev_err(dev, "%s: hif_lib_client_register(%d) failed\n", __func__, client->id);
		goto err0;
	}

	netif_info(priv, drv, dev, "%s: registered client: %p\n", __func__,  client);

	/* Enable gemac tx clock */
	clk_enable(priv->gemtx_clk);

	pfe_gemac_init(priv);

	if (!is_valid_ether_addr(dev->dev_addr)) {
		netdev_err(dev, "%s: invalid MAC address\n", __func__);
		rc = -EADDRNOTAVAIL;
		goto err1;
	}

	gemac_set_laddrN( priv->EMAC_baseaddr, ( MAC_ADDR *)dev->dev_addr, 1 );

	pfe_ctrl_set_eth_state(priv->id, 1, dev->dev_addr);

	if (!(priv->einfo->phy_flags & GEMAC_NO_PHY)) {
		rc = pfe_phy_init(dev);
		if (rc) {
			netdev_err(dev, "%s: pfe_phy_init() failed\n", __func__);
			goto err2;
		}
	}

	napi_enable(&priv->high_napi);
	napi_enable(&priv->low_napi);
	napi_enable(&priv->lro_napi);

	rc = pfe_eth_start( priv );
	netif_tx_wake_all_queues(dev);

	priv->tx_timer.expires = jiffies + ( COMCERTO_TX_RECOVERY_TIMEOUT_MS * HZ )/1000;
	add_timer(&priv->tx_timer);

	return 0;
err2:
	pfe_ctrl_set_eth_state(priv->id, 0, NULL);

err1:
	hif_lib_client_unregister(&priv->client);
	clk_disable(priv->gemtx_clk);

err0:
	return rc;
}

/** pfe_eth_close
 */
static int pfe_eth_close( struct net_device *dev )
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	int i;

	netif_info(priv, ifdown, dev, "%s\n", __func__);

	del_timer_sync(&priv->tx_timer);

	for(i = 0; i < EMAC_TXQ_CNT; i++)
		hrtimer_cancel(&priv->fast_tx_timeout[i].timer);

	netif_tx_stop_all_queues(dev);

	pfe_eth_flush_tx(priv, 1);

	pfe_eth_stop(dev);

	napi_disable(&priv->lro_napi);
	napi_disable(&priv->low_napi);
	napi_disable(&priv->high_napi);

	if (!(priv->einfo->phy_flags & GEMAC_NO_PHY))
		pfe_phy_exit(dev);

	pfe_ctrl_set_eth_state(priv->id, 0, NULL);

	/* Disable gemac tx clock */
	clk_disable(priv->gemtx_clk);

	hif_lib_client_unregister(&priv->client);

	return 0;
}


/** pfe_eth_get_priority
 *
 */
static int pfe_eth_get_queuenum( struct pfe_eth_priv_s *priv, struct sk_buff *skb )
{
	int queuenum;
	unsigned long flags;

	/* Get the FPP queue number from the CONNMARK value */
#if defined(CONFIG_IP_NF_CONNTRACK_MARK) || defined(CONFIG_NF_CONNTRACK_MARK)
	if (skb->nfct) {
		enum ip_conntrack_info cinfo;
		struct nf_conn *ct;
		ct = nf_ct_get(skb, &cinfo);

		if (ct) {
			u_int32_t connmark;
			connmark = ct->mark;

			if ((connmark & 0x80000000) && priv->id != 0)
				connmark >>= 16;

			queuenum = connmark & EMAC_QUEUENUM_MASK;
		}
		else
			queuenum = 0;
	}
	else  /* continued after #endif ... */
#endif
		if (skb->mark & EMAC_QUEUENUM_MASK)
			queuenum = skb->mark & EMAC_QUEUENUM_MASK;
		else{
			spin_lock_irqsave(&priv->lock, flags);	
			queuenum = priv->default_priority & EMAC_QUEUENUM_MASK;
			spin_unlock_irqrestore(&priv->lock, flags);	
		}
	return queuenum;
}

/** pfe_tso_to_desc
 */
static unsigned int pfe_tso_to_desc(struct sk_buff *skb)
{
	struct skb_shared_info *sh = skb_shinfo(skb);
	unsigned int sh_len = skb_transport_offset(skb) + tcp_hdrlen(skb);
	skb_frag_t *f; /* current fragment */
	int len;
	int f_size;  /* size of the current fragment */
	unsigned int s_size; /* segment size */
	unsigned int n_desc = 0;
	int i;

	/* linear part */
	len = skb_headlen(skb) - sh_len;
	s_size = sh->gso_size;

	while (len >= s_size) {
		printk(KERN_INFO "%s: remainig leanier len : %d\n", __func__, len);
		len -= s_size;
		n_desc++;
	}

	if (len) {
		printk(KERN_INFO "%s: remainig leanier len : %d\n", __func__, len);
		n_desc++;
		s_size -= len;
	}

	/* fragment part */
	for (i = 0; i < sh->nr_frags; i++) {
		f = &sh->frags[i];
		f_size = skb_frag_size(f);

		while (f_size >= s_size) {
			f_size -= s_size;
			n_desc++;
			s_size = sh->gso_size;
		}

		if (f_size) {
			n_desc++;
			s_size -= f_size;
		}
	}

	return n_desc;
}

/** pfe_eth_get_req_desc
 *  Compute number of required HIF descriptors and number of packets on wire
 *  n_desc : number of HIF descriptors
 *  n_segs : number of segments on wire
 */
static void pfe_eth_get_req_desc(struct sk_buff *skb, unsigned int *n_desc, unsigned int *n_segs)
{
	struct skb_shared_info *sh = skb_shinfo(skb);
	/**
	* TSO case
	* Number of descriptors required = payload scatter +
	*                                  sh->gso_segs (pre segment header) +
	*                                  sh->gso_segs (hif/tso segment header)
	* sh->gso_segs : number of packets on wire
	*/
	if (skb_is_gso(skb)) {
		*n_desc = pfe_tso_to_desc(skb) + 2 * sh->gso_segs;
		*n_segs = sh->gso_segs;
	}
	// Scattered data
	else if (sh->nr_frags) {
		*n_desc = sh->nr_frags + 1;
		*n_segs = 1;
	}
	// Regular case
	else {
		*n_desc = 1;
		*n_segs = 1;
	}
	return;
}

/** pfe_eth_might_stop_tx
 *
 */
static int pfe_eth_might_stop_tx(struct pfe_eth_priv_s *priv, int queuenum, struct netdev_queue *tx_queue, unsigned int n_desc, unsigned int n_segs)
{
	int tried = 0;
	ktime_t kt;

try_again:
	if (unlikely((__hif_tx_avail(&pfe->hif) < n_desc)
	|| (hif_lib_tx_avail(&priv->client, queuenum) < n_desc)
	|| (hif_lib_tx_credit_avail(pfe, priv->id, queuenum) < n_segs))) {

		if (!tried) {
			hif_tx_unlock(&pfe->hif);
			pfe_eth_flush_txQ(priv, queuenum, 1, n_desc);
			hif_lib_update_credit(&priv->client, queuenum);
			tried = 1;
			hif_tx_lock(&pfe->hif);
			goto try_again;
		}
#ifdef PFE_ETH_TX_STATS
		if (__hif_tx_avail(&pfe->hif) < n_desc)
			priv->stop_queue_hif[queuenum]++;
		else if (hif_lib_tx_avail(&priv->client, queuenum) < n_desc) {
			priv->stop_queue_hif_client[queuenum]++;
		}
		else if (hif_lib_tx_credit_avail(pfe, priv->id, queuenum) < n_segs) {
			priv->stop_queue_credit[queuenum]++;
		}
		priv->stop_queue_total[queuenum]++;
#endif
		netif_tx_stop_queue(tx_queue);

		kt = ktime_set(0, COMCERTO_TX_FAST_RECOVERY_TIMEOUT_MS * NSEC_PER_MSEC);
		hrtimer_start(&priv->fast_tx_timeout[queuenum].timer, kt, HRTIMER_MODE_REL);
		return -1;
	}
	else {
		return 0;
	}
}

#define inc_txq_idx(idxname) idxname = (idxname+1) & (EMAC_TXQ_DEPTH-1)
/** pfe_tso
 */
static void pfe_tso(struct sk_buff *skb, struct pfe_eth_priv_s *priv, int queuenum)
{
	struct skb_shared_info *sh = skb_shinfo(skb);
	skb_frag_t *f;
	struct tcphdr *th;
	unsigned int tcp_off = skb_transport_offset(skb);
	unsigned int ip_off = skb_network_offset(skb);
	unsigned int sh_len = tcp_off + tcp_hdrlen(skb); /* segment header length (link + network + transport) */
	unsigned int data_len = skb->len - sh_len;
	unsigned int f_id;	/* id of the current fragment */
	unsigned int f_len;	/* size of the current fragment */
	unsigned int s_len;	/* size of the current segment */
	unsigned int ip_tcp_hdr_len;
	unsigned int l_len;	/* skb linear size */
	unsigned int len;
	int segment;
	unsigned int off, f_off;
	u32 ctrl;
	unsigned int id;
	unsigned int seq;
#ifdef ETH_HIF_NODMA_MAP
	int wrIndx;
	struct hif_frag_info_s *f_info;
	struct hif_frag_dma_map_s *f_dma, *q_dma_base, *f_dma_first;
#endif
	unsigned int tx_bytes = 0;

#ifdef PFE_ETH_TSO_STATS
	priv->tso_len_counters[((u32)skb->len >> 11) & 0x1f]++;
#endif

	// ctrl = (skb->ip_summed == CHECKSUM_PARTIAL) ? HIF_CTRL_TX_CHECKSUM : 0;
	ctrl = HIF_CTRL_TX_CHECKSUM;

	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *ih = ip_hdr(skb);

		id = ntohs(ih->id);

		ctrl |= HIF_CTRL_TX_TSO;

	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		id = 0;
		ctrl |= HIF_CTRL_TX_TSO6;
	}

	th = tcp_hdr(skb);
	ip_tcp_hdr_len = sh_len - ip_off;

	seq = ntohl(th->seq);

	/* linear part */
	l_len = skb_headlen(skb) - sh_len;
	off = sh_len;

	/* fragment part */
	f_id = 0;
	f_len = 0;

#ifdef ETH_HIF_NODMA_MAP
	f_info = (struct hif_frag_info_s *)skb->cb;

	/* In case of TSO segmentation, we might need to transmit single skb->fragment
	 * in multiple segments/scatters. So hif will perform cache flush, for each scatter
         * of same fragment separatly. Since cahche flush is expensive operatrion we will 
         * perform dma mapping for full fragment here and provide physical addresses to HIF.
	 *
	 * Preserve dmamapping information corresponding to skb fragments in array, this 
         * information is used while freeing SKB.
	 */
	wrIndx = hif_lib_get_tx_wrIndex(&priv->client, queuenum);
	q_dma_base = (struct hif_frag_dma_map_s *)&priv->dma_map_array[queuenum][0];
	f_dma = f_info->map = q_dma_base + wrIndx;
	f_dma_first = f_dma;
	f_dma->data = dma_map_single(pfe->hif.dev, skb->data, skb_headlen(skb), DMA_TO_DEVICE);
	f_dma->len = skb_headlen(skb);
	f_info->frag_count = 1;
	//printk("data : %x, len :%d base : %p\n",f_dma->data, f_dma->len, f_info->map);
#endif


	netif_info(priv, tx_queued, priv->dev,"%s: orig_seq: %x, orig_id: %x, seg_size: %d, seg_n: %d, data_len: %d, hdr_len: %d skb_fras: %d\n", __func__, seq, id, s_len, sh->gso_segs, data_len, sh_len, sh->nr_frags);

	for (segment = 0; segment < sh->gso_segs; segment++) {
		unsigned int hif_flags = 0;

		if (segment == (sh->gso_segs - 1)) {
			ctrl |= HIF_CTRL_TX_TSO_END;
		}

		/* The last segment may be less than gso_size. */
		if (data_len < sh->gso_size)
			s_len = data_len;
		else
			s_len = sh->gso_size;

		data_len -= s_len;

		netif_info(priv, tx_queued, priv->dev, "%s: seq: %x, id: %x, ctrl: %x\n", __func__, seq, id, ctrl);

		/* hif/tso header */
		__hif_lib_xmit_tso_hdr(&priv->client, queuenum, ctrl, ip_off, id, ip_tcp_hdr_len + s_len, tcp_off, seq);

		id++;
		seq += s_len;

		/* ethernet/ip/tcp header */
#ifdef ETH_HIF_NODMA_MAP
		__hif_lib_xmit_pkt(&priv->client, queuenum, (void *)f_dma_first->data, sh_len, 0, HIF_DONT_DMA_MAP, skb);
#else
		__hif_lib_xmit_pkt(&priv->client, queuenum, skb->data, sh_len, 0, 0, skb);
#endif
		tx_bytes += sh_len;

		while (l_len && s_len)
		{
			len = s_len;
			if (len > l_len)
				len = l_len;

			l_len -= len;
			s_len -= len;

			if (!s_len)
				hif_flags = HIF_LAST_BUFFER;

			printk(KERN_INFO "%s: Extra lienear data addr: %p, len: %d, flags: %x\n", __func__,
						skb->data + off, len, hif_flags);

#ifdef ETH_HIF_NODMA_MAP
			__hif_lib_xmit_pkt(&priv->client, queuenum,  (void *)((u32)f_dma->data + off),
									len, 0, hif_flags | HIF_DONT_DMA_MAP, skb);
#else
			__hif_lib_xmit_pkt(&priv->client, queuenum, skb->data + off, len, 0, hif_flags, skb);
#endif
			off += len;
			tx_bytes += len;
		}

		while (s_len) {
#ifdef ETH_HIF_NODMA_MAP
			hif_flags = HIF_DONT_DMA_MAP;
#else
			hif_flags = 0;
#endif

			/* Advance as needed. */
			if (!f_len) {
				f = &sh->frags[f_id];
				f_len = skb_frag_size(f);
				f_off = 0;
				f_id++;
#ifdef ETH_HIF_NODMA_MAP
				/* In case of TSO segmentation, we might need to transmit single skb->fragment
				 * in multiple segments/scatters. So hif will perform cache flush, for each scatter
			         * of same fragment separatly. Since cahche flush is expensive operatrion we will
			         * perform dma mapping for full fragment here and provide physical addresses to HIF.
				 *
				 * Preserve dmamapping information corresponding to skb fragments in array, this
			         * information is used while freeing SKB.
				 */
			        inc_txq_idx(wrIndx);
				f_dma = q_dma_base + wrIndx;
				f_dma->data = dma_map_single(pfe->hif.dev, skb_frag_address(f), f_len, DMA_TO_DEVICE);
				f_dma->len =  f_len;
				f_info->frag_count++;
				//printk("data : %x, len :%d\n",f_dma->data, f_dma->len);
#endif
				netif_info(priv, tx_queued, priv->dev, "%s: frag addr: %p, len: %d\n", __func__, f, f_len);
			}

			/* Use bytes from the current fragment. */
			len = s_len;
			if (len > f_len)
				len = f_len;

			f_len -= len;
			s_len -= len;

			if (!s_len) {
				hif_flags |= HIF_LAST_BUFFER;

				if (segment == (sh->gso_segs - 1))
					hif_flags |= HIF_DATA_VALID;
			}

			netif_info(priv, tx_queued, priv->dev, "%s: scatter addr: %p, len: %d, flags: %x\n", __func__,
					skb_frag_address(f) + f_off, len, hif_flags);

#ifdef ETH_HIF_NODMA_MAP
			__hif_lib_xmit_pkt(&priv->client, queuenum, (void *)((u32)f_dma->data + f_off), len, 0, hif_flags, skb);
#else
			__hif_lib_xmit_pkt(&priv->client, queuenum, skb_frag_address(f) + f_off, len, 0, hif_flags, skb);
#endif
			f_off += len;
			tx_bytes += len;
		}
	}
	hif_lib_tx_credit_use(pfe, priv->id, queuenum, sh->gso_segs);
	priv->stats.tx_packets += sh->gso_segs;
	priv->stats.tx_bytes += tx_bytes;
	hif_tx_dma_start();
}


#define SA_MAX_OP 2
/** pfe_hif_send_packet
 *
 * At this level if TX fails we drop the packet
 */
static void pfe_hif_send_packet( struct sk_buff *skb, struct  pfe_eth_priv_s *priv, int queuenum)
{
	struct skb_shared_info *sh;
	unsigned int nr_frags;
	u32 ctrl;
#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
	int i;
	u16 sah[SA_MAX_OP] = {0};
	struct sec_path *sp;
	struct hif_ipsec_hdr *hif_ipsec;
#endif

	netif_info(priv, tx_queued, priv->dev, "%s\n", __func__);

	if (skb_is_gso(skb)) {
		pfe_tso(skb, priv, queuenum);
		return;
	}

	ctrl = (skb->ip_summed == CHECKSUM_PARTIAL) ? HIF_CTRL_TX_CHECKSUM : 0;

#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
	/* check if packet sent from Host to PFE needs IPsec processing */
	if (skb->ipsec_offload)
	{
		if (skb->sp)
		{
			sp = secpath_dup(skb->sp);
			
			if(sp)
			{
				for (i = sp->len-1; i >= 0; i--)
				{
					struct xfrm_state *x = sp->xvec[i];
					sah[i] = htons(x->handle);
				}

				ctrl |= HIF_CTRL_TX_IPSEC_OUT;
				
				/* add SA info to the hif header*/
				hif_ipsec = (struct hif_ipsec_hdr *)(skb->data - sizeof(struct hif_ipsec_hdr));
				hif_ipsec->sa_handle[0] = sah[0];
				hif_ipsec->sa_handle[1] = sah[1];
				
				skb->data -= sizeof(struct hif_ipsec_hdr);
				skb->len += sizeof(struct hif_ipsec_hdr);
			}
			else
				printk(KERN_ERR "%s: secure path duplication failed\n", __func__);
		}
		else
			printk(KERN_ERR "%s: secure path data not found\n", __func__);
	}
#endif

	sh = skb_shinfo(skb);
	nr_frags = sh->nr_frags;

	if (nr_frags) {
		skb_frag_t *f;
		int i;

		__hif_lib_xmit_pkt(&priv->client, queuenum, skb->data, skb_headlen(skb), ctrl, HIF_FIRST_BUFFER, skb);

		for (i = 0; i < nr_frags - 1; i++) {
			f = &sh->frags[i];
			__hif_lib_xmit_pkt(&priv->client, queuenum, skb_frag_address(f), skb_frag_size(f), 0x0, 0x0, skb);
		}

		f = &sh->frags[i];

		__hif_lib_xmit_pkt(&priv->client, queuenum, skb_frag_address(f), skb_frag_size(f), 0x0, HIF_LAST_BUFFER|HIF_DATA_VALID, skb);

		netif_info(priv, tx_queued, priv->dev, "%s: pkt sent successfully skb:%p nr_frags:%d len:%d\n", __func__, skb, nr_frags, skb->len);
	}
	else {
		__hif_lib_xmit_pkt(&priv->client, queuenum, skb->data, skb->len, ctrl, HIF_FIRST_BUFFER | HIF_LAST_BUFFER | HIF_DATA_VALID, skb);
		netif_info(priv, tx_queued, priv->dev, "%s: pkt sent successfully skb:%p len:%d\n", __func__, skb, skb->len);
	}
	hif_tx_dma_start();
	priv->stats.tx_packets++;
	priv->stats.tx_bytes += skb->len;
	hif_lib_tx_credit_use(pfe, priv->id, queuenum, 1);
}

/** pfe_eth_flush_txQ
 */
static void pfe_eth_flush_txQ(struct pfe_eth_priv_s *priv, int txQ_num, int from_tx, int n_desc)
{
	struct sk_buff *skb;
	struct netdev_queue *tx_queue = netdev_get_tx_queue(priv->dev, txQ_num);
	int count = max(TX_FREE_MAX_COUNT, n_desc);
	unsigned int flags;

	netif_info(priv, tx_done, priv->dev, "%s\n", __func__);

	if (!from_tx)
		__netif_tx_lock(tx_queue, smp_processor_id());

	/* Clean HIF and client queue */
	while (count && (skb = hif_lib_tx_get_next_complete(&priv->client, txQ_num, &flags, count))) {

		/* FIXME : Invalid data can be skipped in hif_lib itself */
		if (flags & HIF_DATA_VALID) {
#ifdef ETH_HIF_NODMA_MAP
			if (flags & HIF_DONT_DMA_MAP) {
				struct hif_frag_info_s *fi = (struct hif_frag_info_s *)skb->cb;
				int ii;
				//printk("frag count : %d, map :%p\n", fi->frag_count, fi->map);

				for (ii = 0; ii < fi->frag_count; ii++) {
					struct hif_frag_dma_map_s *dma = fi->map + ii;

				//	printk("frag addr : %x len : %d\n",  dma->data, dma->len);

					dma_unmap_single(pfe->hif.dev, dma->data, dma->len, DMA_TO_DEVICE);
				}
			}
#endif
			dev_kfree_skb_any(skb);

		}
		// When called from the timer, flush all descriptors
		if (from_tx)
			count--;
	}

	if (!from_tx)
		__netif_tx_unlock(tx_queue);
}

/** pfe_eth_flush_tx
 */
static void pfe_eth_flush_tx(struct pfe_eth_priv_s *priv, int force)
{
	int ii;

	netif_info(priv, tx_done, priv->dev, "%s\n", __func__);

	for (ii = 0; ii < EMAC_TXQ_CNT; ii++) {
		if (force || (time_after(jiffies, priv->client.tx_q[ii].jiffies_last_packet + (COMCERTO_TX_RECOVERY_TIMEOUT_MS * HZ)/1000))) {
			pfe_eth_flush_txQ(priv, ii, 0, 0); //We will release everything we can based on from_tx param, so the count param can be set to any value
			hif_lib_update_credit(&priv->client, ii);
		}
	}
}


/** pfe_eth_send_packet
 */
static int pfe_eth_send_packet(struct sk_buff *skb, struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	int txQ_num = skb_get_queue_mapping(skb);
	int n_desc, n_segs, count;
	struct netdev_queue *tx_queue = netdev_get_tx_queue(priv->dev, txQ_num);

	netif_info(priv, tx_queued, dev, "%s\n", __func__);

	if ((!skb_is_gso(skb)) && (skb_headroom(skb) < (PFE_PKT_HEADER_SZ + sizeof(unsigned long)))) {

		netif_warn(priv, tx_err, priv->dev, "%s: copying skb\n", __func__);

		if (pskb_expand_head(skb, (PFE_PKT_HEADER_SZ + sizeof(unsigned long)), 0, GFP_ATOMIC)) {
			/* No need to re-transmit, no way to recover*/
			kfree_skb(skb);
			priv->stats.tx_dropped++;
			return NETDEV_TX_OK;
		}
	}

	pfe_eth_get_req_desc(skb, &n_desc, &n_segs);

	hif_tx_lock(&pfe->hif);
	if(unlikely(pfe_eth_might_stop_tx(priv, txQ_num, tx_queue, n_desc, n_segs))) {
#ifdef PFE_ETH_TX_STATS
		if(priv->was_stopped[txQ_num]) {
			priv->clean_fail[txQ_num]++;
			priv->was_stopped[txQ_num] = 0;
		}
#endif
		hif_tx_unlock(&pfe->hif);
		return NETDEV_TX_BUSY;
	}

	pfe_hif_send_packet(skb, priv, txQ_num);

	hif_tx_unlock(&pfe->hif);

	dev->trans_start = jiffies;

	// Recycle buffers if a socket's send buffer becomes half full or if the HIF client queue starts filling up
	if (((count = (hif_lib_tx_pending(&priv->client, txQ_num) - HIF_CL_TX_FLUSH_MARK)) > 0)
		|| (skb->sk && ((sk_wmem_alloc_get(skb->sk) << 1) > skb->sk->sk_sndbuf)))
		pfe_eth_flush_txQ(priv, txQ_num, 1, count);

#ifdef PFE_ETH_TX_STATS
	priv->was_stopped[txQ_num] = 0;
#endif

	return NETDEV_TX_OK;
}

/** pfe_eth_select_queue
 *
 */
static u16 pfe_eth_select_queue( struct net_device *dev, struct sk_buff *skb )
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);

	return pfe_eth_get_queuenum(priv, skb);
}


/** pfe_eth_get_stats
 */
static struct net_device_stats *pfe_eth_get_stats(struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);

	netif_info(priv, drv, dev, "%s\n", __func__);

	return &priv->stats;
}


/** pfe_eth_change_mtu
 */
static int pfe_eth_change_mtu(struct net_device *dev, int new_mtu)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	int oldsize = dev->mtu ;
	int frame_size = new_mtu + ETH_HLEN +4;

	netif_info(priv, drv, dev, "%s\n", __func__);

#if !defined(JUMBO_FRAME_SIZE)
#ifdef CONFIG_COMCERTO_JUMBO_FRAME_SIZE_ENABLE
#     define JUMBO_FRAME_SIZE 3574
#else
#     define JUMBO_FRAME_SIZE 0x2048
#endif
#endif
	if ((frame_size < 64) || (frame_size > JUMBO_FRAME_SIZE)) {
		netif_err(priv, drv, dev, "Invalid MTU setting\n");
		return -EINVAL;
	}
#ifndef CONFIG_COMCERTO_JUMBO_FRAME_SIZE_ENABLE
	if (frame_size > 1536) {
		// Can not support larger packets.
		// Would need to:
		// change ip_network config
		// rx buffer size in dma config
		// rx staggering in buffer pool
		netif_err(priv, drv, dev, "Invalid MTU setting - Jumbo frames not supported\n");

		return -EINVAL;
	}
#endif
	/* Only stop and start the controller if it isn't already
	 * stopped, and we changed something */
	if ((oldsize != new_mtu) && (dev->flags & IFF_UP)){
		netdev_err(dev, "Can not change MTU - fast_path must be disabled and ifconfig down must be issued first\n");

		return -EINVAL;
	}

	dev->mtu = new_mtu;

	/* If the mtu is larger than the max size for standard
	 * ethernet frames (ie, a jumbo frame), setup Jumbo frames */
	if (frame_size > 1536) {
		//FIXME : Jumbo frames feature need revisit.
		netdev_err(dev, "Jumbo frames handling is not implemented\n");
	} else {
		if (frame_size > 1500 + ETH_HLEN+4)
			gemac_enable_1536_rx(priv->EMAC_baseaddr);
		else
			gemac_disable_1536_rx(priv->EMAC_baseaddr);
	}

	return 0;
}

/** pfe_eth_set_mac_address
 */
static int pfe_eth_set_mac_address(struct net_device *dev, void *addr)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	struct sockaddr *sa = addr;

	netif_info(priv, drv, dev, "%s\n", __func__);

	if (!is_valid_ether_addr(sa->sa_data))
		return -EADDRNOTAVAIL;

	memcpy(dev->dev_addr, sa->sa_data, ETH_ALEN);

	gemac_set_laddrN(priv->EMAC_baseaddr, (MAC_ADDR *)dev->dev_addr, 1);

	return 0;

}

/** pfe_eth_enet_addr_byte_mac
 */
int pfe_eth_enet_addr_byte_mac(u8 * enet_byte_addr, MAC_ADDR *enet_addr)
{
	if ((enet_byte_addr == NULL) || (enet_addr == NULL))
	{
		return -1;
	}
	else
	{
		enet_addr->bottom = enet_byte_addr[0] |
			(enet_byte_addr[1] << 8) |
			(enet_byte_addr[2] << 16) |
			(enet_byte_addr[3] << 24);
		enet_addr->top = enet_byte_addr[4] |
			(enet_byte_addr[5] << 8);
		return 0;
	}
}

/** pfe_eth_get_hash
 */
static int pfe_eth_get_hash(u8 * addr)
{
	u8 temp1,temp2,temp3,temp4,temp5,temp6,temp7,temp8;
	temp1 = addr[0] & 0x3F ;
	temp2 = ((addr[0] & 0xC0)  >> 6)| ((addr[1] & 0x0F) << 2);
	temp3 = ((addr[1] & 0xF0) >> 4) | ((addr[2] & 0x03) << 4);
	temp4 = (addr[2] & 0xFC) >> 2;
	temp5 = addr[3] & 0x3F;
	temp6 = ((addr[3] & 0xC0) >> 6) | ((addr[4] & 0x0F) << 2);
	temp7 = ((addr[4] & 0xF0) >>4 ) | ((addr[5] & 0x03) << 4);
	temp8 = ((addr[5] &0xFC) >> 2);
	return (temp1 ^ temp2 ^ temp3 ^ temp4 ^ temp5 ^ temp6 ^ temp7 ^ temp8);
}

/** pfe_eth_set_multi
 */
static void pfe_eth_set_multi(struct net_device *dev)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);
	MAC_ADDR    hash_addr;          /* hash register structure */
	MAC_ADDR    spec_addr;		/* specific mac address register structure */
	int         result;          /* index into hash register to set.. */
	int 	    uc_count = 0;
	struct netdev_hw_addr *ha;

	if (dev->flags & IFF_PROMISC) {
		netif_info(priv, drv, dev, "entering promiscuous mode\n");

		priv->promisc = 1;
		gemac_enable_copy_all(priv->EMAC_baseaddr);
	} else {
		priv->promisc = 0;
		gemac_disable_copy_all(priv->EMAC_baseaddr);
	}

	/* Enable broadcast frame reception if required. */
	if (dev->flags & IFF_BROADCAST) {
		gemac_allow_broadcast(priv->EMAC_baseaddr);
	} else {
		netif_info(priv, drv, dev, "disabling broadcast frame reception\n");

		gemac_no_broadcast(priv->EMAC_baseaddr);
	}

	if (dev->flags & IFF_ALLMULTI) {
		/* Set the hash to rx all multicast frames */
		hash_addr.bottom = 0xFFFFFFFF;
		hash_addr.top = 0xFFFFFFFF;
		gemac_set_hash(priv->EMAC_baseaddr, &hash_addr);
		gemac_enable_multicast(priv->EMAC_baseaddr);
		netdev_for_each_uc_addr(ha, dev) {
			if(uc_count >= MAX_UC_SPEC_ADDR_REG) break;
			pfe_eth_enet_addr_byte_mac(ha->addr, &spec_addr);
			gemac_set_laddrN(priv->EMAC_baseaddr, &spec_addr, uc_count + 2);
			uc_count++;
		}
	} else if ((netdev_mc_count(dev) > 0)  || (netdev_uc_count(dev))) {
		u8 *addr;

		hash_addr.bottom = 0;
		hash_addr.top = 0;

		netdev_for_each_mc_addr(ha, dev) {
			addr = ha->addr;

			netif_info(priv, drv, dev, "adding multicast address %X:%X:%X:%X:%X:%X to gem filter\n",
						addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

			result = pfe_eth_get_hash(addr);

			if (result >= EMAC_HASH_REG_BITS) {
				break;
			} else {
				if (result < 32) {
					hash_addr.bottom |= (1 << result);
				} else {
					hash_addr.top |= (1 << (result - 32));
				}
			}

		}

		uc_count = -1;
		netdev_for_each_uc_addr(ha, dev) {
			addr = ha->addr;

			if(++uc_count < MAX_UC_SPEC_ADDR_REG)  
			{
				netdev_info(dev, "adding unicast address %02x:%02x:%02x:%02x:%02x:%02x to gem filter\n",
						addr[0], addr[1], addr[2],
						addr[3], addr[4], addr[5]);

				pfe_eth_enet_addr_byte_mac(addr, &spec_addr);
				gemac_set_laddrN(priv->EMAC_baseaddr, &spec_addr, uc_count + 2);
			}
			else
			{
				netif_info(priv, drv, dev, "adding unicast address %02x:%02x:%02x:%02x:%02x:%02x to gem hash\n",
							addr[0], addr[1], addr[2],
							addr[3], addr[4], addr[5]);

				result = pfe_eth_get_hash(addr);
				if (result >= EMAC_HASH_REG_BITS) {
					break;
				} else {
					if (result < 32)
						hash_addr.bottom |= (1 << result);
					else
						hash_addr.top |= (1 << (result - 32));
				}


			}
		}

		gemac_set_hash(priv->EMAC_baseaddr, &hash_addr);
		if(netdev_mc_count(dev))
			gemac_enable_multicast(priv->EMAC_baseaddr);
		else
			gemac_disable_multicast(priv->EMAC_baseaddr);		
	}

	if(netdev_uc_count(dev) >= MAX_UC_SPEC_ADDR_REG)
		gemac_enable_unicast(priv->EMAC_baseaddr);
	else
	{
		/* Check if there are any specific address HW registers that need 
		 *  to be flushed 
		 *  */
		for(uc_count = netdev_uc_count(dev); uc_count < MAX_UC_SPEC_ADDR_REG; uc_count++) 
			gemac_clear_laddrN(priv->EMAC_baseaddr, uc_count + 2);

		gemac_disable_unicast(priv->EMAC_baseaddr);
	}

	if (dev->flags & IFF_LOOPBACK) {
		gemac_set_loop(priv->EMAC_baseaddr, LB_LOCAL);
	}

	return;
}

/** pfe_eth_set_features
 */
static int pfe_eth_set_features(struct net_device *dev, u32 features)
{
	struct pfe_eth_priv_s *priv = netdev_priv(dev);

	if (dev->features & NETIF_F_RXCSUM)
		gemac_enable_rx_checksum_offload(priv->EMAC_baseaddr);
	else
		gemac_disable_rx_checksum_offload(priv->EMAC_baseaddr);

	if (features & NETIF_F_LRO)
		pfe_ctrl_set_lro(1);
	else
		pfe_ctrl_set_lro(0);

	return 0;
}

/** pfe_eth_tx_timeout
 */
void pfe_eth_tx_timeout(unsigned long data )
{
	struct pfe_eth_priv_s *priv = (struct pfe_eth_priv_s *)data;

	netif_info(priv, timer, priv->dev, "%s\n", __func__);

	pfe_eth_flush_tx(priv, 0);

	priv->tx_timer.expires = jiffies + ( COMCERTO_TX_RECOVERY_TIMEOUT_MS * HZ )/1000;
	add_timer(&priv->tx_timer);
}

/** pfe_eth_fast_tx_timeout
 */
static enum hrtimer_restart pfe_eth_fast_tx_timeout(struct hrtimer *timer)
{
	struct pfe_eth_fast_timer *fast_tx_timeout = container_of(timer, struct pfe_eth_fast_timer, timer);
	struct pfe_eth_priv_s *priv =  container_of(fast_tx_timeout->base, struct pfe_eth_priv_s, fast_tx_timeout);
	struct netdev_queue *tx_queue = netdev_get_tx_queue(priv->dev, fast_tx_timeout->queuenum);

	if(netif_tx_queue_stopped(tx_queue)) {
#ifdef PFE_ETH_TX_STATS
		priv->was_stopped[fast_tx_timeout->queuenum] = 1;
#endif
		netif_tx_wake_queue(tx_queue);
	}

	return HRTIMER_NORESTART;
}

/** pfe_eth_fast_tx_timeout_init
 */
static void pfe_eth_fast_tx_timeout_init(struct pfe_eth_priv_s *priv)
{
	int i;
	for (i = 0; i < EMAC_TXQ_CNT; i++) {
		priv->fast_tx_timeout[i].queuenum = i;
		hrtimer_init(&priv->fast_tx_timeout[i].timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		priv->fast_tx_timeout[i].timer.function = pfe_eth_fast_tx_timeout;
		priv->fast_tx_timeout[i].base = priv->fast_tx_timeout;
	}
}

static struct sk_buff *pfe_eth_rx_skb(struct net_device *dev, struct pfe_eth_priv_s *priv, unsigned int qno)
{
	void *buf_addr;
	unsigned int rx_ctrl;
	unsigned int desc_ctrl = 0;
	struct hif_ipsec_hdr *ipsec_hdr;
	struct sk_buff *skb;
	struct sk_buff *skb_frag, *skb_frag_last = NULL;
	int length = 0, offset;
#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
	struct timespec ktime;
#endif

	skb = priv->skb_inflight[qno];

	if (skb && (skb_frag_last = skb_shinfo(skb)->frag_list)) {
		while (skb_frag_last->next)
			skb_frag_last = skb_frag_last->next;
	}

	while (!(desc_ctrl & CL_DESC_LAST)) {

		buf_addr = hif_lib_receive_pkt(&priv->client, qno, &length, &offset, &rx_ctrl, &desc_ctrl, (void **)&ipsec_hdr);
		if (!buf_addr)
			goto incomplete;

#ifdef PFE_ETH_NAPI_STATS
		priv->napi_counters[NAPI_DESC_COUNT]++;
#endif

		/* First frag */
		if (desc_ctrl & CL_DESC_FIRST) {
#if defined(CONFIG_PLATFORM_EMULATION) || defined(CONFIG_PLATFORM_PCI)
			skb = dev_alloc_skb(PFE_BUF_SIZE);
			if (unlikely(!skb)) {
				goto pkt_drop;
			}

			skb_copy_to_linear_data(skb, buf_addr, length + offset);
			kfree(buf_addr);
#else
			skb = alloc_skb_header(PFE_BUF_SIZE, buf_addr, GFP_ATOMIC);
			if (unlikely(!skb)) {
				goto pkt_drop;
			}
#endif
			skb_reserve(skb, offset);
			skb_put(skb, length);
			skb->dev = dev;

			if ((dev->features & NETIF_F_RXCSUM) && (rx_ctrl & HIF_CTRL_RX_CHECKSUMMED))
				skb->ip_summed = CHECKSUM_UNNECESSARY;
			else
				skb_checksum_none_assert(skb);

#if defined(CONFIG_INET_IPSEC_OFFLOAD) || defined(CONFIG_INET6_IPSEC_OFFLOAD)
			if (rx_ctrl & HIF_CTRL_RX_IPSEC_IN) {
				if (ipsec_hdr) {
					struct sec_path *sp;
					struct xfrm_state *x;
					unsigned short *sah = &ipsec_hdr->sa_handle[0];
					int i = 0;

					sp = secpath_dup(skb->sp);

					if (!sp)
					{
						kfree_skb(skb);
						goto pkt_drop;
					}

					skb->sp = sp;

					/* at maximum 2 SA are expected */
					while (i <= 1)
					{
						if(!*sah)
							break;

						if ((x = xfrm_state_lookup_byhandle(dev_net(dev), ntohs(*sah))) == NULL)
						{
							kfree_skb(skb);
							goto pkt_drop;
						}

						sp->xvec[i] = x;
						
						if (!x->curlft.use_time)
						{
							ktime = current_kernel_time();
							x->curlft.use_time = (unsigned long)ktime.tv_sec;
						}

						i++; sah++;
					}

					sp->len = i;
				}
			}
#endif
		} else {

			/* Next frags */
			if (unlikely(!skb)) {
				printk(KERN_ERR "%s: NULL skb_inflight\n", __func__);
				goto pkt_drop;
			}

			skb_frag = alloc_skb_header(PFE_BUF_SIZE, buf_addr, GFP_ATOMIC);
			if (unlikely(!skb_frag)) {
				kfree(buf_addr);
				goto pkt_drop;
			}

			skb_reserve(skb_frag, offset);
			skb_put(skb_frag, length);

			skb_frag->dev = dev;

			if (skb_shinfo(skb)->frag_list)
				skb_frag_last->next = skb_frag;
			else
				skb_shinfo(skb)->frag_list = skb_frag;

			skb->truesize += skb_frag->truesize;
			skb->data_len += length;
			skb->len += length;
			skb_frag_last = skb_frag;
		}
	}

	priv->skb_inflight[qno] = NULL;
	return skb;

incomplete:
	priv->skb_inflight[qno] = skb;
	return NULL;

pkt_drop:
	priv->skb_inflight[qno] = NULL;

	if (skb) {
		kfree_skb(skb);
	} else {
		kfree(buf_addr);
	}

	priv->stats.rx_errors++;

	return NULL;
}


static struct sk_buff *pfe_eth_rx_page(struct net_device *dev, struct pfe_eth_priv_s *priv, unsigned int qno)
{
	struct page *p;
	void *buf_addr;
	unsigned int rx_ctrl;
	unsigned int desc_ctrl;
	struct sk_buff *skb;
	int length, offset, data_offset;
	struct hif_lro_hdr *lro_hdr;
	unsigned long page_offset;
	u32 pe_id;

	while (1) {
		buf_addr = hif_lib_receive_pkt(&priv->client, qno, &length, &offset, &rx_ctrl, &desc_ctrl, (void **)&lro_hdr);
		if (!buf_addr)
			goto empty;

		if (qno == 2)
			pe_id = (rx_ctrl >> 16) & 0xf;
		else
			pe_id = 0;

		skb = priv->skb_inflight[qno + pe_id];

#ifdef PFE_ETH_NAPI_STATS
		priv->napi_counters[NAPI_DESC_COUNT]++;
#endif

		/* This is always zero for PAGE_RATIO = 1 */
		page_offset = ((unsigned long)buf_addr & ~PAGE_MASK) & ~(PAGE_SIZE / PAGE_RATIO - 1);

		/* First frag */
		if ((desc_ctrl & CL_DESC_FIRST) && !skb) {
			p = virt_to_page(buf_addr);

			skb = dev_alloc_skb(MAX_HDR_SIZE + PFE_PKT_HEADROOM + 2);
			if (unlikely(!skb)) {
				goto pkt_drop;
			}

			skb_reserve(skb, PFE_PKT_HEADROOM + 2);

			if (lro_hdr) {
				data_offset = lro_hdr->data_offset;
				if (lro_hdr->mss)
					skb_shinfo(skb)->gso_size = lro_hdr->mss;

//				printk(KERN_INFO "mss: %d, offset: %d, data_offset: %d, len: %d\n", lro_hdr->mss, offset, lro_hdr->data_offset, length);
			} else {
				data_offset = MAX_HDR_SIZE;
			}

			/* We don't need the fragment if the whole packet */
			/* has been copied in the first linear skb        */
			if (length <= data_offset) {
				__memcpy(skb->data, buf_addr + offset, length);
				skb_put(skb, length);
				free_page((unsigned long)buf_addr);
			} else {
				__memcpy(skb->data, buf_addr + offset, data_offset);
				skb_put(skb, data_offset);

				skb_add_rx_frag(skb, 0, p, page_offset + offset + data_offset, length - data_offset);
			}

			skb->dev = dev;

			if ((dev->features & NETIF_F_RXCSUM) && (rx_ctrl & HIF_CTRL_RX_CHECKSUMMED))
				skb->ip_summed = CHECKSUM_UNNECESSARY;
			else
				skb_checksum_none_assert(skb);

		} else {
			/* Next frags */
			if (unlikely(!skb)) {
				printk(KERN_ERR "%s: NULL skb_inflight\n", __func__);
				goto pkt_drop;
			}

			p = virt_to_page(buf_addr);

			skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, p, page_offset + offset, length);
		}

		/* Last buffer in a software chain */
		if ((desc_ctrl & CL_DESC_LAST) && !(rx_ctrl & HIF_CTRL_RX_CONTINUED))
			break;

		/* Keep track of skb for this queue/pe */
		priv->skb_inflight[qno + pe_id] = skb;
	}

	priv->skb_inflight[qno + pe_id] = NULL;

	return skb;

pkt_drop:
	priv->skb_inflight[qno + pe_id] = NULL;

	if (skb) {
		kfree_skb(skb);
	} else {
		free_page((unsigned long)buf_addr);
	}

	priv->stats.rx_errors++;

	return NULL;

empty:
	return NULL;
}

/** pfe_eth_poll
 */
static int pfe_eth_poll(struct pfe_eth_priv_s *priv, struct napi_struct *napi, unsigned int qno, int budget)
{
	struct net_device *dev = priv->dev;
	struct sk_buff *skb;
	int work_done = 0;
	unsigned int len;

	netif_info(priv, intr, priv->dev, "%s\n", __func__);

#ifdef PFE_ETH_NAPI_STATS
	priv->napi_counters[NAPI_POLL_COUNT]++;
#endif

	do {
		if (page_mode)
			skb = pfe_eth_rx_page(dev, priv, qno);
		else
			skb = pfe_eth_rx_skb(dev, priv, qno);

		if (!skb)
			break;

		len = skb->len;

		/* Packet will be processed */
		skb->protocol = eth_type_trans(skb, dev);

#ifdef PFE_ETH_LRO_STATS
		priv->lro_len_counters[((u32)skb->len >> 11) & (LRO_LEN_COUNT_MAX - 1)]++;
		priv->lro_nb_counters[skb_shinfo(skb)->nr_frags & (LRO_NB_COUNT_MAX - 1)]++;
#endif

		netif_receive_skb(skb);

		priv->stats.rx_packets++;
		priv->stats.rx_bytes += len;

		dev->last_rx = jiffies;

		work_done++;

#ifdef PFE_ETH_NAPI_STATS
		priv->napi_counters[NAPI_PACKET_COUNT]++;
#endif

	} while (work_done < budget);

	/* If no Rx receive nor cleanup work was done, exit polling mode.
	 * No more netif_running(dev) check is required here , as this is checked in
	 * net/core/dev.c ( 2.6.33.5 kernel specific).
	 */
	if (work_done < budget) {
		napi_complete(napi);

		hif_lib_event_handler_start(&priv->client, EVENT_RX_PKT_IND, qno);
	}
#ifdef PFE_ETH_NAPI_STATS
	else
		priv->napi_counters[NAPI_FULL_BUDGET_COUNT]++;
#endif

	return work_done;
}

/** pfe_eth_lro_poll
 */
static int pfe_eth_lro_poll(struct napi_struct *napi, int budget)
{
	struct pfe_eth_priv_s *priv = container_of(napi, struct pfe_eth_priv_s, lro_napi);

	netif_info(priv, intr, priv->dev, "%s\n", __func__);

	return pfe_eth_poll(priv, napi, 2, budget);
}


/** pfe_eth_low_poll
 */
static int pfe_eth_low_poll(struct napi_struct *napi, int budget)
{
	struct pfe_eth_priv_s *priv = container_of(napi, struct pfe_eth_priv_s, low_napi);

	netif_info(priv, intr, priv->dev, "%s\n", __func__);

	return pfe_eth_poll(priv, napi, 1, budget);
}

/** pfe_eth_high_poll
 */
static int pfe_eth_high_poll(struct napi_struct *napi, int budget )
{
	struct pfe_eth_priv_s *priv = container_of(napi, struct pfe_eth_priv_s, high_napi);

	netif_info(priv, intr, priv->dev, "%s\n", __func__);

	return pfe_eth_poll(priv, napi, 0, budget);
}

static const struct net_device_ops pfe_netdev_ops = {
	.ndo_open = pfe_eth_open,
	.ndo_stop = pfe_eth_close,
	.ndo_start_xmit = pfe_eth_send_packet,
	.ndo_select_queue = pfe_eth_select_queue,
	.ndo_get_stats = pfe_eth_get_stats,
	.ndo_change_mtu = pfe_eth_change_mtu,
	.ndo_set_mac_address = pfe_eth_set_mac_address,
	.ndo_set_rx_mode = pfe_eth_set_multi,
	.ndo_set_features = pfe_eth_set_features,
	.ndo_validate_addr = eth_validate_addr,
};


/** pfe_eth_init_one 
 */

static int pfe_eth_init_one( struct pfe *pfe, int id )
{
	struct net_device *dev = NULL;
	struct pfe_eth_priv_s *priv = NULL;
	struct comcerto_eth_platform_data *einfo;
	struct comcerto_mdio_platform_data *minfo;
	struct comcerto_pfe_platform_data *pfe_info;
	int err;

	/* Extract pltform data */
#if defined(CONFIG_PLATFORM_EMULATION) || defined(CONFIG_PLATFORM_PCI)
	pfe_info = (struct comcerto_pfe_platform_data *) &comcerto_pfe_pdata;
#else
	pfe_info = (struct comcerto_pfe_platform_data *) pfe->dev->platform_data;
#endif
	if (!pfe_info) {
		printk(KERN_ERR "%s: pfe missing additional platform data\n", __func__);
		err = -ENODEV;
		goto err0;
	}

	einfo = (struct comcerto_eth_platform_data *) pfe_info->comcerto_eth_pdata;

	/* einfo never be NULL, but no harm in having this check */ 
	if (!einfo) {
		printk(KERN_ERR "%s: pfe missing additional gemacs platform data\n", __func__);
		err = -ENODEV;
		goto err0;
	}

	minfo = (struct comcerto_mdio_platform_data *) pfe_info->comcerto_mdio_pdata;

	/* einfo never be NULL, but no harm in having this check */ 
	if (!minfo) {
		printk(KERN_ERR "%s: pfe missing additional mdios platform data\n", __func__);
		err = -ENODEV;
		goto err0;
	}

	/*
	 * FIXME: Need to check some flag in "einfo" to know whether
	 *        GEMAC is enabled Or not.
	 */

	/* Create an ethernet device instance */
	dev = alloc_etherdev_mq(sizeof (*priv), EMAC_TXQ_CNT);

	if (!dev) {
		printk(KERN_ERR "%s: gemac %d device allocation failed\n", __func__, einfo[id].gem_id);
		err = -ENOMEM;
		goto err0;
	}

	priv = netdev_priv(dev);
	priv->dev = dev;
	priv->id = einfo[id].gem_id;
	priv->pfe = pfe;
	/* get gemac tx clock */
	priv->gemtx_clk = clk_get(NULL, "gemtx");

	if (IS_ERR(priv->gemtx_clk)) {
		printk(KERN_ERR "%s: Unable to get the clock for gemac %d\n", __func__, priv->id);
		err = -ENODEV;
		goto err1; 
	}

	pfe->eth.eth_priv[id] = priv;

	/* Set the info in the priv to the current info */
	priv->einfo = &einfo[id];
	priv->EMAC_baseaddr = cbus_emac_base[id];
	priv->GPI_baseaddr = cbus_gpi_base[id];

	/* FIXME : For now TMU queue numbers hardcoded, later should be taken from pfe.h */	
#define HIF_GEMAC_TMUQ_BASE	6
	priv->low_tmuQ	=  HIF_GEMAC_TMUQ_BASE + (id * 2);	
	priv->high_tmuQ	=  priv->low_tmuQ + 1;	

	spin_lock_init(&priv->lock);
	priv->tx_timer.data = (unsigned long)priv;
	priv->tx_timer.function = pfe_eth_tx_timeout;
	priv->tx_timer.expires = jiffies + ( COMCERTO_TX_RECOVERY_TIMEOUT_MS * HZ )/1000;
	init_timer(&priv->tx_timer);

	pfe_eth_fast_tx_timeout_init(priv);

	/* Copy the station address into the dev structure, */
	memcpy(dev->dev_addr, einfo[id].mac_addr, ETH_ALEN);

	err = dev_alloc_name(dev, einfo[id].name);

	if (err < 0) {
		netdev_err(dev, "%s: dev_alloc_name(%s) failed\n", __func__, einfo[id].name);
		err = -EINVAL;
		goto err2;
	}

	/* Initialize mdio */
	if (minfo[id].enabled) {

		if ((err = pfe_eth_mdio_init(priv, &minfo[id]))) {
			netdev_err(dev, "%s: pfe_eth_mdio_init() failed\n", __func__);
			goto err2;
		}
	}

	dev->mtu = 1500;

	/* supported features */
	dev->hw_features = NETIF_F_RXCSUM | NETIF_F_IP_CSUM |  NETIF_F_IPV6_CSUM |
				NETIF_F_SG | NETIF_F_TSO;

	/* enabled by default */
	dev->features = dev->hw_features;

	if (lro_mode) {
		dev->hw_features |= NETIF_F_LRO;
		dev->features |= NETIF_F_LRO;
		pfe_ctrl_set_lro(1);
	}

	dev->netdev_ops = &pfe_netdev_ops;

	dev->ethtool_ops = &pfe_ethtool_ops;

	/* Enable basic messages by default */
	priv->msg_enable = NETIF_MSG_IFUP | NETIF_MSG_IFDOWN | NETIF_MSG_LINK | NETIF_MSG_PROBE;

	err = register_netdev(dev);

	if (err) {
		netdev_err(dev, "register_netdev() failed\n");
		goto err3;
	}

	netif_napi_add(dev, &priv->low_napi, pfe_eth_low_poll, HIF_RX_POLL_WEIGHT - 16);
	netif_napi_add(dev, &priv->high_napi, pfe_eth_high_poll, HIF_RX_POLL_WEIGHT - 16);
	netif_napi_add(dev, &priv->lro_napi, pfe_eth_lro_poll, HIF_RX_POLL_WEIGHT - 16);

	/* Create all the sysfs files */
	if(pfe_eth_sysfs_init(dev))
		goto err4;

    /* Allocate the rx & tx queue buffers on device init */
    if (alloc_on_init) {
        printk(KERN_INFO "%s: preallocating rx & tx queue buffers \n", __func__);
        priv->tx_buf = kzalloc(EMAC_TXQ_CNT * EMAC_TXQ_DEPTH * sizeof(struct tx_queue_desc), GFP_KERNEL);
        if (!priv->tx_buf) {
            goto err5;
        }

        priv->rx_buf = kzalloc(EMAC_RXQ_CNT * EMAC_RXQ_DEPTH * sizeof(struct rx_queue_desc), GFP_KERNEL);
        if (!priv->rx_buf) {
            kfree(priv->tx_buf);
            goto err5;
        }
    }
    else {
        priv->tx_buf = NULL;
        priv->rx_buf = NULL;
    }

	netif_info(priv, probe, dev, "%s: created interface, baseaddr: %p\n", __func__, priv->EMAC_baseaddr);

	return 0;

err5:
	pfe_eth_sysfs_exit(priv->dev);
err4:
	unregister_netdev(dev);
err3:
	pfe_eth_mdio_exit(priv->mii_bus);
err2:
	clk_put(priv->gemtx_clk);
err1:
	free_netdev(priv->dev);

err0:
	return err;
}

/** pfe_eth_init
 */
int pfe_eth_init(struct pfe *pfe)
{
	int ii = 0;
	int err;

	printk(KERN_INFO "%s\n", __func__);

	cbus_emac_base[0] = EMAC1_BASE_ADDR;
	cbus_emac_base[1] = EMAC2_BASE_ADDR;
	cbus_emac_base[2] = EMAC3_BASE_ADDR;

	cbus_gpi_base[0] = EGPI1_BASE_ADDR;
	cbus_gpi_base[1] = EGPI2_BASE_ADDR;
	cbus_gpi_base[2] = EGPI3_BASE_ADDR;

	for (ii = 0; ii < NUM_GEMAC_SUPPORT; ii++) {
		if ((err = pfe_eth_init_one(pfe, ii)))
			goto err0;
	}

	return 0;

err0:
	while(ii--){
		pfe_eth_exit_one( pfe->eth.eth_priv[ii] );
	} 

	/* Register three network devices in the kernel */
	return err;
}

/** pfe_eth_exit_one
 */
static void pfe_eth_exit_one( struct pfe_eth_priv_s *priv )
{
	netif_info(priv, probe, priv->dev, "%s\n", __func__);

    if (alloc_on_init) {
        kfree(priv->tx_buf);
        kfree(priv->rx_buf);
    }

	pfe_eth_sysfs_exit(priv->dev);

	clk_put(priv->gemtx_clk);

	unregister_netdev(priv->dev);

	pfe_eth_mdio_exit(priv->mii_bus);

	free_netdev(priv->dev);
}

/** pfe_eth_exit
 */
void pfe_eth_exit(struct pfe *pfe)
{
	int ii;

	printk(KERN_INFO "%s\n", __func__);

	for(ii = 0; ii < NUM_GEMAC_SUPPORT; ii++ ) {
		/*
		 * FIXME: Need to check some flag in "einfo" to know whether
		 *        GEMAC is enabled Or not.
		 */

		pfe_eth_exit_one(pfe->eth.eth_priv[ii]);
	}
}


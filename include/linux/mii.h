 
#ifndef __LINUX_MII_H__
#define __LINUX_MII_H__

#include <linux/types.h>

#define MII_BMCR            0x00         
#define MII_BMSR            0x01         
#define MII_PHYSID1         0x02         
#define MII_PHYSID2         0x03         
#define MII_ADVERTISE       0x04         
#define MII_LPA             0x05         
#define MII_EXPANSION       0x06         
#define MII_CTRL1000        0x09         
#define MII_STAT1000        0x0a         
#define MII_ESTATUS	    0x0f	 
#define MII_DCOUNTER        0x12         
#define MII_FCSCOUNTER      0x13         
#define MII_NWAYTEST        0x14         
#define MII_RERRCOUNTER     0x15         
#define MII_SREVISION       0x16         
#define MII_RESV1           0x17         
#define MII_LBRERROR        0x18         
#define MII_PHYADDR         0x19         
#define MII_RESV2           0x1a         
#define MII_TPISTATUS       0x1b         
#define MII_NCONFIG         0x1c         

#define BMCR_RESV               0x003f   
#define BMCR_SPEED1000		0x0040   
#define BMCR_CTST               0x0080   
#define BMCR_FULLDPLX           0x0100   
#define BMCR_ANRESTART          0x0200   
#define BMCR_ISOLATE            0x0400   
#define BMCR_PDOWN              0x0800   
#define BMCR_ANENABLE           0x1000   
#define BMCR_SPEED100           0x2000   
#define BMCR_LOOPBACK           0x4000   
#define BMCR_RESET              0x8000   

#define BMSR_ERCAP              0x0001   
#define BMSR_JCD                0x0002   
#define BMSR_LSTATUS            0x0004   
#define BMSR_ANEGCAPABLE        0x0008   
#define BMSR_RFAULT             0x0010   
#define BMSR_ANEGCOMPLETE       0x0020   
#define BMSR_RESV               0x00c0   
#define BMSR_ESTATEN		0x0100	 
#define BMSR_100HALF2           0x0200   
#define BMSR_100FULL2           0x0400   
#define BMSR_10HALF             0x0800   
#define BMSR_10FULL             0x1000   
#define BMSR_100HALF            0x2000   
#define BMSR_100FULL            0x4000   
#define BMSR_100BASE4           0x8000   

#define ADVERTISE_SLCT          0x001f   
#define ADVERTISE_CSMA          0x0001   
#define ADVERTISE_10HALF        0x0020   
#define ADVERTISE_1000XFULL     0x0020   
#define ADVERTISE_10FULL        0x0040   
#define ADVERTISE_1000XHALF     0x0040   
#define ADVERTISE_100HALF       0x0080   
#define ADVERTISE_1000XPAUSE    0x0080   
#define ADVERTISE_100FULL       0x0100   
#define ADVERTISE_1000XPSE_ASYM 0x0100   
#define ADVERTISE_100BASE4      0x0200   
#define ADVERTISE_PAUSE_CAP     0x0400   
#define ADVERTISE_PAUSE_ASYM    0x0800   
#define ADVERTISE_RESV          0x1000   
#define ADVERTISE_RFAULT        0x2000   
#define ADVERTISE_LPACK         0x4000   
#define ADVERTISE_NPAGE         0x8000   

#define ADVERTISE_FULL (ADVERTISE_100FULL | ADVERTISE_10FULL | \
			ADVERTISE_CSMA)
#define ADVERTISE_ALL (ADVERTISE_10HALF | ADVERTISE_10FULL | \
                       ADVERTISE_100HALF | ADVERTISE_100FULL)

#define LPA_SLCT                0x001f   
#define LPA_10HALF              0x0020   
#define LPA_1000XFULL           0x0020   
#define LPA_10FULL              0x0040   
#define LPA_1000XHALF           0x0040   
#define LPA_100HALF             0x0080   
#define LPA_1000XPAUSE          0x0080   
#define LPA_100FULL             0x0100   
#define LPA_1000XPAUSE_ASYM     0x0100   
#define LPA_100BASE4            0x0200   
#define LPA_PAUSE_CAP           0x0400   
#define LPA_PAUSE_ASYM          0x0800   
#define LPA_RESV                0x1000   
#define LPA_RFAULT              0x2000   
#define LPA_LPACK               0x4000   
#define LPA_NPAGE               0x8000   

#define LPA_DUPLEX		(LPA_10FULL | LPA_100FULL)
#define LPA_100			(LPA_100FULL | LPA_100HALF | LPA_100BASE4)

#define EXPANSION_NWAY          0x0001   
#define EXPANSION_LCWP          0x0002   
#define EXPANSION_ENABLENPAGE   0x0004   
#define EXPANSION_NPCAPABLE     0x0008   
#define EXPANSION_MFAULTS       0x0010   
#define EXPANSION_RESV          0xffe0   

#define ESTATUS_1000_TFULL	0x2000	 
#define ESTATUS_1000_THALF	0x1000	 

#define NWAYTEST_RESV1          0x00ff   
#define NWAYTEST_LOOPBACK       0x0100   
#define NWAYTEST_RESV2          0xfe00   

#define ADVERTISE_1000FULL      0x0200   
#define ADVERTISE_1000HALF      0x0100   

#define LPA_1000LOCALRXOK       0x2000   
#define LPA_1000REMRXOK         0x1000   
#define LPA_1000FULL            0x0800   
#define LPA_1000HALF            0x0400   

#define FLOW_CTRL_TX		0x01
#define FLOW_CTRL_RX		0x02

struct mii_ioctl_data {
	__u16		phy_id;
	__u16		reg_num;
	__u16		val_in;
	__u16		val_out;
};

#ifdef __KERNEL__ 

#include <linux/if.h>

struct ethtool_cmd;

struct mii_if_info {
	int phy_id;
	int advertising;
	int phy_id_mask;
	int reg_num_mask;

	unsigned int full_duplex : 1;	 
	unsigned int force_media : 1;	 
	unsigned int supports_gmii : 1;  
#ifdef CONFIG_SYNO_PLX_PORTING
	unsigned int using_1000 : 1;     
	unsigned int using_100 : 1;      
	unsigned int using_pause : 1;	 
#endif

	struct net_device *dev;
	int (*mdio_read) (struct net_device *dev, int phy_id, int location);
	void (*mdio_write) (struct net_device *dev, int phy_id, int location, int val);
};

extern int mii_link_ok (struct mii_if_info *mii);
extern int mii_nway_restart (struct mii_if_info *mii);
extern int mii_ethtool_gset(struct mii_if_info *mii, struct ethtool_cmd *ecmd);
extern int mii_ethtool_sset(struct mii_if_info *mii, struct ethtool_cmd *ecmd);
extern int mii_check_gmii_support(struct mii_if_info *mii);
extern void mii_check_link (struct mii_if_info *mii);
extern unsigned int mii_check_media (struct mii_if_info *mii,
				     unsigned int ok_to_print,
				     unsigned int init_media);
#ifdef CONFIG_SYNO_PLX_PORTING
extern unsigned int mii_check_media_ex(struct mii_if_info *mii,
                                    unsigned int ok_to_print,
                                    unsigned int init_media,
                                    int *has_speed_changed,
									 int *has_pause_changed,
									 void (*link_state_change_callback)(int link_state, void* arg),
									 void *link_state_change_arg);
#endif
extern int generic_mii_ioctl(struct mii_if_info *mii_if,
                      	     struct mii_ioctl_data *mii_data, int cmd,
			     unsigned int *duplex_changed);

static inline struct mii_ioctl_data *if_mii(struct ifreq *rq)
{
	return (struct mii_ioctl_data *) &rq->ifr_ifru;
}

static inline unsigned int mii_nway_result (unsigned int negotiated)
{
	unsigned int ret;

	if (negotiated & LPA_100FULL)
		ret = LPA_100FULL;
	else if (negotiated & LPA_100BASE4)
		ret = LPA_100BASE4;
	else if (negotiated & LPA_100HALF)
		ret = LPA_100HALF;
	else if (negotiated & LPA_10FULL)
		ret = LPA_10FULL;
	else
		ret = LPA_10HALF;

	return ret;
}

#ifdef CONFIG_SYNO_PLX_PORTING
static inline unsigned int mii_nway_result_1000(unsigned int lpa_1000, unsigned int advertised_1000)
{
	int full_negotiated = (lpa_1000 & LPA_1000FULL) &&
						  (advertised_1000 & ADVERTISE_1000FULL);

	int half_negotiated = (lpa_1000 & LPA_1000HALF) &&
						  (advertised_1000 & ADVERTISE_1000HALF);
	
	if (full_negotiated) {
		return LPA_1000FULL;
	} else if (half_negotiated) {
		return LPA_1000HALF;
	} else {
		return 0;
	}
}
#endif

static inline unsigned int mii_duplex (unsigned int duplex_lock,
				       unsigned int negotiated)
{
	if (duplex_lock)
		return 1;
	if (mii_nway_result(negotiated) & LPA_DUPLEX)
		return 1;
	return 0;
}

static inline u16 mii_advertise_flowctrl(int cap)
{
	u16 adv = 0;

	if (cap & FLOW_CTRL_RX)
		adv = ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;
	if (cap & FLOW_CTRL_TX)
		adv ^= ADVERTISE_PAUSE_ASYM;

	return adv;
}

static inline u8 mii_resolve_flowctrl_fdx(u16 lcladv, u16 rmtadv)
{
	u8 cap = 0;

	if (lcladv & rmtadv & ADVERTISE_PAUSE_CAP) {
		cap = FLOW_CTRL_TX | FLOW_CTRL_RX;
	} else if (lcladv & rmtadv & ADVERTISE_PAUSE_ASYM) {
		if (lcladv & ADVERTISE_PAUSE_CAP)
			cap = FLOW_CTRL_RX;
		else if (rmtadv & ADVERTISE_PAUSE_CAP)
			cap = FLOW_CTRL_TX;
	}

	return cap;
}

#endif  
#endif  

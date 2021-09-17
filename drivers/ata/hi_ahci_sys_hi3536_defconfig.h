/* hisilicon satav200 reg */
#define HI_SATA_PORT_FIFOTH	0x44
#define HI_SATA_PORT_PHYCTL1	0x48
#define HI_SATA_PORT_PHYCTL	0x74

#define HI_SATA_PHY_CTL1	0xA4
#define HI_SATA_PHY_CTL2	0xB0

#define HI_SATA_FIFOTH_VALUE	0x7ED9F24
#define HI_SATA_BIGENDINE       (1 << 3)

#define HI_SATA_PHY_MODE_1_5G	0
#define HI_SATA_PHY_MODE_3G	1
#define HI_SATA_PHY_MODE_6G	2

#define HI_SATA_PHY_1_5G	0x0e180000
#define HI_SATA_PHY_3G		0x0e390000
#define HI_SATA_PHY_6G		0x0e5a0000

#define HI_SATA_PHY_SG_1_5G	0x18
#define HI_SATA_PHY_SG_3G	0x18
#define HI_SATA_PHY_SG_6G	0x18

void hisata_phy_init(void __iomem *mmio, int phy_mode, int n_port);

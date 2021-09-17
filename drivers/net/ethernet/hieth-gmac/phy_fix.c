#include "higmac.h"
#include "phy_fix.h"

static const u32 phy_fix_param[] = {
#include "festa_v200.h"
};

int set_phy_expanded_access_mode(struct phy_device *phy_dev, int access_mode)
{
	int v, ret;

	v = phy_read(phy_dev, MII_MISC_CTL);
	v &= (~0x3);
	v |= (access_mode & 0x3);
	ret = phy_write(phy_dev, MII_MISC_CTL, v);

	return ret;
}

int phy_expanded_read(struct phy_device *phy_dev, u32 reg_addr)
{
	int v, ret;

	v = phy_read(phy_dev, MII_BMCR);
	v |= BMCR_PDOWN;
	phy_write(phy_dev, MII_BMCR, v);

	phy_write(phy_dev, MII_EXPMA, reg_addr);
	ret = phy_read(phy_dev, MII_EXPMD);

	v = phy_read(phy_dev, MII_BMCR);
	v &= (~BMCR_PDOWN);
	phy_write(phy_dev, MII_BMCR, v);

	return ret;
}

int phy_expanded_write(struct phy_device *phy_dev, u32 reg_addr, u16 val)
{
	int v, ret;

	v = phy_read(phy_dev, MII_BMCR);
	v |= BMCR_PDOWN;
	phy_write(phy_dev, MII_BMCR, v);

	phy_write(phy_dev, MII_EXPMA, reg_addr);
	ret = phy_write(phy_dev, MII_EXPMD, val);

	v = phy_read(phy_dev, MII_BMCR);
	v &= (~BMCR_PDOWN);
	phy_write(phy_dev, MII_BMCR, v);

	return ret;
}

int phy_expanded_write_bulk(struct phy_device *phy_dev, const u32 reg_and_val[],
			    int count)
{
	int i, v, ret = 0;
	u32 reg_addr;
	u16 val;

	v = phy_read(phy_dev, MII_BMCR);
	v |= BMCR_PDOWN;
	phy_write(phy_dev, MII_BMCR, v);

	for (i = 0; i < (2 * count); i += 2) {
		reg_addr = reg_and_val[i];
		val = (u16) reg_and_val[i + 1];
		phy_write(phy_dev, MII_EXPMA, reg_addr);
		ret = phy_write(phy_dev, MII_EXPMD, val);
	}

	v = phy_read(phy_dev, MII_BMCR);
	v &= (~BMCR_PDOWN);
	phy_write(phy_dev, MII_BMCR, v);

	return ret;
}

/* fix FEPHY for better eye diagram */
int hisilicon_fephy_fix(struct phy_device *phy_dev)
{
	int count;

	count = sizeof(phy_fix_param) / sizeof(phy_fix_param[0]);
	if (count % 2)
		pr_warn("internal FEPHY fix register count is not right.\n");
	count /= 2;

	phy_expanded_write_bulk(phy_dev, phy_fix_param, count);

	return 0;
}

/* for a better Electromagnetic Compatibility */
int realtek_gephy_fix(struct phy_device *phy_dev)
{
#if 0
	int v;

	pr_info("RealTek phy fix: phy id=0x%x\n", phy_dev->phy_id);

	v = phy_read(phy_dev, 16);      /* PHYCR reg */
	v |= 1 << 4;                    /* clk125 remains at logic low */
	phy_write(phy_dev, 16, v);

	phy_write(phy_dev, 31, 0x0007);	/* set to extension page */
	phy_write(phy_dev, 30, 0x00A0);	/* set to extension page 160 */

	v = phy_read(phy_dev, 26);
	v &= ~(1 << 2);			/* enable RXC SSC */
	phy_write(phy_dev, 26, v);

	phy_write(phy_dev, 31, 0x0);	/* back to page 0 */

#endif

	return 0;
}

/* copy from phy_quirk() in hieth-sf/net.c */
int KSZ8051MNL_phy_fix(struct phy_device *phy_dev)
{
	u32 v;

	v = phy_read(phy_dev, 0x1F);
	v |= (1 << 7);       /* set phy RMII 50MHz clk; */
	phy_write(phy_dev, 0x1F, v);

	v = phy_read(phy_dev, 0x16);
	v |= (1 << 1);       /* set phy RMII override; */
	phy_write(phy_dev, 0x16, v);

	return 0;
}

/* copy from phy_quirk() in hieth-sf/net.c */
int KSZ8081RNB_phy_fix(struct phy_device *phy_dev)
{
	u32 v;

	v = phy_read(phy_dev, 0x1F);
	v |= (1 << 7);       /* set phy RMII 50MHz clk; */
	phy_write(phy_dev, 0x1F, v);

	return 0;
}

void phy_register_fixups(void)
{
	phy_register_fixup_for_uid(REALTEK_PHY_ID_8211EG,
			REALTEK_PHY_MASK, realtek_gephy_fix);
	phy_register_fixup_for_uid(HISILICON_PHY_ID_FESTAV200,
			HISILICON_PHY_MASK, hisilicon_fephy_fix);
	phy_register_fixup_for_uid(PHY_ID_KSZ8051MNL,
			DEFAULT_PHY_MASK, KSZ8051MNL_phy_fix);
	phy_register_fixup_for_uid(PHY_ID_KSZ8081RNB,
			DEFAULT_PHY_MASK, KSZ8081RNB_phy_fix);
}

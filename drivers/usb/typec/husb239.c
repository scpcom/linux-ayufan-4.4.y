// SPDX-License-Identifier: GPL-2.0+
/*
 * Hynetek HUSB239 Type-C DRP Port Controller Driver
 *
 * Copyright (C) 2024 Spacemit Corp.
 */

#include <linux/module.h>
#include <linux/bitfield.h>
#include <linux/i2c.h>
#include <linux/usb/role.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/gpio/consumer.h>
#include <linux/regmap.h>
#include <linux/usb/typec.h>
#include <linux/usb/pd.h>
#include <linux/usb/typec_mux.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include "mux.h"

#define HUSB239_REG_PORTROLE		0x00
#define HUSB239_REG_CONTROL			0x01
#define HUSB239_REG_CONTROL1		0x02
#define HUSB239_REG_MANUAL			0x03
#define HUSB239_REG_RESET			0x04
#define HUSB239_REG_MASK			0x05
#define HUSB239_REG_MASK1			0x06
#define HUSB239_REG_MASK2			0x07
#define HUSB239_REG_INT				0x09
#define HUSB239_REG_INT1			0x0A
#define HUSB239_REG_INT2			0x0B
#define HUSB239_REG_USER_CFG0		0x0C
#define HUSB239_REG_USER_CFG1		0x0D
#define HUSB239_REG_USER_CFG2		0x0E

#define HUSB239_REG_GO_COMMAND		0x18
#define HUSB239_REG_SRC_PDO			0x19

#define HUSB239_REG_STATUS			0x63
#define HUSB239_REG_STATUS1			0x64
#define HUSB239_REG_TYPE			0x65

#define HUSB239_REG_SRC_PDO_5V		0x6A
#define HUSB239_REG_SRC_PDO_9V		0x6B
#define HUSB239_REG_SRC_PDO_12V		0x6C

#define HUSB239_REG_MAX				0x97

#define HUSB239_REG_PORTROLE_ORIENTDEB			BIT(6)
#define HUSB239_REG_PORTROLE_MASK				GENMASK(5, 4)
#define HUSB239_REG_PORTROLE_DRP_DEFAULT		(0x0 << 4)
#define HUSB239_REG_PORTROLE_DRP_TRY_SNK		(0x1 << 4)
#define HUSB239_REG_PORTROLE_DRP_TRY_SRC		(0x2 << 4)
#define HUSB239_REG_PORTROLE_AUDIOACC			BIT(3)
#define HUSB239_REG_PORTROLE_DRP				BIT(2)

#define HUSB239_REG_CONTROL_T_DRP				GENMASK(7, 6)
#define HUSB239_REG_CONTROL_T_DRP_60			(0x0 << 6)
#define HUSB239_REG_CONTROL_T_DRP_70			(0x1 << 6)
#define HUSB239_REG_CONTROL_T_DRP_80			(0x2 << 6)
#define HUSB239_REG_CONTROL_T_DRP_90			(0x3 << 6)
#define HUSB239_REG_CONTROL_DRPTOGGLE			GENMASK(5, 4)
#define HUSB239_REG_CONTROL_DRPTOGGLE_60_40		(0x0 << 4)
#define HUSB239_REG_CONTROL_DRPTOGGLE_50_50		(0x1 << 4)
#define HUSB239_REG_CONTROL_DRPTOGGLE_40_60		(0x2 << 4)
#define HUSB239_REG_CONTROL_DRPTOGGLE_30_70		(0x3 << 4)
#define HUSB239_REG_CONTROL_HOST_CUR			GENMASK(2, 1)
#define HUSB239_REG_CONTROL_HOST_CUR_DEFAULT	(0x1 << 1)
#define HUSB239_REG_CONTROL_HOST_CUR_1_5A		(0x2 << 1)
#define HUSB239_REG_CONTROL_HOST_CUR_3A			(0x3 << 1)
#define HUSB239_REG_CONTROL_INT_MASK			BIT(0)

#define HUSB239_REG_CONTROL1_EN_DPM_HIZ			BIT(5)
#define HUSB239_REG_CONTROL1_VDM_RESPOND		BIT(4)
#define HUSB239_REG_CONTROL1_I2C_ENABLE			BIT(3)
#define HUSB239_REG_CONTROL1_TCCDEB				GENMASK(2, 0)
#define HUSB239_REG_CONTROL1_TCCDEB_150			(0x3 << 0)

#define HUSB239_REG_MASK_ALL					0xFF

#define HUSB239_REG_MASK_FLGIN					BIT(7)
#define HUSB239_REG_MASK_OREINT					BIT(6)
#define HUSB239_REG_MASK_FAULT					BIT(5)
#define HUSB239_REG_MASK_VBUS_CHG				BIT(4)
#define HUSB239_REG_MASK_VBUS_OV				BIT(3)
#define HUSB239_REG_MASK_BC_LVL					BIT(2)
#define HUSB239_REG_MASK_DETACH					BIT(1)
#define HUSB239_REG_MASK_ATTACH					BIT(0)

#define HUSB239_REG_MASK1_TSD					BIT(7)
#define HUSB239_REG_MASK1_VBUS_UV				BIT(6)
#define HUSB239_REG_MASK1_DR_ROLE				BIT(5)
#define HUSB239_REG_MASK1_PR_ROLE				BIT(4)
#define HUSB239_REG_MASK1_SRC_ALERT				BIT(3)
#define HUSB239_REG_MASK1_FRC_FAIL				BIT(2)
#define HUSB239_REG_MASK1_FRC_SUCC				BIT(1)
#define HUSB239_REG_MASK1_VDM_MSG				BIT(0)

#define HUSB239_REG_MASK2_EXIT_EPR				BIT(3)
#define HUSB239_REG_MASK2_GO_FAIL				BIT(2)
#define HUSB239_REG_MASK2_EPR_MODE				BIT(1)
#define HUSB239_REG_MASK2_PD_HV					BIT(0)

#define HUSB239_REG_USER_CFG2_PD_PRIOR			BIT(2)

#define HUSB239_REG_GO_COMMAND_MASK				GENMASK(4, 0)
#define HUSB239_REG_GO_PDO_SELECT				0x1

#define HUSB239_REG_SRC_PDO_SEL_MASK			GENMASK(7, 3)
#define HUSB239_REG_SRC_PDO_SEL_5V				(0x1 << 3)
#define HUSB239_REG_SRC_PDO_SEL_9V				(0x2 << 3)
#define HUSB239_REG_SRC_PDO_SEL_12V				(0x3 << 3)

#define HUSB239_REG_STATUS_AMS_PROCESS			BIT(7)
#define HUSB239_REG_STATUS_PD_EPR_SNK			BIT(6)
#define HUSB239_REG_STATUS_ORIENT_MASK			GENMASK(5, 4)
#define HUSB239_REG_STATUS_CC1_RPRD				(0x1 << 4)
#define HUSB239_REG_STATUS_CC2_RPRD				(0x2 << 4)
#define HUSB239_REG_STATUS_TSD					BIT(3)
#define HUSB239_REG_STATUS_BC_LVL				GENMASK(2, 1)
#define HUSB239_REG_STATUS_ATTACH				BIT(0)

#define HUSB239_REG_STATUS1_FLGIN				BIT(7)
#define HUSB239_REG_STATUS1_POWER_ROLE			BIT(6)
#define HUSB239_REG_STATUS1_PD_HV				BIT(5)
#define HUSB239_REG_STATUS1_PD_COMM				BIT(4)
#define HUSB239_REG_STATUS1_SRC_ALERT			BIT(3)
#define HUSB239_REG_STATUS1_AMS_SUCC			BIT(2)
#define HUSB239_REG_STATUS1_FAULT				BIT(1)
#define HUSB239_REG_STATUS1_DATA_ROLE			BIT(0)

#define HUSB239_REG_TYPE_CC_RX_ACTIVE			BIT(7)
#define HUSB239_REG_TYPE_DEBUGSRC				BIT(6)
#define HUSB239_REG_TYPE_DEBUGSNK				BIT(5)
#define HUSB239_REG_TYPE_SINK					BIT(4)
#define HUSB239_REG_TYPE_SOURCE					BIT(3)
#define HUSB239_REG_TYPE_ACTIVECABLE			BIT(2)
#define HUSB239_REG_TYPE_AUDIOVBUS				BIT(1)
#define HUSB239_REG_TYPE_AUDIO					BIT(0)

#define HUSB239_REG_SRC_DETECT					(0x1 << 7)

#define HUSB239_DATA_ROLE(s)					(!!((s) & BIT(0)))
#define HUSB239_PD_COMM(s)						(!!((s) & BIT(4)))
#define HUSB239_POWER_ROLE(s)					(!!((s) & BIT(6)))

struct typec_info {
	int request_current; /* ma */
	int request_voltage; /* mv */

	int data_role;
	int power_role;

	struct typec_port *port;
	struct typec_partner *partner;
	struct typec_switch_dev *sw;
	struct usb_role_switch *role_sw;

	struct typec_capability cap;
	bool pd_supported;
	bool switch_supported;

	u32 src_pdo[PDO_MAX_OBJECTS];
	u32 sink_pdo[PDO_MAX_OBJECTS];

	u8 src_pdo_nr;
	u8 sink_pdo_nr;
	u8 sink_watt;
	u8 sink_voltage;
};

struct husb239 {
	struct device *dev;
	struct regmap *regmap;
	struct regulator *vbus_supply;
	struct gpio_descs *vbus_gpiod;

	struct gpio_desc *en_gpiod; /* chip enable gpio */
	struct gpio_desc *chg_gpiod;/* stop charge while set vbus */
	struct gpio_desc *aud_gpiod;/* audio switch gpio */

	struct gpio_desc *sel_gpiod;/* sel gpio, for orient switch */
	struct gpio_desc *oe_gpiod; /* oe gpio, for orient switch */

	struct gpio_desc *int_gpiod;
	int gpio_irq;
	struct work_struct work;
	struct workqueue_struct *workqueue;

	struct typec_info info;

	bool vbus_on;
	bool audio_on;
	bool attached;

	u32 voltage;
	u32 req_voltage;
	u32 max_current;
	u32 op_current;

	enum power_supply_usb_type usb_type;
	struct power_supply *psy;
	struct power_supply_desc psy_desc;
};

static void husb239_set_gpios_value(struct gpio_descs *gpios,
						int value)
{
	if (!IS_ERR(gpios)) {
		unsigned long *values;
		int nvalues = gpios->ndescs;

		values = bitmap_alloc(nvalues, GFP_KERNEL);
		if (!values)
			return;

		if (value)
			bitmap_fill(values, nvalues);
		else
			bitmap_zero(values, nvalues);

		gpiod_set_array_value_cansleep(nvalues, gpios->desc,
					       gpios->info, values);

		bitmap_free(values);
	}
}

static enum typec_pwr_opmode husb239_get_pwr_opmode(struct husb239 *husb239)
{
	unsigned int mode;
	int ret;

	ret = regmap_read(husb239->regmap, HUSB239_REG_CONTROL, &mode);
	if (ret) {
		dev_err(husb239->dev, "Unable to get pwr opmode: %d\n", ret);
		return TYPEC_PWR_MODE_USB;
	}

	return (FIELD_GET(HUSB239_REG_CONTROL_HOST_CUR, mode) - 1);
}

static enum typec_accessory husb239_get_accessory(struct husb239 *husb239)
{
	unsigned int type;
	int ret;

	ret = regmap_read(husb239->regmap, HUSB239_REG_TYPE, &type);
	if (ret < 0)
		return ret;

	switch (type) {
	case HUSB239_REG_TYPE_DEBUGSRC:
	case HUSB239_REG_TYPE_DEBUGSNK:
		return TYPEC_ACCESSORY_DEBUG;
	case HUSB239_REG_TYPE_AUDIOVBUS:
	case HUSB239_REG_TYPE_AUDIO:
		return TYPEC_ACCESSORY_AUDIO;
	default:
		return TYPEC_ACCESSORY_NONE;
	}
}

static void husb239_set_data_role(struct husb239 *husb239,
				    enum typec_data_role data_role,
				    bool attached)
{
	struct typec_info *info = &husb239->info;
	enum usb_role usb_role = USB_ROLE_NONE;

	if (attached) {
		if (data_role == TYPEC_HOST)
			usb_role = USB_ROLE_HOST;
		else
			usb_role = USB_ROLE_DEVICE;
	}

	usb_role_switch_set_role(info->role_sw, usb_role);
	typec_set_data_role(info->port, data_role);
}

static int husb239_dr_set(struct typec_port *port, enum typec_data_role role)
{
	struct husb239 *husb239 = typec_get_drvdata(port);

	husb239_set_data_role(husb239, role, true);

	return 0;
}

static const struct typec_operations husb239_ops = {
	.dr_set = husb239_dr_set
};

static int husb239_get_cc_orientation(struct husb239 *husb239)
{
	struct typec_info *info = &husb239->info;
	struct typec_switch_dev *sw_dev = info->sw;
	enum typec_orientation orientation;
	int ret, status;

	if (IS_ERR_OR_NULL(sw_dev))
		return 0;

	ret = regmap_read(husb239->regmap, HUSB239_REG_STATUS, &status);
	if (ret)
		return ret;

	switch (status & HUSB239_REG_STATUS_ORIENT_MASK) {
	case HUSB239_REG_STATUS_CC1_RPRD:
		orientation = TYPEC_ORIENTATION_NORMAL;
		break;
	case HUSB239_REG_STATUS_CC2_RPRD:
		orientation = TYPEC_ORIENTATION_REVERSE;
		break;
	default:
		orientation = TYPEC_ORIENTATION_NONE;
		break;
	}

	if (typec_get_orientation(info->port) != orientation) {
		dev_info(husb239->dev, "set orientation %d\n", orientation);
		typec_set_orientation(info->port, orientation);
		sw_dev->set(sw_dev, orientation);
	}

	return 0;
}

static int husb239_register_partner(struct husb239 *husb239,
				    int pd, int accessory)
{
	struct typec_info *info = &husb239->info;
	struct typec_partner_desc desc;
	struct typec_partner *partner;

	if (info->partner)
		return 0;

	desc.usb_pd = pd;
	desc.accessory = accessory;
	desc.identity = NULL;
	partner = typec_register_partner(info->port, &desc);
	if (IS_ERR(partner))
		return PTR_ERR(partner);

	info->partner = partner;

	return 0;
}

static int husb239_usbpd_request_voltage(struct husb239 *husb239)
{
	unsigned int src_pdo;
	int ret;
	int count = 10;

	while(--count) {
		ret = regmap_read(husb239->regmap, HUSB239_REG_SRC_PDO_9V, &src_pdo);
		if (ret)
			return ret;

		dev_dbg(husb239->dev, "husb239_attach src_pdo: %x\n", src_pdo);
		if (src_pdo & HUSB239_REG_SRC_DETECT)
			break;

		msleep(100);
	}

	if (count == 0)
		return -EINVAL;

	dev_info(husb239->dev, "pd detect \n");
	ret = regmap_update_bits(husb239->regmap, HUSB239_REG_SRC_PDO,
				HUSB239_REG_SRC_PDO_SEL_MASK, HUSB239_REG_SRC_PDO_SEL_9V);
	if (ret)
		return ret;

	msleep(100);

	ret = regmap_write_bits(husb239->regmap, HUSB239_REG_GO_COMMAND,
				HUSB239_REG_GO_COMMAND_MASK, HUSB239_REG_GO_PDO_SELECT);
	if (ret)
		return ret;

	return ret;
}

static int husb239_attach(struct husb239 *husb239)
{
	struct typec_info *info = &husb239->info;
	int ret, status, status1;

	if (regmap_read(husb239->regmap, HUSB239_REG_STATUS, &status) ||
		regmap_read(husb239->regmap, HUSB239_REG_STATUS1, &status1))
		return -EINVAL;

	dev_info(husb239->dev, "husb239_attach status: %x status1: %x\n", status, status1);

	ret = husb239_get_cc_orientation(husb239);
	if (ret)
		return ret;

	if (husb239->aud_gpiod &&
			(husb239_get_accessory(husb239) == TYPEC_ACCESSORY_AUDIO)) {
		/* sel = 0 audp/audn, sel = 1 hdp/hdn */
		gpiod_set_value(husb239->aud_gpiod, 1);
		husb239->audio_on = true;
		dev_info(husb239->dev, "audo accessory attach\n");
		ret = husb239_register_partner(husb239, 0, TYPEC_ACCESSORY_AUDIO);
		if (ret) {
			dev_info(husb239->dev, "register partner failed: %d\n", ret);
			goto vbus_disable;
		}
		return ret;
	}

	if (HUSB239_POWER_ROLE(status1) == TYPEC_SOURCE) {
		if (husb239->vbus_supply) {
			ret = regulator_enable(husb239->vbus_supply);
			if (ret) {
				dev_err(husb239->dev,
					"Failed to enable vbus supply: %d\n", ret);
				return ret;
			}
		}
		gpiod_set_value(husb239->chg_gpiod, 1);
		husb239_set_gpios_value(husb239->vbus_gpiod, 1);
		husb239->vbus_on = true;
		dev_info(husb239->dev, "enable vbus supply\n");
	} else {
		if (!husb239_usbpd_request_voltage(husb239)) {
			husb239->voltage     = 9000;
			husb239->op_current  = 2000;
			husb239->max_current = 2000;
		} else {
			husb239->voltage     = 5000;
			husb239->op_current  = 500;
			husb239->max_current = 500;
		}
		power_supply_changed(husb239->psy);
	}

	typec_set_pwr_role(info->port, HUSB239_POWER_ROLE(status1));
	typec_set_pwr_opmode(info->port, husb239_get_pwr_opmode(husb239));
	husb239_set_data_role(husb239, HUSB239_DATA_ROLE(status1), true);

	husb239->attached = true;
	return 0;

vbus_disable:
	if (husb239->audio_on) {
		gpiod_set_value(husb239->aud_gpiod, 0);
		husb239->audio_on = false;
	}
	if (husb239->vbus_on) {
		husb239_set_gpios_value(husb239->vbus_gpiod, 0);
		gpiod_set_value(husb239->chg_gpiod, 0);
		if (husb239->vbus_supply)
			regulator_disable(husb239->vbus_supply);
		husb239->vbus_on = false;
		dev_info(husb239->dev, "disable vbus supply\n");
	}
	return ret;
}

static void husb239_detach(struct husb239 *husb239)
{
	struct typec_info *info = &husb239->info;
	int status, status1;

	if (regmap_read(husb239->regmap, HUSB239_REG_STATUS, &status) ||
		regmap_read(husb239->regmap, HUSB239_REG_STATUS1, &status1))
		return;

	dev_dbg(husb239->dev, "husb239_detach status: %x status1: %x\n", status, status1);

	typec_unregister_partner(info->partner);
	info->partner = NULL;

	typec_set_pwr_role(info->port, HUSB239_POWER_ROLE(status1));
	typec_set_pwr_opmode(info->port, TYPEC_PWR_MODE_USB);
	husb239_set_data_role(husb239, HUSB239_DATA_ROLE(status1), false);

	if (husb239->audio_on) {
		gpiod_set_value(husb239->aud_gpiod, 0);
		husb239->audio_on = false;
		dev_info(husb239->dev, "audo accessory detach\n");
	}

	if (husb239->vbus_on) {
		husb239_set_gpios_value(husb239->vbus_gpiod, 0);
		gpiod_set_value(husb239->chg_gpiod, 0);
		if (husb239->vbus_supply)
			regulator_disable(husb239->vbus_supply);
		husb239->vbus_on = false;
		dev_info(husb239->dev, "disable vbus supply\n");
	}

	husb239->attached = false;
	husb239->voltage  = 0;
	husb239->max_current = husb239->op_current = 0;
	power_supply_changed(husb239->psy);
}

static void husb239_pd_contract(struct husb239 *husb239)
{
	int status, status1;

	if (regmap_read(husb239->regmap, HUSB239_REG_STATUS, &status) ||
		regmap_read(husb239->regmap, HUSB239_REG_STATUS1, &status1))
		return;
	dev_info(husb239->dev, "husb239_pd_contract status: %x status1: %x\n", status, status1);
}

static void husb239_get_gpio_irq(struct husb239 *husb239)
{
	husb239->int_gpiod = devm_gpiod_get(husb239->dev, "int", GPIOD_IN);
	if (IS_ERR_OR_NULL(husb239->int_gpiod)) {
		dev_err(husb239->dev, "no interrupt gpio property\n");
		return;
	}

	husb239->gpio_irq = gpiod_to_irq(husb239->int_gpiod);
	if (husb239->gpio_irq < 0)
		dev_err(husb239->dev, "failed to get GPIO IRQ\n");
}

static int husb23_usb_set_orientation(struct typec_switch_dev *sw,
				       enum typec_orientation orientation)
{
	struct husb239 *husb239 = typec_switch_get_drvdata(sw);

	if (orientation == TYPEC_ORIENTATION_REVERSE)
		gpiod_set_value(husb239->sel_gpiod, 1);
	else
		gpiod_set_value(husb239->sel_gpiod, 0);

	return 0;
}

static int husb239_chip_init(struct husb239 *husb239)
{
	int ret;

	husb239->en_gpiod = devm_gpiod_get_optional(husb239->dev, "en", GPIOD_OUT_LOW);
	if (IS_ERR(husb239->en_gpiod)) {
		return PTR_ERR(husb239->en_gpiod);
	}
	gpiod_set_value(husb239->en_gpiod, 0);
	msleep(10);

	/* PORTROLE init */
	ret = regmap_write(husb239->regmap, HUSB239_REG_PORTROLE,
				HUSB239_REG_PORTROLE_ORIENTDEB |
				HUSB239_REG_PORTROLE_AUDIOACC |
				HUSB239_REG_PORTROLE_DRP);
	if (ret)
		return ret;

	ret = regmap_write(husb239->regmap, HUSB239_REG_CONTROL,
				HUSB239_REG_CONTROL_T_DRP_70 |
				HUSB239_REG_CONTROL_DRPTOGGLE_60_40 |
				HUSB239_REG_CONTROL_HOST_CUR_3A);
	if (ret)
		return ret;

	ret = regmap_write(husb239->regmap, HUSB239_REG_CONTROL1,
				HUSB239_REG_CONTROL1_VDM_RESPOND |
				HUSB239_REG_CONTROL1_I2C_ENABLE |
				HUSB239_REG_CONTROL1_TCCDEB_150);
	if (ret)
		return ret;

	/* PD has high Priority */
	ret = regmap_update_bits(husb239->regmap, HUSB239_REG_USER_CFG2,
				HUSB239_REG_USER_CFG2_PD_PRIOR, HUSB239_REG_USER_CFG2_PD_PRIOR);
	if (ret)
		return ret;

	/* Mask all interruption */
	ret = (regmap_update_bits(husb239->regmap, HUSB239_REG_MASK,
				HUSB239_REG_MASK_ALL, HUSB239_REG_MASK_ALL) ||
			regmap_update_bits(husb239->regmap, HUSB239_REG_MASK1,
				HUSB239_REG_MASK_ALL, HUSB239_REG_MASK_ALL) ||
			regmap_update_bits(husb239->regmap, HUSB239_REG_MASK2,
				HUSB239_REG_MASK_ALL, HUSB239_REG_MASK_ALL));
	if (ret)
		return ret;

	/* Clear all interruption */
	ret = (regmap_update_bits(husb239->regmap, HUSB239_REG_INT,
				HUSB239_REG_MASK_ALL, HUSB239_REG_MASK_ALL) ||
			regmap_update_bits(husb239->regmap, HUSB239_REG_INT1,
				HUSB239_REG_MASK_ALL, HUSB239_REG_MASK_ALL) ||
			regmap_update_bits(husb239->regmap, HUSB239_REG_INT2,
				HUSB239_REG_MASK_ALL, HUSB239_REG_MASK_ALL));
	if (ret)
		return ret;

	return 0;
}

static void husb239_work_func(struct work_struct *work)
{
	struct husb239 *husb239 = container_of(work, struct husb239, work);
	int ret, int0, int1;

	if (regmap_read(husb239->regmap, HUSB239_REG_INT, &int0) ||
		regmap_read(husb239->regmap, HUSB239_REG_INT1, &int1))
		goto err;

	dev_dbg(husb239->dev, "int0: 0x%x int1: 0x%x\n", int0, int1);
	regmap_write(husb239->regmap, HUSB239_REG_INT, int0);
	regmap_write(husb239->regmap, HUSB239_REG_INT1, int1);

	if (int1 & HUSB239_REG_MASK_ATTACH) {
		ret = husb239_attach(husb239);
		if (ret) {
			dev_err(husb239->dev, "husb239_attach error, ret: %x\n", ret);
			goto err;
		}
	}

	if (int1 & HUSB239_REG_MASK_DETACH) {
		husb239_detach(husb239);
	}

	if (int0 & HUSB239_REG_MASK2_PD_HV) {
		husb239_pd_contract(husb239);
	}

err:
	enable_irq(husb239->gpio_irq);
	return;
}

static irqreturn_t husb239_irq_handler(int irq, void *data)
{
	struct husb239 *husb239 = (struct husb239 *)data;

	disable_irq_nosync(husb239->gpio_irq);
	queue_work(husb239->workqueue, &husb239->work);

	return IRQ_HANDLED;
}

static int husb239_irq_init(struct husb239 *husb239)
{
	int ret;

	INIT_WORK(&husb239->work, husb239_work_func);
	husb239->workqueue = alloc_workqueue("husb239_work",
					  WQ_FREEZABLE |
					  WQ_MEM_RECLAIM,
					  1);
	if (!husb239->workqueue) {
		dev_err(husb239->dev, "fail to create work queue\n");
		ret = -ENOMEM;
		return ret;
	}

	ret = devm_request_threaded_irq(husb239->dev, husb239->gpio_irq, NULL,
				husb239_irq_handler,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				"husb239", husb239);
	if (ret)
		goto free_wq;

	/* Unmask ATTACH and DETACH interruption */
	ret = regmap_write_bits(husb239->regmap, HUSB239_REG_MASK,
				HUSB239_REG_MASK_DETACH |
				HUSB239_REG_MASK_ATTACH, 0);
	if (ret)
		goto free_wq;

	/* Unmask PD_HV interruption */
	ret = regmap_write_bits(husb239->regmap, HUSB239_REG_MASK2,
				HUSB239_REG_MASK2_PD_HV, 0);
	if (ret)
		goto free_wq;

	return 0;

free_wq:
	destroy_workqueue(husb239->workqueue);
	return ret;
}

static int husb239_typec_port_probe(struct husb239 *husb239)
{
	struct typec_info *info = &husb239->info;
	struct typec_capability *cap = &info->cap;
	struct fwnode_handle *connector, *ep;
	struct device *dev = husb239->dev;
	int ret, i;

	husb239->vbus_supply = devm_regulator_get_optional(dev, "vbus");
	if (IS_ERR(husb239->vbus_supply)) {
		ret = PTR_ERR(husb239->vbus_supply);
		if (ret != -ENODEV)
			return ret;
		husb239->vbus_supply = NULL;
	}

	husb239->vbus_gpiod = devm_gpiod_get_array_optional(dev, "vbus", GPIOD_OUT_LOW);
	if (IS_ERR(husb239->vbus_gpiod)) {
		return PTR_ERR(husb239->vbus_gpiod);
	}

	husb239->chg_gpiod = devm_gpiod_get_optional(dev, "chg", GPIOD_OUT_LOW);
	if (IS_ERR(husb239->chg_gpiod)) {
		return PTR_ERR(husb239->chg_gpiod);
	}

	husb239->aud_gpiod = devm_gpiod_get_optional(dev, "aud", GPIOD_OUT_LOW);
	if (IS_ERR(husb239->aud_gpiod)) {
		return PTR_ERR(husb239->aud_gpiod);
	}

	connector = device_get_named_child_node(dev, "connector");
	if (connector) {
		info->role_sw = fwnode_usb_role_switch_get(connector);
	} else {
		ep = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
		if (!ep)
			return -ENODEV;
		connector = fwnode_graph_get_remote_port_parent(ep);
		fwnode_handle_put(ep);
		if (!connector)
			return -ENODEV;
		info->role_sw = fwnode_usb_role_switch_get(connector);
	}

	if (IS_ERR(info->role_sw)) {
		dev_err(dev, "fail to get role sw\n");
		ret = PTR_ERR(info->role_sw);
		goto err_put_fwnode;
	}

	ret = typec_get_fw_cap(cap, connector);
	if (ret)
		goto err_put_role;

	info->pd_supported = !fwnode_property_read_bool(connector, "pd-disable");

	/* Get source pdos */
	ret = fwnode_property_count_u32(connector, "source-pdos");
	if (ret > 0) {
		info->src_pdo_nr = min_t(u8, ret, PDO_MAX_OBJECTS);
		ret = fwnode_property_read_u32_array(connector, "source-pdos",
						     info->src_pdo,
						     info->src_pdo_nr);
		if (ret < 0) {
			dev_err(dev, "source cap validate failed: %d\n", ret);
			goto err_put_role;
		}
	}

	ret = fwnode_property_count_u32(connector, "sink-pdos");
	if (ret > 0) {
		info->sink_pdo_nr = min_t(u8, ret, PDO_MAX_OBJECTS);
		ret = fwnode_property_read_u32_array(connector, "sink-pdos",
						     info->sink_pdo,
						     info->sink_pdo_nr);
		if (ret < 0) {
			dev_err(dev, "sink cap validate failed: %d\n", ret);
			goto err_put_role;
		}

		for (i = 0; i < info->sink_pdo_nr; i++) {
			ret = 0;
			switch (pdo_type(info->sink_pdo[i])) {
			case PDO_TYPE_FIXED:
				ret = pdo_fixed_voltage(info->sink_pdo[i]);
				break;
			case PDO_TYPE_BATT:
			case PDO_TYPE_VAR:
				ret = pdo_max_voltage(info->sink_pdo[i]);
				break;
			case PDO_TYPE_APDO:
			default:
				ret = 0;
				break;
			}

			/* 100mv per unit */
			info->sink_voltage = max(5000, ret) / 100;
		}
	}

	if (!fwnode_property_read_u32(connector, "op-sink-microwatt", &ret)) {
		info->sink_watt = ret / 500000; /* 500mw per unit */
	}

	cap->revision = USB_TYPEC_REV_1_2;
	cap->pd_revision = 0x300;	/* USB-PD spec release 3.0 */
	cap->accessory[0] = TYPEC_ACCESSORY_AUDIO;
	cap->ops = &husb239_ops;
	cap->driver_data = husb239;

	info->port = typec_register_port(dev, cap);
	if (IS_ERR(info->port)) {
		ret = PTR_ERR(info->port);
		goto err_put_role;
	}

	fwnode_handle_put(connector);
	return 0;

err_put_role:
	usb_role_switch_put(info->role_sw);
err_put_fwnode:
	fwnode_handle_put(connector);
	return ret;
}

static int husb239_typec_switch_probe(struct husb239 *husb239)
{
	struct typec_info *info = &husb239->info;
	struct typec_switch_desc sw_desc = { };
	struct fwnode_handle *fwnode = husb239->dev->fwnode;

	info->switch_supported = fwnode_property_read_bool(fwnode, "orientation");
	if (!info->switch_supported)
		return 0;

	husb239->oe_gpiod = devm_gpiod_get_optional(husb239->dev, "orient-oe", GPIOD_OUT_LOW);
	if (IS_ERR(husb239->oe_gpiod)) {
		return PTR_ERR(husb239->oe_gpiod);
	}

	husb239->sel_gpiod = devm_gpiod_get_optional(husb239->dev, "orient-sel", GPIOD_OUT_LOW);
	if (IS_ERR(husb239->sel_gpiod)) {
		return PTR_ERR(husb239->sel_gpiod);
	}

	sw_desc.fwnode = fwnode;
	sw_desc.drvdata = husb239;
	sw_desc.name = fwnode_get_name(fwnode);
	sw_desc.set = husb23_usb_set_orientation;

	info->sw = typec_switch_register(husb239->dev, &sw_desc);
	if (IS_ERR(info->sw)) {
		dev_err(husb239->dev, "switch register failed\n");
		return PTR_ERR(info->sw);
	}

	return 0;
}

static enum power_supply_usb_type husb239_psy_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_C,
	POWER_SUPPLY_USB_TYPE_PD,
	POWER_SUPPLY_USB_TYPE_PD_PPS,
};

static const enum power_supply_property husb239_psy_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CURRENT_NOW
};

static int husb239_psy_set_prop(struct power_supply *psy,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct husb239 *husb239 = power_supply_get_drvdata(psy);

	if (psp == POWER_SUPPLY_PROP_VOLTAGE_NOW) {
		husb239->req_voltage = val->intval / 1000;
		return husb239_usbpd_request_voltage(husb239);
	}

	return -EINVAL;
}

static int husb239_psy_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct husb239 *husb239 = power_supply_get_drvdata(psy);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = husb239->attached;
		break;
	case POWER_SUPPLY_PROP_USB_TYPE:
		val->intval = husb239->usb_type;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = husb239->voltage;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		val->intval = husb239->max_current;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = husb239->op_current;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int husb239_psy_prop_writeable(struct power_supply *psy,
				enum power_supply_property psp)
{
	if (psp == POWER_SUPPLY_PROP_VOLTAGE_NOW)
		return 1;
	return 0;
}

static int husb239_psy_register(struct husb239 *husb239)
{
	struct power_supply_desc *psy_desc = &husb239->psy_desc;
	struct power_supply_config psy_cfg = {};
	char *psy_name;

	psy_cfg.fwnode = dev_fwnode(husb239->dev);
	psy_cfg.drv_data = husb239;

	psy_name = devm_kasprintf(husb239->dev, GFP_KERNEL, "husb239-source-psy-%s",
				  dev_name(husb239->dev));
	if (!psy_name)
		return -ENOMEM;

	psy_desc->name = psy_name;
	psy_desc->type = POWER_SUPPLY_TYPE_USB;
	psy_desc->usb_types = husb239_psy_usb_types;
	psy_desc->num_usb_types = ARRAY_SIZE(husb239_psy_usb_types);
	psy_desc->properties = husb239_psy_props;
	psy_desc->num_properties = ARRAY_SIZE(husb239_psy_props);

	psy_desc->get_property = husb239_psy_get_prop;
	psy_desc->set_property = husb239_psy_set_prop;
	psy_desc->property_is_writeable = husb239_psy_prop_writeable;

	husb239->usb_type = POWER_SUPPLY_USB_TYPE_C;
	husb239->psy = devm_power_supply_register(husb239->dev, psy_desc, &psy_cfg);
	if (IS_ERR(husb239->psy))
		dev_warn(husb239->dev, "unable to register psy\n");

	return PTR_ERR_OR_ZERO(husb239->psy);
}

static const struct regmap_config config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = HUSB239_REG_MAX,
};

static int husb239_probe(struct i2c_client *client)
{
	struct husb239 *husb239;
	int ret;

	husb239 = devm_kzalloc(&client->dev, sizeof(struct husb239),
				 GFP_KERNEL);
	if (!husb239)
		return -ENOMEM;

	husb239->dev = &client->dev;
	i2c_set_clientdata(client, husb239);

	husb239->regmap = devm_regmap_init_i2c(client, &config);
	if (IS_ERR(husb239->regmap))
		return PTR_ERR(husb239->regmap);

	/* Initialize the hardware */
	ret = husb239_chip_init(husb239);
	if (ret)
		return ret;

	ret = husb239_typec_port_probe(husb239);
	if (ret) {
		dev_err(husb239->dev, "fail to probe typec property.\n");
		ret = -ENODEV;
		return ret;
	}

	ret = husb239_typec_switch_probe(husb239);
	if (ret) {
		dev_err(husb239->dev, "fail to probe typec switch\n");
		goto err_unreg_port;
	}

	ret = husb239_psy_register(husb239);
	if (ret) {
		dev_err(husb239->dev, "register psy\n");
		goto err_unreg_switch;
	}

	husb239->gpio_irq = client->irq;
	if (!husb239->gpio_irq)
		husb239_get_gpio_irq(husb239);

	if (!husb239->gpio_irq) {
		dev_err(husb239->dev, "fail to get interrupt IRQ\n");
		goto err_unreg_switch;
	}

	ret = husb239_irq_init(husb239);
	if (ret)
		goto err_unreg_switch;

	return 0;

err_unreg_switch:
	typec_switch_unregister(husb239->info.sw);
err_unreg_port:
	typec_unregister_port(husb239->info.port);
	usb_role_switch_put(husb239->info.role_sw);
	return ret;
}

static void husb239_remove(struct i2c_client *client)
{
	struct husb239 *husb239 = i2c_get_clientdata(client);
	struct typec_info *info = &husb239->info;

	if (husb239->workqueue)
		destroy_workqueue(husb239->workqueue);

	typec_switch_unregister(info->sw);
	typec_unregister_port(info->port);
	usb_role_switch_put(info->role_sw);
}

static const struct of_device_id dev_ids[] = {
	{ .compatible = "hynetek,husb239"},
	{}
};
MODULE_DEVICE_TABLE(of, dev_ids);

static struct i2c_driver husb239_driver = {
	.driver = {
		.name = "husb239",
		.of_match_table = of_match_ptr(dev_ids),
	},
	.probe = husb239_probe,
	.remove =  husb239_remove,
};

module_i2c_driver(husb239_driver);

MODULE_DESCRIPTION("hynetek HUSB239 DRP Port Controller Driver");
MODULE_LICENSE("GPL");

/* DVB USB framework compliant Linux driver for the
 * Google DVB-S SC100 tuner.
 *
 * Copyright (C) 2015 Ke Dong (kedong@google.com)
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation, version 2.
 *
 * see Documentation/dvb/README.dvb-usb for more information
 */
#include "sc100.h"
#include "dvbsky_m88rs6000.h"

#define SC100_READ_MSG 0
#define SC100_WRITE_MSG 1
#define SC100_MAC_ADDR_INDEX 1

#define	err_str "did not find the firmware file. (%s) " \
		"Please see linux/Documentation/dvb/ for more details " \
		"on firmware-problems."

/* debug */
static int dvb_usb_sc100_debug;
module_param_named(debug, dvb_usb_sc100_debug, int, 0644);
static char *mac_addr_str = "90:09:1e:f1:be:33";
module_param_named(mac_addr, mac_addr_str, charp, 0000);
MODULE_PARM_DESC(debug, "set debugging level (1=info)." DVB_USB_DEBUG_STATUS);

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

/* Control register for LNP settings
 * B7: I2C Control
 *     1 - enabled, 0 - disabled
 * B6: Reserved
 * B5: Tone Gate
 *     1 - on, 0 - off
 * B4: Tone Mode
 *     1 - internal, 0 - external
 * B3: LNP output voltage
 *     1 - enabled, 0 - disabled
 * B2-1: Output voltage selection
 *     0 - 13V, 4 - 18V
 */
#define LNB_I2C_ADDRESS (0x60)
#define LNB_CONTROL_REGISTER_1 (0x00)
#define LNB_CONTROL_REGISTER_1_I2C_CONTROL_MASK (0x80)
#define LNB_CONTROL_REGISTER_1_I2C_CONTROL_SHIFT (0x07)
#define LNB_CONTROL_REGISTER_1_I2C_CONTROL_ENABLED (0x01)
#define LNB_CONTROL_REGISTER_1_I2C_CONTROL_DISABLED (0x00)
#define LNB_CONTROL_REGISTER_1_TONE_GATE_MASK (0x20)
#define LNB_CONTROL_REGISTER_1_TONE_GATE_SHIFT (0x05)
#define LNB_CONTROL_REGISTER_1_TONE_GATE_ON (0x01)
#define LNB_CONTROL_REGISTER_1_TONE_GATE_OFF (0x00)
#define LNB_CONTROL_REGISTER_1_TONE_MODE_MASK (0x10)
#define LNB_CONTROL_REGISTER_1_TONE_MODE_SHIFT (0x4)
#define LNB_CONTROL_REGISTER_1_TONE_MODE_INTERNAL (0x01)
#define LNB_CONTROL_REGISTER_1_TONE_MODE_EXTERNAL (0x00)
#define LNB_CONTROL_REGISTER_1_LNB_OUTPUT_VOLTAGE_MASK (0x08)
#define LNB_CONTROL_REGISTER_1_LNB_OUTPUT_VOLTAGE_SHIFT (0x03)
#define LNB_CONTROL_REGISTER_1_LNB_OUTPUT_VOLTAGE_ON (0x01)
#define LNB_CONTROL_REGISTER_1_LNB_OUTPUT_VOLTAGE_OFF 0x00
#define LNB_CONTROL_REGISTER_1_LNB_VOLTAGE_SELECTION_MASK 0x07
#define LNB_CONTROL_REGISTER_1_LNB_VOLTAGE_SELECTION_SHIFT 0x00
#define LNB_CONTROL_REGISTER_1_LNB_VOLTAGE_SELECTION_13V 0x00
#define LNB_CONTROL_REGISTER_1_LNB_VOLTAGE_SELECTION_18V 0x04
#define LNB_FIELD_SET(data, field, value) \
{ \
	data &= ~(LNB_CONTROL_REGISTER_1_ ## field ## _MASK); \
	data |= LNB_CONTROL_REGISTER_1_ ## field ## _ ## value \
	<< (LNB_CONTROL_REGISTER_1_ ## field ## _ ## SHIFT); \
}

struct i2c_adapter *sc100_i2c_adap;
struct usb_device *sc100_udev;

static int sc100_read_mac_address(struct dvb_usb_device *d, u8 mac[6])
{
	if (!mac_pton(mac_addr_str, mac)) {
		return -EINVAL;
	}
	return 0;
};

static int sc100_writereg(u8 reg, u8 data)
{
	u8 buf[] = { reg, data };
	struct i2c_msg msg = { .addr = LNB_I2C_ADDRESS,
		.flags = 0, .buf = buf, .len = 2 };
	int ret;

	deb_info("%s: [W] R:0x%02x, V:0x%02x", __func__, reg, data);

	ret = i2c_transfer(sc100_i2c_adap, &msg, 1);
	if (ret != 1) {
		err("%s: [W] R:0x%02x, V:0x%02x, E:%d", __func__, reg, data, ret);
		return -EREMOTEIO;
	}
	return 0;
}

static u8 sc100_readreg(u8 reg)
{
	int ret;
	u8 b0[] = { reg };
	u8 b1[] = { 0 };

	struct i2c_msg msg[] = {
		{ .addr = LNB_I2C_ADDRESS, .flags = 0, .buf = b0, .len = 1 },
		{ .addr = LNB_I2C_ADDRESS, .flags = I2C_M_RD, .buf = b1, .len = 1 }
	};
	ret = i2c_transfer(sc100_i2c_adap, msg, 2);

	if (ret != 2) {
		err("%s: [R] R:0x%x, E:%d", __func__, reg, ret);
		return ret;
	}
	deb_info("%s: [R] R:0x%02x, V:0x%02x", __func__, reg, b1[0]);

	return b1[0];
}

static int sc100_set_tone(struct dvb_frontend* fe, fe_sec_tone_mode_t tone)
{
	u8 lnb_reg = sc100_readreg(LNB_CONTROL_REGISTER_1);
	switch (tone) {
	case SEC_TONE_ON:
		info("%s: LNB TONE=ON\n", __func__);
		LNB_FIELD_SET(lnb_reg, I2C_CONTROL, ENABLED);
		LNB_FIELD_SET(lnb_reg, TONE_GATE, ON);
		LNB_FIELD_SET(lnb_reg, TONE_MODE, INTERNAL);
		break;
	case SEC_TONE_OFF:
		info("%s: LNB TONE=OFF\n", __func__);
		LNB_FIELD_SET(lnb_reg, I2C_CONTROL, ENABLED);
		LNB_FIELD_SET(lnb_reg, TONE_GATE, OFF);
		break;
	default:
		return -EINVAL;
	}
	return sc100_writereg(LNB_CONTROL_REGISTER_1, lnb_reg);
}

static int sc100_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	u8 lnb_reg = sc100_readreg(LNB_CONTROL_REGISTER_1);
	switch (voltage) {
	case SEC_VOLTAGE_18:
		info("%s: LNB VOLTAGE=18V\n", __func__);
		LNB_FIELD_SET(lnb_reg, I2C_CONTROL, ENABLED);
		LNB_FIELD_SET(lnb_reg, LNB_OUTPUT_VOLTAGE, ON);
		LNB_FIELD_SET(lnb_reg, LNB_VOLTAGE_SELECTION, 18V);
		break;
	case SEC_VOLTAGE_13:
		info("%s: LNB VOLTAGE=13V\n", __func__);
		LNB_FIELD_SET(lnb_reg, I2C_CONTROL, ENABLED);
		LNB_FIELD_SET(lnb_reg, LNB_OUTPUT_VOLTAGE, ON);
		LNB_FIELD_SET(lnb_reg, LNB_VOLTAGE_SELECTION, 13V);
		break;
	default:
		info("%s: LNB VOLTAGE=OFF\n", __func__);
		LNB_FIELD_SET(lnb_reg, I2C_CONTROL, ENABLED);
		LNB_FIELD_SET(lnb_reg, LNB_OUTPUT_VOLTAGE, OFF);
		break;
	}
	return sc100_writereg(LNB_CONTROL_REGISTER_1, lnb_reg);
}

static int sc100_set_ts_params(struct dvb_frontend* fe, int is_punctured)
{
	return usb_control_msg(sc100_udev, usb_sndctrlpipe(sc100_udev,0),
			0xa4, USB_TYPE_VENDOR, 0, 0, 0, 0, 5000);
}

static struct dvbsky_m88rs6000_config sc100_config = {
	.demod_address = 0x69,
	.ci_mode = 2,
	.pin_ctrl = 0x80,
	.ts_mode = 0,
	.tuner_readstops = 1,
	.set_ts_params = sc100_set_ts_params,
};

static struct dvb_usb_device_properties sc100_properties;

static int sc100_frontend_attach(struct dvb_usb_adapter *d)
{
	/* Initializes i2c adapter when the frontend is attached. */
	sc100_i2c_adap = i2c_get_adapter(0);
	d->fe_adap[0].fe = dvb_attach(dvbsky_m88rs6000_attach, &sc100_config,
				sc100_i2c_adap);
	if (d->fe_adap[0].fe != NULL) {
		d->fe_adap[0].fe->ops.set_tone = sc100_set_tone;
		d->fe_adap[0].fe->ops.set_voltage = sc100_set_voltage;
		info("Attached SC100!\n");
		return 0;
	}
	return -EIO;
}

static struct usb_device_id sc100_table[] = {
	{USB_DEVICE(USB_VID_CYPRESS, USB_PID_SC100_COLD)},
	{USB_DEVICE(USB_VID_CYPRESS, USB_PID_SC100_WARM)},
	{ }
};
MODULE_DEVICE_TABLE(usb, sc100_table);

static struct dvb_usb_device_properties sc100_properties = {
	.usb_ctrl = CYPRESS_FX2,
	.firmware = "dvb-usb-sc100.fw",

	/* parameter for the MPEG2-data transfer */
	.num_adapters = 1,
	.read_mac_address = sc100_read_mac_address,
	.adapter = {
		{
		.num_frontends = 1,
		.fe = {{
			.frontend_attach = sc100_frontend_attach,
			.stream = {
				.type = USB_BULK,
				.count = MAX_NO_URBS_FOR_DATA_STREAM,
				.endpoint = 0x82,
				.u = {
					.bulk = {
						.buffersize = 16384,
					}
				}
			},
		}},
		}
	},
	.num_device_descs = 1,
	.devices = {
		{ .name = "Google DVB-S SC100 USB2.0",
		  .cold_ids = { &sc100_table[0], NULL },
		  .warm_ids = { &sc100_table[1], NULL },
		},
		{NULL},
	}
};

static int sc100_probe(struct usb_interface *intf,
		const struct usb_device_id *id)
{
	sc100_udev = interface_to_usbdev(intf);
	if (0 == dvb_usb_device_init(intf, &sc100_properties,
			THIS_MODULE, NULL, adapter_nr))
		return 0;
	return -ENODEV;
}

static struct usb_driver sc100_driver = {
	.name = "sc100",
	.probe = sc100_probe,
	.disconnect = dvb_usb_device_exit,
	.id_table = sc100_table,
};

static int __init sc100_module_init(void)
{
	int ret = usb_register(&sc100_driver);
	if (ret)
		err("usb_register failed. Error number %d", ret);

	return ret;
}

static void __exit sc100_module_exit(void)
{
	usb_deregister(&sc100_driver);
}

module_init(sc100_module_init);
module_exit(sc100_module_exit);

MODULE_AUTHOR("Ke Dong (c) kedong@google.com");
MODULE_DESCRIPTION("Driver for Google SC100 device");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");

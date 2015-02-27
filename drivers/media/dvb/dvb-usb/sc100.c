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
#include "mach/comcerto-2000.h"

#define SC100_VOLTAGE_CTRL (0x60)
#define SC100_READ_MSG 0
#define SC100_WRITE_MSG 1
#define SC100_MAC_ADDR_INDEX 1

#define	err_str "did not find the firmware file. (%s) " \
		"Please see linux/Documentation/dvb/ for more details " \
		"on firmware-problems."

/* debug */
static int dvb_usb_sc100_debug;
module_param_named(debug, dvb_usb_sc100_debug, int, 0644);
MODULE_PARM_DESC(debug, "set debugging level (1=info 2=xfer 4=rc(or-able))."
						DVB_USB_DEBUG_STATUS);

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static int sc100_read_mac_address(struct dvb_usb_device *d, u8 mac[6])
{
	return comcerto_mac_addr_get(mac, SC100_MAC_ADDR_INDEX);
};

static int sc100_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	static u8 command_13v[] = {0x00, 0x88};
	static u8 command_18v[] = {0x00, 0x8c};
	static u8 command_off[] = {0x00, 0x80};
	struct i2c_msg msg = {
		.addr = SC100_VOLTAGE_CTRL,
		.flags = 0,
		.buf = command_off,
		.len = 2,
	};
	struct i2c_adapter *i2c_adap = i2c_get_adapter(0);

	if (voltage == SEC_VOLTAGE_18)
		msg.buf = command_18v;
	else if (voltage == SEC_VOLTAGE_13)
		msg.buf = command_13v;

	i2c_transfer(i2c_adap, &msg, 1);

	return 0;
}

static struct dvbsky_m88rs6000_config sc100_config = {
	.demod_address = 0x69,
	.ci_mode = 1,
	.pin_ctrl = 0x80,
	.ts_mode = 0,
	.tuner_readstops = 1,
};

static struct dvb_usb_device_properties sc100_properties;

static int sc100_frontend_attach(struct dvb_usb_adapter *d)
{
	struct i2c_adapter *i2c_adap = i2c_get_adapter(0);
	d->fe_adap[0].fe = dvb_attach(dvbsky_m88rs6000_attach, &sc100_config,
				i2c_adap);
	if (d->fe_adap[0].fe != NULL) {
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
				.count = 4,
				.endpoint = 0x02,
				.u = {
					.bulk = {
						.buffersize = 512,
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

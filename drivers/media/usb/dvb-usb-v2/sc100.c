/*
 * Driver for SC100 DVB-S USB2.0 receiver
 *
 * The hardware design is based on a Montage M88RS6000 tuner/demodulator,
 * a Cypress CY7C68013A USB bridge and a TI TPS65233 voltage regulator.
 * The M88RS6000 chip integrates a M88RS6000 tuner and a M88DS3103
 * demodulator. The demodulator is directly connected to the I2C bus.
 * The tuner is connected internally to the M88DS3103 and can be
 * accessed through an I2C mux. The CY7C68013A USB bridge is connected
 * to the MPEG-TS interface of the M88RS6000 and makes the data on the
 * MPEG-TS available to the host through an USB bulk endpoint. The
 * TPS65233 has an I2C interface, however in the sc100 design it is
 * controlled through pins driven by the M88RS6000 chip.
 *
 * Copyright (C) 2015 Google Inc.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "dvb_usb.h"
#include "m88ds3103.h"
#include "m88rs6000t.h"
#include "cypress_firmware.h"

static char *mac_addr_str = "90:09:1e:f1:be:33";
module_param_named(mac_addr, mac_addr_str, charp, 0000);

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

struct sc100_state {
	struct mutex stream_mutex;
	struct i2c_client *i2c_client_tuner;
};

static int sc100_read_mac_addr(struct dvb_usb_adapter *adap, u8 mac[6])
{
	if (!mac_pton(mac_addr_str, mac)) {
		return -EINVAL;
	}
	return 0;
}

static int sc100_download_firmware(struct dvb_usb_device *d,
	const struct firmware *fw)
{
	pr_debug("Loading dvb-usb-sc100 firmware\n");

	return cypress_load_firmware(d->udev, fw, CYPRESS_FX2);
}

static int sc100_identify_state(struct dvb_usb_device *d, const char **name)
{
	if (d->udev->descriptor.idProduct == USB_PID_SC100_WARM)
		return WARM;
	else
		return COLD;
}

static int sc100_stream_ctrl(struct dvb_usb_device *d, u8 onoff)
{
	struct sc100_state *state = d_to_priv(d);
	int ret = 0;

	mutex_lock(&state->stream_mutex);

	if (onoff) {
		/* Configure the TS parameters */
		/* TODO: details? */
		ret = usb_control_msg(d->udev, usb_sndctrlpipe(d->udev, 0),
				0xa4, USB_TYPE_VENDOR, 0, 0, 0, 0, 5000);
	}

	mutex_unlock(&state->stream_mutex);

	return ret;
}

static int sc100_streaming_ctrl(struct dvb_frontend *fe, int onoff)
{
	struct dvb_usb_device *d = fe_to_d(fe);

	return sc100_stream_ctrl(d, (onoff == 0) ? 0 : 1);
}

static const struct m88ds3103_config sc100_m88rs6000_cfg = {
	.i2c_addr = 0x69,
	.clock = 27000000,
	.i2c_wr_max = 33,
	.ts_mode = M88DS3103_TS_CI,
	.ts_clk = 16000,
	.ts_clk_pol = 1,
	.agc = 0x99,
	.lnb_hv_pol = 0,
	.lnb_en_pol = 1,
};

static int sc100_attach(struct dvb_usb_adapter *adap)
{
	struct sc100_state *state = adap_to_priv(adap);
	struct dvb_usb_device *d = adap_to_d(adap);
	int ret = 0;
	struct i2c_adapter *sc100_i2c_adap = i2c_get_adapter(0);
	/* demod I2C adapter */
	struct i2c_adapter *i2c_adapter;
	struct i2c_client *client_tuner;
	struct i2c_board_info info;
	struct m88rs6000t_config m88rs6000t_config;
	memset(&info, 0, sizeof(struct i2c_board_info));

	/* attach demod */
	adap->fe[0] = dvb_attach(m88ds3103_attach,
				&sc100_m88rs6000_cfg,
				sc100_i2c_adap,
				&i2c_adapter);
	if (!adap->fe[0]) {
		dev_err(&d->udev->dev, "sc100_attach fail.\n");
		ret = -ENODEV;
		goto fail_attach;
	}

	/* attach tuner */
	m88rs6000t_config.fe = adap->fe[0];
	strlcpy(info.type, "m88rs6000t", I2C_NAME_SIZE);
	info.addr = 0x21;
	info.platform_data = &m88rs6000t_config;
	request_module(info.type);
	client_tuner = i2c_new_device(i2c_adapter, &info);
	if (client_tuner == NULL || client_tuner->dev.driver == NULL) {
		dvb_frontend_detach(adap->fe[0]);
		ret = -ENODEV;
		goto fail_attach;
	}
	state->i2c_client_tuner = client_tuner;

	if (!try_module_get(client_tuner->dev.driver->owner)) {
		i2c_unregister_device(client_tuner);
		dvb_frontend_detach(adap->fe[0]);
		ret = -ENODEV;
		goto fail_attach;
	}

	/* delegate signal strength measurement to tuner */
	adap->fe[0]->ops.read_signal_strength =
		adap->fe[0]->ops.tuner_ops.get_rf_strength;

	return ret;

fail_attach:
	return ret;
}

static int sc100_init(struct dvb_usb_device *d)
{
	struct sc100_state *state = d_to_priv(d);

	mutex_init(&state->stream_mutex);

	return 0;
}

static void sc100_exit(struct dvb_usb_device *d)
{
	struct sc100_state *state = d_to_priv(d);
	struct i2c_client *client;

	client = state->i2c_client_tuner;
	/* remove I2C tuner */
	if (client) {
		module_put(client->dev.driver->owner);
		i2c_unregister_device(client);
	}
}

static struct dvb_usb_device_properties sc100_props = {
	.firmware = "dvb-usb-sc100.fw",

	.driver_name = KBUILD_MODNAME,
	.owner = THIS_MODULE,
	.adapter_nr = adapter_nr,
	.size_of_priv = sizeof(struct sc100_state),

	.frontend_attach	= sc100_attach,
	.init			= sc100_init,
	.streaming_ctrl		= sc100_streaming_ctrl,
	.identify_state		= sc100_identify_state,
	.exit			= sc100_exit,
	.read_mac_address	= sc100_read_mac_addr,
	.download_firmware	= sc100_download_firmware,

	.num_adapters = 1,
	.adapter = {
		{
			.stream = DVB_USB_STREAM_BULK(
				0x82, MAX_NO_URBS_FOR_DATA_STREAM, 16384),
		}
	}
};

static const struct usb_device_id sc100_id_table[] = {
	{ DVB_USB_DEVICE(USB_VID_CYPRESS,
		USB_PID_SC100_COLD,
		&sc100_props, "Google DVB-S SC100 USB2.0",
		NULL) },
	{ DVB_USB_DEVICE(USB_VID_CYPRESS,
		USB_PID_SC100_WARM,
		&sc100_props, "Google DVB-S SC100 USB2.0",
		NULL) },
	{ }
};
MODULE_DEVICE_TABLE(usb, sc100_id_table);

static struct usb_driver sc100_usb_driver = {
	.name = KBUILD_MODNAME,
	.id_table = sc100_id_table,
	.probe = dvb_usbv2_probe,
	.disconnect = dvb_usbv2_disconnect,
	.suspend = dvb_usbv2_suspend,
	.resume = dvb_usbv2_resume,
	.reset_resume = dvb_usbv2_reset_resume,
	.no_dynamic_id = 1,
	.soft_unbind = 1,
};

module_usb_driver(sc100_usb_driver);

MODULE_AUTHOR("Matthias Kaehlcke <mka@google.com>");
MODULE_DESCRIPTION("Driver for sc100 USB satellite receiver");
MODULE_LICENSE("GPL");

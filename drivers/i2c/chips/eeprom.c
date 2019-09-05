/*
 *  Copyright (C) 2008 Mindspeed Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>

MODULE_AUTHOR("Mindspeed Technologies, Inc.");
MODULE_DESCRIPTION("I2C EEPROM driver");
MODULE_LICENSE("GPL");

#define CONFIG_EEPROM_PAGE_SIZE 128
#define CONFIG_EEPROM_CHIP_SIZE (512*1024)
#define CONFIG_EEPROM_WRITE_TIME 5

#define EEPROM_PAGE	(CONFIG_EEPROM_PAGE_SIZE)
#define EEPROM_SIZE	(CONFIG_EEPROM_CHIP_SIZE)
#define EEPROM_WAIT	(CONFIG_EEPROM_WRITE_TIME)

#define DRV_NAME	"eeprom"

struct i2c_client *eeprom_i2c_client;

static inline int eeprom_set_address(struct i2c_msg *msg, u32 addr)
{
	int addr_len = 1;

	msg->buf[0] = addr >> 8;
	msg->len++;
	if (EEPROM_SIZE > 256) {
		msg->buf[1] = addr & 255;
		msg->len++;
		addr_len++;
	}

	return addr_len;
}

static int eeprom_set_pointer(struct i2c_client *client, u32 addr)
{
	int err;
	u8 buf[2];
	struct i2c_msg msg =
	{
		.addr = client->addr,
		.flags = 0,
		.len = 0,
		.buf = buf,
	};

	eeprom_set_address(&msg, addr);
	if ((err = i2c_transfer(client->adapter, &msg, 1)) != 1) {
		dev_err(&client->dev, "read transaction failed - couldn't set address, code: %d\n", err);
		return -EIO;
	}

	return 0;
}

static int eeprom_read_data(struct i2c_client *client, void *buf, int len)
{
	int err;
	struct i2c_msg	msg =
	{
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = len,
		.buf = buf,
	};

	if ((err = i2c_transfer(client->adapter, &msg, 1)) != 1) {
		dev_err(&client->dev, "read transaction failed, code: %d\n", err);
		return -EIO;
	}

	return 0;
}

static int eeprom_write_data(struct i2c_client *client, u32 addr, void *buf, void *src, int len)
{
	int err;
	struct i2c_msg msg =
	{
		.addr = client->addr,
		.flags = 0,
		.len = len,
		.buf = buf,
	};

	/* set address and copy data just after address bytes */
	memcpy(buf + eeprom_set_address(&msg, addr), src, len);

	if ((err = i2c_transfer(client->adapter, &msg, 1)) != 1) {
		dev_err(&client->dev, "write transaction failed, code: %d\n", err);
		return -EIO;
	}

	mdelay(EEPROM_WAIT);

	return 0;
}

static int eeprom_read_write(struct kobject *kobj, char *buf, loff_t offs, size_t len, int read)
{
	struct i2c_client *client = to_i2c_client(container_of(kobj, struct device, kobj));
	int count;

	if (offs + len > EEPROM_SIZE)
		len = EEPROM_SIZE - offs;
	if (len < 0)
		return -EINVAL;
	if (len == 0)
		return 0;

	if (read) {
		count = eeprom_set_pointer(client, offs);	/* for the read transaction set address once at start */
		if (count == 0)
			count = eeprom_read_data(client, buf, len);
		if (count == 0)
			count = len;
	}
	else {
		u8 *txbuf = kmalloc(EEPROM_PAGE + 2, GFP_KERNEL);
		u32 tmp;
		int req, err;

		if (txbuf != NULL) {
			/* write all data in no more than one page size transaction */
			count = 0;

			do {
				tmp = (offs + count + EEPROM_PAGE) & ~(EEPROM_PAGE - 1);
				req = tmp - offs - count;
				if (req + count > len)
					req = len - count;

				err = eeprom_write_data(client, offs+count, txbuf, buf+count, req);
				if (err < 0) {
					count = err;
					break;
				}

				count += req;
			} while (count < len);

			kfree(txbuf);
		} else {
			dev_err(&client->dev, "failed to allocate memory\n");
			count = -ENOMEM;
		}
	}

	return count;
}

static ssize_t eeprom_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0;
	loff_t off = 0x50;
	size_t ret, count = 4;

	ret = eeprom_read_write(&dev->kobj, buf, off + i, count, 1);

	return ret;
}

static ssize_t eeprom_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	loff_t off = 0x50;

	return eeprom_read_write(&dev->kobj, buf, off, count, 0);
}

static struct device_attribute eeprom_attr = __ATTR(eeprom, 0644, eeprom_read, eeprom_write);

int eeprom_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err = 0;

	client = eeprom_i2c_client;

	err = sysfs_create_file(&client->dev.kobj, &eeprom_attr.attr);
	if (err) {
		dev_err(&client->dev, "failed to create sysfs node\n");
		goto err;
	}

	return 0;

err:
	return err;
}

static int eeprom_remove(struct i2c_client *client)
{
	sysfs_remove_bin_file(&client->dev.kobj, &eeprom_attr);

	return 0;
}

#ifdef CONFIG_PM
static int eeprom_suspend(struct i2c_client *client, pm_message_t mesg)
{
	return 0;
}

static int eeprom_resume(struct i2c_client *client)
{
         return 0;
}
#else
#define eeprom_suspend         NULL
#define eeprom_resume          NULL
#endif /* CONFIG_PM */

static struct i2c_device_id eeprom_idtable[] = {
	{ "eeprom", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, eeprom_idtable);

static struct i2c_driver eeprom_driver = {
	.id_table       = eeprom_idtable,
	.probe		= eeprom_probe,
	.remove 	= eeprom_remove,
	.suspend        = eeprom_suspend,
	.resume         = eeprom_resume,
	.driver = {
		.name   = DRV_NAME,
		.owner  =THIS_MODULE,
	},
};

static unsigned short normal_i2c[] = { 0x50, I2C_CLIENT_END };

static int eeprom_init(void)
{
	struct i2c_adapter *i2c_adap;
	struct i2c_board_info i2c_info;

	i2c_adap = i2c_get_adapter(0);
	memset(&i2c_info, 0, sizeof(struct i2c_board_info));
	strlcpy(i2c_info.type, "eeprom", I2C_NAME_SIZE);
	eeprom_i2c_client = i2c_new_probed_device(i2c_adap, &i2c_info, normal_i2c, NULL);

	i2c_put_adapter(i2c_adap);

	i2c_add_driver(&eeprom_driver);

	return 0;
}

static void eeprom_exit(void)
{
	i2c_del_driver(&eeprom_driver);
}

module_init(eeprom_init);
module_exit(eeprom_exit);


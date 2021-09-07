#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef MY_DEF_HERE
 
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <asm/fsl_pixis.h>

#define VOUT_MODE	0x20	 
#define DEVICE_ID	0xE4	 
#define READ_VOUT	0x8B	 
#define READ_IOUT	0x8C	 

#define LEN_BYTE		0x01	 
#define LEN_WORD		0x02	 
#define ZL6100_ID		"\x10ZL6100-002-FE03"	 

struct zl6100_data {
	struct device *hwmon_dev;
	s32 val;
};

static s32 read_register(struct i2c_client *client, u8 cmd, u8 len)
{

	struct zl6100_data *priv_data = i2c_get_clientdata(client);

	mdelay(2);

	switch (len) {
	case LEN_BYTE:
		priv_data->val = i2c_smbus_read_byte_data(client, cmd);
		break;

	case LEN_WORD:
		priv_data->val = i2c_smbus_read_word_data(client, cmd);
		break;

	default:
		priv_data->val = -EIO;
	}

	return priv_data->val;
}

static int check_data_mode(struct i2c_client *client)
{
	s32	mode;

	mode = read_register(client, VOUT_MODE, LEN_BYTE);
	if (mode != 0x13) {
		printk(KERN_WARNING "chip 0x%2x mode is not LINEAR.\n",
				client->addr);
		return mode;
	}

	return 0;
}

static int zl6100_check_id(struct i2c_client *client)
{
	s32 id;
	u8	buf[16];

	mdelay(2);
	id = i2c_smbus_read_i2c_block_data(client, DEVICE_ID, 16, buf);
	if (id != 16 || memcmp(buf, ZL6100_ID, 7))
		printk(KERN_WARNING
			"chip 0x%2x ID does not match.\n", client->addr);

	return 0;
}

static int show_volt(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	s32 volt;

	struct i2c_client *client = to_i2c_client(dev);

	volt = read_register(client, READ_VOUT, LEN_WORD);
	if (volt < 0)
		return sprintf(buf, "%d\n", 0);

	volt = pmbus_2volt(volt);

	return sprintf(buf, "%d\n", volt);
}

static int show_curr(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	s32	cur;

	struct i2c_client *client = to_i2c_client(dev);

	cur = read_register(client, READ_IOUT, LEN_WORD);

	cur = pmbus_2cur(cur);

	return sprintf(buf, "%d\n", cur);
}

static DEVICE_ATTR(in0_input, S_IRUGO, show_volt, NULL);
static DEVICE_ATTR(curr1_input, S_IRUGO, show_curr, NULL);

static struct attribute *zl6100_attributes[] = {
	&dev_attr_in0_input.attr,
	&dev_attr_curr1_input.attr,
	NULL
};

static const struct attribute_group zl6100_group = {
	.attrs = zl6100_attributes,
};

static int zl6100_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	int err;
	struct zl6100_data *data = NULL;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_WORD_DATA |
			I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_I2C_BLOCK))
		return -EIO;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);

	err = sysfs_create_group(&client->dev.kobj, &zl6100_group);
	if (err)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	mdelay(2);
	i2c_smbus_read_word_data(client, READ_VOUT);

	check_data_mode(client);
	zl6100_check_id(client);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &zl6100_group);
exit_free:
	kfree(data);
	i2c_set_clientdata(client, NULL);
	return err;
}

static int zl6100_remove(struct i2c_client *client)
{
	struct zl6100_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &zl6100_group);
	kfree(data);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id zl6100_id[] = {
	{ "zl6100", 0},
	{ }
};

static struct i2c_driver zl6100_driver = {
	.driver = {
		.name = "zl6100",
	},
	.probe = zl6100_probe,
	.remove = zl6100_remove,
	.id_table = zl6100_id,
};

static int __init zl6100_init(void)
{
	return i2c_add_driver(&zl6100_driver);
}

static void __exit zl6100_exit(void)
{
	i2c_del_driver(&zl6100_driver);
}

MODULE_AUTHOR("Yuantian Tang <b29983@freescale.com>");
MODULE_DESCRIPTION("Intersil zl6100 driver");
MODULE_LICENSE("GPL");

module_init(zl6100_init);
module_exit(zl6100_exit);
#endif  

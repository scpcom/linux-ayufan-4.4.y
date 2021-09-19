#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon-vid.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/dmi.h>
#include <linux/acpi.h>
#include <linux/io.h>

#define DRVNAME "it87"

enum chips { it87, it8712, it8716, it8718, it8720, it8721, it8728, it8771,
	     it8772, it8782, it8783 };

static unsigned short force_id;
module_param(force_id, ushort, 0);
MODULE_PARM_DESC(force_id, "Override the detected device ID");

static struct platform_device *pdev;

#define	REG	0x2e	 
#define	DEV	0x07	 
#define	VAL	0x2f	 
#define PME	0x04	 

#define GPIO	0x07

#define	DEVID	0x20	 
#define	DEVREV	0x22	 

static inline int superio_inb(int reg)
{
	outb(reg, REG);
	return inb(VAL);
}

static inline void superio_outb(int reg, int val)
{
	outb(reg, REG);
	outb(val, VAL);
}

static int superio_inw(int reg)
{
	int val;
	outb(reg++, REG);
	val = inb(VAL) << 8;
	outb(reg, REG);
	val |= inb(VAL);
	return val;
}

static inline void superio_select(int ldn)
{
	outb(DEV, REG);
	outb(ldn, VAL);
}

static inline int superio_enter(void)
{
	 
	if (!request_muxed_region(REG, 2, DRVNAME))
		return -EBUSY;

	outb(0x87, REG);
	outb(0x01, REG);
	outb(0x55, REG);
	outb(0x55, REG);
	return 0;
}

static inline void superio_exit(void)
{
	outb(0x02, REG);
	outb(0x02, VAL);
	release_region(REG, 2);
}

#define IT8712F_DEVID 0x8712
#define IT8705F_DEVID 0x8705
#define IT8716F_DEVID 0x8716
#define IT8718F_DEVID 0x8718
#define IT8720F_DEVID 0x8720
#define IT8721F_DEVID 0x8721
#define IT8726F_DEVID 0x8726
#define IT8728F_DEVID 0x8728
#define IT8771E_DEVID 0x8771
#define IT8772E_DEVID 0x8772
#define IT8782F_DEVID 0x8782
#define IT8783E_DEVID 0x8783
#define IT87_ACT_REG  0x30
#define IT87_BASE_REG 0x60

#define IT87_SIO_GPIO1_REG	0x25
#define IT87_SIO_GPIO3_REG	0x27
#define IT87_SIO_GPIO5_REG	0x29
#define IT87_SIO_PINX1_REG	0x2a	 
#define IT87_SIO_PINX2_REG	0x2c	 
#define IT87_SIO_SPI_REG	0xef	 
#define IT87_SIO_VID_REG	0xfc	 
#define IT87_SIO_BEEP_PIN_REG	0xf6	 

static bool update_vbat;

static bool fix_pwm_polarity;

#define IT87_EXTENT 8

#define IT87_EC_EXTENT 2

#define IT87_EC_OFFSET 5

#define IT87_ADDR_REG_OFFSET 0
#define IT87_DATA_REG_OFFSET 1

#define IT87_REG_CONFIG        0x00

#define IT87_REG_ALARM1        0x01
#define IT87_REG_ALARM2        0x02
#define IT87_REG_ALARM3        0x03

#define IT87_REG_VID           0x0a
 
#define IT87_REG_FAN_DIV       0x0b
#define IT87_REG_FAN_16BIT     0x0c

static const u8 IT87_REG_FAN[]		= { 0x0d, 0x0e, 0x0f, 0x80, 0x82 };
static const u8 IT87_REG_FAN_MIN[]	= { 0x10, 0x11, 0x12, 0x84, 0x86 };
static const u8 IT87_REG_FANX[]		= { 0x18, 0x19, 0x1a, 0x81, 0x83 };
static const u8 IT87_REG_FANX_MIN[]	= { 0x1b, 0x1c, 0x1d, 0x85, 0x87 };
static const u8 IT87_REG_TEMP_OFFSET[]	= { 0x56, 0x57, 0x59 };

#define IT87_REG_FAN_MAIN_CTRL 0x13
#define IT87_REG_FAN_CTL       0x14
#define IT87_REG_PWM(nr)       (0x15 + (nr))
#define IT87_REG_PWM_DUTY(nr)  (0x63 + (nr) * 8)

#define IT87_REG_VIN(nr)       (0x20 + (nr))
#define IT87_REG_TEMP(nr)      (0x29 + (nr))

#define IT87_REG_VIN_MAX(nr)   (0x30 + (nr) * 2)
#define IT87_REG_VIN_MIN(nr)   (0x31 + (nr) * 2)
#define IT87_REG_TEMP_HIGH(nr) (0x40 + (nr) * 2)
#define IT87_REG_TEMP_LOW(nr)  (0x41 + (nr) * 2)

#define IT87_REG_VIN_ENABLE    0x50
#define IT87_REG_TEMP_ENABLE   0x51
#define IT87_REG_TEMP_EXTRA    0x55
#define IT87_REG_BEEP_ENABLE   0x5c

#define IT87_REG_CHIPID        0x58

#define IT87_REG_AUTO_TEMP(nr, i) (0x60 + (nr) * 8 + (i))
#define IT87_REG_AUTO_PWM(nr, i)  (0x65 + (nr) * 8 + (i))

struct it87_devices {
	const char *name;
	u16 features;
	u8 peci_mask;
	u8 old_peci_mask;
};

#define FEAT_12MV_ADC		(1 << 0)
#define FEAT_NEWER_AUTOPWM	(1 << 1)
#define FEAT_OLD_AUTOPWM	(1 << 2)
#define FEAT_16BIT_FANS		(1 << 3)
#define FEAT_TEMP_OFFSET	(1 << 4)
#define FEAT_TEMP_PECI		(1 << 5)
#define FEAT_TEMP_OLD_PECI	(1 << 6)

static const struct it87_devices it87_devices[] = {
	[it87] = {
		.name = "it87",
		.features = FEAT_OLD_AUTOPWM,	 
	},
	[it8712] = {
		.name = "it8712",
		.features = FEAT_OLD_AUTOPWM,	 
	},
	[it8716] = {
		.name = "it8716",
		.features = FEAT_16BIT_FANS | FEAT_TEMP_OFFSET,
	},
	[it8718] = {
		.name = "it8718",
		.features = FEAT_16BIT_FANS | FEAT_TEMP_OFFSET
		  | FEAT_TEMP_OLD_PECI,
		.old_peci_mask = 0x4,
	},
	[it8720] = {
		.name = "it8720",
		.features = FEAT_16BIT_FANS | FEAT_TEMP_OFFSET
		  | FEAT_TEMP_OLD_PECI,
		.old_peci_mask = 0x4,
	},
	[it8721] = {
		.name = "it8721",
		.features = FEAT_NEWER_AUTOPWM | FEAT_12MV_ADC | FEAT_16BIT_FANS
		  | FEAT_TEMP_OFFSET | FEAT_TEMP_OLD_PECI | FEAT_TEMP_PECI,
		.peci_mask = 0x05,
		.old_peci_mask = 0x02,	 
	},
	[it8728] = {
		.name = "it8728",
		.features = FEAT_NEWER_AUTOPWM | FEAT_12MV_ADC | FEAT_16BIT_FANS
		  | FEAT_TEMP_OFFSET | FEAT_TEMP_PECI,
		.peci_mask = 0x07,
	},
	[it8771] = {
		.name = "it8771",
		.features = FEAT_NEWER_AUTOPWM | FEAT_12MV_ADC | FEAT_16BIT_FANS
		  | FEAT_TEMP_OFFSET | FEAT_TEMP_PECI,
					 
		.peci_mask = 0x07,
	},
	[it8772] = {
		.name = "it8772",
		.features = FEAT_NEWER_AUTOPWM | FEAT_12MV_ADC | FEAT_16BIT_FANS
		  | FEAT_TEMP_OFFSET | FEAT_TEMP_PECI,
					 
		.peci_mask = 0x07,
	},
	[it8782] = {
		.name = "it8782",
		.features = FEAT_16BIT_FANS | FEAT_TEMP_OFFSET
		  | FEAT_TEMP_OLD_PECI,
		.old_peci_mask = 0x4,
	},
	[it8783] = {
		.name = "it8783",
		.features = FEAT_16BIT_FANS | FEAT_TEMP_OFFSET
		  | FEAT_TEMP_OLD_PECI,
		.old_peci_mask = 0x4,
	},
};

#define has_16bit_fans(data)	((data)->features & FEAT_16BIT_FANS)
#define has_12mv_adc(data)	((data)->features & FEAT_12MV_ADC)
#define has_newer_autopwm(data)	((data)->features & FEAT_NEWER_AUTOPWM)
#define has_old_autopwm(data)	((data)->features & FEAT_OLD_AUTOPWM)
#define has_temp_offset(data)	((data)->features & FEAT_TEMP_OFFSET)
#define has_temp_peci(data, nr)	(((data)->features & FEAT_TEMP_PECI) && \
				 ((data)->peci_mask & (1 << nr)))
#define has_temp_old_peci(data, nr) \
				(((data)->features & FEAT_TEMP_OLD_PECI) && \
				 ((data)->old_peci_mask & (1 << nr)))

struct it87_sio_data {
	enum chips type;
	 
	u8 revision;
	u8 vid_value;
	u8 beep_pin;
	u8 internal;	 
	 
	u16 skip_in;
	u8 skip_vid;
	u8 skip_fan;
	u8 skip_pwm;
	u8 skip_temp;
};

struct it87_data {
	struct device *hwmon_dev;
	enum chips type;
	u16 features;
	u8 peci_mask;
	u8 old_peci_mask;

	unsigned short addr;
	const char *name;
	struct mutex update_lock;
	char valid;		 
	unsigned long last_updated;	 

	u16 in_scaled;		 
	u8 in[9][3];		 
	u8 has_fan;		 
	u16 fan[5][2];		 
	u8 has_temp;		 
	s8 temp[3][4];		 
	u8 sensor;		 
	u8 extra;		 
	u8 fan_div[3];		 
	u8 vid;			 
	u8 vrm;
	u32 alarms;		 
	u8 beeps;		 
	u8 fan_main_ctrl;	 
	u8 fan_ctl;		 

	u8 pwm_ctrl[3];		 
	u8 pwm_duty[3];		 
	u8 pwm_temp_map[3];	 

	u8 auto_pwm[3][4];	 
	s8 auto_temp[3][5];	 
};

static int adc_lsb(const struct it87_data *data, int nr)
{
	int lsb = has_12mv_adc(data) ? 12 : 16;
	if (data->in_scaled & (1 << nr))
		lsb <<= 1;
	return lsb;
}

static u8 in_to_reg(const struct it87_data *data, int nr, long val)
{
	val = DIV_ROUND_CLOSEST(val, adc_lsb(data, nr));
	return clamp_val(val, 0, 255);
}

static int in_from_reg(const struct it87_data *data, int nr, int val)
{
	return val * adc_lsb(data, nr);
}

static inline u8 FAN_TO_REG(long rpm, int div)
{
	if (rpm == 0)
		return 255;
	rpm = clamp_val(rpm, 1, 1000000);
	return clamp_val((1350000 + rpm * div / 2) / (rpm * div), 1, 254);
}

static inline u16 FAN16_TO_REG(long rpm)
{
	if (rpm == 0)
		return 0xffff;
	return clamp_val((1350000 + rpm) / (rpm * 2), 1, 0xfffe);
}

#define FAN_FROM_REG(val, div) ((val) == 0 ? -1 : (val) == 255 ? 0 : \
				1350000 / ((val) * (div)))
 
#define FAN16_FROM_REG(val) ((val) == 0 ? -1 : (val) == 0xffff ? 0 : \
			     1350000 / ((val) * 2))

#define TEMP_TO_REG(val) (clamp_val(((val) < 0 ? (((val) - 500) / 1000) : \
				    ((val) + 500) / 1000), -128, 127))
#define TEMP_FROM_REG(val) ((val) * 1000)

static u8 pwm_to_reg(const struct it87_data *data, long val)
{
	if (has_newer_autopwm(data))
		return val;
	else
		return val >> 1;
}

static int pwm_from_reg(const struct it87_data *data, u8 reg)
{
	if (has_newer_autopwm(data))
		return reg;
	else
		return (reg & 0x7f) << 1;
}

static int DIV_TO_REG(int val)
{
	int answer = 0;
	while (answer < 7 && (val >>= 1))
		answer++;
	return answer;
}
#define DIV_FROM_REG(val) (1 << (val))

static const unsigned int pwm_freq[8] = {
	48000000 / 128,
	24000000 / 128,
	12000000 / 128,
	8000000 / 128,
	6000000 / 128,
	3000000 / 128,
	1500000 / 128,
	750000 / 128,
};

static int it87_probe(struct platform_device *pdev);
static int it87_remove(struct platform_device *pdev);

static int it87_read_value(struct it87_data *data, u8 reg);
static void it87_write_value(struct it87_data *data, u8 reg, u8 value);
static struct it87_data *it87_update_device(struct device *dev);
static int it87_check_pwm(struct device *dev);
static void it87_init_device(struct platform_device *pdev);

static struct platform_driver it87_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= DRVNAME,
	},
	.probe	= it87_probe,
	.remove	= it87_remove,
};

static ssize_t show_in(struct device *dev, struct device_attribute *attr,
		       char *buf)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", in_from_reg(data, nr, data->in[nr][index]));
}

static ssize_t set_in(struct device *dev, struct device_attribute *attr,
		      const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->in[nr][index] = in_to_reg(data, nr, val);
	it87_write_value(data,
			 index == 1 ? IT87_REG_VIN_MIN(nr)
				    : IT87_REG_VIN_MAX(nr),
			 data->in[nr][index]);
	mutex_unlock(&data->update_lock);
	return count;
}

static SENSOR_DEVICE_ATTR_2(in0_input, S_IRUGO, show_in, NULL, 0, 0);
static SENSOR_DEVICE_ATTR_2(in0_min, S_IRUGO | S_IWUSR, show_in, set_in,
			    0, 1);
static SENSOR_DEVICE_ATTR_2(in0_max, S_IRUGO | S_IWUSR, show_in, set_in,
			    0, 2);

static SENSOR_DEVICE_ATTR_2(in1_input, S_IRUGO, show_in, NULL, 1, 0);
static SENSOR_DEVICE_ATTR_2(in1_min, S_IRUGO | S_IWUSR, show_in, set_in,
			    1, 1);
static SENSOR_DEVICE_ATTR_2(in1_max, S_IRUGO | S_IWUSR, show_in, set_in,
			    1, 2);

static SENSOR_DEVICE_ATTR_2(in2_input, S_IRUGO, show_in, NULL, 2, 0);
static SENSOR_DEVICE_ATTR_2(in2_min, S_IRUGO | S_IWUSR, show_in, set_in,
			    2, 1);
static SENSOR_DEVICE_ATTR_2(in2_max, S_IRUGO | S_IWUSR, show_in, set_in,
			    2, 2);

static SENSOR_DEVICE_ATTR_2(in3_input, S_IRUGO, show_in, NULL, 3, 0);
static SENSOR_DEVICE_ATTR_2(in3_min, S_IRUGO | S_IWUSR, show_in, set_in,
			    3, 1);
static SENSOR_DEVICE_ATTR_2(in3_max, S_IRUGO | S_IWUSR, show_in, set_in,
			    3, 2);

static SENSOR_DEVICE_ATTR_2(in4_input, S_IRUGO, show_in, NULL, 4, 0);
static SENSOR_DEVICE_ATTR_2(in4_min, S_IRUGO | S_IWUSR, show_in, set_in,
			    4, 1);
static SENSOR_DEVICE_ATTR_2(in4_max, S_IRUGO | S_IWUSR, show_in, set_in,
			    4, 2);

static SENSOR_DEVICE_ATTR_2(in5_input, S_IRUGO, show_in, NULL, 5, 0);
static SENSOR_DEVICE_ATTR_2(in5_min, S_IRUGO | S_IWUSR, show_in, set_in,
			    5, 1);
static SENSOR_DEVICE_ATTR_2(in5_max, S_IRUGO | S_IWUSR, show_in, set_in,
			    5, 2);

static SENSOR_DEVICE_ATTR_2(in6_input, S_IRUGO, show_in, NULL, 6, 0);
static SENSOR_DEVICE_ATTR_2(in6_min, S_IRUGO | S_IWUSR, show_in, set_in,
			    6, 1);
static SENSOR_DEVICE_ATTR_2(in6_max, S_IRUGO | S_IWUSR, show_in, set_in,
			    6, 2);

static SENSOR_DEVICE_ATTR_2(in7_input, S_IRUGO, show_in, NULL, 7, 0);
static SENSOR_DEVICE_ATTR_2(in7_min, S_IRUGO | S_IWUSR, show_in, set_in,
			    7, 1);
static SENSOR_DEVICE_ATTR_2(in7_max, S_IRUGO | S_IWUSR, show_in, set_in,
			    7, 2);

static SENSOR_DEVICE_ATTR_2(in8_input, S_IRUGO, show_in, NULL, 8, 0);

static ssize_t show_temp(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	struct it87_data *data = it87_update_device(dev);

	return sprintf(buf, "%d\n", TEMP_FROM_REG(data->temp[nr][index]));
}

static ssize_t set_temp(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	struct it87_data *data = dev_get_drvdata(dev);
	long val;
	u8 reg, regval;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);

	switch (index) {
	default:
	case 1:
		reg = IT87_REG_TEMP_LOW(nr);
		break;
	case 2:
		reg = IT87_REG_TEMP_HIGH(nr);
		break;
	case 3:
		regval = it87_read_value(data, IT87_REG_BEEP_ENABLE);
		if (!(regval & 0x80)) {
			regval |= 0x80;
			it87_write_value(data, IT87_REG_BEEP_ENABLE, regval);
		}
		data->valid = 0;
		reg = IT87_REG_TEMP_OFFSET[nr];
		break;
	}

	data->temp[nr][index] = TEMP_TO_REG(val);
	it87_write_value(data, reg, data->temp[nr][index]);
	mutex_unlock(&data->update_lock);
	return count;
}

static SENSOR_DEVICE_ATTR_2(temp1_input, S_IRUGO, show_temp, NULL, 0, 0);
static SENSOR_DEVICE_ATTR_2(temp1_min, S_IRUGO | S_IWUSR, show_temp, set_temp,
			    0, 1);
static SENSOR_DEVICE_ATTR_2(temp1_max, S_IRUGO | S_IWUSR, show_temp, set_temp,
			    0, 2);
static SENSOR_DEVICE_ATTR_2(temp1_offset, S_IRUGO | S_IWUSR, show_temp,
			    set_temp, 0, 3);
static SENSOR_DEVICE_ATTR_2(temp2_input, S_IRUGO, show_temp, NULL, 1, 0);
static SENSOR_DEVICE_ATTR_2(temp2_min, S_IRUGO | S_IWUSR, show_temp, set_temp,
			    1, 1);
static SENSOR_DEVICE_ATTR_2(temp2_max, S_IRUGO | S_IWUSR, show_temp, set_temp,
			    1, 2);
static SENSOR_DEVICE_ATTR_2(temp2_offset, S_IRUGO | S_IWUSR, show_temp,
			    set_temp, 1, 3);
static SENSOR_DEVICE_ATTR_2(temp3_input, S_IRUGO, show_temp, NULL, 2, 0);
static SENSOR_DEVICE_ATTR_2(temp3_min, S_IRUGO | S_IWUSR, show_temp, set_temp,
			    2, 1);
static SENSOR_DEVICE_ATTR_2(temp3_max, S_IRUGO | S_IWUSR, show_temp, set_temp,
			    2, 2);
static SENSOR_DEVICE_ATTR_2(temp3_offset, S_IRUGO | S_IWUSR, show_temp,
			    set_temp, 2, 3);

static ssize_t show_temp_type(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;
	struct it87_data *data = it87_update_device(dev);
	u8 reg = data->sensor;	     
	u8 extra = data->extra;

	if ((has_temp_peci(data, nr) && (reg >> 6 == nr + 1))
	    || (has_temp_old_peci(data, nr) && (extra & 0x80)))
		return sprintf(buf, "6\n");   
	if (reg & (1 << nr))
		return sprintf(buf, "3\n");   
	if (reg & (8 << nr))
		return sprintf(buf, "4\n");   
	return sprintf(buf, "0\n");       
}

static ssize_t set_temp_type(struct device *dev, struct device_attribute *attr,
			     const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	long val;
	u8 reg, extra;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	reg = it87_read_value(data, IT87_REG_TEMP_ENABLE);
	reg &= ~(1 << nr);
	reg &= ~(8 << nr);
	if (has_temp_peci(data, nr) && (reg >> 6 == nr + 1 || val == 6))
		reg &= 0x3f;
	extra = it87_read_value(data, IT87_REG_TEMP_EXTRA);
	if (has_temp_old_peci(data, nr) && ((extra & 0x80) || val == 6))
		extra &= 0x7f;
	if (val == 2) {	 
		dev_warn(dev,
			 "Sensor type 2 is deprecated, please use 4 instead\n");
		val = 4;
	}
	 
	if (val == 3)
		reg |= 1 << nr;
	else if (val == 4)
		reg |= 8 << nr;
	else if (has_temp_peci(data, nr) && val == 6)
		reg |= (nr + 1) << 6;
	else if (has_temp_old_peci(data, nr) && val == 6)
		extra |= 0x80;
	else if (val != 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->sensor = reg;
	data->extra = extra;
	it87_write_value(data, IT87_REG_TEMP_ENABLE, data->sensor);
	if (has_temp_old_peci(data, nr))
		it87_write_value(data, IT87_REG_TEMP_EXTRA, data->extra);
	data->valid = 0;	 
	mutex_unlock(&data->update_lock);
	return count;
}

static SENSOR_DEVICE_ATTR(temp1_type, S_IRUGO | S_IWUSR, show_temp_type,
			  set_temp_type, 0);
static SENSOR_DEVICE_ATTR(temp2_type, S_IRUGO | S_IWUSR, show_temp_type,
			  set_temp_type, 1);
static SENSOR_DEVICE_ATTR(temp3_type, S_IRUGO | S_IWUSR, show_temp_type,
			  set_temp_type, 2);

static int pwm_mode(const struct it87_data *data, int nr)
{
	int ctrl = data->fan_main_ctrl & (1 << nr);

	if (ctrl == 0)					 
		return 0;
	if (data->pwm_ctrl[nr] & 0x80)			 
		return 2;
	else						 
		return 1;
}

static ssize_t show_fan(struct device *dev, struct device_attribute *attr,
			char *buf)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;
	int speed;
	struct it87_data *data = it87_update_device(dev);

	speed = has_16bit_fans(data) ?
		FAN16_FROM_REG(data->fan[nr][index]) :
		FAN_FROM_REG(data->fan[nr][index],
			     DIV_FROM_REG(data->fan_div[nr]));
	return sprintf(buf, "%d\n", speed);
}

static ssize_t show_fan_div(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", DIV_FROM_REG(data->fan_div[nr]));
}
static ssize_t show_pwm_enable(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", pwm_mode(data, nr));
}
static ssize_t show_pwm(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n",
		       pwm_from_reg(data, data->pwm_duty[nr]));
}
static ssize_t show_pwm_freq(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct it87_data *data = it87_update_device(dev);
	int index = (data->fan_ctl >> 4) & 0x07;

	return sprintf(buf, "%u\n", pwm_freq[index]);
}

static ssize_t set_fan(struct device *dev, struct device_attribute *attr,
		       const char *buf, size_t count)
{
	struct sensor_device_attribute_2 *sattr = to_sensor_dev_attr_2(attr);
	int nr = sattr->nr;
	int index = sattr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	long val;
	u8 reg;

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);

	if (has_16bit_fans(data)) {
		data->fan[nr][index] = FAN16_TO_REG(val);
		it87_write_value(data, IT87_REG_FAN_MIN[nr],
				 data->fan[nr][index] & 0xff);
		it87_write_value(data, IT87_REG_FANX_MIN[nr],
				 data->fan[nr][index] >> 8);
	} else {
		reg = it87_read_value(data, IT87_REG_FAN_DIV);
		switch (nr) {
		case 0:
			data->fan_div[nr] = reg & 0x07;
			break;
		case 1:
			data->fan_div[nr] = (reg >> 3) & 0x07;
			break;
		case 2:
			data->fan_div[nr] = (reg & 0x40) ? 3 : 1;
			break;
		}
		data->fan[nr][index] =
		  FAN_TO_REG(val, DIV_FROM_REG(data->fan_div[nr]));
		it87_write_value(data, IT87_REG_FAN_MIN[nr],
				 data->fan[nr][index]);
	}

	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t set_fan_div(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int min;
	u8 old;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	old = it87_read_value(data, IT87_REG_FAN_DIV);

	min = FAN_FROM_REG(data->fan[nr][1], DIV_FROM_REG(data->fan_div[nr]));

	switch (nr) {
	case 0:
	case 1:
		data->fan_div[nr] = DIV_TO_REG(val);
		break;
	case 2:
		if (val < 8)
			data->fan_div[nr] = 1;
		else
			data->fan_div[nr] = 3;
	}
	val = old & 0x80;
	val |= (data->fan_div[0] & 0x07);
	val |= (data->fan_div[1] & 0x07) << 3;
	if (data->fan_div[2] == 3)
		val |= 0x1 << 6;
	it87_write_value(data, IT87_REG_FAN_DIV, val);

	data->fan[nr][1] = FAN_TO_REG(min, DIV_FROM_REG(data->fan_div[nr]));
	it87_write_value(data, IT87_REG_FAN_MIN[nr], data->fan[nr][1]);

	mutex_unlock(&data->update_lock);
	return count;
}

static int check_trip_points(struct device *dev, int nr)
{
	const struct it87_data *data = dev_get_drvdata(dev);
	int i, err = 0;

	if (has_old_autopwm(data)) {
		for (i = 0; i < 3; i++) {
			if (data->auto_temp[nr][i] > data->auto_temp[nr][i + 1])
				err = -EINVAL;
		}
		for (i = 0; i < 2; i++) {
			if (data->auto_pwm[nr][i] > data->auto_pwm[nr][i + 1])
				err = -EINVAL;
		}
	}

	if (err) {
		dev_err(dev,
			"Inconsistent trip points, not switching to automatic mode\n");
		dev_err(dev, "Adjust the trip points and try again\n");
	}
	return err;
}

static ssize_t set_pwm_enable(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	long val;

	if (kstrtol(buf, 10, &val) < 0 || val < 0 || val > 2)
		return -EINVAL;

	if (val == 2) {
		if (check_trip_points(dev, nr) < 0)
			return -EINVAL;
	}

	mutex_lock(&data->update_lock);

	if (val == 0) {
		int tmp;
		 
		tmp = it87_read_value(data, IT87_REG_FAN_CTL);
		it87_write_value(data, IT87_REG_FAN_CTL, tmp | (1 << nr));
		 
		data->fan_main_ctrl &= ~(1 << nr);
		it87_write_value(data, IT87_REG_FAN_MAIN_CTRL,
				 data->fan_main_ctrl);
	} else {
		if (val == 1)				 
			data->pwm_ctrl[nr] = has_newer_autopwm(data) ?
					     data->pwm_temp_map[nr] :
					     data->pwm_duty[nr];
		else					 
			data->pwm_ctrl[nr] = 0x80 | data->pwm_temp_map[nr];
		it87_write_value(data, IT87_REG_PWM(nr), data->pwm_ctrl[nr]);
		 
		data->fan_main_ctrl |= (1 << nr);
		it87_write_value(data, IT87_REG_FAN_MAIN_CTRL,
				 data->fan_main_ctrl);
	}

	mutex_unlock(&data->update_lock);
	return count;
}
static ssize_t set_pwm(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	long val;

	if (kstrtol(buf, 10, &val) < 0 || val < 0 || val > 255)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	if (has_newer_autopwm(data)) {
		 
		if (data->pwm_ctrl[nr] & 0x80) {
			mutex_unlock(&data->update_lock);
			return -EBUSY;
		}
		data->pwm_duty[nr] = pwm_to_reg(data, val);
		it87_write_value(data, IT87_REG_PWM_DUTY(nr),
				 data->pwm_duty[nr]);
	} else {
		data->pwm_duty[nr] = pwm_to_reg(data, val);
		 
		if (!(data->pwm_ctrl[nr] & 0x80)) {
			data->pwm_ctrl[nr] = data->pwm_duty[nr];
			it87_write_value(data, IT87_REG_PWM(nr),
					 data->pwm_ctrl[nr]);
		}
	}
	mutex_unlock(&data->update_lock);
	return count;
}
static ssize_t set_pwm_freq(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct it87_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int i;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;

	for (i = 0; i < 7; i++) {
		if (val > (pwm_freq[i] + pwm_freq[i+1]) / 2)
			break;
	}

	mutex_lock(&data->update_lock);
	data->fan_ctl = it87_read_value(data, IT87_REG_FAN_CTL) & 0x8f;
	data->fan_ctl |= i << 4;
	it87_write_value(data, IT87_REG_FAN_CTL, data->fan_ctl);
	mutex_unlock(&data->update_lock);

	return count;
}
static ssize_t show_pwm_temp_map(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	int map;

	if (data->pwm_temp_map[nr] < 3)
		map = 1 << data->pwm_temp_map[nr];
	else
		map = 0;			 
	return sprintf(buf, "%d\n", map);
}
static ssize_t set_pwm_temp_map(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	long val;
	u8 reg;

	if (!has_old_autopwm(data)) {
		dev_notice(dev, "Mapping change disabled for safety reasons\n");
		return -EINVAL;
	}

	if (kstrtol(buf, 10, &val) < 0)
		return -EINVAL;

	switch (val) {
	case (1 << 0):
		reg = 0x00;
		break;
	case (1 << 1):
		reg = 0x01;
		break;
	case (1 << 2):
		reg = 0x02;
		break;
	default:
		return -EINVAL;
	}

	mutex_lock(&data->update_lock);
	data->pwm_temp_map[nr] = reg;
	 
	if (data->pwm_ctrl[nr] & 0x80) {
		data->pwm_ctrl[nr] = 0x80 | data->pwm_temp_map[nr];
		it87_write_value(data, IT87_REG_PWM(nr), data->pwm_ctrl[nr]);
	}
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_auto_pwm(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct it87_data *data = it87_update_device(dev);
	struct sensor_device_attribute_2 *sensor_attr =
			to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int point = sensor_attr->index;

	return sprintf(buf, "%d\n",
		       pwm_from_reg(data, data->auto_pwm[nr][point]));
}

static ssize_t set_auto_pwm(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct it87_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sensor_attr =
			to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int point = sensor_attr->index;
	long val;

	if (kstrtol(buf, 10, &val) < 0 || val < 0 || val > 255)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->auto_pwm[nr][point] = pwm_to_reg(data, val);
	it87_write_value(data, IT87_REG_AUTO_PWM(nr, point),
			 data->auto_pwm[nr][point]);
	mutex_unlock(&data->update_lock);
	return count;
}

static ssize_t show_auto_temp(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct it87_data *data = it87_update_device(dev);
	struct sensor_device_attribute_2 *sensor_attr =
			to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int point = sensor_attr->index;

	return sprintf(buf, "%d\n", TEMP_FROM_REG(data->auto_temp[nr][point]));
}

static ssize_t set_auto_temp(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct it87_data *data = dev_get_drvdata(dev);
	struct sensor_device_attribute_2 *sensor_attr =
			to_sensor_dev_attr_2(attr);
	int nr = sensor_attr->nr;
	int point = sensor_attr->index;
	long val;

	if (kstrtol(buf, 10, &val) < 0 || val < -128000 || val > 127000)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->auto_temp[nr][point] = TEMP_TO_REG(val);
	it87_write_value(data, IT87_REG_AUTO_TEMP(nr, point),
			 data->auto_temp[nr][point]);
	mutex_unlock(&data->update_lock);
	return count;
}

static SENSOR_DEVICE_ATTR_2(fan1_input, S_IRUGO, show_fan, NULL, 0, 0);
static SENSOR_DEVICE_ATTR_2(fan1_min, S_IRUGO | S_IWUSR, show_fan, set_fan,
			    0, 1);
static SENSOR_DEVICE_ATTR(fan1_div, S_IRUGO | S_IWUSR, show_fan_div,
			  set_fan_div, 0);

static SENSOR_DEVICE_ATTR_2(fan2_input, S_IRUGO, show_fan, NULL, 1, 0);
static SENSOR_DEVICE_ATTR_2(fan2_min, S_IRUGO | S_IWUSR, show_fan, set_fan,
			    1, 1);
static SENSOR_DEVICE_ATTR(fan2_div, S_IRUGO | S_IWUSR, show_fan_div,
			  set_fan_div, 1);

static SENSOR_DEVICE_ATTR_2(fan3_input, S_IRUGO, show_fan, NULL, 2, 0);
static SENSOR_DEVICE_ATTR_2(fan3_min, S_IRUGO | S_IWUSR, show_fan, set_fan,
			    2, 1);
static SENSOR_DEVICE_ATTR(fan3_div, S_IRUGO | S_IWUSR, show_fan_div,
			  set_fan_div, 2);

static SENSOR_DEVICE_ATTR_2(fan4_input, S_IRUGO, show_fan, NULL, 3, 0);
static SENSOR_DEVICE_ATTR_2(fan4_min, S_IRUGO | S_IWUSR, show_fan, set_fan,
			    3, 1);

static SENSOR_DEVICE_ATTR_2(fan5_input, S_IRUGO, show_fan, NULL, 4, 0);
static SENSOR_DEVICE_ATTR_2(fan5_min, S_IRUGO | S_IWUSR, show_fan, set_fan,
			    4, 1);

static SENSOR_DEVICE_ATTR(pwm1_enable, S_IRUGO | S_IWUSR,
			  show_pwm_enable, set_pwm_enable, 0);
static SENSOR_DEVICE_ATTR(pwm1, S_IRUGO | S_IWUSR, show_pwm, set_pwm, 0);
static DEVICE_ATTR(pwm1_freq, S_IRUGO | S_IWUSR, show_pwm_freq, set_pwm_freq);
static SENSOR_DEVICE_ATTR(pwm1_auto_channels_temp, S_IRUGO | S_IWUSR,
			  show_pwm_temp_map, set_pwm_temp_map, 0);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point1_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 0, 0);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point2_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 0, 1);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point3_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 0, 2);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point4_pwm, S_IRUGO,
			    show_auto_pwm, NULL, 0, 3);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point1_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 0, 1);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point1_temp_hyst, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 0, 0);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point2_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 0, 2);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point3_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 0, 3);
static SENSOR_DEVICE_ATTR_2(pwm1_auto_point4_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 0, 4);

static SENSOR_DEVICE_ATTR(pwm2_enable, S_IRUGO | S_IWUSR,
			  show_pwm_enable, set_pwm_enable, 1);
static SENSOR_DEVICE_ATTR(pwm2, S_IRUGO | S_IWUSR, show_pwm, set_pwm, 1);
static DEVICE_ATTR(pwm2_freq, S_IRUGO, show_pwm_freq, NULL);
static SENSOR_DEVICE_ATTR(pwm2_auto_channels_temp, S_IRUGO | S_IWUSR,
			  show_pwm_temp_map, set_pwm_temp_map, 1);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point1_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 1, 0);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point2_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 1, 1);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point3_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 1, 2);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point4_pwm, S_IRUGO,
			    show_auto_pwm, NULL, 1, 3);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point1_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 1, 1);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point1_temp_hyst, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 1, 0);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point2_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 1, 2);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point3_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 1, 3);
static SENSOR_DEVICE_ATTR_2(pwm2_auto_point4_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 1, 4);

static SENSOR_DEVICE_ATTR(pwm3_enable, S_IRUGO | S_IWUSR,
			  show_pwm_enable, set_pwm_enable, 2);
static SENSOR_DEVICE_ATTR(pwm3, S_IRUGO | S_IWUSR, show_pwm, set_pwm, 2);
static DEVICE_ATTR(pwm3_freq, S_IRUGO, show_pwm_freq, NULL);
static SENSOR_DEVICE_ATTR(pwm3_auto_channels_temp, S_IRUGO | S_IWUSR,
			  show_pwm_temp_map, set_pwm_temp_map, 2);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point1_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 2, 0);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point2_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 2, 1);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point3_pwm, S_IRUGO | S_IWUSR,
			    show_auto_pwm, set_auto_pwm, 2, 2);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point4_pwm, S_IRUGO,
			    show_auto_pwm, NULL, 2, 3);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point1_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 2, 1);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point1_temp_hyst, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 2, 0);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point2_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 2, 2);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point3_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 2, 3);
static SENSOR_DEVICE_ATTR_2(pwm3_auto_point4_temp, S_IRUGO | S_IWUSR,
			    show_auto_temp, set_auto_temp, 2, 4);

static ssize_t show_alarms(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%u\n", data->alarms);
}
static DEVICE_ATTR(alarms, S_IRUGO, show_alarms, NULL);

static ssize_t show_alarm(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int bitnr = to_sensor_dev_attr(attr)->index;
	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%u\n", (data->alarms >> bitnr) & 1);
}

static ssize_t clear_intrusion(struct device *dev, struct device_attribute
		*attr, const char *buf, size_t count)
{
	struct it87_data *data = dev_get_drvdata(dev);
	long val;
	int config;

	if (kstrtol(buf, 10, &val) < 0 || val != 0)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	config = it87_read_value(data, IT87_REG_CONFIG);
	if (config < 0) {
		count = config;
	} else {
		config |= 1 << 5;
		it87_write_value(data, IT87_REG_CONFIG, config);
		 
		data->valid = 0;
	}
	mutex_unlock(&data->update_lock);

	return count;
}

static SENSOR_DEVICE_ATTR(in0_alarm, S_IRUGO, show_alarm, NULL, 8);
static SENSOR_DEVICE_ATTR(in1_alarm, S_IRUGO, show_alarm, NULL, 9);
static SENSOR_DEVICE_ATTR(in2_alarm, S_IRUGO, show_alarm, NULL, 10);
static SENSOR_DEVICE_ATTR(in3_alarm, S_IRUGO, show_alarm, NULL, 11);
static SENSOR_DEVICE_ATTR(in4_alarm, S_IRUGO, show_alarm, NULL, 12);
static SENSOR_DEVICE_ATTR(in5_alarm, S_IRUGO, show_alarm, NULL, 13);
static SENSOR_DEVICE_ATTR(in6_alarm, S_IRUGO, show_alarm, NULL, 14);
static SENSOR_DEVICE_ATTR(in7_alarm, S_IRUGO, show_alarm, NULL, 15);
static SENSOR_DEVICE_ATTR(fan1_alarm, S_IRUGO, show_alarm, NULL, 0);
static SENSOR_DEVICE_ATTR(fan2_alarm, S_IRUGO, show_alarm, NULL, 1);
static SENSOR_DEVICE_ATTR(fan3_alarm, S_IRUGO, show_alarm, NULL, 2);
static SENSOR_DEVICE_ATTR(fan4_alarm, S_IRUGO, show_alarm, NULL, 3);
static SENSOR_DEVICE_ATTR(fan5_alarm, S_IRUGO, show_alarm, NULL, 6);
static SENSOR_DEVICE_ATTR(temp1_alarm, S_IRUGO, show_alarm, NULL, 16);
static SENSOR_DEVICE_ATTR(temp2_alarm, S_IRUGO, show_alarm, NULL, 17);
static SENSOR_DEVICE_ATTR(temp3_alarm, S_IRUGO, show_alarm, NULL, 18);
static SENSOR_DEVICE_ATTR(intrusion0_alarm, S_IRUGO | S_IWUSR,
			  show_alarm, clear_intrusion, 4);

static ssize_t show_beep(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	int bitnr = to_sensor_dev_attr(attr)->index;
	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%u\n", (data->beeps >> bitnr) & 1);
}
static ssize_t set_beep(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int bitnr = to_sensor_dev_attr(attr)->index;
	struct it87_data *data = dev_get_drvdata(dev);
	long val;

	if (kstrtol(buf, 10, &val) < 0
	 || (val != 0 && val != 1))
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->beeps = it87_read_value(data, IT87_REG_BEEP_ENABLE);
	if (val)
		data->beeps |= (1 << bitnr);
	else
		data->beeps &= ~(1 << bitnr);
	it87_write_value(data, IT87_REG_BEEP_ENABLE, data->beeps);
	mutex_unlock(&data->update_lock);
	return count;
}

static SENSOR_DEVICE_ATTR(in0_beep, S_IRUGO | S_IWUSR,
			  show_beep, set_beep, 1);
static SENSOR_DEVICE_ATTR(in1_beep, S_IRUGO, show_beep, NULL, 1);
static SENSOR_DEVICE_ATTR(in2_beep, S_IRUGO, show_beep, NULL, 1);
static SENSOR_DEVICE_ATTR(in3_beep, S_IRUGO, show_beep, NULL, 1);
static SENSOR_DEVICE_ATTR(in4_beep, S_IRUGO, show_beep, NULL, 1);
static SENSOR_DEVICE_ATTR(in5_beep, S_IRUGO, show_beep, NULL, 1);
static SENSOR_DEVICE_ATTR(in6_beep, S_IRUGO, show_beep, NULL, 1);
static SENSOR_DEVICE_ATTR(in7_beep, S_IRUGO, show_beep, NULL, 1);
 
static SENSOR_DEVICE_ATTR(fan1_beep, S_IRUGO, show_beep, set_beep, 0);
static SENSOR_DEVICE_ATTR(fan2_beep, S_IRUGO, show_beep, set_beep, 0);
static SENSOR_DEVICE_ATTR(fan3_beep, S_IRUGO, show_beep, set_beep, 0);
static SENSOR_DEVICE_ATTR(fan4_beep, S_IRUGO, show_beep, set_beep, 0);
static SENSOR_DEVICE_ATTR(fan5_beep, S_IRUGO, show_beep, set_beep, 0);
static SENSOR_DEVICE_ATTR(temp1_beep, S_IRUGO | S_IWUSR,
			  show_beep, set_beep, 2);
static SENSOR_DEVICE_ATTR(temp2_beep, S_IRUGO, show_beep, NULL, 2);
static SENSOR_DEVICE_ATTR(temp3_beep, S_IRUGO, show_beep, NULL, 2);

static ssize_t show_vrm_reg(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct it87_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%u\n", data->vrm);
}
static ssize_t store_vrm_reg(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct it87_data *data = dev_get_drvdata(dev);
	unsigned long val;

	if (kstrtoul(buf, 10, &val) < 0)
		return -EINVAL;

	data->vrm = val;

	return count;
}
static DEVICE_ATTR(vrm, S_IRUGO | S_IWUSR, show_vrm_reg, store_vrm_reg);

static ssize_t show_vid_reg(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%ld\n", (long) vid_from_reg(data->vid, data->vrm));
}
static DEVICE_ATTR(cpu0_vid, S_IRUGO, show_vid_reg, NULL);

static ssize_t show_label(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	static const char * const labels[] = {
		"+5V",
		"5VSB",
		"Vbat",
	};
	static const char * const labels_it8721[] = {
		"+3.3V",
		"3VSB",
		"Vbat",
	};
	struct it87_data *data = dev_get_drvdata(dev);
	int nr = to_sensor_dev_attr(attr)->index;

	return sprintf(buf, "%s\n", has_12mv_adc(data) ? labels_it8721[nr]
						       : labels[nr]);
}
static SENSOR_DEVICE_ATTR(in3_label, S_IRUGO, show_label, NULL, 0);
static SENSOR_DEVICE_ATTR(in7_label, S_IRUGO, show_label, NULL, 1);
static SENSOR_DEVICE_ATTR(in8_label, S_IRUGO, show_label, NULL, 2);

static ssize_t show_name(struct device *dev, struct device_attribute
			 *devattr, char *buf)
{
	struct it87_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", data->name);
}
static DEVICE_ATTR(name, S_IRUGO, show_name, NULL);

static struct attribute *it87_attributes_in[9][5] = {
{
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in0_min.dev_attr.attr,
	&sensor_dev_attr_in0_max.dev_attr.attr,
	&sensor_dev_attr_in0_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in1_min.dev_attr.attr,
	&sensor_dev_attr_in1_max.dev_attr.attr,
	&sensor_dev_attr_in1_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in2_min.dev_attr.attr,
	&sensor_dev_attr_in2_max.dev_attr.attr,
	&sensor_dev_attr_in2_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in3_min.dev_attr.attr,
	&sensor_dev_attr_in3_max.dev_attr.attr,
	&sensor_dev_attr_in3_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_in4_input.dev_attr.attr,
	&sensor_dev_attr_in4_min.dev_attr.attr,
	&sensor_dev_attr_in4_max.dev_attr.attr,
	&sensor_dev_attr_in4_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_in5_input.dev_attr.attr,
	&sensor_dev_attr_in5_min.dev_attr.attr,
	&sensor_dev_attr_in5_max.dev_attr.attr,
	&sensor_dev_attr_in5_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_in6_input.dev_attr.attr,
	&sensor_dev_attr_in6_min.dev_attr.attr,
	&sensor_dev_attr_in6_max.dev_attr.attr,
	&sensor_dev_attr_in6_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_in7_input.dev_attr.attr,
	&sensor_dev_attr_in7_min.dev_attr.attr,
	&sensor_dev_attr_in7_max.dev_attr.attr,
	&sensor_dev_attr_in7_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_in8_input.dev_attr.attr,
	NULL
} };

static const struct attribute_group it87_group_in[9] = {
	{ .attrs = it87_attributes_in[0] },
	{ .attrs = it87_attributes_in[1] },
	{ .attrs = it87_attributes_in[2] },
	{ .attrs = it87_attributes_in[3] },
	{ .attrs = it87_attributes_in[4] },
	{ .attrs = it87_attributes_in[5] },
	{ .attrs = it87_attributes_in[6] },
	{ .attrs = it87_attributes_in[7] },
	{ .attrs = it87_attributes_in[8] },
};

static struct attribute *it87_attributes_temp[3][6] = {
{
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp1_min.dev_attr.attr,
	&sensor_dev_attr_temp1_type.dev_attr.attr,
	&sensor_dev_attr_temp1_alarm.dev_attr.attr,
	NULL
} , {
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp2_max.dev_attr.attr,
	&sensor_dev_attr_temp2_min.dev_attr.attr,
	&sensor_dev_attr_temp2_type.dev_attr.attr,
	&sensor_dev_attr_temp2_alarm.dev_attr.attr,
	NULL
} , {
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_temp3_max.dev_attr.attr,
	&sensor_dev_attr_temp3_min.dev_attr.attr,
	&sensor_dev_attr_temp3_type.dev_attr.attr,
	&sensor_dev_attr_temp3_alarm.dev_attr.attr,
	NULL
} };

static const struct attribute_group it87_group_temp[3] = {
	{ .attrs = it87_attributes_temp[0] },
	{ .attrs = it87_attributes_temp[1] },
	{ .attrs = it87_attributes_temp[2] },
};

static struct attribute *it87_attributes_temp_offset[] = {
	&sensor_dev_attr_temp1_offset.dev_attr.attr,
	&sensor_dev_attr_temp2_offset.dev_attr.attr,
	&sensor_dev_attr_temp3_offset.dev_attr.attr,
};

static struct attribute *it87_attributes[] = {
	&dev_attr_alarms.attr,
	&sensor_dev_attr_intrusion0_alarm.dev_attr.attr,
	&dev_attr_name.attr,
	NULL
};

static const struct attribute_group it87_group = {
	.attrs = it87_attributes,
};

static struct attribute *it87_attributes_in_beep[] = {
	&sensor_dev_attr_in0_beep.dev_attr.attr,
	&sensor_dev_attr_in1_beep.dev_attr.attr,
	&sensor_dev_attr_in2_beep.dev_attr.attr,
	&sensor_dev_attr_in3_beep.dev_attr.attr,
	&sensor_dev_attr_in4_beep.dev_attr.attr,
	&sensor_dev_attr_in5_beep.dev_attr.attr,
	&sensor_dev_attr_in6_beep.dev_attr.attr,
	&sensor_dev_attr_in7_beep.dev_attr.attr,
	NULL
};

static struct attribute *it87_attributes_temp_beep[] = {
	&sensor_dev_attr_temp1_beep.dev_attr.attr,
	&sensor_dev_attr_temp2_beep.dev_attr.attr,
	&sensor_dev_attr_temp3_beep.dev_attr.attr,
};

static struct attribute *it87_attributes_fan[5][3+1] = { {
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan1_min.dev_attr.attr,
	&sensor_dev_attr_fan1_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_fan2_min.dev_attr.attr,
	&sensor_dev_attr_fan2_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_fan3_input.dev_attr.attr,
	&sensor_dev_attr_fan3_min.dev_attr.attr,
	&sensor_dev_attr_fan3_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_fan4_input.dev_attr.attr,
	&sensor_dev_attr_fan4_min.dev_attr.attr,
	&sensor_dev_attr_fan4_alarm.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_fan5_input.dev_attr.attr,
	&sensor_dev_attr_fan5_min.dev_attr.attr,
	&sensor_dev_attr_fan5_alarm.dev_attr.attr,
	NULL
} };

static const struct attribute_group it87_group_fan[5] = {
	{ .attrs = it87_attributes_fan[0] },
	{ .attrs = it87_attributes_fan[1] },
	{ .attrs = it87_attributes_fan[2] },
	{ .attrs = it87_attributes_fan[3] },
	{ .attrs = it87_attributes_fan[4] },
};

static const struct attribute *it87_attributes_fan_div[] = {
	&sensor_dev_attr_fan1_div.dev_attr.attr,
	&sensor_dev_attr_fan2_div.dev_attr.attr,
	&sensor_dev_attr_fan3_div.dev_attr.attr,
};

static struct attribute *it87_attributes_pwm[3][4+1] = { {
	&sensor_dev_attr_pwm1_enable.dev_attr.attr,
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&dev_attr_pwm1_freq.attr,
	&sensor_dev_attr_pwm1_auto_channels_temp.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_pwm2_enable.dev_attr.attr,
	&sensor_dev_attr_pwm2.dev_attr.attr,
	&dev_attr_pwm2_freq.attr,
	&sensor_dev_attr_pwm2_auto_channels_temp.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_pwm3_enable.dev_attr.attr,
	&sensor_dev_attr_pwm3.dev_attr.attr,
	&dev_attr_pwm3_freq.attr,
	&sensor_dev_attr_pwm3_auto_channels_temp.dev_attr.attr,
	NULL
} };

static const struct attribute_group it87_group_pwm[3] = {
	{ .attrs = it87_attributes_pwm[0] },
	{ .attrs = it87_attributes_pwm[1] },
	{ .attrs = it87_attributes_pwm[2] },
};

static struct attribute *it87_attributes_autopwm[3][9+1] = { {
	&sensor_dev_attr_pwm1_auto_point1_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point2_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point3_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point4_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point1_temp.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point1_temp_hyst.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point2_temp.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point3_temp.dev_attr.attr,
	&sensor_dev_attr_pwm1_auto_point4_temp.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_pwm2_auto_point1_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm2_auto_point2_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm2_auto_point3_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm2_auto_point4_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm2_auto_point1_temp.dev_attr.attr,
	&sensor_dev_attr_pwm2_auto_point1_temp_hyst.dev_attr.attr,
	&sensor_dev_attr_pwm2_auto_point2_temp.dev_attr.attr,
	&sensor_dev_attr_pwm2_auto_point3_temp.dev_attr.attr,
	&sensor_dev_attr_pwm2_auto_point4_temp.dev_attr.attr,
	NULL
}, {
	&sensor_dev_attr_pwm3_auto_point1_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm3_auto_point2_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm3_auto_point3_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm3_auto_point4_pwm.dev_attr.attr,
	&sensor_dev_attr_pwm3_auto_point1_temp.dev_attr.attr,
	&sensor_dev_attr_pwm3_auto_point1_temp_hyst.dev_attr.attr,
	&sensor_dev_attr_pwm3_auto_point2_temp.dev_attr.attr,
	&sensor_dev_attr_pwm3_auto_point3_temp.dev_attr.attr,
	&sensor_dev_attr_pwm3_auto_point4_temp.dev_attr.attr,
	NULL
} };

static const struct attribute_group it87_group_autopwm[3] = {
	{ .attrs = it87_attributes_autopwm[0] },
	{ .attrs = it87_attributes_autopwm[1] },
	{ .attrs = it87_attributes_autopwm[2] },
};

static struct attribute *it87_attributes_fan_beep[] = {
	&sensor_dev_attr_fan1_beep.dev_attr.attr,
	&sensor_dev_attr_fan2_beep.dev_attr.attr,
	&sensor_dev_attr_fan3_beep.dev_attr.attr,
	&sensor_dev_attr_fan4_beep.dev_attr.attr,
	&sensor_dev_attr_fan5_beep.dev_attr.attr,
};

static struct attribute *it87_attributes_vid[] = {
	&dev_attr_vrm.attr,
	&dev_attr_cpu0_vid.attr,
	NULL
};

static const struct attribute_group it87_group_vid = {
	.attrs = it87_attributes_vid,
};

static struct attribute *it87_attributes_label[] = {
	&sensor_dev_attr_in3_label.dev_attr.attr,
	&sensor_dev_attr_in7_label.dev_attr.attr,
	&sensor_dev_attr_in8_label.dev_attr.attr,
	NULL
};

static const struct attribute_group it87_group_label = {
	.attrs = it87_attributes_label,
};

static int __init it87_find(unsigned short *address,
	struct it87_sio_data *sio_data)
{
	int err;
	u16 chip_type;
	const char *board_vendor, *board_name;

	err = superio_enter();
	if (err)
		return err;

	err = -ENODEV;
	chip_type = force_id ? force_id : superio_inw(DEVID);

	switch (chip_type) {
	case IT8705F_DEVID:
		sio_data->type = it87;
		break;
	case IT8712F_DEVID:
		sio_data->type = it8712;
		break;
	case IT8716F_DEVID:
	case IT8726F_DEVID:
		sio_data->type = it8716;
		break;
	case IT8718F_DEVID:
		sio_data->type = it8718;
		break;
	case IT8720F_DEVID:
		sio_data->type = it8720;
		break;
	case IT8721F_DEVID:
		sio_data->type = it8721;
		break;
	case IT8728F_DEVID:
		sio_data->type = it8728;
		break;
	case IT8771E_DEVID:
		sio_data->type = it8771;
		break;
	case IT8772E_DEVID:
		sio_data->type = it8772;
		break;
	case IT8782F_DEVID:
		sio_data->type = it8782;
		break;
	case IT8783E_DEVID:
		sio_data->type = it8783;
		break;
	case 0xffff:	 
		goto exit;
	default:
		pr_debug("Unsupported chip (DEVID=0x%x)\n", chip_type);
		goto exit;
	}

	superio_select(PME);
	if (!(superio_inb(IT87_ACT_REG) & 0x01)) {
		pr_info("Device not activated, skipping\n");
		goto exit;
	}

	*address = superio_inw(IT87_BASE_REG) & ~(IT87_EXTENT - 1);
	if (*address == 0) {
		pr_info("Base address not set, skipping\n");
		goto exit;
	}

	err = 0;
	sio_data->revision = superio_inb(DEVREV) & 0x0f;
	pr_info("Found IT%04xF chip at 0x%x, revision %d\n",
		chip_type, *address, sio_data->revision);

	sio_data->internal = (1 << 2);

	if (sio_data->type == it87) {
		 
		sio_data->skip_vid = 1;

		superio_select(5);
		sio_data->beep_pin = superio_inb(IT87_SIO_BEEP_PIN_REG) & 0x3f;
	} else if (sio_data->type == it8783) {
		int reg25, reg27, reg2a, reg2c, regef;

		sio_data->skip_vid = 1;	 

		superio_select(GPIO);

		reg25 = superio_inb(IT87_SIO_GPIO1_REG);
		reg27 = superio_inb(IT87_SIO_GPIO3_REG);
		reg2a = superio_inb(IT87_SIO_PINX1_REG);
		reg2c = superio_inb(IT87_SIO_PINX2_REG);
		regef = superio_inb(IT87_SIO_SPI_REG);

		if ((reg27 & (1 << 0)) || !(reg2c & (1 << 2)))
			sio_data->skip_fan |= (1 << 2);
		if ((reg25 & (1 << 4))
		    || (!(reg2a & (1 << 1)) && (regef & (1 << 0))))
			sio_data->skip_pwm |= (1 << 2);

		if (reg27 & (1 << 7))
			sio_data->skip_fan |= (1 << 1);
		if (reg27 & (1 << 3))
			sio_data->skip_pwm |= (1 << 1);

		if ((reg27 & (1 << 0)) || (reg2c & (1 << 2)))
			sio_data->skip_in |= (1 << 5);  

		if (reg27 & (1 << 1))
			sio_data->skip_in |= (1 << 6);  

		if (reg27 & (1 << 2)) {
			 
			if (!(reg2c & (1 << 1))) {
				reg2c |= (1 << 1);
				superio_outb(IT87_SIO_PINX2_REG, reg2c);
				pr_notice("Routing internal VCCH5V to in7.\n");
			}
			pr_notice("in7 routed to internal voltage divider, with external pin disabled.\n");
			pr_notice("Please report if it displays a reasonable voltage.\n");
		}

		if (reg2c & (1 << 0))
			sio_data->internal |= (1 << 0);
		if (reg2c & (1 << 1))
			sio_data->internal |= (1 << 1);

		sio_data->beep_pin = superio_inb(IT87_SIO_BEEP_PIN_REG) & 0x3f;

	} else {
		int reg;
		bool uart6;

		superio_select(GPIO);

		reg = superio_inb(IT87_SIO_GPIO3_REG);
		if (sio_data->type == it8721 || sio_data->type == it8728 ||
		    sio_data->type == it8771 || sio_data->type == it8772 ||
		    sio_data->type == it8782) {
			 
			sio_data->skip_vid = 1;
		} else {
			 
			if (reg & 0x0f) {
				pr_info("VID is disabled (pins used for GPIO)\n");
				sio_data->skip_vid = 1;
			}
		}

		if (reg & (1 << 6))
			sio_data->skip_pwm |= (1 << 2);
		if (reg & (1 << 7))
			sio_data->skip_fan |= (1 << 2);

		reg = superio_inb(IT87_SIO_GPIO5_REG);
		if (reg & (1 << 1))
			sio_data->skip_pwm |= (1 << 1);
		if (reg & (1 << 2))
			sio_data->skip_fan |= (1 << 1);

		if ((sio_data->type == it8718 || sio_data->type == it8720)
		 && !(sio_data->skip_vid))
			sio_data->vid_value = superio_inb(IT87_SIO_VID_REG);

		reg = superio_inb(IT87_SIO_PINX2_REG);

		uart6 = sio_data->type == it8782 && (reg & (1 << 2));

		if ((sio_data->type == it8720 || uart6) && !(reg & (1 << 1))) {
			reg |= (1 << 1);
			superio_outb(IT87_SIO_PINX2_REG, reg);
			pr_notice("Routing internal VCCH to in7\n");
		}
		if (reg & (1 << 0))
			sio_data->internal |= (1 << 0);
		if ((reg & (1 << 1)) || sio_data->type == it8721 ||
		    sio_data->type == it8728 ||
		    sio_data->type == it8771 ||
		    sio_data->type == it8772)
			sio_data->internal |= (1 << 1);

		if (uart6) {
			sio_data->skip_in |= (1 << 5) | (1 << 6);
			sio_data->skip_temp |= (1 << 2);
		}

		sio_data->beep_pin = superio_inb(IT87_SIO_BEEP_PIN_REG) & 0x3f;
	}
	if (sio_data->beep_pin)
		pr_info("Beeping is supported\n");

	board_vendor = dmi_get_system_info(DMI_BOARD_VENDOR);
	board_name = dmi_get_system_info(DMI_BOARD_NAME);
	if (board_vendor && board_name) {
		if (strcmp(board_vendor, "nVIDIA") == 0
		 && strcmp(board_name, "FN68PT") == 0) {
			 
			pr_info("Disabling pwm2 due to hardware constraints\n");
			sio_data->skip_pwm = (1 << 1);
		}
	}

exit:
	superio_exit();
	return err;
}

static void it87_remove_files(struct device *dev)
{
	struct it87_data *data = platform_get_drvdata(pdev);
	struct it87_sio_data *sio_data = dev->platform_data;
	int i;

	sysfs_remove_group(&dev->kobj, &it87_group);
	for (i = 0; i < 9; i++) {
		if (sio_data->skip_in & (1 << i))
			continue;
		sysfs_remove_group(&dev->kobj, &it87_group_in[i]);
		if (it87_attributes_in_beep[i])
			sysfs_remove_file(&dev->kobj,
					  it87_attributes_in_beep[i]);
	}
	for (i = 0; i < 3; i++) {
		if (!(data->has_temp & (1 << i)))
			continue;
		sysfs_remove_group(&dev->kobj, &it87_group_temp[i]);
		if (has_temp_offset(data))
			sysfs_remove_file(&dev->kobj,
					  it87_attributes_temp_offset[i]);
		if (sio_data->beep_pin)
			sysfs_remove_file(&dev->kobj,
					  it87_attributes_temp_beep[i]);
	}
	for (i = 0; i < 5; i++) {
		if (!(data->has_fan & (1 << i)))
			continue;
		sysfs_remove_group(&dev->kobj, &it87_group_fan[i]);
		if (sio_data->beep_pin)
			sysfs_remove_file(&dev->kobj,
					  it87_attributes_fan_beep[i]);
		if (i < 3 && !has_16bit_fans(data))
			sysfs_remove_file(&dev->kobj,
					  it87_attributes_fan_div[i]);
	}
	for (i = 0; i < 3; i++) {
		if (sio_data->skip_pwm & (1 << 0))
			continue;
		sysfs_remove_group(&dev->kobj, &it87_group_pwm[i]);
		if (has_old_autopwm(data))
			sysfs_remove_group(&dev->kobj,
					   &it87_group_autopwm[i]);
	}
	if (!sio_data->skip_vid)
		sysfs_remove_group(&dev->kobj, &it87_group_vid);
	sysfs_remove_group(&dev->kobj, &it87_group_label);
}

static int it87_probe(struct platform_device *pdev)
{
	struct it87_data *data;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct it87_sio_data *sio_data = dev->platform_data;
	int err = 0, i;
	int enable_pwm_interface;
	int fan_beep_need_rw;

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!devm_request_region(&pdev->dev, res->start, IT87_EC_EXTENT,
				 DRVNAME)) {
		dev_err(dev, "Failed to request region 0x%lx-0x%lx\n",
			(unsigned long)res->start,
			(unsigned long)(res->start + IT87_EC_EXTENT - 1));
		return -EBUSY;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(struct it87_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->addr = res->start;
	data->type = sio_data->type;
	data->features = it87_devices[sio_data->type].features;
	data->peci_mask = it87_devices[sio_data->type].peci_mask;
	data->old_peci_mask = it87_devices[sio_data->type].old_peci_mask;
	data->name = it87_devices[sio_data->type].name;
	 
	switch (data->type) {
	case it87:
		if (sio_data->revision >= 0x03) {
			data->features &= ~FEAT_OLD_AUTOPWM;
			data->features |= FEAT_16BIT_FANS;
		}
		break;
	case it8712:
		if (sio_data->revision >= 0x08) {
			data->features &= ~FEAT_OLD_AUTOPWM;
			data->features |= FEAT_16BIT_FANS;
		}
		break;
	default:
		break;
	}

	if ((it87_read_value(data, IT87_REG_CONFIG) & 0x80)
	 || it87_read_value(data, IT87_REG_CHIPID) != 0x90)
		return -ENODEV;

	platform_set_drvdata(pdev, data);

	mutex_init(&data->update_lock);

	enable_pwm_interface = it87_check_pwm(dev);

	if (has_12mv_adc(data)) {
		if (sio_data->internal & (1 << 0))
			data->in_scaled |= (1 << 3);	 
		if (sio_data->internal & (1 << 1))
			data->in_scaled |= (1 << 7);	 
		if (sio_data->internal & (1 << 2))
			data->in_scaled |= (1 << 8);	 
	} else if (sio_data->type == it8782 || sio_data->type == it8783) {
		if (sio_data->internal & (1 << 0))
			data->in_scaled |= (1 << 3);	 
		if (sio_data->internal & (1 << 1))
			data->in_scaled |= (1 << 7);	 
	}

	data->has_temp = 0x07;
	if (sio_data->skip_temp & (1 << 2)) {
		if (sio_data->type == it8782
		    && !(it87_read_value(data, IT87_REG_TEMP_EXTRA) & 0x80))
			data->has_temp &= ~(1 << 2);
	}

	it87_init_device(pdev);

	err = sysfs_create_group(&dev->kobj, &it87_group);
	if (err)
		return err;

	for (i = 0; i < 9; i++) {
		if (sio_data->skip_in & (1 << i))
			continue;
		err = sysfs_create_group(&dev->kobj, &it87_group_in[i]);
		if (err)
			goto error;
		if (sio_data->beep_pin && it87_attributes_in_beep[i]) {
			err = sysfs_create_file(&dev->kobj,
						it87_attributes_in_beep[i]);
			if (err)
				goto error;
		}
	}

	for (i = 0; i < 3; i++) {
		if (!(data->has_temp & (1 << i)))
			continue;
		err = sysfs_create_group(&dev->kobj, &it87_group_temp[i]);
		if (err)
			goto error;
		if (has_temp_offset(data)) {
			err = sysfs_create_file(&dev->kobj,
						it87_attributes_temp_offset[i]);
			if (err)
				goto error;
		}
		if (sio_data->beep_pin) {
			err = sysfs_create_file(&dev->kobj,
						it87_attributes_temp_beep[i]);
			if (err)
				goto error;
		}
	}

	fan_beep_need_rw = 1;
	for (i = 0; i < 5; i++) {
		if (!(data->has_fan & (1 << i)))
			continue;
		err = sysfs_create_group(&dev->kobj, &it87_group_fan[i]);
		if (err)
			goto error;

		if (i < 3 && !has_16bit_fans(data)) {
			err = sysfs_create_file(&dev->kobj,
						it87_attributes_fan_div[i]);
			if (err)
				goto error;
		}

		if (sio_data->beep_pin) {
			err = sysfs_create_file(&dev->kobj,
						it87_attributes_fan_beep[i]);
			if (err)
				goto error;
			if (!fan_beep_need_rw)
				continue;

			if (sysfs_chmod_file(&dev->kobj,
					     it87_attributes_fan_beep[i],
					     S_IRUGO | S_IWUSR))
				dev_dbg(dev, "chmod +w fan%d_beep failed\n",
					i + 1);
			fan_beep_need_rw = 0;
		}
	}

	if (enable_pwm_interface) {
		for (i = 0; i < 3; i++) {
			if (sio_data->skip_pwm & (1 << i))
				continue;
			err = sysfs_create_group(&dev->kobj,
						 &it87_group_pwm[i]);
			if (err)
				goto error;

			if (!has_old_autopwm(data))
				continue;
			err = sysfs_create_group(&dev->kobj,
						 &it87_group_autopwm[i]);
			if (err)
				goto error;
		}
	}

	if (!sio_data->skip_vid) {
		data->vrm = vid_which_vrm();
		 
		data->vid = sio_data->vid_value;
		err = sysfs_create_group(&dev->kobj, &it87_group_vid);
		if (err)
			goto error;
	}

	for (i = 0; i < 3; i++) {
		if (!(sio_data->internal & (1 << i)))
			continue;
		err = sysfs_create_file(&dev->kobj,
					it87_attributes_label[i]);
		if (err)
			goto error;
	}

	data->hwmon_dev = hwmon_device_register(dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto error;
	}

	return 0;

error:
	it87_remove_files(dev);
	return err;
}

static int it87_remove(struct platform_device *pdev)
{
	struct it87_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(data->hwmon_dev);
	it87_remove_files(&pdev->dev);

	return 0;
}

static int it87_read_value(struct it87_data *data, u8 reg)
{
	outb_p(reg, data->addr + IT87_ADDR_REG_OFFSET);
	return inb_p(data->addr + IT87_DATA_REG_OFFSET);
}

static void it87_write_value(struct it87_data *data, u8 reg, u8 value)
{
	outb_p(reg, data->addr + IT87_ADDR_REG_OFFSET);
	outb_p(value, data->addr + IT87_DATA_REG_OFFSET);
}

static int it87_check_pwm(struct device *dev)
{
	struct it87_data *data = dev_get_drvdata(dev);
	 
	int tmp = it87_read_value(data, IT87_REG_FAN_CTL);
	if ((tmp & 0x87) == 0) {
		if (fix_pwm_polarity) {
			 
			int i;
			u8 pwm[3];

			for (i = 0; i < 3; i++)
				pwm[i] = it87_read_value(data,
							 IT87_REG_PWM(i));

			if (!((pwm[0] | pwm[1] | pwm[2]) & 0x80)) {
				dev_info(dev,
					 "Reconfiguring PWM to active high polarity\n");
				it87_write_value(data, IT87_REG_FAN_CTL,
						 tmp | 0x87);
				for (i = 0; i < 3; i++)
					it87_write_value(data,
							 IT87_REG_PWM(i),
							 0x7f & ~pwm[i]);
				return 1;
			}

			dev_info(dev,
				 "PWM configuration is too broken to be fixed\n");
		}

		dev_info(dev,
			 "Detected broken BIOS defaults, disabling PWM interface\n");
		return 0;
	} else if (fix_pwm_polarity) {
		dev_info(dev,
			 "PWM configuration looks sane, won't touch\n");
	}

	return 1;
}

static void it87_init_device(struct platform_device *pdev)
{
	struct it87_sio_data *sio_data = pdev->dev.platform_data;
	struct it87_data *data = platform_get_drvdata(pdev);
	int tmp, i;
	u8 mask;

	for (i = 0; i < 3; i++) {
		data->pwm_temp_map[i] = i;
		data->pwm_duty[i] = 0x7f;	 
		data->auto_pwm[i][3] = 0x7f;	 
	}

	for (i = 0; i < 8; i++) {
		tmp = it87_read_value(data, IT87_REG_VIN_MIN(i));
		if (tmp == 0xff)
			it87_write_value(data, IT87_REG_VIN_MIN(i), 0);
	}
	for (i = 0; i < 3; i++) {
		tmp = it87_read_value(data, IT87_REG_TEMP_HIGH(i));
		if (tmp == 0xff)
			it87_write_value(data, IT87_REG_TEMP_HIGH(i), 127);
	}

	tmp = it87_read_value(data, IT87_REG_VIN_ENABLE);
	if ((tmp & 0xff) == 0) {
		 
		it87_write_value(data, IT87_REG_VIN_ENABLE, 0xff);
	}

	mask = 0x70 & ~(sio_data->skip_fan << 4);
	data->fan_main_ctrl = it87_read_value(data, IT87_REG_FAN_MAIN_CTRL);
	if ((data->fan_main_ctrl & mask) == 0) {
		 
		data->fan_main_ctrl |= mask;
		it87_write_value(data, IT87_REG_FAN_MAIN_CTRL,
				 data->fan_main_ctrl);
	}
	data->has_fan = (data->fan_main_ctrl >> 4) & 0x07;

	if (has_16bit_fans(data)) {
		tmp = it87_read_value(data, IT87_REG_FAN_16BIT);
		if (~tmp & 0x07 & data->has_fan) {
			dev_dbg(&pdev->dev,
				"Setting fan1-3 to 16-bit mode\n");
			it87_write_value(data, IT87_REG_FAN_16BIT,
					 tmp | 0x07);
		}
		 
		if (data->type != it87 && data->type != it8782 &&
		    data->type != it8783) {
			if (tmp & (1 << 4))
				data->has_fan |= (1 << 3);  
			if (tmp & (1 << 5))
				data->has_fan |= (1 << 4);  
		}
	}

	data->has_fan &= ~sio_data->skip_fan;

	it87_write_value(data, IT87_REG_CONFIG,
			 (it87_read_value(data, IT87_REG_CONFIG) & 0x3e)
			 | (update_vbat ? 0x41 : 0x01));
}

static void it87_update_pwm_ctrl(struct it87_data *data, int nr)
{
	data->pwm_ctrl[nr] = it87_read_value(data, IT87_REG_PWM(nr));
	if (has_newer_autopwm(data)) {
		data->pwm_temp_map[nr] = data->pwm_ctrl[nr] & 0x03;
		data->pwm_duty[nr] = it87_read_value(data,
						     IT87_REG_PWM_DUTY(nr));
	} else {
		if (data->pwm_ctrl[nr] & 0x80)	 
			data->pwm_temp_map[nr] = data->pwm_ctrl[nr] & 0x03;
		else				 
			data->pwm_duty[nr] = data->pwm_ctrl[nr] & 0x7f;
	}

	if (has_old_autopwm(data)) {
		int i;

		for (i = 0; i < 5 ; i++)
			data->auto_temp[nr][i] = it87_read_value(data,
						IT87_REG_AUTO_TEMP(nr, i));
		for (i = 0; i < 3 ; i++)
			data->auto_pwm[nr][i] = it87_read_value(data,
						IT87_REG_AUTO_PWM(nr, i));
	}
}

static struct it87_data *it87_update_device(struct device *dev)
{
	struct it87_data *data = dev_get_drvdata(dev);
	int i;

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ + HZ / 2)
	    || !data->valid) {
		if (update_vbat) {
			 
			it87_write_value(data, IT87_REG_CONFIG,
				it87_read_value(data, IT87_REG_CONFIG) | 0x40);
		}
		for (i = 0; i <= 7; i++) {
			data->in[i][0] =
				it87_read_value(data, IT87_REG_VIN(i));
			data->in[i][1] =
				it87_read_value(data, IT87_REG_VIN_MIN(i));
			data->in[i][2] =
				it87_read_value(data, IT87_REG_VIN_MAX(i));
		}
		 
		data->in[8][0] = it87_read_value(data, IT87_REG_VIN(8));

		for (i = 0; i < 5; i++) {
			 
			if (!(data->has_fan & (1 << i)))
				continue;

			data->fan[i][1] =
				it87_read_value(data, IT87_REG_FAN_MIN[i]);
			data->fan[i][0] = it87_read_value(data,
				       IT87_REG_FAN[i]);
			 
			if (has_16bit_fans(data)) {
				data->fan[i][0] |= it87_read_value(data,
						IT87_REG_FANX[i]) << 8;
				data->fan[i][1] |= it87_read_value(data,
						IT87_REG_FANX_MIN[i]) << 8;
			}
		}
		for (i = 0; i < 3; i++) {
			if (!(data->has_temp & (1 << i)))
				continue;
			data->temp[i][0] =
				it87_read_value(data, IT87_REG_TEMP(i));
			data->temp[i][1] =
				it87_read_value(data, IT87_REG_TEMP_LOW(i));
			data->temp[i][2] =
				it87_read_value(data, IT87_REG_TEMP_HIGH(i));
			if (has_temp_offset(data))
				data->temp[i][3] =
				  it87_read_value(data,
						  IT87_REG_TEMP_OFFSET[i]);
		}

		if ((data->has_fan & 0x07) && !has_16bit_fans(data)) {
			i = it87_read_value(data, IT87_REG_FAN_DIV);
			data->fan_div[0] = i & 0x07;
			data->fan_div[1] = (i >> 3) & 0x07;
			data->fan_div[2] = (i & 0x40) ? 3 : 1;
		}

		data->alarms =
			it87_read_value(data, IT87_REG_ALARM1) |
			(it87_read_value(data, IT87_REG_ALARM2) << 8) |
			(it87_read_value(data, IT87_REG_ALARM3) << 16);
		data->beeps = it87_read_value(data, IT87_REG_BEEP_ENABLE);

		data->fan_main_ctrl = it87_read_value(data,
				IT87_REG_FAN_MAIN_CTRL);
		data->fan_ctl = it87_read_value(data, IT87_REG_FAN_CTL);
		for (i = 0; i < 3; i++)
			it87_update_pwm_ctrl(data, i);

		data->sensor = it87_read_value(data, IT87_REG_TEMP_ENABLE);
		data->extra = it87_read_value(data, IT87_REG_TEMP_EXTRA);
		 
		if (data->type == it8712 || data->type == it8716) {
			data->vid = it87_read_value(data, IT87_REG_VID);
			 
			data->vid &= 0x3f;
		}
		data->last_updated = jiffies;
		data->valid = 1;
	}

	mutex_unlock(&data->update_lock);

	return data;
}

static int __init it87_device_add(unsigned short address,
				  const struct it87_sio_data *sio_data)
{
	struct resource res = {
		.start	= address + IT87_EC_OFFSET,
		.end	= address + IT87_EC_OFFSET + IT87_EC_EXTENT - 1,
		.name	= DRVNAME,
		.flags	= IORESOURCE_IO,
	};
	int err;

	err = acpi_check_resource_conflict(&res);
	if (err)
		goto exit;

	pdev = platform_device_alloc(DRVNAME, address);
	if (!pdev) {
		err = -ENOMEM;
		pr_err("Device allocation failed\n");
		goto exit;
	}

	err = platform_device_add_resources(pdev, &res, 1);
	if (err) {
		pr_err("Device resource addition failed (%d)\n", err);
		goto exit_device_put;
	}

	err = platform_device_add_data(pdev, sio_data,
				       sizeof(struct it87_sio_data));
	if (err) {
		pr_err("Platform data allocation failed\n");
		goto exit_device_put;
	}

	err = platform_device_add(pdev);
	if (err) {
		pr_err("Device addition failed (%d)\n", err);
		goto exit_device_put;
	}

	return 0;

exit_device_put:
	platform_device_put(pdev);
exit:
	return err;
}

static int __init sm_it87_init(void)
{
	int err;
	unsigned short isa_address = 0;
	struct it87_sio_data sio_data;

	memset(&sio_data, 0, sizeof(struct it87_sio_data));
	err = it87_find(&isa_address, &sio_data);
	if (err)
		return err;
	err = platform_driver_register(&it87_driver);
	if (err)
		return err;

	err = it87_device_add(isa_address, &sio_data);
	if (err) {
		platform_driver_unregister(&it87_driver);
		return err;
	}

	return 0;
}

static void __exit sm_it87_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&it87_driver);
}

#ifdef MY_ABC_HERE
int syno_sys_temperature(int *Temperature)
{
	unsigned short address;
	resource_size_t res_start;

	superio_enter();
	superio_inw(DEVID);

	superio_select(PME);
	if (!(superio_inb(IT87_ACT_REG) & 0x01)) {
		printk("it87: Device not activated, skipping\n");
		return -1;
	}

	address = superio_inw(IT87_BASE_REG) & ~(IT87_EXTENT - 1);
	if (address == 0) {
		printk("it87: Base address not set, skipping\n");
		return -1;
	}
	res_start = address + IT87_EC_OFFSET;

	outb_p(IT87_REG_TEMP(0), res_start + IT87_ADDR_REG_OFFSET);
	*Temperature = inb_p(res_start + IT87_DATA_REG_OFFSET)<<1;

	superio_exit();

	return 0;
}
EXPORT_SYMBOL(syno_sys_temperature);
#endif  

#ifdef MY_ABC_HERE
struct writable_pin {
	u32         pin;
	u32         en_reg;
	u32         en_bit;
	u32         gpio_set;
	u32         gpio_bit;
	u32         gpio_val;
};
static struct writable_pin writable_pin_setting[] =
{
	{32, 0x27, 2, 2 , 2, 0},
	{33, 0x27, 3, 2 , 3, 0},
	{11, 0x25, 1, 0 , 1, 0}
};

u32 syno_superio_gpio_pin(int pin, int *pValue, int isWrite)
{
	int ret = -1;
	int i = 0, j = 0;
	u16 gpiobase, addr_lvl, val_lvl;
	u8 addr_use_select, addr_io_select;
	u8 val_en, val_use_select, val_io_select;
	u8 tmpVal = 0;

	superio_enter();
	superio_inw(DEVID);

	superio_select(PME);
	if (!(superio_inb(IT87_ACT_REG) & 0x01)) {
		printk("it87: Device not activated, skipping\n");
		goto END;
	}

	superio_select(GPIO);
	gpiobase = superio_inw(0x62);
	if (gpiobase == 0) {
		printk("it87: GPIO base address not set, skipping\n");
		goto END;
	}

	for ( i = 0; i < sizeof(writable_pin_setting)/sizeof(writable_pin_setting[0]); i++) {
		if (pin == writable_pin_setting[i].pin) {
			break;
		}
	}
	if ( i == sizeof(writable_pin_setting)/sizeof(writable_pin_setting[0]) ) {
		printk("pin %d is protected by superio driver.\n", pin);
		goto END;
	}

	val_en = superio_inb(writable_pin_setting[i].en_reg);
	tmpVal = 1 << writable_pin_setting[i].en_bit;
	val_en |= tmpVal;
	outb(val_en, writable_pin_setting[i].en_reg);

	if ( 1 == isWrite ) {
		 
		addr_use_select = writable_pin_setting[i].gpio_set + 0xc0;
		val_use_select = superio_inb(addr_use_select);
		tmpVal = 1 << writable_pin_setting[i].gpio_bit;
		val_use_select |= tmpVal;
		outb(val_use_select, addr_use_select);

		addr_io_select = writable_pin_setting[i].gpio_set + 0xc8;
		val_io_select = superio_inb(addr_io_select);
		tmpVal = 1 << writable_pin_setting[i].gpio_bit;
		val_io_select |= tmpVal;
		outb(val_io_select, addr_io_select);

		addr_lvl = writable_pin_setting[i].gpio_set + gpiobase;
		val_lvl = 0;
		for ( j = 0; j < sizeof(writable_pin_setting)/sizeof(writable_pin_setting[0]); j++) {
			if (writable_pin_setting[i].gpio_set == writable_pin_setting[j].gpio_set) {
				if (writable_pin_setting[j].gpio_val == 1) {
					val_lvl |= (1<<writable_pin_setting[j].gpio_bit);
				}
			}
		}
		if ( 1 == *pValue ) {
			tmpVal = 1 << writable_pin_setting[i].gpio_bit;
			val_lvl |= tmpVal;
			outw(val_lvl, addr_lvl);
			writable_pin_setting[i].gpio_val = 1;
		} else {
			tmpVal = ~(1 << writable_pin_setting[i].gpio_bit);
			val_lvl &= tmpVal;
			outw(val_lvl, addr_lvl);
			writable_pin_setting[i].gpio_val = 0;
		}
	} else {
		*pValue = writable_pin_setting[i].gpio_val;
	}

	ret = 0;
	END:
	superio_exit();

	return ret;
}
EXPORT_SYMBOL(syno_superio_gpio_pin);
#endif  

#ifdef MY_ABC_HERE
int syno_superio_regist_read(u8 ldn, u8 reg, u8 *value)
{
	int iRet = -1;

	superio_enter();
	superio_select(ldn);
	*value = superio_inb(reg);
	superio_exit();

	iRet = 0;
	return iRet;
}
EXPORT_SYMBOL(syno_superio_regist_read);

int syno_superio_regist_write(u8 ldn, u8 reg, u8 value)
{
	int iRet = -1;

	superio_enter();
	superio_select(ldn);
	superio_outb(reg, value);
	superio_exit();

	iRet = 0;
	return iRet;
}
EXPORT_SYMBOL(syno_superio_regist_write);
#endif  

MODULE_AUTHOR("Chris Gauthron, Jean Delvare <khali@linux-fr.org>");
MODULE_DESCRIPTION("IT8705F/IT871xF/IT872xF hardware monitoring driver");
module_param(update_vbat, bool, 0);
MODULE_PARM_DESC(update_vbat, "Update vbat if set else return powerup value");
module_param(fix_pwm_polarity, bool, 0);
MODULE_PARM_DESC(fix_pwm_polarity,
		 "Force PWM polarity to active high (DANGEROUS)");
MODULE_LICENSE("GPL");

module_init(sm_it87_init);
module_exit(sm_it87_exit);

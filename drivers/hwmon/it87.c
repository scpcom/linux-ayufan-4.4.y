#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
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

enum chips { it87, it8712, it8716, it8718, it8720 };

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

static inline int
superio_inb(int reg)
{
	outb(reg, REG);
	return inb(VAL);
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

static inline void
superio_select(int ldn)
{
	outb(DEV, REG);
	outb(ldn, VAL);
}

static inline void
superio_enter(void)
{
	outb(0x87, REG);
	outb(0x01, REG);
	outb(0x55, REG);
	outb(0x55, REG);
}

static inline void
superio_exit(void)
{
	outb(0x02, REG);
	outb(0x02, VAL);
}

#define IT8712F_DEVID 0x8712
#define IT8705F_DEVID 0x8705
#define IT8716F_DEVID 0x8716
#define IT8718F_DEVID 0x8718
#define IT8720F_DEVID 0x8720
#define IT8726F_DEVID 0x8726
#define IT87_ACT_REG  0x30
#define IT87_BASE_REG 0x60

#define IT87_SIO_PINX2_REG	0x2c	 
#define IT87_SIO_VID_REG	0xfc	 

static int update_vbat;

static int fix_pwm_polarity;

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
#define IT87_REG_FAN_MAIN_CTRL 0x13
#define IT87_REG_FAN_CTL       0x14
#define IT87_REG_PWM(nr)       (0x15 + (nr))

#define IT87_REG_VIN(nr)       (0x20 + (nr))
#define IT87_REG_TEMP(nr)      (0x29 + (nr))

#define IT87_REG_VIN_MAX(nr)   (0x30 + (nr) * 2)
#define IT87_REG_VIN_MIN(nr)   (0x31 + (nr) * 2)
#define IT87_REG_TEMP_HIGH(nr) (0x40 + (nr) * 2)
#define IT87_REG_TEMP_LOW(nr)  (0x41 + (nr) * 2)

#define IT87_REG_VIN_ENABLE    0x50
#define IT87_REG_TEMP_ENABLE   0x51

#define IT87_REG_CHIPID        0x58

#define IN_TO_REG(val)  (SENSORS_LIMIT((((val) + 8)/16),0,255))
#define IN_FROM_REG(val) ((val) * 16)

static inline u8 FAN_TO_REG(long rpm, int div)
{
	if (rpm == 0)
		return 255;
	rpm = SENSORS_LIMIT(rpm, 1, 1000000);
	return SENSORS_LIMIT((1350000 + rpm * div / 2) / (rpm * div), 1,
			     254);
}

static inline u16 FAN16_TO_REG(long rpm)
{
	if (rpm == 0)
		return 0xffff;
	return SENSORS_LIMIT((1350000 + rpm) / (rpm * 2), 1, 0xfffe);
}

#define FAN_FROM_REG(val,div) ((val)==0?-1:(val)==255?0:1350000/((val)*(div)))
 
#define FAN16_FROM_REG(val) ((val)==0?-1:(val)==0xffff?0:1350000/((val)*2))

#define TEMP_TO_REG(val) (SENSORS_LIMIT(((val)<0?(((val)-500)/1000):\
					((val)+500)/1000),-128,127))
#define TEMP_FROM_REG(val) ((val) * 1000)

#define PWM_TO_REG(val)   ((val) >> 1)
#define PWM_FROM_REG(val) (((val)&0x7f) << 1)

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

struct it87_sio_data {
	enum chips type;
	 
	u8 revision;
	u8 vid_value;
	 
	u8 skip_pwm;
};

struct it87_data {
	struct device *hwmon_dev;
	enum chips type;
	u8 revision;

	unsigned short addr;
	const char *name;
	struct mutex update_lock;
	char valid;		 
	unsigned long last_updated;	 

	u8 in[9];		 
	u8 in_max[8];		 
	u8 in_min[8];		 
	u8 has_fan;		 
	u16 fan[5];		 
	u16 fan_min[5];		 
	s8 temp[3];		 
	s8 temp_high[3];	 
	s8 temp_low[3];		 
	u8 sensor;		 
	u8 fan_div[3];		 
	u8 vid;			 
	u8 vrm;
	u32 alarms;		 
	u8 fan_main_ctrl;	 
	u8 fan_ctl;		 
	u8 manual_pwm_ctl[3];    
};

static inline int has_16bit_fans(const struct it87_data *data)
{
	 
	return (data->type == it87 && data->revision >= 0x03)
	    || (data->type == it8712 && data->revision >= 0x08)
	    || data->type == it8716
	    || data->type == it8718
	    || data->type == it8720;
}

static int it87_probe(struct platform_device *pdev);
static int __devexit it87_remove(struct platform_device *pdev);

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
	.remove	= __devexit_p(it87_remove),
};

static ssize_t show_in(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", IN_FROM_REG(data->in[nr]));
}

static ssize_t show_in_min(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", IN_FROM_REG(data->in_min[nr]));
}

static ssize_t show_in_max(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", IN_FROM_REG(data->in_max[nr]));
}

static ssize_t set_in_min(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	data->in_min[nr] = IN_TO_REG(val);
	it87_write_value(data, IT87_REG_VIN_MIN(nr),
			data->in_min[nr]);
	mutex_unlock(&data->update_lock);
	return count;
}
static ssize_t set_in_max(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	data->in_max[nr] = IN_TO_REG(val);
	it87_write_value(data, IT87_REG_VIN_MAX(nr),
			data->in_max[nr]);
	mutex_unlock(&data->update_lock);
	return count;
}

#define show_in_offset(offset)					\
static SENSOR_DEVICE_ATTR(in##offset##_input, S_IRUGO,		\
		show_in, NULL, offset);

#define limit_in_offset(offset)					\
static SENSOR_DEVICE_ATTR(in##offset##_min, S_IRUGO | S_IWUSR,	\
		show_in_min, set_in_min, offset);		\
static SENSOR_DEVICE_ATTR(in##offset##_max, S_IRUGO | S_IWUSR,	\
		show_in_max, set_in_max, offset);

show_in_offset(0);
limit_in_offset(0);
show_in_offset(1);
limit_in_offset(1);
show_in_offset(2);
limit_in_offset(2);
show_in_offset(3);
limit_in_offset(3);
show_in_offset(4);
limit_in_offset(4);
show_in_offset(5);
limit_in_offset(5);
show_in_offset(6);
limit_in_offset(6);
show_in_offset(7);
limit_in_offset(7);
show_in_offset(8);

static ssize_t show_temp(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", TEMP_FROM_REG(data->temp[nr]));
}
static ssize_t show_temp_max(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", TEMP_FROM_REG(data->temp_high[nr]));
}
static ssize_t show_temp_min(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", TEMP_FROM_REG(data->temp_low[nr]));
}
static ssize_t set_temp_max(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	int val = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	data->temp_high[nr] = TEMP_TO_REG(val);
	it87_write_value(data, IT87_REG_TEMP_HIGH(nr), data->temp_high[nr]);
	mutex_unlock(&data->update_lock);
	return count;
}
static ssize_t set_temp_min(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	int val = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	data->temp_low[nr] = TEMP_TO_REG(val);
	it87_write_value(data, IT87_REG_TEMP_LOW(nr), data->temp_low[nr]);
	mutex_unlock(&data->update_lock);
	return count;
}
#define show_temp_offset(offset)					\
static SENSOR_DEVICE_ATTR(temp##offset##_input, S_IRUGO,		\
		show_temp, NULL, offset - 1);				\
static SENSOR_DEVICE_ATTR(temp##offset##_max, S_IRUGO | S_IWUSR,	\
		show_temp_max, set_temp_max, offset - 1);		\
static SENSOR_DEVICE_ATTR(temp##offset##_min, S_IRUGO | S_IWUSR,	\
		show_temp_min, set_temp_min, offset - 1);

show_temp_offset(1);
show_temp_offset(2);
show_temp_offset(3);

static ssize_t show_sensor(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	u8 reg = data->sensor;  
	
	if (reg & (1 << nr))
		return sprintf(buf, "3\n");   
	if (reg & (8 << nr))
		return sprintf(buf, "4\n");   
	return sprintf(buf, "0\n");       
}
static ssize_t set_sensor(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	int val = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);

	data->sensor &= ~(1 << nr);
	data->sensor &= ~(8 << nr);
	if (val == 2) {	 
		dev_warn(dev, "Sensor type 2 is deprecated, please use 4 "
			 "instead\n");
		val = 4;
	}
	 
	if (val == 3)
	    data->sensor |= 1 << nr;
	else if (val == 4)
	    data->sensor |= 8 << nr;
	else if (val != 0) {
		mutex_unlock(&data->update_lock);
		return -EINVAL;
	}
	it87_write_value(data, IT87_REG_TEMP_ENABLE, data->sensor);
	mutex_unlock(&data->update_lock);
	return count;
}
#define show_sensor_offset(offset)					\
static SENSOR_DEVICE_ATTR(temp##offset##_type, S_IRUGO | S_IWUSR,	\
		show_sensor, set_sensor, offset - 1);

show_sensor_offset(1);
show_sensor_offset(2);
show_sensor_offset(3);

static ssize_t show_fan(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf,"%d\n", FAN_FROM_REG(data->fan[nr], 
				DIV_FROM_REG(data->fan_div[nr])));
}
static ssize_t show_fan_min(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf,"%d\n",
		FAN_FROM_REG(data->fan_min[nr], DIV_FROM_REG(data->fan_div[nr])));
}
static ssize_t show_fan_div(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", DIV_FROM_REG(data->fan_div[nr]));
}
static ssize_t show_pwm_enable(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf,"%d\n", (data->fan_main_ctrl & (1 << nr)) ? 1 : 0);
}
static ssize_t show_pwm(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf,"%d\n", data->manual_pwm_ctl[nr]);
}
static ssize_t show_pwm_freq(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct it87_data *data = it87_update_device(dev);
	int index = (data->fan_ctl >> 4) & 0x07;

	return sprintf(buf, "%u\n", pwm_freq[index]);
}
static ssize_t set_fan_min(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	int val = simple_strtol(buf, NULL, 10);
	u8 reg;

	mutex_lock(&data->update_lock);
	reg = it87_read_value(data, IT87_REG_FAN_DIV);
	switch (nr) {
	case 0: data->fan_div[nr] = reg & 0x07; break;
	case 1: data->fan_div[nr] = (reg >> 3) & 0x07; break;
	case 2: data->fan_div[nr] = (reg & 0x40) ? 3 : 1; break;
	}

	data->fan_min[nr] = FAN_TO_REG(val, DIV_FROM_REG(data->fan_div[nr]));
	it87_write_value(data, IT87_REG_FAN_MIN[nr], data->fan_min[nr]);
	mutex_unlock(&data->update_lock);
	return count;
}
static ssize_t set_fan_div(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int min;
	u8 old;

	mutex_lock(&data->update_lock);
	old = it87_read_value(data, IT87_REG_FAN_DIV);

	min = FAN_FROM_REG(data->fan_min[nr], DIV_FROM_REG(data->fan_div[nr]));

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

	data->fan_min[nr] = FAN_TO_REG(min, DIV_FROM_REG(data->fan_div[nr]));
	it87_write_value(data, IT87_REG_FAN_MIN[nr], data->fan_min[nr]);

	mutex_unlock(&data->update_lock);
	return count;
}
static ssize_t set_pwm_enable(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;

	struct it87_data *data = dev_get_drvdata(dev);
	int val = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);

	if (val == 0) {
		int tmp;
		 
		tmp = it87_read_value(data, IT87_REG_FAN_CTL);
		it87_write_value(data, IT87_REG_FAN_CTL, tmp | (1 << nr));
		 
		data->fan_main_ctrl &= ~(1 << nr);
		it87_write_value(data, IT87_REG_FAN_MAIN_CTRL, data->fan_main_ctrl);
	} else if (val == 1) {
		 
		data->fan_main_ctrl |= (1 << nr);
		it87_write_value(data, IT87_REG_FAN_MAIN_CTRL, data->fan_main_ctrl);
		 
		it87_write_value(data, IT87_REG_PWM(nr), PWM_TO_REG(data->manual_pwm_ctl[nr]));
	} else {
		mutex_unlock(&data->update_lock);
		return -EINVAL;
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
	int val = simple_strtol(buf, NULL, 10);

	if (val < 0 || val > 255)
		return -EINVAL;

	mutex_lock(&data->update_lock);
	data->manual_pwm_ctl[nr] = val;
	if (data->fan_main_ctrl & (1 << nr))
		it87_write_value(data, IT87_REG_PWM(nr), PWM_TO_REG(data->manual_pwm_ctl[nr]));
	mutex_unlock(&data->update_lock);
	return count;
}
static ssize_t set_pwm_freq(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct it87_data *data = dev_get_drvdata(dev);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int i;

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

#define show_fan_offset(offset)					\
static SENSOR_DEVICE_ATTR(fan##offset##_input, S_IRUGO,		\
		show_fan, NULL, offset - 1);			\
static SENSOR_DEVICE_ATTR(fan##offset##_min, S_IRUGO | S_IWUSR, \
		show_fan_min, set_fan_min, offset - 1);		\
static SENSOR_DEVICE_ATTR(fan##offset##_div, S_IRUGO | S_IWUSR, \
		show_fan_div, set_fan_div, offset - 1);

show_fan_offset(1);
show_fan_offset(2);
show_fan_offset(3);

#define show_pwm_offset(offset)						\
static SENSOR_DEVICE_ATTR(pwm##offset##_enable, S_IRUGO | S_IWUSR,	\
		show_pwm_enable, set_pwm_enable, offset - 1);		\
static SENSOR_DEVICE_ATTR(pwm##offset, S_IRUGO | S_IWUSR,		\
		show_pwm, set_pwm, offset - 1);				\
static DEVICE_ATTR(pwm##offset##_freq,					\
		(offset == 1 ? S_IRUGO | S_IWUSR : S_IRUGO),		\
		show_pwm_freq, (offset == 1 ? set_pwm_freq : NULL));

show_pwm_offset(1);
show_pwm_offset(2);
show_pwm_offset(3);

static ssize_t show_fan16(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;
	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", FAN16_FROM_REG(data->fan[nr]));
}

static ssize_t show_fan16_min(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;
	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%d\n", FAN16_FROM_REG(data->fan_min[nr]));
}

static ssize_t set_fan16_min(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct sensor_device_attribute *sensor_attr = to_sensor_dev_attr(attr);
	int nr = sensor_attr->index;
	struct it87_data *data = dev_get_drvdata(dev);
	int val = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	data->fan_min[nr] = FAN16_TO_REG(val);
	it87_write_value(data, IT87_REG_FAN_MIN[nr],
			 data->fan_min[nr] & 0xff);
	it87_write_value(data, IT87_REG_FANX_MIN[nr],
			 data->fan_min[nr] >> 8);
	mutex_unlock(&data->update_lock);
	return count;
}

#define show_fan16_offset(offset) \
static struct sensor_device_attribute sensor_dev_attr_fan##offset##_input16 \
	= SENSOR_ATTR(fan##offset##_input, S_IRUGO,		\
		show_fan16, NULL, offset - 1);			\
static struct sensor_device_attribute sensor_dev_attr_fan##offset##_min16 \
	= SENSOR_ATTR(fan##offset##_min, S_IRUGO | S_IWUSR,	\
		show_fan16_min, set_fan16_min, offset - 1)

show_fan16_offset(1);
show_fan16_offset(2);
show_fan16_offset(3);
show_fan16_offset(4);
show_fan16_offset(5);

static ssize_t show_alarms(struct device *dev, struct device_attribute *attr, char *buf)
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

static ssize_t
show_vrm_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct it87_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%u\n", data->vrm);
}
static ssize_t
store_vrm_reg(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct it87_data *data = dev_get_drvdata(dev);
	u32 val;

	val = simple_strtoul(buf, NULL, 10);
	data->vrm = val;

	return count;
}
static DEVICE_ATTR(vrm, S_IRUGO | S_IWUSR, show_vrm_reg, store_vrm_reg);

static ssize_t
show_vid_reg(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct it87_data *data = it87_update_device(dev);
	return sprintf(buf, "%ld\n", (long) vid_from_reg(data->vid, data->vrm));
}
static DEVICE_ATTR(cpu0_vid, S_IRUGO, show_vid_reg, NULL);

static ssize_t show_name(struct device *dev, struct device_attribute
			 *devattr, char *buf)
{
	struct it87_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%s\n", data->name);
}
static DEVICE_ATTR(name, S_IRUGO, show_name, NULL);

static struct attribute *it87_attributes[] = {
	&sensor_dev_attr_in0_input.dev_attr.attr,
	&sensor_dev_attr_in1_input.dev_attr.attr,
	&sensor_dev_attr_in2_input.dev_attr.attr,
	&sensor_dev_attr_in3_input.dev_attr.attr,
	&sensor_dev_attr_in4_input.dev_attr.attr,
	&sensor_dev_attr_in5_input.dev_attr.attr,
	&sensor_dev_attr_in6_input.dev_attr.attr,
	&sensor_dev_attr_in7_input.dev_attr.attr,
	&sensor_dev_attr_in8_input.dev_attr.attr,
	&sensor_dev_attr_in0_min.dev_attr.attr,
	&sensor_dev_attr_in1_min.dev_attr.attr,
	&sensor_dev_attr_in2_min.dev_attr.attr,
	&sensor_dev_attr_in3_min.dev_attr.attr,
	&sensor_dev_attr_in4_min.dev_attr.attr,
	&sensor_dev_attr_in5_min.dev_attr.attr,
	&sensor_dev_attr_in6_min.dev_attr.attr,
	&sensor_dev_attr_in7_min.dev_attr.attr,
	&sensor_dev_attr_in0_max.dev_attr.attr,
	&sensor_dev_attr_in1_max.dev_attr.attr,
	&sensor_dev_attr_in2_max.dev_attr.attr,
	&sensor_dev_attr_in3_max.dev_attr.attr,
	&sensor_dev_attr_in4_max.dev_attr.attr,
	&sensor_dev_attr_in5_max.dev_attr.attr,
	&sensor_dev_attr_in6_max.dev_attr.attr,
	&sensor_dev_attr_in7_max.dev_attr.attr,
	&sensor_dev_attr_in0_alarm.dev_attr.attr,
	&sensor_dev_attr_in1_alarm.dev_attr.attr,
	&sensor_dev_attr_in2_alarm.dev_attr.attr,
	&sensor_dev_attr_in3_alarm.dev_attr.attr,
	&sensor_dev_attr_in4_alarm.dev_attr.attr,
	&sensor_dev_attr_in5_alarm.dev_attr.attr,
	&sensor_dev_attr_in6_alarm.dev_attr.attr,
	&sensor_dev_attr_in7_alarm.dev_attr.attr,

	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_temp1_max.dev_attr.attr,
	&sensor_dev_attr_temp2_max.dev_attr.attr,
	&sensor_dev_attr_temp3_max.dev_attr.attr,
	&sensor_dev_attr_temp1_min.dev_attr.attr,
	&sensor_dev_attr_temp2_min.dev_attr.attr,
	&sensor_dev_attr_temp3_min.dev_attr.attr,
	&sensor_dev_attr_temp1_type.dev_attr.attr,
	&sensor_dev_attr_temp2_type.dev_attr.attr,
	&sensor_dev_attr_temp3_type.dev_attr.attr,
	&sensor_dev_attr_temp1_alarm.dev_attr.attr,
	&sensor_dev_attr_temp2_alarm.dev_attr.attr,
	&sensor_dev_attr_temp3_alarm.dev_attr.attr,

	&dev_attr_alarms.attr,
	&dev_attr_name.attr,
	NULL
};

static const struct attribute_group it87_group = {
	.attrs = it87_attributes,
};

static struct attribute *it87_attributes_opt[] = {
	&sensor_dev_attr_fan1_input16.dev_attr.attr,
	&sensor_dev_attr_fan1_min16.dev_attr.attr,
	&sensor_dev_attr_fan2_input16.dev_attr.attr,
	&sensor_dev_attr_fan2_min16.dev_attr.attr,
	&sensor_dev_attr_fan3_input16.dev_attr.attr,
	&sensor_dev_attr_fan3_min16.dev_attr.attr,
	&sensor_dev_attr_fan4_input16.dev_attr.attr,
	&sensor_dev_attr_fan4_min16.dev_attr.attr,
	&sensor_dev_attr_fan5_input16.dev_attr.attr,
	&sensor_dev_attr_fan5_min16.dev_attr.attr,

	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan1_min.dev_attr.attr,
	&sensor_dev_attr_fan1_div.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_fan2_min.dev_attr.attr,
	&sensor_dev_attr_fan2_div.dev_attr.attr,
	&sensor_dev_attr_fan3_input.dev_attr.attr,
	&sensor_dev_attr_fan3_min.dev_attr.attr,
	&sensor_dev_attr_fan3_div.dev_attr.attr,

	&sensor_dev_attr_fan1_alarm.dev_attr.attr,
	&sensor_dev_attr_fan2_alarm.dev_attr.attr,
	&sensor_dev_attr_fan3_alarm.dev_attr.attr,
	&sensor_dev_attr_fan4_alarm.dev_attr.attr,
	&sensor_dev_attr_fan5_alarm.dev_attr.attr,

	&sensor_dev_attr_pwm1_enable.dev_attr.attr,
	&sensor_dev_attr_pwm2_enable.dev_attr.attr,
	&sensor_dev_attr_pwm3_enable.dev_attr.attr,
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm2.dev_attr.attr,
	&sensor_dev_attr_pwm3.dev_attr.attr,
	&dev_attr_pwm1_freq.attr,
	&dev_attr_pwm2_freq.attr,
	&dev_attr_pwm3_freq.attr,

	&dev_attr_vrm.attr,
	&dev_attr_cpu0_vid.attr,
	NULL
};

static const struct attribute_group it87_group_opt = {
	.attrs = it87_attributes_opt,
};

static int __init it87_find(unsigned short *address,
	struct it87_sio_data *sio_data)
{
	int err = -ENODEV;
	u16 chip_type;
	const char *board_vendor, *board_name;

	superio_enter();
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
	case 0xffff:	 
		goto exit;
	default:
		pr_debug(DRVNAME ": Unsupported chip (DEVID=0x%x)\n",
			 chip_type);
		goto exit;
	}

	superio_select(PME);
	if (!(superio_inb(IT87_ACT_REG) & 0x01)) {
		pr_info("it87: Device not activated, skipping\n");
		goto exit;
	}

	*address = superio_inw(IT87_BASE_REG) & ~(IT87_EXTENT - 1);
	if (*address == 0) {
		pr_info("it87: Base address not set, skipping\n");
		goto exit;
	}

	err = 0;
	sio_data->revision = superio_inb(DEVREV) & 0x0f;
	pr_info("it87: Found IT%04xF chip at 0x%x, revision %d\n",
		chip_type, *address, sio_data->revision);

	if (sio_data->type != it87) {
		int reg;

		superio_select(GPIO);
		if (sio_data->type == it8718 || sio_data->type == it8720)
			sio_data->vid_value = superio_inb(IT87_SIO_VID_REG);

		reg = superio_inb(IT87_SIO_PINX2_REG);
		if (reg & (1 << 0))
			pr_info("it87: in3 is VCC (+5V)\n");
		if (reg & (1 << 1))
			pr_info("it87: in7 is VCCH (+5V Stand-By)\n");
	}

	board_vendor = dmi_get_system_info(DMI_BOARD_VENDOR);
	board_name = dmi_get_system_info(DMI_BOARD_NAME);
	if (board_vendor && board_name) {
		if (strcmp(board_vendor, "nVIDIA") == 0
		 && strcmp(board_name, "FN68PT") == 0) {
			 
			pr_info("it87: Disabling pwm2 due to "
				"hardware constraints\n");
			sio_data->skip_pwm = (1 << 1);
		}
	}

exit:
	superio_exit();
	return err;
}

static int __devinit it87_probe(struct platform_device *pdev)
{
	struct it87_data *data;
	struct resource *res;
	struct device *dev = &pdev->dev;
	struct it87_sio_data *sio_data = dev->platform_data;
	int err = 0;
	int enable_pwm_interface;
	static const char *names[] = {
		"it87",
		"it8712",
		"it8716",
		"it8718",
		"it8720",
	};

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (!request_region(res->start, IT87_EC_EXTENT, DRVNAME)) {
		dev_err(dev, "Failed to request region 0x%lx-0x%lx\n",
			(unsigned long)res->start,
			(unsigned long)(res->start + IT87_EC_EXTENT - 1));
		err = -EBUSY;
		goto ERROR0;
	}

	if (!(data = kzalloc(sizeof(struct it87_data), GFP_KERNEL))) {
		err = -ENOMEM;
		goto ERROR1;
	}

	data->addr = res->start;
	data->type = sio_data->type;
	data->revision = sio_data->revision;
	data->name = names[sio_data->type];

	if ((it87_read_value(data, IT87_REG_CONFIG) & 0x80)
	 || it87_read_value(data, IT87_REG_CHIPID) != 0x90) {
		err = -ENODEV;
		goto ERROR2;
	}

	platform_set_drvdata(pdev, data);

	mutex_init(&data->update_lock);

	enable_pwm_interface = it87_check_pwm(dev);

	it87_init_device(pdev);

	if ((err = sysfs_create_group(&dev->kobj, &it87_group)))
		goto ERROR2;

	if (has_16bit_fans(data)) {
		 
		if (data->has_fan & (1 << 0)) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_fan1_input16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan1_min16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan1_alarm.dev_attr)))
				goto ERROR4;
		}
		if (data->has_fan & (1 << 1)) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_fan2_input16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan2_min16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan2_alarm.dev_attr)))
				goto ERROR4;
		}
		if (data->has_fan & (1 << 2)) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_fan3_input16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan3_min16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan3_alarm.dev_attr)))
				goto ERROR4;
		}
		if (data->has_fan & (1 << 3)) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_fan4_input16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan4_min16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan4_alarm.dev_attr)))
				goto ERROR4;
		}
		if (data->has_fan & (1 << 4)) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_fan5_input16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan5_min16.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan5_alarm.dev_attr)))
				goto ERROR4;
		}
	} else {
		 
		if (data->has_fan & (1 << 0)) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_fan1_input.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan1_min.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan1_div.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan1_alarm.dev_attr)))
				goto ERROR4;
		}
		if (data->has_fan & (1 << 1)) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_fan2_input.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan2_min.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan2_div.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan2_alarm.dev_attr)))
				goto ERROR4;
		}
		if (data->has_fan & (1 << 2)) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_fan3_input.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan3_min.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan3_div.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_fan3_alarm.dev_attr)))
				goto ERROR4;
		}
	}

	if (enable_pwm_interface) {
		if (!(sio_data->skip_pwm & (1 << 0))) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_pwm1_enable.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_pwm1.dev_attr))
			 || (err = device_create_file(dev,
			     &dev_attr_pwm1_freq)))
				goto ERROR4;
		}
		if (!(sio_data->skip_pwm & (1 << 1))) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_pwm2_enable.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_pwm2.dev_attr))
			 || (err = device_create_file(dev,
			     &dev_attr_pwm2_freq)))
				goto ERROR4;
		}
		if (!(sio_data->skip_pwm & (1 << 2))) {
			if ((err = device_create_file(dev,
			     &sensor_dev_attr_pwm3_enable.dev_attr))
			 || (err = device_create_file(dev,
			     &sensor_dev_attr_pwm3.dev_attr))
			 || (err = device_create_file(dev,
			     &dev_attr_pwm3_freq)))
				goto ERROR4;
		}
	}

	if (data->type == it8712 || data->type == it8716
	 || data->type == it8718 || data->type == it8720) {
		data->vrm = vid_which_vrm();
		 
		data->vid = sio_data->vid_value;
		if ((err = device_create_file(dev,
		     &dev_attr_vrm))
		 || (err = device_create_file(dev,
		     &dev_attr_cpu0_vid)))
			goto ERROR4;
	}

	data->hwmon_dev = hwmon_device_register(dev);
	if (IS_ERR(data->hwmon_dev)) {
		err = PTR_ERR(data->hwmon_dev);
		goto ERROR4;
	}

	return 0;

ERROR4:
	sysfs_remove_group(&dev->kobj, &it87_group);
	sysfs_remove_group(&dev->kobj, &it87_group_opt);
ERROR2:
	platform_set_drvdata(pdev, NULL);
	kfree(data);
ERROR1:
	release_region(res->start, IT87_EC_EXTENT);
ERROR0:
	return err;
}

static int __devexit it87_remove(struct platform_device *pdev)
{
	struct it87_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &it87_group);
	sysfs_remove_group(&pdev->dev.kobj, &it87_group_opt);

	release_region(data->addr, IT87_EC_EXTENT);
	platform_set_drvdata(pdev, NULL);
	kfree(data);

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

static int __devinit it87_check_pwm(struct device *dev)
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
				dev_info(dev, "Reconfiguring PWM to "
					 "active high polarity\n");
				it87_write_value(data, IT87_REG_FAN_CTL,
						 tmp | 0x87);
				for (i = 0; i < 3; i++)
					it87_write_value(data,
							 IT87_REG_PWM(i),
							 0x7f & ~pwm[i]);
				return 1;
			}

			dev_info(dev, "PWM configuration is "
				 "too broken to be fixed\n");
		}

		dev_info(dev, "Detected broken BIOS "
			 "defaults, disabling PWM interface\n");
		return 0;
	} else if (fix_pwm_polarity) {
		dev_info(dev, "PWM configuration looks "
			 "sane, won't touch\n");
	}

	return 1;
}

static void __devinit it87_init_device(struct platform_device *pdev)
{
	struct it87_data *data = platform_get_drvdata(pdev);
	int tmp, i;

	for (i = 0; i < 3; i++) {
		data->manual_pwm_ctl[i] = 0xff;
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

	tmp = it87_read_value(data, IT87_REG_TEMP_ENABLE);
	if ((tmp & 0x3f) == 0) {
		 
		tmp = (tmp & 0xc0) | 0x2a;
		it87_write_value(data, IT87_REG_TEMP_ENABLE, tmp);
	}
	data->sensor = tmp;

	tmp = it87_read_value(data, IT87_REG_VIN_ENABLE);
	if ((tmp & 0xff) == 0) {
		 
		it87_write_value(data, IT87_REG_VIN_ENABLE, 0xff);
	}

	data->fan_main_ctrl = it87_read_value(data, IT87_REG_FAN_MAIN_CTRL);
	if ((data->fan_main_ctrl & 0x70) == 0) {
		 
		data->fan_main_ctrl |= 0x70;
		it87_write_value(data, IT87_REG_FAN_MAIN_CTRL, data->fan_main_ctrl);
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
		 
		if (data->type != it87) {
			if (tmp & (1 << 4))
				data->has_fan |= (1 << 3);  
			if (tmp & (1 << 5))
				data->has_fan |= (1 << 4);  
		}
	}

	for (i = 0; i < 3; i++) {
		if (data->fan_main_ctrl & (1 << i)) {
			 
			tmp = it87_read_value(data, IT87_REG_PWM(i));
			if (tmp & 0x80) {
				 
			} else {
				 
				data->manual_pwm_ctl[i] = PWM_FROM_REG(tmp);
			}
		}
 	}

	it87_write_value(data, IT87_REG_CONFIG,
			 (it87_read_value(data, IT87_REG_CONFIG) & 0x36)
			 | (update_vbat ? 0x41 : 0x01));
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
			data->in[i] =
			    it87_read_value(data, IT87_REG_VIN(i));
			data->in_min[i] =
			    it87_read_value(data, IT87_REG_VIN_MIN(i));
			data->in_max[i] =
			    it87_read_value(data, IT87_REG_VIN_MAX(i));
		}
		 
		data->in[8] =
		    it87_read_value(data, IT87_REG_VIN(8));

		for (i = 0; i < 5; i++) {
			 
			if (!(data->has_fan & (1 << i)))
				continue;

			data->fan_min[i] =
			    it87_read_value(data, IT87_REG_FAN_MIN[i]);
			data->fan[i] = it87_read_value(data,
				       IT87_REG_FAN[i]);
			 
			if (has_16bit_fans(data)) {
				data->fan[i] |= it87_read_value(data,
						IT87_REG_FANX[i]) << 8;
				data->fan_min[i] |= it87_read_value(data,
						IT87_REG_FANX_MIN[i]) << 8;
			}
		}
		for (i = 0; i < 3; i++) {
			data->temp[i] =
			    it87_read_value(data, IT87_REG_TEMP(i));
			data->temp_high[i] =
			    it87_read_value(data, IT87_REG_TEMP_HIGH(i));
			data->temp_low[i] =
			    it87_read_value(data, IT87_REG_TEMP_LOW(i));
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
		data->fan_main_ctrl = it87_read_value(data,
				IT87_REG_FAN_MAIN_CTRL);
		data->fan_ctl = it87_read_value(data, IT87_REG_FAN_CTL);

		data->sensor = it87_read_value(data, IT87_REG_TEMP_ENABLE);
		 
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
		printk(KERN_ERR DRVNAME ": Device allocation failed\n");
		goto exit;
	}

	err = platform_device_add_resources(pdev, &res, 1);
	if (err) {
		printk(KERN_ERR DRVNAME ": Device resource addition failed "
		       "(%d)\n", err);
		goto exit_device_put;
	}

	err = platform_device_add_data(pdev, sio_data,
				       sizeof(struct it87_sio_data));
	if (err) {
		printk(KERN_ERR DRVNAME ": Platform data allocation failed\n");
		goto exit_device_put;
	}

	err = platform_device_add(pdev);
	if (err) {
		printk(KERN_ERR DRVNAME ": Device addition failed (%d)\n",
		       err);
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
	unsigned short isa_address=0;
	struct it87_sio_data sio_data;

	memset(&sio_data, 0, sizeof(struct it87_sio_data));
	err = it87_find(&isa_address, &sio_data);
	if (err)
		return err;
	err = platform_driver_register(&it87_driver);
	if (err)
		return err;

	err = it87_device_add(isa_address, &sio_data);
	if (err){
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

#ifdef MY_DEF_HERE
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

#ifdef MY_DEF_HERE
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

MODULE_AUTHOR("Chris Gauthron, "
	      "Jean Delvare <khali@linux-fr.org>");
MODULE_DESCRIPTION("IT8705F/8712F/8716F/8718F/8720F/8726F, SiS950 driver");
module_param(update_vbat, bool, 0);
MODULE_PARM_DESC(update_vbat, "Update vbat if set else return powerup value");
module_param(fix_pwm_polarity, bool, 0);
MODULE_PARM_DESC(fix_pwm_polarity, "Force PWM polarity to active high (DANGEROUS)");
MODULE_LICENSE("GPL");

module_init(sm_it87_init);
module_exit(sm_it87_exit);

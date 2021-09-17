#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __STI_THERMAL_SYSCFG_H
#define __STI_THERMAL_SYSCFG_H

#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/thermal.h>

enum {
	 
	TEMP_PWR = 0, INT_THRESH_HI = 0,
	DCORRECT,
	OVERFLOW,
	DATA,
#ifdef MY_DEF_HERE
	DATARDY,
#endif  
	INT_ENABLE,
	INT_THRESH_LOW,
#ifdef MY_DEF_HERE
	DC_CALIB,
#endif  
	 
	MAX_REGFIELDS
};

#ifdef MY_DEF_HERE
enum {
	TH_REGS,
	TH_CALIB,
	MAX_IOBASE
};
#endif  

enum st_thermal_power_state {
	POWER_OFF = 0,
	POWER_ON = 1
};

struct st_thermal_sensor;

struct st_thermal_sensor_ops {
	int (*power_ctrl)(struct st_thermal_sensor *,
		enum st_thermal_power_state);
	int (*alloc_regfields)(struct st_thermal_sensor *);
	int (*do_memmap_regmap)(struct st_thermal_sensor *);
	int (*register_irq)(struct st_thermal_sensor *);
	int (*enable_irq)(struct st_thermal_sensor *);
	void (*clear_irq)(struct thermal_zone_device *th, unsigned int);
};

struct st_thermal_compat_data {
	const struct reg_field *reg_fields;
	char *sys_compat;
	struct st_thermal_sensor_ops *ops;
	unsigned int calibration_val;
	int temp_adjust_val;
	unsigned int crit_temp;
	unsigned int passive_temp;
};

struct st_thermal_sensor {
	struct device *dev;
	struct thermal_zone_device *th_dev;
	struct thermal_cooling_device *cdev;
	struct st_thermal_sensor_ops *ops;
	const struct st_thermal_compat_data *data;
	struct clk *clk;
	unsigned int passive_temp;
#ifdef MY_DEF_HERE
	unsigned int dc_offset;
	struct regmap *regmap[MAX_IOBASE];
#else  
	struct regmap *regmap;
#endif  
	struct regmap_field *pwr;
	struct regmap_field *dcorrect;
	struct regmap_field *overflow;
	struct regmap_field *temp_data;
#ifdef MY_DEF_HERE
	struct regmap_field *datardy;
#endif  
	struct regmap_field *int_thresh_hi;
	struct regmap_field *int_thresh_low;
	struct regmap_field *int_enable;
#ifdef MY_DEF_HERE
	struct regmap_field *dc_calib;
#endif  
	int irq;
#ifdef MY_DEF_HERE
#else  
	void __iomem *mmio_base;
#endif  
};

#define thzone_to_sensor(th)			((th)->devdata)
#define sensor_to_dev(sensor)			((sensor)->dev)
#define mcelsius(temp)				((temp) * 1000)

#ifdef CONFIG_ST_THERMAL_SYSCFG
extern struct st_thermal_compat_data st_415sas_data;
extern struct st_thermal_compat_data st_415mpe_data;
extern struct st_thermal_compat_data st_416sas_data;
extern struct st_thermal_compat_data st_127_data;
#endif
#ifdef CONFIG_ST_THERMAL_MEMMAP
extern struct st_thermal_compat_data st_416mpe_data;
extern struct st_thermal_compat_data st_407_data;
#endif

int st_thermal_common_alloc_regfields(struct st_thermal_sensor *sensor);

#endif  

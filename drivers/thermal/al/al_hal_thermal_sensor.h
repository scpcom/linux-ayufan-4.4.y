 
#ifndef _AL_HAL_THERMAL_SENSE_H_
#define _AL_HAL_THERMAL_SENSE_H_

#include "al_hal_common.h"

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
#define AL_THERMAL_SENSOR_LOW_THRESHOLD_DISABLE		-1000
 
#define AL_THERMAL_SENSOR_HIGH_THRESHOLD_DISABLE	1000
#endif

struct al_thermal_sensor_handle {
	struct al_thermal_sensor_regs __iomem	*regs;
	uint32_t readout_raw;
};

int al_thermal_sensor_handle_init(
	struct al_thermal_sensor_handle	*thermal_sensor_handle,
	void __iomem			*thermal_sensor_reg_base);

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
void al_thermal_sensor_trim_set(
	struct al_thermal_sensor_handle *thermal_sensor_handle,
	unsigned int			trim);

void al_thermal_sensor_threshold_config(
	struct al_thermal_sensor_handle *thermal_sensor_handle,
	int				low_temp_threshold,
	int				high_temp_threshold);
#endif

void al_thermal_sensor_enable_set(
	struct al_thermal_sensor_handle	*thermal_sensor_handle,
	int				enable);

int al_thermal_sensor_is_ready(
	struct al_thermal_sensor_handle	*thermal_sensor_handle);

void al_thermal_sensor_trigger_once(
	struct al_thermal_sensor_handle	*thermal_sensor_handle);

void al_thermal_sensor_trigger_continuous(
	struct al_thermal_sensor_handle	*thermal_sensor_handle);

int al_thermal_sensor_readout_is_valid(
	struct al_thermal_sensor_handle	*thermal_sensor_handle);

int al_thermal_sensor_readout_get(
	struct al_thermal_sensor_handle	*thermal_sensor_handle);

#endif
 
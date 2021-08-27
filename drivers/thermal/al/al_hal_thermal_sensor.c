 
#include "al_hal_thermal_sensor.h"
#include "al_hal_thermal_sensor_regs.h"

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
#define OFFSET		1154	 
#define MULTIPLIER	3523	 

#define READOUT_TO_CELCIUS(readout)		\
	((((((int)(readout)) * MULTIPLIER) / 4096) - OFFSET) / 10)

#define CELCIUS_TO_READOUT(celcius)		\
	((((10 * (celcius)) + OFFSET) * 4096) / MULTIPLIER)

#define AL_THERMAL_SENSOR_MIN_THRESHOLD_VAL	0
#define AL_THERMAL_SENSOR_MAX_THRESHOLD_VAL	0xfff
#else
#define READOUT_TO_CELCIUS(readout)		\
	(((((int)(readout)) * 244) / 4096) - 56)
#endif

int al_thermal_sensor_handle_init(
	struct al_thermal_sensor_handle	*thermal_sensor_handle,
	void __iomem		*thermal_sensor_reg_base)
{
	al_assert(thermal_sensor_handle);
	al_assert(thermal_sensor_reg_base);

	thermal_sensor_handle->regs = (struct al_thermal_sensor_regs __iomem *)
		thermal_sensor_reg_base;

	return 0;
}

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
void al_thermal_sensor_trim_set(
	struct al_thermal_sensor_handle *thermal_sensor_handle,
	unsigned int			trim)
{
	struct al_thermal_sensor_regs __iomem *regs;

	al_assert(thermal_sensor_handle);

	regs = thermal_sensor_handle->regs;

	al_reg_write32_masked(
		&regs->unit.static_config,
		THERMAL_SENSE_UNIT_STATIC_CONFIG_TRIM_MASK,
		trim << THERMAL_SENSE_UNIT_STATIC_CONFIG_TRIM_SHIFT);
}

void al_thermal_sensor_threshold_config(
	struct al_thermal_sensor_handle *thermal_sensor_handle,
	int				low_temp_threshold,
	int				high_temp_threshold)
{
	struct al_thermal_sensor_regs __iomem *regs;
	unsigned int low_threshold =
		(low_temp_threshold ==
		 AL_THERMAL_SENSOR_LOW_THRESHOLD_DISABLE) ?
		AL_THERMAL_SENSOR_MIN_THRESHOLD_VAL :
		(unsigned int)CELCIUS_TO_READOUT(low_temp_threshold);
	unsigned int high_threshold =
		(high_temp_threshold ==
		 AL_THERMAL_SENSOR_HIGH_THRESHOLD_DISABLE) ?
		AL_THERMAL_SENSOR_MAX_THRESHOLD_VAL :
		(unsigned int)CELCIUS_TO_READOUT(high_temp_threshold);

	al_assert(thermal_sensor_handle);

	regs = thermal_sensor_handle->regs;

	al_reg_write32(
		&regs->unit.threshold_config,
		(low_threshold <<
		THERMAL_SENSE_UNIT_THRESHOLD_CONFIG_LOW_TEMP_THRESHOLD_SHIFT) |
		(high_threshold <<
		THERMAL_SENSE_UNIT_THRESHOLD_CONFIG_HIGH_TEMP_THRESHOLD_SHIFT));
}
#endif

void al_thermal_sensor_enable_set(
	struct al_thermal_sensor_handle	*thermal_sensor_handle,
	int				enable)
{
	struct al_thermal_sensor_regs __iomem *regs;

	al_assert(thermal_sensor_handle);

	regs = thermal_sensor_handle->regs;

	if (!enable)
		al_reg_write32(&regs->unit.dynamic_config, 0);

	al_reg_write32_masked(
		&regs->unit.static_config,
		THERMAL_SENSE_UNIT_STATIC_CONFIG_POWER_DOWN |
		THERMAL_SENSE_UNIT_STATIC_CONFIG_ENABLE,
		enable ?
		THERMAL_SENSE_UNIT_STATIC_CONFIG_ENABLE :
		THERMAL_SENSE_UNIT_STATIC_CONFIG_POWER_DOWN);
}

int al_thermal_sensor_is_ready(
	struct al_thermal_sensor_handle	*thermal_sensor_handle)
{
	struct al_thermal_sensor_regs __iomem *regs;
	uint32_t status_reg_val;
	int is_valid;

	al_assert(thermal_sensor_handle);

	regs = thermal_sensor_handle->regs;

	status_reg_val = al_reg_read32(
		&regs->unit.status);

	is_valid = ((status_reg_val & THERMAL_SENSE_UNIT_STATUS_T_PWR_OK) &&
		(status_reg_val & THERMAL_SENSE_UNIT_STATUS_T_INIT_DONE));

	return is_valid;
}

void al_thermal_sensor_trigger_once(
	struct al_thermal_sensor_handle	*thermal_sensor_handle)
{
	struct al_thermal_sensor_regs __iomem *regs;

	al_assert(thermal_sensor_handle);

	regs = thermal_sensor_handle->regs;

	al_reg_write32(&regs->unit.dynamic_config,
		THERMAL_SENSE_UNIT_DYNAMIC_CONFIG_RUN_ONCE);
}

void al_thermal_sensor_trigger_continuous(
	struct al_thermal_sensor_handle	*thermal_sensor_handle)
{
	struct al_thermal_sensor_regs __iomem *regs;

	al_assert(thermal_sensor_handle);

	regs = thermal_sensor_handle->regs;

	al_reg_write32(&regs->unit.dynamic_config,
		THERMAL_SENSE_UNIT_DYNAMIC_CONFIG_KEEP_RUNNING);
}

int al_thermal_sensor_readout_is_valid(
	struct al_thermal_sensor_handle	*thermal_sensor_handle)
{
	struct al_thermal_sensor_regs __iomem *regs;
	uint32_t status_reg_val;
	int is_valid;

	al_assert(thermal_sensor_handle);

	regs = thermal_sensor_handle->regs;

	status_reg_val = al_reg_read32(
		&regs->unit.status);

	is_valid = ((status_reg_val & THERMAL_SENSE_UNIT_STATUS_T_PWR_OK) &&
		(status_reg_val & THERMAL_SENSE_UNIT_STATUS_T_INIT_DONE) &&
		(status_reg_val & THERMAL_SENSE_UNIT_STATUS_T_VALID));

	if (is_valid)
		thermal_sensor_handle->readout_raw = (status_reg_val &
			THERMAL_SENSE_UNIT_STATUS_T_RESULT_MASK) >>
			THERMAL_SENSE_UNIT_STATUS_T_RESULT_SHIFT;

	return is_valid;
}

int al_thermal_sensor_readout_get(
	struct al_thermal_sensor_handle	*thermal_sensor_handle)
{
	int readout;

	al_assert(thermal_sensor_handle);

	readout = READOUT_TO_CELCIUS(thermal_sensor_handle->readout_raw);

	return readout;
}

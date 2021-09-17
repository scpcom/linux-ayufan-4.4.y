#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LINUX_REGULATOR_CONSUMER_H_
#define __LINUX_REGULATOR_CONSUMER_H_

struct device;
struct notifier_block;

#define REGULATOR_MODE_FAST			0x1
#define REGULATOR_MODE_NORMAL			0x2
#define REGULATOR_MODE_IDLE			0x4
#define REGULATOR_MODE_STANDBY			0x8

#define REGULATOR_EVENT_UNDER_VOLTAGE		0x01
#define REGULATOR_EVENT_OVER_CURRENT		0x02
#define REGULATOR_EVENT_REGULATION_OUT		0x04
#define REGULATOR_EVENT_FAIL			0x08
#define REGULATOR_EVENT_OVER_TEMP		0x10
#define REGULATOR_EVENT_FORCE_DISABLE		0x20
#define REGULATOR_EVENT_VOLTAGE_CHANGE		0x40
#define REGULATOR_EVENT_DISABLE 		0x80

struct regulator;

struct regulator_bulk_data {
	const char *supply;
	struct regulator *consumer;

	int ret;
};

#if defined(CONFIG_REGULATOR)

struct regulator *__must_check regulator_get(struct device *dev,
					     const char *id);
struct regulator *__must_check devm_regulator_get(struct device *dev,
					     const char *id);
struct regulator *__must_check regulator_get_exclusive(struct device *dev,
						       const char *id);
#if defined(MY_ABC_HERE)
struct regulator *__must_check regulator_get_optional(struct device *dev,
						      const char *id);
struct regulator *__must_check devm_regulator_get_optional(struct device *dev,
							   const char *id);
#endif  
void regulator_put(struct regulator *regulator);
void devm_regulator_put(struct regulator *regulator);

int __must_check regulator_enable(struct regulator *regulator);
int regulator_disable(struct regulator *regulator);
int regulator_force_disable(struct regulator *regulator);
int regulator_is_enabled(struct regulator *regulator);
int regulator_disable_deferred(struct regulator *regulator, int ms);

int __must_check regulator_bulk_get(struct device *dev, int num_consumers,
				    struct regulator_bulk_data *consumers);
int __must_check devm_regulator_bulk_get(struct device *dev, int num_consumers,
					 struct regulator_bulk_data *consumers);
int __must_check regulator_bulk_enable(int num_consumers,
				       struct regulator_bulk_data *consumers);
int regulator_bulk_disable(int num_consumers,
			   struct regulator_bulk_data *consumers);
int regulator_bulk_force_disable(int num_consumers,
			   struct regulator_bulk_data *consumers);
void regulator_bulk_free(int num_consumers,
			 struct regulator_bulk_data *consumers);

int regulator_can_change_voltage(struct regulator *regulator);
int regulator_count_voltages(struct regulator *regulator);
int regulator_list_voltage(struct regulator *regulator, unsigned selector);
int regulator_is_supported_voltage(struct regulator *regulator,
				   int min_uV, int max_uV);
int regulator_set_voltage(struct regulator *regulator, int min_uV, int max_uV);
int regulator_set_voltage_time(struct regulator *regulator,
			       int old_uV, int new_uV);
int regulator_get_voltage(struct regulator *regulator);
int regulator_sync_voltage(struct regulator *regulator);
int regulator_set_current_limit(struct regulator *regulator,
			       int min_uA, int max_uA);
int regulator_get_current_limit(struct regulator *regulator);

int regulator_set_mode(struct regulator *regulator, unsigned int mode);
unsigned int regulator_get_mode(struct regulator *regulator);
int regulator_set_optimum_mode(struct regulator *regulator, int load_uA);

int regulator_allow_bypass(struct regulator *regulator, bool allow);

int regulator_register_notifier(struct regulator *regulator,
			      struct notifier_block *nb);
int regulator_unregister_notifier(struct regulator *regulator,
				struct notifier_block *nb);

void *regulator_get_drvdata(struct regulator *regulator);
void regulator_set_drvdata(struct regulator *regulator, void *data);

#else

static inline struct regulator *__must_check regulator_get(struct device *dev,
	const char *id)
{
	 
	return NULL;
}

static inline struct regulator *__must_check
devm_regulator_get(struct device *dev, const char *id)
{
	return NULL;
}

#if defined(MY_ABC_HERE)
static inline struct regulator *__must_check
regulator_get_exclusive(struct device *dev, const char *id)
{
	return NULL;
}

static inline struct regulator *__must_check
regulator_get_optional(struct device *dev, const char *id)
{
	return NULL;
}

static inline struct regulator *__must_check
devm_regulator_get_optional(struct device *dev, const char *id)
{
	return NULL;
}
#endif  

static inline void regulator_put(struct regulator *regulator)
{
}

static inline void devm_regulator_put(struct regulator *regulator)
{
}

static inline int regulator_enable(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_disable(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_force_disable(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_disable_deferred(struct regulator *regulator,
					     int ms)
{
	return 0;
}

static inline int regulator_is_enabled(struct regulator *regulator)
{
	return 1;
}

static inline int regulator_bulk_get(struct device *dev,
				     int num_consumers,
				     struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int devm_regulator_bulk_get(struct device *dev, int num_consumers,
					  struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int regulator_bulk_enable(int num_consumers,
					struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int regulator_bulk_disable(int num_consumers,
					 struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline int regulator_bulk_force_disable(int num_consumers,
					struct regulator_bulk_data *consumers)
{
	return 0;
}

static inline void regulator_bulk_free(int num_consumers,
				       struct regulator_bulk_data *consumers)
{
}

static inline int regulator_set_voltage(struct regulator *regulator,
					int min_uV, int max_uV)
{
	return 0;
}

#if defined(MY_ABC_HERE)
static inline int regulator_set_voltage_time(struct regulator *regulator,
					     int old_uV, int new_uV)
{
	return 0;
}
#endif  

static inline int regulator_get_voltage(struct regulator *regulator)
{
	return -EINVAL;
}

static inline int regulator_is_supported_voltage(struct regulator *regulator,
				   int min_uV, int max_uV)
{
	return 0;
}

static inline int regulator_set_current_limit(struct regulator *regulator,
					     int min_uA, int max_uA)
{
	return 0;
}

static inline int regulator_get_current_limit(struct regulator *regulator)
{
	return 0;
}

static inline int regulator_set_mode(struct regulator *regulator,
	unsigned int mode)
{
	return 0;
}

static inline unsigned int regulator_get_mode(struct regulator *regulator)
{
	return REGULATOR_MODE_NORMAL;
}

static inline int regulator_set_optimum_mode(struct regulator *regulator,
					int load_uA)
{
	return REGULATOR_MODE_NORMAL;
}

static inline int regulator_allow_bypass(struct regulator *regulator,
					 bool allow)
{
	return 0;
}

static inline int regulator_register_notifier(struct regulator *regulator,
			      struct notifier_block *nb)
{
	return 0;
}

static inline int regulator_unregister_notifier(struct regulator *regulator,
				struct notifier_block *nb)
{
	return 0;
}

static inline void *regulator_get_drvdata(struct regulator *regulator)
{
	return NULL;
}

static inline void regulator_set_drvdata(struct regulator *regulator,
	void *data)
{
}

static inline int regulator_count_voltages(struct regulator *regulator)
{
	return 0;
}
#endif

static inline int regulator_set_voltage_tol(struct regulator *regulator,
					    int new_uV, int tol_uV)
{
	return regulator_set_voltage(regulator,
				     new_uV - tol_uV, new_uV + tol_uV);
}

static inline int regulator_is_supported_voltage_tol(struct regulator *regulator,
						     int target_uV, int tol_uV)
{
	return regulator_is_supported_voltage(regulator,
					      target_uV - tol_uV,
					      target_uV + tol_uV);
}

#endif

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __CPU_COOLING_H__
#define __CPU_COOLING_H__

#if defined(MY_ABC_HERE)
#include <linux/of.h>
#endif  
#include <linux/thermal.h>
#include <linux/cpumask.h>

#ifdef CONFIG_CPU_THERMAL
 
struct thermal_cooling_device *
cpufreq_cooling_register(const struct cpumask *clip_cpus);

#if defined(MY_ABC_HERE)
 
#ifdef CONFIG_THERMAL_OF
struct thermal_cooling_device *
of_cpufreq_cooling_register(struct device_node *np,
			    const struct cpumask *clip_cpus);
#else
static inline struct thermal_cooling_device *
of_cpufreq_cooling_register(struct device_node *np,
			    const struct cpumask *clip_cpus)
{
	return NULL;
}
#endif
#endif  

void cpufreq_cooling_unregister(struct thermal_cooling_device *cdev);

unsigned long cpufreq_cooling_get_level(unsigned int cpu, unsigned int freq);
#else  
static inline struct thermal_cooling_device *
cpufreq_cooling_register(const struct cpumask *clip_cpus)
{
	return NULL;
}
#if defined(MY_ABC_HERE)
static inline struct thermal_cooling_device *
of_cpufreq_cooling_register(struct device_node *np,
			    const struct cpumask *clip_cpus)
{
	return NULL;
}
#endif  
static inline
void cpufreq_cooling_unregister(struct thermal_cooling_device *cdev)
{
	return;
}
static inline
unsigned long cpufreq_cooling_get_level(unsigned int cpu, unsigned int freq)
{
	return THERMAL_CSTATE_INVALID;
}
#endif	 

#endif  

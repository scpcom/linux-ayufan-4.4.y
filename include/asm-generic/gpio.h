#ifndef _ASM_GENERIC_GPIO_H
#define _ASM_GENERIC_GPIO_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>

#ifdef CONFIG_GPIOLIB

#include <linux/compiler.h>

#ifndef ARCH_NR_GPIOS
#define ARCH_NR_GPIOS		256
#endif

static inline int gpio_is_valid(int number)
{
	 
	return ((unsigned)number) < ARCH_NR_GPIOS;
}

struct seq_file;
struct module;

struct gpio_chip {
	const char		*label;
	struct device		*dev;
	struct module		*owner;

	int			(*request)(struct gpio_chip *chip,
						unsigned offset);
	void			(*free)(struct gpio_chip *chip,
						unsigned offset);

	int			(*direction_input)(struct gpio_chip *chip,
						unsigned offset);
	int			(*get)(struct gpio_chip *chip,
						unsigned offset);
	int			(*direction_output)(struct gpio_chip *chip,
						unsigned offset, int value);
	void			(*set)(struct gpio_chip *chip,
						unsigned offset, int value);

	int			(*to_irq)(struct gpio_chip *chip,
						unsigned offset);

	void			(*dbg_show)(struct seq_file *s,
						struct gpio_chip *chip);

#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
	int				(*iInterruptClear)(struct gpio_chip *Chip, const unsigned int uiGpio);
#endif
#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
	void			(*iHWReset)(struct gpio_chip *Chip, const unsigned int uiGpio);
#endif

	int			base;
	u16			ngpio;
	char			**names;
	unsigned		can_sleep:1;
	unsigned		exported:1;
};

extern const char *gpiochip_is_requested(struct gpio_chip *chip,
			unsigned offset);
extern int __must_check gpiochip_reserve(int start, int ngpio);

extern int gpiochip_add(struct gpio_chip *chip);
extern int __must_check gpiochip_remove(struct gpio_chip *chip);

extern int gpio_request(unsigned gpio, const char *label);
extern void gpio_free(unsigned gpio);

extern int gpio_direction_input(unsigned gpio);
extern int gpio_direction_output(unsigned gpio, int value);

extern int gpio_get_value_cansleep(unsigned gpio);
extern void gpio_set_value_cansleep(unsigned gpio, int value);

#ifdef CONFIG_SYNO_QORIQ_FIX_DEEP_WAKE_FAIL
extern int gpio_hw_reset(unsigned gpio);
#endif
#ifdef CONFIG_SYNO_QORIQ_EN_DEEP_WAKE_PIN
extern int iGpioInterruptClear(const unsigned int gpio);
#endif

extern int __gpio_get_value(unsigned gpio);
extern void __gpio_set_value(unsigned gpio, int value);

extern int __gpio_cansleep(unsigned gpio);

extern int __gpio_to_irq(unsigned gpio);

#ifdef CONFIG_GPIO_SYSFS

extern int gpio_export(unsigned gpio, bool direction_may_change);
extern int gpio_export_link(struct device *dev, const char *name,
			unsigned gpio);
extern void gpio_unexport(unsigned gpio);

#endif	 

#else	 

static inline int gpio_is_valid(int number)
{
	 
	return number >= 0;
}

static inline int gpio_cansleep(unsigned gpio)
{
	return 0;
}

static inline int gpio_get_value_cansleep(unsigned gpio)
{
	might_sleep();
	return gpio_get_value(gpio);
}

static inline void gpio_set_value_cansleep(unsigned gpio, int value)
{
	might_sleep();
	gpio_set_value(gpio, value);
}

#endif  

#ifndef CONFIG_GPIO_SYSFS

static inline int gpio_export(unsigned gpio, bool direction_may_change)
{
	return -ENOSYS;
}

static inline int gpio_export_link(struct device *dev, const char *name,
				unsigned gpio)
{
	return -ENOSYS;
}

static inline void gpio_unexport(unsigned gpio)
{
}
#endif	 

#endif  

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef PINCTRL_DEVINFO_H
#define PINCTRL_DEVINFO_H

#ifdef CONFIG_PINCTRL

#include <linux/pinctrl/consumer.h>

struct dev_pin_info {
	struct pinctrl *p;
	struct pinctrl_state *default_state;
#if defined (MY_DEF_HERE)
#ifdef CONFIG_PM
	struct pinctrl_state *sleep_state;
	struct pinctrl_state *idle_state;
#endif
#endif  
};

extern int pinctrl_bind_pins(struct device *dev);

#else

static inline int pinctrl_bind_pins(struct device *dev)
{
	return 0;
}

#endif  
#endif  

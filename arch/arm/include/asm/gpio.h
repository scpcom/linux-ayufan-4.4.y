#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _ARCH_ARM_GPIO_H
#define _ARCH_ARM_GPIO_H


#ifdef CONFIG_SYNO_ALPINE
#ifdef CONFIG_NEED_MACH_GPIO_H
#include <mach/gpio.h>
#endif
#else
#include <mach/gpio.h>
#endif

#ifndef __ARM_GPIOLIB_COMPLEX
 
#include <asm-generic/gpio.h>

#define gpio_get_value  __gpio_get_value
#define gpio_set_value  __gpio_set_value
#define gpio_cansleep   __gpio_cansleep
#endif

#ifndef gpio_to_irq
#define gpio_to_irq	__gpio_to_irq
#endif

#endif  

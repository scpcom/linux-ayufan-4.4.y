#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
  
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/synobios.h>
#include <linux/module.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <mach/hardware.h>

static DEFINE_MUTEX(ox820_gpio_lock);

#ifdef  MY_ABC_HERE
#include <linux/synobios.h>
#endif

typedef struct _SYNO_PLX_GPIO {
	int disk1_act;
	int model_1;
	int model_2;
	int model_3;
	int gpio_a_output;
	int gpio_a_input;
	int gpio_b_output;
	int gpio_b_input;
}SYNO_PLX_GPIO;

#define SYNO_MODEL_DS111j 0x1

static SYNO_PLX_GPIO gPLXGPIO;

int SYNO820GPIOSet(int pin, int val);
int SYNO820GPIOGet(int pin);

static unsigned int SynoModelIDGet(SYNO_PLX_GPIO *pGpio)
{
	return (SYNO820GPIOGet(pGpio->model_3) << 2) | (SYNO820GPIOGet(pGpio->model_2) << 1) | SYNO820GPIOGet(pGpio->model_1);
}

static void ds111j_gpio_set(SYNO_PLX_GPIO *pGpio)
{
	SYNO_PLX_GPIO ds111j_gpio = {
		.disk1_act = 21,
		.model_1 = 9,
		.model_2 = 26,
		.model_3 = 27,
		.gpio_a_output = (1 << 21),
		.gpio_a_input = (1 << 9) | (1 << 26) | (1 << 27),
		.gpio_b_output = 0,
		.gpio_b_input = 0,
	};

	*pGpio = ds111j_gpio;
}

static int gpio_setup(void)
{

	if (syno_is_hw_version(HW_DS111j)) {
		ds111j_gpio_set(&gPLXGPIO);
		printk("Apply DS111j gpio setting\n");
	} else {
		goto END;
	}
			
	if (mutex_lock_interruptible(&ox820_gpio_lock))
                return -ERESTARTSYS;

	if (gPLXGPIO.gpio_a_output || gPLXGPIO.gpio_a_input) {
		unsigned int gpio = gPLXGPIO.gpio_a_output | gPLXGPIO.gpio_a_input;
		writel(readl(SYS_CTRL_SECONDARY_SEL)   & ~(gpio), SYS_CTRL_SECONDARY_SEL);
		writel(readl(SYS_CTRL_TERTIARY_SEL)    & ~(gpio), SYS_CTRL_TERTIARY_SEL);
		writel(readl(SYS_CTRL_QUATERNARY_SEL)  & ~(gpio), SYS_CTRL_QUATERNARY_SEL);
		writel(readl(SYS_CTRL_DEBUG_SEL)       & ~(gpio), SYS_CTRL_DEBUG_SEL);
		writel(readl(SYS_CTRL_ALTERNATIVE_SEL) & ~(gpio), SYS_CTRL_ALTERNATIVE_SEL);
	}

	if (gPLXGPIO.gpio_b_output || gPLXGPIO.gpio_b_input) {
		unsigned int gpio = gPLXGPIO.gpio_b_output | gPLXGPIO.gpio_b_input;
		writel(readl(SEC_CTRL_SECONDARY_SEL)   & ~(gpio), SEC_CTRL_SECONDARY_SEL);
		writel(readl(SEC_CTRL_TERTIARY_SEL)    & ~(gpio), SEC_CTRL_TERTIARY_SEL);
		writel(readl(SEC_CTRL_QUATERNARY_SEL)  & ~(gpio), SEC_CTRL_QUATERNARY_SEL);
		writel(readl(SEC_CTRL_DEBUG_SEL)       & ~(gpio), SEC_CTRL_DEBUG_SEL);
		writel(readl(SEC_CTRL_ALTERNATIVE_SEL) & ~(gpio), SEC_CTRL_ALTERNATIVE_SEL);
	}

	if (gPLXGPIO.gpio_a_input)
		writel((gPLXGPIO.gpio_a_input), GPIO_A_OUTPUT_ENABLE_CLEAR);
	if (gPLXGPIO.gpio_b_input)
		writel((gPLXGPIO.gpio_b_input), GPIO_B_OUTPUT_ENABLE_CLEAR);
	
	if (gPLXGPIO.gpio_a_output) {
		writel(gPLXGPIO.gpio_a_output, GPIO_A_OUTPUT_CLEAR);
		writel(gPLXGPIO.gpio_a_output, GPIO_A_OUTPUT_ENABLE_SET);
	}
	if (gPLXGPIO.gpio_b_output) {
		writel(gPLXGPIO.gpio_b_output, GPIO_B_OUTPUT_CLEAR);
		writel(gPLXGPIO.gpio_b_output, GPIO_B_OUTPUT_ENABLE_SET);
	}

	mutex_unlock(&ox820_gpio_lock);

	if (SYNO_MODEL_DS111j == SynoModelIDGet(&gPLXGPIO)) {
		SYNO820GPIOSet(gPLXGPIO.disk1_act, 1);
	} else {
		goto END;
	}

END:
	return 0;
}

int SYNO820GPIOGet(int pin)
{
	uint32_t val = 0;
	unsigned int new_pin = 0;
	unsigned int base = 0;

	if (pin > 31) {
		new_pin = pin - 31;
		base = GPIO_B_DATA;

		if (! (gPLXGPIO.gpio_b_input & (1 << new_pin))) {
			printk("This pin %d cannot read\n", new_pin);
			goto END;
		}
	} else {
		new_pin = pin;
		base = GPIO_A_DATA;

		if (! (gPLXGPIO.gpio_a_input & (1 << new_pin))) {
			printk("This pin %d cannot read\n", new_pin);
			goto END;
		}
	}

	if (mutex_lock_interruptible(&ox820_gpio_lock))
		return -ERESTARTSYS;

	val = readl(base) ;
	mutex_unlock(&ox820_gpio_lock);

	val = (val & (1 << pin));
END:
	return val ? 1 : 0;
}
EXPORT_SYMBOL(SYNO820GPIOGet);

int SYNO820GPIOSet(int pin, int val)
{
	unsigned int new_pin = 0;
	unsigned int set_base = 0, clear_base = 0;

	if (pin > 31) {
		new_pin = pin - 31;
		set_base = GPIO_B_OUTPUT_SET;
		clear_base = GPIO_B_OUTPUT_CLEAR;

		if (! (gPLXGPIO.gpio_b_output & (1 << new_pin))) {
			printk("This pin %d cannot set\n", new_pin);
			goto END;
		}
	} else {
		new_pin = pin;
		set_base = GPIO_A_OUTPUT_SET;
		clear_base = GPIO_A_OUTPUT_CLEAR;

		if (! (gPLXGPIO.gpio_a_output & (1 << new_pin))) {
			printk("This pin %d cannot set\n", new_pin);
			goto END;
		}
	}
	if (mutex_lock_interruptible(&ox820_gpio_lock))
		return -ERESTARTSYS;

	val ? writel((0x1UL << pin), set_base) : writel((0x1UL << pin), clear_base);

	mutex_unlock(&ox820_gpio_lock);
END:
	return 0;
}
EXPORT_SYMBOL(SYNO820GPIOSet);

extern char gszSynoHWVersion[];
int synology_gpio_init(void)
{
	int ret;
	printk(KERN_INFO "Synology GPIO initialized\n");

	ret = gpio_setup();
	if (ret) {
		printk(KERN_INFO "failedto setup synology GPIO\n");
		return ret;
	}
	return 0;
}

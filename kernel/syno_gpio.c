#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/synobios.h>
#include <linux/syno_gpio.h>

#ifdef MY_DEF_HERE
#include <linux/synolib.h>
#include <linux/of.h>
#endif  

SYNO_GPIO syno_gpio = {
	.fan_ctrl =NULL,
	.fan_fail =NULL,
	.hdd_fail_led =NULL,
	.hdd_present_led =NULL,
	.hdd_act_led =NULL,
	.hdd_detect =NULL,
	.hdd_enable =NULL,
	.model_id =NULL,
	.alarm_led =NULL,
	.power_led =NULL,
	.disk_led_ctrl =NULL,
	.phy_led_ctrl =NULL,
	.copy_button_detect =NULL,
	.redundant_power_detect =NULL,
};
EXPORT_SYMBOL(syno_gpio);

#ifdef MY_DEF_HERE
extern int giSynoSpinupGroupDebug;
#endif  

void syno_gpio_direction_output(int pin, int pValue)
{
	int iErr = 0;
	iErr = gpio_request(pin, NULL);
	if (iErr) {
		printk("%s:%s(%d) gpio_request pin %d fail!\n", __FILE__, __FUNCTION__, __LINE__, pin);
		goto END;
	}
	iErr = gpio_direction_output(pin, pValue);
	if (iErr) {
		printk("%s:%s(%d) set gpio pin %d value %d fail!\n", __FILE__, __FUNCTION__, __LINE__, pin, pValue);
		goto UNLOCK;
	}
UNLOCK:
	gpio_free(pin);
END:
	return;
}
EXPORT_SYMBOL(syno_gpio_direction_output);

void syno_gpio_direction_input(int pin)
{
	int iErr = 0;
	iErr = gpio_request(pin, NULL);
	if (iErr) {
		printk("%s:%s(%d) gpio_request pin %d fail!\n", __FILE__, __FUNCTION__, __LINE__, pin);
		goto END;
	}
	iErr = gpio_direction_input(pin);
	if (iErr) {
		printk("%s:%s(%d) set gpio pin %d input fail!\n", __FILE__, __FUNCTION__, __LINE__, pin);
		goto UNLOCK;
	}
UNLOCK:
	gpio_free(pin);
END:
	return;
}
EXPORT_SYMBOL(syno_gpio_direction_input);

int syno_gpio_to_irq(int pin)
{
	return gpio_to_irq(pin);
}
EXPORT_SYMBOL(syno_gpio_to_irq);

int SYNO_GPIO_READ(int pin)
{
#if defined(MY_DEF_HERE)
	int iVal=0;
	syno_gpio_value_get(pin, &iVal);
	return iVal;
#else
#if defined(MY_DEF_HERE)
	 
	pin = pin < 36 ? (512 - 36 + pin) : (512 - 36 - 30) + pin - 36;
#endif  
	return gpio_get_value(pin);
#endif
}
EXPORT_SYMBOL(SYNO_GPIO_READ);

void SYNO_GPIO_WRITE(int pin, int pValue)
{
#if defined(MY_DEF_HERE)
	syno_gpio_value_set(pin, pValue);
#elif defined(MY_DEF_HERE)
	syno_gpio_direction_output(pin, pValue);
#else
#if defined(MY_DEF_HERE)
	 
	pin = pin < 36 ? (512 - 36 + pin) : (512 - 36 - 30) + pin - 36;
#endif  
	gpio_set_value(pin, pValue);
#endif
}
EXPORT_SYMBOL(SYNO_GPIO_WRITE);

#ifdef MY_DEF_HERE
int SynoHaveRPDetectPin(void)
{
	if (syno_gpio.redundant_power_detect &&
		HAVE_RP_DETECT(1) &&
		HAVE_RP_DETECT(2)) {
		return 1;
	}
	return 0;
}
int SynoAllRedundantPowerDetected(void)
{
	if (syno_gpio.redundant_power_detect && 2 == syno_gpio.redundant_power_detect->nr_gpio &&
		!(SYNO_GPIO_READ(RP_DETECT_PIN(1)) ^ SYNO_GPIO_READ(RP_DETECT_PIN(2)))) {
		return 1;
	}
	return 0;
}
void DBG_SpinupGroupListGpio(void)
{
	int i = 0;
	if (giSynoSpinupGroupDebug && NULL != syno_gpio.hdd_detect) {
		for (i = 0; i < syno_gpio.hdd_detect->nr_gpio; i++) {
			printk("gpio debug: hdd detect pin %d, value= %d\n", HDD_DETECT_PIN(i + 1), SYNO_GPIO_READ(HDD_DETECT_PIN(i + 1)));
		}
		for (i = 0; i < syno_gpio.hdd_enable->nr_gpio; i++) {
			printk("gpio debug: hdd enable pin %d, value= %d\n", HDD_ENABLE_PIN(i + 1), SYNO_GPIO_READ(HDD_ENABLE_PIN(i + 1)));
		}
		if (syno_gpio.redundant_power_detect && 2 == syno_gpio.redundant_power_detect->nr_gpio) {
			printk("gpio debug: redundant power detect pin %d, value= %d\n", RP_DETECT_PIN(1), SYNO_GPIO_READ(RP_DETECT_PIN(1)));
			printk("gpio debug: redundant power detect pin %d, value= %d\n", RP_DETECT_PIN(2), SYNO_GPIO_READ(RP_DETECT_PIN(2)));
		}
	}
}
EXPORT_SYMBOL(DBG_SpinupGroupListGpio);
#endif  

#ifdef MY_DEF_HERE
 
u32 syno_disk_gpio_pin_get(const int diskPort, const char *szPropertyName, const int propertyIndex)
{
	int index= 0;
	u32 synoGpioPin = U32_MAX;
	struct device_node *pSlotNode = NULL;

	if (NULL == szPropertyName || 0 > diskPort || 0 >propertyIndex) {
		goto END;
	}

	for_each_child_of_node(of_root, pSlotNode) {
		 
		if (!pSlotNode->full_name || 1 != sscanf(pSlotNode->full_name, "/"DT_INTERNAL_SLOT"@%d", &index)) {
			continue;
		}
		if (diskPort == index) {
			break;
		}
	}

	if (NULL == pSlotNode) {
		goto END;
	}
	of_property_read_u32_index(pSlotNode, szPropertyName, propertyIndex, &synoGpioPin);
	of_node_put(pSlotNode);
END:
	return synoGpioPin;
}
EXPORT_SYMBOL(syno_disk_gpio_pin_get);

int syno_disk_gpio_pin_have(const int diskPort, const char *szPropertyName)
{
	u32 synoGpioPin = U32_MAX;
	int ret = -1;

	synoGpioPin = syno_disk_gpio_pin_get(diskPort, szPropertyName, SYNO_GPIO_PIN);

	if (U32_MAX != synoGpioPin) {
		ret = 1;
	} else {
		ret = 0;
	}
	return ret;
}
EXPORT_SYMBOL(syno_disk_gpio_pin_have);

u32 syno_led_pin_get(const int diskPort, const char *szLedName, const int propertyIndex)
{
	int index= 0;
	u32 synoGpioPin = U32_MAX;
	struct device_node *pSlotNode = NULL, *pLedNode = NULL;

	if (NULL == szLedName || 0 > diskPort || 0 >propertyIndex) {
		goto END;
	}

	for_each_child_of_node(of_root, pSlotNode) {
		
		if (!pSlotNode->full_name || 1 != sscanf(pSlotNode->full_name, "/"DT_INTERNAL_SLOT"@%d", &index)) {
			continue;
		}
		if (diskPort == index) {
			break;
		}
	}

	if (NULL == pSlotNode) {
		goto END;
	}
	pLedNode = of_get_child_by_name(pSlotNode, szLedName);
	of_node_put(pSlotNode);
	if (NULL == pLedNode) {
		goto END;
	}
	of_property_read_u32_index(pLedNode, DT_SYNO_GPIO, propertyIndex, &synoGpioPin);
	of_node_put(pLedNode);

END:
	return synoGpioPin;
}
EXPORT_SYMBOL(syno_led_pin_get);


int syno_led_pin_have(const int diskPort, const char *szLedName)
{
	u32 synoGpioPin = U32_MAX;
	int ret = -1;

	synoGpioPin = syno_led_pin_get(diskPort, szLedName, SYNO_GPIO_PIN);

	if (U32_MAX != synoGpioPin) {
		ret = 1;
	} else {
		ret = 0;
	}
	return ret;
}
EXPORT_SYMBOL(syno_led_pin_have);

#endif  

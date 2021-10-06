/* Copyright (c) 2009-2015 Synology Inc. All rights reserved. */
#ifndef SYNO_ARM_GPIO_TYPE_H
#define SYNO_ARM_GPIO_TYPE_H

#include <linux/synobios.h>
#include <linux/gpio.h>

#define GPIO_UNDEF 0xFF
#define SYNO_GPIO_PIN_MAX_NUM 8

#define INPUT 0
#define OUTPUT 1

#define INIT_LOW 0
#define INIT_HIGH 1
#define INIT_KEEP_VALUE 0xFF

#define ACTIVE_HIGH 0
#define ACTIVE_LOW 1
#define ACTIVE_IGNORE 0xFF

#ifdef CONFIG_SYNO_PORT_MAPPING_V2
enum SYNO_GPIO_INDEX
{
	SYNO_GPIO_PIN = 0,
	SYNO_POLARITY_PIN,

	/* Must be the last item, DO NOT append after this. */
	SYNO_GPIO_INDEX_MAX
};
#endif /* CONFIG_SYNO_PORT_MAPPING_V2 */

/* The following GPIO macro are 1-based */
#define HAVE_GPIO_PIN(index, type)             ((syno_gpio.type) && (0 < index) && (index <= syno_gpio.type->nr_gpio))
#define GPIO_PORT(index, type)                 syno_gpio.type->gpio_port[index-1]
#define GPIO_POLARITY(type)                    syno_gpio.type->gpio_polarity

#define HAVE_FAN_CTRL(index)                   HAVE_GPIO_PIN(index, fan_ctrl)
#define HAVE_FAN_FAIL(index)                   HAVE_GPIO_PIN(index, fan_fail)
#ifdef CONFIG_SYNO_PORT_MAPPING_V2
#define HAVE_HDD_FAIL_LED(index)               syno_led_pin_have(DT_INTERNAL_SLOT, index, DT_HDD_ORANGE_LED) /* Deprecated, see HAVE_HDD_FAIL_LED_BY_SLOT */
#define HAVE_HDD_PRESENT_LED(index)            syno_led_pin_have(DT_INTERNAL_SLOT, index, DT_HDD_GREEN_LED) /* Deprecated, see HAVE_HDD_PRESENT_LED_BY_SLOT */
#define HAVE_HDD_ACT_LED(index)                syno_led_pin_have(DT_INTERNAL_SLOT, index, DT_HDD_ACT_LED) /* Deprecated, see HAVE_HDD_ACT_LED_BY_SLOT */
#define HAVE_HDD_DETECT(index)                 syno_disk_gpio_pin_have(index, DT_DETECT_PIN_GPIO)
#define HAVE_HDD_ENABLE(index)                 syno_disk_gpio_pin_have(index, DT_POWER_PIN_GPIO)
/* Testify existence of led pin of "name@index" */
#define HAVE_HDD_FAIL_LED_BY_SLOT(name, index)             syno_led_pin_have(name, index, DT_HDD_ORANGE_LED)
#define HAVE_HDD_PRESENT_LED_BY_SLOT(name, index)          syno_led_pin_have(name, index, DT_HDD_GREEN_LED)
#define HAVE_HDD_ACT_LED_BY_SLOT(name, index)              syno_led_pin_have(name, index, DT_HDD_ACT_LED)
#else
#define HAVE_HDD_FAIL_LED(index)               HAVE_GPIO_PIN(index, hdd_fail_led)
#define HAVE_HDD_PRESENT_LED(index)            HAVE_GPIO_PIN(index, hdd_present_led)
#define HAVE_HDD_ACT_LED(index)                HAVE_GPIO_PIN(index, hdd_act_led)
#define HAVE_HDD_DETECT(index)                 HAVE_GPIO_PIN(index, hdd_detect)
#define HAVE_HDD_ENABLE(index)                 HAVE_GPIO_PIN(index, hdd_enable)
#endif /* CONFIG_SYNO_PORT_MAPPING_V2 */

#define HAVE_MODEL_ID(index)                   HAVE_GPIO_PIN(index, model_id)
#define HAVE_ALARM_LED()                       HAVE_GPIO_PIN(1, alarm_led)
#define HAVE_POWER_LED()                       HAVE_GPIO_PIN(1, power_led)
#define HAVE_DISK_LED_CTRL()                   HAVE_GPIO_PIN(1, disk_led_ctrl)
#define HAVE_PHY_LED_CTRL()                    HAVE_GPIO_PIN(1, phy_led_ctrl)
#define HAVE_COPY_BUTTON_DETECT()              HAVE_GPIO_PIN(1, copy_button_detect)
#define HAVE_MUTE_BUTTON_DETECT()              HAVE_GPIO_PIN(1, mute_button_detect)
#define HAVE_BUZZER_MUTE_CTRL()                HAVE_GPIO_PIN(1, buzzer_mute_ctrl)
#define HAVE_RP_DETECT(index)                  HAVE_GPIO_PIN(index, redundant_power_detect)

#define FAN_CTRL_PIN(index)                    GPIO_PORT(index, fan_ctrl)
#define FAN_FAIL_PIN(index)                    GPIO_PORT(index, fan_fail)
#ifdef CONFIG_SYNO_PORT_MAPPING_V2
#define HDD_FAIL_LED_PIN(index)                syno_led_pin_get(DT_INTERNAL_SLOT, index, DT_HDD_ORANGE_LED, SYNO_GPIO_PIN) /* Deprecated, see HDD_FAIL_LED_PIN_BY_SLOT */
#define HDD_PRESENT_LED_PIN(index)             syno_led_pin_get(DT_INTERNAL_SLOT, index, DT_HDD_GREEN_LED, SYNO_GPIO_PIN) /* Deprecated, see HDD_PRESENT_LED_PIN_BY_SLOT */
#define HDD_ACT_LED_PIN(index)                 syno_led_pin_get(DT_INTERNAL_SLOT, index, DT_HDD_ACT_LED, SYNO_GPIO_PIN) /* Deprecated, see HDD_ACT_LED_PIN_BY_SLOT */
#define HDD_DETECT_PIN(index)                  syno_disk_gpio_pin_get(index, DT_DETECT_PIN_GPIO, SYNO_GPIO_PIN)
#define HDD_ENABLE_PIN(index)                  syno_disk_gpio_pin_get(index, DT_POWER_PIN_GPIO, SYNO_GPIO_PIN)
/* Get led pin# of "name@index" */
#define HDD_FAIL_LED_PIN_BY_SLOT(name, index)          syno_led_pin_get(name, index, DT_HDD_ORANGE_LED, SYNO_GPIO_PIN)
#define HDD_PRESENT_LED_PIN_BY_SLOT(name, index)       syno_led_pin_get(name, index, DT_HDD_GREEN_LED, SYNO_GPIO_PIN)
#define HDD_ACT_LED_PIN_BY_SLOT(name, index)           syno_led_pin_get(name, index, DT_HDD_ACT_LED, SYNO_GPIO_PIN)
#else
#define HDD_FAIL_LED_PIN(index)                GPIO_PORT(index, hdd_fail_led)
#define HDD_PRESENT_LED_PIN(index)             GPIO_PORT(index, hdd_present_led)
#define HDD_ACT_LED_PIN(index)                 GPIO_PORT(index, hdd_act_led)
#define HDD_DETECT_PIN(index)                  GPIO_PORT(index, hdd_detect)
#define HDD_ENABLE_PIN(index)                  GPIO_PORT(index, hdd_enable)
#endif /* CONFIG_SYNO_PORT_MAPPING_V2 */

#define MODEL_ID_PIN(index)                    GPIO_PORT(index, model_id)
#define ALARM_LED_PIN()                        GPIO_PORT(1, alarm_led)
#define POWER_LED_PIN()                        GPIO_PORT(1, power_led)
#define DISK_LED_CTRL_PIN()                    GPIO_PORT(1, disk_led_ctrl)
#define PHY_LED_CTRL_PIN()                     GPIO_PORT(1, phy_led_ctrl)
#define COPY_BUTTON_DETECT_PIN()               GPIO_PORT(1, copy_button_detect)
#define MUTE_BUTTON_DETECT_PIN()               GPIO_PORT(1, mute_button_detect)
#define BUZZER_MUTE_CTRL_PIN()                 GPIO_PORT(1, buzzer_mute_ctrl)
#define RP_DETECT_PIN(index)                   GPIO_PORT(index, redundant_power_detect)

#define FAN_CTRL_POLARITY()                    GPIO_POLARITY(fan_ctrl)
#define FAN_FAIL_POLARITY()                    GPIO_POLARITY(fan_fail)
#ifdef CONFIG_SYNO_PORT_MAPPING_V2
#define HDD_FAIL_LED_POLARITY(index)           syno_led_pin_get(DT_INTERNAL_SLOT, index, DT_HDD_ORANGE_LED, SYNO_POLARITY_PIN) /* Deprecated, see HDD_FAIL_LED_POLARITY_BY_SLOT */
#define HDD_PRESENT_LED_POLARITY(index)        syno_led_pin_get(DT_INTERNAL_SLOT, index, DT_HDD_GREEN_LED, SYNO_POLARITY_PIN) /* Deprecated, HDD_PRESENT_LED_POLARITY_BY_SLOT */
#define HDD_ACT_LED_POLARITY(index)            syno_led_pin_get(DT_INTERNAL_SLOT, index, DT_HDD_ACT_LED, SYNO_POLARITY_PIN) /* Deprecated, HDD_ACT_LED_POLARITY_BY_SLOT */
#define HDD_DETECT_POLARITY(index)             syno_disk_gpio_pin_get(index, DT_DETECT_PIN_GPIO, SYNO_POLARITY_PIN)
#define HDD_ENABLE_POLARITY(index)             syno_disk_gpio_pin_get(index, DT_POWER_PIN_GPIO, SYNO_POLARITY_PIN)
/* Get led pin polarity of "name@index" */
#define HDD_FAIL_LED_POLARITY_BY_SLOT(name, index)     syno_led_pin_get(name, index, DT_HDD_ORANGE_LED, SYNO_POLARITY_PIN)
#define HDD_PRESENT_LED_POLARITY_BY_SLOT(name, index)  syno_led_pin_get(name, index, DT_HDD_GREEN_LED, SYNO_POLARITY_PIN)
#define HDD_ACT_LED_POLARITY_BY_SLOT(name, index)      syno_led_pin_get(name, index, DT_HDD_ACT_LED, SYNO_POLARITY_PIN)
#else
#define HDD_FAIL_LED_POLARITY()                GPIO_POLARITY(hdd_fail_led)
#define HDD_PRESENT_LED_POLARITY()             GPIO_POLARITY(hdd_present_led)
#define HDD_ACT_LED_POLARITY()                 GPIO_POLARITY(hdd_act_led)
#define HDD_DETECT_POLARITY()                  GPIO_POLARITY(hdd_detect)
#define HDD_ENABLE_POLARITY()                  GPIO_POLARITY(hdd_enable)
#endif /* CONFIG_SYNO_PORT_MAPPING_V2 */
#define MODEL_ID_POLARITY()                    GPIO_POLARITY(model_id)
#define ALARM_LED_POLARITY()                   GPIO_POLARITY(alarm_led)
#define POWER_LED_POLARITY()                   GPIO_POLARITY(power_led)
#define DISK_LED_CTRL_POLARITY()               GPIO_POLARITY(disk_led_ctrl)
#define PHY_LED_CTRL_POLARITY()                GPIO_POLARITY(phy_led_ctrl)
#define COPY_BUTTON_DETECT_POLARITY()          GPIO_POLARITY(copy_button_detect)
#define MUTE_BUTTON_DETECT_POLARITY()          GPIO_POLARITY(mute_button_detect)
#define BUZZER_MUTE_CTRL_POLARITY()            GPIO_POLARITY(buzzer_mute_ctrl)
#define RP_DETECT_POLARITY()                   GPIO_POLARITY(redundant_power_detect)

typedef struct _tag_SYNO_GPIO_INFO {
	const char *name;
	u8 nr_gpio;
	u8 gpio_port[SYNO_GPIO_PIN_MAX_NUM];
	u8 gpio_direction;
	u8 gpio_init_value;
	u8 gpio_polarity;
} SYNO_GPIO_INFO;

typedef struct __tag_SYNO_GPIO {
	SYNO_GPIO_INFO *fan_ctrl;
	SYNO_GPIO_INFO *fan_fail;
	SYNO_GPIO_INFO *hdd_fail_led;
	SYNO_GPIO_INFO *hdd_present_led;
	SYNO_GPIO_INFO *hdd_act_led;
	SYNO_GPIO_INFO *hdd_detect;
	SYNO_GPIO_INFO *hdd_enable;
	SYNO_GPIO_INFO *model_id;
	SYNO_GPIO_INFO *alarm_led;
	SYNO_GPIO_INFO *power_led;
	SYNO_GPIO_INFO *disk_led_ctrl; // control all disk led on/off
	SYNO_GPIO_INFO *phy_led_ctrl;  // control all phy led on/off
	SYNO_GPIO_INFO *copy_button_detect;
	SYNO_GPIO_INFO *mute_button_detect;
	SYNO_GPIO_INFO *buzzer_mute_ctrl;
	SYNO_GPIO_INFO *redundant_power_detect;
} SYNO_GPIO;

#ifdef CONFIG_SYNO_PORT_MAPPING_V2
u32 syno_disk_gpio_pin_get(const int diskPort, const char *szPropertyName, const int propertyIndex);
int syno_disk_gpio_pin_have(const int diskPort, const char *szPropertyName);
u32 syno_led_pin_get(const char* name, const int diskPort, const char *szLedName, const int propertyIndex);
int syno_led_pin_have(const char* name, const int diskPort, const char *szLedName);
#endif /* CONFIG_SYNO_PORT_MAPPING_V2 */
#endif /* SYNO_ARM_GPIO_TYPE_H */

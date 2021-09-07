// Copyright (c) 2000-2009 Synology Inc. All rights reserved.

#include <linux/synobios.h>
#include "../mapping.h"

int GetModel(void);
int GetGpioPin(GPIO_PIN *pPin);
int SetGpioPin(GPIO_PIN *pPin);

extern int SetUart(const char* cmd);
extern char gszSynoHWVersion[];
extern struct model_ops ds710p_ops;
extern struct model_ops ds1010p_ops;
extern struct model_ops ds410p_ops;
extern struct model_ops rs810p_ops;
extern struct model_ops rs810rpp_ops;

extern int syno_ttys_write(const int index, const char* szBuf);			
extern u32 syno_ich9_lpc_gpio_pin(int pin, int *pValue, int isWrite);
extern u32 syno_superio_gpio_pin(int pin, int *pValue, int isWrite);
extern int syno_sys_temperature(int *Temperature);
extern int syno_cpu_temperature(struct _SynoCpuTemp *pCpuTemp);

#define SYNO_GPP_HDD1_LED_0		16
#define SYNO_GPP_HDD1_LED_1		18
#define SYNO_GPP_HDD2_LED_0		20
#define SYNO_GPP_HDD2_LED_1		32
#define SYNO_GPP_HDD3_LED_0		33
#define SYNO_GPP_HDD3_LED_1		34
#define SYNO_GPP_HDD4_LED_0		49
#define SYNO_GPP_HDD4_LED_1		55
#define SYNO_GPP_RS_BUZZER_OFF	57
#define SYNO_GPP_HDD5_LED_0		133
#define SYNO_GPP_HDD5_LED_1		132

#define SZ_UART_CMD_PREFIX         "-"
#define SZ_UART_ALARM_LED_ON       "LA1"
#define SZ_UART_ALARM_LED_BLINKING "LA2"
#define SZ_UART_ALARM_LED_OFF      "LA3"
#define SZ_UART_FAN_DUTY_CYCLE     "V"
#define SZ_UART_FAN_FREQUENCY      "W"
#define SZ_UART_CPUFAN_DUTY_CYCLE  "X"
#define SZ_UART_CPUFAN_FREQUENCY   "Y"
 
#define UART_TTYS_INDEX 1

/* Mapping FAN_SPEED to duty cycle */
typedef struct _tag_BANDON_FAN_SPEED_MAPPING_ {
	FAN_SPEED fanSpeed;
	int       iDutyCycle; // Range [0, 99]
} BANDON_FAN_SPEED_MAPPING;

struct model_ops {
	int	(*x86_init_module_type)(struct synobios_ops *ops);
	int	(*x86_fan_speed_mapping)(FAN_SPEED speed);
	int	(*x86_set_esata_led_status)(SYNO_DISK_LED status);
	int	(*x86_cpufan_speed_mapping)(FAN_SPEED speed);
	int	(*x86_get_buzzer_cleared)(unsigned char *buzzer_cleared);
	int	(*x86_get_power_status)(POWER_INFO *power_info);
};

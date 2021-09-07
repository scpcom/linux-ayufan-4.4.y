 
#ifndef __ARCH_SYNOLOGY_KIRKWOOD_H_
#define __ARCH_SYNOLOGY_KIRKWOOD_H_

#define SYNO_DS409_ID           (0x13)
#define SYNO_DS409slim_ID       (0x14)
#define SYNO_DS109_ID           (0x15)

#define SATAHC_LED_CONFIG_REG	(SATA_VIRT_BASE | 0x2C)
#define SATAHC_LED_ACT          0x0
#define SATAHC_LED_ACT_PRESENT  0x4

#define DISK_LED_OFF        0
#define DISK_LED_GREEN_SOLID    1
#define DISK_LED_ORANGE_SOLID   2
#define DISK_LED_ORANGE_BLINK   3

typedef struct __tag_SYNO_FAN_GPIO {
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan_fail;
} SYNO_6281_FAN_GPIO;

typedef struct __tag_SYNO_6281_HDD_LED_GPIO {
	u8 hdd1_led_0;
	u8 hdd1_led_1;
	u8 hdd2_led_0;
	u8 hdd2_led_1;
	u8 hdd3_led_0;
	u8 hdd3_led_1;
	u8 hdd4_led_0;
	u8 hdd4_led_1;
	u8 hdd5_led_0;
	u8 hdd5_led_1;
} SYNO_6281_HDD_LED_GPIO;

typedef struct __tag_SYNO_109_GPIO {
	u8 hdd2_fail_led;
	u8 hdd1_fail_led;
	u8 hdd2_power;
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan1_fail;
}SYNO_109_GPIO;

typedef struct __tag_SYNO_409_GPIO {
	u8 alarm_led;
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan_sense;
	u8 inter_lock;
	u8 model_id_0;
	u8 model_id_1;
	u8 hdd1_led_0;
	u8 hdd1_led_1;
	u8 hdd2_led_0;
	u8 hdd2_led_1;
	u8 hdd3_led_0;
	u8 hdd3_led_1;
	u8 hdd4_led_0;
	u8 hdd4_led_1;
	u8 hdd5_led_0;
	u8 hdd5_led_1;
	u8 buzzer_mute_req;
	u8 buzzer_mute_ack;
	u8 rps1_on;
	u8 rps2_on;
} SYNO_409_GPIO;

typedef struct __tag_SYNO_409slim_GPIO {
	u8 hdd1_led_0;
	u8 hdd1_led_1;
	u8 hdd2_led_0;
	u8 hdd2_led_1;
	u8 hdd3_led_0;
	u8 hdd3_led_1;
	u8 hdd4_led_0;
	u8 hdd4_led_1;
	u8 bp_lock_out;
	u8 fan_1;
	u8 fan_2;
	u8 fan_3;
	u8 fan1_fail;
} SYNO_409slim_GPIO;
#endif

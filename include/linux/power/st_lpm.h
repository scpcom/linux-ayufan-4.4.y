#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LPM_H
#define __LPM_H

#include <linux/rtc.h>
#include "st_lpm_def.h"

enum st_lpm_wakeup_devices {
	ST_LPM_WAKEUP_IR	=	BIT(0),
	ST_LPM_WAKEUP_CEC	=	BIT(1),
	ST_LPM_WAKEUP_FRP	=	BIT(2),
	ST_LPM_WAKEUP_WOL	=	BIT(3),
	ST_LPM_WAKEUP_RTC	=	BIT(4),
	ST_LPM_WAKEUP_ASC	=	BIT(5),
	ST_LPM_WAKEUP_NMI	=	BIT(6),
	ST_LPM_WAKEUP_HPD	=	BIT(7),
	ST_LPM_WAKEUP_PIO	=	BIT(8),
	ST_LPM_WAKEUP_EXT	=	BIT(9),
	ST_LPM_WAKEUP_MANUAL_RST =	BIT(10),
	ST_LPM_WAKEUP_SW_WDT_RST =	BIT(11),
	ST_LPM_WAKEUP_CUST	=	BIT(15)
};

struct st_lpm_wkup_dev_name {
	unsigned int index;
	char name[5];
};

struct st_sbc_platform_data {
	unsigned gpio;
	unsigned trigger_level;
};

enum st_lpm_reset_type {
	ST_LPM_SOC_RESET,
	ST_LPM_SBC_RESET = BIT(0),
	ST_LPM_BOOT_RESET = BIT(1),
	ST_LPM_HOST_RESET = BIT(2)
};

enum st_lpm_sbc_state {
	ST_LPM_SBC_BOOT		=	1,
	ST_LPM_SBC_RUNNING	=	4,
	ST_LPM_SBC_STANDBY	=	5
};

struct st_lpm_version {
	char major_comm_protocol;
	char minor_comm_protocol;
	char major_soft;
	char minor_soft;
	char patch_soft;
	char month;
	char day;
	char year;
	char custom_id;
};

struct st_lpm_fp_setting {
	char owner;
	char am_pm;
	char brightness;
};

enum st_lpm_pio_level {
 
	ST_LPM_PIO_LOW,
 
	ST_LPM_PIO_HIGH
};

enum st_lpm_pio_direction {
	ST_LPM_PIO_INPUT,
	ST_LPM_PIO_OUTPUT
};

enum st_lpm_pio_use {
	ST_LPM_PIO_POWER = 1,
	ST_LPM_PIO_ETH_MDINT,
	ST_LPM_PIO_WAKEUP,
	ST_LPM_PIO_EXT_IT,
	ST_LPM_PIO_OTHER,
	ST_LPM_PIO_FP_PIO,
};

struct st_lpm_pio_setting {
	char pio_bank;
	char pio_pin;
	bool pio_direction;
	bool interrupt_enabled;
	bool pio_level;
	enum st_lpm_pio_use  pio_use;
};

enum st_lpm_adv_feature_name {
	ST_LPM_USE_EXT_VCORE = 1,
	ST_LPM_USE_INT_VOLT_DETECT,
	ST_LPM_EXT_CLOCK,
	ST_LPM_RTC_SOURCE,
	ST_LPM_WU_TRIGGERS,
	ST_LPM_DE_BOUNCE,
	ST_LPM_DC_STABLE,
	ST_LPM_SBC_IDLE,
	ST_LPM_RTC_CALIBRATION_TIME,
	ST_LPM_SET_EXIT_CPS,
	ST_LPM_SBC_USER_CUSTOM_MSG = 0x40,
};

struct st_lpm_adv_feature {
	enum st_lpm_adv_feature_name feature_name;
	union {
		unsigned char set_params[12];
		unsigned char get_params[10];
	} params;
};

#define MAX_IR_FIFO_DEPTH	64

#define MIN_RTC_YEAR 2000
#define MAX_RTC_YEAR 2255

struct st_lpm_ir_fifo {
	u16 mark;
	u16 symbol;
};

struct st_lpm_ir_key {
	u8 key_index;
	u8 num_patterns;
	struct st_lpm_ir_fifo fifo[MAX_IR_FIFO_DEPTH];
};

struct st_lpm_ir_keyinfo {
	u8 ir_id;
	u16 time_period;
	u16 time_out;
	u8 tolerance;
	struct st_lpm_ir_key ir_key;
};

struct st_lpm_cec_address {
	u16 phy_addr;
	u16 logical_addr;
};

enum st_lpm_cec_select {
	ST_LPM_CONFIG_CEC_WU_REASON = 1,
	ST_LPM_CONFIG_CEC_WU_CUSTOM_MSG,
	ST_LPM_CONFIG_CEC_VERSION,
	ST_LPM_CONFIG_CEC_DEVICE_VENDOR_ID,
};

enum st_lpm_cec_wu_reason {
	ST_LPM_CEC_WU_STREAM_PATH		= BIT(0),
	ST_LPM_CEC_WU_USER_POWER		= BIT(1),
	ST_LPM_CEC_WU_STREAM_POWER_ON		= BIT(2),
	ST_LPM_CEC_WU_STREAM_POWER_TOGGLE	= BIT(3),
	ST_LPM_CEC_WU_USER_MSG			= BIT(4),
};

enum st_lpm_sbc_dmem_offset_type {
	ST_SBC_DMEM_DTU,
	ST_SBC_DMEM_CPS_TAG,
	ST_SBC_DMEM_PEN_HOLDING_VAR,
};

struct st_lpm_cec_custom_msg {
	u8 size;
	u8 opcode;
	u8 oprand[10];
};

enum st_lpm_inform_host_event {
	ST_LPM_LONG_PIO_PRESS = 1,
#ifdef MY_DEF_HERE
	ST_LPM_ALARM_TIMER,
	ST_LPM_TRACE_DATA,
#else  
	ST_LPM_ALARM_TIMER
#endif  
};

#define ST_LPM_CEC_MAX_OSD_NAME_LENGTH	14

struct st_lpm_cec_osd_msg {
	u8 size;
	u8 name[ST_LPM_CEC_MAX_OSD_NAME_LENGTH];
};

#define ST_LPM_EDID_BLK_SIZE		128
#define ST_LPM_EDID_MAX_BLK_NUM		3
#define ST_LPM_EDID_BLK_END		0xFF
#ifdef MY_DEF_HERE
 
enum st_lpm_trace_mask {
	ST_LPM_TRACE_IR = BIT(0),
	ST_LPM_TRACE_CEC = BIT(1),
	ST_LPM_TRACE_PWM = BIT(2),
	ST_LPM_TRACE_WOL = BIT(3),
	ST_LPM_TRACE_RTC = BIT(4),
	ST_LPM_TRACE_ASC = BIT(5),
	ST_LPM_TRACE_TIMER = BIT(6),
	ST_LPM_TRACE_CUSTOM = BIT(7),
	ST_LPM_TRACE_PIO = BIT(8),
	ST_LPM_TRACE_I2C = BIT(9),
	ST_LPM_TRACE_EDID = BIT(10),
	ST_LPM_TRACE_KEYSCN = BIT(11),
	ST_LPM_TRACE_SPI = BIT(12),
	ST_LPM_TRACE_GENERAL = BIT(13)
};
#endif  

union st_lpm_cec_params {
	u8 cec_wu_reasons;
	struct st_lpm_cec_custom_msg cec_msg;
};

int st_lpm_configure_wdt(u16 time_in_ms, u8 wdt_type);

#ifdef MY_DEF_HERE
#ifdef CONFIG_ST_LPM
int st_lpm_get_fw_state(enum st_lpm_sbc_state *fw_state);
#else
static inline int st_lpm_get_fw_state(enum st_lpm_sbc_state *fw_state)
{
	return -EINVAL;
}
#endif
#else  
int st_lpm_get_fw_state(enum st_lpm_sbc_state *fw_state);
#endif  

int st_lpm_get_wakeup_device(enum st_lpm_wakeup_devices *wakeupdevice);

#ifdef MY_DEF_HERE
#else  
int st_lpm_get_trigger_data(enum st_lpm_wakeup_devices wakeup_device,
		unsigned int size_max, unsigned int size_min,
		char *data);
#endif  

#ifdef MY_DEF_HERE
int st_lpm_get_wakeup_info(enum st_lpm_wakeup_devices wakeupdevice,
#else  
int st_lpm_get_wakeup_info(enum st_lpm_wakeup_devices *wakeupdevice,
#endif  
	s16 *validsize, u16 datasize, char  *data);

int st_lpm_write_edid(u8 *data, u8 block_num);

int st_lpm_read_edid(u8 *data, u8 block_num);

int st_lpm_write_dmem(unsigned char *data, unsigned int size,
		      int offset);

int st_lpm_read_dmem(unsigned char *data, unsigned int size,
		     int offset);

int st_lpm_get_dmem_offset(enum st_lpm_sbc_dmem_offset_type offset_type);

int st_lpm_get_version(struct st_lpm_version *drv_ver,
	struct st_lpm_version *fw_ver);

int st_lpm_reset(enum st_lpm_reset_type reset_type);

int st_lpm_setup_fp(struct st_lpm_fp_setting *fp_setting);

int st_lpm_setup_ir(u8 num_keys, struct st_lpm_ir_keyinfo *keys);

int st_lpm_set_rtc(struct rtc_time *new_rtc);

int st_lpm_get_rtc(struct rtc_time *new_rtc);

int st_lpm_set_wakeup_device(u16  wakeup_devices);

int st_lpm_set_wakeup_time(u32 timeout);

int st_lpm_setup_pio(struct st_lpm_pio_setting *pio_setting);

int st_lpm_setup_keyscan(u16 key_data);

int st_lpm_set_adv_feature(u8 enabled, struct st_lpm_adv_feature *feature);

int st_lpm_get_adv_feature(bool all_features, bool custom_feature,
				struct st_lpm_adv_feature *feature);

enum st_lpm_callback_type {
	ST_LPM_FP_PIO_PRESS,
	ST_LPM_RTC,
#ifdef MY_DEF_HERE
	ST_LPM_GPIO_WAKEUP,
#endif  
	ST_LPM_MAX,  
};

int st_lpm_register_callback(enum st_lpm_callback_type type,
	int (*func)(void *), void *data);

int st_lpm_notify(enum st_lpm_callback_type type);

#ifdef MY_DEF_HERE
int st_lpm_reload_fw_prepare(void);

int st_start_loaded_fw(void);
#endif  

int st_lpm_setup_fp_pio(struct st_lpm_pio_setting *pio_setting,
			u32 long_press_delay, u32 default_reset_delay);

int st_lpm_setup_power_on_delay(u16 de_bounce_delay, u16 dc_stability_delay);

int st_lpm_setup_rtc_calibration_time(u8 cal_time);

int st_lpm_set_cec_addr(struct st_lpm_cec_address *addr);

int st_lpm_cec_set_osd_name(struct st_lpm_cec_osd_msg *params);

int st_lpm_cec_config(enum st_lpm_cec_select use,
			union st_lpm_cec_params *params);

int st_lpm_get_rtc(struct rtc_time *new_rtc);

#define LPM_MAX_MSG_DATA	14

int st_lpm_get_command_data_size(unsigned char command_id);

enum st_lpm_config_reboot_type {
	ST_LPM_REBOOT_WITH_DDR_SELF_REFRESH,
	ST_LPM_REBOOT_WITH_DDR_OFF,
};

int st_lpm_config_reboot(enum st_lpm_config_reboot_type type);

int st_lpm_sbc_ir_enable(bool enable);

int st_lpm_poweroff(void);
#ifdef MY_DEF_HERE
int st_lpm_setup_tracedata(u16 trace_modules);
#endif  

struct lpm_message {
	unsigned char command_id;
	unsigned char transaction_id;
	unsigned char buf[LPM_MAX_MSG_DATA];
} __packed;

struct st_lpm_ops {
	int (*exchange_msg)(struct lpm_message *cmd,
		struct lpm_message *response, void *private_data);
	int (*write_bulk)(u16 size, const char *msg, void *private_data,
			  int *offset_dmem);
	int (*read_bulk)(u16 size, u16 offset, char *msg, void *private_data);
	int (*get_dmem_offset)(enum st_lpm_sbc_dmem_offset_type offset_type,
			       void *private_data);
	int (*config_reboot)(enum st_lpm_config_reboot_type type,
		void *private_data);
	void (*ir_enable)(bool enable, void *private_data);

	int (*write_edid)(u8 *edid_buf, u8 block_num, void *data);

	int (*read_edid)(u8 *edid_buf, u8 block_num, void *data);

#ifdef MY_DEF_HERE
	int (*reload_fw_prepare)(void *data);

	int (*start_loaded_fw)(void *data);

	int (*setup_tracedata)(u16 trace_modules, void *data);
#endif  
};

int st_lpm_set_ops(struct st_lpm_ops *ops, void *private_data);

#endif  

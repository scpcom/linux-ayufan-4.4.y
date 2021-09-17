#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/module.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/power/st_lpm.h>
#include <linux/power/st_lpm_def.h>

static struct st_lpm_ops *st_lpm_ops;
static void *st_lpm_private_data;

static DEFINE_MUTEX(st_lpm_mutex);

struct st_lpm_callback {
	int (*fn)(void *);
	void *data;
};
static struct st_lpm_callback st_lpm_callbacks[ST_LPM_MAX];

int st_lpm_get_command_data_size(unsigned char command_id)
{
	static const unsigned char size[] = {
	[LPM_MSG_NOP]		= LPM_MSG_NOP_SIZE,
	[LPM_MSG_VER]		= LPM_MSG_VER_SIZE,
	[LPM_MSG_READ_RTC]	= LPM_MSG_READ_RTC_SIZE,
	[LPM_MSG_SET_TRIM]	= LPM_MSG_SET_TRIM_SIZE,
	[LPM_MSG_ENTER_PASSIVE] = LPM_MSG_ENTER_PASSIVE_SIZE,
	[LPM_MSG_SET_WDT]	= LPM_MSG_SET_WDT_SIZE,
	[LPM_MSG_SET_RTC]	= LPM_MSG_SET_RTC_SIZE,
	[LPM_MSG_SET_FP]	= LPM_MSG_SET_FP_SIZE,
	[LPM_MSG_SET_TIMER]	= LPM_MSG_SET_TIMER_SIZE,
	[LPM_MSG_GET_STATUS]	= LPM_MSG_GET_STATUS_SIZE,
	[LPM_MSG_GEN_RESET]	= LPM_MSG_GEN_RESET_SIZE,
	[LPM_MSG_SET_WUD]	= LPM_MSG_SET_WUD_SIZE,
	[LPM_MSG_GET_WUD]	= LPM_MSG_GET_WUD_SIZE,
	[LPM_MSG_LGWR_OFFSET]	= LPM_MSG_LGWR_OFFSET_SIZE,
	[LPM_MSG_SET_PIO]	= LPM_MSG_SET_PIO_SIZE,
	[LPM_MSG_SET_ADV_FEA]	= LPM_MSG_SET_ADV_FEA_SIZE,
	[LPM_MSG_SET_KEY_SCAN]	= LPM_MSG_SET_KEY_SCAN_SIZE,
	[LPM_MSG_CEC_ADDR]	= LPM_MSG_CEC_ADDR_SIZE,
	[LPM_MSG_CEC_PARAMS]	= LPM_MSG_CEC_PARAMS_SIZE,
	[LPM_MSG_CEC_SET_OSD_NAME] = LPM_MSG_CEC_SET_OSD_NAME_SIZE,
	[LPM_MSG_SET_IR]	= LPM_MSG_SET_IR_SIZE,
	[LPM_MSG_GET_IRQ]	= LPM_MSG_GET_IRQ_SIZE,
	[LPM_MSG_TRACE_DATA]	= LPM_MSG_TRACE_DATA_SIZE,
	[LPM_MSG_BKBD_READ]	= LPM_MSG_BKBD_READ_SIZE,
	[LPM_MSG_BKBD_WRITE]	= LPM_MSG_BKBD_WRITE_SIZE,
	[LPM_MSG_REPLY]		= LPM_MSG_REPLY_SIZE,
	[LPM_MSG_ERR]		= LPM_MSG_ERR_SIZE,
	[LPM_MSG_EDID_INFO]	= LPM_MSG_EDID_INFO_SIZE,
	};

	if (command_id > LPM_MSG_ERR)
		return -EINVAL;
	return size[command_id];
}

static inline int st_lpm_ops_exchange_msg(
	struct lpm_message *command,
	struct lpm_message *response)
{
	if (!st_lpm_ops || !st_lpm_ops->exchange_msg)
		return -EINVAL;

	return st_lpm_ops->exchange_msg(command,
		response, st_lpm_private_data);
}

static inline int st_lpm_ops_write_bulk(u16 size, const char *msg)
{
	if (!st_lpm_ops || !st_lpm_ops->write_bulk)
		return -EINVAL;

	return st_lpm_ops->write_bulk(size, msg,
		st_lpm_private_data, NULL);
}

static inline int st_lpm_ops_read_bulk(u16 size,
	u16 offset, char *msg)
{
	if (!st_lpm_ops || !st_lpm_ops->read_bulk)
		return -EINVAL;
	return st_lpm_ops->read_bulk(size, offset, msg,
		st_lpm_private_data);
}

static inline int st_lpm_ops_config_reboot(enum st_lpm_config_reboot_type type)
{
	if (!st_lpm_ops || !st_lpm_ops->config_reboot)
		return -EINVAL;
	return st_lpm_ops->config_reboot(type, st_lpm_private_data);
}

int st_lpm_set_ops(struct st_lpm_ops *ops, void *private_data)
{
	if (!ops)
		return -EINVAL;
	mutex_lock(&st_lpm_mutex);
	if (st_lpm_ops) {
		mutex_unlock(&st_lpm_mutex);
		return -EBUSY;
	}
	st_lpm_ops = ops;
	st_lpm_private_data = private_data;
	mutex_unlock(&st_lpm_mutex);
	return 0;
}

int st_lpm_write_edid(unsigned char *data, u8 block_num)
{
	if (!st_lpm_ops || !st_lpm_ops->write_edid || !data)
		return -EINVAL;

	return st_lpm_ops->write_edid(data, block_num, st_lpm_private_data);
}
EXPORT_SYMBOL(st_lpm_write_edid);

int st_lpm_read_edid(unsigned char *data, u8 block_num)
{
	if (!st_lpm_ops || !st_lpm_ops->read_edid || !data)
		return -EINVAL;

	return st_lpm_ops->read_edid(data, block_num, st_lpm_private_data);
}
EXPORT_SYMBOL(st_lpm_read_edid);

#ifdef MY_DEF_HERE
 
int st_lpm_setup_tracedata(u16 trace_modules)
{
	if (!st_lpm_ops || !st_lpm_ops->setup_tracedata)
		return -EINVAL;

	return st_lpm_ops->setup_tracedata(trace_modules, st_lpm_private_data);
}
EXPORT_SYMBOL(st_lpm_setup_tracedata);
#endif  

int st_lpm_write_dmem(unsigned char *data, unsigned int size,
		      int offset)
{
	if (!st_lpm_ops || !st_lpm_ops->write_bulk || !data)
		return -EINVAL;

	if (offset < 0)
		return -EINVAL;

	return st_lpm_ops->write_bulk(size, data, st_lpm_private_data, &offset);
}
EXPORT_SYMBOL(st_lpm_write_dmem);

int st_lpm_read_dmem(unsigned char *data, unsigned int size,
		     int offset)
{
	if (!st_lpm_ops || !st_lpm_ops->read_bulk || !data)
		return -EINVAL;

	if (offset < 0)
		return -EINVAL;

	return st_lpm_ops->read_bulk(size, offset, data, st_lpm_private_data);
}
EXPORT_SYMBOL(st_lpm_read_dmem);

int st_lpm_get_dmem_offset(enum st_lpm_sbc_dmem_offset_type offset_type)
{
	if (!st_lpm_ops || !st_lpm_ops->get_dmem_offset)
		return -EINVAL;

	return st_lpm_ops->get_dmem_offset(offset_type, st_lpm_private_data);
}
EXPORT_SYMBOL(st_lpm_get_dmem_offset);

int st_lpm_get_version(struct st_lpm_version *driver_version,
	struct st_lpm_version *fw_version)
{
	int ret = 0;
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_VER,
	};

	if (unlikely(!driver_version || !fw_version))
		return -EINVAL;

	ret = st_lpm_ops_exchange_msg(&command, &response);

	if (ret)
		return ret;

	fw_version->major_comm_protocol = response.buf[0] >> 4;
	fw_version->minor_comm_protocol = response.buf[0] & 0x0F;
	fw_version->major_soft = response.buf[1] >> 4;
	fw_version->minor_soft = response.buf[1] & 0x0F;
	fw_version->patch_soft = response.buf[2] >> 4;
	fw_version->month = response.buf[2] & 0x0F;
	memcpy(&fw_version->day, &response.buf[3], 3);

	driver_version->major_comm_protocol = LPM_MAJOR_PROTO_VER;
	driver_version->minor_comm_protocol = LPM_MINOR_PROTO_VER;
	driver_version->major_soft = LPM_MAJOR_SOFT_VER;
	driver_version->minor_soft = LPM_MINOR_SOFT_VER;
	driver_version->patch_soft = LPM_PATCH_SOFT_VER;
	driver_version->month = LPM_BUILD_MONTH;
	driver_version->day = LPM_BUILD_DAY;
	driver_version->year = LPM_BUILD_YEAR;

	return ret;
}
EXPORT_SYMBOL(st_lpm_get_version);

int st_lpm_setup_ir(u8 num_keys, struct st_lpm_ir_keyinfo *ir_key_info)
{
	struct st_lpm_ir_keyinfo *this_key;
	u16 ir_size;
	char *buf, *org_buf;
	int count, i, err = 0;

	for (count = 0; count < num_keys; count++) {
		struct st_lpm_ir_key *key_info;
		this_key = ir_key_info;

		ir_key_info++;
		key_info = &this_key->ir_key;
		 
		if (unlikely(this_key->time_period == 0 ||
			     key_info->num_patterns >= 64))
			return -EINVAL;

		ir_size = key_info->num_patterns*2 + 12;
		buf = kmalloc(ir_size, GFP_KERNEL);
		if (!buf)
			return -ENOMEM;

		org_buf = buf;

		*buf++ = LPM_MSG_SET_IR;
		*buf++ = 0;
		*buf++ = this_key->ir_id & 0xF;
		*buf++ = ir_size;
		*buf++ = this_key->time_period & 0xFF;
		*buf++ = (this_key->time_period >> 8) & 0xFF;
		*buf++ = this_key->time_out & 0xFF;
		*buf++ = (this_key->time_out >> 8) & 0xFF;

		if (!this_key->tolerance)
			this_key->tolerance = 10;

		*buf++ = this_key->tolerance;
		*buf++ = key_info->key_index & 0xF;
		*buf++ = key_info->num_patterns;
		 
		buf = org_buf + 12;

		for (i = 0; i < key_info->num_patterns; i++) {
			key_info->fifo[i].mark /= this_key->time_period;
			*buf++ = key_info->fifo[i].mark;
			key_info->fifo[i].symbol /=  this_key->time_period;
			*buf++ = key_info->fifo[i].symbol;
		}

		err = st_lpm_ops_write_bulk(ir_size, org_buf);
		kfree(org_buf);

		if (err < 0)
			break;
	}

	return err;
}
EXPORT_SYMBOL(st_lpm_setup_ir);

#ifdef MY_DEF_HERE
#else  
 
int st_lpm_get_trigger_data(enum st_lpm_wakeup_devices wakeup_device,
		unsigned int size_max, unsigned int size_min,
		char *data)
{
	int err = 0;
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_GET_IRQ,
	};

	command.buf[0] = wakeup_device;
	command.buf[1] = size_min;
	command.buf[2] = size_max;

	err = st_lpm_ops_exchange_msg(&command, &response);

	if (!err)
		memcpy(data, &response.buf[2], 2);

	return err;
}
EXPORT_SYMBOL(st_lpm_get_trigger_data);
#endif  

#ifdef MY_DEF_HERE
int st_lpm_get_wakeup_info(enum st_lpm_wakeup_devices wakeupdevice,
#else  
int st_lpm_get_wakeup_info(enum st_lpm_wakeup_devices *wakeupdevice,
#endif  
			s16 *validsize, u16 datasize, char *data)
{
	int err = 0;
	unsigned short offset;
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_GET_IRQ,
	};

#ifdef MY_DEF_HERE
	command.buf[0] = wakeupdevice;
	command.buf[1] = (wakeupdevice & 0xFF00) >> 8;
#else  
	command.buf[0] = *wakeupdevice;
	command.buf[1] = (*wakeupdevice & 0xFF00) >> 8;
#endif  

	put_unaligned_le16(datasize, &command.buf[2]);
	err = st_lpm_ops_exchange_msg(&command, &response);
	if (unlikely(err < 0))
		goto exit;

	if (response.command_id == LPM_MSG_BKBD_READ) {
		 
		offset = get_unaligned_le32(&response.buf[2]);

		st_lpm_ops_read_bulk(2, offset + 2, (char *)validsize);

		if (*validsize < 0) {
			pr_err("st lpm: Error data size not valid\n");
			err = -EINVAL;
			goto exit;
		}

		if (unlikely(*validsize > datasize)) {
			pr_err("st lpm: more data than allowed\n");
			err = -EINVAL;
			goto exit;
		}
		st_lpm_ops_read_bulk(*validsize, offset + 4, data);

	} else {
		*validsize = get_unaligned_le16(response.buf);
		 
		if (*validsize < 0) {
			pr_err("st lpm: Error data size not valid\n");
			err = -EINVAL;
			goto exit;
		}

		if (unlikely(*validsize > datasize)) {
			pr_err("st lpm: more data than allowed\n");
			err = -EINVAL;
			goto exit;
		}
		 
		memcpy(data, &response.buf[2], *validsize);
	}
exit:
	return err;
}
EXPORT_SYMBOL(st_lpm_get_wakeup_info);

int st_lpm_configure_wdt(u16 time_in_ms, u8 wdt_type)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_SET_WDT,
	};

	if (!time_in_ms)
		return -EINVAL;

	put_unaligned_le16(time_in_ms, command.buf);
	command.buf[2] = wdt_type;

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_configure_wdt);

int st_lpm_get_fw_state(enum st_lpm_sbc_state *fw_state)
{
	int ret = 0;
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_GET_STATUS,
	};

	if (!fw_state)
		return -EINVAL;

	ret = st_lpm_ops_exchange_msg(&command, &response);

	if (likely(ret == 0))
		*fw_state = response.buf[0];

	return ret;
}
EXPORT_SYMBOL(st_lpm_get_fw_state);

int st_lpm_reset(enum st_lpm_reset_type reset_type)
{
	int ret = 0;
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_GEN_RESET,
	};

	command.buf[0] = reset_type;
	ret = st_lpm_ops_exchange_msg(&command, &response);

	if (!ret && reset_type == ST_LPM_SBC_RESET) {
		int i = 0;
		enum st_lpm_sbc_state fw_state;
		 
		for (i = 0; i < 10; ++i) {
			ret = st_lpm_get_fw_state(&fw_state);
			if (!ret)
				break;
			mdelay(100);
		}
	}
	return ret;
}
EXPORT_SYMBOL(st_lpm_reset);

int st_lpm_set_wakeup_device(u16 devices)
{
	struct st_lpm_adv_feature feature;
	feature.feature_name = ST_LPM_WU_TRIGGERS;
	put_unaligned_le16(devices, feature.params.set_params);

	return st_lpm_set_adv_feature(1, &feature);
}
EXPORT_SYMBOL(st_lpm_set_wakeup_device);

int st_lpm_set_wakeup_time(u32 timeout)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_SET_TIMER,
	};

	timeout = cpu_to_le32(timeout);
	 
	memcpy(command.buf, &timeout, 4);

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_set_wakeup_time);

int st_lpm_set_rtc(struct rtc_time *new_rtc)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_SET_RTC,
	};

	if (new_rtc->tm_year >= MIN_RTC_YEAR &&
	    new_rtc->tm_year <= MAX_RTC_YEAR)
		command.buf[5] = new_rtc->tm_year - MIN_RTC_YEAR;
	else
		return  -EINVAL;

	command.buf[4] = new_rtc->tm_mon;
	command.buf[3] = new_rtc->tm_mday;
	command.buf[2] = new_rtc->tm_sec;
	command.buf[1] = new_rtc->tm_min;
	command.buf[0] = new_rtc->tm_hour;

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_set_rtc);

int st_lpm_cec_set_osd_name(struct st_lpm_cec_osd_msg *params)
{
	int len = 0;
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_CEC_SET_OSD_NAME,
	};

	if (!params)
		return -EINVAL;

	command.buf[0] = params->size;

	if (command.buf[0] > ST_LPM_CEC_MAX_OSD_NAME_LENGTH)
		return -EINVAL;

	if (command.buf[0] < ST_LPM_CEC_MAX_OSD_NAME_LENGTH)
		len = command.buf[0];
	else if (command.buf[0] == ST_LPM_CEC_MAX_OSD_NAME_LENGTH)
		len = command.buf[0]-1;

	memcpy(&command.buf[1], &params->name, len);
	if (command.buf[0] == ST_LPM_CEC_MAX_OSD_NAME_LENGTH)
		command.transaction_id =
			params->name[ST_LPM_CEC_MAX_OSD_NAME_LENGTH-1];

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_cec_set_osd_name);

int st_lpm_get_rtc(struct rtc_time *new_rtc)
{
	int err = 0, hours, t1;
	unsigned long time_in_sec;
	struct rtc_time time;
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_READ_RTC,
	};

	if (!new_rtc)
		return -EINVAL;

	err = st_lpm_ops_exchange_msg(&command, &response);

	if (err >= 0 && (response.command_id & LPM_MSG_REPLY)) {
		hours = (response.buf[9] << 24) |
			(response.buf[8] << 16) |
			(response.buf[7] << 8)  |
			(response.buf[6]);

		t1 = (((hours * 60) + response.buf[1]) * 60 +
				response.buf[2]);

		time.tm_year = response.buf[12] + MIN_RTC_YEAR;
		time.tm_mon = response.buf[11];
		time.tm_mday = response.buf[10];
		time.tm_hour = 0;
		time.tm_min = 0;
		time.tm_sec = 0;
		time_in_sec = mktime(time.tm_year, time.tm_mon, time.tm_mday,
				time.tm_hour, time.tm_min, time.tm_sec);
		time_in_sec += t1;
		rtc_time_to_tm(time_in_sec , new_rtc);
	}

	return err;
}
EXPORT_SYMBOL(st_lpm_get_rtc);

int stm_lpm_get_standby_time(u32 *time)
{
	int err;
	struct st_lpm_adv_feature feature = {0};

	err = st_lpm_get_adv_feature(0, 0, &feature);
	if (likely(err == 0))
		memcpy((char *)time, &feature.params.get_params[6], 4);

	return err;
}
EXPORT_SYMBOL(stm_lpm_get_standby_time);

int st_lpm_get_wakeup_device(enum st_lpm_wakeup_devices *wakeup_device)
{
	int err = 0;
	struct st_lpm_adv_feature  feature = {0};

	if (unlikely(wakeup_device == NULL))
		return -EINVAL;

	err = st_lpm_get_adv_feature(0, 0, &feature);

	if (!err) {
		*wakeup_device = feature.params.get_params[5] << 8;
		*wakeup_device |= feature.params.get_params[4];
	}

	return err;
}
EXPORT_SYMBOL(st_lpm_get_wakeup_device);

int st_lpm_setup_fp(struct st_lpm_fp_setting *fp_setting)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_SET_FP,
	};

	if (unlikely(fp_setting == NULL))
		return -EINVAL;

	command.buf[0] = (fp_setting->owner & OWNER_MASK)
		| (fp_setting->am_pm & 1) << 2
		| (fp_setting->brightness & NIBBLE_MASK) << 4;

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_setup_fp);

int st_lpm_setup_pio(struct st_lpm_pio_setting *pio_setting)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_SET_PIO,
	};

	if (unlikely(pio_setting == NULL ||
		(pio_setting->pio_direction &&
		 pio_setting->interrupt_enabled)))
		return -EINVAL;

	command.buf[0] = pio_setting->pio_bank;
	if (pio_setting->pio_use == ST_LPM_PIO_EXT_IT)
		command.buf[0] = 0xFF;

	pio_setting->pio_pin &= NIBBLE_MASK;
	command.buf[1] = pio_setting->pio_level << PIO_LEVEL_SHIFT |
		 pio_setting->interrupt_enabled <<  PIO_IT_SHIFT |
		 pio_setting->pio_direction << PIO_DIRECTION_SHIFT |
		 pio_setting->pio_pin;
	command.buf[2] = pio_setting->pio_use;

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_setup_pio);

int st_lpm_setup_keyscan(u16 key_data)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_SET_KEY_SCAN,
	};

	memcpy(command.buf, &key_data, 2);

	return  st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_setup_keyscan);

int st_lpm_set_adv_feature(u8 enabled, struct st_lpm_adv_feature *feature)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_SET_ADV_FEA,
	};
#ifdef MY_DEF_HERE
	int ret;
#endif  

	if (unlikely(feature == NULL))
		return -EINVAL;

	memcpy(&command.buf[2], feature->params.set_params, 12);
	command.buf[0] = feature->feature_name;
	command.buf[1] = enabled;

#ifdef MY_DEF_HERE
	 
	if (feature->feature_name == ST_LPM_SBC_IDLE)
		ret = st_lpm_ops_exchange_msg(&command, NULL);
	else
		ret = st_lpm_ops_exchange_msg(&command, &response);

	return ret;
#else  
	return st_lpm_ops_exchange_msg(&command, &response);
#endif  
}
EXPORT_SYMBOL(st_lpm_set_adv_feature);

int st_lpm_get_adv_feature(bool all_features, bool custom_feature,
			struct st_lpm_adv_feature *features)
{
	int err = 0;
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_GET_ADV_FEA,
	};

	if (unlikely(!features))
		return -EINVAL;

	if (all_features == true)
		command.buf[0] = BIT(0);

	if (custom_feature)
		command.buf[0] |= BIT(7);

	err = st_lpm_ops_exchange_msg(&command, &response);
	if (likely(!err))
		memcpy(features->params.get_params, response.buf, 10);

	return err;
}
EXPORT_SYMBOL(st_lpm_get_adv_feature);

int st_lpm_setup_fp_pio(struct st_lpm_pio_setting *pio_setting,
			u32 long_press_delay, u32 default_reset_delay)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_SET_PIO,
	};

	if (!pio_setting ||
	    (pio_setting->pio_direction && pio_setting->interrupt_enabled) ||
	    pio_setting->pio_use != ST_LPM_PIO_FP_PIO)
		return -EINVAL;

	command.buf[0] = pio_setting->pio_bank;

	pio_setting->pio_pin &= NIBBLE_MASK;
	command.buf[1] = pio_setting->pio_level << PIO_LEVEL_SHIFT |
		pio_setting->interrupt_enabled <<  PIO_IT_SHIFT |
		pio_setting->pio_direction << PIO_DIRECTION_SHIFT |
		pio_setting->pio_pin;

	command.buf[2] = pio_setting->pio_use;

	memcpy(&command.buf[6], &long_press_delay, 4);
	memcpy(&command.buf[10], &default_reset_delay, 4);

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_setup_fp_pio);

int st_lpm_setup_power_on_delay(u16 de_bounce_delay,
				u16 dc_stable_delay)
{
	struct st_lpm_adv_feature feature;
	feature.feature_name = ST_LPM_DE_BOUNCE;

	put_unaligned_le16(de_bounce_delay, feature.params.set_params);
	st_lpm_set_adv_feature(1, &feature);
	feature.feature_name = ST_LPM_DC_STABLE;
	put_unaligned_le16(dc_stable_delay, feature.params.set_params);

	return st_lpm_set_adv_feature(1, &feature);
}
EXPORT_SYMBOL(st_lpm_setup_power_on_delay);

int st_lpm_setup_rtc_calibration_time(u8 cal_time)
{
	struct st_lpm_adv_feature feature;
	feature.feature_name = ST_LPM_RTC_CALIBRATION_TIME;

	feature.params.set_params[0] = cal_time;

	return st_lpm_set_adv_feature(1, &feature);
}
EXPORT_SYMBOL(st_lpm_setup_rtc_calibration_time);

int st_lpm_set_cec_addr(struct st_lpm_cec_address *addr)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_CEC_ADDR,
	};

	if (unlikely(!addr))
		return -EINVAL;

	put_unaligned_le16(addr->phy_addr, command.buf);
	put_unaligned_le16(addr->logical_addr, &command.buf[2]);

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_set_cec_addr);

int st_lpm_cec_config(enum st_lpm_cec_select use,
			union st_lpm_cec_params *params)
{
	struct lpm_message response = {0};
	struct lpm_message command = {
		.command_id = LPM_MSG_CEC_PARAMS,
	};

	if (unlikely(NULL == params))
		return -EINVAL;

	command.buf[0] = use;
	if (use == ST_LPM_CONFIG_CEC_WU_REASON)
		command.buf[1] = params->cec_wu_reasons;
	else {
		command.buf[2] = params->cec_msg.size;
		memcpy(&command.buf[3],
			&params->cec_msg.opcode, command.buf[2]);
	}

	return st_lpm_ops_exchange_msg(&command, &response);
}
EXPORT_SYMBOL(st_lpm_cec_config);

int st_lpm_poweroff(void)
{
	struct lpm_message command = {
		.command_id = LPM_MSG_ENTER_PASSIVE,
	};

	return st_lpm_ops_exchange_msg(&command, NULL);
};
EXPORT_SYMBOL(st_lpm_poweroff);

int st_lpm_config_reboot(enum st_lpm_config_reboot_type type)
{
	switch (type) {
	case ST_LPM_REBOOT_WITH_DDR_SELF_REFRESH:
	case ST_LPM_REBOOT_WITH_DDR_OFF:
		return st_lpm_ops_config_reboot(type);
	break;
	default:
		pr_err("%s: configuration NOT supported!\n", __func__);
	}
	return -EINVAL;
}
EXPORT_SYMBOL(st_lpm_config_reboot);

int st_lpm_sbc_ir_enable(bool enable)
{
	if (!st_lpm_ops || !st_lpm_ops->ir_enable)
		return -EINVAL;

	st_lpm_ops->ir_enable(enable, st_lpm_private_data);

	return 0;
}
EXPORT_SYMBOL(st_lpm_sbc_ir_enable);

int st_lpm_register_callback(enum st_lpm_callback_type type,
	int (*fnc)(void *), void *data)
{
	int ret = 0;
	if (type >= ST_LPM_MAX)
		return -EINVAL;

	mutex_lock(&st_lpm_mutex);

	if (!st_lpm_callbacks[type].fn) {
		st_lpm_callbacks[type].fn = fnc;
		st_lpm_callbacks[type].data = data;
	} else {
		ret = -EBUSY;
	}
	mutex_unlock(&st_lpm_mutex);

	return ret;
}
EXPORT_SYMBOL(st_lpm_register_callback);

int st_lpm_notify(enum st_lpm_callback_type type)
{
	struct st_lpm_callback *cb;

	if (type >= ST_LPM_MAX)
		return -EINVAL;

	cb = &st_lpm_callbacks[type];
	if (cb->fn)
		return cb->fn(cb->data);

	return 0;
}
EXPORT_SYMBOL(st_lpm_notify);

#ifdef MY_DEF_HERE
int st_lpm_reload_fw_prepare(void)
{
	if (!st_lpm_ops || !st_lpm_ops->reload_fw_prepare)
		return -EINVAL;

	st_lpm_ops->reload_fw_prepare(st_lpm_private_data);

	return 0;
}
EXPORT_SYMBOL(st_lpm_reload_fw_prepare);

int st_start_loaded_fw(void)
{
	if (!st_lpm_ops || !st_lpm_ops->start_loaded_fw)
		return -EINVAL;

	st_lpm_ops->start_loaded_fw(st_lpm_private_data);

	return 0;
}
EXPORT_SYMBOL(st_start_loaded_fw);
#endif  

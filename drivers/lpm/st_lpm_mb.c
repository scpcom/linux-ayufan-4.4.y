#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/firmware.h>
#include <linux/elf.h>
#include <asm/unaligned.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/power/st_lpm.h>
#include <linux/power/st_lpm_def.h>
#include <linux/of_platform.h>

static const struct st_lpm_wkup_dev_name wkup_dev_name_tab[] = {
	{ST_LPM_WAKEUP_IR, "ir"},
	{ST_LPM_WAKEUP_CEC, "cec"},
	{ST_LPM_WAKEUP_FRP, "frp"},
	{ST_LPM_WAKEUP_WOL, "wol"},
	{ST_LPM_WAKEUP_RTC, "rtc"},
	{ST_LPM_WAKEUP_ASC, "asc"},
	{ST_LPM_WAKEUP_NMI, "nmi"},
	{ST_LPM_WAKEUP_HPD, "hpd"},
	{ST_LPM_WAKEUP_PIO, "pio"},
	{ST_LPM_WAKEUP_EXT, "ext"},
	{ST_LPM_WAKEUP_CUST, "cust"}
};

enum lpm_services {
#ifdef MY_DEF_HERE
	LPM_FW_RELOAD,
#endif  
	LPM_CUST_FEAT,
	LPM_EDID,
#ifdef MY_DEF_HERE
	LPM_SBC_TRACES,
	LPM_SBC_TRACES_IN_SUSPEND
#endif  
};

static struct st_lpm_version lpm_fw_ver_vs_services[] = {
#ifdef MY_DEF_HERE
	  
	[LPM_FW_RELOAD] = {.major_soft = 1, .minor_soft = 2, .patch_soft = 0},
#endif  
	  
	[LPM_CUST_FEAT] = {.major_soft = 1, .minor_soft = 4, .patch_soft = 0},
	 
#ifdef MY_DEF_HERE
	[LPM_EDID] = {.major_soft = 1, .minor_soft = 4,	.patch_soft = 2},
	 
	[LPM_SBC_TRACES] = {.major_soft = 1, .minor_soft = 8, .patch_soft = 0},
	 
	[LPM_SBC_TRACES_IN_SUSPEND] = {
		.major_soft = 1, .minor_soft = 8, .patch_soft = 1},
#else  
	[LPM_EDID] = {.major_soft = 1, .minor_soft = 4,	.patch_soft = 2,},
#endif  
};

static bool lpm_check_fw_version(struct st_lpm_version *fw_ver,
				 enum lpm_services service)
{
	struct st_lpm_version *service_version;

	service_version = &lpm_fw_ver_vs_services[service];

	if (fw_ver->major_soft > service_version->major_soft)
		return true;
	else if (fw_ver->major_soft < service_version->major_soft)
		return false;
	else
		if (fw_ver->minor_soft > service_version->minor_soft)
			return true;
		else if (fw_ver->minor_soft < service_version->minor_soft)
			return false;
		else
			if (fw_ver->patch_soft > service_version->patch_soft)
				return true;
			else if (fw_ver->patch_soft <
					service_version->patch_soft)
				return false;
			else
				return true;
}

struct st_lpm_driver_data {
	void * __iomem iomem_base[3];
	struct lpm_message fw_reply_msg;
	struct lpm_message fw_request_msg;
#ifdef MY_DEF_HERE
	char fw_name[30];
#else  
	char fw_name[20];
#endif  
	int reply_from_sbc;
	wait_queue_head_t wait_queue;
	struct mutex mutex;
	unsigned char glbtransaction_id;
	enum st_lpm_sbc_state sbc_state;
	unsigned int power_on_gpio;
	unsigned int trigger_level;
	unsigned int pmem_offset;
	unsigned int const *data_ptr;
	struct notifier_block wakeup_cb;
	u16 wakeup_bitmap;
	struct mutex fwlock;
	bool fwstatus;
	struct device *dev;
	struct st_lpm_version fw_ver;
#ifdef MY_DEF_HERE
	const struct firmware *fw;
	u16 trace_data_value;
#endif  
};

#define lpm_write32(drv, index, offset, value)	iowrite32(value,	\
			(drv)->iomem_base[index] + offset)

#define lpm_read32(drv, idx, offset)	ioread32(			\
			(drv)->iomem_base[idx] + offset)

#define SBC_PRG_MEMORY_ELF_MARKER	0x00400000

#define SBC_MBX_OFFSET		0x0

#define SBC_CONFIG_OFFSET	0x0

#define MBX_WRITE_STATUS(i)	(SBC_MBX_OFFSET + (i) * 0x4)

#define MBX_READ_STATUS(i)	(SBC_MBX_OFFSET + 0x100 + (i) * 0x4)

#define MBX_READ_CLR_STATUS1	(SBC_MBX_OFFSET + 0x144)

#define MBX_INT_ENABLE		(SBC_MBX_OFFSET + 0x164)
 
#define MBX_INT_SET_ENABLE	(SBC_MBX_OFFSET + 0x184)

#define MBX_INT_CLR_ENABLE	(SBC_MBX_OFFSET + 0x1A4)

#define SBC_REPLY_NO		0x0
#define SBC_REPLY_YES		0x1
#define SBC_REPLY_NO_IRQ	0x2

#define CPS_EXIT_EXIT_CPS	0x9b

#define DEFAULT_EXIT_CPS	0xb8

#ifdef MY_DEF_HERE
static int lpm_config_reboot(enum st_lpm_config_reboot_type type, void *data)
{
	struct st_lpm_adv_feature feature;

	feature.feature_name = ST_LPM_SET_EXIT_CPS;

	switch (type) {
	case ST_LPM_REBOOT_WITH_DDR_SELF_REFRESH:
	 
		feature.params.set_params[0] = CPS_EXIT_EXIT_CPS;
		break;

	case ST_LPM_REBOOT_WITH_DDR_OFF:
	 
		feature.params.set_params[0] = DEFAULT_EXIT_CPS;
		break;
	}

	st_lpm_set_adv_feature(1, &feature);

	return 0;
}

static void lpm_configure_power_on_gpio(struct st_lpm_driver_data *lpm_drv)
{
	int err = 0;

	if (lpm_drv->power_on_gpio >= 0) {
		struct st_lpm_pio_setting pio_setting = {
			.pio_bank = lpm_drv->power_on_gpio / 8,
			.pio_pin = lpm_drv->power_on_gpio % 8,
			.pio_level = lpm_drv->trigger_level ?
				ST_LPM_PIO_LOW : ST_LPM_PIO_HIGH,
			.pio_use = ST_LPM_PIO_POWER,
			.pio_direction = 1,
			.interrupt_enabled = 0,
		};

		err = st_lpm_setup_pio(&pio_setting);
		if (err < 0)
			dev_warn(lpm_drv->dev,
				"Setup power_on gpio failed: err = %d\n", err);
	}
}
#endif  

static int lpm_exchange_msg(struct lpm_message *command,
	struct lpm_message *response, void *private_data);

#ifdef MY_DEF_HERE
 
static int lpm_setup_tracedata(u16 trace_modules, void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;
	struct lpm_message command = {0};
	struct lpm_message response = {0};
	struct st_lpm_version *service_version;

	if (!lpm_check_fw_version(&lpm_drv->fw_ver, LPM_SBC_TRACES)) {
		service_version = &lpm_fw_ver_vs_services[LPM_SBC_TRACES];

		dev_warn(lpm_drv->dev,
			 "This LPM Fw does not support SBC traces service\n");
		dev_warn(lpm_drv->dev,
			 "SBC traces service available from LPM Fw v%d.%d.%d\n",
			 service_version->major_soft,
			 service_version->minor_soft,
			 service_version->patch_soft);

		return -EINVAL;
	}

	lpm_drv->trace_data_value = trace_modules;

	command.command_id = LPM_MSG_TRACE_DATA;
	command.buf[0] = trace_modules & 0xFF;
	command.buf[1] = (trace_modules >> 8) & 0xFF;

	return lpm_exchange_msg(&command, &response, lpm_drv);
}

static void lpm_sec_init(struct st_lpm_driver_data *lpm_drv)
{
	struct st_lpm_version driver_ver;
	int err = 0;

	dev_dbg(lpm_drv->dev, "SBC Firmware is running\n");

	err = st_lpm_get_version(&driver_ver, &lpm_drv->fw_ver);

	if (err >= 0) {
		dev_info(lpm_drv->dev, "LPM: firmware ver %d.%d.%d",
			 lpm_drv->fw_ver.major_soft,
			 lpm_drv->fw_ver.minor_soft,
			 lpm_drv->fw_ver.patch_soft);
		dev_info(lpm_drv->dev,
			 "(%02d-%02d-20%02d)\n", lpm_drv->fw_ver.day,
			 lpm_drv->fw_ver.month, lpm_drv->fw_ver.year);
	}

	lpm_config_reboot(ST_LPM_REBOOT_WITH_DDR_OFF, lpm_drv);

	lpm_configure_power_on_gpio(lpm_drv);

#ifdef CONFIG_GENERAL_SBC_TRACES
	lpm_setup_tracedata(ST_LPM_TRACE_GENERAL, lpm_drv);
#endif
}
#endif  

static irqreturn_t lpm_isr(int this_irq, void *params)
{
	struct st_lpm_driver_data *lpm_drv =
		(struct st_lpm_driver_data *)params;
	u32 msg_read[4], i;
	struct lpm_message *msg, *msg_p;

	lpm_drv->reply_from_sbc = 0;

	for (i = 0; i < 4; i++) {
		msg_read[i] = lpm_read32(lpm_drv, 1, MBX_READ_STATUS(1 + i));
		msg_read[i] = cpu_to_le32(msg_read[i]);
		}
	 
	msg_p = (struct lpm_message *)msg_read;

	if (msg_p->command_id == LPM_MSG_INFORM_HOST) {
		if (msg_p->buf[0] == ST_LPM_ALARM_TIMER) {
			st_lpm_notify(ST_LPM_RTC);

			lpm_write32(lpm_drv, 1, MBX_READ_CLR_STATUS1,
					0xFFFFFFFF);

			return IRQ_HANDLED;
		}
	}

	if ((msg_p->command_id & LPM_MSG_REPLY) ||
			(msg_p->command_id == LPM_MSG_BKBD_READ)) {
		msg = &lpm_drv->fw_reply_msg;
		lpm_drv->reply_from_sbc = 1;
	} else {
		msg = &lpm_drv->fw_request_msg;
	}

	memcpy(msg, &msg_read, 16);

	lpm_write32(lpm_drv, 1, MBX_READ_CLR_STATUS1, 0xFFFFFFFF);

	if (lpm_drv->reply_from_sbc != 1)
		return IRQ_WAKE_THREAD;

	wake_up_interruptible(&lpm_drv->wait_queue);

	return IRQ_HANDLED;
}

static irqreturn_t lpm_threaded_isr(int this_irq, void *params)
{
	struct st_lpm_driver_data *lpm_drv =
		(struct st_lpm_driver_data *)params;
	int err = 0;
	struct lpm_message command;
	struct lpm_message *msg_p;
	char *buf = command.buf;
#ifdef MY_DEF_HERE
	u8 length;
	u32 offset;
	char data[128];
#endif  

	msg_p = &lpm_drv->fw_request_msg;
	dev_dbg(lpm_drv->dev, "Send reply to firmware\nrecd command id %x\n",
		msg_p->command_id);

	switch (msg_p->command_id) {
	case LPM_MSG_VER:
		 
		buf[0] = (LPM_MAJOR_PROTO_VER << 4) | LPM_MINOR_PROTO_VER;
		buf[1] = (LPM_MAJOR_SOFT_VER << 4) | LPM_MINOR_SOFT_VER;
		buf[2] = (LPM_PATCH_SOFT_VER << 4) | LPM_BUILD_MONTH;
		buf[3] = LPM_BUILD_DAY;
		buf[4] = LPM_BUILD_YEAR;
		command.command_id = LPM_MSG_VER | LPM_MSG_REPLY;
		break;
	case LPM_MSG_INFORM_HOST:
#ifdef MY_DEF_HERE
		if (msg_p->buf[0] == ST_LPM_LONG_PIO_PRESS) {
			err = st_lpm_notify(ST_LPM_FP_PIO_PRESS);
			if (err >= 0) {
				memcpy(&command.buf[2], &err, 4);
				command.command_id = LPM_MSG_INFORM_HOST |
					LPM_MSG_REPLY;
				break;
			} else {
				dev_dbg(lpm_drv->dev,
					"PIO Reset callback error reported (%d)\n",
					err);
			}
		}

		if (msg_p->buf[0] == ST_LPM_ALARM_TIMER)
			return IRQ_HANDLED;

		if (msg_p->buf[0] == ST_LPM_TRACE_DATA) {
			length = msg_p->buf[1];
			offset = get_unaligned_le32(&(msg_p->buf[2]));
			memset(data, 0, 128);
			memcpy_fromio(data, lpm_drv->iomem_base[0] +
					DATA_OFFSET + offset, length);
			dev_info(lpm_drv->dev, "SBC f/w traces:%s\n", data);
#else  
		err = st_lpm_notify(ST_LPM_FP_PIO_PRESS);
		if (err >= 0) {
			memcpy(&command.buf[2], &err, 4);
			command.command_id = LPM_MSG_INFORM_HOST |
				LPM_MSG_REPLY;
		} else {
			dev_dbg(lpm_drv->dev,
				"PIO Reset callback error reported (%d)\n",
				err);
#endif  
			return IRQ_HANDLED;
		}
	default:
		 
		buf[0] = msg_p->command_id;
		buf[1] = -EINVAL;
		command.command_id = LPM_MSG_ERR;
		break;
	}

	command.transaction_id = msg_p->transaction_id;

	lpm_exchange_msg(&command, NULL, lpm_drv);
	msg_p->command_id = 0;
	return IRQ_HANDLED;
}

static int lpm_send_msg(struct st_lpm_driver_data *lpm_drv,
	struct lpm_message *msg, unsigned char msg_size)
{
	int err = 0, count;
	u32 *tmp_i = (u32 *)msg;

	if (!(lpm_drv->sbc_state == ST_LPM_SBC_RUNNING ||
	      lpm_drv->sbc_state == ST_LPM_SBC_BOOT))
		return -EREMOTEIO;

	for (count = (msg_size + 1) / 4; count >= 0; count--) {
		*(tmp_i + count) = cpu_to_le32(*(tmp_i + count));
		lpm_write32(lpm_drv, 1, (MBX_WRITE_STATUS(1 + count)),
				    *(tmp_i + count));
	}

	return err;
}

static int lpm_get_response(struct st_lpm_driver_data *lpm_drv,
	struct lpm_message *response)
{
	int count, i;
	u32 msg_read1[4];

	for (count = 0; count < 100; count++) {
		msg_read1[0] = lpm_read32(lpm_drv, 1, MBX_READ_STATUS(1));
		msg_read1[0] = cpu_to_le32(msg_read1[0]);
		 
		if (msg_read1[0] & 0xFF)
			break;
		mdelay(10);
	}

	if (count == 100) {
		dev_err(lpm_drv->dev, "count %d value %x\n",
				count, msg_read1[0]);
		return 1;
	}

	for (i = 1; i < 4; i++) {
		msg_read1[i] = lpm_read32(lpm_drv, 1, MBX_READ_STATUS(1 + i));
		msg_read1[i] = cpu_to_le32(msg_read1[i]);
	}
	 
	memcpy(&lpm_drv->fw_reply_msg, (void *)msg_read1, 16);
	lpm_write32(lpm_drv, 1, MBX_READ_CLR_STATUS1, 0xFFFFFFFF);

	return 0;
}

static int lpm_exchange_msg(struct lpm_message *command,
	struct lpm_message *response, void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;
	struct lpm_message lpm_msg = {0};
	int err = 0, data_size;
	int reply_type =  SBC_REPLY_YES;

	dev_dbg(lpm_drv->dev, "%s\n", __func__);

	if (irqs_disabled()) {
		err = mutex_trylock(&lpm_drv->mutex);
		reply_type = SBC_REPLY_NO_IRQ;
		if (!err)
			return -EAGAIN;
	} else {
		mutex_lock(&lpm_drv->mutex);
		if (response == NULL)
			reply_type = SBC_REPLY_NO;
	}

	lpm_msg.command_id = command->command_id;

	if (lpm_msg.command_id & LPM_MSG_REPLY || command->transaction_id)
		lpm_msg.transaction_id = command->transaction_id;
	else
		lpm_msg.transaction_id = lpm_drv->glbtransaction_id++;

	data_size = st_lpm_get_command_data_size(command->command_id);
	if (data_size)
		memcpy(lpm_msg.buf, command->buf, data_size);

	dev_dbg(lpm_drv->dev, "Sending msg {%x, %x}\n", lpm_msg.command_id,
		lpm_msg.transaction_id);

	lpm_drv->reply_from_sbc = 0;

	err = lpm_send_msg(lpm_drv, &lpm_msg, data_size);

	if (unlikely(err < 0)) {
		dev_err(lpm_drv->dev, "firmware not loaded\n");
		goto exit_fun;
	}

	switch (reply_type) {
	case SBC_REPLY_NO_IRQ:
		err = lpm_get_response(lpm_drv, response);
		if (err)  
			err = -ETIMEDOUT;
		break;
	case  SBC_REPLY_YES:
		 
		do {
			err = wait_event_interruptible_timeout(
				lpm_drv->wait_queue,
				lpm_drv->reply_from_sbc == 1,
				msecs_to_jiffies(100));
		} while (err < 0);

		if (err == 0)  
			err = -ETIMEDOUT;
		 
		if (err > 0)
			err = 0;
		break;
	case SBC_REPLY_NO:
		goto exit_fun;
		break;
	}

	dev_dbg(lpm_drv->dev, "recd reply  %x {%x, %x }\n", err,
		lpm_drv->fw_reply_msg.command_id,
	lpm_drv->fw_reply_msg.transaction_id);

	if (unlikely(err ==  -ETIMEDOUT)) {
		dev_err(lpm_drv->dev, "f/w is not responding\n");
		err = -EAGAIN;
		goto exit_fun;
	}

	BUG_ON(!(lpm_drv->fw_reply_msg.command_id & LPM_MSG_REPLY));

	memcpy(response, &lpm_drv->fw_reply_msg,
	       sizeof(struct lpm_message));

	if (lpm_msg.transaction_id == lpm_drv->fw_reply_msg.transaction_id) {
		if (response->command_id == LPM_MSG_ERR) {
			dev_err(lpm_drv->dev,
				"Firmware error code %d\n", response->buf[1]);
			 
			err = -EREMOTEIO;
		}
		 
		if (response->command_id == LPM_MSG_BKBD_READ)
			dev_dbg(lpm_drv->dev, "Got in reply a big message\n");
	} else {
		 
		dev_dbg(lpm_drv->dev,
			"Received ID %x\n", response->transaction_id);
	}

exit_fun:
	mutex_unlock(&lpm_drv->mutex);

	return err;
}

static int lpm_write_bulk(u16 size, const char *sbc_msg, void *data,
			  int *offset_dmem)
{
	struct st_lpm_driver_data *lpm_drv = data;
	int err = 0;
	unsigned int offset;
	struct lpm_message command = {
		.command_id = LPM_MSG_LGWR_OFFSET,
	};
	struct lpm_message response = {0};

	if (!offset_dmem) {
		put_unaligned_le16(size, command.buf);

		err = lpm_exchange_msg(&command, &response, lpm_drv);

		if (err)
			return err;

		offset = get_unaligned_le32(&response.buf[2]);
	} else {
		offset = *offset_dmem;
	}

	if (size == sizeof(unsigned long))
		writel(*((unsigned long *)sbc_msg),
		       lpm_drv->iomem_base[0] + DATA_OFFSET + offset);
	else
		memcpy_toio(lpm_drv->iomem_base[0] + DATA_OFFSET + offset,
			    sbc_msg, size);
	if (!offset_dmem) {
		 
		put_unaligned_le16(size, command.buf);
		put_unaligned_le32(offset, &command.buf[2]);
		command.command_id = LPM_MSG_BKBD_WRITE;
		return lpm_exchange_msg(&command, &response, lpm_drv);
	}

	return 0;
}

static int lpm_read_bulk(u16 size, u16 offset, char *msg, void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;

	memcpy_fromio(msg,
		offset + DATA_OFFSET + lpm_drv->iomem_base[0], size);

	return 0;
}

static int lpm_write_edid_info(u8 *edid_buf, u8 block_num, void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;
	int err = 0;
	u32 offset;
	struct lpm_message command = {0};
	struct lpm_message response = {0};

	if (!lpm_check_fw_version(&lpm_drv->fw_ver, LPM_EDID)) {
		dev_warn(lpm_drv->dev,
			 "This LPM Fw does not support EDID service\n");
		return -EINVAL;
	}

	if (block_num > ST_LPM_EDID_MAX_BLK_NUM) {
		dev_err(lpm_drv->dev, "Block number is out of range\n");
		return -EINVAL;
	}

	command.command_id = LPM_MSG_LGWR_OFFSET;
	put_unaligned_le16(ST_LPM_EDID_BLK_SIZE, command.buf);

	err = lpm_exchange_msg(&command, &response, lpm_drv);
	if (err)
		return err;

	offset = get_unaligned_le32(&response.buf[2]);

	memcpy_toio(lpm_drv->iomem_base[0] + DATA_OFFSET + offset,
		    (const void *)edid_buf, ST_LPM_EDID_BLK_SIZE);

	command.command_id = LPM_MSG_EDID_INFO,
	command.buf[0] = ST_LPM_EDID_BLK_SIZE;
	command.buf[1] = block_num;
	put_unaligned_le32(offset, &command.buf[2]);
	err = lpm_exchange_msg(&command, &response, lpm_drv);
	if (err || response.buf[0] != 0 ||
	    response.buf[1] != command.buf[1])
		return -EIO;

	command.command_id = LPM_MSG_EDID_INFO,
	command.buf[0] = ST_LPM_EDID_BLK_SIZE;
	command.buf[1] = ST_LPM_EDID_BLK_END;
	command.buf[2] = 0;
	err = lpm_exchange_msg(&command, &response, lpm_drv);
	if (err || response.buf[0] != 0 ||
	    response.buf[1] != ST_LPM_EDID_BLK_END)
		return -EIO;

	return 0;
}

static int lpm_read_edid_info(u8 *edid_buf, u8 block_num, void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;
	int err = 0;
	u32 offset;
	struct lpm_message command = {0};
	struct lpm_message response = {0};

	if (!lpm_check_fw_version(&lpm_drv->fw_ver, LPM_EDID)) {
		dev_warn(lpm_drv->dev,
			 "This LPM Fw does not support EDID service\n");
		return -EINVAL;
	}

	if (block_num > ST_LPM_EDID_MAX_BLK_NUM) {
		dev_err(lpm_drv->dev, "Block number is out of range\n");
		return -EINVAL;
	}

	command.command_id = LPM_MSG_EDID_INFO,
	command.buf[0] = 0;
	command.buf[1] = block_num;
	err = lpm_exchange_msg(&command, &response, lpm_drv);
	if (err || response.buf[0] != ST_LPM_EDID_BLK_SIZE ||
	    response.buf[1] != command.buf[1])
		return -EIO;
	offset = get_unaligned_le32(&response.buf[2]);
	memcpy_fromio(edid_buf, lpm_drv->iomem_base[0] + DATA_OFFSET + offset,
		      ST_LPM_EDID_BLK_SIZE);

	return 0;
}

#ifdef MY_DEF_HERE
 
static int lpm_reload_fw_prepare(void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;
	struct st_lpm_adv_feature feature;
	int err = 0, i = 0;
	u32 power_status;

	if (!lpm_check_fw_version(&lpm_drv->fw_ver, LPM_FW_RELOAD)) {
		dev_warn(lpm_drv->dev, "FW reload not supported\n");
		return -EOPNOTSUPP;
	}

	dev_dbg(lpm_drv->dev, "Go for SBC IDLE\n");

	mutex_lock(&lpm_drv->fwlock);

	feature.feature_name = ST_LPM_SBC_IDLE;
	feature.params.set_params[0] = 0x55;
	feature.params.set_params[1] = 0xAA;

	st_lpm_set_adv_feature(1, &feature);

	do {
		power_status = readl(lpm_drv->iomem_base[2] + LPM_POWER_STATUS);
	} while (((power_status & XP70_IDLE_MODE) == 0) && (i++ < 1000));

	if (power_status == 0) {
		dev_info(lpm_drv->dev, "Failed to put SBC in Idle\n");
		mutex_unlock(&lpm_drv->fwlock);
		return -ETIMEDOUT;
	}

	dev_dbg(lpm_drv->dev, "SBC is in IDLE\n");

	writel(BIT(1), lpm_drv->iomem_base[2]);

	i = 0;

	do {
		power_status = readl(lpm_drv->iomem_base[2] + LPM_POWER_STATUS);
	} while (((power_status & XP70_IDLE_MODE) != 0) && (i++ < 1000));

	if (power_status != 0) {
		dev_info(lpm_drv->dev, "SBC not ready for new Fw\n");
		mutex_unlock(&lpm_drv->fwlock);
		return -ETIMEDOUT;
	}

	lpm_drv->fwstatus = false;

	mutex_unlock(&lpm_drv->fwlock);

	dev_dbg(lpm_drv->dev, "SBC is ready for new Fw\n");

	return err;
}

static int lpm_start_loaded_fw(void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;
	int err, i;

	dev_info(lpm_drv->dev, "Booting SBC Firmware...\n");

	mutex_lock(&lpm_drv->fwlock);

	writel(3, lpm_drv->iomem_base[2]);
	mdelay(10);
	writel(1, lpm_drv->iomem_base[2]);

	lpm_drv->sbc_state = ST_LPM_SBC_BOOT;
	lpm_write32(lpm_drv, 1, MBX_INT_SET_ENABLE, 0xFF);

	for (i = 0; i < 10; i++) {
		mdelay(100);
		err = st_lpm_get_fw_state(&lpm_drv->sbc_state);
		if (err == 0 && lpm_drv->sbc_state == ST_LPM_SBC_RUNNING)
			break;
	}

	if (err) {
		dev_err(lpm_drv->dev, "Failed to start SBC Fw\n");
		mutex_unlock(&lpm_drv->fwlock);
		return err;
	}

	lpm_drv->fwstatus = true;

	lpm_sec_init(lpm_drv);

	st_lpm_notify(ST_LPM_GPIO_WAKEUP);

	mutex_unlock(&lpm_drv->fwlock);

	dev_info(lpm_drv->dev, "SBC Fw booted.\n");

	return 0;
}
#endif  

static int lpm_offset_dmem(enum st_lpm_sbc_dmem_offset_type type, void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;

	if (!lpm_drv->data_ptr)
		return -EINVAL;

	return  lpm_drv->data_ptr[type];
}

#ifdef MY_DEF_HERE
#else  
static int lpm_config_reboot(enum st_lpm_config_reboot_type type, void *data)
{
	struct st_lpm_adv_feature feature;

	feature.feature_name = ST_LPM_SET_EXIT_CPS;

	switch (type) {
	case ST_LPM_REBOOT_WITH_DDR_SELF_REFRESH:
	 
		feature.params.set_params[0] = CPS_EXIT_EXIT_CPS;
		break;

	case ST_LPM_REBOOT_WITH_DDR_OFF:
	 
		feature.params.set_params[0] = DEFAULT_EXIT_CPS;
		break;
	}

	st_lpm_set_adv_feature(1, &feature);

	return 0;
}
#endif  

static void lpm_ir_enable(bool enable, void *data)
{
	struct st_lpm_driver_data *lpm_drv = data;
	u32 lpm_config_1 = lpm_read32(lpm_drv, 2, LPM_CONFIG_1_OFFSET);

	if (enable)
		lpm_config_1 |= IRB_ENABLE;
	else
		lpm_config_1 &= ~IRB_ENABLE;

	lpm_write32(lpm_drv, 2, LPM_CONFIG_1_OFFSET, lpm_config_1);
}

static struct st_lpm_ops st_lpm_mb_ops = {
	.exchange_msg = lpm_exchange_msg,
	.write_bulk = lpm_write_bulk,
	.read_bulk = lpm_read_bulk,
	.get_dmem_offset = lpm_offset_dmem,
	.config_reboot = lpm_config_reboot,
	.ir_enable = lpm_ir_enable,
	.write_edid = lpm_write_edid_info,
	.read_edid = lpm_read_edid_info,
#ifdef MY_DEF_HERE
	.reload_fw_prepare = lpm_reload_fw_prepare,
	.start_loaded_fw = lpm_start_loaded_fw,
	.setup_tracedata = lpm_setup_tracedata,
#endif  
};

static struct of_device_id lpm_child_match_table[] = {
	{ .compatible = "st,rtc-sbc", },
	{ .compatible = "st,wakeup-pins", },
	{ }
};

#ifdef MY_DEF_HERE
#ifndef CONFIG_SBC_FW_LOADED_BY_PBL
static int lpm_load_fw(struct st_lpm_driver_data *lpm_drv, bool reload)
{
	int i, ret;
	struct elf64_hdr *ehdr;
	struct elf64_phdr *phdr;
	const u8 *elf_data;

	signed long offset;
	unsigned long size;
	int dmem_offset;

	const struct firmware *fw = lpm_drv->fw;

	if (!reload) {
		if (unlikely(!lpm_drv->fw)) {
			dev_info(lpm_drv->dev, "LPM Firmware not found.");
			dev_info(lpm_drv->dev, "Controller Passive Standby not operational.");
			dev_info(lpm_drv->dev, "Use sysfs FW trigger to load and activate LPM Firmware\n");
			return -EINVAL;
		}
	} else {
		ret = request_firmware(&fw, lpm_drv->fw_name, lpm_drv->dev);
		if (ret || unlikely(!fw)) {
			dev_err(lpm_drv->dev, "LPM Firmware not found.");
			return ret;
		}
	}

	dev_info(lpm_drv->dev, "SBC Fw %s Found, Loading ...\n",
		 lpm_drv->fw_name);

	mutex_lock(&lpm_drv->fwlock);

	elf_data = fw->data;
	ehdr = (struct elf64_hdr *)elf_data;
	phdr = (struct elf64_phdr *)(elf_data + ehdr->e_phoff);

	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_paddr != SBC_PRG_MEMORY_ELF_MARKER) {
			if (phdr->p_paddr == 0 && lpm_drv->data_ptr) {
				dmem_offset = lpm_drv->data_ptr
						[ST_SBC_DMEM_PEN_HOLDING_VAR];
				dmem_offset += DMEM_PEN_HOLDING_CODE_SZ;
				offset = DATA_OFFSET + dmem_offset;
					size = phdr->p_memsz;
				memcpy_toio(lpm_drv->iomem_base[0] + offset,
					elf_data + phdr->p_offset + dmem_offset,
					size - dmem_offset);
				continue;
			}
			offset = DATA_OFFSET + phdr->p_paddr;
		} else
			offset = lpm_drv->pmem_offset;

		size = phdr->p_memsz;

		memcpy_toio(lpm_drv->iomem_base[0] + offset,
			    elf_data + phdr->p_offset, size);
	}

	mutex_unlock(&lpm_drv->fwlock);

	dev_info(lpm_drv->dev, "SBC Fw %s loaded.\n", lpm_drv->fw_name);
	return 0;
}

static int lpm_firmware_cb(const struct firmware *fw,
		struct st_lpm_driver_data *lpm_drv)
{
	int err = 1;

	lpm_drv->fw = fw;

	err = lpm_load_fw(lpm_drv, false);
	if (err >= 0)
		err = lpm_start_loaded_fw(lpm_drv);

	if (err >= 0)
		of_platform_populate(lpm_drv->dev->of_node,
				     lpm_child_match_table,
				     NULL, lpm_drv->dev);

	return err;
}
#endif
#else  
static void lpm_sec_init(struct st_lpm_driver_data *lpm_drv_p)
{
	struct st_lpm_version driver_ver;
	int err = 0;

	dev_dbg(lpm_drv_p->dev, "SBC Firmware is running\n");

	err = st_lpm_get_version(&driver_ver, &lpm_drv_p->fw_ver);

	if (err >= 0) {
		dev_info(lpm_drv_p->dev, "LPM: firmware ver %d.%d.%d",
			 lpm_drv_p->fw_ver.major_soft,
			 lpm_drv_p->fw_ver.minor_soft,
			 lpm_drv_p->fw_ver.patch_soft);
		dev_info(lpm_drv_p->dev,
			 "(%02d-%02d-20%02d)\n", lpm_drv_p->fw_ver.day,
			 lpm_drv_p->fw_ver.month, lpm_drv_p->fw_ver.year);
	}

	st_lpm_config_reboot(ST_LPM_REBOOT_WITH_DDR_OFF);

	if (lpm_drv_p->power_on_gpio >= 0) {
		struct st_lpm_pio_setting pio_setting = {
			.pio_bank = lpm_drv_p->power_on_gpio / 8,
			.pio_pin = lpm_drv_p->power_on_gpio % 8,
			.pio_level = lpm_drv_p->trigger_level ?
				ST_LPM_PIO_LOW : ST_LPM_PIO_HIGH,
			.pio_use = ST_LPM_PIO_POWER,
			.pio_direction = 1,
			.interrupt_enabled = 0,
		};

		err = st_lpm_setup_pio(&pio_setting);
		if (err < 0)
			dev_err(lpm_drv_p->dev,
				"st_lpm_setup_pio failed: %d\n", err);
	}

	of_platform_populate(lpm_drv_p->dev->of_node, lpm_child_match_table,
			     NULL, lpm_drv_p->dev);

	lpm_drv_p->fwstatus = true;
}

static int lpm_start_fw(struct st_lpm_driver_data *lpm_drv)
{
	int ret, i;
	u32 confreg;

	dev_dbg(lpm_drv->dev, "Booting SBC Firmware...\n");

	confreg = readl(lpm_drv->iomem_base[2]);
	confreg = confreg | BIT(0);
	writel(confreg, lpm_drv->iomem_base[2]);

	msleep(100);  

	for (i = 0; i < 10; ++i) {
		ret = st_lpm_get_fw_state(&lpm_drv->sbc_state);
		if (ret < 0 || lpm_drv->sbc_state == ST_LPM_SBC_RUNNING)
			break;
		msleep(100);
	}

	return ret;
}

static int lpm_load_fw(const struct firmware *fw,
		struct st_lpm_driver_data *lpm_drv_p)
{
	int i, err = 0;
	struct elf64_hdr *ehdr;
	struct elf64_phdr *phdr;
	const u8 *elf_data;

	signed long offset;
	unsigned long size;
	int dmem_offset;

	dev_dbg(lpm_drv_p->dev,
		" %s firmware not running. Requested firmware (%s)\n",
		__func__, lpm_drv_p->fw_name);

	if (unlikely(!fw)) {
		dev_info(lpm_drv_p->dev, "LPM Firmware not found.");
		dev_info(lpm_drv_p->dev, "Controller Passive Standby not operational. Use sysfs FW trigger to load and activate LPM Firmware\n");
		return -EINVAL;
	}

	dev_dbg(lpm_drv_p->dev, "SBC Fw %s Found, Loading ...\n",
			lpm_drv_p->fw_name);

	elf_data = fw->data;
	ehdr = (struct elf64_hdr *)elf_data;
	phdr = (struct elf64_phdr *)(elf_data + ehdr->e_phoff);

	for (i = 0; i < ehdr->e_phnum; i++, phdr++) {
		if (phdr->p_type != PT_LOAD)
			continue;

		if (phdr->p_paddr != SBC_PRG_MEMORY_ELF_MARKER) {
			if (phdr->p_paddr == 0 && lpm_drv_p->data_ptr) {
				dmem_offset = lpm_drv_p->data_ptr
						[ST_SBC_DMEM_PEN_HOLDING_VAR];
				dmem_offset += DMEM_PEN_HOLDING_CODE_SZ;
				offset = DATA_OFFSET + dmem_offset;
					size = phdr->p_memsz;
				memcpy_toio(lpm_drv_p->iomem_base[0] + offset,
					elf_data + phdr->p_offset + dmem_offset,
					size - dmem_offset);
				continue;
			}
			offset = DATA_OFFSET + phdr->p_paddr;
		} else
			offset = lpm_drv_p->pmem_offset;

		size = phdr->p_memsz;

		memcpy_toio(lpm_drv_p->iomem_base[0] + offset,
			    elf_data + phdr->p_offset, size);
	}

	err = lpm_start_fw(lpm_drv_p);
	if (err) {
		dev_err(lpm_drv_p->dev, "Failed to start SBC Fw\n");
		return err;
	}

	lpm_sec_init(lpm_drv_p);

	return 1;
}
#endif  

static ssize_t st_lpm_show_wakeup(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int i;
	char data[2] = {0, 0};
	char wkup_dev_name[50] = "";
	enum st_lpm_wakeup_devices wakeup_device = 0;
#ifdef MY_DEF_HERE
	s16 ValidSize;
#endif  

	if (st_lpm_get_wakeup_device(&wakeup_device) < 0)
		dev_err(dev, "<%s> firmware not responding\n", __func__);
#ifdef MY_DEF_HERE
	if (ST_LPM_WAKEUP_PIO & wakeup_device)
		if (st_lpm_get_wakeup_info(ST_LPM_WAKEUP_PIO, &ValidSize,
					   2, data) < 0)
			dev_err(dev, "<%s> trigger data message failed\n",
					__func__);
#else  
	if (ST_LPM_WAKEUP_PIO & wakeup_device)
		if (st_lpm_get_trigger_data(ST_LPM_WAKEUP_PIO, 2, 2, data) < 0)
			dev_err(dev, "<%s> trigger data message failed\n",
					__func__);
#endif  

	for (i = 0; i < sizeof(wkup_dev_name_tab) /
			sizeof(struct st_lpm_wkup_dev_name); i++) {
		if (wkup_dev_name_tab[i].index & wakeup_device) {
			if (strlen(wkup_dev_name_tab[i].name) <
					ARRAY_SIZE(wkup_dev_name))
				sprintf(wkup_dev_name, "%s",
					wkup_dev_name_tab[i].name);
		}
	}

	return sprintf(buf, "{%s} 0x%x 0x%x\n", wkup_dev_name,
			data[0], data[1]);
}

#ifdef MY_DEF_HERE
#ifndef CONFIG_SBC_FW_LOADED_BY_PBL
static ssize_t st_lpm_load_fw(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t count)
{
	struct st_lpm_driver_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	if (val != 1) {
		dev_err(dev,
			"Invalid value given on loadfw, legal value is 1\n");
		return -EINVAL;
	}

	if (data->fwstatus) {
		dev_err(dev, "LPM F/w already loaded and running\n");
		ret = -EBUSY;
		goto exit;
	}

	ret = lpm_load_fw(data, false);
	if (ret >= 0)
		ret = lpm_start_loaded_fw(data);

	if (ret >= 0) {
		of_platform_populate(data->dev->of_node,
				     lpm_child_match_table,
				     NULL, data->dev);

		ret = count;
	}

exit:
	return ret;
}

static ssize_t st_lpm_reload_fw(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	struct st_lpm_driver_data *lpm_drv = dev_get_drvdata(dev);
	int ret;

	ret = snprintf(lpm_drv->fw_name, count, "%s", buf);

	BUG_ON(ret >= sizeof(lpm_drv->fw_name));

	dev_info(lpm_drv->dev, "About to load new firmware (%s)\n",
			 lpm_drv->fw_name);

	ret = request_firmware(&lpm_drv->fw, lpm_drv->fw_name, lpm_drv->dev);
	if (ret) {
		dev_warn(lpm_drv->dev, "LPM Fw %s not found, abort\n",
			lpm_drv->fw_name);
		goto exit;
	}

	ret = lpm_reload_fw_prepare(lpm_drv);
	if (ret < 0) {
		goto exit;
	}

	ret = lpm_load_fw(lpm_drv, true);
	if (ret >= 0)
		ret = lpm_start_loaded_fw(lpm_drv);

	if (ret >= 0)
		ret = count;

exit:
	return ret;
}

static DEVICE_ATTR(loadfw, S_IWUSR, NULL, st_lpm_load_fw);
static DEVICE_ATTR(reloadfw, S_IWUSR, NULL, st_lpm_reload_fw);
#endif

static ssize_t st_lpm_set_tracedata_config(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf,
					   size_t count)
{
	struct st_lpm_driver_data *lpm_drv = dev_get_drvdata(dev);
	unsigned int val;
	int ret;

	ret = sscanf(buf, "0x%04X", &val);
	if (ret)
		lpm_setup_tracedata(val, lpm_drv);

	return count;
}

static DEVICE_ATTR(wakeup, S_IRUGO, st_lpm_show_wakeup, NULL);
static DEVICE_ATTR(trace_config, S_IWUSR, NULL, st_lpm_set_tracedata_config);

#else  
static ssize_t st_lpm_load_fw(struct device *dev,
			      struct device_attribute *attr, const char *buf,
			      size_t count)
{
	struct st_lpm_driver_data *data = dev_get_drvdata(dev);
	unsigned long val;
	int ret;
	const struct firmware *fw;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return ret;

	if (val != 1) {
		dev_err(dev,
			"Invalid value given on loadfw, legal value is 1\n");
		return -EINVAL;
	}

	mutex_lock(&data->fwlock);

	if (data->fwstatus) {
		dev_err(dev, "LPM F/w already loaded and running\n");
		ret = -EBUSY;
		goto exit;
	}

	ret = request_firmware(&fw, data->fw_name, dev);
	if (ret) {
		dev_err(dev, "LPM F/w load failed\n");
		goto exit;
	}

	ret = lpm_load_fw(fw, data);
	if (ret < 0) {
		dev_err(dev, "LPM F/w not loaded to device or not running\n");
		goto exit;
	}

	ret = count;
exit:
	mutex_unlock(&data->fwlock);
	return ret;
}

static DEVICE_ATTR(wakeup, S_IRUGO, st_lpm_show_wakeup, NULL);
static DEVICE_ATTR(loadfw, S_IWUSR, NULL, st_lpm_load_fw);
#endif  

static int st_sbc_get_devtree_pdata(struct device *dev,
				    struct st_sbc_platform_data *pdata,
				    struct st_lpm_driver_data *lpm_drv)
{
	int result;
	struct device_node *node;
	enum of_gpio_flags flags;
	const char *fw_name;

	node = dev->of_node;
	if (node == NULL)
		return -ENODEV;

	memset(pdata, 0, sizeof(*pdata));

	if (!of_find_property(node, "gpios", NULL)) {
		dev_err(dev, "LPM without power-on-gpio\n");
		return -ENODEV;
	}

	result = of_get_gpio_flags(node, 0, &flags);
	if (result < 0)
		return result;

	pdata->gpio = (unsigned int)result;
	pdata->trigger_level = !(flags & OF_GPIO_ACTIVE_LOW);

	if (of_property_read_string(node, "st,fw_name", &fw_name))
		return -ENODEV;

	result = snprintf(lpm_drv->fw_name, sizeof(lpm_drv->fw_name),
			  "%s", fw_name);

	BUG_ON(result >= sizeof(lpm_drv->fw_name));

	return 0;
}

#ifdef MY_DEF_HERE
#ifdef CONFIG_PM_SLEEP
static void st_lpm_check_wakeup_device(
	struct st_lpm_driver_data *lpm_drv,
	struct device *dev, unsigned int enable)
{
	int bit = 0;

	pr_info("sti: -> device %s as wakeup %s\n", dev_name(dev),
		(enable ? "enabled" : "disabled"));

	if (strstr(dev_name(dev), "rc"))
		bit = ST_LPM_WAKEUP_IR;
	else if (strstr(dev_name(dev), "dwmac"))
		bit = ST_LPM_WAKEUP_WOL;
	else if (strstr(dev_name(dev), "st-keyscan"))
		bit = ST_LPM_WAKEUP_FRP;
	else if (strstr(dev_name(dev), "ttyAS"))
		bit = ST_LPM_WAKEUP_ASC;
	else if (strstr(dev_name(dev), "rtc_sbc"))
		bit = ST_LPM_WAKEUP_RTC;
	else if (strstr(dev_name(dev), "stm_cec"))
		bit = ST_LPM_WAKEUP_CEC;
	else if (strstr(dev_name(dev), "gpio_keys"))
		bit = ST_LPM_WAKEUP_PIO;
	else if (strstr(dev_name(dev), "st_wakeup_pin.pio"))
		bit = ST_LPM_WAKEUP_PIO;
	else if (strstr(dev_name(dev), "st_wakeup_pin.ext_it"))
		bit = ST_LPM_WAKEUP_EXT;
	else if (strstr(dev_name(dev), "hdmi"))
		bit = ST_LPM_WAKEUP_HPD;
	else {
		dev_dbg(lpm_drv->dev,
			"%s: device wakeup not managed via lpm\n",
			dev_name(dev));
		return;
	}

	if (enable)
		lpm_drv->wakeup_bitmap |= bit;
	else
		lpm_drv->wakeup_bitmap &= ~bit;
}

static int sti_wakeup_notifier(struct notifier_block *nb,
	unsigned long action, void *data)
{
	struct st_lpm_driver_data *lpm_drv =
		container_of(nb, struct st_lpm_driver_data, wakeup_cb);
	struct device *dev = data;

	switch (action) {
	case WAKEUP_SOURCE_ADDED:
	case WAKEUP_SOURCE_REMOVED:
		st_lpm_check_wakeup_device(lpm_drv, dev,
			action == WAKEUP_SOURCE_ADDED ? 1UL : 0UL);
		break;
	}

	return NOTIFY_OK;
}
#else
static inline int sti_wakeup_notifier(struct notifier_block *nb,
	unsigned long action, void *data)
{
	return NOTIFY_OK;
}
#endif
#else  
static void st_lpm_check_wakeup_device(
	struct st_lpm_driver_data *lpm_drv,
	struct device *dev, unsigned int enable)
{
	int bit = 0;

	pr_info("sti: -> device %s as wakeup %s\n", dev_name(dev),
		(enable ? "enabled" : "disabled"));

	if (strstr(dev_name(dev), "rc"))
		bit = ST_LPM_WAKEUP_IR;
	else if (strstr(dev_name(dev), "dwmac"))
		bit = ST_LPM_WAKEUP_WOL;
	else if (strstr(dev_name(dev), "st-keyscan"))
		bit = ST_LPM_WAKEUP_FRP;
	else if (strstr(dev_name(dev), "ttyAS"))
		bit = ST_LPM_WAKEUP_ASC;
	else if (strstr(dev_name(dev), "rtc_sbc"))
		bit = ST_LPM_WAKEUP_RTC;
	else if (strstr(dev_name(dev), "stm_cec"))
		bit = ST_LPM_WAKEUP_CEC;
	else if (strstr(dev_name(dev), "gpio_keys"))
		bit = ST_LPM_WAKEUP_PIO;
	else if (strstr(dev_name(dev), "st_wakeup_pin.pio"))
		bit = ST_LPM_WAKEUP_PIO;
	else if (strstr(dev_name(dev), "st_wakeup_pin.ext_it"))
		bit = ST_LPM_WAKEUP_EXT;
	else if (strstr(dev_name(dev), "hdmi"))
		bit = ST_LPM_WAKEUP_HPD;
	else {
		dev_dbg(lpm_drv->dev,
			"%s: device wakeup not managed via lpm\n",
			dev_name(dev));
		return;
	}

	if (enable)
		lpm_drv->wakeup_bitmap |= bit;
	else
		lpm_drv->wakeup_bitmap &= ~bit;
}

static int sti_wakeup_notifier(struct notifier_block *nb,
	unsigned long action, void *data)
{
	struct st_lpm_driver_data *lpm_drv =
		container_of(nb, struct st_lpm_driver_data, wakeup_cb);
	struct device *dev = data;

	switch (action) {
	case WAKEUP_SOURCE_ADDED:
	case WAKEUP_SOURCE_REMOVED:
		st_lpm_check_wakeup_device(lpm_drv, dev,
			action == WAKEUP_SOURCE_ADDED ? 1UL : 0UL);
		break;
	}

	return NOTIFY_OK;
}
#endif  

static int st_lpm_probe(struct platform_device *pdev)
{
	struct st_lpm_driver_data *lpm_drv;
	const struct st_sbc_platform_data *pdata = pdev->dev.platform_data;
	struct st_sbc_platform_data alt_pdata;
	struct resource *res;
	int err = 0;
	int count = 0;
	u32 confreg;

#ifdef MY_DEF_HERE
	dev_dbg(&pdev->dev, "st lpm probe\n");
#else  
	dev_dbg(lpm_drv->dev, "st lpm probe\n");
#endif  

	lpm_drv = devm_kzalloc(&pdev->dev, sizeof(struct st_lpm_driver_data),
				GFP_KERNEL);
	if (unlikely(lpm_drv == NULL)) {
		dev_err(lpm_drv->dev, "%s: Request memory failed\n", __func__);
		return -ENOMEM;
	}

	lpm_drv->dev = &pdev->dev;
	if (!pdata) {
		err = st_sbc_get_devtree_pdata(&pdev->dev, &alt_pdata, lpm_drv);
		if (err)
			return err;
		pdata = &alt_pdata;
	}

	lpm_drv->data_ptr = of_match_node(pdev->dev.driver->of_match_table,
			 pdev->dev.of_node)->data;

	lpm_drv->power_on_gpio = pdata->gpio;
	lpm_drv->trigger_level = pdata->trigger_level;

	err = devm_gpio_request(&pdev->dev, lpm_drv->power_on_gpio,
				"lpm_power_on_gpio");
	if (err) {
		dev_err(lpm_drv->dev, "%s: gpio_request failed\n", __func__);
		return err;
	}

	err = gpio_direction_output(lpm_drv->power_on_gpio,
			lpm_drv->trigger_level);
	if (err) {
		dev_err(lpm_drv->dev, "%s: gpio_direction_output failed\n",
				__func__);
		return err;
	}

	for (count = 0; count < 3; count++) {
		res = platform_get_resource(pdev, IORESOURCE_MEM, count);
		if (!res)
			return -ENODEV;

		dev_dbg(lpm_drv->dev,
			"mem:SBC res->start %x %x\n", res->start, res->end);
		if (!devm_request_mem_region(&pdev->dev, res->start,
					     resource_size(res), "st-lpm")) {
			dev_err(lpm_drv->dev, "%s: Request mem 0x%x region failed\n",
			       __func__, res->start);
			return -ENOMEM;
		}

		lpm_drv->iomem_base[count] = devm_ioremap_nocache(&pdev->dev,
							res->start,
							resource_size(res));
		if (!lpm_drv->iomem_base[count]) {
			dev_err(lpm_drv->dev, "%s: Request iomem 0x%x region failed\n",
			       __func__, (unsigned int)res->start);
			return -ENOMEM;
		}
		dev_dbg(lpm_drv->dev,
			"lpm_add %x\n", (u32)lpm_drv->iomem_base[count]);
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "pmem");
	if (!res)
		return -ENODEV;

	lpm_drv->pmem_offset = res->start;

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(lpm_drv->dev, "%s irq resource %x not provided\n",
			__func__, res->start);
		return -ENODEV;
	}

	init_waitqueue_head(&lpm_drv->wait_queue);
	mutex_init(&lpm_drv->mutex);

	mutex_init(&lpm_drv->fwlock);

	if (devm_request_threaded_irq(&pdev->dev, res->start, lpm_isr,
		lpm_threaded_isr, IRQF_NO_SUSPEND,
		"st-lpm", (void *)lpm_drv) < 0) {
		dev_err(lpm_drv->dev, "%s: Request stlpm irq not done\n",
			__func__);
		return -ENODEV;
	}

	platform_set_drvdata(pdev, lpm_drv);

	lpm_write32(lpm_drv, 1, MBX_INT_SET_ENABLE, 0xFF);
	lpm_drv->sbc_state = ST_LPM_SBC_BOOT;

	lpm_drv->wakeup_cb.notifier_call = sti_wakeup_notifier;
	wakeup_source_notifier_register(&lpm_drv->wakeup_cb);

	st_lpm_set_ops(&st_lpm_mb_ops, (void *)lpm_drv);

	confreg = readl(lpm_drv->iomem_base[2]);
	if (confreg & BIT(0)) {  
		err = st_lpm_get_fw_state(&lpm_drv->sbc_state);

#ifdef MY_DEF_HERE
		if (!err && lpm_drv->sbc_state == ST_LPM_SBC_RUNNING) {
#else  
		if (lpm_drv->sbc_state == ST_LPM_SBC_RUNNING)
#endif  
			lpm_sec_init(lpm_drv);
#ifdef MY_DEF_HERE
			of_platform_populate(lpm_drv->dev->of_node,
					     lpm_child_match_table,
					     NULL, lpm_drv->dev);
		} else {
			dev_err(lpm_drv->dev,
				"SBC Fw is not responding... Abort\n");
			dev_info(lpm_drv->dev,
				"Controller Passive Standby not operational.");
			dev_info(lpm_drv->dev,
				"Use sysfs FW trigger to load and activate LPM Firmware\n");
		}
#endif  
	} else {
#ifdef CONFIG_SBC_FW_LOADED_BY_PBL
		dev_dbg(lpm_drv->dev, "SBC Firmware loaded but not booted\n");
#ifdef MY_DEF_HERE
		err = lpm_start_loaded_fw(lpm_drv);
#else  
		err = lpm_start_fw(lpm_drv);
#endif  
		if (err) {
#ifdef MY_DEF_HERE
			dev_err(lpm_drv->dev, "Failed to start SBC Fw\n");
#else  
			dev_err(pdev->dev, "Failed to start SBC Fw\n");
#endif  
			return err;
		}

#ifdef MY_DEF_HERE
		of_platform_populate(lpm_drv->dev->of_node,
				     lpm_child_match_table,
				     NULL, lpm_drv->dev);
#else  
		lpm_sec_init(lpm_drv);
#endif  
#else
		dev_dbg(lpm_drv->dev, "SBC Firmware loading requested\n");
		 
		err = request_firmware_nowait(THIS_MODULE, 1, lpm_drv->fw_name,
					      &pdev->dev, GFP_KERNEL,
					      (struct st_lpm_driver_data *)
					      lpm_drv,
#ifdef MY_DEF_HERE
					      (void *)lpm_firmware_cb);
#else  
					      (void *)lpm_load_fw);
#endif  
		if (err)
			return err;
#endif
	}

	err = device_create_file(&pdev->dev, &dev_attr_wakeup);
	if (err) {
		dev_err(&pdev->dev, "Cannot create wakeup sysfs entry\n");
		return err;
	}

#ifdef MY_DEF_HERE
#ifndef CONFIG_SBC_FW_LOADED_BY_PBL
	err = device_create_file(&pdev->dev, &dev_attr_loadfw);
	if (err) {
		dev_err(&pdev->dev, "Cannot create loadfw sysfs entry\n");
		return err;
	}

	err = device_create_file(&pdev->dev, &dev_attr_reloadfw);
	if (err) {
		dev_err(&pdev->dev, "Cannot create reloadfw sysfs entry\n");
		return err;
	}
#endif

	err = device_create_file(&pdev->dev, &dev_attr_trace_config);
	if (err) {
		dev_err(&pdev->dev, "Cannot create trace_config sysfs entry\n");
		return err;
	}
#else  
	err = device_create_file(&pdev->dev, &dev_attr_loadfw);
	if (err) {
		dev_err(&pdev->dev, "Cannot create loadfw sysfs entry\n");
		return err;
	}
#endif  

	return 0;
}

static int  st_lpm_remove(struct platform_device *pdev)
{
	struct st_lpm_driver_data *lpm_drv =
		platform_get_drvdata(pdev);

	dev_dbg(lpm_drv->dev, "st_lpm_remove\n");

	return 0;
}

unsigned int stih407_family_lpm_data[] = {0x94, 0x84, 0xa4};

static struct of_device_id st_lpm_match[] = {
	{
		.compatible = "st,stih407-family-lpm",
		.data = stih407_family_lpm_data,
	},
	{
		.compatible = "st,lpm",
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, st_lpm_match);

#ifdef MY_DEF_HERE
static void st_lpm_setup_ir_init(void)
{
	static struct st_lpm_ir_keyinfo ir_key[2] = {
	[0] = {
		.ir_id = 8,
		.time_period = 560,
		.tolerance = 30,
		.ir_key.key_index = 0x0,
		.ir_key.num_patterns = 33,
		.ir_key.fifo = {
			[0] = {
				.mark = 8960,
				.symbol = 13440,
			},
			[1 ... 8] = {
				.mark = 560,
				.symbol = 1120,
			},
			[9 ... 15] = {
				.mark = 560,
				.symbol = 560 * 4,
			},
			[16 ... 24] = {
				.mark = 560,
				.symbol = 1120,
			},
			[25 ... 32] = {
				.mark = 560,
				.symbol = 560 * 4,
			},
			[33] = {
				.mark = 560,
				.symbol = 560 * 2,
			},
		},
	},

	[1] = {
		.ir_id = 0x7,
		.time_period = 900,
		.tolerance = 30,
		.ir_key.key_index = 0x2,
		.ir_key.num_patterns = 10,
		.ir_key.fifo = {
			[0] = {
				.mark = 900,
				.symbol = 1800,
			},
			[1] = {
				.mark = 1800,
				.symbol = 2700,
			},
			[2] = {
				.mark = 900,
				.symbol = 2700,
			},
			[3] = {
				.mark = 1800,
				.symbol = 2700,
			},
			[4 ... 6] = {
				.mark = 900,
				.symbol = 1800,
			},
			[7] = {
				.mark = 900,
				.symbol = 2700,
			},
			[8] = {
				.mark = 900,
				.symbol = 1800,
			},
			[9] = {
				.mark = 1800,
				.symbol = 2700,
			},
		},
		},
	};
	static int st_lpm_setup_ir_done_once;

	if (st_lpm_setup_ir_done_once)
		return;
	 
	st_lpm_setup_ir(ARRAY_SIZE(ir_key), ir_key);
	st_lpm_setup_ir_done_once = 1;
}

#ifdef CONFIG_PM_SLEEP
static int st_lpm_pm_suspend(struct device *dev)
{
	struct st_lpm_driver_data *lpm_drv = dev_get_drvdata(dev);
	struct st_lpm_version *version;
	u16 current_traces;

	st_lpm_setup_ir_init();

	if (!lpm_drv->wakeup_bitmap) {
		 
		pr_err("No device is able to wakeup\n");
		return -EINVAL;
	}
	st_lpm_set_wakeup_device(lpm_drv->wakeup_bitmap);

	if (lpm_drv->trace_data_value &&
	    !lpm_check_fw_version(&lpm_drv->fw_ver, LPM_SBC_TRACES_IN_SUSPEND)) {
		version = &lpm_fw_ver_vs_services[LPM_SBC_TRACES_IN_SUSPEND];

		dev_warn(lpm_drv->dev,
			 "This LPM Fw does not support SBC traces during suspend\n");
		dev_warn(lpm_drv->dev,
			 "SBC traces during suspend available from LPM Fw v%d.%d.%d\n",
			 version->major_soft, version->minor_soft,
			 version->patch_soft);

		current_traces = lpm_drv->trace_data_value;
		lpm_setup_tracedata(0x0, lpm_drv);
		lpm_drv->trace_data_value = current_traces;
	}

	lpm_config_reboot(ST_LPM_REBOOT_WITH_DDR_SELF_REFRESH, lpm_drv);

	return 0;
}

static int st_lpm_pm_resume(struct device *dev)
{
	struct st_lpm_driver_data *lpm_drv = dev_get_drvdata(dev);

	if (lpm_drv->trace_data_value &&
	    !lpm_check_fw_version(&lpm_drv->fw_ver, LPM_SBC_TRACES_IN_SUSPEND))
		lpm_setup_tracedata(lpm_drv->trace_data_value, lpm_drv);
	 
	lpm_config_reboot(ST_LPM_REBOOT_WITH_DDR_OFF, lpm_drv);

	return 0;
}

SIMPLE_DEV_PM_OPS(st_lpm_pm_ops, st_lpm_pm_suspend, st_lpm_pm_resume);
#define ST_LPM_PM_OPS		(&st_lpm_pm_ops)
#else
#define ST_LPM_PM_OPS		NULL
#endif
#else  
#ifdef CONFIG_PM
static void st_lpm_setup_ir_init(void)
{
	static struct st_lpm_ir_keyinfo ir_key[2] = {
	[0] = {
		.ir_id = 8,
		.time_period = 560,
		.tolerance = 30,
		.ir_key.key_index = 0x0,
		.ir_key.num_patterns = 33,
		.ir_key.fifo = {
			[0] = {
				.mark = 8960,
				.symbol = 13440,
			},
			[1 ... 8] = {
				.mark = 560,
				.symbol = 1120,
			},
			[9 ... 15] = {
				.mark = 560,
				.symbol = 560 * 4,
			},
			[16 ... 24] = {
				.mark = 560,
				.symbol = 1120,
			},
			[25 ... 32] = {
				.mark = 560,
				.symbol = 560 * 4,
			},
			[33] = {
				.mark = 560,
				.symbol = 560 * 2,
			},
		},
	},

	[1] = {
		.ir_id = 0x7,
		.time_period = 900,
		.tolerance = 30,
		.ir_key.key_index = 0x2,
		.ir_key.num_patterns = 10,
		.ir_key.fifo = {
			[0] = {
				.mark = 900,
				.symbol = 1800,
			},
			[1] = {
				.mark = 1800,
				.symbol = 2700,
			},
			[2] = {
				.mark = 900,
				.symbol = 2700,
			},
			[3] = {
				.mark = 1800,
				.symbol = 2700,
			},
			[4 ... 6] = {
				.mark = 900,
				.symbol = 1800,
			},
			[7] = {
				.mark = 900,
				.symbol = 2700,
			},
			[8] = {
				.mark = 900,
				.symbol = 1800,
			},
			[9] = {
				.mark = 1800,
				.symbol = 2700,
			},
		},
		},
	};
	static int st_lpm_setup_ir_done_once;

	if (st_lpm_setup_ir_done_once)
		return;
	 
	st_lpm_setup_ir(ARRAY_SIZE(ir_key), ir_key);
	st_lpm_setup_ir_done_once = 1;
}

static int st_lpm_pm_suspend(struct device *dev)
{
	struct st_lpm_driver_data *lpm_drv =
		dev_get_drvdata(dev);

	st_lpm_setup_ir_init();

	if (!lpm_drv->wakeup_bitmap) {
		 
		pr_err("No device is able to wakeup\n");
		return -EINVAL;
	}
	st_lpm_set_wakeup_device(lpm_drv->wakeup_bitmap);

	st_lpm_config_reboot(ST_LPM_REBOOT_WITH_DDR_SELF_REFRESH);

	return 0;
}

static int st_lpm_pm_resume(struct device *dev)
{
	 
	st_lpm_config_reboot(ST_LPM_REBOOT_WITH_DDR_OFF);

	return 0;
}

SIMPLE_DEV_PM_OPS(st_lpm_pm_ops, st_lpm_pm_suspend, st_lpm_pm_resume);
#define ST_LPM_PM_OPS		(&st_lpm_pm_ops)
#else
#define ST_LPM_PM_OPS		NULL
#endif
#endif  

static void st_lpm_shutdown(struct platform_device *pdev)
{
	struct st_lpm_driver_data *lpm_drv =
		platform_get_drvdata(pdev);
	static unsigned char reset_buf[] = LPM_DMEM_PEN_HOLD_VAR_RESET;
	int offset;

	st_lpm_setup_ir_init();

	st_lpm_set_wakeup_device(lpm_drv->wakeup_bitmap);

#ifdef MY_DEF_HERE
	lpm_config_reboot(ST_LPM_REBOOT_WITH_DDR_OFF, lpm_drv);
#else  
	st_lpm_config_reboot(ST_LPM_REBOOT_WITH_DDR_OFF);
#endif  
	offset = st_lpm_get_dmem_offset(ST_SBC_DMEM_PEN_HOLDING_VAR);
	if (offset >= 0)
		st_lpm_write_dmem(reset_buf, 4, offset);
}

static struct platform_driver st_lpm_driver = {
	.driver = {
		.name = "st-lpm",
		.owner = THIS_MODULE,
		.pm = ST_LPM_PM_OPS,
		.of_match_table = of_match_ptr(st_lpm_match),
	},
	.probe = st_lpm_probe,
	.remove = st_lpm_remove,
	.shutdown = st_lpm_shutdown,
};

static int __init st_lpm_init(void)
{
	int err = 0;

	err = platform_driver_register(&st_lpm_driver);
	if (err)
		pr_err("st-lpm driver fails on registrating (%x)\n" , err);
	else
		pr_info("st-lpm driver registered\n");

	return err;
}

void __exit st_lpm_exit(void)
{
	pr_info("st-lpm driver removed\n");
	platform_driver_unregister(&st_lpm_driver);
}

module_init(st_lpm_init);
module_exit(st_lpm_exit);

MODULE_AUTHOR("STMicroelectronics  <www.st.com>");
MODULE_DESCRIPTION("lpm device driver for STMicroelectronics devices");
MODULE_LICENSE("GPL");

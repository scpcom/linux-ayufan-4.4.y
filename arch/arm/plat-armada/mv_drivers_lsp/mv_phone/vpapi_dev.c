/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell 
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File in accordance with the terms and conditions of the General 
Public License Version 2, June 1991 (the "GPL License"), a copy of which is 
available along with the File in the license.txt file or by writing to the Free 
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or 
on the worldwide web at http://www.gnu.org/licenses/gpl.txt. 

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED 
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY 
DISCLAIMED.  The GPL License provides additional details about this warranty 
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or 
modify this File under the following licensing terms. 
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer. 

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution. 

    *   Neither the name of Marvell nor the names of its contributors may be 
        used to endorse or promote products derived from this software without 
        specific prior written permission. 
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

******************************************************************************/

#include <linux/poll.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include "vpapi_dev.h"

#if !defined(CONFIG_MV_TDM_SUPPORT) && defined(CONFIG_ZARLINK_SLIC_VE880)
#include "gpp/mvGppRegs.h"
#endif

/* Defines */
#define GET_DEV_STATUS(deviceId)	vpapi_dev_status[deviceId]
#define GET_LINE_STATUS(lineId)		vpapi_line_status[lineId]
#define REGISTER_DEVICE(deviceId)	\
	vpapi_dev_status[deviceId] = 1;
#define REGISTER_LINE(lineId)		\
	vpapi_line_status[lineId] = 1;

#define MAX_PROFILE_SIZE		128
#define GET_DEVICE(lineId)		(lineId/MAX_LINES_PER_DEVICE)
#define GET_LINE(lineId)		(lineId % MAX_LINES_PER_DEVICE)
#define MAX_EVENT_QUEUE_SIZE		256
#define VPAPI_TICK_TIMER_PERIOD		1
#define VPAPI_MOD_NAME                  "vpapi"

/* VE880 */
#if defined(CONFIG_ZARLINK_SLIC_VE880)

#define MAX_DEVICES			2
#define MAX_LINES			4
#define MAX_LINES_PER_DEVICE		2
	
static VpDevCtxType pDevCtx[MAX_DEVICES];
static VpLineCtxType pLineCtx[MAX_DEVICES][MAX_LINES_PER_DEVICE];
static Vp880DeviceObjectType pDevObj[MAX_DEVICES];
static Vp880LineObjectType pLineObj[MAX_DEVICES][MAX_LINES_PER_DEVICE];

/* VE792 */
#elif defined(CONFIG_ZARLINK_SLIC_VE792)

#define MAX_DEVICES			4
#define MAX_LINES			32
#define MAX_LINES_PER_DEVICE		8

static VpDevCtxType pDevCtx[MAX_DEVICES];
static VpLineCtxType pLineCtx[MAX_DEVICES][MAX_LINES_PER_DEVICE];
static Vp792DeviceObjectType pDevObj[MAX_DEVICES];
static Vp792LineObjectType pLineObj[MAX_DEVICES][MAX_LINES_PER_DEVICE];

extern int BattOn(int vbhSetting, int vblSetting, int vbpSetting);
extern int BattOff(void);

#endif

#if !defined(CONFIG_MV_TDM_SUPPORT) && defined(CONFIG_ZARLINK_SLIC_VE880) 
static irqreturn_t vpapi_slic_isr(int irq, void* dev_id);
#endif
static void vpapi_tick_handler(unsigned long data);
static ssize_t vpapi_read(struct file *file, char __user *buf, size_t size, loff_t * ppos);
static ssize_t vpapi_write(struct file *file, const char __user *buf, size_t size, loff_t * ppos);
static unsigned int vpapi_poll(struct file *pFile, poll_table *pPollTable);
static int vpapi_ioctl(struct inode *pInode, struct file *pFile, unsigned int cmd, unsigned long arg);
static int vpapi_open(struct inode *pInode, struct file *pFile);
static int vpapi_release(struct inode *pInode, struct file *pFile);
//static int __init vpapi_module_init(void);
//static void __exit vpapi_module_exit(void);

/* VP-API-II Dispatchers */
static int vpapi_make_dev_object(unsigned long arg);
static int vpapi_make_line_object(unsigned long arg);
static int vpapi_map_line_id(unsigned long arg);
static int vpapi_map_slac_id(unsigned long arg);
static int vpapi_free_line_context(unsigned long arg);
static int vpapi_init_device(unsigned long arg);
static int vpapi_cal_line(unsigned long arg);
static int vpapi_set_line_state(unsigned long arg);
static int vpapi_set_option(unsigned long arg);
int vpapi_get_event(unsigned long arg);
#if defined(CONFIG_ZARLINK_SLIC_VE792)
static int vpapi_batt_on(unsigned long arg);
static int vpapi_batt_off(unsigned long arg);
#endif
#if defined(CONFIG_ZARLINK_SLIC_VE880)
static int vpapi_reg_read(unsigned long arg);
static int vpapi_reg_write(unsigned long arg);
#endif

/* Enumurators */
typedef struct {
	unsigned char valid;		/* valid event */
	VpEventType vp_event;
} vpapi_event;

/* Structs */
static struct file_operations vpapi_fops = {
    owner:      THIS_MODULE,
    llseek:     NULL,
    read:       vpapi_read,
    write:      vpapi_write,
    poll:       vpapi_poll,
    ioctl:      vpapi_ioctl,
    open:       vpapi_open,
    release:    vpapi_release,
    fasync:     NULL
};

/* Globals */
static DEFINE_SPINLOCK(vpapi_lock);
static DECLARE_WAIT_QUEUE_HEAD(vpapi_wait);
static atomic_t event_count;
static atomic_t vpapi_init;
static vpapi_event event_queue[MAX_EVENT_QUEUE_SIZE];
static u8 vpapi_dev_status[MAX_DEVICES];
static u8 vpapi_line_status[MAX_LINES];
static volatile u32 next_event = 0, curr_event = 0;
static struct timer_list vpapi_timer;
static u16 total_devs = 0, total_lines = 0;

static struct miscdevice vpapi_misc_dev = {
	.minor = SLICDEV_MINOR,
	.name = VPAPI_MOD_NAME,
	.fops = &vpapi_fops,
};

static ssize_t vpapi_read(struct file *file, char __user *buf, size_t size, loff_t * ppos)
{
	return 0;
}

static ssize_t vpapi_write(struct file *file, const char __user *buf, size_t size, loff_t * ppos)
{
	return 0;
}

static unsigned int vpapi_poll(struct file *pFile, poll_table *pPollTable)
{
	int mask = 0;

	poll_wait(pFile, &vpapi_wait, pPollTable);
	
	if(atomic_read(&event_count) > 0) {
		mask |= POLLPRI;
	}

	return mask;
}

static int vpapi_ioctl(struct inode *pInode, struct file *pFile, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	unsigned long flags = 0;

	/* Argument checking */
	if (_IOC_TYPE(cmd) != VPAPI_MOD_IOCTL_MAGIC) {
		printk("%s: invalid VPAPI MOD Magic Num %i %i\n", __func__, _IOC_TYPE(cmd), VPAPI_MOD_IOCTL_MAGIC);
		return -ENOTTY;
	}

	if ((_IOC_NR(cmd) > VPAPI_MOD_IOCTL_MAX) || (_IOC_NR(cmd) < VPAPI_MOD_IOCTL_MIN)) {
		printk("%s: invalid VPAPI MOD IOCTL request\n", __func__);
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ) {
		ret = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
	} 
	else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		ret = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
	}

	if (ret) {
		printk("%s: invalid VPAPI MOD access type %i from cmd %i\n", __func__, _IOC_DIR(cmd), cmd);
		return -EFAULT;
	}

	spin_lock_irqsave(&vpapi_lock, flags);

	switch (cmd) {
		case VPAPI_MOD_IOX_MK_DEV_OBJ:
			//printk("ioctl: VPAPI_MOD_IOX_MK_DEV_OBJ\n");
			ret = vpapi_make_dev_object(arg);	
			break;
		
		case VPAPI_MOD_IOX_MK_LN_OBJ:
			//printk("ioctl: VPAPI_MOD_IOX_MK_LN_OBJ\n");
			ret = vpapi_make_line_object(arg);
			break;

		case VPAPI_MOD_IOX_MAP_LN_ID:
			//printk("ioctl: VPAPI_MOD_IOX_MAP_LN_ID\n");
			ret = vpapi_map_line_id(arg);
			break;

		case VPAPI_MOD_IOX_MAP_SLAC_ID:
			//printk("ioctl: VPAPI_MOD_IOX_MAP_SLAC_ID\n");
			ret  = vpapi_map_slac_id(arg);
			break;

		case VPAPI_MOD_IOX_FREE_LN_CTX:
			//printk("ioctl: VPAPI_MOD_IOX_FREE_LN_CTX\n");
			ret = vpapi_free_line_context(arg);
			break;

		case VPAPI_MOD_IOX_INIT_DEV:
			//printk("ioctl: VPAPI_MOD_IOX_INIT_DEV\n");
			ret = vpapi_init_device(arg);
			break;

		case VPAPI_MOD_IOX_CAL_LN:
			//printk("ioctl: VPAPI_MOD_IOX_CAL_LN\n");
			ret = vpapi_cal_line(arg);
			break;

		case VPAPI_MOD_IOX_SET_LN_ST:
			//printk("ioctl: VPAPI_MOD_IOX_SET_LN_ST\n");
			ret = vpapi_set_line_state(arg);
			break;

		case VPAPI_MOD_IOX_SET_OPTION:
			//printk("ioctl: VPAPI_MOD_IOX_SET_OPTION\n");
			ret = vpapi_set_option(arg);
			break;

		case VPAPI_MOD_IOX_GET_EVENT:
			//printk("ioctl: VPAPI_MOD_IOX_GET_EVENT\n");
			ret = vpapi_get_event(arg);
			break;

#if defined(CONFIG_ZARLINK_SLIC_VE792)		
		case VPAPI_MOD_IOX_BATT_ON:
			//printk("ioctl: VPAPI_MOD_IOX_BATT_ON\n");
			ret = vpapi_batt_on(arg);
			break;

		case VPAPI_MOD_IOX_BATT_OFF:
			//printk("ioctl: VPAPI_MOD_IOX_BATT_OFF\n");
			ret = vpapi_batt_off(arg);
			break;
#endif
#if defined(CONFIG_ZARLINK_SLIC_VE880)
		case VPAPI_MOD_IOX_REG_READ:
			ret = vpapi_reg_read(arg);
			break;

		case VPAPI_MOD_IOX_REG_WRITE:
			ret = vpapi_reg_write(arg);
			break;
#endif
		default:
			printk("%s: error, ioctl command(0x%x) not supported !!!\n", __func__, cmd);
			ret = -EFAULT;
			break;
	}

	spin_unlock_irqrestore(&vpapi_lock, flags);

	return ret;
}

static int vpapi_make_dev_object(unsigned long arg)
{
	VpApiModMkDevObjType data;
	VpDeviceType deviceType;
	VpDeviceIdType deviceId;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModMkDevObjType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	deviceType = data.deviceType;
	deviceId = data.deviceId;
	
	data.status = VpMakeDeviceObject(deviceType, deviceId, &pDevCtx[deviceId], &pDevObj[deviceId]);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModMkDevObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_make_line_object(unsigned long arg)
{
	VpApiModMkLnObjType data;
	VpTermType termType;
	VpLineIdType lineId;
	VpDeviceIdType deviceId;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModMkLnObjType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	termType = data.termType;
	lineId = GET_LINE(data.lineId);
	deviceId = GET_DEVICE(data.lineId);

	data.status = VpMakeLineObject(termType, lineId, &pLineCtx[deviceId][lineId],
                        	&pLineObj[deviceId][lineId], &pDevCtx[deviceId]);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModMkLnObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_map_line_id(unsigned long arg)
{
	VpApiModMapLnIdType data;
	VpLineIdType lineId;
	VpDeviceIdType deviceId;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModMapLnIdType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	lineId = GET_LINE(data.lineId);
	deviceId = GET_DEVICE(data.lineId);

	data.status = VpMapLineId(&pLineCtx[deviceId][lineId], data.lineId);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModMapLnIdType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_map_slac_id(unsigned long arg)
{
	VpApiModMapSlacIdType data;
	VpDeviceIdType deviceId;
	u8 slacId;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModMapSlacIdType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}
	
	deviceId = data.deviceId;
	slacId = data.slacId;

	data.status = VpMapSlacId(&pDevCtx[deviceId], slacId);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModMapSlacIdType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
	
}

static int vpapi_free_line_context(unsigned long arg)
{
	VpApiModFreeLnCtxType data;
	VpLineIdType lineId;
	VpDeviceIdType deviceId;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModFreeLnCtxType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	lineId = GET_LINE(data.lineId);
	deviceId = GET_DEVICE(data.lineId);

	data.status = VpFreeLineCtx(&pLineCtx[deviceId][lineId]);

	if(data.status == VP_STATUS_SUCCESS)
		vpapi_line_status[data.lineId] = 0;

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModFreeLnCtxType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_init_device(unsigned long arg)
{
	VpApiModInitDeviceType data;
	VpDeviceIdType deviceId;
	VpProfileDataType devProfile[MAX_PROFILE_SIZE];
	VpProfileDataType acProfile[MAX_PROFILE_SIZE];
	VpProfileDataType dcProfile[MAX_PROFILE_SIZE];
	VpProfileDataType ringProfile[MAX_PROFILE_SIZE];
	VpProfileDataType fxoAcProfile[MAX_PROFILE_SIZE];
	VpProfileDataType fxoCfgProfile[MAX_PROFILE_SIZE];
	VpProfilePtrType pDevProfile = NULL, pAcProfile = NULL;
	VpProfilePtrType pDcProfile = NULL, pRingProfile = NULL;
	VpProfilePtrType pFxoAcProfile = NULL, pFxoCfgProfile = NULL;
	u16 devProfileSize, acProfileSize, dcProfileSize;
	u16 ringProfileSize, fxoAcProfileSize, fxoCfgProfileSize;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModInitDeviceType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	deviceId = data.deviceId;
	devProfileSize = data.devProfileSize;
	acProfileSize = data.acProfileSize;
	dcProfileSize = data.dcProfileSize;
	ringProfileSize = data.ringProfileSize;
	fxoAcProfileSize = data.fxoAcProfileSize;
	fxoCfgProfileSize = data.fxoCfgProfileSize;

	if(devProfileSize) {
		/* Get device profile */
		if(copy_from_user(devProfile, (void*)data.pDevProfile, (sizeof(VpProfileDataType)*devProfileSize))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
		}
		pDevProfile = devProfile;
	}

	if(acProfileSize) {
		/* Get AC profile */
		if(copy_from_user(acProfile, (void*)data.pAcProfile, (sizeof(VpProfileDataType)*acProfileSize))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
		}
		pAcProfile = acProfile;
	}

	if(dcProfileSize) {
		/* Get DC profile */
		if(copy_from_user(dcProfile, (void*)data.pDcProfile, (sizeof(VpProfileDataType)*dcProfileSize))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
		}
		pDcProfile = dcProfile;
	}

	if(ringProfileSize) {
		/* Get ring profile */
		if(copy_from_user(ringProfile, (void*)data.pRingProfile, (sizeof(VpProfileDataType)*ringProfileSize))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
		}
		pRingProfile = ringProfile;
	}

	if(fxoAcProfileSize) {
		/* Get FXO AC profile */
		if(copy_from_user(fxoAcProfile, (void*)data.pFxoAcProfile, (sizeof(VpProfileDataType)*fxoAcProfileSize))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
		}
		pFxoAcProfile = fxoAcProfile;
	}

	if(fxoCfgProfileSize) {
		/* Get FXO configuration profile */
		if(copy_from_user(fxoCfgProfile, (void*)data.pFxoCfgProfile,
					 (sizeof(VpProfileDataType)*fxoCfgProfileSize))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
		}
		pFxoCfgProfile = fxoCfgProfile;
	}

	data.status = VpInitDevice(&pDevCtx[deviceId], pDevProfile, pAcProfile, pDcProfile, pRingProfile,
					 pFxoAcProfile, pFxoCfgProfile);

	if(data.status == VP_STATUS_SUCCESS) {
		total_devs++;
		REGISTER_DEVICE(deviceId);

		if(!atomic_read(&vpapi_init))
			atomic_set(&vpapi_init, 1);
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModInitDeviceType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_cal_line(unsigned long arg)
{
	VpApiModCalLnType data;
	VpLineIdType lineId;
	VpDeviceIdType deviceId;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModCalLnType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	lineId = GET_LINE(data.lineId);
	deviceId = GET_DEVICE(data.lineId);

	data.status = VpCalLine(&pLineCtx[deviceId][lineId]);

	if(data.status == VP_STATUS_SUCCESS) {
		total_lines++;
		REGISTER_LINE(data.lineId);
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModCalLnType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_set_line_state(unsigned long arg)
{
	VpApiModSetLnStType data;
	VpLineIdType lineId;
	VpDeviceIdType deviceId;
	VpLineStateType state;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModSetLnStType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	lineId = GET_LINE(data.lineId);
	deviceId = GET_DEVICE(data.lineId);
	state = data.state;

	data.status = VpSetLineState(&pLineCtx[deviceId][lineId], state);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModSetLnStType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_set_option(unsigned long arg)
{
	VpApiModSetOptionType data;
	u8 lineRequest;
	VpLineIdType lineId;
	VpDeviceIdType deviceId;
	VpOptionIdType option;
	void *pOptInfo;
	long size;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModSetOptionType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	option = data.option;
	lineRequest = data.lineRequest;
	deviceId = data.deviceId;
	lineId = GET_LINE(data.lineId);

	switch(option) {
		case VP_OPTION_ID_TIMESLOT:
			size = sizeof(VpOptionTimeslotType);
			pOptInfo = (VpOptionTimeslotType*)kmalloc(size, GFP_KERNEL);
			break;
		case VP_OPTION_ID_CODEC:
			size = sizeof(VpOptionCodecType);
			pOptInfo = (VpOptionCodecType*)kmalloc(size, GFP_KERNEL);
			break;
		case VP_OPTION_ID_LOOPBACK:
			size = sizeof(VpOptionLoopbackType);
			pOptInfo = (VpOptionLoopbackType*)kmalloc(size, GFP_KERNEL);
			break;
		case VP_OPTION_ID_EVENT_MASK:
			size = sizeof(VpOptionEventMaskType);
			pOptInfo = (VpOptionEventMaskType*)kmalloc(size, GFP_KERNEL);
			break;
		default:
			printk("%s: option(%d) not supported\n",__func__, option);
			return -EFAULT;
	}

	/* Get option info */
	if(copy_from_user(pOptInfo, (void*)data.pValue, size)) {
			printk("%s: copy_from_user failed\n", __func__);
			kfree(pOptInfo);
			return -EFAULT;
	}

	/* Set option to line/device */
	if(lineRequest)
		data.status = VpSetOption(&pLineCtx[deviceId][lineId], VP_NULL, option, pOptInfo);
	else
		data.status = VpSetOption(VP_NULL, &pDevCtx[deviceId], option, pOptInfo);

	kfree(pOptInfo);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModSetOptionType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

int vpapi_get_event(unsigned long arg)
{
	VpApiModGetEventType data;
	VpDeviceIdType deviceId;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpApiModGetEventType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	deviceId = data.deviceId;

	if(atomic_read(&event_count) == 0) {
		data.newEvent = FALSE;
	}
	else {
		/* Copy event info back to user */
		if(copy_to_user(data.pEvent, &event_queue[curr_event].vp_event, sizeof(VpEventType))) {
			printk("%s: copy_to_user failed\n", __func__);
			return  -EFAULT;
		}

		event_queue[curr_event].valid = 0;
		data.newEvent = TRUE;
		atomic_dec(&event_count);
		curr_event++;
		if(curr_event == MAX_EVENT_QUEUE_SIZE)
			curr_event = 0;
	}

	/* Copy status and event info back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpApiModGetEventType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}
#if defined(CONFIG_ZARLINK_SLIC_VE792)
static int vpapi_batt_on(unsigned long arg)
{
	VpModBatteryOnType data;
	int vbh, vbl, vbp;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpModBatteryOnType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}
	
	vbh = data.vbh;
	vbl = data.vbl;
	vbp = data.vbp;

	data.status = BattOn(vbh, vbl, vbp);

	/* Copy status and event info back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpModBatteryOnType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_batt_off(unsigned long arg)
{
	VpModBatteryOffType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpModBatteryOffType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}
	
	data.status = BattOff();

	/* Copy status and event info back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpModBatteryOffType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}
#endif

#if defined(CONFIG_ZARLINK_SLIC_VE880)
static int vpapi_reg_read(unsigned long arg)
{
	VpModRegOpType data;
	VpLineIdType	line_id;
	unsigned char	cmd;
	unsigned short  cmd_len;
	unsigned char *buff_p = NULL;
	unsigned char ec_val[] = {0x1, 0x2};

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpModRegOpType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	line_id = data.lineId;
	cmd = data.cmd;
	cmd_len = data.cmdLen;
	buff_p = data.buff;

	VpMpiCmd(GET_DEVICE(line_id), ec_val[GET_LINE(line_id)], (cmd | 1), cmd_len, buff_p);

	/* Copy status and event info back to user */
	if(copy_to_user((void*)arg, &data, sizeof(VpModRegOpType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int vpapi_reg_write(unsigned long arg)
{
	VpModRegOpType data;
	VpLineIdType	line_id;
	unsigned char	cmd;
	unsigned short  cmd_len;
	unsigned char *buff_p = NULL;
	unsigned char ec_val[] = {0x1, 0x2};

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(VpModRegOpType))) {
			printk("%s: copy_from_user failed\n", __func__);
			return -EFAULT;
	}

	line_id = data.lineId;
	cmd = data.cmd;
	cmd_len = data.cmdLen;
	buff_p = data.buff;

	VpMpiCmd(GET_DEVICE(line_id), ec_val[GET_LINE(line_id)], cmd, cmd_len, buff_p);

	return 0;
}
#endif

static int vpapi_open(struct inode *pInode, struct file *pFile)
{
	try_module_get(THIS_MODULE);
	return 0;
}

static int vpapi_release(struct inode *pInode, struct file *pFile)
{
	module_put(THIS_MODULE);
	return 0;
}

#if !defined(CONFIG_MV_TDM_SUPPORT) && defined(CONFIG_ZARLINK_SLIC_VE880) 
static irqreturn_t vpapi_slic_isr(int irq, void* dev_id)
{
	unsigned int gpp;
	unsigned int deviceId;

	/* Mark interrupt/s for active SLAC device/s */
	for(deviceId = 0; deviceId < MAX_DEVICES; deviceId++) {
		if(GET_DEV_STATUS(deviceId))
			VpVirtualISR(&pDevCtx[deviceId]);
	}
	//MV_REG_WRITE(MV_GPP_IRQ_CAUSE_REG(0), ~BIT23);

	/* ack interrupt */
	gpp = MV_REG_READ(GPP_DATA_IN_POL_REG(0));
        gpp = gpp^(1 << 23);
        MV_REG_WRITE(GPP_DATA_IN_POL_REG(0), gpp);
	
	return IRQ_HANDLED;
}
#endif

#if defined(SLIC_TIMER_EVENT_SUPPORT)
static void vpapi_tick_handler(unsigned long data)
{
	u8 deviceId;
	unsigned long flags;
	vpapi_event *pEvent;
#if !defined(CONFIG_ZARLINK_SLIC_VE792)
	MV_STD_BOOL eventStatus;
#endif

	/* Check if events are already active */
	if(atomic_read(&vpapi_init) == 0)
		goto timer_exit;

	spin_lock_irqsave(&vpapi_lock, flags);

	for(deviceId = 0; deviceId < MAX_DEVICES; deviceId++) {

		if(GET_DEV_STATUS(deviceId) == 0)
			continue;

		/* Check for free resources */
		if(atomic_read(&event_count) >= MAX_EVENT_QUEUE_SIZE)
			goto timer_exit;

#if !defined(CONFIG_ZARLINK_SLIC_VE792)
		if(VP_STATUS_SUCCESS == VpApiTick(&pDevCtx[deviceId], &eventStatus)) {
			if(eventStatus == TRUE) {
#endif
				pEvent = &event_queue[next_event];
				while(VpGetEvent(&pDevCtx[deviceId], &pEvent->vp_event) == TRUE) {
					if(pEvent->vp_event.status != VP_STATUS_SUCCESS) {
						printk("%s: bad status(%d)\n", __func__, pEvent->vp_event.status);
						break;
					}
					
					if(pEvent->vp_event.eventId == 0)  {
						printk("%s: warning, empty event\n", __func__);
						break;
					}
					
					next_event++;
					if(next_event == MAX_EVENT_QUEUE_SIZE) {
						next_event = 0;
					}

					atomic_inc(&event_count);
					
					if(pEvent->valid == 0) {
						pEvent->valid = 1;			
					}
					else {
						printk("%s: error, event(%u) was overrided\n", __func__, next_event);
						break;
					}

					pEvent = &event_queue[next_event];
				}
#if !defined(CONFIG_ZARLINK_SLIC_VE792)
			}
		}
#endif
	}

	spin_unlock_irqrestore(&vpapi_lock, flags);

timer_exit:

	/* Checks if user application should be signaled */
	if(atomic_read(&event_count) > 0) {
		wake_up_interruptible(&vpapi_wait);
	}

	/* Schedule next timer tick */
	vpapi_timer.expires = jiffies + VPAPI_TICK_TIMER_PERIOD;
    	add_timer(&vpapi_timer);
}
#endif

int __init vpapi_module_init(void)
{
	int status;

	printk("Loading Marvell %s device\n", VPAPI_MOD_NAME);
	status = misc_register(&vpapi_misc_dev);

	/* Register VPAPI device module */
	if (status < 0) {
		printk("Error, failed to load %s module(%d)\n", VPAPI_MOD_NAME, status);
		return status;
	}

	atomic_set(&vpapi_init, 0);
	total_devs = 0;
	total_lines = 0;
	next_event = 0;
	curr_event = 0;
	memset(vpapi_dev_status, 0, MAX_DEVICES);
	memset(vpapi_line_status, 0, MAX_LINES);

	/* Reset event counter */
	atomic_set(&event_count, 0);

	/* Clear event queue */
	memset(event_queue, 0, (MAX_EVENT_QUEUE_SIZE * sizeof(vpapi_event)));

#if defined(SLIC_TIMER_EVENT_SUPPORT)
	memset(&vpapi_timer, 0, sizeof(struct timer_list));
	init_timer(&vpapi_timer);
    	vpapi_timer.function = vpapi_tick_handler;
    	vpapi_timer.data = -1;
	vpapi_timer.expires = jiffies + VPAPI_TICK_TIMER_PERIOD;
	add_timer(&vpapi_timer);
#endif

#if !defined(CONFIG_MV_TDM_SUPPORT) && defined(CONFIG_ZARLINK_SLIC_VE880)
	/* Register SLIC interrupt */
	if (request_irq((23+IRQ_GPP_START), vpapi_slic_isr, IRQF_DISABLED, "slic", event_queue)) {
		printk("%s: Failed to connect irq(%d)\n", __func__, (23+IRQ_GPP_START));
		return MV_ERROR;
	}
#endif

	return 0;
}

void __exit vpapi_module_exit(void)
{
	printk("Unloading %s device module\n", VPAPI_MOD_NAME);

#if defined(SLIC_TIMER_EVENT_SUPPORT)
	del_timer(&vpapi_timer);
#endif

	/* Unregister VPAPI misc device */
	misc_deregister(&vpapi_misc_dev);

#if !defined(CONFIG_MV_TDM_SUPPORT) && defined(CONFIG_ZARLINK_SLIC_VE880)
	free_irq((23+IRQ_GPP_START), event_queue);
#endif

	return;
}

/* Module stuff */
module_init(vpapi_module_init);
module_exit(vpapi_module_exit);
MODULE_DESCRIPTION("Zarlink VPAPI-II Device");
MODULE_AUTHOR("Eran Ben-Avi <benavi@marvell.com>");
MODULE_LICENSE("GPL");

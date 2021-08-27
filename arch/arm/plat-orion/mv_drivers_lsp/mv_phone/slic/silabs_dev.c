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
#include "silabs_dev.h"
#include "spi.h"
#include "timer.h"
#include "boardEnv/mvBoardEnvLib.h"

/* SI3226 */
#if defined(CONFIG_SILAB_SLIC_SI3226x)
#define NUMBER_OF_DEVICES 1
#define NUMBER_OF_PROSLIC NUMBER_OF_DEVICES
#define CHAN_PER_DEVICE 2
#define NUMBER_OF_CHAN (NUMBER_OF_DEVICES*CHAN_PER_DEVICE)
#define PROSLIC_DEVICE_TYPE SI3226X_TYPE
#endif

/* SI3217 */
#if defined(CONFIG_SILAB_SLIC_SI3217x)
#define NUMBER_OF_DEVICES 1
#define NUMBER_OF_PROSLIC NUMBER_OF_DEVICES
#define CHAN_PER_DEVICE 1
#define NUMBER_OF_CHAN (NUMBER_OF_DEVICES*CHAN_PER_DEVICE)
#define PROSLIC_DEVICE_TYPE SI3217X_TYPE
#endif

/* Defines */
#define GET_DEV_STATUS(deviceNum)	silabs_dev_status[deviceNum]
#define GET_LINE_STATUS(chanNum)	silabs_line_status[chanNum]
#define REGISTER_DEVICE(deviceNum)	silabs_dev_status[deviceNum] = 1;
#define REGISTER_LINE(chanNum)		silabs_line_status[chanNum] = 1;
#define MAX_PROFILE_SIZE			128
#define GET_DEVICE(chanNum)			(chanNum / CHAN_PER_DEVICE)
#define GET_LINE(chanNum)			(chanNum % CHAN_PER_DEVICE)
#define MAX_EVENT_QUEUE_SIZE		256
#define SILABS_TICK_TIMER_PERIOD	1
#define SILABS_MOD_NAME				"silabs"
#define SLIC_TIMER_EVENT_SUPPORT

static void silabs_tick_handler(unsigned long data);
static ssize_t silabs_read(struct file *file, char __user *buf, size_t size, loff_t * ppos);
static ssize_t silabs_write(struct file *file, const char __user *buf, size_t size, loff_t * ppos);
static unsigned int silabs_poll(struct file *pFile, poll_table *pPollTable);
static int silabs_ioctl(struct inode *pInode, struct file *pFile, unsigned int cmd, unsigned long arg);
static int silabs_open(struct inode *pInode, struct file *pFile);
static int silabs_release(struct inode *pInode, struct file *pFile);
//static int __init silabs_module_init(void);
//static void __exit silabs_module_exit(void);

/* SI-API-II Dispatchers */
static int silabs_control_interface(unsigned long arg);
static int silabs_register_access(unsigned long arg);
static int silabs_device_init(unsigned long arg);
static int silabs_channel_all(unsigned long arg);
static int silabs_channel_init(unsigned long arg);
static int silabs_channel_setup(unsigned long arg);
static int silabs_channel_operation(unsigned long arg);
static int silabs_PCM_TS_setup(unsigned long arg);
static int silabs_get_event(unsigned long arg);
static int silabs_channel_set_line_feed(unsigned long arg);
static int silabs_channel_read_hook_status(unsigned long arg);
static int silabs_channel_set_loopback(unsigned long arg);

#if defined(CONFIG_SILAB_SLIC_SI3226x)
extern Si3226x_General_Cfg Si3226x_General_Configuration;
#endif
#if defined(CONFIG_SILAB_SLIC_SI3217x)
extern Si3217x_General_Cfg Si3217x_General_Configuration;
#endif

/* Enumurators */
typedef struct {
	unsigned char valid;		/* valid event */

	SiEventType si_event;
} silabs_event;

typedef struct _chanState {
	proslicChanType_ptr		ProObj;
} chanState;

/* Structs */
static struct file_operations silabs_fops = {
    owner:      THIS_MODULE,
    llseek:     NULL,
    read:       silabs_read,
    write:      silabs_write,
    poll:       silabs_poll,
    ioctl:      silabs_ioctl,
    open:       silabs_open,
    release:    silabs_release,
    fasync:     NULL
};

/* Globals */
static chanState ports[NUMBER_OF_CHAN];			/* User’s channel object, which has a member defined as proslicChanType_ptr ProObj */
static controlInterfaceType *ProHWIntf; 		/* Define ProSLIC control interface object */
static ProslicDeviceType *ProSLICDevices[NUMBER_OF_PROSLIC];	/* Define array of ProSLIC device objects */
static proslicChanType_ptr arrayOfProslicChans[NUMBER_OF_CHAN];
ctrl_S spiGciObj;

static DEFINE_SPINLOCK(silabs_lock);
static DECLARE_WAIT_QUEUE_HEAD(silabs_wait);
static atomic_t event_count;
static atomic_t silabs_init;
static atomic_t silabs_in_ioctl;
static silabs_event event_queue[MAX_EVENT_QUEUE_SIZE];
static u8 silabs_dev_status[NUMBER_OF_DEVICES];
static u8 silabs_line_status[NUMBER_OF_CHAN];
static volatile u32 next_event = 0, curr_event = 0;
static struct timer_list silabs_timer;
static u16 total_devs = 0, total_lines = 0;

static struct miscdevice silabs_misc_dev = {
	.minor = SLICDEV_MINOR,
	.name = SILABS_MOD_NAME,
	.fops = &silabs_fops,
};

static ssize_t silabs_read(struct file *file, char __user *buf, size_t size, loff_t * ppos)
{
	return 0;
}

static ssize_t silabs_write(struct file *file, const char __user *buf, size_t size, loff_t * ppos)
{
	return 0;
}

static unsigned int silabs_poll(struct file *pFile, poll_table *pPollTable)
{
	int mask = 0;

	poll_wait(pFile, &silabs_wait, pPollTable);

	if(atomic_read(&event_count) > 0) {
		mask |= POLLPRI;
	}

	return mask;
}

static int silabs_ioctl(struct inode *pInode, struct file *pFile, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	/* Argument checking */
	if (_IOC_TYPE(cmd) != SILABS_MOD_IOCTL_MAGIC) {
		printk("%s: invalid SILABS MOD Magic Num %i %i\n", __func__, _IOC_TYPE(cmd), SILABS_MOD_IOCTL_MAGIC);
		return -ENOTTY;
	}

	if ((_IOC_NR(cmd) > SILABS_MOD_IOCTL_MAX) || (_IOC_NR(cmd) < SILABS_MOD_IOCTL_MIN)) {
		printk("%s: invalid SILABS MOD IOCTL request\n", __func__);
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ) {
		ret = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
	}
	else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		ret = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
	}

	if (ret) {
		printk("%s: invalid SILABS MOD access type %i from cmd %i\n", __func__, _IOC_DIR(cmd), cmd);
		return -EFAULT;
	}

#if defined(SLIC_TIMER_EVENT_SUPPORT)
	/* Disable timer routine processing */
	atomic_set(&silabs_in_ioctl, 1);
#endif

	switch (cmd) {

		case SILABS_MOD_IOX_CTRL_IF:
			printk("ioctl: SILABS_MOD_IOX_CTRL_IF\n");
			ret = silabs_control_interface(arg);
			break;

		case SILABS_MOD_IOX_DEVICE_INIT:
			printk("ioctl: SILABS_MOD_IOX_DEVICE_INIT\n");
			ret = silabs_device_init(arg);
			break;

		case SILABS_MOD_IOX_CHAN_ALL:
			printk("ioctl: SILABS_MOD_IOX_CHAN_ALL\n");
			ret = silabs_channel_all(arg);
			break;

		case SILABS_MOD_IOX_CHAN_INIT:
			printk("ioctl: SILABS_MOD_IOX_CHAN_INIT\n");
			ret  = silabs_channel_init(arg);
			break;

		case SILABS_MOD_IOX_CHAN_SETUP:
			printk("ioctl: SILABS_MOD_IOX_CHAN_SETUP\n");
			ret = silabs_channel_setup(arg);
			break;

		case SILABS_MOD_IOX_CHAN_OP:
			printk("ioctl: SILABS_MOD_IOX_CHAN_OP\n");
			ret = silabs_channel_operation(arg);
			break;

		case SILABS_MOD_IOX_PCM_TS_SETUP:
			printk("ioctl: SILABS_MOD_IOX_PCM_TS_SETUP\n");
			ret = silabs_PCM_TS_setup(arg);
			break;

		case SILABS_MOD_IOX_GET_EVENT:
//			printk("ioctl: SILABS_MOD_IOX_GET_EVENT\n");
			ret = silabs_get_event(arg);
			break;

		case SILABS_MOD_IOX_REG_CTRL:
			printk("ioctl: SILABS_MOD_IOX_REG_CTRL\n");
			ret = silabs_register_access(arg);
			break;

		case SILABS_MOD_IOX_CHAN_LINE_FEED:
			printk("ioctl: SILABS_MOD_IOX_CHAN_LINE_FEED\n");
			ret = silabs_channel_set_line_feed(arg);
			break;

		case SILABS_MOD_IOX_CHAN_HOOK_STATUS:
			printk("ioctl: SILABS_MOD_IOX_CHAN_HOOK_STATUS\n");
			ret = silabs_channel_read_hook_status(arg);
			break;

		case SILABS_MOD_IOX_CHAN_LOOPBACK:
			printk("ioctl: SILABS_MOD_IOX_CHAN_LOOPBACK\n");
			ret = silabs_channel_set_loopback(arg);
			break;

		default:
			printk("%s: error, ioctl command(0x%x) not supported !!!\n", __func__, cmd);
			ret = -EFAULT;
			break;
	}

#if defined(SLIC_TIMER_EVENT_SUPPORT)
	/* Enable timer routine processing */
	atomic_set(&silabs_in_ioctl, 0);
#endif

	return ret;
}

static int silabs_register_access(unsigned long arg)
{
	SilabsRegObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsRegObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	switch (data.func) {
		case SI_REG_READ:
			data.value = ctrl_ReadRegisterWrapper (&spiGciObj, data.chanNum, data.regAddr);
			break;
		case SI_REG_WRITE:
			ctrl_WriteRegisterWrapper(&spiGciObj, data.chanNum, data.regAddr, data.value);
			break;
		case SI_RAM_READ:
			data.ramValue = ctrl_ReadRAMWrapper (&spiGciObj, data.chanNum, data.ramAddr);
			break;
		case SI_RAM_WRITE:
			ctrl_WriteRAMWrapper(&spiGciObj, data.chanNum, data.ramAddr, data.ramValue);
			break;
		default:
			printk("%s: error, function (0x%x) not supported !!!\n", __func__, data.func);
			return  -EFAULT;
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsRegObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_control_interface(unsigned long arg)
{
	SilabsModCtrlObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModCtrlObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	switch (data.func) {
		case SI_IF_CREATE:
#if defined(CONFIG_SILAB_SLIC_SI3226x)
			Si3226x_General_Configuration.irqen2 = 0x3;
#endif
#if defined(CONFIG_SILAB_SLIC_SI3217x)
			Si3217x_General_Configuration.irqen2 = 0x3;
#endif
			data.status = ProSLIC_createControlInterface (&ProHWIntf);
			break;
		case SI_IF_DESTROY:
			data.status = ProSLIC_destroyControlInterface (&ProHWIntf);
			break;
		case SI_IF_SET_FUNCS:
			ProSLIC_setControlInterfaceCtrlObj (ProHWIntf, &spiGciObj);
			ProSLIC_setControlInterfaceReset (ProHWIntf, ctrl_ResetWrapper);
			ProSLIC_setControlInterfaceWriteRegister (ProHWIntf, ctrl_WriteRegisterWrapper);
			ProSLIC_setControlInterfaceReadRegister (ProHWIntf, ctrl_ReadRegisterWrapper);
			ProSLIC_setControlInterfaceWriteRAM (ProHWIntf, ctrl_WriteRAMWrapper);
			ProSLIC_setControlInterfaceReadRAM (ProHWIntf, ctrl_ReadRAMWrapper);
			ProSLIC_setControlInterfaceTimerObj (ProHWIntf, NULL);
			ProSLIC_setControlInterfaceDelay (ProHWIntf, time_DelayWrapper);
			ProSLIC_setControlInterfaceTimeElapsed (ProHWIntf, NULL);
			ProSLIC_setControlInterfaceGetTime (ProHWIntf, NULL);
			ProSLIC_setControlInterfaceSemaphore (ProHWIntf, NULL);

			break;
		default:
			printk("%s: error, function (0x%x) not supported !!!\n", __func__, data.func);
			return  -EFAULT;
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModCtrlObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_device_init(unsigned long arg)
{
	SilabsModDevObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModDevObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	switch (data.func) {
		case SI_DEVICE_CREATE:
			data.status = ProSLIC_createDevice (&(ProSLICDevices[data.devNum]));
			total_devs++;
			REGISTER_DEVICE(data.devNum);

			break;
		case SI_DEVICE_DESTROY:
			data.status = ProSLIC_destroyDevice (&(ProSLICDevices[data.devNum]));
#if defined(SLIC_TIMER_EVENT_SUPPORT)
			if(total_lines == 0) {
				atomic_set(&silabs_init, 0);
				del_timer(&silabs_timer);
			}
#endif
			break;
		default:
			printk("%s: error, function (0x%x) not supported !!!\n", __func__, data.func);
			return  -EFAULT;
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModDevObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_channel_all(unsigned long arg)
{
	SilabsModChannelAllObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModChannelAllObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	switch (data.func) {
		case SI_CHANNEL_ALL_INIT:
			data.status = ProSLIC_Init(arrayOfProslicChans, NUMBER_OF_CHAN);
			break;
		case SI_CHANNEL_ALL_CAL:
			data.status = ProSLIC_Cal(arrayOfProslicChans, NUMBER_OF_CHAN);
			break;
		case SI_CHANNEL_ALL_LBCAL:
			data.status = ProSLIC_LBCal(arrayOfProslicChans, NUMBER_OF_CHAN);
			break;
		default:
			printk("%s: error, function (0x%x) not supported !!!\n", __func__, data.func);
			return  -EFAULT;
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModChannelAllObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;

}

static int silabs_channel_init(unsigned long arg)
{
	SilabsModChannelObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModChannelObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	switch (data.func) {
		case SI_CHANNEL_CREATE:
			data.status = ProSLIC_createChannel (&(ports[data.chanNum].ProObj));
			REGISTER_LINE(data.chanNum);
			break;
		case SI_CHANNEL_DESTROY:
			data.status = ProSLIC_destroyChannel (&(ports[data.chanNum].ProObj));
			break;
		case SI_CHANNEL_BC_INIT:
			data.status = ProSLIC_InitBroadcast (&(ports[data.chanNum].ProObj));
			break;
		case SI_CHANNEL_SW_INIT:
			data.status = ProSLIC_SWInitChan (ports[data.chanNum].ProObj, data.chanNum,
						PROSLIC_DEVICE_TYPE, ProSLICDevices[GET_DEVICE(data.chanNum)], ProHWIntf);

			arrayOfProslicChans[data.chanNum] = ports[data.chanNum].ProObj;
			ProSLIC_setSWDebugMode(ports[data.chanNum].ProObj,TRUE);  /* optional */
			break;
		default:
			printk("%s: error, function (0x%x) not supported !!!\n", __func__, data.func);
			return  -EFAULT;
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModChannelObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_channel_setup(unsigned long arg)
{
	SilabsModChannelSetupObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModChannelSetupObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	switch (data.func) {
		case SI_CHANNEL_SETUP_RING:
			data.status = ProSLIC_RingSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_TONEGEN:
			data.status = ProSLIC_ToneGenSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_FSK:
			data.status = ProSLIC_FSKSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_DTMF_DECODE:
			data.status = ProSLIC_DTMFDecodeSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_ZSYNTH:
			data.status = ProSLIC_ZsynthSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_GCICI:
			data.status = ProSLIC_GciCISetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_MODEM_DET:
			data.status = ProSLIC_ModemDetSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_TX_AUDIO_GAIN:
			data.status = ProSLIC_TXAudioGainSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_RX_AUDIO_GAIN:
			data.status = ProSLIC_RXAudioGainSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_DC_FEED:
			data.status = ProSLIC_DCFeedSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_PULSE_METER:
			data.status = ProSLIC_PulseMeterSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		case SI_CHANNEL_SETUP_PCM:
			data.status = ProSLIC_PCMSetup (ports[data.chanNum].ProObj, data.preset);
			break;
		default:
			printk("%s: error, function (0x%x) not supported !!!\n", __func__, data.func);
			return  -EFAULT;
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModChannelSetupObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_channel_operation(unsigned long arg)
{
	SilabsModChannelOpObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModChannelOpObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	switch (data.func) {
		case SI_CHANNEL_OP_RESET:
			data.status = ProSLIC_Reset (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_CLEAR_ERR:
			data.status = ProSLIC_clearErrorFlag (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_GPIO_SETUP:
			data.status = ProSLIC_GPIOSetup (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_VERIFY_CTRL_IF:
			data.status = ProSLIC_VerifyControlInterface (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_SHUT_DOWN_CHAN:
			data.status = ProSLIC_ShutdownChannel (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_RING_START:
			data.status = ProSLIC_RingStart (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_RING_STOP:
			data.status = ProSLIC_RingStop (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_ENA_CID:
			data.status = ProSLIC_EnableCID (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_DIS_CID:
			data.status = ProSLIC_DisableCID (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_PLL_FREE_RUN_START:
			data.status = ProSLIC_PLLFreeRunStart (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_PLL_FREE_RUN_STOP:
			data.status = ProSLIC_PLLFreeRunStop (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_PULSE_METER_START:
			data.status = ProSLIC_PulseMeterStart (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_PULSE_METER_STOP:
			data.status = ProSLIC_PulseMeterStop (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_PWR_UP_CONV:
			data.status = ProSLIC_PowerUpConverter (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_PWR_DOWN_CONV:
			data.status = ProSLIC_PowerDownConverter (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_ENA_INT:
			data.status = ProSLIC_EnableInterrupts (ports[data.chanNum].ProObj);

			if(!atomic_read(&silabs_init)) {
#if defined(SLIC_TIMER_EVENT_SUPPORT)
				memset(&silabs_timer, 0, sizeof(struct timer_list));
				init_timer(&silabs_timer);
				silabs_timer.function = silabs_tick_handler;
				silabs_timer.data = -1;
				silabs_timer.expires = jiffies + SILABS_TICK_TIMER_PERIOD;
				add_timer(&silabs_timer);
#endif
				atomic_set(&silabs_init, 1);
			}
			break;
		case SI_CHANNEL_OP_PCM_START:
			data.status = ProSLIC_PCMStart (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_PCM_STOP:
			data.status = ProSLIC_PCMStop (ports[data.chanNum].ProObj);
			break;
		case SI_CHANNEL_OP_TONE_GEN_START:
			data.status = ProSLIC_ToneGenStart (ports[data.chanNum].ProObj, 0);
			break;
		case SI_CHANNEL_OP_TONE_GEN_STOP:
			data.status = ProSLIC_ToneGenStop (ports[data.chanNum].ProObj);
			break;
		default:
			printk("%s: error, function (0x%x) not supported !!!\n", __func__, data.func);
			return  -EFAULT;
	}

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModChannelOpObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_PCM_TS_setup(unsigned long arg)
{
	SilabsModPCMTSSetupObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModPCMTSSetupObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	data.status = ProSLIC_PCMTimeSlotSetup(ports[data.chanNum].ProObj, data.rxcount, data.txcount);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModPCMTSSetupObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_channel_set_line_feed(unsigned long arg)
{
	SilabsModChannelLineFeedObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModChannelLineFeedObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	data.status = ProSLIC_SetLinefeedStatus(ports[data.chanNum].ProObj, data.newLineFeed);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModChannelLineFeedObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

int silabs_get_event(unsigned long arg)
{
	SilabsModGetEventType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModGetEventType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	if(atomic_read(&event_count) == 0) {
		data.newEvent = FALSE;
	}
	else {
		/* Copy event info back to user */
		if(copy_to_user(data.pEvent, &event_queue[curr_event].si_event, sizeof(SiEventType))) {
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
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModGetEventType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_channel_read_hook_status(unsigned long arg)
{
	SilabsModChannelReadHookStatObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModChannelReadHookStatObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	data.status = ProSLIC_ReadHookStatus (ports[data.chanNum].ProObj, &data.hookStatus);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModChannelReadHookStatObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_channel_set_loopback(unsigned long arg)
{
	SilabsModChannelSetLoopbackObjType data;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(SilabsModChannelSetLoopbackObjType))) {
		printk("%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	data.status = ProSLIC_SetLoopbackMode (ports[data.chanNum].ProObj, data.newMode);

	/* Copy status back to user */
	if(copy_to_user((void*)arg, &data, sizeof(SilabsModChannelSetLoopbackObjType))) {
		printk("%s: copy_to_user failed\n", __func__);
		return  -EFAULT;
	}

	return 0;
}

static int silabs_open(struct inode *pInode, struct file *pFile)
{
	try_module_get(THIS_MODULE);
	return 0;
}

static int silabs_release(struct inode *pInode, struct file *pFile)
{
	module_put(THIS_MODULE);
	return 0;
}

#if defined(SLIC_TIMER_EVENT_SUPPORT)
static void silabs_tick_handler(unsigned long data)
{
	u8 chanNum;
	unsigned long flags;

	silabs_event *pEvent;

	proslicIntType siEvent;
	ProslicInt irqs[MAX_PROSLIC_IRQS];

	/* Check if events are already active and not processing IOCTL */
	if ((atomic_read(&silabs_init) == 0) || (atomic_read(&silabs_in_ioctl) == 1))
		goto timer_exit;

	spin_lock_irqsave(&silabs_lock, flags);

	siEvent.irqs = irqs;

	for (chanNum = 0; chanNum < NUMBER_OF_CHAN; chanNum++) {

		if (GET_LINE_STATUS(chanNum) == 0)
			continue;

		siEvent.number = 0;

		/* Check for free resources */
		if (atomic_read(&event_count) >= MAX_EVENT_QUEUE_SIZE)
			goto timer_exit;

		if (ProSLIC_GetInterrupts(ports[chanNum].ProObj, &siEvent)) {

			printk("We got new %d events! \n", siEvent.number);

			pEvent = &event_queue[next_event];
			pEvent->si_event.chanNum = chanNum;
			pEvent->si_event.devNum = GET_DEVICE(chanNum);
			pEvent->si_event.eventsNum = siEvent.number;
			memcpy(pEvent->si_event.irqs, siEvent.irqs, sizeof(ProslicInt)*MAX_PROSLIC_IRQS);

			next_event++;

			if (next_event == MAX_EVENT_QUEUE_SIZE)
				next_event = 0;

			atomic_inc(&event_count);

			if (pEvent->valid == 0) {
				pEvent->valid = 1;
			} else {
				printk("%s: error, event(%u) was overrided\n", __func__, next_event);
				break;
			}
		}
	}

	spin_unlock_irqrestore(&silabs_lock, flags);

timer_exit:

	/* Checks if user application should be signaled */
	if (atomic_read(&event_count) > 0) {
		wake_up_interruptible(&silabs_wait);
	}

	/* Schedule next timer tick */
	silabs_timer.expires = jiffies + SILABS_TICK_TIMER_PERIOD;
	add_timer(&silabs_timer);
}
#endif

int __init silabs_module_init(void)
{
	int status = 0;

	printk("Loading Marvell %s device\n", SILABS_MOD_NAME);

	status = misc_register(&silabs_misc_dev);

	/* Register SILABS device module */
	if (status < 0) {
		printk("Error, failed to load %s module(%d)\n", SILABS_MOD_NAME, status);
		return status;
	}

	atomic_set(&silabs_init, 0);
	atomic_set(&silabs_in_ioctl, 0);
	total_devs = 0;
	total_lines = 0;
	next_event = 0;
	curr_event = 0;
	memset(silabs_dev_status, 0, NUMBER_OF_DEVICES);
	memset(silabs_line_status, 0, NUMBER_OF_CHAN);

	/* Reset event counter */
	atomic_set(&event_count, 0);

	/* Clear event queue */
	memset(event_queue, 0, (MAX_EVENT_QUEUE_SIZE * sizeof(silabs_event)));

	spiGciObj.portID = mvBoardTdmSpiIdGet();
	SPI_Init (&spiGciObj);

	return 0;
}

void __exit silabs_module_exit(void)
{
	printk("Unloading %s device module\n", SILABS_MOD_NAME);

#if defined(SLIC_TIMER_EVENT_SUPPORT)
	if(total_lines == 0)
		del_timer(&silabs_timer);
#endif

	/* Unregister SILABS misc device */
	misc_deregister(&silabs_misc_dev);

	return;
}

/* Module stuff */
module_init(silabs_module_init);
module_exit(silabs_module_exit);
MODULE_DESCRIPTION("Silicon Labs Proslic Device");
MODULE_AUTHOR("Nadav Haklai <nadavh@marvell.com>");
MODULE_LICENSE("GPL");

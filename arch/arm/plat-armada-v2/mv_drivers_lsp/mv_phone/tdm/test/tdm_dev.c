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

#include "mvSysHwConfig.h"
#include "mvSysTdmConfig.h"
#include "tdm_dev.h"
#include "mv_phone/tdm/tdm_if.h"
#include "mv_phone/tdm/tal.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/miscdevice.h>
#ifdef CONFIG_MV_TDM_SUPPORT
 #include "voiceband/tdm/mvTdm.h"
#else
 #include "voiceband/commUnit/mvCommUnit.h"
#endif

#define TDM_DEV_NAME 	"tdm"
#define DISABLE		0
#define ENABLE		1
#define MAX_POLL	12 /* millisecond */

#ifdef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
#include "ctrlEnv/mvCtrlEnvLib.h"
#include <asm/hardware/gic.h>
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */

/* Globals */
static DEFINE_MUTEX(tdm_ioctl_mutex);
static DECLARE_WAIT_QUEUE_HEAD(tdm_dev_wait);
static DEFINE_SPINLOCK(tdm_dev_lock);
static tal_params_t tdm_dev_params;
static tal_mmp_ops_t tdm_dev_ops;
static atomic_t tdm_init, rx_ready, tx_ready;
static unsigned char *rx_buff_p = NULL, *tx_buff_p = NULL;
static unsigned char rx_temp_buff[MV_TDM_TOTAL_CHANNELS * MV_TDM_TOTAL_CH_SAMPLES * 4];
#if defined(MV_TDM_USE_DCO)
static tdm_dev_clk_t tdm_dev_clk;
#endif

/* Forward declarations */
static int tdm_dev_tdm_start(unsigned long arg);
static ssize_t tdm_dev_read(struct file *file_p, char __user *buf, size_t size, loff_t * ppos);
static ssize_t tdm_dev_write(struct file *file_p, const char __user *buf, size_t size, loff_t * ppos);
static unsigned int tdm_dev_poll(struct file *file_p, poll_table *poll_table_p);
static long tdm_dev_ioctl(struct file *file_p, unsigned int cmd, unsigned long arg);
static int tdm_dev_open(struct inode *inode_p, struct file *file_p);
static int tdm_dev_release(struct inode *inode_p, struct file *file_p);
void tdm_dev_tx_callback(unsigned char* tx_buff, int size);
void tdm_dev_rx_callback(unsigned char* rx_buff, int size);
static int __init tdm_dev_init(void);
static void __exit tdm_dev_exit(void);

static const struct file_operations tdm_dev_fops = {
	.owner		= THIS_MODULE,
	.llseek		= NULL,
	.read		= tdm_dev_read,
	.write		= tdm_dev_write,
	.poll		= tdm_dev_poll,
	.unlocked_ioctl	= tdm_dev_ioctl,
	.open		= tdm_dev_open,
	.release	= tdm_dev_release,
	.fasync		= NULL
};

static struct miscdevice tdm_dev_misc_dev = {
	.minor = TDMDEV_MINOR,
	.name = TDM_DEV_NAME,
	.fops = &tdm_dev_fops,
};

static int __init tdm_dev_init(void)
{
	int status;

	printk("Loading Marvell tdm device\n");
	status = misc_register(&tdm_dev_misc_dev);

	/* Register tdm device */
	if (status < 0) {
		printk("Error, failed to load %s device(status %d)\n", TDM_DEV_NAME, status);
		return status;
	}

	atomic_set(&tdm_init, DISABLE);

	return 0;
}

static int tdm_dev_tdm_start(unsigned long arg)
{
	tdm_dev_params_t data;
	int i;

	/* Get user data */
	if(copy_from_user(&data, (void*)arg, sizeof(tdm_dev_params_t))) {
		printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
		return -EFAULT;
	}

	/* Check parameters */
	if((data.pcm_format != 1) && (data.pcm_format != 2) && (data.pcm_format != 4)) {
		printk(KERN_ERR "%s: bad parameter(pcm_format=%u)\n", __func__, data.pcm_format);
		return -EFAULT;
	}

	if(data.total_lines > MV_TDM_TOTAL_CHANNELS) {
		printk(KERN_ERR "%s: bad parameter(data.total_lines=%u)\n", __func__, data.total_lines);
		return -EFAULT;
	}

	tdm_dev_params.pcm_format = (tal_pcm_format_t)data.pcm_format;
	/* Fill time slot table */
	for(i = 0; i < data.total_lines; i++)
		tdm_dev_params.pcm_slot[i] = ((i+1) * data.pcm_format); /* skip time slot #0 */

	tdm_dev_params.sampling_period = MV_TDM_BASE_SAMPLING_PERIOD;
	tdm_dev_params.total_lines = data.total_lines;
	tdm_dev_params.test_enable = 1;

	/* Assign Rx/Tx callbacks */
	tdm_dev_ops.tal_mmp_rx_callback = tdm_dev_rx_callback;
	tdm_dev_ops.tal_mmp_tx_callback = tdm_dev_tx_callback;

	if(tal_init(&tdm_dev_params, &tdm_dev_ops) != MV_OK) {
		printk(KERN_ERR "%s: Error, could not init tdm driver\n", __func__);
		return -EFAULT;
	}

	/* Prepare globals */
	atomic_set(&tdm_init, ENABLE);
	atomic_set(&rx_ready, DISABLE);
	atomic_set(&tx_ready, DISABLE);
	rx_buff_p = NULL;
	tx_buff_p = NULL;

	return 0;
}

static ssize_t tdm_dev_read(struct file *file_p, char __user *buf, size_t size, loff_t * ppos)
{
	size_t ret = size;

	TRC_REC("->%s\n", __func__);

#ifdef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
	gic_mask_fiq(mvCtrlTdmUnitIrqGet());
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */
	if(rx_buff_p != NULL) {
		if (copy_to_user(buf, rx_buff_p, size))
			ret = -EFAULT;
		rx_buff_p = NULL;
		atomic_set(&rx_ready, DISABLE);
	} else {
		ret = 0;
		TRC_REC("%s: missed Rx buffer\n", __func__);
	}

	TRC_REC("<-%s\n", __func__);

#ifdef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
	gic_unmask_fiq(mvCtrlTdmUnitIrqGet());
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */
	return ret;
}

static ssize_t tdm_dev_write(struct file *file_p, const char __user *buf, size_t size, loff_t * ppos)
{
	unsigned long flags = 0;
	MV_STATUS status;
	size_t ret = size;

	TRC_REC("->%s\n", __func__);

#ifdef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
	gic_mask_fiq(mvCtrlTdmUnitIrqGet());
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */
	if(tx_buff_p != NULL) {
		if (copy_from_user(tx_buff_p, buf, size))
			ret = -EFAULT;
		atomic_set(&tx_ready, DISABLE);
		spin_lock_irqsave(&tdm_dev_lock, flags);
#ifdef CONFIG_MV_TDM_SUPPORT
		status = mvTdmTx(tx_buff_p);
#else
		status = mvCommUnitTx(tx_buff_p);
#endif
		spin_unlock_irqrestore(&tdm_dev_lock, flags);
		tx_buff_p = NULL;
		if(status != MV_OK)
			printk(KERN_WARNING "%s: could not fill Tx buffer\n", __func__);
	} else {
		ret = 0;
		TRC_REC("%s: missed Tx buffer\n", __func__);
	}
	TRC_REC("<-%s\n", __func__);

#ifdef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
	gic_unmask_fiq(mvCtrlTdmUnitIrqGet());
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */
	return ret;
}

static unsigned int tdm_dev_poll(struct file *file_p, poll_table *poll_table_p)
{
	int mask = 0;

#ifdef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
	int max_poll = 0;

	while (!atomic_read(&rx_ready) && !atomic_read(&tx_ready) && (max_poll < MAX_POLL)) {
		mdelay(1);
		max_poll++;
	}

	if (max_poll >= MAX_POLL)
		printk(KERN_WARNING "%s: Warning, no Rx/Tx callback for at least 10ms\n", __func__);

	if (atomic_read(&rx_ready)) {
		atomic_set(&rx_ready, DISABLE);
		mask |= POLLIN | POLLRDNORM;	/* readable */
		TRC_REC("poll can read\n");
	}

	if (atomic_read(&tx_ready)) {
		atomic_set(&tx_ready, DISABLE);
		mask |= POLLOUT | POLLWRNORM;	/* writable */
		TRC_REC("poll can write\n");
	}
#else /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */
	TRC_REC("->%s\n", __func__);

	poll_wait(file_p, &tdm_dev_wait, poll_table_p);

	if(atomic_read(&rx_ready)) {
		mask |= POLLIN | POLLRDNORM;	/* readable */
		TRC_REC("poll can read\n");
	}

	if(atomic_read(&tx_ready)) {
		mask |= POLLOUT | POLLWRNORM;	/* writable */
		TRC_REC("poll can write\n");
	}

	TRC_REC("<-%s\n", __func__);
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */

	return mask;
}

static long tdm_dev_ioctl(struct file *file_p, unsigned int cmd, unsigned long arg)
{
	int ret = 0;

	mutex_lock(&tdm_ioctl_mutex);

	/* Argument checking */
	if (_IOC_TYPE(cmd) != TDM_DEV_IOCTL_MAGIC) {
		printk(KERN_ERR "%s: invalid TDM DEV Magic Num %i %i\n", __func__, _IOC_TYPE(cmd), TDM_DEV_IOCTL_MAGIC);
		mutex_unlock(&tdm_ioctl_mutex);
		return -ENOTTY;
	}

	if ((_IOC_NR(cmd) > TDM_DEV_IOCTL_MAX) || (_IOC_NR(cmd) < TDM_DEV_IOCTL_MIN)) {
		printk(KERN_ERR "%s: invalid TDM DEV IOCTL request\n", __func__);
		mutex_unlock(&tdm_ioctl_mutex);
		return -ENOTTY;
	}

	if (_IOC_DIR(cmd) & _IOC_READ) {
		ret = !access_ok(VERIFY_WRITE, (void __user*)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		ret = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
	}

	if (ret) {
		printk(KERN_ERR "%s: invalid TDM DEV access type %i from cmd %i\n", __func__, _IOC_DIR(cmd), cmd);
		mutex_unlock(&tdm_ioctl_mutex);
		return -EFAULT;
	}

	switch (cmd) {
		case TDM_DEV_TDM_START:
			printk("ioctl: TDM_DEV_TDM_START\n");
			ret = tdm_dev_tdm_start(arg);
			break;

		case TDM_DEV_TDM_STOP:
			atomic_set(&tdm_init, DISABLE);
			tal_exit();
			printk("ioctl: TDM_DEV_TDM_STOP\n");
			break;

		case TDM_DEV_PCM_START:
			printk("ioctl: TDM_DEV_PCM_START\n");
			atomic_set(&rx_ready, DISABLE);
			atomic_set(&tx_ready, DISABLE);
			rx_buff_p = NULL;
			tx_buff_p = NULL;
			tal_pcm_start();
			break;

		case TDM_DEV_PCM_STOP:
			tal_pcm_stop();
			printk("ioctl: TDM_DEV_PCM_STOP\n");
			break;

#if defined(MV_TDM_USE_DCO)
		case TDM_DEV_TDM_CLK_CONFIG:
			mvCtrlTdmClkCtrlConfig();
			printk(KERN_INFO "ioctl: TDM_DEV_TDM_CLK_CONFIG\n");
			break;

		case TDM_DEV_TDM_CLK_GET:
			tdm_dev_clk.correction = mvCtrlTdmClkCtrlGet();
			printk(KERN_INFO "ioctl: TDM_DEV_TDM_CLK_GET: %x\n", tdm_dev_clk.correction);
			/* Put user data */
			if (copy_to_user((void *)arg, &tdm_dev_clk, sizeof(tdm_dev_clk_t))) {
				printk(KERN_ERR "%s: copy_to_user failed\n", __func__);
				mutex_unlock(&tdm_ioctl_mutex);
				return -EFAULT;
			}
			printk(KERN_INFO "ioctl: TDM_DEV_TDM_CLK_GET\n");
			break;

		case TDM_DEV_TDM_CLK_SET:
			/* Get user data */
			if (copy_from_user(&tdm_dev_clk, (void *)arg, sizeof(tdm_dev_clk_t))) {
				printk(KERN_ERR "%s: copy_from_user failed\n", __func__);
				mutex_unlock(&tdm_ioctl_mutex);
				return -EFAULT;
			}
			printk(KERN_INFO "ioctl: TDM_DEV_TDM_CLK_SET: %x\n", tdm_dev_clk.correction);
			mvCtrlTdmClkCtrlSet(tdm_dev_clk.correction);
			break;
#endif
	}

	mutex_unlock(&tdm_ioctl_mutex);
	return (long)ret;
}
static int tdm_dev_open(struct inode *inode_p, struct file *file_p)
{
	try_module_get(THIS_MODULE);
	return 0;
}

static int tdm_dev_release(struct inode *inode_p, struct file *file_p)
{
	module_put(THIS_MODULE);
	return 0;
}

void tdm_dev_tx_callback(unsigned char* tx_buff, int size)
{
	TRC_REC("->%s\n", __func__);

	tx_buff_p = tx_buff;
	atomic_set(&tx_ready, ENABLE);
#ifndef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
	wake_up_interruptible(&tdm_dev_wait);
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */

	TRC_REC("<-%s\n", __func__);
	return;
}

void tdm_dev_rx_callback(unsigned char* rx_buff, int size)
{
	TRC_REC("->%s\n", __func__);

	rx_buff_p = rx_temp_buff;
	memcpy(rx_buff_p, rx_buff, size);
	atomic_set(&rx_ready, ENABLE);
#ifndef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
	wake_up_interruptible(&tdm_dev_wait);
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */

	TRC_REC("<-%s\n", __func__);
	return;
}

static void __exit tdm_dev_exit(void)
{
	printk("Marvell telephony test device exits\n");

	/* Stop TDM channels and release all resources */
	tal_exit();

	/* Unregister tdm misc device */
	misc_deregister(&tdm_dev_misc_dev);
}

/* Module stuff */
module_init(tdm_dev_init);
module_exit(tdm_dev_exit);
MODULE_DESCRIPTION("Marvell Telephony Test Device - www.marvell.com");
MODULE_AUTHOR("Eran Ben-Avi <benavi@marvell.com>");
MODULE_LICENSE("GPL");

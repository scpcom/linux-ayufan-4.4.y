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

#include "tdm_if.h"
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <plat/drv_dxt_if.h>
#ifndef CONFIG_MV_TDM_SUPPORT
#include "gpp/mvGppRegs.h"
#endif

/* TDM Interrupt Service Routine */
static irqreturn_t tdm_if_isr(int irq, void* dev_id);

/* PCM start/stop */
static void tdm_if_pcm_start(void);
static void tdm_if_pcm_stop(void);

/* Rx/Tx Tasklets  */
#if !(defined CONFIG_MV_PHONE_USE_IRQ_PROCESSING) && !(defined CONFIG_MV_PHONE_USE_FIQ_PROCESSING)
static void tdm_if_pcm_rx_process(unsigned long arg);
static void tdm_if_pcm_tx_process(unsigned long arg);
#else
static inline void tdm_if_pcm_rx_process(void);
static inline void tdm_if_pcm_tx_process(void);
#endif
/* TDM proc-fs statistics */
static int proc_tdm_init_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr);
static int proc_rx_miss_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr);
static int proc_tx_miss_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr);
static int proc_rx_over_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr);
static int proc_tx_under_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr);
#ifdef CONFIG_MV_TDM_EXT_STATS
static int proc_dump_ext_stats(char *buffer, char **buffer_location, off_t offset,
				int buffer_length, int *zero, void *ptr);
#endif

#ifdef CONFIG_MV_TDM_SUPPORT
/* TDM SW Reset */
static void tdm_if_stop_channels(unsigned long args);
#endif

/* Module */
static int __init tdm_if_module_init(void);
static void __exit tdm_if_module_exit(void);

/* Globals */
static tdm_if_register_ops_t* tdm_if_register_ops;
#if !(defined CONFIG_MV_PHONE_USE_IRQ_PROCESSING) && !(defined CONFIG_MV_PHONE_USE_FIQ_PROCESSING)
static DECLARE_TASKLET(tdm_if_rx_tasklet, tdm_if_pcm_rx_process, 0);
static DECLARE_TASKLET(tdm_if_tx_tasklet, tdm_if_pcm_tx_process, 0);
#endif
#ifdef CONFIG_MV_TDM_SUPPORT
static DECLARE_TASKLET(tdm_if_stop_tasklet, tdm_if_stop_channels, 0);
#endif
static DEFINE_SPINLOCK(tdm_if_lock);
static unsigned char *rxBuff = NULL, *txBuff = NULL;
static char irqnr;
static unsigned int rx_miss = 0, tx_miss = 0;
static unsigned int rx_over = 0, tx_under = 0;
static struct proc_dir_entry *tdm_stats;
static int pcm_enable = 0;
static int irq_init = 0;
static int tdm_init = 0;
static int buff_size = 0;
static unsigned short test_enable = 0;
#ifdef CONFIG_MV_TDM_EXT_STATS
static unsigned int pcm_stop_fail;
#endif
#ifdef CONFIG_MV_TDM_SUPPORT
static int pcm_stop_flag;
static int pcm_stop_status;
static unsigned int pcm_start_stop_state;
static unsigned int is_pcm_stopping;
#endif

static int proc_tdm_init_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr)
{
	return sprintf(buffer, "%u\n", tdm_init);
}

static int proc_rx_miss_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr)
{
	return sprintf(buffer, "%u\n", rx_miss);
}

static int proc_tx_miss_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr)
{
	return sprintf(buffer, "%u\n", tx_miss);
}

static int proc_rx_over_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr)
{
	return sprintf(buffer, "%u\n", rx_over);
}

static int proc_tx_under_read(char *buffer, char **buffer_location, off_t offset,
                            int buffer_length, int *zero, void *ptr)
{
	return sprintf(buffer, "%u\n", tx_under);
}

#ifdef CONFIG_MV_TDM_EXT_STATS
static int proc_dump_ext_stats(char *buffer, char **buffer_location, off_t offset,
				int buffer_length, int *zero, void *ptr)
{
	char *str;
	MV_TDM_EXTENDED_STATS tdm_ext_stats;

	if (offset > 0)
		return 0;

	mvTdmExtStatsGet(&tdm_ext_stats);

	str = buffer;
	str += sprintf(str, "\nTDM Extended Statistics:\n");
	str += sprintf(str, "intRxCount = %u\n", tdm_ext_stats.intRxCount);
	str += sprintf(str, "intTxCount = %u\n", tdm_ext_stats.intTxCount);
	str += sprintf(str, "intRx0Count = %u\n", tdm_ext_stats.intRx0Count);
	str += sprintf(str, "intTx0Count = %u\n", tdm_ext_stats.intTx0Count);
	str += sprintf(str, "intRx1Count = %u\n", tdm_ext_stats.intRx1Count);
	str += sprintf(str, "intTx1Count = %u\n", tdm_ext_stats.intTx1Count);
	str += sprintf(str, "intRx0Miss = %u\n", tdm_ext_stats.intRx0Miss);
	str += sprintf(str, "intTx0Miss	= %u\n", tdm_ext_stats.intTx0Miss);
	str += sprintf(str, "intRx1Miss	= %u\n", tdm_ext_stats.intRx1Miss);
	str += sprintf(str, "intTx1Miss	= %u\n", tdm_ext_stats.intTx1Miss);
	str += sprintf(str, "pcmRestartCount = %u\n", tdm_ext_stats.pcmRestartCount);
	str += sprintf(str, "pcm_stop_fail = %u\n", pcm_stop_fail);

	return (int)(str - buffer);
}
#endif

MV_STATUS tdm_if_init(tdm_if_register_ops_t* register_ops, tdm_if_params_t* tdm_if_params)
{
	MV_TDM_PARAMS tdm_params;

	if (tdm_init) {
		printk(KERN_INFO "Marvell Telephony Driver already started...\n");
		return MV_OK;
	}

	printk(KERN_INFO "Loading Marvell Telephony Driver\n");

	/* Check if any SLIC module exists */
	if (mvCtrlSocUnitInfoNumGet(TDM_UNIT_ID) == 0) {
		mvCtrlPwrClckSet(TDM_UNIT_ID, 0, MV_FALSE);
		printk(KERN_WARNING "%s: Warning, no SLIC module is connected\n", __func__);
		return MV_OK;
	}

	if ((register_ops == NULL) || (tdm_if_params == NULL)) {
		printk(KERN_ERR "%s: bad parameters\n", __func__);
		return MV_ERROR;

	}

	/* Check callbacks */
	if (register_ops->tdm_if_pcm_ops.pcm_tx_callback == NULL ||
		register_ops->tdm_if_pcm_ops.pcm_rx_callback == NULL) {
		printk(KERN_ERR "%s: missing parameters\n", __func__);
		return MV_ERROR;
	}

	/* Reset globals */
	rxBuff = txBuff = NULL;
	irq_init = 0;
	tdm_init = 0;

#ifdef CONFIG_MV_TDM_SUPPORT
	pcm_enable = 0;
	is_pcm_stopping = 0;
	pcm_stop_flag = 0;
	pcm_stop_status = 0;
#else
	pcm_enable = 1;
#endif

#ifdef CONFIG_MV_TDM_EXT_STATS
	pcm_stop_fail = 0;
#endif

	/* Extract test enable */
	test_enable = tdm_if_params->test_enable;

	/* Calculate Rx/Tx buffer size(use in callbacks) */
	buff_size = (tdm_if_params->pcm_format * tdm_if_params->total_lines * 80 *
			(tdm_if_params->sampling_period/MV_TDM_BASE_SAMPLING_PERIOD));

	/* Extract TDM irq number */
	irqnr = mvCtrlTdmUnitIrqGet();

	/* Enable Marvell tracing */
	TRC_START();
	TRC_INIT();
	TRC_REC("->%s\n", __func__);

	/* Assign TDM parameters */
	memcpy(&tdm_params, tdm_if_params, sizeof(MV_TDM_PARAMS));

	/* Assign control callbacks */
	tdm_if_register_ops = register_ops;
	tdm_if_register_ops->tdm_if_ctl_ops.ctl_pcm_start = tdm_if_pcm_start;
	tdm_if_register_ops->tdm_if_ctl_ops.ctl_pcm_stop = tdm_if_pcm_stop;

#ifdef CONFIG_MV_TDM_SUPPORT
	/* Soft reset to PCM I/F */
	mvTdmPcmIfReset();
#endif

	/* TDM init */
	if (mvSysTdmInit(&tdm_params) != MV_OK) {
			printk(KERN_ERR "%s: Error, TDM initialization failed !!!\n", __func__);
			return MV_ERROR;
	}
	tdm_init = 1;

	/* Register TDM interrupt */
#ifdef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
	if (request_fiq(irqnr, tdm_if_isr, IRQF_DISABLED, "tdm", NULL)) {
		printk(KERN_ERR "%s: Failed to connect fiq(%d)\n", __func__, irqnr);
		return MV_ERROR;
	}
#else /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */
	if (request_irq(irqnr, tdm_if_isr, IRQF_DISABLED, "tdm", NULL)) {
		printk(KERN_ERR "%s: Failed to connect irq(%d)\n", __func__, irqnr);
		return MV_ERROR;
	}
#endif /* CONFIG_MV_PHONE_USE_FIQ_PROCESSING */

	irq_init = 1;

	/* Create TDM procFS statistics */
	tdm_stats = proc_mkdir("tdm", NULL);
	if (tdm_stats != NULL) {
		create_proc_read_entry("tdm_init", 0, tdm_stats, proc_tdm_init_read, NULL);
		create_proc_read_entry("rx_miss", 0, tdm_stats, proc_rx_miss_read, NULL);
		create_proc_read_entry("tx_miss", 0, tdm_stats, proc_tx_miss_read, NULL);
		create_proc_read_entry("rx_over", 0, tdm_stats, proc_rx_over_read, NULL);
		create_proc_read_entry("tx_under", 0, tdm_stats, proc_tx_under_read, NULL);
#ifdef CONFIG_MV_TDM_EXT_STATS
		create_proc_read_entry("tdm_extended_stats", 0, tdm_stats, proc_dump_ext_stats, NULL);
#endif
	}

	TRC_REC("Marvell Telephony Driver Loaded Successfully\n");

#ifdef CONFIG_MV_COMM_UNIT_SUPPORT
	/* WA to stop the MCDMA gracefully after commUnit initialization */
	tdm_if_pcm_stop();
#endif

	TRC_REC("<-%s\n", __func__);
	return MV_OK;
}

void tdm_if_exit(void)
{
	u32 max_poll = 0;

	/* Check if already stopped */
	if (!irq_init && !pcm_enable && !tdm_init)
		return;

	TRC_REC("->%s\n", __func__);

	/* Stop PCM channels */
	if (pcm_enable)
		tdm_if_pcm_stop();

	while ((is_pcm_stopping != 0) && (max_poll < 20)) {
		mdelay(1);
		max_poll++;
	}

	if (max_poll >= 20) {
		printk(KERN_WARNING "%s: waiting for pcm channels to stop exceeded 20ms\n", __func__);
		/*printk("is_pcm_stopping(%d), pcm_enable(%d)\n", is_pcm_stopping, pcm_enable);
		printk("pcm_stop_flag(%d), pcm_start_stop_state(%d)\n", pcm_stop_flag, pcm_start_stop_state);
		printk("pcm_stop_status(%d)\n", pcm_stop_status);*/
	}

	if (irq_init) {
		/* Release interrupt */
#ifndef CONFIG_MV_PHONE_USE_FIQ_PROCESSING
		free_irq(irqnr, NULL);
#else /* !CONFIG_MV_PHONE_USE_FIQ_PROCESSING */
		free_fiq(irqnr, NULL);
#endif /* !CONFIG_MV_PHONE_USE_FIQ_PROCESSING */
		irq_init = 0;
	}

	if (tdm_init) {
#ifdef CONFIG_MV_TDM_SUPPORT
		mvTdmRelease();
#else
		mvCommUnitRelease();
#endif
		/* Remove proc directory & entries */
		remove_proc_entry("tdm_init", tdm_stats);
		remove_proc_entry("rx_miss", tdm_stats);
		remove_proc_entry("tx_miss", tdm_stats);
		remove_proc_entry("rx_over", tdm_stats);
		remove_proc_entry("tx_under", tdm_stats);
#ifdef CONFIG_MV_TDM_EXT_STATS
		remove_proc_entry("tdm_extended_stats", tdm_stats);
#endif
		remove_proc_entry("tdm", NULL);

		tdm_init = 0;
	}

	TRC_REC("<-%s\n", __func__);

	/* Dump output and release Marvell trace resources */
	TRC_OUTPUT(0, 1);
	TRC_RELEASE();
}

static void tdm_if_pcm_start(void)
{
	unsigned long flags;

	TRC_REC("->%s\n", __func__);

	spin_lock_irqsave(&tdm_if_lock, flags);
	if (!pcm_enable) {
		pcm_enable = 1;
#ifdef CONFIG_MV_TDM_SUPPORT
		if (is_pcm_stopping == 0) {
			pcm_stop_flag = 0;
			pcm_stop_status = 0;
			pcm_start_stop_state = 0;
			rxBuff = txBuff = NULL;
			mvTdmPcmStart();
		} else {
			pcm_start_stop_state++;
			TRC_REC("pcm_start_stop_state(%d)\n", pcm_start_stop_state);
		}
#else
		rxBuff = txBuff = NULL;
		mvCommUnitPcmStart();
#endif
	}
	spin_unlock_irqrestore(&tdm_if_lock, flags);

	TRC_REC("<-%s\n", __func__);
	return;
}

static void tdm_if_pcm_stop(void)
{
	unsigned long flags;

	TRC_REC("->%s\n", __func__);

	spin_lock_irqsave(&tdm_if_lock, flags);
	if (pcm_enable) {
		pcm_enable = 0;
#ifdef CONFIG_MV_TDM_SUPPORT
		if (is_pcm_stopping == 0) {
			is_pcm_stopping = 1;
			mvTdmPcmStop();
		} else {
			pcm_start_stop_state--;
			TRC_REC("pcm_start_stop_state(%d)\n", pcm_start_stop_state);
		}
#else
		mvCommUnitPcmStop();
#endif
	}
	spin_unlock_irqrestore(&tdm_if_lock, flags);

	TRC_REC("<-%s\n", __func__);
	return;
}

static irqreturn_t tdm_if_isr(int irq, void* dev_id)
{
	MV_TDM_INT_INFO tdm_int_info;
	unsigned int int_type;
#ifdef CONFIG_MV_TDM_SUPPORT
	int ret;
#endif

	TRC_REC("->%s\n", __func__);

	/* Extract interrupt information from low level ISR */
#ifdef CONFIG_MV_TDM_SUPPORT
	ret = mvTdmIntLow(&tdm_int_info);
#else
	mvCommUnitIntLow(&tdm_int_info);
#endif

	int_type = tdm_int_info.intType;
	/*device_id = tdm_int_info.cs;*/

	/* Nothing to do - return */
	if (int_type == MV_EMPTY_INT)
		goto out;

#ifdef CONFIG_MV_TDM_SUPPORT
	if ((ret == -1) && (pcm_stop_status == 0))  {
		pcm_stop_status = 1;

		/* If Rx/Tx tasklets already scheduled, let them do the work. */
		if ((!rxBuff) && (!txBuff)) {
			TRC_REC("Stopping the TDM\n");
			tdm_if_pcm_stop();
			pcm_stop_flag = 0;
			tasklet_hi_schedule(&tdm_if_stop_tasklet);
		} else {
			TRC_REC("Some tasklet is running, mark pcm_stop_flag\n");
			pcm_stop_flag = 1;
		}
	}

	/* Restarting PCM, skip Rx/Tx handling */
	if (pcm_stop_status)
		goto skip_rx_tx;
#endif

	/* Support multiple interrupt handling */
	/* RX interrupt */
	if (int_type & MV_RX_INT) {
		if (rxBuff != NULL) {
			rx_miss++;
			TRC_REC("%s: Warning, missed Rx buffer processing !!!\n", __func__);
		}
		else {
			rxBuff = tdm_int_info.tdmRxBuff;
#if (defined CONFIG_MV_PHONE_USE_IRQ_PROCESSING) || (defined CONFIG_MV_PHONE_USE_FIQ_PROCESSING)
			TRC_REC("%s: running Rx in ISR\n", __func__);
			tdm_if_pcm_rx_process();
#else
			/* Schedule Rx processing within SOFT_IRQ context */
			TRC_REC("%s: schedule Rx tasklet\n", __func__);
			tasklet_hi_schedule(&tdm_if_rx_tasklet);
#endif
		}
	}

	/* TX interrupt */
	if (int_type & MV_TX_INT) {
		if (txBuff != NULL) {
			tx_miss++;
			TRC_REC("%s: Warning, missed Tx buffer processing !!!\n", __func__);
		}
		else {
			txBuff = tdm_int_info.tdmTxBuff;
#if (defined CONFIG_MV_PHONE_USE_IRQ_PROCESSING) || (defined CONFIG_MV_PHONE_USE_FIQ_PROCESSING)
			TRC_REC("%s: running Tx in ISR\n", __func__);
			tdm_if_pcm_tx_process();
#else
			/* Schedule Tx processing within SOFT_IRQ context */
			TRC_REC("%s: schedule Tx tasklet\n", __func__);
			tasklet_hi_schedule(&tdm_if_tx_tasklet);
#endif
		}
	}

#ifdef CONFIG_MV_TDM_SUPPORT
	/* TDM2CH PCM channels stop indication */
	if ((int_type & MV_CHAN_STOP_INT) && (tdm_int_info.data == 4)) {
		TRC_REC("%s: Received MV_CHAN_STOP_INT indication\n", __func__);
		is_pcm_stopping = 0;
		if (pcm_start_stop_state) {
			TRC_REC("%s: calling to tdm_if_pcm_start()\n", __func__);
			pcm_enable = 0;
			tdm_if_pcm_start();
		}
	}

skip_rx_tx:
#endif
	/* PHONE interrupt */
	if (int_type & MV_PHONE_INT) {
		/* TBD */
#ifdef CONFIG_LANTIQ_SLIC_SUPPORT
		drv_dxt_if_signal_interrupt();
#endif
	}

	/* ERROR interrupt */
	if (int_type & MV_ERROR_INT) {
		if (int_type & MV_RX_ERROR_INT)
			rx_over++;

		if (int_type & MV_TX_ERROR_INT)
			tx_under++;
	}

out:
	TRC_REC("<-%s\n", __func__);
	return IRQ_HANDLED;
}
#if (defined CONFIG_MV_PHONE_USE_IRQ_PROCESSING) || (defined CONFIG_MV_PHONE_USE_FIQ_PROCESSING)
static inline void tdm_if_pcm_rx_process(void)
#else
/* Rx tasklet */
static void tdm_if_pcm_rx_process(unsigned long arg)
#endif
{
	unsigned long flags;

	TRC_REC("->%s\n", __func__);
	if (pcm_enable) {
		if(rxBuff == NULL) {
			TRC_REC("%s: Error, empty Rx processing\n", __func__);
			return;
		}

		/* Fill TDM Rx aggregated buffer */
#ifdef CONFIG_MV_TDM_SUPPORT
		if (mvTdmRx(rxBuff) == MV_OK)
			tdm_if_register_ops->tdm_if_pcm_ops.pcm_rx_callback(rxBuff, buff_size); /* Dispatch Rx handler */
#else
		if (mvCommUnitRx(rxBuff) == MV_OK) {
			tdm_if_register_ops->tdm_if_pcm_ops.pcm_rx_callback(rxBuff, buff_size); /* Dispatch Rx handler */
			/* Since data buffer is shared among MCDMA and CPU, need to invalidate
				before it accessed by MCDMA. MMP may stop channels from this context,
				so make sure the buffer is still valid	*/
			if (pcm_enable)
				mvOsCacheInvalidate(NULL, rxBuff, buff_size);
		}
#endif
		else
			printk(KERN_WARNING "%s: could not fill Rx buffer\n", __func__);

	}

	spin_lock_irqsave(&tdm_if_lock, flags);

	/* Clear rxBuff for next iteration */
	rxBuff = NULL;

#ifdef CONFIG_MV_TDM_SUPPORT
	if ((pcm_stop_flag == 1) && !txBuff) {
		TRC_REC("Stopping TDM from Rx tasklet\n");
		tdm_if_pcm_stop();
		pcm_stop_flag = 0;
		tasklet_hi_schedule(&tdm_if_stop_tasklet);
	}
#endif
	spin_unlock_irqrestore(&tdm_if_lock, flags);

	TRC_REC("<-%s\n", __func__);
	return;
}

#if (defined CONFIG_MV_PHONE_USE_IRQ_PROCESSING) || (defined CONFIG_MV_PHONE_USE_FIQ_PROCESSING)
static inline void tdm_if_pcm_tx_process(void)
#else
/* Tx tasklet */
static void tdm_if_pcm_tx_process(unsigned long arg)
#endif
{
	unsigned long flags;

	TRC_REC("->%s\n", __func__);

	if (pcm_enable) {
		if (txBuff == NULL) {
			TRC_REC("%s: Error, empty Tx processing\n", __func__);
			return;
		}

		/* Dispatch Tx handler */
		tdm_if_register_ops->tdm_if_pcm_ops.pcm_tx_callback(txBuff, buff_size);

		if (test_enable == 0) {
			/* Fill Tx aggregated buffer */
#ifdef CONFIG_MV_TDM_SUPPORT
			if(mvTdmTx(txBuff) != MV_OK)
#else
			if(mvCommUnitTx(txBuff) != MV_OK)
#endif /* CONFIG_MV_TDM_SUPPORT */
				printk(KERN_WARNING "%s: could not fill Tx buffer\n", __func__);
		}
	}

	spin_lock_irqsave(&tdm_if_lock, flags);

	/* Clear txBuff for next iteration */
	txBuff = NULL;

#ifdef CONFIG_MV_TDM_SUPPORT
	if ((pcm_stop_flag == 1) && !rxBuff) {
		TRC_REC("Stopping TDM from Tx tasklet\n");
		tdm_if_pcm_stop();
		pcm_stop_flag = 0;
		tasklet_hi_schedule(&tdm_if_stop_tasklet);
	}
#endif
	spin_unlock_irqrestore(&tdm_if_lock, flags);

	TRC_REC("<-%s\n", __func__);
	return;
}

void tdm_if_stats_get(tdm_if_stats_t* tdm_if_stats)
{
	if (tdm_init == 0)
		return;

	tdm_if_stats->tdm_init = tdm_init;
	tdm_if_stats->rx_miss = rx_miss;
	tdm_if_stats->tx_miss = tx_miss;
	tdm_if_stats->rx_over = rx_over;
	tdm_if_stats->tx_under = tx_under;
#ifdef CONFIG_MV_TDM_EXT_STATS
	mvTdmExtStatsGet(&tdm_if_stats->tdm_ext_stats);
#endif
	return;
}

#ifdef CONFIG_MV_TDM_SUPPORT
static void tdm_if_stop_channels(unsigned long arg)
{
	u32 max_poll = 0;
	unsigned long flags;

	TRC_REC("->%s\n", __func__);

	/* Wait for all channels to stop  */
	while (((MV_REG_READ(CH_ENABLE_REG(0)) & 0x101) ||
		(MV_REG_READ(CH_ENABLE_REG(1)) & 0x101)) && (max_poll < 30)) {
		mdelay(1);
		max_poll++;
	}

	TRC_REC("Finished polling on channels disable\n");
	if (max_poll >= 30) {
		MV_REG_WRITE(CH_ENABLE_REG(0), 0);
		MV_REG_WRITE(CH_ENABLE_REG(1), 0);
		printk(KERN_WARNING "\n\npolling on channels disabling exceeded 30ms\n\n");
#ifdef CONFIG_MV_TDM_EXT_STATS
		pcm_stop_fail++;
#endif
		mdelay(10);
	}

	spin_lock_irqsave(&tdm_if_lock, flags);
	is_pcm_stopping = 0;
	tdm_if_pcm_start();
	spin_unlock_irqrestore(&tdm_if_lock, flags);

	TRC_REC("<-%s\n", __func__);
	return;
}
#endif

static int __init tdm_if_module_init(void)
{
	/* The real init is done later */
	return 0;
}

static void __exit tdm_if_module_exit(void)
{
	tdm_if_exit();
	return;
}

/* Module stuff */
module_init(tdm_if_module_init);
module_exit(tdm_if_module_exit);
MODULE_DESCRIPTION("Marvell TDM I/F Device Driver - www.marvell.com");
MODULE_AUTHOR("Eran Ben-Avi <benavi@marvell.com>");
MODULE_LICENSE("GPL");

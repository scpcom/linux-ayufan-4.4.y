/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#include "fpp.h"
#include "modules.h"
#include "channels.h"
#include "events.h"
#include "module_tx.h"
#include "system.h"
#include "fpart.h"
#include "fe.h"
#include "module_timer.h"
#include "module_qm.h"
#include "module_tx.h"
#include "module_hidrv.h"
#include "module_expt.h"
#include "asm/bitops.h"


#define FFS(x)	__fls(x)

//#define QM_DEBUG

// The global stat array is initialized to invalid values, to force the initial state to be
// written to the class PEs at initialization time.
static u8 qm_global_stat[MAX_PHY_PORTS] = {0xFF, 0xFF, 0xFF};

/** Convert qos context to Big-endian.
 * This function copies the parameters of the qos context
 * into another context in BE format before sending it to PFE.
 *
 * @param le_context_ctl	QOS control context in LE format.
 * @param be_context     	QOS context in BE format.
 *
 */
static void qm_convert_context_to_be (PQM_context_ctl le_context, PQM_context be_context)
{
	int i, j;

	memset (be_context , 0, sizeof(QM_context));
#if defined(CONFIG_PLATFORM_PCI)
	be_context->tmu_id = le_context->tmu_id + 2;
#else
	be_context->tmu_id = le_context->tmu_id;
#endif
	be_context->num_shapers = le_context->num_shapers;
	be_context->num_sched = le_context->num_sched;
	be_context->chip_revision = CHIP_REVISION();


	for (i = 0; i < NUM_SHAPERS; i++)
	{
		be_context->shaper[i].baseaddr = cpu_to_be32(le_context->shaper[i].baseaddr);	
		if (le_context->shaper_ctl[i].enable)
			be_context->shaper[i].qmask = cpu_to_be32(le_context->shaper[i].qmask);	
	}

	for (i =0 ; i < NUM_SCHEDULERS; i++)
	{
		be_context->sched[i].baseaddr = cpu_to_be32(le_context->sched[i].baseaddr);	
		be_context->sched[i].qmask = cpu_to_be32(le_context->sched[i].qmask);
		be_context->sched[i].alg = le_context->sched[i].alg;
		be_context->sched[i].numqueues = le_context->sched[i].numqueues;
		for (j = 0; j < MAX_SCHEDULER_QUEUES; j++)
		{
			be_context->sched[i].queue_mask[j] = cpu_to_be32(le_context->sched[i].queue_mask[j]);
		}
	}

	for (i = 0; i < NUM_QUEUES; i++)
	{
		be_context->weight[i] = cpu_to_be16(le_context->weight[i]);
		be_context->q_len_masks[i] = cpu_to_be32(le_context->q_len_masks[i]);
		be_context->qresult_regoffset[i] = le_context->qresult_regoffset[i];
	}

	return;
}

/** Convert qos context to Big-endian.
 * This function copies the parameters of the qos context
 * into another context in BE format into the LMEM.
 *
 * @param le_shaper	Shaper control structure in LE format.
 * @param be_shaper     Shaper control structure in BE format. (char array)
 *
 */
static void qm_convert_shaper_to_be(PQM_ShaperDesc_ctl le_shaper, u8* be_shaper)
{
	//PQM_ShaperDesc_ctl temp = (PQM_ShaperDesc_ctl) virt_to_tmu_lmem((void*)be_shaper);
	PQM_ShaperDesc_ctl temp = (PQM_ShaperDesc_ctl) (be_shaper);
	int i;

	memset(temp, 0 , QM_CMD_SIZE);

	for (i = 0; i < NUM_SHAPERS; i++, temp++,le_shaper++)
	{
		temp->enable = le_shaper->enable;
		temp->int_wt = le_shaper->int_wt;
		temp->frac_wt = cpu_to_be16(le_shaper->frac_wt);
		temp->max_credit = cpu_to_be32(le_shaper->max_credit);
		temp->clk_div = cpu_to_be32(le_shaper->clk_div);
	}

	return;
}

/** Update the shaper information in the qos context.
 * This function updates the shaper configuration in QOS context.
 * It updates the shaper's qmask based on the queues
 * assigned to the shaper.
 *
 * @param qm_context_ctl	 Qos context control structure.
 *
 */
static void QM_update_shaper(PQM_context_ctl qm_context_ctl)
{

	int num_shapers = 0;
	int i;
	PQM_QDesc pq;

	for (i = 0; i < NUM_SHAPERS; i++)
		qm_context_ctl->shaper[i].qmask = 0;

	for (i = 0; i < NUM_QUEUES; i++)
	{
		int shaper_num;
		pq = &qm_context_ctl->q[i];
		shaper_num = pq->shaper_num;
		if (shaper_num >= NUM_SHAPERS)
			continue;
		qm_context_ctl->shaper[shaper_num].qmask |= 1 << i;
	}

	for (i = 0; i < NUM_SHAPERS; i++)
	{
		if (qm_context_ctl->shaper_ctl[i].enable == TRUE)
			num_shapers = i + 1;
	}
	qm_context_ctl->num_shapers = num_shapers;

	return;
}

/** Update the scheduler information in the qos context.
 * This function updates the scheduler configuration in QOS context. 
 * It updates the scheduler's qmask based on the queues
 * assigned to the shaper.
 *
 * @param qm_context_ctl	 Qos context control structure.
 *
 */
static void QM_update_scheduler(PQM_context_ctl qm_context_ctl)
{
	int i, num_sched = 0;
	PQM_QDesc pq;

	// update schedulers
	qm_context_ctl->sched_mask = 0;
	for (i = 0; i < NUM_SCHEDULERS; i++)
	{
		qm_context_ctl->sched[i].qmask = 0;
		qm_context_ctl->sched[i].numqueues = 0;
	}

	for (i = 0; i < NUM_QUEUES; i++)
	{
		int sched_num;
		pq = &qm_context_ctl->q[i];
		/* Scheduler Configuration should be applied to queue 
		   whether S/W qos is enabled or not */
		sched_num = pq->sched_num;
		if (sched_num >= NUM_SCHEDULERS)
			continue;
		qm_context_ctl->sched[sched_num].qmask |= 1 << i;
		qm_context_ctl->sched[sched_num].queue_mask[qm_context_ctl->sched[sched_num].numqueues] = (1<<i);

		qm_context_ctl->sched_mask |= 1 << sched_num;

		qm_context_ctl->sched[sched_num].numqueues++;
	}

	for (i = 0; i < NUM_SCHEDULERS; i++)
	{
		if (qm_context_ctl->sched[i].qmask != 0)
			num_sched = i + 1;

	}
	qm_context_ctl->num_sched = num_sched;

#ifdef QM_DEBUG
	printk(KERN_INFO "global enable for scheds: %x-%x-%x-%x for tmu %d\n", (unsigned int)TMU_TDQ0_SCH_CTRL, (unsigned int) TMU_TDQ1_SCH_CTRL , (unsigned int) TMU_TDQ2_SCH_CTRL , (unsigned int) TMU_TDQ3_SCH_CTRL, qm_context_ctl->tmu_id);
#endif
	/* Global Enable for the schedulers on Phy */
	switch(qm_context_ctl->tmu_id)
	{
		case TMU0_ID:
			writel(qm_context_ctl->sched_mask, (void*)TMU_TDQ0_SCH_CTRL);
			break;
		case TMU1_ID:
			writel(qm_context_ctl->sched_mask, (void*)TMU_TDQ1_SCH_CTRL);
			break;
		case TMU2_ID:
			writel(qm_context_ctl->sched_mask,(void*) TMU_TDQ2_SCH_CTRL);
			break;
		case TMU3_ID:
			writel(qm_context_ctl->sched_mask, (void*)TMU_TDQ3_SCH_CTRL);
			break;
	}

	return;
}

static int QM_update_qos_enabled_status(u32 port_index, u8 qos_status)
{
	struct pfe_ctrl *ctrl = &pfe->ctrl;
        int id;
	
	if (port_index > 2 )
	{
		printk (KERN_ERR "%s: Invalid portindex %d\n", __func__, port_index);
		return -1;
	}

	// only update class PEs if status changes
	if (qm_global_stat[port_index] == qos_status)
		return NO_ERR;

	qm_global_stat[port_index] = qos_status;

        if (pe_sync_stop(ctrl, CLASS_MASK) < 0)
                return CMD_ERR;

        /* update the DMEM in class-pe */
        for (id = CLASS0_ID; id <= CLASS_MAX_ID; id++)
        {
		pe_dmem_writeb(id, qos_status,virt_to_class_dmem(&g_qos_enable[port_index]));
        }

        pe_start(ctrl, CLASS_MASK);

        return NO_ERR;

}

/** Send the context update request to PFE TMU.
* This function updates the QOS context to TMU and sends the request
* to TMU to configure the shaper and scheduler registers. It converts
* the little-endian structures to big-endian before copying to PE's memory.
*
* @param qm_context_ctl         Qos context control structure.
*
*/

static int QM_update_TMU(PQM_context_ctl qm_context_ctl, u32 flags)
{
	struct pfe_ctrl *ctrl = &pfe->ctrl;

	qm_convert_context_to_be(qm_context_ctl, &g_qm_context);

	if (flags & SHAPER_CONFIG)
	{
		qm_convert_shaper_to_be(qm_context_ctl->shaper_ctl, g_qm_cmd_info);
		pe_dmem_memcpy_to32(qm_context_ctl->tmu_id, virt_to_tmu_dmem(&g_qm_cmd_info), &g_qm_cmd_info, QM_CMD_SIZE);
	}

	/* Write the context to TMU's DMEM */
	pe_dmem_memcpy_to32(qm_context_ctl->tmu_id, virt_to_tmu_dmem(&g_qm_context), &g_qm_context, sizeof (QM_context));

#ifdef QM_DEBUG
	printk (KERN_INFO "Sending context update command to TMU \n");
#endif
	if (tmu_pe_request(ctrl,qm_context_ctl->tmu_id, flags) < 0)
		return CMD_ERR ;

	return NO_ERR;
}


/** Send the request to PFE TMU3 to update the shaper config.
 * This function updates the QOS context to TMU and sends the request
 * to TMU to configure the shaper. It converts
 * the little-endian structures to big-endian before copying to PE's memory.
 *
 * @param qm_context_ctl  Qos context control structure.
 * @param qm_ctl_flags    The control flag denoting the type of action
 *                        to be performed.
 *
 */

static int QM_update_TMU3(PQM_context_ctl qm_context_ctl, u32 qm_ctl_flags)
{
	int rc = NO_ERR;
	struct pfe_ctrl *ctrl = &pfe->ctrl;

	qm_convert_shaper_to_be(qm_context_ctl->shaper_ctl, g_qm_cmd_info);

	pe_dmem_memcpy_to32(TMU3_ID, virt_to_tmu_dmem(&g_qm_cmd_info), &g_qm_cmd_info, QM_CMD_SIZE);
	if (tmu_pe_request(ctrl, TMU3_ID, qm_ctl_flags)  < 0)
		rc = CMD_ERR;

	return rc;
}

/** Update qlenmasks.
 * This function updates the q_len_masks based the scheduler and shaper
 * configured for the queue.
 *
 * @param qm_context_ctl	 Qos control context structure.
 *
 */
static void QM_update_qlenmask(PQM_context_ctl qm_context_ctl)
{
	int i;

	for (i = 0; i < NUM_QUEUES; i++)
		qm_context_ctl->q_len_masks[i] = 0;

	for (i = 0; i < NUM_QUEUES; i++)
	{
		/* q_len_masks[i] :
		 * 1) says if 0th queue is winner, 
		 In which all schedulers and shapers,
		 *     Length should be written.
		 *  2) q_len_masks[i][7:0] = scheduler_len_mask
		 *  3) q_len_masks[i][17:8] = shaper_len_mask
		 */
		/* TODO - Need to verify if its correct */

		qm_context_ctl->q_len_masks[i] = (1 << qm_context_ctl->q[i].sched_num);
		if ((qm_context_ctl->q[i].shaper_num >= 0) && (qm_context_ctl->q[i].shaper_num < NUM_SHAPERS))
			qm_context_ctl->q_len_masks[i] |= (1 << (8 + qm_context_ctl->q[i].shaper_num));
	}
}


/** Update qdepth to the PFE H/W.
 * This function updates the qdepth parameters to the 
 * Hardware registers 
 *
 * @param qm_context_ctl	 Qos context control structure.           
 *
 */
static void QM_update_qdepth(PQM_context_ctl qm_context_ctl)
{
	int i;
	u32 qdepth;

	for (i =0; i < NUM_QUEUES; i++)
	{
		qdepth = qm_context_ctl->max_qdepth[i];

		// LOG: 68855
		// The following is a workaround for the reordered packet and BMU2 buffer leakage issue.
		if (CHIP_REVISION() == 0)
			qdepth = 31;   // ignore configured value, and force all qdepths to be 31 for now

		if (qdepth)
		{
			/* Write the phyno and queueno to CTL register */
			/* current phyno : 8:11 , qno: 0:7 */
			writel(((qm_context_ctl->port << 8) | i) ,(void*) TMU_TEQ_CTRL);
			/* Enable the tail drop for the queues */
	//		writel( TEQ_HTD , (void*)TMU_TEQ_QCFG);

			/* Write the qmax value to the probability 82:103 bits
			   i.e (82:95)first 14 bits in HW_PROB_CFG2 register
			   and (96:103) 8 bits in HW_PROB_CFG3 register */
			writel( (qdepth & 0x3fff) << 18 , (void*)TMU_TEQ_HW_PROB_CFG2);
			writel( (qdepth & 0x3fffff) >> 14 , (void*)TMU_TEQ_HW_PROB_CFG3);
		}
	}

	return;
}

/** Update inter-frame-gap for a TMU in PFE H/W registers.
 * This function updates the ifg parameter configured
 * in the PFE hardware register for a TMU.
 *
 * @param qm_context_ctl	 Qos context control structure.
 *
 */
static void QM_update_ifg(PQM_context_ctl qm_context_ctl)
{
	u32 temp;
	u8* val;

	temp = readl((void*)TMU_TDQ_IIFG_CFG);
	val = (u8*) &temp;
	val[qm_context_ctl->port] = qm_context_ctl->ifg;
	writel(temp, (void*)TMU_TDQ_IIFG_CFG);
#ifdef QM_DEBUG
	printk(KERN_INFO "Updating ifg at addr: %x port: %d val = %x\n", (unsigned int)TMU_TDQ_IIFG_CFG , qm_context_ctl->port, temp); 
#endif

	return;
}


/** Update the qos control context with configuration change.
 * This function updates the QOS control context based on the
 * configuration change and control flags passed.
 *
 * @param index	 	port index (GEM0 , GEM1, GEM2)
 * @param qm_ctl_flags	The control flag denoting the type of action
 *			to be performed.
 *
 */
static int QM_update_context(u32 index, u32 qm_ctl_flags)
{
	int rtn_code = NO_ERR;
	PQM_context_ctl qm_context_ctl;

	if (qm_global_stat[index] == QM_DISABLE)
		qm_context_ctl = QM_GET_QOSOFF_CONTEXT(index);
	else
		qm_context_ctl = QM_GET_CONTEXT(index);

	if (qm_ctl_flags & QDEPTH_CONFIG)
		QM_update_qdepth(qm_context_ctl);

	if (qm_ctl_flags & IFG_CONFIG)
		QM_update_ifg(qm_context_ctl);

	if (qm_ctl_flags & SCHEDULER_CONFIG)
		QM_update_scheduler(qm_context_ctl);

	if (qm_ctl_flags & SHAPER_CONFIG)
		QM_update_shaper(qm_context_ctl);

	if (qm_ctl_flags & (SCHEDULER_CONFIG | SHAPER_CONFIG))
		QM_update_qlenmask(qm_context_ctl);

	// update the TMU
	if (qm_ctl_flags & (SCHEDULER_CONFIG | SHAPER_CONFIG))
		rtn_code = QM_update_TMU(qm_context_ctl, qm_ctl_flags & (SCHEDULER_CONFIG | SHAPER_CONFIG));

	return rtn_code;
}

/** Updates the base addresses of the scheduler and shapers.
 * This function initializes the base addresses of the
 * SHAPER and SCHEDULER registers
 *
 * @param qm_context_ctl	 Qos control context structure.
 *
 */

static void QM_baseaddr_init(PQM_context_ctl qm_context_ctl)
{

	/*Initialize shaper base addresses */
	qm_context_ctl->shaper[0].baseaddr = SHAPER0_BASE_ADDR;
	qm_context_ctl->shaper[1].baseaddr = SHAPER1_BASE_ADDR;
	qm_context_ctl->shaper[2].baseaddr = SHAPER2_BASE_ADDR;
	qm_context_ctl->shaper[3].baseaddr = SHAPER3_BASE_ADDR;
	qm_context_ctl->shaper[4].baseaddr = SHAPER4_BASE_ADDR;
	qm_context_ctl->shaper[5].baseaddr = SHAPER5_BASE_ADDR;
	qm_context_ctl->shaper[6].baseaddr = SHAPER6_BASE_ADDR;
	qm_context_ctl->shaper[7].baseaddr = SHAPER7_BASE_ADDR;
	qm_context_ctl->shaper[8].baseaddr = SHAPER8_BASE_ADDR;
	qm_context_ctl->shaper[9].baseaddr = SHAPER9_BASE_ADDR;


	/*Initialize scheduler base addresses */
	qm_context_ctl->sched[0].baseaddr = SCHED0_BASE_ADDR;
	qm_context_ctl->sched[1].baseaddr = SCHED1_BASE_ADDR;
	qm_context_ctl->sched[2].baseaddr = SCHED2_BASE_ADDR;
	qm_context_ctl->sched[3].baseaddr = SCHED3_BASE_ADDR;
	qm_context_ctl->sched[4].baseaddr = SCHED4_BASE_ADDR;
	qm_context_ctl->sched[5].baseaddr = SCHED5_BASE_ADDR;
	qm_context_ctl->sched[6].baseaddr = SCHED6_BASE_ADDR;
	qm_context_ctl->sched[7].baseaddr = SCHED7_BASE_ADDR;



}

/** Updates the hardware phyqueues . 
 * This function initializes the hardware phy queues 
 * in the qos context 
 *
 * @param qm_context_ctl	 Qos control context structure.           
 *
 */
static void QM_hwqueue_init(PQM_context_ctl qm_context_ctl)
{
	int i;
	/*
	   Queue result mapping:
		TMU0: 	QUEUE_RESULT0, QUEUE_RESULT1, QUEUE_RESULT2 --> PHY0_INQ_ADDR
		TMU1: 	QUEUE_RESULT0, QUEUE_RESULT1, QUEUE_RESULT2 --> PHY1_INQ_ADDR
		TMU2: 	QUEUE_RESULT0, QUEUE_RESULT1, QUEUE_RESULT2 --> PHY2_INQ_ADDR
		TMU3: 	QUEUE_RESULT0 --> PHY3_INQ_ADDR, 
			QUEUE_RESULT1 --> PHY4_INQ_ADDR, 
			QUEUE_RESULT2 --> PHY5_INQ_ADDR

	Initialized by host:
		PHY0_INQ_ADDR = GPI0
		PHY1_INQ_ADDR = GPI1
		PHY2_INQ_ADDR = GPI2
		PHY3_INQ_ADDR = HIF
		PHY4_INQ_ADDR = HIFNCPY
		PHY5_INQ_ADDR = UTIL-PE
	 */

	if (qm_context_ctl->tmu_id == TMU3_ID)
	{
		/* TMU 3 Queues 0-3 => Util PE*/
		for (i = 0; i <= 3; i++)
			qm_context_ctl->qresult_regoffset[i] = QUEUE_RESULT2_REGOFFSET;
		/* TMU 3 Queues 4-15 => HIF */
		for (i = 4; i <= 15; i++)
			qm_context_ctl->qresult_regoffset[i] = QUEUE_RESULT0_REGOFFSET;

		qm_context_ctl->qresult_regoffset[TMU_QUEUE_RTP_CUTTHRU] = QUEUE_RESULT1_REGOFFSET;
	}
	else
	{
		for (i = 0; i < NUM_QUEUES; i++)
			qm_context_ctl->qresult_regoffset[i] = QUEUE_RESULT0_REGOFFSET;
	}

}


/** Resets to the default configuration for a context.
 * This function resets the QM control context to 
 * its default configuration.
 *
 * @param qm_context_ctl  QOS control context stucture
 * @param index	 port index (GEM0 , GEM1, GEM2)
 *
 */
/*QM_reset -- reset QM context to initial default values */
static int QM_reset(PQM_context_ctl qm_context_ctl, int index) __attribute__ ((noinline));
static int QM_reset(PQM_context_ctl qm_context_ctl, int index)
{
	int i;

	if (index >= GEM_PORTS)
		return CMD_ERR;

	memset(qm_context_ctl, 0, sizeof(struct tQM_context_ctl));

	qm_context_ctl->port = index;
	qm_context_ctl->tmu_id = index + TMU0_ID;

	/* Assign first 8 queues to scheduler 0
	   and other 8 queues to scheduler 1 */
	for (i = 0; i < NUM_QUEUES; i++) {
		if ( i < 8)
			qm_context_ctl->max_qdepth[i] = DEFAULT_LOWPRI_MAX_QDEPTH ;
		else
			qm_context_ctl->max_qdepth[i] = DEFAULT_HIPRI_MAX_QDEPTH ;
		qm_context_ctl->q[i].shaper_num = 0;
		qm_context_ctl->weight[i] = 0;

		if (i < 8)
			qm_context_ctl->q[i].sched_num = 0;
		else
			qm_context_ctl->q[i].sched_num = 1;
	}

	QM_baseaddr_init(qm_context_ctl);
	QM_hwqueue_init(qm_context_ctl);
	qm_context_ctl->num_shapers = 0;
	qm_context_ctl->ifg = QM_IFG_SIZE + 4; // add 4 to account for FCS

	/* Shapers are disabled */
	for (i = 0; i < NUM_SHAPERS; i++) {
		qm_context_ctl->shaper_ctl[i].enable = FALSE;
	}

	qm_context_ctl->num_sched = 2;
	for (i = 0; i < NUM_SCHEDULERS; i++) {
		qm_context_ctl->sched[i].alg = QM_ALG_PQ;
	}


	/* Configure shaper 1GBps for TMU0 and all queues */
	if (qm_context_ctl->tmu_id == TMU0_ID) {
		PQM_ShaperDesc_ctl pshaper_ctl;

		pshaper_ctl = &qm_context_ctl->shaper_ctl[0];

		pshaper_ctl->enable = 1;
		pshaper_ctl->int_wt = 2;
		pshaper_ctl->frac_wt = 128;
		pshaper_ctl->clk_div = 4;
		pshaper_ctl->max_credit = 1;

		for (i = 0; i < NUM_QUEUES; i++)
			qm_context_ctl->q[i].shaper_num = 0;
	}
	return NO_ERR;
}


#if 0
/** Resets to the default configuration for exception path TMU.
 * This function resets the QM control context  to
 * its default configuration.
 *
 */
static int QM_reset_tmu3(void)
{
	int qno, i;

	PQM_context_ctl qm_context_ctl;

	qm_context_ctl = &gQMExptCtx;

	qm_context_ctl->port = EXPT_PORT_ID;
	qm_context_ctl->tmu_id = TMU3_ID;

	for (i = 0; i < NUM_SHAPERS; i++) {
		qm_context_ctl->shaper_ctl[i].enable = FALSE;
	}
	for (qno = 0; qno < NUM_QUEUES; qno++)
	{
		qm_context_ctl->max_qdepth[qno] = DEFAULT_LOWPRI_MAX_QDEPTH;
		qm_context_ctl->q[qno].shaper_num = 0;
		qm_context_ctl->weight[i] = 0;
		switch(qno)
		{
			case TMU_QUEUE_PCAP:
			case TMU_QUEUE_RESERVED1:
			case TMU_QUEUE_RESERVED2:
				qm_context_ctl->q[qno].sched_num = 0;
				break;
			case TMU_QUEUE_IPSEC_IN:
			case TMU_QUEUE_IPSEC_OUT:
				qm_context_ctl->q[qno].sched_num = 1;
				break;
			case TMU_QUEUE_RX0_LOW:
			case TMU_QUEUE_RX1_LOW:
			case TMU_QUEUE_RX2_LOW:
				qm_context_ctl->q[qno].shaper_num = 0;
				qm_context_ctl->q[qno].sched_num = 1;
				break;
			case TMU_QUEUE_WIFI_LOW:
				qm_context_ctl->q[qno].shaper_num = 1;
				qm_context_ctl->q[qno].sched_num = 1;
				break;
			case TMU_QUEUE_RX0_HIGH:
			case TMU_QUEUE_RX1_HIGH:
			case TMU_QUEUE_RX2_HIGH:
				qm_context_ctl->q[qno].shaper_num = 0;
				qm_context_ctl->q[qno].sched_num = 2;
				break;
			case TMU_QUEUE_WIFI_HIGH:
				qm_context_ctl->q[qno].shaper_num = 1;
				qm_context_ctl->q[qno].sched_num = 2;
				break;
			case TMU_QUEUE_RTP_RELAY:
			case TMU_QUEUE_RTP_CUTTHRU:
			case TMU_QUEUE_EVENTIND:
				qm_context_ctl->q[qno].sched_num = 3;
				break;
		}
	}

	QM_baseaddr_init(qm_context_ctl);
	QM_hwqueue_init(qm_context_ctl);
	qm_context_ctl->num_shapers = 2 ;
	qm_context_ctl->ifg = 0;

	for (i = 0; i < qm_context_ctl->num_shapers; i++) {
		qm_context_ctl->shaper_ctl[i].enable = TRUE;

		/* TODO- update rate, intwt, fracwt and burst size */
		qm_context_ctl->shaper_ctl[i].int_wt = 0x3;
		qm_context_ctl->shaper_ctl[i].frac_wt = 0xff;
		qm_context_ctl->shaper_ctl[i].clk_div = 0;
		qm_context_ctl->shaper_ctl[i].max_credit = DEFAULT_EXPT_RATE * 10; // MAX CREDIT value
	}


	qm_context_ctl->num_sched = 4;
	for (i = 0; i < qm_context_ctl->num_sched; i++) {
		qm_context_ctl->sched[i].alg = QM_ALG_RR;
	}
}
#endif



/** DSCP to Queue mapping configuration.
 * This function handles the dscp to queue mapping in the local structure
 * and transfers the same structure to all classpe's
 * 
 * @param p        DSCP to queue command structure.
 * @param Length   length of the command passed.
 *
 */

static int QM_Handle_DSCP_QueueMod(U16 *p, U16 Length)
{
	QoSDSCPQmodCommand 	cmd;
	int i, id;
	struct pfe_ctrl *ctrl = &pfe->ctrl;

	if (Length > sizeof(QoSDSCPQmodCommand))
		return ERR_WRONG_COMMAND_SIZE;

	SFL_memcpy((U8*)&cmd, (U8*)p,  sizeof(QoSDSCPQmodCommand));

	if(cmd.queue >= NUM_QUEUES)
		return ERR_QM_QUEUE_OUT_OF_RANGE;

	if(cmd.num_dscp > NUM_DSCP_VALUES)
		return ERR_QM_NUM_DSCP_OUT_OF_RANGE;

	for(i = 0; i < cmd.num_dscp; i++)
		if(cmd.dscp[i] >= NUM_DSCP_VALUES)
			return ERR_QM_DSCP_OUT_OF_RANGE;

	//the whole command is correct, we can assign dscp to queues
	for(i = 0; i < cmd.num_dscp; i++)
		DSCP_to_Qmod[cmd.dscp[i]] = cmd.queue;

	if( pe_sync_stop(ctrl, CLASS_MASK) < 0)
		return CMD_ERR;

	/* update the DMEM in class-pe */
	for (id = CLASS0_ID; id <= CLASS_MAX_ID; id++)
		pe_dmem_memcpy_to32(id, virt_to_class_dmem(DSCP_to_Qmod), DSCP_to_Qmod, sizeof (DSCP_to_Qmod));

	pe_start(ctrl, CLASS_MASK);

	return NO_ERR;
}

/** Fill the QOS query command.
 * This function is used to fill the information from the qos context
 * to the command strucutre on issue of query command.
 *
 * @param pQoscmd  Qos query command structure.
 *
 */
static void QM_Get_Info(pQosQueryCmd pQoscmd)
{

	PQM_context_ctl qm_context_ctl;
	PQM_ShaperDesc pshaper;
	PQM_ShaperDesc_ctl pshaper_ctl;
	PQM_SchedDesc psched;
	//PQM_QDesc pq;
	int i;


	qm_context_ctl = QM_GET_CONTEXT(pQoscmd->port);

	pQoscmd->queue_qosenable_mask =  qm_global_stat[qm_context_ctl->port] == QM_ENABLE ? 0xffffffff : 0;
	//pQoscmd->max_txdepth = 0;  // not used for C2K

	for (i = 0; i < NUM_SHAPERS; i++)
	{
		pshaper = &qm_context_ctl->shaper[i];
		pshaper_ctl = &qm_context_ctl->shaper_ctl[i];
		pQoscmd->shaper_qmask[i] = pshaper->qmask;
		pQoscmd->tokens_per_clock_period[i] = pshaper->shaper_rate;
		pQoscmd->bucket_size[i] = pshaper->bucket_size;
	}

	for (i = 0; i < NUM_SCHEDULERS; i++)
	{
		psched = &qm_context_ctl->sched[i];
		pQoscmd->sched_qmask[i] = psched->qmask;
		pQoscmd->sched_alg[i] = psched->alg;
	}

	for (i = 0; i < NUM_QUEUES; i++)
	{
		//pq = &qm_context->q[i];
		pQoscmd->max_qdepth[i] = qm_context_ctl->max_qdepth[i];
		//pQoscmd->weight[i] = pq->weight;
	}


	return;


}


/** QOS calculate weights and clock divider based on rate.
 * This function calculates weights and clock divider value according to
 * the rate configured.
 *
 * Note that the clock divider must be a power of two.
 *
 * @param rate   Rate configured in Kbps.
 * @param shaper_ctl shaper structure where shaper info needs to be configured.
 *
 */

#define MAX_CLKDIV	8192
#define MAX_WT		1023

static int  qm_cal_shaperwts( u32 rate , PQM_ShaperDesc_ctl shaper_ctl)
{
	struct pfe_ctrl *ctrl = &pfe->ctrl;
	u32 sysclk_khz = ctrl->sys_clk;
	u32 clkdiv, trydiv;
        u32 wt;
	u32 w_i, w_f;

        if ((rate < 8) || (rate > 1000000))
        {
                printk(KERN_ERR "Invalid shaper rate requested (%d) -- must be 8 to 1000000\n", rate);
                return -1;
        }

#ifdef QM_DEBUG
	printk("axi_clk = %d\n", sysclk_khz);
#endif

	clkdiv = 1;
	// find the clkdiv value that gives us the largest valid wt value
	while (clkdiv < MAX_CLKDIV)
	{
		trydiv = clkdiv << 1;
		wt = (rate * trydiv * (256 / 8)) / sysclk_khz;
		if (wt > MAX_WT)
			break;
		clkdiv = trydiv;
	}

	wt = (rate * clkdiv * (256 / 8)) / sysclk_khz;
	w_i = wt >> 8;
        w_f = wt & 0xff;

	if (CHIP_REVISION() == 0)
	{
		// Workaround for hardware bug
		while (w_f >= 128 && clkdiv > 1)
		{
			clkdiv /= 2;
			wt = (rate * clkdiv * (256 / 8)) / sysclk_khz;
			w_i = wt >> 8;
			w_f = wt & 0xFF;
		}
	}

        shaper_ctl->clk_div = clkdiv;
        shaper_ctl->int_wt = w_i;
        shaper_ctl->frac_wt = w_f;

#ifdef QM_DEBUG
        printk(KERN_INFO "shaper settings: rate=%d, clock_div=%d, int_wt=%d, frac_wt=%d \n", rate, shaper_ctl->clk_div, shaper_ctl->int_wt, shaper_ctl->frac_wt);
#endif

/*
 * There will be a slight error due to w_i and w_f calculations.
 * The actual rate will always be less than or equal to the desired rate.
 *
 * The error can be calculated as:
 *	float actual_rate, error;
 *	actual_rate  = ((float)sysclk_khz / (float)clk_div) * ((float)w_i + ((float)w_f / 256.0)) * 8.0;
 *	error = ((float)rate - actual_rate) / (float)rate;
 */

        return 0;
}





/** QOS calculate weights and clock divider based on rate for TMU3.
 * This function calculates weights and clock divider value according to 
 * the rate configured.
 *
 * @param pkts_per_msec   Packets per msec.
 * @param shaper_ctl shaper structure where shaper info needs to be configured.
 *
 */
static void qm_cal_shaperwts_tmu3( u16 pkts_per_msec , PQM_ShaperDesc_ctl shaper_ctl)
{
	shaper_ctl->int_wt = 3;
	shaper_ctl->frac_wt = 0;
	//shaper_ctl->clk_div = QM_SYS_CLK / ((rate / 8) / shaper_ctl->int_wt );
	shaper_ctl->clk_div = 8;
	shaper_ctl->max_credit = 1000;
	return;
}

/** QOS command executer.
 * This function is the QOS handler function / the entry point
 * to process the qos commands
 *
 * @param cmd_code   Command code.
 * @param cmd_len    Command length.
 * @param p          Command structure.
 *
 */

static U16 M_qm_cmdproc(U16 cmd_code, U16 cmd_len, U16 *p)
{
	struct pfe_ctrl *ctrl = &pfe->ctrl;
	struct tQM_context_ctl *qm_context_ctl;
	int i;
	U16 rtncode = 0;
	U16 retlen = 2;

	rtncode = CMD_OK;
#ifdef QM_DEBUG
	printk(KERN_INFO "%s: cmd_code=0x%x\n", __func__, cmd_code);
#endif
	switch (cmd_code)
	{
		// enable/disable QOS processing
		case CMD_QM_QOSALG:
		case CMD_QM_MAX_TXDEPTH:
		case CMD_QM_RATE_LIMIT:
		case CMD_QM_QUEUE_QOSENABLE:
			break;

		case CMD_QM_QOSENABLE:
		{
			PQueueQosEnableCommand pcmd = (PQueueQosEnableCommand)p;
			qm_context_ctl = QM_GET_CONTEXT(pcmd->port);
#ifdef QM_DEBUG
			printk(KERN_INFO "QOS %s for port %d\n", pcmd->enable_flag ? "enable" : "disable", pcmd->port);
#endif
			if (qm_global_stat[pcmd->port] == pcmd->enable_flag)
				break;

			if (pe_sync_stop(ctrl, 1 << qm_context_ctl->tmu_id) < 0)
			{
				rtncode = CMD_ERR;
				break;
			}
			QM_update_qos_enabled_status(qm_context_ctl->port, pcmd->enable_flag);
			rtncode = QM_update_context(pcmd->port,
					SCHEDULER_CONFIG|SHAPER_CONFIG|QDEPTH_CONFIG|IFG_CONFIG);
			pe_start(ctrl, 1 << qm_context_ctl->tmu_id);
			break;
		}

		case CMD_QM_MAX_QDEPTH:
		{
			PQosMaxqdepthCommand pcmd;
#ifdef QM_DEBUG
			printk(KERN_INFO "MAX QDEPTH command received %d\n", cmd_code);
#endif
			pcmd = (PQosMaxqdepthCommand)p;
			qm_context_ctl = QM_GET_CONTEXT(pcmd->port);
			for (i = 0; i < NUM_QUEUES; i++) {
				if (pcmd->maxqdepth[i] > 0)
					qm_context_ctl->max_qdepth[i] = pcmd->maxqdepth[i];
			}
			if (pe_sync_stop(ctrl, 1 << qm_context_ctl->tmu_id) < 0)
			{
				rtncode = CMD_ERR;
				break;
			}
			rtncode = QM_update_context(pcmd->port, QDEPTH_CONFIG);
			pe_start(ctrl, 1 << qm_context_ctl->tmu_id);
			break;
		}
					// set weight parameters
		case CMD_QM_MAX_WEIGHT:
		{
			PQosWeightCommand pcmd = (PQosWeightCommand)p;
			qm_context_ctl = QM_GET_CONTEXT(pcmd->port);
			for (i = 0; i < NUM_QUEUES; i++) {
				if (pcmd->weight[i] > 0)
				{
#ifdef QM_DEBUG
					printk(KERN_INFO "Setting qweight: port=%d, queue=%d, weight=%d\n", pcmd->port, i, pcmd->weight[i]);
#endif
					qm_context_ctl->weight[i] = pcmd->weight[i];
				}
			}
			if (pe_sync_stop(ctrl, 1 << qm_context_ctl->tmu_id) < 0)
			{
				rtncode = CMD_ERR;
				break;
			}
			rtncode = QM_update_context(pcmd->port, SCHEDULER_CONFIG);
			pe_start(ctrl, 1 << qm_context_ctl->tmu_id);
			break;
		}
					// set exception handler rate limit
		case CMD_QM_EXPT_RATE:
		{
			PQosExptRateCommand pcmd = (PQosExptRateCommand)p;
			PQM_ShaperDesc_ctl pshaper_ctl;
#ifdef QM_DEBUG
			printk(KERN_INFO "EXPT Rate command received %d\n", cmd_code);
#endif
			if ((pcmd->expt_iftype != EXPT_TYPE_ETH) && (pcmd->expt_iftype != EXPT_TYPE_WIFI))
			{
				rtncode = CMD_ERR;
				break;
			}

			gQMExptCtx.shaper[pcmd->expt_iftype].shaper_rate = pcmd->pkts_per_msec;
			pshaper_ctl = &gQMExptCtx.shaper_ctl[pcmd->expt_iftype];
			qm_cal_shaperwts_tmu3(pcmd->pkts_per_msec, pshaper_ctl);
			if (pe_sync_stop(ctrl, 1 << TMU3_ID) < 0)
			{
				rtncode = CMD_ERR;
				break;
			}
			rtncode = QM_update_TMU3(&gQMExptCtx,SHAPER_CONFIG);
			//gExptGlobals.TxRateLimit = pcmd->pkts_per_msec;
			pe_start(ctrl, 1 << TMU3_ID);
			break;
		}

		case CMD_QM_QUERY:
		{
		       pQosQueryCmd pcmd = (pQosQueryCmd)p;
#ifdef QM_DEBUG
		       printk(KERN_INFO "QUERY command received %d - cmdlen %d -  size%d\n", cmd_code,cmd_len, sizeof(QosQueryCmd));
#endif
		       if (cmd_len != sizeof(QosQueryCmd))
		       {
			       rtncode = CMD_ERR;
			       break;
		       }
		       QM_Get_Info(pcmd);
		       retlen = sizeof(QosQueryCmd);

		       break;
		}

		case CMD_QM_QUERY_EXPT_RATE:
		{
			PQosExptRateCommand pcmd = (PQosExptRateCommand)p;
			if ((pcmd->expt_iftype != EXPT_TYPE_ETH) && (pcmd->expt_iftype != EXPT_TYPE_WIFI))
			{
				rtncode = CMD_ERR;
				break;
			}
			pcmd->pkts_per_msec = gQMExptCtx.shaper[pcmd->expt_iftype].shaper_rate;
			retlen = sizeof(QosExptRateCommand);
			break;
		}

		case CMD_QM_RESET:
		{
			PQosResetCommand pcmd = (PQosResetCommand)p;
			qm_context_ctl = QM_GET_CONTEXT(pcmd->port);
			rtncode = QM_reset(qm_context_ctl, pcmd->port);
			if (rtncode != NO_ERR)
				break;
			if (pe_sync_stop(ctrl, 1 << qm_context_ctl->tmu_id) < 0)
			{
				rtncode = CMD_ERR;
				break;
			}
			QM_update_qos_enabled_status(qm_context_ctl->port, QM_INITIAL_ENABLE_STATE);
			rtncode = QM_update_context(pcmd->port,
					SCHEDULER_CONFIG|SHAPER_CONFIG|QDEPTH_CONFIG|IFG_CONFIG);
			pe_start(ctrl, 1 << qm_context_ctl->tmu_id);
			break;
		}

		case CMD_QM_SHAPER_CONFIG:
		{
			U32 shaper_num;
			PQM_ShaperDesc pshaper;
			PQM_ShaperDesc_ctl pshaper_ctl;
			U32 qmask;
			PQosShaperConfigCommand pcmd = (PQosShaperConfigCommand)p;
			qm_context_ctl = QM_GET_CONTEXT(pcmd->port);
			shaper_num = pcmd->shaper_num;
			if (shaper_num >= NUM_SHAPERS)
			{
				rtncode = CMD_ERR;
				break;
			}
			pshaper = &qm_context_ctl->shaper[shaper_num];
			pshaper_ctl = &qm_context_ctl->shaper_ctl[shaper_num];
			if (pcmd->enable_disable_control == 1)
				pshaper_ctl->enable = TRUE;
			else if (pcmd->enable_disable_control == 2)
				pshaper_ctl->enable = FALSE;
			if (pcmd->ifg_change_flag)
				//pshaper->ifg = pcmd->ifg;
				qm_context_ctl->ifg = pcmd->ifg + 4;	// add 4 to account for FCS

			if (pcmd->rate)
			{
				U32 bucket_size;
				/*Tokens stored in bits per 1ms
				clock period =
				(rate * bps_PER_Kbps)/MILLISEC_PER_SEC
				* bps_PER_Kbps = 1000 & MILLISEC_PER_SEC = 1000
				* Hence tokens per 1ms clock period = rate */
				qm_context_ctl->shaper[shaper_num].shaper_rate = pcmd->rate;
				qm_cal_shaperwts(pcmd->rate, pshaper_ctl);
				bucket_size = pcmd->bucket_size;
				qm_context_ctl->shaper[shaper_num].bucket_size = bucket_size;
				if (bucket_size == 0)
					bucket_size = pcmd->rate / 8;  // default bucket size is bytes per msec
				pshaper_ctl->max_credit = bucket_size;
			}
			qmask = pcmd->qmask;
			while (qmask)
			{
				i = FFS(qmask);
				qm_context_ctl->q[i].shaper_num = shaper_num;
				qmask &= ~(1 << i);
			}
			if (pe_sync_stop(ctrl, 1 << qm_context_ctl->tmu_id) < 0)
			{
				rtncode = CMD_ERR;
				break;
			}
			if (pcmd->ifg_change_flag)
				rtncode = QM_update_context(pcmd->port, SHAPER_CONFIG | IFG_CONFIG);
			else
				rtncode = QM_update_context(pcmd->port, SHAPER_CONFIG);

			pe_start(ctrl, 1 << qm_context_ctl->tmu_id);
			break;
		}

		case CMD_QM_SCHEDULER_CONFIG:
		{
		      U32 sched_num;
		      PQM_SchedDesc psched;
		      U32 qmask;
		      U32 numqueues;
		      PQosSchedulerConfigCommand pcmd = (PQosSchedulerConfigCommand)p;
		      qm_context_ctl = QM_GET_CONTEXT(pcmd->port);
		      sched_num = pcmd->sched_num;
		      if (sched_num >= NUM_SCHEDULERS)
		      {
			      rtncode = CMD_ERR;
			      break;
		      }
		      psched = &qm_context_ctl->sched[sched_num];	
		      qmask = pcmd->qmask | psched->qmask;
		      numqueues = 0;
		      while(qmask)
		      {
			      i = FFS(qmask);
			      numqueues++;
			      qmask &= ~(1 << i);
		      }
		      if (numqueues > MAX_SCHEDULER_QUEUES)
		      {
			      rtncode = CMD_ERR;
			      break;
		      }

		      if (pcmd->alg_change_flag)
			      psched->alg = pcmd->alg;
		      qmask = pcmd->qmask;
		      while (qmask)
		      {
			      i = FFS(qmask);
			      qm_context_ctl->q[i].sched_num = sched_num;
			      qmask &= ~(1 << i);
		      }
			if (pe_sync_stop(ctrl, 1 << qm_context_ctl->tmu_id) < 0)
			{
				rtncode = CMD_ERR;
				break;
			}
		      rtncode = QM_update_context(pcmd->port, SCHEDULER_CONFIG);
			pe_start(ctrl, 1 << qm_context_ctl->tmu_id);
		      break;
		}

		case CMD_QM_DSCP_QM:
		{
			rtncode = QM_Handle_DSCP_QueueMod(p, cmd_len);
			break;
		}

		// unknown command code
		default:
		{
			rtncode = CMD_ERR;
			break;
		}
	}

	*p = rtncode;
#ifdef QM_DEBUG
	if (rtncode != 0)
		printk(KERN_INFO "%s: Command error, rtncode=%d", __func__, (short)rtncode);
#endif
	return retlen;
}

/** QOS init function.
 * This function initializes the qos control context with default configuration
 * and sends the same configuration to TMU.
 *
 */
int qm_init()
{
	int i;
	struct pfe_ctrl *ctrl = &pfe->ctrl;
	PQM_context_ctl qm_context_ctl;

	set_cmd_handler(EVENT_QM,M_qm_cmdproc);
	set_cmd_handler(EVENT_EXPT,M_expt_cmdproc);

	pe_sync_stop(ctrl, TMU_MASK);

	for (i = 0; i < GEM_PORTS; i++)
	{
		qm_context_ctl = &gQMCtx[i];
		gQMpCtx[i] = qm_context_ctl;

		/* init to default values */
		QM_reset(QM_GET_QOSOFF_CONTEXT(i), i);
		QM_reset(qm_context_ctl, i);
		QM_update_qos_enabled_status(i, QM_INITIAL_ENABLE_STATE);
		QM_update_context(i, SCHEDULER_CONFIG|SHAPER_CONFIG|QDEPTH_CONFIG|IFG_CONFIG);
	}

#ifdef QM_DEBUG
	printk (KERN_INFO "Writing the tail drop mechanism at addr : %x val :%d \n", (unsigned int) TMU_TEQ_DISABLE_DROPCHK, 0x5);
#endif
	/* Enable the taildrop mechanism */
	writel(0x5, (void*)TMU_TEQ_DISABLE_DROPCHK );

#if 0
	memset(&gQMExptCtx, 0, sizeof(struct tQM_context_ctl)); 
	/* Initialization for Exception Path QOS */
	/* TODO - I think this initialization can be done in TMU itself
	   if we dont use shapers for configuration */
	QM_reset_tmu3();
#endif

#ifdef QM_DEBUG
	printk (KERN_INFO "ifg = %x\n", readl((void*)TMU_TDQ_IIFG_CFG));
#endif

	pe_start(ctrl, TMU_MASK);
	return NO_ERR;
}

/** QOS exit function.
*/
void qm_exit(void)
{

}


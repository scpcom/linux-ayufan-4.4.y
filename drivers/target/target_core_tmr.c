/*******************************************************************************
 * Filename:  target_core_tmr.c
 *
 * This file contains SPC-3 task management infrastructure
 *
 * Copyright (c) 2009,2010 Rising Tide Systems
 * Copyright (c) 2009,2010 Linux-iSCSI.org
 *
 * Nicholas A. Bellinger <nab@kernel.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 ******************************************************************************/

#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/export.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>

#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include <target/target_core_configfs.h>

#include "target_core_internal.h"
#include "target_core_alua.h"
#include "target_core_pr.h"

#if 0
#include "iscsi/iscsi_target_core.h"
#endif

int core_tmr_alloc_req(
	struct se_cmd *se_cmd,
	void *fabric_tmr_ptr,
	u8 function,
	gfp_t gfp_flags)
{
	struct se_tmr_req *tmr;

	tmr = kzalloc(sizeof(struct se_tmr_req), gfp_flags);
	if (!tmr) {
		pr_err("Unable to allocate struct se_tmr_req\n");
		return -ENOMEM;
	}

	se_cmd->se_cmd_flags |= SCF_SCSI_TMR_CDB;
	se_cmd->se_tmr_req = tmr;
	tmr->task_cmd = se_cmd;
	tmr->fabric_tmr_ptr = fabric_tmr_ptr;
	tmr->function = function;
	INIT_LIST_HEAD(&tmr->tmr_list);

	return 0;
}
EXPORT_SYMBOL(core_tmr_alloc_req);

void core_tmr_release_req(
	struct se_tmr_req *tmr)
{
	struct se_device *dev = tmr->tmr_dev;
	unsigned long flags;

	if (!dev) {
		kfree(tmr);
		return;
	}

	spin_lock_irqsave(&dev->se_tmr_lock, flags);


#if defined(CONFIG_MACH_QNAPTS)
	/* 2014/11/20, adamhsu, redmine 10760 
	 * it shall use list_empty() first before to delete ...
	 */
	if (!list_empty(&tmr->tmr_list))
		list_del_init(&tmr->tmr_list);
#else
	list_del(&tmr->tmr_list);
#endif

	spin_unlock_irqrestore(&dev->se_tmr_lock, flags);

	kfree(tmr);
}

static int target_check_cdb_and_preempt(struct list_head *list,
		struct se_cmd *cmd)
{
	struct t10_pr_registration *reg;

	if (!list)
		return 0;

	list_for_each_entry(reg, list, pr_reg_abort_list) {
		if (reg->pr_res_key == cmd->pr_res_key)
			return 0;
	}

	return 1;
}


#if defined(CONFIG_MACH_QNAPTS)
/* 2014/08/16, adamhsu, redmine 9055,9076,9278 (start) */
static int __check_tmf_i_t_nexus(
	int tmf_code,
	int tas,
	struct se_cmd *se_cmd,
	struct se_node_acl *tmr_nacl
	)
{
	/* 2014/04/14, adamhsu, redmine 7885
	 * Follow the SAM5R11 to modify the code about the iSCSI
	 * doesn't need to reply the TASK ABORTED status for aborted
	 * task after to receive the ABORT TASK of TMF
	 */
	se_cmd->tmf_code = tmf_code;

	if (tmr_nacl && (tmr_nacl != se_cmd->se_sess->se_node_acl)){
		se_cmd->tmf_diff_it_nexus = 1;
		if (tas)
			se_cmd->tmf_resp_tas = 1;
	}
	else {
		se_cmd->tmf_diff_it_nexus = 0;
		se_cmd->tmf_resp_tas = 0;
	}

	pr_debug("LUN_RESET: TMF(0x%x) req comes from %s i_t_nexus\n", 
		tmf_code, ((se_cmd->tmf_diff_it_nexus)? "diff": "same"));

	return 0;
}


static void core_tmr_handle_tas_abort(
	struct se_node_acl *tmr_nacl,
	struct se_cmd *cmd,
	int tas,
	int fe_count,
	int caller
	)
{
	unsigned long flags;
	int exit = 0;


	spin_lock_irqsave(&cmd->t_state_lock, flags);


	/* 2014/10/17, adamhsu */
	pr_info("LUN_RESET(%s it_nexus): cmd(ITT:0x%08x), cdb:0x%2x. "
		"This call comes from %s. duration (ms): %d\n", 
		((cmd->tmf_diff_it_nexus) ? "diff" : "same"),
		cmd->se_tfo->get_task_tag(cmd), cmd->t_task_cdb[0], 
		((caller == 1)? "drain_cmd": "drain_task"),
		jiffies_to_msecs(jiffies - cmd->creation_jiffies));

	pr_debug("LUN_RESET(%s it_nexus): cmd(ITT:0x%08x), i_state:%d, "
		"t_state:%d, transport_state:0x%x, t_task_cdbs_left:%d, "
		"t_task_cdbs_sent:%d\n",
		((cmd->tmf_diff_it_nexus) ? "diff" : "same"), 
		cmd->se_tfo->get_task_tag(cmd),
		cmd->se_tfo->get_cmd_state(cmd), cmd->t_state,	
		cmd->transport_state, atomic_read(&cmd->t_task_cdbs_left),
		atomic_read(&cmd->t_task_cdbs_sent));

	if (!fe_count) {
		cmd->tmf_resp_tas = 0;
		cmd->tmf_diff_it_nexus = 0;
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);

		transport_cmd_finish_abort(cmd, 1);
		return;
	}

	cmd->se_tfo->set_clear_delay_remove(cmd, 1, 0);
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	cmd->scsi_status = SAM_STAT_TASK_ABORTED;
	cmd->se_cmd_flags |= SCF_SENT_DELAYED_TAS;
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);
	cmd->se_tfo->tmf_queue_status(cmd);

	transport_cmd_finish_abort(cmd, 0);
	
}


void core_tmr_abort_task(
	struct se_device *dev,
	struct se_tmr_req *tmr,
	struct se_session *se_sess)
{
	struct se_node_acl *tmr_nacl = NULL;
	struct se_cmd *se_cmd, *tmp_cmd;
	unsigned long flags;

	/* 2014/10/17, adamhsu */
	int ref_tag, tas, to_wait_tasks = 0, recv_got_aborted = 0;

#if 0
	struct se_task *task = NULL, *task_tmp = NULL;
#endif

	spin_lock_irqsave(&se_sess->sess_cmd_lock, flags);
	list_for_each_entry_safe(se_cmd, tmp_cmd,
			&se_sess->sess_cmd_list, se_cmd_list) {

		if (dev != se_cmd->se_dev)
			continue;

		ref_tag = se_cmd->se_tfo->get_task_tag(se_cmd);
		if (tmr->ref_task_tag != ref_tag)
			continue;

		if (tmr && tmr->task_cmd && tmr->task_cmd->se_sess)
			tmr_nacl = tmr->task_cmd->se_sess->se_node_acl;

		/* 2014/10/17, adamhsu */
		pr_info("ABORT_TASK[%s i_t_nexus]: Found referenced %s "
			"task_tag: 0x%8x. duration(ms): %d\n",
			((tmr_nacl && \
			(tmr_nacl != se_cmd->se_sess->se_node_acl)) ? \
			"diff" : "same"), se_cmd->se_tfo->get_fabric_name(),
			ref_tag, 
			jiffies_to_msecs(jiffies - se_cmd->creation_jiffies));

		tas = se_cmd->se_dev->se_sub_dev->se_dev_attrib.emulate_tas;

		if (se_cmd->data_direction == DMA_FROM_DEVICE
		|| se_cmd->data_direction == DMA_NONE
		){
			spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);
			pr_debug("ABORT_TASK[%s i_t_nexus]: skip the ref_tag:0x%8x\n", 
				((tmr_nacl && \
				(tmr_nacl != se_cmd->se_sess->se_node_acl)) ? \
				"diff" : "same"), ref_tag);
			goto _FUNC_COMPLETE_;
		}

		spin_lock_irq(&se_cmd->t_state_lock);

		/* RFC3720, p80,
		 * If the abort request is received and the target can determine
		 * (based on the Referenced Task Tag) that the command was
		 * received and executed and also that the response was sent
		 * prior to the abort, then the target MUST respond with the
		 * response code of "Task Does Not Exist".
		 */

		if (se_cmd->transport_state & CMD_T_SEND_STATUS){
			pr_info("ABORT_TASK[%s i_t_nexus]: ref_tag: "
			"0x%8x %s, queued status already, "
			"to skip it\n",
			((tmr_nacl && \
			(tmr_nacl != se_cmd->se_sess->se_node_acl)) ? \
			"diff" : "same"), ref_tag,
			((se_cmd->t_state == TRANSPORT_COMPLETE) ? \
			"already complete and": ""));

			spin_unlock_irq(&se_cmd->t_state_lock);
			spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);
			goto _TASK_NOT_EXIST_;
		}


		if (se_cmd->t_state == TRANSPORT_WRITE_PENDING
		|| se_cmd->t_state == TRANSPORT_PROCESS_WRITE
		){
			pr_info("ABORT_TASK[%s i_t_nexus]: ref_tag: "
				"0x%8x stay in %s, skip it\n", 
				((tmr_nacl && \
				(tmr_nacl != se_cmd->se_sess->se_node_acl)) ? \
				"diff" : "same"), ref_tag,
				((se_cmd->t_state == TRANSPORT_WRITE_PENDING) \
				? "TRANSPORT_WRITE_PENDING" : \
				"TRANSPORT_PROCESS_WRITE"));

			spin_unlock_irq(&se_cmd->t_state_lock);
			spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);

			/* we are in write processing theread now, so we will 
			 * not handle the TRANSPORT_WRITE_PENDING and
			 * TRANSPORT_PROCESS_WRITE. Just treat them as be complete
			 */
			goto _FUNC_COMPLETE_;

		} else {
			pr_info("neither TRANSPORT_WRITE_PENDING nor "
				"TRANSPORT_PROCESS_WRITE for cmd(ITT:0x%08x), "
				"i_state:%d, t_state:0x%x\n", 
				se_cmd->se_tfo->get_task_tag(se_cmd),
				se_cmd->se_tfo->get_cmd_state(se_cmd), 
				se_cmd->t_state);
		}

		se_cmd->transport_state |= (CMD_T_ABORTED_1 | CMD_T_ABORTED);

		spin_lock(&se_cmd->tmf_data_lock);
		se_cmd->tmf_transport_state |= (CMD_T_ABORTED_1 | CMD_T_ABORTED);
		__check_tmf_i_t_nexus(TMR_ABORT_TASK, tas, se_cmd, tmr_nacl);
		spin_unlock(&se_cmd->tmf_data_lock);

		se_cmd->se_tfo->set_clear_delay_remove(se_cmd, 1, 0);

		pr_info("ABORT_TASK[%s i_t_nexus]: cmd(ITT:0x%08x), "
			"i_state:%d, t_state:%d, transoprt_state:0x%x, "
			"cdb:0x%02x\n", 
			((tmr_nacl && \
			(tmr_nacl != se_cmd->se_sess->se_node_acl)) ? \
			"diff" : "same"), se_cmd->se_tfo->get_task_tag(se_cmd), 
			se_cmd->se_tfo->get_cmd_state(se_cmd), se_cmd->t_state, 
			se_cmd->transport_state, se_cmd->t_task_cdb[0]);

		spin_unlock_irq(&se_cmd->t_state_lock);
		list_del_init(&se_cmd->se_cmd_list);

		/* not call kref_get() for our modfication here */
		spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);

		cancel_work_sync(&se_cmd->work);

		/* The command may NOT be completed now .... For example,
		 * the block i/o use the blk end io to check whteher all tasks
		 * were completed or not. In the other words, it may take long
		 * time before we got all tasks were completed
		 */

		spin_lock_irqsave(&se_cmd->t_state_lock, flags);

		if (se_cmd->t_state != TRANSPORT_COMPLETE){

			/* 2014/10/17, adamhsu, solve race condition symptom 
			 * for task management function 
			 */
			if (se_cmd->transport_state & CMD_T_GOT_ABORTED){
				recv_got_aborted = 1;
				to_wait_tasks = 0;
			} else {
				recv_got_aborted = 0;
				to_wait_tasks = 1;
			}

			pr_info("ABORT_TASK[%s i_t_nexus]: "
				"cmd(ITT:0x%08x), t_state != TRANSPORT_COMPLETE "
				"%s CMD_T_GOT_ABORTED, %s wait all tasks\n", 
				((tmr_nacl && \
				(tmr_nacl != se_cmd->se_sess->se_node_acl)) ? \
				"diff" : "same"), 
				se_cmd->se_tfo->get_task_tag(se_cmd),
				((recv_got_aborted) ? "but GOT ": "and NOT got"),
				((recv_got_aborted) ? "NOT": "to"));

			spin_unlock_irqrestore(&se_cmd->t_state_lock, flags);

			if (to_wait_tasks)
				transport_wait_for_tasks(se_cmd);

			spin_lock_irqsave(&se_cmd->t_state_lock, flags);

		}

#if 0
		list_for_each_entry_safe(task, task_tmp, 
			&se_cmd->t_task_list, t_list){
			pr_info("ABORT_TASK: cmd(ITT:0x%8x), "
				"task->task_flags:0x%x\n", 
				se_cmd->se_tfo->get_task_tag(se_cmd),
				task->task_flags);
		}
#endif

		se_cmd->scsi_status = SAM_STAT_TASK_ABORTED;
		se_cmd->se_cmd_flags |= SCF_SENT_DELAYED_TAS;

		spin_unlock_irqrestore(&se_cmd->t_state_lock, flags);

		se_cmd->se_tfo->tmf_queue_status(se_cmd);

		transport_cmd_finish_abort(se_cmd, 0);
		goto _FUNC_COMPLETE_;
	}
	spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);

_TASK_NOT_EXIST_:
	pr_info("ABORT_TASK: Sending TMR_TASK_DOES_NOT_EXIST for ref_tag: "
		"0x%8x\n", tmr->ref_task_tag);
	tmr->response = TMR_TASK_DOES_NOT_EXIST;
	return;

_FUNC_COMPLETE_:
	pr_info("ABORT_TASK: Sending TMR_FUNCTION_COMPLETE for"
			" ref_tag: 0x%8x\n", ref_tag);
	tmr->response = TMR_FUNCTION_COMPLETE;
	return;

}

static void core_tmr_drain_task_list(
	struct se_device *dev,
	struct se_cmd *prout_cmd,
	struct se_node_acl *tmr_nacl,
	int tas,
	struct list_head *preempt_and_abort_list)
{
	LIST_HEAD(drain_task_list);
	struct se_cmd *cmd;
	struct se_task *task, *task_tmp;
	unsigned long flags;
	int fe_count;

	/*
	 * Complete outstanding struct se_task CDBs with TASK_ABORTED SAM status.
	 * This is following sam4r17, section 5.6 Aborting commands, Table 38
	 * for TMR LUN_RESET:
	 *
	 * a) "Yes" indicates that each command that is aborted on an I_T nexus
	 * other than the one that caused the SCSI device condition is
	 * completed with TASK ABORTED status, if the TAS bit is set to one in
	 * the Control mode page (see SPC-4). "No" indicates that no status is
	 * returned for aborted commands.
	 *
	 * d) If the logical unit reset is caused by a particular I_T nexus
	 * (e.g., by a LOGICAL UNIT RESET task management function), then "yes"
	 * (TASK_ABORTED status) applies.
	 *
	 * Otherwise (e.g., if triggered by a hard reset), "no"
	 * (no TASK_ABORTED SAM status) applies.
	 *
	 * Note that this seems to be independent of TAS (Task Aborted Status)
	 * in the Control Mode Page.
	 */


	/* TODO, adamhsu
	 * Current LUN RESET will not handle the DMA_FROM_DEVICE and any
	 * command works with workqueue. In the other words, it handle the seq
	 * DMA_TO_DEVICE (i.e. solicited data). This shall be modified in the
	 * future
	 */

	spin_lock_irqsave(&dev->execute_task_lock, flags);
	list_for_each_entry_safe(task, task_tmp, &dev->state_task_list,
				t_state_list) {
		if (!task->task_se_cmd) {
			pr_err("task->task_se_cmd is NULL!\n");
			continue;
		}
		cmd = task->task_se_cmd;
	
		/*
		 * For PREEMPT_AND_ABORT usage, only process commands
		 * with a matching reservation key.
		 */
		if (target_check_cdb_and_preempt(preempt_and_abort_list, cmd))
			continue;

		/* Not aborting PROUT PREEMPT_AND_ABORT CDB.. */
		if (prout_cmd == cmd)
			continue;
	
		pr_debug("LUN_RESET[drain task-1]: cmd:0x%p, task:0x%p"
			" ITT:0x%08x, i_state:%d, t_state:%d"
			" cdb: 0x%02x, t_task_cdbs_left:%d,"
			" t_task_cdbs_sent:%d, CMD_T_ACTIVE:%d,"
			" CMD_T_STOP:%d, CMD_T_SENT: %d\n",
			cmd, task, cmd->se_tfo->get_task_tag(cmd),
			cmd->se_tfo->get_cmd_state(cmd), cmd->t_state,
			cmd->t_task_cdb[0], atomic_read(&cmd->t_task_cdbs_left),
			atomic_read(&cmd->t_task_cdbs_sent), 
			(cmd->transport_state & CMD_T_ACTIVE) != 0,
			(cmd->transport_state & CMD_T_STOP) != 0,
			(cmd->transport_state & CMD_T_SENT) != 0);

	
		if (cmd->data_direction == DMA_FROM_DEVICE 
		|| cmd->data_direction == DMA_NONE
		){
			pr_debug("LUN_RESET[drain task-1]: cmd(ITT:0x%8x) "
				"is %s, to skip it\n", 
				cmd->se_tfo->get_task_tag(cmd),
				((cmd->data_direction == DMA_FROM_DEVICE) ? \
				"DMA_FROM_DEVICE" : "DMA_NONE"));
			continue;
		}

#if defined(SUPPORT_CONCURRENT_TASKS)	
		spin_lock(&cmd->wq_lock);
		if (cmd->use_wq == true){
			spin_unlock(&cmd->wq_lock);
			pr_debug("LUN_RESET[drain task-1]: cmd(ITT:0x%8x) "
				"works by wq, to skit it. "
				"data_dir:0x%x, i_state:%d, t_state:%d, "
				"t_task_cdbs_left:%d, t_task_cdbs_sent:%d, "
				"CMD_T_ACTIVE:%d, CMD_T_STOP:%d, "
				"CMD_T_SENT: %d\n", 
				cmd->se_tfo->get_task_tag(cmd), 
				cmd->data_direction, 
				cmd->se_tfo->get_cmd_state(cmd), cmd->t_state,
				atomic_read(&cmd->t_task_cdbs_left),
				atomic_read(&cmd->t_task_cdbs_sent), 
				(cmd->transport_state & CMD_T_ACTIVE) != 0,
				(cmd->transport_state & CMD_T_STOP) != 0,
				(cmd->transport_state & CMD_T_SENT) != 0);
	
	
			continue;
		}
		spin_unlock(&cmd->wq_lock);
#endif	
		spin_lock(&cmd->tmf_t_state_lock);
	
		if (cmd->tmf_t_state == TRANSPORT_WRITE_PENDING
		|| cmd->tmf_t_state == TRANSPORT_PROCESS_WRITE
		|| cmd->tmf_t_state == TRANSPORT_COMPLETE
		)
		{
			pr_debug("LUN_RESET[drain task-1]: found cmd(ITT:0x%8x) "
				"was %s, to skip it. data_dir:0x%x, "
				"i_state:%d, t_state:%d, t_task_cdbs_left:%d, "
				"t_task_cdbs_sent:%d, "
				"CMD_T_ACTIVE:%d, CMD_T_STOP:%d, "
				"CMD_T_SENT: %d\n", 
				cmd->se_tfo->get_task_tag(cmd), 
				((cmd->tmf_t_state == TRANSPORT_COMPLETE) ? \
				"completed": \
				((cmd->tmf_t_state == TRANSPORT_WRITE_PENDING) ? \
				"TRANSPORT_WRITE_PENDING": "TRANSPORT_PROCESS_WRITE")),
				cmd->data_direction,
				cmd->se_tfo->get_cmd_state(cmd), 
				cmd->tmf_t_state,
				atomic_read(&cmd->t_task_cdbs_left),
				atomic_read(&cmd->t_task_cdbs_sent), 
				(cmd->transport_state & CMD_T_ACTIVE) != 0,
				(cmd->transport_state & CMD_T_STOP) != 0,
				(cmd->transport_state & CMD_T_SENT) != 0);


			spin_unlock(&cmd->tmf_t_state_lock);

			continue;
		}
	
		spin_lock(&cmd->tmf_data_lock);
		cmd->tmf_transport_state |= (CMD_T_ABORTED_1 | CMD_T_ABORTED);
		__check_tmf_i_t_nexus(TMR_LUN_RESET, tas, cmd, tmr_nacl);
		spin_unlock(&cmd->tmf_data_lock);
	
		cmd->se_tfo->set_clear_delay_remove(cmd, 1, 0);
	
		list_move_tail(&task->t_state_list, &drain_task_list);
		task->t_state_active = false;
		/*
		 * Remove from task execute list before processing drain_task_list
		 */
		if (!list_empty(&task->t_execute_list))
			__transport_remove_task_from_execute_queue(task, dev);
	}
	spin_unlock_irqrestore(&dev->execute_task_lock, flags);
	
	while (!list_empty(&drain_task_list)) {
		task = list_entry(drain_task_list.next, struct se_task, 
				t_state_list);
		list_del(&task->t_state_list);
		cmd = task->task_se_cmd;
	
		spin_lock_irqsave(&cmd->t_state_lock, flags);
		cmd->transport_state |= (CMD_T_ABORTED_1 | CMD_T_ABORTED);

		pr_debug("LUN RESET[drain task-2]: (before) task->task_flags:"
			"0x%x\n", task->task_flags);

		target_stop_task(task, &flags);
	
		pr_debug("LUN RESET[drain task-2]: (after) task->task_flags:"
			"0x%x\n", task->task_flags);

		if (!atomic_dec_and_test(&cmd->t_task_cdbs_ex_left)) {
			spin_unlock_irqrestore(&cmd->t_state_lock, flags);
			pr_debug("LUN_RESET[drain task-2]: Skipping "
				"task: %p, dev: %p for "
				"t_task_cdbs_ex_left: %d\n", task, dev,
				atomic_read(&cmd->t_task_cdbs_ex_left));
			continue;
		}
		fe_count = atomic_read(&cmd->t_fe_count);
	
		if (!(cmd->transport_state & CMD_T_ACTIVE)) {
			pr_debug("LUN_RESET[drain task-2]: got "
				"CMD_T_ACTIVE for task: %p, "
				"t_fe_count: %d dev: %p\n", task,
				fe_count, dev);
		} else
			pr_debug("LUN_RESET[drain task-2]: Got "
				"!CMD_T_ACTIVE for task: %p,  t_fe_count: %d "
				"dev: %p\n", task, fe_count, dev);
	
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
	
		core_tmr_handle_tas_abort(tmr_nacl, cmd, tas, 
			fe_count, 0);
	
	}
}


static void core_tmr_drain_cmd_list(
	struct se_device *dev,
	struct se_cmd *prout_cmd,
	struct se_node_acl *tmr_nacl,
	int tas,
	struct list_head *preempt_and_abort_list)
{
	LIST_HEAD(drain_cmd_list);
	struct se_queue_obj *qobj = &dev->dev_queue_obj;
	struct se_cmd *cmd, *tcmd;
	unsigned long flags;
	/*
	 * Release all commands remaining in the struct se_device cmd queue.
	 *
	 * This follows the same logic as above for the struct se_device
	 * struct se_task state list, where commands are returned with
	 * TASK_ABORTED status, if there is an outstanding $FABRIC_MOD
	 * reference, otherwise the struct se_cmd is released.
	 */
	spin_lock_irqsave(&qobj->cmd_queue_lock, flags);
	list_for_each_entry_safe(cmd, tcmd, &qobj->qobj_list, se_queue_node) {

		/*
		 * For PREEMPT_AND_ABORT usage, only process commands
		 * with a matching reservation key.
		 */
		if (target_check_cdb_and_preempt(preempt_and_abort_list, cmd))
			continue;
		/*
		 * Not aborting PROUT PREEMPT_AND_ABORT CDB..
		 */
		if (prout_cmd == cmd)
			continue;

		/* 2014/11/20, adamhsu, redmine 10760 */
		if (cmd->se_cmd_flags & SCF_SCSI_TMR_CDB){
			/* TODO:
			 * Not found any information that rfc 3720 describe how
			 * to resopnse the TMF if those TMFs were requested to
			 * be aborted by LUN RESET or TASK ABORT ..., so not
			 * to drain them first
			 */
			pr_info("LUN_RESET[drain cmd]: found TMF (ITT:0x%08x), "
				"to skip it\n", cmd->se_tfo->get_task_tag(cmd));
			continue;
		}

		pr_debug("LUN_RESET[drain cmd]: found cmd(ITT:0x%08x)\n", 
			cmd->se_tfo->get_task_tag(cmd));

		cmd->transport_state |= (CMD_T_ABORTED_1 | CMD_T_ABORTED);
		cmd->transport_state &= ~CMD_T_QUEUED;

		spin_lock(&cmd->tmf_data_lock);
		cmd->tmf_transport_state |= (CMD_T_ABORTED_1 | CMD_T_ABORTED);
		__check_tmf_i_t_nexus(TMR_LUN_RESET, tas, cmd, tmr_nacl);
		spin_unlock(&cmd->tmf_data_lock);

		atomic_dec(&qobj->queue_cnt);
		list_move_tail(&cmd->se_queue_node, &drain_cmd_list);
	}

	spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);

	while (!list_empty(&drain_cmd_list)) {

		cmd = list_entry(drain_cmd_list.next, struct se_cmd, se_queue_node);
		list_del_init(&cmd->se_queue_node);

		pr_debug("LUN_RESET: %s from Device Queue: cmd: %p t_state:"
			" %d t_fe_count: %d, ITT: 0x%8x\n", 
			(preempt_and_abort_list) ? "Preempt" : "", cmd, cmd->t_state,
			atomic_read(&cmd->t_fe_count), 
			cmd->se_tfo->get_task_tag(cmd));

		core_tmr_handle_tas_abort(tmr_nacl, cmd, tas,
			atomic_read(&cmd->t_fe_count), 1);

	}
}
/* 2014/08/16, adamhsu, redmine 9055,9076,9278 (end) */


/* 2014/11/20, adamhsu, redmine 10760 (start) */
static void core_tmr_drain_tmr_list(
	struct se_device *dev,
	struct se_tmr_req *tmr,
	struct list_head *preempt_and_abort_list)
{
	LIST_HEAD(drain_tmr_list);
	struct se_tmr_req *tmr_p, *tmr_pp;
	struct se_cmd *cmd;
	unsigned long flags;

	/*
	 * Release all pending and outgoing TMRs aside from the received
	 * LUN_RESET tmr..
	 */
	spin_lock_irqsave(&dev->se_tmr_lock, flags);
	list_for_each_entry_safe(tmr_p, tmr_pp, &dev->dev_tmr_list, tmr_list) {

		/*
		 * Allow the received TMR to return with FUNCTION_COMPLETE.
		 */
		if (tmr_p == tmr)
			continue;

		cmd = tmr_p->task_cmd;
		if (!cmd) {
			pr_err("Unable to locate struct se_cmd for TMR\n");
			continue;
		}
		
		/*
		 * If this function was called with a valid pr_res_key
		 * parameter (eg: for PROUT PREEMPT_AND_ABORT service action
		 * skip non regisration key matching TMRs.
		 */
		if (target_check_cdb_and_preempt(preempt_and_abort_list, cmd))
			continue;

		spin_lock(&cmd->t_state_lock);
		if (!(cmd->transport_state & CMD_T_ACTIVE)) {
			spin_unlock(&cmd->t_state_lock);
			continue;
		}
		if ((cmd->t_state == TRANSPORT_ISTATE_PROCESSING)
		|| (cmd->t_state == TRANSPORT_PROCESS_TMR)
		)
		{
			/* For current design, if cmd->t_state is TRANSPORT_PROCESS_TMR,
			 * it means the tmf req is still in obj queue NOT going
			 * to transport_processing_thread yet.
			 *
			 * i.e.
			 * transport_generic_handle_tmr() -> transport_add_cmd_to_queue()
			 */
			pr_info("LUN_RESET: ITT:0x%08x, t_state: %d, "
				"to skip it\n", cmd->se_tfo->get_task_tag(cmd),
				cmd->t_state);

			spin_unlock(&cmd->t_state_lock);
			continue;
		}
		spin_unlock(&cmd->t_state_lock);

		list_move_tail(&tmr_p->tmr_list, &drain_tmr_list);
	}
	spin_unlock_irqrestore(&dev->se_tmr_lock, flags);

	list_for_each_entry_safe(tmr_p, tmr_pp, &drain_tmr_list, tmr_list) {
		list_del_init(&tmr_p->tmr_list);
		cmd = tmr_p->task_cmd;

#if defined(CONFIG_MACH_QNAPTS)
		/* 2014/08/16, adamhsu, redmine 9055,9076,9278 */
		/* 2014/10/17, adamhsu */
		pr_info("LUN_RESET: %s releasing TMR %p Function: 0x%02x,"
			" Response: 0x%02x, t_state: %d. duration(ms): %d\n",
			(preempt_and_abort_list) ? "Preempt" : "", tmr_p,
			tmr_p->function, tmr_p->response, cmd->t_state,
			jiffies_to_msecs(jiffies - cmd->creation_jiffies));
#endif

		transport_cmd_finish_abort(cmd, 1);
	}
}
/* 2014/11/20, adamhsu, redmine 10760 (end) */


#else /* !defined(CONFIG_MACH_QNAPTS) */
static void core_tmr_handle_tas_abort(
	struct se_node_acl *tmr_nacl,
	struct se_cmd *cmd,
	int tas,
	int fe_count)
{
	if (!fe_count) {
		transport_cmd_finish_abort(cmd, 1);
		return;
	}

	/* TASK ABORTED status (TAS) bit support */
	if ((tmr_nacl &&
	     (tmr_nacl == cmd->se_sess->se_node_acl)) || tas)
		transport_send_task_abort(cmd);

	transport_cmd_finish_abort(cmd, 0);
}

void core_tmr_abort_task(
	struct se_device *dev,
	struct se_tmr_req *tmr,
	struct se_session *se_sess)
{
	struct se_cmd *se_cmd, *tmp_cmd;
	unsigned long flags;
	int ref_tag;
#if 0
	struct iscsi_session *i_sess;

	printk("ADAM: TMF - cmdsn:0x%x, itt:0x%x\n",
		tmr->task_cmd->cmd_sn, tmr->task_cmd->itt
		);
#endif
	spin_lock_irqsave(&se_sess->sess_cmd_lock, flags);
	list_for_each_entry_safe(se_cmd, tmp_cmd,
			&se_sess->sess_cmd_list, se_cmd_list) {

		if (dev != se_cmd->se_dev)
			continue;

		ref_tag = se_cmd->se_tfo->get_task_tag(se_cmd);
		if (tmr->ref_task_tag != ref_tag)
			continue;

		printk("ABORT_TASK: Found referenced %s task_tag: %d\n",
			se_cmd->se_tfo->get_fabric_name(), ref_tag);

/////////////////////////////////////////
#if 0
		i_sess = (struct iscsi_session *)se_cmd->se_sess->fabric_sess_ptr;

		printk("ADAM: iname:%s\n",i_sess->sess_ops->InitiatorName);
		printk("ADAM: val:0x%x\n", atomic_read(&se_cmd->stop_val));
		printk("ADAM: se_cmd->se_lun:0x%p\n", se_cmd->se_lun);
		printk("ADAM: total ms:%d\n", 
			jiffies_to_msecs(jiffies - se_cmd->cmd_jiffies));
		printk("ADAM: se_cmd->cmdsn:0x%x, se_cmd->itt:0x%x\n",
			se_cmd->cmd_sn, se_cmd->itt
			);


		if (se_cmd->se_lun){
			spin_lock(&se_cmd->se_lun->lun_sep_lock);
			printk("ADAM: se_cmd->se_lun->lun_sep:0x%p\n", se_cmd->se_lun->lun_sep);
			if (se_cmd->se_lun->lun_sep){
				printk("ADAM: wwn:%s\n",
					se_cmd->se_lun->lun_sep->sep_tpg->se_tpg_tfo->tpg_get_wwn(se_cmd->se_lun->lun_sep->sep_tpg));
			}
			spin_unlock(&se_cmd->se_lun->lun_sep_lock);
		}
		printk("ADAM: se_dev:%p, name:%s, cdb[0]:0x%x, ",
			se_cmd->se_dev, se_cmd->se_dev->transport->name,
			se_cmd->t_task_cdb[0]
			);


		
		if (se_cmd->se_dev->transport->is_thin_lun(se_cmd->se_dev))
			printk("lun type: thin\n");
		else
			printk("lun type: thick\n");
#endif
/////////////////////////////////////////

		spin_lock_irq(&se_cmd->t_state_lock);
		if (se_cmd->transport_state & CMD_T_COMPLETE) {
			printk("ABORT_TASK: ref_tag: %d already complete, skipping\n", ref_tag);
			spin_unlock_irq(&se_cmd->t_state_lock);
			spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);
			goto out;
		}
		se_cmd->transport_state |= CMD_T_ABORTED;
		spin_unlock_irq(&se_cmd->t_state_lock);

		list_del_init(&se_cmd->se_cmd_list);
		kref_get(&se_cmd->cmd_kref);
		spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);

		cancel_work_sync(&se_cmd->work);
		transport_wait_for_tasks(se_cmd);

		/*
		 * Now send SAM_STAT_TASK_ABORTED status for the referenced
		 * se_cmd descriptor..
		 */
		transport_send_task_abort(se_cmd);

		/*
		 * Also deal with possible extra acknowledge reference..
		 */
		if (se_cmd->se_cmd_flags & SCF_ACK_KREF)
			target_put_sess_cmd(se_sess, se_cmd);

		target_put_sess_cmd(se_sess, se_cmd);

		printk("ABORT_TASK: Sending TMR_FUNCTION_COMPLETE for"
				" ref_tag: %d\n", ref_tag);
		tmr->response = TMR_FUNCTION_COMPLETE;
		return;
	}
	spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);

out:
	printk("ABORT_TASK: Sending TMR_TASK_DOES_NOT_EXIST for ref_tag: %d\n",
			tmr->ref_task_tag);
	tmr->response = TMR_TASK_DOES_NOT_EXIST;
}


static void core_tmr_drain_task_list(
	struct se_device *dev,
	struct se_cmd *prout_cmd,
	struct se_node_acl *tmr_nacl,
	int tas,
	struct list_head *preempt_and_abort_list)
{
	LIST_HEAD(drain_task_list);
	struct se_cmd *cmd;
	struct se_task *task, *task_tmp;
	unsigned long flags;
	int fe_count;
	/*
	 * Complete outstanding struct se_task CDBs with TASK_ABORTED SAM status.
	 * This is following sam4r17, section 5.6 Aborting commands, Table 38
	 * for TMR LUN_RESET:
	 *
	 * a) "Yes" indicates that each command that is aborted on an I_T nexus
	 * other than the one that caused the SCSI device condition is
	 * completed with TASK ABORTED status, if the TAS bit is set to one in
	 * the Control mode page (see SPC-4). "No" indicates that no status is
	 * returned for aborted commands.
	 *
	 * d) If the logical unit reset is caused by a particular I_T nexus
	 * (e.g., by a LOGICAL UNIT RESET task management function), then "yes"
	 * (TASK_ABORTED status) applies.
	 *
	 * Otherwise (e.g., if triggered by a hard reset), "no"
	 * (no TASK_ABORTED SAM status) applies.
	 *
	 * Note that this seems to be independent of TAS (Task Aborted Status)
	 * in the Control Mode Page.
	 */
	spin_lock_irqsave(&dev->execute_task_lock, flags);
	list_for_each_entry_safe(task, task_tmp, &dev->state_task_list,
				t_state_list) {
		if (!task->task_se_cmd) {
			pr_err("task->task_se_cmd is NULL!\n");
			continue;
		}
		cmd = task->task_se_cmd;

		/*
		 * For PREEMPT_AND_ABORT usage, only process commands
		 * with a matching reservation key.
		 */
		if (target_check_cdb_and_preempt(preempt_and_abort_list, cmd))
			continue;
		/*
		 * Not aborting PROUT PREEMPT_AND_ABORT CDB..
		 */
		if (prout_cmd == cmd)
			continue;

		list_move_tail(&task->t_state_list, &drain_task_list);
		task->t_state_active = false;

		/*
		 * Remove from task execute list before processing drain_task_list
		 */
		if (!list_empty(&task->t_execute_list))
			__transport_remove_task_from_execute_queue(task, dev);
	}
	spin_unlock_irqrestore(&dev->execute_task_lock, flags);


	while (!list_empty(&drain_task_list)) {
		task = list_entry(drain_task_list.next, struct se_task, t_state_list);
		list_del(&task->t_state_list);
		cmd = task->task_se_cmd;

		pr_debug("LUN_RESET: %s cmd: %p task: %p"
			" ITT/CmdSN: 0x%08x/0x%08x, i_state: %d, t_state: %d"
			" cdb: 0x%02x\n",
			(preempt_and_abort_list) ? "Preempt" : "", cmd, task,
			cmd->se_tfo->get_task_tag(cmd), 0,
			cmd->se_tfo->get_cmd_state(cmd), cmd->t_state,
			cmd->t_task_cdb[0]);

		pr_debug("LUN_RESET: ITT[0x%08x] - pr_res_key: 0x%016Lx"
			" t_task_cdbs: %d t_task_cdbs_left: %d"
			" t_task_cdbs_sent: %d -- CMD_T_ACTIVE: %d"
			" CMD_T_STOP: %d CMD_T_SENT: %d\n",
			cmd->se_tfo->get_task_tag(cmd), cmd->pr_res_key,
			cmd->t_task_list_num,
			atomic_read(&cmd->t_task_cdbs_left),
			atomic_read(&cmd->t_task_cdbs_sent),
			(cmd->transport_state & CMD_T_ACTIVE) != 0,
			(cmd->transport_state & CMD_T_STOP) != 0,
			(cmd->transport_state & CMD_T_SENT) != 0);

		/*
		 * If the command may be queued onto a workqueue cancel it now.
		 *
		 * This is equivalent to removal from the execute queue in the
		 * loop above, but we do it down here given that
		 * cancel_work_sync may block.
		 */
		if (cmd->t_state == TRANSPORT_COMPLETE)
			cancel_work_sync(&cmd->work);

		spin_lock_irqsave(&cmd->t_state_lock, flags);
		target_stop_task(task, &flags);

		if (!atomic_dec_and_test(&cmd->t_task_cdbs_ex_left)) {
			spin_unlock_irqrestore(&cmd->t_state_lock, flags);
			pr_debug("LUN_RESET: Skipping task: %p, dev: %p for"
				" t_task_cdbs_ex_left: %d\n", task, dev,
				atomic_read(&cmd->t_task_cdbs_ex_left));
			continue;
		}
		fe_count = atomic_read(&cmd->t_fe_count);

		if (!(cmd->transport_state & CMD_T_ACTIVE)) {
			pr_debug("LUN_RESET: got CMD_T_ACTIVE for"
				" task: %p, t_fe_count: %d dev: %p\n", task,
				fe_count, dev);
			cmd->transport_state |= CMD_T_ABORTED;

			spin_unlock_irqrestore(&cmd->t_state_lock, flags);
			core_tmr_handle_tas_abort(tmr_nacl, cmd, tas, fe_count);
			continue;
		}

		pr_debug("LUN_RESET: Got !CMD_T_ACTIVE for task: %p,"
			" t_fe_count: %d dev: %p\n", task, fe_count, dev);
		cmd->transport_state |= CMD_T_ABORTED;
		core_tmr_handle_tas_abort(tmr_nacl, cmd, tas, fe_count);

	}
}


static void core_tmr_drain_cmd_list(
	struct se_device *dev,
	struct se_cmd *prout_cmd,
	struct se_node_acl *tmr_nacl,
	int tas,
	struct list_head *preempt_and_abort_list)
{
	LIST_HEAD(drain_cmd_list);
	struct se_queue_obj *qobj = &dev->dev_queue_obj;
	struct se_cmd *cmd, *tcmd;
	unsigned long flags;
	/*
	 * Release all commands remaining in the struct se_device cmd queue.
	 *
	 * This follows the same logic as above for the struct se_device
	 * struct se_task state list, where commands are returned with
	 * TASK_ABORTED status, if there is an outstanding $FABRIC_MOD
	 * reference, otherwise the struct se_cmd is released.
	 */
	spin_lock_irqsave(&qobj->cmd_queue_lock, flags);
	list_for_each_entry_safe(cmd, tcmd, &qobj->qobj_list, se_queue_node) {

		/*
		 * For PREEMPT_AND_ABORT usage, only process commands
		 * with a matching reservation key.
		 */
		if (target_check_cdb_and_preempt(preempt_and_abort_list, cmd))
			continue;
		/*
		 * Not aborting PROUT PREEMPT_AND_ABORT CDB..
		 */
		if (prout_cmd == cmd)
			continue;

		cmd->transport_state &= ~CMD_T_QUEUED;
		atomic_dec(&qobj->queue_cnt);
		list_move_tail(&cmd->se_queue_node, &drain_cmd_list);
	}

	spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);

	while (!list_empty(&drain_cmd_list)) {

		cmd = list_entry(drain_cmd_list.next, struct se_cmd, se_queue_node);
		list_del_init(&cmd->se_queue_node);

		pr_debug("LUN_RESET: %s from Device Queue: cmd: %p t_state:"
			" %d t_fe_count: %d\n", (preempt_and_abort_list) ?
			"Preempt" : "", cmd, cmd->t_state,
			atomic_read(&cmd->t_fe_count));

		core_tmr_handle_tas_abort(tmr_nacl, cmd, tas,
				atomic_read(&cmd->t_fe_count));

	}
}



static void core_tmr_drain_tmr_list(
	struct se_device *dev,
	struct se_tmr_req *tmr,
	struct list_head *preempt_and_abort_list)
{
	LIST_HEAD(drain_tmr_list);
	struct se_tmr_req *tmr_p, *tmr_pp;
	struct se_cmd *cmd;
	unsigned long flags;

	/*
	 * Release all pending and outgoing TMRs aside from the received
	 * LUN_RESET tmr..
	 */
	spin_lock_irqsave(&dev->se_tmr_lock, flags);
	list_for_each_entry_safe(tmr_p, tmr_pp, &dev->dev_tmr_list, tmr_list) {

		/*
		 * Allow the received TMR to return with FUNCTION_COMPLETE.
		 */
		if (tmr_p == tmr)
			continue;

		cmd = tmr_p->task_cmd;
		if (!cmd) {
			pr_err("Unable to locate struct se_cmd for TMR\n");
			continue;
		}
		
		/*
		 * If this function was called with a valid pr_res_key
		 * parameter (eg: for PROUT PREEMPT_AND_ABORT service action
		 * skip non regisration key matching TMRs.
		 */
		if (target_check_cdb_and_preempt(preempt_and_abort_list, cmd))
			continue;

		spin_lock(&cmd->t_state_lock);
		if (!(cmd->transport_state & CMD_T_ACTIVE)) {
			spin_unlock(&cmd->t_state_lock);
			continue;
		}
		if (cmd->t_state == TRANSPORT_ISTATE_PROCESSING) {
			spin_unlock(&cmd->t_state_lock);
			continue;
		}
		spin_unlock(&cmd->t_state_lock);

		list_move_tail(&tmr_p->tmr_list, &drain_tmr_list);
	}
	spin_unlock_irqrestore(&dev->se_tmr_lock, flags);

	list_for_each_entry_safe(tmr_p, tmr_pp, &drain_tmr_list, tmr_list) {
		list_del_init(&tmr_p->tmr_list);
		cmd = tmr_p->task_cmd;
		transport_cmd_finish_abort(cmd, 1);
	}
}
#endif


int core_tmr_lun_reset(
        struct se_device *dev,
        struct se_tmr_req *tmr,
        struct list_head *preempt_and_abort_list,
        struct se_cmd *prout_cmd)
{
	struct se_node_acl *tmr_nacl = NULL;
	struct se_portal_group *tmr_tpg = NULL;
	int tas;

        /*
	 * TASK_ABORTED status bit, this is configurable via ConfigFS
	 * struct se_device attributes.  spc4r17 section 7.4.6 Control mode page
	 *
	 * A task aborted status (TAS) bit set to zero specifies that aborted
	 * tasks shall be terminated by the device server without any response
	 * to the application client. A TAS bit set to one specifies that tasks
	 * aborted by the actions of an I_T nexus other than the I_T nexus on
	 * which the command was received shall be completed with TASK ABORTED
	 * status (see SAM-4).
	 */
	tas = dev->se_sub_dev->se_dev_attrib.emulate_tas;

	/*
	 * Determine if this se_tmr is coming from a $FABRIC_MOD
	 * or struct se_device passthrough..
	 */
	if (tmr && tmr->task_cmd && tmr->task_cmd->se_sess) {
		tmr_nacl = tmr->task_cmd->se_sess->se_node_acl;
		tmr_tpg = tmr->task_cmd->se_sess->se_tpg;
		if (tmr_nacl && tmr_tpg) {
			pr_debug("LUN_RESET: TMR caller fabric: %s"
				" initiator port %s\n",
				tmr_tpg->se_tpg_tfo->get_fabric_name(),
				tmr_nacl->initiatorname);
		}
	}
	pr_debug("LUN_RESET: %s starting for [%s], tas: %d\n",
		(preempt_and_abort_list) ? "Preempt" : "TMR",
		dev->transport->name, tas);

	core_tmr_drain_tmr_list(dev, tmr, preempt_and_abort_list);

	core_tmr_drain_task_list(dev, prout_cmd, tmr_nacl, tas,
				preempt_and_abort_list);

	core_tmr_drain_cmd_list(dev, prout_cmd, tmr_nacl, tas,
				preempt_and_abort_list);
	/*
	 * Clear any legacy SPC-2 reservation when called during
	 * LOGICAL UNIT RESET
	 */
	if (!preempt_and_abort_list &&
	     (dev->dev_flags & DF_SPC2_RESERVATIONS)) {
		spin_lock(&dev->dev_reservation_lock);
		dev->dev_reserved_node_acl = NULL;
		dev->dev_flags &= ~DF_SPC2_RESERVATIONS;
		spin_unlock(&dev->dev_reservation_lock);
		pr_debug("LUN_RESET: SCSI-2 Released reservation\n");
	}

	spin_lock_irq(&dev->stats_lock);
	dev->num_resets++;
	spin_unlock_irq(&dev->stats_lock);

	pr_debug("LUN_RESET: %s for [%s] Complete\n",
			(preempt_and_abort_list) ? "Preempt" : "TMR",
			dev->transport->name);
	return 0;
}


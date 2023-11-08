/*******************************************************************************
 * Filename:  target_core_transport.c
 *
 * This file contains the Generic Target Engine Core.
 *
 * Copyright (c) 2002, 2003, 2004, 2005 PyX Technologies, Inc.
 * Copyright (c) 2005, 2006, 2007 SBE, Inc.
 * Copyright (c) 2007-2010 Rising Tide Systems
 * Copyright (c) 2008-2010 Linux-iSCSI.org
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

#include <linux/net.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/in.h>
#include <linux/cdrom.h>
#include <linux/module.h>
#include <linux/ratelimit.h>
#include <asm/unaligned.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_tcq.h>

#if defined(CONFIG_MACH_QNAPTS)
/* 2014/06/14, adamhsu, redmine 8530 (start) */
#include <linux/fs.h>
/* 2014/06/14, adamhsu, redmine 8530 (end) */
#endif

#include <target/target_core_base.h>
#include <target/target_core_backend.h>
#include <target/target_core_fabric.h>
#include <target/target_core_configfs.h>

#include "target_core_internal.h"
#include "target_core_alua.h"
#include "target_core_pr.h"
#include "target_core_ua.h"

#if defined(CONFIG_MACH_QNAPTS)
#include "target_core_ua.h"
#include "target_core_iblock.h"
#include "target_core_file.h"
#include "vaai_target_struc.h"
#include "target_general.h"
#include "target_core_extern.h"

#if defined(SUPPORT_FAST_BLOCK_CLONE)
#include "target_fast_clone.h"
#endif
#if defined(SUPPORT_VAAI) //Benjamin 20121105 sync VAAI support from Adam. 
#include "vaai_helper.h"
#endif
#if defined(SUPPORT_TPC_CMD)
#include "tpc_helper.h"
#endif
#if defined(SUPPORT_TP)
#include "tp_def.h"
#endif

#if defined(SUPPORT_CONCURRENT_TASKS)
// 20130628, Jonathan Ho, add workqueue for executing iscsi tasks concurrently
static struct workqueue_struct *multi_tasks_wq;
#endif
#endif /* defined(CONFIG_MACH_QNAPTS) */

//
//
//
static int sub_api_initialized;

static struct workqueue_struct *target_completion_wq;
static struct kmem_cache *se_sess_cache;
struct kmem_cache *se_ua_cache;
struct kmem_cache *t10_pr_reg_cache;
struct kmem_cache *t10_alua_lu_gp_cache;
struct kmem_cache *t10_alua_lu_gp_mem_cache;
struct kmem_cache *t10_alua_tg_pt_gp_cache;
struct kmem_cache *t10_alua_tg_pt_gp_mem_cache;


#if defined(SUPPORT_PARALLEL_TASK_WQ)
#include <linux/list.h>
static void p_task_work_func(struct work_struct *work);
static int p_task_thread(void *param);
int p_task_remove_relationship_with_other_task(
    struct se_task *r_task, 
    struct se_device *se_dev
    );
#endif

static int transport_generic_write_pending(struct se_cmd *);
static int transport_processing_thread(void *param);
static int __transport_execute_tasks(struct se_device *dev, struct se_cmd *);
static void transport_complete_task_attr(struct se_cmd *cmd);
static void transport_handle_queue_full(struct se_cmd *cmd,
		struct se_device *dev);
static void transport_free_dev_tasks(struct se_cmd *cmd);
static int transport_generic_get_mem(struct se_cmd *cmd);
static void transport_put_cmd(struct se_cmd *cmd);
static void transport_remove_cmd_from_queue(struct se_cmd *cmd);
static int transport_set_sense_codes(struct se_cmd *cmd, u8 asc, u8 ascq);
static void target_complete_ok_work(struct work_struct *work);

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_CONCURRENT_TASKS)
static void transport_execute_tasks_wq(struct work_struct *work); // 20130628, Jonathan Ho, add workqueue for executing iscsi tasks concurrently
#endif

/* adamhsu, 
 * (1) redmine bug 6915 - bugzilla 40743
 * (2) redmine bug 6916 - bugzilla 41183
 */
static void __create_cmd_rec(LIO_SE_CMD *se_cmd);
static void __remove_cmd_rec(LIO_SE_CMD *se_cmd);

/* 2014/01/13, support verify(10),(16) for HCK 2.1 (ver:8.100.26063) */
extern int __verify_10_16(LIO_SE_TASK * se_task);
#endif


/* decalre the global data here */
#if defined(CONFIG_MACH_QNAPTS)

/* table to record the error reason */
ERR_REASON_TABLE gErrReasonTable[MAX_ERR_REASON_INDEX] = {
	// 0 - ERR_UNKNOWN_SAM_OPCODE
	{TCM_UNSUPPORTED_SCSI_OPCODE, "Unsupported scsi opcode"},
	// 1 - ERR_REQ_TOO_MANY_SECTORS
	{TCM_SECTOR_COUNT_TOO_MANY, "Too many sectors"},
	// 2 - ERR_INVALID_CDB_FIELD
	{TCM_INVALID_CDB_FIELD, "Invalid cdb field"},
	// 3 - ERR_INVALID_PARAMETER_LIST
	{TCM_INVALID_PARAMETER_LIST, "Invalid parameter list"},
	// 4 - ERR_LOGICAL_UNIT_COMMUNICATION_FAILURE
	{TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE, "LU communication failure"},
	// 5 - ERR_UNKNOWN_MODE_PAGE
	{TCM_UNKNOWN_MODE_PAGE, "Unknown mode page"},
	// 6 - ERR_WRITE_PROTECTEDS
	{TCM_WRITE_PROTECTED, "Write protected"},
	// 7 - ERR_RESERVATION_CONFLICT
	{TCM_RESERVATION_CONFLICT, "Reservation conflict"},
	// 8 - ERR_ILLEGAL_REQUEST
	{TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE, "Illegal request"},
	// 9 - ERR_CHECK_CONDITION_NOT_READY
	{TCM_CHECK_CONDITION_NOT_READY , "Check Condition: Not ready"},  
	// 10 - ERR_LBA_OUT_OF_RANGE
	{TCM_ADDRESS_OUT_OF_RANGE, "LBA out of range"},
	// 11 - ERR_MISCOMPARE_DURING_VERIFY_OP
	{TCM_MISCOMPARE_DURING_VERIFY_OP, "Miscompare during to verify"},
	// 12 - ERR_PARAMETER_LIST_LEN_ERROR
	{TCM_PARAMETER_LIST_LEN_ERROR, "Parameter list len error"},
	// 13 - ERR_UNREACHABLE_COPY_TARGET
	{TCM_UNREACHABLE_COPY_TARGET, "Unreachable copy target"},
	// 14 - ERR_3RD_PARTY_DEVICE_FAILURE
	{TCM_3RD_PARTY_DEVICE_FAILURE, "Third-party device failure"},
	// 15 - ERR_INCORRECT_COPY_TARGET_DEV_TYPE
	{TCM_INCORRECT_COPY_TARGET_DEV_TYPE, "Incorrect copy target dev type"},
	// 16 - ERR_TOO_MANY_TARGET_DESCRIPTORS
	{TCM_TOO_MANY_TARGET_DESCRIPTORS, "Too many target descs"},
	// 17 - ERR_TOO_MANY_SEGMENT_DESCRIPTORS
	{TCM_TOO_MANY_SEGMENT_DESCRIPTORS, "Too many segment descs"},
	// 18 - ERR_ILLEGAL_REQ_DATA_OVERRUN_COPY_TARGET
	{TCM_ILLEGAL_REQ_DATA_OVERRUN_COPY_TARGET, 
		"Illegal req for data overrun copy target"},
	// 19 - ERR_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET
	{TCM_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET,
		"Illegal req for data underrun copy target"},
	// 20 - ERR_COPY_ABORT_DATA_OVERRUN_COPY_TARGET
	{TCM_COPY_ABORT_DATA_OVERRUN_COPY_TARGET,  
		"Copy abort for data overrun copy target"},
	// 21 - ERR_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET
	{TCM_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET, 
		"Copy abort for data underrun copy target"},
	// 22 - ERR_INSUFFICIENT_RESOURCES
	{TCM_INSUFFICIENT_RESOURCES, "Insufficient resources"},
	// 23 - ERR_INSUFFICIENT_RESOURCES_TO_CREATE_ROD
	{TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD, 
		"Insufficient resources to create ROD"},
	// 24 - ERR_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN
	{TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN, 
		"Insufficient resources to create ROD TOKEN"},
	// 25 - ERR_OPERATION_IN_PROGRESS
	{TCM_OPERATION_IN_PROGRESS, "Operation is in progress"},		     
	// 26 - ERR_INVALID_TOKEN_OP_AND_INVALID_TOKEN_LEN
	{TCM_INVALID_TOKEN_OP_AND_INVALID_TOKEN_LEN,     
		"Invalid token op for invalid token len"},
	// 27 - ERR_INVALID_TOKEN_OP_AND_CAUSE_NOT_REPORTABLE
	{TCM_INVALID_TOKEN_OP_AND_CAUSE_NOT_REPORTABLE, 
		"Invalid token op for not reportable"},
	// 28 - ERR_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_CREATION_NOT_SUPPORTED
	{TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_CREATION_NOT_SUPPORTED,
		"Invalid token op for remote rod token creation is not supported"},
	// 29 - ERR_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_USAGE_NOT_SUPPORTED
	{TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_USAGE_NOT_SUPPORTED, 
		"Invalid token op for remote rod token usage is not supported"},
	// 30 - ERR_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED
	{TCM_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED, 
		"Invalid token op for token was cancelled"},
	// 31 - ERR_INVALID_TOKEN_OP_AND_TOKEN_CORRUPT
	{TCM_INVALID_TOKEN_OP_AND_TOKEN_CORRUPT, 
		"Invalid token op for token was corrupted"},
	// 32 - ERR_INVALID_TOKEN_OP_AND_TOKEN_DELETED
	{TCM_INVALID_TOKEN_OP_AND_TOKEN_DELETED, 
		"Invalid token op for token was deleted"},
	// 33 - ERR_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED
	{TCM_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED, 
		"Invalid token op for token was expired"},
	// 34 - ERR_INVALID_TOKEN_OP_AND_TOKEN_REVOKED
	{TCM_INVALID_TOKEN_OP_AND_TOKEN_REVOKED, 
		"Invalid token op for token is revoked"},
	// 35 - ERR_INVALID_TOKEN_OP_AND_TOKEN_UNKNOWN
	{TCM_INVALID_TOKEN_OP_AND_TOKEN_UNKNOWN,  
		"Invalid token op for token is unknown"},
	// 36 - ERR_INVALID_TOKEN_OP_AND_UNSUPPORTED_TOKEN_TYPE
	{TCM_INVALID_TOKEN_OP_AND_UNSUPPORTED_TOKEN_TYPE, 
		"Invalid token op for unsupported token"},
	// 37 - ERR_NO_SPACE_WRITE_PROTECT
	{TCM_SPACE_ALLOCATION_FAILED_WRITE_PROTECT, 
		"Space allocation fail with write protect"},
	// 38 - ERR_OUT_OF_RESOURCES
	{TCM_OUT_OF_RESOURCES, "Out of resources"},
};
#endif


/**/
int init_se_kmem_caches(void)
{
	se_sess_cache = kmem_cache_create("se_sess_cache",
			sizeof(struct se_session), __alignof__(struct se_session),
			0, NULL);
	if (!se_sess_cache) {
		pr_err("kmem_cache_create() for struct se_session"
				" failed\n");
		goto out;
	}
	se_ua_cache = kmem_cache_create("se_ua_cache",
			sizeof(struct se_ua), __alignof__(struct se_ua),
			0, NULL);
	if (!se_ua_cache) {
		pr_err("kmem_cache_create() for struct se_ua failed\n");
		goto out_free_sess_cache;
	}
	t10_pr_reg_cache = kmem_cache_create("t10_pr_reg_cache",
			sizeof(struct t10_pr_registration),
			__alignof__(struct t10_pr_registration), 0, NULL);
	if (!t10_pr_reg_cache) {
		pr_err("kmem_cache_create() for struct t10_pr_registration"
				" failed\n");
		goto out_free_ua_cache;
	}
	t10_alua_lu_gp_cache = kmem_cache_create("t10_alua_lu_gp_cache",
			sizeof(struct t10_alua_lu_gp), __alignof__(struct t10_alua_lu_gp),
			0, NULL);
	if (!t10_alua_lu_gp_cache) {
		pr_err("kmem_cache_create() for t10_alua_lu_gp_cache"
				" failed\n");
		goto out_free_pr_reg_cache;
	}
	t10_alua_lu_gp_mem_cache = kmem_cache_create("t10_alua_lu_gp_mem_cache",
			sizeof(struct t10_alua_lu_gp_member),
			__alignof__(struct t10_alua_lu_gp_member), 0, NULL);
	if (!t10_alua_lu_gp_mem_cache) {
		pr_err("kmem_cache_create() for t10_alua_lu_gp_mem_"
				"cache failed\n");
		goto out_free_lu_gp_cache;
	}
	t10_alua_tg_pt_gp_cache = kmem_cache_create("t10_alua_tg_pt_gp_cache",
			sizeof(struct t10_alua_tg_pt_gp),
			__alignof__(struct t10_alua_tg_pt_gp), 0, NULL);
	if (!t10_alua_tg_pt_gp_cache) {
		pr_err("kmem_cache_create() for t10_alua_tg_pt_gp_"
				"cache failed\n");
		goto out_free_lu_gp_mem_cache;
	}
	t10_alua_tg_pt_gp_mem_cache = kmem_cache_create(
			"t10_alua_tg_pt_gp_mem_cache",
			sizeof(struct t10_alua_tg_pt_gp_member),
			__alignof__(struct t10_alua_tg_pt_gp_member),
			0, NULL);
	if (!t10_alua_tg_pt_gp_mem_cache) {
		pr_err("kmem_cache_create() for t10_alua_tg_pt_gp_"
				"mem_t failed\n");
		goto out_free_tg_pt_gp_cache;
	}

	target_completion_wq = alloc_workqueue("target_completion",
					       WQ_MEM_RECLAIM, 0);
	if (!target_completion_wq)
		goto out_free_tg_pt_gp_mem_cache;

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_CONCURRENT_TASKS)
	// 20130628, Jonathan Ho, add workqueue for executing iscsi tasks concurrently
	multi_tasks_wq = alloc_workqueue("multi_tasks_wq",
        (WQ_MEM_RECLAIM | WQ_UNBOUND | WQ_NON_REENTRANT), 0);

	if (!multi_tasks_wq) {
		destroy_workqueue(target_completion_wq);
		goto out_free_tg_pt_gp_mem_cache;
	}
#endif
#endif

	return 0;

out_free_tg_pt_gp_mem_cache:
	kmem_cache_destroy(t10_alua_tg_pt_gp_mem_cache);
out_free_tg_pt_gp_cache:
	kmem_cache_destroy(t10_alua_tg_pt_gp_cache);
out_free_lu_gp_mem_cache:
	kmem_cache_destroy(t10_alua_lu_gp_mem_cache);
out_free_lu_gp_cache:
	kmem_cache_destroy(t10_alua_lu_gp_cache);
out_free_pr_reg_cache:
	kmem_cache_destroy(t10_pr_reg_cache);
out_free_ua_cache:
	kmem_cache_destroy(se_ua_cache);
out_free_sess_cache:
	kmem_cache_destroy(se_sess_cache);
out:
	return -ENOMEM;
}

void release_se_kmem_caches(void)
{
#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_CONCURRENT_TASKS)
	destroy_workqueue(multi_tasks_wq); // 20130628, Jonathan Ho, add workqueue for executing iscsi tasks concurrently
#endif
#endif
	destroy_workqueue(target_completion_wq);
	kmem_cache_destroy(se_sess_cache);
	kmem_cache_destroy(se_ua_cache);
	kmem_cache_destroy(t10_pr_reg_cache);
	kmem_cache_destroy(t10_alua_lu_gp_cache);
	kmem_cache_destroy(t10_alua_lu_gp_mem_cache);
	kmem_cache_destroy(t10_alua_tg_pt_gp_cache);
	kmem_cache_destroy(t10_alua_tg_pt_gp_mem_cache);
}

/* This code ensures unique mib indexes are handed out. */
static DEFINE_SPINLOCK(scsi_mib_index_lock);
static u32 scsi_mib_index[SCSI_INDEX_TYPE_MAX];

/*
 * Allocate a new row index for the entry type specified
 */
u32 scsi_get_new_index(scsi_index_t type)
{
	u32 new_index;

	BUG_ON((type < 0) || (type >= SCSI_INDEX_TYPE_MAX));

	spin_lock(&scsi_mib_index_lock);
	new_index = ++scsi_mib_index[type];
	spin_unlock(&scsi_mib_index_lock);

	return new_index;
}

static void transport_init_queue_obj(struct se_queue_obj *qobj)
{
	atomic_set(&qobj->queue_cnt, 0);
	INIT_LIST_HEAD(&qobj->qobj_list);
	init_waitqueue_head(&qobj->thread_wq);
	spin_lock_init(&qobj->cmd_queue_lock);
}

void transport_subsystem_check_init(void)
{
	int ret;

	if (sub_api_initialized)
		return;

	ret = request_module("target_core_iblock");
	if (ret != 0)
		pr_err("Unable to load target_core_iblock\n");

	ret = request_module("target_core_file");
	if (ret != 0)
		pr_err("Unable to load target_core_file\n");

	ret = request_module("target_core_pscsi");
	if (ret != 0)
		pr_err("Unable to load target_core_pscsi\n");

	ret = request_module("target_core_stgt");
	if (ret != 0)
		pr_err("Unable to load target_core_stgt\n");

	sub_api_initialized = 1;
	return;
}

struct se_session *transport_init_session(void)
{
	struct se_session *se_sess;

	se_sess = kmem_cache_zalloc(se_sess_cache, GFP_KERNEL);
	if (!se_sess) {
		pr_err("Unable to allocate struct se_session from"
				" se_sess_cache\n");
		return ERR_PTR(-ENOMEM);
	}
	INIT_LIST_HEAD(&se_sess->sess_list);
	INIT_LIST_HEAD(&se_sess->sess_acl_list);
	INIT_LIST_HEAD(&se_sess->sess_cmd_list);
	INIT_LIST_HEAD(&se_sess->sess_wait_list);
	spin_lock_init(&se_sess->sess_cmd_lock);

#if defined(CONFIG_MACH_QNAPTS)
#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
	transport_init_tag_pool(se_sess);
#endif

#endif
	kref_init(&se_sess->sess_kref);

	return se_sess;
}
EXPORT_SYMBOL(transport_init_session);

/*
 * Called with spin_lock_irqsave(&struct se_portal_group->session_lock called.
 */
void __transport_register_session(
	struct se_portal_group *se_tpg,
	struct se_node_acl *se_nacl,
	struct se_session *se_sess,
	void *fabric_sess_ptr)
{
	unsigned char buf[PR_REG_ISID_LEN];

	se_sess->se_tpg = se_tpg;
	se_sess->fabric_sess_ptr = fabric_sess_ptr;
	/*
	 * Used by struct se_node_acl's under ConfigFS to locate active se_session-t
	 *
	 * Only set for struct se_session's that will actually be moving I/O.
	 * eg: *NOT* discovery sessions.
	 */
	if (se_nacl) {
		/*
		 * If the fabric module supports an ISID based TransportID,
		 * save this value in binary from the fabric I_T Nexus now.
		 */
		if (se_tpg->se_tpg_tfo->sess_get_initiator_sid != NULL) {
			memset(&buf[0], 0, PR_REG_ISID_LEN);
			se_tpg->se_tpg_tfo->sess_get_initiator_sid(se_sess,
					&buf[0], PR_REG_ISID_LEN);
			se_sess->sess_bin_isid = get_unaligned_be64(&buf[0]);
		}
		kref_get(&se_nacl->acl_kref);

		spin_lock_irq(&se_nacl->nacl_sess_lock);
		/*
		 * The se_nacl->nacl_sess pointer will be set to the
		 * last active I_T Nexus for each struct se_node_acl.
		 */
		se_nacl->nacl_sess = se_sess;

		list_add_tail(&se_sess->sess_acl_list,
			      &se_nacl->acl_sess_list);
		spin_unlock_irq(&se_nacl->nacl_sess_lock);
	}
	list_add_tail(&se_sess->sess_list, &se_tpg->tpg_sess_list);

	pr_debug("TARGET_CORE[%s]: Registered fabric_sess_ptr: %p\n",
		se_tpg->se_tpg_tfo->get_fabric_name(), se_sess->fabric_sess_ptr);
}
EXPORT_SYMBOL(__transport_register_session);

void transport_register_session(
	struct se_portal_group *se_tpg,
	struct se_node_acl *se_nacl,
	struct se_session *se_sess,
	void *fabric_sess_ptr)
{
	unsigned long flags;

	spin_lock_irqsave(&se_tpg->session_lock, flags);
	__transport_register_session(se_tpg, se_nacl, se_sess, fabric_sess_ptr);
	spin_unlock_irqrestore(&se_tpg->session_lock, flags);
}
EXPORT_SYMBOL(transport_register_session);

static void target_release_session(struct kref *kref)
{
	struct se_session *se_sess = container_of(kref,
			struct se_session, sess_kref);
	struct se_portal_group *se_tpg = se_sess->se_tpg;

	se_tpg->se_tpg_tfo->close_session(se_sess);
}

void target_get_session(struct se_session *se_sess)
{
	kref_get(&se_sess->sess_kref);
}
EXPORT_SYMBOL(target_get_session);

int target_put_session(struct se_session *se_sess)
{
	return kref_put(&se_sess->sess_kref, target_release_session);
}
EXPORT_SYMBOL(target_put_session);

static void target_complete_nacl(struct kref *kref)
{
	struct se_node_acl *nacl = container_of(kref,
				struct se_node_acl, acl_kref);

	complete(&nacl->acl_free_comp);
}

void target_put_nacl(struct se_node_acl *nacl)
{
	kref_put(&nacl->acl_kref, target_complete_nacl);
}

void transport_deregister_session_configfs(struct se_session *se_sess)
{
	struct se_node_acl *se_nacl;
	unsigned long flags;
	/*
	 * Used by struct se_node_acl's under ConfigFS to locate active struct se_session
	 */
	se_nacl = se_sess->se_node_acl;
	if (se_nacl) {
		spin_lock_irqsave(&se_nacl->nacl_sess_lock, flags);
		if (se_nacl->acl_stop == 0)
			list_del(&se_sess->sess_acl_list);
		/*
		 * If the session list is empty, then clear the pointer.
		 * Otherwise, set the struct se_session pointer from the tail
		 * element of the per struct se_node_acl active session list.
		 */
		if (list_empty(&se_nacl->acl_sess_list))
			se_nacl->nacl_sess = NULL;
		else {
			se_nacl->nacl_sess = container_of(
					se_nacl->acl_sess_list.prev,
					struct se_session, sess_acl_list);
		}
		spin_unlock_irqrestore(&se_nacl->nacl_sess_lock, flags);
	}
}
EXPORT_SYMBOL(transport_deregister_session_configfs);

void transport_free_session(struct se_session *se_sess)
{

/* 20140513, adamhsu, redmine 8253 */
#ifdef CONFIG_MACH_QNAPTS
#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))

	if (se_sess->sess_cmd_map) {
		percpu_ida_destroy(&se_sess->sess_tag_pool);
		if (is_vmalloc_addr(se_sess->sess_cmd_map))
			vfree(se_sess->sess_cmd_map);
		else
			kfree(se_sess->sess_cmd_map);
	}


	transport_free_extra_tag_pool(se_sess);


#endif
#endif
	kmem_cache_free(se_sess_cache, se_sess);
}
EXPORT_SYMBOL(transport_free_session);

void transport_deregister_session(struct se_session *se_sess)
{
	struct se_portal_group *se_tpg = se_sess->se_tpg;
	struct target_core_fabric_ops *se_tfo;
	struct se_node_acl *se_nacl;
	unsigned long flags;
	bool comp_nacl = true;

	if (!se_tpg) {
		transport_free_session(se_sess);
		return;
	}
	se_tfo = se_tpg->se_tpg_tfo;

	spin_lock_irqsave(&se_tpg->session_lock, flags);
	list_del(&se_sess->sess_list);
	se_sess->se_tpg = NULL;
	se_sess->fabric_sess_ptr = NULL;
	spin_unlock_irqrestore(&se_tpg->session_lock, flags);

#ifdef CONFIG_MACH_QNAPTS	// Eric Gu for VMWare 5.0 certification
	/*
            SPC2r20 5.5.2 The Reserve/Release management methodThe reserve/release management 
            method commands, RESERVE(6), RESERVE(10), RELEASE(6), andRELEASE(10) are used among
            multiple initiators that do not require operations to be protected across 
            initiator failures (and subsequent hard resets).  The reserve/release reservations
            management method also allows anapplication client to provide restricted device 
            access to one additional initiator (a third-party initiator), usually a temporary 
            initiator performing a service for the application client sending the reservation
            command. Reservations managed using the reserve/release method do not persist across 
            some recovery actions (e.g., hardresets).  When a target performs one of these 
            recovery actions, the application client(s) have to re-discover the configuration
            and re-establish the required reservations.  Reserve/release managed reservations 
            are retained by the device server until released or until reset by mechanisms 
            specified in this standard.
	*/
    core_enumerate_hba_for_deregister_session(se_sess);    
#endif

	/*
	 * Determine if we need to do extra work for this initiator node's
	 * struct se_node_acl if it had been previously dynamically generated.
	 */
	se_nacl = se_sess->se_node_acl;

	spin_lock_irqsave(&se_tpg->acl_node_lock, flags);
	if (se_nacl && se_nacl->dynamic_node_acl) {
		if (!se_tfo->tpg_check_demo_mode_cache(se_tpg)) {
			list_del(&se_nacl->acl_list);
			se_tpg->num_node_acls--;
			spin_unlock_irqrestore(&se_tpg->acl_node_lock, flags);
			core_tpg_wait_for_nacl_pr_ref(se_nacl);
			core_free_device_list_for_node(se_nacl, se_tpg);
			se_tfo->tpg_release_fabric_acl(se_tpg, se_nacl);

			comp_nacl = false;
			spin_lock_irqsave(&se_tpg->acl_node_lock, flags);
		}
	}
	spin_unlock_irqrestore(&se_tpg->acl_node_lock, flags);

	pr_debug("TARGET_CORE[%s]: Deregistered fabric_sess\n",
		se_tpg->se_tpg_tfo->get_fabric_name());
	/*
	 * If last kref is dropping now for an explict NodeACL, awake sleeping
	 * ->acl_free_comp caller to wakeup configfs se_node_acl->acl_group
	 * removal context.
	 */
	if (se_nacl && comp_nacl == true)
		target_put_nacl(se_nacl);

	transport_free_session(se_sess);
}
EXPORT_SYMBOL(transport_deregister_session);

/*
 * Called with cmd->t_state_lock held.
 */
static void transport_all_task_dev_remove_state(struct se_cmd *cmd)
{
	struct se_device *dev = cmd->se_dev;
	struct se_task *task;
	unsigned long flags;

	if (!dev)
		return;

	list_for_each_entry(task, &cmd->t_task_list, t_list) {
		if (task->task_flags & TF_ACTIVE)
			continue;

		spin_lock_irqsave(&dev->execute_task_lock, flags);
		if (task->t_state_active) {
			pr_debug("Removed ITT: 0x%08x dev: %p task[%p]\n",
				cmd->se_tfo->get_task_tag(cmd), dev, task);

			list_del(&task->t_state_list);
			atomic_dec(&cmd->t_task_cdbs_ex_left);
			task->t_state_active = false;
		}
		spin_unlock_irqrestore(&dev->execute_task_lock, flags);
	}

}

/*	transport_cmd_check_stop():
 *
 *	'transport_off = 1' determines if CMD_T_ACTIVE should be cleared.
 *	'transport_off = 2' determines if task_dev_state should be removed.
 *
 *	A non-zero u8 t_state sets cmd->t_state.
 *	Returns 1 when command is stopped, else 0.
 */
static int transport_cmd_check_stop(
	struct se_cmd *cmd,
	int transport_off,
	u8 t_state)
{
	unsigned long flags;

	spin_lock_irqsave(&cmd->t_state_lock, flags);

	/*
	 * Determine if IOCTL context caller in requesting the stopping of this
	 * command for LUN shutdown purposes.
	 */
	if (cmd->transport_state & CMD_T_LUN_STOP) {
		pr_debug("%s:%d CMD_T_LUN_STOP for ITT: 0x%08x\n",
			__func__, __LINE__, cmd->se_tfo->get_task_tag(cmd));

		cmd->transport_state &= ~CMD_T_ACTIVE;
		if (transport_off == 2)
			transport_all_task_dev_remove_state(cmd);
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);

		complete(&cmd->transport_lun_stop_comp);
		return 1;
	}
	/*
	 * Determine if frontend context caller is requesting the stopping of
	 * this command for frontend exceptions.
	 */
	if (cmd->transport_state & CMD_T_STOP) {
		pr_debug("%s:%d CMD_T_STOP for ITT: 0x%08x\n",
			__func__, __LINE__,
			cmd->se_tfo->get_task_tag(cmd));

		if (transport_off == 2)
			transport_all_task_dev_remove_state(cmd);

		/*
		 * Clear struct se_cmd->se_lun before the transport_off == 2 handoff
		 * to FE.
		 */
		if (transport_off == 2)
			cmd->se_lun = NULL;

		spin_unlock_irqrestore(&cmd->t_state_lock, flags);

		complete(&cmd->t_transport_stop_comp);
		return 1;
	}
	if (transport_off) {
		cmd->transport_state &= ~CMD_T_ACTIVE;
		if (transport_off == 2) {
			transport_all_task_dev_remove_state(cmd);
			/*
			 * Clear struct se_cmd->se_lun before the transport_off == 2
			 * handoff to fabric module.
			 */
			cmd->se_lun = NULL;

			/*
			 * Some fabric modules like tcm_loop can release
			 * their internally allocated I/O reference now and
			 * struct se_cmd now.
			 *
			 * Fabric modules are expected to return '1' here if the
			 * se_cmd being passed is released at this point,
			 * or zero if not being released.
			 */
			if (cmd->se_tfo->check_stop_free != NULL) {
				spin_unlock_irqrestore(
					&cmd->t_state_lock, flags);

				return cmd->se_tfo->check_stop_free(cmd);
			}
		}
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);

		return 0;
	} else if (t_state)
		cmd->t_state = t_state;
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	return 0;
}

static int transport_cmd_check_stop_to_fabric(struct se_cmd *cmd)
{
	return transport_cmd_check_stop(cmd, 2, 0);
}

static void transport_lun_remove_cmd(struct se_cmd *cmd)
{
	struct se_lun *lun = cmd->se_lun;
	unsigned long flags;

	if (!lun)
		return;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	if (cmd->transport_state & CMD_T_DEV_ACTIVE) {
		cmd->transport_state &= ~CMD_T_DEV_ACTIVE;
		transport_all_task_dev_remove_state(cmd);
	}
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	spin_lock_irqsave(&lun->lun_cmd_lock, flags);
	if (!list_empty(&cmd->se_lun_node))
		list_del_init(&cmd->se_lun_node);
	spin_unlock_irqrestore(&lun->lun_cmd_lock, flags);

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TPC_CMD)
	tpc_free_track_data(cmd);
#endif
#endif

}

void transport_cmd_finish_abort(struct se_cmd *cmd, int remove)
{

#if defined(CONFIG_MACH_QNAPTS) 	
	/* adamhsu, 
	 * (1) redmine bug 6915 - bugzilla 40743
	 * (2) redmine bug 6916 - bugzilla 41183
	 */
	if (remove)
		__remove_cmd_rec(cmd);
#endif

	if (!(cmd->se_cmd_flags & SCF_SCSI_TMR_CDB))
		transport_lun_remove_cmd(cmd);

	if (transport_cmd_check_stop_to_fabric(cmd))
		return;

	if (remove) {
		/* 2014/04/5, adamhsu, redmine 7916
		 *
		 * Before to remove cmd, we need to make sure the caller
		 * removes the relationship between iscsi cmd and connection
		 * structure already.
		 *
		 * this is dangerous call !!
		 */
		transport_remove_cmd_from_queue(cmd);
		transport_put_cmd(cmd);
	}
}

static void transport_add_cmd_to_queue(struct se_cmd *cmd, int t_state,
		bool at_head)
{
	struct se_device *dev = cmd->se_dev;
	struct se_queue_obj *qobj = &dev->dev_queue_obj;
	unsigned long flags;

	if (t_state) {
		spin_lock_irqsave(&cmd->t_state_lock, flags);
		cmd->t_state = t_state;
		cmd->transport_state |= CMD_T_ACTIVE;
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
	}

	spin_lock_irqsave(&qobj->cmd_queue_lock, flags);

	/* If the cmd is already on the list, remove it before we add it */
	if (!list_empty(&cmd->se_queue_node))
		list_del(&cmd->se_queue_node);
	else
		atomic_inc(&qobj->queue_cnt);

	if (at_head)
		list_add(&cmd->se_queue_node, &qobj->qobj_list);
	else
		list_add_tail(&cmd->se_queue_node, &qobj->qobj_list);
	cmd->transport_state |= CMD_T_QUEUED;

	spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);
	wake_up_interruptible(&qobj->thread_wq);
}

static struct se_cmd *
transport_get_cmd_from_queue(struct se_queue_obj *qobj)
{
	struct se_cmd *cmd;
	unsigned long flags;

	spin_lock_irqsave(&qobj->cmd_queue_lock, flags);
	if (list_empty(&qobj->qobj_list)) {
		spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);
		return NULL;
	}
	cmd = list_first_entry(&qobj->qobj_list, struct se_cmd, se_queue_node);
	cmd->transport_state &= ~CMD_T_QUEUED;

	list_del_init(&cmd->se_queue_node);
	atomic_dec(&qobj->queue_cnt);
	spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);

	return cmd;
}

static void transport_remove_cmd_from_queue(struct se_cmd *cmd)
{
	struct se_queue_obj *qobj = &cmd->se_dev->dev_queue_obj;
	unsigned long flags;

	spin_lock_irqsave(&qobj->cmd_queue_lock, flags);
	if (!(cmd->transport_state & CMD_T_QUEUED)) {
		spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);
		return;
	}
	cmd->transport_state &= ~CMD_T_QUEUED;

	atomic_dec(&qobj->queue_cnt);
	list_del_init(&cmd->se_queue_node);
	spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);
}

/*
 * Completion function used by TCM subsystem plugins (such as FILEIO)
 * for queueing up response from struct se_subsystem_api->do_task()
 */
void transport_complete_sync_cache(struct se_cmd *cmd, int good)
{
	struct se_task *task = list_entry(cmd->t_task_list.next,
				struct se_task, t_list);

	if (good) {
		cmd->scsi_status = SAM_STAT_GOOD;
		task->task_scsi_status = GOOD;
	} else {
		task->task_scsi_status = SAM_STAT_CHECK_CONDITION;
		task->task_se_cmd->scsi_sense_reason =
				TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;

	}

	transport_complete_task(task, good);
}
EXPORT_SYMBOL(transport_complete_sync_cache);

static void target_complete_failure_work(struct work_struct *work)
{
	struct se_cmd *cmd = container_of(work, struct se_cmd, work);
	transport_generic_request_failure(cmd);
}

/*	transport_complete_task():
 *
 *	Called from interrupt and non interrupt context depending
 *	on the transport plugin.
 */
void transport_complete_task(struct se_task *task, int success)
{
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	unsigned long flags;

#if defined(SUPPORT_PARALLEL_TASK_WQ)
    /* after the task was done, to remove relationship of current task */
    p_task_remove_relationship_with_other_task(task, dev);
    wake_up_interruptible(&dev->p_task_thread_wq);
#endif

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	task->task_flags &= ~TF_ACTIVE;

	/*
	 * See if any sense data exists, if so set the TASK_SENSE flag.
	 * Also check for any other post completion work that needs to be
	 * done by the plugins.
	 */
	if (dev && dev->transport->transport_complete) {
		if (dev->transport->transport_complete(task) != 0) {
			cmd->se_cmd_flags |= SCF_TRANSPORT_TASK_SENSE;
			task->task_flags |= TF_HAS_SENSE;
			success = 1;
		}
	}

	/*
	 * See if we are waiting for outstanding struct se_task
	 * to complete for an exception condition
	 */
	if (task->task_flags & TF_REQUEST_STOP) {
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		complete(&task->task_stop_comp);
		return;
	}

	/* Add no space and run-time capacity handling */
	if (!success && (cmd->transport_state != CMD_T_NO_SPACE_IO_FAILED)
			    && (cmd->transport_state != CMD_T_CAP_CHANGE) )
		cmd->transport_state |= CMD_T_FAILED;

	/*
	 * Decrement the outstanding t_task_cdbs_left count.  The last
	 * struct se_task from struct se_cmd will complete itself into the
	 * device queue depending upon int success.
	 */
	if (!atomic_dec_and_test(&cmd->t_task_cdbs_left)) {
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		return;
	}

	/*
	 * Check for case where an explict ABORT_TASK has been received
	 * and transport_wait_for_tasks() will be waiting for completion..
	 */

	/* 2014/08/16, adamhsu, redmine 9055,9076,9278 */
	if ((cmd->transport_state & CMD_T_ABORTED)
		&& (cmd->transport_state & CMD_T_STOP)){

		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		pr_info("[%s] cmd(ITT:0x%8x) got CMD_T_STOP req, "
			"wait to be completed\n", __func__, 
			cmd->se_tfo->get_task_tag(cmd));

		complete(&cmd->t_transport_stop_comp);
		pr_info("[%s] cmd(ITT:0x%8x) got CMD_T_STOP req, "
			"done to wait\n", __func__,
			cmd->se_tfo->get_task_tag(cmd));

		return;
	} 
	else if (cmd->transport_state & CMD_T_FAILED) {
		cmd->scsi_sense_reason = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
		INIT_WORK(&cmd->work, target_complete_failure_work);
	} 
#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
	else if (cmd->transport_state & CMD_T_NO_SPACE_IO_FAILED) { // for no space response
		cmd->scsi_sense_reason = TCM_SPACE_ALLOCATION_FAILED_WRITE_PROTECT;
		INIT_WORK(&cmd->work, target_complete_failure_work);
	} else if (cmd->transport_state & CMD_T_CAP_CHANGE) { // for run-time capacity change
		cmd->scsi_sense_reason = TCM_CAPACITY_DATA_HAS_CHANGED;
		INIT_WORK(&cmd->work, target_complete_failure_work);	
	}
#endif	
	else {
		INIT_WORK(&cmd->work, target_complete_ok_work);
	}

	cmd->t_state = TRANSPORT_COMPLETE;
	cmd->transport_state |= (CMD_T_COMPLETE | CMD_T_ACTIVE);
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	queue_work(target_completion_wq, &cmd->work);
}
EXPORT_SYMBOL(transport_complete_task);

/*
 * Called by transport_add_tasks_from_cmd() once a struct se_cmd's
 * struct se_task list are ready to be added to the active execution list
 * struct se_device

 * Called with se_dev_t->execute_task_lock called.
 */
static inline int transport_add_task_check_sam_attr(
	struct se_task *task,
	struct se_task *task_prev,
	struct se_device *dev)
{
	/*
	 * No SAM Task attribute emulation enabled, add to tail of
	 * execution queue
	 */
	if (dev->dev_task_attr_type != SAM_TASK_ATTR_EMULATED) {
		list_add_tail(&task->t_execute_list, &dev->execute_task_list);
		return 0;
	}
	/*
	 * HEAD_OF_QUEUE attribute for received CDB, which means
	 * the first task that is associated with a struct se_cmd goes to
	 * head of the struct se_device->execute_task_list, and task_prev
	 * after that for each subsequent task
	 */
	if (task->task_se_cmd->sam_task_attr == MSG_HEAD_TAG) {
		list_add(&task->t_execute_list,
				(task_prev != NULL) ?
				&task_prev->t_execute_list :
				&dev->execute_task_list);

		pr_debug("Set HEAD_OF_QUEUE for task CDB: 0x%02x"
				" in execution queue\n",
				task->task_se_cmd->t_task_cdb[0]);
		return 1;
	}
	/*
	 * For ORDERED, SIMPLE or UNTAGGED attribute tasks once they have been
	 * transitioned from Dermant -> Active state, and are added to the end
	 * of the struct se_device->execute_task_list
	 */
	list_add_tail(&task->t_execute_list, &dev->execute_task_list);
	return 0;
}

/*	__transport_add_task_to_execute_queue():
 *
 *	Called with se_dev_t->execute_task_lock called.
 */
static void __transport_add_task_to_execute_queue(
	struct se_task *task,
	struct se_task *task_prev,
	struct se_device *dev)
{
	int head_of_queue;

	head_of_queue = transport_add_task_check_sam_attr(task, task_prev, dev);
	atomic_inc(&dev->execute_tasks);

	if (task->t_state_active)
		return;
	/*
	 * Determine if this task needs to go to HEAD_OF_QUEUE for the
	 * state list as well.  Running with SAM Task Attribute emulation
	 * will always return head_of_queue == 0 here
	 */
	if (head_of_queue)
		list_add(&task->t_state_list, (task_prev) ?
				&task_prev->t_state_list :
				&dev->state_task_list);
	else
		list_add_tail(&task->t_state_list, &dev->state_task_list);

	task->t_state_active = true;

	pr_debug("Added ITT: 0x%08x task[%p] to dev: %p\n",
		task->task_se_cmd->se_tfo->get_task_tag(task->task_se_cmd),
		task, dev);
}

static void transport_add_tasks_to_state_queue(struct se_cmd *cmd)
{
	struct se_device *dev = cmd->se_dev;
	struct se_task *task;
	unsigned long flags;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	list_for_each_entry(task, &cmd->t_task_list, t_list) {
		spin_lock(&dev->execute_task_lock);
		if (!task->t_state_active) {
			list_add_tail(&task->t_state_list,
				      &dev->state_task_list);
			task->t_state_active = true;

			pr_debug("Added ITT: 0x%08x task[%p] to dev: %p\n",
				task->task_se_cmd->se_tfo->get_task_tag(
				task->task_se_cmd), task, dev);
		}
		spin_unlock(&dev->execute_task_lock);
	}
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);
}

static void __transport_add_tasks_from_cmd(struct se_cmd *cmd)
{
	struct se_device *dev = cmd->se_dev;
	struct se_task *task, *task_prev = NULL;

	list_for_each_entry(task, &cmd->t_task_list, t_list) {
		if (!list_empty(&task->t_execute_list))
			continue;
		/*
		 * __transport_add_task_to_execute_queue() handles the
		 * SAM Task Attribute emulation if enabled
		 */
		__transport_add_task_to_execute_queue(task, task_prev, dev);
		task_prev = task;
	}
}

static void transport_add_tasks_from_cmd(struct se_cmd *cmd)
{
	unsigned long flags;
	struct se_device *dev = cmd->se_dev;

	spin_lock_irqsave(&dev->execute_task_lock, flags);
	__transport_add_tasks_from_cmd(cmd);
	spin_unlock_irqrestore(&dev->execute_task_lock, flags);
}

void __transport_remove_task_from_execute_queue(struct se_task *task,
		struct se_device *dev)
{
	list_del_init(&task->t_execute_list);
	atomic_dec(&dev->execute_tasks);
}

static void transport_remove_task_from_execute_queue(
	struct se_task *task,
	struct se_device *dev)
{
	unsigned long flags;

	//if (WARN_ON(list_empty(&task->t_execute_list)))
	if (list_empty(&task->t_execute_list))
		return;

	spin_lock_irqsave(&dev->execute_task_lock, flags);
	__transport_remove_task_from_execute_queue(task, dev);
	spin_unlock_irqrestore(&dev->execute_task_lock, flags);
}

/*
 * Handle QUEUE_FULL / -EAGAIN and -ENOMEM status
 */

static void target_qf_do_work(struct work_struct *work)
{
	struct se_device *dev = container_of(work, struct se_device,
					qf_work_queue);
	LIST_HEAD(qf_cmd_list);
	struct se_cmd *cmd, *cmd_tmp;

	spin_lock_irq(&dev->qf_cmd_lock);
	list_splice_init(&dev->qf_cmd_list, &qf_cmd_list);
	spin_unlock_irq(&dev->qf_cmd_lock);

	list_for_each_entry_safe(cmd, cmd_tmp, &qf_cmd_list, se_qf_node) {
		list_del(&cmd->se_qf_node);
		atomic_dec(&dev->dev_qf_count);
		smp_mb__after_atomic_dec();

		pr_debug("Processing %s cmd: %p QUEUE_FULL in work queue"
			" context: %s\n", cmd->se_tfo->get_fabric_name(), cmd,
			(cmd->t_state == TRANSPORT_COMPLETE_QF_OK) ? "COMPLETE_OK" :
			(cmd->t_state == TRANSPORT_COMPLETE_QF_WP) ? "WRITE_PENDING"
			: "UNKNOWN");

		transport_add_cmd_to_queue(cmd, cmd->t_state, true);
	}
}

unsigned char *transport_dump_cmd_direction(struct se_cmd *cmd)
{
	switch (cmd->data_direction) {
	case DMA_NONE:
		return "NONE";
	case DMA_FROM_DEVICE:
		return "READ";
	case DMA_TO_DEVICE:
		return "WRITE";
	case DMA_BIDIRECTIONAL:
		return "BIDI";
	default:
		break;
	}

	return "UNKNOWN";
}

void transport_dump_dev_state(
	struct se_device *dev,
	char *b,
	int *bl)
{
	*bl += sprintf(b + *bl, "Status: ");
	switch (dev->dev_status) {
	case TRANSPORT_DEVICE_ACTIVATED:
		*bl += sprintf(b + *bl, "ACTIVATED");
		break;
	case TRANSPORT_DEVICE_DEACTIVATED:
		*bl += sprintf(b + *bl, "DEACTIVATED");
		break;
	case TRANSPORT_DEVICE_SHUTDOWN:
		*bl += sprintf(b + *bl, "SHUTDOWN");
		break;
	case TRANSPORT_DEVICE_OFFLINE_ACTIVATED:
	case TRANSPORT_DEVICE_OFFLINE_DEACTIVATED:
		*bl += sprintf(b + *bl, "OFFLINE");
		break;
	default:
		*bl += sprintf(b + *bl, "UNKNOWN=%d", dev->dev_status);
		break;
	}

	*bl += sprintf(b + *bl, "  Execute/Max Queue Depth: %d/%d",
		atomic_read(&dev->execute_tasks), dev->queue_depth);
	*bl += sprintf(b + *bl, "  SectorSize: %u  MaxSectors: %u\n",
		dev->se_sub_dev->se_dev_attrib.block_size, dev->se_sub_dev->se_dev_attrib.max_sectors);
	*bl += sprintf(b + *bl, "        ");
}

void transport_dump_vpd_proto_id(
	struct t10_vpd *vpd,
	unsigned char *p_buf,
	int p_buf_len)
{
	unsigned char buf[VPD_TMP_BUF_SIZE];
	int len;

	memset(buf, 0, VPD_TMP_BUF_SIZE);
	len = sprintf(buf, "T10 VPD Protocol Identifier: ");

	switch (vpd->protocol_identifier) {
	case 0x00:
		sprintf(buf+len, "Fibre Channel\n");
		break;
	case 0x10:
		sprintf(buf+len, "Parallel SCSI\n");
		break;
	case 0x20:
		sprintf(buf+len, "SSA\n");
		break;
	case 0x30:
		sprintf(buf+len, "IEEE 1394\n");
		break;
	case 0x40:
		sprintf(buf+len, "SCSI Remote Direct Memory Access"
				" Protocol\n");
		break;
	case 0x50:
		sprintf(buf+len, "Internet SCSI (iSCSI)\n");
		break;
	case 0x60:
		sprintf(buf+len, "SAS Serial SCSI Protocol\n");
		break;
	case 0x70:
		sprintf(buf+len, "Automation/Drive Interface Transport"
				" Protocol\n");
		break;
	case 0x80:
		sprintf(buf+len, "AT Attachment Interface ATA/ATAPI\n");
		break;
	default:
		sprintf(buf+len, "Unknown 0x%02x\n",
				vpd->protocol_identifier);
		break;
	}

	if (p_buf)
		strncpy(p_buf, buf, p_buf_len);
	else
		pr_debug("%s", buf);
}

void
transport_set_vpd_proto_id(struct t10_vpd *vpd, unsigned char *page_83)
{
	/*
	 * Check if the Protocol Identifier Valid (PIV) bit is set..
	 *
	 * from spc3r23.pdf section 7.5.1
	 */
	 if (page_83[1] & 0x80) {
		vpd->protocol_identifier = (page_83[0] & 0xf0);
		vpd->protocol_identifier_set = 1;
		transport_dump_vpd_proto_id(vpd, NULL, 0);
	}
}
EXPORT_SYMBOL(transport_set_vpd_proto_id);

int transport_dump_vpd_assoc(
	struct t10_vpd *vpd,
	unsigned char *p_buf,
	int p_buf_len)
{
	unsigned char buf[VPD_TMP_BUF_SIZE];
	int ret = 0;
	int len;

	memset(buf, 0, VPD_TMP_BUF_SIZE);
	len = sprintf(buf, "T10 VPD Identifier Association: ");

	switch (vpd->association) {
	case 0x00:
		sprintf(buf+len, "addressed logical unit\n");
		break;
	case 0x10:
		sprintf(buf+len, "target port\n");
		break;
	case 0x20:
		sprintf(buf+len, "SCSI target device\n");
		break;
	default:
		sprintf(buf+len, "Unknown 0x%02x\n", vpd->association);
		ret = -EINVAL;
		break;
	}

	if (p_buf)
		strncpy(p_buf, buf, p_buf_len);
	else
		pr_debug("%s", buf);

	return ret;
}

int transport_set_vpd_assoc(struct t10_vpd *vpd, unsigned char *page_83)
{
	/*
	 * The VPD identification association..
	 *
	 * from spc3r23.pdf Section 7.6.3.1 Table 297
	 */
	vpd->association = (page_83[1] & 0x30);
	return transport_dump_vpd_assoc(vpd, NULL, 0);
}
EXPORT_SYMBOL(transport_set_vpd_assoc);

int transport_dump_vpd_ident_type(
	struct t10_vpd *vpd,
	unsigned char *p_buf,
	int p_buf_len)
{
	unsigned char buf[VPD_TMP_BUF_SIZE];
	int ret = 0;
	int len;

	memset(buf, 0, VPD_TMP_BUF_SIZE);
	len = sprintf(buf, "T10 VPD Identifier Type: ");

	switch (vpd->device_identifier_type) {
	case 0x00:
		sprintf(buf+len, "Vendor specific\n");
		break;
	case 0x01:
		sprintf(buf+len, "T10 Vendor ID based\n");
		break;
	case 0x02:
		sprintf(buf+len, "EUI-64 based\n");
		break;
	case 0x03:
		sprintf(buf+len, "NAA\n");
		break;
	case 0x04:
		sprintf(buf+len, "Relative target port identifier\n");
		break;
	case 0x08:
		sprintf(buf+len, "SCSI name string\n");
		break;
	default:
		sprintf(buf+len, "Unsupported: 0x%02x\n",
				vpd->device_identifier_type);
		ret = -EINVAL;
		break;
	}

	if (p_buf) {
		if (p_buf_len < strlen(buf)+1)
			return -EINVAL;
		strncpy(p_buf, buf, p_buf_len);
	} else {
		pr_debug("%s", buf);
	}

	return ret;
}

int transport_set_vpd_ident_type(struct t10_vpd *vpd, unsigned char *page_83)
{
	/*
	 * The VPD identifier type..
	 *
	 * from spc3r23.pdf Section 7.6.3.1 Table 298
	 */
	vpd->device_identifier_type = (page_83[1] & 0x0f);
	return transport_dump_vpd_ident_type(vpd, NULL, 0);
}
EXPORT_SYMBOL(transport_set_vpd_ident_type);

int transport_dump_vpd_ident(
	struct t10_vpd *vpd,
	unsigned char *p_buf,
	int p_buf_len)
{
	unsigned char buf[VPD_TMP_BUF_SIZE];
	int ret = 0;

	memset(buf, 0, VPD_TMP_BUF_SIZE);

	switch (vpd->device_identifier_code_set) {
	case 0x01: /* Binary */
		sprintf(buf, "T10 VPD Binary Device Identifier: %s\n",
			&vpd->device_identifier[0]);
		break;
	case 0x02: /* ASCII */
		sprintf(buf, "T10 VPD ASCII Device Identifier: %s\n",
			&vpd->device_identifier[0]);
		break;
	case 0x03: /* UTF-8 */
		sprintf(buf, "T10 VPD UTF-8 Device Identifier: %s\n",
			&vpd->device_identifier[0]);
		break;
	default:
		sprintf(buf, "T10 VPD Device Identifier encoding unsupported:"
			" 0x%02x", vpd->device_identifier_code_set);
		ret = -EINVAL;
		break;
	}

	if (p_buf)
		strncpy(p_buf, buf, p_buf_len);
	else
		pr_debug("%s", buf);

	return ret;
}

int
transport_set_vpd_ident(struct t10_vpd *vpd, unsigned char *page_83)
{
	static const char hex_str[] = "0123456789abcdef";
	int j = 0, i = 4; /* offset to start of the identifer */

	/*
	 * The VPD Code Set (encoding)
	 *
	 * from spc3r23.pdf Section 7.6.3.1 Table 296
	 */
	vpd->device_identifier_code_set = (page_83[0] & 0x0f);
	switch (vpd->device_identifier_code_set) {
	case 0x01: /* Binary */
		vpd->device_identifier[j++] =
				hex_str[vpd->device_identifier_type];
		while (i < (4 + page_83[3])) {
			vpd->device_identifier[j++] =
				hex_str[(page_83[i] & 0xf0) >> 4];
			vpd->device_identifier[j++] =
				hex_str[page_83[i] & 0x0f];
			i++;
		}
		break;
	case 0x02: /* ASCII */
	case 0x03: /* UTF-8 */
		while (i < (4 + page_83[3]))
			vpd->device_identifier[j++] = page_83[i++];
		break;
	default:
		break;
	}

	return transport_dump_vpd_ident(vpd, NULL, 0);
}
EXPORT_SYMBOL(transport_set_vpd_ident);

static void core_setup_task_attr_emulation(struct se_device *dev)
{
	/*
	 * If this device is from Target_Core_Mod/pSCSI, disable the
	 * SAM Task Attribute emulation.
	 *
	 * This is currently not available in upsream Linux/SCSI Target
	 * mode code, and is assumed to be disabled while using TCM/pSCSI.
	 */
	if (dev->transport->transport_type == TRANSPORT_PLUGIN_PHBA_PDEV) {
		dev->dev_task_attr_type = SAM_TASK_ATTR_PASSTHROUGH;
		return;
	}

	dev->dev_task_attr_type = SAM_TASK_ATTR_EMULATED;
	pr_debug("%s: Using SAM_TASK_ATTR_EMULATED for SPC: 0x%02x"
		" device\n", dev->transport->name,
		dev->transport->get_device_rev(dev));
}

static void scsi_dump_inquiry(struct se_device *dev)
{
	struct t10_wwn *wwn = &dev->se_sub_dev->t10_wwn;
	char buf[17];
	int i, device_type;
	/*
	 * Print Linux/SCSI style INQUIRY formatting to the kernel ring buffer
	 */
	for (i = 0; i < 8; i++)
		if (wwn->vendor[i] >= 0x20)
			buf[i] = wwn->vendor[i];
		else
			buf[i] = ' ';
	buf[i] = '\0';
	pr_debug("  Vendor: %s\n", buf);

	for (i = 0; i < 16; i++)
		if (wwn->model[i] >= 0x20)
			buf[i] = wwn->model[i];
		else
			buf[i] = ' ';
	buf[i] = '\0';
	pr_debug("  Model: %s\n", buf);

	for (i = 0; i < 4; i++)
		if (wwn->revision[i] >= 0x20)
			buf[i] = wwn->revision[i];
		else
			buf[i] = ' ';
	buf[i] = '\0';
	pr_debug("  Revision: %s\n", buf);

	device_type = dev->transport->get_device_type(dev);
	pr_debug("  Type:   %s ", scsi_device_type(device_type));
	pr_debug("                 ANSI SCSI revision: %02x\n",
				dev->transport->get_device_rev(dev));
}

struct se_device *transport_add_device_to_core_hba(
	struct se_hba *hba,
	struct se_subsystem_api *transport,
	struct se_subsystem_dev *se_dev,
	u32 device_flags,
	void *transport_dev,
	struct se_dev_limits *dev_limits,
	const char *inquiry_prod,
	const char *inquiry_rev)
{
	int force_pt;
	struct se_device  *dev;

	dev = kzalloc(sizeof(struct se_device), GFP_KERNEL);
	if (!dev) {
		pr_err("Unable to allocate memory for se_dev_t\n");
		return NULL;
	}

	transport_init_queue_obj(&dev->dev_queue_obj);
	dev->dev_flags		= device_flags;
	dev->dev_status		|= TRANSPORT_DEVICE_DEACTIVATED;
	dev->dev_ptr		= transport_dev;
	dev->se_hba		= hba;
	dev->se_sub_dev		= se_dev;
	dev->transport		= transport;
	INIT_LIST_HEAD(&dev->dev_list);
	INIT_LIST_HEAD(&dev->dev_sep_list);
	INIT_LIST_HEAD(&dev->dev_tmr_list);
	INIT_LIST_HEAD(&dev->execute_task_list);
	INIT_LIST_HEAD(&dev->delayed_cmd_list);
	INIT_LIST_HEAD(&dev->state_task_list);
	INIT_LIST_HEAD(&dev->qf_cmd_list);
	spin_lock_init(&dev->execute_task_lock);
	spin_lock_init(&dev->delayed_cmd_lock);
	spin_lock_init(&dev->dev_reservation_lock);
	spin_lock_init(&dev->dev_status_lock);
	spin_lock_init(&dev->se_port_lock);
	spin_lock_init(&dev->se_tmr_lock);
	spin_lock_init(&dev->qf_cmd_lock);
	atomic_set(&dev->dev_ordered_id, 0);


#if defined(CONFIG_MACH_QNAPTS)

	dev->fast_blk_clone = 0;

	INIT_LIST_HEAD(&dev->cmd_rec_list);
	spin_lock_init(&dev->cmd_rec_lock);
	atomic_set(&dev->cmd_rec_count, 0);

#if defined(SUPPORT_PARALLEL_TASK_WQ)
    INIT_LIST_HEAD(&dev->dev_r_task_list);
    INIT_LIST_HEAD(&dev->dev_run_task_list);
    INIT_LIST_HEAD(&dev->dev_q_task_list);
    spin_lock_init(&dev->dev_r_task_lock);
    spin_lock_init(&dev->dev_run_task_lock);
    spin_lock_init(&dev->dev_q_task_lock);
    atomic_set(&dev->dev_r_task_cnt, 0);
#endif
	transport_setup_support_fbc(dev);
	transport_set_pool_blk_sectors(dev, dev_limits);
#endif

	se_dev_set_default_attribs(dev, dev_limits);

	dev->dev_index = scsi_get_new_index(SCSI_DEVICE_INDEX);
	dev->creation_time = get_jiffies_64();
	spin_lock_init(&dev->stats_lock);

	spin_lock(&hba->device_lock);
	list_add_tail(&dev->dev_list, &hba->hba_dev_list);
	hba->dev_count++;
	spin_unlock(&hba->device_lock);
	/*
	 * Setup the SAM Task Attribute emulation for struct se_device
	 */
	core_setup_task_attr_emulation(dev);
	/*
	 * Force PR and ALUA passthrough emulation with internal object use.
	 */
	force_pt = (hba->hba_flags & HBA_FLAGS_INTERNAL_USE);
	/*
	 * Setup the Reservations infrastructure for struct se_device
	 */
	core_setup_reservations(dev, force_pt);
	/*
	 * Setup the Asymmetric Logical Unit Assignment for struct se_device
	 */
	if (core_setup_alua(dev, force_pt) < 0)
		goto out;

	/*
	 * Startup the struct se_device processing thread
	 */
#ifdef CONFIG_MACH_QNAPTS // 2009/7/31 Nike Chen change ID to QNAP
#if defined(Athens)
	dev->process_thread = kthread_run(transport_processing_thread, dev,
					  "Cisco_%s", dev->transport->name);
#elif defined(IS_G)
	dev->process_thread = kthread_run(transport_processing_thread, dev,
					  "NAS_%s", dev->transport->name);
#else
	dev->process_thread = kthread_run(transport_processing_thread, dev,
					  "QNAP_%s", dev->transport->name);
#endif /* #if defined(Athens) */
#else
	dev->process_thread = kthread_run(transport_processing_thread, dev,
					  "LIO_%s", dev->transport->name);
#endif /* #ifdef CONFIG_MACH_QNAPTS */    
	if (IS_ERR(dev->process_thread)) {
		pr_err("Unable to create kthread: LIO_%s\n",
			dev->transport->name);
		goto out;
	}

#if defined(SUPPORT_PARALLEL_TASK_WQ)
    dev->p_task_work_queue = alloc_workqueue("p_task_wq",
                                WQ_MEM_RECLAIM | WQ_UNBOUND, 0);

    if (!dev->p_task_work_queue){
        pr_err("Unable to create p_task_wq\n");
        goto out;
    }

    init_waitqueue_head(&dev->p_task_thread_wq);
    dev->p_task_thread = kthread_run(p_task_thread, dev,
                                "p_task_%s", dev->transport->name);

    if (IS_ERR(dev->p_task_thread)) {
        pr_err("Unable to create p_task_%s\n", dev->transport->name);
        goto out;
    }
#endif

	/*
	 * Setup work_queue for QUEUE_FULL
	 */
	INIT_WORK(&dev->qf_work_queue, target_qf_do_work);

	/*
	 * Preload the initial INQUIRY const values if we are doing
	 * anything virtual (IBLOCK, FILEIO, RAMDISK), but not for TCM/pSCSI
	 * passthrough because this is being provided by the backend LLD.
	 * This is required so that transport_get_inquiry() copies these
	 * originals once back into DEV_T10_WWN(dev) for the virtual device
	 * setup.
	 */
	if (dev->transport->transport_type != TRANSPORT_PLUGIN_PHBA_PDEV) {
		if (!inquiry_prod || !inquiry_rev) {
			pr_err("All non TCM/pSCSI plugins require"
				" INQUIRY consts\n");
			goto out;
		}
#ifdef CONFIG_MACH_QNAPTS // 2009/7/31 Nike Chen change ID to QNAP
#if defined(Athens)
		strncpy(&dev->se_sub_dev->t10_wwn.vendor[0], "Cisco", 6);
#elif defined(IS_G)
		strncpy(&dev->se_sub_dev->t10_wwn.vendor[0], "NAS", 4);
#else
		strncpy(&dev->se_sub_dev->t10_wwn.vendor[0], "QNAP", 5);
#endif /* #if defined(Athens) */
#else
		strncpy(&dev->se_sub_dev->t10_wwn.vendor[0], "LIO-ORG", 8);
#endif /* #ifdef CONFIG_MACH_QNAPTS */
        
		strncpy(&dev->se_sub_dev->t10_wwn.model[0], inquiry_prod, 16);
		strncpy(&dev->se_sub_dev->t10_wwn.revision[0], inquiry_rev, 4);
	}
	scsi_dump_inquiry(dev);

	return dev;

out:

#if defined(SUPPORT_PARALLEL_TASK_WQ)
    if (dev->p_task_work_queue)
        destroy_workqueue(dev->p_task_work_queue);

    if (dev->p_task_thread)
        kthread_stop(dev->p_task_thread);
#endif

	kthread_stop(dev->process_thread);

	spin_lock(&hba->device_lock);
	list_del(&dev->dev_list);
	hba->dev_count--;
	spin_unlock(&hba->device_lock);

	se_release_vpd_for_dev(dev);

	kfree(dev);

	return NULL;
}
EXPORT_SYMBOL(transport_add_device_to_core_hba);

/*	transport_generic_prepare_cdb():
 *
 *	Since the Initiator sees iSCSI devices as LUNs,  the SCSI CDB will
 *	contain the iSCSI LUN in bits 7-5 of byte 1 as per SAM-2.
 *	The point of this is since we are mapping iSCSI LUNs to
 *	SCSI Target IDs having a non-zero LUN in the CDB will throw the
 *	devices and HBAs for a loop.
 */
static inline void transport_generic_prepare_cdb(
	unsigned char *cdb)
{
	switch (cdb[0]) {
	case READ_10: /* SBC - RDProtect */
	case READ_12: /* SBC - RDProtect */
	case READ_16: /* SBC - RDProtect */
	case SEND_DIAGNOSTIC: /* SPC - SELF-TEST Code */
	case VERIFY: /* SBC - VRProtect */
	case VERIFY_16: /* SBC - VRProtect */
	case WRITE_VERIFY: /* SBC - VRProtect */
	case WRITE_VERIFY_12: /* SBC - VRProtect */
#ifdef CONFIG_MACH_QNAPTS // Benjamin 20120531 for WRITE_VERIFY_16 
	case WRITE_VERIFY_16: /* SBC - VRProtect */
#endif         
		break;
	default:
		cdb[1] &= 0x1f; /* clear logical unit number */
		break;
	}
}

static struct se_task *
transport_generic_get_task(struct se_cmd *cmd,
		enum dma_data_direction data_direction)
{
	struct se_task *task;
	struct se_device *dev = cmd->se_dev;

	task = dev->transport->alloc_task(cmd->t_task_cdb);
	if (!task) {
		pr_err("Unable to allocate struct se_task\n");
		return NULL;
	}

	INIT_LIST_HEAD(&task->t_list);
	INIT_LIST_HEAD(&task->t_execute_list);
	INIT_LIST_HEAD(&task->t_state_list);

	init_completion(&task->task_stop_comp);
	task->task_se_cmd = cmd;
	task->task_data_direction = data_direction;

#if defined(SUPPORT_PARALLEL_TASK_WQ)
    INIT_WORK(&task->t_work, p_task_work_func);
    INIT_LIST_HEAD(&task->t_node);
    INIT_LIST_HEAD(&task->t_rec_list);
    spin_lock_init(&task->t_rec_lock);
#endif

	return task;
}

static int transport_generic_cmd_sequencer(struct se_cmd *, unsigned char *);

// prevent from canceling not initialed work item
static void target_work_nop(struct work_struct *work)
{
    return;
}

/*
 * Used by fabric modules containing a local struct se_cmd within their
 * fabric dependent per I/O descriptor.
 */
void transport_init_se_cmd(
	struct se_cmd *cmd,
	struct target_core_fabric_ops *tfo,
	struct se_session *se_sess,
	u32 data_length,
	int data_direction,
	int task_attr,
	unsigned char *sense_buffer)
{
	INIT_LIST_HEAD(&cmd->se_lun_node);
	INIT_LIST_HEAD(&cmd->se_delayed_node);
	INIT_LIST_HEAD(&cmd->se_qf_node);
	INIT_LIST_HEAD(&cmd->se_queue_node);
	INIT_LIST_HEAD(&cmd->se_cmd_list);
	INIT_LIST_HEAD(&cmd->t_task_list);
	init_completion(&cmd->transport_lun_fe_stop_comp);
	init_completion(&cmd->transport_lun_stop_comp);
	init_completion(&cmd->t_transport_stop_comp);
	init_completion(&cmd->cmd_wait_comp);

	spin_lock_init(&cmd->t_state_lock);
	cmd->transport_state = CMD_T_DEV_ACTIVE;
	cmd->se_tfo = tfo;
	cmd->se_sess = se_sess;
	cmd->data_length = data_length;
	cmd->data_direction = data_direction;
	cmd->sam_task_attr = task_attr;
	cmd->sense_buffer = sense_buffer;
    
	// prevent from canceling not initialed work item
	INIT_WORK(&cmd->work, target_work_nop);

#if defined(CONFIG_MACH_QNAPTS)

	/* 2014/08/16, adamhsu, redmine 9055,9076,9278 */
	cmd->tmf_code = 0;
	cmd->tmf_resp_tas = 0;
	cmd->tmf_diff_it_nexus = 0;
	spin_lock_init(&cmd->tmf_data_lock);

	cmd->byte_err_offset = 0;
	cmd->cur_se_task = NULL;

#if defined(SUPPORT_CONCURRENT_TASKS)
	cmd->use_wq = false;
#endif

#if defined(SUPPORT_VAAI)
	memset((void*)&cmd->xcopy_info, 0, sizeof(XCOPY_INFO));
#endif

#if defined(SUPPORT_TPC_CMD)
	INIT_LIST_HEAD(&cmd->t_cmd_node);
	cmd->t_track_rec = NULL;
	memset((void*)cmd->t_isid, 0, PR_REG_ISID_LEN);
	cmd->t_tiqn      = NULL;
	cmd->is_tpc      = 0;
	cmd->t_list_id   = 0;
	cmd->t_op_sac    = 0;
	cmd->t_pg_tag    = 0;
#endif
#endif /* defined(CONFIG_MACH_QNAPTS) */

}
EXPORT_SYMBOL(transport_init_se_cmd);

static int transport_check_alloc_task_attr(struct se_cmd *cmd)
{
	/*
	 * Check if SAM Task Attribute emulation is enabled for this
	 * struct se_device storage object
	 */
	if (cmd->se_dev->dev_task_attr_type != SAM_TASK_ATTR_EMULATED)
		return 0;

	if (cmd->sam_task_attr == MSG_ACA_TAG) {
		pr_debug("SAM Task Attribute ACA"
			" emulation is not supported\n");
		return -EINVAL;
	}
	/*
	 * Used to determine when ORDERED commands should go from
	 * Dormant to Active status.
	 */
	cmd->se_ordered_id = atomic_inc_return(&cmd->se_dev->dev_ordered_id);
	smp_mb__after_atomic_inc();
	pr_debug("Allocated se_ordered_id: %u for Task Attr: 0x%02x on %s\n",
			cmd->se_ordered_id, cmd->sam_task_attr,
			cmd->se_dev->transport->name);
	return 0;
}

/*	transport_generic_allocate_tasks():
 *
 *	Called from fabric RX Thread.
 */
int transport_generic_allocate_tasks(
	struct se_cmd *cmd,
	unsigned char *cdb)
{
	int ret;

	transport_generic_prepare_cdb(cdb);


	/*
	 * Ensure that the received CDB is less than the max (252 + 8) bytes
	 * for VARIABLE_LENGTH_CMD
	 */
	if (scsi_command_size(cdb) > SCSI_MAX_VARLEN_CDB_SIZE) {
		pr_err("Received SCSI CDB with command_size: %d that"
			" exceeds SCSI_MAX_VARLEN_CDB_SIZE: %d\n",
			scsi_command_size(cdb), SCSI_MAX_VARLEN_CDB_SIZE);
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = TCM_INVALID_CDB_FIELD;
		return -EINVAL;
	}
	/*
	 * If the received CDB is larger than TCM_MAX_COMMAND_SIZE,
	 * allocate the additional extended CDB buffer now..  Otherwise
	 * setup the pointer from __t_task_cdb to t_task_cdb.
	 */
	if (scsi_command_size(cdb) > sizeof(cmd->__t_task_cdb)) {
		cmd->t_task_cdb = kzalloc(scsi_command_size(cdb),
						GFP_KERNEL);
		if (!cmd->t_task_cdb) {
			pr_err("Unable to allocate cmd->t_task_cdb"
				" %u > sizeof(cmd->__t_task_cdb): %lu ops\n",
				scsi_command_size(cdb),
				(unsigned long)sizeof(cmd->__t_task_cdb));
			cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
			cmd->scsi_sense_reason =
					TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
			return -ENOMEM;
		}
	} else
		cmd->t_task_cdb = &cmd->__t_task_cdb[0];
	/*
	 * Copy the original CDB into cmd->
	 */
	memcpy(cmd->t_task_cdb, cdb, scsi_command_size(cdb));
	/*
	 * Setup the received CDB based on SCSI defined opcodes and
	 * perform unit attention, persistent reservations and ALUA
	 * checks for virtual device backends.  The cmd->t_task_cdb
	 * pointer is expected to be setup before we reach this point.
	 */
	ret = transport_generic_cmd_sequencer(cmd, cdb);
	if (ret < 0)
		return ret;
	/*
	 * Check for SAM Task Attribute Emulation
	 */
	if (transport_check_alloc_task_attr(cmd) < 0) {
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = TCM_INVALID_CDB_FIELD;
		return -EINVAL;
	}
	spin_lock(&cmd->se_lun->lun_sep_lock);
	if (cmd->se_lun->lun_sep)
		cmd->se_lun->lun_sep->sep_stats.cmd_pdus++;
	spin_unlock(&cmd->se_lun->lun_sep_lock);
	return 0;
}
EXPORT_SYMBOL(transport_generic_allocate_tasks);

/*
 * Used by fabric module frontends to queue tasks directly.
 * Many only be used from process context only
 */
int transport_handle_cdb_direct(
	struct se_cmd *cmd)
{
	int ret;

	if (!cmd->se_lun) {
		dump_stack();
		pr_err("cmd->se_lun is NULL\n");
		return -EINVAL;
	}
	if (in_interrupt()) {
		dump_stack();
		pr_err("transport_generic_handle_cdb cannot be called"
				" from interrupt context\n");
		return -EINVAL;
	}
	/*
	 * Set TRANSPORT_NEW_CMD state and CMD_T_ACTIVE following
	 * transport_generic_handle_cdb*() -> transport_add_cmd_to_queue()
	 * in existing usage to ensure that outstanding descriptors are handled
	 * correctly during shutdown via transport_wait_for_tasks()
	 *
	 * Also, we don't take cmd->t_state_lock here as we only expect
	 * this to be called for initial descriptor submission.
	 */
	cmd->t_state = TRANSPORT_NEW_CMD;
	cmd->transport_state |= CMD_T_ACTIVE;

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TPC_CMD)
	/* The code entry will come from iscsit_execute_cmd()  
	 *
	 * -1: not tpc cmd or other fatal error, 0: in progress, 1: not in progress 
	 */
	if (tpc_is_in_progress(cmd) == 0){
		/* SPC4R36, page 240
		 *
		 * Unless otherwise specificed, the list id is the uniquely
		 * identifies a copy operation among all those being processed
		 * that were received on a specific I_T nexus. If the copy 
		 * manager detects a duplicate list identifier value,then the
		 * originating 3rd copy command shall be terminated with 
		 * CHECK CONDITION status,with the sense key set to 
		 * ILLEGAL REQUEST, and the ASC set to OPERATION IN PROGRESS
		 */
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = TCM_OPERATION_IN_PROGRESS;
		transport_generic_request_failure(cmd);
		return 0;
	}
#endif
#endif

	/*
	 * transport_generic_new_cmd() is already handling QUEUE_FULL,
	 * so follow TRANSPORT_NEW_CMD processing thread context usage
	 * and call transport_generic_request_failure() if necessary..
	 */
	ret = transport_generic_new_cmd(cmd);
	if (ret < 0)
		transport_generic_request_failure(cmd);

	return 0;
}
EXPORT_SYMBOL(transport_handle_cdb_direct);

/**
 * target_submit_cmd - lookup unpacked lun and submit uninitialized se_cmd
 *
 * @se_cmd: command descriptor to submit
 * @se_sess: associated se_sess for endpoint
 * @cdb: pointer to SCSI CDB
 * @sense: pointer to SCSI sense buffer
 * @unpacked_lun: unpacked LUN to reference for struct se_lun
 * @data_length: fabric expected data transfer length
 * @task_addr: SAM task attribute
 * @data_dir: DMA data direction
 * @flags: flags for command submission from target_sc_flags_tables
 *
 * This may only be called from process context, and also currently
 * assumes internal allocation of fabric payload buffer by target-core.
 **/
void target_submit_cmd(struct se_cmd *se_cmd, struct se_session *se_sess,
		unsigned char *cdb, unsigned char *sense, u32 unpacked_lun,
		u32 data_length, int task_attr, int data_dir, int flags)
{
	struct se_portal_group *se_tpg;
	int rc;

	se_tpg = se_sess->se_tpg;
	BUG_ON(!se_tpg);
	BUG_ON(se_cmd->se_tfo || se_cmd->se_sess);
	BUG_ON(in_interrupt());
	/*
	 * Initialize se_cmd for target operation.  From this point
	 * exceptions are handled by sending exception status via
	 * target_core_fabric_ops->queue_status() callback
	 */
	transport_init_se_cmd(se_cmd, se_tpg->se_tpg_tfo, se_sess,
				data_length, data_dir, task_attr, sense);
	/*
	 * Obtain struct se_cmd->cmd_kref reference and add new cmd to
	 * se_sess->sess_cmd_list.  A second kref_get here is necessary
	 * for fabrics using TARGET_SCF_ACK_KREF that expect a second
	 * kref_put() to happen during fabric packet acknowledgement.
	 */
	target_get_sess_cmd(se_sess, se_cmd, (flags & TARGET_SCF_ACK_KREF));
	/*
	 * Signal bidirectional data payloads to target-core
	 */
	if (flags & TARGET_SCF_BIDI_OP)
		se_cmd->se_cmd_flags |= SCF_BIDI;
	/*
	 * Locate se_lun pointer and attach it to struct se_cmd
	 */
	if (transport_lookup_cmd_lun(se_cmd, unpacked_lun) < 0) {
		transport_send_check_condition_and_sense(se_cmd,
				se_cmd->scsi_sense_reason, 0);
		target_put_sess_cmd(se_sess, se_cmd);
		return;
	}
	/*
	 * Sanitize CDBs via transport_generic_cmd_sequencer() and
	 * allocate the necessary tasks to complete the received CDB+data
	 */
	rc = transport_generic_allocate_tasks(se_cmd, cdb);
	if (rc != 0) {
		transport_generic_request_failure(se_cmd);
		return;
	}
	/*
	 * Dispatch se_cmd descriptor to se_lun->lun_se_dev backend
	 * for immediate execution of READs, otherwise wait for
	 * transport_generic_handle_data() to be called for WRITEs
	 * when fabric has filled the incoming buffer.
	 */
	transport_handle_cdb_direct(se_cmd);
	return;
}
EXPORT_SYMBOL(target_submit_cmd);

static void target_complete_tmr_failure(struct work_struct *work)
{
	struct se_cmd *se_cmd = container_of(work, struct se_cmd, work);

	se_cmd->se_tmr_req->response = TMR_LUN_DOES_NOT_EXIST;
	se_cmd->se_tfo->queue_tm_rsp(se_cmd);
//	transport_generic_free_cmd(se_cmd, 0);  // Benjamin 20121030 patch for fixing double-free of se_cmd in target_complete_tmr_failure.
}

/**
 * target_submit_tmr - lookup unpacked lun and submit uninitialized se_cmd
 *                     for TMR CDBs
 *
 * @se_cmd: command descriptor to submit
 * @se_sess: associated se_sess for endpoint
 * @sense: pointer to SCSI sense buffer
 * @unpacked_lun: unpacked LUN to reference for struct se_lun
 * @fabric_context: fabric context for TMR req
 * @tm_type: Type of TM request
 * @gfp: gfp type for caller
 * @tag: referenced task tag for TMR_ABORT_TASK
 * @flags: submit cmd flags
 *
 * Callable from all contexts.
 **/

int target_submit_tmr(struct se_cmd *se_cmd, struct se_session *se_sess,
		unsigned char *sense, u32 unpacked_lun,
		void *fabric_tmr_ptr, unsigned char tm_type,
		gfp_t gfp, unsigned int tag, int flags)
{
	struct se_portal_group *se_tpg;
	int ret;

	se_tpg = se_sess->se_tpg;
	BUG_ON(!se_tpg);

	transport_init_se_cmd(se_cmd, se_tpg->se_tpg_tfo, se_sess,
			      0, DMA_NONE, MSG_SIMPLE_TAG, sense);
	/*
	 * FIXME: Currently expect caller to handle se_cmd->se_tmr_req
	 * allocation failure.
	 */
	ret = core_tmr_alloc_req(se_cmd, fabric_tmr_ptr, tm_type, gfp);
	if (ret < 0)
		return -ENOMEM;

	if (tm_type == TMR_ABORT_TASK)
		se_cmd->se_tmr_req->ref_task_tag = tag;

	/* See target_submit_cmd for commentary */
	target_get_sess_cmd(se_sess, se_cmd, (flags & TARGET_SCF_ACK_KREF));

	ret = transport_lookup_tmr_lun(se_cmd, unpacked_lun);
	if (ret) {
		/*
		 * For callback during failure handling, push this work off
		 * to process context with TMR_LUN_DOES_NOT_EXIST status.
		 */
		INIT_WORK(&se_cmd->work, target_complete_tmr_failure);
		schedule_work(&se_cmd->work);
		return 0;
	}
	transport_generic_handle_tmr(se_cmd);
	return 0;
}
EXPORT_SYMBOL(target_submit_tmr);

/*
 * Used by fabric module frontends defining a TFO->new_cmd_map() caller
 * to  queue up a newly setup se_cmd w/ TRANSPORT_NEW_CMD_MAP in order to
 * complete setup in TCM process context w/ TFO->new_cmd_map().
 */
int transport_generic_handle_cdb_map(
	struct se_cmd *cmd)
{
	if (!cmd->se_lun) {
		dump_stack();
		pr_err("cmd->se_lun is NULL\n");
		return -EINVAL;
	}

	transport_add_cmd_to_queue(cmd, TRANSPORT_NEW_CMD_MAP, false);
	return 0;
}
EXPORT_SYMBOL(transport_generic_handle_cdb_map);

/*	transport_generic_handle_data():
 *
 *
 */
int transport_generic_handle_data(
	struct se_cmd *cmd)
{
	/*
	 * For the software fabric case, then we assume the nexus is being
	 * failed/shutdown when signals are pending from the kthread context
	 * caller, so we return a failure.  For the HW target mode case running
	 * in interrupt code, the signal_pending() check is skipped.
	 */
	if (!in_interrupt() && signal_pending(current))
		return -EPERM;
	/*
	 * If the received CDB has aleady been ABORTED by the generic
	 * target engine, we now call transport_check_aborted_status()
	 * to queue any delated TASK_ABORTED status for the received CDB to the
	 * fabric module as we are expecting no further incoming DATA OUT
	 * sequences at this point.
	 */
	if (transport_check_aborted_status(cmd, 1) != 0)
		return 0;

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TPC_CMD)
	/* This code entry will come from iscsit_handle_data_out()
	 * 
	 * -1: not tpc cmd or other fatal error, 0: in progress, 1: not in progress 
	 */
	if (tpc_is_in_progress(cmd) == 0){
		/* SPC4R36, page 240 */
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = TCM_OPERATION_IN_PROGRESS;
		transport_generic_request_failure(cmd);
		return -EINVAL;
	}
#endif
#endif

	transport_add_cmd_to_queue(cmd, TRANSPORT_PROCESS_WRITE, false);
	return 0;
}
EXPORT_SYMBOL(transport_generic_handle_data);

/*	transport_generic_handle_tmr():
 *
 *
 */
int transport_generic_handle_tmr(
	struct se_cmd *cmd)
{
	transport_add_cmd_to_queue(cmd, TRANSPORT_PROCESS_TMR, false);
	return 0;
}
EXPORT_SYMBOL(transport_generic_handle_tmr);

/*
 * If the task is active, request it to be stopped and sleep until it
 * has completed.
 */
bool target_stop_task(struct se_task *task, unsigned long *flags)
{
	struct se_cmd *cmd = task->task_se_cmd;
	bool was_active = false;

	if (task->task_flags & TF_ACTIVE) {
		task->task_flags |= TF_REQUEST_STOP;
		spin_unlock_irqrestore(&cmd->t_state_lock, *flags);

		pr_debug("Task %p waiting to complete\n", task);
		wait_for_completion(&task->task_stop_comp);
		pr_debug("Task %p stopped successfully\n", task);

		spin_lock_irqsave(&cmd->t_state_lock, *flags);
		atomic_dec(&cmd->t_task_cdbs_left);

		task->task_flags &= ~(TF_ACTIVE | TF_REQUEST_STOP);
		was_active = true;
	}

	return was_active;
}

static int transport_stop_tasks_for_cmd(struct se_cmd *cmd)
{
	struct se_task *task, *task_tmp;
	unsigned long flags;
	int ret = 0;

	pr_debug("ITT[0x%08x] - Stopping tasks\n",
		cmd->se_tfo->get_task_tag(cmd));

	/*
	 * No tasks remain in the execution queue
	 */
	spin_lock_irqsave(&cmd->t_state_lock, flags);
	list_for_each_entry_safe(task, task_tmp,
				&cmd->t_task_list, t_list) {
		pr_debug("Processing task %p\n", task);

		/*
		 * If the struct se_task has not been sent and is not active,
		 * remove the struct se_task from the execution queue.
		 */
		if (!(task->task_flags & (TF_ACTIVE | TF_SENT))) {
			spin_unlock_irqrestore(&cmd->t_state_lock,
					flags);
			transport_remove_task_from_execute_queue(task,
					cmd->se_dev);

			pr_debug("Task %p removed from execute queue\n", task);
			spin_lock_irqsave(&cmd->t_state_lock, flags);
			continue;
		}

#if defined(CONFIG_MACH_QNAPTS) 
#if defined(SUPPORT_CONCURRENT_TASKS)
        /* TF_REQUEST_STOP may be set by another work thread so we need to
         * complete task when in different work thread at the same time.
         *
         * i.e.
         * work thread 1 wait the task 2 to be completed in target_stop_task()
         * and work thread 2 executes the task 2 in this function 
         */
		if (task->task_flags & TF_REQUEST_STOP){
			spin_unlock_irqrestore(&cmd->t_state_lock, flags);
			printk(KERN_DEBUG "task:0x%p in cmd:0x%p was requested "
			        "to be complete ...\n", task, cmd);
			complete(&task->task_stop_comp);
			printk(KERN_DEBUG "complete successfully\n");
			spin_lock_irqsave(&cmd->t_state_lock, flags);
			continue;
		}
#endif
#endif


		if (!target_stop_task(task, &flags)) {
			pr_debug("Task %p - did nothing\n", task);
			ret++;
		}
	}
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	return ret;
}

/*
 * Handle SAM-esque emulation for generic transport request failures.
 */
void transport_generic_request_failure(struct se_cmd *cmd)
{
	int ret = 0;

	pr_debug("-----[ Storage Engine Exception for cmd: %p ITT: 0x%08x"
		" CDB: 0x%02x\n", cmd, cmd->se_tfo->get_task_tag(cmd),
		cmd->t_task_cdb[0]);
	pr_debug("-----[ i_state: %d t_state: %d scsi_sense_reason: %d\n",
		cmd->se_tfo->get_cmd_state(cmd),
		cmd->t_state, cmd->scsi_sense_reason);
	pr_debug("-----[ t_tasks: %d t_task_cdbs_left: %d"
		" t_task_cdbs_sent: %d t_task_cdbs_ex_left: %d --"
		" CMD_T_ACTIVE: %d CMD_T_STOP: %d CMD_T_SENT: %d\n",
		cmd->t_task_list_num,
		atomic_read(&cmd->t_task_cdbs_left),
		atomic_read(&cmd->t_task_cdbs_sent),
		atomic_read(&cmd->t_task_cdbs_ex_left),
		(cmd->transport_state & CMD_T_ACTIVE) != 0,
		(cmd->transport_state & CMD_T_STOP) != 0,
		(cmd->transport_state & CMD_T_SENT) != 0);

	/*
	 * For SAM Task Attribute emulation for failed struct se_cmd
	 */
	if (cmd->se_dev->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
		transport_complete_task_attr(cmd);

	switch (cmd->scsi_sense_reason) {
	case TCM_NON_EXISTENT_LUN:
	case TCM_UNSUPPORTED_SCSI_OPCODE:
	case TCM_INVALID_CDB_FIELD:
	case TCM_INVALID_PARAMETER_LIST:
	case TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE:
	case TCM_UNKNOWN_MODE_PAGE:
	case TCM_WRITE_PROTECTED:
	case TCM_ADDRESS_OUT_OF_RANGE:
	case TCM_CHECK_CONDITION_ABORT_CMD:
	case TCM_CHECK_CONDITION_UNIT_ATTENTION:
	case TCM_CHECK_CONDITION_NOT_READY:

#if defined(CONFIG_MACH_QNAPTS) //Benjamin 20121105 sync VAAI support from Adam. 
	case TCM_MISCOMPARE_DURING_VERIFY_OP:
	case TCM_PARAMETER_LIST_LEN_ERROR:
	case TCM_UNREACHABLE_COPY_TARGET:
	case TCM_3RD_PARTY_DEVICE_FAILURE:
	case TCM_INCORRECT_COPY_TARGET_DEV_TYPE:
	case TCM_TOO_MANY_TARGET_DESCRIPTORS:
	case TCM_TOO_MANY_SEGMENT_DESCRIPTORS:
	case TCM_ILLEGAL_REQ_DATA_OVERRUN_COPY_TARGET:
	case TCM_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET:
	case TCM_COPY_ABORT_DATA_OVERRUN_COPY_TARGET:
	case TCM_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET:
	case TCM_INSUFFICIENT_RESOURCES:
	case TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD:
	case TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN:
	case TCM_OPERATION_IN_PROGRESS:
	case TCM_INVALID_TOKEN_OP_AND_INVALID_TOKEN_LEN:
	case TCM_INVALID_TOKEN_OP_AND_CAUSE_NOT_REPORTABLE:
	case TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_CREATION_NOT_SUPPORTED:
	case TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_USAGE_NOT_SUPPORTED:
	case TCM_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED:
	case TCM_INVALID_TOKEN_OP_AND_TOKEN_CORRUPT:
	case TCM_INVALID_TOKEN_OP_AND_TOKEN_DELETED:
	case TCM_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED:
	case TCM_INVALID_TOKEN_OP_AND_TOKEN_REVOKED:
	case TCM_INVALID_TOKEN_OP_AND_TOKEN_UNKNOWN:
	case TCM_INVALID_TOKEN_OP_AND_UNSUPPORTED_TOKEN_TYPE:
	case TCM_SPACE_ALLOCATION_FAILED_WRITE_PROTECT:
	case TCM_THIN_PROVISIONING_SOFT_THRESHOLD_REACHED:
	case TCM_CAPACITY_DATA_HAS_CHANGED:
#endif
		break;

	case TCM_OUT_OF_RESOURCES:
#if defined(CONFIG_MACH_QNAPTS)
        cmd->scsi_sense_reason = TCM_INSUFFICIENT_RESOURCES;
#else
        cmd->scsi_sense_reason = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
#endif
		break;

	case TCM_RESERVATION_CONFLICT:
		/*
		 * No SENSE Data payload for this case, set SCSI Status
		 * and queue the response to $FABRIC_MOD.
		 *
		 * Uses linux/include/scsi/scsi.h SAM status codes defs
		 */
		cmd->scsi_status = SAM_STAT_RESERVATION_CONFLICT;
		/*
		 * For UA Interlock Code 11b, a RESERVATION CONFLICT will
		 * establish a UNIT ATTENTION with PREVIOUS RESERVATION
		 * CONFLICT STATUS.
		 *
		 * See spc4r17, section 7.4.6 Control Mode Page, Table 349
		 */
		if (cmd->se_sess &&
		    cmd->se_dev->se_sub_dev->se_dev_attrib.emulate_ua_intlck_ctrl == 2)
			core_scsi3_ua_allocate(cmd->se_sess->se_node_acl,
				cmd->orig_fe_lun, 0x2C,
				ASCQ_2CH_PREVIOUS_RESERVATION_CONFLICT_STATUS);

		ret = cmd->se_tfo->queue_status(cmd);
		if (ret == -EAGAIN || ret == -ENOMEM)
			goto queue_full;
		goto check_stop;
	default:
		pr_err("Unknown transport error for CDB 0x%02x: %d\n",
			cmd->t_task_cdb[0], cmd->scsi_sense_reason);
		cmd->scsi_sense_reason = TCM_UNSUPPORTED_SCSI_OPCODE;
		break;
	}
	/*
	 * If a fabric does not define a cmd->se_tfo->new_cmd_map caller,
	 * make the call to transport_send_check_condition_and_sense()
	 * directly.  Otherwise expect the fabric to make the call to
	 * transport_send_check_condition_and_sense() after handling
	 * possible unsoliticied write data payloads.
	 */
	ret = transport_send_check_condition_and_sense(cmd,
			cmd->scsi_sense_reason, 0);
	if (ret == -EAGAIN || ret == -ENOMEM)
		goto queue_full;

check_stop:
	transport_lun_remove_cmd(cmd);
	if (!transport_cmd_check_stop_to_fabric(cmd))
		;
	return;

queue_full:
	cmd->t_state = TRANSPORT_COMPLETE_QF_OK;
	transport_handle_queue_full(cmd, cmd->se_dev);
}
EXPORT_SYMBOL(transport_generic_request_failure);

static inline u32 transport_lba_21(unsigned char *cdb)
{
	return ((cdb[1] & 0x1f) << 16) | (cdb[2] << 8) | cdb[3];
}

static inline u32 transport_lba_32(unsigned char *cdb)
{
	return (cdb[2] << 24) | (cdb[3] << 16) | (cdb[4] << 8) | cdb[5];
}

static inline unsigned long long transport_lba_64(unsigned char *cdb)
{
	unsigned int __v1, __v2;

	__v1 = (cdb[2] << 24) | (cdb[3] << 16) | (cdb[4] << 8) | cdb[5];
	__v2 = (cdb[6] << 24) | (cdb[7] << 16) | (cdb[8] << 8) | cdb[9];

	return ((unsigned long long)__v2) | (unsigned long long)__v1 << 32;
}

/*
 * For VARIABLE_LENGTH_CDB w/ 32 byte extended CDBs
 */
static inline unsigned long long transport_lba_64_ext(unsigned char *cdb)
{
	unsigned int __v1, __v2;

	__v1 = (cdb[12] << 24) | (cdb[13] << 16) | (cdb[14] << 8) | cdb[15];
	__v2 = (cdb[16] << 24) | (cdb[17] << 16) | (cdb[18] << 8) | cdb[19];

	return ((unsigned long long)__v2) | (unsigned long long)__v1 << 32;
}

static void transport_set_supported_SAM_opcode(struct se_cmd *se_cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&se_cmd->t_state_lock, flags);
	se_cmd->se_cmd_flags |= SCF_SUPPORTED_SAM_OPCODE;
	spin_unlock_irqrestore(&se_cmd->t_state_lock, flags);
}

/*
 * Called from Fabric Module context from transport_execute_tasks()
 *
 * The return of this function determins if the tasks from struct se_cmd
 * get added to the execution queue in transport_execute_tasks(),
 * or are added to the delayed or ordered lists here.
 */
static inline int transport_execute_task_attr(struct se_cmd *cmd)
{
	if (cmd->se_dev->dev_task_attr_type != SAM_TASK_ATTR_EMULATED)
		return 1;	
	/*
	 * Check for the existence of HEAD_OF_QUEUE, and if true return 1
	 * to allow the passed struct se_cmd list of tasks to the front of the list.
	 */
	 if (cmd->sam_task_attr == MSG_HEAD_TAG) {
		pr_debug("Added HEAD_OF_QUEUE for CDB:"
			" 0x%02x, se_ordered_id: %u\n",
			cmd->t_task_cdb[0],
			cmd->se_ordered_id);
		return 1;
	} else if (cmd->sam_task_attr == MSG_ORDERED_TAG) {
		atomic_inc(&cmd->se_dev->dev_ordered_sync);
		smp_mb__after_atomic_inc();

		pr_debug("Added ORDERED for CDB: 0x%02x to ordered"
				" list, se_ordered_id: %u\n",
				cmd->t_task_cdb[0],
				cmd->se_ordered_id);
		/*
		 * Add ORDERED command to tail of execution queue if
		 * no other older commands exist that need to be
		 * completed first.
		 */
		if (!atomic_read(&cmd->se_dev->simple_cmds))
			return 1;
	} else {
		/*
		 * For SIMPLE and UNTAGGED Task Attribute commands
		 */
		//  Benjamin 20121001: Write is a SIMPLE and UNTAGGED Task Attribute command.
		atomic_inc(&cmd->se_dev->simple_cmds);
		smp_mb__after_atomic_inc();
	}
	/*
	 * Otherwise if one or more outstanding ORDERED task attribute exist,
	 * add the dormant task(s) built for the passed struct se_cmd to the
	 * execution queue and become in Active state for this struct se_device.
	 */
	if (atomic_read(&cmd->se_dev->dev_ordered_sync) != 0) {
		/*
		 * Otherwise, add cmd w/ tasks to delayed cmd queue that
		 * will be drained upon completion of HEAD_OF_QUEUE task.
		 */		 
		spin_lock(&cmd->se_dev->delayed_cmd_lock);
		cmd->se_cmd_flags |= SCF_DELAYED_CMD_FROM_SAM_ATTR;
		list_add_tail(&cmd->se_delayed_node,
				&cmd->se_dev->delayed_cmd_list);
		spin_unlock(&cmd->se_dev->delayed_cmd_lock);

		pr_debug("Added CDB: 0x%02x Task Attr: 0x%02x to"
			" delayed CMD list, se_ordered_id: %u\n",
			cmd->t_task_cdb[0], cmd->sam_task_attr,
			cmd->se_ordered_id);
		/*
		 * Return zero to let transport_execute_tasks() know
		 * not to add the delayed tasks to the execution list.
		 */
		return 0;
	}
	/*
	 * Otherwise, no ORDERED task attributes exist..
	 */
	//  Benjamin 20121001: Write has no ORDERED task attributes exist.
	 
	return 1;
}

/*
 * Called from fabric module context in transport_generic_new_cmd() and
 * transport_generic_process_write()
 */
static int transport_execute_tasks(struct se_cmd *cmd)
{
	int add_tasks;
	struct se_device *se_dev = cmd->se_dev;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_CONCURRENT_TASKS)
	u32 len;

	/* 20130628, Jonathan Ho, add workqueue for executing iscsi tasks concurrently
	 *
	 * FIXED ME
	 * (1) This is magic for seq r/w for QNAP workqueue arch 
	 * (2) Not all scsi commands cdb[] field contains the lba / nr blks.
	 *     This code shall be modified in the future
	 */
	len = (se_dev->prev_len / se_dev->se_sub_dev->se_dev_attrib.block_size);
	cmd->use_wq = false;

	/* The WQ will be only used for normal read (6),(10),(12),(16) and
	 * write (6),(10),(12),(16) */
	if ((cmd->t_task_cdb[0] == READ_6) || (cmd->t_task_cdb[0] == READ_10)
	||  (cmd->t_task_cdb[0] == READ_12) || (cmd->t_task_cdb[0] == READ_16)
	||  (cmd->t_task_cdb[0] == WRITE_6) || (cmd->t_task_cdb[0] == WRITE_10)
	||  (cmd->t_task_cdb[0] == WRITE_12) || (cmd->t_task_cdb[0] == WRITE_16)
	)
	{
		if(cmd->t_task_lba == (se_dev->prev_lba + len)) 
			cmd->use_wq = false;
		else
			cmd->use_wq = true;
		
		se_dev->prev_lba = cmd->t_task_lba;
		se_dev->prev_len = cmd->data_length;
	}
#endif

	/*
	 * Call transport_cmd_check_stop() to see if a fabric exception
	 * has occurred that prevents execution.
	 */
	if (!transport_cmd_check_stop(cmd, 0, TRANSPORT_PROCESSING)) {
		/*
		 * Check for SAM Task Attribute emulation and HEAD_OF_QUEUE
		 * attribute for the tasks of the received struct se_cmd CDB
		 */
		add_tasks = transport_execute_task_attr(cmd);
		if (!add_tasks)
			goto execute_tasks;
		/*
		 * __transport_execute_tasks() -> __transport_add_tasks_from_cmd()
		 * adds associated se_tasks while holding dev->execute_task_lock
		 * before I/O dispath to avoid a double spinlock access.
		 */
		__transport_execute_tasks(se_dev, cmd);
		return 0;
	}

execute_tasks:
	__transport_execute_tasks(se_dev, NULL);
	return 0;
}

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_CONCURRENT_TASKS)
// 20130628, Jonathan Ho, add workqueue for executing iscsi tasks concurrently
static void transport_execute_tasks_wq(struct work_struct *work)
{
	struct se_task *task = 
		container_of(work, struct se_task, multi_tasks_work);
	struct se_cmd *cmd = task->task_se_cmd;
	struct se_device *dev = cmd->se_dev;
	int error;
	unsigned long flags;

	if (cmd->execute_task)
		error = cmd->execute_task(task);
	else
		error = dev->transport->do_task(task);

	if (error != 0) {
		spin_lock_irqsave(&cmd->t_state_lock, flags);
		task->task_flags &= ~TF_ACTIVE;
		cmd->transport_state &= ~CMD_T_SENT;
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);

		transport_stop_tasks_for_cmd(cmd);
		transport_generic_request_failure(cmd);
	}
}
#endif

/*
 * Called to check struct se_device tcq depth window, and once open pull struct se_task
 * from struct se_device->execute_task_list and
 *
 * Called from transport_processing_thread()
 */
static int __transport_execute_tasks(struct se_device *dev, struct se_cmd *new_cmd)
{
	int error;
	struct se_cmd *cmd = NULL;
	struct se_task *task = NULL;
	unsigned long flags;

check_depth:
	spin_lock_irq(&dev->execute_task_lock);
	if (new_cmd != NULL)
		__transport_add_tasks_from_cmd(new_cmd);

	if (list_empty(&dev->execute_task_list)) {
		spin_unlock_irq(&dev->execute_task_lock);
		return 0;
	}
	task = list_first_entry(&dev->execute_task_list,
				struct se_task, t_execute_list);

	__transport_remove_task_from_execute_queue(task, dev);

	spin_unlock_irq(&dev->execute_task_lock);

	cmd = task->task_se_cmd;
#if defined(CONFIG_MACH_QNAPTS)
	cmd->cur_se_task = task;
#endif

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	task->task_flags |= (TF_ACTIVE | TF_SENT);
	atomic_inc(&cmd->t_task_cdbs_sent);

	if (atomic_read(&cmd->t_task_cdbs_sent) ==
	    cmd->t_task_list_num)
		cmd->transport_state |= CMD_T_SENT;

	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

#if 0 // defined(SUPPORT_PARALLEL_TASK_WQ)
        /* not enable this option */
        spin_lock_irqsave(&dev->dev_r_task_lock, flags);
        atomic_inc(&dev->dev_r_task_cnt);
        list_add_tail(&task->t_node, &dev->dev_r_task_list);
        spin_unlock_irqrestore(&dev->dev_r_task_lock, flags);

//        printk("%s: r_cnt:0x%x\n", __func__, atomic_read(&dev->dev_r_task_cnt));
        wake_up_interruptible(&dev->p_task_thread_wq);
#endif


#if defined(CONFIG_MACH_QNAPTS)

	if (task->task_data_direction == DMA_TO_DEVICE) {

		spin_lock_irqsave(&dev->se_sub_dev->se_dev_lock, flags);
		if (dev->se_sub_dev->se_dev_attrib.syswp){

			spin_unlock_irqrestore(&dev->se_sub_dev->se_dev_lock, flags);

			/* if now is system write protected case, to skip any
			 * command for DMA_TO_DEVICE, and treat the cmd resp is
			 * GOOD. This modification is for host side.
			 * please refer redmine 12672,12789
			 */
			task->task_scsi_status = GOOD;
			transport_complete_task(task, 1);

			if (cmd->t_state & TRANSPORT_COMPLETE){

				SUBSYSTEM_TYPE type;
				__do_get_subsystem_dev_type(dev, &type);

				pr_warn("%s: detected write protected "
				"lu:0x%08x\n", 
				((type == SUBSYSTEM_BLOCK) ? "IBLOCK": \
				((type == SUBSYSTEM_FILE) ? "LIO(File I/O)" : \
				"Unknown Type")),
				cmd->orig_fe_lun);
			}
			new_cmd = NULL;
			goto check_depth;
		}

		spin_unlock_irqrestore(&dev->se_sub_dev->se_dev_lock, flags);
	}


#if defined(SUPPORT_CONCURRENT_TASKS)
	/* 20130628, Jonathan Ho, add workqueue for executing iscsi tasks concurrently */

	/* go workqueue if file i/o device */
	if(!strcmp(cmd->se_dev->transport->name, "fileio")) {
		if (cmd->use_wq == false)
			goto _NON_WQ_PATH_;

		INIT_WORK(&task->multi_tasks_work, transport_execute_tasks_wq);
		queue_work(multi_tasks_wq, &task->multi_tasks_work);
		new_cmd = NULL;
		goto check_depth;
	}

	/* (1) If not file i/o device, go this path
	 * (2) If file i/o device but not use wq, then go this path
	 */

_NON_WQ_PATH_:
#endif
#endif
	if (cmd->execute_task)
		error = cmd->execute_task(task);
	else
		error = dev->transport->do_task(task);

	if (error != 0) {
		spin_lock_irqsave(&cmd->t_state_lock, flags);
		task->task_flags &= ~TF_ACTIVE;
		cmd->transport_state &= ~CMD_T_SENT;
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		transport_stop_tasks_for_cmd(cmd);
		transport_generic_request_failure(cmd);
	}



	new_cmd = NULL;
	goto check_depth;

	return 0;
}

static inline u32 transport_get_sectors_6(
	unsigned char *cdb,
	struct se_cmd *cmd,
	int *ret)
{
	struct se_device *dev = cmd->se_dev;

	/*
	 * Assume TYPE_DISK for non struct se_device objects.
	 * Use 8-bit sector value.
	 */
	if (!dev)
		goto type_disk;

	/*
	 * Use 24-bit allocation length for TYPE_TAPE.
	 */
	if (dev->transport->get_device_type(dev) == TYPE_TAPE)
		return (u32)(cdb[2] << 16) + (cdb[3] << 8) + cdb[4];

	/*
	 * Everything else assume TYPE_DISK Sector CDB location.
	 * Use 8-bit sector value.  SBC-3 says:
	 *
	 *   A TRANSFER LENGTH field set to zero specifies that 256
	 *   logical blocks shall be written.  Any other value
	 *   specifies the number of logical blocks that shall be
	 *   written.
	 */
type_disk:
	return cdb[4] ? : 256;
}

static inline u32 transport_get_sectors_10(
	unsigned char *cdb,
	struct se_cmd *cmd,
	int *ret)
{
	struct se_device *dev = cmd->se_dev;

	/*
	 * Assume TYPE_DISK for non struct se_device objects.
	 * Use 16-bit sector value.
	 */
	if (!dev)
		goto type_disk;

	/*
	 * XXX_10 is not defined in SSC, throw an exception
	 */
	if (dev->transport->get_device_type(dev) == TYPE_TAPE) {
		*ret = -EINVAL;
		return 0;
	}

	/*
	 * Everything else assume TYPE_DISK Sector CDB location.
	 * Use 16-bit sector value.
	 */
type_disk:
	return (u32)(cdb[7] << 8) + cdb[8];
}

static inline u32 transport_get_sectors_12(
	unsigned char *cdb,
	struct se_cmd *cmd,
	int *ret)
{
	struct se_device *dev = cmd->se_dev;

	/*
	 * Assume TYPE_DISK for non struct se_device objects.
	 * Use 32-bit sector value.
	 */
	if (!dev)
		goto type_disk;

	/*
	 * XXX_12 is not defined in SSC, throw an exception
	 */
	if (dev->transport->get_device_type(dev) == TYPE_TAPE) {
		*ret = -EINVAL;
		return 0;
	}

	/*
	 * Everything else assume TYPE_DISK Sector CDB location.
	 * Use 32-bit sector value.
	 */
type_disk:
	return (u32)(cdb[6] << 24) + (cdb[7] << 16) + (cdb[8] << 8) + cdb[9];
}

static inline u32 transport_get_sectors_16(
	unsigned char *cdb,
	struct se_cmd *cmd,
	int *ret)
{
	struct se_device *dev = cmd->se_dev;

	/*
	 * Assume TYPE_DISK for non struct se_device objects.
	 * Use 32-bit sector value.
	 */
	if (!dev)
		goto type_disk;

	/*
	 * Use 24-bit allocation length for TYPE_TAPE.
	 */
	if (dev->transport->get_device_type(dev) == TYPE_TAPE)
		return (u32)(cdb[12] << 16) + (cdb[13] << 8) + cdb[14];

type_disk:
	return (u32)(cdb[10] << 24) + (cdb[11] << 16) +
		    (cdb[12] << 8) + cdb[13];
}

/*
 * Used for VARIABLE_LENGTH_CDB WRITE_32 and READ_32 variants
 */
static inline u32 transport_get_sectors_32(
	unsigned char *cdb,
	struct se_cmd *cmd,
	int *ret)
{
	/*
	 * Assume TYPE_DISK for non struct se_device objects.
	 * Use 32-bit sector value.
	 */
	return (u32)(cdb[28] << 24) + (cdb[29] << 16) +
		    (cdb[30] << 8) + cdb[31];

}

static inline u32 transport_get_size(
	u32 sectors,
	unsigned char *cdb,
	struct se_cmd *cmd)
{
	struct se_device *dev = cmd->se_dev;

	if (dev->transport->get_device_type(dev) == TYPE_TAPE) {
		if (cdb[1] & 1) { /* sectors */
			return dev->se_sub_dev->se_dev_attrib.block_size * sectors;
		} else /* bytes */
			return sectors;
	}
#if 0
	pr_debug("Returning block_size: %u, sectors: %u == %u for"
			" %s object\n", dev->se_sub_dev->se_dev_attrib.block_size, sectors,
			dev->se_sub_dev->se_dev_attrib.block_size * sectors,
			dev->transport->name);
#endif
	return dev->se_sub_dev->se_dev_attrib.block_size * sectors;
}

static void transport_xor_callback(struct se_cmd *cmd)
{
	unsigned char *buf, *addr;
	struct scatterlist *sg;
	unsigned int offset;
	int i;
	int count;
	/*
	 * From sbc3r22.pdf section 5.48 XDWRITEREAD (10) command
	 *
	 * 1) read the specified logical block(s);
	 * 2) transfer logical blocks from the data-out buffer;
	 * 3) XOR the logical blocks transferred from the data-out buffer with
	 *    the logical blocks read, storing the resulting XOR data in a buffer;
	 * 4) if the DISABLE WRITE bit is set to zero, then write the logical
	 *    blocks transferred from the data-out buffer; and
	 * 5) transfer the resulting XOR data to the data-in buffer.
	 */
	buf = kmalloc(cmd->data_length, GFP_KERNEL);
	if (!buf) {
		pr_err("Unable to allocate xor_callback buf\n");
		return;
	}
	/*
	 * Copy the scatterlist WRITE buffer located at cmd->t_data_sg
	 * into the locally allocated *buf
	 */
	sg_copy_to_buffer(cmd->t_data_sg,
			  cmd->t_data_nents,
			  buf,
			  cmd->data_length);

	/*
	 * Now perform the XOR against the BIDI read memory located at
	 * cmd->t_mem_bidi_list
	 */

	offset = 0;
	for_each_sg(cmd->t_bidi_data_sg, sg, cmd->t_bidi_data_nents, count) {
		addr = kmap_atomic(sg_page(sg));
		if (!addr)
			goto out;

		for (i = 0; i < sg->length; i++)
			*(addr + sg->offset + i) ^= *(buf + offset + i);

		offset += sg->length;
		kunmap_atomic(addr);
	}

out:
	kfree(buf);
}

/*
 * Used to obtain Sense Data from underlying Linux/SCSI struct scsi_cmnd
 */
static int transport_get_sense_data(struct se_cmd *cmd)
{
	unsigned char *buffer = cmd->sense_buffer, *sense_buffer = NULL;
	struct se_device *dev = cmd->se_dev;
	struct se_task *task = NULL, *task_tmp;
	unsigned long flags;
	u32 offset = 0;

	WARN_ON(!cmd->se_lun);

	if (!dev)
		return 0;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	if (cmd->se_cmd_flags & SCF_SENT_CHECK_CONDITION) {
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		return 0;
	}

	list_for_each_entry_safe(task, task_tmp,
				&cmd->t_task_list, t_list) {
		if (!(task->task_flags & TF_HAS_SENSE))
			continue;

		if (!dev->transport->get_sense_buffer) {
			pr_err("dev->transport->get_sense_buffer"
					" is NULL\n");
			continue;
		}

		sense_buffer = dev->transport->get_sense_buffer(task);
		if (!sense_buffer) {
			pr_err("ITT[0x%08x]_TASK[%p]: Unable to locate"
				" sense buffer for task with sense\n",
				cmd->se_tfo->get_task_tag(cmd), task);
			continue;
		}
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);

		offset = cmd->se_tfo->set_fabric_sense_len(cmd,
				TRANSPORT_SENSE_BUFFER);

		memcpy(&buffer[offset], sense_buffer,
				TRANSPORT_SENSE_BUFFER);
		cmd->scsi_status = task->task_scsi_status;
		/* Automatically padded */
		cmd->scsi_sense_length =
				(TRANSPORT_SENSE_BUFFER + offset);

		pr_debug("HBA_[%u]_PLUG[%s]: Set SAM STATUS: 0x%02x"
				" and sense\n",
			dev->se_hba->hba_id, dev->transport->name,
				cmd->scsi_status);
		return 0;
	}
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	return -1;
}

static inline long long transport_dev_end_lba(struct se_device *dev)
{
	return dev->transport->get_blocks(dev) + 1;
}

static int transport_cmd_get_valid_sectors(struct se_cmd *cmd)
{
	struct se_device *dev = cmd->se_dev;
	u32 sectors;

	if (dev->transport->get_device_type(dev) != TYPE_DISK)
		return 0;

	sectors = (cmd->data_length / dev->se_sub_dev->se_dev_attrib.block_size);

#if defined(CONFIG_MACH_QNAPTS)
	/* Benjamin 20121105 sync VAAI support from Adam. 
	 * Here shall handle the special scsi command like COMPARE AND WRITE, 
	 * WRITE SAME or others ...
	 */
#endif

	if ((cmd->t_task_lba + sectors) > transport_dev_end_lba(dev)) {
		pr_err("LBA: %llu Sectors: %u exceeds"
			" transport_dev_end_lba(): %llu\n",
			cmd->t_task_lba, sectors,
			transport_dev_end_lba(dev));
		return -EINVAL;
	}

	return 0;
}

static int target_check_write_same_discard(unsigned char *flags, struct se_device *dev)
{
	/*
	 * Determine if the received WRITE_SAME is used to for direct
	 * passthrough into Linux/SCSI with struct request via TCM/pSCSI
	 * or we are signaling the use of internal WRITE_SAME + UNMAP=1
	 * emulation for -> Linux/BLOCK disbard with TCM/IBLOCK code.
	 */
	int passthrough = (dev->transport->transport_type ==
				TRANSPORT_PLUGIN_PHBA_PDEV);

	if (!passthrough) {
		if ((flags[0] & 0x04) || (flags[0] & 0x02)) {
			pr_err("WRITE_SAME PBDATA and LBDATA"
				" bits not supported for Block Discard"
				" Emulation\n");
			return -ENOSYS;
		}
		/*
		 * Currently for the emulated case we only accept
		 * tpws with the UNMAP=1 bit set.
		 */
		if (!(flags[0] & 0x08)) {
			pr_err("WRITE_SAME w/o UNMAP bit not"
				" supported for Block Discard Emulation\n");
			return -ENOSYS;
		}
	}

	return 0;
}

/*	transport_generic_cmd_sequencer():
 *
 *	Generic Command Sequencer that should work for most DAS transport
 *	drivers.
 *
 *	Called from transport_generic_allocate_tasks() in the $FABRIC_MOD
 *	RX Thread.
 *
 *	FIXME: Need to support other SCSI OPCODES where as well.
 */
static int transport_generic_cmd_sequencer(
	struct se_cmd *cmd,
	unsigned char *cdb)
{
	struct se_device *dev = cmd->se_dev;
	struct se_subsystem_dev *su_dev = dev->se_sub_dev;
	int ret = 0, sector_ret = 0, passthrough;
	u32 sectors = 0, size = 0, pr_reg_type = 0;
	u16 service_action;
	u8 alua_ascq = 0;
	/*
	 * Check for an existing UNIT ATTENTION condition
	 */
	if (core_scsi3_ua_check(cmd, cdb) < 0) {
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = TCM_CHECK_CONDITION_UNIT_ATTENTION;
		return -EINVAL;
	}

	/*
	 * Check status of Asymmetric Logical Unit Assignment port
	 */
	ret = su_dev->t10_alua.alua_state_check(cmd, cdb, &alua_ascq);
	if (ret != 0) {
		/*
		 * Set SCSI additional sense code (ASC) to 'LUN Not Accessible';
		 * The ALUA additional sense code qualifier (ASCQ) is determined
		 * by the ALUA primary or secondary access state..
		 */
		if (ret > 0) {
#if 0
			pr_debug("[%s]: ALUA TG Port not available,"
				" SenseKey: NOT_READY, ASC/ASCQ: 0x04/0x%02x\n",
				cmd->se_tfo->get_fabric_name(), alua_ascq);
#endif
			transport_set_sense_codes(cmd, 0x04, alua_ascq);
			cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
			cmd->scsi_sense_reason = TCM_CHECK_CONDITION_NOT_READY;
			return -EINVAL;
		}
		goto out_invalid_cdb_field;
	}
	/*
	 * Check status for SPC-3 Persistent Reservations
	 */
	if (su_dev->t10_pr.pr_ops.t10_reservation_check(cmd, &pr_reg_type) != 0) {
		if (su_dev->t10_pr.pr_ops.t10_seq_non_holder(
					cmd, cdb, pr_reg_type) != 0) {
			cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
			cmd->se_cmd_flags |= SCF_SCSI_RESERVATION_CONFLICT;
			cmd->scsi_status = SAM_STAT_RESERVATION_CONFLICT;
			cmd->scsi_sense_reason = TCM_RESERVATION_CONFLICT;
			return -EBUSY;
		}
		/*
		 * This means the CDB is allowed for the SCSI Initiator port
		 * when said port is *NOT* holding the legacy SPC-2 or
		 * SPC-3 Persistent Reservation.
		 */
	}

	/*
	 * If we operate in passthrough mode we skip most CDB emulation and
	 * instead hand the commands down to the physical SCSI device.
	 */
	passthrough =
		(dev->transport->transport_type == TRANSPORT_PLUGIN_PHBA_PDEV);

#if defined(CONFIG_MACH_QNAPTS) //Benjamin 20121105 sync VAAI support from Adam. 
	pr_debug("cdb: %02x %02x %02x %02x %02x %02x %02x %02x "
		"%02x %02x %02x %02x %02x %02x %02x %02x\n",
		cdb[0],cdb[1],cdb[2],cdb[3], cdb[4],cdb[5],cdb[6],cdb[7],
		cdb[8],cdb[9],cdb[10],cdb[11], cdb[12],cdb[13],cdb[14],cdb[15]);
#endif

	switch (cdb[0]) {
	case READ_6:
		sectors = transport_get_sectors_6(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_21(cdb);
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
	case READ_10:
		sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_32(cdb);
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
	case READ_12:
		sectors = transport_get_sectors_12(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_32(cdb);
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
	case READ_16:
		sectors = transport_get_sectors_16(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_64(cdb);
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
	case WRITE_6:
		sectors = transport_get_sectors_6(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_21(cdb);
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
	case WRITE_10:
		sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_32(cdb);
		if (cdb[1] & 0x8)
			cmd->se_cmd_flags |= SCF_FUA;
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
	case WRITE_12:
		sectors = transport_get_sectors_12(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_32(cdb);
		if (cdb[1] & 0x8)
			cmd->se_cmd_flags |= SCF_FUA;
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
	case WRITE_16:
		sectors = transport_get_sectors_16(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_64(cdb);
		if (cdb[1] & 0x8)
			cmd->se_cmd_flags |= SCF_FUA;
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
#ifdef CONFIG_MACH_QNAPTS // Benjamin 20110223 for "WRITE and VERIFY"
    // FIXME: Note that these verify commands are work-around.
    case WRITE_VERIFY:  // WRITE AND VERIFY (10)
		sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_32(cdb);
		if (cdb[1] & 0x8)
			cmd->se_cmd_flags |= SCF_FUA;
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
	case WRITE_VERIFY_12:
		sectors = transport_get_sectors_12(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_32(cdb);
		if (cdb[1] & 0x8)
			cmd->se_cmd_flags |= SCF_FUA;
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
    // Benjamin 20110815 for "WRITE and VERIFY (16). See SBC3 5.38
	case WRITE_VERIFY_16:
		sectors = transport_get_sectors_16(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_64(cdb);
		if (cdb[1] & 0x8)
			cmd->se_cmd_flags |= SCF_FUA;
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;
		break;
#endif /* #ifdef CONFIG_MACH_QNAPTS  */          
	case XDWRITEREAD_10:
		if ((cmd->data_direction != DMA_TO_DEVICE) ||
		    !(cmd->se_cmd_flags & SCF_BIDI))
			goto out_invalid_cdb_field;
		sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;
		size = transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = transport_lba_32(cdb);
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;

		/*
		 * Do now allow BIDI commands for passthrough mode.
		 */
		if (passthrough)
			goto out_unsupported_cdb;

		/*
		 * Setup BIDI XOR callback to be run after I/O completion.
		 */
		cmd->transport_complete_callback = &transport_xor_callback;
		if (cdb[1] & 0x8)
			cmd->se_cmd_flags |= SCF_FUA;
		break;
	case VARIABLE_LENGTH_CMD:
		service_action = get_unaligned_be16(&cdb[8]);
		switch (service_action) {
		case XDWRITEREAD_32:
			sectors = transport_get_sectors_32(cdb, cmd, &sector_ret);
			if (sector_ret)
				goto out_unsupported_cdb;
			size = transport_get_size(sectors, cdb, cmd);
			/*
			 * Use WRITE_32 and READ_32 opcodes for the emulated
			 * XDWRITE_READ_32 logic.
			 */
			cmd->t_task_lba = transport_lba_64_ext(cdb);
			cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;

			/*
			 * Do now allow BIDI commands for passthrough mode.
			 */
			if (passthrough)
				goto out_unsupported_cdb;

			/*
			 * Setup BIDI XOR callback to be run during after I/O
			 * completion.
			 */
			cmd->transport_complete_callback = &transport_xor_callback;
			if (cdb[1] & 0x8)
				cmd->se_cmd_flags |= SCF_FUA;
			break;
		case WRITE_SAME_32:
			sectors = transport_get_sectors_32(cdb, cmd, &sector_ret);
			if (sector_ret)
				goto out_unsupported_cdb;

			if (sectors)
				size = transport_get_size(1, cdb, cmd);
			else {
				pr_err("WSNZ=1, WRITE_SAME w/sectors=0 not"
				       " supported\n");
				goto out_invalid_cdb_field;
			}

			cmd->t_task_lba = get_unaligned_be64(&cdb[12]);
			cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_VAAI) //Benjamin 20121105 sync VAAI support from Adam. 
			/* Pleaes refer the p205 for sbc3r35j version. It needs
			 * to report INVALID FILED IN CDB for Error Action Field 
			 */
			if (vaai_check_do_ws(&cdb[10], dev) < 0)
				goto out_invalid_cdb_field;
			if (!passthrough)
				cmd->execute_task = vaai_do_ws_v1;
#else
			if (target_check_write_same_discard(&cdb[10], dev) < 0)
				goto out_unsupported_cdb;
			if (!passthrough)
				cmd->execute_task = target_emulate_write_same;
#endif
			break;

		default:
			pr_err("VARIABLE_LENGTH_CMD service action"
				" 0x%04x not supported\n", service_action);
			goto out_unsupported_cdb;
		}
		break;
	case MAINTENANCE_IN:
		if (dev->transport->get_device_type(dev) != TYPE_ROM) {
			/* MAINTENANCE_IN from SCC-2 */
			/*
			 * Check for emulated MI_REPORT_TARGET_PGS.
			 */
			if (cdb[1] == MI_REPORT_TARGET_PGS &&
			    su_dev->t10_alua.alua_type == SPC3_ALUA_EMULATED) {
				cmd->execute_task =
					target_emulate_report_target_port_groups;
			}
			size = (cdb[6] << 24) | (cdb[7] << 16) |
			       (cdb[8] << 8) | cdb[9];
		} else {
			/* GPCMD_SEND_KEY from multi media commands */
			size = (cdb[8] << 8) + cdb[9];
		}
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case MODE_SELECT:
		size = cdb[4];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case MODE_SELECT_10:
		size = (cdb[7] << 8) + cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case MODE_SENSE:
		size = cdb[4];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		if (!passthrough)
			cmd->execute_task = target_emulate_modesense;
		break;
	case MODE_SENSE_10:
		size = (cdb[7] << 8) + cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		if (!passthrough)
			cmd->execute_task = target_emulate_modesense;
		break;
	case GPCMD_READ_BUFFER_CAPACITY:
	case GPCMD_SEND_OPC:
	case LOG_SELECT:
	case LOG_SENSE:
		size = (cdb[7] << 8) + cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
#if defined(CONFIG_MACH_QNAPTS)
        if (cdb[0] == LOG_SENSE){
            if (!passthrough)
                cmd->execute_task = target_emulate_logsense;
        }

#endif		
		break;
	case READ_BLOCK_LIMITS:
		size = READ_BLOCK_LEN;
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case GPCMD_GET_CONFIGURATION:
	case GPCMD_READ_FORMAT_CAPACITIES:
	case GPCMD_READ_DISC_INFO:
	case GPCMD_READ_TRACK_RZONE_INFO:
		size = (cdb[7] << 8) + cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case PERSISTENT_RESERVE_IN:
		if (su_dev->t10_pr.res_type == SPC3_PERSISTENT_RESERVATIONS)
			cmd->execute_task = target_scsi3_emulate_pr_in;
		size = (cdb[7] << 8) + cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case PERSISTENT_RESERVE_OUT:
		if (su_dev->t10_pr.res_type == SPC3_PERSISTENT_RESERVATIONS)
			cmd->execute_task = target_scsi3_emulate_pr_out;
		size = (cdb[7] << 8) + cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case GPCMD_MECHANISM_STATUS:
	case GPCMD_READ_DVD_STRUCTURE:
		size = (cdb[8] << 8) + cdb[9];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case READ_POSITION:
		size = READ_POSITION_LEN;
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case MAINTENANCE_OUT:
		if (dev->transport->get_device_type(dev) != TYPE_ROM) {
			/* MAINTENANCE_OUT from SCC-2
			 *
			 * Check for emulated MO_SET_TARGET_PGS.
			 */
			if (cdb[1] == MO_SET_TARGET_PGS &&
			    su_dev->t10_alua.alua_type == SPC3_ALUA_EMULATED) {
				cmd->execute_task =
					target_emulate_set_target_port_groups;
			}

			size = (cdb[6] << 24) | (cdb[7] << 16) |
			       (cdb[8] << 8) | cdb[9];
		} else  {
			/* GPCMD_REPORT_KEY from multi media commands */
			size = (cdb[8] << 8) + cdb[9];
		}
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case INQUIRY:
		size = (cdb[3] << 8) + cdb[4];
		/*
		 * Do implict HEAD_OF_QUEUE processing for INQUIRY.
		 * See spc4r17 section 5.3
		 */
		if (cmd->se_dev->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
			cmd->sam_task_attr = MSG_HEAD_TAG;
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		if (!passthrough)
			cmd->execute_task = target_emulate_inquiry;
		break;
	case READ_BUFFER:
		size = (cdb[6] << 16) + (cdb[7] << 8) + cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case READ_CAPACITY:
		size = READ_CAP_LEN;
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		if (!passthrough)
			cmd->execute_task = target_emulate_readcapacity;
		break;
	case READ_MEDIA_SERIAL_NUMBER:
	case SECURITY_PROTOCOL_IN:
	case SECURITY_PROTOCOL_OUT:
		size = (cdb[6] << 24) | (cdb[7] << 16) | (cdb[8] << 8) | cdb[9];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case SERVICE_ACTION_IN:
		switch (cmd->t_task_cdb[1] & 0x1f) {
		case SAI_READ_CAPACITY_16:
			if (!passthrough)
				cmd->execute_task =
					target_emulate_readcapacity_16;
			break;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_TP)
/* Jonathan Ho, 20141225, GET_LBA_STATUS for X31 without SUPPORT_VOLUME_BASED */
#if defined(SUPPORT_VOLUME_BASED) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26))
		case SAI_GET_LBA_STATUS:
			if (!passthrough)
				cmd->execute_task = target_emulate_getlbastatus;
			break;
#endif /* defined(SUPPORT_VOLUME_BASED) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26)) */
#endif
		default:
			if (passthrough)
				break;

			pr_err("Unsupported SA: 0x%02x\n",
				cmd->t_task_cdb[1] & 0x1f);
			goto out_invalid_cdb_field;
		}
		/*FALLTHROUGH*/
	case ACCESS_CONTROL_IN:
	case ACCESS_CONTROL_OUT:
	case EXTENDED_COPY:
	case READ_ATTRIBUTE:
	case RECEIVE_COPY_RESULTS:
	case WRITE_ATTRIBUTE:
		size = (cdb[10] << 24) | (cdb[11] << 16) |
		       (cdb[12] << 8) | cdb[13];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;

#if defined(CONFIG_MACH_QNAPTS)
//Benjamin 20121105 sync VAAI support from Adam. 
		if((RECEIVE_COPY_RESULTS == cdb[0]) || (EXTENDED_COPY == cdb[0])){
			if (!passthrough){
				ret = __isntall_tpc_proc(cdb, 
						(void**)&cmd->execute_task);
				if (ret == -1){
					pr_err("TARGET_CORE[%s]: Unsupported "
						"SCSI cmd for 3rd-party copy "
						"operation. cdb[0]:0x%02x, "
						"cdb[1]:0x%02x\n",
						cmd->se_tfo->get_fabric_name(), 
						cdb[0], cdb[1]);
					goto out_unsupported_cdb;
				}
			}
		}
#endif
		break;
	case RECEIVE_DIAGNOSTIC:
	case SEND_DIAGNOSTIC:
		size = (cdb[3] << 8) | cdb[4];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
/* #warning FIXME: Figure out correct GPCMD_READ_CD blocksize. */
#if 0
	case GPCMD_READ_CD:
		sectors = (cdb[6] << 16) + (cdb[7] << 8) + cdb[8];
		size = (2336 * sectors);
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
#endif
	case READ_TOC:
		size = cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case REQUEST_SENSE:
		size = cdb[4];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		if (!passthrough)
			cmd->execute_task = target_emulate_request_sense;
		break;
	case READ_ELEMENT_STATUS:
		size = 65536 * cdb[7] + 256 * cdb[8] + cdb[9];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case WRITE_BUFFER:
		size = (cdb[6] << 16) + (cdb[7] << 8) + cdb[8];
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;
	case RESERVE:
	case RESERVE_10:
		/*
		 * The SPC-2 RESERVE does not contain a size in the SCSI CDB.
		 * Assume the passthrough or $FABRIC_MOD will tell us about it.
		 */
		if (cdb[0] == RESERVE_10)
			size = (cdb[7] << 8) | cdb[8];
		else
			size = cmd->data_length;

		/*
		 * Setup the legacy emulated handler for SPC-2 and
		 * >= SPC-3 compatible reservation handling (CRH=1)
		 * Otherwise, we assume the underlying SCSI logic is
		 * is running in SPC_PASSTHROUGH, and wants reservations
		 * emulation disabled.
		 */
		if (su_dev->t10_pr.res_type != SPC_PASSTHROUGH)
			cmd->execute_task = target_scsi2_reservation_reserve;

		cmd->se_cmd_flags |= SCF_SCSI_NON_DATA_CDB;
		break;
	case RELEASE:
	case RELEASE_10:
		/*
		 * The SPC-2 RELEASE does not contain a size in the SCSI CDB.
		 * Assume the passthrough or $FABRIC_MOD will tell us about it.
		*/
		if (cdb[0] == RELEASE_10)
			size = (cdb[7] << 8) | cdb[8];
		else
			size = cmd->data_length;

		if (su_dev->t10_pr.res_type != SPC_PASSTHROUGH)
			cmd->execute_task = target_scsi2_reservation_release;
		cmd->se_cmd_flags |= SCF_SCSI_NON_DATA_CDB;
		break;
	case SYNCHRONIZE_CACHE:
	case SYNCHRONIZE_CACHE_16:
		/*
		 * Extract LBA and range to be flushed for emulated SYNCHRONIZE_CACHE
		 */
		if (cdb[0] == SYNCHRONIZE_CACHE) {
			sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
			cmd->t_task_lba = transport_lba_32(cdb);
		} else {
			sectors = transport_get_sectors_16(cdb, cmd, &sector_ret);
			cmd->t_task_lba = transport_lba_64(cdb);
		}
		if (sector_ret)
			goto out_unsupported_cdb;

		size = transport_get_size(sectors, cdb, cmd);
		cmd->se_cmd_flags |= SCF_SCSI_NON_DATA_CDB;

		if (passthrough)
			break;

		/*
		 * Check to ensure that LBA + Range does not exceed past end of
		 * device for IBLOCK and FILEIO ->do_sync_cache() backend calls
		 */
		if ((cmd->t_task_lba != 0) || (sectors != 0)) {
			if (transport_cmd_get_valid_sectors(cmd) < 0)
				goto out_invalid_cdb_field;
		}
		cmd->execute_task = target_emulate_synchronize_cache;
		break;
	case UNMAP:
		size = get_unaligned_be16(&cdb[7]);
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		if (!passthrough)
			cmd->execute_task = target_emulate_unmap;
		break;
	case WRITE_SAME_16:
		sectors = transport_get_sectors_16(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;

		if (sectors)
			size = transport_get_size(1, cdb, cmd);
		else {
			pr_err("WSNZ=1, WRITE_SAME w/sectors=0 not supported\n");
			goto out_invalid_cdb_field;
		}

		cmd->t_task_lba = get_unaligned_be64(&cdb[2]);
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_VAAI) //Benjamin 20121105 sync VAAI support from Adam. 
		/* Pleaes refer the p205 for sbc3r35j version. It needs to 
		 * report INVALID FILED IN CDB for Error Action Field */
		if (vaai_check_do_ws(&cdb[1], dev) < 0)
			goto out_invalid_cdb_field;
		if (!passthrough)
			cmd->execute_task = vaai_do_ws_v1;
#else
		if (target_check_write_same_discard(&cdb[1], dev) < 0)
			goto out_unsupported_cdb;
		if (!passthrough)
			cmd->execute_task = target_emulate_write_same;
#endif  /* #if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_VAAI) */
		break;
	case WRITE_SAME:
		sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
		if (sector_ret)
			goto out_unsupported_cdb;

		if (sectors)
			size = transport_get_size(1, cdb, cmd);
		else {
			pr_err("WSNZ=1, WRITE_SAME w/sectors=0 not supported\n");
			goto out_invalid_cdb_field;
		}

		cmd->t_task_lba = get_unaligned_be32(&cdb[2]);
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_VAAI) //Benjamin 20121105 sync VAAI support from Adam. 
		/* Pleaes refer the p205 for sbc3r35j version. It needs to 
		 * report INVALID FILED IN CDB for Error Action Field */
		if (vaai_check_do_ws(&cdb[1], dev) < 0)
			goto out_invalid_cdb_field;
		if (!passthrough)
			cmd->execute_task = vaai_do_ws_v1;
#else
		/*
		 * Follow sbcr26 with WRITE_SAME (10) and check for the existence
		 * of byte 1 bit 3 UNMAP instead of original reserved field
		 */
		if (target_check_write_same_discard(&cdb[1], dev) < 0)
			goto out_unsupported_cdb;
		if (!passthrough)
			cmd->execute_task = target_emulate_write_same;
#endif
		break;

#if defined(CONFIG_MACH_QNAPTS) && defined(SUPPORT_VAAI) //Benjamin 20121105 sync VAAI support from Adam. 
	case SCSI_COMPARE_AND_WRITE:
		sectors = (u32)cdb[13];
		/* there are two instance buffers */
		size = 2 * transport_get_size(sectors, cdb, cmd);
		cmd->t_task_lba = get_unaligned_be64(&cdb[2]);
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		if (!passthrough)
			cmd->execute_task = vaai_do_ats_v1;
        break;
#endif

	case VERIFY:
#ifdef CONFIG_MACH_QNAPTS // Benjamin 20110815 for "VERIFY_16"
	case VERIFY_16:

		/* 2014/01/13, support verify(10),(16) for HCK 2.1 (ver:8.100.26063) */
		/* sbc3r35j, p173 */

		if (VERIFY == cdb[0]){
			sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
			cmd->t_task_lba = transport_lba_32(cdb);
		}
		else{
			sectors = transport_get_sectors_16(cdb, cmd, &sector_ret);
			cmd->t_task_lba = transport_lba_64(cdb);
		}

		if (sector_ret)
			goto out_unsupported_cdb;

		size = transport_get_size(sectors, cdb, cmd);

		pr_debug("cdb[0]:0x%x, cdb[1]:0x%x, size:0x%x, "
			"cmd->data_length:0x%x\n",cdb[0], cdb[1], 
			size, cmd->data_length);

		switch ((cdb[1] & 0x06)){
		case 0x00:
			/* Specification said data-out buffer will NOT be used
			 * but NOT said whether the host will carry the
			 * data-out buffer or not
			 *
			 * FIXED ME
			 * So to set the size and cmd->data_length to zero here
			 * , they shall be modified (or fixed) in the future
			 */
			WARN_ON((cmd->data_length != 0));
			size = 0;
			cmd->se_cmd_flags |= SCF_SCSI_NON_DATA_CDB;
			break;
		case 0x02:
			/* data len is the same as VERIFICATION LENGTH field */
		case 0x04:
			/* sbc3r35j, p173, not defined this */
		default:
			/* 0x06, sbc3r35j, p174, data len is one logical block size */
			if ((cdb[1] & 0x06) == 0x06)
				size = transport_get_size(1, cdb, cmd);			

			cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
			break;
		}

		if (!passthrough)
			cmd->execute_task = __verify_10_16;

		break;
#endif
	case ALLOW_MEDIUM_REMOVAL:
	case ERASE:
	case REZERO_UNIT:
	case SEEK_10:
	case SPACE:
	case START_STOP:
	case TEST_UNIT_READY:
	case WRITE_FILEMARKS:
		cmd->se_cmd_flags |= SCF_SCSI_NON_DATA_CDB;
		if (!passthrough)
			cmd->execute_task = target_emulate_noop;
		break;
	case GPCMD_CLOSE_TRACK:
	case INITIALIZE_ELEMENT_STATUS:
	case GPCMD_LOAD_UNLOAD:
	case GPCMD_SET_SPEED:
	case MOVE_MEDIUM:
		cmd->se_cmd_flags |= SCF_SCSI_NON_DATA_CDB;
		break;
	case REPORT_LUNS:
		cmd->execute_task = target_report_luns;
		size = (cdb[6] << 24) | (cdb[7] << 16) | (cdb[8] << 8) | cdb[9];
		/*
		 * Do implict HEAD_OF_QUEUE processing for REPORT_LUNS
		 * See spc4r17 section 5.3
		 */
		if (cmd->se_dev->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
			cmd->sam_task_attr = MSG_HEAD_TAG;
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		break;

	default:
		pr_warn("TARGET_CORE[%s]: Unsupported SCSI Opcode"
			" 0x%02x, sending CHECK_CONDITION.\n",
			cmd->se_tfo->get_fabric_name(), cdb[0]);
		goto out_unsupported_cdb;
	}

	if (size != cmd->data_length) {
		pr_warn("TARGET_CORE[%s]: Expected Transfer Length:"
			" %u does not match SCSI CDB Length: %u for SAM Opcode:"
			" 0x%02x\n", cmd->se_tfo->get_fabric_name(),
				cmd->data_length, size, cdb[0]);
#ifdef CONFIG_MACH_QNAPTS // 2010/07/12 Nike Chen   // FIXME: Check again...
        size = (0 == size) ? cmd->data_length : size;
#endif
		cmd->cmd_spdtl = size;

		if (cmd->data_direction == DMA_TO_DEVICE) {
			pr_err("Rejecting underflow/overflow"
					" WRITE data\n");
			goto out_invalid_cdb_field;
		}
		/*
		 * Reject READ_* or WRITE_* with overflow/underflow for
		 * type SCF_SCSI_DATA_SG_IO_CDB.
		 */
		if (!ret && (dev->se_sub_dev->se_dev_attrib.block_size != 512))  {
			pr_err("Failing OVERFLOW/UNDERFLOW for LBA op"
				" CDB on non 512-byte sector setup subsystem"
				" plugin: %s\n", dev->transport->name);
			/* Returns CHECK_CONDITION + INVALID_CDB_FIELD */
			goto out_invalid_cdb_field;
		}
		/*
		 * For the overflow case keep the existing fabric provided
		 * ->data_length.  Otherwise for the underflow case, reset
		 * ->data_length to the smaller SCSI expected data transfer
		 * length.
		 */
		if (size > cmd->data_length) {
			cmd->se_cmd_flags |= SCF_OVERFLOW_BIT;
			cmd->residual_count = (size - cmd->data_length);
		} else {
			cmd->se_cmd_flags |= SCF_UNDERFLOW_BIT;
			cmd->residual_count = (cmd->data_length - size);
    		cmd->data_length = size;
		}
	}

	if (transport_check_sectors_exceeds_max_limits_blks(cmd, sectors) < 0)
		goto out_invalid_cdb_field;

	/* reject any command that we don't have a handler for */
	if (!(passthrough || cmd->execute_task ||
	     (cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB)))
		goto out_unsupported_cdb;

	transport_set_supported_SAM_opcode(cmd);
	return ret;

out_unsupported_cdb:
	cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
	cmd->scsi_sense_reason = TCM_UNSUPPORTED_SCSI_OPCODE;
	return -EINVAL;
out_invalid_cdb_field:
	cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
	cmd->scsi_sense_reason = TCM_INVALID_CDB_FIELD;
	return -EINVAL;
}

/*
 * Called from I/O completion to determine which dormant/delayed
 * and ordered cmds need to have their tasks added to the execution queue.
 */
static void transport_complete_task_attr(struct se_cmd *cmd)
{
	struct se_device *dev = cmd->se_dev;
	struct se_cmd *cmd_p, *cmd_tmp;
	int new_active_tasks = 0;

	if (cmd->sam_task_attr == MSG_SIMPLE_TAG) {
		atomic_dec(&dev->simple_cmds);
		smp_mb__after_atomic_dec();
		dev->dev_cur_ordered_id++;
		pr_debug("Incremented dev->dev_cur_ordered_id: %u for"
			" SIMPLE: %u\n", dev->dev_cur_ordered_id,
			cmd->se_ordered_id);
	} else if (cmd->sam_task_attr == MSG_HEAD_TAG) {
		dev->dev_cur_ordered_id++;
		pr_debug("Incremented dev_cur_ordered_id: %u for"
			" HEAD_OF_QUEUE: %u\n", dev->dev_cur_ordered_id,
			cmd->se_ordered_id);
	} else if (cmd->sam_task_attr == MSG_ORDERED_TAG) {
		atomic_dec(&dev->dev_ordered_sync);
		smp_mb__after_atomic_dec();

		dev->dev_cur_ordered_id++;
		pr_debug("Incremented dev_cur_ordered_id: %u for ORDERED:"
			" %u\n", dev->dev_cur_ordered_id, cmd->se_ordered_id);
	}
	/*
	 * Process all commands up to the last received
	 * ORDERED task attribute which requires another blocking
	 * boundary
	 */
	spin_lock(&dev->delayed_cmd_lock);
	list_for_each_entry_safe(cmd_p, cmd_tmp,
			&dev->delayed_cmd_list, se_delayed_node) {

		list_del(&cmd_p->se_delayed_node);
		spin_unlock(&dev->delayed_cmd_lock);

		pr_debug("Calling add_tasks() for"
			" cmd_p: 0x%02x Task Attr: 0x%02x"
			" Dormant -> Active, se_ordered_id: %u\n",
			cmd_p->t_task_cdb[0],
			cmd_p->sam_task_attr, cmd_p->se_ordered_id);

		transport_add_tasks_from_cmd(cmd_p);
		new_active_tasks++;

		spin_lock(&dev->delayed_cmd_lock);
		if (cmd_p->sam_task_attr == MSG_ORDERED_TAG)
			break;
	}
	spin_unlock(&dev->delayed_cmd_lock);
	/*
	 * If new tasks have become active, wake up the transport thread
	 * to do the processing of the Active tasks.
	 */
	if (new_active_tasks != 0)
		wake_up_interruptible(&dev->dev_queue_obj.thread_wq);
}

static void transport_complete_qf(struct se_cmd *cmd)
{
	int ret = 0;

	if (cmd->se_dev->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
		transport_complete_task_attr(cmd);

	if (cmd->se_cmd_flags & SCF_TRANSPORT_TASK_SENSE) {
		ret = cmd->se_tfo->queue_status(cmd);
		if (ret)
			goto out;
	}

	switch (cmd->data_direction) {
	case DMA_FROM_DEVICE:
		ret = cmd->se_tfo->queue_data_in(cmd);
		break;
	case DMA_TO_DEVICE:
		if (cmd->t_bidi_data_sg) {
			ret = cmd->se_tfo->queue_data_in(cmd);
			if (ret < 0)
				break;
		}
		/* Fall through for DMA_TO_DEVICE */
	case DMA_NONE:
		ret = cmd->se_tfo->queue_status(cmd);
		break;
	default:
		break;
	}

out:
	if (ret < 0) {
		transport_handle_queue_full(cmd, cmd->se_dev);
		return;
	}
	transport_lun_remove_cmd(cmd);
	transport_cmd_check_stop_to_fabric(cmd);
}

static void transport_handle_queue_full(
	struct se_cmd *cmd,
	struct se_device *dev)
{
	spin_lock_irq(&dev->qf_cmd_lock);
	list_add_tail(&cmd->se_qf_node, &cmd->se_dev->qf_cmd_list);
	atomic_inc(&dev->dev_qf_count);
	smp_mb__after_atomic_inc();
	spin_unlock_irq(&cmd->se_dev->qf_cmd_lock);

	schedule_work(&cmd->se_dev->qf_work_queue);
}

static void target_complete_ok_work(struct work_struct *work)
{
	struct se_cmd *cmd = container_of(work, struct se_cmd, work);
	int reason = 0, ret;

	/*
	 * Check if we need to move delayed/dormant tasks from cmds on the
	 * delayed execution list after a HEAD_OF_QUEUE or ORDERED Task
	 * Attribute.
	 */

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_CONCURRENT_TASKS)
	/* Avoid to access the NULL of se_lun here when code executed
	 * transport_send_check_condition_and_sense() already under
	 * concurrent task mechanism
	 */
	unsigned long flags;
	spin_lock_irqsave(&cmd->t_state_lock, flags);
	if (cmd->se_cmd_flags & SCF_SENT_CHECK_CONDITION){
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		pr_info("[%s] cmd(ITT:0x%8x) got the "
			"SCF_SENT_CHECK_CONDITION, to exit\n",
			__func__, cmd->se_tfo->get_task_tag(cmd));
		transport_lun_remove_cmd(cmd);
		transport_cmd_check_stop_to_fabric(cmd);
		return;
	}
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	if (!cmd->se_lun){
		pr_info("[%s] cmd(ITT:0x%8x) se_lun is NULL already, "
			"to exit\n", __func__, cmd->se_tfo->get_task_tag(cmd));
		transport_lun_remove_cmd(cmd);
		transport_cmd_check_stop_to_fabric(cmd);
		return;
	}
#endif
#endif

	if (cmd->se_dev->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
		transport_complete_task_attr(cmd);
	/*
	 * Check to schedule QUEUE_FULL work, or execute an existing
	 * cmd->transport_qf_callback()
	 */
	if (atomic_read(&cmd->se_dev->dev_qf_count) != 0)
		schedule_work(&cmd->se_dev->qf_work_queue);

	/*
	 * Check if we need to retrieve a sense buffer from
	 * the struct se_cmd in question.
	 */
	if (cmd->se_cmd_flags & SCF_TRANSPORT_TASK_SENSE) {
		if (transport_get_sense_data(cmd) < 0)
			reason = TCM_NON_EXISTENT_LUN;

		/*
		 * Only set when an struct se_task->task_scsi_status returned
		 * a non GOOD status.
		 */
		if (cmd->scsi_status) {
			ret = transport_send_check_condition_and_sense(
					cmd, reason, 1);
			if (ret == -EAGAIN || ret == -ENOMEM)
				goto queue_full;

			transport_lun_remove_cmd(cmd);
			transport_cmd_check_stop_to_fabric(cmd);
			return;
		}
	}

	/*
	 * Check for a callback, used by amongst other things
	 * XDWRITE_READ_10 emulation.
	 */
	if (cmd->transport_complete_callback)
		cmd->transport_complete_callback(cmd);

	switch (cmd->data_direction) {
	case DMA_FROM_DEVICE:
		spin_lock(&cmd->se_lun->lun_sep_lock);
		if (cmd->se_lun->lun_sep) {
			cmd->se_lun->lun_sep->sep_stats.tx_data_octets +=
					cmd->data_length;
		}
		spin_unlock(&cmd->se_lun->lun_sep_lock);

		ret = cmd->se_tfo->queue_data_in(cmd);
		if (ret == -EAGAIN || ret == -ENOMEM)
			goto queue_full;
		break;
	case DMA_TO_DEVICE:
		spin_lock(&cmd->se_lun->lun_sep_lock);
		if (cmd->se_lun->lun_sep) {
			cmd->se_lun->lun_sep->sep_stats.rx_data_octets +=
				cmd->data_length;
		}
		spin_unlock(&cmd->se_lun->lun_sep_lock);
		/*
		 * Check if we need to send READ payload for BIDI-COMMAND
		 */
		if (cmd->t_bidi_data_sg) {
			spin_lock(&cmd->se_lun->lun_sep_lock);
			if (cmd->se_lun->lun_sep) {
				cmd->se_lun->lun_sep->sep_stats.tx_data_octets +=
					cmd->data_length;
			}
			spin_unlock(&cmd->se_lun->lun_sep_lock);
			ret = cmd->se_tfo->queue_data_in(cmd);
			if (ret == -EAGAIN || ret == -ENOMEM)
				goto queue_full;
			break;
		}
		/* Fall through for DMA_TO_DEVICE */
	case DMA_NONE:
		ret = cmd->se_tfo->queue_status(cmd);
		if (ret == -EAGAIN || ret == -ENOMEM)
			goto queue_full;
		break;
	default:
		break;
	}

	transport_lun_remove_cmd(cmd);
	transport_cmd_check_stop_to_fabric(cmd);
	return;

queue_full:
	pr_debug("Handling complete_ok QUEUE_FULL: se_cmd: %p,"
		" data_direction: %d\n", cmd, cmd->data_direction);
	cmd->t_state = TRANSPORT_COMPLETE_QF_OK;
	transport_handle_queue_full(cmd, cmd->se_dev);
}

static void transport_free_dev_tasks(struct se_cmd *cmd)
{
	struct se_task *task, *task_tmp;
	unsigned long flags;
	LIST_HEAD(dispose_list);

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	list_for_each_entry_safe(task, task_tmp,
				&cmd->t_task_list, t_list) {

#if defined(CONFIG_MACH_QNAPTS)
		/* 2014/08/16, adamhsu, redmine 9055,9076,9278 */
		if (task->task_flags & TF_ACTIVE)
			pr_err("[%s] - cmd(ITT:0x%8x), task still stay in "
				"TF_ACTIVE\n", __func__, 
				cmd->se_tfo->get_task_tag(cmd));
#endif

		if (!(task->task_flags & TF_ACTIVE))
			list_move_tail(&task->t_list, &dispose_list);
	}
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	while (!list_empty(&dispose_list)) {
		task = list_first_entry(&dispose_list, struct se_task, t_list);

		if (task->task_sg != cmd->t_data_sg &&
		    task->task_sg != cmd->t_bidi_data_sg)
			kfree(task->task_sg);

		list_del(&task->t_list);

		if (cmd->se_dev == NULL)
			printk("Trace: [SCSI] [***] $cmd->se_dev had been freed\n");
		else if (cmd->se_dev->transport == NULL)
			printk("Trace: [SCSI] [***] $cmd->se_dev->transport had been freed\n");
		else
			cmd->se_dev->transport->free_task(task);
	}
}

static inline void transport_free_sgl(struct scatterlist *sgl, int nents)
{
	struct scatterlist *sg;
	int count;

	for_each_sg(sgl, sg, nents, count)
		__free_page(sg_page(sg));

	kfree(sgl);
}

static inline void transport_free_pages(struct se_cmd *cmd)
{
	if (cmd->se_cmd_flags & SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC)
		return;

	transport_free_sgl(cmd->t_data_sg, cmd->t_data_nents);
	cmd->t_data_sg = NULL;
	cmd->t_data_nents = 0;

	transport_free_sgl(cmd->t_bidi_data_sg, cmd->t_bidi_data_nents);
	cmd->t_bidi_data_sg = NULL;
	cmd->t_bidi_data_nents = 0;
}

/**
 * transport_release_cmd - free a command
 * @cmd:       command to free
 *
 * This routine unconditionally frees a command, and reference counting
 * or list removal must be done in the caller.
 */
static void transport_release_cmd(struct se_cmd *cmd)
{
	BUG_ON(!cmd->se_tfo);

	if (cmd->se_cmd_flags & SCF_SCSI_TMR_CDB)
		core_tmr_release_req(cmd->se_tmr_req);
	if (cmd->t_task_cdb != cmd->__t_task_cdb)
		kfree(cmd->t_task_cdb);

	/*
	 * If this cmd has been setup with target_get_sess_cmd(), drop
	 * the kref and call ->release_cmd() in kref callback.
	 */
	 if (cmd->check_release != 0) {
		target_put_sess_cmd(cmd->se_sess, cmd);
		return;
	}

	cmd->se_tfo->release_cmd(cmd);
}

/**
 * transport_put_cmd - release a reference to a command
 * @cmd:       command to release
 *
 * This routine releases our reference to the command and frees it if possible.
 */
static void transport_put_cmd(struct se_cmd *cmd)
{
	unsigned long flags;
	int free_tasks = 0;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	if (atomic_read(&cmd->t_fe_count)) {
		if (!atomic_dec_and_test(&cmd->t_fe_count))
			goto out_busy;
	}

	if (atomic_read(&cmd->t_se_count)) {
		if (!atomic_dec_and_test(&cmd->t_se_count))
			goto out_busy;
	}

	if (cmd->transport_state & CMD_T_DEV_ACTIVE) {
		cmd->transport_state &= ~CMD_T_DEV_ACTIVE;
		transport_all_task_dev_remove_state(cmd);
		free_tasks = 1;
	}
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	if (free_tasks != 0)
		transport_free_dev_tasks(cmd);

	transport_free_pages(cmd);
	transport_release_cmd(cmd);
	return;
out_busy:
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);
}

/*
 * transport_generic_map_mem_to_cmd - Use fabric-alloced pages instead of
 * allocating in the core.
 * @cmd:  Associated se_cmd descriptor
 * @mem:  SGL style memory for TCM WRITE / READ
 * @sg_mem_num: Number of SGL elements
 * @mem_bidi_in: SGL style memory for TCM BIDI READ
 * @sg_mem_bidi_num: Number of BIDI READ SGL elements
 *
 * Return: nonzero return cmd was rejected for -ENOMEM or inproper usage
 * of parameters.
 */
int transport_generic_map_mem_to_cmd(
	struct se_cmd *cmd,
	struct scatterlist *sgl,
	u32 sgl_count,
	struct scatterlist *sgl_bidi,
	u32 sgl_bidi_count)
{
	if (!sgl || !sgl_count)
		return 0;

	if ((cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB) ||
	    (cmd->se_cmd_flags & SCF_SCSI_CONTROL_SG_IO_CDB)) {
		/*
		 * Reject SCSI data overflow with map_mem_to_cmd() as incoming
		 * scatterlists already have been set to follow what the fabric
		 * passes for the original expected data transfer length.
		 */
		if (cmd->se_cmd_flags & SCF_OVERFLOW_BIT) {
			pr_warn("Rejecting SCSI DATA overflow for fabric using"
				" SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC\n");
			cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
			cmd->scsi_sense_reason = TCM_INVALID_CDB_FIELD;
			return -EINVAL;
		}

		cmd->t_data_sg = sgl;
		cmd->t_data_nents = sgl_count;

		if (sgl_bidi && sgl_bidi_count) {
			cmd->t_bidi_data_sg = sgl_bidi;
			cmd->t_bidi_data_nents = sgl_bidi_count;
		}
		cmd->se_cmd_flags |= SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC;
	}

	return 0;
}
EXPORT_SYMBOL(transport_generic_map_mem_to_cmd);

void *transport_kmap_data_sg(struct se_cmd *cmd)
{
	struct scatterlist *sg = cmd->t_data_sg;
	struct page **pages;
	int i;

	BUG_ON(!sg);
	/*
	 * We need to take into account a possible offset here for fabrics like
	 * tcm_loop who may be using a contig buffer from the SCSI midlayer for
	 * control CDBs passed as SGLs via transport_generic_map_mem_to_cmd()
	 */
	if (!cmd->t_data_nents)
		return NULL;
	else if (cmd->t_data_nents == 1)
		return kmap(sg_page(sg)) + sg->offset;

	/* >1 page. use vmap */
	pages = kmalloc(sizeof(*pages) * cmd->t_data_nents, GFP_KERNEL);
	if (!pages)
		return NULL;

	/* convert sg[] to pages[] */
	for_each_sg(cmd->t_data_sg, sg, cmd->t_data_nents, i) {
		pages[i] = sg_page(sg);
	}

	cmd->t_data_vmap = vmap(pages, cmd->t_data_nents,  VM_MAP, PAGE_KERNEL);
	kfree(pages);
	if (!cmd->t_data_vmap)
		return NULL;

	return cmd->t_data_vmap + cmd->t_data_sg[0].offset;
}
EXPORT_SYMBOL(transport_kmap_data_sg);

void transport_kunmap_data_sg(struct se_cmd *cmd)
{
	if (!cmd->t_data_nents) {
		return;
	} else if (cmd->t_data_nents == 1) {
		kunmap(sg_page(cmd->t_data_sg));
		return;
	}

	vunmap(cmd->t_data_vmap);
	cmd->t_data_vmap = NULL;
}
EXPORT_SYMBOL(transport_kunmap_data_sg);

static int
transport_generic_get_mem(struct se_cmd *cmd)
{
	u32 length = cmd->data_length;
	unsigned int nents;
	struct page *page;
	gfp_t zero_flag;
	int i = 0;

	nents = DIV_ROUND_UP(length, PAGE_SIZE);
	cmd->t_data_sg = kmalloc(sizeof(struct scatterlist) * nents, GFP_KERNEL);
	if (!cmd->t_data_sg)
		return -ENOMEM;

	cmd->t_data_nents = nents;
	sg_init_table(cmd->t_data_sg, nents);

	zero_flag = cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB ? 0 : __GFP_ZERO;

	while (length) {
		u32 page_len = min_t(u32, length, PAGE_SIZE);
		page = alloc_page(GFP_KERNEL | zero_flag);
		if (!page)
			goto out;

		sg_set_page(&cmd->t_data_sg[i], page, page_len, 0);
		length -= page_len;
		i++;
	}
	return 0;

out:
	while (i >= 0) {
		__free_page(sg_page(&cmd->t_data_sg[i]));
		i--;
	}
	kfree(cmd->t_data_sg);
	cmd->t_data_sg = NULL;
	return -ENOMEM;
}

/* Reduce sectors if they are too long for the device */
static inline sector_t transport_limit_task_sectors(
	struct se_device *dev,
	unsigned long long lba,
	sector_t sectors)
{
	sectors = min_t(sector_t, sectors, dev->se_sub_dev->se_dev_attrib.max_sectors);

	if (dev->transport->get_device_type(dev) == TYPE_DISK)
		if ((lba + sectors) > transport_dev_end_lba(dev))
			sectors = ((transport_dev_end_lba(dev) - lba) + 1);

	return sectors;
}


/*
 * This function can be used by HW target mode drivers to create a linked
 * scatterlist from all contiguously allocated struct se_task->task_sg[].
 * This is intended to be called during the completion path by TCM Core
 * when struct target_core_fabric_ops->check_task_sg_chaining is enabled.
 */
void transport_do_task_sg_chain(struct se_cmd *cmd)
{
	struct scatterlist *sg_first = NULL;
	struct scatterlist *sg_prev = NULL;
	int sg_prev_nents = 0;
	struct scatterlist *sg;
	struct se_task *task;
	u32 chained_nents = 0;
	int i;

	BUG_ON(!cmd->se_tfo->task_sg_chaining);

	/*
	 * Walk the struct se_task list and setup scatterlist chains
	 * for each contiguously allocated struct se_task->task_sg[].
	 */
	list_for_each_entry(task, &cmd->t_task_list, t_list) {
		if (!task->task_sg)
			continue;

		if (!sg_first) {
			sg_first = task->task_sg;
			chained_nents = task->task_sg_nents;
		} else {
			sg_chain(sg_prev, sg_prev_nents, task->task_sg);
			chained_nents += task->task_sg_nents;
		}
		/*
		 * For the padded tasks, use the extra SGL vector allocated
		 * in transport_allocate_data_tasks() for the sg_prev_nents
		 * offset into sg_chain() above.
		 *
		 * We do not need the padding for the last task (or a single
		 * task), but in that case we will never use the sg_prev_nents
		 * value below which would be incorrect.
		 */
		sg_prev_nents = (task->task_sg_nents + 1);
		sg_prev = task->task_sg;
	}
	/*
	 * Setup the starting pointer and total t_tasks_sg_linked_no including
	 * padding SGs for linking and to mark the end.
	 */
	cmd->t_tasks_sg_chained = sg_first;
	cmd->t_tasks_sg_chained_no = chained_nents;

	pr_debug("Setup cmd: %p cmd->t_tasks_sg_chained: %p and"
		" t_tasks_sg_chained_no: %u\n", cmd, cmd->t_tasks_sg_chained,
		cmd->t_tasks_sg_chained_no);

	for_each_sg(cmd->t_tasks_sg_chained, sg,
			cmd->t_tasks_sg_chained_no, i) {

		pr_debug("SG[%d]: %p page: %p length: %d offset: %d\n",
			i, sg, sg_page(sg), sg->length, sg->offset);
		if (sg_is_chain(sg))
			pr_debug("SG: %p sg_is_chain=1\n", sg);
		if (sg_is_last(sg))
			pr_debug("SG: %p sg_is_last=1\n", sg);
	}
}
EXPORT_SYMBOL(transport_do_task_sg_chain);

/*
 * Break up cmd into chunks transport can handle
 */
static int
transport_allocate_data_tasks(struct se_cmd *cmd,
	enum dma_data_direction data_direction,
	struct scatterlist *cmd_sg, unsigned int sgl_nents)
{
	struct se_device *dev = cmd->se_dev;
	int task_count, i;
	unsigned long long lba;
	sector_t sectors, dev_max_sectors;
	u32 sector_size;


	if (transport_cmd_get_valid_sectors(cmd) < 0)
		return -EINVAL;

	dev_max_sectors = dev->se_sub_dev->se_dev_attrib.max_sectors;
	sector_size = dev->se_sub_dev->se_dev_attrib.block_size;

	WARN_ON(cmd->data_length % sector_size);

	lba = cmd->t_task_lba;
	sectors = DIV_ROUND_UP(cmd->data_length, sector_size);
	task_count = DIV_ROUND_UP_SECTOR_T(sectors, dev_max_sectors);

	/*
	 * If we need just a single task reuse the SG list in the command
	 * and avoid a lot of work.
	 */
	if (task_count == 1) {
		struct se_task *task;
		unsigned long flags;

		task = transport_generic_get_task(cmd, data_direction);
		if (!task)
			return -ENOMEM;

		task->task_sg = cmd_sg;
		task->task_sg_nents = sgl_nents;

		task->task_lba = lba;
		task->task_sectors = sectors;
		task->task_size = task->task_sectors * sector_size;

		spin_lock_irqsave(&cmd->t_state_lock, flags);
		list_add_tail(&task->t_list, &cmd->t_task_list);
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);

		return task_count;
	}

	for (i = 0; i < task_count; i++) {
		struct se_task *task;
		unsigned int task_size, task_sg_nents_padded;
		struct scatterlist *sg;
		unsigned long flags;
		int count;

		task = transport_generic_get_task(cmd, data_direction);
		if (!task)
			return -ENOMEM;

		task->task_lba = lba;
		task->task_sectors = min(sectors, dev_max_sectors);
		task->task_size = task->task_sectors * sector_size;

		/*
		 * This now assumes that passed sg_ents are in PAGE_SIZE chunks
		 * in order to calculate the number per task SGL entries
		 */
		task->task_sg_nents = DIV_ROUND_UP(task->task_size, PAGE_SIZE);

		/*
		 * Check if the fabric module driver is requesting that all
		 * struct se_task->task_sg[] be chained together..  If so,
		 * then allocate an extra padding SG entry for linking and
		 * marking the end of the chained SGL for every task except
		 * the last one for (task_count > 1) operation, or skipping
		 * the extra padding for the (task_count == 1) case.
		 */
		if (cmd->se_tfo->task_sg_chaining && (i < (task_count - 1))) {
			task_sg_nents_padded = (task->task_sg_nents + 1);
		} else
			task_sg_nents_padded = task->task_sg_nents;

		task->task_sg = kmalloc(sizeof(struct scatterlist) *
					task_sg_nents_padded, GFP_KERNEL);
		if (!task->task_sg) {
			cmd->se_dev->transport->free_task(task);
			return -ENOMEM;
		}

		sg_init_table(task->task_sg, task_sg_nents_padded);

		task_size = task->task_size;

		/* Build new sgl, only up to task_size */

		for_each_sg(task->task_sg, sg, task->task_sg_nents, count) {

			if (cmd_sg->length > task_size)
				break;


			*sg = *cmd_sg;
#if defined(CONFIG_MACH_QNAPTS)
			/* The sg end bit will be overwrited by 
			 * '*sg = *cmd_sg'. To mark it again to avoid to access
			 * the invalid kernel paging request after to call
			 * sg_next() for last one
			 */
			if (task_sg_nents_padded == task->task_sg_nents){
				if (count == (task->task_sg_nents - 1))
					sg_mark_end(sg);
			}
#endif
			task_size -= cmd_sg->length;
			cmd_sg = sg_next(cmd_sg);
		}


		lba += task->task_sectors;
		sectors -= task->task_sectors;

		spin_lock_irqsave(&cmd->t_state_lock, flags);
		list_add_tail(&task->t_list, &cmd->t_task_list);
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
	}

	return task_count;
}

static int
transport_allocate_control_task(struct se_cmd *cmd)
{
	struct se_task *task;
	unsigned long flags;

	/* Workaround for handling zero-length control CDBs */
	if ((cmd->se_cmd_flags & SCF_SCSI_CONTROL_SG_IO_CDB) &&
	    !cmd->data_length)
		return 0;

	task = transport_generic_get_task(cmd, cmd->data_direction);
	if (!task)
		return -ENOMEM;

	task->task_sg = cmd->t_data_sg;
	task->task_size = cmd->data_length;
	task->task_sg_nents = cmd->t_data_nents;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	list_add_tail(&task->t_list, &cmd->t_task_list);
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	/* Success! Return number of tasks allocated */
	return 1;
}

/*
 * Allocate any required ressources to execute the command, and either place
 * it on the execution queue if possible.  For writes we might not have the
 * payload yet, thus notify the fabric via a call to ->write_pending instead.
 */
int transport_generic_new_cmd(struct se_cmd *cmd)
{
	struct se_device *dev = cmd->se_dev;
	int task_cdbs, task_cdbs_bidi = 0;
	int set_counts = 1;
	int ret = 0;

	/*
	 * Determine is the TCM fabric module has already allocated physical
	 * memory, and is directly calling transport_generic_map_mem_to_cmd()
	 * beforehand.
	 */
	if (!(cmd->se_cmd_flags & SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC) &&
	    cmd->data_length) {
		ret = transport_generic_get_mem(cmd);
		if (ret < 0)
			goto out_fail;
	}

	/*
	 * For BIDI command set up the read tasks first.
	 */
	if (cmd->t_bidi_data_sg &&
	    dev->transport->transport_type != TRANSPORT_PLUGIN_PHBA_PDEV) {
		BUG_ON(!(cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB));

		task_cdbs_bidi = transport_allocate_data_tasks(cmd,
				DMA_FROM_DEVICE, cmd->t_bidi_data_sg,
				cmd->t_bidi_data_nents);
		if (task_cdbs_bidi <= 0)
			goto out_fail;

		atomic_inc(&cmd->t_fe_count);
		atomic_inc(&cmd->t_se_count);
		set_counts = 0;
	}

	if (cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB) {
		task_cdbs = transport_allocate_data_tasks(cmd,
					cmd->data_direction, cmd->t_data_sg,
					cmd->t_data_nents);
	} else {
		task_cdbs = transport_allocate_control_task(cmd);
	}

	if (task_cdbs < 0)
		goto out_fail;
	else if (!task_cdbs && (cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB)) {
		spin_lock_irq(&cmd->t_state_lock);
		cmd->t_state = TRANSPORT_COMPLETE;
		cmd->transport_state |= CMD_T_ACTIVE;
		spin_unlock_irq(&cmd->t_state_lock);

		if (cmd->t_task_cdb[0] == REQUEST_SENSE) {
			u8 ua_asc = 0, ua_ascq = 0;

			core_scsi3_ua_clear_for_request_sense(cmd,
					&ua_asc, &ua_ascq);
		}
		INIT_WORK(&cmd->work, target_complete_ok_work);
		queue_work(target_completion_wq, &cmd->work);
		return 0;
	}

	if (set_counts) {
		atomic_inc(&cmd->t_fe_count);
		atomic_inc(&cmd->t_se_count);
	}

	cmd->t_task_list_num = (task_cdbs + task_cdbs_bidi);
	atomic_set(&cmd->t_task_cdbs_left, cmd->t_task_list_num);
	atomic_set(&cmd->t_task_cdbs_ex_left, cmd->t_task_list_num);

#if defined(CONFIG_MACH_QNAPTS) 
	/* adamhsu, 
	 * (1) redmine bug 6915 - bugzilla 40743
	 * (2) redmine bug 6916 - bugzilla 41183
	 */
	__create_cmd_rec(cmd);
#endif

	/*
	 * For WRITEs, let the fabric know its buffer is ready..
	 * This WRITE struct se_cmd (and all of its associated struct se_task's)
	 * will be added to the struct se_device execution queue after its WRITE
	 * data has arrived. (ie: It gets handled by the transport processing
	 * thread a second time)
	 */
	if (cmd->data_direction == DMA_TO_DEVICE) {
		transport_add_tasks_to_state_queue(cmd);
		return transport_generic_write_pending(cmd);
	}
	/*
	 * Everything else but a WRITE, add the struct se_cmd's struct se_task's
	 * to the execution queue.
	 */
	transport_execute_tasks(cmd);

	return 0;

out_fail:
	cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
	cmd->scsi_sense_reason = TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE;
	return -EINVAL;
}
EXPORT_SYMBOL(transport_generic_new_cmd);

/*	transport_generic_process_write():
 *
 *
 */
void transport_generic_process_write(struct se_cmd *cmd)
{
#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TPC_CMD)
	if(tpc_is_to_cancel_rod_token_func1(cmd) == 1)
		tpc_is_to_cancel_rod_token_func2(cmd);
#endif
#endif
	transport_execute_tasks(cmd);
}
EXPORT_SYMBOL(transport_generic_process_write);

static void transport_write_pending_qf(struct se_cmd *cmd)
{
	int ret;

	ret = cmd->se_tfo->write_pending(cmd);
	if (ret == -EAGAIN || ret == -ENOMEM) {
		pr_debug("Handling write_pending QUEUE__FULL: se_cmd: %p\n",
			 cmd);
		transport_handle_queue_full(cmd, cmd->se_dev);
	}
}

static int transport_generic_write_pending(struct se_cmd *cmd)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	cmd->t_state = TRANSPORT_WRITE_PENDING;
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	/*
	 * Clear the se_cmd for WRITE_PENDING status in order to set
	 * CMD_T_ACTIVE so that transport_generic_handle_data can be called
	 * from HW target mode interrupt code.  This is safe to be called
	 * with transport_off=1 before the cmd->se_tfo->write_pending
	 * because the se_cmd->se_lun pointer is not being cleared.
	 */
	transport_cmd_check_stop(cmd, 1, 0);

	/*
	 * Call the fabric write_pending function here to let the
	 * frontend know that WRITE buffers are ready.
	 */
	ret = cmd->se_tfo->write_pending(cmd);
	if (ret == -EAGAIN || ret == -ENOMEM)
		goto queue_full;
	else if (ret < 0)
		return ret;

	return 1;

queue_full:
	pr_debug("Handling write_pending QUEUE__FULL: se_cmd: %p\n", cmd);
	cmd->t_state = TRANSPORT_COMPLETE_QF_WP;
	transport_handle_queue_full(cmd, cmd->se_dev);
	return 0;
}

void transport_generic_free_cmd(struct se_cmd *cmd, int wait_for_tasks)
{
#if defined(CONFIG_MACH_QNAPTS) 
	/* adamhsu, 
	 * (1) redmine bug 6915 - bugzilla 40743
	 * (2) redmine bug 6916 - bugzilla 41183
	 */
	__remove_cmd_rec(cmd);
#endif

	if (!(cmd->se_cmd_flags & SCF_SE_LUN_CMD)) {
		if (wait_for_tasks && (cmd->se_cmd_flags & SCF_SCSI_TMR_CDB))
			 transport_wait_for_tasks(cmd);

		transport_release_cmd(cmd);
	} else {
		if (wait_for_tasks)
			transport_wait_for_tasks(cmd);

		core_dec_lacl_count(cmd->se_sess->se_node_acl, cmd);

		if (cmd->se_lun)
			transport_lun_remove_cmd(cmd);

#if defined(CONFIG_MACH_QNAPTS) 
		if (!cmd->se_dev)
			pr_debug("%s - cmd:0x%p, cmd->se_dev is NULL\n",	
				__FUNCTION__, cmd);
		else{
			pr_debug("%s - cmd:0x%p, cmd->se_dev is NOT NULL\n", 
					__FUNCTION__, cmd);
			transport_free_dev_tasks(cmd);
		}
#else
		transport_free_dev_tasks(cmd);
#endif
		transport_put_cmd(cmd);
	}
}
EXPORT_SYMBOL(transport_generic_free_cmd);

/* target_get_sess_cmd - Add command to active ->sess_cmd_list
 * @se_sess:	session to reference
 * @se_cmd:	command descriptor to add
 * @ack_kref:	Signal that fabric will perform an ack target_put_sess_cmd()
 */
void target_get_sess_cmd(struct se_session *se_sess, struct se_cmd *se_cmd,
			bool ack_kref)
{
	unsigned long flags;

	kref_init(&se_cmd->cmd_kref);
	/*
	 * Add a second kref if the fabric caller is expecting to handle
	 * fabric acknowledgement that requires two target_put_sess_cmd()
	 * invocations before se_cmd descriptor release.
	 */
	if (ack_kref == true) {
		kref_get(&se_cmd->cmd_kref);
		se_cmd->se_cmd_flags |= SCF_ACK_KREF;
	}

	spin_lock_irqsave(&se_sess->sess_cmd_lock, flags);
	list_add_tail(&se_cmd->se_cmd_list, &se_sess->sess_cmd_list);
	se_cmd->check_release = 1;
	spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);
}
EXPORT_SYMBOL(target_get_sess_cmd);

static void target_release_cmd_kref(struct kref *kref)
{
	struct se_cmd *se_cmd = container_of(kref, struct se_cmd, cmd_kref);
	struct se_session *se_sess = se_cmd->se_sess;
	unsigned long flags;

	spin_lock_irqsave(&se_sess->sess_cmd_lock, flags);
	if (list_empty(&se_cmd->se_cmd_list)) {
		spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);
		se_cmd->se_tfo->release_cmd(se_cmd);
		return;
	}

	if (se_sess->sess_tearing_down && se_cmd->cmd_wait_set) {
		spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);
		complete(&se_cmd->cmd_wait_comp);
		return;
	}
	list_del(&se_cmd->se_cmd_list);
	spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);

	se_cmd->se_tfo->release_cmd(se_cmd);

}

/* target_put_sess_cmd - Check for active I/O shutdown via kref_put
 * @se_sess:	session to reference
 * @se_cmd:	command descriptor to drop
 */
int target_put_sess_cmd(struct se_session *se_sess, struct se_cmd *se_cmd)
{
	return kref_put(&se_cmd->cmd_kref, target_release_cmd_kref);
}
EXPORT_SYMBOL(target_put_sess_cmd);

/* target_splice_sess_cmd_list - Split active cmds into sess_wait_list
 * @se_sess:	session to split
 */
void target_splice_sess_cmd_list(struct se_session *se_sess)
{
	struct se_cmd *se_cmd;
	unsigned long flags;

	WARN_ON(!list_empty(&se_sess->sess_wait_list));
	INIT_LIST_HEAD(&se_sess->sess_wait_list);

	spin_lock_irqsave(&se_sess->sess_cmd_lock, flags);
	se_sess->sess_tearing_down = 1;

	list_splice_init(&se_sess->sess_cmd_list, &se_sess->sess_wait_list);

	list_for_each_entry(se_cmd, &se_sess->sess_wait_list, se_cmd_list)
		se_cmd->cmd_wait_set = 1;

	spin_unlock_irqrestore(&se_sess->sess_cmd_lock, flags);
}
EXPORT_SYMBOL(target_splice_sess_cmd_list);

/* target_wait_for_sess_cmds - Wait for outstanding descriptors
 * @se_sess:    session to wait for active I/O
 * @wait_for_tasks:	Make extra transport_wait_for_tasks call
 */
void target_wait_for_sess_cmds(
	struct se_session *se_sess,
	int wait_for_tasks)
{
	struct se_cmd *se_cmd, *tmp_cmd;
	bool rc = false;

	list_for_each_entry_safe(se_cmd, tmp_cmd,
				&se_sess->sess_wait_list, se_cmd_list) {
		list_del(&se_cmd->se_cmd_list);

		pr_debug("Waiting for se_cmd: %p t_state: %d, fabric state:"
			" %d\n", se_cmd, se_cmd->t_state,
			se_cmd->se_tfo->get_cmd_state(se_cmd));

		if (wait_for_tasks) {
			pr_debug("Calling transport_wait_for_tasks se_cmd: %p t_state: %d,"
				" fabric state: %d\n", se_cmd, se_cmd->t_state,
				se_cmd->se_tfo->get_cmd_state(se_cmd));

			rc = transport_wait_for_tasks(se_cmd);

			pr_debug("After transport_wait_for_tasks se_cmd: %p t_state: %d,"
				" fabric state: %d\n", se_cmd, se_cmd->t_state,
				se_cmd->se_tfo->get_cmd_state(se_cmd));
		}

		if (!rc) {
			wait_for_completion(&se_cmd->cmd_wait_comp);
			pr_debug("After cmd_wait_comp: se_cmd: %p t_state: %d"
				" fabric state: %d\n", se_cmd, se_cmd->t_state,
				se_cmd->se_tfo->get_cmd_state(se_cmd));
		}

		se_cmd->se_tfo->release_cmd(se_cmd);
	}
}
EXPORT_SYMBOL(target_wait_for_sess_cmds);

/*	transport_lun_wait_for_tasks():
 *
 *	Called from ConfigFS context to stop the passed struct se_cmd to allow
 *	an struct se_lun to be successfully shutdown.
 */
static int transport_lun_wait_for_tasks(struct se_cmd *cmd, struct se_lun *lun)
{
	unsigned long flags;
	int ret;
	/*
	 * If the frontend has already requested this struct se_cmd to
	 * be stopped, we can safely ignore this struct se_cmd.
	 */
	spin_lock_irqsave(&cmd->t_state_lock, flags);
	if (cmd->transport_state & CMD_T_STOP) {
		cmd->transport_state &= ~CMD_T_LUN_STOP;

		pr_debug("ConfigFS ITT[0x%08x] - CMD_T_STOP, skipping\n",
			 cmd->se_tfo->get_task_tag(cmd));
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		transport_cmd_check_stop(cmd, 1, 0);
		return -EPERM;
	}
	cmd->transport_state |= CMD_T_LUN_FE_STOP;
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	wake_up_interruptible(&cmd->se_dev->dev_queue_obj.thread_wq);

	ret = transport_stop_tasks_for_cmd(cmd);

	pr_debug("ConfigFS: cmd: %p t_tasks: %d stop tasks ret:"
			" %d\n", cmd, cmd->t_task_list_num, ret);
	if (!ret) {
		pr_debug("ConfigFS: ITT[0x%08x] - stopping cmd....\n",
				cmd->se_tfo->get_task_tag(cmd));
		wait_for_completion(&cmd->transport_lun_stop_comp);
		pr_debug("ConfigFS: ITT[0x%08x] - stopped cmd....\n",
				cmd->se_tfo->get_task_tag(cmd));
	}
	transport_remove_cmd_from_queue(cmd);

	return 0;
}

static void __transport_clear_lun_from_sessions(struct se_lun *lun)
{
	struct se_cmd *cmd = NULL;
	unsigned long lun_flags, cmd_flags;
	/*
	 * Do exception processing and return CHECK_CONDITION status to the
	 * Initiator Port.
	 */
	spin_lock_irqsave(&lun->lun_cmd_lock, lun_flags);

	while (!list_empty(&lun->lun_cmd_list)) {
		cmd = list_first_entry(&lun->lun_cmd_list,
		       struct se_cmd, se_lun_node);
		list_del_init(&cmd->se_lun_node);
		/*
		 * This will notify iscsi_target_transport.c:
		 * transport_cmd_check_stop() that a LUN shutdown is in
		 * progress for the iscsi_cmd_t.
		 */
		spin_lock(&cmd->t_state_lock);
		pr_debug("SE_LUN[%d] - Setting cmd->transport"
			"_lun_stop for  ITT: 0x%08x\n",
			cmd->se_lun->unpacked_lun,
			cmd->se_tfo->get_task_tag(cmd));
		cmd->transport_state |= CMD_T_LUN_STOP;
		spin_unlock(&cmd->t_state_lock);

		spin_unlock_irqrestore(&lun->lun_cmd_lock, lun_flags);

		if (!cmd->se_lun) {
			pr_err("ITT: 0x%08x, [i,t]_state: %u/%u\n",
				cmd->se_tfo->get_task_tag(cmd),
				cmd->se_tfo->get_cmd_state(cmd), cmd->t_state);
			BUG();
		}
		/*
		 * If the Storage engine still owns the iscsi_cmd_t, determine
		 * and/or stop its context.
		 */
		pr_debug("SE_LUN[%d] - ITT: 0x%08x before transport"
			"_lun_wait_for_tasks()\n", cmd->se_lun->unpacked_lun,
			cmd->se_tfo->get_task_tag(cmd));

		if (transport_lun_wait_for_tasks(cmd, cmd->se_lun) < 0) {
			spin_lock_irqsave(&lun->lun_cmd_lock, lun_flags);
			continue;
		}

		pr_debug("SE_LUN[%d] - ITT: 0x%08x after transport_lun"
			"_wait_for_tasks(): SUCCESS\n",
			cmd->se_lun->unpacked_lun,
			cmd->se_tfo->get_task_tag(cmd));

		spin_lock_irqsave(&cmd->t_state_lock, cmd_flags);
		if (!(cmd->transport_state & CMD_T_DEV_ACTIVE)) {
			spin_unlock_irqrestore(&cmd->t_state_lock, cmd_flags);
			goto check_cond;
		}

		cmd->transport_state &= ~CMD_T_DEV_ACTIVE;
		transport_all_task_dev_remove_state(cmd);
		spin_unlock_irqrestore(&cmd->t_state_lock, cmd_flags);

		transport_free_dev_tasks(cmd);
		/*
		 * The Storage engine stopped this struct se_cmd before it was
		 * send to the fabric frontend for delivery back to the
		 * Initiator Node.  Return this SCSI CDB back with an
		 * CHECK_CONDITION status.
		 */
check_cond:
		transport_send_check_condition_and_sense(cmd,
				TCM_NON_EXISTENT_LUN, 0);
		/*
		 *  If the fabric frontend is waiting for this iscsi_cmd_t to
		 * be released, notify the waiting thread now that LU has
		 * finished accessing it.
		 */
		spin_lock_irqsave(&cmd->t_state_lock, cmd_flags);
		if (cmd->transport_state & CMD_T_LUN_FE_STOP) {
			pr_debug("SE_LUN[%d] - Detected FE stop for"
				" struct se_cmd: %p ITT: 0x%08x\n",
				lun->unpacked_lun,
				cmd, cmd->se_tfo->get_task_tag(cmd));

			spin_unlock_irqrestore(&cmd->t_state_lock,
					cmd_flags);
			transport_cmd_check_stop(cmd, 1, 0);
			complete(&cmd->transport_lun_fe_stop_comp);
			spin_lock_irqsave(&lun->lun_cmd_lock, lun_flags);
			continue;
		}
		pr_debug("SE_LUN[%d] - ITT: 0x%08x finished processing\n",
			lun->unpacked_lun, cmd->se_tfo->get_task_tag(cmd));

		spin_unlock_irqrestore(&cmd->t_state_lock, cmd_flags);
		spin_lock_irqsave(&lun->lun_cmd_lock, lun_flags);
	}
	spin_unlock_irqrestore(&lun->lun_cmd_lock, lun_flags);
}

static int transport_clear_lun_thread(void *p)
{
	struct se_lun *lun = p;

	__transport_clear_lun_from_sessions(lun);
	complete(&lun->lun_shutdown_comp);

	return 0;
}

int transport_clear_lun_from_sessions(struct se_lun *lun)
{
	struct task_struct *kt;

	kt = kthread_run(transport_clear_lun_thread, lun,
			"tcm_cl_%u", lun->unpacked_lun);
	if (IS_ERR(kt)) {
		pr_err("Unable to start clear_lun thread\n");
		return PTR_ERR(kt);
	}
	wait_for_completion(&lun->lun_shutdown_comp);

	return 0;
}

/**
 * transport_wait_for_tasks - wait for completion to occur
 * @cmd:	command to wait
 *
 * Called from frontend fabric context to wait for storage engine
 * to pause and/or release frontend generated struct se_cmd.
 */
bool transport_wait_for_tasks(struct se_cmd *cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	if (!(cmd->se_cmd_flags & SCF_SE_LUN_CMD) &&
	    !(cmd->se_cmd_flags & SCF_SCSI_TMR_CDB)) {
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		return false;
	}
	/*
	 * Only perform a possible wait_for_tasks if SCF_SUPPORTED_SAM_OPCODE
	 * has been set in transport_set_supported_SAM_opcode().
	 */
	if (!(cmd->se_cmd_flags & SCF_SUPPORTED_SAM_OPCODE) &&
	    !(cmd->se_cmd_flags & SCF_SCSI_TMR_CDB)) {
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		return false;
	}
	/*
	 * If we are already stopped due to an external event (ie: LUN shutdown)
	 * sleep until the connection can have the passed struct se_cmd back.
	 * The cmd->transport_lun_stopped_sem will be upped by
	 * transport_clear_lun_from_sessions() once the ConfigFS context caller
	 * has completed its operation on the struct se_cmd.
	 */
	if (cmd->transport_state & CMD_T_LUN_STOP) {
		pr_debug("wait_for_tasks: Stopping"
			" wait_for_completion(&cmd->t_tasktransport_lun_fe"
			"_stop_comp); for ITT: 0x%08x\n",
			cmd->se_tfo->get_task_tag(cmd));
		/*
		 * There is a special case for WRITES where a FE exception +
		 * LUN shutdown means ConfigFS context is still sleeping on
		 * transport_lun_stop_comp in transport_lun_wait_for_tasks().
		 * We go ahead and up transport_lun_stop_comp just to be sure
		 * here.
		 */
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);

		complete(&cmd->transport_lun_stop_comp);
		wait_for_completion(&cmd->transport_lun_fe_stop_comp);

		spin_lock_irqsave(&cmd->t_state_lock, flags);

		transport_all_task_dev_remove_state(cmd);
		/*
		 * At this point, the frontend who was the originator of this
		 * struct se_cmd, now owns the structure and can be released through
		 * normal means below.
		 */
		pr_debug("wait_for_tasks: Stopped"
			" wait_for_completion(&cmd->t_tasktransport_lun_fe_"
			"stop_comp); for ITT: 0x%08x\n",
			cmd->se_tfo->get_task_tag(cmd));

		cmd->transport_state &= ~CMD_T_LUN_STOP;
	}

	if (!(cmd->transport_state & CMD_T_ACTIVE)) {
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		return false;
	}

	cmd->transport_state |= CMD_T_STOP;

	pr_debug("wait_for_tasks: Stopping %p ITT: 0x%08x"
		" i_state: %d, t_state: %d, CMD_T_STOP\n",
		cmd, cmd->se_tfo->get_task_tag(cmd),
		cmd->se_tfo->get_cmd_state(cmd), cmd->t_state);

	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	wake_up_interruptible(&cmd->se_dev->dev_queue_obj.thread_wq);
	wait_for_completion(&cmd->t_transport_stop_comp);

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	cmd->transport_state &= ~(CMD_T_ACTIVE | CMD_T_STOP);

	pr_debug("wait_for_tasks: Stopped wait_for_compltion("
		"&cmd->t_transport_stop_comp) for ITT: 0x%08x\n",
		cmd->se_tfo->get_task_tag(cmd));

	spin_unlock_irqrestore(&cmd->t_state_lock, flags);
	return true;
}
EXPORT_SYMBOL(transport_wait_for_tasks);

static int transport_get_sense_codes(
	struct se_cmd *cmd,
	u8 *asc,
	u8 *ascq)
{
	*asc = cmd->scsi_asc;
	*ascq = cmd->scsi_ascq;

	return 0;
}

static int transport_set_sense_codes(
	struct se_cmd *cmd,
	u8 asc,
	u8 ascq)
{
	cmd->scsi_asc = asc;
	cmd->scsi_ascq = ascq;

	return 0;
}

int transport_send_check_condition_and_sense(
	struct se_cmd *cmd,
	u8 reason,
	int from_transport)
{
	unsigned char *buffer = cmd->sense_buffer;
	unsigned long flags;
	int offset;
	u8 asc = 0, ascq = 0;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	if (cmd->se_cmd_flags & SCF_SENT_CHECK_CONDITION) {
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		return 0;
	}
	cmd->se_cmd_flags |= SCF_SENT_CHECK_CONDITION;
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	if (!reason && from_transport)
		goto after_reason;

	if (!from_transport)
		cmd->se_cmd_flags |= SCF_EMULATED_TASK_SENSE;
	/*
	 * Data Segment and SenseLength of the fabric response PDU.
	 *
	 * TRANSPORT_SENSE_BUFFER is now set to SCSI_SENSE_BUFFERSIZE
	 * from include/scsi/scsi_cmnd.h
	 */
	offset = cmd->se_tfo->set_fabric_sense_len(cmd,
				TRANSPORT_SENSE_BUFFER);
	/*
	 * Actual SENSE DATA, see SPC-3 7.23.2  SPC_SENSE_KEY_OFFSET uses
	 * SENSE KEY values from include/scsi/scsi.h
	 */
	switch (reason) {
	case TCM_NON_EXISTENT_LUN:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ILLEGAL REQUEST */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		/* LOGICAL UNIT NOT SUPPORTED */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x25;
		break;
	case TCM_UNSUPPORTED_SCSI_OPCODE:
	case TCM_SECTOR_COUNT_TOO_MANY:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ILLEGAL REQUEST */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		/* INVALID COMMAND OPERATION CODE */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x20;
		break;
	case TCM_UNKNOWN_MODE_PAGE:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ILLEGAL REQUEST */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		/* INVALID FIELD IN CDB */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x24;
		break;
	case TCM_CHECK_CONDITION_ABORT_CMD:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ABORTED COMMAND */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		/* BUS DEVICE RESET FUNCTION OCCURRED */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x29;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x03;
		break;
	case TCM_INCORRECT_AMOUNT_OF_DATA:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ABORTED COMMAND */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		/* WRITE ERROR */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0c;
		/* NOT ENOUGH UNSOLICITED DATA */
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x0d;
		break;
	case TCM_INVALID_CDB_FIELD:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ILLEGAL REQUEST */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		/* INVALID FIELD IN CDB */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x24;
		break;
	case TCM_INVALID_PARAMETER_LIST:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ILLEGAL REQUEST */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		/* INVALID FIELD IN PARAMETER LIST */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x26;
		break;
	case TCM_UNEXPECTED_UNSOLICITED_DATA:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ABORTED COMMAND */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		/* WRITE ERROR */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0c;
		/* UNEXPECTED_UNSOLICITED_DATA */
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x0c;
		break;
	case TCM_SERVICE_CRC_ERROR:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ABORTED COMMAND */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		/* PROTOCOL SERVICE CRC ERROR */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x47;
		/* N/A */
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x05;
		break;
	case TCM_SNACK_REJECTED:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ABORTED COMMAND */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		/* READ ERROR */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x11;
		/* FAILED RETRANSMISSION REQUEST */
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x13;
		break;
	case TCM_WRITE_PROTECTED:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* DATA PROTECT */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = DATA_PROTECT;
		/* WRITE PROTECTED */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x27;
		break;

	case TCM_ADDRESS_OUT_OF_RANGE:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ILLEGAL REQUEST */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		/* LOGICAL BLOCK ADDRESS OUT OF RANGE */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x21;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x00;
		break;

	case TCM_CHECK_CONDITION_UNIT_ATTENTION:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* UNIT ATTENTION */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = UNIT_ATTENTION;
		core_scsi3_ua_for_check_condition(cmd, &asc, &ascq);
		buffer[offset+SPC_ASC_KEY_OFFSET] = asc;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = ascq;
		break;
	case TCM_CHECK_CONDITION_NOT_READY:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* Not Ready */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = NOT_READY;
		transport_get_sense_codes(cmd, &asc, &ascq);
		buffer[offset+SPC_ASC_KEY_OFFSET] = asc;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = ascq;
		break;

#if defined(CONFIG_MACH_QNAPTS) //Benjamin 20121105 sync VAAI support from Adam. 
	case TCM_MISCOMPARE_DURING_VERIFY_OP:
		// bit 7 should be set to 1
		buffer[offset] = (0x70 + 0x80);

		/* byte 3 ~ byte 6 should be put the offset where data buffer
		 * doesn't match we compared before.
		 */
		put_unaligned_be32(cmd->byte_err_offset, &buffer[offset+3]);
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = MISCOMPARE;

		// MISCOMPARE DURING VERIFY OPERATION
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x1D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x00;
		break;

	case TCM_PARAMETER_LIST_LEN_ERROR:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;

		// PARAMETER LIST LEN ERROR
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x1A;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x00;
		break;

	case TCM_UNREACHABLE_COPY_TARGET:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = COPY_ABORTED;

		// UNREACHABLE COPY TARGET
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x08;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x04;
		break;

	case TCM_3RD_PARTY_DEVICE_FAILURE:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = COPY_ABORTED;

		// 3RD PARTY DEVICE FAILURE
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x01;
		break;

	case TCM_INCORRECT_COPY_TARGET_DEV_TYPE:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = COPY_ABORTED;

		// 3RD PARTY DEVICE FAILURE
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x03;
		break;

	case TCM_TOO_MANY_TARGET_DESCRIPTORS:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;

		// TOO MANY TARGET DESCRIPTORS:
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x26;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x06;
		break;

	case TCM_TOO_MANY_SEGMENT_DESCRIPTORS:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;

		// TOO MANY SEGMENT DESCRIPTORS:
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x26;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x08;
		break;

	case TCM_ILLEGAL_REQ_DATA_OVERRUN_COPY_TARGET:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x05;
		break;

	case TCM_ILLEGAL_REQ_DATA_UNDERRUN_COPY_TARGET:
		buffer[offset] = (0x70 | 0x80);

		/* Information field shall be zero */
		buffer[offset+3] = 0;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x04;
		break;

	case TCM_COPY_ABORT_DATA_OVERRUN_COPY_TARGET:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = COPY_ABORTED;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x05;
		break;

	case TCM_COPY_ABORT_DATA_UNDERRUN_COPY_TARGET:
		buffer[offset] = (0x70 | 0x80);

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_TPC_CMD)
		if (cmd->is_tpc && cmd->t_track_rec){
			/* FIXED ME
			 *
			 * The transfer count is 8 bytes for specification but
			 * the information field only have 4 bytes width ... 
			 */
			spin_lock(&((TPC_TRACK_DATA *)cmd->t_track_rec)->t_count_lock);
			put_unaligned_be32(
				(u32)(((TPC_TRACK_DATA *)cmd->t_track_rec)->t_transfer_count),
				&buffer[offset+3]
				);
			spin_unlock(&((TPC_TRACK_DATA *)cmd->t_track_rec)->t_count_lock);
		}
#endif
#endif
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = COPY_ABORTED;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0D;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x04;
        break;

	case TCM_INSUFFICIENT_RESOURCES:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x55;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x03;
		break;

	case TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x55;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x0C;
		break;

	case TCM_INSUFFICIENT_RESOURCES_TO_CREATE_ROD_TOKEN:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x55;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x0D;
		break;

	case TCM_OPERATION_IN_PROGRESS:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x00;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x16;
		break;

	case TCM_INVALID_TOKEN_OP_AND_INVALID_TOKEN_LEN:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x0A;
		break;

	case TCM_INVALID_TOKEN_OP_AND_CAUSE_NOT_REPORTABLE:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x00;
		break;

	case TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_CREATION_NOT_SUPPORTED:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x03;
		break;

	case TCM_INVALID_TOKEN_OP_AND_REMOTE_ROD_TOKEN_USAGE_NOT_SUPPORTED:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x02;
		break;

	case TCM_INVALID_TOKEN_OP_AND_TOKEN_CANCELLED:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x08;
		break;

	case TCM_INVALID_TOKEN_OP_AND_TOKEN_CORRUPT:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x05;
		break;

	case TCM_INVALID_TOKEN_OP_AND_TOKEN_DELETED:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x09;
		break;

	case TCM_INVALID_TOKEN_OP_AND_TOKEN_EXPIRED:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x07;
		break;

	case TCM_INVALID_TOKEN_OP_AND_TOKEN_REVOKED:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x06;
		break;

	case TCM_INVALID_TOKEN_OP_AND_TOKEN_UNKNOWN:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x04;
		break;

	case TCM_INVALID_TOKEN_OP_AND_UNSUPPORTED_TOKEN_TYPE:
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x23;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x01;
		break;

#if defined(SUPPORT_TP)
	case TCM_SPACE_ALLOCATION_FAILED_WRITE_PROTECT:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* DATA_PROTECT */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = DATA_PROTECT;
		/* SPACE ALLOCATION FAILED WRITE PROTECT */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x27;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x07;		
		break;
	case TCM_THIN_PROVISIONING_SOFT_THRESHOLD_REACHED:
		/* CURRENT ERROR */
		buffer[offset] = 0x70 | 0x80; // Valid bit set to 1

		put_unaligned_be32(
			(u32)(0x80 | (THRESHOLD_TYPE_SOFTWARE << 3) | THRESHOLD_ARM_INC),
			&buffer[offset+SPC_INFORMATION_OFFSET]
		);

		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* UNIT_ATTENTION */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = UNIT_ATTENTION ;
		/* THIN PROVISIONING SOFT THRESHOLD REACHED */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x38;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x07;		
		break;
	case TCM_CAPACITY_DATA_HAS_CHANGED:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* UNIT_ATTENTION */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = UNIT_ATTENTION;
		/* CAPACITY DATA HAS CHANGED */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x2a;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x09;		
		break;
#endif /* defined(SUPPORT_TP) */
#endif  /* #if defined(CONFIG_MACH_QNAPTS) */
	case TCM_LOGICAL_UNIT_COMMUNICATION_FAILURE:
	default:
		/* CURRENT ERROR */
		buffer[offset] = 0x70;
		buffer[offset+SPC_ADD_SENSE_LEN_OFFSET] = 10;
		/* ILLEGAL REQUEST */
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		/* LOGICAL UNIT COMMUNICATION FAILURE */
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x08;
		break;
	}
	/*
	 * This code uses linux/include/scsi/scsi.h SAM status codes!
	 */
	cmd->scsi_status = SAM_STAT_CHECK_CONDITION;
	/*
	 * Automatically padded, this value is encoded in the fabric's
	 * data_length response PDU containing the SCSI defined sense data.
	 */
	cmd->scsi_sense_length  = TRANSPORT_SENSE_BUFFER + offset;

after_reason:
	return cmd->se_tfo->queue_status(cmd);
}
EXPORT_SYMBOL(transport_send_check_condition_and_sense);

int transport_check_aborted_status(struct se_cmd *cmd, int send_status)
{
	int ret = 0;

	if (cmd->transport_state & CMD_T_ABORTED)
	{
		if (!send_status ||
		     (cmd->se_cmd_flags & SCF_SENT_DELAYED_TAS))
			return 1;
#if 0
		pr_err("Sending delayed SAM_STAT_TASK_ABORTED"
			" status for CDB: 0x%02x ITT: 0x%08x\n",
			cmd->t_task_cdb[0],
			cmd->se_tfo->get_task_tag(cmd));
#endif
		cmd->se_cmd_flags |= SCF_SENT_DELAYED_TAS;
		cmd->se_tfo->queue_status(cmd);

		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL(transport_check_aborted_status);


void transport_send_task_abort(struct se_cmd *cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&cmd->t_state_lock, flags);
	if (cmd->se_cmd_flags & SCF_SENT_CHECK_CONDITION) {
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		return;
	}
	spin_unlock_irqrestore(&cmd->t_state_lock, flags);

	/*
	 * If there are still expected incoming fabric WRITEs, we wait
	 * until until they have completed before sending a TASK_ABORTED
	 * response.  This response with TASK_ABORTED status will be
	 * queued back to fabric module by transport_check_aborted_status().
	 */
	if (cmd->data_direction == DMA_TO_DEVICE) {
		if (cmd->se_tfo->write_pending_status(cmd) != 0) {
			cmd->transport_state |= CMD_T_ABORTED;
			smp_mb__after_atomic_inc();
		}
	}

	cmd->scsi_status = SAM_STAT_TASK_ABORTED;
#if 0
	pr_err("Setting SAM_STAT_TASK_ABORTED status for CDB: 0x%02x,"
		" ITT: 0x%08x\n", cmd->t_task_cdb[0],
		cmd->se_tfo->get_task_tag(cmd));
#endif
	cmd->se_tfo->queue_status(cmd);


}




static int transport_generic_do_tmr(struct se_cmd *cmd)
{
	struct se_device *dev = cmd->se_dev;
	struct se_tmr_req *tmr = cmd->se_tmr_req;
	int ret;

#if defined(CONFIG_MACH_QNAPTS) 
	/* adamhsu, 
	 * (1) redmine bug 6915 - bugzilla 40743
	 * (2) redmine bug 6916 - bugzilla 41183
	 */
	__create_cmd_rec(cmd);
#endif

	switch (tmr->function) {
	case TMR_ABORT_TASK:
		core_tmr_abort_task(dev, tmr, cmd->se_sess);
		break;
	case TMR_ABORT_TASK_SET:
	case TMR_CLEAR_ACA:
	case TMR_CLEAR_TASK_SET:
		tmr->response = TMR_TASK_MGMT_FUNCTION_NOT_SUPPORTED;
		break;
	case TMR_LUN_RESET:
		ret = core_tmr_lun_reset(dev, tmr, NULL, NULL);
		tmr->response = (!ret) ? TMR_FUNCTION_COMPLETE :
					 TMR_FUNCTION_REJECTED;
		break;
	case TMR_TARGET_WARM_RESET:
		tmr->response = TMR_FUNCTION_REJECTED;
		break;
	case TMR_TARGET_COLD_RESET:
		tmr->response = TMR_FUNCTION_REJECTED;
		break;
	default:
		pr_err("Uknown TMR function: 0x%02x.\n",
				tmr->function);
		tmr->response = TMR_FUNCTION_REJECTED;
		break;
	}

	cmd->t_state = TRANSPORT_ISTATE_PROCESSING;
	cmd->se_tfo->queue_tm_rsp(cmd);
	transport_cmd_check_stop_to_fabric(cmd);
	return 0;
}

#if defined(SUPPORT_PARALLEL_TASK_WQ)
void __p_task_dump_info(
    LIO_SE_TASK *task
    )
{
    /**/
#if 0
    printk("[%s], task(0x%p)\n", __func__, task);

    if (task->t_go_do_task == 1){
        printk("cdb[0]:0x%x, lba:0x%llx, range:0x%x\n",
            task->task_se_cmd->t_task_cdb[0], task->task_lba, task->task_sectors);
    }else {
        printk("task(0x%p), cdb[0]:0x%x, cdb[1]:0x%x\n", task, 
            task->task_se_cmd->t_task_cdb[0], task->task_se_cmd->t_task_cdb[1]);
    }
#endif
    return;
}


static int __p_task_get_gen_cmd_dir(
    LIO_SE_TASK *task
    )
{
    int ret = 1;    /* dma to device */

    /* FIXED ME !! */

    switch (task->task_se_cmd->t_task_cdb[0]){
    case WRITE_6:
    case WRITE_10:
    case WRITE_16:
        break;

    case READ_6:
    case READ_10:
    case READ_16:
        ret = 0;    /* dma from device */
        break;

    default:
        printk("error !! unsupported cdb[0] !! please check this\n");
        break;
    }

    return ret;

}


static int __p_task_make_t_rec(
    LIO_SE_TASK *n_task,
    LIO_SE_TASK *o_task
    )
{
    TASK_REC *t_rec = NULL;

    /* FIXED ME 
     *
     * This is too stupid !! Need to find out other best method to make
     * relationship between them !!
     */
    t_rec = kmalloc(sizeof(TASK_REC), GFP_ATOMIC);
    if (!t_rec){
        printk("fail to alloc t_rec\n");
        return 1;
    }

   
    INIT_LIST_HEAD(&t_rec->rec_node);
    t_rec->se_task = o_task;
    list_add_tail(&t_rec->rec_node, &n_task->t_rec_list);
    return 0;
}


static int __p_task_check_task_conflicted(
    LIO_SE_TASK *n_task,
    LIO_SE_TASK *c_task
    )
{
    LIO_SE_TASK *n = n_task, *c = c_task;
    int n_dir = 0, c_dir = 0, ret = 1;
    sector_t n_end_lba = 0, c_end_lba = 0;

    n_end_lba = (n->task_lba + n->task_sectors - 1);
    c_end_lba = (c->task_lba + c->task_sectors - 1);

    /* we have two cases ... */
    if (((n->task_lba >= c->task_lba) && (n->task_lba <= c_end_lba))
    ||  ((n_end_lba >= c->task_lba) && (n_end_lba <= c_end_lba))
    )
    {  
        n_dir = __p_task_get_gen_cmd_dir(n_task);
        c_dir = __p_task_get_gen_cmd_dir(c_task);
    
        if (((n_dir == 1) && (c_dir == 1)) || (n_dir != c_dir))
            ret = 0;
#if 0
        else
            /* (n_dir,c_dir) = (0,0) and do nothing now ... */
#endif
    }


    if (!ret ){
        printk("[new coming task:0x%p, lba:0x%llx, range:0x%x] conflicts with "
                "[checked task:0x%p, lba:0x%llx, range:0x%x], "
                "n_dir:0x%x, c_dir:0x%x\n", 
                n_task, (unsigned long long)n_task->task_lba, n_task->task_sectors,
                c_task, (unsigned long long)c_task->task_lba, c_task->task_sectors,
                n_dir, c_dir, ret
                );
    }
    return ret;
}


static int __p_task_is_gen_rw_cmd(
    LIO_SE_TASK *n_task
    )
{
    int ret = 0;

    /* FIXED ME !! */

    switch (n_task->task_se_cmd->t_task_cdb[0]){
    case WRITE_6:
    case WRITE_10:
    case WRITE_12:
    case WRITE_16:
    case READ_6:
    case READ_10:
    case READ_12:
    case READ_16:
        n_task->t_go_do_task = 1;
        break;
    default:
        n_task->t_go_do_task = 0;
        ret = 1;
        break;
    }

    return ret;
}


static void p_task_work_func(
    struct work_struct *work
    )
{
    LIO_SE_CMD *cmd = NULL;
    LIO_SE_DEVICE *se_dev = NULL;
    LIO_SE_TASK *task = container_of(work, LIO_SE_TASK, t_work);
    int error;
    unsigned long flags;

#if 1
//    printk("curr: %s, pid: %d\n", current->comm, current->pid);

#else
    /**/
    printk("%s: task:0x%p, current: %s, pid: %d\n", __func__,
                task, current->comm, current->pid);
    __p_task_dump_info(task);
#endif

    BUG_ON((!task->task_se_cmd));
    cmd = task->task_se_cmd;

    BUG_ON((!cmd->se_dev));
    se_dev = cmd->se_dev;

    /**/
    if (cmd->execute_task)
        error = cmd->execute_task(task);
    else
        error = se_dev->transport->do_task(task);

	if (error != 0) {
        p_task_remove_relationship_with_other_task(task, se_dev);
        wake_up_interruptible(&se_dev->p_task_thread_wq);
		spin_lock_irqsave(&cmd->t_state_lock, flags);
		task->task_flags &= ~TF_ACTIVE;
		cmd->transport_state &= ~CMD_T_SENT;
		spin_unlock_irqrestore(&cmd->t_state_lock, flags);
		transport_stop_tasks_for_cmd(cmd);
		transport_generic_request_failure(cmd);
	}

    return;
}


int p_task_remove_relationship_with_other_task(
    struct se_task *r_task, 
    struct se_device *se_dev
    )
{
    TASK_REC *rec = NULL, *tmp_rec = NULL;
    struct se_task *o_task = NULL;
    unsigned long flags;

    /* remove from running-task queue */
//    printk("remove task(0x%p) from r list\n", r_task);

    spin_lock(&se_dev->dev_run_task_lock);
    list_del_init(&r_task->t_node);
    spin_unlock(&se_dev->dev_run_task_lock);

    /**/
    if (r_task->t_go_do_task == 0)
        return 0;

    /* remove from queued-task queue */
//    printk("check remove task from q list\n");

    spin_lock(&se_dev->dev_q_task_lock);
    list_for_each_entry(o_task, &se_dev->dev_q_task_list, t_node){

        spin_lock(&o_task->t_rec_lock);
        list_for_each_entry_safe(rec, tmp_rec, &o_task->t_list, rec_node){
            if (rec->se_task == r_task){
                /* found it ! remove it and free resource */
                printk("found task(0x%p) relates to q task(0x%p), "
                            "to remove it \n", r_task, o_task);
                list_del_init(&rec->rec_node);
                kfree(rec);
            }
        }
        spin_unlock(&o_task->t_rec_lock);
    }
    spin_unlock(&se_dev->dev_q_task_lock);
    return 0;
}
EXPORT_SYMBOL(p_task_remove_relationship_with_other_task);

static int p_task_make_relationship_with_other_task(
    LIO_SE_TASK *task, 
    LIO_SE_DEVICE *se_dev
    )
{
    LIO_SE_TASK *n = task, *o = NULL, *t = NULL;
    int ret = 1;
    unsigned long flags;

    /* FIXED ME 
     *
     * we do this ONLY for general read/write command currently ...
     */
    if (__p_task_is_gen_rw_cmd(task) == 1)
        return 1;        


    /* To check whether new task conflicts with any task in running task list */
//    printk("check relationship with r task list\n");

    spin_lock(&se_dev->dev_run_task_lock);
    list_for_each_entry_safe(o, t, &se_dev->dev_run_task_list, t_node){
        if (__p_task_check_task_conflicted(n, o) == 0)
            __p_task_make_t_rec(n, o);
    }
    spin_unlock(&se_dev->dev_run_task_lock);

    /* To check new task conflicts with queued task list */
//    printk("check relationship with q task list\n");

    spin_lock(&se_dev->dev_q_task_lock);
    list_for_each_entry_safe(o, t, &se_dev->dev_q_task_list, t_node){
        if (__p_task_check_task_conflicted(n, o) == 0)
            __p_task_make_t_rec(n, o);
    }
    spin_unlock(&se_dev->dev_q_task_lock);

    /* return 0 if need to put new task to q list */    
    spin_lock(&n->t_rec_lock);
    ret = list_empty(&n->t_rec_list);
    spin_unlock(&n->t_rec_lock);

    return ret;
}

static int p_task_thread(
    void *param
    )
{
    LIO_SE_DEVICE *se_dev = param;
    LIO_SE_TASK *n_task = NULL, *o_task = NULL, *t_task = NULL;
    int ret, to_sbmit_q_task;
    unsigned long flags;


    while (!kthread_should_stop()) {
        ret = wait_event_interruptible(se_dev->p_task_thread_wq,
                    atomic_read(&se_dev->dev_r_task_cnt) ||
                    kthread_should_stop());

//        printk("%s, ret:0x%x\n",__func__, ret);
        if (ret < 0)
            goto _OUT_;

        /* step 1: 
         * 
         * check any queued task ? if yes, to process any queued task now ...
         */
        spin_lock(&se_dev->dev_q_task_lock);
        list_for_each_entry_safe(o_task, t_task, &se_dev->dev_q_task_list, t_node){

            /* check any task related to this q task ?  */
            to_sbmit_q_task = 0;
            spin_lock(&o_task->t_rec_lock);
            if (!list_empty(&o_task->t_rec_list)){

                /* to remove this task from q list */
                printk("found q task, to submit it\n");
                __p_task_dump_info(o_task);

                to_sbmit_q_task = 1;
                list_del_init(&o_task->t_node);
            }
            spin_unlock(&o_task->t_rec_lock);
            spin_unlock(&se_dev->dev_q_task_lock);

            /**/
            if (to_sbmit_q_task == 1){
                /* to insert q task into r list again */
                spin_lock(&se_dev->dev_run_task_lock);
                list_add_tail(&o_task->t_node, &se_dev->dev_run_task_list);
                spin_unlock(&se_dev->dev_run_task_lock);
                queue_work(se_dev->p_task_work_queue, &o_task->t_work);
            }

            /* go to chect next dp cmd */
            spin_lock(&se_dev->dev_q_task_lock);
        }
        spin_unlock(&se_dev->dev_q_task_lock);

        /* step 2: 
         *
         * get new task from device first 
         */
        n_task = NULL;

        spin_lock(&se_dev->dev_r_task_lock);
        if (!list_empty(&se_dev->dev_r_task_list)){
            n_task = list_first_entry(&se_dev->dev_r_task_list, LIO_SE_TASK, t_node);
            list_del_init(&n_task->t_node);
            atomic_dec(&se_dev->dev_r_task_cnt);
//            printk("get new task !!\n");
            /* here won't print information cause of we don't still identify new task yet */
        }
        spin_unlock(&se_dev->dev_r_task_lock);

        if (!n_task)
            continue;

        /* step 3:
         *
         * to check new task conflicts with any task in running task queue or 
         * queued task queue
         *
         * NOTE (FIXED ME !!):
         * (1) For general read / write command (cdb contains the start LBA and range)
         * (2) Other special command (cdb doesn't contain enough information, we
         *     need to get them from parameter data buffer
         * (3) To check it is enough or not
         */
        if(p_task_make_relationship_with_other_task(n_task, se_dev) == 0){
            /* new task conflicts with other task in running task list or
             * queued task list
             */
            spin_lock(&se_dev->dev_q_task_lock);
            list_add_tail(&n_task->t_node, &se_dev->dev_q_task_list);
            spin_unlock(&se_dev->dev_q_task_lock);
            continue;
        }

        /* ok.. let's fire it */

//        printk("not conflicts with others, submit it now !!\n");
        __p_task_dump_info(n_task);

        spin_lock(&se_dev->dev_run_task_lock);
        list_add_tail(&n_task->t_node, &se_dev->dev_run_task_list);
        spin_unlock(&se_dev->dev_run_task_lock);
        queue_work(se_dev->p_task_work_queue, &n_task->t_work);
	}

_OUT_:
    se_dev->p_task_thread = NULL;
    return 0;
}
#endif

/*	transport_processing_thread():
 *
 *
 */
static int transport_processing_thread(void *param)
{
	int ret;
	struct se_cmd *cmd;
	struct se_device *dev = param;


#if defined(CONFIG_MACH_QNAPTS)
	set_user_nice(current, -20);
#endif

	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(dev->dev_queue_obj.thread_wq,
				atomic_read(&dev->dev_queue_obj.queue_cnt) ||
				kthread_should_stop());
		if (ret < 0)
			goto out;

get_cmd:
		cmd = transport_get_cmd_from_queue(&dev->dev_queue_obj);
		if (!cmd)
			continue;

		switch (cmd->t_state) {
		case TRANSPORT_NEW_CMD:
			BUG();
			break;
		case TRANSPORT_NEW_CMD_MAP:
			if (!cmd->se_tfo->new_cmd_map) {
				pr_err("cmd->se_tfo->new_cmd_map is"
					" NULL for TRANSPORT_NEW_CMD_MAP\n");
				BUG();
			}
			ret = cmd->se_tfo->new_cmd_map(cmd);
			if (ret < 0) {
				transport_generic_request_failure(cmd);
				break;
			}
			ret = transport_generic_new_cmd(cmd);
			if (ret < 0) {
				transport_generic_request_failure(cmd);
				break;
			}
			break;
		case TRANSPORT_PROCESS_WRITE:
			transport_generic_process_write(cmd);
			break;
		case TRANSPORT_PROCESS_TMR:
			transport_generic_do_tmr(cmd);
			break;
		case TRANSPORT_COMPLETE_QF_WP:
			transport_write_pending_qf(cmd);
			break;
		case TRANSPORT_COMPLETE_QF_OK:
			transport_complete_qf(cmd);
			break;
		default:
			pr_err("Unknown t_state: %d  for ITT: 0x%08x "
				"i_state: %d on SE LUN: %u\n",
				cmd->t_state,
				cmd->se_tfo->get_task_tag(cmd),
				cmd->se_tfo->get_cmd_state(cmd),
				cmd->se_lun->unpacked_lun);
			BUG();
		}

		goto get_cmd;
	}

out:
	WARN_ON(!list_empty(&dev->state_task_list));
	WARN_ON(!list_empty(&dev->dev_queue_obj.qobj_list));
	dev->process_thread = NULL;
	return 0;
}

#if defined(CONFIG_MACH_QNAPTS)
//Benjamin 20121105 sync VAAI support from Adam. 

/*
 * @fn u32 __call_transport_get_size(IN u32 sectors, IN u8 *cdb, IN LIO_SE_CMD *se_cmd)
 * @brief Get total size from (a) number of blocks and (b) one block size
 *
 * @note This function is only a wrapper of transport_get_size().
 * @sa transport_get_size
 * @param[in] sectors   number of blocks
 * @param[in] cdb    point to scsi cdb data
 * @param[in] se_cmd
 * @retval u32 value for total size
 */
u32 __call_transport_get_size(IN u32 sectors, IN u8 *cdb, IN LIO_SE_CMD *se_cmd)
{
	return transport_get_size(sectors, cdb, se_cmd);
}

/* adamhsu 2013/06/07 - Support to set the logical block size from NAS GUI. */

#if defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
/*
 * @fn u32 __blkio_transfer_task_lba_to_block_lba(IN u32 logical_block_size, IN OUT sector_t *out_lba)
 * 
 * @brief transfer the task lba to block lba for block i/o device
 *
 * @param[in] logical_block_size
 * @param[in,out] task_lba for IN parameter / output block lba for OUT parameter
 *
 * @retval    0: successful / others: fail
 */
int __blkio_transfer_task_lba_to_block_lba(
    IN u32 logical_block_size,
    IN OUT sector_t *out_lba
    )
{
    sector_t curr_lba = 0;

    if (!logical_block_size || !out_lba)
        return -EOPNOTSUPP;

    /* Here refer the concept of iblock_do_task()
     *
     * Do starting conversion up from non 512-byte blocksize with
     * struct se_task SCSI blocksize into Linux/Block 512 units for BIO.
     */
    curr_lba = *out_lba;

    if (logical_block_size == 4096)
        *out_lba = (sector_t)(curr_lba << 3);
    else if (logical_block_size == 2048)
        *out_lba = (sector_t)(curr_lba << 2);
    else if (logical_block_size == 1024)
        *out_lba = (sector_t)(curr_lba << 1);
    else if (logical_block_size == 512)
        *out_lba = (sector_t)curr_lba;
    else {
        pr_err("%s: Unsupported SCSI -> BLOCK LBA conversion: %u\n", __func__,
            logical_block_size);
        return -ENOSYS;
    }
    return 0;
}
EXPORT_SYMBOL(__blkio_transfer_task_lba_to_block_lba);

#endif

u32 __call_transport_lba_21(IN u8 *cdb){
	return transport_lba_21(cdb);
}

u32 __call_transport_lba_32(IN u8 *cdb){
	return transport_lba_32(cdb);
}

unsigned long long __call_transport_lba_64(IN u8 *cdb){
	return transport_lba_64(cdb);
}

/* For VARIABLE_LENGTH_CDB w/ 32 byte extended CDBs */
unsigned long long __call_transport_lba_64_ext(IN u8 *cdb){
	return transport_lba_64_ext(cdb);
}

u32 __call_transport_get_sectors_6(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	)
{
	return transport_get_sectors_6(cdb, se_cmd, ret);
}

u32 __call_transport_get_sectors_10(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	)
{
	return transport_get_sectors_10(cdb, se_cmd, ret);
}

u32 __call_transport_get_sectors_12(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	)
{
	return transport_get_sectors_12(cdb, se_cmd, ret);
}

u32 __call_transport_get_sectors_16(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	)
{
	return transport_get_sectors_16(cdb, se_cmd, ret);
}


/* Used for VARIABLE_LENGTH_CDB WRITE_32 and READ_32 variants */
u32 __call_transport_get_sectors_32(
	IN u8 *cdb, 
	IN LIO_SE_CMD *se_cmd, 
	IN OUT int *ret
	)
{
	return transport_get_sectors_32(cdb, se_cmd, ret);
}

int is_thin_lun(IN LIO_SE_DEVICE *dev)
{
	struct se_subsystem_dev *se_sub_dev = dev->se_sub_dev;

	if (!strnicmp(se_sub_dev->se_dev_provision, "thin", 
		sizeof(se_sub_dev->se_dev_provision)))
	{
//		pr_info("found thin lun\n");
		return 1;
	}

	return 0;
}
EXPORT_SYMBOL(is_thin_lun);

/*
 * @fn int __get_lba_and_nr_blks_ws_cdb(IN LIO_SE_CMD *pSeCmd, IN OUT sector_t *pLba, IN OUT u32 *pu32NumBlocks)
 * @brief Function to get the lba and block ranges from write same(10,16,32) command
 *
 * @param[in]     pSeCmd
 * @param[in,out] pLba
 * @param[in,out] pu32NumBlocks
 * @retval 0 - success / 1 - fail
 */
int __get_lba_and_nr_blks_ws_cdb(
	IN LIO_SE_CMD *pSeCmd,
	IN OUT sector_t *pLba,
	IN OUT u32 *pu32NumBlocks
	)
{
	u8 *pu8Cdb = NULL;

	/**/
	pu8Cdb = CMD_TO_TASK_CDB(pSeCmd)->t_task_cdb;

	if (pu8Cdb[0] == WRITE_SAME){
		*pLba = (sector_t)get_unaligned_be32(&pu8Cdb[2]);
	        *pu32NumBlocks = (u32)get_unaligned_be16(&pu8Cdb[7]);

	}else if (pu8Cdb[0] == WRITE_SAME_16){
		*pLba = (sector_t)get_unaligned_be64(&pu8Cdb[2]);
		*pu32NumBlocks = (u32)get_unaligned_be32(&pu8Cdb[10]);

	}else if ((pu8Cdb[0] == VARIABLE_LENGTH_CMD) 
		&& (get_unaligned_be16(&pu8Cdb[8]) == WRITE_SAME_32)){
		/*
		 * Here only handle WRITE SAME(32) command even if passed
		 * cdb is variable length command format
		 */
		*pLba = (sector_t)get_unaligned_be64(&pu8Cdb[12]);
		*pu32NumBlocks = (u32)get_unaligned_be32(&pu8Cdb[28]);

	}else {
		/* return error for other case ... */
		return 1;
	}
	return 0;

}

/*
 * @fn int __isntall_tpc_proc (IN u8 *pu8Cdb, IN void **ppHook)
 * @brief Install worker function relative to 3rd party copy command set
 *
 * @param[in] pu8Cdb
 * @param[in] ppHook
 * @retval 0: Success to install functions / -1: Fail to install functions
 */
int __isntall_tpc_proc(
    IN u8 *pu8Cdb,
    IN void **ppHook
    )
{
    TPC_SAC *pTpcSac = NULL;
    u8 u8Index0 = 0, u8Index1 = 0, u8SAC = 0;

    BUG_ON(!pu8Cdb);
    BUG_ON(!ppHook);

    u8SAC = (pu8Cdb[1] & 0x1f);

    // To check the TPC opcode ...
    for (u8Index0 = 0; u8Index0 < MAX_TPC_CMD_INDEX; u8Index0++){
        if (pu8Cdb[0] != gTpcTable[u8Index0].u8OpCode)
            continue;

        // To check the service action code for TPC ...
        pTpcSac = gTpcTable[u8Index0].pSAC;
        for (u8Index1 = 0; (0xff != pTpcSac[u8Index1].u8SAC); u8Index1++){    
            if ((u8SAC == pTpcSac[u8Index1].u8SAC) && (pTpcSac[u8Index1].pProc)){
                    pr_debug("%s: Success to install TPC proc, cdb[0]:0x%x, u8SAC:0x%x\n",
                        __func__, pu8Cdb[0], u8SAC);

                *ppHook = pTpcSac[u8Index1].pProc;
                return 0;
            }
        }
    }

    *ppHook = NULL;
    return -1;
}


/*
 * @fn void __set_err_reason(IN ERR_REASON_INDEX ErrCode, IN OUT u8 *pOutReason)
 * @brief Wrapper function to set command error code in scsi_sense_reason
 *
 * @sa 
 * @param[in] ErrCode
 * @param[in] pOutReason
 * @retval N/A
 */
void __set_err_reason(
	IN ERR_REASON_INDEX ErrCode,
	IN OUT u8 *pOutReason
	)
{
	if (pOutReason == NULL || ErrCode > MAX_ERR_REASON_INDEX)
		return;

	pr_debug("warning: %s\n", gErrReasonTable[ErrCode].err_str);
	*pOutReason = (u8)gErrReasonTable[ErrCode].err_reason;
	return;
}
EXPORT_SYMBOL(__set_err_reason);


void __init_cb_data(
	CB_DATA *pData,
	void *pContects
	)
{
	pData->wait = pContects;
	pData->nospc_err= 0;
	atomic_set(&pData->BioCount, 1);
	atomic_set(&pData->BioErrCount, 0);
	return;
}


#define D4_T_S	10

int  __submit_bio_wait(
	struct bio_list *bio_lists,
	u8 cmd,
	unsigned long timeout
	)
{
	DECLARE_COMPLETION_ONSTACK(wait);
	CB_DATA cb_data;
	unsigned long t;
	struct bio *pBio = NULL;
	struct blk_plug Plug;
	IO_REC *rec = NULL;

	if (bio_lists == NULL)
		BUG_ON(TRUE);

	if (timeout)
		t = timeout;
	else
		t = msecs_to_jiffies(D4_T_S * 1000);

	__init_cb_data(&cb_data, &wait);
	blk_start_plug(&Plug);
	while (TRUE) {
		pBio = bio_list_pop(bio_lists);
		if (!pBio)
			break;
		rec = (IO_REC *)pBio->bi_private;
		rec->cb_data = &cb_data;
		atomic_inc(&(cb_data.BioCount));
		submit_bio(cmd, pBio);
	}
	blk_finish_plug(&Plug);

	if (!atomic_dec_and_test(&(cb_data.BioCount))) {
		while (wait_for_completion_timeout(&wait, t) == 0)
			pr_err("wait bio to be done\n");
	}

	if (atomic_read(&cb_data.BioErrCount)) {
		if (cb_data.nospc_err)
			return -ENOSPC;
		else
			return -EIO;
	}

	return 0;
}


void __do_pop_put_bio(
    IN struct bio_list *biolist
    )
{
	struct bio *bio = NULL;

	if (!biolist)
		return;

	while (TRUE){
		bio = bio_list_pop(biolist);
		if (!bio)
			break;
		bio_put(bio);
	}
	return;
}


struct bio *__get_one_bio(
	GEN_RW_TASK *task,
	BIO_DATA *bio_data,
	sector_t block_lba
	)
{
	struct iblock_dev *ib_dev = NULL; 
	struct bio *mybio = NULL;

	 /**/
	if (!task)
		return NULL;

	ib_dev  = task->se_dev->dev_ptr;

	/* To limit to allocate one bio for this function */
	mybio = bio_alloc_bioset(GFP_NOIO, 1, ib_dev->ibd_bio_set);
	if (!mybio){
		pr_err("unable to allocate memory for bio\n");
		return NULL;
	}

	mybio->bi_bdev = ib_dev->ibd_bd;

#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26)) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6))
	mybio->bi_destructor = bio_data->bi_destructor;
#elif (LINUX_VERSION_CODE == KERNEL_VERSION(3,10,20)) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
#else
#error "Ooo.. what kernel version do you compile ??"
#endif

	mybio->bi_end_io = bio_data->bi_end_io;
	mybio->bi_sector = block_lba;

	pr_debug("%s - allocated bio: 0x%p, lba:0x%llx\n", __FUNCTION__, 
		mybio, (unsigned long long)mybio->bi_sector);

	return mybio;
}


void __add_page_to_one_bio(
	struct bio *bio,
	struct page *page,
	unsigned int len,
	unsigned int off
	)
{
	bio->bi_io_vec[0].bv_page = page;
	bio->bi_io_vec[0].bv_len = len;
	bio->bi_io_vec[0].bv_offset = off;
	bio->bi_flags = 1 << BIO_UPTODATE;
	bio->bi_vcnt = 1;
	bio->bi_idx = 0;
	bio->bi_size = len;
	return;
}

void __make_rw_task(
	GEN_RW_TASK *task,
	LIO_SE_DEVICE *se_dev,
	sector_t lba,
	u32 nr_blks,
	unsigned long timeout,
	enum dma_data_direction dir
	)
{
	task->se_dev = se_dev;
	task->lba = lba;
	task->nr_blks = nr_blks;
	task->dev_bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
	task->dir = dir;
	task->timeout_jiffies = timeout;
	task->is_timeout = 0;
	task->ret_code = 0;
	return;
}
EXPORT_SYMBOL(__make_rw_task);

void __generic_free_sg_list(
	struct scatterlist *sg_list,
	u32 sg_nent
	)
{
	int i = 0;

	if (!sg_list || !sg_nent)
		return;

	for (i = 0; i < sg_nent; i++)
		__free_pages(sg_page(&sg_list[i]), get_order(sg_list[i].length));

	kfree(sg_list);
	return;
}
EXPORT_SYMBOL(__generic_free_sg_list);


int __generic_alloc_sg_list(
	u64 *data_size,
	struct scatterlist **sg_list,
	u32 *sg_nent
	)
{
#define MAX_ALLOC_SIZE	(1024*1024)

	u64 buf_size = 0, tmp_data_size = 0; 
	int alloc_size = SG_MAX_IO, nents = 0, i = 0, max_alloc = 0;
	struct scatterlist *sgl = NULL;
	struct page *page = NULL;
	struct list_head tmp_sg_data_list;
	
	typedef struct _tmp_sg_data{
	    struct list_head sg_node;
	    struct scatterlist *sg;
	} TMP_SG_DATA;

	TMP_SG_DATA *sg_data = NULL, *tmp_sg_data = NULL;

	/**/
	if (!data_size)
		return -EINVAL;

	if (*data_size == 0)
		return -EINVAL;

	max_alloc = (MAX_ALLOC_SIZE / alloc_size);
	tmp_data_size = *data_size;
	INIT_LIST_HEAD(&tmp_sg_data_list);

	/* To prepare the tmp sg list. Here will try to find how many sg
	 * we can allocate. Please note the allocation unit must be KB unit here
	 */
	while (tmp_data_size){

_AGAIN_:
		buf_size = min_t(int, tmp_data_size, alloc_size);
		page = alloc_pages((GFP_KERNEL | __GFP_ZERO | __GFP_NOWARN), 
			get_order(buf_size));

		/* give the chance to re-allocate memory */
		if (!page){
			if (alloc_size < PAGE_SIZE)
				alloc_size -= alloc_size;
			else
				alloc_size -= PAGE_SIZE;

			if (alloc_size)
				goto _AGAIN_;

			if (!list_empty(&tmp_sg_data_list))
				break;
			return -ENOMEM;
		}

		sgl = kzalloc(sizeof(struct scatterlist), GFP_KERNEL);
		sg_data = kzalloc(sizeof(TMP_SG_DATA), GFP_KERNEL);

		if (!sg_data || !sgl || !page){
			if (page)
				__free_pages(page, get_order(buf_size));
			if (sgl)
				kfree(sgl);
			if (sg_data)
				kfree(sg_data);
			break;
		}

		sg_init_table(sgl, 1);
		sg_set_page(sgl, page, buf_size, 0);

		sg_data->sg = sgl;
		INIT_LIST_HEAD(&sg_data->sg_node);
		list_add_tail(&sg_data->sg_node, &tmp_sg_data_list);

		tmp_data_size -= buf_size;
		nents++;

		if (nents == max_alloc)
			break;

	}

	if (!nents)
		return -ENOMEM;

	BUG_ON(list_empty(&tmp_sg_data_list));

	/**/
	sgl = kzalloc(sizeof(struct scatterlist) * nents, GFP_KERNEL);
	if (!sgl){
		list_for_each_entry_safe(sg_data, tmp_sg_data, 
			&tmp_sg_data_list, sg_node)
		{
			list_del_init(&sg_data->sg_node);
			__free_pages(sg_page(sg_data->sg), 
				get_order(sg_data->sg->length));
			kfree(sg_data->sg);
			kfree(sg_data);
		}
		return -ENOMEM;
	}

	/* To prepare the real sg list */
	tmp_data_size = 0;
	sg_init_table(sgl, nents);

	list_for_each_entry_safe(sg_data, tmp_sg_data, 
		&tmp_sg_data_list, sg_node)
	{
		list_del_init(&sg_data->sg_node);
		tmp_data_size += sg_data->sg->length;
		sg_set_page(&sgl[i++], sg_page(sg_data->sg), 
			sg_data->sg->length, sg_data->sg->offset);

		/* remove the tmp data*/
		kfree(sg_data->sg);
		kfree(sg_data);
	}


	*data_size = tmp_data_size;
	*sg_list = sgl;
	*sg_nent = nents;
	return 0;
}
EXPORT_SYMBOL(__generic_alloc_sg_list);


int __do_get_max_sectors_in_bytes(
	LIO_SE_DEVICE *pSeDev,
	u32 *pu32MaxSectorsInBytes
	)
{
	struct block_device *pBD = NULL;
	struct file *pFile = NULL;
	struct inode *pInode = NULL;
	void *pDevice= NULL;
	int Ret = 0;

	SUBSYSTEM_TYPE Type = MAX_SUBSYSTEM_TYPE;

	*pu32MaxSectorsInBytes = 0;

	if(__do_get_subsystem_dev_type(pSeDev, &Type) != 0)
		return 1;

	if(__do_get_subsystem_dev_ptr_by_type(pSeDev, Type, &pDevice) != 0)
		return 1;

	if(SUBSYSTEM_BLOCK == Type){

		pBD = (struct block_device *)pDevice;   
		*pu32MaxSectorsInBytes  = (queue_max_sectors(bdev_get_queue(pBD)) << 9);
		Ret = 0;

	}else if(SUBSYSTEM_FILE == Type){

		//
		// I refer the fd_create_virtdevice() to do this ...
		//
		pFile = (struct file *)pDevice;   
		pInode = pFile->f_mapping->host;
		if (S_ISBLK(pInode->i_mode)) {
			*pu32MaxSectorsInBytes  = (queue_max_sectors(bdev_get_queue(pInode->i_bdev)) << 9);
		}else{
			*pu32MaxSectorsInBytes  = (FD_MAX_SECTORS << 9);
		}
		Ret = 0;

	}else if(SUBSYSTEM_PSCSI == Type){
		Ret = 1;
	}else{
		Ret = 1;
	}

	return Ret;

}

int __do_get_max_hw_sectors_in_bytes(
	LIO_SE_DEVICE *pSeDev,
	u32 *pu32MaxHwSectorsInBytes
	)
{
	struct block_device *pBD = NULL;
	struct file *pFile = NULL;
	struct inode *pInode = NULL;
	void *pDevice= NULL;
	int Ret = 0;
	SUBSYSTEM_TYPE Type = MAX_SUBSYSTEM_TYPE;

	*pu32MaxHwSectorsInBytes = 0;

	if(__do_get_subsystem_dev_type(pSeDev, &Type) != 0)
		return 1;

	if(__do_get_subsystem_dev_ptr_by_type(pSeDev, Type, &pDevice) != 0)
		return 1;

	if(SUBSYSTEM_BLOCK == Type){
		pBD = (struct block_device *)pDevice;   
		*pu32MaxHwSectorsInBytes  = (queue_max_hw_sectors(bdev_get_queue(pBD)) << 9);
		Ret = 0;
	}else if(SUBSYSTEM_FILE == Type){

		//
		// I refer the fd_create_virtdevice() to do this ...
		//
		pFile = (struct file *)pDevice;   
		pInode = pFile->f_mapping->host;
		if (S_ISBLK(pInode->i_mode)) {
			*pu32MaxHwSectorsInBytes  = (queue_max_hw_sectors(bdev_get_queue(pInode->i_bdev)) << 9);
		}else{
			*pu32MaxHwSectorsInBytes  = (FD_MAX_SECTORS << 9);
		}
		Ret = 0;

	}else if(SUBSYSTEM_PSCSI == Type){
		Ret = 1;
	}else{
		Ret = 1;
	}
	return Ret;

}

u8 __do_check_is_thin_lun(
	IN LIO_SE_DEVICE *pSeDev
	)
{
	if(pSeDev == NULL)
		return FALSE;

	/* FIXED ME !!
	 * 
	 * Here to use the emulate_tpu value and emulate_tpws to identify
	 * this device is thin or think.
	 */
	if (pSeDev->se_sub_dev->se_dev_attrib.emulate_tpu 
	|| pSeDev->se_sub_dev->se_dev_attrib.emulate_tpws
	)
		return TRUE;
	else
		return FALSE;
}

int __do_get_subsystem_dev_type(
	LIO_SE_DEVICE *pSeDev,
	SUBSYSTEM_TYPE *pType
	)
{
	LIO_SE_SUBSYSTEM_API *pTransport = NULL;
	int Ret = 0;

	// FIXED ME
	//
	// This is simple method to check the sub-system device type. It may be
	// changed in the future
	//

	pTransport = pSeDev->transport;

	*pType = MAX_SUBSYSTEM_TYPE;

	if(!strcmp(pTransport->name, "iblock")){
		*pType = SUBSYSTEM_BLOCK;
	}else if(!strcmp(pTransport->name, "fileio")){
		*pType = SUBSYSTEM_FILE;
//	}else if(!strcmp(pTransport->name, "pscsi")){
//		*pType = SUBSYSTEM_PSCSI;
	}else{
		Ret = 1;  // currently, not support pscsi i/o
	}

	if(Ret == 0)
		pr_debug("subsystem_dev_type: 0x%x\n", *pType);

	return Ret;
}

int __do_get_subsystem_dev_ptr_by_type( 
	LIO_SE_DEVICE *pSeDev,
	SUBSYSTEM_TYPE Type,
	void **ppSubSysDev
	)
{
	LIO_IBLOCK_DEV *pIBD = NULL;
	LIO_FD_DEV *pFdDev = NULL;
	struct block_device *pBD = NULL;
	struct file *pFile = NULL;
	int Ret = 0;

	switch(Type){
	case SUBSYSTEM_BLOCK:
		pIBD = pSeDev->dev_ptr;
		pBD  = pIBD->ibd_bd;
		*ppSubSysDev = (void*)pBD;
		break;

	case SUBSYSTEM_FILE:
		//
		// I refer the fd_create_virtdevice() to do this ...
		//
		pFdDev = pSeDev->dev_ptr;
		pFile  = pFdDev->fd_file;
		*ppSubSysDev = (void*)pFile;
		break;

	case SUBSYSTEM_PSCSI:
	case MAX_SUBSYSTEM_TYPE:
	default:
		Ret = 1;
		break;
	}

	return Ret;
}

static void __bio_end_io(
	struct bio *bio,
	int err
	)
{
	CB_DATA *p = NULL;
	IO_REC *rec = NULL;

//	pr_err("%s\n", __func__);

	rec = (IO_REC *)bio->bi_private;
	p = rec->cb_data;

	if (!test_bit(BIO_UPTODATE, &bio->bi_flags) && !err)
		err = -EIO;

	rec->transfer_done = 1;
	if (err != 0) {
		if (err == -ENOSPC)
			p->nospc_err = 1;

		rec->transfer_done = -1; // treat it as error
		atomic_inc(&p->BioErrCount);
		smp_mb__after_atomic_inc();
	}

// TODO: REMOVE
//	if (atomic_read(&p->BioErrCount))
//		rec->transfer_done = -1;

	bio_put(bio);
	if (atomic_dec_and_test(&p->BioCount))
		complete(p->wait);

	return;
}

#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26)) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6))
static void __bio_destructor(
	struct bio *bio
	)
{
	IO_REC *rec = (IO_REC *)bio->bi_private;

	if (rec) {
		LIO_IBLOCK_DEV *ibd = (LIO_IBLOCK_DEV *)rec->pIBlockDev;
		bio_free(bio, ibd->ibd_bio_set);
	}
	return;
}
#endif

static sector_t __get_done_blks_by_io_rec_list(
	struct list_head *io_rec_list
	)
{
	IO_REC *rec;
	sector_t done = 0;
	
	list_for_each_entry(rec, io_rec_list, node){
		/* Only computed the transferred-done part. This shall
		 * match the __bio_end_io() function
		 */
		if (rec->transfer_done != 1)
			break;
		done += (sector_t)rec->nr_blks;
	}
	return done;
}

static void __free_io_rec_by_io_rec_list(
	struct list_head *io_rec_list
	)
{
	IO_REC *rec = NULL, *tmp_rec = NULL;

	list_for_each_entry_safe(rec, tmp_rec, io_rec_list, node)
		kfree(rec);
	return;
}

static int __do_vfs_rw(
	GEN_RW_TASK *task,
	VFS_RW vfs_rw_func   
	)
{
	LIO_SE_DEVICE *se_dev = NULL;
	struct scatterlist *sg = NULL;
	struct fd_dev *f_dev = NULL;
	loff_t pos = 0;
	sector_t dest_lba = 0;
	u32 i = 0, done = 0;
	u64 expected_bcs = 0, len = 0;
	int ret = 0, code = -EINVAL;
	struct iovec iov;
	mm_segment_t old_fs;

	int err_1, err_2;
	loff_t start, end;
	struct inode *inode;

	if (!task)
		goto _EXIT_;

	if ((!task->se_dev)
	|| (task->dir == DMA_BIDIRECTIONAL)
	|| (task->dir == DMA_NONE)
	)
		goto _EXIT_;

	expected_bcs = ((u64)task->nr_blks << task->dev_bs_order);
	if (!expected_bcs)
		goto _EXIT_;

	code = 0;
	dest_lba = task->lba;
	se_dev = task->se_dev;
	f_dev = se_dev->dev_ptr;

	/* Here, we do vfs_rw per sg at a time. The reason is we need to compuated
	 * the transfer bytes for the result of every i/o.
	 */
	for_each_sg(task->sg_list, sg, task->sg_nents, i) {

		if (!expected_bcs)
			break;

		/* To prepare iov. To care the expected transfer bytes may be
		 * more or less than the sg->length
		 */
		len = min_t(u64, expected_bcs, sg->length);
		iov.iov_len  = len;
		iov.iov_base = sg_virt(sg);

		pr_debug("%s - dir:0x%x, expected_bcs:0x%llx, sg->length:0x%x,"
			"iov_base:0x%p, iov_len:0x%llx\n", __FUNCTION__, 
			task->dir, expected_bcs, sg->length,
			iov.iov_base, (u64)iov.iov_len);

		/**/
		pos = (dest_lba << task->dev_bs_order);

		start = pos;
		end = start + len;

		dest_lba += (len >> task->dev_bs_order);
		expected_bcs -= len;

		pr_debug("%s - dir:0x%x, pos:0x%llx, dest_lba:0x%llx\n",
			__FUNCTION__, task->dir, pos, (unsigned long long)dest_lba);

		if (IS_TIMEOUT(task->timeout_jiffies)){
			task->is_timeout = 1;
			break;
		}

		/* FIXED ME (need to be checked)
		 * (1)  Here is to use one vector only. The reason is we need 
		 *      to reoprt real data size we transfer from src to dest. 
		 * (2)  In the other words, we need to know which io vector was
		 *      error if we use multiple io vectors
		 */
		old_fs = get_fs();
		set_fs(get_ds());
		ret = vfs_rw_func(f_dev->fd_file, &iov, 1, &pos);
		set_fs(old_fs);

		if (ret <= 0){
			code = ret;
			break;
		} else{
			done += ((u32)ret >> task->dev_bs_order);
			pr_debug("%s - dir:0x%x, done blks:0x%x\n", 
				__FUNCTION__, task->dir, done);
			if (ret != len){
				code = -EIO;
				break;
			}

#if defined(SUPPORT_TP)
			/* TODO: 
			 * To sync cache again if write ok and the sync cache
		  	 * behavior shall work for thin lun only 
		  	 */
			if (task->dir != DMA_TO_DEVICE)
				continue;

			if (!is_thin_lun(se_dev))
				continue;


			inode = f_dev->fd_file->f_mapping->host;

			/* check whether was no space already ? */
			err_1 = check_dm_thin_cond(inode->i_bdev);
			if (err_1 == 0)
				continue;

			/* time to do sync i/o
			 * 1. hit the sync i/o threshold area
			 * 2. or, space is full BUT need to handle lba where
			 * was mapped or not
			 */
			if (err_1 == 1 || err_1 == -ENOSPC){
				err_1 = __do_sync_cache_range(f_dev->fd_file, 
					start, end);

				if (err_1 != 0){
					/* TODO:
					 * thin i/o may go here (lba wasn't mapped to
					 * any block) or something wrong during normal
					 * sync-cache
					 */
					if (err_1 != -ENOSPC){

						/* call again to make sure it is no space
						 * really or not
						 */
						err_2 = check_dm_thin_cond(inode->i_bdev);
						if (err_2 == -ENOSPC){
							pr_warn("%s: space was full "
								"already\n", __func__);
							err_1 = err_2;
						}		
						/* it may something wrong duing sync-cache */
					} else
						pr_warn("%s: space was full "
							"already\n", __func__);

					code = err_1;
					break;
				}
			}

			/* fall-through */
#endif
		}
	}


	/**/
	if (task->is_timeout){
		pr_err("%s - jiffies > cmd expected-timeout value!!\n",
			__FUNCTION__);
	}
	if (ret <= 0){
		pr_err("%s - vfs_rw_func(dir:%d) returned %d\n", 
			__FUNCTION__, task->dir, ret);
	}
	else if (ret != len){
		pr_err("%s - vfs_rw_func(dir:%d) - "
			"return size:0x%x != expected len:0x%llx\n",
			__FUNCTION__, task->dir, ret, len);
	}

_EXIT_:
	task->ret_code = code;
	return done;

}

int __do_b_rw(
	GEN_RW_TASK *task
	)
{
	LIO_IBLOCK_DEV *ib_dev = NULL; 
	struct bio *mybio = NULL;
	IO_REC *rec = NULL;
	sector_t block_lba = 0, t_lba = 0;
	u32 i = 0, dev_bs_order = 0, err = 0, done = 0;
	u64 expected_bcs = 0, len = 0;
	u8 cmd = 0;
	struct scatterlist *sg = NULL;
	unsigned bio_cnt = 0;
	struct bio_list bio_lists;
	struct list_head io_rec_list;
	int code = -EINVAL;
	BIO_DATA bio_data = {
		.bi_end_io = __bio_end_io,
#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26)) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6))
		.bi_destructor = __bio_destructor
#elif (LINUX_VERSION_CODE == KERNEL_VERSION(3,10,20)) || (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
#else
#error "Ooo.. what kernel version do you compile ??"
#endif
		};


	/* Here refers the iblock_do_task() */
	if (!task)
		goto _EXIT_;

	if ((!task->se_dev)
	|| (task->dir == DMA_BIDIRECTIONAL)
	|| (task->dir == DMA_NONE)
	)
		goto _EXIT_;

	/**/
	cmd = ((task->dir == DMA_FROM_DEVICE) ? 0 : REQ_WRITE);
	ib_dev = task->se_dev->dev_ptr;
	block_lba = t_lba = task->lba;
	dev_bs_order = task->dev_bs_order;
	expected_bcs = ((sector_t)task->nr_blks << dev_bs_order);
	if (!expected_bcs)
		goto _EXIT_;

	/**/
	code = 0;
	INIT_LIST_HEAD(&io_rec_list);
	bio_list_init(&bio_lists);

	/**/
	for_each_sg(task->sg_list, sg, task->sg_nents, i){
		if (!expected_bcs)
			break;

		/* Here will map one sg to one bio */
		rec = kzalloc(sizeof(IO_REC), GFP_KERNEL);
		if (!rec){
			if (!bio_list_size(&bio_lists)){
				pr_err("unable to allocate memory for io rec\n");
				err = 1;
				code = -ENOMEM;
				break;
			}
			goto _DO_SUBMIT_;
		}
		INIT_LIST_HEAD(&rec->node);

#if defined(SUPPORT_LOGICAL_BLOCK_4KB_FROM_NAS_GUI)
		__blkio_transfer_task_lba_to_block_lba(
			(1 << dev_bs_order), &block_lba);
#endif
		mybio = __get_one_bio(task, &bio_data, block_lba);
		if (!mybio) {
			kfree(rec);
			if (!bio_list_size(&bio_lists)){
				code = -ENOMEM;
				err = 1;
				break;
			}
			goto _DO_SUBMIT_;
		}

		/* Set something for bio */
		len = min_t(u64, expected_bcs, sg->length);
		__add_page_to_one_bio(mybio, sg_page(sg), len, sg->offset);

		mybio->bi_private = (void *)rec;
		rec->cb_data = NULL;
		rec->nr_blks = (len >> dev_bs_order);
		rec->pIBlockDev = ib_dev;
		list_add_tail(&rec->node, &io_rec_list);

		pr_debug("[%s] cmd:0x%x, sg->page:0x%p, sg->length:0x%x\n",
			__FUNCTION__, cmd, sg_page(sg), sg->length);

		pr_debug("[%s] mybio:0x%p, task lba:0x%llx, "
			"bio block_lba:0x%llx, expected_bcs:0x%llx, "
			"real len:0x%llx \n", __FUNCTION__,
			mybio,
			(unsigned long long)t_lba, (unsigned long long)block_lba,
			(unsigned long long)expected_bcs, (unsigned long long)len);

		bio_list_add(&bio_lists, mybio);
		bio_cnt++;

		/**/
		t_lba += (sector_t)(len >> dev_bs_order);
		block_lba = t_lba;
		expected_bcs -= len;

		if ((bio_cnt < BLOCK_MAX_BIO_PER_TASK) && expected_bcs)
			continue;

_DO_SUBMIT_:
		err = __submit_bio_wait(&bio_lists, cmd, 0);

		/* after to submit, we will do ... */
		done += (u32)__get_done_blks_by_io_rec_list(&io_rec_list);
		__do_pop_put_bio(&bio_lists);
		__free_io_rec_by_io_rec_list(&io_rec_list);

		pr_debug("[%s] done blks:0x%x\n", __FUNCTION__, done);

		if (err){
			code = err;
			break;
		}

		/* To check if timeout happens after to submit */
		if (IS_TIMEOUT(task->timeout_jiffies)){
			task->is_timeout = 1;
			break;
		}

		/**/
		INIT_LIST_HEAD(&io_rec_list);
		bio_list_init(&bio_lists);
		bio_cnt = 0;
	}

	/**/
_EXIT_:
	task->ret_code = code;

	if (err || task->is_timeout){
		if (task->is_timeout)
			pr_err("[%s] jiffies > cmd expected-timeout value !!\n",
				__FUNCTION__);

#if 0 // TODO: REMOVE
		if (err && atomic_read(&cb_data.BioErrCount))
			pr_err("[%s] one of bio was fail !!\n", __FUNCTION__);
		else
			pr_err("[%s] other err happens !!\n", __FUNCTION__);

		__do_pop_put_bio(&bio_lists);
#endif
		return -1;
	}
	return done;
}
EXPORT_SYMBOL(__do_b_rw);


int __do_f_rw(
	GEN_RW_TASK *task
	)
{
	LIO_SE_DEVICE *se_dev =  NULL;
	loff_t start = 0, end = 0, data_size = 0;
	struct fd_dev *f_dev = NULL;
	int ret = 0, sync_ret = 0;

	/* Here refers the fd_do_task() */
	if (!task)
		return -EINVAL;

	if ((!task->se_dev) 
	|| (task->dir == DMA_BIDIRECTIONAL)
	|| (task->dir == DMA_NONE)
	)
		return -EINVAL;

	if (task->dir == DMA_FROM_DEVICE)
		ret = __do_vfs_rw(task, vfs_readv);
	else{

		se_dev = task->se_dev;
		ret = __do_vfs_rw(task, vfs_writev);

		if ((ret > 0)
		&&  (se_dev->se_sub_dev->se_dev_attrib.emulate_write_cache > 0)
		&&  (se_dev->se_sub_dev->se_dev_attrib.emulate_fua_write > 0)
		&&  (task->task_flag & TASK_FLAG_DO_FUA)
		)
		{
			/**/
			f_dev = se_dev->dev_ptr;
			data_size = ((sector_t)task->nr_blks << task->dev_bs_order);
			start = ((sector_t)task->lba << task->dev_bs_order);
			end = start + data_size - 1;

			sync_ret = vfs_fsync_range(f_dev->fd_file, start, end, 1);
			if (sync_ret != 0)
				pr_err("[t_f_rw w/fua] func is failed: 0x%x\n", sync_ret);
		}
	}

	return ret;

}
EXPORT_SYMBOL(__do_f_rw);


/* adamhsu, 
 * (1) redmine bug 6915 - bugzilla 40743
 * (2) redmine bug 6916 - bugzilla 41183
 */
void __call_transport_free_dev_tasks(struct se_cmd *cmd)
{
	transport_free_dev_tasks(cmd);
	return;
}

/* adamhsu, 
 * (1) redmine bug 6915 - bugzilla 40743
 * (2) redmine bug 6916 - bugzilla 41183
 */
/*
 * @fn static void __remove_cmd_rec(LIO_SE_CMD *se_cmd)
 * @brief Remove the temporary se_cmd record from se_dev
 *
 * @note
 * @
 * @param[in] se_cmd
 * @retval None
 */
static void __remove_cmd_rec(
	LIO_SE_CMD *se_cmd
	)
{
	LIO_SE_DEVICE *se_dev;
	T_CMD_REC *rec, *tmp_rec;
	unsigned long flags;

	/* This will be called from transport_generic_free_cmd() ONLY. When
	 * code comes here, the se_dev may be freed already 
	 */
	if (!se_cmd->se_dev){
		pr_debug("%s -  se_dev is NULL\n", __FUNCTION__);
		return;
	}

	se_dev = se_cmd->se_dev;

	spin_lock_irqsave(&se_dev->cmd_rec_lock, flags);

	pr_debug("%s - [before] curr cmd rec count:%d\n", __FUNCTION__, 
		atomic_read(&se_dev->cmd_rec_count));

	list_for_each_entry_safe(rec, tmp_rec, &se_dev->cmd_rec_list, rec_node){
		if (rec->se_cmd == se_cmd){
			pr_debug("%s - found rec se_cmd:0x%p\n", __FUNCTION__, 
				se_cmd);
			list_del_init(&rec->rec_node);
			atomic_dec(&se_dev->cmd_rec_count);
			kfree(rec);
		}
	}

	pr_debug("%s - [after] curr cmd rec count:%d\n", __FUNCTION__, 
		atomic_read(&se_dev->cmd_rec_count));

	spin_unlock_irqrestore(&se_dev->cmd_rec_lock, flags);
	return;	

}

/* adamhsu, 
 * (1) redmine bug 6915 - bugzilla 40743
 * (2) redmine bug 6916 - bugzilla 41183
 */
/*
 * @fn static void __create_cmd_rec(LIO_SE_CMD *se_cmd)
 * @brief Create the temporary se_cmd record on se_dev
 *
 * @note
 * @
 * @param[in] se_cmd
 * @retval None
 */
static void __create_cmd_rec(
	LIO_SE_CMD *se_cmd
	)
{
	LIO_SE_DEVICE *se_dev = se_cmd->se_dev;
	T_CMD_REC *new_rec = NULL, *old_rec;
	unsigned long flags;
	int found = 0;

	new_rec = kzalloc(sizeof(T_CMD_REC), GFP_ATOMIC);

	INIT_LIST_HEAD(&new_rec->rec_node);

	spin_lock_irqsave(&se_dev->cmd_rec_lock, flags);
	if (list_empty(&se_dev->cmd_rec_list))
		goto _ADD_;

	list_for_each_entry(old_rec, &se_dev->cmd_rec_list, rec_node){
		if (old_rec->se_cmd == se_cmd)
			found = 1;
	}

_ADD_:
	if (!found){
		pr_debug("%s - add new rec se_cmd:0x%p\n", __FUNCTION__, se_cmd);
		new_rec->se_cmd = se_cmd;
		list_add_tail(&new_rec->rec_node, &se_dev->cmd_rec_list);
		atomic_inc(&se_dev->cmd_rec_count);
	}
	spin_unlock_irqrestore(&se_dev->cmd_rec_lock, flags);
	return;	

}

#if defined(SUPPORT_TP)
/* 2014/06/14, adamhsu, redmine 8530 (start) */
int __get_file_lba_map_status(
	LIO_SE_DEVICE *se_dev, 
	struct block_device *bd,
	struct inode *inode,
	sector_t lba, 
	u32 *desc_count, 
	u8 *desc_buf
	)
{
#define SIZE_ORDER	20

	struct fiemap_extent *file_ext = NULL;
	struct fiemap_extent_info file_info;
	u32 idx = 0, fe_idx = 0, bs_order = 0, real_count = 0;
	u32 nr_blks, total_blks = 0, tmp = *desc_count;
	u64 maxbytes = 0;
	sector_t curr_lba, next_lba;
	loff_t pos, len;
	int ret = -ENODEV, is_not_map, found_map = 0;
	SUBSYSTEM_TYPE type;
	LBA_STATUS_DESC *lba_stats_desc = (LBA_STATUS_DESC *)desc_buf;

	/**/
	if (__do_get_subsystem_dev_type(se_dev, &type) != 0)
		goto _EXIT_;

	if (type == SUBSYSTEM_BLOCK){
		/* block i/o + file-based configuration */
		if (!bd)
			goto _EXIT_;
		if (strncmp(bd->bd_disk->disk_name, "fbdisk", 6))
			goto _EXIT_;

	} else if (type == SUBSYSTEM_FILE){
		if (S_ISBLK(inode->i_mode))
			goto _EXIT_;
	} else
		BUG_ON(1);

	if (!inode->i_op->fiemap){
		ret = -EOPNOTSUPP;
		goto _EXIT_;
	}

	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
	pos = ((loff_t)lba << bs_order);
	len = (loff_t)(tmp << SIZE_ORDER);

	/* please refer the fiemap_check_ranges() */
	maxbytes = (u64) inode->i_sb->s_maxbytes;
	if (pos > maxbytes){
		ret = -EFBIG;
		goto _EXIT_;
	}		

	/* Shrink request scope to what the fs can actually handle. */
	if (len > maxbytes || (maxbytes - len) < pos)
		len = maxbytes - pos;

	/* The final count / len may be smaller than original ones
	 * so to re-get them
	 */
	tmp = (len >> SIZE_ORDER);
	len = (tmp << SIZE_ORDER);

	pr_debug("%s, count:0x%llx\n", __FUNCTION__, (unsigned long long)tmp);
	pr_debug("%s, pos:0x%llx\n", __FUNCTION__, (unsigned long long)pos);
	pr_debug("%s, len:0x%llx\n", __FUNCTION__, (unsigned long long)len);

	file_ext = kzalloc((tmp * sizeof(struct fiemap_extent)), 
			GFP_KERNEL);
	if (!file_ext){
		ret = -ENOMEM;
		goto _EXIT_;
	}

	/* Do flush first */
	if (type == SUBSYSTEM_BLOCK){
		ret = blkdev_issue_flush(bd, GFP_KERNEL, NULL);
		if (ret != 0)
			goto _EXIT_;		
	}

	/**/
	file_info.fi_flags = FIEMAP_FLAG_SYNC;;
	file_info.fi_extents_max = tmp;
	file_info.fi_extents_mapped = 0;
	file_info.fi_extents_start = file_ext;

	if (file_info.fi_flags & FIEMAP_FLAG_SYNC)
		filemap_write_and_wait(inode->i_mapping);

	ret = inode->i_op->fiemap(inode, &file_info, pos, len);
	if (ret != 0)
		goto _EXIT_;		

	if (unlikely(!file_info.fi_extents_mapped)){
		pr_debug("%s: not found any mapped extent\n", __FUNCTION__);
		goto _NOT_FOUND_MAP_;
	}

	pr_debug("%s: mapped extent count:%d\n", __FUNCTION__, 
			file_info.fi_extents_mapped);

	/* (1) the lba status desc count may be larger than fi_extents_mapped
	 * (2) we need to take care the gap (deallocated) case
	 *
	 * i.e:
	 * If want to get status of lba:0x123 (off:0x24600) but the mapping was
	 * started from lba:0x180 (off:0x30000). Hence, the lba status 
	 * descriptor will be
	 *
	 * desc[0] - 0x123 ~ 0x17f (deallocated)
	 * desc[1] - 0x180 ~ 0xYYY (mapped)
	 *
	 * (3) if possible, we may prepare one descriptor at the tail
	 */
	idx = 0;
	real_count = 0;

	for (fe_idx = 0; fe_idx < file_info.fi_extents_mapped; fe_idx++){
	
#if 0
		pr_err("idx:%d, file_ext_idx:%d\n",idx, fe_idx);
		pr_err("file_info.fe_logical:0x%llx\n", 
			(unsigned long long)file_ext[fe_idx].fe_logical);
		pr_err("file_info.fe_physical:0x%llx\n", 
			(unsigned long long)file_ext[fe_idx].fe_physical);
		pr_err("file_info.fe_length:0x%llx\n", 
			(unsigned long long)file_ext[fe_idx].fe_length);
		pr_err("file_info.fe_flags:0x%x\n", file_ext[fe_idx].fe_flags);
#endif

		if (likely(file_ext[fe_idx].fe_logical == pos))
			found_map = 1;
		else {
			/* the fs block size is 4kb, so the pos may not be 
			 * aligned by fe_logical value. Just for 1st ext info
			 * i.e.
			 * pos: 0x896c00
			 * fe_logical: 0x896000
			 */
			if (fe_idx == 0)
				found_map = 1;
			else
				found_map = 0;
		}

_PREPARE_:
		if (found_map){
			found_map = 0;
			is_not_map = 0;
			curr_lba = (file_ext[fe_idx].fe_logical >> bs_order);
			nr_blks = (file_ext[fe_idx].fe_length >> bs_order);
	
			/* next pos */
			pos = file_ext[fe_idx].fe_logical + \
					file_ext[fe_idx].fe_length;
		} else {
			found_map = 1;
			is_not_map = 1;
			curr_lba = (pos >> bs_order);
			nr_blks = ((file_ext[fe_idx].fe_logical - pos) >> bs_order);
	
			/* next pos */
			pos = file_ext[fe_idx].fe_logical;
		}
	
		put_unaligned_be64((u64)curr_lba, &lba_stats_desc[idx].lba[0]);
		put_unaligned_be32(nr_blks, &lba_stats_desc[idx].nr_blks[0]);
		lba_stats_desc[idx].provisioning_status |= is_not_map;
	
		total_blks += nr_blks;
		real_count++;
	
		if (real_count == tmp)
			break;

		if (found_map){
			/* go to next desc pos */
			idx++;
			goto _PREPARE_;
		}

		/* go to next desc pos */
		idx++;
	
		/* current extent may NOT be last one ... */
		if (file_ext[fe_idx].fe_flags & FIEMAP_EXTENT_LAST)
			break;

		/* break the loop if next map ext info is invalid ... */
		if (file_ext[fe_idx + 1].fe_length == 0)
			break;
	
	}

	if (real_count){
		/* To check whether we need to prepare the tail of 
		 * lba status descriptor finally */
		if ((total_blks != (len >> bs_order)) 
		&& (tmp > real_count)
		)
		{
			curr_lba += nr_blks;
			put_unaligned_be64((u64)curr_lba, &lba_stats_desc[idx].lba[0]);
			nr_blks = ((len >> bs_order) - total_blks);
			put_unaligned_be32(nr_blks, &lba_stats_desc[idx].nr_blks[0]);
			
			lba_stats_desc[idx].provisioning_status = 1;
			real_count++;
		}
	}

_NOT_FOUND_MAP_:
	if (!real_count){
		/* if not found any map, to report status to all deallocated */
		idx = 0;
		real_count = 1;
		curr_lba = lba;
		nr_blks = ((tmp << SIZE_ORDER) >> bs_order);

		put_unaligned_be64((u64)curr_lba, &lba_stats_desc[idx].lba[0]);
		put_unaligned_be32(nr_blks, &lba_stats_desc[idx].nr_blks[0]);
		lba_stats_desc[idx].provisioning_status |= 0x1;
	}

	pr_debug("%s: real_count:%d\n", __FUNCTION__, real_count);

	*desc_count = real_count;
	ret = 0;

_EXIT_:	
	if (file_ext)
		kfree(file_ext);
	
	return ret;

}
EXPORT_SYMBOL(__get_file_lba_map_status);
/* 2014/06/14, adamhsu, redmine 8530 (end) */
#endif


/* 20140513, adamhsu, redmine 8253 */
#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))



void transport_init_tag_pool(
	struct se_session *se_sess
	)
{
	int i;
	struct new_tag_pool *pool;

	for (i = 0; i < MAX_TAG_POOL; i++){
		pool = &se_sess->tag_pool[i];
		spin_lock_init(&pool->tag_count_lock);
		atomic_set(&pool->tag_count, 0);
	}
	return;
}


int transport_alloc_pool_tag(
	struct se_session *se_sess, 
	struct se_cmd *se_cmd, 
	int is_session_reinstatement
	)
{
	int tag = -1, new_pool_idx, from_emergency_pool = 0;
	struct new_tag_pool *new_pool;

	/* 1.
	 * Here, we will pass the GFP_ATOMIC to percpu_ida_alloc() only to
	 * avoid it will be stuck if not found any available tag. So, we can do
	 * our handling procedure
	 *
	 * 2.
	 * This call may be called from software interrupt (timer) context
	 * for allocating iSCSI NopINs
	 */
	se_cmd->tag_src_pool = TAG_IS_INVALID;

	/* step1: try alloc tag from native code design */
	if (se_sess->sess_cmd_map){
		tag = percpu_ida_alloc(&se_sess->sess_tag_pool, GFP_ATOMIC);
		if (tag >= 0){
			se_cmd->tag_src_pool = TAG_FROM_NATIVE_POOL;
			goto _EXIT_;
		}
		pr_debug("fail to alloc tag from sess_tag_pool, "
			"se_sess:0x%p\n", se_sess);
	}

	/* step2: try alloc from new tag pool */
	for (new_pool_idx = 0; new_pool_idx < MAX_TAG_POOL; new_pool_idx++){

		if ((MAX_TAG_POOL >= 2) 
		&& (new_pool_idx == (MAX_TAG_POOL - 1)))
		{
			if (!(is_session_reinstatement || in_interrupt()))
				break;

			/* if new tag pool count >= 2 and now is at final pool,
			 * try check whether we are in interrupt context or 
			 * session reinstatement, if yes, try alloc tag from
			 * final emergency pool
			 */
			pr_warn("try alloc tag from emergency tag pool\n");
			from_emergency_pool = 1;
		}

		new_pool = &se_sess->tag_pool[new_pool_idx];
		if (new_pool->sess_cmd_map){
			if (from_emergency_pool)
				pr_info("try emergency tag pool %d, "
				"se_sess:0x%p\n", new_pool_idx, se_sess);

			tag = percpu_ida_alloc(&new_pool->sess_tag_pool, GFP_ATOMIC);
			if (tag >= 0){
				spin_lock(&new_pool->tag_count_lock);
				atomic_inc(&new_pool->tag_count);
				spin_unlock(&new_pool->tag_count_lock);
				se_cmd->tag_src_pool = new_pool_idx;
				goto _EXIT_;
			}

			if (from_emergency_pool){
				pr_info("fail to alloc tag from "
					"emergency tag pool %d, se_sess:0x%p\n", 
					new_pool_idx, se_sess);
			}
		}
	}


	if (se_cmd->tag_src_pool == TAG_IS_INVALID)
		pr_warn("can not alloc tag from native tag pool, "
			"new tag pool %s\n", 
			((from_emergency_pool) ? "and final emergency pool" : ""));

_EXIT_:
	return tag;

}
EXPORT_SYMBOL(transport_alloc_pool_tag);

static int transport_free_extra_tag_pool(
	struct se_session *se_sess
	)
{
	int i, tag_count;
	struct new_tag_pool *pool;

	for (i = 0; i < MAX_TAG_POOL; i++){
		pool = &se_sess->tag_pool[i];

		if (pool->sess_cmd_map){
			spin_lock(&pool->tag_count_lock);
			tag_count = atomic_read(&pool->tag_count);
			spin_unlock(&pool->tag_count_lock);

			WARN_ON((tag_count != 0));
			pr_debug("free new tag pool %d, cmd_map:0x%p, "
				"remain tag count:%d, on se_sess:0x%p\n", 
				i, pool->sess_cmd_map, tag_count, se_sess);

			percpu_ida_destroy(&pool->sess_tag_pool);

			if (is_vmalloc_addr(pool->sess_cmd_map))
				vfree(pool->sess_cmd_map);
			else
				kfree(pool->sess_cmd_map);
		}
	}


	return 0;
}

static int __transport_prepare_extra_tag_pool(
	struct se_session *se_sess,
	struct new_tag_pool *pool,
	unsigned int tag_num, 
	unsigned int tag_size,
	int pool_num
	)
{
	int rc;

	pool->sess_cmd_map = kzalloc(tag_num * tag_size,
		GFP_KERNEL | __GFP_NOWARN | __GFP_REPEAT);

	if (!pool->sess_cmd_map) {
		pool->sess_cmd_map = vzalloc(tag_num * tag_size);
		if (!pool->sess_cmd_map) {
			pr_err("unable to allocate cmd_map "
				"for new tag pool %d by vzalloc()\n", pool_num);
			return -ENOMEM;
		}
	}

	pr_debug("sess_cmd_map:0x%p\n", pool->sess_cmd_map);

	rc = percpu_ida_init(&pool->sess_tag_pool, tag_num);
	if (rc < 0) {
		pr_err("unable to init new tag tool %d, tag_num: %u\n", 
			pool_num, tag_num);
		if (is_vmalloc_addr(pool->sess_cmd_map))
			vfree(pool->sess_cmd_map);
		else
			kfree(pool->sess_cmd_map);
		pool->sess_cmd_map = NULL;
		return -ENOMEM;
	}
	
	return 0;

}


int transport_prepare_extra_tag_pool(
	struct se_session *se_sess,
	unsigned int tag_num, 
	unsigned int tag_size
	)
{
	int i, retry_idx, rc;
	struct new_tag_pool *new_pool;

	for (i = 0; i < MAX_TAG_POOL; i++){

		pr_debug("start to prepare new tag pool %d, se_sess:0x%p\n", 
			i, se_sess);

		new_pool = &se_sess->tag_pool[i];

		for (retry_idx = 0; retry_idx < RETRY_EXTRA_POOL_ALLOC_COUNT; 
			retry_idx++)
		{
			rc = __transport_prepare_extra_tag_pool(
				se_sess, new_pool, tag_num, tag_size, i);

			if (rc == 0){
				pr_debug("done to prepare new tag pool %d, "
					"se_sess:0x%p\n", i, se_sess);
				break;
			}
		
			pr_warn("fail to alloc new tag pool %d, try again. "
				"se_sess:0x%p\n", i, se_sess);
		}

		if (retry_idx == RETRY_EXTRA_POOL_ALLOC_COUNT)
			pr_warn("execeed the retry count after "
				"to alloc new tag pool %d\n", i);

	}

	return 0;
}



int transport_alloc_session_tags(struct se_session *se_sess,
			         unsigned int tag_num, unsigned int tag_size)
{
	int rc;

	se_sess->sess_cmd_map = kzalloc(tag_num * tag_size,
					GFP_KERNEL | __GFP_NOWARN | __GFP_REPEAT);
	if (!se_sess->sess_cmd_map) {
		se_sess->sess_cmd_map = vzalloc(tag_num * tag_size);
		if (!se_sess->sess_cmd_map) {
			pr_err("Unable to allocate se_sess->sess_cmd_map\n");
			return -ENOMEM;
		}
	}

	rc = percpu_ida_init(&se_sess->sess_tag_pool, tag_num);
	if (rc < 0) {
		pr_err("Unable to init se_sess->sess_tag_pool,"
			" tag_num: %u\n", tag_num);
		if (is_vmalloc_addr(se_sess->sess_cmd_map))
			vfree(se_sess->sess_cmd_map);
		else
			kfree(se_sess->sess_cmd_map);
		se_sess->sess_cmd_map = NULL;
		return -ENOMEM;
	}



	/* come here means we have one sess tag pool at least, now try to
	 * alloc others again
	 */
	transport_prepare_extra_tag_pool(se_sess, tag_num, tag_size);



	return 0;
}
EXPORT_SYMBOL(transport_alloc_session_tags);


struct se_session *transport_init_session_tags(unsigned int tag_num,
					       unsigned int tag_size)
{
	struct se_session *se_sess;
	int rc;

	se_sess = transport_init_session();
	if (IS_ERR(se_sess))
		return se_sess;

	rc = transport_alloc_session_tags(se_sess, tag_num, tag_size);
	if (rc < 0) {
		transport_free_session(se_sess);
		return ERR_PTR(-ENOMEM);
	}

	return se_sess;
}
EXPORT_SYMBOL(transport_init_session_tags);

/* adamhsu
 *
 * TODO:
 * For the latest kernel code, the target_submit_cmd() will call 
 * target_submit_cmd_map_sgls() directly. But, we separate them now. It may be
 * changed in the future
 */

/*
 * target_submit_cmd_map_sgls - lookup unpacked lun and submit uninitialized
 * 			 se_cmd + use pre-allocated SGL memory.
 *
 * @se_cmd: command descriptor to submit
 * @se_sess: associated se_sess for endpoint
 * @cdb: pointer to SCSI CDB
 * @sense: pointer to SCSI sense buffer
 * @unpacked_lun: unpacked LUN to reference for struct se_lun
 * @data_length: fabric expected data transfer length
 * @task_addr: SAM task attribute
 * @data_dir: DMA data direction
 * @flags: flags for command submission from target_sc_flags_tables
 * @sgl: struct scatterlist memory for unidirectional mapping
 * @sgl_count: scatterlist count for unidirectional mapping
 * @sgl_bidi: struct scatterlist memory for bidirectional READ mapping
 * @sgl_bidi_count: scatterlist count for bidirectional READ mapping
 *
 * Returns non zero to signal active I/O shutdown failure.  All other
 * setup exceptions will be returned as a SCSI CHECK_CONDITION response,
 * but still return zero here.
 *
 * This may only be called from process context, and also currently
 * assumes internal allocation of fabric payload buffer by target-core.
 */
int target_submit_cmd_map_sgls(struct se_cmd *se_cmd, struct se_session *se_sess,
		unsigned char *cdb, unsigned char *sense, u32 unpacked_lun,
		u32 data_length, int task_attr, int data_dir, int flags,
		struct scatterlist *sgl, u32 sgl_count,
		struct scatterlist *sgl_bidi, u32 sgl_bidi_count)
{
	struct se_portal_group *se_tpg;
	int rc;

	se_tpg = se_sess->se_tpg;
	BUG_ON(!se_tpg);
	BUG_ON(se_cmd->se_tfo || se_cmd->se_sess);
	BUG_ON(in_interrupt());

	/*
	 * Initialize se_cmd for target operation.  From this point
	 * exceptions are handled by sending exception status via
	 * target_core_fabric_ops->queue_status() callback
	 */
	transport_init_se_cmd(se_cmd, se_tpg->se_tpg_tfo, se_sess,
				data_length, data_dir, task_attr, sense);
	/*
	 * Obtain struct se_cmd->cmd_kref reference and add new cmd to
	 * se_sess->sess_cmd_list.  A second kref_get here is necessary
	 * for fabrics using TARGET_SCF_ACK_KREF that expect a second
	 * kref_put() to happen during fabric packet acknowledgement.
	 */
	target_get_sess_cmd(se_sess, se_cmd, (flags & TARGET_SCF_ACK_KREF));

	/*
	 * Signal bidirectional data payloads to target-core
	 */
	if (flags & TARGET_SCF_BIDI_OP)
		se_cmd->se_cmd_flags |= SCF_BIDI;

	/*
	 * Locate se_lun pointer and attach it to struct se_cmd
	 */
	if (transport_lookup_cmd_lun(se_cmd, unpacked_lun) < 0) {
		transport_send_check_condition_and_sense(se_cmd,
				se_cmd->scsi_sense_reason, 0);
		target_put_sess_cmd(se_sess, se_cmd);
		return 0;
	}
	/*
	 * Sanitize CDBs via transport_generic_cmd_sequencer() and
	 * allocate the necessary tasks to complete the received CDB+data
	 */
	rc = transport_generic_allocate_tasks(se_cmd, cdb);
	if (rc != 0) {
		transport_generic_request_failure(se_cmd);
		return 0;
	}

	/*
	 * When a non zero sgl_count has been passed perform SGL passthrough
	 * mapping for pre-allocated fabric memory instead of having target
	 * core perform an internal SGL allocation..
	 */
	if (sgl_count != 0) {
		BUG_ON(!sgl);

		/*
		 * A work-around for tcm_loop as some userspace code via
		 * scsi-generic do not memset their associated read buffers,
		 * so go ahead and do that here for type non-data CDBs.  Also
		 * note that this is currently guaranteed to be a single SGL
		 * for this case by target core in target_setup_cmd_from_cdb()
		 * -> transport_generic_cmd_sequencer().
		 */
		if (!(se_cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB) &&
		     se_cmd->data_direction == DMA_FROM_DEVICE) {
			unsigned char *buf = NULL;

			if (sgl)
				buf = kmap(sg_page(sgl)) + sgl->offset;

			if (buf) {
				memset(buf, 0, sgl->length);
				kunmap(sg_page(sgl));
			}
		}

		rc = transport_generic_map_mem_to_cmd(se_cmd, sgl, sgl_count,
				sgl_bidi, sgl_bidi_count);
		if (rc != 0) {
			transport_generic_request_failure(se_cmd);
			return 0;
		}
	}


	/*
	 * Dispatch se_cmd descriptor to se_lun->lun_se_dev backend
	 * for immediate execution of READs, otherwise wait for
	 * transport_generic_handle_data() to be called for WRITEs
	 * when fabric has filled the incoming buffer.
	 */
	transport_handle_cdb_direct(se_cmd);
	return 0;
}
EXPORT_SYMBOL(target_submit_cmd_map_sgls);
#endif

/* 20140513, adamhsu, redmine 8253 */
#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
void target_execute_cmd(struct se_cmd *cmd)
{
	transport_execute_tasks(cmd);
}
EXPORT_SYMBOL(target_execute_cmd);
#endif


/* 20140626, adamhsu, redmine 8745,8777,8778 (start) */
void __create_aligned_range_desc(
	ALIGN_DESC *desc,
	sector_t start_lba,
	u32 nr_blks_range,
	u32 block_size_order,
	u32 aligned_size
	)
{
	u32 aligned_size_order;
	u64 total_bytes, s_lba_bytes, e_lba_bytes;

	desc->lba = start_lba,
	desc->nr_blks = nr_blks_range;
	desc->bs_order = block_size_order;
	desc->e_align_bytes = aligned_size;
	desc->is_aligned = 0;

	total_bytes = ((u64)nr_blks_range << block_size_order);
	if (total_bytes < (u64)aligned_size)
		return;

	aligned_size_order = ilog2(aligned_size);

	/* convert to byte unit first */
	s_lba_bytes = start_lba << block_size_order;
	e_lba_bytes = s_lba_bytes + total_bytes - (1 << block_size_order);

	pr_debug("%s: s_lba_bytes:0x%llx, e_lba_bytes:0x%llx, "
		"total_bytes:0x%llx\n", __FUNCTION__, 
		(unsigned long long)s_lba_bytes, 
		(unsigned long long)e_lba_bytes, 
		(unsigned long long)total_bytes);

	/* get the new s_lba is aligned by aligned_size */
	s_lba_bytes =  
		(((s_lba_bytes + aligned_size - (1 << block_size_order)) >> \
			aligned_size_order) << aligned_size_order);

	pr_debug("%s: new align s_lba_bytes:0x%llx\n", __FUNCTION__,
		(unsigned long long)s_lba_bytes);
	
	if ((s_lba_bytes > e_lba_bytes)
	|| ((e_lba_bytes - s_lba_bytes + (1 << block_size_order)) < (u64)aligned_size)
	)
		return;

	/* get how many bytes which is multiplied by aligned_size */
	total_bytes = 
		(((e_lba_bytes - s_lba_bytes + (1 << block_size_order)) >> \
		aligned_size_order) << aligned_size_order);

	pr_debug("%s: new align total bytes:0x%llx\n", __FUNCTION__, 
		(unsigned long long)total_bytes);
	
	/* convert to original unit finally */
	desc->lba = (s_lba_bytes >> block_size_order);
	desc->nr_blks = (total_bytes >> block_size_order);
	desc->is_aligned = 1;

	pr_debug("%s: desc->align_lba:0x%llx, desc->align_blks:0x%x\n", 
		__FUNCTION__, 	(unsigned long long)desc->lba, 
		desc->nr_blks);

	return;
}
EXPORT_SYMBOL(__create_aligned_range_desc);
/* 20140626, adamhsu, redmine 8745,8777,8778 (end) */


/* title: bugzilla #32389 and #32390
 * bug desc: please refer bugzilla #32389 and #32390
 * author: adamhsu 2013/04/19
 * method: to use special flag to indicate this discard bio is special or normal
 */

/* Refer from blk-lib.c file */

struct __bio_batch {
	atomic_t		done;
	unsigned long		flags;
	struct completion	*wait;
	int nospc_err;
};

static void __bio_batch_end_io(struct bio *bio, int err)
{
	/* This function was referred by bio_batch_end_io() in blk-lib.c */
	struct __bio_batch *bb = bio->bi_private;

	if (err && (err != -EOPNOTSUPP)){
		clear_bit(BIO_UPTODATE, &bb->flags);
		if (err == -ENOSPC)
			bb->nospc_err = 1;
	}

	if (atomic_dec_and_test(&bb->done))
		complete(bb->wait);

	bio_put(bio);
}


/* This function was referred from blkdev_issue_discard() */
int blkdev_issue_special_discard(
	IN struct block_device *bdev, 
	IN sector_t sector,
	IN sector_t nr_sects, 
	IN gfp_t gfp_mask, 
	IN unsigned long flags
	)
{
	DECLARE_COMPLETION_ONSTACK(wait);
	struct request_queue *q = bdev_get_queue(bdev);
	unsigned int max_discard_sectors;
	struct __bio_batch bb;
	struct bio *bio;
	int ret = 0;

	/* 20140702, adamhsu, redmine 8753 (start) */

	/* The REQ_QNAP_MAP_ZERO is ONLY supported in kernel
	 * - 3.4.6 / 3.10.20 / 3.12.6
	 * For 3.2.26 kernel, the dm-thin layer keep old style code
	 */

#if ((LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6)) \
	|| (LINUX_VERSION_CODE == KERNEL_VERSION(3,10,20)) \
	|| (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6)))

	int type = REQ_WRITE | REQ_QNAP_MAP_ZERO;
		
#elif (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26))
	int type = REQ_WRITE | REQ_DISCARD;
#else
#error "Ooo.. what kernel version do you compile ??"
#endif
	/* 20140702, adamhsu, redmine 8753 (end) */

	if (!q)
	    return -ENXIO;

	if (!blk_queue_discard(q))
	    return -EOPNOTSUPP;

	/*
	 * Ensure that max_discard_sectors is of the proper
	 * granularity
	 */
	max_discard_sectors = min(q->limits.max_discard_sectors, UINT_MAX >> 9);
	if (unlikely(!max_discard_sectors)) {
	    /* Avoid infinite loop below. Being cautious never hurts. */
	    return -EOPNOTSUPP;
	} else if (q->limits.discard_granularity) {
	    unsigned int disc_sects = q->limits.discard_granularity >> 9;
	    max_discard_sectors &= ~(disc_sects - 1);
	}
	
	if (flags & BLKDEV_DISCARD_SECURE) {
	    if (!blk_queue_secdiscard(q))
		return -EOPNOTSUPP;
	    type |= REQ_SECURE;
	}
	
	atomic_set(&bb.done, 1);
	bb.flags = 1 << BIO_UPTODATE;
	bb.wait = &wait;
	bb.nospc_err = 0;


	while (nr_sects) {
		bio = bio_alloc(gfp_mask, 1);
		if (!bio) {
			ret = -ENOMEM;
			break;
		}

		/* 20140702, adamhsu, redmine 8753 (start) */

#if ((LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6)) \
	|| (LINUX_VERSION_CODE == KERNEL_VERSION(3,10,20)) \
	|| (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6)))

		/* nothing to happen ... */				

#elif (LINUX_VERSION_CODE == KERNEL_VERSION(3,2,26))

		 /* To indicate we will do special discard req */
		bio->bi_flags |= (1UL << QNAP_ALLOC_ZEROED_OFFSET);
		pr_err("bi_flags:0x%llx\n",(unsigned long long)bio->bi_flags);
#else
#error "Ooo.. what kernel version do you compile ??"
#endif
		/* 20140702, adamhsu, redmine 8753 (end) */

		bio->bi_sector = sector;
		bio->bi_end_io = __bio_batch_end_io;
		bio->bi_bdev = bdev;
		bio->bi_private = &bb;

		if (nr_sects > max_discard_sectors) {
			bio->bi_size = max_discard_sectors << 9;
			nr_sects -= max_discard_sectors;
			sector += max_discard_sectors;
		} else {
			bio->bi_size = nr_sects << 9;
			nr_sects = 0;
		}
		atomic_inc(&bb.done);
		submit_bio(type, bio);
	}

	/* Wait for bios in-flight */
	if (!atomic_dec_and_test(&bb.done))
		wait_for_completion(&wait);

	if (!test_bit(BIO_UPTODATE, &bb.flags))
		ret = -EIO;

	if (bb.nospc_err)
		ret = -ENOSPC;

	return ret;
}
EXPORT_SYMBOL(blkdev_issue_special_discard);



#if defined(SUPPORT_TP)
#if defined(SUPPORT_VOLUME_BASED)

#if ((LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6)) \
|| (LINUX_VERSION_CODE == KERNEL_VERSION(3,10,20)) \
|| (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6)) \
)


extern int dm_thin_volume_is_full(void *data);

/* 0: normal i/o (not hit sync i/o threshold)
 * 1: hit sync i/o threshold
 * -ENOSPC: pool space is full
 * -EINVAL: wrong parameter to call function
 */
int check_dm_thin_cond(
	struct block_device *bd
	)
{
	struct request_queue *q = NULL; 

	if (!bd)
		return -EINVAL;

	q = bdev_get_queue(bd);
	if (q)
		return dm_thin_volume_is_full(rq_get_thin_hook(q));

	return -EINVAL;	
}
EXPORT_SYMBOL(check_dm_thin_cond);


#else

/* -EINVAL: always return -EINVAL for non-supported kernel */
int check_dm_thin_cond(
	struct block_device *bd
	)
{
//	pr_warn("%s: not supported\n", __func__);
	return -EINVAL;
}
EXPORT_SYMBOL(check_dm_thin_cond);
#endif


#else  /* !defined(SUPPORT_VOLUME_BASED) */
/* -EINVAL: always return -EINVAL for 
 * 1. unsupported kernel
 * 2. product w/o dm-thin 
 */
int check_dm_thin_cond(
	struct block_device *bd
	)
{
//	pr_warn("%s: not supported\n", __func__);
	return -EINVAL;
}
EXPORT_SYMBOL(check_dm_thin_cond);
#endif


int __do_sync_cache_range(
	struct file *file,
	loff_t start_byte,
	loff_t end_byte	
	)
{
	int err_1, err_msg;

	err_msg = 1;
	err_1 = filemap_fdatawrite_range(
		file->f_mapping, start_byte, end_byte);

	if (unlikely(err_1 != 0))
		goto _ERR_SYNC_CACHE_;

	err_msg = 2;
	err_1 = filemap_fdatawait_range(
		file->f_mapping, start_byte, end_byte);

	if (unlikely(err_1 != 0))
		goto _ERR_SYNC_CACHE_;

	return 0;

_ERR_SYNC_CACHE_:

#if 0
	pr_err("%s: %s is failed: %d\n", __func__, 
		((err_msg == 1) ? "filemap_fdatawrite_range": \
		"filemap_fdatawait_range"), err_1);
#endif
	return err_1;
}
EXPORT_SYMBOL(__do_sync_cache_range);
#endif

int transport_check_sectors_exceeds_max_limits_blks(
	LIO_SE_CMD *se_cmd,
	u32 sectors
	)
{
	int ret = 0, max_transfer_blks;
	
	max_transfer_blks = transport_get_max_transfer_sectors(se_cmd->se_dev);

	/* TODO, sbc3r35j, page 287 
	 * 1. here won't check the WIRTE USING TOKEN command cause of
	 *    max trnasfer size for WRITE USING TOKEN command we reported is 
	 *    larger than max transfer len in block limit vpd
	 */
	switch(se_cmd->t_task_cdb[0]){
	case WRITE_6:
	case WRITE_10:
	case WRITE_12:
	case WRITE_16:
	case READ_6:
	case READ_10:
	case READ_12:
	case READ_16:
	case VERIFY:
	case VERIFY_16:
	case WRITE_VERIFY:
	case WRITE_VERIFY_12:
	case WRITE_VERIFY_16:
	case XDWRITEREAD_10:
		if (sectors > max_transfer_blks){
			pr_err("scsi op: %02xh with too big sectors %u "
				"exceeds the max limits:%d\n",
				se_cmd->t_task_cdb[0], sectors, 
				max_transfer_blks);

			__set_err_reason(ERR_INVALID_CDB_FIELD, 
				&se_cmd->scsi_sense_reason);
			ret = -EINVAL;
		}
		break;
	default:
		break;
	}

	return ret;
}


int transport_get_pool_blk_size_kb()
{
	int size_kb;

/* Bug#63140 Use 1024 KB for ATTO Initiator for Mac */
//#if defined(SUPPORT_FAST_BLOCK_CLONE)
//	size_kb = POOL_BLK_SIZE_512_KB;
//#else
	size_kb = POOL_BLK_SIZE_1024_KB;
//#endif
	return size_kb;
}
int transport_get_pool_blk_sectors(
	LIO_SE_DEVICE *se_dev
	)
{
	int pool_sectors, size_kb, bs_order;
	
	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
	size_kb = transport_get_pool_blk_size_kb();

	return ((size_kb << 10) >> bs_order);
}

int transport_set_pool_blk_sectors(
	LIO_SE_DEVICE *se_dev,
	struct se_dev_limits *dev_limits
	)
{
	int bs_order, pool_blk_sectors, pool_size_kb;

	/* This function will be called in transport_add_device_to_core_hba(),
	 * in the other words, it means we can NOT get the se_sub_dev->se_dev_attrib.block_size
	 * at this stage. The dev_limits->limits.logical_block_size had been setup
	 * for 512b or 4096b, so it is safe to use it
	 */
	bs_order = ilog2(dev_limits->limits.logical_block_size);

	pool_size_kb = transport_get_pool_blk_size_kb();
	se_dev->pool_blk_sectors = (pool_size_kb << 10) >> bs_order;
	return 0;
}


int transport_get_max_hw_transfer_sectors(
	LIO_SE_DEVICE *se_dev
	)
{
	int bs_order;
	
	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
	return ((MAX_TRANSFER_LEN_MB << 20) >> bs_order);
}


int transport_get_max_transfer_sectors(
	LIO_SE_DEVICE *se_dev
	)
{
	int bs_order;
	int pool_blk_size;

	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);
	/* force the max transfer sectors to dm-thin block size */
	pool_blk_size =  transport_get_pool_blk_size_kb();
	return ((pool_blk_size << 10) >> bs_order);
}


int transport_get_opt_transfer_sectors(
	LIO_SE_DEVICE *se_dev
	)
{
	int sectors;

#if defined(CONFIG_MACH_QNAPTS)
	int bs_order;
	int pool_blk_size;

	bs_order = ilog2(se_dev->se_sub_dev->se_dev_attrib.block_size);

	/* force the optimal transfer sectors to dm-thin block size */
	pool_blk_size =  transport_get_pool_blk_size_kb();
	sectors = ((pool_blk_size << 10) >> bs_order);
#else
	sectors = DA_FABRIC_MAX_SECTORS;
#endif
	return sectors;

}



/* 2015/03/10, adamhsu, redmine 12226, bugzilla 58041 
 * record the backend dev supports fast block-clone or not
 */
void transport_setup_support_fbc(
	LIO_SE_DEVICE *se_dev
	)
{
	int ret;
	SUBSYSTEM_TYPE type;
	LIO_IBLOCK_DEV *ib_dev = NULL;
	LIO_FD_DEV *fd_dev = NULL;
	struct file *file = NULL;
	struct block_device *bd = NULL;
	struct inode *inode = NULL;

	se_dev->fast_blk_clone = 0;

#if defined(SUPPORT_FAST_BLOCK_CLONE)

	ret = __do_get_subsystem_dev_type(se_dev, &type);
	if (ret == 1)
		return;

	if (type == SUBSYSTEM_BLOCK) {
		/* here is for block i/o  */
		ib_dev = se_dev->dev_ptr;
		bd =  ib_dev->ibd_bd;

		if (quick_check_support_fbc(bd) == 0){
			se_dev->fast_blk_clone = 1;
			pr_info("IBLOCK: support fast clone\n");
		} else
			pr_info("IBLOCK: not support fast clone\n");

	} else {
		/* here is for file i/o */
		fd_dev = se_dev->dev_ptr;
		file = fd_dev->fd_file;
		inode = file->f_mapping->host;

		if (S_ISBLK(inode->i_mode)) {
			if(quick_check_support_fbc(inode->i_bdev) == 0){
				se_dev->fast_blk_clone = 1;
				pr_info("FD: LIO(File I/O) + Block Backend: "
					"support fast clone\n");
			} else
				pr_info("FD: LIO(File I/O) + Block Backend: "
					"not support fast clone\n");
		} else
			pr_info("FD: LIO(File I/O) + File Backend: "
				"not support fast clone\n");
	}
#endif

	return;

}

#endif  /* #if defined(CONFIG_MACH_QNAPTS) */


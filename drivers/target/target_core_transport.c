#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#define TARGET_CORE_TRANSPORT_C

#include <linux/version.h>
#include <linux/net.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/spinlock.h>
#include <linux/smp_lock.h>
#include <linux/kthread.h>
#include <linux/in.h>
#include <linux/cdrom.h>
#include <asm/unaligned.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/libsas.h>  

#include <target/target_core_base.h>
#include <target/target_core_device.h>
#include <target/target_core_hba.h>
#include <target/target_core_scdb.h>
#include <target/target_core_pr.h>
#include <target/target_core_alua.h>
#include <target/target_core_tmr.h>
#include <target/target_core_tpg.h>
#include <target/target_core_ua.h>
#include <target/target_core_transport.h>
#include <target/target_core_plugin.h>
#include <target/target_core_seobj.h>
#include <target/target_core_transport_plugin.h>
#include <target/target_core_fabric_ops.h>
#include <target/target_core_configfs.h>

#undef TARGET_CORE_TRANSPORT_C

#ifdef DEBUG_CDB_HANDLER
#define DEBUG_CDB_H(x...) printk(KERN_INFO x)
#else
#define DEBUG_CDB_H(x...)
#endif

#ifdef DEBUG_CMD_MAP
#define DEBUG_CMD_M(x...) printk(KERN_INFO x)
#else
#define DEBUG_CMD_M(x...)
#endif

#ifdef DEBUG_MEM_ALLOC
#define DEBUG_MEM(x...) printk(KERN_INFO x)
#else
#define DEBUG_MEM(x...)
#endif

#ifdef DEBUG_MEM2_ALLOC
#define DEBUG_MEM2(x...) printk(KERN_INFO x)
#else
#define DEBUG_MEM2(x...)
#endif

#ifdef DEBUG_SG_CALC
#define DEBUG_SC(x...) printk(KERN_INFO x)
#else
#define DEBUG_SC(x...)
#endif

#ifdef DEBUG_SE_OBJ
#define DEBUG_SO(x...) printk(KERN_INFO x)
#else
#define DEBUG_SO(x...)
#endif

#ifdef DEBUG_CMD_VOL
#define DEBUG_VOL(x...) printk(KERN_INFO x)
#else
#define DEBUG_VOL(x...)
#endif

#ifdef DEBUG_CMD_STOP
#define DEBUG_CS(x...) printk(KERN_INFO x)
#else
#define DEBUG_CS(x...)
#endif

#ifdef DEBUG_PASSTHROUGH
#define DEBUG_PT(x...) printk(KERN_INFO x)
#else
#define DEBUG_PT(x...)
#endif

#ifdef DEBUG_TASK_STOP
#define DEBUG_TS(x...) printk(KERN_INFO x)
#else
#define DEBUG_TS(x...)
#endif

#ifdef DEBUG_TRANSPORT_STOP
#define DEBUG_TRANSPORT_S(x...) printk(KERN_INFO x)
#else
#define DEBUG_TRANSPORT_S(x...)
#endif

#ifdef DEBUG_TASK_FAILURE
#define DEBUG_TF(x...) printk(KERN_INFO x)
#else
#define DEBUG_TF(x...)
#endif

#ifdef DEBUG_DEV_OFFLINE
#define DEBUG_DO(x...) printk(KERN_INFO x)
#else
#define DEBUG_DO(x...)
#endif

#ifdef DEBUG_TASK_STATE
#define DEBUG_TSTATE(x...) printk(KERN_INFO x)
#else
#define DEBUG_TSTATE(x...)
#endif

#ifdef DEBUG_STATUS_THR
#define DEBUG_ST(x...) printk(KERN_INFO x)
#else
#define DEBUG_ST(x...)
#endif

#ifdef DEBUG_TASK_TIMEOUT
#define DEBUG_TT(x...) printk(KERN_INFO x)
#else
#define DEBUG_TT(x...)
#endif

#ifdef DEBUG_GENERIC_REQUEST_FAILURE
#define DEBUG_GRF(x...) printk(KERN_INFO x)
#else
#define DEBUG_GRF(x...)
#endif

#ifdef DEBUG_SAM_TASK_ATTRS
#define DEBUG_STA(x...) printk(KERN_INFO x)
#else
#define DEBUG_STA(x...)
#endif

se_global_t *se_global;
EXPORT_SYMBOL(se_global);

struct kmem_cache *se_cmd_cache;
struct kmem_cache *se_task_cache;
struct kmem_cache *se_tmr_req_cache;
struct kmem_cache *se_sess_cache;
struct kmem_cache *se_hba_cache;
struct kmem_cache *se_ua_cache;
struct kmem_cache *se_mem_cache;
struct kmem_cache *t10_pr_reg_cache;
struct kmem_cache *t10_alua_lu_gp_cache;
struct kmem_cache *t10_alua_lu_gp_mem_cache;
struct kmem_cache *t10_alua_tg_pt_gp_cache;
struct kmem_cache *t10_alua_tg_pt_gp_mem_cache;

static int transport_generic_write_pending(se_cmd_t *);
static int transport_processing_thread(void *);
#ifdef SYNO_LIO_TRANSPORT_PATCHES
static int transport_new_cmd_obj(struct se_cmd_s *cmd,
		struct se_transform_info_s *ti, void *obj_ptr, int post_execute);
#elif defined(MY_ABC_HERE)
int transport_new_cmd_obj(struct se_cmd_s *cmd,
		struct se_transform_info_s *ti, void *obj_ptr, int post_execute);
#endif

static char *transport_passthrough_get_fabric_name(void)
{
	return "PT";
}

static u32 transport_passthrough_get_task_tag(se_cmd_t *cmd)
{
	return 0;
}

static int transport_passthrough_get_cmd_state(se_cmd_t *cmd)
{
	return 0;
}

static void transport_passthrough_release_cmd_direct(se_cmd_t *cmd)
{
	return;
}

static u16 transport_passthrough_set_fabric_sense_len(
	se_cmd_t *cmd,
	u32 sense_len)
{
	return 0;
}

static u16 transport_passthrough_get_fabric_sense_len(void)
{
	return 0;
}

struct target_core_fabric_ops passthrough_fabric_ops = {
	.release_cmd_direct	= transport_passthrough_release_cmd_direct,
	.get_fabric_name	= transport_passthrough_get_fabric_name,
	.get_task_tag		= transport_passthrough_get_task_tag,
	.get_cmd_state		= transport_passthrough_get_cmd_state,
	.set_fabric_sense_len	= transport_passthrough_set_fabric_sense_len,
	.get_fabric_sense_len	= transport_passthrough_get_fabric_sense_len,
};

int init_se_global(void)
{
	se_global_t *global;

	global = kzalloc(sizeof(se_global_t), GFP_KERNEL);
	if (!(global)) {
		printk(KERN_ERR "Unable to allocate memory for se_global_t\n");
		return -1;
	}

	INIT_LIST_HEAD(&global->g_lu_gps_list);
	INIT_LIST_HEAD(&global->g_se_tpg_list);
	INIT_LIST_HEAD(&global->g_hba_list);
	INIT_LIST_HEAD(&global->g_se_dev_list);
	spin_lock_init(&global->g_device_lock);
	spin_lock_init(&global->hba_lock);
	spin_lock_init(&global->se_tpg_lock);
	spin_lock_init(&global->lu_gps_lock);
	spin_lock_init(&global->plugin_class_lock);

	se_cmd_cache = kmem_cache_create("se_cmd_cache",
			sizeof(se_cmd_t), __alignof__(se_cmd_t), 0, NULL);
	if (!(se_cmd_cache)) {
		printk(KERN_ERR "kmem_cache_create for se_cmd_t failed\n");
		goto out;
	}
	se_task_cache = kmem_cache_create("se_task_cache",
			sizeof(se_task_t), __alignof__(se_task_t), 0, NULL);
	if (!(se_task_cache)) {
		printk(KERN_ERR "kmem_cache_create for se_task_t failed\n");
		goto out;
	}
	se_tmr_req_cache = kmem_cache_create("se_tmr_cache",
			sizeof(se_tmr_req_t), __alignof__(se_tmr_req_t),
			0, NULL);
	if (!(se_tmr_req_cache)) {
		printk(KERN_ERR "kmem_cache_create() for se_tmr_req_t"
				" failed\n");
		goto out;
	}
	se_sess_cache = kmem_cache_create("se_sess_cache",
			sizeof(se_session_t), __alignof__(se_session_t),
			0, NULL);
	if (!(se_sess_cache)) {
		printk(KERN_ERR "kmem_cache_create() for se_session_t"
				" failed\n");
		goto out;
	}
	se_hba_cache = kmem_cache_create("se_hba_cache",
			sizeof(se_hba_t), __alignof__(se_hba_t),
			0, NULL);
	if (!(se_hba_cache)) {
		printk(KERN_ERR "kmem_cache_create() for se_hba_t"
				" failed\n");
		goto out;
	}
	se_ua_cache = kmem_cache_create("se_ua_cache",
			sizeof(se_ua_t), __alignof__(se_ua_t), 0, NULL);
	if (!(se_ua_cache)) {
		printk(KERN_ERR "kmem_cache_create() for se_ua_t failed\n");
		goto out;
	}
	se_mem_cache = kmem_cache_create("se_mem_cache",
			sizeof(se_mem_t), __alignof__(se_mem_t), 0, NULL);
	if (!(se_mem_cache)) {
		printk(KERN_ERR "kmem_cache_create() for se_mem_t failed\n");
		goto out;
	}
	t10_pr_reg_cache = kmem_cache_create("t10_pr_reg_cache",
			sizeof(t10_pr_registration_t),
			__alignof__(t10_pr_registration_t), 0, NULL);
	if (!(t10_pr_reg_cache)) {
		printk(KERN_ERR "kmem_cache_create() for t10_pr_registration_t"
				" failed\n");
		goto out;
	}
	t10_alua_lu_gp_cache = kmem_cache_create("t10_alua_lu_gp_cache",
			sizeof(t10_alua_lu_gp_t), __alignof__(t10_alua_lu_gp_t),
			0, NULL);
	if (!(t10_alua_lu_gp_cache)) {
		printk(KERN_ERR "kmem_cache_create() for t10_alua_lu_gp_cache"
				" failed\n");
		goto out;
	}
	t10_alua_lu_gp_mem_cache = kmem_cache_create("t10_alua_lu_gp_mem_cache",
			sizeof(t10_alua_lu_gp_member_t),
			__alignof__(t10_alua_lu_gp_member_t), 0, NULL);
	if (!(t10_alua_lu_gp_mem_cache)) {
		printk(KERN_ERR "kmem_cache_create() for t10_alua_lu_gp_mem_"
				"cache failed\n");
		goto out;
	}
	t10_alua_tg_pt_gp_cache = kmem_cache_create("t10_alua_tg_pt_gp_cache",
			sizeof(t10_alua_tg_pt_gp_t),
			__alignof__(t10_alua_tg_pt_gp_t), 0, NULL);
	if (!(t10_alua_tg_pt_gp_cache)) {
		printk(KERN_ERR "kmem_cache_create() for t10_alua_tg_pt_gp_"
				"cache failed\n");
		goto out;
	}
	t10_alua_tg_pt_gp_mem_cache = kmem_cache_create(
			"t10_alua_tg_pt_gp_mem_cache",
			sizeof(t10_alua_tg_pt_gp_member_t),
			__alignof__(t10_alua_tg_pt_gp_member_t),
			0, NULL);
	if (!(t10_alua_tg_pt_gp_mem_cache)) {
		printk(KERN_ERR "kmem_cache_create() for t10_alua_tg_pt_gp_"
				"mem_t failed\n");
		goto out;
	}

	global->plugin_class_list = kzalloc((sizeof(se_plugin_class_t) *
				MAX_PLUGIN_CLASSES), GFP_KERNEL);
	if (!(global->plugin_class_list)) {
		printk(KERN_ERR "Unable to allocate global->"
			"plugin_class_list\n");
		goto out;
	}

	se_global = global;

	return 0;
out:
	kfree(global->plugin_class_list);
	if (se_cmd_cache)
		kmem_cache_destroy(se_cmd_cache);
	if (se_task_cache)
		kmem_cache_destroy(se_task_cache);
	if (se_tmr_req_cache)
		kmem_cache_destroy(se_tmr_req_cache);
	if (se_sess_cache)
		kmem_cache_destroy(se_sess_cache);
	if (se_hba_cache)
		kmem_cache_destroy(se_hba_cache);
	if (se_ua_cache)
		kmem_cache_destroy(se_ua_cache);
	if (se_mem_cache)
		kmem_cache_destroy(se_mem_cache);
	if (t10_pr_reg_cache)
		kmem_cache_destroy(t10_pr_reg_cache);
	if (t10_alua_lu_gp_cache)
		kmem_cache_destroy(t10_alua_lu_gp_cache);
	if (t10_alua_lu_gp_mem_cache)
		kmem_cache_destroy(t10_alua_lu_gp_mem_cache);
	if (t10_alua_tg_pt_gp_cache)
		kmem_cache_destroy(t10_alua_tg_pt_gp_cache);
	if (t10_alua_tg_pt_gp_mem_cache)
		kmem_cache_destroy(t10_alua_tg_pt_gp_mem_cache);
	kfree(global);
	return -1;
}

void release_se_global(void)
{
	se_global_t *global;

	global = se_global;
	if (!(global))
		return;

	kfree(global->plugin_class_list);
	kmem_cache_destroy(se_cmd_cache);
	kmem_cache_destroy(se_task_cache);
	kmem_cache_destroy(se_tmr_req_cache);
	kmem_cache_destroy(se_sess_cache);
	kmem_cache_destroy(se_hba_cache);
	kmem_cache_destroy(se_ua_cache);
	kmem_cache_destroy(se_mem_cache);
	kmem_cache_destroy(t10_pr_reg_cache);
	kmem_cache_destroy(t10_alua_lu_gp_cache);
	kmem_cache_destroy(t10_alua_lu_gp_mem_cache);
	kmem_cache_destroy(t10_alua_tg_pt_gp_cache);
	kmem_cache_destroy(t10_alua_tg_pt_gp_mem_cache);
	kfree(global);

	se_global = NULL;
}

#ifdef DEBUG_DEV

int __iscsi_debug_dev(se_device_t *dev)
{
	int fail_task = 0;
	fd_dev_t *fd_dev;
	iblock_dev_t *ib_dev;
	rd_dev_t *rd_dev;
	struct scsi_device *sd;

	spin_lock(&se_global->debug_dev_lock);
	switch (dev->se_hba->type) {
	case PSCSI:
		sd = (struct scsi_device *) dev->dev_ptr;
		if (dev->dev_flags & DF_DEV_DEBUG) {
			printk(KERN_INFO "HBA[%u] - Failing PSCSI Task for"
				" %d/%d/%d\n", dev->se_hba->hba_id,
				sd->channel, sd->id, sd->lun);
			fail_task = 1;
		}
		break;
	case IBLOCK:
		ib_dev = (iblock_dev_t *) dev->dev_ptr;
		if (dev->dev_flags & DF_DEV_DEBUG) {
			printk(KERN_INFO "HBA[%u] - Failing IBLOCK Task for"
				" %u/%u\n", dev->se_hba->hba_id,
				ib_dev->ibd_major, ib_dev->ibd_minor);
			fail_task = 1;
		}
		break;
	case FILEIO:
		fd_dev = (fd_dev_t *) dev->dev_ptr;
		if (dev->dev_flags & DF_DEV_DEBUG) {
			printk(KERN_INFO "HBA[%u] - Failing FILEIO Task for"
				" %u\n", dev->se_hba->hba_id,
				fd_dev->fd_dev_id);
			fail_task = 1;
		}
		break;
	case RAMDISK_DR:
	case RAMDISK_MCP:
		rd_dev = (rd_dev_t *) dev->dev_ptr;
		if (dev->dev_flags & DF_DEV_DEBUG) {
			printk(KERN_INFO "HBA[%u] - Failing RAMDISK Task for"
				" %u\n", dev->se_hba->hba_id,
				rd_dev->rd_dev_id);
			fail_task = 1;
		}
		break;
	default:
		if (dev->dev_flags & DF_DEV_DEBUG) {
			printk(KERN_INFO "HBA[%u] - Failing unknown Task\n",
				dev->se_hba->hba_id);
			fail_task = 1;
		}
		break;
	}
	spin_unlock(&se_global->debug_dev_lock);

	return fail_task;
}

#endif  

unsigned char *transport_get_iqn_sn(void)
{
	 
	return "1234567890";
}

void transport_init_queue_obj(se_queue_obj_t *qobj)
{
	atomic_set(&qobj->queue_cnt, 0);
	INIT_LIST_HEAD(&qobj->qobj_list);
	init_waitqueue_head(&qobj->thread_wq);
	init_completion(&qobj->thread_create_comp);
	init_completion(&qobj->thread_done_comp);
	spin_lock_init(&qobj->cmd_queue_lock);
}
EXPORT_SYMBOL(transport_init_queue_obj);

void transport_load_plugins(void)
{
	int ret = 0;

#ifdef PARALLEL_SCSI
	plugin_register((void *)&pscsi_template, pscsi_template.type,
			pscsi_template.name, PLUGIN_TYPE_TRANSPORT,
			pscsi_template.get_plugin_info, NULL, NULL, &ret);
#endif
#ifdef STGT_PLUGIN
	plugin_register((void *)&stgt_template, stgt_template.type,
			stgt_template.name, PLUGIN_TYPE_TRANSPORT,
			stgt_template.get_plugin_info,
			stgt_template.plugin_init,
			stgt_template.plugin_free, &ret);
#endif
#ifdef PYX_IBLOCK
	plugin_register((void *)&iblock_template, iblock_template.type,
			iblock_template.name, PLUGIN_TYPE_TRANSPORT,
			iblock_template.get_plugin_info, NULL, NULL, &ret);
#endif
#ifdef PYX_RAMDISK
	plugin_register((void *)&rd_dr_template, rd_dr_template.type,
			rd_dr_template.name, PLUGIN_TYPE_TRANSPORT,
			rd_dr_template.get_plugin_info, NULL, NULL, &ret);
	plugin_register((void *)&rd_mcp_template, rd_mcp_template.type,
			rd_mcp_template.name, PLUGIN_TYPE_TRANSPORT,
			rd_mcp_template.get_plugin_info, NULL, NULL, &ret);
#endif
#ifdef PYX_FILEIO
	plugin_register((void *)&fileio_template, fileio_template.type,
			fileio_template.name, PLUGIN_TYPE_TRANSPORT,
			fileio_template.get_plugin_info, NULL, NULL, &ret);
#endif
}

se_plugin_t *transport_core_get_plugin_by_name(const char *name)
{
	se_plugin_class_t *pc;
	se_plugin_t *p;
	int i;

	pc = plugin_get_class(PLUGIN_TYPE_TRANSPORT);
	if (!(pc))
		return NULL;

	for (i = 0; i < MAX_PLUGINS; i++) {
		p = &pc->plugin_array[i];

		if (!p->plugin_obj)
			continue;

		if (!(strncmp(name, p->plugin_name, strlen(p->plugin_name))))
			return p;
	}

	return NULL;
}

void transport_check_dev_params_delim(char *ptr, char **cur)
{
	char *ptr2;

	if (ptr) {
		ptr2 = strstr(ptr, ",");
		if ((ptr2)) {
			*ptr2 = '\0';
			*cur = (ptr2 + 1);  
		} else
			*cur = NULL;
	}
}

se_session_t *transport_init_session(void)
{
	se_session_t *se_sess;

	se_sess = kmem_cache_zalloc(se_sess_cache, GFP_KERNEL);
	if (!(se_sess)) {
		printk(KERN_ERR "Unable to allocate se_session_t from"
				" se_sess_cache\n");
		return ERR_PTR(-ENOMEM);
	}
	INIT_LIST_HEAD(&se_sess->sess_list);
	INIT_LIST_HEAD(&se_sess->sess_acl_list);

	return se_sess;
}
EXPORT_SYMBOL(transport_init_session);

void __transport_register_session(
	se_portal_group_t *se_tpg,
	se_node_acl_t *se_nacl,
	se_session_t *se_sess,
	void *fabric_sess_ptr)
{
	unsigned char buf[PR_REG_ISID_LEN];

	se_sess->se_tpg = se_tpg;
	se_sess->fabric_sess_ptr = fabric_sess_ptr;
	 
	if (se_nacl) {
		 
		if (TPG_TFO(se_tpg)->sess_get_initiator_sid != NULL) {
			memset(&buf[0], 0, PR_REG_ISID_LEN);
			TPG_TFO(se_tpg)->sess_get_initiator_sid(se_sess,
					&buf[0], PR_REG_ISID_LEN);
			se_sess->sess_bin_isid = get_unaligned_be64(&buf[0]);
		}
		spin_lock_bh(&se_nacl->nacl_sess_lock);
		 
		se_nacl->nacl_sess = se_sess;

		list_add_tail(&se_sess->sess_acl_list,
			      &se_nacl->acl_sess_list);
		spin_unlock_bh(&se_nacl->nacl_sess_lock);
	}
	list_add_tail(&se_sess->sess_list, &se_tpg->tpg_sess_list);

#ifndef MY_ABC_HERE
	printk(KERN_INFO "TARGET_CORE[%s]: Registered fabric_sess_ptr: %p\n",
		TPG_TFO(se_tpg)->get_fabric_name(), se_sess->fabric_sess_ptr);
#endif
}
EXPORT_SYMBOL(__transport_register_session);

void transport_register_session(
	se_portal_group_t *se_tpg,
	se_node_acl_t *se_nacl,
	se_session_t *se_sess,
	void *fabric_sess_ptr)
{
	spin_lock_bh(&se_tpg->session_lock);
	__transport_register_session(se_tpg, se_nacl, se_sess, fabric_sess_ptr);
	spin_unlock_bh(&se_tpg->session_lock);
}
EXPORT_SYMBOL(transport_register_session);

void transport_deregister_session_configfs(se_session_t *se_sess)
{
	se_node_acl_t *se_nacl;

	se_nacl = se_sess->se_node_acl;
	if ((se_nacl)) {
		spin_lock_bh(&se_nacl->nacl_sess_lock);
		list_del(&se_sess->sess_acl_list);
		 
		if (list_empty(&se_nacl->acl_sess_list))
			se_nacl->nacl_sess = NULL;
		else {
			se_nacl->nacl_sess = container_of(
					se_nacl->acl_sess_list.prev,
					se_session_t, sess_acl_list);	
		}
		spin_unlock_bh(&se_nacl->nacl_sess_lock);
	}
}
EXPORT_SYMBOL(transport_deregister_session_configfs);

void transport_free_session(se_session_t *se_sess)
{
	kmem_cache_free(se_sess_cache, se_sess);
}
EXPORT_SYMBOL(transport_free_session);

void transport_deregister_session(se_session_t *se_sess)
{
	se_portal_group_t *se_tpg = se_sess->se_tpg;
	se_node_acl_t *se_nacl;

	if (!(se_tpg)) {
		transport_free_session(se_sess);
		return;
	}

	spin_lock_bh(&se_tpg->session_lock);
	list_del(&se_sess->sess_list);
	se_sess->se_tpg = NULL;
	se_sess->fabric_sess_ptr = NULL;
	spin_unlock_bh(&se_tpg->session_lock);

	se_nacl = se_sess->se_node_acl;
	if ((se_nacl)) {
		spin_lock_bh(&se_tpg->acl_node_lock);
		if (se_nacl->nodeacl_flags & NAF_DYNAMIC_NODE_ACL) {
			if (!(TPG_TFO(se_tpg)->tpg_check_demo_mode_cache(
					se_tpg))) {
				list_del(&se_nacl->acl_list);
				se_tpg->num_node_acls--;
				spin_unlock_bh(&se_tpg->acl_node_lock);

				core_tpg_wait_for_nacl_pr_ref(se_nacl);
				core_free_device_list_for_node(se_nacl, se_tpg);
				TPG_TFO(se_tpg)->tpg_release_fabric_acl(se_tpg,
						se_nacl);
				kfree(se_nacl);
				spin_lock_bh(&se_tpg->acl_node_lock);
			}
		}
		spin_unlock_bh(&se_tpg->acl_node_lock);
	}

	transport_free_session(se_sess);

#ifndef MY_ABC_HERE
	printk(KERN_INFO "TARGET_CORE[%s]: Deregistered fabric_sess\n",
		TPG_TFO(se_tpg)->get_fabric_name());
#endif
}
EXPORT_SYMBOL(transport_deregister_session);

static void transport_all_task_dev_remove_state(se_cmd_t *cmd)
{
	se_device_t *dev;
	se_task_t *task;
	unsigned long flags;

	if (!T_TASK(cmd))
		return;

	list_for_each_entry(task, &T_TASK(cmd)->t_task_list, t_list) {
		dev = task->se_dev;
		if (!(dev))
			continue;

		if (atomic_read(&task->task_active))
			continue;

		if (!(atomic_read(&task->task_state_active)))
			continue;

		spin_lock_irqsave(&dev->execute_task_lock, flags);
		list_del(&task->t_state_list);
		DEBUG_TSTATE("Removed ITT: 0x%08x dev: %p task[%p]\n",
			CMD_TFO(cmd)->tfo_get_task_tag(cmd), dev, task);
		spin_unlock_irqrestore(&dev->execute_task_lock, flags);

		atomic_set(&task->task_state_active, 0);
		atomic_dec(&T_TASK(cmd)->t_task_cdbs_ex_left);
	}
}

void transport_task_dev_remove_state(se_task_t *task, se_device_t *dev)
{
	se_cmd_t *cmd = task->task_se_cmd;
	unsigned long flags;

	if (atomic_read(&task->task_active)) {
#if 0
		printk(KERN_ERR "Skipping Removal of state for ITT: 0x%08x"
			" dev: %p task[%p]\n"
			CMD_TFO(task->task_se_cmd)->tfo_get_task_tag(
			task->task_se_cmd), dev, task);
#endif
		return;
	}

	if (atomic_read(&task->task_state_active)) {
		spin_lock_irqsave(&dev->execute_task_lock, flags);
		list_del(&task->t_state_list);
		DEBUG_TSTATE("Removed ITT: 0x%08x dev: %p task[%p]\n",
			CMD_TFO(task->task_se_cmd)->tfo_get_task_tag(
			task->task_se_cmd), dev, task);
		spin_unlock_irqrestore(&dev->execute_task_lock, flags);

		atomic_set(&task->task_state_active, 0);
		atomic_dec(&T_TASK(cmd)->t_task_cdbs_ex_left);
	}
}

static void transport_passthrough_check_stop(se_cmd_t *cmd)
{
	if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH))
		return;

	if (!cmd->transport_passthrough_done) {
		if (cmd->callback) {
			cmd->callback(cmd, cmd->callback_arg,
				transport_passthrough_complete(cmd));
		} else
			complete(&T_TASK(cmd)->t_transport_passthrough_comp);

		return;
	}

	cmd->transport_passthrough_done(cmd);
}

static int transport_cmd_check_stop(
	se_cmd_t *cmd,
	int transport_off,
	u8 t_state)
{
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	 
	if (atomic_read(&T_TASK(cmd)->transport_lun_stop)) {
		DEBUG_CS("%s:%d atomic_read(&T_TASK(cmd)->transport_lun_stop)"
			" == TRUE for ITT: 0x%08x\n", __func__, __LINE__,
			CMD_TFO(cmd)->get_task_tag(cmd));

		cmd->deferred_t_state = cmd->t_state;
		cmd->t_state = TRANSPORT_DEFERRED_CMD;
		atomic_set(&T_TASK(cmd)->t_transport_active, 0);
		if (transport_off == 2)
			transport_all_task_dev_remove_state(cmd);
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

		complete(&T_TASK(cmd)->transport_lun_stop_comp);
		return 1;
	}
	 
	if (atomic_read(&T_TASK(cmd)->t_transport_stop)) {
		DEBUG_CS("%s:%d atomic_read(&T_TASK(cmd)->t_transport_stop) =="
			" TRUE for ITT: 0x%08x\n", __func__, __LINE__,
			CMD_TFO(cmd)->get_task_tag(cmd));

		cmd->deferred_t_state = cmd->t_state;
		cmd->t_state = TRANSPORT_DEFERRED_CMD;
		if (transport_off == 2)
			transport_all_task_dev_remove_state(cmd);

		if ((transport_off == 2) && !(cmd->se_cmd_flags &
						SCF_CMD_PASSTHROUGH))
			cmd->se_lun = NULL;
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

		complete(&T_TASK(cmd)->t_transport_stop_comp);
		return 1;
	}
	if (transport_off) {
		atomic_set(&T_TASK(cmd)->t_transport_active, 0);
		if (transport_off == 2) {
			transport_all_task_dev_remove_state(cmd);
			 
			if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH))
				cmd->se_lun = NULL;
			 
			if (CMD_TFO(cmd)->check_stop_free != NULL) {
				spin_unlock_irqrestore(
					&T_TASK(cmd)->t_state_lock, flags);

				CMD_TFO(cmd)->check_stop_free(cmd);
				return 1;
			}
		}
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

		return 0;
	} else if (t_state)
		cmd->t_state = t_state;
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	return 0;
}

static int transport_cmd_check_stop_to_fabric(se_cmd_t *cmd)
{
	return transport_cmd_check_stop(cmd, 2, 0);
}

static void transport_lun_remove_cmd(se_cmd_t *cmd)
{
	se_lun_t *lun = SE_LUN(cmd);
	unsigned long flags;

	if (!lun)
		return;
	 
	if (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH)
		return;

#ifdef MY_ABC_HERE
	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (!(atomic_read(&T_TASK(cmd)->transport_dev_active))) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		goto check_lun;
	}
	atomic_set(&T_TASK(cmd)->transport_dev_active, 0);
	transport_all_task_dev_remove_state(cmd);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	transport_free_dev_tasks(cmd);

check_lun:
#endif
	spin_lock_irqsave(&lun->lun_cmd_lock, flags);
	if (atomic_read(&T_TASK(cmd)->transport_lun_active)) {
		list_del(&cmd->se_lun_list);
		atomic_set(&T_TASK(cmd)->transport_lun_active, 0);
#if 0
		printk(KERN_INFO "Removed ITT: 0x%08x from LUN LIST[%d]\n"
			CMD_TFO(cmd)->get_task_tag(cmd), lun->unpacked_lun);
#endif
	}
	spin_unlock_irqrestore(&lun->lun_cmd_lock, flags);
}

void transport_cmd_finish_abort(se_cmd_t *cmd, int remove)
{
#ifdef MY_ABC_HERE
	transport_remove_cmd_from_queue(cmd, SE_DEV(cmd)->dev_queue_obj);
#else
	transport_remove_cmd_from_queue(cmd,
		CMD_ORIG_OBJ_API(cmd)->get_queue_obj(
			cmd->se_orig_obj_ptr));
#endif

	transport_lun_remove_cmd(cmd);

	if (!(transport_cmd_check_stop(cmd, 1, 0))) {
		transport_passthrough_check_stop(cmd);
		return;
	}
	if (remove)
		transport_generic_remove(cmd, 0, 0);
}

void transport_cmd_finish_abort_tmr(se_cmd_t *cmd)
{
#ifdef MY_ABC_HERE
	transport_remove_cmd_from_queue(cmd, SE_DEV(cmd)->dev_queue_obj);
#else
	transport_remove_cmd_from_queue(cmd,
			CMD_ORIG_OBJ_API(cmd)->get_queue_obj(
			cmd->se_orig_obj_ptr));
#endif

	if (!(transport_cmd_check_stop(cmd, 1, 0))) {
		transport_passthrough_check_stop(cmd);
		return;
	}
	transport_generic_remove(cmd, 0, 0);
}

int transport_add_cmd_to_queue(
	se_cmd_t *cmd,
#ifndef SYNO_LIO_TRANSPORT_PATCHES
	se_queue_obj_t *qobj,
#endif
	u8 t_state)
{
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	struct se_device_s *dev = cmd->se_dev;
	struct se_queue_obj_s *qobj = dev->dev_queue_obj;
#endif
	se_queue_req_t *qr;
	unsigned long flags;

	qr = kzalloc(sizeof(se_queue_req_t), GFP_ATOMIC);
	if (!(qr)) {
		printk(KERN_ERR "Unable to allocate memory for"
				" se_queue_req_t\n");
		return -1;
	}
	INIT_LIST_HEAD(&qr->qr_list);

	qr->cmd = (void *)cmd;
	qr->state = t_state;

	if (t_state) {
		spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
		cmd->t_state = t_state;
		atomic_set(&T_TASK(cmd)->t_transport_active, 1);
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
	}

	spin_lock_irqsave(&qobj->cmd_queue_lock, flags);
	list_add_tail(&qr->qr_list, &qobj->qobj_list);
	atomic_inc(&T_TASK(cmd)->t_transport_queue_active);
	spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);

	atomic_inc(&qobj->queue_cnt);
	wake_up_interruptible(&qobj->thread_wq);
	return 0;
}

#ifndef SYNO_LIO_TRANSPORT_PATCHES
static int transport_add_cmd_to_dev_queue(se_cmd_t *cmd, u8 t_state)
{
	se_device_t *dev = cmd->se_dev;

	return transport_add_cmd_to_queue(cmd, dev->dev_queue_obj, t_state);
}
#endif

se_queue_req_t *__transport_get_qr_from_queue(se_queue_obj_t *qobj)
{
	se_cmd_t *cmd;
	se_queue_req_t *qr = NULL;

	if (list_empty(&qobj->qobj_list))
		return NULL;

	list_for_each_entry(qr, &qobj->qobj_list, qr_list)
		break;

	if (qr->cmd) {
		cmd = (se_cmd_t *)qr->cmd;
		atomic_dec(&T_TASK(cmd)->t_transport_queue_active);
	}
	list_del(&qr->qr_list);
	atomic_dec(&qobj->queue_cnt);

	return qr;
}

se_queue_req_t *transport_get_qr_from_queue(se_queue_obj_t *qobj)
{
	se_cmd_t *cmd;
	se_queue_req_t *qr;
	unsigned long flags;

	spin_lock_irqsave(&qobj->cmd_queue_lock, flags);
	if (list_empty(&qobj->qobj_list)) {
		spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);
		return NULL;
	}

	list_for_each_entry(qr, &qobj->qobj_list, qr_list)
		break;

	if (qr->cmd) {
		cmd = (se_cmd_t *)qr->cmd;
		atomic_dec(&T_TASK(cmd)->t_transport_queue_active);
	}
	list_del(&qr->qr_list);
	atomic_dec(&qobj->queue_cnt);
	spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);

	return qr;
}

void transport_remove_cmd_from_queue(se_cmd_t *cmd, se_queue_obj_t *qobj)
{
	se_cmd_t *q_cmd;
	se_queue_req_t *qr = NULL, *qr_p = NULL;
	unsigned long flags;

	spin_lock_irqsave(&qobj->cmd_queue_lock, flags);
	if (!(atomic_read(&T_TASK(cmd)->t_transport_queue_active))) {
		spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);
		return;
	}

	list_for_each_entry_safe(qr, qr_p, &qobj->qobj_list, qr_list) {
		q_cmd = (se_cmd_t *)qr->cmd;
		if (q_cmd != cmd)
			continue;

		atomic_dec(&T_TASK(q_cmd)->t_transport_queue_active);
		atomic_dec(&qobj->queue_cnt);
		list_del(&qr->qr_list);
		kfree(qr);
	}
	spin_unlock_irqrestore(&qobj->cmd_queue_lock, flags);

	if (atomic_read(&T_TASK(cmd)->t_transport_queue_active)) {
		printk(KERN_ERR "ITT: 0x%08x t_transport_queue_active: %d\n",
			CMD_TFO(cmd)->get_task_tag(cmd),
			atomic_read(&T_TASK(cmd)->t_transport_queue_active));
	}
}

void transport_complete_cmd(se_cmd_t *cmd, int success)
{
	int t_state;
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (!success) {
		cmd->transport_error_status = PYX_TRANSPORT_LU_COMM_FAILURE;
		t_state = TRANSPORT_COMPLETE_FAILURE;
	} else {
		t_state = TRANSPORT_COMPLETE_OK;
	}
	atomic_set(&T_TASK(cmd)->t_transport_complete, 1);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	transport_add_cmd_to_queue(cmd, t_state);
#else
	cmd->transport_add_cmd_to_queue(cmd, t_state);
#endif
}

void transport_complete_task(se_task_t *task, int success)
{
	se_cmd_t *cmd = TASK_CMD(task);
	se_device_t *dev = task->se_dev;
	int t_state;
	unsigned long flags;
#if 0
	printk(KERN_INFO "task: %p CDB: 0x%02x obj_ptr: %p\n", task,
			T_TASK(cmd)->t_task_cdb[0], dev);
#endif
	if (dev) {
		spin_lock_irqsave(&SE_HBA(dev)->hba_queue_lock, flags);
		atomic_inc(&dev->depth_left);
		atomic_inc(&SE_HBA(dev)->left_queue_depth);
		spin_unlock_irqrestore(&SE_HBA(dev)->hba_queue_lock, flags);
	}

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	atomic_set(&task->task_active, 0);

	if (!dev)
		goto check_task_stop;

	if (TRANSPORT(dev)->transport_complete(task) != 0) {
		cmd->se_cmd_flags |= SCF_TRANSPORT_TASK_SENSE;
		task->task_sense = 1;
		success = 1;
	}

check_task_stop:
	if (atomic_read(&task->task_stop)) {
		 
		if (atomic_read(&task->task_timeout)) {
			atomic_dec(&T_TASK(cmd)->t_se_count);
			atomic_set(&task->task_timeout, 0);
		}
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

		complete(&task->task_stop_comp);
		return;
	}
	 
	if (atomic_read(&task->task_timeout)) {
		if (!(atomic_dec_and_test(
				&T_TASK(cmd)->t_task_cdbs_timeout_left))) {
			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock,
				flags);
			return;
		}
		t_state = TRANSPORT_COMPLETE_TIMEOUT;
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

#ifdef SYNO_LIO_TRANSPORT_PATCHES
		transport_add_cmd_to_queue(cmd, t_state);
#else
		cmd->transport_add_cmd_to_queue(cmd, t_state);
#endif
		return;
	}
	atomic_dec(&T_TASK(cmd)->t_task_cdbs_timeout_left);

#ifdef DEBUG_DEV
	if (dev) {
		if (__iscsi_debug_dev(dev) != 0) {
			success = 0;
			task->task_scsi_status = 1;
			cmd->transport_error_status =
				PYX_TRANSPORT_LU_COMM_FAILURE;
		}
	}
#endif  

	if (!(atomic_dec_and_test(&T_TASK(cmd)->t_task_cdbs_left))) {
		if (!success)
			T_TASK(cmd)->t_tasks_failed = 1;

		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return;
	}

	if (!success || T_TASK(cmd)->t_tasks_failed) {
		t_state = TRANSPORT_COMPLETE_FAILURE;
		if (!task->task_error_status) {
			task->task_error_status =
				PYX_TRANSPORT_UNKNOWN_SAM_OPCODE;
			cmd->transport_error_status =
				PYX_TRANSPORT_UNKNOWN_SAM_OPCODE;
		}
	} else {
		atomic_set(&T_TASK(cmd)->t_transport_complete, 1);
		t_state = TRANSPORT_COMPLETE_OK;
	}
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	transport_add_cmd_to_queue(cmd, t_state);
#else
	cmd->transport_add_cmd_to_queue(cmd, t_state);
#endif
}

static inline int transport_add_task_check_sam_attr(
	se_task_t *task,
	se_task_t *task_prev,
	se_device_t *dev)
{
	 
	if (dev->dev_task_attr_type != SAM_TASK_ATTR_EMULATED) {
		list_add_tail(&task->t_execute_list, &dev->execute_task_list);
		return 0;
	}
	 
	if (task->task_se_cmd->sam_task_attr == TASK_ATTR_HOQ) {
		list_add(&task->t_execute_list,
				(task_prev != NULL) ?
				&task_prev->t_execute_list :
				&dev->execute_task_list);

		DEBUG_STA("Set HEAD_OF_QUEUE for task CDB: 0x%02x"
				" in execution queue\n",
				T_TASK(task->task_se_cmd)->t_task_cdb[0]);
		return 1;
	}
	 
	list_add_tail(&task->t_execute_list, &dev->execute_task_list);
	return 0;
}

static void __transport_add_task_to_execute_queue(
	se_task_t *task,
	se_task_t *task_prev,
	se_device_t *dev)
{
	int head_of_queue;

	head_of_queue = transport_add_task_check_sam_attr(task, task_prev, dev);
	atomic_inc(&dev->execute_tasks);

	if (atomic_read(&task->task_state_active))
		return;
	 
	if (head_of_queue)
		list_add(&task->t_state_list, (task_prev) ?
				&task_prev->t_state_list :
				&dev->state_task_list);
	else
		list_add_tail(&task->t_state_list, &dev->state_task_list);

	atomic_set(&task->task_state_active, 1);

	DEBUG_TSTATE("Added ITT: 0x%08x task[%p] to dev: %p\n",
		CMD_TFO(task->task_se_cmd)->get_task_tag(task->task_se_cmd),
		task, dev);
}

static void transport_add_tasks_to_state_queue(se_cmd_t *cmd)
{
	se_device_t *dev;
	se_task_t *task;
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	list_for_each_entry(task, &T_TASK(cmd)->t_task_list, t_list) {
		dev = task->se_dev;

		if (atomic_read(&task->task_state_active))
			continue;

		spin_lock(&dev->execute_task_lock);
		list_add_tail(&task->t_state_list, &dev->state_task_list);
		atomic_set(&task->task_state_active, 1);

		DEBUG_TSTATE("Added ITT: 0x%08x task[%p] to dev: %p\n",
			CMD_TFO(task->task_se_cmd)->get_task_tag(
			task->task_se_cmd), task, dev);

		spin_unlock(&dev->execute_task_lock);
	}
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
}

void transport_add_tasks_from_cmd(se_cmd_t *cmd)
{
	se_device_t *dev = SE_DEV(cmd);
	se_task_t *task, *task_prev = NULL;
	unsigned long flags;

	spin_lock_irqsave(&dev->execute_task_lock, flags);
	list_for_each_entry(task, &T_TASK(cmd)->t_task_list, t_list) {
		if (atomic_read(&task->task_execute_queue))
			continue;
		 
		__transport_add_task_to_execute_queue(task, task_prev, dev);
		atomic_set(&task->task_execute_queue, 1);
		task_prev = task;
	}
	spin_unlock_irqrestore(&dev->execute_task_lock, flags);

	return;
}

#ifdef SYNO_LIO_TRANSPORT_PATCHES
static
#endif
se_task_t *transport_get_task_from_execute_queue(se_device_t *dev)
{
	se_task_t *task;

	if (list_empty(&dev->execute_task_list))
		return NULL;

	list_for_each_entry(task, &dev->execute_task_list, t_execute_list)
		break;

	list_del(&task->t_execute_list);
	atomic_dec(&dev->execute_tasks);

	return task;
}

static void transport_remove_task_from_execute_queue(
	se_task_t *task,
	se_device_t *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->execute_task_lock, flags);
	list_del(&task->t_execute_list);
	atomic_dec(&dev->execute_tasks);
	spin_unlock_irqrestore(&dev->execute_task_lock, flags);
}

int transport_check_device_tcq(
	se_device_t *dev,
	u32 unpacked_lun,
	u32 device_tcq)
{
	if (device_tcq > dev->queue_depth) {
		printk(KERN_ERR "Attempting to set storage device queue depth"
			" to %d while transport maximum is %d on LUN: %u,"
			" ignoring request\n", device_tcq, dev->queue_depth,
			unpacked_lun);
		return -1;
	} else if (!device_tcq) {
		printk(KERN_ERR "Attempting to set storage device queue depth"
			" to 0 on LUN: %u, ignoring request\n", unpacked_lun);
		return -1;
	}

	dev->queue_depth = device_tcq;
	atomic_set(&dev->depth_left, dev->queue_depth);
	printk(KERN_INFO "Reset Device Queue Depth to %u for Logical Unit"
		" Number: %u\n", dev->queue_depth, unpacked_lun);

	return 0;
}
EXPORT_SYMBOL(transport_check_device_tcq);

unsigned char *transport_dump_cmd_direction(se_cmd_t *cmd)
{
#ifdef MY_ABC_HERE
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
#else
	switch (cmd->data_direction) {
	case SE_DIRECTION_NONE:
		return "NONE";
	case SE_DIRECTION_READ:
		return "READ";
	case SE_DIRECTION_WRITE:
		return "WRITE";
	case SE_DIRECTION_BIDI:
		return "BIDI";
	default:
		break;
	}
#endif

	return "UNKNOWN";
}

void transport_dump_dev_state(
	se_device_t *dev,
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

	*bl += sprintf(b + *bl, "  Execute/Left/Max Queue Depth: %d/%d/%d",
		atomic_read(&dev->execute_tasks), atomic_read(&dev->depth_left),
		dev->queue_depth);
	*bl += sprintf(b + *bl, "  SectorSize: %u  MaxSectors: %u\n",
		DEV_ATTRIB(dev)->block_size, DEV_ATTRIB(dev)->max_sectors);
	*bl += sprintf(b + *bl, "        ");
}

void transport_dump_dev_info(
	se_device_t *dev,
	se_lun_t *lun,
	unsigned long long total_bytes,
	char *b,         
	int *bl)
{
	se_subsystem_api_t *t;
	int ret = 0;

	t = (se_subsystem_api_t *)plugin_get_obj(PLUGIN_TYPE_TRANSPORT,
			dev->type, &ret);
	if (!t || (ret != 0))
		return;

	t->get_dev_info(dev, b, bl);
	*bl += sprintf(b + *bl, "        ");
	*bl += sprintf(b + *bl, "Type: %s ",
		scsi_device_type(TRANSPORT(dev)->get_device_type(dev)));
	*bl += sprintf(b + *bl, "ANSI SCSI revision: %02x  ",
		TRANSPORT(dev)->get_device_rev(dev));

#ifdef MY_ABC_HERE
	if (DEV_T10_WWN(dev)) {
		t10_wwn_t *wwn = DEV_T10_WWN(dev);
#else
	if (DEV_OBJ_API(dev)->get_t10_wwn) {
		t10_wwn_t *wwn = DEV_OBJ_API(dev)->get_t10_wwn((void *)dev);
#endif

		*bl += sprintf(b + *bl, "Unit Serial: %s  ",
			((strlen(wwn->unit_serial) != 0) ?
			(char *)wwn->unit_serial : "None"));
	}
	*bl += sprintf(b + *bl, "%s", "DIRECT");

#ifdef MY_ABC_HERE
	if (atomic_read(&dev->dev_access_obj.obj_access_count))
		*bl += sprintf(b + *bl, "  ACCESSED\n");
	else if (atomic_read(&dev->dev_export_obj.obj_access_count))
		*bl += sprintf(b + *bl, "  EXPORTED\n");
#else
	if ((DEV_OBJ_API(dev)->check_count(&dev->dev_access_obj)) ||
	    (DEV_OBJ_API(dev)->check_count(&dev->dev_feature_obj)))
		*bl += sprintf(b + *bl, "  ACCESSED\n");
	else if (DEV_OBJ_API(dev)->check_count(&dev->dev_export_obj))
		*bl += sprintf(b + *bl, "  EXPORTED\n");
#endif
	else
		*bl += sprintf(b + *bl, "  FREE\n");

	if (lun) {
		*bl += sprintf(b + *bl, "        Core Host ID: %u LUN: %u",
			dev->se_hba->hba_id, lun->unpacked_lun);
		if (!(TRANSPORT(dev)->get_device_type(dev))) {
			*bl += sprintf(b + *bl, "  Active Cmds: %d  Total Bytes"
				": %llu\n", atomic_read(&dev->active_cmds),
			total_bytes);
		} else {
			*bl += sprintf(b + *bl, "  Active Cmds: %d\n",
				atomic_read(&dev->active_cmds));
		}
	} else {
		if (!(TRANSPORT(dev)->get_device_type(dev))) {
			*bl += sprintf(b + *bl, "        Core Host ID: %u"
				"  Active Cmds: %d  Total Bytes: %llu\n",
				dev->se_hba->hba_id,
				atomic_read(&dev->active_cmds), total_bytes);
		} else {
			*bl += sprintf(b + *bl, "        CoreI Host ID: %u"
				"  Active Cmds: %d\n", dev->se_hba->hba_id,
				atomic_read(&dev->active_cmds));
		}
	}
}

static void transport_release_all_cmds(se_device_t *dev)
{
	se_cmd_t *cmd = NULL;
	se_queue_req_t *qr = NULL, *qr_p = NULL;
	int bug_out = 0, t_state;
	unsigned long flags;

	spin_lock_irqsave(&dev->dev_queue_obj->cmd_queue_lock, flags);
	list_for_each_entry_safe(qr, qr_p, &dev->dev_queue_obj->qobj_list,
				qr_list) {

		cmd = (se_cmd_t *)qr->cmd;
		t_state = qr->state;
		list_del(&qr->qr_list);
		kfree(qr);
		spin_unlock_irqrestore(&dev->dev_queue_obj->cmd_queue_lock,
				flags);

		printk(KERN_ERR "Releasing %s ITT: 0x%08x, i_state: %u,"
			" t_state: %u directly\n",
			(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH) ?
			"Passthrough" : "Normal",
			CMD_TFO(cmd)->get_task_tag(cmd),
			CMD_TFO(cmd)->get_cmd_state(cmd), t_state);

		transport_release_fe_cmd(cmd);
		bug_out = 1;

		spin_lock_irqsave(&dev->dev_queue_obj->cmd_queue_lock, flags);
	}
	spin_unlock_irqrestore(&dev->dev_queue_obj->cmd_queue_lock, flags);
#if 0
	if (bug_out)
		BUG();
#endif
}

static int transport_dev_write_pending_nop(se_task_t *task)
{
	return 0;
}

static int transport_get_inquiry(
#ifndef MY_ABC_HERE
	se_obj_lun_type_t *obj_api,
#endif
	t10_wwn_t *wwn,
	void *obj_ptr)
{
	se_cmd_t *cmd;
	unsigned char *buf;
	int i;
	unsigned char cdb[SCSI_CDB_SIZE];

	memset(cdb, 0, SCSI_CDB_SIZE);
	cdb[0] = INQUIRY;
	cdb[3] = (INQUIRY_LEN >> 8) & 0xff;
	cdb[4] = (INQUIRY_LEN & 0xff);

#ifdef MY_ABC_HERE
	cmd = transport_allocate_passthrough(&cdb[0],  DMA_FROM_DEVICE,
			0, NULL, 0, INQUIRY_LEN, obj_ptr);
#elif defined(MY_ABC_HERE)
	cmd = transport_allocate_passthrough(&cdb[0],  SE_DIRECTION_READ,
			0, NULL, 0, INQUIRY_LEN, obj_ptr);
#else
	cmd = transport_allocate_passthrough(&cdb[0],  SE_DIRECTION_READ,
			0, NULL, 0, INQUIRY_LEN, obj_api, obj_ptr);
#endif
	if (!(cmd))
		return -1;

	if (transport_generic_passthrough(cmd) < 0) {
		transport_passthrough_release(cmd);
		return -1;
	}

	buf = (unsigned char *)T_TASK(cmd)->t_task_buf;
	 
	memcpy((void *)&wwn->vendor[0], (void *)&buf[8],
			sizeof(wwn->vendor));
	memcpy((void *)&wwn->model[0], (void *)&buf[16],
			sizeof(wwn->model));
	memcpy((void *)&wwn->revision[0], (void *)&buf[32],
			sizeof(wwn->revision));

#ifndef MY_ABC_HERE
	printk("  Vendor: ");
	for (i = 8; i < 16; i++)
		if (buf[i] >= 0x20 && i < buf[4] + 5)
			printk("%c", buf[i]);
		else
			printk(" ");

	printk("  Model: ");
	for (i = 16; i < 32; i++)
		if (buf[i] >= 0x20 && i < buf[4] + 5)
			printk("%c", buf[i]);
		else
			printk(" ");

	printk("  Revision: ");
	for (i = 32; i < 36; i++)
		if (buf[i] >= 0x20 && i < buf[4] + 5)
			printk("%c", buf[i]);
		else
			printk(" ");

	printk("\n");
#endif

	i = buf[0] & 0x1f;

#ifndef MY_ABC_HERE
	printk("  Type:   %s ", scsi_device_type(i));
	printk("                 ANSI SCSI revision: %02x",
				buf[2] & 0x07);
	if ((buf[2] & 0x07) == 1 && (buf[3] & 0x0f) == 1)
		printk(" CCS\n");
	else
		printk("\n");
#endif

	transport_passthrough_release(cmd);
	return 0;
}

static int transport_get_inquiry_vpd_serial(
#ifndef MY_ABC_HERE
	se_obj_lun_type_t *obj_api,
#endif
	t10_wwn_t *wwn,
	void *obj_ptr)
{
	unsigned char *buf;
	se_cmd_t *cmd;
	unsigned char cdb[SCSI_CDB_SIZE];

	memset(cdb, 0, SCSI_CDB_SIZE);
	cdb[0] = INQUIRY;
	cdb[1] = 0x01;  
	cdb[2] = 0x80;  
	cdb[3] = (INQUIRY_VPD_SERIAL_LEN >> 8) & 0xff;
	cdb[4] = (INQUIRY_VPD_SERIAL_LEN & 0xff);

#ifdef MY_ABC_HERE
	cmd = transport_allocate_passthrough(&cdb[0], DMA_FROM_DEVICE,
			0, NULL, 0, INQUIRY_VPD_SERIAL_LEN, obj_ptr);
#elif defined(MY_ABC_HERE)
	cmd = transport_allocate_passthrough(&cdb[0], SE_DIRECTION_READ,
			0, NULL, 0, INQUIRY_VPD_SERIAL_LEN, obj_ptr);
#else
	cmd = transport_allocate_passthrough(&cdb[0], SE_DIRECTION_READ,
			0, NULL, 0, INQUIRY_VPD_SERIAL_LEN, obj_api, obj_ptr);
#endif
	if (!(cmd))
		return -1;

	if (transport_generic_passthrough(cmd) < 0) {
		transport_passthrough_release(cmd);
		return -1;
	}

	buf = (unsigned char *)T_TASK(cmd)->t_task_buf;

#ifndef MY_ABC_HERE
	printk(KERN_INFO "T10 VPD Unit Serial Number: %s\n", &buf[4]);
#endif
	snprintf(&wwn->unit_serial[0], INQUIRY_VPD_SERIAL_LEN, "%s", &buf[4]);

	transport_passthrough_release(cmd);
	return 0;
}

static const char hex_str[] = "0123456789abcdef";

void transport_dump_vpd_proto_id(
	t10_vpd_t *vpd,
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
#ifndef MY_ABC_HERE
	else
		printk(KERN_INFO "%s", buf);
#endif
}

void transport_set_vpd_proto_id(t10_vpd_t *vpd, unsigned char *page_83)
{
	 
	if (page_83[1] & 0x80) {
		vpd->protocol_identifier = (page_83[0] & 0xf0);
		vpd->protocol_identifier_set = 1;
		transport_dump_vpd_proto_id(vpd, NULL, 0);
	}
}

int transport_dump_vpd_assoc(
	t10_vpd_t *vpd,
	unsigned char *p_buf,
	int p_buf_len)
{
	unsigned char buf[VPD_TMP_BUF_SIZE];
	int ret = 0, len;

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
		ret = -1;
		break;
	}

	if (p_buf)
		strncpy(p_buf, buf, p_buf_len);
#ifndef MY_ABC_HERE
	else
		printk("%s", buf);
#endif

	return ret;
}

static int transport_set_vpd_assoc(t10_vpd_t *vpd, unsigned char *page_83)
{
	 
	vpd->association = (page_83[1] & 0x30);
	return transport_dump_vpd_assoc(vpd, NULL, 0);
}

int transport_dump_vpd_ident_type(
	t10_vpd_t *vpd,
	unsigned char *p_buf,
	int p_buf_len)
{
	unsigned char buf[VPD_TMP_BUF_SIZE];
	int ret = 0, len;

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
		ret = -1;
		break;
	}

	if (p_buf)
		strncpy(p_buf, buf, p_buf_len);
#ifndef MY_ABC_HERE
	else
		printk("%s", buf);
#endif

	return ret;
}

int transport_set_vpd_ident_type(t10_vpd_t *vpd, unsigned char *page_83)
{
	 
	vpd->device_identifier_type = (page_83[1] & 0x0f);
	return transport_dump_vpd_ident_type(vpd, NULL, 0);
}

int transport_dump_vpd_ident(
	t10_vpd_t *vpd,
	unsigned char *p_buf,
	int p_buf_len)
{
	unsigned char buf[VPD_TMP_BUF_SIZE];
	int ret = 0;

	memset(buf, 0, VPD_TMP_BUF_SIZE);

	switch (vpd->device_identifier_code_set) {
	case 0x01:  
		sprintf(buf, "T10 VPD Binary Device Identifier: %s\n",
			&vpd->device_identifier[0]);
		break;
	case 0x02:  
		sprintf(buf, "T10 VPD ASCII Device Identifier: %s\n",
			&vpd->device_identifier[0]);
		break;
	case 0x03:  
		sprintf(buf, "T10 VPD UTF-8 Device Identifier: %s\n",
			&vpd->device_identifier[0]);
		break;
	default:
		sprintf(buf, "T10 VPD Device Identifier encoding unsupported:"
			" 0x%02x", vpd->device_identifier_code_set);
		ret = -1;
		break;
	}

	if (p_buf)
		strncpy(p_buf, buf, p_buf_len);
#ifndef MY_ABC_HERE
	else
		printk("%s", buf);
#endif

	return ret;
}

int transport_set_vpd_ident(t10_vpd_t *vpd, unsigned char *page_83)
{
	int j = 0, i = 4;  

	vpd->device_identifier_code_set = (page_83[0] & 0x0f);
	switch (vpd->device_identifier_code_set) {
	case 0x01:  
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
	case 0x02:  
	case 0x03:  
		while (i < (4 + page_83[3]))
			vpd->device_identifier[j++] = page_83[i++];

		break;
	default:
		break;
	}

	return transport_dump_vpd_ident(vpd, NULL, 0);
}

static int transport_get_inquiry_vpd_device_ident(
#ifndef MY_ABC_HERE
	se_obj_lun_type_t *obj_api,
#endif
	t10_wwn_t *wwn,
	void *obj_ptr)
{
	unsigned char *buf, *page_83;
	se_cmd_t *cmd;
	t10_vpd_t *vpd;
	unsigned char cdb[SCSI_CDB_SIZE];
	int ident_len, page_len, off = 4, ret = 0;

	memset(cdb, 0, SCSI_CDB_SIZE);
	cdb[0] = INQUIRY;
	cdb[1] = 0x01;  
	cdb[2] = 0x83;  
	cdb[3] = (INQUIRY_VPD_DEVICE_IDENTIFIER_LEN >> 8) & 0xff;
	cdb[4] = (INQUIRY_VPD_DEVICE_IDENTIFIER_LEN & 0xff);

#ifdef MY_ABC_HERE
	cmd = transport_allocate_passthrough(&cdb[0], DMA_FROM_DEVICE,
			0, NULL, 0, INQUIRY_VPD_DEVICE_IDENTIFIER_LEN,
			obj_ptr);
#elif defined(MY_ABC_HERE)
	cmd = transport_allocate_passthrough(&cdb[0], SE_DIRECTION_READ,
			0, NULL, 0, INQUIRY_VPD_DEVICE_IDENTIFIER_LEN,
			obj_ptr);
#else
	cmd = transport_allocate_passthrough(&cdb[0], SE_DIRECTION_READ,
			0, NULL, 0, INQUIRY_VPD_DEVICE_IDENTIFIER_LEN,
			obj_api, obj_ptr);
#endif
	if (!(cmd))
		return -1;

	if (transport_generic_passthrough(cmd) < 0) {
		transport_passthrough_release(cmd);
		return -1;
	}

	buf = (unsigned char *)T_TASK(cmd)->t_task_buf;
	page_len = (buf[2] << 8) | buf[3];
#ifndef MY_ABC_HERE
	printk("T10 VPD Page Length: %d\n", page_len);
#endif

	while (page_len > 0) {
		 
		page_83 = &buf[off];
		ident_len = page_83[3];
		if (!(ident_len)) {
			printk(KERN_ERR "page_83[3]: identifier"
					" length zero!\n");
			break;
		}
#ifndef MY_ABC_HERE
		printk(KERN_INFO "T10 VPD Identifer Length: %d\n", ident_len);
#endif

		vpd = kzalloc(sizeof(t10_vpd_t), GFP_KERNEL);
		if (!(vpd)) {
			printk(KERN_ERR "Unable to allocate memory for"
					" t10_vpd_t\n");
			ret = -1;
			goto out;
		}
		INIT_LIST_HEAD(&vpd->vpd_list);

		transport_set_vpd_proto_id(vpd, page_83);
		transport_set_vpd_assoc(vpd, page_83);

		if (transport_set_vpd_ident_type(vpd, page_83) < 0) {
			off += (ident_len + 4);
			page_len -= (ident_len + 4);
			kfree(vpd);
			continue;
		}
		if (transport_set_vpd_ident(vpd, page_83) < 0) {
			off += (ident_len + 4);
			page_len -= (ident_len + 4);
			kfree(vpd);
			continue;
		}

		list_add_tail(&vpd->vpd_list, &wwn->t10_vpd_list);
		off += (ident_len + 4);
		page_len -= (ident_len + 4);
	}
out:
	transport_passthrough_release(cmd);
	return 0;
}

int transport_rescan_evpd_device_ident(
	se_device_t *dev)
{
	se_release_vpd_for_dev(dev);
#ifdef MY_ABC_HERE
	transport_get_inquiry_vpd_device_ident(DEV_T10_WWN(dev), (void *)dev);
#else
	transport_get_inquiry_vpd_device_ident(DEV_OBJ_API(dev),
			DEV_T10_WWN(dev), (void *)dev);
#endif
	return 0;
}

static int transport_get_read_capacity(se_device_t *dev)
{
	unsigned char cdb[SCSI_CDB_SIZE], *buf;
	u32 blocks, v1, v2;
	se_cmd_t *cmd;
	unsigned long long blocks_long;

	memset(cdb, 0, SCSI_CDB_SIZE);
	cdb[0] = 0x25;  

#ifdef MY_ABC_HERE
	cmd = transport_allocate_passthrough(&cdb[0], DMA_FROM_DEVICE,
			0, NULL, 0, READ_CAP_LEN, (void *)dev);
#elif defined(MY_ABC_HERE)
	cmd = transport_allocate_passthrough(&cdb[0], SE_DIRECTION_READ,
			0, NULL, 0, READ_CAP_LEN, (void *)dev);
#else
	cmd = transport_allocate_passthrough(&cdb[0], SE_DIRECTION_READ,
			0, NULL, 0, READ_CAP_LEN, DEV_OBJ_API(dev),
			(void *)dev);
#endif
	if (!(cmd))
		return -1;

	if (transport_generic_passthrough(cmd) < 0) {
		transport_passthrough_release(cmd);
		return -1;
	}

	buf = (unsigned char *)T_TASK(cmd)->t_task_buf;
	blocks = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

	transport_passthrough_release(cmd);

	if (blocks != 0xFFFFFFFF) {
		dev->dev_sectors_total = blocks;
		dev->dev_generate_cdb = &split_cdb_RW_10;
		return 0;
	}

#ifdef MY_ABC_HERE
	printk(KERN_INFO "iSCSI - READ_CAPACITY returned 0xFFFFFFFF, issuing"
			" SAI_READ_CAPACITY_16\n");
#else
	printk(KERN_INFO "READ_CAPACITY returned 0xFFFFFFFF, issuing"
			" SAI_READ_CAPACITY_16\n");
#endif

	memset(cdb, 0, SCSI_CDB_SIZE);
	cdb[0] = 0x9e;  
	cdb[1] = 0x10;  
	cdb[13] = 12;

#ifdef MY_ABC_HERE
	cmd = transport_allocate_passthrough(&cdb[0], DMA_FROM_DEVICE,
			0, NULL, 0, 12, (void *)dev);
#elif defined(MY_ABC_HERE)
	cmd = transport_allocate_passthrough(&cdb[0], SE_DIRECTION_READ,
			0, NULL, 0, 12, (void *)dev);
#else
	cmd = transport_allocate_passthrough(&cdb[0], SE_DIRECTION_READ,
			0, NULL, 0, 12, DEV_OBJ_API(dev), (void *)dev);
#endif
	if (!(cmd))
		return -1;

	if (transport_generic_passthrough(cmd) < 0) {
		transport_passthrough_release(cmd);
		return -1;
	}

	buf = (unsigned char *)T_TASK(cmd)->t_task_buf;
	v1 = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	v2 = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
	blocks_long = ((unsigned long long)v2 | (unsigned long long)v1 << 32);

	transport_passthrough_release(cmd);

	dev->dev_sectors_total = blocks_long;
	dev->dev_generate_cdb = &split_cdb_RW_16;

	return 0;
}

static void core_setup_task_attr_emulation(se_device_t *dev)
{
	 
	if (TRANSPORT(dev)->transport_type == TRANSPORT_PLUGIN_PHBA_PDEV) {
		dev->dev_task_attr_type = SAM_TASK_ATTR_PASSTHROUGH;
		return;
	}

	dev->dev_task_attr_type = SAM_TASK_ATTR_EMULATED;
	DEBUG_STA("%s: Using SAM_TASK_ATTR_EMULATED for SPC: 0x%02x"
		" device\n", TRANSPORT(dev)->name,
		TRANSPORT(dev)->get_device_rev(dev));
}

se_device_t *transport_add_device_to_core_hba(
	se_hba_t *hba,
	se_subsystem_api_t *transport,
	se_subsystem_dev_t *se_dev,
	u32 device_flags,
	void *transport_dev)
{
	int ret = 0, force_pt;
	se_device_t  *dev;

	dev = kzalloc(sizeof(se_device_t), GFP_KERNEL);
	if (!(dev)) {
		printk(KERN_ERR "Unable to allocate memory for se_dev_t\n");
		return NULL;
	}
	dev->dev_queue_obj = kzalloc(sizeof(se_queue_obj_t), GFP_KERNEL);
	if (!(dev->dev_queue_obj)) {
		printk(KERN_ERR "Unable to allocate memory for"
				" dev->dev_queue_obj\n");
		kfree(dev);
		return NULL;
	}
	transport_init_queue_obj(dev->dev_queue_obj);

	dev->dev_status_queue_obj = kzalloc(sizeof(se_queue_obj_t),
					GFP_KERNEL);
	if (!(dev->dev_status_queue_obj)) {
		printk(KERN_ERR "Unable to allocate memory for"
				" dev->dev_status_queue_obj\n");
		kfree(dev->dev_queue_obj);
		kfree(dev);
		return NULL;
	}
	transport_init_queue_obj(dev->dev_status_queue_obj);

	dev->dev_flags		= device_flags;
	dev->dev_status		|= TRANSPORT_DEVICE_DEACTIVATED;
	dev->type		= transport->type;
	dev->dev_ptr		= (void *) transport_dev;
	dev->se_hba		= hba;
	dev->se_sub_dev		= se_dev;
	dev->transport		= transport;
	atomic_set(&dev->active_cmds, 0);
	INIT_LIST_HEAD(&dev->dev_list);
	INIT_LIST_HEAD(&dev->dev_sep_list);
	INIT_LIST_HEAD(&dev->dev_tmr_list);
	INIT_LIST_HEAD(&dev->execute_task_list);
	INIT_LIST_HEAD(&dev->delayed_cmd_list);
	INIT_LIST_HEAD(&dev->ordered_cmd_list);
	INIT_LIST_HEAD(&dev->state_task_list);
	spin_lock_init(&dev->execute_task_lock);
	spin_lock_init(&dev->delayed_cmd_lock);
	spin_lock_init(&dev->ordered_cmd_lock);
	spin_lock_init(&dev->state_task_lock);
	spin_lock_init(&dev->dev_alua_lock);
	spin_lock_init(&dev->dev_reservation_lock);
	spin_lock_init(&dev->dev_status_lock);
	spin_lock_init(&dev->dev_status_thr_lock);
	spin_lock_init(&dev->se_port_lock);
	spin_lock_init(&dev->se_tmr_lock);

	dev->queue_depth	= TRANSPORT(dev)->get_queue_depth(dev);
	atomic_set(&dev->depth_left, dev->queue_depth);
	atomic_set(&dev->dev_ordered_id, 0);

	se_dev_set_default_attribs(dev);

	dev->write_pending = (transport->write_pending) ?
		transport->write_pending : &transport_dev_write_pending_nop;

#ifdef SNMP_SUPPORT
	dev->dev_index = scsi_get_new_index(SCSI_DEVICE_INDEX);
	dev->creation_time = get_jiffies_64();
	spin_lock_init(&dev->stats_lock);
#endif  

	spin_lock(&hba->device_lock);
	list_add_tail(&dev->dev_list, &hba->hba_dev_list);
	hba->dev_count++;
	spin_unlock(&hba->device_lock);

#ifndef MY_ABC_HERE
	 
	dev->dev_obj_api = se_obj_get_api(TRANSPORT_LUN_TYPE_DEVICE);
	if (!(dev->dev_obj_api))
		goto out;
#endif
	 
	core_setup_task_attr_emulation(dev);
	 
	force_pt = (hba->hba_flags & HBA_FLAGS_INTERNAL_USE);
	 
	core_setup_reservations(dev, force_pt);
	 
	if (core_setup_alua(dev, force_pt) < 0)
		goto out;
	 
	if (transport_generic_activate_device(dev) < 0)
		goto out;

#ifdef MY_ABC_HERE
	ret = transport_get_inquiry(DEV_T10_WWN(dev), (void *)dev);
#else
	ret = transport_get_inquiry(DEV_OBJ_API(dev),
			DEV_T10_WWN(dev), (void *)dev);
#endif
	if (ret < 0)
		goto out;
	 
#ifdef MY_ABC_HERE
	if (!(transport_get_inquiry_vpd_serial(DEV_T10_WWN(dev),
			(void *)dev))) {
		 
		transport_get_inquiry_vpd_device_ident(DEV_T10_WWN(dev),
			(void *)dev);
	}
#else
	if (!(transport_get_inquiry_vpd_serial(DEV_OBJ_API(dev),
			DEV_T10_WWN(dev), (void *)dev))) {
		 
		transport_get_inquiry_vpd_device_ident(DEV_OBJ_API(dev),
			DEV_T10_WWN(dev), (void *)dev);
	}
#endif

	if (TRANSPORT(dev)->get_device_type(dev) != 0)
		return dev;

	ret = transport_get_read_capacity(dev);
	if (ret < 0)
		goto out;
out:
	if (!ret)
		return dev;

	transport_generic_release_phydevice(dev, 0);

	transport_generic_deactivate_device(dev);

	spin_lock(&hba->device_lock);
	list_del(&dev->dev_list);
	hba->dev_count--;
	spin_unlock(&hba->device_lock);

	se_release_vpd_for_dev(dev);

	kfree(dev->dev_status_queue_obj);
	kfree(dev->dev_queue_obj);
	kfree(dev);

	return NULL;
}

int transport_generic_activate_device(se_device_t *dev)
{
	char name[16];

	if (TRANSPORT(dev)->activate_device)
		TRANSPORT(dev)->activate_device(dev);

	memset(name, 0, 16);
	snprintf(name, 16, "LIO_%s", TRANSPORT(dev)->name);

	dev->process_thread = kthread_run(transport_processing_thread,
			(void *)dev, name);
	if (IS_ERR(dev->process_thread)) {
		printk(KERN_ERR "Unable to create kthread: %s\n", name);
		return -1;
	}

	wait_for_completion(&dev->dev_queue_obj->thread_create_comp);

	return 0;
}

void transport_generic_deactivate_device(se_device_t *dev)
{
	if (TRANSPORT(dev)->deactivate_device)
		TRANSPORT(dev)->deactivate_device(dev);

	kthread_stop(dev->process_thread);

	wait_for_completion(&dev->dev_queue_obj->thread_done_comp);
}

int transport_generic_claim_phydevice(se_device_t *dev)
{
	int ret;
	se_hba_t *hba;

	if (!TRANSPORT(dev)->claim_phydevice)
		return 0;

	if (dev->dev_flags & DF_READ_ONLY)
		return 0;

	if (dev->dev_flags & DF_CLAIMED_BLOCKDEV)
		return 0;

	hba = dev->se_hba;
	if (!(hba)) {
		printk(KERN_ERR "se_device_t->se_hba is NULL!\n");
		return -1;
	}

	ret = TRANSPORT(dev)->claim_phydevice(hba, dev);
	if (ret < 0)
		return ret;

	dev->dev_flags |= DF_CLAIMED_BLOCKDEV;

	return 1;
}
EXPORT_SYMBOL(transport_generic_claim_phydevice);

void transport_generic_release_phydevice(se_device_t *dev, int check_pscsi)
{
	if (!TRANSPORT(dev)->release_phydevice)
		return;

	if (dev->dev_flags & DF_READ_ONLY) {
		if (check_pscsi &&
		   (TRANSPORT(dev)->transport_type !=
		    TRANSPORT_PLUGIN_PHBA_PDEV))
				return;

		TRANSPORT(dev)->release_phydevice(dev);
		return;
	}

	if (!(dev->dev_flags & DF_CLAIMED_BLOCKDEV))
		return;

	if (!dev->dev_ptr) {
		printk(KERN_ERR "se_device_t->dev_ptr is NULL!\n");
		BUG();
	}

	if (check_pscsi) {
		if (TRANSPORT(dev)->transport_type !=
		    TRANSPORT_PLUGIN_PHBA_PDEV)
			return;

		if (dev->dev_flags & DF_PERSISTENT_CLAIMED_BLOCKDEV)
			return;
	}

	TRANSPORT(dev)->release_phydevice(dev);
	dev->dev_flags &= ~DF_CLAIMED_BLOCKDEV;
}

void transport_generic_free_device(se_device_t *dev)
{
	if (!(dev->dev_ptr))
		return;

	transport_generic_deactivate_device(dev);

	transport_generic_release_phydevice(dev, 0);

	if (TRANSPORT(dev)->free_device)
		TRANSPORT(dev)->free_device(dev->dev_ptr);
}
EXPORT_SYMBOL(transport_generic_free_device);

static inline int transport_allocate_iovecs_for_cmd(
	se_cmd_t *cmd,
	u32 iov_count)
{
	cmd->iov_data = kzalloc(iov_count * sizeof(struct iovec), GFP_KERNEL);
	if (!(cmd->iov_data)) {
		printk(KERN_ERR "Unable to allocate memory for"
			" iscsi_cmd_t->iov_data.\n");
		return -1;
	}
	cmd->orig_iov_data_count = iov_count;

	return 0;
}

int transport_generic_allocate_iovecs(
	se_cmd_t *cmd)
{
	u32 iov_count;

	iov_count = T_TASK(cmd)->t_task_se_num;
	if (!(iov_count))
		iov_count = 1;
#if 0
	printk(KERN_INFO "Allocated %d iovecs for ITT: 0x%08x t_task_se_num:"
		" %u\n", iov_count, CMD_TFO(cmd)->get_task_tag(cmd),
		T_TASK(cmd)->t_task_se_num);
#endif
	iov_count += TRANSPORT_IOV_DATA_BUFFER;

	return transport_allocate_iovecs_for_cmd(cmd, iov_count);
}
EXPORT_SYMBOL(transport_generic_allocate_iovecs);

static inline void transport_generic_prepare_cdb(
	unsigned char *cdb)
{
	switch (cdb[0]) {
	case READ_10:  
	case READ_12:  
	case READ_16:  
	case SEND_DIAGNOSTIC:  
	case VERIFY:  
	case VERIFY_16:  
	case WRITE_VERIFY:  
	case WRITE_VERIFY_12:  
		break;
	default:
		cdb[1] &= 0x1f;  
		break;
	}
}

static inline int transport_check_device_cdb_sector_count(
#ifndef MY_ABC_HERE
	se_obj_lun_type_t *se_obj_api,
#endif
	void *se_obj_ptr,
	u32 sectors)
{
	u32 max_sectors;

#ifdef MY_ABC_HERE
	max_sectors = dev_obj_max_sectors(se_obj_ptr);
#else
	max_sectors = se_obj_api->max_sectors(se_obj_ptr);
#endif
	if (!(max_sectors)) {
		printk(KERN_ERR "TRANSPORT->get_max_sectors returned zero!\n");
		return 1;
	}

	if (sectors > max_sectors)
		return -1;

	return 0;
}

#ifdef MY_ABC_HERE
static se_task_t *transport_generic_get_task(
	se_transform_info_t *ti,
	se_cmd_t *cmd,
	void *se_obj_ptr)
#else
static se_task_t *transport_generic_get_task(
	se_transform_info_t *ti,
	se_cmd_t *cmd,
	void *se_obj_ptr,
	se_obj_lun_type_t *se_obj_api)
#endif
{
	se_task_t *task;
#ifdef MY_ABC_HERE
	struct se_device_s *dev = SE_DEV(cmd);
#endif
	unsigned long flags;

	task = kmem_cache_zalloc(se_task_cache, GFP_KERNEL);
	if (!(task)) {
		printk(KERN_ERR "Unable to allocate se_task_t\n");
		return NULL;
	}

	INIT_LIST_HEAD(&task->t_list);
	INIT_LIST_HEAD(&task->t_execute_list);
	INIT_LIST_HEAD(&task->t_state_list);
	init_completion(&task->task_stop_comp);
	task->task_no = T_TASK(cmd)->t_task_no++;
	task->task_se_cmd = cmd;
#ifdef MY_ABC_HERE
	task->se_dev = dev;

	DEBUG_SO("se_obj_ptr: %p\n", se_obj_ptr);

	task->transport_req = TRANSPORT(dev)->allocate_request(task, dev);
	if (!(task->transport_req)) {
		kmem_cache_free(se_task_cache, task);
		return NULL;
	}

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	list_add_tail(&task->t_list, &T_TASK(cmd)->t_task_list);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	task->se_obj_ptr = se_obj_ptr;
#else

	DEBUG_SO("se_obj_ptr: %p\n", se_obj_ptr);
	DEBUG_SO("se_obj_api: %p\n", se_obj_api);
	DEBUG_SO("Plugin: %s\n", se_obj_api->obj_plugin->plugin_name);

	task->transport_req = se_obj_api->get_transport_req(se_obj_ptr, task);
	if (!(task->transport_req))
		return NULL;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	list_add_tail(&task->t_list, &T_TASK(cmd)->t_task_list);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	task->se_obj_api = se_obj_api;
	task->se_obj_ptr = se_obj_ptr;
#endif

	return task;
}

#ifdef MY_ABC_HERE
static inline int transport_generic_obj_start(
	se_transform_info_t *ti,
	void *p,
	unsigned long long starting_lba)
{
	ti->ti_lba = starting_lba;
	ti->ti_obj_ptr = p;

	return 0;
}
#else
int transport_generic_obj_start(
	se_transform_info_t *ti,
	se_obj_lun_type_t *obj_api,
	void *p,
	unsigned long long starting_lba)
{
	ti->ti_lba = starting_lba;
	ti->ti_obj_api = obj_api;
	ti->ti_obj_ptr = p;

	return 0;
}
#endif

static int transport_process_data_sg_transform(
	se_cmd_t *cmd,
	se_transform_info_t *ti)
{
	 
	return 0;
}

static int transport_process_control_sg_transform(
	se_cmd_t *cmd,
	se_transform_info_t *ti)
{
	unsigned char *cdb;
	se_task_t *task;
	se_mem_t *se_mem, *se_mem_lout = NULL;
#ifdef MY_ABC_HERE
	struct se_device_s *dev = SE_DEV(cmd);
#endif
	int ret;
	u32 se_mem_cnt = 0, task_offset = 0;

	list_for_each_entry(se_mem, T_TASK(cmd)->t_mem_list, se_list)
		break;

	if (!se_mem) {
		printk(KERN_ERR "se_mem is NULL!\n");
		return -1;
	}

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	task = transport_generic_get_task(ti, cmd, ti->se_obj_ptr);
	if (!(task))
		return -1;

	task->transport_map_task = dev_obj_get_map_SG(ti->se_obj_ptr,
				cmd->data_direction);

	cdb = TRANSPORT(dev)->get_cdb(task);
#elif defined(MY_ABC_HERE)
	task = cmd->transport_get_task(ti, cmd, ti->se_obj_ptr);
	if (!(task))
		return -1;

	task->transport_map_task = dev_obj_get_map_SG(ti->se_obj_ptr,
				cmd->data_direction);

	cdb = TRANSPORT(dev)->get_cdb(task);
#else
	task = cmd->transport_get_task(ti, cmd, ti->se_obj_ptr,
				ti->se_obj_api);
	if (!(task))
		return -1;

	task->transport_map_task = ti->se_obj_api->get_map_SG(
			ti->se_obj_ptr, cmd->data_direction);

	cdb = ti->se_obj_api->get_cdb(ti->se_obj_ptr, task);
#endif
	if (cdb)
		memcpy(cdb, T_TASK(cmd)->t_task_cdb, SCSI_CDB_SIZE);

	task->task_size = cmd->data_length;
	task->task_sg_num = 1;

	atomic_inc(&T_TASK(cmd)->t_fe_count);
	atomic_inc(&T_TASK(cmd)->t_se_count);

#ifdef MY_ABC_HERE
	ret = dev_obj_do_se_mem_map(ti->se_obj_ptr, task,
			T_TASK(cmd)->t_mem_list, NULL, se_mem, &se_mem_lout,
			&se_mem_cnt, &task_offset);
#else
	ret = ti->se_obj_api->do_se_mem_map(ti->se_obj_ptr, task,
			T_TASK(cmd)->t_mem_list, NULL, se_mem, &se_mem_lout,
			&se_mem_cnt, &task_offset);
#endif
	if (ret < 0)
		return ret;

	DEBUG_CDB_H("task_no[%u]: SCF_SCSI_CONTROL_SG_IO_CDB task_size: %d\n",
			task->task_no, task->task_size);
	return 0;
}

static int transport_process_control_nonsg_transform(
	se_cmd_t *cmd,
	se_transform_info_t *ti)
{
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	struct se_device_s *dev = SE_DEV(cmd);
	unsigned char *cdb;
	se_task_t *task;

	task = transport_generic_get_task(ti, cmd, ti->se_obj_ptr);
	if (!(task))
		return -1;

	task->transport_map_task = dev_obj_get_map_non_SG(ti->se_obj_ptr,
			cmd->data_direction);

	cdb = TRANSPORT(dev)->get_cdb(task);
#elif defined(MY_ABC_HERE)
	struct se_device_s *dev = SE_DEV(cmd);
	unsigned char *cdb;
	se_task_t *task;

	task = cmd->transport_get_task(ti, cmd, ti->se_obj_ptr);
	if (!(task))
		return -1;

	task->transport_map_task = dev_obj_get_map_non_SG(ti->se_obj_ptr,
			cmd->data_direction);

	cdb = TRANSPORT(dev)->get_cdb(task);
#else
	unsigned char *cdb;
	se_task_t *task;

	task = cmd->transport_get_task(ti, cmd, ti->se_obj_ptr,
				ti->se_obj_api);
	if (!(task))
		return -1;

	task->transport_map_task = ti->se_obj_api->get_map_non_SG(
			ti->se_obj_ptr, cmd->data_direction);

	cdb = ti->se_obj_api->get_cdb(ti->se_obj_ptr, task);
#endif
	if (cdb)
		memcpy(cdb, T_TASK(cmd)->t_task_cdb, SCSI_CDB_SIZE);

	task->task_size = cmd->data_length;
	task->task_sg_num = 0;

	atomic_inc(&T_TASK(cmd)->t_fe_count);
	atomic_inc(&T_TASK(cmd)->t_se_count);

	DEBUG_CDB_H("task_no[%u]: SCF_SCSI_CONTROL_NONSG_IO_CDB task_size:"
			" %d\n", task->task_no, task->task_size);
	return 0;
}

static int transport_process_non_data_transform(
	se_cmd_t *cmd,
	se_transform_info_t *ti)
{
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	struct se_device_s *dev = SE_DEV(cmd);
	unsigned char *cdb;
	se_task_t *task;

	task = transport_generic_get_task(ti, cmd, ti->se_obj_ptr);
	if (!(task))
		return -1;

	task->transport_map_task = dev_obj_get_map_none(ti->se_obj_ptr);

	cdb = TRANSPORT(dev)->get_cdb(task);
#elif defined(MY_ABC_HERE)
	struct se_device_s *dev = SE_DEV(cmd);
	unsigned char *cdb;
	se_task_t *task;

	task = cmd->transport_get_task(ti, cmd, ti->se_obj_ptr);
	if (!(task))
		return -1;

	task->transport_map_task = dev_obj_get_map_none(ti->se_obj_ptr);

	cdb = TRANSPORT(dev)->get_cdb(task);
#else
	unsigned char *cdb;
	se_task_t *task;

	task = cmd->transport_get_task(ti, cmd, ti->se_obj_ptr,
				ti->se_obj_api);
	if (!(task))
		return -1;

	task->transport_map_task = ti->se_obj_api->get_map_none(ti->se_obj_ptr);

	cdb = ti->se_obj_api->get_cdb(ti->se_obj_ptr, task);
#endif
	if (cdb)
		memcpy(cdb, T_TASK(cmd)->t_task_cdb, SCSI_CDB_SIZE);

	task->task_size = cmd->data_length;
	task->task_sg_num = 0;

	atomic_inc(&T_TASK(cmd)->t_fe_count);
	atomic_inc(&T_TASK(cmd)->t_se_count);

	DEBUG_CDB_H("task_no[%u]: SCF_SCSI_NON_DATA_CDB task_size: %d\n",
			task->task_no, task->task_size);
	return 0;
}

static int transport_generic_cmd_sequencer(se_cmd_t *, unsigned char *);

void transport_device_setup_cmd(se_cmd_t *cmd)
{
#ifndef SYNO_LIO_TRANSPORT_PATCHES
	cmd->transport_add_cmd_to_queue = &transport_add_cmd_to_dev_queue;
#endif
	cmd->se_dev = SE_LUN(cmd)->se_dev;
}

se_cmd_t *__transport_alloc_se_cmd(
	struct target_core_fabric_ops *tfo,
	se_session_t *se_sess,
	void *fabric_cmd_ptr,
	u32 data_length,
	int data_direction,
	int task_attr)
{
	se_cmd_t *cmd;
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	unsigned char *sense_buffer;
#endif
	int gfp_type = (in_interrupt()) ? GFP_ATOMIC : GFP_KERNEL;

#ifdef MY_ABC_HERE
	if (data_direction == DMA_BIDIRECTIONAL) {
		printk(KERN_ERR "SCSI BiDirectional mode not supported yet\n");
		return ERR_PTR(-ENOSYS);
	}
#else
	if (data_direction == SE_DIRECTION_BIDI) {
		printk(KERN_ERR "SCSI BiDirectional mode not supported yet\n");
		return ERR_PTR(-ENOSYS);
	}
#endif

	cmd = kmem_cache_zalloc(se_cmd_cache, gfp_type);
	if (!(cmd)) {
		printk(KERN_ERR "kmem_cache_alloc() failed for se_cmd_cache\n");
		return ERR_PTR(-ENOMEM);
	}
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	sense_buffer = kzalloc(
			TRANSPORT_SENSE_BUFFER + tfo->get_fabric_sense_len(),
			gfp_type);
	if (!(sense_buffer)) {
		printk(KERN_ERR "Unable to allocate memory for"
			" cmd->sense_buffer\n");
		kmem_cache_free(se_cmd_cache, cmd);
		return NULL;
	}
	 
	transport_init_se_cmd(cmd, tfo, se_sess, data_length, data_direction,
			task_attr, sense_buffer);
	 
	cmd->se_fabric_cmd_ptr = fabric_cmd_ptr;

	return cmd;
#else
	INIT_LIST_HEAD(&cmd->se_lun_list);
	INIT_LIST_HEAD(&cmd->se_delayed_list);
	INIT_LIST_HEAD(&cmd->se_ordered_list);

	cmd->t_task = kzalloc(sizeof(se_transport_task_t), gfp_type);
	if (!(cmd->t_task)) {
		printk(KERN_ERR "Unable to allocate cmd->t_task\n");
		kmem_cache_free(se_cmd_cache, cmd);
		return NULL;
	}

	cmd->sense_buffer = kzalloc(
			TRANSPORT_SENSE_BUFFER + tfo->get_fabric_sense_len(),
			gfp_type);
	if (!(cmd->sense_buffer)) {
		printk(KERN_ERR "Unable to allocate memory for"
			" cmd->sense_buffer\n");
		kfree(cmd->t_task);
		kmem_cache_free(se_cmd_cache, cmd);
		return NULL;
	}
	INIT_LIST_HEAD(&T_TASK(cmd)->t_task_list);
	init_completion(&T_TASK(cmd)->transport_lun_fe_stop_comp);
	init_completion(&T_TASK(cmd)->transport_lun_stop_comp);
	init_completion(&T_TASK(cmd)->t_transport_stop_comp);
	init_completion(&T_TASK(cmd)->t_transport_passthrough_comp);
	init_completion(&T_TASK(cmd)->t_transport_passthrough_wcomp);
	spin_lock_init(&T_TASK(cmd)->t_state_lock);
#ifdef MY_ABC_HERE
	atomic_set(&T_TASK(cmd)->transport_dev_active, 1);
#endif

	cmd->se_tfo = tfo;
	cmd->se_sess = se_sess;
	cmd->se_fabric_cmd_ptr = fabric_cmd_ptr;
	cmd->data_length = data_length;
	cmd->data_direction = data_direction;
	cmd->sam_task_attr = task_attr;

	return cmd;
#endif
}

#ifdef SYNO_LIO_TRANSPORT_PATCHES
 
void transport_init_se_cmd(
		struct se_cmd_s *cmd,
		struct target_core_fabric_ops *tfo,
		struct se_session_s *se_sess,
		u32 data_length,
		int data_direction,
		int task_attr,
		unsigned char *sense_buffer)
{
	INIT_LIST_HEAD(&cmd->se_lun_list);
	INIT_LIST_HEAD(&cmd->se_delayed_list);
	INIT_LIST_HEAD(&cmd->se_ordered_list);
	 
	cmd->t_task = &cmd->t_task_backstore;

	INIT_LIST_HEAD(&T_TASK(cmd)->t_task_list);
	init_completion(&T_TASK(cmd)->transport_lun_fe_stop_comp);
	init_completion(&T_TASK(cmd)->transport_lun_stop_comp);
	init_completion(&T_TASK(cmd)->t_transport_stop_comp);
	init_completion(&T_TASK(cmd)->t_transport_passthrough_comp);
	init_completion(&T_TASK(cmd)->t_transport_passthrough_wcomp);
	spin_lock_init(&T_TASK(cmd)->t_state_lock);
	atomic_set(&T_TASK(cmd)->transport_dev_active, 1);

	cmd->se_tfo = tfo;
	cmd->se_sess = se_sess;
	cmd->data_length = data_length;
	cmd->data_direction = data_direction;
	cmd->sam_task_attr = task_attr;
	cmd->sense_buffer = sense_buffer;
}
EXPORT_SYMBOL(transport_init_se_cmd);
#endif

int transport_check_alloc_task_attr(se_cmd_t *cmd)
{
	 
	if (SE_DEV(cmd)->dev_task_attr_type != SAM_TASK_ATTR_EMULATED)
		return 0;

	if (cmd->sam_task_attr == TASK_ATTR_ACA) {
		DEBUG_STA("SAM Task Attribute ACA"
			" emulation is not supported\n");
		return -1;
	}
	 
	cmd->se_ordered_id = atomic_inc_return(&SE_DEV(cmd)->dev_ordered_id);
	smp_mb__after_atomic_inc();
	DEBUG_STA("Allocated se_ordered_id: %u for Task Attr: 0x%02x on %s\n",
			cmd->se_ordered_id, cmd->sam_task_attr,
			TRANSPORT(cmd->se_dev)->name);
	return 0;
}

se_cmd_t *transport_alloc_se_cmd(
	struct target_core_fabric_ops *tfo_api,
	se_session_t *se_sess,
	void *fabric_cmd_ptr,
	u32 data_length,
	int data_direction,
	int task_attr)
{
	return __transport_alloc_se_cmd(tfo_api, se_sess, fabric_cmd_ptr,
				data_length, data_direction, task_attr);
}
EXPORT_SYMBOL(transport_alloc_se_cmd);

void transport_free_se_cmd(
	se_cmd_t *se_cmd)
{
	if (se_cmd->se_tmr_req)
		core_tmr_release_req(se_cmd->se_tmr_req);

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	 
	kfree(se_cmd->iov_data);
	 
	if (se_cmd->se_fabric_cmd_ptr) {
		kfree(se_cmd->sense_buffer);
		kmem_cache_free(se_cmd_cache, se_cmd);
	}
#else
	kfree(se_cmd->iov_data);
	kfree(se_cmd->sense_buffer);
	kfree(se_cmd->t_task);
	kmem_cache_free(se_cmd_cache, se_cmd);
#endif
}
EXPORT_SYMBOL(transport_free_se_cmd);

static void transport_generic_wait_for_tasks(se_cmd_t *, int, int);

int transport_generic_allocate_tasks(
	se_cmd_t *cmd,
	unsigned char *cdb)
{
	int non_data_cdb;

	transport_generic_prepare_cdb(cdb);

	cmd->transport_wait_for_tasks = &transport_generic_wait_for_tasks;

#ifdef MY_ABC_HERE
	transport_device_setup_cmd(cmd);
#else
	CMD_ORIG_OBJ_API(cmd)->transport_setup_cmd(cmd->se_orig_obj_ptr, cmd);
#endif
	 
	non_data_cdb = transport_generic_cmd_sequencer(cmd, cdb);
	if (non_data_cdb < 0)
		return -1;
	 
	memcpy(T_TASK(cmd)->t_task_cdb, cdb, SCSI_CDB_SIZE);
	 
	if (transport_check_alloc_task_attr(cmd) < 0) {
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = INVALID_CDB_FIELD;
		return -2;
	}
#ifdef SNMP_SUPPORT
	spin_lock(&cmd->se_lun->lun_sep_lock);
	if (cmd->se_lun->lun_sep)
		cmd->se_lun->lun_sep->sep_stats.cmd_pdus++;
	spin_unlock(&cmd->se_lun->lun_sep_lock);
#endif  

	switch (non_data_cdb) {
	case 0:
		DEBUG_CDB_H("Set cdb[0]: 0x%02x to "
				"SCF_SCSI_DATA_SG_IO_CDB\n", cdb[0]);
		cmd->se_cmd_flags |= SCF_SCSI_DATA_SG_IO_CDB;

		T_TASK(cmd)->t_task_lba = (cmd->transport_get_long_lba) ?
			cmd->transport_get_long_lba(cdb) :
			cmd->transport_get_lba(cdb);

		break;
	case 1:
		DEBUG_CDB_H("Set cdb[0]: 0x%02x to"
				" SCF_SCSI_CONTROL_SG_IO_CDB\n", cdb[0]);
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_SG_IO_CDB;
		cmd->transport_cdb_transform =
				&transport_process_control_sg_transform;
		break;
	case 2:
		DEBUG_CDB_H("Set cdb[0]: 0x%02x to "
				"SCF_SCSI_CONTROL_NONSG_IO_CDB\n", cdb[0]);
		cmd->se_cmd_flags |= SCF_SCSI_CONTROL_NONSG_IO_CDB;
		cmd->transport_cdb_transform =
				&transport_process_control_nonsg_transform;
		break;
	case 3:
		DEBUG_CDB_H("Set cdb[0]: 0x%02x to "
				"SCF_SCSI_NON_DATA_CDB\n", cdb[0]);
		cmd->se_cmd_flags |= SCF_SCSI_NON_DATA_CDB;
		cmd->transport_cdb_transform =
				&transport_process_non_data_transform;
		break;
	case 4:
		DEBUG_CDB_H("Set cdb[0]: 0x%02x to"
				" SCF_SCSI_UNSUPPORTED_CDB\n", cdb[0]);
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = UNSUPPORTED_SCSI_OPCODE;
		return -2;
	case 5:
		DEBUG_CDB_H("Set cdb[0]: 0x%02x to"
				" SCF_SCSI_RESERVATION_CONFLICT\n", cdb[0]);
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->se_cmd_flags |= SCF_SCSI_RESERVATION_CONFLICT;
		cmd->scsi_status = SAM_STAT_RESERVATION_CONFLICT;
		 
		if (SE_SESS(cmd) &&
		    DEV_ATTRIB(cmd->se_dev)->emulate_ua_intlck_ctrl == 2)
			core_scsi3_ua_allocate(SE_SESS(cmd)->se_node_acl,
				cmd->orig_fe_lun, 0x2C,
				ASCQ_2CH_PREVIOUS_RESERVATION_CONFLICT_STATUS);
		return -2;
	case 6:
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = INVALID_CDB_FIELD;
		return -2;
	case 7:
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = ILLEGAL_REQUEST;
		return -2;
	case 8:
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = CHECK_CONDITION_UNIT_ATTENTION;
		return -2;
	case 9:
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = CHECK_CONDITION_NOT_READY;
		return -2;
	default:
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = UNSUPPORTED_SCSI_OPCODE;
		return -2;
	}

	return 0;
}
EXPORT_SYMBOL(transport_generic_allocate_tasks);

int transport_generic_handle_cdb(
	se_cmd_t *cmd)
{
	if (!SE_LUN(cmd)) {
		printk(KERN_ERR "SE_LUN(cmd) is NULL\n");
		return -1;
	}

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	transport_add_cmd_to_queue(cmd, TRANSPORT_NEW_CMD);
#else
	cmd->transport_add_cmd_to_queue(cmd, TRANSPORT_NEW_CMD);
#endif
	return 0;
}
EXPORT_SYMBOL(transport_generic_handle_cdb);

int transport_generic_handle_data(
	se_cmd_t *cmd)
{
	 
	while (atomic_read(&T_TASK(cmd)->t_transport_active)) {
		msleep_interruptible(10);
		if (signal_pending(current))
			return -1;
	}
	 
	if (transport_check_aborted_status(cmd, 1) != 0)
		return 0;

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	transport_add_cmd_to_queue(cmd, TRANSPORT_PROCESS_WRITE);
#else
	cmd->transport_add_cmd_to_queue(cmd, TRANSPORT_PROCESS_WRITE);
#endif
	return 0;
}
EXPORT_SYMBOL(transport_generic_handle_data);

int transport_generic_handle_tmr(
	se_cmd_t *cmd)
{
	 
	cmd->transport_wait_for_tasks = &transport_generic_wait_for_tasks;
#ifdef MY_ABC_HERE
	transport_device_setup_cmd(cmd);
#else
	CMD_ORIG_OBJ_API(cmd)->transport_setup_cmd(cmd->se_orig_obj_ptr, cmd);
#endif

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	transport_add_cmd_to_queue(cmd, TRANSPORT_PROCESS_TMR);
#else
	cmd->transport_add_cmd_to_queue(cmd, TRANSPORT_PROCESS_TMR);
#endif
	return 0;
}
EXPORT_SYMBOL(transport_generic_handle_tmr);

#ifdef MY_ABC_HERE
int transport_stop_tasks_for_cmd(se_cmd_t *cmd)
#else
void transport_stop_tasks_for_cmd(se_cmd_t *cmd)
#endif
{
	se_task_t *task, *task_tmp;
	unsigned long flags;
#ifdef MY_ABC_HERE
	int ret = 0;
#endif

	DEBUG_TS("ITT[0x%08x] - Stopping tasks\n",
		CMD_TFO(cmd)->get_task_tag(cmd));

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	list_for_each_entry_safe(task, task_tmp,
				&T_TASK(cmd)->t_task_list, t_list) {
		DEBUG_TS("task_no[%d] - Processing task %p\n",
				task->task_no, task);
		 
		if (!atomic_read(&task->task_sent) &&
		    !atomic_read(&task->task_active)) {
			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock,
					flags);
			transport_remove_task_from_execute_queue(task,
					task->se_dev);

			DEBUG_TS("task_no[%d] - Removed from execute queue\n",
				task->task_no);
			spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
			continue;
		}

		if (atomic_read(&task->task_active)) {
			atomic_set(&task->task_stop, 1);
			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock,
					flags);

			DEBUG_TS("task_no[%d] - Waiting to complete\n",
				task->task_no);
			wait_for_completion(&task->task_stop_comp);
			DEBUG_TS("task_no[%d] - Stopped successfully\n",
				task->task_no);

			spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
			atomic_dec(&T_TASK(cmd)->t_task_cdbs_left);

			atomic_set(&task->task_active, 0);
			atomic_set(&task->task_stop, 0);
		} else {
			DEBUG_TS("task_no[%d] - Did nothing\n", task->task_no);
#ifdef MY_ABC_HERE
			ret++;
#endif
		}

		__transport_stop_task_timer(task, &flags);
	}
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

#ifdef MY_ABC_HERE
	return ret;
#endif
}

static void transport_failure_reset_queue_depth(se_device_t *dev)
{
	unsigned long flags;

	spin_lock_irqsave(&SE_HBA(dev)->hba_queue_lock, flags);;
	atomic_inc(&dev->depth_left);
	atomic_inc(&SE_HBA(dev)->left_queue_depth);
	spin_unlock_irqrestore(&SE_HBA(dev)->hba_queue_lock, flags);
}

void transport_generic_request_failure(
	se_cmd_t *cmd,
	se_device_t *dev,
	int complete,
	int sc)
{
	DEBUG_GRF("-----[ Storage Engine Exception for cmd: %p ITT: 0x%08x"
		" CDB: 0x%02x\n", cmd, CMD_TFO(cmd)->get_task_tag(cmd),
		T_TASK(cmd)->t_task_cdb[0]);
#ifndef MY_ABC_HERE
	DEBUG_GRF("-----[ se_obj_api: %p se_obj_ptr: %p\n", cmd->se_obj_api,
		cmd->se_obj_ptr);
	DEBUG_GRF("-----[ se_orig_obj_api: %p se_orig_obj_ptr: %p\n",
		cmd->se_orig_obj_api, cmd->se_orig_obj_ptr);
#endif
	DEBUG_GRF("-----[ i_state: %d t_state/def_t_state:"
		" %d/%d transport_error_status: %d\n",
		CMD_TFO(cmd)->get_cmd_state(cmd),
		cmd->t_state, cmd->deferred_t_state,
		cmd->transport_error_status);
	DEBUG_GRF("-----[ t_task_cdbs: %d t_task_cdbs_left: %d"
		" t_task_cdbs_sent: %d t_task_cdbs_ex_left: %d --"
		" t_transport_active: %d t_transport_stop: %d"
		" t_transport_sent: %d\n", T_TASK(cmd)->t_task_cdbs,
		atomic_read(&T_TASK(cmd)->t_task_cdbs_left),
		atomic_read(&T_TASK(cmd)->t_task_cdbs_sent),
		atomic_read(&T_TASK(cmd)->t_task_cdbs_ex_left),
		atomic_read(&T_TASK(cmd)->t_transport_active),
		atomic_read(&T_TASK(cmd)->t_transport_stop),
		atomic_read(&T_TASK(cmd)->t_transport_sent));

	transport_stop_all_task_timers(cmd);

	if (dev)
		transport_failure_reset_queue_depth(dev);
	 
	if (cmd->se_dev->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
		transport_complete_task_attr(cmd);

	if (complete) {
		transport_direct_request_timeout(cmd);
		cmd->transport_error_status = PYX_TRANSPORT_LU_COMM_FAILURE;
	}

	switch (cmd->transport_error_status) {
	case PYX_TRANSPORT_UNKNOWN_SAM_OPCODE:
		cmd->scsi_sense_reason = UNSUPPORTED_SCSI_OPCODE;
		break;
	case PYX_TRANSPORT_REQ_TOO_MANY_SECTORS:
		cmd->scsi_sense_reason = SECTOR_COUNT_TOO_MANY;
		break;
	case PYX_TRANSPORT_INVALID_CDB_FIELD:
		cmd->scsi_sense_reason = INVALID_CDB_FIELD;
		break;
	case PYX_TRANSPORT_INVALID_PARAMETER_LIST:
		cmd->scsi_sense_reason = INVALID_PARAMETER_LIST;
		break;
	case PYX_TRANSPORT_OUT_OF_MEMORY_RESOURCES:
		if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH)) {
			if (!sc)
				transport_new_cmd_failure(cmd);
			 
			CMD_TFO(cmd)->fall_back_to_erl0(cmd->se_sess);
			CMD_TFO(cmd)->stop_session(cmd->se_sess, 0, 0);

			goto check_stop;
		} else {
			cmd->scsi_sense_reason =
				LOGICAL_UNIT_COMMUNICATION_FAILURE;
		}
		break;
	case PYX_TRANSPORT_LU_COMM_FAILURE:
		cmd->scsi_sense_reason = LOGICAL_UNIT_COMMUNICATION_FAILURE;
		break;
	case PYX_TRANSPORT_UNKNOWN_MODE_PAGE:
		cmd->scsi_sense_reason = UNKNOWN_MODE_PAGE;
		break;
	case PYX_TRANSPORT_WRITE_PROTECTED:
		cmd->scsi_sense_reason = WRITE_PROTECTED;
		break;
	case PYX_TRANSPORT_RESERVATION_CONFLICT:
		 
		cmd->scsi_status = SAM_STAT_RESERVATION_CONFLICT;
		 
		if (SE_SESS(cmd) &&
		    DEV_ATTRIB(cmd->se_dev)->emulate_ua_intlck_ctrl == 2)
			core_scsi3_ua_allocate(SE_SESS(cmd)->se_node_acl,
				cmd->orig_fe_lun, 0x2C,
				ASCQ_2CH_PREVIOUS_RESERVATION_CONFLICT_STATUS);

		if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH))
			CMD_TFO(cmd)->queue_status(cmd);

		goto check_stop;

	case PYX_TRANSPORT_ILLEGAL_REQUEST:
		cmd->scsi_sense_reason = ILLEGAL_REQUEST;
		break;
	default:
		printk(KERN_ERR "Unknown transport error for CDB 0x%02x: %d\n",
			T_TASK(cmd)->t_task_cdb[0],
			cmd->transport_error_status);
		cmd->scsi_sense_reason = UNSUPPORTED_SCSI_OPCODE;
		break;
	}

	if (!sc)
		transport_new_cmd_failure(cmd);
	else
		transport_send_check_condition_and_sense(cmd,
			cmd->scsi_sense_reason, 0);
check_stop:
	transport_lun_remove_cmd(cmd);
	if (!(transport_cmd_check_stop_to_fabric(cmd)))
		transport_passthrough_check_stop(cmd);
}

void transport_direct_request_timeout(se_cmd_t *cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (!(atomic_read(&T_TASK(cmd)->t_transport_timeout))) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return;
	}
	if (atomic_read(&T_TASK(cmd)->t_task_cdbs_timeout_left)) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return;
	}

	atomic_sub(atomic_read(&T_TASK(cmd)->t_transport_timeout),
		   &T_TASK(cmd)->t_se_count);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
}

void transport_generic_request_timeout(se_cmd_t *cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (atomic_read(&T_TASK(cmd)->t_transport_timeout) > 1) {
		int tmp = (atomic_read(&T_TASK(cmd)->t_transport_timeout) - 1);

		atomic_sub(tmp, &T_TASK(cmd)->t_se_count);
	}
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	transport_generic_remove(cmd, 0, 0);
}

#ifdef iscsi_linux_calculate_map_segment_DEBUG
#define DEBUG_MAP_SEGMENTS(buf...) PYXPRINT(buf)
#else
#define DEBUG_MAP_SEGMENTS(buf...)
#endif

static inline void transport_calculate_map_segment(
	u32 *data_length,
	se_offset_map_t *lm)
{
	u32 sg_offset = 0;
	se_mem_t *se_mem = lm->map_se_mem;

	DEBUG_MAP_SEGMENTS(" START Mapping se_mem: %p, Length: %d"
		"  Remaining iSCSI Data: %u\n", se_mem, se_mem->se_len,
		*data_length);
	 
	if (!lm->map_reset) {
		lm->iovec_length = (lm->sg_length > PAGE_SIZE) ?
					PAGE_SIZE : lm->sg_length;
		if (*data_length < lm->iovec_length) {
			DEBUG_MAP_SEGMENTS("LINUX_MAP: Reset lm->iovec_length"
				" to %d\n", *data_length);

			lm->iovec_length = *data_length;
		}
		lm->iovec_base = page_address(lm->sg_page) + sg_offset;

		DEBUG_MAP_SEGMENTS("LINUX_MAP: Set lm->iovec_base to %p from"
			" lm->sg_page: %p\n", lm->iovec_base, lm->sg_page);
		return;
	}

	lm->map_reset = 0;
	sg_offset = se_mem->se_off;
	lm->sg_page = se_mem->se_page;
	lm->sg_length = se_mem->se_len;

	DEBUG_MAP_SEGMENTS("LINUX_MAP1[%p]: Starting to se_mem->se_len: %u,"
		" se_mem->se_off: %u, se_mem->se_page: %p\n", se_mem,
		se_mem->se_len, se_mem->se_off, se_mem->se_page);;
	 
recalc:
	lm->iovec_length = (lm->sg_length > (PAGE_SIZE - sg_offset)) ?
			   (PAGE_SIZE - sg_offset) : lm->sg_length;

	DEBUG_MAP_SEGMENTS("LINUX_MAP: lm->iovec_length: %u, lm->sg_length: %u,"
		" sg_offset: %u\n", lm->iovec_length, lm->sg_length, sg_offset);
	 
	if (!lm->current_offset) {
		lm->iovec_base = page_address(lm->sg_page) + sg_offset;

		if (*data_length < lm->iovec_length) {
			DEBUG_MAP_SEGMENTS("LINUX_MAP1[%p]: Reset"
				" lm->iovec_length to %d\n", se_mem,
				*data_length);
			lm->iovec_length = *data_length;
		}

		DEBUG_MAP_SEGMENTS("LINUX_MAP2[%p]: No current_offset,"
			" set iovec_base to %p and set Current Page to %p\n",
			se_mem, lm->iovec_base, lm->sg_page);

		return;
	}

	if (lm->current_offset >= lm->iovec_length) {
		DEBUG_MAP_SEGMENTS("LINUX_MAP3[%p]: Next Page:"
			" lm->current_offset: %u, iovec_length: %u"
			" sg_offset: %u\n", se_mem, lm->current_offset,
			lm->iovec_length, sg_offset);

		lm->current_offset -= lm->iovec_length;
		lm->sg_length -= lm->iovec_length;
		lm->sg_page++;
		sg_offset = 0;

		DEBUG_MAP_SEGMENTS("LINUX_MAP3[%p]: ** Skipping to Next Page,"
			" updated values: lm->current_offset: %u\n", se_mem,
			lm->current_offset);

		goto recalc;
	}

	lm->iovec_base = page_address(lm->sg_page);

	DEBUG_MAP_SEGMENTS("LINUX_MAP4[%p]: Set lm->iovec_base to %p\n", se_mem,
			lm->iovec_base);

	lm->iovec_base += sg_offset;
	lm->iovec_base += lm->current_offset;
	DEBUG_MAP_SEGMENTS("****** the OLD lm->iovec_length: %u lm->sg_length:"
		" %u\n", lm->iovec_length, lm->sg_length);

	if ((lm->iovec_length - lm->current_offset) < *data_length)
		lm->iovec_length -= lm->current_offset;
	else
		lm->iovec_length = *data_length;

	if ((lm->sg_length - lm->current_offset) < *data_length)
		lm->sg_length -= lm->current_offset;
	else
		lm->sg_length = *data_length;

	lm->current_offset = 0;

	DEBUG_MAP_SEGMENTS("****** the NEW lm->iovec_length %u lm->sg_length:"
		" %u\n", lm->iovec_length, lm->sg_length);
}

#ifdef iscsi_linux_get_iscsi_offset_DEBUG
#define DEBUG_GET_ISCSI_OFFSET(buf...) PYXPRINT(buf)
#else
#define DEBUG_GET_ISCSI_OFFSET(buf...)
#endif

static int transport_get_iscsi_offset(
	se_offset_map_t *lmap,
	se_unmap_sg_t *usg)
{
	u32 current_length = 0, current_iscsi_offset = lmap->iscsi_offset;
	u32 total_offset = 0;
	se_cmd_t *cmd = usg->se_cmd;
	se_mem_t *se_mem;

	list_for_each_entry(se_mem, T_TASK(cmd)->t_mem_list, se_list)
		break;

	if (!se_mem) {
		printk(KERN_ERR "Unable to locate se_mem from"
				" T_TASK(cmd)->t_mem_list\n");
		return -1;
	}

	while (lmap->iscsi_offset != current_length) {
		 
		total_offset += se_mem->se_len;

		DEBUG_GET_ISCSI_OFFSET("ISCSI_OFFSET: current_length: %u,"
			" total_offset: %u, sg->length: %u\n",
			current_length, total_offset, se_mem->se_len);

		if (total_offset > lmap->iscsi_offset) {
			current_length += current_iscsi_offset;
			lmap->orig_offset = lmap->current_offset =
				usg->t_offset = current_iscsi_offset;
			DEBUG_GET_ISCSI_OFFSET("ISCSI_OFFSET: Within Current"
				" se_mem_t: %p, current_length incremented to"
				" %u\n", se_mem, current_length);
		} else {
			current_length += se_mem->se_len;
			current_iscsi_offset -= se_mem->se_len;

			DEBUG_GET_ISCSI_OFFSET("ISCSI_OFFSET: Outside of"
				" Current se_mem: %p, current_length"
				" incremented to %u and current_iscsi_offset"
				" decremented to %u\n", se_mem, current_length,
				current_iscsi_offset);

			list_for_each_entry_continue(se_mem,
					T_TASK(cmd)->t_mem_list, se_list)
				break;

			if (!se_mem) {
				printk(KERN_ERR "Unable to locate se_mem_t\n");
				return -1;
			}
		}
	}
	lmap->map_orig_se_mem = se_mem;
	usg->cur_se_mem = se_mem;

	return 0;
}

#ifdef iscsi_OS_set_SG_iovec_ptrs_DEBUG
#define DEBUG_IOVEC_SCATTERLISTS(buf...) PYXPRINT(buf)

static void iscsi_check_iovec_map(
	u32 iovec_count,
	u32 map_length,
	se_map_sg_t *map_sg,
	se_unmap_sg_t *unmap_sg)
{
	u32 i, iovec_map_length = 0;
	se_cmd_t *cmd = map_sg->se_cmd;
	struct iovec *iov = map_sg->iov;
	se_mem_t *se_mem;

	for (i = 0; i < iovec_count; i++)
		iovec_map_length += iov[i].iov_len;

	if (iovec_map_length == map_length)
		return;

	printk(KERN_INFO "Calculated iovec_map_length: %u does not match passed"
		" map_length: %u\n", iovec_map_length, map_length);
	printk(KERN_INFO "ITT: 0x%08x data_length: %u data_direction %d\n",
		CMD_TFO(cmd)->get_task_tag(cmd), cmd->data_length,
		cmd->data_direction);

	iovec_map_length = 0;

	for (i = 0; i < iovec_count; i++) {
		printk(KERN_INFO "iov[%d].iov_[base,len]: %p / %u bytes------"
			"-->\n", i, iov[i].iov_base, iov[i].iov_len);

		printk(KERN_INFO "iovec_map_length from %u to %u\n",
			iovec_map_length, iovec_map_length + iov[i].iov_len);
		iovec_map_length += iov[i].iov_len;

		printk(KERN_INFO "XXXX_map_length from %u to %u\n", map_length,
				(map_length - iov[i].iov_len));
		map_length -= iov[i].iov_len;
	}

	list_for_each_entry(se_mem, T_TASK(cmd)->t_mem_list, se_list) {
		printk(KERN_INFO "se_mem[%p]: offset: %u length: %u\n",
			se_mem, se_mem->se_off, se_mem->se_len);
	}

	BUG();
}

#else
#define DEBUG_IOVEC_SCATTERLISTS(buf...)
#define iscsi_check_iovec_map(a, b, c, d)
#endif

int transport_generic_set_iovec_ptrs(
	se_map_sg_t *map_sg,
	se_unmap_sg_t *unmap_sg)
{
	u32 i = 0  , j = 0  ;
#ifdef iscsi_OS_set_SG_iovec_ptrs_DEBUG
	u32 orig_map_length = map_sg->data_length;
#endif
	se_cmd_t *cmd = map_sg->se_cmd;
	se_offset_map_t *lmap = &unmap_sg->lmap;
	struct iovec *iov = map_sg->iov;

	if (!T_TASK(cmd)->t_task_se_num) {
		DEBUG_IOVEC_SCATTERLISTS("ITT: 0x%08x No se_mem_t elements"
			" present\n", CMD_TFO(cmd)->get_task_tag(cmd));
		iov[0].iov_base = (unsigned char *) T_TASK(cmd)->t_task_buf +
							map_sg->data_offset;
		iov[0].iov_len  = map_sg->data_length;
		return 1;
	}

	lmap->map_reset = 1;

	DEBUG_IOVEC_SCATTERLISTS("[-------------------] ITT: 0x%08x OS"
		" Independent Network POSIX defined iovectors to SE Memory"
		" [-------------------]\n\n", CMD_TFO(cmd)->get_task_tag(cmd));

	lmap->iscsi_offset = map_sg->data_offset;
	if (map_sg->map_flags & MAP_SG_KMAP) {
		unmap_sg->se_cmd = map_sg->se_cmd;
		transport_get_iscsi_offset(lmap, unmap_sg);
		unmap_sg->data_length = map_sg->data_length;
	} else {
		lmap->current_offset = lmap->orig_offset;
	}
	lmap->map_se_mem = lmap->map_orig_se_mem;

	DEBUG_IOVEC_SCATTERLISTS("OS_IOVEC: Total map_sg->data_length: %d,"
		" lmap->iscsi_offset: %d, cmd->orig_iov_data_count: %d\n",
		map_sg->data_length, lmap->iscsi_offset,
		cmd->orig_iov_data_count);

	while (map_sg->data_length) {
		 
		transport_calculate_map_segment(&map_sg->data_length, lmap);

		iov[i].iov_base = lmap->iovec_base;
		iov[i].iov_len = lmap->iovec_length;

		map_sg->data_length -= iov[i].iov_len;
		lmap->sg_length -= iov[i].iov_len;

		DEBUG_IOVEC_SCATTERLISTS("OS_IOVEC: iov[%u].iov_len: %u\n",
				i, iov[i].iov_len);
		DEBUG_IOVEC_SCATTERLISTS("OS_IOVEC: lmap->sg_length: from %u"
			" to %u\n", lmap->sg_length + iov[i].iov_len,
				lmap->sg_length);
		DEBUG_IOVEC_SCATTERLISTS("OS_IOVEC: Changed total"
			" map_sg->data_length from %u to %u\n",
			map_sg->data_length + iov[i].iov_len,
			map_sg->data_length);

		if ((++i + 1) > cmd->orig_iov_data_count) {
			printk(KERN_ERR "Current iovec count %u is greater than"
				" se_cmd_t->orig_data_iov_count %u, cannot"
				" continue.\n", i+1, cmd->orig_iov_data_count);
			return -1;
		}

		if (!lmap->sg_length || !map_sg->data_length) {
			list_for_each_entry(lmap->map_se_mem,
					&lmap->map_se_mem->se_list, se_list)
				break;

			if (!lmap->map_se_mem) {
				printk(KERN_ERR "Unable to locate next"
					" lmap->map_se_mem_t entry\n");
				return -1;
			}
			j++;

			lmap->sg_page = NULL;
			lmap->map_reset = 1;

			DEBUG_IOVEC_SCATTERLISTS("OS_IOVEC: Done with current"
				" scatterlist, incremented Generic scatterlist"
				" Counter to %d and reset = 1\n", j);
		} else
			lmap->sg_page++;
	}

	unmap_sg->sg_count = j;

	iscsi_check_iovec_map(i, orig_map_length, map_sg, unmap_sg);

	return i;
}
EXPORT_SYMBOL(transport_generic_set_iovec_ptrs);

int transport_generic_allocate_buf(
	se_cmd_t *cmd,
	u32 data_length,
	u32 dma_size)
{
	unsigned char *buf;

	buf = kzalloc(data_length, GFP_KERNEL);
	if (!(buf)) {
		printk(KERN_ERR "Unable to allocate memory for buffer\n");
		return -1;
	}

	T_TASK(cmd)->t_task_se_num = 0;
	T_TASK(cmd)->t_task_buf = buf;

	return 0;
}

static int transport_generic_allocate_none(
	se_cmd_t *cmd,
	u32 data_length,
	u32 dma_size)
{
	return 0;
}

static void transport_generic_map_SG_segments(se_unmap_sg_t *unmap_sg)
{
	u32 i = 0;
	se_cmd_t *cmd = unmap_sg->se_cmd;
	se_mem_t *se_mem = unmap_sg->cur_se_mem;

	if (!(T_TASK(cmd)->t_task_se_num))
		return;

	list_for_each_entry_continue(se_mem, T_TASK(cmd)->t_mem_list, se_list) {
		kmap(se_mem->se_page);

		if (++i == unmap_sg->sg_count)
			break;
	}
}

static void transport_generic_unmap_SG_segments(se_unmap_sg_t *unmap_sg)
{
	u32 i = 0;
	se_cmd_t *cmd = unmap_sg->se_cmd;
	se_mem_t *se_mem = unmap_sg->cur_se_mem;

	if (!(T_TASK(cmd)->t_task_se_num))
		return;

	list_for_each_entry_continue(se_mem, T_TASK(cmd)->t_mem_list, se_list) {
		kunmap(se_mem->se_page);

		if (++i == unmap_sg->sg_count)
			break;
	}

	return;
}

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

void transport_set_supported_SAM_opcode(se_cmd_t *se_cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(se_cmd)->t_state_lock, flags);
	se_cmd->se_cmd_flags |= SCF_SUPPORTED_SAM_OPCODE;
	spin_unlock_irqrestore(&T_TASK(se_cmd)->t_state_lock, flags);
}

void transport_task_timeout_handler(unsigned long data)
{
	se_task_t *task = (se_task_t *)data;
	se_cmd_t *cmd = TASK_CMD(task);
	unsigned long flags;

	DEBUG_TT("transport task timeout fired! task: %p cmd: %p\n", task, cmd);

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (task->task_flags & TF_STOP) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return;
	}
	task->task_flags &= ~TF_RUNNING;

	if (!(atomic_read(&task->task_active))) {
		DEBUG_TT("transport task: %p cmd: %p timeout task_active"
				" == 0\n", task, cmd);
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return;
	}

	atomic_inc(&T_TASK(cmd)->t_se_count);
	atomic_inc(&T_TASK(cmd)->t_transport_timeout);
	T_TASK(cmd)->t_tasks_failed = 1;

	atomic_set(&task->task_timeout, 1);
	task->task_error_status = PYX_TRANSPORT_TASK_TIMEOUT;
	task->task_scsi_status = 1;

	if (atomic_read(&task->task_stop)) {
		DEBUG_TT("transport task: %p cmd: %p timeout task_stop"
				" == 1\n", task, cmd);
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		complete(&task->task_stop_comp);
		return;
	}

	if (!(atomic_dec_and_test(&T_TASK(cmd)->t_task_cdbs_left))) {
		DEBUG_TT("transport task: %p cmd: %p timeout non zero"
				" t_task_cdbs_left\n", task, cmd);
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return;
	}
	DEBUG_TT("transport task: %p cmd: %p timeout ZERO t_task_cdbs_left\n",
			task, cmd);

	cmd->t_state = TRANSPORT_COMPLETE_FAILURE;
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	transport_add_cmd_to_queue(cmd, TRANSPORT_COMPLETE_FAILURE);
#else
	cmd->transport_add_cmd_to_queue(cmd, TRANSPORT_COMPLETE_FAILURE);
#endif
}

u32 transport_get_default_task_timeout(se_device_t *dev)
{
	if (TRANSPORT(dev)->get_device_type(dev) == TYPE_DISK)
		return TRANSPORT_TIMEOUT_TYPE_DISK;

	if (TRANSPORT(dev)->get_device_type(dev) == TYPE_ROM)
		return TRANSPORT_TIMEOUT_TYPE_ROM;

	if (TRANSPORT(dev)->get_device_type(dev) == TYPE_TAPE)
		return TRANSPORT_TIMEOUT_TYPE_TAPE;

	return TRANSPORT_TIMEOUT_TYPE_OTHER;
}
EXPORT_SYMBOL(transport_get_default_task_timeout);

void transport_start_task_timer(se_task_t *task)
{
#ifdef MY_ABC_HERE
	struct se_device_s *dev = task->se_obj_ptr;
#endif
	int timeout;

	if (task->task_flags & TF_RUNNING)
		return;
	 
#ifdef MY_ABC_HERE
	timeout = DEV_ATTRIB(dev)->task_timeout;
#else
	timeout = task->se_obj_api->get_task_timeout(task->se_obj_ptr);
#endif
	if (!(timeout))
		return;

	init_timer(&task->task_timer);
	task->task_timer.expires = (get_jiffies_64() + timeout * HZ);
	task->task_timer.data = (unsigned long) task;
	task->task_timer.function = transport_task_timeout_handler;

	task->task_flags |= TF_RUNNING;
	add_timer(&task->task_timer);
#if 0
	printk(KERN_INFO "Starting task timer for cmd: %p task: %p seconds:"
		" %d\n", task->task_se_cmd, task, timeout);
#endif
}

void __transport_stop_task_timer(se_task_t *task, unsigned long *flags)
{
	se_cmd_t *cmd = TASK_CMD(task);

	if (!(task->task_flags & TF_RUNNING))
		return;

	task->task_flags |= TF_STOP;
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, *flags);

	del_timer_sync(&task->task_timer);

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, *flags);
	task->task_flags &= ~TF_RUNNING;
	task->task_flags &= ~TF_STOP;
}

void transport_stop_task_timer(se_task_t *task)
{
	se_cmd_t *cmd = TASK_CMD(task);
	unsigned long flags;
#if 0
	printk(KERN_INFO "Stopping task timer for cmd: %p task: %p\n",
			cmd, task);
#endif
	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (!(task->task_flags & TF_RUNNING)) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return;
	}
	task->task_flags |= TF_STOP;
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	del_timer_sync(&task->task_timer);

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	task->task_flags &= ~TF_RUNNING;
	task->task_flags &= ~TF_STOP;
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
}

void transport_stop_all_task_timers(se_cmd_t *cmd)
{
	se_task_t *task = NULL, *task_tmp;
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	list_for_each_entry_safe(task, task_tmp,
				&T_TASK(cmd)->t_task_list, t_list)
		__transport_stop_task_timer(task, &flags);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
}

static inline int transport_tcq_window_closed(se_device_t *dev)
{
	if (dev->dev_tcq_window_closed++ <
			PYX_TRANSPORT_WINDOW_CLOSED_THRESHOLD) {
		msleep(PYX_TRANSPORT_WINDOW_CLOSED_WAIT_SHORT);
	} else
		msleep(PYX_TRANSPORT_WINDOW_CLOSED_WAIT_LONG);

	wake_up_interruptible(&dev->dev_queue_obj->thread_wq);
	return 0;
}

static inline int transport_execute_task_attr(se_cmd_t *cmd)
{
	if (SE_DEV(cmd)->dev_task_attr_type != SAM_TASK_ATTR_EMULATED)
		return 1;
	 
	 if (cmd->sam_task_attr == TASK_ATTR_HOQ) {
		atomic_inc(&SE_DEV(cmd)->dev_hoq_count);
		smp_mb__after_atomic_inc();
		DEBUG_STA("Added HEAD_OF_QUEUE for CDB:"
			" 0x%02x, se_ordered_id: %u\n",
			T_TASK(cmd)->t_task_cdb[0],
			cmd->se_ordered_id);
		return 1;
	} else if (cmd->sam_task_attr == TASK_ATTR_ORDERED) {
		spin_lock(&SE_DEV(cmd)->ordered_cmd_lock);
		list_add_tail(&cmd->se_ordered_list,
				&SE_DEV(cmd)->ordered_cmd_list);
		spin_unlock(&SE_DEV(cmd)->ordered_cmd_lock);

		atomic_inc(&SE_DEV(cmd)->dev_ordered_sync);
		smp_mb__after_atomic_inc();

		DEBUG_STA("Added ORDERED for CDB: 0x%02x to ordered"
				" list, se_ordered_id: %u\n",
				T_TASK(cmd)->t_task_cdb[0],
				cmd->se_ordered_id);
		 
		if (!(atomic_read(&SE_DEV(cmd)->simple_cmds)))
			return 1;
	} else {
		 
		atomic_inc(&SE_DEV(cmd)->simple_cmds);
		smp_mb__after_atomic_inc();
	}
	 
	if (atomic_read(&SE_DEV(cmd)->dev_ordered_sync) != 0) {
		 
		spin_lock(&SE_DEV(cmd)->delayed_cmd_lock);
		cmd->se_cmd_flags |= SCF_DELAYED_CMD_FROM_SAM_ATTR;
		list_add_tail(&cmd->se_delayed_list,
				&SE_DEV(cmd)->delayed_cmd_list);
		spin_unlock(&SE_DEV(cmd)->delayed_cmd_lock);

		DEBUG_STA("Added CDB: 0x%02x Task Attr: 0x%02x to"
			" delayed CMD list, se_ordered_id: %u\n",
			T_TASK(cmd)->t_task_cdb[0], cmd->sam_task_attr,
			cmd->se_ordered_id);
		 
		return 0;
	}
	 
	return 1;
}

int transport_execute_tasks(se_cmd_t *cmd)
{
	int add_tasks;

	if (!(cmd->se_cmd_flags & SCF_SE_DISABLE_ONLINE_CHECK)) {
#ifdef MY_ABC_HERE
		if (dev_obj_check_online(cmd->se_orig_obj_ptr) != 0) {
#else
		if (CMD_ORIG_OBJ_API(cmd)->check_online(
					cmd->se_orig_obj_ptr) != 0) {
#endif
			cmd->transport_error_status =
				PYX_TRANSPORT_LU_COMM_FAILURE;
			transport_generic_request_failure(cmd, NULL, 0, 1);
			return 0;
		}
	}
	 
	if (!(transport_cmd_check_stop(cmd, 0, TRANSPORT_PROCESSING))) {
		 
		add_tasks = transport_execute_task_attr(cmd);
		if (add_tasks == 0)
			goto execute_tasks;
		 
#ifdef MY_ABC_HERE
		transport_add_tasks_from_cmd(cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->add_tasks(cmd->se_orig_obj_ptr, cmd);
#endif
	}
	 
execute_tasks:
#ifdef MY_ABC_HERE
	__transport_execute_tasks(SE_DEV(cmd));
#else
	CMD_ORIG_OBJ_API(cmd)->execute_tasks(cmd->se_orig_obj_ptr);
#endif
	return 0;
}

int __transport_execute_tasks(se_device_t *dev)
{
	int error;
	se_cmd_t *cmd = NULL;
	se_task_t *task;
	unsigned long flags;

check_depth:
	spin_lock_irqsave(&SE_HBA(dev)->hba_queue_lock, flags);
	if (!(atomic_read(&dev->depth_left)) ||
	    !(atomic_read(&SE_HBA(dev)->left_queue_depth))) {
		spin_unlock_irqrestore(&SE_HBA(dev)->hba_queue_lock, flags);
		return transport_tcq_window_closed(dev);
	}
	dev->dev_tcq_window_closed = 0;

	spin_lock(&dev->execute_task_lock);
	task = transport_get_task_from_execute_queue(dev);
	spin_unlock(&dev->execute_task_lock);

	if (!task) {
		spin_unlock_irqrestore(&SE_HBA(dev)->hba_queue_lock, flags);
		return 0;
	}

	atomic_dec(&dev->depth_left);
	atomic_dec(&SE_HBA(dev)->left_queue_depth);
	spin_unlock_irqrestore(&SE_HBA(dev)->hba_queue_lock, flags);

	cmd = TASK_CMD(task);

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	atomic_set(&task->task_active, 1);
	atomic_set(&task->task_sent, 1);
	atomic_inc(&T_TASK(cmd)->t_task_cdbs_sent);

	if (atomic_read(&T_TASK(cmd)->t_task_cdbs_sent) ==
	    T_TASK(cmd)->t_task_cdbs)
		atomic_set(&cmd->transport_sent, 1);

	transport_start_task_timer(task);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
	 
	if (cmd->transport_emulate_cdb) {
		error = cmd->transport_emulate_cdb(cmd);
		if (error != 0) {
			cmd->transport_error_status = error;
			atomic_set(&task->task_active, 0);
			atomic_set(&cmd->transport_sent, 0);
			transport_stop_tasks_for_cmd(cmd);
			transport_generic_request_failure(cmd, dev, 0, 1);
			goto check_depth;
		}
		 
		cmd->scsi_status = SAM_STAT_GOOD;
		task->task_scsi_status = GOOD;
		transport_complete_task(task, 1);
	} else {
		error = TRANSPORT(dev)->do_task(task);
		if (error != 0) {
			cmd->transport_error_status = error;
			atomic_set(&task->task_active, 0);
			atomic_set(&cmd->transport_sent, 0);
			transport_stop_tasks_for_cmd(cmd);
			transport_generic_request_failure(cmd, dev, 0, 1);
		}
	}

	goto check_depth;

	return 0;
}

void transport_new_cmd_failure(se_cmd_t *se_cmd)
{
	unsigned long flags;
	 
	spin_lock_irqsave(&T_TASK(se_cmd)->t_state_lock, flags);
	se_cmd->se_cmd_flags |= SCF_SE_CMD_FAILED;
	se_cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
	spin_unlock_irqrestore(&T_TASK(se_cmd)->t_state_lock, flags);

	CMD_TFO(se_cmd)->new_cmd_failure(se_cmd);
}

#ifndef SYNO_LIO_TRANSPORT_PATCHES
static int transport_generic_map_buffers_to_tasks(se_cmd_t *);
#endif
static void transport_nop_wait_for_tasks(se_cmd_t *, int, int);

static inline u32 transport_get_sectors_6(
	unsigned char *cdb,
	se_cmd_t *cmd,
	int *ret)
{
	se_device_t *dev = SE_LUN(cmd)->se_dev;

	if (!dev)
		goto type_disk;

	if (TRANSPORT(dev)->get_device_type(dev) == TYPE_TAPE)
		return (u32)(cdb[2] << 16) + (cdb[3] << 8) + cdb[4];

type_disk:
	return (u32)cdb[4];
}

static inline u32 transport_get_sectors_10(
	unsigned char *cdb,
	se_cmd_t *cmd,
	int *ret)
{
	se_device_t *dev = SE_LUN(cmd)->se_dev;

	if (!dev)
		goto type_disk;

	if (TRANSPORT(dev)->get_device_type(dev) == TYPE_TAPE) {
		*ret = -1;
		return 0;
	}

type_disk:
	return (u32)(cdb[7] << 8) + cdb[8];
}

static inline u32 transport_get_sectors_12(
	unsigned char *cdb,
	se_cmd_t *cmd,
	int *ret)
{
	se_device_t *dev = SE_LUN(cmd)->se_dev;

	if (!dev)
		goto type_disk;

	if (TRANSPORT(dev)->get_device_type(dev) == TYPE_TAPE) {
		*ret = -1;
		return 0;
	}

type_disk:
	return (u32)(cdb[6] << 24) + (cdb[7] << 16) + (cdb[8] << 8) + cdb[9];
}

static inline u32 transport_get_sectors_16(
	unsigned char *cdb,
	se_cmd_t *cmd,
	int *ret)
{
	se_device_t *dev = SE_LUN(cmd)->se_dev;

	if (!dev)
		goto type_disk;

	if (TRANSPORT(dev)->get_device_type(dev) == TYPE_TAPE)
		return (u32)(cdb[12] << 16) + (cdb[13] << 8) + cdb[14];

type_disk:
	return (u32)(cdb[10] << 24) + (cdb[11] << 16) +
		    (cdb[12] << 8) + cdb[13];
}

static inline u32 transport_get_size(
	u32 sectors,
	unsigned char *cdb,
	se_cmd_t *cmd)
{
#ifdef MY_ABC_HERE
	struct se_device_s *dev = SE_DEV(cmd);

	if (TRANSPORT(dev)->get_device_type(dev) == TYPE_TAPE) {
		if (cdb[1] & 1) {  
			return DEV_ATTRIB(dev)->block_size * sectors;
		} else  
			return sectors;
	}

	return DEV_ATTRIB(dev)->block_size * sectors;
#else
	return CMD_ORIG_OBJ_API(cmd)->get_cdb_size(cmd->se_orig_obj_ptr,
		sectors, cdb);
#endif
}

static inline void transport_get_maps(se_cmd_t *cmd)
{
	cmd->transport_map_SG_segments = &transport_generic_map_SG_segments;
	cmd->transport_unmap_SG_segments = &transport_generic_unmap_SG_segments;
}

unsigned char transport_asciihex_to_binaryhex(unsigned char val[2])
{
	unsigned char result = 0;
	 
	if ((val[0] >= 'a') && (val[0] <= 'f'))
		result = ((val[0] - 'a' + 10) & 0xf) << 4;
	else
		if ((val[0] >= 'A') && (val[0] <= 'F'))
			result = ((val[0] - 'A' + 10) & 0xf) << 4;
		else  
			result = ((val[0] - '0') & 0xf) << 4;
	 
	if ((val[1] >= 'a') && (val[1] <= 'f'))
		result |= ((val[1] - 'a' + 10) & 0xf);
	else
		if ((val[1] >= 'A') && (val[1] <= 'F'))
			result |= ((val[1] - 'A' + 10) & 0xf);
		else  
			result |= ((val[1] - '0') & 0xf);

	return result;
}
EXPORT_SYMBOL(transport_asciihex_to_binaryhex);

extern int transport_generic_emulate_inquiry(
	se_cmd_t *cmd,
	unsigned char type,
	unsigned char *prod,
	unsigned char *version,
	unsigned char *se_location)
{
	se_device_t *dev = SE_DEV(cmd);
	se_lun_t *lun = SE_LUN(cmd);
	se_port_t *port = NULL;
	se_portal_group_t *tpg = NULL;
	t10_alua_lu_gp_member_t *lu_gp_mem;
	t10_alua_tg_pt_gp_t *tg_pt_gp;
	t10_alua_tg_pt_gp_member_t *tg_pt_gp_mem;
	unsigned char *buf = (unsigned char *) T_TASK(cmd)->t_task_buf;
	unsigned char *cdb = T_TASK(cmd)->t_task_cdb;
	unsigned char *iqn_sn, binary, binary_new;
	u32 prod_len, iqn_sn_len, se_location_len;
	u32 unit_serial_len, off = 0;
	int i;
	u16 len = 0, id_len;

	if (!(cdb[1] & 0x1)) {
		 
		if (cmd->data_length < 6) {
			printk(KERN_ERR "SCSI Inquiry payload length: %u"
				" too small for EVPD=0\n", cmd->data_length);
			return -1;
		}
		buf[0] = type;

		if (type == TYPE_TAPE)
			buf[1] = 0x80;
		buf[2]          = TRANSPORT(dev)->get_device_rev(dev);
		 
		if (T10_ALUA(dev->se_sub_dev)->alua_type ==
				SPC3_ALUA_EMULATED) {
			 
			buf[5]	= 0x80;
			 
			port = lun->lun_sep;
			if (!(port))
				goto after_tpgs;
			tg_pt_gp_mem = port->sep_alua_tg_pt_gp_mem;
			if (!(tg_pt_gp_mem))
				goto after_tpgs;

			spin_lock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
			tg_pt_gp = tg_pt_gp_mem->tg_pt_gp;
			if (!(tg_pt_gp)) {
				spin_unlock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
				goto after_tpgs;
			}
			buf[5] |= tg_pt_gp->tg_pt_gp_alua_access_type;
			spin_unlock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
		}
after_tpgs:
		if (cmd->data_length < 8) {
			buf[4] = 1;  
			return 0;
		}

		buf[7]		= 0x32;  
		 
		if (cmd->data_length < 36) {
			buf[4] = 3;  
			return 0;
		}

#ifdef MY_ABC_HERE
		snprintf((unsigned char *)&buf[8], 5, "SYNO");
#else
		snprintf((unsigned char *)&buf[8], 8, "LIO-ORG");
#endif
		snprintf((unsigned char *)&buf[16], 16, "%s", prod);
		snprintf((unsigned char *)&buf[32], 4, "%s", version);
		buf[4] = 31;  
		return 0;
	}
	 
	if (cmd->data_length < 4) {
		printk(KERN_ERR "SCSI Inquiry payload length: %u"
			" too small for EVPD=1\n", cmd->data_length);
		return -1;
	}
	buf[0] = type;

	switch (cdb[2]) {
	case 0x00:  
		buf[1] = 0x00;
		buf[3] = 3;
		if (cmd->data_length < 8)
			return 0;
		buf[4] = 0x0;
		buf[5] = 0x80;
		buf[6] = 0x83;
		len = 3;
		break;
	case 0x80:  
		buf[1] = 0x80;
		if (dev->se_sub_dev->su_dev_flags &
					SDF_EMULATED_VPD_UNIT_SERIAL) {
			unit_serial_len =
				strlen(&DEV_T10_WWN(dev)->unit_serial[0]);
			unit_serial_len++;  

			if (((len + 4) + unit_serial_len) > cmd->data_length) {
				len += unit_serial_len;
				goto set_len;
			}
			len += sprintf((unsigned char *)&buf[4], "%s",
				&DEV_T10_WWN(dev)->unit_serial[0]);
		 } else {
			iqn_sn = transport_get_iqn_sn();
			iqn_sn_len = strlen(iqn_sn);
			iqn_sn_len++;  
			se_location_len = strlen(se_location);
			se_location_len++;  

			if (((len + 4) + (iqn_sn_len + se_location_len)) >
					cmd->data_length) {
				len += (iqn_sn_len + se_location_len);
				goto set_len;
			}
			len += sprintf((unsigned char *)&buf[4], "%s:%s",
				iqn_sn, se_location);
		}
		len++;  
		buf[3] = len;
		break;
	case 0x83:
		 
		buf[1] = 0x83;
		off = 4;
		 
		if (!(dev->se_sub_dev->su_dev_flags &
					SDF_EMULATED_VPD_UNIT_SERIAL))
			goto check_t10_vend_desc;
		if ((off + 20) > cmd->data_length)
			goto check_t10_vend_desc;
		 
		buf[off++] = 0x1;
		 
		buf[off] = 0x00;
		 
		buf[off++] = 0x3;
		off++;
		 
		buf[off++] = 0x10;
		 
		buf[off++] = (0x6 << 4);
		 
		buf[off++] = 0x01;
		buf[off++] = 0x40;
		buf[off] = (0x5 << 4);
		 
		binary = transport_asciihex_to_binaryhex(
					&DEV_T10_WWN(dev)->unit_serial[0]);
		buf[off++] |= (binary & 0xf0) >> 4;
		for (i = 0; i < 24; i += 2) {
			binary_new = transport_asciihex_to_binaryhex(
				&DEV_T10_WWN(dev)->unit_serial[i+2]);
			buf[off] = (binary & 0x0f) << 4;
			buf[off++] |= (binary_new & 0xf0) >> 4;
			binary = binary_new;
		}
		len = 20;
		off = (len + 4);
check_t10_vend_desc:
		 
		id_len = 8;  
		prod_len = 4;  
		prod_len += 8;  
		prod_len += strlen(prod);
		prod_len++;  

		if (dev->se_sub_dev->su_dev_flags &
					SDF_EMULATED_VPD_UNIT_SERIAL) {
			unit_serial_len =
				strlen(&DEV_T10_WWN(dev)->unit_serial[0]);
			unit_serial_len++;  

			if ((len + (id_len + 4) +
			    (prod_len + unit_serial_len)) >
					cmd->data_length) {
				len += (prod_len + unit_serial_len);
				goto check_port;
			}
			id_len += sprintf((unsigned char *)&buf[off+12],
					"%s:%s", prod,
					&DEV_T10_WWN(dev)->unit_serial[0]);
		} else {
			iqn_sn = transport_get_iqn_sn();
			iqn_sn_len = strlen(iqn_sn);
			iqn_sn_len++;  
			se_location_len = strlen(se_location);
			se_location_len++;  

			if ((len + (id_len + 4) + (prod_len + iqn_sn_len +
					se_location_len)) > cmd->data_length) {
				len += (prod_len + iqn_sn_len +
						se_location_len);
				goto check_port;
			}
			id_len += sprintf((unsigned char *)&buf[off+12],
				"%s:%s:%s", prod, iqn_sn, se_location);
		}
		buf[off] = 0x2;  
		buf[off+1] = 0x1;  
		buf[off+2] = 0x0;
#ifdef MY_ABC_HERE
		memcpy((unsigned char *)&buf[off+4], "SYNO", 5);
#else
		memcpy((unsigned char *)&buf[off+4], "LIO-ORG", 8);
#endif
		 
		id_len++;
		 
		buf[off+3] = id_len;
		 
		len += (id_len + 4);
		off += (id_len + 4);
		 
check_port:
		port = lun->lun_sep;
		if (port) {
			t10_alua_lu_gp_t *lu_gp;
			u32 padding, scsi_name_len;
			u16 lu_gp_id = 0;
			u16 tg_pt_gp_id = 0;
			u16 tpgt;

			tpg = port->sep_tpg;
			 
			if (((len + 4) + 8) > cmd->data_length) {
				len += 8;
				goto check_tpgi;
			}
			buf[off] =
				(TPG_TFO(tpg)->get_fabric_proto_ident() << 4);
			buf[off++] |= 0x1;  
			buf[off] = 0x80;  
			 
			buf[off] |= 0x10;
			 
			buf[off++] |= 0x4;
			off++;  
			buf[off++] = 4;  
			 
			off += 2;
			buf[off++] = ((port->sep_rtpi >> 8) & 0xff);
			buf[off++] = (port->sep_rtpi & 0xff);
			len += 8;  
			 
check_tpgi:
			if (T10_ALUA(dev->se_sub_dev)->alua_type !=
					SPC3_ALUA_EMULATED)
				goto check_scsi_name;

			if (((len + 4) + 8) > cmd->data_length) {
				len += 8;
				goto check_lu_gp;
			}
			tg_pt_gp_mem = port->sep_alua_tg_pt_gp_mem;
			if (!(tg_pt_gp_mem))
				goto check_lu_gp;

			spin_lock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
			tg_pt_gp = tg_pt_gp_mem->tg_pt_gp;
			if (!(tg_pt_gp)) {
				spin_unlock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);
				goto check_lu_gp;
			}
			tg_pt_gp_id = tg_pt_gp->tg_pt_gp_id;
			spin_unlock(&tg_pt_gp_mem->tg_pt_gp_mem_lock);

			buf[off] =
				(TPG_TFO(tpg)->get_fabric_proto_ident() << 4);
			buf[off++] |= 0x1;  
			buf[off] = 0x80;  
			 
			buf[off] |= 0x10;
			 
			buf[off++] |= 0x5;
			off++;  
			buf[off++] = 4;  
			off += 2;  
			buf[off++] = ((tg_pt_gp_id >> 8) & 0xff);
			buf[off++] = (tg_pt_gp_id & 0xff);
			len += 8;  
			 
check_lu_gp:
			if (((len + 4) + 8) > cmd->data_length) {
				len += 8;
				goto check_scsi_name;
			}
			lu_gp_mem = dev->dev_alua_lu_gp_mem;
			if (!(lu_gp_mem))
				goto check_scsi_name;

			spin_lock(&lu_gp_mem->lu_gp_mem_lock);
			lu_gp = lu_gp_mem->lu_gp;
			if (!(lu_gp)) {
				spin_unlock(&lu_gp_mem->lu_gp_mem_lock);
				goto check_scsi_name;
			}
			lu_gp_id = lu_gp->lu_gp_id;
			spin_unlock(&lu_gp_mem->lu_gp_mem_lock);

			buf[off++] |= 0x1;  
			 
			buf[off++] |= 0x6;
			off++;  
			buf[off++] = 4;  
			off += 2;  
			buf[off++] = ((lu_gp_id >> 8) & 0xff);
			buf[off++] = (lu_gp_id & 0xff);
			len += 8;  
			 
check_scsi_name:
			scsi_name_len = strlen(TPG_TFO(tpg)->tpg_get_wwn(tpg));
			 
			scsi_name_len += 10;
			 
			padding = ((-scsi_name_len) & 3);
			if (padding != 0)
				scsi_name_len += padding;
			 
			scsi_name_len += 4;

			if (((len + 4) + scsi_name_len) > cmd->data_length) {
				len += scsi_name_len;
				goto set_len;
			}
			buf[off] =
				(TPG_TFO(tpg)->get_fabric_proto_ident() << 4);
			buf[off++] |= 0x3;  
			buf[off] = 0x80;  
			 
			buf[off] |= 0x10;
			 
			buf[off++] |= 0x8;
			off += 2;  
			 
			tpgt = TPG_TFO(tpg)->tpg_get_tag(tpg);
			scsi_name_len = sprintf(&buf[off], "%s,t,0x%04x",
					TPG_TFO(tpg)->tpg_get_wwn(tpg), tpgt);
			scsi_name_len += 1  ;
			 
			if (padding)
				scsi_name_len += padding;

			buf[off-1] = scsi_name_len;
			off += scsi_name_len;
			 
			len += (scsi_name_len + 4);
		}
set_len:
		buf[2] = ((len >> 8) & 0xff);
		buf[3] = (len & 0xff);  
		break;
	default:
		printk(KERN_ERR "Unknown VPD Code: 0x%02x\n", cdb[2]);
		return -1;
	}

	return 0;
}

int transport_generic_emulate_readcapacity(
	se_cmd_t *cmd,
	u32 blocks)
{
	se_device_t *dev = SE_DEV(cmd);
	unsigned char *buf = (unsigned char *) T_TASK(cmd)->t_task_buf;

	buf[0] = (blocks >> 24) & 0xff;
	buf[1] = (blocks >> 16) & 0xff;
	buf[2] = (blocks >> 8) & 0xff;
	buf[3] = blocks & 0xff;
	buf[4] = (DEV_ATTRIB(dev)->block_size >> 24) & 0xff;
	buf[5] = (DEV_ATTRIB(dev)->block_size >> 16) & 0xff;
	buf[6] = (DEV_ATTRIB(dev)->block_size >> 8) & 0xff;
	buf[7] = DEV_ATTRIB(dev)->block_size & 0xff;

	return 0;
}

int transport_generic_emulate_readcapacity_16(
	se_cmd_t *cmd,
	unsigned long long blocks)
{
	se_device_t *dev = SE_DEV(cmd);
	unsigned char *buf = (unsigned char *) T_TASK(cmd)->t_task_buf;

	buf[0] = (blocks >> 56) & 0xff;
	buf[1] = (blocks >> 48) & 0xff;
	buf[2] = (blocks >> 40) & 0xff;
	buf[3] = (blocks >> 32) & 0xff;
	buf[4] = (blocks >> 24) & 0xff;
	buf[5] = (blocks >> 16) & 0xff;
	buf[6] = (blocks >> 8) & 0xff;
	buf[7] = blocks & 0xff;
	buf[8] = (DEV_ATTRIB(dev)->block_size >> 24) & 0xff;
	buf[9] = (DEV_ATTRIB(dev)->block_size >> 16) & 0xff;
	buf[10] = (DEV_ATTRIB(dev)->block_size >> 8) & 0xff;
	buf[11] = DEV_ATTRIB(dev)->block_size & 0xff;

	return 0;
}

static int transport_modesense_rwrecovery(unsigned char *p)
{
	p[0] = 0x01;
	p[1] = 0x0a;

	return 12;
}

static int transport_modesense_control(se_device_t *dev, unsigned char *p)
{
	p[0] = 0x0a;
	p[1] = 0x0a;
	p[2] = 2;
	 
	p[4] = (DEV_ATTRIB(dev)->emulate_ua_intlck_ctrl == 2) ? 0x30 :
	       (DEV_ATTRIB(dev)->emulate_ua_intlck_ctrl == 1) ? 0x20 : 0x00;
	 
	p[5] = (DEV_ATTRIB(dev)->emulate_tas) ? 0x40 : 0x00;
	p[8] = 0xff;
	p[9] = 0xff;
	p[11] = 30;

	return 12;
}

static int transport_modesense_caching(unsigned char *p)
{
	p[0] = 0x08;
	p[1] = 0x12;
#if 0
	p[2] = 0x04;  
#endif
	p[12] = 0x20;  

	return 20;
}

#if 0
static int transport_modesense_devicecaps(unsigned char *p)
{
	p[0] = 0x2a;
	p[1] = 0x0a;

	return 12;
}
#endif

static void transport_modesense_write_protect(
	unsigned char *buf,
	int type)
{
	 
	switch (type) {
	case TYPE_DISK:
	case TYPE_TAPE:
	default:
		buf[0] |= 0x80;  
		break;
	}
}

int transport_generic_emulate_modesense(
	se_cmd_t *cmd,
	unsigned char *cdb,
	unsigned char *rbuf,
	int ten,
	int type)
{
	se_device_t *dev = SE_DEV(cmd);
	int offset = (ten) ? 8 : 4;
	int length = 0;
	unsigned char buf[SE_MODE_PAGE_BUF];

	memset(buf, 0, SE_MODE_PAGE_BUF);

	switch (cdb[2] & 0x3f) {
	case 0x01:
		length = transport_modesense_rwrecovery(&buf[offset]);
		break;
	case 0x08:
		length = transport_modesense_caching(&buf[offset]);
		break;
	case 0x0a:
		length = transport_modesense_control(dev, &buf[offset]);
		break;
#if 0
	case 0x2a:
		length = transport_modesense_devicecaps(&buf[offset]);
		break;
#endif
	case 0x3f:
		length = transport_modesense_rwrecovery(&buf[offset]);
		length += transport_modesense_caching(&buf[offset+length]);
		length += transport_modesense_control(dev, &buf[offset+length]);
#if 0
		length += transport_modesense_devicecaps(&buf[offset+length]);
#endif
		break;
	default:
#ifdef MY_ABC_HERE
		printk(KERN_ERR "iSCSI - Got Unknown Mode Page: 0x%02x\n", cdb[2] & 0x3f);
#else
		printk(KERN_ERR "Got Unknown Mode Page: 0x%02x\n",
				cdb[2] & 0x3f);
#endif
		return PYX_TRANSPORT_UNKNOWN_MODE_PAGE;
	}
	offset += length;

	if (ten) {
		offset -= 2;
		buf[0] = (offset >> 8) & 0xff;
		buf[1] = offset & 0xff;

		if ((SE_LUN(cmd)->lun_access & TRANSPORT_LUNFLAGS_READ_ONLY) ||
		    (cmd->se_deve &&
		    (cmd->se_deve->lun_flags & TRANSPORT_LUNFLAGS_READ_ONLY)))
			transport_modesense_write_protect(&buf[3], type);

		if ((offset + 2) > cmd->data_length)
			offset = cmd->data_length;

	} else {
		offset -= 1;
		buf[0] = offset & 0xff;

		if ((SE_LUN(cmd)->lun_access & TRANSPORT_LUNFLAGS_READ_ONLY) ||
		    (cmd->se_deve &&
		    (cmd->se_deve->lun_flags & TRANSPORT_LUNFLAGS_READ_ONLY)))
			transport_modesense_write_protect(&buf[2], type);

		if ((offset + 1) > cmd->data_length)
			offset = cmd->data_length;
	}
	memcpy(rbuf, buf, offset);

	return 0;
}

int transport_generic_emulate_request_sense(
	se_cmd_t *cmd,
	unsigned char *cdb)
{
	unsigned char *buf = (unsigned char *) T_TASK(cmd)->t_task_buf;
	u8 ua_asc = 0, ua_ascq = 0;

	if (cdb[1] & 0x01) {
		printk(KERN_ERR "REQUEST_SENSE description emulation not"
			" supported\n");
		return PYX_TRANSPORT_INVALID_CDB_FIELD;
	}
	if (!(core_scsi3_ua_clear_for_request_sense(cmd, &ua_asc, &ua_ascq))) {
		 
		buf[0] = 0x70;
		buf[SPC_SENSE_KEY_OFFSET] = UNIT_ATTENTION;
		 
		if (cmd->data_length <= 18) {
			buf[7] = 0x00;
			return 0;
		}
		 
		buf[SPC_ASC_KEY_OFFSET] = ua_asc;
		buf[SPC_ASCQ_KEY_OFFSET] = ua_ascq;
		buf[7] = 0x0A;
	} else {
		 
		buf[0] = 0x70;
		buf[SPC_SENSE_KEY_OFFSET] = NO_SENSE;
		 
		if (cmd->data_length <= 18) {
			buf[7] = 0x00;
			return 0;
		}
		 
		buf[SPC_ASC_KEY_OFFSET] = 0x00;
		buf[7] = 0x0A;
	}

	return 0;
}

int transport_get_sense_data(se_cmd_t *cmd)
{
	unsigned char *buffer = cmd->sense_buffer, *sense_buffer = NULL;
	se_device_t *dev;
	se_task_t *task = NULL, *task_tmp;
	unsigned long flags;
	u32 offset = 0;

	if (!SE_LUN(cmd)) {
		printk(KERN_ERR "SE_LUN(cmd) is NULL\n");
		return -1;
	}
	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (cmd->se_cmd_flags & SCF_SENT_CHECK_CONDITION) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return 0;
	}

	list_for_each_entry_safe(task, task_tmp,
				&T_TASK(cmd)->t_task_list, t_list) {

		if (!task->task_sense)
			continue;

		dev = task->se_dev;
		if (!(dev))
			continue;

		if (!TRANSPORT(dev)->get_sense_buffer) {
			printk(KERN_ERR "TRANSPORT(dev)->get_sense_buffer"
					" is NULL\n");
			continue;
		}

		sense_buffer = TRANSPORT(dev)->get_sense_buffer(task);
		if (!(sense_buffer)) {
			printk(KERN_ERR "ITT[0x%08x]_TASK[%d]: Unable to locate"
				" sense buffer for task with sense\n",
				CMD_TFO(cmd)->get_task_tag(cmd), task->task_no);
			continue;
		}
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

		offset = CMD_TFO(cmd)->set_fabric_sense_len(cmd,
				TRANSPORT_SENSE_BUFFER);

		memcpy((void *)&buffer[offset], (void *)sense_buffer,
				TRANSPORT_SENSE_BUFFER);
		cmd->scsi_status = task->task_scsi_status;
		 
		cmd->scsi_sense_length =
				(TRANSPORT_SENSE_BUFFER + offset);

		printk(KERN_INFO "HBA_[%u]_PLUG[%s]: Set SAM STATUS: 0x%02x"
				" and sense\n",
			dev->se_hba->hba_id, TRANSPORT(dev)->name,
				cmd->scsi_status);
		return 0;
	}
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	return -1;
}

#ifndef SYNO_LIO_TRANSPORT_PATCHES
 
#define SET_GENERIC_TRANSPORT_FUNCTIONS(cmd)				\
do {									\
	cmd->transport_get_task = &transport_generic_get_task;		\
	cmd->transport_map_buffers_to_tasks =				\
			&transport_generic_map_buffers_to_tasks;	\
	cmd->transport_set_iovec_ptrs =					\
			&transport_generic_set_iovec_ptrs;		\
} while (0)
#endif

static int transport_generic_cmd_sequencer(
	se_cmd_t *cmd,
	unsigned char *cdb)
{
	se_device_t *dev = SE_DEV(cmd);
	se_subsystem_dev_t *su_dev = dev->se_sub_dev;
	int ret = 0, sector_ret = 0;
	u32 sectors = 0, size = 0, pr_reg_type = 0;
	u8 alua_ascq = 0;
	 
	if (core_scsi3_ua_check(cmd, cdb) < 0) {
		cmd->transport_wait_for_tasks =
				&transport_nop_wait_for_tasks;
		transport_get_maps(cmd);
		return 8;  
	}
	 
	ret = T10_ALUA(su_dev)->alua_state_check(cmd, cdb, &alua_ascq);
	if (ret != 0) {
		cmd->transport_wait_for_tasks = &transport_nop_wait_for_tasks;
		transport_get_maps(cmd);
		 
		if (ret > 0) {
#if 0
			printk(KERN_INFO "[%s]: ALUA TG Port not available,"
				" SenseKey: NOT_READY, ASC/ASCQ: 0x04/0x%02x\n",
				CMD_TFO(cmd)->get_fabric_name(), alua_ascq);
#endif
			transport_set_sense_codes(cmd, 0x04, alua_ascq);
			return 9;  
		}
		return 6;  
	}
	 
	if (T10_RES(su_dev)->t10_reservation_check(cmd, &pr_reg_type) != 0) {
		if (T10_RES(su_dev)->t10_seq_non_holder(
					cmd, cdb, pr_reg_type) != 0) {
			cmd->transport_wait_for_tasks =
					&transport_nop_wait_for_tasks;
			transport_get_maps(cmd);
			return 5;  
		}
		 
	}

	switch (cdb[0]) {
	case READ_6:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = transport_get_sectors_6(cdb, cmd, &sector_ret);
		if (sector_ret)
			return 4;
		size = transport_get_size(sectors, cdb, cmd);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		cmd->transport_split_cdb = &split_cdb_XX_6;
		cmd->transport_get_lba = &transport_lba_21;
		break;
	case READ_10:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
		if (sector_ret)
			return 4;
		size = transport_get_size(sectors, cdb, cmd);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		cmd->transport_split_cdb = &split_cdb_XX_10;
		cmd->transport_get_lba = &transport_lba_32;
		break;
	case READ_12:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = transport_get_sectors_12(cdb, cmd, &sector_ret);
		if (sector_ret)
			return 4;
		size = transport_get_size(sectors, cdb, cmd);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		cmd->transport_split_cdb = &split_cdb_XX_12;
		cmd->transport_get_lba = &transport_lba_32;
		break;
	case READ_16:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = transport_get_sectors_16(cdb, cmd, &sector_ret);
		if (sector_ret)
			return 4;
		size = transport_get_size(sectors, cdb, cmd);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		cmd->transport_split_cdb = &split_cdb_XX_16;
		cmd->transport_get_long_lba = &transport_lba_64;
		break;
	case WRITE_6:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = transport_get_sectors_6(cdb, cmd, &sector_ret);
		if (sector_ret)
			return 4;
		size = transport_get_size(sectors, cdb, cmd);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		cmd->transport_split_cdb = &split_cdb_XX_6;
		cmd->transport_get_lba = &transport_lba_21;
		break;
	case WRITE_10:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = transport_get_sectors_10(cdb, cmd, &sector_ret);
		if (sector_ret)
			return 4;
		size = transport_get_size(sectors, cdb, cmd);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		cmd->transport_split_cdb = &split_cdb_XX_10;
		cmd->transport_get_lba = &transport_lba_32;
		break;
	case WRITE_12:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = transport_get_sectors_12(cdb, cmd, &sector_ret);
		if (sector_ret)
			return 4;
		size = transport_get_size(sectors, cdb, cmd);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		cmd->transport_split_cdb = &split_cdb_XX_12;
		cmd->transport_get_lba = &transport_lba_32;
		break;
	case WRITE_16:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = transport_get_sectors_16(cdb, cmd, &sector_ret);
		if (sector_ret)
			return 4;
		size = transport_get_size(sectors, cdb, cmd);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		cmd->transport_split_cdb = &split_cdb_XX_16;
		cmd->transport_get_long_lba = &transport_lba_64;
		break;
	case 0xa3:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		if (TRANSPORT(dev)->get_device_type(dev) != TYPE_ROM) {
			 
			if (cdb[1] == MI_REPORT_TARGET_PGS) {
				cmd->transport_emulate_cdb =
				(T10_ALUA(su_dev)->alua_type ==
				 SPC3_ALUA_EMULATED) ?
				&core_scsi3_emulate_report_target_port_groups :
				NULL;
			}
			size = (cdb[6] << 24) | (cdb[7] << 16) |
			       (cdb[8] << 8) | cdb[9];
		} else {
			 
			size = (cdb[8] << 8) + cdb[9];
		}
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case MODE_SELECT:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = cdb[4];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 1;
		break;
	case MODE_SELECT_10:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[7] << 8) + cdb[8];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 1;
		break;
	case MODE_SENSE:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = cdb[4];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case MODE_SENSE_10:
	case GPCMD_READ_BUFFER_CAPACITY:
	case GPCMD_SEND_OPC:
	case LOG_SELECT:
	case LOG_SENSE:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[7] << 8) + cdb[8];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case READ_BLOCK_LIMITS:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = READ_BLOCK_LEN;
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case GPCMD_GET_CONFIGURATION:
	case GPCMD_READ_FORMAT_CAPACITIES:
	case GPCMD_READ_DISC_INFO:
	case GPCMD_READ_TRACK_RZONE_INFO:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[7] << 8) + cdb[8];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 1;
		break;
	case PERSISTENT_RESERVE_IN:
	case PERSISTENT_RESERVE_OUT:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		cmd->transport_emulate_cdb =
			(T10_RES(su_dev)->res_type ==
			 SPC3_PERSISTENT_RESERVATIONS) ?
			&core_scsi3_emulate_pr : NULL;
		size = (cdb[7] << 8) + cdb[8];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case GPCMD_MECHANISM_STATUS:
	case GPCMD_READ_DVD_STRUCTURE:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[8] << 8) + cdb[9];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_SG(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 1;
		break;
	case READ_POSITION:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = READ_POSITION_LEN;
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case 0xa4:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		if (TRANSPORT(dev)->get_device_type(dev) != TYPE_ROM) {
			 
			if (cdb[1] == MO_SET_TARGET_PGS) {
				cmd->transport_emulate_cdb =
				(T10_ALUA(su_dev)->alua_type ==
					SPC3_ALUA_EMULATED) ?
				&core_scsi3_emulate_set_target_port_groups :
				NULL;
			}

			size = (cdb[6] << 24) | (cdb[7] << 16) |
			       (cdb[8] << 8) | cdb[9];
		} else  {
			 
			size = (cdb[8] << 8) + cdb[9];
		}
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case INQUIRY:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[3] << 8) + cdb[4];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		 
		if (SE_DEV(cmd)->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
			cmd->sam_task_attr = TASK_ATTR_HOQ;
		ret = 2;
		break;
	case READ_BUFFER:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[6] << 16) + (cdb[7] << 8) + cdb[8];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case READ_CAPACITY:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = READ_CAP_LEN;
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case READ_MEDIA_SERIAL_NUMBER:
	case SECURITY_PROTOCOL_IN:
	case SECURITY_PROTOCOL_OUT:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[6] << 24) | (cdb[7] << 16) | (cdb[8] << 8) | cdb[9];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case SERVICE_ACTION_IN:
	case ACCESS_CONTROL_IN:
	case ACCESS_CONTROL_OUT:
	case EXTENDED_COPY:
	case READ_ATTRIBUTE:
	case RECEIVE_COPY_RESULTS:
	case WRITE_ATTRIBUTE:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[10] << 24) | (cdb[11] << 16) |
		       (cdb[12] << 8) | cdb[13];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case VARIABLE_LENGTH_CMD:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[10] << 8) | cdb[11];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case RECEIVE_DIAGNOSTIC:
	case SEND_DIAGNOSTIC:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[3] << 8) | cdb[4];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
 
#if 0
	case GPCMD_READ_CD:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		sectors = (cdb[6] << 16) + (cdb[7] << 8) + cdb[8];
		size = (2336 * sectors);
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
#endif
	case READ_TOC:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = cdb[8];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case REQUEST_SENSE:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = cdb[4];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case READ_ELEMENT_STATUS:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = 65536 * cdb[7] + 256 * cdb[8] + cdb[9];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case WRITE_BUFFER:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		size = (cdb[6] << 16) + (cdb[7] << 8) + cdb[8];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		ret = 2;
		break;
	case RESERVE:
	case RESERVE_10:
		 
		if (cdb[0] == RESERVE_10)
			size = (cdb[7] << 8) | cdb[8];
		else
			size = cmd->data_length;

#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		cmd->transport_allocate_resources =
				&transport_generic_allocate_none;
		transport_get_maps(cmd);
		 
		cmd->transport_emulate_cdb =
				(T10_RES(su_dev)->res_type !=
				 SPC_PASSTHROUGH) ?
				&core_scsi2_emulate_crh : NULL;
		ret = 3;
		break;
	case RELEASE:
	case RELEASE_10:
		 
		if (cdb[0] == RELEASE_10)
			size = (cdb[7] << 8) | cdb[8];
		else
			size = cmd->data_length;

#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		cmd->transport_allocate_resources =
				&transport_generic_allocate_none;
		transport_get_maps(cmd);
		cmd->transport_emulate_cdb =
				(T10_RES(su_dev)->res_type !=
				 SPC_PASSTHROUGH) ?
				&core_scsi2_emulate_crh : NULL;
		ret = 3;
		break;
	case ALLOW_MEDIUM_REMOVAL:
	case GPCMD_CLOSE_TRACK:
	case ERASE:
	case INITIALIZE_ELEMENT_STATUS:
	case GPCMD_LOAD_UNLOAD:
	case REZERO_UNIT:
	case SEEK_10:
	case GPCMD_SET_SPEED:
	case SPACE:
	case START_STOP:
	case SYNCHRONIZE_CACHE:
	case TEST_UNIT_READY:
	case VERIFY:
#ifdef MY_ABC_HERE
	case VERIFY_16:
#endif
	case WRITE_FILEMARKS:
	case MOVE_MEDIUM:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		cmd->transport_allocate_resources =
				&transport_generic_allocate_none;
		transport_get_maps(cmd);
		ret = 3;
		break;
	case REPORT_LUNS:
#ifndef SYNO_LIO_TRANSPORT_PATCHES
		SET_GENERIC_TRANSPORT_FUNCTIONS(cmd);
#endif
		cmd->transport_emulate_cdb =
				&transport_core_report_lun_response;
		size = (cdb[6] << 24) | (cdb[7] << 16) | (cdb[8] << 8) | cdb[9];
#ifdef MY_ABC_HERE
		dev_obj_get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#else
		CMD_ORIG_OBJ_API(cmd)->get_mem_buf(cmd->se_orig_obj_ptr, cmd);
#endif
		transport_get_maps(cmd);
		 
		if (SE_DEV(cmd)->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
			cmd->sam_task_attr = TASK_ATTR_HOQ;
		ret = 2;
		break;
	default:
		printk(KERN_WARNING "TARGET_CORE[%s]: Unsupported SCSI Opcode"
			" 0x%02x, sending CHECK_CONDITION.\n",
			CMD_TFO(cmd)->get_fabric_name(), cdb[0]);
		cmd->transport_wait_for_tasks = &transport_nop_wait_for_tasks;
		transport_get_maps(cmd);
		return 4;
	}

	if (size != cmd->data_length) {
		printk(KERN_WARNING "TARGET_CORE[%s]: Expected Transfer Length:"
			" %u does not match SCSI CDB Length: %u for SAM Opcode:"
			" 0x%02x\n", CMD_TFO(cmd)->get_fabric_name(),
				cmd->data_length, size, cdb[0]);

		cmd->cmd_spdtl = size;

#ifdef MY_ABC_HERE
		if (cmd->data_direction == DMA_TO_DEVICE) {
			printk(KERN_ERR "Rejecting underflow/overflow"
					" WRITE data or BIDI data\n");
			return 6;
		}
#else
		if (cmd->data_direction == SE_DIRECTION_WRITE) {
			printk(KERN_ERR "Rejecting underflow/overflow"
					" WRITE data\n");
			return 6;
		}
#endif
		 
		if (!(ret) && (DEV_ATTRIB(dev)->block_size != 512))  {
			printk(KERN_ERR "Failing OVERFLOW/UNDERFLOW for LBA op"
				" CDB on non 512-byte sector setup subsystem"
				" plugin: %s\n", TRANSPORT(dev)->name);
			 
			return 6;
		}

		if (size > cmd->data_length) {
			cmd->se_cmd_flags |= SCF_OVERFLOW_BIT;
			cmd->residual_count = (size - cmd->data_length);
		} else {
			cmd->se_cmd_flags |= SCF_UNDERFLOW_BIT;
			cmd->residual_count = (cmd->data_length - size);
		}
		cmd->data_length = size;
	}

	transport_set_supported_SAM_opcode(cmd);
	return ret;
}

static inline se_cmd_t *transport_alloc_passthrough_cmd(
	u32 data_length,
	int data_direction)
{
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	return __transport_alloc_se_cmd(&passthrough_fabric_ops, NULL,
			(void *)1, data_length, data_direction, TASK_ATTR_SIMPLE);
#else
	return __transport_alloc_se_cmd(&passthrough_fabric_ops, NULL, NULL,
			data_length, data_direction, TASK_ATTR_SIMPLE);
#endif
}

#ifdef MY_ABC_HERE
static inline void transport_release_tasks(se_cmd_t *);
#endif

se_cmd_t *transport_allocate_passthrough(
	unsigned char *cdb,
	int data_direction,
	u32 se_cmd_flags,
	void *mem,
	u32 se_mem_num,
	u32 length,
#ifndef MY_ABC_HERE
	se_obj_lun_type_t *obj_api,
#endif
	void *type_ptr)
{
	se_cmd_t *cmd;
	se_transform_info_t ti;

	cmd = transport_alloc_passthrough_cmd(length, data_direction);
	if (!(cmd))
		return NULL;
	 
	cmd->se_lun = kzalloc(sizeof(se_lun_t), GFP_KERNEL);
	if (!(cmd->se_lun)) {
		printk(KERN_ERR "Unable to allocate cmd->se_lun\n");
		goto fail;
	}

	spin_lock_init(&cmd->se_lun->lun_sep_lock);
#ifdef MY_ABC_HERE
	SE_LUN(cmd)->lun_type_ptr = type_ptr;
#else
	SE_LUN(cmd)->lun_type = obj_api->se_obj_type;
	SE_LUN(cmd)->lun_type_ptr = type_ptr;
	SE_LUN(cmd)->lun_obj_api = obj_api;

	cmd->se_orig_obj_api = obj_api;
#endif
	cmd->se_orig_obj_ptr = type_ptr;
	cmd->se_cmd_flags = se_cmd_flags;
	SE_LUN(cmd)->se_dev = (se_device_t *) type_ptr;

#ifdef MY_ABC_HERE
	if (dev_obj_check_online(type_ptr) != 0) {
		DEBUG_SO("dev_obj_check_online() failed!\n");
		goto fail;
	}
#else
	if (obj_api->check_online(type_ptr) != 0) {
		DEBUG_SO("obj_api->check_online() failed!\n");
		goto fail;
	}
#endif

	cmd->data_length = length;
	cmd->data_direction = data_direction;
	cmd->se_cmd_flags |= SCF_CMD_PASSTHROUGH;

	if (transport_generic_allocate_tasks(cmd, cdb) < 0)
		goto fail;

	memset(&ti, 0, sizeof(se_transform_info_t));
	ti.ti_data_length = cmd->data_length;
	ti.ti_dev = SE_LUN(cmd)->se_dev;
	ti.ti_se_cmd = cmd;
	ti.se_obj_ptr = type_ptr;
#ifdef MY_ABC_HERE
	DEBUG_SO("ti.se_obj_ptr: %p\n", ti.se_obj_ptr);
#else
	ti.se_obj_api = SE_LUN(cmd)->lun_obj_api;

	DEBUG_SO("ti.se_obj_ptr: %p\n", ti.se_obj_ptr);
	DEBUG_SO("ti.se_obj_api: %p\n", ti.se_obj_api);
	DEBUG_SO("Plugin: %s\n", ti.se_obj_api->obj_plugin->plugin_name);
#endif

	if (!mem) {
		if (cmd->transport_allocate_resources(cmd, cmd->data_length,
					PAGE_SIZE) < 0)
			goto fail;
	} else {
		 
		T_TASK(cmd)->t_mem_list = (struct list_head *)mem;
		T_TASK(cmd)->t_task_se_num = se_mem_num;
		cmd->se_cmd_flags |= SCF_CMD_PASSTHROUGH_NOALLOC;

#ifdef DEBUG_PASSTHROUGH
		{
		u32 total_se_length = 0;
		se_mem_t *se_mem, *se_mem_tmp;

		DEBUG_PT("Preallocated se_mem_list: %p se_mem_num: %d\n",
				mem, se_mem_num);

		list_for_each_entry_safe(se_mem, se_mem_tmp,
				T_TASK(cmd)->t_mem_list, se_list) {
			total_se_length += se_mem->se_len;
			DEBUG_PT("se_mem: %p se_mem->se_page: %p %d:%d\n",
				se_mem, se_mem->se_page, se_mem->se_len,
				se_mem->se_off);
		}
		DEBUG_PT("Total calculated total_se_length: %u\n",
				total_se_length);

		if (total_se_length != length) {
			printk(KERN_ERR "Passed length: %u does not equal"
				" total_se_length: %u\n", length,
					total_se_length);
			BUG();
		}
		}
#endif
	}

#ifdef MY_ABC_HERE
	if (transport_get_sectors(cmd, type_ptr) < 0)
		goto fail;

	if (transport_new_cmd_obj(cmd, &ti, type_ptr, 0) < 0)
		goto fail;
#else
	if (transport_get_sectors(cmd, SE_LUN(cmd)->lun_obj_api, type_ptr) < 0)
		goto fail;

	if (transport_new_cmd_obj(cmd, &ti, SE_LUN(cmd)->lun_obj_api,
			type_ptr, 0) < 0)
		goto fail;
#endif

	return cmd;

fail:
	if (T_TASK(cmd))
		transport_release_tasks(cmd);
	kfree(T_TASK(cmd));
	kfree(cmd->se_lun);
	transport_free_se_cmd(cmd);

	return NULL;
}

void transport_passthrough_release(
	se_cmd_t *cmd)
{
	if (!cmd) {
		printk(KERN_ERR "transport_passthrough_release passed"
			" NULL se_cmd_t\n");
		return;
	}

	if (cmd->transport_wait_for_tasks)
		cmd->transport_wait_for_tasks(cmd, 0, 0);

	transport_generic_remove(cmd, 0, 0);
}

int transport_passthrough_complete(
	se_cmd_t *cmd)
{
#ifdef MY_ABC_HERE
	if (dev_obj_check_shutdown(cmd->se_orig_obj_ptr) != 0)
		return -2;
#else
	if (cmd->se_orig_obj_api->check_shutdown(cmd->se_orig_obj_ptr) != 0)
		return -2;
#endif

	switch (cmd->scsi_status) {
	case 0x00:  
		DEBUG_PT("SCSI Status: GOOD\n");
		return 0;
	case 0x02:  
		DEBUG_PT("SCSI Status: CHECK_CONDITION\n");
 
		return -1;
	default:
		DEBUG_PT("SCSI Status: 0x%02x\n", cmd->scsi_status);
		return -1;
	}

	return 0;
}

void transport_memcpy_write_contig(
	se_cmd_t *cmd,
	struct scatterlist *sg_d,
	unsigned char *src)
{
	u32 i = 0, length = 0, total_length = cmd->data_length;
	void *dst;

	while (total_length) {
		length = sg_d[i].length;

		if (length > total_length)
			length = total_length;

		dst = sg_virt(&sg_d[i]);

		memcpy(dst, src, length);

		if (!(total_length -= length))
			return;

		src += length;
		i++;
        }
}
EXPORT_SYMBOL(transport_memcpy_write_contig);

void transport_memcpy_read_contig(
	se_cmd_t *cmd,
	unsigned char *dst,
	struct scatterlist *sg_s)
{
	u32 i = 0, length = 0, total_length = cmd->data_length;
	void *src;

	while (total_length) {
		length = sg_s[i].length;

		if (length > total_length)
			length = total_length;

		src = sg_virt(&sg_s[i]);

		memcpy(dst, src, length);

		if (!(total_length -= length))
			return;

		dst += length;
		i++;
	}
}
EXPORT_SYMBOL(transport_memcpy_read_contig);

int transport_generic_passthrough_async(
	se_cmd_t *cmd,
	void (*callback)(se_cmd_t *cmd,
		void *callback_arg, int complete_status),
	void *callback_arg)
{
#ifdef MY_ABC_HERE
	int write = (cmd->data_direction == DMA_TO_DEVICE);
#else
	int write = (cmd->data_direction == SE_DIRECTION_WRITE);
#endif
	int no_alloc = (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH_NOALLOC);
	int pt_done = (cmd->transport_passthrough_done != NULL);

	if (callback) {
		cmd->callback = callback;
		cmd->callback_arg = callback_arg;
	}

	if (transport_generic_handle_cdb(cmd) < 0)
		return -1;

	if (write && !no_alloc) {
		if (wait_for_completion_interruptible(
			&T_TASK(cmd)->t_transport_passthrough_wcomp) != 0)
			return -1;

		transport_generic_process_write(cmd);
	}

	if (callback || pt_done)
		return 0;

	wait_for_completion(&T_TASK(cmd)->t_transport_passthrough_comp);

	return transport_passthrough_complete(cmd);
}

int transport_generic_passthrough(se_cmd_t *cmd)
{
	return transport_generic_passthrough_async(cmd, NULL, NULL);
}

void transport_complete_task_attr(se_cmd_t *cmd)
{
	se_device_t *dev = SE_DEV(cmd);
	se_cmd_t *cmd_p, *cmd_tmp;
	int new_active_tasks = 0;

	if (cmd->sam_task_attr == TASK_ATTR_SIMPLE) {
		atomic_dec(&dev->simple_cmds);
		smp_mb__after_atomic_dec();
		dev->dev_cur_ordered_id++;
		DEBUG_STA("Incremented dev->dev_cur_ordered_id: %u for"
			" SIMPLE: %u\n", dev->dev_cur_ordered_id,
			cmd->se_ordered_id);
	} else if (cmd->sam_task_attr == TASK_ATTR_HOQ) {
		atomic_dec(&dev->dev_hoq_count);
		smp_mb__after_atomic_dec();
		dev->dev_cur_ordered_id++;
		DEBUG_STA("Incremented dev_cur_ordered_id: %u for"
			" HEAD_OF_QUEUE: %u\n", dev->dev_cur_ordered_id,
			cmd->se_ordered_id);
	} else if (cmd->sam_task_attr == TASK_ATTR_ORDERED) {
		spin_lock(&dev->ordered_cmd_lock);
		list_del(&cmd->se_ordered_list);
		atomic_dec(&dev->dev_ordered_sync);
		smp_mb__after_atomic_dec();
		spin_unlock(&dev->ordered_cmd_lock);

		dev->dev_cur_ordered_id++;
		DEBUG_STA("Incremented dev_cur_ordered_id: %u for ORDERED:"
			" %u\n", dev->dev_cur_ordered_id, cmd->se_ordered_id);
	}
	 
	spin_lock(&dev->delayed_cmd_lock);
	list_for_each_entry_safe(cmd_p, cmd_tmp,
			&dev->delayed_cmd_list, se_delayed_list) {

		list_del(&cmd_p->se_delayed_list);
		spin_unlock(&dev->delayed_cmd_lock);

		DEBUG_STA("Calling add_tasks() for"
			" cmd_p: 0x%02x Task Attr: 0x%02x"
			" Dormant -> Active, se_ordered_id: %u\n",
			T_TASK(cmd_p)->t_task_cdb[0],
			cmd_p->sam_task_attr, cmd_p->se_ordered_id);

#ifdef MY_ABC_HERE
		transport_add_tasks_from_cmd(cmd_p);
#else
		CMD_ORIG_OBJ_API(cmd_p)->add_tasks(
				cmd_p->se_orig_obj_ptr, cmd_p);
#endif
		new_active_tasks++;

		spin_lock(&dev->delayed_cmd_lock);
		if (cmd_p->sam_task_attr == TASK_ATTR_ORDERED)
			break;
	}
	spin_unlock(&dev->delayed_cmd_lock);
	 
	if (new_active_tasks != 0)
		wake_up_interruptible(&dev->dev_queue_obj->thread_wq);
}

void transport_generic_complete_ok(se_cmd_t *cmd)
{
	int reason = 0;
	 
	if (SE_DEV(cmd)->dev_task_attr_type == SAM_TASK_ATTR_EMULATED)
		transport_complete_task_attr(cmd);
	 
	if (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH) {
		transport_lun_remove_cmd(cmd);
		if (!(transport_cmd_check_stop_to_fabric(cmd)))
			transport_passthrough_check_stop(cmd);
		return;
	} else if (cmd->se_cmd_flags & SCF_TRANSPORT_TASK_SENSE) {
		if (transport_get_sense_data(cmd) < 0)
			reason = NON_EXISTENT_LUN;

		if (cmd->scsi_status) {
			transport_send_check_condition_and_sense(
					cmd, reason, 1);
			transport_lun_remove_cmd(cmd);
			transport_cmd_check_stop_to_fabric(cmd);
			return;
		}
	}

	switch (cmd->data_direction) {
#ifdef MY_ABC_HERE
	case DMA_FROM_DEVICE:
#else
	case SE_DIRECTION_READ:
#endif
#ifdef SNMP_SUPPORT
		spin_lock(&cmd->se_lun->lun_sep_lock);
		if (SE_LUN(cmd)->lun_sep) {
			SE_LUN(cmd)->lun_sep->sep_stats.tx_data_octets +=
					cmd->data_length;
		}
		spin_unlock(&cmd->se_lun->lun_sep_lock);
#endif
		CMD_TFO(cmd)->queue_data_in(cmd);
		break;
#ifdef MY_ABC_HERE
	case DMA_TO_DEVICE:
#else
	case SE_DIRECTION_WRITE:
#endif
#ifdef SNMP_SUPPORT
		spin_lock(&cmd->se_lun->lun_sep_lock);
		if (SE_LUN(cmd)->lun_sep) {
			SE_LUN(cmd)->lun_sep->sep_stats.rx_data_octets +=
				cmd->data_length;
		}
		spin_unlock(&cmd->se_lun->lun_sep_lock);
#endif
		 
#ifdef MY_ABC_HERE
	case DMA_NONE:
#else
	case SE_DIRECTION_NONE:
#endif
		CMD_TFO(cmd)->queue_status(cmd);
		break;
	default:
		break;
	}

	transport_lun_remove_cmd(cmd);
	transport_cmd_check_stop_to_fabric(cmd);
}

void transport_free_dev_tasks(se_cmd_t *cmd)
{
	se_task_t *task, *task_tmp;
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	list_for_each_entry_safe(task, task_tmp,
				&T_TASK(cmd)->t_task_list, t_list) {
		if (atomic_read(&task->task_active))
			continue;

		if (!task->transport_req)
			continue;

		kfree(task->task_sg);

		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
#ifdef MY_ABC_HERE
		if (task->se_dev && TRANSPORT(task->se_dev))
#else
		if (task->se_dev)
#endif
			TRANSPORT(task->se_dev)->free_task(task);
		else
			printk(KERN_ERR "task[%u] - task->se_dev is NULL\n",
				task->task_no);
		spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);

		list_del(&task->t_list);
		kmem_cache_free(se_task_cache, task);
	}
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
}

static inline void transport_free_pages(se_cmd_t *cmd)
{
	se_mem_t *se_mem, *se_mem_tmp;
	int free_page =
		((cmd->se_cmd_flags & SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC) == 0);

	if (T_TASK(cmd)->t_task_buf) {
		kfree(T_TASK(cmd)->t_task_buf);
		T_TASK(cmd)->t_task_buf = NULL;
		return;
	}

	if (cmd->transport_free_resources) {
		cmd->transport_free_resources(cmd);
		return;
	}
	 
	if (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH_NOALLOC)
		return;

	if (!(T_TASK(cmd)->t_task_se_num))
		return;

	list_for_each_entry_safe(se_mem, se_mem_tmp,
			T_TASK(cmd)->t_mem_list, se_list) {
		 
		if (free_page)
			__free_page(se_mem->se_page);

		list_del(&se_mem->se_list);
		kmem_cache_free(se_mem_cache, se_mem);
	}

	kfree(T_TASK(cmd)->t_mem_list);
	T_TASK(cmd)->t_mem_list = NULL;
	T_TASK(cmd)->t_task_se_num = 0;
}

#ifdef MY_ABC_HERE
static inline void transport_release_tasks(se_cmd_t *cmd)
{
	transport_free_dev_tasks(cmd);
}
#else
void transport_release_tasks(se_cmd_t *cmd)
{
	CMD_ORIG_OBJ_API(cmd)->free_tasks(cmd->se_orig_obj_ptr, cmd);
}
#endif

static inline int transport_dec_and_check(se_cmd_t *cmd)
{
	unsigned long flags;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (atomic_read(&T_TASK(cmd)->t_fe_count)) {
		if (!(atomic_dec_and_test(&T_TASK(cmd)->t_fe_count))) {
			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock,
					flags);
			return 1;
		}
	}

	if (atomic_read(&T_TASK(cmd)->t_se_count)) {
		if (!(atomic_dec_and_test(&T_TASK(cmd)->t_se_count))) {
			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock,
					flags);
			return 1;
		}
	}
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	return 0;
}

#ifdef SYNO_LIO_TRANSPORT_PATCHES
static inline void transport_release_se_cmd(struct se_cmd_s *cmd)
{
	 
	if (cmd->se_fabric_cmd_ptr) {
		CMD_TFO(cmd)->release_cmd_direct(cmd);
		transport_free_se_cmd(cmd);
	} else {
		transport_free_se_cmd(cmd);
		CMD_TFO(cmd)->release_cmd_direct(cmd);
	}
}
#endif

void transport_release_fe_cmd(se_cmd_t *cmd)
{
	unsigned long flags;

	if (transport_dec_and_check(cmd))
		return;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
#ifdef MY_ABC_HERE
	if (!(atomic_read(&T_TASK(cmd)->transport_dev_active))) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		goto free_pages;
	}
	atomic_set(&T_TASK(cmd)->transport_dev_active, 0);
#endif
	transport_all_task_dev_remove_state(cmd);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	transport_release_tasks(cmd);
#ifdef MY_ABC_HERE
free_pages:
#endif
	transport_free_pages(cmd);

	if (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH)
		kfree(cmd->se_lun);

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	transport_release_se_cmd(cmd);
#else
	CMD_TFO(cmd)->release_cmd_direct(cmd);
	transport_free_se_cmd(cmd);
#endif
}

int transport_generic_remove(
	se_cmd_t *cmd,
	int release_to_pool,
	int session_reinstatement)
{
	unsigned long flags;

	if (!(T_TASK(cmd)))
		goto release_cmd;

	if (transport_dec_and_check(cmd)) {
		if (session_reinstatement) {
			spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
			transport_all_task_dev_remove_state(cmd);
			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock,
					flags);
		}
		return 1;
	}

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
#ifdef MY_ABC_HERE
	if (!(atomic_read(&T_TASK(cmd)->transport_dev_active))) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		goto free_pages;
	}
	atomic_set(&T_TASK(cmd)->transport_dev_active, 0);
#endif
	transport_all_task_dev_remove_state(cmd);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	transport_release_tasks(cmd);
#ifdef MY_ABC_HERE
free_pages:
#endif
	transport_free_pages(cmd);

release_cmd:
	if (release_to_pool && !(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH))
		transport_release_cmd_to_pool(cmd);
	else {
		if (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH)
			kfree(cmd->se_lun);

#ifdef SYNO_LIO_TRANSPORT_PATCHES
		transport_release_se_cmd(cmd);
#else
		CMD_TFO(cmd)->release_cmd_direct(cmd);
		transport_free_se_cmd(cmd);
#endif
	}

	return 0;
}

int transport_generic_map_mem_to_cmd(
	se_cmd_t *cmd,
	void *mem,
	u32 se_mem_num)
{
#ifdef MY_ABC_HERE
	u32 se_mem_cnt_out = 0;
#else
	u32 se_mem_cnt_out = 0, se_mem_task_off = 0;
#endif
	int ret;

	if (!(mem) || !(se_mem_num))
		return 0;

	if (!(cmd->se_cmd_flags & SCF_PASSTHROUGH_SG_TO_MEM)) {
		T_TASK(cmd)->t_mem_list = (struct list_head *)mem;
		T_TASK(cmd)->t_task_se_num = se_mem_num;
		cmd->se_cmd_flags |= SCF_CMD_PASSTHROUGH_NOALLOC;
		return 0;
	}
	 
	if ((cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB) ||
	    (cmd->se_cmd_flags & SCF_SCSI_CONTROL_SG_IO_CDB)) {
		  
		T_TASK(cmd)->t_mem_list = transport_init_se_mem_list();
		if (!(T_TASK(cmd)->t_mem_list))
			return -1;

#ifdef MY_ABC_HERE
		ret = transport_map_sg_to_mem(cmd,
			T_TASK(cmd)->t_mem_list, mem, &se_mem_cnt_out);
#else
		ret = transport_map_sg_to_mem(cmd,
			T_TASK(cmd)->t_mem_list, mem, &se_mem_cnt_out,
			&se_mem_task_off);
#endif
		if (ret < 0)
			return -1;

		T_TASK(cmd)->t_task_se_num = se_mem_cnt_out;
		cmd->se_cmd_flags |= SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC;

	} else if (cmd->se_cmd_flags & SCF_SCSI_CONTROL_NONSG_IO_CDB) {
		 
		cmd->se_cmd_flags |= SCF_PASSTHROUGH_CONTIG_TO_SG;
		T_TASK(cmd)->t_task_pt_buf = mem;
	}

	return 0;
}
EXPORT_SYMBOL(transport_generic_map_mem_to_cmd);
	
#ifndef SYNO_LIO_TRANSPORT_PATCHES
 
static int transport_generic_map_buffers_to_tasks(se_cmd_t *cmd)
{
	se_task_t *task = NULL;
	int ret;

	if (cmd->se_cmd_flags & SCF_SCSI_NON_DATA_CDB)
		goto non_scsi_data;
	else if (cmd->se_cmd_flags & SCF_SCSI_CONTROL_NONSG_IO_CDB) {
		list_for_each_entry(task, &T_TASK(cmd)->t_task_list, t_list) {
			if (atomic_read(&task->task_sent))
				continue;

			ret = task->transport_map_task(task, task->task_size);
			if (ret < 0)
				return ret;

			DEBUG_CMD_M("Mapping SCF_SCSI_CONTROL_NONSG_IO_CDB"
				" task_size: %u\n", task->task_size);
		}
		return 0;
	}

	list_for_each_entry(task, &T_TASK(cmd)->t_task_list, t_list) {
		if (atomic_read(&task->task_sent))
			continue;

		ret = task->transport_map_task(task, task->task_size);
		if (ret < 0)
			return ret;

		DEBUG_CMD_M("Mapping task[%d]_se_obj_ptr[%p] %s_IO task_lba:"
			" %llu task_size: %u task_sg_num: %d\n",
			task->task_no, task->se_obj_ptr,
			(cmd->se_cmd_flags & SCF_SCSI_CONTROL_SG_IO_CDB) ?
			"CONTROL" : "DATA", task->task_lba, task->task_size,
			task->task_sg_num);
	}

	return 0;

non_scsi_data:
	list_for_each_entry(task, &T_TASK(cmd)->t_task_list, t_list) {
		if (atomic_read(&task->task_sent))
			continue;

		ret = task->transport_map_task(task, task->task_size);
		if (ret < 0)
			return ret;

		DEBUG_CMD_M("Mapping SCF_SCSI_NON_DATA_CDB task_size: %u"
			" task->task_sg_num: %d\n", task->task_size,
				task->task_sg_num);
	}

	return 0;
}
#endif

#ifndef SYNO_LIO_TRANSPORT_PATCHES
 
int transport_generic_do_transform(se_cmd_t *cmd, se_transform_info_t *ti)
{
	if (cmd->transport_cdb_transform(cmd, ti) < 0)
		return -1;

	return 0;
}
#endif

#ifdef MY_ABC_HERE
int transport_get_sectors(
	se_cmd_t *cmd,
	void *obj_ptr)
{
	struct se_device_s *dev = SE_DEV(cmd);

	if (!(cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB))
		return 0;

	T_TASK(cmd)->t_task_sectors =
		(cmd->data_length / DEV_ATTRIB(dev)->block_size);
	if (!(T_TASK(cmd)->t_task_sectors))
		T_TASK(cmd)->t_task_sectors = 1;

	if (TRANSPORT(dev)->get_device_type(dev) != TYPE_DISK)
		return 0;

	if ((T_TASK(cmd)->t_task_lba + T_TASK(cmd)->t_task_sectors) >
		dev_obj_end_lba(obj_ptr)) {
		printk(KERN_ERR "LBA: %llu Sectors: %u exceeds"
				" dev_obj_end_lba(): %llu\n",
				T_TASK(cmd)->t_task_lba, T_TASK(cmd)->t_task_sectors,
				dev_obj_end_lba(obj_ptr));
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = SECTOR_COUNT_TOO_MANY;
		return PYX_TRANSPORT_REQ_TOO_MANY_SECTORS;
	}

	return 0;
}
#else
int transport_get_sectors(
	se_cmd_t *cmd,
	se_obj_lun_type_t *obj_api,
	void *obj_ptr)
{
	if (!(cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB))
		return 0;

	T_TASK(cmd)->t_task_sectors =
		(cmd->data_length / obj_api->blocksize(obj_ptr));
	if (!(T_TASK(cmd)->t_task_sectors))
		T_TASK(cmd)->t_task_sectors = 1;

	if (obj_api->get_device_type(obj_ptr) != TYPE_DISK)
		return 0;

	if ((T_TASK(cmd)->t_task_lba + T_TASK(cmd)->t_task_sectors) >
	     obj_api->total_sectors(obj_ptr)) {
		printk(KERN_ERR "LBA: %llu Sectors: %u exceeds"
			" obj_api->total_sectors(): %llu\n",
			T_TASK(cmd)->t_task_lba, T_TASK(cmd)->t_task_sectors,
			obj_api->total_sectors(obj_ptr));
		cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
		cmd->scsi_sense_reason = SECTOR_COUNT_TOO_MANY;
		return PYX_TRANSPORT_REQ_TOO_MANY_SECTORS;
	}

	return 0;
}
#endif

#ifdef SYNO_LIO_TRANSPORT_PATCHES
static
#endif
int transport_new_cmd_obj(
	se_cmd_t *cmd,
	se_transform_info_t *ti,
#ifndef MY_ABC_HERE
	se_obj_lun_type_t *obj_api,
#endif
	void *obj_ptr,
	int post_execute)
{
	u32 task_cdbs = 0, task_offset = 0;
	se_mem_t *se_mem_out = NULL;
#ifdef MY_ABC_HERE
	struct se_device_s *dev = SE_DEV(cmd);
#endif

	if (!(cmd->se_cmd_flags & SCF_SCSI_DATA_SG_IO_CDB)) {
		task_cdbs++;
		T_TASK(cmd)->t_task_cdbs++;
	} else {
		ti->ti_set_counts = 1;
#ifdef MY_ABC_HERE
		ti->ti_dev = dev;

		task_cdbs = transport_generic_get_cdb_count(cmd, ti, obj_ptr,
				T_TASK(cmd)->t_task_lba,
				T_TASK(cmd)->t_task_sectors,
				NULL, &se_mem_out, &task_offset);
#else
		task_cdbs = obj_api->get_cdb_count(obj_ptr, ti,
				T_TASK(cmd)->t_task_lba,
				T_TASK(cmd)->t_task_sectors,
				NULL, &se_mem_out, &task_offset);
#endif
		if (!(task_cdbs)) {
			cmd->se_cmd_flags |= SCF_SCSI_CDB_EXCEPTION;
			cmd->scsi_sense_reason =
					LOGICAL_UNIT_COMMUNICATION_FAILURE;
			return PYX_TRANSPORT_LU_COMM_FAILURE;
		}
		T_TASK(cmd)->t_task_cdbs += task_cdbs;

		cmd->transport_cdb_transform =
				&transport_process_data_sg_transform;
#if 0
#ifdef MY_ABC_HERE
		printk(KERN_INFO "data_length: %u, LBA: %llu t_task_sectors:"
			" %u, t_task_cdbs: %u\n", obj_ptr, cmd->data_length,
			T_TASK(cmd)->t_task_lba, T_TASK(cmd)->t_task_sectors,
			T_TASK(cmd)->t_task_cdbs);
#else
		printk(KERN_INFO "[%s]: api: %p ptr: %p data_length: %u, LBA:"
			" %llu t_task_sectors: %u, t_task_cdbs: %u\n",
			obj_api->obj_plugin->plugin_name, obj_api, obj_ptr,
			cmd->data_length, T_TASK(cmd)->t_task_lba,
			T_TASK(cmd)->t_task_sectors, T_TASK(cmd)->t_task_cdbs);
#endif
#endif
	}

#ifndef SYNO_LIO_TRANSPORT_PATCHES
	cmd->transport_do_transform = &transport_generic_do_transform;
#endif
	if (!post_execute) {
		atomic_set(&T_TASK(cmd)->t_task_cdbs_left, task_cdbs);
		atomic_set(&T_TASK(cmd)->t_task_cdbs_ex_left, task_cdbs);
		atomic_set(&T_TASK(cmd)->t_task_cdbs_timeout_left, task_cdbs);
	} else {
		atomic_add(task_cdbs, &T_TASK(cmd)->t_task_cdbs_left);
		atomic_add(task_cdbs, &T_TASK(cmd)->t_task_cdbs_ex_left);
		atomic_add(task_cdbs, &T_TASK(cmd)->t_task_cdbs_timeout_left);
	}

	return 0;
}

unsigned char *transport_get_vaddr(se_mem_t *se_mem)
{
	return page_address(se_mem->se_page) + se_mem->se_off;
}

struct list_head *transport_init_se_mem_list(void)
{
	struct list_head *se_mem_list;

	se_mem_list = kzalloc(sizeof(struct list_head), GFP_KERNEL);
	if (!(se_mem_list)) {
		printk(KERN_ERR "Unable to allocate memory for se_mem_list\n");
		return NULL;
	}
	INIT_LIST_HEAD(se_mem_list);

	return se_mem_list;
}

void transport_free_se_mem_list(struct list_head *se_mem_list)
{
	se_mem_t *se_mem, *se_mem_tmp;

	if (!se_mem_list)
		return;

	list_for_each_entry_safe(se_mem, se_mem_tmp, se_mem_list, se_list) {
		list_del(&se_mem->se_list);
		kmem_cache_free(se_mem_cache, se_mem);
	}
	kfree(se_mem_list);
}

int transport_generic_get_mem(se_cmd_t *cmd, u32 length, u32 dma_size)
{
	unsigned char *buf;
	se_mem_t *se_mem;

	T_TASK(cmd)->t_mem_list = transport_init_se_mem_list();
	if (!(T_TASK(cmd)->t_mem_list))
		return -1;

	while (length) {
		se_mem = kmem_cache_zalloc(se_mem_cache, GFP_KERNEL);
		if (!(se_mem)) {
			printk(KERN_ERR "Unable to allocate se_mem_t\n");
			goto out;
		}
		INIT_LIST_HEAD(&se_mem->se_list);
		se_mem->se_len = (length > dma_size) ? dma_size : length;

		se_mem->se_page = (struct page *) alloc_pages(GFP_KERNEL, 0);
		if (!(se_mem->se_page)) {
			printk(KERN_ERR "alloc_pages() failed\n");
			goto out;
		}

		buf = kmap_atomic(se_mem->se_page, KM_IRQ0);
		if (!(buf)) {
			printk(KERN_ERR "kmap_atomic() failed\n");
			goto out;
		}
		memset(buf, 0, se_mem->se_len);
		kunmap_atomic(buf, KM_IRQ0);

		list_add_tail(&se_mem->se_list, T_TASK(cmd)->t_mem_list);
		T_TASK(cmd)->t_task_se_num++;

		DEBUG_MEM("Allocated se_mem_t page(%p) Length(%u)"
			" Offset(%u)\n", se_mem->se_page, se_mem->se_len,
			se_mem->se_off);

		length -= se_mem->se_len;
	}

	DEBUG_MEM("Allocated total se_mem_t elements(%u)\n",
			T_TASK(cmd)->t_task_se_num);

	return 0;
out:
	return -1;
}

#ifdef MY_ABC_HERE
extern u32 transport_calc_sg_num(
	se_task_t *task,
	se_mem_t *in_se_mem,
	u32 task_offset)
{
	se_cmd_t *se_cmd = task->task_se_cmd;
	se_mem_t *se_mem = in_se_mem;
	u32 sg_length, task_size = task->task_size;

	while (task_size != 0) {
		DEBUG_SC("se_mem->se_page(%p) se_mem->se_len(%u)"
			" se_mem->se_off(%u) task_offset(%u)\n",
			se_mem->se_page, se_mem->se_len,
			se_mem->se_off, task_offset);

		if (task_offset == 0) {
			if (task_size >= se_mem->se_len) {
				sg_length = se_mem->se_len;

				if (!(list_is_last(&se_mem->se_list,
						T_TASK(se_cmd)->t_mem_list)))
					se_mem = list_entry(se_mem->se_list.next,
							se_mem_t, se_list);
			} else {
				sg_length = task_size;
				task_size -= sg_length;
				goto next;
			}

			DEBUG_SC("sg_length(%u) task_size(%u)\n",
					sg_length, task_size);
		} else {
			if ((se_mem->se_len - task_offset) > task_size) {
				sg_length = task_size;
				task_size -= sg_length;
				goto next;
			} else {
				sg_length = (se_mem->se_len - task_offset);

				if (!(list_is_last(&se_mem->se_list,
						T_TASK(se_cmd)->t_mem_list)))
					se_mem = list_entry(se_mem->se_list.next,
							se_mem_t, se_list);
			}

			DEBUG_SC("sg_length(%u) task_size(%u)\n",
					sg_length, task_size);

			task_offset = 0;
		}
		task_size -= sg_length;
next:
		DEBUG_SC("task[%u] - Reducing task_size to(%u)\n",
			task->task_no, task_size);

		task->task_sg_num++;
	}

	task->task_sg = kzalloc(task->task_sg_num *
			sizeof(struct scatterlist), GFP_KERNEL);
	if (!(task->task_sg)) {
		printk(KERN_ERR "Unable to allocate memory for"
				" task->task_sg\n");
		return 0;
	}

	sg_init_table(&task->task_sg[0], task->task_sg_num);

	DEBUG_SC("Successfully allocated task->task_sg_num(%u)\n",
			task->task_sg_num);

	return task->task_sg_num;
}
#else
extern u32 transport_calc_sg_num(
	se_task_t *task,
	se_mem_t *in_se_mem,
	u32 task_offset)
{
	se_mem_t *se_mem = in_se_mem;
	u32 sg_length, sg_offset, task_size = task->task_size;
	u32 saved_task_offset = 0;

	while (task_size) {
		DEBUG_SC("se_mem->se_page(%p) se_mem->se_len(%u)"
			" se_mem->se_off(%u) task_offset(%u)\n",
			se_mem->se_page, se_mem->se_len,
			se_mem->se_off, task_offset);

		if (task_offset == 0) {
			if (task_size > se_mem->se_len)
				sg_length = se_mem->se_len;
			else
				sg_length = task_size;

			DEBUG_SC("sg_length(%u) task_size(%u)\n",
					sg_length, task_size);

			if (saved_task_offset)
				task_offset = saved_task_offset;
		} else {
			sg_offset = task_offset;

			if ((se_mem->se_len - task_offset) > task_size)
				sg_length = task_size;
			else
				sg_length = (se_mem->se_len - task_offset);

			DEBUG_SC("sg_length(%u) task_size(%u)\n",
					sg_length, task_size);

			saved_task_offset = task_offset;
			task_offset = 0;
		}
		task_size -= sg_length;

		DEBUG_SC("task[%u] - Reducing task_size to(%u)\n",
			task->task_no, task_size);

		task->task_sg_num++;

		list_for_each_entry_continue(se_mem,
				task->task_se_cmd->t_task->t_mem_list, se_list)
			break;

		if (!se_mem)
			break;
	}

	task->task_sg = kzalloc(task->task_sg_num *
			sizeof(struct scatterlist), GFP_KERNEL);
	if (!(task->task_sg)) {
		printk(KERN_ERR "Unable to allocate memory for"
				" task->task_sg\n");
		return 0;
	}

	sg_init_table(&task->task_sg[0], task->task_sg_num);

	DEBUG_SC("Successfully allocated task->task_sg_num(%u)\n",
			task->task_sg_num);

	return task->task_sg_num;
}
#endif

#ifdef MY_ABC_HERE
static inline int transport_set_task_sectors_disk(
	se_task_t *task,
	void *obj_ptr,
	unsigned long long lba,
	u32 sectors,
	int *max_sectors_set)
{
	if ((lba + sectors) > dev_obj_end_lba(obj_ptr)) {
		task->task_sectors = ((dev_obj_end_lba(obj_ptr) - lba) + 1);

		if (task->task_sectors > dev_obj_max_sectors(obj_ptr)) {
			task->task_sectors = dev_obj_max_sectors(obj_ptr);
			*max_sectors_set = 1;
		}
	} else {
		if (sectors > dev_obj_max_sectors(obj_ptr)) {
			task->task_sectors = dev_obj_max_sectors(obj_ptr);
			*max_sectors_set = 1;
		} else
			task->task_sectors = sectors;
	}

	return 0;
}
#else
static inline int transport_set_task_sectors_disk(
	se_task_t *task,
	se_obj_lun_type_t *obj_api,
	void *obj_ptr,
	unsigned long long lba,
	u32 sectors,
	int *max_sectors_set)
{
	if ((lba + sectors) > obj_api->end_lba(obj_ptr, 1)) {
		task->task_sectors = ((obj_api->end_lba(obj_ptr, 1) - lba) + 1);

		if (task->task_sectors > obj_api->max_sectors(obj_ptr)) {
			task->task_sectors = obj_api->max_sectors(obj_ptr);
			*max_sectors_set = 1;
		}
	} else {
		if (sectors > obj_api->max_sectors(obj_ptr)) {
			task->task_sectors = obj_api->max_sectors(obj_ptr);
			*max_sectors_set = 1;
		} else
			task->task_sectors = sectors;
	}

	return 0;
}
#endif

#ifdef MY_ABC_HERE
static inline int transport_set_task_sectors_non_disk(
	se_task_t *task,
	void *obj_ptr,
	unsigned long long lba,
	u32 sectors,
	int *max_sectors_set)
{
	if (sectors > dev_obj_max_sectors(obj_ptr)) {
		task->task_sectors = dev_obj_max_sectors(obj_ptr);
		*max_sectors_set = 1;
	} else
		task->task_sectors = sectors;

	return 0;
}
#else
static inline int transport_set_task_sectors_non_disk(
	se_task_t *task,
	se_obj_lun_type_t *obj_api,
	void *obj_ptr,
	unsigned long long lba,
	u32 sectors,
	int *max_sectors_set)
{
	if (sectors > obj_api->max_sectors(obj_ptr)) {
		task->task_sectors = obj_api->max_sectors(obj_ptr);
		*max_sectors_set = 1;
	} else
		task->task_sectors = sectors;

	return 0;
}
#endif

#ifdef MY_ABC_HERE
static inline int transport_set_task_sectors(
	se_task_t *task,
	void *obj_ptr,
	unsigned long long lba,
	u32 sectors,
	int *max_sectors_set)
{
	struct se_device_s *dev = obj_ptr;

	return (TRANSPORT(dev)->get_device_type(dev) == TYPE_DISK) ?
		transport_set_task_sectors_disk(task, obj_ptr, lba, sectors,
				max_sectors_set) :
		transport_set_task_sectors_non_disk(task, obj_ptr, lba, sectors,
				max_sectors_set);
}
#else
static inline int transport_set_task_sectors(
	se_task_t *task,
	se_obj_lun_type_t *obj_api,
	void *obj_ptr,
	unsigned long long lba,
	u32 sectors,
	int *max_sectors_set)
{
	return (obj_api->get_device_type(obj_ptr) == TYPE_DISK)	?
		transport_set_task_sectors_disk(task, obj_api, obj_ptr,
				lba, sectors, max_sectors_set) :
		transport_set_task_sectors_non_disk(task, obj_api, obj_ptr,
				lba, sectors, max_sectors_set);
}
#endif

#ifdef MY_ABC_HERE
int transport_map_sg_to_mem(
	se_cmd_t *cmd,
	struct list_head *se_mem_list,
	void *in_mem,
	u32 *se_mem_cnt)
{
	se_mem_t *se_mem;
	struct scatterlist *sg;
	u32 sg_count = 1, cmd_size = cmd->data_length;

	if (!in_mem) {
		printk(KERN_ERR "No source scatterlist\n");
		return -1;
	}
	sg = (struct scatterlist *)in_mem;

	while (cmd_size) {
		se_mem = kmem_cache_zalloc(se_mem_cache, GFP_KERNEL);
		if (!(se_mem)) {
			printk(KERN_ERR "Unable to allocate se_mem_t\n");
			return -1;
		}
		INIT_LIST_HEAD(&se_mem->se_list);
		DEBUG_MEM("sg_to_mem: Starting loop with cmd_size: %u"
			" sg_page: %p offset: %d length: %d\n", cmd_size,
			sg_page(sg), sg->offset, sg->length);

		se_mem->se_page = sg_page(sg);
		se_mem->se_off = sg->offset;

		if (cmd_size > sg->length) {
			se_mem->se_len = sg->length;
			sg = sg_next(sg);
			sg_count++;
		} else
			se_mem->se_len = cmd_size;

		cmd_size -= se_mem->se_len;

		DEBUG_MEM("sg_to_mem: *se_mem_cnt: %u cmd_size: %u\n",
			*se_mem_cnt, cmd_size);
		DEBUG_MEM("sg_to_mem: Final se_page: %p se_off: %d se_len: %d\n",
			se_mem->se_page, se_mem->se_off, se_mem->se_len);

		list_add_tail(&se_mem->se_list, se_mem_list);
		(*se_mem_cnt)++;
	}

	DEBUG_MEM("task[0] - Mapped(%u) struct scatterlist segments to(%u)"
		" se_mem_t\n", sg_count, *se_mem_cnt);

	if (sg_count != *se_mem_cnt)
		BUG();

	return 0;
}
#else
int transport_map_sg_to_mem(
	se_cmd_t *cmd,
	struct list_head *se_mem_list,
	void *in_mem,
	u32 *se_mem_cnt,
	u32 *task_offset)
{
	se_mem_t *se_mem;
	struct scatterlist *sg_s;
	u32 j = 0, saved_task_offset = 0, task_size = cmd->data_length;

	if (!in_mem) {
		printk(KERN_ERR "No source scatterlist\n");
		return -1;
	}
	sg_s = (struct scatterlist *)in_mem;

	while (task_size) {
		se_mem = kmem_cache_zalloc(se_mem_cache, GFP_KERNEL);
		if (!(se_mem)) {
			printk(KERN_ERR "Unable to allocate se_mem_t\n");
			return -1;
		}
		INIT_LIST_HEAD(&se_mem->se_list);

		if (*task_offset == 0) {
			se_mem->se_page = sg_page(&sg_s[j]);
			se_mem->se_off = sg_s[j].offset;

			if (task_size >= sg_s[j].length)
				se_mem->se_len =  sg_s[j++].length;
			else {
				se_mem->se_len = task_size;

				task_size -= se_mem->se_len;
				if (!(task_size)) {
					*task_offset = (se_mem->se_len +
							saved_task_offset);
					goto next;
				}
			}

			if (saved_task_offset)
				*task_offset = saved_task_offset;
		} else {
			se_mem->se_page = sg_page(&sg_s[j]);
			se_mem->se_off = (*task_offset + sg_s[j].offset);

			if ((sg_s[j].length - *task_offset) > task_size) {
				se_mem->se_len = task_size;

				task_size -= se_mem->se_len;
				if (!(task_size)) {
					*task_offset += se_mem->se_len;
					goto next;
				}
			} else
				se_mem->se_len = (sg_s[j++].length -
						*task_offset);

			saved_task_offset = *task_offset;
			*task_offset = 0;
		}
		task_size -= se_mem->se_len;
next:
		list_add_tail(&se_mem->se_list, se_mem_list);
		(*se_mem_cnt)++;
	}

	DEBUG_MEM("task[0] - Mapped(%u) struct scatterlist segments to(%u)"
		" se_mem_t\n", j, *se_mem_cnt);

	return 0;
}
#endif

#ifndef MY_ABC_HERE
int transport_map_mem_to_mem(
	se_task_t *task,
	struct list_head *se_mem_list,
	void *in_mem,
	se_mem_t *in_se_mem,
	se_mem_t **out_se_mem,
	u32 *se_mem_cnt,
	u32 *task_offset)
{
	se_mem_t *se_mem = in_se_mem, *se_mem_new;
	u32 saved_task_offset = 0, task_size = task->task_size;

	if (!se_mem) {
		printk(KERN_ERR "Invalid se_mem_t pointer\n");
		return -1;
	}

	while (task_size) {
		se_mem_new = kmem_cache_zalloc(se_mem_cache, GFP_KERNEL);
		if (!(se_mem_new)) {
			printk(KERN_ERR "Unable to allocate se_mem_t\n");
			return -1;
		}
		INIT_LIST_HEAD(&se_mem_new->se_list);

		if (*task_offset == 0) {
			se_mem_new->se_page = se_mem->se_page;
			se_mem_new->se_off = se_mem->se_off;

			if (task_size >= se_mem->se_len) {
				se_mem_new->se_len = se_mem->se_len;

				se_mem = list_entry(se_mem->se_list.next,
							se_mem_t, se_list);
				if (!(se_mem)) {
					printk(KERN_ERR "Unable to locate next"
							" se_mem_t\n");
					return -1;
				}
			} else {
				se_mem_new->se_len = task_size;

				task_size -= se_mem_new->se_len;
				if (!(task_size)) {
					*task_offset = (se_mem_new->se_len +
							saved_task_offset);
					goto next;
				}
			}

			if (saved_task_offset)
				*task_offset = saved_task_offset;
		} else {
			se_mem_new->se_page = se_mem->se_page;
			se_mem_new->se_off = (*task_offset + se_mem->se_off);

			if ((se_mem->se_len - *task_offset) > task_size) {
				se_mem_new->se_len = task_size;

				if (!(task_size -= se_mem_new->se_len)) {
					*task_offset += se_mem_new->se_len;
					goto next;
				}
			} else {
				se_mem_new->se_len = (se_mem->se_len -
							*task_offset);

				se_mem = list_entry(se_mem->se_list.next,
							se_mem_t, se_list);
				if (!(se_mem)) {
					printk(KERN_ERR "Unable to locate next"
							" se_mem_t\n");
					return -1;
				}
			}

			saved_task_offset = *task_offset;
			*task_offset = 0;
		}
		task_size -= se_mem_new->se_len;
next:
		list_add_tail(&se_mem_new->se_list, se_mem_list);
		(*se_mem_cnt)++;

		DEBUG_MEM2("task[%u] - se_mem_cnt(%u) se_page(%p) se_off(%u)"
			" se_len(%u)\n", task->task_no, *se_mem_cnt,
			se_mem_new->se_page, se_mem_new->se_off,
			se_mem->se_len);
		DEBUG_MEM2("task[%u] - Reducing task_size to(%u)\n",
			task->task_no, task_size);
	}
	*out_se_mem = se_mem;

	return 0;
}
#endif

#ifdef MY_ABC_HERE
int transport_map_mem_to_sg(
	se_task_t *task,
	struct list_head *se_mem_list,
	void *in_mem,
	se_mem_t *in_se_mem,
	se_mem_t **out_se_mem,
	u32 *se_mem_cnt,
	u32 *task_offset)
{
	se_cmd_t *se_cmd = task->task_se_cmd;
	se_mem_t *se_mem = in_se_mem;
	struct scatterlist *sg = (struct scatterlist *)in_mem;
	u32 task_size = task->task_size, sg_no = 0;

	if (!sg) {
		printk(KERN_ERR "Unable to locate valid struct"
				" scatterlist pointer\n");
		return -1;
	}

	while (task_size != 0) {
		 
		sg_assign_page(sg, se_mem->se_page);

		if (*task_offset == 0) {
			sg->offset = se_mem->se_off;

			if (task_size >= se_mem->se_len) {
				sg->length = se_mem->se_len;

				if (!(list_is_last(&se_mem->se_list,
						T_TASK(se_cmd)->t_mem_list))) {
					se_mem = list_entry(se_mem->se_list.next,
							se_mem_t, se_list);
					(*se_mem_cnt)++;
				}
			} else {
				sg->length = task_size;
				 
				task_size -= sg->length;
				if (!(task_size))
					*task_offset = sg->length;

				goto next;
			}
		} else {
			sg->offset = (*task_offset + se_mem->se_off);

			if ((se_mem->se_len - *task_offset) > task_size) {
				sg->length = task_size;
				 
				task_size -= sg->length;
				if (!(task_size))
					*task_offset += sg->length;

				goto next;
			} else {
				sg->length = (se_mem->se_len - *task_offset);

				if (!(list_is_last(&se_mem->se_list,
						T_TASK(se_cmd)->t_mem_list))) {
					se_mem = list_entry(se_mem->se_list.next,
							se_mem_t, se_list);
					(*se_mem_cnt)++;
				}
			}

			*task_offset = 0;
		}
		task_size -= sg->length;
next:
		DEBUG_MEM("task[%u] mem_to_sg - sg[%u](%p)(%u)(%u) - Reducing"
			" task_size to(%u), task_offset: %u\n", task->task_no, sg_no,
			sg_page(sg), sg->length, sg->offset, task_size, *task_offset);

		sg_no++;
		if (!(task_size))
			break;

		sg = sg_next(sg);

		if (task_size > se_cmd->data_length)
			BUG();
	}
	*out_se_mem = se_mem;

	DEBUG_MEM("task[%u] - Mapped(%u) struct se_mem segments to total(%u)"
		" SGs\n", task->task_no, *se_mem_cnt, sg_no);

	return 0;
}
#else
int transport_map_mem_to_sg(
	se_task_t *task,
	struct list_head *se_mem_list,
	void *in_mem,
	se_mem_t *in_se_mem,
	se_mem_t **out_se_mem,
	u32 *se_mem_cnt,
	u32 *task_offset)
{
	se_mem_t *se_mem = in_se_mem;
	struct scatterlist *sg = (struct scatterlist *)in_mem;
	u32 saved_task_offset = 0, sg_no = 0;
	u32 task_size = task->task_size;

	if (!sg) {
		printk(KERN_ERR "Unable to locate valid struct"
				" scatterlist pointer\n");
		return -1;
	}

	while (task_size) {
		if (*task_offset == 0) {
			sg_assign_page(&sg[sg_no], se_mem->se_page);
			sg[sg_no].offset = se_mem->se_off;

			if (task_size >= se_mem->se_len) {
				sg[sg_no].length = se_mem->se_len;

				se_mem = list_entry(se_mem->se_list.next,
							se_mem_t, se_list);
				if (!(se_mem)) {
					printk(KERN_ERR "Unable to locate"
						" next se_mem_t\n");
					return -1;
				}
				(*se_mem_cnt)++;
			} else {
				sg[sg_no].length = task_size;
				 
				task_size -= sg[sg_no].length;
				if (!(task_size)) {
					*task_offset = (sg[sg_no].length +
							saved_task_offset);
					goto next;
				}
			}

			if (saved_task_offset)
				*task_offset = saved_task_offset;
		} else {
			sg_assign_page(&sg[sg_no], se_mem->se_page);
			sg[sg_no].offset = (*task_offset + se_mem->se_off);

			if ((se_mem->se_len - *task_offset) > task_size) {
				sg[sg_no].length = task_size;
				 
				task_size -= sg[sg_no].length;
				if (!(task_size)) {
					*task_offset += sg[sg_no].length;
					goto next;
				}
			} else {
				sg[sg_no].length = (se_mem->se_len -
						*task_offset);

				se_mem = list_entry(se_mem->se_list.next,
						se_mem_t, se_list);
				if (!(se_mem)) {
					printk(KERN_ERR "Unable to locate"
						" next se_mem_t\n");
					return -1;
				}
				(*se_mem_cnt)++;
			}

			saved_task_offset = *task_offset;
			*task_offset = 0;
		}
		task_size -= sg[sg_no].length;
next:
		DEBUG_MEM("task[%u] - sg[%u](%p)(%u)(%u) - Reducing task_size"
			" to(%u)\n", task->task_no, sg_no,
			sg_page(&sg[sg_no]), sg[sg_no].length,
			sg[sg_no].offset, task_size);

		sg_no++;
	}
	*out_se_mem = se_mem;
	task->task_sg_num = sg_no;

	DEBUG_MEM("task[%u] - Mapped(%u) se_mem_t segments to total(%u) SGs"
		" saved task_offset(%u)\n", task->task_no, *se_mem_cnt,
			sg_no, *task_offset);

	return 0;
}
#endif

#ifdef MY_ABC_HERE
u32 transport_generic_get_cdb_count(
	se_cmd_t *cmd,
	se_transform_info_t *ti,
	void *head_obj_ptr,
	unsigned long long starting_lba,
	u32 sectors,
	se_mem_t *se_mem_in,
	se_mem_t **se_mem_out,
	u32 *task_offset_in)
{
	unsigned char *cdb = NULL;
	void *obj_ptr;
	se_task_t *task;
	se_mem_t *se_mem, *se_mem_lout = NULL;
	struct se_device_s *dev = SE_DEV(cmd);
	int max_sectors_set = 0, ret;
	u32 se_mem_cnt = 0, task_cdbs = 0;
	unsigned long long lba;

	if (!se_mem_in) {
		list_for_each_entry(se_mem_in, T_TASK(cmd)->t_mem_list, se_list)
			break;

		if (!se_mem_in) {
			printk(KERN_ERR "se_mem_in is NULL\n");
			return 0;
		}
	}
	se_mem = se_mem_in;

	ret = transport_generic_obj_start(ti, head_obj_ptr, starting_lba);
	if (ret < 0)
		return 0;

	lba = ti->ti_lba;
	obj_ptr = ti->ti_obj_ptr;
	DEBUG_VOL("Starting Physical LBA(%llu)\n", lba);

	while (sectors) {
		DEBUG_VOL("ITT[0x%08x] LBA(%llu) SectorsLeft(%u) EOBJ(%llu)\n",
			CMD_TFO(cmd)->get_task_tag(cmd), lba, sectors,
			dev_obj_end_lba(obj_ptr));

#ifdef SYNO_LIO_TRANSPORT_PATCHES
		task = transport_generic_get_task(ti, cmd, obj_ptr);
#else
		task = cmd->transport_get_task(ti, cmd, obj_ptr);
#endif
		if (!(task))
			goto out;

		transport_set_task_sectors(task, obj_ptr, lba, sectors,
				&max_sectors_set);

		task->task_lba = lba;
		lba += task->task_sectors;
		sectors -= task->task_sectors;
		task->task_size = (task->task_sectors *
				DEV_ATTRIB(dev)->block_size);
		task->transport_map_task = dev_obj_get_map_SG(
				obj_ptr, cmd->data_direction);
		cdb = TRANSPORT(dev)->get_cdb(task);
		if ((cdb)) {
			memcpy(cdb, T_TASK(cmd)->t_task_cdb, SCSI_CDB_SIZE);
			cmd->transport_split_cdb(task->task_lba,
					&task->task_sectors, cdb);
		}

		ret = dev_obj_do_se_mem_map(obj_ptr, task,
				T_TASK(cmd)->t_mem_list, NULL, se_mem,
				&se_mem_lout, &se_mem_cnt, task_offset_in);
		if (ret < 0)
			goto out;

		se_mem = se_mem_lout;
		*se_mem_out = se_mem_lout;
		task_cdbs++;

		DEBUG_VOL("Incremented task_cdbs(%u) task->task_sg_num(%u)\n",
				task_cdbs, task->task_sg_num);

		if (max_sectors_set) {
			max_sectors_set = 0;
			continue;
		}

		if (!sectors)
			break;

	}

	if (ti->ti_set_counts) {
		atomic_inc(&T_TASK(cmd)->t_fe_count);
		atomic_inc(&T_TASK(cmd)->t_se_count);
	}

	DEBUG_VOL("ITT[0x%08x] total cdbs(%u)\n",
		CMD_TFO(cmd)->get_task_tag(cmd), task_cdbs);

	return task_cdbs;
out:
	return 0;
}
 
#else
u32 transport_generic_get_cdb_count(
	se_cmd_t *cmd,
	se_transform_info_t *ti,
	se_obj_lun_type_t *head_obj_api,
	void *head_obj_ptr,
	unsigned long long starting_lba,
	u32 sectors,
	se_mem_t *se_mem_in,
	se_mem_t **se_mem_out,
	u32 *task_offset_in)
{
	unsigned char *cdb = NULL;
	void *obj_ptr, *next_obj_ptr = NULL;
	se_task_t *task;
	se_mem_t *se_mem, *se_mem_lout = NULL;
	se_obj_lun_type_t *obj_api;
	int max_sectors_set = 0, ret;
	u32 se_mem_cnt = 0, task_cdbs = 0;
	unsigned long long lba;

	if (!se_mem_in) {
		list_for_each_entry(se_mem_in, T_TASK(cmd)->t_mem_list, se_list)
			break;

		if (!se_mem_in) {
			printk(KERN_ERR "se_mem_in is NULL\n");
			return 0;
		}
	}
	se_mem = se_mem_in;

	head_obj_api->obtain_obj_lock(head_obj_ptr);
	if (head_obj_api->obj_start(head_obj_ptr, ti, starting_lba) < 0) {
		head_obj_api->release_obj_lock(head_obj_ptr);
		return 0;
	}

	lba = ti->ti_lba;
	obj_api = ti->ti_obj_api;
	obj_ptr = ti->ti_obj_ptr;
	DEBUG_VOL("Starting Physical LBA(%llu) for head_obj_api->(%p)\n",
			lba, head_obj_api);

	while (sectors) {
		if (!obj_api) {
			head_obj_api->release_obj_lock(head_obj_ptr);
			printk(KERN_ERR "obj_api is NULL LBA(%llu)->Sectors"
				"(%u)\n", lba, sectors);
			return 0;
		}

		DEBUG_VOL("ITT[0x%08x] LBA(%llu) SectorsLeft(%u) EOBJ(%llu)\n",
			CMD_TFO(cmd)->get_task_tag(cmd), lba, sectors,
			obj_api->end_lba(obj_ptr, 1));

		head_obj_api->release_obj_lock(head_obj_ptr);

		task = cmd->transport_get_task(ti, cmd, obj_ptr, obj_api);
		if (!(task))
			goto out;

		transport_set_task_sectors(task, obj_api, obj_ptr, lba,
				sectors, &max_sectors_set);

		task->task_lba = lba;
		lba += task->task_sectors;
		sectors -= task->task_sectors;
		task->task_size = (task->task_sectors *
					obj_api->blocksize(obj_ptr));
		task->transport_map_task =
			obj_api->get_map_SG(obj_ptr, cmd->data_direction);

		cdb = obj_api->get_cdb(obj_ptr, task);
		if ((cdb)) {
			memcpy(cdb, T_TASK(cmd)->t_task_cdb, SCSI_CDB_SIZE);
			cmd->transport_split_cdb(task->task_lba,
					&task->task_sectors, cdb);
		}

		ret = obj_api->do_se_mem_map(obj_ptr, task,
				T_TASK(cmd)->t_mem_list, NULL, se_mem,
				&se_mem_lout, &se_mem_cnt, task_offset_in);
		if (ret < 0)
			goto out;

		head_obj_api->obtain_obj_lock(head_obj_ptr);

		se_mem = se_mem_lout;
		*se_mem_out = se_mem_lout;
		task_cdbs++;

		DEBUG_VOL("Incremented task_cdbs(%u) task->task_sg_num(%u)\n",
				task_cdbs, task->task_sg_num);

		if (max_sectors_set) {
			max_sectors_set = 0;
			continue;
		}

		if (!sectors)
			break;

		obj_api = obj_api->get_next_obj_api(obj_ptr, &next_obj_ptr);
		if (obj_api) {
			obj_ptr = next_obj_ptr;
			lba = obj_api->get_next_lba(obj_ptr, lba);
		}
	}
	head_obj_api->release_obj_lock(head_obj_ptr);

	if (ti->ti_set_counts) {
		atomic_inc(&T_TASK(cmd)->t_fe_count);
		atomic_inc(&T_TASK(cmd)->t_se_count);
	}

	DEBUG_VOL("ITT[0x%08x] total cdbs(%u)\n",
		CMD_TFO(cmd)->get_task_tag(cmd), task_cdbs);

	return task_cdbs;
out:
	return 0;
}
#endif

int transport_generic_new_cmd(se_cmd_t *cmd)
{
	se_portal_group_t *se_tpg;
	se_transform_info_t ti;
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	struct se_task_s *task;
#endif
	int ret = 0;
	 
	memset((void *)&ti, 0, sizeof(se_transform_info_t));
	ti.ti_se_cmd = cmd;
	ti.se_obj_ptr = SE_LUN(cmd)->lun_type_ptr;
#ifndef MY_ABC_HERE
	ti.se_obj_api = SE_LUN(cmd)->lun_obj_api;
#endif

	if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH)) {
		 
		if (!(cmd->se_cmd_flags & SCF_PASSTHROUGH_SG_TO_MEM_NOALLOC)) {
			 
			ret = cmd->transport_allocate_resources(cmd,
					cmd->data_length, PAGE_SIZE);
			if (ret < 0)
				goto failure;
		}

#ifdef MY_ABC_HERE
		ret = transport_get_sectors(cmd, SE_LUN(cmd)->lun_type_ptr);
		if (ret < 0)
			goto failure;

		ret = transport_new_cmd_obj(cmd, &ti,
					SE_LUN(cmd)->lun_type_ptr, 0);
		if (ret < 0)
			goto failure;
#else
		ret = transport_get_sectors(cmd, SE_LUN(cmd)->lun_obj_api,
					SE_LUN(cmd)->lun_type_ptr);
		if (ret < 0)
			goto failure;

		ret = transport_new_cmd_obj(cmd, &ti, SE_LUN(cmd)->lun_obj_api,
					SE_LUN(cmd)->lun_type_ptr, 0);
		if (ret < 0)
			goto failure;
#endif
		 
		se_tpg = SE_LUN(cmd)->lun_sep->sep_tpg;
		if (TPG_TFO(se_tpg)->alloc_cmd_iovecs != NULL) {
			ret = TPG_TFO(se_tpg)->alloc_cmd_iovecs(cmd);
			if (ret < 0) {
				ret = PYX_TRANSPORT_OUT_OF_MEMORY_RESOURCES;
				goto failure;
			}
		}
	}
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	if (!(cmd->transport_cdb_transform)) {
		dump_stack();
		ret = PYX_TRANSPORT_OUT_OF_MEMORY_RESOURCES;
		goto failure;
	}

	if (cmd->transport_cdb_transform(cmd, &ti) < 0) {
		ret = PYX_TRANSPORT_OUT_OF_MEMORY_RESOURCES;
		goto failure;
	}
#else
	 
	if (cmd->transport_do_transform(cmd, &ti) < 0) {
		ret = PYX_TRANSPORT_OUT_OF_MEMORY_RESOURCES;
		goto failure;
	}
#endif
	 
#ifdef SYNO_LIO_TRANSPORT_PATCHES
	list_for_each_entry(task, &T_TASK(cmd)->t_task_list, t_list) {
		if (atomic_read(&task->task_sent))
			continue;

		ret = task->transport_map_task(task, task->task_size);
		if (ret < 0)
			goto failure;
	}
#else
	ret = cmd->transport_map_buffers_to_tasks(cmd);
	if (ret < 0)
		goto failure;
#endif
	 
#ifdef MY_ABC_HERE
	if (cmd->data_direction == DMA_TO_DEVICE) {
		transport_add_tasks_to_state_queue(cmd);
		return transport_generic_write_pending(cmd);
	}
#else
	if (cmd->data_direction == SE_DIRECTION_WRITE) {
		transport_add_tasks_to_state_queue(cmd);
		return transport_generic_write_pending(cmd);
	}
#endif
	 
	transport_execute_tasks(cmd);
	return 0;

failure:
	return ret;
}

void transport_generic_process_write(se_cmd_t *cmd)
{
#if 0
	 
	if (cmd->se_cmd_flags & SCF_UNDERFLOW_BIT) {
		if (!T_TASK(cmd)->t_task_se_num) {
			unsigned char *dst, *buf =
				(unsigned char *)T_TASK(cmd)->t_task_buf;

			dst = kzalloc(cmd->cmd_spdtl), GFP_KERNEL);
			if (!(dst)) {
				printk(KERN_ERR "Unable to allocate memory for"
						" WRITE underflow\n");
				transport_generic_request_failure(cmd, NULL,
					PYX_TRANSPORT_REQ_TOO_MANY_SECTORS, 1);
				return;
			}
			memcpy(dst, buf, cmd->cmd_spdtl);

			kfree(T_TASK(cmd)->t_task_buf);
			T_TASK(cmd)->t_task_buf = dst;
		} else {
			struct scatterlist *sg =
				(struct scatterlist *sg)T_TASK(cmd)->t_task_buf;
			struct scatterlist *orig_sg;

			orig_sg = kzalloc(sizeof(struct scatterlist) *
					T_TASK(cmd)->t_task_se_num,
					GFP_KERNEL))) {
			if (!(orig_sg)) {
				printk(KERN_ERR "Unable to allocate memory"
						" for WRITE underflow\n");
				transport_generic_request_failure(cmd, NULL,
					PYX_TRANSPORT_REQ_TOO_MANY_SECTORS, 1);
				return;
			}

			memcpy(orig_sg, T_TASK(cmd)->t_task_buf,
					sizeof(struct scatterlist) *
					T_TASK(cmd)->t_task_se_num);

			cmd->data_length = cmd->cmd_spdtl;
			 
			if (transport_generic_new_cmd(cmd) < 0) {
				transport_generic_request_failure(cmd, NULL,
					PYX_TRANSPORT_REQ_TOO_MANY_SECTORS, 1);
				kfree(orig_sg);
				return;
			}

			transport_memcpy_write_sg(cmd, orig_sg);
		}
	}
#endif
	transport_execute_tasks(cmd);
}
EXPORT_SYMBOL(transport_generic_process_write);

static int transport_generic_write_pending(se_cmd_t *cmd)
{
	unsigned long flags;
	int ret;

	if (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH) {
		if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH_NOALLOC)) {
			complete(&T_TASK(cmd)->t_transport_passthrough_wcomp);
			transport_cmd_check_stop(cmd, 1, 0);
			return PYX_TRANSPORT_WRITE_PENDING;
		}

		transport_generic_process_write(cmd);
		transport_cmd_check_stop(cmd, 1, 0);
		return PYX_TRANSPORT_WRITE_PENDING;
	}

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	cmd->t_state = TRANSPORT_WRITE_PENDING;
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
	 
	ret = CMD_TFO(cmd)->write_pending(cmd);
	if (ret < 0) {
		transport_cmd_check_stop(cmd, 1, 0);
		return ret;
	}

	transport_cmd_check_stop(cmd, 1, 0);
	return PYX_TRANSPORT_WRITE_PENDING;
}

void transport_release_cmd_to_pool(se_cmd_t *cmd)
{
	if (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH)
		kfree(cmd->se_lun);

#ifdef SYNO_LIO_TRANSPORT_PATCHES
	 
	if (!(cmd->se_fabric_cmd_ptr)) {
		transport_free_se_cmd(cmd);
		 
		if ((T_TASK(cmd) && (CMD_TFO(cmd))))
			CMD_TFO(cmd)->release_cmd_to_pool(cmd);
		else {
			printk(KERN_ERR "T_TASK(cmd) && (CMD_TFO(cmd) NULL for"
					" se_fabric_cmd_ptr=NULL inside of"
					" transport_release_cmd_to_pool()\n");
			dump_stack();
		}

		return;
	}
	 
	if ((T_TASK(cmd) && (CMD_TFO(cmd))))
		CMD_TFO(cmd)->release_cmd_to_pool(cmd);
	else {
		dump_stack();
		printk(KERN_ERR "NULL T_TASK(cmd) && (CMD_TFO(cmd) for"
				" se_fabric_cmd_ptr=1 inside of"
				" transport_release_cmd_to_pool()\n");
	}
#else
	 
	CMD_TFO(cmd)->release_cmd_to_pool(cmd);
#endif

	transport_free_se_cmd(cmd);
}
EXPORT_SYMBOL(transport_release_cmd_to_pool);

void transport_generic_free_cmd(
	se_cmd_t *cmd,
	int wait_for_tasks,
	int release_to_pool,
	int session_reinstatement)
{
	if (!(cmd->se_cmd_flags & SCF_SE_LUN_CMD) || !T_TASK(cmd))
		transport_release_cmd_to_pool(cmd);
	else {
		core_dec_lacl_count(cmd->se_sess->se_node_acl, cmd);

		if (SE_LUN(cmd)) {
#if 0
			printk(KERN_INFO "cmd: %p ITT: 0x%08x contains"
				" SE_LUN(cmd)\n", cmd,
				CMD_TFO(cmd)->get_task_tag(cmd));
#endif
			transport_lun_remove_cmd(cmd);
		}

		if (wait_for_tasks && cmd->transport_wait_for_tasks)
			cmd->transport_wait_for_tasks(cmd, 0, 0);

		transport_generic_remove(cmd, release_to_pool,
				session_reinstatement);
	}
}
EXPORT_SYMBOL(transport_generic_free_cmd);

static void transport_nop_wait_for_tasks(
	se_cmd_t *cmd,
	int remove_cmd,
	int session_reinstatement)
{
	return;
}

int transport_lun_wait_for_tasks(se_cmd_t *cmd, se_lun_t *lun)
{
	unsigned long flags;
#ifdef MY_ABC_HERE
	int ret;
#endif
	 
	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (atomic_read(&T_TASK(cmd)->t_transport_stop)) {
		atomic_set(&T_TASK(cmd)->transport_lun_stop, 0);
		DEBUG_TRANSPORT_S("ConfigFS ITT[0x%08x] - t_transport_stop =="
			" TRUE, skipping\n", CMD_TFO(cmd)->get_task_tag(cmd));
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
#ifdef MY_ABC_HERE
		transport_cmd_check_stop(cmd, 1, 0);
#endif
		return -1;
	}
#ifdef MY_ABC_HERE
	atomic_set(&T_TASK(cmd)->transport_lun_fe_stop, 1);
#endif
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

#ifdef MY_ABC_HERE
	wake_up_interruptible(&SE_DEV(cmd)->dev_queue_obj->thread_wq);
#else
	CMD_ORIG_OBJ_API(cmd)->notify_obj(cmd->se_orig_obj_ptr);
#endif

#ifdef MY_ABC_HERE
	ret = transport_stop_tasks_for_cmd(cmd);
	DEBUG_TRANSPORT_S("ConfigFS: cmd: %p t_task_cdbs: %d stop tasks ret:"
			" %d\n", cmd, T_TASK(cmd)->t_task_cdbs, ret);
	if (!ret) {
		DEBUG_TRANSPORT_S("ConfigFS: ITT[0x%08x] - stopping cmd....\n",
				CMD_TFO(cmd)->get_task_tag(cmd));
		wait_for_completion(&T_TASK(cmd)->transport_lun_stop_comp);
		DEBUG_TRANSPORT_S("ConfigFS: ITT[0x%08x] - stopped cmd....\n",
				CMD_TFO(cmd)->get_task_tag(cmd));
	}
#ifdef MY_ABC_HERE
	transport_remove_cmd_from_queue(cmd, SE_DEV(cmd)->dev_queue_obj);
#else
	transport_remove_cmd_from_queue(cmd,
		CMD_ORIG_OBJ_API(cmd)->get_queue_obj(cmd->se_orig_obj_ptr));
#endif
#else
	DEBUG_TRANSPORT_S("ConfigFS: ITT[0x%08x] - stopping cmd....\n",
		CMD_TFO(cmd)->get_task_tag(cmd));
	wait_for_completion(&T_TASK(cmd)->transport_lun_stop_comp);
	DEBUG_TRANSPORT_S("ConfigFS: ITT[0x%08x] - stopped cmd....\n",
		CMD_TFO(cmd)->get_task_tag(cmd));

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	atomic_set(&T_TASK(cmd)->transport_lun_stop, 0);
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
#endif

	return 0;
}
EXPORT_SYMBOL(transport_lun_wait_for_tasks);

#ifdef DEBUG_CLEAR_LUN
#define DEBUG_CLEAR_L(x...) printk(KERN_INFO x)
#else
#define DEBUG_CLEAR_L(x...)
#endif

#ifdef MY_ABC_HERE
static void __transport_clear_lun_from_sessions(se_lun_t *lun)
{
	se_cmd_t *cmd = NULL;
	unsigned long lun_flags, cmd_flags;
	 
	spin_lock_irqsave(&lun->lun_cmd_lock, lun_flags);
	while (!list_empty_careful(&lun->lun_cmd_list)) {
		cmd = list_entry(lun->lun_cmd_list.next,
				struct se_cmd_s, se_lun_list);
		list_del(&cmd->se_lun_list);

		if (!(T_TASK(cmd))) {
			printk(KERN_ERR "ITT: 0x%08x, T_TASK(cmd) = NULL"
				"[i,t]_state: %u/%u\n",
				CMD_TFO(cmd)->get_task_tag(cmd),
				CMD_TFO(cmd)->get_cmd_state(cmd), cmd->t_state);
			BUG();
		}
		atomic_set(&T_TASK(cmd)->transport_lun_active, 0);
		 
		spin_lock(&T_TASK(cmd)->t_state_lock);
		DEBUG_CLEAR_L("SE_LUN[%d] - Setting T_TASK(cmd)->transport"
			"_lun_stop for  ITT: 0x%08x\n",
			SE_LUN(cmd)->unpacked_lun,
			CMD_TFO(cmd)->get_task_tag(cmd));
		atomic_set(&T_TASK(cmd)->transport_lun_stop, 1);
		spin_unlock(&T_TASK(cmd)->t_state_lock);

		spin_unlock_irqrestore(&lun->lun_cmd_lock, lun_flags);

		if (!(SE_LUN(cmd))) {
			printk(KERN_ERR "ITT: 0x%08x, [i,t]_state: %u/%u\n",
				CMD_TFO(cmd)->get_task_tag(cmd),
				CMD_TFO(cmd)->get_cmd_state(cmd), cmd->t_state);
			BUG();
		}
		 
		DEBUG_CLEAR_L("SE_LUN[%d] - ITT: 0x%08x before transport"
			"_lun_wait_for_tasks()\n", SE_LUN(cmd)->unpacked_lun,
			CMD_TFO(cmd)->get_task_tag(cmd));

		if (transport_lun_wait_for_tasks(cmd, SE_LUN(cmd)) < 0) {
			spin_lock_irqsave(&lun->lun_cmd_lock, lun_flags);
			continue;
		}

		DEBUG_CLEAR_L("SE_LUN[%d] - ITT: 0x%08x after transport_lun"
			"_wait_for_tasks(): SUCCESS\n",
			SE_LUN(cmd)->unpacked_lun,
			CMD_TFO(cmd)->get_task_tag(cmd));

		spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, cmd_flags);
		if (!(atomic_read(&T_TASK(cmd)->transport_dev_active))) {
			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, cmd_flags);
			goto check_cond;
		}
		atomic_set(&T_TASK(cmd)->transport_dev_active, 0);
		transport_all_task_dev_remove_state(cmd);
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, cmd_flags);

               transport_free_dev_tasks(cmd);
		 
check_cond:
		transport_send_check_condition_and_sense(cmd,
				NON_EXISTENT_LUN, 0);
		 
		spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, cmd_flags);
		if (atomic_read(&T_TASK(cmd)->transport_lun_fe_stop)) {
			DEBUG_CLEAR_L("SE_LUN[%d] - Detected FE stop for"
				" se_cmd_t: %p ITT: 0x%08x\n",
				lun->unpacked_lun,
				cmd, CMD_TFO(cmd)->get_task_tag(cmd));

			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock,
					cmd_flags);
			transport_cmd_check_stop(cmd, 1, 0);
			complete(&T_TASK(cmd)->transport_lun_fe_stop_comp);
			spin_lock_irqsave(&lun->lun_cmd_lock, lun_flags);
			continue;
		}
		DEBUG_CLEAR_L("SE_LUN[%d] - ITT: 0x%08x finished processing\n",
			lun->unpacked_lun, CMD_TFO(cmd)->get_task_tag(cmd));

		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, cmd_flags);
		spin_lock_irqsave(&lun->lun_cmd_lock, lun_flags);
	}
	spin_unlock_irqrestore(&lun->lun_cmd_lock, lun_flags);
}
EXPORT_SYMBOL(__transport_clear_lun_from_sessions);

static int transport_clear_lun_thread(void *p)
{
	struct se_lun_s *lun = (struct se_lun_s *)p;

	__transport_clear_lun_from_sessions(lun);
	complete(&lun->lun_shutdown_comp);

	return 0;
}

int transport_clear_lun_from_sessions(struct se_lun_s *lun)
{
	struct task_struct *kt;

	kt = kthread_run(transport_clear_lun_thread, (void *)lun,
			"tcm_cl_%u", lun->unpacked_lun);
	if (IS_ERR(kt)) {
		printk(KERN_ERR "Unable to start clear_lun thread\n");
		return -1;
	}
	wait_for_completion(&lun->lun_shutdown_comp);

	return 0;
}
EXPORT_SYMBOL(transport_clear_lun_from_sessions);

#else

void transport_clear_lun_from_sessions(se_lun_t *lun)
{
	se_cmd_t *cmd = NULL, *cmd_p = NULL;
	unsigned long flags;
	 
	spin_lock_irqsave(&lun->lun_cmd_lock, flags);
	list_for_each_entry_safe(cmd, cmd_p, &lun->lun_cmd_list, se_lun_list) {
		if (!(T_TASK(cmd))) {
			printk(KERN_ERR "ITT: 0x%08x, T_TASK(cmd) = NULL"
				"[i,t]_state: %u/%u\n",
				CMD_TFO(cmd)->get_task_tag(cmd),
				CMD_TFO(cmd)->get_cmd_state(cmd), cmd->t_state);
			BUG();
		}
		list_del(&cmd->se_lun_list);
		atomic_set(&T_TASK(cmd)->transport_lun_active, 0);
		 
		spin_lock(&T_TASK(cmd)->t_state_lock);
		DEBUG_CLEAR_L("SE_LUN[%d] - Setting T_TASK(cmd)->transport"
			"_lun_stop for  ITT: 0x%08x\n",
			SE_LUN(cmd)->unpacked_lun,
			CMD_TFO(cmd)->get_task_tag(cmd));
		atomic_set(&T_TASK(cmd)->transport_lun_stop, 1);
		spin_unlock(&T_TASK(cmd)->t_state_lock);

		spin_unlock_irqrestore(&lun->lun_cmd_lock, flags);

		if (!(SE_LUN(cmd))) {
			printk(KERN_ERR "ITT: 0x%08x, [i,t]_state: %u/%u\n",
				CMD_TFO(cmd)->get_task_tag(cmd),
				CMD_TFO(cmd)->get_cmd_state(cmd), cmd->t_state);
			BUG();
		}
		 
		DEBUG_CLEAR_L("SE_LUN[%d] - ITT: 0x%08x before transport"
			"_lun_wait_for_tasks()\n", SE_LUN(cmd)->unpacked_lun,
			CMD_TFO(cmd)->get_task_tag(cmd));

		if (transport_lun_wait_for_tasks(cmd, SE_LUN(cmd)) < 0) {
			spin_lock_irqsave(&lun->lun_cmd_lock, flags);
			continue;
		}

		DEBUG_CLEAR_L("SE_LUN[%d] - ITT: 0x%08x after transport_lun"
			"_wait_for_tasks(): SUCCESS\n",
			SE_LUN(cmd)->unpacked_lun,
			CMD_TFO(cmd)->get_task_tag(cmd));
		 
		transport_send_check_condition_and_sense(cmd,
				NON_EXISTENT_LUN, 0);
		 
		spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
		if (atomic_read(&T_TASK(cmd)->transport_lun_fe_stop)) {
			DEBUG_CLEAR_L("SE_LUN[%d] - Detected FE stop for"
				" se_cmd_t: %p ITT: 0x%08x\n",
				lun->unpacked_lun,
				cmd, CMD_TFO(cmd)->get_task_tag(cmd));

			spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock,
					flags);
			complete(&T_TASK(cmd)->transport_lun_fe_stop_comp);
			spin_lock_irqsave(&lun->lun_cmd_lock, flags);
			continue;
		}
		atomic_set(&T_TASK(cmd)->transport_lun_stop, 0);

		DEBUG_CLEAR_L("SE_LUN[%d] - ITT: 0x%08x finished processing\n",
			lun->unpacked_lun, CMD_TFO(cmd)->get_task_tag(cmd));

		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		spin_lock_irqsave(&lun->lun_cmd_lock, flags);
	}
	spin_unlock_irqrestore(&lun->lun_cmd_lock, flags);
}
EXPORT_SYMBOL(transport_clear_lun_from_sessions);
#endif

static void transport_generic_wait_for_tasks(
	se_cmd_t *cmd,
	int remove_cmd,
	int session_reinstatement)
{
	unsigned long flags;

#ifdef MY_ABC_HERE
	if (!(cmd->se_cmd_flags & SCF_SE_LUN_CMD) && !(cmd->se_tmr_req))
		return;
#else
	if (!(cmd->se_cmd_flags & SCF_SE_LUN_CMD))
		return;
#endif

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	 
	if (atomic_read(&T_TASK(cmd)->transport_lun_stop)) {
#ifndef MY_ABC_HERE
		atomic_set(&T_TASK(cmd)->transport_lun_fe_stop, 1);
#endif

		DEBUG_TRANSPORT_S("wait_for_tasks: Stopping"
			" wait_for_completion(&T_TASK(cmd)transport_lun_fe"
			"_stop_comp); for ITT: 0x%08x\n",
			CMD_TFO(cmd)->get_task_tag(cmd));
		 
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		complete(&T_TASK(cmd)->transport_lun_stop_comp);
		wait_for_completion(&T_TASK(cmd)->transport_lun_fe_stop_comp);
		spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);

#ifdef MY_ABC_HERE
		transport_all_task_dev_remove_state(cmd);
#endif
		 
		DEBUG_TRANSPORT_S("wait_for_tasks: Stopped"
			" wait_for_completion(&T_TASK(cmd)transport_lun_fe_"
			"stop_comp); for ITT: 0x%08x\n",
			CMD_TFO(cmd)->get_task_tag(cmd));

#ifndef MY_ABC_HERE
		atomic_set(&T_TASK(cmd)->transport_lun_fe_stop, 0);
#endif
		atomic_set(&T_TASK(cmd)->transport_lun_stop, 0);
	}
	if (!atomic_read(&T_TASK(cmd)->t_transport_active))
		goto remove;

	atomic_set(&T_TASK(cmd)->t_transport_stop, 1);

	DEBUG_TRANSPORT_S("wait_for_tasks: Stopping %p ITT: 0x%08x"
		" i_state: %d, t_state/def_t_state: %d/%d, t_transport_stop"
		" = TRUE\n", cmd, CMD_TFO(cmd)->get_task_tag(cmd),
		CMD_TFO(cmd)->get_cmd_state(cmd), cmd->t_state,
		cmd->deferred_t_state);

	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

#ifdef MY_ABC_HERE
	wake_up_interruptible(&SE_DEV(cmd)->dev_queue_obj->thread_wq);
#else
	CMD_ORIG_OBJ_API(cmd)->notify_obj(cmd->se_orig_obj_ptr);
#endif

	wait_for_completion(&T_TASK(cmd)->t_transport_stop_comp);

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	atomic_set(&T_TASK(cmd)->t_transport_active, 0);
	atomic_set(&T_TASK(cmd)->t_transport_stop, 0);

	DEBUG_TRANSPORT_S("wait_for_tasks: Stopped wait_for_compltion("
		"&T_TASK(cmd)->t_transport_stop_comp) for ITT: 0x%08x\n",
		CMD_TFO(cmd)->get_task_tag(cmd));
remove:
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
	if (!remove_cmd)
		return;

	transport_generic_free_cmd(cmd, 0, 0, session_reinstatement);
}

int transport_get_sense_codes(
	se_cmd_t *cmd,
	u8 *asc,
	u8 *ascq)
{
	*asc = cmd->scsi_asc;
	*ascq = cmd->scsi_ascq;

	return 0;
}

int transport_set_sense_codes(
	se_cmd_t *cmd,
	u8 asc,
	u8 ascq)
{
	cmd->scsi_asc = asc;
	cmd->scsi_ascq = ascq;

	return 0;
}

int transport_send_check_condition_and_sense(
	se_cmd_t *cmd,
	u8 reason,
	int from_transport)
{
	unsigned char *buffer = cmd->sense_buffer;
	unsigned long flags;
	int offset;
	u8 asc = 0, ascq = 0;

	spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
	if (cmd->se_cmd_flags & SCF_SENT_CHECK_CONDITION) {
		spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);
		return 0;
	}
	cmd->se_cmd_flags |= SCF_SENT_CHECK_CONDITION;
	spin_unlock_irqrestore(&T_TASK(cmd)->t_state_lock, flags);

	if (!reason && from_transport)
		goto after_reason;

	if (!from_transport)
		cmd->se_cmd_flags |= SCF_EMULATED_TASK_SENSE;
	 
	offset = CMD_TFO(cmd)->set_fabric_sense_len(cmd,
				TRANSPORT_SENSE_BUFFER);
	 
	switch (reason) {
	case NON_EXISTENT_LUN:
	case UNSUPPORTED_SCSI_OPCODE:
	case SECTOR_COUNT_TOO_MANY:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x20;
		break;
	case UNKNOWN_MODE_PAGE:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x24;
		break;
	case CHECK_CONDITION_ABORT_CMD:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x29;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x03;
		break;
	case INCORRECT_AMOUNT_OF_DATA:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0c;
		 
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x0d;
		break;
	case INVALID_CDB_FIELD:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x24;
		break;
	case INVALID_PARAMETER_LIST:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x26;
		break;
	case UNEXPECTED_UNSOLICITED_DATA:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x0c;
		 
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x0c;
		break;
	case SERVICE_CRC_ERROR:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x47;
		 
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x05;
		break;
	case SNACK_REJECTED:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ABORTED_COMMAND;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x11;
		 
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = 0x13;
		break;
	case WRITE_PROTECTED:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = DATA_PROTECT;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x27;
		break;
	case CHECK_CONDITION_UNIT_ATTENTION:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = UNIT_ATTENTION;
		core_scsi3_ua_for_check_condition(cmd, &asc, &ascq);
		buffer[offset+SPC_ASC_KEY_OFFSET] = asc;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = ascq;
		break;
	case CHECK_CONDITION_NOT_READY:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = NOT_READY;
		transport_get_sense_codes(cmd, &asc, &ascq);
		buffer[offset+SPC_ASC_KEY_OFFSET] = asc;
		buffer[offset+SPC_ASCQ_KEY_OFFSET] = ascq;
		break;
	case LOGICAL_UNIT_COMMUNICATION_FAILURE:
	default:
		 
		buffer[offset] = 0x70;
		 
		buffer[offset+SPC_SENSE_KEY_OFFSET] = ILLEGAL_REQUEST;
		 
		buffer[offset+SPC_ASC_KEY_OFFSET] = 0x80;
		break;
	}
	 
	cmd->scsi_status = SAM_STAT_CHECK_CONDITION;
	 
	cmd->scsi_sense_length  = TRANSPORT_SENSE_BUFFER + offset;

after_reason:
	if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH))
		CMD_TFO(cmd)->queue_status(cmd);

	return 0;
}
EXPORT_SYMBOL(transport_send_check_condition_and_sense);

int transport_check_aborted_status(se_cmd_t *cmd, int send_status)
{
	int ret = 0;

	if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH))
		return 0;

	if (atomic_read(&T_TASK(cmd)->t_transport_aborted) != 0) {
		if (!(send_status) ||
		     (cmd->se_cmd_flags & SCF_SENT_DELAYED_TAS))
			return 1;
#if 0
		printk(KERN_INFO "Sending delayed SAM_STAT_TASK_ABORTED"
			" status for CDB: 0x%02x ITT: 0x%08x\n",
			T_TASK(cmd)->t_task_cdb[0],
			CMD_TFO(cmd)->get_task_tag(cmd));
#endif
		cmd->se_cmd_flags |= SCF_SENT_DELAYED_TAS;
		CMD_TFO(cmd)->queue_status(cmd);
		ret = 1;
	}
	return ret;
}
EXPORT_SYMBOL(transport_check_aborted_status);

void transport_send_task_abort(se_cmd_t *cmd)
{
#ifdef MY_ABC_HERE
	if (cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH)
		return;
#else
	if (!(cmd->se_cmd_flags & SCF_CMD_PASSTHROUGH))
		return;
#endif
	 
#ifdef MY_ABC_HERE
	if ((cmd->data_direction == DMA_TO_DEVICE) ||
	    (cmd->data_direction == DMA_BIDIRECTIONAL)) {
#else
	if ((cmd->data_direction == SE_DIRECTION_WRITE) ||
	    (cmd->data_direction == SE_DIRECTION_BIDI)) {
#endif
		if (CMD_TFO(cmd)->write_pending_status(cmd) != 0) {
			atomic_inc(&T_TASK(cmd)->t_transport_aborted);
			smp_mb__after_atomic_inc();
			cmd->scsi_status = SAM_STAT_TASK_ABORTED;
			transport_new_cmd_failure(cmd);
			return;
		}
	}
	cmd->scsi_status = SAM_STAT_TASK_ABORTED;
#if 0
	printk(KERN_INFO "Setting SAM_STAT_TASK_ABORTED status for CDB: 0x%02x,"
		" ITT: 0x%08x\n", T_TASK(cmd)->t_task_cdb[0],
		CMD_TFO(cmd)->get_task_tag(cmd));
#endif
	CMD_TFO(cmd)->queue_status(cmd);
}

int transport_generic_do_tmr(se_cmd_t *cmd)
{
	se_cmd_t *ref_cmd;
	se_device_t *dev = SE_DEV(cmd);
	se_tmr_req_t *tmr = cmd->se_tmr_req;
	int ret;

	switch (tmr->function) {
	case ABORT_TASK:
		ref_cmd = tmr->ref_cmd;
		tmr->response = TMR_FUNCTION_REJECTED;
		break;
	case ABORT_TASK_SET:
	case CLEAR_ACA:
	case CLEAR_TASK_SET:
		tmr->response = TMR_TASK_MGMT_FUNCTION_NOT_SUPPORTED;
		break;
	case LUN_RESET:
		ret = core_tmr_lun_reset(dev, tmr, NULL, NULL);
		tmr->response = (!ret) ? TMR_FUNCTION_COMPLETE :
					 TMR_FUNCTION_REJECTED;
		break;
#if 0
	case TARGET_WARM_RESET:
		transport_generic_host_reset(dev->se_hba);
		tmr->response = TMR_FUNCTION_REJECTED;
		break;
	case TARGET_COLD_RESET:
		transport_generic_host_reset(dev->se_hba);
		transport_generic_cold_reset(dev->se_hba);
		tmr->response = TMR_FUNCTION_REJECTED;
		break;
#endif
	default:
		printk(KERN_ERR "Uknown TMR function: 0x%02x.\n",
				tmr->function);
		tmr->response = TMR_FUNCTION_REJECTED;
		break;
	}

	cmd->t_state = TRANSPORT_ISTATE_PROCESSING;
	CMD_TFO(cmd)->queue_tm_rsp(cmd);

#ifdef MY_ABC_HERE
	transport_cmd_check_stop(cmd, 2, 0);
#endif
	return 0;
}

se_task_t *transport_get_task_from_state_list(se_device_t *dev)
{
	se_task_t *task;

	if (list_empty(&dev->state_task_list))
		return NULL;

	list_for_each_entry(task, &dev->state_task_list, t_state_list)
		break;

	list_del(&task->t_state_list);
	atomic_set(&task->task_state_active, 0);

	return task;
}

static void transport_processing_shutdown(se_device_t *dev)
{
	se_cmd_t *cmd;
	se_queue_req_t *qr;
	se_task_t *task;
	u8 state;
	unsigned long flags;
	 
	spin_lock_irqsave(&dev->execute_task_lock, flags);
	while ((task = transport_get_task_from_state_list(dev))) {
		if (!(TASK_CMD(task))) {
			printk(KERN_ERR "TASK_CMD(task) is NULL!\n");
			continue;
		}
		cmd = TASK_CMD(task);

		if (!T_TASK(cmd)) {
			printk(KERN_ERR "T_TASK(cmd) is NULL for task: %p cmd:"
				" %p ITT: 0x%08x\n", task, cmd,
				CMD_TFO(cmd)->get_task_tag(cmd));
			continue;
		}
		spin_unlock_irqrestore(&dev->execute_task_lock, flags);

		spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);

		DEBUG_DO("PT: cmd: %p task: %p ITT/CmdSN: 0x%08x/0x%08x,"
			" i_state/def_i_state: %d/%d, t_state/def_t_state:"
			" %d/%d cdb: 0x%02x\n", cmd, task,
			CMD_TFO(cmd)->get_task_tag(cmd), cmd->cmd_sn,
			CMD_TFO(cmd)->get_cmd_state(cmd), cmd->deferred_i_state,
			cmd->t_state, cmd->deferred_t_state,
			T_TASK(cmd)->t_task_cdb[0]);
		DEBUG_DO("PT: ITT[0x%08x] - t_task_cdbs: %d t_task_cdbs_left:"
			" %d t_task_cdbs_sent: %d -- t_transport_active: %d"
			" t_transport_stop: %d t_transport_sent: %d\n",
			CMD_TFO(cmd)->get_task_tag(cmd),
			T_TASK(cmd)->t_task_cdbs,
			atomic_read(&T_TASK(cmd)->t_task_cdbs_left),
			atomic_read(&T_TASK(cmd)->t_task_cdbs_sent),
			atomic_read(&T_TASK(cmd)->t_transport_active),
			atomic_read(&T_TASK(cmd)->t_transport_stop),
			atomic_read(&T_TASK(cmd)->t_transport_sent));

		if (atomic_read(&task->task_active)) {
			atomic_set(&task->task_stop, 1);
			spin_unlock_irqrestore(
				&T_TASK(cmd)->t_state_lock, flags);

			DEBUG_DO("Waiting for task: %p to shutdown for dev:"
				" %p\n", task, dev);
			wait_for_completion(&task->task_stop_comp);
			DEBUG_DO("Completed task: %p shutdown for dev: %p\n",
				task, dev);

			spin_lock_irqsave(&T_TASK(cmd)->t_state_lock, flags);
			atomic_dec(&T_TASK(cmd)->t_task_cdbs_left);

			atomic_set(&task->task_active, 0);
			atomic_set(&task->task_stop, 0);
		}
		__transport_stop_task_timer(task, &flags);

		if (!(atomic_dec_and_test(&T_TASK(cmd)->t_task_cdbs_ex_left))) {
			spin_unlock_irqrestore(
					&T_TASK(cmd)->t_state_lock, flags);

			DEBUG_DO("Skipping task: %p, dev: %p for"
				" t_task_cdbs_ex_left: %d\n", task, dev,
				atomic_read(&T_TASK(cmd)->t_task_cdbs_ex_left));

			spin_lock_irqsave(&dev->execute_task_lock, flags);
			continue;
		}

		if (atomic_read(&T_TASK(cmd)->t_transport_active)) {
			DEBUG_DO("got t_transport_active = 1 for task: %p, dev:"
					" %p\n", task, dev);

			if (atomic_read(&T_TASK(cmd)->t_fe_count)) {
				spin_unlock_irqrestore(
					&T_TASK(cmd)->t_state_lock, flags);
				transport_send_check_condition_and_sense(
					cmd, LOGICAL_UNIT_COMMUNICATION_FAILURE,
					0);
#ifdef MY_ABC_HERE
				transport_remove_cmd_from_queue(cmd,
						SE_DEV(cmd)->dev_queue_obj);
#else
				transport_remove_cmd_from_queue(cmd,
					CMD_ORIG_OBJ_API(cmd)->get_queue_obj(
						cmd->se_orig_obj_ptr));
#endif

				transport_lun_remove_cmd(cmd);
				if (!(transport_cmd_check_stop(cmd, 1, 0)))
					transport_passthrough_check_stop(cmd);
			} else {
				spin_unlock_irqrestore(
					&T_TASK(cmd)->t_state_lock, flags);

#ifdef MY_ABC_HERE
				transport_remove_cmd_from_queue(cmd,
						SE_DEV(cmd)->dev_queue_obj);
#else
				transport_remove_cmd_from_queue(cmd,
					CMD_ORIG_OBJ_API(cmd)->get_queue_obj(
						cmd->se_orig_obj_ptr));
#endif

				transport_lun_remove_cmd(cmd);

				if (!(transport_cmd_check_stop(cmd, 1, 0)))
					transport_passthrough_check_stop(cmd);
				else
					transport_generic_remove(cmd, 0, 0);
			}

			spin_lock_irqsave(&dev->execute_task_lock, flags);
			continue;
		}
		DEBUG_DO("Got t_transport_active = 0 for task: %p, dev: %p\n",
				task, dev);

		if (atomic_read(&T_TASK(cmd)->t_fe_count)) {
			spin_unlock_irqrestore(
				&T_TASK(cmd)->t_state_lock, flags);
			transport_send_check_condition_and_sense(cmd,
				LOGICAL_UNIT_COMMUNICATION_FAILURE, 0);
#ifdef MY_ABC_HERE
			transport_remove_cmd_from_queue(cmd,
					SE_DEV(cmd)->dev_queue_obj);
#else
			transport_remove_cmd_from_queue(cmd,
				CMD_ORIG_OBJ_API(cmd)->get_queue_obj(
					cmd->se_orig_obj_ptr));
#endif

			transport_lun_remove_cmd(cmd);
			if (!(transport_cmd_check_stop(cmd, 1, 0)))
				transport_passthrough_check_stop(cmd);
		} else {
			spin_unlock_irqrestore(
				&T_TASK(cmd)->t_state_lock, flags);

#ifdef MY_ABC_HERE
			transport_remove_cmd_from_queue(cmd,
					SE_DEV(cmd)->dev_queue_obj);
#else
			transport_remove_cmd_from_queue(cmd,
				CMD_ORIG_OBJ_API(cmd)->get_queue_obj(
					cmd->se_orig_obj_ptr));
#endif
			transport_lun_remove_cmd(cmd);

			if (!(transport_cmd_check_stop(cmd, 1, 0)))
				transport_passthrough_check_stop(cmd);
			else
				transport_generic_remove(cmd, 0, 0);
		}

		spin_lock_irqsave(&dev->execute_task_lock, flags);
	}
	spin_unlock_irqrestore(&dev->execute_task_lock, flags);
	 
	spin_lock_irqsave(&dev->dev_queue_obj->cmd_queue_lock, flags);
	while ((qr = __transport_get_qr_from_queue(dev->dev_queue_obj))) {
		spin_unlock_irqrestore(
				&dev->dev_queue_obj->cmd_queue_lock, flags);
		cmd = (se_cmd_t *)qr->cmd;
		state = qr->state;
		kfree(qr);

		DEBUG_DO("From Device Queue: cmd: %p t_state: %d\n",
				cmd, state);

		if (atomic_read(&T_TASK(cmd)->t_fe_count)) {
			transport_send_check_condition_and_sense(cmd,
				LOGICAL_UNIT_COMMUNICATION_FAILURE, 0);

			transport_lun_remove_cmd(cmd);
			if (!(transport_cmd_check_stop(cmd, 1, 0)))
				transport_passthrough_check_stop(cmd);
		} else {
			transport_lun_remove_cmd(cmd);

			if (!(transport_cmd_check_stop(cmd, 1, 0)))
				transport_passthrough_check_stop(cmd);
			else
				transport_generic_remove(cmd, 0, 0);
		}
		spin_lock_irqsave(&dev->dev_queue_obj->cmd_queue_lock, flags);
	}
	spin_unlock_irqrestore(&dev->dev_queue_obj->cmd_queue_lock, flags);
}

static int transport_processing_thread(void *param)
{
	int ret, t_state;
	se_cmd_t *cmd;
	se_device_t *dev = (se_device_t *) param;
	se_queue_req_t *qr;

	current->policy = SCHED_NORMAL;
	set_user_nice(current, -20);
	spin_lock_irq(&current->sighand->siglock);
	siginitsetinv(&current->blocked, SHUTDOWN_SIGS);
	recalc_sigpending();
	spin_unlock_irq(&current->sighand->siglock);

	complete(&dev->dev_queue_obj->thread_create_comp);

	while (!(kthread_should_stop())) {
		ret = wait_event_interruptible(dev->dev_queue_obj->thread_wq,
				atomic_read(&dev->dev_queue_obj->queue_cnt) ||
				kthread_should_stop());
		if (ret < 0)
			goto out;

		spin_lock(&dev->dev_status_lock);
		if (dev->dev_status & TRANSPORT_DEVICE_SHUTDOWN) {
			spin_unlock(&dev->dev_status_lock);
			transport_processing_shutdown(dev);
			continue;
		}
		spin_unlock(&dev->dev_status_lock);

get_cmd:
		__transport_execute_tasks(dev);

		qr = transport_get_qr_from_queue(dev->dev_queue_obj);
		if (!(qr))
			continue;

		cmd = (se_cmd_t *)qr->cmd;
		t_state = qr->state;
		kfree(qr);

		switch (t_state) {
		case TRANSPORT_NEW_CMD:
			ret = transport_generic_new_cmd(cmd);
			if (ret < 0) {
				cmd->transport_error_status = ret;
#ifdef MY_ABC_HERE
				transport_generic_request_failure(cmd, NULL,
					0, (cmd->data_direction !=
					 DMA_TO_DEVICE));
#else
				transport_generic_request_failure(cmd, NULL,
					0, (cmd->data_direction !=
					 SE_DIRECTION_WRITE));
#endif
			}
			break;
		case TRANSPORT_PROCESS_WRITE:
			transport_generic_process_write(cmd);
			break;
		case TRANSPORT_COMPLETE_OK:
			transport_stop_all_task_timers(cmd);
			transport_generic_complete_ok(cmd);
			break;
		case TRANSPORT_REMOVE:
			transport_generic_remove(cmd, 1, 0);
			break;
		case TRANSPORT_PROCESS_TMR:
			transport_generic_do_tmr(cmd);
			break;
		case TRANSPORT_COMPLETE_FAILURE:
			transport_generic_request_failure(cmd, NULL, 1, 1);
			break;
		case TRANSPORT_COMPLETE_TIMEOUT:
			transport_stop_all_task_timers(cmd);
			transport_generic_request_timeout(cmd);
			break;
		default:
			printk(KERN_ERR "Unknown t_state: %d deferred_t_state:"
				" %d for ITT: 0x%08x i_state: %d on SE LUN:"
				" %u\n", t_state, cmd->deferred_t_state,
				CMD_TFO(cmd)->get_task_tag(cmd),
				CMD_TFO(cmd)->get_cmd_state(cmd),
				SE_LUN(cmd)->unpacked_lun);
			BUG();
		}

		goto get_cmd;
	}

out:
	transport_release_all_cmds(dev);
	dev->process_thread = NULL;
	complete(&dev->dev_queue_obj->thread_done_comp);
	return 0;
}

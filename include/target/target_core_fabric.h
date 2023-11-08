#ifndef TARGET_CORE_FABRIC_H
#define TARGET_CORE_FABRIC_H

/* 20140513, adamhsu, redmine 8253 */
#ifdef CONFIG_MACH_QNAPTS
#include <linux/version.h>
#endif

struct target_core_fabric_ops {
	struct configfs_subsystem *tf_subsys;
	/*
	 * Optional to signal struct se_task->task_sg[] padding entries
	 * for scatterlist chaining using transport_do_task_sg_link(),
	 * disabled by default
	 */
	bool task_sg_chaining;
	char *(*get_fabric_name)(void);
	u8 (*get_fabric_proto_ident)(struct se_portal_group *);
	char *(*tpg_get_wwn)(struct se_portal_group *);
	u16 (*tpg_get_tag)(struct se_portal_group *);
	u32 (*tpg_get_default_depth)(struct se_portal_group *);
	u32 (*tpg_get_pr_transport_id)(struct se_portal_group *,
				struct se_node_acl *,
				struct t10_pr_registration *, int *,
				unsigned char *);
	u32 (*tpg_get_pr_transport_id_len)(struct se_portal_group *,
				struct se_node_acl *,
				struct t10_pr_registration *, int *);
	char *(*tpg_parse_pr_out_transport_id)(struct se_portal_group *,
				const char *, u32 *, char **);
	int (*tpg_check_demo_mode)(struct se_portal_group *);
	int (*tpg_check_demo_mode_cache)(struct se_portal_group *);
	int (*tpg_check_demo_mode_write_protect)(struct se_portal_group *);
	int (*tpg_check_prod_mode_write_protect)(struct se_portal_group *);
	/*
	 * Optionally used by fabrics to allow demo-mode login, but not
	 * expose any TPG LUNs, and return 'not connected' in standard
	 * inquiry response
	 */
	int (*tpg_check_demo_mode_login_only)(struct se_portal_group *);
	struct se_node_acl *(*tpg_alloc_fabric_acl)(
					struct se_portal_group *);
	void (*tpg_release_fabric_acl)(struct se_portal_group *,
					struct se_node_acl *);
	u32 (*tpg_get_inst_index)(struct se_portal_group *);
	/*
	 * Optional function pointer for TCM to perform command map
	 * from TCM processing thread context, for those struct se_cmd
	 * initially allocated in interrupt context.
	 */
	int (*new_cmd_map)(struct se_cmd *);
	/*
	 * Optional to release struct se_cmd and fabric dependent allocated
	 * I/O descriptor in transport_cmd_check_stop().
	 *
	 * Returning 1 will signal a descriptor has been released.
	 * Returning 0 will signal a descriptor has not been released.
	 */
	int (*check_stop_free)(struct se_cmd *);
	void (*release_cmd)(struct se_cmd *);
	/*
	 * Called with spin_lock_bh(struct se_portal_group->session_lock held.
	 */
	int (*shutdown_session)(struct se_session *);
	void (*close_session)(struct se_session *);
	u32 (*sess_get_index)(struct se_session *);
	/*
	 * Used only for SCSI fabrics that contain multi-value TransportIDs
	 * (like iSCSI).  All other SCSI fabrics should set this to NULL.
	 */
	u32 (*sess_get_initiator_sid)(struct se_session *,
				      unsigned char *, u32);
	int (*write_pending)(struct se_cmd *);
	int (*write_pending_status)(struct se_cmd *);
	void (*set_default_node_attributes)(struct se_node_acl *);
#ifdef CONFIG_MACH_QNAPTS   // 2009/09/23 Nike Chen add for default initiator 
	void (*copy_node_attributes) (struct se_node_acl *, struct se_node_acl *);

	/* 2014/04/06, adamhsu, redmine 7904 */
	void (*inc_maxcmdsn)(struct se_cmd *);

	/* 2014/04/06, adamhsu, redmine 7923 */
	void (*del_cmd_from_conn_list)(struct se_cmd *);

	/* 2014/08/16, adamhsu, redmine 9055,9076,9278 */
	int (*set_clear_delay_remove)(struct se_cmd *, int, int);
	int (*tmf_queue_status)(struct se_cmd *);


#endif        
	u32 (*get_task_tag)(struct se_cmd *);
	int (*get_cmd_state)(struct se_cmd *);
	int (*queue_data_in)(struct se_cmd *);
	int (*queue_status)(struct se_cmd *);
	int (*queue_tm_rsp)(struct se_cmd *);
	u16 (*set_fabric_sense_len)(struct se_cmd *, u32);
	u16 (*get_fabric_sense_len)(void);
	/*
	 * fabric module calls for target_core_fabric_configfs.c
	 */
	struct se_wwn *(*fabric_make_wwn)(struct target_fabric_configfs *,
				struct config_group *, const char *);
	void (*fabric_drop_wwn)(struct se_wwn *);
	struct se_portal_group *(*fabric_make_tpg)(struct se_wwn *,
				struct config_group *, const char *);
	void (*fabric_drop_tpg)(struct se_portal_group *);
	int (*fabric_post_link)(struct se_portal_group *,
				struct se_lun *);
	void (*fabric_pre_unlink)(struct se_portal_group *,
				struct se_lun *);
	struct se_tpg_np *(*fabric_make_np)(struct se_portal_group *,
				struct config_group *, const char *);
	void (*fabric_drop_np)(struct se_tpg_np *);
	struct se_node_acl *(*fabric_make_nodeacl)(struct se_portal_group *,
				struct config_group *, const char *);
	void (*fabric_drop_nodeacl)(struct se_node_acl *);
};

struct se_session *transport_init_session(void);



#ifdef CONFIG_MACH_QNAPTS

/* 2014/08/16, adamhsu, redmine 9055,9076,9278 */
int __do_check_cmd_aborted_func1(struct se_cmd *se_cmd);
int __do_check_cmd_aborted_func2(struct se_cmd *se_cmd);
int __do_check_set_send_status_func1(struct se_cmd *se_cmd);
int __do_check_set_send_status_func2(struct se_cmd *se_cmd);


#if defined(SUPPORT_TP)
/* 2014/06/14, adamhsu, redmine 8530 (start) */
int __get_file_lba_map_status(struct se_device *se_dev,  
	struct block_device *bd, struct inode *inode,
	sector_t lba, u32 *desc_count, u8 *desc_buf);

/* 2014/06/14, adamhsu, redmine 8530 (start) */
#endif

/* 20140513, adamhsu, redmine 8253 */
#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
int transport_alloc_session_tags(struct se_session *, unsigned int,
		unsigned int);
struct se_session *transport_init_session_tags(unsigned int, unsigned int);
#endif
#endif

void	__transport_register_session(struct se_portal_group *,
		struct se_node_acl *, struct se_session *, void *);
void	transport_register_session(struct se_portal_group *,
		struct se_node_acl *, struct se_session *, void *);
void	target_get_session(struct se_session *);
int	target_put_session(struct se_session *);
void	transport_free_session(struct se_session *);
void	target_put_nacl(struct se_node_acl *);
void	transport_deregister_session_configfs(struct se_session *);
void	transport_deregister_session(struct se_session *);


void	transport_init_se_cmd(struct se_cmd *, struct target_core_fabric_ops *,
		struct se_session *, u32, int, int, unsigned char *);
int	transport_lookup_cmd_lun(struct se_cmd *, u32);
int	transport_generic_allocate_tasks(struct se_cmd *, unsigned char *);

/* 20140513, adamhsu, redmine 8253 */
#ifdef CONFIG_MACH_QNAPTS
#if (LINUX_VERSION_CODE == KERNEL_VERSION(3,12,6))
int	target_submit_cmd_map_sgls(struct se_cmd *, struct se_session *,
		unsigned char *, unsigned char *, u32, u32, int, int, int,
		struct scatterlist *, u32, struct scatterlist *, u32);

void	target_execute_cmd(struct se_cmd *cmd);
#endif
#endif

void	target_submit_cmd(struct se_cmd *, struct se_session *, unsigned char *,
		unsigned char *, u32, u32, int, int, int);
int	target_submit_tmr(struct se_cmd *se_cmd, struct se_session *se_sess,
		unsigned char *sense, u32 unpacked_lun,
		void *fabric_tmr_ptr, unsigned char tm_type,
		gfp_t, unsigned int, int);
int	transport_handle_cdb_direct(struct se_cmd *);
int	transport_generic_handle_cdb_map(struct se_cmd *);
int	transport_generic_handle_data(struct se_cmd *);
int	transport_generic_map_mem_to_cmd(struct se_cmd *cmd,
		struct scatterlist *, u32, struct scatterlist *, u32);
void	transport_do_task_sg_chain(struct se_cmd *);
int	transport_generic_new_cmd(struct se_cmd *);

void	transport_generic_process_write(struct se_cmd *);

void	transport_generic_free_cmd(struct se_cmd *, int);

bool	transport_wait_for_tasks(struct se_cmd *);
int	transport_check_aborted_status(struct se_cmd *, int);
int	transport_send_check_condition_and_sense(struct se_cmd *, u8, int);

void	target_get_sess_cmd(struct se_session *, struct se_cmd *, bool);
int	target_put_sess_cmd(struct se_session *, struct se_cmd *);
void	target_splice_sess_cmd_list(struct se_session *);
void	target_wait_for_sess_cmds(struct se_session *, int);

int	core_alua_check_nonop_delay(struct se_cmd *);

int	core_tmr_alloc_req(struct se_cmd *, void *, u8, gfp_t);
void	core_tmr_release_req(struct se_tmr_req *);
int	transport_generic_handle_tmr(struct se_cmd *);
void	transport_generic_request_failure(struct se_cmd *);
int	transport_lookup_tmr_lun(struct se_cmd *, u32);

struct se_node_acl *core_tpg_check_initiator_node_acl(struct se_portal_group *,
		unsigned char *);
void	core_tpg_clear_object_luns(struct se_portal_group *);
struct se_node_acl *core_tpg_add_initiator_node_acl(struct se_portal_group *,
		struct se_node_acl *, const char *, u32);
int	core_tpg_del_initiator_node_acl(struct se_portal_group *,
		struct se_node_acl *, int);
int	core_tpg_set_initiator_node_queue_depth(struct se_portal_group *,
		unsigned char *, u32, int);
int	core_tpg_register(struct target_core_fabric_ops *, struct se_wwn *,
		struct se_portal_group *, void *, int);
int	core_tpg_deregister(struct se_portal_group *);

/* SAS helpers */
u8	sas_get_fabric_proto_ident(struct se_portal_group *);
u32	sas_get_pr_transport_id(struct se_portal_group *, struct se_node_acl *,
		struct t10_pr_registration *, int *, unsigned char *);
u32	sas_get_pr_transport_id_len(struct se_portal_group *, struct se_node_acl *,
		struct t10_pr_registration *, int *);
char	*sas_parse_pr_out_transport_id(struct se_portal_group *, const char *,
		u32 *, char **);

/* FC helpers */
u8	fc_get_fabric_proto_ident(struct se_portal_group *);
u32	fc_get_pr_transport_id(struct se_portal_group *, struct se_node_acl *,
		struct t10_pr_registration *, int *, unsigned char *);
u32	fc_get_pr_transport_id_len(struct se_portal_group *, struct se_node_acl *,
		struct t10_pr_registration *, int *);
char	*fc_parse_pr_out_transport_id(struct se_portal_group *, const char *,
		u32 *, char **);

/* iSCSI helpers */
u8	iscsi_get_fabric_proto_ident(struct se_portal_group *);
u32	iscsi_get_pr_transport_id(struct se_portal_group *, struct se_node_acl *,
		struct t10_pr_registration *, int *, unsigned char *);
u32	iscsi_get_pr_transport_id_len(struct se_portal_group *, struct se_node_acl *,
		struct t10_pr_registration *, int *);
char	*iscsi_parse_pr_out_transport_id(struct se_portal_group *, const char *,
		u32 *, char **);

#endif /* TARGET_CORE_FABRICH */

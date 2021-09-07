#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef TARGET_CORE_TPG_H
#define TARGET_CORE_TPG_H

#ifdef MY_ABC_HERE
#define SYNO_LIO_DEFAULT_ACL_INITIATOR "iqn.2000-01.com.synology:default.acl"
#endif

extern se_global_t *se_global;
 
extern se_node_acl_t *__core_tpg_get_initiator_node_acl(se_portal_group_t *tpg,
						const char *);
extern se_node_acl_t *core_tpg_get_initiator_node_acl(se_portal_group_t *tpg,
						unsigned char *);
extern void core_tpg_add_node_to_devs(struct se_node_acl_s *,
						struct se_portal_group_s *);
extern struct se_node_acl_s *core_tpg_check_initiator_node_acl(
						struct se_portal_group_s *,
						unsigned char *);
extern void core_tpg_wait_for_nacl_pr_ref(struct se_node_acl_s *);
extern void core_tpg_free_node_acls(struct se_portal_group_s *);
extern void core_tpg_clear_object_luns(struct se_portal_group_s *);
extern se_node_acl_t *core_tpg_add_initiator_node_acl(se_portal_group_t *,
						const char *, u32);
extern int core_tpg_del_initiator_node_acl(se_portal_group_t *,
						se_node_acl_t *, int);
extern int core_tpg_set_initiator_node_queue_depth(se_portal_group_t *,
						unsigned char *, u32, int);
extern se_portal_group_t *core_tpg_register(struct target_core_fabric_ops *,
					void *, int);
extern int core_tpg_deregister(struct se_portal_group_s *);
extern se_lun_t *core_tpg_pre_addlun(se_portal_group_t *, u32);
#ifdef MY_ABC_HERE
extern int core_tpg_post_addlun(se_portal_group_t *, se_lun_t *, int, u32,
				void *);
#else
extern int core_tpg_post_addlun(se_portal_group_t *, se_lun_t *, int, u32,
				void *, struct se_obj_lun_type_s *);
#endif
#ifdef MY_ABC_HERE
extern void core_tpg_shutdown_lun(struct se_portal_group_s *,
				struct se_lun_s *);
#endif
extern se_lun_t *core_tpg_pre_dellun(se_portal_group_t *, u32, int, int *);
extern int core_tpg_post_dellun(se_portal_group_t *, se_lun_t *);

#endif  

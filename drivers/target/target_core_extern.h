#ifndef TARGET_CORE_EXTERN_H
#define TARGET_CORE_EXTERN_H
void iblock_update_allocated(struct se_device *dev);
/* 20140422, adamhsu, JS chen, redmine 8042 */
void __update_allocated_attr(struct se_device *dev);

struct node_info {
	unsigned char *i_port;
	unsigned char *i_sid;
	unsigned char *t_port;
	u64 sa_res_key;
	u32 mapped_lun;
	u32 target_lun;
	bool res_holder;
	bool all_tg_pt;
	u16 tpgt;
	u16 port_rpti;
	u16 type;
	u16 scope;
};


int __qnap_scsi3_parse_aptpl_data(
	struct se_device *se_dev,
	char *data,
	struct node_info *s,
	struct node_info *d
	);

int __qnap_scsi3_check_aptpl_metadata_file_exists(
	struct se_device *dev,
	struct file **fp
	);

int qnap_transport_scsi3_check_aptpl_registration(
	struct se_device *dev,
	struct se_portal_group *tpg,
	struct se_lun *lun,
	struct se_session *se_sess,
	struct se_node_acl *nacl,
	u32 mapped_lun
	);

int qnap_transport_check_aptpl_registration(
	struct se_session *se_sess,
	struct se_node_acl *nacl,
	struct se_portal_group *tpg
	);
#endif

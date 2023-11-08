#ifndef ISCSI_TARGET_NODEATTRIB_H
#define ISCSI_TARGET_NODEATTRIB_H

extern void iscsit_set_default_node_attribues(struct iscsi_node_acl *);
#ifdef CONFIG_MACH_QNAPTS // 2009/09/23 Nike Chen add for default initiator
extern void iscsi_copy_node_attribues (struct iscsi_node_acl *, struct iscsi_node_acl *);
#endif
extern int iscsit_na_dataout_timeout(struct iscsi_node_acl *, u32);
extern int iscsit_na_dataout_timeout_retries(struct iscsi_node_acl *, u32);
extern int iscsit_na_nopin_timeout(struct iscsi_node_acl *, u32);
extern int iscsit_na_nopin_response_timeout(struct iscsi_node_acl *, u32);
extern int iscsit_na_random_datain_pdu_offsets(struct iscsi_node_acl *, u32);
extern int iscsit_na_random_datain_seq_offsets(struct iscsi_node_acl *, u32);
extern int iscsit_na_random_r2t_offsets(struct iscsi_node_acl *, u32);
extern int iscsit_na_default_erl(struct iscsi_node_acl *, u32);

#endif /* ISCSI_TARGET_NODEATTRIB_H */

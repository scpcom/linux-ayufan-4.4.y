#ifndef ISCSI_TARGET_LOGIN_H
#define ISCSI_TARGET_LOGIN_H

extern int iscsi_login_setup_crypto(struct iscsi_conn *);
extern int iscsi_check_for_session_reinstatement(struct iscsi_conn *);
extern int iscsi_login_post_auth_non_zero_tsih(struct iscsi_conn *, u16, u32);
extern int iscsi_target_setup_login_socket(struct iscsi_np *,
				struct __kernel_sockaddr_storage *);
extern int iscsi_target_login_thread(void *);
extern int iscsi_login_disable_FIM_keys(struct iscsi_param_list *, struct iscsi_conn *);
#ifdef CONFIG_MACH_QNAPTS	// 20120720 Benjamin added for supporting connection log
extern int iscsi_post_log(int, int, struct iscsi_session *, char *);
#endif  /* #ifdef CONFIG_MACH_QNAPTS */
#endif   /*** ISCSI_TARGET_LOGIN_H ***/

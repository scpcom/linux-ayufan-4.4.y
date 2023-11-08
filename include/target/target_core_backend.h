#ifndef TARGET_CORE_BACKEND_H
#define TARGET_CORE_BACKEND_H

#define TRANSPORT_PLUGIN_PHBA_PDEV		1
#define TRANSPORT_PLUGIN_VHBA_PDEV		2
#define TRANSPORT_PLUGIN_VHBA_VDEV		3

struct se_subsystem_api {
	struct list_head sub_api_list;

	char name[16];
	struct module *owner;

	u8 transport_type;

	unsigned int fua_write_emulated : 1;
	unsigned int write_cache_emulated : 1;

	int (*attach_hba)(struct se_hba *, u32);
	void (*detach_hba)(struct se_hba *);
	int (*pmode_enable_hba)(struct se_hba *, unsigned long);
	void *(*allocate_virtdevice)(struct se_hba *, const char *);
	struct se_device *(*create_virtdevice)(struct se_hba *,
				struct se_subsystem_dev *, void *);
	void (*free_device)(void *);
	int (*transport_complete)(struct se_task *task);
	struct se_task *(*alloc_task)(unsigned char *cdb);
	int (*do_task)(struct se_task *);

#if defined(CONFIG_MACH_QNAPTS)
	/* 20140626, adamhsu, redmine 8745,8777,8778 */
	int (*do_discard)(struct se_cmd *, sector_t, u32);
#else
	int (*do_discard)(struct se_device *, sector_t, u32);
#endif

	void (*do_sync_cache)(struct se_task *);
	void (*free_task)(struct se_task *);
	ssize_t (*check_configfs_dev_params)(struct se_hba *,
			struct se_subsystem_dev *);
	ssize_t (*set_configfs_dev_params)(struct se_hba *,
			struct se_subsystem_dev *, const char *, ssize_t);
	ssize_t (*show_configfs_dev_params)(struct se_hba *,
			struct se_subsystem_dev *, char *);
	u32 (*get_device_rev)(struct se_device *);
	u32 (*get_device_type)(struct se_device *);
	sector_t (*get_blocks)(struct se_device *);
	unsigned char *(*get_sense_buffer)(struct se_task *);
#ifdef CONFIG_MACH_QNAPTS // 2010/07/20 Nike Chen, support online lun expansion	
	int (*change_dev_size)(struct se_device *);  /* change_dev_size(): support online lun expansion */
#endif    

#if defined(CONFIG_MACH_QNAPTS)
#if defined(SUPPORT_VAAI)

	/* api for write same function */
	void (*do_prepare_ws_buffer)(struct se_cmd *, u32, u32, void *, void *);
	int  (*do_check_ws_zero_buffer)(struct se_cmd *);
	int  (*do_check_before_ws)(struct se_cmd *);
	int  (*do_ws_wo_unmap)(struct se_cmd *);
	int  (*do_ws_w_anchor)(struct se_cmd *);
	int  (*do_ws_w_unmap)(struct se_cmd *);

	/* api for atomic test and set (ATS) function */
	int  (*do_check_before_ats)(struct se_cmd *);
	int  (*do_ats)(struct se_cmd *);
#endif

#if defined(SUPPORT_TP)
/* 2014/06/14, adamhsu, redmine 8530 (start) */
	int  (*do_get_lba_map_status)(struct se_cmd *, sector_t, 
			u32 , u8 *, int *);
/* 2014/06/14, adamhsu, redmine 8530 (end) */
#endif



#if defined(SUPPORT_TPC_CMD)
    /* populate token */  
    int   (*do_pt)(struct se_cmd *, void *);
    int   (*do_chk_before_pt)(struct se_cmd *);

    /* write using rod token */
    int   (*do_wrt)(struct se_cmd *, void *token_src_obj, void *new_obj);

    /* write using zero rod token */
    int   (*do_wzrt)(struct se_cmd *, void *new_obj);
    int   (*do_chk_before_wrt)(struct se_cmd *);

    /* receive rod token */
    int   (*do_receive_rt)(struct se_cmd *);
#endif
#endif /* defined(CONFIG_MACH_QNAPTS) */
};

int	transport_subsystem_register(struct se_subsystem_api *);
void	transport_subsystem_release(struct se_subsystem_api *);

struct se_device *transport_add_device_to_core_hba(struct se_hba *,
		struct se_subsystem_api *, struct se_subsystem_dev *, u32,
		void *, struct se_dev_limits *, const char *, const char *);

void	transport_complete_sync_cache(struct se_cmd *, int);
void	transport_complete_task(struct se_task *, int);

void	target_get_task_cdb(struct se_task *, unsigned char *);

void	transport_set_vpd_proto_id(struct t10_vpd *, unsigned char *);
int	transport_set_vpd_assoc(struct t10_vpd *, unsigned char *);
int	transport_set_vpd_ident_type(struct t10_vpd *, unsigned char *);
int	transport_set_vpd_ident(struct t10_vpd *, unsigned char *);

/* core helpers also used by command snooping in pscsi */
void	*transport_kmap_data_sg(struct se_cmd *);
void	transport_kunmap_data_sg(struct se_cmd *);

void	array_free(void *array, int n);

/**/
#if defined(CONFIG_MACH_QNAPTS) 

#if defined(SUPPORT_VAAI)
void do_prepare_ws_buffer(
    struct se_cmd *cmd,
    u32 blk_size,
    u32 total_bytes,
    void *src,
    void *dest
    );

int do_check_before_ws(struct se_cmd *cmd);
int do_check_ws_zero_buffer(struct se_cmd *cmd);
int iblock_do_ws_wo_unmap(struct se_cmd *cmd);
int iblock_do_ws_w_anchor(struct se_cmd *cmd);
int iblock_do_ws_w_unmap(struct se_cmd *cmd);

int fd_do_ws_wo_unmap(struct se_cmd *cmd);
int fd_do_ws_w_anchor(struct se_cmd *cmd);
int fd_do_ws_w_unmap(struct se_cmd *cmd);

/**/
int do_check_before_ats(struct se_cmd *cmd);
int iblock_do_ats(struct se_cmd *cmd);
int fd_do_ats(struct se_cmd *cmd);
#endif


#if defined(SUPPORT_TP)
/* 2014/06/14, adamhsu, redmine 8530 (start) */
int __iblock_get_lba_map_status(struct se_cmd *se_cmd, sector_t lba,
		u32 desc_count, u8 *param, int *err);

int __fd_get_lba_map_status(struct se_cmd *se_cmd, sector_t lba,
		u32 desc_count, u8 *param, int *err);
/* 2014/06/14, adamhsu, redmine 8530 (end) */
#endif



#if defined(SUPPORT_TPC_CMD)
/**/
int iblock_do_populate_token(struct se_cmd *cmd, void *obj);
int iblock_before_populate_token(struct se_cmd *cmd);
int iblock_do_write_by_token(struct se_cmd *cmd, void *token_src_obj, void *new_obj);
int iblock_do_write_by_zero_rod_token(struct se_cmd *cmd, void *new_obj);
int iblock_before_write_by_token(struct se_cmd *cmd);
int iblock_receive_rod_token(struct se_cmd *cmd);

/**/
int fd_do_populate_token(struct se_cmd *cmd, void *obj);
int fd_before_populate_token(struct se_cmd *cmd);
int fd_do_write_by_token(struct se_cmd *cmd, void *token_src_obj, void *new_obj);
int fd_do_write_by_zero_rod_token(struct se_cmd *cmd, void *new_obj);
int fd_before_write_by_token(struct se_cmd *cmd);
int fd_receive_rod_token(struct se_cmd *cmd);
#endif
#endif /* CONFIG_MACH_QNAPTS */

#endif /* TARGET_CORE_BACKEND_H */

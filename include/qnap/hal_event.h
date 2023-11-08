/*
    Remember sync the same file in Kernel/include/qnap and NasLib/hal
*/
#ifndef __HAL_NETLINK_HDR
#define __HAL_NETLINK_HDR

typedef enum
{
    HAL_EVENT_GENERAL_DISK = 0,
    HAL_EVENT_ENCLOSURE,
    HAL_EVENT_RAID,
    HAL_EVENT_THIN,
    HAL_EVENT_NET,
    HAL_EVENT_USB_PRINTER,
    HAL_EVENT_VOLUME,
// #ifdef STORAGE_V2
    HAL_EVENT_LVM,
    HAL_EVENT_ISCSI,
// #endif
    HAL_EVENT_USB_MTP,
    HAL_EVENT_GPIO,
    HAL_EVENT_MAX_EVT_NUM,
} EVT_FUNC_TYPE;

#ifndef USB_DRV_DEFINED
typedef enum
{
    USB_DRV_UNKNOWN_HCD = 0,
    USB_DRV_UHCI_HCD,
    USB_DRV_EHCI_HCD,
    USB_DRV_XHCI_HCD,
    USB_DRV_ETXHCI_HCD,
} USB_DRV_TYPE;
#define USB_DRV_DEFINED
#endif
typedef enum
{
// For GENERAL_DISK, ENCLOSURE,NET,PRT
    ATTACH = 1,
    DETACH,
    SCAN,
    SET_IDENT,
// For ENCLOSURE
    SET_FAN_FORCE_SPEED = 10,
    CLEAR_FAN_FORCE_SPEED,
    SET_FAN_MODE_SMART_INTELLIGENT,
    SET_FAN_MODE_SMART_CUSTOM,
    SET_FAN_MODE_FIXED,
    SET_MAX_WAKEUP_PD_NUM,  //for netlink
    SET_SMART_TEMP_UNIT,
    SET_PD_STANDBY_TIMEOUT,
    NORMAL_POWER_OFF,
    SET_PWR_REDUNDANT,
    SET_POWER_BUTTON,       //for hw test
    RELOAD_USB_DRV,
    SET_PIC_WOL,
    CPU_THERMAL_THROTTLING_EVENT,
// For GENERAL_DISK
    SELF_TEST = 50,
    SET_PD_STANDBY,         //for netlink
    CLEAR_PD_STANDBY,       //for netlink
    SET_PD_ERROR,           //for netlink
    CLEAR_PD_ERROR,         //for netlink
    SET_PD_TEMP_WARNING,
    SELF_TEST_SCHEDULE,
    FAIL_DRIVE,
    REMOVABLE_DRIVE_DETACH,
    SET_NCQ_BY_USER,
    SET_NCQ_BY_KERNEL,

// For RAID    
    REPAIR_RAID_READ_ERROR = 100,   //1,5,6,10, for netlink,reconstruct
    SET_RAID_PD_ERROR,              //1,5,6,10, for netlink
    SET_RAID_RO,                    //1,5,6,10, for netlink,degrade and other drive error
    RESYNCING_START,                //for netlink
    RESYNCING_SKIP,                 //for netlink
    RESYNCING_COMPLETE,             //for netlink
    REBUILDING_START,               //for netlink
    REBUILDING_SKIP,                //for netlink
    REBUILDING_COMPLETE,            //for netlink
    //[SDMD START] 20130124 by csw: for raid hot-replace event
    HOTREPLACING_START,             //for netlink
    HOTREPLACING_SKIP,              //for netlink
    HOTREPLACING_COMPLETE,          //for netlink
    RAID_PD_HOTREPLACED,            //for netlink
    BAD_BLOCK_ERROR_DETECT,         //for md badblock
    BAD_BLOCK_ERROR_STRIPE,         //for md badblock
    BAD_BLOCK_ERROR_REBUILD,	    //for md badblock
    THIN_ERR_VERSION_DETECT,	    //for version-checking of dm-thin
    //[SDMD END]
// For NET
    BLOCK_IP_FILTER = 150,          //for NVR
    NET_EVT_MASK,
    NET_IP_DEL,
    NET_IP_NEW,
    NET_SCAN,
// For THIN
    THIN_SB_BACKUP_FAIL = 160,
// For VOLUME
    CHECK_FREE_SIZE = 180,
// #ifdef STORAGE_V2
// For LVM
    CHECK_LVM_STATUS = 190,
    HIT_LUN_THRESHOLD,	// for iscsi    
    PRINT_SHOW_INFO,
    CLEAN_SHOW_INFO,
// #endif
// For Misc    
    SHOW = 200,
    RETRIEVE_LOST_EVENT,                 //for netlink
    DEBUG_LEVEL,
    PM_EVENT,
    PM_PREPARE_SUSPEND_COMP,
    RELOAD_CONF,
    CHECK_GPIO_STATUS,
} EVT_FUNC_ACTION;

typedef struct
{
    EVT_FUNC_ACTION action;
    union __EVT_FUNC_ARG
    {
#if !defined(__KERNEL__) && !defined(KERRD)   
        struct
        {
            int                 action;
        }__attribute__ ((__packed__)) pwr_redundant_check;
        struct
        {
            int                 action;
            int                 value;
        }__attribute__ ((__packed__)) vol_free_size;
// #ifdef STORAGE_V2
        struct
        {
            int                 pool_id;
            int                 vol_id;
            int                 value;
        }__attribute__ ((__packed__)) check_lvm_status;
// #endif
        struct
        {
            int action;
            int time;//min
        }__attribute__ ((__packed__)) pd_standby_timeout;
        struct
        {
            int unit;
        }__attribute__ ((__packed__)) smart_unit;
        struct
        {
            PD_DEV_ID           dev_id;
            int                 action;
            int                 value;
        }__attribute__ ((__packed__)) temp_warning;
       struct
        {
            unsigned int flags;
        }__attribute__ ((__packed__)) debug_level;
        struct
        {
            char dev_sys_id[MAX_SYS_ID_LEN];
            char enc_sys_id[MAX_SYS_ID_LEN];
            time_t time_stamp;
        } __attribute__ ((__packed__)) add_remove;
        struct
        {
            PD_DEV_ID           dev_id;
            PD_SELFTEST_MODE    mode;
            unsigned long long  value;
        } __attribute__ ((__packed__)) self_test;
        struct
        {
            FAN_SPEED speed_level;
            int custom_unit;
            int custom_stop_temp;
            int custom_low_temp;
            int custom_high_temp;        
        } __attribute__ ((__packed__)) fan_control;
        struct
        {
            PD_DEV_ID           dev_id;
            int                 enable;
            int                 timeout;//min
        } __attribute__ ((__packed__)) ident_led;
#endif        
        struct __netlink_enc_cb
        {
            int enc_id;
            int max_wakeup_pds;
        } __attribute__ ((__packed__)) netlink_enc;
        struct __netlink_pd_cb
        {
            int enc_id;             //only use for set_pd_standby
            int scsi_bus[4];
            unsigned char error_sense_key[3]; //sense_key,ASC,ASCQ
            unsigned char error_scsi_cmd[16];
        } __attribute__ ((__packed__)) netlink_pd;
        struct __netlink_raid_cb
        {
            int raid_id;
            char pd_scsi_name[32];
            unsigned long long pd_repair_sector;
            //[SDMD] 20130221 by csw: add structure for log
            char pd_scsi_spare_name[32];
        } __attribute__ ((__packed__)) netlink_raid;
        struct __net_link_change_cb
        {
            char ifi_name[16];
            int ifi_role; //0->standlone, 1->slave in bonding mode, 2->master in bonding mode
        } __attribute__ ((__packed__)) net_link_change;
        struct __net_ip_change_cb
        {
            int monitor;
            char dev_name[16]; //eth0,
            char ip_addr[64]; //string
        } __attribute__ ((__packed__)) net_ip_change;
        struct __netlink_ip_block_cb
        {
            unsigned int addr;
            unsigned short protocol;
            unsigned short port;
        } __attribute__ ((__packed__)) netlink_ip_block;
        struct __netlink_usb_drv
        {
            USB_DRV_TYPE type;
        } __attribute__ ((__packed__)) netlink_usb_drv;
        struct __netlink_pic_wol
        {
            int enable;
        } __attribute__ ((__packed__)) netlink_pic_wol;
        struct __netlink_pm_cb
        {
            int pm_event; //0->suspend_prepare, 1->post_suspend
        } __attribute__ ((__packed__)) netlink_pm;
	struct __iscsi_lun
	{
		int lun_index;
		int tp_threshold;
		int tp_avail;
	} __attribute__ ((__packed__)) iscsi_lun;
        struct __badblock
        {
            char pd_scsi_name[32];
            unsigned long long first_bad;
            unsigned long long bad_sectors;
            unsigned int count;
            int testMode;
        }__attribute__((__packed__)) badblock;
        struct __thin_err_version
        {
            char thin_pool_name[32];
        }__attribute__((__packed__)) thin_err_version;
        struct __cpu_thermal_throttling_event
        {
            int throttle_enable;
            int cpu_id;
        }__attribute__((__packed__)) cpu_thermal_throttling_event;
        struct __netlink_ncq_cb
        {
            int scsi_bus[4];
            int on_off;
        } __attribute__ ((__packed__)) netlink_ncq;
        struct __pool_message
        {
            char pool_name[32];
        } __attribute__ ((__packed__)) pool_message;        
    } param;
}__attribute__ ((__packed__))
EVT_FUNC_ARG;

typedef struct
{
    EVT_FUNC_TYPE type;
    EVT_FUNC_ARG arg;
} __attribute__ ((__packed__))
NETLINK_EVT;

#endif

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#if defined(MY_DEF_HERE)
extern volatile void __iomem *rpc_common_base;
#define IPC_SHM_VIRT			(rpc_common_base+0x000000C4)

struct RTK119X_ipc_shm {
/*0C4*/	volatile uint32_t		sys_assign_serial;
/*0C8*/	volatile uint32_t		pov_boot_vd_std_ptr;
/*0CC*/	volatile uint32_t		pov_boot_av_info;
/*0D0*/	volatile uint32_t		audio_rpc_flag;
/*0D4*/	volatile uint32_t		suspend_mask;
/*0D8*/	volatile uint32_t		suspend_flag;
/*0DC*/ volatile uint32_t		vo_vsync_flag;
/*0E0*/	volatile uint32_t		audio_fw_entry_pt;
/*0E4*/	volatile uint32_t		power_saving_ptr;
/*0E8*/	volatile unsigned char	printk_buffer[24];
/*100*/	volatile uint32_t		ir_extended_tbl_pt;
/*104*/	volatile uint32_t		vo_int_sync;
/*108*/	volatile uint32_t		bt_wakeup_flag;//Bit31~24:magic key(0xEA), Bit23:high-active(1) low-active(0), Bit22~0:mask
/*10C*/	volatile uint32_t		ir_scancode_mask;
/*110*/	volatile uint32_t		ir_wakeup_scancode;
/*114*/	volatile uint32_t	    suspend_wakeup_flag;                /* [31-24] magic key(0xEA) [5] cec [4] timer [3] alarm [2] gpio [1] ir [0] lan , (0) disable (1) enable */
/*118*/	volatile uint32_t	    acpu_resume_state;                  /* [31-24] magic key(0xEA) [23-16] enum { NONE = 0, UNKNOW, IR, GPIO, LAN, ALARM, TIMER, CEC}  [15-0] ex GPIO Number */
/*11C*/	volatile uint32_t        gpio_wakeup_enable;                 /* [31-24] magic key(0xEA) [23-0] mapping to the number of iso gpio 0~23 */
/*120*/	volatile uint32_t        gpio_wakeup_activity;               /* [31-24] magic key(0xEA) [23-0] mapping to the number of iso gpio 0~23 , (0) low activity (1) high activity */
/*124*/	volatile uint32_t        gpio_output_change_enable;          /* [31-24] magic key(0xEA) [23-0] mapping to the number of iso gpio 0~23 */
/*128*/	volatile uint32_t        gpio_output_change_activity;        /* [31-24] magic key(0xEA) [23-0] mapping to the number of iso gpio 0~23 , (0) low activity (1) high activity AT SUSPEND TIME */
/*12C*/ volatile uint32_t        audio_reciprocal_timer_sec;         /* [31-24] magic key(0xEA) [23-0] audio reciprocal timer (sec) */
/*130*/ volatile uint32_t        u_boot_version_magic;
/*134*/ volatile uint32_t        u_boot_version_info;
/*138*/ volatile uint32_t        suspend_watchdog;                   /* [31-24] magic key(0xEA) [23] state (0) disable (1) enable [22] acpu response state [15-0] watch timeout (sec) */
/*13C*/ volatile uint32_t       xen_domu_boot_st;                    /* [31-24] magic key(0xEA) [23-20] version [19-16] Author (1) ACPU (2) SCPU [15-8] STATE [7-0] EXT */
/*140*/	volatile uint32_t		gpio_wakeup_enable2;				/* [31-24] magic key(0xEA) [10-0] mapping to the number of iso gpio 24~34 */
/*144*/	volatile uint32_t		gpio_wakeup_activity2;				/* [31-24] magic key(0xEA) [10-0] mapping to the number of iso gpio 24~34 , (0) low activity (1) high activity */
/*148*/	volatile uint32_t		gpio_output_change_enable2;			/* [31-24] magic key(0xEA) [10-0] mapping to the number of iso gpio 24~34 */
/*14C*/	volatile uint32_t		gpio_output_change_activity2;		/* [31-24] magic key(0xEA) [10-0] mapping to the number of iso gpio 24~34 , (0) low activity (1) high activity AT SUSPEND TIME */
};

#endif /* MY_DEF_HERE */

#if defined(MY_DEF_HERE)
/*
 * rtk_ipc_shm.h
 *
 * Copyright (c) 2017 Realtek Semiconductor Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 */

#ifndef __ASM_ARCH_RTK_IPC_SHM_H
#define __ASM_ARCH_RTK_IPC_SHM_H

extern volatile void __iomem *rpc_common_base;
#define IPC_SHM_VIRT (rpc_common_base + 0x000000C4)

struct rtk_ipc_shm {
	volatile uint32_t sys_assign_serial;
	volatile uint32_t pov_boot_vd_std_ptr;
	volatile uint32_t pov_boot_av_info;
	volatile uint32_t audio_rpc_flag;
	volatile uint32_t suspend_mask;
	volatile uint32_t suspend_flag;
	volatile uint32_t vo_vsync_flag;
	volatile uint32_t audio_fw_entry_pt;
	volatile uint32_t power_saving_ptr;
	volatile unsigned char printk_buffer[24];
	volatile uint32_t ir_extended_tbl_pt;
	volatile uint32_t vo_int_sync;
	volatile uint32_t bt_wakeup_flag;
	volatile uint32_t ir_scancode_mask;
	volatile uint32_t ir_wakeup_scancode;
	volatile uint32_t suspend_wakeup_flag;
	volatile uint32_t acpu_resume_state;
	volatile uint32_t gpio_wakeup_enable;
	volatile uint32_t gpio_wakeup_activity;
	volatile uint32_t gpio_output_change_enable;
	volatile uint32_t gpio_output_change_activity;
	volatile uint32_t audio_reciprocal_timer_sec;
	volatile uint32_t u_boot_version_magic;
	volatile uint32_t u_boot_version_info;
	volatile uint32_t suspend_watchdog;
	volatile uint32_t xen_domu_boot_st;
	volatile uint32_t gpio_wakeup_enable2;
	volatile uint32_t gpio_wakeup_activity2;
	volatile uint32_t gpio_output_change_enable2;
	volatile uint32_t gpio_output_change_activity2;
#ifdef CONFIG_ARCH_RTD139x
	volatile uint32_t gpio_wakeup_enable3;
	volatile uint32_t gpio_wakeup_activity3;
#endif
};

struct avcpu_syslog_struct{
	volatile uint32_t log_buf_addr;
	volatile uint32_t log_buf_len;
	volatile uint32_t logged_chars;
	volatile uint32_t log_start;
	volatile uint32_t con_start;
	volatile uint32_t log_end;
};

#if defined(CONFIG_RTK_XEN_SUPPORT) && defined(CONFIG_RTK_RPC)
#define SETMASK(bits, pos)		(((-1U) >> (32-bits))  << (pos))
#define CLRMASK(bits, pos)		(~(SETMASK(bits, pos)))
#define SET_VAL(val,bits,pos)		((val << pos) & SETMASK(bits, pos))
#define GET_VAL(reg,bits,pos)		((reg & SETMASK(bits, pos)) >> pos)

#define XEN_DOMU_BOOT_ST_MAGIC_KEY			(0xEA)
#define XEN_DOMU_BOOT_ST_MAGIC_KEY_MASK			(XEN_DOMU_BOOT_ST_MAGIC_KEY << 24)

#define XEN_DOMU_BOOT_ST_VERSION_SET(reg)		SET_VAL(reg, 4,20)
#define XEN_DOMU_BOOT_ST_VERSION_GET(reg)		GET_VAL(reg, 4,20)
#define XEN_DOMU_BOOT_ST_VERSION			(1)

#define XEN_DOMU_BOOT_ST_AUTHOR_SET(reg)		SET_VAL(reg, 4,16)
#define XEN_DOMU_BOOT_ST_AUTHOR_GET(reg)		GET_VAL(reg, 4,16)
#define XEN_DOMU_BOOT_ST_AUTHOR_ACPU			(1)
#define XEN_DOMU_BOOT_ST_AUTHOR_SCPU			(2)

#define XEN_DOMU_BOOT_ST_STATE_SET(reg)			SET_VAL(reg, 8, 8)
#define XEN_DOMU_BOOT_ST_STATE_GET(reg)			GET_VAL(reg, 8, 8)
#define XEN_DOMU_BOOT_ST_STATE_SCPU_BOOT		(1)
#define XEN_DOMU_BOOT_ST_STATE_SCPU_RESTART		(2)
#define XEN_DOMU_BOOT_ST_STATE_SCPU_POWOFF		(3)
#define XEN_DOMU_BOOT_ST_STATE_ACPU_LOCK		(4)
#define XEN_DOMU_BOOT_ST_STATE_ACPU_UNLOCK		(5)
#define XEN_DOMU_BOOT_ST_STATE_SCPU_WAIT_DONE		(6)
#define XEN_DOMU_BOOT_ST_STATE_ACPU_ENTER_IDLE		(7)
#define XEN_DOMU_BOOT_ST_STATE_SCPU_SUSPEND		(8)
#define XEN_DOMU_BOOT_ST_STATE_SCPU_RESUME		(9)
#define XEN_DOMU_BOOT_ST_STATE_ACPU_ACK			(10)

#define XEN_DOMU_BOOT_ST_EXT_SET(reg)			SET_VAL(reg, 8, 0)
#define XEN_DOMU_BOOT_ST_EXT_GET(reg)			GET_VAL(reg, 8, 0)
#endif /* defined(CONFIG_RTK_XEN_SUPPORT) && defined(CONFIG_RTK_RPC) */

#endif /* End of  __ASM_ARCH_RTK_IPC_SHM_H */
#endif /* MY_DEF_HERE */

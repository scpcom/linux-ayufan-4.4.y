/*
 * arch/arm/mach-comcerto/pm.c
 * C2K Power Management
 *
 * Copyright (C) 2012 Mindspeed
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/suspend.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/suspend.h>
#include <asm/irq.h>
#include <linux/atomic.h>
#include <asm/mach/time.h>
#include <asm/mach/irq.h>
#include <linux/console.h>


#include <linux/smp.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>
//#include <asm/hardware/gic.h>
#include <asm/mach-types.h>
#include <asm/smp_scu.h>
#include <mach/comcerto-2000/pm.h>
#include <mach/comcerto-2000/timer.h>
#include <linux/gpio.h>
#include <linux/clk.h>


unsigned int host_utilpe_shared_pmu_bitmask = ~(USB2p0_IRQ|WOL_IRQ) ;

/* Externs */
extern void comcerto_cpu_suspend(int save_state);
extern	unsigned int * c2k_get_restore_pointer(void);
extern void comcerto_cpu_restore (void);

unsigned int c2k_pm_bitmask_show(void)
{
	return host_utilpe_shared_pmu_bitmask;
}

void c2k_pm_bitmask_store(unsigned int bitmask_value)
{
	/*
	 * Initialize the shared pmu bitmask
	 * This information can be configurable run time.
	 * Can be passed from bootloader also (Not Implimented Yet)
	 */
	host_utilpe_shared_pmu_bitmask = bitmask_value;

	/* Pass the bitmask info to UtilPE */
	writel_relaxed(host_utilpe_shared_pmu_bitmask, (void*)HOST_UTILPE_SHARED_BITMASK);
}


static int comcerto_do_sram_idle(unsigned long save_state)
{
	comcerto_cpu_suspend(save_state);
	return 0;
}

/*------------------------- L2 Cache and SCU Save Resume ----------------------------*/
#define SCU_DATA_SIZE                32
#define L2_DATA_SIZE                 96

extern void pl310_save(void);
extern void pl310_resume(void);

typedef struct
{
    /* 0x00 */  volatile unsigned int control;
    /* 0x04 */  const unsigned int configuration;
    /* 0x08 */  union
                {
                    volatile unsigned int w;
                    volatile unsigned char b[4];
                } power_status;
    /* 0x0c */  volatile unsigned int invalidate_all;
                char padding1[48];
    /* 0x40 */  volatile unsigned int filtering_start;
    /* 0x44 */  volatile unsigned int filtering_end;
                char padding2[8];
    /* 0x50 */  volatile unsigned int access_control;
    /* 0x54 */  volatile unsigned int ns_access_control;
} a9_scu_registers;


void save_a9_scu(u32 *pointer, unsigned scu_address)
{
    a9_scu_registers *scu = (a9_scu_registers *)scu_address;

    pointer[0] = scu->control;
    pointer[1] = scu->power_status.w;
    pointer[2] = scu->filtering_start;
    pointer[3] = scu->filtering_end;
    pointer[4] = scu->access_control;
    pointer[5] = scu->ns_access_control;
}

void restore_a9_scu(u32 *pointer, unsigned scu_address)
{
    a9_scu_registers *scu = (a9_scu_registers *)scu_address;

    scu->invalidate_all = 0xffff;
    scu->filtering_start = pointer[2];
    scu->filtering_end = pointer[3];
    scu->access_control = pointer[4];
    scu->ns_access_control = pointer[5];
    scu->power_status.w = pointer[1];
    scu->control = pointer[0];
}


struct lockdown_regs
{
    unsigned int d, i;
};

typedef struct
{
    /* 0x000 */ const unsigned cache_id;
    /* 0x004 */ const unsigned cache_type;
                char padding1[0x0F8];
    /* 0x100 */ volatile unsigned control;
    /* 0x104 */ volatile unsigned aux_control;
    /* 0x108 */ volatile unsigned tag_ram_control;
    /* 0x10C */ volatile unsigned data_ram_control;
                char padding2[0x0F0];
    /* 0x200 */ volatile unsigned ev_counter_ctrl;
    /* 0x204 */ volatile unsigned ev_counter1_cfg;
    /* 0x208 */ volatile unsigned ev_counter0_cfg;
    /* 0x20C */ volatile unsigned ev_counter1;
    /* 0x210 */ volatile unsigned ev_counter0;
    /* 0x214 */ volatile unsigned int_mask;
    /* 0x218 */ const volatile unsigned int_mask_status;
    /* 0x21C */ const volatile unsigned int_raw_status;
    /* 0x220 */ volatile unsigned int_clear;
                char padding3[0x50C];
    /* 0x730 */ volatile unsigned cache_sync;
                char padding4[0x03C];
    /* 0x770 */ volatile unsigned inv_pa;
                char padding5[0x008];
    /* 0x77C */ volatile unsigned inv_way;
                char padding6[0x030];
    /* 0x7B0 */ volatile unsigned clean_pa;
                char padding7[0x004];
    /* 0x7B8 */ volatile unsigned clean_index;
    /* 0x7BC */ volatile unsigned clean_way;
                char padding8[0x030];
    /* 0x7F0 */ volatile unsigned clean_inv_pa;
                char padding9[0x004];
    /* 0x7F8 */ volatile unsigned clean_inv_index;
    /* 0x7FC */ volatile unsigned clean_inv_way;
                char paddinga[0x100];
    /* 0x900 */ volatile struct lockdown_regs lockdown[8];
                char paddingb[0x010];
    /* 0x950 */ volatile unsigned lock_line_en;
    /* 0x954 */ volatile unsigned unlock_way;
                char paddingc[0x2A8];
    /* 0xC00 */ volatile unsigned addr_filtering_start;
    /* 0xC04 */ volatile unsigned addr_filtering_end;
                char paddingd[0x338];
    /* 0xF40 */ volatile unsigned debug_ctrl;
                char paddinge[0x01C];
    /* 0xF60 */ volatile unsigned prefetch_ctrl;
                char paddingf[0x01C];
    /* 0xF80 */ volatile unsigned power_ctrl;
} pl310_registers;


typedef struct
{
    unsigned int aux_control;
    unsigned int tag_ram_control;
    unsigned int data_ram_control;
    unsigned int ev_counter_ctrl;
    unsigned int ev_counter1_cfg;
    unsigned int ev_counter0_cfg;
    unsigned int ev_counter1;
    unsigned int ev_counter0;
    unsigned int int_mask;
    unsigned int lock_line_en;
    struct lockdown_regs lockdown[8];
    unsigned int unlock_way;
    unsigned int addr_filtering_start;
    unsigned int addr_filtering_end;
    unsigned int debug_ctrl;
    unsigned int prefetch_ctrl;
    unsigned int power_ctrl;
} pl310_context;


void save_pl310(u32 *pointer, unsigned int pl310_address)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    pl310_context *context = (pl310_context *)pointer;
    int i;

    /* TODO: are all these registers are present in earlier PL310 versions? */
    context->aux_control = pl310->aux_control;
    context->tag_ram_control = pl310->tag_ram_control;
    context->data_ram_control = pl310->data_ram_control;
    context->ev_counter_ctrl = pl310->ev_counter_ctrl;
    context->ev_counter1_cfg = pl310->ev_counter1_cfg;
    context->ev_counter0_cfg = pl310->ev_counter0_cfg;
    context->ev_counter1 = pl310->ev_counter1;
    context->ev_counter0 = pl310->ev_counter0;
    context->int_mask = pl310->int_mask;
    context->lock_line_en = pl310->lock_line_en;

    for (i=0; i<8; ++i)
    {
        context->lockdown[i].d = pl310->lockdown[i].d;
        context->lockdown[i].i = pl310->lockdown[i].i;
    }
    context->addr_filtering_start = pl310->addr_filtering_start;
    context->addr_filtering_end = pl310->addr_filtering_end;
    context->debug_ctrl = pl310->debug_ctrl;
    context->prefetch_ctrl = pl310->prefetch_ctrl;
    context->power_ctrl = pl310->power_ctrl;
}

void restore_pl310(u32 *pointer, unsigned int pl310_address)
{
    pl310_registers *pl310 = (pl310_registers *)pl310_address;
    pl310_context *context = (pl310_context *)pointer;
    int i;

    /* We may need to disable the PL310 if the boot code has turned it on */
    if (pl310->control)
    {
        /* Wait for the cache to be idle, then disable */
        pl310->cache_sync = 0;
        dsb();
        pl310->control = 0;
    }

    /* TODO: are all these registers present in earlier PL310 versions? */
    pl310->aux_control = context->aux_control;
    pl310->tag_ram_control = context->tag_ram_control;
    pl310->data_ram_control = context->data_ram_control;
    pl310->ev_counter_ctrl = context->ev_counter_ctrl;
    pl310->ev_counter1_cfg = context->ev_counter1_cfg;
    pl310->ev_counter0_cfg = context->ev_counter0_cfg;
    pl310->ev_counter1 = context->ev_counter1;
    pl310->ev_counter0 = context->ev_counter0;
    pl310->int_mask = context->int_mask;
    pl310->lock_line_en = context->lock_line_en;
    for (i=0; i<8; ++i)
    {
        pl310->lockdown[i].d = context->lockdown[i].d;
        pl310->lockdown[i].i= context->lockdown[i].i;
    }
    pl310->addr_filtering_start = context->addr_filtering_start;
    pl310->addr_filtering_end = context->addr_filtering_end;
    pl310->debug_ctrl = context->debug_ctrl;
    pl310->prefetch_ctrl = context->prefetch_ctrl;
    pl310->power_ctrl = context->power_ctrl;
    dsb();
    pl310->control = 1;
    dsb();
}

/*------------------------- L2 Cache and SCU Save Resume ----------------------------*/

static void C2k_pm_suspend(void)
{
	/* Variable to tell what needs to be saved and restored
     	 * in C2k_pm_suspend_new */

	/* save_state = 0 => Nothing to save and restored */
	/* save_state = 1 => Only L1 and logic lost */
	/* save_state = 2 => Only L2 lost */
	/* save_state = 3 => L1, L2 and logic lost */
 	int save_state = 3;
	unsigned int * p0;
	struct clk *clk_axi;
	unsigned long axi_clk_rate;

	unsigned int scu_data[SCU_DATA_SIZE];
	unsigned int pl310_data[L2_DATA_SIZE];

	clk_axi = clk_get(NULL, "axi");

	if (IS_ERR(clk_axi)){
	    pr_err("%s: Unable to obtain axi clock: %ld\n",__func__,PTR_ERR(clk_axi));
	    /* System cannot proceed from here */
	    BUG();
	}

	axi_clk_rate = clk_get_rate(clk_axi);

	printk(KERN_INFO "PM: C2000 Device is trying to enter Suspend mode ...\n");

	p0 = (unsigned int *) comcerto_cpu_restore;

	__raw_writel(virt_to_phys(p0), phys_to_virt(0x20));
	__raw_writel((unsigned int)JUMP_TO_RESUME_1 , phys_to_virt(0x00));
	__raw_writel((unsigned int)JUMP_TO_RESUME_2 , phys_to_virt(0x04));
	smp_wmb();
	__cpuc_flush_dcache_area((void *)phys_to_virt(0x00), 0x24);
	outer_clean_range(__pa(phys_to_virt(0x00)), __pa(phys_to_virt(0x24)));

	printk(KERN_INFO "PM: C2000 Jump Location Installed ... -- 0x%x  -- 0x%x  -- 0x%x \n", (unsigned int)p0, (unsigned int)comcerto_cpu_restore, virt_to_phys(p0));

	printk(KERN_INFO "PM: Saving SCU Context ...\n");
	save_a9_scu(&scu_data[0], COMCERTO_SCU_VADDR);

	printk(KERN_INFO "PM: Saving L2 Cache Context ...\n");
	save_pl310(&pl310_data[0], COMCERTO_L310_VADDR);

	/* Pass the bitmask information to the PMU */
	*(((volatile unsigned int *)(HOST_UTILPE_SHARED_ADDRESS))+4) = host_utilpe_shared_pmu_bitmask;

	/* Pass the AXI Frequency information to the PMU */
	*((volatile unsigned int *)HOST_UTILPE_SHARED_PFEAXI_FREQ) = axi_clk_rate;

	printk(KERN_INFO "PM: Going to Suspend ...\n");

	cpu_suspend(save_state, comcerto_do_sram_idle);

	restore_a9_scu(&scu_data[0], COMCERTO_SCU_VADDR);
	restore_pl310(&pl310_data[0], COMCERTO_L310_VADDR);

	printk(KERN_INFO "PM: C2000  is re-starting from Suspend State ...\n");

	return;
}

/*  C2k_pm_enter
 *  @state:         State we're entering.
 */

static int C2k_pm_enter(suspend_state_t state)
{
	switch(state)
	{

	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		C2k_pm_suspend();
		break;
	default:
		return -EINVAL;
	}
	pr_info("PM: C2000 Leaving C2k_pm_enter \n");
	return 0;
}

static int C2k_pm_valid_state(suspend_state_t state)
{
	switch (state) {
		case PM_SUSPEND_ON:
		case PM_SUSPEND_STANDBY:
		case PM_SUSPEND_MEM:
			return 1;
		default:
			return 0;
	}
}

static suspend_state_t target_state;

/*
 * Called after processes are frozen, but before we shutdown devices.
 */
static int C2k_pm_begin(suspend_state_t state)
{
	target_state = state;
	return 0;
}

/*
 * Called right prior to thawing processes.
 */
static void C2k_pm_finish(void)
{
	printk(KERN_INFO "Suspend process is completed, Wait for C2000 device to resume \n");
}


/*
 * Called right prior to thawing processes.
 */
static void C2k_pm_end(void)
{
	printk(KERN_INFO "Resume process is completed, C2000 device is Power on Again \n");
        target_state = PM_SUSPEND_ON;
}


static const struct platform_suspend_ops C2k_pm_ops = {
	.valid	   = C2k_pm_valid_state,
	.begin     = C2k_pm_begin,
	.enter     = C2k_pm_enter,
	.finish    = C2k_pm_finish,
	.end       = C2k_pm_end,
};

static int __init C2k_pm_init(void)
{
	printk(KERN_INFO "Power Management Mode Support For C2000: \n");

	suspend_set_ops(&C2k_pm_ops);
        return 0;
}
arch_initcall(C2k_pm_init);


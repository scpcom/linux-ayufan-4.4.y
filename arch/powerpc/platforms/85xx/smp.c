/*
 * Author: Andy Fleming <afleming@freescale.com>
 * 	   Kumar Gala <galak@kernel.crashing.org>
 *
 * Copyright 2006-2008 Freescale Semiconductor Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/of.h>
#ifdef CONFIG_SYNO_QORIQ
#include <linux/cpu.h>
#endif

#include <asm/machdep.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/mpic.h>
#include <asm/cacheflush.h>
#include <asm/dbell.h>

#include <sysdev/fsl_soc.h>

#ifdef CONFIG_SYNO_QORIQ
#define MPC85xx_BPTR_OFF		0x00020
#define MPC85xx_ECM_EEBPCR_OFF		0x01010
#define MPC85xx_PIC_PIR_OFF		0x41090

extern void mpc85xx_cpu_down(void) __attribute__((noreturn));
#endif
extern void __early_start(void);
#ifdef CONFIG_SYNO_QORIQ
extern void __secondary_start_page(void);
extern volatile unsigned long __spin_table;

struct epapr_entry {
	u32	addr_h;
	u32	addr_l;
	u32	r3_h;
	u32	r3_l;
	u32	reserved;
	u32	pir;
	u32	r6_h;
	u32	r6_l;
};

/* hold epapr parameter table address */
static phys_addr_t epapr_tbl[NR_CPUS];

/* access per cpu vars from generic smp.c */
DECLARE_PER_CPU(int, cpu_state);

#if defined(CONFIG_HOTPLUG_CPU) && defined(CONFIG_PM)
static void __cpuinit
smp_85xx_mach_cpu_die(void)
{
	unsigned int cpu = smp_processor_id();

	local_irq_disable();
	idle_task_exit();
	per_cpu(cpu_state, cpu) = CPU_DEAD;
	mb();

	mtspr(SPRN_TSR, TSR_ENW | TSR_WIS | TSR_DIS | TSR_FIS);
	mtspr(SPRN_TCR, 0);
	mpc85xx_cpu_down();
}
#endif

static void __cpuinit
smp_85xx_reset_core(int nr)
{
	__iomem u32 *ecm_vaddr;
	__iomem u32 *pic_vaddr;
	u32 pcr, pir, cpu;

	cpu = (1 << 24) << nr;
	ecm_vaddr = ioremap(get_immrbase() + MPC85xx_ECM_EEBPCR_OFF, 4);
	pcr = in_be32(ecm_vaddr);
	if (pcr & cpu) {
		pic_vaddr = ioremap(get_immrbase() + MPC85xx_PIC_PIR_OFF, 4);
		pir = in_be32(pic_vaddr);
		/* reset assert */
		pir |= (1 << nr);
		out_be32(pic_vaddr, pir);
		pir = in_be32(pic_vaddr);
		pir &= ~(1 << nr);
		/* reset negate */
		out_be32(pic_vaddr, pir);
		(void)in_be32(pic_vaddr);
		iounmap(pic_vaddr);
	} else {
		out_be32(ecm_vaddr, pcr | cpu);
		(void)in_be32(ecm_vaddr);
	}
	iounmap(ecm_vaddr);
}

static int __cpuinit
smp_85xx_map_bootpg(unsigned long pa)
{
	__iomem u32 *bootpg_ptr;
	u32 bptr;

	/* Get the BPTR */
	bootpg_ptr = ioremap(get_immrbase() + MPC85xx_BPTR_OFF, 4);

	/* Set the BPTR to the secondary boot page */
	(void)in_be32(bootpg_ptr);

	bptr = (0x80000000 | (pa >> 12));
	out_be32(bootpg_ptr, bptr);
	(void)in_be32(bootpg_ptr);
	iounmap(bootpg_ptr);
	return 0;
}

static int __cpuinit
smp_85xx_unmap_bootpg(void)
{
	__iomem u32 *bootpg_ptr;

	/* Get the BPTR */
	bootpg_ptr = ioremap(get_immrbase() + MPC85xx_BPTR_OFF, 4);

	/* Restore the BPTR */
	if (in_be32(bootpg_ptr) & 0x80000000) {
		out_be32(bootpg_ptr, 0);
		(void)in_be32(bootpg_ptr);
	}
	iounmap(bootpg_ptr);
	return 0;
}

static void __cpuinit
#else
#define BOOT_ENTRY_ADDR_UPPER	0
#define BOOT_ENTRY_ADDR_LOWER	1
#define BOOT_ENTRY_R3_UPPER	2
#define BOOT_ENTRY_R3_LOWER	3
#define BOOT_ENTRY_RESV		4
#define BOOT_ENTRY_PIR		5
#define BOOT_ENTRY_R6_UPPER	6
#define BOOT_ENTRY_R6_LOWER	7
#define NUM_BOOT_ENTRY		8
#define SIZE_BOOT_ENTRY		(NUM_BOOT_ENTRY * sizeof(u32))

static void __init
#endif
smp_85xx_kick_cpu(int nr)
{
	unsigned long flags;
	const u64 *cpu_rel_addr;
#ifdef CONFIG_SYNO_QORIQ
	__iomem struct epapr_entry *epapr;
#else
	__iomem u32 *bptr_vaddr;
#endif
	struct device_node *np;
	int n = 0;

	WARN_ON (nr < 0 || nr >= NR_CPUS);

	pr_debug("smp_85xx_kick_cpu: kick CPU #%d\n", nr);

	np = of_get_cpu_node(nr, NULL);
	cpu_rel_addr = of_get_property(np, "cpu-release-addr", NULL);

	if (cpu_rel_addr == NULL) {
		printk(KERN_ERR "No cpu-release-addr for cpu %d\n", nr);
		return;
	}
#ifdef CONFIG_SYNO_QORIQ
	/* FIXME: following part doesn't care 36-bit addr mode. */
	if (epapr_tbl[nr] == 0)
		epapr_tbl[nr] = PAGE_MASK | (u32)*cpu_rel_addr;
	else {
		epapr_tbl[nr] = ((u32)&__spin_table - PAGE_OFFSET + nr * 0x20)
								| PAGE_MASK;
		pr_debug("cpu_release_addr=%08x, __spin_table=%p, nr=%08x\n",
					(u32)epapr_tbl[nr], &__spin_table, nr );
	}

	local_irq_save(flags);

	per_cpu(cpu_state, nr) = CPU_UP_PREPARE;

	if (system_state < SYSTEM_RUNNING) {
		epapr = ioremap(epapr_tbl[nr], sizeof(struct epapr_entry));
		out_be32(&epapr->pir, nr);
		out_be32(&epapr->addr_l, __pa(__early_start));
	} else {
		smp_85xx_map_bootpg(__pa(__secondary_start_page));
		epapr = ioremap(epapr_tbl[nr], sizeof(struct epapr_entry));
		smp_85xx_reset_core(nr);

		/* wait until core(nr) is ready... */
		while ((in_be32(&epapr->addr_l) != 1) && (++n < 1000))
			udelay(100);

		out_be32(&epapr->pir, nr);
		out_be32(&epapr->addr_l, __pa(__early_start));
	}

	/* Wait a bit for the CPU to ack. */
	n = 0;
	while ((__secondary_hold_acknowledge != nr) && (++n < 1000))
		mdelay(100);

	smp_85xx_unmap_bootpg();
	/* require dcache flush for cpu-release-addr ? */

	local_irq_restore(flags);
	iounmap(epapr);
#else

	/* Map the spin table */
	bptr_vaddr = ioremap(*cpu_rel_addr, SIZE_BOOT_ENTRY);

	local_irq_save(flags);

	out_be32(bptr_vaddr + BOOT_ENTRY_PIR, nr);
	out_be32(bptr_vaddr + BOOT_ENTRY_ADDR_LOWER, __pa(__early_start));

	/* Wait a bit for the CPU to ack. */
	while ((__secondary_hold_acknowledge != nr) && (++n < 1000))
		mdelay(1);

	local_irq_restore(flags);

	iounmap(bptr_vaddr);
#endif

	pr_debug("waited %d msecs for CPU #%d.\n", n, nr);
}

#ifdef CONFIG_SYNO_QORIQ
struct smp_ops_t smp_85xx_ops = {
};
#else
static void __init
smp_85xx_setup_cpu(int cpu_nr)
{
	mpic_setup_this_cpu();
}

struct smp_ops_t smp_85xx_ops = {
	.kick_cpu = smp_85xx_kick_cpu,
};
#endif

void __init mpc85xx_smp_init(void)
{
	struct device_node *np;
#ifdef CONFIG_SYNO_QORIQ
	int i;

	for (i = 0; i < NR_CPUS; i++)
		epapr_tbl[i] = 0;
#endif

	np = of_find_node_by_type(NULL, "open-pic");
	if (np) {
		smp_85xx_ops.probe = smp_mpic_probe;
#ifdef CONFIG_SYNO_QORIQ
		smp_85xx_ops.setup_cpu = smp_mpic_setup_cpu;
#else
		smp_85xx_ops.setup_cpu = smp_85xx_setup_cpu;
#endif
		smp_85xx_ops.message_pass = smp_mpic_message_pass;
#ifdef CONFIG_SYNO_QORIQ
		smp_85xx_ops.kick_cpu = smp_85xx_kick_cpu;
#if defined(CONFIG_HOTPLUG_CPU)
		smp_85xx_ops.give_timebase = smp_generic_give_timebase;
		smp_85xx_ops.take_timebase = smp_generic_take_timebase;
		smp_85xx_ops.cpu_disable   = generic_cpu_disable;
		smp_85xx_ops.cpu_die	= generic_cpu_die;
#ifdef CONFIG_PM
		ppc_md.cpu_die		= smp_85xx_mach_cpu_die;
#endif
#endif
#endif
	}

	if (cpu_has_feature(CPU_FTR_DBELL))
		smp_85xx_ops.message_pass = smp_dbell_message_pass;

	BUG_ON(!smp_85xx_ops.message_pass);

	smp_ops = &smp_85xx_ops;
}

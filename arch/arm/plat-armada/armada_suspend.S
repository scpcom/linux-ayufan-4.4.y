/*
 * arch/arm/plat-armada/armadaxp_suspend.S
 *
 * CPU idle low level implementation for Marvell ARMADA-XP SoCs
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 *
 */
#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/hardware.h>

#if 1  /* B0-GP board GPIO pins*/
#define MPP_CTRL_PM_REG				(INTER_REGS_BASE + 0x18008)
#define GPIO_OUT_VALUE_REG			(INTER_REGS_BASE + 0x18100)
#define GPIO_OUT_CTRL_REG			(INTER_REGS_BASE + 0x18104)
#define MPP_SET_GPIO_MASK			(0xFFFFF000)
#define GPIO_PIN_MASK				(0xFFF8FFFF)
#define GPIO_CMD_VALUE				(0x10000)
#define GPIO_ACK_VALUE				(0x70000)
#else /* B0-HE board GPIO pins */
#define MPP_SET_GPIO_MASK			(0xFFFFF000)
#define GPIO_PIN_MASK				(0xFFFFFFF8)
#define GPIO_CMD_VALUE				(0x1)
#define GPIO_ACK_VALUE				(0x7)
#define MPP_CTRL_PM_REG				(INTER_REGS_BASE + 0x18020)
#define GPIO_OUT_VALUE_REG			(INTER_REGS_BASE + 0x18180)
#define GPIO_OUT_CTRL_REG			(INTER_REGS_BASE + 0x18184)
#endif

#define MV_COHERENCY_FABRIC_CTRL_REG		(0x20200)

/*
* armadaxp_snoop_dis_virt:
* Disable delivery of snoop requests to the CPU core by setting
*/
ENTRY(armadaxp_snoop_dis_virt)
/* Save ARM registers */
	stmfd	sp!, {r4 - r11, lr}	@ save registers on stack

/* Disable SnoopEna */
	mrc	15, 0, r1, cr0, cr0, 5
	and	r1, r1, #15
	mov	r4, #1
	add	r5, r1, #24
	ldr	r2, =(MV_COHERENCY_FABRIC_CTRL_REG + INTER_REGS_BASE)
1:
	ldrex r3, [r2]
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	bic	r3, r3, r4, lsl r5
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	strex r0, r3, [r2]
	cmp	r0, #0
	bne 1b

	ldmfd	sp!, {r4 - r11, pc}	@ restore regs and return
ENDPROC(armadaxp_snoop_dis_virt)

/*
* armadaxp_cpu_suspend: enter cpu deepIdle state
* input:
*/
ENTRY(armadaxp_cpu_suspend)
/* Save ARM registers */
	stmfd	sp!, {r4 - r11, lr}	@ save registers on stack

	bl armadaxp_fabric_prepare_deepIdle

	/*
	 * Invalidate L1 data cache. Even though only invalidate is
	 * necessary exported flush API is used here. Doing clean
	 * on already clean cache would be almost NOP.
	*/
	bl v7_flush_dcache_all

	/*
	 * Clear the SCTLR.C bit to prevent further data cache
	 * allocation. Clearing SCTLR.C would make all the data accesses
	 * strongly ordered and would not hit the cache.
	*/
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(1 << 2)	@ Disable the C bit
	mcr	p15, 0, r0, c1, c0, 0
	isb

	bl v7_flush_dcache_all

	/* Data memory barrier and Data sync barrier */
	dsb
	dmb

	bl armadaxp_snoop_dis_virt

#ifdef CONFIG_ARMADA_DEEP_IDLE_SUPPORT_DRAM_SR
	ldr	r3, =(INTER_REGS_BASE + 0x1418)
	ldr	r4, [r3]
#ifdef CONFIG_BE8_ON_LE
	rev r4, r4
#endif
	orr	r4, r4, #0x7
#ifdef CONFIG_BE8_ON_LE
	rev r4, r4
#endif
	dsb				@ Data Synchronization Barrier

	.align 5
	str	r4, [r3]		@ Self refresh command
#endif

/*
 * ===================================
 * == WFI instruction => Enter idle ==
 * ===================================
 */

	wfi				@ wait for interrupt
/*
 * ===================================
 * == Resume path for non-OFF modes ==
 * ===================================
 */

/* Enable SnoopEna - Exclusive */
	mrc	15, 0, r1, cr0, cr0, 5
	and	r1, r1, #15
	mov	r4, #1
	add	r5, r1, #24
	ldr	r2, =(MV_COHERENCY_FABRIC_CTRL_REG + INTER_REGS_BASE)
1:
	ldrex r3, [r2]
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	orr	r3, r3, r4, lsl r5
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	strex r0, r3, [r2]
	cmp	r0, #0
	bne 1b

/* Re-enable C-bit if needed */
	mrc	p15, 0, r0, c1, c0, 0
	tst	r0, #(1 << 2)		@ Check C bit enabled?
	orreq	r0, r0, #(1 << 2)	@ Enable the C bit if cleared
	mcreq	p15, 0, r0, c1, c0, 0
	isb

	ldmfd	sp!, {r4 - r11, pc}	@ restore regs and return
ENDPROC(armadaxp_cpu_suspend)

/*
* armadaxp_cpu_resume: exit cpu deepIdle state
*/
ENTRY(armadaxp_cpu_resume)

#ifdef CONFIG_CPU_ENDIAN_BE32
	/* convert CPU to big endian */
	.word 0x100f11ee /* mrc p15, 0, r0, c1, c0 */
	.word 0x800080e3 /* orr r0, r0, #0x80 */
	.word 0x100f01ee /* mcr p15, 0, r0, c1, c0 */
#endif
#ifdef CONFIG_CPU_ENDIAN_BE8
	setend  be
#endif

/* Enable SnoopEna - Exclusive */
	mrc	15, 0, r1, cr0, cr0, 5
	and	r1, r1, #15
	mov	r6, #1
	add	r7, r1, #24
	ldr	r2, =(MV_COHERENCY_FABRIC_CTRL_REG + INTER_REGS_PHYS_BASE)
1:
	ldrex r3, [r2]
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	orr	r3, r3, r6, lsl r7
#ifdef CONFIG_BE8_ON_LE
	rev r3, r3
#endif
	strex r0, r3, [r2]
	cmp	r0, #0
	bne 1b

	/* Now branch to the common CPU resume function */
	b	cpu_resume

ENDPROC(armadaxp_cpu_resume)

/*
* armadaxp_powerdown: suspend to RAM - enter cpu deepIdle state
*/
ENTRY(armadaxp_powerdown)
	/* Save ARM registers */
	stmfd	sp!, {r4-r12, lr}		@ save registers on stack

	/*
	* Flush L1 DCache
	*/
	bl v7_flush_kern_cache_all

#ifdef CONFIG_CACHE_AURORA_L2
	/* Flush L2 Cache */
	bl auroraL2_flush_all
#endif

	/*
	* Issue a Data Synchronization Barrier instruction to ensure that all
	* state saving has been	completed.
	*/
	dsb

	/* Flush the DLB and wait ~7 usec*/
	ldr r2, =SDRAM_DLB_EVICT_REG
	ldr	r0, [r2]
	bic     r0, #0x000000FF
	str     r0, [r2]

	ldr r1, = 6000  /* WC freq =  1.6 Ghz, 2 cycles per loop */
1:	subs r1,r1,#1
	bne 1b

	/* Set DDR in battery backup mode
	 * It will exit self-refresh only on reset */
	ldr 	r2, =SDRAM_CONFIG_REG
	ldr	r0, [r2]
	bic     r0, #0x01000000
	str     r0, [r2]

	/* Prepare register for DDR Self refresh */
	ldr	r2, =(SDRAM_OPERATION_REG - INTER_REGS_BASE)
	ldr	r0, =INTER_REGS_BASE
	orr	r2, r2, r0
	ldr	r0, [r2]
#ifdef CONFIG_CPU_BIG_ENDIAN
	ldr	r3, =0x07000000
#else
	ldr	r3, =0x00000007
#endif
	orr	r0, r0, r3

	/*
	 * Write 0x1 then 0x7 through MPPs to PIC that controls power
	 * 0x1 - Power off all voltages;  0x7 - Acknowledge command
	 */

	/* Set MPPs to GPIO */

	ldr r3, =(MPP_CTRL_PM_REG)
	ldr r5, =(MPP_SET_GPIO_MASK)
	ldr r4, [r3]
	and r4, r4, r5
	str r4, [r3]

	/* Set output value */
	ldr r3, =(GPIO_OUT_VALUE_REG)
	ldr r6, =(GPIO_PIN_MASK)
	ldr r7, =(GPIO_CMD_VALUE)
	ldr r5, [r3]
	and r5, r5, r6
	orr r5, r5, r7
	str r5, [r3]

	/* Set pins as out */
	ldr r4, =(GPIO_OUT_CTRL_REG)
	ldr r5, [r4]
	and r5, r5, r6
	str r5, [r4]

	/* Prepare the ACK command */
	ldr r7, =(GPIO_ACK_VALUE)
	ldr r5, [r3]
	and r5, r5, r6
	orr r5, r5, r7

	/*
	 * Wait between cmd (0x1) and cmd ack (0x7)
	 * TODO - Need to reduce this delay
	 */
	ldr r1, =2000000000
1:	subs r1,r1,#1
	bne 1b

	/*
	 * Put Dram into self refresh. From here on we can perform
	 * 8 instructions to ensure execution from I-Cache
	 */
	.align 5
	str	r0, [r2]

	/* Wait 100 cycles for DDR to enter self refresh */
	ldr r1, = 50
1:	subs r1,r1,#1
	bne 1b

	/* Issue the cmd ack. This will turn of the board */
	str r5, [r3]

	/* trap the processor */
	b .

ENDPROC(armadaxp_powerdown)

	.macro	sheeva_pj4b_config
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_4948
	mrc 	p15, 1, r0, c15, c1, 0
	orr     r0, r0, #1			@ Disable L0 cache.
	mcr 	p15, 1, r0, c15, c1, 0
#endif
#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_4786
	mrc 	p15, 1, r0, c15, c1, 0
	orr     r0, r0, #(1 << 15)		@ Disable Dual issue for coprocessor instructions
	mcr 	p15, 1, r0, c15, c1, 0
#endif
	mrc 	p15, 1, r0, c15, c1, 1
	orr     r0, r0, #(1 << 16)		@ Disable data transfer for clean line.
	mcr 	p15, 1, r0, c15, c1, 1

	mrc 	p15, 1, r0, c15, c1, 2 
	bic     r0, r0, #(1 << 23)		@ Enable fast LDR.
	orr     r0, r0, #(1 << 25)		@ Disable interleave between normal write and snoop data intervene.
	orr     r0, r0, #(1 << 27)		@ Disable Critical Word First feature.
	orr     r0, r0, #(1 << 29)		@ Disable SO/device/EX non cacheable request to get out outstanding.
	orr     r0, r0, #(1 << 30) 		@ MBU change the way when it get second linefill
	mcr 	p15, 1, r0, c15, c1, 2

#ifdef CONFIG_SHEEVA_ERRATA_ARM_CPU_5315
	mrc     p15, 1, r0, c15, c2, 0
	orr     r0, r0, #(1 << 7)              @ Disable spec read from the MBu/LSU
        mcr     p15, 1, r0, c15, c2, 0
#else
	/* Speculative Prefetch is not enabled here - managed at runtime */
#endif

#ifdef CONFIG_AURORA_SF_ENABLED
	mrc 	p15, 1, r0, c15, c1, 2
	orr     r0, r0, #(1 << 31)		@ Enable Write Evict
	mcr 	p15, 1, r0, c15, c1, 2

	ldr     r0, =AXP_SNOOP_FILTER_PHYS_REG
        ldr     r10, [r0]
        orr     r10 , r10, #0x1                 @ SF_Enable (bit 0)
        str     r10,[r0] 
#endif

	.endm

/*
 * skip_errata_if_axp_b0 - get the chip revision and branch if B0 to skip errata.
 */
	.macro	skip_errata_if_axp_b0, reg
	ldr		\reg, soc_revision
	cmp		\reg, #0x2		/* MV_78XX0_B0_REV */
	beq		1f
	.endm

/*
 * a0_with_b0_errata_6075 - get the chip revision and branch if B0 to skip errata.
 */
	.macro	a0_with_b0_errata_6075, reg
	mrc      p15, 0, \reg, c1, c0, 0
	ands	\reg, \reg, #0x1
	ldr		\reg, =soc_revision
	bne		1f
	and		\reg, \reg, #0xFFFFFFF
1:
	ldr		\reg, [\reg]
	cmp		\reg, #0x2		/* MV_78XX0_B0_REV */
	beq		2f
	dsb
	b 		3f
2:
	dmb
3:
	.endm

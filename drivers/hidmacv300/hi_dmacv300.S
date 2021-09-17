	.arch armv7-a
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 2
	.eabi_attribute 34, 1
	.eabi_attribute 18, 4
	.file	"hi_dmacv300.c"
	.text
	.align	2
	.global	hi_dmac_channel_free
	.type	hi_dmac_channel_free, %function
hi_dmac_channel_free:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	add	ip, r3, r0, asl #2
	mov	r1, r0
	mov	r2, #0
	str	lr, [sp, #-4]!
	.save {lr}
	mov	r0, r2
	mov	lr, #121
	str	r2, [ip, #32]
	str	lr, [r3, r1, asl #2]
	ldr	pc, [sp], #4
	.fnend
	.size	hi_dmac_channel_free, .-hi_dmac_channel_free
	.align	2
	.global	hi_dmac_start_m2m
	.type	hi_dmac_start_m2m, %function
hi_dmac_start_m2m:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	ip, #:lower16:.LANCHOR0
	movt	ip, #:upper16:.LANCHOR0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	.save {r4, r5, r6, r7, r8, lr}
	mov	lr, r0, asl #6
	ldr	r5, [ip, #64]
	add	lr, lr, #2064
	add	r4, lr, #12
	mov	r0, #0
	add	r6, lr, #4
	add	r7, lr, #8
	str	r0, [r5, r4]
	movw	r5, #16512
	ldr	r8, [ip, #64]
	movt	r5, 49972
	str	r3, [r8, lr]
	ldr	r3, [ip, #64]
	str	r1, [r3, r6]
	ldr	r3, [ip, #64]
	str	r2, [r3, r7]
	ldr	r3, [ip, #64]
	str	r5, [r3, r4]
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}
	.fnend
	.size	hi_dmac_start_m2m, .-hi_dmac_start_m2m
	.align	2
	.global	hi_dmac_register_isr
	.type	hi_dmac_register_isr, %function
hi_dmac_register_isr:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r0, #7
	mov	r2, r0
	stmfd	sp!, {r4, lr}
	.save {r4, lr}
	bhi	.L11
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldr	r0, [r3, r0, asl #2]
	cmp	r0, #121
	bne	.L12
	ldr	ip, [r3, #64]
	add	r4, r3, r2, asl #2
	mov	lr, #1
	mov	r0, #0
	mov	lr, lr, asl r2
	str	r1, [r4, #32]
	str	lr, [ip, #1536]
	mov	r1, #120
	str	lr, [ip, #1552]
	str	r1, [r3, r2, asl #2]
	str	lr, [ip, #1560]
	ldmfd	sp!, {r4, pc}
.L11:
	mov	r1, r0
	movw	r0, #:lower16:.LC0
	movt	r0, #:upper16:.LC0
	bl	printk
	mvn	r0, #0
	ldmfd	sp!, {r4, pc}
.L12:
	mov	r1, r2
	movw	r0, #:lower16:.LC1
	movt	r0, #:upper16:.LC1
	bl	printk
	mvn	r0, #0
	ldmfd	sp!, {r4, pc}
	.fnend
	.size	hi_dmac_register_isr, .-hi_dmac_register_isr
	.section	.exit.text,"ax",%progbits
	.align	2
	.type	hi_dmac_module_exit, %function
hi_dmac_module_exit:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r3, r4, r5, lr}
	.save {r3, r4, r5, lr}
	movw	r4, #:lower16:.LANCHOR0
	movt	r4, #:upper16:.LANCHOR0
	mov	r1, #0
	mov	r0, #43
	movw	r5, #:lower16:.LANCHOR1
	movt	r5, #:upper16:.LANCHOR1
	bl	free_irq
	ldr	r0, [r4, #64]
	bl	__arm_iounmap
	mov	r0, r5
	bl	platform_device_unregister
	add	r0, r5, #376
	bl	platform_driver_unregister
	mov	r3, #0
	str	r3, [r4, #68]
	ldmfd	sp!, {r3, r4, r5, pc}
	.fnend
	.size	hi_dmac_module_exit, .-hi_dmac_module_exit
	.text
	.align	2
	.type	hi_iommu_remove, %function
hi_iommu_remove:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r3, r4, r5, lr}
	.save {r3, r4, r5, lr}
	add	r5, r0, #16
	mov	r0, r5
	bl	dev_get_drvdata
	movw	r3, #:lower16:.LANCHOR1
	movt	r3, #:upper16:.LANCHOR1
	ldr	r3, [r3, #460]
	cmp	r3, #2
	mov	r4, r0
	ble	.L22
.L16:
	mov	r0, r5
	mov	r1, #0
	bl	dev_set_drvdata
	cmp	r4, #0
	beq	.L17
	ldr	r0, [r4, #4]
	mov	r1, r4
	bl	free_irq
	ldr	r0, [r4]
	bl	__arm_iounmap
	mov	r0, r4
	bl	kfree
.L17:
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	mov	r2, #0
	mov	r0, r2
	str	r2, [r3, #72]
	ldmfd	sp!, {r3, r4, r5, pc}
.L22:
	movw	r2, #1028
	movw	r0, #:lower16:.LC2
	movw	r1, #:lower16:.LANCHOR2
	movt	r0, #:upper16:.LC2
	movt	r1, #:upper16:.LANCHOR2
	bl	printk
	movw	r0, #:lower16:.LC3
	movt	r0, #:upper16:.LC3
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
	b	.L16
	.fnend
	.size	hi_iommu_remove, .-hi_iommu_remove
	.align	2
	.type	hi_iommu_irq, %function
hi_iommu_irq:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	.save {r4, r5, r6, r7, r8, lr}
	movw	r0, #:lower16:.LC5
	mov	r6, r1
	movt	r0, #:upper16:.LC5
	ldr	r1, .L32
	mov	r8, #88
	bl	printk
	ldr	r7, .L32+4
	movt	r8, 1
	movw	r4, #32856
.L26:
	ldr	r3, [r6]
	add	r3, r3, r4
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr r5, [r3]
@ 0 "" 2
@ 819 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	movw	r3, #510
	and	r3, r3, r5
	cmp	r3, #0
	bne	.L31
.L24:
#APP
@ 834 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	ldr	r3, [r6]
	add	r3, r3, r4
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r5, [r3]
@ 0 "" 2
	add	r4, r4, #4096
	add	r7, r7, #12
	cmp	r4, r8
	bne	.L26
	mov	r0, #1
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}
.L31:
	ldr	r2, [r6]
	add	r3, r4, #8
	add	r3, r2, r3
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr r2, [r3]
@ 0 "" 2
@ 821 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	ldr	r2, [r6]
	add	r3, r4, #16
	add	r3, r2, r3
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr r2, [r3]
@ 0 "" 2
@ 822 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	ldr	r3, [r6]
	add	r3, r3, r4
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr r5, [r3]
@ 0 "" 2
@ 825 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
@ 826 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	ldr	r3, [r6]
	add	r3, r3, r4
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r5, [r3]
@ 0 "" 2
	movw	r0, #:lower16:.LC6
	movt	r0, #:upper16:.LC6
	bl	printk
	mov	r0, r7
	mov	r1, #3
	mov	r2, #1
	mov	r3, #0
	bl	__wake_up
	mov	r0, #94
	bl	disable_irq_nosync
	b	.L24
.L33:
	.align	2
.L32:
	.word	.LANCHOR2+16
	.word	.LANCHOR0+76
	.fnend
	.size	hi_iommu_irq, .-hi_iommu_irq
	.align	2
	.global	hi_dmac_channelstart
	.type	hi_dmac_channelstart, %function
hi_dmac_channelstart:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	cmp	r0, #7
	stmfd	sp!, {r3, lr}
	.save {r3, lr}
	bhi	.L38
	movw	r2, #:lower16:.LANCHOR0
	movt	r2, #:upper16:.LANCHOR0
	mov	r3, r0, asl #6
	mov	ip, #114
	ldr	r1, [r2, #64]
	add	r3, r3, #2064
	add	r3, r3, #12
	str	ip, [r2, r0, asl #2]
	mov	r0, #0
	ldr	r2, [r1, r3]
	orr	r2, r2, #1
	str	r2, [r1, r3]
	ldmfd	sp!, {r3, pc}
.L38:
	mov	r1, #8
	movw	r0, #:lower16:.LC7
	movt	r0, #:upper16:.LC7
	bl	printk
	mvn	r0, #21
	ldmfd	sp!, {r3, pc}
	.fnend
	.size	hi_dmac_channelstart, .-hi_dmac_channelstart
	.align	2
	.global	hi_iommu_checkpagefault
	.type	hi_iommu_checkpagefault, %function
hi_iommu_checkpagefault:
	.fnstart
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	stmfd	sp!, {r4, lr}
	.save {r4, lr}
	mov	r2, r0, asl #12
	ldr	r1, [r3, #72]
	add	r2, r2, #32768
	add	r2, r2, #88
	.pad #32
	sub	sp, sp, #32
	ldr	r1, [r1]
	add	r1, r1, r2
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr ip, [r1]
@ 0 "" 2
@ 123 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	movw	r1, #510
	mov	r0, r0, asl #2
	and	r1, r1, ip
	add	r4, sp, #32
	add	lr, r4, r0
	cmp	r1, #0
	str	ip, [lr, #-32]
	beq	.L39
#APP
@ 125 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	ldr	r1, [r3, #72]
	add	r3, sp, #32
	add	r0, r3, r0
	ldr	r1, [r1]
	ldr	r3, [r0, #-32]
	add	r2, r1, r2
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r2]
@ 0 "" 2
.L39:
	add	sp, sp, #32
	@ sp needed
	ldmfd	sp!, {r4, pc}
	.fnend
	.size	hi_iommu_checkpagefault, .-hi_iommu_checkpagefault
	.align	2
	.type	dmac_isr, %function
dmac_isr:
	.fnstart
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, lr}
	.save {r4, r5, r6, r7, r8, r9, r10, fp, lr}
	.pad #28
	sub	sp, sp, #28
	movw	r2, #:lower16:.LANCHOR0
	movt	r2, #:upper16:.LANCHOR0
	str	r2, [sp, #8]
	add	r2, sp, #24
	ldr	r1, [sp, #8]
	mov	r3, #0
	str	r2, [sp, #12]
	mov	ip, r3
	mov	r8, #1
	ldr	r2, [r1, #64]
	mov	r9, r1
	ldr	r7, [r2]
	ldr	r2, [sp, #12]
	str	r3, [r2, #-4]!
	str	r2, [sp, #12]
.L51:
	mov	lr, r8, asl ip
	movw	r4, #:lower16:.LANCHOR0
	tst	lr, r7
	add	r2, r9, ip, asl #2
	mov	r10, #1
	movt	r4, #:upper16:.LANCHOR0
	beq	.L46
	ldr	r3, [sp, #8]
	mov	r1, r2
	ldr	r0, [sp, #12]
	ldr	r5, [r3, #64]
	ldr	r3, [r5, #4]
	ldr	r6, [r5, #12]
	ldr	fp, [r5, #16]
	mov	r3, r3, lsr ip
	ands	r3, r3, r10
	mov	r6, r6, lsr ip
	mov	fp, fp, lsr ip
	bne	.L59
	ands	r6, r6, #1
	bne	.L60
	tst	fp, #1
	bne	.L61
	ldr	r3, [r2, #32]
	cmp	r3, #0
	beq	.L46
	blx	r3
.L46:
	ldr	ip, [sp, #20]
	add	ip, ip, #1
	str	ip, [sp, #20]
	cmp	ip, #7
	bls	.L51
.L48:
	mov	r0, #1
	add	sp, sp, #28
	@ sp needed
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L59:
	add	r0, ip, ip, asl #1
	add	r3, r4, #76
	add	r6, r4, ip, asl #2
	str	lr, [r5, #1536]
	add	r0, r3, r0, asl #2
	mov	r2, r10
	mov	r1, #3
	mov	r3, #0
	mov	lr, #116
	str	r10, [r6, #172]
	str	lr, [r4, ip, asl #2]
	bl	__wake_up
	mov	r0, #1
	add	sp, sp, #28
	@ sp needed
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L60:
	add	r0, ip, ip, asl #1
	add	r1, r4, #76
	add	r7, r4, ip, asl #2
	mov	r6, #3
	str	lr, [r5, #1552]
	add	r0, r1, r0, asl #2
	mov	r2, r10
	mov	r1, r6
	mvn	lr, #116
	str	r6, [r7, #172]
	str	lr, [r4, ip, asl #2]
	bl	__wake_up
	mov	r0, #1
	add	sp, sp, #28
	@ sp needed
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L61:
	mov	r0, ip
	mvn	r2, #117
	str	r2, [r4, ip, asl #2]
	str	r6, [sp, #4]
	bl	hi_iommu_checkpagefault
	ldr	r2, [sp, #20]
	mov	ip, #3
	ldr	lr, [r4, #64]
	add	r1, r4, #76
	ldr	r3, [sp, #4]
	mov	r5, r10, asl r2
	add	r0, r2, r2, asl #1
	add	r4, r4, r2, asl #2
	str	r5, [lr, #1560]
	mov	r2, r10
	add	r0, r1, r0, asl #2
	mov	r1, ip
	str	ip, [r4, #172]
	bl	__wake_up
	mov	r0, #1
	add	sp, sp, #28
	@ sp needed
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, pc}
	.fnend
	.size	dmac_isr, .-dmac_isr
	.align	2
	.global	hi_iommu_checkpagefault_poll
	.type	hi_iommu_checkpagefault_poll, %function
hi_iommu_checkpagefault_poll:
	.fnstart
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	stmfd	sp!, {r4, lr}
	.save {r4, lr}
	mov	r2, r0, asl #12
	ldr	r1, [r3, #72]
	add	r2, r2, #32768
	add	r2, r2, #88
	.pad #32
	sub	sp, sp, #32
	ldr	r1, [r1]
	add	r1, r1, r2
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr ip, [r1]
@ 0 "" 2
@ 135 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	movw	r1, #510
	mov	r0, r0, asl #2
	and	r1, r1, ip
	add	r4, sp, #32
	add	lr, r4, r0
	cmp	r1, #0
	str	ip, [lr, #-32]
	beq	.L62
#APP
@ 137 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	ldr	r1, [r3, #72]
	add	r3, sp, #32
	add	r0, r3, r0
	ldr	r1, [r1]
	ldr	r3, [r0, #-32]
	add	r2, r1, r2
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r2]
@ 0 "" 2
.L62:
	add	sp, sp, #32
	@ sp needed
	ldmfd	sp!, {r4, pc}
	.fnend
	.size	hi_iommu_checkpagefault_poll, .-hi_iommu_checkpagefault_poll
	.align	2
	.global	hi_dmac_wait
	.type	hi_dmac_wait, %function
hi_dmac_wait:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	.save {r4, r5, r6, lr}
	movw	r5, #:lower16:.LANCHOR0
	movt	r5, #:upper16:.LANCHOR0
	mov	r2, #1
	mov	r6, r0
	mov	r2, r2, asl r0
	ldr	r3, [r5, #64]
	mov	ip, #1048576
	b	.L73
.L69:
	tst	r4, r2
	bne	.L77
	tst	lr, r2
	bne	.L78
	subs	ip, ip, #1
	beq	.L79
.L73:
	ldr	lr, [r3, #1536]
	ldr	r4, [r3, #1552]
	tst	r2, lr
	ldr	lr, [r3, #1560]
	beq	.L69
	mov	r2, #1
	mov	r0, #116
	mov	r2, r2, asl r6
	str	r0, [r5, r6, asl #2]
	str	r2, [r3, #1536]
	ldmfd	sp!, {r4, r5, r6, pc}
.L78:
	mov	r0, r6
	mvn	r3, #117
	str	r3, [r5, r6, asl #2]
	bl	hi_iommu_checkpagefault_poll
	ldr	r3, [r5, #64]
	mov	r2, #1
	mov	r6, r2, asl r6
	mvn	r0, #0
	str	r6, [r3, #1560]
	ldmfd	sp!, {r4, r5, r6, pc}
.L79:
	mvn	r0, #0
	ldmfd	sp!, {r4, r5, r6, pc}
.L77:
	mov	r1, r6
	movw	r0, #:lower16:.LC8
	movt	r0, #:upper16:.LC8
	mvn	r3, #116
	str	r3, [r5, r6, asl #2]
	bl	printk
	ldr	r3, [r5, #64]
	mov	r2, #1
	mov	r6, r2, asl r6
	mvn	r0, #0
	str	r6, [r3, #1552]
	ldmfd	sp!, {r4, r5, r6, pc}
	.fnend
	.size	hi_dmac_wait, .-hi_dmac_wait
	.align	2
	.global	hi_dmac_check_over
	.type	hi_dmac_check_over, %function
hi_dmac_check_over:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	stmfd	sp!, {r4, lr}
	.save {r4, lr}
	ldr	r4, [r3, r0, asl #2]
	cmn	r4, #117
	beq	.L98
	cmn	r4, #118
	beq	.L98
	cmp	r4, #114
	beq	.L82
	cmp	r4, #120
	beq	.L82
	cmp	r4, #121
	beq	.L82
	cmn	r4, #119
	beq	.L99
	cmp	r4, #116
	beq	.L82
	ldmfd	sp!, {r4, pc}
.L99:
	mov	r1, r0
	movw	r0, #:lower16:.LC9
	movt	r0, #:upper16:.LC9
	bl	printk
.L82:
	mov	r0, r4
	ldmfd	sp!, {r4, pc}
.L98:
	mov	r2, r0, asl #6
	ldr	ip, [r3, #64]
	add	r2, r2, #2064
	mov	lr, #0
	add	r2, r2, #12
	mov	r1, #121
	str	lr, [ip, r2]
	str	r1, [r3, r0, asl #2]
	mov	r0, r4
	ldmfd	sp!, {r4, pc}
	.fnend
	.size	hi_dmac_check_over, .-hi_dmac_check_over
	.align	2
	.global	hi_dmac_channel_allocate
	.type	hi_dmac_channel_allocate, %function
hi_dmac_channel_allocate:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	.save {r4, r5, r6, lr}
	mov	r6, r0
	ldr	r0, .L110
	mov	r4, #0
	bl	_raw_spin_lock_irqsave
	mov	r5, r0
.L102:
	mov	r0, r4
	add	r4, r4, #1
	bl	hi_dmac_check_over
	cmp	r4, #8
	bne	.L102
	movw	ip, #:lower16:.LANCHOR0
	movt	ip, #:upper16:.LANCHOR0
	sub	r3, ip, #4
	mov	r4, #0
	ldr	r2, [ip, #64]
	ldr	r1, [r2, #1680]
	uxth	r1, r1
	b	.L106
.L103:
	add	r4, r4, #1
	cmp	r4, #8
	beq	.L109
.L106:
	ldr	r2, [r3, #4]!
	cmp	r2, #121
	bne	.L103
	mov	r2, r1, lsr r4
	tst	r2, #1
	bne	.L103
	add	r2, ip, r4, asl #2
	mov	r1, r5
	ldr	r0, .L110
	mov	r3, #120
	str	r6, [r2, #32]
	str	r3, [ip, r4, asl #2]
	bl	_raw_spin_unlock_irqrestore
	mov	r0, r4
	ldmfd	sp!, {r4, r5, r6, pc}
.L109:
	mov	r1, r5
	ldr	r0, .L110
	bl	_raw_spin_unlock_irqrestore
	mov	r0, #101
	ldmfd	sp!, {r4, r5, r6, pc}
.L111:
	.align	2
.L110:
	.word	.LANCHOR0+204
	.fnend
	.size	hi_dmac_channel_allocate, .-hi_dmac_channel_allocate
	.align	2
	.global	hi_dmac_init
	.type	hi_dmac_init, %function
hi_dmac_init:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r3, lr}
	.save {r3, lr}
	mov	r0, #0
	mov	r1, #12288
	movt	r0, 4648
	mov	r2, #0
	bl	__arm_ioremap
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	cmp	r0, #0
	str	r0, [r3, #64]
	beq	.L115
.L113:
	mov	r0, #0
	ldmfd	sp!, {r3, pc}
.L115:
	movw	r0, #:lower16:.LC10
	mov	r1, #0
	movt	r0, #:upper16:.LC10
	movt	r1, 4648
	bl	printk
	b	.L113
	.fnend
	.size	hi_dmac_init, .-hi_dmac_init
	.align	2
	.global	dmac_config_addr_feature
	.type	dmac_config_addr_feature, %function
dmac_config_addr_feature:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	str	lr, [sp, #-4]!
	.save {lr}
	mov	r0, r0, asl #6
	ldr	lr, [r3, #64]
	add	r0, r0, #2080
	sub	r1, r1, #1
	movw	ip, #65534
	movt	ip, 48895
	sub	r2, r2, #1
	cmp	r1, ip
	mov	r1, #0
	str	r1, [lr, r0]
	ldr	r1, [r3, #64]
	ldr	ip, [r1, r0]
	bicls	ip, ip, #131072
	orrhi	ip, ip, #131072
	str	ip, [r1, r0]
	movw	r1, #65534
	ldr	r3, [r3, #64]
	movt	r1, 48895
	cmp	r2, r1
	ldr	r2, [r3, r0]
	bicls	r2, r2, #32
	orrhi	r2, r2, #32
	str	r2, [r3, r0]
	ldr	pc, [sp], #4
	.fnend
	.size	dmac_config_addr_feature, .-dmac_config_addr_feature
	.align	2
	.global	hi_dmac_driver_init
	.type	hi_dmac_driver_init, %function
hi_dmac_driver_init:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	.save {r4, r5, r6, r7, r8, lr}
	.pad #8
	sub	sp, sp, #8
	bl	hi_dmac_init
	mov	r3, #0
	mov	r2, r3
	str	r3, [sp, #4]
	movw	r0, #:lower16:.LC11
	movw	r1, #:lower16:dmac_isr
	movt	r0, #:upper16:.LC11
	movt	r1, #:upper16:dmac_isr
	str	r0, [sp]
	mov	r0, #43
	bl	request_threaded_irq
	subs	r8, r0, #0
	bne	.L127
	movw	r7, #:lower16:.LANCHOR0
	movt	r7, #:upper16:.LANCHOR0
	sub	r5, r7, #4
	add	r4, r7, #76
	ldr	r3, [r7, #64]
	add	r7, r7, #28
	mov	r6, #121
	mov	r2, #255
	str	r2, [r3, #24]
	str	r2, [r3, #32]
	str	r2, [r3, #36]
.L125:
	mov	r0, r4
	str	r6, [r5, #4]!
	movw	r1, #:lower16:.LC13
	ldr	r2, .L128
	movt	r1, #:upper16:.LC13
	add	r4, r4, #12
	bl	__init_waitqueue_head
	cmp	r5, r7
	bne	.L125
.L124:
	mov	r0, r8
	add	sp, sp, #8
	@ sp needed
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}
.L127:
	movw	r0, #:lower16:.LC12
	mov	r1, #43
	movt	r0, #:upper16:.LC12
	mvn	r8, #0
	bl	printk
	b	.L124
.L129:
	.align	2
.L128:
	.word	.LANCHOR0+208
	.fnend
	.size	hi_dmac_driver_init, .-hi_dmac_driver_init
	.section	.init.text,"ax",%progbits
	.align	2
	.type	hi_dmac_module_init, %function
hi_dmac_module_init:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, lr}
	.save {r4, lr}
#APP
@ 1090 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	mov	r3, #0
	mov	r2, #10
	movt	r3, 64772
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r2, [r3, #224]
@ 0 "" 2
	movw	r3, #:lower16:arm_delay_ops
	movt	r3, #:upper16:arm_delay_ops
	movw	r0, #55032
	movt	r0, 163
	ldr	r3, [r3, #4]
	blx	r3
	movw	r0, #:lower16:.LANCHOR1
	movt	r0, #:upper16:.LANCHOR1
	bl	platform_device_register
	subs	r4, r0, #0
	beq	.L131
	movw	r0, #:lower16:.LC14
	ldr	r1, .L137
	movt	r0, #:upper16:.LC14
	movw	r2, #1096
	bl	printk
	movw	r0, #:lower16:.LC15
	movt	r0, #:upper16:.LC15
	b	.L136
.L131:
	ldr	r0, .L137+4
	bl	platform_driver_register
	subs	r4, r0, #0
	beq	.L133
	movw	r0, #:lower16:.LANCHOR1
	movt	r0, #:upper16:.LANCHOR1
	bl	platform_device_unregister
	movw	r0, #:lower16:.LC14
	ldr	r1, .L137
	movt	r0, #:upper16:.LC14
	movw	r2, #1103
	bl	printk
	movw	r0, #:lower16:.LC16
	movt	r0, #:upper16:.LC16
.L136:
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
	mov	r0, r4
	ldmfd	sp!, {r4, pc}
.L133:
	bl	hi_dmac_driver_init
	cmp	r0, #0
	movweq	r3, #:lower16:.LANCHOR0
	movteq	r3, #:upper16:.LANCHOR0
	moveq	r2, #1
	mvnne	r0, #0
	streq	r2, [r3, #68]
	ldmfd	sp!, {r4, pc}
.L138:
	.align	2
.L137:
	.word	.LANCHOR2+32
	.word	.LANCHOR1+376
	.fnend
	.size	hi_dmac_module_init, .-hi_dmac_module_init
	.text
	.align	2
	.global	hi_iommu_attach
	.type	hi_iommu_attach, %function
hi_iommu_attach:
	.fnstart
	@ args = 0, pretend = 0, frame = 32
	@ frame_needed = 0, uses_anonymous_args = 0
	str	lr, [sp, #-4]!
	.save {lr}
	.pad #36
	sub	sp, sp, #36
	add	lr, sp, #32
	movw	r2, #:lower16:init_mm
	mov	r3, sp
	bic	r1, r3, #8128
	bic	r1, r1, #63
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	add	ip, lr, r0, asl #2
	ldr	r1, [r1, #12]
	mov	lr, #0
	ldr	r3, [r3, #72]
	movt	r2, #:upper16:init_mm
	str	lr, [ip, #-32]
	ldr	r1, [r1, #196]
	ldr	r3, [r3]
	cmp	r1, lr
	moveq	r1, r2
#APP
@ 607 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	ip, sp, #32
	mov	r2, r0, asl #12
	add	lr, ip, r0, asl #2
	add	ip, r2, #34304
	add	ip, ip, #24
	ldr	lr, [lr, #-32]
	add	ip, r3, ip
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str lr, [ip]
@ 0 "" 2
	add	lr, sp, #32
	ldr	r1, [r1, #40]
	add	ip, lr, r0, asl #2
#APP
@ 175 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/memory.h" 1
	@ __pv_stub
1:	add	r1, r1, #-2130706432
	.pushsection .pv_table,"a"
	.long	1b
	.popsection

@ 0 "" 2
	orr	r1, r1, #72
	str	r1, [ip, #-32]
#APP
@ 612 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	lr, sp, #32
	add	r1, r2, #32768
	add	ip, lr, r0, asl #2
	add	r1, r1, #32
	add	r1, r3, r1
	ldr	ip, [ip, #-32]
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str ip, [r1]
@ 0 "" 2
	add	ip, sp, #32
	add	r1, ip, r0, asl #2
	mov	ip, #1
	str	ip, [r1, #-32]
#APP
@ 615 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	lr, sp, #32
	add	r1, r2, #32768
	add	ip, lr, r0, asl #2
	add	r1, r1, #4
	add	r1, r3, r1
	ldr	ip, [ip, #-32]
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str ip, [r1]
@ 0 "" 2
	add	ip, sp, #32
	add	r1, ip, r0, asl #2
	movw	ip, #4135
	str	ip, [r1, #-32]
#APP
@ 618 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	lr, sp, #32
	add	r2, r2, #32768
	add	r0, lr, r0, asl #2
	add	r3, r3, r2
	ldr	r2, [r0, #-32]
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r2, [r3]
@ 0 "" 2
	mov	r0, #0
	add	sp, sp, #36
	@ sp needed
	ldr	pc, [sp], #4
	.fnend
	.size	hi_iommu_attach, .-hi_iommu_attach
	.align	2
	.global	hi_iommu_detach
	.type	hi_iommu_detach, %function
hi_iommu_detach:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.fnend
	.size	hi_iommu_detach, .-hi_iommu_detach
	.align	2
	.global	hi_iommu_resume
	.type	hi_iommu_resume, %function
hi_iommu_resume:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldr	r3, [r3, #72]
	ldr	r3, [r3]
#APP
@ 632 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	r0, r3, r0, asl #12
	mov	r3, #0
	add	r0, r0, #32768
	add	r0, r0, #8
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r0]
@ 0 "" 2
	bx	lr
	.fnend
	.size	hi_iommu_resume, .-hi_iommu_resume
	.align	2
	.global	dmac_transfer_fix
	.type	dmac_transfer_fix, %function
dmac_transfer_fix:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r3, #:lower16:.LANCHOR0
	movt	r3, #:upper16:.LANCHOR0
	ldr	r3, [r3, #72]
	ldr	r3, [r3]
#APP
@ 632 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	r0, r3, r0, asl #12
	mov	r3, #0
	add	r0, r0, #32768
	add	r0, r0, #8
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r0]
@ 0 "" 2
	bx	lr
	.fnend
	.size	dmac_transfer_fix, .-dmac_transfer_fix
	.align	2
	.global	disable_intr_channel
	.type	disable_intr_channel, %function
disable_intr_channel:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	.save {r4, r5, r6, lr}
	movw	r4, #:lower16:.LANCHOR0
	movt	r4, #:upper16:.LANCHOR0
	mov	r6, r0
	add	r5, r4, #208
	mov	r0, r5
	bl	_raw_spin_lock_irqsave
	ldr	r3, [r4, #64]
	mov	r2, #1
	mvn	r2, r2, asl r6
	ldr	ip, [r3, #24]
	and	ip, ip, r2
	str	ip, [r3, #24]
	ldr	ip, [r3, #32]
	and	ip, ip, r2
	str	ip, [r3, #32]
	ldr	ip, [r3, #36]
	and	r2, r2, ip
	str	r2, [r3, #36]
	mov	r1, r0
	mov	r0, r5
	ldmfd	sp!, {r4, r5, r6, lr}
	b	_raw_spin_unlock_irqrestore
	.fnend
	.size	disable_intr_channel, .-disable_intr_channel
	.align	2
	.global	enable_intr_channel
	.type	enable_intr_channel, %function
enable_intr_channel:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, lr}
	.save {r4, r5, r6, lr}
	movw	r4, #:lower16:.LANCHOR0
	movt	r4, #:upper16:.LANCHOR0
	mov	r6, r0
	add	r5, r4, #208
	mov	r0, r5
	bl	_raw_spin_lock_irqsave
	ldr	r3, [r4, #64]
	mov	r2, #1
	mov	r2, r2, asl r6
	ldr	ip, [r3, #24]
	orr	ip, r2, ip
	str	ip, [r3, #24]
	ldr	ip, [r3, #32]
	orr	ip, r2, ip
	str	ip, [r3, #32]
	ldr	ip, [r3, #36]
	orr	r2, r2, ip
	str	r2, [r3, #36]
	mov	r1, r0
	mov	r0, r5
	ldmfd	sp!, {r4, r5, r6, lr}
	b	_raw_spin_unlock_irqrestore
	.fnend
	.size	enable_intr_channel, .-enable_intr_channel
	.align	2
	.global	hi_dmac_m2m_transfer
	.type	hi_dmac_m2m_transfer, %function
hi_dmac_m2m_transfer:
	.fnstart
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, lr}
	.save {r4, r5, r6, r7, r8, r9, r10, fp, lr}
	mov	r8, r0
	.pad #28
	sub	sp, sp, #28
	mov	r0, #0
	mov	r7, r1
	mov	fp, r2
	bl	hi_dmac_channel_allocate
	cmp	r0, #101
	mov	r4, r0
	beq	.L163
	mov	r3, sp
	mov	r2, r8
	bic	r10, r3, #8128
	mov	r1, r7
	bl	dmac_config_addr_feature
	bic	r3, r10, #63
	ldr	r3, [r3, #4]
	bics	r2, r3, #1073741824
	bne	.L175
.L152:
	movw	r5, #:lower16:.LANCHOR0
	mov	r6, r4, asl #2
	movt	r5, #:upper16:.LANCHOR0
	mov	r0, r4
	add	r9, r5, r6
	bl	hi_iommu_attach
	mov	r1, r8
	mov	r2, r7
	mov	r3, fp
	mov	r0, r4
	mov	ip, #0
	str	ip, [r9, #172]
	bl	hi_dmac_start_m2m
	cmp	r0, #0
	beq	.L153
.L174:
	mvn	r0, #0
.L154:
	add	r6, r5, r6
	mov	r2, #0
	mov	r3, #121
	str	r3, [r5, r4, asl #2]
	str	r2, [r6, #32]
.L151:
	add	sp, sp, #28
	@ sp needed
	ldmfd	sp!, {r4, r5, r6, r7, r8, r9, r10, fp, pc}
.L175:
	mov	r0, r4
	bl	disable_intr_channel
	b	.L152
.L153:
	mov	r0, r4
	bl	hi_dmac_channelstart
	subs	r7, r0, #0
	bne	.L174
	bic	r10, r10, #63
	ldr	r3, [r10, #4]
	bics	r2, r3, #1073741824
	bne	.L176
	ldr	r0, [r9, #172]
	cmp	r0, #0
	bne	.L158
	ldr	r3, [r10, #12]
	add	r8, r4, r4, asl #1
	add	r2, r5, #76
	mov	r7, #100
	add	r8, r2, r8, asl #2
	add	r2, sp, #16
	stmib	sp, {r0, r3}
	movw	r3, #:lower16:autoremove_wake_function
	str	r2, [sp, #16]
	movt	r3, #:upper16:autoremove_wake_function
	str	r2, [sp, #20]
	str	r3, [sp, #12]
	b	.L161
.L177:
	bl	schedule_timeout
	subs	r7, r0, #0
	beq	.L160
.L161:
	mov	r0, r8
	add	r1, sp, #4
	mov	r2, #2
	bl	prepare_to_wait
	ldr	r3, [r9, #172]
	mov	r0, r7
	cmp	r3, #0
	beq	.L177
	cmp	r7, #0
	beq	.L160
.L162:
	mov	r0, r8
	add	r1, sp, #4
	bl	finish_wait
	cmp	r7, #0
	ble	.L174
	add	r3, r5, r6
	ldr	r0, [r3, #172]
.L158:
	cmp	r0, #3
	movne	r0, #0
	mvneq	r0, #0
	b	.L154
.L176:
	mov	r0, r4
	bl	hi_dmac_wait
	cmp	r0, #116
	mov	r0, r4
	beq	.L157
	bl	enable_intr_channel
	b	.L174
.L163:
	mvn	r0, #0
	b	.L151
.L160:
	add	r3, r5, r6
	ldr	r7, [r3, #172]
	adds	r7, r7, #0
	movne	r7, #1
	b	.L162
.L157:
	bl	enable_intr_channel
	mov	r0, r7
	b	.L154
	.fnend
	.size	hi_dmac_m2m_transfer, .-hi_dmac_m2m_transfer
	.align	2
	.global	hi_memcpy
	.type	hi_memcpy, %function
hi_memcpy:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r3, lr}
	.save {r3, lr}
	mov	r3, r0
	mov	r0, r1
	mov	r1, r3
	bl	hi_dmac_m2m_transfer
	and	r0, r0, r0, asr #31
	ldmfd	sp!, {r3, pc}
	.fnend
	.size	hi_memcpy, .-hi_memcpy
	.align	2
	.global	hi_iommu_hardware_init
	.type	hi_iommu_hardware_init, %function
hi_iommu_hardware_init:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, r5, r6, r7, r8, lr}
	.save {r4, r5, r6, r7, r8, lr}
	ldr	r4, [r0]
#APP
@ 846 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	mov	r3, #0
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4, #96]
@ 0 "" 2
@ 847 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4, #108]
@ 0 "" 2
@ 848 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4, #104]
@ 0 "" 2
@ 849 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	r7, r4, #112
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4, #112]
@ 0 "" 2
@ 475 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4, #112]
@ 0 "" 2
	add	r5, r4, #116
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr r3, [r4, #116]
@ 0 "" 2
@ 476 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	tst	r3, #1
	beq	.L182
	movw	r6, #16959
	movw	r8, #:lower16:arm_delay_ops
	movt	r6, 15
	movt	r8, #:upper16:arm_delay_ops
	b	.L183
.L185:
	subs	r6, r6, #1
	beq	.L217
.L183:
	ldr	r3, [r8, #4]
	movw	r0, #41838
	movt	r0, 1
	blx	r3
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr r3, [r5]
@ 0 "" 2
@ 476 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	tst	r3, #1
	bne	.L185
.L182:
#APP
@ 861 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	movw	r3, #6198
	movt	r3, 32
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4]
@ 0 "" 2
@ 866 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	mov	r3, #33554432
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4, #4]
@ 0 "" 2
	mov	r3, #0
.L188:
#APP
@ 871 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	orr	r1, r3, #-2147483648
	add	r2, r4, r3, asl #2
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r1, [r2, #2048]
@ 0 "" 2
	add	r3, r3, #1
	cmp	r3, #8
	bne	.L188
	mov	r3, #0
.L190:
#APP
@ 878 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	orr	r1, r3, #50331648
	add	r2, r4, r3, asl #2
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r1, [r2, #3072]
@ 0 "" 2
	add	r3, r3, #1
	cmp	r3, #8
	bne	.L190
	mov	r3, #0
	mov	r1, #126976
.L192:
#APP
@ 884 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	r2, r4, r3, asl #2
	add	r2, r2, #4096
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r1, [r2]
@ 0 "" 2
	add	r3, r3, #1
	cmp	r3, #8
	bne	.L192
	mov	r3, #0
	mov	r1, r3
.L194:
#APP
@ 889 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	r2, r4, r3, asl #2
	add	r2, r2, #6144
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r1, [r2]
@ 0 "" 2
	add	r3, r3, #1
	cmp	r3, #8
	bne	.L194
	add	r0, r4, #32768
	mov	r3, #0
	add	r0, r0, #16
	add	r2, r4, #32768
	mov	ip, r3
.L196:
#APP
@ 894 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	r1, r0, r3
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str ip, [r1]
@ 0 "" 2
	add	r3, r3, #4096
	cmp	r3, #32768
	bne	.L196
	add	r0, r4, #32768
	mov	r3, #0
	add	r0, r0, #48
	mov	ip, r3
.L198:
#APP
@ 899 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	r1, r0, r3
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str ip, [r1]
@ 0 "" 2
	add	r3, r3, #4096
	cmp	r3, #32768
	bne	.L198
	add	r2, r2, #52
	mov	r3, #0
.L200:
#APP
@ 904 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r2]
@ 0 "" 2
	add	r3, r3, #1
	add	r2, r2, #4096
	cmp	r3, #8
	bne	.L200
	add	r1, r4, #32768
	movw	r0, #33192
	add	r1, r1, #56
	movt	r0, 10
	mov	r3, #0
.L202:
#APP
@ 938 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	add	r2, r1, r3
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r0, [r2]
@ 0 "" 2
	add	r3, r3, #4096
	cmp	r3, #32768
	bne	.L202
	add	r0, r4, #32768
	mov	r3, #0
	add	r0, r0, #60
.L204:
#APP
@ 943 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	movw	r2, #16448
	add	r1, r0, r3
	bfi	r2, r2, #16, #16
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r2, [r1]
@ 0 "" 2
	add	r3, r3, #4096
	cmp	r3, #32768
	bne	.L204
#APP
@ 947 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	mov	r3, #0
#APP
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4, #100]
@ 0 "" 2
@ 948 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
@ 90 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	str r3, [r4, #112]
@ 0 "" 2
.L206:
#APP
@ 107 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/io.h" 1
	ldr r0, [r7]
@ 0 "" 2
@ 950 "drivers/hidmacv300/hi_dmacv300.c" 1
	dsb
@ 0 "" 2
	ands	r0, r0, #1
	bne	.L206
	ldmfd	sp!, {r4, r5, r6, r7, r8, pc}
.L217:
	ldr	r1, .L218
	mov	r2, #480
	movw	r0, #:lower16:.LC14
	movt	r0, #:upper16:.LC14
	bl	printk
	movw	r0, #:lower16:.LC17
	movt	r0, #:upper16:.LC17
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
	b	.L182
.L219:
	.align	2
.L218:
	.word	.LANCHOR2+52
	.fnend
	.size	hi_iommu_hardware_init, .-hi_iommu_hardware_init
	.align	2
	.type	hi_iommu_probe, %function
hi_iommu_probe:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	movw	r3, #:lower16:.LANCHOR1
	movt	r3, #:upper16:.LANCHOR1
	stmfd	sp!, {r4, r5, r6, lr}
	.save {r4, r5, r6, lr}
	mov	r5, r0
	ldr	r3, [r3, #460]
	.pad #8
	sub	sp, sp, #8
	cmp	r3, #2
	ble	.L233
.L221:
	movw	r3, #:lower16:kmalloc_caches
	movt	r3, #:upper16:kmalloc_caches
	movw	r1, #32976
	ldr	r0, [r3, #24]
	bl	kmem_cache_alloc
	subs	r4, r0, #0
	beq	.L234
	mov	r0, r5
	mov	r1, #512
	mov	r2, #0
	bl	platform_get_resource
	subs	r6, r0, #0
	beq	.L235
	ldr	r1, .L239
	movw	r0, #:lower16:.LC20
	ldr	r2, [r6]
	movt	r0, #:upper16:.LC20
	bl	printk
	ldr	r0, [r6]
	mov	r1, #65536
	mov	r2, #0
	bl	__arm_ioremap
	cmp	r0, #0
	str	r0, [r4]
	beq	.L236
	mov	r1, r4
	add	r0, r5, #16
	bl	dev_set_drvdata
	mov	r0, r5
	mov	r1, #0
	bl	platform_get_irq
	subs	r3, r0, #0
	blt	.L237
	mov	r2, #0
	str	r3, [r4, #4]
	movw	r1, #:lower16:.LC23
	str	r4, [sp, #4]
	movt	r1, #:upper16:.LC23
	mov	r3, r2
	str	r1, [sp]
	movw	r1, #:lower16:hi_iommu_irq
	movt	r1, #:upper16:hi_iommu_irq
	bl	request_threaded_irq
	subs	r6, r0, #0
	bne	.L238
	movw	r5, #:lower16:.LANCHOR0
	movt	r5, #:upper16:.LANCHOR0
	str	r6, [r4, #8]
	mov	r0, r4
	str	r6, [r5, #204]
	str	r6, [r5, #208]
	bl	hi_iommu_hardware_init
	mov	r0, r6
	str	r4, [r5, #72]
.L231:
	add	sp, sp, #8
	@ sp needed
	ldmfd	sp!, {r4, r5, r6, pc}
.L233:
	ldr	r1, .L239
	movw	r2, #966
	movw	r0, #:lower16:.LC2
	movt	r0, #:upper16:.LC2
	bl	printk
	movw	r0, #:lower16:.LC3
	movt	r0, #:upper16:.LC3
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
	b	.L221
.L238:
	ldr	r1, .L239
	movw	r2, #1002
	movw	r0, #:lower16:.LC14
	movt	r0, #:upper16:.LC14
	bl	printk
	movw	r0, #:lower16:.LC24
	movt	r0, #:upper16:.LC24
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
.L225:
	ldr	r0, [r4]
	cmp	r0, #0
	beq	.L230
	bl	__arm_iounmap
.L230:
	mov	r0, r4
	bl	kfree
	mov	r0, r6
	b	.L231
.L237:
	ldr	r1, .L239
	movw	r2, #994
	movw	r0, #:lower16:.LC14
	movt	r0, #:upper16:.LC14
	bl	printk
	movw	r0, #:lower16:.LC22
	movt	r0, #:upper16:.LC22
	mvn	r6, #21
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
	b	.L225
.L236:
	ldr	r1, .L239
	movw	r2, #985
	movw	r0, #:lower16:.LC14
	movt	r0, #:upper16:.LC14
	bl	printk
	movw	r0, #:lower16:.LC21
	movt	r0, #:upper16:.LC21
	mvn	r6, #11
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
	b	.L225
.L234:
	ldr	r1, .L239
	movw	r2, #970
	movw	r0, #:lower16:.LC14
	movt	r0, #:upper16:.LC14
	bl	printk
	movw	r0, #:lower16:.LC18
	movt	r0, #:upper16:.LC18
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
	mvn	r0, #11
	b	.L231
.L235:
	ldr	r1, .L239
	movw	r2, #977
	movw	r0, #:lower16:.LC14
	movt	r0, #:upper16:.LC14
	bl	printk
	movw	r0, #:lower16:.LC19
	movt	r0, #:upper16:.LC19
	mvn	r6, #18
	bl	printk
	movw	r0, #:lower16:.LC4
	movt	r0, #:upper16:.LC4
	bl	printk
	b	.L225
.L240:
	.align	2
.L239:
	.word	.LANCHOR2+72
	.fnend
	.size	hi_iommu_probe, .-hi_iommu_probe
	.align	2
	.global	hi_iommu_hardware_exit
	.type	hi_iommu_hardware_exit, %function
hi_iommu_hardware_exit:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.fnend
	.size	hi_iommu_hardware_exit, .-hi_iommu_hardware_exit
	.global	hi_function
	.global	hi_g_channel_status
	.global	dmac_wait_queue
	.global	g_host
	.global	wake_channel_flag
	.global	irq_flag
	.global	reg_lock
	.global	my_lock
	.global	hi_reg_dmac_base_va
	.global	vdma_flag
	.global	iommu_trace_level
	.section	.rodata
	.align	2
.LANCHOR2 = . + 0
	.type	__func__.24102, %object
	.size	__func__.24102, 16
__func__.24102:
	.ascii	"hi_iommu_remove\000"
	.type	__func__.24023, %object
	.size	__func__.24023, 13
__func__.24023:
	.ascii	"hi_iommu_irq\000"
	.space	3
	.type	__func__.24114, %object
	.size	__func__.24114, 20
__func__.24114:
	.ascii	"hi_dmac_module_init\000"
	.type	__func__.23923, %object
	.size	__func__.23923, 18
__func__.23923:
	.ascii	"arm_smmu_tlb_sync\000"
	.space	2
	.type	__func__.24093, %object
	.size	__func__.24093, 15
__func__.24093:
	.ascii	"hi_iommu_probe\000"
	.space	1
	.type	__param_str_iommu_trace_level, %object
	.size	__param_str_iommu_trace_level, 30
__param_str_iommu_trace_level:
	.ascii	"hi_dmacv300.iommu_trace_level\000"
	.section	.initcall6.init,"aw",%progbits
	.align	2
	.type	__initcall_hi_dmac_module_init6, %object
	.size	__initcall_hi_dmac_module_init6, 4
__initcall_hi_dmac_module_init6:
	.word	hi_dmac_module_init
	.section	___ksymtab+vdma_flag,"a",%progbits
	.align	2
	.type	__ksymtab_vdma_flag, %object
	.size	__ksymtab_vdma_flag, 8
__ksymtab_vdma_flag:
	.word	vdma_flag
	.word	__kstrtab_vdma_flag
	.section	___ksymtab+hi_dmac_register_isr,"a",%progbits
	.align	2
	.type	__ksymtab_hi_dmac_register_isr, %object
	.size	__ksymtab_hi_dmac_register_isr, 8
__ksymtab_hi_dmac_register_isr:
	.word	hi_dmac_register_isr
	.word	__kstrtab_hi_dmac_register_isr
	.section	___ksymtab+hi_dmac_channelstart,"a",%progbits
	.align	2
	.type	__ksymtab_hi_dmac_channelstart, %object
	.size	__ksymtab_hi_dmac_channelstart, 8
__ksymtab_hi_dmac_channelstart:
	.word	hi_dmac_channelstart
	.word	__kstrtab_hi_dmac_channelstart
	.section	___ksymtab+hi_dmac_start_m2m,"a",%progbits
	.align	2
	.type	__ksymtab_hi_dmac_start_m2m, %object
	.size	__ksymtab_hi_dmac_start_m2m, 8
__ksymtab_hi_dmac_start_m2m:
	.word	hi_dmac_start_m2m
	.word	__kstrtab_hi_dmac_start_m2m
	.section	__modver,"a",%progbits
	.align	2
	.type	__modver_attr, %object
	.size	__modver_attr, 4
__modver_attr:
	.word	___modver_attr
	.section	___ksymtab+hi_dmac_wait,"a",%progbits
	.align	2
	.type	__ksymtab_hi_dmac_wait, %object
	.size	__ksymtab_hi_dmac_wait, 8
__ksymtab_hi_dmac_wait:
	.word	hi_dmac_wait
	.word	__kstrtab_hi_dmac_wait
	.section	.exitcall.exit,"aw",%progbits
	.align	2
	.type	__exitcall_hi_dmac_module_exit, %object
	.size	__exitcall_hi_dmac_module_exit, 4
__exitcall_hi_dmac_module_exit:
	.word	hi_dmac_module_exit
	.section	__ksymtab_strings,"a",%progbits
	.type	__kstrtab_hi_dmac_m2m_transfer, %object
	.size	__kstrtab_hi_dmac_m2m_transfer, 21
__kstrtab_hi_dmac_m2m_transfer:
	.ascii	"hi_dmac_m2m_transfer\000"
	.type	__kstrtab_hi_dmac_wait, %object
	.size	__kstrtab_hi_dmac_wait, 13
__kstrtab_hi_dmac_wait:
	.ascii	"hi_dmac_wait\000"
	.type	__kstrtab_hi_dmac_start_m2m, %object
	.size	__kstrtab_hi_dmac_start_m2m, 18
__kstrtab_hi_dmac_start_m2m:
	.ascii	"hi_dmac_start_m2m\000"
	.type	__kstrtab_hi_dmac_channel_allocate, %object
	.size	__kstrtab_hi_dmac_channel_allocate, 25
__kstrtab_hi_dmac_channel_allocate:
	.ascii	"hi_dmac_channel_allocate\000"
	.type	__kstrtab_hi_dmac_channelstart, %object
	.size	__kstrtab_hi_dmac_channelstart, 21
__kstrtab_hi_dmac_channelstart:
	.ascii	"hi_dmac_channelstart\000"
	.type	__kstrtab_hi_dmac_channel_free, %object
	.size	__kstrtab_hi_dmac_channel_free, 21
__kstrtab_hi_dmac_channel_free:
	.ascii	"hi_dmac_channel_free\000"
	.type	__kstrtab_hi_dmac_register_isr, %object
	.size	__kstrtab_hi_dmac_register_isr, 21
__kstrtab_hi_dmac_register_isr:
	.ascii	"hi_dmac_register_isr\000"
	.type	__kstrtab_hi_memcpy, %object
	.size	__kstrtab_hi_memcpy, 10
__kstrtab_hi_memcpy:
	.ascii	"hi_memcpy\000"
	.type	__kstrtab_vdma_flag, %object
	.size	__kstrtab_vdma_flag, 10
__kstrtab_vdma_flag:
	.ascii	"vdma_flag\000"
	.data
	.align	3
.LANCHOR1 = . + 0
	.type	hi_iommu_device, %object
	.size	hi_iommu_device, 376
hi_iommu_device:
	.word	.LC23
	.word	1
	.space	8
	.space	248
	.word	hiiommu_dmamask
	.space	4
	.word	-1
	.word	-1
	.space	80
	.word	2
	.word	hi_iommu_resources
	.space	8
	.type	hi_iommu_driver, %object
	.size	hi_iommu_driver, 84
hi_iommu_driver:
	.word	hi_iommu_probe
	.word	hi_iommu_remove
	.space	4
	.word	0
	.word	0
	.word	.LC23
	.space	56
	.space	4
	.type	iommu_trace_level, %object
	.size	iommu_trace_level, 4
iommu_trace_level:
	.word	1
	.type	___modver_attr, %object
	.size	___modver_attr, 36
___modver_attr:
	.word	.LC25
	.short	292
	.space	2
	.word	__modver_version_show
	.space	16
	.word	.LC26
	.word	.LC27
	.space	4
	.type	hiiommu_dmamask, %object
	.size	hiiommu_dmamask, 8
hiiommu_dmamask:
	.word	-1
	.word	-1
	.type	hi_iommu_resources, %object
	.size	hi_iommu_resources, 56
hi_iommu_resources:
	.word	320798720
	.word	320864255
	.space	4
	.word	512
	.space	12
	.word	94
	.word	94
	.space	4
	.word	1024
	.space	12
	.section	.rodata.str1.4,"aMS",%progbits,1
	.align	2
.LC0:
	.ascii	"channel which choosed %d is error !\012\000"
	.space	3
.LC1:
	.ascii	"dma chn %d is in used!\012\000"
.LC2:
	.ascii	"\0016%s:%d: \000"
	.space	2
.LC3:
	.ascii	"begin\000"
	.space	2
.LC4:
	.ascii	"\012\000"
	.space	2
.LC5:
	.ascii	"wake up in %s\012\000"
	.space	1
.LC6:
	.ascii	"wake up in page_fault\012\000"
	.space	1
.LC7:
	.ascii	"channel number is larger than or equal to DMAC_MAX_"
	.ascii	"CHANNELS %d\012\000"
.LC8:
	.ascii	"Config error in DMAC %d channel!\012\000"
	.space	2
.LC9:
	.ascii	"The transfer of Channel %d has timeout!\012\000"
	.space	3
.LC10:
	.ascii	"Can't remap addr:%.8x to virt.\012\000"
.LC11:
	.ascii	"Hisilicon Dmac\000"
	.space	1
.LC12:
	.ascii	"DMA Irq %d request failed\012\000"
	.space	1
.LC13:
	.ascii	"&dmac_wait_queue[i]\000"
.LC14:
	.ascii	"\0013iommu:%s:%d: \000"
.LC15:
	.ascii	"iommu device register failed!\012\000"
	.space	1
.LC16:
	.ascii	"iommu driver register failed!\012\000"
	.space	1
.LC17:
	.ascii	"TLB sync timed out -- SMMU may be deadlocked\012\000"
	.space	2
.LC18:
	.ascii	"no mem for hi iommu controller\012\000"
.LC19:
	.ascii	"no ioaddr resources config \012\000"
	.space	3
.LC20:
	.ascii	"%s iores_start:%x\012\000"
	.space	1
.LC21:
	.ascii	"no mem for iommu base!\012\000"
.LC22:
	.ascii	"no IRQ defined!\012\000"
	.space	3
.LC23:
	.ascii	"hi_iommu\000"
	.space	3
.LC24:
	.ascii	"request irq error\000"
	.space	2
.LC25:
	.ascii	"version\000"
.LC26:
	.ascii	"hi_dmacv300\000"
.LC27:
	.ascii	"HI_VERSION=HIDMACV300 @Hi3536_OSDRV 2014-07-121 9:4"
	.ascii	"5:37\000"
	.section	__param,"a",%progbits
	.align	2
	.type	__param_iommu_trace_level, %object
	.size	__param_iommu_trace_level, 16
__param_iommu_trace_level:
	.word	__param_str_iommu_trace_level
	.word	param_ops_int
	.short	384
	.short	-1
	.word	iommu_trace_level
	.section	___ksymtab+hi_dmac_channel_free,"a",%progbits
	.align	2
	.type	__ksymtab_hi_dmac_channel_free, %object
	.size	__ksymtab_hi_dmac_channel_free, 8
__ksymtab_hi_dmac_channel_free:
	.word	hi_dmac_channel_free
	.word	__kstrtab_hi_dmac_channel_free
	.bss
	.align	2
.LANCHOR0 = . + 0
	.type	hi_g_channel_status, %object
	.size	hi_g_channel_status, 32
hi_g_channel_status:
	.space	32
	.type	hi_function, %object
	.size	hi_function, 32
hi_function:
	.space	32
	.type	hi_reg_dmac_base_va, %object
	.size	hi_reg_dmac_base_va, 4
hi_reg_dmac_base_va:
	.space	4
	.type	vdma_flag, %object
	.size	vdma_flag, 4
vdma_flag:
	.space	4
	.type	g_host, %object
	.size	g_host, 4
g_host:
	.space	4
	.type	dmac_wait_queue, %object
	.size	dmac_wait_queue, 96
dmac_wait_queue:
	.space	96
	.type	wake_channel_flag, %object
	.size	wake_channel_flag, 32
wake_channel_flag:
	.space	32
	.type	my_lock, %object
	.size	my_lock, 4
my_lock:
	.space	4
	.type	__key.23886, %object
	.size	__key.23886, 0
__key.23886:
	.type	reg_lock, %object
	.size	reg_lock, 4
reg_lock:
	.space	4
	.type	irq_flag, %object
	.size	irq_flag, 4
irq_flag:
	.space	4
	.section	___ksymtab+hi_memcpy,"a",%progbits
	.align	2
	.type	__ksymtab_hi_memcpy, %object
	.size	__ksymtab_hi_memcpy, 8
__ksymtab_hi_memcpy:
	.word	hi_memcpy
	.word	__kstrtab_hi_memcpy
	.section	___ksymtab+hi_dmac_m2m_transfer,"a",%progbits
	.align	2
	.type	__ksymtab_hi_dmac_m2m_transfer, %object
	.size	__ksymtab_hi_dmac_m2m_transfer, 8
__ksymtab_hi_dmac_m2m_transfer:
	.word	hi_dmac_m2m_transfer
	.word	__kstrtab_hi_dmac_m2m_transfer
	.section	___ksymtab+hi_dmac_channel_allocate,"a",%progbits
	.align	2
	.type	__ksymtab_hi_dmac_channel_allocate, %object
	.size	__ksymtab_hi_dmac_channel_allocate, 8
__ksymtab_hi_dmac_channel_allocate:
	.word	hi_dmac_channel_allocate
	.word	__kstrtab_hi_dmac_channel_allocate
	.ident	"GCC: (Hisilicon_v300) 4.8.3 20131202 (prerelease)"
	.section	.note.GNU-stack,"",%progbits

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
	.file	"hi_dmacv300_misc.c"
	.text
	.align	2
	.type	hi_dmac_open, %function
hi_dmac_open:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.fnend
	.size	hi_dmac_open, .-hi_dmac_open
	.align	2
	.type	hi_dmac_release, %function
hi_dmac_release:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	mov	r0, #0
	bx	lr
	.fnend
	.size	hi_dmac_release, .-hi_dmac_release
	.section	.exit.text,"ax",%progbits
	.align	2
	.type	hi_dmac_exit, %function
hi_dmac_exit:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r0, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LANCHOR0
	b	misc_deregister
	.fnend
	.size	hi_dmac_exit, .-hi_dmac_exit
	.text
	.align	2
	.type	hi_dmac_ioctl, %function
hi_dmac_ioctl:
	.fnstart
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	stmfd	sp!, {r4, lr}
	.save {r4, lr}
	.pad #16
	sub	sp, sp, #16
	mov	r4, r1
	mov	r0, sp
	bic	r3, r0, #8128
	bic	r3, r3, #63
	ldr	r3, [r3, #8]
#APP
@ 438 "/home/qxjun/svn/6_hi3536/opensource/kernel/linux-3.10.y/arch/arm/include/asm/uaccess.h" 1
	adds r1, r2, #16; sbcccs r1, r1, r3; movcc r3, #0
@ 0 "" 2
	cmp	r3, #0
	bne	.L5
	mov	r1, r2
	mov	r2, #16
	bl	hi_copy_from_user
	cmp	r0, #0
	bne	.L15
	cmp	r4, #4
	bne	.L12
	ldr	r0, [sp, #4]
	ldr	r1, [sp]
	ldr	r2, [sp, #8]
	bl	hi_memcpy
.L6:
	add	sp, sp, #16
	@ sp needed
	ldmfd	sp!, {r4, pc}
.L5:
	mov	r0, sp
	mov	r1, #16
	bl	__memzero
	mvn	r0, #21
	add	sp, sp, #16
	@ sp needed
	ldmfd	sp!, {r4, pc}
.L12:
	mov	r2, #48
	movw	r0, #:lower16:.LC0
	movw	r1, #:lower16:.LANCHOR1
	movt	r0, #:upper16:.LC0
	movt	r1, #:upper16:.LANCHOR1
	bl	printk
	mov	r1, r4
	movw	r0, #:lower16:.LC1
	movt	r0, #:upper16:.LC1
	bl	printk
	movw	r0, #:lower16:.LC2
	movt	r0, #:upper16:.LC2
	bl	printk
	mvn	r0, #0
	b	.L6
.L15:
	mvn	r0, #21
	b	.L6
	.fnend
	.size	hi_dmac_ioctl, .-hi_dmac_ioctl
	.section	.init.text,"ax",%progbits
	.align	2
	.type	hi_dmac_init, %function
hi_dmac_init:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	movw	r0, #:lower16:.LANCHOR0
	movt	r0, #:upper16:.LANCHOR0
	b	misc_register
	.fnend
	.size	hi_dmac_init, .-hi_dmac_init
	.global	len
	.global	src
	.global	dest
	.section	.rodata
	.align	2
.LANCHOR1 = . + 0
	.type	__func__.20787, %object
	.size	__func__.20787, 14
__func__.20787:
	.ascii	"hi_dmac_ioctl\000"
	.space	2
	.type	hi_dmac_fops, %object
	.size	hi_dmac_fops, 108
hi_dmac_fops:
	.word	0
	.space	28
	.word	hi_dmac_ioctl
	.space	8
	.word	hi_dmac_open
	.space	4
	.word	hi_dmac_release
	.space	52
	.section	.exitcall.exit,"aw",%progbits
	.align	2
	.type	__exitcall_hi_dmac_exit, %object
	.size	__exitcall_hi_dmac_exit, 4
__exitcall_hi_dmac_exit:
	.word	hi_dmac_exit
	.section	.initcall6.init,"aw",%progbits
	.align	2
	.type	__initcall_hi_dmac_init6, %object
	.size	__initcall_hi_dmac_init6, 4
__initcall_hi_dmac_init6:
	.word	hi_dmac_init
	.data
	.align	2
.LANCHOR0 = . + 0
	.type	hi_dmac_misc_device, %object
	.size	hi_dmac_misc_device, 36
hi_dmac_misc_device:
	.word	255
	.word	.LC3
	.word	hi_dmac_fops
	.space	24
	.section	.rodata.str1.4,"aMS",%progbits,1
	.align	2
.LC0:
	.ascii	"\0013hidmac:%s:%d: \000"
	.space	3
.LC1:
	.ascii	"unknow command :%x\012\000"
.LC2:
	.ascii	"\012\000"
	.space	2
.LC3:
	.ascii	"hi_dmac\000"
	.bss
	.align	2
	.type	len, %object
	.size	len, 4
len:
	.space	4
	.type	src, %object
	.size	src, 4
src:
	.space	4
	.type	dest, %object
	.size	dest, 4
dest:
	.space	4
	.ident	"GCC: (Hisilicon_v300) 4.8.3 20131202 (prerelease)"
	.section	.note.GNU-stack,"",%progbits

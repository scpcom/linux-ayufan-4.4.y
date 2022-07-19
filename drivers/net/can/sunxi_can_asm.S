	.arch armv7-a
	.fpu softvfp
	.eabi_attribute 20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute 21, 1	@ Tag_ABI_FP_exceptions
	.eabi_attribute 23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute 24, 1	@ Tag_ABI_align8_needed
	.eabi_attribute 25, 1	@ Tag_ABI_align8_preserved
	.eabi_attribute 26, 2	@ Tag_ABI_enum_size
	.eabi_attribute 30, 4	@ Tag_ABI_optimization_goals
	.eabi_attribute 34, 1	@ Tag_CPU_unaligned_access
	.eabi_attribute 18, 2	@ Tag_ABI_PCS_wchar_t
	.file	"sunxi_can_asm.c"
@ GNU C89 (Linaro GCC 5.3-2016.05) version 5.3.1 20160412 (arm-linux-gnueabi)
@	compiled by GNU C version 4.8.4, GMP version 6.0.0, MPFR version 3.1.3, MPC version 1.0.3
@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc -I ./arch/arm/include
@ -I ./arch/arm/include/generated -I ./include -I ./arch/arm/include/uapi
@ -I ./arch/arm/include/generated/uapi -I ./include/uapi
@ -I ./include/generated/uapi -imultilib . -imultiarch arm-linux-gnueabi
@ -iprefix /home/wujiayi/T113_release/out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/../lib/gcc/arm-linux-gnueabi/5.3.1/
@ -isysroot /home/wujiayi/T113_release/out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/../arm-linux-gnueabi/libc
@ -D __KERNEL__ -D __LINUX_ARM_ARCH__=7 -U arm
@ -D KBUILD_BASENAME="sunxi_can_asm" -D KBUILD_MODNAME="sunxi_can_asm"
@ -isystem /home/wujiayi/T113_release/out/gcc-linaro-5.3.1-2016.05-x86_64_arm-linux-gnueabi/bin/../lib/gcc/arm-linux-gnueabi/5.3.1/include
@ -include ./include/linux/kconfig.h
@ -include ./include/linux/compiler_types.h
@ -MD drivers/net/can/.sunxi_can_asm.s.d drivers/net/can/sunxi_can_asm.c
@ -mlittle-endian -mabi=aapcs-linux -mfpu=vfp -marm -march=armv7-a
@ -mfloat-abi=soft -mtune=cortex-a9 -mtls-dialect=gnu
@ -auxbase-strip drivers/net/can/sunxi_can_asm.s -Os -Wall -Wundef
@ -Werror=strict-prototypes -Wno-trigraphs
@ -Werror=implicit-function-declaration -Werror=implicit-int
@ -Wno-format-security -Wframe-larger-than=2048
@ -Wno-unused-but-set-variable -Wdeclaration-after-statement -Wvla
@ -Wno-pointer-sign -Wno-array-bounds -Wno-maybe-uninitialized
@ -Werror=date-time -Werror=incompatible-pointer-types
@ -Werror=designated-init -std=gnu90 -fno-strict-aliasing -fno-common
@ -fshort-wchar -fno-PIE -fno-dwarf2-cfi-asm -fno-ipa-sra -funwind-tables
@ -fno-delete-null-pointer-checks -fstack-protector-strong
@ -fomit-frame-pointer -fno-strict-overflow -fno-merge-all-constants
@ -fmerge-constants -fstack-check=no -fconserve-stack -fverbose-asm
@ --param allow-store-data-races=0
@ options enabled:  -faggressive-loop-optimizations -falign-functions
@ -falign-jumps -falign-labels -falign-loops -fauto-inc-dec
@ -fbranch-count-reg -fcaller-saves -fchkp-check-incomplete-type
@ -fchkp-check-read -fchkp-check-write -fchkp-instrument-calls
@ -fchkp-narrow-bounds -fchkp-optimize -fchkp-store-bounds
@ -fchkp-use-static-bounds -fchkp-use-static-const-bounds
@ -fchkp-use-wrappers -fcombine-stack-adjustments -fcompare-elim
@ -fcprop-registers -fcrossjumping -fcse-follow-jumps -fdefer-pop
@ -fdevirtualize -fdevirtualize-speculatively -fearly-inlining
@ -feliminate-unused-debug-types -fexpensive-optimizations
@ -fforward-propagate -ffunction-cse -fgcse -fgcse-lm -fgnu-runtime
@ -fgnu-unique -fguess-branch-probability -fhoist-adjacent-loads -fident
@ -fif-conversion -fif-conversion2 -findirect-inlining -finline
@ -finline-atomics -finline-functions -finline-functions-called-once
@ -finline-small-functions -fipa-cp -fipa-cp-alignment -fipa-icf
@ -fipa-icf-functions -fipa-icf-variables -fipa-profile -fipa-pure-const
@ -fipa-ra -fipa-reference -fira-hoist-pressure -fira-share-save-slots
@ -fira-share-spill-slots -fisolate-erroneous-paths-dereference -fivopts
@ -fkeep-static-consts -fleading-underscore -flifetime-dse -flra-remat
@ -flto-odr-type-merging -fmath-errno -fmerge-constants
@ -fmerge-debug-strings -fmove-loop-invariants -fomit-frame-pointer
@ -foptimize-sibling-calls -fpartial-inlining -fpeephole -fpeephole2 -fplt
@ -fprefetch-loop-arrays -freg-struct-return -freorder-blocks
@ -freorder-functions -frerun-cse-after-loop
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-pressure -fsched-rank-heuristic -fsched-spec
@ -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-insns2
@ -fsection-anchors -fsemantic-interposition -fshow-column -fshrink-wrap
@ -fsigned-zeros -fsplit-ivs-in-unroller -fsplit-wide-types -fssa-phiopt
@ -fstack-protector-strong -fstdarg-opt -fstrict-volatile-bitfields
@ -fsync-libcalls -fthread-jumps -ftoplevel-reorder -ftrapping-math
@ -ftree-bit-ccp -ftree-builtin-call-dce -ftree-ccp -ftree-ch
@ -ftree-coalesce-vars -ftree-copy-prop -ftree-copyrename -ftree-cselim
@ -ftree-dce -ftree-dominator-opts -ftree-dse -ftree-forwprop -ftree-fre
@ -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
@ -ftree-loop-optimize -ftree-parallelize-loops= -ftree-phiprop -ftree-pre
@ -ftree-pta -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-slsr
@ -ftree-sra -ftree-switch-conversion -ftree-tail-merge -ftree-ter
@ -ftree-vrp -funit-at-a-time -funwind-tables -fverbose-asm
@ -fzero-initialized-in-bss -marm -mglibc -mlittle-endian
@ -mpic-data-is-text-relative -msched-prolog -munaligned-access
@ -mvectorize-with-neon-quad

	.syntax divided
	.syntax unified
	.arm
	.syntax unified
	.text
	.align	2
	.global	can_asm_write_cmdreg
	.syntax unified
	.arm
	.type	can_asm_write_cmdreg, %function
can_asm_write_cmdreg:
	.fnstart
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, lr}	@
	.save {r4, r5, lr}
	mov	r3, #9	@ tmp143,
	.pad #20
	sub	sp, sp, #20	@,,
	mov	r5, r0	@ mod_reg_val, mod_reg_val
	mov	r4, r1	@ mod_reg_addr, mod_reg_addr
	str	r3, [sp, #8]	@ tmp143, mod_reg_val
	mov	r3, #5	@ tmp144,
	str	r3, [sp, #12]	@ tmp144, mod_reg_addr
	ldr	r3, [sp, #8]	@ D.65567, mod_reg_val
	add	r2, r3, #1	@ D.65567, D.65567,
	tst	r3, #8	@ D.65567,
	str	r2, [sp, #8]	@ D.65567, mod_reg_val
	ldreq	r2, [sp, #8]	@ D.65567, mod_reg_val
	ldreq	r3, [sp, #12]	@ D.65567, mod_reg_addr
	muleq	r3, r3, r2	@ D.65567, D.65567, D.65567
	streq	r3, [sp, #12]	@ D.65567, mod_reg_addr
	movw	r3, #770	@ tmp146,
	ldr	r2, [sp, #12]	@ D.65567, mod_reg_addr
	cmp	r2, r3	@ D.65567, tmp146
	ldrne	r3, [sp, #8]	@ D.65567, mod_reg_val
	ldrne	r2, [sp, #12]	@ D.65567, mod_reg_addr
	addne	r3, r3, r2	@ D.65567, D.65567, D.65567
	strne	r3, [sp, #12]	@ D.65567, mod_reg_addr
	movw	r3, #819	@ tmp147,
	ldr	r2, [sp, #12]	@ D.65567, mod_reg_addr
	mul	r3, r3, r2	@ D.65567, tmp147, D.65567
	str	r3, [sp, #8]	@ D.65567, mod_reg_val
	ldr	r3, [sp, #8]	@ D.65567, mod_reg_val
	.syntax divided
@ 287 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r5, [r4, #4]	@ mod_reg_val, MEM[(volatile u32 *)_6]
@ 0 "" 2
	.arm
	.syntax unified
	mov	r3, #9	@ tmp148,
	str	r3, [sp]	@ tmp148, mod_reg_val
	mov	r3, #7	@ tmp149,
	str	r3, [sp, #4]	@ tmp149, mod_reg_addr
	ldr	r3, [sp]	@ D.65567, mod_reg_val
	bic	r3, r3, #356	@ D.65567, D.65567,
	bic	r3, r3, #1	@ D.65567, D.65567,
	lsl	r3, r3, #22	@ D.65567, D.65567,
	lsr	r3, r3, #22	@ D.65567, D.65567,
	str	r3, [sp, #4]	@ D.65567, mod_reg_addr
	ldr	r3, [sp]	@ D.65567, mod_reg_val
	add	r2, r3, #1	@ D.65567, D.65567,
	tst	r3, #8	@ D.65567,
	str	r2, [sp]	@ D.65567, mod_reg_val
	ldreq	r2, [sp]	@ D.65567, mod_reg_val
	ldreq	r3, [sp, #4]	@ D.65567, mod_reg_addr
	muleq	r3, r3, r2	@ D.65567, D.65567, D.65567
	streq	r3, [sp, #4]	@ D.65567, mod_reg_addr
	ldr	r2, [sp]	@ D.65567, mod_reg_val
	ldr	r3, [sp, #4]	@ D.65567, mod_reg_addr
	eor	r3, r3, r2	@ D.65567, D.65567, D.65567
	str	r3, [sp, #4]	@ D.65567, mod_reg_addr
	movw	r3, #819	@ tmp152,
	ldr	r2, [sp]	@ D.65567, mod_reg_val
	cmp	r2, r3	@ D.65567, tmp152
	ldrne	r3, [sp]	@ D.65567, mod_reg_val
	ldreq	r3, [sp, #4]	@ D.65567, mod_reg_addr
	bicne	r3, r3, #356	@ D.65567, D.65567,
	bicne	r3, r3, #1	@ D.65567, D.65567,
	lslne	r3, r3, #22	@ D.65567, D.65567,
	lsrne	r3, r3, #22	@ D.65567, D.65567,
	strne	r3, [sp, #4]	@ D.65567, mod_reg_addr
	ldrne	r3, [sp]	@ D.65567, mod_reg_val
	add	sp, sp, #20	@,,
	@ sp needed	@
	pop	{r4, r5, pc}	@
	.fnend
	.size	can_asm_write_cmdreg, .-can_asm_write_cmdreg
	.align	2
	.global	can_asm_probe
	.syntax unified
	.arm
	.type	can_asm_probe, %function
can_asm_probe:
	.fnstart
	@ args = 0, pretend = 0, frame = 120
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, r6, r7, r8, lr}	@
	.save {r4, r5, r6, r7, r8, lr}
	.pad #144
	sub	sp, sp, #144	@,,
	ldr	r4, .L28	@ tmp140,
	add	r2, sp, #144	@ tmp146,,
	mov	r8, r0	@ node, node
	mov	r5, r1	@ num, num
	ldr	r0, .L28+4	@,
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	str	r3, [sp, #140]	@ __stack_chk_guard, D.65582
	mvn	r3, #0	@ tmp142,
	str	r3, [r2, #-92]!	@ tmp142, res.a0
	mov	r3, #0	@ tmp145,
	str	r2, [sp, #16]	@ tmp146,
	mov	r2, r1	@, num
	str	r3, [sp, #20]	@ tmp145,
	str	r3, [sp, #12]	@ tmp145,
	str	r3, [sp, #8]	@ tmp145,
	str	r3, [sp, #4]	@ tmp145,
	str	r3, [sp]	@ tmp145,
	ldr	r1, .L28+8	@,
	bl	__arm_smccc_smc	@
	ldr	r6, [sp, #52]	@ res.a0, res.a0
	cmp	r6, #0	@ res.a0,
	beq	.L9	@,
.L11:
	mvn	r0, #0	@ D.65579,
	b	.L10	@
.L9:
	mov	r2, r6	@, res.a0
	ldr	r1, .L28+12	@,
	mov	r0, r8	@, node
	bl	of_get_property	@
	mov	r7, r0	@ be_addr,
	mov	r0, r8	@, node
	bl	of_irq_find_parent	@
	cmp	r0, #0	@ p
	beq	.L11	@,
	mov	r3, #3	@ tmp153,
	str	r0, [sp, #68]	@ p, oirq.np
	str	r3, [sp, #72]	@ tmp153, oirq.args_count
	mov	r3, #9	@ tmp155,
	str	r3, [sp, #44]	@ tmp155, mod_reg_val
	mov	r3, #12	@ tmp156,
	str	r3, [sp, #48]	@ tmp156, mod_reg_addr
	ldr	r3, [sp, #44]	@ D.65579, mod_reg_val
	str	r6, [sp, #76]	@ res.a0, oirq.args
	cmp	r3, #1200	@ D.65579,
	beq	.L25	@,
	ldr	r3, [sp, #44]	@ D.65579, mod_reg_val
	add	r3, r3, #1	@ D.65579, D.65579,
	str	r3, [sp, #44]	@ D.65579, mod_reg_val
	ldr	r3, [sp, #48]	@ D.65579, mod_reg_addr
	add	r3, r3, #51	@ D.65579, D.65579,
	str	r3, [sp, #48]	@ D.65579, mod_reg_addr
	movw	r3, #5334	@ tmp157,
	ldr	r2, [sp, #48]	@ D.65579, mod_reg_addr
	cmp	r2, r3	@ D.65579, tmp157
	ldrne	r3, [sp, #44]	@ D.65579, mod_reg_val
	bne	.L13	@,
.L25:
	ldr	r3, [sp, #48]	@ D.65579, mod_reg_addr
.L13:
	mov	r3, #16	@ tmp159,
	add	r5, r5, #21	@ mod_reg_val, num,
	str	r3, [sp, #36]	@ tmp159, mod_reg_val
	mov	r3, #15	@ tmp160,
	str	r3, [sp, #40]	@ tmp160, mod_reg_addr
	movw	r3, #5334	@ tmp161,
	ldr	r2, [sp, #36]	@ D.65579, mod_reg_val
	str	r5, [sp, #80]	@ mod_reg_val, oirq.args
	cmp	r2, r3	@ D.65579, tmp161
	beq	.L26	@,
	ldr	r2, [sp, #36]	@ D.65579, mod_reg_val
	ldr	r3, [sp, #40]	@ D.65579, mod_reg_addr
	cmp	r2, r3	@ D.65579, D.65579
	ldrge	r3, [sp, #36]	@ D.65579, mod_reg_val
	bge	.L16	@,
.L26:
	ldr	r3, [sp, #40]	@ D.65579, mod_reg_addr
.L16:
	mov	r5, #4	@ tmp162,
	add	r1, sp, #68	@ tmp174,,
	mov	r0, r7	@, be_addr
	str	r5, [sp, #84]	@ tmp162, oirq.args
	bl	of_irq_parse_raw	@
	mov	r3, #1	@ tmp164,
	add	r0, sp, #68	@ tmp175,,
	str	r3, [sp, #28]	@ tmp164, mod_reg_val
	movw	r3, #819	@ tmp166,
	str	r5, [sp, #32]	@ tmp162, mod_reg_addr
	ldr	r2, [sp, #28]	@ D.65579, mod_reg_val
	cmp	r2, r3	@ D.65579, tmp166
	ldreq	r3, [sp, #32]	@ D.65579, mod_reg_addr
	ldrne	r3, [sp, #28]	@ D.65579, mod_reg_val
	bl	irq_create_of_mapping	@
.L10:
	ldr	r2, [sp, #140]	@ D.65582, D.65582
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	cmp	r2, r3	@ D.65582, __stack_chk_guard
	beq	.L20	@,
	bl	__stack_chk_fail	@
.L20:
	add	sp, sp, #144	@,,
	@ sp needed	@
	pop	{r4, r5, r6, r7, r8, pc}	@
.L29:
	.align	2
.L28:
	.word	__stack_chk_guard
	.word	-1308622748
	.word	-65534
	.word	.LC0
	.fnend
	.size	can_asm_probe, .-can_asm_probe
	.align	2
	.global	can_asm_fun0
	.syntax unified
	.arm
	.type	can_asm_fun0, %function
can_asm_fun0:
	.fnstart
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}	@
	.save {r4, lr}
	.pad #48
	sub	sp, sp, #48	@,,
	ldr	r4, .L33	@ tmp112,
	add	r2, sp, #28	@ tmp117,,
	str	r2, [sp, #16]	@ tmp117,
	mov	r2, r0	@, num
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	ldr	r1, .L33+4	@,
	ldr	r0, .L33+8	@,
	str	r3, [sp, #44]	@ __stack_chk_guard, D.65589
	mov	r3, #0	@ tmp116,
	str	r3, [sp, #20]	@ tmp116,
	str	r3, [sp, #12]	@ tmp116,
	str	r3, [sp, #8]	@ tmp116,
	str	r3, [sp, #4]	@ tmp116,
	str	r3, [sp]	@ tmp116,
	bl	__arm_smccc_smc	@
	ldr	r2, [sp, #44]	@ D.65589, D.65589
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	cmp	r2, r3	@ D.65589, __stack_chk_guard
	beq	.L31	@,
	bl	__stack_chk_fail	@
.L31:
	add	sp, sp, #48	@,,
	@ sp needed	@
	pop	{r4, pc}	@
.L34:
	.align	2
.L33:
	.word	__stack_chk_guard
	.word	-65529
	.word	-1308622748
	.fnend
	.size	can_asm_fun0, .-can_asm_fun0
	.align	2
	.global	can_asm_fun1
	.syntax unified
	.arm
	.type	can_asm_fun1, %function
can_asm_fun1:
	.fnstart
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}	@
	.save {r4, lr}
	.pad #48
	sub	sp, sp, #48	@,,
	ldr	r4, .L38	@ tmp112,
	add	r2, sp, #28	@ tmp117,,
	str	r2, [sp, #16]	@ tmp117,
	mov	r2, r0	@, num
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	ldr	r1, .L38+4	@,
	ldr	r0, .L38+8	@,
	str	r3, [sp, #44]	@ __stack_chk_guard, D.65593
	mov	r3, #0	@ tmp116,
	str	r3, [sp, #20]	@ tmp116,
	str	r3, [sp, #12]	@ tmp116,
	str	r3, [sp, #8]	@ tmp116,
	str	r3, [sp, #4]	@ tmp116,
	str	r3, [sp]	@ tmp116,
	bl	__arm_smccc_smc	@
	ldr	r2, [sp, #44]	@ D.65593, D.65593
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	cmp	r2, r3	@ D.65593, __stack_chk_guard
	beq	.L36	@,
	bl	__stack_chk_fail	@
.L36:
	add	sp, sp, #48	@,,
	@ sp needed	@
	pop	{r4, pc}	@
.L39:
	.align	2
.L38:
	.word	__stack_chk_guard
	.word	-65533
	.word	-1308622748
	.fnend
	.size	can_asm_fun1, .-can_asm_fun1
	.align	2
	.global	can_asm_fun2
	.syntax unified
	.arm
	.type	can_asm_fun2, %function
can_asm_fun2:
	.fnstart
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}	@
	.save {r4, lr}
	.pad #48
	sub	sp, sp, #48	@,,
	ldr	r4, .L43	@ tmp112,
	add	r2, sp, #28	@ tmp117,,
	str	r2, [sp, #16]	@ tmp117,
	mov	r2, r0	@, num
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	ldr	r1, .L43+4	@,
	ldr	r0, .L43+8	@,
	str	r3, [sp, #44]	@ __stack_chk_guard, D.65597
	mov	r3, #0	@ tmp116,
	str	r3, [sp, #20]	@ tmp116,
	str	r3, [sp, #12]	@ tmp116,
	str	r3, [sp, #8]	@ tmp116,
	str	r3, [sp, #4]	@ tmp116,
	str	r3, [sp]	@ tmp116,
	bl	__arm_smccc_smc	@
	ldr	r2, [sp, #44]	@ D.65597, D.65597
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	cmp	r2, r3	@ D.65597, __stack_chk_guard
	beq	.L41	@,
	bl	__stack_chk_fail	@
.L41:
	add	sp, sp, #48	@,,
	@ sp needed	@
	pop	{r4, pc}	@
.L44:
	.align	2
.L43:
	.word	__stack_chk_guard
	.word	-65532
	.word	-1308622748
	.fnend
	.size	can_asm_fun2, .-can_asm_fun2
	.align	2
	.global	can_asm_fun3
	.syntax unified
	.arm
	.type	can_asm_fun3, %function
can_asm_fun3:
	.fnstart
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}	@
	.save {r4, lr}
	.pad #48
	sub	sp, sp, #48	@,,
	ldr	r4, .L48	@ tmp111,
	add	r2, sp, #28	@ tmp116,,
	str	r2, [sp, #16]	@ tmp116,
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	ldr	r1, .L48+4	@,
	ldr	r0, .L48+8	@,
	str	r3, [sp, #44]	@ __stack_chk_guard, D.65600
	mov	r3, #0	@ tmp115,
	mov	r2, r3	@,
	str	r3, [sp, #20]	@ tmp115,
	str	r3, [sp, #12]	@ tmp115,
	str	r3, [sp, #8]	@ tmp115,
	str	r3, [sp, #4]	@ tmp115,
	str	r3, [sp]	@ tmp115,
	bl	__arm_smccc_smc	@
	ldr	r2, [sp, #44]	@ D.65600, D.65600
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	cmp	r2, r3	@ D.65600, __stack_chk_guard
	beq	.L46	@,
	bl	__stack_chk_fail	@
.L46:
	add	sp, sp, #48	@,,
	@ sp needed	@
	pop	{r4, pc}	@
.L49:
	.align	2
.L48:
	.word	__stack_chk_guard
	.word	-65531
	.word	-1308622748
	.fnend
	.size	can_asm_fun3, .-can_asm_fun3
	.align	2
	.global	can_asm_fun4
	.syntax unified
	.arm
	.type	can_asm_fun4, %function
can_asm_fun4:
	.fnstart
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}	@
	.save {r4, lr}
	.pad #48
	sub	sp, sp, #48	@,,
	ldr	r4, .L53	@ tmp111,
	add	r2, sp, #28	@ tmp116,,
	str	r2, [sp, #16]	@ tmp116,
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	ldr	r1, .L53+4	@,
	ldr	r0, .L53+8	@,
	str	r3, [sp, #44]	@ __stack_chk_guard, D.65603
	mov	r3, #0	@ tmp115,
	mov	r2, r3	@,
	str	r3, [sp, #20]	@ tmp115,
	str	r3, [sp, #12]	@ tmp115,
	str	r3, [sp, #8]	@ tmp115,
	str	r3, [sp, #4]	@ tmp115,
	str	r3, [sp]	@ tmp115,
	bl	__arm_smccc_smc	@
	ldr	r2, [sp, #44]	@ D.65603, D.65603
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	cmp	r2, r3	@ D.65603, __stack_chk_guard
	beq	.L51	@,
	bl	__stack_chk_fail	@
.L51:
	add	sp, sp, #48	@,,
	@ sp needed	@
	pop	{r4, pc}	@
.L54:
	.align	2
.L53:
	.word	__stack_chk_guard
	.word	-65530
	.word	-1308622748
	.fnend
	.size	can_asm_fun4, .-can_asm_fun4
	.align	2
	.global	can_asm_fun5
	.syntax unified
	.arm
	.type	can_asm_fun5, %function
can_asm_fun5:
	.fnstart
	@ args = 0, pretend = 0, frame = 24
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, lr}	@
	.save {r4, lr}
	.pad #48
	sub	sp, sp, #48	@,,
	ldr	r4, .L58	@ tmp116,
	add	r2, sp, #28	@ tmp121,,
	str	r2, [sp, #16]	@ tmp121,
	mov	r2, r0	@, num
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	ldr	r1, .L58+4	@,
	ldr	r0, .L58+8	@,
	str	r3, [sp, #44]	@ __stack_chk_guard, D.65608
	mov	r3, #0	@ tmp120,
	str	r3, [sp, #20]	@ tmp120,
	str	r3, [sp, #12]	@ tmp120,
	str	r3, [sp, #8]	@ tmp120,
	str	r3, [sp, #4]	@ tmp120,
	str	r3, [sp]	@ tmp120,
	bl	__arm_smccc_smc	@
	ldr	r1, [sp, #36]	@, res.a2
	ldr	r0, [sp, #32]	@, res.a1
	bl	ioremap	@
	ldr	r2, [sp, #44]	@ D.65608, D.65608
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	cmp	r2, r3	@ D.65608, __stack_chk_guard
	beq	.L56	@,
	bl	__stack_chk_fail	@
.L56:
	add	sp, sp, #48	@,,
	@ sp needed	@
	pop	{r4, pc}	@
.L59:
	.align	2
.L58:
	.word	__stack_chk_guard
	.word	-65528
	.word	-1308622748
	.fnend
	.size	can_asm_fun5, .-can_asm_fun5
	.align	2
	.global	can_asm_start
	.syntax unified
	.arm
	.type	can_asm_start, %function
can_asm_start:
	.fnstart
	@ args = 0, pretend = 0, frame = 72
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, lr}	@
	.save {r4, r5, lr}
	mov	r3, r1	@ ctrlmode, ctrlmode
	ldr	r4, .L74	@ tmp170,
	.pad #100
	sub	sp, sp, #100	@,,
	mov	r5, r0	@ mod_reg_addr, mod_reg_addr
	ldr	r1, [r4]	@ __stack_chk_guard, __stack_chk_guard
	str	r1, [sp, #92]	@ __stack_chk_guard, D.65633
	mov	r1, #10	@ tmp172,
	str	r1, [sp, #28]	@ tmp172, mod_reg_val
	mov	r1, #6	@ tmp173,
	str	r1, [sp, #32]	@ tmp173, mod_reg_addr
	ldr	r1, [sp, #28]	@ D.65629, mod_reg_val
	bic	r1, r1, #356	@ D.65629, D.65629,
	bic	r1, r1, #1	@ D.65629, D.65629,
	lsl	r1, r1, #22	@ D.65629, D.65629,
	lsr	r1, r1, #22	@ D.65629, D.65629,
	str	r1, [sp, #32]	@ D.65629, mod_reg_addr
	ldr	r0, [sp, #28]	@ D.65629, mod_reg_val
	ldr	r1, [sp, #32]	@ D.65629, mod_reg_addr
	cmp	r0, r1	@ D.65629, D.65629
	ldrlt	r0, [sp, #28]	@ D.65629, mod_reg_val
	ldrlt	r1, [sp, #32]	@ D.65629, mod_reg_addr
	addlt	r0, r0, r0, lsl #1	@ D.65629, D.65629, D.65629,
	addlt	r0, r0, r1	@ D.65629, D.65629, D.65629
	strlt	r0, [sp, #32]	@ D.65629, mod_reg_addr
	ldr	r0, [sp, #28]	@ D.65629, mod_reg_val
	ldr	r1, [sp, #32]	@ D.65629, mod_reg_addr
	cmp	r0, r1	@ D.65629, D.65629
	ldrlt	r0, [sp, #28]	@ D.65629, mod_reg_val
	ldrlt	r1, [sp, #32]	@ D.65629, mod_reg_addr
	addlt	r0, r0, r0, lsl #1	@ D.65629, D.65629, D.65629,
	addlt	r0, r0, r1	@ D.65629, D.65629, D.65629
	strlt	r0, [sp, #32]	@ D.65629, mod_reg_addr
	ldr	r1, [sp, #28]	@ D.65629, mod_reg_val
	add	r0, r1, #1	@ D.65629, D.65629,
	tst	r1, #8	@ D.65629,
	str	r0, [sp, #28]	@ D.65629, mod_reg_val
	ldreq	r1, [sp, #28]	@ D.65629, mod_reg_val
	ldreq	r0, [sp, #32]	@ D.65629, mod_reg_addr
	muleq	r0, r0, r1	@ D.65629, D.65629, D.65629
	streq	r0, [sp, #32]	@ D.65629, mod_reg_addr
	mov	r0, #0	@ tmp184,
	ldr	r1, [sp, #28]	@ D.65629, mod_reg_val
	add	r1, sp, #76	@ tmp185,,
	str	r0, [sp, #20]	@ tmp184,
	str	r1, [sp, #16]	@ tmp185,
	str	r0, [sp, #12]	@ tmp184,
	str	r0, [sp, #8]	@ tmp184,
	str	r0, [sp, #4]	@ tmp184,
	str	r0, [sp]	@ tmp184,
	ldr	r1, .L74+4	@,
	ldr	r0, .L74+8	@,
	bl	__arm_smccc_smc	@
	ldr	r3, [sp, #76]	@ res.a0, res.a0
	cmp	r3, #0	@ res.a0,
	bne	.L64	@,
	mov	r3, #8	@ tmp191,
	str	r3, [sp, #44]	@ tmp191, mod_reg_val
	mov	r3, #3	@ tmp192,
	str	r3, [sp, #48]	@ tmp192, mod_reg_addr
	ldr	r3, [sp, #44]	@ D.65629, mod_reg_val
	cmp	r3, #1200	@ D.65629,
	ldrne	r3, [sp, #44]	@ D.65629, mod_reg_val
	ldreq	r3, [sp, #48]	@ D.65629, mod_reg_addr
	andne	r3, r3, #243	@ D.65629, D.65629,
	strne	r3, [sp, #48]	@ D.65629, mod_reg_addr
	ldrne	r3, [sp, #44]	@ D.65629, mod_reg_val
	mov	r3, #4	@ tmp193,
	str	r3, [sp, #36]	@ tmp193, mod_reg_val
	str	r3, [sp, #40]	@ tmp193, mod_reg_addr
	ldr	r3, [sp, #40]	@ D.65629, mod_reg_addr
	str	r3, [sp, #36]	@ D.65629, mod_reg_val
	ldr	r3, [sp, #40]	@ D.65629, mod_reg_addr
	str	r3, [sp, #36]	@ D.65629, mod_reg_val
	ldr	r2, [sp, #36]	@ D.65629, mod_reg_val
	ldr	r3, [sp, #40]	@ D.65629, mod_reg_addr
	cmp	r2, r3	@ D.65629, D.65629
	ldrge	r3, [sp, #36]	@ D.65629, mod_reg_val
	ldrlt	r3, [sp, #40]	@ D.65629, mod_reg_addr
	andge	r3, r3, #243	@ D.65629, D.65629,
	strge	r3, [sp, #40]	@ D.65629, mod_reg_addr
	ldrge	r3, [sp, #36]	@ D.65629, mod_reg_val
	b	.L60	@
.L64:
	mov	r3, #1200	@ tmp195,
	str	r3, [sp, #52]	@ tmp195, mod_reg_val
	mov	r3, #4800	@ tmp196,
	str	r3, [sp, #56]	@ tmp196, mod_reg_addr
	ldr	r3, [sp, #56]	@ D.65629, mod_reg_addr
	add	r3, r3, #100	@ D.65629, D.65629,
	str	r3, [sp, #56]	@ D.65629, mod_reg_addr
	ldr	r3, [sp, #56]	@ D.65629, mod_reg_addr
	sub	r3, r3, #100	@ D.65629, D.65629,
	str	r3, [sp, #56]	@ D.65629, mod_reg_addr
	ldr	r3, [sp, #52]	@ D.65629, mod_reg_val
	ldr	r2, [sp, #56]	@ D.65629, mod_reg_addr
	add	r3, r3, r2	@ D.65629, D.65629, D.65629
	cmp	r3, #384	@ D.65629,
	bhi	.L73	@,
	movw	r3, #375	@ tmp199,
	str	r3, [sp, #60]	@ tmp199, mod_reg_val
	movw	r3, #5625	@ tmp200,
	str	r3, [sp, #64]	@ tmp200, mod_reg_addr
	ldr	r3, [sp, #60]	@ D.65629, mod_reg_val
	ldr	r2, [sp, #64]	@ D.65629, mod_reg_addr
	add	r3, r3, r2	@ D.65629, D.65629, D.65629
	cmp	r3, #384	@ D.65629,
	bcs	.L70	@,
.L73:
	.syntax divided
@ 384 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	mov	r3, #48	@ tmp202,
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r3, [r5, #16]	@ tmp202,
@ 0 "" 2
	.arm
	.syntax unified
.L70:
	mov	r3, #8	@ tmp203,
	str	r3, [sp, #68]	@ tmp203, mod_reg_val
	mov	r3, #6	@ tmp204,
	str	r3, [sp, #72]	@ tmp204, mod_reg_addr
	ldr	r2, [sp, #68]	@ D.65629, mod_reg_val
	ldr	r3, [sp, #72]	@ D.65629, mod_reg_addr
	eor	r3, r3, r2	@ D.65629, D.65629, D.65629
	str	r3, [sp, #72]	@ D.65629, mod_reg_addr
	ldr	r2, [sp, #68]	@ D.65629, mod_reg_val
	ldr	r3, [sp, #72]	@ D.65629, mod_reg_addr
	eor	r3, r3, r2	@ D.65629, D.65629, D.65629
	str	r3, [sp, #72]	@ D.65629, mod_reg_addr
	ldr	r3, [sp, #68]	@ D.65629, mod_reg_val
.L60:
	ldr	r2, [sp, #92]	@ D.65633, D.65633
	ldr	r3, [r4]	@ __stack_chk_guard, __stack_chk_guard
	cmp	r2, r3	@ D.65633, __stack_chk_guard
	beq	.L71	@,
	bl	__stack_chk_fail	@
.L71:
	add	sp, sp, #100	@,,
	@ sp needed	@
	pop	{r4, r5, pc}	@
.L75:
	.align	2
.L74:
	.word	__stack_chk_guard
	.word	-65535
	.word	-1308622748
	.fnend
	.size	can_asm_start, .-can_asm_start
	.align	2
	.global	can_asm_set_bittiming
	.syntax unified
	.arm
	.type	can_asm_set_bittiming, %function
can_asm_set_bittiming:
	.fnstart
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r4, r5, lr}	@
	.save {r4, r5, lr}
	mov	r3, #9	@ tmp132,
	.pad #20
	sub	sp, sp, #20	@,,
	mov	r4, r2	@ cfg, cfg
	mov	r5, r0	@ mod_reg_addr, mod_reg_addr
	str	r3, [sp]	@ tmp132, mod_reg_val
	mov	r3, #5	@ tmp133,
	str	r3, [sp, #4]	@ tmp133, mod_reg_addr
	ldr	r3, [sp]	@ D.65642, mod_reg_val
	cmp	r3, #1200	@ D.65642,
	ldrne	r2, [sp, #4]	@ D.65642, mod_reg_addr
	ldrne	r3, [sp]	@ D.65642, mod_reg_val
	ldreq	r3, [sp, #4]	@ D.65642, mod_reg_addr
	eorne	r3, r3, r2	@ D.65642, D.65642, D.65642
	strne	r3, [sp]	@ D.65642, mod_reg_val
	ldrne	r3, [sp]	@ D.65642, mod_reg_val
	tst	r1, #4	@ mod_reg_val,
	ldrne	r3, [r4]	@ *cfg_6(D), *cfg_6(D)
	orrne	r3, r3, #8388608	@ D.65643, *cfg_6(D),
	strne	r3, [r4]	@ D.65643, *cfg_6(D)
	mov	r3, #14	@ tmp137,
	str	r3, [sp, #8]	@ tmp137, mod_reg_val
	mov	r3, #15	@ tmp138,
	str	r3, [sp, #12]	@ tmp138, mod_reg_addr
	ldr	r3, [sp, #12]	@ D.65642, mod_reg_addr
	cmp	r3, #1200	@ D.65642,
	ldr	r3, [sp, #8]	@ D.65642, mod_reg_val
	beq	.L81	@,
	and	r3, r3, #243	@ D.65642, D.65642,
	str	r3, [sp, #12]	@ D.65642, mod_reg_addr
	ldr	r3, [sp, #8]	@ D.65642, mod_reg_val
	cmp	r3, #1200	@ D.65642,
	ldreq	r3, [sp, #12]	@ D.65642, mod_reg_addr
	ldrne	r3, [sp, #8]	@ D.65642, mod_reg_val
.L81:
	.syntax divided
@ 396 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	ldr	r3, [r4]	@ D.65643, *cfg_6(D)
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r3, [r5, #20]	@ D.65643, MEM[(volatile u32 *)_15]
@ 0 "" 2
	.arm
	.syntax unified
	add	sp, sp, #20	@,,
	@ sp needed	@
	pop	{r4, r5, pc}	@
	.fnend
	.size	can_asm_set_bittiming, .-can_asm_set_bittiming
	.align	2
	.global	can_asm_clean_transfer_err
	.syntax unified
	.arm
	.type	can_asm_clean_transfer_err, %function
can_asm_clean_transfer_err:
	.fnstart
	@ args = 0, pretend = 0, frame = 16
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	.pad #16
	sub	sp, sp, #16	@,,
	mov	r3, #9	@ tmp150,
	str	r3, [sp, #8]	@ tmp150, mod_reg_val
	mov	r3, #2	@ tmp151,
	str	r3, [sp, #12]	@ tmp151, mod_reg_addr
	movw	r3, #770	@ tmp152,
	ldr	ip, [sp, #12]	@ D.65654, mod_reg_addr
	cmp	ip, r3	@ D.65654, tmp152
	ldrne	r3, [sp, #8]	@ D.65654, mod_reg_val
	ldrne	ip, [sp, #12]	@ D.65654, mod_reg_addr
	addne	r3, r3, ip	@ D.65654, D.65654, D.65654
	movw	ip, #5334	@ tmp153,
	strne	r3, [sp, #12]	@ D.65654, mod_reg_addr
	ldr	r3, [sp, #8]	@ D.65654, mod_reg_val
	add	r3, r3, #1	@ D.65654, D.65654,
	str	r3, [sp, #8]	@ D.65654, mod_reg_val
	ldr	r3, [sp, #12]	@ D.65654, mod_reg_addr
	add	r3, r3, #51	@ D.65654, D.65654,
	str	r3, [sp, #12]	@ D.65654, mod_reg_addr
	ldr	r3, [sp, #12]	@ D.65654, mod_reg_addr
	cmp	r3, ip	@ D.65654, tmp153
	beq	.L96	@,
	ldr	r3, [sp, #8]	@ D.65654, mod_reg_val
	add	r3, r3, #1	@ D.65654, D.65654,
	str	r3, [sp, #8]	@ D.65654, mod_reg_val
	ldr	r3, [sp, #12]	@ D.65654, mod_reg_addr
	add	r3, r3, #51	@ D.65654, D.65654,
	str	r3, [sp, #12]	@ D.65654, mod_reg_addr
	ldr	r3, [sp, #12]	@ D.65654, mod_reg_addr
	cmp	r3, ip	@ D.65654, tmp153
	ldrne	r3, [sp, #8]	@ D.65654, mod_reg_val
	bne	.L90	@,
.L96:
	ldr	r3, [sp, #12]	@ D.65654, mod_reg_addr
.L90:
	.syntax divided
@ 113 "./arch/arm/include/asm/io.h" 1
	ldr r0, [r0, #28]	@ errors, MEM[(volatile u32 *)_4]
@ 0 "" 2
@ 404 "drivers/net/can/sunxi_can_asm.c" 1
	dsb 
@ 0 "" 2
	.arm
	.syntax unified
	mov	r3, #14	@ tmp156,
	str	r3, [sp]	@ tmp156, mod_reg_val
	mov	r3, #13	@ tmp157,
	str	r3, [sp, #4]	@ tmp157, mod_reg_addr
	ldr	ip, [sp]	@ D.65654, mod_reg_val
	ldr	r3, [sp, #4]	@ D.65654, mod_reg_addr
	cmp	ip, r3	@ D.65654, D.65654
	ldrlt	r3, [sp]	@ D.65654, mod_reg_val
	ldrlt	ip, [sp, #4]	@ D.65654, mod_reg_addr
	addlt	r3, r3, r3, lsl #1	@ D.65654, D.65654, D.65654,
	addlt	r3, r3, ip	@ D.65654, D.65654, D.65654
	strlt	r3, [sp, #4]	@ D.65654, mod_reg_addr
	ldr	r3, [sp, #4]	@ D.65654, mod_reg_addr
	str	r3, [sp]	@ D.65654, mod_reg_val
	ldr	r3, [sp, #4]	@ D.65654, mod_reg_addr
	str	r3, [sp]	@ D.65654, mod_reg_val
	ldr	r3, [sp]	@ D.65654, mod_reg_val
	add	r3, r3, #1	@ D.65654, D.65654,
	str	r3, [sp]	@ D.65654, mod_reg_val
	ldr	r3, [sp]	@ D.65654, mod_reg_val
	cmp	r3, #1200	@ D.65654,
	ldreq	r3, [sp, #4]	@ D.65654, mod_reg_addr
	ldrne	r3, [sp]	@ D.65654, mod_reg_val
	uxtb	r3, r0	@ errors, errors
	ubfx	r0, r0, #16, #8	@ D.65656, errors,,
	strh	r3, [r1]	@ movhi	@ errors, *t_err_9(D)
	strh	r0, [r2]	@ movhi	@ D.65656, *r_err_14(D)
	add	sp, sp, #16	@,,
	@ sp needed	@
	bx	lr	@
	.fnend
	.size	can_asm_clean_transfer_err, .-can_asm_clean_transfer_err
	.align	2
	.global	can_asm_start_xmit
	.syntax unified
	.arm
	.type	can_asm_start_xmit, %function
can_asm_start_xmit:
	.fnstart
	@ args = 8, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r0, r1, r2, r4, r5, r6, r7, r8, r9, lr}	@
	.save {r4, r5, r6, r7, r8, r9, lr}
	.pad #12
	tst	r2, #1073741824	@ id,
	ldr	r8, [r3]	@ dlc, *flag_6(D)
	mov	r6, r3	@ flag, flag
	mov	r5, r0	@ mod_reg_addr, mod_reg_addr
	mov	r4, r2	@ id, id
	ldr	r7, [sp, #40]	@ mod_reg_val, mod_reg_val
	orrne	r3, r8, #64	@ D.65676, dlc,
	ldr	r9, [sp, #44]	@ pdata, pdata
	strne	r3, [r6]	@ D.65676, *flag_6(D)
	cmp	r2, #0	@ id,
	bge	.L99	@,
	ldr	r3, [r6]	@ *flag_6(D), *flag_6(D)
	orr	r3, r3, #128	@ D.65676, *flag_6(D),
	str	r3, [r6]	@ D.65676, *flag_6(D)
	mov	r3, #84	@ tmp209,
	str	r3, [r7]	@ tmp209, *mod_reg_val_17(D)
	.syntax divided
@ 426 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	ubfx	r3, r4, #21, #8	@ D.65674, id,,
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r3, [r5, #68]	@ D.65674, MEM[(volatile u32 *)_24]
@ 0 "" 2
	.arm
	.syntax unified
	mov	r3, #9	@ tmp211,
	str	r3, [sp]	@ tmp211, mod_reg_val
	mov	r3, #8	@ tmp212,
	str	r3, [sp, #4]	@ tmp212, mod_reg_addr
	movw	r3, #819	@ tmp213,
	ldr	r2, [sp]	@ D.65675, mod_reg_val
	cmp	r2, r3	@ D.65675, tmp213
	beq	.L114	@,
	ldr	r3, [sp]	@ D.65675, mod_reg_val
	add	r2, r3, #1	@ D.65675, D.65675,
	tst	r3, #8	@ D.65675,
	str	r2, [sp]	@ D.65675, mod_reg_val
	ldreq	r2, [sp]	@ D.65675, mod_reg_val
	ldreq	r3, [sp, #4]	@ D.65675, mod_reg_addr
	muleq	r3, r3, r2	@ D.65675, D.65675, D.65675
	streq	r3, [sp, #4]	@ D.65675, mod_reg_addr
	ldr	r3, [sp]	@ D.65675, mod_reg_val
	add	r3, r3, #1	@ D.65675, D.65675,
	str	r3, [sp]	@ D.65675, mod_reg_val
	ldr	r3, [sp, #4]	@ D.65675, mod_reg_addr
	add	r3, r3, #51	@ D.65675, D.65675,
	str	r3, [sp, #4]	@ D.65675, mod_reg_addr
	movw	r3, #5334	@ tmp215,
	ldr	r2, [sp, #4]	@ D.65675, mod_reg_addr
	cmp	r2, r3	@ D.65675, tmp215
	beq	.L114	@,
	ldr	r2, [sp]	@ D.65675, mod_reg_val
	cmp	r2, r3	@ D.65675, tmp215
	beq	.L114	@,
	ldr	r2, [sp]	@ D.65675, mod_reg_val
	add	r2, r2, #1	@ D.65675, D.65675,
	str	r2, [sp]	@ D.65675, mod_reg_val
	ldr	r2, [sp, #4]	@ D.65675, mod_reg_addr
	add	r2, r2, #51	@ D.65675, D.65675,
	str	r2, [sp, #4]	@ D.65675, mod_reg_addr
	ldr	r2, [sp, #4]	@ D.65675, mod_reg_addr
	cmp	r2, r3	@ D.65675, tmp215
	ldrne	r3, [sp]	@ D.65675, mod_reg_val
	bne	.L101	@,
.L114:
	ldr	r3, [sp, #4]	@ D.65675, mod_reg_addr
.L101:
	.syntax divided
@ 428 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	ubfx	r3, r4, #13, #8	@ D.65674, id,,
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r3, [r5, #72]	@ D.65674, MEM[(volatile u32 *)_30]
@ 0 "" 2
@ 429 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	ubfx	r3, r4, #5, #8	@ D.65674, id,,
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r3, [r5, #76]	@ D.65674, MEM[(volatile u32 *)_35]
@ 0 "" 2
@ 430 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	lsl	r4, r4, #3	@ D.65674, id,
	bl	arm_heavy_mb	@
	uxtb	r4, r4	@ D.65674, D.65674
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r4, [r5, #80]	@ D.65674, MEM[(volatile u32 *)_40]
@ 0 "" 2
	.arm
	.syntax unified
	b	.L106	@
.L99:
	mov	r3, #76	@ tmp221,
	str	r3, [r7]	@ tmp221, *mod_reg_val_17(D)
	.syntax divided
@ 433 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	ubfx	r3, r4, #3, #8	@ D.65674, id,,
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r3, [r5, #68]	@ D.65674, MEM[(volatile u32 *)_46]
@ 0 "" 2
@ 434 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	lsl	r4, r4, #5	@ D.65674, id,
	bl	arm_heavy_mb	@
	uxtb	r4, r4	@ D.65674, D.65674
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r4, [r5, #72]	@ D.65674, MEM[(volatile u32 *)_51]
@ 0 "" 2
	.arm
	.syntax unified
.L106:
	mov	r4, #0	@ i,
.L107:
	cmp	r4, r8	@ i, dlc
	beq	.L115	@,
	.syntax divided
@ 438 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	ldr	r3, [r7]	@ *mod_reg_val_17(D), *mod_reg_val_17(D)
	ldrb	r2, [r9, r4]	@ zero_extendqisi2	@ D.65674, MEM[base: pdata_55(D), index: _92, offset: 0B]
	add	r3, r3, r4, lsl #2	@ D.65674, *mod_reg_val_17(D), i,
	add	r3, r5, r3	@ D.65678, mod_reg_addr, D.65674
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r2, [r3]	@ D.65674, MEM[(volatile u32 *)_63]
@ 0 "" 2
	.arm
	.syntax unified
	add	r4, r4, #1	@ i, i,
	b	.L107	@
.L115:
	.syntax divided
@ 440 "drivers/net/can/sunxi_can_asm.c" 1
	dsb st
@ 0 "" 2
	.arm
	.syntax unified
	bl	arm_heavy_mb	@
	ldr	r3, [r6]	@ D.65674, *flag_6(D)
	.syntax divided
@ 95 "./arch/arm/include/asm/io.h" 1
	str r3, [r5, #64]	@ D.65674, MEM[(volatile u32 *)_68]
@ 0 "" 2
	.arm
	.syntax unified
	add	sp, sp, #12	@,,
	@ sp needed	@
	pop	{r4, r5, r6, r7, r8, r9, pc}	@
	.fnend
	.size	can_asm_start_xmit, .-can_asm_start_xmit
	.align	2
	.global	can_asm_rx
	.syntax unified
	.arm
	.type	can_asm_rx, %function
can_asm_rx:
	.fnstart
	@ args = 0, pretend = 0, frame = 8
	@ frame_needed = 0, uses_anonymous_args = 0
	push	{r0, r1, r4, lr}	@
	.save {r4, lr}
	.pad #8
	.syntax divided
@ 113 "./arch/arm/include/asm/io.h" 1
	ldr lr, [r0, #64]	@ __v, MEM[(volatile u32 *)_3]
@ 0 "" 2
@ 446 "drivers/net/can/sunxi_can_asm.c" 1
	dsb 
@ 0 "" 2
	.arm
	.syntax unified
	tst	lr, #128	@ __v,
	uxtb	ip, lr	@ fi, __v
	beq	.L117	@,
	mov	lr, #84	@ tmp162,
	str	lr, [r2]	@ tmp162, *mod_reg_val_9(D)
	mov	r2, #1	@ tmp163,
	str	r2, [sp]	@ tmp163, mod_reg_val
	mov	r2, #16	@ tmp164,
	str	r2, [sp, #4]	@ tmp164, mod_reg_addr
	movw	r2, #770	@ tmp165,
	ldr	lr, [sp, #4]	@ D.65689, mod_reg_addr
	cmp	lr, r2	@ D.65689, tmp165
	ldrne	r2, [sp]	@ D.65689, mod_reg_val
	ldrne	lr, [sp, #4]	@ D.65689, mod_reg_addr
	addne	r2, r2, lr	@ D.65689, D.65689, D.65689
	strne	r2, [sp, #4]	@ D.65689, mod_reg_addr
	movw	r2, #5334	@ tmp166,
	ldr	lr, [sp]	@ D.65689, mod_reg_val
	cmp	lr, r2	@ D.65689, tmp166
	beq	.L125	@,
	ldr	lr, [sp]	@ D.65689, mod_reg_val
	movw	r2, #819	@ tmp167,
	cmp	lr, r2	@ D.65689, tmp167
	bne	.L121	@,
.L125:
	ldr	r2, [sp, #4]	@ D.65689, mod_reg_addr
	b	.L120	@
.L121:
	ldr	lr, [sp]	@ D.65689, mod_reg_val
	ldr	r2, [sp, #4]	@ D.65689, mod_reg_addr
	cmp	lr, r2	@ D.65689, D.65689
	ldrlt	r2, [sp]	@ D.65689, mod_reg_val
	ldrlt	lr, [sp, #4]	@ D.65689, mod_reg_addr
	addlt	r2, r2, r2, lsl #1	@ D.65689, D.65689, D.65689,
	addlt	r2, r2, lr	@ D.65689, D.65689, D.65689
	strlt	r2, [sp, #4]	@ D.65689, mod_reg_addr
	ldr	r2, [sp]	@ D.65689, mod_reg_val
	and	r2, r2, #243	@ D.65689, D.65689,
	str	r2, [sp, #4]	@ D.65689, mod_reg_addr
	ldr	r2, [sp]	@ D.65689, mod_reg_val
.L120:
	.syntax divided
@ 113 "./arch/arm/include/asm/io.h" 1
	ldr r4, [r0, #68]	@ __v, MEM[(volatile u32 *)_12]
@ 0 "" 2
@ 450 "drivers/net/can/sunxi_can_asm.c" 1
	dsb 
@ 0 "" 2
@ 113 "./arch/arm/include/asm/io.h" 1
	ldr r2, [r0, #72]	@ __v, MEM[(volatile u32 *)_16]
@ 0 "" 2
@ 451 "drivers/net/can/sunxi_can_asm.c" 1
	dsb 
@ 0 "" 2
@ 113 "./arch/arm/include/asm/io.h" 1
	ldr lr, [r0, #76]	@ __v, MEM[(volatile u32 *)_21]
@ 0 "" 2
@ 452 "drivers/net/can/sunxi_can_asm.c" 1
	dsb 
@ 0 "" 2
@ 113 "./arch/arm/include/asm/io.h" 1
	ldr r0, [r0, #80]	@ __v, MEM[(volatile u32 *)_26]
@ 0 "" 2
@ 453 "drivers/net/can/sunxi_can_asm.c" 1
	dsb 
@ 0 "" 2
	.arm
	.syntax unified
	lsl	r2, r2, #13	@ D.65688, __v,
	ubfx	r0, r0, #3, #5	@ D.65688, __v,,
	orr	r2, r2, r4, lsl #21	@ D.65688, D.65688, __v,
	orr	r2, r2, #-2147483648	@ D.65688, D.65688,
	orr	r2, r2, lr, lsl #5	@ D.65688, D.65688, __v,
	orr	r2, r2, r0	@ D.65688, D.65688, D.65688
	b	.L126	@
.L117:
	mov	lr, #76	@ tmp184,
	str	lr, [r2]	@ tmp184, *mod_reg_val_9(D)
	.syntax divided
@ 113 "./arch/arm/include/asm/io.h" 1
	ldr lr, [r0, #68]	@ __v, MEM[(volatile u32 *)_37]
@ 0 "" 2
@ 457 "drivers/net/can/sunxi_can_asm.c" 1
	dsb 
@ 0 "" 2
@ 113 "./arch/arm/include/asm/io.h" 1
	ldr r2, [r0, #72]	@ __v, MEM[(volatile u32 *)_40]
@ 0 "" 2
@ 458 "drivers/net/can/sunxi_can_asm.c" 1
	dsb 
@ 0 "" 2
	.arm
	.syntax unified
	ubfx	r2, r2, #5, #3	@ D.65688, __v,,
	orr	r2, r2, lr, lsl #3	@ D.65688, D.65688, __v,
.L126:
	str	r2, [r3]	@ D.65688, *id_32(D)
	strb	ip, [r1]	@ fi, *r_fi_47(D)
	add	sp, sp, #8	@,,
	@ sp needed	@
	pop	{r4, pc}	@
	.fnend
	.size	can_asm_rx, .-can_asm_rx
	.section	.rodata.str1.1,"aMS",%progbits,1
.LC0:
	.ascii	"reg\000"
	.ident	"GCC: (Linaro GCC 5.3-2016.05) 5.3.1 20160412"
	.section	.note.GNU-stack,"",%progbits

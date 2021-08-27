/*
 * include/asm-arm/arch-aurora/vmalloc.h
 */

/* Dove LCD driver performs big allocations for FrameBuffer memory, we need to
 * move CONSISTENT_BASE by 32MB
 */
/* Was 0x2000000 */

#define VMALLOC_END	(0xfa800000)

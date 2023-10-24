/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _ISR_H_
#define _ISR_H_

#include "types.h"

#define LO_EXPVECT_BASE 0x00000000
#define HI_EXPVECT_BASE 0xFFFF0000

#define RESETVEC		0x0
#define UNDEFVEC		0x4
#define SWIVEC			0x8
#define PABTVEC			0xc
#define DABTVEC			0x10
#define IRQVEC			0x18
#define FIQVEC			0x1c

BOOL ISR_setVector(int vecnum, INTVECHANDLER hdlr);

void irq_fromhost(void);
void irq_tohost(void);

void irq_timerA(void);
void irq_timerSW(void);

#endif /* _ISR_H */

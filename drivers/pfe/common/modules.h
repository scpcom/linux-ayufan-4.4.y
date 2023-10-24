/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#ifndef _MODULES_H_
#define _MODULES_H_

#include "types.h"



#ifdef COMCERTO_2000
#include "mtd.h"

#define NUM_CHANNEL EVENT_MAX

typedef struct tModuleDesc {
	void (*entry)(void);
	int (*get_fill_level)(void);
} ModuleDesc, *PModuleDesc;
#else

typedef struct tModuleDesc {
	void (*entry)(void);
	U16 (*cmdproc)(U16 cmd_code, U16 cmd_len, U16 *pcmd);
} ModuleDesc, *PModuleDesc;
#endif

/* modules core functions */
typedef BOOL (*MODULE_INIT)(PModuleDesc);
void module_register(U32 event_id, MODULE_INIT init) __attribute__ ((noinline));


#endif /* _MODULES_H_ */

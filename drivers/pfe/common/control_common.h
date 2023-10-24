/*
 *  Copyright (c) 2012 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#ifndef _CONTROL_COMMON_H_
#define _CONTROL_COMMON_H_

#include "types.h"

#if defined (COMCERTO_2000)
U32 hw_entry_get_field(U32 *field);
void hw_entry_set_field(U32 *field, U32 val);
void hw_entry_set_flags(U32 *hw_flags, U32 flags);
#endif

#endif


/*
 *  Copyright (c) 2012 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#include "control_common.h"
#include "hal.h"


#if defined (COMCERTO_2000)

/** Gets the a field in an hardware  entry.
* The function read a hw entry field atomically
*
* @param field	pointer to a hardware entry field
* @return		field value
*
*/
U32 hw_entry_get_field(U32 *field)	
{
	return readl(field);
}

/** Sets the a field in an hardware  entry.
* The function write to hw entry field atomically
*
* @param field	pointer to a hardware entry field
* @param val	field value to set
*
*/
void hw_entry_set_field(U32 *field, U32 val)
{
	writel(val, field);
}

/** Sets the flags field in an hardware  entry.
* The function converts the value written from host format to PFE/DDR format (endianess conversion).
*
* @param hw_flags	pointer to an hardware entry
* @param flags		flags value to set
*
*/
void hw_entry_set_flags(U32 *hw_flags, U32 flags)
{
	/* FIXME if the entire conntrack is in non-cacheable/non-bufferable memory,
	  there should be no need for the memory barrier */
	wmb();
	writel(cpu_to_be32(flags), hw_flags);
}
#endif


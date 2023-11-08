/**
 *
 * @file	vaai_comp_opt.h
 * @brief	This file contains the generic definition used by TARGET code
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 *
 */


#ifndef _TARGET_COMPILER_OPTION_H_
#define _TARGET_COMPILER_OPTION_H_

#include <linux/version.h>

/*
 * FIXED ME !!
 *
 * (1) 
 * Cuase of different LIO module may have different structure name or member, 
 * we use the target core module version (please refer the TARGET_CORE_MOD_VERSION
 * in target_core_base.h file) to identify them here. (note: We may need to
 * find out another best way to do it)
 *
 * (2)
 * We may consider that we will use different LIO module on one single kernel or
 * one LIO module on different kernel version code. (note: We may remove these
 * in the future ..)
 *
 */
#if 0
#define	SUPPORT_TARGET_VER	TARGET_CORE_VER
#define TGCM_VER(a,b,c,d)	(((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

#if((SUPPORT_TARGET_VER == TGCM_VER(4,1,0,2)) && (LINUX_VERSION_CODE == KERNEL_VERSION(3,4,6)))
/* we may do something here ... */
#endif
#endif

#endif /* _TARGET_COMPILER_OPTION_H_ */


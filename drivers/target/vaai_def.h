/**
 *
 * @file 	vaai_def.h
 * @brief	Basic strcuture declaration header file for VAAI code
 *
 * @author	Adam Hsu
 * @date	2012/06/04
 *
 */

#ifndef VAAI_DEF_H
#define VAAI_DEF_H


#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/bio.h>
#include "tpc_def.h"

//
//
//


#ifdef ENABLE_DBG_PRINT
#define DBG_PRINT(fmt, args...) \
    do{ \
        printk(KERN_DEBUG "[VAAI_DBG] " fmt, ##args); \
    }while(0)
#else
#define DBG_PRINT(fmt, args...)
#endif

//
//
//
#define DO_READ			0x0
#define DO_WRITE		0x1

/* TODO
 * (1) Hard-limit the max len to 1 sector for vmware environment
 * (2) If you want to modify this, please also modify the related i/o code
 */
#define MAX_ATS_LEN		1
#if (MAX_ATS_LEN != 1)
#error max ats len is 1 sector currently
#endif

#define XCOPY_TIMEOUT		60    // 60 second
//
//
//
#define XCOPY_RET_PASS		0
#define XCOPY_RET_NO_XFS_PASS	1 // If parameter list length is zero, don't treat it as error
#define XCOPY_RET_FAIL		2
#define ATS_RET_PASS		0 
#define ATS_RET_FAIL		1



#endif /* VAAI_DEF_H */


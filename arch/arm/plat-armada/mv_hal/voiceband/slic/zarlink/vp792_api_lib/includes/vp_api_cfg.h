/** \file vp_api_cfg.h
 * vp_api_cfg.h
 *
 * This file contains the configuration and compile time settings for
 * building appropriate VP-API library modules needed for any application.
 *
 * Copyright (c) 2010, Zarlink Semiconductor, Inc.
 *
 * $Revision: 7167 $
 * $LastChangedDate: 2010-05-13 19:04:07 -0500 (Thu, 13 May 2010) $
 */

#ifndef VP_API_CFG_H
#define VP_API_CFG_H

#ifdef MV_KERNEL_SLIC_SUPPORT
#include "mvOs.h"
#include "mvSysTdmConfig.h"
#endif
/******************************************************************************
 * COMPILE-TIME OPTIONS:: API DEBUG SPECIFICATIONS                            *
 *****************************************************************************/
/* Define VP_DEBUG to include debug output that can be enabled/disabled at
 * runtime with the VP_OPTION_ID_DEBUG_SELECT option.  The various message
 * types can also be compiled in or out (see vp_debug.h). */
#undef VP_DEBUG

/******************************************************************************
 * COMPILE-TIME OPTIONS:: Conditionally-Compiled API Libraries                *
 *****************************************************************************/
/*
 * Define (or undefine) the appropriate compile time switches based on your
 * application needs.
 *
 * NOTE: Such library selection compile time option(s) MUST be defined before
 * including any other file from VoicePath library.
 *
 * NOTE: More than one Library modules can be built simultaneosuly (if needed).
 */

#define VP_CC_792_SERIES    /**< define to build 792 specific API library;
                             *   undef to exclude this library. */

/******************************************************************************
 * Include Files for the API                                                  *
 *****************************************************************************/
/* Include the API types for this architecture */
#include "vp_api_types.h"

/******************************************************************************
 * Device Context Size                                                        *
 *****************************************************************************/
/*
 * Define the maximum number of lines per device in your system. Note that a
 * system (i.e., all devices controlled by one instance of VP-API) could have
 * more than one type of device, each of those devices may support different
 * number of lines. For example in a system, device A might support 4 lines
 * and device B might support 32 lines, in such a scenario, the following macro
 * should be defined as 32. */
#if defined(VP_CC_792_SERIES)
#define VP_MAX_LINES_PER_DEVICE     (8)
#endif

/******************************************************************************
 * Library Specific COMPILE-TIME OPTIONS and defines                          *
 *****************************************************************************/
#ifdef VP_CC_792_SERIES

#undef VP_COMMON_ADDRESS_SPACE         /* Define if all VP-API-II data
                                           structures (Line Objects, Line Contexts,
                                           Device Objects, Device Contexts, Profiles)
                                           are accessible at the same address in
                                           all processes (tasks) which call VP-API-II
                                           functions. */

#define VP_CC_792_GROUP                 /* Define to include support for sharing
                                         * an interrupt pin between multiple
                                         * VP792 devices. */

#define VP792_MAILBOX_SPINWAIT 50000    /* Number of times to poll the device's
                                           command mailbox before returning
                                           VP_STATUS_MAILBOX_BUSY. */

#undef VP792_INCLUDE_TESTLINE_CODE     /* Defines whether or not to enable
                                         * 792 line test capabilities */

#define VP792_SUPPORT_REV_F_SILICON     /* Define this if you want to support
                                           Revision F of the Le792388 silicon. An
                                           appropriate SLAC firmware patch will
                                           be compiled in. */

#endif /* VP_CC_792_SERIES */

typedef uint8 VpScratchMemType;         /* VpBootLoad() is not used for VE792 devices */

/* Include internal options required to build the VP-API-II library */
#include "vp_api_cfg_int.h"

#endif /* VP_API_CFG_H */

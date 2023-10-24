/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */

#ifndef _VERSION_H_
#define _VERSION_H_

// Helper macros to create version string
#ifdef GCC_TOOLCHAIN
#define MAKE_VERS(v)	TOSTR(v)
#else
#define cat3(x,y,z)			x##y##z
#define MAKE_VERS(v)		TOSTR(cat3($Version:\40,v,\40$))
#endif

// The version string needs to be compliant to the Unix ident
// program.
// A version string that can be queried by the Unix ident program
// has to be of the format
// $<Tag>: <String> $
// Note the spaces after the : and before the trailing $
// The following macros guarantees that the final version string
// stored in FPP_version_string will always be in the right
// format. To generate a new version string, only the value of
// #define VERSION needs to be changed
//

#define VERSION mainline
#define VERSION_LENGTH 32

#endif /* _VERSION_H_ */

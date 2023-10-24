/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.
 *
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


/*******************************************************************
 *
 *    NAME: types.h     
 * 
 *    DESCRIPTION: Defines types
 *
 *******************************************************************/

#ifndef _TYPES_H_
#define _TYPES_H_

// Make sure ENDIAN variable is defined properly
#if !defined(ENDIAN_LITTLE) && !defined(ENDIAN_BIG)
#error Must define either ENDIAN_LITTLE or ENDIAN_BIG
#endif

typedef unsigned char U8;
typedef unsigned short U16;
typedef unsigned int U32;
typedef unsigned long long U64;

typedef volatile unsigned char V8;
typedef volatile unsigned short V16;
typedef volatile unsigned int V32;
typedef volatile unsigned long long V64;

typedef unsigned char BOOL;

typedef void VOID; 
typedef void *PVOID; 

typedef signed char  S8;
typedef signed short S16;
typedef signed int   S32;

typedef void (*INTVECHANDLER)(void);
typedef	void (*EVENTHANDLER)(void);
typedef int (*GETFILLHANDLER)(void);

typedef struct tSTIME {
	U32 msec;
	U32 cycles;
} STIME;

/** Structure common to all interface types */
struct itf {
	struct itf *phys;	/**< pointer to lower lever interface */
	U8 type;		/**< interface type */
	U8 index;		/**< unique interface index */
};

struct physical_port {
	struct itf itf;
	U8 mac_addr[6];
	U8 id;
};

typedef struct tDataQueue {
	void* head;
	void* tail;
} DataQueue, *PDataQueue;

struct tMetadata;

#if defined(COMCERTO_100) || defined(COMCERTO_1000)
typedef int channel_t;
#elif defined(COMCERTO_2000)
typedef void (*channel_t)(struct tMetadata *);
#endif

#define INLINE	__inline

#define TRUE	1
#define FALSE	0

#if !defined(COMCERTO_2000_CONTROL)
#define NULL	(PVOID)0		/* match rtxc def */
#endif

#define HANDLE	PVOID

#define K 			1024
#define M			(K*K)

#define __TOSTR(v)	#v
#define TOSTR(v)	__TOSTR(v)

// enum used to idenity mtd source 
enum FPP_PROTO {
    PROTO_IPV4 = 0,
    PROTO_IPV6,
    PROTO_PPPOE,
    PROTO_MC4,    
    PROTO_MC6
};
#define PROTO_NONE 0xFF

#define MAX_L3_PROTO	PROTO_MC6

enum FPP_ITF {
	ITF_ETH0 = 0,
	ITF_ETH2
};


#endif /* _TYPES_H_ */

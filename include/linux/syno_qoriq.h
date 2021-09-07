/* Copyright (c) 2003-2013 Synology Inc. All rights reserved. */

#ifndef _SYNO_QORIR_H
#define _SYNO_QORIQ_H

#ifdef CONFIG_SYNO_QORIQ_GIANFAR_DROP_CACHE
enum {
	SYNO_DROP_CACHE_UNKNOW = 0,
	SYNO_DROP_CACHE_PAGE = 1,
	SYNO_DROP_CACHE_SLAB = 2
};
#endif

#endif // _SYNO_QORIQ_H

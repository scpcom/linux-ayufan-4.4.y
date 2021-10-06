#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
// Copyright (c) 2003-2015 Synology Inc. All rights reserved.
#ifndef __SYNO_SATA_USER_H_
#define __SYNO_SATA_USER_H_

/**
 * Copy from scsi. Used in both marvell and libata
 * when we ask ebox tell us how many disks they had.
 *
 * @param index  [IN] scsi disk index.
 * @param szBuf  [OUT] disk name. Should not be NULL.
 *
 * @return disk name
 */
static inline char
*DeviceNameGet(const int index, char *szBuf)
{
#if !defined(MY_DEF_HERE)
	if (index < 26) {
		sprintf(szBuf, "sd%c", 'a' + index % 26);
	} else if (index < (26 + 1) * 26) {
		sprintf(szBuf, "sd%c%c",
			'a' + index / 26 - 1,'a' + index % 26);
	} else {
		const unsigned int m1 = (index / 26 - 1) / 26 - 1;
		const unsigned int m2 = (index / 26 - 1) % 26;
		const unsigned int m3 =  index % 26;
		sprintf(szBuf, "sd%c%c%c",
			'a' + m1, 'a' + m2, 'a' + m3);
	}
#endif /* MY_DEF_HERE */

	return szBuf;
}

#endif /* __SYNO_SATA_USER_H_ */

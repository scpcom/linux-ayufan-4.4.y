/*******************************************************************************
Copyright (C) Marvell International Ltd. and its affiliates

This software file (the "File") is owned and distributed by Marvell
International Ltd. and/or its affiliates ("Marvell") under the following
alternative licensing terms.  Once you have made an election to distribute the
File under one of the following license alternatives, please (i) delete this
introductory statement regarding license alternatives, (ii) delete the two
license alternatives that you have not elected to use and (iii) preserve the
Marvell copyright notice above.

********************************************************************************
Marvell Commercial License Option

If you received this File from Marvell and you have entered into a commercial
license agreement (a "Commercial License") with Marvell, the File is licensed
to you under the terms of the applicable Commercial License.

********************************************************************************
Marvell GPL License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File in accordance with the terms and conditions of the General
Public License Version 2, June 1991 (the "GPL License"), a copy of which is
available along with the File in the license.txt file or by writing to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
on the worldwide web at http://www.gnu.org/licenses/gpl.txt.

THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
DISCLAIMED.  The GPL License provides additional details about this warranty
disclaimer.
********************************************************************************
Marvell BSD License Option

If you received this File from Marvell, you may opt to use, redistribute and/or
modify this File under the following licensing terms.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    *   Redistributions of source code must retain the above copyright notice,
	    this list of conditions and the following disclaimer.

    *   Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.

    *   Neither the name of Marvell nor the names of its contributors may be
        used to endorse or promote products derived from this software without
        specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef _VPAPI_DEV_H_
#define _VPAPI_DEV_H_

#include "vp_api_common.h"
#if defined(CONFIG_ZARLINK_SLIC_VE880)
#include "vp880_api.h"
#elif defined(CONFIG_ZARLINK_SLIC_VE792)
#include "vp792_api.h"
#endif

#define VPAPI_MOD_IOCTL_MAGIC           'z'

#define VPAPI_MOD_IOCTL_MIN 		1

/* VP-API System Configuration Functions */
#define VPAPI_MOD_IOX_MK_DEV_OBJ	_IOWR(VPAPI_MOD_IOCTL_MAGIC, 1, VpApiModMkDevObjType)
#define VPAPI_MOD_IOX_MK_LN_OBJ		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 2, VpApiModMkLnObjType)
#define VPAPI_MOD_IOX_MAP_LN_ID		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 3, VpApiModMapLnIdType)
#define VPAPI_MOD_IOX_MAP_SLAC_ID	_IOWR(VPAPI_MOD_IOCTL_MAGIC, 4, VpApiModMapSlacIdType)
#define VPAPI_MOD_IOX_FREE_LN_CTX	_IOWR(VPAPI_MOD_IOCTL_MAGIC, 5, VpApiModFreeLnCtxType)

/* VP-API Initialization Functions */
#define VPAPI_MOD_IOX_INIT_DEV		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 6, VpApiModInitDeviceType)
#define VPAPI_MOD_IOX_CAL_LN		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 7, VpApiModCalLnType)

/* VP-API Control Functions */
#define VPAPI_MOD_IOX_SET_LN_ST		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 8, VpApiModSetLnStType)
#define VPAPI_MOD_IOX_SET_OPTION	_IOWR(VPAPI_MOD_IOCTL_MAGIC, 9, VpApiModSetOptionType)

/* VP-API Status and Query Functions */
#define VPAPI_MOD_IOX_GET_EVENT		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 10, VpApiModGetEventType)

/* VE792 Battery Control */
#define VPAPI_MOD_IOX_BATT_ON		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 11, VpModBatteryOnType)
#define VPAPI_MOD_IOX_BATT_OFF		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 12, VpModBatteryOffType)

/* SLIC register read/write */
#define VPAPI_MOD_IOX_REG_READ		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 13, VpModRegOpType)
#define VPAPI_MOD_IOX_REG_WRITE		_IOWR(VPAPI_MOD_IOCTL_MAGIC, 14, VpModRegOpType)

#define VPAPI_MOD_IOCTL_MAX 		14
#define MAX_SLIC_RDWR_BUFF_SIZE		128

/******************** VP-API System Configuration Structs *********************/
typedef struct VpApiModMkDevObj {
	/* Input arg(s) */
	VpDeviceType	deviceType;
	VpDeviceIdType	deviceId;

	/* Output arg(s) */
	VpStatusType		status;
} VpApiModMkDevObjType;

typedef struct VpApiModMkLnObj {
	/* Input arg(s) */
	VpTermType	termType;
	VpLineIdType	lineId;

	/* Output arg(s) */
	VpStatusType	status;
} VpApiModMkLnObjType;

typedef struct VpApiModMapLnId {
	/* Input arg(s) */
	VpLineIdType	lineId;

	/* Output arg(s) */
	VpStatusType	status;
} VpApiModMapLnIdType;

typedef struct VpApiModMapSlacId {
	/* Input arg(s) */
	VpDeviceIdType	deviceId;
	unsigned char	slacId;

	/* Output arg(s) */
	VpStatusType	status;
} VpApiModMapSlacIdType;

typedef struct VpApiModFreeLnCtx {
	/* Input arg(s) */
	VpLineIdType	lineId;

	/* Output arg(s) */
	VpStatusType	status;
} VpApiModFreeLnCtxType;

/************************ VP-API Initialization Structs ************************/
typedef struct VpApiModInitDevice {
	/* Input arg(s) */
	VpDeviceIdType		deviceId;
	VpProfilePtrType	pDevProfile;
	VpProfilePtrType	pAcProfile;
	VpProfilePtrType	pDcProfile;
	VpProfilePtrType	pRingProfile;
	VpProfilePtrType	pFxoAcProfile;
	VpProfilePtrType	pFxoCfgProfile;
	unsigned short		devProfileSize;
	unsigned short		acProfileSize;
	unsigned short		dcProfileSize;
	unsigned short		ringProfileSize;
	unsigned short		fxoAcProfileSize;
	unsigned short		fxoCfgProfileSize;

	/* Output arg(s) */
	VpStatusType		status;
} VpApiModInitDeviceType;

typedef struct VpApiModCalLn {
	/* Input arg(s) */
	VpLineIdType	lineId;

	/* Output arg(s) */
	VpStatusType	status;
} VpApiModCalLnType;

/****************************VP-API Control Structs ***************************/
typedef struct VpApiModSetLnSt {
	/* Input arg(s) */
	VpLineIdType	lineId;
	VpLineStateType	state;

	/* Output arg(s) */
	VpStatusType	status;
} VpApiModSetLnStType;

typedef struct VpApiModSetOption {
	/* Input arg(s) */
	unsigned char	lineRequest;
	VpLineIdType	lineId;
	VpDeviceIdType	deviceId;
	VpOptionIdType	option;
	void		*pValue;

	/* Output arg(s) */
	VpStatusType	status;
} VpApiModSetOptionType;

#if 0
typedef struct VpApiModLowLvlCmd {
	/* Input arg(s) */
	const VpModLineRegNumType	lineRegNum;
	uint8				*pCmdData;
	const uint8			len;
	const uint16			handle;

	/* Output arg(s) */
	VpStatusType			status;
} VpApiModLowLvlCmdType;
#endif

/********************** VP-API Status and Query Structs ***********************/
typedef struct VpApiModGetEvent {
	/* Input arg(s) */
	VpDeviceIdType	deviceId;

	/* Output arg(s) */
	bool		newEvent;
	VpEventType	*pEvent;
} VpApiModGetEventType;

/********************** VE792 Battery Control ***********************/
typedef struct VpModBatteryOn {
	/* Input arg(s) */
	int	vbh;
	int	vbl;
	int	vbp;

	/* Output arg(s) */
	int	status;
} VpModBatteryOnType;

typedef struct VpModBatteryOff {

	/* Output arg(s) */
	int	status;
} VpModBatteryOffType;

/********************** SLIC register read/write ********************/
typedef struct VpModRegOp {
	/* Input arg(s) */
	VpLineIdType	lineId;
	unsigned char	cmd;
	unsigned short  cmdLen;
	unsigned char buff[MAX_SLIC_RDWR_BUFF_SIZE];

	/* Output arg(s) */
	VpStatusType	status;
} VpModRegOpType;

/* APIs */
int vpapi_module_init(void);
void vpapi_module_exit(void);

#endif /*_VPAPI_DEV_H_*/

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

#ifndef _SILABS_DEV_H_
#define _SILABS_DEV_H_

#include "proslic.h"
#include "si3226x.h"
#include "si3217x.h"

#define SILABS_MOD_IOCTL_MAGIC           'z'

#define SILABS_MOD_IOCTL_MIN 			1

/* SI-API System Configuration Functions */
#define SILABS_MOD_IOX_CTRL_IF			_IOWR(SILABS_MOD_IOCTL_MAGIC, 1, SilabsModCtrlObjType)
#define SILABS_MOD_IOX_DEVICE_INIT		_IOWR(SILABS_MOD_IOCTL_MAGIC, 2, SilabsModDevObjType)
#define SILABS_MOD_IOX_CHAN_ALL			_IOWR(SILABS_MOD_IOCTL_MAGIC, 3, SilabsModChannelAllObjType)
#define SILABS_MOD_IOX_CHAN_INIT		_IOWR(SILABS_MOD_IOCTL_MAGIC, 4, SilabsModChannelObjType)
#define SILABS_MOD_IOX_CHAN_SETUP		_IOWR(SILABS_MOD_IOCTL_MAGIC, 5, SilabsModChannelSetupObjType)
#define SILABS_MOD_IOX_CHAN_OP			_IOWR(SILABS_MOD_IOCTL_MAGIC, 6, SilabsModChannelOpObjType)
#define SILABS_MOD_IOX_PCM_TS_SETUP		_IOWR(SILABS_MOD_IOCTL_MAGIC, 7, SilabsModPCMTSSetupObjType)
#define SILABS_MOD_IOX_GET_EVENT		_IOWR(SILABS_MOD_IOCTL_MAGIC, 8, SilabsModGetEventType)
#define SILABS_MOD_IOX_REG_CTRL			_IOWR(SILABS_MOD_IOCTL_MAGIC, 9, SilabsRegObjType)
#define SILABS_MOD_IOX_CHAN_LINE_FEED	_IOWR(SILABS_MOD_IOCTL_MAGIC, 10, SilabsModChannelLineFeedObjType)
#define SILABS_MOD_IOX_CHAN_HOOK_STATUS	_IOWR(SILABS_MOD_IOCTL_MAGIC, 11, SilabsModChannelReadHookStatObjType)
#define SILABS_MOD_IOX_CHAN_LOOPBACK	_IOWR(SILABS_MOD_IOCTL_MAGIC, 12, SilabsModChannelSetLoopbackObjType)
//#define SILABS_MOD_IOX_SET_PROFILE		_IOWR(SILABS_MOD_IOCTL_MAGIC, 13, SilabsModCSetProfileObjType)

/* SI-API Initialization Functions */
/* SI-API Control Functions */
/* SI-API Status and Query Functions */
/* SLIC register read/write */

#define SILABS_MOD_IOCTL_MAX 		14
#define MAX_SLIC_RDWR_BUFF_SIZE		128

/******************** SI-API Enums *********************/

typedef enum {
	SI_IF_CREATE,
	SI_IF_DESTROY,
	SI_IF_SET_FUNCS
} ControlFuncType;

typedef enum {
	SI_DEVICE_CREATE,
	SI_DEVICE_DESTROY
} DeviceFuncType;

typedef enum {
	SI_CHANNEL_CREATE,
	SI_CHANNEL_DESTROY,
	SI_CHANNEL_BC_INIT,
	SI_CHANNEL_SW_INIT,
} ChannelFuncType;

typedef enum {
	SI_CHANNEL_ALL_INIT,
	SI_CHANNEL_ALL_CAL,
	SI_CHANNEL_ALL_LBCAL
} ChannelAllFuncType;

typedef enum {
	SI_CHANNEL_SETUP_RING,
	SI_CHANNEL_SETUP_TONEGEN,
	SI_CHANNEL_SETUP_FSK,
	SI_CHANNEL_SETUP_DTMF_DECODE,
	SI_CHANNEL_SETUP_ZSYNTH,
	SI_CHANNEL_SETUP_GCICI,
	SI_CHANNEL_SETUP_MODEM_DET,
	SI_CHANNEL_SETUP_TX_AUDIO_GAIN,
	SI_CHANNEL_SETUP_RX_AUDIO_GAIN,
	SI_CHANNEL_SETUP_DC_FEED,
	SI_CHANNEL_SETUP_PULSE_METER,
	SI_CHANNEL_SETUP_PCM,
} ChannelSetupFuncType;

typedef enum {
	SI_CHANNEL_OP_RESET,
	SI_CHANNEL_OP_CLEAR_ERR,
	SI_CHANNEL_OP_GPIO_SETUP,
	SI_CHANNEL_OP_VERIFY_CTRL_IF,
	SI_CHANNEL_OP_SHUT_DOWN_CHAN,
	SI_CHANNEL_OP_RING_START,
	SI_CHANNEL_OP_RING_STOP,
	SI_CHANNEL_OP_ENA_CID,
	SI_CHANNEL_OP_DIS_CID,
	SI_CHANNEL_OP_PLL_FREE_RUN_START,
	SI_CHANNEL_OP_PLL_FREE_RUN_STOP,
	SI_CHANNEL_OP_PULSE_METER_START,
	SI_CHANNEL_OP_PULSE_METER_STOP,
	SI_CHANNEL_OP_PWR_UP_CONV,
	SI_CHANNEL_OP_PWR_DOWN_CONV,
	SI_CHANNEL_OP_ENA_INT,
	SI_CHANNEL_OP_PCM_START,
	SI_CHANNEL_OP_PCM_STOP,
	SI_CHANNEL_OP_TONE_GEN_START,
	SI_CHANNEL_OP_TONE_GEN_STOP,
} ChannelOPFuncType;

typedef enum {
	SI_REG_READ,
	SI_REG_WRITE,
	SI_RAM_READ,
	SI_RAM_WRITE
} RegFuncType;

/************************ SI-API Initialization Structs ************************/

typedef struct SilabsModCtrlObj {
	/* Input arg(s) */
	ControlFuncType			func;
	/* Output arg(s) */
	int						status;
} SilabsModCtrlObjType;

typedef struct SilabsModDevObj {
	/* Input arg(s) */
	int						devNum;
	DeviceFuncType			func;
	/* Output arg(s) */
	int						status;
} SilabsModDevObjType;

typedef struct SilabsModChannelObj {
	/* Input arg(s) */
	int						chanNum;
	ChannelFuncType			func;
	/* Output arg(s) */
	int						status;
} SilabsModChannelObjType;

/******************** SI-API System Configuration Structs *********************/

typedef struct SilabsModChannelSetupObj {
	/* Input arg(s) */
	int						chanNum;
	int						preset;
	ChannelSetupFuncType	func;
		/* Output arg(s) */
	int						status;
} SilabsModChannelSetupObjType;

typedef struct SilabsModChannelAllObj {
	/* Input arg(s) */
	ChannelAllFuncType		func;
	/* Output arg(s) */
	int						status;
} SilabsModChannelAllObjType;

/****************************VP-API Control Structs ***************************/

typedef struct SilabsModPCMTSSetupObj {
	/* Input arg(s) */
	int						chanNum;
	uInt16 					rxcount;
	uInt16 					txcount;
	/* Output arg(s) */
	int						status;
} SilabsModPCMTSSetupObjType;

typedef struct SilabsModChannelOpObj {
	/* Input arg(s) */
	int						chanNum;
	ChannelOPFuncType		func;
	/* Output arg(s) */
	int						status;
} SilabsModChannelOpObjType;

typedef struct SilabsModChannelLoopbackModeObj {
	/* Input arg(s) */
	int						chanNum;
	ProslicLoopbackModes 	newMode;
	/* Output arg(s) */
	int						status;
} SilabsModChannelLoopbackModeObjType;

typedef struct SilabsModChannelLineFeedObj {
	/* Input arg(s) */
	int						chanNum;
	uInt8				 	newLineFeed;
	/* Output arg(s) */
	int						status;
} SilabsModChannelLineFeedObjType;

typedef struct SilabsModChannelSetLoopbackObj {
	/* Input arg(s) */
	int						chanNum;
	ProslicLoopbackModes 	newMode;
	/* Output arg(s) */
	int						status;
} SilabsModChannelSetLoopbackObjType;

#if 0
typedef struct SilabsModCSetProfileObj {
	/* Input arg(s) */
	Si3226x_General_Cfg User_Si3226x_General_Configuration;
	Si3226x_GPIO_Cfg User_Si3226x_GPIO_Configuration;
	Si3226x_CI_Cfg User_Si3226x_CI_Presets;
	Si3226x_audioGain_Cfg User_Si3226x_audioGain_Presets;
	Si3226x_Ring_Cfg User_Si3226x_Ring_Presets;
	Si3226x_DCfeed_Cfg User_Si3226x_DCfeed_Presets;
	Si3226x_Impedance_Cfg User_Si3226x_Impedance_Presets;
	Si3226x_FSK_Cfg User_Si3226x_FSK_Presets;
	Si3226x_PulseMeter_Cfg User_Si3226x_PulseMeter_Presets;
	Si3226x_Tone_Cfg User_Si3226x_Tone_Presets[5];
	Si3226x_PCM_Cfg User_Si3226x_PCM_Presets[4];

	/* Output arg(s) */
	int						status;
} SilabsModCSetProfileObjType
#endif
/********************** VP-API Status and Query Structs ***********************/

typedef struct SiEventTypeObj {
	int 					chanNum;	/* Channel that caused the event */
	int 					devNum;		/* device chip select ID corresponding to the
											device that caused the event */
	uInt8					eventsNum;
	ProslicInt				irqs[MAX_PROSLIC_IRQS];	/* The events that occurred.  Requires that the event
														catagory be known to interpret */
} SiEventType;

typedef struct SilabsModGetEventObj {
	/* Input arg(s) */
	int						devNum;
	/* Output arg(s) */
	bool					newEvent;
	SiEventType				*pEvent;
	int						status;
} SilabsModGetEventType;

typedef struct SilabsModChannelReadHookStatObj {
	/* Input arg(s) */
	int 					chanNum;	/* Channel that caused the event */
	/* Output arg(s) */
	uInt8					hookStatus;
	int						status;
}
SilabsModChannelReadHookStatObjType;

/********************** SLIC register read/write ********************/
typedef struct SilabsRegObj {
	/* Input arg(s) */
	int						chanNum;
	RegFuncType				func;
	unsigned char 			regAddr;
	uInt16 					ramAddr;
	unsigned char 			value;
	unsigned int 			ramValue;

	/* Output arg(s) */
	int						status;
} SilabsRegObjType;

/* APIs */
int silabs_module_init(void);
void silabs_module_exit(void);

#endif /*_SILABS_DEV_H_*/

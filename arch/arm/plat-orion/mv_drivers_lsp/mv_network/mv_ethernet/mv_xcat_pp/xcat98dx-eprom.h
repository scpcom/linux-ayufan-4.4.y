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
*******************************************************************************/

#ifndef __INCmvPresteraEEPROMh
#define __INCmvPresteraEEPROMh

#include "mvTypes.h"

MV_STATUS mvPpEeprom_DB_XCat2A1_QFP_24FE_4GP(void);
MV_STATUS mvPpEeprom_DB_XCat2A1_48FE_4GP(void);
MV_STATUS mvPpEeprom_DB_XCat2A1_48FE_4GP_dev(MV_U32 dev);
MV_STATUS mvPpEeprom_DB_XCat2A1_24GE_4GP(void);
MV_STATUS simulate_PP_EEPROM_4122_24GE_PHY_A0(void);
MV_STATUS simulate_PP_EEPROM_2122_24FE(void);
MV_STATUS simulate_PP_EEPROM_DB_xCat2A0_BGA_24GE_4GP(void);
MV_STATUS simulate_PP_EEPROM_DB_xCat2A0_QFP_24GE_2SFP(void);
MV_STATUS simulate_PP_EEPROM_DB_xCat2A1_QFP_24GE_2SFP(void);
MV_STATUS mvPpEeprom_sleed_board(void);

#endif /* __INCmvPresteraEEPROMh */

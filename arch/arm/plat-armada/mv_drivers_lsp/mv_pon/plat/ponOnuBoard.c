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

******************************************************************************/

/******************************************************************************
**  FILE        : ponOnuBoard.c                                              **
**                                                                           **
**  DESCRIPTION : This file implements ONU Board specific                    **
*******************************************************************************
*                                                                             *                              
*  MODIFICATION HISTORY:                                                      *
*                                                                             *
*   29Oct06  Oren Ben Hayun   created                                         *  
* =========================================================================== *      
******************************************************************************/

/* Include Files
------------------------------------------------------------------------------*/
#include "ponOnuHeader.h"
#include "mvCommon.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "boardEnv/mvBoardEnvSpec.h"

/* Local Constant
------------------------------------------------------------------------------*/                                               

/* Global Variables
------------------------------------------------------------------------------*/

/* Global functions
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/

/*******************************************************************************
**
**  onuGponLedHandler
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function handles led operation
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR          
**                   
*******************************************************************************/
MV_STATUS onuGponLedHandler(MV_U32 led, MV_U32 action)
{
  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDisableSnSetHandler
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function saves disable SN state to non-volatile memory
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR          
**                   
*******************************************************************************/
MV_STATUS onuGponDisableSnSetHandler(MV_BOOL state)
{
  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponDisableSnGetHandler
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function extract disable SN state from non-volatile memory
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR          
**                   
*******************************************************************************/
MV_STATUS onuGponDisableSnGetHandler(MV_BOOL *state)
{
  *state = MV_FALSE;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponPasswordHandler
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function extract password from external source
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR          
**                   
*******************************************************************************/
MV_STATUS onuGponPasswordHandler(MV_U8 *password)
{
  MV_U8 defaultPassword[10] = ONU_GPON_PASSWORD_DEFAULT;

  memcpy(password, defaultPassword, 10);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponSerialNumberHandler
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function extract serial number from external source
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR          
**                   
*******************************************************************************/
MV_STATUS onuGponSerialNumberHandler(MV_U8 *serialNumber)
{
  MV_U8 defaultSerialNumber[8] = ONU_GPON_SN_DEFAULT;

  memcpy(serialNumber, defaultSerialNumber, 8);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuPonSerdesInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function set serdes
**               
**  PARAMETERS:  None
**
**  OUTPUTS:     None    
**
**  RETURNS:     MV_OK or error
**
*******************************************************************************/
MV_STATUS onuPonSerdesInit(void)
{
  MV_STATUS status;
  MV_U32    rxReady = 0;
  MV_U32    txReady = 0;
  MV_U32    initDone;
  MV_U32    boardType;
  MV_U32    temp;

  /* GPON/EPON configuration of XVR SD */
  /* ================================= */
  boardType = mvBoardIdGet();
 
  switch(boardType) 
  {
    /* mapped to MPP18 */
    case DB_88F6535_BP_ID: 
      status  = asicOntMiscRegWrite(mvAsicReg_PON_MPP_18, DB_88F65xx_XVR_SD, 0); 
      if (status != MV_OK) 
        return(status);
      break;
 
    /* mapped to MPP69 */
    case RD_88F6510_SFU_ID:
    case RD_88F6560_GW_ID: 
      status  = asicOntMiscRegWrite(mvAsicReg_PON_MPP_69, RD_88F65xx_XVR_SD, 0); 
      if (status != MV_OK) 
        return(status);
      break;
 
    /* mapped to MPP69 */
    case RD_88F6530_MDU_ID:
      status  = asicOntMiscRegWrite(mvAsicReg_PON_MPP_69, RD_88F65xx_XVR_SD, 0); 
      if (status != MV_OK) 
        return(status);
      break;
  }

  /* GPON/EPON configuration/SerDes power up and init sequence */
  /* ========================================================= */
  
  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_RST, 0x1, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_PU_Pll, 0x1, 0);
  if (status != MV_OK) 
    return(status);
 
  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_PU_RX, 0x1, 0); 
  if (status != MV_OK) 
    return(status);
 
  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_PU_TX, 0x1, 0);  
  if (status != MV_OK) 
    return(status);

#ifdef CONFIG_MV_GPON_MODULE

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_SEL_GEPON, 0x0, 0); 
 if (status != MV_OK) 
   return(status);

#else /* CONFIG_MV_EPON_MODULE */

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_SEL_GEPON, 0x1, 0); 
  if (status != MV_OK) 
    return(status);

#endif /* CONFIG_MV_GPON_MODULE */

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_REF_CLK_25M, 0x1, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_RST, 0x0, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_PASSWORD, 0x76, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_EN_LOOP_TIMING, 0x1, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

#ifdef CONFIG_MV_GPON_MODULE

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_PON_SELECT, 0x0, 0); 
  if (status != MV_OK) 
    return(status);

#else /* CONFIG_MV_EPON_MODULE */

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_PON_SELECT, 0x3, 0); 
  if (status != MV_OK) 
    return(status);

#endif /* CONFIG_MV_GPON_MODULE */

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_PU_Pll, 0x0, 0);
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_PU_RX, 0x0, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_PU_TX, 0x0, 0);  
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  do 
  {
    status  = asicOntMiscRegRead(mvAsicReg_PON_SERDES_PHY_CTRL_0_READY_TX, &txReady, 0);  
    if (status != MV_OK) 
      return(status);

    status  = asicOntMiscRegRead(mvAsicReg_PON_SERDES_PHY_CTRL_0_READY_RX, &rxReady, 0);  
    if (status != MV_OK) 
      return(status);

  } while ((txReady == 0) || (rxReady == 0));

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_RX_INIT, 0x1, 0);  
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  do 
  {
    status  = asicOntMiscRegRead(mvAsicReg_PON_SERDES_PHY_CTRL_0_INIT_DONE, &initDone, 0);  
    if (status != MV_OK) 
      return(status);
 
  } while (initDone == 0);
 
  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_RX_INIT, 0x0, 0);  
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_RST_TX_DOUT, 0x0, 0);  
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_OPEN_TX_DOOR, 0x0, 0);  
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_EN_LOOP_TIMING, 0x1, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_RX_INIT, 0x1, 0);  
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  do 
  {
    status  = asicOntMiscRegRead(mvAsicReg_PON_SERDES_PHY_CTRL_0_INIT_DONE, &initDone, 0);  
    if (status != MV_OK) 
      return(status);
 
  } while (initDone == 0);
 
  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0_RX_INIT, 0x0, 0);  
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_CLK_EN, 0x0, 0); 
  if (status != MV_OK) 
      return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_CLK_SEL, 0x1, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_CLK_EN, 0x1, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_1_BEN_IO_EN, 0x0, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(10);

  // Erez I - Sequence Start
  // =======================

  asicOntMiscRegRead(mvAsicReg_PON_SERDES_PHY_CTRL_0, &temp, 0);
  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0, temp | 0xF, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(100);

  asicOntMiscRegRead(mvAsicReg_PON_SERDES_PHY_CTRL_0, &temp, 0);
  temp &= ~(0x8);
  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0, temp, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(100);

  asicOntMiscRegRead(mvAsicReg_PON_SERDES_INTERNAL_PASSWORD, &temp, 0);
  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_PASSWORD, temp | 0x76, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(100);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_EN_LOOP_TIMING, 0x1, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(100);

  asicOntMiscRegRead(mvAsicReg_PON_SERDES_PHY_CTRL_0, &temp, 0);
  temp &= ~(0xF);
  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_PHY_CTRL_0, temp, 0); 
  if (status != MV_OK) 
    return(status);

  mvOsDelay(100);

  status  = asicOntMiscRegWrite(mvAsicReg_PON_SERDES_INTERNAL_OPEN_TX_DOOR, 0x0, 0);  
  if (status != MV_OK) 
    return(status);

  // Erez I - Sequence End
  // =====================

  return(MV_OK);
}

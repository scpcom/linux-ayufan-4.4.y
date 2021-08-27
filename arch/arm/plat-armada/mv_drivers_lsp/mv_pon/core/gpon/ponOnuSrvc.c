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
**  FILE        : ponOnuSrvc.c                                               **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON Service Routines             **
**                functionality                                              **
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

/* Local Constant
------------------------------------------------------------------------------*/  
#define __FILE_DESC__ "mv_pon/core/gpon/ponOnuSrvc.c"

#define POLYNOMIAL (0x04c11db7)

static unsigned long crc_table[256];

/* Global Variables
------------------------------------------------------------------------------*/
S_DbrBlockSize gponDbrArray[2048];
MV_U32         gponDbrBlock2048AndMore;

/* Local Variables
------------------------------------------------------------------------------*/
  
/* Export Functions
------------------------------------------------------------------------------*/
MV_STATUS     onuGponSrvcGenCrcTable(void);
unsigned long onuGponSrvcUpdateCrc(unsigned long crc_accum, char *data_blk_ptr, int data_blk_size);
MV_STATUS     onuGponSrvcGenDbrTable(void);

/* Local Functions
------------------------------------------------------------------------------*/

/*******************************************************************************
**
**  onuGponSrvcInit
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function init onu CRC generator
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK 
**
*******************************************************************************/
MV_STATUS onuGponSrvcInit(void)
{
  MV_STATUS rcode;

  rcode = onuGponSrvcGenDbrTable();
  if (rcode != MV_OK) 
    return(MV_ERROR);

  rcode = onuGponSrvcGenCrcTable();
  if (rcode != MV_OK) 
    return(MV_ERROR);

  return (MV_OK);
}

/*******************************************************************************
**
**  onuGponSrvcGenCrcTable
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function generate the table of CRC remainders 
**               for all possible bytes 
**               
**  PARAMETERS:  None  
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuGponSrvcGenCrcTable(void)
{ 
  register int           i, j;  
  register unsigned long crc_accum;

  for ( i = 0;  i < 256;  i++ )
  { 
    crc_accum = ( (unsigned long) i << 24 );
    for ( j = 0;  j < 8;  j++ )
    { 
      if ( crc_accum & 0x80000000 )
      {
        crc_accum = ( crc_accum << 1 ) ^ POLYNOMIAL;
      }
      else
      {
        crc_accum = ( crc_accum << 1 ); 
      }
    }
    crc_table[i] = crc_accum; 
  } 

  return(MV_OK); 
}

/*******************************************************************************
**
**  onuGponSrvcUpdateCrc
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function CRC table 
**               
** 	PARAMETERS:  unsigned long crc_accum
** 				 char          *data_blk_ptr
**				 int           data_blk_size  
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
unsigned long onuGponSrvcUpdateCrc(unsigned long crc_accum, char *data_blk_ptr, int data_blk_size)
{ 
   register int i, j;
   
   for ( j = 0;  j < data_blk_size;  j++ )
   { 
     i = ( (int) ( crc_accum >> 24) ^ *data_blk_ptr++ ) & 0xff;
     crc_accum = ( crc_accum << 8 ) ^ crc_table[i]; 
   }

   return (crc_accum ^ 0xFFFFFFFF); 
}
 
/*******************************************************************************
**
**  onuGponSrvcSerialNumberSet
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function set SN in database and GPON MAC 
**               
**  PARAMETERS:  MV_U8 *serialNumber 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error 
**
*******************************************************************************/
MV_STATUS onuGponSrvcSerialNumberSet(MV_U8 *serialNumber)
{
  MV_STATUS rcode;
  MV_U32	 snMsg[ONU_GPON_MSG_LEN];
  MV_U32    onuId;

  /* Update Serial Number In Data Base */
  /* --------------------------------- */
  rcode = onuGponDbSerialNumSet((MV_U8*)serialNumber);
  if (rcode != MV_OK) 
	return (rcode);

  /* Get Serial Number Message from Database */
  onuGponDbSnMsgGet(snMsg);

  /* Update the Serial Number in S/N Message */
  snMsg[0] = (snMsg[0] & 0xFFFF0000) |
             (((MV_U32)(serialNumber[1])) | 
              (((MV_U32)(serialNumber[0])) << 8));
  snMsg[1] = (((MV_U32)(serialNumber[5])) | 
              (((MV_U32)(serialNumber[4])) << 8) |
              (((MV_U32)(serialNumber[3])) << 16)|
							(((MV_U32)(serialNumber[2])) << 24));
  snMsg[2] = ((((MV_U32)(serialNumber[7])) << 16) | 
              (((MV_U32)(serialNumber[6])) << 24) |
                         0x00000505);

  /* Put back the updated Serial Number Message to Database */
  onuGponDbSnMsgSet(snMsg);

  /* Update Serial Number in ASIC */
  /* ---------------------------- */
  onuId = onuGponDbOnuIdGet();
	
  rcode = mvOnuGponMacTxConstSerialNumberMsgSet(onuId,serialNumber,0);

  if (rcode != MV_OK) 
    return (rcode);

  return (MV_OK);
}

/*******************************************************************************
**
**  onuGponSrvcConstPloamFromDbInit
**  ____________________________________________________________________________
**
** 	DESCRIPTION: The function initialize the Constant Ploams in ASIC
**	  			 by values are in Database 
**               
**  PARAMETERS:  MV_BOOL initTime 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or error  
**
*******************************************************************************/
MV_STATUS onuGponSrvcConstPloamFromDbInit(MV_BOOL initTime)
{
  MV_U32    onuId;
  MV_U8     serialNumber[8];
  MV_STATUS rcode;

  if (initTime != MV_FALSE)	/* In init time we wouldn't want to use semaphore */
  {
	  onuId = onuGponDbOnuIdGet();
      onuGponDbSerialNumGet(serialNumber);
     
      rcode  = mvOnuGponMacTxConstIdleMsgSet(onuId);
      rcode |= mvOnuGponMacTxConstSerialNumberMsgSet(onuId, serialNumber,0);

      return (rcode);
  }

  onuId = onuGponDbOnuIdGet();
  onuGponDbSerialNumGet(serialNumber);

  rcode  = mvOnuGponMacTxConstIdleMsgSet(onuId);
  rcode |= mvOnuGponMacTxConstSerialNumberMsgSet(onuId, serialNumber,0);

  return (rcode);
}

/*******************************************************************************
**
**  onuGponSrvcOnuIdUpdate
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function update the ONU ID in the ASIC register and 
**               const ram
**               
**  PARAMETERS:  MV_U32 onuId
**
**  OUTPUTS:     None    
**
**  RETURNS:     MV_OK or error  
**
*******************************************************************************/
MV_STATUS onuGponSrvcOnuIdUpdate(MV_U32 onuId, MV_BOOL valid)
{
  MV_STATUS rcode;
  MV_U32	snMsg[ONU_GPON_MSG_LEN];
  MV_U32	idleMsg[ONU_GPON_MSG_LEN];
  MV_U8     serialNumber[8];

  /* update onu Id register */
  rcode = mvOnuGponMacOnuIdSet (onuId, valid); 
  if (rcode != MV_OK) return (rcode);

  /* Get Serial Number Message from Database */
  onuGponDbSnMsgGet(snMsg);

  /* Update the Serial Number in S/N Message */
  snMsg[0] = (ONU_GPON_US_MSG_SN_ONU << 16) | (onuId << 24) |
             (snMsg[0] & 0x0000FFFF);

  /* Put back the updated Serial Number Message to Database */
  onuGponDbSnMsgSet(snMsg);
	
  /* Get Idle Message from Database */
  onuGponDbIdleMsgGet(idleMsg);

  /* Update the Serial Number in S/N Message */
  idleMsg[0] = (ONU_GPON_US_MSG_NO_MESSAGE << 16) | (onuId << 24) |
               (idleMsg[0] & 0x0000FFFF);
  /* Put back the updated Serial Number Message to Database */
  onuGponDbIdleMsgSet(idleMsg);

  onuGponDbSerialNumGet(serialNumber);

  /* Write to ASIC the new Constant Messages */
  rcode  = mvOnuGponMacTxConstIdleMsgSet(onuId);
  rcode |= mvOnuGponMacTxConstSerialNumberMsgSet(onuId, serialNumber,0);

  return(rcode);
}

/*******************************************************************************
**
**  onuGponSrvcAlarmNotify
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function call Alarm Notification routine (if exist)
**               
** 	PARAMETERS:  MV_U32 alarm
**	             MV_U32 status 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponSrvcAlarmNotify(MV_U32 alarm, MV_U32 status)
{
  ALARMNOTIFYFUNC alarmFunc;     

  alarmFunc = onuGponDbAlarmNotifyGet();
  if (alarmFunc != NULL)
  {
	  alarmFunc(alarm, status);
  }
}

/*******************************************************************************
**
**  onuGponSrvcStatusNotify
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function call Status Notification routine (if exist)
**               
**  PARAMETERS:  MV_U32 status 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponSrvcStatusNotify(MV_U32 status)
{
  STATUSNOTIFYFUNC statusFunc = NULL; 

  /* Send signal to management layer ONLY for US sync */
  if (status != GPON_ONU_STATUS_RANGED)
    return;

  statusFunc = onuGponDbStatusNotifyGet();
  if (statusFunc != NULL)
  {
	  statusFunc(status);
  }
}

/*******************************************************************************
**
**  onuGponSrvcOmccNotify
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function call OMCC Port Notification routine (if exist)
**               
**  PARAMETERS:  MV_U32 omccPortId 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponSrvcOmccNotify(MV_U32 omccPortId)
{
  OMCCNOTIFYFUNC omccFunc;   

  omccFunc = onuGponDbOmccNotifyGet();
  if (omccFunc != NULL)
  {
	  omccFunc(omccPortId);
  }
}

/*******************************************************************************
**
**  onuGponSrvcDisableMsgNotify
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function call onu dusable Notification routine (if exist)
**               
**  PARAMETERS:  MV_BOOL disable 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponSrvcDisableMsgNotify(MV_BOOL disable)
{
  DISABLENOTIFYFUNC	disableFunc;   

  disableFunc = onuGponDbDisableNotifyGet();
  if (disableFunc != NULL)
  {
	  disableFunc(disable);
  }
}

/*******************************************************************************
**
**  onuGponSrvcAesKeyGenerate
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function generates AES key
**               
**  PARAMETERS:  MV_U8 *key 
**
**  OUTPUTS:     8 bytes buffer
**
**  RETURNS:     None 
**
*******************************************************************************/
void onuGponSrvcAesKeyGenerate(MV_U8 *key)
{
  MV_U32        aesKey[4] = {0x00000000,0x00000000,0x00000000,0x00000000};
  MV_U32        i;
  MV_U32        j;
  static MV_U32 counter = 0;
  MV_U32        microSec;
  MV_U32        microSec_1;
  MV_U32        advancer;
  MV_U32        bw;
  MV_U32        sfc;

  mvOnuGponMacMicroSecCounterGet(&microSec);
  mvOnuGponMacTxPloamDataFreeGet(&advancer);
  mvOnuGponMacMicroSecCounterGet(&microSec_1);
  microSec_1 += 0x11111111;
  bw = 0xA5A5A5A5;
  mvOnuGponMacRxSuperFrameCounterGet(&sfc);
  bw ^= sfc;

  for (i = 0 ; i < 8 ; i++)
  {
    aesKey[0] |= (MV_U32)((microSec >> i)   & 0x1) << (4*i);
    aesKey[0] |= (MV_U32)((bw >> i)         & 0x1) << (4*i + 1);
    aesKey[0] |= (MV_U32)((counter >> i)    & 0x1) << (4*i + 2);
    aesKey[0] |= (MV_U32)((microSec_1 >> i) & 0x1) << (4*i + 3);
  }              
  for (i = 8 ; i < 16 ; i++)
  {
    aesKey[1] |= (MV_U32)((microSec >> i)   & 0x1) << (4*(i-8));
    aesKey[1] |= (MV_U32)((bw >> i)         & 0x1) << (4*(i-8) + 1);
    aesKey[1] |= (MV_U32)((counter >> i)    & 0x1) << (4*(i-8) + 2);
    aesKey[1] |= (MV_U32)((microSec_1 >> i) & 0x1) << (4*(i-8) + 3);
  }
  for (i = 16 ; i < 24 ; i++)
  {
    aesKey[2] |= (MV_U32)((microSec >> i)   & 0x1) << (4*(i-16));
    aesKey[2] |= (MV_U32)((bw >> i)         & 0x1) << (4*(i-16) + 1);
    aesKey[2] |= (MV_U32)((counter >> i)    & 0x1) << (4*(i-16) + 2);
    aesKey[2] |= (MV_U32)((microSec_1 >> i) & 0x1) << (4*(i-16) + 3);
  }
  for (i = 24 ; i < 32 ; i++)
  {
    aesKey[3] |= (MV_U32)((microSec >> i)   & 0x1) << (4*(i-24));
    aesKey[3] |= (MV_U32)((bw >> i)         & 0x1) << (4*(i-24) + 1);
    aesKey[3] |= (MV_U32)((counter >> i)    & 0x1) << (4*(i-24) + 2);
    aesKey[3] |= (MV_U32)((microSec_1 >> i) & 0x1) << (4*(i-24) + 3);
  }

  counter += advancer;

  for (i = 0 ; i < 4 ; i++)
  {
    for (j = 0 ; j < 4 ; j++)
    {
      key[(j * 4) + i] = (MV_U8)((aesKey[j] >> (8 * (3 - i))) & 0xFF);
    }
  }
}

/*******************************************************************************
**
**  onuGponSrvcGenDbrTable
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function init onu DBR table
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK 
**
*******************************************************************************/
MV_STATUS onuGponSrvcGenDbrTable(void)
{
  MV_U32 index;

/*  DBR table for all block size supported - NOT sorted 
    ===================================================
    
    Config 	1/512	1/256	1/128	1/64	1/32	1/16	SUM	    1/SUM	    (1/SUM) * 4	 Block Size
    0	    0.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000      0.0000       0
    000001	0.0000	0.0000	0.0000	0.0000	0.0000	0.0625	0.0625	16.0000  	64.0000      64
    000010	0.0000	0.0000	0.0000	0.0000	0.0313	0.0000	0.0313	32.0000  	128.0000     128
    000011	0.0000	0.0000	0.0000	0.0000	0.0313	0.0625	0.0938	10.6667  	42.6667	     43
    000100	0.0000	0.0000	0.0000	0.0156	0.0000	0.0000	0.0156	64.0000  	256.0000     256
    000101	0.0000	0.0000	0.0000	0.0156	0.0000	0.0625	0.0781	12.8000  	51.2000      51
    000110	0.0000	0.0000	0.0000	0.0156	0.0313	0.0000	0.0469	21.3333  	85.3333      85
    000111	0.0000	0.0000	0.0000	0.0156	0.0313	0.0625	0.1094	9.1429      36.5714      37
    001000	0.0000	0.0000	0.0078	0.0000	0.0000	0.0000	0.0078	128.0000	512.0000     512
    001001	0.0000	0.0000	0.0078	0.0000	0.0000	0.0625	0.0703	14.2222  	56.8889	     57
    001010	0.0000	0.0000	0.0078	0.0000	0.0313	0.0000	0.0391	25.6000  	102.4000     102
    001011	0.0000	0.0000	0.0078	0.0000	0.0313	0.0625	0.1016	9.8462      39.3846	     39
    001100	0.0000	0.0000	0.0078	0.0156	0.0000	0.0000	0.0234	42.6667  	170.6667     171
    001101	0.0000	0.0000	0.0078	0.0156	0.0000	0.0625	0.0859	11.6364  	46.5455      47
    001110	0.0000	0.0000	0.0078	0.0156	0.0313	0.0000	0.0547	18.2857  	73.1429      73
    001111	0.0000	0.0000	0.0078	0.0156	0.0313	0.0625	0.1172	8.5333      34.1333      34
    010000	0.0000	0.0039	0.0000	0.0000	0.0000	0.0000	0.0039	256.0000	1024.0000    1024
    010001	0.0000	0.0039	0.0000	0.0000	0.0000	0.0625	0.0664	15.0588  	60.2353	     60
    010010	0.0000	0.0039	0.0000	0.0000	0.0313	0.0000	0.0352	28.4444  	113.7778     114
    010011	0.0000	0.0039	0.0000	0.0000	0.0313	0.0625	0.0977	10.2400  	40.9600	     41
    010100	0.0000	0.0039	0.0000	0.0156	0.0000	0.0000	0.0195	51.2000  	204.8000     205
    010101	0.0000	0.0039	0.0000	0.0156	0.0000	0.0625	0.0820	12.1905  	48.7619      49
    010110	0.0000	0.0039	0.0000	0.0156	0.0313	0.0000	0.0508	19.6923  	78.7692      79
    010111	0.0000	0.0039	0.0000	0.0156	0.0313	0.0625	0.1133	8.8276      35.3103      35
    011000	0.0000	0.0039	0.0078	0.0000	0.0000	0.0000	0.0117	85.3333  	341.3333     341
    011001	0.0000	0.0039	0.0078	0.0000	0.0000	0.0625	0.0742	13.4737  	53.8947      54
    011010	0.0000	0.0039	0.0078	0.0000	0.0313	0.0000	0.0430	23.2727  	93.0909      93
    011011	0.0000	0.0039	0.0078	0.0000	0.0313	0.0625	0.1055	9.4815      37.9259      38
    011100	0.0000	0.0039	0.0078	0.0156	0.0000	0.0000	0.0273	36.5714  	146.2857     146
    011101	0.0000	0.0039	0.0078	0.0156	0.0000	0.0625	0.0898	11.1304  	44.5217      45
    011110	0.0000	0.0039	0.0078	0.0156	0.0313	0.0000	0.0586	17.0667  	68.2667      68
    011111	0.0000	0.0039	0.0078	0.0156	0.0313	0.0625	0.1211	8.2581      33.0323      33
    100000	0.0020	0.0000	0.0000	0.0000	0.0000	0.0000	0.0020	512.0000	2048.0000    2048
    100001	0.0020	0.0000	0.0000	0.0000	0.0000	0.0625	0.0645	15.5152  	62.0606	     62
    100010	0.0020	0.0000	0.0000	0.0000	0.0313	0.0000	0.0332	30.1176  	120.4706     120
    100011	0.0020	0.0000	0.0000	0.0000	0.0313	0.0625	0.0957	10.4490  	41.7959      42
    100100	0.0020	0.0000	0.0000	0.0156	0.0000	0.0000	0.0176	56.8889  	227.5556     228
    100101	0.0020	0.0000	0.0000	0.0156	0.0000	0.0625	0.0801	12.4878  	49.9512      50
    100110	0.0020	0.0000	0.0000	0.0156	0.0313	0.0000	0.0488	20.4800  	81.9200      82
    100111	0.0020	0.0000	0.0000	0.0156	0.0313	0.0625	0.1113	8.9825      35.9298      36
    101000	0.0020	0.0000	0.0078	0.0000	0.0000	0.0000	0.0098	102.4000	409.6000     410
    101001	0.0020	0.0000	0.0078	0.0000	0.0000	0.0625	0.0723	13.8378  	55.3514      55
    101010	0.0020	0.0000	0.0078	0.0000	0.0313	0.0000	0.0410	24.3810  	97.5238      98
    101011	0.0020	0.0000	0.0078	0.0000	0.0313	0.0625	0.1035	9.6604      38.6415      39
    101100	0.0020	0.0000	0.0078	0.0156	0.0000	0.0000	0.0254	39.3846  	157.5385     158
    101101	0.0020	0.0000	0.0078	0.0156	0.0000	0.0625	0.0879	11.3778  	45.5111      46
    101110	0.0020	0.0000	0.0078	0.0156	0.0313	0.0000	0.0566	17.6552  	70.6207      71
    101111	0.0020	0.0000	0.0078	0.0156	0.0313	0.0625	0.1191	8.3934      33.5738      34
    110000	0.0020	0.0039	0.0000	0.0000	0.0000	0.0000	0.0059	170.6667	682.6667     683
    110001	0.0020	0.0039	0.0000	0.0000	0.0000	0.0625	0.0684	14.6286  	58.5143      59
    110010	0.0020	0.0039	0.0000	0.0000	0.0313	0.0000	0.0371	26.9474  	107.7895     108
    110011	0.0020	0.0039	0.0000	0.0000	0.0313	0.0625	0.0996	10.0392  	40.1569      40
    110100	0.0020	0.0039	0.0000	0.0156	0.0000	0.0000	0.0215	46.5455  	186.1818     186
    110101	0.0020	0.0039	0.0000	0.0156	0.0000	0.0625	0.0840	11.9070  	47.6279      48
    110110	0.0020	0.0039	0.0000	0.0156	0.0313	0.0000	0.0527	18.9630  	75.8519      76
    110111	0.0020	0.0039	0.0000	0.0156	0.0313	0.0625	0.1152	8.6780      34.7119      35
    111000	0.0020	0.0039	0.0078	0.0000	0.0000	0.0000	0.0137	73.1429  	292.5714     293
    111001	0.0020	0.0039	0.0078	0.0000	0.0000	0.0625	0.0762	13.1282  	52.5128      53
    111010	0.0020	0.0039	0.0078	0.0000	0.0313	0.0000	0.0449	22.2609  	89.0435      89
    111011	0.0020	0.0039	0.0078	0.0000	0.0313	0.0625	0.1074	9.3091      37.2364      37
    111100	0.0020	0.0039	0.0078	0.0156	0.0000	0.0000	0.0293	34.1333  	136.5333     137
    111101	0.0020	0.0039	0.0078	0.0156	0.0000	0.0625	0.0918	10.8936  	43.5745      44
    111110	0.0020	0.0039	0.0078	0.0156	0.0313	0.0000	0.0605	16.5161  	66.0645      66
    111111	0.0020	0.0039	0.0078	0.0156	0.0313	0.0625	0.1230	8.1270      32.5079      33
    
    DBR table for all block size supported - Sorted by Block Size
    =============================================================
    
    Config	1/512	1/256	1/128	1/64	1/32	1/16	SUM	    1/SUM	  (1/SUM) * 4	Block Size
    0	    0.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000	0.0000    0.0000        0
    111111	0.0020	0.0039	0.0078	0.0156	0.0313	0.0625	0.1230	8.1270    32.5079    	33
    011111	0.0000	0.0039	0.0078	0.0156	0.0313	0.0625	0.1211	8.2581    33.0323    	33
    101111	0.0020	0.0000	0.0078	0.0156	0.0313	0.0625	0.1191	8.3934    33.5738    	34
    001111	0.0000	0.0000	0.0078	0.0156	0.0313	0.0625	0.1172	8.5333    34.1333    	34
    110111	0.0020	0.0039	0.0000	0.0156	0.0313	0.0625	0.1152	8.6780    34.7119    	35
    010111	0.0000	0.0039	0.0000	0.0156	0.0313	0.0625	0.1133	8.8276    35.3103    	35
    100111	0.0020	0.0000	0.0000	0.0156	0.0313	0.0625	0.1113	8.9825    35.9298    	36
    000111	0.0000	0.0000	0.0000	0.0156	0.0313	0.0625	0.1094	9.1429    36.5714    	37
    111011	0.0020	0.0039	0.0078	0.0000	0.0313	0.0625	0.1074	9.3091    37.2364    	37
    011011	0.0000	0.0039	0.0078	0.0000	0.0313	0.0625	0.1055	9.4815    37.9259    	38
    101011	0.0020	0.0000	0.0078	0.0000	0.0313	0.0625	0.1035	9.6604    38.6415    	39
    001011	0.0000	0.0000	0.0078	0.0000	0.0313	0.0625	0.1016	9.8462    39.3846    	39
    110011	0.0020	0.0039	0.0000	0.0000	0.0313	0.0625	0.0996	10.0392 	40.1569    	40
    010011	0.0000	0.0039	0.0000	0.0000	0.0313	0.0625	0.0977	10.2400 	40.9600    	41
    100011	0.0020	0.0000	0.0000	0.0000	0.0313	0.0625	0.0957	10.4490 	41.7959    	42
    000011	0.0000	0.0000	0.0000	0.0000	0.0313	0.0625	0.0938	10.6667 	42.6667    	43
    111101	0.0020	0.0039	0.0078	0.0156	0.0000	0.0625	0.0918	10.8936 	43.5745    	44
    011101	0.0000	0.0039	0.0078	0.0156	0.0000	0.0625	0.0898	11.1304 	44.5217    	45
    101101	0.0020	0.0000	0.0078	0.0156	0.0000	0.0625	0.0879	11.3778 	45.5111    	46
    001101	0.0000	0.0000	0.0078	0.0156	0.0000	0.0625	0.0859	11.6364 	46.5455    	47
    110101	0.0020	0.0039	0.0000	0.0156	0.0000	0.0625	0.0840	11.9070 	47.6279    	48
    010101	0.0000	0.0039	0.0000	0.0156	0.0000	0.0625	0.0820	12.1905 	48.7619    	49
    100101	0.0020	0.0000	0.0000	0.0156	0.0000	0.0625	0.0801	12.4878 	49.9512    	50
    000101	0.0000	0.0000	0.0000	0.0156	0.0000	0.0625	0.0781	12.8000 	51.2000    	51
    111001	0.0020	0.0039	0.0078	0.0000	0.0000	0.0625	0.0762	13.1282 	52.5128    	53
    011001	0.0000	0.0039	0.0078	0.0000	0.0000	0.0625	0.0742	13.4737 	53.8947    	54
    101001	0.0020	0.0000	0.0078	0.0000	0.0000	0.0625	0.0723	13.8378 	55.3514    	55
    001001	0.0000	0.0000	0.0078	0.0000	0.0000	0.0625	0.0703	14.2222 	56.8889    	57
    110001	0.0020	0.0039	0.0000	0.0000	0.0000	0.0625	0.0684	14.6286 	58.5143    	59
    010001	0.0000	0.0039	0.0000	0.0000	0.0000	0.0625	0.0664	15.0588 	60.2353    	60
    100001	0.0020	0.0000	0.0000	0.0000	0.0000	0.0625	0.0645	15.5152 	62.0606    	62
    000001	0.0000	0.0000	0.0000	0.0000	0.0000	0.0625	0.0625	16.0000 	64.0000    	64
    111110	0.0020	0.0039	0.0078	0.0156	0.0313	0.0000	0.0605	16.5161 	66.0645    	66
    011110	0.0000	0.0039	0.0078	0.0156	0.0313	0.0000	0.0586	17.0667 	68.2667    	68
    101110	0.0020	0.0000	0.0078	0.0156	0.0313	0.0000	0.0566	17.6552 	70.6207    	71
    001110	0.0000	0.0000	0.0078	0.0156	0.0313	0.0000	0.0547	18.2857 	73.1429    	73
    110110	0.0020	0.0039	0.0000	0.0156	0.0313	0.0000	0.0527	18.9630 	75.8519    	76
    010110	0.0000	0.0039	0.0000	0.0156	0.0313	0.0000	0.0508	19.6923 	78.7692    	79
    100110	0.0020	0.0000	0.0000	0.0156	0.0313	0.0000	0.0488	20.4800 	81.9200    	82
    000110	0.0000	0.0000	0.0000	0.0156	0.0313	0.0000	0.0469	21.3333 	85.3333    	85
    111010	0.0020	0.0039	0.0078	0.0000	0.0313	0.0000	0.0449	22.2609 	89.0435    	89
    011010	0.0000	0.0039	0.0078	0.0000	0.0313	0.0000	0.0430	23.2727 	93.0909    	93
    101010	0.0020	0.0000	0.0078	0.0000	0.0313	0.0000	0.0410	24.3810 	97.5238    	98
    001010	0.0000	0.0000	0.0078	0.0000	0.0313	0.0000	0.0391	25.6000 	102.4000    102
    110010	0.0020	0.0039	0.0000	0.0000	0.0313	0.0000	0.0371	26.9474 	107.7895    108
    010010	0.0000	0.0039	0.0000	0.0000	0.0313	0.0000	0.0352	28.4444 	113.7778    114
    100010	0.0020	0.0000	0.0000	0.0000	0.0313	0.0000	0.0332	30.1176 	120.4706    120
    000010	0.0000	0.0000	0.0000	0.0000	0.0313	0.0000	0.0313	32.0000 	128.0000    128
    111100	0.0020	0.0039	0.0078	0.0156	0.0000	0.0000	0.0293	34.1333 	136.5333    137
    011100	0.0000	0.0039	0.0078	0.0156	0.0000	0.0000	0.0273	36.5714 	146.2857    146
    101100	0.0020	0.0000	0.0078	0.0156	0.0000	0.0000	0.0254	39.3846 	157.5385    158
    001100	0.0000	0.0000	0.0078	0.0156	0.0000	0.0000	0.0234	42.6667 	170.6667    171
    110100	0.0020	0.0039	0.0000	0.0156	0.0000	0.0000	0.0215	46.5455 	186.1818    186
    010100	0.0000	0.0039	0.0000	0.0156	0.0000	0.0000	0.0195	51.2000 	204.8000    205
    100100	0.0020	0.0000	0.0000	0.0156	0.0000	0.0000	0.0176	56.8889 	227.5556    228
    000100	0.0000	0.0000	0.0000	0.0156	0.0000	0.0000	0.0156	64.0000 	256.0000    256
    111000	0.0020	0.0039	0.0078	0.0000	0.0000	0.0000	0.0137	73.1429 	292.5714    293
    011000	0.0000	0.0039	0.0078	0.0000	0.0000	0.0000	0.0117	85.3333 	341.3333    341
    101000	0.0020	0.0000	0.0078	0.0000	0.0000	0.0000	0.0098	102.4000	409.6000    410
    001000	0.0000	0.0000	0.0078	0.0000	0.0000	0.0000	0.0078	128.0000	512.0000    512
    110000	0.0020	0.0039	0.0000	0.0000	0.0000	0.0000	0.0059	170.6667	682.6667    683
    010000	0.0000	0.0039	0.0000	0.0000	0.0000	0.0000	0.0039	256.0000	1024.0000 	1024
    100000	0.0020	0.0000	0.0000	0.0000	0.0000	0.0000	0.0020	512.0000	2048.0000 	2048
*/

  for (index = 0; index < 34; index++)
  {
      gponDbrArray[index].bitMask   = 0x3F; /*0b111111;*/
      gponDbrArray[index].blockSize = 33;
  }
  gponDbrArray[ 34].bitMask   = 0x2F; /*0b101111;*/
  gponDbrArray[ 34].blockSize = 34;
  gponDbrArray[ 35].bitMask   = 0x37; /*0b110111;*/
  gponDbrArray[ 35].blockSize = 35; 
  gponDbrArray[ 36].bitMask   = 0x27; /*0b100111;*/
  gponDbrArray[ 36].blockSize = 36; 
  gponDbrArray[ 37].bitMask   = 0x07; /*0b000111;*/
  gponDbrArray[ 37].blockSize = 37; 
  gponDbrArray[ 38].bitMask   = 0x1B; /*0b011011;*/
  gponDbrArray[ 38].blockSize = 38; 
  gponDbrArray[ 39].bitMask   = 0x2B; /*0b101011;*/
  gponDbrArray[ 39].blockSize = 39; 
  gponDbrArray[ 40].bitMask   = 0x33; /*0b110011;*/
  gponDbrArray[ 40].blockSize = 40; 
  gponDbrArray[ 41].bitMask   = 0x13; /*0b010011;*/
  gponDbrArray[ 41].blockSize = 41; 
  gponDbrArray[ 42].bitMask   = 0x23; /*0b100011;*/
  gponDbrArray[ 42].blockSize = 42; 
  gponDbrArray[ 43].bitMask   = 0x03; /*0b000011;*/
  gponDbrArray[ 43].blockSize = 43; 
  gponDbrArray[ 44].bitMask   = 0x3D; /*0b111101;*/
  gponDbrArray[ 44].blockSize = 44; 
  gponDbrArray[ 45].bitMask   = 0x1D; /*0b011101;*/
  gponDbrArray[ 45].blockSize = 45; 
  gponDbrArray[ 46].bitMask   = 0x2D; //0b101101; 
  gponDbrArray[ 46].blockSize = 46; 
  gponDbrArray[ 47].bitMask   = 0x0D; //0b001101; 
  gponDbrArray[ 47].blockSize = 47; 
  gponDbrArray[ 48].bitMask   = 0x35; //0b110101; 
  gponDbrArray[ 48].blockSize = 48; 
  gponDbrArray[ 49].bitMask   = 0x15; //0b010101; 
  gponDbrArray[ 49].blockSize = 49; 
  gponDbrArray[ 50].bitMask   = 0x25; //0b100101; 
  gponDbrArray[ 50].blockSize = 50; 
  gponDbrArray[ 51].bitMask   = 0x05; //0b000101; 
  gponDbrArray[ 51].blockSize = 51; 
  gponDbrArray[ 52].bitMask   = 0x05; //0b000101; 
  gponDbrArray[ 52].blockSize = 51; 
  gponDbrArray[ 53].bitMask   = 0x39; //0b111001;
  gponDbrArray[ 53].blockSize = 53; 
  gponDbrArray[ 54].bitMask   = 0x19; //0b011001;
  gponDbrArray[ 54].blockSize = 54; 
  gponDbrArray[ 55].bitMask   = 0x29; //0b101001; 
  gponDbrArray[ 55].blockSize = 55; 
  gponDbrArray[ 56].bitMask   = 0x29; //0b101001; 
  gponDbrArray[ 56].blockSize = 55; 
  gponDbrArray[ 57].bitMask   = 0x09; //0b001001;
  gponDbrArray[ 57].blockSize = 57; 
  gponDbrArray[ 58].bitMask   = 0x09; //0b001001;
  gponDbrArray[ 58].blockSize = 57; 
  gponDbrArray[ 59].bitMask   = 0x31; //0b110001; 
  gponDbrArray[ 59].blockSize = 59; 
  gponDbrArray[ 60].bitMask   = 0x11; //0b010001; 
  gponDbrArray[ 60].blockSize = 60; 
  gponDbrArray[ 61].bitMask   = 0x11; //0b010001; 
  gponDbrArray[ 61].blockSize = 60; 
  gponDbrArray[ 62].bitMask   = 0x21; //0b100001; 
  gponDbrArray[ 62].blockSize = 62; 
  gponDbrArray[ 63].bitMask   = 0x21; //0b100001; 
  gponDbrArray[ 63].blockSize = 62; 
  gponDbrArray[ 64].bitMask   = 0x01; //0b000001; 
  gponDbrArray[ 64].blockSize = 64; 
  gponDbrArray[ 65].bitMask   = 0x01; //0b000001;  
  gponDbrArray[ 65].blockSize = 64;  
  gponDbrArray[ 66].bitMask   = 0x3E; //0b111110;  
  gponDbrArray[ 66].blockSize = 66;  
  gponDbrArray[ 67].bitMask   = 0x3E; //0b111110;  
  gponDbrArray[ 67].blockSize = 66;  
  gponDbrArray[ 68].bitMask   = 0x1E; //0b011110;  
  gponDbrArray[ 68].blockSize = 68;  
  gponDbrArray[ 69].bitMask   = 0x1E; //0b011110;
  gponDbrArray[ 69].blockSize = 68;
  gponDbrArray[ 70].bitMask   = 0x1E; //0b011110;
  gponDbrArray[ 70].blockSize = 68;
  gponDbrArray[ 71].bitMask   = 0x2E; //0b101110;
  gponDbrArray[ 71].blockSize = 71;
  gponDbrArray[ 72].bitMask   = 0x2E; //0b101110;
  gponDbrArray[ 72].blockSize = 71;

  for (index = 73; index < 76; index++)
  {
      gponDbrArray[index].bitMask   = 0x0E; //0b001110;
      gponDbrArray[index].blockSize = 73;
  }
  for (index = 76; index < 79; index++)
  {
      gponDbrArray[index].bitMask   = 0x36; //0b110110;
      gponDbrArray[index].blockSize = 76;
  }
  for (index = 79; index < 82; index++)
  {
      gponDbrArray[index].bitMask   = 0x16; //0b010110;
      gponDbrArray[index].blockSize = 79;
  }
  for (index = 82; index < 85; index++)
  {
      gponDbrArray[index].bitMask   = 0x26; //0b100110;
      gponDbrArray[index].blockSize = 82;
  }
  for (index = 85; index < 89; index++)
  {
      gponDbrArray[index].bitMask   = 0x06; //0b000110;
      gponDbrArray[index].blockSize = 85;
  }
  for (index = 89; index < 93; index++)
  {
      gponDbrArray[index].bitMask   = 0x3A; //0b111010;
      gponDbrArray[index].blockSize = 89;
  }
  for (index = 93; index < 98; index++)
  {
      gponDbrArray[index].bitMask   = 0x1A; //0b011010;
      gponDbrArray[index].blockSize = 93;
  }
  for (index = 98; index < 102; index++)
  {
      gponDbrArray[index].bitMask   = 0x2A; //0b101010;
      gponDbrArray[index].blockSize = 98;
  }
  for (index = 102; index < 108; index++)
  {
      gponDbrArray[index].bitMask   = 0x0A; //0b001010;
      gponDbrArray[index].blockSize = 102;
  }
  for (index = 108; index < 114; index++)
  {
      gponDbrArray[index].bitMask   = 0x32; //0b110010;
      gponDbrArray[index].blockSize = 108;
  }
  for (index = 114; index < 120; index++)
  {
      gponDbrArray[index].bitMask   = 0x12; //0b010010;
      gponDbrArray[index].blockSize = 114;
  }
  for (index = 120; index < 128; index++)
  {
      gponDbrArray[index].bitMask   = 0x22; //0b100010;
      gponDbrArray[index].blockSize = 120;
  }
  for (index = 128; index < 137; index++)
  {
      gponDbrArray[index].bitMask   = 0x02; //0b000010;
      gponDbrArray[index].blockSize = 128;
  }
  for (index = 137; index < 146; index++)
  {
      gponDbrArray[index].bitMask   = 0x3C; //0b111100;
      gponDbrArray[index].blockSize = 137;
  }
  for (index = 137; index < 146; index++)
  {
      gponDbrArray[index].bitMask   = 0x3C; //0b111100;
      gponDbrArray[index].blockSize = 137;
  }
  for (index = 146; index < 158; index++)
  {
      gponDbrArray[index].bitMask   = 0x1C; //0b011100;
      gponDbrArray[index].blockSize = 146;
  }
  for (index = 158; index < 171; index++)
  {
      gponDbrArray[index].bitMask   = 0x2C; //0b101100;
      gponDbrArray[index].blockSize = 158;
  }
  for (index = 171; index < 186; index++)
  {
      gponDbrArray[index].bitMask   = 0x0C; //0b001100;
      gponDbrArray[index].blockSize = 171;
  }
  for (index = 186; index < 205; index++)
  {
      gponDbrArray[index].bitMask   = 0x34; //0b110100;
      gponDbrArray[index].blockSize = 186;
  }
  for (index = 205; index < 228; index++)
  {
      gponDbrArray[index].bitMask   = 0x14; //0b010100;
      gponDbrArray[index].blockSize = 205;
  }
  for (index = 228; index < 256; index++)
  {
      gponDbrArray[index].bitMask   = 0x24; //0b100100;
      gponDbrArray[index].blockSize = 228;
  }
  for (index = 256; index < 293; index++)
  {
      gponDbrArray[index].bitMask   = 0x04; //0b000100;
      gponDbrArray[index].blockSize = 256;
  }
  for (index = 293; index < 341; index++)
  {
      gponDbrArray[index].bitMask   = 0x38; //0b111000;
      gponDbrArray[index].blockSize = 293;
  }
  for (index = 341; index < 410; index++)
  {
      gponDbrArray[index].bitMask   = 0x18; //0b011000;
      gponDbrArray[index].blockSize = 341;
  }
  for (index = 410; index < 512; index++)
  {
      gponDbrArray[index].bitMask   = 0x28; //0b101000;
      gponDbrArray[index].blockSize = 410;
  }
  for (index = 512; index < 683; index++)
  {
      gponDbrArray[index].bitMask   = 0x08; //0b001000;
      gponDbrArray[index].blockSize = 512;
  }
  for (index = 683; index < 1024; index++)
  {
      gponDbrArray[index].bitMask   = 0x30; //0b110000;
      gponDbrArray[index].blockSize = 683;
  }
  for (index = 1024; index < 2048; index++)
  {
      gponDbrArray[index].bitMask   = 0x10; //0b010000;
      gponDbrArray[index].blockSize = 1024;
  }

  gponDbrBlock2048AndMore = 0x20; //0b100000;

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponSrvcDbrBlockSizeSet
**  ____________________________________________________________________________
**
**  DESCRIPTION: The function configure DBR block size
**               
**  PARAMETERS:  MV_U32 blockSize 
**
**  OUTPUTS:     None
**
**  RETURNS:     None 
**
*******************************************************************************/
MV_STATUS onuGponSrvcDbrBlockSizeSet(MV_U32 blockSize, MV_U32 *actualBlockSize)
{
  MV_STATUS rcode;
  MV_U32    macBlockSizeBitMask;
  MV_U32    macBlockSizeValue;

  if (blockSize >= 2048)
  {
    macBlockSizeBitMask = gponDbrBlock2048AndMore;
    macBlockSizeValue   = 2048;
  }
  else
  {
    macBlockSizeBitMask = gponDbrArray[blockSize].bitMask;
    macBlockSizeValue   = gponDbrArray[blockSize].blockSize;
  }

  rcode = mvOnuGponMacTxDbrBlockSizeSet(macBlockSizeBitMask);
  if (rcode != MV_OK)
    return (rcode);

  *actualBlockSize = macBlockSizeValue;

  return (rcode);
}

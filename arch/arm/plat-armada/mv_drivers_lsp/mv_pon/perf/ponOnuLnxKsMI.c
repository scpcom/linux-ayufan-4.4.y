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
**  FILE        : ponOnuLnxKsMI.c                                            **
**                                                                           **
**  DESCRIPTION : This file implements ONU GPON Management Interface         **
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
#define __FILE_DESC__ "mv_pon/perf/ponOnuLnxKsMI.c"

/* Global Variables
------------------------------------------------------------------------------*/

/* Local Variables
------------------------------------------------------------------------------*/
static int      deviceOpen = 0;
S_PonModuleCdev ponDev;

/* udev class's */
struct class  *pon_udev_class;
struct device *pon_udev_dev;

struct fasync_struct *mvPonAsqueue;

/* Export Functions
------------------------------------------------------------------------------*/

/* Local Functions
------------------------------------------------------------------------------*/

/******************************************************************************/
/******************************************************************************/
/* ========================================================================== */
/* ========================================================================== */
/* ==                                                                      == */
/* ==           =========   =========   =========   ===       ==           == */                       
/* ==           =========   =========   =========   ====      ==           == */
/* ==           ==          ==     ==   ==     ==   == ==     ==           == */
/* ==           ==          ==     ==   ==     ==   ==  ==    ==           == */
/* ==           =========   =========   ==     ==   ==   ==   ==           == */
/* ==           =========   =========   ==     ==   ==    ==  ==           == */
/* ==           ==     ==   ==          ==     ==   ==     == ==           == */
/* ==           ==     ==   ==          ==     ==   ==      ====           == */
/* ==           =========   ==          =========   ==       ===           == */
/* ==           =========   ==          =========   ==        ==           == */
/* ==                                                                      == */
/* ========================================================================== */
/* ========================================================================== */
/******************************************************************************/
/******************************************************************************/

#ifdef CONFIG_MV_GPON_MODULE

/*******************************************************************************
**
**  onuGponMiInit
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function init onu 
**               
**  PARAMETERS:  MV_U32 alloc
**               MV_U32 tcont 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
MV_STATUS onuGponMiInit(S_GponIoctlInfo *ioctlInfo)
{
  return(onuPonStart(ioctlInfo));
}

/*******************************************************************************
**
**  onuGponMiInfoGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu info
**               
**  PARAMETERS:  S_IoctlInfo *onuInfo 
**                        
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR 
**                   
*******************************************************************************/
MV_STATUS onuGponMiInfoGet(S_GponIoctlInfo *onuInfo)
{
  onuInfo->onuState  = onuGponDbOnuStateGet();
  onuInfo->onuId     = onuGponDbOnuIdGet();
  onuInfo->omccPort  = onuGponDbOmccPortGet();
  onuInfo->omccValid = onuGponDbOmccValidGet();

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponMiAlarm
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu alarms
**               
**  PARAMETERS:  S_IoctlAlarm *ioctlAlarm
**                        
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR 
**                   
*******************************************************************************/
MV_STATUS onuGponMiAlarm(S_GponIoctlAlarm *ioctlAlarm)
{
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_LOS]  = onuGponAlarmGet(ONU_GPON_ALARM_LOS);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_LOF]  = onuGponAlarmGet(ONU_GPON_ALARM_LOF);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_LCDA] = onuGponAlarmGet(ONU_GPON_ALARM_LCDA);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_LCDG] = onuGponAlarmGet(ONU_GPON_ALARM_LCDG);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_SF]   = onuGponAlarmGet(ONU_GPON_ALARM_SF);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_SD]   = onuGponAlarmGet(ONU_GPON_ALARM_SD);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_TF]   = onuGponAlarmGet(ONU_GPON_ALARM_TF);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_SUF]  = onuGponAlarmGet(ONU_GPON_ALARM_SUF);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_MEM]  = onuGponAlarmGet(ONU_GPON_ALARM_MEM);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_DACT] = onuGponAlarmGet(ONU_GPON_ALARM_DACT);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_DIS]  = onuGponAlarmGet(ONU_GPON_ALARM_DIS);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_MIS]  = onuGponAlarmGet(ONU_GPON_ALARM_MIS);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_PEE]  = onuGponAlarmGet(ONU_GPON_ALARM_PEE);
  ioctlAlarm->alarmTbl[ONU_GPON_ALARM_RDI]  = onuGponAlarmGet(ONU_GPON_ALARM_RDI);

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponMiPm
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu pm info
**               
**  PARAMETERS:  S_IoctlPm *ioctlPm
**                        
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR 
**                   
*******************************************************************************/
MV_STATUS onuGponMiPm(S_GponIoctlPm *ioctlPm)
{
  MV_STATUS            status;
  S_GponIoctlPloamTxPm ploamTxPm;
  S_GponIoctlPloamRxPm ploamRxPm;

  switch (ioctlPm->section) 
  {
    case E_GPON_IOCTL_PM_PLOAM_RX:
      status  = onuGponApiPmRxPloamPmGet(&(ioctlPm->ploamRx), MV_TRUE);
      status |= onuGponApiAdvancedPloamsCounterGet(&ploamTxPm, &(ioctlPm->ploamRx), MV_TRUE);
      return(status);
      break;
    case E_GPON_IOCTL_PM_PLOAM_TX:
      status = onuGponApiAdvancedPloamsCounterGet(&(ioctlPm->ploamTx), &ploamRxPm, MV_TRUE);
      break;
    case E_GPON_IOCTL_PM_BW_MAP:  
      return(onuGponApiPmRxBwMapPmGet(&(ioctlPm->bwMap), MV_TRUE));
      break;
    case E_GPON_IOCTL_PM_FEC:     
      return(onuGponApiPmFecPmGet(&(ioctlPm->fec), MV_TRUE));
      break;
    case E_GPON_IOCTL_PM_GEM_RX:
      return(onuGponApiGemRxCounterGet(&(ioctlPm->gemRx), MV_TRUE));
      break;
    case E_GPON_IOCTL_PM_GEM_TX:
      return(onuGponApiGemTxCounterGet(&(ioctlPm->gemTx), MV_TRUE));
      break;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuGponMiTcontConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu tcont <--> alloc
**               
**  PARAMETERS:  MV_U32 alloc
**               MV_U32 tcont 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
MV_STATUS onuGponMiTcontConfig(MV_U32 alloc, MV_U32 tcont)
{
  return(onuGponApiTcontConfig(alloc, tcont));
}

/*******************************************************************************
**
**  onuGponMiResetTcontsConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function reset onu bw map table
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
MV_STATUS onuGponMiResetTcontsConfig(void)
{
  return(onuGponApiTcontsReset());
}

/*******************************************************************************
**
**  mvPonCdevIoctl
**  ___________________________________________________________________________
** 
**  DESCRIPTION: The function execute IO commands
**               
**  PARAMETERS:  struct inode *inode
**               struct file *filp   
**               unsigned int cmd
**               unsigned long arg   
**
**  OUTPUTS:     None
**
**  RETURNS:     (0)
**                   
*******************************************************************************/
int mvPonCdevIoctl(struct inode *inode, struct file *filp, unsigned int cmd,
                   unsigned long arg)
{
  MV_STATUS        status;
  S_GponIoctlData  ioctlData;
  S_GponIoctlInfo  ioctlInfo;
  S_GponIoctlAlarm ioctlAlarm;
  S_GponIoctlPm    ioctlPm;
  S_GponIoctlXvr   ioctlXvr;
  int              ret = -EINVAL;

  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId);   

  switch(cmd)
  {
    /* =========================== */
    /* ====== Init Section ======= */
    /* =========================== */

    /* ====== MVGPON_IOCTL_INIT ==================== */
    case MVGPON_IOCTL_INIT:
      if(copy_from_user(&ioctlInfo, (S_GponIoctlInfo*)arg, sizeof(S_GponIoctlInfo)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }
      status = onuGponMiInit(&ioctlInfo);
      if(status != MV_OK)
        goto ioctlErr;
      ret = 0;
      break;

    /* ====== MVGPON_IOCTL_BEN_INIT ================ */
    case MVGPON_IOCTL_BEN_INIT:
      if(copy_from_user(&ioctlXvr, (S_GponIoctlXvr*)arg, sizeof(S_GponIoctlXvr)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }
      status = mvOnuGponMacTxBurstEnParamSet(ioctlXvr.mask, 
                                             ioctlXvr.polarity, 
                                             ioctlXvr.delay, 
                                             ioctlXvr.enStop, 
                                             ioctlXvr.enStart);
      if(status != MV_OK)
        goto ioctlErr;
      ret = 0;
      break;

    /* =========================== */
    /* ====== Data Section ======= */
    /* =========================== */

    /* ====== MVGPON_IOCTL_DATA_TCONT_CONFIG ======= */
    case MVGPON_IOCTL_DATA_TCONT_CONFIG:
      if(copy_from_user(&ioctlData, (S_GponIoctlData*)arg, sizeof(S_GponIoctlData)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }
      status = onuGponMiTcontConfig(ioctlData.alloc, 
                                    ioctlData.tcont);
      if(status != MV_OK)
        goto ioctlErr;
      ret = 0;
      break;

    /* ====== MVGPON_IOCTL_DATA_TCONT_RESET ======== */
    case MVGPON_IOCTL_DATA_TCONT_RESET:
      status = onuGponMiResetTcontsConfig();
      if(status == MV_OK)
        ret = 0;
      break;
   
    /* =========================== */
    /* ====== Info Section ======= */
    /* =========================== */

    /* ====== MVGPON_IOCTL_INFO ==================== */
    case MVGPON_IOCTL_INFO:
      status = onuGponMiInfoGet(&ioctlInfo);
      if(status != MV_OK)
        goto ioctlErr;
      if(copy_to_user((S_GponIoctlInfo*)arg, &ioctlInfo, sizeof(S_GponIoctlInfo)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_to_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }
      ret = 0;
      break;

    /* ====== MVGPON_IOCTL_ALARM =================== */
    case MVGPON_IOCTL_ALARM:
      status = onuGponMiAlarm(&ioctlAlarm);
      if(status != MV_OK)
        goto ioctlErr;
      if(copy_to_user((S_GponIoctlAlarm*)arg, &ioctlAlarm, sizeof(S_GponIoctlAlarm)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_to_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }
      ret = 0;
      break;
   
      /* ====== MVGPON_IOCTL_PM ==================== */
    case MVGPON_IOCTL_PM:
      status = onuGponMiPm(&ioctlPm);
      if(status != MV_OK)
        goto ioctlErr;
      if(copy_to_user((S_GponIoctlPm*)arg, &ioctlPm, sizeof(S_GponIoctlPm)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_to_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }
      ret = 0;
      break;

    default:
      ret = -EINVAL;
  }

ioctlErr:

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId);   

  return(ret);
}

/*******************************************************************************
**
**  mvPonCdevFasync
**  ___________________________________________________________________________
** 
**  DESCRIPTION: The function execute notification to User space
**               
**  PARAMETERS:  struct inode *inode
**               struct file *filp   
**               unsigned int cmd
**               unsigned long arg   
**
**  OUTPUTS:     None
**
**  RETURNS:     (0)
**                   
*******************************************************************************/

/******************************************************************************
** USER SPACE CODE
** ===============
** void action(int signo, siginfo_t *si, void *data)
** {
**   printf(" -------------- Got the %d signal ------------\n", signo);
** } 
**
** int main(int argc, char *argv[])
** {
**   int fd, oflags, retval = 0;
**   unsigned int buffer;
**
**   struct sigaction sa = {0}, oa;
**   sa.sa_sigaction = action;
**   sa.sa_flags = SA_SIGINFO;
**   sigemptyset(&sa.sa_mask);
**
**   sigaction(SIGIO, &sa, &oa); 
**
**   // Open device file device for communication
**   fd = open("/dev/pon", O_RDWR);
**   if(fd < 0)
**   {
**     fprintf(stderr, "\nOpen Failed\n");
**     exit(EXIT_FAILURE);
**   }
**
**   // set this process as owner of device file
**   retval = fcntl(fd, F_SETOWN, getpid());
**   if(retval < 0)
**   {
**     printf("F_SETOWN fails \n");
**   }
**   
**   retval = 0;
**   // enable the gpon async notifications
**   oflags = fcntl(fd, F_GETFL);
**   retval = fcntl(fd, F_SETFL, oflags | FASYNC);
**   if(retval < 0)
**   {
**     printf("F_SETFL Fails \n");
**   } 
**
**   close(fd);
**   exit(EXIT_SUCCESS);
** }
*/

void onuPonMiNotifyCallback(MV_U32 onuState)
{
  if(mvPonAsqueue)
  {
    kill_fasync(&mvPonAsqueue, SIGIO, POLL_IN);

#ifdef MV_GPON_DEBUG_PRINT
    mvPonPrint(PON_PRINT_DEBUG, PON_API_MODULE,
               "DEBUG: (%s:%d) Notify value[%d]\n", __FILE_DESC__, __LINE__, onuState);
#endif /* MV_GPON_DEBUG_PRINT */
  }
}

int mvPonCdevFasync(int fd, struct file *filp, int mode)
{
  return(fasync_helper(fd, filp, mode, &mvPonAsqueue));
}

#else /* CONFIG_MV_EPON_MODULE */

/******************************************************************************/
/******************************************************************************/
/* ========================================================================== */
/* ========================================================================== */
/* ==                                                                      == */
/* ==           =========   =========   =========   ===       ==           == */                       
/* ==           =========   =========   =========   ====      ==           == */
/* ==           ==          ==     ==   ==     ==   == ==     ==           == */
/* ==           ==          ==     ==   ==     ==   ==  ==    ==           == */
/* ==           =========   =========   ==     ==   ==   ==   ==           == */
/* ==           =========   =========   ==     ==   ==    ==  ==           == */
/* ==           ==          ==          ==     ==   ==     == ==           == */
/* ==           ==          ==          ==     ==   ==      ====           == */
/* ==           =========   ==          =========   ==       ===           == */
/* ==           =========   ==          =========   ==        ==           == */
/* ==                                                                      == */
/* ========================================================================== */
/* ========================================================================== */
/******************************************************************************/
/******************************************************************************/

/*******************************************************************************
**
**  onuEponMiInfoGet
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu info
**               
**  PARAMETERS:  S_IoctlInfo *info 
**               MV_U32      macId
**                        
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR 
**                   
*******************************************************************************/
MV_STATUS onuEponMiInfoGet(S_EponIoctlInfo *info, MV_U32 macId)
{
  return(onuEponApiInformationGet(info, macId));
}

/*******************************************************************************
**
**  onuEponMiPm
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function return onu pm info
**               
**  PARAMETERS:  S_IoctlPm *ioctlPm
**               MV_U32    macId
**                        
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR 
**                   
*******************************************************************************/
MV_STATUS onuEponMiPm(S_EponIoctlPm *ioctlPm, MV_U32 macId)
{
  switch (ioctlPm->section) 
  {
    case E_EPON_IOCTL_PM_RX:
      return(onuEponApiRxPmGet((&(ioctlPm->rxCnt)), MV_TRUE, 0));
      break;
    case E_EPON_IOCTL_PM_TX:
      return(onuEponApiTxPmGet((&(ioctlPm->txCnt)), MV_TRUE, macId));
      break;
    case E_EPON_IOCTL_PM_SW:  
      return(onuEponApiSwPmGet((&(ioctlPm->swCnt)), MV_TRUE, macId));
      break;
    case E_EPON_IOCTL_PM_GPM:
      return(onuEponApiGpmPmGet((&(ioctlPm->gpmCnt)), MV_TRUE, macId));
      break;
  }

  return(MV_OK);
}

/*******************************************************************************
**
**  onuEponMiFecConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu FEC
**               
**  PARAMETERS:  MV_U32 rxGenFecEn
**               MV_U32 txGenFecEn 
**               MV_U32 txMacFecEn[8]
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
MV_STATUS onuEponMiFecConfig(MV_U32 rxGenFecEn, MV_U32 txGenFecEn, MV_U32 *txMacFecEn)
{
  return(onuEponApiFecConfig(rxGenFecEn, txGenFecEn, txMacFecEn));
}

/*******************************************************************************
**
**  onuEponMiEncConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu ENC
**               
**  PARAMETERS:  MV_U32 onuEncryptCfg 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
MV_STATUS onuEponMiEncConfig(MV_U32 onuEncryptCfg)
{
  MV_STATUS status;

  status  = onuEponApiEncryptionConfig(onuEncryptCfg);

  return(status);
}

/*******************************************************************************
**
**  onuEponMiEncConfig
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function configure onu ENC
**               
**  PARAMETERS:  MV_U32 encryptKeyIndex0
**               MV_U32 encryptKeyIndex1
**               MV_U32 macId 
**
**  OUTPUTS:     None
**
**  RETURNS:     None
**                   
*******************************************************************************/
MV_STATUS onuEponMiEncKey(MV_U32 encryptKeyIndex0, MV_U32 encryptKeyIndex1, MV_U32 macId)
{
  MV_STATUS status;

  status = onuEponApiEncryptionKeyConfig(encryptKeyIndex0, encryptKeyIndex1, macId);

  return(status);
}

/*******************************************************************************
**
**  mvPonCdevIoctl
**  ___________________________________________________________________________
** 
**  DESCRIPTION: The function execute IO commands
**               
**  PARAMETERS:  struct inode *inode
**               struct file *filp   
**               unsigned int cmd
**               unsigned long arg   
**
**  OUTPUTS:     None
**
**  RETURNS:     (0)
**                   
*******************************************************************************/
int mvPonCdevIoctl(struct inode *inode, struct file *filp, unsigned int cmd,
                   unsigned long arg)
{
  MV_STATUS       status;
  S_EponIoctlInit ioctlInit;
  S_EponIoctlInfo ioctlInfo;
  S_EponIoctlPm   ioctlPm;
  S_EponIoctlFec  ioctlFec;
  S_EponIoctlEnc  ioctlEnc;
  int             ret = -EINVAL;

  onuPonIrqLock(onuPonResourceTbl_s.onuPonIrqId);   

  switch(cmd)
  {
    /* ====== MVEPON_IOCTL_INIT ======= */
    case MVEPON_IOCTL_INIT:
      if(copy_from_user(&ioctlInit, (S_EponIoctlInit*)arg, sizeof(S_EponIoctlInit)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }

      mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                 "ERROR: EPON_IOCTL_INIT Not Implemented!!!!!!\n");

      ret = 0;
      break;

    /* ====== MVEPON_IOCTL_FEC_CONFIG ======= */
    case MVEPON_IOCTL_FEC_CONFIG:
      if(copy_from_user(&ioctlFec, (S_EponIoctlFec*)arg, sizeof(S_EponIoctlFec)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }

      status = onuEponMiFecConfig(ioctlFec.rxGenFecEn, 
                                  ioctlFec.txGenFecEn,
                                  ioctlFec.txMacFecEn);
      if(status != MV_OK)
        goto ioctlErr;
      ret = 0;
      break;

    /* ====== MVEPON_IOCTL_ENC_CONFIG ======== */
    case MVEPON_IOCTL_ENC_CONFIG:
      if(copy_from_user(&ioctlEnc, (S_EponIoctlEnc*)arg, sizeof(S_EponIoctlEnc)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }

      status = onuEponMiEncConfig(ioctlEnc.encEnable);
      if(status != MV_OK)
        goto ioctlErr;
      ret = 0;
      break;

    /* ====== MVEPON_IOCTL_ENC_KEY ======== */
    case MVEPON_IOCTL_ENC_KEY:
      if(copy_from_user(&ioctlEnc, (S_EponIoctlEnc*)arg, sizeof(S_EponIoctlEnc)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }

      status = onuEponMiEncKey(ioctlEnc.encKeyIndex0,
                               ioctlEnc.encKeyIndex1,
                               ioctlEnc.macId);
      if(status != MV_OK)
        goto ioctlErr;
      ret = 0;
      break;

    /* ====== MVEPON_IOCTL_INFO ==================== */
    case MVEPON_IOCTL_INFO:
      if(copy_from_user(&ioctlInfo, (S_EponIoctlInfo*)arg, sizeof(S_EponIoctlInfo)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }

      status = onuEponMiInfoGet(&ioctlInfo, ioctlInfo.macId);
      if(status != MV_OK)
        goto ioctlErr;

      if(copy_to_user((S_EponIoctlInfo*)arg, &ioctlInfo, sizeof(S_EponIoctlInfo)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_to_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }
      ret = 0;
      break;

    /* ====== MVEPON_IOCTL_PM ==================== */
    case MVEPON_IOCTL_PM:
      if(copy_from_user(&ioctlPm, (S_EponIoctlPm*)arg, sizeof(S_EponIoctlPm)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_from_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }

      status = onuEponMiPm(&ioctlPm, ioctlPm.macId);
      if(status != MV_OK)
        goto ioctlErr;

      if(copy_to_user((S_EponIoctlPm*)arg, &ioctlPm, sizeof(S_EponIoctlPm)))
      {
        mvPonPrint(PON_PRINT_ERROR, PON_API_MODULE,
                   "ERROR: (%s:%d) copy_to_user failed\n", __FILE_DESC__, __LINE__);
        goto ioctlErr;
      }
      ret = 0;
      break;

    default:
      ret = -EINVAL;
  }

ioctlErr:

  onuPonIrqUnlock(onuPonResourceTbl_s.onuPonIrqId);   

  return(ret);
}

/*******************************************************************************
**
**  mvPonCdevFasync
**  ___________________________________________________________________________
** 
**  DESCRIPTION: The function execute notification to User space
**               
**  PARAMETERS:  struct inode *inode
**               struct file *filp   
**               unsigned int cmd
**               unsigned long arg   
**
**  OUTPUTS:     None
**
**  RETURNS:     (0)
**                   
*******************************************************************************/

/******************************************************************************
** USER SPACE CODE
** ===============
** void action(int signo, siginfo_t *si, void *data)
** {
**   printf(" -------------- Got the %d signal ------------\n", signo);
** } 
**
** int main(int argc, char *argv[])
** {
**   int fd, oflags, retval = 0;
**   unsigned int buffer;
**
**   struct sigaction sa = {0}, oa;
**   sa.sa_sigaction = action;
**   sa.sa_flags = SA_SIGINFO;
**   sigemptyset(&sa.sa_mask);
**
**   sigaction(SIGIO, &sa, &oa); 
**
**   // Open device file device for communication
**   fd = open("/dev/gpon", O_RDWR);
**   if(fd < 0)
**   {
**     fprintf(stderr, "\nOpen Failed\n");
**     exit(EXIT_FAILURE);
**   }
**
**   // set this process as owner of device file
**   retval = fcntl(fd, F_SETOWN, getpid());
**   if(retval < 0)
**   {
**     printf("F_SETOWN fails \n");
**   }
**   
**   retval = 0;
**   // enable the gpon async notifications
**   oflags = fcntl(fd, F_GETFL);
**   retval = fcntl(fd, F_SETFL, oflags | FASYNC);
**   if(retval < 0)
**   {
**     printf("F_SETFL Fails \n");
**   } 
**
**   close(fd);
**   exit(EXIT_SUCCESS);
** }
*/
void onuPonMiNotifyCallback(MV_U32 link)
{
  if(mvPonAsqueue)
  {
    kill_fasync(&mvPonAsqueue, SIGIO, POLL_IN);
  }
}

int mvPonCdevFasync(int fd, struct file *filp, int mode)
{
  return(fasync_helper(fd, filp, mode, &mvPonAsqueue));
}

#endif /* CONFIG_MV_GPON_MODULE */

/*******************************************************************************
**
**  mvGponCdevOpen
**  ___________________________________________________________________________
** 
**  DESCRIPTION: The function opens the GPON Char device
**               
**  PARAMETERS:  struct inode *inode
**               struct file *filp     
**
**  OUTPUTS:     None
**
**  RETURNS:     (0)
**                   
*******************************************************************************/
int mvPonCdevOpen(struct inode *inode, struct file *filp)
{
  S_PonModuleCdev *dev;

  if (deviceOpen)
    return(-EBUSY);

  deviceOpen++;
  try_module_get(THIS_MODULE);

  /* find the device structure */
  dev = container_of(inode->i_cdev, S_PonModuleCdev, cdev);
  filp->private_data = dev;

  return(0);
}

/*******************************************************************************
**
**  mvGponCdevRelease
**  ___________________________________________________________________________
** 
**  DESCRIPTION: The function releases the GPON Char device
**               
**  PARAMETERS:  struct inode *inode
**               struct file *filp     
**
**  OUTPUTS:     None
**
**  RETURNS:     (0)
**                   
*******************************************************************************/
int mvPonCdevRelease(struct inode *inode, struct file *filp)
{
  deviceOpen--;	
  module_put(THIS_MODULE);

  return(0);
}

/*******************************************************************************
**  PON device operations 
*******************************************************************************/
struct file_operations ponCdevFops = 
{
  .owner   = THIS_MODULE,
  .open    = mvPonCdevOpen,
  .release = mvPonCdevRelease,
  .ioctl   = mvPonCdevIoctl,
  .fasync  = mvPonCdevFasync
};

/*******************************************************************************
**
**  onuPonMngInterfaceCreate
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function create management interface - char device
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR
**                   
*******************************************************************************/
MV_STATUS onuPonMngInterfaceCreate(void)
{
  int   rcode;
  dev_t dev;

  dev   = MKDEV(MV_PON_MAJOR, 0);
  rcode = register_chrdev_region(dev, PON_NUM_DEVICES, PON_DEV_NAME);
  if(rcode < 0) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) Pon Char Device\n", __FILE_DESC__, __LINE__);
    return(MV_ERROR);
  }

  cdev_init(&ponDev.cdev, &ponCdevFops);
  ponDev.cdev.owner = THIS_MODULE;
  ponDev.cdev.ops   = &ponCdevFops;

  rcode = cdev_add(&ponDev.cdev, dev, 1);
  if(rcode < 0) 
  {
    mvPonPrint(PON_PRINT_ERROR, PON_INIT_MODULE,
               "ERROR: (%s:%d) Gpon Char Device Add\n", __FILE_DESC__, __LINE__);

    cdev_del(&ponDev.cdev);
	unregister_chrdev_region(dev, PON_NUM_DEVICES);

    return(MV_ERROR);
  }

  /* create device for udev */
  pon_udev_class = class_create(THIS_MODULE, PON_DEV_NAME);
  pon_udev_dev   = device_create(pon_udev_class, NULL, dev, NULL, PON_DEV_NAME);

  return(MV_OK); 
}

/*******************************************************************************
**
**  onuPonMngInterfaceRelease
**  ____________________________________________________________________________
** 
**  DESCRIPTION: The function release management interface - char device
**               
**  PARAMETERS:  None 
**
**  OUTPUTS:     None
**
**  RETURNS:     MV_OK or MV_ERROR
**                   
*******************************************************************************/
MV_STATUS onuPonMngInterfaceRelease(void)
{
  dev_t dev = MKDEV(MV_PON_MAJOR, 0);

  device_destroy(pon_udev_class, dev);
  class_unregister(pon_udev_class);
  class_destroy(pon_udev_class);
  
  unregister_chrdev_region(dev, PON_NUM_DEVICES);

  return(MV_OK);
}

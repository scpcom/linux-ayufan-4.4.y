#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "cntmr/mvCntmr.h"
#include "gpp/mvGpp.h"
#include "twsi/mvTwsi.h"
#include "pex/mvPex.h"
#include "device/mvDevice.h"
#include "eth/gbe/mvEthRegs.h"

#ifdef MV_DEBUG
	#define DB(x)	x
#else
	#define DB(x)
#endif

extern MV_CPU_ARM_CLK _cpuARMDDRCLK[];

#define CODE_IN_ROM		MV_FALSE
#define CODE_IN_RAM		MV_TRUE

extern	MV_BOARD_INFO*	boardInfoTbl[];
#define BOARD_INFO(boardId)	boardInfoTbl[boardId - BOARD_ID_BASE]

static MV_DEV_CS_INFO*  boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);

MV_U32 tClkRate   = -1;

MV_VOID mvBoardEnvInit(MV_VOID)
{
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardEnvInit:Board unknown.\n");
		return;

	}

#ifndef MY_ABC_HERE
	 
	MV_REG_WRITE(NAND_READ_PARAMS_REG, BOARD_INFO(boardId)->nandFlashReadParams);
	MV_REG_WRITE(NAND_WRITE_PARAMS_REG, BOARD_INFO(boardId)->nandFlashWriteParams);
	MV_REG_WRITE(NAND_CTRL_REG, BOARD_INFO(boardId)->nandFlashControl);

	MV_REG_WRITE(GPP_DATA_OUT_REG(0), BOARD_INFO(boardId)->gppOutValLow);
	MV_REG_WRITE(GPP_DATA_OUT_REG(1), BOARD_INFO(boardId)->gppOutValHigh);

	mvGppPolaritySet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValLow);
	mvGppPolaritySet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValHigh);

    if(mvCtrlRevGet()==MV_88F6XXX_A0_REV)
    {
        BOARD_INFO(boardId)->gppOutEnValLow &= 0xfffffffd;
        BOARD_INFO(boardId)->gppOutEnValLow |= (BOARD_INFO(boardId)->gppOutEnValHigh) & 0x00000002;
    }  

	mvGppTypeSet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValLow);
	mvGppTypeSet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValHigh);
#endif

	MV_REG_BIT_SET(NAND_CTRL_REG, NAND_ACTCEBOOT_BIT);
}

MV_U16 mvBoardModelGet(MV_VOID)
{
	return (mvBoardIdGet() >> 16);
}

MV_U16 mvBoardRevGet(MV_VOID)
{
	return (mvBoardIdGet() & 0xFFFF);
}

MV_STATUS mvBoardNameGet(char *pNameBuff)
{
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsSPrintf (pNameBuff, "Board unknown.\n");
		return MV_ERROR;

	}

	mvOsSPrintf (pNameBuff, "%s",BOARD_INFO(boardId)->boardName);

	return MV_OK;
}

MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum)
{
    MV_BOOL ethPortSgmiiSupport[BOARD_ETH_PORT_NUM] = MV_ETH_PORT_SGMII;

    if(ethPortNum >= BOARD_ETH_PORT_NUM)
    {
	    mvOsPrintf ("Invalid portNo=%d\n", ethPortNum);
		return MV_FALSE;
    }
    return ethPortSgmiiSupport[ethPortNum];
}

MV_BOOL mvBoardIsPortInGmii(MV_VOID)
{
	MV_U32 devClassId, devClass = 0;
	if (mvBoardMppGroupTypeGet(devClass) == MV_BOARD_AUTO)
	{
		 
		devClassId = mvBoarModuleTypeGet(devClass);
		if (MV_BOARD_MODULE_GMII_ID == devClassId)
			return MV_TRUE;
	}
	else if (mvBoardMppGroupTypeGet(devClass) == MV_BOARD_GMII)
		return MV_TRUE;

    return MV_FALSE;
}
 
MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum)
{
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardPhyAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr;
}

MV_BOARD_MAC_SPEED      mvBoardMacSpeedGet(MV_U32 ethPortNum)
{
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMacSpeedGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardMacSpeed;
}

MV_32	mvBoardLinkStatusIrqGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardLinkStatusIrqGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].linkStatusIrq;
}

MV_32	mvBoardSwitchPortGet(MV_U32 ethPortNum, MV_U8 boardPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardSwitchPortGet: Board unknown.\n");
		return MV_ERROR;
	}
	if (boardPortNum >= BOARD_ETH_SWITCH_PORT_NUM)
	{
		mvOsPrintf("mvBoardSwitchPortGet: Illegal board port number.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].qdPort[boardPortNum];
}

MV_32	mvBoardSwitchCpuPortGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardSwitchCpuPortGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].qdCpuPort;
}

MV_32	mvBoardIsSwitchConnected(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardIsSwitchConnected: Board unknown.\n");
		return MV_ERROR;
	}

	if(ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
	{
		mvOsPrintf("mvBoardIsSwitchConnected: Illegal port number(%u)\n", ethPortNum);
		return MV_ERROR;
	}
	
	if((MV_32)(BOARD_INFO(boardId)->pSwitchInfo))	
	return (MV_32)(BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].switchOnPort == ethPortNum);
	else
		return 0;
}
 
MV_32	mvBoardSmiScanModeGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardSmiScanModeGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pSwitchInfo[ethPortNum].smiScanMode;
}
 
MV_BOOL mvBoardSpecInitGet(MV_U32* regOff, MV_U32* data)
{
	return MV_FALSE;
}

MV_U32 mvBoardTclkGet(MV_VOID)
{
    if(mvCtrlModelGet()==MV_6281_DEV_ID || mvCtrlModelGet()==MV_6282_DEV_ID)
    {
#if defined(TCLK_AUTO_DETECT)
	MV_U32 tmpTClkRate = MV_BOARD_TCLK_166MHZ;

    tmpTClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET);
    tmpTClkRate &= MSAR_TCLCK_MASK;

    switch (tmpTClkRate)
    {
    case MSAR_TCLCK_166:
            return MV_BOARD_TCLK_166MHZ;
            break;
    case MSAR_TCLCK_200:
            return MV_BOARD_TCLK_200MHZ;
            break;
    }
#else
    return MV_BOARD_TCLK_200MHZ;
#endif
    }

        return MV_BOARD_TCLK_166MHZ;

}
 
static MV_U32  mvBoard6180SysClkGet(MV_VOID)
{
	MV_U32 	sysClkRate=0;
	MV_CPU_ARM_CLK _cpu6180_ddr_l2_CLK[] = MV_CPU6180_DDR_L2_CLCK_TBL;

	sysClkRate = MV_REG_READ(MPP_SAMPLE_AT_RESET);
	sysClkRate = sysClkRate & MSAR_CPUCLCK_MASK_6180;
	sysClkRate = sysClkRate >> MSAR_CPUCLCK_OFFS_6180;
			
	sysClkRate = _cpu6180_ddr_l2_CLK[sysClkRate].ddrClk;

	return sysClkRate;

}

MV_U32  mvBoardSysClkGet(MV_VOID)
{
#ifdef SYSCLK_AUTO_DETECT
	MV_U32 sysClkRate, tmp, pClkRate, indexDdrRtio;
	MV_U32 cpuCLK[] = MV_CPU_CLCK_TBL;
	MV_U32 ddrRtio[][2] = MV_DDR_CLCK_RTIO_TBL;

	if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
		return mvBoard6180SysClkGet();

	tmp = MV_REG_READ(MPP_SAMPLE_AT_RESET);
	pClkRate = MSAR_CPUCLCK_EXTRACT(tmp);
	pClkRate = cpuCLK[pClkRate];

	indexDdrRtio = tmp & MSAR_DDRCLCK_RTIO_MASK;
	indexDdrRtio = indexDdrRtio >> MSAR_DDRCLCK_RTIO_OFFS;
	if(ddrRtio[indexDdrRtio][0] != 0)
        	sysClkRate = ((pClkRate * ddrRtio[indexDdrRtio][1]) / ddrRtio[indexDdrRtio][0]);
	else
		sysClkRate = 0;

	return sysClkRate;
#else
	return MV_BOARD_DEFAULT_SYSCLK;
#endif
}
#ifdef MY_ABC_HERE
EXPORT_SYMBOL(mvBoardSysClkGet);
#endif

MV_U32 mvBoardPexBridgeIntPinGet(MV_U32 devNum, MV_U32 intPin)
{
	MV_U32 realIntPin = ((intPin + (3 - (devNum % 4))) %4 );

	if (realIntPin == 0) return 4;
		else return realIntPin;

}

MV_U32 mvBoardDebugLedNumGet(MV_U32 boardId)
{
	return BOARD_INFO(boardId)->activeLedsNumber;
}

MV_VOID mvBoardDebugLed(MV_U32 hexNum)
{
    MV_U32 val = 0,totalMask, currentBitMask = 1,i;
    MV_U32 boardId= mvBoardIdGet();

    if (BOARD_INFO(boardId)->pLedGppPin == NULL)
	return;

    totalMask = (1 << BOARD_INFO(boardId)->activeLedsNumber) -1;
    hexNum &= totalMask;
    totalMask = 0;

    for (i = 0 ; i < BOARD_INFO(boardId)->activeLedsNumber ; i++)
    {
	if (hexNum & currentBitMask)
	{
	    val |= (1 << BOARD_INFO(boardId)->pLedGppPin[i]);
	}

	totalMask |= (1 << BOARD_INFO(boardId)->pLedGppPin[i]);

	currentBitMask = (currentBitMask << 1);
    }

    if (BOARD_INFO(boardId)->ledsPolarity)
    {
	mvGppValueSet(0, totalMask, val);
    }
    else
    {
	mvGppValueSet(0, totalMask, ~val);
    }
}

MV_32 mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS class, MV_U32 index)
{
	MV_U32 boardId, i;
	MV_U32 indexFound = 0;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardRTCGpioPinGet:Board unknown.\n");
		return MV_ERROR;

	}

        for (i = 0; i < BOARD_INFO(boardId)->numBoardGppInfo; i++)
		if (BOARD_INFO(boardId)->pBoardGppInfo[i].devClass == class) {
			if (indexFound == index)
        			return (MV_U32)BOARD_INFO(boardId)->pBoardGppInfo[i].gppPinNum;
			else
				indexFound++;

		}

	return MV_ERROR;
}

MV_32 mvBoardRTCGpioPinGet(MV_VOID)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_RTC, 0);
}

MV_VOID	mvBoardReset(MV_VOID)
{
	MV_32 resetPin;

	resetPin = mvBoardResetGpioPinGet();
	if (resetPin != MV_ERROR)
	{
        	MV_REG_BIT_RESET( GPP_DATA_OUT_REG(0) ,(1 << resetPin));
		MV_REG_BIT_RESET( GPP_DATA_OUT_EN_REG(0) ,(1 << resetPin));

	}
	else
	{
	     
	    MV_REG_BIT_SET( CPU_RSTOUTN_MASK_REG , BIT2);
	    MV_REG_BIT_SET( CPU_SYS_SOFT_RST_REG , BIT0);
	}
}

MV_32 mvBoardResetGpioPinGet(MV_VOID)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_RESET, 0);
}
 
MV_32  mvBoardSDIOGpioPinGet(MV_VOID)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_SDIO_DETECT, 0);
}

MV_32 mvBoardUSBVbusGpioPinGet(MV_32 devId)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_USB_VBUS, devId);
}

MV_32 mvBoardUSBVbusEnGpioPinGet(MV_32 devId)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_USB_VBUS_EN, devId);
}

MV_32 mvBoardGpioIntMaskLowGet(MV_VOID)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardGpioIntMaskGet:Board unknown.\n");
		return MV_ERROR;

	}

	return BOARD_INFO(boardId)->intsGppMaskLow;
}
MV_32 mvBoardGpioIntMaskHighGet(MV_VOID)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardGpioIntMaskGet:Board unknown.\n");
		return MV_ERROR;

	}

	return BOARD_INFO(boardId)->intsGppMaskHigh;
}

MV_32 mvBoardMppGet(MV_U32 mppGroupNum)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
		return MV_ERROR;

	}

	return BOARD_INFO(boardId)->pBoardMppConfigValue[0].mppGroup[mppGroupNum];
}

MV_VOID mvBoardMppGroupIdUpdate(MV_VOID)
{

	MV_BOARD_MPP_GROUP_CLASS devClass;
	MV_BOARD_MODULE_ID_CLASS devClassId;
	MV_BOARD_MPP_TYPE_CLASS mppGroupType;
	MV_U32 devId;
	MV_U32 maxMppGrp = 1;
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			maxMppGrp = MV_6281_MPP_MAX_MODULE;
			break;
        case MV_6282_DEV_ID:
            maxMppGrp = MV_6282_MPP_MAX_MODULE;
            break;
        case MV_6280_DEV_ID:
            maxMppGrp = MV_6280_MPP_MAX_MODULE;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			maxMppGrp = MV_6192_MPP_MAX_MODULE;
			break;
        case MV_6190_DEV_ID:
            maxMppGrp = MV_6190_MPP_MAX_MODULE;
            break;
		case MV_6180_DEV_ID:
			maxMppGrp = MV_6180_MPP_MAX_MODULE;
			break;		
	}

	for (devClass = 0; devClass < maxMppGrp; devClass++)
	{
		 
		if (mvBoardMppGroupTypeGet(devClass) == MV_BOARD_AUTO)
		{
			 
			devClassId = mvBoarModuleTypeGet(devClass);
			if (MV_ERROR != devClassId)
			{
				switch(devClassId)
				{
				case MV_BOARD_MODULE_TDM_ID:
				case MV_BOARD_MODULE_TDM_5CHAN_ID:
					mppGroupType = MV_BOARD_TDM;
					break;
				case MV_BOARD_MODULE_AUDIO_ID:
					mppGroupType = MV_BOARD_AUDIO;
					break;
				case MV_BOARD_MODULE_RGMII_ID:
					mppGroupType = MV_BOARD_RGMII;
					break;
				case MV_BOARD_MODULE_GMII_ID:
					mppGroupType = MV_BOARD_GMII;
					break;
				case MV_BOARD_MODULE_TS_ID:
					mppGroupType = MV_BOARD_TS;
					break;
				case MV_BOARD_MODULE_MII_ID:
					mppGroupType = MV_BOARD_MII;
					break;
				case MV_BOARD_MODULE_LCD_ID:
					mppGroupType = MV_BOARD_LCD;
					break;
				default:
					mppGroupType = MV_BOARD_OTHER;
					break;
				}
			}
			else
				 
				mppGroupType = MV_BOARD_OTHER;
			
			mvBoardMppGroupTypeSet(devClass, mppGroupType);
		}

		if ((mvBoardMppGroupTypeGet(devClass) == MV_BOARD_RGMII))
			MV_REG_BIT_SET(MPP_OUTPUT_DRIVE_REG,MPP_1_8_RGMII1_OUTPUT_DRIVE | MPP_1_8_RGMII0_OUTPUT_DRIVE);
        	else 
		{
			if ((mvBoardMppGroupTypeGet(devClass) == MV_BOARD_GMII))
        		{
				MV_REG_BIT_RESET(MPP_OUTPUT_DRIVE_REG, BIT7 | BIT15);
            			MV_REG_BIT_RESET(ETH_PORT_SERIAL_CTRL_1_REG(0),BIT3);
            			MV_REG_BIT_RESET(ETH_PORT_SERIAL_CTRL_1_REG(1),BIT3);
        		}
        		else if ((mvBoardMppGroupTypeGet(devClass) == MV_BOARD_MII))
        		{
				 
				MV_REG_BIT_RESET(MPP_OUTPUT_DRIVE_REG, BIT7 | BIT15);
				 
            			MV_REG_BIT_RESET(ETH_PORT_SERIAL_CTRL_1_REG(1),BIT3);
        		}
		}
	}

	devId = mvCtrlModelGet();
	mppGroupType = mvBoardMppGroupTypeGet(0);
	if ((devId == MV_6282_DEV_ID) && (mppGroupType == MV_BOARD_LCD))
	{
	     
	    MV_REG_BIT_SET(NAND_AUDIO_PIN_MUX, NAND_AUDIO_PIN_MUX_SELECT_AUDIO_MODE);

	    mvBoardMppGroupTypeSet(1, MV_BOARD_LCD);
	}
}

MV_BOARD_MPP_TYPE_CLASS mvBoardMppGroupTypeGet(MV_BOARD_MPP_GROUP_CLASS mppGroupClass)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
		return MV_ERROR;

	}
	
	if (mppGroupClass == MV_BOARD_MPP_GROUP_1)
		return BOARD_INFO(boardId)->pBoardMppTypeValue[0].boardMppGroup1;
	else
		return BOARD_INFO(boardId)->pBoardMppTypeValue[0].boardMppGroup2;
}

MV_VOID mvBoardMppGroupTypeSet(MV_BOARD_MPP_GROUP_CLASS mppGroupClass,
						MV_BOARD_MPP_TYPE_CLASS mppGroupType)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
	}

	if (mppGroupClass == MV_BOARD_MPP_GROUP_1)
		BOARD_INFO(boardId)->pBoardMppTypeValue[0].boardMppGroup1 = mppGroupType;
	else
		BOARD_INFO(boardId)->pBoardMppTypeValue[0].boardMppGroup2 = mppGroupType;

}

MV_VOID mvBoardMppMuxSet(MV_VOID)
{

	MV_BOARD_MPP_GROUP_CLASS devClass;
	MV_BOARD_MPP_TYPE_CLASS mppGroupType;
	MV_U32 devId;
	MV_U8 muxVal = 0xf;
	MV_U32 maxMppGrp = 1;
    MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			maxMppGrp = MV_6281_MPP_MAX_MODULE;
			break;
        case MV_6282_DEV_ID:
            maxMppGrp = MV_6282_MPP_MAX_MODULE;
            break;
        case MV_6280_DEV_ID:
            maxMppGrp = MV_6280_MPP_MAX_MODULE;
			break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			maxMppGrp = MV_6192_MPP_MAX_MODULE;
			break;
        case MV_6190_DEV_ID:
            maxMppGrp = MV_6190_MPP_MAX_MODULE;
            break;
		case MV_6180_DEV_ID:
			maxMppGrp = MV_6180_MPP_MAX_MODULE;
			break;		
	}

	for (devClass = 0; devClass < maxMppGrp; devClass++)
	{
		mppGroupType = mvBoardMppGroupTypeGet(devClass);

		switch(mppGroupType)
		{
			case MV_BOARD_TDM:
				muxVal &= ~(devClass ? (0x2 << (devClass * 2)):0x0);
				break;
			case MV_BOARD_AUDIO:
				 muxVal &= ~(devClass ? 0x7 : 0x0);  
				break;
			case MV_BOARD_TS:
				 muxVal &= ~(devClass ? (0x2 << (devClass * 2)):0x0);
				break;
			case MV_BOARD_LCD:
				 muxVal = 0x0;
				break;
			default:
				muxVal |= (devClass ? 0xf : 0);
				break;
		}
	}

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

    	DB(mvOsPrintf("Board: twsi exp set\n"));
    	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(MV_BOARD_MUX_I2C_ADDR_ENTRY);
    	twsiSlave.slaveAddr.type = mvBoardTwsiExpAddrTypeGet(MV_BOARD_MUX_I2C_ADDR_ENTRY);
    	twsiSlave.validOffset = MV_TRUE;
	 
    	twsiSlave.offset = 2;
    	twsiSlave.moreThen256 = MV_FALSE;

    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
    	{
    		DB(mvOsPrintf("Board: twsi exp out val fail\n"));
        	return;
    	}
    	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));
    	
    	twsiSlave.offset = 6;
	muxVal = 0;
	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
    	{
    		DB(mvOsPrintf("Board: twsi exp change to out fail\n"));
        	return;
    	}
    	DB(mvOsPrintf("Board: twsi exp change to out succeded\n"));
	
}

MV_VOID mvBoardTdmMppSet(MV_32 chType)
{

	MV_BOARD_MPP_GROUP_CLASS devClass;
	MV_BOARD_MPP_TYPE_CLASS mppGroupType;
	MV_U32 devId;
	MV_U8 muxVal = 1;
	MV_U8 muxValMask = 1;
	MV_U8 twsiVal;
	MV_U32 maxMppGrp = 1;
    	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	
	devId = mvCtrlModelGet();
	
	switch(devId)
	{
		case MV_6281_DEV_ID:
			maxMppGrp = MV_6281_MPP_MAX_MODULE;
			break;
        	case MV_6282_DEV_ID:
            		maxMppGrp = MV_6282_MPP_MAX_MODULE;
            		break;
        	case MV_6280_DEV_ID:
            		maxMppGrp = MV_6280_MPP_MAX_MODULE;
			break;
		case MV_6192_DEV_ID:
		case MV_6701_DEV_ID:
		case MV_6702_DEV_ID:
			maxMppGrp = MV_6192_MPP_MAX_MODULE;
			break;
        	case MV_6190_DEV_ID:
            		maxMppGrp = MV_6190_MPP_MAX_MODULE;
            		break;
		case MV_6180_DEV_ID:
			maxMppGrp = MV_6180_MPP_MAX_MODULE;
			break;		
	}

	for (devClass = 0; devClass < maxMppGrp; devClass++)
	{
		mppGroupType = mvBoardMppGroupTypeGet(devClass);
		if(mppGroupType == MV_BOARD_TDM)
			break;
	}

	if(devClass == maxMppGrp)
		return;		 

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

    	DB(mvOsPrintf("Board: twsi exp set\n"));
    	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(devClass);
    	twsiSlave.slaveAddr.type = ADDR7_BIT;
    	twsiSlave.validOffset = MV_TRUE;
	 
    	twsiSlave.offset = 3;
    	twsiSlave.moreThen256 = MV_FALSE;

	if(mvBoardIdGet() == RD_88F6281A_ID)
	{
		muxVal = 0xc;
		muxValMask = 0xf3;
	}

	mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        muxVal = (twsiVal & muxValMask) | muxVal;

    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
    	{
    		mvOsPrintf("Board(1): twsi exp out val fail\n");
        	return;
    	}
    	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));
    	
    	twsiSlave.offset = 7;
	muxVal = 0xfe;
	if(mvBoardIdGet() == RD_88F6281A_ID)
		muxVal = 0xf3;

	mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
	muxVal = (twsiVal & muxVal);

	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
    	{
    		mvOsPrintf("Board: twsi exp change to out fail\n");
        	return;
    	}
    	DB(mvOsPrintf("Board: twsi exp change to out succeded\n"));
	 
    	twsiSlave.offset = 3;
	muxVal = 0;
	muxValMask = 1;

	if(mvBoardIdGet() == RD_88F6281A_ID)
	{
		muxVal = 0x0;
		muxValMask = 0xf3;
	}

	mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        muxVal = (twsiVal & muxValMask) | muxVal;

    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
    	{
    		mvOsPrintf("Board(2): twsi exp out val fail\n");
        	return;
    	}
    	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));

	mvOsDelay(20);

    	twsiSlave.offset = 3;
	muxVal = 1;
	muxValMask = 1;

	if(mvBoardIdGet() == RD_88F6281A_ID)
	{
		muxVal = 0xc;
		muxValMask = 0xf3;
		if(chType)  
		{
			MV_REG_BIT_SET(GPP_DATA_OUT_REG(1), MV_GPP12);
			mvOsDelay(50);
			MV_REG_BIT_RESET(GPP_DATA_OUT_REG(1), MV_GPP12);
		}
		else  
		{
		    
		   MV_REG_WRITE(MPP_CONTROL_REG5, ((MV_REG_READ(MPP_CONTROL_REG5) & 0xFFF0FFFF)  | BIT17));	
		}	
	}

	mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        muxVal = (twsiVal & muxValMask) | muxVal;

    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &muxVal, 1) )
    	{
    		mvOsPrintf("Board: twsi exp out val fail\n");
        	return;
    	}

    	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));
}

MV_32 mvBoardTdmSpiModeGet(MV_VOID)
{
	switch(mvBoardIdGet())
	{
		case RD_88F6281A_ID:
			return DAISY_CHAIN_MODE;
		case DB_88F6281A_BP_ID:
		case RD_88F6192A_ID:
		case DB_88F6192A_BP_ID:
		case DB_88F6701A_BP_ID:
		case DB_88F6702A_BP_ID:
		case DB_88F6282A_BP_ID:
		case RD_88F6282A_ID:
#ifdef MY_ABC_HERE
		case SYNO_DS409_ID:
		case SYNO_DS109_ID:
		case SYNO_DS409slim_ID:
		case SYNO_DS211_ID:
		case SYNO_DS011_ID:
		case SYNO_DS411slim_ID:
		case SYNO_RS_6282_ID:
		case SYNO_DS411_ID:
		case SYNO_DS212_ID:
		case SYNO_6702_1BAY_ID:
		case SYNO_RS213_ID:
#endif
			 return DUAL_CHIP_SELECT_MODE;
		default:
			mvOsPrintf("%s: unknown boardId(0x%x)\n",__FUNCTION__, mvBoardIdGet());
			return DUAL_CHIP_SELECT_MODE;
	}
}

MV_VOID mvBoardMppModuleTypePrint(MV_VOID)
{

	MV_BOARD_MPP_GROUP_CLASS devClass;
	MV_BOARD_MPP_TYPE_CLASS mppGroupType;
	MV_U32 devId;
	MV_U32 maxMppGrp = 1;
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			maxMppGrp = MV_6281_MPP_MAX_MODULE;
			break;
        case MV_6282_DEV_ID:
            maxMppGrp = MV_6282_MPP_MAX_MODULE;
            break;
        case MV_6280_DEV_ID:
            maxMppGrp = MV_6280_MPP_MAX_MODULE;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			maxMppGrp = MV_6192_MPP_MAX_MODULE;
			break;
        case MV_6190_DEV_ID:
            maxMppGrp = MV_6190_MPP_MAX_MODULE;
            break;
		case MV_6180_DEV_ID:
			maxMppGrp = MV_6180_MPP_MAX_MODULE;
			break;		
	}

	for (devClass = 0; devClass < maxMppGrp; devClass++)
	{
		mppGroupType = mvBoardMppGroupTypeGet(devClass);

		switch(mppGroupType)
		{
			case MV_BOARD_TDM:
                if(devId != MV_6190_DEV_ID)
                    mvOsPrintf("Module %d is TDM\n", devClass);
				break;
			case MV_BOARD_AUDIO:
                if(devId != MV_6190_DEV_ID)
                    mvOsPrintf("Module %d is AUDIO\n", devClass);
				break;
            case MV_BOARD_RGMII:
                if(devId != MV_6190_DEV_ID)
                    mvOsPrintf("Module %d is RGMII\n", devClass);
				break;
			case MV_BOARD_GMII:
                if(devId != MV_6190_DEV_ID)
                    mvOsPrintf("Module %d is GMII\n", devClass);
				break;
			case MV_BOARD_TS:
                if(devId != MV_6190_DEV_ID)
                    mvOsPrintf("Module %d is TS\n", devClass);
				break;
			default:
				break;
		}
	}
}

MV_32 mvBoardGetDevicesNumber(MV_BOARD_DEV_CLASS devClass)
{
	MV_U32	foundIndex=0,devNum;
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("mvBoardGetDeviceNumber:Board unknown.\n");
		return 0xFFFFFFFF;

	}

	for (devNum = START_DEV_CS; devNum < BOARD_INFO(boardId)->numBoardDeviceIf; devNum++)
	{
		if (BOARD_INFO(boardId)->pDevCsInfo[devNum].devClass == devClass)
		{
			foundIndex++;
		}
	}

    return foundIndex;

}

MV_32 mvBoardGetDeviceBaseAddr(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO* devEntry;
	devEntry = boardGetDevEntry(devNum,devClass);
	if (devEntry != NULL)
	{
		return mvCpuIfTargetWinBaseLowGet(DEV_TO_TARGET(devEntry->deviceCS));

	}

	return 0xFFFFFFFF;
}

MV_32 mvBoardGetDeviceBusWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO* devEntry;

	devEntry = boardGetDevEntry(devNum,devClass);
	if (devEntry != NULL)
	{
		return 8; 
	}

	return 0xFFFFFFFF;

}

MV_32 mvBoardGetDeviceWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO* devEntry;
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum,devClass);
	if (devEntry != NULL)
		return devEntry->devWidth;

	return MV_ERROR;

}

MV_32 mvBoardGetDeviceWinSize(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO* devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum,devClass);
	if (devEntry != NULL)
	{
		return mvCpuIfTargetWinSizeGet(DEV_TO_TARGET(devEntry->deviceCS));
	}

	return 0xFFFFFFFF;
}

static MV_DEV_CS_INFO*  boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_U32	foundIndex=0,devIndex;
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("boardGetDevEntry: Board unknown.\n");
		return NULL;

	}

	for (devIndex = START_DEV_CS; devIndex < BOARD_INFO(boardId)->numBoardDeviceIf; devIndex++)
	{
		 
		if (BOARD_INFO(boardId)->pDevCsInfo[devIndex].devClass == devClass)
		{
            		if (foundIndex == devNum)
			{
				return &(BOARD_INFO(boardId)->pDevCsInfo[devIndex]);
			}
			foundIndex++;
		}
	}

	return NULL;
}

MV_U32 boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO* devEntry;
	MV_U32 boardId= mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE)&&(boardId < MV_MAX_BOARD_ID)))
	{
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;

	}

	devEntry = boardGetDevEntry(devNum,devClass);
	if (devEntry != NULL)
		return devEntry->deviceCS;

	return 0xFFFFFFFF;

}

MV_U8 mvBoardRtcTwsiAddrTypeGet()
{
	int i;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_TWSI_RTC)
			return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
	return (MV_ERROR);
}

MV_U8 mvBoardRtcTwsiAddrGet()
{
	int i;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_TWSI_RTC)
			return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
	return (0xFF);
}

MV_U8 mvBoardA2DTwsiAddrTypeGet()
{
	int i;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_TWSI_AUDIO_DEC)
			return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
	return (MV_ERROR);
}

MV_U8 mvBoardA2DTwsiAddrGet()
{
	int i;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_TWSI_AUDIO_DEC)
			return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
	return (0xFF);
}

MV_U8 mvBoardTwsiExpAddrTypeGet(MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_EXP)
		{
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
			else
				indexFound++;
		}

	return (MV_ERROR);
}

MV_U8 mvBoardTwsiExpAddrGet(MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_EXP)
		{
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
			else
				indexFound++;
		}

	return (0xFF);
}

MV_U8 mvBoardTwsiSatRAddrTypeGet(MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_SATR)
		{
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
			else
				indexFound++;
		}

	return (MV_ERROR);
}

MV_U8 mvBoardTwsiSatRAddrGet(MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId= mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++)
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == BOARD_DEV_TWSI_SATR)
		{
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
			else
				indexFound++;
		}

	return (0xFF);
}

MV_32 mvBoardNandWidthGet(void)
{
	MV_U32 devNum;
	MV_U32 devWidth;
	MV_U32 boardId= mvBoardIdGet();

	for (devNum = START_DEV_CS; devNum < BOARD_INFO(boardId)->numBoardDeviceIf; devNum++)
	{
		devWidth = mvBoardGetDeviceWidth(devNum, BOARD_DEV_NAND_FLASH);
		if (devWidth != MV_ERROR)
			return (devWidth / 8);
	}
		
	return MV_ERROR;
}

MV_U32 gBoardId = -1;

MV_U32 mvBoardIdGet(MV_VOID)
{
	MV_U32 tmpBoardId = -1;

	if(gBoardId == -1)
        {
		#if defined(DB_88F6281A)
		tmpBoardId = DB_88F6281A_BP_ID;
		#elif defined(DB_88F6282A)
		tmpBoardId = DB_88F6282A_BP_ID;
		#elif defined(DB_88F6280A)
		tmpBoardId = DB_88F6280A_BP_ID;
		#elif defined(RD_88F6281A)
		tmpBoardId = RD_88F6281A_ID;
		#elif defined(RD_88F6282A)
		tmpBoardId = RD_88F6282A_ID;
		#elif defined(DB_88F6192A)
		tmpBoardId = DB_88F6192A_BP_ID;
		#elif defined(DB_88F6701A)
		tmpBoardId = DB_88F6701A_BP_ID;
		#elif defined(DB_88F6702A)
		tmpBoardId = DB_88F6702A_BP_ID;
		#elif defined(DB_88F6190A)
		tmpBoardId = DB_88F6190A_BP_ID;
		#elif defined(RD_88F6192A)
		tmpBoardId = RD_88F6192A_ID;
		#elif defined(RD_88F6190A)
		tmpBoardId = RD_88F6190A_ID;
		#elif defined(DB_88F6180A)
		tmpBoardId = DB_88F6180A_BP_ID;
		#elif defined(RD_88F6281A_PCAC)
		tmpBoardId = RD_88F6281A_PCAC_ID;
		#elif defined(RD_88F6281A_SHEEVA_PLUG)
		tmpBoardId = SHEEVA_PLUG_ID;
		#elif defined(DB_CUSTOMER)
		tmpBoardId = DB_CUSTOMER_ID;
		#endif
		gBoardId = tmpBoardId;
	}

#ifndef MY_ABC_HERE
	switch (gBoardId) {
	case 0x13:  
	case 0x14:  
	case 0x15:  
		gBoardId = DB_88F6281A_BP_ID;
	case 0x17: case 0x18: case 0x19:   
		gBoardId = DB_88F6282A_BP_ID;
		break;
	default:
		break;
	}
#endif
	return gBoardId;
}

MV_BOARD_MODULE_ID_CLASS mvBoarModuleTypeGet(MV_BOARD_MPP_GROUP_CLASS devClass)
{
    	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
    	MV_U8 data;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

    	DB(mvOsPrintf("Board: Read MPP module ID\n"));
    	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(devClass);
    	twsiSlave.slaveAddr.type = mvBoardTwsiExpAddrTypeGet(devClass);
    	twsiSlave.validOffset = MV_TRUE;
	 
    	twsiSlave.offset = 0;
    	twsiSlave.moreThen256 = MV_FALSE;

    	if( MV_OK != mvTwsiRead (0, &twsiSlave, &data, 1) )
    	{
    		DB(mvOsPrintf("Board: Read MPP module ID fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: Read MPP module ID succeded\n"));
	
	return data;
}

MV_U8 mvBoarTwsiSatRGet(MV_U8 devNum, MV_U8 regNum)
{
    	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
    	MV_U8 data;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

    	DB(mvOsPrintf("Board: Read S@R device read\n"));
    	twsiSlave.slaveAddr.address = mvBoardTwsiSatRAddrGet(devNum);
    	twsiSlave.slaveAddr.type = mvBoardTwsiSatRAddrTypeGet(devNum);
    	twsiSlave.validOffset = MV_TRUE;
	 
    	twsiSlave.offset = regNum;
    	twsiSlave.moreThen256 = MV_FALSE;

    	if( MV_OK != mvTwsiRead (0, &twsiSlave, &data, 1) )
    	{
    		DB(mvOsPrintf("Board: Read S@R fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: Read S@R succeded\n"));
	
	return data;
}

MV_STATUS mvBoarTwsiSatRSet(MV_U8 devNum, MV_U8 regNum, MV_U8 regVal)
{
    	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	
	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

    	twsiSlave.slaveAddr.address = mvBoardTwsiSatRAddrGet(devNum);
    	twsiSlave.slaveAddr.type = mvBoardTwsiSatRAddrTypeGet(devNum);
    	twsiSlave.validOffset = MV_TRUE;
    	DB(mvOsPrintf("Board: Write S@R device addr %x, type %x, data %x\n", twsiSlave.slaveAddr.address,\
								twsiSlave.slaveAddr.type, regVal));
	 
    	twsiSlave.offset = regNum;
    	twsiSlave.moreThen256 = MV_FALSE;
    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &regVal, 1) )
    	{
    		DB(mvOsPrintf("Board: Write S@R fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: Write S@R succeded\n"));
	
	return MV_OK;
}

MV_32 mvBoardSlicGpioPinGet(MV_U32 slicNum)
{
	MV_U32 boardId;
	boardId = mvBoardIdGet();

	switch (boardId)
	{
	case DB_88F6281A_BP_ID:
	case DB_88F6282A_BP_ID:
	case RD_88F6281A_ID:
	case RD_88F6282A_ID:
#ifdef MY_ABC_HERE
	case SYNO_DS409_ID:
	case SYNO_DS109_ID:
	case SYNO_DS409slim_ID:
	case SYNO_DS211_ID:
	case SYNO_DS011_ID:
	case SYNO_DS411slim_ID:
	case SYNO_RS_6282_ID:
	case SYNO_DS411_ID:
	case SYNO_DS212_ID:
	case SYNO_6702_1BAY_ID:
	case SYNO_RS213_ID:
#endif
	default:
		return MV_ERROR;
		break;

	}
}

MV_STATUS mvBoardFanPowerControl(MV_BOOL mode)
{

	MV_U8 val = 1, twsiVal;
   	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	
	if(mvBoardIdGet() != RD_88F6281A_ID)
        return MV_ERROR;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

    	DB(mvOsPrintf("Board: twsi exp set\n"));
    	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(1);
    	twsiSlave.slaveAddr.type = ADDR7_BIT;
    	twsiSlave.validOffset = MV_TRUE;
	 
    	twsiSlave.offset = 3;
    	twsiSlave.moreThen256 = MV_FALSE;
        if(mode == MV_TRUE)
            val = 0x1;
        else
            val = 0;
        mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        val = (twsiVal & 0xfe) | val;

        if( MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1) )
    	{
    		DB(mvOsPrintf("Board: twsi exp out val fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));
    	
    	twsiSlave.offset = 7;
        mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        val = (twsiVal & 0xfe);
    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1) )
    	{
    		DB(mvOsPrintf("Board: twsi exp change to out fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: twsi exp change to out succeded\n"));
        return MV_OK;
}

MV_STATUS mvBoardHDDPowerControl(MV_BOOL mode)
{

	MV_U8 val = 1, twsiVal;
   	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	
	if(mvBoardIdGet() != RD_88F6281A_ID)
        return MV_ERROR;

	if(mvBoardIdGet() != RD_88F6282A_ID)
        return MV_ERROR;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

    	DB(mvOsPrintf("Board: twsi exp set\n"));
    	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(1);
    	twsiSlave.slaveAddr.type = ADDR7_BIT;
    	twsiSlave.validOffset = MV_TRUE;
	 
    	twsiSlave.offset = 3;
    	twsiSlave.moreThen256 = MV_FALSE;
        if(mode == MV_TRUE)
            val = 0x2;
        else
            val = 0;
        mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        val = (twsiVal & 0xfd) | val;
    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1) )
    	{
    		DB(mvOsPrintf("Board: twsi exp out val fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));
    	
    	twsiSlave.offset = 7;
        mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        val = (twsiVal & 0xfd);
    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1) )
    	{
    		DB(mvOsPrintf("Board: twsi exp change to out fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: twsi exp change to out succeded\n"));
        return MV_OK;
}

MV_STATUS mvBoardSDioWPControl(MV_BOOL mode)
{

	MV_U8 val = 1, twsiVal;
   	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	
	if(mvBoardIdGet() != RD_88F6281A_ID)
        return MV_ERROR;

	if(mvBoardIdGet() != RD_88F6282A_ID)
        return MV_ERROR;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

    	DB(mvOsPrintf("Board: twsi exp set\n"));
    	twsiSlave.slaveAddr.address = mvBoardTwsiExpAddrGet(0);
    	twsiSlave.slaveAddr.type = ADDR7_BIT;
    	twsiSlave.validOffset = MV_TRUE;
	 
    	twsiSlave.offset = 3;
    	twsiSlave.moreThen256 = MV_FALSE;
        if(mode == MV_TRUE)
            val = 0x10;
        else
            val = 0;
        mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        val = (twsiVal & 0xef) | val;
    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1) )
    	{
    		DB(mvOsPrintf("Board: twsi exp out val fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: twsi exp out val succeded\n"));
    	
    	twsiSlave.offset = 7;
        mvTwsiRead(0, &twsiSlave, &twsiVal, 1);
        val = (twsiVal & 0xef);
    	if( MV_OK != mvTwsiWrite (0, &twsiSlave, &val, 1) )
    	{
    		DB(mvOsPrintf("Board: twsi exp change to out fail\n"));
        	return MV_ERROR;
    	}
    	DB(mvOsPrintf("Board: twsi exp change to out succeded\n"));
        return MV_OK;
}

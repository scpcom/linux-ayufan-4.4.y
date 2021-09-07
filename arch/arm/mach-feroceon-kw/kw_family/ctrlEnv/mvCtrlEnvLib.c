#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"
#include "mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"

#if defined(MV_INCLUDE_PEX)
#include "pex/mvPex.h"
#include "ctrlEnv/sys/mvSysPex.h"
#endif

#if defined(MV_INCLUDE_GIG_ETH)
#include "ctrlEnv/sys/mvSysGbe.h"
#endif

#if defined(MV_INCLUDE_XOR)
#include "ctrlEnv/sys/mvSysXor.h"
#endif

#if defined(MV_INCLUDE_SATA)
#include "ctrlEnv/sys/mvSysSata.h"
#endif

#if defined(MV_INCLUDE_USB)
#include "ctrlEnv/sys/mvSysUsb.h"
#endif

#if defined(MV_INCLUDE_AUDIO)
#include "ctrlEnv/sys/mvSysAudio.h"
#endif

#if defined(MV_INCLUDE_CESA)
#include "ctrlEnv/sys/mvSysCesa.h"
#endif

#if defined(MV_INCLUDE_TS)
#include "ctrlEnv/sys/mvSysTs.h"
#endif

#ifdef MY_ABC_HERE
#include "gpp/mvGppRegs.h"
#endif

#ifdef MV_DEBUG
	#define DB(x)	x
#else
	#define DB(x)
#endif	

#ifdef MY_ABC_HERE
extern long g_internal_netif_num;
#endif

#ifdef MY_ABC_HERE
MV_STATUS SYNOMppCtrlRegWrite(MV_U32 mppPin, MV_U32 mppVal)
{
	MV_U32 origVal;
	MV_U32 mppGroup;

	if(49 < mppPin)
		return -EINVAL;
		
		mppGroup = mppPin / 8;
		mppVal &= 0x0F;

		origVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));

		origVal &= ~(0xF << ((mppPin % 8)*4));
		origVal |= (mppVal << ((mppPin % 8)*4));

		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), origVal);

		return MV_OK;
}
#endif
 
MV_STATUS mvCtrlEnvInit(MV_VOID)
{
    	MV_U32 mppGroup;
	MV_U32 devId;
	MV_U32 boardId;
	MV_U32 i;
	MV_U32 maxMppGrp = 1;
	MV_U32 mppVal = 0;
	MV_U32 bootVal = 0;
	MV_U32 mppGroupType = 0;
	MV_U32 mppGroup1[][3] = MPP_GROUP_1_TYPE;
	MV_U32 mppGroup2[][3] = MPP_GROUP_2_TYPE;

	devId = mvCtrlModelGet();
	boardId= mvBoardIdGet();

	switch(devId){
		case MV_6281_DEV_ID:
			maxMppGrp = MV_6281_MPP_MAX_GROUP;
			break;
        case MV_6282_DEV_ID:
            maxMppGrp = MV_6282_MPP_MAX_GROUP;
            break;
        case MV_6280_DEV_ID:
            maxMppGrp = MV_6280_MPP_MAX_GROUP;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			maxMppGrp = MV_6192_MPP_MAX_GROUP;
			break;
        case MV_6190_DEV_ID:
            maxMppGrp = MV_6190_MPP_MAX_GROUP;
            break;
		case MV_6180_DEV_ID:
			maxMppGrp = MV_6180_MPP_MAX_GROUP;
			break;		
	}
	
	for (mppGroup = 0; mppGroup < 3; mppGroup++)
	{
		mppVal = mvBoardMppGet(mppGroup);
		if (mppGroup == 0)
		{
		    bootVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));
		    if (mvCtrlIsBootFromSPI())
		    {
			mppVal &= ~0xffff;
			bootVal &= 0xffff;
			mppVal |= bootVal;
		    }
		    else if (mvCtrlIsBootFromSPIUseNAND())
		    {
			mppVal &= ~0xf0000000;
			bootVal &= 0xf0000000;
			mppVal |= bootVal;
		    }
		    else if (mvCtrlIsBootFromNAND())
		    {
			mppVal &= ~0xffffff;
			bootVal &= 0xffffff;
			mppVal |= bootVal;
		    }
		}
		
		if (mppGroup == 2)
		{
		    bootVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));
		    if (mvCtrlIsBootFromNAND())
		    {
			mppVal &= ~0xff00;
			bootVal &= 0xff00;
			mppVal |= bootVal;
		    }
		}

		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}

	mvBoardMppGroupIdUpdate();

	if ((boardId == DB_88F6281A_BP_ID) ||
        (boardId == DB_88F6282A_BP_ID) ||
#ifdef MY_ABC_HERE
		(boardId == SYNO_DS409_ID) ||
		(boardId == SYNO_DS109_ID) ||
		(boardId == SYNO_DS409slim_ID) ||
		(boardId == SYNO_DS211_ID) ||
		(boardId == SYNO_DS411slim_ID) ||
		(boardId == SYNO_RS_6282_ID) ||
		(boardId == SYNO_DS411_ID)||
		(boardId == SYNO_DS212_ID)||
		(boardId == SYNO_6702_1BAY_ID)||
		(boardId == SYNO_RS213_ID) ||
#endif
		(boardId == DB_88F6180A_BP_ID))
		mvBoardMppMuxSet();

	mppGroupType = mvBoardMppGroupTypeGet(MV_BOARD_MPP_GROUP_1);

    if (devId != MV_6180_DEV_ID && devId != MV_6280_DEV_ID)
    {
        i = 0;
    	for (mppGroup = 2; mppGroup < 5; mppGroup++)
    	{
    		if ((mppGroupType == MV_BOARD_OTHER) ||
    			(boardId == RD_88F6281A_ID) ||
    			(boardId == RD_88F6282A_ID) ||
    			(boardId == RD_88F6192A_ID) ||
                (boardId == RD_88F6190A_ID) ||
                (boardId == RD_88F6281A_PCAC_ID) ||
                (boardId == SHEEVA_PLUG_ID))
    			mppVal = mvBoardMppGet(mppGroup);
    		else
    		{
    			mppVal = mppGroup1[mppGroupType][i];
    			i++;
    		}
    
    		if (mppGroup == 2)
    		{
                bootVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));
    			mppVal &= ~0xffff;
    			bootVal &= 0xffff;
    			mppVal |= bootVal;
    		}
    
    		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
    	}
    }

	if ((devId == MV_6192_DEV_ID) || (devId == MV_6190_DEV_ID) || (devId == MV_6701_DEV_ID) || (devId == MV_6702_DEV_ID))
		return MV_OK;
 	
	mppGroupType = mvBoardMppGroupTypeGet(MV_BOARD_MPP_GROUP_2);
	 
	i = 0;
	for (mppGroup = 4; mppGroup < 7; mppGroup++)
	{
		if ((mppGroupType == MV_BOARD_OTHER) ||
			(boardId == RD_88F6281A_ID) ||
			(boardId == RD_88F6282A_ID) ||
            (boardId == RD_88F6281A_PCAC_ID) ||
            (boardId == SHEEVA_PLUG_ID))
			mppVal = mvBoardMppGet(mppGroup);
		else
		{
			mppVal = mppGroup2[mppGroupType][i];
			i++;
		}

		if (mppGroup == 4)
		{
            bootVal = MV_REG_READ(mvCtrlMppRegGet(mppGroup));
			mppVal &= ~0xffff;
			bootVal &= 0xffff;
			mppVal |= bootVal;
		}

		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}

    if(mvBoardIdGet() == DB_88F6281A_BP_ID || mvBoardIdGet() == DB_88F6282A_BP_ID || 
       mvBoardIdGet() == DB_88F6192A_BP_ID ||
       mvBoardIdGet() == DB_88F6701A_BP_ID ||
       mvBoardIdGet() == DB_88F6702A_BP_ID ||
#ifdef MY_ABC_HERE
	   mvBoardIdGet() == SYNO_DS211_ID ||
	   mvBoardIdGet() == SYNO_DS411slim_ID ||
	   mvBoardIdGet() == SYNO_RS_6282_ID ||
	   mvBoardIdGet() == SYNO_DS411_ID ||
	   mvBoardIdGet() == SYNO_DS212_ID ||
	   mvBoardIdGet() == SYNO_6702_1BAY_ID ||
	   mvBoardIdGet() == SYNO_RS213_ID ||
#endif
       mvBoardIdGet() == DB_88F6190A_BP_ID || mvBoardIdGet() == DB_88F6180A_BP_ID ||
       mvBoardIdGet() == DB_88F6280A_BP_ID)
        MV_REG_WRITE(0x100d8, 0x53);

	return MV_OK;
}

MV_U32 mvCtrlMppRegGet(MV_U32 mppGroup)
{
        MV_U32 ret;

        switch(mppGroup){
                case (0):       ret = MPP_CONTROL_REG0;
                                break;
                case (1):       ret = MPP_CONTROL_REG1;
                                break;
                case (2):       ret = MPP_CONTROL_REG2;
                                break;
                case (3):       ret = MPP_CONTROL_REG3;
                                break;
                case (4):       ret = MPP_CONTROL_REG4;
                                break;
                case (5):       ret = MPP_CONTROL_REG5;
                                break;
                case (6):       ret = MPP_CONTROL_REG6;
                                break;
                default:        ret = MPP_CONTROL_REG0;
                                break;
        }
        return ret;
}
#if defined(MV_INCLUDE_PEX) 
 
MV_U32 mvCtrlPexMaxIfGet(MV_VOID)
{
	MV_U32 devId;

	devId = mvCtrlModelGet();

	switch(devId){
        case MV_6280_DEV_ID:
            return MV_PEX_MAX_IF_6280;
            break;
        case MV_6282_DEV_ID:
            return MV_PEX_MAX_IF_6282;
            break;
        default:
            return MV_PEX_MAX_IF;
            break;
    }
}
#endif

#if defined(MV_INCLUDE_GIG_ETH)
 
MV_U32 mvCtrlEthMaxPortGet(MV_VOID)
{
	MV_U32 devId;

#ifdef MY_ABC_HERE
	if (g_internal_netif_num >= 0) {
		return g_internal_netif_num;
	}else{
		return 2;
	}
#endif
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			return MV_6281_ETH_MAX_PORTS;
			break;
        case MV_6282_DEV_ID:
            return MV_6282_ETH_MAX_PORTS;
            break;
        case MV_6280_DEV_ID:
            return MV_6280_ETH_MAX_PORTS;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			return MV_6192_ETH_MAX_PORTS;
			break;
        case MV_6190_DEV_ID:
            return MV_6190_ETH_MAX_PORTS;
            break;
		case MV_6180_DEV_ID:
			return MV_6180_ETH_MAX_PORTS;
			break;		
	}
	return 0;

}
#endif

#if defined(MV_INCLUDE_XOR)
 
MV_U32 mvCtrlXorMaxChanGet(MV_VOID)
{
	return MV_XOR_MAX_CHAN; 
}
#endif

#if defined(MV_INCLUDE_USB)
 
MV_U32 mvCtrlUsbMaxGet(void)
{
	return MV_USB_MAX_PORTS;
}
#endif

#if defined(MV_INCLUDE_NAND)
 
MV_U32	  mvCtrlNandSupport(MV_VOID)
{
	MV_U32 devId;
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			return MV_6281_NAND;
			break;
        case MV_6282_DEV_ID:
            return MV_6282_NAND;
            break;
        case MV_6280_DEV_ID:
            return MV_6280_NAND;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			return MV_6192_NAND;
			break;
        case MV_6190_DEV_ID:
            return MV_6190_NAND;
            break;
		case MV_6180_DEV_ID:
			return MV_6180_NAND;
			break;		
	}
	return 0;

}
#endif

#if defined(MV_INCLUDE_SDIO)
 
MV_U32	  mvCtrlSdioSupport(MV_VOID)
{
	MV_U32 devId;
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			return MV_6281_SDIO;
			break;
        case MV_6282_DEV_ID:
            return MV_6282_SDIO;
            break;
        case MV_6280_DEV_ID:
            return MV_6280_SDIO;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			return MV_6192_SDIO;
			break;
        case MV_6190_DEV_ID:
            return MV_6190_SDIO;
            break;
		case MV_6180_DEV_ID:
			return MV_6180_SDIO;
			break;		
	}
	return 0;

}
#endif

#if defined(MV_INCLUDE_TS)
 
MV_U32	  mvCtrlTsSupport(MV_VOID)
{
	MV_U32 devId;
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			return MV_6281_TS;
			break;
        case MV_6282_DEV_ID:
            return MV_6282_TS;
            break;
        case MV_6280_DEV_ID:
            return MV_6280_TS;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			return MV_6192_TS;
			break;
        case MV_6190_DEV_ID:
            return MV_6190_TS;
            break;
		case MV_6180_DEV_ID:
			return MV_6180_TS;
			break;		
	}
	return 0;
}
#endif

#if defined(MV_INCLUDE_AUDIO)
 
MV_U32	  mvCtrlAudioSupport(MV_VOID)
{
	MV_U32 devId;
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			return MV_6281_AUDIO;
			break;
        case MV_6282_DEV_ID:
            return MV_6282_AUDIO;
            break;
        case MV_6280_DEV_ID:
            return MV_6280_AUDIO;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			return MV_6192_AUDIO;
			break;
        case MV_6190_DEV_ID:
            return MV_6190_AUDIO;
            break;
		case MV_6180_DEV_ID:
			return MV_6180_AUDIO;
			break;		
	}
	return 0;

}
#endif

#if defined(MV_INCLUDE_TDM)
 
MV_U32	  mvCtrlTdmSupport(MV_VOID)
{
	MV_U32 devId;
	
	devId = mvCtrlModelGet();

	switch(devId){
		case MV_6281_DEV_ID:
			return MV_6281_TDM;
			break;
        case MV_6282_DEV_ID:
            return MV_6282_TDM;
            break;
        case MV_6280_DEV_ID:
            return MV_6280_TDM;
            break;
		case MV_6192_DEV_ID:
	case MV_6701_DEV_ID:
	case MV_6702_DEV_ID:
			return MV_6192_TDM;
			break;
        case MV_6190_DEV_ID:
            return MV_6190_TDM;
            break;
		case MV_6180_DEV_ID:
			return MV_6180_TDM;
			break;		
	}
	return 0;

}
#endif

MV_U16 mvCtrlModelGet(MV_VOID)
{
	MV_U32 devId;	
	MV_U16 model = 0;

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	 
	MV_U32 pexPower;
	pexPower = mvCtrlPwrClckGet(PEX_UNIT_ID,0);
	if (pexPower == MV_FALSE)
		mvCtrlPwrClckSet(PEX_UNIT_ID, 0, MV_TRUE);
#endif
	devId = MV_REG_READ(CHIP_BOND_REG);
	devId &= PCKG_OPT_MASK;

	switch(devId){
        case 2:
            if (((MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0,PEX_DEVICE_AND_VENDOR_ID))& 0xffff0000) >> 16)
                    == MV_6281_DEV_ID)
                model =	MV_6281_DEV_ID;
            else
                model =	MV_6282_DEV_ID;
        break;
        case 1:
            if (((MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0,PEX_DEVICE_AND_VENDOR_ID))& 0xffff0000) >> 16)
                    == MV_6701_DEV_ID)
                model =	MV_6701_DEV_ID;

            else if (((MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0,PEX_DEVICE_AND_VENDOR_ID))& 0xffff0000) >> 16)
                    == MV_6702_DEV_ID)
                model =	MV_6702_DEV_ID;
            else if (((MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0,PEX_DEVICE_AND_VENDOR_ID))& 0xffff0000) >> 16)
                    == MV_6190_DEV_ID)
                model =	MV_6190_DEV_ID;
            else
                model =	MV_6192_DEV_ID;
        break;
        case 0:
            if (((MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0,PEX_DEVICE_AND_VENDOR_ID))& 0xffff0000) >> 16)
                        == MV_6280_DEV_ID)
                model =	MV_6280_DEV_ID;
            else
                model =	MV_6180_DEV_ID;
        break;
	}

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	 
	if (pexPower == MV_FALSE)
		mvCtrlPwrClckSet(PEX_UNIT_ID, 0, MV_FALSE);
#endif

	return model;
}
 
MV_U8 mvCtrlRevGet(MV_VOID)
{
	MV_U8 revNum;
#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	 
	MV_U32 pexPower;
	pexPower = mvCtrlPwrClckGet(PEX_UNIT_ID,0);
	if (pexPower == MV_FALSE)
		mvCtrlPwrClckSet(PEX_UNIT_ID, 0, MV_TRUE);
#endif
	revNum = (MV_U8)MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0,PCI_CLASS_CODE_AND_REVISION_ID));
#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	 
	if (pexPower == MV_FALSE)
		mvCtrlPwrClckSet(PEX_UNIT_ID, 0, MV_FALSE);
#endif
	return ((revNum & PCCRIR_REVID_MASK) >> PCCRIR_REVID_OFFS);
}

MV_STATUS mvCtrlNameGet(char *pNameBuff)
{
	mvOsSPrintf (pNameBuff, "%s%x Rev %d", SOC_NAME_PREFIX, 
				mvCtrlModelGet(), mvCtrlRevGet()); 
	
	return MV_OK;
}

MV_U32	mvCtrlModelRevGet(MV_VOID)
{
	return ((mvCtrlModelGet() << 16) | mvCtrlRevGet());
}

MV_STATUS mvCtrlModelRevNameGet(char *pNameBuff)
{

        switch (mvCtrlModelRevGet())
        {
        case MV_6281_A0_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6281_A0_NAME); 
                break;
        case MV_6192_A0_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6192_A0_NAME); 
                break;
        case MV_6180_A0_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6180_A0_NAME); 
                break;
        case MV_6190_A0_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6190_A0_NAME); 
                break;
        case MV_6281_A1_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6281_A1_NAME);
                break;
        case MV_6192_A1_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6192_A1_NAME);
                break;
        case MV_6701_A1_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6701_A1_NAME);
                break;
        case MV_6702_A1_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6702_A1_NAME);
                break;
        case MV_6180_A1_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6180_A1_NAME);
                break;
        case MV_6190_A1_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6190_A1_NAME);
                break;
        case MV_6282_A0_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6282_A0_NAME); 
                break;
        case MV_6282_A1_ID:
                mvOsSPrintf (pNameBuff, "%s",MV_6282_A1_NAME); 
                break;
        default:
                mvCtrlNameGet(pNameBuff);
                break;
        }

        return MV_OK;
}

MV_STATUS ctrlWinOverlapTest(MV_ADDR_WIN *pAddrWin1, MV_ADDR_WIN *pAddrWin2)
{
    MV_U32 winBase1, winBase2;
    MV_U32 winTop1, winTop2;
    
	if (((0xffffffff - pAddrWin1->baseLow) < pAddrWin1->size-1)||
	   ((0xffffffff - pAddrWin2->baseLow) < pAddrWin2->size-1))
	{
		return MV_TRUE;
	}

    winBase1 = pAddrWin1->baseLow;
    winBase2 = pAddrWin2->baseLow;
    winTop1  = winBase1 + pAddrWin1->size-1;
    winTop2  = winBase2 + pAddrWin2->size-1;

    if (((winBase1 <= winTop2 ) && ( winTop2 <= winTop1)) ||
        ((winBase1 <= winBase2) && (winBase2 <= winTop1)))
    {
        return MV_TRUE;
    }
    else
    {
        return MV_FALSE;
    }
}

MV_STATUS ctrlWinWithinWinTest(MV_ADDR_WIN *pAddrWin1, MV_ADDR_WIN *pAddrWin2)
{
    MV_U32 winBase1, winBase2;
    MV_U32 winTop1, winTop2;
    
    winBase1 = pAddrWin1->baseLow;
    winBase2 = pAddrWin2->baseLow;
    winTop1  = winBase1 + pAddrWin1->size -1;
    winTop2  = winBase2 + pAddrWin2->size -1;
    
    if (((winBase1 >= winBase2 ) && ( winBase1 <= winTop2)) ||
        ((winTop1  >= winBase2) && (winTop1 <= winTop2)))
    {
        return MV_TRUE;
    }
    else
    {
        return MV_FALSE;
    }
}

static const char* cntrlName[] = TARGETS_NAME_ARRAY;

const MV_8* mvCtrlTargetNameGet( MV_TARGET target )
{

	if (target >= MAX_TARGETS)
	{
		return "target unknown";
	}

	return cntrlName[target];
}

MV_VOID mvCtrlAddrDecShow(MV_VOID)
{
    mvCpuIfAddDecShow();
    mvAhbToMbusAddDecShow();
#if defined(MV_INCLUDE_PEX)
	mvPexAddrDecShow();
#endif
#if defined(MV_INCLUDE_USB)
    	mvUsbAddrDecShow();
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	mvEthAddrDecShow();
#endif
#if defined(MV_INCLUDE_XOR)
	mvXorAddrDecShow();
#endif
#if defined(MV_INCLUDE_SATA)
    mvSataAddrDecShow();
#endif
#if defined(MV_INCLUDE_AUDIO)
    mvAudioAddrDecShow();
#endif
#if defined(MV_INCLUDE_TS)
    mvTsuAddrDecShow();
#endif
#if defined(MV_INCLUDE_LCD)
    if(mvCtrlModelGet() == MV_6282_DEV_ID)
    	mvLcdAddrDecShow();
#endif
}

MV_U32	ctrlSizeToReg(MV_U32 size, MV_U32 alignment)
{
	MV_U32 retVal;

	if ((0 == size) || (MV_IS_NOT_ALIGN(size, alignment)))
	{
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Size is zero or not aligned.\n"));
		return -1;
	}
	
	alignment--;	 
					 
	while(alignment & 1)	 
	{
		size = (size >> 1);  	
		alignment = (alignment >> 1);
	}
	
	if (alignment)
	{
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Alignment parameter 0x%x invalid.\n", 
			(MV_U32)alignment));
		return -1;
	}

	size--;          
    
	retVal = size ;
	
	while(size & 1)	 
	{
		size = (size >> 1);  	
	}

    if (size)  
	{
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Size parameter 0x%x invalid.\n", 
                                                                        size));
		return -1;
	}
	
    return retVal;
	
}

MV_U32	ctrlRegToSize(MV_U32 regSize, MV_U32 alignment)
{
   	MV_U32 temp;

	temp = regSize;		 
	
	while(temp & 1)	 
	{
		temp = (temp >> 1);  	
	}

    if (temp)  
	{
		DB(mvOsPrintf("ctrlRegToSize: ERR. Size parameter 0x%x invalid.\n", 
					regSize));
	   	return -1;
	}
	
	temp = alignment - 1; 
					
	while(temp & 1)	 
	{
		temp = (temp >> 1);  	
	}
	
	if (temp)
	{
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Alignment parameter 0x%x invalid.\n", 
					alignment));
		return -1;
	}

	regSize++;       

	alignment--;	 

	while(alignment & 1)	 
	{
		regSize   = (regSize << 1);  	
		alignment = (alignment >> 1);
	}
		
    return regSize;	
}

MV_U32	ctrlSizeRegRoundUp(MV_U32 size, MV_U32 alignment)
{
	MV_U32 msbBit = 0;
    MV_U32 retSize;
	
	if (!(-1 == ctrlSizeToReg(size, alignment)))
	{
		return size;
	}
    
    while(size)
	{
		size = (size >> 1);
        msbBit++;
	}

    retSize = (1 << msbBit);
    
    if (retSize < alignment)
    {
        return alignment;
    }
    else
    {
        return retSize;
    }
}
 
MV_U32	mvCtrlSysRstLengthCounterGet(MV_VOID)
{
	static volatile MV_U32 Count = 0;

	if(!Count) {
		Count = (MV_REG_READ(SYSRST_LENGTH_COUNTER_REG) & SLCR_COUNT_MASK);
		Count = (Count / (MV_BOARD_REFCLK_25MHZ / 1000));
		 
		MV_REG_BIT_SET(SYSRST_LENGTH_COUNTER_REG, SLCR_CLR_MASK);	
	}

	DB(mvOsPrintf("mvCtrlSysRstLengthCounterGet: Reset button was pressed for %u milliseconds\n", Count));

	return Count;		
}

MV_BOOL	  mvCtrlIsBootFromSPI(MV_VOID)
{
    MV_U32 satr = 0;
    satr = MV_REG_READ(MPP_SAMPLE_AT_RESET);
    if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
    {
        if (MSAR_BOOT_MODE_6180(satr) == MSAR_BOOT_SPI_WITH_BOOTROM_6180)
            return MV_TRUE;
        else
            return MV_FALSE;
    }
    satr = satr & MSAR_BOOT_MODE_MASK;    
    if (satr == MSAR_BOOT_SPI_WITH_BOOTROM)
        return MV_TRUE;
    else
        return MV_FALSE;
}

MV_BOOL	  mvCtrlIsBootFromSPIUseNAND(MV_VOID)
{
    MV_U32 satr = 0;
    if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
        return MV_FALSE;
    satr = MV_REG_READ(MPP_SAMPLE_AT_RESET);
    satr = satr & MSAR_BOOT_MODE_MASK;
    
    if (satr == MSAR_BOOT_SPI_USE_NAND_WITH_BOOTROM)
        return MV_TRUE;
    else
        return MV_FALSE;
}

MV_BOOL	  mvCtrlIsBootFromNAND(MV_VOID)
{
    MV_U32 satr = 0;
    satr = MV_REG_READ(MPP_SAMPLE_AT_RESET);
    if(mvCtrlModelGet() == MV_6180_DEV_ID || mvCtrlModelGet() == MV_6280_DEV_ID)
    {
        if (MSAR_BOOT_MODE_6180(satr) == MSAR_BOOT_NAND_WITH_BOOTROM_6180)
            return MV_TRUE;
        else
            return MV_FALSE;
    }
    satr = satr & MSAR_BOOT_MODE_MASK;    
    if ((satr == MSAR_BOOT_NAND_WITH_BOOTROM))
        return MV_TRUE;
    else
        return MV_FALSE;
}

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
 
MV_VOID   mvCtrlPwrSaveOn(MV_VOID)
{
	unsigned long old,temp;
	 
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "orr %1, %0, #0xc0\n"
			     "msr cpsr_c, %1"
			     : "=r" (old), "=r" (temp)
			     :
			     : "memory");

	MV_REG_BIT_SET(POWER_MNG_CTRL_REG, BIT11);
	 
	__asm__ __volatile__("mcr    p15, 0, r0, c7, c0, 4");

	__asm__ __volatile__("msr cpsr_c, %0"
			     :
			     : "r" (old)
			     : "memory");
}
#ifdef MY_ABC_HERE
EXPORT_SYMBOL(mvCtrlPwrSaveOn);
#endif

MV_VOID   mvCtrlPwrSaveOff(MV_VOID)
{
	unsigned long old,temp;
	 
	__asm__ __volatile__("mrs %0, cpsr\n"
			     "orr %1, %0, #0xc0\n"
			     "msr cpsr_c, %1"
			     : "=r" (old), "=r" (temp)
			     :
			     : "memory");

	MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, BIT11);
	 
	__asm__ __volatile__("mcr    p15, 0, r0, c7, c0, 4");

	__asm__ __volatile__("msr cpsr_c, %0"
			     :
			     : "r" (old)
			     : "memory");
}
#ifdef MY_ABC_HERE
EXPORT_SYMBOL(mvCtrlPwrSaveOff);
#endif

MV_VOID   mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	switch (unitId)
    {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_PEXSTOPCLOCK_MASK(index));
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_PEXSTOPCLOCK_MASK(index));
		}
		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_GESTOPCLOCK_MASK(index));
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_GESTOPCLOCK_MASK(index));
		}
		break;
#endif
#if defined(MV_INCLUDE_INTEG_SATA)
	case SATA_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SATASTOPCLOCK_MASK(index));
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SATASTOPCLOCK_MASK(index));
		}
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SESTOPCLOCK_MASK);
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SESTOPCLOCK_MASK);
		}
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_USBSTOPCLOCK_MASK);
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_USBSTOPCLOCK_MASK);
		}
		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_AUDIOSTOPCLOCK_MASK);
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_AUDIOSTOPCLOCK_MASK);
		}
		break;
#endif
#if defined(MV_INCLUDE_TS)
	case TS_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_TSSTOPCLOCK_MASK);
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_TSSTOPCLOCK_MASK);
		}
		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SDIOSTOPCLOCK_MASK);
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SDIOSTOPCLOCK_MASK);
		}
		break;
#endif
#if defined(MV_INCLUDE_TDM)
	case TDM_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_TDMSTOPCLOCK_MASK);
		}
		else
		{
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_TDMSTOPCLOCK_MASK);
		}
		break;
#endif

	default:

		break;

	}
}

MV_BOOL		mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg = MV_REG_READ(POWER_MNG_CTRL_REG);
	MV_BOOL state = MV_TRUE;

	switch (unitId)
    {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if ((reg & PMC_PEXSTOPCLOCK_MASK(index)) == PMC_PEXSTOPCLOCK_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;

		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if ((reg & PMC_GESTOPCLOCK_MASK(index)) == PMC_GESTOPCLOCK_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		if ((reg & PMC_SATASTOPCLOCK_MASK(index)) == PMC_SATASTOPCLOCK_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if ((reg & PMC_SESTOPCLOCK_MASK) == PMC_SESTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if ((reg & PMC_USBSTOPCLOCK_MASK) == PMC_USBSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if ((reg & PMC_AUDIOSTOPCLOCK_MASK) == PMC_AUDIOSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TS)
	case TS_UNIT_ID:
		if ((reg & PMC_TSSTOPCLOCK_MASK) == PMC_TSSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if ((reg & PMC_SDIOSTOPCLOCK_MASK)== PMC_SDIOSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TDM)
	case TDM_UNIT_ID:
		if ((reg & PMC_TDMSTOPCLOCK_MASK) == PMC_TDMSTOPCLOCK_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif

	default:
		state = MV_TRUE;
		break;
	}

	return state;	
}
 
MV_VOID   mvCtrlPwrMemSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	switch (unitId)
    {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG, PMC_PEXSTOPMEM_MASK(index));
		}
		else
		{
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG, PMC_PEXSTOPMEM_MASK(index));
		}
		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG, PMC_GESTOPMEM_MASK(index));
		}
		else
		{
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG, PMC_GESTOPMEM_MASK(index));
		}
		break;
#endif
#if defined(MV_INCLUDE_INTEG_SATA)
	case SATA_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG, PMC_SATASTOPMEM_MASK(index));
		}
		else
		{
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG, PMC_SATASTOPMEM_MASK(index));
		}
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG, PMC_SESTOPMEM_MASK);
		}
		else
		{
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG, PMC_SESTOPMEM_MASK);
		}
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG, PMC_USBSTOPMEM_MASK);
		}
		else
		{
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG, PMC_USBSTOPMEM_MASK);
		}
		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG, PMC_AUDIOSTOPMEM_MASK);
		}
		else
		{
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG, PMC_AUDIOSTOPMEM_MASK);
		}
		break;
#endif
#if defined(MV_INCLUDE_XOR)
	case XOR_UNIT_ID:
		if (enable == MV_FALSE)
		{
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG, PMC_XORSTOPMEM_MASK(index));
		}
		else
		{
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG, PMC_XORSTOPMEM_MASK(index));
		}
		break;
#endif
	default:

		break;

	}
}

MV_BOOL		mvCtrlPwrMemGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG);
	MV_BOOL state = MV_TRUE;

	switch (unitId)
    {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if ((reg & PMC_PEXSTOPMEM_MASK(index)) == PMC_PEXSTOPMEM_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;

		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if ((reg & PMC_GESTOPMEM_MASK(index)) == PMC_GESTOPMEM_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		if ((reg & PMC_SATASTOPMEM_MASK(index)) == PMC_SATASTOPMEM_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if ((reg & PMC_SESTOPMEM_MASK) == PMC_SESTOPMEM_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if ((reg & PMC_USBSTOPMEM_MASK) == PMC_USBSTOPMEM_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if ((reg & PMC_AUDIOSTOPMEM_MASK) == PMC_AUDIOSTOPMEM_STOP)
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_XOR)
	case XOR_UNIT_ID:
		if ((reg & PMC_XORSTOPMEM_MASK(index)) == PMC_XORSTOPMEM_STOP(index))
		{
			state = MV_FALSE;
		}
		else state = MV_TRUE;
		break;
#endif

	default:
		state = MV_TRUE;
		break;
	}

	return state;	
}
#else
MV_VOID   mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable) {return;}
MV_BOOL	  mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index) {return MV_TRUE;}
#endif  

MV_VOID   mvMPPConfigToSPI(MV_VOID)
{
	MV_U32 mppVal = 0;
	MV_U32 bootVal = 0;

    if(!mvCtrlIsBootFromSPIUseNAND())
        return;
    mppVal = 0x00002220;  
    bootVal = MV_REG_READ(mvCtrlMppRegGet(0));
    bootVal &= 0xffff000f;
        mppVal |= bootVal;
    
    MV_REG_WRITE(mvCtrlMppRegGet(0), mppVal);
}

MV_VOID   mvMPPConfigToDefault(MV_VOID)
{
	MV_U32 mppVal = 0;
	MV_U32 bootVal = 0;

    if(!mvCtrlIsBootFromSPIUseNAND())
        return;
    mppVal = mvBoardMppGet(0);
    bootVal = MV_REG_READ(mvCtrlMppRegGet(0));
    mppVal &= ~0xffff000f;
    bootVal &= 0xffff000f;
        mppVal |= bootVal;
    
    MV_REG_WRITE(mvCtrlMppRegGet(0), mppVal);
}

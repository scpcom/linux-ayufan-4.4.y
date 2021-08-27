#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvCommon.h"
#include "mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"
#include "mvSysEthConfig.h"

#if defined(MV_INCLUDE_PEX)
#include "pex/mvPex.h"
#include "pex/mvPexRegs.h"
#endif

#if defined(MV_INCLUDE_GIG_ETH)
#if defined(MV_ETH_LEGACY)
#include "eth/mvEth.h"
#else
#include "neta/gbe/mvNeta.h"
#endif  
#endif

#if defined(MV_INCLUDE_XOR)
#include "xor/mvXor.h"
#endif

#if defined(MV_INCLUDE_SATA)
#include "mvSysSataConfig.h"
#include "sata/CoreDriver/mvSata.h"
#endif
#if defined(MV_INCLUDE_USB)
#include "usb/mvUsb.h"
#endif

#if defined(MV_INCLUDE_TDM)
#include "mvSysTdmConfig.h"
static MV_VOID mvCtrlTdmClkCtrlSet(MV_VOID);
#endif

#undef MV_DEBUG
#ifdef MV_DEBUG
#define DB(x)	x
#else
#define DB(x)
#endif

MV_U32 mvCtrlGetCpuNum(MV_VOID)
{
	return 0;
}

MV_BOOL mvCtrlIsValidSatR(MV_VOID)
{
	return MV_TRUE;
}

MV_STATUS mvCtrlEnvInit(MV_VOID)
{
	MV_U32 mppGroup;
	MV_U32 mppVal;

	MV_REG_BIT_RESET(SOC_COHERENCY_FABRIC_CTRL_REG, BIT8);

	MV_REG_BIT_SET(SOC_CIB_CTRL_CFG_REG, BIT8);

	for (mppGroup = 0; mppGroup < 1; mppGroup++) {
		mppVal = mvBoardMppGet(mppGroup);	 
		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}

	mvBoardMppModulesScan();

	mvBoardUpdateMppAfterScan();

	for (mppGroup = 0; mppGroup < MV_MPP_MAX_GROUP; mppGroup++) {
		mppVal = mvBoardMppGet(mppGroup);	 
		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}

	mvBoardUpdateEthAfterScan();

	if (MV_OK != mvCtrlSerdesPhyConfig())
		mvOsPrintf("mvCtrlEnvInit: Can't init some or all SERDES lanes\n");

	mvOsDelay(10);

#if defined(MV_INCLUDE_TDM)
	mvCtrlTdmClkCtrlSet();
#endif

	return MV_OK;
}

MV_U32 mvCtrlMppRegGet(MV_U32 mppGroup)
{
	MV_U32 ret;

	if (mppGroup >= MV_MPP_MAX_GROUP)
		mppGroup = 0;

	ret = MPP_CONTROL_REG(mppGroup);

	return ret;
}

#if defined(MV_INCLUDE_PEX)
 
MV_U32 mvCtrlPexMaxIfGet(MV_VOID)
{
#if defined(MY_DEF_HERE)
	MV_U32 boardId = mvBoardIdGet();

	switch (boardId) {
	case SYNO_DS213j_ID:
		return 1;
	case SYNO_US3_ID:
		return 1;
	case SYNO_RS214_ID:
		return 2;
	case SYNO_DS214se_ID:
		return 1;
	case SYNO_DS414slim_ID:
		return 2;
	case SYNO_DS115j_ID:
		return 1;
	case SYNO_DS216se_ID:
		return 1;
	default:
		return MV_PEX_MAX_IF;
	}
#else
	return MV_PEX_MAX_IF;
#endif
}

MV_U32 mvCtrlPexMaxUnitGet(MV_VOID)
{
	return MV_PEX_MAX_UNIT;
}

#endif

MV_U32 mvCtrlEthMaxPortGet(MV_VOID)
{
	return MV_ETH_MAX_PORTS;
}

MV_U8 mvCtrlEthMaxCPUsGet(MV_VOID)
{
	return 1;
}

#if defined(MV_INCLUDE_SATA)
 
MV_U32 mvCtrlSataMaxPortGet(MV_VOID)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	case MV_6W11_DEV_ID:
		res = MV_SATA_MV6W11_CHAN;
		break;
	default:
		res = MV_SATA_MAX_CHAN;
		break;
	}
	return res;
}
#endif

#if defined(MV_INCLUDE_XOR)
 
MV_U32 mvCtrlXorMaxChanGet(MV_VOID)
{
	return MV_XOR_MAX_CHAN;
}

MV_U32 mvCtrlXorMaxUnitGet(MV_VOID)
{
	return MV_XOR_MAX_UNIT;
}

#endif

#if defined(MV_INCLUDE_USB)
 
MV_U32 mvCtrlUsbMaxGet(void)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	case MV_FPGA_DEV_ID:
		res = 0;
		break;

	default:
		res = MV_MAX_USB_PORTS;
		break;
	}

	return res;
}
#endif

#if defined(MV_INCLUDE_LEGACY_NAND)
 
MV_U32 mvCtrlNandSupport(MV_VOID)
{
	return ARMADA_370_NAND;
}
#endif

#if defined(MV_INCLUDE_SDIO)
 
MV_U32 mvCtrlSdioSupport(MV_VOID)
{
	return ARMADA_370_SDIO;
}
#endif

MV_U32 mvCtrlTdmSupport(MV_VOID)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	case MV_6W11_DEV_ID:
	case MV_6710_DEV_ID:
		res = ARMADA_370_TDM;
		break;
	case MV_6707_DEV_ID:
	default:
		res = 0;
		break;
	}
	return res;
}

#if defined(MV_INCLUDE_TDM)
 
static MV_VOID mvCtrlTdmClkCtrlSet(MV_VOID)
{
	MV_U32 pllCtrlReg, pcmClkFreq = TDM_FULL_DIV_8M;

#if defined(MV_TDM_PCM_CLK_4MHZ)
	pcmClkFreq = TDM_FULL_DIV_4M;
#elif defined(MV_TDM_PCM_CLK_2MHZ)
	pcmClkFreq = TDM_FULL_DIV_2M;
#endif
	pllCtrlReg = MV_REG_READ(TDM_PLL_CONTROL_REG);
	pllCtrlReg = pllCtrlReg & ~(TDM_CLK_ENABLE_MASK | TDM_FULL_DIV_MASK);
	pllCtrlReg |= (TDM_CLK_ENABLE_MASK | pcmClkFreq);
	MV_REG_WRITE(TDM_PLL_CONTROL_REG, pllCtrlReg);
}

MV_U32 mvCtrlTdmMaxGet(MV_VOID)
{
	return ARMADA_370_MAX_TDM_PORTS;
}

MV_UNIT_ID mvCtrlTdmUnitTypeGet(MV_VOID)
{
	return TDM_UNIT_2CH;
}

MV_U32 mvCtrlTdmUnitIrqGet(MV_VOID)
{
	return MV_TDM_IRQ_NUM;
}

#endif  

MV_U32 mvCtrlAudioSupport(MV_VOID)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	case MV_6W11_DEV_ID:
		res = MV_FALSE;
		break;
	default:
		res = MV_TRUE;
		break;
	}
	return res;
}

MV_U16 mvCtrlModelGet(MV_VOID)
{
	MV_U32 devId;
	MV_U16 model = 0;
	MV_U32 reg, reg2;

	reg = MV_REG_READ(POWER_MNG_CTRL_REG);
	if ((reg & PMC_PEXSTOPCLOCK_MASK(0)) == PMC_PEXSTOPCLOCK_STOP(0)) {
		reg2 = ((reg & ~PMC_PEXSTOPCLOCK_MASK(0)) | PMC_PEXSTOPCLOCK_EN(0));
		MV_REG_WRITE(POWER_MNG_CTRL_REG, reg2);
	}

	devId = MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0, PEX_DEVICE_AND_VENDOR_ID));

	if ((reg & PMC_PEXSTOPCLOCK_MASK(0)) == PMC_PEXSTOPCLOCK_STOP(0))
		MV_REG_WRITE(POWER_MNG_CTRL_REG, reg);

	model = (MV_U16) ((devId >> 16) & 0xFFFF);
	return model;
}

MV_U8 mvCtrlRevGet(MV_VOID)
{
	MV_U8 revNum;
#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	 
	MV_U32 pexPower;
	pexPower = mvCtrlPwrClckGet(PEX_UNIT_ID, 0);
	if (pexPower == MV_FALSE)
		mvCtrlPwrClckSet(PEX_UNIT_ID, 0, MV_TRUE);
#endif
	revNum = (MV_U8) MV_REG_READ(PEX_CFG_DIRECT_ACCESS(0, PCI_CLASS_CODE_AND_REVISION_ID));
#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	 
	if (pexPower == MV_FALSE)
		mvCtrlPwrClckSet(PEX_UNIT_ID, 0, MV_FALSE);
#endif
	return ((revNum & PCCRIR_REVID_MASK) >> PCCRIR_REVID_OFFS);
}

MV_STATUS mvCtrlNameGet(char *pNameBuff)
{
	if (mvCtrlModelGet() == 0x6710)
		mvOsSPrintf(pNameBuff, "%s6710", SOC_NAME_PREFIX);
	else if (mvCtrlModelGet() == 0x6707)
		mvOsSPrintf(pNameBuff, "%s6707", SOC_NAME_PREFIX);
	else if (mvCtrlModelGet() == 0x6711)
		mvOsSPrintf(pNameBuff, "%s6W11", SOC_NAME_PREFIX);
	else
		mvOsSPrintf(pNameBuff, "%s%x Rev %d", SOC_NAME_PREFIX, mvCtrlModelGet(), mvCtrlRevGet());
	return MV_OK;
}

MV_U32 mvCtrlModelRevGet(MV_VOID)
{
	return ((mvCtrlModelGet() << 16) | mvCtrlRevGet());
}

MV_STATUS mvCtrlModelRevNameGet(char *pNameBuff)
{
	switch (mvCtrlModelRevGet()) {
	case MV_6710_A1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_6710_A1_NAME);
		break;

	case MV_6710_A0_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_6710_A0_NAME);
		break;

	case MV_6707_A1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_6707_A1_NAME);
		break;

	case MV_6707_A0_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_6707_A0_NAME);
		break;

	case MV_6W11_A1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_6W11_A1_NAME);
		break;

	case MV_6W11_A0_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_6W11_A0_NAME);
		break;

	default:
		mvCtrlNameGet(pNameBuff);
		break;
	}

	return MV_OK;
}
MV_U32 gDevId = -1;
 
MV_U32 mvCtrlDevFamilyIdGet(MV_U16 ctrlModel)
{
	if (gDevId == -1)
	{
		switch (ctrlModel) {
		case MV_6710_DEV_ID:
		case MV_6W11_DEV_ID:
		case MV_6707_DEV_ID:
			gDevId=MV_67XX;
			return gDevId;
			break;
		default:
			return MV_ERROR;
		}
	}
	else
		return gDevId;
}

static const char *cntrlName[] = TARGETS_NAME_ARRAY;

const MV_8 *mvCtrlTargetNameGet(MV_TARGET target)
{
	if (target >= MAX_TARGETS)
		return "target unknown";

	return cntrlName[target];
}

static MV_VOID mvCtrlPexAddrDecShow(MV_VOID)
{
	MV_PEX_BAR pexBar;
	MV_PEX_DEC_WIN win;
	MV_U32 pexIf;
	MV_U32 bar, winNum;

	for (pexIf = 0; pexIf < MV_PEX_MAX_IF; pexIf++) {

		if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexIf))
			continue;
		mvOsOutput("\n");
		mvOsOutput("PEX%d:\n", pexIf);
		mvOsOutput("-----\n");

		mvOsOutput("\nPex Bars \n\n");

		for (bar = 0; bar < PEX_MAX_BARS; bar++) {
			memset(&pexBar, 0, sizeof(MV_PEX_BAR));

			mvOsOutput("%s ", pexBarNameGet(bar));

			if (mvPexBarGet(pexIf, bar, &pexBar) == MV_OK) {
				if (pexBar.enable) {
					mvOsOutput("base %08x, ", pexBar.addrWin.baseLow);
					if (pexBar.addrWin.size == 0)
						mvOsOutput("size %3dGB ", 4);
					else
						mvSizePrint(pexBar.addrWin.size);
					mvOsOutput("\n");
				} else
					mvOsOutput("disable\n");
			}
		}
		mvOsOutput("\nPex Decode Windows\n\n");

		for (winNum = 0; winNum < PEX_MAX_TARGET_WIN - 2; winNum++) {
			memset(&win, 0, sizeof(MV_PEX_DEC_WIN));

			mvOsOutput("win%d - ", winNum);

			if (mvPexTargetWinRead(pexIf, winNum, &win) == MV_OK) {
				if (win.winInfo.enable) {
					mvOsOutput("%s base %08x, ",
						   mvCtrlTargetNameGet(mvCtrlTargetByWinInfoGet(&win.winInfo)),
						   win.winInfo.addrWin.baseLow);
					mvOsOutput("....");
					mvSizePrint(win.winInfo.addrWin.size);

					mvOsOutput("\n");
				} else
					mvOsOutput("disable\n");
			}
		}

		memset(&win, 0, sizeof(MV_PEX_DEC_WIN));

		mvOsOutput("default win - ");

		if (mvPexTargetWinRead(pexIf, MV_PEX_WIN_DEFAULT, &win) == MV_OK) {
			mvOsOutput("%s ", mvCtrlTargetNameGet(win.target));
			mvOsOutput("\n");
		}
		memset(&win, 0, sizeof(MV_PEX_DEC_WIN));

		mvOsOutput("Expansion ROM - ");

		if (mvPexTargetWinRead(pexIf, MV_PEX_WIN_EXP_ROM, &win) == MV_OK) {
			mvOsOutput("%s ", mvCtrlTargetNameGet(win.target));
			mvOsOutput("\n");
		}
	}
}

static void mvUnitAddrDecShow(MV_U8 numUnits, MV_UNIT_ID unitId, const char *name, MV_WIN_GET_FUNC_PTR winGetFuncPtr)
{
	MV_UNIT_WIN_INFO win;
	MV_U32 unit, i;

	for (unit = 0; unit < numUnits; unit++) {

		if (MV_FALSE == mvCtrlPwrClckGet(unitId, unit))
			continue;
		mvOsOutput("\n");
		mvOsOutput("%s %d:\n", name, unit);
		mvOsOutput("----\n");

		for (i = 0; i < 16; i++) {
			memset(&win, 0, sizeof(MV_UNIT_WIN_INFO));

			if (winGetFuncPtr(unit, i, &win) == MV_OK) {
				mvOsOutput("win%d - ", i);
				if (win.enable) {
					mvOsOutput("%s base %08x, ",
						   mvCtrlTargetNameGet(mvCtrlTargetByWinInfoGet(&win)),
						   win.addrWin.baseLow);
					mvOsOutput("....");
					mvSizePrint(win.addrWin.size);
				} else
					mvOsOutput("disable");

				mvOsOutput("\n");
			}
		}
	}
	return;
}

MV_VOID mvCtrlAddrDecShow(MV_VOID)
{
	mvCpuIfAddDecShow();
	mvAhbToMbusAddDecShow();
#if defined(MV_INCLUDE_PEX)
	mvCtrlPexAddrDecShow();
#endif
#if defined(MV_INCLUDE_USB)
	mvUnitAddrDecShow(mvCtrlUsbMaxGet(), USB_UNIT_ID, "USB", mvUsbWinRead);
#endif

#if defined(MV_INCLUDE_GIG_ETH)
#if defined(MV_ETH_LEGACY)
	mvUnitAddrDecShow(mvCtrlEthMaxPortGet(), ETH_GIG_UNIT_ID, "ETH", mvEthWinRead);
#else
	mvUnitAddrDecShow(mvCtrlEthMaxPortGet(), ETH_GIG_UNIT_ID, "ETH", mvNetaWinRead);
#endif  
#endif  

#if defined(MV_INCLUDE_XOR)
	mvUnitAddrDecShow(mvCtrlXorMaxUnitGet(), XOR_UNIT_ID, "XOR", mvXorTargetWinRead);
#endif
#if defined(MV_INCLUDE_SATA)
	mvUnitAddrDecShow(mvCtrlSataMaxPortGet(), SATA_UNIT_ID, "Sata", mvSataWinRead);
#endif
}

MV_U32 ctrlSizeToReg(MV_U32 size, MV_U32 alignment)
{
	MV_U32 retVal;

	if ((0 == size) || (MV_IS_NOT_ALIGN(size, alignment))) {
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Size is zero or not aligned.\n"));
		return -1;
	}

	alignment--;		 
	 
	while (alignment & 1) {	 
		size = (size >> 1);	 
		alignment = (alignment >> 1);
	}

	if (alignment) {
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Alignment parameter 0x%x invalid.\n", (MV_U32) alignment));
		return -1;
	}

	size--;			 
	retVal = size;

	while (size & 1)	 
		size = (size >> 1);	 

	if (size) {		 
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Size parameter 0x%x invalid.\n", size));
		return -1;
	}
	return retVal;
}

MV_U32 ctrlRegToSize(MV_U32 regSize, MV_U32 alignment)
{
	MV_U32 temp;

	temp = regSize;		 

	while (temp & 1)	 
		temp = (temp >> 1);	 

	if (temp) {		 
		DB(mvOsPrintf("ctrlRegToSize: ERR. Size parameter 0x%x invalid.\n", regSize));
		return -1;
	}

	temp = alignment - 1;	 

	while (temp & 1)	 
		temp = (temp >> 1);	 

	if (temp) {
		DB(mvOsPrintf("ctrlSizeToReg: ERR. Alignment parameter 0x%x invalid.\n", alignment));
		return -1;
	}

	regSize++;		 

	alignment--;		 

	while (alignment & 1) {	 
		regSize = (regSize << 1);	 
		alignment = (alignment >> 1);
	}

	return regSize;
}

MV_U32 ctrlSizeRegRoundUp(MV_U32 size, MV_U32 alignment)
{
	MV_U32 msbBit = 0;
	MV_U32 retSize;

	if (!(-1 == ctrlSizeToReg(size, alignment)))
		return size;

	while (size) {
		size = (size >> 1);
		msbBit++;
	}

	retSize = (1 << msbBit);

	if (retSize < alignment)
		return alignment;
	else
		return retSize;
}

MV_U32 mvCtrlIsBootFromNOR(MV_VOID)
{
	MV_U32 satr;
	MV_U32 satrList[] = MSAR_BOOT_NOR_LIST;
	MV_U32 i = 0;

	satr = (MV_REG_READ(MPP_SAMPLE_AT_RESET) & MSAR_BOOT_MODE_MASK) >> MSAR_BOOT_MODE_OFFS;

	while (satrList[i] != 0xFFFFFFFF) {
		if (satrList[i] == satr) {
			if (satr & 0x1)
				return MV_NAND_NOR_BOOT_16BIT;
			else
				return MV_NAND_NOR_BOOT_8BIT;
		}
		i++;
	}

	return 0;
}

MV_U32 mvCtrlIsBootFromSPI(MV_VOID)
{
	MV_U32 satr;
	MV_U32 satrLowList[] = MSAR_BOOT_SPI_LOW_LIST;
	MV_U32 satrHighList[] = MSAR_BOOT_SPI_HIGH_LIST;
	MV_U32 i;

	satr = (MV_REG_READ(MPP_SAMPLE_AT_RESET) & MSAR_BOOT_MODE_MASK) >> MSAR_BOOT_MODE_OFFS;

	i = 0;
	while (satrLowList[i] != 0xFFFFFFFF) {
		if (satrLowList[i] == satr)
			return MV_SPI_LOW_MPPS;
		i++;
	}

	i = 0;
	while (satrHighList[i] != 0xFFFFFFFF) {
		if (satrHighList[i] == satr)
			return MV_SPI_HIGH_MPPS;
		i++;
	}

	return 0;
}

MV_U32 mvCtrlIsBootFromNAND(MV_VOID)
{
	MV_U32 satr;
	MV_U32 satrList[] = MSAR_BOOT_NAND_LIST;
	MV_U32 i = 0;

	satr = (MV_REG_READ(MPP_SAMPLE_AT_RESET) & MSAR_BOOT_MODE_MASK) >> MSAR_BOOT_MODE_OFFS;

	while (satrList[i] != 0xFFFFFFFF) {
		if (satrList[i] == satr) {
			if (satr < 0x20)
				return MV_NAND_NOR_BOOT_8BIT;
			else
				return MV_NAND_NOR_BOOT_16BIT;
		}
		i++;
	}

	return 0;
}

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
 
MV_VOID mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_PEXSTOPCLOCK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_PEXSTOPCLOCK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_GESTOPCLOCK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_GESTOPCLOCK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_INTEG_SATA)
	case SATA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SATASTOPCLOCK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SATASTOPCLOCK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_CESASTOPCLOCK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_CESASTOPCLOCK_MASK);

		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_USBSTOPCLOCK_MASK(index));
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_USBSTOPCLOCK_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_SDIOSTOPCLOCK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_SDIOSTOPCLOCK_MASK);

		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_AUSTOPCLOCK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_AUSTOPCLOCK_MASK);

		break;
#endif
#if defined(MV_INCLUDE_TDM)
	case TDM_2CH_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_TDMSTOPCLOCK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_TDMSTOPCLOCK_MASK);

		break;
#endif
	default:
		break;
	}
}

MV_BOOL mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg = MV_REG_READ(POWER_MNG_CTRL_REG);
	MV_BOOL state = MV_TRUE;

	if (mvCtrlModelGet() == MV_FPGA_DEV_ID)
		return MV_TRUE;

	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if ((reg & PMC_PEXSTOPCLOCK_MASK(index)) == PMC_PEXSTOPCLOCK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if ((reg & PMC_GESTOPCLOCK_MASK(index)) == PMC_GESTOPCLOCK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		if ((reg & PMC_SATASTOPCLOCK_MASK(index)) == PMC_SATASTOPCLOCK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if ((reg & PMC_CESASTOPCLOCK_MASK) == PMC_CESASTOPCLOCK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if ((reg & PMC_USBSTOPCLOCK_MASK(index)) == PMC_USBSTOPCLOCK_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SDIO)
	case SDIO_UNIT_ID:
		if ((reg & PMC_SDIOSTOPCLOCK_MASK) == PMC_SDIOSTOPCLOCK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_TDM)
	case TDM_2CH_UNIT_ID:
		if ((reg & PMC_TDMSTOPCLOCK_MASK) == PMC_TDMSTOPCLOCK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if ((reg & PMC_AUSTOPCLOCK_MASK) == PMC_AUSTOPCLOCK_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
	default:
		state = MV_TRUE;
		break;
	}

	return state;
}

MV_VOID mvCtrlPwrMemSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PEX), PMC_PEXSTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PEX), PMC_PEXSTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_GE), PMC_GESTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_GE), PMC_GESTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_INTEG_SATA)
	case SATA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_SATA), PMC_SATASTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_SATA), PMC_SATASTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_CESA), PMC_CESASTOPMEM_STOP);
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_CESA), PMC_CESASTOPMEM_MASK);

		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_USB), PMC_USBSTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_USB), PMC_USBSTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_XOR)
	case XOR_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_XOR), PMC_XORSTOPMEM_STOP(index));
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_XOR), PMC_XORSTOPMEM_MASK(index));

		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_AUDIO), PMC_AUSTOPMEM_STOP);
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_AUDIO), PMC_AUSTOPMEM_MASK);
		break;
#endif

	default:
		break;
	}
}

MV_BOOL mvCtrlPwrMemGet(MV_UNIT_ID unitId, MV_U32 index)
{
	MV_U32 reg;
	MV_BOOL state = MV_TRUE;

	switch (unitId) {
#if defined(MV_INCLUDE_PEX)
	case PEX_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PEX));
		if ((reg & PMC_PEXSTOPMEM_MASK(index)) == PMC_PEXSTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_GIG_ETH)
	case ETH_GIG_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_GE));
		if ((reg & PMC_GESTOPMEM_MASK(index)) == PMC_GESTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_SATA)
	case SATA_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_SATA));
		if ((reg & PMC_SATASTOPMEM_MASK(index)) == PMC_SATASTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_CESA)
	case CESA_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_CESA));
		if ((reg & PMC_CESASTOPMEM_MASK) == PMC_CESASTOPMEM_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_USB)
	case USB_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_USB));
		if ((reg & PMC_USBSTOPMEM_MASK(index)) == PMC_USBSTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_XOR)
	case XOR_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_XOR));
		if ((reg & PMC_XORSTOPMEM_MASK(index)) == PMC_XORSTOPMEM_STOP(index))
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_AUDIO)
	case AUDIO_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_XOR));
		if ((reg & PMC_AUSTOPMEM_MASK) == PMC_AUSTOPMEM_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif

	default:
		state = MV_TRUE;
		break;
	}

	return state;
}
#else
MV_VOID mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	return;
}

MV_BOOL mvCtrlPwrClckGet(MV_UNIT_ID unitId, MV_U32 index)
{
	return MV_TRUE;
}
#endif  

MV_U32 mvCtrlSerdesMaxLinesGet(MV_VOID)
{

	switch (mvCtrlModelGet()) {
	case MV_FPGA_DEV_ID:
		return 0;
	default:
		return MV_SERDES_MAX_LANES;
	}
}

MV_U32 mvCtrlDDRBudWidth(MV_VOID)
{
	MV_U32 reg;
	reg = MV_REG_READ(0x1400);

	return (reg & 0x8000) ? 32 : 16;
}
MV_BOOL mvCtrlDDRThruXbar(MV_VOID)
{
	MV_U32 reg;
	reg = MV_REG_READ(0x20184);

	return (reg & 0x1) ? MV_FALSE : MV_TRUE;
}

static const MV_U8 serdesCfg[][5] = SERDES_CFG;

MV_STATUS mvCtrlSerdesPhyConfig(MV_VOID)
{
	MV_U32		socCtrlReg;
	MV_U8		pexUnit;
	MV_U32 		pexIfNum = mvCtrlPexMaxIfGet();
	MV_BOARD_PEX_INFO 	*boardPexInfo = mvBoardPexInfoGet();

	memset(boardPexInfo, 0, sizeof(MV_BOARD_PEX_INFO));

	socCtrlReg = MV_REG_READ(SOC_CTRL_REG);

	for (pexUnit = 0; pexUnit < pexIfNum; pexUnit++) {
		if ((socCtrlReg & (1<< pexUnit)) == 0)
			boardPexInfo->pexUnitCfg[pexUnit] = PEX_BUS_DISABLED;
			else
			boardPexInfo->pexUnitCfg[pexUnit] = PEX_BUS_MODE_X1;
		}
	boardPexInfo->boardPexIfNum = pexIfNum;
	return MV_OK;
}

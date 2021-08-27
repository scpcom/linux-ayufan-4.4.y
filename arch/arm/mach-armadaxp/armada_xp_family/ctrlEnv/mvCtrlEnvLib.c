 
#include "mvCommon.h"
#include "mvCtrlEnvLib.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"
#include "mvSysEthConfig.h"

#include "pex/mvPex.h"
#include "pex/mvPexRegs.h"

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
#include "sata/CoreDriver/mvSata.h"
#endif
#if defined(MV_INCLUDE_USB)
#include "usb/mvUsb.h"
#endif

#if defined(MV_INCLUDE_TDM)
#include "mvSysTdmConfig.h"
#endif

#include "ddr2_3/mvDramIfRegs.h"

#undef MV_DEBUG
#ifdef MV_DEBUG
#define DB(x)	x
#else
#define DB(x)
#endif
MV_U32 dummyFlavour = 0;
MV_BIOS_MODE bios_modes[BIOS_MODES_NUM] = {
 
{"78230",0x13, 0x7823, 0x1,  0x3,      0x0,	 0x1a, 		0x5,		0x1,	     0x1,	    0x0,	0x1, 	     0x3,	0x1},
{"78260",0x14, 0x7826, 0x1,  0x3,      0x0,	 0x1a,		0x5,		0x1,	     0x1,	    0x0,	0x0, 	     0x3,	0x1},
{"78460",0x15, 0x7846, 0x3,  0x3,      0x0,	 0x1a, 		0x5,		0x1,	     0x3,	    0x0,	0x0, 	     0x3,	0x1},
{"78480",0x16, 0x7846, 0x3,  0x3,      0x0,	 0x1a, 		0x5,		0x1,	     0x3,	    0x0,	0x0, 	     0x3,	0x1}

};

MV_BIOS_MODE bios_modes_b0[BIOS_MODES_NUM] = {
 
{"78230",0x13, 0x7823, 0x1,  0x3,      0x0,	 		0x5, 		0x5,			0x1,	     0x1,	    0x0,		0x1, 	   0x3,		0x1},
{"78260",0x14, 0x7826, 0x1,  0x3,      0x0,	 		0x5,		0x5,			0x1,	     0x1,	    0x0,		0x0, 	   0x3,		0x1},
{"78460",0x15, 0x7846, 0x3,  0x3,      0x0,	 		0x5, 		0x5,			0x1,	     0x3,	    0x1,		0x0, 	   0x3,		0x1},
{"78480",0x16, 0x7846, 0x3,  0x3,      0x0,	 		0x5, 		0x5,			0x1,	     0x3,	    0x1,		0x0, 	   0x3,		0x1}
};

MV_U32 mvCtrlGetCpuNum(MV_VOID)
{
	return ((MV_REG_READ(MPP_SAMPLE_AT_RESET(1)) & SAR1_CPU_CORE_MASK) >> SAR1_CPU_CORE_OFFSET);
}
MV_U32 mvCtrlGetQuadNum(MV_VOID)
{
	return ((MV_REG_READ(MPP_SAMPLE_AT_RESET(0)) & SAR0_L2_SIZE_MASK) >> SAR0_L2_SIZE_OFFSET);
}
MV_BOOL mvCtrlIsValidSatR(MV_VOID)
{
	int i = 0;
	MV_U32 tmpSocCores;
	MV_U8 cpuEna = 0;
	MV_U8 l2size;
	MV_U8 cpuFreq;
	MV_U8 fabricFreq;
	MV_U8 cpuFreqMode;
	MV_U8 fabricFreqMode;
	MV_BIOS_MODE * pBbiosModes;

#if defined(RD_88F78460_SERVER) || defined(DB_78X60_AMC)
	MV_U32 confId = 0x15;
#else
	MV_U32 confId = mvBoardConfIdGet();
#endif
	l2size = (MV_REG_READ(MPP_SAMPLE_AT_RESET(0)) & SAR0_L2_SIZE_MASK) >> SAR0_L2_SIZE_OFFSET;
	cpuFreq = (MV_REG_READ(MPP_SAMPLE_AT_RESET(0)) & SAR0_CPU_FREQ_MASK) >> SAR0_CPU_FREQ_OFFSET;
	fabricFreq = (MV_REG_READ(MPP_SAMPLE_AT_RESET(0)) & SAR0_FABRIC_FREQ_MASK) >> SAR0_FABRIC_FREQ_OFFSET;
	tmpSocCores = (MV_REG_READ(MPP_SAMPLE_AT_RESET(1)) & SAR1_CPU_CORE_MASK) >> SAR1_CPU_CORE_OFFSET;
	cpuFreqMode = (MV_REG_READ(MPP_SAMPLE_AT_RESET(1)) & SAR1_CPU_MODE_MASK) >> SAR1_CPU_MODE_OFFSET;
	fabricFreqMode = (MV_REG_READ(MPP_SAMPLE_AT_RESET(1)) & SAR1_FABRIC_MODE_MASK) >> SAR1_FABRIC_MODE_OFFSET;
	 
	cpuEna |= (tmpSocCores & 0x2) >> 1;
	cpuEna |= (tmpSocCores & 0x1) << 1;
	if (mvCtrlRevGet() == 2)
		pBbiosModes = bios_modes_b0;
	else
		pBbiosModes = bios_modes;

	for (i = 0; i < BIOS_MODES_NUM; i++) {
		if (pBbiosModes->confId == confId) {
			DB(mvOsPrintf("confId = 0x%x\n", confId));
			DB(mvOsPrintf("cpuFreq [0x%x] = 0x%x\n", cpuFreq, pBbiosModes->cpuFreq));
			DB(mvOsPrintf("fabricFreq [0x%x] = 0x%x\n", fabricFreq, pBbiosModes->fabricFreq));
			DB(mvOsPrintf("cpuEna [0x%x] = 0x%x\n", cpuEna, pBbiosModes->cpuEna));
			DB(mvOsPrintf("cpuFreqMode [0x%x] = 0x%x\n", cpuFreqMode, pBbiosModes->cpuFreqMode));
			DB(mvOsPrintf("fabricFreqMode [0x%x] = 0x%x\n", fabricFreqMode, pBbiosModes->fabricFreqMode));
			DB(mvOsPrintf("l2size [0x%x] = 0x%x\n", l2size, pBbiosModes->l2size));
			if ((cpuFreq == pBbiosModes->cpuFreq) &&
				(fabricFreq ==  pBbiosModes->fabricFreq) &&
				(cpuEna == pBbiosModes->cpuEna) &&
				(cpuFreqMode == pBbiosModes->cpuFreqMode) &&
				(fabricFreqMode == pBbiosModes->fabricFreqMode) &&
				(l2size == pBbiosModes->l2size)) {
				return MV_TRUE;
			} else {
				return MV_FALSE;
			}
		}
		pBbiosModes++;
	}
	return MV_FALSE;
}
 
MV_STATUS mvCtrlEnvInit(MV_VOID)
{
	MV_U32 mppGroup;
	MV_U32 mppVal;
	MV_U32 i, gppMask;

	MV_REG_BIT_RESET(SOC_COHERENCY_FABRIC_CTRL_REG, BIT8);

	MV_REG_BIT_SET(SOC_CIB_CTRL_CFG_REG, BIT8);

	mvBoardMppModulesScan();

	for (mppGroup = 0; mppGroup < MV_MPP_MAX_GROUP; mppGroup++) {
		mppVal = mvBoardMppGet(mppGroup);	 
		MV_REG_WRITE(mvCtrlMppRegGet(mppGroup), mppVal);
	}

	for (i = 0; i < MV_GPP_MAX_GROUP; i++) {
		MV_REG_WRITE(GPP_INT_MASK_REG(i), 0x0);
		MV_REG_WRITE(GPP_INT_LVL_REG(i), 0x0);
	}

	for (i = 0; i < MV_GPP_MAX_GROUP; i++)
		MV_REG_WRITE(GPP_INT_CAUSE_REG(i), 0x0);

	for (i = 0; i < MV_GPP_MAX_GROUP; i++) {
		gppMask = mvBoardGpioIntMaskGet(i);
		mvGppTypeSet(i, gppMask , (MV_GPP_IN & gppMask));
		mvGppPolaritySet(i, gppMask , (MV_GPP_IN_INVERT & gppMask));
	}

	mvBoardOtherModulesScan();

	if (MV_OK != mvCtrlSerdesPhyConfig())
		mvOsPrintf("mvCtrlEnvInit: Can't init some or all SERDES lanes\n");

	MV_REG_BIT_SET(PUP_EN_REG,0x17);  

	mvOsDelay(100);

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
	switch (mvCtrlModelGet()) {
	case MV_78130_DEV_ID:
	case MV_6710_DEV_ID:
	case MV_78230_DEV_ID:
		return 7;

	case MV_78160_DEV_ID:
	case MV_78260_DEV_ID:
	case MV_78460_DEV_ID:
	case MV_78000_DEV_ID:
		return MV_PEX_MAX_IF;

	default:
		return 0;
	}
}
#endif

MV_U32 mvCtrlPexMaxUnitGet(MV_VOID)
{
	switch (mvCtrlModelGet()) {
	case MV_78130_DEV_ID:
	case MV_6710_DEV_ID:
	case MV_78230_DEV_ID:
		return 2;

	case MV_78160_DEV_ID:
	case MV_78260_DEV_ID:
		return 3;

	case MV_78460_DEV_ID:
	case MV_78000_DEV_ID:
		return MV_PEX_MAX_UNIT;

	default:
		return 0;
	}
}

#if defined(MV_INCLUDE_PCI)
 
MV_U32 mvCtrlPciMaxIfGet(MV_VOID)
{
	switch (mvCtrlModelGet()) {
	case MV_FPGA_DEV_ID:
		return 1;

	default:
		return 0;
	}
}
#endif

MV_U32 mvCtrlEthMaxPortGet(MV_VOID)
{
	MV_U32 devId;

	devId = mvCtrlModelGet();
	switch (devId) {
	case MV_78130_DEV_ID:
	case MV_6710_DEV_ID:
	case MV_78230_DEV_ID:
		return MV_78130_ETH_MAX_PORT;

	case MV_78160_DEV_ID:
	case MV_78260_DEV_ID:
	case MV_78460_DEV_ID:
	case MV_78000_DEV_ID:
		return MV_78460_ETH_MAX_PORT;

	default:
		return 0;
	}
}

MV_U8 mvCtrlEthMaxCPUsGet(MV_VOID)
{
	MV_U32 devId;

	devId = mvCtrlModelGet();

	devId = MV_78460_DEV_ID;

	switch (devId) {
	case MV_78130_DEV_ID:
	case MV_78230_DEV_ID:
	case MV_78160_DEV_ID:
	case MV_78260_DEV_ID:
	case MV_78460_DEV_ID:
		return 4;

	default:
		return 0;
	}
}

#if defined(MV_INCLUDE_SATA)
 
MV_U32 mvCtrlSataMaxPortGet(MV_VOID)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	default:
		res = MV_SATA_MAX_CHAN;
		break;
	}
	return res;
}
#endif

#if defined(MV_INCLUDE_IDMA)
 
MV_U32 mvCtrlIdmaMaxChanGet(MV_VOID)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	default:
		res = MV_IDMA_MAX_CHAN;
		break;
	}
	return res;
}

MV_U32 mvCtrlIdmaMaxUnitGet(MV_VOID)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	default:
		res = MV_IDMA_MAX_UNIT;
		break;
	}
	return res;
}
#endif  

#if defined(MV_INCLUDE_XOR)
 
MV_U32 mvCtrlXorMaxChanGet(MV_VOID)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	default:
		res = MV_XOR_MAX_CHAN;
		break;
	}
	return res;
}

MV_U32 mvCtrlXorMaxUnitGet(MV_VOID)
{
	MV_U32 devId;
	MV_U32 res = 0;

	devId = mvCtrlModelGet();

	switch (devId) {
	default:
		res = MV_XOR_MAX_UNIT;
		break;
	}
	return res;
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
		res = ARMADA_XP_MAX_USB_PORTS;
		break;
	}

	return res;
}
#endif

#if defined(MV_INCLUDE_LEGACY_NAND)
 
MV_U32 mvCtrlNandSupport(MV_VOID)
{
	return ARMADA_XP_NAND;
}
#endif

#if defined(MV_INCLUDE_SDIO)
 
MV_U32 mvCtrlSdioSupport(MV_VOID)
{
	return ARMADA_XP_SDIO;
}
#endif

#if defined(MV_INCLUDE_TDM)
 
MV_U32 mvCtrlTdmSupport(MV_VOID)
{
	return ARMADA_XP_TDM;
}

MV_U32 mvCtrlTdmMaxGet(MV_VOID)
{
	return ARMADA_XP_MAX_TDM_PORTS;
}

MV_UNIT_ID mvCtrlTdmUnitTypeGet(MV_VOID)
{
	return TDM_UNIT_32CH;
}

MV_U32 mvCtrlTdmUnitIrqGet(MV_VOID)
{
	return MV_TDM_IRQ_NUM;
}

#endif  

MV_U16 mvCtrlModelGet(MV_VOID)
{
#if defined(CONFIG_SYNO_ARMADA_ARCH)
	MV_U32 model = MV_78230_DEV_ID;
#else
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
#endif
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
	if (mvCtrlModelGet() == 0x7800)
		mvOsSPrintf(pNameBuff, "%s78XX", SOC_NAME_PREFIX);
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
	case MV_78130_Z1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_78130_Z1_NAME);
		break;

	case MV_6710_Z1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_6710_Z1_NAME);
		break;

	case MV_78230_Z1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_78230_Z1_NAME);
		break;
	case MV_78160_Z1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_78160_Z1_NAME);
		break;
	case MV_78260_Z1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_78260_Z1_NAME);
		break;
	case MV_78460_Z1_ID:
		mvOsSPrintf(pNameBuff, "%s", MV_78460_Z1_NAME);
		break;

	 case MV_78130_A0_ID:
               mvOsSPrintf(pNameBuff, "%s", MV_78130_A0_NAME);
               break;
       case MV_78230_A0_ID:
               mvOsSPrintf(pNameBuff, "%s", MV_78230_A0_NAME);
               break;
       case MV_78160_A0_ID:
               mvOsSPrintf(pNameBuff, "%s", MV_78160_A0_NAME);
               break;
       case MV_78260_A0_ID:
               mvOsSPrintf(pNameBuff, "%s", MV_78260_A0_NAME);
               break;
       case MV_78460_A0_ID:
              mvOsSPrintf(pNameBuff, "%s", MV_78460_A0_NAME);
               break;
	case MV_78130_B0_ID:
			  mvOsSPrintf(pNameBuff, "%s", MV_78130_B0_NAME);
			  break;
	  case MV_78230_B0_ID:
			  mvOsSPrintf(pNameBuff, "%s", MV_78230_B0_NAME);
			  break;
	  case MV_78160_B0_ID:
			  mvOsSPrintf(pNameBuff, "%s", MV_78160_B0_NAME);
			  break;
	  case MV_78260_B0_ID:
			  mvOsSPrintf(pNameBuff, "%s", MV_78260_B0_NAME);
			  break;
	  case MV_78460_B0_ID:
			 mvOsSPrintf(pNameBuff, "%s", MV_78460_B0_NAME);
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
		case MV_78130_DEV_ID:
		case MV_78160_DEV_ID:
		case MV_78230_DEV_ID:
		case MV_78260_DEV_ID:
		case MV_78460_DEV_ID:
		case MV_78000_DEV_ID:
			gDevId=MV_78XX0;
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
	MV_BOARD_PEX_INFO 	*boardPexInfo = mvBoardPexInfoGet();
	MV_U32 pexHWInf = 0;

	for (pexIf = 0; pexIf < boardPexInfo->boardPexIfNum; pexIf++) {
		pexHWInf = boardPexInfo->pexMapping[pexIf];

		if (MV_FALSE == mvCtrlPwrClckGet(PEX_UNIT_ID, pexHWInf))
			continue;
		mvOsOutput("\n");
		mvOsOutput("PEX%d:\n", pexHWInf);
		mvOsOutput("-----\n");

		mvOsOutput("\nPex Bars \n\n");

		for (bar = 0; bar < PEX_MAX_BARS; bar++) {
			memset(&pexBar, 0, sizeof(MV_PEX_BAR));

			mvOsOutput("%s ", pexBarNameGet(bar));

			if (mvPexBarGet(pexHWInf, bar, &pexBar) == MV_OK) {
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

			if (mvPexTargetWinRead(pexHWInf, winNum, &win) == MV_OK) {
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

		if (mvPexTargetWinRead(pexHWInf, MV_PEX_WIN_DEFAULT, &win) == MV_OK) {
			mvOsOutput("%s ", mvCtrlTargetNameGet(win.target));
			mvOsOutput("\n");
		}
		memset(&win, 0, sizeof(MV_PEX_DEC_WIN));

		mvOsOutput("Expansion ROM - ");

		if (mvPexTargetWinRead(pexHWInf, MV_PEX_WIN_EXP_ROM, &win) == MV_OK) {
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

			mvOsOutput("win%d - ", i);

			if (winGetFuncPtr(unit, i, &win) == MV_OK) {
				if (win.enable) {
					mvOsOutput("%s base %08x, ",
						   mvCtrlTargetNameGet(mvCtrlTargetByWinInfoGet(&win)),
						   win.addrWin.baseLow);
					mvOsOutput("....");
					if (win.addrWin.size == 0)
						mvOsOutput("size %3dGB ", 4);
					else
						mvSizePrint(win.addrWin.size);
					mvOsOutput("\n");
				} else
					mvOsOutput("disable\n");
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
	mvUnitAddrDecShow(mvCtrlXorMaxChanGet(), XOR_UNIT_ID, "XOR", mvXorTargetWinRead);
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

MV_BOOL mvCtrlIsBootFromNOR(MV_VOID)
{
	MV_U32 satr;

	satr = MV_REG_READ(MPP_SAMPLE_AT_RESET(0)) & MSAR_BOOT_MODE_MASK;

	if (satr == MSAR_BOOT_NOR)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_BOOL mvCtrlIsBootFromSPI(MV_VOID)
{
	MV_U32 satr;

	satr = MV_REG_READ(MPP_SAMPLE_AT_RESET(0)) & MSAR_BOOT_MODE_MASK;

	if (satr == MSAR_BOOT_SPI)
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_BOOL mvCtrlIsBootFromNAND(MV_VOID)
{
	MV_U32 satr;

	satr = MV_REG_READ(MPP_SAMPLE_AT_RESET(0)) & MSAR_BOOT_MODE_MASK;

	if ((satr == MSAR_BOOT_DOVE_NAND) || (satr == MSAR_BOOT_LEGACY_NAND))
		return MV_TRUE;
	else
		return MV_FALSE;
}

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
 
MV_VOID mvCtrlPwrClckSet(MV_UNIT_ID unitId, MV_U32 index, MV_BOOL enable)
{
	 
	if (mvCtrlModelGet() == MV_FPGA_DEV_ID)
		return;

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
	case TDM_32CH_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_RESET(POWER_MNG_CTRL_REG, PMC_TDMSTOPCLOCK_MASK);
		else
			MV_REG_BIT_SET(POWER_MNG_CTRL_REG, PMC_TDMSTOPCLOCK_MASK);
		break;
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
	case TDM_32CH_UNIT_ID:
		if ((reg & PMC_TDMSTOPCLOCK_MASK) == PMC_TDMSTOPCLOCK_STOP)
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
#if defined(MV_INCLUDE_BM)
	case BM_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_BM), PMC_BMSTOPMEM_STOP);
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_BM), PMC_BMSTOPMEM_MASK);

		break;
#endif
#if defined(MV_INCLUDE_PNC)
	case PNC_UNIT_ID:
		if (enable == MV_FALSE)
			MV_REG_BIT_SET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PNC), PMC_PNCSTOPMEM_STOP);
		else
			MV_REG_BIT_RESET(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PNC), PMC_PNCSTOPMEM_MASK);

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
#if defined(MV_INCLUDE_BM)
	case BM_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_BM));
		if ((reg & PMC_BMSTOPMEM_MASK) == PMC_BMSTOPMEM_STOP)
			state = MV_FALSE;
		else
			state = MV_TRUE;
		break;
#endif
#if defined(MV_INCLUDE_PNC)
	case PNC_UNIT_ID:
		reg = MV_REG_READ(POWER_MNG_MEM_CTRL_REG(PMC_MCR_NUM_PNC));
		if ((reg & PMC_PNCSTOPMEM_MASK) == PMC_PNCSTOPMEM_STOP)
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
	case MV_78130_DEV_ID:
	case MV_6710_DEV_ID:
	case MV_78230_DEV_ID:
		return 7;
	case MV_78160_DEV_ID:
	case MV_78260_DEV_ID:
		return 12;
		break;
	case MV_78460_DEV_ID:
	case MV_78000_DEV_ID:
		return 16;
	default:
		return 0;
	}
}

MV_U32 mvCtrlDDRBudWidth(MV_VOID)
{
	MV_U32 reg;
	reg = MV_REG_READ(0x1400);

	return (reg & 0x8000) ? 64 : 32;
}
MV_BOOL mvCtrlDDRThruXbar(MV_VOID)
{
	MV_U32 reg;
	reg = MV_REG_READ(0x20184);

	return (reg & 0x1) ? MV_FALSE : MV_TRUE;
}

MV_BOOL mvCtrlDDRECC(MV_VOID)
{
	MV_U32 reg;
	reg = MV_REG_READ(REG_SDRAM_CONFIG_ADDR);

	return (reg & (0x1 << REG_SDRAM_CONFIG_ECC_OFFS)) ? MV_TRUE : MV_FALSE;
}

static const MV_U8 serdesCfg[][8] = SERDES_CFG;

MV_STATUS mvCtrlSerdesPhyConfig(MV_VOID)
{
	MV_U32		socCtrlReg, RegX4, serdesLine0_7;
	MV_U32		serdesLineCfg;
	MV_U8		serdesLineNum;
	MV_U8		pexIf;
	MV_U8		pexUnit;
	MV_STATUS	status = MV_OK;
	MV_U32 		pexIfNum = mvCtrlPexMaxIfGet();
	MV_U8		maxSerdesLines = mvCtrlSerdesMaxLinesGet();
	MV_BOARD_PEX_INFO 	*boardPexInfo = mvBoardPexInfoGet();

	MV_U32	powermngmntctrlregmap = 0x0;
	MV_U32	tmpcounter = 0;

	if (maxSerdesLines == 0)
		return MV_OK;

	memset(boardPexInfo, 0, sizeof(MV_BOARD_PEX_INFO));
	socCtrlReg = MV_REG_READ(SOC_CTRL_REG);
	RegX4 = MV_REG_READ(GEN_PURP_RES_2_REG);
	boardPexInfo->pexUnitCfg[0].pexCfg = ((RegX4 & 0x0F) == 0x0F) ? PEX_BUS_MODE_X4: PEX_BUS_MODE_X1;
	boardPexInfo->pexUnitCfg[1].pexCfg = ((RegX4 & 0x0F0) == 0x0F0) ? PEX_BUS_MODE_X4: PEX_BUS_MODE_X1;
	boardPexInfo->pexUnitCfg[2].pexCfg = ((RegX4 & 0x0F00) == 0x0F00) ? PEX_BUS_MODE_X4: PEX_BUS_MODE_X1;
	boardPexInfo->pexUnitCfg[3].pexCfg = ((RegX4 & 0x0F000) == 0x0F000) ? PEX_BUS_MODE_X4: PEX_BUS_MODE_X1;

	for (pexIf = 0; pexIf < pexIfNum; pexIf++) {
		 
		pexUnit    = (pexIf<9)? (pexIf >> 2) : 3;
		if ((socCtrlReg & (1<< pexUnit)) == 0){
			boardPexInfo->pexUnitCfg[pexUnit].pexCfg = PEX_BUS_DISABLED;
		   continue;
			}
		   boardPexInfo->pexMapping[boardPexInfo->boardPexIfNum] = pexIf;
				boardPexInfo->boardPexIfNum++;
		   boardPexInfo->pexUnitCfg[pexUnit].pexLaneStat[pexIf] = 0x1;
		   powermngmntctrlregmap = powermngmntctrlregmap | (0x1<<(pexIf+5));
		   if (pexIf < 8) {
			   if (boardPexInfo->pexUnitCfg[pexUnit].pexCfg == PEX_BUS_MODE_X4){
				   powermngmntctrlregmap |= (0xf<<(pexIf+5));
				   pexIf += 3;
				}
			   else
				   powermngmntctrlregmap |= (0x1<<(pexIf+5));
			}
		   else
			   powermngmntctrlregmap |= (0x1<<(18+pexIf));
	}
	serdesLine0_7 = MV_REG_READ(SERDES_LINE_MUX_REG_0_7);

	for (serdesLineNum = 0; serdesLineNum < 8; serdesLineNum++) {

		serdesLineCfg =(serdesLine0_7 >> (serdesLineNum << 2)) & 0xF;

		if (serdesLineCfg == serdesCfg[serdesLineNum][SERDES_UNIT_SATA]) {

			if ((serdesLineNum == 4) || (serdesLineNum == 6))
				powermngmntctrlregmap |= PMC_SATASTOPCLOCK_MASK(0);
			else if (serdesLineNum == 5)
				powermngmntctrlregmap |= PMC_SATASTOPCLOCK_MASK(1);
			else
				goto err_cfg;

		} else if (serdesLineCfg == serdesCfg[serdesLineNum][SERDES_UNIT_SGMII0])
				powermngmntctrlregmap |= PMC_GESTOPCLOCK_MASK(0);
			else if (serdesLineCfg == serdesCfg[serdesLineNum][SERDES_UNIT_SGMII1])
				powermngmntctrlregmap |=  PMC_GESTOPCLOCK_MASK(1);
			else if (serdesLineCfg == serdesCfg[serdesLineNum][SERDES_UNIT_SGMII2])
				powermngmntctrlregmap |= PMC_GESTOPCLOCK_MASK(2);
			else if (serdesLineCfg == serdesCfg[serdesLineNum][SERDES_UNIT_SGMII3])
				powermngmntctrlregmap |= PMC_GESTOPCLOCK_MASK(3);
			else if (serdesLineCfg == serdesCfg[serdesLineNum][SERDES_UNIT_QSGMII])
				powermngmntctrlregmap |= PMC_GESTOPCLOCK_MASK(0) | PMC_GESTOPCLOCK_MASK(1) | PMC_GESTOPCLOCK_MASK(2) | PMC_GESTOPCLOCK_MASK(3);
	}

#if defined(MV_INCLUDE_CLK_PWR_CNTRL)
	powermngmntctrlregmap = powermngmntctrlregmap | BIT4;  
	 
		if (MV_TRUE ==  mvBoardIsGbEPortConnected(1))
			powermngmntctrlregmap = powermngmntctrlregmap | PMC_GESTOPCLOCK_MASK(1);

	powermngmntctrlregmap = powermngmntctrlregmap | (BIT0  | BIT13 | (0x1FF<<16) | BIT24 | BIT25 | BIT28 | BIT31);
	DB(mvOsPrintf("%s:Shutting down unused interfaces:\n", __func__));
	 
	if (!(powermngmntctrlregmap & PMC_SATASTOPCLOCK_MASK(0))) {
		DB(mvOsPrintf("%s:       SATA0\n", __func__));
		mvCtrlPwrClckSet(SATA_UNIT_ID, 0, MV_FALSE);
	}
	if (!(powermngmntctrlregmap & PMC_SATASTOPCLOCK_MASK(1))) {
		DB(mvOsPrintf("%s:       SATA1\n", __func__));
		mvCtrlPwrClckSet(SATA_UNIT_ID, 1, MV_FALSE);
	}
	for (tmpcounter = 0; tmpcounter < 4; tmpcounter++) {
		if (!(powermngmntctrlregmap & (1 << (4 - tmpcounter)))) {
			 
			DB(mvOsPrintf("%s:       GBE%d\n", __func__, tmpcounter));
			mvCtrlPwrClckSet(ETH_GIG_UNIT_ID, tmpcounter, MV_FALSE);
		}
	}
	for (tmpcounter = 0; tmpcounter < 8; tmpcounter++) {
		if (!(powermngmntctrlregmap & (1 << (5 + tmpcounter)))) {
			DB(mvOsPrintf("%s:       PEX%d.%d\n", __func__, tmpcounter>>2, tmpcounter % 4));
			mvCtrlPwrClckSet(PEX_UNIT_ID, tmpcounter, MV_FALSE);
		}
	}
	if (!(powermngmntctrlregmap & BIT26)) {
		DB(mvOsPrintf("%s:       PEX2\n", __func__));
		mvCtrlPwrClckSet(PEX_UNIT_ID, 8, MV_FALSE);
	}
	if (!(powermngmntctrlregmap & BIT27)) {
		DB(mvOsPrintf("%s:       PEX3\n", __func__));
		mvCtrlPwrClckSet(PEX_UNIT_ID, 9, MV_FALSE);
	}

	if(!(powermngmntctrlregmap & BIT25)) {
		DB(mvOsPrintf("%s:       TDM\n", __func__));
		mvCtrlPwrClckSet(TDM_32CH_UNIT_ID, 0, MV_FALSE);
	}
	 
	MV_REG_WRITE(POWER_MNG_CTRL_REG, MV_REG_READ(POWER_MNG_CTRL_REG) & powermngmntctrlregmap);
	 
	MV_REG_WRITE(POWER_MNG_CTRL_REG, MV_REG_READ(POWER_MNG_CTRL_REG) | (BIT15 | BIT30));
#endif  

	return status;
err_cfg:
	DB(mvOsPrintf("%s: Wrong CFG (%#x) for SERDES line %d.\n",
		__func__, serdesLineCfg, serdesLineNum));
	return MV_ERROR;

}

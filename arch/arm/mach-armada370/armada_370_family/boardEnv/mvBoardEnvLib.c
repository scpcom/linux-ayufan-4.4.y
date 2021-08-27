 
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "boardEnv/mvBoardEnvLib.h"
#include "ctrlEnv/sys/mvCpuIf.h"
#include "cpu/mvCpu.h"
#include "cntmr/mvCntmr.h"
#include "gpp/mvGpp.h"
#include "twsi/mvTwsi.h"
#include "pex/mvPex.h"
#include "device/mvDevice.h"
#include "neta/gbe/mvEthRegs.h"
#include "neta/gbe/mvNeta.h"
#include "gpp/mvGppRegs.h"

#undef MV_DEBUG
#ifdef MV_DEBUG
#define DB(x)	x
#define DB1(x)	x
#else
#define DB(x)
#define DB1(x)
#endif

#define CODE_IN_ROM		MV_FALSE
#define CODE_IN_RAM		MV_TRUE

#define FILL_TWSI_SLAVE(slv, addr)				\
{								\
	slv.slaveAddr.address = addr;				\
	slv.slaveAddr.type = MV_BOARD_MODULES_ADDR_TYPE;	\
	slv.validOffset = MV_TRUE;				\
	slv.offset = 0;						\
	slv.moreThen256 = MV_FALSE;				\
}

extern MV_BOARD_INFO *boardInfoTbl[];
#define BOARD_INFO(boardId)	boardInfoTbl[boardId - BOARD_ID_BASE]

static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass);

MV_U32 tClkRate = -1;

MV_U32 gBoardMppType2Index[] = {MV_BOARD_AUTO, MV_BOARD_TDM, MV_BOARD_I2S, MV_BOARD_GMII0, MV_BOARD_SDIO,
	MV_BOARD_RGMII0, MV_BOARD_RGMII1};

MV_VOID mvBoardEnvInit(MV_VOID)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_U32 norDev;
	MV_U32 i, gppMask;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardEnvInit:Board unknown.\n");
		return;
	}

#if defined(CONFIG_SYNO_ARMADA_ARCH)
#else
 
	norDev = boardGetDevCSNum(0, BOARD_DEV_NOR_FLASH);
	if (norDev != 0xFFFFFFFF) {
		 
		MV_REG_WRITE(DEV_BANK_PARAM_REG(norDev), BOARD_INFO(boardId)->norFlashReadParams);
		MV_REG_WRITE(DEV_BANK_PARAM_REG_WR(norDev), BOARD_INFO(boardId)->norFlashWriteParams);
		MV_REG_WRITE(DEV_BUS_SYNC_CTRL, 0x11);
	}

#ifdef MV_INCLUDE_NOR
	MV_REG_BIT_RESET(SOC_DEVICE_MUX_REG, BIT0);
#else
	MV_REG_BIT_SET(SOC_DEVICE_MUX_REG, BIT0);
#endif

	MV_REG_WRITE(MV_RUNIT_PMU_REGS_OFFSET + 0x4, BOARD_INFO(boardId)->pmuPwrUpPolarity);
	MV_REG_WRITE(MV_RUNIT_PMU_REGS_OFFSET + 0x14, BOARD_INFO(boardId)->pmuPwrUpDelay);

	MV_REG_WRITE(GPP_DATA_OUT_REG(0), BOARD_INFO(boardId)->gppOutValLow);
	MV_REG_WRITE(GPP_DATA_OUT_REG(1), BOARD_INFO(boardId)->gppOutValMid);
	MV_REG_WRITE(GPP_DATA_OUT_REG(2), BOARD_INFO(boardId)->gppOutValHigh);

	mvGppPolaritySet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValLow);
	mvGppPolaritySet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValMid);
	mvGppPolaritySet(2, 0xFFFFFFFF, BOARD_INFO(boardId)->gppPolarityValHigh);

	mvGppTypeSet(0, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValLow);
	mvGppTypeSet(1, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValMid);
	mvGppTypeSet(2, 0xFFFFFFFF, BOARD_INFO(boardId)->gppOutEnValHigh);

	for (i = 0; i < MV_GPP_MAX_GROUP; i++) {
		gppMask = mvBoardGpioIntMaskGet(i);
		mvGppTypeSet(i, gppMask , (MV_GPP_IN & gppMask));
		mvGppPolaritySet(i, gppMask , (MV_GPP_IN_INVERT & gppMask));
	}
#endif
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
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsSPrintf(pNameBuff, "Board unknown.\n");
		return MV_ERROR;
	}

	mvOsSPrintf(pNameBuff, "%s", BOARD_INFO(boardId)->boardName);

	return MV_OK;
}

MV_BOOL mvBoardIsPortInSgmii(MV_U32 ethPortNum)
{
	MV_U32 serdesMode = mvBoardSerdesModeGet();

	if (ethPortNum == 0) {
		if (serdesMode & (SRDS_MOD_SGMII0_LANE1 | SRDS_MOD_SGMII0_LANE2))
			return MV_TRUE;
	}

	if (ethPortNum == 1) {
		if (serdesMode & (SRDS_MOD_SGMII1_LANE0 | SRDS_MOD_SGMII1_LANE3))
			return MV_TRUE;
	}

	return MV_FALSE;
}

MV_BOOL mvBoardIsPortInGmii(MV_U32 ethPortNum)
{
	if (mvBoardIsGMIIConnected() && (ethPortNum ==0))
		return MV_TRUE;
	else
		return MV_FALSE;
}

MV_BOOL mvBoardIsPortInRgmii(MV_U32 ethPortNum)
{

	if (ethPortNum == 0) {
		if (mvBoardMppModulesCfgGet(1) & MV_BOARD_RGMII0)
			return MV_TRUE;
	}

	if (ethPortNum == 1) {
		if (mvBoardMppModulesCfgGet(1) & MV_BOARD_RGMII1)
			return MV_TRUE;
	}

	return MV_FALSE;
}

MV_32 mvBoardSwitchPortGet(MV_U32 switchIdx, MV_U32 boardPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchPortGet: Board unknown.\n");
		return -1;
	}
	if (boardPortNum >= BOARD_ETH_SWITCH_PORT_NUM) {
		mvOsPrintf("mvBoardSwitchPortGet: Illegal board port number.\n");
		return -1;
	}
	if ((BOARD_INFO(boardId)->switchInfoNum == 0) || (switchIdx >= BOARD_INFO(boardId)->switchInfoNum))
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[switchIdx].switchPort[boardPortNum];
}

MV_32 mvBoardSwitchConnectedPortGet(MV_U32 ethPort)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchConnectedPortGet: Board unknown.\n");
		return -1;
	}
	if (BOARD_INFO(boardId)->switchInfoNum == 0)
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[0].connectedPort[ethPort];
}

MV_32 mvBoardSwitchCpuPortGet(MV_U32 switchIdx)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchCpuPortGet: Board unknown.\n");
		return -1;
	}
	if ((BOARD_INFO(boardId)->switchInfoNum == 0) || (switchIdx >= BOARD_INFO(boardId)->switchInfoNum))
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[switchIdx].cpuPort;
}

MV_32 mvBoardSwitchIrqGet(MV_VOID)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardLinkStatusIrqGet: Board unknown.\n");
		return -1;
	}
	if (BOARD_INFO(boardId)->switchInfoNum == 0)
		return -1;

	return BOARD_INFO(boardId)->pSwitchInfo[0].switchIrq;
}

MV_BOOL mvBoardIsQsgmiiModuleConnected(MV_VOID)
{
	return MV_FALSE;
}

MV_32 mvBoardGePhySwitchPortGet(MV_VOID)
{
	return -1;
}

MV_32 mvBoardRgmiiASwitchPortGet(MV_VOID)
{
	return -1;
}

MV_32 mvBoardSwitchPortMap(MV_U32 switchIdx, MV_U32 switchPortNum)
{
	int i;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSwitchPortMap: Board unknown.\n");
		return -1;
	}
	if ((BOARD_INFO(boardId)->switchInfoNum == 0) || (switchIdx >= BOARD_INFO(boardId)->switchInfoNum))
		return -1;

	for (i = 0; i < BOARD_ETH_SWITCH_PORT_NUM; i++) {
		if (BOARD_INFO(boardId)->pSwitchInfo[switchIdx].switchPort[i] == switchPortNum)
			return i;
	}
	return -1;
}

MV_32 mvBoardPhyAddrGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardPhyAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr;
}
 
MV_32 mvBoardQuadPhyAddr0Get(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardQuadPhyAddr0Get: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardEthSmiAddr0;
}

MV_32 mvBoardPhyLinkCryptPortAddrGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardPhyLinkCryptPortAddrGet: Board unknown.\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].LinkCryptPortAddr;
}

MV_BOARD_MAC_SPEED mvBoardMacSpeedGet(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardMacSpeedGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo) {
		mvOsPrintf("mvBoardMacSpeedGet: illegal port number\n");
		return MV_ERROR;
	}

	return BOARD_INFO(boardId)->pBoardMacInfo[ethPortNum].boardMacSpeed;
}

MV_BOOL mvBoardSpecInitGet(MV_U32 *regOff, MV_U32 *data)
{
	return MV_FALSE;
}

MV_U32 mvBoardTclkGet(MV_VOID)
{
#ifdef TCLK_AUTO_DETECT
	if ((MV_REG_READ(MPP_SAMPLE_AT_RESET) & MSAR_TCLK_MASK) != 0)
		return MV_BOARD_TCLK_200MHZ;
	else
		return MV_BOARD_TCLK_166MHZ;
#else
	return MV_BOARD_TCLK_200MHZ;
#endif
}

MV_U32 mvBoardSysClkGet(MV_VOID)
{
#ifdef SYSCLK_AUTO_DETECT
	MV_U32 idx;
	MV_U32 cpuFreqMhz, ddrFreqMhz;
	MV_CPU_ARM_CLK_RATIO clockRatioTbl[] = MV_DDR_L2_CLK_RATIO_TBL;

	idx = MSAR_DDR_L2_CLK_RATIO_IDX(MV_REG_READ(MPP_SAMPLE_AT_RESET));

	if (clockRatioTbl[idx].vco2cpu != 0) {	 
		cpuFreqMhz = mvCpuPclkGet() / 1000000;	 
		cpuFreqMhz *= clockRatioTbl[idx].vco2cpu;	 
		ddrFreqMhz = cpuFreqMhz / clockRatioTbl[idx].vco2ddr;
		 
		if (((cpuFreqMhz % clockRatioTbl[idx].vco2ddr) * 10 / clockRatioTbl[idx].vco2ddr) >= 5)
			ddrFreqMhz++;

		return ddrFreqMhz * 1000000;
	} else
		return 0;
#else
	return MV_BOARD_DEFAULT_SYSCLK;
#endif
}

MV_U32 mvBoardDebugLedNumGet(MV_U32 boardId)
{
	return BOARD_INFO(boardId)->activeLedsNumber;
}

MV_VOID mvBoardDebugLed(MV_U32 hexNum)
{
	MV_U32 val[MV_GPP_MAX_GROUP] = {0};
	MV_U32 mask[MV_GPP_MAX_GROUP] = {0};
	MV_U32 digitMask;
	MV_U32 i, pinNum, gppGroup;
	MV_U32 boardId = mvBoardIdGet();

	if (BOARD_INFO(boardId)->pLedGppPin == NULL)
		return;

	hexNum &= (1 << BOARD_INFO(boardId)->activeLedsNumber) - 1;

	for (i = 0, digitMask = 1; i < BOARD_INFO(boardId)->activeLedsNumber; i++, digitMask <<= 1) {
			pinNum = BOARD_INFO(boardId)->pLedGppPin[i];
			gppGroup = pinNum / 32;
			if (hexNum & digitMask)
				val[gppGroup]  |= (1 << (pinNum - gppGroup * 32));
			mask[gppGroup] |= (1 << (pinNum - gppGroup * 32));
	}

	for (gppGroup = 0; gppGroup < MV_GPP_MAX_GROUP; gppGroup++) {
		 
		if (mask[gppGroup])
			mvGppValueSet(gppGroup, mask[gppGroup], BOARD_INFO(boardId)->ledsPolarity == 0 ?
					val[gppGroup] : ~val[gppGroup]);
	}
}

MV_32 mvBoarGpioPinNumGet(MV_BOARD_GPP_CLASS gppClass, MV_U32 index)
{
	MV_U32 boardId, i;
	MV_U32 indexFound = 0;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardRTCGpioPinGet:Board unknown.\n");
		return MV_ERROR;
	}

	for (i = 0; i < BOARD_INFO(boardId)->numBoardGppInfo; i++) {
		if (BOARD_INFO(boardId)->pBoardGppInfo[i].devClass == gppClass) {
			if (indexFound == index)
				return (MV_U32) BOARD_INFO(boardId)->pBoardGppInfo[i].gppPinNum;
			else
				indexFound++;
		}
	}
	return MV_ERROR;
}

MV_VOID mvBoardReset(MV_VOID)
{
	MV_32 resetPin;

	resetPin = mvBoardResetGpioPinGet();
	if (resetPin != MV_ERROR) {
		MV_REG_BIT_RESET(GPP_DATA_OUT_REG(0), (1 << resetPin));
		MV_REG_BIT_RESET(GPP_DATA_OUT_EN_REG(0), (1 << resetPin));
	} else {
		 
		MV_REG_BIT_SET(CPU_RSTOUTN_MASK_REG, BIT0);
		MV_REG_BIT_SET(CPU_SYS_SOFT_RST_REG, BIT0);
	}
}

MV_32 mvBoardResetGpioPinGet(MV_VOID)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_RESET, 0);
}

MV_32 mvBoardSDIOGpioPinGet(MV_BOARD_GPP_CLASS type)
{
	if ((type != BOARD_GPP_SDIO_POWER) && (type != BOARD_GPP_SDIO_DETECT) && (type != BOARD_GPP_SDIO_WP))
		return MV_FAIL;

	return mvBoarGpioPinNumGet(type, 0);
}

MV_32 mvBoardUSBVbusGpioPinGet(MV_32 devId)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_USB_VBUS, devId);
}

MV_32 mvBoardUSBVbusEnGpioPinGet(MV_32 devId)
{
	return mvBoarGpioPinNumGet(BOARD_GPP_USB_VBUS_EN, devId);
}

MV_U32 mvBoardGpioIntMaskGet(MV_U32 gppGrp)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGpioIntMaskGet:Board unknown.\n");
		return MV_ERROR;
	}

	switch (gppGrp) {
	case (0):
		return BOARD_INFO(boardId)->intsGppMaskLow;
		break;
	case (1):
		return BOARD_INFO(boardId)->intsGppMaskMid;
		break;
	case (2):
		return BOARD_INFO(boardId)->intsGppMaskHigh;
		break;
	default:
		return MV_ERROR;
	}
}

MV_32 mvBoardMppGet(MV_U32 mppGroupNum)
{
	MV_U32 boardId;
	MV_U32 mppMod;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardMppGet:Board unknown.\n");
		return MV_ERROR;
	}

	if (mppGroupNum >= BOARD_INFO(boardId)->numBoardMppConfigValue)
		mppMod = 0;  

	return BOARD_INFO(boardId)->pBoardMppConfigValue[mppMod].mppGroup[mppGroupNum];
}

MV_U32 mvBoardGppConfigGet(void)
{
	MV_U32 boardId, i;
	MV_U32 result = 0;
	MV_U32 gpp;

	boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGppConfigGet: Board unknown.\n");
		return 0;
	}

	for (i = 0; i < BOARD_INFO(boardId)->numBoardGppInfo; i++) {
		if (BOARD_INFO(boardId)->pBoardGppInfo[i].devClass == BOARD_GPP_CONF) {
			gpp = BOARD_INFO(boardId)->pBoardGppInfo[i].gppPinNum;
			result <<= 1;
			result |= (mvGppValueGet(gpp >> 5, 1 << (gpp & 0x1F)) >> (gpp & 0x1F));
		}
	}
	return result;

}

MV_32 mvBoardTdmSpiModeGet(MV_VOID)
{
	return DUAL_CHIP_SELECT_MODE;
}

MV_U8 mvBoardTdmDevicesCountGet(void)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_16 index;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardTdmDevicesCountGet: Board unknown.\n");
		return 0;
	}

	index = BOARD_INFO(boardId)->boardTdmInfoIndex;
	if (index == (MV_8)-1)
		return 0;

	return BOARD_INFO(boardId)->numBoardTdmInfo[(MV_U8)index];
}

MV_U8 mvBoardTdmSpiCsGet(MV_U8 devId)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_16 index;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardTdmDevicesCountGet: Board unknown.\n");
		return -1;
	}

	index = BOARD_INFO(boardId)->boardTdmInfoIndex;
	if (index == (MV_8)-1)
		return 0;

	if (devId >= BOARD_INFO(boardId)->numBoardTdmInfo[(MV_U8)index])
		return -1;

	return BOARD_INFO(boardId)->pBoardTdmInt2CsInfo[(MV_U8)index][devId].spiCs;
}

MV_U8 mvBoardTdmSpiIdGet(MV_VOID)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardTdmSpiIdGet: Board unknown.\n");
		return -1;
	}

	return BOARD_INFO(boardId)->pBoardTdmSpiInfo[0].spiId;
}

MV_U32 mvBoardSerdesModeGet(void)
{
	MV_U32 serdesInfo = MV_REG_READ(SERDES_LINE_MUX_REG_0_3);
	MV_U32 serdesMode = 0;

	switch (serdesInfo & 0x0f){
	case 1:
		serdesMode |= SRDS_MOD_PCIE0_LANE0;
		break;
	case 2:
		serdesMode |= SRDS_MOD_SATA0_LANE0;
		break;
	case 3:
		serdesMode |= SRDS_MOD_SGMII1_LANE0;
		break;
	case 0:
	default:
		break;
	}

	switch (serdesInfo & 0x0f0){
	case 0x10:
		serdesMode |= SRDS_MOD_PCIE1_LANE1;
		break;
	case 0x20:
		serdesMode |= SRDS_MOD_SGMII0_LANE1;
		break;
	default:
		break;
	}

	switch (serdesInfo & 0x0f00){
	case 0x100:
		serdesMode |= SRDS_MOD_SATA0_LANE2;
		break;
	case 0x200:
		serdesMode |= SRDS_MOD_SGMII0_LANE2;
		break;
	default:
		break;
	}

	switch (serdesInfo & 0xf000){
	case 0x1000:
		serdesMode |= SRDS_MOD_SATA1_LANE3;
		break;
	case 0x2000:
		serdesMode |= SRDS_MOD_SGMII1_LANE3;
		break;
	default:
		break;
	}

	return serdesMode;
}
 
MV_VOID mvBoardMppModuleTypePrint(MV_VOID)
{
	MV_U32 mppGrp1 = mvBoardMppModulesCfgGet(1);
	MV_U32 mppGrp2 = mvBoardMppModulesCfgGet(2);

	mvOsOutput("Modules/Interfaces Detected:\n");

	if (((mppGrp1 & MV_BOARD_TDM) || (mppGrp2 & MV_BOARD_TDM)) && mvCtrlTdmSupport())
		mvOsOutput("       TDM Module\n");

	if ((mppGrp1 & MV_BOARD_I2S) || (mppGrp2 & MV_BOARD_I2S))
		mvOsOutput("       I2S Module\n");

	if (mppGrp1 & MV_BOARD_GMII0)
		mvOsOutput("       GMII0 Module\n");

	if (mppGrp1 & MV_BOARD_SDIO)
		mvOsOutput("       SDIO\n");

	if (mppGrp1 & MV_BOARD_RGMII0)
			mvOsOutput("       RGMII0 Phy\n");

	if (mppGrp1 & MV_BOARD_RGMII1) {
		if (mvBoardIsSwitchConnected())
			mvOsOutput("       RGMII1 Switch module\n");
		else
			mvOsOutput("       RGMII1 Phy\n");
	}

	return;
}

MV_VOID mvBoardOtherModuleTypePrint(MV_VOID)
{
	MV_U32 srdsCfg = mvBoardSerdesModeGet();

	if (srdsCfg & SRDS_MOD_PCIE0_LANE0)
		mvOsOutput("       PEX0 (Lane 0)\n");
	if (srdsCfg & SRDS_MOD_PCIE1_LANE1)
		mvOsOutput("       PEX1 (Lane 1)\n");

	if (srdsCfg & SRDS_MOD_SATA0_LANE0)
		mvOsOutput("       SATA0 (Lane 0)\n");
	if (srdsCfg & SRDS_MOD_SATA0_LANE2)
		mvOsOutput("       SATA0 (Lane 2)\n");
	if (srdsCfg & SRDS_MOD_SATA1_LANE3 && (mvCtrlSataMaxPortGet() == 2))
		mvOsOutput("       SATA1 (Lane 3)\n");

	if (srdsCfg & SRDS_MOD_SGMII0_LANE1)
		mvOsOutput("       SGMII0 Phy module (Lane 1)\n");
	if (srdsCfg & SRDS_MOD_SGMII0_LANE2)
		mvOsOutput("       SGMII0 Phy module (Lane 2)\n");
	if (srdsCfg & SRDS_MOD_SGMII1_LANE0)
		mvOsOutput("       SGMII1 Phy module (Lane 0)\n");
	if (srdsCfg & SRDS_MOD_SGMII1_LANE3)
		mvOsOutput("       SGMII1 Phy module (Lane 3)\n");

	return;
}

MV_BOOL mvBoardIsGbEPortConnected(MV_U32 ethPortNum)
{
	MV_U32 boardId = mvBoardIdGet();
	MV_U32 mppMask;
	MV_U32 srdsMask = mvBoardSerdesModeGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsGbEPortConnected: Board unknown.\n");
		return -1;
	}

	if (ethPortNum >= BOARD_INFO(boardId)->numBoardMacInfo)
		return MV_FALSE;

	mppMask = BOARD_INFO(boardId)->pBoardModTypeValue->boardMppGrp1Mod;

	if ((ethPortNum == 0) && (((mppMask & (MV_BOARD_RGMII0 | MV_BOARD_GMII0))) ||
			(srdsMask & (SRDS_MOD_SGMII0_LANE1 | SRDS_MOD_SGMII0_LANE2))))
		return MV_TRUE;

	if ((ethPortNum == 1) && ((mppMask & MV_BOARD_RGMII1) ||
			(srdsMask & (SRDS_MOD_SGMII1_LANE0 | SRDS_MOD_SGMII1_LANE3))))
		return MV_TRUE;

	return MV_FALSE;
}

MV_32 mvBoardGetDevicesNumber(MV_BOARD_DEV_CLASS devClass)
{
	MV_U32 foundIndex = 0, devNum;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardGetDeviceNumber:Board unknown.\n");
		return 0xFFFFFFFF;
	}

	for (devNum = START_DEV_CS; devNum < BOARD_INFO(boardId)->numBoardDeviceIf; devNum++) {
		if (BOARD_INFO(boardId)->pDevCsInfo[devNum].devClass == devClass)
			foundIndex++;
	}

	return foundIndex;
}

MV_32 mvBoardGetDeviceBaseAddr(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return mvCpuIfTargetWinBaseLowGet(DEV_TO_TARGET(devEntry->deviceCS));

	return 0xFFFFFFFF;
}

MV_32 mvBoardGetDeviceBusWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->busWidth;

	return 0xFFFFFFFF;
}

MV_32 mvBoardGetDeviceWidth(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->devWidth;

	return MV_ERROR;
}

MV_32 mvBoardGetDeviceWinSize(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return mvCpuIfTargetWinSizeGet(DEV_TO_TARGET(devEntry->deviceCS));

	return 0xFFFFFFFF;
}

static MV_DEV_CS_INFO *boardGetDevEntry(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_U32 foundIndex = 0, devIndex;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("boardGetDevEntry: Board unknown.\n");
		return NULL;
	}

	for (devIndex = START_DEV_CS; devIndex < BOARD_INFO(boardId)->numBoardDeviceIf; devIndex++) {
		if (BOARD_INFO(boardId)->pDevCsInfo[devIndex].devClass == devClass) {
			if (foundIndex == devNum)
				return &(BOARD_INFO(boardId)->pDevCsInfo[devIndex]);
			foundIndex++;
		}
	}

	return NULL;
}

MV_U32 boardGetDevCSNum(MV_32 devNum, MV_BOARD_DEV_CLASS devClass)
{
	MV_DEV_CS_INFO *devEntry;
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("Board unknown.\n");
		return 0xFFFFFFFF;
	}

	devEntry = boardGetDevEntry(devNum, devClass);
	if (devEntry != NULL)
		return devEntry->deviceCS;

	return 0xFFFFFFFF;
}

MV_U8 mvBoardTwsiAddrTypeGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId = mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++) {
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == twsiClass) {
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddrType;
			else
				indexFound++;
		}
	}
	return (MV_ERROR);
}

MV_U8 mvBoardTwsiAddrGet(MV_BOARD_TWSI_CLASS twsiClass, MV_U32 index)
{
	int i;
	MV_U32 indexFound = 0;
	MV_U32 boardId = mvBoardIdGet();

	for (i = 0; i < BOARD_INFO(boardId)->numBoardTwsiDev; i++) {
		if (BOARD_INFO(boardId)->pBoardTwsiDev[i].devClass == twsiClass) {
			if (indexFound == index)
				return BOARD_INFO(boardId)->pBoardTwsiDev[i].twsiDevAddr;
			else
				indexFound++;
		}
	}
	return (0xFF);
}

MV_32 mvBoardNandWidthGet(void)
{
	MV_U32 devNum;
	MV_U32 devWidth;
	MV_U32 boardId = mvBoardIdGet();

	for (devNum = START_DEV_CS; devNum < BOARD_INFO(boardId)->numBoardDeviceIf; devNum++) {
		devWidth = mvBoardGetDeviceWidth(devNum, BOARD_DEV_NAND_FLASH);
		if (devWidth != MV_ERROR)
			return (devWidth / 8);
	}

	return MV_ERROR;
}

MV_U32 gBoardId = -1;
 
MV_VOID mvBoardIdSet(MV_VOID)
{
	if (gBoardId == -1) {
#if defined(DB_88F6710)
		gBoardId = DB_88F6710_BP_ID;
#elif defined(DB_88F6710_PCAC)
		gBoardId = DB_88F6710_PCAC_ID;
#elif defined(RD_88F6710)
		gBoardId = RD_88F6710_ID;
#else
		mvOsPrintf("mvBoardIdSet: Board ID must be defined!\n");
		while (1) {
			continue;
		}
#endif
	}
}
 
MV_U32 mvBoardIdGet(MV_VOID)
{
	if (gBoardId == -1) {
#if defined(DB_88F6710)
		gBoardId = DB_88F6710_BP_ID;
#elif defined(DB_88F6710_PCAC)
		gBoardId = DB_88F6710_PCAC_ID;
#elif defined(RD_88F6710)
		gBoardId = RD_88F6710_ID;
#else
		mvOsWarning();
		return INVALID_BAORD_ID;
#endif
	}

	return gBoardId;
}

MV_U8 mvBoardTwsiSatRGet(MV_U8 devNum, MV_U8 regNum)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;
	MV_U8 data;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	DB(mvOsPrintf("Board: Read S@R device read\n"));
	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(BOARD_DEV_TWSI_SATR, devNum);
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(BOARD_DEV_TWSI_SATR, devNum);

	twsiSlave.validOffset = MV_TRUE;
	 
	twsiSlave.offset = regNum;
	twsiSlave.moreThen256 = MV_FALSE;

	if (MV_OK != mvTwsiRead(0, &twsiSlave, &data, 1)) {
		DB(mvOsPrintf("Board: Read S@R fail\n"));
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Read S@R succeded\n"));

	return data;
}

MV_STATUS mvBoardTwsiSatRSet(MV_U8 devNum, MV_U8 regNum, MV_U8 regVal)
{
	MV_TWSI_SLAVE twsiSlave;
	MV_TWSI_ADDR slave;

	slave.type = ADDR7_BIT;
	slave.address = 0;
	mvTwsiInit(0, TWSI_SPEED, mvBoardTclkGet(), &slave, 0);

	twsiSlave.slaveAddr.address = mvBoardTwsiAddrGet(BOARD_DEV_TWSI_SATR, devNum);
	twsiSlave.slaveAddr.type = mvBoardTwsiAddrTypeGet(BOARD_DEV_TWSI_SATR, devNum);
	twsiSlave.validOffset = MV_TRUE;
	DB(mvOsPrintf("Board: Write S@R device addr %x, type %x, data %x\n",
		      twsiSlave.slaveAddr.address, twsiSlave.slaveAddr.type, regVal));
	 
	twsiSlave.offset = regNum;
	twsiSlave.moreThen256 = MV_FALSE;
	if (MV_OK != mvTwsiWrite(0, &twsiSlave, &regVal, 1)) {
		DB1(mvOsPrintf("Board: Write S@R fail\n"));
		return MV_ERROR;
	}
	DB(mvOsPrintf("Board: Write S@R succeded\n"));

	return MV_OK;
}

static MV_U8 mvBoardSatrSwapBits(MV_U8 val, MV_U8 width)
{
	MV_U8 i;
	MV_U8 res = 0;

	for (i = 0; i < width; i++) {
		if ((1 << i) & val)
			res |= (1 << (width - i - 1));
	}
	return res;
}

MV_U8 mvBoardFabFreqGet(MV_VOID)
{
	MV_U8 sar0, sar1, res;

	sar0 = mvBoardTwsiSatRGet(0, 0);
	sar1 = mvBoardTwsiSatRGet(1, 0);

	if (((MV_8)MV_ERROR == (MV_8)sar0) || ((MV_8)MV_ERROR == (MV_8)sar1))
		return MV_ERROR;

	res = ((sar0 & 0x10) | (mvBoardSatrSwapBits(sar1, 4) & 0xF));
	return res;
}

MV_STATUS mvBoardFabFreqSet(MV_U8 freqVal)
{
	MV_U8 sar0, sar1;

	sar0 = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar0)
		return MV_ERROR;

	sar1 = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar1)
		return MV_ERROR;

	sar0 &= ~0x10;
	sar0 |= (freqVal & 0x10);
	if (MV_OK != mvBoardTwsiSatRSet(0, 0, sar0)) {
		DB1(mvOsPrintf("Board: Write FreqOpt S@R fail\n"));
		return MV_ERROR;
	}

	sar1 &= ~0xF;
	sar1 |= mvBoardSatrSwapBits(freqVal, 4);
	if (MV_OK != mvBoardTwsiSatRSet(1, 0, sar1)) {
		DB1(mvOsPrintf("Board: Write FreqOpt S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write FreqOpt S@R succeeded\n"));
	return MV_OK;
}

MV_U8 mvBoardCpuFreqGet(MV_VOID)
{
	MV_U8 sar;
	MV_U8 res;

	sar = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	res = sar & 0xF;
	res = mvBoardSatrSwapBits(res, 4);
	return res;
}

MV_STATUS mvBoardCpuFreqSet(MV_U8 freqVal)
{
	MV_U8 sar;

	sar = mvBoardTwsiSatRGet(0, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	freqVal = mvBoardSatrSwapBits(freqVal, 4);
	sar &= ~0xF;
	sar |= (freqVal & 0xF);
	if (MV_OK != mvBoardTwsiSatRSet(0, 0, sar)) {
		DB1(mvOsPrintf("Board: Write CpuFreq S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write CpuFreq S@R succeeded\n"));
	return MV_OK;
}

MV_U8 mvBoardBootDevGet(MV_VOID)
{
	MV_U8 sar;
	MV_U8 result;

	sar = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	result = ((sar & 0x10) << 1);

	sar = mvBoardTwsiSatRGet(2, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;
	result |= mvBoardSatrSwapBits(sar, 5);

	return result;
}

MV_STATUS mvBoardBootDevSet(MV_U8 val)
{
	MV_U8 sar;

	sar = mvBoardTwsiSatRGet(1, 0);
	if ((MV_8)MV_ERROR == (MV_8)sar)
		return MV_ERROR;

	sar &= ~(0x10);
	sar |= ((val & 0x20) >> 1);
	if (MV_OK != mvBoardTwsiSatRSet(1, 0, sar)) {
		DB1(mvOsPrintf("Board: Write BootDev S@R fail\n"));
		return MV_ERROR;
	}

	if (MV_OK != mvBoardTwsiSatRSet(2, 0, mvBoardSatrSwapBits(val, 5))) {
		DB1(mvOsPrintf("Board: Write BootDev S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write BootDev S@R succeeded\n"));
	return MV_OK;
}

MV_STATUS mvBoardPexCapabilitySet(MV_U16 conf)
{
	if (MV_OK != mvBoardTwsiSatRSet(1, 1, conf)) {
		DB(mvOsPrintf("Board: Write confID S@R fail\n"));
		return MV_ERROR;
	}

	DB(mvOsPrintf("Board: Write confID S@R succeeded\n"));
	return MV_OK;
}

MV_U16 mvBoardPexCapabilityGet(MV_VOID)
{
	MV_U8 sar;

	sar = mvBoardTwsiSatRGet(1, 1);
	return (sar & 0xFF);
}

MV_STATUS mvBoardMppModulesScan(void)
{
	MV_U8 regVal;
	MV_TWSI_SLAVE twsiSlave, twsiSlaveGMII;
	MV_U32 boardId = mvBoardIdGet();
	MV_BOOL scanEn = mvBoardIsModScanEnabled();
	MV_BOARD_MODULE_TYPE_INFO *modInfo;
	MV_U8 swCfg;

	if (scanEn == MV_FALSE)
		return MV_OK;

	modInfo = BOARD_INFO(boardId)->pBoardModTypeValue;

	modInfo->boardMppGrp1Mod  = 0;			 
	modInfo->boardMppGrp2Mod  = 0;			 

	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_GIGA_CON_ADDR);
	FILL_TWSI_SLAVE(twsiSlaveGMII, MV_BOARD_GIGA_CON_GMII_ADDR);

	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		switch (regVal & MV_BOARD_MODULE_ID_MASK) {
		 
		case (MV_BOARD_SWITCH_MODULE_ID):
			modInfo->boardMppGrp1Mod |= MV_BOARD_RGMII0;
			modInfo->boardMppGrp1Mod |= MV_BOARD_RGMII1;
			break;
		 
		case (MV_BOARD_TDM_GMII_MODULE_TDM_ID):
			modInfo->boardMppGrp1Mod |= MV_BOARD_TDM;
			break;
		 
		case (MV_BOARD_I2S_SPDIF_MODULE_ID):
			modInfo->boardMppGrp1Mod |= MV_BOARD_I2S;
			break;
		default:
			break;
		}

	} else if (mvTwsiRead(0, &twsiSlaveGMII, &regVal, 1) == MV_OK) {
		if ((regVal & MV_BOARD_MODULE_ID_MASK) == MV_BOARD_TDM_GMII_MODULE_GMII_ID)
			modInfo->boardMppGrp1Mod |= MV_BOARD_GMII0;

	} else {  
		 
		FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_EEPROM_MODULE_ADDR);
		mvTwsiRead(0, &twsiSlave, &swCfg, 1);
		if (MV_BOARD_CFG_SDIO_MODE(swCfg) == 1)
			modInfo->boardMppGrp1Mod |= MV_BOARD_SDIO;
		else
			modInfo->boardMppGrp1Mod |= MV_BOARD_RGMII0;
		 
		modInfo->boardMppGrp1Mod |= MV_BOARD_RGMII1;
	}

	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_DEVICE_CON_ADDR);
	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		switch (regVal & MV_BOARD_MODULE_ID_MASK) {
		 
		case (MV_BOARD_TDM_GMII_MODULE_TDM_ID):
			modInfo->boardMppGrp2Mod |= MV_BOARD_TDM;
			break;
		 
		case (MV_BOARD_I2S_SPDIF_MODULE_ID):
			modInfo->boardMppGrp2Mod |= MV_BOARD_I2S;
			break;
		default:
			break;
		}
	} else {  

	}

	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_SERDES_CON_ADDR);
	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		modInfo->boardMppGrp1Mod &= ~MV_BOARD_RGMII0;
		modInfo->boardMppGrp1Mod &= ~MV_BOARD_RGMII1;
		modInfo->boardMppGrp1Mod &= ~MV_BOARD_GMII0;
	}

	return MV_OK;
}

static MV_U8 mvBoardMppTypeIndexGet(MV_BOARD_MPP_TYPE_CLASS type)
{
	MV_U8 i = 0;

	while (gBoardMppType2Index[i] != 0xFFFFFFFF) {
		if (gBoardMppType2Index[i] == type)
			return i;
		i++;
	}

	return 0x0;
}

MV_STATUS mvBoardUpdateMppAfterScan(void)
{
	MV_BOOL scanEn = mvBoardIsModScanEnabled();
	MV_U32 boardId = mvBoardIdGet();
	MV_U32 *mppList = BOARD_INFO(boardId)->pBoardMppConfigValue->mppGroup;
	MV_U32 mppGroup1[][4][2] = MPP_GROUP_1_TYPE;
	MV_U32 mppGroup2[][4][2] = MPP_GROUP_2_TYPE;
	MV_BOARD_MODULE_TYPE_INFO *modInfo;
	MV_U32 mpp, mppIdx;
	MV_U32 bootVal, mask, width;
	MV_U8 index, i;

	modInfo = BOARD_INFO(boardId)->pBoardModTypeValue;

	if (scanEn == MV_FALSE)
		return MV_OK;

	for (i = 0; i < 32; i++) {
		if (!((1 << i) &  modInfo->boardMppGrp1Mod))
			continue;
		index = mvBoardMppTypeIndexGet((1 << i));
		for (mpp = 0; mpp < 4; mpp++) {
			if (mppGroup1[index][mpp][0] != 0x0) {
				mppList[mpp] &= ~mppGroup1[index][mpp][0];
				mppList[mpp] |= mppGroup1[index][mpp][1];
			}
		}
	}

	for (i = 0; i < 32; i++) {
		if (!((1 << i) &  modInfo->boardMppGrp2Mod))
			continue;
		index = mvBoardMppTypeIndexGet((1 << i));
		for (mpp = 0; mpp <= 4; mpp++) {
			mppIdx = mpp + 4;
			if (mppGroup2[index][mpp][0] != 0x0) {
				mppList[mppIdx] &= ~mppGroup2[index][mpp][0];
				mppList[mppIdx] |= mppGroup2[index][mpp][1];
			}
			width = mvCtrlIsBootFromNAND() || mvCtrlIsBootFromNOR();
			mask = 0x0;
			if (mvCtrlIsBootFromSPI() == MV_SPI_LOW_MPPS) {
				if (mppIdx == 4)
					mask = 0xFFFF0;
			} else if (mvCtrlIsBootFromSPI() == MV_SPI_HIGH_MPPS) {
				switch (mppIdx) {
				case 4:
					mask = 0xF;
					break;
				case 7:
					mask = 0xF0000000;
					break;
				case 8:
					mask = 0xFF;
					break;
				default:
					mask = 0x0;
					break;
				}
			} else if (width) {
				switch (mppIdx) {
				case 4:
					mask = 0xFFFFFFF0;
					break;
				case 5:
					if (width == MV_NAND_NOR_BOOT_8BIT)
						mask = 0x0FFFFFFF;
					else
						mask = 0xFFFFFFFF;
				case 6:
					if (width == MV_NAND_NOR_BOOT_16BIT)
						mask = 0xFFFFFFFF;
					break;
				case 7:
					if (width == MV_NAND_NOR_BOOT_16BIT)
						mask = 0xFFF;
					break;
				default:
					mask = 0x0;
					break;
				}
			}

			if (mask != 0) {
				bootVal = MV_REG_READ(mvCtrlMppRegGet(mppIdx));
				mppList[mppIdx] &= ~mask;
				mppList[mppIdx] |= (bootVal & mask);
			}
		}
	}

	return MV_OK;
}

MV_BOOL mvBoardIsModScanEnabled(void)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsModScanEnabled:Board unknown.\n");
		return MV_FALSE;
	}

	return BOARD_INFO(boardId)->enableModuleScan;
}

MV_BOOL mvBoardIsSwitchConnected(void)
{
	MV_U8 regVal;
	MV_TWSI_SLAVE twsiSlave;
	MV_U32 boardId = mvBoardIdGet();
	MV_BOOL scanEn = mvBoardIsModScanEnabled();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsSwitchConnected:Board unknown.\n");
		return MV_FALSE;
	}

	if (scanEn == MV_FALSE) {
		if (BOARD_INFO(boardId)->switchInfoNum > 0)
			return MV_TRUE;
		return MV_FALSE;
	}

	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_GIGA_CON_ADDR);
	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		if ((regVal & MV_BOARD_MODULE_ID_MASK) == MV_BOARD_SWITCH_MODULE_ID)
			return MV_TRUE;
	}

	return MV_FALSE;
}

MV_BOOL mvBoardIsGMIIConnected(void)
{
	MV_U8 regVal;
	MV_TWSI_SLAVE twsiSlave;
	MV_U32 boardId = mvBoardIdGet();
	MV_BOOL scanEn = mvBoardIsModScanEnabled();

	if (scanEn == MV_FALSE)
		return MV_FALSE;

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardIsGMIIConnected:Board unknown.\n");
		return MV_FALSE;
	}

	FILL_TWSI_SLAVE(twsiSlave, MV_BOARD_GIGA_CON_GMII_ADDR);
	if (mvTwsiRead(0, &twsiSlave, &regVal, 1) == MV_OK) {
		if ((regVal & MV_BOARD_MODULE_ID_MASK) == MV_BOARD_TDM_GMII_MODULE_GMII_ID)
			return MV_TRUE;
	}

	return MV_FALSE;
}

MV_32 mvBoardSmiScanModeGet(MV_U32 switchIdx)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardSmiScanModeGet: Board unknown.\n");
		return -1;
	}

	return BOARD_INFO(boardId)->pSwitchInfo[switchIdx].smiScanMode;
}

MV_STATUS mvBoardUpdateEthAfterScan(void)
{
	MV_BOOL scanEn = mvBoardIsModScanEnabled();
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardUpdateEthAfterScan:Board unknown.\n");
		return MV_ERROR;
	}

	if (scanEn == MV_FALSE)
		return MV_OK;

	if (MV_TRUE == mvBoardIsSwitchConnected()) {
		 
		BOARD_INFO(boardId)->pBoardMacInfo[1].boardMacSpeed = BOARD_MAC_SPEED_1000M;
		BOARD_INFO(boardId)->pBoardMacInfo[1].boardEthSmiAddr = 0x10;
	} else {
		BOARD_INFO(boardId)->pSwitchInfo = NULL;
		BOARD_INFO(boardId)->switchInfoNum = 0;
	}

	if (MV_TRUE == mvBoardIsGMIIConnected())
		BOARD_INFO(boardId)->pBoardMacInfo[0].boardEthSmiAddr = 0x8;

	return MV_OK;
}

MV_U32 mvBoardMppModulesCfgGet(MV_U8 group)
{
	MV_U32 boardId = mvBoardIdGet();

	if (!((boardId >= BOARD_ID_BASE) && (boardId < MV_MAX_BOARD_ID))) {
		mvOsPrintf("mvBoardMppModulesCfgGet: Board unknown.\n");
		return MV_ERROR;
	}

	if (group == 1)
		return BOARD_INFO(boardId)->pBoardModTypeValue->boardMppGrp1Mod;
	else
		return BOARD_INFO(boardId)->pBoardModTypeValue->boardMppGrp2Mod;
}

MV_BOARD_PEX_INFO *mvBoardPexInfoGet(void)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	switch (boardId) {
	case DB_88F6710_BP_ID:
	case DB_88F6710_PCAC_ID:
	case RD_88F6710_ID:
#if defined(CONFIG_SYNO_ARMADA_ARCH)
	case SYNO_DS213j_ID:
	case SYNO_US3_ID:
	case SYNO_RS214_ID:
	case SYNO_DS214se_ID:
	case SYNO_DS414slim_ID:
	case SYNO_DS115j_ID:
	case SYNO_DS216se_ID:
#endif
		return &BOARD_INFO(boardId)->boardPexInfo;
		break;
	default:
		DB(mvOsPrintf("mvBoardPexInfoGet: Unsupported board!\n"));
		return NULL;
	}
}

MV_VOID mvBoardBitMaskConfigSet(MV_U32 config)
{
	MV_U32 boardId;

	boardId = mvBoardIdGet();

	if (boardId == RD_88F6710_ID) {
		 
		if (config & BIT0) {
			MV_REG_BIT_SET(GPP_DATA_OUT_REG(1), BIT31);
			MV_REG_BIT_RESET(GPP_DATA_OUT_REG(2), BIT0);
			MV_REG_BIT_SET(GPP_DATA_OUT_REG(2), BIT1);
		} else {
			MV_REG_BIT_RESET(GPP_DATA_OUT_REG(1), BIT31);
			MV_REG_BIT_SET(GPP_DATA_OUT_REG(2), BIT0);
			MV_REG_BIT_RESET(GPP_DATA_OUT_REG(2), BIT1);
		}
	}
}

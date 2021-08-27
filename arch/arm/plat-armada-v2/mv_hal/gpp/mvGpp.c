 
#include "mvCommon.h"
#include "mvOs.h"
#include "ctrlEnv/mvCtrlEnvSpec.h"
#include "mvSysGppConfig.h"
#include "mvGppRegs.h"
#include "mvGpp.h"

#ifdef MV_DEBUG
#define DB(x)	x
#else
#define DB(x)
#endif

static MV_GPP_HAL_DATA gppHalData;

MV_STATUS mvGppInit(MV_GPP_HAL_DATA *halData)
{
	mvOsMemcpy(&gppHalData, halData, sizeof(MV_GPP_HAL_DATA));
	return MV_OK;
}

MV_STATUS mvGppTypeSet(MV_U32 group, MV_U32 mask, MV_U32 value)
{
	if (group >= MV_GPP_MAX_GROUP) {
		DB(mvOsPrintf("mvGppTypeSet: ERR. invalid group number \n"));
		return MV_BAD_PARAM;
	}

	gppRegSet(group, GPP_DATA_OUT_EN_REG(group), mask, value);

	if (gppHalData.ctrlRev == MV_88F6XXX_A0_REV && (group == 1)) {
		mask &= 0x2;
		gppRegSet(0, GPP_DATA_OUT_EN_REG(0), mask, value);
	}
	 
	return MV_OK;
}

MV_STATUS mvGppBlinkEn(MV_U32 group, MV_U32 mask, MV_U32 value)
{
	if (group >= MV_GPP_MAX_GROUP) {
		DB(mvOsPrintf("mvGppBlinkEn: ERR. invalid group number \n"));
		return MV_BAD_PARAM;
	}

	gppRegSet(group, GPP_BLINK_EN_REG(group), mask, value);

	return MV_OK;

}

MV_STATUS mvGppPolaritySet(MV_U32 group, MV_U32 mask, MV_U32 value)
{
	if (group >= MV_GPP_MAX_GROUP) {
		DB(mvOsPrintf("mvGppPolaritySet: ERR. invalid group number \n"));
		return MV_BAD_PARAM;
	}

	gppRegSet(group, GPP_DATA_IN_POL_REG(group), mask, value);

	return MV_OK;

}

MV_U32 mvGppPolarityGet(MV_U32 group, MV_U32 mask)
{
	MV_U32 regVal;

	if (group >= MV_GPP_MAX_GROUP) {
		DB(mvOsPrintf("mvGppActiveSet: Error invalid group number \n"));
		return MV_ERROR;
	}
	regVal = MV_REG_READ(GPP_DATA_IN_POL_REG(group));

	return (regVal & mask);
}

MV_U32 mvGppValueGet(MV_U32 group, MV_U32 mask)
{
	MV_U32 gppData;

	gppData = MV_REG_READ(GPP_DATA_IN_REG(group));

	gppData &= mask;

	return gppData;

}

MV_STATUS mvGppValueSet(MV_U32 group, MV_U32 mask, MV_U32 value)
{
	MV_U32 outEnable, tmp;
	MV_U32 i;

	if (group >= MV_GPP_MAX_GROUP) {
		DB(mvOsPrintf("mvGppValueSet: Error invalid group number \n"));
		return MV_BAD_PARAM;
	}

	outEnable = ~MV_REG_READ(GPP_DATA_OUT_EN_REG(group));

	if (gppHalData.ctrlRev == MV_88F6XXX_A0_REV && (group == 1)) {
		tmp = ~MV_REG_READ(GPP_DATA_OUT_EN_REG(0));
		outEnable &= 0xfffffffd;
		outEnable |= (tmp & 0x2);
	}
	 
	for (i = 0; i < 32; i++) {
		if (((mask & (1 << i)) & (outEnable & (1 << i))) != (mask & (1 << i))) {
			mvOsPrintf("mvGppValueSet: Err. An attempt to set output "
				   "value to GPP %d in input mode.\n", i);
			return MV_ERROR;
		}
	}

	gppRegSet(group, GPP_DATA_OUT_REG(group), mask, value);

	return MV_OK;

}

MV_VOID gppRegSet(MV_U32 group, MV_U32 regOffs, MV_U32 mask, MV_U32 value)
{
	MV_U32 gppData;

	gppData = MV_REG_READ(regOffs);

	gppData &= ~mask;

	gppData |= (value & mask);

	MV_REG_WRITE(regOffs, gppData);
}

MV_STATUS mvGppAtomicValueClear(MV_U32 gpionumber)
{
	if(gpionumber < 64)
		MV_REG_WRITE(GPP_OUT_CLEAR_REG((int)(gpionumber >> 5)) , 1 << (gpionumber%32));
	else
		MV_REG_WRITE(GPP_64_66_DATA_OUT_CLEAR_REG , 1 << (gpionumber%32));
	return MV_OK;
}

MV_STATUS mvGppAtomicValueSet(MV_U32 gpionumber)
{
	if(gpionumber < 64)
                MV_REG_WRITE(GPP_OUT_SET_REG((int)(gpionumber >> 5)) , 1 << (gpionumber%32));
        else
                MV_REG_WRITE(GPP_64_66_DATA_OUT_SET_REG , 1 << (gpionumber%32));
        return MV_OK;
}

#ifdef CONFIG_SYNO_ARMADA_V2
 
#include <linux/export.h>
EXPORT_SYMBOL(mvGppPolarityGet);
EXPORT_SYMBOL(mvGppPolaritySet);
#endif

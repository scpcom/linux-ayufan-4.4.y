#include "vp880_api_int.h"
#include "vp_api_int.h"

#undef BATT_DEBUG

#ifdef BATT_DEBUG
#define DBG(args...) mvOsPrintf(args)
#else
#define DBG(args...)
#endif

#define EC_1    VP880_EC_CH1
#define EC_2    VP880_EC_CH2
#define EC_BOTH (EC_1 | EC_2)

/* Set device IDs according to the implementation of VpMpiCmd.
 * Chip selects used:
 *  CS0 - empty
 *  CS1 - First device (gDev1Id) (VBH, VBL)
 *  CS2 - Second device (gDev2Id) (VBP)
 */
VpDeviceIdType gDev1Id = 4;
VpDeviceIdType gDev2Id = 5;

/* Calibration arrays initially filled with median values from testing boards
 * 022 and up as of April 5, 2010 */
unsigned short gVbhCalAry[30] = {
	420, 908, 1407, 1901, 2387, 2886, 3387, 3885, 4372, 4872,
	5365, 5863, 6355, 6858, 7353, 7859, 8328, 8817, 9322, 9819,
	10325, 10824, 11319, 11820, 12318, 12814, 13314, 13808, 14320, 14759
};
unsigned short gVblCalAry[30] = {
	406, 898, 1396, 1891, 2378, 2879, 3372, 3879, 4377, 4865,
	5360, 5861, 6360, 6859, 7358, 7856, 8340, 8839, 9335, 9837,
	10334, 10839, 11332, 11837, 12322, 12828, 13322, 13823, 14322, 14685
};
unsigned short gVbpCalAry[30] = {
	635, 1136, 1640, 2135, 2642, 3146, 3647, 4151, 4649, 5151,
	5641, 6143, 6650, 7151, 7650, 8148, 8652, 9156, 9659, 10161,
	10651, 11149, 11654, 12160, 12659, 13158, 13659, 14162, 14672, 15086
};

extern int BattOn(int vbhSetting, int vblSetting, int vbpSetting);

extern int BattOff(void);

static void CalculateRegisters(uint16 target, uint16 *pCalAry, uint8 *pSwReg, uint8 *pCalReg);

static int MpiTest(VpDeviceIdType devId);

int BattOn(int vbhIn, int vblIn, int vbpIn)
{
	uint8 data[20] = { 0 };
	int i;
	uint16 vbhTarget;
	uint16 vblTarget;
	uint16 vbpTarget;
	uint8 vbhHex;
	uint8 vblHex;
	uint8 vbpHex;
	uint8 calVbhHex;
	uint8 calVblHex;
	uint8 calVbpHex;

	/* Argument checking */
	if (vblIn > 0) {
		mvOsPrintf("  VBL must be negative or zero\n");
		return -1;
	}
	if (vbhIn > 0) {
		mvOsPrintf("  VBH must be negative or zero\n");
		return -1;
	}
	if (vbpIn < 0) {
		mvOsPrintf("  VBP must be positive or zero\n");
		return -1;
	}
	if (vblIn - vbhIn < 5 && vblIn != 0) {
		mvOsPrintf("  VBH must be at least 5V more negative than VBL\n");
		return -1;
	}

	/* 16 no-ops to clear MPI buffer */
	for (i = 0; i < 16; i++)
		data[i] = VP880_NO_OP;

	VpMpiCmd(gDev1Id, EC_BOTH, VP880_NO_OP, 16, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_NO_OP, 16, data);

	/* Read RCN/PCN and check that the devices are what we expect */
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_DEVTYPE_CMD, VP880_DEVTYPE_LEN, data);
	if (data[0] != VP880_REV_JE || data[1] != VP880_DEV_PCN_88506) {
		mvOsPrintf("Invalid device 1, RCN/PCN 0x%02X%02X\n", data[0], data[1]);
		return -1;
	}
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_DEVTYPE_CMD, VP880_DEVTYPE_LEN, data);
	if (data[0] != VP880_REV_JE || data[1] != VP880_DEV_PCN_88506) {
		mvOsPrintf("Invalid device 2, RCN/PCN 0x%02X%02X\n", data[0], data[1]);
		return -1;
	}

	/* Run MPI checks on both devices to make sure that it's safe to start
	 * programming the devices.  If the signal integrity is bad, it could be
	 * dangerous to continue. */
	if (MpiTest(gDev1Id) < 0 || MpiTest(gDev2Id) < 0)
		return -1;

	/* HW reset */
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_HW_RESET_CMD, 0, NULL);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_HW_RESET_CMD, 0, NULL);

	/* Configure 8.192MHz mclk */
	data[0] = 0x8A;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_MCLK_CNT_WRT, VP880_MCLK_CNT_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_MCLK_CNT_WRT, VP880_MCLK_CNT_LEN, data);

	/* Unmask CFAIL */
	data[0] = 0xFF & ~(VP880_CFAIL_MASK);
	data[1] = 0xFF;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_INT_MASK_WRT, VP880_INT_MASK_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_INT_MASK_WRT, VP880_INT_MASK_LEN, data);

	/* Make sure CFAIL clears on both devices */
	for (i = 0; i < 11; i++) {
		VpMpiCmd(gDev1Id, EC_BOTH, VP880_UL_SIGREG_RD, VP880_UL_SIGREG_LEN, data);
		if ((data[0] & VP880_CFAIL_MASK) == 0)
			break;

		if (i == 10) {
			mvOsPrintf("Couldn't clear CFAIL on dev 1");
			return -1;
		}
		mvOsDelay(10);
	}
	for (i = 0; i < 11; i++) {
		VpMpiCmd(gDev2Id, EC_BOTH, VP880_UL_SIGREG_RD, VP880_UL_SIGREG_LEN, data);
		if ((data[0] & VP880_CFAIL_MASK) == 0)
			break;

		if (i == 10) {
			mvOsPrintf("Couldn't clear CFAIL on dev 2");
			return -1;
		}
		mvOsDelay(10);
	}

	/* Set the switcher timings */
	data[0] = 0xB2;		/* Device defaults */
	data[1] = 0x00;
	data[2] = 0xB1;
	data[3] = 0x00;
	data[4] = 0xB0;
	data[5] = 0x40;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_INT_SWREG_PARAM_WRT, VP880_INT_SWREG_PARAM_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_INT_SWREG_PARAM_WRT, VP880_INT_SWREG_PARAM_LEN, data);

	/* ICR2 settings: c_tip_on and c_ring_on for measuring in disconnect,
	 * 150V battery limit */
	data[0] = VP880_ICR2_TIP_SENSE | VP880_ICR2_RING_SENSE;
	data[1] = VP880_ICR2_TIP_SENSE | VP880_ICR2_RING_SENSE;
	data[2] = VP880_ICR2_SWY_LIM_CTRL | VP880_ICR2_SWY_LIM_CTRL1;
	data[3] = VP880_ICR2_SWY_LIM_CTRL | VP880_ICR2_SWY_LIM_CTRL1;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_ICR2_WRT, VP880_ICR2_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_ICR2_WRT, VP880_ICR2_LEN, data);

	/* ICR3 settings: VREF ctrl, "line block control circuitry override" for
	 * measuring in disconnect */
	data[0] = VP880_ICR3_VREF_CTRL | VP880_ICR3_LINE_CTRL;
	data[1] = VP880_ICR3_VREF_CTRL | VP880_ICR3_LINE_CTRL;
	data[2] = 0;
	data[3] = 0;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_ICR3_WRT, VP880_ICR3_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_ICR3_WRT, VP880_ICR3_LEN, data);

	/* ICR4 settings: voice ADC override for measuring in disconnect */
	data[0] = VP880_ICR4_VOICE_ADC_CTRL;
	data[1] = VP880_ICR4_VOICE_ADC_CTRL;
	data[2] = 0;
	data[3] = 0;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_ICR4_WRT, VP880_ICR4_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_ICR4_WRT, VP880_ICR4_LEN, data);

	/* Wait for VREF to stabilize */
	mvOsDelay(20);

	/* Disable auto system state ctrl, zero cross ringing, auto clock fail
	 * switching, and thermal fault switching.  Enable auto battery shutdown */
	data[0] = VP880_ACFS_DIS | VP880_ATFS_DIS | VP880_ZXR_DIS | VP880_AUTO_SSC_DIS | VP880_AUTO_BAT_SHUTDOWN_EN;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_SS_CONFIG_WRT, VP880_SS_CONFIG_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_SS_CONFIG_WRT, VP880_SS_CONFIG_LEN, data);

	/* Zero out signal generator params for the channel (ch2 on dev1) that will
	 * be in ringing */
	for (i = 0; i < VP880_SIGA_PARAMS_LEN; i++)
		data[i] = 0;

	VpMpiCmd(gDev1Id, EC_2, VP880_SIGA_PARAMS_WRT, VP880_SIGA_PARAMS_LEN, data);

	/* Calculate the register settings.  Calculations are performed  based on
	 * units of cV and unsigned, for example -65.3 V == 6530 */
	vbhTarget = (ABS(vbhIn) * 100);
	vblTarget = (ABS(vblIn) * 100);
	vbpTarget = (ABS(vbpIn) * 100);

	DBG("VBH Calibration:\n");
	CalculateRegisters(vbhTarget, gVbhCalAry, &vbhHex, &calVbhHex);
	DBG("VBH results, vbhHex %02X, calVbhHex %02X\n\n", vbhHex, calVbhHex);

	DBG("VBL Calibration:\n");
	CalculateRegisters(vblTarget, gVblCalAry, &vblHex, &calVblHex);
	DBG("VBL results, vblHex %02X, calVblHex %02X\n\n", vblHex, calVblHex);

	DBG("VBP Calibration:\n");
	CalculateRegisters(vbpTarget, gVbpCalAry, &vbpHex, &calVbpHex);
	DBG("VBP results, vbpHex %02X, calVbpHex %02X\n\n", vbpHex, calVbpHex);

	/* Set up switching regulator params - fixed, programmable */
	data[0] = VP880_FLYBACK_MODE | VP880_ZRING_TRACK_DIS | VP880_YRING_TRACK_DIS;
	data[1] = VP880_SWY_AUTOPOWER_DIS | vbhHex;
	data[2] = VP880_SWZ_AUTOPOWER_DIS | vblHex;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_REGULATOR_PARAM_WRT, VP880_REGULATOR_PARAM_LEN, data);

	data[0] = VP880_FLYBACK_MODE | VP880_ZRING_TRACK_DIS | VP880_YRING_TRACK_DIS;
	data[1] = VP880_SWY_AUTOPOWER_DIS | vbpHex;
	data[2] = VP880_SWZ_AUTOPOWER_DIS;
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_REGULATOR_PARAM_WRT, VP880_REGULATOR_PARAM_LEN, data);

	/* Set battery calibration registers */
	data[1] = 0;
	data[0] = calVbhHex << 3;
	VpMpiCmd(gDev1Id, EC_1, VP880_BAT_CALIBRATION_WRT, VP880_BAT_CALIBRATION_LEN, data);
	data[0] = calVblHex << 3;
	VpMpiCmd(gDev1Id, EC_2, VP880_BAT_CALIBRATION_WRT, VP880_BAT_CALIBRATION_LEN, data);
	data[0] = calVbpHex << 3;
	VpMpiCmd(gDev2Id, EC_1, VP880_BAT_CALIBRATION_WRT, VP880_BAT_CALIBRATION_LEN, data);

	/* Disable high-pass filter, cut off TX/RX */
	data[0] = VP880_HIGH_PASS_DIS | VP880_CUT_TXPATH | VP880_CUT_RXPATH;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_OP_COND_WRT, VP880_OP_COND_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_OP_COND_WRT, VP880_OP_COND_LEN, data);

	/* Set to linear mode, use default filters */
	data[0] = VP880_LINEAR_CODEC | VP880_DEFAULT_OP_FUNC_MODE;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_OP_FUNC_WRT, VP880_OP_FUNC_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_OP_FUNC_WRT, VP880_OP_FUNC_LEN, data);

	/* Set default DISN (00) */
	data[0] = VP880_DEFAULT_DISN_GAIN;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_DISN_WRT, VP880_DISN_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_DISN_WRT, VP880_DISN_LEN, data);

	mvOsDelay(50);

    /*** Bring up HBAT ***/
	/* Set HBAT switcher to low power. */
	data[0] = 0;
	if (vbhTarget != 0)
		data[0] |= VP880_SWY_LP;

	VpMpiCmd(gDev1Id, EC_BOTH, VP880_REGULATOR_CTRL_WRT, VP880_REGULATOR_CTRL_LEN, data);

	mvOsDelay(10);

	/* Set the line state:
	 *  Dev 1 ch 1: disconnect (switcher Y)
	 * Disconnect is needed so that the voltages don't track to VOC */
	data[0] = VP880_SS_DISCONNECT | VP880_SS_ACTIVATE_MASK;
	VpMpiCmd(gDev1Id, EC_1, VP880_SYS_STATE_WRT, VP880_SYS_STATE_LEN, data);

	mvOsDelay(100);

    /*** Bring up LBAT ***/
	/* Do a read/modify/write of ICR2 to override the LBAT switcher (Z) to off
	 * so that we can set the line state to ringing before turning it on */
	VpMpiCmd(gDev1Id, EC_2, VP880_ICR2_RD, VP880_ICR2_LEN, data);
	data[2] |= VP880_ICR2_SWY_CTRL_EN;
	data[3] &= ~VP880_ICR2_SWY_CTRL_EN;
	VpMpiCmd(gDev1Id, EC_2, VP880_ICR2_WRT, VP880_ICR2_LEN, data);

	/* Set the switcher control to low power.  It won't actually turn on yet
	 * because of the ICR2 command above, but setting the register will allow
	 * the line state to be set to ringing. */
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_REGULATOR_CTRL_RD, VP880_REGULATOR_CTRL_LEN, data);
	data[0] = 0;
	if (vblTarget != 0)
		data[0] |= VP880_SWZ_LP;

	VpMpiCmd(gDev1Id, EC_BOTH, VP880_REGULATOR_CTRL_WRT, VP880_REGULATOR_CTRL_LEN, data);

	mvOsDelay(10);

	/* Set the line state:
	 *  Dev 1 ch 2: ringing (switcher Z) */
	data[0] = VP880_SS_BALANCED_RINGING;
	VpMpiCmd(gDev1Id, EC_2, VP880_SYS_STATE_WRT, VP880_SYS_STATE_LEN, data);

	mvOsDelay(10);

	/* Turn the switcher on */
	VpMpiCmd(gDev1Id, EC_2, VP880_ICR2_RD, VP880_ICR2_LEN, data);
	data[2] |= VP880_ICR2_SWY_CTRL_EN;
	data[3] |= VP880_ICR2_SWY_CTRL_EN;
	VpMpiCmd(gDev1Id, EC_2, VP880_ICR2_WRT, VP880_ICR2_LEN, data);

	mvOsDelay(100);

    /*** Bring up PBAT ***/
	/* Set PBAT switcher to low power. */
	data[0] = 0;
	if (vbpTarget != 0)
		data[0] |= VP880_SWY_LP;

	VpMpiCmd(gDev2Id, EC_BOTH, VP880_REGULATOR_CTRL_WRT, VP880_REGULATOR_CTRL_LEN, data);

	mvOsDelay(10);

	/* Set the line states:
	 *  Dev 2 ch 1: disconnect (switcher Y)
	 * Disconnect is needed so that the voltages don't track to VOC */
	data[0] = VP880_SS_DISCONNECT | VP880_SS_ACTIVATE_MASK;
	VpMpiCmd(gDev2Id, EC_1, VP880_SYS_STATE_WRT, VP880_SYS_STATE_LEN, data);

	mvOsDelay(100);

	/* Set switchers to high power for non-zero batteries, one at a time */
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_REGULATOR_CTRL_RD, VP880_REGULATOR_CTRL_LEN, data);

	if (vbhTarget != 0) {
		data[0] &= ~VP880_SWY_MODE_MASK;
		data[0] |= VP880_SWY_HP;
		VpMpiCmd(gDev1Id, EC_BOTH, VP880_REGULATOR_CTRL_WRT, VP880_REGULATOR_CTRL_LEN, data);
	}

	mvOsDelay(20);

	if (vblTarget != 0) {
		data[0] &= ~VP880_SWZ_MODE_MASK;
		data[0] |= VP880_SWZ_HP;
		VpMpiCmd(gDev1Id, EC_BOTH, VP880_REGULATOR_CTRL_WRT, VP880_REGULATOR_CTRL_LEN, data);
	}

	mvOsDelay(20);

	data[0] = 0;
	if (vbpTarget != 0) {
		data[0] |= VP880_SWY_HP;
		VpMpiCmd(gDev2Id, EC_BOTH, VP880_REGULATOR_CTRL_WRT, VP880_REGULATOR_CTRL_LEN, data);
	}

	/* Set IOs to outputs to control the module LEDs */
	data[0] = VP880_IODIR_IO1_OUTPUT | VP880_IODIR_IO2_OUTPUT;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_IODIR_REG_WRT, VP880_IODIR_REG_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_IODIR_REG_WRT, VP880_IODIR_REG_LEN, data);

	/* Turn on the LEDs for non-zero batteries. */
	data[0] = 0;
	if (vbhTarget != 0) {
		/* gDev1Id IO1 controls the VBH (v1) LED */
		data[0] |= VP880_IODATA_IO1;
	}
	if (vblTarget != 0) {
		/* gDev1Id IO2 controls the VBL (v2) LED */
		data[0] |= VP880_IODATA_IO2;
	}
	VpMpiCmd(gDev1Id, EC_1, VP880_IODATA_REG_WRT, VP880_IODATA_REG_LEN, data);

	data[0] = 0;
	if (vbpTarget != 0) {
		/* gDev2Id IO1 controls the VBP (v3) LED */
		data[0] |= VP880_IODATA_IO1;
	}
	VpMpiCmd(gDev2Id, EC_1, VP880_IODATA_REG_WRT, VP880_IODATA_REG_LEN, data);

	mvOsPrintf("Power supply initialized successfully\n");

	return 0;
}

int BattOff(void)
{
	uint8 data[20];
	int i;

	/* 16 no-ops to clear MPI buffer */
	for (i = 0; i < 16; i++)
		data[i] = VP880_NO_OP;

	VpMpiCmd(gDev1Id, EC_BOTH, VP880_NO_OP, 16, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_NO_OP, 16, data);

	/* Read RCN/PCN and check that the devices are what we expect */
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_DEVTYPE_CMD, VP880_DEVTYPE_LEN, data);
	if (data[0] != VP880_REV_JE || data[1] != VP880_DEV_PCN_88506) {
		mvOsPrintf("Invalid device 1, RCN/PCN 0x%02X%02X", data[0], data[1]);
		return -1;
	}
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_DEVTYPE_CMD, VP880_DEVTYPE_LEN, data);
	if (data[0] != VP880_REV_JE || data[1] != VP880_DEV_PCN_88506) {
		mvOsPrintf("Invalid device 2, RCN/PCN 0x%02X%02X", data[0], data[1]);
		return -1;
	}

	/* Run MPI checks on both devices to make sure that it's safe to start
	 * programming the devices.  If the signal integrity is bad, it could be
	 * dangerous to continue. */
	if (MpiTest(gDev1Id) < 0 || MpiTest(gDev2Id) < 0)
		return -1;

	/* Turn switchers off */
	data[0] = VP880_SWY_OFF | VP880_SWZ_OFF;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_REGULATOR_CTRL_WRT, VP880_REGULATOR_CTRL_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_REGULATOR_CTRL_WRT, VP880_REGULATOR_CTRL_LEN, data);

	/* Doubly make sure the switchers are off by setting ontimes to 0 */
	data[0] = 0x00;
	data[1] = 0x40;
	data[2] = 0x00;
	data[3] = 0x40;
	data[4] = 0x00;
	data[5] = 0x40;
	VpMpiCmd(gDev1Id, EC_BOTH, VP880_INT_SWREG_PARAM_WRT, VP880_INT_SWREG_PARAM_LEN, data);
	VpMpiCmd(gDev2Id, EC_BOTH, VP880_INT_SWREG_PARAM_WRT, VP880_INT_SWREG_PARAM_LEN, data);

	/* Turn off the LEDs */
	data[0] = 0;
	VpMpiCmd(gDev1Id, EC_1, VP880_IODATA_REG_WRT, VP880_IODATA_REG_LEN, data);
	VpMpiCmd(gDev2Id, EC_1, VP880_IODATA_REG_WRT, VP880_IODATA_REG_LEN, data);

	return 0;
}

/*
 * This function uses the calibration data at pCalAry to calculate the switching
 * regulator and battery calibration register settings needed to produce a
 * voltage as close as possible to the given target (in |cV|).  The register
 * values are returned in the data pointed to by pSwReg and pCalReg.
 */
static void CalculateRegisters(uint16 target, uint16 *pCalAry, uint8 *pSwReg, uint8 *pCalReg)
{
	int16 minError;
	uint8 minIndex;
	uint8 index;
	int16 error;
	int8 cal;

	DBG("Target = %d\n", target);

	if (target == 0) {
		*pSwReg = 0;
		*pCalReg = 0;
		return;
	}

	minIndex = 0;
	minError = 32768;

	/* The calibration data contains measured voltage values at each 5V
	 * switching regulator step.  Find the 5V step measured value that is closest
	 * to the target voltage. */
	for (index = 0; index < 30; index++) {
		error = target - pCalAry[index];
		if (ABS(error) < ABS(minError)) {
			minIndex = index;
			minError = error;
		}
	}
	DBG("Closest is %d (%d), %d.  Error %d (%d)\n",
	    minIndex, (minIndex + 1) * 5, pCalAry[minIndex], minError, (int)minError / 125);

	/* The battery calibration register allows adjustment in steps of.25V.
	 * Calculate how many 1.25V steps are necessary to adjust for the difference
	 * between the target voltage and the closest measured 5V step. */
	if (minError > 0)
		cal = (minError + (125 / 2)) / 125;
	else
		cal = (minError - (125 / 2)) / 125;

	DBG("Cal offset %d\n", cal);

	DBG("Actual output should be %d\n", pCalAry[minIndex] + (cal * 125));

	/* The battery calibration register can only adjust 3 steps in either
	 * direction (3.75V).  If we need more than that (meaning an error of at
	 * least 4.375), print a warning.
	 * Since we have measurements at every 5V step, errors should never be much
	 * worse than 2.5V.  This would only reasonably happen if the target voltage
	 * is above or below the ranged of measured voltages (such as 0.5 or 155) */
	if (cal < -3) {
		mvOsPrintf("Output error may be large.  cal=%d, limited to -3", cal);
		cal = -3;
	}
	if (cal > 3) {
		mvOsPrintf("Output error may be large.  cal=%d, limited to 3", cal);
		cal = 3;
	}

	/* The hex value for the switching regulator params register is the same as
	 * the index into the calibration array. */
	*pSwReg = minIndex;

	switch (cal) {
	case -3:
		*pCalReg = 0x7;
		break;
	case -2:
		*pCalReg = 0x6;
		break;
	case -1:
		*pCalReg = 0x5;
		break;
	case 0:
		*pCalReg = 0x0;
		break;
	case 1:
		*pCalReg = 0x1;
		break;
	case 2:
		*pCalReg = 0x2;
		break;
	case 3:
		*pCalReg = 0x3;
		break;
	default:
		*pCalReg = 0;
		mvOsPrintf("This shouldn't be possible, cal=%d", cal);
		break;
	}
}

/*
 * This function performs a series of writes and reads to verify the signal
 * integrity of the MPI interface.
 */
static int MpiTest(VpDeviceIdType devId)
{
	uint8 writeData[20] = { 0 };
	uint8 readData[20] = { 0 };
	int x;
	int y;
	int i;

	for (x = 0; x <= 0xFF; x++) {
		for (i = 0; i < VP880_R_FILTER_LEN; i++)
			writeData[i] = x;

		VpMpiCmd(devId, EC_1, VP880_R_FILTER_WRT, VP880_R_FILTER_LEN, writeData);
		VpMpiCmd(devId, EC_1, VP880_R_FILTER_RD, VP880_R_FILTER_LEN, readData);
		for (i = 0; i < VP880_R_FILTER_LEN; i++) {
			if (writeData[i] != readData[i]) {
				mvOsPrintf("Failed MPI test 1-%d on devId 0x%X", x, devId);
				return -1;
			}
		}
	}

	for (x = 0; x <= 0xFF; x++) {
		y = x % 0x10;
		for (i = 0; i < VP880_R_FILTER_LEN; i++) {
			writeData[i] = y << 4;
			y = (y + 1) % 0x10;
			writeData[i] |= y;
			y = (y + 1) % 0x10;
		}
		VpMpiCmd(devId, EC_1, VP880_R_FILTER_WRT, VP880_R_FILTER_LEN, writeData);
		VpMpiCmd(devId, EC_1, VP880_R_FILTER_RD, VP880_R_FILTER_LEN, readData);
		for (i = 0; i < VP880_R_FILTER_LEN; i++) {
			if (writeData[i] != readData[i]) {
				mvOsPrintf("Failed MPI test 2-%d on devId 0x%X", x, devId);
				return -1;
			}
		}
	}

	return 0;
}

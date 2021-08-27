/** \file vp_hal.c
 * vp_hal.c
 *
 * This file contains the platform dependent code for the Hardware Abstraction
 * Layer (HAL). This is example code only to be used by the customer to help
 * clarify HAL requirements.
 *
 * Copyright (c) 2008, Zarlink Semiconductor, Inc.
 */

#include "mpi_hal.h"
#include "hbi_hal.h"
#include "voiceband/mvSysTdmSpi.h"

#undef DEBUG
#ifdef DEBUG
#define DBG(fmt, arg...)	mvOsPrintf(KERN_INFO fmt, ##arg)
#else
#define DBG(fmt, arg...)
#endif

/* MPI */
#define READ_COMMAND			1
#define CSLAC_EC_REG_RD			0x4B   /* Same for all CSLAC devices */
#define CSLAC_EC_REG_WRT		0x4A   /* Same for all CSLAC devices */
#define MPI_MAX_CMD_LEN			255

/* HBI */
#define HBI_CMD_BYTES			2
#define HBI_DATA_BYTES(numWords)	((numWords + 1) * 2)
#define HBI_MAX_DATA_LEN		256

uint16 readBuff[HBI_MAX_DATA_LEN];
uint16 writeBuff[HBI_MAX_DATA_LEN];

/**
 * VpHalHbiInit(): Configures the HBI bus and glue logic (if any)
 *
 * This function performs any tasks necessary to prepare the system for
 * communicating through the HBI, including writing the HBI configuration
 * register.  The HBI read and write functions should work after HbiHalInit()
 * is successfully executed. HbiHalInit() should be well-behaved even if called
 * more than once between system resets. HbiHalInit() is called from
 * VpBootLoad() since VpBootLoad() is normally the first VoicePath function
 * that the host application will call.
 *
 * This function is called by VpBootLoad() before sending the VCP firmware
 * image through the HBI.
 *
 * Params:
 *  uint8 deviceId: Device Id (chip select ID)
 *
 * Returns:
 *  This function returns FALSE if some error occurred during HBI initialization
 *  or TRUE otherwise.
 */
uint8 VpHalHbiInit(
    VpDeviceIdType deviceId)
{
	return TRUE;
} /* VpHalHbiInit() */

/**
 * VpHalHbiCmd(): Sends a command word over the HBI, with no data words.
 *
 *  Accepts a uint16 HBI command which is little-endian or big-endian,
 * depending on the host architecture.  Command words on the HBI bus are always
 * big-endian. This function is responsible for byte-swapping if required.
 *
 * Params:
 * uint8 deviceId: Device Id (chip select ID)
 * uint16 cmd: the command word to send
 *
 * Returns:
 *   TRUE on success, FALSE on failure
 */
uint8 VpHalHbiCmd(
    VpDeviceIdType deviceId,
    uint16 cmd)
{
	uint16 cmdSwapped = MV_16BIT_BE(cmd);

	DBG("%s: WRITE(cmd-0x%x)\n", __func__, cmd);
	mvSysTdmSpiWrite(deviceId, (uint8p)&cmdSwapped, HBI_CMD_BYTES, NULL, 0);

	return TRUE;
} /* VpHalHbiCmd() */

/**
 * VpHalHbiWrite(): Sends a command word and up to 256 data words over the HBI.
 *
 *  Accepts a uint16 HBI command which is little-endian or big-endian, depending
 * on the host architecture.  Command words on the HBI bus are always big-
 * endian.  This function is responsible for byte-swapping the command word, if
 * required.
 *
 *  Accepts an array of uint16 data words.  No byte-swapping is necessary on
 * data words in this function.  Instead, the HBI bus can be configured in
 * VpHalHbiInit() to match the endianness of the host platform.
 *
 * Params:
 *   uint8 deviceId: Device Id (chip select ID)
 *   uint16 cmd: the command word to send
 *   uint8 numwords: the number of data words to send, minus 1
 *   uint16p data: the data itself; use data = (uint16p)0 to send
 *      zeroes for all data words
 *
 * Returns:
 *   TRUE on success, FALSE on failure
 */
uint8 VpHalHbiWrite(
    VpDeviceIdType deviceId,
    uint16 cmd,
    uint8 numwords,
    uint16p data)
{
	uint8 i;
	uint16 cmdSwapped = MV_16BIT_BE(cmd);
	uint16p pWriteBuff = &writeBuff[0];

	if ((numwords + 1) > HBI_MAX_DATA_LEN) {
		mvOsPrintf("%s: Error, HBI data length too big(%u)\n", __func__, (numwords + 1));
		return FALSE;
	}

	for (i = 0; i < (numwords + 1); i++)
		pWriteBuff[i] = MV_16BIT_BE(data[i]);

	DBG("%s: WRITE(cmd-0x%x), (size-%d bytes)\n", __func__, cmd, HBI_DATA_BYTES(numwords));
	mvSysTdmSpiWrite(deviceId, (uint8p)&cmdSwapped, HBI_CMD_BYTES, (uint8p)pWriteBuff, HBI_DATA_BYTES(numwords));

	return TRUE;
} /* VpHalHbiWrite() */

/**
 * VpHalHbiRead(): Sends a command, and receives up to 256 data words over the
 * HBI.
 *
 *  Accepts a uint16 HBI command which is little-endian or big-endian, depending
 * on the host architecture.  Command words on the HBI bus are always big-
 * endian.  This function is responsible for byte-swapping the command word, if
 * required.
 *
 * Retrieves an array of uint16 data words.  No byte-swapping is necessary on
 * data words in this function.  Instead, the HBI bus can be configured in
 * VpHalHbiInit() to match the endianness of the host platform.
 *
 * Params:
 *   uint8 deviceId: Device Id (chip select ID)
 *   uint8 numwords: the number of words to receive, minus 1
 *   uint16p data: where to put them
 *
 * Returns:
 *   TRUE on success, FALSE on failure
 */
uint8 VpHalHbiRead(
    VpDeviceIdType deviceId,
    uint16 cmd,
    uint8 numwords,
    uint16p data)
{
	uint8 i;
	uint16 cmdSwapped = MV_16BIT_BE(cmd);
	uint16p pReadBuff = &readBuff[0];

	if ((numwords + 1) > HBI_MAX_DATA_LEN) {
		mvOsPrintf("%s: Error, HBI data length too big(%u)\n", __func__, (numwords + 1));
		return FALSE;
	}

	DBG("%s: READ(cmd-0x%x), (size-%d bytes)\n", __func__, cmd, HBI_DATA_BYTES(numwords));
	mvSysTdmSpiRead(deviceId, (uint8p)&cmdSwapped, HBI_CMD_BYTES, (uint8p)pReadBuff, HBI_DATA_BYTES(numwords));

	for (i = 0; i < (numwords + 1); i++)
		data[i] = MV_16BIT_BE(pReadBuff[i]);

	return TRUE;

} /* VpHalHbiRead() */

/*****************************************************************************
 * HAL functions for CSLAC devices. Not necessary for VCP
 ****************************************************************************/
/**
 * VpMpiCmd()
 *  This function executes a Device MPI command through the MPI port. It
 * executes both read and write commands. The read or write operation is
 * determined by the "cmd" argument (odd = read, even = write). The caller must
 * ensure that the data array is large enough to hold the data being collected.
 * Because this command used hardware resources, this procedure is not
 * re-entrant.
 *
 * Note: For API-II to support multi-threading, this function has to write to
 * the EC register of the device to set the line being controlled, in addition
 * to the command being passed. The EC register write/read command is the same
 * for every CSLAC device and added to this function. The only exception is
 * if the calling function is accessing the EC register (read), in which case
 * the EC write cannot occur.
 *
 * This example assumes the implementation of two byte level commands:
 *
 *    MpiReadByte(VpDeviceIdType deviceId, uint8 *data);
 *    MpiWriteByte(VpDeviceIdType deviceId, uint8 data);
 *
 * Preconditions:
 *  The device must be initialized.
 *
 * Postconditions:
 *   The data pointed to by dataPtr, using the command "cmd", with length
 * "cmdLen" has been sent to the MPI bus via the chip select associated with
 * deviceId.
 */
void
VpMpiCmd(
    VpDeviceIdType deviceId,    /* Chip select */
    uint8 ecVal,        	/* Value to write to the EC register */
    uint8 cmd,          	/* Command number */
    uint8 cmdLen,       	/* Number of bytes used by command (cmd) */
    uint8 *dataPtr)     	/* Pointer to the data location */
{

    uint8 cmdBuff[4];
    uint8 cmdSize = 0;
    uint8 isRead = (cmd & READ_COMMAND);

     if (cmdLen > MPI_MAX_CMD_LEN)
	mvOsPrintf("Error, MPI data length too big(%u)\n", cmdLen);

    /* If a EC read is being preformed don't set the EC register */
    if (CSLAC_EC_REG_RD != cmd) {
	/* Write the EC register value passed to the device */
	cmdBuff[cmdSize++] = CSLAC_EC_REG_WRT;
	cmdBuff[cmdSize++] = ecVal;
    }

    /* Write the command byte to MPI. */
    cmdBuff[cmdSize++] = cmd;

    if (isRead) {
	DBG("%s: READ - cmdSize=%d, dataSize=%d\n", __func__, cmdSize, cmdLen);
	mvSysTdmSpiRead(deviceId, cmdBuff, cmdSize, dataPtr, cmdLen);
    } else {
	DBG("%s: WRITE - cmdSize=%d, dataSize=%d\n", __func__, cmdSize, cmdLen);
	mvSysTdmSpiWrite(deviceId, cmdBuff, cmdSize, dataPtr, cmdLen);
    }

    return;
} /* End VpMpiCmd */

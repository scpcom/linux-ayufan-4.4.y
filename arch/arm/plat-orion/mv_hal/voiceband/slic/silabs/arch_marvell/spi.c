/*
** $Id: spi.c 109 2008-10-22 19:45:09Z lajordan@SILABS.COM $
**
** spi.h
** SPI driver implementation file
**
** Author(s):
** laj
**
** Distributed by:
** Silicon Laboratories, Inc
**
** File Description:
** This is the implementation file for the SPI driver used
** in the ProSLIC demonstration code.
**
** Dependancies:
**
**
*/
#include "spi.h"
#include "voiceband/mvSysTdmSpi.h"
#include "spi/mvSpi.h"
#include "timer.h"

/*
** Function: SPI_Init
**
** Description:
** Initializes the SPI interface
**
** Input Parameters:
** none
**
** Return:
** none
*/
int SPI_Init (ctrl_S * hSpiGci)
{
	uInt8 portId = hSpiGci->portID;
	MV_SPI_IF_PARAMS ifParams;
	MV_SPI_TIMING_PARAMS tmngParams;

	ifParams.clockPolLow = TRUE;
	ifParams.clockPhase = SPI_CLK_BEGIN_CYC;
	ifParams.txMsbFirst = FALSE;
	ifParams.rxMsbFirst = FALSE;

	tmngParams.tcsh = 0x3F;
	tmngParams.tmisoSample = 0;
	tmngParams.tcsSetup = 0x4;
	tmngParams.tcsHold = 0x4;

	mvSpiIfConfigSet(portId, &ifParams);
	mvSpiTimingParamsSet(portId, &tmngParams);

	return 0;
}

#define CNUM_TO_CID_QUAD(channelNumber)   (((channelNumber<<4)&0x10)|((channelNumber<<2)&0x8)|((channelNumber>>2)&0x2)|((channelNumber>>4)&0x1)|(channelNumber&0x4))

static uInt8 ReadRegister (ctrl_S * hSpiGci, uInt8 channel, uInt8 regAddr)
{
	uInt8 cmdBuff[2];
	uInt8 dataBuff[5];
	uInt8 portId = hSpiGci->portID;
#ifdef PROFILE
	profile.regAccess++;
#endif

	if (channel == 0xFF)
		cmdBuff[0] = 0x80 | 0x60;
	else
		cmdBuff[0] = CNUM_TO_CID_QUAD(channel) | 0x60;

	cmdBuff[1] = regAddr;

	mvSysTdmSpiRead(portId, cmdBuff, 2, dataBuff, 1);

	return dataBuff[0];
}

static void WriteRegister (ctrl_S * hSpiGci, uInt8 channel, uInt8 regAddr, uInt8 data)
{
	uInt8 cmdBuff[2];
	uInt8 portId = hSpiGci->portID;

#ifdef PROFILE
	profile.regAccess++;
#endif
	if (channel == 0xFF)
		cmdBuff[0] = 0x80 | 0x20;
	else
		cmdBuff[0] = CNUM_TO_CID_QUAD(channel) | 0x20;

	cmdBuff[0] &= 0xBF;
	cmdBuff[1] = regAddr;

	mvSysTdmSpiWrite(portId, cmdBuff,2 , &data, 1);

	return;
}

#define DELAY_TEST 0
/*
** ReadRAM()
**
** Read from ProSLIC RAM space
**
*/
static uInt32 ReadRAM(ctrl_S * hSpiGci, uInt8 channel, uInt16 address)
{
	uInt8 dataByte;
	uInt8 addrByte;
	uInt32 dataWord = 0L;
	uInt32 userTimeout = 100000; //USER_TIMEOUT_VAL; /* user defined timeout counter */

	/* Wait for RAM to finish */
	while ((ReadRegister(hSpiGci, channel, RAMSTAT) & 0x01) && (userTimeout > 0)) {
		userTimeout--;
	}
	/*
	while ((ReadRegister(hSpiGci, channel, RAMSTAT) & 0x01)) {
		userTimeout--;
	}*/
	/* RAM_ADR_HI[7:5] = ramAddr[10:8] */
	addrByte = (uInt8)((address >> 3) & 0x00E0);
	WriteRegister(hSpiGci, channel, RAM_ADDR_HI, addrByte);
	time_DelayWrapper (NULL, DELAY_TEST);
	/* RAM_ADR_LO[7:0] = ramAddr[7:0] */
	addrByte = (uInt8)((address & 0x00FF));
	WriteRegister(hSpiGci, channel, RAM_ADDR_LO, addrByte);
	time_DelayWrapper (NULL, DELAY_TEST);
	/* Wait for RAM to finish */
	while ((ReadRegister(hSpiGci, channel, RAMSTAT) & 0x01) && (userTimeout > 0)) {
		userTimeout--;
	}
	/*
	while ((ReadRegister(hSpiGci, channel, RAMSTAT) & 0x01)) {
	userTimeout--;
}*/
	time_DelayWrapper (NULL, DELAY_TEST);
	/* ramData[4:0] = RAM_DATA_B0[7:3] */
	dataByte = ReadRegister(hSpiGci, channel, RAM_DATA_B0);
	dataWord |= ((dataByte >> 3) & 0x0000001FL);
	/* ramData[12:5] = RAM_DATA_B1[7:0] */
	time_DelayWrapper (NULL, DELAY_TEST);
	dataByte = ReadRegister(hSpiGci, channel, RAM_DATA_B1);
	dataWord |= ((dataByte << 5) & 0x000001FE0L);
	/* ramData[20:13] = RAM_DATA_B2[7:0] */
	time_DelayWrapper (NULL, DELAY_TEST);
	dataByte = ReadRegister(hSpiGci, channel, RAM_DATA_B2);
	dataWord |= ((dataByte << 13) & 0x0001FE000L);
	/* ramData[28:21] = RAM_DATA_B3[7:0] */
	time_DelayWrapper (NULL, DELAY_TEST);
	dataByte = ReadRegister(hSpiGci, channel, RAM_DATA_B3);
	dataWord |= ((dataByte << 21) & 0x1FE00000L);

	return dataWord;
}

/*
** WriteRAM()
**
** Write to ProSLIC RAM space
**
*/
void WriteRAM(ctrl_S * hSpiGci, uInt8 channel, uInt16 address, uInt32 data)
{
	uInt8 dataByte;
	uInt8 addrByte;
	uInt32 userTimeout = 100000; //USER_TIMEOUT_VAL;

	/* User defined timeout counter */
	/* Wait for RAM to finish */
	while ((ReadRegister(hSpiGci, channel, RAMSTAT) & 0x01) && (userTimeout > 0)) {
		userTimeout--;
	}
	/*
	while ((ReadRegister(hSpiGci, channel, RAMSTAT) & 0x01)) {
	userTimeout--;
}*/
	time_DelayWrapper (NULL, DELAY_TEST);

	/* RAM_ADR_HI[7:5] = ramAddr[10:8] */
	addrByte = (uInt8)((address >> 3) & 0x00E0);
	WriteRegister(hSpiGci, channel, RAM_ADDR_HI, addrByte);
	/* RAM_DATA_B0[7:3] = ramData[4:0] */
	time_DelayWrapper (NULL, DELAY_TEST);
	dataByte = (uInt8)((data << 3) & 0x000000F1L);
	WriteRegister(hSpiGci, channel, RAM_DATA_B0, dataByte);
	/* RAM_DATA_B1[7:0] = ramData[12:5] */
	time_DelayWrapper (NULL, DELAY_TEST);
	dataByte = (uInt8)((data >> 5) & 0x000000FFL);
	WriteRegister(hSpiGci, channel, RAM_DATA_B1, dataByte);
	/* RAM_DATA_B2[7:0] = ramData[20:13] */
	time_DelayWrapper (NULL, DELAY_TEST);
	dataByte = (uInt8)((data >> 13) & 0x000000FFL);
	WriteRegister(hSpiGci, channel, RAM_DATA_B2, dataByte);
	/* RAM_DATA_B3[7:0] = ramData[28:21] */
	time_DelayWrapper (NULL, DELAY_TEST);
	dataByte = (uInt8)((data >> 21) & 0x000000FFL);
	WriteRegister(hSpiGci, channel, RAM_DATA_B3, dataByte);
	/* RAM_ADR_LO[7:0] = ramAddr[7:0] */
	time_DelayWrapper (NULL, DELAY_TEST);
	addrByte = (uInt8)((address & 0x00FF));
	WriteRegister(hSpiGci, channel, RAM_ADDR_LO, addrByte);
}

/*
** Function: spiGci_ResetWrapper
**
** Description:
** Sets the reset pin of the ProSLIC
*/
int ctrl_ResetWrapper (void * hSpiGci, int status)
{
	/*if (status)
		RSTNLow();
	else
		RSTNHigh();*/
	return 0;
}

/*
** SPI/GCI register read
**
** Description:
** Reads a single ProSLIC register
**
** Input Parameters:
** channel: ProSLIC channel to read from
** num: number of reads to perform
** regAddr: Address of register to read
** addr_inc: whether to increment address after each read
** data: data to read from register
**
** Return:
** none
*/
uInt8 ctrl_ReadRegisterWrapper (void * hSpiGci, uInt8 channel,uInt8 regAddr)
{

	return ReadRegister(hSpiGci, channel, regAddr);
}

/*
** Function: spiGci_WriteRegisterWrapper
**
** Description:
** Writes a single ProSLIC register
**
** Input Parameters:
** channel: ProSLIC channel to write to
** address: Address of register to write
** data: data to write to register
**
** Return:
** none
*/
int ctrl_WriteRegisterWrapper (void * hSpiGci, uInt8 channel,  uInt8 regAddr, uInt8 data)
{
	WriteRegister(hSpiGci, channel, regAddr, data);
	return 0;
}

/*
** Function: SPI_ReadRAMWrapper
**
** Description:
** Reads a single ProSLIC RAM location
**
** Input Parameters:
** channel: ProSLIC channel to read from
** address: Address of RAM location to read
** pData: data to read from RAM location
**
** Return:
** none
*/
ramData ctrl_ReadRAMWrapper (void * hSpiGci, uInt8 channel, uInt16 ramAddr)
{
	return ReadRAM(hSpiGci, channel, ramAddr);;
}

/*
** Function: SPI_WriteRAMWrapper
**
** Description:
** Writes a single ProSLIC RAM location
**
** Input Parameters:
** channel: ProSLIC channel to write to
** address: Address of RAM location to write
** data: data to write to RAM location
**
** Return:
** none
*/
int ctrl_WriteRAMWrapper (void * hSpiGci, uInt8 channel, uInt16 ramAddr, ramData data)
{
	WriteRAM(hSpiGci, channel, ramAddr, data);
	return 0;
}

/*
** $Log: dummy_spi.c,v $
** Revision 1.3  2008/07/24 21:06:16  lajordan
** no message
**
** Revision 1.2  2007/10/22 21:38:10  lajordan
** fixed some warnings
**
** Revision 1.1  2007/10/22 20:52:08  lajordan
** no message
**
*/

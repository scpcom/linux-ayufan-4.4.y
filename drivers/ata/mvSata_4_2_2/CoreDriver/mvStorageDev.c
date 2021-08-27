#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "mvOsS.h"
#include "mvSata.h"
#include "mvStorageDev.h"
#include "mvRegs.h"

MV_BOOLEAN waitWhileStorageDevIsBusy(MV_SATA_ADAPTER* pAdapter,
                                     MV_BUS_ADDR_T ioBaseAddr,
                                     MV_U32 eDmaRegsOffset, MV_U32 loops,
                                     MV_U32 delayParam);
MV_BOOLEAN waitForDRQToClear(MV_SATA_ADAPTER* pAdapter,
                             MV_BUS_ADDR_T ioBaseAddr,
                             MV_U32 eDmaRegsOffset, MV_U32 loops,
                             MV_U32 delayParam);

void enableStorageDevInterrupt (MV_SATA_CHANNEL *pSataChannel);
void disableStorageDevInterrupt(MV_SATA_CHANNEL *pSataChannel);
static MV_BOOLEAN isStorageDevReadyForPIO(MV_SATA_CHANNEL *pSataChannel);
void dumpAtaDeviceRegisters(MV_SATA_ADAPTER *pAdapter,
                            MV_U8 channelIndex, MV_BOOLEAN isEXT,
                            MV_STORAGE_DEVICE_REGISTERS *pRegisters);
MV_BOOLEAN _doSoftReset(MV_SATA_CHANNEL *pSataChannel);
extern void _setActivePMPort(MV_SATA_CHANNEL *pSataChannel, MV_U8 PMPort);
extern void disableSaDevInterrupts(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex);

MV_BOOLEAN  _PMAccessReg(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex,
                         MV_U8 PMPort, MV_U8 PMReg, MV_U32 *pValue,
                         MV_STORAGE_DEVICE_REGISTERS *registerStruct,
                         MV_BOOLEAN isRead);

MV_BOOLEAN executeNonUDMACommand(MV_SATA_ADAPTER *pAdapter,
                                 MV_U8 channelIndex,
                                 MV_U8  PMPort,
                                 MV_NON_UDMA_PROTOCOL protocolType,
                                 MV_BOOLEAN  isEXT,
                                 MV_U16_PTR bufPtr, MV_U32 count,
                                 MV_U16 features,
                                 MV_U16 sectorCount,
                                 MV_U16 lbaLow, MV_U16 lbaMid,
                                 MV_U16 lbaHigh, MV_U8 device,
                                 MV_U8 command);

MV_BOOLEAN waitWhileStorageDevIsBusy_88SX60X1(MV_SATA_ADAPTER* pAdapter,
					      MV_BUS_ADDR_T ioBaseAddr,
					      MV_U32 eDmaRegsOffset, MV_U8 channelIndex,
					      MV_U32 loops,
					      MV_U32 delayParam);

MV_BOOLEAN waitForDRQ(MV_SATA_ADAPTER* pAdapter,
                      MV_BUS_ADDR_T ioBaseAddr,
                      MV_U32 eDmaRegsOffset, MV_U32 loops,
                      MV_U32 delayParam);

void _startSoftResetDevice(MV_SATA_CHANNEL *pSataChannel);
MV_BOOLEAN _isDeviceBsyBitOff(MV_SATA_CHANNEL *pSataChannel);
 
MV_BOOLEAN waitWhileStorageDevIsBusy(MV_SATA_ADAPTER* pAdapter,
                                     MV_BUS_ADDR_T ioBaseAddr,
                                     MV_U32 eDmaRegsOffset, MV_U32 loops,
                                     MV_U32 delayParam)
{
    MV_U8   ATAstatus = 0;
    MV_U32  i;

    for (i = 0;i < loops; i++)
    {
        ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                     MV_ATA_DEVICE_STATUS_REG_OFFSET);
        if ((ATAstatus & MV_ATA_BUSY_STATUS) == 0)
        {

            if ((ATAstatus & MV_ATA_ERROR_STATUS) == 0)
            {
                mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, "waitWhileStorageDevIsBusy: %d loops *"
                         "%d usecs\n", i, delayParam);
                return MV_TRUE;
            }
            else
            {
                mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, "waitWhileStorageDevIsBusy<FAILED>: Device ERROR"
                         " Status: 0x%02x\n", ATAstatus);
                return MV_FALSE;
            }
        }
        mvMicroSecondsDelay(pAdapter, delayParam);
    }
    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "waitWhileStorageDevIsBusy<FAILED>: Time out - Device ERROR"
             " Status: 0x%02x. loops %d, delay %d\n", ATAstatus, loops, delayParam);

    return MV_FALSE;
}

MV_BOOLEAN waitWhileStorageDevIsBusy_88SX60X1(MV_SATA_ADAPTER* pAdapter,
                                                                    MV_BUS_ADDR_T ioBaseAddr,
                                                                    MV_U32 eDmaRegsOffset, MV_U8 channelIndex,
                                                                    MV_U32 loops,
                                                                    MV_U32 delayParam)
{
    MV_U8   ATAstatus = 0;
    MV_U32  i,intReg;
    MV_U8   sataUnit = channelIndex >> 2, portNum = (channelIndex & 0x3);

    for (i = 0;i < loops; i++)
    {
        intReg = MV_REG_READ_DWORD (ioBaseAddr, MV_SATAHC_REGS_BASE_OFFSET(sataUnit) +
                                    MV_SATAHC_INTERRUPT_CAUSE_REG_OFFSET);

        if (intReg & (1 << (8 + portNum)))
        {
            ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                         MV_ATA_DEVICE_STATUS_REG_OFFSET);
            MV_REG_WRITE_DWORD (ioBaseAddr, MV_SATAHC_REGS_BASE_OFFSET(sataUnit) +
                                MV_SATAHC_INTERRUPT_CAUSE_REG_OFFSET,
                                ~(1 << (8 + portNum)));
            if ((ATAstatus & MV_ATA_ERROR_STATUS) == 0)
            {
                mvLogMsg(MV_CORE_DRIVER_LOG_ID,MV_DEBUG, "waitWhileStorageDevIsBusy: %d loops *"
                         "%d usecs\n", i, delayParam);
                return MV_TRUE;
            }
            else
            {
                mvLogMsg(MV_CORE_DRIVER_LOG_ID,MV_DEBUG_ERROR, "waitWhileStorageDevIsBusy<FAILED>: Device ERROR"
                         " Status: 0x%02x\n", ATAstatus);
                return MV_FALSE;
            }
        }
        mvMicroSecondsDelay(pAdapter, delayParam);
    }
    ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                 MV_ATA_DEVICE_STATUS_REG_OFFSET);
 
    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, "waitWhileStorageDevIsBusy<FAILED>: Time out - Device ERROR"
             " Status: 0x%02x. loops %d, delay %d\n", ATAstatus, loops, delayParam);
    return MV_FALSE;
}

MV_BOOLEAN waitForDRQ(MV_SATA_ADAPTER* pAdapter,
                      MV_BUS_ADDR_T ioBaseAddr,
                      MV_U32 eDmaRegsOffset, MV_U32 loops,
                      MV_U32 delayParam)
{
    MV_U8   ATAstatus = 0;
    MV_U32  i;

    for (i = 0;i < loops; i++)
    {
        ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                     MV_ATA_DEVICE_STATUS_REG_OFFSET);
        if ((ATAstatus & MV_ATA_BUSY_STATUS) == 0)
        {
            if (ATAstatus & MV_ATA_DATA_REQUEST_STATUS)
            {
                mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, "waitWhileStorageDevIsBusy: %d loops *"
                         "%d usecs\n", i, delayParam);
                return MV_TRUE;
            }
        }
        mvMicroSecondsDelay(pAdapter, delayParam);
    }
    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "waitWhileStorageDevIsBusy<FAILED>: Time out - Device ERROR"
             " Status: 0x%02x. loops %d, delay %d\n", ATAstatus, loops, delayParam);

    return MV_FALSE;
}
 
void enableStorageDevInterrupt(MV_SATA_CHANNEL *pSataChannel)
{

    MV_REG_WRITE_BYTE(pSataChannel->mvSataAdapter->adapterIoBaseAddress,
                      pSataChannel->eDmaRegsOffset +
                      MV_ATA_DEVICE_CONTROL_REG_OFFSET,0);
    MV_REG_READ_BYTE(pSataChannel->mvSataAdapter->adapterIoBaseAddress,
                     pSataChannel->eDmaRegsOffset +
                     MV_ATA_DEVICE_CONTROL_REG_OFFSET);
}

void disableStorageDevInterrupt(MV_SATA_CHANNEL *pSataChannel)
{

    MV_REG_WRITE_BYTE(pSataChannel->mvSataAdapter->adapterIoBaseAddress,
                      pSataChannel->eDmaRegsOffset +
                      MV_ATA_DEVICE_CONTROL_REG_OFFSET, MV_BIT1);
    MV_REG_READ_BYTE(pSataChannel->mvSataAdapter->adapterIoBaseAddress,
                     pSataChannel->eDmaRegsOffset +
                     MV_ATA_DEVICE_STATUS_REG_OFFSET);
}

static MV_BOOLEAN isStorageDevReadyForPIO(MV_SATA_CHANNEL *pSataChannel)
{
    MV_BUS_ADDR_T ioBaseAddr =pSataChannel->mvSataAdapter->adapterIoBaseAddress;
    MV_U32  eDmaRegsOffset = pSataChannel->eDmaRegsOffset;
    MV_U8   ATAcontrolRegValue;

    if (pSataChannel->queueCommandsEnabled == MV_TRUE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d: PIO command failed:"
                 "EDMA is active\n", pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber);
        return MV_FALSE;
    }
     
    ATAcontrolRegValue = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                          MV_ATA_DEVICE_CONTROL_REG_OFFSET);
    if ((ATAcontrolRegValue & MV_ATA_BUSY_STATUS)!= 0)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  control regiser is "
                 "0x%02x\n",pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber,ATAcontrolRegValue);
        return MV_FALSE;
    }
    if ( (pSataChannel->deviceType != MV_SATA_DEVICE_TYPE_ATAPI_DEVICE) && 
        ((ATAcontrolRegValue & MV_ATA_READY_STATUS) != MV_ATA_READY_STATUS))
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  storage drive is not"
                 " ready, ATA STATUS=0x%02x\n",
                 pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber, ATAcontrolRegValue);
        return MV_FALSE;
    }
     
    return MV_TRUE;
}

MV_BOOLEAN mvStorageDevATAIdleImmediate(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex)
{
    MV_SATA_CHANNEL *pSataChannel;
    MV_BUS_ADDR_T ioBaseAddr;
    MV_U32  eDmaRegsOffset;

    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvStorageDevATAIdentif"
                 "yDevice failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    if (pSataChannel == NULL)
    {      
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  channel data "
                 "structure is not allocated\n", pAdapter->adapterId,
                 channelIndex);
        return MV_FALSE;
    }

    ioBaseAddr =pSataChannel->mvSataAdapter->adapterIoBaseAddress;

    mvOsSemTake(&pSataChannel->semaphore);
    eDmaRegsOffset = pSataChannel->eDmaRegsOffset;

    if (pSataChannel->queueCommandsEnabled == MV_TRUE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  mvStorageDevATAIdle"
                 "Immediate command failed: EDMA is active\n",
                 pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, "Issue IDLE IMMEDIATE COMMAND\n");
    disableStorageDevInterrupt(pSataChannel);
    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset + MV_ATA_DEVICE_COMMAND_REG_OFFSET,
                      MV_ATA_COMMAND_IDLE_IMMEDIATE);

    if (waitWhileStorageDevIsBusy(pAdapter,
                                  ioBaseAddr, eDmaRegsOffset, 10000, 100) == MV_FALSE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  Idle Immediate failed\n",
                 pSataChannel->mvSataAdapter->adapterId, pSataChannel->channelNumber);

        enableStorageDevInterrupt(pSataChannel);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }

    enableStorageDevInterrupt(pSataChannel);
    mvOsSemRelease( &pSataChannel->semaphore);
    return MV_TRUE;
}

MV_BOOLEAN mvStorageDevATAIdentifyDevice(MV_SATA_ADAPTER *pAdapter,
                                         MV_U8 channelIndex,
                                         MV_U8 PMPort,
                                         MV_U16_PTR  identifyDeviceResult
                                        )
{
    MV_BOOLEAN result;
     
    MV_SATA_CHANNEL *pSataChannel;
    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvStorageDevATAIdentif"
                 "yDevice failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    if (pSataChannel == NULL)
    {      
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  channel data "
                 "structure is not allocated\n", pAdapter->adapterId,
                 channelIndex);
        return MV_FALSE;
    }
    if (identifyDeviceResult == NULL)
    {      
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  identify data buffer"
                 " is not allocated\n", pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    result = mvStorageDevATAExecuteNonUDMACommand(pAdapter, channelIndex,
                                                  PMPort,
                                                  MV_NON_UDMA_PROTOCOL_PIO_DATA_IN,
                                                  MV_FALSE,
                                                   
                                                  identifyDeviceResult,
                                                  256,      
                                                  0,        
                                                  0,        
                                                  0,        
                                                  0,        
                                                  0,        
                                                  0,        
                                                   
                                                  MV_ATA_COMMAND_IDENTIFY);
    if (result == MV_FALSE)
    {
        return MV_FALSE;
    }
    if (identifyDeviceResult[IDEN_ATA_VERSION] & (MV_BIT7 | MV_BIT6 | MV_BIT5))
    {
         
        MV_U8 crc = 0;
        MV_U16 count;
        MV_U8_PTR pointer = (MV_U8_PTR)identifyDeviceResult;
         
        if (pointer[510] != 0xa5)
        {
            return MV_TRUE;
        }
        for (count = 0 ; count < ATA_SECTOR_SIZE ; count ++)
        {
            crc += pointer[count];
        }
        if (crc != 0)
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  IDENTIFY DEVICE "
                     "ATA Command failed due to wrong CRC checksum (%02x)\n",
                     pAdapter->adapterId, channelIndex,crc);
            return MV_FALSE;
        }

    }
    return MV_TRUE;
}

MV_BOOLEAN mvStorageDevATASoftResetDevice(MV_SATA_ADAPTER *pAdapter,
                                          MV_U8 channelIndex,
                                          MV_U8 PMPort,
                                          MV_STORAGE_DEVICE_REGISTERS *registerStruct
                                         )
{
    MV_SATA_CHANNEL *pSataChannel;
    MV_BUS_ADDR_T   ioBaseAddr;
    MV_U32          eDmaRegsOffset;
    MV_BOOLEAN      result;
    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvStorageDevATASoftRes"
                 "etDevice Failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    ioBaseAddr = pAdapter->adapterIoBaseAddress;
    if (pSataChannel == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  channel data structu"
                 "re is not allocated\n", pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }

    mvOsSemTake(&pSataChannel->semaphore);
    eDmaRegsOffset = pSataChannel->eDmaRegsOffset;

    if (pSataChannel->queueCommandsEnabled == MV_TRUE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  mvStorageDevATASoft"
                 "ResetDevice command failed: EDMA is active\n",
                 pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }
    _setActivePMPort(pSataChannel, PMPort);
    result = _doSoftReset(pSataChannel);
    if (registerStruct)
    {
        dumpAtaDeviceRegisters(pAdapter, channelIndex, MV_FALSE,
                               registerStruct);
    }
    mvOsSemRelease( &pSataChannel->semaphore);
    return result;
}

void _startSoftResetDevice(MV_SATA_CHANNEL *pSataChannel)
{
    MV_BUS_ADDR_T   ioBaseAddr =
    pSataChannel->mvSataAdapter->adapterIoBaseAddress;

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_NON_UDMA_COMMAND | MV_DEBUG, "Issue SRST COMMAND\n");

    MV_REG_WRITE_BYTE(ioBaseAddr, pSataChannel->eDmaRegsOffset +
                      MV_ATA_DEVICE_CONTROL_REG_OFFSET, MV_BIT2|MV_BIT1);
    MV_REG_READ_BYTE(ioBaseAddr, pSataChannel->eDmaRegsOffset +
                     MV_ATA_DEVICE_CONTROL_REG_OFFSET);
    mvMicroSecondsDelay(pSataChannel->mvSataAdapter, 10);
     
    enableStorageDevInterrupt(pSataChannel);
}

MV_BOOLEAN _isDeviceBsyBitOff(MV_SATA_CHANNEL *pSataChannel)
{
    MV_BUS_ADDR_T   ioBaseAddr =
    pSataChannel->mvSataAdapter->adapterIoBaseAddress;
    MV_U8           ATAstatus;
    MV_U32          eDmaRegsOffset = pSataChannel->eDmaRegsOffset;

    ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                 MV_ATA_DEVICE_STATUS_REG_OFFSET);
    if ((ATAstatus & MV_ATA_BUSY_STATUS) == 0)
    {
        return MV_TRUE;
    }
    else
    {
#ifdef MV_LOGGER
        if (pSataChannel->mvSataAdapter->sataAdapterGeneration >=
            MV_SATA_GEN_II)
        {
            MV_U32 ifStatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                               MV_SATA_II_IF_STATUS_REG_OFFSET);
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG,
                     "[%d %d] SATA interface status register = 0x%X\n",
                     pSataChannel->mvSataAdapter->adapterId,
                     pSataChannel->channelNumber,
                     ifStatus);
        }
#endif
        return MV_FALSE;
    }
}

MV_BOOLEAN mvStorageDevATAStartSoftResetDevice(MV_SATA_ADAPTER *pAdapter,
                                               MV_U8 channelIndex,
                                               MV_U8 PMPort
                                              )
{
    MV_SATA_CHANNEL *pSataChannel;
    MV_BUS_ADDR_T   ioBaseAddr;
    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvStorageDevATASoftRes"
                 "etDevice Failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    ioBaseAddr = pAdapter->adapterIoBaseAddress;
    if (pSataChannel == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  channel data structu"
                 "re is not allocated\n", pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }

    mvOsSemTake(&pSataChannel->semaphore);
    if (pSataChannel->queueCommandsEnabled == MV_TRUE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  mvStorageDevATASoft"
                 "ResetDevice command failed: EDMA is active\n",
                 pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }
    _setActivePMPort(pSataChannel, PMPort);
    _startSoftResetDevice(pSataChannel);
    mvOsSemRelease( &pSataChannel->semaphore);
    return MV_TRUE;
}

MV_BOOLEAN mvStorageIsDeviceBsyBitOff(MV_SATA_ADAPTER *pAdapter,
                                      MV_U8 channelIndex,
                                      MV_STORAGE_DEVICE_REGISTERS *registerStruct
                                     )
{
    MV_SATA_CHANNEL *pSataChannel;
    MV_BUS_ADDR_T   ioBaseAddr;
    MV_BOOLEAN      result;
    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvStorageDevATASoftRes"
                 "etDevice Failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    ioBaseAddr = pAdapter->adapterIoBaseAddress;
    if (pSataChannel == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  channel data structu"
                 "re is not allocated\n", pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    mvOsSemTake(&pSataChannel->semaphore);
    result = _isDeviceBsyBitOff(pSataChannel);
    if (registerStruct)
    {
        dumpAtaDeviceRegisters(pAdapter, channelIndex, MV_FALSE,
                               registerStruct);
    }
    mvOsSemRelease( &pSataChannel->semaphore);
    return result;
}

MV_BOOLEAN mvStorageDevATASetFeatures(MV_SATA_ADAPTER *pAdapter,
                                      MV_U8 channelIndex,
                                      MV_U8   PMPort,
                                      MV_U8 subCommand,
                                      MV_U8 subCommandSpecific1,
                                      MV_U8 subCommandSpecific2,
                                      MV_U8 subCommandSpecific3,
                                      MV_U8 subCommandSpecific4)
{
    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG|MV_DEBUG_NON_UDMA_COMMAND,
             "ATA Set Features: %x , %x , %x , %x , %x\n", subCommand,
             subCommandSpecific1, subCommandSpecific2, subCommandSpecific3,
             subCommandSpecific4);
    return mvStorageDevATAExecuteNonUDMACommand(pAdapter, channelIndex,
                                                PMPort,
                                                MV_NON_UDMA_PROTOCOL_NON_DATA,
                                                MV_FALSE,
                                                NULL,     
                                                0,        
                                                subCommand,      
                                                 
                                                subCommandSpecific1,
                                                subCommandSpecific2,     
                                                subCommandSpecific3,     
                                                 
                                                subCommandSpecific4,
                                                0,       
                                                 
                                                MV_ATA_COMMAND_SET_FEATURES);
}

MV_BOOLEAN mvStorageDevATAExecuteNonUDMACommand(MV_SATA_ADAPTER *pAdapter,
                                                MV_U8 channelIndex,
                                                MV_U8 PMPort,
                                                MV_NON_UDMA_PROTOCOL protocolType,
                                                MV_BOOLEAN  isEXT,
                                                MV_U16_PTR bufPtr, MV_U32 count,
                                                MV_U16 features,
                                                MV_U16 sectorCount,
                                                MV_U16 lbaLow, MV_U16 lbaMid,
                                                MV_U16 lbaHigh, MV_U8 device,
                                                MV_U8 command)
{
    MV_BOOLEAN result;
    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvStorageDevATAExecute"
                 "NonUDMACommand Failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    if (pAdapter->sataChannel[channelIndex] == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  mvStorageDevATAExecu"
                 "teNonUDMACommand Failed, channel data structure not allocated"
                 "\n",
                 pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    mvOsSemTake(&pAdapter->sataChannel[channelIndex]->semaphore);
    result = executeNonUDMACommand(pAdapter, channelIndex, PMPort, protocolType,
                                   isEXT, bufPtr, count, features, sectorCount,
                                   lbaLow, lbaMid, lbaHigh, device, command);
    mvOsSemRelease(&pAdapter->sataChannel[channelIndex]->semaphore);
    return result;
}
#if 0
MV_BOOLEAN executePacketCommand(MV_SATA_ADAPTER *pAdapter,
                                 MV_U8 channelIndex,
                                 MV_NON_UDMA_PROTOCOL   protocolType, 
                                 MV_U8  PMPort,
                                 MV_U16_PTR cdb,
                                MV_U8   cdb_len,
                                 MV_U16_PTR dataBufPtr
                                 )
{
    MV_SATA_CHANNEL *pSataChannel = pAdapter->sataChannel[channelIndex];
    MV_BUS_ADDR_T   ioBaseAddr = pAdapter->adapterIoBaseAddress;
    MV_U32          eDmaRegsOffset;
    MV_U32          i;
    MV_U32          count;
    MV_U8           ATAstatus;

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG|MV_DEBUG_NON_UDMA_COMMAND, " %d %d send PACKET "
             " command: protocol(%d) cdb %p cdb len %p buffer %p \n", pAdapter->adapterId,
             channelIndex, protocolType, cdb, cdb_len, dataBufPtr);

    eDmaRegsOffset = pSataChannel->eDmaRegsOffset;
    if ((PMPort) && ((pSataChannel->PMSupported == MV_FALSE) ||
                     (pSataChannel->deviceType != MV_SATA_DEVICE_TYPE_PM)))
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  executePacketCommand"
                 " failed PM not supported for this channel\n",
                 pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }
    {
        if (isStorageDevReadyForPIO(pSataChannel) == MV_FALSE)
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                     " %d %d : Error in Issue NON UDMA command:"
                     " isStorageDevReadyForPIO failed\n",
                     pAdapter->adapterId, channelIndex);

            return MV_FALSE;
        }
    }
    _setActivePMPort(pSataChannel, PMPort);
    if (pSataChannel->queueCommandsEnabled == MV_TRUE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  PIO command failed:"
                 "EDMA is active\n", pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber);
        return MV_FALSE;
    }

    if (pAdapter->sataAdapterGeneration == MV_SATA_GEN_I)
    {
        disableStorageDevInterrupt(pSataChannel);
    }

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_FEATURES_REG_OFFSET, 0);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_SECTOR_COUNT_REG_OFFSET, 0);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_LBA_LOW_REG_OFFSET, 0);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_LBA_MID_REG_OFFSET,  0);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_LBA_HIGH_REG_OFFSET, 0x20);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_HEAD_REG_OFFSET, 0);
    MV_CPU_WRITE_BUFFER_FLUSH();

    if (pAdapter->sataAdapterGeneration >= MV_SATA_GEN_II)
    {
        enableStorageDevInterrupt(pSataChannel);
    }

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_COMMAND_REG_OFFSET,MV_ATA_COMMAND_PACKET );

    if (waitWhileStorageDevIsBusy(pAdapter, ioBaseAddr, eDmaRegsOffset, 3100, 10000) ==
        MV_FALSE)
    {
        enableStorageDevInterrupt(pSataChannel);
        return MV_FALSE;
    }
    if (protocolType == MV_NON_UDMA_PROTOCOL_PACKET_PIO_NON_DATA)
    {
        enableStorageDevInterrupt(pSataChannel);
        pSataChannel->recoveredErrorsCounter = 0;
        return MV_TRUE;
    }
    
    ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                 MV_ATA_DEVICE_STATUS_REG_OFFSET);
    if (pAdapter->sataAdapterGeneration == MV_SATA_GEN_I)
    {
        if (!(ATAstatus & MV_ATA_DATA_REQUEST_STATUS))
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d: DRQ bit in ATA STATUS"
                     " register is not set\n", pAdapter->adapterId, channelIndex);
            enableStorageDevInterrupt(pSataChannel);
            return MV_FALSE;
        }
    }
    if (pAdapter->sataAdapterGeneration >= MV_SATA_GEN_II)
    {

        if (waitForDRQ(pAdapter, ioBaseAddr, eDmaRegsOffset, 500, 10000)
            == MV_FALSE)
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d: DRQ bit in ATA STATUS"
                     " register is not set\n", pAdapter->adapterId, channelIndex);
            enableStorageDevInterrupt(pSataChannel);
            return MV_FALSE;
        }
    }
    for (i = 0; i < cdb_len; i++)
    {
            MV_REG_WRITE_WORD(ioBaseAddr, eDmaRegsOffset +
                              MV_ATA_DEVICE_PIO_DATA_REG_OFFSET, cdb[i]);
            MV_CPU_WRITE_BUFFER_FLUSH();
    }

    if (waitForDRQ(pAdapter, ioBaseAddr, eDmaRegsOffset, 500, 10000)
          == MV_FALSE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d: DRQ bit in ATA STATUS"
                     " register is not set\n", pAdapter->adapterId, channelIndex);
        enableStorageDevInterrupt(pSataChannel);
        return MV_FALSE;
    }

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " Status: %x\n", 
        MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset + MV_ATA_DEVICE_STATUS_REG_OFFSET));

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " Sector Count: %x\n", 
        MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset + MV_ATA_DEVICE_SECTOR_COUNT_REG_OFFSET));

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " LBA Mid: %x\n", 
        MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset + MV_ATA_DEVICE_LBA_MID_REG_OFFSET));

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " LBA High: %x\n", 
        MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset + MV_ATA_DEVICE_LBA_HIGH_REG_OFFSET));
    
    count =  MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset + MV_ATA_DEVICE_LBA_MID_REG_OFFSET) + 
            (MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset + MV_ATA_DEVICE_LBA_HIGH_REG_OFFSET) << 8);
    count >>= 1;
    for ( i = 0 ; i < count; i++)
    {
        if(protocolType == MV_NON_UDMA_PROTOCOL_PACKET_PIO_DATA_IN)
        {
            dataBufPtr[i] = MV_REG_READ_WORD(ioBaseAddr, eDmaRegsOffset +
                              MV_ATA_DEVICE_PIO_DATA_REG_OFFSET);
        }
        else
        {
           MV_REG_WRITE_WORD(ioBaseAddr, eDmaRegsOffset +
                              MV_ATA_DEVICE_PIO_DATA_REG_OFFSET, dataBufPtr[i]);
 
        }
    }
    
    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " %d %d: on non-UDMA sequence - checking if"
             " device is has finished the command\n",
             pAdapter->adapterId, channelIndex);

    if (waitWhileStorageDevIsBusy(pAdapter,
                                  ioBaseAddr, eDmaRegsOffset, 50000, 100) ==
        MV_FALSE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " Status: %x\n", 
            MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset + MV_ATA_DEVICE_STATUS_REG_OFFSET));
        
        enableStorageDevInterrupt(pSataChannel);
        return MV_FALSE;
    }

    if (pAdapter->sataAdapterGeneration >= MV_SATA_GEN_II)
    {

        if (waitForDRQToClear(pAdapter, ioBaseAddr, eDmaRegsOffset, 50000, 100)
            == MV_FALSE)
        {
            enableStorageDevInterrupt(pSataChannel);
            return MV_FALSE;
        }
    }

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " %d %d: Finish NonUdma Command. Status=0x%02x"
             "\n", pAdapter->adapterId, channelIndex,
             MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                              MV_ATA_DEVICE_STATUS_REG_OFFSET));
    enableStorageDevInterrupt(pSataChannel);
    pSataChannel->recoveredErrorsCounter = 0;
    return MV_TRUE;
}
#endif  
MV_BOOLEAN executeNonUDMACommand(MV_SATA_ADAPTER *pAdapter,
                                 MV_U8 channelIndex,
                                 MV_U8  PMPort,
                                 MV_NON_UDMA_PROTOCOL protocolType,
                                 MV_BOOLEAN  isEXT,
                                 MV_U16_PTR bufPtr, MV_U32 count,
                                 MV_U16 features,
                                 MV_U16 sectorCount,
                                 MV_U16 lbaLow, MV_U16 lbaMid,
                                 MV_U16 lbaHigh, MV_U8 device,
                                 MV_U8 command)
{
    MV_SATA_CHANNEL *pSataChannel = pAdapter->sataChannel[channelIndex];
    MV_BUS_ADDR_T   ioBaseAddr = pAdapter->adapterIoBaseAddress;
    MV_U32          eDmaRegsOffset;
    MV_U32          i;
    MV_U8           ATAstatus;

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG|MV_DEBUG_NON_UDMA_COMMAND, " %d %d Issue NON "
             "UDMA command: protocol(%d) %p , %x , %x , %x , %x.%x.%x %x "
             "command=%x\n", pAdapter->adapterId, channelIndex, protocolType,
             bufPtr, count, features, sectorCount, lbaLow, lbaMid,
             lbaHigh, device, command);

    eDmaRegsOffset = pSataChannel->eDmaRegsOffset;
    if ((PMPort) && ((pSataChannel->PMSupported == MV_FALSE) ||
                     (pSataChannel->deviceType != MV_SATA_DEVICE_TYPE_PM)))
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  executeNonUDMACommand"
                 " failed PM not supported for this channel\n",
                 pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }
    if (command != MV_ATA_COMMAND_PM_READ_REG &&
        command != MV_ATA_COMMAND_PM_WRITE_REG)
    {
        if (isStorageDevReadyForPIO(pSataChannel) == MV_FALSE)
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                     " %d %d : Error in Issue NON UDMA command:"
                     " isStorageDevReadyForPIO failed\n",
                     pAdapter->adapterId, channelIndex);

            return MV_FALSE;
        }
    }
    _setActivePMPort(pSataChannel, PMPort);
    if (pSataChannel->queueCommandsEnabled == MV_TRUE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  PIO command failed:"
                 "EDMA is active\n", pSataChannel->mvSataAdapter->adapterId,
                 pSataChannel->channelNumber);
        return MV_FALSE;
    }

    if (pAdapter->sataAdapterGeneration == MV_SATA_GEN_I)
    {
        disableStorageDevInterrupt(pSataChannel);
    }

    if (isEXT == MV_TRUE)
    {
        MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                          MV_ATA_DEVICE_FEATURES_REG_OFFSET,
                          (features & 0xff00) >> 8);
        MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                          MV_ATA_DEVICE_SECTOR_COUNT_REG_OFFSET,
                          (sectorCount & 0xff00) >> 8);
        MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                          MV_ATA_DEVICE_LBA_LOW_REG_OFFSET,
                          (lbaLow & 0xff00) >> 8);
        MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                          MV_ATA_DEVICE_LBA_MID_REG_OFFSET,
                          (lbaMid & 0xff00) >> 8);
        MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                          MV_ATA_DEVICE_LBA_HIGH_REG_OFFSET,
                          (lbaHigh & 0xff00) >> 8);
    }
    else
    {
        if ((features & 0xff00) || (sectorCount & 0xff00) || (lbaLow & 0xff00) ||
            (lbaMid & 0xff00) || (lbaHigh & 0xff00))
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                     " %d %d : Error in Issue NON UDMA command:"
                     " bits[15:8] of register values should be reserved"
                     " Features 0x%02x, SectorCount 0x%02x, LBA Low 0x%02x,"
                     " LBA Mid 0x%02x, LBA High 0x%02x\n",
                     pAdapter->adapterId, channelIndex, features,
                     sectorCount, lbaLow, lbaMid, lbaHigh);
            enableStorageDevInterrupt(pSataChannel);
            return MV_FALSE;
        }
    }

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_FEATURES_REG_OFFSET, features & 0xff);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_SECTOR_COUNT_REG_OFFSET, sectorCount & 0xff);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_LBA_LOW_REG_OFFSET, lbaLow & 0xff);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_LBA_MID_REG_OFFSET, lbaMid & 0xff);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_LBA_HIGH_REG_OFFSET, lbaHigh & 0xff);

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_HEAD_REG_OFFSET, device);
    MV_CPU_WRITE_BUFFER_FLUSH();

    if (pAdapter->sataAdapterGeneration >= MV_SATA_GEN_II)
    {
        enableStorageDevInterrupt(pSataChannel);
    }

    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_COMMAND_REG_OFFSET, command);

    if (protocolType == MV_NON_UDMA_PROTOCOL_NON_DATA)
    {
         
        if (waitWhileStorageDevIsBusy(pAdapter, ioBaseAddr, eDmaRegsOffset, 3100, 10000) ==
            MV_FALSE)
        {
            enableStorageDevInterrupt(pSataChannel);
            return MV_FALSE;
        }
        enableStorageDevInterrupt(pSataChannel);
        pSataChannel->recoveredErrorsCounter = 0;
        return MV_TRUE;
    }
     
    if (waitWhileStorageDevIsBusy(pAdapter, ioBaseAddr, eDmaRegsOffset, 3100, 10000) ==
        MV_FALSE)
    {
        enableStorageDevInterrupt(pSataChannel);
        return MV_FALSE;
    }
     
    ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                 MV_ATA_DEVICE_STATUS_REG_OFFSET);
    if (pAdapter->sataAdapterGeneration == MV_SATA_GEN_I)
    {
        if (!(ATAstatus & MV_ATA_DATA_REQUEST_STATUS))
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d: DRQ bit in ATA STATUS"
                     " register is not set\n", pAdapter->adapterId, channelIndex);
            enableStorageDevInterrupt(pSataChannel);
            return MV_FALSE;
        }
    }
    if (pAdapter->sataAdapterGeneration >= MV_SATA_GEN_II)
    {

        if (waitForDRQ(pAdapter, ioBaseAddr, eDmaRegsOffset, 500, 10000)
            == MV_FALSE)
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d: DRQ bit in ATA STATUS"
                     " register is not set\n", pAdapter->adapterId, channelIndex);
            enableStorageDevInterrupt(pSataChannel);
            return MV_FALSE;
        }
    }
    for (i = 0; i < count; i++)
    {
         
        if ((i & (((MV_U32)pSataChannel->DRQDataBlockSize * ATA_SECTOR_SIZE_IN_WORDS) - 1)) == 0)
        {
            if (pAdapter->sataAdapterGeneration >= MV_SATA_GEN_II)
            {
                 
                MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                 MV_ATA_DEVICE_ALTERNATE_REG_OFFSET);
                 
                if (i != 0)
                {
                    if (waitWhileStorageDevIsBusy_88SX60X1(pAdapter,
                                                                                  ioBaseAddr, eDmaRegsOffset, channelIndex,
                                                                                  50000, 100) == MV_FALSE)
                    {
                        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                                 "Sata device interrupt timeout...i = %d\n",i);
                        enableStorageDevInterrupt(pSataChannel);
                        return MV_FALSE;
                    }
                }
                else
                {
                    MV_U8 sataUnit = channelIndex >> 2,portNum = channelIndex & 3;

                    if (waitWhileStorageDevIsBusy(pAdapter,ioBaseAddr,
                                                  eDmaRegsOffset, 50000, 100) == MV_FALSE)
                    {
                        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                                 "Busy bit timeout...i = %d\n",i);
                        enableStorageDevInterrupt(pSataChannel);
                        return MV_FALSE;
                    }
                    MV_REG_WRITE_DWORD (ioBaseAddr, MV_SATAHC_REGS_BASE_OFFSET(sataUnit) +
                                        MV_SATAHC_INTERRUPT_CAUSE_REG_OFFSET,
                                        ~(1 << (8 + portNum)));
                }
                if (waitForDRQ(pAdapter, ioBaseAddr, eDmaRegsOffset, 50000, 100)
                    == MV_FALSE)
                {
                    enableStorageDevInterrupt(pSataChannel);
                    return MV_FALSE;
                }
            }
            else if (pAdapter->sataAdapterGeneration == MV_SATA_GEN_I)
            {
                if (waitWhileStorageDevIsBusy(pAdapter,
                                              ioBaseAddr, eDmaRegsOffset,
                                              50000, 100) == MV_FALSE)
                {
                    enableStorageDevInterrupt(pSataChannel);
                    return MV_FALSE;
                }
            }
        }
        if (protocolType == MV_NON_UDMA_PROTOCOL_PIO_DATA_IN)
        {
            bufPtr[i] = MV_REG_READ_WORD(ioBaseAddr, eDmaRegsOffset +
                                         MV_ATA_DEVICE_PIO_DATA_REG_OFFSET);
        }
        else
        {
            MV_REG_WRITE_WORD(ioBaseAddr, eDmaRegsOffset +
                              MV_ATA_DEVICE_PIO_DATA_REG_OFFSET, bufPtr[i]);
            MV_CPU_WRITE_BUFFER_FLUSH();
        }
    }

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " %d %d: on non-UDMA sequence - checking if"
             " device is has finished the command\n",
             pAdapter->adapterId, channelIndex);

    if (waitWhileStorageDevIsBusy(pAdapter,
                                  ioBaseAddr, eDmaRegsOffset, 50000, 100) ==
        MV_FALSE)
    {
        enableStorageDevInterrupt(pSataChannel);
        return MV_FALSE;
    }

    if (pAdapter->sataAdapterGeneration >= MV_SATA_GEN_II)
    {

        if (waitForDRQToClear(pAdapter, ioBaseAddr, eDmaRegsOffset, 50000, 100)
            == MV_FALSE)
        {
            enableStorageDevInterrupt(pSataChannel);
            return MV_FALSE;
        }
    }

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, " %d %d: Finish NonUdma Command. Status=0x%02x"
             "\n", pAdapter->adapterId, channelIndex,
             MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                              MV_ATA_DEVICE_STATUS_REG_OFFSET));
    enableStorageDevInterrupt(pSataChannel);
    pSataChannel->recoveredErrorsCounter = 0;
    return MV_TRUE;
}
MV_BOOLEAN  _PMAccessReg(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex,
                         MV_U8 PMPort, MV_U8 PMReg, MV_U32 *pValue,
                         MV_STORAGE_DEVICE_REGISTERS *registerStruct,
                         MV_BOOLEAN isRead)
{
    MV_BOOLEAN result;

    if (isRead == MV_TRUE)
    {
        result = executeNonUDMACommand(pAdapter, channelIndex,
                                       MV_SATA_PM_CONTROL_PORT,
                                       MV_NON_UDMA_PROTOCOL_NON_DATA,
                                       MV_TRUE ,
                                       NULL ,
                                       0 ,
                                       PMReg  , 0 ,
                                       0  , 0  , 0  ,
                                       PMPort ,
                                       MV_ATA_COMMAND_PM_READ_REG );
        if (result == MV_TRUE)
        {
            MV_BUS_ADDR_T   ioBaseAddr = pAdapter->adapterIoBaseAddress;
            MV_U32 eDmaRegsOffset = pAdapter->sataChannel[channelIndex]->eDmaRegsOffset;

            *pValue = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                       MV_ATA_DEVICE_SECTOR_COUNT_REG_OFFSET);
            *pValue |= MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                        MV_ATA_DEVICE_LBA_LOW_REG_OFFSET) << 8;
            *pValue |= MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                        MV_ATA_DEVICE_LBA_MID_REG_OFFSET) << 16;
            *pValue |= MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                        MV_ATA_DEVICE_LBA_HIGH_REG_OFFSET) << 24;
        }
    }
    else
    {
        result = executeNonUDMACommand(pAdapter, channelIndex,
                                       MV_SATA_PM_CONTROL_PORT,
                                       MV_NON_UDMA_PROTOCOL_NON_DATA,
                                       MV_TRUE ,
                                       NULL ,
                                       0 ,
                                       PMReg  ,
                                       (MV_U16)((*pValue) & 0xff) ,
                                       (MV_U16)(((*pValue) & 0xff00) >> 8)  ,
                                       (MV_U16)(((*pValue) & 0xff0000) >> 16)    ,
                                       (MV_U16)(((*pValue) & 0xff000000) >> 24)  ,
                                       PMPort ,
                                       MV_ATA_COMMAND_PM_WRITE_REG );
    }
    if (registerStruct)
    {
        dumpAtaDeviceRegisters(pAdapter, channelIndex, MV_FALSE,
                               registerStruct);
    }
    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG|MV_DEBUG_PM, " %d %d: %s PM Reg %s: PM Port %x"
             ", PM Reg %d, value %x\n", pAdapter->adapterId, channelIndex,
             (isRead == MV_TRUE) ? "Read" : "Write",
             (result == MV_TRUE) ? "Succeeded" : "Failed",
             PMPort, PMReg, *pValue);

    return result;
}

MV_BOOLEAN waitForDRQToClear(MV_SATA_ADAPTER* pAdapter,
                             MV_BUS_ADDR_T ioBaseAddr,
                             MV_U32 eDmaRegsOffset, MV_U32 loops,
                             MV_U32 delayParam)
{
    MV_U8   ATAstatus = 0;
    MV_U32  i;

    for (i = 0;i < loops; i++)
    {
        ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                     MV_ATA_DEVICE_STATUS_REG_OFFSET);
        if ((ATAstatus & MV_ATA_BUSY_STATUS) == 0)
        {
            if (!(ATAstatus & MV_ATA_DATA_REQUEST_STATUS))
            {
                mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG, "waitWhileStorageDevIsBusy: %d loops *"
                         "%d usecs\n", i, delayParam);
                return MV_TRUE;
            }
        }
        mvMicroSecondsDelay(pAdapter, delayParam);
    }
    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, "waitWhileStorageDevIsBusy<FAILED>: Time out - Device ERROR"
             " Status: 0x%02x. loops %d, delay %d\n", ATAstatus, loops, delayParam);

    return MV_FALSE;
}

void dumpAtaDeviceRegisters(MV_SATA_ADAPTER *pAdapter,
                            MV_U8 channelIndex, MV_BOOLEAN isEXT,
                            MV_STORAGE_DEVICE_REGISTERS *pRegisters)
{
    MV_BUS_ADDR_T   ioBaseAddr = pAdapter->adapterIoBaseAddress;
    MV_U32 eDmaRegsOffset = pAdapter->sataChannel[channelIndex]->eDmaRegsOffset;

	if (pAdapter->sataAdapterGeneration < MV_SATA_GEN_IIE)
	{
	    if (MV_REG_READ_DWORD(ioBaseAddr, eDmaRegsOffset +
			MV_EDMA_COMMAND_REG_OFFSET) & MV_BIT0)
		{
			mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR,
				" %d %d: dumpAtaDeviceRegisters: Edma is active!!!\n",
				pAdapter->adapterId, channelIndex);
			return;
		}
	}
    MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                      MV_ATA_DEVICE_CONTROL_REG_OFFSET, 0);

    pRegisters->errorRegister =
    MV_REG_READ_BYTE(ioBaseAddr,
                     eDmaRegsOffset + MV_ATA_DEVICE_ERROR_REG_OFFSET);

    pRegisters->sectorCountRegister =
    MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                     MV_ATA_DEVICE_SECTOR_COUNT_REG_OFFSET) & 0x00ff;
    pRegisters->lbaLowRegister =
    MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                     MV_ATA_DEVICE_LBA_LOW_REG_OFFSET) & 0x00ff;

    pRegisters->lbaMidRegister =
    MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                     MV_ATA_DEVICE_LBA_MID_REG_OFFSET) & 0x00ff;

    pRegisters->lbaHighRegister =
    MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                     MV_ATA_DEVICE_LBA_HIGH_REG_OFFSET) & 0x00ff;

    if (isEXT == MV_TRUE)
    {
         
        MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                          MV_ATA_DEVICE_CONTROL_REG_OFFSET, MV_BIT7);

        pRegisters->sectorCountRegister |= (MV_REG_READ_BYTE(ioBaseAddr,
                                                             eDmaRegsOffset +
                                                             MV_ATA_DEVICE_SECTOR_COUNT_REG_OFFSET) << 8) & 0xff00;

        pRegisters->lbaLowRegister |= (MV_REG_READ_BYTE(ioBaseAddr,
                                                        eDmaRegsOffset + MV_ATA_DEVICE_LBA_LOW_REG_OFFSET) << 8)
                                      & 0xff00;

        pRegisters->lbaMidRegister |= (MV_REG_READ_BYTE(ioBaseAddr,
                                                        eDmaRegsOffset + MV_ATA_DEVICE_LBA_MID_REG_OFFSET) << 8)
                                      & 0xff00;

        pRegisters->lbaHighRegister |= (MV_REG_READ_BYTE(ioBaseAddr,
                                                         eDmaRegsOffset + MV_ATA_DEVICE_LBA_HIGH_REG_OFFSET) << 8)
                                       & 0xff00;
        MV_REG_WRITE_BYTE(ioBaseAddr, eDmaRegsOffset +
                          MV_ATA_DEVICE_CONTROL_REG_OFFSET, 0);

    }

    pRegisters->deviceRegister = MV_REG_READ_BYTE(ioBaseAddr,
                                                  eDmaRegsOffset + MV_ATA_DEVICE_HEAD_REG_OFFSET);

    pRegisters->statusRegister = MV_REG_READ_BYTE(ioBaseAddr,
                                                  eDmaRegsOffset + MV_ATA_DEVICE_STATUS_REG_OFFSET);

}

MV_BOOLEAN _doSoftReset(MV_SATA_CHANNEL *pSataChannel)
{
    MV_BUS_ADDR_T   ioBaseAddr = pSataChannel->mvSataAdapter->adapterIoBaseAddress;
    MV_U32          i;
    MV_U8           ATAstatus;
    MV_U32          eDmaRegsOffset = pSataChannel->eDmaRegsOffset;

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_NON_UDMA_COMMAND | MV_DEBUG, "Issue SRST COMMAND\n");

    MV_REG_WRITE_BYTE(ioBaseAddr, pSataChannel->eDmaRegsOffset +
                      MV_ATA_DEVICE_CONTROL_REG_OFFSET, MV_BIT2|MV_BIT1);
    MV_REG_READ_BYTE(ioBaseAddr, pSataChannel->eDmaRegsOffset +
                     MV_ATA_DEVICE_CONTROL_REG_OFFSET);
    mvMicroSecondsDelay(pSataChannel->mvSataAdapter, 10);

    enableStorageDevInterrupt(pSataChannel);

    mvMicroSecondsDelay(pSataChannel->mvSataAdapter, 500);
    mvMicroSecondsDelay(pSataChannel->mvSataAdapter, 500);
    mvMicroSecondsDelay(pSataChannel->mvSataAdapter, 500);
    mvMicroSecondsDelay(pSataChannel->mvSataAdapter, 500);

    for (i = 0;i < 31000; i++)
    {
        ATAstatus = MV_REG_READ_BYTE(ioBaseAddr, eDmaRegsOffset +
                                     MV_ATA_DEVICE_STATUS_REG_OFFSET);
        if ((ATAstatus & MV_ATA_BUSY_STATUS) == 0)
        {
            return MV_TRUE;
        }
        mvMicroSecondsDelay(pSataChannel->mvSataAdapter, 1000);
    }
    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d: Software reset failed "
             "Status=0x%02x\n", pSataChannel->mvSataAdapter->adapterId,
             pSataChannel->channelNumber, ATAstatus);

    return MV_FALSE;
}

MV_BOOLEAN  mvPMDevReadReg(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex,
                           MV_U8 PMPort, MV_U8 PMReg, MV_U32 *pValue,
                           MV_STORAGE_DEVICE_REGISTERS *registerStruct)
{
    MV_SATA_CHANNEL *pSataChannel;
    MV_BOOLEAN      result;

    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvStorageDevPMReadReg"
                 " Failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    if (pSataChannel == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  channel data structu"
                 "re is not allocated\n", pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }

    mvOsSemTake(&pSataChannel->semaphore);
    if (pSataChannel->PMSupported == MV_FALSE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  mvStorageDevPMReadReg"
                 " failed PM not supported for this channel\n",
                 pAdapter->adapterId, channelIndex);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }
    if (pSataChannel->queueCommandsEnabled == MV_TRUE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  mvStorageDevPMReadReg"
                 " command failed: EDMA is active\n",
                 pAdapter->adapterId, channelIndex);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }

    result = _PMAccessReg(pAdapter, channelIndex, PMPort, PMReg, pValue,
                          registerStruct, MV_TRUE);

    mvOsSemRelease( &pSataChannel->semaphore);
    return result;
}

MV_BOOLEAN  mvPMDevWriteReg(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex,
                            MV_U8 PMPort, MV_U8 PMReg, MV_U32 value,
                            MV_STORAGE_DEVICE_REGISTERS *registerStruct)
{
    MV_SATA_CHANNEL *pSataChannel;
    MV_BOOLEAN      result;

    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvStorageDevPMWriteReg"
                 " Failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    if (pSataChannel == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  channel data structu"
                 "re is not allocated\n", pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }

    mvOsSemTake(&pSataChannel->semaphore);
    if (pSataChannel->PMSupported == MV_FALSE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  mvStorageDevPMWriteReg"
                 " failed PM not supported for this channel\n",
                 pAdapter->adapterId, channelIndex);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }
    if (pSataChannel->queueCommandsEnabled == MV_TRUE)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR, " %d %d:  mvStorageDevPMWriteReg"
                 " command failed: EDMA is active\n",
                 pAdapter->adapterId, channelIndex);
        mvOsSemRelease( &pSataChannel->semaphore);
        return MV_FALSE;
    }

    result = _PMAccessReg(pAdapter, channelIndex, PMPort, PMReg, &value,
                          registerStruct, MV_FALSE);

    mvOsSemRelease( &pSataChannel->semaphore);
    return result;
}

static MV_BOOLEAN _checkPMPortSStatus(MV_SATA_ADAPTER* pAdapter,
                                      MV_U8 channelIndex,
                                      MV_U8 PMPort,
                                      MV_BOOLEAN *error)
{
    MV_BOOLEAN result;
    MV_U32 SStatus;

    result = mvPMDevReadReg(pAdapter, channelIndex, PMPort,
                            MV_SATA_PSCR_SSTATUS_REG_NUM, &SStatus, NULL);

    if (result == MV_FALSE)
    {
        *error = MV_TRUE;
        return result;
    }
    *error = MV_FALSE;
    SStatus &= (MV_BIT0 | MV_BIT1 | MV_BIT2);
    if ((SStatus == (MV_BIT0 | MV_BIT1)) || (SStatus == 0))
    {
        return MV_TRUE;
    }
    return MV_FALSE;
}

MV_BOOLEAN  mvPMLinkUp(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex, MV_U8 PMPort,
		       MV_BOOLEAN force_gen1)
{
    MV_BOOLEAN  result;
    MV_U32	speed_force = 0;
    
    if(force_gen1 == MV_TRUE)
	 speed_force = 0x10;

    result = mvPMDevWriteReg(pAdapter, channelIndex, PMPort,
                             MV_SATA_PSCR_SCONTROL_REG_NUM, 0x301 | speed_force, NULL);
    if (result == MV_FALSE)
    {
        return result;
    }
    mvMicroSecondsDelay(pAdapter, MV_SATA_COMM_INIT_DELAY);
    result = mvPMDevWriteReg(pAdapter, channelIndex, PMPort,
                             MV_SATA_PSCR_SCONTROL_REG_NUM, 0x300 | speed_force, NULL);
    return result;
}

MV_BOOLEAN  mvPMDevEnableStaggeredSpinUp(MV_SATA_ADAPTER *pAdapter,
                                         MV_U8 channelIndex, MV_U8 PMPort)
{
    return mvPMLinkUp(pAdapter, channelIndex, PMPort, MV_FALSE);
}

MV_BOOLEAN mvPMDevEnableStaggeredSpinUpAll(MV_SATA_ADAPTER *pSataAdapter,
                                           MV_U8 channelIndex,
                                           MV_U8 PMNumOfPorts,
                                           MV_U16 *bitmask)
{
    MV_U8 PMPort;
    MV_U8 retryCount;
    MV_U8 tmpBitmask = 1;
    if (bitmask == NULL)
    {
        return MV_FALSE;
    }
     
    *bitmask = 1;
    for (PMPort = 0; PMPort < PMNumOfPorts; PMPort++)
    {
        MV_BOOLEAN error;
         
        if (_checkPMPortSStatus(pSataAdapter, channelIndex, PMPort, &error) ==
             MV_TRUE)
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG,
                     "[%d %d %d]: sata communication already established.\n",
                     pSataAdapter->adapterId, channelIndex, PMPort);
            tmpBitmask |= (1 << PMPort);
            continue;
        }
        if (mvPMDevEnableStaggeredSpinUp(pSataAdapter,
                                         channelIndex,
                                         PMPort) == MV_TRUE)
        {
            tmpBitmask |= (1 << PMPort);
        }
        else
        {
            mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                     "Error [%d %d %d]: "
                     "PM enable staggered spin-up failed.\n",
                     pSataAdapter->adapterId, channelIndex, PMPort);
            return MV_FALSE;
        }
    }
    mvMicroSecondsDelay(pSataAdapter, MV_SATA_COMM_INIT_WAIT_DELAY);
    for (retryCount = 0; retryCount < 200; retryCount++)
    {
        for (PMPort = 0; PMPort < PMNumOfPorts; PMPort++)
        {
            MV_BOOLEAN error;
            if ((*bitmask) & (1 << PMPort))
            {
                continue;
            }
            if (_checkPMPortSStatus(pSataAdapter,
                                    channelIndex, PMPort, &error) == MV_FALSE)
            {
                if (error == MV_TRUE)
                {
                    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                             "[%d %d %d]: "
                             "Fatal error - cannot read PM port SStatus.\n",
                             pSataAdapter->adapterId, channelIndex, PMPort);
                    break;
                }
                mvMicroSecondsDelay(pSataAdapter, 1000);
            }
            else
            {
                if (bitmask != NULL)
                {
                    *bitmask |= (1 << PMPort);
                }
                mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG,
                         "[%d %d %d] PM SATA PHY ready after %d msec\n",
                         pSataAdapter->adapterId, channelIndex,
                         PMPort, retryCount);
            }
        }
        if (tmpBitmask == *bitmask)
        {
            break;
        }
    }
    if (tmpBitmask != *bitmask)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                 "[%d %d %d]: "
                 "Some of PM ports PHY are not initialized.\n",
                 pSataAdapter->adapterId, channelIndex, PMPort);

    }
    return MV_TRUE;
}

MV_BOOLEAN mvPMDevEnableStaggeredSpinUpPort(MV_SATA_ADAPTER *pSataAdapter,
					    MV_U8 channelIndex,
 					    MV_U8 PMPort,
 					    MV_BOOLEAN force_speed_gen1)

{
    MV_U8 retryCount;

    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG,
	     "[%d %d %d]: init sata communication.\n",
	     pSataAdapter->adapterId, channelIndex, PMPort);
 
    if (mvPMLinkUp(pSataAdapter, channelIndex, PMPort,
 		   force_speed_gen1) != MV_TRUE)
    {
	    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
                     "Error [%d %d %d]: "
                     "PM enable staggered spin-up failed.\n",
                     pSataAdapter->adapterId, channelIndex, PMPort);
            return MV_FALSE;
    }
    mvMicroSecondsDelay(pSataAdapter, MV_SATA_COMM_INIT_WAIT_DELAY);
    for (retryCount = 0; retryCount < 200; retryCount++)
    {
            MV_BOOLEAN error;
            
            if (_checkPMPortSStatus(pSataAdapter,
                                    channelIndex, PMPort, &error) == MV_FALSE)
            {
		    if (error == MV_TRUE)
		    {
			    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ERROR,
				     "[%d %d %d]: "
				     "Fatal error - cannot read PM port SStatus.\n",
				     pSataAdapter->adapterId, channelIndex, PMPort);
			    return MV_FALSE;
		    }
		    mvMicroSecondsDelay(pSataAdapter, 1000);
            }
            else
            {
		    mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG,
			     "[%d %d %d] PM SATA PHY ready after %d msec\n",
			     pSataAdapter->adapterId, channelIndex,
			     PMPort, retryCount);
		    break;
            }
        
    }

    return MV_TRUE;
}

MV_BOOLEAN mvStorageDevExecutePIO(MV_SATA_ADAPTER *pAdapter,
                                  MV_U8 channelIndex,
                                  MV_U8 PMPort,
                                  MV_NON_UDMA_PROTOCOL protocolType,
                                  MV_BOOLEAN  isEXT, MV_U16_PTR bufPtr,
                                  MV_U32 count,
                                  MV_STORAGE_DEVICE_REGISTERS *pInATARegs,
                                  MV_STORAGE_DEVICE_REGISTERS *pOutATARegs)
{
    MV_BOOLEAN result;
    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  mvPMDevExecutePIO"
                 "Command Failed, Bad adapter data structure pointer\n");
        return MV_FALSE;
    }
    if (pAdapter->sataChannel[channelIndex] == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  mvPMDevExecutePIO"
                 "Command Failed, channel data structure not allocated\n",
                 pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }
    mvOsSemTake(&pAdapter->sataChannel[channelIndex]->semaphore);
    result = executeNonUDMACommand(pAdapter, channelIndex, PMPort,
                                   protocolType, isEXT,
                                   bufPtr, count, pInATARegs->featuresRegister,
                                   pInATARegs->sectorCountRegister,
                                   pInATARegs->lbaLowRegister,
                                   pInATARegs->lbaMidRegister,
                                   pInATARegs->lbaHighRegister,
                                   pInATARegs->deviceRegister,
                                   pInATARegs->commandRegister);
    if (pOutATARegs)
    {
        dumpAtaDeviceRegisters(pAdapter, channelIndex, isEXT, pOutATARegs);
    }
    mvOsSemRelease(&pAdapter->sataChannel[channelIndex]->semaphore);
    return result;
}

MV_BOOLEAN  mvStorageDevSetDeviceType(MV_SATA_ADAPTER *pAdapter, MV_U8 channelIndex,
                                      MV_SATA_DEVICE_TYPE deviceType)
{
    MV_SATA_CHANNEL *pSataChannel;

    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  "
                 " mvStorageDevSetDeviceType Failed, Bad adapter data structure "
                 "pointer\n");
        return MV_SATA_DEVICE_TYPE_UNKNOWN;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    if (pSataChannel == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  channel data structu"
                 "re is not allocated\n", pAdapter->adapterId, channelIndex);
        return MV_FALSE;
    }

    pSataChannel->deviceType = deviceType;
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
    pSataChannel->oldDeviceType = deviceType;
#endif

    return MV_TRUE;
}

MV_SATA_DEVICE_TYPE mvStorageDevGetDeviceType(MV_SATA_ADAPTER *pAdapter,
                                              MV_U8 channelIndex)
{
    MV_SATA_CHANNEL *pSataChannel;

    if (pAdapter == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, "    :  "
                 " mvStorageDevGetDeviceType Failed, Bad adapter data structure "
                 "pointer\n");
        return MV_SATA_DEVICE_TYPE_UNKNOWN;
    }
    pSataChannel = pAdapter->sataChannel[channelIndex];
    if (pSataChannel == NULL)
    {
        mvLogMsg(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR, " %d %d:  "
                 "channel data structure is not allocated\n",
                 pAdapter->adapterId, channelIndex);
        return MV_SATA_DEVICE_TYPE_UNKNOWN;
    }

    return pSataChannel->deviceType;
}

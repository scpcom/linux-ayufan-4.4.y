#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif


#ifndef __INCmvIALCommonh
#define __INCmvIALCommonh

#ifdef __cplusplus
extern "C" {
#endif 


#include "mvSata.h"
#include "mvStorageDev.h"

#if defined(MY_ABC_HERE)
#include <linux/synosata.h>
#endif




#define MV_IAL_ASYNC_TIMER_PERIOD       500
#define MV_IAL_SRST_TIMEOUT             31000
#define MV_IAL_WAIT_FOR_RDY_TIMEOUT     10000


#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
#define syno_eh_printk(pMvSataAdapter, channel, fmt, args...) \
        printk("mvSata[%d %d]: "fmt".\n", pMvSataAdapter->adapterId, channel, ##args)
#endif
#ifdef MY_ABC_HERE
extern struct workqueue_struct *mvSata_aux_wq;
#endif

typedef enum mvAdapterState
{
    ADAPTER_INITIALIZING,
    ADAPTER_READY,
    ADAPTER_FATAL_ERROR
} MV_ADAPTER_STATE;

typedef enum mvChannelState
{
    CHANNEL_NOT_CONNECTED,
    CHANNEL_CONNECTED,
    CHANNEL_IN_SRST,
    CHANNEL_PM_INIT_DEVICES,
    CHANNEL_READY,
    CHANNEL_PM_HOT_PLUG,
} MV_CHANNEL_STATE;

typedef struct mvDriveSerialNumber
{
    MV_U8 serial[IDEN_SERIAL_NUM_SIZE];    
}   MV_DRIVE_SERIAL_NUMBER;

typedef struct mvDrivesInfo
{
    MV_U16                      drivesSnapshotSaved;
    MV_DRIVE_SERIAL_NUMBER      driveSerialSaved[MV_SATA_PM_MAX_PORTS];    
    MV_U16                      drivesSnapshotCurrent;
    MV_DRIVE_SERIAL_NUMBER      driveSerialCurrent[MV_SATA_PM_MAX_PORTS];    
}   MV_DRIVES_INFO;

typedef enum mvPortState
{
    MV_PORT_NOT_INITIALIZED,
    MV_PORT_WAIT_FOR_RDY, 
    MV_PORT_ISSUE_SRST,
    MV_PORT_IN_SRST,
    MV_PORT_INIT_DEVICE,
    MV_PORT_DONE, 
    MV_PORT_FAILED
} MV_PORT_STATE;

typedef struct mvIALChannelExtension
{
    MV_U8                       PMnumberOfPorts;
    MV_U16                      PMdevsToInit;
    MV_U8                       devInSRST;
    MV_PORT_STATE		port_state;
    MV_BOOLEAN                  completionError;
    MV_U8                       pmAccessType;
    MV_U8                       pmReg;
    MV_BOOLEAN                  pmRegAccessInProgress;
    MV_BOOLEAN                  pmAsyncNotifyEnabled;
    MV_U8                       pmRegPollCounter;
    MV_U32                      SRSTTimerThreshold;
    MV_U32                      SRSTTimerValue;
    MV_VOID_PTR                 IALChannelPendingCmdQueue;
    MV_BOOLEAN                  bHotPlug;
    MV_DRIVES_INFO              drivesInfo;
    MV_STORAGE_DEVICE_REGISTERS mvStorageDevRegisters;
#ifdef MV_SATA_STORE_COMMANDS_INFO_ON_IAL_STACK
    MV_QUEUE_COMMAND_INFO       commandInfo;
#endif
} MV_IAL_COMMON_CHANNEL_EXTENSION;

typedef struct mvIALCommonAdapterExtension
{
    MV_SATA_ADAPTER   *pSataAdapter;
    MV_ADAPTER_STATE  adapterState;
    MV_CHANNEL_STATE  channelState[MV_SATA_CHANNELS_NUM];
    MV_IAL_COMMON_CHANNEL_EXTENSION IALChannelExt[MV_SATA_CHANNELS_NUM];
} MV_IAL_COMMON_ADAPTER_EXTENSION;


MV_BOOLEAN mvAdapterStartInitialization(MV_SATA_ADAPTER* pSataAdapter,
                                        MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

void mvRestartChannel(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                      MV_U8 channelIndex,
                      MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt,
                      MV_BOOLEAN    bBusReset);

void mvStopChannel(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                   MV_U8 channelIndex,
                   MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE) || defined(MY_ABC_HERE)
void SynomvStopChannel(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                   MV_U8 channelIndex,
                   MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);
extern void SynoIALSCSINotify(struct mvSataAdapter *pSataAdapter, MV_U16 drivesSnapshotSave, MV_U8 channelIndex);
#endif

#ifdef MY_ABC_HERE
MV_U32 syno_mvSata_pmp_read_gpio(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt, 
                                 MV_U8 channelIndex, 
                                 SYNO_PM_PKG *pPM_pkg);

MV_U32 syno_mvSata_pmp_write_gpio(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt, 
                                  MV_U8 channelIndex, 
                                  SYNO_PM_PKG *pPM_pkg);
MV_BOOLEAN syno_mvSata_is_synology_pm(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt, MV_U8 channelIndex);
#endif

#ifdef MY_ABC_HERE
void syno_mvSata_pm_power_ctl(MV_IAL_COMMON_ADAPTER_EXTENSION *pIALExt,
                              MV_U8 channelIndex,
                              SYNO_PM_PKG *pPKG,
                              MV_U8 blPowerOn,
                              MV_U8 blHotplug);
#endif

#ifdef MY_ABC_HERE
void SynoChannelErrorHandle(struct work_struct *work);
extern void channel_do_scsi_done(MV_VOID_PTR pAdapter, struct mvSataAdapter *pSataAdapter, MV_U8 channel);
extern void SynoInitChannelEH(MV_VOID_PTR *pAdapter, MV_SATA_ADAPTER *pMvSataAdapter);
#endif

void mvPMHotPlugDetected(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                         MV_U8 channelIndex,
                         MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

MV_SCSI_COMMAND_STATUS_TYPE mvExecuteScsiCommand(MV_SATA_SCSI_CMD_BLOCK  *pScb,
                                                 MV_BOOLEAN canQueue);

MV_BOOLEAN  mvIALTimerCallback(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                               MV_SAL_ADAPTER_EXTENSION *scsiAdapterExt);

void mvCommandCompletionErrorHandler(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                     MV_U8 channelIndex);

MV_BOOLEAN mvRemoveFromSCSICommandQueue(MV_IAL_COMMON_ADAPTER_EXTENSION *ialExt,
                                        MV_U8 channelIndex,
                                        MV_SATA_SCSI_CMD_BLOCK *pScb);



MV_BOOLEAN IALConfigQueuingMode(MV_SATA_ADAPTER *pSataAdapter,
                                MV_U8 channelIndex,
                                MV_EDMA_MODE mode,
                                MV_SATA_SWITCHING_MODE switchingMode,
                                MV_BOOLEAN  use128Entries);

MV_BOOLEAN IALInitChannel(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex);

void IALReleaseChannel(MV_SATA_ADAPTER *pSataAdapter, MV_U8 channelIndex);
MV_BOOLEAN IALBusChangeNotify(MV_SATA_ADAPTER *pSataAdapter,
                              MV_U8 channelIndex);
#ifdef MY_ABC_HERE
MV_BOOLEAN IALBusChangeNotifyEx(MV_SATA_ADAPTER *pSataAdapter, 
                                MV_U8 channelIndex, 
                                MV_U16 targetsToRemove,
                                MV_U16 targetsToAdd,
                                MV_U32 eh_flag);
#else
MV_BOOLEAN IALBusChangeNotifyEx(MV_SATA_ADAPTER *pSataAdapter,
                                MV_U8 channelIndex,
                                MV_U16 targetsToRemove,
                                MV_U16 targetsToAdd);
#endif

#ifdef __cplusplus
}
#endif 

#endif 

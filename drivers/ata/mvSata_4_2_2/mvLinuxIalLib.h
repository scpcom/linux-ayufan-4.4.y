#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvLinuxIalLibh
#define __INCmvLinuxIalLibh

#include "mvLinuxIalHt.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION (2,4,23)
#define irqreturn_t         void
#define IRQ_RETVAL(foo)
#endif
#define MV_LINUX_ASYNC_TIMER_PERIOD       ((MV_IAL_ASYNC_TIMER_PERIOD * HZ) / 1000)

struct pci_dev;
struct IALAdapter;
struct IALHost;
struct mv_comp_info;

int mv_ial_lib_allocate_edma_queues(struct IALAdapter *pAdapter);

void mv_ial_lib_free_edma_queues(struct IALAdapter *pAdapter);

int mv_ial_lib_init_channel(struct IALAdapter *pAdapter, MV_U8 channelNum);

void mv_ial_lib_free_channel(struct IALAdapter *pAdapter, MV_U8 channelNum);

#ifndef MV_PRD_TABLE_SIZE
 #define MV_PRD_TABLE_SIZE                  64  
#endif

int mv_ial_lib_prd_destroy(struct IALHost *pHost);
int mv_ial_lib_prd_init(struct IALHost *);

int mv_ial_lib_generate_prd(MV_SATA_ADAPTER *pMvSataAdapter, struct scsi_cmnd *SCpnt,
                            struct mv_comp_info *);

irqreturn_t mv_ial_lib_int_handler (int irq, void *dev_id);

MV_BOOLEAN mv_ial_lib_udma_command_completion_call_back(MV_SATA_ADAPTER *pMvSataAdapter,
                                           MV_U8 channelNum,
                                           MV_COMPLETION_TYPE comp_type,
                                           void *commandId,
                                           MV_U16 responseFlags,
                                           MV_U32 timeStamp,
                                           MV_STORAGE_DEVICE_REGISTERS *registerStruct);

MV_BOOLEAN mv_ial_lib_event_notify(MV_SATA_ADAPTER *pMvSataAdapter, MV_EVENT_TYPE eventType,
                             MV_U32 param1, MV_U32 param2);
void asyncStartTimerFunction(unsigned long data);

void mv_ial_lib_add_done_queue (struct IALAdapter *pAdapter,
                                MV_U8 channel,
                                struct scsi_cmnd   *scsi_cmnd);

struct scsi_cmnd * mv_ial_lib_get_first_cmnd (struct IALAdapter *pAdapter,
                                       MV_U8 channel);
#ifdef MY_ABC_HERE
struct scsi_cmnd * syno_ial_lib_clear_cmnd (struct IALAdapter *pAdapter, MV_U8 channel);
#endif

void mv_ial_lib_do_done (struct scsi_cmnd *cmnd);

void mv_ial_block_requests(struct IALAdapter *pAdapter, MV_U8 channelIndex);

#endif  

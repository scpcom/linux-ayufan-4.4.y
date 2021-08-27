#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __INCmvLinuxIalHth
#define __INCmvLinuxIalHth

#include <linux/version.h>
#include <generated/autoconf.h>
#include <linux/module.h>
#include <generated/autoconf.h>
#include <linux/init.h>
#include <linux/types.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#include <scsi/scsi_eh.h>
#else
#include <linux/blk.h>
#include "scsi.h"
#include "hosts.h"
#endif

#include "mvOs.h"
#include "mvSata.h"
#include "mvStorageDev.h"
#include "mvScsiAtaLayer.h"
#include "mvLinuxIalLib.h"
#include "mvIALCommon.h"

#include <linux/blkdev.h>
#include <linux/spinlock.h>
 
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
extern int mv_ial_ht_detect (Scsi_Host_Template *);
#else
typedef struct scsi_host_template Scsi_Host_Template;
#endif
extern int mv_ial_ht_release (struct Scsi_Host *);
extern int mv_ial_ht_queuecommand (struct scsi_cmnd *, void (*done) (struct scsi_cmnd *));
extern int mv_ial_ht_bus_reset (struct scsi_cmnd *);
extern int mv_ial_ht_abort(struct scsi_cmnd *SCpnt);

#define HOSTDATA(host) ((IAL_HOST_T *)&host->hostdata)
#define MV_IAL_ADAPTER(host) (HOSTDATA(host)->pAdapter)

#define TEMP_DATA_BUFFER_LENGTH		    512


#ifdef CONFIG_PCI_MSI

#endif

#ifndef MRVL_SATA_BUFF_BOUNDARY
#define MRVL_SATA_BUFF_BOUNDARY (1 << 24)
#endif  

#define MRVL_SATA_BOUNDARY_MASK (MRVL_SATA_BUFF_BOUNDARY - 1)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#if defined(MY_ABC_HERE) || defined(MY_ABC_HERE) || defined(SYNO_SATA_POWER_CTL)

#ifdef MY_ABC_HERE
extern struct class_device_attribute *mvSata_shost_attrs[];
#else
#define mvSata_shost_attrs NULL
#endif

#ifdef SYNO_SATA_POWER_CTL
#define SYNO_SHUTDOWN_PORT syno_host_power_ctl: syno_mvSata_port_power_ctl,
#else
#define SYNO_SHUTDOWN_PORT
#endif

#ifdef MY_ABC_HERE
#define SYNO_INDEX_GET syno_index_get: syno_mvSata_index_get,
#else
#define SYNO_INDEX_GET
#endif

#define SynoMvSata                                                          \
{                                                                           \
    module:     THIS_MODULE,\
    proc_name:          "mvSata",                         \
    proc_info:          mv_ial_ht_proc_info,        \
    slave_configure:    mv_ial_ht_slave_configure,\
    name:               "Marvell SCSI to SATA adapter",              \
    release:            mv_ial_ht_release,                     \
    queuecommand:       mv_ial_ht_queuecommand,           \
    bios_param:         NULL     ,      \
    eh_device_reset_handler: NULL ,                   \
    eh_bus_reset_handler: mv_ial_ht_bus_reset,                              \
    eh_abort_handler:   mv_ial_ht_abort,                                    \
    can_queue:          MV_SATA_SW_QUEUE_SIZE,                 \
    this_id:            MV_SATA_PM_MAX_PORTS,                              \
    sg_tablesize:       64,                                  \
    max_sectors:        256,                                                \
    cmd_per_lun:        MV_SATA_SW_QUEUE_SIZE,                 \
    unchecked_isa_dma:  0,                               \
    emulated:           1,                        \
    SYNO_INDEX_GET                                                          \
    SYNO_SHUTDOWN_PORT                                                      \
    shost_attrs:        mvSata_shost_attrs,                                 \
    use_clustering:     ENABLE_CLUSTERING                  \
}
#endif
#define mvSata                                                          \
{                                                                           \
    module:     THIS_MODULE,\
    proc_name:          "mvSata",                         \
    proc_info:          mv_ial_ht_proc_info,        \
    slave_configure:    mv_ial_ht_slave_configure,\
    name:               "Marvell SCSI to SATA adapter",              \
    release:            mv_ial_ht_release,                     \
    queuecommand:       mv_ial_ht_queuecommand,           \
    bios_param:         NULL     ,      \
    eh_device_reset_handler: NULL ,                   \
    eh_bus_reset_handler: mv_ial_ht_bus_reset,                              \
    eh_abort_handler:   mv_ial_ht_abort,                                    \
    can_queue:          MV_SATA_SW_QUEUE_SIZE,                 \
    this_id:            MV_SATA_PM_MAX_PORTS,                              \
    sg_tablesize:       64,                                  \
    max_sectors:        256,                                                \
    cmd_per_lun:        MV_SATA_SW_QUEUE_SIZE,                 \
    unchecked_isa_dma:  0,                               \
    emulated:           1,                        \
    use_clustering:     ENABLE_CLUSTERING                  \
}
#else
#define mvSata                                                          \
{                                                                           \
    proc_name:          "mvSata",                         \
    proc_info:          mv_ial_ht_proc_info24,      \
    select_queue_depths: NULL,              \
    name:               "Marvell SCSI to SATA adapter",              \
    detect:             mv_ial_ht_detect,                       \
    release:            mv_ial_ht_release,                     \
    command:            NULL,                                  \
    queuecommand:       mv_ial_ht_queuecommand,           \
    bios_param:         NULL     ,      \
    eh_device_reset_handler: NULL ,                   \
    eh_bus_reset_handler: mv_ial_ht_bus_reset,                              \
    eh_abort_handler:   mv_ial_ht_abort,                                    \
    can_queue:          MV_SATA_SW_QUEUE_SIZE,                               \
    this_id:            MV_SATA_PM_MAX_PORTS,                                  \
    sg_tablesize:       64,                                  \
    max_sectors:        256,                                                \
    cmd_per_lun:        MV_SATA_SW_QUEUE_SIZE,                 \
    unchecked_isa_dma:  0,                               \
    emulated:           1,                        \
    use_new_eh_code:    1,                                                  \
    highmem_io:         1,                            \
    use_clustering:     ENABLE_CLUSTERING                  \
}
#endif

#define MV_IAL_HT_SACOALT_DEFAULT   4
#define MV_IAL_HT_SAITMTH_DEFAULT   (150 * 50)

struct IALHost;

typedef struct IALAdapter
{
    MV_SATA_ADAPTER     mvSataAdapter;
    MV_U8               activeHosts;
    int                 maxHosts;
    struct IALHost      *host[MV_SATA_CHANNELS_NUM];
    struct pci_dev      *pcidev;
    u8                  rev_id;  
    u8                  *requestsArrayBaseAddr;
    u8                  *requestsArrayBaseAlignedAddr;
    dma_addr_t          requestsArrayBaseDmaAddr;
    dma_addr_t          requestsArrayBaseDmaAlignedAddr;
    u8                  *responsesArrayBaseAddr;
    u8                  *responsesArrayBaseAlignedAddr;
    dma_addr_t          responsesArrayBaseDmaAddr;
    dma_addr_t          responsesArrayBaseDmaAlignedAddr;
    u32                  requestQueueSize;
    u32                  responseQueueSize;
    u32                 procNumOfInterrupts;
    MV_IAL_COMMON_ADAPTER_EXTENSION ialCommonExt;
    MV_BOOLEAN          stopAsyncTimer;
    struct timer_list   asyncStartTimer;
    MV_SAL_ADAPTER_EXTENSION  *ataScsiAdapterExt;
    spinlock_t          adapter_lock;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    struct semaphore    rescan_mutex;
    atomic_t            stopped;
#endif
    MV_U16		tempDataBuffer[TEMP_DATA_BUFFER_LENGTH/2];
} IAL_ADAPTER_T;

typedef struct IALHost
{
    struct Scsi_Host* scsihost;
    MV_U8 channelIndex;
    IAL_ADAPTER_T* pAdapter;
    MV_EDMA_MODE mode;
    MV_SATA_SWITCHING_MODE switchingMode;
    MV_BOOLEAN  use128Entries;
    void  *prdPool[MV_SATA_GEN2E_SW_QUEUE_SIZE];
    void  *prdPoolAligned[MV_SATA_GEN2E_SW_QUEUE_SIZE];
    MV_U32  freePRDsNum;
    struct scsi_cmnd *scsi_cmnd_done_head, *scsi_cmnd_done_tail;
    MV_BOOLEAN  hostBlocked;
} IAL_HOST_T;

struct mv_comp_info
{
    struct scsi_cmnd           *SCpnt;
    MV_SATA_EDMA_PRD_ENTRY  *cpu_PRDpnt;
    dma_addr_t      dma_PRDpnt;
    dma_addr_t      single_buff_busaddr;
    unsigned int        allocated_entries;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    unsigned int        kmap_buffer;
#endif
    unsigned int        seq_number;
    MV_SATA_SCSI_CMD_BLOCK  *pSALBlock;
    struct scsi_cmnd           *next_done;
};

#define pci64_map_single(d,c,s,dir) pci_map_single((d),(c),(s),(dir))
#define pci64_map_sg(d,s,n,dir) pci_map_sg((d),(s),(n),(dir))
#define pci64_unmap_single(d,a,s,dir) pci_unmap_single((d),(a),(s),(dir))
#define pci64_unmap_sg(d,s,n,dir) pci_unmap_sg((d),(s),(n),(dir))

#if (BITS_PER_LONG > 32) || defined(CONFIG_HIGHMEM64G)
#define pci64_dma_hi32(a) ((u32) (0xffffffff & (((u64)(a))>>32)))
#define pci64_dma_lo32(a) ((u32) (0xffffffff & (((u64)(a)))))
#else
#define pci64_dma_hi32(a) 0
#define pci64_dma_lo32(a) (a)
#endif   
#define sg_dma64_address(s) sg_dma_address(s)
#define sg_dma64_len(s) sg_dma_len(s)

#endif  

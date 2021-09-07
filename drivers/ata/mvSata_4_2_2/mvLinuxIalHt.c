#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef LINUX_VERSION_CODE
    #include <linux/version.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
    #error "This driver works only with kernel 2.4.0 or higher!"
#endif

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)))
    #error "This driver does not support kernel 2.5!"
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/kdev_t.h>

#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
#include "ctrlEnv/mvCtrlEnvLib.h"
#include "ctrlEnv/sys/mvSysSata.h"
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <linux/blk.h>
#include "scsi.h"
#include "hosts.h"
#else
#include <scsi/scsi.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <scsi/scsi_host.h>
#include <scsi/scsi_tcq.h>
#endif

#include <linux/timer.h>
#include <linux/spinlock.h>
#include <asm/dma.h>
#include <asm/system.h>
#include <asm/io.h>

#include "mvLinuxIalHt.h"
#include "mvRegs.h"
#include "mvIALCommon.h"
#include "mvLinuxIalSmart.h"

extern Scsi_Host_Template driver_template;

static void mv_ial_init_log(void);

static char mv_ial_proc_version[]="Version_1_1";
extern void release_ata_mem(struct mv_comp_info * pInfo);
extern MV_BOOLEAN IALCompletion(struct mvSataAdapter *pSataAdapter,
                                MV_SATA_SCSI_CMD_BLOCK *pCmdBlock);

static struct pci_device_id mvSata_pci_table[] =
{
    {MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_5080, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_5081, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_5040, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_5041, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_6081, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_6041, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_6042, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {MV_SATA_VENDOR_ID, MV_SATA_DEVICE_ID_7042, PCI_ANY_ID, PCI_ANY_ID, 0, 0},
    {0,}
};

int          adapterId = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)

#ifndef __devexit_p
#define __devexit_p(x)  x
#endif
static void mv_ial_ht_select_queue_depths (struct Scsi_Host* pHost,
                                           struct scsi_device* pDevs);

static inline struct Scsi_Host *scsi_host_alloc(Scsi_Host_Template *t, size_t s)
{
    return scsi_register(t, s);
}
static inline void scsi_host_put(struct Scsi_Host *h)
{
    scsi_unregister(h);
}

#define scsi_scan_host(x...)
#define scsi_remove_host(x...)

#else

static int mv_ial_ht_slave_configure (struct scsi_device* pDevs);
static int __devinit  mv_ial_probe_device(struct pci_dev *pci_dev, const struct pci_device_id *ent);
static void __devexit mv_ial_remove_device(struct pci_dev *pci_dev);

MODULE_DEVICE_TABLE(pci, mvSata_pci_table);

static char mv_hot_plug_name[] = "mvSata";

static struct pci_driver mv_ial_pci_driver =
{
    .name       = mv_hot_plug_name,
    .id_table   = mvSata_pci_table,
    .probe      = mv_ial_probe_device,
    .remove     = __devexit_p(mv_ial_remove_device),
};

#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
static int __devinit mv_ial_init_soc_sata(void);
#endif
IAL_ADAPTER_T       *pSocAdapter = NULL;

#ifdef MY_ABC_HERE
struct workqueue_struct *mvSata_aux_wq;
#endif

static int __init mv_ial_init(void)
{
    mv_ial_init_log();
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "mvSata init.\n");
    driver_template.module = THIS_MODULE;

#ifdef MY_ABC_HERE
    mvSata_aux_wq = create_singlethread_workqueue("mvSata_aux");
    if (!mvSata_aux_wq) {
        printk("\n## Cannot create mvSata workqueue ##\n");
        destroy_workqueue(mvSata_aux_wq);
        return -ENOMEM;
    }    
#endif

#ifdef MY_ABC_HERE
    int ret = pci_register_driver(&mv_ial_pci_driver); 
#endif

#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
	if (MV_FALSE == mvCtrlPwrClckGet(SATA_UNIT_ID, 0)) 
	{
		printk("\nWarning Sata is Powered Off\n");
	}
	else
	{
        	printk("Integrated Sata device found\n");
        	mv_ial_init_soc_sata();
	}
#endif

#ifdef MY_ABC_HERE
    return ret; 
#else
    return (int)pci_register_driver(&mv_ial_pci_driver);
#endif
}

static void __exit mv_ial_exit(void)
{

#ifdef MY_ABC_HERE
    destroy_workqueue(mvSata_aux_wq);
#endif
#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
      mv_ial_remove_device(NULL);
#endif
    pci_unregister_driver(&mv_ial_pci_driver);
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "mvSata exit.\n");
}

module_init(mv_ial_init);
module_exit(mv_ial_exit);

#endif

static void mv_ial_init_log(void)
{
#ifdef MV_LOGGER
    char *szModules[] = {"Core Driver",
        "SAL",
        "Common IAL",
        "Linux IAL"
    };
#if defined (MV_LOG_DEBUG)
    mvLogRegisterModule(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_ENABLE_ALL,
                        szModules[MV_CORE_DRIVER_LOG_ID]);
    mvLogRegisterModule(MV_SAL_LOG_ID, MV_DEBUG_ENABLE_ALL,
                        szModules[MV_SAL_LOG_ID]);
    mvLogRegisterModule(MV_IAL_COMMON_LOG_ID, MV_DEBUG_ENABLE_ALL,
                        szModules[MV_IAL_COMMON_LOG_ID]);
    mvLogRegisterModule(MV_IAL_LOG_ID, MV_DEBUG_ENABLE_ALL,
                        szModules[MV_IAL_LOG_ID]);
#elif defined (MV_LOG_ERROR)
    mvLogRegisterModule(MV_CORE_DRIVER_LOG_ID, MV_DEBUG_FATAL_ERROR | MV_DEBUG_ERROR |
			MV_DEBUG_INFO,
                        szModules[MV_CORE_DRIVER_LOG_ID]);
    mvLogRegisterModule(MV_SAL_LOG_ID, MV_DEBUG_FATAL_ERROR | MV_DEBUG_ERROR |
			MV_DEBUG_INFO,
                        szModules[MV_SAL_LOG_ID]);
    mvLogRegisterModule(MV_IAL_COMMON_LOG_ID, MV_DEBUG_FATAL_ERROR | MV_DEBUG_ERROR |
			MV_DEBUG_INFO,
                        szModules[MV_IAL_COMMON_LOG_ID]);
    mvLogRegisterModule(MV_IAL_LOG_ID, MV_DEBUG_FATAL_ERROR | MV_DEBUG_ERROR | 
			MV_DEBUG_INFO,
                        szModules[MV_IAL_LOG_ID]);
#endif
#endif
}

static int set_device_regs(MV_SATA_ADAPTER *pMvSataAdapter,
                           struct pci_dev   *pcidev)
{
    pMvSataAdapter->intCoalThre[0]= MV_IAL_HT_SACOALT_DEFAULT;
    pMvSataAdapter->intCoalThre[1]= MV_IAL_HT_SACOALT_DEFAULT;
    pMvSataAdapter->intTimeThre[0] = MV_IAL_HT_SAITMTH_DEFAULT;
    pMvSataAdapter->intTimeThre[1] = MV_IAL_HT_SAITMTH_DEFAULT;
    pMvSataAdapter->pciCommand = MV_PCI_COMMAND_REG_DEFAULT;
    pMvSataAdapter->pciSerrMask = MV_PCI_SERR_MASK_REG_ENABLE_ALL;
    pMvSataAdapter->pciInterruptMask = MV_PCI_INTERRUPT_MASK_REG_ENABLE_ALL;
    pMvSataAdapter->mvSataEventNotify = mv_ial_lib_event_notify;

    return 0;
}

static int mv_ial_get_num_of_ports(const struct pci_device_id *id)
{
    switch(id->device)
    {
        case MV_SATA_DEVICE_ID_5080:
        case MV_SATA_DEVICE_ID_5081:
        case MV_SATA_DEVICE_ID_6081:
            return 8;
        case MV_SATA_DEVICE_ID_5040:
        case MV_SATA_DEVICE_ID_5041:
        case MV_SATA_DEVICE_ID_6041:
        case MV_SATA_DEVICE_ID_6042:
        case MV_SATA_DEVICE_ID_7042:
            return 4;
        default:
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_FATAL_ERROR,
                     "getMaxNumberOfPorts() Unknown device ID.\n");
            return 0;
    }
}

static void mv_ial_free_scsi_hosts(IAL_ADAPTER_T *pAdapter, MV_BOOLEAN freeAdapter)
{
    int i;
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if (pAdapter->host[i] != NULL)
        {
            mv_ial_lib_prd_destroy(pAdapter->host[i]);
            scsi_host_put(pAdapter->host[i]->scsihost);
            pAdapter->host[i] = NULL;
        }
    }
    pAdapter->activeHosts = 0;
    if (MV_TRUE == freeAdapter)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,
                     "[%d] freeing Adapter data structure.\n", pAdapter->mvSataAdapter.adapterId);
        kfree(pAdapter);
    }
}

#ifdef MY_ABC_HERE
 
void
channel_do_scsi_done(MV_VOID_PTR ptr,
                     struct mvSataAdapter *pSataAdapter,                     
                     MV_U8 channel)
{
    struct scsi_cmnd *cmnds_done_list = NULL;
    unsigned long flags_io_request_lock;
    IAL_ADAPTER_T *pAdapter = ptr;    

    if (!pSataAdapter ||
        !pAdapter ||
        !pAdapter->host[channel] ||
        !pAdapter->host[channel]->scsihost) {
        WARN_ON(1);
        return;
    }

    cmnds_done_list = mv_ial_lib_get_first_cmnd (pAdapter, channel);
    if (cmnds_done_list) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        spin_lock_irqsave(&io_request_lock, flags_io_request_lock);
#else
        spin_lock_irqsave(pAdapter->host[channel]->scsihost->host_lock, flags_io_request_lock);
#endif
        mv_ial_lib_do_done(cmnds_done_list);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        spin_unlock_irqrestore(&io_request_lock, flags_io_request_lock);
#else
        spin_unlock_irqrestore(pAdapter->host[channel]->scsihost->host_lock, flags_io_request_lock);
#endif
    }
}

void 
SynoInitChannelEH(MV_VOID_PTR *pIALAdapter, MV_SATA_ADAPTER *pMvSataAdapter)
{
    MV_U8 channelIndex;
    SYNO_EH *pEH;
    IAL_ADAPTER_T *pAdapter = (IAL_ADAPTER_T *)pIALAdapter;
    
    for (channelIndex = 0; channelIndex < pMvSataAdapter->numberOfChannels; channelIndex++)
    {
        pEH = &pMvSataAdapter->eh[channelIndex];
        pEH->flags = 0;
        pEH->channel = channelIndex;
        pEH->pSataAdapter = pMvSataAdapter;
        pEH->retry_count = 0;
        pEH->pataScsiAdapterExt = pAdapter->ataScsiAdapterExt;
        pEH->pIalExt = &pAdapter->ialCommonExt;
        INIT_DELAYED_WORK(&(pEH->work), SynoChannelErrorHandle);
    }
}
#endif

static int __devinit  mv_ial_probe_device(struct pci_dev *pcidev,
                                          const struct pci_device_id *id)
{

    MV_SATA_ADAPTER     *pMvSataAdapter;
    IAL_ADAPTER_T       *pAdapter;
    MV_U8                 i;

    pci_set_drvdata(pcidev, NULL);

    if (pci_enable_device(pcidev))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,
                 "pci_enable_device() failed\n");
        return -ENODEV;
    }

    pci_set_master(pcidev);
    if (0 == pci_set_dma_mask(pcidev, 0xffffffffffffffffULL))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,"using 64-bit DMA.\n");
    }
    else if (0 == pci_set_dma_mask(pcidev, 0xffffffffUL))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "using 32-bit DMA.\n");
    }
    else
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "DMA 32-bit not supported"
                 " in the system\n");
        pci_disable_device(pcidev);
        return -ENODEV;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if (pci_request_regions(pcidev, mv_hot_plug_name) != 0)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "pci_request_regions() failed\n");
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
#endif

    pAdapter = (IAL_ADAPTER_T*)kmalloc(sizeof(IAL_ADAPTER_T), GFP_ATOMIC);
    if (pAdapter == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "IAL Adapter allocation failed\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
    memset(pAdapter, 0, sizeof(IAL_ADAPTER_T));
    pAdapter->activeHosts = 0;
    pAdapter->maxHosts = mv_ial_get_num_of_ports(id);
    if (pAdapter->maxHosts == 0)
    {
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "mv_ial_get_num_of_ports() failed\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        struct Scsi_Host    *pshost = scsi_host_alloc(&driver_template, sizeof(IAL_HOST_T));
        if (pshost == NULL)
        {
            mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "Scsi_Host allocation failed\n");
 #if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
            pci_release_regions(pcidev);
 #endif
            pci_disable_device(pcidev);
            return -ENOMEM;
        }
        pAdapter->host[i] = HOSTDATA(pshost);
        memset(pAdapter->host[i], 0, sizeof(IAL_HOST_T));
        pAdapter->host[i]->scsihost = pshost;
        pAdapter->host[i]->pAdapter = pAdapter;
        pAdapter->host[i]->channelIndex = (MV_U8)i;
        pAdapter->activeHosts |= (1 << i);
    }
    pAdapter->pcidev = pcidev;
    pMvSataAdapter = &(pAdapter->mvSataAdapter);
    pMvSataAdapter->IALData = pAdapter;
    spin_lock_init (&pAdapter->adapter_lock);
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        pAdapter->host[i]->scsi_cmnd_done_head = NULL;
        pAdapter->host[i]->scsi_cmnd_done_tail = NULL;
    }
    
    pAdapter->host[0]->scsihost->base = pci_resource_start(pcidev, 0);
    for (i = 1; i < pAdapter->maxHosts; i++)
    {
        if (pAdapter->host[i] != NULL)
            pAdapter->host[i]->scsihost->base = pAdapter->host[0]->scsihost->base;
    }
    pMvSataAdapter->adapterIoBaseAddress =
        (MV_BUS_ADDR_T)ioremap(pAdapter->host[0]->scsihost->base,
                               pci_resource_len(pcidev, 0));
    if (!pMvSataAdapter->adapterIoBaseAddress)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "Failed to remap memory io spcae\n");
        
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
    else
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "io base address 0x%08lx\n",
                 (ulong)pMvSataAdapter->adapterIoBaseAddress);
    }
    
    pMvSataAdapter->adapterId = adapterId++;
     
    if (pci_read_config_byte(pcidev, PCI_REVISION_ID, &pAdapter->rev_id))
    {
        printk(KERN_WARNING "mvSata: Failed to get revision id.\n");
        iounmap(pMvSataAdapter->adapterIoBaseAddress);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
    pMvSataAdapter->pciConfigRevisionId = pAdapter->rev_id;
    pMvSataAdapter->pciConfigDeviceId = id->device;
    if (set_device_regs(pMvSataAdapter, pcidev))
    {
        iounmap(pMvSataAdapter->adapterIoBaseAddress);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
     
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    init_MUTEX(&pAdapter->rescan_mutex);
    atomic_set(&pAdapter->stopped, 1);
#endif

    if (mvSataInitAdapter(pMvSataAdapter) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: core failed to initialize the adapter\n",
                 pMvSataAdapter->adapterId);
        iounmap(pMvSataAdapter->adapterIoBaseAddress);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
    if (mv_ial_lib_allocate_edma_queues(pAdapter))
    {
        mvLogMsg(MV_IAL_LOG_ID,MV_DEBUG_ERROR,
                 "Failed to allocate memory for EDMA queues\n");
        iounmap(pMvSataAdapter->adapterIoBaseAddress);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }

    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if ((pAdapter->activeHosts & (1 << i)) == 0)
        {
            continue;
        }
        if (mv_ial_lib_prd_init(pAdapter->host[i]))
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,
                     "Failed to init PRD memory manager - host %d\n", i);
            iounmap(pMvSataAdapter->adapterIoBaseAddress);
            mv_ial_lib_free_edma_queues(pAdapter);
            mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
            pci_release_regions(pcidev);
#endif
            pci_disable_device(pcidev);
            return -ENOMEM;
        }
    }
    pAdapter->ataScsiAdapterExt = (MV_SAL_ADAPTER_EXTENSION*)kmalloc(sizeof(MV_SAL_ADAPTER_EXTENSION),
                                                                     GFP_ATOMIC);
    if (pAdapter->ataScsiAdapterExt == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,"[%d]: out of memory, failed to allocate MV_SAL_ADAPTER_EXTENSION\n",
                 pAdapter->mvSataAdapter.adapterId);
        iounmap(pMvSataAdapter->adapterIoBaseAddress);
        mv_ial_lib_free_edma_queues(pAdapter);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
    mvSataScsiInitAdapterExt(pAdapter->ataScsiAdapterExt,
                             pMvSataAdapter);
     
    pAdapter->ataScsiAdapterExt->UAMask = MV_BIT0;
     
#ifdef MV_SUPPORT_MSI
    {
    	int err;
	if ((err = pci_enable_msi(pcidev)))
	{
	    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: Unable to allocate MSI interrupt Error: %d\n",
		    pMvSataAdapter->adapterId, err);
	}
    }
#endif

    if (request_irq(pcidev->irq, mv_ial_lib_int_handler,
                    (IRQF_DISABLED | IRQF_SAMPLE_RANDOM | IRQF_SHARED), "mvSata",
                    pAdapter) < 0)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: unable to allocate IRQ for controler\n",
                 pMvSataAdapter->adapterId);
#ifdef MV_SUPPORT_MSI
	pci_disable_msi(pAdapter->pcidev);
#endif
        kfree(pAdapter->ataScsiAdapterExt);
        iounmap(pMvSataAdapter->adapterIoBaseAddress);
        mv_ial_lib_free_edma_queues(pAdapter);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if ((pAdapter->activeHosts & (1 << i)) == 0)
        {
            continue;
        }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        scsi_set_device(pAdapter->host[i]->scsihost, &pcidev->dev);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        scsi_set_pci_device(pAdapter->host[i]->scsihost, pcidev);
#endif
        pAdapter->host[i]->scsihost->irq = pcidev->irq;
         
        if (pMvSataAdapter->sataAdapterGeneration == MV_SATA_GEN_I)
        {
            pAdapter->host[i]->scsihost->max_id = 1;
        }
        else
        {
            pAdapter->host[i]->scsihost->max_id = MV_SATA_PM_MAX_PORTS;
        }
        pAdapter->host[i]->scsihost->max_lun = 1;
        pAdapter->host[i]->scsihost->max_channel = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        pAdapter->host[i]->scsihost->select_queue_depths = mv_ial_ht_select_queue_depths;
#endif
    }
    if (MV_FALSE == mvAdapterStartInitialization(pMvSataAdapter,
                                                 &pAdapter->ialCommonExt,
                                                 pAdapter->ataScsiAdapterExt))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: mvAdapterStartInitialization"
                 " Failed\n", pMvSataAdapter->adapterId);
        free_irq (pcidev->irq, pMvSataAdapter);
#ifdef MV_SUPPORT_MSI
	pci_disable_msi(pAdapter->pcidev);
#endif
        kfree(pAdapter->ataScsiAdapterExt);
        iounmap(pMvSataAdapter->adapterIoBaseAddress);
        mv_ial_lib_free_edma_queues(pAdapter);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        pci_release_regions(pcidev);
#endif
        pci_disable_device(pcidev);
        return -ENOMEM;
    }
    pci_set_drvdata(pcidev, pAdapter);
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if ((pAdapter->activeHosts & (1 << i)) == 0)
        {
            continue;
        }
        mv_ial_block_requests(pAdapter, i);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        if (scsi_add_host(pAdapter->host[i]->scsihost, &pcidev->dev) != 0)
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: scsi_add_host() failed.\n"
                     , pMvSataAdapter->adapterId);
            free_irq (pcidev->irq, pMvSataAdapter);
#ifdef MV_SUPPORT_MSI
	    pci_disable_msi(pAdapter->pcidev);
#endif
	    kfree(pAdapter->ataScsiAdapterExt);
            iounmap(pMvSataAdapter->adapterIoBaseAddress);
            mv_ial_lib_free_edma_queues(pAdapter);
            mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
            pci_release_regions(pcidev);
            pci_disable_device(pcidev);
            return -ENODEV;
        }
#endif
    }

    pAdapter->stopAsyncTimer = MV_FALSE;
    init_timer(&pAdapter->asyncStartTimer);
    pAdapter->asyncStartTimer.data = (unsigned long)pAdapter;
    pAdapter->asyncStartTimer.function = asyncStartTimerFunction;
    pAdapter->asyncStartTimer.expires = jiffies + MV_LINUX_ASYNC_TIMER_PERIOD;
    add_timer (&pAdapter->asyncStartTimer);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if ((pAdapter->activeHosts & (1 << i)) != 0)
        {
            scsi_scan_host(pAdapter->host[i]->scsihost);
        }
    }
     
    atomic_set(&pAdapter->stopped, 0);
#endif
    return 0;
}

#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
static int __devinit mv_ial_init_soc_sata(void)
{
    MV_SATA_ADAPTER     *pMvSataAdapter;
    IAL_ADAPTER_T       *pAdapter;
    MV_U8                 i;

    mvSataWinInit();
    
    pAdapter = (IAL_ADAPTER_T*)kmalloc(sizeof(IAL_ADAPTER_T), GFP_ATOMIC);
    if (pAdapter == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "IAL Adapter allocation failed\n");
        return -ENOMEM;
    }
    pSocAdapter = pAdapter;
    memset(pAdapter, 0, sizeof(IAL_ADAPTER_T));
    pAdapter->activeHosts = 0;

    if(MV_5182_DEV_ID == mvCtrlModelGet())
	pAdapter->maxHosts = MV_SATA_5182_PORT_NUM;
    else if(MV_5082_DEV_ID == mvCtrlModelGet())
	pAdapter->maxHosts = MV_SATA_5082_PORT_NUM;
    else if(MV_6082_DEV_ID == mvCtrlModelGet())
	pAdapter->maxHosts = MV_SATA_6082_PORT_NUM;
#ifdef MV88F6281
    else if(MV_6281_DEV_ID == mvCtrlModelGet())
	pAdapter->maxHosts = MV_SATA_6281_PORT_NUM;
    else if(MV_6192_DEV_ID == mvCtrlModelGet())
	pAdapter->maxHosts = MV_SATA_6192_PORT_NUM;
    else if(MV_6190_DEV_ID == mvCtrlModelGet())
        pAdapter->maxHosts = MV_SATA_6190_PORT_NUM;
#endif
    else if ((mvCtrlModelGet() == MV_78100_DEV_ID) || 
		(mvCtrlModelGet() == MV_78200_DEV_ID) || 
		(mvCtrlModelGet() == MV_78XX0_DEV_ID))
	pAdapter->maxHosts = MV_SATA_78XX0_PORT_NUM;

	for (i = 0; i < pAdapter->maxHosts; i++)
	{	
		if (MV_FALSE == mvCtrlPwrClckGet(SATA_UNIT_ID, (MV_U32)i))
		{
			printk("Warning: SATA %d is powered off\n", i);
			mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
			return -ENOMEM;
		}
	}
    
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        struct Scsi_Host    *pshost = scsi_host_alloc(&driver_template, sizeof(IAL_HOST_T));
        if (pshost == NULL)
        {
            mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "Scsi_Host allocation failed\n");
            return -ENOMEM;
        }
        pAdapter->host[i] = HOSTDATA(pshost);
        memset(pAdapter->host[i], 0, sizeof(IAL_HOST_T));
        pAdapter->host[i]->scsihost = pshost;
        pAdapter->host[i]->pAdapter = pAdapter;
        pAdapter->host[i]->channelIndex = (MV_U8)i;
        pAdapter->activeHosts |= (1 << i);
    }
    pAdapter->pcidev = NULL;
    pMvSataAdapter = &(pAdapter->mvSataAdapter);
    pMvSataAdapter->IALData = pAdapter;
    spin_lock_init (&pAdapter->adapter_lock);
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        pAdapter->host[i]->scsi_cmnd_done_head = NULL;
        pAdapter->host[i]->scsi_cmnd_done_tail = NULL;
    }
    
    pAdapter->host[0]->scsihost->base = 0 ;
    for (i = 1; i < pAdapter->maxHosts; i++)
    {
        if (pAdapter->host[i] != NULL)
            pAdapter->host[i]->scsihost->base = pAdapter->host[0]->scsihost->base;
    }
    pMvSataAdapter->adapterIoBaseAddress = (MV_BUS_ADDR_T)(INTER_REGS_BASE + SATA_REG_BASE - 
                                            0x20000);
    
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "io base address 0x%08lx\n",
             (ulong)pMvSataAdapter->adapterIoBaseAddress);
    
    pMvSataAdapter->adapterId = adapterId++;
     
    pMvSataAdapter->pciConfigRevisionId = 0;
    pMvSataAdapter->pciConfigDeviceId = mvCtrlModelGet();
    if (set_device_regs(pMvSataAdapter, NULL))
    {
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
        return -ENOMEM;
    }
     
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    init_MUTEX(&pAdapter->rescan_mutex);
    atomic_set(&pAdapter->stopped, 1);
#endif

    if (mvSataInitAdapter(pMvSataAdapter) == MV_FALSE)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,
                 "[%d]: core failed to initialize the adapter\n",
                 pMvSataAdapter->adapterId);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
        return -ENOMEM;
    }
    if (mv_ial_lib_allocate_edma_queues(pAdapter))
    {
        mvLogMsg(MV_IAL_LOG_ID,MV_DEBUG_ERROR,
                 "Failed to allocate memory for EDMA queues\n");

        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
        return -ENOMEM;
    }

    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if ((pAdapter->activeHosts & (1 << i)) == 0)
        {
            continue;
        }
        if (mv_ial_lib_prd_init(pAdapter->host[i]))
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,
                     "Failed to init PRD memory manager - host %d\n", i);
            mv_ial_lib_free_edma_queues(pAdapter);
            mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
            return -ENOMEM;
        }
    }
    pAdapter->ataScsiAdapterExt = (MV_SAL_ADAPTER_EXTENSION*)kmalloc(sizeof(MV_SAL_ADAPTER_EXTENSION),
                                                                     GFP_ATOMIC);
    if (pAdapter->ataScsiAdapterExt == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG_ERROR,"[%d]: out of memory, failed to allocate MV_SAL_ADAPTER_EXTENSION\n",
                 pAdapter->mvSataAdapter.adapterId);
        mv_ial_lib_free_edma_queues(pAdapter);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
        return -ENOMEM;
    }
    mvSataScsiInitAdapterExt(pAdapter->ataScsiAdapterExt,
                             pMvSataAdapter);
     
    pAdapter->ataScsiAdapterExt->UAMask = MV_BIT0;
     
    if (request_irq(SATA_IRQ_NUM, mv_ial_lib_int_handler,
                    (IRQF_DISABLED | IRQF_SAMPLE_RANDOM | IRQF_SHARED), "mvSata",
                    pAdapter) < 0)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: unable to allocate IRQ for controler\n",
                 pMvSataAdapter->adapterId);
        kfree(pAdapter->ataScsiAdapterExt);
        mv_ial_lib_free_edma_queues(pAdapter);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
        return -ENOMEM;
    }
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if ((pAdapter->activeHosts & (1 << i)) == 0)
        {
            continue;
        }
        pAdapter->host[i]->scsihost->irq = SATA_IRQ_NUM;
        pAdapter->host[i]->scsihost->max_id = MV_SATA_PM_MAX_PORTS;
        
        pAdapter->host[i]->scsihost->max_lun = 1;
        pAdapter->host[i]->scsihost->max_channel = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        pAdapter->host[i]->scsihost->select_queue_depths = mv_ial_ht_select_queue_depths;
#endif
    }
    if (MV_FALSE == mvAdapterStartInitialization(pMvSataAdapter,
                                                 &pAdapter->ialCommonExt,
                                                 pAdapter->ataScsiAdapterExt))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: mvAdapterStartInitialization"
                 " Failed\n", pMvSataAdapter->adapterId);
        free_irq (SATA_IRQ_NUM, pMvSataAdapter);
        kfree(pAdapter->ataScsiAdapterExt);
        mv_ial_lib_free_edma_queues(pAdapter);
        mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
        return -ENOMEM;
    }

    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if ((pAdapter->activeHosts & (1 << i)) == 0)
        {
            continue;
        }
        mv_ial_block_requests(pAdapter, i);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        if (scsi_add_host(pAdapter->host[i]->scsihost, NULL) != 0)
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d]: scsi_add_host() failed.\n"
                     , pMvSataAdapter->adapterId);
            free_irq (SATA_IRQ_NUM , pMvSataAdapter);
            kfree(pAdapter->ataScsiAdapterExt);
            mv_ial_lib_free_edma_queues(pAdapter);
            mv_ial_free_scsi_hosts(pAdapter, MV_TRUE);
            return -ENODEV;
        }
#endif
    }

    pAdapter->stopAsyncTimer = MV_FALSE;
    init_timer(&pAdapter->asyncStartTimer);
    pAdapter->asyncStartTimer.data = (unsigned long)pAdapter;
    pAdapter->asyncStartTimer.function = asyncStartTimerFunction;
    pAdapter->asyncStartTimer.expires = jiffies + MV_LINUX_ASYNC_TIMER_PERIOD;
    add_timer (&pAdapter->asyncStartTimer);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    for (i = 0; i < pAdapter->maxHosts; i++)
    {
        if ((pAdapter->activeHosts & (1 << i)) != 0)
        {
            scsi_scan_host(pAdapter->host[i]->scsihost);
        }
    }
     
    atomic_set(&pAdapter->stopped, 0);
#endif
    return 0;

}
#endif 

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
 
int mv_ial_ht_detect (Scsi_Host_Template *tpnt)
{
    int                 num_hosts=0;
    struct pci_dev      *pcidev = NULL;
    int                 index;
    struct pci_device_id *id = &mvSata_pci_table[0];

    mv_ial_init_log();
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    if (!pci_present())
    {
        printk ("mvSata: PCI BIOS not present\n");
        return 0;
    }
#endif

    if (sizeof(struct mv_comp_info) > sizeof(Scsi_Pointer))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "WARNING mv_comp_info must be "
                 "re-defined - its too big");
        return -1;
    }
    index = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_unlock_irq (&io_request_lock);
#endif

    while (1)
    {
        if (id[index].device == 0)
        {
            break;
        }
        pcidev = NULL;

        while ((pcidev = pci_find_device (MV_SATA_VENDOR_ID,
                                          id[index].device, pcidev)) != NULL)
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "PCI device found, DeviceId 0x%x "
                     "BAR0=%lx\n",
                      id[index].device, pci_resource_start(pcidev,0));
            if (mv_ial_probe_device(pcidev, &id[index]) == 0)
            {
                IAL_ADAPTER_T *pAdapter = pci_get_drvdata(pcidev);
                num_hosts += pAdapter->maxHosts;
            }
        }
        index ++;
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_lock_irq (&io_request_lock);
#endif
    return num_hosts;
}
#endif

int mv_ial_ht_release (struct Scsi_Host *pHost)
{
    IAL_ADAPTER_T *pAdapter = MV_IAL_ADAPTER (pHost);
    MV_U8 channel;
    MV_SATA_ADAPTER * pMvSataAdapter = &pAdapter->mvSataAdapter;
    unsigned long lock_flags;
    struct scsi_cmnd *cmnds_done_list = NULL;
    IAL_HOST_T          *ial_host = HOSTDATA(pHost);

    channel = ial_host->channelIndex;
    pAdapter->activeHosts &= ~ (1 << channel);
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, ": release host %d\n", pHost->host_no);
    spin_lock_irqsave (&pAdapter->adapter_lock, lock_flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    if (pAdapter->stopAsyncTimer != MV_TRUE)
    {
         
        pAdapter->stopAsyncTimer = MV_TRUE;
        del_timer_sync(&pAdapter->asyncStartTimer);
    }
#endif

    if (pMvSataAdapter->sataChannel[channel])
    {
        mvSataDisableChannelDma(pMvSataAdapter, channel);

        mvSataFlushDmaQueue(pMvSataAdapter, channel,
                            MV_FLUSH_TYPE_CALLBACK);
        mv_ial_lib_free_channel(pAdapter, channel);
   }
      
    cmnds_done_list = mv_ial_lib_get_first_cmnd (pAdapter, channel);
    if (cmnds_done_list)
    {
        unsigned long flags_io_request_lock;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        spin_lock_irqsave(&io_request_lock, flags_io_request_lock);
#else
        spin_lock_irqsave(ial_host->scsihost->host_lock, flags_io_request_lock);
#endif
        mv_ial_lib_do_done(cmnds_done_list);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        spin_unlock_irqrestore(&io_request_lock, flags_io_request_lock);
#else
        spin_unlock_irqrestore(ial_host->scsihost->host_lock, flags_io_request_lock);
#endif
    }
    if (0 == pAdapter->activeHosts)
    {
      mvSataShutdownAdapter(pMvSataAdapter);
    }
    pAdapter->host[channel] = NULL;
    mv_ial_lib_prd_destroy(ial_host);
    spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);
    scsi_remove_host(pHost);
    scsi_host_put(pHost);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    if (0 == pAdapter->activeHosts)
    {
        struct pci_dev *dev = pAdapter->pcidev;
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,
                     "[%d] freeing Adapter resources.\n", pAdapter->mvSataAdapter.adapterId);
        free_irq (pAdapter->pcidev->irq, pMvSataAdapter);
#ifdef MV_SUPPORT_MSI
	pci_disable_msi(pAdapter->pcidev);
#endif
        kfree(pAdapter->ataScsiAdapterExt);
        iounmap(pMvSataAdapter->adapterIoBaseAddress);
        mv_ial_lib_free_edma_queues(pAdapter);
        kfree(pAdapter);
	pci_disable_device(dev);
    }
#endif
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
static void __devexit mv_ial_remove_device(struct pci_dev *pdev)
{
    IAL_ADAPTER_T       *pAdapter = (pdev != NULL) ? pci_get_drvdata(pdev) : pSocAdapter;
    int numhosts;
    int i;
    unsigned long lock_flags;

    if (pAdapter == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_FATAL_ERROR,
                     "mv_ial_remove_device() No valid Adapter IAL structure found.\n");
        return;
    }

    numhosts           = pAdapter->maxHosts;
     
    atomic_inc(&pAdapter->stopped);
    flush_scheduled_work();
     
    spin_lock_irqsave (&pAdapter->adapter_lock, lock_flags);
    pAdapter->stopAsyncTimer = MV_TRUE;
    del_timer_sync(&pAdapter->asyncStartTimer);
    spin_unlock_irqrestore(&pAdapter->adapter_lock, lock_flags);

    for (i = 0; i < numhosts; i++)
    {
        if (pAdapter->host[i] != NULL)
        {
            mv_ial_ht_release (pAdapter->host[i]->scsihost);
        }
    }
    if (pdev != NULL)  
    {
        free_irq (pAdapter->pcidev->irq, &pAdapter->mvSataAdapter);
#ifdef MV_SUPPORT_MSI
	pci_disable_msi(pAdapter->pcidev);
#endif
	kfree(pAdapter->ataScsiAdapterExt);
	iounmap(pAdapter->mvSataAdapter.adapterIoBaseAddress);
	mv_ial_lib_free_edma_queues(pAdapter);
	kfree(pAdapter);
	pci_release_regions(pdev);
	pci_disable_device(pdev);
    }
#ifdef CONFIG_MV_INCLUDE_INTEG_SATA
    else  
    {
        free_irq (SATA_IRQ_NUM, &pAdapter->mvSataAdapter);
	kfree(pAdapter->ataScsiAdapterExt);
	mv_ial_lib_free_edma_queues(pAdapter);
	kfree(pAdapter);
    }
#endif
}
#endif

int mv_ial_ht_queuecommand (struct scsi_cmnd * SCpnt, void (*done) (struct scsi_cmnd *))
{
    IAL_ADAPTER_T   *pAdapter = MV_IAL_ADAPTER(SCpnt->device->host);
    MV_SATA_ADAPTER *pMvSataAdapter;
    IAL_HOST_T      *pHost = HOSTDATA(SCpnt->device->host);
    MV_U8            channel = pHost->channelIndex;
    int             build_prd_table = 0;
    unchar *cmd = (unchar *) SCpnt->cmnd;
    struct mv_comp_info *completion_info;
    unsigned long lock_flags;

    struct scsi_cmnd   *cmnds_done_list = NULL;

    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, " :queuecommand host=%d, bus=%d, channel=%d\n",
             SCpnt->device->host->host_no,
             SCpnt->device->channel,
             channel);
    if (done == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, ": in queuecommand, done function can't be NULL\n");
        return 0;
    }

    if ((pAdapter == NULL) || (channel >= MV_SATA_CHANNELS_NUM)||
        (pAdapter->host[channel] == NULL))
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_FATAL_ERROR,": in queuecommand, "
                 "command queued for released host!!\n");
        SCpnt->result = DID_NO_CONNECT << 16;
        done(SCpnt);
        return 0;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_unlock_irq (&io_request_lock);
#else
    spin_unlock_irq(pHost->scsihost->host_lock);
#endif

    spin_lock_irqsave (&pAdapter->adapter_lock, lock_flags);

    if (SCpnt->retries > 0)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,": retry command host=%d, bus=%d"
                 " SCpnt = %p\n", SCpnt->device->host->host_no, channel, SCpnt);
    }

    if (MV_TRUE == pAdapter->host[channel]->hostBlocked)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,": command received for "
                 "blocked host=%d, bus=%d, channel=%d, SCpnt = %p\n",
                 SCpnt->device->host->host_no,
                 SCpnt->device->channel,
                 channel, SCpnt);
#if 0
        spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        spin_lock_irq (&io_request_lock);
#else
        spin_lock_irq(pHost->scsihost->host_lock);
#endif
        return SCSI_MLQUEUE_HOST_BUSY;
#endif
    }

    pMvSataAdapter = &pAdapter->mvSataAdapter;

    SCpnt->result = DID_ERROR << 16;
    SCpnt->scsi_done = done;

    completion_info = ( struct mv_comp_info *) &(SCpnt->SCp);
    completion_info->pSALBlock =
    (MV_SATA_SCSI_CMD_BLOCK *) kmalloc(sizeof(MV_SATA_SCSI_CMD_BLOCK),
                                       GFP_ATOMIC);
    if (completion_info->pSALBlock == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,  "in queuecommand: Failed to allocate SAL Block\n");
        spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);
		done(SCpnt);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        spin_lock_irq (&io_request_lock);
#else
        spin_lock_irq(pHost->scsihost->host_lock);
#endif
        return -1;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    completion_info->kmap_buffer = 0;
#endif
     
    if ((*cmd == READ_6) || (*cmd == READ_10) || (*cmd == READ_16) ||
	(*cmd == WRITE_6) || (*cmd == WRITE_10) || (*cmd == WRITE_16))
    {
        build_prd_table = 1;
    }
#ifdef MY_ABC_HERE
    else if((pAdapter->ataScsiAdapterExt->ataDriveData[channel][SCpnt->device->id].identifyInfo.deviceType == MV_SATA_DEVICE_TYPE_ATAPI_DEVICE) && (SCpnt->sdb.table.nents))
#else
    else if((pAdapter->ataScsiAdapterExt->ataDriveData[channel][SCpnt->device->id].identifyInfo.deviceType == MV_SATA_DEVICE_TYPE_ATAPI_DEVICE) && (SCpnt->use_sg))
#endif
    {
	  
	 if(!(pAdapter->mvSataAdapter.sataAdapterGeneration == MV_SATA_GEN_II)){
	      mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,
		       "in queuecommand: PRD for non data command for ATAPI device\n");
	      build_prd_table = 1;
	 }
    }
     if((pAdapter->ataScsiAdapterExt->ataDriveData[channel][SCpnt->device->id].identifyInfo.deviceType == MV_SATA_DEVICE_TYPE_ATAPI_DEVICE))
    {
        BUG_ON(((unsigned int)SCpnt->cmnd) & 0x1);
    }
     
    if(build_prd_table)
    {
        if(pAdapter->ataScsiAdapterExt->ataDriveData[channel][SCpnt->device->id].identifyInfo.deviceType == MV_SATA_DEVICE_TYPE_ATAPI_DEVICE)
        {
           mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "in queuecommand: Data command for ATAPI device\n");
        }
        else
        {
              mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "in queuecommand: Data command for ATA device\n");
 
        }
        if (mv_ial_lib_generate_prd(pMvSataAdapter, SCpnt, completion_info))
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "in queuecommand: illegal requested buffer\n");
            spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);
	   		done(SCpnt);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
            spin_lock_irq (&io_request_lock);
#else
            spin_lock_irq(pHost->scsihost->host_lock);
#endif
            return -1;
        }
        completion_info->pSALBlock->pDataBuffer = NULL;
    }
    else
    {
#ifdef MY_ABC_HERE
        completion_info->pSALBlock->pDataBuffer = SCpnt->sdb.table.sgl;
#else
        completion_info->pSALBlock->pDataBuffer = SCpnt->request_buffer;
#endif
        completion_info->cpu_PRDpnt = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#ifdef MY_ABC_HERE
	if (SCpnt->sdb.table.nents)
#else
	if (SCpnt->use_sg)
#endif
	{
       	    completion_info->kmap_buffer  = 1;
	}
#endif
    }
    completion_info->SCpnt = SCpnt;

    completion_info->pSALBlock->bus = channel;

    completion_info->pSALBlock->target = SCpnt->device->id;
    completion_info->pSALBlock->lun = SCpnt->device->lun;
    completion_info->pSALBlock->pSalAdapterExtension = pAdapter->ataScsiAdapterExt;
    completion_info->pSALBlock->pIalAdapterExtension = &pAdapter->ialCommonExt;
    completion_info->pSALBlock->completionCallBack = IALCompletion;
    completion_info->pSALBlock->IALData = SCpnt;
#ifdef MY_ABC_HERE
    completion_info->pSALBlock->dataBufferLength = SCpnt->sdb.length;
#else
    completion_info->pSALBlock->dataBufferLength = SCpnt->request_bufflen;
#endif
    completion_info->pSALBlock->pSenseBuffer = SCpnt->sense_buffer;
    completion_info->pSALBlock->ScsiCdb = SCpnt->cmnd;
    completion_info->pSALBlock->ScsiCdbLength = SCpnt->cmd_len;
    completion_info->pSALBlock->senseBufferLength = SCSI_SENSE_BUFFERSIZE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if (completion_info->kmap_buffer)
    {
	struct scatterlist *sg;
#ifdef MY_ABC_HERE
	sg = (struct scatterlist *) SCpnt->sdb.table.sgl;
#else
	sg = (struct scatterlist *) SCpnt->request_buffer;
#endif
	mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "SCpnt %p, cmd %x need to use"
		" temp data buffer.lengh %d \n", SCpnt, *cmd ,sg->length);
	completion_info->pSALBlock->pDataBuffer = kmalloc(sg->length,GFP_ATOMIC);
	if (completion_info->pSALBlock->pDataBuffer == NULL)
    	{
        	mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR,  "in queuecommand: Failed to allocate temp buffer for kmap\n");
	        spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);
		done(SCpnt);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	        spin_lock_irq (&io_request_lock);
#else
        	spin_lock_irq(pHost->scsihost->host_lock);
#endif
	        return -1;
    	}
        completion_info->pSALBlock->dataBufferLength = sg->length;
	if( SCpnt->sc_data_direction == DMA_TO_DEVICE) 
	{
		struct scatterlist *sg;
	        MV_U8*          pBuffer;
#ifdef MY_ABC_HERE
			sg = (struct scatterlist *) SCpnt->sdb.table.sgl;
#else
			sg = (struct scatterlist *) SCpnt->request_buffer;
#endif

		mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "SCpnt %p, cmd %x kmap temp data buffer and copy data.lengh %d \n", SCpnt, *cmd ,sg->length);
#ifdef MY_ABC_HERE
	        pBuffer = kmap_atomic(sg_page(sg), KM_USER0) + sg->offset;
#else
	        pBuffer = kmap_atomic(sg->page, KM_USER0) + sg->offset;
#endif
	        memcpy(completion_info->pSALBlock->pDataBuffer, pBuffer , sg->length);
	        kunmap_atomic(pBuffer - sg->offset, KM_USER0);
	}

    }
#endif
    switch(SCpnt->sc_data_direction)
    {
        case DMA_FROM_DEVICE:
            completion_info->pSALBlock->dataDirection = MV_SCSI_COMMAND_DATA_DIRECTION_IN;
            break;
        case DMA_TO_DEVICE:
            completion_info->pSALBlock->dataDirection = MV_SCSI_COMMAND_DATA_DIRECTION_OUT;
            break;
        default:
           completion_info->pSALBlock->dataDirection = MV_SCSI_COMMAND_DATA_DIRECTION_NON;
    }

    if (*cmd != SCSI_OPCODE_MVSATA_SMART)
    {
        mvExecuteScsiCommand(completion_info->pSALBlock, MV_TRUE);
    }
    else
    {
        mvScsiAtaSendSmartCommand(pMvSataAdapter, completion_info->pSALBlock);
    }

    cmnds_done_list = mv_ial_lib_get_first_cmnd(pAdapter, channel);
    spin_unlock_irqrestore(&pAdapter->adapter_lock, lock_flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_lock_irq (&io_request_lock);
#else
    spin_lock_irq(pHost->scsihost->host_lock);
#endif
    if (cmnds_done_list)
    {
        mv_ial_lib_do_done(cmnds_done_list);
    }
    return 0;
}
 
int mv_ial_ht_bus_reset (struct scsi_cmnd *SCpnt)
{
    IAL_ADAPTER_T   *pAdapter = MV_IAL_ADAPTER(SCpnt->device->host);
    MV_SATA_ADAPTER *pMvSataAdapter = &pAdapter->mvSataAdapter;
    IAL_HOST_T      *pHost = HOSTDATA(SCpnt->device->host);
    MV_U8 channel = pHost->channelIndex;

    unsigned long lock_flags;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_unlock_irq (&io_request_lock);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
    spin_unlock_irq(pHost->scsihost->host_lock);
#endif
    spin_lock_irqsave (&pAdapter->adapter_lock, lock_flags);
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "Bus Reset: host=%d, channel=%d, target=%d\n",
             SCpnt->device->host->host_no, SCpnt->device->channel, SCpnt->device->id);
    if (pMvSataAdapter->sataChannel[channel] == NULL)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "trying to reset disabled channel, host=%d, channel=%d\n",
                 SCpnt->device->host->host_no, channel);
        spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
        spin_lock_irq(&io_request_lock);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
        spin_lock_irq(pHost->scsihost->host_lock);
#endif
        return FAILED;
    }

    mvSataDisableChannelDma(pMvSataAdapter, channel);

    mvSataFlushDmaQueue (pMvSataAdapter, channel, MV_FLUSH_TYPE_CALLBACK);

    mvSataChannelHardReset(pMvSataAdapter, channel);

    if (pMvSataAdapter->sataChannel[channel])
    {
        mvRestartChannel(&pAdapter->ialCommonExt, channel,
                         pAdapter->ataScsiAdapterExt, MV_TRUE);
        mv_ial_block_requests(pAdapter, channel);
    }
     
#ifdef MY_ABC_HERE
    syno_ial_lib_clear_cmnd(pAdapter, channel);
#else
    mv_ial_lib_get_first_cmnd(pAdapter, channel);
#endif
    spin_unlock_irqrestore(&pAdapter->adapter_lock, lock_flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_lock_irq(&io_request_lock);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
    spin_lock_irq(pHost->scsihost->host_lock);
#endif
    return SUCCESS;
}

static MV_VOID mvAta2HostString(IN MV_U16 *source,
                                OUT MV_U16 *target,
                                IN MV_U32 wordsCount
                               )
{
    MV_U32 i;
    for (i=0 ; i < wordsCount; i++)
    {
        target[i] = MV_LE16_TO_CPU(target[i]);
    }
}

int mv_ial_ht_proc_info(struct Scsi_Host *pshost,
                        char *buffer, char **start, off_t offset,
                        int length, int inout)
{
    int len = 0, temp, pmPort;
    IAL_ADAPTER_T       *pAdapter;
    MV_SATA_ADAPTER *pMvSataAdapter;
    IAL_HOST_T       *pHost = HOSTDATA(pshost);

    unsigned long lock_flags;

    pAdapter = MV_IAL_ADAPTER(pshost);
    pMvSataAdapter = &pAdapter->mvSataAdapter;
    temp = pHost->channelIndex;
    spin_lock_irqsave (&pAdapter->adapter_lock, lock_flags);
    if (inout == 1)
    {                      
         
        int i;
         
        if (!strncmp (buffer, "int_coal", strlen ("int_coal")))
        {
            int sata_unit;
            u32 time_thre, coal_thre;
            i = sscanf (buffer + strlen ("int_coal"), "%d %d %d\n",
                        &sata_unit, &coal_thre, &time_thre);
            if (i == 3)
            {         
                mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d]: Modifying interrupt coalescing of unit %d to %d threshold and %d timer\n",pMvSataAdapter->adapterId, sata_unit, coal_thre, time_thre);
                mvSataSetIntCoalParams (pMvSataAdapter, sata_unit, coal_thre, time_thre);
            }
            else
            {
                mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d]: Error in interrupt coalescing parameters\n",
                         pMvSataAdapter->adapterId);
            }
        }
         
        else if (!strncmp (buffer, "sata_phy_shutdown", strlen ("sata_phy_shutdown")))
        {
            int sata_phy;
            i = sscanf (buffer + strlen ("sata_phy_shutdown"), "%d\n", &sata_phy);
            if (i == 1)
            {         

                if (mvSataIsStorageDeviceConnected (pMvSataAdapter, sata_phy, NULL) == MV_TRUE)
                {
                    mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: Warning - shutting down a phy that is connected to a storage device\n", pMvSataAdapter->adapterId, sata_phy);
                }
                if (mvSataChannelPhyShutdown (pMvSataAdapter, sata_phy) == MV_TRUE)
                {
                    mvLogMsg(MV_IAL_LOG_ID,  MV_DEBUG, "[%d,%d]: Shutting down SATA phy\n", pMvSataAdapter->adapterId, sata_phy);
                }
            }
            else
            {
                mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d]: Error in shutting down SATA phy parameters\n",
                         pMvSataAdapter->adapterId);
            }
        }
        else if (!strncmp (buffer, "sata_phy_powerup", strlen ("sata_phy_powerup")))
        {
            int sata_phy;
            i = sscanf (buffer + strlen ("sata_phy_powerup"), "%d\n", &sata_phy);
            if (i == 1)
            {         
                if (mvSataChannelPhyPowerOn (pMvSataAdapter, sata_phy) == MV_TRUE)
                {
                    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d,%d]: Turning on SATA phy\n", pMvSataAdapter->adapterId, sata_phy);
                }
            }
            else
            {
                mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG,"[%d]: Error in powering up SATA phy parameters\n",
                         pMvSataAdapter->adapterId);
            }
        }
        spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);
        return length;
    }
    else
    {       
        int i;
         
        len += snprintf (buffer + len,length - len, "%s\n", mv_ial_proc_version);
        if (len >= length)
        {
            goto out;
        }
        len += snprintf (buffer + len,length - len, "\nTimeStamp :\n%ld\t%d\n",
                         jiffies, HZ);
        if (len >= length)
        {
            goto out;
        }
         
        len += snprintf (buffer + len,length - len, "\nNumber of interrupts generated by the adapter is : \n%d\n",
                         pAdapter->procNumOfInterrupts);
        if (len >= length)
        {
            goto out;
        }
        if (pAdapter->pcidev)
        {
            len += snprintf (buffer + len, length - len, "\nPCI location: Bus %d, Slot %d\n",
                             pAdapter->pcidev->bus->number,
                             PCI_SLOT(pAdapter->pcidev->devfn));
        
            if (len >= length)
            {
                goto out;
            }
            len += snprintf (buffer + len, length - len, "DeviceID: %x, Rev %x,"
                             " adapterId %d, channel %d \n",
                             pAdapter->mvSataAdapter.pciConfigDeviceId,
                             pAdapter->mvSataAdapter.pciConfigRevisionId,   
                             pAdapter->mvSataAdapter.adapterId,
                             pHost->channelIndex);

            if (len >= length)
            {
                goto out;
            }
        }
        else  
        {
            len += snprintf (buffer + len, length - len, "\nIntegrated Sata adapterId %d,  "
                             "channel %d\n",pAdapter->mvSataAdapter.adapterId,
                             pHost->channelIndex);
            
            if (len >= length)
            {
                goto out;
            }  
        }
        if (pMvSataAdapter->sataChannel[temp])
        {
            if (pMvSataAdapter->sataChannel[temp]->deviceType == MV_SATA_DEVICE_TYPE_PM)
            {
                len += snprintf (buffer + len, length - len, 
                                 "Port Multiplier connected, switching mode: %s\n",
                                 (pMvSataAdapter->sataChannel[temp]->FBSEnabled == MV_TRUE) ?
                                 "FBS":"CBS");

                if (len >= length)
                {
                    goto out;
                }
            }
        }
         
        len += snprintf (buffer + len,length - len,"\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t\t%s\t%s\n",
                         "Adapter", "Channel", "Id", "LUN", "TO", "TS", "Vendor",
                         "Mode", "LBA48");
        if (len >= length)
        {
            goto out;
        }
        if ((len + 100) >= length)
        {
            goto out;
        }
        for (i = 0 ; i < 80 ; i++)
            buffer [len + i] = '-';
        len += i;
        len += snprintf (buffer + len,length - len, "\n");
        if (len >= length)
        {
            goto out;
        }
        
        if (pMvSataAdapter->sataChannel[temp])
        {
            for (pmPort = 0; pmPort < MV_SATA_PM_MAX_PORTS; pmPort++)
            {
                if (pmPort > 0 &&
                    (pMvSataAdapter->sataChannel[temp]->deviceType != MV_SATA_DEVICE_TYPE_PM))
                {
                    break;
                }
                if (pAdapter->ataScsiAdapterExt->ataDriveData[temp][pmPort].driveReady == MV_FALSE)
                {
                    continue;
                }

                len += snprintf (buffer + len,length - len, "%d\t%d\t%d\t%d\t%u\t%u\t",
                                 pAdapter->mvSataAdapter.adapterId, temp, pmPort, 0,
                                 pAdapter->ataScsiAdapterExt->ataDriveData[temp][pmPort].stats.totalIOs,
                                 pAdapter->ataScsiAdapterExt->ataDriveData[temp][pmPort].stats.totalSectorsTransferred);
                if (len >= length)
                {
                    goto out;
                }
                 
                if ((len+10) >= length)
                {
                    goto out;
                }
                memcpy (buffer+len,
                        pAdapter->ataScsiAdapterExt->ataDriveData[temp][pmPort].identifyInfo.model, 10);
                mvAta2HostString((MV_U16 *)(buffer+len), (MV_U16 *)(buffer+len), 5);
                 
                for (i=0 ; i<10 ; i+=2)
                {
                    char ch = buffer[len + i];
                    buffer[len + i] = buffer[len+1 + i];
                    buffer[len+1 + i] = ch;
                    if (buffer[len + i] == ' ')
                    {
                        buffer[len + i + 1] = ' ';
                        break;
                    }
                    if (buffer[len+1 + i] == ' ')
                    {
                        break;
                    }
                }
                if ((len + 10) >= length)
                {
                    goto out;
                }
                for (; i < 10; i++)
                {
                    buffer[len + i] = ' ';
                }

                len += 10;
                len += snprintf (buffer + len,length - len, "\t%s \t%d\n",
                                 (pMvSataAdapter->sataChannel[temp]->queuedDMA == MV_EDMA_MODE_QUEUED) ?
                                  "TCQ" : (pMvSataAdapter->sataChannel[temp]->queuedDMA == MV_EDMA_MODE_NATIVE_QUEUING) ?
                                  "NCQ":"Normal",
                                 (pAdapter->ataScsiAdapterExt->ataDriveData[temp][pmPort].identifyInfo.LBA48Supported == MV_TRUE)  ? 1 : 0);
                if (len >= length)
                {
                    goto out;
                }
            }
        }
        if ((!pMvSataAdapter->sataChannel[temp]) &&
            (mvSataIsStorageDeviceConnected (pMvSataAdapter, temp, NULL) == MV_TRUE))
            len += snprintf (buffer + len,length - len, "Storage device connected to channel %d is malfunction\n", temp);
        if (len >= length)
        {
            goto out;
        }
        len += snprintf (buffer + len,length - len,"\n\n\nTO           - Total Outstanding commands accumulated\n");
        if (len >= length)
        {
            goto out;
        }
        len += snprintf (buffer + len,length - len,"TSA          - Total number of IOs accumulated\n");
        if (len >= length)
        {
            goto out;
        }
        len += snprintf (buffer + len,length - len,"TS           - Total number of sectors transferred (both read/write)\n");
        if (len >= length)
        {
            goto out;
        }
        len += snprintf (buffer + len,length - len,"Mode         - EDMA mode (TCQ|NCQ|Normal)\n");
        if (len >= length)
        {
            goto out;
        }
        len += snprintf (buffer + len,length - len,"LBA48        - Large Block Address 48 feature set enabled\n");
        if (len >= length)
        {
            goto out;
        }
    }
    out:
    spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);
    return(len);
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
 
int mv_ial_ht_proc_info24(char *buffer, char **start, off_t offset,
                        int length, int inode, int inout)
{
    struct Scsi_Host *pshost = 0;

    for (pshost = scsi_hostlist; pshost; pshost = pshost->next)
    {
        if (pshost->host_no == inode)
        {
            return mv_ial_ht_proc_info(pshost, buffer, start,
                                       offset,length, inout);
        }
    }
    return -EINVAL;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)

static int mv_ial_ht_slave_configure (struct scsi_device* pDevs)
{
    IAL_HOST_T *pHost = HOSTDATA (pDevs->host);
    struct Scsi_Host* scsiHost = pDevs->host;
    struct scsi_device*    pDevice = NULL;
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d]: slave configure\n",
                        pHost->pAdapter->mvSataAdapter.adapterId);

    if (pHost->use128Entries == MV_TRUE)
    {
        pHost->scsihost->can_queue = MV_SATA_GEN2E_SW_QUEUE_SIZE;
    }
    else
    {
        pHost->scsihost->can_queue = MV_SATA_SW_QUEUE_SIZE;
    }
    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d %d]: adjust host[channel] queue depth"
             " to %d\n", pHost->pAdapter->mvSataAdapter.adapterId, pHost->channelIndex,
                  pHost->scsihost->can_queue);
    shost_for_each_device(pDevice, scsiHost)
    {
        int deviceQDepth = 2;
        
        if(pHost->pAdapter->ataScsiAdapterExt->ataDriveData[pHost->channelIndex][pDevice->id].identifyInfo.deviceType == MV_SATA_DEVICE_TYPE_ATAPI_DEVICE)
        {
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d %d %d]: ATAPI device found\n", 
                  pHost->pAdapter->mvSataAdapter.adapterId, pHost->channelIndex,
                  pDevice->id);
            pDevice->use_10_for_rw = 1;
            pDevice->use_10_for_ms = 1;
            scsi_adjust_queue_depth(pDevice, 0, 1);
 
	    blk_queue_max_sectors(pDevice->request_queue, 256);
        }
        else
        {

            if (pHost->mode != MV_EDMA_MODE_NOT_QUEUED)
            {
                deviceQDepth = 31;
                if (pHost->scsihost->can_queue >= 32)
                {
                    deviceQDepth = 32;
                }
            }
            mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d %d %d]: adjust device queue "
                     "depth to %d\n", pHost->pAdapter->mvSataAdapter.adapterId,
                     pDevice->channel, pDevice->id, deviceQDepth);
            scsi_adjust_queue_depth(pDevice, MSG_SIMPLE_TAG, deviceQDepth);
#ifdef MV_SUPPORT_1MBYTE_IOS
            if(pHost->pAdapter->ataScsiAdapterExt->ataDriveData[pHost->channelIndex][pDevice->id].identifyInfo.LBA48Supported == MV_TRUE)
	    {
	        blk_queue_max_sectors(pDevice->request_queue, 2048);
 	        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d %d %d]: set device max sectors to 2048 \n",
	            pHost->pAdapter->mvSataAdapter.adapterId,
                    pHost->channelIndex, pDevice->id);
            }
#endif
        }
    }
    scsiHost->max_cmd_len = 16;    
    return 0;
}
#else
static void mv_ial_ht_select_queue_depths (struct Scsi_Host* pHost,
                                           struct scsi_device* pDevs)
{
    IAL_HOST_T *ial_host = HOSTDATA (pHost);
    struct scsi_device* pDevice;
    if (ial_host != NULL)
    {
        
        pHost->cmd_per_lun = 31;
        if (ial_host->mode != MV_EDMA_MODE_NOT_QUEUED)
        {   
            if (ial_host->use128Entries == MV_TRUE)
            {
                pHost->can_queue = MV_SATA_GEN2E_SW_QUEUE_SIZE;
                pHost->cmd_per_lun = 32;
            }
            else
            {
                pHost->can_queue = MV_SATA_SW_QUEUE_SIZE;
            }
        }
        else
        {
            pHost->can_queue = MV_DEFAULT_QUEUE_DEPTH;
        }
        
        for (pDevice = pDevs; pDevice; pDevice = pDevice->next)
        {
            if (pDevice->host == pHost)
            {
                pDevice->queue_depth = MV_SATA_SW_QUEUE_SIZE;
            }
        }
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG, "[%d %d]: adjust queue depth to %d\n",
            ial_host->pAdapter->mvSataAdapter.adapterId,
            ial_host->channelIndex,
            pHost->can_queue);
    }
}
#endif

int mv_ial_ht_abort(struct scsi_cmnd *SCpnt)
{
    IAL_ADAPTER_T   *pAdapter;
    IAL_HOST_T      *pHost;

    MV_SATA_ADAPTER *pMvSataAdapter;
    MV_U8           channel;
    unsigned long lock_flags;
    struct scsi_cmnd *cmnds_done_list = NULL;

    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "abort command %p\n", SCpnt);
    if (SCpnt == NULL)
    {
        return FAILED;
    }
    pHost = HOSTDATA(SCpnt->device->host);
    channel = pHost->channelIndex;
    pAdapter = MV_IAL_ADAPTER(SCpnt->device->host);
    pMvSataAdapter = &pAdapter->mvSataAdapter;
    
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    if (SCpnt->serial_number != SCpnt->serial_number_at_timeout)
    {
        mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d %d %d]: abort failed, "
                 "serial number mismatch\n",SCpnt->device->host->host_no,
                 channel, SCpnt->device->id);
        return FAILED;
    }
    spin_unlock_irq (&io_request_lock);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
    spin_unlock_irq (pHost->scsihost->host_lock);
#endif
    spin_lock_irqsave (&pAdapter->adapter_lock, lock_flags);

    mvRestartChannel(&pAdapter->ialCommonExt, channel,
                     pAdapter->ataScsiAdapterExt, MV_TRUE);
    mv_ial_block_requests(pAdapter, channel);

    cmnds_done_list = mv_ial_lib_get_first_cmnd(pAdapter, channel);

    spin_unlock_irqrestore (&pAdapter->adapter_lock, lock_flags);

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
    spin_lock_irq(&io_request_lock);
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
    spin_lock_irq (pHost->scsihost->host_lock);
#endif

    if (cmnds_done_list)
    {
        scsi_report_bus_reset(SCpnt->device->host, SCpnt->device->channel);
        mv_ial_lib_do_done(cmnds_done_list);
        return SUCCESS;
    }

    mvLogMsg(MV_IAL_LOG_ID, MV_DEBUG_ERROR, "[%d %d %d]: command abort failed\n",
             SCpnt->device->host->host_no, SCpnt->device->channel, SCpnt->device->id);
    return FAILED;
}

#ifdef MY_ABC_HERE
static int syno_mvSata_index_get(struct Scsi_Host *shost, uint channel, uint id, uint lun)
{
    int index = 0;    

#ifdef MY_ABC_HERE
    IAL_HOST_T *ial_host = HOSTDATA(shost);
    IAL_ADAPTER_T *pAdapter = MV_IAL_ADAPTER(shost);

    if (MV_TRUE == syno_mvSata_is_synology_pm(&pAdapter->ialCommonExt, ial_host->channelIndex))
        index = ((shost->host_no+1)*26) + id;  
    else 
#endif
         
        index = shost->host_no;

    return index;
}
#endif  

#ifdef SYNO_SATA_POWER_CTL
int
syno_mvSata_port_power_ctl(struct Scsi_Host *host, MV_U8 blPowerOn)
{
    IAL_HOST_T *ial_host = HOSTDATA(host);
    IAL_ADAPTER_T *pAdapter = MV_IAL_ADAPTER(host);
    MV_SATA_ADAPTER * pMvSataAdapter = &pAdapter->mvSataAdapter;
    MV_U8 channelIndex = ial_host->channelIndex;
    MV_SATA_CHANNEL *pSataChannel = pMvSataAdapter->sataChannel[channelIndex];
    int ret = 0;  

    if (NULL == pSataChannel) {
        goto END;
    }

    if (MV_SATA_DEVICE_TYPE_PM != pSataChannel->deviceType) {
         
        goto END;
    } else {
#ifdef MY_ABC_HERE
        if (!syno_mvSata_is_synology_pm(&pAdapter->ialCommonExt, ial_host->channelIndex)) {
            goto END;
        }

        syno_mvSata_pm_power_ctl(&pAdapter->ialCommonExt, ial_host->channelIndex, NULL, blPowerOn, MV_FALSE);
#endif
    }

END:
	return ret;
}
#endif  

#ifdef MY_ABC_HERE

static MV_U8 inline
defer_gpio_cmd(MV_CHANNEL_STATE stat, MV_U32 input, MV_U8 rw)
{
	MV_U8 ret = 0;

	if (WRITE == rw && 
		GPIO_3726_CMD_POWER_CLR == input) {
		 
		goto END;
	}

	if (CHANNEL_READY != stat) {
		ret = 1;
		goto END;
	}

END:
	return ret;
}

#ifdef MY_ABC_HERE
static ssize_t
syno_pm_gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
#else
static ssize_t
syno_pm_gpio_show(struct class_device *class_dev, char *buf)
#endif
{
#ifdef MY_ABC_HERE
    struct Scsi_Host *shost = class_to_shost(dev);
#else
    struct Scsi_Host *shost = class_to_shost(class_dev);
#endif
    IAL_HOST_T *ial_host = HOSTDATA(shost);
    IAL_ADAPTER_T *pAdapter = MV_IAL_ADAPTER(shost);
    ssize_t len = 0;

    if (MV_TRUE == syno_mvSata_is_synology_pm(&pAdapter->ialCommonExt, ial_host->channelIndex) &&
        !defer_gpio_cmd(pAdapter->ialCommonExt.channelState[ial_host->channelIndex], 0, READ)) {
        SYNO_PM_PKG pm_pkg;

        memset(&pm_pkg, 0, sizeof(pm_pkg));
        
        if (MV_TRUE != syno_mvSata_pmp_read_gpio(&pAdapter->ialCommonExt, ial_host->channelIndex, &pm_pkg)) {
            len = -EIO;
            sprintf(buf, "%s%s", "gpio=\"\"", "\n");
        }else {
            len = sprintf(buf, "gpio=\"0x%x\"%s", pm_pkg.var, "\n");
        }
    } else {
        len = sprintf(buf, "%s%s", "gpio=\"\"", "\n");
    }

    return len;
}

#ifdef MY_ABC_HERE
static ssize_t
syno_pm_gpio_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
#else
static ssize_t
syno_pm_gpio_store(struct class_device *class_dev, const char * buf, size_t count)
#endif
{
#ifdef MY_ABC_HERE
    struct Scsi_Host *shost = class_to_shost(dev);
#else
    struct Scsi_Host *shost = class_to_shost(class_dev);
#endif
    IAL_HOST_T *ial_host = HOSTDATA(shost);
    IAL_ADAPTER_T *pAdapter = MV_IAL_ADAPTER(shost);
    SYNO_PM_PKG pm_pkg;
     
    size_t ret = -EIO;

    memset(&pm_pkg, 0, sizeof(pm_pkg));
    sscanf(buf, "%x", &(pm_pkg.var));

    if (MV_TRUE == syno_mvSata_is_synology_pm(&pAdapter->ialCommonExt, ial_host->channelIndex) &&
        !defer_gpio_cmd(pAdapter->ialCommonExt.channelState[ial_host->channelIndex], pm_pkg.var, WRITE)) {
        ret = 
            (MV_TRUE == syno_mvSata_pmp_write_gpio(
                &pAdapter->ialCommonExt, 
                ial_host->channelIndex, &pm_pkg)) ? count : -EIO;
    }
    return ret;
}
#ifdef MY_ABC_HERE
static DEVICE_ATTR(syno_pm_gpio, S_IRUGO | S_IWUGO, syno_pm_gpio_show, syno_pm_gpio_store);
#else
static CLASS_DEVICE_ATTR(syno_pm_gpio, S_IRUGO | S_IWUGO, syno_pm_gpio_show, syno_pm_gpio_store);
#endif

#ifdef MY_ABC_HERE
static ssize_t
syno_pm_info_show(struct device *dev, struct device_attribute *attr, char *buf)
#else
static ssize_t
syno_pm_info_show(struct class_device *class_dev, char *buf)
#endif
{
#ifdef MY_ABC_HERE
    struct Scsi_Host *shost = class_to_shost(dev);
#else
    struct Scsi_Host *shost = class_to_shost(class_dev);
#endif
    IAL_HOST_T *ial_host = HOSTDATA(shost);
    IAL_ADAPTER_T *pAdapter = MV_IAL_ADAPTER(shost);
    ssize_t len = 0;
    int index, start_idx;
    int NumOfPMPorts = 0;

    if (MV_TRUE == syno_mvSata_is_synology_pm(&pAdapter->ialCommonExt, ial_host->channelIndex)) {
        char szTmp[BDEVNAME_SIZE];
        char szTmp1[PAGE_SIZE];

        NumOfPMPorts = syno_support_disk_num(pAdapter->mvSataAdapter.sataChannel[ial_host->channelIndex]->PMvendorId,
                                             pAdapter->mvSataAdapter.sataChannel[ial_host->channelIndex]->PMdeviceId,
                                             pAdapter->mvSataAdapter.sataChannel[ial_host->channelIndex]->PMSynoUnique);

        memset(szTmp, 0, sizeof(szTmp));
        memset(szTmp1, 0, sizeof(szTmp1));

        start_idx = syno_mvSata_index_get(shost, 0, 0, 0);
        for (index=0; index<NumOfPMPorts; index++ ) {
            DeviceNameGet(index+start_idx, szTmp);
            if (0 == index) {
                snprintf(szTmp1, PAGE_SIZE, "/dev/%s", szTmp);
            }else {				
                strcat(szTmp1, ",/dev/");
                strncat(szTmp1, szTmp, BDEVNAME_SIZE);
            }
        }
        snprintf(buf, PAGE_SIZE, "%s%s%s", "syno_device_list=\"", szTmp1, "\"\n");

        snprintf(szTmp, 
                 BDEVNAME_SIZE, 
                 "vendorid=%s0x%x%s", "\"",
                 pAdapter->mvSataAdapter.sataChannel[ial_host->channelIndex]->PMvendorId,
                 "\"\n");
        snprintf(szTmp1, PAGE_SIZE, "%s", szTmp);
        snprintf(szTmp, 
                 BDEVNAME_SIZE, 
                 "deviceid=%s%x%s", "\"",
                 pAdapter->mvSataAdapter.sataChannel[ial_host->channelIndex]->PMdeviceId,
                 "\"\n");
        strncat(szTmp1, szTmp, BDEVNAME_SIZE);

        snprintf(szTmp, 
                 BDEVNAME_SIZE, 
                 "error_handle=%s%s%s", "\"",
                 CHANNEL_READY != pAdapter->ialCommonExt.channelState[ial_host->channelIndex] ? "yes" : "no",
                 "\"\n");
        strncat(szTmp1, szTmp, BDEVNAME_SIZE);         

        len = snprintf(buf, PAGE_SIZE, "%s%s", buf, szTmp1);
    } else {
        len = snprintf(buf, PAGE_SIZE, "%s%s", "syno_device_list=\"\"", "\n");
    }

    return len;
}

#ifdef MY_ABC_HERE
static DEVICE_ATTR(syno_pm_info, S_IRUGO, syno_pm_info_show, NULL);
#else
static CLASS_DEVICE_ATTR(syno_pm_info, S_IRUGO, syno_pm_info_show, NULL);
#endif

struct class_device_attribute *mvSata_shost_attrs[] = {
#ifdef MY_ABC_HERE
    &dev_attr_syno_pm_gpio.attr,
    &dev_attr_syno_pm_info.attr,
#else
    &class_device_attr_syno_pm_gpio,
    &class_device_attr_syno_pm_info,
#endif
    NULL
};
#endif  

#if defined(MY_ABC_HERE) || defined(SYNO_SATA_POWER_CTL) || defined(MY_ABC_HERE)
Scsi_Host_Template driver_template = SynoMvSata;
#else
Scsi_Host_Template driver_template = mvSata;
#endif

MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Marvell Serial ATA PCI-X Adapter");
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include "scsi_module.c"
#endif

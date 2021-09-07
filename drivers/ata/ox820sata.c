 
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/sysdev.h>
#include <linux/module.h>
#include <linux/leds.h>
#include <linux/ata.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/raid/md_p.h>
#include <linux/raid/md_u.h>

#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>
#include <scsi/scsi_device.h>
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/desc_alloc.h>
#include <mach/memory.h>
#include <mach/ox820sata.h>

#define ERROR_INJECTION
#define CRAZY_DUMP_DEBUG

#undef LOCKING_DEBUG

#ifdef LOCKING_DEBUG
#define LPRINTK(fmt, args...) \
    printk(KERN_INFO "%d %s: " fmt, raw_smp_processor_id(), __FUNCTION__, ## args)
#else
#define LPRINTK(fmt, args...) {while(0);}
#endif

#if 0
    #if 0
        typedef struct {
            u32 a;
            u32 d;
            u32 w;
        } regaccess;
        static u32 regindex = 0;
        static regaccess regarray[1024];
        
        #define newcommand {regarray[regindex].w |= 2;}
    #endif

    #ifdef writel
    #undef writel
    #endif
    static inline void writel(u32 v,u32 a) {printk("[%08x]<=%08x\n",a,v);*((volatile u32*)(a)) = v;} 
     
    #ifdef readl
    #undef readl
    #endif
    static inline u32 myreadl(u32 a) {u32 v =(*((volatile u32*)(a))); printk("[%08x]=>%08x\n",a,v);return v;}
     
    #define readl(a) (myreadl(a))
#endif

#include <linux/libata.h>
#include "libata.h"
 
#define DRIVER_AUTHOR   "Oxford Semiconductor Ltd."
#define DRIVER_DESC     "934 SATA core controler"
#define DRIVER_NAME     "oxnassata"
 
MODULE_LICENSE("GPL");
MODULE_VERSION(1.0);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);

#if defined(CONFIG_ARCH_OXNAS_FPGA) && defined(CONFIG_ARCH_OX820)
#warning "Limiting SATA link to 1.5Gb/s"
#define LIMIT_TO_1pt5Gbs
#endif

typedef struct {
    volatile u32 qualifier;
    volatile u32 control;
    dma_addr_t src_pa;
    dma_addr_t dst_pa;
} __attribute ((aligned(4),packed)) sgdma_request_t;

typedef struct {
    struct kobject kobj;
    struct platform_driver driver;
    struct ata_port* ap[OX820SATA_MAX_PORTS];
#ifdef ERROR_INJECTION
    unsigned int error_period;
    unsigned long next_error;
#endif
    
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
     
    unsigned long port_frozen;
#endif    
    unsigned long port_in_eh;
} ox820sata_driver_t;

typedef struct
{
    u32* sgdma_controller;
    u32* dma_controller;
    sgdma_request_t* sgdma_request_va;
    dma_addr_t sgdma_request_pa;
    u32* reg_base;
} ox820sata_private_data;

static int  ox820sata_driver_probe(struct platform_device *);
static int  ox820sata_driver_remove(struct platform_device *);

static void ox820sata_dev_config(struct ata_device *);
static void ox820sata_tf_load(struct ata_port *ap, const struct ata_taskfile *tf);
static bool ox820sata_qc_fill_rtf(struct ata_queued_cmd *qc);
static void ox820sata_tf_read(struct ata_port *ap, struct ata_taskfile *tf);
static u8 ox820sata_check_status(struct ata_port *ap);
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
static void ox820sata_freeze(struct ata_port* ap);
static void ox820sata_thaw(struct ata_port* ap);
#endif
static int ox820sata_qc_new(struct ata_port *ap);
static int ox820sata_qc_defer(struct ata_queued_cmd *qc);
static void ox820sata_qc_free(struct ata_queued_cmd *qc);
static void ox820sata_qc_prep(struct ata_queued_cmd *qc);
static unsigned int ox820sata_qc_issue(struct ata_queued_cmd *qc);

static irqreturn_t ox820sata_irq_handler(int, void *);
static void ox820sata_irq_on(struct ata_port *ap);
static void ox820sata_irq_clear(struct ata_port *);

static int ox820sata_port_start(struct ata_port *ap);
static void ox820sata_port_stop(struct ata_port *ap);
static void ox820sata_host_stop(struct ata_host *host_set);
static u32* ox820sata_get_io_base(struct ata_port* ap);

void CrazyDumpDebug( void );

static void ox820sata_reset_core(void);

static void ox820sata_error_handler(struct ata_port *ap);

static void ox820sata_postreset(struct ata_link *link, unsigned int *classes);
static int ox820sata_softreset(struct ata_link *link, unsigned int *class, unsigned long deadline);
static void ox820sata_post_reset_init(struct ata_port* ap);
static void ox820sata_post_internal_cmd(struct ata_queued_cmd *qc);
void ox820sata_set_mode(u32 mode, u32 force);
cleanup_recovery_t ox820sata_cleanup(void);

u32 ox820sata_link_read(u32* core_addr, unsigned int sc_reg);
void ox820sata_link_write(u32* core_addr, unsigned int sc_reg, u32 val);
static int ox820sata_scr_read_port(struct ata_port *ap, unsigned int sc_reg, u32 *val);
static int ox820sata_scr_write_port(struct ata_port *ap, unsigned int sc_reg, u32 val);
static int ox820sata_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val);
static int ox820sata_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val);

#ifdef ERROR_INJECTION
static int ox820sata_error_inject_show(char *page, char **start, off_t off, int count, int *eof, void *data);
static int ox820sata_error_inject_store(struct file *file,const char __user *buffer,unsigned long count,void *data);
#endif

static int acquire_hw(int port_no, int may_sleep, int timeout_jiffies);

#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
extern void ox820hwraid_restart_queue(void);
#endif

ox820sata_driver_t ox820sata_driver = 
{
    .driver = {
        .driver.name = DRIVER_NAME,
        .driver.bus = &platform_bus_type,
        .probe = ox820sata_driver_probe, 
        .remove = ox820sata_driver_remove,
    },
    .ap = {0,0},
#ifdef ERROR_INJECTION
    .error_period = 0,
#endif
};

static struct ata_port_operations ox820sata_port_ops =
{
	.inherits			= &sata_port_ops,
                        
	.qc_defer			= ox820sata_qc_defer,
	.qc_prep			= ox820sata_qc_prep,
	.qc_issue			= ox820sata_qc_issue,
	.qc_fill_rtf		= ox820sata_qc_fill_rtf,
                        
	.qc_new				= ox820sata_qc_new,
	.qc_free			= ox820sata_qc_free,
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
    .freeze             = ox820sata_freeze,
    .thaw               = ox820sata_thaw,
#endif                        
	.softreset			= ox820sata_softreset,
    .dev_config         = ox820sata_dev_config,
                        
    .scr_read           = ox820sata_scr_read,
    .scr_write          = ox820sata_scr_write,
                        
    .port_start         = ox820sata_port_start,
    .port_stop          = ox820sata_port_stop,
    .host_stop          = ox820sata_host_stop,
                        
	.postreset		    = ox820sata_postreset,
	.post_internal_cmd  = ox820sata_post_internal_cmd,
    .error_handler      = ox820sata_error_handler,

	.acquire_hw         = acquire_hw,
};

static struct scsi_host_template ox820sata_sht = 
{
    ATA_BASE_SHT(DRIVER_NAME),
    .sg_tablesize       = CONFIG_ARCH_OXNAS_MAX_SATA_SG_ENTRIES,
    .dma_boundary       = ~0UL,  
    .unchecked_isa_dma  = 0,
};

static const struct ata_port_info ox820sata_port_info = {
    .flags = ATA_FLAG_SATA |
             ATA_FLAG_SATA_RESET |
             ATA_FLAG_NO_LEGACY |
             ATA_FLAG_NO_ATAPI |
             ATA_FLAG_PIO_DMA |
             ATA_FLAG_PMP ,
    .pio_mask   = 0x1f,  
    .mwdma_mask = 0x07,  
    .udma_mask  = 0x7f,  
    .port_ops   = &ox820sata_port_ops,
};

static spinlock_t access_lock = SPIN_LOCK_UNLOCKED;
static int core_locked = 0;
static int reentrant_port_no = -1;
static int hw_lock_count = 0;
static int direct_lock_count = 0;
static void *locker_uid = 0;
static sata_locker_t current_locker_type = SATA_UNLOCKED;
static const void *HW_LOCKER_UID = (void*)0xdeadbeef;
static DECLARE_WAIT_QUEUE_HEAD(sata_wait_queue);
static atomic_t scsi_wants_access = ATOMIC_INIT(0);
static ox820sata_isr_callback_t ox820sata_isr_callback = NULL;
static unsigned long ox820sata_isr_arg = 0;

static spinlock_t async_register_lock = SPIN_LOCK_UNLOCKED; 

#define PH_GAIN         2
#define FR_GAIN         3
#define PH_GAIN_OFFSET  6
#define FR_GAIN_OFFSET  8
#define PH_GAIN_MASK  (0x3 << PH_GAIN_OFFSET)
#define FR_GAIN_MASK  (0x3 << FR_GAIN_OFFSET)
#define USE_INT_SETTING  (1<<5)

#define CR_READ_ENABLE  (1<<16)
#define CR_WRITE_ENABLE (1<<17)
#define CR_CAP_DATA     (1<<18)

#define SATA_PHY_ASIC_STAT (SATA_PHY_BASE + 0x00)
#define SATA_PHY_ASIC_DATA (SATA_PHY_BASE + 0x04)

static void wait_cr_ack(void){
	while ((readl(SATA_PHY_ASIC_STAT) >> 16) & 0x1f)
		  ;
}

static u16 read_cr(u16 address) {
	writel(address, SATA_PHY_ASIC_STAT);
	wait_cr_ack();
	writel(CR_READ_ENABLE, SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	return readl(SATA_PHY_ASIC_STAT);
}

static void write_cr(u16 data, u16 address) {
	writel(address, SATA_PHY_ASIC_STAT);
	wait_cr_ack();
	writel((data | CR_CAP_DATA), SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	writel(CR_WRITE_ENABLE, SATA_PHY_ASIC_DATA);
	wait_cr_ack();
	return ;
}

void workaround5458(void){
	unsigned i;
	
	for (i=0; i<2;i++){
		u16 rx_control = read_cr( 0x201d + (i<<8));
		rx_control &= ~(PH_GAIN_MASK | FR_GAIN_MASK);
		rx_control |= PH_GAIN << PH_GAIN_OFFSET;
		rx_control |= (FR_GAIN << FR_GAIN_OFFSET) | USE_INT_SETTING ;
		write_cr( rx_control, 0x201d+(i<<8));
	}
}

static int __acquire_sata_core(
	int                      port_no,
	ox820sata_isr_callback_t callback,
	unsigned long            arg,
	int                      may_sleep,
	int                      timeout_jiffies,
	int                      hw_access,
	void                    *uid,
	sata_locker_t            locker_type)
{
	unsigned long end = jiffies + timeout_jiffies;
	int           acquired = 0;
	unsigned long flags;
	int           timed_out = 0;
	DEFINE_WAIT(wait);

	spin_lock_irqsave(&access_lock, flags);

	LPRINTK("Entered uid %p, port %d, h/w count %d, d count %d, callback %p, "
		    "hw_access %d, core_locked %d, reentrant_port_no %d, ox820sata_isr_callback %p\n",
		uid, port_no, hw_lock_count, direct_lock_count, callback, hw_access,
		core_locked, reentrant_port_no, ox820sata_isr_callback);

	while (!timed_out) {
		if (core_locked) {
			BUG_ON(!hw_lock_count && !direct_lock_count);

			if (hw_access && (port_no == reentrant_port_no)) {
				BUG_ON(!hw_lock_count);
				++hw_lock_count;

				LPRINTK("Allow SCSI/SATA re-entrant access to uid %p port %d\n", uid, port_no);
				acquired = 1;
				break;
			} else if (!hw_access) {
				if ((locker_type == SATA_READER) && (current_locker_type == SATA_READER)) {
					WARN(1,
						"Already locked by reader, uid %p, locker_uid %p, port %d, "
						"h/w count %d, d count %d, hw_access %d\n", uid, locker_uid,
						port_no, hw_lock_count, direct_lock_count, hw_access);
					goto check_uid;
				}
				
				if ((locker_type != SATA_READER) && (locker_type != SATA_WRITER)) {
					goto wait_for_lock;
				}

check_uid:
				WARN(uid == locker_uid, "Attempt to lock by locker type %d "
					"uid %p, already locked by locker type %d with "
					"locker_uid %p, port %d, h/w count %d, d count %d, "
					"hw_access %d\n", locker_type, uid, current_locker_type,
					locker_uid, port_no, hw_lock_count, direct_lock_count, hw_access);
			}
		} else {
			BUG_ON(hw_lock_count || direct_lock_count);
			BUG_ON(current_locker_type != SATA_UNLOCKED);

			WARN(locker_uid, "Attempt to lock uid %p when locker_uid %p is "
				"non-zero,  port %d, h/w count %d, d count %d, hw_access %d\n",
				uid, locker_uid, port_no, hw_lock_count, direct_lock_count,
				hw_access);

			if (!hw_access) {
				 
				BUG_ON(!callback);	 
				BUG_ON(reentrant_port_no != -1);  

				ox820sata_isr_callback = callback;
				ox820sata_isr_arg = arg;
				++direct_lock_count;

				current_locker_type = locker_type;
			} else {
				 
				BUG_ON(callback);	 
				BUG_ON(arg);		 

				BUG_ON(ox820sata_isr_callback);	 
				BUG_ON(ox820sata_isr_arg);		 

				++hw_lock_count;
				reentrant_port_no = port_no;
				
				current_locker_type = SATA_SCSI_STACK;
			}

			core_locked = 1;
			acquired = 1;
			locker_uid = uid;
			break;
		}

wait_for_lock:
		if (!may_sleep) {
		    LPRINTK("Denying for uid %p port %d as cannot sleep\n", uid, port_no);
			break;
		}

		for (;;) {
			prepare_to_wait(&sata_wait_queue, &wait, TASK_UNINTERRUPTIBLE);
			if (!core_locked) {
				 
				smp_rmb();
				break;
			}
			if (time_after(jiffies, end)) {
				printk("__acquire_sata_core() uid %p failing for port %d timed out, "
					   "locker_uid %p, h/w count %d, d count %d, callback %p, hw_access %d, "
					   "core_locked %d, reentrant_port_no %d, ox820sata_isr_callback %p, "
					   "ox820sata_isr_arg %p\n", uid, port_no, locker_uid,
					   hw_lock_count, direct_lock_count, callback, hw_access,
					   core_locked, reentrant_port_no, ox820sata_isr_callback,
					   (void*)ox820sata_isr_arg);
				timed_out = 1;
				break;
			}
			spin_unlock_irqrestore(&access_lock, flags);
			if (!schedule_timeout(HZ)) {
				printk(KERN_INFO "__acquire_sata_core() uid %p, locker_uid %p, "
					"timed-out of schedule(), checking overall timeout\n",
					uid, locker_uid);
			}
			spin_lock_irqsave(&access_lock, flags);
		}
		finish_wait(&sata_wait_queue, &wait);
	}

	if (hw_access) {
		atomic_set(&scsi_wants_access, !acquired);
	}

	LPRINTK("Leaving uid %p with acquired = %d, port %d, callback %p\n", uid, acquired, port_no, callback);

	spin_unlock_irqrestore(&access_lock, flags);

	return acquired;
}

static inline int ox820sata_is_host_frozen(void)
{
    smp_rmb();
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
    return( ox820sata_driver.port_frozen || ox820sata_driver.port_in_eh);
#else
    return ox820sata_driver.port_in_eh;
#endif
}

int acquire_sata_core_direct(
	ox820sata_isr_callback_t callback,
	unsigned long            arg,
    int                      timeout_jiffies,
	void                    *uid,
	sata_locker_t            locker_type)
{
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID
    unsigned long end = jiffies + timeout_jiffies;
    int ret = 0;
    do {
         
        if (unlikely(ox820sata_is_host_frozen())) {
            msleep(50);
            continue;
        }
         
        ret = __acquire_sata_core(0, callback, arg, 1, timeout_jiffies, 0, uid, locker_type); 
        if (likely(ret)) {
             
            if (unlikely(ox820sata_is_host_frozen())) {
                 
                release_sata_core_without_restart(locker_type);
                ret = 0;
            } else {
                break;
            }
        }
        
    } while (time_before(jiffies, end));
    
	return ret;
#else  
	return __acquire_sata_core(0, callback, arg, 1, timeout_jiffies, 0, uid, locker_type);
#endif    
}
EXPORT_SYMBOL(acquire_sata_core_direct);

int acquire_sata_core_hwraid(
	ox820sata_isr_callback_t callback,
	unsigned long            arg,
	void                    *uid)
{
	return likely(!ox820sata_is_host_frozen() &&
	    __acquire_sata_core(0, callback, arg, 0, 0, 0, uid, SATA_HWRAID));
}

void release_sata_core_without_restart(sata_locker_t locker_type)
{
	unsigned long flags;

	spin_lock_irqsave(&access_lock, flags);

	LPRINTK("Entered, h/w count %d, d count %d, reentrant_port_no %d, core_locked %d, ox820sata_isr_callback %p\n",
		hw_lock_count, direct_lock_count, reentrant_port_no, core_locked, ox820sata_isr_callback);

	BUG_ON(!direct_lock_count || (direct_lock_count > 1));
	BUG_ON(hw_lock_count);
	BUG_ON(reentrant_port_no != -1);
	BUG_ON(!core_locked);

	if (unlikely(locker_type != current_locker_type)) {
		WARN(1, "Attempt to unlock by a SATA locker type %d when is locked by a "
			"SATA locker type %d (locker uid %p)\n", locker_type,
			current_locker_type, locker_uid);
	}

	WARN(!locker_uid || (locker_uid == HW_LOCKER_UID), "Invalid locker_uid %p, "
		"h/w count %d, d count %d, reentrant_port_no %d, core_locked %d, "
		"ox820sata_isr_callback %p\n", locker_uid, hw_lock_count, direct_lock_count,
		reentrant_port_no, core_locked, ox820sata_isr_callback);

	ox820sata_isr_callback = NULL;
	--direct_lock_count;
	locker_uid = 0;
	current_locker_type = SATA_UNLOCKED;
	core_locked = 0;
	wake_up(&sata_wait_queue);

	LPRINTK("Leaving\n");

	spin_unlock_irqrestore(&access_lock, flags);
}
 
void release_sata_core(sata_locker_t locker_type)
{
	release_sata_core_without_restart(locker_type);
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID    
	ox820hwraid_restart_queue();
#endif    
}
EXPORT_SYMBOL(release_sata_core);

int sata_core_has_waiters(void)
{
    smp_rmb();
	return !list_empty(&sata_wait_queue.task_list) || atomic_read(&scsi_wants_access);
}
EXPORT_SYMBOL(sata_core_has_waiters);

static int acquire_hw(
	int port_no,
	int may_sleep,
	int timeout_jiffies)
{
	return __acquire_sata_core(port_no, NULL, 0, may_sleep, timeout_jiffies, 1, (void*)HW_LOCKER_UID, SATA_SCSI_STACK);
}

static void release_hw(unsigned int port_no)
{
	unsigned long flags;
	int released = 0;

	spin_lock_irqsave(&access_lock, flags);

	LPRINTK("Entered port_no = %d, h/w count %d, d count %d, core locked = %d, "
		"reentrant_port_no = %d, ox820sata_isr_callback %p\n", port_no,
		hw_lock_count, direct_lock_count, core_locked, reentrant_port_no, ox820sata_isr_callback);

	if (!core_locked) {
		 
		printk(KERN_WARNING "Nobody holds SATA lock, port_no %d\n", port_no);
        released = 1;
	} else if (!hw_lock_count) {
		 
		printk(KERN_WARNING "SCSI/SATA does not hold SATA lock, port_no %d\n", port_no);
	} else {
		 
		BUG_ON(reentrant_port_no == -1);
		BUG_ON(port_no != reentrant_port_no);
		BUG_ON(direct_lock_count);
		BUG_ON(current_locker_type != SATA_SCSI_STACK);

		WARN(!locker_uid || (locker_uid != HW_LOCKER_UID), "Invalid locker "
			"uid %p, h/w count %d, d count %d, reentrant_port_no %d, "
			"core_locked %d, ox820sata_isr_callback %p\n", locker_uid,
			hw_lock_count, direct_lock_count, reentrant_port_no, core_locked,
			ox820sata_isr_callback);

		if (--hw_lock_count) {
			LPRINTK("Still nested port_no %d\n", port_no);
		} else {
			LPRINTK("Release port_no %d\n", port_no);
			reentrant_port_no = -1;
			ox820sata_isr_callback = NULL;
			current_locker_type = SATA_UNLOCKED;
			locker_uid = 0;
			core_locked = 0;
			released = 1;
			wake_up(&sata_wait_queue);
		}
	}

	LPRINTK("Leaving, port_no %d, count %d\n", port_no, hw_lock_count);

    spin_unlock_irqrestore(&access_lock, flags);
#ifdef CONFIG_SATA_OX820_DIRECT_HWRAID    
    if (released)
        ox820hwraid_restart_queue();
#endif    
}

static inline u32* ox820sata_get_io_base(struct ata_port* ap)
{
    return ((ox820sata_private_data* )(ap->private_data))->reg_base;
}

static inline u32 ox820sata_hostportbusy(struct ata_port* ap) {
     
    u32 reg;
    reg = readl((u32* )SATA0_REGS_BASE + OX820SATA_SATA_COMMAND);
    if (unlikely(reg & CMD_CORE_BUSY)) {
       return 1;
    }
    reg = readl((u32* )SATA1_REGS_BASE + OX820SATA_SATA_COMMAND);
    if (unlikely(reg & CMD_CORE_BUSY)) {
       return 1;
    }
    
    return 0;
}

static inline u32 ox820sata_hostdmabusy(struct ata_port* ap) {
    ox820sata_private_data* pd = (ox820sata_private_data*)ap->private_data;
    
    if (unlikely(readl(pd->sgdma_controller + OX820SATA_SGDMA_STATUS) & OX820SATA_SGDMA_BUSY)) {
       return 1;
    }
    
    return 0;
}

static void ox820sata_reset_core( void ){
     
    writel(1UL << SYS_CTRL_CKEN_SATA_BIT, SYS_CTRL_CKEN_SET_CTRL);
    wmb();

    writel( (1UL << SYS_CTRL_RSTEN_SATA_BIT)      |
            (1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT) |
            (1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT), SYS_CTRL_RSTEN_SET_CTRL);
    wmb();
    udelay(50);
    
    writel(1UL << SYS_CTRL_RSTEN_SATA_PHY_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
    udelay(50);
    writel( (1UL << SYS_CTRL_RSTEN_SATA_LINK_BIT) |
            (1UL << SYS_CTRL_RSTEN_SATA_BIT), SYS_CTRL_RSTEN_CLR_CTRL);
    udelay(50);

    workaround5458();
     
    ox820sata_link_write((u32* )SATA0_REGS_BASE , 0x60, 0x2988 );

    ox820sata_link_write((u32* )SATA0_REGS_BASE , 0x70, 0x55629 );
    ox820sata_link_write((u32* )SATA1_REGS_BASE , 0x70, 0x55629 );
    udelay(50);
}

static int ox820sata_driver_probe(struct platform_device* pdev)
{
    u32 version;
    struct ata_host *host;
    void __iomem* iomem;
    const struct ata_port_info* port_info[] = {
        &ox820sata_port_info,
        &ox820sata_port_info,
        NULL
    };
    struct resource* memres = platform_get_resource(pdev, IORESOURCE_MEM, 0 );
    int irq = platform_get_irq(pdev, 0);
    
    if ((memres == NULL) || (irq < 0)) {
        return 0;
    }
    iomem = (void __iomem* ) memres->start;
    
    version = readl(((u32* )iomem) + OX820SATA_VERSION);
    switch (version) {
        case OX820SATA_CORE_VERSION:
            printk(KERN_INFO"ox820sata: OX820 sata core.\n");   
            break;
        default:
            printk(KERN_ERR"ox820sata: unknown sata core (version register = 0x%08x)\n",version);     
            return 0;
            break;
    }

    host = ata_host_alloc_pinfo(&(pdev->dev), port_info, OX820SATA_MAX_PORTS);
    if (!host) {
        printk(KERN_ERR DRIVER_NAME " Couldn't create an ata host.\n");
    }

    host->iomap  = iomem;

    ata_host_activate(host, irq, ox820sata_irq_handler, IRQF_SHARED, &ox820sata_sht);

    return 0;
}

static int ox820sata_driver_remove(struct platform_device* pdev)
{
    struct ata_host *host_set = dev_get_drvdata( &(pdev->dev) );
    struct ata_port *ap;
    unsigned int i;
    
    for (i = 0; i < host_set->n_ports; i++) 
    {
        ap = host_set->ports[i];
        scsi_remove_host( ap->scsi_host );
    }
    
    writel(1UL << SYS_CTRL_CKEN_SATA_BIT, SYS_CTRL_CKEN_CLR_CTRL);
    
    return 0;
}

static int __init ox820sata_init_driver( void )
{
    int ret;
    ret = platform_driver_register( &ox820sata_driver.driver );
    DPRINTK(" %i\n", ret);
#ifdef ERROR_INJECTION
    {
        struct proc_dir_entry *res=create_proc_entry("ox820sata_errorinject",0,NULL);
        if (res) {
            res->read_proc=ox820sata_error_inject_show;
            res->write_proc=ox820sata_error_inject_store;
            res->data=NULL;
        }
    }
#endif
    return ret; 
}

static void __exit ox820sata_exit_driver( void )
{
#ifdef ERROR_INJECTION
    remove_proc_entry("ox820sata_errorinject", NULL);
#endif
    platform_driver_unregister( &ox820sata_driver.driver );
}

module_init(ox820sata_init_driver);
module_exit(ox820sata_exit_driver);

static void ox820sata_dev_config(struct ata_device* pdev)
{
    
    u32 reg;
    u32 *ioaddr = ox820sata_get_io_base(pdev->link->ap);

    reg = readl(ioaddr + OX820SATA_DRIVE_CONTROL);
    reg &= ~3;
    reg |= (pdev->flags & ATA_DFLAG_LBA48) ? OX820SATA_DR_CON_48 : OX820SATA_DR_CON_28;
    writel(reg, ioaddr + OX820SATA_DRIVE_CONTROL);

    if (pdev->flags & ATA_DFLAG_LBA48) {
        reg = readl(ioaddr + OX820SATA_PORT_CONTROL);
        reg |= 2;
        writel(reg, ioaddr + OX820SATA_PORT_CONTROL);
    }
}

static void tfdump(const struct ata_taskfile* tf)
{
    if (tf->flags & ATA_TFLAG_LBA48) {
#ifdef SATA_TF_DUMP
    printk("Cmd %x Ft %x%x, LBA-48 %02x%02x%02x%02x%02x%02x, nsect %02x%02x, ctl %02x, dev %x\n",
#else  
    DPRINTK("Cmd %x Ft %x%x, LBA-48 %02x%02x%02x%02x%02x%02x, nsect %02x%02x, ctl %02x, dev %x\n",
#endif  
        tf->command,

        tf->hob_feature,
        tf->feature,

        tf->hob_lbah,
        tf->hob_lbam,
        tf->hob_lbal,
        tf->lbah,
        tf->lbam,
        tf->lbal,

        tf->hob_nsect,
        tf->nsect,
        tf->ctl,
        tf->device );
    } else {
#ifdef SATA_TF_DUMP
    printk("Cmd %x Ft %x, LBA-28 %01x%02x%02x%02x, nsect %02x, ctl %02x, dev %x\n",
#else  
    DPRINTK("Cmd %x Ft %x, LBA-28 %01x%02x%02x%02x, nsect %02x, ctl %02x, dev %x\n",
#endif  
        tf->command,

        tf->feature,

        tf->device & 0x0f,        
        tf->lbah,
        tf->lbam,
        tf->lbal,

        tf->nsect,
        tf->ctl,
        tf->device );
    }
}

static void ox820sata_tf_load(struct ata_port *ap, const struct ata_taskfile *tf)
{
    u32 count = 0;
    u32 Orb1 = 0; 
    u32 Orb2 = 0; 
    u32 Orb3 = 0;
    u32 Orb4 = 0;
    u32 Command_Reg;
    u32 *ioaddr = ox820sata_get_io_base(ap);
    unsigned int is_addr = tf->flags & ATA_TFLAG_ISADDR;

    do {
        Command_Reg = readl(ioaddr + OX820SATA_SATA_COMMAND);
        if (!(Command_Reg & CMD_CORE_BUSY)) {
            break;
		}
        count++;
		udelay(50);
    } while (count < 200);

    if (tf->ctl & ATA_NIEN) {
         
        u32 mask = (OX820SATA_COREINT_END << ap->port_no );
        writel(mask, OX820SATA_CORE_INT_DISABLE);
        ox820sata_irq_clear(ap);
    } else {
        ox820sata_irq_on(ap);
    }

    Orb2 |= (tf->command)    << 24;
    
    if (is_addr) {
         
        Orb1 |= (tf->device & ATA_LBA) << 24;

        if (tf->flags & ATA_TFLAG_LBA48) {
             
            Orb1 |= ATA_LBA << 24;

            Orb2 |= (tf->hob_nsect)  << 8 ;

            Orb3 |= (tf->hob_lbal)   << 24;

            Orb4 |= (tf->hob_lbam)   << 0 ;
            Orb4 |= (tf->hob_lbah)   << 8 ;
            Orb4 |= (tf->hob_feature)<< 16;
        } else {
            Orb3 |= (tf->device & 0xf)<< 24;
        }

        Orb2 |= (tf->nsect)      << 0 ;
        Orb2 |= (tf->feature)    << 16;

        Orb3 |= (tf->lbal)       << 0 ;
        Orb3 |= (tf->lbam)       << 8 ;
        Orb3 |= (tf->lbah)       << 16;

        Orb4 |= (tf->ctl)        << 24;

    }

    if (tf->flags & ATA_TFLAG_DEVICE) {
        Orb1 |= (tf->device) << 24;
    }
    ap->last_ctl = tf->ctl;

    writel(Orb1, ioaddr + OX820SATA_ORB1 );
    writel(Orb2, ioaddr + OX820SATA_ORB2 );
    writel(Orb3, ioaddr + OX820SATA_ORB3 );
    writel(Orb4, ioaddr + OX820SATA_ORB4 );

    tfdump(tf);
}

static bool ox820sata_qc_fill_rtf(struct ata_queued_cmd *qc)
{
    DPRINTK("tag %d\n", qc->tag);

	ox820sata_tf_read(qc->ap, &qc->result_tf);
	return true;
}

static void ox820sata_tf_read(struct ata_port *ap, struct ata_taskfile *tf)
{
    u32 *ioaddr = ox820sata_get_io_base(ap);

    u32 Orb1 = readl(ioaddr + OX820SATA_ORB1); 
    u32 Orb2 = readl(ioaddr + OX820SATA_ORB2); 
    u32 Orb3 = readl(ioaddr + OX820SATA_ORB3);
    u32 Orb4 = readl(ioaddr + OX820SATA_ORB4);

    tf->device  = (Orb1 >> 24);
    tf->nsect   = (Orb2 >> 0);
    tf->feature = (Orb2 >> 16);
    tf->command = ox820sata_check_status(ap);

    if (tf->flags & ATA_TFLAG_LBA48) {
         
        tf->hob_nsect = (Orb2 >> 8) ;
        
        tf->lbal      = (Orb3 >> 0) ;
        tf->lbam      = (Orb3 >> 8) ;
        tf->lbah      = (Orb3 >> 16) ;
        tf->hob_lbal  = (Orb3 >> 24) ;
        
        tf->hob_lbam  = (Orb4 >> 0) ;
        tf->hob_lbah  = (Orb4 >> 8) ;
         
    } else {
         
        tf->lbal      = (Orb3 >> 0) ;
        tf->lbam      = (Orb3 >> 8) ;
        tf->lbah      = (Orb3 >> 16) ;
    }

}

static u8 ox820sata_check_status(struct ata_port *ap)
{
    u32 Reg;
    u8 status;
    u32 *ioaddr = ox820sata_get_io_base(ap);

    status = readl(ioaddr + OX820SATA_ORB2) >> 24;

    ox820sata_scr_read_port(ap, SCR_STATUS, &Reg );

    if (!(Reg & 0x1)) { 
        status |= ATA_DF;
        status |= ATA_ERR;
    }
     
    return status;
}

static int ox820sata_qc_new(struct ata_port *ap)
{
	DPRINTK("\n");
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
    smp_rmb();
    if (ox820sata_driver.port_frozen || ox820sata_driver.port_in_eh) {
        return 1;
    } else {
        return !acquire_hw(ap->port_no, 0, 0);
    }
#else
    smp_rmb();
    return ox820sata_driver.port_in_eh ? 1 : !acquire_hw(ap->port_no, 0, 0);
#endif
}

static int ox820sata_qc_defer(struct ata_queued_cmd *qc)
{
	DPRINTK("\n");
	return ata_std_qc_defer(qc);
}

static void ox820sata_qc_free(struct ata_queued_cmd *qc)
{
    DPRINTK("\n");
	release_hw(qc->ap->port_no);
}

#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
static void ox820sata_freeze(struct ata_port* ap)
{
    DPRINTK("\n");
    set_bit(ap->port_no, &ox820sata_driver.port_frozen);
    smp_wmb();
}

static void ox820sata_thaw(struct ata_port* ap)
{
    DPRINTK("\n");
    clear_bit(ap->port_no, &ox820sata_driver.port_frozen);
    smp_wmb();
}
#endif

static void ox820sata_qc_prep(struct ata_queued_cmd* qc) 
{
    ox820sata_private_data* pd;
    int port_no;

    DPRINTK("\n");

#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
    port_no = qc->ap->port_no;
     
    ox820sata_set_mode(OXNASSATA_NOTRAID, 0);

    writel( 0x0, OX820SATA_PORT_ERROR_MASK);
#else
    port_no = 0;
#endif    
     
    if (!ox820sata_check_link(qc->ap->port_no)) {
        printk(KERN_ERR"port %d not connected completing with error\n",qc->ap->port_no);
        qc->err_mask |= AC_ERR_ATA_BUS;
        ata_qc_complete(qc);
    }
    
	if (ata_is_dma(qc->tf.protocol) || ata_is_pio(qc->tf.protocol) )
	{
         
        ata_sff_qc_prep(qc);
        
        pd = (ox820sata_private_data*)qc->ap->private_data;
    
        writel(pd->sgdma_request_pa,
            pd->sgdma_controller + OX820SATA_SGDMA_REQUESTPTR );
        
        if (port_no == 0) {
            pd->sgdma_request_va->control = (qc->dma_dir == DMA_FROM_DEVICE) ? 
                    OX820SATA_SGDMA_REQCTL0IN : OX820SATA_SGDMA_REQCTL0OUT ;
        } else {
            pd->sgdma_request_va->control = (qc->dma_dir == DMA_FROM_DEVICE) ? 
                    OX820SATA_SGDMA_REQCTL1IN : OX820SATA_SGDMA_REQCTL1OUT ;
        }
        pd->sgdma_request_va->qualifier = OX820SATA_SGDMA_REQQUAL;
        pd->sgdma_request_va->src_pa = qc->ap->prd_dma;
        pd->sgdma_request_va->dst_pa = qc->ap->prd_dma;
        smp_wmb();

        writel(OX820SATA_SGDMA_CONTROL_NOGO,
            pd->sgdma_controller + OX820SATA_SGDMA_CONTROL);
        
    }
}

static unsigned int ox820sata_qc_issue(struct ata_queued_cmd *qc)
{
    ox820sata_private_data* pd;
    u32 reg;
    u32* ioaddr;
    int port_no;

    DPRINTK("\n");
    
    pd = (ox820sata_private_data*)qc->ap->private_data;
    ioaddr = ox820sata_get_io_base(qc->ap);
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
    port_no = qc->ap->port_no;
#else
    port_no = 0;
#endif

    if (readl(ioaddr + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY)
    {
        int count = 0;
        DPRINTK(KERN_ERR"core busy for a command on port %d\n",qc->ap->port_no);
        do {
            mdelay(1);
            if (++count > 100) {
                DPRINTK(KERN_ERR"core busy for a command on port %d\n",qc->ap->port_no);
                 
                ox820sata_cleanup();
            }
        } while (readl(ioaddr + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY);
    }

    reg = readl(OX820SATA_DATA_PLANE_CTRL);
    reg &= ~(OX820SATA_DPC_ERROR_MASK_BIT << qc->ap->port_no);
    writel(reg, OX820SATA_DATA_PLANE_CTRL);

#ifdef CONFIG_SATA_OXNAS_SINGLE_SATA    
	 
	writel(~0, (u32*)SATA0_REGS_BASE + OX820SATA_INT_DISABLE);
	writel(~0, (u32*)SATA1_REGS_BASE + OX820SATA_INT_DISABLE);
	writel(~0, (u32*)SATARAID_REGS_BASE + OX820SATA_INT_DISABLE);

	writel(~0, OX820SATA_CORE_INT_DISABLE);
    wmb();
#endif

	ox820sata_tf_load(qc->ap, &qc->tf);
	
	if (ata_is_dma(qc->tf.protocol) || ata_is_pio(qc->tf.protocol) )
	{
         
        writel(OX820SATA_SGDMA_CONTROL_GO,
            pd->sgdma_controller + OX820SATA_SGDMA_CONTROL);
        wmb();
    }

#ifdef CONFIG_SATA_OXNAS_SINGLE_SATA    
     
    writel(OX820SATA_INT_WANT, ioaddr + OX820SATA_INT_ENABLE);
    writel(OX820SATA_COREINT_END, OX820SATA_CORE_INT_ENABLE);
    wmb();
#endif
	
#ifdef ERROR_INJECTION
    {
        u32 val = 1;
        
        if (ox820sata_driver.error_period) {
            static int error_on_last_command = 0;
        
            if (time_after(jiffies, ox820sata_driver.next_error) && 
                !error_on_last_command)
            {
                DPRINTK("ox820sata_exec_command: error injection on\n");
                ox820sata_driver.next_error = jiffies + ox820sata_driver.error_period;
                error_on_last_command = 1;
                val |= (qc->dma_dir == DMA_TO_DEVICE) ? 4 : 0 ;
                val |= (qc->dma_dir == DMA_FROM_DEVICE) ? 8 : 0 ;
                printk(".");
            } else {
                error_on_last_command = 0;
            }
        }
        ox820sata_link_write( ioaddr, 0x14 , val );
    }
#endif

	reg = readl(ioaddr + OX820SATA_SATA_COMMAND);
    reg &= ~SATA_OPCODE_MASK;
    reg |= CMD_WRITE_TO_ORB_REGS;
    writel(reg , ioaddr + OX820SATA_SATA_COMMAND);
    wmb();
    
    return 0;
}

void ox820sata_checkforhotplug(int port_no)
{
    struct ata_port* ap = ox820sata_driver.ap[port_no];
    ata_ehi_hotplugged(&ap->link.eh_info);
    ata_port_freeze(ap);
}

static void ox820sata_port_irq(struct ata_port* ap)
{    
    struct ata_queued_cmd* qc;
    ox820sata_private_data* pd;
    u32 int_status;
    unsigned long flags = 0;
    u32* ioaddr = ox820sata_get_io_base(ap);

    qc = ata_qc_from_tag(ap, ap->link.active_tag);    
    pd = (ox820sata_private_data*)ap->private_data;

    int_status = readl(ioaddr + OX820SATA_INT_STATUS);

    if (likely(qc)) {
         
        qc->err_mask = ac_err_mask(ox820sata_check_status(ap));

        DPRINTK(" returning err_mask=0x%x\n", qc->err_mask);
        local_irq_save(flags);
        ox820sata_irq_clear(ap);
        local_irq_restore(flags);
        ata_qc_complete(qc);
    } else {
        VPRINTK("Ignoring interrupt, can't find the command tag=  %d %08x\n", ap->link.active_tag, ap->qc_active );
    }

#if !defined(CONFIG_SATA_OXNAS_SINGLE_SATA) || defined(CONFIG_SYNO_PLX_PORTING)
     
    if (unlikely(int_status & OX820SATA_INT_LINK_SERROR)) {
        u32 serror;
        ox820sata_scr_read_port(ap, SCR_ERROR, &serror);
        if(serror & (SERR_DEV_XCHG | SERR_PHYRDY_CHG)) {
            ata_ehi_hotplugged(&ap->link.eh_info);
            ata_port_freeze(ap);
        }
    }
#endif
}

static irqreturn_t ox820sata_irq_handler(int irq, void *dev_instance)
#ifdef CONFIG_SATA_OXNAS_SINGLE_SATA
{
    u32 int_status;
    irqreturn_t ret = IRQ_NONE;

    DPRINTK("\n");

    while ( (int_status = readl(OX820SATA_CORE_INT_STATUS)) & OX820SATA_COREINT_END ) {
		int isr_handled = 0;
		int port;
		int is_read;
		int quads_transferred;
		int remainder;
		int sector_quads_remaining;

        writel(int_status, OX820SATA_CORE_INT_CLEAR);

		port = 0;

		is_read = !(readl(OX820SATA_DM_DBG1) &
			(1UL << (port ? OX820SATA_CORE_PORT1_DATA_DIR_BIT :
							OX820SATA_CORE_PORT0_DATA_DIR_BIT)));

		quads_transferred =
			readl(port ? OX820SATA_DATACOUNT_PORT1 : OX820SATA_DATACOUNT_PORT0);

		remainder = quads_transferred & 0x7f;
		sector_quads_remaining = remainder ? (0x80 - remainder): 0;

		if (is_read && (sector_quads_remaining == 2)) {
			u32 sg_info_phys;
			oxnas_dma_simple_sg_info_t *sg_info;
			u32 entry_phys;
			prd_table_entry_t *entry;
			int total_len;
			int last_prd_len;
			int sata_offset;
			void *sata_data_ptr;
			u32 memory_phys;
			volatile u32 *memory_data_ptr;
			dma_addr_t mapped_adr;

#if (!defined(CONFIG_ODRB_USE_PRDS_FOR_SATA) || (defined(CONFIG_OXNAS_FAST_READS_AND_WRITES) && !defined(CONFIG_ODRB_USE_PRDS)))
#error "SATA read fixup only supported with PRD descriptors"
#endif

			sg_info_phys = readl(OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_REQUESTPTR);
			sg_info_phys |= DESCRIPTORS_BASE_PA;
 
			sg_info = (oxnas_dma_simple_sg_info_t*)descriptors_phys_to_virt(sg_info_phys);
 
			entry_phys = sg_info->dst_entries;
 
			entry = (prd_table_entry_t*)descriptors_phys_to_virt(entry_phys);
 
			total_len = 0;
 
			while (!(entry->flags_len & PRD_EOF_MASK)) {
				total_len += (entry->flags_len ?: PRD_MAX_LEN);
 
				entry++;
			}
			last_prd_len = ((entry->flags_len & ~PRD_EOF_MASK) ?: PRD_MAX_LEN);
			total_len += last_prd_len;
 
			BUG_ON(!total_len);
			sata_offset = ((total_len - 1) % 2048);
 
			sata_data_ptr = (void*)(port ? OX820SATA_DATA_MUX_RAM1 :
				OX820SATA_DATA_MUX_RAM0) + sata_offset - 7;

			memory_phys = entry->adr;
 
			memory_data_ptr = (volatile u32*)(phys_to_virt(memory_phys) + last_prd_len - 8);
 
			mapped_adr = dma_map_single(0, (void*)memory_data_ptr, 8, DMA_FROM_DEVICE);
			dma_unmap_single(0, mapped_adr, 8, DMA_FROM_DEVICE);

			*memory_data_ptr       = readl(sata_data_ptr);
			*(memory_data_ptr + 1) = readl(sata_data_ptr + 4);
 
			mapped_adr = dma_map_single(0, (void*)memory_data_ptr, 8, DMA_TO_DEVICE);
			dma_unmap_single(0, mapped_adr, 8, DMA_TO_DEVICE);
		} else if (sector_quads_remaining) {
			if (is_read) {
				printk(KERN_WARNING "SATA read fixup cannot deal with %d quads remaining\n",
					sector_quads_remaining);
			} else {
				printk(KERN_WARNING "SATA write fixup of %d quads remaining not supported\n",
					sector_quads_remaining);
			}
		}

		smp_rmb();
		if (ox820sata_isr_callback) {
			if (ox820sata_isr_callback(int_status, ox820sata_isr_arg) == IRQ_HANDLED) {
				isr_handled = 1;
				ret = IRQ_HANDLED;
			} else {
				printk(KERN_WARNING "Direct SATA did NOT return IRQ_HANDLED\n");
			}
		}

		if (!isr_handled) {
            u32 port_no;
            for (port_no = 0; port_no < OX820SATA_MAX_PORTS; ++port_no) {
                 
                u32 mask = (OX820SATA_CORERAW_HOST << port_no );
                if (int_status & mask) {
                     
                    writel(mask, OX820SATA_CORE_INT_CLEAR);
                    ox820sata_port_irq(((struct ata_host* )dev_instance)->ports[port_no]);
                    ret = IRQ_HANDLED;
                }
            }
        }
    }

    return ret;
}
#else  
{
    u32 int_status;
    irqreturn_t ret = IRQ_NONE;
    
    while ( (int_status = readl(OX820SATA_CORE_INT_STATUS)) & 0x0300 ) {
		smp_rmb();
        if (ox820sata_isr_callback) {
             
            ret |= ox820sata_isr_callback(irq, ox820sata_isr_arg);
        } else {
            u32 port_no;
             
            writel(int_status, OX820SATA_CORE_INT_CLEAR);
            
            for (port_no = 0; port_no < OX820SATA_MAX_PORTS; ++port_no) {
                 
                u32 mask = (OX820SATA_COREINT_END << port_no );
                if (int_status & mask) {
                     
                    writel(mask, OX820SATA_CORE_INT_CLEAR);
                    ox820sata_port_irq(((struct ata_host* )dev_instance)->ports[port_no]);
                    ret = IRQ_HANDLED;
                    break;
                }
            }
        }
    }
    return ret;
}
#endif  

static void ox820sata_irq_clear(struct ata_port* ap)
{
    u32 *ioaddr = ox820sata_get_io_base(ap);
     
    writel(~0, ioaddr + OX820SATA_INT_CLEAR);
    writel(OX820SATA_COREINT_END, OX820SATA_CORE_INT_CLEAR);
}

u32 ox820sata_link_read(u32* core_addr, unsigned int link_reg) 
{
    u32 result;
    u32 patience;
    unsigned long flags;
    
    spin_lock_irqsave(&async_register_lock, flags);

    writel(link_reg, core_addr + OX820SATA_LINK_RD_ADDR );

    for (patience = 0x100000; patience > 0; --patience) {
        if (readl(core_addr + OX820SATA_LINK_CONTROL) & 0x00000001) {
            break;
		}
    }

    result = readl(core_addr + OX820SATA_LINK_DATA);
	spin_unlock_irqrestore(&async_register_lock, flags);

#ifdef LIMIT_TO_1pt5Gbs    
     
    if ( sc_reg == 0x28 ) {
        VPRINTK("Reporting a 1.5Gb speed limit\n");
        result |= 0x00000010 ;
    }
#endif  
     
    return result;
}
EXPORT_SYMBOL(ox820sata_link_read);
 
static int ox820sata_scr_read_port(struct ata_port *ap, unsigned int sc_reg, u32 *val)
{
    u32* ioaddr = ox820sata_get_io_base(ap);
	*val = ox820sata_link_read(ioaddr, 0x20 + (sc_reg*4));
    return 0;
}

static int ox820sata_scr_read(struct ata_link *link, unsigned int sc_reg, u32 *val)
{
	return ox820sata_scr_read_port(link->ap, sc_reg, val);
}

void ox820sata_link_write(u32* core_addr, unsigned int link_reg, u32 val)
{
    u32 patience;
    unsigned long flags;

    spin_lock_irqsave(&async_register_lock, flags);
     
    writel(val, core_addr + OX820SATA_LINK_DATA );
    writel(link_reg , core_addr + OX820SATA_LINK_WR_ADDR );

    for (patience = 0x100000; patience > 0;--patience) {
        if (readl(core_addr + OX820SATA_LINK_CONTROL) & 0x00000001) {
            break;
		}
    }
	spin_unlock_irqrestore(&async_register_lock, flags);
}
 
static int ox820sata_scr_write_port(struct ata_port *ap, unsigned int sc_reg, u32 val)
{
    u32 *ioaddr = ox820sata_get_io_base(ap);
    ox820sata_link_write(ioaddr, 0x20 + (sc_reg * 4), val);
	return 0;
}

static int ox820sata_scr_write(struct ata_link *link, unsigned int sc_reg, u32 val)
{
	return ox820sata_scr_write_port(link->ap, sc_reg, val);
}

static int  ox820sata_port_start(struct ata_port *ap)
{
    ox820sata_private_data* pd;
    int dma_channel;

    pd = (ox820sata_private_data* )kmalloc(sizeof(ox820sata_private_data), GFP_KERNEL);
    if (!pd) {
        return -ENOMEM;
    }

    ox820sata_driver.ap[ap->port_no] = ap;

	ap->private_data = pd;
    DPRINTK("ap[%d] = %p, pd = %p\n", ap->port_no, ap, ap->private_data );

#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
    dma_channel = ap->port_no;
#else
    dma_channel = 0;
#endif
    
    pd->reg_base = (u32*)(ap->host->iomap + (ap->port_no * OX820SATA_PORT_SIZE));
    pd->dma_controller = 
        (u32* )(SATADMA_REGS_BASE + (dma_channel * OX820SATA_DMA_CORESIZE));  
    pd->sgdma_controller = 
        (u32* )(SATASGDMA_REGS_BASE + (dma_channel * OX820SATA_SGDMA_CORESIZE));  

    pd->sgdma_request_va = (sgdma_request_t* )(OX820SATA_SGDMA_REQ + 
        (dma_channel * sizeof(sgdma_request_t)));
    pd->sgdma_request_pa = (dma_addr_t)(OX820SATA_SGDMA_REQ_PA + 
        (dma_channel * sizeof(sgdma_request_t)));
        
    ap->prd = (struct ata_prd* )(OX820SATA_PRD +
        (dma_channel * CONFIG_ODRB_NUM_SATA_PRD_ARRAYS * sizeof(struct ata_prd)));
    ap->prd_dma = OX820SATA_PRD_PA +
        (dma_channel * CONFIG_ODRB_NUM_SATA_PRD_ARRAYS * sizeof(struct ata_prd));
    
    ox820sata_post_reset_init(ap);

    return 0;
}

static void ox820sata_port_stop(struct ata_port *ap)
{
    DPRINTK("\n");
    kfree(ap->private_data);
}

void ox820sata_set_mode(u32 mode, u32 force) {
    unsigned int* src;
    unsigned int dst;
    unsigned int progmicrocode = 0;
    unsigned int changeparameters = 0;
    static u32 previous_mode = OX820SATA_UNKNOWN_MODE;
    
    static const unsigned int jbod[] = {
        0x07B400AC, 0x0228A280, 0x00200001, 0x00204002, 0x00224001,
        0x00EE0009, 0x00724901, 0x01A24903, 0x00E40009, 0x00224001,
        0x00621120, 0x0183C908, 0x00E20005, 0x00718908, 0x0198A206,
        0x00621124, 0x0183C908, 0x00E20046, 0x00621104, 0x0183C908,
        0x00E20015, 0x00EE009D, 0x01A3E301, 0x00E2001B, 0x0183C900,
        0x00E2001B, 0x00210001, 0x00EE0020, 0x01A3E302, 0x00E2009D,
        0x0183C901, 0x00E2009D, 0x00210002, 0x0235D700, 0x0208A204,
        0x0071C908, 0x000F8207, 0x000FC207, 0x0071C920, 0x000F8507,
        0x000FC507, 0x0228A240, 0x02269A40, 0x00094004, 0x00621104,
        0x0180C908, 0x00E40031, 0x00621112, 0x01A3C801, 0x00E2002B,
        0x00294000, 0x0228A220, 0x01A69ABF, 0x002F8000, 0x002FC000,
        0x0198A204, 0x0001C022, 0x01B1A220, 0x0001C106, 0x00088007,
        0x0183C903, 0x00E2009D, 0x0228A220, 0x0071890C, 0x0208A206,
        0x0198A206, 0x0001C022, 0x01B1A220, 0x0001C106, 0x00088007,
        0x00EE009D, 0x00621104, 0x0183C908, 0x00E2004A, 0x00EE009D,
        0x01A3C901, 0x00E20050, 0x0021E7FF, 0x0183E007, 0x00E2009D,
        0x00EE0054, 0x0061600B, 0x0021E7FF, 0x0183C507, 0x00E2009D,
        0x01A3E301, 0x00E2005A, 0x0183C900, 0x00E2005A, 0x00210001,
        0x00EE005F, 0x01A3E302, 0x00E20005, 0x0183C901, 0x00E20005,
        0x00210002, 0x0235D700, 0x0208A204, 0x000F8109, 0x000FC109,
        0x0071C918, 0x000F8407, 0x000FC407, 0x0001C022, 0x01A1A2BF,
        0x0001C106, 0x00088007, 0x02269A40, 0x00094004, 0x00621112,
        0x01A3C801, 0x00E4007F, 0x00621104, 0x0180C908, 0x00E4008D,
        0x00621128, 0x0183C908, 0x00E2006C, 0x01A3C901, 0x00E2007B,
        0x0021E7FF, 0x0183E007, 0x00E2007F, 0x00EE006C, 0x0061600B,
        0x0021E7FF, 0x0183C507, 0x00E4006C, 0x00621111, 0x01A3C801,
        0x00E2007F, 0x00621110, 0x01A3C801, 0x00E20082, 0x0228A220,
        0x00621119, 0x01A3C801, 0x00E20086, 0x0001C022, 0x01B1A220,
        0x0001C106, 0x00088007, 0x0198A204, 0x00294000, 0x01A69ABF,
        0x002F8000, 0x002FC000, 0x0183C903, 0x00E20005, 0x0228A220,
        0x0071890C, 0x0208A206, 0x0198A206, 0x0001C022, 0x01B1A220,
        0x0001C106, 0x00088007, 0x00EE009D, 0x00621128, 0x0183C908,
        0x00E20005, 0x00621104, 0x0183C908, 0x00E200A6, 0x0062111C,
        0x0183C908, 0x00E20005, 0x0071890C, 0x0208A206, 0x0198A206,
        0x00718908, 0x0208A206, 0x00EE0005, ~0
    };
    
    static const unsigned int raid[] = {
        0x00F20145, 0x00EE20FA, 0x00EE20A7, 0x0001C009, 0x00EE0004,
        0x00220000, 0x0001000B, 0x037003FF, 0x00700018, 0x037003FE,
        0x037043FD, 0x00704118, 0x037043FC, 0x01A3D240, 0x00E20017,
        0x00B3C235, 0x00E40018, 0x0093C104, 0x00E80014, 0x0093C004,
        0x00E80017, 0x01020000, 0x00274020, 0x00EE0083, 0x0080C904,
        0x0093C104, 0x00EA0020, 0x0093C103, 0x00EC001F, 0x00220002,
        0x00924104, 0x0005C009, 0x00EE0058, 0x0093CF04, 0x00E80026,
        0x00900F01, 0x00600001, 0x00910400, 0x00EE0058, 0x00601604,
        0x01A00003, 0x00E2002C, 0x01018000, 0x00274040, 0x00EE0083,
        0x0093CF03, 0x00EC0031, 0x00220003, 0x00924F04, 0x0005C009,
        0x00810104, 0x00B3C235, 0x00E20037, 0x0022C000, 0x00218210,
        0x00EE0039, 0x0022C001, 0x00218200, 0x00600401, 0x00A04901,
        0x00604101, 0x01A0C401, 0x00E20040, 0x00216202, 0x00EE0041,
        0x00216101, 0x02018506, 0x00EE2141, 0x00904901, 0x00E20049,
        0x00A00401, 0x00600001, 0x02E0C301, 0x00EE2141, 0x00216303,
        0x037003EE, 0x01A3C001, 0x00E40105, 0x00250080, 0x00204000,
        0x002042F1, 0x0004C001, 0x00230001, 0x00100006, 0x02C18605,
        0x00100006, 0x01A3D502, 0x00E20055, 0x00EE0053, 0x00004009,
        0x00000004, 0x00B3C235, 0x00E40062, 0x0022C001, 0x0020C000,
        0x00EE2141, 0x0020C001, 0x00EE2141, 0x00EE006B, 0x0022C000,
        0x0060D207, 0x00EE2141, 0x00B3C242, 0x00E20069, 0x01A3D601,
        0x00E2006E, 0x02E0C301, 0x00EE2141, 0x00230001, 0x00301303,
        0x00EE007B, 0x00218210, 0x01A3C301, 0x00E20073, 0x00216202,
        0x00EE0074, 0x00216101, 0x02018506, 0x00214000, 0x037003EE,
        0x01A3C001, 0x00E40108, 0x00230001, 0x00100006, 0x00250080,
        0x00204000, 0x002042F1, 0x0004C001, 0x00EE007F, 0x0024C000,
        0x01A3D1F0, 0x00E20088, 0x00230001, 0x00300000, 0x01A3D202,
        0x00E20085, 0x00EE00A5, 0x00B3C800, 0x00E20096, 0x00218000,
        0x00924709, 0x0005C009, 0x00B20802, 0x00E40093, 0x037103FD,
        0x00710418, 0x037103FC, 0x00EE0006, 0x00220000, 0x0001000F,
        0x00EE0006, 0x00800B0C, 0x00B00001, 0x00204000, 0x00208550,
        0x00208440, 0x002083E0, 0x00208200, 0x00208100, 0x01008000,
        0x037083EE, 0x02008212, 0x02008216, 0x01A3C201, 0x00E400A5,
        0x0100C000, 0x00EE20FA, 0x02800000, 0x00208000, 0x00B24C00,
        0x00E400AD, 0x00224001, 0x00724910, 0x0005C009, 0x00B3CDC4,
        0x00E200D5, 0x00B3CD29, 0x00E200D5, 0x00B3CD20, 0x00E200D5,
        0x00B3CD24, 0x00E200D5, 0x00B3CDC5, 0x00E200D2, 0x00B3CD39,
        0x00E200D2, 0x00B3CD30, 0x00E200D2, 0x00B3CD34, 0x00E200D2,
        0x00B3CDCA, 0x00E200CF, 0x00B3CD35, 0x00E200CF, 0x00B3CDC8,
        0x00E200CC, 0x00B3CD25, 0x00E200CC, 0x00B3CD40, 0x00E200CB,
        0x00B3CD42, 0x00E200CB, 0x01018000, 0x00EE0083, 0x0025C000,
        0x036083EE, 0x0000800D, 0x00EE00D8, 0x036083EE, 0x00208035,
        0x00EE00DA, 0x036083EE, 0x00208035, 0x00EE00DA, 0x00208007,
        0x036083EE, 0x00208025, 0x036083EF, 0x02400000, 0x01A3D208,
        0x00E200D8, 0x0067120A, 0x0021C000, 0x0021C224, 0x00220000,
        0x00404B1C, 0x00600105, 0x00800007, 0x0020C00E, 0x00214000,
        0x01004000, 0x01A0411F, 0x00404E01, 0x01A3C101, 0x00E200F1,
        0x00B20800, 0x00E400D8, 0x00220001, 0x0080490B, 0x00B04101,
        0x0040411C, 0x00EE00E1, 0x02269A01, 0x01020000, 0x02275D80,
        0x01A3D202, 0x00E200F4, 0x01B75D80, 0x01030000, 0x01B69A01,
        0x00EE00D8, 0x01A3D204, 0x00E40104, 0x00224000, 0x0020C00E,
        0x0020001E, 0x00214000, 0x01004000, 0x0212490E, 0x00214001,
        0x01004000, 0x02400000, 0x00B3D702, 0x00E80112, 0x00EE010E,
        0x00B3D702, 0x00E80112, 0x00B3D702, 0x00E4010E, 0x00230001,
        0x00EE0140, 0x00200005, 0x036003EE, 0x00204001, 0x00EE0116,
        0x00230001, 0x00100006, 0x02C18605, 0x00100006, 0x01A3D1F0,
        0x00E40083, 0x037003EE, 0x01A3C002, 0x00E20121, 0x0020A300,
        0x0183D102, 0x00E20124, 0x037003EE, 0x01A00005, 0x036003EE,
        0x01A0910F, 0x00B3C20F, 0x00E2012F, 0x01A3D502, 0x00E20116,
        0x01A3C002, 0x00E20116, 0x00B3D702, 0x00E4012C, 0x00300000,
        0x00EE011F, 0x02C18605, 0x00100006, 0x00EE0116, 0x01A3D1F0,
        0x00E40083, 0x037003EE, 0x01A3C004, 0x00E20088, 0x00200003,
        0x036003EE, 0x01A3D502, 0x00E20136, 0x00230001, 0x00B3C101,
        0x00E4012C, 0x00100006, 0x02C18605, 0x00100006, 0x00204000,
        0x00EE0116, 0x00100006, 0x01A3D1F0, 0x00E40083, 0x01000000,
        0x02400000, ~0
    };
    
    if (force) {
        previous_mode = OX820SATA_UNKNOWN_MODE;
    }
    
    if (mode == previous_mode) {
        return;
    }
    
    switch(previous_mode) {
    case OXNASSATA_RAID1:
        switch(mode) {
        case OXNASSATA_RAID0:
            changeparameters = 1;
            break;
        case OXNASSATA_NOTRAID:
            changeparameters = 1;
            progmicrocode = 1;
            break;
        }
        break;
    case OXNASSATA_RAID0:
        switch(mode) {
        case OXNASSATA_RAID1:
            changeparameters = 1;
            break;
        case OXNASSATA_NOTRAID:
            changeparameters = 1;
            progmicrocode = 1;
            break;
        }
        break;
    case OXNASSATA_NOTRAID:
    case OX820SATA_UNKNOWN_MODE:
        changeparameters = 1;
        progmicrocode = 1;
        break;
    }

    if (progmicrocode) {
         
        writel(1, OX820SATA_PROC_RESET);
        wmb();
        
        switch(mode) {
        case OXNASSATA_RAID1:
        case OXNASSATA_RAID0:
            VPRINTK("Loading RAID micro-code\n");
            src = (unsigned int*)&raid[1];
            break;
        case OXNASSATA_NOTRAID:
            VPRINTK("Loading JBOD micro-code\n");
            src = (unsigned int*)&jbod[1];
            break;
        default:
            BUG();
            break;
        }
    
        dst = OX820SATA_UCODE_STORE;
        while (*src != ~0) {
            writel(*src,dst);
            src++;
            dst += sizeof(*src);
        }
        wmb();
    }
    
    if (changeparameters) {
        u32 reg;
             
        switch(mode) {
        case OXNASSATA_RAID1:
             
            reg = readl(OX820SATA_DATA_PLANE_CTRL);
            reg |= OX820SATA_DPC_JBOD_UCODE;
            reg &= ~OX820SATA_DPC_FIS_SWCH;
            writel(reg, OX820SATA_DATA_PLANE_CTRL);
            wmb();
            
            writel( 0, OX820SATA_RAID_WP_BOT_LOW );
            writel( 0, OX820SATA_RAID_WP_BOT_HIGH);
            writel( 0xffffffff, OX820SATA_RAID_WP_TOP_LOW );
            writel( 0x7fffffff, OX820SATA_RAID_WP_TOP_HIGH);
            writel( 0, OX820SATA_RAID_SIZE_LOW   );
            writel( 0, OX820SATA_RAID_SIZE_HIGH  );
            wmb();
            break;
        case OXNASSATA_RAID0:
             
            reg = readl(OX820SATA_DATA_PLANE_CTRL);
            reg |= OX820SATA_DPC_JBOD_UCODE;
            reg &= ~OX820SATA_DPC_FIS_SWCH;
            writel(reg, OX820SATA_DATA_PLANE_CTRL);
            wmb();
            
            writel( 0, OX820SATA_RAID_WP_BOT_LOW );
            writel( 0, OX820SATA_RAID_WP_BOT_HIGH);
            writel( 0xffffffff, OX820SATA_RAID_WP_TOP_LOW );
            writel( 0x7fffffff, OX820SATA_RAID_WP_TOP_HIGH);
            writel( 0xffffffff, OX820SATA_RAID_SIZE_LOW   );
            writel( 0x7fffffff, OX820SATA_RAID_SIZE_HIGH  );
            wmb();
            break;
        case OXNASSATA_NOTRAID:
             
            reg = readl(OX820SATA_DATA_PLANE_CTRL);
            reg &= ~OX820SATA_DPC_JBOD_UCODE;
            reg |=  OX820SATA_DPC_FIS_SWCH;
            writel(reg, OX820SATA_DATA_PLANE_CTRL);
            wmb();

            writel(1, OX820SATA_PROC_START);
            break;
        default:
            break;
        }
    }

    previous_mode = mode;
}
EXPORT_SYMBOL( ox820sata_set_mode );

static void ox820sata_post_reset_init(struct ata_port* ap)
{
    uint dev;
    u32* ioaddr = ox820sata_get_io_base(ap);

    VPRINTK("\n");
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
     
    ox820sata_set_mode(OXNASSATA_NOTRAID, 1);
#endif

    ox820sata_link_write(ioaddr, 0x0C, 0x30003);

    ox820sata_scr_write_port(ap, SCR_ERROR, ~0);
    ox820sata_scr_write_port(ap, OX820SATA_SERROR_IRQ_MASK, 0x03feffff);
    ox820sata_scr_write_port(ap, SCR_ACTIVE, ~0 & ~(1 << 26) & ~(1 << 16));
    
    ox820sata_irq_on(ap);
    
    for (dev = 0; dev < ATA_MAX_DEVICES; ++dev) {
        if (ap->link.device[dev].class == ATA_DEV_ATA) {
            sata_std_hardreset(&ap->link, NULL, jiffies + HZ);
            ox820sata_dev_config(&(ap->link.device[dev]));
		}
    }

    ox820sata_scr_write_port(ap, SCR_ERROR, ~0);
    VPRINTK("done\n");
}

static void ox820sata_host_stop(struct ata_host *host_set)
{
    DPRINTK("\n");
}

static inline void ox820sata_send_sync_escape(u32* base)
{
    u32 reg;
     
    if ((ox820sata_link_read(base, 0x20) & 3) == 3) {
        reg = readl(base + OX820SATA_SATA_COMMAND);
        reg &= ~SATA_OPCODE_MASK;
        reg |= CMD_SYNC_ESCAPE;
        writel(reg, base + OX820SATA_SATA_COMMAND);
    }
}

static inline void ox820sata_clear_CS_error(u32* base)
{
    u32 reg;
    reg = readl(base + OX820SATA_SATA_CONTROL);
    reg &= OX820SATA_SATA_CTL_ERR_MASK;
    writel(reg, base + OX820SATA_SATA_CONTROL);
}

static inline void ox820sata_clear_reg_access_error(u32* base)
{
    u32 reg;
    reg = readl(base + OX820SATA_INT_STATUS);
    if (reg & OX820SATA_INT_REG_ACCESS_ERR) {
        printk("clearing register access error\n");
        writel(OX820SATA_INT_REG_ACCESS_ERR, base + OX820SATA_INT_STATUS);
    }
    if (reg & OX820SATA_INT_REG_ACCESS_ERR) { 
        printk("register access error didn't clear\n");
    }    
}

cleanup_recovery_t ox820sata_cleanup(void) {
    int actions_required = 0;
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA
    u32 reg;
    u32 count;
    int was_a_write;

    const u32 delay_loops = 10000;
    count = delay_loops;
    was_a_write = readl(OX820SATA_DM_DBG1) &
        (OX820SATA_CORE_PORT0_DATA_DIR | OX820SATA_CORE_PORT1_DATA_DIR);
    if (!was_a_write) {
        
        ox820sata_clear_reg_access_error((u32*)SATA0_REGS_BASE);
        ox820sata_clear_reg_access_error((u32*)SATA1_REGS_BASE);
    
        DPRINTK("ox820sata resetting some things.\n");
         
        writel( OX820SATA_SGDMA_RESETS_CTRL, OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_RESETS);
        writel( OX820SATA_SGDMA_RESETS_CTRL, OX820SATA_SGDMA_BASE1 + OX820SATA_SGDMA_RESETS);
        
        reg = readl(OX820SATA_DMA_BASE0 + OX820SATA_DMA_CONTROL);
        reg |= OX820SATA_DMA_CONTROL_RESET;
        writel( reg, OX820SATA_DMA_BASE0 + OX820SATA_DMA_CONTROL);
        reg = readl(OX820SATA_DMA_BASE1 + OX820SATA_DMA_CONTROL);
        reg |= OX820SATA_DMA_CONTROL_RESET;
        writel( reg, OX820SATA_DMA_BASE1 + OX820SATA_DMA_CONTROL);
    
        reg = readl(OX820SATA_DEVICE_CONTROL);
        reg |= OX820SATA_DEVICE_CONTROL_DMABT << 0 ;
        reg |= OX820SATA_DEVICE_CONTROL_DMABT << 1 ;
        reg |= OX820SATA_DEVICE_CONTROL_ABORT ;
        writel( reg, OX820SATA_DEVICE_CONTROL);
        
        count = 0;
        while ((count < delay_loops) && (
               (readl((u32*)SATA0_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
               (readl((u32*)SATA1_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
               (readl((u32*)OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_STATUS)) ||
               (readl((u32*)OX820SATA_SGDMA_BASE1 + OX820SATA_SGDMA_STATUS)) ))
        {
            count++;
            udelay(50);
        }
    
        if (count >= delay_loops ) {
            DPRINTK("ox820sata sending sync escape\n");
            ox820sata_send_sync_escape((u32*)SATA0_REGS_BASE);
            ox820sata_send_sync_escape((u32*)SATA1_REGS_BASE);
            actions_required |= softreset;
        }
    
        count = 0;
        while ((count < delay_loops) && (
               (readl((u32*)SATA0_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
               (readl((u32*)SATA1_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
               (readl((u32*)OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_STATUS)) ||
               (readl((u32*)OX820SATA_SGDMA_BASE1 + OX820SATA_SGDMA_STATUS)) ))
        {
            count++;
            udelay(50);
        }
    
        if (count < delay_loops ) {
            DPRINTK("Core idle, clear resets.\n");
            
            ox820sata_clear_CS_error((u32*)SATA0_REGS_BASE);
            ox820sata_clear_CS_error((u32*)SATA1_REGS_BASE);
            
            ox820sata_scr_write_port(ox820sata_driver.ap[0], SCR_ERROR, ~0);
            ox820sata_scr_write_port(ox820sata_driver.ap[1], SCR_ERROR, ~0);
        
            reg = readl((u32*)SATA0_REGS_BASE + OX820SATA_SATA_CONTROL);
            reg |= OX820SATA_SCTL_CLR_ERR ;
            writel(reg, (u32*)SATA0_REGS_BASE + OX820SATA_SATA_CONTROL);
            reg = readl((u32*)SATA1_REGS_BASE + OX820SATA_SATA_CONTROL);
            reg |= OX820SATA_SCTL_CLR_ERR ;
            writel(reg, (u32*)SATA1_REGS_BASE + OX820SATA_SATA_CONTROL);
            reg = readl((u32*)SATARAID_REGS_BASE + OX820SATA_SATA_CONTROL);
            reg |= OX820SATA_RAID_CLR_ERR ;
            writel(reg, (u32*)SATARAID_REGS_BASE + OX820SATA_SATA_CONTROL);
            
            reg = readl(OX820SATA_DMA_BASE0 + OX820SATA_DMA_CONTROL);
            reg &= ~OX820SATA_DMA_CONTROL_RESET;
            writel( reg, OX820SATA_DMA_BASE0 + OX820SATA_DMA_CONTROL);
            reg = readl(OX820SATA_DMA_BASE1 + OX820SATA_DMA_CONTROL);
            reg &= ~OX820SATA_DMA_CONTROL_RESET;
            writel( reg, OX820SATA_DMA_BASE1 + OX820SATA_DMA_CONTROL);
        
            reg = readl(OX820SATA_DEVICE_CONTROL);
            reg &= ~OX820SATA_DEVICE_CONTROL_DMABT << 0 ;
            reg &= ~OX820SATA_DEVICE_CONTROL_DMABT << 1 ;
            reg &= ~OX820SATA_DEVICE_CONTROL_ABORT ;
            writel(reg, OX820SATA_DEVICE_CONTROL);
    
            reg = readl(OX820SATA_DEVICE_CONTROL);
            reg |= (OX820SATA_DEVICE_CONTROL_PRTRST |
                OX820SATA_DEVICE_CONTROL_RAMRST) << 0 ;
            reg |= (OX820SATA_DEVICE_CONTROL_PRTRST |
                OX820SATA_DEVICE_CONTROL_RAMRST) << 1 ;
            writel( reg, OX820SATA_DEVICE_CONTROL);
            wmb();
            
            writel(OX820SATA_CONFIG_IN_RESUME, OX820SATA_CONFIG_IN);
    
            count = 0;
            while ((count < delay_loops) && (
                   (readl((u32*)SATA0_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
                   (readl((u32*)SATA1_REGS_BASE + OX820SATA_SATA_COMMAND) & CMD_CORE_BUSY) ||
                   (readl((u32*)OX820SATA_SGDMA_BASE0 + OX820SATA_SGDMA_STATUS)) ||
                   (readl((u32*)OX820SATA_SGDMA_BASE1 + OX820SATA_SGDMA_STATUS)) ))
            {
                count++;
                udelay(50);
            }
        }
    }

    if  (count >= delay_loops) {
         
        printk(KERN_ERR"ox820sata unable to carefully recover "
            "from SATA error, reseting core\n");
#else  
    {
        printk(KERN_ERR"ox820sata: reseting SATA core\n");
#endif  

        mdelay(5);
        ox820sata_reset_core();
        mdelay(5);
        actions_required |= re_init;
         
        if (ox820sata_driver.ap[0]) {
            ox820sata_post_reset_init(ox820sata_driver.ap[0]);
        }
        if (ox820sata_driver.ap[1]) {
            ox820sata_post_reset_init(ox820sata_driver.ap[1]);
        }
    }

    return actions_required;
}

void ox820sata_freeze_host(int port_no)
{
    set_bit(port_no, &ox820sata_driver.port_in_eh);
    smp_wmb();
}

void ox820sata_thaw_host(int port_no)
{
    clear_bit(port_no, &ox820sata_driver.port_in_eh);
    smp_wmb();
}

#define ERROR_HW_ACQUIRE_TIMEOUT_JIFFIES (10 * HZ)
static void ox820sata_error_handler(struct ata_port *ap)
{
	DPRINTK("Enter port_no %d\n", ap->port_no);
	ox820sata_freeze_host(ap->port_no);

    if (!acquire_hw(ap->port_no, 1, ERROR_HW_ACQUIRE_TIMEOUT_JIFFIES)) {
        DPRINTK("unable to get hardware\n");
         
        return;
    }

    ox820sata_cleanup();

	ata_std_error_handler(ap);
	 
	DPRINTK("Releasing SATA core lock, port_no %d\n", ap->port_no);
	ox820sata_thaw_host(ap->port_no);
	release_hw(ap->port_no);
}

static void ox820sata_post_internal_cmd(struct ata_queued_cmd *qc) {
    if (qc->flags & ATA_QCFLAG_FAILED) {
          
        ox820sata_cleanup();       
    }
}

static void ox820sata_irq_on(struct ata_port *ap)
{
    u32* ioaddr = ox820sata_get_io_base(ap);
    u32 mask = (OX820SATA_COREINT_END << ap->port_no );

    VPRINTK("\n");

    writel(~0, ioaddr + OX820SATA_INT_CLEAR);
    writel(mask, OX820SATA_CORE_INT_STATUS);
    wmb();
    
    writel(OX820SATA_INT_WANT, ioaddr + OX820SATA_INT_ENABLE);
    writel(mask, OX820SATA_CORE_INT_ENABLE);
}

static int ox820sata_check_ready(struct ata_link *link)
{
	u8 status = ox820sata_check_status(link->ap);

	return ata_check_ready(status);
}

int ox820sata_check_link(int port_no) 
{
    int reg;
    struct ata_port* ap = ox820sata_driver.ap[port_no];
    int result = 0;
    if (ap) {
        ox820sata_scr_read_port(ap, SCR_STATUS, &reg );
    
        if (reg & 0x1) { 
            result = 1;
        }
    }
    
    return result;
}
EXPORT_SYMBOL( ox820sata_check_link );

static int ox820sata_softreset(struct ata_link *link, unsigned int *class,
			      unsigned long deadline)
{
	int rc;
	struct ata_port *ap;
    u32 *ioaddr;
	struct ata_taskfile tf;
    u32 Command_Reg;

	DPRINTK("ENTER\n");

	ap = link->ap;
    ioaddr = ox820sata_get_io_base(ap);

	if (ata_link_offline(link)) {
		DPRINTK("PHY reports no device\n");
		*class = ATA_DEV_NONE;
		goto out;
	}

    writel((ap->ctl) << 24, ioaddr + OX820SATA_ORB4);

    Command_Reg = readl(ioaddr + OX820SATA_SATA_COMMAND);
    Command_Reg &= ~SATA_OPCODE_MASK;
    Command_Reg |= CMD_WRITE_TO_ORB_REGS_NO_COMMAND;
    writel(Command_Reg, ioaddr + OX820SATA_SATA_COMMAND);
	udelay(20);	 

    writel((ap->ctl | ATA_SRST) << 24, ioaddr + OX820SATA_ORB4);

    Command_Reg &= ~SATA_OPCODE_MASK;
    Command_Reg |= CMD_WRITE_TO_ORB_REGS_NO_COMMAND;
    writel(Command_Reg, ioaddr + OX820SATA_SATA_COMMAND);
	udelay(20);	 
    
    writel((ap->ctl) << 24, ioaddr + OX820SATA_ORB4);

    Command_Reg &= ~SATA_OPCODE_MASK;
    Command_Reg |= CMD_WRITE_TO_ORB_REGS_NO_COMMAND;
    writel(Command_Reg, ioaddr + OX820SATA_SATA_COMMAND);

	msleep(150);

    rc = ata_wait_ready(link, deadline, ox820sata_check_ready);
    
	if (rc && (rc != -ENODEV || sata_scr_valid(link))) {
		ata_link_printk(link, KERN_ERR, "SRST failed (errno=%d)\n", rc);
		return rc;
	}

	ox820sata_tf_read(ap, &tf);
	*class = ata_dev_classify(&tf);

   	if (*class == ATA_DEV_UNKNOWN) {
		*class = ATA_DEV_NONE;
    }
 out:
	DPRINTK("EXIT, class=%u\n", *class );
	return 0;
}
    
static void ox820sata_postreset(struct ata_link *link, unsigned int *classes)
{
	struct ata_port *ap = link->ap;
    unsigned int dev;

	DPRINTK("ENTER\n");

	ata_std_postreset(link, classes);
    
    ox820sata_link_write((u32* )SATA0_REGS_BASE , 0x0c, 0x30003 );
    ox820sata_link_write((u32* )SATA1_REGS_BASE , 0x0c, 0x30003 );

	if (classes[0] == ATA_DEV_NONE && classes[1] == ATA_DEV_NONE) {
		DPRINTK("EXIT, no device\n");
		return;
	}

    for (dev = 0; dev < ATA_MAX_DEVICES; ++dev) {
        if (ap->link.device[dev].class == ATA_DEV_ATA) {
            ox820sata_dev_config(&(ap->link.device[dev]));
		}
    }

	DPRINTK("EXIT\n");
}

static void DumpPRDTable(struct ata_prd* prd) {
    u32 count = 0;
    printk("PRD Table at %p\n",prd);
    while(prd) {
        printk("    A %08x LF %x\n",prd->addr, prd->flags_len);
        if ((prd->flags_len & ATA_PRD_EOT) || (count > ATA_MAX_PRD))
            break;
        count++;
        prd++;
    } 
}

void CrazyDumpDebug(void)
{
#ifdef CRAZY_DUMP_DEBUG
    u32 offset;
    u32 result;
    u32 patience;
    volatile u32* ioaddr;

#if 0
    {
        u32 i ;
        for(i = 0;i < 1024;++i) {
            if (regarray[regindex].w & 2) printk("new sata command\n");
            printk("[%08x]%s%08x\n",
                regarray[regindex].a,
                (regarray[regindex].w & 1) ? "<=" : "=>",
                regarray[regindex].d
                );
            ++regindex;
            regindex &= 1023;
        }
    }
#endif

    ioaddr = (u32* )SATA0_REGS_BASE;
    printk("Port 0 High level registers\n");
    for(offset = 0; offset < 48;offset++)
    {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    printk("Port 0 link layer registers\n");
    for(offset = 0; offset < 16;++offset)
    {
        *(ioaddr + OX820SATA_LINK_RD_ADDR ) = (offset*4);
        wmb();
    
        for (patience = 0x100000;patience > 0;--patience)
        {
            if (*(ioaddr + OX820SATA_LINK_CONTROL) & 0x00000001)
                break;
        }
    
        result = *(ioaddr + OX820SATA_LINK_DATA);
        printk("[%02x] %08x\n", offset*4, result);
    }

    ioaddr = (u32* )SATA1_REGS_BASE;
    printk("Port 1 High level registers\n");
    for(offset = 0; offset < 48;offset++)
    {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    printk("Port 1 link layer registers\n");
    for(offset = 0; offset < 16;++offset)
    {
        *(ioaddr + OX820SATA_LINK_RD_ADDR ) = (offset*4);
        wmb();
    
        for (patience = 0x100000;patience > 0;--patience)
        {
            if (*(ioaddr + OX820SATA_LINK_CONTROL) & 0x00000001)
                break;
        }
    
        result = *(ioaddr + OX820SATA_LINK_DATA);
        printk("[%02x] %08x\n", offset*4, result);
    }
    
    ioaddr = (u32* )SATADMA_REGS_BASE;
    printk("DMA registers\n");
    for(offset = 0; offset < (2*8);offset++)
    {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    ioaddr = (u32* )SATASGDMA_REGS_BASE;
    printk("SGDMA registers\n");
    for(offset = 0; offset < 2*4;offset++)
    {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }

    DumpPRDTable((struct ata_prd* )(OX820SATA_PRD + (256 * 8 * 0)));
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
    DumpPRDTable((struct ata_prd* )(OX820SATA_PRD + (256 * 8 * 1)));
#endif
    
    ioaddr = (u32* )SATARAID_REGS_BASE;
    printk("RAID registers (port 15)\n");
    for(offset = 0; offset < 48;offset++)
    {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }
    
    ioaddr = (u32* )SATACORE_REGS_BASE;
    printk("CORE registers (port 14)\n");
    for(offset = 0; offset < 80;offset++)
    {
        printk("[%02x] %08x\n", offset * 4, *(ioaddr + offset));
    }
    
    printk("micro-code program counter poll\n");
    for(offset = 0; offset < 80;offset++)
    {
        printk("%d\n", *((volatile u32*)OX820SATA_PROC_PC) );
    }

    printk("core_locked %d\n", core_locked);
    printk("reentrant port number %d\n", reentrant_port_no);
    printk("libata/hw lock count %d\n", hw_lock_count);
    printk("direct lock count %d\n", direct_lock_count);
    printk("locker uid %p\n", locker_uid);
    printk("locker type is %d\n", current_locker_type);
    printk("scsi_wants_access %d\n", atomic_read(&scsi_wants_access));
    printk("ISR callback %p\n", ox820sata_isr_callback);
    printk("async_register_lock: ");
    if (spin_trylock(&async_register_lock)) {
        spin_unlock(&async_register_lock);
        printk("un");
    }
    printk("locked\n");
#ifndef CONFIG_SATA_OXNAS_SINGLE_SATA    
    printk("port frozen bitfield %lx\n", ox820sata_driver.port_frozen );
#endif    
    printk("port error-handler bitfield %lx\n", ox820sata_driver.port_in_eh );
#endif
}

#ifdef ERROR_INJECTION

static int ox820sata_error_inject_show(
    char  *page,
	char **start,
	off_t  off,
	int    count,
	int   *eof,
	void  *data)
{
    if (page) {
        if ( ox820sata_driver.error_period ) {
            int ret;
            ret = sprintf(page,"%d\n",ox820sata_driver.error_period);
            return ret;
        } else {
            return sprintf(page, "off\n" );
        }
    }

    return -EIO;
}

static int ox820sata_error_inject_store(
	struct file       *file,
	const char __user *buffer,
	unsigned long      count,
	void              *data) 
{
    if (count) {
        sscanf(buffer, "%d", &(ox820sata_driver.error_period));
        ox820sata_driver.next_error = jiffies + ox820sata_driver.error_period;
        return count;
    }

    return -EIO;
}
#endif  

static struct resource ox820sata_resources[] = {
	{
        .name       = "sata_registers",
		.start		= SATA0_REGS_BASE,
		.end		= SATA0_REGS_BASE + 0xfffff,
		.flags		= IORESOURCE_MEM,
	},
    {
        .name       = "sata_irq",
        .start      = SATA_INTERRUPT,
		.flags		= IORESOURCE_IRQ,
    }
};

static struct platform_device ox820sata_dev = 
{
    .name = DRIVER_NAME,
    .id = 0,
    .num_resources = 2,
	.resource  = ox820sata_resources,
    .dev.coherent_dma_mask = 0xffffffff,
}; 

static int __init ox820sata_device_init( void )
{
    int ret;

    ox820sata_reset_core();

    if (ATA_PRD_TBL_SZ > OX820SATA_PRD_SIZE) {
        printk(KERN_ERR"PRD table size is bigger than the space allocated for it in hardware.h");
        BUG();
    }

    if (sizeof(sgdma_request_t) > OX820SATA_SGDMA_SIZE) {
        printk(KERN_ERR"sgdma_request_t has grown beyond the space allocated for it in hardware.h");
        BUG();
    }

#ifndef CONFIG_OX820SATA_SINGLE_SATA
     
    if ((2 * ATA_PRD_TBL_SZ) > OX820SATA_PRD_SIZE) {
        printk(KERN_ERR"PRD table size is bigger than the space allocated for it in hardware.h");
        BUG();
    }

    if ((2 * sizeof(sgdma_request_t)) > OX820SATA_SGDMA_SIZE) {
        printk(KERN_ERR"sgdma_request_t has grown beyond the space allocated for it in hardware.h");
        BUG();
    }
#endif
     
    ret = platform_device_register( &ox820sata_dev );
    DPRINTK(" %i\n", ret);
   
    return ret;
}

static void __exit ox820sata_device_exit(void)
{
    platform_device_unregister( &ox820sata_dev );
}

int oxnassata_get_port_no(struct request_queue* q)
{
    struct ata_port* ap = 0;
    struct scsi_device* sdev = 0;
    
    ap = ox820sata_driver.ap[0];
    if (ap)
        shost_for_each_device(sdev, ap->scsi_host) {
            if (sdev->request_queue == q) {
                DPRINTK("Queue %p on port 0\n", q);
                return 0;
            }
        }
    
    ap = ox820sata_driver.ap[1];
    if (ap)
        shost_for_each_device(sdev, ap->scsi_host) {
            if (sdev->request_queue == q) {
                DPRINTK("Queue %p on port 1\n", q);
                return 1;
            }
        }

    return -1;  
}

EXPORT_SYMBOL( oxnassata_get_port_no );

int oxnassata_LBA_schemes_compatible( void ) {
    return  1;
}

struct ata_port* ox820sata_get_ap(int port_no)
{
    switch (port_no) {
    case 0:
    case 1:
        return ox820sata_driver.ap[port_no];
        break;
    default:
        return NULL;
        break;
    }
}

module_init(ox820sata_device_init);
module_exit(ox820sata_device_exit);

static struct proc_dir_entry* debug_proc;
static int debugproc_read(char *buf, char **start, off_t offset, int count,
    int *eof, void *unused) 
{
    int len = 0;
    CrazyDumpDebug();
    *eof=1;
    return len;
}

static int __init debugproc(void) {
    debug_proc = create_proc_entry("diskdump", 0444, 0);
    if (!debug_proc) {
        printk(KERN_ERR"unable to create proc entry for debug stuff\n");
        return -ENOMEM;
    }
    debug_proc->read_proc = debugproc_read;
    return 0;
}
module_init(debugproc);

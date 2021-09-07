 
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/bitops.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <mach/dma.h>
#include <mach/desc_alloc.h>
#ifdef CONFIG_SYNO_PLX_PORTING
#include <linux/sched.h>
#endif

#ifdef OXNAS_DMA_TEST
#define DMA_DEBUG
static void dma_test(unsigned long length);
#endif  

#ifdef OXNAS_DMA_SG_TEST
#define DMA_DEBUG
static void dma_sg_test(void);
#endif  

#ifdef OXNAS_DMA_SG_TEST_2
#define DMA_DEBUG
static void dma_sg_test2(void);
#endif  

#ifdef DMA_DEBUG
#define DBG(args...) printk(args)
#else
#define DBG(args...) do { } while(0)
#endif

#define DMA_REGS_PER_CHANNEL 8

#define DMA_CTRL_STATUS      0x0
#define DMA_BASE_SRC_ADR     0x4
#define DMA_BASE_DST_ADR     0x8
#define DMA_BYTE_CNT         0xC
#define DMA_CURRENT_SRC_ADR  0x10
#define DMA_CURRENT_DST_ADR  0x14
#define DMA_CURRENT_BYTE_CNT 0x18
#define DMA_INTR_ID          0x1C
#define DMA_INTR_CLEAR_REG   (DMA_CURRENT_SRC_ADR)

#define DMA_CTRL_STATUS_FAIR_SHARE_ARB            (1 << 0)
#define DMA_CTRL_STATUS_IN_PROGRESS               (1 << 1)
#define DMA_CTRL_STATUS_SRC_DREQ_MASK             (0x0000003C)
#define DMA_CTRL_STATUS_SRC_DREQ_SHIFT            2
#define DMA_CTRL_STATUS_DEST_DREQ_MASK            (0x000003C0)
#define DMA_CTRL_STATUS_DEST_DREQ_SHIFT           6
#define DMA_CTRL_STATUS_INTR                      (1 << 10)
#define DMA_CTRL_STATUS_NXT_FREE                  (1 << 11)
#define DMA_CTRL_STATUS_RESET                     (1 << 12)
#define DMA_CTRL_STATUS_DIR_MASK                  (0x00006000)
#define DMA_CTRL_STATUS_DIR_SHIFT                 13
#define DMA_CTRL_STATUS_SRC_ADR_MODE              (1 << 15)
#define DMA_CTRL_STATUS_DEST_ADR_MODE             (1 << 16)
#define DMA_CTRL_STATUS_TRANSFER_MODE_A           (1 << 17)
#define DMA_CTRL_STATUS_TRANSFER_MODE_B           (1 << 18)
#define DMA_CTRL_STATUS_SRC_WIDTH_MASK            (0x00380000)
#define DMA_CTRL_STATUS_SRC_WIDTH_SHIFT           19
#define DMA_CTRL_STATUS_DEST_WIDTH_MASK           (0x01C00000)
#define DMA_CTRL_STATUS_DEST_WIDTH_SHIFT          22
#define DMA_CTRL_STATUS_PAUSE                     (1 << 25)
#define DMA_CTRL_STATUS_INTERRUPT_ENABLE          (1 << 26)
#define DMA_CTRL_STATUS_SOURCE_ADDRESS_FIXED      (1 << 27)
#define DMA_CTRL_STATUS_DESTINATION_ADDRESS_FIXED (1 << 28)
#define DMA_CTRL_STATUS_STARVE_LOW_PRIORITY       (1 << 29)
#define DMA_CTRL_STATUS_INTR_CLEAR_ENABLE         (1 << 30)

#define DMA_BYTE_CNT_MASK                         ((1 << 21) - 1)
#define DMA_BYTE_CNT_INC4_SET_MASK                (1 << 28)
#define DMA_BYTE_CNT_HPROT_MASK                   (1 << 29)
#define DMA_BYTE_CNT_WR_EOT_MASK                  (1 << 30)
#define DMA_BYTE_CNT_RD_EOT_MASK                  (1 << 31)

#define DMA_INTR_ID_GET_NUM_CHANNELS(reg_contents) (((reg_contents) >> 16) & 0xFF)
#define DMA_INTR_ID_GET_VERSION(reg_contents)      (((reg_contents) >> 24) & 0xFF)
#define DMA_INTR_ID_INT_BIT         0
#define DMA_INTR_ID_INT_NUM_BITS    (MAX_OXNAS_DMA_CHANNELS)
#define DMA_INTR_ID_INT_MASK        (((1 << DMA_INTR_ID_INT_NUM_BITS) - 1) << DMA_INTR_ID_INT_BIT)

#define DMA_HAS_V4_INTR_CLEAR(version) ((version) > 3)

#define OXNAS_DMA_NUM_SG_REGS 4

#define DMA_SG_CONTROL  0x0
#define DMA_SG_STATUS   0x04
#define DMA_SG_REQ_PTR  0x08
#define DMA_SG_RESETS   0x0C

#define DMA_CALC_REG_ADR(channel, register) (DMA_BASE + ((channel) << 5) + (register))

#define DMA_SG_CALC_REG_ADR(channel, register) (DMA_SG_BASE + ((channel) << 4) + (register))

#ifdef CONFIG_OXNAS_ODRB_DMA_SUPPORT
#ifdef CONFIG_ARCH_OXNAS
 
#define RESERVED_ODRB_DMA_CHANNEL_NUMBER 4
#define ODRB_CALC_REG_ADR		DMA_CALC_REG_ADR
#define ORDB_SG_CALC_REG_ADR	DMA_SG_CALC_REG_ADR
#else  
 
#define RESERVED_ODRB_DMA_CHANNEL_NUMBER 0
#define SATADMA_REGS_BASE   (SATA_REG_BASE + 0xa0000)
#define SATASGDMA_REGS_BASE (SATA_REG_BASE + 0xb0000)
#define ODRB_CALC_REG_ADR(channel, register) (SATADMA_REGS_BASE + ((channel) << 5) + (register))
#define ORDB_SG_CALC_REG_ADR(channel, register) (SATASGDMA_REGS_BASE + ((channel) << 4) + (register))
#endif  
#endif  

#define DMA_SG_CONTROL_START_BIT            0
#define DMA_SG_CONTROL_QUEUING_ENABLE_BIT   1
#define DMA_SG_CONTROL_HBURST_ENABLE_BIT    2
#define DMA_SG_CONTROL_CLR_LAST_IRQ_BIT     4
#define DMA_SG_CONTROL_PRD_TABLE_BIT        5

#define DMA_SG_STATUS_ERROR_CODE_BIT        0
#define DMA_SG_STATUS_ERROR_CODE_NUM_BITS   6
#define DMA_SG_STATUS_BUSY_BIT              7

#define DMA_SG_RESETS_CONTROL_BIT 0
#define DMA_SG_RESETS_ARBITER_BIT 1
#define DMA_SG_RESETS_AHB_BIT	   2

#define OXNAS_DMA_SG_QUALIFIER_BIT      0
#define OXNAS_DMA_SG_QUALIFIER_NUM_BITS 16
#define OXNAS_DMA_SG_DST_EOT_BIT        16
#define OXNAS_DMA_SG_DST_EOT_NUM_BITS   2
#define OXNAS_DMA_SG_SRC_EOT_BIT        20
#define OXNAS_DMA_SG_SRC_EOT_NUM_BITS   2
#define OXNAS_DMA_SG_CHANNEL_BIT        24
#define OXNAS_DMA_SG_CHANNEL_NUM_BITS   8

#define OXNAS_DMA_ADR_MASK       ((1UL << (MEM_MAP_ALIAS_SHIFT)) - 1)

typedef enum oxnas_dma_transfer_bus
{
    OXNAS_DMA_SIDE_A,
    OXNAS_DMA_SIDE_B
} oxnas_dma_transfer_bus_t;

typedef enum oxnas_dma_transfer_direction
{
    OXNAS_DMA_A_TO_A,
    OXNAS_DMA_B_TO_A,
    OXNAS_DMA_A_TO_B,
    OXNAS_DMA_B_TO_B
} oxnas_dma_transfer_direction_t;

typedef enum oxnas_dma_transfer_width
{
    OXNAS_DMA_TRANSFER_WIDTH_8BITS,
    OXNAS_DMA_TRANSFER_WIDTH_16BITS,
    OXNAS_DMA_TRANSFER_WIDTH_32BITS
} oxnas_dma_transfer_width_t;

typedef enum oxnas_dma_transfer_mode
{
    OXNAS_DMA_TRANSFER_MODE_SINGLE,
    OXNAS_DMA_TRANSFER_MODE_BURST
} oxnas_dma_transfer_mode_t;

typedef enum oxnas_dma_dreq
{
    OXNAS_DMA_DREQ_SATA     = 0,
    OXNAS_DMA_DREQ_DPE_RX   = 1,
    OXNAS_DMA_DREQ_DPE_TX   = 2,
    OXNAS_DMA_DREQ_AUDIO_TX = 5,
    OXNAS_DMA_DREQ_AUDIO_RX = 6,    
    OXNAS_DMA_DREQ_MEMORY   = 15
} oxnas_dma_dreq_t;

oxnas_dma_device_settings_t oxnas_sata_dma_settings = {
#ifdef CONFIG_ARCH_OXNAS
    .address_              = SATA_DATA_BASE_PA,
#else  
    .address_              = 0,	 
#endif  
    .fifo_size_            = 16,
    .dreq_                 = OXNAS_DMA_DREQ_SATA,
    .read_eot_policy_      = OXNAS_DMA_EOT_FINAL,
    .write_eot_policy_     = OXNAS_DMA_EOT_FINAL,
#ifdef CONFIG_ARCH_OXNAS
    .bus_                  = OXNAS_DMA_SIDE_A,
#else  
    .bus_                  = OXNAS_DMA_SIDE_B,
#endif  
    .width_                = OXNAS_DMA_TRANSFER_WIDTH_32BITS,
    .transfer_mode_        = OXNAS_DMA_TRANSFER_MODE_BURST,
    .address_mode_         = OXNAS_DMA_MODE_FIXED,
    .address_really_fixed_ = 0
};

oxnas_dma_device_settings_t oxnas_ram_only_src_dma_settings = {
    .address_              = 0,
    .fifo_size_            = 0,
    .dreq_                 = OXNAS_DMA_DREQ_MEMORY,
    .read_eot_policy_      = OXNAS_DMA_EOT_FINAL,
    .write_eot_policy_     = OXNAS_DMA_EOT_NONE,
    .bus_                  = OXNAS_DMA_SIDE_A,       
    .width_                = OXNAS_DMA_TRANSFER_WIDTH_32BITS,
    .transfer_mode_        = OXNAS_DMA_TRANSFER_MODE_BURST,
    .address_mode_         = OXNAS_DMA_MODE_INC,
    .address_really_fixed_ = 1
};

oxnas_dma_device_settings_t oxnas_ram_generic_dma_settings = {
    .address_              = 0,
    .fifo_size_            = 0,
    .dreq_                 = OXNAS_DMA_DREQ_MEMORY,
    .read_eot_policy_      = OXNAS_DMA_EOT_NONE,
    .write_eot_policy_     = OXNAS_DMA_EOT_NONE,
    .bus_                  = OXNAS_DMA_SIDE_B,
    .width_                = OXNAS_DMA_TRANSFER_WIDTH_32BITS,
    .transfer_mode_        = OXNAS_DMA_TRANSFER_MODE_BURST,
    .address_mode_         = OXNAS_DMA_MODE_INC,
    .address_really_fixed_ = 1
};

#ifdef CONFIG_OXNAS_ODRB_DMA_SUPPORT
oxnas_dma_device_settings_t oxnas_ram_odrb_dma_settings = {
    .address_              = 0,
    .fifo_size_            = 0,
    .dreq_                 = OXNAS_DMA_DREQ_MEMORY,
    .read_eot_policy_      = OXNAS_DMA_EOT_NONE,
    .write_eot_policy_     = OXNAS_DMA_EOT_NONE,
#ifdef CONFIG_ARCH_OXNAS
    .bus_                  = OXNAS_DMA_SIDE_B,
#else  
    .bus_                  = OXNAS_DMA_SIDE_A,
#endif  
    .width_                = OXNAS_DMA_TRANSFER_WIDTH_32BITS,
    .transfer_mode_        = OXNAS_DMA_TRANSFER_MODE_BURST,
    .address_mode_         = OXNAS_DMA_MODE_INC,
    .address_really_fixed_ = 1
};
#endif  

static oxnas_dma_controller_t  dma_controller;

static oxnas_dma_sg_entry_t* alloc_sg_entry(int in_atomic)
{
	unsigned long flags;

    oxnas_dma_sg_entry_t* entry = 0;
    if (in_atomic) {
        if (down_trylock(&dma_controller.sg_entry_sem_)) {
            return (oxnas_dma_sg_entry_t*)0;
        }
    } else {
         
        while (down_interruptible(&dma_controller.sg_entry_sem_));
    }

    spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

    BUG_ON(!dma_controller.sg_entry_head_);

    entry = dma_controller.sg_entry_head_;
    dma_controller.sg_entry_head_ = dma_controller.sg_entry_head_->next_;
    --dma_controller.sg_entry_available_;

    spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);

    return entry;
}

static void free_sg_entry(oxnas_dma_sg_entry_t* entry)
{
	unsigned long flags;

	spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

	entry->next_ = dma_controller.sg_entry_head_;
	dma_controller.sg_entry_head_ = entry;
	++dma_controller.sg_entry_available_;

	spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);

	up(&dma_controller.sg_entry_sem_);
}

void oxnas_dma_free_sg_entries(oxnas_dma_sg_entry_t* entries)
{
	while (entries) {
		oxnas_dma_sg_entry_t* next = entries->next_;
		free_sg_entry(entries);
		entries = next;
	}
}

int oxnas_dma_alloc_sg_entries(
    oxnas_dma_sg_entry_t **entries,
    unsigned               required,
	int                    in_atomic)
{
	if (likely(required)) {
		oxnas_dma_sg_entry_t* prev;
		oxnas_dma_sg_entry_t* entry;
		unsigned acquired = 0;

		*entries = alloc_sg_entry(in_atomic);
		if (!*entries) {
			return 1;
		}

		(*entries)->next_ = 0;
		prev = *entries;

		while (++acquired < required) {
			entry = alloc_sg_entry(in_atomic);
			if (!entry) {
				 
				oxnas_dma_free_sg_entries(*entries);
				return 1;
			}
			entry->next_ = 0;
			prev->next_ = entry;
			prev = entry;
		}
	}

    return 0;
}

oxnas_dma_channel_t* oxnas_dma_request(int block)
{
    oxnas_dma_channel_t* channel = OXNAS_DMA_CHANNEL_NUL;
    while (channel == OXNAS_DMA_CHANNEL_NUL) {
        if (block) {
             
            if (down_interruptible(&dma_controller.channel_sem_)) {
                 
                continue;
            }
        } else {
             
            if (down_trylock(&dma_controller.channel_sem_)) {
                 
                break;
            }
        }

        spin_lock_bh(&dma_controller.channel_alloc_spinlock_);

        BUG_ON(!dma_controller.channel_head_);

        channel = dma_controller.channel_head_;
        dma_controller.channel_head_ = dma_controller.channel_head_->next_;

        spin_unlock_bh(&dma_controller.channel_alloc_spinlock_);
    }
    return channel;
}

void oxnas_dma_free(oxnas_dma_channel_t* channel)
{
    if (oxnas_dma_is_active(channel)) {
        printk(KERN_WARNING "oxnas_dma_free() Freeing channel %u while active\n", channel->channel_number_);
    }

    spin_lock_bh(&dma_controller.channel_alloc_spinlock_);

    channel->next_ = dma_controller.channel_head_;
    dma_controller.channel_head_ = channel;

    spin_unlock_bh(&dma_controller.channel_alloc_spinlock_);

    up(&dma_controller.channel_sem_);
}

#ifdef CONFIG_OXNAS_ODRB_DMA_SUPPORT
static unsigned long odrb_read_encoded_control_status;
static unsigned long odrb_read_encoded_eot;
static unsigned long odrb_read_sq_qualifier;
static unsigned long odrb_write_encoded_control_status;
static unsigned long odrb_write_encoded_eot;
static unsigned long odrb_write_sq_qualifier;

#ifdef CONFIG_ODRB_USE_PRDS_FOR_SATA
#else  
static odrb_sg_list_t ordb_sata_sg_list[CONFIG_ODRB_NUM_SATA_SG_LISTS];
#endif  

static odrb_sg_entry_t *odrb_sata_sq_entry;
static dma_addr_t       odrb_sata_sq_entry_phys;

#ifdef CONFIG_ODRB_USE_PRDS
prd_table_entry_t *odrb_sata_prd_entry;
dma_addr_t         odrb_sata_prd_entry_phys;
#endif  

static oxnas_dma_simple_sg_info_t *odrb_sq_info;
static dma_addr_t                  odrb_sq_info_phys;

#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
#ifdef CONFIG_ODRB_USE_PRDS
static odrb_prd_list_t ordb_prd_arrays[CONFIG_ODRB_NUM_WRITER_PRD_ARRAYS + CONFIG_ODRB_NUM_READER_PRD_ARRAYS];
#else  
static odrb_sg_list_t ordb_sg_list[CONFIG_ODRB_NUM_WRITER_SG_LISTS + CONFIG_ODRB_NUM_READER_SG_LISTS];
#endif  
#endif  

static unsigned long encode_control_status_ex(
    oxnas_dma_device_settings_t *src_settings,
    oxnas_dma_device_settings_t *dst_settings,
    int                          paused,
	int							 enable_interrupts);

static unsigned long encode_eot(
    oxnas_dma_device_settings_t* src_settings,
    oxnas_dma_device_settings_t* dst_settings,
    unsigned long length,
    int isFinalTransfer);

static unsigned long encode_length(
	unsigned long ctrl_status,
	unsigned long length)
{
	ctrl_status &= ~DMA_BYTE_CNT_MASK;
	return ctrl_status | length;
}

void odrb_dma_init(void)
{
	odrb_read_encoded_control_status = encode_control_status_ex(
		&oxnas_sata_dma_settings, &oxnas_ram_odrb_dma_settings, 0, 1),

	odrb_read_encoded_eot = encode_eot(
		&oxnas_sata_dma_settings, &oxnas_ram_odrb_dma_settings, 0, 1);

	odrb_read_sq_qualifier =
		((RESERVED_ODRB_DMA_CHANNEL_NUMBER << OXNAS_DMA_SG_CHANNEL_BIT) |
		 (oxnas_sata_dma_settings.read_eot_policy_ << OXNAS_DMA_SG_SRC_EOT_BIT) |
		 (oxnas_ram_odrb_dma_settings.write_eot_policy_ << OXNAS_DMA_SG_DST_EOT_BIT) |
		 (1 << OXNAS_DMA_SG_QUALIFIER_BIT));

	odrb_write_encoded_control_status = encode_control_status_ex(
		&oxnas_ram_odrb_dma_settings, &oxnas_sata_dma_settings, 0, 1),

	odrb_write_encoded_eot = encode_eot(
		&oxnas_ram_odrb_dma_settings, &oxnas_sata_dma_settings, 0, 1);

	odrb_write_sq_qualifier = 
		((RESERVED_ODRB_DMA_CHANNEL_NUMBER << OXNAS_DMA_SG_CHANNEL_BIT) |
		 (oxnas_ram_odrb_dma_settings.read_eot_policy_ << OXNAS_DMA_SG_SRC_EOT_BIT) |
		 (oxnas_sata_dma_settings.write_eot_policy_ << OXNAS_DMA_SG_DST_EOT_BIT) |
		 (1 << OXNAS_DMA_SG_QUALIFIER_BIT));
}

#define ODRB_CTRL_REG_ADR ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_CTRL_STATUS)
#define ODRB_SRC_REG_ADR  ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_BASE_SRC_ADR)
#define ODRB_DST_REG_ADR  ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_BASE_DST_ADR)
#define ODRB_CNT_REG_ADR  ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_BYTE_CNT)

void odrb_dma_sata_single(
	oxnas_dma_direction_t dir,
	dma_addr_t            adr,
	unsigned long         len)
{
	unsigned long length_eot =
		dir == OXNAS_DMA_TO_DEVICE ? encode_length(odrb_write_encoded_eot, len) :
									 encode_length(odrb_read_encoded_eot, len);

	BUG_ON(len > MAX_OXNAS_DMA_TRANSFER_LENGTH);

    writel(dir == OXNAS_DMA_TO_DEVICE ? odrb_write_encoded_control_status :
		odrb_read_encoded_control_status, ODRB_CTRL_REG_ADR);

    writel(dir == OXNAS_DMA_TO_DEVICE ? adr : oxnas_sata_dma_settings.address_,
		ODRB_SRC_REG_ADR);

    writel(dir == OXNAS_DMA_TO_DEVICE ? oxnas_sata_dma_settings.address_ : adr,
		ODRB_DST_REG_ADR);

    wmb();

    writel(length_eot, ODRB_CNT_REG_ADR);
}
EXPORT_SYMBOL(odrb_dma_sata_single);

#ifndef CONFIG_ODRB_USE_PRDS
int odrb_alloc_sg_list(
	odrb_sg_list_t **lists,
	int              count,
	int              may_sleep)
{
	unsigned long flags;
	int retval = 1;

	spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

	if (may_sleep) {
		DEFINE_WAIT(wait);

		for (;;) {
			prepare_to_wait(&dma_controller.odrb_sg_list_wait_queue_, &wait, TASK_UNINTERRUPTIBLE);
			if (likely(dma_controller.odrb_num_free_sg_lists_ >= count)) {
				smp_rmb();
				break;
			}
			spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);
			if (!schedule_timeout(HZ)) {
				printk(KERN_INFO "odrb_alloc_sg_list() A second has passed while waiting\n");
			}
			spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);
		}
		finish_wait(&dma_controller.odrb_sg_list_wait_queue_, &wait);
	}

	if (likely(dma_controller.odrb_num_free_sg_lists_ >= count)) {
		int i;
		for (i=0; i < count; i++) {
			lists[i] = list_entry(dma_controller.odrb_sg_list_head_.next,
				odrb_sg_list_t, head);
			list_del(&lists[i]->head);
		}
		dma_controller.odrb_num_free_sg_lists_ -= count;
		retval = 0;
	}

	spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);

	return retval;
}
EXPORT_SYMBOL(odrb_alloc_sg_list);

void odrb_free_sg_list(odrb_sg_list_t *sg_list)
{
	unsigned long flags;

    spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

	list_add(&sg_list->head, &dma_controller.odrb_sg_list_head_);
	++dma_controller.odrb_num_free_sg_lists_;
	wake_up(&dma_controller.odrb_sg_list_wait_queue_);

    spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);
}
EXPORT_SYMBOL(odrb_free_sg_list);

int odrb_reader_alloc_sg_list(
	odrb_sg_list_t **list)
{
	int           retval = 1;
	unsigned long flags = 0;

	spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);
	if (dma_controller.odrb_reader_num_free_sg_lists_ >= 1) {
		list[0] = list_entry(dma_controller.odrb_reader_sg_list_head_.next,
			odrb_sg_list_t, head);
		list_del(&list[0]->head);

		dma_controller.odrb_reader_num_free_sg_lists_ -= 1;
		retval = 0;
	}
	spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);

	return retval;
}
EXPORT_SYMBOL(odrb_reader_alloc_sg_list);

void odrb_reader_free_sg_list(odrb_sg_list_t *sg_list)
{
	unsigned long flags;

    spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

	list_add(&sg_list->head, &dma_controller.odrb_reader_sg_list_head_);
	++dma_controller.odrb_reader_num_free_sg_lists_;

    spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);
}
EXPORT_SYMBOL(odrb_reader_free_sg_list);

#endif  

#ifdef CONFIG_ODRB_NUM_SATA_SG_LISTS
int odrb_alloc_sata_sg_list(
	odrb_sg_list_t **lists,
	int                   count)
{
	int           retval = 1;
	unsigned long flags;

	spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);
	if (dma_controller.odrb_sata_num_free_sg_lists_ >= count) {
		int i;
		for (i=0; i < count; i++) {
			lists[i] = list_entry(dma_controller.odrb_sata_sg_list_head_.next,
				odrb_sg_list_t, head);
			list_del(&lists[i]->head);
		}
		dma_controller.odrb_sata_num_free_sg_lists_ -= count;
		retval = 0;
	}
	spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);

	return retval;
}
EXPORT_SYMBOL(odrb_alloc_sata_sg_list);

void odrb_free_sata_sg_list(odrb_sg_list_t *sg_list)
{
	unsigned long flags;

    spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

	list_add(&sg_list->head, &dma_controller.odrb_sata_sg_list_head_);
	++dma_controller.odrb_sata_num_free_sg_lists_;

    spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);
}
EXPORT_SYMBOL(odrb_free_sata_sg_list);
#endif  

void odrb_dump_dma_regs(void)
{
	oxnas_dma_dump_registers_single(RESERVED_ODRB_DMA_CHANNEL_NUMBER);
}
EXPORT_SYMBOL(odrb_dump_dma_regs);

void odrb_dma_dump_sg_regs(void)
{
	printk("DMA_SG_CONTROL = 0x%p\n", (void*)readl(ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_CONTROL)));
	printk("DMA_SG_STATUS  = 0x%p\n", (void*)readl(ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_STATUS)));
	printk("DMA_SG_REQ_PTR = 0x%p\n", (void*)readl(ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_REQ_PTR)));
	printk("DMA_SG_RESETS = 0x%p\n",  (void*)readl(ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_RESETS)));
}
EXPORT_SYMBOL(odrb_dma_dump_sg_regs);

void odrb_dump_sg_info(void)
{
	printk("odrb_sq_info virt = 0x%p, phys = 0x%p\n", odrb_sq_info, (void*)odrb_sq_info_phys);
	printk("  control = 0x%p, qualifier = 0x%p, src_entries = 0x%p, dst_entries = 0x%p\n", (void*)odrb_sq_info->control, (void*)odrb_sq_info->qualifier, (void*)odrb_sq_info->src_entries, (void*)odrb_sq_info->dst_entries);
}
EXPORT_SYMBOL(odrb_dump_sg_info);

void odrb_decode_sg_error(void) {
    u32 error = readl(
        ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_STATUS));
    if (error & 0x80) {
        printk(KERN_ERR"SGDMA Busy\n");
    }
    
    if (error & 0x7f) {
        switch (error & 0x30) {
        case 0x10:
            printk(KERN_ERR"SGDMA request qualifer not equal to 1\n");
            break;
        case 0x20:
            switch (error & 0x3) {
            case 0x1:
                printk(KERN_ERR"SGDMA destination pointer is NULL\n");
                break;
            case 0x2:
                printk(KERN_ERR"SGDMA source pointer is NULL\n");
                break;
            case 0x3:
                printk(KERN_ERR"SGDMA source and destination pointers are NULL\n");
                break;
            }
            break;
        case 0x30:
            switch (error & 0xf) {
            case  5:
            case  7:
            case 13:
                printk(KERN_ERR"SGDMA ran out of destination buffers\n");
                break;
            case 10:
            case 11:
            case 14:
                printk(KERN_ERR"SGDMA ran out of source buffers\n");
                break;
            }
            break;
        }
    }
}
EXPORT_SYMBOL(odrb_decode_sg_error);

static inline void _odrb_dma_sata_sq(
    oxnas_dma_direction_t dir,
	unsigned long         nsects,
	dma_addr_t            sg_phys,
	int                   legacy,
	int                   go)
{
	u32 sg_control;

	odrb_sata_sq_entry->length_ = nsects << SECTOR_SHIFT;

	if (dir == OXNAS_DMA_TO_DEVICE) {
		odrb_sq_info->control     = odrb_write_encoded_control_status;
		odrb_sq_info->qualifier   = odrb_write_sq_qualifier;
		odrb_sq_info->src_entries = sg_phys;
		odrb_sq_info->dst_entries = odrb_sata_sq_entry_phys;
	} else {
		odrb_sq_info->control     = odrb_read_encoded_control_status;
		odrb_sq_info->qualifier   = odrb_read_sq_qualifier;
		odrb_sq_info->src_entries = odrb_sata_sq_entry_phys;
		odrb_sq_info->dst_entries = sg_phys;
	}

	writel(1UL << DMA_SG_RESETS_CONTROL_BIT, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_RESETS));

	writel(odrb_sq_info_phys, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_REQ_PTR));

	sg_control = (go ? (1UL << DMA_SG_CONTROL_START_BIT) : 0) |
				 (1UL << DMA_SG_CONTROL_QUEUING_ENABLE_BIT) |
				 (1UL << DMA_SG_CONTROL_HBURST_ENABLE_BIT);

	 if (!legacy) {
		 sg_control |= (1UL << DMA_SG_CONTROL_CLR_LAST_IRQ_BIT);
	 }

	writel(sg_control, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_CONTROL));
}

void odrb_dma_sata_sq(
    oxnas_dma_direction_t dir,
	unsigned long         nsects,
	dma_addr_t            sg_phys,
	int                   legacy)
{
    _odrb_dma_sata_sq(dir, nsects, sg_phys, legacy, 1);
}
EXPORT_SYMBOL(odrb_dma_sata_sq);

void odrb_dma_sata_sq_nogo(
    oxnas_dma_direction_t dir,
	unsigned long         nsects,
	dma_addr_t            sg_phys)
{
    _odrb_dma_sata_sq(dir, nsects, sg_phys, 0, 0);
}
EXPORT_SYMBOL(odrb_dma_sata_sq_nogo);

inline void odrb_dma_sata_sq_go(void)
{
	u32 sg_control;
	sg_control = (1UL << DMA_SG_CONTROL_START_BIT) |
				 (1UL << DMA_SG_CONTROL_QUEUING_ENABLE_BIT) |
				 (1UL << DMA_SG_CONTROL_HBURST_ENABLE_BIT) |
				 (1UL << DMA_SG_CONTROL_CLR_LAST_IRQ_BIT) ;

	writel(sg_control, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_CONTROL));
}

EXPORT_SYMBOL(odrb_dma_sata_sq_go);

int odrb_dma_isactive(int is_sg)
{
	if (is_sg) {
		u32 status = readl(ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_STATUS));
		return status & (1UL << DMA_SG_STATUS_BUSY_BIT);
	} else {
		unsigned long ctrl_status = readl(ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_CTRL_STATUS));
		return ctrl_status & DMA_CTRL_STATUS_IN_PROGRESS;
	}
}
EXPORT_SYMBOL(odrb_dma_isactive);

void odrb_dma_abort(int is_sg)
{
     
    u32 ctrl_status = readl(ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_CTRL_STATUS));
    ctrl_status |= DMA_CTRL_STATUS_RESET;
    writel(ctrl_status, ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_CTRL_STATUS));

    while (readl(ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_CTRL_STATUS)) & DMA_CTRL_STATUS_IN_PROGRESS);

    ctrl_status = readl(ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_CTRL_STATUS));
    ctrl_status &= ~DMA_CTRL_STATUS_RESET;
    writel(ctrl_status, ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_CTRL_STATUS));
}
EXPORT_SYMBOL(odrb_dma_abort);

void odrb_dma_postop_housekeeping(int is_sg)
{
	 
	while (readl(ODRB_CALC_REG_ADR(0, DMA_INTR_ID)) & (1 << RESERVED_ODRB_DMA_CHANNEL_NUMBER)) {
		writel(0, ODRB_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_INTR_CLEAR_REG));
	}

	if (is_sg) {
		 
		writel(1, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_STATUS));
	}
}
EXPORT_SYMBOL(odrb_dma_postop_housekeeping);

#ifdef CONFIG_ODRB_USE_PRDS
static inline void _odrb_dma_sata_prd(
    oxnas_dma_direction_t dir,
	unsigned long         nsects,
	dma_addr_t            prds_phys,
	int                   legacy,
	int                   go)
{
	u32        sg_control;
	dma_addr_t sata_prds_phys = odrb_dma_prepare_sata_prd_table(nsects);

	if (dir == OXNAS_DMA_TO_DEVICE) {
		odrb_sq_info->control     = odrb_write_encoded_control_status;
		odrb_sq_info->qualifier   = odrb_write_sq_qualifier;
		odrb_sq_info->src_entries = prds_phys;
		odrb_sq_info->dst_entries = sata_prds_phys;
	} else {
		odrb_sq_info->control     = odrb_read_encoded_control_status;
		odrb_sq_info->qualifier   = odrb_read_sq_qualifier;
		odrb_sq_info->src_entries = sata_prds_phys;
		odrb_sq_info->dst_entries = prds_phys;
	}

	writel(1UL << DMA_SG_RESETS_CONTROL_BIT, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_RESETS));

	writel(odrb_sq_info_phys, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_REQ_PTR));

	sg_control = (go ? (1UL << DMA_SG_CONTROL_START_BIT) : 0) |
				 (1UL << DMA_SG_CONTROL_QUEUING_ENABLE_BIT) |
				 (1UL << DMA_SG_CONTROL_HBURST_ENABLE_BIT) |
				 (1UL << DMA_SG_CONTROL_PRD_TABLE_BIT);

	 if (!legacy) {
		 sg_control |= (1UL << DMA_SG_CONTROL_CLR_LAST_IRQ_BIT);
	 }

	writel(sg_control, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_CONTROL));
}

void odrb_dma_sata_prd(
    oxnas_dma_direction_t dir,
	unsigned long         nsects,
	dma_addr_t            prds_phys,
	int                   legacy)
{
    _odrb_dma_sata_prd(dir, nsects, prds_phys, legacy, 1);
}

EXPORT_SYMBOL(odrb_dma_sata_prd);

void odrb_dma_sata_prd_nogo(
    oxnas_dma_direction_t dir,
	unsigned long         nsects,
	dma_addr_t            prds_phys)
{
    _odrb_dma_sata_prd(dir, nsects, prds_phys, 0, 0);
}

EXPORT_SYMBOL(odrb_dma_sata_prd_nogo);

inline void odrb_dma_sata_prd_go(void)
{
	u32 sg_control;
	sg_control = (1UL << DMA_SG_CONTROL_START_BIT) |
				 (1UL << DMA_SG_CONTROL_QUEUING_ENABLE_BIT) |
				 (1UL << DMA_SG_CONTROL_HBURST_ENABLE_BIT) |
				 (1UL << DMA_SG_CONTROL_CLR_LAST_IRQ_BIT) |
				 (1UL << DMA_SG_CONTROL_PRD_TABLE_BIT);

	writel(sg_control, ORDB_SG_CALC_REG_ADR(RESERVED_ODRB_DMA_CHANNEL_NUMBER, DMA_SG_CONTROL));
}

EXPORT_SYMBOL(odrb_dma_sata_prd_go);

int __odrb_alloc_prd_array(
	odrb_prd_list_t **lists,
	int               count,
	int               may_sleep
#ifdef DEBUG_PRD_ALLOC
	,const char       *file,
	int               line
#endif  
	)
{
	unsigned long flags;
	int retval = 1;

	spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

	if (may_sleep) {
		DEFINE_WAIT(wait);

		for (;;) {
#ifdef DEBUG_PRD_ALLOC
			int timed_out = 0;
#endif  

			prepare_to_wait(&dma_controller.odrb_prd_array_wait_queue_, &wait, TASK_UNINTERRUPTIBLE);
			if (likely(dma_controller.odrb_num_free_prd_arrays_ >= count)) {
				smp_rmb();
				break;
			}
			spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);
			if (!schedule_timeout(HZ)) {
				printk(KERN_INFO "odrb_alloc_prd_array() A second has passed while waiting\n");
#ifdef DEBUG_PRD_ALLOC
				timed_out = 1;
#endif  
			}
			spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

#ifdef DEBUG_PRD_ALLOC
			if (unlikely(timed_out)) {
				struct list_head *p;
				odrb_prd_list_t  *prd;

				printk(KERN_INFO "PRD tables in use (jiffies %lu):\n", jiffies);
				list_for_each(p, &dma_controller.odrb_prd_inuse_array_head_) {
					prd = list_entry(p, odrb_prd_list_t, head);
					printk(KERN_INFO "\tfile %s, line %d, jiffies %lu\n",
						prd->file, prd->line, prd->jiffies);
				}
				printk(KERN_INFO "\n");
			}
#endif  
		}
		finish_wait(&dma_controller.odrb_prd_array_wait_queue_, &wait);
	}

	if (likely(dma_controller.odrb_num_free_prd_arrays_ >= count)) {
		int i;
		for (i=0; i < count; i++) {
			lists[i] = list_entry(dma_controller.odrb_prd_array_head_.next,
				odrb_prd_list_t, head);
			list_del(&lists[i]->head);
#ifdef DEBUG_PRD_ALLOC
			list_add(&lists[i]->head, &dma_controller.odrb_prd_inuse_array_head_);
			lists[i]->file = file;
			lists[i]->line = line;
			lists[i]->jiffies = jiffies;
#endif  
		}
		dma_controller.odrb_num_free_prd_arrays_ -= count;
		retval = 0;
	}

	spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);

	return retval;
}
EXPORT_SYMBOL(__odrb_alloc_prd_array);

void odrb_free_prd_array(odrb_prd_list_t *prd_list)
{
	unsigned long flags;

    spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

#ifdef DEBUG_PRD_ALLOC
    list_del(&prd_list->head);
#endif  
	list_add(&prd_list->head, &dma_controller.odrb_prd_array_head_);
#ifdef DEBUG_PRD_ALLOC
	prd_list->file = NULL;
	prd_list->line = 0;
	prd_list->jiffies = 0;
#endif  
	++dma_controller.odrb_num_free_prd_arrays_;
	wake_up(&dma_controller.odrb_prd_array_wait_queue_);

    spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);
}
EXPORT_SYMBOL(odrb_free_prd_array);

int odrb_reader_alloc_prd_array(
	odrb_prd_list_t **list)
{
	int           retval = 1;
	unsigned long flags;

	spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);
	if (dma_controller.odrb_reader_num_free_prd_arrays_ >= 1) {
		list[0] = list_entry(dma_controller.odrb_reader_prd_array_head_.next,
			odrb_prd_list_t, head);
		list_del(&list[0]->head);

		dma_controller.odrb_reader_num_free_prd_arrays_ -= 1;
		retval = 0;
	}
	spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);

	return retval;
}
EXPORT_SYMBOL(odrb_reader_alloc_prd_array);

void odrb_reader_free_prd_array(odrb_prd_list_t *prd_list)
{
	unsigned long flags;

    spin_lock_irqsave(&dma_controller.alloc_spinlock_, flags);

	list_add(&prd_list->head, &dma_controller.odrb_reader_prd_array_head_);
	++dma_controller.odrb_reader_num_free_prd_arrays_;

    spin_unlock_irqrestore(&dma_controller.alloc_spinlock_, flags);
}
EXPORT_SYMBOL(odrb_reader_free_prd_array);
#endif  
#endif  

static irqreturn_t oxnas_dma_interrupt(int irq, void *dev_id)
{
    oxnas_dma_channel_t *channel = 0;
    unsigned channel_number = 0;
	int need_bh = 0;

DBG("oxnas_dma_interrupt() from interrupt line %u\n", irq);

#ifdef CONFIG_OXNAS_ODRB_DMA_SUPPORT
if (irq == DMA_INTERRUPT_4) {
	printk(KERN_WARNING "ODRB DMA interrupt\n");
	odrb_dma_postop_housekeeping(1);
	return IRQ_HANDLED;
}
#endif  

	if (likely(irq == DMA_INTERRUPT_4)) {
		channel = &dma_controller.channels_[4];
	} else {
		channel = &dma_controller.channels_[irq - DMA_INTERRUPT_0];
	}
	channel_number = channel->channel_number_;
DBG("RPS interrupt %u from channel %u\n", irq, channel_number);

    channel->error_code_ = OXNAS_DMA_ERROR_CODE_NONE;

	need_bh = (channel->notification_callback_ != OXNAS_DMA_CALLBACK_NUL);

    while (readl(DMA_CALC_REG_ADR(0, DMA_INTR_ID)) & (1 << channel_number)) {
DBG("Ack'ing interrupt for channel %u\n", channel_number);
         
        writel(0, DMA_CALC_REG_ADR(channel_number, DMA_INTR_CLEAR_REG));

        atomic_inc(&channel->interrupt_count_);
    }
DBG("Left int ack'ing loop\n");

	if (channel->v_sg_info_->v_srcEntries_) {
		 
		u32 error_code = readl(DMA_SG_CALC_REG_ADR(channel_number, DMA_SG_STATUS));
		channel->error_code_ =
			((error_code >> DMA_SG_STATUS_ERROR_CODE_BIT) &
			 ((1UL << DMA_SG_STATUS_ERROR_CODE_NUM_BITS) - 1));

		 if (channel->auto_sg_entries_) {
			  
DBG("ISR channel %d is auto SG\n", channel->channel_number_);
			 need_bh = 1;
		 } else {
DBG("ISR channel %d not auto SG\n", channel->channel_number_);
			 
			channel->v_sg_info_->p_srcEntries_ = 0;
			channel->v_sg_info_->v_srcEntries_ = 0;
			channel->v_sg_info_->p_dstEntries_ = 0;
			channel->v_sg_info_->v_dstEntries_ = 0;
		 }

DBG("Return SG controller to idle, error_code = 0x%08x\n", error_code);
		 
		writel(1, DMA_SG_CALC_REG_ADR(channel_number, DMA_SG_STATUS));
	}

	if (likely(!need_bh)) {
DBG("ISR channel %d do not call bh\n", channel->channel_number_);
		atomic_set(&channel->interrupt_count_, 0);
		atomic_set(&channel->active_count_, 0);
	} else {
DBG("Marking channel %d as requiring its bottom half to run\n", channel_number);
		 
		set_bit(channel_number, (void*)&dma_controller.run_bh_);

DBG("Scheduling tasklet\n");
		 
		tasklet_schedule(&dma_controller.tasklet_);
	}

DBG("Returning\n");
    return IRQ_HANDLED;
}

static void dma_bh(unsigned long data)
{
     
    u32 run_bh = atomic_read(&dma_controller.run_bh_);
    while (run_bh) {
        unsigned i;

		u32 temp_run_bh = run_bh;
        for (i = 0; i < dma_controller.numberOfChannels_; i++, temp_run_bh >>= 1) {
            if (temp_run_bh & 1) {
                oxnas_dma_channel_t* channel = &dma_controller.channels_[i];
DBG("Bottom halve for channel %u\n", channel->channel_number_);
				if (channel->auto_sg_entries_) {
					 
					oxnas_dma_sg_entry_t* sg_entry = channel->v_sg_info_->v_srcEntries_;
DBG("Freeing SG resources for channel %d\n", channel->channel_number_);
					while (sg_entry) {
						oxnas_dma_sg_entry_t* next = sg_entry->v_next_;
						free_sg_entry(sg_entry);
						sg_entry = next;
					}

					sg_entry = channel->v_sg_info_->v_dstEntries_;
					while (sg_entry) {
						oxnas_dma_sg_entry_t* next = sg_entry->v_next_;
						free_sg_entry(sg_entry);
						sg_entry = next;
					}

					channel->v_sg_info_->p_srcEntries_ = 0;
					channel->v_sg_info_->v_srcEntries_ = 0;
					channel->v_sg_info_->p_dstEntries_ = 0;
					channel->v_sg_info_->v_dstEntries_ = 0;
				}
            }
        }

        atomic_sub(run_bh, &dma_controller.run_bh_);

        for (i = 0; i < dma_controller.numberOfChannels_; i++, run_bh >>= 1) {
            if (run_bh & 1) {
                int interrupt_count;
                oxnas_dma_channel_t* channel = &dma_controller.channels_[i];

                interrupt_count = atomic_read(&channel->interrupt_count_);
                atomic_sub(interrupt_count, &channel->interrupt_count_);

                if (atomic_read(&channel->active_count_)) {
                    atomic_dec(&channel->active_count_);
                }

                if (channel->notification_callback_ != OXNAS_DMA_CALLBACK_NUL) {
DBG("Notifying channel %u, %d outstanding interrupts\n", channel->channel_number_, interrupt_count);
                    (*channel->notification_callback_)(
						&dma_controller.channels_[i],
						channel->notification_arg_, channel->error_code_,
						interrupt_count);
                }
            }
        }

        run_bh = atomic_read(&dma_controller.run_bh_);
    }
}

void __init oxnas_dma_init()
{
    unsigned i;
    unsigned long intId;
    oxnas_dma_sg_info_t *v_info;
    dma_addr_t           p_info;

#ifdef CONFIG_ARCH_OX810
     
    writel(1UL << SYS_CTRL_RSTEN_DMA_BIT, SYS_CTRL_RSTEN_SET_CTRL);
    writel(1UL << SYS_CTRL_RSTEN_DMA_BIT, SYS_CTRL_RSTEN_CLR_CTRL);
#endif  

    writel(1UL << SYS_CTRL_RSTEN_SGDMA_BIT, SYS_CTRL_RSTEN_SET_CTRL);
    writel(1UL << SYS_CTRL_RSTEN_SGDMA_BIT, SYS_CTRL_RSTEN_CLR_CTRL);

    writel(1UL << SYS_CTRL_CKEN_DMA_BIT, SYS_CTRL_CKEN_SET_CTRL);

    atomic_set(&dma_controller.run_bh_, 0);
    spin_lock_init(&dma_controller.spinlock_);
    spin_lock_init(&dma_controller.alloc_spinlock_);
    spin_lock_init(&dma_controller.channel_alloc_spinlock_);
    sema_init(&dma_controller.csum_engine_sem_, 1);

    dma_controller.channel_head_ = 0;
    sema_init(&dma_controller.channel_sem_, 0);
     
    dma_controller.sg_entry_head_ = 0;
    sema_init(&dma_controller.sg_entry_sem_, 0);
    dma_controller.sg_entry_available_ = 0;

    tasklet_init(&dma_controller.tasklet_, dma_bh, 0);

    intId = readl(DMA_CALC_REG_ADR(0, DMA_INTR_ID));
    dma_controller.numberOfChannels_ = DMA_INTR_ID_GET_NUM_CHANNELS(intId);
    if (dma_controller.numberOfChannels_ > MAX_OXNAS_DMA_CHANNELS) {
        printk(KERN_WARNING "DMA: Too many DMA channels");
        dma_controller.numberOfChannels_ = MAX_OXNAS_DMA_CHANNELS;
    }

    dma_controller.version_ = DMA_INTR_ID_GET_VERSION(intId);
    printk(KERN_INFO "Number of DMA channels = %u, version = %u\n",
        dma_controller.numberOfChannels_, dma_controller.version_);

#ifdef CONFIG_OXNAS_ODRB_DMA_SUPPORT
	printk(KERN_INFO "Reserving a DMA channel for DirectRAID\n");
	--dma_controller.numberOfChannels_;
#endif  

    if (!DMA_HAS_V4_INTR_CLEAR(dma_controller.version_)) {
        panic("DMA: Trying to use v4+ interrupt clearing on DMAC version without support\n");
    }

#ifdef CONFIG_OXNAS_ODRB_DMA_SUPPORT
	{
		odrb_sg_entry_t   *temp = (odrb_sg_entry_t*)DMA_DESC_ALLOC_START;
		dma_addr_t         temp_phy = DMA_DESC_ALLOC_START_PA;
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
		int                count;
#ifdef CONFIG_ODRB_USE_PRDS
		prd_table_entry_t *prds;

		INIT_LIST_HEAD(&dma_controller.odrb_prd_array_head_);
#ifdef DEBUG_PRD_ALLOC
		INIT_LIST_HEAD(&dma_controller.odrb_prd_inuse_array_head_);
#endif  
		init_waitqueue_head(&dma_controller.odrb_prd_array_wait_queue_);
		dma_controller.odrb_num_free_prd_arrays_ = 0;

		INIT_LIST_HEAD(&dma_controller.odrb_reader_prd_array_head_);
		dma_controller.odrb_reader_num_free_prd_arrays_ = 0;  

		prds = (prd_table_entry_t*)DMA_DESC_ALLOC_START;
		temp_phy = DMA_DESC_ALLOC_START_PA;
		count = -1;
		while (++count < CONFIG_ODRB_NUM_WRITER_PRD_ARRAYS) {
			odrb_prd_list_t *prd_list = &ordb_prd_arrays[count];

			INIT_LIST_HEAD(&prd_list->head);
			prd_list->prds = prds;
			prd_list->phys = temp_phy;

			list_add_tail(&prd_list->head, &dma_controller.odrb_prd_array_head_);
			++dma_controller.odrb_num_free_prd_arrays_ ;

			prds     += CONFIG_ODRB_WRITER_PRD_ARRAY_SIZE;
			temp_phy += (sizeof(prd_table_entry_t) * CONFIG_ODRB_WRITER_PRD_ARRAY_SIZE);
		}

		while (count < CONFIG_ODRB_NUM_WRITER_PRD_ARRAYS + CONFIG_ODRB_NUM_READER_PRD_ARRAYS) {
			odrb_prd_list_t *prd_list = &ordb_prd_arrays[count];

			INIT_LIST_HEAD(&prd_list->head);
			prd_list->prds = prds;
			prd_list->phys = temp_phy;

			list_add_tail(&prd_list->head, &dma_controller.odrb_reader_prd_array_head_);
			++dma_controller.odrb_reader_num_free_prd_arrays_ ;

			prds     += CONFIG_ODRB_READER_PRD_ARRAY_SIZE;
			temp_phy += (sizeof(prd_table_entry_t) * CONFIG_ODRB_READER_PRD_ARRAY_SIZE);

			count ++;
		}

		temp = (odrb_sg_entry_t*)prds;
#else  
		 
		INIT_LIST_HEAD(&dma_controller.odrb_sg_list_head_);
		init_waitqueue_head(&dma_controller.odrb_sg_list_wait_queue_);
		dma_controller.odrb_num_free_sg_lists_ = 0;

		INIT_LIST_HEAD(&dma_controller.odrb_reader_sg_list_head_);
		dma_controller.odrb_reader_num_free_sg_lists_ = 0;  

		temp = (odrb_sg_entry_t*)DMA_DESC_ALLOC_START;
		temp_phy = DMA_DESC_ALLOC_START_PA;
		count = -1;
		while (++count < CONFIG_ODRB_NUM_WRITER_SG_LISTS) {
			odrb_sg_list_t *sg_list = &ordb_sg_list[count];

			INIT_LIST_HEAD(&sg_list->head);
			sg_list->sg_entries = temp;
			sg_list->phys = temp_phy;

			list_add_tail(&sg_list->head, &dma_controller.odrb_sg_list_head_);
			++dma_controller.odrb_num_free_sg_lists_ ;

			temp     += CONFIG_ODRB_NUM_WRITER_SG_ENTRIES;
			temp_phy += (sizeof(odrb_sg_entry_t) * CONFIG_ODRB_NUM_WRITER_SG_ENTRIES);
		}

		while (count < CONFIG_ODRB_NUM_WRITER_SG_LISTS + CONFIG_ODRB_NUM_READER_SG_LISTS) {
			odrb_sg_list_t *sg_list = &ordb_sg_list[count];

			INIT_LIST_HEAD(&sg_list->head);
			sg_list->sg_entries = temp;
			sg_list->phys = temp_phy;

			list_add_tail(&sg_list->head, &dma_controller.odrb_reader_sg_list_head_);
			++dma_controller.odrb_reader_num_free_sg_lists_ ;

			temp     += CONFIG_ODRB_NUM_READER_SG_ENTRIES;
			temp_phy += (sizeof(odrb_sg_entry_t) * CONFIG_ODRB_NUM_READER_SG_ENTRIES);

			count++;
		}
#endif  
#endif  

#ifdef CONFIG_ODRB_USE_PRDS_FOR_SATA
#else  
		 
		INIT_LIST_HEAD(&dma_controller.odrb_sata_sg_list_head_);
		dma_controller.odrb_sata_num_free_sg_lists_ = 0; 

		count = -1;
		while (++count < CONFIG_ODRB_NUM_SATA_SG_LISTS) {
			odrb_sg_list_t *sg_list = &ordb_sata_sg_list[count];

			INIT_LIST_HEAD(&sg_list->head);
			sg_list->sg_entries = temp;
			sg_list->phys = temp_phy;

			list_add_tail(&sg_list->head, &dma_controller.odrb_sata_sg_list_head_);
			++dma_controller.odrb_sata_num_free_sg_lists_ ;

			temp     += CONFIG_ODRB_NUM_SATA_SG_ENTRIES;
			temp_phy += (sizeof(odrb_sg_entry_t) * CONFIG_ODRB_NUM_SATA_SG_ENTRIES);
		}
#endif  

#ifdef CONFIG_ODRB_USE_PRDS
 
		odrb_sata_prd_entry = (prd_table_entry_t*)temp;
		odrb_sata_prd_entry_phys = temp_phy;

		for (i=0; i < ODRB_NUM_SATA_PRD_ENTRIES; ++i) {
			odrb_sata_prd_entry[i].adr = oxnas_sata_dma_settings.address_;
 
		}

		odrb_sata_sq_entry 
			= (odrb_sg_entry_t*)(odrb_sata_prd_entry + ODRB_NUM_SATA_PRD_ENTRIES);
 
		odrb_sata_sq_entry_phys = odrb_sata_prd_entry_phys +
			(ODRB_NUM_SATA_PRD_ENTRIES * sizeof(prd_table_entry_t));
 
#else  
		odrb_sata_sq_entry = (odrb_sg_entry_t*)temp;
		odrb_sata_sq_entry_phys = temp_phy;
#endif  

		odrb_sata_sq_entry->addr_ = oxnas_sata_dma_settings.address_;
		odrb_sata_sq_entry->next_ = 0;

		odrb_sq_info      = (oxnas_dma_simple_sg_info_t*)(odrb_sata_sq_entry + 1);
		odrb_sq_info_phys =
			odrb_sata_sq_entry_phys + sizeof(odrb_sg_entry_t);

		dma_controller.v_sg_infos_ = (oxnas_dma_sg_info_t*)(odrb_sq_info + 1);
		dma_controller.p_sg_infos_ =
			odrb_sq_info_phys + sizeof(oxnas_dma_simple_sg_info_t);
	}
#else  
    dma_controller.v_sg_infos_ = (oxnas_dma_sg_info_t*)DMA_DESC_ALLOC_START;
    dma_controller.p_sg_infos_ = DMA_DESC_ALLOC_START_PA;
#endif  

    if (!dma_controller.v_sg_infos_) {
        panic("DMA: Coherent alloc of SG info struct array");
    }

#ifdef CONFIG_OXNAS_ODRB_DMA_SUPPORT
#ifdef CONFIG_OXNAS_FAST_READS_AND_WRITES
	{
 
	}
#endif  
 
#endif  

    {
		 
		unsigned long alloc_start = (unsigned long)(dma_controller.v_sg_infos_);
		unsigned long alloc_start_pa = dma_controller.p_sg_infos_;

		unsigned long sg_info_alloc_size =
			(dma_controller.numberOfChannels_ * sizeof(oxnas_dma_sg_info_t));

		unsigned long available_size =
			DMA_DESC_ALLOC_SIZE - ((alloc_start - DMA_DESC_ALLOC_START) + sg_info_alloc_size);

        unsigned num_sg_entries = available_size / sizeof(oxnas_dma_sg_entry_t);

        oxnas_dma_sg_entry_t* entry_v =
			(oxnas_dma_sg_entry_t*)(alloc_start + sg_info_alloc_size);

		oxnas_dma_sg_entry_t* entry_p =
			(oxnas_dma_sg_entry_t*)(alloc_start_pa + sg_info_alloc_size);

		printk(KERN_INFO "Allocating %u SRAM generic DMA descriptors\n", num_sg_entries);
        for (i=0; i < num_sg_entries; ++i, ++entry_v, ++entry_p) {
            entry_v->paddr_ = (dma_addr_t)entry_p;
            free_sg_entry(entry_v);
        }
    }

    v_info = dma_controller.v_sg_infos_;
    p_info = dma_controller.p_sg_infos_;
    for (i=0; i < dma_controller.numberOfChannels_; i++) {
        oxnas_dma_channel_t *channel = &dma_controller.channels_[i];

        channel->channel_number_ = i;
        channel->notification_callback_ = OXNAS_DMA_CALLBACK_NUL;
        channel->notification_arg_ = OXNAS_DMA_CALLBACK_ARG_NUL;

        channel->v_sg_info_ = v_info++;
        channel->p_sg_info_ = p_info;
        p_info += sizeof(oxnas_dma_sg_info_t);

        channel->v_sg_info_->p_srcEntries_ = 0;
        channel->v_sg_info_->p_dstEntries_ = 0;
        channel->v_sg_info_->v_srcEntries_ = 0;
        channel->v_sg_info_->v_dstEntries_ = 0;

        channel->error_code_ = 0;

        atomic_set(&channel->interrupt_count_, 0);

        atomic_set(&channel->active_count_, 0);

        sema_init(&channel->default_semaphore_, 0);

        oxnas_dma_free(channel);
    }

    dma_controller.channels_[0].rps_interrupt_ = DMA_INTERRUPT_0;
    if (request_irq(DMA_INTERRUPT_0, &oxnas_dma_interrupt, 0, "DMA 0", 0)) {
        panic("DMA: Failed to allocate interrupt %u\n", DMA_INTERRUPT_0);
    }
    dma_controller.channels_[1].rps_interrupt_ = DMA_INTERRUPT_1;
    if (request_irq(DMA_INTERRUPT_1, &oxnas_dma_interrupt, 0, "DMA 1", 0)) {
        panic("DMA: Failed to allocate interrupt %u\n", DMA_INTERRUPT_1);
    }
    dma_controller.channels_[2].rps_interrupt_ = DMA_INTERRUPT_2;
    if (request_irq(DMA_INTERRUPT_2, &oxnas_dma_interrupt, 0, "DMA 2", 0)) {
        panic("DMA: Failed to allocate interrupt %u\n", DMA_INTERRUPT_2);
    }
    dma_controller.channels_[3].rps_interrupt_ = DMA_INTERRUPT_3;
    if (request_irq(DMA_INTERRUPT_3, &oxnas_dma_interrupt, 0, "DMA 3", 0)) {
        panic("DMA: Failed to allocate interrupt %u\n", DMA_INTERRUPT_3);
    }
#ifndef CONFIG_OXNAS_ODRB_DMA_SUPPORT
    dma_controller.channels_[4].rps_interrupt_ = DMA_INTERRUPT_4;
    if (request_irq(DMA_INTERRUPT_4, &oxnas_dma_interrupt, 0, "DMA 4", 0)) {
        panic("DMA: Failed to allocate interrupt %u\n", DMA_INTERRUPT_4);
    }
#endif  

#ifdef CONFIG_OXNAS_ODRB_DMA_SUPPORT
	odrb_dma_init();
#endif  

#ifdef OXNAS_DMA_OVERALL_TEST_LOOPS
    {
        int j;
        for (j=0; j < OXNAS_DMA_OVERALL_TEST_LOOPS; ++j) {
#ifdef OXNAS_DMA_TEST
            {
                int i;
                for (i=0; i < OXNAS_DMA_TEST_ITERATIONS; ++i) {
                    dma_test(512);
                }
            }
#endif  
#ifdef OXNAS_DMA_SG_TEST    
            {
                int i;
                for (i=0; i < OXNAS_DMA_SG_TEST_ITERATIONS; ++i) {
                    dma_sg_test();
                }
            }
#endif  
#ifdef OXNAS_DMA_SG_TEST_2    
            {
                int i;
                for (i=0; i < OXNAS_DMA_SG_TEST_ITERATIONS; ++i) {
                    dma_sg_test2();
                }
            }
#endif  
#ifdef OXNAS_DMA_TEST
            {
                int i;
                for (i=0; i < OXNAS_DMA_TEST_AFTER_SG_ITERATIONS; ++i) {
                    dma_test(512);
                }
            }
#endif  
        }
    }
#endif  
}

void oxnas_dma_shutdown()
{
    dma_controller.sg_entry_head_ = 0;
}

int oxnas_dma_is_active(oxnas_dma_channel_t* channel)
{
    return atomic_read(&channel->active_count_);
}

int oxnas_dma_raw_isactive(oxnas_dma_channel_t* channel)
{
    unsigned long ctrl_status = readl(DMA_CALC_REG_ADR(channel->channel_number_, DMA_CTRL_STATUS));
    return ctrl_status & DMA_CTRL_STATUS_IN_PROGRESS;
}

int oxnas_dma_raw_sg_isactive(oxnas_dma_channel_t* channel)
{
     
    u32 status = readl(DMA_SG_CALC_REG_ADR(channel->channel_number_, DMA_SG_STATUS));
    return status & (1UL << DMA_SG_STATUS_BUSY_BIT);
}

int oxnas_dma_get_raw_direction(oxnas_dma_channel_t* channel)
{
    unsigned long ctrl_status = readl(DMA_CALC_REG_ADR(channel->channel_number_, DMA_CTRL_STATUS));
    return (ctrl_status & DMA_CTRL_STATUS_DIR_MASK) >> DMA_CTRL_STATUS_DIR_SHIFT;
}

static unsigned long encode_control_status_ex(
    oxnas_dma_device_settings_t *src_settings,
    oxnas_dma_device_settings_t *dst_settings,
    int                          paused,
	int							 enable_interrupts)
{
    unsigned long ctrl_status;
    oxnas_dma_transfer_direction_t direction;

    ctrl_status  = paused ? DMA_CTRL_STATUS_PAUSE : 0;							 
	if (enable_interrupts) {
		ctrl_status |= DMA_CTRL_STATUS_INTERRUPT_ENABLE;
	}
    ctrl_status |= (DMA_CTRL_STATUS_FAIR_SHARE_ARB   |							 
					DMA_CTRL_STATUS_INTR_CLEAR_ENABLE);						 
    ctrl_status |= (src_settings->dreq_ << DMA_CTRL_STATUS_SRC_DREQ_SHIFT);	 
    ctrl_status |= (dst_settings->dreq_ << DMA_CTRL_STATUS_DEST_DREQ_SHIFT);	 

    if (src_settings->bus_ == OXNAS_DMA_SIDE_A) {
         
        if (src_settings->transfer_mode_ == OXNAS_DMA_TRANSFER_MODE_BURST) {
            ctrl_status |= DMA_CTRL_STATUS_TRANSFER_MODE_A;
        } else {
            ctrl_status &= ~DMA_CTRL_STATUS_TRANSFER_MODE_A;
        }

        if (dst_settings->bus_ == OXNAS_DMA_SIDE_A) {
            direction = OXNAS_DMA_A_TO_A;
        } else {
            direction = OXNAS_DMA_A_TO_B;

            if (dst_settings->transfer_mode_ == OXNAS_DMA_TRANSFER_MODE_BURST) {
                ctrl_status |= DMA_CTRL_STATUS_TRANSFER_MODE_B;
            } else {
                ctrl_status &= ~DMA_CTRL_STATUS_TRANSFER_MODE_B;
            }
        }
    } else {
         
        if (src_settings->transfer_mode_ == OXNAS_DMA_TRANSFER_MODE_BURST) {
            ctrl_status |= DMA_CTRL_STATUS_TRANSFER_MODE_B;
        } else {
            ctrl_status &= ~DMA_CTRL_STATUS_TRANSFER_MODE_B;
        }

        if (dst_settings->bus_ == OXNAS_DMA_SIDE_A) {
            direction = OXNAS_DMA_B_TO_A;

            if (dst_settings->transfer_mode_ == OXNAS_DMA_TRANSFER_MODE_BURST) {
                ctrl_status |= DMA_CTRL_STATUS_TRANSFER_MODE_A;
            } else {
                ctrl_status &= ~DMA_CTRL_STATUS_TRANSFER_MODE_A;
            }
        } else {
            direction = OXNAS_DMA_B_TO_B;
        }
    }
    ctrl_status |= (direction << DMA_CTRL_STATUS_DIR_SHIFT);

    if (src_settings->address_mode_ == OXNAS_DMA_MODE_FIXED) {
         
        ctrl_status &= ~(DMA_CTRL_STATUS_SRC_ADR_MODE);

        if (src_settings->address_really_fixed_) {
            ctrl_status |= DMA_CTRL_STATUS_SOURCE_ADDRESS_FIXED;
        } else {
            ctrl_status &= ~DMA_CTRL_STATUS_SOURCE_ADDRESS_FIXED;
        }
    } else {
         
        ctrl_status |= DMA_CTRL_STATUS_SRC_ADR_MODE;
        ctrl_status &= ~DMA_CTRL_STATUS_SOURCE_ADDRESS_FIXED;
    }

    if (dst_settings->address_mode_ == OXNAS_DMA_MODE_FIXED) {
         
        ctrl_status &= ~(DMA_CTRL_STATUS_DEST_ADR_MODE);
        
        if (dst_settings->address_really_fixed_) {
            ctrl_status |= DMA_CTRL_STATUS_DESTINATION_ADDRESS_FIXED;
        } else {
            ctrl_status &= ~DMA_CTRL_STATUS_DESTINATION_ADDRESS_FIXED;
        }
    } else {
         
        ctrl_status |= DMA_CTRL_STATUS_DEST_ADR_MODE;
        ctrl_status &= ~DMA_CTRL_STATUS_DESTINATION_ADDRESS_FIXED;
    }

    ctrl_status |= (src_settings->width_ << DMA_CTRL_STATUS_SRC_WIDTH_SHIFT);
    ctrl_status |= (dst_settings->width_ << DMA_CTRL_STATUS_DEST_WIDTH_SHIFT);

    ctrl_status &= ~DMA_CTRL_STATUS_STARVE_LOW_PRIORITY;     

    return ctrl_status;
}

static inline unsigned long encode_control_status(
    oxnas_dma_device_settings_t *src_settings,
    oxnas_dma_device_settings_t *dst_settings,
    int                          paused)
{
	return encode_control_status_ex(src_settings, dst_settings, paused, 1);
}

static unsigned long encode_eot(
    oxnas_dma_device_settings_t* src_settings,
    oxnas_dma_device_settings_t* dst_settings,
    unsigned long length,
    int isFinalTransfer)
{
     
    unsigned long encoded = length |
                            DMA_BYTE_CNT_INC4_SET_MASK |     
                            DMA_BYTE_CNT_HPROT_MASK;         

    encoded &= ~DMA_BYTE_CNT_RD_EOT_MASK;
    switch (src_settings->read_eot_policy_) {
        case OXNAS_DMA_EOT_FINAL:
            if (!isFinalTransfer) {
                break;
            }
             
        case OXNAS_DMA_EOT_ALL:
            encoded |= DMA_BYTE_CNT_RD_EOT_MASK;
            break;
        default:
            break;
    }

    encoded &= ~DMA_BYTE_CNT_WR_EOT_MASK;
    switch (dst_settings->write_eot_policy_) {
        case OXNAS_DMA_EOT_FINAL:
            if (!isFinalTransfer) {
                break;
            }
             
        case OXNAS_DMA_EOT_ALL:
            encoded |= DMA_BYTE_CNT_WR_EOT_MASK;
            break;
        default:
            break;
    }

    return encoded;
}

static unsigned long encode_start(unsigned long ctrl_status)
{
    ctrl_status &= ~DMA_CTRL_STATUS_PAUSE;
    return ctrl_status;
}

static void oxnas_dma_set_common_lowlevel(
    oxnas_dma_channel_t *channel,
    unsigned long        ctrl_status,
    dma_addr_t           src_address,
    dma_addr_t           dst_address,
    unsigned long        lengthAndEOT)
{
    unsigned channel_number = channel->channel_number_;

    spin_lock(&dma_controller.spinlock_);

    writel(ctrl_status, DMA_CALC_REG_ADR(channel_number, DMA_CTRL_STATUS));

    wmb();

    writel(src_address, DMA_CALC_REG_ADR(channel_number, DMA_BASE_SRC_ADR));

    writel(dst_address, DMA_CALC_REG_ADR(channel_number, DMA_BASE_DST_ADR));

    writel(lengthAndEOT, DMA_CALC_REG_ADR(channel_number, DMA_BYTE_CNT));

    wmb();

    spin_unlock(&dma_controller.spinlock_);

    atomic_inc(&channel->active_count_);
}

static int oxnas_dma_set_common(
    oxnas_dma_channel_t*         channel,
    unsigned long                length,
    oxnas_dma_device_settings_t *src_settings,
    oxnas_dma_device_settings_t *dst_settings,
    int                          isFinalTransfer,
    int                          paused)
{
    int status = 0;

    if (length > MAX_OXNAS_DMA_TRANSFER_LENGTH) {
        printk(KERN_WARNING "oxnas_dma_set_common() length exceeds hardware allowed maximum\n");
        status = 1;
    } else {
        oxnas_dma_set_common_lowlevel(
            channel,
            encode_control_status(src_settings, dst_settings, paused),
            (dma_addr_t)src_settings->address_,
            (dma_addr_t)dst_settings->address_,
            encode_eot(src_settings, dst_settings, length, isFinalTransfer));
    }
    return status;
}

int oxnas_dma_set(
    oxnas_dma_channel_t *channel,
    unsigned char       *src_adr,    
    unsigned long        length,
    unsigned char       *dst_adr,    
    oxnas_dma_mode_t     src_mode,
    oxnas_dma_mode_t     dst_mode,
    int                  paused)
{
	 
	oxnas_dma_device_settings_t src_settings = oxnas_ram_only_src_dma_settings;
	oxnas_dma_device_settings_t dst_settings = oxnas_ram_generic_dma_settings;

    if (oxnas_dma_is_active(channel)) {
        printk(KERN_WARNING "oxnas_dma_set() Trying to use channel %u while active\n", channel->channel_number_);
    }

	src_settings.address_ = (unsigned long)src_adr;

	src_settings.address_ &= OXNAS_DMA_ADR_MASK;
	src_settings.address_mode_ = src_mode;

	dst_settings.address_ = ((unsigned long)dst_adr) & OXNAS_DMA_ADR_MASK;
	dst_settings.address_mode_ = dst_mode;

	return oxnas_dma_set_common(channel, length, &src_settings, &dst_settings, 1, paused);
}

int oxnas_dma_device_set(
    oxnas_dma_channel_t         *channel,
    oxnas_dma_direction_t        direction,
    unsigned char               *mem_adr,    
    unsigned long                length,
    oxnas_dma_device_settings_t *device_settings,
    oxnas_dma_mode_t             mem_mode,
    int                          paused)
{
    oxnas_dma_device_settings_t mem_settings;

    if (oxnas_dma_is_active(channel)) {
        printk(KERN_WARNING "oxnas_dma_device_set() Trying to use channel %u while active\n", channel->channel_number_);
    }

    mem_settings = oxnas_ram_generic_dma_settings;
    mem_settings.address_ = ((unsigned long)mem_adr) & OXNAS_DMA_ADR_MASK;
    mem_settings.address_mode_ = mem_mode;

    device_settings->address_ &= OXNAS_DMA_ADR_MASK;

    return oxnas_dma_set_common(
        channel,
        length,
        (direction == OXNAS_DMA_TO_DEVICE)   ? &mem_settings : device_settings,
        (direction == OXNAS_DMA_FROM_DEVICE) ? &mem_settings : device_settings,
        1,
        paused);
}

int oxnas_dma_device_pair_set(
    oxnas_dma_channel_t*         channel,
    unsigned long                length,
    oxnas_dma_device_settings_t *src_device_settings,
    oxnas_dma_device_settings_t *dst_device_settings,
    int                          paused)
{
    if (oxnas_dma_is_active(channel)) {
        printk(KERN_WARNING "oxnas_dma_device_pair_set() Trying to use channel %u while active\n", channel->channel_number_);
    }

    src_device_settings->address_ &= OXNAS_DMA_ADR_MASK;
    dst_device_settings->address_ &= OXNAS_DMA_ADR_MASK;
    return oxnas_dma_set_common(channel, length, src_device_settings, dst_device_settings, 1, paused);
}

static int oxnas_dma_set_sg_common(
    oxnas_dma_channel_t*         channel,
    struct scatterlist*          src_sg,
    unsigned                     src_sg_count,
    struct scatterlist*          dst_sg,
    unsigned                     dst_sg_count,
    oxnas_dma_device_settings_t* src_settings,
    oxnas_dma_device_settings_t* dst_settings,
	int                          in_atomic)
{
    int i;
    int failed = 0;
    oxnas_dma_sg_entry_t *sg_entry;
    oxnas_dma_sg_entry_t *previous_entry;

    oxnas_dma_sg_info_t *sg_info = channel->v_sg_info_;

	channel->auto_sg_entries_ = 1;

    sg_info->v_srcEntries_ = 0;
    sg_info->p_srcEntries_ = 0;
    sg_info->v_dstEntries_ = 0;
    sg_info->p_dstEntries_ = 0;

    sg_entry = 0;
    previous_entry = 0;
    for (i=0; i < src_sg_count; i++) {
         
#if 0
        if (previous_entry &&
            ((previous_entry->addr_ + previous_entry->length_) == (src_sg[i].dma_address & OXNAS_DMA_ADR_MASK)) &&
            ((previous_entry->length_ + src_sg[i].length) <= MAX_OXNAS_DMA_TRANSFER_LENGTH)) {
             
            previous_entry->length_ += src_sg[i].length;
        } else
#endif
        {
             
            oxnas_dma_sg_entry_t *new_sg_entry = alloc_sg_entry(in_atomic);
            if (!new_sg_entry) {
                failed = 1;
                break;
            }
            sg_entry = new_sg_entry;

            if (previous_entry) {
                 
                previous_entry->v_next_ = sg_entry;
                previous_entry->p_next_ = sg_entry->paddr_;
            } else {
                 
                sg_info->v_srcEntries_ = sg_entry;
                sg_info->p_srcEntries_ = sg_entry->paddr_;
            }
            previous_entry = sg_entry;

            sg_entry->addr_ = src_sg[i].dma_address & OXNAS_DMA_ADR_MASK;

            sg_entry->length_ = (src_sg[i].length <= MAX_OXNAS_DMA_TRANSFER_LENGTH) ? src_sg[i].length : 0;
            if (!sg_entry->length_) {
                printk(KERN_WARNING "oxnas_dma_set_sg_common() Source entry too long, zeroing\n");
            }
        }
    }
    if (sg_entry) {
         
        sg_entry->p_next_ = 0;
        sg_entry->v_next_ = 0;
    }

    if (failed) {
         
        oxnas_dma_sg_entry_t* sg_entry = sg_info->v_srcEntries_;
        while (sg_entry) {
            oxnas_dma_sg_entry_t* next = sg_entry->v_next_;
            free_sg_entry(sg_entry);
            sg_entry = next;
        }
        channel->v_sg_info_->p_srcEntries_ = 0;
        channel->v_sg_info_->v_srcEntries_ = 0;
        return 1;
    }

    sg_entry = 0;
    previous_entry = 0;
    for (i=0; i < dst_sg_count; i++) {
         
#if 0
        if (previous_entry &&
            ((previous_entry->addr_ + previous_entry->length_) == (dst_sg[i].dma_address & OXNAS_DMA_ADR_MASK)) &&
            ((previous_entry->length_ + dst_sg[i].length) <= MAX_OXNAS_DMA_TRANSFER_LENGTH)) {
             
            previous_entry->length_ += dst_sg[i].length;
        } else 
#endif
        {
             
            oxnas_dma_sg_entry_t *new_sg_entry = alloc_sg_entry(in_atomic);
            if (!new_sg_entry) {
                failed = 1;
                break;
            }
            sg_entry = new_sg_entry;

            if (previous_entry) {
                 
                previous_entry->v_next_ = sg_entry;
                previous_entry->p_next_ = sg_entry->paddr_;
            } else {
                 
                sg_info->v_dstEntries_ = sg_entry;
                sg_info->p_dstEntries_ = sg_entry->paddr_;
            }
            previous_entry = sg_entry;

            sg_entry->addr_   = dst_sg[i].dma_address & OXNAS_DMA_ADR_MASK;

            sg_entry->length_ = (dst_sg[i].length <= MAX_OXNAS_DMA_TRANSFER_LENGTH) ? dst_sg[i].length : 0;
            if (!sg_entry->length_) {
                printk(KERN_WARNING "oxnas_dma_set_sg_common() Destination entry too long, zeroing\n");
            }
        }
    }
    if (sg_entry) {
         
        sg_entry->p_next_ = 0;
        sg_entry->v_next_ = 0;
    }

    if (failed) {
         
        oxnas_dma_sg_entry_t* sg_entry = sg_info->v_dstEntries_;
        while (sg_entry) {
            oxnas_dma_sg_entry_t* next = sg_entry->v_next_;
            free_sg_entry(sg_entry);
            sg_entry = next;
        }
        sg_info->p_dstEntries_ = 0;
        sg_info->v_dstEntries_ = 0;

        sg_entry = sg_info->v_srcEntries_;
        while (sg_entry) {
            oxnas_dma_sg_entry_t* next = sg_entry->v_next_;
            free_sg_entry(sg_entry);
            sg_entry = next;
        }
        sg_info->p_srcEntries_ = 0;
        sg_info->v_srcEntries_ = 0;
        return 1;
    }

    sg_info->qualifer_ = ((channel->channel_number_ << OXNAS_DMA_SG_CHANNEL_BIT) |
                          (src_settings->read_eot_policy_ << OXNAS_DMA_SG_SRC_EOT_BIT) |
                          (dst_settings->write_eot_policy_ << OXNAS_DMA_SG_DST_EOT_BIT) |
                          (1 << OXNAS_DMA_SG_QUALIFIER_BIT));

    sg_info->control_ = encode_control_status(src_settings, dst_settings, 0);

    atomic_inc(&channel->active_count_);

    return 0;
}

int oxnas_dma_set_sg(
    oxnas_dma_channel_t* channel,
    struct scatterlist*  src_sg,
    unsigned             src_sg_count,
    struct scatterlist*  dst_sg,
    unsigned             dst_sg_count,
    oxnas_dma_mode_t     src_mode,
    oxnas_dma_mode_t     dst_mode,
	int                  in_atomic)
{
	 
	oxnas_dma_device_settings_t src_settings = oxnas_ram_only_src_dma_settings;
	oxnas_dma_device_settings_t dst_settings = oxnas_ram_generic_dma_settings;

    if (oxnas_dma_is_active(channel)) {
        printk(KERN_WARNING "oxnas_dma_set_sg() Trying to use channel %u while active\n", channel->channel_number_);
    }

	src_settings.address_ = 0;
	src_settings.address_mode_ = src_mode;

	dst_settings.address_ = 0;
	dst_settings.address_mode_ = dst_mode;

	return oxnas_dma_set_sg_common(channel, src_sg, src_sg_count, dst_sg,
		dst_sg_count, &src_settings, &dst_settings, in_atomic);
}

int oxnas_dma_device_set_sg(
    oxnas_dma_channel_t*         channel,
    oxnas_dma_direction_t        direction,
    struct scatterlist*          mem_sg,
    unsigned                     mem_sg_count,
    oxnas_dma_device_settings_t* device_settings,
    oxnas_dma_mode_t             mem_mode,
	int                          in_atomic)
{
    int i;
    struct scatterlist *sg;
    struct scatterlist  dev_sg;

    oxnas_dma_device_settings_t mem_settings;

    if (oxnas_dma_is_active(channel)) {
        printk(KERN_WARNING "oxnas_dma_device_set_sg() Trying to use channel %u while active\n", channel->channel_number_);
    }

    mem_settings = oxnas_ram_generic_dma_settings;
    mem_settings.address_ = 0;   
    mem_settings.address_mode_ = mem_mode;

    dev_sg.dma_address = device_settings->address_;
    for (i=0, sg=mem_sg, dev_sg.length = 0; i < mem_sg_count; i++, sg++) {
        dev_sg.length += sg->length;
    }
 
    return oxnas_dma_set_sg_common(
        channel,
        (direction == OXNAS_DMA_TO_DEVICE)   ? mem_sg        : &dev_sg,
        (direction == OXNAS_DMA_TO_DEVICE)   ? mem_sg_count  : 1,
        (direction == OXNAS_DMA_FROM_DEVICE) ? mem_sg        : &dev_sg,
        (direction == OXNAS_DMA_FROM_DEVICE) ? mem_sg_count  : 1,
        (direction == OXNAS_DMA_TO_DEVICE)   ? &mem_settings : device_settings,
        (direction == OXNAS_DMA_FROM_DEVICE) ? &mem_settings : device_settings,
		in_atomic);
}
EXPORT_SYMBOL(oxnas_dma_device_set_sg);

static int oxnas_dma_set_prd_common(
    oxnas_dma_channel_t         *channel,
    struct ata_prd              *src_prd,
    struct ata_prd              *dst_prd,
    oxnas_dma_device_settings_t *src_settings,
    oxnas_dma_device_settings_t *dst_settings,
	oxnas_dma_sg_entry_t		 *sg_entries)
{
    int i;
    int failed = 0;
    oxnas_dma_sg_entry_t *sg_entry, *previous_entry, *next_entry;
    u32 eot;
	u32 tot_src_len = 0, tot_dst_len = 0;

    oxnas_dma_sg_info_t *sg_info = channel->v_sg_info_;

	channel->auto_sg_entries_ = 0;

    sg_info->v_srcEntries_ = 0;
    sg_info->p_srcEntries_ = 0;
    sg_info->v_dstEntries_ = 0;
    sg_info->p_dstEntries_ = 0;

    sg_entry = previous_entry = 0;
    next_entry = sg_entries;
    i=0;
    do {
        u32 addr;
        u32 length;
        u32 flags_len;

        addr = src_prd[i].addr;
        flags_len = le32_to_cpu(src_prd[i++].flags_len);
        length = flags_len & ~ATA_PRD_EOT;
        eot = flags_len & ATA_PRD_EOT;

        if (!length) length = 0x10000;

		tot_src_len += length;

#if 0
        if (previous_entry &&
            ((previous_entry->addr_ + previous_entry->length_) == (addr & OXNAS_DMA_ADR_MASK)) &&
            ((previous_entry->length_ + length) <= MAX_OXNAS_DMA_TRANSFER_LENGTH)) {
             
            previous_entry->length_ += length;
        } else 
#endif
        {
			 
			if (!next_entry) {
				failed = 1;
				break;
			}
			sg_entry = next_entry;

            if (previous_entry) {
                 
                previous_entry->v_next_ = sg_entry;
                previous_entry->p_next_ = sg_entry->paddr_;
            } else {
                 
                sg_info->v_srcEntries_ = sg_entry;
                sg_info->p_srcEntries_ = sg_entry->paddr_;
            }
            previous_entry = sg_entry;

            sg_entry->addr_ = addr & OXNAS_DMA_ADR_MASK;

            if (length > MAX_OXNAS_DMA_TRANSFER_LENGTH) {
                printk(KERN_WARNING "oxnas_dma_set_prd_common() Source entry too long (0x%x), zeroing\n", length);
                sg_entry->length_ = 0;
            } else {
                sg_entry->length_ = length;
            }

			next_entry = sg_entry->next_;
        }
    } while (!eot);
    if (sg_entry) {
         
        sg_entry->p_next_ = 0;
        sg_entry->v_next_ = 0;
    }

    if (failed) {
         
        channel->v_sg_info_->p_srcEntries_ = 0;
        channel->v_sg_info_->v_srcEntries_ = 0;
		printk(KERN_WARNING "Too few SG entries to satisfy source requirements\n");
        return 1;
    }

    sg_entry = previous_entry = 0;
    i=0;
    do {
        u32 addr;
        u32 length;
        u32 flags_len;

        addr = dst_prd[i].addr;
        flags_len = le32_to_cpu(dst_prd[i++].flags_len);
        length = flags_len & ~ATA_PRD_EOT;
        eot = flags_len & ATA_PRD_EOT;

        if (!length) length = 0x10000;

		tot_dst_len += length;

#if 0
        if (previous_entry &&
            ((previous_entry->addr_ + previous_entry->length_) == (addr & OXNAS_DMA_ADR_MASK)) &&
            ((previous_entry->length_ + length) <= MAX_OXNAS_DMA_TRANSFER_LENGTH)) {
             
            previous_entry->length_ += length;
        } else 
#endif
        {
			 
			if (!next_entry) {
				failed = 1;
				break;
			}
			sg_entry = next_entry;

            if (previous_entry) {
                 
                previous_entry->v_next_ = sg_entry;
                previous_entry->p_next_ = sg_entry->paddr_;
            } else {
                 
                sg_info->v_dstEntries_ = sg_entry;
                sg_info->p_dstEntries_ = sg_entry->paddr_;
            }
            previous_entry = sg_entry;

            sg_entry->addr_ = addr & OXNAS_DMA_ADR_MASK;

            if (length > MAX_OXNAS_DMA_TRANSFER_LENGTH) {
                printk(KERN_WARNING "oxnas_dma_set_prd_common() Destination entry too long (0x%x), zeroing\n", length);
                sg_entry->length_ = 0;
            } else {
                sg_entry->length_ = length;
            }

			next_entry = sg_entry->next_;
        }
    } while (!eot);
    if (sg_entry) {
         
        sg_entry->p_next_ = 0;
        sg_entry->v_next_ = 0;
    }

    if (failed) {
         
        sg_info->p_dstEntries_ = 0;
        sg_info->v_dstEntries_ = 0;
        sg_info->p_srcEntries_ = 0;
        sg_info->v_srcEntries_ = 0;
		printk(KERN_WARNING "Too few SG entries to satisfy destination requirements\n");
        return 1;
    }

	if ((sg_entry = sg_info->v_srcEntries_) && !sg_entry->v_next_) {
		sg_entry->length_ = tot_dst_len;
	} else if ((sg_entry = sg_info->v_dstEntries_) && !sg_entry->v_next_) {
		sg_entry->length_ = tot_src_len;
	}

    sg_info->qualifer_ = ((channel->channel_number_ << OXNAS_DMA_SG_CHANNEL_BIT) |
                          (src_settings->read_eot_policy_ << OXNAS_DMA_SG_SRC_EOT_BIT) |
                          (dst_settings->write_eot_policy_ << OXNAS_DMA_SG_DST_EOT_BIT) |
                          (1 << OXNAS_DMA_SG_QUALIFIER_BIT));

    sg_info->control_ = encode_control_status(src_settings, dst_settings, 0);

    atomic_inc(&channel->active_count_);

    return 0;
}

int oxnas_dma_device_set_prd(
    oxnas_dma_channel_t         *channel,
    oxnas_dma_direction_t        direction,
    struct ata_prd              *mem_prd,
    oxnas_dma_device_settings_t *device_settings,
    oxnas_dma_mode_t             mem_mode,
	oxnas_dma_sg_entry_t		 *sg_entries)
{
    struct ata_prd dev_prd;
    oxnas_dma_device_settings_t mem_settings;

    if (unlikely(oxnas_dma_is_active(channel))) {
        printk(KERN_WARNING "oxnas_dma_device_set_prd() Trying to use channel %u while active\n", channel->channel_number_);
    }

    mem_settings = oxnas_ram_generic_dma_settings;
    mem_settings.address_ = 0;   
    mem_settings.address_mode_ = mem_mode;

    dev_prd.addr = device_settings->address_;
    dev_prd.flags_len = ATA_PRD_EOT;

    return oxnas_dma_set_prd_common(
        channel,
        (direction == OXNAS_DMA_TO_DEVICE)   ? mem_prd       : &dev_prd,
        (direction == OXNAS_DMA_FROM_DEVICE) ? mem_prd       : &dev_prd,
        (direction == OXNAS_DMA_TO_DEVICE)   ? &mem_settings : device_settings,
        (direction == OXNAS_DMA_FROM_DEVICE) ? &mem_settings : device_settings,
		sg_entries);
}

void oxnas_dma_set_callback(oxnas_dma_channel_t* channel, oxnas_dma_callback_t callback, oxnas_callback_arg_t arg)
{
#if defined(OXNAS_DMA_TEST) || defined(OXNAS_DMA_SG_TEST)    
printk("Registering callback 0x%08x for channel %u\n", (unsigned)callback, channel->channel_number_);
#endif  
    channel->notification_callback_ = callback;
    channel->notification_arg_ = arg;
}

void oxnas_dma_abort(oxnas_dma_channel_t *channel)
{
    u32 ctrl_status;
    unsigned channel_number = channel->channel_number_;

    spin_lock(&dma_controller.spinlock_);
    ctrl_status = readl(DMA_CALC_REG_ADR(channel_number, DMA_CTRL_STATUS));
    ctrl_status |= DMA_CTRL_STATUS_RESET;
    writel(ctrl_status, DMA_CALC_REG_ADR(channel_number, DMA_CTRL_STATUS));
    spin_unlock(&dma_controller.spinlock_);

    while (readl(DMA_CALC_REG_ADR(channel_number, DMA_CTRL_STATUS)) & DMA_CTRL_STATUS_IN_PROGRESS);

    spin_lock(&dma_controller.spinlock_);
    ctrl_status = readl(DMA_CALC_REG_ADR(channel_number, DMA_CTRL_STATUS));
    ctrl_status &= ~DMA_CTRL_STATUS_RESET;
    writel(ctrl_status, DMA_CALC_REG_ADR(channel_number, DMA_CTRL_STATUS));
    spin_unlock(&dma_controller.spinlock_);

	if (channel->v_sg_info_->v_srcEntries_) {
		 if (channel->auto_sg_entries_) {
			 
			oxnas_dma_sg_entry_t* sg_entry = channel->v_sg_info_->v_srcEntries_;
			while (sg_entry) {
				oxnas_dma_sg_entry_t* next = sg_entry->v_next_;
				free_sg_entry(sg_entry);
				sg_entry = next;
			}

			sg_entry = channel->v_sg_info_->v_dstEntries_;
			while (sg_entry) {
				oxnas_dma_sg_entry_t* next = sg_entry->v_next_;
				free_sg_entry(sg_entry);
				sg_entry = next;
			}
		 }

		channel->v_sg_info_->p_srcEntries_ = 0;
		channel->v_sg_info_->v_srcEntries_ = 0;
		channel->v_sg_info_->p_dstEntries_ = 0;
		channel->v_sg_info_->v_dstEntries_ = 0;
	}

	atomic_set(&channel->interrupt_count_, 0);
	atomic_set(&channel->active_count_, 0);
}

void oxnas_dma_start(oxnas_dma_channel_t* channel)
{
     
    if (channel->v_sg_info_->v_srcEntries_) {
#ifdef OXNAS_DMA_SG_TEST_DUMP_DESCRIPTORS
         
        oxnas_dma_sg_entry_t* d = channel->v_sg_info_->v_srcEntries_;
        printk("qualifer_ = 0x%08lx, control_ = 0x%lx\n", channel->v_sg_info_->qualifer_, channel->v_sg_info_->control_);
        printk("Source Descriptors:\n");
        while (d) {
            printk("v_addr=0x%08x, p_addr=0x%08x, addr_=0x%08x, length_=0x%08lx, next=0x%08x\n", (u32)d, (u32)d->paddr_, d->addr_, d->length_, d->p_next_);
            d = d->v_next_;
        }
        printk("Destination Descriptors:\n");
        d = channel->v_sg_info_->v_dstEntries_;
        while (d) {
            printk("v_addr=0x%08x, p_addr=0x%08x, addr_=0x%08x, length_=0x%08lx, next=0x%08x\n", (u32)d, (u32)d->paddr_, d->addr_, d->length_, d->p_next_);
            d = d->v_next_;
        }
#endif  

		writel(1UL << DMA_SG_RESETS_CONTROL_BIT, DMA_SG_CALC_REG_ADR(channel->channel_number_, DMA_SG_RESETS));

        writel(channel->p_sg_info_, DMA_SG_CALC_REG_ADR(channel->channel_number_, DMA_SG_REQ_PTR));

#ifdef OXNAS_DMA_SG_TEST
printk("p_sg_info_ = 0x%08x written to 0x%08x\n", (u32)channel->p_sg_info_, DMA_SG_CALC_REG_ADR(channel->channel_number_, DMA_SG_REQ_PTR));
printk("*(DMA_SG_CONTROL) = 0x%08x\n", readl(DMA_SG_CALC_REG_ADR(channel->channel_number_, DMA_SG_CONTROL)));
printk("*(DMA_SG_STATUS)  = 0x%08x\n", readl(DMA_SG_CALC_REG_ADR(channel->channel_number_, DMA_SG_STATUS)));
printk("*(DMA_SG_REQ_PTR) = 0x%08x\n", readl(DMA_SG_CALC_REG_ADR(channel->channel_number_, DMA_SG_REQ_PTR)));
#endif  

        writel((1UL << DMA_SG_CONTROL_START_BIT) |
               (1UL << DMA_SG_CONTROL_QUEUING_ENABLE_BIT) |
               (1UL << DMA_SG_CONTROL_HBURST_ENABLE_BIT),
               DMA_SG_CALC_REG_ADR(channel->channel_number_, DMA_SG_CONTROL));
    } else {
         
        spin_lock(&dma_controller.spinlock_);
        writel(encode_start(readl(DMA_CALC_REG_ADR(channel->channel_number_, DMA_CTRL_STATUS))),
               DMA_CALC_REG_ADR(channel->channel_number_, DMA_CTRL_STATUS));
        spin_unlock(&dma_controller.spinlock_);
    }
}

void oxnas_dma_dump_registers()
{
    unsigned long* adr = (unsigned long*)DMA_CALC_REG_ADR(0, 0);
    unsigned long* end = (adr + DMA_REGS_PER_CHANNEL);
    int i;

    printk("oxnas_dma_dump_registers(), adr= 0x%08lx, end=0x%08lx\n", (unsigned long)adr, (unsigned long)(adr + (DMA_REGS_PER_CHANNEL * dma_controller.numberOfChannels_)));

    for (i=0; i < dma_controller.numberOfChannels_; i++) {
        for (; adr < end; adr++) {
            printk("0x%08lx\n", *adr);
        }
	printk("SG-Debug: 0x%08x\n", readl(DMA_SG_CALC_REG_ADR(i, DMA_SG_RESETS)));
        printk("-----------------------\n");
        end += DMA_REGS_PER_CHANNEL;
    }
    printk("oxnas_dma_dump_registers() - end\n");
}

void oxnas_dma_dump_registers_single(int channel_number)
{
    unsigned long* adr = (unsigned long*)DMA_CALC_REG_ADR(channel_number, 0);
    unsigned long* end = (adr + DMA_REGS_PER_CHANNEL);

    printk("DMA channel %d regs:\n", channel_number);
    for (; adr < end; adr++) {
        printk("0x%08lx\n", *adr);
    }
}

#if defined(OXNAS_DMA_TEST) || defined(OXNAS_DMA_SG_TEST)
static __DECLARE_SEMAPHORE_GENERIC(callback_semaphore, 0);    

static void dma_callback(
    oxnas_dma_channel_t         *channel,
    oxnas_callback_arg_t         arg,
    oxnas_dma_callback_status_t  error_code,
    int                          interrupt_count)
{
    printk("dma_callback() for channel %u, arg = 0x%lx, status = 0x%04x, interrupt_count = %d\n", channel->channel_number_, (unsigned long)arg, error_code, interrupt_count);
    up(&callback_semaphore);
}

#include <linux/dma-mapping.h>
#include <linux/slab.h>

#ifdef OXNAS_DMA_TEST
static void dma_test(unsigned long length)
{
    void* memory1;
    void* memory2;
    unsigned long* ptr;
    unsigned long quads;
    int i;
    unsigned long* end;
    dma_addr_t dma_address1;
    dma_addr_t dma_address2;
    oxnas_dma_channel_t* channels[MAX_OXNAS_DMA_CHANNELS];

    printk("*************************************************************\n");
    printk("                                                               \n");
    printk("Simple DMA Test, length = %lu, number of channel = %u\n", length, MAX_OXNAS_DMA_CHANNELS);
    printk("                                                               \n");
    printk("*************************************************************\n");

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        channels[i] = oxnas_dma_request(0);
        if (channels[i] == OXNAS_DMA_CHANNEL_NUL) {
            printk("No DMA channels[%d] obtained\n", i);
        } else {
            printk("Obtained DMA channels[%d] %u, isActive=%d\n", i, channels[i]->channel_number_, oxnas_dma_is_active(channels[i]));
        }
    }

    printk("Calling kmalloc()\n");
    memory1 = kmalloc(length, GFP_KERNEL | GFP_DMA);
    memory2 = kmalloc(length, GFP_KERNEL | GFP_DMA);

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        int j;

        ptr = (unsigned long*)memory1;
        quads = length/sizeof(unsigned long);
        for (j=0; j < quads; j++) {
            *ptr++ = 0xdeadbeef;
        }
        ptr = (unsigned long*)memory2;
        for (j=0; j < quads; j++) {
            *ptr++ = 0xc001babe;
        }
    
        printk("Before:\n");
        ptr = (unsigned long*)memory1;
        end = (unsigned long*)(memory1 + length);
        while (ptr < end) {
            for (j=0; j < 8; j++) {
                printk("0x%08lx ", *ptr++);
            }
            printk("\n");
        }
        printk("---------------------------------------------------------\n");
        ptr = (unsigned long*)memory2;
        end = (unsigned long*)(memory2 + length);
        while (ptr < end) {
            for (j=0; j < 8; j++) {
                printk("0x%08lx ", *ptr++);
            }
            printk("\n");
        }
    
        dma_address1 = dma_map_single(0, memory1, length, DMA_TO_DEVICE);
        if (dma_mapping_error(dma_address1)) {
            printk("Consistent DMA mapping 1 failed\n");
        }
    
        dma_address2 = dma_map_single(0, memory2, length, DMA_BIDIRECTIONAL);
        if (dma_mapping_error(dma_address2)) {
            printk("Consistent DMA mapping 2 failed\n");
        }
    
        printk("Calling oxnas_dma_set(), memory1 = 0x%08lx, memory2 = 0x%08lx\n", (unsigned long)memory1, (unsigned long)memory2);
        oxnas_dma_set(channels[i], (unsigned char*)dma_address1, length,
            (unsigned char*)dma_address2, OXNAS_DMA_MODE_INC,
			OXNAS_DMA_MODE_INC, 0, 1);  
    
        oxnas_dma_set_callback(channels[i], dma_callback, OXNAS_DMA_CALLBACK_ARG_NUL);
    
        printk("oxnas_dma_start() for channel %u\n", channels[i]->channel_number_);
        oxnas_dma_start(channels[i]);
    
        printk("Waiting for channel to be inactive\n");
    
        while (down_interruptible(&callback_semaphore));
        oxnas_dma_set_callback(channels[i], OXNAS_DMA_CALLBACK_NUL, OXNAS_DMA_CALLBACK_ARG_NUL);
    
        dma_unmap_single(0, dma_address1, length, DMA_TO_DEVICE);
        dma_unmap_single(0, dma_address2, length, DMA_BIDIRECTIONAL);
    
        printk("After:\n");
        ptr = (unsigned long*)memory1;
        end = (unsigned long*)(memory1 + length);
        while (ptr < end) {
            for (j=0; j < 8; j++) {
                printk("0x%08lx ", *ptr++);
            }
            printk("\n");
        }
        printk("---------------------------------------------------------\n");
        ptr = (unsigned long*)memory2;
        end = (unsigned long*)(memory2 + length);
        while (ptr < end) {
            for (j=0; j < 8; j++) {
                printk("0x%08lx ", *ptr++);
            }
            printk("\n");
        }
    }

    printk("Calling kfree()\n");
    kfree(memory1);
    kfree(memory2);
    printk("Returned from kfree()\n");

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        oxnas_dma_free(channels[i]);
    }

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        channels[i] = oxnas_dma_request(0);
        if (channels[i] == OXNAS_DMA_CHANNEL_NUL) {
            printk("No DMA channels[%d] obtained\n", i);
        } else {
            printk("Obtained DMA channels[%d] %u, isActive=%d\n", i, channels[i]->channel_number_, oxnas_dma_is_active(channels[i]));
        }
    }

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        oxnas_dma_free(channels[i]);
    }
}
#endif  

#ifdef OXNAS_DMA_SG_TEST
static void dma_sg_test(void)
{
    int i;
    struct scatterlist* src_scatterlist = 0;
    struct scatterlist* dst_scatterlist = 0;
    const int num_src_buffers = 8;
    const int num_dst_buffers = 3;
    unsigned long src_fill_value = 0;
    unsigned long total_src_len = 0;
    int channel_number;
    oxnas_dma_channel_t* channels[MAX_OXNAS_DMA_CHANNELS];

    printk("*************************************************************\n");
    printk("                                                               \n");
    printk("Scatter-Gather DMA Test\n");
    printk("                                                               \n");
    printk("*************************************************************\n");

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        channels[i] = oxnas_dma_request(0);
        if (channels[i] == OXNAS_DMA_CHANNEL_NUL) {
            printk("No DMA channels[%d] obtained\n", i);
        } else {
            printk("Obtained DMA channels[%d] %u, isActive=%d\n", i, channels[i]->channel_number_, oxnas_dma_is_active(channels[i]));
        }
    }

    for (channel_number=0; channel_number < MAX_OXNAS_DMA_CHANNELS; ++channel_number) {
        if (num_src_buffers) {
            printk("Allocating source SG list and entry buffers\n");
             
            src_scatterlist = (struct scatterlist*)kmalloc(sizeof(struct scatterlist) * num_src_buffers, GFP_KERNEL);
            src_scatterlist[0].offset = (unsigned int)kmalloc(8*1024,  GFP_KERNEL | GFP_DMA);
            src_scatterlist[0].__address = (char*)(8*1024);     
            src_scatterlist[0].length = 8*1024;
            src_scatterlist[0].page = (struct page*)0xdeadbeef;  
            src_scatterlist[1].offset = (unsigned int)kmalloc(8,     GFP_KERNEL | GFP_DMA);
            src_scatterlist[1].__address = (char*)8;        
            src_scatterlist[1].length = 8;
            src_scatterlist[1].page = (struct page*)0xc001babe;  
            src_scatterlist[2].offset = (unsigned int)kmalloc(48*1024, GFP_KERNEL | GFP_DMA);
            src_scatterlist[2].__address = (char*)(48*1024);    
            src_scatterlist[2].length = 16*1024;
            src_scatterlist[2].page = (struct page*)0x22222222;  
            src_scatterlist[3].offset = src_scatterlist[2].offset + src_scatterlist[2].length;
            src_scatterlist[3].__address = (char*)0;          
            src_scatterlist[3].length = 16*1024;
            src_scatterlist[3].page = (struct page*)0x33333333;  
            src_scatterlist[4].offset = src_scatterlist[3].offset + src_scatterlist[3].length;
            src_scatterlist[4].__address = (char*)0;          
            src_scatterlist[4].length = 16*1024;
            src_scatterlist[4].page = (struct page*)0x44444444;  
            src_scatterlist[5].offset = (unsigned int)kmalloc(64,      GFP_KERNEL | GFP_DMA);
            src_scatterlist[5].__address = (char*)64;         
            src_scatterlist[5].length = 64;
            src_scatterlist[5].page = (struct page*)0x55555555;  
            src_scatterlist[6].offset = (unsigned int)kmalloc(256,     GFP_KERNEL | GFP_DMA);
            src_scatterlist[6].__address = (char*)256;        
            src_scatterlist[6].length = 128;
            src_scatterlist[6].page = (struct page*)0x66666666;  
            src_scatterlist[7].offset = src_scatterlist[6].offset + src_scatterlist[6].length;
            src_scatterlist[7].__address = (char*)0;          
            src_scatterlist[7].length = 128;
            src_scatterlist[7].page = (struct page*)0x77777777;  
        }
    
        for (i=0; i < num_src_buffers; i++) {
            unsigned long* ptr = (unsigned long*)src_scatterlist[i].offset;
            int quads = src_scatterlist[i].length/sizeof(unsigned long);
            int j=0;
            printk("Filling source buffer %u\n", i);
            src_fill_value = (unsigned long)(src_scatterlist[i].page);
            for (; j < quads; j++) {
                *ptr++ = src_fill_value;
            }
        }
    
    #ifdef OXNAS_DMA_SG_TEST_DUMP_BUFFERS
         
        printk("Source Before:\n");
        for (i=0; i < num_src_buffers; i++) {
            unsigned long* ptr = (unsigned long*)src_scatterlist[i].offset;
            unsigned long* end = (unsigned long*)(src_scatterlist[i].offset + src_scatterlist[i].length);
            printk("Buffer %d\n", i);
            while (ptr < end) {
                int j=0;
                for (; j < 8; j++) {
                    printk("0x%08lx ", *ptr++);
                }
                printk("\n");
            }
        }
    #endif  
    
        for (i=0; i < num_src_buffers; i++) {
            printk("Creating DMA mappings for source entry buffer %u\n", i);
            src_scatterlist[i].dma_address = dma_map_single(0, (void*)src_scatterlist[i].offset, src_scatterlist[i].length, DMA_TO_DEVICE);
            if (dma_mapping_error(src_scatterlist[i].dma_address)) {
                printk("Consistent source DMA mapping %d failed\n", i);
            }
        }
    
        if (num_dst_buffers) {
            unsigned long dst_length;
            unsigned long offset;
            
            printk("Allocating destination SG list and entry buffers\n");
            total_src_len = 0;
            for (i=0; i < num_src_buffers; i++) {
                total_src_len += src_scatterlist[i].length;
            }
    
            dst_length = total_src_len / num_dst_buffers;
            dst_scatterlist = (struct scatterlist*)kmalloc(sizeof(struct scatterlist) * num_dst_buffers, GFP_KERNEL);
    
            dst_scatterlist[0].offset = (unsigned int)kmalloc(total_src_len,  GFP_KERNEL | GFP_DMA);
            dst_scatterlist[0].__address = (char*)total_src_len;  
            dst_scatterlist[0].length = dst_length;
    
            offset = dst_length;
            for (i=1; i < num_dst_buffers; i++) {
                dst_scatterlist[i].offset = dst_scatterlist[0].offset + offset;
                dst_scatterlist[i].__address = 0;  
                dst_scatterlist[i].length = dst_length;
    
                offset += dst_length;
            }
        }
    
        for (i=0; i < num_dst_buffers; i++) {
            unsigned long* ptr = (unsigned long*)dst_scatterlist[i].offset;
            int quads = dst_scatterlist[i].length/sizeof(unsigned long);
            int j=0;
            printk("Filling destination buffer %u\n", i);
            for (; j < quads; j++) {
                *ptr++ = 0x000000;
            }
        }
    
        for (i=0; i < num_dst_buffers; i++) {
            printk("Creating DMA mappings for destination entry buffer %u\n", i);
            dst_scatterlist[i].dma_address = dma_map_single(0, (void*)dst_scatterlist[i].offset, dst_scatterlist[i].length, DMA_BIDIRECTIONAL);
            if (dma_mapping_error(dst_scatterlist[i].dma_address)) {
                printk("Consistent destination DMA mapping %d failed\n", i);
            }
        }
    
        printk("Setting up transfer\n");
        oxnas_dma_set_sg(channels[channel_number], src_scatterlist,
			num_src_buffers, dst_scatterlist, num_dst_buffers,
			OXNAS_DMA_MODE_INC, OXNAS_DMA_MODE_INC, 0);

        oxnas_dma_set_callback(channels[channel_number], dma_callback, OXNAS_DMA_CALLBACK_ARG_NUL);
    
        printk("Starting the transfer\n");
        oxnas_dma_start(channels[channel_number]);
    
        printk("Waiting for transfer to complete...\n");
    
        while (down_interruptible(&callback_semaphore));
        oxnas_dma_set_callback(channels[channel_number], OXNAS_DMA_CALLBACK_NUL, OXNAS_DMA_CALLBACK_ARG_NUL);
    
        for (i=0; i < num_src_buffers; i++) {
            printk("Releasing DMA mappings for source entry buffer %u\n", i);
            dma_unmap_single(0, src_scatterlist[i].dma_address, src_scatterlist[i].length, DMA_TO_DEVICE);
        }
    
        for (i=0; i < num_dst_buffers; i++) {
            printk("Releasing DMA mappings for destination entry buffer %u\n", i);
            dma_unmap_single(0, dst_scatterlist[i].dma_address, dst_scatterlist[i].length, DMA_BIDIRECTIONAL);
        }
    
    {
    u32 sw_csum = 0;
    for (i=0; i < num_src_buffers; i++) {
        sw_csum = csum_partial((u8*)src_scatterlist[i].offset, src_scatterlist[i].length, sw_csum);
    }
    printk("S/W generated src csum = 0x%04hx\n", csum_fold(sw_csum));
    
    sw_csum = 0;
    for (i=0; i < num_dst_buffers; i++) {
        sw_csum = csum_partial((u8*)dst_scatterlist[i].offset, dst_scatterlist[i].length, sw_csum);
    }
    printk("S/W generated dst csum = 0x%04hx\n", csum_fold(sw_csum));
    }
    
    #ifdef OXNAS_DMA_SG_TEST_DUMP_BUFFERS
     
        printk("Destination After:\n");
        for (i=0; i < num_dst_buffers; i++) {
            unsigned long* ptr = (unsigned long*)dst_scatterlist[i].offset;
            unsigned long* end = (unsigned long*)(dst_scatterlist[i].offset + dst_scatterlist[i].length);
            printk("Buffer %d\n", i);
            while (ptr < end) {
                int j=0;
                for (; j < 8; j++) {
                    printk("0x%08lx ", *ptr++);
                }
                printk("\n");
            }
        }
    #endif  
    
        for (i=0; i < num_src_buffers; i++) {
             
            if (src_scatterlist[i].__address) {
                printk("Freeing source SG entry buffer, adr = 0x%08x, len = 0x%08x\n", src_scatterlist[i].offset, (u32)src_scatterlist[i].__address);            
                kfree((void*)src_scatterlist[i].offset);
            }
        }
    
        if (src_scatterlist) {
            printk("Freeing source SG scatter list structure\n");
            kfree(src_scatterlist);
        }
    
        for (i=0; i < num_dst_buffers; i++) {
            if (dst_scatterlist[i].__address) {
                printk("Freeing destination SG entry, adr = 0x%08x, len = 0x%08x\n", dst_scatterlist[i].offset, (u32)dst_scatterlist[i].__address);            
                kfree((void*)dst_scatterlist[i].offset);
            }
        }
    
        if (dst_scatterlist) {
            printk("Freeing source SG scatter list structure\n");
            kfree(dst_scatterlist);
        }
    }

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        oxnas_dma_free(channels[i]);
    }
}
#endif  

#ifdef OXNAS_DMA_SG_TEST_2
static void dma_sg_test2()
{
     
    static const unsigned char bad_src_data0[] = {
        0xff, 0xff, 0x00, 0xa0, 0xd2, 0x05, 0x06, 0xec, 0x00, 0xcf, 0x52, 0x49, 0xc3, 0x03, 0x08, 0x00,
        0x45, 0x00, 0x05, 0xb4, 0x99, 0x45, 0x40, 0x00, 0x40, 0x06, 0x42, 0xf5, 0xac, 0x1f, 0x00, 0x65,
        0xac, 0x1f, 0x00, 0x66
    };

    static const unsigned char bad_src_data1[] = {
        0x04, 0x00, 0x13, 0x89, 0x02, 0x8a, 0x5c, 0x83, 0x52, 0xde, 0xc7, 0x0c, 0x80, 0x19, 0x0b, 0x68,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x08, 0x0a, 0xff, 0xff, 0xb3, 0x9d, 0x3f, 0x82, 0xf0, 0xff,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
        0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
        0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
        0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
        0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31
    };
    
    static const unsigned char good_src_data0[] = {
        0xff, 0xff, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5
    };

    static const unsigned char good_src_data1[] = {
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5,
        0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5, 0xa5
    };

    static const int src_offset = 2;     
    static const int dst_buffer_size = 512;

    const unsigned char *src_data0 = bad_src_data0;
    const unsigned char *src_data1 = bad_src_data1;
    unsigned long src_data0_len = sizeof(bad_src_data0);
    unsigned long src_data1_len = sizeof(bad_src_data1);
    int channel_number;
    oxnas_dma_channel_t* channels[MAX_OXNAS_DMA_CHANNELS];
    int i;

    printk("*************************************************************\n");
    printk("                                                               \n");
    printk("Scatter-Gather DMA Test 2\n");
    printk("                                                               \n");
    printk("*************************************************************\n");

    printk("seg0 0x%08x, %lu\n", (u32)src_data0, src_data0_len);
    printk("seg1 0x%08x, %lu\n", (u32)src_data1, src_data1_len);

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        channels[i] = oxnas_dma_request(0);
        if (channels[i] == OXNAS_DMA_CHANNEL_NUL) {
            printk("No DMA channels[%d] obtained\n", i);
        } else {
            printk("Obtained DMA channels[%d] %u, isActive=%d\n", i, channels[i]->channel_number_, oxnas_dma_is_active(channels[i]));
        }
    }

    for (channel_number=0; channel_number < MAX_OXNAS_DMA_CHANNELS; ++channel_number) {
    
        struct scatterlist* src_scatterlist = (struct scatterlist*)kmalloc(sizeof(struct scatterlist) * 2, GFP_KERNEL);
    
        unsigned long total_src_length = src_data0_len + src_data1_len;
        src_scatterlist[0].offset = (unsigned int)kmalloc(total_src_length,  GFP_KERNEL | GFP_DMA) + src_offset;
        src_scatterlist[0].length = src_data0_len - src_offset;
        memcpy((u8*)src_scatterlist[0].offset, src_data0, src_scatterlist[0].length);
    
        src_scatterlist[1].offset = src_scatterlist[0].offset + src_scatterlist[0].length;
        src_scatterlist[1].length = src_data1_len;
        memcpy((u8*)src_scatterlist[1].offset, src_data1, src_scatterlist[1].length);
    
        unsigned long total_dst_length = total_src_length - src_offset;   
        unsigned num_dst_buffers = total_dst_length / dst_buffer_size;
        if ((num_dst_buffers * dst_buffer_size) < total_dst_length) {
            ++num_dst_buffers;
        }
        printk("total_src_length = %lu, src_offset = %u, total_dst_length = %lu, dst_buffer_size = %u, num_dst_buffers = %u\n", total_src_length, src_offset, total_dst_length, dst_buffer_size, num_dst_buffers);
        struct scatterlist* dst_scatterlist = (struct scatterlist*)kmalloc(sizeof(struct scatterlist) * num_dst_buffers, GFP_KERNEL);
    
        int i;
        unsigned long remainder = total_dst_length;
        for (i=0; i < num_dst_buffers; ++i) {
            dst_scatterlist[i].offset = (unsigned int)kmalloc(dst_buffer_size,  GFP_KERNEL | GFP_DMA);
            dst_scatterlist[i].length = (remainder < dst_buffer_size) ? remainder : dst_buffer_size;
            remainder -= dst_scatterlist[i].length;
        }
    
        int j;
        for (j=0; j < OXNAS_DMA_SG_TEST2_ITERATIONS; ++j) {
            src_scatterlist[0].dma_address = dma_map_single(0, (void*)src_scatterlist[0].offset, src_scatterlist[0].length, DMA_TO_DEVICE);
            if (dma_mapping_error(src_scatterlist[0].dma_address)) {
                printk("Consistent source DMA mapping 0 failed\n");
            }
    
            src_scatterlist[1].dma_address = dma_map_single(0, (void*)src_scatterlist[1].offset, src_scatterlist[1].length, DMA_TO_DEVICE);
            if (dma_mapping_error(src_scatterlist[1].dma_address)) {
                printk("Consistent source DMA mapping 1 failed\n");
            }
    
            printk("num_dst_buffers = %u\n", num_dst_buffers);
            for (i=0; i < num_dst_buffers; i++) {
                memset((void*)dst_scatterlist[i].offset, 0, dst_scatterlist[i].length);
    
                dst_scatterlist[i].dma_address = dma_map_single(0, (void*)dst_scatterlist[i].offset, dst_scatterlist[i].length, DMA_BIDIRECTIONAL);
                if (dma_mapping_error(dst_scatterlist[i].dma_address)) {
                    printk("Consistent destination DMA mapping %d failed\n", i);
                }
            }
    
            printk("Setting up transfer\n");
            oxnas_dma_set_sg(channels[channel_number], src_scatterlist, 2,
                dst_scatterlist, num_dst_buffers, OXNAS_DMA_MODE_INC,
                OXNAS_DMA_MODE_INC, 0);

            oxnas_dma_set_callback(channels[channel_number], dma_callback, OXNAS_DMA_CALLBACK_ARG_NUL);
    
            printk("Starting the transfer\n");
            oxnas_dma_start(channels[channel_number]);
    
            printk("Waiting for transfer to complete...\n");
            while (down_interruptible(&callback_semaphore));
            oxnas_dma_set_callback(channels[channel_number], OXNAS_DMA_CALLBACK_NUL, OXNAS_DMA_CALLBACK_ARG_NUL);
    
            printk("Error code = %u\n", channels[channel_number]->error_code_);
    
            for (i=0; i < 2; i++) {
                dma_unmap_single(0, src_scatterlist[i].dma_address, src_scatterlist[i].length, DMA_TO_DEVICE);
            }
    
            for (i=0; i < num_dst_buffers; i++) {
                printk("Releasing DMA mappings for destination entry buffer %u\n", i);
                dma_unmap_single(0, dst_scatterlist[i].dma_address, dst_scatterlist[i].length, DMA_BIDIRECTIONAL);
            }
    
            u32 sw_csum = 0;
     
            sw_csum = csum_partial((u8*)src_scatterlist[1].offset, src_scatterlist[1].length, sw_csum);
            printk("S/W generated src csum = 0x%04hx\n", csum_fold(sw_csum));
    
            sw_csum = 0;
            unsigned offset = src_scatterlist[0].length;
     
            for (i=0; i < num_dst_buffers; i++) {
                sw_csum = csum_partial((u8*)dst_scatterlist[i].offset + offset, dst_scatterlist[i].length - offset, sw_csum);
                offset = 0;
            }
            printk("S/W generated dst csum = 0x%04hx\n", csum_fold(sw_csum));
        }
    
        for (i=0; i < num_dst_buffers; ++i) {
            kfree((void*)dst_scatterlist[i].offset);
        }
        kfree(dst_scatterlist);
    
        kfree((void*)(src_scatterlist[0].offset - src_offset));
        kfree(src_scatterlist);
    }

    for (i=0; i < MAX_OXNAS_DMA_CHANNELS; ++i) {
        oxnas_dma_free(channels[i]);
    }
}
#endif  
#endif  

EXPORT_SYMBOL(oxnas_dma_request);
EXPORT_SYMBOL(oxnas_dma_free);
EXPORT_SYMBOL(oxnas_dma_set_callback);
EXPORT_SYMBOL(oxnas_dma_set_common);
EXPORT_SYMBOL(oxnas_dma_is_active);
EXPORT_SYMBOL(oxnas_dma_raw_isactive);
EXPORT_SYMBOL(oxnas_dma_set);
EXPORT_SYMBOL(oxnas_dma_device_set);
EXPORT_SYMBOL(oxnas_dma_abort);
EXPORT_SYMBOL(oxnas_dma_dump_registers);
EXPORT_SYMBOL(oxnas_dma_dump_registers_single);
EXPORT_SYMBOL(oxnas_dma_start);

#ifdef CONFIG_SATA_OX810
EXPORT_SYMBOL(oxnas_sata_dma_settings);
#endif  

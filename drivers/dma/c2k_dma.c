#include <linux/init.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>
#include <asm/io.h>
#include <mach/c2k_dma.h>
#include <linux/dmaengine.h>
#include <linux/circ_buf.h>
#include <linux/delay.h>

#define XOR_MAX_SRC_CNT	6

#define virt_to_xor_dma(pbase, vbase, vaddr)		((pbase + ((unsigned long)vaddr - (unsigned long)vbase)))

struct comcerto_xor_device {
	struct dma_device        device;
};

struct comcerto_xor_chan {
	struct comcerto_xor_device        *device;
	struct dma_chan                 chan;
	struct tasklet_struct       irq_tasklet;
	spinlock_t                  lock;
	dma_cookie_t                completed_cookie;
};

struct comcerto_sw_xor_desc {
	struct dma_async_tx_descriptor async_tx;
	int src_cnt;
	size_t len;
	dma_addr_t dma_dest;
	dma_addr_t dma_src[XOR_MAX_SRC_CNT];
};

void *comcerto_xor_pool_virt;
dma_addr_t comcerto_xor_pool_phy;

spinlock_t	comcerto_xor_cleanup_lock;
static DECLARE_WAIT_QUEUE_HEAD(comcerto_xor_wait_queue);
static int comcerto_xor_sleeping = 0;

static int xor_rd_idx = 0;
static int xor_wr_idx = 0;
static int xor_dma_idx = 0;
static int xor_sw_rd_idx = 0;
static int xor_sw_wr_idx = 0;
static int xor_sw_wr_prev_idx = 0;
static int xor_current_batch_count = 0;

#define XOR_FDESC_COUNT 512

#if defined(CONFIG_COMCERTO_64K_PAGES)
#define XOR_SW_FDESC_COUNT 32
#else
#define XOR_SW_FDESC_COUNT XOR_FDESC_COUNT
#endif

struct timer_list comcerto_xor_timer;
#define COMPLETION_TIMEOUT      msecs_to_jiffies(100)
static int comerto_xor_timer_first = 1;

struct comcerto_xor_inbound_fdesc *xor_in_fdesc[XOR_FDESC_COUNT];
struct comcerto_xor_outbound_fdesc *xor_out_fdesc[XOR_FDESC_COUNT];
struct comcerto_sw_xor_desc     sw_xor_desc[XOR_SW_FDESC_COUNT];

struct comcerto_xor_device *comcerto_xor_dev;
struct comcerto_xor_chan   comcerto_xor_ch;
#define OWNER_XOR_FREE	1
#define OWNER_MEMCPY_FREE 2
#define OWNER_XOR_BUSY	3
#define OWNER_MEMCPY_BUSY	4
static int dma_owned = 0;
static int memcpy_processed_ongoing = 0;
static int memcpy_pending_count = 0;


//static int mdma_busy = 0;
static int mdma_done;
static spinlock_t mdma_lock;

static void *virtbase;

#define M2IO_CONTROL           (virtbase)
#define M2IO_HEAD              (virtbase + 0x4)
#define M2IO_BURST             (virtbase + 0x8)
#define M2IO_FLEN              (virtbase + 0xC)
#define M2IO_IRQ_ENABLE        (virtbase + 0x10)
#define M2IO_IRQ_STATUS        (virtbase + 0x14)
#define M2IO_RESET             (virtbase + 0x20)

#define IO2M_CONTROL           (virtbase + 0x80)
#define IO2M_HEAD              (virtbase + 0x84)
#define IO2M_BURST             (virtbase + 0x88)
#define IO2M_FLEN              (virtbase + 0x8C)
#define IO2M_IRQ_ENABLE        (virtbase + 0x90)
#define IO2M_IRQ_STATUS        (virtbase + 0x94)
#define IO2M_RESET             (virtbase + 0xA0)

#define FDONE_MASK	0x80000000

#define FLENEN          0x2

static DECLARE_WAIT_QUEUE_HEAD(mdma_memcpy_busy_queue);
static DECLARE_WAIT_QUEUE_HEAD(mdma_done_queue);

unsigned long mdma_in_desc_phy;
unsigned long mdma_out_desc_phy;

struct comcerto_memcpy_inbound_fdesc *mdma_in_desc;
struct comcerto_memcpy_outbound_fdesc *mdma_out_desc;

EXPORT_SYMBOL(mdma_in_desc);
EXPORT_SYMBOL(mdma_out_desc);

static inline void comcerto_xor_set_in_bdesc(u32 buf_idx, u32 bdesc_idx, u32 addr, u32 ctrl)
{
	xor_in_fdesc[buf_idx]->bdesc[bdesc_idx].bpointer = addr;
	xor_in_fdesc[buf_idx]->bdesc[bdesc_idx].bcontrol = ctrl;
}

static inline void comcerto_xor_set_out_bdesc(u32 buf_idx, u32 bdesc_idx, u32 addr, u32 ctrl)
{
	xor_out_fdesc[buf_idx]->bdesc[bdesc_idx].bpointer = addr;
	xor_out_fdesc[buf_idx]->bdesc[bdesc_idx].bcontrol = ctrl;
}

static void comcerto_xor_set_desc(sw_idx,  hw_idx)
{
	int i,split_no;
	u32 fstatus0 = 0;
	u32 addr;
	int split_size;
	dma_addr_t dest;
	dma_addr_t *srcs;	
	u32 block_size;
	int src_cnt;

	block_size = sw_xor_desc[sw_idx].len;
	src_cnt = sw_xor_desc[sw_idx].src_cnt;
	srcs = sw_xor_desc[sw_idx].dma_src;
	dest = sw_xor_desc[sw_idx].dma_dest;

	if(block_size != PAGE_SIZE)
		printk("%s: input buffers not %d len\n",__func__, (unsigned int)PAGE_SIZE);	

#if defined(CONFIG_COMCERTO_64K_PAGES)
	block_size = block_size/16; //to get 4K
	split_size = 16;
#else
	split_size = 1;
#endif

	for(split_no = 0 ; split_no < split_size; split_no++)
	{

		for(i = 0; i < src_cnt - 1; i++) {
			addr = (u32)sw_xor_desc[sw_idx].dma_src[i] + 4096 * split_no;
			comcerto_xor_set_in_bdesc(hw_idx, i, addr, block_size);
		}

		addr = (u32)sw_xor_desc[sw_idx].dma_src[src_cnt - 1] + 4096 * split_no;
		comcerto_xor_set_in_bdesc(hw_idx, src_cnt - 1, addr, block_size | BLAST);

		fstatus0 = 1; // New Req, reset block counter, block offset, clear scratchpad (overwrite existing data)
		fstatus0 |=  (1 << 1); // Read SP, return content of scratch pad after processing input data
		fstatus0 |=  (0 << 2); // Mode, Encode
		fstatus0 |=  (src_cnt << 4); // Number of blocks to be processed
		fstatus0 |=  (1 << 9); // Type, XOR
		fstatus0 |=  (XOR_BLOCK_SIZE_4096 << 11);

		xor_in_fdesc[hw_idx]->fcontrol = 0;
		xor_in_fdesc[hw_idx]->fstatus0 = fstatus0;
		xor_in_fdesc[hw_idx]->fstatus1 = 0;

		addr = (u32)sw_xor_desc[sw_idx].dma_dest + 4096 * split_no;
		comcerto_xor_set_out_bdesc(hw_idx, 0, addr, block_size | BLAST);

		xor_out_fdesc[hw_idx]->fcontrol = 0;
		xor_out_fdesc[hw_idx]->fstatus0 = 0;
		xor_out_fdesc[hw_idx]->fstatus1 = 0;

		hw_idx = (hw_idx + 1) % XOR_FDESC_COUNT;
	}

	xor_wr_idx = hw_idx;
}

static inline int comcerto_dma_busy(void)
{
	return (readl_relaxed(IO2M_CONTROL) & 0x1);
}

static void comcerto_xor_update_dma_head(int idx)
{
	u32 out_desc_head, in_desc_head;
	out_desc_head = virt_to_xor_dma(comcerto_xor_pool_phy, comcerto_xor_pool_virt, xor_out_fdesc[idx]);
	in_desc_head = virt_to_xor_dma(comcerto_xor_pool_phy, comcerto_xor_pool_virt, xor_in_fdesc[idx]);

	wmb();
	writel_relaxed(out_desc_head, IO2M_HEAD);
	writel_relaxed(in_desc_head, M2IO_HEAD);
}

static void comcerto_xor_update_dma_flen(int flen)
{
	wmb();
	writel_relaxed(flen, M2IO_FLEN);
	writel_relaxed(flen, IO2M_FLEN);
}

static int comcerto_xor_rb_full(void)
{
	if(CIRC_SPACE(xor_sw_wr_idx, xor_sw_rd_idx, XOR_SW_FDESC_COUNT) > 0)
		return 0;
	else
		return 1;
}

void comcerto_xor_request_wait(void)
{
	DEFINE_WAIT(wait);

	prepare_to_wait(&comcerto_xor_wait_queue, &wait, TASK_UNINTERRUPTIBLE);

	comcerto_xor_sleeping++;

	while (comcerto_xor_rb_full()) {
		spin_unlock_bh(&comcerto_xor_ch.lock);
		schedule();
		spin_lock_bh(&comcerto_xor_ch.lock);
		prepare_to_wait(&comcerto_xor_wait_queue, &wait, TASK_UNINTERRUPTIBLE);
	}

	finish_wait(&comcerto_xor_wait_queue, &wait);
	comcerto_xor_sleeping--;
}

static void comcerto_dma_process(void)
{
	unsigned long pending_count;

	if(!dma_owned || dma_owned==OWNER_XOR_FREE)
	{
		pending_count = CIRC_CNT(xor_wr_idx, xor_dma_idx, XOR_FDESC_COUNT);
		if(pending_count)
		{
			dma_owned = OWNER_XOR_BUSY;
			xor_current_batch_count = pending_count;
			comcerto_xor_update_dma_flen(pending_count);
			comcerto_xor_update_dma_head(xor_dma_idx);
		}
	}
}

static void comcerto_xor_cleanup(void)
{
	int i,j,k;
	int idx;
	int cleanup_count;
	unsigned long flags;
	int split_size;
	struct comcerto_sw_xor_desc *sw_desc;

	spin_lock_irqsave(&mdma_lock, flags);
	cleanup_count = CIRC_CNT(xor_dma_idx, xor_rd_idx, XOR_FDESC_COUNT);
	spin_unlock_irqrestore(&mdma_lock, flags);

#if defined(CONFIG_COMCERTO_64K_PAGES)
	split_size = 16;
#else
	split_size = 1;
#endif

	if(cleanup_count && (cleanup_count % split_size == 0))
	{
		for(i = 0 ; i < cleanup_count/split_size; i++)
		{
			struct dma_async_tx_descriptor *tx;

#if defined(CONFIG_COMCERTO_64K_PAGES)
			if(xor_rd_idx%16)
				printk("%s: xor_rd_idx %d not multiple of 16\n",__func__, xor_rd_idx);
#endif
			idx = xor_sw_rd_idx;
			tx = &sw_xor_desc[idx].async_tx;
			sw_desc = &sw_xor_desc[idx];

			for (j = 0; j < sw_desc->src_cnt; j++) {
				dma_unmap_page(NULL, sw_desc->dma_src[j], sw_desc->len, DMA_TO_DEVICE);
			}
			dma_unmap_page(NULL, sw_desc->dma_dest, sw_desc->len, DMA_BIDIRECTIONAL);

			comcerto_xor_ch.completed_cookie = tx->cookie;

			if (tx->callback) {
				tx->callback(tx->callback_param);
				tx->callback = NULL;
			}
//Patch by QNAP: mark message
#ifndef CONFIG_MACH_QNAPTS
			else
				printk("No Callback\n");
#endif
			smp_mb();		

			spin_lock_irqsave(&mdma_lock, flags);
			for(k = 0 ; k < split_size; k++)
			{
				xor_rd_idx = (xor_rd_idx + 1) % XOR_FDESC_COUNT;
			}
			spin_unlock_irqrestore(&mdma_lock, flags);

			xor_sw_rd_idx = (xor_sw_rd_idx + 1) % XOR_SW_FDESC_COUNT;
		}
	}
	else
	{
		if(cleanup_count)
			printk("%s: cleanup_count %d not multiple of 16\n",__func__, cleanup_count);
	}
}

static void comcerto_xor_tasklet(unsigned long data)
{
	spin_lock_bh(&comcerto_xor_ch.lock);

	comcerto_xor_cleanup();

	spin_unlock_bh(&comcerto_xor_ch.lock);
}

static void comcerto_xor_timer_fnc(unsigned long data)
{
	unsigned long           flags;

	spin_lock_irqsave(&mdma_lock, flags);

	if(comcerto_xor_sleeping)
		wake_up(&comcerto_xor_wait_queue);

	spin_unlock_irqrestore(&mdma_lock, flags);

	comcerto_xor_tasklet(0);

	mod_timer(&comcerto_xor_timer, jiffies + COMPLETION_TIMEOUT);

}


static void comcerto_xor_issue_pending(struct dma_chan *chan)
{
	unsigned long           flags;


	spin_lock_irqsave(&mdma_lock, flags);

	comcerto_dma_process();

	if(comcerto_xor_sleeping)
		wake_up(&comcerto_xor_wait_queue);

	spin_unlock_irqrestore(&mdma_lock, flags);

	comcerto_xor_tasklet(0);
}

static inline struct comcerto_sw_xor_desc *txd_to_comcerto_desc(struct dma_async_tx_descriptor *txd)
{
	return container_of(txd, struct comcerto_sw_xor_desc, async_tx);
}


static dma_cookie_t comcerto_xor_tx_submit(struct dma_async_tx_descriptor *tx)
{
	unsigned long flags;

	struct dma_chan *c = tx->chan;
	dma_cookie_t cookie;

	spin_lock_bh(&comcerto_xor_ch.lock);

	cookie = c->cookie;
	cookie++;
	if (cookie < 0)
		cookie = 1;
	tx->cookie = cookie;
	c->cookie = cookie;

	spin_lock_irqsave(&mdma_lock, flags);

	comcerto_xor_set_desc(xor_sw_wr_prev_idx, xor_wr_idx);
	xor_sw_wr_prev_idx = (xor_sw_wr_prev_idx + 1) % XOR_SW_FDESC_COUNT ;
	comcerto_dma_process();

	spin_unlock_irqrestore(&mdma_lock, flags);

	spin_unlock_bh(&comcerto_xor_ch.lock);

	if(comerto_xor_timer_first)
	{
		mod_timer(&comcerto_xor_timer, jiffies + COMPLETION_TIMEOUT);
		comerto_xor_timer_first = 0;
	}
	return cookie;
}

	struct dma_async_tx_descriptor *
comcerto_xor_prep_dma_xor(struct dma_chan *chan, dma_addr_t dest, dma_addr_t *srcs,
		unsigned int src_cnt, size_t len, unsigned long xor_flags)
{
	int desc_idx;
	int i;

	spin_lock_bh(&comcerto_xor_ch.lock);
	if(comcerto_xor_rb_full())
	{
		comcerto_xor_request_wait();
	}

	desc_idx = xor_sw_wr_idx;
	xor_sw_wr_idx = (xor_sw_wr_idx + 1) % XOR_SW_FDESC_COUNT ;

	sw_xor_desc[desc_idx].async_tx.flags = xor_flags;
	sw_xor_desc[desc_idx].src_cnt = src_cnt;
	sw_xor_desc[desc_idx].len = len;
	sw_xor_desc[desc_idx].dma_dest = dest;
	for(i = 0; i < src_cnt; i++)
		sw_xor_desc[desc_idx].dma_src[i] = srcs[i];

	spin_unlock_bh(&comcerto_xor_ch.lock);

	return &sw_xor_desc[desc_idx].async_tx;

}


static int comcerto_xor_alloc_chan_resources(struct dma_chan *chan)
{
	int i;

	for(i = 0; i < XOR_SW_FDESC_COUNT; i++) {
		memset(&sw_xor_desc[i], 0, sizeof(struct comcerto_sw_xor_desc));
		sw_xor_desc[i].async_tx.tx_submit = comcerto_xor_tx_submit;
		sw_xor_desc[i].async_tx.cookie = 0;
		dma_async_tx_descriptor_init(&sw_xor_desc[i].async_tx, chan);
	}
	return XOR_SW_FDESC_COUNT;
}

static void comcerto_xor_free_chan_resources(struct dma_chan *chan)
{
	//TODO

	printk("*** %s ***\n",__func__);

	return;
}

static enum dma_status comcerto_xor_status(struct dma_chan *chan,
		dma_cookie_t cookie,
		struct dma_tx_state *txstate)
{
	dma_cookie_t last_used;
	dma_cookie_t last_complete;

	last_used = chan->cookie;
	last_complete = comcerto_xor_ch.completed_cookie;
	dma_set_tx_state(txstate, last_complete, last_used, 0);

	return dma_async_is_complete(cookie, last_complete, last_used);
}


#if defined(CONFIG_COMCERTO_MDMA_PROF)

unsigned int mdma_time_counter[256];
unsigned int mdma_reqtime_counter[256];
unsigned int mdma_data_counter[256];
static struct timeval last_mdma;
unsigned int init_mdma_prof = 0;
unsigned int enable_mdma_prof = 0;

void comcerto_dma_profiling_start(struct comcerto_dma_sg *sg, unsigned int len)
{
	long diff_time_us;

	if (enable_mdma_prof) {
		do_gettimeofday(&sg->start);

		if (init_mdma_prof) {
			diff_time_us = ((sg->start.tv_sec - last_mdma.tv_sec) * 1000000 + sg->start.tv_usec - last_mdma.tv_usec) >> 4;
			if (diff_time_us < 256) {
				mdma_time_counter[diff_time_us]++;
			}
			else {
				mdma_time_counter[255]++;
			}
		}

		len >>= 13;

		if (len < 256)
			mdma_data_counter[len]++;
		else
			mdma_data_counter[255]++;
	}
}

void comcerto_dma_profiling_end(struct comcerto_dma_sg *sg)
{
	long diff_time_us;

	if (enable_mdma_prof) {
		do_gettimeofday(&sg->end);

		diff_time_us = ((sg->end.tv_sec - sg->start.tv_sec) * 1000000 + sg->end.tv_usec - sg->start.tv_usec) >> 4;
		if (diff_time_us < 256) {
			mdma_reqtime_counter[diff_time_us]++;
		}
		else
			mdma_reqtime_counter[255]++;

		if (!init_mdma_prof)
			init_mdma_prof = 1;

		last_mdma = sg->end;
	}
}

#else
void comcerto_dma_profiling_start(struct comcerto_dma_sg *sg, unsigned int len) {}
void comcerto_dma_profiling_end(struct comcerto_dma_sg *sg) {}
#endif


static inline dma_addr_t dma_acp_map_page(struct comcerto_dma_sg *sg, void *p, unsigned int len, int dir, int use_acp)
{
	dma_addr_t phys_addr = virt_to_phys(p);
	dma_addr_t low, high;

	if (!use_acp)
		goto map;

	if ((phys_addr >= sg->low_phys_addr) && (phys_addr + len) < sg->high_phys_addr)
	{
		/* In range, skip mapping */
		return COMCERTO_AXI_ACP_BASE + phys_addr;
	}

	/* Try to grow window, if possible */
	if (phys_addr < sg->low_phys_addr)
		low = phys_addr & ~(COMCERTO_AXI_ACP_SIZE - 1);
	else
		low = sg->low_phys_addr;

	if ((phys_addr + len) > sg->high_phys_addr)
		high = (phys_addr + len + COMCERTO_AXI_ACP_SIZE - 1) & ~(COMCERTO_AXI_ACP_SIZE - 1);
	else
		high = sg->high_phys_addr;

	if ((high - low) <= COMCERTO_AXI_ACP_SIZE) {
		sg->low_phys_addr = low;
		sg->high_phys_addr = high;

		return COMCERTO_AXI_ACP_BASE + phys_addr;
	}

map:
	return dma_map_single(NULL, p, len, dir); //TODO add proper checks
}


int comcerto_dma_sg_add_input(struct comcerto_dma_sg *sg, void *p, unsigned int len, int use_acp)
{
	dma_addr_t phys_addr;

	if (unlikely(len > (MDMA_MAX_BUF_SIZE + 1))) {
		printk(KERN_ERR "%s: tried to add a page larger than %d kB.\n", __func__, MDMA_MAX_BUF_SIZE + 1);
		return -2;
	}

	if (len <= MDMA_MAX_BUF_SIZE) {
		if (sg->input_idx >= MDMA_INBOUND_BUF_DESC)
			return -1;

		phys_addr = dma_acp_map_page(sg, p, len, DMA_TO_DEVICE, use_acp);

		sg->in_bdesc[sg->input_idx].split = 0;
		sg->in_bdesc[sg->input_idx].phys_addr = phys_addr;
		sg->in_bdesc[sg->input_idx].len = len;
		sg->input_idx++;

		if (sg->input_idx < MDMA_INBOUND_BUF_DESC)
			sg->in_bdesc[sg->input_idx].len = 0; // Clean-up will stop when reaching a zero length bdesc


		return 0;
	}
	else { /* len = MSPD_MDMA_MAX_BUF_SIZE +1, split it in 2 pieces */
		if (sg->input_idx >= (MDMA_INBOUND_BUF_DESC - 1))
			return -1;

		phys_addr = dma_acp_map_page(sg, p, len, DMA_TO_DEVICE, use_acp);

		sg->in_bdesc[sg->input_idx].split = 1;
		sg->in_bdesc[sg->input_idx].phys_addr = phys_addr;
		sg->in_bdesc[sg->input_idx].len = MDMA_SPLIT_BUF_SIZE;
		sg->input_idx++;
		sg->in_bdesc[sg->input_idx].split = 0;
		sg->in_bdesc[sg->input_idx].phys_addr = phys_addr + MDMA_SPLIT_BUF_SIZE;
		sg->in_bdesc[sg->input_idx].len = len - MDMA_SPLIT_BUF_SIZE;
		sg->input_idx++;

		if (sg->input_idx < MDMA_INBOUND_BUF_DESC)
			sg->in_bdesc[sg->input_idx].len = 0; // Clean-up will stop when reaching a zero length bdesc

		return 0;
	}
}
EXPORT_SYMBOL(comcerto_dma_sg_add_input);

int comcerto_dma_sg_add_output(struct comcerto_dma_sg *sg, void *p, unsigned int len, int use_acp)
{
	dma_addr_t phys_addr;

	if (unlikely(len > (MDMA_MAX_BUF_SIZE + 1))) {
		printk(KERN_ERR "%s: tried to add a page larger than %d kB.\n", __func__, MDMA_MAX_BUF_SIZE + 1);
		return -2;
	}

	if (len <= MDMA_MAX_BUF_SIZE) {
		if (sg->output_idx >= MDMA_OUTBOUND_BUF_DESC)
			return -1;

		phys_addr = dma_acp_map_page(sg, p, len, DMA_FROM_DEVICE, use_acp);

		sg->out_bdesc[sg->output_idx].split = 0;
		sg->out_bdesc[sg->output_idx].phys_addr = phys_addr;
		sg->out_bdesc[sg->output_idx].len = len;
		sg->output_idx++;

		if (sg->output_idx < MDMA_OUTBOUND_BUF_DESC)
			sg->out_bdesc[sg->output_idx].len = 0; // Clean-up will stop when reaching a zero length bdesc

		return 0;
	}
	else { /* len = MDMA_MAX_BUF_SIZE +1, split it in 2 pieces */
		if (sg->output_idx >= (MDMA_OUTBOUND_BUF_DESC - 1))
			return -1;

		phys_addr = dma_acp_map_page(sg, p, len, DMA_FROM_DEVICE, use_acp);

		sg->out_bdesc[sg->output_idx].split = 1;
		sg->out_bdesc[sg->output_idx].phys_addr = phys_addr;
		sg->out_bdesc[sg->output_idx].len = MDMA_SPLIT_BUF_SIZE;
		sg->output_idx++;
		sg->out_bdesc[sg->output_idx].split = 0;
		sg->out_bdesc[sg->output_idx].phys_addr = phys_addr + MDMA_SPLIT_BUF_SIZE;
		sg->out_bdesc[sg->output_idx].len = len - MDMA_SPLIT_BUF_SIZE;
		sg->output_idx++;

		if (sg->output_idx < MDMA_OUTBOUND_BUF_DESC)
			sg->out_bdesc[sg->output_idx].len = 0; // Clean-up will stop when reaching a zero length bdesc

		return 0;
	}
}
EXPORT_SYMBOL(comcerto_dma_sg_add_output);

void comcerto_dma_sg_setup(struct comcerto_dma_sg *sg, unsigned int len)
{
	int i;
	unsigned int remaining;

	comcerto_dma_profiling_start(sg, len);

	writel_relaxed(sg->low_phys_addr |
			AWUSER_COHERENT(WRITEBACK) | AWPROT(0x0) | AWCACHE(CACHEABLE | BUFFERABLE) |
			ARUSER_COHERENT(WRITEBACK) | ARPROT(0x0) | ARCACHE(CACHEABLE | BUFFERABLE),
			COMCERTO_GPIO_A9_ACP_CONF_REG);

	remaining = len;
	i = 0;
	while (remaining > sg->in_bdesc[i].len) {

		if (sg->in_bdesc[i].phys_addr >= COMCERTO_AXI_ACP_BASE)
			sg->in_bdesc[i].phys_addr -= sg->low_phys_addr;

		comcerto_dma_set_in_bdesc(i, sg->in_bdesc[i].phys_addr, sg->in_bdesc[i].len);
		remaining -= sg->in_bdesc[i].len;
		i++;
	}

	if (sg->in_bdesc[i].phys_addr >= COMCERTO_AXI_ACP_BASE)
		sg->in_bdesc[i].phys_addr -= sg->low_phys_addr;

	comcerto_dma_set_in_bdesc(i, sg->in_bdesc[i].phys_addr, remaining | BLAST);

	remaining = len;
	i = 0;

	while (remaining > sg->out_bdesc[i].len) {
		if (sg->out_bdesc[i].phys_addr >= COMCERTO_AXI_ACP_BASE)
			sg->out_bdesc[i].phys_addr -= sg->low_phys_addr;

		comcerto_dma_set_out_bdesc(i, sg->out_bdesc[i].phys_addr, sg->out_bdesc[i].len);
		remaining -= sg->out_bdesc[i].len;
		i++;
	}

	if (sg->out_bdesc[i].phys_addr >= COMCERTO_AXI_ACP_BASE)
		sg->out_bdesc[i].phys_addr -= sg->low_phys_addr;

	comcerto_dma_set_out_bdesc(i, sg->out_bdesc[i].phys_addr, remaining | BLAST);
}
EXPORT_SYMBOL(comcerto_dma_sg_setup);

void comcerto_dma_sg_cleanup(struct comcerto_dma_sg *sg)
{
	int i;

	i = 0;
	while ((i < MDMA_INBOUND_BUF_DESC) && sg->in_bdesc[i].len) {
		if (sg->in_bdesc[i].split) {
			sg->in_bdesc[i+1].phys_addr = sg->in_bdesc[i].phys_addr;
			sg->in_bdesc[i+1].len += sg->in_bdesc[i].len;
		}
		else {
			if (sg->in_bdesc[i].phys_addr < COMCERTO_AXI_ACP_BASE)
				dma_unmap_page(NULL, sg->in_bdesc[i].phys_addr, sg->in_bdesc[i].len, DMA_TO_DEVICE);
		}
		i++;
	}

	i = 0;
	while ((i < MDMA_OUTBOUND_BUF_DESC) && sg->out_bdesc[i].len) {
		if (sg->out_bdesc[i].split) {
			sg->out_bdesc[i+1].phys_addr = sg->out_bdesc[i].phys_addr;
			sg->out_bdesc[i+1].len += sg->out_bdesc[i].len;
		}
		else {
			if (sg->out_bdesc[i].phys_addr < COMCERTO_AXI_ACP_BASE)
				dma_unmap_page(NULL, sg->out_bdesc[i].phys_addr, sg->out_bdesc[i].len, DMA_FROM_DEVICE);
		}
		i++;
	}


	comcerto_dma_profiling_end(sg);
}
EXPORT_SYMBOL(comcerto_dma_sg_cleanup);
void comcerto_dma_get(void)
{
	unsigned long flags;
	DEFINE_WAIT(wait);

	spin_lock_irqsave(&mdma_lock, flags);

	if (dma_owned && (dma_owned != OWNER_MEMCPY_FREE)) {
		prepare_to_wait(&mdma_memcpy_busy_queue, &wait, TASK_UNINTERRUPTIBLE);

		memcpy_pending_count++;

		while (!dma_owned || !(dma_owned == OWNER_MEMCPY_FREE)) {
			spin_unlock_irqrestore(&mdma_lock, flags);
			schedule();
			spin_lock_irqsave(&mdma_lock, flags);
			prepare_to_wait(&mdma_memcpy_busy_queue, &wait, TASK_UNINTERRUPTIBLE);
		}

		memcpy_pending_count--;

		finish_wait(&mdma_memcpy_busy_queue, &wait);
	}
	dma_owned = OWNER_MEMCPY_BUSY;

	spin_unlock_irqrestore(&mdma_lock, flags);
}
EXPORT_SYMBOL(comcerto_dma_get);

void comcerto_dma_put(void)
{
#if 0
	unsigned long flags;
	spin_lock_irqsave(&mdma_lock, flags);
	dma_owned = 0;
	spin_unlock_irqrestore(&mdma_lock, flags);
#endif
}
EXPORT_SYMBOL(comcerto_dma_put);

/* Called once to setup common registers */
static void comcerto_dma_setup(void)
{
	/* IO2M_IRQ_ENABLE: Enable IRQ_IRQFDON*/
	writel_relaxed(IRQ_IRQFDON|IRQ_IRQFLEN|IRQ_IRQFTHLD|IRQ_IRQFLST, IO2M_IRQ_ENABLE);
	writel_relaxed(IRQ_IRQFDON|IRQ_IRQFLEN|IRQ_IRQFTHLD|IRQ_IRQFLST, M2IO_IRQ_ENABLE);

	writel_relaxed(FLENEN, M2IO_CONTROL);
	writel_relaxed(0xf | (0x3ff << 8), M2IO_BURST);
	writel_relaxed(FLENEN, IO2M_CONTROL);
	writel_relaxed(0xf | (0x3ff << 8), IO2M_BURST);
}


void comcerto_dma_start(void)
{
	mdma_done = 0;

	mdma_in_desc->next_desc = 0;
	mdma_in_desc->fcontrol = 0;
	mdma_in_desc->fstatus0 = 0;
	mdma_in_desc->fstatus1 = 0;

	// outbound
	mdma_out_desc->next_desc = 0;
	mdma_out_desc->fcontrol = 0;
	mdma_out_desc->fstatus0 = 0;
	mdma_out_desc->fstatus1 = 0;

	wmb();

	// Initialize the Outbound Head Pointer
	writel_relaxed(mdma_out_desc_phy, IO2M_HEAD);

	// Initialize the Inbound Head Pointer
	writel_relaxed(mdma_in_desc_phy, M2IO_HEAD);

	writel_relaxed(1, M2IO_FLEN);
	writel_relaxed(1, IO2M_FLEN);

	wmb();
}
EXPORT_SYMBOL(comcerto_dma_start);


void comcerto_dma_wait(void)
{
	DEFINE_WAIT(wait);

	prepare_to_wait(&mdma_done_queue, &wait, TASK_UNINTERRUPTIBLE);

	if (!mdma_done)
		schedule();

	finish_wait(&mdma_done_queue, &wait);
}
EXPORT_SYMBOL(comcerto_dma_wait);

static void comcerto_dump_regs(void)
{
	u32 val;

	val = readl_relaxed(M2IO_CONTROL);
	printk(KERN_ERR"M2IO_CONTROL         0x%8x.\n",val);

	val = readl_relaxed(M2IO_HEAD);
	printk(KERN_ERR"M2IO_HEAD            0x%8x.\n",val);

	val = readl_relaxed(M2IO_BURST);
	printk(KERN_ERR"M2IO_BURST           0x%8x.\n",val);

	val = readl_relaxed(M2IO_FLEN);
	printk(KERN_ERR"M2IO_FLEN            0x%8x.\n",val);

	val = readl_relaxed(M2IO_IRQ_ENABLE);
	printk(KERN_ERR"M2IO_IRQ_ENABLE      0x%8x.\n",val);

	val = readl_relaxed(M2IO_IRQ_STATUS);
	printk(KERN_ERR"M2IO_IRQ_STATUS      0x%8x.\n",val);

	val = readl_relaxed(M2IO_RESET);
	printk(KERN_ERR"M2IO_RESET           0x%8x.\n",val);

	val = readl_relaxed(IO2M_CONTROL);
	printk(KERN_ERR"IO2M_CONTROL         0x%8x.\n",val);

	val = readl_relaxed(IO2M_HEAD);
	printk(KERN_ERR"IO2M_HEAD            0x%8x.\n",val);

	val = readl_relaxed(IO2M_BURST);
	printk(KERN_ERR"IO2M_BURST           0x%8x.\n",val);

	val = readl_relaxed(IO2M_FLEN);
	printk(KERN_ERR"IO2M_FLEN            0x%8x.\n",val);

	val = readl_relaxed(IO2M_IRQ_ENABLE);
	printk(KERN_ERR"IO2M_IRQ_ENABLE      0x%8x.\n",val);

	val = readl_relaxed(IO2M_IRQ_STATUS);
	printk(KERN_ERR"IO2M_IRQ_STATUS      0x%8x.\n",val);

	val = readl_relaxed(IO2M_RESET);
	printk(KERN_ERR"IO2M_RESET           0x%8x.\n",val);
}

static irqreturn_t c2k_dma_handle_interrupt(int irq, void *data)
{
	int i;
	int pending_count;
	unsigned long flags;
	u32 intr_cause = readl_relaxed(IO2M_IRQ_STATUS);

	writel_relaxed(intr_cause, IO2M_IRQ_STATUS);

	if (unlikely(intr_cause & ~(IRQ_IRQFDON | IRQ_IRQFLST | IRQ_IRQFLEN))) {
		if (intr_cause & IRQ_IRQFRDYN)
			printk(KERN_ALERT "IRQFRDYN: A frame is started but the frame is not ready");

		if (intr_cause & IRQ_IRQFLSH)
			printk(KERN_ALERT "IRQFLSH: IO has more data than the memory buffer");

		if (intr_cause & IRQ_IRQFTHLD)
			printk(KERN_ALERT "IRQFTHLD: Frame threshold reached. FLEN=FTHLDL");

		if (intr_cause & IRQ_IRQFCTRL)
			printk(KERN_ALERT "IRQFCTRL: 1 frame is completed or when a frame is started but not ready");	

		comcerto_dump_regs();
	}

	if (intr_cause & IRQ_IRQFDON) {

	}

	if (intr_cause & IRQ_IRQFLEN) {

		spin_lock_irqsave(&mdma_lock, flags);
		if(!dma_owned)
			printk(KERN_ALERT " NULL MDMA Ownership !!!\n");


		if(dma_owned==OWNER_XOR_BUSY)
		{

			for(i = 0 ; i < xor_current_batch_count; i++)
				xor_dma_idx = (xor_dma_idx + 1) % XOR_FDESC_COUNT;

			xor_current_batch_count = 0;


			if(memcpy_pending_count)
			{
				dma_owned = OWNER_MEMCPY_FREE;
				wake_up(&mdma_memcpy_busy_queue);
			}
			else
			{
				pending_count = CIRC_CNT(xor_wr_idx, xor_dma_idx, XOR_FDESC_COUNT);
				if(pending_count)
				{
					dma_owned = OWNER_XOR_BUSY;
					xor_current_batch_count = pending_count;
					comcerto_xor_update_dma_flen(xor_current_batch_count);
					comcerto_xor_update_dma_head(xor_dma_idx);
				}
				else
				{
					dma_owned = 0;
				}
			}

			if(comcerto_xor_sleeping)
			{
				wake_up(&comcerto_xor_wait_queue);
			}

			tasklet_schedule(&comcerto_xor_ch.irq_tasklet);
		}
		else //memcpy
		{
			mdma_done = 1;
			wake_up(&mdma_done_queue);

			memcpy_processed_ongoing++;
			pending_count = CIRC_CNT(xor_wr_idx, xor_dma_idx, XOR_FDESC_COUNT);

			if(pending_count)
			{
				memcpy_processed_ongoing = 0;
				dma_owned = OWNER_XOR_BUSY;
				xor_current_batch_count = pending_count;
				comcerto_xor_update_dma_flen(xor_current_batch_count);
				comcerto_xor_update_dma_head(xor_dma_idx);

				if(comcerto_xor_sleeping)
				{
					wake_up(&comcerto_xor_wait_queue);
				}
			}
			else
			{
				if(memcpy_pending_count)
				{
					dma_owned = OWNER_MEMCPY_FREE;
					wake_up(&mdma_memcpy_busy_queue);
				}
				else
				{
					dma_owned = 0;
				}
			}

		}
		spin_unlock_irqrestore(&mdma_lock, flags);
	}

	return IRQ_HANDLED;
}

static int __devexit comcerto_dma_remove(struct platform_device *pdev)
{
	int irq;

	irq = platform_get_irq(pdev, 0);

	iounmap(virtbase);

	free_irq(irq, NULL);

	platform_set_drvdata(pdev, NULL);

	return 0;
}

static int __devinit comcerto_dma_probe(struct platform_device *pdev)
{
	struct resource      *io;
	int                  irq;
	void *memcpy_pool = (void *)IRAM_MEMORY_VADDR;
	void *xor_pool;
	int i;
	struct dma_device    *dma_dev;
	int ret = 0;

	/* Retrieve related resources(mem, irq) from platform_device */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!io)
		return -ENODEV;

	irq = platform_get_irq(pdev,0);
	if (irq < 0)
		return irq;

	ret = request_irq(irq, c2k_dma_handle_interrupt, 0, "MDMA", NULL);
	if (ret < 0)
		return ret;

	virtbase = ioremap(io->start, resource_size(io));
	if (!virtbase)
		goto err_free_irq;

	/* Initialize comcerto_xor_device */
	comcerto_xor_dev = devm_kzalloc(&pdev->dev, sizeof(struct comcerto_xor_device), GFP_KERNEL);

	if(!comcerto_xor_dev)
	{
		ret = -ENOMEM;
		goto err_free_remap;
	}

	dma_dev = &comcerto_xor_dev->device;
	INIT_LIST_HEAD(&dma_dev->channels);
	dma_cap_set(DMA_XOR,dma_dev->cap_mask);
	dma_dev->dev = &pdev->dev;
	dma_dev->device_alloc_chan_resources = comcerto_xor_alloc_chan_resources;
	dma_dev->device_free_chan_resources  = comcerto_xor_free_chan_resources;
	dma_dev->device_tx_status            = comcerto_xor_status;
	dma_dev->device_issue_pending        = comcerto_xor_issue_pending;
	dma_dev->device_prep_dma_xor         = comcerto_xor_prep_dma_xor;
	dma_dev->max_xor = XOR_MAX_SRC_CNT;
	platform_set_drvdata(pdev,comcerto_xor_dev);

	/* Initialize comcerto_xor_chan */
	comcerto_xor_ch.chan.device = dma_dev;
	list_add_tail(&comcerto_xor_ch.chan.device_node,&dma_dev->channels);

	ret = dma_async_device_register(dma_dev);
	if (unlikely(ret)) {
		printk(KERN_ERR "%s: Failed to register XOR DMA channel %d\n",__func__,ret);
		goto err_free_dma;
	}
	else
		printk(KERN_INFO "%s: XOR DMA channel registered\n",__func__);

	spin_lock_init(&comcerto_xor_ch.lock);
	spin_lock_init(&comcerto_xor_cleanup_lock);

	spin_lock_init(&mdma_lock);

	//Memcpy descriptor initializing
	mdma_in_desc = (struct comcerto_memcpy_inbound_fdesc *) (memcpy_pool);
	memcpy_pool += sizeof(struct comcerto_memcpy_inbound_fdesc);
	memcpy_pool = (void *)((unsigned long)(memcpy_pool + 15) & ~15);
	mdma_out_desc = (struct comcerto_memcpy_outbound_fdesc *) (memcpy_pool);
	memcpy_pool += sizeof(struct comcerto_memcpy_outbound_fdesc);
	memcpy_pool = (void *)((unsigned long)(memcpy_pool + 15) & ~15);

	mdma_in_desc_phy = virt_to_aram(mdma_in_desc);
	mdma_out_desc_phy = virt_to_aram(mdma_out_desc);

	//XOR descriptor initializing
	comcerto_xor_pool_virt = dma_alloc_coherent(NULL, XOR_FDESC_COUNT * (sizeof(struct comcerto_xor_inbound_fdesc) 
				+  sizeof(struct comcerto_xor_outbound_fdesc)), &comcerto_xor_pool_phy, GFP_KERNEL);
	xor_pool = comcerto_xor_pool_virt;

	for (i = 0; i < XOR_FDESC_COUNT; i++) {
		xor_in_fdesc[i] = (struct comcerto_xor_inbound_fdesc *) (xor_pool);
		xor_pool += sizeof(struct comcerto_xor_inbound_fdesc);
		xor_pool = (void *)((unsigned long)(xor_pool + 15) & ~15);
		xor_out_fdesc[i] = (struct comcerto_xor_outbound_fdesc *) (xor_pool);
		xor_pool += sizeof(struct comcerto_xor_outbound_fdesc);
		xor_pool = (void *)((unsigned long)(xor_pool + 15) & ~15);

		memset(xor_in_fdesc[i], 0 , sizeof(struct comcerto_xor_inbound_fdesc));
		memset(xor_out_fdesc[i], 0 , sizeof(struct comcerto_xor_outbound_fdesc));
	}

	for(i = 0; i < XOR_FDESC_COUNT - 1; i++) {
		xor_in_fdesc[i]->next_desc = virt_to_xor_dma(comcerto_xor_pool_phy, comcerto_xor_pool_virt, xor_in_fdesc[i+1]);
		xor_out_fdesc[i]->next_desc = virt_to_xor_dma(comcerto_xor_pool_phy, comcerto_xor_pool_virt, xor_out_fdesc[i+1]);
	}
	xor_in_fdesc[XOR_FDESC_COUNT-1]->next_desc = virt_to_xor_dma(comcerto_xor_pool_phy, comcerto_xor_pool_virt, xor_in_fdesc[0]);
	xor_out_fdesc[XOR_FDESC_COUNT-1]->next_desc = virt_to_xor_dma(comcerto_xor_pool_phy, comcerto_xor_pool_virt, xor_out_fdesc[0]);

	init_timer(&comcerto_xor_timer);
	comcerto_xor_timer.function = comcerto_xor_timer_fnc;
	comcerto_xor_timer.data = 0;

	tasklet_init(&comcerto_xor_ch.irq_tasklet, comcerto_xor_tasklet, 0);

	comcerto_dma_setup();

	goto out;

err_free_dma:
	platform_set_drvdata(pdev,NULL);
	kfree(comcerto_xor_dev);
err_free_remap:
	iounmap(virtbase);	
err_free_irq:
	free_irq(irq, NULL);
out:
	return ret;
}


static struct platform_driver comcerto_dma_driver = {
	.probe        = comcerto_dma_probe,
	.remove       = comcerto_dma_remove,
	.driver       = {
		.owner = THIS_MODULE,
		.name  = "comcerto_dma",
	},
};

static int __init comcerto_dma_init(void)
{
	return platform_driver_register(&comcerto_dma_driver);
}
module_init(comcerto_dma_init);

static void __exit comcerto_dma_exit(void)
{
	platform_driver_unregister(&comcerto_dma_driver);
}
module_exit(comcerto_dma_exit);

MODULE_DESCRIPTION("DMA engine driver for Mindspeed Comcerto C2000 devices");
MODULE_LICENSE("GPL");


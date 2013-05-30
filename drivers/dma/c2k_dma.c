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


static int mdma_busy = 0;
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

static DECLARE_WAIT_QUEUE_HEAD(mdma_busy_queue);
static DECLARE_WAIT_QUEUE_HEAD(mdma_done_queue);

unsigned long mdma_in_desc_phy;
unsigned long mdma_out_desc_phy;

struct comcerto_xor_inbound_fdesc *mdma_in_desc;
struct comcerto_xor_outbound_fdesc *mdma_out_desc;

EXPORT_SYMBOL(mdma_in_desc);
EXPORT_SYMBOL(mdma_out_desc);


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


static inline dma_addr_t dma_acp_map_page(struct comcerto_dma_sg *sg, struct page *page, unsigned int offset, unsigned int len, int dir, int use_acp)
{
	dma_addr_t phys_addr = page_to_phys(page) + offset;
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
	return dma_map_page(NULL, page, offset, len, dir); //TODO add proper checks
}


int comcerto_dma_sg_add_input(struct comcerto_dma_sg *sg, struct page *page, unsigned int offset, unsigned int len, int use_acp)
{
	dma_addr_t phys_addr;

	if (unlikely(len > (MDMA_MAX_BUF_SIZE + 1))) {
		printk(KERN_ERR "%s: tried to add a page larger than %d kB.\n", __func__, MDMA_MAX_BUF_SIZE + 1);
		return -2;
	}

	if (len <= MDMA_MAX_BUF_SIZE) {
		if (sg->input_idx >= MDMA_INBOUND_BUF_DESC)
			return -1;

		phys_addr = dma_acp_map_page(sg, page, offset, len, DMA_TO_DEVICE, use_acp);

		sg->in_bdesc[sg->input_idx].phys_addr = phys_addr;
		sg->in_bdesc[sg->input_idx].len = len;
		sg->input_idx++;

		return 0;
	}
	else { /* len = MSPD_MDMA_MAX_BUF_SIZE +1, split it in 2 pieces */
		if (sg->input_idx >= (MDMA_INBOUND_BUF_DESC - 1))
			return -1;

		phys_addr = dma_acp_map_page(sg, page, offset, len, DMA_TO_DEVICE, use_acp);

		sg->in_bdesc[sg->input_idx].phys_addr = phys_addr;
		sg->in_bdesc[sg->input_idx].len = MDMA_SPLIT_BUF_SIZE;
		sg->input_idx++;
		sg->in_bdesc[sg->input_idx].phys_addr = phys_addr + MDMA_SPLIT_BUF_SIZE;
		sg->in_bdesc[sg->input_idx].len = MDMA_SPLIT_BUF_SIZE;
		sg->input_idx++;

		return 0;
	}
}
EXPORT_SYMBOL(comcerto_dma_sg_add_input);

int comcerto_dma_sg_add_output(struct comcerto_dma_sg *sg, struct page *page, unsigned int offset, unsigned int len, int use_acp)
{
	dma_addr_t phys_addr;

	if (unlikely(len > (MDMA_MAX_BUF_SIZE + 1))) {
		printk(KERN_ERR "%s: tried to add a page larger than %d kB.\n", __func__, MDMA_MAX_BUF_SIZE + 1);
		return -2;
	}

	if (len <= MDMA_MAX_BUF_SIZE) {
		if (sg->output_idx >= MDMA_OUTBOUND_BUF_DESC)
			return -1;

		phys_addr = dma_acp_map_page(sg, page, offset, len, DMA_FROM_DEVICE, use_acp);

		sg->out_bdesc[sg->output_idx].phys_addr = phys_addr;
		sg->out_bdesc[sg->output_idx].len = len;
		sg->output_idx++;

		return 0;
	}
	else { /* len = MDMA_MAX_BUF_SIZE +1, split it in 2 pieces */
		if (sg->output_idx >= (MDMA_OUTBOUND_BUF_DESC - 1))
			return -1;

		phys_addr = dma_acp_map_page(sg, page, offset, len, DMA_FROM_DEVICE, use_acp);

		sg->out_bdesc[sg->output_idx].phys_addr = phys_addr;
		sg->out_bdesc[sg->output_idx].len = MDMA_SPLIT_BUF_SIZE;
		sg->output_idx++;
		sg->out_bdesc[sg->output_idx].phys_addr = phys_addr + MDMA_SPLIT_BUF_SIZE;
		sg->out_bdesc[sg->output_idx].len = MDMA_SPLIT_BUF_SIZE;
		sg->output_idx++;

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

void comcerto_dma_sg_cleanup(struct comcerto_dma_sg *sg, unsigned int len)
{
	int i;
	unsigned int remaining;

	remaining = len;
	i = 0;
	while (remaining > sg->in_bdesc[i].len) {
		if (sg->in_bdesc[i].phys_addr < COMCERTO_AXI_ACP_BASE)
			dma_unmap_page(NULL, sg->in_bdesc[i].phys_addr, sg->in_bdesc[i].len, DMA_TO_DEVICE);

		remaining -= sg->in_bdesc[i].len;
		i++;
	}

	if (sg->in_bdesc[i].phys_addr < COMCERTO_AXI_ACP_BASE)
		dma_unmap_page(NULL, sg->in_bdesc[i].phys_addr, sg->in_bdesc[i].len, DMA_TO_DEVICE);

	remaining = len;
	i = 0;
	while (remaining > sg->out_bdesc[i].len) {
		if (sg->out_bdesc[i].phys_addr < COMCERTO_AXI_ACP_BASE)
			dma_unmap_page(NULL, sg->out_bdesc[i].phys_addr, sg->out_bdesc[i].len, DMA_FROM_DEVICE);

		remaining -= sg->out_bdesc[i].len;
		i++;
	}

	if (sg->out_bdesc[i].phys_addr < COMCERTO_AXI_ACP_BASE)
		dma_unmap_page(NULL, sg->out_bdesc[i].phys_addr, sg->out_bdesc[i].len, DMA_FROM_DEVICE);


	comcerto_dma_profiling_end(sg);
}
EXPORT_SYMBOL(comcerto_dma_sg_cleanup);

void comcerto_dma_get(void)
{
	unsigned long flags;
	DEFINE_WAIT(wait);

	spin_lock_irqsave(&mdma_lock, flags);

	if (mdma_busy) {
		prepare_to_wait(&mdma_busy_queue, &wait, TASK_UNINTERRUPTIBLE);

		while (mdma_busy) {
			spin_unlock_irqrestore(&mdma_lock, flags);
			schedule();
			spin_lock_irqsave(&mdma_lock, flags);
			prepare_to_wait(&mdma_busy_queue, &wait, TASK_UNINTERRUPTIBLE);
		}

		finish_wait(&mdma_busy_queue, &wait);
	}

	mdma_busy = 1;

	spin_unlock_irqrestore(&mdma_lock, flags);
}
EXPORT_SYMBOL(comcerto_dma_get);

void comcerto_dma_put(void)
{
	unsigned long flags;

	spin_lock_irqsave(&mdma_lock, flags);
	mdma_busy = 0;
	spin_unlock_irqrestore(&mdma_lock, flags);

	wake_up(&mdma_busy_queue);
}
EXPORT_SYMBOL(comcerto_dma_put);

/* Called once to setup common registers */
static void comcerto_dma_setup(void)
{
	/* IO2M_IRQ_ENABLE: Enable IRQ_IRQFDON*/
	writel_relaxed(IRQ_IRQFDON, IO2M_IRQ_ENABLE);

	writel_relaxed(0x0, M2IO_CONTROL);
	writel_relaxed(0xf, M2IO_BURST);

	writel_relaxed(0x0, IO2M_CONTROL);
	writel_relaxed(0xf, IO2M_BURST);
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
		if (unlikely(!(mdma_out_desc->fstatus1 & FDONE_MASK)))
			printk(KERN_INFO "Fdesc not done\n");

		mdma_done = 1;
		wake_up(&mdma_done_queue);
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
	void *aram_pool = (void *)IRAM_MEMORY_VADDR;
	int ret;

	/* Retrieve related resources(mem, irq) from platform_device */
	io = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!io)
		return -ENODEV;

	irq = platform_get_irq(pdev,0);
	if (irq < 0)
		return irq;

	ret = request_irq(irq, c2k_dma_handle_interrupt, 0, "MDMA", NULL);
	if (ret < 0)
		goto err_irq;

	virtbase = ioremap(io->start, resource_size(io));
	if (!virtbase)
		goto err_ioremap;

	spin_lock_init(&mdma_lock);

	//initializing
	mdma_in_desc = (struct comcerto_xor_inbound_fdesc *) (aram_pool);
	aram_pool += sizeof(struct comcerto_xor_inbound_fdesc);
	aram_pool = (void *)((unsigned long)(aram_pool + 15) & ~15);
	mdma_out_desc = (struct comcerto_xor_outbound_fdesc *) (aram_pool);

	mdma_in_desc_phy = virt_to_aram(mdma_in_desc);
	mdma_out_desc_phy = virt_to_aram(mdma_out_desc);

	comcerto_dma_setup();

	return 0;

err_ioremap:
	free_irq(irq, NULL);

err_irq:
	return -1;
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


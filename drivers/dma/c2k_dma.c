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
#include <asm/io.h>
#include "mach/c2k_dma.h"

#define BUF_LEN (132 * 1024) /* For testing */

/* Buffer length. Maximum 64K-1 bytes */
#define SRC_SIZE 1460
#define DST_SIZE 4096

static int dmaread_done = 0;

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

static DECLARE_WAIT_QUEUE_HEAD(mdma_wait_queue);

unsigned long mdma_in_desc_phy;
unsigned long mdma_out_desc_phy;

EXPORT_SYMBOL(mdma_in_desc_phy);
EXPORT_SYMBOL(mdma_out_desc_phy);

struct comcerto_memcpy_inbound_fdesc *mdma_in_desc;
struct comcerto_memcpy_outbound_fdesc *mdma_out_desc;

void comcerto_dma_get(void)
{


}
EXPORT_SYMBOL(comcerto_dma_get);


void comcerto_dma_put(void)
{

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
	mdma_in_desc->next_desc = 0;
	mdma_in_desc->fcontrol = 0;
	mdma_in_desc->fstatus0 = 0;
	mdma_in_desc->fstatus1 = 0;

	// outbound
	mdma_out_desc->next_desc = 0;
	mdma_out_desc->fcontrol = 0;
	mdma_out_desc->fstatus0 = 0;
	mdma_out_desc->fstatus1 = 0;

	// Initialize the Outbound Head Pointer
	writel_relaxed(mdma_out_desc_phy, IO2M_HEAD);

	// Initialize the Inbound Head Pointer
	writel_relaxed(mdma_in_desc_phy, M2IO_HEAD);
}
EXPORT_SYMBOL(comcerto_dma_start);


int comcerto_dma_wait(void)
{
#if 0
	u32 status = 0;

	while(!status)
	{
		status = readl((u32)&mdma_out_desc->fstatus1);
	}
	printk("dma:: DMA is completed!!!!\n");


#else

#if 1
	if(wait_event_interruptible(mdma_wait_queue,  dmaread_done == 1))
	{
		return -ERESTARTSYS;
	}

	//TODO check fstatus1 values

#endif
#endif
}

static void mdma_transfer_single(u32 dst, u32 src, u32 size)
{
	struct timeval t0, t1, t2, diff;
	u32 size_now, count, offset;
	int i;

	do_gettimeofday(&t0);

	comcerto_dma_get();

	size_now = size;
	offset = 0;
	count = 0;

	while (size_now > SRC_SIZE) {
		comcerto_dma_set_in_bdesc(count, src + offset, SRC_SIZE);
		size_now -= SRC_SIZE;
		offset += SRC_SIZE;
		count++;
	}

	if (size_now)
		comcerto_dma_set_in_bdesc(count, src + offset, size_now | BLAST);

	size_now = size;
	offset = 0;
	count = 0;

	while (size_now > DST_SIZE) {
		comcerto_dma_set_out_bdesc(count, dst + offset, DST_SIZE);
		size_now -= DST_SIZE;
		offset += DST_SIZE;
		count++;
	}

	if (size_now)
		comcerto_dma_set_out_bdesc(count, dst + offset, size_now | BLAST);

	comcerto_dma_start();

	do_gettimeofday(&t1);

	comcerto_dma_wait();

	comcerto_dma_put();

	do_gettimeofday(&t2);

#if 0
	timersub(&t1, &t0, &diff);

	printk("dma:: %14s: %lu.%06lu [sec]\n", "Dma setup time",
	diff.tv_sec, (unsigned long) (diff.tv_usec));

	timersub(&t2, &t1, &diff);

	printk("dma:: %14s: %lu.%06lu [sec]\n", "Dma execution time",
	diff.tv_sec, (unsigned long) (diff.tv_usec));
#endif

}

static void start_dma_test(void)
{
	void *src, *dst;
	unsigned long src_pa, dst_pa;
	u32 size, i;
	struct timeval t0, t1, t2, diff;

	printk(KERN_ERR "dma test started \n");

	dst = kmalloc(BUF_LEN, GFP_DMA);
	src = kmalloc(BUF_LEN, GFP_DMA);

	printk (KERN_INFO "%s: dst=0x%x src=0x%x\n", __func__, dst, src);

	for (i = 0; i < BUF_LEN; i++)
		((u8 *)src)[i] = i;

	dst_pa = __pa(dst);
	src_pa = __pa(src);

	printk (KERN_INFO "%s: dst=0x%x src=0x%x\n", __func__, dst_pa, src_pa);

	mdma_transfer_single(dst_pa, src_pa, BUF_LEN);

	u32 memcmp_res = 0;
	memcmp_res = memcmp(dst, src, BUF_LEN);
	if (!memcmp_res)
		printk("dma: Source and Destination buffer contents are same. Size = %d \n", BUF_LEN);
	else
		printk("dma: Source and Destination buffer differ!!!! Size = %d \n", BUF_LEN);


	do_gettimeofday(&t0);
	memcpy(dst, src, BUF_LEN);
	do_gettimeofday(&t1);

#if 0
	timersub(&t1, &t0, &diff);

	printk("dma:: %14s: %lu.%06lu [sec]\n", "memcpy exec time",
	diff.tv_sec, (unsigned long) (diff.tv_usec));
#endif

	kfree(dst);
	kfree(src);
}

static void comcerto_dump_regs(void)
{
	u32 val;

	val = __raw_readl(M2IO_CONTROL);
	printk(KERN_ERR"M2IO_CONTROL         0x%8x.\n",val);

	val = __raw_readl(M2IO_HEAD);
	printk(KERN_ERR"M2IO_HEAD            0x%8x.\n",val);

	val = __raw_readl(M2IO_BURST);
	printk(KERN_ERR"M2IO_BURST           0x%8x.\n",val);

	val = __raw_readl(M2IO_FLEN);
	printk(KERN_ERR"M2IO_FLEN            0x%8x.\n",val);

	val = __raw_readl(M2IO_IRQ_ENABLE);
	printk(KERN_ERR"M2IO_IRQ_ENABLE      0x%8x.\n",val);

	val = __raw_readl(M2IO_IRQ_STATUS);
	printk(KERN_ERR"M2IO_IRQ_STATUS      0x%8x.\n",val);

	val = __raw_readl(M2IO_RESET);
	printk(KERN_ERR"M2IO_RESET           0x%8x.\n",val);

	val = __raw_readl(IO2M_CONTROL);
	printk(KERN_ERR"IO2M_CONTROL         0x%8x.\n",val);

	val = __raw_readl(IO2M_HEAD);
	printk(KERN_ERR"IO2M_HEAD            0x%8x.\n",val);

	val = __raw_readl(IO2M_BURST);
	printk(KERN_ERR"IO2M_BURST           0x%8x.\n",val);

	val = __raw_readl(IO2M_FLEN);
	printk(KERN_ERR"IO2M_FLEN            0x%8x.\n",val);

	val = __raw_readl(IO2M_IRQ_ENABLE);
	printk(KERN_ERR"IO2M_IRQ_ENABLE      0x%8x.\n",val);

	val = __raw_readl(IO2M_IRQ_STATUS);
	printk(KERN_ERR"IO2M_IRQ_STATUS      0x%8x.\n",val);

	val = __raw_readl(IO2M_RESET);
	printk(KERN_ERR"IO2M_RESET           0x%8x.\n",val);
}

static irqreturn_t c2k_dma_handle_interrupt(int irq, void *data)
{
	u32 intr_cause = __raw_readl(IO2M_IRQ_STATUS);

	if(intr_cause & IRQ_IRQFRDYN) {
		printk(KERN_ALERT "IRQFRDYN: A frame is started but the frame is not ready");
		comcerto_dump_regs( );
		__raw_writel(IRQ_IRQFRDYN, IO2M_IRQ_STATUS);
	}

	if(intr_cause & IRQ_IRQFLST) {
		__raw_writel(IRQ_IRQFLST, IO2M_IRQ_STATUS);
	}

	if(intr_cause & IRQ_IRQFDON) {
		if(readl((u32)&mdma_out_desc->fstatus1))
		__raw_writel(IRQ_IRQFDON, IO2M_IRQ_STATUS);
	}

	if(intr_cause & IRQ_IRQFLSH) {
		printk(KERN_ALERT "IRQFLSH: IO has more data than the memory buffer");
		comcerto_dump_regs( );
		__raw_writel(IRQ_IRQFLSH, IO2M_IRQ_STATUS);
	}

	if(intr_cause & IRQ_IRQFLEN) {
		__raw_writel(IRQ_IRQFLEN, IO2M_IRQ_STATUS);
	}

	if(intr_cause & IRQ_IRQFTHLD) {
		printk(KERN_ALERT "IRQFTHLD: Frame threshold reached. FLEN=FTHLDL");
		comcerto_dump_regs( );
		__raw_writel(IRQ_IRQFTHLD, IO2M_IRQ_STATUS);
	}

	if(intr_cause & IRQ_IRQFCTRL) {
		printk(KERN_ALERT "IRQFCTRL: 1 frame is completed or when a frame is started but not ready");
		comcerto_dump_regs( );
		__raw_writel(IRQ_IRQFCTRL, IO2M_IRQ_STATUS);
	}

//	wake up process
	if (intr_cause & IRQ_IRQFDON) {
		dmaread_done = 1;
		wake_up_interruptible(&mdma_wait_queue);
	}

	return IRQ_HANDLED;
}

static int comcerto_dma_remove(struct platform_device *pdev)
{
	int irq;

	irq = platform_get_irq(pdev,0);

	iounmap(virtbase);

	free_irq(irq, pdev->id);

	platform_set_drvdata(pdev,NULL);

	return 0;
}

static int comcerto_dma_probe(struct platform_device *pdev)
{
	struct resource      *io;
	int                  irq;
	void *aram_pool = IRAM_MEMORY_VADDR;
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

	//initializing
	mdma_in_desc = (struct comcerto_xor_inbound_fdesc *) (aram_pool);
	aram_pool += sizeof(struct comcerto_xor_inbound_fdesc);
	aram_pool = (u32)(aram_pool + 15) & ~15;
	mdma_out_desc = (struct comcerto_xor_outbound_fdesc *) (aram_pool);

	mdma_in_desc_phy = virt_to_aram(mdma_in_desc);
	mdma_out_desc_phy = virt_to_aram(mdma_out_desc);

	comcerto_dma_setup();

//	start_dma_test();

	return 0;

err_ioremap:
	free_irq(irq, pdev->id);

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


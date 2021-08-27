 
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/module.h>

#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/reboot.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#ifdef CONFIG_HW_MUTEXES
#include <linux/hw_mutex.h>
#endif
#include <linux/delay.h>

#include <linux/io.h>

#include "ce5xx_spi_flash.h"

#ifdef SYNO_EVANSPORT_FLASH_PART
#include "synoflashmap.h"
#endif
 
#define MB 			(1024*1024)
#define SIZE_4_MB	(4*MB)
#define SIZE_8_MB	(8*MB)
#define SIZE_16_MB	(16*MB)
#define SIZE_32_MB	(32*MB)
#define SIZE_64_MB	(64*MB)

#define BITS_OF_BYTES(bytenum)	(BITS_PER_BYTE*(bytenum))
 
static const struct pci_device_id ce5xx_sflash_pci_tbl[] __devinitdata = {
  { PCI_DEVICE( INTEL_VENDOR_ID, CE5XX_SPI_FLASH_CONTROLLER_ID), .driver_data = 1 },
  {0},
};
struct spi_board_info CE5xx_sflash_devices[] = {
  {
#ifdef SYNO_EVANSPORT_FLASH_PART
    .modalias = "m25p80",
    .platform_data = &spi_flashdata,
#else
    .modalias = "nmyx25",
#endif
    .chip_select = 0,
    .bus_num = 1,
  },
};

#ifdef  SPI_CE_DEBUG
#define spi_dbg(fmt, args...) do \
                             { \
                               printk(KERN_INFO fmt, ##args); \
                              } while(0)
                              
#define spi_dbg_func	spi_dbg("func %s, line %d\n",__FUNCTION__,__LINE__)
#else
#define spi_dbg(fmt,args...) do {} while(0)
#define spi_dbg_func	do {} while(0)
#endif

MODULE_DEVICE_TABLE(pci, ce5xx_sflash_pci_tbl);

static struct ce5xx_address_split addr_split_tbl[] = {

		{SIZE_4_MB,	0,		0x8,	0x0,	0x0,	0x0},
		{0,		SIZE_4_MB,	0x0,	0x0,	0x8,	0x0},		

		{SIZE_8_MB,	0,		0x8,	0x0,	0x0,	0x0},
		{0,		SIZE_8_MB,	0x0,	0x0,	0x8,	0x0},		

		{SIZE_16_MB,	0,		0x8,	0x0,	0x0,	0x0},
		{0, 	SIZE_16_MB,	0x0,	0x0,	0x8,	0x0},

		{SIZE_32_MB,	0,	0x8,	0x1,	0xA,	0x1},
		{0,	SIZE_32_MB,	0x8,	0x1,	0xA,	0x1},

		{SIZE_64_MB,	0,		0x8,	0x0,	0x0,	0x0},
		{0, 	SIZE_64_MB,	0x0,	0x0,	0x8,	0x0},		
		
		{SIZE_8_MB,	SIZE_8_MB,	0x8,	0x0,	0x9,	0x0},
		
		{SIZE_16_MB,	SIZE_16_MB,	0x8,	0x0,	0x9,	0x0},
		
		{SIZE_32_MB,	SIZE_32_MB,	0x8,	0x1,	0xA,	0x1},
		
		{},
};

static inline uint32_t flash_read32(void volatile *addr)
{
	return readl(addr);
}	

static inline void flash_write32(uint32_t data, void volatile *addr)
{
	return writel(data, addr);
}	
static struct ce5xx_address_split * match_address_split(struct flash_cs_info *cntl_data)
{
	int tmp = 0;
	uint32_t cs0_size = cntl_data->cs0_size;
	uint32_t cs1_size = cntl_data->cs1_size;
	struct ce5xx_address_split *split = NULL;	
	
	spi_dbg(" [%s] cs0 0x%x, cs1 0x%x\n", __FUNCTION__,cs0_size,cs1_size);

	for (tmp = 0; tmp < ARRAY_SIZE(addr_split_tbl) - 1; tmp++) {
		split = &addr_split_tbl[tmp];
		if ((split->cs0_size== cs0_size) && (split->cs1_size == cs1_size)){
			spi_dbg("found address split method \n");
			return &addr_split_tbl[tmp];
		}
	}
	return 0;
}
 
static int address_split_cfg(struct ce5xx_sflash *dev)
{
	struct ce5xx_address_split *split = dev->addr_split_methd;
	volatile void __iomem *reg = dev->regs_base;

	uint32_t write_data = 0;
	
	if (split)
		return 0;
	
	split = match_address_split(dev->cntl_data);
	if (!split)
		return -ENODEV;
	
	write_data = ((uint32_t)split->cs0_cmp<<CS0_CMP)|((uint32_t)split->cs0_mask<<CS0_MASK)|((uint32_t)split->cs1_cmp<<CS1_CMP)|((uint32_t)split->cs1_mask<<CS1_MASK);
	__raw_writel(write_data, reg + ADDR_SPLIT_REG);

	return 0;
}

static void ce5xx_sflash_turn_off_chip_sel(struct ce5xx_sflash *dev) 
{	

	volatile void __iomem *reg = dev->regs_base;
	if (dev->mode & SPI_MODE_QUAD_IO){
		__raw_writel(0, reg + HIGH_EFFICY_TRS_PAR_REG);
	}
	else{  
		__raw_writel(0, reg + DATA_COMMAND_REG);
	}

}
static void ce5xx_sflash_cs_enable(struct ce5xx_sflash *dev, int cs0_on, int cs1_on)
{
	volatile void __iomem *reg = dev->regs_base;
	uint32_t writeData = 0;
	uint32_t readData = 0;	
	readData = __raw_readl(reg + MODE_CONTL_REG);
	writeData = readData;
	
	if (cs0_on)
		writeData |= MODE_CONTL_CS0_MODE_ENABLE;
	else
		writeData &= (~MODE_CONTL_CS0_MODE_ENABLE);
	
	if (cs1_on)
		writeData |= MODE_CONTL_CS1_MODE_ENABLE;
	else
		writeData &= (~MODE_CONTL_CS1_MODE_ENABLE);
	
	if (writeData != readData)
		__raw_writel(writeData, reg + MODE_CONTL_REG);
}

static uint32_t ce5xx_sflash_unit_write(struct ce5xx_sflash* dev, const char * buf, size_t len)
{
	uint32_t writeData = 0;	
	volatile void __iomem *reg = dev->regs_base;
	uint32_t tmpData = 0;
	volatile uint8_t *src = (uint8_t *)&tmpData;
	uint32_t finished = 0;
	
	memcpy((void *)src,buf,len);
	if (dev->mode & SPI_MODE_QUAD_IO){
		 
		if ((!len) || (len > HETPR_NBYTES_WR_MAX))
			return 0;
		
		writeData = (uint32_t)((0x1<<HETPR_CS_HOLD)|(len<<HETPR_NBYTES_WR));
		__raw_writel(writeData, reg + HIGH_EFFICY_TRS_PAR_REG);
		printk("len %x, high efficiency write Data %x\n", len, writeData);

		writeData = __cpu_to_be32p((uint32_t *)src);
		__raw_writel(writeData, reg + HIGH_EFFICY_CMD_DATA_REG);
		printk("original data %x, len %x, write Data %x\n",*(uint32_t *)src, len, writeData);
		finished = len;
	}
	else{  
		 
		if ((!len) || ( len > DCR_NBYTES_MAX ))
			return 0;
		 
		writeData = __cpu_to_be32p((uint32_t *)src)>>BITS_PER_BYTE;

		 writeData = (uint32_t)((0x1<<DCR_CS_HOLD) | (len<<DCR_NBYTES) | (writeData & 0x00FFFFFF));

		__raw_writel(writeData, reg + DATA_COMMAND_REG);
		finished = len;
		 
	}

	return finished;
	}
 
static uint32_t sflash_unit_read_csr(struct ce5xx_sflash *dev, const u_char *buf, size_t len) 
{
	uint32_t readData = 0;
    uint32_t writeData = 0;	

	volatile void __iomem *reg = dev->regs_base;

	int num1 = 0, num2 = 0, i;
	volatile u_char *dst = (u_char *)&readData;
	uint32_t finished = 0;
	uint32_t tmp = 0;

	if (dev->mode & SPI_MODE_QUAD_IO){
		spi_dbg_func;

		if ((!len) || ( len > HETPR_NBYTES_RD_MAX))
			return 0;

		num1 = len/(sizeof(uint32_t));
		num2 = len%(sizeof(uint32_t));
		
		writeData = (uint32_t)((0x1<<HETPR_CS_HOLD)|(len<<HETPR_NBYTES_RD));
		__raw_writel(writeData, reg + HIGH_EFFICY_TRS_PAR_REG);

		if (num1){
			for (i = 0; i < num1; i++){
				readData =__raw_readl(reg + HIGH_EFFICY_CMD_DATA_REG)<<BITS_OF_BYTES(4-len);
				tmp = (*(uint32_t *)dst>>BITS_OF_BYTES(len))<<BITS_OF_BYTES(len);
				*(uint32_t *)dst = __be32_to_cpu(readData)|tmp;

				finished += sizeof(uint32_t);
			}
		}
		if (num2){
			readData = __raw_readl(reg + HIGH_EFFICY_CMD_DATA_REG)<<BITS_OF_BYTES(4-len);
			tmp = (*(uint32_t *)dst>>BITS_OF_BYTES(len))<<BITS_OF_BYTES(len);
			*(uint32_t *)dst = __be32_to_cpu(readData)|tmp;

			finished+=num2;
		}
	}
	else {

		if ((!len)||( len > DCR_NBYTES_MAX))
			return 0;
		__raw_writel((0x1<<DCR_CS_HOLD)|(len<<DCR_NBYTES),reg + DATA_COMMAND_REG);

	    readData = __raw_readl(reg + DATA_COMMAND_REG)<<BITS_OF_BYTES(4-len);
		readData = __be32_to_cpu(readData);
		finished+=len;
	}
	memcpy((void *)buf,(void *)dst,len);
	 
	return finished;
}

static uint32_t ce5xx_sflash_transmiter(struct ce5xx_sflash	 *dev, void *buf, size_t len)
{
	uint32_t remain = len;
	uint32_t finished = 0;
	u_char * src = (u_char *)buf;
	int limit = 0;
	int count =0;

	if ((!len) || (!src))
		return 0;
		
	if (dev->mode & SPI_MODE_QUAD_IO)
		limit = HETPR_NBYTES_WR_MAX;
	else
		limit = DCR_NBYTES_MAX;
	
	while (remain)
	{
		count = (remain>limit?limit:remain);
		count = ce5xx_sflash_unit_write(dev,src + finished,count);
		finished += count;
		remain -=count;
	}
 
	return finished;	   
}
 
static uint32_t ce5xx_sflash_csr_receiver(struct ce5xx_sflash *dev, void *buf, size_t len) 
{
	uint32_t remain = len;
	uint32_t finished = 0;
	u_char * src = (u_char *)buf;

	int limit = 0;
	int count =0;
	
	if (!len)
		return 0;
		
	if (dev->mode & SPI_MODE_QUAD_IO)
		limit = HETPR_NBYTES_RD_MAX;
	else
		limit = DCR_NBYTES_MAX;

	while (remain)
	{
		count = (remain>limit?limit:remain);	
		count = sflash_unit_read_csr(dev,src + finished,count);
		finished += count;
		remain -=count;
	}
	return finished;	   
}
 
static uint32_t ce5xx_sflash_dma_receiver(struct ce5xx_sflash *dev, void *to, uint32_t offset, size_t len) 
{	
	void __iomem *from;

	if ((!len) || (!to))
		return 0;
	
	from = dev->mem_base + offset;
	spi_dbg(" [%s], flash addr: 0x%x, to 0x%x, len 0x%x\n",__FUNCTION__,(uint32_t)(dev->mem_base + offset), len, to);
	memcpy(to,from, len);
	
	return len;	
}

static void ce5xx_sflash_cs_switch(struct ce5xx_sflash *dev, int cs_num)
{
	ce5xx_sflash_cs_enable(dev,(cs_num == 0),(cs_num == 1));
}
 
static void ce5xx_sflash_work_one(struct ce5xx_sflash *dev, struct spi_message *m)
{
	struct spi_device *spi = m->spi;
	struct spi_transfer *t;
		
	ce5xx_sflash_turn_off_chip_sel(dev); 
	list_for_each_entry(t, &m->transfers, transfer_list) {
		
		void *txbuf = (void *)t->tx_buf;
		void *rxbuf = (void *)t->rx_buf;
		uint32_t len = t->len;
		uint32_t	actual_len = 0;

		if (t->rx_buf && m->is_dma_mapped) {
			 
			ce5xx_sflash_cs_enable(dev,dev->cntl_data->cs0_size,dev->cntl_data->cs1_size);

			actual_len = dev->dma_receiver(dev,rxbuf,t->rx_dma,len);
		}
		else {
			
			ce5xx_sflash_cs_switch(dev, spi->chip_select);

			if (t->tx_buf)
				actual_len = dev->transmiter(dev,txbuf,len);
			if (t->rx_buf)
				actual_len = dev->csr_receiver(dev,rxbuf,len);

		}
		m->actual_length += actual_len;
	}
		spi_dbg_func;
	ce5xx_sflash_turn_off_chip_sel(dev);	

	m->status = 0;
	m->complete(m->context);
	return;
}

static void ce5xx_sflash_work(struct work_struct *work)
{
	struct ce5xx_sflash *c = container_of(work, struct ce5xx_sflash, work);
	unsigned long flags;

	spin_lock_irqsave(&c->lock, flags);
	while (!list_empty(&c->queue)) {
		struct spi_message *m;
		m = container_of(c->queue.next, struct spi_message, queue);
		list_del_init(&m->queue);
		spin_unlock_irqrestore(&c->lock, flags);
		ce5xx_sflash_work_one(c, m);
			spi_dbg_func;	
		spin_lock_irqsave(&c->lock, flags);
	}
	spin_unlock_irqrestore(&c->lock, flags);
	spi_dbg_func;

}

static int ce5xx_sflash_transfer(struct spi_device *spi, struct spi_message *m)
{
	struct spi_master *master = spi->master;
	struct ce5xx_sflash *c = spi_master_get_devdata(master);
	struct spi_transfer *t;
	unsigned long flags;

	m->actual_length = 0;

	list_for_each_entry (t, &m->transfers, transfer_list) {
		if (!t->len)
			return -EINVAL;
		else if (!t->tx_buf && !t->rx_buf)
			return -EINVAL;
	}

	spin_lock_irqsave(&c->lock, flags);
	if (c->status & (SPI_FLASH_SUSPEND | SPI_FLASH_REMOVE)) {
		spin_unlock_irqrestore(&c->lock, flags);
		return -ESHUTDOWN;
	}
	list_add_tail(&m->queue, &c->queue);
	queue_work(c->workqueue, &c->work);
	spin_unlock_irqrestore(&c->lock, flags);

	return 0;
}
 
static int __devinit ce5xx_sflash_set_up_default_mode(struct ce5xx_sflash *dev)
{
	uint32_t	mode_cntl = 0;

	void __iomem *reg = dev->regs_base;
	mode_cntl = __raw_readl(reg + MODE_CONTL_REG);
	 
	mode_cntl = (mode_cntl>>3<<3)|(0x1<<MODE_CONTL_CLK_RATIOR_SHIFT);
	 
#if	CE5XX_BOOT_MODE_ENABLE
	mode_cntl |= MODE_CONTL_BOOT_MODE_ENABLE;
#else
	mode_cntl &= MODE_CONTL_BOOT_MODE_DISABLE;
#endif
	 
	if (!(MODE_CONTL_SPI_UNIT_EN & mode_cntl)) 
		return -ENODEV;

	mode_cntl |= MODE_CONTL_SS1_EN;

	mode_cntl |= MODE_CONTL_CMD_WIDTH_EQUAL_TO_DATA;

	mode_cntl |= MODE_CONTL_SPI_WIDTH_1_BIT;

	mode_cntl |= MODE_CONTL_N_ADDR_3_BYTES;			

	mode_cntl |= MODE_CONTL_CS0_WP|MODE_CONTL_CS0_MODE_ENABLE|MODE_CONTL_CS1_MODE_ENABLE|MODE_CONTL_CS1_WP;	

	mode_cntl |= 0x4<<MODE_CONTL_CS_TAR_SHIFT;
	
	__raw_writel(mode_cntl, reg + MODE_CONTL_REG);

	spi_dbg("initial mode cntl 0x%x\n",mode_cntl);
	return 0;
}	

static int ce5xx_sflash_setup(struct spi_device *spi)
{
	uint32_t	mode_cntl = 0;
	struct ce5xx_sflash *dev = spi_master_get_devdata(spi->master) ;
	void __iomem *reg		 = dev->regs_base;
	unsigned long flags;

	dev->cntl_data			= spi->controller_data;
	dev->mode				= spi->mode;

	spi_dbg_func;

	spin_lock_irqsave(&dev->lock, flags);
	if (dev->status & (SPI_FLASH_SUSPEND | SPI_FLASH_REMOVE)) {
		spin_unlock_irqrestore(&dev->lock, flags);
		return -ESHUTDOWN;
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	 
	if (!dev->cntl_data)
		return ce5xx_sflash_set_up_default_mode(dev);
	 
	if (address_split_cfg(dev))
		return -ENODEV;
	
	mode_cntl	=	__raw_readl(reg + MODE_CONTL_REG);

	mode_cntl &= MODE_CONTL_N_ADDR_BYTES_MASK;
	switch (spi->bits_per_word>>3)
	{

		case 3:
			mode_cntl |= MODE_CONTL_N_ADDR_3_BYTES;			
			break;
		case 4:
			mode_cntl |= MODE_CONTL_N_ADDR_4_BYTES;			
			break;
		default:
			dev_err(&spi->dev, "Error: not supported yet addr_width %d\n",spi->bits_per_word>>3);
			return -ENODEV;
	}
	 
	mode_cntl &= MODE_CONTL_SPI_WIDTH_BIT_MASK;
	if (dev->mode & SPI_MODE_QUAD_IO)
		mode_cntl |= MODE_CONTL_SPI_WIDTH_4_BIT;
	else
		mode_cntl |= MODE_CONTL_SPI_WIDTH_1_BIT;

	mode_cntl &= (MODE_CONTL_CS0_MODE_DISABLE_MASK & MODE_CONTL_CS1_MODE_DISABLE_MASK);
	if (dev->cntl_data->cs0_size)
		mode_cntl |= MODE_CONTL_CS0_MODE_ENABLE;
	if (dev->cntl_data->cs1_size)
		mode_cntl |= MODE_CONTL_CS1_MODE_ENABLE;

	__raw_writel(mode_cntl, reg + MODE_CONTL_REG);
	return 0;
}
static int ce5xx_sflash_reboot(struct notifier_block *self, unsigned long event, void *data)
{
	return NOTIFY_DONE;
}
static struct notifier_block ce5xx_sflash_reboot_notifier = {
           .notifier_call   = ce5xx_sflash_reboot
};

static int __devinit ce5xx_sflash_probe (struct pci_dev *pdev,
                                const struct pci_device_id *id)
{
	struct spi_master *master;
	struct ce5xx_sflash *c;
	int ret = -ENODEV;

	if ((pdev->device != CE5XX_SPI_FLASH_CONTROLLER_ID) || (pdev->revision < CE5300_SPI_FLASH_REVISION_ID)){
		   return -ENODEV;		
	}
	else
		spi_dbg("found device 0x%08x, rev id 0x%08x\n", pdev->device, pdev->revision);

	ret = pci_enable_device(pdev);
	if (ret)
		 return ret;

	master = spi_alloc_master(&pdev->dev, sizeof(*c));
	if (!master) {
		ret = -ENOMEM;
		return ret;
	}
	c = spi_master_get_devdata(master);
	c->master = master;
	c->pdev	= pdev;

	pci_request_region(pdev, 0, "spi_flash_csr");
	pci_request_region(pdev, 1, "spi_flash_mem");

	c->regs_base = (void __iomem * )pci_ioremap_bar(pdev, 0);
	if (!c->regs_base){
		dev_err(&pdev->dev, "error, failed to ioremap sflash registers, regs_base %x\n",(uint32_t)c->regs_base);
		ret = -ENOMEM;
		goto out_release_master;
	}
	c->mem_base = (void __iomem * )pci_ioremap_bar(pdev, 1);
	if (!c->regs_base){
		dev_err(&pdev->dev, "error, failed to ioremap sflash mem space, mem_base %x\n",(uint32_t)c->mem_base);
		ret = -ENOMEM;
		goto out_release_bar0;
	}	

	dev_info(&pdev->dev, "csr iobase 0x%x, iosize 0x%x , mapped to 0x%x\n",(uint32_t)pci_resource_start(pdev,0),(uint32_t)pci_resource_len(pdev,0),(uint32_t)c->regs_base);
	dev_info(&pdev->dev, "mem iobase 0x%x, iosize 0x%x , mapped to 0x%x\n",(uint32_t)pci_resource_start(pdev,1),(uint32_t)pci_resource_len(pdev,1),(uint32_t)c->mem_base);

	INIT_WORK(&c->work, ce5xx_sflash_work);
	spin_lock_init(&c->lock);
	INIT_LIST_HEAD(&c->queue);

	c->workqueue = create_singlethread_workqueue(
					dev_name(master->dev.parent));
	if (!c->workqueue)
		goto out_free_region;
		
	master->bus_num 	= id->driver_data;
	master->setup 		= ce5xx_sflash_setup;
	master->transfer 	= ce5xx_sflash_transfer;
	master->num_chipselect = 2;  
	master->mode_bits	= SPI_MODE_0|SPI_MODE_QUAD_IO;

	pci_set_drvdata(pdev, c);

	c->transmiter	= ce5xx_sflash_transmiter;
	c->csr_receiver	= ce5xx_sflash_csr_receiver;
	c->dma_receiver	= ce5xx_sflash_dma_receiver;
	
	spi_register_board_info(CE5xx_sflash_devices,ARRAY_SIZE(CE5xx_sflash_devices));

	ret = spi_register_master(master);
	if (ret)
		goto out_unregister_board;

	c->reboot_notifier	= &ce5xx_sflash_reboot_notifier;
	register_reboot_notifier(c->reboot_notifier);
	return 0;

out_unregister_board:	
	spi_unregister_board_info(CE5xx_sflash_devices,1);
	pci_set_drvdata(pdev, NULL);

	if (c->workqueue)
		destroy_workqueue(c->workqueue);
out_free_region:	
	pci_release_region(pdev, 0);
	pci_release_region(pdev, 1);
	iounmap(c->mem_base);
out_release_bar0:
	iounmap(c->regs_base);

out_release_master:
	spi_master_put(master);
	pci_disable_device(pdev);
	return ret;
}

static void __devexit ce5xx_sflash_remove(struct pci_dev *pdev)
{
	struct ce5xx_sflash *c = pci_get_drvdata(pdev);
	unsigned long flags;
	spi_dbg_func;
	
	spin_lock_irqsave(&c->lock, flags);
	c->status |= SPI_FLASH_REMOVE;
	spin_unlock_irqrestore(&c->lock, flags);

	flush_workqueue(c->workqueue);

	unregister_reboot_notifier(c->reboot_notifier);
	spi_unregister_board_info(CE5xx_sflash_devices,1);	
	pci_set_drvdata(pdev,NULL);	
	if (c->workqueue)
		destroy_workqueue(c->workqueue);
	if (c->regs_base)
		iounmap(c->regs_base);
	if (c->mem_base)
		iounmap(c->mem_base);
	pci_release_region(pdev, 0);
	pci_release_region(pdev, 1);
	spi_unregister_master(c->master);
	pci_disable_device(pdev);

}

#ifdef CONFIG_PM
 static int ce5xx_sflash_device_suspend(struct device *dev)
 {
	struct ce5xx_sflash *c = dev_get_drvdata(dev);
	struct pci_dev *pdev = to_pci_dev(dev);
	unsigned long flags;
	int ret = 0;

	spin_lock_irqsave(&c->lock, flags);
	c->status |= SPI_FLASH_SUSPEND;
	spin_unlock_irqrestore(&c->lock, flags);
	
	flush_workqueue(c->workqueue);

	c->mode_contrl = flash_read32(c->regs_base + MODE_CONTL_REG);
	c->addr_split  = flash_read32(c->regs_base + ADDR_SPLIT_REG);
	c->inf_conf = flash_read32(c->regs_base + INTERFACE_CONFIG_REG);
	c->hetp = flash_read32(c->regs_base + HIGH_EFFICY_TRS_PAR_REG);
	c->heop = flash_read32(c->regs_base + HIGH_EFFICY_OPCODE_REG);

	pci_disable_device(pdev);
	pci_save_state(pdev);
	pci_set_power_state(pdev, PCI_D3hot);

	return ret;
 }
 
 static int ce5xx_sflash_device_resume(struct device *dev)
 {
	struct ce5xx_sflash *c = dev_get_drvdata(dev);
	struct pci_dev *pdev = to_pci_dev(dev);
	unsigned long flags;
	int ret = 0;

	pci_set_power_state(pdev, PCI_D0);
	pci_restore_state(pdev);
	pci_enable_device(pdev);

	flash_write32(c->mode_contrl, c->regs_base + MODE_CONTL_REG);
	flash_write32(c->addr_split, c->regs_base + ADDR_SPLIT_REG);
	flash_write32(c->inf_conf, c->regs_base + INTERFACE_CONFIG_REG);
	flash_write32(c->hetp & (0xF0000000), c->regs_base + HIGH_EFFICY_TRS_PAR_REG);
	flash_write32(c->heop, c->regs_base + HIGH_EFFICY_OPCODE_REG);

	spin_lock_irqsave(&c->lock, flags);
	c->status &= ~SPI_FLASH_SUSPEND;
	spin_unlock_irqrestore(&c->lock, flags);
	return ret;
 }

static const struct dev_pm_ops nmyx25_pm_ops = {
	.suspend    = ce5xx_sflash_device_suspend,
	.resume     = ce5xx_sflash_device_resume,
};
#endif

static struct pci_driver ce5xx_sflash_driver = {
	.name			= "ce5xx-spi-flash",
	.id_table		= ce5xx_sflash_pci_tbl,
	.probe			= ce5xx_sflash_probe,
	.remove			= __devexit_p(ce5xx_sflash_remove),
#ifdef CONFIG_PM
	.driver.pm		= &nmyx25_pm_ops,
#endif
};

static int __devinit ce5xx_sflash_init(void)
{
	printk(KERN_INFO "Intel(R) SPI FLASH CONTROLLER Driver built on %s @ %s\n", __DATE__, __TIME__);
	
	return pci_register_driver(&ce5xx_sflash_driver);

}
static void __devexit ce5xx_sflash_exit(void)
{
	pci_unregister_driver(&ce5xx_sflash_driver);
}

module_init(ce5xx_sflash_init);
module_exit(ce5xx_sflash_exit);

MODULE_DESCRIPTION("Intel(R) SPI FLASH CONTROLLER Driver");
MODULE_AUTHOR("Intel Corporation");
MODULE_LICENSE("GPL");

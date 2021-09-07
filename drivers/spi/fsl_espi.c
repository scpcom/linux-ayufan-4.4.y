#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef MY_DEF_HERE
 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/platform_device.h>
#include <linux/fsl_devices.h>
#include <linux/of_platform.h>
#include <linux/of_device.h>
#include <linux/of_spi.h>
#include <sysdev/fsl_soc.h>
#include <linux/io.h>

struct fsl_espi_reg {
	__be32 mode;
	__be32 event;
	__be32 mask;
	__be32 command;
	__be32 transmit;
	__be32 receive;
	__be32 res[2];
	__be32 csmode[4];
};

#define MAX_CS_NUM		4

#define CSMODE_CI_INACTIVEHIGH	(1 << 31)
#define CSMODE_CP_BEGIN_EDGECLK	(1 << 30)
#define CSMODE_REV		(1 << 29)
#define CSMODE_DIV16		(1 << 28)
#define CSMODE_PM(x)		((x) << 24)
#define CSMODE_POL_1		(1 << 20)
#define CSMODE_LEN(x)		((x) << 16)
#define CS_BEF(x)		((x) << 12)
#define CS_AFT(x)		((x) << 8)
#define CS_CG(x)		((x) << 3)

#define SPMODE_ENABLE		(1 << 31)
#define SPMODE_LOOP		(1 << 30)
#define SPIMODE_TXTHR(x)	((x) << 8)
#define SPIMODE_RXTHR(x)	((x) << 0)

#define CSMODE_INIT_VAL (CSMODE_POL_1 | CS_BEF(0) | CS_AFT(0) | CS_CG(1))

#define SPMODE_INIT_VAL (SPIMODE_TXTHR(4) | SPIMODE_RXTHR(3))

#define SPIE_RXT	0x00002000	 
#define SPIE_NE		0x00000200	 
#define SPIE_NF		0x00000100	 
#define SPIE_RXCNT(reg)     ((reg >> 24) & 0x3F)
#define SPIE_TXCNT(reg)     ((reg >> 16) & 0x3F)

#define SPIM_NE		0x00000200	 
#define SPIM_NF		0x00000100	 

#define MODEBITS	(SPI_CPOL | SPI_CPHA | SPI_CS_HIGH \
			| SPI_LSB_FIRST | SPI_LOOP)

struct fsl_espi {
	 
	struct spi_bitbang bitbang;
	struct fsl_espi_reg __iomem *regs;
	struct completion done;

	const void *tx;
	void *rx;

	void (*get_rx) (u32 rx_data, struct fsl_espi *);
	u32 (*get_tx) (struct fsl_espi *);

	int count;
	int irq;

	u32 spibrg;		 
	u32 rx_shift;		 
	u32 tx_shift;		 
};

struct fsl_espi_cs {
	 
	void (*get_rx) (u32 rx_data, struct fsl_espi *);
	u32 (*get_tx) (struct fsl_espi *);
	u32 rx_shift;		 
	u32 tx_shift;		 
	u32 hw_mode;		 
};

static void fsl_espi_rx_buf(u32 data, struct fsl_espi *fsl_espi)
{
	u32 *rx = fsl_espi->rx;
	*rx++ = (u32)(data >> fsl_espi->rx_shift);
	fsl_espi->rx = rx;
}

static u32 fsl_espi_tx_buf(struct fsl_espi *fsl_espi)
{
	u32 data;
	const u32 *tx = fsl_espi->tx;
	if (!tx)
		return 0;
	data = *tx++ << fsl_espi->tx_shift;
	fsl_espi->tx = tx;
	return data;
}

static
int fsl_espi_setup_transfer(struct spi_device *spi, struct spi_transfer *t)
{
	struct fsl_espi *fsl_espi;
	unsigned long flags;
	u32 regval;
	u8 bits_per_word, pm, cs_sel = spi->chip_select;
	u32 hz;
	struct fsl_espi_cs *cs = spi->controller_state;

	fsl_espi = spi_master_get_devdata(spi->master);

	if (t) {
		bits_per_word = t->bits_per_word;
		hz = t->speed_hz;
	} else {
		bits_per_word = 0;
		hz = 0;
	}

	if (!bits_per_word)
		bits_per_word = spi->bits_per_word;

	if (bits_per_word < 4 || bits_per_word > 16)
		return -EINVAL;
	bits_per_word -= 1;

	if (!hz)
		hz = spi->max_speed_hz;

	cs->rx_shift = 0;
	cs->tx_shift = 0;
	cs->get_rx = fsl_espi_rx_buf;
	cs->get_tx = fsl_espi_tx_buf;

	fsl_espi->rx_shift = cs->rx_shift;
	fsl_espi->tx_shift = cs->tx_shift;
	fsl_espi->get_rx = cs->get_rx;
	fsl_espi->get_tx = cs->get_tx;

	cs->hw_mode &= ~(CSMODE_LEN(0xF) | CSMODE_DIV16 | CSMODE_PM(0xF));

	cs->hw_mode |= CSMODE_LEN(bits_per_word) | CSMODE_INIT_VAL;

	if ((fsl_espi->spibrg / hz) > 32) {
		cs->hw_mode |= CSMODE_DIV16;
		pm = fsl_espi->spibrg / (hz * 32);
		if (pm > 16) {
			pm = 16;
			dev_err(&spi->dev, "Requested speed is too "
				"low: %d Hz. Will use %d Hz instead.\n",
				hz, fsl_espi->spibrg / 32 * 16);
		}
	} else
		pm = fsl_espi->spibrg / (hz * 2);
	if (pm)
		pm--;
	cs->hw_mode |= CSMODE_PM(pm);

	regval = in_be32(&fsl_espi->regs->mode);
	local_irq_save(flags);
	 
	out_be32(&fsl_espi->regs->mode, regval & ~SPMODE_ENABLE);
	out_be32(&fsl_espi->regs->csmode[cs_sel], cs->hw_mode);
	out_be32(&fsl_espi->regs->mode, regval);
	local_irq_restore(flags);

	return 0;
}

static int fsl_espi_bufs(struct spi_device *spi, struct spi_transfer *t)
{
	struct fsl_espi *fsl_espi;
	u32 word, len, bits_per_word;

	fsl_espi = spi_master_get_devdata(spi->master);

	fsl_espi->tx = t->tx_buf;
	fsl_espi->rx = t->rx_buf;
	bits_per_word = spi->bits_per_word;
	if (t->bits_per_word)
		bits_per_word = t->bits_per_word;
	len = t->len;
	fsl_espi->count = len;

	out_be32(&fsl_espi->regs->command,
			(spi->chip_select << 30) | (len - 1));
	INIT_COMPLETION(fsl_espi->done);

	out_be32(&fsl_espi->regs->mask, SPIM_NE);

	word = fsl_espi->get_tx(fsl_espi);
	out_be32(&fsl_espi->regs->transmit, word);

	wait_for_completion(&fsl_espi->done);

	return t->len;
}

static void fsl_espi_chipselect(struct spi_device *spi, int is_on)
{
	return;
}

static int fsl_espi_setup(struct spi_device *spi)
{
	struct fsl_espi *fsl_espi;
	int retval;
	u32 hw_mode;
	struct fsl_espi_cs	*cs = spi->controller_state;

	if (spi->mode & ~MODEBITS) {
		dev_dbg(&spi->dev, "setup: unsupported mode bits %x\n",
			spi->mode & ~MODEBITS);
		return -EINVAL;
	}

	if (!spi->max_speed_hz)
		return -EINVAL;

	if (!cs) {
		cs = kzalloc(sizeof(*cs), GFP_KERNEL);
		if (!cs)
			return -ENOMEM;
		spi->controller_state = cs;
	}

	fsl_espi = spi_master_get_devdata(spi->master);

	if (!spi->bits_per_word)
		spi->bits_per_word = 8;

	hw_mode = cs->hw_mode;  
	cs->hw_mode =
		in_be32(&fsl_espi->regs->csmode[spi->chip_select]);
	 
	cs->hw_mode &= ~(CSMODE_CP_BEGIN_EDGECLK | CSMODE_CI_INACTIVEHIGH
			 | CSMODE_REV);

	if (spi->mode & SPI_CPHA)
		cs->hw_mode |= CSMODE_CP_BEGIN_EDGECLK;
	if (spi->mode & SPI_CPOL)
		cs->hw_mode |= CSMODE_CI_INACTIVEHIGH;
	if (!(spi->mode & SPI_LSB_FIRST))
		cs->hw_mode |= CSMODE_REV;

	retval = fsl_espi_setup_transfer(spi, NULL);
	if (retval < 0) {
		cs->hw_mode = hw_mode;  
		return retval;
	}

	dev_dbg(&spi->dev, "%s, mode %d, %u bits/w, %u Hz\n",
		__func__, spi->mode & (SPI_CPOL | SPI_CPHA),
		spi->bits_per_word, spi->max_speed_hz);
	return 0;
}

irqreturn_t fsl_espi_irq(s32 irq, void *context_data)
{
	struct fsl_espi *fsl_espi = context_data;
	u32 event, rx_data, word;
	int ret;

	event = in_be32(&fsl_espi->regs->event);

	if (event & SPIE_NE) {
		 
		void *event_ptr = &fsl_espi->regs->event;
		int limit = min(4, fsl_espi->count);

		ret = spin_event_timeout(
			SPIE_RXCNT(event = in_be32(event_ptr)) >=  limit,
			500, 0);
		if (!ret)
			return IRQ_NONE;
		rx_data = in_be32(&fsl_espi->regs->receive);

		if (fsl_espi->rx)
			fsl_espi->get_rx(rx_data, fsl_espi);
	} else {
		 
		out_be32(&fsl_espi->regs->event, event);
		return IRQ_HANDLED;
	}

	fsl_espi->count -= 4;
	if (fsl_espi->count > 0) {
		if ((event & SPIE_NF) == 0) {
			 
			ret = spin_event_timeout((event =
				in_be32(&fsl_espi->regs->event)) & SPIE_NF,
				500, 0);
			if (!ret)
				return IRQ_NONE;
		}
		word = fsl_espi->get_tx(fsl_espi);
		out_be32(&fsl_espi->regs->transmit, word);
	} else {
		fsl_espi->count = 0;
		 
		out_be32(&fsl_espi->regs->mask, 0);
		complete(&fsl_espi->done);
	}

	out_be32(&fsl_espi->regs->event, event);

	return IRQ_HANDLED;
}

static void fsl_espi_cleanup(struct spi_device *spi)
{
	kfree(spi->controller_state);
	spi->controller_state = NULL;
}

static int __init fsl_espi_probe(struct of_device *ofdev,
					const struct of_device_id *match)
{
	struct spi_master *master;
	struct fsl_espi *fsl_espi;
	struct resource r_irq_struct;
	struct resource r_mem_struct;
	struct resource *r_irq = &r_irq_struct;
	struct resource *r_mem = &r_mem_struct;
	const u32 *prop;
	u32 regval;
	int ret = 0;
	int len;

	master = spi_alloc_master(&ofdev->dev, sizeof(struct fsl_espi));

	if (master == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	dev_set_drvdata(&ofdev->dev, master);

	ret = of_address_to_resource(ofdev->node, 0, r_mem);
	if (ret) {
		dev_warn(&ofdev->dev, "invalid address\n");
		goto free_master;
	}

	ret = of_irq_to_resource(ofdev->node, 0, r_irq);
	if (ret == NO_IRQ) {
		dev_warn(&ofdev->dev, "no IRQ found\n");
		goto free_master;
	}

	master->setup = fsl_espi_setup;
	master->cleanup = fsl_espi_cleanup;

	fsl_espi = spi_master_get_devdata(master);
	fsl_espi->bitbang.master = spi_master_get(master);
	fsl_espi->bitbang.chipselect = fsl_espi_chipselect;
	fsl_espi->bitbang.txrx_bufs = fsl_espi_bufs;
	fsl_espi->bitbang.setup_transfer = fsl_espi_setup_transfer;
	fsl_espi->get_rx = fsl_espi_rx_buf;
	fsl_espi->get_tx = fsl_espi_tx_buf;
	fsl_espi->spibrg = fsl_get_sys_freq() / 2;

	fsl_espi->rx_shift = 0;
	fsl_espi->tx_shift = 0;

	init_completion(&fsl_espi->done);

	if (!request_mem_region(r_mem->start,
				r_mem->end - r_mem->start + 1, "fsl-espi")) {
		ret = -ENXIO;
		dev_warn(&ofdev->dev, "memory request failure\n");
		goto free_master;
	}

	fsl_espi->regs = ioremap(r_mem->start, r_mem->end - r_mem->start + 1);
	if (fsl_espi->regs == NULL) {
		ret = -ENOMEM;
		goto put_master;
	}

	fsl_espi->irq = r_irq->start;
	if (fsl_espi->irq < 0) {
		ret = -ENXIO;
		goto unmap_io;
	}

	ret = request_irq(fsl_espi->irq, fsl_espi_irq,
			  0, "fsl-espi", fsl_espi);

	if (ret != 0)
		goto unmap_io;

	master->bus_num = -1;	 
	 
	prop = of_get_property(ofdev->node, "espi,num-ss-bits", &len);
	if (!prop || len < sizeof(*prop)) {
		dev_warn(&ofdev->dev, "no 'espi,num-ss-bits' property\n");
		goto free_irq;
	}
	master->num_chipselect = *prop;

	out_be32(&fsl_espi->regs->mode, 0);
	out_be32(&fsl_espi->regs->mask, 0);
	out_be32(&fsl_espi->regs->command, 0);
	out_be32(&fsl_espi->regs->event, 0xffffffff);

	regval = SPMODE_INIT_VAL | SPMODE_ENABLE;
	out_be32(&fsl_espi->regs->mode, regval);
	 
	for (ret = 0; ret < MAX_CS_NUM; ret++)
		out_be32(&fsl_espi->regs->csmode[ret],
				CSMODE_INIT_VAL);

	ret = spi_bitbang_start(&fsl_espi->bitbang);
	if (ret != 0) {
		dev_err(&ofdev->dev, "spi_bitbang_start FAILED\n");
		goto free_irq;
	}

	dev_info(&ofdev->dev,
	       "Freescale eSPI Controller driver at 0x%p (irq = %d)\n",
	       fsl_espi->regs, fsl_espi->irq);

	of_register_spi_devices(master, ofdev->node);

	return ret;

free_irq:
	free_irq(fsl_espi->irq, fsl_espi);
unmap_io:
	iounmap(fsl_espi->regs);
put_master:
	spi_master_put(master);
free_master:
	kfree(master);
err:
	return ret;
}

static int __exit fsl_espi_remove(struct of_device *ofdev)
{
	struct fsl_espi *fsl_espi;
	struct spi_master *master;
	struct resource r_mem;

	master = platform_get_drvdata(ofdev);
	fsl_espi = spi_master_get_devdata(master);

	spi_bitbang_stop(&fsl_espi->bitbang);

	free_irq(fsl_espi->irq, fsl_espi);
	iounmap(fsl_espi->regs);
	fsl_espi->regs = NULL;
	if (!of_address_to_resource(ofdev->node, 0, &r_mem))
		release_mem_region(r_mem.start, resource_size(&r_mem));
	dev_set_drvdata(&ofdev->dev, 0);
	spi_master_put(fsl_espi->bitbang.master);

	return 0;
}

#if defined(MY_DEF_HERE) && defined(CONFIG_PM)
static int fsl_espi_suspend(struct of_device *ofdev, pm_message_t pmsg)
{
	struct fsl_espi *fsl_espi;
	struct spi_master *master;

	master = platform_get_drvdata(ofdev);
	fsl_espi = spi_master_get_devdata(master);

	out_be32(&fsl_espi->regs->mode, (in_be32(&fsl_espi->regs->mode) & ~SPMODE_ENABLE));

	return 0;
}

static int fsl_espi_resume(struct of_device *ofdev)
{
	struct fsl_espi *fsl_espi;
	struct spi_master *master;

	master = platform_get_drvdata(ofdev);
	fsl_espi = spi_master_get_devdata(master);

	out_be32(&fsl_espi->regs->mode, (in_be32(&fsl_espi->regs->mode) | SPMODE_ENABLE));

	return 0;
}

#else
#define fsl_espi_suspend NULL
#define fsl_espi_resume  NULL
#endif

static struct of_device_id fsl_espi_of_match[] = {
	{ .compatible = "fsl,espi",},
	{}
};
MODULE_DEVICE_TABLE(of, fsl_espi_of_match);

MODULE_ALIAS("platform:fsl-espi");
static struct of_platform_driver fsl_espi_driver = {
	.owner = THIS_MODULE,
	.name = "fsl-espi",
	.match_table = fsl_espi_of_match,
	.probe = fsl_espi_probe,
	.remove = __exit_p(fsl_espi_remove),
	.driver = {
		.name = "fsl-espi",
		.owner = THIS_MODULE,
	},
#if defined(MY_DEF_HERE) && defined(CONFIG_PM)
	.suspend = fsl_espi_suspend,
	.resume  = fsl_espi_resume,
#endif
};

static int __init fsl_espi_init(void)
{
	return of_register_platform_driver(&fsl_espi_driver);
}

static void __exit fsl_espi_exit(void)
{
	of_unregister_platform_driver(&fsl_espi_driver);
}

module_init(fsl_espi_init);
module_exit(fsl_espi_exit);

MODULE_AUTHOR("Chen Gong <g.chen@freescale.com>, "
	      "Jerry Huang <Chang-Ming.Huang@freescale.com>");
MODULE_DESCRIPTION("Freescale eSPI Driver");
MODULE_LICENSE("GPL");
#endif  

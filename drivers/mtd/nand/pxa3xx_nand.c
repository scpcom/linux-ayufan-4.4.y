#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#if defined(MY_ABC_HERE)
#include <linux/of_mtd.h>

#if defined(CONFIG_ARCH_PXA) || defined(CONFIG_ARCH_MMP)
#define ARCH_HAS_DMA
#endif

#ifdef ARCH_HAS_DMA
#include <mach/dma.h>
#endif
#else  
#include <mach/dma.h>
#endif  

#include <linux/platform_data/mtd-nand-pxa3xx.h>

#if defined(MY_ABC_HERE)
#define NAND_DEV_READY_TIMEOUT  50
#endif  
#define	CHIP_DELAY_TIMEOUT	(2 * HZ/10)
#define NAND_STOP_DELAY		(2 * HZ/50)
#define PAGE_CHUNK_SIZE		(2048)

#if defined(MY_ABC_HERE)
 
#define INIT_BUFFER_SIZE	256
#endif  

#define NDCR		(0x00)  
#define NDTR0CS0	(0x04)  
#define NDTR1CS0	(0x0C)  
#define NDSR		(0x14)  
#define NDPCR		(0x18)  
#define NDBDR0		(0x1C)  
#define NDBDR1		(0x20)  
#if defined(MY_ABC_HERE)
#define NDECCCTRL	(0x28)  
#endif  
#define NDDB		(0x40)  
#define NDCB0		(0x48)  
#define NDCB1		(0x4C)  
#define NDCB2		(0x50)  

#define NDCR_SPARE_EN		(0x1 << 31)
#define NDCR_ECC_EN		(0x1 << 30)
#define NDCR_DMA_EN		(0x1 << 29)
#define NDCR_ND_RUN		(0x1 << 28)
#define NDCR_DWIDTH_C		(0x1 << 27)
#define NDCR_DWIDTH_M		(0x1 << 26)
#define NDCR_PAGE_SZ		(0x1 << 24)
#define NDCR_NCSX		(0x1 << 23)
#define NDCR_ND_MODE		(0x3 << 21)
#define NDCR_NAND_MODE   	(0x0)
#define NDCR_CLR_PG_CNT		(0x1 << 20)
#define NDCR_STOP_ON_UNCOR	(0x1 << 19)
#define NDCR_RD_ID_CNT_MASK	(0x7 << 16)
#define NDCR_RD_ID_CNT(x)	(((x) << 16) & NDCR_RD_ID_CNT_MASK)

#define NDCR_RA_START		(0x1 << 15)
#define NDCR_PG_PER_BLK		(0x1 << 14)
#define NDCR_ND_ARB_EN		(0x1 << 12)
#define NDCR_INT_MASK           (0xFFF)

#define NDSR_MASK		(0xfff)
#if defined(MY_ABC_HERE)
#define NDSR_ERR_CNT_OFF	(16)
#define NDSR_ERR_CNT_MASK       (0x1f)
#define NDSR_ERR_CNT(sr)	((sr >> NDSR_ERR_CNT_OFF) & NDSR_ERR_CNT_MASK)
#endif  
#define NDSR_RDY                (0x1 << 12)
#define NDSR_FLASH_RDY          (0x1 << 11)
#define NDSR_CS0_PAGED		(0x1 << 10)
#define NDSR_CS1_PAGED		(0x1 << 9)
#define NDSR_CS0_CMDD		(0x1 << 8)
#define NDSR_CS1_CMDD		(0x1 << 7)
#define NDSR_CS0_BBD		(0x1 << 6)
#define NDSR_CS1_BBD		(0x1 << 5)
#if defined(MY_ABC_HERE)
#define NDSR_UNCORERR		(0x1 << 4)
#define NDSR_CORERR		(0x1 << 3)
#else  
#define NDSR_DBERR		(0x1 << 4)
#define NDSR_SBERR		(0x1 << 3)
#endif  
#define NDSR_WRDREQ		(0x1 << 2)
#define NDSR_RDDREQ		(0x1 << 1)
#define NDSR_WRCMDREQ		(0x1)

#if defined(MY_ABC_HERE)
#define NDCB0_LEN_OVRD		(0x1 << 28)
#endif  
#define NDCB0_ST_ROW_EN         (0x1 << 26)
#define NDCB0_AUTO_RS		(0x1 << 25)
#define NDCB0_CSEL		(0x1 << 24)
#if defined(MY_ABC_HERE)
#define NDCB0_EXT_CMD_TYPE_MASK	(0x7 << 29)
#define NDCB0_EXT_CMD_TYPE(x)	(((x) << 29) & NDCB0_EXT_CMD_TYPE_MASK)
#endif  
#define NDCB0_CMD_TYPE_MASK	(0x7 << 21)
#define NDCB0_CMD_TYPE(x)	(((x) << 21) & NDCB0_CMD_TYPE_MASK)
#define NDCB0_NC		(0x1 << 20)
#define NDCB0_DBC		(0x1 << 19)
#define NDCB0_ADDR_CYC_MASK	(0x7 << 16)
#define NDCB0_ADDR_CYC(x)	(((x) << 16) & NDCB0_ADDR_CYC_MASK)
#define NDCB0_CMD2_MASK		(0xff << 8)
#define NDCB0_CMD1_MASK		(0xff)
#define NDCB0_ADDR_CYC_SHIFT	(16)

#if defined(MY_ABC_HERE)
#define EXT_CMD_TYPE_DISPATCH	6  
#define EXT_CMD_TYPE_NAKED_RW	5  
#define EXT_CMD_TYPE_READ	4  
#define EXT_CMD_TYPE_DISP_WR	4  
#define EXT_CMD_TYPE_FINAL	3  
#define EXT_CMD_TYPE_LAST_RW	1  
#define EXT_CMD_TYPE_MONO	0  
#endif  

#define nand_writel(info, off, val)	\
	__raw_writel((val), (info)->mmio_base + (off))

#define nand_readl(info, off)		\
	__raw_readl((info)->mmio_base + (off))

enum {
	ERR_NONE	= 0,
	ERR_DMABUSERR	= -1,
	ERR_SENDCMD	= -2,
#if defined(MY_ABC_HERE)
	ERR_UNCORERR	= -3,
	ERR_BBERR	= -4,
	ERR_CORERR	= -5,
#else  
	ERR_DBERR	= -3,
	ERR_BBERR	= -4,
	ERR_SBERR	= -5,
#endif  
};

enum {
	STATE_IDLE = 0,
	STATE_PREPARED,
	STATE_CMD_HANDLE,
	STATE_DMA_READING,
	STATE_DMA_WRITING,
	STATE_DMA_DONE,
	STATE_PIO_READING,
	STATE_PIO_WRITING,
	STATE_CMD_DONE,
	STATE_READY,
};

#if defined(MY_ABC_HERE)
enum pxa3xx_nand_variant {
	PXA3XX_NAND_VARIANT_PXA,
	PXA3XX_NAND_VARIANT_ARMADA370,
};
#endif  

#if defined(MY_ABC_HERE)
struct pxa3xx_nand_host {
	struct nand_chip	chip;
	struct mtd_info         *mtd;
	void			*info_data;

	int			use_ecc;
	int			cs;

	unsigned int		col_addr_cycles;
	unsigned int		row_addr_cycles;
	size_t			read_id_bytes;
};
#else  
struct pxa3xx_nand_host {
	struct nand_chip	chip;
	struct pxa3xx_nand_cmdset *cmdset;
	struct mtd_info         *mtd;
	void			*info_data;

	unsigned int		page_size;
	int			use_ecc;
	int			cs;

	unsigned int		col_addr_cycles;
	unsigned int		row_addr_cycles;
	size_t			read_id_bytes;

	uint32_t		reg_ndcr;
	uint32_t		ndtr0cs0;
	uint32_t		ndtr1cs0;
};
#endif  

struct pxa3xx_nand_info {
	struct nand_hw_control	controller;
	struct platform_device	 *pdev;

	struct clk		*clk;
	void __iomem		*mmio_base;
	unsigned long		mmio_phys;
#if defined(MY_ABC_HERE)
	struct completion	cmd_complete, dev_ready;
#else  
	struct completion	cmd_complete;
#endif  

	unsigned int 		buf_start;
	unsigned int		buf_count;
#if defined(MY_ABC_HERE)
	unsigned int		buf_size;
	unsigned int		data_buff_pos;
	unsigned int		oob_buff_pos;
#endif  

	int			drcmr_dat;
	int			drcmr_cmd;

	unsigned char		*data_buff;
	unsigned char		*oob_buff;
	dma_addr_t 		data_buff_phys;
	int 			data_dma_ch;
	struct pxa_dma_desc	*data_desc;
	dma_addr_t 		data_desc_addr;

	struct pxa3xx_nand_host *host[NUM_CHIP_SELECT];
	unsigned int		state;

#if defined(MY_ABC_HERE)
	 
	enum pxa3xx_nand_variant variant;

	int			cs;
	int			use_ecc;	 
	int			ecc_bch;	 
	int			use_dma;	 
	int			use_spare;	 
	int			need_wait;

	unsigned int		data_size;	 
	unsigned int		chunk_size;	 
	unsigned int		oob_size;
	unsigned int		spare_size;
	unsigned int		ecc_size;
	unsigned int		ecc_err_cnt;
	unsigned int		max_bitflips;
	int 			retcode;

	uint32_t		reg_ndcr;
	uint32_t		ndtr0cs0;
	uint32_t		ndtr1cs0;

	uint32_t		ndcb0;
	uint32_t		ndcb1;
	uint32_t		ndcb2;
	uint32_t		ndcb3;
#else  
	int			cs;
	int			use_ecc;	 
	int			use_dma;	 
	int			is_ready;

	unsigned int		page_size;	 
	unsigned int		data_size;	 
	unsigned int		oob_size;
	int 			retcode;

	uint32_t		ndcb0;
	uint32_t		ndcb1;
	uint32_t		ndcb2;
#endif  
};

static bool use_dma = 1;
module_param(use_dma, bool, 0444);
MODULE_PARM_DESC(use_dma, "enable DMA for data transferring to/from NAND HW");

#if defined(MY_ABC_HERE)
 
#else  
 
static struct pxa3xx_nand_cmdset default_cmdset = {
	.read1		= 0x3000,
	.read2		= 0x0050,
	.program	= 0x1080,
	.read_status	= 0x0070,
	.read_id	= 0x0090,
	.erase		= 0xD060,
	.reset		= 0x00FF,
	.lock		= 0x002A,
	.unlock		= 0x2423,
	.lock_status	= 0x007A,
};
#endif  

static struct pxa3xx_nand_timing timing[] = {
	{ 40, 80, 60, 100, 80, 100, 90000, 400, 40, },
	{ 10,  0, 20,  40, 30,  40, 11123, 110, 10, },
	{ 10, 25, 15,  25, 15,  30, 25000,  60, 10, },
	{ 10, 35, 15,  25, 15,  25, 25000,  60, 10, },
};

static struct pxa3xx_nand_flash builtin_flash_types[] = {
{ "DEFAULT FLASH",      0,   0, 2048,  8,  8,    0, &timing[0] },
{ "64MiB 16-bit",  0x46ec,  32,  512, 16, 16, 4096, &timing[1] },
{ "256MiB 8-bit",  0xdaec,  64, 2048,  8,  8, 2048, &timing[1] },
{ "4GiB 8-bit",    0xd7ec, 128, 4096,  8,  8, 8192, &timing[1] },
{ "128MiB 8-bit",  0xa12c,  64, 2048,  8,  8, 1024, &timing[2] },
{ "128MiB 16-bit", 0xb12c,  64, 2048, 16, 16, 1024, &timing[2] },
{ "512MiB 8-bit",  0xdc2c,  64, 2048,  8,  8, 4096, &timing[2] },
{ "512MiB 16-bit", 0xcc2c,  64, 2048, 16, 16, 4096, &timing[2] },
{ "256MiB 16-bit", 0xba20,  64, 2048, 16, 16, 2048, &timing[3] },
};

#if defined(MY_ABC_HERE)
static u8 bbt_pattern[] = {'M', 'V', 'B', 'b', 't', '0' };
static u8 bbt_mirror_pattern[] = {'1', 't', 'b', 'B', 'V', 'M' };

static struct nand_bbt_descr bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		 
	.pattern = bbt_pattern
};

static struct nand_bbt_descr bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	8,
	.len = 6,
	.veroffs = 14,
	.maxblocks = 8,		 
	.pattern = bbt_mirror_pattern
};

static struct nand_ecclayout ecc_layout_2KB_bch4bit = {
	.eccbytes = 32,
	.eccpos = {
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = { {2, 30} }
};

static struct nand_ecclayout ecc_layout_4KB_bch4bit = {
	.eccbytes = 64,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63,
		96,  97,  98,  99,  100, 101, 102, 103,
		104, 105, 106, 107, 108, 109, 110, 111,
		112, 113, 114, 115, 116, 117, 118, 119,
		120, 121, 122, 123, 124, 125, 126, 127},
	 
	.oobfree = { {6, 26}, { 64, 32} }
};

static struct nand_ecclayout ecc_layout_4KB_bch8bit = {
	.eccbytes = 128,
	.eccpos = {
		32,  33,  34,  35,  36,  37,  38,  39,
		40,  41,  42,  43,  44,  45,  46,  47,
		48,  49,  50,  51,  52,  53,  54,  55,
		56,  57,  58,  59,  60,  61,  62,  63},
	.oobfree = { }
};
#endif  

#define DEFAULT_FLASH_TYPE (&builtin_flash_types[0])

#if defined(MY_ABC_HERE)
 
#else  
const char *mtd_names[] = {"pxa3xx_nand-0", "pxa3xx_nand-1", NULL};
#endif  

#define NDTR0_tCH(c)	(min((c), 7) << 19)
#define NDTR0_tCS(c)	(min((c), 7) << 16)
#define NDTR0_tWH(c)	(min((c), 7) << 11)
#define NDTR0_tWP(c)	(min((c), 7) << 8)
#define NDTR0_tRH(c)	(min((c), 7) << 3)
#define NDTR0_tRP(c)	(min((c), 7) << 0)

#define NDTR1_tR(c)	(min((c), 65535) << 16)
#define NDTR1_tWHR(c)	(min((c), 15) << 4)
#define NDTR1_tAR(c)	(min((c), 15) << 0)

#define ns2cycle(ns, clk)	(int)((ns) * (clk / 1000000) / 1000)

#if defined(MY_ABC_HERE)
static struct of_device_id pxa3xx_nand_dt_ids[] = {
	{
		.compatible = "marvell,pxa3xx-nand",
		.data       = (void *)PXA3XX_NAND_VARIANT_PXA,
	},
	{
		.compatible = "marvell,armada370-nand",
		.data       = (void *)PXA3XX_NAND_VARIANT_ARMADA370,
	},
	{}
};
MODULE_DEVICE_TABLE(of, pxa3xx_nand_dt_ids);

static enum pxa3xx_nand_variant
pxa3xx_nand_get_variant(struct platform_device *pdev)
{
	const struct of_device_id *of_id =
			of_match_device(pxa3xx_nand_dt_ids, &pdev->dev);
	if (!of_id)
		return PXA3XX_NAND_VARIANT_PXA;
	return (enum pxa3xx_nand_variant)of_id->data;
}
#endif  

static void pxa3xx_nand_set_timing(struct pxa3xx_nand_host *host,
				   const struct pxa3xx_nand_timing *t)
{
	struct pxa3xx_nand_info *info = host->info_data;
	unsigned long nand_clk = clk_get_rate(info->clk);
	uint32_t ndtr0, ndtr1;

	ndtr0 = NDTR0_tCH(ns2cycle(t->tCH, nand_clk)) |
		NDTR0_tCS(ns2cycle(t->tCS, nand_clk)) |
		NDTR0_tWH(ns2cycle(t->tWH, nand_clk)) |
		NDTR0_tWP(ns2cycle(t->tWP, nand_clk)) |
		NDTR0_tRH(ns2cycle(t->tRH, nand_clk)) |
		NDTR0_tRP(ns2cycle(t->tRP, nand_clk));

	ndtr1 = NDTR1_tR(ns2cycle(t->tR, nand_clk)) |
		NDTR1_tWHR(ns2cycle(t->tWHR, nand_clk)) |
		NDTR1_tAR(ns2cycle(t->tAR, nand_clk));

#if defined(MY_ABC_HERE)
	info->ndtr0cs0 = ndtr0;
	info->ndtr1cs0 = ndtr1;
#else  
	host->ndtr0cs0 = ndtr0;
	host->ndtr1cs0 = ndtr1;
#endif  
	nand_writel(info, NDTR0CS0, ndtr0);
	nand_writel(info, NDTR1CS0, ndtr1);
}

#if defined(MY_ABC_HERE)
 
static void pxa3xx_set_datasize(struct pxa3xx_nand_info *info,
				struct mtd_info *mtd)
{
	int oob_enable = info->reg_ndcr & NDCR_SPARE_EN;

	info->data_size = mtd->writesize;
	if (!oob_enable)
		return;

	info->oob_size = info->spare_size;
	if (!info->use_ecc)
		info->oob_size += info->ecc_size;
}
#else  
static void pxa3xx_set_datasize(struct pxa3xx_nand_info *info)
{
	struct pxa3xx_nand_host *host = info->host[info->cs];
	int oob_enable = host->reg_ndcr & NDCR_SPARE_EN;

	info->data_size = host->page_size;
	if (!oob_enable) {
		info->oob_size = 0;
		return;
	}

	switch (host->page_size) {
	case 2048:
		info->oob_size = (info->use_ecc) ? 40 : 64;
		break;
	case 512:
		info->oob_size = (info->use_ecc) ? 8 : 16;
		break;
	}
}
#endif  

static void pxa3xx_nand_start(struct pxa3xx_nand_info *info)
{
#if defined(MY_ABC_HERE)
	uint32_t ndcr;

	ndcr = info->reg_ndcr;

	if (info->use_ecc) {
		ndcr |= NDCR_ECC_EN;
		if (info->ecc_bch)
			nand_writel(info, NDECCCTRL, 0x1);
	} else {
		ndcr &= ~NDCR_ECC_EN;
		if (info->ecc_bch)
			nand_writel(info, NDECCCTRL, 0x0);
	}

	if (info->use_dma)
		ndcr |= NDCR_DMA_EN;
	else
		ndcr &= ~NDCR_DMA_EN;

	if (info->use_spare)
		ndcr |= NDCR_SPARE_EN;
	else
		ndcr &= ~NDCR_SPARE_EN;
#else  
	struct pxa3xx_nand_host *host = info->host[info->cs];
	uint32_t ndcr;

	ndcr = host->reg_ndcr;
	ndcr |= info->use_ecc ? NDCR_ECC_EN : 0;
	ndcr |= info->use_dma ? NDCR_DMA_EN : 0;
#endif  
	ndcr |= NDCR_ND_RUN;

	nand_writel(info, NDCR, 0);
	nand_writel(info, NDSR, NDSR_MASK);
	nand_writel(info, NDCR, ndcr);
}

static void pxa3xx_nand_stop(struct pxa3xx_nand_info *info)
{
	uint32_t ndcr;
	int timeout = NAND_STOP_DELAY;

	ndcr = nand_readl(info, NDCR);
	while ((ndcr & NDCR_ND_RUN) && (timeout-- > 0)) {
		ndcr = nand_readl(info, NDCR);
		udelay(1);
	}

	if (timeout <= 0) {
		ndcr &= ~NDCR_ND_RUN;
		nand_writel(info, NDCR, ndcr);
	}
	 
	nand_writel(info, NDSR, NDSR_MASK);
}

#if defined(MY_ABC_HERE)
static void __maybe_unused
enable_int(struct pxa3xx_nand_info *info, uint32_t int_mask)
#else  
static void enable_int(struct pxa3xx_nand_info *info, uint32_t int_mask)
#endif  
{
	uint32_t ndcr;

	ndcr = nand_readl(info, NDCR);
	nand_writel(info, NDCR, ndcr & ~int_mask);
}

static void disable_int(struct pxa3xx_nand_info *info, uint32_t int_mask)
{
	uint32_t ndcr;

	ndcr = nand_readl(info, NDCR);
	nand_writel(info, NDCR, ndcr | int_mask);
}

#if defined(MY_ABC_HERE)
static void handle_data_pio(struct pxa3xx_nand_info *info)
{
	unsigned int do_bytes = min(info->data_size, info->chunk_size);

	switch (info->state) {
	case STATE_PIO_WRITING:
		__raw_writesl(info->mmio_base + NDDB,
			      info->data_buff + info->data_buff_pos,
			      DIV_ROUND_UP(do_bytes, 4));

		if (info->oob_size > 0)
			__raw_writesl(info->mmio_base + NDDB,
				      info->oob_buff + info->oob_buff_pos,
				      DIV_ROUND_UP(info->oob_size, 4));
		break;
	case STATE_PIO_READING:
		__raw_readsl(info->mmio_base + NDDB,
			     info->data_buff + info->data_buff_pos,
			     DIV_ROUND_UP(do_bytes, 4));

		if (info->oob_size > 0)
			__raw_readsl(info->mmio_base + NDDB,
				     info->oob_buff + info->oob_buff_pos,
				     DIV_ROUND_UP(info->oob_size, 4));
		break;
	default:
		dev_err(&info->pdev->dev, "%s: invalid state %d\n", __func__,
				info->state);
		BUG();
	}

	info->data_buff_pos += do_bytes;
	info->oob_buff_pos += info->oob_size;
	info->data_size -= do_bytes;
}
#else  
static void handle_data_pio(struct pxa3xx_nand_info *info)
{
	switch (info->state) {
	case STATE_PIO_WRITING:
		__raw_writesl(info->mmio_base + NDDB, info->data_buff,
				DIV_ROUND_UP(info->data_size, 4));
		if (info->oob_size > 0)
			__raw_writesl(info->mmio_base + NDDB, info->oob_buff,
					DIV_ROUND_UP(info->oob_size, 4));
		break;
	case STATE_PIO_READING:
		__raw_readsl(info->mmio_base + NDDB, info->data_buff,
				DIV_ROUND_UP(info->data_size, 4));
		if (info->oob_size > 0)
			__raw_readsl(info->mmio_base + NDDB, info->oob_buff,
					DIV_ROUND_UP(info->oob_size, 4));
		break;
	default:
		dev_err(&info->pdev->dev, "%s: invalid state %d\n", __func__,
				info->state);
		BUG();
	}
}
#endif  

#if !defined(MY_ABC_HERE) || (defined(MY_ABC_HERE) && defined(ARCH_HAS_DMA))
static void start_data_dma(struct pxa3xx_nand_info *info)
{
	struct pxa_dma_desc *desc = info->data_desc;
	int dma_len = ALIGN(info->data_size + info->oob_size, 32);

	desc->ddadr = DDADR_STOP;
	desc->dcmd = DCMD_ENDIRQEN | DCMD_WIDTH4 | DCMD_BURST32 | dma_len;

	switch (info->state) {
	case STATE_DMA_WRITING:
		desc->dsadr = info->data_buff_phys;
		desc->dtadr = info->mmio_phys + NDDB;
		desc->dcmd |= DCMD_INCSRCADDR | DCMD_FLOWTRG;
		break;
	case STATE_DMA_READING:
		desc->dtadr = info->data_buff_phys;
		desc->dsadr = info->mmio_phys + NDDB;
		desc->dcmd |= DCMD_INCTRGADDR | DCMD_FLOWSRC;
		break;
	default:
		dev_err(&info->pdev->dev, "%s: invalid state %d\n", __func__,
				info->state);
		BUG();
	}

	DRCMR(info->drcmr_dat) = DRCMR_MAPVLD | info->data_dma_ch;
	DDADR(info->data_dma_ch) = info->data_desc_addr;
	DCSR(info->data_dma_ch) |= DCSR_RUN;
}

static void pxa3xx_nand_data_dma_irq(int channel, void *data)
{
	struct pxa3xx_nand_info *info = data;
	uint32_t dcsr;

	dcsr = DCSR(channel);
	DCSR(channel) = dcsr;

	if (dcsr & DCSR_BUSERR) {
		info->retcode = ERR_DMABUSERR;
	}

	info->state = STATE_DMA_DONE;
	enable_int(info, NDCR_INT_MASK);
	nand_writel(info, NDSR, NDSR_WRDREQ | NDSR_RDDREQ);
}
#else
static void start_data_dma(struct pxa3xx_nand_info *info)
{}
#endif

static irqreturn_t pxa3xx_nand_irq(int irq, void *devid)
{
	struct pxa3xx_nand_info *info = devid;
#if defined(MY_ABC_HERE)
	unsigned int status, is_completed = 0, is_ready = 0;
#else  
	unsigned int status, is_completed = 0;
#endif  
	unsigned int ready, cmd_done;

	if (info->cs == 0) {
		ready           = NDSR_FLASH_RDY;
		cmd_done        = NDSR_CS0_CMDD;
	} else {
		ready           = NDSR_RDY;
		cmd_done        = NDSR_CS1_CMDD;
	}

	status = nand_readl(info, NDSR);

#if defined(MY_ABC_HERE)
	if (status & NDSR_UNCORERR)
		info->retcode = ERR_UNCORERR;
	if (status & NDSR_CORERR) {
		info->retcode = ERR_CORERR;
		if (info->variant == PXA3XX_NAND_VARIANT_ARMADA370 &&
		    info->ecc_bch)
			info->ecc_err_cnt = NDSR_ERR_CNT(status);
		else
			info->ecc_err_cnt = 1;

		info->max_bitflips = max_t(unsigned int,
					   info->max_bitflips,
					   info->ecc_err_cnt);
	}
#else  
	if (status & NDSR_DBERR)
		info->retcode = ERR_DBERR;
	if (status & NDSR_SBERR)
		info->retcode = ERR_SBERR;
#endif  
	if (status & (NDSR_RDDREQ | NDSR_WRDREQ)) {
		 
		if (info->use_dma) {
			disable_int(info, NDCR_INT_MASK);
			info->state = (status & NDSR_RDDREQ) ?
				      STATE_DMA_READING : STATE_DMA_WRITING;
			start_data_dma(info);
			goto NORMAL_IRQ_EXIT;
		} else {
			info->state = (status & NDSR_RDDREQ) ?
				      STATE_PIO_READING : STATE_PIO_WRITING;
			handle_data_pio(info);
		}
	}
	if (status & cmd_done) {
		info->state = STATE_CMD_DONE;
		is_completed = 1;
	}
	if (status & ready) {
#if defined(MY_ABC_HERE)
		info->state = STATE_READY;
		is_ready = 1;
#else  
		info->is_ready = 1;
		info->state = STATE_READY;
#endif  
	}

	if (status & NDSR_WRCMDREQ) {
		nand_writel(info, NDSR, NDSR_WRCMDREQ);
		status &= ~NDSR_WRCMDREQ;
		info->state = STATE_CMD_HANDLE;

		nand_writel(info, NDCB0, info->ndcb0);
		nand_writel(info, NDCB0, info->ndcb1);
		nand_writel(info, NDCB0, info->ndcb2);

#if defined(MY_ABC_HERE)
		 
		if (info->variant == PXA3XX_NAND_VARIANT_ARMADA370)
			nand_writel(info, NDCB0, info->ndcb3);
#endif  
	}

	nand_writel(info, NDSR, status);
	if (is_completed)
		complete(&info->cmd_complete);
#if defined(MY_ABC_HERE)
	if (is_ready)
		complete(&info->dev_ready);
#endif  
NORMAL_IRQ_EXIT:
	return IRQ_HANDLED;
}

static inline int is_buf_blank(uint8_t *buf, size_t len)
{
	for (; len > 0; len--)
		if (*buf++ != 0xff)
			return 0;
	return 1;
}

#if defined(MY_ABC_HERE)
static void set_command_address(struct pxa3xx_nand_info *info,
		unsigned int page_size, uint16_t column, int page_addr)
{
	 
	if (page_size < PAGE_CHUNK_SIZE) {
		info->ndcb1 = ((page_addr & 0xFFFFFF) << 8)
				| (column & 0xFF);

		info->ndcb2 = 0;
	} else {
		info->ndcb1 = ((page_addr & 0xFFFF) << 16)
				| (column & 0xFFFF);

		if (page_addr & 0xFF0000)
			info->ndcb2 = (page_addr & 0xFF0000) >> 16;
		else
			info->ndcb2 = 0;
	}
}

static void prepare_start_command(struct pxa3xx_nand_info *info, int command)
{
	struct pxa3xx_nand_host *host = info->host[info->cs];
	struct mtd_info *mtd = host->mtd;

	info->buf_start		= 0;
	info->buf_count		= 0;
	info->oob_size		= 0;
	info->data_buff_pos	= 0;
	info->oob_buff_pos	= 0;
	info->use_ecc		= 0;
	info->use_spare		= 1;
	info->retcode		= ERR_NONE;
	info->ecc_err_cnt	= 0;
	info->ndcb3		= 0;
	info->need_wait		= 0;

	switch (command) {
	case NAND_CMD_READ0:
	case NAND_CMD_PAGEPROG:
		info->use_ecc = 1;
	case NAND_CMD_READOOB:
		pxa3xx_set_datasize(info, mtd);
		break;
	case NAND_CMD_PARAM:
		info->use_spare = 0;
		break;
	default:
		info->ndcb1 = 0;
		info->ndcb2 = 0;
		break;
	}

	if (command == NAND_CMD_READ0 ||
	    command == NAND_CMD_READOOB ||
	    command == NAND_CMD_SEQIN) {

		info->buf_count = mtd->writesize + mtd->oobsize;
		memset(info->data_buff, 0xFF, info->buf_count);
	}

}
#endif  

#if defined(MY_ABC_HERE)
static int prepare_set_command(struct pxa3xx_nand_info *info, int command,
		int ext_cmd_type, uint16_t column, int page_addr)
{
#else  
static int prepare_command_pool(struct pxa3xx_nand_info *info, int command,
		uint16_t column, int page_addr)
{
	uint16_t cmd;
#endif  
	int addr_cycle, exec_cmd;
	struct pxa3xx_nand_host *host;
	struct mtd_info *mtd;

	host = info->host[info->cs];
	mtd = host->mtd;
	addr_cycle = 0;
	exec_cmd = 1;

#if defined(MY_ABC_HERE)
	 
#else  
	 
	info->buf_start		= 0;
	info->buf_count		= 0;
	info->oob_size		= 0;
	info->use_ecc		= 0;
	info->is_ready		= 0;
	info->retcode		= ERR_NONE;
#endif  

	if (info->cs != 0)
		info->ndcb0 = NDCB0_CSEL;
	else
		info->ndcb0 = 0;

#if defined(MY_ABC_HERE)
	if (command == NAND_CMD_SEQIN)
		exec_cmd = 0;
#else  
	switch (command) {
	case NAND_CMD_READ0:
	case NAND_CMD_PAGEPROG:
		info->use_ecc = 1;
	case NAND_CMD_READOOB:
		pxa3xx_set_datasize(info);
		break;
	case NAND_CMD_SEQIN:
		exec_cmd = 0;
		break;
	default:
		info->ndcb1 = 0;
		info->ndcb2 = 0;
		break;
	}
#endif  

	addr_cycle = NDCB0_ADDR_CYC(host->row_addr_cycles
				    + host->col_addr_cycles);

	switch (command) {
	case NAND_CMD_READOOB:
	case NAND_CMD_READ0:
#if defined(MY_ABC_HERE)
		info->buf_start = column;
		info->ndcb0 |= NDCB0_CMD_TYPE(0)
				| addr_cycle
				| NAND_CMD_READ0;

		if (command == NAND_CMD_READOOB)
			info->buf_start += mtd->writesize;
#else  
		cmd = host->cmdset->read1;
		if (command == NAND_CMD_READOOB)
			info->buf_start = mtd->writesize + column;
		else
			info->buf_start = column;

		if (unlikely(host->page_size < PAGE_CHUNK_SIZE))
			info->ndcb0 |= NDCB0_CMD_TYPE(0)
					| addr_cycle
					| (cmd & NDCB0_CMD1_MASK);
		else
			info->ndcb0 |= NDCB0_CMD_TYPE(0)
					| NDCB0_DBC
					| addr_cycle
					| cmd;
#endif  

#if defined(MY_ABC_HERE)
		 
		if (mtd->writesize == PAGE_CHUNK_SIZE) {
			info->ndcb0 |= NDCB0_DBC | (NAND_CMD_READSTART << 8);
		} else if (mtd->writesize > PAGE_CHUNK_SIZE) {
			info->ndcb0 |= NDCB0_DBC | (NAND_CMD_READSTART << 8)
					| NDCB0_LEN_OVRD
					| NDCB0_EXT_CMD_TYPE(ext_cmd_type);
			info->ndcb3 = info->chunk_size +
				      info->oob_size;
		}

		set_command_address(info, mtd->writesize, column, page_addr);
		break;
#endif  

	case NAND_CMD_SEQIN:
#if defined(MY_ABC_HERE)
		info->buf_start = column;
		set_command_address(info, mtd->writesize, 0, page_addr);

		if (mtd->writesize > PAGE_CHUNK_SIZE) {
			info->ndcb0 |= NDCB0_CMD_TYPE(0x1)
				| NDCB0_EXT_CMD_TYPE(ext_cmd_type)
				| addr_cycle
				| command;
			 
			info->data_size = 0;
			exec_cmd = 1;
		}
#else  
		 
		if (unlikely(host->page_size < PAGE_CHUNK_SIZE)) {
			info->ndcb1 = ((page_addr & 0xFFFFFF) << 8)
					| (column & 0xFF);

			info->ndcb2 = 0;
		} else {
			info->ndcb1 = ((page_addr & 0xFFFF) << 16)
					| (column & 0xFFFF);

			if (page_addr & 0xFF0000)
				info->ndcb2 = (page_addr & 0xFF0000) >> 16;
			else
				info->ndcb2 = 0;
		}

		info->buf_count = mtd->writesize + mtd->oobsize;
		memset(info->data_buff, 0xFF, info->buf_count);
#endif  

		break;

	case NAND_CMD_PAGEPROG:
		if (is_buf_blank(info->data_buff,
					(mtd->writesize + mtd->oobsize))) {
			exec_cmd = 0;
			break;
		}

#if defined(MY_ABC_HERE)
		 
		if (mtd->writesize > PAGE_CHUNK_SIZE) {
			 
			info->ndcb0 |= NDCB0_CMD_TYPE(0x1)
					| NDCB0_LEN_OVRD
					| NDCB0_EXT_CMD_TYPE(ext_cmd_type);
			info->ndcb3 = info->chunk_size +
				      info->oob_size;

			if (info->data_size == 0) {
				info->ndcb0 = NDCB0_CMD_TYPE(0x1)
					| NDCB0_EXT_CMD_TYPE(ext_cmd_type)
					| command;
				info->ndcb1 = 0;
				info->ndcb2 = 0;
				info->ndcb3 = 0;
			}
		} else {
			info->ndcb0 |= NDCB0_CMD_TYPE(0x1)
					| NDCB0_AUTO_RS
					| NDCB0_ST_ROW_EN
					| NDCB0_DBC
					| (NAND_CMD_PAGEPROG << 8)
					| NAND_CMD_SEQIN
					| addr_cycle;
		}
#else  
		cmd = host->cmdset->program;
		info->ndcb0 |= NDCB0_CMD_TYPE(0x1)
				| NDCB0_AUTO_RS
				| NDCB0_ST_ROW_EN
				| NDCB0_DBC
				| cmd
				| addr_cycle;
#endif  
		break;

#if defined(MY_ABC_HERE)
	case NAND_CMD_PARAM:
		info->buf_count = 256;
		info->ndcb0 |= NDCB0_CMD_TYPE(0)
				| NDCB0_ADDR_CYC(1)
				| NDCB0_LEN_OVRD
				| command;
		info->ndcb1 = (column & 0xFF);
		info->ndcb3 = 256;
		info->data_size = 256;
		break;
#endif  

	case NAND_CMD_READID:
#if defined(MY_ABC_HERE)
		info->buf_count = host->read_id_bytes;
		info->ndcb0 |= NDCB0_CMD_TYPE(3)
				| NDCB0_ADDR_CYC(1)
				| command;
		info->ndcb1 = (column & 0xFF);
#else  
		cmd = host->cmdset->read_id;
		info->buf_count = host->read_id_bytes;
		info->ndcb0 |= NDCB0_CMD_TYPE(3)
				| NDCB0_ADDR_CYC(1)
				| cmd;
#endif  

		info->data_size = 8;
		break;
	case NAND_CMD_STATUS:
#if defined(MY_ABC_HERE)
		info->buf_count = 1;
		info->ndcb0 |= NDCB0_CMD_TYPE(4)
				| NDCB0_ADDR_CYC(1)
				| command;
#else  
		cmd = host->cmdset->read_status;
		info->buf_count = 1;
		info->ndcb0 |= NDCB0_CMD_TYPE(4)
				| NDCB0_ADDR_CYC(1)
				| cmd;
#endif  

		info->data_size = 8;
		break;

	case NAND_CMD_ERASE1:
#if defined(MY_ABC_HERE)
		info->ndcb0 |= NDCB0_CMD_TYPE(2)
				| NDCB0_AUTO_RS
				| NDCB0_ADDR_CYC(3)
				| NDCB0_DBC
				| (NAND_CMD_ERASE2 << 8)
				| NAND_CMD_ERASE1;
#else  
		cmd = host->cmdset->erase;
		info->ndcb0 |= NDCB0_CMD_TYPE(2)
				| NDCB0_AUTO_RS
				| NDCB0_ADDR_CYC(3)
				| NDCB0_DBC
				| cmd;
#endif  
		info->ndcb1 = page_addr;
		info->ndcb2 = 0;

		break;
	case NAND_CMD_RESET:
#if defined(MY_ABC_HERE)
		info->ndcb0 |= NDCB0_CMD_TYPE(5)
				| command;
#else  
		cmd = host->cmdset->reset;
		info->ndcb0 |= NDCB0_CMD_TYPE(5)
				| cmd;
#endif  

		break;

	case NAND_CMD_ERASE2:
		exec_cmd = 0;
		break;

	default:
		exec_cmd = 0;
		dev_err(&info->pdev->dev, "non-supported command %x\n",
				command);
		break;
	}

	return exec_cmd;
}

#if defined(MY_ABC_HERE)
static void nand_cmdfunc(struct mtd_info *mtd, unsigned command,
			 int column, int page_addr)
#else  
static void pxa3xx_nand_cmdfunc(struct mtd_info *mtd, unsigned command,
				int column, int page_addr)
#endif  
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;
	int ret, exec_cmd;

#if defined(MY_ABC_HERE)
	if (info->reg_ndcr & NDCR_DWIDTH_M)
#else  
	if (host->reg_ndcr & NDCR_DWIDTH_M)
#endif  
		column /= 2;

	if (info->cs != host->cs) {
		info->cs = host->cs;
#if defined(MY_ABC_HERE)
		nand_writel(info, NDTR0CS0, info->ndtr0cs0);
		nand_writel(info, NDTR1CS0, info->ndtr1cs0);
#else  
		nand_writel(info, NDTR0CS0, host->ndtr0cs0);
		nand_writel(info, NDTR1CS0, host->ndtr1cs0);
#endif  
	}

#if defined(MY_ABC_HERE)
	prepare_start_command(info, command);

	info->state = STATE_PREPARED;
	exec_cmd = prepare_set_command(info, command, 0, column, page_addr);
#else  
	info->state = STATE_PREPARED;
	exec_cmd = prepare_command_pool(info, command, column, page_addr);
#endif  

	if (exec_cmd) {
		init_completion(&info->cmd_complete);
#if defined(MY_ABC_HERE)
		init_completion(&info->dev_ready);
		info->need_wait = 1;
#endif  
		pxa3xx_nand_start(info);

		ret = wait_for_completion_timeout(&info->cmd_complete,
				CHIP_DELAY_TIMEOUT);
		if (!ret) {
			dev_err(&info->pdev->dev, "Wait time out!!!\n");
			 
			pxa3xx_nand_stop(info);
		}
	}
	info->state = STATE_IDLE;
}

#if defined(MY_ABC_HERE)
static void nand_cmdfunc_extended(struct mtd_info *mtd,
				  const unsigned command,
				  int column, int page_addr)
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;
	int ret, exec_cmd, ext_cmd_type;

	if (info->reg_ndcr & NDCR_DWIDTH_M)
		column /= 2;

	if (info->cs != host->cs) {
		info->cs = host->cs;
		nand_writel(info, NDTR0CS0, info->ndtr0cs0);
		nand_writel(info, NDTR1CS0, info->ndtr1cs0);
	}

	switch (command) {
	case NAND_CMD_READ0:
	case NAND_CMD_READOOB:
		ext_cmd_type = EXT_CMD_TYPE_MONO;
		break;
	case NAND_CMD_SEQIN:
		ext_cmd_type = EXT_CMD_TYPE_DISPATCH;
		break;
	case NAND_CMD_PAGEPROG:
		ext_cmd_type = EXT_CMD_TYPE_NAKED_RW;
		break;
	default:
		ext_cmd_type = 0;
		break;
	}

	prepare_start_command(info, command);

	info->need_wait = 1;
	init_completion(&info->dev_ready);
	do {
		info->state = STATE_PREPARED;
		exec_cmd = prepare_set_command(info, command, ext_cmd_type,
					       column, page_addr);
		if (!exec_cmd) {
			info->need_wait = 0;
			complete(&info->dev_ready);
			break;
		}

		init_completion(&info->cmd_complete);
		pxa3xx_nand_start(info);

		ret = wait_for_completion_timeout(&info->cmd_complete,
				CHIP_DELAY_TIMEOUT);
		if (!ret) {
			dev_err(&info->pdev->dev, "Wait time out!!!\n");
			 
			pxa3xx_nand_stop(info);
			break;
		}

		if (info->data_size == 0 && command != NAND_CMD_PAGEPROG)
			break;

		if (info->data_size == 0 &&
		    command == NAND_CMD_PAGEPROG &&
		    ext_cmd_type == EXT_CMD_TYPE_DISPATCH)
			break;

		if (command == NAND_CMD_READ0 || command == NAND_CMD_READOOB) {
			 
			if (info->data_size == info->chunk_size)
				ext_cmd_type = EXT_CMD_TYPE_LAST_RW;
			else
				ext_cmd_type = EXT_CMD_TYPE_NAKED_RW;

		} else if (command == NAND_CMD_PAGEPROG &&
			   info->data_size == 0) {
				ext_cmd_type = EXT_CMD_TYPE_DISPATCH;
		}
	} while (1);

	info->state = STATE_IDLE;
}
#endif  

static int pxa3xx_nand_write_page_hwecc(struct mtd_info *mtd,
		struct nand_chip *chip, const uint8_t *buf, int oob_required)
{
	chip->write_buf(mtd, buf, mtd->writesize);
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);

	return 0;
}

static int pxa3xx_nand_read_page_hwecc(struct mtd_info *mtd,
		struct nand_chip *chip, uint8_t *buf, int oob_required,
		int page)
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;

	chip->read_buf(mtd, buf, mtd->writesize);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

#if defined(MY_ABC_HERE)
	if (info->retcode == ERR_CORERR && info->use_ecc) {
		mtd->ecc_stats.corrected += info->ecc_err_cnt;

	} else if (info->retcode == ERR_UNCORERR) {
		 
		if (is_buf_blank(buf, mtd->writesize))
			info->retcode = ERR_NONE;
		else
			mtd->ecc_stats.failed++;
	}

	return info->max_bitflips;
#else  
	if (info->retcode == ERR_SBERR) {
		switch (info->use_ecc) {
		case 1:
			mtd->ecc_stats.corrected++;
			break;
		case 0:
		default:
			break;
		}
	} else if (info->retcode == ERR_DBERR) {
		 
		if (is_buf_blank(buf, mtd->writesize))
			info->retcode = ERR_NONE;
		else
			mtd->ecc_stats.failed++;
	}

	return 0;
#endif  
}

static uint8_t pxa3xx_nand_read_byte(struct mtd_info *mtd)
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;
	char retval = 0xFF;

	if (info->buf_start < info->buf_count)
		 
		retval = info->data_buff[info->buf_start++];

	return retval;
}

static u16 pxa3xx_nand_read_word(struct mtd_info *mtd)
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;
	u16 retval = 0xFFFF;

	if (!(info->buf_start & 0x01) && info->buf_start < info->buf_count) {
		retval = *((u16 *)(info->data_buff+info->buf_start));
		info->buf_start += 2;
	}
	return retval;
}

static void pxa3xx_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(buf, info->data_buff + info->buf_start, real_len);
	info->buf_start += real_len;
}

static void pxa3xx_nand_write_buf(struct mtd_info *mtd,
		const uint8_t *buf, int len)
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;
	int real_len = min_t(size_t, len, info->buf_count - info->buf_start);

	memcpy(info->data_buff + info->buf_start, buf, real_len);
	info->buf_start += real_len;
}

static void pxa3xx_nand_select_chip(struct mtd_info *mtd, int chip)
{
	return;
}

static int pxa3xx_nand_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;
#if defined(MY_ABC_HERE)
	int ret;

	if (info->need_wait) {
		ret = wait_for_completion_timeout(&info->dev_ready,
				CHIP_DELAY_TIMEOUT);
		info->need_wait = 0;
		if (!ret) {
			dev_err(&info->pdev->dev, "Ready time out!!!\n");
			return NAND_STATUS_FAIL;
		}
	}
#endif  

	if (this->state == FL_WRITING || this->state == FL_ERASING) {
		if (info->retcode == ERR_NONE)
			return 0;
#if defined(MY_ABC_HERE)
		else
			return NAND_STATUS_FAIL;
#else  
		else {
			 
			return 0x01;
		}
#endif  
	}

#if defined(MY_ABC_HERE)
	return NAND_STATUS_READY;
#else  
	return 0;
#endif  
}

static int pxa3xx_nand_config_flash(struct pxa3xx_nand_info *info,
				    const struct pxa3xx_nand_flash *f)
{
	struct platform_device *pdev = info->pdev;
#if defined(MY_ABC_HERE)
	struct pxa3xx_nand_platform_data *pdata = dev_get_platdata(&pdev->dev);
#else  
	struct pxa3xx_nand_platform_data *pdata = pdev->dev.platform_data;
#endif  
	struct pxa3xx_nand_host *host = info->host[info->cs];
	uint32_t ndcr = 0x0;  

	if (f->page_size != 2048 && f->page_size != 512) {
		dev_err(&pdev->dev, "Current only support 2048 and 512 size\n");
		return -EINVAL;
	}

	if (f->flash_width != 16 && f->flash_width != 8) {
		dev_err(&pdev->dev, "Only support 8bit and 16 bit!\n");
		return -EINVAL;
	}

#if defined(MY_ABC_HERE)
	 
#else  
	host->cmdset = &default_cmdset;
	host->page_size = f->page_size;
#endif  
	host->read_id_bytes = (f->page_size == 2048) ? 4 : 2;

	host->col_addr_cycles = (f->page_size == 2048) ? 2 : 1;

	if (f->num_blocks * f->page_per_block > 65536)
		host->row_addr_cycles = 3;
	else
		host->row_addr_cycles = 2;

	ndcr |= (pdata->enable_arbiter) ? NDCR_ND_ARB_EN : 0;
	ndcr |= (host->col_addr_cycles == 2) ? NDCR_RA_START : 0;
	ndcr |= (f->page_per_block == 64) ? NDCR_PG_PER_BLK : 0;
	ndcr |= (f->page_size == 2048) ? NDCR_PAGE_SZ : 0;
	ndcr |= (f->flash_width == 16) ? NDCR_DWIDTH_M : 0;
	ndcr |= (f->dfc_width == 16) ? NDCR_DWIDTH_C : 0;

	ndcr |= NDCR_RD_ID_CNT(host->read_id_bytes);
	ndcr |= NDCR_SPARE_EN;  

#if defined(MY_ABC_HERE)
	info->reg_ndcr = ndcr;
#else  
	host->reg_ndcr = ndcr;
#endif  

	pxa3xx_nand_set_timing(host, f->timing);
	return 0;
}

static int pxa3xx_nand_detect_config(struct pxa3xx_nand_info *info)
{
	 
	struct pxa3xx_nand_host *host = info->host[0];
	uint32_t ndcr = nand_readl(info, NDCR);

#if defined(MY_ABC_HERE)
	if (ndcr & NDCR_PAGE_SZ) {
		 
		info->chunk_size = 2048;
		host->read_id_bytes = 4;
	} else {
		info->chunk_size = 512;
		host->read_id_bytes = 2;
	}
#else  
	if (ndcr & NDCR_PAGE_SZ) {
		host->page_size = 2048;
		host->read_id_bytes = 4;
	} else {
		host->page_size = 512;
		host->read_id_bytes = 2;
	}
#endif  

#if defined(MY_ABC_HERE)
	 
	info->reg_ndcr = ndcr & ~NDCR_INT_MASK;
	info->ndtr0cs0 = nand_readl(info, NDTR0CS0);
	info->ndtr1cs0 = nand_readl(info, NDTR1CS0);
#else  
	host->reg_ndcr = ndcr & ~NDCR_INT_MASK;
	host->cmdset = &default_cmdset;

	host->ndtr0cs0 = nand_readl(info, NDTR0CS0);
	host->ndtr1cs0 = nand_readl(info, NDTR1CS0);
#endif  

	return 0;
}

#if defined(MY_ABC_HERE)
 
#else  
 
#define MAX_BUFF_SIZE	PAGE_SIZE
#endif  

#if !defined(MY_ABC_HERE) || (defined(MY_ABC_HERE) && defined(ARCH_HAS_DMA))
static int pxa3xx_nand_init_buff(struct pxa3xx_nand_info *info)
{
	struct platform_device *pdev = info->pdev;
#if defined(MY_ABC_HERE)
	int data_desc_offset = info->buf_size - sizeof(struct pxa_dma_desc);
#else  
	int data_desc_offset = MAX_BUFF_SIZE - sizeof(struct pxa_dma_desc);
#endif  

	if (use_dma == 0) {
#if defined(MY_ABC_HERE)
		info->data_buff = kmalloc(info->buf_size, GFP_KERNEL);
#else  
		info->data_buff = kmalloc(MAX_BUFF_SIZE, GFP_KERNEL);
#endif  
		if (info->data_buff == NULL)
			return -ENOMEM;
		return 0;
	}

#if defined(MY_ABC_HERE)
	info->data_buff = dma_alloc_coherent(&pdev->dev, info->buf_size,
#else  
	info->data_buff = dma_alloc_coherent(&pdev->dev, MAX_BUFF_SIZE,
#endif  
				&info->data_buff_phys, GFP_KERNEL);
	if (info->data_buff == NULL) {
		dev_err(&pdev->dev, "failed to allocate dma buffer\n");
		return -ENOMEM;
	}

	info->data_desc = (void *)info->data_buff + data_desc_offset;
	info->data_desc_addr = info->data_buff_phys + data_desc_offset;

	info->data_dma_ch = pxa_request_dma("nand-data", DMA_PRIO_LOW,
				pxa3xx_nand_data_dma_irq, info);
	if (info->data_dma_ch < 0) {
		dev_err(&pdev->dev, "failed to request data dma\n");
#if defined(MY_ABC_HERE)
		dma_free_coherent(&pdev->dev, info->buf_size,
#else  
		dma_free_coherent(&pdev->dev, MAX_BUFF_SIZE,
#endif  
				info->data_buff, info->data_buff_phys);
		return info->data_dma_ch;
	}

#if defined(MY_ABC_HERE)
	 
	info->use_dma = 1;
#endif  
	return 0;
}

#if defined(MY_ABC_HERE)
static void pxa3xx_nand_free_buff(struct pxa3xx_nand_info *info)
{
	struct platform_device *pdev = info->pdev;
	if (info->use_dma) {
		pxa_free_dma(info->data_dma_ch);
		dma_free_coherent(&pdev->dev, info->buf_size,
				  info->data_buff, info->data_buff_phys);
	} else {
		kfree(info->data_buff);
	}
}
#endif  
#else
static int pxa3xx_nand_init_buff(struct pxa3xx_nand_info *info)
{
	info->data_buff = kmalloc(info->buf_size, GFP_KERNEL);
	if (info->data_buff == NULL)
		return -ENOMEM;
	return 0;
}

static void pxa3xx_nand_free_buff(struct pxa3xx_nand_info *info)
{
	kfree(info->data_buff);
}
#endif

static int pxa3xx_nand_sensing(struct pxa3xx_nand_info *info)
{
	struct mtd_info *mtd;
#if defined(MY_ABC_HERE)
	struct nand_chip *chip;
#endif  
	int ret;

	mtd = info->host[info->cs]->mtd;
#if defined(MY_ABC_HERE)
	chip = mtd->priv;
#endif  

	ret = pxa3xx_nand_config_flash(info, &builtin_flash_types[0]);
	if (ret)
		return ret;

#if defined(MY_ABC_HERE)
	chip->cmdfunc(mtd, NAND_CMD_RESET, 0, 0);
	ret = chip->waitfunc(mtd, chip);
	if (ret & NAND_STATUS_FAIL)
		return -ENODEV;

	return 0;
#else  
	pxa3xx_nand_cmdfunc(mtd, NAND_CMD_RESET, 0, 0);
	if (info->is_ready)
		return 0;

	return -ENODEV;
#endif  
}

#if defined(MY_ABC_HERE)
static int pxa_ecc_init(struct pxa3xx_nand_info *info,
			struct nand_ecc_ctrl *ecc,
			int strength, int ecc_stepsize, int page_size)
{
	if (strength == 1 && ecc_stepsize == 512 && page_size == 2048) {
		info->chunk_size = 2048;
		info->spare_size = 40;
		info->ecc_size = 24;
		ecc->mode = NAND_ECC_HW;
		ecc->size = 512;
		ecc->strength = 1;
		return 1;

	} else if (strength == 1 && ecc_stepsize == 512 && page_size == 512) {
		info->chunk_size = 512;
		info->spare_size = 8;
		info->ecc_size = 8;
		ecc->mode = NAND_ECC_HW;
		ecc->size = 512;
		ecc->strength = 1;
		return 1;

	} else if (strength == 4 && ecc_stepsize == 512 && page_size == 2048) {
		info->ecc_bch = 1;
		info->chunk_size = 2048;
		info->spare_size = 32;
		info->ecc_size = 32;
		ecc->mode = NAND_ECC_HW;
		ecc->size = info->chunk_size;
		ecc->layout = &ecc_layout_2KB_bch4bit;
		ecc->strength = 16;
		return 1;

	} else if (strength == 4 && ecc_stepsize == 512 && page_size == 4096) {
		info->ecc_bch = 1;
		info->chunk_size = 2048;
		info->spare_size = 32;
		info->ecc_size = 32;
		ecc->mode = NAND_ECC_HW;
		ecc->size = info->chunk_size;
		ecc->layout = &ecc_layout_4KB_bch4bit;
		ecc->strength = 16;
		return 1;

	} else if (strength == 8 && ecc_stepsize == 512 && page_size == 4096) {
		info->ecc_bch = 1;
		info->chunk_size = 1024;
		info->spare_size = 0;
		info->ecc_size = 32;
		ecc->mode = NAND_ECC_HW;
		ecc->size = info->chunk_size;
		ecc->layout = &ecc_layout_4KB_bch8bit;
		ecc->strength = 16;
		return 1;
	}
	return 0;
}
#endif  

static int pxa3xx_nand_scan(struct mtd_info *mtd)
{
	struct pxa3xx_nand_host *host = mtd->priv;
	struct pxa3xx_nand_info *info = host->info_data;
	struct platform_device *pdev = info->pdev;
#if defined(MY_ABC_HERE)
	struct pxa3xx_nand_platform_data *pdata = dev_get_platdata(&pdev->dev);
#else  
	struct pxa3xx_nand_platform_data *pdata = pdev->dev.platform_data;
#endif  
	struct nand_flash_dev pxa3xx_flash_ids[2], *def = NULL;
	const struct pxa3xx_nand_flash *f = NULL;
	struct nand_chip *chip = mtd->priv;
	uint32_t id = -1;
	uint64_t chipsize;
	int i, ret, num;
#if defined(MY_ABC_HERE)
	uint16_t ecc_strength, ecc_step;
#endif  

	if (pdata->keep_config && !pxa3xx_nand_detect_config(info))
		goto KEEP_CONFIG;

	ret = pxa3xx_nand_sensing(info);
	if (ret) {
		dev_info(&info->pdev->dev, "There is no chip on cs %d!\n",
			 info->cs);

		return ret;
	}

	chip->cmdfunc(mtd, NAND_CMD_READID, 0, 0);
	id = *((uint16_t *)(info->data_buff));
	if (id != 0)
		dev_info(&info->pdev->dev, "Detect a flash id %x\n", id);
	else {
		dev_warn(&info->pdev->dev,
			 "Read out ID 0, potential timing set wrong!!\n");

		return -EINVAL;
	}

	num = ARRAY_SIZE(builtin_flash_types) + pdata->num_flash - 1;
	for (i = 0; i < num; i++) {
		if (i < pdata->num_flash)
			f = pdata->flash + i;
		else
			f = &builtin_flash_types[i - pdata->num_flash + 1];

		if (f->chip_id == id)
			break;
	}

	if (i >= (ARRAY_SIZE(builtin_flash_types) + pdata->num_flash - 1)) {
		dev_err(&info->pdev->dev, "ERROR!! flash not defined!!!\n");

		return -EINVAL;
	}

	ret = pxa3xx_nand_config_flash(info, f);
	if (ret) {
		dev_err(&info->pdev->dev, "ERROR! Configure failed\n");
		return ret;
	}

	pxa3xx_flash_ids[0].name = f->name;
	pxa3xx_flash_ids[0].dev_id = (f->chip_id >> 8) & 0xffff;
	pxa3xx_flash_ids[0].pagesize = f->page_size;
	chipsize = (uint64_t)f->num_blocks * f->page_per_block * f->page_size;
	pxa3xx_flash_ids[0].chipsize = chipsize >> 20;
	pxa3xx_flash_ids[0].erasesize = f->page_size * f->page_per_block;
	if (f->flash_width == 16)
		pxa3xx_flash_ids[0].options = NAND_BUSWIDTH_16;
	pxa3xx_flash_ids[1].name = NULL;
	def = pxa3xx_flash_ids;
KEEP_CONFIG:
#if defined(MY_ABC_HERE)
	if (info->reg_ndcr & NDCR_DWIDTH_M)
		chip->options |= NAND_BUSWIDTH_16;

	if (info->variant == PXA3XX_NAND_VARIANT_ARMADA370)
		nand_writel(info, NDECCCTRL, 0x0);
#else  
	chip->ecc.mode = NAND_ECC_HW;
	chip->ecc.size = host->page_size;
	chip->ecc.strength = 1;

	if (host->reg_ndcr & NDCR_DWIDTH_M)
		chip->options |= NAND_BUSWIDTH_16;
#endif  

	if (nand_scan_ident(mtd, 1, def))
		return -ENODEV;

#if defined(MY_ABC_HERE)
	if (pdata->flash_bbt) {
		 
		chip->bbt_options |= NAND_BBT_USE_FLASH |
				     NAND_BBT_NO_OOB_BBM;
		chip->bbt_td = &bbt_main_descr;
		chip->bbt_md = &bbt_mirror_descr;
	}

	if (mtd->writesize > PAGE_CHUNK_SIZE) {
		if (info->variant == PXA3XX_NAND_VARIANT_ARMADA370) {
			chip->cmdfunc = nand_cmdfunc_extended;
		} else {
			dev_err(&info->pdev->dev,
				"unsupported page size on this variant\n");
			return -ENODEV;
		}
	}

	ecc_strength = chip->ecc_strength_ds;
	ecc_step = chip->ecc_step_ds;

	if (ecc_strength < 1 && ecc_step < 1) {
		ecc_strength = 1;
		ecc_step = 512;
	}

	ret = pxa_ecc_init(info, &chip->ecc, ecc_strength,
			   ecc_step, mtd->writesize);
	if (!ret) {
		dev_err(&info->pdev->dev,
			"ECC strength %d at page size %d is not supported\n",
			chip->ecc_strength_ds, mtd->writesize);
		return -ENODEV;
	}
#endif  

	if (mtd->writesize >= 2048)
		host->col_addr_cycles = 2;
	else
		host->col_addr_cycles = 1;

#if defined(MY_ABC_HERE)
	 
	kfree(info->data_buff);

	info->buf_size = mtd->writesize + mtd->oobsize;
	ret = pxa3xx_nand_init_buff(info);
	if (ret)
		return ret;
#endif  
	info->oob_buff = info->data_buff + mtd->writesize;
	if ((mtd->size >> chip->page_shift) > 65536)
		host->row_addr_cycles = 3;
	else
		host->row_addr_cycles = 2;

#if defined(MY_ABC_HERE)
	 
#else  
	mtd->name = mtd_names[0];
#endif  
	return nand_scan_tail(mtd);
}

static int alloc_nand_resource(struct platform_device *pdev)
{
	struct pxa3xx_nand_platform_data *pdata;
	struct pxa3xx_nand_info *info;
	struct pxa3xx_nand_host *host;
	struct nand_chip *chip = NULL;
	struct mtd_info *mtd;
	struct resource *r;
	int ret, irq, cs;

#if defined(MY_ABC_HERE)
	pdata = dev_get_platdata(&pdev->dev);
	info = devm_kzalloc(&pdev->dev, sizeof(*info) + (sizeof(*mtd) +
			    sizeof(*host)) * pdata->num_cs, GFP_KERNEL);
	if (!info)
		return -ENOMEM;
#else  
	pdata = pdev->dev.platform_data;
	info = kzalloc(sizeof(*info) + (sizeof(*mtd) +
		       sizeof(*host)) * pdata->num_cs, GFP_KERNEL);
	if (!info) {
		dev_err(&pdev->dev, "failed to allocate memory\n");
		return -ENOMEM;
	}
#endif  

	info->pdev = pdev;
#if defined(MY_ABC_HERE)
	info->variant = pxa3xx_nand_get_variant(pdev);
#endif  
	for (cs = 0; cs < pdata->num_cs; cs++) {
		mtd = (struct mtd_info *)((unsigned int)&info[1] +
		      (sizeof(*mtd) + sizeof(*host)) * cs);
		chip = (struct nand_chip *)(&mtd[1]);
		host = (struct pxa3xx_nand_host *)chip;
		info->host[cs] = host;
		host->mtd = mtd;
		host->cs = cs;
		host->info_data = info;
		mtd->priv = host;
		mtd->owner = THIS_MODULE;

		chip->ecc.read_page	= pxa3xx_nand_read_page_hwecc;
		chip->ecc.write_page	= pxa3xx_nand_write_page_hwecc;
		chip->controller        = &info->controller;
		chip->waitfunc		= pxa3xx_nand_waitfunc;
		chip->select_chip	= pxa3xx_nand_select_chip;
#if defined(MY_ABC_HERE)
		 
#else  
		chip->cmdfunc		= pxa3xx_nand_cmdfunc;
#endif  
		chip->read_word		= pxa3xx_nand_read_word;
		chip->read_byte		= pxa3xx_nand_read_byte;
		chip->read_buf		= pxa3xx_nand_read_buf;
		chip->write_buf		= pxa3xx_nand_write_buf;
#if defined(MY_ABC_HERE)
		chip->options		|= NAND_NO_SUBPAGE_WRITE;
		chip->cmdfunc		= nand_cmdfunc;
#endif  
	}

	spin_lock_init(&chip->controller->lock);
	init_waitqueue_head(&chip->controller->wq);
#if defined(MY_ABC_HERE)
	info->clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(info->clk)) {
		dev_err(&pdev->dev, "failed to get nand clock\n");
		return PTR_ERR(info->clk);
	}
	ret = clk_prepare_enable(info->clk);
	if (ret < 0)
		return ret;

	if (use_dma) {
		 
		if (pdev->dev.of_node &&
		    of_machine_is_compatible("marvell,pxa3xx")) {
			info->drcmr_dat = 97;
			info->drcmr_cmd = 99;
		} else {
			r = platform_get_resource(pdev, IORESOURCE_DMA, 0);
			if (r == NULL) {
				dev_err(&pdev->dev,
					"no resource defined for data DMA\n");
				ret = -ENXIO;
				goto fail_disable_clk;
			}
			info->drcmr_dat = r->start;

			r = platform_get_resource(pdev, IORESOURCE_DMA, 1);
			if (r == NULL) {
				dev_err(&pdev->dev,
					"no resource defined for cmd DMA\n");
				ret = -ENXIO;
				goto fail_disable_clk;
			}
			info->drcmr_cmd = r->start;
		}
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no IRQ resource defined\n");
		ret = -ENXIO;
		goto fail_disable_clk;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	info->mmio_base = devm_ioremap_resource(&pdev->dev, r);
	if (IS_ERR(info->mmio_base)) {
		ret = PTR_ERR(info->mmio_base);
		goto fail_disable_clk;
	}
	info->mmio_phys = r->start;

	info->buf_size = INIT_BUFFER_SIZE;
	info->data_buff = kmalloc(info->buf_size, GFP_KERNEL);
	if (info->data_buff == NULL) {
		ret = -ENOMEM;
		goto fail_disable_clk;
	}

	disable_int(info, NDSR_MASK);

	ret = request_irq(irq, pxa3xx_nand_irq, 0, pdev->name, info);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request IRQ\n");
		goto fail_free_buf;
	}

	platform_set_drvdata(pdev, info);

	return 0;

fail_free_buf:
	free_irq(irq, info);
	kfree(info->data_buff);
fail_disable_clk:
	clk_disable_unprepare(info->clk);
	return ret;
#else  
	info->clk = clk_get(&pdev->dev, NULL);
	if (IS_ERR(info->clk)) {
		dev_err(&pdev->dev, "failed to get nand clock\n");
		ret = PTR_ERR(info->clk);
		goto fail_free_mtd;
	}
	clk_enable(info->clk);

	if (pdev->dev.of_node && cpu_is_pxa3xx()) {
		info->drcmr_dat = 97;
		info->drcmr_cmd = 99;
	} else {
		r = platform_get_resource(pdev, IORESOURCE_DMA, 0);
		if (r == NULL) {
			dev_err(&pdev->dev, "no resource defined for data DMA\n");
			ret = -ENXIO;
			goto fail_put_clk;
		}
		info->drcmr_dat = r->start;

		r = platform_get_resource(pdev, IORESOURCE_DMA, 1);
		if (r == NULL) {
			dev_err(&pdev->dev, "no resource defined for command DMA\n");
			ret = -ENXIO;
			goto fail_put_clk;
		}
		info->drcmr_cmd = r->start;
	}

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no IRQ resource defined\n");
		ret = -ENXIO;
		goto fail_put_clk;
	}

	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (r == NULL) {
		dev_err(&pdev->dev, "no IO memory resource defined\n");
		ret = -ENODEV;
		goto fail_put_clk;
	}

	r = request_mem_region(r->start, resource_size(r), pdev->name);
	if (r == NULL) {
		dev_err(&pdev->dev, "failed to request memory resource\n");
		ret = -EBUSY;
		goto fail_put_clk;
	}

	info->mmio_base = ioremap(r->start, resource_size(r));
	if (info->mmio_base == NULL) {
		dev_err(&pdev->dev, "ioremap() failed\n");
		ret = -ENODEV;
		goto fail_free_res;
	}
	info->mmio_phys = r->start;

	ret = pxa3xx_nand_init_buff(info);
	if (ret)
		goto fail_free_io;

	disable_int(info, NDSR_MASK);

	ret = request_irq(irq, pxa3xx_nand_irq, IRQF_DISABLED,
			  pdev->name, info);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to request IRQ\n");
		goto fail_free_buf;
	}

	platform_set_drvdata(pdev, info);

	return 0;

fail_free_buf:
	free_irq(irq, info);
	if (use_dma) {
		pxa_free_dma(info->data_dma_ch);
		dma_free_coherent(&pdev->dev, MAX_BUFF_SIZE,
			info->data_buff, info->data_buff_phys);
	} else
		kfree(info->data_buff);
fail_free_io:
	iounmap(info->mmio_base);
fail_free_res:
	release_mem_region(r->start, resource_size(r));
fail_put_clk:
	clk_disable(info->clk);
	clk_put(info->clk);
fail_free_mtd:
	kfree(info);
	return ret;
#endif  
}

static int pxa3xx_nand_remove(struct platform_device *pdev)
{
	struct pxa3xx_nand_info *info = platform_get_drvdata(pdev);
	struct pxa3xx_nand_platform_data *pdata;
#if defined(MY_ABC_HERE)
	 
#else  
	struct resource *r;
#endif  
	int irq, cs;

	if (!info)
		return 0;

#if defined(MY_ABC_HERE)
	pdata = dev_get_platdata(&pdev->dev);

	irq = platform_get_irq(pdev, 0);
	if (irq >= 0)
		free_irq(irq, info);
	pxa3xx_nand_free_buff(info);

	clk_disable_unprepare(info->clk);

	for (cs = 0; cs < pdata->num_cs; cs++)
		nand_release(info->host[cs]->mtd);
#else  
	pdata = pdev->dev.platform_data;
	platform_set_drvdata(pdev, NULL);

	irq = platform_get_irq(pdev, 0);
	if (irq >= 0)
		free_irq(irq, info);
	if (use_dma) {
		pxa_free_dma(info->data_dma_ch);
		dma_free_writecombine(&pdev->dev, MAX_BUFF_SIZE,
				info->data_buff, info->data_buff_phys);
	} else
		kfree(info->data_buff);

	iounmap(info->mmio_base);
	r = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(r->start, resource_size(r));

	clk_disable(info->clk);
	clk_put(info->clk);

	for (cs = 0; cs < pdata->num_cs; cs++)
		nand_release(info->host[cs]->mtd);
	kfree(info);
#endif  
	return 0;
}

#if defined(MY_ABC_HERE)
 
#else  
#ifdef CONFIG_OF
static struct of_device_id pxa3xx_nand_dt_ids[] = {
	{ .compatible = "marvell,pxa3xx-nand" },
	{}
};
MODULE_DEVICE_TABLE(of, i2c_pxa_dt_ids);
#endif
#endif  

#if defined(CONFIG_OF) || defined(MY_ABC_HERE)
static int pxa3xx_nand_probe_dt(struct platform_device *pdev)
{
	struct pxa3xx_nand_platform_data *pdata;
	struct device_node *np = pdev->dev.of_node;
	const struct of_device_id *of_id =
			of_match_device(pxa3xx_nand_dt_ids, &pdev->dev);

	if (!of_id)
		return 0;

	pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	if (of_get_property(np, "marvell,nand-enable-arbiter", NULL))
		pdata->enable_arbiter = 1;
	if (of_get_property(np, "marvell,nand-keep-config", NULL))
		pdata->keep_config = 1;
	of_property_read_u32(np, "num-cs", &pdata->num_cs);
#if defined(MY_ABC_HERE)
	pdata->flash_bbt = of_get_nand_on_flash_bbt(np);
#endif  

	pdev->dev.platform_data = pdata;

	return 0;
}
#else
#if defined(MY_ABC_HERE)
 
#else  
static inline int pxa3xx_nand_probe_dt(struct platform_device *pdev)
{
	return 0;
}
#endif  
#endif

static int pxa3xx_nand_probe(struct platform_device *pdev)
{
	struct pxa3xx_nand_platform_data *pdata;
	struct mtd_part_parser_data ppdata = {};
	struct pxa3xx_nand_info *info;
	int ret, cs, probe_success;

#if defined(MY_ABC_HERE)
#ifndef ARCH_HAS_DMA
	if (use_dma) {
		use_dma = 0;
		dev_warn(&pdev->dev,
			 "This platform can't do DMA on this device\n");
	}
#endif
#endif  
	ret = pxa3xx_nand_probe_dt(pdev);
	if (ret)
		return ret;

#if defined(MY_ABC_HERE)
	pdata = dev_get_platdata(&pdev->dev);
#else  
	pdata = pdev->dev.platform_data;
#endif  
	if (!pdata) {
		dev_err(&pdev->dev, "no platform data defined\n");
		return -ENODEV;
	}

	ret = alloc_nand_resource(pdev);
	if (ret) {
		dev_err(&pdev->dev, "alloc nand resource failed\n");
		return ret;
	}

	info = platform_get_drvdata(pdev);
	probe_success = 0;
	for (cs = 0; cs < pdata->num_cs; cs++) {
#if defined(MY_ABC_HERE)
		struct mtd_info *mtd = info->host[cs]->mtd;

		mtd->name = "pxa3xx_nand-0";
		info->cs = cs;
		ret = pxa3xx_nand_scan(mtd);
#else  
		info->cs = cs;
		ret = pxa3xx_nand_scan(info->host[cs]->mtd);
#endif  
		if (ret) {
			dev_warn(&pdev->dev, "failed to scan nand at cs %d\n",
				cs);
			continue;
		}

		ppdata.of_node = pdev->dev.of_node;
#if defined(MY_ABC_HERE)
		ret = mtd_device_parse_register(mtd, NULL,
#else  
		ret = mtd_device_parse_register(info->host[cs]->mtd, NULL,
#endif  
						&ppdata, pdata->parts[cs],
						pdata->nr_parts[cs]);
		if (!ret)
			probe_success = 1;
	}

	if (!probe_success) {
		pxa3xx_nand_remove(pdev);
		return -ENODEV;
	}

	return 0;
}

#ifdef CONFIG_PM
static int pxa3xx_nand_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct pxa3xx_nand_info *info = platform_get_drvdata(pdev);
	struct pxa3xx_nand_platform_data *pdata;
	struct mtd_info *mtd;
	int cs;

#if defined(MY_ABC_HERE)
	pdata = dev_get_platdata(&pdev->dev);
#else  
	pdata = pdev->dev.platform_data;
#endif  
	if (info->state) {
		dev_err(&pdev->dev, "driver busy, state = %d\n", info->state);
		return -EAGAIN;
	}

	for (cs = 0; cs < pdata->num_cs; cs++) {
		mtd = info->host[cs]->mtd;
		mtd_suspend(mtd);
	}

	return 0;
}

static int pxa3xx_nand_resume(struct platform_device *pdev)
{
	struct pxa3xx_nand_info *info = platform_get_drvdata(pdev);
	struct pxa3xx_nand_platform_data *pdata;
	struct mtd_info *mtd;
	int cs;

#if defined(MY_ABC_HERE)
	pdata = dev_get_platdata(&pdev->dev);
#else  
	pdata = pdev->dev.platform_data;
#endif  
	 
	disable_int(info, NDCR_INT_MASK);

	info->cs = 0xff;

	nand_writel(info, NDSR, NDSR_MASK);
	for (cs = 0; cs < pdata->num_cs; cs++) {
		mtd = info->host[cs]->mtd;
		mtd_resume(mtd);
	}

	return 0;
}
#else
#define pxa3xx_nand_suspend	NULL
#define pxa3xx_nand_resume	NULL
#endif

static struct platform_driver pxa3xx_nand_driver = {
	.driver = {
		.name	= "pxa3xx-nand",
#if defined(MY_ABC_HERE)
		.of_match_table = pxa3xx_nand_dt_ids,
#else  
		.of_match_table = of_match_ptr(pxa3xx_nand_dt_ids),
#endif  
	},
	.probe		= pxa3xx_nand_probe,
	.remove		= pxa3xx_nand_remove,
	.suspend	= pxa3xx_nand_suspend,
	.resume		= pxa3xx_nand_resume,
};

module_platform_driver(pxa3xx_nand_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("PXA3xx NAND controller driver");

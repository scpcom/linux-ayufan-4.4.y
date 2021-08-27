#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <asm/io.h>
#include <asm/sizes.h>
#include <mach/hardware.h>
#include <plat/orion_nand.h>

#if defined(CONFIG_SYNO_ARMADA) || defined(CONFIG_SYNO_ARMADA_V2)
#ifdef CONFIG_MTD_NAND_RS_ECC

static struct nand_ecclayout mv_nand_rs_oobinfo = {
	.eccbytes = 40,
	.eccpos = {
		24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63
	},

	.oobfree = {{6, 18}}
};

#define	MV_NAND_ECC_4BIT	4
#define MV_NAND_ECC_1BIT	1

static unsigned int ecc_mode = MV_NAND_ECC_1BIT;

static int ecc_mode_setup(char *str)
{
	int mode, ret;

	ret = get_option(&str, &mode);
	if (!ret)
		printk("%s: wrong nandEcc string\n", __func__);

	if (mode == MV_NAND_ECC_1BIT || mode == MV_NAND_ECC_4BIT)
		ecc_mode = mode;
	else
		ecc_mode = MV_NAND_ECC_1BIT;  

	return 1;
}

__setup("nandEcc=", ecc_mode_setup);

#define mm 10	   
#define	nn 1023    
#define tt 4       
#define kk 1015    

static char rs_initialized = 0;

typedef u_short tgf;   

short pp[mm+1] = { 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1 };

tgf alpha_to[nn + 1];

tgf index_of[nn + 1];

tgf Gg[nn - kk + 1];

#define	minimum(a,b)	((a) < (b) ? (a) : (b))

#define	BLANK(a,n) {\
	short ci;\
	for(ci=0; ci<(n); ci++)\
		(a)[ci] = 0;\
	}

#define	COPY(a,b,n) {\
	short ci;\
	for(ci=(n)-1;ci >=0;ci--)\
		(a)[ci] = (b)[ci];\
	}
#define	COPYDOWN(a,b,n) {\
	short ci;\
	for(ci=(n)-1;ci >=0;ci--)\
		(a)[ci] = (b)[ci];\
	}

void generate_gf(void)
{
	register int i, mask;

	mask = 1;
	alpha_to[mm] = 0;
	for (i = 0; i < mm; i++) {
		alpha_to[i] = mask;
		index_of[alpha_to[i]] = i;
		if (pp[i] != 0)
			alpha_to[mm] ^= mask;
		mask <<= 1;
	}
	index_of[alpha_to[mm]] = mm;

	mask >>= 1;
	for (i = mm + 1; i < nn; i++) {
		if (alpha_to[i - 1] >= mask)
			alpha_to[i] = alpha_to[mm] ^ ((alpha_to[i - 1] ^ mask) << 1);
		else
			alpha_to[i] = alpha_to[i - 1] << 1;
		index_of[alpha_to[i]] = i;
	}
	index_of[0] = nn;
	alpha_to[nn] = 0;
}

void gen_poly(void)
{
	register int i, j;

	Gg[0] = alpha_to[1];  
	Gg[1] = 1;		      
	for (i = 2; i <= nn - kk; i++) {
		Gg[i] = 1;
		 
		for (j = i - 1; j > 0; j--)
			if (Gg[j] != 0)
				Gg[j] = Gg[j - 1] ^ alpha_to[((index_of[Gg[j]]) + i)%nn];
			else
				Gg[j] = Gg[j - 1];
		Gg[0] = alpha_to[((index_of[Gg[0]]) + i) % nn];
	}
	 
	for (i = 0; i <= nn - kk; i++)
		Gg[i] = index_of[Gg[i]];
}

static inline char encode_rs(u_short data[kk], u_short bb[nn-kk])
{
	register int i, j;
	tgf feedback;

	BLANK(bb,nn-kk);
	for (i = kk - 1; i >= 0; i--) {
		if(data[i] > nn)
			return -1;	 
		feedback = index_of[data[i] ^ bb[nn - kk - 1]];
		if (feedback != nn) {	 
			for (j = nn - kk - 1; j > 0; j--)
				if (Gg[j] != nn)
					bb[j] = bb[j - 1] ^ alpha_to[(Gg[j] + feedback)%nn];
				else
					bb[j] = bb[j - 1];
			bb[0] = alpha_to[(Gg[0] + feedback)%nn];
		} else {
			for (j = nn - kk - 1; j > 0; j--)
				bb[j] = bb[j - 1];
			bb[0] = 0;
		}
	}
	return 0;
}

static inline int decode_rs(u_short data[nn])
{
	int deg_lambda, el, deg_omega;
	int i, j, r;
	tgf q,tmp,num1,num2,den,discr_r;
	tgf recd[nn];
	tgf lambda[nn-kk + 1], s[nn-kk + 1];	 
	tgf b[nn-kk + 1], t[nn-kk + 1], omega[nn-kk + 1];
	tgf root[nn-kk], reg[nn-kk + 1], loc[nn-kk];
	int syn_error, count;

	for (i = nn-1; i >= 0; i--){

		if(data[i] > nn)
			return -1;	 

		recd[i] = index_of[data[i]];
	}

	syn_error = 0;

	for (i = 1; i <= nn-kk; i++) {
		tmp = 0;

		for (j = 0; j < nn; j++)
			if (recd[j] != nn)	 
				tmp ^= alpha_to[(recd[j] + (1+i-1)*j)%nn];

		syn_error |= tmp;	 
		 
		s[i] = index_of[tmp];
	}

	if (!syn_error) {
		 
		return 0;
	}

	BLANK(&lambda[1],nn-kk);

	lambda[0] = 1;

	for(i=0;i<nn-kk+1;i++)
		b[i] = index_of[lambda[i]];

	r = 0;
	el = 0;
	while (++r <= nn-kk) {	 
		 
		discr_r = 0;

		for (i = 0; i < r; i++) {
			if ((lambda[i] != 0) && (s[r - i] != nn)) {
				discr_r ^= alpha_to[(index_of[lambda[i]] + s[r - i])%nn];
			}
		}

		discr_r = index_of[discr_r];	 
		if (discr_r == nn) {
			 
			COPYDOWN(&b[1],b,nn-kk);
			b[0] = nn;
		} else {
			 
			t[0] = lambda[0];
			for (i = 0 ; i < nn-kk; i++) {
				if(b[i] != nn)
					 
					t[i+1] = lambda[i+1] ^ alpha_to[(discr_r + b[i])%nn];
				else
					t[i+1] = lambda[i+1];
			}
			if (2 * el <= r - 1) {
				el = r - el;
				 
				for (i = 0; i <= nn-kk; i++)
					 
					b[i] = (lambda[i] == 0) ? nn : ((index_of[lambda[i]] - discr_r + nn)%nn);
			} else {
				 
				COPYDOWN(&b[1],b,nn-kk);
				b[0] = nn;
			}
			COPY(lambda,t,nn-kk+1);
		}
	}

	deg_lambda = 0;
	for (i = 0; i < nn - kk + 1; i++) {
		lambda[i] = index_of[lambda[i]];
		if (lambda[i] != nn)
			deg_lambda = i;
	}

	COPY(&reg[1],&lambda[1],nn-kk);
	count = 0;		 
	for (i = 1; i <= nn; i++) {
		q = 1;
		for (j = deg_lambda; j > 0; j--)
			if (reg[j] != nn) {
				 
				reg[j] = (reg[j] + j)%nn;
				q ^= alpha_to[reg[j]];
			}
		if (!q) {
			 
			root[count] = i;
			loc[count] = nn - i;
			count++;
		}
	}

	if (deg_lambda != count) {
		 
		return -1;
	}

	deg_omega = 0;
	for (i = 0; i < nn - kk; i++) {
		tmp = 0;
		j = (deg_lambda < i) ? deg_lambda : i;
		for (; j >= 0; j--) {
			if ((s[i + 1 - j] != nn) && (lambda[j] != nn))
				 
				tmp ^= alpha_to[(s[i + 1 - j] + lambda[j])%nn];
		}
		if (tmp != 0)
			deg_omega = i;
		omega[i] = index_of[tmp];
	}
	omega[nn-kk] = nn;

	for (j = count-1; j >=0; j--) {
		num1 = 0;
		for (i = deg_omega; i >= 0; i--) {
			if (omega[i] != nn)
				 
				num1  ^= alpha_to[(omega[i] + i * root[j])%nn];
		}
		 
		num2 = alpha_to[(root[j] * (1 - 1) + nn)%nn];
		den = 0;

		for (i = minimum(deg_lambda,nn-kk-1) & ~1; i >= 0; i -=2) {
			if(lambda[i+1] != nn)
				 
				den ^= alpha_to[(lambda[i+1] + i * root[j])%nn];
		}
		if (den == 0) {
#ifdef DEBUG
			printk("\n ERROR: denominator = 0\n");
#endif
			return -1;
		}
		 
		if (num1 != 0) {
			 
			data[loc[j]] ^= alpha_to[(index_of[num1] + index_of[num2] + nn - index_of[den])%nn];
		}
	}
	return count;
}

int mv_nand_calculate_ecc_rs(struct mtd_info *mtd, const u_char *data, u_char *ecc_code)
{
	int i;
	u_short rsdata[nn];

	if (!rs_initialized) {
		generate_gf();
		gen_poly();
		rs_initialized = 1;
	}

	for (i = 512; i < nn; i++)
		rsdata[i] = 0;

	for (i = 0; i < 512; i++)
		rsdata[i] = (u_short) data[i];

	if ((encode_rs(rsdata,&(rsdata[kk]))) != 0)
		return -1;

	*(ecc_code) 	= (unsigned char) rsdata[kk];
	*(ecc_code+1) 	= ((rsdata[0x3F7])   >> 8) | ((rsdata[0x3F7+1]) << 2);
	*(ecc_code+2) 	= ((rsdata[0x3F7+1]) >> 6) | ((rsdata[0x3F7+2]) << 4);
	*(ecc_code+3) 	= ((rsdata[0x3F7+2]) >> 4) | ((rsdata[0x3F7+3]) << 6);
	*(ecc_code+4) 	= ((rsdata[0x3F7+3]) >> 2);
	*(ecc_code+5) 	= (unsigned char) rsdata[kk+4];
	*(ecc_code+6)	= ((rsdata[0x3F7+4])   >> 8) | ((rsdata[0x3F7+1+4]) << 2);
	*(ecc_code+7) 	= ((rsdata[0x3F7+1+4]) >> 6) | ((rsdata[0x3F7+2+4]) << 4);
	*(ecc_code+8) 	= ((rsdata[0x3F7+2+4]) >> 4) | ((rsdata[0x3F7+3+4]) << 6);
	*(ecc_code+9) 	= ((rsdata[0x3F7+3+4]) >> 2);

	return 0;
}

int mv_nand_correct_data_rs(struct mtd_info *mtd, u_char *data, u_char *store_ecc, u_char *calc_ecc)
{
	int ret,i=0;
	u_short rsdata[nn];

	if (!rs_initialized) {
		generate_gf();
		gen_poly();
		rs_initialized = 1;
	}

	if ((*(u32*)store_ecc == *(u32*)calc_ecc) &&
			(*(u32*)(store_ecc + 4) == *(u32*)(calc_ecc + 4)) &&
			(*(u16*)(store_ecc + 8) == *(u16*)(calc_ecc + 8)))
		return 0;

	for (i = 0; i < 512 ;i += 4) {
		if (*(u32*)(data+i) != 0xFFFFFFFF) {
			 
			goto correct;
		}
	}
	 
	return 0;

correct:

	for(i=512; i<nn; i++)
		rsdata[i] = 0;

	for (i = 0; i < 512; i++)
		rsdata[i] = (u_short) data[i];

	rsdata[kk]   = ( (*(store_ecc+1) & 0x03) <<8) | (*(store_ecc));
	rsdata[kk+1] = ( (*(store_ecc+2) & 0x0F) <<6) | (*(store_ecc+1)>>2);
	rsdata[kk+2] = ( (*(store_ecc+3) & 0x3F) <<4) | (*(store_ecc+2)>>4);
	rsdata[kk+3] = (*(store_ecc+4) <<2) | (*(store_ecc+3)>>6);

	rsdata[kk+4] = ( (*(store_ecc+1+5) & 0x03) <<8) | (*(store_ecc+5));
	rsdata[kk+5] = ( (*(store_ecc+2+5) & 0x0F) <<6) | (*(store_ecc+1+5)>>2);
	rsdata[kk+6] = ( (*(store_ecc+3+5) & 0x3F) <<4) | (*(store_ecc+2+5)>>4);
	rsdata[kk+7] = (*(store_ecc+4+5) <<2) | (*(store_ecc+3+5)>>6);

	ret = decode_rs(rsdata);

	if ((ret > tt) || (ret < 0)) {
		printk("%s: uncorrectable error !!!\n",__FUNCTION__);
		return -1;
	}

	for (i = 0; i < 512; i++)
		data[i] = (unsigned char)rsdata[i];

	return 0;
}

static void mv_nand_enable_hwecc(struct mtd_info *mtd, int mode)
{
	return;
}

#endif  
#endif

static void orion_nand_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *nc = mtd->priv;
	struct orion_nand_data *board = nc->priv;
	u32 offs;

	if (cmd == NAND_CMD_NONE)
		return;

	if (ctrl & NAND_CLE)
		offs = (1 << board->cle);
	else if (ctrl & NAND_ALE)
		offs = (1 << board->ale);
	else
		return;

	if (nc->options & NAND_BUSWIDTH_16)
		offs <<= 1;

	writeb(cmd, nc->IO_ADDR_W + offs);
}

static void orion_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	void __iomem *io_base = chip->IO_ADDR_R;
	uint64_t *buf64;
	int i = 0;

	while (len && (unsigned long)buf & 7) {
		*buf++ = readb(io_base);
		len--;
	}
	buf64 = (uint64_t *)buf;
	while (i < len/8) {
		 
		register uint64_t x asm ("r2");

		asm volatile ("ldrd\t%0, [%1]" : "=&r" (x) : "r" (io_base));
		buf64[i++] = x;
	}
	i *= 8;
	while (i < len)
		buf[i++] = readb(io_base);
}

static int __init orion_nand_probe(struct platform_device *pdev)
{
	struct mtd_info *mtd;
	struct nand_chip *nc;
	struct orion_nand_data *board;
	struct resource *res;
	void __iomem *io_base;
	int ret = 0;

	nc = kzalloc(sizeof(struct nand_chip) + sizeof(struct mtd_info), GFP_KERNEL);
	if (!nc) {
		printk(KERN_ERR "orion_nand: failed to allocate device structure.\n");
		ret = -ENOMEM;
		goto no_res;
	}
	mtd = (struct mtd_info *)(nc + 1);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		ret = -ENODEV;
		goto no_res;
	}

	io_base = ioremap(res->start, resource_size(res));
	if (!io_base) {
		printk(KERN_ERR "orion_nand: ioremap failed\n");
		ret = -EIO;
		goto no_res;
	}

	board = pdev->dev.platform_data;

	mtd->priv = nc;
	mtd->owner = THIS_MODULE;

	nc->priv = board;
	nc->IO_ADDR_R = nc->IO_ADDR_W = io_base;
	nc->cmd_ctrl = orion_nand_cmd_ctrl;
	nc->read_buf = orion_nand_read_buf;
	nc->ecc.mode = NAND_ECC_SOFT;

#if defined(CONFIG_SYNO_ARMADA) || defined(CONFIG_SYNO_ARMADA_V2)
#ifdef CONFIG_MTD_NAND_RS_ECC
	printk("Using %s ECC for NAND device\n",
		(ecc_mode == MV_NAND_ECC_4BIT ?
		"Reed-Solomon 4-bit" : "Hamming 1-bit"));

	if (ecc_mode == MV_NAND_ECC_4BIT) {
		nc->ecc.hwctl		= mv_nand_enable_hwecc;
		nc->ecc.calculate	= mv_nand_calculate_ecc_rs;
		nc->ecc.correct	= mv_nand_correct_data_rs;
		nc->ecc.size		= 512;
		nc->ecc.bytes		= 10;
		nc->ecc.layout = &mv_nand_rs_oobinfo;
		nc->ecc.mode = NAND_ECC_HW;
	}
#endif
#endif

	if (board->chip_delay)
		nc->chip_delay = board->chip_delay;

	if (board->width == 16)
		nc->options |= NAND_BUSWIDTH_16;

	if (board->dev_ready)
		nc->dev_ready = board->dev_ready;

	platform_set_drvdata(pdev, mtd);

	if (nand_scan(mtd, 1)) {
		ret = -ENXIO;
		goto no_dev;
	}

	mtd->name = "orion_nand";
	ret = mtd_device_parse_register(mtd, NULL, 0,
			board->parts, board->nr_parts);
	if (ret) {
		nand_release(mtd);
		goto no_dev;
	}

	return 0;

no_dev:
	platform_set_drvdata(pdev, NULL);
	iounmap(io_base);
no_res:
	kfree(nc);

	return ret;
}

static int __devexit orion_nand_remove(struct platform_device *pdev)
{
	struct mtd_info *mtd = platform_get_drvdata(pdev);
	struct nand_chip *nc = mtd->priv;

	nand_release(mtd);

	iounmap(nc->IO_ADDR_W);

	kfree(nc);

	return 0;
}

static struct platform_driver orion_nand_driver = {
	.remove		= __devexit_p(orion_nand_remove),
	.driver		= {
		.name	= "orion_nand",
		.owner	= THIS_MODULE,
	},
};

static int __init orion_nand_init(void)
{
	return platform_driver_probe(&orion_nand_driver, orion_nand_probe);
}

static void __exit orion_nand_exit(void)
{
	platform_driver_unregister(&orion_nand_driver);
}

module_init(orion_nand_init);
module_exit(orion_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tzachi Perelstein");
MODULE_DESCRIPTION("NAND glue for Orion platforms");
MODULE_ALIAS("platform:orion_nand");

#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mtt/mtt.h>
#include <linux/pinctrl/consumer.h>
#include <linux/mtt/systrace.h>
#include <linux/mtt/mtt.h>
#include <linux/debugfs.h>
#include <linux/of_device.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

MODULE_AUTHOR("Marc Titinger <marc.titinger@st.com>");
MODULE_DESCRIPTION("System Trace Modules driver.");
MODULE_LICENSE("GPL");

#ifndef __raw_writeq
#define __raw_writeq(v, a) \
	(__chk_io_ptr(a), *(volatile uint64_t __force *)(a) = (v))
#endif

struct st_systrace_ops {
	uint32_t devid;
	uint32_t ch_shift;
	uint32_t ch_offset;
	uint32_t ma_shift;
	uint32_t ts_offset;
	uint32_t ch_num;
	uint32_t peri_id;
	uint32_t cell_id;
};

struct st_systrace_dev {
	struct device *dev;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct resource *mem_base;
	void __iomem *base;
	void __iomem *regs;
	struct regmap *regmap;
	const struct st_systrace_ops *data;
	void *private;

	unsigned int last_ch_ker;
};

static uint32_t stm_ip_ma_shift;
static uint32_t stm_ip_ts_offset;

static inline void stm_write_buf(void *chan_addr, char *str, unsigned int len)
{
	register char *ptr = str;
	unsigned long long tmp = 0;

	void __iomem *dst = chan_addr;

	while (len > 8) {
		__raw_writeq(*(uint64_t *) ptr, dst);
		ptr += 8;
		len -= 8;
	}

	if (len == 8) {
		__raw_writeq(*(uint64_t *) ptr, (dst + stm_ip_ts_offset));
		return;
	};
	memcpy(&tmp, ptr, len);
	__raw_writeq(tmp, (dst + stm_ip_ts_offset));
}

static void *mtt_drv_stm_comp_alloc(const uint32_t comp_id, void *data)
{
	struct st_systrace_dev *trc_dev = (struct st_systrace_dev *)data;
	uint32_t ch_num = trc_dev->data->ch_num;
	uint32_t stm_ip_ch_shift = trc_dev->data->ch_shift;
	uint32_t stm_ip_ma_shift = trc_dev->data->ma_shift;

	unsigned int ret = (unsigned int)trc_dev->base;

	if (comp_id & MTT_COMPID_ST) {
		 
		ret += (((comp_id & MTT_COMPID_CHMSK)
		    >> MTT_COMPID_CHSHIFT) << stm_ip_ch_shift);

		ret += (((comp_id & MTT_COMPID_MAMSK)
		    >> MTT_COMPID_MASHIFT) << stm_ip_ma_shift);
		return (void *)ret;
	}

	if (trc_dev->last_ch_ker == MTT_CH_LIN_KER_INVALID(ch_num)) {
		ret += (MTT_CH_LIN_KMUX << stm_ip_ch_shift);
		return (void *)ret;
	}

	ret += (trc_dev->last_ch_ker << stm_ip_ch_shift);
	trc_dev->last_ch_ker++;
	return (void *)ret;
};

static int mtt_drv_stm_mmap(struct file *filp, struct vm_area_struct *vma,
			    void *data)
{
	struct st_systrace_dev *trc_dev =
	    (struct st_systrace_dev *)data;

	u64 off = (u64) (vma->vm_pgoff) << PAGE_SHIFT;
	u64 physical = trc_dev->mem_base->start + off;
	u64 vsize = (vma->vm_end - vma->vm_start);
	u64 psize = (trc_dev->mem_base->end - trc_dev->mem_base->start) - off;

	if (vsize > psize)
		vsize = psize;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_ops = NULL;

	mtt_printk(KERN_DEBUG "mtt_drv_stm_mmap: " \
		   "vm_start=%lx, vm_end=%lx, vm_pgoff=%lx, "
		   "physical=%llx (size=%llx)\n",
		   vma->vm_start, vma->vm_end, vma->vm_pgoff, physical, vsize);

	if (remap_pfn_range(vma, vma->vm_start, physical >> PAGE_SHIFT,
			    vsize, vma->vm_page_prot)) {
		printk(KERN_ERR " Failure returning from stm_mmap\n");
		return -EAGAIN;
	}
	return 0;
}

static void mtt_drv_stm_write(mtt_packet_t *p, int lock)
{
	struct mtt_component_obj *co = (struct mtt_component_obj *)p->comp;

	if (unlikely(((unsigned int)(co->private) & MTT_COMPID_ST) == 0))
		co->private = (void *)
		  ((unsigned int)(co->private) +
		   (raw_smp_processor_id() << stm_ip_ma_shift));

	stm_write_buf(co->private, p->u.buf, p->length);
}

static int systrace_remove(struct platform_device *pdev)
{
	struct st_systrace_dev *trc_dev = platform_get_drvdata(pdev);

	mtt_unregister_output_driver(trc_dev->private);

	return 0;
}

static void systrace_of_phyver(struct st_systrace_dev *trc_dev)
{
	uint32_t stm_pid;
	uint32_t stm_cid;
	stm_pid =
		((readl(trc_dev->regs + STM_IP_PID3_OFF)) << 24) +
		((readl(trc_dev->regs + STM_IP_PID2_OFF)) << 16) +
		((readl(trc_dev->regs + STM_IP_PID1_OFF)) << 8) +
		((readl(trc_dev->regs + STM_IP_PID0_OFF))) ;
	stm_cid =
		((readl(trc_dev->regs + STM_IP_CID3_OFF)) << 24) +
		((readl(trc_dev->regs + STM_IP_CID2_OFF)) << 16) +
		((readl(trc_dev->regs + STM_IP_CID1_OFF)) << 8) +
		((readl(trc_dev->regs + STM_IP_CID0_OFF))) ;
	printk(KERN_NOTICE "Systrace: STM IP revision is V%d\n",
				trc_dev->data->devid);
	printk(KERN_NOTICE "Systrace: STM_PID=0x%08x(0x%08x)\n",
				stm_pid, trc_dev->data->peri_id);
	printk(KERN_NOTICE "Systrace: STM_CID=0x%08x(0x%08x)\n",
				stm_cid, trc_dev->data->cell_id);
}

static int systrace_of_phyconf(struct st_systrace_dev *trc_dev)
{
	struct device_node	*np = trc_dev->dev->of_node;
	unsigned int		num_sysc = 0;
	struct device_node	*cnfbank;
	struct device_node	*cnfregs;

	cnfbank = of_get_child_by_name(np, "systrace-phy-config");
	if (cnfbank == NULL) {
		if (trc_dev->data->devid == STM_IPv3_VERSION) {
			__raw_writel(0x00600, trc_dev->regs + STM_IPv3_CR_OFF);
			__raw_writel(0x00000, trc_dev->regs + STM_IPv3_MCR_OFF);
			__raw_writel(0x03ff,  trc_dev->regs + STM_IPv3_TER_OFF);
			__raw_writel(1,       trc_dev->regs + STM_IPv3_FTR_OFF);
			__raw_writel(1,       trc_dev->regs + STM_IPv3_CTR_OFF);
		} else if (trc_dev->data->devid == STM_IPv1_VERSION) {
			 
			__raw_writel(0x00C0, trc_dev->regs + STM_IPv1_CR_OFF);
			__raw_writel(0x0000, trc_dev->regs + STM_IPv1_MMC_OFF);
			__raw_writel(0x023d, trc_dev->regs + STM_IPv1_TER_OFF);
		} else {
			dev_err(trc_dev->dev, "phyconf: unsupported IP\n");
			return -EINVAL;
		}

		dev_warn(trc_dev->dev, "No systrace-phy-config specified\n");
		return 0;
	}

	do {
		size_t                phyconfofs;
		uint32_t              phyconfval;
		const char           *phyregname;
		char                  bufname[16];
		uint32_t              temp;

		snprintf(bufname, 16, "stm_reg%d", num_sysc);
		cnfregs = of_get_child_by_name(cnfbank, bufname);
		if (cnfregs == NULL)
			break;

		of_property_read_string(cnfregs, "nam", &phyregname);
		of_property_read_u32(cnfregs, "ofs", &phyconfofs);
		of_property_read_u32(cnfregs, "val", &phyconfval);

		temp = (uint32_t)readl(trc_dev->regs + phyconfofs);
		__raw_writel(phyconfval, trc_dev->regs + phyconfofs);

		dev_notice(trc_dev->dev, "Systrace: %s[0x%04x] = 0x%08x (prev."
		    " 0x%08x)\n",
		    phyregname,
		    (uint32_t)(phyconfofs),
		    (uint32_t)readl(trc_dev->regs + phyconfofs),
		    temp);
		num_sysc++;
	} while (cnfregs != NULL);

	if (!num_sysc)
		return -EINVAL;
	else
		return 0;
}

static int systrace_of_sysconf(struct st_systrace_dev *trc_dev)
{
	struct device_node	*np = trc_dev->dev->of_node;
	struct platform_device	*pdev = to_platform_device(trc_dev->dev);
	char			bufname[16];
	unsigned int		num_sysc = 0;
	int			syscfg;
	struct device_node	*cnfvals;
	struct resource		*res;
	int			ret;

	trc_dev->regmap = syscon_regmap_lookup_by_phandle(np, "st,syscfg");
	if (IS_ERR(trc_dev->regmap)) {
		dev_info(&pdev->dev, "No syscfg phandle specified\n");
		return 0;
	}

	cnfvals = of_get_child_by_name(np, "v_sysconfs");
	if (IS_ERR(cnfvals)) {
		dev_err(&pdev->dev, "No v_sysconfs specified\n");
		return PTR_ERR(cnfvals);
	}

	do {
		uint32_t sysconfval;
		uint32_t sysconfvalo;

		snprintf(bufname, 16, "syscfg%d", num_sysc);
		res = platform_get_resource_byname(pdev,
				IORESOURCE_MEM, bufname);
		if (!res)
			break;
		syscfg = res->start;

		snprintf(bufname, 16, "sysconf%d", num_sysc);
		if(of_property_read_u32(cnfvals, bufname, &sysconfval)) {
			dev_err(&pdev->dev, "Fail to get %s value\n", bufname);
			return -ENODEV;
		}

		ret = regmap_read(trc_dev->regmap, syscfg, &sysconfvalo);
		if (ret < 0) {
			dev_err(&pdev->dev, "Syscfg read error\n");
			return ret;
		}

		ret = regmap_write(trc_dev->regmap, syscfg, sysconfval);
		if (ret < 0) {
			dev_err(&pdev->dev, "Syscfg write error\n");
			return ret;
		}

		ret = regmap_read(trc_dev->regmap, syscfg, &sysconfval);
		if (ret < 0) {
			dev_err(&pdev->dev, "Syscfg read error\n");
			return ret;
		}

		dev_notice(&pdev->dev, "Systrace: syscfg%d=0x%08x (prev:"
				" 0x%08x)\n",
				num_sysc, sysconfval, sysconfvalo);
		num_sysc++;
	} while(res);

	return 0;
}

static void systrace_flush(struct st_systrace_dev *drv_data)
{
	int iloop = 0;
	uint32_t stm_ip_ts_offset = drv_data->data->ts_offset;
	uint32_t stm_ip_ch_offset = drv_data->data->ch_offset;

	for (iloop = 0; iloop < 20; iloop++) {
		void __iomem *base = drv_data->base +
			(iloop*stm_ip_ch_offset) + stm_ip_ts_offset;
		__raw_writeq(0x0LL, base);
	}
}

#ifdef SYSTRACE_DRV_DEBUG
static void systrace_test_pattern(struct st_systrace_dev *drv_data)
{
	unsigned long long value_to_write64[8];
	unsigned long     *value_to_write  =
				(unsigned long *)value_to_write64;
	char              *value_to_string =
				(unsigned char *)(&(value_to_write[5]));
	uint32_t stm_ip_ch_offset = drv_data->data->ch_offset;
	unsigned int ux = 0;
	unsigned int ix = 0;
	value_to_write[ix++] = 0xeee00300;  
	value_to_write[ix++] = 0x00000000;  
	 
	value_to_write[ix++] = 0x00000000;  
	value_to_write[ix++] = 0x00000000;  
	 
	value_to_write[ix++] = 0x01020018;  
	value_to_write[ix++] = 0x70747473;
	 
	value_to_write[ix++] = 0x5453203A;
	value_to_write[ix++] = 0x4e49204d;
	 
	value_to_write[ix++] = 0x41495449;
	value_to_write[ix++] = 0x455A494C;
	 
	value_to_write[ix++] = 0x00000044;
	value_to_write[ix++] = 0x0;
	 
	stm_write_buf(drv_data->base, (char *)value_to_write, ix*4);

	for (ux = 0; ux < (256*stm_ip_ch_offset); ux += stm_ip_ch_offset) {
		sprintf(value_to_string, "Hello World 1 from  %3d ",
			ux/0x100);
		stm_write_buf(drv_data->base+ux, (char *)value_to_write,
			ix*4);
		sprintf(value_to_string, "Hello World 2 from  %3d ",
			ux/0x100);
		stm_write_buf(drv_data->base+ux, (char *)value_to_write,
			ix*4);
		sprintf(value_to_string, "Hello World 3 from  %3d ",
			ux/0x100);
		stm_write_buf(drv_data->base+ux, (char *)value_to_write,
			ix*4);
		sprintf(value_to_string, "Hello World 4 from  %3d ",
			ux/0x100);
		stm_write_buf(drv_data->base+ux, (char *)value_to_write,
			ix*4);
	}
}
#endif

static struct st_systrace_ops stih41x_IPv1_ops = {
	.devid =     STM_IPv1_VERSION,
	.ch_shift =  STM_IPv1_CH_SHIFT,
	.ch_offset = STM_IPv1_CH_OFFSET,
	.ma_shift =  STM_IPv1_MA_SHIFT,
	.ts_offset = STM_IPv1_TS_OFFSET,
	.ch_num =    STM_IPv1_CH_NUMBER,
	.peri_id =   STM_IPv1_PERI_ID,
	.cell_id =   STM_CELL_ID,
};

static struct st_systrace_ops stihxxx_IPv3_ops = {
	.devid =     STM_IPv3_VERSION,
	.ch_shift =  STM_IPv3_CH_SHIFT,
	.ch_offset = STM_IPv3_CH_OFFSET,
	.ma_shift =  STM_IPv3_MA_SHIFT,
	.ts_offset = STM_IPv3_TS_OFFSET,
	.ch_num =    STM_IPv3_CH_NUMBER,
	.peri_id =   STM_IPv3_PERI_ID,
	.cell_id =   STM_CELL_ID,
};

static struct of_device_id st_stm_systrace_match[] = {
	{
		.compatible = "st,systrace-ipv1",
		.data = (void *)&stih41x_IPv1_ops},
	{
		.compatible = "st,systrace-ipv3",
		.data = (void *)&stihxxx_IPv3_ops},
	{},
};

static int systrace_probe(struct platform_device *pdev)
{
	const struct of_device_id *of_id;
	struct device_node *np = pdev->dev.of_node;
	struct mtt_output_driver *mtt_drv;
	struct st_systrace_dev *trc_dev;
	struct resource *res;
	int ret;

	trc_dev = devm_kzalloc(&pdev->dev, sizeof(*trc_dev), GFP_KERNEL);
	if (!trc_dev)
		return -ENOMEM;

	memset(trc_dev, 0, sizeof(*trc_dev));

	if (np && (of_id = of_match_node(st_stm_systrace_match, np)))
		trc_dev->data = of_id->data;

	if (!trc_dev->data) {
		dev_err(&pdev->dev, "compatible data not provided\n");
		return -EINVAL;
	}

	trc_dev->dev = &pdev->dev;

	stm_ip_ma_shift = trc_dev->data->ma_shift;
	stm_ip_ts_offset = trc_dev->data->ts_offset;

	trc_dev->mem_base = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (IS_ERR(trc_dev->mem_base))
		return PTR_ERR(trc_dev->mem_base);

	trc_dev->base = devm_ioremap_nocache(&pdev->dev,
					trc_dev->mem_base->start,
					trc_dev->mem_base->end -
					trc_dev->mem_base->start + 1);
	if (IS_ERR(trc_dev->base))
		return PTR_ERR(trc_dev->base);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (IS_ERR(res))
		return PTR_ERR(res);

	trc_dev->regs = devm_ioremap_nocache(&pdev->dev, res->start,
					 res->end - res->start + 1);
	if (IS_ERR(trc_dev->regs))
		return PTR_ERR(trc_dev->regs);

	trc_dev->pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(trc_dev->pinctrl))
		return PTR_ERR(trc_dev->pinctrl);

	trc_dev->pins_default = pinctrl_lookup_state(trc_dev->pinctrl,
						 PINCTRL_STATE_DEFAULT);
	if (IS_ERR(trc_dev->pins_default)) {
		dev_err(&pdev->dev, "could not get default pinstate\n");
		return -EIO;
	}
	else
		pinctrl_select_state(trc_dev->pinctrl, trc_dev->pins_default);

	ret = systrace_of_sysconf(trc_dev);
	if (ret) {
		dev_err(&pdev->dev, "error: sysconf failed\n");
		return ret;
	}

	ret = systrace_of_phyconf(trc_dev);
	if (ret) {
		dev_err(&pdev->dev, "error: phyconf failed\n");
		return ret;
	}

	systrace_of_phyver(trc_dev);

	mtt_drv = devm_kzalloc(&pdev->dev,
			sizeof(struct mtt_output_driver), GFP_KERNEL);
	if (IS_ERR(mtt_drv))
		return PTR_ERR(mtt_drv);

	mtt_drv->write_func = mtt_drv_stm_write;
	mtt_drv->mmap_func = mtt_drv_stm_mmap;
	mtt_drv->comp_alloc_func = mtt_drv_stm_comp_alloc;
	mtt_drv->guid = MTT_DRV_GUID_STM;
	mtt_drv->private = trc_dev;
	mtt_drv->last_error = 0;
	mtt_drv->devid = trc_dev->data->devid;
	mtt_drv->last_ch_ker = MTT_CH_LIN_KER_FIRST(trc_dev->data->ch_num);
	mtt_drv->invl_ch_ker = MTT_CH_LIN_KER_INVALID(trc_dev->data->ch_num);
	trc_dev->last_ch_ker = MTT_CH_LIN_KER_FIRST(trc_dev->data->ch_num);
	trc_dev->private = mtt_drv;

	platform_set_drvdata(pdev, trc_dev);

	mtt_register_output_driver(mtt_drv);

	systrace_flush(trc_dev);

#ifdef SYSTRACE_DRV_DEBUG
	 
	systrace_test_pattern(drv_data);
#endif

	dev_info(&pdev->dev, "Systrace: driver probed.\n");
	return 0;
}

#ifdef MY_DEF_HERE
#ifdef CONFIG_PM
static int systrace_suspend(struct device *dev)
{
	dev_notice(dev, "Systrace: driver suspend.\n");
	pinctrl_pm_select_sleep_state(dev);

	return 0;
}

static int systrace_resume(struct device *dev)
{
	struct st_systrace_dev *trc_dev = dev_get_drvdata(dev);

	dev_notice(dev, "Systrace: driver resume.\n");
	pinctrl_pm_select_default_state(dev);
	systrace_of_sysconf(trc_dev);
	systrace_of_phyconf(trc_dev);

	return 0;
}

static SIMPLE_DEV_PM_OPS(systrace_pm_ops,
	systrace_suspend, systrace_resume);

#define SYSTRACE_PM_OPS                (&systrace_pm_ops)
#else
#define SYSTRACE_PM_OPS                ((void *)NULL)
#endif
#endif  

MODULE_DEVICE_TABLE(of, st_stm_systrace_match);
static struct platform_driver systrace_driver = {
	.probe = systrace_probe,
	.remove = systrace_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "stm-systrace",
#ifdef MY_DEF_HERE
		   .pm = SYSTRACE_PM_OPS,
#endif  
		   .of_match_table = st_stm_systrace_match,
	},
};
module_platform_driver(systrace_driver);

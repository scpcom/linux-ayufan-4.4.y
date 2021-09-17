#define pr_fmt(fmt) "himmc: " fmt

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/mmc/host.h>
#include <linux/mmc/dw_mmc.h>
#include <linux/of.h>
#include <linux/of_gpio.h>

#include "dw_mmc.h"
#include "dw_mmc-pltfm.h"

#define HIMMC_IO_SIZE              0x1000
#define HI3716CV200_MMC_BASE_CRG   IO_ADDRESS(0xF8A22000)
#define HI3716CV200_PERI_CRG39     0x9C
#define HI3716CV200_PERI_CRG40     0xA0
#define HI3716CV200_MMC0_IOBASE    0xF9820000
#define HI3716CV200_MMC1_IOBASE    0xF9830000
#define HI3716CV200_MMC0_INTR      66
#define HI3716CV200_MMC1_INTR      67

#define HIMMC_NAME                 "himmc"
/*****************************************************************************/

static struct dw_mci_board himmc_board_data = {
	.num_slots       = 1,
	.quirks          = 0,
	.bus_hz          = 50 * 1000 * 1000,
	.detect_delay_ms = 200,
	.fifo_depth      = 32,
	.caps            = 0,
};
/*****************************************************************************/

static int himmc_priv_init(struct dw_mci *host)
{
	host->pdata = &himmc_board_data;
	return 0;
}
/*****************************************************************************/

static unsigned long himmc_caps[2] = {
	MMC_CAP_ERASE | MMC_CAP_4_BIT_DATA | MMC_CAP_SD_HIGHSPEED
		| MMC_CAP_MMC_HIGHSPEED,
	MMC_CAP_ERASE | MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA
		| MMC_CAP_SD_HIGHSPEED | MMC_CAP_MMC_HIGHSPEED,
};

static struct dw_mci_drv_data himmc_drv_data = {
	.caps = himmc_caps,
	.init = himmc_priv_init,
};
/*****************************************************************************/

static int himmc_pltfm_probe(struct platform_device *pdev)
{
	return dw_mci_pltfm_register(pdev, &himmc_drv_data);
}
/*****************************************************************************/

static void himmc_pltfm_shutdown(struct platform_device *pdev)
{
}
/*****************************************************************************/

static struct platform_driver himmc_pltfm_driver = {
	.probe    = himmc_pltfm_probe,
	.remove   = __exit_p(dw_mci_pltfm_remove),
	.shutdown = himmc_pltfm_shutdown,
	.driver   = {
		.name = HIMMC_NAME,
		.pm   = &dw_mci_pltfm_pmops,
	},
};
/*****************************************************************************/

static struct resource himmc0_resources[] = {
	[0] = {
		.start  = HI3716CV200_MMC0_IOBASE,
		.end    = HI3716CV200_MMC0_IOBASE + HIMMC_IO_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = HI3716CV200_MMC_BASE_CRG + HI3716CV200_PERI_CRG39,
		.end    = HI3716CV200_MMC_BASE_CRG + HI3716CV200_PERI_CRG39,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {
		.start  = HI3716CV200_MMC0_INTR,
		.end    = HI3716CV200_MMC0_INTR,
		.flags  = IORESOURCE_IRQ,
	},
};
/*****************************************************************************/

static struct resource himmc1_resources[] = {
	[0] = {
		.start  = HI3716CV200_MMC1_IOBASE,
		.end    = HI3716CV200_MMC1_IOBASE + HIMMC_IO_SIZE - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = HI3716CV200_MMC_BASE_CRG + HI3716CV200_PERI_CRG40,
		.end    = HI3716CV200_MMC_BASE_CRG + HI3716CV200_PERI_CRG40,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {
		.start  = HI3716CV200_MMC1_INTR,
		.end    = HI3716CV200_MMC1_INTR,
		.flags  = IORESOURCE_IRQ,
	},
};
/*****************************************************************************/

static void himmc_platdev_release(struct device *dev)
{
}
/*****************************************************************************/

static u64 himmc_dmamask = DMA_BIT_MASK(32);

/*****************************************************************************/

static struct platform_device himmc0_platform_device = {
	.name = HIMMC_NAME,
	.id = 0,
	.dev = {
		.release = himmc_platdev_release,
		.dma_mask = &himmc_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		},
	.num_resources = ARRAY_SIZE(himmc0_resources),
	.resource = himmc0_resources,
};
/*****************************************************************************/

static struct platform_device himmc1_platform_device = {
	.name = HIMMC_NAME,
	.id = 1,
	.dev = {
		.release = himmc_platdev_release,
		.dma_mask = &himmc_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
		},
	.num_resources = ARRAY_SIZE(himmc1_resources),
	.resource = himmc1_resources,
};
/*****************************************************************************/

static int __init himmc_init(void)
{
	int ret;

	ret = platform_device_register(&himmc1_platform_device);
	if (ret) {
		pr_err("device register fail.\n");
		return ret;
	}

	ret = platform_device_register(&himmc0_platform_device);
	if (ret) {
		platform_device_unregister(&himmc1_platform_device);
		pr_err("device register fail.\n");
		return ret;
	}

	ret = platform_driver_register(&himmc_pltfm_driver);
	if (ret) {
		platform_device_unregister(&himmc0_platform_device);
		platform_device_unregister(&himmc1_platform_device);
		pr_err("driver register fail.\n");
		return ret;
	}
	return ret;
}
/*****************************************************************************/

static void __exit himmc_exit(void)
{
	platform_driver_unregister(&himmc_pltfm_driver);
	platform_device_unregister(&himmc1_platform_device);
	platform_device_unregister(&himmc0_platform_device);
}

module_init(himmc_init);
module_exit(himmc_exit);

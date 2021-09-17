#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/leds.h>
#include <linux/pwm.h>
#include <linux/leds_pwm.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/gpio_keys.h>
#if defined(MY_ABC_HERE)
#include <linux/pm_opp.h>
#else  
#include <linux/opp.h>
#endif  
#include <linux/cpu.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/mmc/host.h>
#include <linux/usb/phy.h>
#include <linux/usb/nop-usb-xceiv.h>

#include <linux/regulator/machine.h>
#include <linux/i2c/twl.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>

#include <video/omapdss.h>
#include <video/omap-panel-data.h>
#include <linux/platform_data/mtd-nand-omap2.h>

#include "common.h"
#include "omap_device.h"
#include "gpmc.h"
#include "soc.h"
#include "mux.h"
#include "hsmmc.h"
#include "pm.h"
#include "board-flash.h"
#include "common-board-devices.h"

#define	NAND_CS	0

static struct pwm_lookup pwm_lookup[] = {
	 
	PWM_LOOKUP("twl-pwmled", 1, "leds_pwm", "beagleboard::pmu_stat"),
};

static struct led_pwm pwm_leds[] = {
	{
		.name		= "beagleboard::pmu_stat",
		.max_brightness	= 127,
		.pwm_period_ns	= 7812500,
	},
};

static struct led_pwm_platform_data pwm_data = {
	.num_leds	= ARRAY_SIZE(pwm_leds),
	.leds		= pwm_leds,
};

static struct platform_device leds_pwm = {
	.name	= "leds_pwm",
	.id	= -1,
	.dev	= {
		.platform_data = &pwm_data,
	},
};

enum {
	OMAP3BEAGLE_BOARD_UNKN = 0,
	OMAP3BEAGLE_BOARD_AXBX,
	OMAP3BEAGLE_BOARD_C1_3,
	OMAP3BEAGLE_BOARD_C4,
	OMAP3BEAGLE_BOARD_XM,
	OMAP3BEAGLE_BOARD_XMC,
};

static u8 omap3_beagle_version;

static struct {
	int mmc1_gpio_wp;
	bool usb_pwr_level;	 
	int dvi_pd_gpio;
	int usr_button_gpio;
	int mmc_caps;
} beagle_config = {
	.mmc1_gpio_wp = -EINVAL,
	.usb_pwr_level = 0,
	.dvi_pd_gpio = -EINVAL,
	.usr_button_gpio = 4,
	.mmc_caps = MMC_CAP_4_BIT_DATA | MMC_CAP_8_BIT_DATA,
};

static struct gpio omap3_beagle_rev_gpios[] __initdata = {
	{ 171, GPIOF_IN, "rev_id_0"    },
	{ 172, GPIOF_IN, "rev_id_1" },
	{ 173, GPIOF_IN, "rev_id_2"    },
};

static void __init omap3_beagle_init_rev(void)
{
	int ret;
	u16 beagle_rev = 0;

	omap_mux_init_gpio(171, OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_gpio(172, OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_gpio(173, OMAP_PIN_INPUT_PULLUP);

	ret = gpio_request_array(omap3_beagle_rev_gpios,
				 ARRAY_SIZE(omap3_beagle_rev_gpios));
	if (ret < 0) {
		printk(KERN_ERR "Unable to get revision detection GPIO pins\n");
		omap3_beagle_version = OMAP3BEAGLE_BOARD_UNKN;
		return;
	}

	beagle_rev = gpio_get_value(171) | (gpio_get_value(172) << 1)
			| (gpio_get_value(173) << 2);

	gpio_free_array(omap3_beagle_rev_gpios,
			ARRAY_SIZE(omap3_beagle_rev_gpios));

	switch (beagle_rev) {
	case 7:
		printk(KERN_INFO "OMAP3 Beagle Rev: Ax/Bx\n");
		omap3_beagle_version = OMAP3BEAGLE_BOARD_AXBX;
		beagle_config.mmc1_gpio_wp = 29;
		beagle_config.dvi_pd_gpio = 170;
		beagle_config.usr_button_gpio = 7;
		break;
	case 6:
		printk(KERN_INFO "OMAP3 Beagle Rev: C1/C2/C3\n");
		omap3_beagle_version = OMAP3BEAGLE_BOARD_C1_3;
		beagle_config.mmc1_gpio_wp = 23;
		beagle_config.dvi_pd_gpio = 170;
		beagle_config.usr_button_gpio = 7;
		break;
	case 5:
		printk(KERN_INFO "OMAP3 Beagle Rev: C4\n");
		omap3_beagle_version = OMAP3BEAGLE_BOARD_C4;
		beagle_config.mmc1_gpio_wp = 23;
		beagle_config.dvi_pd_gpio = 170;
		beagle_config.usr_button_gpio = 7;
		break;
	case 0:
		printk(KERN_INFO "OMAP3 Beagle Rev: xM Ax/Bx\n");
		omap3_beagle_version = OMAP3BEAGLE_BOARD_XM;
		beagle_config.usb_pwr_level = 1;
		beagle_config.mmc_caps &= ~MMC_CAP_8_BIT_DATA;
		break;
	case 2:
		printk(KERN_INFO "OMAP3 Beagle Rev: xM C\n");
		omap3_beagle_version = OMAP3BEAGLE_BOARD_XMC;
		beagle_config.mmc_caps &= ~MMC_CAP_8_BIT_DATA;
		break;
	default:
		printk(KERN_INFO "OMAP3 Beagle Rev: unknown %hd\n", beagle_rev);
		omap3_beagle_version = OMAP3BEAGLE_BOARD_UNKN;
	}
}

static struct mtd_partition omap3beagle_nand_partitions[] = {
	 
	{
		.name		= "X-Loader",
		.offset		= 0,
		.size		= 4 * NAND_BLOCK_SIZE,
		.mask_flags	= MTD_WRITEABLE,	 
	},
	{
		.name		= "U-Boot",
		.offset		= MTDPART_OFS_APPEND,	 
		.size		= 15 * NAND_BLOCK_SIZE,
		.mask_flags	= MTD_WRITEABLE,	 
	},
	{
		.name		= "U-Boot Env",
		.offset		= MTDPART_OFS_APPEND,	 
		.size		= 1 * NAND_BLOCK_SIZE,
	},
	{
		.name		= "Kernel",
		.offset		= MTDPART_OFS_APPEND,	 
		.size		= 32 * NAND_BLOCK_SIZE,
	},
	{
		.name		= "File System",
		.offset		= MTDPART_OFS_APPEND,	 
		.size		= MTDPART_SIZ_FULL,
	},
};

static struct tfp410_platform_data dvi_panel = {
	.i2c_bus_num = 3,
	.power_down_gpio = -1,
};

static struct omap_dss_device beagle_dvi_device = {
	.type = OMAP_DISPLAY_TYPE_DPI,
	.name = "dvi",
	.driver_name = "tfp410",
	.data = &dvi_panel,
	.phy.dpi.data_lines = 24,
};

static struct omap_dss_device beagle_tv_device = {
	.name = "tv",
	.driver_name = "venc",
	.type = OMAP_DISPLAY_TYPE_VENC,
	.phy.venc.type = OMAP_DSS_VENC_TYPE_SVIDEO,
};

static struct omap_dss_device *beagle_dss_devices[] = {
	&beagle_dvi_device,
	&beagle_tv_device,
};

static struct omap_dss_board_info beagle_dss_data = {
	.num_devices = ARRAY_SIZE(beagle_dss_devices),
	.devices = beagle_dss_devices,
	.default_device = &beagle_dvi_device,
};

#include "sdram-micron-mt46h32m32lf-6.h"

static struct omap2_hsmmc_info mmc[] = {
	{
		.mmc		= 1,
		.caps		= MMC_CAP_4_BIT_DATA,
		.gpio_wp	= -EINVAL,
		.deferred	= true,
	},
	{}	 
};

static struct regulator_consumer_supply beagle_vmmc1_supply[] = {
	REGULATOR_SUPPLY("vmmc", "omap_hsmmc.0"),
};

static struct regulator_consumer_supply beagle_vsim_supply[] = {
	REGULATOR_SUPPLY("vmmc_aux", "omap_hsmmc.0"),
};

static struct gpio_led gpio_leds[];

static struct nop_usb_xceiv_platform_data hsusb2_phy_data = {
	.needs_vcc = true,
};

static struct usbhs_phy_data phy_data[] = {
	{
		.port = 2,
		.reset_gpio = 147,
		.vcc_gpio = -1,		 
		.vcc_polarity = 1,	 
		.platform_data = &hsusb2_phy_data,
	},
};

static int beagle_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	int r;

	mmc[0].gpio_wp = beagle_config.mmc1_gpio_wp;
	 
	mmc[0].gpio_cd = gpio + 0;
	omap_hsmmc_late_init(mmc);

	if (cpu_is_omap3630()) {
		 
		r = gpio_request_one(gpio + 1, GPIOF_OUT_INIT_LOW,
				     "nDVI_PWR_EN");
		if (r)
			pr_err("%s: unable to configure nDVI_PWR_EN\n",
				__func__);

		beagle_config.dvi_pd_gpio = gpio + 2;

	} else {
		 
		if (gpio_request_one(gpio + 1, GPIOF_IN, "EHCI_nOC"))
			pr_err("%s: unable to configure EHCI_nOC\n", __func__);
	}
	dvi_panel.power_down_gpio = beagle_config.dvi_pd_gpio;

	phy_data[0].vcc_gpio = gpio + TWL4030_GPIO_MAX;
	phy_data[0].vcc_polarity = beagle_config.usb_pwr_level;

	usbhs_init_phys(phy_data, ARRAY_SIZE(phy_data));
	return 0;
}

static struct twl4030_gpio_platform_data beagle_gpio_data = {
	.use_leds	= true,
	.pullups	= BIT(1),
	.pulldowns	= BIT(2) | BIT(6) | BIT(7) | BIT(8) | BIT(13)
				| BIT(15) | BIT(16) | BIT(17),
	.setup		= beagle_twl_gpio_setup,
};

static struct regulator_init_data beagle_vmmc1 = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(beagle_vmmc1_supply),
	.consumer_supplies	= beagle_vmmc1_supply,
};

static struct regulator_init_data beagle_vsim = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(beagle_vsim_supply),
	.consumer_supplies	= beagle_vsim_supply,
};

static struct twl4030_platform_data beagle_twldata = {
	 
	.gpio		= &beagle_gpio_data,
	.vmmc1		= &beagle_vmmc1,
	.vsim		= &beagle_vsim,
};

static struct i2c_board_info __initdata beagle_i2c_eeprom[] = {
       {
               I2C_BOARD_INFO("eeprom", 0x50),
       },
};

static int __init omap3_beagle_i2c_init(void)
{
	omap3_pmic_get_config(&beagle_twldata,
			TWL_COMMON_PDATA_USB | TWL_COMMON_PDATA_MADC |
			TWL_COMMON_PDATA_AUDIO,
			TWL_COMMON_REGULATOR_VDAC | TWL_COMMON_REGULATOR_VPLL2);

	beagle_twldata.vpll2->constraints.name = "VDVI";

	omap3_pmic_init("twl4030", &beagle_twldata);
	 
	omap_register_i2c_bus(3, 100, beagle_i2c_eeprom, ARRAY_SIZE(beagle_i2c_eeprom));
	return 0;
}

static struct gpio_led gpio_leds[] = {
	{
		.name			= "beagleboard::usr0",
		.default_trigger	= "heartbeat",
		.gpio			= 150,
	},
	{
		.name			= "beagleboard::usr1",
		.default_trigger	= "mmc0",
		.gpio			= 149,
	},
};

static struct gpio_led_platform_data gpio_led_info = {
	.leds		= gpio_leds,
	.num_leds	= ARRAY_SIZE(gpio_leds),
};

static struct platform_device leds_gpio = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_led_info,
	},
};

static struct gpio_keys_button gpio_buttons[] = {
	{
		.code			= BTN_EXTRA,
		 
		.gpio			= -EINVAL,
		.desc			= "user",
		.wakeup			= 1,
	},
};

static struct gpio_keys_platform_data gpio_key_info = {
	.buttons	= gpio_buttons,
	.nbuttons	= ARRAY_SIZE(gpio_buttons),
};

static struct platform_device keys_gpio = {
	.name	= "gpio-keys",
	.id	= -1,
	.dev	= {
		.platform_data	= &gpio_key_info,
	},
};

static struct platform_device madc_hwmon = {
	.name	= "twl4030_madc_hwmon",
	.id	= -1,
};

static struct platform_device *omap3_beagle_devices[] __initdata = {
	&leds_gpio,
	&keys_gpio,
	&madc_hwmon,
	&leds_pwm,
};

static struct usbhs_omap_platform_data usbhs_bdata __initdata = {
	.port_mode[1] = OMAP_EHCI_PORT_MODE_PHY,
};

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#endif

static int __init beagle_opp_init(void)
{
	int r = 0;

	if (!machine_is_omap3_beagle())
		return 0;

	r = omap3_opp_init();
	if (r < 0 && (r != -EEXIST)) {
		pr_err("%s: opp default init failed\n", __func__);
		return r;
	}

	if (cpu_is_omap3630()) {
		struct device *mpu_dev, *iva_dev;

		mpu_dev = get_cpu_device(0);
		iva_dev = omap_device_get_by_hwmod_name("iva");

		if (IS_ERR(mpu_dev) || IS_ERR(iva_dev)) {
			pr_err("%s: Aiee.. no mpu/dsp devices? %p %p\n",
				__func__, mpu_dev, iva_dev);
			return -ENODEV;
		}
		 
#if defined(MY_ABC_HERE)
		r = dev_pm_opp_enable(mpu_dev, 800000000);
#else  
		r = opp_enable(mpu_dev, 800000000);
#endif  
		 
#if defined(MY_ABC_HERE)
		r |= dev_pm_opp_enable(iva_dev, 660000000);
#else  
		r |= opp_enable(iva_dev, 660000000);
#endif  
		 
		if (r) {
			pr_err("%s: failed to enable higher opp %d\n",
				__func__, r);
			 
#if defined(MY_ABC_HERE)
			dev_pm_opp_disable(mpu_dev, 800000000);
			dev_pm_opp_disable(iva_dev, 660000000);
#else  
			opp_disable(mpu_dev, 800000000);
			opp_disable(iva_dev, 660000000);
#endif  
		}
	}
	return 0;
}
omap_device_initcall(beagle_opp_init);

static void __init omap3_beagle_init(void)
{
	omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);
	omap3_beagle_init_rev();

	if (gpio_is_valid(beagle_config.mmc1_gpio_wp))
		omap_mux_init_gpio(beagle_config.mmc1_gpio_wp, OMAP_PIN_INPUT);
	mmc[0].caps = beagle_config.mmc_caps;
	omap_hsmmc_init(mmc);

	omap3_beagle_i2c_init();

	gpio_buttons[0].gpio = beagle_config.usr_button_gpio;

	platform_add_devices(omap3_beagle_devices,
			ARRAY_SIZE(omap3_beagle_devices));
	if (gpio_is_valid(beagle_config.dvi_pd_gpio))
		omap_mux_init_gpio(beagle_config.dvi_pd_gpio, OMAP_PIN_OUTPUT);
	omap_display_init(&beagle_dss_data);
	omap_serial_init();
	omap_sdrc_init(mt46h32m32lf6_sdrc_params,
				  mt46h32m32lf6_sdrc_params);

	usb_bind_phy("musb-hdrc.0.auto", 0, "twl4030_usb");
	usb_musb_init(NULL);

	usbhs_init(&usbhs_bdata);

	board_nand_init(omap3beagle_nand_partitions,
			ARRAY_SIZE(omap3beagle_nand_partitions), NAND_CS,
			NAND_BUSWIDTH_16, NULL);
	omap_twl4030_audio_init("omap3beagle", NULL);

	omap_mux_init_signal("sys_drm_msecure", OMAP_PIN_OFF_OUTPUT_HIGH);

	omap_mux_init_signal("sdrc_cke0", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("sdrc_cke1", OMAP_PIN_OUTPUT);

	pwm_add_table(pwm_lookup, ARRAY_SIZE(pwm_lookup));
}

MACHINE_START(OMAP3_BEAGLE, "OMAP3 Beagle Board")
	 
	.atag_offset	= 0x100,
	.reserve	= omap_reserve,
	.map_io		= omap3_map_io,
	.init_early	= omap3_init_early,
	.init_irq	= omap3_init_irq,
	.handle_irq	= omap3_intc_handle_irq,
	.init_machine	= omap3_beagle_init,
	.init_late	= omap3_init_late,
	.init_time	= omap3_secure_sync32k_timer_init,
	.restart	= omap3xxx_restart,
MACHINE_END

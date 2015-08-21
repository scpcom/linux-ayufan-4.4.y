#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>

#include "pinctrl-utils.h"
#include "core.h"

#define LS1024A_GPIO_OUTPUT_REG			0x00
#define LS1024A_GPIO_OE_REG			0x04
#define LS1024A_GPIO_INT_CFG_REG		0x08
#define LS1024A_GPIO_INPUT_REG			0x10
#define LS1024A_PIN_SELECT_15_0_REG		0x58
#define LS1024A_PIN_SELECT_31_16_REG		0x5C
#define LS1024A_MISC_PIN_SELECT			0x60
#define LS1024A_GPIO_63_32_PIN_OUTPUT		0xD0
#define LS1024A_GPIO_63_32_PIN_OUTPUT_EN	0xD4
#define LS1024A_GPIO_63_32_PIN_INPUT		0xD8
#define LS1024A_GPIO_63_32_PIN_SELECT		0xDC

#define LS1024A_GPIO_NUM_IRQS	8

struct ls1024a_pinctrl {
	struct device *dev;
	struct pinctrl_dev *pctrl;
	struct gpio_chip gc;
	struct irq_chip	*irqchip;
	struct irq_domain *irq_domain;

	void __iomem *reg;

	struct of_phandle_args parent_phandle_args;

	/*
	 * Used to lock ls1024a_pinctrl->data. Also, this is needed to keep
	 * shadowed and real data registers writes together.
	 */
	spinlock_t lock;

	/* Shadowed data register to clear/set bits safely. */
	u32 data[2];

	/* Shadowed direction registers to clear/set direction safely. */
	u32 dir[2];
};

static inline struct ls1024a_pinctrl *gc_to_ls1024a_pinctrl(struct gpio_chip *gc)
{
	return container_of(gc, struct ls1024a_pinctrl, gc);
}

#define LS1024A_PIN_GEM0_RXD0		1
#define LS1024A_PIN_GEM0_RXD1		2
#define LS1024A_PIN_GEM0_RXD2		3
#define LS1024A_PIN_GEM0_RXD3		4
#define LS1024A_PIN_GEM0_RX_CTL		5
#define LS1024A_PIN_GEM0_RXC		6
#define LS1024A_PIN_GEM0_TXD0		7
#define LS1024A_PIN_GEM0_TXD1		8
#define LS1024A_PIN_GEM0_TXD2		9
#define LS1024A_PIN_GEM0_TXD3		10
#define LS1024A_PIN_GEM0_TX_CTL		11
#define LS1024A_PIN_GEM0_TXC		12
#define LS1024A_PIN_GEM1_RXD0		13
#define LS1024A_PIN_GEM1_RXD1		14
#define LS1024A_PIN_GEM1_RXD2		15
#define LS1024A_PIN_GEM1_RXD3		16
#define LS1024A_PIN_GEM1_RX_CTL		17
#define LS1024A_PIN_GEM1_RXC		18
#define LS1024A_PIN_GEM1_TXD0		19
#define LS1024A_PIN_GEM1_TXD1		20
#define LS1024A_PIN_GEM1_TXD2		21
#define LS1024A_PIN_GEM1_TXD3		22
#define LS1024A_PIN_GEM1_TX_CTL		23
#define LS1024A_PIN_GEM1_TXC		24
#define LS1024A_PIN_GEM2_RXD0		25
#define LS1024A_PIN_GEM2_RXD1		26
#define LS1024A_PIN_GEM2_RXD2		27
#define LS1024A_PIN_GEM2_RXD3		28
#define LS1024A_PIN_GEM2_RX_CTL		29
#define LS1024A_PIN_GEM2_RXC		30
#define LS1024A_PIN_GEM2_TXD0		31
#define LS1024A_PIN_GEM2_TXD1		32
#define LS1024A_PIN_GEM2_TXD2		33
#define LS1024A_PIN_GEM2_TXD3		34
#define LS1024A_PIN_GEM2_TX_CTL		35
#define LS1024A_PIN_GEM2_TXC		36
#define LS1024A_PIN_GEM2_REFCLK		37
#define LS1024A_PIN_GPIO00		38
#define LS1024A_PIN_GPIO01		39
#define LS1024A_PIN_GPIO02		40
#define LS1024A_PIN_GPIO03		41
#define LS1024A_PIN_GPIO04		42
#define LS1024A_PIN_GPIO05		43
#define LS1024A_PIN_GPIO06		44
#define LS1024A_PIN_GPIO07		45
#define LS1024A_PIN_GPIO08		46
#define LS1024A_PIN_GPIO09		47
#define LS1024A_PIN_GPIO10		48
#define LS1024A_PIN_GPIO11		49
#define LS1024A_PIN_GPIO12		50
#define LS1024A_PIN_GPIO13		51
#define LS1024A_PIN_GPIO14		52
#define LS1024A_PIN_GPIO15		53
#define LS1024A_PIN_I2C_SCL		54
#define LS1024A_PIN_I2C_SDA		55
#define LS1024A_PIN_SPI_SS0_N		56
#define LS1024A_PIN_SPI_SS1_N		57
#define LS1024A_PIN_SPI_2_SS1_N		58
#define LS1024A_PIN_SPI_SS2_N		59
#define LS1024A_PIN_SPI_SS3_N		60
#define LS1024A_PIN_EXP_CS2_N		61
#define LS1024A_PIN_EXP_CS3_N		62
#define LS1024A_PIN_EXP_ALE		63
#define LS1024A_PIN_EXP_RDY		64
#define LS1024A_PIN_TM_EXT_RESET	65
#define LS1024A_PIN_EXP_NAND_CS		66
#define LS1024A_PIN_EXP_NAND_RDY	67
#define LS1024A_PIN_SPI_TXD		68
#define LS1024A_PIN_SPI_SCLK		69
#define LS1024A_PIN_SPI_RXD		70
#define LS1024A_PIN_SPI_2_RXD		71
#define LS1024A_PIN_SPI_2_SS0_N		72
#define LS1024A_PIN_EXP_DQ08		73
#define LS1024A_PIN_EXP_DQ09		74
#define LS1024A_PIN_EXP_DQ10		75
#define LS1024A_PIN_EXP_DQ11		76
#define LS1024A_PIN_EXP_DQ12		77
#define LS1024A_PIN_EXP_DQ13		78
#define LS1024A_PIN_EXP_DQ14		79
#define LS1024A_PIN_EXP_DQ15		80
#define LS1024A_PIN_EXP_DM1		81
#define LS1024A_PIN_CORESIGHT_D0	82
#define LS1024A_PIN_CORESIGHT_D1	83
#define LS1024A_PIN_CORESIGHT_D2	84
#define LS1024A_PIN_CORESIGHT_D3	85
#define LS1024A_PIN_CORESIGHT_D4	86
#define LS1024A_PIN_CORESIGHT_D5	87
#define LS1024A_PIN_CORESIGHT_D6	88
#define LS1024A_PIN_CORESIGHT_D7	89
#define LS1024A_PIN_CORESIGHT_D8	90
#define LS1024A_PIN_CORESIGHT_D9	91
#define LS1024A_PIN_CORESIGHT_D10	92
#define LS1024A_PIN_CORESIGHT_D11	93
#define LS1024A_PIN_CORESIGHT_D12	94
#define LS1024A_PIN_CORESIGHT_D13	95
#define LS1024A_PIN_CORESIGHT_D14	96
#define LS1024A_PIN_CORESIGHT_D15	97
#define LS1024A_PIN_UART1_RX		98
#define LS1024A_PIN_UART1_TX		99
#define LS1024A_PIN_TDM_CK		100
#define LS1024A_PIN_TDM_FS		101
#define LS1024A_PIN_TDM_DR		102
#define LS1024A_PIN_TDM_DX		103



static const struct pinctrl_pin_desc ls1024a_pins_desc[] = {
	PINCTRL_PIN(LS1024A_PIN_GEM0_RXD0, "gem0_rxd0"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_RXD1, "gem0_rxd1"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_RXD2, "gem0_rxd2"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_RXD3, "gem0_rxd3"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_RX_CTL, "gem0_rx_ctl"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_RXC, "gem0_rxc"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_TXD0, "gem0_txd0"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_TXD1, "gem0_txd1"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_TXD2, "gem0_txd2"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_TXD3, "gem0_txd3"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_TX_CTL, "gem0_tx_ctl"),
	PINCTRL_PIN(LS1024A_PIN_GEM0_TXC, "gem0_txc"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_RXD0, "gem1_rxd0"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_RXD1, "gem1_rxd1"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_RXD2, "gem1_rxd2"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_RXD3, "gem1_rxd3"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_RX_CTL, "gem1_rx_ctl"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_RXC, "gem1_rxc"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_TXD0, "gem1_txd0"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_TXD1, "gem1_txd1"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_TXD2, "gem1_txd2"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_TXD3, "gem1_txd3"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_TX_CTL, "gem1_tx_ctl"),
	PINCTRL_PIN(LS1024A_PIN_GEM1_TXC, "gem1_txc"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_RXD0, "gem2_rxd0"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_RXD1, "gem2_rxd1"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_RXD2, "gem2_rxd2"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_RXD3, "gem2_rxd3"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_RX_CTL, "gem2_rx_ctl"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_RXC, "gem2_rxc"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_TXD0, "gem2_txd0"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_TXD1, "gem2_txd1"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_TXD2, "gem2_txd2"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_TXD3, "gem2_txd3"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_TX_CTL, "gem2_tx_ctl"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_TXC, "gem2_txc"),
	PINCTRL_PIN(LS1024A_PIN_GEM2_REFCLK, "gem2_refclk"),
	PINCTRL_PIN(LS1024A_PIN_GPIO00, "gpio00"),
	PINCTRL_PIN(LS1024A_PIN_GPIO01, "gpio01"),
	PINCTRL_PIN(LS1024A_PIN_GPIO02, "gpio02"),
	PINCTRL_PIN(LS1024A_PIN_GPIO03, "gpio03"),
	PINCTRL_PIN(LS1024A_PIN_GPIO04, "gpio04"),
	PINCTRL_PIN(LS1024A_PIN_GPIO05, "gpio05"),
	PINCTRL_PIN(LS1024A_PIN_GPIO06, "gpio06"),
	PINCTRL_PIN(LS1024A_PIN_GPIO07, "gpio07"),
	PINCTRL_PIN(LS1024A_PIN_GPIO08, "gpio08"),
	PINCTRL_PIN(LS1024A_PIN_GPIO09, "gpio09"),
	PINCTRL_PIN(LS1024A_PIN_GPIO10, "gpio10"),
	PINCTRL_PIN(LS1024A_PIN_GPIO11, "gpio11"),
	PINCTRL_PIN(LS1024A_PIN_GPIO12, "gpio12"),
	PINCTRL_PIN(LS1024A_PIN_GPIO13, "gpio13"),
	PINCTRL_PIN(LS1024A_PIN_GPIO14, "gpio14"),
	PINCTRL_PIN(LS1024A_PIN_GPIO15, "gpio15"),
	PINCTRL_PIN(LS1024A_PIN_I2C_SCL, "i2c_scl"), /* GPIO 16 */
	PINCTRL_PIN(LS1024A_PIN_I2C_SDA, "i2c_sda"), /* GPIO 17 */
	PINCTRL_PIN(LS1024A_PIN_SPI_SS0_N, "spi_ss0_n"), /* GPIO 18 */
	PINCTRL_PIN(LS1024A_PIN_SPI_SS1_N, "spi_ss1_n"), /* GPIO 19 */
	PINCTRL_PIN(LS1024A_PIN_SPI_2_SS1_N, "spi_2_ss1_n"), /* GPIO 20 */
	PINCTRL_PIN(LS1024A_PIN_SPI_SS2_N, "spi_ss2_n"), /* GPIO 21 */
	PINCTRL_PIN(LS1024A_PIN_SPI_SS3_N, "spi_ss3_n"), /* GPIO 22 */
	PINCTRL_PIN(LS1024A_PIN_EXP_CS2_N, "exp_cs2_n"), /* GPIO 23 */
	PINCTRL_PIN(LS1024A_PIN_EXP_CS3_N, "exp_cs3_n"), /* GPIO 24 */
	PINCTRL_PIN(LS1024A_PIN_EXP_ALE, "exp_ale"), /* GPIO 25 */
	PINCTRL_PIN(LS1024A_PIN_EXP_RDY, "exp_rdy"), /* GPIO 26 */
	PINCTRL_PIN(LS1024A_PIN_TM_EXT_RESET, "tm_ext_reset"), /* GPIO 27 */
	PINCTRL_PIN(LS1024A_PIN_EXP_NAND_CS, "exp_nand_cs"), /* GPIO 28 */
	PINCTRL_PIN(LS1024A_PIN_EXP_NAND_RDY, "exp_nand_rdy"), /* GPIO 29 */
	PINCTRL_PIN(LS1024A_PIN_SPI_TXD, "spi_txd"), /* GPIO 30 */
	PINCTRL_PIN(LS1024A_PIN_SPI_SCLK, "spi_sclk"), /* GPIO 31 */
	PINCTRL_PIN(LS1024A_PIN_SPI_RXD, "spi_rxd"), /* GPIO 32 */
	PINCTRL_PIN(LS1024A_PIN_SPI_2_RXD, "spi_2_rxd"), /* GPIO 33 */
	PINCTRL_PIN(LS1024A_PIN_SPI_2_SS0_N, "spi_2_ss0_n"), /* GPIO 34 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DQ08, "exp_dq08"), /* GPIO 35 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DQ09, "exp_dq09"), /* GPIO 36 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DQ10, "exp_dq10"), /* GPIO 37 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DQ11, "exp_dq11"), /* GPIO 38 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DQ12, "exp_dq12"), /* GPIO 39 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DQ13, "exp_dq13"), /* GPIO 40 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DQ14, "exp_dq14"), /* GPIO 41 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DQ15, "exp_dq15"), /* GPIO 42 */
	PINCTRL_PIN(LS1024A_PIN_EXP_DM1, "exp_dm1"), /* GPIO 43 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D0, "coresight_d0"), /* GPIO 44 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D1, "coresight_d1"), /* GPIO 45 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D2, "coresight_d2"), /* GPIO 46 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D3, "coresight_d3"), /* GPIO 47 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D4, "coresight_d4"), /* GPIO 48 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D5, "coresight_d5"), /* GPIO 49 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D6, "coresight_d6"), /* GPIO 50 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D7, "coresight_d7"), /* GPIO 51 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D8, "coresight_d8"), /* GPIO 52 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D9, "coresight_d9"), /* GPIO 53 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D10, "coresight_d10"), /* GPIO 54 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D11, "coresight_d11"), /* GPIO 55 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D12, "coresight_d12"), /* GPIO 56 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D13, "coresight_d13"), /* GPIO 57 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D14, "coresight_d14"), /* GPIO 58 */
	PINCTRL_PIN(LS1024A_PIN_CORESIGHT_D15, "coresight_d15"), /* GPIO 59 */
	PINCTRL_PIN(LS1024A_PIN_UART1_RX, "uart1_rx"), /* not usable as GPIO */
	PINCTRL_PIN(LS1024A_PIN_UART1_TX, "uart1_tx"), /* not usable as GPIO */
	/* Warning: Following four GPIOs are in reverse order */
	PINCTRL_PIN(LS1024A_PIN_TDM_CK, "tdm_ck"), /* GPIO 63 */
	PINCTRL_PIN(LS1024A_PIN_TDM_FS, "tdm_fs"), /* GPIO 62 */
	PINCTRL_PIN(LS1024A_PIN_TDM_DR, "tdm_dr"), /* GPIO 61 */
	PINCTRL_PIN(LS1024A_PIN_TDM_DX, "tdm_dx"), /* GPIO 60 */
};

struct ls1024a_group {
	const char *name;
	const unsigned int *pins;
	const unsigned num_pins;
	const unsigned reg;
	const unsigned shift;
	const unsigned bits;
};

static const unsigned pwm0_pins[] = { LS1024A_PIN_GPIO04 };
static const unsigned pwm1_pins[] = { LS1024A_PIN_GPIO05 };
static const unsigned pwm2_pins[] = { LS1024A_PIN_GPIO06 };
static const unsigned pwm3_pins[] = { LS1024A_PIN_GPIO07 };
static const unsigned pwm4_pins[] = { LS1024A_PIN_GPIO12 };
static const unsigned pwm5_pins[] = { LS1024A_PIN_GPIO13 };
static const unsigned i2c_pins[] = { LS1024A_PIN_I2C_SCL, LS1024A_PIN_I2C_SDA };
static const unsigned uart0_pins[] = { LS1024A_PIN_GPIO08, LS1024A_PIN_GPIO09, LS1024A_PIN_GPIO10, LS1024A_PIN_GPIO11 };
static const unsigned uart1_pins[] = { LS1024A_PIN_UART1_RX, LS1024A_PIN_UART1_TX };
static const unsigned spi_2_pins[] = { LS1024A_PIN_SPI_2_RXD }; /* SPI_2_TXD and SPI_2_SCLK are not muxed */
static const unsigned spi_2_ss0_pins[] = { LS1024A_PIN_SPI_2_SS0_N };
static const unsigned spi_2_ss1_pins[] = { LS1024A_PIN_SPI_2_SS1_N };
static const unsigned gpio_pins[] = {
	LS1024A_PIN_GPIO00, LS1024A_PIN_GPIO01, LS1024A_PIN_GPIO02,
	LS1024A_PIN_GPIO03, LS1024A_PIN_GPIO04, LS1024A_PIN_GPIO05,
	LS1024A_PIN_GPIO06, LS1024A_PIN_GPIO07, LS1024A_PIN_GPIO08,
	LS1024A_PIN_GPIO09, LS1024A_PIN_GPIO10, LS1024A_PIN_GPIO11,
	LS1024A_PIN_GPIO12, LS1024A_PIN_GPIO13, LS1024A_PIN_GPIO14,
	LS1024A_PIN_GPIO15, LS1024A_PIN_I2C_SCL, LS1024A_PIN_I2C_SDA,
	LS1024A_PIN_SPI_SS0_N, LS1024A_PIN_SPI_SS1_N, LS1024A_PIN_SPI_2_SS1_N,
	LS1024A_PIN_SPI_SS2_N, LS1024A_PIN_SPI_SS3_N, LS1024A_PIN_EXP_CS2_N,
	LS1024A_PIN_EXP_CS3_N, LS1024A_PIN_EXP_ALE, LS1024A_PIN_EXP_RDY,
	LS1024A_PIN_TM_EXT_RESET, LS1024A_PIN_EXP_NAND_CS,
	LS1024A_PIN_EXP_NAND_RDY, LS1024A_PIN_SPI_TXD, LS1024A_PIN_SPI_SCLK,
	LS1024A_PIN_SPI_RXD, LS1024A_PIN_SPI_2_RXD, LS1024A_PIN_SPI_2_SS0_N,
	LS1024A_PIN_EXP_DQ08, LS1024A_PIN_EXP_DQ09, LS1024A_PIN_EXP_DQ10,
	LS1024A_PIN_EXP_DQ11, LS1024A_PIN_EXP_DQ12, LS1024A_PIN_EXP_DQ13,
	LS1024A_PIN_EXP_DQ14, LS1024A_PIN_EXP_DQ15, LS1024A_PIN_EXP_DM1,
	LS1024A_PIN_CORESIGHT_D0, LS1024A_PIN_CORESIGHT_D1,
	LS1024A_PIN_CORESIGHT_D2, LS1024A_PIN_CORESIGHT_D3,
	LS1024A_PIN_CORESIGHT_D4, LS1024A_PIN_CORESIGHT_D5,
	LS1024A_PIN_CORESIGHT_D6, LS1024A_PIN_CORESIGHT_D7,
	LS1024A_PIN_CORESIGHT_D8, LS1024A_PIN_CORESIGHT_D9,
	LS1024A_PIN_CORESIGHT_D10, LS1024A_PIN_CORESIGHT_D11,
	LS1024A_PIN_CORESIGHT_D12, LS1024A_PIN_CORESIGHT_D13,
	LS1024A_PIN_CORESIGHT_D14, LS1024A_PIN_CORESIGHT_D15,
	/* The last four GPIOs are in a non-intuitive order, i.e. they are in
	 * reverse order compared to how the pins are defined. */
	LS1024A_PIN_TDM_DX, LS1024A_PIN_TDM_DR, LS1024A_PIN_TDM_FS, LS1024A_PIN_TDM_CK,
};

static const struct ls1024a_group ls1024a_groups[] = {
	{
		.name = "gpio",
		.pins = gpio_pins,
		.num_pins = ARRAY_SIZE(gpio_pins),
	},
	{
		.name = "pwm0",
		.pins = pwm0_pins,
		.num_pins = ARRAY_SIZE(pwm0_pins),
		.reg = LS1024A_PIN_SELECT_15_0_REG,
		.shift = 8,
		.bits = 2,
	},
	{
		.name = "pwm1",
		.pins = pwm1_pins,
		.num_pins = ARRAY_SIZE(pwm1_pins),
		.reg = LS1024A_PIN_SELECT_15_0_REG,
		.shift = 10,
		.bits = 2,
	},
	{
		.name = "pwm2",
		.pins = pwm2_pins,
		.num_pins = ARRAY_SIZE(pwm2_pins),
		.reg = LS1024A_PIN_SELECT_15_0_REG,
		.shift = 12,
		.bits = 2,
	},
	{
		.name = "pwm3",
		.pins = pwm3_pins,
		.num_pins = ARRAY_SIZE(pwm3_pins),
		.reg = LS1024A_PIN_SELECT_15_0_REG,
		.shift = 14,
		.bits = 2,
	},
	{
		.name = "pwm4",
		.pins = pwm4_pins,
		.num_pins = ARRAY_SIZE(pwm4_pins),
		.reg = LS1024A_PIN_SELECT_15_0_REG,
		.shift = 24,
		.bits = 2,
	},
	{
		.name = "pwm5",
		.pins = pwm5_pins,
		.num_pins = ARRAY_SIZE(pwm5_pins),
		.reg = LS1024A_PIN_SELECT_15_0_REG,
		.shift = 26,
		.bits = 2,
	},
	{
		.name = "i2c",
		.pins = i2c_pins,
		.num_pins = ARRAY_SIZE(i2c_pins),
		.reg = LS1024A_PIN_SELECT_31_16_REG,
		.shift = 0,
		.bits = 4,
	},
	{
		.name = "uart0",
		.pins = uart0_pins,
		.num_pins = ARRAY_SIZE(uart0_pins),
	},
	{
		.name = "uart1",
		.pins = uart1_pins,
		.num_pins = ARRAY_SIZE(uart1_pins),
	},
	{
		.name = "spi_2",
		.pins = spi_2_pins,
		.num_pins = ARRAY_SIZE(spi_2_pins),
		.reg = LS1024A_GPIO_63_32_PIN_SELECT,
		.shift = 1,
		.bits = 1,
	},
	{
		.name = "spi_2_ss0",
		.pins = spi_2_ss0_pins,
		.num_pins = ARRAY_SIZE(spi_2_ss0_pins),
		.reg = LS1024A_GPIO_63_32_PIN_SELECT,
		.shift = 2,
		.bits = 1,
	},
	{
		.name = "spi_2_ss1",
		.pins = spi_2_ss1_pins,
		.num_pins = ARRAY_SIZE(spi_2_ss1_pins),
		.reg = LS1024A_PIN_SELECT_31_16_REG,
		.shift = 8, /* TODO */
		.bits = 2,
	},
};

static const char * const pwm_groups[] = { "pwm0", "pwm1", "pwm2", "pwm3", "pwm4", "pwm5" };
static const char * const i2c_groups[] = { "i2c" };
static const char * const uart0_groups[] = { "uart0" };
static const char * const uart1_groups[] = { "uart1" };
static const char * const spi_2_groups[] = { "spi_2", "spi_2_ss0", "spi_2_ss1" };

struct ls1024a_pmx_func {
	const char *name;
	const char * const *groups;
	const unsigned num_groups;
	const unsigned muxsetting;
};

static const struct ls1024a_pmx_func ls1024a_functions[] = {
	{
		.name = "pwm",
		.groups = pwm_groups,
		.num_groups = ARRAY_SIZE(pwm_groups),
		.muxsetting = 1,
	},
	{
		.name = "i2c",
		.groups = i2c_groups,
		.num_groups = ARRAY_SIZE(i2c_groups),
		.muxsetting = 0,
	},
	{
		.name = "uart0",
		.groups = uart0_groups,
		.num_groups = ARRAY_SIZE(uart0_groups),
		.muxsetting = 2,
	},
	{
		.name = "uart1",
		.groups = uart1_groups,
		.num_groups = ARRAY_SIZE(uart1_groups),
		.muxsetting = 0,
	},
	{
		.name = "spi_2",
		.groups = spi_2_groups,
		.num_groups = ARRAY_SIZE(spi_2_groups),
		.muxsetting = 0,
	},
};

static int ls1024a_get_groups_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(ls1024a_groups);
}

static const char *ls1024a_get_group_name(struct pinctrl_dev *pctldev,
				      unsigned group)
{
	return ls1024a_groups[group].name;
}

static int ls1024a_get_group_pins(struct pinctrl_dev *pctldev,
			      unsigned group,
			      const unsigned **pins,
			      unsigned *num_pins)
{
	*pins = ls1024a_groups[group].pins;
	*num_pins = ls1024a_groups[group].num_pins;
	return 0;
}

static int ls1024a_get_functions_count(struct pinctrl_dev *pctldev)
{
	return ARRAY_SIZE(ls1024a_functions);
}

static const char *ls1024a_get_function_name(struct pinctrl_dev *pctldev,
					 unsigned function)
{
	return ls1024a_functions[function].name;
}

static int ls1024a_get_function_groups(struct pinctrl_dev *pctldev,
				   unsigned function,
				   const char * const **groups,
				   unsigned * const num_groups)
{
	*groups = ls1024a_functions[function].groups;
	*num_groups = ls1024a_functions[function].num_groups;
	return 0;
}

static int ls1024a_pinmux_set_mux(struct pinctrl_dev *pctldev,
			      unsigned function,
			      unsigned group)
{
	struct ls1024a_pinctrl *lpc = pinctrl_dev_get_drvdata(pctldev);
	const struct ls1024a_group *g;
	const struct ls1024a_pmx_func *f;
	unsigned long flags;
	u32 val;
	u32 mask;

	g = &ls1024a_groups[group];
	f = &ls1024a_functions[function];

	spin_lock_irqsave(&lpc->lock, flags);

	val = readl(lpc->reg + g->reg);
	mask = GENMASK(g->shift + g->bits - 1, g->shift);
	val &= ~mask;
	val |= f->muxsetting << g->shift;
	writel(val, lpc->reg + g->reg);

	spin_unlock_irqrestore(&lpc->lock, flags);

	return 0;
}

static void _ls1024a_gpio_set_direction(struct ls1024a_pinctrl *lpc, unsigned int gpio, int out);

static int ls1024a_gpio_request_enable (struct pinctrl_dev *pctldev,
				struct pinctrl_gpio_range *range,
				unsigned offset)
{
	struct ls1024a_pinctrl *lpc = pinctrl_dev_get_drvdata(pctldev);
	unsigned long flags;
	u32 val;
	u32 newval;
	int i;

	spin_lock_irqsave(&lpc->lock, flags);
	if (offset >= LS1024A_PIN_TDM_CK && offset <= LS1024A_PIN_TDM_DX) {
		/* GPIO 60 to GPIO 63 */
		val = readl(lpc->reg + LS1024A_MISC_PIN_SELECT);
		newval = (val & ~GENMASK(5,4)) | 2<<4;
		if (newval != val) {
			for (i=60;i<=63;i++) {
				_ls1024a_gpio_set_direction(lpc, i, 0);
			}
			writel(newval, lpc->reg + LS1024A_MISC_PIN_SELECT);
		}
	} else if (offset >= LS1024A_PIN_SPI_RXD && offset <= LS1024A_PIN_CORESIGHT_D15) {
		/* GPIO 32 to GPIO 59 */
		i = offset - LS1024A_PIN_SPI_RXD;
		val = readl(lpc->reg + LS1024A_GPIO_63_32_PIN_SELECT);
		newval = val | BIT(i);
		if (newval != val) {
			_ls1024a_gpio_set_direction(lpc, i, 0);
			writel(newval, lpc->reg + LS1024A_GPIO_63_32_PIN_SELECT);
		}
	} else if ( (offset >= LS1024A_PIN_I2C_SCL && offset <= LS1024A_PIN_EXP_RDY) ||
			(offset >= LS1024A_PIN_SPI_TXD && offset <= LS1024A_PIN_SPI_SCLK) ) {
		/* GPIO 16 to GPIO 26  and GPIO 30 to GPIO 31
		 * From the datasheet:
		 * "GPIO[27] is used as TM_EXT_RESET and is not muxed
		 *  GPIO[28] is used as EXP_NAND_CS and is not muxed
		 *  GPIO[29] is used as EXP_NAND_RDY and is not muxed"
		 * */
		i = offset - LS1024A_PIN_I2C_SCL;
		val = readl(lpc->reg + LS1024A_PIN_SELECT_31_16_REG);
		newval = (val & ~GENMASK(i*2 + 1, i*2)) | (1<<i*2);
		if (newval != val) {
			_ls1024a_gpio_set_direction(lpc, i, 0);
			writel(newval, lpc->reg + LS1024A_PIN_SELECT_31_16_REG);
		}
	} else if (offset >= LS1024A_PIN_GPIO04 && offset <= LS1024A_PIN_GPIO15) {
		/* GPIO 4 to GPIO 15 */
		i = offset - LS1024A_PIN_GPIO00;
		val = readl(lpc->reg + LS1024A_PIN_SELECT_15_0_REG);
		newval = val & ~GENMASK(i*2 + 1, i*2);
		if (newval != val) {
			_ls1024a_gpio_set_direction(lpc, i, 0);
			writel(newval, lpc->reg + LS1024A_PIN_SELECT_15_0_REG);
		}
	} else if (offset >= LS1024A_PIN_GPIO00 && offset <= LS1024A_PIN_GPIO03) {
		/* No work change needed. GPIOs 0 through 3 are always selected. */
	}

	spin_unlock_irqrestore(&lpc->lock, flags);
	return 0;
}

static const struct pinmux_ops ls1024a_pinmux_ops = {
	.get_functions_count	= ls1024a_get_functions_count,
	.get_function_name	= ls1024a_get_function_name,
	.get_function_groups	= ls1024a_get_function_groups,
	.set_mux		= ls1024a_pinmux_set_mux,
	.gpio_request_enable	= ls1024a_gpio_request_enable,
};

static const struct pinctrl_ops ls1024a_pinctrl_ops = {
	.get_groups_count	= ls1024a_get_groups_count,
	.get_group_name		= ls1024a_get_group_name,
	.get_group_pins		= ls1024a_get_group_pins,
	.dt_node_to_map		= pinconf_generic_dt_node_to_map_group,
	.dt_free_map		= pinctrl_utils_dt_free_map,
};

static struct pinctrl_desc ls1024a_pinctrl_desc = {
	.pins = ls1024a_pins_desc,
	.npins = ARRAY_SIZE(ls1024a_pins_desc),
	.pctlops = &ls1024a_pinctrl_ops,
	.pmxops = &ls1024a_pinmux_ops,
	.owner = THIS_MODULE,
};



static int ls1024a_gpio_get(struct gpio_chip *gc, unsigned pin)
{
	u32 val;
	struct ls1024a_pinctrl *lpc = gc_to_ls1024a_pinctrl(gc);

	if (pin >= gc->ngpio)
		return -EINVAL;

	if (pin < 32) {
		val = readl(lpc->reg + LS1024A_GPIO_INPUT_REG);
	} else {
		val = readl(lpc->reg + LS1024A_GPIO_63_32_PIN_INPUT);
	}

	if (val & BIT_MASK(pin))
		return 1;

	return 0;
}

static void ls1024a_gpio_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct ls1024a_pinctrl *lpc = gc_to_ls1024a_pinctrl(gc);
	unsigned long flags;

	if (gpio >= gc->ngpio)
		return;

	spin_lock_irqsave(&lpc->lock, flags);

	if (val)
		lpc->data[BIT_WORD(gpio)] |= BIT_MASK(gpio);
	else
		lpc->data[BIT_WORD(gpio)] &= ~BIT_MASK(gpio);

	writel(lpc->data[0], lpc->reg + LS1024A_GPIO_OUTPUT_REG);
	writel(lpc->data[1], lpc->reg + LS1024A_GPIO_63_32_PIN_OUTPUT);

	spin_unlock_irqrestore(&lpc->lock, flags);
}

static void _ls1024a_gpio_set_direction(struct ls1024a_pinctrl *lpc, unsigned int gpio, int out) {
	if (out)
		lpc->dir[BIT_WORD(gpio)] |= BIT_MASK(gpio);
	else
		lpc->dir[BIT_WORD(gpio)] &= ~BIT_MASK(gpio);

	writel(lpc->dir[0], lpc->reg + LS1024A_GPIO_OE_REG);
	/* LS1024A_GPIO_63_32_PIN_OUTPUT_EN uses inverse logic */
	writel(~lpc->dir[1], lpc->reg + LS1024A_GPIO_63_32_PIN_OUTPUT_EN);
}

static int ls1024a_gpio_set_direction(struct gpio_chip *gc, unsigned int gpio, int out) {
	struct ls1024a_pinctrl *lpc = gc_to_ls1024a_pinctrl(gc);
	unsigned long flags;

	if (gpio >= gc->ngpio)
		return -EINVAL;

	spin_lock_irqsave(&lpc->lock, flags);

	_ls1024a_gpio_set_direction(lpc, gpio, out);

	spin_unlock_irqrestore(&lpc->lock, flags);

	return 0;
}

static int ls1024a_dir_in(struct gpio_chip *gc, unsigned int gpio)
{
	return ls1024a_gpio_set_direction(gc, gpio, 0);
}

static int ls1024a_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	ls1024a_gpio_set(gc, gpio, val);
	return ls1024a_gpio_set_direction(gc, gpio, 1);
}

static int ls1024a_gpio_request(struct gpio_chip *chip, unsigned gpio_pin)
{
	if (gpio_pin < chip->ngpio)
		return pinctrl_request_gpio(chip->base + gpio_pin);

	return -EINVAL;
}

static void ls1024a_gpio_free(struct gpio_chip *chip, unsigned gpio_pin)
{
	pinctrl_free_gpio(chip->base + gpio_pin);
}

static int ls1024a_set_type(struct irq_data *data, unsigned int type)
{
	struct ls1024a_pinctrl *lpc = irq_data_get_irq_chip_data(data);
	int ls1024a_type;
	int ret;
	unsigned long flags;
	u32 value;
	switch(type) {
		case IRQ_TYPE_EDGE_RISING:
			ls1024a_type = 0x2;
			break;
		case IRQ_TYPE_EDGE_FALLING:
			ls1024a_type = 0x1;
			break;
		case IRQ_TYPE_LEVEL_HIGH:
			ls1024a_type = 0x3;
			break;
		default:
			WARN_ON(1);
			/* Fall through */
		case IRQ_TYPE_NONE:
			ls1024a_type = 0x0;
			break;
	}
	spin_lock_irqsave(&lpc->lock, flags);
	value = readl(lpc->reg + LS1024A_GPIO_INT_CFG_REG);
	value &= ~(0x3 << data->hwirq*2);
	value |= ls1024a_type << data->hwirq*2;
	writel(value, lpc->reg + LS1024A_GPIO_INT_CFG_REG);
	spin_unlock_irqrestore(&lpc->lock, flags);

	data = data->parent_data;
	ret = data->chip->irq_set_type(data, type);
	return ret;
}

static struct irq_chip ls1024a_gpio_irq_chip = {
	.name			= "GPIO",
	.irq_mask		= irq_chip_mask_parent,
	.irq_unmask		= irq_chip_unmask_parent,
	.irq_eoi		= irq_chip_eoi_parent,
	.irq_set_type		= ls1024a_set_type,
	.irq_retrigger		= irq_chip_retrigger_hierarchy,
	.irq_set_affinity	= irq_chip_set_affinity_parent,
};

static int ls1024a_domain_alloc(struct irq_domain *domain, unsigned int virq,
				   unsigned int nr_irqs, void *arg)
{
	int i, ret;
	irq_hw_number_t hwirq;
	unsigned int type;
	struct of_phandle_args *irq_data = arg;
	struct of_phandle_args gic_data = *irq_data;
	struct ls1024a_pinctrl *lpc = domain->host_data;

	if (irq_data->args_count != 2)
		return -EINVAL;

	ret = domain->ops->xlate(domain, irq_data->np, irq_data->args,
			irq_data->args_count, &hwirq, &type);
	if (ret)
		return ret;
	for (i = 0; i < nr_irqs; i++)
		irq_domain_set_hwirq_and_chip(domain, virq + i, hwirq + i,
					      &ls1024a_gpio_irq_chip,
					      domain->host_data);
	gic_data = lpc->parent_phandle_args;
	gic_data.args[1] += hwirq;
	gic_data.args[2] = type;
	return irq_domain_alloc_irqs_parent(domain, virq, nr_irqs, &gic_data);
}

static struct irq_domain_ops ls1024a_domain_ops = {
	.xlate = irq_domain_xlate_twocell,
	.alloc = ls1024a_domain_alloc,
	.free = irq_domain_free_irqs_common,
};


static int ls1024a_pinctrl_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node = dev->of_node;
	struct resource *res;
	struct ls1024a_pinctrl *lpc;
	struct irq_domain *domain_parent;
	unsigned long flags;
	int rc;

	lpc = devm_kzalloc(&pdev->dev, sizeof(*lpc), GFP_KERNEL);
	if (!lpc)
		return -ENOMEM;
	lpc->dev = &pdev->dev;

	lpc->gc.dev = dev;
#ifdef CONFIG_OF_GPIO
	lpc->gc.of_node = of_node_get(node);
#endif
	spin_lock_init(&lpc->lock);
	lpc->gc.dev = dev;
	lpc->gc.label = dev_name(dev);
	lpc->gc.base = 0;
	lpc->gc.ngpio = 64;
	lpc->gc.request = ls1024a_gpio_request;
	lpc->gc.free = ls1024a_gpio_free;

	lpc->gc.direction_input = ls1024a_dir_in;
	lpc->gc.direction_output = ls1024a_dir_out;
	lpc->gc.set = ls1024a_gpio_set;
	lpc->gc.get = ls1024a_gpio_get;
	lpc->gc.can_sleep = false;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	lpc->reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(lpc->reg)) {
		rc = PTR_ERR(lpc->reg);
		goto err;
	}

	platform_set_drvdata(pdev, lpc);

	ls1024a_pinctrl_desc.name = dev_name(&pdev->dev);
	lpc->pctrl = pinctrl_register(&ls1024a_pinctrl_desc, &pdev->dev, lpc);
	if (!lpc->pctrl) {
		dev_err(&pdev->dev, "Couldn't register pinctrl driver\n");
		rc = -ENODEV;
		goto err_iounmap;
	}

	rc = of_irq_parse_one(node, 0, &lpc->parent_phandle_args);
	if (rc) {
		goto err_unregister_pinctrl;
	}
	domain_parent = irq_find_host(lpc->parent_phandle_args.np);
	if (!domain_parent) {
		dev_err(dev, "cannot find irq parent domain\n");
		rc = -EPROBE_DEFER;
		goto err_unregister_pinctrl;
	}
	lpc->irq_domain = irq_domain_add_hierarchy(domain_parent, 0,
			LS1024A_GPIO_NUM_IRQS, node,
			&ls1024a_domain_ops, lpc);
	if (!lpc->irq_domain) {
		rc = -ENOMEM;
		goto err_unregister_pinctrl;
	}

	spin_lock_irqsave(&lpc->lock, flags);
	lpc->dir[0] = readl(lpc->reg + LS1024A_GPIO_OE_REG);
	/* LS1024A_GPIO_63_32_PIN_OUTPUT_EN uses inverse logic */
	lpc->dir[1] = ~readl(lpc->reg + LS1024A_GPIO_63_32_PIN_OUTPUT_EN);
	lpc->data[0] = readl(lpc->reg + LS1024A_GPIO_OUTPUT_REG);
	lpc->data[1] = readl(lpc->reg + LS1024A_GPIO_63_32_PIN_OUTPUT);
	spin_unlock_irqrestore(&lpc->lock, flags);

	lpc->gc.owner = THIS_MODULE;

	rc = gpiochip_add(&lpc->gc);
	if (rc)
		goto err_remove_domain;
	return 0;

err_remove_domain:
	irq_domain_remove(lpc->irq_domain);
err_unregister_pinctrl:
	pinctrl_unregister(lpc->pctrl);
err_iounmap:
	devm_iounmap(dev, lpc->reg);
err:
	platform_set_drvdata(pdev, NULL);
	devm_kfree(dev, lpc);
	return rc;
}

static const struct of_device_id ls1024a_pinctrl_of_match[] = {
	{ .compatible = "fsl,ls1024a-pinctrl", },
	{ },
};

static struct platform_driver ls1024a_pinctrl_driver = {
	.driver = {
		.name = "ls1024a-pinctrl",
		.of_match_table = ls1024a_pinctrl_of_match,
	},
	.probe = ls1024a_pinctrl_probe,
};

module_platform_driver(ls1024a_pinctrl_driver);

MODULE_AUTHOR("Daniel Mentz <danielmentz@google.com>");
MODULE_DESCRIPTION("Freescale QorIQ LS1024A pinctrl driver");
MODULE_LICENSE("GPL v2");
MODULE_DEVICE_TABLE(of, ls1024a_pinctrl_of_match);

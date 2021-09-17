#include <linux/platform_device.h>

#define A17_AXI_SCALE_REG   IO_ADDRESS(0x12040000 + 0x30)
#define REG_PERI_CRG57     IO_ADDRESS(0x12040000 + 0xe4)

#define get_bus_clk()({\
	unsigned long tmp_reg, busclk;\
	tmp_reg = readl((volatile void *)A17_AXI_SCALE_REG);\
	tmp_reg = tmp_reg & 0x3;\
	if (tmp_reg  == 0x1)\
		busclk = 300000000;\
	else if (tmp_reg == 0x2)\
		busclk = 250000000;\
	else if (tmp_reg == 0x3)\
		busclk = 200000000;\
	else\
		busclk = 200000000;\
	busclk;\
})

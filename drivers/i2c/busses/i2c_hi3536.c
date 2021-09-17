#include "i2c_hi3536.h"

unsigned int get_apb_clk(void)
{
	unsigned int apb_clk;

	apb_clk = get_bus_clk() / 2;

	return apb_clk;
}

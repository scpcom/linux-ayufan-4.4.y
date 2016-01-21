#include <linux/clk-provider.h>
#include <linux/slab.h>
#include <linux/of_address.h>
#include <mach/hardware.h>
#include "clock.h"

struct clk_hw_comcerto2000 {
	struct clk_hw hw;
	void __iomem *reg;
	u32 divreg_offset;
	int div_bypass;
};

/*
 * CLKGEN Divider registers
 * The divider bypass bit in several configuration registers
 * can only be written (if you read back you get zero).
 *
 * The Bug is in registers: 0x84 (A9DP_CLKDIV_CNTRL), 0x84 + 16 (L2CC_CLKDIV_CNTRL), 0x84 + 32
 *	(TPI_CLKDIV_CNTRL), etc... until PLL_ADDR_SPACE at 0x1C0.
 */

/*
 * Barebox uses IRAM to mirror the clock divider registers
 * Linux will relocate this mirror from IRAM to DDR to free up IRAM.
 */
#define IRAM_CLK_REG_MIRROR		(0x8300FC00 - COMCERTO_AXI_IRAM_BASE + IRAM_MEMORY_VADDR)
#define CLK_REG_DIV_BUG_BASE		AXI_CLK_DIV_CNTRL
#define CLK_REG_DIV_BUG_SIZE		(PLL0_M_LSB - AXI_CLK_DIV_CNTRL)

static u8 clk_div_backup_table [CLK_REG_DIV_BUG_SIZE];

void c2k_clk_div_backup_relocate_table (void)
{
	memcpy (clk_div_backup_table, (void*) IRAM_CLK_REG_MIRROR, CLK_REG_DIV_BUG_SIZE);
}
EXPORT_SYMBOL(c2k_clk_div_backup_relocate_table);

#define to_clk_hw_comcerto2000(_hw) container_of(_hw, struct clk_hw_comcerto2000, hw)

unsigned long comcerto2000_recalc(struct clk_hw *hw,
		unsigned long parent_rate) {
	struct clk_hw_comcerto2000 *clk_hw = to_clk_hw_comcerto2000(hw);
	unsigned long rate;
	u32 div;

	if (!parent_rate)
		return 0;

	if (clk_hw->div_bypass)
		div = 1;
	else {
		/* Get clock divider bypass value from IRAM Clock Divider registers mirror location */
		div = readl(clk_hw->reg + clk_hw->divreg_offset); //TODO(danielmentz): #define for div offset
		div &= 0x1f;
	}

	rate = parent_rate / div;

	// pr_err("clock %s running at %lu div %u parent rate %lu\n", __clk_get_name(hw->clk), rate, div, parent_rate);

	return rate;
}

long comcerto2000_round_rate(struct clk_hw *hw, unsigned long target_rate,
			unsigned long *parent_rate) {
	u32 div;

	/* Get the divider value for clock */
	div = (*parent_rate + target_rate - 1) / target_rate;
	if (div == 0 || div > 0x1f)
		return -EINVAL;
	return *parent_rate / div;
}

int comcerto2000_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate) {
	struct clk_hw_comcerto2000 *clk_hw = to_clk_hw_comcerto2000(hw);
	u32 div, val;

	/* Get the divider value for clock */
	div = (parent_rate + rate - 1) / rate;
	if (div == 0 || div > 0x1f)
		return -EINVAL;

	val = readl(clk_hw->reg + clk_hw->divreg_offset);
	if (div == 1) {
		//TODO(danielmentz): bypass bug
		/* Enable the Bypass bit in hw reg (clk_div_bypass in div_reg) */
		val |= CLK_DIV_BYPASS;
		clk_hw->div_bypass = 1;
	} else {
		//TODO(danielmentz): bypass bug
		/* Clear the Bypass bit in hw reg (clk_div_bypass in div_reg) */
		clk_hw->div_bypass = 0;
		val &= ~CLK_DIV_BYPASS;
		val &= ~0x1f;
		val |= div;
	}
	writel(val, clk_hw->reg + clk_hw->divreg_offset);

	return 0;
}


const struct clk_ops comcerto2000_clk_ops = {
	.set_rate	= &comcerto2000_set_rate,
	.recalc_rate	= &comcerto2000_recalc,
	.round_rate	= &comcerto2000_round_rate,
};

static void __init of_c2k_clk_setup(struct device_node *node) {
	const char *name;
	int num_parents;
	const char **parent_names = NULL;
	struct clk *clk;
	struct clk_mux *mux = NULL;
	struct clk_gate *gate = NULL;
	struct clk_hw_comcerto2000 *rate = NULL;
	void __iomem *regbase;
	resource_size_t regbase_phys;
	struct resource res;
	u32 div_reg;
	int i;

	name = node->name;
	of_property_read_string(node, "clock-output-names", &name);
	num_parents = of_clk_get_parent_count(node);
	if (num_parents != 5) {
		pr_err("%s must have five parents\n", node->name);
		goto cleanup;
	}
	parent_names = kzalloc(sizeof(char *) * num_parents, GFP_KERNEL);
	if (!parent_names)
		goto cleanup;
	for (i = 0; i < num_parents; i++)
		parent_names[i] = of_clk_get_parent_name(node, i);
	regbase = of_iomap(node, 0);
	WARN_ON(!regbase);
	if (of_address_to_resource(node, 0, &res))
		goto cleanup;
	regbase_phys = res.start;
	mux = kzalloc(sizeof(*mux), GFP_KERNEL);
	if (!mux)
		goto cleanup;
	mux->reg = regbase + 0; // TODO(danielmentz): #define for mux offset
	mux->shift = CLK_PLL_SRC_SHIFT;
	mux->mask = CLK_PLL_SRC_MASK;
	mux->flags = CLK_MUX_READ_ONLY;
	mux->lock = 0; //TODO(danielmentz)

	gate = kzalloc(sizeof(*gate), GFP_KERNEL);
	if (!gate)
		goto cleanup;
	gate->flags = 0;
	gate->reg = regbase + 0; // TODO(danielmentz): #define for gate offset
	gate->bit_idx = 0; //TODO(danielmentz): #define for bit_idx
	gate->lock = 0; //TODO(danielmentz)

	rate = kzalloc(sizeof(*rate), GFP_KERNEL);
	if (!rate)
		goto cleanup;
	rate->reg = regbase;
	rate->divreg_offset = 4;// TODO(danielmentz): #define for div offset == 4
	of_property_read_u32(node, "fsl,divreg-offset", &rate->divreg_offset);
	div_reg = *((u32 *) (regbase_phys + rate->divreg_offset -
			CLK_REG_DIV_BUG_BASE + clk_div_backup_table));
	rate->div_bypass = (div_reg & CLK_DIV_BYPASS) ? 1 : 0;
	clk = clk_register_composite(NULL, name,
			parent_names, num_parents,
			&mux->hw, &clk_mux_ro_ops,
			&rate->hw, &comcerto2000_clk_ops,
			&gate->hw, &clk_gate_ops,
			0); // TODO: flags)
	if (IS_ERR(clk))
		goto cleanup;
	of_clk_add_provider(node, of_clk_src_simple_get, clk);

	goto out;

cleanup:
	kfree(mux);
	kfree(gate);
	kfree(rate);
out:
	/*
	 * We don't need to retain parent_names. This array is only used for
	 * initialization.
	 * */
	kfree(parent_names);
}
CLK_OF_DECLARE(c2k_pll_clock, "fsl,ls1024a-clock", of_c2k_clk_setup);

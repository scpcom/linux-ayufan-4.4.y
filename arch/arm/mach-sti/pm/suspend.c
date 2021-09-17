#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/irqchip/arm-gic.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include "suspend.h"
#include "poke_table.h"

#define L2CC_WAY_MASK			0x10000
#define L2CC_AUX_CNTRL_REG_OFFSET	0x104
#define WAY_MASK_SIXTEEN_WAYS		0xffff
#define WAY_MASK_EIGHT_WAYS		0xff
#define WAY_SIZE_MASK			0x7
#define WAY_SIZE_SHIFT			17
#define MIN_WAY_SIZE			16384

static int sti_suspend_valid(suspend_state_t state);
static int sti_suspend_begin(suspend_state_t state);
static int sti_suspend_prepare(void);
static int sti_suspend_enter(suspend_state_t state);

struct sti_hw_state_desc sti_hw_pltf_states[MAX_SUSPEND_STATE],
	sti_hw_states[MAX_SUSPEND_STATE] = {
	{
		.target_state = PM_SUSPEND_STANDBY,
		.desc = "HOST PASSIVE STANDBY",
		.setup = sti_hps_setup,
		.enter = sti_hps_enter,
		.prepare = sti_hps_prepare,
		.clock_state = CLOCK_REDUCED,
		.ddr_state = DDR_SR,
		.init_time_prepare = 0,
		.mmu_on_while_ddr_off = 0,
	},
	{
		.target_state = PM_SUSPEND_MEM,
		.desc =	"CONTROLLER PASSIVE STANDBY",
		.setup = sti_cps_setup,
		.enter = sti_cps_enter,
		.prepare = sti_cps_prepare,
		.clock_state = CLOCKS_OFF,
		.ddr_state = DDR_SR,
		.init_time_prepare = 1,
		.mmu_on_while_ddr_off = 1,
	},
};

struct sti_platform_suspend sti_suspend = {
	.hw_states = sti_hw_pltf_states,
	.hw_state_nr = 0,
	.index = -1,
	.ops.valid = sti_suspend_valid,
	.ops.begin = sti_suspend_begin,
	.ops.prepare_late = sti_suspend_prepare,
	.ops.enter = sti_suspend_enter,
};

static const unsigned long suspend_end_table[4] = { OP_END_POKES, 0, 0, 0 };

enum tbl_level {
	TBL_ENTER,
	TBL_EXIT
};

static const struct sti_low_power_syscfg_info ddr3_syscfg_info = {
	.ddr3_cfg_offset = 0x160,
	.ddr3_stat_offset = 0x920,
	.cfg_shift = 0,
	.stat_shift = 0,
};

static struct of_device_id suspend_of_match[] = {
	 
	{
		 
		.compatible = "st,stih407-ddr-controller",
		.data = &ddr3_syscfg_info,
	},
	 
	{}
};

static int sti_suspend_valid(suspend_state_t state)
{
	int i;

	for (i = 0; i < sti_suspend.hw_state_nr; i++) {
		 
		if (state == sti_suspend.hw_states[i].target_state) {
			pr_info("sti pm: support mode: %s\n",
				sti_suspend.hw_states[i].desc);
			return true;
		}
	}

	return false;
}

static int sti_suspend_begin(suspend_state_t state)
{
	int i;

	sti_suspend.index = -1;

	for (i = 0; i < sti_suspend.hw_state_nr; i++) {
		 
		if (state == sti_suspend.hw_states[i].target_state) {
			sti_suspend.index = i;
			return 0;
		}
	}

	return -EINVAL;
}

static void sti_copy_suspend_table(struct sti_suspend_table *table,
				   void **__va_add,
				   enum tbl_level level)
{
	unsigned long const *tbl_ptr;
	unsigned long tbl_len_byte;

	if (level == TBL_ENTER) {
		tbl_ptr = table->enter;
		tbl_len_byte = table->enter_size;
	} else {
		tbl_ptr = table->exit;
		tbl_len_byte = table->exit_size;
	}

	if (!table->base_address)
		memcpy(*__va_add, tbl_ptr, tbl_len_byte);
	else
		patch_poke_table_copy(*__va_add, tbl_ptr,
				      table->base_address,
				      tbl_len_byte/sizeof(long));

	*__va_add += tbl_len_byte;
}

static void sti_insert_poke_table_end(void **__va_add)
{
	memcpy(*__va_add, suspend_end_table,
	       ARRAY_SIZE(suspend_end_table) * sizeof(long));

	*__va_add += ARRAY_SIZE(suspend_end_table) * sizeof(long);
}

static int sti_suspend_prepare(void)
{
	int index;
	struct sti_suspend_table *table;
	void *__va_buf;

	if (sti_suspend.index == -1)
		return -EINVAL;

	index = sti_suspend.index;

	if (sti_suspend.hw_states[index].init_time_prepare == -1)
		return 0;

	pr_info("sti pm: Copying poke table & poke loop to buffer\n");
	__va_buf = sti_suspend.hw_states[index].cache_buffer;

	sti_suspend.hw_states[index].buffer_data.pokeloop = __va_buf;

	memcpy(__va_buf, sti_pokeloop, sti_pokeloop_sz);
	__va_buf += sti_pokeloop_sz;

	sti_suspend.hw_states[index].buffer_data.table_enter = __va_buf;

	list_for_each_entry(table,
			    &sti_suspend.hw_states[index].state_tables,
			    node) {
		if (!table->enter)
			continue;

		sti_copy_suspend_table(table, &__va_buf, TBL_ENTER);
	}

	sti_insert_poke_table_end(&__va_buf);

	sti_suspend.hw_states[index].buffer_data.table_exit = __va_buf;

	list_for_each_entry_reverse(table,
				    &sti_suspend.hw_states[index].
				    state_tables,
				    node) {
		if (!table->exit)
			continue;

		sti_copy_suspend_table(table, &__va_buf, TBL_EXIT);
	}

	sti_insert_poke_table_end(&__va_buf);

	sti_suspend.hw_states[index].buffer_data.sti_buffer_code = __va_buf;

	pr_info("sti pm: entering prepare:%s\n",
		sti_suspend.hw_states[index].desc);

	if (sti_suspend.hw_states[index].prepare)
		return sti_suspend.hw_states[index].
			prepare(&sti_suspend.hw_states[index]);

	return 0;
}

static int sti_suspend_enter(suspend_state_t state)
{
	if (sti_suspend.index == -1 ||
	    state != sti_suspend.hw_states[sti_suspend.index].target_state)
			return -EINVAL;

	pr_info("sti pm: entering suspend:%s\n",
		sti_suspend.hw_states[sti_suspend.index].desc);

	if (!sti_suspend.hw_states[sti_suspend.index].enter) {
		pr_err("sti pm: state enter() is NULL\n");
		return -EFAULT;
	}

	return sti_suspend.hw_states[sti_suspend.index].
			enter(&sti_suspend.hw_states[sti_suspend.index]);
}

static int sti_gic_set_wake(struct irq_data *d, unsigned int on)
{
	if (d->irq <= MAX_GIC_SPI_INT)
		return 0;

	return -ENXIO;
}

static int __init sti_suspend_setup(void)
{
	int i;
	int ret = 0;
	struct device_node *np, *child, *np_ref;
	unsigned long ddr_pctl_addresses_va[MAX_DDR_PCTL_NUMBER];
	unsigned long ddr_pctl_addresses_pa[MAX_DDR_PCTL_NUMBER];
	unsigned int nr_ddr_pctl = 0;
	const struct of_device_id *match = NULL;
	struct sti_ddr3_low_power_info lp_info[MAX_DDR_PCTL_NUMBER];
	void __iomem *cleaner[(2 * MAX_DDR_PCTL_NUMBER) + 1];
	int cleaner_index = 0;

	np = of_find_compatible_node(NULL, NULL, "arm,pl310-cache");
	if (IS_ERR_OR_NULL(np)) {
		pr_err("sti pm cps: L2 Cache controller entry not found in DT\n");
		return -ENODEV;
	}

	sti_suspend.l2cachebase = of_iomap(np, 0);
	if (!sti_suspend.l2cachebase) {
		of_node_put(np);
		return -ENODEV;
	}

	cleaner[cleaner_index++] = sti_suspend.l2cachebase;

	of_node_put(np);

	sti_suspend.l2waymask = readl_relaxed(sti_suspend.l2cachebase +
						L2CC_AUX_CNTRL_REG_OFFSET);

	if (sti_suspend.l2waymask & L2CC_WAY_MASK)
		sti_suspend.l2waymask = WAY_MASK_SIXTEEN_WAYS;  
	else
		sti_suspend.l2waymask = WAY_MASK_EIGHT_WAYS;  

	np = of_find_node_by_name(NULL, "ddr-pctl-controller");
	if (IS_ERR_OR_NULL(np))
		goto err;

	for_each_child_of_node(np, child) {
		struct resource res;

		if (of_address_to_resource(child, 0, &res)) {
			of_node_put(np);
			goto err;
		}

		ddr_pctl_addresses_pa[nr_ddr_pctl] = res.start;

		ddr_pctl_addresses_va[nr_ddr_pctl] = (unsigned int)
				ioremap(res.start, resource_size(&res));

		if (!ddr_pctl_addresses_va[nr_ddr_pctl]) {
			of_node_put(np);
			goto err;
		}
		cleaner[cleaner_index++] = (void *)
					ddr_pctl_addresses_va[nr_ddr_pctl];

		match = of_match_node(suspend_of_match, child);
		if (!match) {
			lp_info[nr_ddr_pctl].sysconf_base = 0;
			nr_ddr_pctl++;
			continue;
		}

		np_ref = of_parse_phandle(child, "st,syscfg", 0);
		if (IS_ERR_OR_NULL(np_ref)) {
			of_node_put(np);
			goto err;
		}

		lp_info[nr_ddr_pctl].sysconf_base = (unsigned int)
							of_iomap(np_ref, 0);

		if (!lp_info[nr_ddr_pctl].sysconf_base) {
			of_node_put(np_ref);
			of_node_put(np);
			goto err;
		}
		cleaner[cleaner_index++] = (void *)
					lp_info[nr_ddr_pctl].sysconf_base;

		of_node_put(np_ref);

		lp_info[nr_ddr_pctl].sysconf_info =
				*((struct sti_low_power_syscfg_info *)
				match->data);

		nr_ddr_pctl++;
	}

	of_node_put(np);

	for (i = 0; i < MAX_SUSPEND_STATE; i++) {
		int mmu_state;
		struct sti_hw_state_desc *hw_state = sti_suspend.hw_states;

		hw_state += sti_suspend.hw_state_nr;
		*hw_state = sti_hw_states[i];

		mmu_state = hw_state->mmu_on_while_ddr_off;

		ret = hw_state->setup(hw_state,
				      mmu_state ? ddr_pctl_addresses_va :
							ddr_pctl_addresses_pa,
				      nr_ddr_pctl,
				      lp_info);

		if (!ret && hw_state->init_time_prepare == 0x1) {
#ifdef MY_DEF_HERE
			sti_suspend.index = sti_suspend.hw_state_nr;
#else  
			sti_suspend.index = i;
#endif  
			ret = sti_suspend_prepare();
			hw_state->init_time_prepare = -1;
		}

		if (!ret)
			sti_suspend.hw_state_nr++;
	}

	if (!sti_suspend.hw_state_nr)
		goto err;

	suspend_set_ops(&sti_suspend.ops);

	if (!gic_arch_extn.irq_set_wake)
		gic_arch_extn.irq_set_wake = sti_gic_set_wake;

	pr_info("sti pm: Suspend support registered\n");

	return 0;
err:
	for (i = 0; i < cleaner_index; i++)
		iounmap(cleaner[cleaner_index]);

	return -ENODEV;
}

unsigned int sti_get_l2cc_way_size(void)
{
	u32 l2ccaux_reg;
	unsigned int way_size_code;

	u32 l2cc_way_size[] = {MIN_WAY_SIZE,
			       MIN_WAY_SIZE,
			       2 * MIN_WAY_SIZE,
			       4 * MIN_WAY_SIZE,
			       8 * MIN_WAY_SIZE,
			       16 * MIN_WAY_SIZE,
			       32 * MIN_WAY_SIZE,
			       32 * MIN_WAY_SIZE,
			};

	l2ccaux_reg = readl_relaxed(sti_suspend.l2cachebase +
						L2CC_AUX_CNTRL_REG_OFFSET);

	way_size_code = (l2ccaux_reg >> WAY_SIZE_SHIFT) & WAY_SIZE_MASK;

	return l2cc_way_size[way_size_code];
}

module_init(sti_suspend_setup);

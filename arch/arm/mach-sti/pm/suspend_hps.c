#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
  
#include <asm/idmap.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>
#include <asm-generic/sections.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/irqchip/arm-gic.h>

#include "suspend.h"
#include "suspend_interface.h"
#include "suspend_internal.h"
#include "poke_table.h"
#include "synopsys_dwc_ddr32.h"

#define CLKA_PLL_CFG(pll_nr, cfg_nr)	((pll_nr) * 0xc + (cfg_nr) * 0x4)
#define CLKA_PLL_LOCK_REG(pll_nr)	CLKA_PLL_CFG(pll_nr, 1)
#define CLKA_PLL_LOCK_STATUS		BIT(31)
#define CLKA_POWER_CFG			(0x18)
#define CLKA_SWITCH_CFG(x)		(0x1c + (x) * 0x4)
#define CLKA_A1_DDR_CLK_ID		5

#define CLKA_FLXGEN_CFG(x)		(0x18 + (x) * 0x4)
#define CLKA_FLXGEN_PLL_CFG		(0x2a0)

#define STID127_A9_CLK_SELECTION	(0x0)
#define STID127_A9_PLL_POWER_DOWN	(0x0)
#define STID127_A9_PLL_LOCK_STATUS	(0x98)

#ifdef MY_DEF_HERE
#else  
#define STIH407_A9_CLK_SELECTION	(0x1a4)
#define STIH407_A9_PLL_POWER_DOWN	(0x1a8)
#define STIH407_A9_PLL_LOCK_STATUS	(0x87c)
#endif  

static DEFINE_MUTEX(hps_notify_mutex);
static LIST_HEAD(hps_notify_list);

struct hps_private_data {
	void __iomem *gic_iomem_interface;
	void __iomem *gic_iomem_dist;
	unsigned int eram_base;
};

static struct hps_private_data hps_data;

static struct sti_suspend_table sti_hps_tables[MAX_SUSPEND_TABLE_SIZE];

static struct poke_operation sti_hps_ddr_enter[] = {
	 
	POKE32(DDR_SCTL, DDR_SCTL_SLEEP),
	WHILE_NE32(DDR_STAT, DDR_STAT_MASK, DDR_STAT_LOW_POWER),

	OR32(DDR_PHY_DXCCR, DDR_PHY_DXCCR_DXODT),
	OR32(DDR_PHY_PIR, DDR_PHY_PIR_PLL_RESET),  
};

static struct poke_operation sti_hps_ddr_exit[] = {
	 
	UPDATE32(DDR_PHY_PIR, ~DDR_PHY_PIR_PLL_RESET, 0),
	 
	WHILE_NE32(DDR_PHY_PGSR0, DDR_PHY_PLLREADY, DDR_PHY_PLLREADY),
	UPDATE32(DDR_PHY_DXCCR, ~DDR_PHY_DXCCR_DXODT, 0),

	POKE32(DDR_SCTL, DDR_SCTL_WAKEUP),
	WHILE_NE32(DDR_STAT, DDR_STAT_MASK, DDR_STAT_ACCESS),
	POKE32(DDR_SCTL, DDR_SCTL_CFG),
	WHILE_NE32(DDR_STAT, DDR_STAT_MASK, DDR_STAT_CONFIG),
	POKE32(DDR_SCTL, DDR_SCTL_GO),
	WHILE_NE32(DDR_STAT, DDR_STAT_MASK, DDR_STAT_ACCESS),
};

#ifdef MY_DEF_HERE
#else  
static  struct poke_operation stih41x_hps_ddr_pll_enter[] = {
	OR32(DDR_PLL_CFG_OFFSET, 1),
};

static  struct poke_operation stih41x_hps_ddr_pll_exit[] = {
	UPDATE32(DDR_PLL_CFG_OFFSET, ~1, 0),
	WHILE_NE32(DDR_PLL_STATUS_OFFSET, 1, 1),
};
#endif  

static  struct poke_operation stid127_hps_ddr_pll_enter[] = {
#ifdef STID127_HPS_ISSUE_RESOLVE
	 
	UPDATE32(CLKA_SWITCH_CFG(0), ~(3 << (CLKA_A1_DDR_CLK_ID * 2)),
		 0 << (CLKA_A1_DDR_CLK_ID * 2)),
	OR32(CLKA_POWER_CFG, 0x1),
#else
	DELAY(1),
#endif
};

static  struct poke_operation stid127_hps_ddr_pll_exit[] = {
	 
	POKE32(CLKA_POWER_CFG, 0x0),
	 
	WHILE_NE32(CLKA_PLL_LOCK_REG(0), CLKA_PLL_LOCK_STATUS,
		   CLKA_PLL_LOCK_STATUS),

	UPDATE32(CLKA_SWITCH_CFG(0), ~(3 << (CLKA_A1_DDR_CLK_ID * 2)),
		 1 << (CLKA_A1_DDR_CLK_ID * 2)),
};

static struct poke_operation stid127_hps_a9_clk_enter[] = {
	 
	OR32(STID127_A9_CLK_SELECTION, 1 << 2),
	OR32(STID127_A9_PLL_POWER_DOWN, 1),
};

static struct poke_operation stid127_hps_a9_clk_exit[] = {
	 
	UPDATE32(STID127_A9_PLL_POWER_DOWN, ~1, 0),
	WHILE_NE32(STID127_A9_PLL_LOCK_STATUS, 1, 1),
	UPDATE32(STID127_A9_CLK_SELECTION, ~(1 << 2), 0),
};

#ifdef MY_DEF_HERE
#else  
static  struct poke_operation stih407_hps_ddr_pll_enter[] = {
#ifdef STIH407_HPS_ISSUE_RESOLVE
	 
	UPDATE32(CLKA_FLXGEN_CFG(0), ~(BIT(6) - 1), 0x1),
	 
	OR32(CLKA_FLXGEN_PLL_CFG, 0x1 << 8),
#else
	DELAY(1),
#endif
};

static  struct poke_operation stih407_hps_ddr_pll_exit[] = {
	 
	UPDATE32(CLKA_FLXGEN_PLL_CFG, ~(0x1 << 8), 0),
	 
	WHILE_NE32(CLKA_FLXGEN_PLL_CFG, 1 << 24, 1 << 24),
	 
	UPDATE32(CLKA_FLXGEN_CFG(0), ~(BIT(6) - 1), 0),
};

static struct poke_operation stih407_hps_a9_clk_enter[] = {
	 
	OR32(STIH407_A9_CLK_SELECTION, 1 << 1),
	OR32(STIH407_A9_PLL_POWER_DOWN, 1),
};

static struct poke_operation stih407_hps_a9_clk_exit[] = {
	 
	UPDATE32(STIH407_A9_PLL_POWER_DOWN, ~1, 0),
	WHILE_NE32(STIH407_A9_PLL_LOCK_STATUS, 1, 1),
	UPDATE32(STIH407_A9_CLK_SELECTION, ~(1 << 1), 0),
};
 
#endif  

static int sti_get_wake_irq(void *gic_addr)
{
	int irq = 0;
	struct irq_data *d;

	irq = readl(gic_addr + GIC_CPU_INTACK);
	d = irq_get_irq_data(irq);
	writel(d->hwirq, gic_addr + GIC_CPU_EOI);

	return irq;
}

int sti_hps_get_eram_size(struct sti_hw_state_desc *state)
{
	struct sti_suspend_table *table;
	int eram_size = 0;

	eram_size = sti_hps_on_eram_sz + sti_pokeloop_sz;

	list_for_each_entry(table, &state->state_tables, node) {
		eram_size += table->enter_size;
		eram_size += table->exit_size;
	}

	eram_size += 2 * sizeof(struct poke_operation);

	return eram_size;
}

int sti_chk_pend(int wkirq)
{
	struct sti_hps_notify *handler;

	list_for_each_entry(handler, &hps_notify_list, list)
		if (handler->irq == wkirq)
			return handler->notify();

	return -1;
}

static enum sti_hps_notify_ret sti_hps_early_check(int wkirq, void *gic_addr)
{
	struct sti_hps_notify *handler;
	int int_num = 32;
	int counter = 1;
	u32 regval, enabled;
	int i;

	int r;
	enum sti_hps_notify_ret ret = STI_HPS_RET_OK;

	list_for_each_entry(handler, &hps_notify_list, list) {
		if (handler->irq == wkirq) {
			if (handler->notify() == STI_HPS_RET_OK)
				return STI_HPS_RET_OK;
			else
				ret = STI_HPS_RET_AGAIN;
		}
	}

	while (int_num <= MAX_GIC_SPI_INT) {
		regval = readl(gic_addr + (GIC_DIST_PENDING_SET + counter * 4));
		enabled = readl(gic_addr + (GIC_DIST_ENABLE_SET + counter * 4));

		if ((regval & enabled) != 0) {
			for (i = 0; i < 32; i++) {
				if (((regval >> i) & 0x1) &&
				    ((int_num + i) != wkirq)) {
					r = sti_chk_pend(int_num + i);
					if (r == 0)
						return STI_HPS_RET_OK;
					else if (r == 1)
						ret = STI_HPS_RET_AGAIN;
				}
			}
		}
		counter++;
		int_num += 32;
	}

	return ret;
}

static struct  sti_hps_notify *__look_for(struct sti_hps_notify *handler)
{
	struct sti_hps_notify *p;

	list_for_each_entry(p,  &hps_notify_list, list)
		if (p == handler || p->irq == handler->irq)
			return p;

	return NULL;
}

int sti_hps_register_notify(struct sti_hps_notify *handler)
{
	if (!handler || !handler->notify || handler->irq > 255)
		return -EINVAL;

	mutex_lock(&hps_notify_mutex);
	if (__look_for(handler)) {
		mutex_unlock(&hps_notify_mutex);
		return -EEXIST;
	}

	list_add(&handler->list, &hps_notify_list);
	mutex_unlock(&hps_notify_mutex);

	return 0;
}
EXPORT_SYMBOL(sti_hps_register_notify);

int sti_hps_unregister_notify(struct sti_hps_notify *handler)
{
	if (!handler)
		return -EINVAL;

	mutex_lock(&hps_notify_mutex);
	if (!__look_for(handler)) {
		mutex_unlock(&hps_notify_mutex);
		return -EINVAL;
	}

	list_del(&handler->list);
	mutex_unlock(&hps_notify_mutex);

	return 0;
}
EXPORT_SYMBOL(sti_hps_unregister_notify);

static inline void *va_2_pa(unsigned int base_pa,
			    void *base_va,
			    void *va)
{
	if ((unsigned int)base_va > base_pa)
		return va - ((unsigned int) base_va) - base_pa;
	else
		return base_pa - ((unsigned int) base_va) + va;
}

int sti_hps_prepare(struct sti_hw_state_desc *state)
{
	unsigned int eram_base = ((struct hps_private_data *)
					state->state_private_data)->eram_base;
	void *eram_va = (void *)state->cache_buffer;

	memcpy(state->buffer_data.sti_buffer_code, sti_hps_on_eram,
	       sti_hps_on_eram_sz);

	state->buffer_data.table_enter = va_2_pa(eram_base,
						 eram_va,
						 state->buffer_data.table_enter
						 );

	state->buffer_data.table_exit = va_2_pa(eram_base,
						eram_va,
						state->buffer_data.table_exit);

	state->buffer_data.sti_buffer_code = va_2_pa(eram_base,
						     eram_va,
						     state->buffer_data.
							sti_buffer_code);
	state->buffer_data.pokeloop = va_2_pa(eram_base,
					      eram_va,
					      state->buffer_data.pokeloop);

	return 0;
}

int sti_hps_enter(struct sti_hw_state_desc *state)
{
	enum sti_hps_notify_ret notify_ret;
	int wake_irq = 0;

	unsigned long  va_2_pa = (unsigned long)_text -
				(unsigned long)__pa(_text);

	if (state == NULL)
		return -EINVAL;

	pr_info("CPU is sleeping\n");

sti_again_hps:
	if (!list_empty(&state->state_tables)) {
		 
		flush_cache_all();
		outer_flush_all();

		cpu_switch_mm(idmap_pgd, &init_mm);

		local_flush_tlb_all();

		sti_hps_exec((struct sti_suspend_buffer_data *)
			     __pa(&state->buffer_data),
			     va_2_pa);

		flush_cache_all();

		cpu_switch_mm(current->mm->pgd, current->mm);

		local_flush_tlb_all();
	}

	wake_irq = sti_get_wake_irq(((struct hps_private_data *)state->
				      state_private_data)->gic_iomem_interface);

	notify_ret = sti_hps_early_check(wake_irq,
					 ((struct hps_private_data *)state->
					   state_private_data)->gic_iomem_dist);
	if (notify_ret == STI_HPS_RET_AGAIN) {
		pr_info("sti pm hps: System woken up from HPS by: %d\n",
			wake_irq);
		pr_info("sti pm hps: Suspending again\n");
		goto sti_again_hps;
	}

	pr_info("sti pm hps: System woken up from HPS by: %d\n", wake_irq);

	return 0;
}

int sti_hps_setup(struct sti_hw_state_desc *state, unsigned long *ddr_pctl_addr,
		  unsigned int no_pctl,
		  struct sti_ddr3_low_power_info *unused)
{
	int index = 0;
	struct device_node *np;
	int reg, size_reg;
	unsigned int ddr_sys_config_base;
	int i;

	if (state->ddr_state == DDR_SR) {
		for (i = 0; i < no_pctl; i++) {
			populate_suspend_table_entry(&sti_hps_tables[index++],
						     (long *)sti_hps_ddr_enter,
						     (long *)sti_hps_ddr_exit,
						     sizeof(sti_hps_ddr_enter),
						     sizeof(sti_hps_ddr_exit),
						     ddr_pctl_addr[i]);
		}

#ifdef MY_DEF_HERE
		if (of_machine_is_compatible("st,stid127")) {
#else  
		if (of_machine_is_compatible("st,stih416")) {
			np = of_find_compatible_node(NULL, NULL,
						     "st,stih416-cpu-syscfg");

			if (IS_ERR_OR_NULL(np))
				return -ENODEV;

			if (of_property_read_u32_index(np, "reg", 0, &reg)) {
				of_node_put(np);
				return -ENODEV;
			}
			ddr_sys_config_base = reg;
			of_node_put(np);

			populate_suspend_table_entry(&sti_hps_tables[index++],
						     (long *)
						     stih41x_hps_ddr_pll_enter,
						     (long *)
						     stih41x_hps_ddr_pll_exit,
						     sizeof(
						     stih41x_hps_ddr_pll_enter),
						     sizeof(
						     stih41x_hps_ddr_pll_exit),
						     ddr_sys_config_base);
		} else if (of_machine_is_compatible("st,stid127")) {
#endif  
			np = of_find_node_by_name(NULL, "ddr-pctl-controller");
			if (IS_ERR_OR_NULL(np))
					return -ENODEV;

			if (of_property_read_u32_index(np, "reg", 0, &reg)) {
				of_node_put(np);
				return -ENODEV;
			}
			of_node_put(np);

			populate_suspend_table_entry(&sti_hps_tables[index++],
						     (long *)
						     stid127_hps_ddr_pll_enter,
						     (long *)
						     stid127_hps_ddr_pll_exit,
						     sizeof(
						     stid127_hps_ddr_pll_enter),
						     sizeof(
						     stid127_hps_ddr_pll_exit),
						     reg);

			np = of_find_node_by_name(NULL, "clockgenA9");
			if (IS_ERR_OR_NULL(np))
				return -ENODEV;

			if (of_property_read_u32_index(np, "reg", 0, &reg)) {
				of_node_put(np);
				return -ENODEV;
			}
			of_node_put(np);

			populate_suspend_table_entry(&sti_hps_tables[index++],
						     (long *)
						     stid127_hps_a9_clk_enter,
						     (long *)
						     stid127_hps_a9_clk_exit,
						     sizeof(
						     stid127_hps_a9_clk_enter),
						     sizeof(
						     stid127_hps_a9_clk_exit),
						     reg);
#ifdef MY_DEF_HERE
		} else if (of_machine_is_compatible("st,stih407") ||
			   of_machine_is_compatible("st,stih410") ||
			   of_machine_is_compatible("st,stih416")) {

			return -ENOTSUPP;
#else  
		} else if (of_machine_is_compatible("st,stih407")) {
			np = of_find_node_by_name(NULL, "ddr-pctl-controller");
			if (IS_ERR_OR_NULL(np))
				return -ENODEV;

			if (of_property_read_u32_index(np, "reg", 0, &reg)) {
				of_node_put(np);
				return -ENODEV;
			}
			of_node_put(np);

			populate_suspend_table_entry(&sti_hps_tables[index++],
						     (long *)
						     stih407_hps_ddr_pll_enter,
						     (long *)
						     stih407_hps_ddr_pll_exit,
						     sizeof(
						     stih407_hps_ddr_pll_enter),
						     sizeof(
						     stih407_hps_ddr_pll_exit),
						     reg);

			np = of_find_node_by_name(NULL, "clockgenA9");
			if (IS_ERR_OR_NULL(np))
				return -ENODEV;

			if (of_property_read_u32_index(np, "reg", 0, &reg)) {
				of_node_put(np);
				return -ENODEV;
			}
			of_node_put(np);

			populate_suspend_table_entry(&sti_hps_tables[index++],
						     (long *)
						     stih407_hps_a9_clk_enter,
						     (long *)
						     stih407_hps_a9_clk_exit,
						     sizeof(
						     stih407_hps_a9_clk_enter),
						     sizeof(
						     stih407_hps_a9_clk_exit),
						     reg);
#endif  
		}
	} else if (state->ddr_state == DDR_OFF) {
		pr_info("sti pm hps: DDR_OFF not supported in HPS\n");
		return -EINVAL;
	}

	INIT_LIST_HEAD(&state->state_tables);
	for (i = 0; i < index; ++i)
		list_add_tail(&sti_hps_tables[i].node,
			      &state->state_tables);

	np = of_find_node_by_type(NULL, "eram");
	if (IS_ERR_OR_NULL(np))
		return -ENODEV;

	index = of_property_match_string(np, "reg-names", "eram_1");
	if (index < 0) {
		of_node_put(np);
		return -ENODEV;
	}

	index = index * (of_n_addr_cells(np) + of_n_size_cells(np));
	if (of_property_read_u32_index(np, "reg", index, &reg)) {
		pr_err("sti pm hps: eram_1 base address not found\n");
		of_node_put(np);
		return -ENODEV;
	}

	hps_data.eram_base = reg;
	of_node_put(np);

	np = of_find_compatible_node(NULL, NULL, "arm,cortex-a9-gic");

	if (IS_ERR_OR_NULL(np))
		return -ENODEV;

	if (of_property_read_u32_index(np, "reg", 0, &reg)) {
		pr_err("sti pm hps: GIC base address (dist.) not found\n");
		of_node_put(np);
		return -ENODEV;
	}

	if (of_property_read_u32_index(np, "reg", 1, &size_reg)) {
		pr_err("sti pm hps: GIC base size (dist.) not found\n");
		of_node_put(np);
		return -ENODEV;
	}

	hps_data.gic_iomem_dist = ioremap_nocache(reg, size_reg);
	if (!hps_data.gic_iomem_dist) {
		pr_err("sti pm hps: GIC dist. base remap failed\n");
		of_node_put(np);
		return -ENODEV;
	}

	if (of_property_read_u32_index(np, "reg", 2, &reg)) {
		pr_err("sti pm hps: GIC base address (inter.) not found\n");
		iounmap(hps_data.gic_iomem_dist);
		of_node_put(np);
		return -ENODEV;
	}

	if (of_property_read_u32_index(np, "reg", 3, &size_reg)) {
		pr_err("sti pm hps: GIC base size (inter.) not found\n");
		of_node_put(np);
		iounmap(hps_data.gic_iomem_dist);
		return -ENODEV;
	}

	hps_data.gic_iomem_interface = ioremap_nocache(reg, size_reg);

	if (!hps_data.gic_iomem_interface) {
		pr_err("sti pm hps: GIC cpu interface base remap failed\n");
		of_node_put(np);
		iounmap(hps_data.gic_iomem_dist);
		return -ENODEV;
	}

	of_node_put(np);

	state->state_private_data = &hps_data;

	state->cache_buffer = ioremap(hps_data.eram_base,
				      sti_hps_get_eram_size(state));
	if (!state->cache_buffer) {
		pr_err("sti pm hps: eram ioremap failed in HPS setup\n");
		iounmap(hps_data.gic_iomem_dist);
		iounmap(hps_data.gic_iomem_interface);
		return -EINVAL;
	}

	return 0;
}

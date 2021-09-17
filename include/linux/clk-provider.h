#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LINUX_CLK_PROVIDER_H
#define __LINUX_CLK_PROVIDER_H

#include <linux/clk.h>

#ifdef CONFIG_COMMON_CLK

#define CLK_SET_RATE_GATE	BIT(0)  
#define CLK_SET_PARENT_GATE	BIT(1)  
#define CLK_SET_RATE_PARENT	BIT(2)  
#define CLK_IGNORE_UNUSED	BIT(3)  
#define CLK_IS_ROOT		BIT(4)  
#define CLK_IS_BASIC		BIT(5)  
#define CLK_GET_RATE_NOCACHE	BIT(6)  

struct clk_hw;

struct clk_ops {
	int		(*prepare)(struct clk_hw *hw);
	void		(*unprepare)(struct clk_hw *hw);
	int		(*is_prepared)(struct clk_hw *hw);
	void		(*unprepare_unused)(struct clk_hw *hw);
	int		(*enable)(struct clk_hw *hw);
	void		(*disable)(struct clk_hw *hw);
	int		(*is_enabled)(struct clk_hw *hw);
	void		(*disable_unused)(struct clk_hw *hw);
	unsigned long	(*recalc_rate)(struct clk_hw *hw,
					unsigned long parent_rate);
	long		(*round_rate)(struct clk_hw *hw, unsigned long,
					unsigned long *);
	int		(*set_parent)(struct clk_hw *hw, u8 index);
	u8		(*get_parent)(struct clk_hw *hw);
	int		(*set_rate)(struct clk_hw *hw, unsigned long,
				    unsigned long);
	void		(*init)(struct clk_hw *hw);
};

struct clk_init_data {
	const char		*name;
	const struct clk_ops	*ops;
	const char		**parent_names;
	u8			num_parents;
	unsigned long		flags;
};

struct clk_hw {
	struct clk *clk;
	const struct clk_init_data *init;
};

struct clk_fixed_rate {
	struct		clk_hw hw;
	unsigned long	fixed_rate;
	u8		flags;
};

extern const struct clk_ops clk_fixed_rate_ops;
struct clk *clk_register_fixed_rate(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		unsigned long fixed_rate);

void of_fixed_clk_setup(struct device_node *np);

struct clk_gate {
	struct clk_hw hw;
	void __iomem	*reg;
	u8		bit_idx;
	u8		flags;
	spinlock_t	*lock;
};

#define CLK_GATE_SET_TO_DISABLE		BIT(0)

extern const struct clk_ops clk_gate_ops;
struct clk *clk_register_gate(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 bit_idx,
		u8 clk_gate_flags, spinlock_t *lock);

struct clk_div_table {
	unsigned int	val;
	unsigned int	div;
};

struct clk_divider {
	struct clk_hw	hw;
	void __iomem	*reg;
	u8		shift;
	u8		width;
	u8		flags;
	const struct clk_div_table	*table;
	spinlock_t	*lock;
};

#define CLK_DIVIDER_ONE_BASED		BIT(0)
#define CLK_DIVIDER_POWER_OF_TWO	BIT(1)
#define CLK_DIVIDER_ALLOW_ZERO		BIT(2)
#if defined (MY_DEF_HERE)
#define CLK_DIVIDER_ROUND_CLOSEST	BIT(3)
#endif  

extern const struct clk_ops clk_divider_ops;
struct clk *clk_register_divider(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 clk_divider_flags, spinlock_t *lock);
struct clk *clk_register_divider_table(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 clk_divider_flags, const struct clk_div_table *table,
		spinlock_t *lock);

struct clk_mux {
	struct clk_hw	hw;
	void __iomem	*reg;
	u32		*table;
	u32		mask;
	u8		shift;
	u8		flags;
	spinlock_t	*lock;
};

#define CLK_MUX_INDEX_ONE		BIT(0)
#define CLK_MUX_INDEX_BIT		BIT(1)

extern const struct clk_ops clk_mux_ops;

struct clk *clk_register_mux(struct device *dev, const char *name,
		const char **parent_names, u8 num_parents, unsigned long flags,
		void __iomem *reg, u8 shift, u8 width,
		u8 clk_mux_flags, spinlock_t *lock);

struct clk *clk_register_mux_table(struct device *dev, const char *name,
		const char **parent_names, u8 num_parents, unsigned long flags,
		void __iomem *reg, u8 shift, u32 mask,
		u8 clk_mux_flags, u32 *table, spinlock_t *lock);

void of_fixed_factor_clk_setup(struct device_node *node);

struct clk_fixed_factor {
	struct clk_hw	hw;
	unsigned int	mult;
	unsigned int	div;
};

extern struct clk_ops clk_fixed_factor_ops;
struct clk *clk_register_fixed_factor(struct device *dev, const char *name,
		const char *parent_name, unsigned long flags,
		unsigned int mult, unsigned int div);

struct clk_composite {
	struct clk_hw	hw;
	struct clk_ops	ops;

	struct clk_hw	*mux_hw;
	struct clk_hw	*rate_hw;
	struct clk_hw	*gate_hw;

	const struct clk_ops	*mux_ops;
	const struct clk_ops	*rate_ops;
	const struct clk_ops	*gate_ops;
};

struct clk *clk_register_composite(struct device *dev, const char *name,
		const char **parent_names, int num_parents,
		struct clk_hw *mux_hw, const struct clk_ops *mux_ops,
		struct clk_hw *rate_hw, const struct clk_ops *rate_ops,
		struct clk_hw *gate_hw, const struct clk_ops *gate_ops,
		unsigned long flags);

struct clk *clk_register(struct device *dev, struct clk_hw *hw);
struct clk *devm_clk_register(struct device *dev, struct clk_hw *hw);

void clk_unregister(struct clk *clk);
void devm_clk_unregister(struct device *dev, struct clk *clk);

const char *__clk_get_name(struct clk *clk);
struct clk_hw *__clk_get_hw(struct clk *clk);
u8 __clk_get_num_parents(struct clk *clk);
struct clk *__clk_get_parent(struct clk *clk);
unsigned int __clk_get_enable_count(struct clk *clk);
unsigned int __clk_get_prepare_count(struct clk *clk);
unsigned long __clk_get_rate(struct clk *clk);
unsigned long __clk_get_flags(struct clk *clk);
bool __clk_is_prepared(struct clk *clk);
bool __clk_is_enabled(struct clk *clk);
struct clk *__clk_lookup(const char *name);

int __clk_prepare(struct clk *clk);
void __clk_unprepare(struct clk *clk);
void __clk_reparent(struct clk *clk, struct clk *new_parent);
unsigned long __clk_round_rate(struct clk *clk, unsigned long rate);

struct of_device_id;

typedef void (*of_clk_init_cb_t)(struct device_node *);

int of_clk_add_provider(struct device_node *np,
			struct clk *(*clk_src_get)(struct of_phandle_args *args,
						   void *data),
			void *data);
void of_clk_del_provider(struct device_node *np);
struct clk *of_clk_src_simple_get(struct of_phandle_args *clkspec,
				  void *data);
struct clk_onecell_data {
	struct clk **clks;
	unsigned int clk_num;
};
struct clk *of_clk_src_onecell_get(struct of_phandle_args *clkspec, void *data);
const char *of_clk_get_parent_name(struct device_node *np, int index);

void of_clk_init(const struct of_device_id *matches);

#define CLK_OF_DECLARE(name, compat, fn)			\
	static const struct of_device_id __clk_of_table_##name	\
		__used __section(__clk_of_table)		\
		= { .compatible = compat, .data = fn };

#endif  
#endif  

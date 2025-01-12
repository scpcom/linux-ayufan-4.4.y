/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (c) 2023, spacemit Corporation.
 */

#ifndef __PINCTRL_SPACEMIT_H
#define __PINCTRL_SPACEMIT_H

#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>

#define SPACEMIT_PINCTRL_PIN(pin)	PINCTRL_PIN(pin, #pin)

#define PINID_TO_BANK(p)	((p) >> 5)
#define PINID_TO_PIN(p)		((p) % 32)

/*
 * pin config bit field definitions
 * config format
 * 0-3  driver_strength
 * 4    pull
 *
 * od:	open drain
 * pe:  pull enable
 * st:	schmit trigger
 * rte:	retention signal bus
 *
 * MSB of each field is presence bit for the config.
 */
#define OD_EN		1
#define OD_DIS		0
#define PE_EN		1
#define PE_DIS		0
#define ST_EN		1
#define ST_DIS		0
#define RTE_EN		1
#define RTE_DIS		0

#define DS_SHIFT	0
#define PULL_SHIFT  4

#define CONFIG_TO_DS(c)		((c) >> DS_SHIFT & 0xf)
#define CONFIG_TO_PULL(c)	((c) >> PULL_SHIFT & 0x1)

struct spacemit_function {
	const char *name;
	const char **groups;
	unsigned ngroups;
};

/*
 * Each pin represented in spacemit,pins consists:
 * - u32 PIN_FUNC_ID
 * - u32 pin muxsel
 * - u32 pin pull_up/down
 * - u32 pin driving strength
 */
#define SPACEMIT_PIN_SIZE 16

struct spacemit_pin {
	unsigned int pin_id;
	u8 muxsel;
	u8 pull;
	unsigned long config;
};

struct spacemit_group {
	const char *name;
	unsigned npins;
	unsigned *pin_ids;
	struct spacemit_pin *pins;
};

struct spacemit_regs {
	u16 cfg;
	u16 reg_len;
};

struct spacemit_pin_conf {
	u8  fs_shift;
	u8  fs_width;
	u8  od_shift;
	u8  pe_shift;
	u8  pull_shift;
	u8  ds_shift;
	u8  st_shift;
	u8  rte_shift;
};

struct spacemit_pinctrl_soc_data {
	const struct spacemit_regs *regs;
	const struct spacemit_pin_conf *pinconf;
	const struct pinctrl_pin_desc *pins;
	unsigned npins;
	struct spacemit_function *functions;
	unsigned nfunctions;
	struct spacemit_group *groups;
	unsigned ngroups;
};

int spacemit_pinctrl_probe(struct platform_device *pdev,
				struct spacemit_pinctrl_soc_data *soc);

#endif /* __PINCTRL_SPACEMIT_H */

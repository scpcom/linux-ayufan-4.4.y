#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/netdevice.h>
#include <linux/pci.h>

#include "e1000.h"

#define E1000_MAX_NIC 32

#define OPTION_UNSET   -1
#define OPTION_DISABLED 0
#define OPTION_ENABLED  1

#define COPYBREAK_DEFAULT 256
unsigned int copybreak = COPYBREAK_DEFAULT;
module_param(copybreak, uint, 0644);
MODULE_PARM_DESC(copybreak,
	"Maximum size of packet that is copied to a new buffer on receive");

#define E1000_PARAM_INIT { [0 ... E1000_MAX_NIC] = OPTION_UNSET }
#define E1000_PARAM(X, desc)					\
	static int __devinitdata X[E1000_MAX_NIC+1]		\
		= E1000_PARAM_INIT;				\
	static unsigned int num_##X;				\
	module_param_array_named(X, X, int, &num_##X, 0);	\
	MODULE_PARM_DESC(X, desc);

E1000_PARAM(TxIntDelay, "Transmit Interrupt Delay");
#define DEFAULT_TIDV 8
#define MAX_TXDELAY 0xFFFF
#define MIN_TXDELAY 0

E1000_PARAM(TxAbsIntDelay, "Transmit Absolute Interrupt Delay");
#define DEFAULT_TADV 32
#define MAX_TXABSDELAY 0xFFFF
#define MIN_TXABSDELAY 0

E1000_PARAM(RxIntDelay, "Receive Interrupt Delay");
#define DEFAULT_RDTR 0
#define MAX_RXDELAY 0xFFFF
#define MIN_RXDELAY 0

E1000_PARAM(RxAbsIntDelay, "Receive Absolute Interrupt Delay");
#define DEFAULT_RADV 8
#define MAX_RXABSDELAY 0xFFFF
#define MIN_RXABSDELAY 0

E1000_PARAM(InterruptThrottleRate, "Interrupt Throttling Rate");
#define DEFAULT_ITR 3
#define MAX_ITR 100000
#define MIN_ITR 100
 
E1000_PARAM(IntMode, "Interrupt Mode");
#define MAX_INTMODE	2
#define MIN_INTMODE	0

E1000_PARAM(SmartPowerDownEnable, "Enable PHY smart power down");

E1000_PARAM(KumeranLockLoss, "Enable Kumeran lock loss workaround");

E1000_PARAM(WriteProtectNVM, "Write-protect NVM [WARNING: disabling this can lead to corrupted NVM]");

E1000_PARAM(CrcStripping, "Enable CRC Stripping, disable if your BMC needs " \
                          "the CRC");

struct e1000_option {
	enum { enable_option, range_option, list_option } type;
	const char *name;
	const char *err;
	int def;
	union {
		struct {  
			int min;
			int max;
		} r;
		struct {  
			int nr;
			struct e1000_opt_list { int i; char *str; } *p;
		} l;
	} arg;
};

static int __devinit e1000_validate_option(unsigned int *value,
					   const struct e1000_option *opt,
					   struct e1000_adapter *adapter)
{
	if (*value == OPTION_UNSET) {
		*value = opt->def;
		return 0;
	}

	switch (opt->type) {
	case enable_option:
		switch (*value) {
		case OPTION_ENABLED:
			e_info("%s Enabled\n", opt->name);
			return 0;
		case OPTION_DISABLED:
			e_info("%s Disabled\n", opt->name);
			return 0;
		}
		break;
	case range_option:
		if (*value >= opt->arg.r.min && *value <= opt->arg.r.max) {
			e_info("%s set to %i\n", opt->name, *value);
			return 0;
		}
		break;
	case list_option: {
		int i;
		struct e1000_opt_list *ent;

		for (i = 0; i < opt->arg.l.nr; i++) {
			ent = &opt->arg.l.p[i];
			if (*value == ent->i) {
				if (ent->str[0] != '\0')
					e_info("%s\n", ent->str);
				return 0;
			}
		}
	}
		break;
	default:
		BUG();
	}

	e_info("Invalid %s value specified (%i) %s\n", opt->name, *value,
	       opt->err);
	*value = opt->def;
	return -1;
}

void __devinit e1000e_check_options(struct e1000_adapter *adapter)
{
	struct e1000_hw *hw = &adapter->hw;
	int bd = adapter->bd_number;

	if (bd >= E1000_MAX_NIC) {
		e_notice("Warning: no configuration for board #%i\n", bd);
		e_notice("Using defaults for all values\n");
	}

	{  
		const struct e1000_option opt = {
			.type = range_option,
			.name = "Transmit Interrupt Delay",
			.err  = "using default of "
				__MODULE_STRING(DEFAULT_TIDV),
			.def  = DEFAULT_TIDV,
			.arg  = { .r = { .min = MIN_TXDELAY,
					 .max = MAX_TXDELAY } }
		};

		if (num_TxIntDelay > bd) {
			adapter->tx_int_delay = TxIntDelay[bd];
			e1000_validate_option(&adapter->tx_int_delay, &opt,
					      adapter);
		} else {
			adapter->tx_int_delay = opt.def;
		}
	}
	{  
		const struct e1000_option opt = {
			.type = range_option,
			.name = "Transmit Absolute Interrupt Delay",
			.err  = "using default of "
				__MODULE_STRING(DEFAULT_TADV),
			.def  = DEFAULT_TADV,
			.arg  = { .r = { .min = MIN_TXABSDELAY,
					 .max = MAX_TXABSDELAY } }
		};

		if (num_TxAbsIntDelay > bd) {
			adapter->tx_abs_int_delay = TxAbsIntDelay[bd];
			e1000_validate_option(&adapter->tx_abs_int_delay, &opt,
					      adapter);
		} else {
			adapter->tx_abs_int_delay = opt.def;
		}
	}
	{  
		struct e1000_option opt = {
			.type = range_option,
			.name = "Receive Interrupt Delay",
			.err  = "using default of "
				__MODULE_STRING(DEFAULT_RDTR),
			.def  = DEFAULT_RDTR,
			.arg  = { .r = { .min = MIN_RXDELAY,
					 .max = MAX_RXDELAY } }
		};

		if (num_RxIntDelay > bd) {
			adapter->rx_int_delay = RxIntDelay[bd];
			e1000_validate_option(&adapter->rx_int_delay, &opt,
					      adapter);
		} else {
			adapter->rx_int_delay = opt.def;
		}
	}
	{  
		const struct e1000_option opt = {
			.type = range_option,
			.name = "Receive Absolute Interrupt Delay",
			.err  = "using default of "
				__MODULE_STRING(DEFAULT_RADV),
			.def  = DEFAULT_RADV,
			.arg  = { .r = { .min = MIN_RXABSDELAY,
					 .max = MAX_RXABSDELAY } }
		};

		if (num_RxAbsIntDelay > bd) {
			adapter->rx_abs_int_delay = RxAbsIntDelay[bd];
			e1000_validate_option(&adapter->rx_abs_int_delay, &opt,
					      adapter);
		} else {
			adapter->rx_abs_int_delay = opt.def;
		}
	}
	{  
		const struct e1000_option opt = {
			.type = range_option,
			.name = "Interrupt Throttling Rate (ints/sec)",
			.err  = "using default of "
				__MODULE_STRING(DEFAULT_ITR),
			.def  = DEFAULT_ITR,
			.arg  = { .r = { .min = MIN_ITR,
					 .max = MAX_ITR } }
		};

		if (num_InterruptThrottleRate > bd) {
			adapter->itr = InterruptThrottleRate[bd];
			switch (adapter->itr) {
			case 0:
				e_info("%s turned off\n", opt.name);
				break;
			case 1:
				e_info("%s set to dynamic mode\n", opt.name);
				adapter->itr_setting = adapter->itr;
				adapter->itr = 20000;
				break;
			case 3:
				e_info("%s set to dynamic conservative mode\n",
					opt.name);
				adapter->itr_setting = adapter->itr;
				adapter->itr = 20000;
				break;
			default:
				 
				if (e1000_validate_option(&adapter->itr, &opt,
							  adapter) &&
				    (adapter->itr == 3)) {
					 
					adapter->itr_setting = adapter->itr;
					adapter->itr = 20000;
				} else {
					 
					adapter->itr_setting =
						adapter->itr & ~3;
				}
				break;
			}
		} else {
			adapter->itr_setting = opt.def;
			adapter->itr = 20000;
		}
	}
	{  
		struct e1000_option opt = {
			.type = range_option,
			.name = "Interrupt Mode",
			.err  = "defaulting to 2 (MSI-X)",
#ifdef MY_DEF_HERE
#ifdef CONFIG_PCI_MSI
			.def  = E1000E_INT_MODE_MSIX,
#else
			.def  = E1000E_INT_MODE_LEGACY,
#endif
#else
 			.def  = E1000E_INT_MODE_MSIX,
#endif
			.arg  = { .r = { .min = MIN_INTMODE,
					 .max = MAX_INTMODE } }
		};

		if (num_IntMode > bd) {
			unsigned int int_mode = IntMode[bd];
			e1000_validate_option(&int_mode, &opt, adapter);
			adapter->int_mode = int_mode;
		} else {
			adapter->int_mode = opt.def;
		}
	}
	{  
		const struct e1000_option opt = {
			.type = enable_option,
			.name = "PHY Smart Power Down",
			.err  = "defaulting to Disabled",
			.def  = OPTION_DISABLED
		};

		if (num_SmartPowerDownEnable > bd) {
			unsigned int spd = SmartPowerDownEnable[bd];
			e1000_validate_option(&spd, &opt, adapter);
			if ((adapter->flags & FLAG_HAS_SMART_POWER_DOWN)
			    && spd)
				adapter->flags |= FLAG_SMART_POWER_DOWN;
		}
	}
	{  
		const struct e1000_option opt = {
			.type = enable_option,
			.name = "CRC Stripping",
			.err  = "defaulting to enabled",
			.def  = OPTION_ENABLED
		};

		if (num_CrcStripping > bd) {
			unsigned int crc_stripping = CrcStripping[bd];
			e1000_validate_option(&crc_stripping, &opt, adapter);
			if (crc_stripping == OPTION_ENABLED)
				adapter->flags2 |= FLAG2_CRC_STRIPPING;
		} else {
			adapter->flags2 |= FLAG2_CRC_STRIPPING;
		}
	}
	{  
		const struct e1000_option opt = {
			.type = enable_option,
			.name = "Kumeran Lock Loss Workaround",
			.err  = "defaulting to Enabled",
			.def  = OPTION_ENABLED
		};

		if (num_KumeranLockLoss > bd) {
			unsigned int kmrn_lock_loss = KumeranLockLoss[bd];
			e1000_validate_option(&kmrn_lock_loss, &opt, adapter);
			if (hw->mac.type == e1000_ich8lan)
				e1000e_set_kmrn_lock_loss_workaround_ich8lan(hw,
								kmrn_lock_loss);
		} else {
			if (hw->mac.type == e1000_ich8lan)
				e1000e_set_kmrn_lock_loss_workaround_ich8lan(hw,
								       opt.def);
		}
	}
	{  
		const struct e1000_option opt = {
			.type = enable_option,
			.name = "Write-protect NVM",
			.err  = "defaulting to Enabled",
			.def  = OPTION_ENABLED
		};

		if (adapter->flags & FLAG_IS_ICH) {
			if (num_WriteProtectNVM > bd) {
				unsigned int write_protect_nvm = WriteProtectNVM[bd];
				e1000_validate_option(&write_protect_nvm, &opt,
						      adapter);
				if (write_protect_nvm)
					adapter->flags |= FLAG_READ_ONLY_NVM;
			} else {
				if (opt.def)
					adapter->flags |= FLAG_READ_ONLY_NVM;
			}
		}
	}
}

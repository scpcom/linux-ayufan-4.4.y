#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __PINCTRL_MVEBU_H__
#define __PINCTRL_MVEBU_H__

struct mvebu_mpp_ctrl {
	const char *name;
	u8 pid;
	u8 npins;
	unsigned *pins;
	int (*mpp_get)(struct mvebu_mpp_ctrl *ctrl, unsigned long *config);
	int (*mpp_set)(struct mvebu_mpp_ctrl *ctrl, unsigned long config);
	int (*mpp_gpio_req)(struct mvebu_mpp_ctrl *ctrl, u8 pid);
	int (*mpp_gpio_dir)(struct mvebu_mpp_ctrl *ctrl, u8 pid, bool input);
};

struct mvebu_mpp_ctrl_setting {
	u8 val;
	const char *name;
	const char *subname;
	u8 variant;
	u8 flags;
#define  MVEBU_SETTING_GPO	(1 << 0)
#define  MVEBU_SETTING_GPI	(1 << 1)
};

struct mvebu_mpp_mode {
	u8 pid;
	struct mvebu_mpp_ctrl_setting *settings;
};

struct mvebu_pinctrl_soc_info {
	u8 variant;
	struct mvebu_mpp_ctrl *controls;
	int ncontrols;
	struct mvebu_mpp_mode *modes;
	int nmodes;
	struct pinctrl_gpio_range *gpioranges;
	int ngpioranges;
};

#define MPP_REG_CTRL(_idl, _idh)				\
	{							\
		.name = NULL,					\
		.pid = _idl,					\
		.npins = _idh - _idl + 1,			\
		.pins = (unsigned[_idh - _idl + 1]) { },	\
		.mpp_get = NULL,				\
		.mpp_set = NULL,				\
		.mpp_gpio_req = NULL,				\
		.mpp_gpio_dir = NULL,				\
	}

#define MPP_FUNC_CTRL(_idl, _idh, _name, _func)			\
	{							\
		.name = _name,					\
		.pid = _idl,					\
		.npins = _idh - _idl + 1,			\
		.pins = (unsigned[_idh - _idl + 1]) { },	\
		.mpp_get = _func ## _get,			\
		.mpp_set = _func ## _set,			\
		.mpp_gpio_req = NULL,				\
		.mpp_gpio_dir = NULL,				\
	}

#define MPP_FUNC_GPIO_CTRL(_idl, _idh, _name, _func)		\
	{							\
		.name = _name,					\
		.pid = _idl,					\
		.npins = _idh - _idl + 1,			\
		.pins = (unsigned[_idh - _idl + 1]) { },	\
		.mpp_get = _func ## _get,			\
		.mpp_set = _func ## _set,			\
		.mpp_gpio_req = _func ## _gpio_req,		\
		.mpp_gpio_dir = _func ## _gpio_dir,		\
	}

#define _MPP_VAR_FUNCTION(_val, _name, _subname, _mask)		\
	{							\
		.val = _val,					\
		.name = _name,					\
		.subname = _subname,				\
		.variant = _mask,				\
		.flags = 0,					\
	}

#if defined(CONFIG_DEBUG_FS)
#define MPP_VAR_FUNCTION(_val, _name, _subname, _mask)		\
	_MPP_VAR_FUNCTION(_val, _name, _subname, _mask)
#else
#define MPP_VAR_FUNCTION(_val, _name, _subname, _mask)		\
	_MPP_VAR_FUNCTION(_val, _name, NULL, _mask)
#endif

#define MPP_FUNCTION(_val, _name, _subname)			\
	MPP_VAR_FUNCTION(_val, _name, _subname, (u8)-1)

#define MPP_MODE(_id, ...)					\
	{							\
		.pid = _id,					\
		.settings = (struct mvebu_mpp_ctrl_setting[]){	\
			__VA_ARGS__, { } },			\
	}

#define MPP_GPIO_RANGE(_id, _pinbase, _gpiobase, _npins)	\
	{							\
		.name = "mvebu-gpio",				\
		.id = _id,					\
		.pin_base = _pinbase,				\
		.base = _gpiobase,				\
		.npins = _npins,				\
	}

int mvebu_pinctrl_probe(struct platform_device *pdev);
int mvebu_pinctrl_remove(struct platform_device *pdev);
#if defined(MY_ABC_HERE)
int mvebu_pinctrl_suspend(struct platform_device *pdev, pm_message_t state);
int mvebu_pinctrl_resume(struct platform_device *pdev);
#endif  

#endif

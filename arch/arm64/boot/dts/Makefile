dtb-$(CONFIG_ARCH_VEXPRESS) += rtsm_ve-aemv8a.dtb foundation-v8.dtb
dtb-$(CONFIG_ARCH_STI) += sti8416-board.dtb

targets += dtbs
ifeq ($(CONFIG_SYNO_LSP_HI3536),y)
DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif
targets += $(DTB_LIST)

dtbs: $(addprefix $(obj)/, $(DTB_LIST))

clean-files := dts/*.dtb *.dtb
else # CONFIG_SYNO_LSP_HI3536
targets += $(dtb-y)

dtbs: $(addprefix $(obj)/, $(dtb-y))

clean-files := *.dtb
endif # CONFIG_SYNO_LSP_HI3536

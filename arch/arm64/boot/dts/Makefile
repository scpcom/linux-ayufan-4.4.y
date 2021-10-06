dts-dirs += altera
dts-dirs += amd
dts-dirs += apm
dts-dirs += arm
dts-dirs += broadcom
dts-dirs += cavium
dts-dirs += exynos
dts-dirs += freescale
dts-dirs += hisilicon
dts-dirs += marvell
dts-dirs += mediatek
dts-dirs += qcom
dts-dirs += rockchip
dts-dirs += sprd
dts-dirs += xilinx
ifneq (,$(filter y, $(CONFIG_SYNO_LSP_RTD129X) $(CONFIG_SYNO_LSP_RTD1619)))
dts-dirs += realtek
endif # CONFIG_SYNO_LSP_RTD129X || CONFIG_SYNO_LSP_RTD1619

subdir-y	:= $(dts-dirs)

dtstree		:= $(srctree)/$(src)

dtb-$(CONFIG_OF_ALL_DTBS) := $(patsubst $(dtstree)/%.dts,%.dtb, $(foreach d,$(dts-dirs), $(wildcard $(dtstree)/$(d)/*.dts)))

always		:= $(dtb-y)
ifeq ($(CONFIG_SYNO_LSP_RTD1619), y)
targets += dtbs
DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM64_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif
targets += $(DTB_LIST)
dtbs: $(addprefix $(obj)/, $(DTB_LIST))
clean-files := dts/*.dtb *.dtb
endif # CONFIG_SYNO_LSP_RTD1619

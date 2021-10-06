ifeq ($(CONFIG_SYNO_LSP_RTD129X), y)
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1296-synology-DS418.dtb
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1293-synology-DS418j.dtb
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1296-synology-DS218play.dtb
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1296-synology-DS118.dtb
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1296-synology-DS218.dtb
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1296-synology-EDS19.dtb
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1296-synology-RS819.dtb
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1296-synology-DS220j.dtb
dtb-$(CONFIG_ARCH_RTD129X) += rtd-1296-synology-DS420j.dtb

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb
endif # CONFIG_SYNO_LSP_RTD129X
ifeq ($(CONFIG_SYNO_LSP_RTD1619), y)
dts-dirs += rtd16xx

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= rtd16xx/*.dtb *.dtb
endif # CONFIG_SYNO_LSP_RTD1619

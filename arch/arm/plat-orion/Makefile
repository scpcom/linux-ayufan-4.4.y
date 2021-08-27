#
# Makefile for the linux kernel.
#

obj-y	:= irq.o pcie.o time.o common.o mpp.o addr-map.o
obj-m	:=
obj-n	:=
obj-	:=

obj-$(CONFIG_GENERIC_GPIO) += gpio.o
obj-$(CONFIG_MV_PHONE)		+= mv_drivers_lsp/mv_phone/
obj-$(CONFIG_MV_CESA)		+= mv_drivers_lsp/mv_cesa/
obj-$(CONFIG_MV_TSU)		+= mv_drivers_lsp/mv_tsu/
obj-$(CONFIG_MV_INCLUDE_SWITCH)	+= mv_drivers_lsp/mv_switch/
obj-y				+= mv_drivers_lsp/mv_btns/
obj-$(CONFIG_KIRKWOOD_PROC)	+= mv_drivers_lsp/mv_proc/
obj-$(CONFIG_MV_INCLUDE_GIG_ETH) += mv_drivers_lsp/mv_network/mv_ethernet/

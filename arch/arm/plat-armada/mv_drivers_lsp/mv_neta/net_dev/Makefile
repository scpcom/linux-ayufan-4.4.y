#
# Makefile for the Marvell Gigabit Ethernet driver
#

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

obj-$(CONFIG_MV_ETH_NFP) += mv_eth_nfp.o
obj-$(CONFIG_MV_ETHERNET) += mv_netdev.o mv_ethernet.o mv_eth_sysfs.o
obj-$(CONFIG_MV_PON)      += mv_pon_sysfs.o
obj-$(CONFIG_MV_ETH_SWITCH) +=  mv_eth_switch.o
obj-$(CONFIG_MV_ETH_TOOL) += mv_eth_tool.o


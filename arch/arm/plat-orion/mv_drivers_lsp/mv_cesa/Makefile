#
# Makefile for the Marvell CESA driver
#

ifeq ($(CONFIG_ARCH_KIRKWOOD),y)
	include $(srctree)/arch/arm/mach-kirkwood/config/mvRules.mk
endif

ifeq ($(CONFIG_MV_CESA_OCF),y)
EXTRA_CFLAGS    += -I$(srctree)/crypto/ocf
endif

obj-$(CONFIG_MV_CESA_TOOL) += cesa_dev.o
obj-$(CONFIG_MV_CESA_OCF)  += cesa_ocf_drv.o

#
# Makefile for the Marvell xCat PP switch
#

ifeq ($(CONFIG_ARCH_FEROCEON_KW),y)
	include $(srctree)/arch/arm/mach-kirkwood/config/mvRules.mk
endif

obj-$(CONFIG_MV_XCAT98DX) += xcat98dx-switch.o xcat98dx-eprom.o \
			     xcat98dx-core.o xcat98dx-proc.o

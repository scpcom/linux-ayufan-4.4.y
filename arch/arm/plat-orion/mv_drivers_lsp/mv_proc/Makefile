#
# Makefile for the Marvell btns Driver
#
ifeq ($(CONFIG_ARCH_FEROCEON_ORION),y)
	include $(srctree)/arch/arm/mach-feroceon-orion/config/mvRules.mk
endif

ifeq ($(CONFIG_ARCH_FEROCEON_KW),y)
	include $(srctree)/arch/arm/mach-kirkwood/config/mvRules.mk
endif

obj-y += proc.o

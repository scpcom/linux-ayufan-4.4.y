#
# Makefile for the linux kernel.
#

KBUILD_CPPFLAGS += -Iarch/arm/mach-alpine/include/al_hal/

obj-y	+= al_hal_udma_config.o
obj-y	+= al_hal_udma_main.o
obj-y	+= al_hal_m2m_udma.o
obj-y	+= al_hal_serdes.o
obj-y   += al_hal_udma_debug.o
obj-y   += al_hal_udma_iofic.o
ifeq ($(CONFIG_SYNO_ALPINE_V2_5_3),y)
obj-y   += al_hal_udma_fast.o
endif
obj-y   += al_hal_iofic.o
ifeq ($(CONFIG_SYNO_ALPINE_V2_5_3),y)
obj-y   += al_hal_ssm.o
endif
obj-y   += al_init_sys_fabric.o
obj-$(CONFIG_PCI)	+= al_hal_pcie.o
obj-$(CONFIG_PCI)	+= al_hal_pcie_interrupts.o
obj-y	+= al_hal_exports.o
obj-$(CONFIG_EDAC_AL_MC)+= al_hal_ddr.o

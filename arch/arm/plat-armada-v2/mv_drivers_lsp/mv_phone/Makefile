

#
# Makefile for the Marvell Phone Device Driver
#
#

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

VB_SRC_PATH = ../../mv_hal/voiceband
LSP_TDM_PATH = tdm
LSP_SPI_PATH = spi
LSP_SLIC_PATH = slic
LSP_TDM_TEST_PATH = tdm/test

obj-$(CONFIG_MV_INCLUDE_TDM) += mv_phone.o

ifeq ($(CONFIG_MV_TDM_SUPPORT),y)
	PHONE_OBJS = $(VB_SRC_PATH)/tdm/mvTdm.o $(VB_SRC_PATH)/tdm/mvTdmAddrDec.o
else # CONFIG_MV_COMM_UNIT_SUPPORT
	PHONE_OBJS = $(VB_SRC_PATH)/commUnit/mvCommUnit.o $(VB_SRC_PATH)/commUnit/mvCommUnitAddrDec.o
endif

PHONE_OBJS += $(LSP_TDM_PATH)/tdm_if.o $(LSP_TDM_PATH)/tal.o ../../../../../$(MACHINE)/mv_hal_if/mvSysTdm.o
PHONE_OBJS += $(LSP_SPI_PATH)/spi_dev.o

EXTRA_CFLAGS += -DMV_KERNEL_SLIC_SUPPORT

# Lantiq support
ifeq ($(CONFIG_LANTIQ_SLIC_SUPPORT), y)
	PHONE_OBJS += $(LSP_SLIC_PATH)/drv_dxt_if.o
endif

# Silabs support
ifeq ($(CONFIG_SILABS_SLIC_SUPPORT),y)
ifeq ($(CONFIG_SILAB_SLIC_EXTERNAL_DRIVER),y)
PHONE_OBJS += $(LSP_SLIC_PATH)/silabs_if.o
endif
endif

# Zarlink support
ifeq ($(CONFIG_ZARLINK_SLIC_SUPPORT),y)
ifeq ($(CONFIG_ZARLINK_SLIC_EXTERNAL_DRIVER),y)
PHONE_OBJS += $(LSP_SLIC_PATH)/zarlink_if.o
endif
endif

mv_phone-objs := $(PHONE_OBJS)
ifeq ($(CONFIG_MV_PHONE_USE_SLIC_LIBS),y)
mv_phone-objs += lib.a
$(obj)/lib.a:
	cp $(obj)/$(LSP_SLIC_PATH)/libs/$(SLIC_LIB_NAME) $(obj)/lib.a

else
	lib-y := $(ZARLINK_OBJS)
endif

obj-y := mv_phone.o $(LSP_TDM_TEST_PATH)/

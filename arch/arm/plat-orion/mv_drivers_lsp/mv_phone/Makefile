

#
# Makefile for the Marvell Phone Device Driver
#
#

ifeq ($(CONFIG_ARCH_FEROCEON_KW),y)
	include $(srctree)/arch/arm/mach-kirkwood/config/mvRules.mk
endif

VB_SRC_PATH = ../../mv_hal/voiceband
LSP_TDM_PATH = tdm
LSP_SLIC_PATH = slic
LSP_TDM_TEST_PATH = tdm/test

obj-$(CONFIG_MV_INCLUDE_TDM) += mv_phone.o

ifeq ($(CONFIG_MV_TDM_SUPPORT),y)
	PHONE_OBJS = $(VB_SRC_PATH)/tdm/mvTdm.o
else # CONFIG_MV_COMM_UNIT_SUPPORT
	PHONE_OBJS = $(VB_SRC_PATH)/commUnit/mvCommUnit.o $(VB_SRC_PATH)/commUnit/mvCommUnitAddrDec.o
endif

PHONE_OBJS += $(LSP_TDM_PATH)/tdm_if.o $(LSP_TDM_PATH)/tal.o

EXTRA_CFLAGS += -DMV_KERNEL_SLIC_SUPPORT

# Silabs support
ifeq ($(CONFIG_SILABS_SLIC_SUPPORT),y)

	SILABS_BASE_PATH = $(VB_SRC_PATH)/slic/silabs
	SILABS_SRC_PATH = $(SILABS_BASE_PATH)/src
	SILABS_CUSTOM_PATH = $(SILABS_BASE_PATH)/custom
	SILABS_ARCH_MARVELL_PATH = $(VB_SRC_PATH)/slic/silabs/arch_marvell
	EXTRA_CFLAGS += -I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/silabs/inc \
			-I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/silabs/custom \
			-I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/silabs/arch_marvell


	SILABS_OBJS = $(SILABS_SRC_PATH)/proslic.o \
				$(SILABS_SRC_PATH)/si_voice.o $(SILABS_SRC_PATH)/si_voice_version.o

	SILABS_CUSTOM_OBJS = $(SILABS_ARCH_MARVELL_PATH)/spi.o $(SILABS_ARCH_MARVELL_PATH)/timer.o

ifeq ($(CONFIG_SILAB_SLIC_SI3226x),y)

	SILABS_OBJS += $(SILABS_SRC_PATH)/si3226x_intf.o

	SILABS_CUSTOM_OBJS += $(SILABS_CUSTOM_PATH)/si3226x_FLBK_constants.o  \
		$(SILABS_CUSTOM_PATH)/si3226x_patch_B_FB_2010SEP22.o $(SILABS_CUSTOM_PATH)/si3226x_patch_C_FB_2011MAY19.o

	SLIC_LIB_NAME=silabs_3226x.lib
endif

ifeq ($(CONFIG_SILAB_SLIC_SI3217x),y)

	SILABS_OBJS += $(SILABS_SRC_PATH)/si3217x_intf.o

	SILABS_CUSTOM_OBJS += $(SILABS_CUSTOM_PATH)/si3217x_FLBK_constants.o  \
		$(SILABS_CUSTOM_PATH)/si3217x_patch_B_FB_2010DEC16.o

	SLIC_LIB_NAME=silabs_3217x.lib
endif

PHONE_OBJS += $(LSP_SLIC_PATH)/silabs_dev.o

endif

# Zarlink support
ifeq ($(CONFIG_ZARLINK_SLIC_SUPPORT),y)

ifeq ($(CONFIG_ZARLINK_SLIC_VE880),y)
	ZARLINK_BASE_PATH = $(VB_SRC_PATH)/slic/zarlink/api_lib
	ZARLINK_COMMON_SRC_PATH = $(ZARLINK_BASE_PATH)/common
	ZARLINK_ARCH_MARVELL_PATH = $(VB_SRC_PATH)/slic/zarlink/arch_marvell
	ZARLINK_VP880API_SRC_PATH = $(ZARLINK_BASE_PATH)/vp880_api

	EXTRA_CFLAGS += -I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/zarlink/api_lib/includes \
			-I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/zarlink/arch_marvell \
			-I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/zarlink/api_lib/vp880_api

	ZARLINK_OBJS =  $(ZARLINK_COMMON_SRC_PATH)/vp_api.o  $(ZARLINK_COMMON_SRC_PATH)/vp_api_common.o \
			$(ZARLINK_COMMON_SRC_PATH)/vp_api_cslac_seq.o $(ZARLINK_COMMON_SRC_PATH)/vp_debug.o \
			$(ZARLINK_ARCH_MARVELL_PATH)/sys_service.o $(ZARLINK_ARCH_MARVELL_PATH)/vp_hal.o \
			$(ZARLINK_VP880API_SRC_PATH)/apiCal.o $(ZARLINK_VP880API_SRC_PATH)/apicnt.o \
			$(ZARLINK_VP880API_SRC_PATH)/apiInit.o $(ZARLINK_VP880API_SRC_PATH)/apiquery.o \
			$(ZARLINK_VP880API_SRC_PATH)/apiseq.o

	SLIC_LIB_NAME=zarlink_880.lib
endif

ifeq ($(CONFIG_ZARLINK_SLIC_VE792),y)

	ZARLINK_BASE_PATH = $(VB_SRC_PATH)/slic/zarlink/vp792_api_lib
	ZARLINK_COMMON_SRC_PATH = $(ZARLINK_BASE_PATH)/common
	ZARLINK_ARCH_MARVELL_PATH = $(VB_SRC_PATH)/slic/zarlink/arch_marvell
	ZARLINK_VP792API_SRC_PATH = $(ZARLINK_BASE_PATH)/vp792_api

	EXTRA_CFLAGS += -I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/zarlink/vp792_api_lib/includes \
			-I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/zarlink/arch_marvell \
			-I$(srctree)/arch/arm/plat-orion/mv_hal/voiceband/slic/zarlink/vp792_api_lib/vp792_api

	ZARLINK_OBJS =  $(ZARLINK_COMMON_SRC_PATH)/vp_api_config.o  $(ZARLINK_COMMON_SRC_PATH)/vp_api_common.o \
			$(ZARLINK_COMMON_SRC_PATH)/vp_api_init.o  $(ZARLINK_COMMON_SRC_PATH)/vp_api_query.o \
			$(ZARLINK_COMMON_SRC_PATH)/vp_api_control.o $(ZARLINK_COMMON_SRC_PATH)/vp_debug.o \
			$(ZARLINK_ARCH_MARVELL_PATH)/sys_service.o $(ZARLINK_ARCH_MARVELL_PATH)/vp_hal.o \
			$(ZARLINK_VP792API_SRC_PATH)/vp792_common.o $(ZARLINK_VP792API_SRC_PATH)/vp792_config.o \
			$(ZARLINK_VP792API_SRC_PATH)/vp792_control.o $(ZARLINK_VP792API_SRC_PATH)/vp792_init.o \
			$(ZARLINK_VP792API_SRC_PATH)/vp792_query.o $(ZARLINK_VP792API_SRC_PATH)/vp792_firmware.o \
			$(ZARLINK_ARCH_MARVELL_PATH)/Le71HP0410G_init.o

	SLIC_LIB_NAME=zarlink_792.lib

endif

PHONE_OBJS += $(LSP_SLIC_PATH)/vpapi_dev.o

endif

ifeq ($(CONFIG_MV_PHONE_USE_SLIC_LIBS),y)
$(obj)/lib.a:
	cp $(obj)/$(LSP_SLIC_PATH)/libs/$(SLIC_LIB_NAME) $(obj)/lib.a
else
	lib-y := $(ZARLINK_OBJS) $(SILABS_OBJS)
endif

obj-y := mv_phone.o $(LSP_TDM_TEST_PATH)/

mv_phone-objs := $(PHONE_OBJS) $(SILABS_CUSTOM_OBJS) lib.a

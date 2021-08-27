
#
# Makefile for the Marvell Phone Device Driver
#
#

ifeq ($(CONFIG_PLAT_ARMADA),y)
	include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

VB_SRC_PATH = ../../mv_hal/voiceband
LSP_TDM_PATH = tdm
LSP_SLIC_PATH = slic
LSP_TDM_TEST_PATH = tdm/test

obj-$(CONFIG_MV_INCLUDE_TDM) += mv_phone.o

ifeq ($(CONFIG_MV_TDM_SUPPORT),y)
	PHONE_OBJS = $(VB_SRC_PATH)/tdm/mvTdm.o $(VB_SRC_PATH)/tdm/mvTdmAddrDec.o
else # CONFIG_MV_COMM_UNIT_SUPPORT
	PHONE_OBJS = $(VB_SRC_PATH)/commUnit/mvCommUnit.o $(VB_SRC_PATH)/commUnit/mvCommUnitAddrDec.o
endif

PHONE_OBJS += $(LSP_TDM_PATH)/tdm_if.o $(LSP_TDM_PATH)/tal.o ../../../../../$(MACHINE)/mv_hal_if/mvSysTdm.o

EXTRA_CFLAGS += -DMV_KERNEL_SLIC_SUPPORT

# Silabs support
ifeq ($(CONFIG_SILABS_SLIC_SUPPORT),y)

	SILABS_BASE_PATH = $(VB_SRC_PATH)/slic/silabs/
	SILABS_SRC_PATH = $(SILABS_BASE_PATH)/src
	SILABS_CUSTOM_PATH = $(SILABS_BASE_PATH)/custom
	EXTRA_CFLAGS += -I$(srctree)/$(MACHINE)/../plat-armada/mv_hal/voiceband/slic/silabs/inc \
			-I$(srctree)/$(MACHINE)/../plat-armada/mv_hal/voiceband/slic/silabs/custom


SILABS_OBJS = $(SILABS_SRC_PATH)/proslic.o $(SILABS_SRC_PATH)/proslic_version.o

ifeq ($(CONFIG_SILABS_SLIC_3215),y)
	SILABS_OBJS += $(SILABS_CUSTOM_PATH)/si321x_constants.o $(SILABS_SRC_PATH)/si321x_intf.o
endif

ifeq ($(CONFIG_SILABS_SLIC_3217),y)
	SILABS_OBJS += $(SILABS_SRC_PATH)/si3217x_intf.o $(SILABS_CUSTOM_PATH)/si3217x_patch_B_BB_2009MAY22.o \
		       $(SILABS_CUSTOM_PATH)/si3217x_BKBT_constants.o  #$(SILABS_CUSTOM_PATH)/si3217x_FLBK_constants.o
endif

endif

# Zarlink support
ifeq ($(CONFIG_ZARLINK_SLIC_SUPPORT),y)

ifeq ($(CONFIG_ZARLINK_SLIC_VE880),y)
	ZARLINK_BASE_PATH = $(VB_SRC_PATH)/slic/zarlink/api_lib
	ZARLINK_COMMON_SRC_PATH = $(ZARLINK_BASE_PATH)/common
	ZARLINK_ARCH_MARVELL_PATH = $(VB_SRC_PATH)/slic/zarlink/arch_marvell
	ZARLINK_VP880API_SRC_PATH = $(ZARLINK_BASE_PATH)/vp880_api

	EXTRA_CFLAGS += -I$(srctree)/$(MACHINE)/../plat-armada/mv_hal/voiceband/slic/zarlink/api_lib/includes \
			-I$(srctree)/$(MACHINE)/../plat-armada/mv_hal/voiceband/slic/zarlink/arch_marvell \
			-I$(srctree)/$(MACHINE)/../plat-armada/mv_hal/voiceband/slic/zarlink/api_lib/vp880_api

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

	EXTRA_CFLAGS += -I$(srctree)/$(MACHINE)/../plat-armada/mv_hal/voiceband/slic/zarlink/vp792_api_lib/includes \
			-I$(srctree)/$(MACHINE)/../plat-armada/mv_hal/voiceband/slic/zarlink/arch_marvell \
			-I$(srctree)/$(MACHINE)/../plat-armada/mv_hal/voiceband/slic/zarlink/vp792_api_lib/vp792_api

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
	lib-$(CONFIG_ZARLINK_SLIC_SUPPORT) := $(ZARLINK_OBJS) $(SILABS_OBJS)
endif

obj-y := mv_phone.o $(LSP_TDM_TEST_PATH)/

mv_phone-objs := $(PHONE_OBJS) lib.a

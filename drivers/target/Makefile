#
#
#

VAAI_OBJ=  vaai_ats.o \
           vaai_ws.o \
           vaai_xcopy.o \
           vaai_xcopy_helper.o \
           vaai_helper.o \
           vaai_fast_ws.o \

TP_OBJ = vaai_bp.o

TPC_OBJ =  tpc_vpd.o \
           tpc_rod.o \
           tpc_helper.o \

FAST_CLONE_OBJ	=	target_fast_clone.o

target_core_mod-y		:= target_core_configfs.o \
				   target_core_device.o \
				   target_core_fabric_configfs.o \
				   target_core_fabric_lib.o \
				   target_core_hba.o \
				   target_core_pr.o \
				   target_core_alua.o \
				   target_core_tmr.o \
				   target_core_tpg.o \
				   target_core_transport.o \
				   target_core_cdb.o \
				   target_core_ua.o \
				   target_core_rd.o \
				   target_core_stat.o \
				   target_core_extern.o \
				   tpc_general.o \
				   div64.o

##########################################################################
#
# SUPPORT_VAAI            - enable the VAAI
# TARGET_CORE_VER         - Indicate the target core version. The purpose for it,  please 
#                           refer the vaai_comp_opt.h.
#
#                           <Format> TARGET_CORE_VER=0xAABBCCDD
#
#                           If LIO ver is 3.5.0, then AA=0x03, BB=0x05, CC=0x00, DD=0x00
#                           If LIO ver is 4.1.0.rc1, then AA=0x04, BB=0x01, CC=0x00, DD=0x01
#
# ENABLE_DBG_PRINT        - enable the debug print
#
##########################################################################

ccflags-$(CONFIG_MACH_QNAPTS) += -I$(FBDISK_INCLUDE)

ifeq ($(SUPPORT_TP),yes)
target_core_mod-y	+= $(TP_OBJ)
endif
ifeq ($(SUPPORT_VAAI),yes)
target_core_mod-y	+= $(VAAI_OBJ)
endif

ifeq ($(SUPPORT_TPC_CMD),yes)
target_core_mod-y	+= $(TPC_OBJ)
endif

ifeq ($(SUPPORT_FAST_BLOCK_CLONE),yes)
target_core_mod-y	+= $(FAST_CLONE_OBJ)
endif


#ccflags-y += -DSUPPORT_VAAI -DTARGET_CORE_VER=0x04010002 -DENABLE_DBG_PRINT=0
# 20121130, adam hsu support iscsi zero-copy
ccflags-y +=  -DSUPPORT_ISCSI_ZERO_COPY
# 20130108, kenny tai disable/enable fileio discard for fbdisk, default is marked
#ccflags-y +=  -DDISABLE_FILEIO_DISCARD

# 20121221, adam hsu
#
# To use SUPPORT_QNAP_IEEE_COMPANY_ID to separate some code in fw 3.8.x and 3.9.0
#
ccflags-y +=  -DSUPPORT_QNAP_IEEE_COMPANY_ID
#endif
#
#
obj-$(CONFIG_TARGET_CORE)	+= target_core_mod.o

# Subsystem modules
obj-$(CONFIG_TCM_IBLOCK)	+= target_core_iblock.o
obj-$(CONFIG_TCM_FILEIO)	+= target_core_file.o
obj-$(CONFIG_TCM_PSCSI)		+= target_core_pscsi.o

# Fabric modules
obj-$(CONFIG_LOOPBACK_TARGET)	+= loopback/
obj-$(CONFIG_TCM_FC)		+= tcm_fc/
obj-$(CONFIG_ISCSI_TARGET)	+= iscsi/

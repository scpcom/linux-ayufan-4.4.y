#
# Makefile for the Marvell Gpon driver
#
ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ifdef CONFIG_MV_HAL_RULES_PATH
include $(srctree)/include/config/auto.conf
include $(srctree)/$(subst ",,$(CONFIG_MV_HAL_RULES_PATH))
endif

ifeq ($(CONFIG_MV_GPON),m)
PON_OBJS += ../../mv_hal/pon/mvPonOnuRegs.o \
            ../../mv_hal/pon/mvPonOnuMac.o \
            core/gpon/ponOnuAlloc.o \
            core/gpon/ponOnuAlrm.o \
	    core/gpon/ponOnuApi.o \
	    core/gpon/ponOnuBer.o \
            core/gpon/ponOnuDb.o \
	    core/gpon/ponOnuInit.o \
	    core/gpon/ponOnuIsr.o \
	    core/gpon/ponOnuMngr.o \
	    core/gpon/ponOnuMngrStateMachine.o \
            core/gpon/ponOnuPm.o \
	    core/gpon/ponOnuSrvc.o \
	    plat/ponOnuBoard.o \
	    plat/ponOnuLnxKsOs.o \
	    perf/ponOnuLnxKsUI.o \
            perf/ponOnuLnxKsMI.o \
	    perf/ponOnuBringUp.o


mv_gpon-objs := $(PON_OBJS)

obj-m := mv_gpon.o

endif
ifeq ($(CONFIG_MV_EPON),m)

PON_OBJS += ../../mv_hal/pon/mvPonOnuRegs.o \
            ../../mv_hal/pon/mvPonOnuMac.o \
            core/epon/ponOnuAlrm.o \
	    core/epon/ponOnuApi.o \
            core/epon/ponOnuDb.o \
	    core/epon/ponOnuInit.o \
	    core/epon/ponOnuIsr.o \
	    core/epon/ponOnuMngr.o \
            core/epon/ponOnuPm.o \
	    plat/ponOnuBoard.o \
	    plat/ponOnuLnxKsOs.o \
	    perf/ponOnuLnxKsUI.o \
            perf/ponOnuLnxKsMI.o \
	    perf/ponOnuBringUp.o

mv_epon-objs := $(PON_OBJS)

obj-m := mv_epon.o

endif


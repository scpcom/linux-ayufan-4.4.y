#
# Makefile for the Marvell IPC driver
#

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

#kernel<->kernel AMP mode
obj-$(CONFIG_MV_IPC_LINUX_AMP_DRIVER) += linux_amp/

#kernel<->freeRTOS AMP mode
obj-$(CONFIG_MV_IPC_FREERTOS_DRIVER) += linux_freertos/mv_ipc_os.o linux_freertos/mv_ipc_doorbell.o \
								linux_freertos/mv_ipc_poll.o linux_freertos/mv_ipc_common.o \
								linux_freertos/ipc_demo.o

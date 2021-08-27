#
# Makefile for the Marvell Linux IPC layer
#

ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

#kernel<->kernel AMP mode
obj-$(CONFIG_MV_IPC_LINUX_AMP_DRIVER) += mv_ipc_os.o mv_ipc_doorbell.o mv_ipc_poll.o mv_ipc_common.o

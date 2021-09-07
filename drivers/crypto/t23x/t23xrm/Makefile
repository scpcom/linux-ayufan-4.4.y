#
# 2.6 makefile for T2/3 extensible crypto driver resource manager
#
# Valid EXTRA_CFLAGS are (-Dnnn):
#     RM_DBG_APITRACE - view requests through all RM interface entry points
#     RM_DBG_DPDVIEW  - dump all descriptors right before initiation
#     RM_DBG_DEVID    - show the ID of the device at startup
#     RM_DBG_EXTENDED_ERROR - show in-depth information on error

#EXTRA_CFLAGS += -DRM_DBG_DEVID -DRM_DBG_APITRACE -DRM_DBG_DPDVIEW -DRM_DBG_EXTENDED_ERROR
EXTRA_CFLAGS += -DRM_DBG_DEVID

obj-$(CONFIG_CRYPTO_DEV_EXTENS_T23X) += t23xrm.o

t23xrm-objs := t23xrmBase.o t23xrmModule.o t23xrmISR.o t23xrmIFreg.o\
    t23xrmRequest.o t23xrmResponse.o t23xrmID.o t23xrmMemUtil.o

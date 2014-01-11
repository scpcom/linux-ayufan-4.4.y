#
# Makefile for DWC_otg Highspeed USB controller driver
#

ifeq ($(CONFIG_DWC_OTG_DEBUG),y)
endif

# Use one of the following flags to compile the software in host-only or
# device-only mode.
ifeq ($(CONFIG_DWC_OTG_HOST_ONLY),y)
EXTRA_CFLAGS   += -DDWC_HOST_ONLY
EXTRA_CFLAGS   += -DDWC_EN_ISOC
EXTRA_CFLAGS   += -DDWC_LINUX
EXTRA_CFLAGS	+= -DDWC_LIBMODULE
endif

ifeq ($(CONFIG_DWC_OTG_DEVICE_ONLY),y)
EXTRA_CFLAGS   += -DDWC_DEVICE_ONLY
endif

obj-$(CONFIG_DWC_OTG)	:= dwc_otg.o

dwc_otg-objs	:= dwc_otg_driver.o dwc_otg_attr.o
dwc_otg-objs	+= dwc_otg_cil.o dwc_otg_cil_intr.o
dwc_otg-objs	+= dwc_otg_hcd_linux.o dwc_otg_adp.o dwc_otg_cfi.o 
dwc_otg-objs	+= dwc_otg_hcd_ddma.o
dwc_otg-objs	+= dwc_otg_hcd.o dwc_otg_hcd_intr.o dwc_otg_hcd_queue.o
dwc_otg-objs	+= dwc_common_linux.o dwc_mem.o

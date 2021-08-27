#
# Makefile for the Marvell USB device controller
#
ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

USB_PATH := $(srctree)/$(MACHINE)/../plat-armada/mv_drivers_lsp/mv_udc/usb_hal/

USB_DEV_DIR := ./usb_hal/device

EXTRA_CFLAGS += -I$(USB_PATH) -I$(USB_PATH)/api -I$(USB_PATH)/common -I$(USB_PATH)/device

OBJS        := mv_udc_main.o $(USB_DEV_DIR)/mvUsbDevCh9.o $(USB_DEV_DIR)/mvUsbDevMain.o 	\
               $(USB_DEV_DIR)/mvUsbDevRecv.o $(USB_DEV_DIR)/mvUsbDevSend.o $(USB_DEV_DIR)/mvUsbDevUtl.o 	\
	       $(USB_DEV_DIR)/mvUsbHsDevUtl.o  $(USB_DEV_DIR)/mvUsbHsDevMain.o $(USB_DEV_DIR)/mvUsbHsDevCncl.o

mv_udc-objs := $(OBJS)
obj-m := mv_udc.o


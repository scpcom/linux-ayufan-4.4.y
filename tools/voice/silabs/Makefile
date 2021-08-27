# kernel sources
srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/../../..
endif

include $(KSRC)/.config

CFLAGS  := $(EXTRA_CFLAGS)

ifdef CONFIG_CPU_BIG_ENDIAN
CFLAGS  += -DCONFIG_CPU_BIG_ENDIAN
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
AR := $(CROSS_COMPILE)ar

#CFLAGS += -I$(KSRC)/tools/voice/

ifeq ($(CONFIG_SILABS_SLIC_SUPPORT),y)
ifeq ($(CONFIG_SILAB_SLIC_EXTERNAL_DRIVER),y)

CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_phone/
CFLAGS += -I$(SILABS_SLIC)/proslic_api/inc -I$(SILABS_SLIC)/proslic_api/custom
CFLAGS += -I$(SILABS_SLIC)/silabs_dev

all: mv_voice_tool-si3217x mv_voice_tool-si3226x

mv_voice_tool-si3217x: mv_voice_tool.c silabs_lib.c
	$(CC) $(CFLAGS) -DMV_TDM_USE_DCO -DSI3217x -o $@ $^

mv_voice_tool-si3226x: mv_voice_tool.c silabs_lib.c
	$(CC) $(CFLAGS) -DMV_TDM_USE_DCO -DSI3226x -o $@ $^

clean:
	rm -f mv_voice_tool-si3217x mv_voice_tool-si3226x

endif #CONFIG_SILAB_SLIC_EXTERNAL_DRIVER
endif #CONFIG_SILABS_SLIC_SUPPORT

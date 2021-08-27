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
TARGETS += siapi_lib.o
LIBS := libsiapi.a
TOOL := mv_voice_tool.c
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_hal/voiceband/slic/silabs/arch_marvell/
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_phone/
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_hal/voiceband/slic/silabs/inc
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_hal/voiceband/slic/silabs/custom/
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_phone/test/

ifeq ($(CONFIG_SILAB_SLIC_SI3217x),y)
 CFLAGS += -DSI3217x
endif
ifeq ($(CONFIG_SILAB_SLIC_SI3226x),y)
 CFLAGS += -DSI3226x
endif

ifeq ($(CONFIG_MV_TDM_USE_DCO),y)
 CFLAGS += -DMV_TDM_USE_DCO
endif

CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_phone/test/

all:
	$(CC) $(CFLAGS) -static -c silabs_lib.c -o $(TARGETS)
	$(AR) rcs $(LIBS) $(TARGETS)
	$(CC) $(CFLAGS) -static $(PROFILE) $(TOOL) -L. $(LIBS) -o mv_voice_tool

mv_voice_tool: $(PROFILE) $(TOOL)
	$(CC) $(CFLAGS) -static $(PROFILE) $(TOOL) -L. $(LIBS) -o mv_voice_tool

clean:
	rm -f *.o *.a mv_voice_tool
	@for subdir in $(SUBDIRS); do \
                (cd $$subdir && $(MAKE) $@) || exit 1; \
        done
endif #CONFIG_SILABS_SLIC_SUPPORT

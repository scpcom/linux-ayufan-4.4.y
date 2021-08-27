# kernel sources
srctree  := $(shell /bin/pwd)

ifndef KSRC
KSRC  := $(srctree)/..
endif

include $(KSRC)/.config

CFLAGS  := $(EXTRA_CFLAGS)

ifdef CONFIG_CPU_BIG_ENDIAN
CFLAGS  += -DCONFIG_CPU_BIG_ENDIAN
endif

CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

SUBDIRS +=

TARGETS=mv_shell

ifeq ($(CONFIG_MV_GATEWAY),y)
CFLAGS += -DCONFIG_MV_GATEWAY
endif

ifeq ($(CONFIG_MV_ETHERNET),y)
CFLAGS += -DCONFIG_MV_ETHERNET
endif

ifeq ($(CONFIG_MV_PON),y)
TARGETS+=mv_omci_tool
CFLAGS +=
endif

ifeq ($(CONFIG_MV_ETH_LEGACY),y)
TARGETS+=mv_eth_tool
CFLAGS += -DCONFIG_MV_ETH_LEGACY
ifeq ($(CONFIG_PLAT_ORION),y)
CFLAGS += -I$(KSRC)/arch/arm/plat-orion/mv_drivers_lsp/mv_network/mv_ethernet/
else
CFLAGS += -I$(KSRC)/arch/arm/plat-feroceon/mv_drivers_lsp/mv_network/mv_ethernet/
endif
endif

ifeq ($(CONFIG_MV_CPU_PERF_CNTRS),y)
CFLAGS += -DCONFIG_MV_CPU_PERF_CNTRS
endif

ifeq ($(CONFIG_MV_CESA),y)
TARGETS += mv_cesa_tool
ifeq ($(CONFIG_PLAT_ORION),y)
CFLAGS  += -I$(KSRC)/arch/arm/plat-orion/mv_drivers_lsp/mv_cesa/
else
CFLAGS  += -I$(KSRC)/arch/arm/plat-armada/mv_drivers_lsp/mv_cesa/
endif
endif

ifeq ($(CONFIG_MV_INCLUDE_USB),y)
TARGETS += testusb
endif

ifeq ($(CONFIG_MV_PHONE),y)
SUBDIRS+=voice
endif

all : Makefile $(TARGETS)
	@for subdir in $(SUBDIRS); do \
                (cd $$subdir && $(MAKE) $@) || exit 1; \
        done

mv_shell : mv_shell.c
	$(CC) $(CFLAGS) -static mv_shell.c -o mv_shell

mv_eth_tool: mv_eth_tool.c
	$(CC) $(CFLAGS) -static mv_eth_tool.c -o mv_eth_tool

mv_set_coal : mv_set_coal.c
	$(CC) $(CFLAGS) mv_set_coal.c -o mv_set_coal

mv_cesa_tool:mv_cesa_tool.c
	$(CC) $(CFLAGS) -static mv_cesa_tool.c -o mv_cesa_tool

testusb: testusb.c
	$(CC) $(CFLAGS) -static testusb.c -o testusb -lpthread

clean:
	rm -f $(TARGETS) *.o
	@for subdir in $(SUBDIRS); do \
                (cd $$subdir && $(MAKE) $@) || exit 1; \
        done

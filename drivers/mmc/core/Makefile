#
# Makefile for the kernel mmc core.
#

obj-$(CONFIG_MMC)		+= mmc_core.o
mmc_core-y			:= core.o bus.o host.o \
				   mmc.o mmc_ops.o sd.o sd_ops.o \
				   sdio.o sdio_ops.o sdio_bus.o \
				   sdio_cis.o sdio_io.o sdio_irq.o \
				   quirks.o slot-gpio.o
ifeq ($(CONFIG_SYNO_LSP_ARMADA_17_10), y)
mmc_core-$(CONFIG_OF)		+= pwrseq.o pwrseq_simple.o pwrseq_emmc.o pwrseq_a3700_sd.o
else # CONFIG_SYNO_LSP_ARMADA_17_10
mmc_core-$(CONFIG_OF)		+= pwrseq.o pwrseq_simple.o pwrseq_emmc.o
endif # CONFIG_SYNO_LSP_ARMADA_17_10  
mmc_core-$(CONFIG_DEBUG_FS)	+= debugfs.o

ifeq ($(CONFIG_SYNO_LSP_RTD129X), y)
#
# Makefile for realtek specific clk
#

#obj-y += clk-rtk.o clk-rtk-debugfs.o clk-factors.o  
obj-$(CONFIG_COMMON_CLK_RTK) += clk-generic.o 
obj-$(CONFIG_COMMON_CLK_RTD129X) += clk-rtd129x.o
endif # CONFIG_SYNO_LSP_RTD129X

ifeq ($(CONFIG_SYNO_LSP_RTD1619), y)
ccflags-y += -I$(src)/include
ccflags-$(CONFIG_COMMON_CLK_RTD119X) += -I$(src)/include/rtd119x/
ccflags-$(CONFIG_COMMON_CLK_RTD129X) += -I$(src)/include/rtd129x/
ccflags-$(CONFIG_COMMON_CLK_RTD139X) += -I$(src)/include/rtd139x/
ccflags-$(CONFIG_COMMON_CLK_RTD16XX) += -I$(src)/include/rtd161x/

obj-$(CONFIG_COMMON_CLK_REALTEK) += clk-rtk.o
obj-$(CONFIG_COMMON_CLK_DUMMY) += clk-dummy.o

clk-rtk-y += common.o
clk-rtk-y += clk-mmio-mux.o
clk-rtk-y += clk-mmio-gate.o
clk-rtk-y += cgc.o
clk-rtk-y += clk-pll.o
clk-rtk-y += cc-platform.o
clk-rtk-y += reset.o
clk-rtk-$(CONFIG_RESET_MUX) += reset-mux.o
clk-rtk-$(CONFIG_COMMON_CLK_RTD119X) += cc-rtd119x.o
clk-rtk-$(CONFIG_COMMON_CLK_RTD129X) += cc-rtd129x.o
clk-rtk-$(CONFIG_COMMON_CLK_RTD139X) += cc-rtd139x.o
clk-rtk-$(CONFIG_COMMON_CLK_RTD16XX) += cc-rtd16xx.o
endif # CONFIG_SYNO_LSP_RTD1619

# SPDX-License-Identifier: GPL-2.0
#
# Makefile for sunxi specific clk
#

obj-$(CONFIG_CLK_SUNXI_DDR) += clk-ddr.o

obj-$(CONFIG_CLK_SUN8IW15) += clk.o
ifneq ($(CONFIG_CLK_SUN8IW15),)
clk-y += clk-sunxi.o clk-factors.o clk-periph.o clk-cpu.o clk-sun8iw15.o
endif

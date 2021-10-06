#
# Makefile for the i2c algorithms
#

obj-$(CONFIG_I2C_ALGOBIT)	+= i2c-algo-bit.o
obj-$(CONFIG_I2C_ALGOPCF)	+= i2c-algo-pcf.o
obj-$(CONFIG_I2C_ALGOPCA)	+= i2c-algo-pca.o
ifeq ($(CONFIG_SYNO_LSP_RTD1619),y)
obj-$(CONFIG_I2C_PHOENIX)	+= i2c-algo-phoenix.o
endif # CONFIG_SYNO_LSP_RTD1619

ccflags-$(CONFIG_I2C_DEBUG_ALGO) := -DDEBUG

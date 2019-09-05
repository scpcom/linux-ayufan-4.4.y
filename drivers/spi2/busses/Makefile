SPI2_MSPD=n
ifeq ($(CONFIG_SPI2_MSPD_HIGH_SPEED),m)
    SPI2_MSPD=m
endif
ifeq ($(CONFIG_SPI2_MSPD_LOW_SPEED),m)
    SPI2_MSPD=m
endif
ifeq ($(CONFIG_SPI2_MSPD_HIGH_SPEED),y)
    SPI2_MSPD=y
endif
ifeq ($(CONFIG_SPI2_MSPD_LOW_SPEED),y)
    SPI2_MSPD=y
endif

obj-$(SPI2_MSPD)        += comcerto_spi.o

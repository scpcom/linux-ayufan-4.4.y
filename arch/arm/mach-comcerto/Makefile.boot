ifeq ($(CONFIG_ARCH_M86XXX),y)
ifeq ($(CONFIG_COMCERTO_ZONE_DMA_NCNB),y)
   zreladdr-y     := 0x04008000
else
   zreladdr-y     := 0x00008000
endif
   params_phys-y  := 0x00000100
endif

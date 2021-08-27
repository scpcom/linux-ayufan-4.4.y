ifdef CONFIG_MV_AMP_ENABLE
   zreladdr-y   := $(CONFIG_MV_ZREL_ADDR)
params_phys-y   := $(CONFIG_MV_PARAM_PHYS)
initrd_phys-y   := $(CONFIG_MV_INITRD_PHYS)
else
    zreladdr-y	:= 0x00008000
 params_phys-y	:= 0x00000100
 initrd_phys-y	:= 0x00800000
endif

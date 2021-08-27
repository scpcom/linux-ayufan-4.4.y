KBUILD_CPPFLAGS += -Iarch/arm/mach-alpine/include/al_hal/

obj-$(CONFIG_NET_AL_ETH) += al_eth_drv.o
ifeq ($(CONFIG_SYNO_ALPINE_A0),y)
al_eth_drv-objs := al_eth.o al_eth_sysfs.o al_hal_eth_main.o al_hal_eth_kr.o al_init_eth_kr.o al_init_eth_lm.o
else
al_eth_drv-objs := al_eth.o al_eth_sysfs.o al_hal_eth_main.o al_hal_eth_kr.o al_init_eth_kr.o
endif

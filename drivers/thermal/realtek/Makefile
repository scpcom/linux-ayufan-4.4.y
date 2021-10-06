ifeq ($(CONFIG_SYNO_LSP_RTD129X), y)
obj-$(CONFIG_RTK_THERMAL) += rtk_thermal.o
obj-$(CONFIG_RTK_THERMAL_CPU_CORE_COOLING) += rtk_cpu_core_cooling.o
endif # CONFIG_SYNO_LSP_RTD129X
ifeq ($(CONFIG_SYNO_LSP_RTD1619), y)
obj-$(CONFIG_RTK_THERMAL) += rtk-thermal.o

rtk-thermal-y += sensor.o
rtk-thermal-$(CONFIG_RTK_THERMAL_RTD119X) += sensor-rtd119x.o
rtk-thermal-$(CONFIG_RTK_THERMAL_RTD129X) += sensor-rtd129x.o
rtk-thermal-$(CONFIG_RTK_THERMAL_RTD139X) += sensor-rtd139x.o
rtk-thermal-$(CONFIG_RTK_THERMAL_RTD16XX) += sensor-rtd16xx.o
rtk-thermal-$(CONFIG_RTK_THERMAL_RTD13XX) += sensor-rtd13xx.o
rtk-thermal-$(CONFIG_RTK_THERMAL_EOH) += sensor-eoh.o
endif # CONFIG_SYNO_LSP_RTD1619

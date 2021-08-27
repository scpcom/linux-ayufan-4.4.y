ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

# This will never compile, because DUMMY will never by defined.
obj-$(DUMMY)   				:= dummy.o
obj-$(CONFIG_SHEEVA_DEEP_IDLE)		+= cpuidle.o armada_suspend.o suspend.o
obj-$(CONFIG_HOTPLUG_CPU)		+= hotplug.o
obj-$(CONFIG_PLAT_ARMADA)		+= pmu.o
obj-$(CONFIG_PCI_MSI)			+= msi.o
obj-$(CONFIG_CPU_FREQ_ARMADA_XP)	+= cpufreq.o

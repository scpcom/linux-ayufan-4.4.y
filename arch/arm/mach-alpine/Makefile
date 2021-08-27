#
# Makefile for the linux kernel.
#

ccflags-$(CONFIG_ARCH_MULTIPLATFORM) := -I$(srctree)/$(src)/include \
        -I$(srctree)/arch/arm/plat-orion/include

ifeq ($(CONFIG_SYNO_ALPINE_V2_5_3),y)
KBUILD_CPPFLAGS += -Iarch/arm/mach-alpine/include/al_hal \
				   -Iarch/arm/mach-alpine/include/mach
else
KBUILD_CPPFLAGS += -Iarch/arm/mach-alpine/include/al_hal
endif

obj-y					+= alpine_machine.o
obj-y					+= sched-clock.o
obj-y					+= clock.o
obj-y					+= alpine_cpu_pm.o
obj-$(CONFIG_PCI)			+= pcie_of.o
obj-y					+= al_fabric.o
obj-y 					+= al_hal/
obj-y 					+= al_patches/
obj-$(CONFIG_SMP)			+= platsmp.o
obj-$(CONFIG_HOTPLUG_CPU)		+= sleep-alpine.o hotplug.o
obj-$(CONFIG_PCI_MSI)			+= msix.o

# only build sleep-alpine once
ifneq ($(CONFIG_HOTPLUG_CPU), y)
obj-$(CONFIG_CPU_IDLE_ALPINE)   += sleep-alpine.o
endif

ifeq ($(CONFIG_ARCH_ALPINE),y)
obj-y					+= synology-gpio.o
endif

#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y := gpio.o time.o dma.o clock.o reset.o pm.o
obj-m :=
obj-n :=
obj- :=

obj-$(CONFIG_ARM_CPU_SUSPEND)			+= sleep.o
obj-$(CONFIG_ARCH_M86XXX) 		    	+= comcerto-2000.o
obj-$(CONFIG_ARCH_M86XXX) 		    	+= comcerto-vwd.o

obj-$(CONFIG_ARCH_M86XXX)               	+= serdes-c2000.o

ifeq ($(CONFIG_PCI),y)
        obj-$(CONFIG_ARCH_M86XXX)               += pcie-c2000.o
endif

obj-$(CONFIG_COMCERTO_MEMBUF)                   += membuf.o
obj-$(CONFIG_COMCERTO_TDM_CLOCK)                += sysfstdm.o
obj-$(CONFIG_RTSM_C2K) 				+= board-c2krtsm.o
obj-$(CONFIG_C2K_EVM) 				+= board-c2kevm.o
obj-$(CONFIG_C2K_MFCN_EVM)			+= board-c2kmfcnevm.o
obj-$(CONFIG_C2K_ASIC) 				+= board-c2kasic.o
obj-$(CONFIG_SEQUOIA)				+= board-sequoia.o
obj-$(CONFIG_SMP)				+= platsmp.o headsmp.o
obj-$(CONFIG_HOTPLUG_CPU)			+= hotplug.o
obj-$(CONFIG_COMCERTO_CSYS_TPI_CLOCK)		+= comcerto-tpicsys.o

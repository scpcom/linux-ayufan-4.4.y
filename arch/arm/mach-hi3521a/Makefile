#
# Makefile for the linux kernel.
#

KBUILD_AFLAGS	:=$(KBUILD_AFLAGS:-msoft-float=-Wa,-mfpu=softvfp+vfp)
LDFLAGS		+=--no-warn-mismatch

obj-y					:= core.o clock.o timer.o setup.o
obj-$(CONFIG_HW_PERF_EVENTS)		+= pmu.o


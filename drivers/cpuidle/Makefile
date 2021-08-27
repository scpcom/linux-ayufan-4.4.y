#
# Makefile for cpuidle.
#
ccflags-$(CONFIG_CPU_IDLE_ALPINE) :=  		\
		-I$(srctree)/arch/arm/mach-alpine/include/al_hal


obj-y += cpuidle.o driver.o governor.o sysfs.o governors/
obj-$(CONFIG_CPU_IDLE_ALPINE) += cpuidle-alpine.o

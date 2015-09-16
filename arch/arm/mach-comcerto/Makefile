ccflags-$(CONFIG_ARCH_MULTIPLATFORM) := -I$(srctree)/$(src)/include

obj-y := board-generic.o comcerto-restart.o common.o io.o clk-comcerto2000.o \
         platsmp.o comcerto-vwd.o

obj-$(CONFIG_SMP)			+= headsmp.o
AFLAGS_headsmp.o			:=-Wa,-march=armv7-a

ccflags-$(CONFIG_DEBUG_DRIVER)	:= -DDEBUG
obj-y				+= core.o cpu.o
ifeq ($(CONFIG_SYNO_RTD1619), y)
obj-$(CONFIG_DEBUG_FS)		+= debugfs.o
endif # CONFIG_SYNO_RTD1619

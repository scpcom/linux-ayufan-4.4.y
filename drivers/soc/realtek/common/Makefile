ifeq ($(CONFIG_SYNO_LSP_RTD1619), y)
obj-y += rtk_memory_remap.o rtk_barrier.o rtk_sb2_dbg.o rtk_dc_sys_dbg.o
obj-y += rtk_chip.o
obj-y += dummy_uctrl.o
obj-$(CONFIG_RTK_DC_SYS_PC) += rtk_dc_sys_pc.o
obj-$(CONFIG_RTK_DC_SYS_PC_DEBUGFS) += rtk_dc_sys_pc-debugfs.o
obj-$(CONFIG_RTK_DC_SYS_PC_EDEV) += rtk_dc_sys_pc-edev.o
obj-$(CONFIG_RTK_DC_SYS_PC_UIODEV) += rtk_dc_sys_pc-uio.o
obj-y += rtk_sb2_sem.o
obj-$(CONFIG_RTK_RPC) += rpc/
obj-$(CONFIG_RTK_REGMAP_I2C) += rtk_regmap-i2c.o
obj-$(CONFIG_RTK_REGMAP_MMIO) += rtk_regmap-mmio.o
obj-$(CONFIG_RTK_MMIO) += rtk_mmio.o
obj-$(CONFIG_ION_RTK) += rtk_ion_of.o
obj-$(CONFIG_RTK_MCP) += rtk_mcp.o rtk_sha1.o
obj-$(CONFIG_RTK_SECURE_DVFS) += secure_dvfs.o
obj-$(CONFIG_RTK_AVCPU_LOG) += rtk_avcpulog.o
obj-y += pwrctrl/
obj-y += info/
obj-$(CONFIG_RTK_HSE) += hse/
obj-y += debug/
endif # CONFIG_SYNO_LSP_RTD1619
obj-$(CONFIG_SYNO_RTD_UART1) += rtd_syno_uart1.o

obj-y				+= common.o addr-map.o irq.o pcie.o mpp.o

obj-$(CONFIG_MACH_DB88F6281_BP)		+= db88f6281-bp-setup.o
obj-$(CONFIG_MACH_XCAT98DX)		+= xcat98dx-setup.o
obj-$(CONFIG_MACH_RD88F6192_NAS)	+= rd88f6192-nas-setup.o
obj-$(CONFIG_MACH_RD88F6281)		+= rd88f6281-setup.o
obj-$(CONFIG_MACH_RD88F6282_A)		+= rd88f6282-a-setup.o
obj-$(CONFIG_MACH_MV88F6281GTW_GE)	+= mv88f6281gtw_ge-setup.o
obj-$(CONFIG_MACH_SHEEVAPLUG)		+= sheevaplug-setup.o
obj-$(CONFIG_MACH_SYNOLOGY_6281)			+= synology-setup.o
obj-$(CONFIG_MACH_ESATA_SHEEVAPLUG)	+= sheevaplug-setup.o
obj-$(CONFIG_MACH_GURUPLUG)		+= guruplug-setup.o
obj-$(CONFIG_MACH_DOCKSTAR)		+= dockstar-setup.o
obj-$(CONFIG_MACH_TS219)		+= ts219-setup.o tsx1x-common.o
obj-$(CONFIG_MACH_TS41X)		+= ts41x-setup.o tsx1x-common.o
obj-$(CONFIG_MACH_OPENRD)		+= openrd-setup.o
obj-$(CONFIG_MACH_NETSPACE_V2)		+= netspace_v2-setup.o lacie_v2-common.o
obj-$(CONFIG_MACH_INETSPACE_V2)		+= netspace_v2-setup.o lacie_v2-common.o
obj-$(CONFIG_MACH_NETSPACE_MAX_V2)	+= netspace_v2-setup.o lacie_v2-common.o
obj-$(CONFIG_MACH_D2NET_V2)		+= d2net_v2-setup.o lacie_v2-common.o
obj-$(CONFIG_MACH_NET2BIG_V2)		+= netxbig_v2-setup.o lacie_v2-common.o
obj-$(CONFIG_MACH_NET5BIG_V2)		+= netxbig_v2-setup.o lacie_v2-common.o
obj-$(CONFIG_MACH_T5325)		+= t5325-setup.o

obj-$(CONFIG_CPU_IDLE)			+= cpuidle.o

obj-y					+= kirkwood.o

# Marvell adds
include $(srctree)/arch/arm/mach-kirkwood/config/mvRules.mk

COMMON_OBJS	= $(COMMON_DIR)/mvDebug.o $(COMMON_DIR)/mvCommon.o $(COMMON_DIR)/mvStack.o

OSSERVICES_OBJS	= $(OSSERV_DIR)/mvOs.o

HAL_OBJS	= $(HAL_TWSI_DIR)/mvTwsi.o $(HAL_GPP_DIR)/mvGpp.o

QD_OBJS		= $(HAL_QD_DIR)/src/driver/gtDrvConfig.o $(HAL_QD_DIR)/src/driver/gtDrvEvents.o \
		  $(HAL_QD_DIR)/src/driver/gtHwCntl.o $(HAL_QD_DIR)/src/platform/gtMiiSmiIf.o \
		  $(HAL_QD_DIR)/src/platform/platformDeps.o $(HAL_QD_DIR)/src/platform/gtSem.o \
		  $(HAL_QD_DIR)/src/platform/gtDebug.o $(HAL_QD_DIR)/src/msapi/gtBrgFdb.o \
		  $(HAL_QD_DIR)/src/msapi/gtBrgStp.o $(HAL_QD_DIR)/src/msapi/gtBrgVlan.o \
		  $(HAL_QD_DIR)/src/msapi/gtEvents.o $(HAL_QD_DIR)/src/msapi/gtPortCtrl.o \
		  $(HAL_QD_DIR)/src/msapi/gtPortStat.o $(HAL_QD_DIR)/src/msapi/gtPortStatus.o \
		  $(HAL_QD_DIR)/src/msapi/gtQosMap.o $(HAL_QD_DIR)/src/msapi/gtPIRL.o \
		  $(HAL_QD_DIR)/src/msapi/gtPhyCtrl.o $(HAL_QD_DIR)/src/msapi/gtPhyInt.o \
		  $(HAL_QD_DIR)/src/msapi/gtSysConfig.o $(HAL_QD_DIR)/src/msapi/gtSysCtrl.o \
		  $(HAL_QD_DIR)/src/msapi/gtVersion.o $(HAL_QD_DIR)/src/msapi/gtUtils.o \
		  $(HAL_QD_DIR)/src/msapi/gtBrgVtu.o $(HAL_QD_DIR)/src/msapi/gtPortRmon.o \
		  $(HAL_QD_DIR)/src/msapi/gtSysStatus.o $(HAL_QD_DIR)/src/msapi/gtPortRateCtrl.o \
		  $(HAL_QD_DIR)/src/msapi/gtPortPav.o $(HAL_QD_DIR)/src/msapi/gtVct.o \
		  $(HAL_QD_DIR)/src/msapi/gtPIRL2.o $(HAL_QD_DIR)/src/msapi/gtCCPVT.o \
		  $(HAL_QD_DIR)/src/msapi/gtPCSCtrl.o $(HAL_QD_DIR)/src/msapi/gtCCPVT.o

KW_FAM_OBJS	= $(BOARD_ENV_DIR)/mvBoardEnvSpec.o \
		  $(SOC_ENV_DIR)/mvCtrlEnvLib.o \
		  $(BOARD_ENV_DIR)/mvBoardEnvLib.o \
		  $(SOC_CPU_DIR)/mvCpu.o

LSP_OBJS	= $(LSP_DIR)/export.o $(LSP_DIR)/btns_device.o

kirkwood-objs	:= $(LSP_OBJS) $(COMMON_OBJS) $(OSSERVICES_OBJS) \
		  $(HAL_OBJS) $(KW_FAM_OBJS)

kirkwood-$(CONFIG_MV_INCLUDE_GIG_ETH)		+= $(HAL_ETH_DIR)/mvEthDebug.o \
						   $(HAL_ETHPHY_DIR)/mvEthPhy.o \
						   $(HAL_ETH_DIR)/mvEth.o

kirkwood-$(CONFIG_MV_ETH_NFP)			+= $(HAL_ETH_NFP_DIR)/mvNfp.o
kirkwood-$(CONFIG_MV_ETH_NFP_NAT_SUPPORT)	+= $(HAL_ETH_NFP_DIR)/mvNfpNat.o
kirkwood-$(CONFIG_MV_ETH_NFP_FDB_SUPPORT)	+= $(HAL_ETH_NFP_DIR)/mvNfpFdb.o
kirkwood-$(CONFIG_MV_ETH_NFP_PPP)		+= $(HAL_ETH_NFP_DIR)/mvNfpPpp.o
kirkwood-$(CONFIG_MV_ETH_NFP_SEC)		+= $(HAL_ETH_NFP_DIR)/mvNfpSec.o

kirkwood-$(CONFIG_MV_INCLUDE_CESA)		+= $(HAL_CESA_DIR)/mvCesa.o \
						   $(HAL_CESA_DIR)/mvCesaDebug.o \
						   $(HAL_CESA_DIR)/mvMD5.o \
						   $(HAL_CESA_DIR)/mvSHA1.o \
						   $(HAL_CESA_AES_DIR)/mvAesAlg.o \
						   $(HAL_CESA_AES_DIR)/mvAesApi.o

kirkwood-$(CONFIG_MV_CESA_TEST)			+= $(HAL_CESA_DIR)/mvCesaTest.o
kirkwood-$(CONFIG_MV_INCLUDE_SPI)		+= $(HAL_SPI_DIR)/mvSpi.o \
						   $(HAL_SPI_DIR)/mvSpiCmnd.o
kirkwood-$(CONFIG_MV_INCLUDE_TDM)		+= $(SOC_SYS_DIR)/mvSysTdm.o
kirkwood-$(CONFIG_MV_INCLUDE_TS)		+= $(HAL_TS_DIR)/mvTsu.o
kirkwood-$(CONFIG_MV_CPU_PERF_CNTRS)		+= $(HAL_CPU_DIR)/mvCpuCntrs.o
kirkwood-$(CONFIG_MV_CPU_L2_PERF_CNTRS)		+= $(HAL_CPU_DIR)/mvCpuL2Cntrs.o

kirkwood-$(CONFIG_MV_INCLUDE_SWITCH)		+= $(QD_OBJS)

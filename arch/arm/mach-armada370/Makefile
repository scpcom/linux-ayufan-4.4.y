#*******************************************************************************
# Marvell GPL License Option
#
# If you received this File from Marvell, you may opt to use, redistribute and/or
# modify this File in accordance with the terms and conditions of the General
# Public License Version 2, June 1991 (the "GPL License"), a copy of which is
# available along with the File in the license.txt file or by writing to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 or
# on the worldwide web at http://www.gnu.org/licenses/gpl.txt.
#
# THE FILE IS DISTRIBUTED AS-IS, WITHOUT WARRANTY OF ANY KIND, AND THE IMPLIED
# WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE ARE EXPRESSLY
# DISCLAIMED.  The GPL License provides additional details about this warranty
# disclaimer.
#*******************************************************************************/
ifneq ($(MACHINE),)
include $(srctree)/$(MACHINE)/config/mvRules.mk
endif

ifdef CONFIG_MV_ETH_NFP
	NFPOBJS += $(LSP_NFP_MGR_DIR)/mv_nfp_mgr.o $(LSP_NFP_MGR_DIR)/nfp_sysfs.o  $(LSP_NFP_MGR_DIR)/mv_nfp_hooks.o \
		$(LSP_NET_DEV_DIR)/mv_eth_nfp.o
endif

ifdef CONFIG_MV_ETH_NFP_FIB
	NFPOBJS += $(LSP_NFP_MGR_DIR)/nfp_fib_arp_sysfs.o
ifdef CONFIG_IPV6
	NFPOBJS += $(LSP_NFP_MGR_DIR)/ipv6_parsing.o $(LSP_NFP_MGR_DIR)/nfp_fib6_arp_sysfs.o
endif
endif

ifdef CONFIG_MV_ETH_NFP_CT
	NFPOBJS += $(LSP_NFP_MGR_DIR)/nfp_ct_sysfs.o
ifdef CONFIG_IPV6
	NFPOBJS += $(LSP_NFP_MGR_DIR)/nfp_ct6_sysfs.o
endif
endif

ifdef CONFIG_MV_ETH_NFP_CLASSIFY
	NFPOBJS += $(LSP_NFP_MGR_DIR)/nfp_classification_sysfs.o $(LSP_NFP_MGR_DIR)/nfp_exact_classification_sysfs.o \
		   $(LSP_NFP_MGR_DIR)/nfp_prio_classification_sysfs.o
endif

ifdef CONFIG_MV_ETH_NFP_BRIDGE
	NFPOBJS += $(LSP_NFP_MGR_DIR)/nfp_bridge_sysfs.o
endif

ifdef CONFIG_MV_ETH_NFP_VLAN
	NFPOBJS += $(LSP_NFP_MGR_DIR)/nfp_vlan_sysfs.o
endif

ifdef CONFIG_MV_ETH_NFP_PPP
	NFPOBJS += $(LSP_NFP_MGR_DIR)/nfp_ppp_sysfs.o
endif

ifdef CONFIG_MV_ETH_NFP
	NFPOBJS += $(HAL_ETH_NFP_DIR)/mvNfp.o
endif

ifdef CONFIG_MV_ETH_NFP_CT
	NFPOBJS += $(HAL_ETH_NFP_DIR)/mvNfpCt.o
endif

ifdef CONFIG_MV_ETH_NFP_BRIDGE
ifdef CONFIG_MV_ETH_NFP_FDB_MODE
	NFPOBJS += $(HAL_ETH_NFP_DIR)/mvNfpFdb.o
else
	NFPOBJS    += $(HAL_ETH_NFP_DIR)/mvNfpBridge.o
endif
endif

ifdef CONFIG_MV_ETH_NFP_FIB
	NFPOBJS += $(HAL_ETH_NFP_DIR)/mvNfpFib.o
endif

ifeq ($(CONFIG_MV_ETH_NFP),m)
nfp-objs = $(NFPOBJS)
obj-m := nfp.o
endif

# Objects list
COMMON_OBJS	= $(COMMON_DIR)/mvDebug.o $(COMMON_DIR)/mvCommon.o $(COMMON_DIR)/mvStack.o $(COMMON_DIR)/mvList.o

OSSERVICES_OBJS	= $(OSSERV_DIR)/mvOs.o

HAL_OBJS	= $(HAL_RTC_DIR)/mvRtc.o					\
                  $(HAL_CNTMR_DIR)/mvCntmr.o $(HAL_IF_DIR)/mvSysCntmr.o		\
		  $(HAL_TWSI_DIR)/mvTwsi.o $(HAL_IF_DIR)/mvSysTwsi.o		\
                  $(HAL_UART_DIR)/mvUart.o $(HAL_GPP_DIR)/mvGpp.o               \
                  $(HAL_DRAM_DIR)/mvDramIf.o  					\
		  $(HAL_IF_DIR)/mvSysDdr.o

ARMADA_FAM_OBJS	= $(BOARD_ENV_DIR)/mvBoardEnvSpec.o $(SOC_ENV_DIR)/mvCtrlEnvLib.o	\
		  $(BOARD_ENV_DIR)/mvBoardEnvLib.o $(SOC_ENV_DIR)/mvCtrlEnvAddrDec.o 	\
		  $(SOC_SYS_DIR)/mvAhbToMbus.o $(SOC_SYS_DIR)/mvCpuIf.o 		\
		  $(SOC_CPU_DIR)/mvCpu.o $(SOC_DEVICE_DIR)/mvDevice.o

QD_OBJS		= $(HAL_QD_DIR)/src/driver/gtDrvConfig.o $(HAL_QD_DIR)/src/driver/gtDrvEvents.o \
                  $(HAL_QD_DIR)/src/driver/gtHwCntl.o $(HAL_QD_DIR)/src/platform/gtMiiSmiIf.o	\
                  $(HAL_QD_DIR)/src/platform/platformDeps.o $(HAL_QD_DIR)/src/platform/gtSem.o	\
                  $(HAL_QD_DIR)/src/platform/gtDebug.o $(HAL_QD_DIR)/src/msapi/gtBrgFdb.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtBrgStp.o $(HAL_QD_DIR)/src/msapi/gtBrgVlan.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtEvents.o $(HAL_QD_DIR)/src/msapi/gtPortCtrl.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtPortStat.o $(HAL_QD_DIR)/src/msapi/gtPortStatus.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtQosMap.o $(HAL_QD_DIR)/src/msapi/gtPIRL.o  		\
                  $(HAL_QD_DIR)/src/msapi/gtPhyCtrl.o $(HAL_QD_DIR)/src/msapi/gtPhyInt.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtSysConfig.o $(HAL_QD_DIR)/src/msapi/gtSysCtrl.o	\
                  $(HAL_QD_DIR)/src/msapi/gtVersion.o $(HAL_QD_DIR)/src/msapi/gtUtils.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtBrgVtu.o $(HAL_QD_DIR)/src/msapi/gtPortRmon.o 	\
                  $(HAL_QD_DIR)/src/msapi/gtSysStatus.o $(HAL_QD_DIR)/src/msapi/gtPortRateCtrl.o\
                  $(HAL_QD_DIR)/src/msapi/gtPortPav.o $(HAL_QD_DIR)/src/msapi/gtVct.o		\
		  $(HAL_QD_DIR)/src/msapi/gtPIRL2.o $(HAL_QD_DIR)/src/msapi/gtCCPVT.o		\
		  $(HAL_QD_DIR)/src/msapi/gtPCSCtrl.o $(HAL_QD_DIR)/src/msapi/gtBrgStu.o

LSP_OBJS        = core.o irq.o time.o leds.o sysmap.o export.o clock.o synology-gpio.o synology-platform.o

obj-y   				:=  armada370.o
armada370-objs  			:=$(LSP_OBJS) $(COMMON_OBJS) $(OSSERVICES_OBJS) $(HAL_OBJS) 	\
					  $(ARMADA_FAM_OBJS)

armada370-$(CONFIG_MV_INCLUDE_SDIO) 	+= $(HAL_SDMMC_DIR)/mvSdmmcAddrDec.o
armada370-$(CONFIG_MV_INCLUDE_XOR) 	+= $(HAL_XOR_DIR)/mvXor.o $(HAL_XOR_DIR)/mvXorAddrDec.o		\
					   $(HAL_IF_DIR)/mvSysXor.o
armada370-$(CONFIG_MV_INCLUDE_PEX) 	+= $(HAL_PEX_DIR)/mvPex.o					\
					   $(HAL_IF_DIR)/mvSysPex.o $(HAL_PEX_DIR)/mvPexAddrDec.o
armada370-$(CONFIG_MV_INCLUDE_PCI) 	+= $(HAL_PCI_DIR)/mvPci.o $(HAL_IF_DIR)/mvSysPci.o
armada370-$(CONFIG_MV_INCLUDE_USB) 	+= $(HAL_USB_DIR)/mvUsb.o $(HAL_USB_DIR)/mvUsbAddrDec.o		\
					   $(HAL_IF_DIR)/mvSysUsb.o
armada370-y				+= $(HAL_ETHPHY_DIR)/mvEthPhy.o $(HAL_IF_DIR)/mvSysEthPhy.o

ifneq ($(CONFIG_MV_ETH_NFP),m)
	armada370-y += $(NFPOBJS)
endif

# Legacy Giga driver
ifeq ($(CONFIG_MV_ETH_LEGACY),y)
armada370-$(CONFIG_MV_ETH_LEGACY)	+= $(HAL_ETH_GBE_DIR)/mvEth.o $(HAL_ETH_GBE_DIR)/mvEthDebug.o \
					   $(HAL_ETH_GBE_DIR)/mvEthAddrDec.o $(HAL_IF_DIR)/mvSysEth.o
armada370-$(CONFIG_MV_ETH_NFP)           += $(HAL_ETH_NFP_DIR)/mvNfp.o
armada370-$(CONFIG_MV_ETH_NFP_NAT) 	+= $(HAL_ETH_NFP_DIR)/mvNfpNat.o
armada370-$(CONFIG_MV_ETH_NFP_FDB) 	+= $(HAL_ETH_NFP_DIR)/mvNfpFdb.o
armada370-$(CONFIG_MV_ETH_NFP_PPP) 	+= $(HAL_ETH_NFP_DIR)/mvNfpPpp.o
armada370-$(CONFIG_MV_ETH_NFP_SEC)	+= $(HAL_ETH_NFP_DIR)/mvNfpSec.o

endif

# NETA Giga driver
ifeq ($(CONFIG_MV_ETH_NETA),y)
armada370-$(CONFIG_MV_ETH_NETA)	        += $(HAL_ETH_GBE_DIR)/mvNeta.o $(HAL_ETH_GBE_DIR)/mvNetaDebug.o \
					   $(HAL_ETH_GBE_DIR)/mvNetaAddrDec.o $(HAL_IF_DIR)/mvSysNeta.o
armada370-$(CONFIG_MV_ETH_PNC)    	+= $(HAL_ETH_PNC_DIR)/mvTcam.o $(HAL_ETH_PNC_DIR)/mvPnc.o
armada370-$(CONFIG_MV_ETH_BM) 	        += $(HAL_ETH_BM_DIR)/mvBm.o
armada370-$(CONFIG_MV_ETH_PMT)	        += $(HAL_ETH_PMT_DIR)/mvPmt.o
armada370-$(CONFIG_MV_ETH_HWF)           += $(HAL_ETH_GBE_DIR)/mvHwf.o
endif

armada370-$(CONFIG_MV_INCLUDE_CESA)     += $(HAL_CESA_DIR)/mvCesa.o $(HAL_CESA_DIR)/mvCesaDebug.o       \
					   $(HAL_CESA_DIR)/mvCesaAddrDec.o				\
					   $(HAL_CESA_DIR)/mvMD5.o $(HAL_CESA_DIR)/mvSHA1.o 		\
					   $(HAL_CESA_DIR)/mvSHA256.o					\
					   $(HAL_CESA_AES_DIR)/mvAesAlg.o $(HAL_CESA_AES_DIR)/mvAesApi.o\
					   $(HAL_IF_DIR)/mvSysCesa.o
armada370-$(CONFIG_MV_INCLUDE_INTEG_SATA)+= $(HAL_IF_DIR)/mvSysSata.o $(HAL_SATA_DIR)/mvSataSoc.o	\
					   $(HAL_SATA_DIR)/mvSataAddrDec.o
armada370-$(CONFIG_MV_INCLUDE_SPI) 	+= $(HAL_SPI_DIR)/mvSpi.o $(HAL_SPI_DIR)/mvSpiCmnd.o 		\
					   $(HAL_SFLASH_DIR)/mvSFlash.o $(HAL_IF_DIR)/mvSysSFlash.o	\
					   $(HAL_IF_DIR)/mvSysSpi.o
armada370-$(CONFIG_MV_INCLUDE_NFC)	+= $(HAL_NFC_DIR)/mvNfc.o
armada370-$(CONFIG_MV_INCLUDE_AUDIO) 	+= $(HAL_AUDIO_DIR)/mvAudio.o $(HAL_IF_DIR)/mvSysAudio.o	\
					   $(HAL_AUDIO_DIR)/mvAudioAddrDec.o
armada370-$(CONFIG_MV_INCLUDE_TS) 	+= $(HAL_TS_DIR)/mvTsu.o $(HAL_IF_DIR)/mvSysTs.o		\
					   $(HAL_TS_DIR)/mvTsuAddrDec.o
armada370-$(CONFIG_MV_CPU_PERF_CNTRS)    += $(HAL_CPU_DIR)/mvCpuCntrs.o $(HAL_CPU_DIR)/pj4/mvPJ4Cntrs.o
armada370-$(CONFIG_MV_CPU_L2_PERF_CNTRS) += $(HAL_CPU_DIR)/mvCpuL2Cntrs.o

obj-$(CONFIG_MV_INCLUDE_SWITCH) 	+= $(QD_OBJS)

# drivers part
# Legacy Giga driver
ifeq ($(CONFIG_MV_ETH_LEGACY),y)
obj-$(CONFIG_MV_ETH_NFP) 	        += $(LSP_NFP_MGR_DIR)/mv_nfp_mgr.o
obj-$(CONFIG_MV_ETH_NFP_SEC)            += $(LSP_NFP_MGR_DIR)/mv_nfp_sec.o
endif

ifeq ($(CONFIG_MV_ETH_NETA),y)
obj-$(CONFIG_MV_ETH_PNC)                += $(LSP_PNC_DIR)/pnc_sysfs.o
obj-$(CONFIG_MV_ETH_BM)                 += $(LSP_BM_DIR)/bm_sysfs.o  $(LSP_BM_DIR)/mv_eth_bm.o
obj-$(CONFIG_MV_ETH_PMT)		+= $(LSP_PMT_DIR)/pmt_sysfs.o
obj-$(CONFIG_MV_ETH_HWF)        	+= $(LSP_HWF_DIR)/hwf_sysfs.o
obj-$(CONFIG_MV_ETH_L2FW)               += $(LSP_L2FW_DIR)/l2fw_sysfs.o $(LSP_L2FW_DIR)/mv_eth_l2fw.o
obj-$(CONFIG_MV_ETH_L2SEC)		+= $(LSP_L2FW_DIR)/mv_eth_l2sec.o
endif

obj-$(CONFIG_MV_USE_XOR_ENGINE) 	+= $(PLAT_DRIVERS)/mv_xor/
obj-$(CONFIG_MV_CESA) 			+= $(PLAT_DRIVERS)/mv_cesa/
#obj-y					+= $(PLAT_DRIVERS)/mv_btns/
obj-y					+= $(PLAT_DRIVERS)/mv_gpio/
obj-$(CONFIG_MV_INCLUDE_SWITCH)         += $(LSP_SWITCH_DIR)/
obj-$(CONFIG_SENSORS_ARMADA)		+= hwmon.o
# The rest of the drivers are compiled through the driver dir directly.


# LSP part
armada370-$(CONFIG_MV_INCLUDE_USB)    	+= usb.o
armada370-$(CONFIG_MV_INCLUDE_PCI)	+= pci.o
armada370-$(CONFIG_MV_INCLUDE_PEX)	+= pex.o
armada370-$(CONFIG_FEROCEON_PROC) 	+= $(PLAT_DRIVERS)/mv_proc/proc.o
armada370-$(CONFIG_MV_DBG_TRACE) 	+= dbg-trace.o
armada370-$(CONFIG_PROC_FS)           += dump_cp15_regs.o


/*
 * This header provides constants for the reset controller based peripheral
 * powerdown requests on the Freescale LS1024A SoC.
 */
#ifndef _DT_BINDINGS_RESET_CONTROLLER_LS1024A
#define _DT_BINDINGS_RESET_CONTROLLER_LS1024A

#define LS1024A_AXI_RTC_RESET		1
#define LS1024A_AXI_LEGACY_SPI_RESET	2
#define LS1024A_AXI_I2C_RESET		3
#define LS1024A_AXI_DMA_RESET		4
#define LS1024A_AXI_FAST_UART_RESET	5
#define LS1024A_AXI_FAST_SPI_RESET	6
#define LS1024A_AXI_TDM_RESET		7
#define LS1024A_PFE_SYS_RESET		8
#define LS1024A_AXI_IPSEC_EAPE_RESET	9
#define LS1024A_AXI_IPSEC_SPACC_RESET	10
#define LS1024A_AXI_DPI_CIE_RESET	11
#define LS1024A_AXI_DPI_DECOMP_RESET	12
#define LS1024A_AXI_USB1_RESET		13 /* USB controller1, AXI Clock Domain reset control */
#define LS1024A_UTMI_USB1_RESET  	14 /* USB controller1,UTMI Clock Domain reset control*/
#define LS1024A_USB1_PHY_RESET		15 /* USB PHY1 Reset control */
#define LS1024A_AXI_USB0_RESET		16 /* USB controller0, AXI Clock Domain reset control*/
#define LS1024A_UTMI_USB0_RESET  	17 /* USB controller0. UTMI Clock Domain reset control */
#define LS1024A_USB0_PHY_RESET		18 /* USB PHY0 Reset Control */
#define LS1024A_AXI_SATA_RESET		19 /* SATA controller AXI Clock Domain Control Both for SATA 0/1*/
#define LS1024A_SERDES_SATA0_RESET	20 /* SATA serdes Controller0  TX,Core Logic and RX clock domain control*/
#define LS1024A_SERDES_SATA1_RESET	21 /* SATA serdes Controller1  TX,Core Logic and RX clock domain control*/
#define LS1024A_AXI_PCIE1_RESET		22 /* PCIE Controller1,AXI Clock Domain reset control*/
#define LS1024A_SERDES_PCIE1_RESET	23 /* PCIE serdes Controller1  Striky register and power register*/
#define LS1024A_AXI_PCIE0_RESET		24 /* PCIE Controller0,AXI Clock Domain reset control*/
#define LS1024A_SERDES_PCIE0_RESET	25 /* PCIE Controller1,TX,Core Logic and RX clock domain control*/
#define LS1024A_PFE_CORE_RESET		26
#define LS1024A_IPSEC_EAPE_CORE_RESET	27
#define LS1024A_GEMTX_RESET		28
#define LS1024A_L2CC_RESET		29
#define LS1024A_DECT_RESET		30
#define LS1024A_DDR_CNTLR_RESET		31
#define LS1024A_DDR_PHY_RESET		32
#define LS1024A_SERDES0_RESET		33
#define LS1024A_SERDES1_RESET		34
#define LS1024A_SERDES2_RESET		35
#define LS1024A_SGMII_RESET		36
#define LS1024A_SATA_PMU_RESET		37
#define LS1024A_SATA_OOB_RESET		38
#define LS1024A_TDMNTG_RESET		39

#endif /* _DT_BINDINGS_RESET_CONTROLLER_LS1024A */

#ifndef HI_IOMMU_REG_H
#define HI_IOMMU_REG_H

/* Global Register Space 0 */
#define ARM_SMMU_GR0_SCR0			0x0
#define ARM_SMMU_GR0_SCR1			0x4
#define ARM_SMMU_GR0_SCR2			0x8
#define ARM_SMMU_GR0_SACR			0x10
#define ARM_SMMU_GR0_IDR0			0x20
#define ARM_SMMU_GR0_IDR1			0x24
/* IDR2~ IDR7 0x28~0x3c */
#define ARM_SMMU_GR0_SGFAR_			0x40
#define ARM_SMMU_GR0_SGFAR_H			0x44
#define ARM_SMMU_GR0_SGFSR			0x48
#define ARM_SMMU_GR0_SGFRRESTORE		0x4c
#define ARM_SMMU_GR0_SGFSYNR0			0x50
#define ARM_SMMU_GR0_SGFSYNR1			0x54
#define ARM_SMMU_GR0_SGFSYNR2			0x58
#define ARM_SMMU_GR0_STLBIALL			0x60
#define ARM_SMMU_GR0_TLBIVMID			0x64
#define ARM_SMMU_GR0_TLBIVALLNSNH		0x68
#define ARM_SMMU_GR0_TLBIVALLH			0x6c
#define ARM_SMMU_GR0_STLBGSYNC			0x70
#define ARM_SMMU_GR0_STLBSTATUS			0x74
#define ARM_SMMU_GR0_TLBIVAH_L			0x78
#define ARM_SMMU_GR0_TLBIVAH_H			0x7c
#define ARM_SMMU_GR0_STLBIVALM_L		0xa0
#define ARM_SMMU_GR0_STLBIVALM_H		0xa4
#define ARM_SMMU_GR0_STLBIVAM_L			0xa8
#define ARM_SMMU_GR0_STLBIVAM_H			0xac
#define ARM_SMMU_GR0_TLBIVALH64_L		0xb0
#define ARM_SMMU_GR0_TLBIVALH64_H		0xb4
#define ARM_SMMU_GR0_TLBIVMIDSI			0xb8
#define ARM_SMMU_GR0_STLBIALLM			0xbc
#define ARM_SMMU_GR0_TLBIVAH64_L		0xc0
#define ARM_SMMU_GR0_TLBIVAH64_H		0xc4
/* too much registers just define required here */
#define ARM_SMMU_GR0_SMR(n)		(0x800 + ((n) << 2))
#define ARM_SMMU_GR0_S2CR(n)            (0xc00 + ((n) << 2))

/* Global Register Space 1 */
#define ARM_SMMU_GR1_CBAR(n)            (0x0 + ((n) << 2))
#define ARM_SMMU_GR1_CBFRSYNRA(n)	(0x400 + ((n) << 2))
#define ARM_SMMU_GR1_CBA2R(n)           (0x800 + ((n) << 2))

/* GR0 SMR register definations */
#define SMR_VALID                       (1 << 31)
#define SMR_MASK_SHIFT                  16
#define SMR_MASK_MASK                   0x7fff
#define SMR_ID_SHIFT                    0
#define SMR_ID_MASK                     0x7fff

/* GR0 S2CR register definations */
#define S2CR_CBNDX_SHIFT                0
#define S2CR_CBNDX_MASK                 0xff
#define S2CR_TYPE_SHIFT                 16
#define S2CR_TYPE_MASK                  0x3
#define S2CR_TYPE_TRANS                 (0 << S2CR_TYPE_SHIFT)
#define S2CR_TYPE_BYPASS                (1 << S2CR_TYPE_SHIFT)
#define S2CR_TYPE_FAULT                 (2 << S2CR_TYPE_SHIFT)
#define S2CR_PRIV_DEF			(0 << 24)
#define S2CR_PRIV_PRIV			(3 << 24)
#define S2CR_PRIV_UNPRIV		(2 << 24)

/* GR1 CBAR register definations */
#define CBAR_VMID_SHIFT                 0
#define CBAR_VMID_MASK                  0xff
#define CBAR_S1_MEMATTR_SHIFT           12
#define CBAR_S1_MEMATTR_MASK            0xf
#define CBAR_S1_MEMATTR_WB              0xf
#define CBAR_TYPE_SHIFT                 16
#define CBAR_TYPE_MASK                  0x3
#define CBAR_TYPE_S2_TRANS              (0 << CBAR_TYPE_SHIFT)
#define CBAR_TYPE_S1_TRANS_S2_BYPASS    (1 << CBAR_TYPE_SHIFT)
#define CBAR_TYPE_S1_TRANS_S2_FAULT     (2 << CBAR_TYPE_SHIFT)
#define CBAR_TYPE_S1_TRANS_S2_TRANS     (3 << CBAR_TYPE_SHIFT)
#define CBAR_IRPTNDX_SHIFT              24
#define CBAR_IRPTNDX_MASK               0xff

/* GR1 CBA2R register definations */
#define CBA2R_RW64_32BIT                (0 << 0)
#define CBA2R_RW64_64BIT                (1 << 0)

/* Context Bank registers */
#define ARM_SMMU_CB_SCTLR		0x0
#define ARM_SMMU_CB_ACTLR		0x4
#define ARM_SMMU_CB_RESUME		0x8
#define ARM_SMMU_CB_TCR2		0x10
#define ARM_SMMU_CB_TTBR0_L		0x20
#define ARM_SMMU_CB_TTBR0_H		0x24
#define ARM_SMMU_CB_TTBR1_L		0x28
#define ARM_SMMU_CB_TTBR1_H		0x2c
#define ARM_SMMU_CB_TCR			0x30
#define ARM_SMMU_CB_CONTEXTIDR		0X34
#define ARM_SMMU_CB_PRRR		0x38
#define ARM_SMMU_CB_NMRR		0x3c
#define ARM_SMMU_CB_PAR_L		0x50
#define ARM_SMMU_CB_PAR_H		0x54
#define ARM_SMMU_CB_FSR			0x58
#define ARM_SMMU_CB_FSRRESTORE		0x5c
#define ARM_SMMU_CB_FAR_L		0x60
#define ARM_SMMU_CB_FAR_H		0x64
#define ARM_SMMU_CB_FSYNR0		0x68
#define ARM_SMMU_CB_FSYNR1		0x6c
#define ARM_SMMU_CB_TLBIVA_L		0x600
#define ARM_SMMU_CB_TLBIVA_H		0x604
#define ARM_SMMU_CB_TLBIVAA_L		0x608
#define ARM_SMMU_CB_TLBIVAA_H		0x60c
#define ARM_SMMU_CB_TLBIASID		0x610
#define ARM_SMMU_CB_TLBIALL		0x618
#define ARM_SMMU_CB_TLBIVAL_L		0x620
#define ARM_SMMU_CB_TLBIVAL_H		0x624
#define ARM_SMMU_CB_TLBIVAAL_L		0x628
#define ARM_SMMU_CB_TLBIVAAL_H		0x62c
#define ARM_SMMU_CB_TLBSYNC		0x7f0
#define ARM_SMMU_CB_TLBSTATUS		0x7f4

#define TTBCR2_SEP_SHIFT                15
#define ARM_SMMU_CB_TTBCR               0x30
#define ARM_SMMU_GR0_TLBIVMID           0x64
#define ARM_SMMU_GR0_sTLBGSYNC          0x70
#define STLBGSTATUS_GSACTIVE            (1 << 0)

#define SCTLR_S1_ASIDPNE                (1 << 12)
#define SCTLR_CFCFG                     (1 << 7)
#define SCTLR_CFIE                      (1 << 6)
#define SCTLR_CFRE                      (1 << 5)
#define SCTLR_E                         (1 << 4)
#define SCTLR_AFE                       (1 << 2)
#define SCTLR_TRE                       (1 << 1)
#define SCTLR_M                         (1 << 0)
#define SCTLR_EAE_SBOP                  (SCTLR_AFE)
#define ARM_SMMU_CB_CONTEXTIDR		0x34
#define ARM_SMMU_CB_TTBCR2              0x10
#define ARM_SMMU_CB_PRRR		0x38
#define ARM_SMMU_CB_NMRR		0x3C
/* Common definitions for PASize and SEP fields */
#define TTBCR2_ADDR_32                  0
#define TTBCR2_PASIZE_SHIFT             0
#define TLB_LOOP_TIMEOUT                1000000 /* 1s! */

/* sCR0 register definations */
#define sCR0_CLIENTPD                   (1 << 0)
#define sCR0_GFRE                       (1 << 1)
#define sCR0_GFIE                       (1 << 2)
#define sCR0_GCFGFRE                    (1 << 4)
#define sCR0_GCFGFIE                    (1 << 5)
#define sCR0_USFCFG                     (1 << 10)
#define sCR0_VMIDPNE                    (1 << 11)
#define sCR0_PTM                        (1 << 12)
#define sCR0_FB                         (1 << 13)
#define sCR0_SMCFCFG			(1 << 21)
#define sCR0_BSU_SHIFT                  14
#define sCR0_BSU_MASK                   0x3

/* sCR1 register definations */
#define sCR1_GEFRO			(1 << 25)

/* TTB attributes definations */
#define TTB_S           (1 << 1)
#define TTB_RGN_NC      (0 << 3)
#define TTB_RGN_OC_WBWA (1 << 3)
#define TTB_RGN_OC_WT   (2 << 3)
#define TTB_RGN_OC_WB   (3 << 3)
#define TTB_NOS         (1 << 5)
#define TTB_IRGN_NC     ((0 << 0) | (0 << 6))
#define TTB_IRGN_WBWA   ((0 << 0) | (1 << 6))
#define TTB_IRGN_WT     ((1 << 0) | (0 << 6))
#define TTB_IRGN_WB     ((1 << 0) | (1 << 6))

/* SMMU FRS definations */
#define FSR_MUTI                        (1 << 31)
#define FSR_SS                          (1 << 30)
#define FSR_FAULT                       0x1fe
#define FSYNR0_WNR                      (1 << 4)
#define RESUME_RETRY			0x0
#endif

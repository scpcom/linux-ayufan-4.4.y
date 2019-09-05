#ifndef _ECC_H
#define _ECC_H

/* ECC Register Set */
/********************/
/* ECC Control Registers */
#define ECC_SHIFT_EN_CFG        (0x0)
#define ECC_GEN_CFG             (0x4)
#define ECC_TAG_CFG             (0x8)
#define ECC_INIT_CFG            (0xC)
#define ECC_PRTY_OUT_SEL_CFG    (0x10)
#define ECC_POLY_START_CFG      (0x14)
#define ECC_CS_SEL_CFG          (0x18)
/* ECC Status Registers */
#define ECC_IDLE_STAT           (0x1C)
#define ECC_POLY_STAT           (0x20)
#define ECC_CORR_STAT           (0x24)
#define ECC_CORR_DONE_STAT      (0x28)
#define ECC_CORR_DATA_STAT      (0x2C)

/* ECC general configuration register parameters */
#define HAMM_MODE       BIT_28_MSK
#define BCH_MODE        (~HAMM_MODE)

#define PRTY_MODE_MASK  BIT_24_MSK
#define PRTY_CALC       PRTY_MODE_MASK
#define SYNDROME_CALC   (~PRTY_CALC)

#define	ECC_LVL_MASK    0x3F0000
#define ECC_LVL_SHIFT   16

#define BLK_SIZE_MASK 0x7FF

#define ECC_LVL_2  0x2
#define ECC_LVL_4  0x4
#define ECC_LVL_6  0x6
#define ECC_LVL_8  0x8
#define ECC_LVL_10 0xA
#define ECC_LVL_12 0xC
#define ECC_LVL_14 0xE
#define ECC_LVL_16 0x10
#define ECC_LVL_18 0x12
#define ECC_LVL_20 0x14
#define ECC_LVL_22 0x16
#define ECC_LVL_24 0x18
#define ECC_LVL_26 0x1A
#define ECC_LVL_28 0x1C
#define ECC_LVL_30 0x1E
#define ECC_LVL_32 0x20

#if defined(CONFIG_NAND_COMCERTO_ECC_24_HW_BCH)
	#define ECC_LVL_VAL ECC_LVL_24 /* ECC Level 24 is used */
#elif defined(CONFIG_NAND_COMCERTO_ECC_8_HW_BCH)
	#define ECC_LVL_VAL ECC_LVL_8 /* ECC Level 8 is used */
#endif

/* Block size used in Bytes*/
#define ECC_BLOCK_SIZE_512 512
#define ECC_BLOCK_SIZE_1024 1024

/* Maximum value of ECC Block size is 2k-(1+14*ECC_LVL/8) Bytes */
#define ECC_BLOCK_SIZE		ECC_BLOCK_SIZE_1024
#define ECC_BLOCK_SIZE_SHIFT	ECC_BLOCK_SIZE_1024_SHIFT

#define ECC_CS4_SEL 0x10
#define ECC_CS3_SEL 0x08
#define ECC_CS2_SEL 0x04
#define ECC_CS1_SEL 0x02
#define ECC_CS0_SEL 0x01

#define ECC_INIT                0x1
#define ECC_SHIFT_ENABLE        0x1
#define ECC_SHIFT_DISABLE       0x0
#define ECC_PARITY_OUT_EN       0x1
#define ECC_PARITY_OUT_DISABLE  0x0

/* Polynomial Start Configuration (ECC_POLY_START_CFG) */
#define ECC_POLY_START          (1 << 0)

/* Idle Status (ECC_IDLE_STAT) */
#define ECC_IDLE                (1 << 0)

/* Polynomial Status (ECC_POLY_STAT) */
#define ECC_CORR_REQ            (1 << 0)
#define ECC_ERASED_PAGE         (1 << 1)
#define ECC_UNCORR_ERR_HAMM     (1 << 2)

/* Correction Status (ECC_CORR_STAT) */
#define ECC_TAG_MASK            0xFFFF
#define ECC_NUM_ERR_MASK        0x3F
#define ECC_NUM_ERR_SHIFT       16
#define ECC_UNCORR              (1 << 24)

/* Correction Done Status (ECC_CORR_DONE_STAT) */
#define ECC_DONE                (1 << 0)

/* Correction Data Status (ECC_CORR_DATA_STAT), BCH Mode */
#define ECC_BCH_MASK            0xFFFF
#define ECC_BCH_INDEX_MASK      0x7FF
#define ECC_BCH_INDEX_SHIFT     16
#define ECC_BCH_VALID           (1 << 31)

/* Correction Data Status (ECC_CORR_DATA_STAT), Hamming Mode */
#define ECC_HAMM_MASK           0xF
#define ECC_HAMM_INDEX_MASK     0x1FF
#define ECC_HAMM_INDEX_SHIFT    16
#define ECC_HAMM_VALID          (1 << 31)

#endif /*_ECC_H */

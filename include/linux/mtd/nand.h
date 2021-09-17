#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __LINUX_MTD_NAND_H
#define __LINUX_MTD_NAND_H

#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/flashchip.h>
#include <linux/mtd/bbm.h>

struct mtd_info;
struct nand_flash_dev;
 
extern int nand_scan(struct mtd_info *mtd, int max_chips);
 
extern int nand_scan_ident(struct mtd_info *mtd, int max_chips,
			   struct nand_flash_dev *table);
extern int nand_scan_tail(struct mtd_info *mtd);

extern void nand_release(struct mtd_info *mtd);

extern void nand_wait_ready(struct mtd_info *mtd);

extern int nand_lock(struct mtd_info *mtd, loff_t ofs, uint64_t len);

extern int nand_unlock(struct mtd_info *mtd, loff_t ofs, uint64_t len);

#define NAND_MAX_CHIPS		8

#if defined(CONFIG_SYNO_LSP_HI3536)
#define NAND_MAX_OOBSIZE	4800
#define NAND_MAX_PAGESIZE	32768
#else  
#define NAND_MAX_OOBSIZE	640
#define NAND_MAX_PAGESIZE	8192
#endif  

#define NAND_NCE		0x01
 
#define NAND_CLE		0x02
 
#define NAND_ALE		0x04

#define NAND_CTRL_CLE		(NAND_NCE | NAND_CLE)
#define NAND_CTRL_ALE		(NAND_NCE | NAND_ALE)
#define NAND_CTRL_CHANGE	0x80

#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_RNDOUT		5
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#if defined(CONFIG_SYNO_LSP_HI3536)
#define NAND_CMD_STATUS_MULTI	0x71
#endif  
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_RNDIN		0x85
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_PARAM		0xec
#define NAND_CMD_GET_FEATURES	0xee
#define NAND_CMD_SET_FEATURES	0xef
#if defined(CONFIG_SYNO_LSP_HI3536)
#define NAND_CMD_SYNC_RESET	0xfc
#endif  
#define NAND_CMD_RESET		0xff

#define NAND_CMD_LOCK		0x2a
#define NAND_CMD_UNLOCK1	0x23
#define NAND_CMD_UNLOCK2	0x24

#define NAND_CMD_READSTART	0x30
#define NAND_CMD_RNDOUTSTART	0xE0
#define NAND_CMD_CACHEDPROG	0x15

#if defined(CONFIG_SYNO_LSP_HI3536)
 
#define NAND_CMD_DEPLETE1	0x100
#define NAND_CMD_DEPLETE2	0x38
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_STATUS_ERROR	0x72
 
#define NAND_CMD_STATUS_ERROR0	0x73
#define NAND_CMD_STATUS_ERROR1	0x74
#define NAND_CMD_STATUS_ERROR2	0x75
#define NAND_CMD_STATUS_ERROR3	0x76
#define NAND_CMD_STATUS_RESET	0x7f
#define NAND_CMD_STATUS_CLEAR	0xff
#endif  

#define NAND_CMD_NONE		-1

#if defined (MY_DEF_HERE)
 
#define NAND_FEATURE_MICRON_ARRAY_OP_MODE	0x90
#endif  

#define NAND_STATUS_FAIL	0x01
#define NAND_STATUS_FAIL_N1	0x02
#define NAND_STATUS_TRUE_READY	0x20
#define NAND_STATUS_READY	0x40
#define NAND_STATUS_WP		0x80
#if defined (MY_DEF_HERE)
#define NAND_STATUS_ECCREWRITE	0x08
#endif  

typedef enum {
	NAND_ECC_NONE,
	NAND_ECC_SOFT,
	NAND_ECC_HW,
	NAND_ECC_HW_SYNDROME,
	NAND_ECC_HW_OOB_FIRST,
#if defined (MY_DEF_HERE)
	NAND_ECC_4BITONDIE,
#endif  
	NAND_ECC_SOFT_BCH,
} nand_ecc_modes_t;

#define NAND_ECC_READ		0
 
#define NAND_ECC_WRITE		1
 
#define NAND_ECC_READSYN	2

#define NAND_GET_DEVICE		0x80

#if defined (MY_DEF_HERE) || defined(CONFIG_SYNO_LSP_HI3536)
 
#define NAND_NO_AUTOINCR	0x00000001
#endif  
 
#define NAND_BUSWIDTH_16	0x00000002

#if defined(CONFIG_SYNO_LSP_HI3536)
 
#define NAND_NO_PADDING		0x00000004
#endif  

#define NAND_CACHEPRG		0x00000008
#if defined (MY_DEF_HERE) || defined(CONFIG_SYNO_LSP_HI3536)

#define NAND_COPYBACK		0x00000010

#endif  

#if defined(CONFIG_SYNO_LSP_HI3536)
 
#define NAND_IS_AND		0x00000020
 
#define NAND_4PAGE_ARRAY	0x00000040
 
#define BBT_AUTO_REFRESH	0x00000080
#endif  

#define NAND_NEED_READRDY	0x00000100

#define NAND_NO_SUBPAGE_WRITE	0x00000200

#define NAND_BROKEN_XD		0x00000400

#define NAND_ROM		0x00000800

#define NAND_SUBPAGE_READ	0x00001000
#if defined (MY_DEF_HERE)
 
#define NAND_CACHERD		0x00001000

#define NAND_MULTIPLANE_READ	0x00002000
 
#define NAND_MULTIPLANE_PROG_ERASE	0x00004000
 
#define NAND_MULTILUN		0x00008000
 
#define NAND_MICRON_4BITONDIEECC	0x00080000
#endif  

#define NAND_SAMSUNG_LP_OPTIONS NAND_CACHEPRG

#if defined (MY_DEF_HERE)
#define NAND_CANAUTOINCR(chip) (!(chip->options & NAND_NO_AUTOINCR))
#endif  
#define NAND_HAS_CACHEPROG(chip) ((chip->options & NAND_CACHEPRG))
#define NAND_HAS_SUBPAGE_READ(chip) ((chip->options & NAND_SUBPAGE_READ))
#if defined (MY_DEF_HERE)
#define NAND_CHIPOPTIONS_MSK	(0x0000ffff & ~NAND_NO_AUTOINCR)
#endif  

#define NAND_SKIP_BBTSCAN	0x00010000
 
#define NAND_OWN_BUFFERS	0x00020000
 
#define NAND_SCAN_SILENT_NODEV	0x00040000
 
#define NAND_BUSWIDTH_AUTO      0x00080000

#define NAND_CONTROLLER_ALLOC	0x80000000

#define NAND_CI_CHIPNR_MSK	0x03
#define NAND_CI_CELLTYPE_MSK	0x0C

struct nand_chip;

#if defined(MY_ABC_HERE)
 
#define ONFI_FEATURE_16_BIT_BUS		(1 << 0)
#define ONFI_FEATURE_EXT_PARAM_PAGE	(1 << 7)
#endif  

#define ONFI_TIMING_MODE_0		(1 << 0)
#define ONFI_TIMING_MODE_1		(1 << 1)
#define ONFI_TIMING_MODE_2		(1 << 2)
#define ONFI_TIMING_MODE_3		(1 << 3)
#define ONFI_TIMING_MODE_4		(1 << 4)
#define ONFI_TIMING_MODE_5		(1 << 5)
#define ONFI_TIMING_MODE_UNKNOWN	(1 << 6)

#define ONFI_FEATURE_ADDR_TIMING_MODE	0x1

#define ONFI_SUBFEATURE_PARAM_LEN	4

struct nand_onfi_params {
	 
	u8 sig[4];
	__le16 revision;
	__le16 features;
	__le16 opt_cmd;
#if defined(MY_ABC_HERE)
	u8 reserved0[2];
	__le16 ext_param_page_length;  
	u8 num_of_param_pages;         
	u8 reserved1[17];
#else  
	u8 reserved[22];
#endif  

	char manufacturer[12];
	char model[20];
	u8 jedec_id;
	__le16 date_code;
	u8 reserved2[13];

	__le32 byte_per_page;
	__le16 spare_bytes_per_page;
	__le32 data_bytes_per_ppage;
	__le16 spare_bytes_per_ppage;
	__le32 pages_per_block;
	__le32 blocks_per_lun;
	u8 lun_count;
	u8 addr_cycles;
	u8 bits_per_cell;
	__le16 bb_per_lun;
	__le16 block_endurance;
	u8 guaranteed_good_blocks;
	__le16 guaranteed_block_endurance;
	u8 programs_per_page;
	u8 ppage_attr;
	u8 ecc_bits;
	u8 interleaved_bits;
	u8 interleaved_ops;
	u8 reserved3[13];

	u8 io_pin_capacitance_max;
	__le16 async_timing_mode;
	__le16 program_cache_timing_mode;
	__le16 t_prog;
	__le16 t_bers;
	__le16 t_r;
	__le16 t_ccs;
	__le16 src_sync_timing_mode;
	__le16 src_ssync_features;
	__le16 clk_pin_capacitance_typ;
	__le16 io_pin_capacitance_typ;
	__le16 input_pin_capacitance_typ;
	u8 input_pin_capacitance_max;
	u8 driver_strenght_support;
	__le16 t_int_r;
	__le16 t_ald;
	u8 reserved4[7];

	u8 reserved5[90];

	__le16 crc;
} __attribute__((packed));

#define ONFI_CRC_BASE	0x4F4E
#if defined (MY_DEF_HERE)

struct nand_timing_spec {
	int	tR;		 
	int	tCLS;		 
	int	tCS;		 
	int	tALS;		 
	int	tDS;		 
	int	tWP;		 
	int	tCLH;		 
	int	tCH;		 
	int	tALH;		 
	int	tDH;		 
	int	tWB;		 
	int	tWH;		 
	int	tWC;		 
	int	tRP;		 
	int	tREH;		 
	int	tRC;		 
	int	tREA;		 
	int	tRHOH;		 
	int	tCEA;		 
	int	tCOH;		 
	int	tCHZ;		 
	int	tCSD;		 
};

#define NAND_ONFI_TIMING_MODES		6
extern struct nand_timing_spec nand_onfi_timing_specs[];
#endif  

#if defined(MY_ABC_HERE)
 
struct onfi_ext_ecc_info {
	u8 ecc_bits;
	u8 codeword_size;
	__le16 bb_per_lun;
	__le16 block_endurance;
	u8 reserved[2];
} __packed;

#define ONFI_SECTION_TYPE_0	0	 
#define ONFI_SECTION_TYPE_1	1	 
#define ONFI_SECTION_TYPE_2	2	 
struct onfi_ext_section {
	u8 type;
	u8 length;
} __packed;

#define ONFI_EXT_SECTION_MAX 8

struct onfi_ext_param_page {
	__le16 crc;
	u8 sig[4];              
	u8 reserved0[10];
	struct onfi_ext_section sections[ONFI_EXT_SECTION_MAX];

} __packed;
#endif  

struct nand_hw_control {
	spinlock_t lock;
	struct nand_chip *active;
	wait_queue_head_t wq;
};

struct nand_ecc_ctrl {
	nand_ecc_modes_t mode;
	int steps;
	int size;
	int bytes;
	int total;
	int strength;
	int prepad;
	int postpad;
	struct nand_ecclayout	*layout;
	void *priv;
	void (*hwctl)(struct mtd_info *mtd, int mode);
	int (*calculate)(struct mtd_info *mtd, const uint8_t *dat,
			uint8_t *ecc_code);
	int (*correct)(struct mtd_info *mtd, uint8_t *dat, uint8_t *read_ecc,
			uint8_t *calc_ecc);
	int (*read_page_raw)(struct mtd_info *mtd, struct nand_chip *chip,
			uint8_t *buf, int oob_required, int page);
	int (*write_page_raw)(struct mtd_info *mtd, struct nand_chip *chip,
			const uint8_t *buf, int oob_required);
	int (*read_page)(struct mtd_info *mtd, struct nand_chip *chip,
			uint8_t *buf, int oob_required, int page);
	int (*read_subpage)(struct mtd_info *mtd, struct nand_chip *chip,
			uint32_t offs, uint32_t len, uint8_t *buf);
	int (*write_subpage)(struct mtd_info *mtd, struct nand_chip *chip,
			uint32_t offset, uint32_t data_len,
			const uint8_t *data_buf, int oob_required);
	int (*write_page)(struct mtd_info *mtd, struct nand_chip *chip,
			const uint8_t *buf, int oob_required);
	int (*write_oob_raw)(struct mtd_info *mtd, struct nand_chip *chip,
			int page);
	int (*read_oob_raw)(struct mtd_info *mtd, struct nand_chip *chip,
			int page);
	int (*read_oob)(struct mtd_info *mtd, struct nand_chip *chip, int page);
	int (*write_oob)(struct mtd_info *mtd, struct nand_chip *chip,
			int page);
};

struct nand_buffers {
	uint8_t	ecccalc[NAND_MAX_OOBSIZE];
	uint8_t	ecccode[NAND_MAX_OOBSIZE];
	uint8_t databuf[NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE];
};

struct nand_chip {
	void __iomem *IO_ADDR_R;
	void __iomem *IO_ADDR_W;

	uint8_t (*read_byte)(struct mtd_info *mtd);
	u16 (*read_word)(struct mtd_info *mtd);
	void (*write_buf)(struct mtd_info *mtd, const uint8_t *buf, int len);
	void (*read_buf)(struct mtd_info *mtd, uint8_t *buf, int len);
#if defined(CONFIG_SYNO_LSP_HI3536)
	int (*verify_buf)(struct mtd_info *mtd, const uint8_t *buf, int len);
#endif  
	void (*select_chip)(struct mtd_info *mtd, int chip);
	int (*block_bad)(struct mtd_info *mtd, loff_t ofs, int getchip);
	int (*block_markbad)(struct mtd_info *mtd, loff_t ofs);
	void (*cmd_ctrl)(struct mtd_info *mtd, int dat, unsigned int ctrl);
	int (*init_size)(struct mtd_info *mtd, struct nand_chip *this,
			u8 *id_data);
	int (*dev_ready)(struct mtd_info *mtd);
	void (*cmdfunc)(struct mtd_info *mtd, unsigned command, int column,
			int page_addr);
	int(*waitfunc)(struct mtd_info *mtd, struct nand_chip *this);
	void (*erase_cmd)(struct mtd_info *mtd, int page);
	int (*scan_bbt)(struct mtd_info *mtd);
	int (*errstat)(struct mtd_info *mtd, struct nand_chip *this, int state,
			int status, int page);
	int (*write_page)(struct mtd_info *mtd, struct nand_chip *chip,
			uint32_t offset, int data_len, const uint8_t *buf,
			int oob_required, int page, int cached, int raw);
	int (*onfi_set_features)(struct mtd_info *mtd, struct nand_chip *chip,
			int feature_addr, uint8_t *subfeature_para);
	int (*onfi_get_features)(struct mtd_info *mtd, struct nand_chip *chip,
			int feature_addr, uint8_t *subfeature_para);

	int chip_delay;
	unsigned int options;
#if defined(MY_ABC_HERE) && defined(CONFIG_MTD_NAND_NFC_MLC_SUPPORT)
	unsigned int	oobsize_ovrd;
	unsigned int	bb_location;
	unsigned int	bb_page;
#endif  
	unsigned int bbt_options;
#if defined (MY_DEF_HERE)
	unsigned int bbm;
#endif  

	int page_shift;
	int phys_erase_shift;
	int bbt_erase_shift;
	int chip_shift;
	int numchips;
	uint64_t chipsize;
	int pagemask;
	int pagebuf;
	unsigned int pagebuf_bitflips;
	int subpagesize;
	uint8_t cellinfo;
#if defined(MY_ABC_HERE)
	uint16_t ecc_strength_ds;
	uint16_t ecc_step_ds;
#endif  
	int badblockpos;
	int badblockbits;
#if defined (MY_DEF_HERE)
	int planes_per_chip;
	int luns_per_chip;
#endif  

	int onfi_version;
	struct nand_onfi_params	onfi_params;

	flstate_t state;

	uint8_t *oob_poi;
	struct nand_hw_control *controller;
	struct nand_ecclayout *ecclayout;

	struct nand_ecc_ctrl ecc;
	struct nand_buffers *buffers;
	struct nand_hw_control hwcontrol;

	uint8_t *bbt;
	struct nand_bbt_descr *bbt_td;
	struct nand_bbt_descr *bbt_md;

	struct nand_bbt_descr *badblock_pattern;

	void *priv;
};

#define NAND_MFR_TOSHIBA	0x98
#define NAND_MFR_SAMSUNG	0xec
#define NAND_MFR_FUJITSU	0x04
#define NAND_MFR_NATIONAL	0x8f
#define NAND_MFR_RENESAS	0x07
#define NAND_MFR_STMICRO	0x20
#define NAND_MFR_HYNIX		0xad
#define NAND_MFR_MICRON		0x2c
#define NAND_MFR_AMD		0x01
#define NAND_MFR_MACRONIX	0xc2
#if defined(CONFIG_SYNO_LSP_HI3536)
#define NAND_MFR_GD		0xc8
#endif  
#define NAND_MFR_EON		0x92
#if defined(CONFIG_SYNO_LSP_HI3536)
#define NAND_MFR_ESMT		0xC8
#define NAND_MFR_WINBOND	0xef
#define NAND_MFR_ATO		0x9b
#endif  

#define NAND_MAX_ID_LEN 8

#define LEGACY_ID_NAND(nm, devid, chipsz, erasesz, opts)          \
	{ .name = (nm), {{ .dev_id = (devid) }}, .pagesize = 512, \
	  .chipsize = (chipsz), .erasesize = (erasesz), .options = (opts) }

#define EXTENDED_ID_NAND(nm, devid, chipsz, opts)                      \
	{ .name = (nm), {{ .dev_id = (devid) }}, .chipsize = (chipsz), \
	  .options = (opts) }

struct nand_flash_dev {
	char *name;
	union {
		struct {
			uint8_t mfr_id;
			uint8_t dev_id;
		};
		uint8_t id[NAND_MAX_ID_LEN];
	};
	unsigned int pagesize;
	unsigned int chipsize;
	unsigned int erasesize;
	unsigned int options;
	uint16_t id_len;
	uint16_t oobsize;
};

struct nand_manufacturers {
	int id;
	char *name;
};

extern struct nand_flash_dev nand_flash_ids[];
extern struct nand_manufacturers nand_manuf_ids[];
#if defined (MY_DEF_HERE)
extern int nand_decode_readid(struct mtd_info *mtd, struct nand_chip *chip,
			      struct nand_flash_dev *type, uint8_t *id,
			      int max_id_len);
extern void nand_derive_bbm(struct mtd_info *mtd, struct nand_chip *chip,
			    uint8_t *id);
#endif  

extern int nand_scan_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd);
extern int nand_update_bbt(struct mtd_info *mtd, loff_t offs);
extern int nand_default_bbt(struct mtd_info *mtd);
extern int nand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt);
extern int nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr,
			   int allowbbt);
extern int nand_do_read(struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, uint8_t *buf);
#if defined (MY_DEF_HERE)
extern void nand_sync(struct mtd_info *mtd);
extern int nand_suspend(struct mtd_info *mtd);
extern void nand_resume(struct mtd_info *mtd);

extern uint8_t *nand_transfer_oob(struct nand_chip *chip, uint8_t *oob,
				  struct mtd_oob_ops *ops, size_t len);
extern int nand_check_wp(struct mtd_info *mtd);
extern uint8_t *nand_fill_oob(struct mtd_info *mtd, uint8_t *oob, size_t len,
			      struct mtd_oob_ops *ops);
extern int nand_do_write_oob(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops);
extern int nand_get_device(struct mtd_info *mtd, int new_state);
extern void nand_release_device(struct mtd_info *mtd);
extern u8 nand_erasebb;
#endif  

struct platform_nand_chip {
	int nr_chips;
	int chip_offset;
	int nr_partitions;
	struct mtd_partition *partitions;
	struct nand_ecclayout *ecclayout;
	int chip_delay;
	unsigned int options;
	unsigned int bbt_options;
	const char **part_probe_types;
};

struct platform_device;

struct platform_nand_ctrl {
	int (*probe)(struct platform_device *pdev);
	void (*remove)(struct platform_device *pdev);
	void (*hwcontrol)(struct mtd_info *mtd, int cmd);
	int (*dev_ready)(struct mtd_info *mtd);
	void (*select_chip)(struct mtd_info *mtd, int chip);
	void (*cmd_ctrl)(struct mtd_info *mtd, int dat, unsigned int ctrl);
	void (*write_buf)(struct mtd_info *mtd, const uint8_t *buf, int len);
	void (*read_buf)(struct mtd_info *mtd, uint8_t *buf, int len);
	unsigned char (*read_byte)(struct mtd_info *mtd);
	void *priv;
};

struct platform_nand_data {
	struct platform_nand_chip chip;
	struct platform_nand_ctrl ctrl;
};

static inline
struct platform_nand_chip *get_platform_nandchip(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	return chip->priv;
}

#if defined(MY_ABC_HERE)
 
static inline int onfi_feature(struct nand_chip *chip)
{
	return chip->onfi_version ? le16_to_cpu(chip->onfi_params.features) : 0;
}
#endif  

static inline int onfi_get_async_timing_mode(struct nand_chip *chip)
{
	if (!chip->onfi_version)
		return ONFI_TIMING_MODE_UNKNOWN;
	return le16_to_cpu(chip->onfi_params.async_timing_mode);
}

static inline int onfi_get_sync_timing_mode(struct nand_chip *chip)
{
	if (!chip->onfi_version)
		return ONFI_TIMING_MODE_UNKNOWN;
	return le16_to_cpu(chip->onfi_params.src_sync_timing_mode);
}

#endif  

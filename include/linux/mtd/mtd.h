#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef __MTD_MTD_H__
#define __MTD_MTD_H__

#include <linux/types.h>
#include <linux/uio.h>
#include <linux/notifier.h>
#include <linux/device.h>

#include <mtd/mtd-abi.h>

#include <asm/div64.h>

#define MTD_CHAR_MAJOR 90
#define MTD_BLOCK_MAJOR 31

#define MTD_ERASE_PENDING	0x01
#define MTD_ERASING		0x02
#define MTD_ERASE_SUSPEND	0x04
#define MTD_ERASE_DONE		0x08
#define MTD_ERASE_FAILED	0x10

#define MTD_FAIL_ADDR_UNKNOWN -1LL

struct erase_info {
	struct mtd_info *mtd;
	uint64_t addr;
	uint64_t len;
	uint64_t fail_addr;
	u_long time;
	u_long retries;
	unsigned dev;
	unsigned cell;
	void (*callback) (struct erase_info *self);
	u_long priv;
	u_char state;
	struct erase_info *next;
#if defined(MY_ABC_HERE)
	u8 *erase_buf;
	u32 erase_buf_ofs;
	bool partial_start;
#endif
};

struct mtd_erase_region_info {
	uint64_t offset;		 
	uint32_t erasesize;		 
	uint32_t numblocks;		 
	unsigned long *lockmap;		 
};

struct mtd_oob_ops {
	unsigned int	mode;
	size_t		len;
	size_t		retlen;
	size_t		ooblen;
	size_t		oobretlen;
	uint32_t	ooboffs;
	uint8_t		*datbuf;
	uint8_t		*oobbuf;
};

#define MTD_MAX_OOBFREE_ENTRIES_LARGE	32
#define MTD_MAX_ECCPOS_ENTRIES_LARGE	448
 
struct nand_ecclayout {
	__u32 eccbytes;
	__u32 eccpos[MTD_MAX_ECCPOS_ENTRIES_LARGE];
	__u32 oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES_LARGE];
};

struct module;	 

#if defined(MY_ABC_HERE)
struct mtd_info;
#endif
struct mtd_info {
	u_char type;
	uint32_t flags;
	uint64_t size;	  

	uint32_t erasesize;
	 
	uint32_t writesize;

	uint32_t writebufsize;

	uint32_t oobsize;    
	uint32_t oobavail;   

	unsigned int erasesize_shift;
	unsigned int writesize_shift;
	 
	unsigned int erasesize_mask;
	unsigned int writesize_mask;

	const char *name;
	int index;

	struct nand_ecclayout *ecclayout;

	int numeraseregions;
	struct mtd_erase_region_info *eraseregions;

	int (*erase) (struct mtd_info *mtd, struct erase_info *instr);

	int (*point) (struct mtd_info *mtd, loff_t from, size_t len,
			size_t *retlen, void **virt, resource_size_t *phys);

	void (*unpoint) (struct mtd_info *mtd, loff_t from, size_t len);

	unsigned long (*get_unmapped_area) (struct mtd_info *mtd,
					    unsigned long len,
					    unsigned long offset,
					    unsigned long flags);

	struct backing_dev_info *backing_dev_info;

	int (*read) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*write) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

	int (*panic_write) (struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf);

	int (*read_oob) (struct mtd_info *mtd, loff_t from,
			 struct mtd_oob_ops *ops);
	int (*write_oob) (struct mtd_info *mtd, loff_t to,
			 struct mtd_oob_ops *ops);

	int (*get_fact_prot_info) (struct mtd_info *mtd, struct otp_info *buf, size_t len);
	int (*read_fact_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*get_user_prot_info) (struct mtd_info *mtd, struct otp_info *buf, size_t len);
	int (*read_user_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*write_user_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf);
	int (*lock_user_prot_reg) (struct mtd_info *mtd, loff_t from, size_t len);

	int (*writev) (struct mtd_info *mtd, const struct kvec *vecs, unsigned long count, loff_t to, size_t *retlen);

	void (*sync) (struct mtd_info *mtd);

	int (*lock) (struct mtd_info *mtd, loff_t ofs, uint64_t len);
	int (*unlock) (struct mtd_info *mtd, loff_t ofs, uint64_t len);
	int (*is_locked) (struct mtd_info *mtd, loff_t ofs, uint64_t len);

	int (*suspend) (struct mtd_info *mtd);
	void (*resume) (struct mtd_info *mtd);

	int (*block_isbad) (struct mtd_info *mtd, loff_t ofs);
	int (*block_markbad) (struct mtd_info *mtd, loff_t ofs);

	struct notifier_block reboot_notifier;   

	struct mtd_ecc_stats ecc_stats;
	 
	int subpage_sft;

	void *priv;

	struct module *owner;
	struct device dev;
	int usecount;

#if defined(MY_ABC_HERE)
	int (*refresh_device)(struct mtd_info *mtd);
	struct mtd_info *split;
#endif

	int (*get_device) (struct mtd_info *mtd);
	void (*put_device) (struct mtd_info *mtd);
};

static inline struct mtd_info *dev_to_mtd(struct device *dev)
{
	return dev ? dev_get_drvdata(dev) : NULL;
}

static inline uint32_t mtd_div_by_eb(uint64_t sz, struct mtd_info *mtd)
{
	if (mtd->erasesize_shift)
		return sz >> mtd->erasesize_shift;
	do_div(sz, mtd->erasesize);
	return sz;
}

static inline uint32_t mtd_mod_by_eb(uint64_t sz, struct mtd_info *mtd)
{
	if (mtd->erasesize_shift)
		return sz & mtd->erasesize_mask;
	return do_div(sz, mtd->erasesize);
}

static inline uint32_t mtd_div_by_ws(uint64_t sz, struct mtd_info *mtd)
{
	if (mtd->writesize_shift)
		return sz >> mtd->writesize_shift;
	do_div(sz, mtd->writesize);
	return sz;
}

static inline uint32_t mtd_mod_by_ws(uint64_t sz, struct mtd_info *mtd)
{
	if (mtd->writesize_shift)
		return sz & mtd->writesize_mask;
	return do_div(sz, mtd->writesize);
}

struct mtd_partition;
struct mtd_part_parser_data;

extern int mtd_device_parse_register(struct mtd_info *mtd,
			      const char **part_probe_types,
			      struct mtd_part_parser_data *parser_data,
			      const struct mtd_partition *defparts,
			      int defnr_parts);
#define mtd_device_register(master, parts, nr_parts)	\
	mtd_device_parse_register(master, NULL, NULL, parts, nr_parts)
#if defined(MY_ABC_HERE)
extern int mtd_device_refresh(struct mtd_info *master);
#endif
extern int mtd_device_unregister(struct mtd_info *master);
extern struct mtd_info *get_mtd_device(struct mtd_info *mtd, int num);
extern int __get_mtd_device(struct mtd_info *mtd);
extern void __put_mtd_device(struct mtd_info *mtd);
extern struct mtd_info *get_mtd_device_nm(const char *name);
extern void put_mtd_device(struct mtd_info *mtd);

struct mtd_notifier {
	void (*add)(struct mtd_info *mtd);
	void (*remove)(struct mtd_info *mtd);
	struct list_head list;
};

extern void register_mtd_user (struct mtd_notifier *new);
extern int unregister_mtd_user (struct mtd_notifier *old);

int default_mtd_writev(struct mtd_info *mtd, const struct kvec *vecs,
		       unsigned long count, loff_t to, size_t *retlen);

int default_mtd_readv(struct mtd_info *mtd, struct kvec *vecs,
		      unsigned long count, loff_t from, size_t *retlen);

void *mtd_kmalloc_up_to(const struct mtd_info *mtd, size_t *size);

void mtd_erase_callback(struct erase_info *instr);

static inline int mtd_is_bitflip(int err) {
	return err == -EUCLEAN;
}

static inline int mtd_is_eccerr(int err) {
	return err == -EBADMSG;
}

static inline int mtd_is_bitflip_or_eccerr(int err) {
	return mtd_is_bitflip(err) || mtd_is_eccerr(err);
}

#ifdef MY_ABC_HERE
 
int SYNOMTDModifyPartInfo(struct mtd_info *mtd, unsigned long offset, unsigned long length);

int SYNOMTDModifyFisInfo(struct mtd_info *mtd, struct SYNO_MTD_FIS_INFO SynoMtdFisInfo);
#endif  

#endif  

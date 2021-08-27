 
#ifndef __ASMARM_SETUP_H
#define __ASMARM_SETUP_H

#include <linux/types.h>

#define COMMAND_LINE_SIZE 1024

#define ATAG_NONE	0x00000000

struct tag_header {
	__u32 size;
	__u32 tag;
};

#define ATAG_CORE	0x54410001

struct tag_core {
	__u32 flags;		 
	__u32 pagesize;
	__u32 rootdev;
};

#define ATAG_MEM	0x54410002

struct tag_mem32 {
	__u32	size;
	__u32	start;	 
};

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#define ATAG_MEM64	0x54420002

struct tag_mem64 {
	__u64	size;
	__u64	start;	 
};
#endif

#define ATAG_VIDEOTEXT	0x54410003

struct tag_videotext {
	__u8		x;
	__u8		y;
	__u16		video_page;
	__u8		video_mode;
	__u8		video_cols;
	__u16		video_ega_bx;
	__u8		video_lines;
	__u8		video_isvga;
	__u16		video_points;
};

#define ATAG_RAMDISK	0x54410004

struct tag_ramdisk {
	__u32 flags;	 
	__u32 size;	 
	__u32 start;	 
};

#define ATAG_INITRD	0x54410005

#define ATAG_INITRD2	0x54420005

struct tag_initrd {
	__u32 start;	 
	__u32 size;	 
};

#define ATAG_SERIAL	0x54410006

struct tag_serialnr {
	__u32 low;
	__u32 high;
};

#define ATAG_REVISION	0x54410007

struct tag_revision {
	__u32 rev;
};

#define ATAG_VIDEOLFB	0x54410008

struct tag_videolfb {
	__u16		lfb_width;
	__u16		lfb_height;
	__u16		lfb_depth;
	__u16		lfb_linelength;
	__u32		lfb_base;
	__u32		lfb_size;
	__u8		red_size;
	__u8		red_pos;
	__u8		green_size;
	__u8		green_pos;
	__u8		blue_size;
	__u8		blue_pos;
	__u8		rsvd_size;
	__u8		rsvd_pos;
};

#define ATAG_CMDLINE	0x54410009

struct tag_cmdline {
	char	cmdline[1];	 
};

#define ATAG_ACORN	0x41000101

struct tag_acorn {
	__u32 memc_control_reg;
	__u32 vram_pages;
	__u8 sounddefault;
	__u8 adfsdrives;
};

#define ATAG_MEMCLK	0x41000402

struct tag_memclk {
	__u32 fmemclk;
};

#if defined(CONFIG_SYNO_ARMADA) || defined(CONFIG_SYNO_ARMADA_V2)
 
#define ATAG_MV_UBOOT   0x41000403
#define MV_UBOOT_ETH_PORTS	4
struct tag_mv_uboot {
        __u32 uboot_version;
        __u32 tclk;
        __u32 sysclk;
        __u32 isUsbHost;
        __u8  macAddr[MV_UBOOT_ETH_PORTS][6];
	__u16 mtu[MV_UBOOT_ETH_PORTS];
	__u32 nand_ecc;
#if !defined (CONFIG_ARCH_ARMADA370)
	__u32 rgmii0Src;
	__u32 feGeSrc;
#endif
#if defined (CONFIG_ARCH_ARMADA370)
	__u32 bit_mask_config;
#endif
};                     
#endif

struct tag {
	struct tag_header hdr;
	union {
		struct tag_core		core;
		struct tag_mem32	mem;
#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_V2)
		struct tag_mem64	mem64;
#endif
		struct tag_videotext	videotext;
		struct tag_ramdisk	ramdisk;
		struct tag_initrd	initrd;
		struct tag_serialnr	serialnr;
		struct tag_revision	revision;
		struct tag_videolfb	videolfb;
		struct tag_cmdline	cmdline;

		struct tag_acorn	acorn;

		struct tag_memclk	memclk;
#if defined(CONFIG_SYNO_ARMADA) || defined(CONFIG_SYNO_ARMADA_V2)
		 
		struct tag_mv_uboot	mv_uboot;
#endif
	} u;
};

struct tagtable {
	__u32 tag;
	int (*parse)(const struct tag *);
};

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#ifdef CONFIG_BE8_ON_LE
#define read_tag(a)	le32_to_cpu(a)
#else
#define read_tag(a)	a
#endif
#endif

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#define tag_member_present(tag,member)				\
	((unsigned long)(&((struct tag *)0L)->member + 1)	\
		<= read_tag((tag)->hdr.size) * 4)
#define tag_next(t)     ((struct tag *)((__u32 *)(t) + read_tag((t)->hdr.size)))
#else
#define tag_member_present(tag,member)				\
 	((unsigned long)(&((struct tag *)0L)->member + 1)	\
		<= (tag)->hdr.size * 4)

#define tag_next(t)	((struct tag *)((__u32 *)(t) + (t)->hdr.size))
#endif
#define tag_size(type)	((sizeof(struct tag_header) + sizeof(struct type)) >> 2)

#if defined(CONFIG_SYNO_ARMADA_ARCH) || defined(CONFIG_SYNO_ARMADA_ARCH_V2)
#define for_each_tag(t,base)		\
	for (t = base; read_tag((t)->hdr.size); t = tag_next(t))
#else
#define for_each_tag(t,base)		\
	for (t = base; t->hdr.size; t = tag_next(t))
#endif

#ifdef __KERNEL__

#define __tag __used __attribute__((__section__(".taglist.init")))
#define __tagtable(tag, fn) \
static const struct tagtable __tagtable_##fn __tag = { tag, fn }

#ifdef CONFIG_ARCH_EP93XX
# define NR_BANKS 16
#else
# define NR_BANKS 8
#endif

struct membank {
	phys_addr_t start;
#ifdef CONFIG_SYNO_ALPINE
	phys_addr_t size;
#else
	unsigned long size;
#endif
	unsigned int highmem;
};

struct meminfo {
	int nr_banks;
	struct membank bank[NR_BANKS];
};

extern struct meminfo meminfo;

#define for_each_bank(iter,mi)				\
	for (iter = 0; iter < (mi)->nr_banks; iter++)

#define bank_pfn_start(bank)	__phys_to_pfn((bank)->start)
#define bank_pfn_end(bank)	__phys_to_pfn((bank)->start + (bank)->size)
#define bank_pfn_size(bank)	((bank)->size >> PAGE_SHIFT)
#define bank_phys_start(bank)	(bank)->start
#define bank_phys_end(bank)	((bank)->start + (bank)->size)
#define bank_phys_size(bank)	(bank)->size

#ifdef CONFIG_SYNO_ALPINE
extern int arm_add_memory(phys_addr_t start, phys_addr_t size);
#else
extern int arm_add_memory(phys_addr_t start, unsigned long size);
#endif
extern void early_print(const char *str, ...);
extern void dump_machine_table(void);

#endif   

#endif

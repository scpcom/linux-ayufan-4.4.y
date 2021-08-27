#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef MTD_PARTITIONS_H
#define MTD_PARTITIONS_H

#include <linux/types.h>

#if defined(MY_ABC_HERE)
struct mtd_info;

struct mtd_partition;
#endif

struct mtd_partition {
	char *name;			 
	uint64_t size;			 
	uint64_t offset;		 
	uint32_t mask_flags;		 
	struct nand_ecclayout *ecclayout;	 
#if defined(MY_ABC_HERE)
	int (*refresh_partition)(struct mtd_info *);
#endif
};

#define MTDPART_OFS_RETAIN	(-3)
#define MTDPART_OFS_NXTBLK	(-2)
#define MTDPART_OFS_APPEND	(-1)
#define MTDPART_SIZ_FULL	(0)

#if !defined(MY_ABC_HERE)
struct mtd_info;
#endif
struct device_node;

struct mtd_part_parser_data {
	unsigned long origin;
	struct device_node *of_node;
};

struct mtd_part_parser {
	struct list_head list;
	struct module *owner;
	const char *name;
	int (*parse_fn)(struct mtd_info *, struct mtd_partition **,
			struct mtd_part_parser_data *);
};

extern int register_mtd_parser(struct mtd_part_parser *parser);
extern int deregister_mtd_parser(struct mtd_part_parser *parser);

int mtd_is_partition(const struct mtd_info *mtd);
int mtd_add_partition(struct mtd_info *master, char *name,
		      long long offset, long long length);
int mtd_del_partition(struct mtd_info *master, int partno);
uint64_t mtd_get_device_size(const struct mtd_info *mtd);

#endif

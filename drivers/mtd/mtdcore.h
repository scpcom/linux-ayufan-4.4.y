/*
 * These are exported solely for the purpose of mtd_blkdevs.c and mtdchar.c.
 * You should not use them for _anything_ else.
 */

extern struct mutex mtd_table_mutex;

struct mtd_info *__mtd_next_device(int i);
int add_mtd_device(struct mtd_info *mtd);
int del_mtd_device(struct mtd_info *mtd);
int add_mtd_partitions(struct mtd_info *, const struct mtd_partition *, int);
int del_mtd_partitions(struct mtd_info *);
#if defined(MY_DEF_HERE)

struct mtd_partitions;

#endif /* MY_DEF_HERE */
int parse_mtd_partitions(struct mtd_info *master, const char * const *types,
#if defined(MY_DEF_HERE)
			 struct mtd_partitions *pparts,
#else /* MY_DEF_HERE */
			 struct mtd_partition **pparts,
#endif /* MY_DEF_HERE */
			 struct mtd_part_parser_data *data);

#if defined(MY_DEF_HERE)
void mtd_part_parser_cleanup(struct mtd_partitions *parts);

#endif /* MY_DEF_HERE */
int __init init_mtdchar(void);
void __exit cleanup_mtdchar(void);

#define mtd_for_each_device(mtd)			\
	for ((mtd) = __mtd_next_device(0);		\
	     (mtd) != NULL;				\
	     (mtd) = __mtd_next_device(mtd->index + 1))

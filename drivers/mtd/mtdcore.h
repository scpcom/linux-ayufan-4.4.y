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
#if defined(CONFIG_SYNO_LSP_RTD1619)

struct mtd_partitions;

#endif /* CONFIG_SYNO_LSP_RTD1619 */
int parse_mtd_partitions(struct mtd_info *master, const char * const *types,
#if defined(CONFIG_SYNO_LSP_RTD1619)
			 struct mtd_partitions *pparts,
#else /* CONFIG_SYNO_LSP_RTD1619 */
			 struct mtd_partition **pparts,
#endif /* CONFIG_SYNO_LSP_RTD1619 */
			 struct mtd_part_parser_data *data);

#if defined(CONFIG_SYNO_LSP_RTD1619)
void mtd_part_parser_cleanup(struct mtd_partitions *parts);

#endif /* CONFIG_SYNO_LSP_RTD1619 */
int __init init_mtdchar(void);
void __exit cleanup_mtdchar(void);

#define mtd_for_each_device(mtd)			\
	for ((mtd) = __mtd_next_device(0);		\
	     (mtd) != NULL;				\
	     (mtd) = __mtd_next_device(mtd->index + 1))

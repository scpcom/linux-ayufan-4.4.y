#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef GCOV_H
#define GCOV_H GCOV_H

#include <linux/types.h>

#if defined (MY_DEF_HERE)
#else  
#define GCOV_COUNTERS		5
#endif  
#define GCOV_DATA_MAGIC		((unsigned int) 0x67636461)
#define GCOV_TAG_FUNCTION	((unsigned int) 0x01000000)
#define GCOV_TAG_COUNTER_BASE	((unsigned int) 0x01a10000)
#define GCOV_TAG_FOR_COUNTER(count)					\
	(GCOV_TAG_COUNTER_BASE + ((unsigned int) (count) << 17))

#if BITS_PER_LONG >= 64
typedef long gcov_type;
#else
typedef long long gcov_type;
#endif

#if defined (MY_DEF_HERE)
 
struct gcov_info;

const char *gcov_info_filename(struct gcov_info *info);
unsigned int gcov_info_version(struct gcov_info *info);
struct gcov_info *gcov_info_next(struct gcov_info *info);
void gcov_info_link(struct gcov_info *info);
void gcov_info_unlink(struct gcov_info *prev, struct gcov_info *info);

#else  
 
struct gcov_fn_info {
	unsigned int ident;
	unsigned int checksum;
	unsigned int n_ctrs[0];
};

struct gcov_ctr_info {
	unsigned int	num;
	gcov_type	*values;
	void		(*merge)(gcov_type *, unsigned int);
};

struct gcov_info {
	unsigned int			version;
	struct gcov_info		*next;
	unsigned int			stamp;
	const char			*filename;
	unsigned int			n_functions;
	const struct gcov_fn_info	*functions;
	unsigned int			ctr_mask;
	struct gcov_ctr_info		counts[0];
};
#endif  

enum gcov_action {
	GCOV_ADD,
	GCOV_REMOVE,
};

void gcov_event(enum gcov_action action, struct gcov_info *info);
void gcov_enable_events(void);

struct seq_file;
struct gcov_iterator;

struct gcov_iterator *gcov_iter_new(struct gcov_info *info);
void gcov_iter_free(struct gcov_iterator *iter);
void gcov_iter_start(struct gcov_iterator *iter);
int gcov_iter_next(struct gcov_iterator *iter);
int gcov_iter_write(struct gcov_iterator *iter, struct seq_file *seq);
struct gcov_info *gcov_iter_get_info(struct gcov_iterator *iter);

void gcov_info_reset(struct gcov_info *info);
int gcov_info_is_compatible(struct gcov_info *info1, struct gcov_info *info2);
void gcov_info_add(struct gcov_info *dest, struct gcov_info *source);
struct gcov_info *gcov_info_dup(struct gcov_info *info);
void gcov_info_free(struct gcov_info *info);

struct gcov_link {
	enum {
		OBJ_TREE,
		SRC_TREE,
	} dir;
	const char *ext;
};
extern const struct gcov_link gcov_link[];

#endif  

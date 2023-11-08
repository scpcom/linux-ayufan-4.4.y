/*
 * qdm_main.h
 */

#ifndef _LINUX_QDM_H
#define _LINUX_QDM_H

#include <linux/blkdev.h>
#include <linux/device-mapper.h>
#include <linux/dm-io.h>

//#define QDM_DEBUG

#ifdef	QDM_DEBUG

#define QDM_DBG(format, arg...) \
		do { \
			printk(KERN_DEBUG "%s: " format "\n" , __FILE__, ## arg); \
		} while (0)

#define ASSERT(expr)    \
		do {	\
			if (!(expr)) { \
				printk(KERN_ALERT "assertion failed! %s[%d]: %s\n", \
				__func__, __LINE__, #expr); \
        	}		\
		} while (0)
#else

#define ASSERT(expr)		do {} while (0)
#define QDM_DBG(format, arg...)	do {} while (0)

#endif

typedef sector_t chunk_t;

#define QDM_OPERATION_VERSION		0x10001

#define FOUND                   	1	//Boolean
#define NOT_FOUND               	0

#define OK                      	0	//Integer
#define ERROR                   	-1

#define QDM_MAPOP_STATUS_BEGIN		0x30001
#define QDM_MAPOP_STATUS_END		0x30003

#define QDM_OPERATION_TYPE_BEGIN	0x50001
#define QDM_OPERATION_TYPE_END		0x50005

#define QDM_VOLUMN_TYPE_BEGIN		0x1
#define QDM_VOLUMN_TYPE_END		0x4

#define QDM_STATUS_BEGIN		0x1
#define QDM_STATUS_END			0x4

enum QDM_MAPOP_STATUS {
	QDM_MAPOP_CONTINUE = QDM_MAPOP_STATUS_BEGIN,
	QDM_MAPOP_PENDING,
};

enum QDM_OPERATION_TYPE {
	QDM_OPERATION_TYPE_NONE = QDM_OPERATION_TYPE_BEGIN,
	QDM_OPERATION_TYPE_REVERSE,
	QDM_OPERATION_TYPE_SNAP,
	QDM_OPERATION_TYPE_CLONE,
};

enum QDM_VOLUME_TYPE {
	QDM_VOLUMN_TYPE_ORIGIN = QDM_VOLUMN_TYPE_BEGIN,
	QDM_VOLUMN_TYPE_SNAP,
	QDM_VOLUMN_TYPE_CLONE,
};

enum QDM_STATUS {
	QDM_STATUS_NORMAL = QDM_STATUS_BEGIN,
	QDM_STATUS_PENDING,
	QDM_STATUS_ERROR,
};

/*
 * Used data structure
 */

struct qdm_operation_tag;
struct qdm_operation_c;

typedef int (*qdm_callback_fn) (struct qdm_operation_tag * op_tag);
typedef int (*op_map_fn) (struct qdm_operation_tag * op_tag, struct bio * bio,
			  union map_info * map_context);

typedef void (*op_dtr_fn) (struct qdm_operation_c * op);
typedef int (*op_suspend_fn) (struct qdm_operation_c * op);
typedef int (*op_resume_fn) (struct qdm_operation_c * op);

struct qdm_operation_c {
	unsigned long long op_version;
	unsigned int op_type;
	struct list_head op_link;
	struct dm_target *origin_ti;
	const char *name;
	op_map_fn op_map;
	op_dtr_fn op_dtr;
	op_suspend_fn op_suspend;
	op_resume_fn op_resume;
	void *private;
};

struct qdm_operation_tag {
	struct qdm_operation_c *op;
	struct bio *bio;
	unsigned long timestamp;
	qdm_callback_fn callback;	//TODO if need
	void *private;
};

int qdm_op_register(struct dm_target *ti, struct qdm_operation_c *op);
void qdm_op_unregister(struct dm_target *ti, struct qdm_operation_c *op);
struct qdm_operation_c *qdm_allocate_op(void);
void qdm_free_op(struct qdm_operation_c *op);
void qdm_open_origin(struct dm_target *ti);
void qdm_close_origin(struct dm_target *ti);
#endif /* _LINUX_QDM_H */

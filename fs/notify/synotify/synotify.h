#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifdef MY_ABC_HERE
#include <linux/fsnotify_backend.h>
#include <linux/path.h>
#include <linux/slab.h>
#include "../../mount.h"

/* flags used for synotify_set_mark() */
#define SYNOTIFY_MARK_ADD		0x00000001
#define SYNOTIFY_MARK_REMOVE	0x00000002

/*
 * Structure for normal synotify events. It gets allocated in
 * synotify_handle_event() and freed when the information is retrieved by
 * userspace
 */
struct synotify_event_info {
	struct fsnotify_event fse;
	/*
	 * We hold ref to this path so it may be dereferenced at any point
	 * during this object's lifetime
	 */
	u32 sync_cookie;
	struct path path;
	const unsigned char *full_name;
	size_t full_name_len;
	/*
	 * for SYNOFetchFullName use file_name and data_type in synotify_merge
	 */
	const unsigned char *file_name;
	int data_type;
};

static inline struct synotify_event_info *SYNOTIFY_E(struct fsnotify_event *fse)
{
	return container_of(fse, struct synotify_event_info, fse);
}

struct synotify_event_info *synotify_alloc_event(struct inode *inode, u32 mask,
						 struct path *path, u32 cookie);
#endif /* MY_ABC_HERE */

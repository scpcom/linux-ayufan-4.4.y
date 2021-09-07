 
#include "dm.h"

#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/blkdev.h>
#include <linux/namei.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <asm/atomic.h>

#define DM_MSG_PREFIX "table"

#define MAX_DEPTH 16
#define NODE_SIZE L1_CACHE_BYTES
#define KEYS_PER_NODE (NODE_SIZE / sizeof(sector_t))
#define CHILDREN_PER_NODE (KEYS_PER_NODE + 1)

struct dm_table {
	struct mapped_device *md;
	atomic_t holders;
	unsigned type;

	unsigned int depth;
	unsigned int counts[MAX_DEPTH];	 
	sector_t *index[MAX_DEPTH];

	unsigned int num_targets;
	unsigned int num_allocated;
	sector_t *highs;
	struct dm_target *targets;

	fmode_t mode;

	struct list_head devices;

	void (*event_fn)(void *);
	void *event_context;

	struct dm_md_mempools *mempools;
};

static unsigned int int_log(unsigned int n, unsigned int base)
{
	int result = 0;

	while (n > 1) {
		n = dm_div_up(n, base);
		result++;
	}

	return result;
}

static inline unsigned int get_child(unsigned int n, unsigned int k)
{
	return (n * CHILDREN_PER_NODE) + k;
}

static inline sector_t *get_node(struct dm_table *t,
				 unsigned int l, unsigned int n)
{
	return t->index[l] + (n * KEYS_PER_NODE);
}

static sector_t high(struct dm_table *t, unsigned int l, unsigned int n)
{
	for (; l < t->depth - 1; l++)
		n = get_child(n, CHILDREN_PER_NODE - 1);

	if (n >= t->counts[l])
		return (sector_t) - 1;

	return get_node(t, l, n)[KEYS_PER_NODE - 1];
}

static int setup_btree_index(unsigned int l, struct dm_table *t)
{
	unsigned int n, k;
	sector_t *node;

	for (n = 0U; n < t->counts[l]; n++) {
		node = get_node(t, l, n);

		for (k = 0U; k < KEYS_PER_NODE; k++)
			node[k] = high(t, l + 1, get_child(n, k));
	}

	return 0;
}

void *dm_vcalloc(unsigned long nmemb, unsigned long elem_size)
{
	unsigned long size;
	void *addr;

	if (nmemb > (ULONG_MAX / elem_size))
		return NULL;

	size = nmemb * elem_size;
	addr = vmalloc(size);
	if (addr)
		memset(addr, 0, size);

	return addr;
}

static int alloc_targets(struct dm_table *t, unsigned int num)
{
	sector_t *n_highs;
	struct dm_target *n_targets;
	int n = t->num_targets;

	n_highs = (sector_t *) dm_vcalloc(num + 1, sizeof(struct dm_target) +
					  sizeof(sector_t));
	if (!n_highs)
		return -ENOMEM;

	n_targets = (struct dm_target *) (n_highs + num);

	if (n) {
		memcpy(n_highs, t->highs, sizeof(*n_highs) * n);
		memcpy(n_targets, t->targets, sizeof(*n_targets) * n);
	}

	memset(n_highs + n, -1, sizeof(*n_highs) * (num - n));
	vfree(t->highs);

	t->num_allocated = num;
	t->highs = n_highs;
	t->targets = n_targets;

	return 0;
}

int dm_table_create(struct dm_table **result, fmode_t mode,
		    unsigned num_targets, struct mapped_device *md)
{
	struct dm_table *t = kzalloc(sizeof(*t), GFP_KERNEL);

	if (!t)
		return -ENOMEM;

	INIT_LIST_HEAD(&t->devices);
	atomic_set(&t->holders, 0);

	if (!num_targets)
		num_targets = KEYS_PER_NODE;

	num_targets = dm_round_up(num_targets, KEYS_PER_NODE);

	if (alloc_targets(t, num_targets)) {
		kfree(t);
		t = NULL;
		return -ENOMEM;
	}

	t->mode = mode;
	t->md = md;
	*result = t;
	return 0;
}

static void free_devices(struct list_head *devices)
{
	struct list_head *tmp, *next;

	list_for_each_safe(tmp, next, devices) {
		struct dm_dev_internal *dd =
		    list_entry(tmp, struct dm_dev_internal, list);
		DMWARN("dm_table_destroy: dm_put_device call missing for %s",
		       dd->dm_dev.name);
		kfree(dd);
	}
}

void dm_table_destroy(struct dm_table *t)
{
	unsigned int i;

	while (atomic_read(&t->holders))
		msleep(1);
	smp_mb();

	if (t->depth >= 2)
		vfree(t->index[t->depth - 2]);

	for (i = 0; i < t->num_targets; i++) {
		struct dm_target *tgt = t->targets + i;

		if (tgt->type->dtr)
			tgt->type->dtr(tgt);

		dm_put_target_type(tgt->type);
	}

	vfree(t->highs);

	if (t->devices.next != &t->devices)
		free_devices(&t->devices);

	dm_free_md_mempools(t->mempools);

	kfree(t);
}

void dm_table_get(struct dm_table *t)
{
	atomic_inc(&t->holders);
}

void dm_table_put(struct dm_table *t)
{
	if (!t)
		return;

	smp_mb__before_atomic_dec();
	atomic_dec(&t->holders);
}

static inline int check_space(struct dm_table *t)
{
	if (t->num_targets >= t->num_allocated)
		return alloc_targets(t, t->num_allocated * 2);

	return 0;
}

static struct dm_dev_internal *find_device(struct list_head *l, dev_t dev)
{
	struct dm_dev_internal *dd;

	list_for_each_entry (dd, l, list)
		if (dd->dm_dev.bdev->bd_dev == dev)
			return dd;

	return NULL;
}

static int open_dev(struct dm_dev_internal *d, dev_t dev,
		    struct mapped_device *md)
{
	static char *_claim_ptr = "I belong to device-mapper";
	struct block_device *bdev;

	int r;

	BUG_ON(d->dm_dev.bdev);

	bdev = open_by_devnum(dev, d->dm_dev.mode);
	if (IS_ERR(bdev))
		return PTR_ERR(bdev);
	r = bd_claim_by_disk(bdev, _claim_ptr, dm_disk(md));
	if (r)
		blkdev_put(bdev, d->dm_dev.mode);
	else
		d->dm_dev.bdev = bdev;
	return r;
}

static void close_dev(struct dm_dev_internal *d, struct mapped_device *md)
{
	if (!d->dm_dev.bdev)
		return;

	bd_release_from_disk(d->dm_dev.bdev, dm_disk(md));
	blkdev_put(d->dm_dev.bdev, d->dm_dev.mode);
	d->dm_dev.bdev = NULL;
}

static int device_area_is_invalid(struct dm_target *ti, struct dm_dev *dev,
				  sector_t start, sector_t len, void *data)
{
	struct queue_limits *limits = data;
	struct block_device *bdev = dev->bdev;
	sector_t dev_size =
		i_size_read(bdev->bd_inode) >> SECTOR_SHIFT;
	unsigned short logical_block_size_sectors =
		limits->logical_block_size >> SECTOR_SHIFT;
	char b[BDEVNAME_SIZE];

	if (!dev_size)
		return 0;

	if ((start >= dev_size) || (start + len > dev_size)) {
		DMWARN("%s: %s too small for target: "
		       "start=%llu, len=%llu, dev_size=%llu",
		       dm_device_name(ti->table->md), bdevname(bdev, b),
		       (unsigned long long)start,
		       (unsigned long long)len,
		       (unsigned long long)dev_size);
		return 1;
	}

	if (logical_block_size_sectors <= 1)
		return 0;

	if (start & (logical_block_size_sectors - 1)) {
		DMWARN("%s: start=%llu not aligned to h/w "
		       "logical block size %u of %s",
		       dm_device_name(ti->table->md),
		       (unsigned long long)start,
		       limits->logical_block_size, bdevname(bdev, b));
		return 1;
	}

	if (len & (logical_block_size_sectors - 1)) {
		DMWARN("%s: len=%llu not aligned to h/w "
		       "logical block size %u of %s",
		       dm_device_name(ti->table->md),
		       (unsigned long long)len,
		       limits->logical_block_size, bdevname(bdev, b));
		return 1;
	}

	return 0;
}

static int upgrade_mode(struct dm_dev_internal *dd, fmode_t new_mode,
			struct mapped_device *md)
{
	int r;
	struct dm_dev_internal dd_new, dd_old;

	dd_new = dd_old = *dd;

	dd_new.dm_dev.mode |= new_mode;
	dd_new.dm_dev.bdev = NULL;

	r = open_dev(&dd_new, dd->dm_dev.bdev->bd_dev, md);
	if (r)
		return r;

	dd->dm_dev.mode |= new_mode;
	close_dev(&dd_old, md);

	return 0;
}

static int __table_get_device(struct dm_table *t, struct dm_target *ti,
			      const char *path, sector_t start, sector_t len,
			      fmode_t mode, struct dm_dev **result)
{
	int r;
	dev_t uninitialized_var(dev);
	struct dm_dev_internal *dd;
	unsigned int major, minor;

	BUG_ON(!t);

	if (sscanf(path, "%u:%u", &major, &minor) == 2) {
		 
		dev = MKDEV(major, minor);
		if (MAJOR(dev) != major || MINOR(dev) != minor)
			return -EOVERFLOW;
	} else {
		 
		struct block_device *bdev = lookup_bdev(path);

		if (IS_ERR(bdev))
			return PTR_ERR(bdev);
		dev = bdev->bd_dev;
		bdput(bdev);
	}

	dd = find_device(&t->devices, dev);
	if (!dd) {
		dd = kmalloc(sizeof(*dd), GFP_KERNEL);
		if (!dd)
			return -ENOMEM;

		dd->dm_dev.mode = mode;
		dd->dm_dev.bdev = NULL;

		if ((r = open_dev(dd, dev, t->md))) {
			kfree(dd);
			return r;
		}

		format_dev_t(dd->dm_dev.name, dev);

		atomic_set(&dd->count, 0);
		list_add(&dd->list, &t->devices);

	} else if (dd->dm_dev.mode != (mode | dd->dm_dev.mode)) {
		r = upgrade_mode(dd, mode, t->md);
		if (r)
			return r;
	}
	atomic_inc(&dd->count);

	*result = &dd->dm_dev;
	return 0;
}

#define min_not_zero(l, r) (l == 0) ? r : ((r == 0) ? l : min(l, r))

int dm_set_device_limits(struct dm_target *ti, struct dm_dev *dev,
			 sector_t start, sector_t len, void *data)
{
	struct queue_limits *limits = data;
	struct block_device *bdev = dev->bdev;
	struct request_queue *q = bdev_get_queue(bdev);
	char b[BDEVNAME_SIZE];

	if (unlikely(!q)) {
		DMWARN("%s: Cannot set limits for nonexistent device %s",
		       dm_device_name(ti->table->md), bdevname(bdev, b));
		return 0;
	}

	if (bdev_stack_limits(limits, bdev, start) < 0)
		DMWARN("%s: adding target device %s caused an alignment inconsistency: "
		       "physical_block_size=%u, logical_block_size=%u, "
		       "alignment_offset=%u, start=%llu",
		       dm_device_name(ti->table->md), bdevname(bdev, b),
		       q->limits.physical_block_size,
		       q->limits.logical_block_size,
		       q->limits.alignment_offset,
		       (unsigned long long) start << SECTOR_SHIFT);

	if (q->merge_bvec_fn && !ti->type->merge)
		limits->max_sectors =
			min_not_zero(limits->max_sectors,
				     (unsigned int) (PAGE_SIZE >> 9));
	return 0;
}
EXPORT_SYMBOL_GPL(dm_set_device_limits);

int dm_get_device(struct dm_target *ti, const char *path, sector_t start,
		  sector_t len, fmode_t mode, struct dm_dev **result)
{
	return __table_get_device(ti->table, ti, path,
				  start, len, mode, result);
}

void dm_put_device(struct dm_target *ti, struct dm_dev *d)
{
	struct dm_dev_internal *dd = container_of(d, struct dm_dev_internal,
						  dm_dev);

	if (atomic_dec_and_test(&dd->count)) {
		close_dev(dd, ti->table->md);
		list_del(&dd->list);
		kfree(dd);
	}
}

static int adjoin(struct dm_table *table, struct dm_target *ti)
{
	struct dm_target *prev;

	if (!table->num_targets)
		return !ti->begin;

	prev = &table->targets[table->num_targets - 1];
	return (ti->begin == (prev->begin + prev->len));
}

static char **realloc_argv(unsigned *array_size, char **old_argv)
{
	char **argv;
	unsigned new_size;

	new_size = *array_size ? *array_size * 2 : 64;
	argv = kmalloc(new_size * sizeof(*argv), GFP_KERNEL);
	if (argv) {
		memcpy(argv, old_argv, *array_size * sizeof(*argv));
		*array_size = new_size;
	}

	kfree(old_argv);
	return argv;
}

int dm_split_args(int *argc, char ***argvp, char *input)
{
	char *start, *end = input, *out, **argv = NULL;
	unsigned array_size = 0;

	*argc = 0;

	if (!input) {
		*argvp = NULL;
		return 0;
	}

	argv = realloc_argv(&array_size, argv);
	if (!argv)
		return -ENOMEM;

	while (1) {
		start = end;

		while (*start && isspace(*start))
			start++;

		if (!*start)
			break;	 

		end = out = start;
		while (*end) {
			 
			if (*end == '\\' && *(end + 1)) {
				*out++ = *(end + 1);
				end += 2;
				continue;
			}

			if (isspace(*end))
				break;	 

			*out++ = *end++;
		}

		if ((*argc + 1) > array_size) {
			argv = realloc_argv(&array_size, argv);
			if (!argv)
				return -ENOMEM;
		}

		if (*end)
			end++;

		*out = '\0';
		argv[*argc] = start;
		(*argc)++;
	}

	*argvp = argv;
	return 0;
}

static int validate_hardware_logical_block_alignment(struct dm_table *table,
						 struct queue_limits *limits)
{
	 
	unsigned short device_logical_block_size_sects =
		limits->logical_block_size >> SECTOR_SHIFT;

	unsigned short next_target_start = 0;

	unsigned short remaining = 0;

	struct dm_target *uninitialized_var(ti);
	struct queue_limits ti_limits;
	unsigned i = 0;

	while (i < dm_table_get_num_targets(table)) {
		ti = dm_table_get_target(table, i++);

		blk_set_default_limits(&ti_limits);

		if (ti->type->iterate_devices)
			ti->type->iterate_devices(ti, dm_set_device_limits,
						  &ti_limits);

		if (remaining < ti->len &&
		    remaining & ((ti_limits.logical_block_size >>
				  SECTOR_SHIFT) - 1))
			break;	 

		next_target_start =
		    (unsigned short) ((next_target_start + ti->len) &
				      (device_logical_block_size_sects - 1));
		remaining = next_target_start ?
		    device_logical_block_size_sects - next_target_start : 0;
	}

	if (remaining) {
		DMWARN("%s: table line %u (start sect %llu len %llu) "
		       "not aligned to h/w logical block size %u",
		       dm_device_name(table->md), i,
		       (unsigned long long) ti->begin,
		       (unsigned long long) ti->len,
		       limits->logical_block_size);
		return -EINVAL;
	}

	return 0;
}

int dm_table_add_target(struct dm_table *t, const char *type,
			sector_t start, sector_t len, char *params)
{
	int r = -EINVAL, argc;
	char **argv;
	struct dm_target *tgt;

	if ((r = check_space(t)))
		return r;

	tgt = t->targets + t->num_targets;
	memset(tgt, 0, sizeof(*tgt));

	if (!len) {
		DMERR("%s: zero-length target", dm_device_name(t->md));
		return -EINVAL;
	}

	tgt->type = dm_get_target_type(type);
	if (!tgt->type) {
		DMERR("%s: %s: unknown target type", dm_device_name(t->md),
		      type);
		return -EINVAL;
	}

	tgt->table = t;
	tgt->begin = start;
	tgt->len = len;
	tgt->error = "Unknown error";

	if (!adjoin(t, tgt)) {
		tgt->error = "Gap in table";
		r = -EINVAL;
		goto bad;
	}

	r = dm_split_args(&argc, &argv, params);
	if (r) {
		tgt->error = "couldn't split parameters (insufficient memory)";
		goto bad;
	}

	r = tgt->type->ctr(tgt, argc, argv);
	kfree(argv);
	if (r)
		goto bad;

	t->highs[t->num_targets++] = tgt->begin + tgt->len - 1;

	return 0;

 bad:
	DMERR("%s: %s: %s", dm_device_name(t->md), type, tgt->error);
	dm_put_target_type(tgt->type);
	return r;
}

int dm_table_set_type(struct dm_table *t)
{
	unsigned i;
	unsigned bio_based = 0, request_based = 0;
	struct dm_target *tgt;
	struct dm_dev_internal *dd;
	struct list_head *devices;

	for (i = 0; i < t->num_targets; i++) {
		tgt = t->targets + i;
		if (dm_target_request_based(tgt))
			request_based = 1;
		else
			bio_based = 1;

		if (bio_based && request_based) {
			DMWARN("Inconsistent table: different target types"
			       " can't be mixed up");
			return -EINVAL;
		}
	}

	if (bio_based) {
		 
		t->type = DM_TYPE_BIO_BASED;
		return 0;
	}

	BUG_ON(!request_based);  

	devices = dm_table_get_devices(t);
	list_for_each_entry(dd, devices, list) {
		if (!blk_queue_stackable(bdev_get_queue(dd->dm_dev.bdev))) {
			DMWARN("table load rejected: including"
			       " non-request-stackable devices");
			return -EINVAL;
		}
	}

	if (t->num_targets > 1) {
		DMWARN("Request-based dm doesn't support multiple targets yet");
		return -EINVAL;
	}

	t->type = DM_TYPE_REQUEST_BASED;

	return 0;
}

unsigned dm_table_get_type(struct dm_table *t)
{
	return t->type;
}

bool dm_table_request_based(struct dm_table *t)
{
	return dm_table_get_type(t) == DM_TYPE_REQUEST_BASED;
}

int dm_table_alloc_md_mempools(struct dm_table *t)
{
	unsigned type = dm_table_get_type(t);

	if (unlikely(type == DM_TYPE_NONE)) {
		DMWARN("no table type is set, can't allocate mempools");
		return -EINVAL;
	}

	t->mempools = dm_alloc_md_mempools(type);
	if (!t->mempools)
		return -ENOMEM;

	return 0;
}

void dm_table_free_md_mempools(struct dm_table *t)
{
	dm_free_md_mempools(t->mempools);
	t->mempools = NULL;
}

struct dm_md_mempools *dm_table_get_md_mempools(struct dm_table *t)
{
	return t->mempools;
}

static int setup_indexes(struct dm_table *t)
{
	int i;
	unsigned int total = 0;
	sector_t *indexes;

	for (i = t->depth - 2; i >= 0; i--) {
		t->counts[i] = dm_div_up(t->counts[i + 1], CHILDREN_PER_NODE);
		total += t->counts[i];
	}

	indexes = (sector_t *) dm_vcalloc(total, (unsigned long) NODE_SIZE);
	if (!indexes)
		return -ENOMEM;

	for (i = t->depth - 2; i >= 0; i--) {
		t->index[i] = indexes;
		indexes += (KEYS_PER_NODE * t->counts[i]);
		setup_btree_index(i, t);
	}

	return 0;
}

int dm_table_complete(struct dm_table *t)
{
	int r = 0;
	unsigned int leaf_nodes;

	leaf_nodes = dm_div_up(t->num_targets, KEYS_PER_NODE);
	t->depth = 1 + int_log(leaf_nodes, CHILDREN_PER_NODE);

	t->counts[t->depth - 1] = leaf_nodes;
	t->index[t->depth - 1] = t->highs;

	if (t->depth >= 2)
		r = setup_indexes(t);

	return r;
}

static DEFINE_MUTEX(_event_lock);
void dm_table_event_callback(struct dm_table *t,
			     void (*fn)(void *), void *context)
{
	mutex_lock(&_event_lock);
	t->event_fn = fn;
	t->event_context = context;
	mutex_unlock(&_event_lock);
}

void dm_table_event(struct dm_table *t)
{
	 
	BUG_ON(in_interrupt());

	mutex_lock(&_event_lock);
	if (t->event_fn)
		t->event_fn(t->event_context);
	mutex_unlock(&_event_lock);
}

sector_t dm_table_get_size(struct dm_table *t)
{
	return t->num_targets ? (t->highs[t->num_targets - 1] + 1) : 0;
}

struct dm_target *dm_table_get_target(struct dm_table *t, unsigned int index)
{
	if (index >= t->num_targets)
		return NULL;

	return t->targets + index;
}

struct dm_target *dm_table_find_target(struct dm_table *t, sector_t sector)
{
	unsigned int l, n = 0, k = 0;
	sector_t *node;

	for (l = 0; l < t->depth; l++) {
		n = get_child(n, k);
		node = get_node(t, l, n);

		for (k = 0; k < KEYS_PER_NODE; k++)
			if (node[k] >= sector)
				break;
	}

	return &t->targets[(KEYS_PER_NODE * n) + k];
}

int dm_calculate_queue_limits(struct dm_table *table,
			      struct queue_limits *limits)
{
	struct dm_target *uninitialized_var(ti);
	struct queue_limits ti_limits;
	unsigned i = 0;

	blk_set_default_limits(limits);

	while (i < dm_table_get_num_targets(table)) {
		blk_set_default_limits(&ti_limits);

		ti = dm_table_get_target(table, i++);

		if (!ti->type->iterate_devices)
			goto combine_limits;

		ti->type->iterate_devices(ti, dm_set_device_limits,
					  &ti_limits);

		if (ti->type->io_hints)
			ti->type->io_hints(ti, &ti_limits);

		if (ti->type->iterate_devices(ti, device_area_is_invalid,
					      &ti_limits))
			return -EINVAL;

combine_limits:
		 
		if (blk_stack_limits(limits, &ti_limits, 0) < 0)
			DMWARN("%s: adding target device "
			       "(start sect %llu len %llu) "
			       "caused an alignment inconsistency",
			       dm_device_name(table->md),
			       (unsigned long long) ti->begin,
			       (unsigned long long) ti->len);
	}

	return validate_hardware_logical_block_alignment(table, limits);
}

static void dm_table_set_integrity(struct dm_table *t)
{
	struct list_head *devices = dm_table_get_devices(t);
	struct dm_dev_internal *prev = NULL, *dd = NULL;

	if (!blk_get_integrity(dm_disk(t->md)))
		return;

	list_for_each_entry(dd, devices, list) {
		if (prev &&
		    blk_integrity_compare(prev->dm_dev.bdev->bd_disk,
					  dd->dm_dev.bdev->bd_disk) < 0) {
			DMWARN("%s: integrity not set: %s and %s mismatch",
			       dm_device_name(t->md),
			       prev->dm_dev.bdev->bd_disk->disk_name,
			       dd->dm_dev.bdev->bd_disk->disk_name);
			goto no_integrity;
		}
		prev = dd;
	}

	if (!prev || !bdev_get_integrity(prev->dm_dev.bdev))
		goto no_integrity;

	blk_integrity_register(dm_disk(t->md),
			       bdev_get_integrity(prev->dm_dev.bdev));

	return;

no_integrity:
	blk_integrity_register(dm_disk(t->md), NULL);

	return;
}

void dm_table_set_restrictions(struct dm_table *t, struct request_queue *q,
			       struct queue_limits *limits)
{
	 
	q->limits = *limits;

	if (limits->no_cluster)
		queue_flag_clear_unlocked(QUEUE_FLAG_CLUSTER, q);
	else
		queue_flag_set_unlocked(QUEUE_FLAG_CLUSTER, q);

	dm_table_set_integrity(t);

	smp_mb();
	if (dm_table_request_based(t))
		queue_flag_set_unlocked(QUEUE_FLAG_STACKABLE, q);
}

unsigned int dm_table_get_num_targets(struct dm_table *t)
{
	return t->num_targets;
}

struct list_head *dm_table_get_devices(struct dm_table *t)
{
	return &t->devices;
}

fmode_t dm_table_get_mode(struct dm_table *t)
{
	return t->mode;
}

static void suspend_targets(struct dm_table *t, unsigned postsuspend)
{
	int i = t->num_targets;
	struct dm_target *ti = t->targets;

	while (i--) {
		if (postsuspend) {
			if (ti->type->postsuspend)
				ti->type->postsuspend(ti);
		} else if (ti->type->presuspend)
			ti->type->presuspend(ti);

		ti++;
	}
}

void dm_table_presuspend_targets(struct dm_table *t)
{
	if (!t)
		return;

	suspend_targets(t, 0);
}

void dm_table_postsuspend_targets(struct dm_table *t)
{
	if (!t)
		return;

	suspend_targets(t, 1);
}

int dm_table_resume_targets(struct dm_table *t)
{
	int i, r = 0;

	for (i = 0; i < t->num_targets; i++) {
		struct dm_target *ti = t->targets + i;

		if (!ti->type->preresume)
			continue;

		r = ti->type->preresume(ti);
		if (r)
			return r;
	}

	for (i = 0; i < t->num_targets; i++) {
		struct dm_target *ti = t->targets + i;

		if (ti->type->resume)
			ti->type->resume(ti);
	}

	return 0;
}

int dm_table_any_congested(struct dm_table *t, int bdi_bits)
{
	struct dm_dev_internal *dd;
	struct list_head *devices = dm_table_get_devices(t);
	int r = 0;

	list_for_each_entry(dd, devices, list) {
		struct request_queue *q = bdev_get_queue(dd->dm_dev.bdev);
		char b[BDEVNAME_SIZE];

		if (likely(q))
			r |= bdi_congested(&q->backing_dev_info, bdi_bits);
		else
			DMWARN_LIMIT("%s: any_congested: nonexistent device %s",
				     dm_device_name(t->md),
				     bdevname(dd->dm_dev.bdev, b));
	}

	return r;
}

int dm_table_any_busy_target(struct dm_table *t)
{
	unsigned i;
	struct dm_target *ti;

	for (i = 0; i < t->num_targets; i++) {
		ti = t->targets + i;
		if (ti->type->busy && ti->type->busy(ti))
			return 1;
	}

	return 0;
}

void dm_table_unplug_all(struct dm_table *t)
{
	struct dm_dev_internal *dd;
	struct list_head *devices = dm_table_get_devices(t);

	list_for_each_entry(dd, devices, list) {
		struct request_queue *q = bdev_get_queue(dd->dm_dev.bdev);
		char b[BDEVNAME_SIZE];

		if (likely(q))
			blk_unplug(q);
		else
			DMWARN_LIMIT("%s: Cannot unplug nonexistent device %s",
				     dm_device_name(t->md),
				     bdevname(dd->dm_dev.bdev, b));
	}
}

struct mapped_device *dm_table_get_md(struct dm_table *t)
{
	dm_get(t->md);

	return t->md;
}

EXPORT_SYMBOL(dm_vcalloc);
EXPORT_SYMBOL(dm_get_device);
EXPORT_SYMBOL(dm_put_device);
EXPORT_SYMBOL(dm_table_event);
EXPORT_SYMBOL(dm_table_get_size);
EXPORT_SYMBOL(dm_table_get_mode);
EXPORT_SYMBOL(dm_table_get_md);
EXPORT_SYMBOL(dm_table_put);
EXPORT_SYMBOL(dm_table_get);
EXPORT_SYMBOL(dm_table_unplug_all);

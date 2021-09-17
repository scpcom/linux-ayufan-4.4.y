/* Copyright (c) 2000-2017 Synology Inc. All rights reserved. */
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/rwlock.h>
#include <linux/mtd/mtd.h>

#define UBOOT_ENV_PARTITION_NAME "RedBoot Config"
#define UBOOT_ENV_SIZE (64 << 10) /* 64 KB */
#define UBOOT_ENV_MAX_DATA_SIZE (UBOOT_ENV_SIZE - sizeof(uint32_t))
#define UBOOT_ENV_MAX_ENTRIES 512

typedef struct {
	char *name;
	char *value;
} nvram_t;

static DEFINE_RWLOCK(nvram_lock);
static uint8_t uboot_env[UBOOT_ENV_SIZE];
static nvram_t env_list[UBOOT_ENV_MAX_ENTRIES];

extern void (*arm_pm_restart)(char, const char *);
static void syno_force_auto_install_helper(struct work_struct *);
static DECLARE_WORK(syno_force_auto_install_work, syno_force_auto_install_helper);

typedef struct {
	uint32_t crc; /* CRC32 over data bytes */
	unsigned char *data; /* Environment data */
} env_t;

static const uint32_t crc_table[256] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t crc32(uint32_t seed, const void *buf, size_t size)
{
	uint32_t crc = seed;
	const unsigned char *p = buf;

	crc ^= ~0U;
	while (size--) {
		crc = crc_table[(crc ^ *p) & 0xFF] ^ (crc >> 8);
		++p;
	}
	crc ^= ~0U;
	return crc;
}

static int syno_nvram_env_init(void)
{
	char *dp, *sp, *name, *value, *dp_end;
	char sep = '\0';
	int idx = 0;
	size_t retlen;
	struct mtd_info *mtd = get_mtd_device_nm(UBOOT_ENV_PARTITION_NAME);

	if (IS_ERR_OR_NULL(mtd)) {
		printk("%s: Cannot find partition: %s\n", __func__, UBOOT_ENV_PARTITION_NAME);
		return -ENODEV;
	}

	if (mtd_read(mtd, 0, UBOOT_ENV_SIZE, &retlen, uboot_env)) {
		printk("%s: Cannot read partition: %s\n", __func__, UBOOT_ENV_PARTITION_NAME);
		return -EIO;
	}

	memset(env_list, 0, sizeof(env_list));

	/* load uboot fake nvram buffer */
	/* point to first data */
	dp = (char*)uboot_env;
	/* point to data buffer */
	dp += 4;
	dp_end = (char*)((uint32_t)uboot_env + UBOOT_ENV_SIZE);

	/* point to first data */
	do {
		/* skip leading white space */
		while ((*dp == ' ') || (*dp == '\t'))
			++dp;

		/* skip comment lines */
		if (*dp == '#') {
			while (*dp && (*dp != sep))
				++dp;
			++dp;
			continue;
		}

		/* parse name */
		for (name = dp; *dp != '=' && *dp && *dp != sep; ++dp)
			;

		*dp++ = '\0'; /* terminate name */

		/* parse value; deal with escapes */
		for (value = sp = dp; *dp && (*dp != sep); ++dp) {
			if ((*dp == '\\') && *(dp + 1))
				++dp;
			*sp++ = *dp;
		}
		*sp++ = '\0'; /* terminate value */
		++dp;

		/* enter into hash table */
		env_list[idx].name = name;
		env_list[idx].value = value;
		//printk("entry%d %s=%s\n", idx, name, value);
		idx++;

		/* check if table is full */
		if (idx >= UBOOT_ENV_MAX_ENTRIES) {
			printk("%s: WARNING - UBoot environment table is full\n", __func__);
			break;
		}

		/* check if end of table */
	} while ((dp < dp_end) && *dp); /* size check needed for text */

	return 0;
}

static int syno_nvram_get(const char *name, char *value, size_t sz)
{
	int i, len;
	nvram_t *tuple;
	int num_entries;

	if (!name || !value)
		return -EINVAL;

	if (0 == (len = strlen(name)))
		return -EINVAL;

	num_entries = sizeof(env_list) / sizeof(nvram_t);
	tuple = &env_list[0];

	for (i = 0; i < num_entries; i++) {
		if (tuple->name && (strncmp(tuple->name, name, len) == 0) && (strlen(tuple->name) == len)) {
			strlcpy(value, tuple->value, sz);
			return 0;
		}

		++tuple;
	}

	return -1;
}

static int syno_nvram_set(const char *name, const char *value)
{
	int i, len, has_found = 0;
	nvram_t *tuple;
	int num_entries;
	size_t total_len = 0;

	if (!name || !value)
		return -EINVAL;

	len = strlen(name);
	if (len == 0)
		return -EINVAL;

	num_entries = sizeof(env_list) / sizeof(nvram_t);

	/* Pass 1: check if total length is within permitted range */
	tuple = &env_list[0];
	for (i = 0; i < num_entries; i++) {
		if (!tuple->name)
			break;
		total_len += strlen(tuple->name);
		if ((strncmp(tuple->name, name, len) == 0) && (strlen(tuple->name) == len)) {
			total_len += strlen(value);
			has_found = 1;
		} else {
			total_len += strlen(tuple->value);
		}
		/* each name-value pair also contains '=' and '\0' */
		total_len += 2;
		tuple++;
	}
	if (!has_found) {
		++i;
		total_len += strlen(name) + strlen(value) + 2;
	}

	if (i > UBOOT_ENV_MAX_ENTRIES || total_len > UBOOT_ENV_MAX_DATA_SIZE) {
		printk("%s: WARNING - UBoot environment table is full\n", __func__);
		return -EINVAL;
	}

	/* Pass 2: update name-value pair */
	if (has_found) {
		tuple = &env_list[0];
		for (i = 0; i < num_entries; i++) {
			if (!tuple->name)
				break;
			if ((strncmp(tuple->name, name, len) == 0) && (strlen(tuple->name) == len)) {
				tuple->value = kstrdup(value, GFP_KERNEL);
				if (!tuple->value)
					goto nomem;
				return 0;
			}
			tuple++;
		}
	} else {
		tuple->name = kstrdup(name, GFP_KERNEL);
		tuple->value = kstrdup(value, GFP_KERNEL);
		if (!tuple->name || !tuple->value)
			goto nomem;
	}
	return 0;

nomem:
	printk("%s: Failed to allocate memory!\n", __func__);
	return -ENOMEM;
}

static size_t serialize_env(unsigned char *dest, size_t size)
{
	int i;
	const char *s;
	char *p = dest;
	size_t len = 0;

	if (dest == NULL || size == 0) {
		return 0;
	}

	memset(p, '\0', size);

	for (i = 0; i < UBOOT_ENV_MAX_ENTRIES; ++i) {
		s = env_list[i].name;
		if (s == NULL)
			break;

		while (*s)
			*p++ = *s++;
		*p++ = '=';

		s = env_list[i].value;
		while (*s)
			*p++ = *s++;
		*p++ = '\0';
		len += strlen(env_list[i].name) + strlen(env_list[i].value) + 2;
	}
	return len;
}

static int syno_erase_mtd_partition(const char *partition_name)
{
	struct erase_info erase;
	struct mtd_info *mtd = get_mtd_device_nm(partition_name);

	if (IS_ERR_OR_NULL(mtd)) {
		printk("%s: Cannot find partition: %s\n", __func__, partition_name);
		return -ENODEV;
	}

	memset(&erase, 0, sizeof(struct erase_info));
	erase.mtd = mtd;
	erase.len = mtd->size;

	if (mtd_erase(mtd, &erase)) {
		printk("%s: Failed to erase %s!\n", __func__, partition_name);
		return -EIO;
	}
	printk("%s: Erased %s!\n", __func__, partition_name);
	return 0;
}

static int syno_mtd_commit(env_t *env)
{
	size_t retlen;
	int ret = 0;
	const int crc_size = sizeof(uint32_t);
	const unsigned char *data = env->data;
	struct mtd_info *mtd = get_mtd_device_nm(UBOOT_ENV_PARTITION_NAME);

	if (IS_ERR_OR_NULL(mtd)) {
		printk("%s: Cannot find partition: %s\n", __func__, UBOOT_ENV_PARTITION_NAME);
		ret = -ENODEV;
		goto end;
	}

	if (syno_erase_mtd_partition(UBOOT_ENV_PARTITION_NAME))
		goto write_error;

	if (mtd_write(mtd, 0, crc_size, &retlen, (u_char *)&(env->crc)))
		goto write_error;
	if (mtd_write(mtd, crc_size, UBOOT_ENV_SIZE - crc_size, &retlen, data))
		goto write_error;
	printk("%s: Written UBoot environment!\n", __func__);

end:
	return ret;
write_error:
	printk("%s: Failed to write UBoot environment variables!\n", __func__);
	return -EIO;
}

/* Saves current_env variable into MTD */
static int syno_nvram_commit(void)
{
	env_t current_env;
	int ret = 0, i;
	unsigned char *data = NULL;
	const int RETRY_LIMIT = 10;

	for (i = 0; i < RETRY_LIMIT; ++i) {
		data = kmalloc(UBOOT_ENV_MAX_DATA_SIZE, GFP_KERNEL);
		if (data)
			break;
		msleep(10);
	}
	if (!data) {
		printk("%s: Failed to allocate memory!\n", __func__);
		return -ENOMEM;
	}

	serialize_env(data, UBOOT_ENV_MAX_DATA_SIZE);

	current_env.data = data;
	current_env.crc = crc32(0, data, UBOOT_ENV_MAX_DATA_SIZE);

	ret = syno_mtd_commit(&current_env);

	kfree(data);
	return ret;
}

static void syno_force_auto_install_helper(struct work_struct *work)
{
	write_lock(&nvram_lock);
	if (syno_nvram_env_init())
		goto fail;
	if (syno_nvram_set("syno_extra_args", "reset_default=yes"))
		goto fail;
	if (syno_nvram_commit())
		goto fail;
	write_unlock(&nvram_lock);

	printk("%s: Updated bootargs successfully!\n", __func__);
	printk("%s: Going to reboot for auto install...\n", __func__);
	msleep(1000);
	arm_pm_restart('\0', NULL);
	return;
fail:
	write_unlock(&nvram_lock);
	printk("%s: Failed to update bootargs!\n", __func__);
	return;
}

int syno_get_uboot_env_variable(const char *name, char *value, size_t len)
{
	if (!name || !value || !len) {
		goto fail;
	}

	write_lock(&nvram_lock);
	if (syno_nvram_env_init()) {
		write_unlock(&nvram_lock);
		goto fail;
	}
	write_unlock(&nvram_lock);

	read_lock(&nvram_lock);
	if (syno_nvram_get(name, value, len))
		goto unlock;
	read_unlock(&nvram_lock);
	return 0;

unlock:
	read_unlock(&nvram_lock);
fail:
	return -1;
}
EXPORT_SYMBOL(syno_get_uboot_env_variable);

void syno_force_auto_install(void)
{
	/* assume called under interrupt context, just queue a work then leave */
	schedule_work(&syno_force_auto_install_work);
}
EXPORT_SYMBOL(syno_force_auto_install);

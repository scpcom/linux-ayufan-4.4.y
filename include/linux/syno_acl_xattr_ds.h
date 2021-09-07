 
#ifndef _SYNO_ACL_XATTR_DS_H
#define _SYNO_ACL_XATTR_DS_H

#include <linux/types.h>

#define SYNO_ACL_MNT_OPT	"synoacl"
#define SYNO_ACL_NOT_MNT_OPT	"no"SYNO_ACL_MNT_OPT

#define SYNO_ACL_XATTR_ACCESS	"system.syno_acl_self"
#define SYNO_ACL_XATTR_ACCESS_NOPERM	"system.syno_acl_noperm_self"
#define SYNO_ACL_XATTR_INHERIT	"system.syno_acl_inherit"
#define SYNO_ACL_XATTR_PSEUDO_INHERIT_ONLY	"system.syno_acl_pseudo_inherit_only"

#define SYNO_XATTR_EA_PREFIX "user.syno."
#define SYNO_XATTR_EA_PREFIX_LEN 10
#define SYNO_XATTR_NETATALK_PREFIX "user.netatalk."
#define SYNO_XATTR_NETATALK_PREFIX_LEN 14

#define SYNO_ACL_XATTR_VERSION		0x0001

#define SYNO_ACL_UNDEFINED_ID	(-1)

#define SYNO_ACL_MAY_EXEC				(0x0001)
#define SYNO_ACL_MAY_WRITE				(0x0002)
#define SYNO_ACL_MAY_READ				(0x0004)
#define SYNO_ACL_MAY_APPEND				(0x0008)
#define SYNO_ACL_MAY_ACCESS 			(0x0010) 
#define SYNO_ACL_MAY_OPEN 				(0x0020) 
#define SYNO_ACL_MAY_READ_EXT_ATTR		(0x0040)
#define SYNO_ACL_MAY_READ_PERMISSION	(0x0080)
#define SYNO_ACL_MAY_READ_ATTR			(0x0100)
#define SYNO_ACL_MAY_WRITE_ATTR			(0x0200)
#define SYNO_ACL_MAY_WRITE_EXT_ATTR		(0x0400)
#define SYNO_ACL_MAY_WRITE_PERMISSION	(0x0800)
#define SYNO_ACL_MAY_DEL				(0x1000)
#define SYNO_ACL_MAY_DEL_CHILD			(0x2000)
#define SYNO_ACL_MAY_GET_OWNER_SHIP		(0x4000)

#define SYNO_PERM_READABLE  ( \
		SYNO_ACL_MAY_READ | SYNO_ACL_MAY_READ_ATTR | SYNO_ACL_MAY_READ_PERMISSION | SYNO_ACL_MAY_READ_EXT_ATTR \
)
#define SYNO_PERM_WRITE_DATA  ( \
		SYNO_ACL_MAY_WRITE | SYNO_ACL_MAY_APPEND | SYNO_ACL_MAY_DEL_CHILD \
)
#define SYNO_PERM_WRITABLE  ( \
		SYNO_PERM_WRITE_DATA | SYNO_ACL_MAY_WRITE_ATTR | SYNO_ACL_MAY_WRITE_EXT_ATTR \
)
#define SYNO_PERM_OWNER ( \
		SYNO_ACL_MAY_READ_PERMISSION | SYNO_ACL_MAY_WRITE_PERMISSION | SYNO_ACL_MAY_GET_OWNER_SHIP \
)
#define SYNO_PERM_FULL_CONTROL  ( \
		SYNO_ACL_MAY_EXEC | \
		SYNO_PERM_READABLE | \
		SYNO_PERM_WRITABLE | \
		SYNO_ACL_MAY_WRITE_PERMISSION | \
        SYNO_ACL_MAY_GET_OWNER_SHIP | \
        SYNO_ACL_MAY_DEL \
)

#define SYNO_ACL_INHERIT_ONLY		        (0x0001)
#define SYNO_ACL_INHERIT_FILE		        (0x0002)
#define SYNO_ACL_INHERIT_DIR		        (0x0004)
#define SYNO_ACL_INHERIT_NO_PROPOGATE		(0x0008)

#define SYNO_ACL_INHERIT_TYPE		        (SYNO_ACL_INHERIT_ONLY | \
											 SYNO_ACL_INHERIT_FILE | \
											 SYNO_ACL_INHERIT_DIR)

#define SYNO_ACL_INHERIT_ALL  (SYNO_ACL_INHERIT_ONLY |  \
                                SYNO_ACL_INHERIT_FILE |  \
                                SYNO_ACL_INHERIT_DIR |  \
                                SYNO_ACL_INHERIT_NO_PROPOGATE)

#define IS_INHERIT_ONE_LEVEL(x) ((x & SYNO_ACL_INHERIT_NO_PROPOGATE) && (x & (SYNO_ACL_INHERIT_FILE|SYNO_ACL_INHERIT_DIR)))
#define IS_INHERIT_ONLY(x) (x & SYNO_ACL_INHERIT_ONLY)
#define IS_MATCH_FILE_TYPE(isdir, x) (isdir?(SYNO_ACL_INHERIT_DIR & x):(SYNO_ACL_INHERIT_FILE & x))

#define SYNO_ACL_XATTR_TAG_IS_DENY				(0X0001)
#define SYNO_ACL_XATTR_TAG_IS_ALLOW				(0X0002)
#define SYNO_ACL_XATTR_TAG_ALLOW_ALL			(SYNO_ACL_XATTR_TAG_IS_DENY | \
												 SYNO_ACL_XATTR_TAG_IS_ALLOW)

#define SYNO_ACL_XATTR_TAG_ID_USER				(0X0004)
#define SYNO_ACL_XATTR_TAG_ID_GROUP				(0X0008)
#define SYNO_ACL_XATTR_TAG_ID_EVERYONE			(0X0010)
#define SYNO_ACL_XATTR_TAG_ID_OWNER		(0X0020)
#define SYNO_ACL_XATTR_TAG_ID_AUTHENTICATEDUSER	(0X0040)
#define SYNO_ACL_XATTR_TAG_ID_SYSTEM		(0X0080)
#define SYNO_ACL_XATTR_TAG_ID_ALL		(SYNO_ACL_XATTR_TAG_ID_USER | \
						 SYNO_ACL_XATTR_TAG_ID_GROUP | \
						 SYNO_ACL_XATTR_TAG_ID_EVERYONE | \
						 SYNO_ACL_XATTR_TAG_ID_OWNER | \
						 SYNO_ACL_XATTR_TAG_ID_AUTHENTICATEDUSER | \
						 SYNO_ACL_XATTR_TAG_ID_SYSTEM)
 
enum {
	SYNO_KERNEL_IS_FS_SUPPORT = 1,  
	SYNO_KERNEL_IS_FILE_SUPPORT,   
};

enum {
	SYNO_ACL_INHERITED = 1,  
	SYNO_ACL_PSEUDO_INHERIT_ONLY,   
};

typedef struct {
	__le16			e_tag;
	__le32			e_perm;
	__le16			e_inherit;
	__le32			e_id;
	__le32			e_level;
} syno_acl_xattr_entry;

typedef struct {
	__le16			a_version;
	syno_acl_xattr_entry	a_entries[0];
} syno_acl_xattr_header;

static inline size_t
syno_acl_xattr_size(int count)
{
	return (sizeof(syno_acl_xattr_header) +
		(count * sizeof(syno_acl_xattr_entry)));
}

static inline int
syno_acl_xattr_count(size_t size)
{
	if (size < sizeof(syno_acl_xattr_header))
		return -1;
	size -= sizeof(syno_acl_xattr_header);
	if (size % sizeof(syno_acl_xattr_entry))
		return -1;
	return size / sizeof(syno_acl_xattr_entry);
}

#endif	 

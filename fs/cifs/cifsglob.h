#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/in.h>
#include <linux/in6.h>
#include <linux/slow-work.h>
#include "cifs_fs_sb.h"
#include "cifsacl.h"
 
#define MAX_UID_INFO 16
#define MAX_SES_INFO 2
#define MAX_TCON_INFO 4

#define MAX_TREE_SIZE (2 + MAX_SERVER_SIZE + 1 + MAX_SHARE_SIZE + 1)
#define MAX_SERVER_SIZE 15
#define MAX_SHARE_SIZE  64	 
#define MAX_USERNAME_SIZE 32	 
#define MAX_PASSWORD_SIZE 16

#define CIFS_MIN_RCV_POOL 4

#define CIFS_MAX_REQ 50

#define RFC1001_NAME_LEN 15
#define RFC1001_NAME_LEN_WITH_NULL (RFC1001_NAME_LEN + 1)

#define SERVER_NAME_LENGTH 40
#define SERVER_NAME_LEN_WITH_NULL     (SERVER_NAME_LENGTH + 1)

#define MAX_NAME 514

#include "cifspdu.h"

#ifndef XATTR_DOS_ATTRIB
#define XATTR_DOS_ATTRIB "user.DOSATTRIB"
#endif

enum statusEnum {
	CifsNew = 0,
	CifsGood,
	CifsExiting,
	CifsNeedReconnect
};

enum securityEnum {
	PLAINTXT = 0, 		 
	LANMAN,			 
	NTLM,			 
	NTLMv2,			 
	RawNTLMSSP,		 
   
	Kerberos,		 
	MSKerberos,		 
};

enum protocolEnum {
	TCP = 0,
	SCTP
	 
};

struct mac_key {
	unsigned int len;
	union {
		char ntlm[CIFS_SESS_KEY_SIZE + 16];
		char krb5[CIFS_SESS_KEY_SIZE + 16];  
		struct {
			char key[16];
			struct ntlmv2_resp resp;
		} ntlmv2;
	} data;
};

struct cifs_cred {
	int uid;
	int gid;
	int mode;
	int cecount;
	struct cifs_sid osid;
	struct cifs_sid gsid;
	struct cifs_ntace *ntaces;
	struct cifs_ace *aces;
};

struct TCP_Server_Info {
	struct list_head tcp_ses_list;
	struct list_head smb_ses_list;
	int srv_count;  
	 
	char server_RFC1001_name[RFC1001_NAME_LEN_WITH_NULL];
	char *hostname;  
	struct socket *ssocket;
	union {
		struct sockaddr_in sockAddr;
		struct sockaddr_in6 sockAddr6;
	} addr;
	wait_queue_head_t response_q;
	wait_queue_head_t request_q;  
	struct list_head pending_mid_q;
	void *Server_NlsInfo;	 
	unsigned short server_codepage;	 
	unsigned long ip_address;	 
	enum protocolEnum protocolType;
	char versionMajor;
	char versionMinor;
	bool svlocal:1;			 
	bool noblocksnd;		 
	bool noautotune;		 
	atomic_t inFlight;   
#ifdef CONFIG_CIFS_STATS2
	atomic_t inSend;  
	atomic_t num_waiters;    
#endif
	enum statusEnum tcpStatus;  
	struct mutex srv_mutex;
	struct task_struct *tsk;
	char server_GUID[16];
	char secMode;
	enum securityEnum secType;
	unsigned int maxReq;	 
	 
	unsigned int maxBuf;	 
	 
	unsigned int max_rw;	 
	 
	unsigned int max_vcs;	 
	char sessid[4];		 
	 
	int capabilities;  
	int timeAdj;   
	__u16 CurrentMid;          
	char cryptKey[CIFS_CRYPTO_KEY_SIZE];
	 
	char workstation_RFC1001_name[RFC1001_NAME_LEN_WITH_NULL];
	__u32 sequence_number;  
	struct mac_key mac_signing_key;
	char ntlmv2_hash[16];
	unsigned long lstrp;  
};

struct cifsUidInfo {
	struct list_head userList;
	struct list_head sessionList;  
	uid_t linux_uid;
	char user[MAX_USERNAME_SIZE + 1];	 
	 
};

struct cifsSesInfo {
	struct list_head smb_ses_list;
	struct list_head tcon_list;
	struct semaphore sesSem;
#if 0
	struct cifsUidInfo *uidInfo;	 
#endif
	struct TCP_Server_Info *server;	 
	int ses_count;		 
	enum statusEnum status;
	unsigned overrideSecFlg;   
	__u16 ipc_tid;		 
	__u16 flags;
	__u16 vcnum;
	char *serverOS;		 
	char *serverNOS;	 
	char *serverDomain;	 
	int Suid;		 
	uid_t linux_uid;         
	int capabilities;
	char serverName[SERVER_NAME_LEN_WITH_NULL * 2];	 
	char userName[MAX_USERNAME_SIZE + 1];
	char *domainName;
	char *password;
	bool need_reconnect:1;  
};
 
#define CIFS_SES_NT4 1
#define CIFS_SES_OS2 2
#define CIFS_SES_W9X 4
 
#define CIFS_SES_LANMAN 8
 
struct cifsTconInfo {
	struct list_head tcon_list;
	int tc_count;
	struct list_head openFileList;
	struct cifsSesInfo *ses;	 
	char treeName[MAX_TREE_SIZE + 1];  
	char *nativeFileSystem;
	char *password;		 
	__u16 tid;		 
	__u16 Flags;		 
	enum statusEnum tidStatus;
#ifdef CONFIG_CIFS_STATS
	atomic_t num_smbs_sent;
	atomic_t num_writes;
	atomic_t num_reads;
	atomic_t num_flushes;
	atomic_t num_oplock_brks;
	atomic_t num_opens;
	atomic_t num_closes;
	atomic_t num_deletes;
	atomic_t num_mkdirs;
	atomic_t num_posixopens;
	atomic_t num_posixmkdirs;
	atomic_t num_rmdirs;
	atomic_t num_renames;
	atomic_t num_t2renames;
	atomic_t num_ffirst;
	atomic_t num_fnext;
	atomic_t num_fclose;
	atomic_t num_hardlinks;
	atomic_t num_symlinks;
	atomic_t num_locks;
	atomic_t num_acl_get;
	atomic_t num_acl_set;
#ifdef CONFIG_CIFS_STATS2
	unsigned long long time_writes;
	unsigned long long time_reads;
	unsigned long long time_opens;
	unsigned long long time_deletes;
	unsigned long long time_closes;
	unsigned long long time_mkdirs;
	unsigned long long time_rmdirs;
	unsigned long long time_renames;
	unsigned long long time_t2renames;
	unsigned long long time_ffirst;
	unsigned long long time_fnext;
	unsigned long long time_fclose;
#endif  
	__u64    bytes_read;
	__u64    bytes_written;
	spinlock_t stat_lock;
#endif  
	FILE_SYSTEM_DEVICE_INFO fsDevInfo;
	FILE_SYSTEM_ATTRIBUTE_INFO fsAttrInfo;  
	FILE_SYSTEM_UNIX_INFO fsUnixInfo;
	bool ipc:1;		 
	bool retry:1;
	bool nocase:1;
	bool seal:1;       
	bool unix_ext:1;   
	bool local_lease:1;  
	bool broken_posix_open;  
	bool need_reconnect:1;  
	 
};

struct cifsLockInfo {
	struct list_head llist;	 
	__u64 offset;
	__u64 length;
	__u8 type;
};

struct cifs_search_info {
	loff_t index_of_last_entry;
	__u16 entries_in_buffer;
	__u16 info_level;
	__u32 resume_key;
	char *ntwrk_buf_start;
	char *srch_entries_start;
	char *last_entry;
	char *presume_name;
	unsigned int resume_name_len;
	bool endOfSearch:1;
	bool emptyDir:1;
	bool unicode:1;
	bool smallBuf:1;  
};

struct cifsFileInfo {
	struct list_head tlist;	 
	struct list_head flist;	 
	unsigned int uid;	 
	__u32 pid;		 
	__u16 netfid;		 
	  ;
	 
	struct file *pfile;  
	struct inode *pInode;  
	struct vfsmount *mnt;
	struct mutex lock_mutex;
	struct list_head llist;  
	bool closePend:1;	 
	bool invalidHandle:1;	 
	bool oplock_break_cancelled:1;
	atomic_t count;		 
	struct mutex fh_mutex;  
	struct cifs_search_info srch_inf;
	struct slow_work oplock_break;  
};

static inline void cifsFileInfo_get(struct cifsFileInfo *cifs_file)
{
	atomic_inc(&cifs_file->count);
}

static inline void cifsFileInfo_put(struct cifsFileInfo *cifs_file)
{
	if (atomic_dec_and_test(&cifs_file->count)) {
		iput(cifs_file->pInode);
		kfree(cifs_file);
	}
}

struct cifsInodeInfo {
	struct list_head lockList;
	 
	struct list_head openFileList;
	int write_behind_rc;
	__u32 cifsAttrs;  
	unsigned long time;	 
	bool clientCanCacheRead:1;	 
	bool clientCanCacheAll:1;	 
	bool delete_pending:1;		 
	u64  server_eof;		 
	u64  uniqueid;			 
	u64  createtime;                 
	struct inode vfs_inode;
};

static inline struct cifsInodeInfo *
CIFS_I(struct inode *inode)
{
	return container_of(inode, struct cifsInodeInfo, vfs_inode);
}

static inline struct cifs_sb_info *
CIFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

static inline char CIFS_DIR_SEP(const struct cifs_sb_info *cifs_sb)
{
	if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_POSIX_PATHS)
		return '/';
	else
		return '\\';
}

#ifdef CONFIG_CIFS_STATS
#define cifs_stats_inc atomic_inc

static inline void cifs_stats_bytes_written(struct cifsTconInfo *tcon,
					    unsigned int bytes)
{
	if (bytes) {
		spin_lock(&tcon->stat_lock);
		tcon->bytes_written += bytes;
		spin_unlock(&tcon->stat_lock);
	}
}

static inline void cifs_stats_bytes_read(struct cifsTconInfo *tcon,
					 unsigned int bytes)
{
	spin_lock(&tcon->stat_lock);
	tcon->bytes_read += bytes;
	spin_unlock(&tcon->stat_lock);
}
#else

#define  cifs_stats_inc(field) do {} while (0)
#define  cifs_stats_bytes_written(tcon, bytes) do {} while (0)
#define  cifs_stats_bytes_read(tcon, bytes) do {} while (0)

#endif

struct mid_q_entry {
	struct list_head qhead;	 
	__u16 mid;		 
	__u16 pid;		 
	__u32 sequence_number;   
	unsigned long when_alloc;   
#ifdef CONFIG_CIFS_STATS2
	unsigned long when_sent;  
	unsigned long when_received;  
#endif
	struct task_struct *tsk;	 
	struct smb_hdr *resp_buf;	 
	int midState;	 
	__u8 command;	 
	bool largeBuf:1;	 
	bool multiRsp:1;	 
	bool multiEnd:1;	 
};

struct oplock_q_entry {
	struct list_head qhead;
	struct inode *pinode;
	struct cifsTconInfo *tcon;
	__u16 netfid;
};

struct dir_notify_req {
       struct list_head lhead;
       __le16 Pid;
       __le16 PidHigh;
       __u16 Mid;
       __u16 Tid;
       __u16 Uid;
       __u16 netfid;
       __u32 filter;  
       int multishot;
       struct file *pfile;
};

struct dfs_info3_param {
	int flags;  
	int path_consumed;
	int server_type;
	int ref_flag;
	char *path_name;
	char *node_name;
};

#define CIFS_FATTR_DFS_REFERRAL		0x1
#define CIFS_FATTR_DELETE_PENDING	0x2
#define CIFS_FATTR_NEED_REVAL		0x4

struct cifs_fattr {
	u32		cf_flags;
	u32		cf_cifsattrs;
	u64		cf_uniqueid;
	u64		cf_eof;
	u64		cf_bytes;
	u64             cf_createtime;
	uid_t		cf_uid;
	gid_t		cf_gid;
	umode_t		cf_mode;
	dev_t		cf_rdev;
	unsigned int	cf_nlink;
	unsigned int	cf_dtype;
	struct timespec	cf_atime;
	struct timespec	cf_mtime;
	struct timespec	cf_ctime;
};

static inline void free_dfs_info_param(struct dfs_info3_param *param)
{
	if (param) {
		kfree(param->path_name);
		kfree(param->node_name);
		kfree(param);
	}
}

static inline void free_dfs_info_array(struct dfs_info3_param *param,
				       int number_of_items)
{
	int i;
	if ((number_of_items == 0) || (param == NULL))
		return;
	for (i = 0; i < number_of_items; i++) {
		kfree(param[i].path_name);
		kfree(param[i].node_name);
	}
	kfree(param);
}

#define   MID_FREE 0
#define   MID_REQUEST_ALLOCATED 1
#define   MID_REQUEST_SUBMITTED 2
#define   MID_RESPONSE_RECEIVED 4
#define   MID_RETRY_NEEDED      8  
#define   MID_NO_RESP_NEEDED 0x10

#define   CIFS_NO_BUFFER        0     
#define   CIFS_SMALL_BUFFER     1
#define   CIFS_LARGE_BUFFER     2
#define   CIFS_IOVEC            4     

#define   CIFS_STD_OP	        0     
#define   CIFS_LONG_OP          1     
#define   CIFS_VLONG_OP         2     
#define   CIFS_BLOCKING_OP      4     
#define   CIFS_ASYNC_OP         8     
#define   CIFS_TIMEOUT_MASK 0x00F     
#define   CIFS_LOG_ERROR    0x010     
#define   CIFS_LARGE_BUF_OP 0x020     
#define   CIFS_NO_RESP      0x040     

#define   CIFSSEC_MAY_SIGN	0x00001
#define   CIFSSEC_MAY_NTLM	0x00002
#define   CIFSSEC_MAY_NTLMV2	0x00004
#define   CIFSSEC_MAY_KRB5	0x00008
#ifdef CONFIG_CIFS_WEAK_PW_HASH
#define   CIFSSEC_MAY_LANMAN	0x00010
#define   CIFSSEC_MAY_PLNTXT	0x00020
#else
#define   CIFSSEC_MAY_LANMAN    0
#define   CIFSSEC_MAY_PLNTXT    0
#endif  
#define   CIFSSEC_MAY_SEAL	0x00040  
#define   CIFSSEC_MAY_NTLMSSP	0x00080  

#define   CIFSSEC_MUST_SIGN	0x01001
 
#define   CIFSSEC_MUST_NTLM	0x02002
#define   CIFSSEC_MUST_NTLMV2	0x04004
#define   CIFSSEC_MUST_KRB5	0x08008
#ifdef CONFIG_CIFS_WEAK_PW_HASH
#define   CIFSSEC_MUST_LANMAN	0x10010
#define   CIFSSEC_MUST_PLNTXT	0x20020
#ifdef CONFIG_CIFS_UPCALL
#define   CIFSSEC_MASK          0xBF0BF  
#else
#define   CIFSSEC_MASK          0xB70B7  
#endif  
#else  
#ifdef CONFIG_CIFS_UPCALL
#define   CIFSSEC_MASK          0x8F08F  
#else
#define	  CIFSSEC_MASK          0x87087  
#endif  
#endif  
#define   CIFSSEC_MUST_SEAL	0x40040  
#define   CIFSSEC_MUST_NTLMSSP	0x80080  

#define   CIFSSEC_DEF (CIFSSEC_MAY_SIGN | CIFSSEC_MAY_NTLM | CIFSSEC_MAY_NTLMV2)
#define   CIFSSEC_MAX (CIFSSEC_MUST_SIGN | CIFSSEC_MUST_NTLMV2)
#define   CIFSSEC_AUTH_MASK (CIFSSEC_MAY_NTLM | CIFSSEC_MAY_NTLMV2 | CIFSSEC_MAY_LANMAN | CIFSSEC_MAY_PLNTXT | CIFSSEC_MAY_KRB5 | CIFSSEC_MAY_NTLMSSP)
 
#define UID_HASH (16)

#ifdef DECLARE_GLOBALS_HERE
#define GLOBAL_EXTERN
#else
#define GLOBAL_EXTERN extern
#endif

GLOBAL_EXTERN struct list_head		cifs_tcp_ses_list;

GLOBAL_EXTERN rwlock_t		cifs_tcp_ses_lock;

GLOBAL_EXTERN rwlock_t GlobalSMBSeslock;

GLOBAL_EXTERN struct list_head GlobalDnotifyReqList;
 
GLOBAL_EXTERN struct list_head GlobalDnotifyRsp_Q;

GLOBAL_EXTERN unsigned int GlobalCurrentXid;	 
GLOBAL_EXTERN unsigned int GlobalTotalActiveXid;  
GLOBAL_EXTERN unsigned int GlobalMaxActiveXid;	 
GLOBAL_EXTERN spinlock_t GlobalMid_Lock;   
					   
GLOBAL_EXTERN char Local_System_Name[15];

GLOBAL_EXTERN atomic_t sesInfoAllocCount;
GLOBAL_EXTERN atomic_t tconInfoAllocCount;
GLOBAL_EXTERN atomic_t tcpSesAllocCount;
GLOBAL_EXTERN atomic_t tcpSesReconnectCount;
GLOBAL_EXTERN atomic_t tconInfoReconnectCount;

GLOBAL_EXTERN atomic_t bufAllocCount;     
#ifdef CONFIG_CIFS_STATS2
GLOBAL_EXTERN atomic_t totBufAllocCount;  
GLOBAL_EXTERN atomic_t totSmBufAllocCount;
#endif
GLOBAL_EXTERN atomic_t smBufAllocCount;
GLOBAL_EXTERN atomic_t midCount;

GLOBAL_EXTERN unsigned int multiuser_mount;  
GLOBAL_EXTERN unsigned int oplockEnabled;
GLOBAL_EXTERN unsigned int experimEnabled;
GLOBAL_EXTERN unsigned int lookupCacheEnabled;
GLOBAL_EXTERN unsigned int extended_security;	 
GLOBAL_EXTERN unsigned int sign_CIFS_PDUs;   
GLOBAL_EXTERN unsigned int linuxExtEnabled; 
#ifdef MY_ABC_HERE
GLOBAL_EXTERN unsigned int SynoPosixSemanticsEnabled; 
#endif
GLOBAL_EXTERN unsigned int CIFSMaxBufSize;   
GLOBAL_EXTERN unsigned int cifs_min_rcv;     
GLOBAL_EXTERN unsigned int cifs_min_small;   
GLOBAL_EXTERN unsigned int cifs_max_pending;  

extern const struct slow_work_ops cifs_oplock_break_ops;

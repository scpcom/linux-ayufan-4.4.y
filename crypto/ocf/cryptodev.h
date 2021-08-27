#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#ifndef _CRYPTO_CRYPTO_H_
#define _CRYPTO_CRYPTO_H_

#if  defined(CONFIG_OCF_M86XXX_MODULE) && (defined(__KERNEL__) || !defined(__GLIBC__) || (__GLIBC__ < 2))
#include <linux/in.h>
#endif

#ifdef CONFIG_SYNO_ARMADA_V2
#ifdef CONFIG_OF
#include "../../arch/arm/mach-mvebu/include/mach/mvTypes.h"
#include "../../drivers/crypto/mvebu_cesa/mvSysCesaConfig.h"
#endif
#endif

#define CRYPTO_DRIVERS_INITIAL	4
#define CRYPTO_SW_SESSIONS	32

#define NULL_HASH_LEN		0
#define MD5_HASH_LEN		16
#define SHA1_HASH_LEN		20
#define RIPEMD160_HASH_LEN	20
#define SHA2_256_HASH_LEN	32
#define SHA2_384_HASH_LEN	48
#define SHA2_512_HASH_LEN	64
#define MD5_KPDK_HASH_LEN	16
#define SHA1_KPDK_HASH_LEN	20
 
#define HASH_MAX_LEN		SHA2_512_HASH_LEN  

#define NULL_HMAC_BLOCK_LEN			1
#define MD5_HMAC_BLOCK_LEN			64
#define SHA1_HMAC_BLOCK_LEN			64
#define RIPEMD160_HMAC_BLOCK_LEN	64
#define SHA2_256_HMAC_BLOCK_LEN		64
#define SHA2_384_HMAC_BLOCK_LEN		128
#define SHA2_512_HMAC_BLOCK_LEN		128
 
#define HMAC_MAX_BLOCK_LEN		SHA2_512_HMAC_BLOCK_LEN  
#define HMAC_IPAD_VAL			0x36
#define HMAC_OPAD_VAL			0x5C

#define NULL_BLOCK_LEN			1
#define DES_BLOCK_LEN			8
#define DES3_BLOCK_LEN			8
#define BLOWFISH_BLOCK_LEN		8
#define SKIPJACK_BLOCK_LEN		8
#define CAST128_BLOCK_LEN		8
#define RIJNDAEL128_BLOCK_LEN	16
#define AES_BLOCK_LEN			RIJNDAEL128_BLOCK_LEN
#define CAMELLIA_BLOCK_LEN		16
#define ARC4_BLOCK_LEN			1
#define EALG_MAX_BLOCK_LEN		AES_BLOCK_LEN  

#define NULL_MIN_KEY_LEN		0
#define NULL_MAX_KEY_LEN		0
#define DES_MIN_KEY_LEN			8
#define DES_MAX_KEY_LEN			8
#define DES3_MIN_KEY_LEN		24
#define DES3_MAX_KEY_LEN		24
#define BLOWFISH_MIN_KEY_LEN	4
#define BLOWFISH_MAX_KEY_LEN	56
#define SKIPJACK_MIN_KEY_LEN	10
#define SKIPJACK_MAX_KEY_LEN	10
#define CAST128_MIN_KEY_LEN		5
#define CAST128_MAX_KEY_LEN		16
#define RIJNDAEL128_MIN_KEY_LEN	16
#define RIJNDAEL128_MAX_KEY_LEN	32
#define AES_MIN_KEY_LEN			RIJNDAEL128_MIN_KEY_LEN
#define AES_MAX_KEY_LEN			RIJNDAEL128_MAX_KEY_LEN
#define CAMELLIA_MIN_KEY_LEN	16
#define CAMELLIA_MAX_KEY_LEN	32
#define ARC4_MIN_KEY_LEN		1
#define ARC4_MAX_KEY_LEN		256

#define CRYPTO_MAX_DATA_LEN		64*1024 - 1

#define CRYPTO_ALGORITHM_MIN	1
#define CRYPTO_DES_CBC			1
#define CRYPTO_3DES_CBC			2
#define CRYPTO_BLF_CBC			3
#define CRYPTO_CAST_CBC			4
#define CRYPTO_SKIPJACK_CBC		5
#define CRYPTO_MD5_HMAC			6
#define CRYPTO_SHA1_HMAC		7
#define CRYPTO_RIPEMD160_HMAC	8
#define CRYPTO_MD5_KPDK			9
#define CRYPTO_SHA1_KPDK		10
#define CRYPTO_RIJNDAEL128_CBC	11  
#define CRYPTO_AES_CBC			11  
#define CRYPTO_ARC4				12
#define CRYPTO_MD5				13
#define CRYPTO_SHA1				14
#define CRYPTO_NULL_HMAC		15
#define CRYPTO_NULL_CBC			16
#define CRYPTO_DEFLATE_COMP		17  
#define CRYPTO_SHA2_256_HMAC	18
#define CRYPTO_SHA2_384_HMAC	19
#define CRYPTO_SHA2_512_HMAC	20
#define CRYPTO_CAMELLIA_CBC		21
#define CRYPTO_SHA2_256			22
#define CRYPTO_SHA2_384			23
#define CRYPTO_SHA2_512			24
#define CRYPTO_RIPEMD160		25
#define	CRYPTO_LZS_COMP			26
#if defined(CONFIG_OCF_M86XXX_MODULE)
#define CRYPTO_ESP_RFC2406 		27
 
#define CRYPTO_ESP_RFC4303  		28
#define CRYPTO_ESP4_RFC4303  		28
#define CRYPTO_ESP6_RFC4303  		29
#define CRYPTO_AH			30
#define CRYPTO_AH4			30
#define CRYPTO_AH6			31
#define CRYPTO_SHA2_HMAC		32  
#define CRYPTO_ALGORITHM_MAX		32  
#else
#define CRYPTO_ALGORITHM_MAX	26  
#endif

#define CRYPTO_ALG_FLAG_SUPPORTED	0x01  
#define CRYPTO_ALG_FLAG_RNG_ENABLE	0x02  
#define CRYPTO_ALG_FLAG_DSA_SHA		0x04  

#define CRYPTO_FLAG_HARDWARE	0x01000000	 
#define CRYPTO_FLAG_SOFTWARE	0x02000000	 

struct session_op {
	u_int32_t	cipher;		 
	u_int32_t	mac;		 

	u_int32_t	keylen;		 
	caddr_t		key;
	int		mackeylen;	 
	caddr_t		mackey;

	u_int32_t	ses;		 
};

struct session2_op {
	u_int32_t	cipher;		 
	u_int32_t	mac;		 

	u_int32_t	keylen;		 
	caddr_t		key;
	int		mackeylen;	 
	caddr_t		mackey;

	u_int32_t	ses;		 
	int		crid;		 
	int		pad[4];		 
};

struct crypt_op {
	u_int32_t	ses;
	u_int16_t	op;		 
#define COP_NONE	0
#define COP_ENCRYPT	1
#define COP_DECRYPT	2
	u_int16_t	flags;
#define	COP_F_BATCH	0x0008		 
	u_int		len;
	caddr_t		src, dst;	 
	caddr_t		mac;		 
	caddr_t		iv;
};

struct crypt_find_op {
	int		crid;		 
	char		name[32];	 
};

struct crparam {
	caddr_t		crp_p;
	u_int		crp_nbits;
};

#define CRK_MAXPARAM	8

struct crypt_kop {
	u_int		crk_op;		 
	u_int		crk_status;	 
	u_short		crk_iparams;	 
	u_short		crk_oparams;	 
	u_int		crk_crid;	 
	struct crparam	crk_param[CRK_MAXPARAM];
};
#define CRK_ALGORITM_MIN	0
#define CRK_MOD_EXP		0
#define CRK_MOD_EXP_CRT		1
#define CRK_DSA_SIGN		2
#define CRK_DSA_VERIFY		3
#define CRK_DH_COMPUTE_KEY	4
#define CRK_ALGORITHM_MAX	4  

#define CRF_MOD_EXP		(1 << CRK_MOD_EXP)
#define CRF_MOD_EXP_CRT		(1 << CRK_MOD_EXP_CRT)
#define CRF_DSA_SIGN		(1 << CRK_DSA_SIGN)
#define CRF_DSA_VERIFY		(1 << CRK_DSA_VERIFY)
#define CRF_DH_COMPUTE_KEY	(1 << CRK_DH_COMPUTE_KEY)

#define CRIOGET		_IOWR('c', 100, u_int32_t)
#define CRIOASYMFEAT	CIOCASYMFEAT
#define CRIOFINDDEV	CIOCFINDDEV

#define CIOCGSESSION	_IOWR('c', 101, struct session_op)
#define CIOCFSESSION	_IOW('c', 102, u_int32_t)
#define CIOCCRYPT	_IOWR('c', 103, struct crypt_op)
#define CIOCKEY		_IOWR('c', 104, struct crypt_kop)
#define CIOCASYMFEAT	_IOR('c', 105, u_int32_t)
#define CIOCGSESSION2	_IOWR('c', 106, struct session2_op)
#define CIOCKEY2	_IOWR('c', 107, struct crypt_kop)
#define CIOCFINDDEV	_IOWR('c', 108, struct crypt_find_op)

struct cryptotstat {
	struct timespec	acc;		 
	struct timespec	min;		 
	struct timespec	max;		 
	u_int32_t	count;		 
};

struct cryptostats {
	u_int32_t	cs_ops;		 
	u_int32_t	cs_errs;	 
	u_int32_t	cs_kops;	 
	u_int32_t	cs_kerrs;	 
	u_int32_t	cs_intrs;	 
	u_int32_t	cs_rets;	 
	u_int32_t	cs_blocks;	 
	u_int32_t	cs_kblocks;	 
	 
	struct cryptotstat cs_invoke;	 
	struct cryptotstat cs_done;	 
	struct cryptotstat cs_cb;	 
	struct cryptotstat cs_finis;	 

	u_int32_t	cs_drops;		 
};

#ifdef __KERNEL__

struct cryptoini {
	int		cri_alg;	 
#if defined(CONFIG_OCF_M86XXX_MODULE)
	int		cri_flags;
	union {
		struct {
			int		cri_mlen;	 
			int			cri_klen;	 
			caddr_t		cri_key;	 
			u_int8_t	cri_iv[EALG_MAX_BLOCK_LEN];	 
		} cri_alg;
		struct {
			u_int32_t basealg;
			struct sockaddr_in tun_source;
			struct sockaddr_in tun_destination;
			int tun_df_mode;
			int tun_ds_mode;
		 	int tun_ttl_value;
		 	int tun_replay_windowsize;
		 	int spivalue ;
		 	int replayinit;   
		 	u_int64_t time_hard_lifetime;
		 	u_int64_t time_soft_lifetime;
		 	u_int64_t byte_hard_lifetime;
		 	u_int64_t byte_soft_lifetime;
		} cri_pack;	
	} u;
#else
	int		cri_klen;	 
	int		cri_mlen;	 
	caddr_t		cri_key;	 
	u_int8_t	cri_iv[EALG_MAX_BLOCK_LEN];	 
#endif
	struct cryptoini *cri_next;
};
#if defined(CONFIG_OCF_M86XXX_MODULE)
#define cri_mlen		u.cri_alg.cri_mlen
#define cri_klen		u.cri_alg.cri_klen
#define cri_key			u.cri_alg.cri_key
#define cri_iv			u.cri_alg.cri_iv
#define crip_basealg			u.cri_pack.basealg
#define crip_tun_source 		u.cri_pack.tun_source
#define crip_tun_destination	u.cri_pack.tun_destination
#define crip_tun_df_mode		u.cri_pack.tun_df_mode
#define crip_tun_ds_mode	u.cri_pack.tun_ds_mode
#define crip_tun_ttl_value	u.cri_pack.tun_ttl_value
#define crip_tun_replay_windowsize u.cri_pack.tun_replay_windowsize
#define crip_spivalue 		u.cri_pack.spivalue
#define crip_replayinit		u.cri_pack.replayinit
#define crip_time_hard_lifetime 	 u.cri_pack.time_hard_lifetime
#define crip_time_soft_lifetime 	 u.cri_pack.time_soft_lifetime
#define crip_byte_hard_lifetime 	 u.cri_pack.byte_hard_lifetime
#define crip_byte_soft_lifetime 	 u.cri_pack.byte_soft_lifetime
#endif

struct cryptodesc {
	int		crd_skip;	 
	int		crd_len;	 
	int		crd_inject;	 
	int		crd_flags;

#define CRD_F_ENCRYPT		0x01	 
#define CRD_F_IV_PRESENT	0x02	 
#define CRD_F_IV_EXPLICIT	0x04	 
#define CRD_F_DSA_SHA_NEEDED	0x08	 
#define CRD_F_KEY_EXPLICIT	0x10	 
#define CRD_F_COMP		0x0f     

	struct cryptoini	CRD_INI;  
#define crd_iv		CRD_INI.cri_iv
#define crd_key		CRD_INI.cri_key
#define crd_alg		CRD_INI.cri_alg
#define crd_klen	CRD_INI.cri_klen
#define crd_mlen	CRD_INI.cri_mlen

	struct cryptodesc *crd_next;
};

struct cryptop {
	struct list_head crp_next;
	wait_queue_head_t crp_waitq;

	u_int64_t	crp_sid;	 
	int		crp_ilen;	 
	int		crp_olen;	 

	int		crp_etype;	 
	int		crp_flags;

#define CRYPTO_F_SKBUF		0x0001	 
#define CRYPTO_F_IOV		0x0002	 
#define CRYPTO_F_REL		0x0004	 
#define CRYPTO_F_BATCH		0x0008	 
#define CRYPTO_F_CBIMM		0x0010	 
#define CRYPTO_F_DONE		0x0020	 
#define CRYPTO_F_CBIFSYNC	0x0040	 

	caddr_t		crp_buf;	 
#if defined(MY_ABC_HERE)
	caddr_t		crp_out_buf;	 
#endif
	caddr_t		crp_opaque;	 
	struct cryptodesc *crp_desc;	 

	int (*crp_callback)(struct cryptop *);  
};
#if defined(CONFIG_OCF_M86XXX_MODULE)
enum crypto_packet_return_code {
		CRYPTO_OK=0,
		CRYPTO_SOFT_TTL = 2,
 		CRYPTO_HARD_TTL,
 		CRYPTO_SA_INACTIVE,
 		CRYPTO_REPLAY,
 		CRYPTO_ICV_FAIL,
 		CRYPTO_SEQ_ROLL,
 		CRYPTO_MEM_ERROR,
 		CRYPTO_VERS_ERROR,
 		CRYPTO_PROT_ERROR,
 		CRYPTO_PYLD_ERROR,
 		CRYPTO_PAD_ERROR 
};

enum crypto_accel_type {
                  CRYPTO_PACKET  =0x2,     
                  CRYPTO_HARDWARE=0x1,
                  CRYPTO_SOFTWARE=0x0
};

enum crypto_flags {
                  CRYPTO_ENCRYPT=0x1, 	 
                  CRYPTO_DECRYPT=0x2,		 
                  CRYPTO_MAC_GEN=0x4,
                  CRYPTO_MAC_CHECK=0x08,
                  CRYPTO_COMPRESS_SMALLER=0x10,
                  CRYPTO_COMPRESS_BIGGER=0x20
};
#endif

#define CRYPTO_BUF_CONTIG	0x0
#define CRYPTO_BUF_IOV		0x1
#define CRYPTO_BUF_SKBUF		0x2

#define CRYPTO_OP_DECRYPT	0x0
#define CRYPTO_OP_ENCRYPT	0x1

#define CRYPTO_HINT_MORE	0x1	 

struct cryptkop {
	struct list_head krp_next;
	wait_queue_head_t krp_waitq;

	int		krp_flags;
#define CRYPTO_KF_DONE		0x0001	 
#define CRYPTO_KF_CBIMM		0x0002	 

	u_int		krp_op;		 
	u_int		krp_status;	 
	u_short		krp_iparams;	 
	u_short		krp_oparams;	 
	u_int		krp_crid;	 
	u_int32_t	krp_hid;
	struct crparam	krp_param[CRK_MAXPARAM];	 
	int		(*krp_callback)(struct cryptkop *);
};

#include <ocf-compat.h>

#define CRYPTO_SESID2HID(_sid)	(((_sid) >> 32) & 0x00ffffff)
#define CRYPTO_SESID2CAPS(_sid)	(((_sid) >> 32) & 0xff000000)
#define CRYPTO_SESID2LID(_sid)	(((u_int32_t) (_sid)) & 0xffffffff)

extern	int crypto_newsession(u_int64_t *sid, struct cryptoini *cri, int hard);
extern	int crypto_freesession(u_int64_t sid);
#define CRYPTOCAP_F_HARDWARE	CRYPTO_FLAG_HARDWARE
#define CRYPTOCAP_F_SOFTWARE	CRYPTO_FLAG_SOFTWARE
#define CRYPTOCAP_F_SYNC	0x04000000	 
extern	int32_t crypto_get_driverid(device_t dev, int flags);
extern	int crypto_find_driver(const char *);
extern	device_t crypto_find_device_byhid(int hid);
extern	int crypto_getcaps(int hid);
extern	int crypto_register(u_int32_t driverid, int alg, u_int16_t maxoplen,
	    u_int32_t flags);
extern	int crypto_kregister(u_int32_t, int, u_int32_t);
extern	int crypto_unregister(u_int32_t driverid, int alg);
extern	int crypto_unregister_all(u_int32_t driverid);
extern	int crypto_dispatch(struct cryptop *crp);
extern	int crypto_kdispatch(struct cryptkop *);
#define CRYPTO_SYMQ	0x1
#define CRYPTO_ASYMQ	0x2
extern	int crypto_unblock(u_int32_t, int);
extern	void crypto_done(struct cryptop *crp);
extern	void crypto_kdone(struct cryptkop *);
extern	int crypto_getfeat(int *);

extern	void crypto_freereq(struct cryptop *crp);
extern	struct cryptop *crypto_getreq(int num);

extern  int crypto_usercrypto;       
extern  int crypto_userasymcrypto;   
extern  int crypto_devallowsoft;     

extern int crypto_rregister(u_int32_t driverid,
		int (*read_random)(void *arg, u_int32_t *buf, int len), void *arg);
extern int crypto_runregister_all(u_int32_t driverid);

struct uio;
extern	void cuio_copydata(struct uio* uio, int off, int len, caddr_t cp);
extern	void cuio_copyback(struct uio* uio, int off, int len, caddr_t cp);
extern	struct iovec *cuio_getptr(struct uio *uio, int loc, int *off);

extern	void crypto_copyback(int flags, caddr_t buf, int off, int size,
	    caddr_t in);
extern	void crypto_copydata(int flags, caddr_t buf, int off, int size,
	    caddr_t out);
extern	int crypto_apply(int flags, caddr_t buf, int off, int len,
	    int (*f)(void *, void *, u_int), void *arg);

#endif  
#endif  

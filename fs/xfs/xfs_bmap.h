 
#ifndef __XFS_BMAP_H__
#define	__XFS_BMAP_H__

struct getbmap;
struct xfs_bmbt_irec;
struct xfs_ifork;
struct xfs_inode;
struct xfs_mount;
struct xfs_trans;

extern kmem_zone_t	*xfs_bmap_free_item_zone;

typedef struct xfs_extdelta
{
	xfs_fileoff_t		xed_startoff;	 
	xfs_filblks_t		xed_blockcount;	 
} xfs_extdelta_t;

typedef struct xfs_bmap_free_item
{
	xfs_fsblock_t		xbfi_startblock; 
	xfs_extlen_t		xbfi_blockcount; 
	struct xfs_bmap_free_item *xbfi_next;	 
} xfs_bmap_free_item_t;

typedef	struct xfs_bmap_free
{
	xfs_bmap_free_item_t	*xbf_first;	 
	int			xbf_count;	 
	int			xbf_low;	 
} xfs_bmap_free_t;

#define	XFS_BMAP_MAX_NMAP	4

#define	XFS_BMAPI_WRITE		0x001	 
#define XFS_BMAPI_DELAY		0x002	 
#define XFS_BMAPI_ENTIRE	0x004	 
#define XFS_BMAPI_METADATA	0x008	 
#define XFS_BMAPI_EXACT		0x010	 
#define XFS_BMAPI_ATTRFORK	0x020	 
#define XFS_BMAPI_ASYNC		0x040	 
#define XFS_BMAPI_RSVBLOCKS	0x080	 
#define	XFS_BMAPI_PREALLOC	0x100	 
#define	XFS_BMAPI_IGSTATE	0x200	 
					 
#define	XFS_BMAPI_CONTIG	0x400	 
 
#define XFS_BMAPI_CONVERT	0x1000	 
					 
static inline int xfs_bmapi_aflag(int w)
{
	return (w == XFS_ATTR_FORK ? XFS_BMAPI_ATTRFORK : 0);
}

#define	DELAYSTARTBLOCK		((xfs_fsblock_t)-1LL)
#define	HOLESTARTBLOCK		((xfs_fsblock_t)-2LL)

static inline void xfs_bmap_init(xfs_bmap_free_t *flp, xfs_fsblock_t *fbp)
{
	((flp)->xbf_first = NULL, (flp)->xbf_count = 0, \
		(flp)->xbf_low = 0, *(fbp) = NULLFSBLOCK);
}

typedef struct xfs_bmalloca {
	xfs_fsblock_t		firstblock;  
	xfs_fsblock_t		rval;	 
	xfs_fileoff_t		off;	 
	struct xfs_trans	*tp;	 
	struct xfs_inode	*ip;	 
	struct xfs_bmbt_irec	*prevp;	 
	struct xfs_bmbt_irec	*gotp;	 
	xfs_extlen_t		alen;	 
	xfs_extlen_t		total;	 
	xfs_extlen_t		minlen;	 
	xfs_extlen_t		minleft;  
	char			eof;	 
	char			wasdel;	 
	char			userdata; 
	char			low;	 
	char			aeof;	 
	char			conv;	 
} xfs_bmalloca_t;

#if defined(__KERNEL__) && defined(XFS_BMAP_TRACE)
 
#define	XFS_BMAP_KTRACE_DELETE	1
#define	XFS_BMAP_KTRACE_INSERT	2
#define	XFS_BMAP_KTRACE_PRE_UP	3
#define	XFS_BMAP_KTRACE_POST_UP	4

#define	XFS_BMAP_TRACE_SIZE	4096	 
#define	XFS_BMAP_KTRACE_SIZE	32	 
extern ktrace_t	*xfs_bmap_trace_buf;

void
xfs_bmap_trace_exlist(
	const char		*fname,		 
	struct xfs_inode	*ip,		 
	xfs_extnum_t		cnt,		 
	int			whichfork);	 
#define	XFS_BMAP_TRACE_EXLIST(ip,c,w)	\
	xfs_bmap_trace_exlist(__func__,ip,c,w)

#else	 

#define	XFS_BMAP_TRACE_EXLIST(ip,c,w)

#endif	 

int					 
xfs_bmap_add_attrfork(
	struct xfs_inode	*ip,	 
	int			size,	 
	int			rsvd);	 

void
xfs_bmap_add_free(
	xfs_fsblock_t		bno,		 
	xfs_filblks_t		len,		 
	xfs_bmap_free_t		*flist,		 
	struct xfs_mount	*mp);		 

void
xfs_bmap_cancel(
	xfs_bmap_free_t		*flist);	 

void
xfs_bmap_compute_maxlevels(
	struct xfs_mount	*mp,	 
	int			whichfork);	 

int						 
xfs_bmap_first_unused(
	struct xfs_trans	*tp,		 
	struct xfs_inode	*ip,		 
	xfs_extlen_t		len,		 
	xfs_fileoff_t		*unused,	 
	int			whichfork);	 

int						 
xfs_bmap_last_before(
	struct xfs_trans	*tp,		 
	struct xfs_inode	*ip,		 
	xfs_fileoff_t		*last_block,	 
	int			whichfork);	 

int						 
xfs_bmap_last_offset(
	struct xfs_trans	*tp,		 
	struct xfs_inode	*ip,		 
	xfs_fileoff_t		*unused,	 
	int			whichfork);	 

int
xfs_bmap_one_block(
	struct xfs_inode	*ip,		 
	int			whichfork);	 

int						 
xfs_bmap_read_extents(
	struct xfs_trans	*tp,		 
	struct xfs_inode	*ip,		 
	int			whichfork);	 

int						 
xfs_bmapi(
	struct xfs_trans	*tp,		 
	struct xfs_inode	*ip,		 
	xfs_fileoff_t		bno,		 
	xfs_filblks_t		len,		 
	int			flags,		 
	xfs_fsblock_t		*firstblock,	 
	xfs_extlen_t		total,		 
	struct xfs_bmbt_irec	*mval,		 
	int			*nmap,		 
	xfs_bmap_free_t		*flist,		 
	xfs_extdelta_t		*delta);	 

int						 
xfs_bmapi_single(
	struct xfs_trans	*tp,		 
	struct xfs_inode	*ip,		 
	int			whichfork,	 
	xfs_fsblock_t		*fsb,		 
	xfs_fileoff_t		bno);		 

int						 
xfs_bunmapi(
	struct xfs_trans	*tp,		 
	struct xfs_inode	*ip,		 
	xfs_fileoff_t		bno,		 
	xfs_filblks_t		len,		 
	int			flags,		 
	xfs_extnum_t		nexts,		 
	xfs_fsblock_t		*firstblock,	 
	xfs_bmap_free_t		*flist,		 
	xfs_extdelta_t		*delta,		 
	int			*done);		 

int
xfs_check_nostate_extents(
	struct xfs_ifork	*ifp,
	xfs_extnum_t		idx,
	xfs_extnum_t		num);

uint
xfs_default_attroffset(
	struct xfs_inode	*ip);

#ifdef __KERNEL__

int						 
xfs_bmap_finish(
	struct xfs_trans	**tp,		 
	xfs_bmap_free_t		*flist,		 
	int			*committed);	 

typedef int (*xfs_bmap_format_t)(void **, struct getbmapx *, int *);

int						 
xfs_getbmap(
	xfs_inode_t		*ip,
	struct getbmapx		*bmv,		 
	xfs_bmap_format_t	formatter,	 
	void			*arg);		 

#ifdef CONFIG_SYNO_PLX_PORTING
int								 
xfs_k_getbmap(
	xfs_inode_t		*ip,
	struct getbmap	*bmv,		 
	struct getbmapx	*bmx,		 
	int              interface); 
#endif

int
xfs_bmap_eof(
	struct xfs_inode        *ip,
	xfs_fileoff_t           endoff,
	int                     whichfork,
	int                     *eof);

int
xfs_bmap_count_blocks(
	xfs_trans_t		*tp,
	struct xfs_inode	*ip,
	int			whichfork,
	int			*count);

#endif	 

#endif	 

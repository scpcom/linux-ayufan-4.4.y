 
#ifndef _LINUX_NET_H
#define _LINUX_NET_H

#include <linux/socket.h>
#include <asm/socket.h>

#define NPROTO		AF_MAX

#define SYS_SOCKET	1		 
#define SYS_BIND	2		 
#define SYS_CONNECT	3		 
#define SYS_LISTEN	4		 
#define SYS_ACCEPT	5		 
#define SYS_GETSOCKNAME	6		 
#define SYS_GETPEERNAME	7		 
#define SYS_SOCKETPAIR	8		 
#define SYS_SEND	9		 
#define SYS_RECV	10		 
#define SYS_SENDTO	11		 
#define SYS_RECVFROM	12		 
#define SYS_SHUTDOWN	13		 
#define SYS_SETSOCKOPT	14		 
#define SYS_GETSOCKOPT	15		 
#define SYS_SENDMSG	16		 
#define SYS_RECVMSG	17		 
#define SYS_ACCEPT4	18		 

typedef enum {
	SS_FREE = 0,			 
	SS_UNCONNECTED,			 
	SS_CONNECTING,			 
	SS_CONNECTED,			 
	SS_DISCONNECTING		 
} socket_state;

#define __SO_ACCEPTCON	(1 << 16)	 

#ifdef __KERNEL__
#include <linux/stringify.h>
#include <linux/random.h>
#include <linux/wait.h>
#include <linux/fcntl.h>	 
#include <linux/kmemcheck.h>

struct poll_table_struct;
struct pipe_inode_info;
struct inode;
struct net;

#define SOCK_ASYNC_NOSPACE	0
#define SOCK_ASYNC_WAITDATA	1
#define SOCK_NOSPACE		2
#define SOCK_PASSCRED		3
#define SOCK_PASSSEC		4

#ifndef ARCH_HAS_SOCKET_TYPES
 
enum sock_type {
	SOCK_STREAM	= 1,
	SOCK_DGRAM	= 2,
	SOCK_RAW	= 3,
	SOCK_RDM	= 4,
	SOCK_SEQPACKET	= 5,
	SOCK_DCCP	= 6,
	SOCK_PACKET	= 10,
};

#define SOCK_MAX (SOCK_PACKET + 1)
 
#define SOCK_TYPE_MASK 0xf

#define SOCK_CLOEXEC	O_CLOEXEC
#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK	O_NONBLOCK
#endif

#endif  

enum sock_shutdown_cmd {
	SHUT_RD		= 0,
	SHUT_WR		= 1,
	SHUT_RDWR	= 2,
};

struct socket {
	socket_state		state;

	kmemcheck_bitfield_begin(type);
	short			type;
	kmemcheck_bitfield_end(type);

	unsigned long		flags;
	 
	struct fasync_struct	*fasync_list;
	wait_queue_head_t	wait;

	struct file		*file;
	struct sock		*sk;
	const struct proto_ops	*ops;
};

struct vm_area_struct;
struct page;
struct kiocb;
struct sockaddr;
struct msghdr;
struct module;

struct proto_ops {
	int		family;
	struct module	*owner;
	int		(*release)   (struct socket *sock);
	int		(*bind)	     (struct socket *sock,
				      struct sockaddr *myaddr,
				      int sockaddr_len);
	int		(*connect)   (struct socket *sock,
				      struct sockaddr *vaddr,
				      int sockaddr_len, int flags);
	int		(*socketpair)(struct socket *sock1,
				      struct socket *sock2);
	int		(*accept)    (struct socket *sock,
				      struct socket *newsock, int flags);
	int		(*getname)   (struct socket *sock,
				      struct sockaddr *addr,
				      int *sockaddr_len, int peer);
	unsigned int	(*poll)	     (struct file *file, struct socket *sock,
				      struct poll_table_struct *wait);
	int		(*ioctl)     (struct socket *sock, unsigned int cmd,
				      unsigned long arg);
	int	 	(*compat_ioctl) (struct socket *sock, unsigned int cmd,
				      unsigned long arg);
	int		(*listen)    (struct socket *sock, int len);
	int		(*shutdown)  (struct socket *sock, int flags);
	int		(*setsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, unsigned int optlen);
	int		(*getsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, int __user *optlen);
	int		(*compat_setsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, unsigned int optlen);
	int		(*compat_getsockopt)(struct socket *sock, int level,
				      int optname, char __user *optval, int __user *optlen);
	int		(*sendmsg)   (struct kiocb *iocb, struct socket *sock,
				      struct msghdr *m, size_t total_len);
	int		(*recvmsg)   (struct kiocb *iocb, struct socket *sock,
				      struct msghdr *m, size_t total_len,
				      int flags);
	int		(*mmap)	     (struct file *file, struct socket *sock,
				      struct vm_area_struct * vma);
	ssize_t		(*sendpage)  (struct socket *sock, struct page *page,
				      int offset, size_t size, int flags);
#ifdef CONFIG_SYNO_PLX_PORTING
	ssize_t		(*sendpages)  (struct socket *sock, struct page **page,
				      int offset, size_t size, int flags);
#endif
	ssize_t 	(*splice_read)(struct socket *sock,  loff_t *ppos,
				       struct pipe_inode_info *pipe, size_t len, unsigned int flags);
};

struct net_proto_family {
	int		family;
	int		(*create)(struct net *net, struct socket *sock, int protocol);
	struct module	*owner;
};

struct iovec;
struct kvec;

enum {
	SOCK_WAKE_IO,
	SOCK_WAKE_WAITD,
	SOCK_WAKE_SPACE,
	SOCK_WAKE_URG,
};

extern int	     sock_wake_async(struct socket *sk, int how, int band);
extern int	     sock_register(const struct net_proto_family *fam);
extern void	     sock_unregister(int family);
extern int	     sock_create(int family, int type, int proto,
				 struct socket **res);
extern int	     sock_create_kern(int family, int type, int proto,
				      struct socket **res);
extern int	     sock_create_lite(int family, int type, int proto,
				      struct socket **res); 
extern void	     sock_release(struct socket *sock);
extern int   	     sock_sendmsg(struct socket *sock, struct msghdr *msg,
				  size_t len);
extern int	     sock_recvmsg(struct socket *sock, struct msghdr *msg,
				  size_t size, int flags);
extern int 	     sock_map_fd(struct socket *sock, int flags);
extern struct socket *sockfd_lookup(int fd, int *err);
#define		     sockfd_put(sock) fput(sock->file)
extern int	     net_ratelimit(void);

#define net_random()		random32()
#define net_srandom(seed)	srandom32((__force u32)seed)

extern int   	     kernel_sendmsg(struct socket *sock, struct msghdr *msg,
				    struct kvec *vec, size_t num, size_t len);
extern int   	     kernel_recvmsg(struct socket *sock, struct msghdr *msg,
				    struct kvec *vec, size_t num,
				    size_t len, int flags);

extern int kernel_bind(struct socket *sock, struct sockaddr *addr,
		       int addrlen);
extern int kernel_listen(struct socket *sock, int backlog);
extern int kernel_accept(struct socket *sock, struct socket **newsock,
			 int flags);
extern int kernel_connect(struct socket *sock, struct sockaddr *addr,
			  int addrlen, int flags);
extern int kernel_getsockname(struct socket *sock, struct sockaddr *addr,
			      int *addrlen);
extern int kernel_getpeername(struct socket *sock, struct sockaddr *addr,
			      int *addrlen);
extern int kernel_getsockopt(struct socket *sock, int level, int optname,
			     char *optval, int *optlen);
extern int kernel_setsockopt(struct socket *sock, int level, int optname,
			     char *optval, unsigned int optlen);
extern int kernel_sendpage(struct socket *sock, struct page *page, int offset,
			   size_t size, int flags);
extern int kernel_sock_ioctl(struct socket *sock, int cmd, unsigned long arg);
extern int kernel_sock_shutdown(struct socket *sock,
				enum sock_shutdown_cmd how);

#ifndef CONFIG_SMP
#define SOCKOPS_WRAPPED(name) name
#define SOCKOPS_WRAP(name, fam)
#else

#define SOCKOPS_WRAPPED(name) __unlocked_##name

#define SOCKCALL_WRAP(name, call, parms, args)		\
static int __lock_##name##_##call  parms		\
{							\
	int ret;					\
	lock_kernel();					\
	ret = __unlocked_##name##_ops.call  args ;\
	unlock_kernel();				\
	return ret;					\
}

#define SOCKCALL_UWRAP(name, call, parms, args)		\
static unsigned int __lock_##name##_##call  parms	\
{							\
	int ret;					\
	lock_kernel();					\
	ret = __unlocked_##name##_ops.call  args ;\
	unlock_kernel();				\
	return ret;					\
}

#define SOCKOPS_WRAP(name, fam)					\
SOCKCALL_WRAP(name, release, (struct socket *sock), (sock))	\
SOCKCALL_WRAP(name, bind, (struct socket *sock, struct sockaddr *uaddr, int addr_len), \
	      (sock, uaddr, addr_len))				\
SOCKCALL_WRAP(name, connect, (struct socket *sock, struct sockaddr * uaddr, \
			      int addr_len, int flags), 	\
	      (sock, uaddr, addr_len, flags))			\
SOCKCALL_WRAP(name, socketpair, (struct socket *sock1, struct socket *sock2), \
	      (sock1, sock2))					\
SOCKCALL_WRAP(name, accept, (struct socket *sock, struct socket *newsock, \
			 int flags), (sock, newsock, flags)) \
SOCKCALL_WRAP(name, getname, (struct socket *sock, struct sockaddr *uaddr, \
			 int *addr_len, int peer), (sock, uaddr, addr_len, peer)) \
SOCKCALL_UWRAP(name, poll, (struct file *file, struct socket *sock, struct poll_table_struct *wait), \
	      (file, sock, wait)) \
SOCKCALL_WRAP(name, ioctl, (struct socket *sock, unsigned int cmd, \
			 unsigned long arg), (sock, cmd, arg)) \
SOCKCALL_WRAP(name, compat_ioctl, (struct socket *sock, unsigned int cmd, \
			 unsigned long arg), (sock, cmd, arg)) \
SOCKCALL_WRAP(name, listen, (struct socket *sock, int len), (sock, len)) \
SOCKCALL_WRAP(name, shutdown, (struct socket *sock, int flags), (sock, flags)) \
SOCKCALL_WRAP(name, setsockopt, (struct socket *sock, int level, int optname, \
			 char __user *optval, unsigned int optlen), (sock, level, optname, optval, optlen)) \
SOCKCALL_WRAP(name, getsockopt, (struct socket *sock, int level, int optname, \
			 char __user *optval, int __user *optlen), (sock, level, optname, optval, optlen)) \
SOCKCALL_WRAP(name, sendmsg, (struct kiocb *iocb, struct socket *sock, struct msghdr *m, size_t len), \
	      (iocb, sock, m, len)) \
SOCKCALL_WRAP(name, recvmsg, (struct kiocb *iocb, struct socket *sock, struct msghdr *m, size_t len, int flags), \
	      (iocb, sock, m, len, flags)) \
SOCKCALL_WRAP(name, mmap, (struct file *file, struct socket *sock, struct vm_area_struct *vma), \
	      (file, sock, vma)) \
	      \
static const struct proto_ops name##_ops = {			\
	.family		= fam,				\
	.owner		= THIS_MODULE,			\
	.release	= __lock_##name##_release,	\
	.bind		= __lock_##name##_bind,		\
	.connect	= __lock_##name##_connect,	\
	.socketpair	= __lock_##name##_socketpair,	\
	.accept		= __lock_##name##_accept,	\
	.getname	= __lock_##name##_getname,	\
	.poll		= __lock_##name##_poll,		\
	.ioctl		= __lock_##name##_ioctl,	\
	.compat_ioctl	= __lock_##name##_compat_ioctl,	\
	.listen		= __lock_##name##_listen,	\
	.shutdown	= __lock_##name##_shutdown,	\
	.setsockopt	= __lock_##name##_setsockopt,	\
	.getsockopt	= __lock_##name##_getsockopt,	\
	.sendmsg	= __lock_##name##_sendmsg,	\
	.recvmsg	= __lock_##name##_recvmsg,	\
	.mmap		= __lock_##name##_mmap,		\
};

#endif

#define MODULE_ALIAS_NETPROTO(proto) \
	MODULE_ALIAS("net-pf-" __stringify(proto))

#define MODULE_ALIAS_NET_PF_PROTO(pf, proto) \
	MODULE_ALIAS("net-pf-" __stringify(pf) "-proto-" __stringify(proto))

#define MODULE_ALIAS_NET_PF_PROTO_TYPE(pf, proto, type) \
	MODULE_ALIAS("net-pf-" __stringify(pf) "-proto-" __stringify(proto) \
		     "-type-" __stringify(type))

#ifdef CONFIG_SYSCTL
#include <linux/sysctl.h>
extern struct ratelimit_state net_ratelimit_state;
#endif

#endif  
#endif	 

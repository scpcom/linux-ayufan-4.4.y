#ifndef _LINUX_SYNOTIFY_H
#define _LINUX_SYNOTIFY_H

#include <linux/types.h>

#define SYNO_ACCESS			0x00000001	 
#define SYNO_MODIFY			0x00000002	 
#define SYNO_ATTRIB			0x00000004	 
#define SYNO_CLOSE_WRITE		0x00000008	 
#define SYNO_CLOSE_NOWRITE	0x00000010	 
#define SYNO_OPEN			0x00000020	 
#define SYNO_MOVE_FROM		0x00000040	 
#define SYNO_MOVE_TO		0x00000080	 
#define SYNO_CREATE			0x00000100	 
#define SYNO_DELETE			0x00000200	 
#define SYNO_Q_OVERFLOW		0x00004000	 
#define SYNO_ONDIR			0x40000000	 

#define SYNO_CLOEXEC		0x00000001
#define SYNO_NONBLOCK		0x00000002

#define SYNO_DONT_FOLLOW		0x01000000	 

#define SYNO_ALL_EVENTS (SYNO_ACCESS | \
						 SYNO_MODIFY | \
						 SYNO_ATTRIB | \
						 SYNO_CLOSE_WRITE | \
						 SYNO_CLOSE_NOWRITE | \
						 SYNO_OPEN | \
						 SYNO_MOVE_FROM | \
						 SYNO_MOVE_TO | \
						 SYNO_CREATE | \
						 SYNO_DELETE | \
						 SYNO_Q_OVERFLOW | \
						 SYNO_ONDIR| \
						 SYNO_Q_OVERFLOW)
 
struct synotify_event {
	__u32		mask;		 
	__u32		cookie;		 
	__u32		len;		 
	char		name[0];	 
};

#endif

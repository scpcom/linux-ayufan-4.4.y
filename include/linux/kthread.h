#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#ifndef _LINUX_KTHREAD_H
#define _LINUX_KTHREAD_H
 
#include <linux/err.h>
#include <linux/sched.h>

struct task_struct *kthread_create(int (*threadfn)(void *data),
				   void *data,
				   const char namefmt[], ...)
	__attribute__((format(printf, 3, 4)));

#define kthread_run(threadfn, data, namefmt, ...)			   \
({									   \
	struct task_struct *__k						   \
		= kthread_create(threadfn, data, namefmt, ## __VA_ARGS__); \
	if (!IS_ERR(__k))						   \
		wake_up_process(__k);					   \
	__k;								   \
})

#ifdef MY_DEF_HERE
#define kthread_run_on_cpu(cpu, threadfn, data, namefmt, ...) \
({ \
	struct task_struct *__k = kthread_create(threadfn, data, namefmt, ## __VA_ARGS__); \
	if (!IS_ERR(__k)) \
	kthread_bind(__k, cpu); \
	wake_up_process(__k); \
	__k; \
})
#endif

void kthread_bind(struct task_struct *k, unsigned int cpu);
int kthread_stop(struct task_struct *k);
int kthread_should_stop(void);

int kthreadd(void *unused);
extern struct task_struct *kthreadd_task;

#endif  

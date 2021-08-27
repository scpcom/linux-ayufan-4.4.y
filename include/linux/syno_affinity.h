 
#ifndef __SYNO_AFFINITY_H_
#define __SYNO_AFFINITY_H_

static inline void SYNOSetTaskAffinity(struct task_struct *tsk, unsigned int cpu)
{
	if (!cpumask_empty(&(tsk->cpus_allowed))) {
		set_cpus_allowed_ptr(tsk, cpumask_of(cpu));
	}
}

#endif  

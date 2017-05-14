#include <linux/syscalls.h>
#include <linux/sched.h>
#include "sched/sched.h"
#include <asm-generic/errno-base.h> // for usage of -EFAULT
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>

static DEFINE_SPINLOCK(set_weight_lock);

bool check_same_owner(struct task_struct *p){
	const struct cred *cred = current_cred(), *pcred;
	bool match;
	
	rcu_read_lock();
	pcred = __task_cred(p);
	match = (uid_eq(cred->euid, pcred->euid) || uid_eq(cred-> euid, pcred->uid));
	rcu_read_unlock();
	
	return match;
}

asmlinkage int sys_sched_setweight(pid_t pid, int weight)
{
	struct task_struct *task = NULL;
	struct pid *pid_struct = NULL;
	struct rq *rq = NULL;
	struct wrr_rq *wrr_rq = NULL;
	int old_weight;

	if (pid < 0)
		return -EINVAL;
	if (weight < 1 || weight > 20)
		return -EINVAL;

	/* if pid is 0, use the calling process task */
	if (pid == 0) 
		task = current;
	else 
	{
		pid_struct = find_get_pid(pid);
		if(pid_struct == NULL)
			return -EINVAL;
		task = get_pid_task(pid_struct, PIDTYPE_PID);
		if(task == NULL)
			return -EINVAL;
	}

	if(task->policy != SCHED_WRR)
		return -EINVAL;

	old_weight = task->wrr.weight;
	
	if(current_uid() != 0 && current_euid() != 0)
	{
		if(!check_same_owner(task))
			return -EPERM;
		
		if(weight > task->wrr.weight)
			return -EPERM;	

		task->wrr.weight = weight;
	}
	else
		task->wrr.weight = (unsigned int) weight;
	
	rq = task_rq(task);
	
	if( rq->curr == task || current == task)
		task->wrr.time_slice = task->wrr.weight * 10;
	else {
		task->wrr.time_slice = task->wrr.weight * 10;
		task->wrr.time_left = task->wrr.time_slice;
	}
	
	spin_lock(&set_weight_lock);

	wrr_rq = &rq->wrr;
	wrr_rq->total_weight -= old_weight;
	wrr_rq->total_weight += weight;
	
	spin_unlock(&set_weight_lock);

	printk(KERN_DEBUG "SET_WEIGHT_INFO task pid %d is set as weight %d", task_pid_nr(current), task->wrr.weight);
	return 0;
}

asmlinkage int sys_sched_getweight(pid_t pid){
	int result;
	struct task_struct *task = NULL;
	struct pid *pid_struct = NULL;

	if (pid < 0)
		return -EINVAL;

	if (pid == 0)
		result = current->wrr.weight;
	else {
		pid_struct = find_get_pid(pid);
		if(pid_struct == NULL)
			return -EINVAL;
		
		task = get_pid_task(pid_struct, PIDTYPE_PID);
		if(task == NULL)
			return -EINVAL;

		result = task->wrr.weight;
	}

	return result;	
}

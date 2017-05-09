#include <linux/syscalls.h>
#include <linux/sched.h>
#include "sched/sched.h"
#include <asm-generic/errno-base.h> // for usage of -EFAULT
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>

static DEFINE_SPINLOCK(set_weight_lock);

int valid_weight(unsigned int weight){
	if(weight < SCHED_WRR_MIN_WEIGHT || weight > SCHED_WRR_MAX_WEIGHT)
		return 0;
	else
		return 1;
}

bool check_same_owner(struct task_struct *p){
	const struct cred *cred = current_cred(), *pcred;
	bool match;
	
	rcu_read_lock();
	pcred = __task_cred(p);
	match = (uid_eq(cred->euid, pcred->euid) || uid_eq(cred-> euid, pcred->uid));
	rcu_read_unlock();
	
	return match;
}

asmlinkage int sched_setweight(pid_t pid, int weight){
	struct task_struct *task = NULL;
	struct pid *pid_struct = NULL;
	struct rq *rq = NULL;
	struct wrr_rq *wrr_rq = NULL;
	int old_weight;

	if (pid < 0)
		return -EINVAL;
	if (weight <0 || !valid_weight(weight))
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
	

	// update the time slice computation
		
	
	
	spin_lock(&set_weight_lock);

	rq = task_rq(task);
	wrr_rq = &rq->wrr;
	wrr_rq->total_weight -= old_weight;
	wrr_rq->total_weight += weight;
	
	spin_unlock(&set_weight_lock);
	return 0;
}

asmlinkage int sched_getweight(pid_t pid){
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

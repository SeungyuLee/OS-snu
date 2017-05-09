/* 
 * Weighted Round Robin Scheduling Class 
 * (mapped to the SCHED_WRR policy)
 */

#include "sched.h"
#include <linux/slab.h>
#define WRR_DEFAULT_TIMESLICE 10

static DEFINE_SPINLOCK(SET_WEIGHT_LOCK);

#ifdef CONFIG_SMP
static void load_balance_wrr(void);
static DEFINE_SPINLOCK(LOAD_BALANCE_LOCK);
#endif

void init_wrr_rq(struct wrr_rq *wrr_rq, struct rq *rq)
{
	INIT_LIST_HEAD(&wrr_rq->queue);
}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
	struct wrr_rq *wrr_rq = &rq->wrr;
	int cpu;
	if (wrr_se == NULL) return;
	if (flags & ENQUEUE_HEAD)
			list_add(&wrr_se->run_list, &wrr_rq->queue);
	else
			list_add_tail(&wrr_se->run_list, &wrr_rq->queue);
	wrr_rq->wrr_nr_running++;
	inc_nr_running(rq);
	cpu = cpu_of(rq);
	raw_spin_lock(&wrr_info_locks[cpu]);
	my_wrr_info.nr_running[cpu]++;
	my_wrr_info.total_weight[cpu] += wrr_se->weight;
	raw_spin_unlock(&wrr_info_locks[cpu]);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	list_del(&(p->wrr.run_list));
	dec_nr_running(rq);
	rq->wrr.nr_running--;
	rq->wrr.total_weight -= p->wrr.weight;
	if (p->wrr.weight > 1)
			p->wrr.weight = p->wrr.weight - 1;

#ifdef CONFIG_SMP
	if (rq->wrr.nr_running == 0)
			pull_task_from_cpus(rq);
#endif
}

static void yield_task_wrr(struct rq *rq)
{
	/* needs to be implemented */
}

static int select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
	int curr_cpu = task_cpu(p);
	int cpu;
	int minimum_weight = INT_MAX;

	if (p->nr_cpus_allowed == 1)
		return curr_cpu;

	if (sd_flag != SD_BALANCE_WAKE && sd_flag != SD_BALANCE_FORK)
		return curr_cpu;

	rcu_read_lock();
	for_each_possible_cpu(cpu){
		if (my_wrr_info.total_weight[cpu] < minimum_weight) {
			minimum_weight = my_wrr_info.total_weight[cpu];
			curr_cpu = cpu;
		}


	}

	rcu_read_unlock();

	return curr_cpu;
}


static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct task_struct *next = NULL;
	if (!list_empty(&(rq->wrr.queue)))
		next = _find_container(rq->wrr.queue.next):
	return next;
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct sched_wr_entity *wrr_se = &p->wrr;
	struct wrr_rq *wrr_rq = &rq->wrr;
	
	if (wrr_se == NULL)
		return;

	if (--p->wrr.time_slice)
		return;

	p->wrr.time_slice = WRR_DEFAULT_TIMESLICE * p->wrr.weight;
	
	list_move_tail(&wrr_se->run_list, &wrr_rq->queue);
		
	set_tsk_need_resched(p);
	return;
}

static bool yield_to_task_wrr(struct rq *rq, struct task_struct *p, bool preempt)
{	return 0;	}
static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags){}
static void put_prev_task_wrr(struct rq *rq, struct task_struct *p){}
static void set_curr_task_wrr(struct rq *rq){}
static void task_fork_wrr(struct task_struct *p){}
static void switched_from_wrr(struct rq *this_rq, struct task_struct *task){}
static void switched_to_wrr(struct rq *this_rq, struct task_struct *task){}
static void prio_changed_wrr(struct rq *this_rq, struct task_struct *task, int oldprio){}
static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{	return 0;	}

const struct sched_class sched_wrr_class = 
{
	.next			= &fair_sched_class,
	.enqueue_task	= enqueue_task_wrr,
	.dequeue_task	= dequeue_task_wrr,
	.yield_task		= yield_task_wrr,	
	.yield_to_task	= yield_to_task_wrr,
	.check_preempt_curr	= check_preempt_curr_wrr,
	.pick_next_task = pick_next_task_wrr,
	.put_prev_task 	= put_prev_task_wrr,
#ifdef CONFIG_SMP
	.select_task_rq	= select_task_rq_wrr,
#endif
	.set_curr_task 	= set_curr_task_wrr,
	.task_tick 		= task_tick_wrr,
	.task_fork		= task_fork_wrr,
	.switched_from	= switched_from_wrr,
	.switched_to	= switched_to_wrr,
	.prio_changed	= prio_changed_wrr,
	.get_rr_interval= get_rr_interval_wrr,
};


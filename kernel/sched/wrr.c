/* 
 * Weighted Round Robin Scheduling Class 
 * (mapped to the SCHED_WRR policy)
 */

static const struct sched_class wrr_sched_class;

#include "sched.h"
#include <linux/slab.h>
#define TIME_SLICE 10
#define DEFAULT_WEIGHT 10

#ifdef CONFIG_SMP
static void load_balance_wrr(void);
static DEFINE_SPINLOCK(LOAD_BALANCE_LOCK);
#endif

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
		struct sched_wrr_entity *wrr_entity;
		wrr_rq->nr_running = 0;
		wrr_rq->size = 0;
		wrr_rq->curr = NULL;
		wrr_rq->total_weight = 0;

		spin_lock_init(&(wrr_rq->wrr_rq_lock));

		wrr_entity = &wrr_rq->run_queue;
		INIT_LIST_HEAD(&wrr_entity->run_list);

		wrr_entity->task = NULL;
		wrr_entity->weight = 0;
		wrr_entity->time_slice = 0;
		wrr_entity->time_left = 0;
}


static void init_task_wrr(struct task_struct *p)
{
	struct sched_wrr_entity *wrr_entity;
	if (p == NULL)	return;

	wrr_entity = &p->wrr;
	wrr_entity->task = p;
	int weight = wrr_entity->weight;
	if (weight == DEFAULT_WEIGHT ||	weight < 1 || weight > 20)	{
		wrr_entity->weight = DEFAULT_WEIGHT;
		wrr_entity->time_slice = DEFAULT_WEIGHT * TIME_SLICE;
		wrr_entity->time_left =	wrr_entity->time_slice;
	} else { 
		wrr_entity->time_slice = wrr_entity->weight * TIME_SLICE;
		wrr_entity->time_left = wrr_entity->time_slice;
	}

	INIT_LIST_HEAD(&wrr_entity->run_list);
}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *new_entity;
	struct sched_wrr_entity *wrr_entity = &wrr_rq->run_queue;
	
	init_task_wrr(p);
	new_entity = &p->wrr;

	if (false == list_empty(&new_entity->run_list)) return;

	spin_lock(&wrr_rq->wrr_rq_lock);
	
	struct list_head *head = &wrr_entity->run_list;
	list_add_tail(&new_entity->run_list, head);

	++wrr_rq->nr_running;
	++wrr_rq->size;
	wrr_rq->total_weight += new_entity->weight;
	
	spin_unlock(&wrr_rq->wrr_rq_lock);
}

static void update_curr_wrr(struct rq *rq)
{
	struct task_struct *curr = rq->curr;

	u64 delta_exec;

	if (curr->sched_class != &wrr_sched_class)
			return;

	delta_exec = rq->clock_task - curr->se.exec_start;
	
	if (unlikely((s64)delta_exec < 0)) delta_exec = 0;

	schedstat_set(curr->se.statistics.exec_max, max(curr->se.statistics.exec_max, delta_exec));

	curr->se.sum_exec_runtime += delta_exec;
	account_group_exec_runtime(curr, delta_exec);

	curr->se.exec_start = rq->clock_task;
	cpuacct_charge(curr, delta_exec);
}

static void requeue_task_wrr(struct rq *rq, struct task_struct *p)
{
	struct list_head *head;
	struct sched_wrr_entity *wrr_entity;
	struct wrr_rq *wrr_rq;
	if (p == NULL) {
		wrr_entity = NULL;
	}
	else {
		wrr_entity = &p->wrr;
	}
	if (rq == NULL) {
		wrr_rq = NULL;
	}
	else {
		wrr_rq = &rq->wrr;
	}

	head = &wrr_rq->run_queue.run_list;

	if (wrr_rq->size == 1)
		return;

	spin_lock(&wrr_rq->wrr_rq_lock);
	list_move_tail(&wrr_entity->run_list, head);
	spin_unlock(&wrr_rq->wrr_rq_lock);
}

static struct wrr_rq *wrr_rq_of_wrr_entity(struct sched_wrr_entity *wrr_entity)
{
	struct task_struct *p = wrr_entity->task;
	struct rq *rq = task_rq(p);
	
	return &rq->wrr;
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	struct wrr_rq *wrr_rq = wrr_rq_of_wrr_entity(wrr_entity);
	
	spin_lock(&wrr_rq->wrr_rq_lock);
		
	update_curr_wrr(rq);
	
	list_del(&wrr_entity->run_list);
	--wrr_rq->nr_running;
	--wrr_rq->size;
							
	wrr_rq->total_weight -= wrr_entity->weight;
								
	spin_unlock(&wrr_rq->wrr_rq_lock);
}

static void yield_task_wrr(struct rq *rq)
{
	requeue_task_wrr(rq, rq->curr);
}


static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct task_struct *p;
	struct sched_wrr_entity *head_entity;
	struct sched_wrr_entity *next_entity;
	struct list_head *head;
	struct wrr_rq *wrr_rq =  &rq->wrr;

	if (rq->nr_running <= 0)
		return NULL;

	head_entity = &wrr_rq->run_queue;
	head = &head_entity->run_list;

	next_entity = list_entry(head->next, struct sched_wrr_entity, run_list);

	p = next_entity->task;

	if (p == NULL)
		return p;

	p->se.exec_start = rq->clock_task;

	return p;
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
		struct sched_wrr_entity *wrr_entity = &p->wrr;

		update_curr_wrr(rq);
printk(KERN_DEBUG "TASK_TICK_INFO cpu number %d task pid %d is ticking %d", task_cpu(p), task_pid_nr(current), wrr_entity->time_left);


		if (--wrr_entity->time_left) return;

		wrr_entity->time_left = wrr_entity->time_slice;

		if (wrr_entity->run_list.prev != wrr_entity->run_list.next) {
			requeue_task_wrr(rq, p);
			set_tsk_need_resched(p);
		} else {
			set_tsk_need_resched(p);
	}

}


static int most_idle_cpu(void)
{	
	int cpu, total_weight;
	int idle_cpu = -1;
	int lowest_total_weight = INT_MAX;
	struct rq *rq;
	struct wrr_rq *wrr_rq;

	for_each_online_cpu(cpu) {
		rq = cpu_rq(cpu);
		wrr_rq = &rq->wrr;
		total_weight = wrr_rq->total_weight;

		if (lowest_total_weight > total_weight) {
			lowest_total_weight = total_weight;
			idle_cpu = cpu;
		}
	}

	return idle_cpu;
}

static int select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
	int cpu = most_idle_cpu();

	if (cpu == -1) return task_cpu(p);

	return cpu;
}

static void set_curr_task_wrr(struct rq *rq)
{
	struct task_struct *p = rq->curr;

	p->se.exec_start = rq->clock_task;

	rq->wrr.curr = &p->wrr;
}

static void load_balance_wrr(void)
{
	/* this has to be implemented */
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{
	update_curr_wrr(rq);
}

static void task_fork_wrr(struct task_struct *p)
{
	struct sched_wrr_entity *wrr_entity;
	if (p == NULL) return;
	wrr_entity = &p->wrr;
	wrr_entity->task = p;

	wrr_entity->time_slice =
			wrr_entity->weight * TIME_SLICE;
	wrr_entity->time_left = 
			wrr_entity->time_slice;
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	if (task == NULL)
		return -EINVAL;
	return task->wrr.weight * TIME_SLICE;
}

static void switched_to_wrr(struct rq *this_rq, struct task_struct *task)
{
	struct sched_wrr_entity *wrr_entity = &task->wrr;
	wrr_entity->task = task;
	wrr_entity->weight = DEFAULT_WEIGHT;
	wrr_entity->time_slice = DEFAULT_WEIGHT * TIME_SLICE;
	wrr_entity->time_left = wrr_entity->time_slice;
}

static void rq_online_wrr(struct rq *rq){}
static void rq_offline_wrr(struct rq *rq){}
static void pre_schedule_wrr(struct rq *rq, struct task_struct *prev){}
static void post_schedule_wrr(struct rq *rq){}
static void task_woken_wrr(struct rq *rq, struct task_struct *p){}
static bool yield_to_task_wrr(struct rq *rq, struct task_struct *p, bool preempt)
{	return 0;	}
static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags){}
static void switched_from_wrr(struct rq *this_rq, struct task_struct *task){}
static void prio_changed_wrr(struct rq *this_rq, struct task_struct *task, int oldprio){}

const struct sched_class wrr_sched_class = 
{
	.next			= &fair_sched_class,
	.enqueue_task	= enqueue_task_wrr,
	.dequeue_task	= dequeue_task_wrr,
	.yield_task		= yield_task_wrr,	
//	.yield_to_task	= yield_to_task_wrr,
	.check_preempt_curr	= check_preempt_curr_wrr,
	.pick_next_task = pick_next_task_wrr,
	.put_prev_task 	= put_prev_task_wrr,
#ifdef CONFIG_SMP
	.select_task_rq	= select_task_rq_wrr,
	.rq_online		= rq_online_wrr,
	.rq_offline		= rq_offline_wrr,
	.pre_schedule	= pre_schedule_wrr,
	.post_schedule	= post_schedule_wrr,
	.task_woken		= task_woken_wrr,
	.switched_from	= switched_from_wrr,
#endif
	.set_curr_task 	= set_curr_task_wrr,
	.task_tick 		= task_tick_wrr,
//	.task_fork		= task_fork_wrr,
	.switched_to	= switched_to_wrr,
	.prio_changed	= prio_changed_wrr,
	.get_rr_interval= get_rr_interval_wrr,
};


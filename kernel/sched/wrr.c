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

static int valid_weight(unsigned int weight)
{
	if(weight >= SCHED_WRR_MIN_WEIGHT && weight <= SCHED_WRR_MAX_WEIGHT)
		return 1;
	else
		return 0;
}

static void init_task_wrr(struct task_struct *p)
{
	struct sched_wrr_entity *wrr_entity;
	if (p == NULL)	return;

	wrr_entity = &p->wrr;
	wrr_entity->task = p;
	/* Use Default Parameters if the weight is still the default, * or weight is invalid */
	if (wrr_entity->weight == SCHED_WRR_DEFAULT_WEIGHT ||								    !valid_weight(wrr_entity->weight)) {

		wrr_entity->weight = SCHED_WRR_DEFAULT_WEIGHT;

		wrr_entity->time_slice = SCHED_WRR_DEFAULT_WEIGHT * SCHED_WRR_TIME_QUANTUM;
		wrr_entity->time_left =	wrr_entity->time_slice / SCHED_WRR_TICK_FACTOR;
	} else { /* Use the current weight value */
		wrr_entity->time_slice = wrr_entity->weight * SCHED_WRR_TIME_QUANTUM;
		wrr_entity->time_left = wrr_entity->time_slice / SCHED_WRR_TICK_FACTOR;

	}
	/* Initialize the list head just to be safe */
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
	
	/* Remove the task from the queue */
	list_del(&wrr_entity->run_list);
					
	/* update statistics counts */
	--wrr_rq->nr_running;
	--wrr_rq->size;
							
	wrr_rq->total_weight -= wrr_entity->weight;
								
	spin_unlock(&wrr_rq->wrr_rq_lock);

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
		next = _find_container(rq->wrr.queue.next);
	return next;
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct sched_wrr_entity *wrr_se = &p->wrr;
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


static int most_idle_cpu(void)
{	// counter needed?
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
	/*
	int cpu;
	int selected_cpu;
	struct rq *rq, *rq_of_the_task, *rq_of_most_idle_wrr;
	struct wrr_rq *wrr_rq;
	struct wrr_rq *most_idle_wrr_rq, *most_busy_wrr_rq;
	struct sched_wrr_entity *wrr_entity, *heaviest_entity_in_most_busy_wrr_rq;
	struct list_head *curr;
	struct list_head *head;
	struct task_struct *task_to_be_moved;

	int lowest_total_weight = INT_MAX;
	int highest_total_weight = INT_MIN;
	
	int biggest_weight 
	*/
}


static bool yield_to_task_wrr(struct rq *rq, struct task_struct *p, bool preempt)
{	return 0;	}
static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags){}
static void put_prev_task_wrr(struct rq *rq, struct task_struct *p){}
static void task_fork_wrr(struct task_struct *p){}
static void switched_from_wrr(struct rq *this_rq, struct task_struct *task){}
static void switched_to_wrr(struct rq *this_rq, struct task_struct *task){}
static void prio_changed_wrr(struct rq *this_rq, struct task_struct *task, int oldprio){}
static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{	return 0;	}

/* task_fork_wrr, get_rr_interval_wrr, switched_to_wrr, put_prev_task_wrr
   yield_task_wrr, load_balance_wrr probably have to be implemented
*/


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


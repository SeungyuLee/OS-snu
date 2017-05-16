/* 
 * Weighted Round Robin Scheduling Class 
 * (mapped to the SCHED_WRR policy)
 */

const struct sched_class wrr_sched_class;

#include "sched.h"
#include <linux/slab.h>
#define TIME_SLICE (HZ / 100)
#define DEFAULT_WEIGHT 10

static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_entity) {
	return container_of(wrr_entity, struct task_struct, wrr);
}

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
		wrr_rq->nr_running = 0;
		wrr_rq->curr = NULL;
		wrr_rq->total_weight = 0;
		raw_spin_lock_init(&wrr_rq->wrr_rq_lock);
		INIT_LIST_HEAD(&wrr_rq->run_queue);

}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	struct list_head *entity_list = &wrr_entity->run_list;
	struct list_head *rq_list = &wrr_rq->run_queue;

	raw_spin_lock(&wrr_rq->wrr_rq_lock);
	
	if (wrr_rq->curr == NULL) {
		wrr_rq->curr = p;
		list_add_tail(entity_list,rq_list);
	}else {
		list_add_tail(entity_list, &(&wrr_rq->curr->wrr)->run_list);
	}

	++wrr_rq->nr_running;
	inc_nr_running(rq);
	wrr_rq->total_weight += wrr_entity->weight;
	p->on_rq = 1;

	raw_spin_unlock(&wrr_rq->wrr_rq_lock);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct list_head *entity_list = &wrr_entity->run_list;
	struct list_head *rq_list = &wrr_rq->run_queue;
	struct list_head *next = (&wrr_entity->run_list)->next;

	raw_spin_lock(&wrr_rq->wrr_rq_lock);
	
	list_del_init(&wrr_entity->run_list);
	
	if (wrr_rq->run_queue.next == &wrr_rq->run_queue){ // runqueue is empty
		wrr_rq->curr = NULL;
	} else if (p == wrr_rq->curr) {
		if(next == rq_list) {
			next = rq_list->next;
		}
		wrr_rq->curr = wrr_task_of(list_entry(next,struct sched_wrr_entity, run_list));
	}
	
	--wrr_rq->nr_running;
	dec_nr_running(rq);
	wrr_rq->total_weight -= wrr_entity->weight;
	p->on_rq = 0;
								
	raw_spin_unlock(&wrr_rq->wrr_rq_lock);
}

static void yield_task_wrr(struct rq *rq)
{
}


static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct task_struct *curr = rq->wrr.curr;

	if ( NULL == curr) {
		return NULL;
	}
	curr->wrr.time_left = curr->wrr.time_slice;
	return curr;
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *wrr_entity;
	struct list_head *next;

	raw_spin_lock(&wrr_rq->wrr_rq_lock);

	if(NULL == p) return;

	wrr_entity = &p->wrr;

	if (--wrr_entity->time_left) {
		raw_spin_unlock(&wrr_rq->wrr_rq_lock);
		return;
	}

	if (wrr_entity->run_list.prev != wrr_entity->run_list.next) {
		next = wrr_entity->run_list.next;
		if(next == &wrr_rq->run_queue)
			next = next->next;
		wrr_rq->curr = wrr_task_of(list_entry(next, struct sched_wrr_entity,run_list));
		set_tsk_need_resched(p);
	} else {
		wrr_entity->time_left = wrr_entity->time_left;
	}
	raw_spin_unlock(&wrr_rq->wrr_rq_lock);

}

static void set_curr_task_wrr(struct rq *rq)
{
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *p)
{
}

static void task_fork_wrr(struct task_struct *p)
{
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	
	wrr_entity->weight = p->real_parent->wrr.weight;
	wrr_entity->time_slice = wrr_entity->weight * TIME_SLICE;
	wrr_entity->time_left = wrr_entity->time_slice;
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *p)
{
	return p->wrr.weight * TIME_SLICE;
}

static int most_idle_cpu(struct task_struct *p)
{	
	int cpu;
	int idle_cpu = -1;
	int lowest_total_weight = INT_MAX;
	struct rq *rq;
	struct wrr_rq *wrr_rq;

	for_each_online_cpu(cpu) {
		rq = cpu_rq(cpu);
		wrr_rq = &rq->wrr;
		if(cpumask_test_cpu(cpu,tsk_cpus_allowed(p))) {
			if (idle_cpu == -1 || lowest_total_weight > wrr_rq->total_weight) {
				lowest_total_weight = wrr_rq->total_weight;
				idle_cpu = cpu;
			}
		}
	}

	return idle_cpu;
}

static int select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
	int cpu = task_cpu(p);
	int newcpu;
	if(p->nr_cpus_allowed == 1)
		return cpu;

	rcu_read_lock();

	newcpu = most_idle_cpu(p);

	if (newcpu != -1) {
		cpu = newcpu;
	}
	
	rcu_read_unlock();

	return cpu;
}


static void rq_online_wrr(struct rq *rq){}
static void rq_offline_wrr(struct rq *rq){}
static void pre_schedule_wrr(struct rq *rq, struct task_struct *prev){}
static void post_schedule_wrr(struct rq *rq){}
static void task_woken_wrr(struct rq *rq, struct task_struct *p){}
static void switched_from_wrr(struct rq *this_rq, struct task_struct *task){}


static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags) {}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	wrr_entity->weight = DEFAULT_WEIGHT;
	wrr_entity->time_slice = DEFAULT_WEIGHT * TIME_SLICE;
	wrr_entity->time_left = wrr_entity->time_slice;
}
static bool yield_to_task_wrr(struct rq *rq, struct task_struct *p, bool preempt)
{	return true;	}
static void prio_changed_wrr(struct rq *this_rq, struct task_struct *task, int oldprio){}

const struct sched_class wrr_sched_class = 
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
	.rq_online		= rq_online_wrr,
	.rq_offline		= rq_offline_wrr,
	.pre_schedule	= pre_schedule_wrr,
	.post_schedule	= post_schedule_wrr,
	.task_woken		= task_woken_wrr,
	.switched_from	= switched_from_wrr,
#endif
	.set_curr_task 	= set_curr_task_wrr,
	.task_tick 		= task_tick_wrr,
	.task_fork		= task_fork_wrr,
	.switched_to	= switched_to_wrr,
	.prio_changed	= prio_changed_wrr,
	.get_rr_interval= get_rr_interval_wrr,
};

#ifdef CONFIG_SCHED_DEBUG

extern void print_wrr_rq(struct seq_file *m, int cpu, struct wrr_rq *wrr_rq);

void print_wrr_stats(struct seq_file *m, int cpu)
{
	wrr_rq_iter_t iter;
	struct wrr_rq *wrr_rq;

	rcu_read_lock();

	for_each_wrr_rq(wrr_rq, iter, cpu_rq(cpu))
		print_wrr_rq(m, cpu, wrr_rq);

	rcu_read_unlock();
}

#endif // CONFIG_SCHED_DEBUG

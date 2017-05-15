/* 
 * Weighted Round Robin Scheduling Class 
 * (mapped to the SCHED_WRR policy)
 */

const struct sched_class wrr_sched_class;

#include "sched.h"
#include <linux/slab.h>
#define TIME_SLICE 10
#define DEFAULT_WEIGHT 10

#ifdef CONFIG_SMP
static DEFINE_SPINLOCK(LOAD_BALANCE_LOCK);
static unsigned long wrr_next_balance;
void wrr_rq_load_balance(void);
#endif

static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_entity) {
	return container_of(wrr_entity, struct task_struct, wrr);
}

#define for_each_wrr_rq(wrr_rq, iter, rq)	\
	for (iter = container_of(&task_groups, typeof(*iter), list);		\
		(iter = next_task_group(iter)) &&	\
		(wrr_rq = iter->wrr_rq[cpu_of(rq)]);)

extern void init_wrr_rq(struct wrr_rq *wrr_rq)
{
//		struct sched_wrr_entity *wrr_entity;
		wrr_rq->curr = NULL;
		wrr_rq->wrr_total_weight = 0;
		wrr_rq->wrr_nr_running = 0;
		wrr_rq->wrr_size = 0;
		INIT_LIST_HEAD(&wrr_rq->run_queue);
		raw_spin_lock_init(&wrr_rq->wrr_rq_lock);		
}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	struct wrr_rq *wrr_rq = &rq->wrr;
	if (NULL == p) return;
	struct sched_wrr_entity *wrr_entity = &p->wrr;	
	if (NULL == wrr_entity) return;
	struct sched_wrr_entity *head = &wrr_rq->run_queue;

	list_add_tail(&wrr_entity->run_list, &head->run_list);
	/*
	if (wrr_rq->curr == NULL) {
		wrr_rq->curr = p;
		list_add_tail(&wrr_entity->run_list, &wrr_rq->run_queue);
	} else {
		list_add_tail(&wrr_entity->run_list, &(&wrr_rq->curr->wrr)->run_list);
	}
	*/
	
	printk(KERN_DEBUG "ENQUEUE_TASK_INFO cpu number %d task pid %d is enqueued", task_cpu(p), task_pid_nr(current));
	++wrr_rq->wrr_nr_running;
	inc_nr_running(rq);
	++wrr_rq->wrr_size;

	wrr_rq->wrr_total_weight += wrr_entity->weight;
}

static void requeue_task_wrr(struct rq *rq, struct task_struct *p)
{
	if (NULL == p) return;
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	if (NULL == wrr_entity) return;
	struct sched_wrr_entity *head = &wrr_rq->run_queue;

	list_move_tail(&wrr_entity->run_list, &head->run_list);
	//list_move_tail(&wrr_se->run_list, &wrr_rq->run_queue);
	// i am not sure which one is right
	printk(KERN_DEBUG "REQUEUE_TASK_INFO cpu number %d task pid %d is requeued", task_cpu(p), task_pid_nr(current));	
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
	if (NULL == p) return;
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	struct wrr_rq *wrr_rq = &rq->wrr;
	if (NULL == wrr_entity) return;
	struct list_head *curr_next = &(&wrr_entity->run_list)->next;

	list_del_init(&wrr_entity->run_list);

	if(wrr_rq->run_queue.next == &wrr_rq->run_queue)
		wrr_rq->curr = NULL;
	else if (p == wrr_rq->curr){
		if (&wrr_rq->run_queue == curr_next)
			curr_next = curr_next->next;
		wrr_rq->curr = wrr_task_of(list_entry(curr_next, struct sched_wrr_entity, run_list));
	}

	printk(KERN_DEBUG "DEQUEUE_TASK_INFO cpu number %d task pid %d is dequeued", task_cpu(p), task_pid_nr(current));	
	--wrr_rq->wrr_nr_running;
	dec_nr_running(rq);
	--wrr_rq->wrr_size;				

	wrr_rq->wrr_total_weight -= wrr_entity->weight;
}

static void yield_task_wrr(struct rq *rq)
{
	requeue_task_wrr(rq, rq->curr);
}

/*
static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct task_struct *p = NULL;
	struct wrr_rq *wrr_rq = &rq->wrr;
	struct sched_wrr_entity *wrr_entity = NULL;
	struct sched_wrr_entity *current_entity = &rq->curr->wrr;

	if (list_empty(&wrr_rq->run_queue))		return NULL;
	
	wrr_entity = list_first_entry(&wrr_rq->run_queue.run_list, struct sched_wrr_entity, run_list);

	if(wrr_entity->time_slice > 0 && current_entity != wrr_entity) 
		return wrr_task_of(wrr_entity);

	p = wrr_task_of(wrr_entity);
	wrr_entity->time_slice = wrr_entity->weight * TIME_SLICE;
	requeue_task_wrr(rq, p);
	wrr_entity = list_first_entry(&wrr_rq->run_queue.run_list, struct sched_wrr_entity, run_list);
	wrr_entity->time_slice = wrr_entity->weight * TIME_SLICE;
	p = wrr_task_of(wrr_entity);

	return p;
}
*/


static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct task_struct *curr = rq->wrr.curr;
	if (curr == NULL) return NULL;
		curr->wrr.time_slice = curr->wrr.weight * TIME_SLICE;
		return curr;
}
static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
	/*
	if(time_after_eq(jiffies, wrr_next_balance)) {
		if(spin_trylock(&LOAD_BALANCE_LOCK)) {
			if(time_after_eq(jiffies, wrr_next_balance)) {
				wrr_next_balance = jiffies + HZ * 2;
				spin_unlock(&LOAD_BALANCE_LOCK);
				wrr_rq_load_balance();
			}
		}
	}
	*/

	if (NULL == p) return;
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	struct wrr_rq *wrr_rq = &rq->wrr;	
	if (NULL == wrr_entity) return;

	if (--wrr_entity->time_slice) {
printk(KERN_DEBUG "TASK_TICK_INFO cpu number %d task pid %d is ticking %d", task_cpu(p), task_pid_nr(current), wrr_entity->time_slice);	
		return;
	}

	if (wrr_entity->run_list.prev != wrr_entity->run_list.next) {
		printk(KERN_DEBUG "right before requeue in task tick");
		requeue_task_wrr(rq, p);
		set_tsk_need_resched(p);
	} else {
		printk(KERN_DEBUG "requeue: no waiting process in run queue");
		wrr_entity->time_slice = wrr_entity->weight * TIME_SLICE;
	}

}

static void set_curr_task_wrr(struct rq *rq)
{
	struct task_struct *p = rq->curr;
	p->se.exec_start = rq->clock_task;
	// rq->wrr.curr = &p->wrr;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *p){ }

static void task_fork_wrr(struct task_struct *p)
{
	/* the weight has to be the same when forking */
	if (NULL == p) return;
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	if (NULL == wrr_entity) return;

	wrr_entity->time_slice =
			wrr_entity->weight * TIME_SLICE;
}

#ifdef CONFIG_SMP

typedef struct task_group *wrr_rq_iter_t;

static inline struct task_group *next_task_group(struct task_group *tg)
{
	do {
		tg = list_entry_rcu(tg->list.next, typeof(struct task_group), list);
	} while (&tg->list != &task_groups && task_group_is_autogroup(tg));

	if (&tg->list == &task_groups)
		tg = NULL;

	return tg;
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *p)
{
	return p->wrr.weight * TIME_SLICE;
}
/*
static int find_cpu(int flag) {

	int cpu, min_weight, max_weight, res_cpu = -1;
	min_weight = 0;
	max_weight = INT_MAX;

	rcu_read_lock();
	for_each_online_cpu(cpu) {
		struct rq *rq = cpu_rq(cpu);
		struct wrr_rq *wrr_rq = &rq->wrr;

		if(flag == 0) {
			if (wrr_rq->wrr_total_weight >= min_weight) {
				min_weight = wrr_rq->wrr_total_weight;
				res_cpu = cpu;
			}
		}

		else {
			if (wrr_rq->wrr_total_weight < max_weight) {
				max_weight = wrr_rq->wrr_total_weight;
				res_cpu = cpu;
			}
		}
	}
	rcu_read_unlock();

	return res_cpu;
}

struct task_struct* move_available_task(struct rq *rq, int diff) {

	 struct sched_wrr_entity * entity;
	 struct task_struct * selected;
	 struct task_struct * max_weight_task = NULL;
	 struct wrr_rq *wrr_rq = &rq->wrr;
	 int max_weight = 0;

	 raw_spin_lock(&wrr_rq->wrr_rq_lock);
	 list_for_each_entry (entity, &wrr_rq->run_queue.run_list, run_list) {

		selected = wrr_task_of(entity);

	 	if(task_running(rq, selected) == 1)
	 		continue;

	 	if(entity->weight >= diff)
	 		continue;

	 	if(entity->weight >= max_weight) {
	 	    max_weight = entity->weight;
            max_weight_task = selected;
        }
	}
	raw_spin_unlock(&wrr_rq->wrr_rq_lock);

	return max_weight_task;
}


void wrr_rq_load_balance(void) {

	unsigned int RQ_MIN_cpu, RQ_MAX_cpu;
	struct rq *max_rq;
	struct rq *min_rq;
	struct wrr_rq *wrr_max_rq;
	struct wrr_rq *wrr_min_rq;
	struct task_struct *move_task;
	unsigned long flags;
	RQ_MIN_cpu = find_cpu(0);
	RQ_MAX_cpu = find_cpu(1);


	printk("wrr_rq_load_balance is called in cpu[%d]", smp_processor_id());
	

	if(RQ_MIN_cpu == RQ_MAX_cpu || RQ_MIN_cpu == -1 || RQ_MAX_cpu == -1) {
		printk("RQ_MIN_cpu : %d", RQ_MIN_cpu);
		printk("RQ_MAX_cpu : %d", RQ_MAX_cpu);
		return;
	}

	else {
		max_rq = cpu_rq(RQ_MAX_cpu);
		min_rq = cpu_rq(RQ_MIN_cpu);
		wrr_max_rq = &max_rq->wrr;
		wrr_min_rq = &min_rq->wrr;
		int diff = wrr_max_rq->wrr_total_weight - wrr_min_rq->wrr_total_weight;
		printk("max_weight is : %d", wrr_max_rq->wrr_total_weight);
		printk("min_weight is : %d", wrr_min_rq->wrr_total_weight);
		
		move_task = move_available_task(max_rq, diff);
		
		
		if (move_task != NULL) {
			local_irq_save(flags);
			double_rq_lock(max_rq, min_rq);
			deactivate_task(max_rq, move_task, 0);
			set_task_cpu(move_task, RQ_MIN_cpu);
			activate_task(min_rq, move_task, 0);
			double_rq_unlock(max_rq, min_rq);
			local_irq_restore(flags);
		}

	}
}
*/
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
		total_weight = wrr_rq->wrr_total_weight;

		if (lowest_total_weight > total_weight) {
			lowest_total_weight = total_weight;
			idle_cpu = cpu;
		}
	}

	return idle_cpu;
}

static int select_task_rq_wrr(struct task_struct *p, int sd_flag, int flags)
{
	int cpu;

	if (p->nr_cpus_allowed == 1)
		return task_cpu(p);

	rcu_read_lock();
	cpu = most_idle_cpu();
	if (cpu == -1) return task_cpu(p);
	rcu_read_unlock();

	return cpu;
}


static void rq_online_wrr(struct rq *rq){}
static void rq_offline_wrr(struct rq *rq){}
static void pre_schedule_wrr(struct rq *rq, struct task_struct *prev){}
static void post_schedule_wrr(struct rq *rq){}
static void task_woken_wrr(struct rq *rq, struct task_struct *p){}
static void switched_from_wrr(struct rq *this_rq, struct task_struct *task){}

#endif

static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags) {}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
	struct sched_wrr_entity *wrr_entity = &p->wrr;
	wrr_entity->weight = DEFAULT_WEIGHT;
	wrr_entity->time_slice = DEFAULT_WEIGHT * TIME_SLICE;
	
	check_preempt_curr_wrr(rq,p,0);
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

#endif /* CONFIG_SCHED_DEBUG */

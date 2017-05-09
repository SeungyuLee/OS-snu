/* 
 * Weighted Round Robin Scheduling Class 
 * (mapped to the SCHED_WRR policy)
 */

#include "sched.h"
#include <linux/slab.h>

static inline struct task_struct *wrr_task_of(struct sched_wrr_entity *wrr_se)
{
	return container_of(wrr_se, struct task_struct, wrr);
}

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


static struct task_struct *pick_next_task_wrr(struct rq *rq)
{
	struct task_struct *p;
	struct wrr_rq *wrr_rq;
	struct sched_wrr_entity *next = NULL;

	wrr_rq = &rq->wrr;

	if (list_empty(&wrr_rq->queue))
		return NULL;

	next = list_entry(wrr_rq->queue.next, 
			struct sched_wrr_entity, run_list);
	p = wrr_task_of(next);
	return p;
}

const struct sched_class sched_wrr_class = 
{
	.next			= &fair_sched_class,
	.enqueue_task	= enqueue_task_wrr,
	.dequeue_task	= dequeue_task_wrr,
	
#ifdef CONFIG_SMP
	.select_task_rq	= select_task_rq_wrr,
#endif
	.pick_next_task	= pick_next_task_wrr,

};


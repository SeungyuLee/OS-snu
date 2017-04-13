#include <linux/rotation.h>
#include <asm/atomic.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/readwritelock.h>
#include <linux/slab.h>

atomic_t rotation = ATOMIC_INIT(0);

void set_rotation(int degree)
{
	atomic_set(&rotation, degree);
}

int get_rotation(void)
{
	return atomic_read(&rotation);
}


void exit_rotlock(struct task_struct *task)
{
	struct list_head *a;
	struct lock_struct *alock;
	struct lock_struct *n;
	
	spin_lock(&current_list_spinlock);
	list_for_each_safe(a, n, &current_lock_list){
		alock = list_entry(a, struct lock_struct, list);
		if(alock->pid == task->pid){
			list_del(&alock->list);
			list_del(&alock->templist);
			kfree(alock);
		}
	}
	spin_unlock(&current_list_spinlock);


	struct list_head *w;
	struct lock_struct *wlock;
	struct lock_struct *m;

	spin_lock(&waiting_list_spinlock);
	list_for_each_safe(w, m, &waiting_lock_list){
		wlock = list_entry(w, struct lock_struct, list);
		if(wlock->pid == task->pid){
			list_del(&wlock->list);
			list_del(&wlock->templist);
			kfree(wlock);
		}
	}
	spin_unlock(&waiting_list_spinlock);
}


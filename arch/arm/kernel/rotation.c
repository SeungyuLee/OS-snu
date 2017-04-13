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


// DEFINE_SPINLOCK(spinlock);
void exit_rotlock(struct task_struct *task)
{
	// spin_lock(&spinlock);
	
	struct list_head *a;
	struct lock_struct *alock; 
	
	list_for_each(a, &current_lock_list){
		alock = list_entry(a, struct lock_struct, list);
		if(alock->pid == &task->pid){
			list_del(&alock->list);
			list_del(&alock->templist);
			kfree(&alock);
		}
	}

	struct list_head *w;
	struct lock_struct *wlock;

	list_for_each(w, &waiting_lock_list){
		wlock = list_entry(w, struct lock_struct, list);
		if(wlock->pid == &task->pid){
			list_del(&wlock->list);
			list_del(&wlock->templist);
			kfree(&wlock);
		}
	}

	// spin_unlock(&spinlock);
}


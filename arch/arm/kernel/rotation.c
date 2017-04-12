#include <linux/rotation.h>
#include <asm/atomic.h>
#include <linux/list.h>
#include <linux/spinlock.h>

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
	
	list_for_each(a, acquired_lock_list){
		alock = list_entry(a, struct lock_struct, list);
		if(alock->pid == task->pid){
			list_del(alock);
			kfree(alock);
		}
	}

	struct list_head *w;
	struct lock_struct *wlock;

	list_for_each(w, waiting_lock_list){
		wlock = list_entry(w, struct lock_struct, list);
		if(lock->pid == task->pid){
			list_del(lock);
			kfree(lock);
		}
	}

	// spin_unlock(&spinlock);
}

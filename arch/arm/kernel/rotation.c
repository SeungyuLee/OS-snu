#include <linux/rotation.h>
#include <asm/atomic.h>
#include <linux/list.h>
#include <linux/spinlock.h>

atomic_t rotation = ATOMIC_INIT(0);
DEFINE_SPINLOCK(spinlock);

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
	spin_lock(&spinlock);
	
	list_for_each(lock in acquired_lock_list){
		if(lock->pid == task->pid){
			list_del(lock);
			kfree(lock);
		}
	}

	list_for_each(lock in waiting_lock_list){
		if(lock->pid == task->pid){
			list_del(lock);
			kfree(lock);
		}
	}

	spin_unlock(&spinlock);
}

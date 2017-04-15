#include<linux/syscalls.h>
#include<linux/sched.h>
#include<asm/uaccess.h>
#include<linux/slab.h>
#include<linux/rotation.h>
#include<linux/spinlock.h>
#include<linux/spinlock_types.h>
#include<linux/readwritelock.h>

static LIST_HEAD(current_lock_list);
static LIST_HEAD(waiting_lock_list);
static DEFINE_SPINLOCK(current_list_spinlock);
static DEFINE_SPINLOCK(waiting_list_spinlock);

enum LockType {
	kInvalid = 0,
	kRead = 1,
	kWrite = 2
};

bool isInRange(int x, int degree, int range) {
	if (range >= 180) {
		return true;
	}
	x = x % 360;
	int start = (degree - range) % 360;
	int end = (degree + range) % 360;
	if (start < end) {
		if ( x >= start && x <= end ) {
			return true;
		}
	}
	else {
		if ( x >= start || x <= end ) {
			return true;
		}
	}
	return false;
}



bool isCrossed(struct lock_struct *a, struct lock_struct *b) {
	return isInRange(a->degree - a->range, b->degree, b->range) || isInRange(a->degree + a->range, b->degree, b->range);
}

bool canLock(struct lock_struct *info, struct list_head *temp_lock_list) {
	
	if(false == isInRange(get_rotation(),info->degree, info->range)) {
		return false;
	}

	struct list_head *head;
	list_for_each(head,&current_lock_list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct,list);
		if ( isCrossed(info,lock) ) { // 겹치는 lock이 있다
			if (info->type == kWrite || lock->type == kWrite) { // 둘다 read lock이 아닐경우
				return false;
			}
		}
	}
	if(NULL != temp_lock_list) {
		list_for_each(head,temp_lock_list) {
			struct lock_struct *lock = list_entry(head,struct lock_struct,templist);
			if ( isCrossed(info,lock) ) { // 겹치는 lock이 있다
				if (info->type == kWrite || lock->type == kWrite) { // 둘다 read lock이 아닐경우
					return false;
				}
			}
		}
	}
	if (info->type == kRead) {
		list_for_each(head, &waiting_lock_list) {
			struct lock_struct *lock = list_entry(head, struct lock_struct, list);
			if(lock->type == kWrite) {
				if ( isCrossed(info,lock) ) { // 겹치는 write lock이 있다
					if ( isInRange(get_rotation(), lock->degree, lock->range) ) { // 현재degree를 포함한다
						struct list_head *head2;
						list_for_each(head2,&current_lock_list) {
							struct lock_struct *lock2 = list_entry(head2,struct lock_struct,list);
							if ( lock2->type == kRead ) { // acquire된 read lock만 검사
								if( isCrossed(lock,lock2) ) { // 이 writer락(lock)이 read 락에 의해서 pending  상태임을 의미한다
									return false;
								}
							}
						}
					}
				}
			}
		}
	}
	return true;
}

int lockProcess(int degree, int range, int type) {
	struct lock_struct *new_lock;
	new_lock = kmalloc(sizeof(*new_lock), GFP_KERNEL);
	new_lock->degree = degree;
	new_lock->range = range;
	new_lock->type = type;
	new_lock->pid = current->pid;
	INIT_LIST_HEAD(&new_lock->list);
	INIT_LIST_HEAD(&new_lock->templist);
	spin_lock(&waiting_list_spinlock);
	list_add(&new_lock->list,&waiting_lock_list);
	spin_unlock(&waiting_list_spinlock);
	printk(KERN_EMERG "lockProcess lock: %d %d %d %d", degree, range, type, new_lock->pid);
	while(1) {
		spin_lock(&current_list_spinlock);
		spin_lock(&waiting_list_spinlock);
		if(canLock(new_lock,NULL)) {
			printk(KERN_EMERG "lockProcess success: %d %d %d %d", degree, range, type, new_lock->pid);
			list_del(&new_lock->list);
			spin_unlock(&waiting_list_spinlock);
			list_add(&new_lock->list,&current_lock_list);
			spin_unlock(&current_list_spinlock); // problem
			break;
		}else {
			printk(KERN_EMERG "lockProcess failed: %d %d %d %d", degree, range, type, new_lock->pid);
			spin_unlock(&waiting_list_spinlock);
			spin_unlock(&current_list_spinlock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}
	}
	printk(KERN_EMERG "lockProcess return success");
	return 0;
}

int wakeUp(void)
{
	int count = 0;
	LIST_HEAD(temp_lock_list);
    spin_lock(&waiting_list_spinlock);
	struct list_head *head;
    list_for_each(head, &waiting_lock_list) {
        struct lock_struct *lock = list_entry(head,struct lock_struct,list);
		spin_lock(&current_list_spinlock);
		if(canLock(lock,&temp_lock_list)) {
			INIT_LIST_HEAD(&lock->templist);
			list_add(&lock->templist,&temp_lock_list);
            wake_up_process(pid_task(find_vpid(lock->pid),PIDTYPE_PID));
            count += 1;
		}
		spin_unlock(&current_list_spinlock);
    }
    spin_unlock(&waiting_list_spinlock);
	
	struct list_head *n;
	list_for_each_safe(head, n, &temp_lock_list) {
		list_del(head);
	}
	printk(KERN_EMERG "wake up success with count %d", count);
	return count;
}


int deleteProcess(int degree, int range, int type) { // 언락이 불릴 땐 무조건 락이 잡혀있다는 가정하에 작업
	spin_lock(&current_list_spinlock);
	struct list_head *head;
	struct list_head *n;
	int count = 0;
	list_for_each_safe(head, n, &current_lock_list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct, list);
		if(lock->degree == degree && lock->range == range && lock->type == type && lock->pid == current->pid ) {
			list_del(&lock->list);
			kfree(lock);
			count++;
		}
	}
	spin_unlock(&current_list_spinlock); // problem
	wakeUp();
	printk(KERN_EMERG "deleteProcess success %d with %d %d %d", count, degree, range, type);
	return count;
}

asmlinkage int sys_rotlock_read(int degree, int range) {
	if(range < 0) return -EINVAL;
	current->isReadWriteLock = true;
	return lockProcess(degree,range,kRead);
}

asmlinkage int sys_rotlock_write(int degree, int range) {
	if(range < 0) return -EINVAL;
	current->isReadWriteLock = true;
	return lockProcess(degree,range,kWrite);
}

asmlinkage int sys_rotunlock_read(int degree, int range) {
	if(range < 0) return -EINVAL;
	int deleted = deleteProcess(degree, range, kRead);
	if(deleted == 0) return -EINVAL; // error handling when deleted > 1 should be added
	return deleted;
}

asmlinkage int sys_rotunlock_write(int degree, int range) {
	if(range < 0) return -EINVAL;
	int deleted = deleteProcess(degree, range, kWrite);
	if(deleted == 0 ) return -EINVAL; // error handling when deleted > 1 should be added
	return deleted;
}


void exit_rotlock(struct task_struct *task)
{
	if(task->isReadWriteLock == false) return;
	printk(KERN_EMERG "is ReadWriteLock and exiting");
	struct list_head *a;
	struct lock_struct *alock;
	struct list_head *n;
	int count = 0;	

	spin_lock(&current_list_spinlock);
	list_for_each_safe(a, n, &current_lock_list){
		alock = list_entry(a, struct lock_struct, list);
		if(alock->pid == task->pid){
			list_del(&alock->list);
			list_del(&alock->templist);
			kfree(alock);
			count ++;
		}
	}
	spin_unlock(&current_list_spinlock);


	struct list_head *w;
	struct lock_struct *wlock;
	struct list_head *m;

	spin_lock(&waiting_list_spinlock);
	list_for_each_safe(w, m, &waiting_lock_list){
		wlock = list_entry(w, struct lock_struct, list);
		if(wlock->pid == task->pid){
			list_del(&wlock->list);
			list_del(&wlock->templist);
			kfree(wlock);
			count ++;
		}
	}
	spin_unlock(&waiting_list_spinlock);
	if(count > 0) {
		wakeUp();
	}
}

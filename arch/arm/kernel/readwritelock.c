#include<linux/syscalls.h>
#include<linux/sched.h>
#include<asm/uaccess.h>
#include<linux/slab.h>
#include<linux/rotation.h>
#include<linux/spinlock.h>
#include<linux/spinlock_types.h>
#include<linux/readwritelock.h>

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
	printk(KERN_DEBUG "trying lock : %d %d %d",info->degree - info->range,info->degree + info->range,info->type);
	
	if(false == isInRange(get_rotation(),info->degree, info->range)) {
		return false;
	}

	struct list_head *head;
	list_for_each(head,&current_lock_list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct,list);
		printk(KERN_DEBUG "current lock : %d %d %d",lock->degree - lock->range, lock->degree + lock->range,lock->type);
		if ( isCrossed(info,lock) ) { // 겹치는 lock이 있다
			if (info->type == kWrite || lock->type == kWrite) { // 둘다 read lock이 아닐경우
				return false;
			}
		}
	}
	if(NULL != temp_lock_list) {
		list_for_each(head,temp_lock_list) {
			struct lock_struct *lock = list_entry(head,struct lock_struct,templist);
			printk(KERN_DEBUG "current lock : %d %d %d",lock->degree - lock->range, lock->degree + lock->range,lock->type);
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
			printk(KERN_DEBUG "wating write lock : %d %d %d",lock->degree - lock->range, lock->degree + lock->range, lock->type);
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
	new_lock->pid = &current->pid;
	INIT_LIST_HEAD(&new_lock->list);
	spin_lock(&waiting_list_spinlock);
	list_add(&new_lock->list,&waiting_lock_list);
	spin_unlock(&waiting_list_spinlock);
	while(1) {
		spin_lock(&current_list_spinlock);
		spin_lock(&waiting_list_spinlock);
		if(canLock(new_lock,NULL)) {
			list_del(&new_lock->list);
			spin_unlock(&waiting_list_spinlock);
			list_add(&new_lock->list,&current_lock_list);
			spin_unlock(&current_list_spinlock);
			break;
		}else {
			spin_unlock(&current_list_spinlock);
			spin_unlock(&waiting_list_spinlock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}
	}
	return 0;
}

int wakeUp(void)
{
	int count = 0;
	LIST_HEAD(temp_lock_list);
    spin_lock(&current_list_spinlock);
    spin_lock(&waiting_list_spinlock);
	struct list_head *head;
    list_for_each(head,&waiting_lock_list) {
        struct lock_struct *lock = list_entry(head,struct lock_struct,list);
        if(canLock(lock,&temp_lock_list)) {
			INIT_LIST_HEAD(&lock->templist);
			list_add(&lock->templist,&temp_lock_list);
            wake_up_process(pid_task(find_vpid(*lock->pid),PIDTYPE_PID));
            count += 1;
		}
    }
    spin_unlock(&current_list_spinlock);
    spin_unlock(&waiting_list_spinlock);
	list_for_each(head,&temp_lock_list) {
		list_del(head);
	}
	return count;
}


int deleteProcess(int degree, int range, int type) { // 언락이 불릴 땐 무조건 락이 잡혀있다는 가정하에 작업
	spin_lock(&current_list_spinlock);
	struct list_head *head;
	list_for_each(head, &current_lock_list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct, list);
		if(lock->degree == degree && lock->range == range && lock->type == type && lock->pid == &current->pid ) {
			list_del(&lock->list);
			kfree(&lock);
		}
	}
	spin_unlock(&current_list_spinlock);
	wakeUp();
	return 0;
}

asmlinkage int sys_rotlock_read(int degree, int range) {
	if(degree >= 360 || degree < 0) return -EINVAL;
	return lockProcess(degree,range,kRead);
}

asmlinkage int sys_rotlock_write(int degree, int range) {
	if(degree >= 360 || degree < 0) return -EINVAL;
	return lockProcess(degree,range,kWrite);
}

asmlinkage int sys_rotunlock_read(int degree, int range) {
	if(degree >= 360 || degree < 0) return -EINVAL;
	return deleteProcess(degree, range, kRead);
}

asmlinkage int sys_rotunlock_write(int degree, int range) {
	if(degree >= 360 || degree < 0) return -EINVAL;
	return deleteProcess(degree, range, kWrite);
}


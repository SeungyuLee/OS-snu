#include<linux/syscalls.h>
#include<linux/sched.h>
#include<asm/uaccess.h>
#include<linux/slab.h>
#include<linux/rotation.h>

enum LockType {
	kInvalid = 0,
	kRead = 1,
	kWrite = 2
};

struct lock_struct {
	int* degree,range;
	enum lockType *type;
	atomic_t* count;
	pid_t* pid;
	struct list_head list;
};

static LIST_HEAD(current_lock_list);
static spinlock_t current_list_spinlock = SPIN_LOCK_UNLOCKED;
static LIST_HEAD(waiting_lock_list);
static spinlock_t waiting_list_spinlock = SPIN_LOCK_UNLOCKED;

bool isInRange(int current, int degree, int range) {
	if (range >= 180) {
		return true;
	}
	current = current % 360;
	int start = (degree - range) % 360;
	int end = (degree + range) % 360;
	if (start < end) {
		if ( current >= start || current <= end ) {
			return true;
		}
	}
	else {
		if ( current <= start || current >= end ) {
			return true;
		}
	}
}

bool isCrossed(struct lock_struct *a, struct lock_struct *b) {
	return isInRange(a->degree - a->range, b->degree, b->range) || isInRange(a->degree + a->range, b->degree, b->range);
}

bool canLock(struct lock_struct *info) {
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
	if (info->type == kRead) {
		list_for_each(head, &waiting_lock_list) {
			struct lock_struct *lock = list_entry(head, struct lock_struct, list);
			printk(KERN_DEBUG "wating write lock : %d %d %d",lock->degree - lock->range, lock->degree + lock->range, lock->type);
			if(lock->type == kWrite) {
				if ( isCrossed(info,lock) ) { // 겹치는 write lock이 있다
					if ( isInRange(get_rotation(), lock->degree, lock -> range) ) { // 현재degree를 포함한다
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

void insertIntoList(struct list_head *list, int degree, int range, enum LockType type) { 
	list_for_each_entry_safe(head, list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct,list);
		if(lock->degreeStart == degree - range && lock->degreeEnd == degree + range && lock->type == type) { 
			atomic_inc(lock->count);
			return;
		}
	}
	// 락풀고
	struct lock_struct *new_lock;
	new_lock = kmalloc(sizeof(*read_lock), GFP_KERNEL);
	new_lock->degreeStart = degree - range;
	new_lock->degreeEnd = degree + range;
	new_lock->type = &type;
	atomic_set(new_lock->count, 1);
	INIT_LIST_HEAD(&new_lock->list);
	// 락해주고
	list_add(&new_lock,&list);
	// 락풀고
}

int deleteFromList(struct list_head *list, struct lock_struct *lock) {
	// 같은걸 지우지 않도록 락거는 시점이 중요
	// 락해주고
	list_for_each_entry_safe(head, list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct,list);
		if(lock->degreeStart == degree - range && lock->degreeEnd == degree + range, lock->type == type) {
			// 위와 마찬가지 이유로 count 락 걸지 않음
			if(atomic_read(lock->count) > 1) {
				atomic_dec(lock->count);
			}else {
				list_del(&lock->list);
				kfree(lock);
			}
			// 락풀고
			return 0;
		}
	}
	// 락풀고
	return -1;
}

int lockProcess(int degree, int range, enum LockType type) {
	struct lock_struct *new_lock;
	new_lock = kmalloc(sizeof(*new_lock), GFP_KERNEL);
	new_lock->degree = degree;
	new_lock->range = range;
	new_lock->type = type;
	INIT_LIST_HEAD(&new_lock->list);
	spin_lock(&waiting_list_spinlock);
	list_add(&new_lock,&waiting_lock_list);
	spin_unlock(&waiting_list_spinlock);
	while(1) {
		spin_lock(&current_list_spinlock);
		spin_lock(&waiting_list_spinlock);
		if(canLock(new_lock)) {
			deletefromList(waiting_lock_list,new_lock);
			spin_unlock(&waiting_list_spinlock);
			list_add(&new_lock,&current_lock_list);
			spin_unlock(&current_list_spinlock);
		}else {
			spin_unlock(&current_list_spinlock);
			spin_unlock(&waiting_list_spinlock);
			set_current_state(TASK_INTERRUPTIBLE);
			schedule();
		}
	}
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
	return deleteFromList(current_lock_list, degree, range, kRead);
}

asmlinkage int sys_rotunlock_write(int degree, int range) {
	if(degree >= 360 || degree < 0) return -EINVAL;
	return deleteFromList(current_lock_list, degree, range, kWrite);
}

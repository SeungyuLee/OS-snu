#include<linux/syscalls.h>
#include<linux/sched.h>
#include<asm/uaccess.h>
#include<linux/slab.h>

enum LockType {
	kInvalid = 0;
	kRead = 1;
	kWrite = 2;
}

struct lock_struct {
	int* degreeStart, degreeEnd;
	enum lockType *type;
	int* count;
	struct list_head list;
}

static LIST_HEAD(current_lock_list);
static LIST_HEAD(waiting_lock_list);

bool canLock(struct lock_struct *info) {
	printk(KERN_EMEG "trying lock : %d %d %d",info->degreeStart,info->degreeEnd,info->type);
	// 현재 degree와 비교 작업 먼저
	list_for_each(head, current_lock_list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct,list);
		printk(KERN_EMEG "current lock : %d %d %d",lock->degreeStart, lock->degreeEnd,lock->type);
		if ((lock->degreeStart <= info->degreeStart && lock->degreeEnd >= info->degreeStart) ||
				(lock->degreeStart <= info->degreeEnd && lock->degreeEnd >= info->degreeEnd)) {
			if (info->type == kWrite || lock->type == kWrite) {
				return false;
			}
		}
	}
	if (info.type == kRead) {
		list_for_each(head, waiting_lock_list) {
			struct lock_struct *lock = list_entry(head,struct lock_struct,list);
			printk(KERN_EMEG "wating write lock : %d %d %d",lock->degreeStart, lock->degreeEnd, lock->type);
			if(lock->type == kWrite) {
				if ((lock->degreeStart <= info->degreeStart && lock->degreeEnd >= info->degreeStart) ||
						(lock->degreeStart <= info->degreeEnd && lock->degreeEnd >= info->degreeEnd)) {
					// 위에 if에 lock이 degree를 포함하고 있는지도 검사
					return false;
				}
			}
		}
	}
	return true;
	// 동시에 write 두개가 can_lock 판정을 받으면 어떡하지? 어떻게 막지? (락거는 시점으로 조절할 수 있을듯)
	// 시작하면서 락 걸고 끝날때 풀어주는 방식이 좋을 것 같음 (지금 짜는 read, write 와 같은 의미)
}

void insertIntoList(struct list_head *list, int degree, int range, enum LockType type) {
	// 락해주고
	list_for_each(head, list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct,list);
		if(lock->degreeStart == degree - range && lock->degreeEnd == degree + range && lock->type == type) { 
			lock->count += 1; // count 락을 따로 해줘야 할 수도 있음, 근데 위에 락에 안걸리고 여기로 오는 경우가 생각이 안남
			//락풀고
			return;
		}
	}
	// 락풀고
	struct lock_struct *new_lock;
	new_lock = kmalloc(sizeof(*read_lock), GFP_KERNEL);
	new_lock->degreeStart = dgree - range;
	new_lock->degreeEnd = degree + range;
	new_lock->type = type;
	new_lock->count = 1;
	INIT_LIST_HEAD(&new_lock->list);
	// 락해주고
	list_add(&new_lock,&list);
	// 락풀고
}

int deleteFromList(struct list_head *list, int degree, int range, enum LockType type) {
	// 같은걸 지우지 않도록 락거는 시점이 중요
	// 락해주고
	list_for_each(head, list) {
		struct lock_struct *lock = list_entry(head,struct lock_struct,list);
		if(lock->degreeStart == degree - range && lock->degreeEnd == degree + range, lock->type == type) {
			// 위와 마찬가지 이유로 count 락 걸지 않음
			if(lock->count > 1) {
				lock->count -= 1;
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
	insertIntoList(wait_lock_list, degree, range, type);
	// canLock이 가능해 질 때 까지 process 재우기
	deleteFromList(wait_lock_list, degree, range, type);
	
	insertIntoList(current_lock_list, degree, range, type);
	return 0;
}

asmlinkage int sys_rotlock_read(int degree, int range) {
	return lockProcess(degree,range,kRead);
}

asmlinkage int sys_rotlock_write(int drgree, int range) {
	return lockProcess(degree,range,kWrite);
}

asmlinkage int sys_rotunlock_read(int degree, int range) {
	return deleteFromList(current_lock_list, degree, range, kRead);
}

asmlinkage int sys_rotunlock_write(int degree, int range) {
	return deleteFromList(current_lock_list, degree, range, kWrite);
}

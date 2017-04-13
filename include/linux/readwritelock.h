#ifndef __READWRITELOCK_H__
#define __READWRITELOCK_H__

#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>

enum LockType;

struct lock_struct {
	int degree,range;
	int type;
	pid_t* pid;
	struct list_head list,templist;
};


static LIST_HEAD(current_lock_list);
static LIST_HEAD(waiting_lock_list);

bool isInRange(int x, int degree, int range);
bool isCrossed(struct lock_struct *a, struct lock_struct *b);

bool canLock(struct lock_struct *info, struct list_head *temp_lock_list);
int lockProcess(int degree, int range, int type);
int wakeUp(void);
int deleteProcess(int degree, int range, int type);

asmlinkage int sys_rotlock_read(int degree, int range);
asmlinkage int sys_rotlock_write(int degree, int range);
asmlinkage int sys_rotunlcok_read(int degree, int range);
asmlinkage int sys_rotunlock_write(int degree, int range);

#endif

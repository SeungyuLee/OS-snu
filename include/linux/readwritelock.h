#include <linux/sched.h>
#include <linux/list.h>
#include <linux/spinlock.h>

enum LockType;

struct lock_struct;

static LIST_HEAD(current_lock_list);
static spinlock_t current_list_spinlock;
static LIST_HEAD(waiting_lock_list);
static spinlock_t waiting_list_spinlock;

bool isInRange(int current, int degree, int range);
bool isCrossed(struct lock_struct *a, struct lock_struct *b);

bool canLock(struct lock_struct *info);
int lockProcess(int degree, int range, enum LockType type);
int deleteProcess(int degree, int range, int type);

asmlinkage int sys_rotlock_read(int degree, int range);
asmlinkage int sys_rotlock_write(int degree, int range);
asmlinkage int sys_rotunlcok_read(int degree, int range);
asmlinkage int sys_rotunlock_write(int degree, int range);

void wakeUp(void);

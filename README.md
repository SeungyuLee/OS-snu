Project 1
===================

## Summary 

The goal of this assignment is implementing a new kernel syncronization primitive which provides reader-writer lock based on device rotation in Linux. First, we wrote new system call to use user-space daemon given from TA. Second, we implemented rotation-based reader-writer locks. Finally, we wrote Selector and Trial for testing.


## Impementation

### 1. set_rotation System Call


```
...
atomic_rotation = ATOMIC_INIT(0);
...
void set_rotation(int degree)
{
	atomic_set(&rotation, degree);
}
...
```

set_rotation() function is implemented in rotation.c . It is called In set_rotation System call to set degree of device to given value. 



### 2. Reader-Writer Lock

#### Approach

First, The policies of determining whether lock is possible are as follows.

	* Policy

        1. If the current degree of device is not in the range of requested lock, lock is not possible.
        2. If requested lock is Reader lock
                2-1. If there is Writer lock that overlaps range of requested lock among acqiured locks, lock is not possible.
                2-2. If there is Writer lock that overlaps range of requested lockand waits (sleep) by Reader lock among pendding locks, lock is not possible.
        3. If requested lock is Writer lock
                3-1. If there is Reader/Writer lock that overlaps range of requested lock among acquired locks, lock is not possible.

First, according to the given rule of rotation_based lock, if the current degree of device is not included in the range of requested lock, Reader/Writer can't grab the lock. (1) If the current degree of device is included in the range of requested lock, Let's look first if the requested lock is a Reader lock. If a lock that overlaps the range of requested lock is currently acquired, Reader can grab a lock if that lock is Reader lock. However, this is not possible if that lock is Writer lock. (2-1) Also, when there is no lock that overlaps the range of requested lock among acquired locks Reader can basically grab a lock. But if a Writer lock that overlaps the range of requested lock is pending by Reader lock, Reader can't grab a lock. (2-2) This is to prevent Writer starvation. Finally, If the requested lock is a Writer lock, Writer can not grab a lock if a lock that overlaps the range of the requested lock is currently acquired. (3-1)

Using this policy, we determined whether Reader/Writer can grab a requested lock. Also we made a **current_lock_list** and **waiting_lock_list** because there may be several grabbed locks at the same time since their range doesn't overlap each other and there may be several pending locks at the same time waiting to satisfy the conditions. Depending on the lock availability, the requested lock is stored in each list. 

Whenever a lock is requested, the process to check the lock condition is executed in the loop. If the condition is satisfied, the requested lock is stored in the **current_lock_list** and returns. That is grabbing a lock. On the other hand, If the condition is not satisfied, the requested lock is inserted into the **wait_lock_list** and the process requesting the lock becomes TASK_INTERRUBTIBLE state.

When an unlock request comes in, it removes the requested lock from the **current_lock_list** and wakes up all the processes that requested a lock that meets the condition in the **wait_lock_list**. This is releasing the lock.

The waking process loops again through the previous lock request system call function.

If multiple process try to access **curren_lock_list** and **wait_lock_list** at the same time, _synchronization_ problems may occur. Therefore, we lock the process each time process access each list using **spinlock**. In order to keep the concept of locking the data not code, we made separate spinlock for **current_lock_list** and spinlock for **wait_lock_list** respectively.


#### Implementation

Lock/Unlock Systemcall for Reader/Writer is implemented in  `arch/arm/kernel/readwritelock.c`. 

```
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
```

Both Reader and Writer call lock systemcall, `lockProcess()` is called. 

```
int lockProcess(int degree, int range, int type);
```

Inside `lockProcess()`, a new `struct lock_struct` that contains the attributes of the requested lock is created. The `struct lock_struct`is declared in readwritelock.h and detailed structure is as follows.  :

```
struct lock_struct {
	int* degree, range;
	int *type; /* lock type */
	pid_t* pid; /* Pid of task_struct of process that request to grab a lock*/
	struct list_head list, templist; /* list_head for  current_lock_list and wait_lock _list */
}
```

`struct list_head list`of `struct lock_struct` is for `current_lock_list` that contains currently grabbed locks and `waiting_lock_list` that contains pending locks. Those list is declared n readwritelock.h like the following: 

```
static LIST_HEAD(current_lock_list);
static LIST_HEAD(waiting_lock_list);
static DEFINE_SPINLOCK(current_list_spinlock);
static DEFINE_SPINLOCK(waiting_list_spinlock);
```
 
Spinlock is also declared to hold the lock for each list. These spinlocks are used in `lockProcess()` to prevent other processes from accessing lists each time locks are insertedand removed from the lists.

The requested lock is determined to be possible to grabbed by the canLock() function. 


```
bool canLock(struct lock_struct *info, struct list_head *temp_lock_list);
```

And than requested lock is inserted into the current_lock_list if lock is possible, and if not, the state of the process is changed and the process enters the sleep state like the following :

```
...
set_current_state(TASK_INTERRUPTIBLE);
schedule();
...
```


When both Reader and Writer call unlock systemcall, `deleteProcess()` is called. 

```
int deleteProcess(int degree, int range, int type)
```

In `deleteProcess()`, the lock requested to be unlocked in the current_lock_list is located using the `list_for_each_safe()`function and deleted. And `wakeUp()`function is called to wake up the process that requested the lock in the wating_lock_list.

```
int wakeUp(void)
```

In `wakeUp()`, A process that requested to grab a lock which is in waiting_lock_list and  determined to be lockable through the canLock() function is awakend through wake_up_process(). We used pid_task() to find the `struct task_struct` to pass as a parameter.

```
…
if(canLock(lock,&temp_lock_list)) {
	INIT_LIST_HEAD(&lock->templist);
	list_add(&lock->templist,&temp_lock_list);
	wake_up_process(pid_task(find_vpid(lock->pid),PIDTYPE_PID));
…
```

### 3. Test Program

* selector.c

* trial.c

## Lessons

## Team Members

* Park Yoonjong
* Lee Seunggyu
* Lee Nayoung

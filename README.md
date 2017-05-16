

Project 3
===================

## Summary 

The goal of this assignment is building our own CPU scheduler to support weighted round-robin scheduling. we added new scheduling policy `WRR` to Linux kernel and implemented functions for scheduling and load balancing. Then we implemented test program using naive Trial Division method. 


## Implementation


### 1. High-level design 

1. `wrr.c`

	- enqueue_task_wrr : put new task in wrr_rq and arrange it by list_add_tail.
	- dequeue_task_wrr : delete task from wrr_rq
	- yield_task_wrr : call when task is yield. move task to the end of list when requeue is called.
	- pick_next_task_wrr : pick next task for CPU. pick front one in list after requeue.
	- task_tick_wrr : reduce time_slice every cycle. reschedule task when time_slice is 0.
	- task_fork_wrr : call when task is forked. set value of time_slice.

2. `sched.h`

	- sched_wrr_entity : information of entity. similar with rt but it has weight.
	- wrr_rq : struct in rq containing information of wrr_rq.
	
	```
	struct wrr_rq {
		unsigned int total_weight; 	// sum of weight of tasks in rq
		unsigned int nr_running 	// the number of tasks which is running now
		raw_spinlock_t wrr_rq_lock; 	// spinlock for wrr_rq
		struct task_struct *curr;	// task on CPU (NULL when there is nothing to run)
		struct list_head run_queue;	// list_head for queueing tasks
	}
	```

3.  System call in `kernel/sched.c`
	
	
	- set weight systemcall
		
		```
		int sched_setweight(pid_t pid, int weight);
		```
		
	: change weight value of process. system call number is 380. change calling process's weight when pid = 0. Only a user who is a root user or has a process should be able to change the weight using `sched_setweight` system call.
	
	- get weight systemcall
		
		```
		int sched_getweight(pid_t pid);
		```
		
	: return weight value of process. system call number is 381. return calling process's weight when pid = 0. Any user can call `sched_getweight` system call.
	
	
4. test program in`test/trial.c`
		
	- make 20 child process using `fork()` to measure execution time of 1~20 weight on parent process.
	- set weight using `sched_setweight` systemcall, check using `sched_getweight` systemcall.
	- use `gettimeofday()` to get execution time.
	- prime factorization using _Trial Division_ method which we used in Project 2.



### 2. Condition of `WRR`

1. defult scheduler change
	- wrr should be default scheduler for systemd and those descendants. 
		- modify **`include/linux/init_task.h`** to operate alongside the existing Linux scheduler. The new policy's name is `SCHED_WRR` and value is 6. This is default class for `systemd`.   
		- only task with policy set by `SCHED_WRR` is scheduled by new scheduler.
		- Tasks using the SCHED_WRR policy should take priority over tasks using the SCHED_NORMAL policy, but not over tasks using the SCHED_RR or SCHED_FIFO policies of real time scheduler
		- when change policy to `SCHED_WRR`  weight should be default value.
	- should be capable on both multiprocessor and uniprocessor
		- we need proper synchronization and rock.  we referenced existing kernel scheduler rock
	- weight : 1~ 20 / time quantom of each job : weight * 10ms
	- weight and `SCHED_WRR` flag of task should be inherited by forking task.
	- When change weight of task, change is applied after task is ended.
	- New job should be allocated to CPU with smallest total weight.
	- Load balancing every 2000ms  


### 3. Load Balancing 

- work only when two or more CPUs are activated.

- Algorithm

	1. scheduler_tick() in `kernel/sched/core.c`.

	2. At each tick, balance_lock is grabbed and cpu check balance_timestamp global variable to check if it is time for load balancing. If it is, the cpu updates the timestamp. Then balance_lock is released and load balancing is started.

	3. Find the MAX and MIN RQ. Afterwards, the locks for the two runqueues are acquired, and the function tries to find a migratable task with maximum weight.

	4. If there exists a migratable task, the function migrates the task to MIN RQ and unlocks those two locks.


### 4. How to build and run

- Comile

```
~/linux-3.10-artik$ arm-linux-gnueabi-gcc -I/include test/factor.c -o factor
```

- Push the test program

```
~/linux-3.10-artik$ push factor /root/factor
```

- Execute (via Artik)

```
root:~> ./factor 6523257244322 (large number taking a lot of factorization time)
```

### 5. Result




## Lessons

1. We found the difficulty of implementing scheduler. If the scheduler does not work properly, we can't even know what went wrong. So We have to go through a lot of trial and error.
2. We now have a better understanding of how the various files interact. We have come to know how core.c and rt.c, fair.c are connected, and overall We have a deep understanding of how the scheduler is implemented and working.


## Team Members

* Park Yoonjong
* Lee Seunggyu
* Lee Nayoung

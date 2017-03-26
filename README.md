
Project 1
===================
## Summary

The goal of this project os implementing a new System Call `ptree` in Linux. It return the process tree information in depth-first-search order.

## Contents

### 1. Add System Call **`ptree`**

added codes to be applicable in `arm` environment.

* Add **`prinfo.h`** in include/linux

	defined `struct prinfo`.

* Add **`ptree.c`** in arch/arm/kernel

	prototype for System Call `sys_ptree`:
	
	```
	asmlikage int sys_ptree(struct prinfo *buf, int *nr);
	```

* Modify **`Makefile`** in arch/arm/kernel

	added `ptree.o`.
	
* Modify **`unistd.h`** in arch/arm/include/asm

	edited the following code:
	
	```
	__NR_syscalls (380=> 384)
	```
	changed the maximum number of System Call from **380** to **384**.
	
* Modify **`unistd.h`** in arch/arm/include/uapi/asm

	added the following code:
	
	```
	#define __NR_ptree			(__NR_SYSCALL_BASE+380)
	```
	
* Modify **`calls.s`** in arch/arm/kernel

	added the following code:
	
	```
	/* 380 */ 		CALL(sys_ptree)
	```

* Modify **`syscalls.h`** in include/linux/syscalls.h

	```
	asmlinkage int sys_ptree(struct prinfo *buf, int *nr);
	```
	

### 2. Detail of implementation

*  **tasklist_lock**

	When traversing the process tree data structures, it is necessary to prevent the data structures from changing in order to ensure consistency. Therefore we grabbed `tasklist_lock` before we begin the traversal, and released the lock when traversal is completed like following codes:

	```
	read_lock(&tasklist_lock);
	do_dfsearch(&init_task, kbuf, &k_nr);  /* function to traverse the process tree data) */
	read_unlock(&tasklist_lock);
	```
	
	
	Since the `copy_from_user` / `copy_to_user` function can not be called many times when the `tacklist_ lock` is held, it is necessary to put the data in a pre-allocated pointer all at once. So we allocated `prinfo` array of size `nr` and `k_nr` in advance like following code:

	```
	struct prinfo *k_buf = kcalloc(*nr, sizeof(struct prinfo), GFP_KERNEL);
	int k_nr = 0;
	```
* **task_struct**

	All the data about the processes we need is inside the `task_struct` structure defined in `include/linux/sched.h`. Also, process tree search is possible through the `children` and `sibling` variables in the form of `list_head` structure in `task_struct` like following code:

	```
	struct list_head children;	/* list of my children */
	struct list_head sibling;	/* linkage in my parent's children list */
	struct task_struct *group_leader;	/* threadgroup leader */
	```
	
	 we can get the root `task_struct` of the tree with the predefined `init_task` variable. 
	
* **DFS**

	DFS was implemented in the function `do_dfsearch`.

	```
	void do_dfsearch(struct task_struct *task, struct prinfo *buf, int *nr)
	```
	
	First, We checked that the task being examined is a process, not a thread. using the is_process function.
	
	```
	bool is_process(struct task_struct *task)
	```

	 If the thread_group is empty or it is thread_group_leader, it is confirmed to be process. And when it is confirmed as a process, The `process_node` function which puts the information of `task_struct` into `prinfo`.

	```
	void process_node(struct prinfo *buf, struct task_struct *task)
	```
	
	when the predefined global variable `process_count` is smaller than `nr`, the number of processes we need. After than, `process_count` is increased by 1.

	
	```
	struct list_head *child = NULL;
	list_for_each(child, &task->children) {
		do_dfsearch(list_entry(child,struct task_struct,sibling),buf,nr);
	}
	```
	
	// list_for_each / list_entry description //

### 3. Test program


## Lessons

## Team Members
* Park yoonjong
* Lee Nayoung
* Lee Seunggyu

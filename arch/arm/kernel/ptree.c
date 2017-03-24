#include <linux/syscalls.h>
#include <linux/sched.h>
#include <asm/uaccess.h> // for usage of copy_from_user and copy_to_user 
#include <asm-generic/errno-base.h> // for usage of -EFAULT
#include <linux/syscalls.h>
#include <linux/prinfo.h>
#include <linux/slab.h> // for usage of kcalloc and kfree

asmlinkage int sys_ptree(struct prinfo *buf, int *nr)
{
	int result;
	int do_dfsearch(struct prinfo *b, int *n);

	struct prinfo *k_buf = kcalloc(*nr, sizeof(struct prinfo), GFP_KERNEL);
	int *k_nr = kcalloc(1, sizeof(int), GFP_KERNEL);

	if(copy_from_user(k_buf, buf, (*nr)*sizeof(struct prinfo))!=0)
		return -EFAULT;
	if(get_user(k_nr, nr)!=0)
		return -EFAULT;

	read_lock(&tasklist_lock);
	do_dfsearch(&init_task,k_buf, k_nr);
	read_unlock(&tasklist_lock);

	if(copy_to_user(buf, k_buf, (*nr)*sizeof(struct prinfo))!=0)
		return -EFAULT;
	if(put_user(nr, k_nr)!=0)
		return -EFAULT;
	
	kfree(k_buf);
	kfree(k_nr);
	
	printk(KERN_EMERG "HelloWorld!\n");
	return 0;
}

int process_count = 0;
int buf_idx = 0;

void process_node(struct prinfo *buf, struct task_struct *task) {
	
}

void do_dfsearch(struct task_struct *task, struct prinfo *buf, int *nr){
	if(NULL == task)
		return;
	if(is_process(task) || 0 == task->pid) {
		printk("%d task is process",task->pid);
		if (process_count < nr) {
			process_node(buf,task);
		}
		process_count += 1;
	}
	struct list_head *children = &task->children;
	if(false == list_empty(children)) {
		do_dfsearch(list_entry(children.next,struct task_struct,sibling));
	}
	struct list_head *head = &task->parent->children;
	if(false == list_is_last(task->sibling,head)) {
		do_dfsearch(ist_entry(&task->sibling,struct task_struct,sibling));
	}

}

bool is_process(struct task_struct *task) {
	if (thread_group_empty(task)) return true;
	else {
		if(thread_group_leader(task))
			return true;
		else
			return false;
	}
}

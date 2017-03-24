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
	void do_dfsearch(struct task_struct *t, struct prinfo *b, int *n);

	printk(KERN_EMERG "HelloWorld!\n");
	struct prinfo *k_buf = kcalloc(*nr, sizeof(struct prinfo), GFP_KERNEL);
	int k_nr = 0;

	if(copy_from_user(k_buf, buf, (*nr)*sizeof(struct prinfo))!=0)
		return -EFAULT;
	if(copy_from_user(&k_nr, nr, sizeof(int))!=0)
		return -EFAULT;

	read_lock(&tasklist_lock);
	do_dfsearch(&init_task,k_buf, &k_nr);
	read_unlock(&tasklist_lock);

	if(copy_to_user(buf, k_buf, (*nr)*sizeof(struct prinfo))!=0)
		return -EFAULT;
	if(copy_to_user(nr, &k_nr, sizeof(int))!=0)
		return -EFAULT;
	
	kfree(k_buf);
	
	printk(KERN_EMERG "ByeWorld!\n");
	return 0;
}

int process_count = 0;
int buf_idx = 0;

bool has_child(struct task_struct *task) {
	return !list_empty(&task->children);
}
	 
bool has_sibling(struct task_struct *task) {
	struct list_head *head = &task->parent->children;
	return !list_is_last(&task->sibling,head);
}

void process_node(struct prinfo *buf, struct task_struct *task) {
	printk(KERN_EMERG "start processing\n");
	struct prinfo newPrinfo;
	newPrinfo.state = task->state;
	printk(KERN_EMERG "state ok\n");
	newPrinfo.pid = task->pid;
	printk(KERN_EMERG "pid ok\n");
	newPrinfo.parent_pid = task->parent->pid;
	printk(KERN_EMERG "parent pid ok\n");
	newPrinfo.first_child_pid = 0;
	newPrinfo.next_sibling_pid = 0;
	newPrinfo.uid = task_uid(task);
	printk(KERN_EMERG "uid ok\n");
	strncpy(newPrinfo.comm, task->comm, 60);
	printk(KERN_EMERG "comm ok\n");
	if (has_child(task)) {
		struct task_struct *first_child = list_entry(task->children.next, struct task_struct, sibling);
		newPrinfo.first_child_pid = first_child->pid;
	}
	printk(KERN_EMERG "first_child ok\n");
	if(has_sibling(task)) {
		struct task_struct *next_sibling = list_entry(task->sibling.next, struct task_struct, sibling);
		newPrinfo.next_sibling_pid = next_sibling->pid;
	}
	printk(KERN_EMERG "sibling ok\n");
	buf[process_count] = newPrinfo;
}

void do_dfsearch(struct task_struct *task, struct prinfo *buf, int *nr){
	bool is_process(struct task_struct *t);

	if(NULL == task)
		return;
	printk(KERN_EMERG "task pid = %d",task->pid);
	if(is_process(task) && 0 != task->pid) {
		printk(KERN_EMERG "%d task is process\n",task->pid);
		if (process_count < *nr) {
			process_node(buf,task);
		}
		process_count += 1;
	}
	if(has_child(task)) {
		do_dfsearch(list_entry(task->children.next,struct task_struct,sibling),buf,nr);
	}
	if(has_sibling(task)) {
		do_dfsearch(list_entry(task->sibling.next,struct task_struct,sibling),buf,nr);
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

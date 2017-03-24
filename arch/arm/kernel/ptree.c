#include <linux/linkage.h>
#include <linux/kernel.h>
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
	do_dfsearch(k_buf, k_nr);
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

int do_dfsearch(struct prinfo *buf, int *nr){

	return 0;
}

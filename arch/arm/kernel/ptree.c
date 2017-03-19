#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage int sys_ptree(struct prinfo *buf, int *nr)
{
	printk(KERN_EMERG "HELLOWORLD\n");
	return 1;
}

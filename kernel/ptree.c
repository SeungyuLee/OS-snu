#include <linux/linkage.h>
#include <linux/kernel.h>

asmlinkage int sys_ptree(void)
{
	printk(KERN_EMERG "HELLOWORLD\n");
	return 1;
}

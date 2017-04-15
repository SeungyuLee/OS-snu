#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/rotation.h>
#include <linux/readwritelock.h>

asmlinkage int sys_set_rotation(int degree)
{
	if(degree >= 360 || degree < 0) return -EINVAL; 
	set_rotation(degree);
	int count = wakeUp();
	return count;
}

#include <linux/syscalls.h>
#include <linux/unistd.h>
#include <linux/kernel.h>
#include <linux/rotation.h>

asmlinkage int sys_set_rotation(int degree)
{
	set_rotation(degree);
	printk("[sys_set_rotation] Set Rotation: %d\n", get_rotation());
	return 0;
}

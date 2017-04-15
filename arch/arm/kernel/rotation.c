#include <linux/rotation.h>
#include <asm/atomic.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/readwritelock.h>
#include <linux/slab.h>
#include <linux/spinlock_types.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/syscalls.h>

atomic_t rotation = ATOMIC_INIT(0);

void set_rotation(int degree)
{
	atomic_set(&rotation, degree);
}

int get_rotation(void)
{
	return atomic_read(&rotation);
}




#include <linux/rotation.h>
#include <asm/atomic.h>

atomic_t rotation = ATOMIC_INIT(0);

void set_rotation(int degree)
{
	atomic_set(&rotation, degree);
}

int get_rotation(void)
{
	return atomic_read(&rotation);
}

void exit_rotlock(void)
{
	return;
}

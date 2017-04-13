#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#define SYSCALL_ROTLOCK_WRITE 382
#define SYSCALL_ROTUNLOCK_WRITE 385

int main(int argc, char* argv[])
{
	int n;
	int arg1 = atoi(argv[1]);
	int arg2 = atoi(argv[2]);
	syscall(SYSCALL_ROTLOCK_WRITE, arg1, arg2);
	printf("rotlock_write from %d to %d\n", arg1-arg2, arg1+arg2);
	scanf("%d", &n);
	syscall(SYSCALL_ROTUNLOCK_WRITE, arg1, arg2);
	printf("rotunlock_write from %d to %d\n", arg1- arg2, arg1+arg2);
	return 0;
}

#include <sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#define SYSCALL_SET_ROTATION 380

int main(int argc, char* argv[])
{
	syscall(380, atoi(argv[1]));
	printf("set rotation: %s\n", argv[1]);
	return 0;
}

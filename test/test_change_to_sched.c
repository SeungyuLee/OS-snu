#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <sched.h>

#define SYSCALL_GET_SCHEDULER 157
#define SYSCALL_SET_SCHEDULER 156
#define SYSCALL_SCHED_SETWEIGHT 380
#define SYSCALL_SCHED_GETWEIGHT 381

int main(int argc, char* argv[]) {

	struct sched_param param;
	pid_t pid;
	int to;
	param.sched_priority = 0;

	pid = atoi(argv[1]);
	to = atoi(argv[2]);
	printf("return value: %ld", syscall(SYSCALL_SET_SCHEDULER, pid, to, &param));
	
	printf("change completed");

	return 0;
}

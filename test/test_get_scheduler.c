#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/sched.h>

#define SYSCALL_GET_SCHEDULER 157
#define SYSCALL_SET_SCHEDULER 156
#define SYSCALL_SCHED_SETWEIGHT 380
#define SYSCALL_SCHED_GETWEIGHT 381

int main(int argc, char* argv[]) {

	pid_t pid;
	
	pid = atoi(argv[1]);
	printf("pid : %d scheduler : %ld", pid ,syscall(SYSCALL_GET_SCHEDULER,pid));

	return 0;
}

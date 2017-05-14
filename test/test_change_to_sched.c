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
	int to;

	pid = atoi(argv[0]);
	to = atoi(argv[1]);
	syscall(SYSCALL_SET_SCHEDULER,pid,to);
	
	print("change completed");

	return 0;
}

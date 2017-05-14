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
	int child = fork();
	if(child == getpid()) {
		printf("child : %d\n",getpid());
		long int value1 =  syscall(SYSCALL_SET_SCHEDULER, getpid(), 6,&param);
		long int value2 = syscall(SYSCALL_SCHED_SETWEIGHT, getpid(), 20);
		printf("child scheduler: %ld, sched_setweight: %ld\n", value1, value2);
		int i;
		for(i=1; ; i++ ) {
			printf("child %d",i);
		}
	}else {
		printf("parent : %d\n".getpid());
		long int value1 =  syscall(SYSCALL_SET_SCHEDULER, getpid(), 6,&param);
		long int value2 = syscall(SYSCALL_SCHED_SETWEIGHT, getpid(), 1);
		printf("parent scheduler: %ld, sched_setweight: %ld\n", value1, value2);
		for(i=1 ;; i++) {
			printf("parent %d",i);
		}
	}
	return 0;
}

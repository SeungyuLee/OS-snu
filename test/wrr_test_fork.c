#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <linux/sched.h>
#include <time.h>
#include <sched.h>

#define SYSCALL_GET_SCHEDULER 157
#define SYSCALL_SET_SCHEDULER 156
#define SYSCALL_SCHED_SETWEIGHT 380
#define SYSCALL_SCHED_GETWEIGHT 381

int main(int argc, char* argv[]) {

	int weight = atoi(argv[1]);
	struct sched_param param;
	param.sched_priority = 0;
	long int value1 =  syscall(SYSCALL_SET_SCHEDULER, getpid(), 6,&param);
	long int value2 = syscall(SYSCALL_SCHED_SETWEIGHT, getpid(), weight);
	printf("set scheduler: %ld, sched_setweight: %ld\n", value1, value2);
	fork();
	fork();
	fork();
	printf("process number %d has weight of %ld\n", getpid(), syscall(SYSCALL_SCHED_GETWEIGHT, getpid()));
	clock_t start = clock();
	int i;
	for(i=0;i<1000000000;i++) {
	}
	clock_t end = clock();
	float seconds = (float)(end - start) / CLOCKS_PER_SEC;
	printf("time : %f\n",seconds);
	return 0;
}

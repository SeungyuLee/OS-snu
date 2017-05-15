#include <stdio.h>
#include <math.h>
#include <wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/sched.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define SYSCALL_GET_SCHEDULER 157
#define SYSCALL_SET_SCHEDULER 156
#define SYSCALL_SCHED_SETWEIGHT 380
#define SYSCALL_SCHED_GETWEIGHT 381

double order_array[20];
static struct timeval start, end, diff;

static void get_start_time(){
	gettimeofday(&start, NULL);
}

static double get_end_time(){
	struct timeval end, diff;
	
	gettimeofday(&end, NULL);
	
	diff.tv_sec = end.tv_sec - start.tv_sec;
	diff.tv_usec = end.tv_usec - start.tv_usec;

	return diff.tv_sec + (diff.tv_usec / pow(10,6));
}

void primeFactor(int num){
	int init = num;
	int factor = 2;
	int cnt = 1;
	long A[100] = {1,0,};

	while(num != 1){
		if(num % factor == 0){
			A[cnt] = factor;
			cnt++;
			num /= factor;		
		}
		else
			factor++;
	}

	printf("%d = ", init);
	for(int j=1; j<cnt-1; j++)
		printf("%ld * ", A[j]);
	printf("%ld\n", A[cnt-1]);
	
	return;
}

int main(int argc, const char *argv[]){
	int weight;
	int get_weight;
	long long number = atoll(argv[1]);
	double execution_time;
	
	int counter = 1;
	int status;
	int processes;	

	pid_t pid = getpid();
	pid_t pid_child;
	pid_t pid_wait;

	struct sched_param param;
	param.sched_priority = 99;
	
	int result;

	//scheduler information
	printf("current scheduler is %d\n", syscall(SYSCALL_GET_SCHEDULER, pid));
	result = sched_setscheduler(pid, 6, &param);
	if(result == 0)
		printf("sched_setscheduler success\n");
	printf("current scheduler is %d\n", syscall(SYSCALL_GET_SCHEDULER, pid));

	//factorization with weight
	for(processes = 1; processes <= 20; processes++ ){
		weight = processes;
		pid_child = fork();
		
		if(pid_child == 0){ //only child
			if(syscall(SYSCALL_SCHED_SETWEIGHT, pid_child, weight) == 0)
				printf("weight updated.");
			
			get_weight = syscall(SYSCALL_SCHED_GETWEIGHT, pid_child);
			printf("get weight : %d", get_weight);

			get_start_time();
			primeFactor(number);
			execution_time = get_end_time();
			order_array[weight-1] = execution_time;
			
			printf("Process : %d, weight : %d, execution time :%f\n", getpid(), get_weight, execution_time);
			break;	
		}
	}

	while(1){
		counter++;
		pid_wait = waitpid(pid_child, &status , WNOHANG);
		if(pid_wait != 0)
			exit(0);
	}

}

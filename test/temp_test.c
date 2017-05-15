#include <time.h>
#include <stdio.h>
#include <math.h>
#include <wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>

static struct timeval start, end, diff;
double ordering_array[20];
void prime_sieve(char *sieve, long long length); 
void factorizing(long long n, char *sieve);
static void start_time()
{
    int ret;
    ret = gettimeofday(&start, NULL);
    if(ret != 0)
        perror("start_time() failed!");
}

static double end_time()
{
    int ret;
    struct timeval end, diff;
    ret = gettimeofday(&end, NULL);
    if(ret != 0)
        perror("end_time() failed!");

    diff.tv_sec = end.tv_sec - start.tv_sec;
    diff.tv_usec = end.tv_usec - start.tv_usec;

    return diff.tv_sec + (diff.tv_usec / pow(10,6));
}

static int is_prime(long long n)
{
    long long i;
    if (n % 2 == 0) return 0;
    long long max = floor(sqrtl(n));
	for (i = 2; i <= max; i++) {
        if (n % i == 0) return 0;
    }
    return 1;
}

static void factorize(long long n)
{
    if (is_prime(n)) {
        printf("%lld is prime.\n", n);
        return;
    }

    long long max = floor(sqrtl(n));
    char *sieve = calloc(max, 1);
    prime_sieve(sieve, max);
    factorizing(n, sieve);
    free(sieve);

}

void prime_sieve(char *sieve, long long length)
{
    long long i, j;

    for (i = 2; i <= length ; i++) {
        if (!sieve[i]) {
            for (j = 2 * i; j < length; j += i)
                sieve[j] = 1;
        }
    }
}

void factorizing(long long n, char *sieve)
{
    long long i;
    long long max = floor(sqrtl(n));
    for (i = 2; i <= max; i++) {
        if (sieve[i]) continue;
        long long prime = i;
        if (prime * prime == n) {
            if(i == max) {
                printf("%lld*%lld\n", prime, prime);
                break;
            } else {
                printf("%lld*%lld", prime, prime);
                break;
            }
        }

        if (n % prime == 0) {
            printf("%lld*", prime);
            long long otherFactor = n/prime;
            if (is_prime(otherFactor))
                printf("%lld\n", otherFactor);

            else
                factorizing(otherFactor, sieve);
            break;
        }
    }
}

int main(int argc, const char *argv[])
{
    int weight;
    int get_weight;
	long long number = atoll(argv[1]);
    double execution_time;
    int counter = 1;
    int status;
    pid_t pid = getpid();
    pid_t pid_child;
    pid_t pid_wait;
	int processes;
    struct sched_param param;
    param.sched_priority = 99;
    int ret;

    printf("current scheduler : %d\n", syscall(157, pid));
    ret = sched_setscheduler(pid, SCHED_RR, &param);
    if(ret != 0) perror("sched_setscheduler failed\n");
    else printf("sched_setscheduler success\n");
	printf("current scheduler : %d\n", syscall(157, pid));

	for(processes = 1; processes <= 20; processes++)
	{	
		weight = processes; 
		pid_child = fork();

		if(pid_child < 0) //error
		{
			perror("fork error");
			exit(0);

		}else if(pid_child == 0) //child
	    {
		    if(syscall(380, pid_child, weight) != 0) perror("set weight failed\n");
            else printf("weight updated!");

            get_weight = syscall(381, pid_child);
            printf("get weight : %d\n", get_weight);
			
			start_time();
			factorize(number);
			execution_time = end_time();
			ordering_array[weight-1] = execution_time;
			
			printf("Process : %d, weight : %d, execution time : %f\n", getpid(), get_weight, execution_time);
			break;

		}
	}	
	
	while(1)
	{
		counter++;
        pid_wait = waitpid(pid_child, &status, WNOHANG);
        if(0 != pid_wait)
        {
			exit(0);
		}
	}		
		
	
}

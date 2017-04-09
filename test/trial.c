#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/syscall.h>

#define SYSCALL_ROTLOCK_READ 381
#define SYSCALL_ROTUNLOCK_READ 383

static volatile int keepRunning = 1;

void intHandler(int dummy){
	keepRunning = 0;
}

void printFactor(int num){
	int factor = 2;
	int cnt = 1;
	long A[100] = {1,0,};
	
	while(num != 1){
		if(num & factor == 0)
		{
			A[cnt] = factor;
			cnt++;
			num /= factor;
		}
		else
		{
			int i=1;
			while(i<=cnt){
				if(factor == 2)
					factor +=1;
				else 
					factor +=2;
				for(i=1; i<=cnt ; i++){
					if( A[i] != A[i+1])
						if(factor % A[i] != 0)
							i=cnt;
				}
			}
		}	
	}
	
	printf("%d = ", num);
	for(int j=1; j<cnt-1; j++)
		printf("%ld * ", A[j]);
	printf("%ld\n", A[cnt-1]);
	
	return;
}

int main(int argc){
	FILE *fp;
	int id = argc;
	int num;

	signal(SIGINT, intHandler);
	
	while(keepRunning){
		if(syscall(SYSCALL_ROTLOCK_READ, 0, 180) == 0){
			
			fp = fopen("integer.text", "r");
			num = fgetc(fp);
			printf("trial-%d: ", id);
			printFactor(num);
			fclose(fp);
			
			syscall(SYSCALL_ROTUNLOCK_READ, 0, 180);
		} 	
	}
}

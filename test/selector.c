#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/syscall.h>

#define SYSCALL_ROTLOCK_WRITE 382
#define SYSCALL_ROTUNLOCK_WRITE 385

static volatile int keepRunning = 1;

void intHandler(int dummy) {
	keepRunning = 0;
}

int main(int argc){
	FILE *fp;
	int inputInt = argc;
	
	signal(SIGINT, intHandler);
	
	while(keepRunning){
		if(syscall(SYSCALL_ROTLOCK_WRITE, 0, 180) == 0){
			fp = fopen("integer.text", "w");
			fprintf(fp, "%d", inputInt);
			printf("select: %d\n", inputInt);
			inputInt++;
			fclose(fp);
			
			syscall(SYSCALL_ROTUNLOCK_WRITE, 0, 180);
		}
	}

	return 0;
}

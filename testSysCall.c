#define __NR_ptree 380
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "include/linux/prinfo.h"


int main()
{

	int num = 100;
	struct prinfo *buf = (struct prinfo *) calloc(num, sizeof(struct prinfo));
	int *nr = &num;

	int result = syscall(380, buf, nr);
	printf("ptree syscall returned with %d\n", result);
	
	int i;
	for(i=0; i<num; i=i+1){
		printf("%s,%d,%ld,%d,%d,%d,%d\n", buf[i].comm, buf[i].pid, 
				buf[i].parent_pid, buf[i].first_child_pid, 
				buf[i].next_sibling_pid, p.uid);
	}

	return 0;
}

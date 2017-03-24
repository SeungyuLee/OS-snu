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
	
	int i, j;
	int tap_num==0;
	for(i=0; i<num; i=i+1){
		printf("%s,%d,%ld,%d,%d,%d,%ld\n", buf[i].comm, buf[i].pid,
				buf[i].state, buf[i].parent_pid, buf[i].first_child_pid,
				buf[i].next_sibling_pid, buf[i].uid);

		if(buf[i].first_child_pid == buf[i+1].pid) tap_number++;
		if(buf[i].next_sibling_pid == 0) tap_number--;
		
		for(j=0; j<tap_num; j=j+1){
			printf("\t");
		}
	}

	return 0;
}

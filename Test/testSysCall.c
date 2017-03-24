#define __NR_ptree 380
#include <unistd.h>
#include <stdio.h>
#include <prinfo.h>
__syscall2(int, ptree, struct *, prinfo, int *, nr)

int main()
{
//	int result = ptree( , );
	printf("user hello");
	return 0;
}

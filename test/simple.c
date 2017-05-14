#include <stdio.h>
#include <unistd.h>

int main(void)
{
	int i = 0;
	while(1){
		printf("hello i am simple program: %d\n", i++);
		sleep(5);
	}

	return 0;
}

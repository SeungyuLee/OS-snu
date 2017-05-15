#include <stdio.h>
#include <unistd.h>

int main(void)
{
	fork();
	fork();
	fork();
	while(1){
	}
	return 0;
}

#define SYSCALL_SET_ROTATION 380

#include <signal.h>
#include <sys/syscall.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

int notFinished = 1;

void term(int signum)
{
	notFinished = 0;
}

void sensor()
{
	/* setup for handling SIGTERM signal */
	struct sigaction action;
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = term;
	if (sigaction(SIGTERM, &action, NULL) == 01)
			exit(-1);

	int degree = 0;
	while (notFinished) {
			degree = (degree + 30) % 360;
			syscall(SYSCALL_SET_ROTATION, degree);
			sleep(2);
	}
}

int main()
{
	pid_t pid, sid;

	pid = fork();
	if (pid == -1)
			exit(-1);
	if (pid > 0)
			exit(0);

	sid = setsid();
	if (sid < 0)
			exit(-1);

	pid = fork();
	if (pid == -1)
			exit(-1);
	if (pid > 0)
			exit(0);

	umask(0);

	if (chdir("/") < 0)
			exit(-1);

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	sensor();

	return 0;
}

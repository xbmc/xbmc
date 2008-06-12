/* run a command with a limited timeout
   tridge@samba.org, June 2005
   metze@samba.org, March 2006

   attempt to be as portable as possible (fighting posix all the way)
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

static pid_t child_pid;

static void usage(void)
{
	printf("usage: timelimit <time> <command>\n");
	printf("   SIGUSR1 - passes SIGTERM to command's process group\n");
	printf("   SIGALRM - passes SIGTERM to command's process group\n");
	printf("             after 5s SIGKILL will be passed and exit(1)\n");
	printf("   SIGTERM - passes SIGTERM to command's process group\n");
	printf("             after 1s SIGKILL will be passed and exit(1)\n");
}

static void sig_alrm_kill(int sig)
{
	fprintf(stderr, "\nMaximum time expired in timelimit - killing\n");
	kill(-child_pid, SIGKILL);
	exit(1);
}

static void sig_alrm_term(int sig)
{
	kill(-child_pid, SIGTERM);
	alarm(5);
	signal(SIGALRM, sig_alrm_kill);
}

static void sig_term(int sig)
{
	kill(-child_pid, SIGTERM);
	alarm(1);
	signal(SIGALRM, sig_alrm_kill);
}

static void sig_usr1(int sig)
{
	kill(-child_pid, SIGTERM);
}

static void new_process_group(void)
{
	if (setpgid(0,0) == -1) {
		perror("setpgid");
		exit(1);
	}
}


int main(int argc, char *argv[])
{
	int maxtime, ret=1;

	if (argc < 3) {
		usage();
		exit(1);
	}

	maxtime = atoi(argv[1]);

	child_pid = fork();
	if (child_pid == 0) {
		new_process_group();
		execvp(argv[2], argv+2);
		perror(argv[2]);
		exit(1);
	}

	signal(SIGTERM, sig_term);
	signal(SIGUSR1, sig_usr1);
	signal(SIGALRM, sig_alrm_term);
	alarm(maxtime);

	do {
		int status;
		pid_t pid = wait(&status);
		if (pid != -1) {
			ret = WEXITSTATUS(status);
		} else if (errno == ECHILD) {
			break;
		}
	} while (1);

	kill(-child_pid, SIGKILL);

	exit(ret);
}

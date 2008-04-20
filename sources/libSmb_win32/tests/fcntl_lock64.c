/* test whether 64 bit fcntl locking really works on this system */

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif

#include <errno.h>

static int sys_waitpid(pid_t pid,int *status,int options)
{
#ifdef HAVE_WAITPID
  return waitpid(pid,status,options);
#else /* USE_WAITPID */
  return wait4(pid, status, options, NULL);
#endif /* USE_WAITPID */
}

#define DATA "conftest.fcntl64"

/* lock a byte range in a open file */
int main(int argc, char *argv[])
{
	struct flock64 lock;
	int fd, ret, status=1;
	pid_t pid;

	if (!(pid=fork())) {
		sleep(2);
		fd = open64(DATA, O_RDONLY);

		if (fd == -1) exit(1);

		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 4;
		lock.l_pid = getpid();
		
		lock.l_type = F_WRLCK;
		
		/* check if a lock applies */
		ret = fcntl(fd,F_GETLK64,&lock);

		if ((ret == -1) ||
		    (lock.l_type == F_UNLCK)) {
/*            printf("No lock conflict\n"); */
			exit(1);
		} else {
/*            printf("lock conflict\n"); */
			exit(0);
		}
	}

	fd = open64(DATA, O_RDWR|O_CREAT|O_TRUNC, 0600);

	lock.l_type = F_WRLCK;
	lock.l_whence = SEEK_SET;
#if defined(COMPILER_SUPPORTS_LL)
	lock.l_start = 0x100000000LL;
#else
	lock.l_start = 0x100000000;
#endif
	lock.l_len = 4;
	lock.l_pid = getpid();

	/* set a 4 byte write lock */
	fcntl(fd,F_SETLK64,&lock);

	sys_waitpid(pid, &status, 0);

#if defined(WIFEXITED) && defined(WEXITSTATUS)
	if(WIFEXITED(status)) {
		status = WEXITSTATUS(status);
	} else {
		status = 1;
	}
#else /* defined(WIFEXITED) && defined(WEXITSTATUS) */
	status = (status == 0) ? 0 : 1;
#endif /* defined(WIFEXITED) && defined(WEXITSTATUS) */

	unlink(DATA);

	exit(status);
}

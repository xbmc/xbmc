/* this tests whether we can use a shared writeable mmap on a file -
   as needed for the mmap varient of FAST_SHARE_MODES */

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DATA "conftest.mmap"

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

main()
{
	int *buf;
	int i; 
	int fd = open(DATA,O_RDWR|O_CREAT|O_TRUNC,0666);
	int count=7;

	if (fd == -1) exit(1);

	for (i=0;i<10000;i++) {
		write(fd,&i,sizeof(i));
	}

	close(fd);

	if (fork() == 0) {
		fd = open(DATA,O_RDWR);
		if (fd == -1) exit(1);

		buf = (int *)mmap(NULL, 10000*sizeof(int), 
				   (PROT_READ | PROT_WRITE), 
				   MAP_FILE | MAP_SHARED, 
				   fd, 0);

		while (count-- && buf[9124] != 55732) sleep(1);

		if (count <= 0) exit(1);

		buf[1763] = 7268;
		exit(0);
	}

	fd = open(DATA,O_RDWR);
	if (fd == -1) exit(1);

	buf = (int *)mmap(NULL, 10000*sizeof(int), 
			   (PROT_READ | PROT_WRITE), 
			   MAP_FILE | MAP_SHARED, 
			   fd, 0);

	if (buf == (int *)-1) exit(1);

	buf[9124] = 55732;

	while (count-- && buf[1763] != 7268) sleep(1);

	unlink(DATA);
		
	if (count > 0) exit(0);
	exit(1);
}

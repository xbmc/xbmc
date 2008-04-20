/*
  test readdir/unlink pattern that OS/2 uses
  tridge@samba.org July 2005
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define NUM_FILES 700
#define READDIR_SIZE 100
#define DELETE_SIZE 4

#define TESTDIR "test.dir"

#define FAILED(d) (fprintf(stderr, "Failed for %s - %s\n", d, strerror(errno)), exit(1), 1)

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

static void cleanup(void)
{
	/* I'm a lazy bastard */
	system("rm -rf " TESTDIR);
	mkdir(TESTDIR, 0700) == 0 || FAILED("mkdir");
}

static void create_files()
{
	int i;
	for (i=0;i<NUM_FILES;i++) {
		char fname[40];
		sprintf(fname, TESTDIR "/test%u.txt", i);
		close(open(fname, O_CREAT|O_RDWR, 0600)) == 0 || FAILED("close");
	}
}

static int os2_delete(DIR *d)
{
	off_t offsets[READDIR_SIZE];
	int i, j;
	struct dirent *de;
	char names[READDIR_SIZE][30];

	/* scan, remembering offsets */
	for (i=0, de=readdir(d); 
	     de && i < READDIR_SIZE; 
	     de=readdir(d), i++) {
		offsets[i] = telldir(d);
		strcpy(names[i], de->d_name);
	}

	if (i == 0) {
		return 0;
	}

	/* delete the first few */
	for (j=0; j<MIN(i, DELETE_SIZE); j++) {
		char fname[40];
		sprintf(fname, TESTDIR "/%s", names[j]);
		unlink(fname) == 0 || FAILED("unlink");
	}

	/* seek to just after the deletion */
	seekdir(d, offsets[j-1]);

	/* return number deleted */
	return j;
}

int main(void)
{
	int total_deleted = 0;
	DIR *d;
	struct dirent *de;

	cleanup();
	create_files();
	
	d = opendir(TESTDIR);

	/* skip past . and .. */
	de = readdir(d);
	strcmp(de->d_name, ".") == 0 || FAILED("match .");
	de = readdir(d);
	strcmp(de->d_name, "..") == 0 || FAILED("match ..");

	while (1) {
		int n = os2_delete(d);
		if (n == 0) break;
		total_deleted += n;
	}
	closedir(d);

	printf("Deleted %d files of %d\n", total_deleted, NUM_FILES);

	rmdir(TESTDIR) == 0 || FAILED("rmdir");

	return 0;
}
/*
  test readdir/unlink pattern that OS/2 uses
  tridge@samba.org July 2005
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define NUM_FILES 700
#define READDIR_SIZE 100
#define DELETE_SIZE 4

#define TESTDIR "test.dir"

#define FAILED(d) (fprintf(stderr, "Failed for %s - %s\n", d, strerror(errno)), exit(1), 1)

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

static void cleanup(void)
{
	/* I'm a lazy bastard */
	system("rm -rf " TESTDIR);
	mkdir(TESTDIR, 0700) == 0 || FAILED("mkdir");
}

static void create_files()
{
	int i;
	for (i=0;i<NUM_FILES;i++) {
		char fname[40];
		sprintf(fname, TESTDIR "/test%u.txt", i);
		close(open(fname, O_CREAT|O_RDWR, 0600)) == 0 || FAILED("close");
	}
}

static int os2_delete(DIR *d)
{
	off_t offsets[READDIR_SIZE];
	int i, j;
	struct dirent *de;
	char names[READDIR_SIZE][30];

	/* scan, remembering offsets */
	for (i=0, de=readdir(d); 
	     de && i < READDIR_SIZE; 
	     de=readdir(d), i++) {
		offsets[i] = telldir(d);
		strcpy(names[i], de->d_name);
	}

	if (i == 0) {
		return 0;
	}

	/* delete the first few */
	for (j=0; j<MIN(i, DELETE_SIZE); j++) {
		char fname[40];
		sprintf(fname, TESTDIR "/%s", names[j]);
		unlink(fname) == 0 || FAILED("unlink");
	}

	/* seek to just after the deletion */
	seekdir(d, offsets[j-1]);

	/* return number deleted */
	return j;
}

int main(void)
{
	int total_deleted = 0;
	DIR *d;
	struct dirent *de;

	cleanup();
	create_files();
	
	d = opendir(TESTDIR);

	/* skip past . and .. */
	de = readdir(d);
	strcmp(de->d_name, ".") == 0 || FAILED("match .");
	de = readdir(d);
	strcmp(de->d_name, "..") == 0 || FAILED("match ..");

	while (1) {
		int n = os2_delete(d);
		if (n == 0) break;
		total_deleted += n;
	}
	closedir(d);

	printf("Deleted %d files of %d\n", total_deleted, NUM_FILES);

	rmdir(TESTDIR) == 0 || FAILED("rmdir");

	return 0;
}

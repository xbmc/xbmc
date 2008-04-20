#ifdef _XBOX
	#include <xtl.h>
	#include <wchar.h>
	#include "config.h"
	#include <signal.h>
	#include <stdio.h>
	#include <fcntl.h>
	#include "includes.h"
#elif
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#endif //_XBOX
#include "tdb.h"

/* this tests tdb by doing lots of ops from several simultaneous
   writers - that stresses the locking code. Build with TDB_DEBUG=1
   for best effect */



#define REOPEN_PROB 30
#define DELETE_PROB 8
#define STORE_PROB 4
#define APPEND_PROB 6
#define LOCKSTORE_PROB 0
#define TRAVERSE_PROB 20
#define CULL_PROB 100
#define KEYLEN 3
#define DATALEN 100
#define LOCKLEN 20

static TDB_CONTEXT *db;

static void tdb_log(TDB_CONTEXT *tdb, int level, const char *format, ...)
{
	va_list ap;
    
	va_start(ap, format);
	vfprintf(stdout, format, ap);
	va_end(ap);
	fflush(stdout);
#if 0
	{
		char *ptr;
		asprintf(&ptr,"xterm -e gdb /proc/%d/exe %d", getpid(), getpid());
		system(ptr);
		free(ptr);
	}
#endif	
}

static void fatal(char *why)
{
	perror(why);
	exit(1);
}

static char *randbuf(int len)
{
	char *buf;
	int i;
	buf = (char *)malloc(len+1);

	for (i=0;i<len;i++) {
		buf[i] = 'a' + (rand() % 26);
	}
	buf[i] = 0;
	return buf;
}

static int cull_traverse(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA dbuf,
			 void *state)
{
	if (random() % CULL_PROB == 0) {
		tdb_delete(tdb, key);
	}
	return 0;
}

static void addrec_db(void)
{
	int klen, dlen, slen;
	char *k, *d, *s;
	TDB_DATA key, data, lockkey;

	klen = 1 + (rand() % KEYLEN);
	dlen = 1 + (rand() % DATALEN);
	slen = 1 + (rand() % LOCKLEN);

	k = randbuf(klen);
	d = randbuf(dlen);
	s = randbuf(slen);

	key.dptr = k;
	key.dsize = klen+1;

	data.dptr = d;
	data.dsize = dlen+1;

	lockkey.dptr = s;
	lockkey.dsize = slen+1;

#if REOPEN_PROB
	if (random() % REOPEN_PROB == 0) {
		tdb_reopen_all(1);
		goto next;
	} 
#endif

#if DELETE_PROB
	if (random() % DELETE_PROB == 0) {
		tdb_delete(db, key);
		goto next;
	}
#endif

#if STORE_PROB
	if (random() % STORE_PROB == 0) {
		if (tdb_store(db, key, data, TDB_REPLACE) != 0) {
			fatal("tdb_store failed");
		}
		goto next;
	}
#endif

#if APPEND_PROB
	if (random() % APPEND_PROB == 0) {
		if (tdb_append(db, key, data) != 0) {
			fatal("tdb_append failed");
		}
		goto next;
	}
#endif

#if LOCKSTORE_PROB
	if (random() % LOCKSTORE_PROB == 0) {
		tdb_chainlock(db, lockkey);
		data = tdb_fetch(db, key);
		if (tdb_store(db, key, data, TDB_REPLACE) != 0) {
			fatal("tdb_store failed");
		}
		if (data.dptr) free(data.dptr);
		tdb_chainunlock(db, lockkey);
		goto next;
	} 
#endif

#if TRAVERSE_PROB
	if (random() % TRAVERSE_PROB == 0) {
		tdb_traverse(db, cull_traverse, NULL);
		goto next;
	}
#endif

	data = tdb_fetch(db, key);
	if (data.dptr) free(data.dptr);

next:
	free(k);
	free(d);
	free(s);
}

static int traverse_fn(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA dbuf,
                       void *state)
{
	tdb_delete(tdb, key);
	return 0;
}

#ifndef NPROC
#define NPROC 6
#endif

#ifndef NLOOPS
#define NLOOPS 200000
#endif

int main(int argc, char *argv[])
{
	int i, seed=0;
	int loops = NLOOPS;
	pid_t pids[NPROC];

	db = tdb_open("torture.tdb", 0, TDB_CLEAR_IF_FIRST, 
		      O_RDWR | O_CREAT, 0600);
	if (!db) {
		fatal("db open failed");
	}

	for (i=0;i<NPROC;i++) {
		pids[i] = fork();
		if (pids[i] == 0) {
			tdb_reopen_all(1);

			tdb_logging_function(db, tdb_log);

			srand(seed + getpid());
			srandom(seed + getpid() + time(NULL));
			for (i=0;i<loops;i++) addrec_db();

			tdb_traverse(db, NULL, NULL);
			tdb_traverse(db, traverse_fn, NULL);
			tdb_traverse(db, traverse_fn, NULL);

			tdb_close(db);
			exit(0);
		}
	}

	for (i=0;i<NPROC;i++) {
		int status;
		if (waitpid(pids[i], &status, 0) != pids[i]) {
			printf("failed to wait for %d\n",
			       (int)pids[i]);
			exit(1);
		}
		if (WEXITSTATUS(status) != 0) {
			printf("child %d exited with status %d\n",
			       (int)pids[i], WEXITSTATUS(status));
			exit(1);
		}
	}
	printf("OK\n");
	return 0;
}

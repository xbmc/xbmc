#ifndef __TDB_H__
#define __TDB_H__

/* 
   Unix SMB/CIFS implementation.
   
   trivial database library
   
   Copyright (C) Andrew Tridgell 1999-2004
   
     ** NOTE! The following LGPL license applies to the tdb
     ** library. This does NOT imply that all of Samba is released
     ** under the LGPL
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifdef  __cplusplus
extern "C" {
#endif

#ifndef PRINTF_ATTRIBUTE
/** Use gcc attribute to check printf fns.  a1 is the 1-based index of
 * the parameter containing the format, and a2 the index of the first
 * argument. Note that some gcc 2.x versions don't handle this
 * properly **/
#if (__GNUC__ >= 3) && (__GNUC_MINOR__ >= 1 )
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif
#endif

/* flags to tdb_store() */
#define TDB_REPLACE 1
#define TDB_INSERT 2
#define TDB_MODIFY 3

/* flags for tdb_open() */
#define TDB_DEFAULT 0 /* just a readability place holder */
#define TDB_CLEAR_IF_FIRST 1
#define TDB_INTERNAL 2 /* don't store on disk */
#define TDB_NOLOCK   4 /* don't do any locking */
#define TDB_NOMMAP   8 /* don't use mmap */
#define TDB_CONVERT 16 /* convert endian (internal use) */
#define TDB_BIGENDIAN 32 /* header is big-endian (internal use) */

#define TDB_ERRCODE(code, ret) ((tdb->ecode = (code)), ret)

/* error codes */
enum TDB_ERROR {TDB_SUCCESS=0, TDB_ERR_CORRUPT, TDB_ERR_IO, TDB_ERR_LOCK, 
		TDB_ERR_OOM, TDB_ERR_EXISTS, TDB_ERR_NOLOCK, TDB_ERR_LOCK_TIMEOUT,
		TDB_ERR_NOEXIST};

#ifndef u32
#define u32 unsigned
#endif

typedef struct {
	char *dptr;
	size_t dsize;
} TDB_DATA;

typedef u32 tdb_len;
typedef u32 tdb_off;

/* this is stored at the front of every database */
struct tdb_header {
	char magic_food[32]; /* for /etc/magic */
	u32 version; /* version of the code */
	u32 hash_size; /* number of hash entries */
	tdb_off rwlocks;
	tdb_off reserved[31];
};

struct tdb_lock_type {
	u32 count;
	u32 ltype;
};

struct tdb_traverse_lock {
	struct tdb_traverse_lock *next;
	u32 off;
	u32 hash;
};

/* this is the context structure that is returned from a db open */
typedef struct tdb_context {
	char *name; /* the name of the database */
	void *map_ptr; /* where it is currently mapped */
	int fd; /* open file descriptor for the database */
	tdb_len map_size; /* how much space has been mapped */
	int read_only; /* opened read-only */
	struct tdb_lock_type *locked; /* array of chain locks */
	enum TDB_ERROR ecode; /* error code for last tdb error */
	struct tdb_header header; /* a cached copy of the header */
	u32 flags; /* the flags passed to tdb_open */
	struct tdb_traverse_lock travlocks; /* current traversal locks */
	struct tdb_context *next; /* all tdbs to avoid multiple opens */
	dev_t device;	/* uniquely identifies this tdb */
	ino_t inode;	/* uniquely identifies this tdb */
	void (*log_fn)(struct tdb_context *tdb, int level, const char *, ...) PRINTF_ATTRIBUTE(3,4); /* logging function */
	u32 (*hash_fn)(TDB_DATA *key);
	int open_flags; /* flags used in the open - needed by reopen */
} TDB_CONTEXT;

typedef int (*tdb_traverse_func)(TDB_CONTEXT *, TDB_DATA, TDB_DATA, void *);
typedef void (*tdb_log_func)(TDB_CONTEXT *, int , const char *, ...);
typedef u32 (*tdb_hash_func)(TDB_DATA *key);

TDB_CONTEXT *tdb_open(const char *name, int hash_size, int tdb_flags,
		      int open_flags, mode_t mode);
TDB_CONTEXT *tdb_open_ex(const char *name, int hash_size, int tdb_flags,
			 int open_flags, mode_t mode,
			 tdb_log_func log_fn,
			 tdb_hash_func hash_fn);

int tdb_reopen(TDB_CONTEXT *tdb);
int tdb_reopen_all(int);
void tdb_logging_function(TDB_CONTEXT *tdb, tdb_log_func);
enum TDB_ERROR tdb_error(TDB_CONTEXT *tdb);
const char *tdb_errorstr(TDB_CONTEXT *tdb);
TDB_DATA tdb_fetch(TDB_CONTEXT *tdb, TDB_DATA key);
int tdb_delete(TDB_CONTEXT *tdb, TDB_DATA key);
int tdb_store(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA dbuf, int flag);
int tdb_append(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA new_dbuf);
int tdb_close(TDB_CONTEXT *tdb);
TDB_DATA tdb_firstkey(TDB_CONTEXT *tdb);
TDB_DATA tdb_nextkey(TDB_CONTEXT *tdb, TDB_DATA key);
int tdb_traverse(TDB_CONTEXT *tdb, tdb_traverse_func fn, void *);
int tdb_exists(TDB_CONTEXT *tdb, TDB_DATA key);
int tdb_lockall(TDB_CONTEXT *tdb);
void tdb_unlockall(TDB_CONTEXT *tdb);

/* Low level locking functions: use with care */
void tdb_set_lock_alarm(sig_atomic_t *palarm);
int tdb_chainlock(TDB_CONTEXT *tdb, TDB_DATA key);
int tdb_chainunlock(TDB_CONTEXT *tdb, TDB_DATA key);

/* Debug functions. Not used in production. */
void tdb_dump_all(TDB_CONTEXT *tdb);
int tdb_printfreelist(TDB_CONTEXT *tdb);

extern TDB_DATA tdb_null;

#ifdef  __cplusplus
}
#endif

#endif /* tdb.h */

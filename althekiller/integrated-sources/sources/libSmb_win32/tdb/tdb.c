 /* 
   Unix SMB/CIFS implementation.

   trivial database library

   Copyright (C) Andrew Tridgell              1999-2004
   Copyright (C) Paul `Rusty' Russell		   2000
   Copyright (C) Jeremy Allison			   2000-2003
   
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


/* NOTE: If you use tdbs under valgrind, and in particular if you run
 * tdbtorture, you may get spurious "uninitialized value" warnings.  I
 * think this is because valgrind doesn't understand that the mmap'd
 * area may be written to by other processes.  Memory can, from the
 * point of view of the grinded process, spontaneously become
 * initialized.
 *
 * I can think of a few solutions.  [mbp 20030311]
 *
 * 1 - Write suppressions for Valgrind so that it doesn't complain
 * about this.  Probably the most reasonable but people need to
 * remember to use them.
 *
 * 2 - Use IO not mmap when running under valgrind.  Not so nice.
 *
 * 3 - Use the special valgrind macros to mark memory as valid at the
 * right time.  Probably too hard -- the process just doesn't know.
 */ 

#ifdef STANDALONE
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include "tdb.h"
#include "spinlock.h"
#else
#include "includes.h"

#if defined(PARANOID_MALLOC_CHECKER)
#ifdef malloc
#undef malloc
#endif

#ifdef realloc
#undef realloc
#endif

#ifdef calloc
#undef calloc
#endif

#ifdef strdup
#undef strdup
#endif

#ifdef strndup
#undef strndup
#endif

#endif

#endif

#define TDB_MAGIC_FOOD "TDB file\n"
#define TDB_VERSION (0x26011967 + 6)
#define TDB_MAGIC (0x26011999U)
#define TDB_FREE_MAGIC (~TDB_MAGIC)
#define TDB_DEAD_MAGIC (0xFEE1DEAD)
#define TDB_ALIGNMENT 4
#define MIN_REC_SIZE (2*sizeof(struct list_struct) + TDB_ALIGNMENT)
#define DEFAULT_HASH_SIZE 131
#define TDB_PAGE_SIZE 0x2000
#define FREELIST_TOP (sizeof(struct tdb_header))
#define TDB_ALIGN(x,a) (((x) + (a)-1) & ~((a)-1))
#define TDB_BYTEREV(x) (((((x)&0xff)<<24)|((x)&0xFF00)<<8)|(((x)>>8)&0xFF00)|((x)>>24))
#define TDB_DEAD(r) ((r)->magic == TDB_DEAD_MAGIC)
#define TDB_BAD_MAGIC(r) ((r)->magic != TDB_MAGIC && !TDB_DEAD(r))
#define TDB_HASH_TOP(hash) (FREELIST_TOP + (BUCKET(hash)+1)*sizeof(tdb_off))
#define TDB_DATA_START(hash_size) (TDB_HASH_TOP(hash_size-1) + TDB_SPINLOCK_SIZE(hash_size))


/* NB assumes there is a local variable called "tdb" that is the
 * current context, also takes doubly-parenthesized print-style
 * argument. */
#define TDB_LOG(x) (tdb->log_fn?((tdb->log_fn x),0) : 0)

/* lock offsets */
#define GLOBAL_LOCK 0
#define ACTIVE_LOCK 4

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

/* free memory if the pointer is valid and zero the pointer */
#ifndef SAFE_FREE
#define SAFE_FREE(x) do { if ((x) != NULL) {free((x)); (x)=NULL;} } while(0)
#endif

#define BUCKET(hash) ((hash) % tdb->header.hash_size)
TDB_DATA tdb_null;

/* all contexts, to ensure no double-opens (fcntl locks don't nest!) */
static TDB_CONTEXT *tdbs = NULL;

static int tdb_munmap(TDB_CONTEXT *tdb)
{
	if (tdb->flags & TDB_INTERNAL)
		return 0;

#ifdef HAVE_MMAP
	if (tdb->map_ptr) {
		int ret = munmap(tdb->map_ptr, tdb->map_size);
		if (ret != 0)
			return ret;
	}
#endif
	tdb->map_ptr = NULL;
	return 0;
}

static void tdb_mmap(TDB_CONTEXT *tdb)
{
	if (tdb->flags & TDB_INTERNAL)
		return;

#ifdef HAVE_MMAP
	if (!(tdb->flags & TDB_NOMMAP)) {
		tdb->map_ptr = mmap(NULL, tdb->map_size, 
				    PROT_READ|(tdb->read_only? 0:PROT_WRITE), 
				    MAP_SHARED|MAP_FILE, tdb->fd, 0);

		/*
		 * NB. When mmap fails it returns MAP_FAILED *NOT* NULL !!!!
		 */

		if (tdb->map_ptr == MAP_FAILED) {
			tdb->map_ptr = NULL;
			TDB_LOG((tdb, 2, "tdb_mmap failed for size %d (%s)\n", 
				 tdb->map_size, strerror(errno)));
		}
	} else {
		tdb->map_ptr = NULL;
	}
#else
	tdb->map_ptr = NULL;
#endif
}

/* Endian conversion: we only ever deal with 4 byte quantities */
static void *convert(void *buf, u32 size)
{
	u32 i, *p = buf;
	for (i = 0; i < size / 4; i++)
		p[i] = TDB_BYTEREV(p[i]);
	return buf;
}
#define DOCONV() (tdb->flags & TDB_CONVERT)
#define CONVERT(x) (DOCONV() ? convert(&x, sizeof(x)) : &x)

/* the body of the database is made of one list_struct for the free space
   plus a separate data list for each hash value */
struct list_struct {
	tdb_off next; /* offset of the next record in the list */
	tdb_len rec_len; /* total byte length of record */
	tdb_len key_len; /* byte length of key */
	tdb_len data_len; /* byte length of data */
	u32 full_hash; /* the full 32 bit hash of the key */
	u32 magic;   /* try to catch errors */
	/* the following union is implied:
		union {
			char record[rec_len];
			struct {
				char key[key_len];
				char data[data_len];
			}
			u32 totalsize; (tailer)
		}
	*/
};

/***************************************************************
 Allow a caller to set a "alarm" flag that tdb can check to abort
 a blocking lock on SIGALRM.
***************************************************************/

static sig_atomic_t *palarm_fired;

void tdb_set_lock_alarm(sig_atomic_t *palarm)
{
	palarm_fired = palarm;
}

/* a byte range locking function - return 0 on success
   this functions locks/unlocks 1 byte at the specified offset.

   On error, errno is also set so that errors are passed back properly
   through tdb_open(). */
static int tdb_brlock(TDB_CONTEXT *tdb, tdb_off offset, 
		      int rw_type, int lck_type, int probe)
{
#ifdef _XBOX
	// xbox: flush data on unlock?
	if (tdb->flags & TDB_NOLOCK)
		return 0;

	OutputDebugString("Todo: tdb_brlock\n");
#else if
	struct flock fl;
	int ret;

	if (tdb->flags & TDB_NOLOCK)
		return 0;
	if ((rw_type == F_WRLCK) && (tdb->read_only)) {
		errno = EACCES;
		return -1;
	}

	fl.l_type = rw_type;
	fl.l_whence = SEEK_SET;
	fl.l_start = offset;
	fl.l_len = 1;
	fl.l_pid = 0;

	do {
		ret = fcntl(tdb->fd,lck_type,&fl);
		if (ret == -1 && errno == EINTR && palarm_fired && *palarm_fired)
			break;
	} while (ret == -1 && errno == EINTR);

	if (ret == -1) {
		if (!probe && lck_type != F_SETLK) {
			/* Ensure error code is set for log fun to examine. */
			if (errno == EINTR && palarm_fired && *palarm_fired)
				tdb->ecode = TDB_ERR_LOCK_TIMEOUT;
			else
				tdb->ecode = TDB_ERR_LOCK;
			TDB_LOG((tdb, 5,"tdb_brlock failed (fd=%d) at offset %d rw_type=%d lck_type=%d\n", 
				 tdb->fd, offset, rw_type, lck_type));
		}
		/* Was it an alarm timeout ? */
		if (errno == EINTR && palarm_fired && *palarm_fired) {
			TDB_LOG((tdb, 5, "tdb_brlock timed out (fd=%d) at offset %d rw_type=%d lck_type=%d\n", 
				 tdb->fd, offset, rw_type, lck_type));
			return TDB_ERRCODE(TDB_ERR_LOCK_TIMEOUT, -1);
		}
		/* Otherwise - generic lock error. errno set by fcntl.
		 * EAGAIN is an expected return from non-blocking
		 * locks. */
		if (errno != EAGAIN) {
			TDB_LOG((tdb, 5, "tdb_brlock failed (fd=%d) at offset %d rw_type=%d lck_type=%d: %s\n", 
				 tdb->fd, offset, rw_type, lck_type, 
				 strerror(errno)));
		}
		return TDB_ERRCODE(TDB_ERR_LOCK, -1);
	}
#endif //_XBOX
	return 0;
}

/* lock a list in the database. list -1 is the alloc list */
static int tdb_lock(TDB_CONTEXT *tdb, int list, int ltype)
{
	if (list < -1 || list >= (int)tdb->header.hash_size) {
		TDB_LOG((tdb, 0,"tdb_lock: invalid list %d for ltype=%d\n", 
			   list, ltype));
		return -1;
	}
	if (tdb->flags & TDB_NOLOCK)
		return 0;

	/* Since fcntl locks don't nest, we do a lock for the first one,
	   and simply bump the count for future ones */
	if (tdb->locked[list+1].count == 0) {
		if (!tdb->read_only && tdb->header.rwlocks) {
			if (tdb_spinlock(tdb, list, ltype)) {
				TDB_LOG((tdb, 0, "tdb_lock spinlock failed on list %d ltype=%d\n", 
					   list, ltype));
				return -1;
			}
		} else if (tdb_brlock(tdb,FREELIST_TOP+4*list,ltype,F_SETLKW, 0)) {
			TDB_LOG((tdb, 0,"tdb_lock failed on list %d ltype=%d (%s)\n", 
					   list, ltype, strerror(errno)));
			return -1;
		}
		tdb->locked[list+1].ltype = ltype;
	}
	tdb->locked[list+1].count++;
	return 0;
}

/* unlock the database: returns void because it's too late for errors. */
	/* changed to return int it may be interesting to know there
	   has been an error  --simo */
static int tdb_unlock(TDB_CONTEXT *tdb, int list, int ltype)
{
	int ret = -1;

	if (tdb->flags & TDB_NOLOCK)
		return 0;

	/* Sanity checks */
	if (list < -1 || list >= (int)tdb->header.hash_size) {
		TDB_LOG((tdb, 0, "tdb_unlock: list %d invalid (%d)\n", list, tdb->header.hash_size));
		return ret;
	}

	if (tdb->locked[list+1].count==0) {
		TDB_LOG((tdb, 0, "tdb_unlock: count is 0\n"));
		return ret;
	}

	if (tdb->locked[list+1].count == 1) {
		/* Down to last nested lock: unlock underneath */
		if (!tdb->read_only && tdb->header.rwlocks) {
			ret = tdb_spinunlock(tdb, list, ltype);
		} else {
			ret = tdb_brlock(tdb, FREELIST_TOP+4*list, F_UNLCK, F_SETLKW, 0);
		}
	} else {
		ret = 0;
	}
	tdb->locked[list+1].count--;

	if (ret)
		TDB_LOG((tdb, 0,"tdb_unlock: An error occurred unlocking!\n")); 
	return ret;
}

/* check for an out of bounds access - if it is out of bounds then
   see if the database has been expanded by someone else and expand
   if necessary 
   note that "len" is the minimum length needed for the db
*/
static int tdb_oob(TDB_CONTEXT *tdb, tdb_off len, int probe)
{
	struct stat st;
	if (len <= tdb->map_size)
		return 0;
	if (tdb->flags & TDB_INTERNAL) {
		if (!probe) {
			/* Ensure ecode is set for log fn. */
			tdb->ecode = TDB_ERR_IO;
			TDB_LOG((tdb, 0,"tdb_oob len %d beyond internal malloc size %d\n",
				 (int)len, (int)tdb->map_size));
		}
		return TDB_ERRCODE(TDB_ERR_IO, -1);
	}

	if (fstat(tdb->fd, &st) == -1)
		return TDB_ERRCODE(TDB_ERR_IO, -1);

	if (st.st_size < (size_t)len) {
		if (!probe) {
			/* Ensure ecode is set for log fn. */
			tdb->ecode = TDB_ERR_IO;
			TDB_LOG((tdb, 0,"tdb_oob len %d beyond eof at %d\n",
				 (int)len, (int)st.st_size));
		}
		return TDB_ERRCODE(TDB_ERR_IO, -1);
	}

	/* Unmap, update size, remap */
	if (tdb_munmap(tdb) == -1)
		return TDB_ERRCODE(TDB_ERR_IO, -1);
	tdb->map_size = st.st_size;
	tdb_mmap(tdb);
	return 0;
}

/* write a lump of data at a specified offset */
static int tdb_write(TDB_CONTEXT *tdb, tdb_off off, void *buf, tdb_len len)
{
	if (tdb_oob(tdb, off + len, 0) != 0)
		return -1;

	if (tdb->map_ptr)
		memcpy(off + (char *)tdb->map_ptr, buf, len);
#ifdef HAVE_PWRITE
	else if (pwrite(tdb->fd, buf, len, off) != (ssize_t)len) {
#else
	else if (lseek(tdb->fd, off, SEEK_SET) != off
		 || write(tdb->fd, buf, len) != (ssize_t)len) {
#endif
		/* Ensure ecode is set for log fn. */
		tdb->ecode = TDB_ERR_IO;
		TDB_LOG((tdb, 0,"tdb_write failed at %d len=%d (%s)\n",
			   off, len, strerror(errno)));
		return TDB_ERRCODE(TDB_ERR_IO, -1);
	}
	return 0;
}

/* read a lump of data at a specified offset, maybe convert */
static int tdb_read(TDB_CONTEXT *tdb,tdb_off off,void *buf,tdb_len len,int cv)
{
	if (tdb_oob(tdb, off + len, 0) != 0)
		return -1;

	if (tdb->map_ptr)
		memcpy(buf, off + (char *)tdb->map_ptr, len);
#ifdef HAVE_PREAD
	else if (pread(tdb->fd, buf, len, off) != (ssize_t)len) {
#else
	else if (lseek(tdb->fd, off, SEEK_SET) != off
		 || read(tdb->fd, buf, len) != (ssize_t)len) {
#endif
		/* Ensure ecode is set for log fn. */
		tdb->ecode = TDB_ERR_IO;
		TDB_LOG((tdb, 0,"tdb_read failed at %d len=%d (%s)\n",
			   off, len, strerror(errno)));
		return TDB_ERRCODE(TDB_ERR_IO, -1);
	}
	if (cv)
		convert(buf, len);
	return 0;
}

/* read a lump of data, allocating the space for it */
static char *tdb_alloc_read(TDB_CONTEXT *tdb, tdb_off offset, tdb_len len)
{
	char *buf;

	if (!(buf = malloc(len))) {
		/* Ensure ecode is set for log fn. */
		tdb->ecode = TDB_ERR_OOM;
		TDB_LOG((tdb, 0,"tdb_alloc_read malloc failed len=%d (%s)\n",
			   len, strerror(errno)));
		return TDB_ERRCODE(TDB_ERR_OOM, buf);
	}
	if (tdb_read(tdb, offset, buf, len, 0) == -1) {
		SAFE_FREE(buf);
		return NULL;
	}
	return buf;
}

/* read/write a tdb_off */
static int ofs_read(TDB_CONTEXT *tdb, tdb_off offset, tdb_off *d)
{
	return tdb_read(tdb, offset, (char*)d, sizeof(*d), DOCONV());
}
static int ofs_write(TDB_CONTEXT *tdb, tdb_off offset, tdb_off *d)
{
	tdb_off off = *d;
	return tdb_write(tdb, offset, CONVERT(off), sizeof(*d));
}

/* read/write a record */
static int rec_read(TDB_CONTEXT *tdb, tdb_off offset, struct list_struct *rec)
{
	if (tdb_read(tdb, offset, rec, sizeof(*rec),DOCONV()) == -1)
		return -1;
	if (TDB_BAD_MAGIC(rec)) {
		/* Ensure ecode is set for log fn. */
		tdb->ecode = TDB_ERR_CORRUPT;
		TDB_LOG((tdb, 0,"rec_read bad magic 0x%x at offset=%d\n", rec->magic, offset));
		return TDB_ERRCODE(TDB_ERR_CORRUPT, -1);
	}
	return tdb_oob(tdb, rec->next+sizeof(*rec), 0);
}
static int rec_write(TDB_CONTEXT *tdb, tdb_off offset, struct list_struct *rec)
{
	struct list_struct r = *rec;
	return tdb_write(tdb, offset, CONVERT(r), sizeof(r));
}

/* read a freelist record and check for simple errors */
static int rec_free_read(TDB_CONTEXT *tdb, tdb_off off, struct list_struct *rec)
{
	if (tdb_read(tdb, off, rec, sizeof(*rec),DOCONV()) == -1)
		return -1;

	if (rec->magic == TDB_MAGIC) {
		/* this happens when a app is showdown while deleting a record - we should
		   not completely fail when this happens */
		TDB_LOG((tdb, 0,"rec_free_read non-free magic 0x%x at offset=%d - fixing\n", 
			 rec->magic, off));
		rec->magic = TDB_FREE_MAGIC;
		if (tdb_write(tdb, off, rec, sizeof(*rec)) == -1)
			return -1;
	}

	if (rec->magic != TDB_FREE_MAGIC) {
		/* Ensure ecode is set for log fn. */
		tdb->ecode = TDB_ERR_CORRUPT;
		TDB_LOG((tdb, 0,"rec_free_read bad magic 0x%x at offset=%d\n", 
			   rec->magic, off));
		return TDB_ERRCODE(TDB_ERR_CORRUPT, -1);
	}
	if (tdb_oob(tdb, rec->next+sizeof(*rec), 0) != 0)
		return -1;
	return 0;
}

/* update a record tailer (must hold allocation lock) */
static int update_tailer(TDB_CONTEXT *tdb, tdb_off offset,
			 const struct list_struct *rec)
{
	tdb_off totalsize;

	/* Offset of tailer from record header */
	totalsize = sizeof(*rec) + rec->rec_len;
	return ofs_write(tdb, offset + totalsize - sizeof(tdb_off),
			 &totalsize);
}

static tdb_off tdb_dump_record(TDB_CONTEXT *tdb, tdb_off offset)
{
	struct list_struct rec;
	tdb_off tailer_ofs, tailer;

	if (tdb_read(tdb, offset, (char *)&rec, sizeof(rec), DOCONV()) == -1) {
		printf("ERROR: failed to read record at %u\n", offset);
		return 0;
	}

	printf(" rec: offset=%u next=%d rec_len=%d key_len=%d data_len=%d full_hash=0x%x magic=0x%x\n",
	       offset, rec.next, rec.rec_len, rec.key_len, rec.data_len, rec.full_hash, rec.magic);

	tailer_ofs = offset + sizeof(rec) + rec.rec_len - sizeof(tdb_off);
	if (ofs_read(tdb, tailer_ofs, &tailer) == -1) {
		printf("ERROR: failed to read tailer at %u\n", tailer_ofs);
		return rec.next;
	}

	if (tailer != rec.rec_len + sizeof(rec)) {
		printf("ERROR: tailer does not match record! tailer=%u totalsize=%u\n",
				(unsigned)tailer, (unsigned)(rec.rec_len + sizeof(rec)));
	}
	return rec.next;
}

static int tdb_dump_chain(TDB_CONTEXT *tdb, int i)
{
	tdb_off rec_ptr, top;
	int hash_length = 0;

	top = TDB_HASH_TOP(i);

	if (tdb_lock(tdb, i, F_WRLCK) != 0)
		return -1;

	if (ofs_read(tdb, top, &rec_ptr) == -1)
		return tdb_unlock(tdb, i, F_WRLCK);

	if (rec_ptr)
		printf("hash=%d\n", i);

	while (rec_ptr) {
		rec_ptr = tdb_dump_record(tdb, rec_ptr);
		hash_length += 1;
	}

	printf("chain %d length %d\n", i, hash_length);

	return tdb_unlock(tdb, i, F_WRLCK);
}

void tdb_dump_all(TDB_CONTEXT *tdb)
{
	int i;
	for (i=0;i<tdb->header.hash_size;i++) {
		tdb_dump_chain(tdb, i);
	}
	tdb_printfreelist(tdb);
}

int tdb_printfreelist(TDB_CONTEXT *tdb)
{
	int ret;
	long total_free = 0;
	tdb_off offset, rec_ptr;
	struct list_struct rec;

	if ((ret = tdb_lock(tdb, -1, F_WRLCK)) != 0)
		return ret;

	offset = FREELIST_TOP;

	/* read in the freelist top */
	if (ofs_read(tdb, offset, &rec_ptr) == -1) {
		tdb_unlock(tdb, -1, F_WRLCK);
		return 0;
	}

	printf("freelist top=[0x%08x]\n", rec_ptr );
	while (rec_ptr) {
		if (tdb_read(tdb, rec_ptr, (char *)&rec, sizeof(rec), DOCONV()) == -1) {
			tdb_unlock(tdb, -1, F_WRLCK);
			return -1;
		}

		if (rec.magic != TDB_FREE_MAGIC) {
			printf("bad magic 0x%08x in free list\n", rec.magic);
			tdb_unlock(tdb, -1, F_WRLCK);
			return -1;
		}

		printf("entry offset=[0x%08x], rec.rec_len = [0x%08x (%d)]\n", rec.next, rec.rec_len, rec.rec_len );
		total_free += rec.rec_len;

		/* move to the next record */
		rec_ptr = rec.next;
	}
	printf("total rec_len = [0x%08x (%d)]\n", (int)total_free, 
               (int)total_free);

	return tdb_unlock(tdb, -1, F_WRLCK);
}

/* Remove an element from the freelist.  Must have alloc lock. */
static int remove_from_freelist(TDB_CONTEXT *tdb, tdb_off off, tdb_off next)
{
	tdb_off last_ptr, i;

	/* read in the freelist top */
	last_ptr = FREELIST_TOP;
	while (ofs_read(tdb, last_ptr, &i) != -1 && i != 0) {
		if (i == off) {
			/* We've found it! */
			return ofs_write(tdb, last_ptr, &next);
		}
		/* Follow chain (next offset is at start of record) */
		last_ptr = i;
	}
	TDB_LOG((tdb, 0,"remove_from_freelist: not on list at off=%d\n", off));
	return TDB_ERRCODE(TDB_ERR_CORRUPT, -1);
}

/* Add an element into the freelist. Merge adjacent records if
   neccessary. */
static int tdb_free(TDB_CONTEXT *tdb, tdb_off offset, struct list_struct *rec)
{
	tdb_off right, left;

	/* Allocation and tailer lock */
	if (tdb_lock(tdb, -1, F_WRLCK) != 0)
		return -1;

	/* set an initial tailer, so if we fail we don't leave a bogus record */
	if (update_tailer(tdb, offset, rec) != 0) {
		TDB_LOG((tdb, 0, "tdb_free: upfate_tailer failed!\n"));
		goto fail;
	}

	/* Look right first (I'm an Australian, dammit) */
	right = offset + sizeof(*rec) + rec->rec_len;
	if (right + sizeof(*rec) <= tdb->map_size) {
		struct list_struct r;

		if (tdb_read(tdb, right, &r, sizeof(r), DOCONV()) == -1) {
			TDB_LOG((tdb, 0, "tdb_free: right read failed at %u\n", right));
			goto left;
		}

		/* If it's free, expand to include it. */
		if (r.magic == TDB_FREE_MAGIC) {
			if (remove_from_freelist(tdb, right, r.next) == -1) {
				TDB_LOG((tdb, 0, "tdb_free: right free failed at %u\n", right));
				goto left;
			}
			rec->rec_len += sizeof(r) + r.rec_len;
		}
	}

left:
	/* Look left */
	left = offset - sizeof(tdb_off);
	if (left > TDB_DATA_START(tdb->header.hash_size)) {
		struct list_struct l;
		tdb_off leftsize;
		
		/* Read in tailer and jump back to header */
		if (ofs_read(tdb, left, &leftsize) == -1) {
			TDB_LOG((tdb, 0, "tdb_free: left offset read failed at %u\n", left));
			goto update;
		}
		left = offset - leftsize;

		/* Now read in record */
		if (tdb_read(tdb, left, &l, sizeof(l), DOCONV()) == -1) {
			TDB_LOG((tdb, 0, "tdb_free: left read failed at %u (%u)\n", left, leftsize));
			goto update;
		}

		/* If it's free, expand to include it. */
		if (l.magic == TDB_FREE_MAGIC) {
			if (remove_from_freelist(tdb, left, l.next) == -1) {
				TDB_LOG((tdb, 0, "tdb_free: left free failed at %u\n", left));
				goto update;
			} else {
				offset = left;
				rec->rec_len += leftsize;
			}
		}
	}

update:
	if (update_tailer(tdb, offset, rec) == -1) {
		TDB_LOG((tdb, 0, "tdb_free: update_tailer failed at %u\n", offset));
		goto fail;
	}

	/* Now, prepend to free list */
	rec->magic = TDB_FREE_MAGIC;

	if (ofs_read(tdb, FREELIST_TOP, &rec->next) == -1 ||
	    rec_write(tdb, offset, rec) == -1 ||
	    ofs_write(tdb, FREELIST_TOP, &offset) == -1) {
		TDB_LOG((tdb, 0, "tdb_free record write failed at offset=%d\n", offset));
		goto fail;
	}

	/* And we're done. */
	tdb_unlock(tdb, -1, F_WRLCK);
	return 0;

 fail:
	tdb_unlock(tdb, -1, F_WRLCK);
	return -1;
}


/* expand a file.  we prefer to use ftruncate, as that is what posix
  says to use for mmap expansion */
static int expand_file(TDB_CONTEXT *tdb, tdb_off size, tdb_off addition)
{
	char buf[1024];
#if HAVE_FTRUNCATE_EXTEND
	if (ftruncate(tdb->fd, size+addition) != 0) {
		TDB_LOG((tdb, 0, "expand_file ftruncate to %d failed (%s)\n", 
			   size+addition, strerror(errno)));
		return -1;
	}
#else
	char b = 0;

#ifdef HAVE_PWRITE
	if (pwrite(tdb->fd,  &b, 1, (size+addition) - 1) != 1) {
#else
	if (lseek(tdb->fd, (size+addition) - 1, SEEK_SET) != (size+addition) - 1 || 
	    write(tdb->fd, &b, 1) != 1) {
#endif
		TDB_LOG((tdb, 0, "expand_file to %d failed (%s)\n", 
			   size+addition, strerror(errno)));
		return -1;
	}
#endif

	/* now fill the file with something. This ensures that the file isn't sparse, which would be
	   very bad if we ran out of disk. This must be done with write, not via mmap */
	memset(buf, 0x42, sizeof(buf));
	while (addition) {
		int n = addition>sizeof(buf)?sizeof(buf):addition;
#ifdef HAVE_PWRITE
		int ret = pwrite(tdb->fd, buf, n, size);
#else
		int ret;
		if (lseek(tdb->fd, size, SEEK_SET) != size)
			return -1;
		ret = write(tdb->fd, buf, n);
#endif
		if (ret != n) {
			TDB_LOG((tdb, 0, "expand_file write of %d failed (%s)\n", 
				   n, strerror(errno)));
			return -1;
		}
		addition -= n;
		size += n;
	}
	return 0;
}


/* expand the database at least size bytes by expanding the underlying
   file and doing the mmap again if necessary */
static int tdb_expand(TDB_CONTEXT *tdb, tdb_off size)
{
	struct list_struct rec;
	tdb_off offset;

	if (tdb_lock(tdb, -1, F_WRLCK) == -1) {
		TDB_LOG((tdb, 0, "lock failed in tdb_expand\n"));
		return -1;
	}

	/* must know about any previous expansions by another process */
	tdb_oob(tdb, tdb->map_size + 1, 1);

	/* always make room for at least 10 more records, and round
           the database up to a multiple of TDB_PAGE_SIZE */
	size = TDB_ALIGN(tdb->map_size + size*10, TDB_PAGE_SIZE) - tdb->map_size;

	if (!(tdb->flags & TDB_INTERNAL))
		tdb_munmap(tdb);

	/*
	 * We must ensure the file is unmapped before doing this
	 * to ensure consistency with systems like OpenBSD where
	 * writes and mmaps are not consistent.
	 */

	/* expand the file itself */
	if (!(tdb->flags & TDB_INTERNAL)) {
		if (expand_file(tdb, tdb->map_size, size) != 0)
			goto fail;
	}

	tdb->map_size += size;

	if (tdb->flags & TDB_INTERNAL) {
		char *new_map_ptr = realloc(tdb->map_ptr, tdb->map_size);
		if (!new_map_ptr) {
			tdb->map_size -= size;
			goto fail;
		}
		tdb->map_ptr = new_map_ptr;
	} else {
		/*
		 * We must ensure the file is remapped before adding the space
		 * to ensure consistency with systems like OpenBSD where
		 * writes and mmaps are not consistent.
		 */

		/* We're ok if the mmap fails as we'll fallback to read/write */
		tdb_mmap(tdb);
	}

	/* form a new freelist record */
	memset(&rec,'\0',sizeof(rec));
	rec.rec_len = size - sizeof(rec);

	/* link it into the free list */
	offset = tdb->map_size - size;
	if (tdb_free(tdb, offset, &rec) == -1)
		goto fail;

	tdb_unlock(tdb, -1, F_WRLCK);
	return 0;
 fail:
	tdb_unlock(tdb, -1, F_WRLCK);
	return -1;
}

/* allocate some space from the free list. The offset returned points
   to a unconnected list_struct within the database with room for at
   least length bytes of total data

   0 is returned if the space could not be allocated
 */
static tdb_off tdb_allocate(TDB_CONTEXT *tdb, tdb_len length,
			    struct list_struct *rec)
{
	tdb_off rec_ptr, last_ptr, newrec_ptr;
	struct list_struct newrec;

	memset(&newrec, '\0', sizeof(newrec));

	if (tdb_lock(tdb, -1, F_WRLCK) == -1)
		return 0;

	/* Extra bytes required for tailer */
	length += sizeof(tdb_off);

 again:
	last_ptr = FREELIST_TOP;

	/* read in the freelist top */
	if (ofs_read(tdb, FREELIST_TOP, &rec_ptr) == -1)
		goto fail;

	/* keep looking until we find a freelist record big enough */
	while (rec_ptr) {
		if (rec_free_read(tdb, rec_ptr, rec) == -1)
			goto fail;

		if (rec->rec_len >= length) {
			/* found it - now possibly split it up  */
			if (rec->rec_len > length + MIN_REC_SIZE) {
				/* Length of left piece */
				length = TDB_ALIGN(length, TDB_ALIGNMENT);

				/* Right piece to go on free list */
				newrec.rec_len = rec->rec_len
					- (sizeof(*rec) + length);
				newrec_ptr = rec_ptr + sizeof(*rec) + length;

				/* And left record is shortened */
				rec->rec_len = length;
			} else
				newrec_ptr = 0;

			/* Remove allocated record from the free list */
			if (ofs_write(tdb, last_ptr, &rec->next) == -1)
				goto fail;

			/* Update header: do this before we drop alloc
                           lock, otherwise tdb_free() might try to
                           merge with us, thinking we're free.
                           (Thanks Jeremy Allison). */
			rec->magic = TDB_MAGIC;
			if (rec_write(tdb, rec_ptr, rec) == -1)
				goto fail;

			/* Did we create new block? */
			if (newrec_ptr) {
				/* Update allocated record tailer (we
                                   shortened it). */
				if (update_tailer(tdb, rec_ptr, rec) == -1)
					goto fail;

				/* Free new record */
				if (tdb_free(tdb, newrec_ptr, &newrec) == -1)
					goto fail;
			}

			/* all done - return the new record offset */
			tdb_unlock(tdb, -1, F_WRLCK);
			return rec_ptr;
		}
		/* move to the next record */
		last_ptr = rec_ptr;
		rec_ptr = rec->next;
	}
	/* we didn't find enough space. See if we can expand the
	   database and if we can then try again */
	if (tdb_expand(tdb, length + sizeof(*rec)) == 0)
		goto again;
 fail:
	tdb_unlock(tdb, -1, F_WRLCK);
	return 0;
}

/* initialise a new database with a specified hash size */
static int tdb_new_database(TDB_CONTEXT *tdb, int hash_size)
{
	struct tdb_header *newdb;
	int size, ret = -1;

	/* We make it up in memory, then write it out if not internal */
	size = sizeof(struct tdb_header) + (hash_size+1)*sizeof(tdb_off);
	if (!(newdb = calloc(size, 1)))
		return TDB_ERRCODE(TDB_ERR_OOM, -1);

	/* Fill in the header */
	newdb->version = TDB_VERSION;
	newdb->hash_size = hash_size;
#ifdef USE_SPINLOCKS
	newdb->rwlocks = size;
#endif
	if (tdb->flags & TDB_INTERNAL) {
		tdb->map_size = size;
		tdb->map_ptr = (char *)newdb;
		memcpy(&tdb->header, newdb, sizeof(tdb->header));
		/* Convert the `ondisk' version if asked. */
		CONVERT(*newdb);
		return 0;
	}
	if (lseek(tdb->fd, 0, SEEK_SET) == -1)
		goto fail;

	if (ftruncate(tdb->fd, 0) == -1)
		goto fail;

	/* This creates an endian-converted header, as if read from disk */
	CONVERT(*newdb);
	memcpy(&tdb->header, newdb, sizeof(tdb->header));
	/* Don't endian-convert the magic food! */
	memcpy(newdb->magic_food, TDB_MAGIC_FOOD, strlen(TDB_MAGIC_FOOD)+1);
	if (write(tdb->fd, newdb, size) != size)
		ret = -1;
	else
		ret = tdb_create_rwlocks(tdb->fd, hash_size);

  fail:
	SAFE_FREE(newdb);
	return ret;
}

/* Returns 0 on fail.  On success, return offset of record, and fills
   in rec */
static tdb_off tdb_find(TDB_CONTEXT *tdb, TDB_DATA key, u32 hash,
			struct list_struct *r)
{
	tdb_off rec_ptr;
	
	/* read in the hash top */
	if (ofs_read(tdb, TDB_HASH_TOP(hash), &rec_ptr) == -1)
		return 0;

	/* keep looking until we find the right record */
	while (rec_ptr) {
		if (rec_read(tdb, rec_ptr, r) == -1)
			return 0;

		if (!TDB_DEAD(r) && hash==r->full_hash && key.dsize==r->key_len) {
			char *k;
			/* a very likely hit - read the key */
			k = tdb_alloc_read(tdb, rec_ptr + sizeof(*r), 
					   r->key_len);
			if (!k)
				return 0;

			if (memcmp(key.dptr, k, key.dsize) == 0) {
				SAFE_FREE(k);
				return rec_ptr;
			}
			SAFE_FREE(k);
		}
		rec_ptr = r->next;
	}
	return TDB_ERRCODE(TDB_ERR_NOEXIST, 0);
}

/* As tdb_find, but if you succeed, keep the lock */
static tdb_off tdb_find_lock_hash(TDB_CONTEXT *tdb, TDB_DATA key, u32 hash, int locktype,
			     struct list_struct *rec)
{
	u32 rec_ptr;

	if (tdb_lock(tdb, BUCKET(hash), locktype) == -1)
		return 0;
	if (!(rec_ptr = tdb_find(tdb, key, hash, rec)))
		tdb_unlock(tdb, BUCKET(hash), locktype);
	return rec_ptr;
}

enum TDB_ERROR tdb_error(TDB_CONTEXT *tdb)
{
	return tdb->ecode;
}

static struct tdb_errname {
	enum TDB_ERROR ecode; const char *estring;
} emap[] = { {TDB_SUCCESS, "Success"},
	     {TDB_ERR_CORRUPT, "Corrupt database"},
	     {TDB_ERR_IO, "IO Error"},
	     {TDB_ERR_LOCK, "Locking error"},
	     {TDB_ERR_OOM, "Out of memory"},
	     {TDB_ERR_EXISTS, "Record exists"},
	     {TDB_ERR_NOLOCK, "Lock exists on other keys"},
	     {TDB_ERR_NOEXIST, "Record does not exist"} };

/* Error string for the last tdb error */
const char *tdb_errorstr(TDB_CONTEXT *tdb)
{
	u32 i;
	for (i = 0; i < sizeof(emap) / sizeof(struct tdb_errname); i++)
		if (tdb->ecode == emap[i].ecode)
			return emap[i].estring;
	return "Invalid error code";
}

/* update an entry in place - this only works if the new data size
   is <= the old data size and the key exists.
   on failure return -1.
*/

static int tdb_update_hash(TDB_CONTEXT *tdb, TDB_DATA key, u32 hash, TDB_DATA dbuf)
{
	struct list_struct rec;
	tdb_off rec_ptr;

	/* find entry */
	if (!(rec_ptr = tdb_find(tdb, key, hash, &rec)))
		return -1;

	/* must be long enough key, data and tailer */
	if (rec.rec_len < key.dsize + dbuf.dsize + sizeof(tdb_off)) {
		tdb->ecode = TDB_SUCCESS; /* Not really an error */
		return -1;
	}

	if (tdb_write(tdb, rec_ptr + sizeof(rec) + rec.key_len,
		      dbuf.dptr, dbuf.dsize) == -1)
		return -1;

	if (dbuf.dsize != rec.data_len) {
		/* update size */
		rec.data_len = dbuf.dsize;
		return rec_write(tdb, rec_ptr, &rec);
	}
 
	return 0;
}

/* find an entry in the database given a key */
/* If an entry doesn't exist tdb_err will be set to
 * TDB_ERR_NOEXIST. If a key has no data attached
 * tdb_err will not be set. Both will return a
 * zero pptr and zero dsize.
 */

TDB_DATA tdb_fetch(TDB_CONTEXT *tdb, TDB_DATA key)
{
	tdb_off rec_ptr;
	struct list_struct rec;
	TDB_DATA ret;
	u32 hash;

	/* find which hash bucket it is in */
	hash = tdb->hash_fn(&key);
	if (!(rec_ptr = tdb_find_lock_hash(tdb,key,hash,F_RDLCK,&rec)))
		return tdb_null;

	if (rec.data_len)
		ret.dptr = tdb_alloc_read(tdb, rec_ptr + sizeof(rec) + rec.key_len,
					  rec.data_len);
	else
		ret.dptr = NULL;
	ret.dsize = rec.data_len;
	tdb_unlock(tdb, BUCKET(rec.full_hash), F_RDLCK);
	return ret;
}

/* check if an entry in the database exists 

   note that 1 is returned if the key is found and 0 is returned if not found
   this doesn't match the conventions in the rest of this module, but is
   compatible with gdbm
*/
static int tdb_exists_hash(TDB_CONTEXT *tdb, TDB_DATA key, u32 hash)
{
	struct list_struct rec;
	
	if (tdb_find_lock_hash(tdb, key, hash, F_RDLCK, &rec) == 0)
		return 0;
	tdb_unlock(tdb, BUCKET(rec.full_hash), F_RDLCK);
	return 1;
}

int tdb_exists(TDB_CONTEXT *tdb, TDB_DATA key)
{
	u32 hash = tdb->hash_fn(&key);
	return tdb_exists_hash(tdb, key, hash);
}

/* record lock stops delete underneath */
static int lock_record(TDB_CONTEXT *tdb, tdb_off off)
{
	return off ? tdb_brlock(tdb, off, F_RDLCK, F_SETLKW, 0) : 0;
}
/*
  Write locks override our own fcntl readlocks, so check it here.
  Note this is meant to be F_SETLK, *not* F_SETLKW, as it's not
  an error to fail to get the lock here.
*/
 
static int write_lock_record(TDB_CONTEXT *tdb, tdb_off off)
{
	struct tdb_traverse_lock *i;
	for (i = &tdb->travlocks; i; i = i->next)
		if (i->off == off)
			return -1;
	return tdb_brlock(tdb, off, F_WRLCK, F_SETLK, 1);
}

/*
  Note this is meant to be F_SETLK, *not* F_SETLKW, as it's not
  an error to fail to get the lock here.
*/

static int write_unlock_record(TDB_CONTEXT *tdb, tdb_off off)
{
	return tdb_brlock(tdb, off, F_UNLCK, F_SETLK, 0);
}
/* fcntl locks don't stack: avoid unlocking someone else's */
static int unlock_record(TDB_CONTEXT *tdb, tdb_off off)
{
	struct tdb_traverse_lock *i;
	u32 count = 0;

	if (off == 0)
		return 0;
	for (i = &tdb->travlocks; i; i = i->next)
		if (i->off == off)
			count++;
	return (count == 1 ? tdb_brlock(tdb, off, F_UNLCK, F_SETLKW, 0) : 0);
}

/* actually delete an entry in the database given the offset */
static int do_delete(TDB_CONTEXT *tdb, tdb_off rec_ptr, struct list_struct*rec)
{
	tdb_off last_ptr, i;
	struct list_struct lastrec;

	if (tdb->read_only) return -1;

	if (write_lock_record(tdb, rec_ptr) == -1) {
		/* Someone traversing here: mark it as dead */
		rec->magic = TDB_DEAD_MAGIC;
		return rec_write(tdb, rec_ptr, rec);
	}
	if (write_unlock_record(tdb, rec_ptr) != 0)
		return -1;

	/* find previous record in hash chain */
	if (ofs_read(tdb, TDB_HASH_TOP(rec->full_hash), &i) == -1)
		return -1;
	for (last_ptr = 0; i != rec_ptr; last_ptr = i, i = lastrec.next)
		if (rec_read(tdb, i, &lastrec) == -1)
			return -1;

	/* unlink it: next ptr is at start of record. */
	if (last_ptr == 0)
		last_ptr = TDB_HASH_TOP(rec->full_hash);
	if (ofs_write(tdb, last_ptr, &rec->next) == -1)
		return -1;

	/* recover the space */
	if (tdb_free(tdb, rec_ptr, rec) == -1)
		return -1;
	return 0;
}

/* Uses traverse lock: 0 = finish, -1 = error, other = record offset */
static int tdb_next_lock(TDB_CONTEXT *tdb, struct tdb_traverse_lock *tlock,
			 struct list_struct *rec)
{
	int want_next = (tlock->off != 0);

	/* Lock each chain from the start one. */
	for (; tlock->hash < tdb->header.hash_size; tlock->hash++) {

		/* this is an optimisation for the common case where
		   the hash chain is empty, which is particularly
		   common for the use of tdb with ldb, where large
		   hashes are used. In that case we spend most of our
		   time in tdb_brlock(), locking empty hash chains.

		   To avoid this, we do an unlocked pre-check to see
		   if the hash chain is empty before starting to look
		   inside it. If it is empty then we can avoid that
		   hash chain. If it isn't empty then we can't believe
		   the value we get back, as we read it without a
		   lock, so instead we get the lock and re-fetch the
		   value below.

		   Notice that not doing this optimisation on the
		   first hash chain is critical. We must guarantee
		   that we have done at least one fcntl lock at the
		   start of a search to guarantee that memory is
		   coherent on SMP systems. If records are added by
		   others during the search then thats OK, and we
		   could possibly miss those with this trick, but we
		   could miss them anyway without this trick, so the
		   semantics don't change.

		   With a non-indexed ldb search this trick gains us a
		   factor of around 80 in speed on a linux 2.6.x
		   system (testing using ldbtest).
		 */
		if (!tlock->off && tlock->hash != 0) {
			u32 off;
			if (tdb->map_ptr) {
				for (;tlock->hash < tdb->header.hash_size;tlock->hash++) {
					if (0 != *(u32 *)(TDB_HASH_TOP(tlock->hash) + (unsigned char *)tdb->map_ptr)) {
						break;
					}
				}
				if (tlock->hash == tdb->header.hash_size) {
					continue;
				}
			} else {
				if (ofs_read(tdb, TDB_HASH_TOP(tlock->hash), &off) == 0 &&
				    off == 0) {
					continue;
				}
			}
		}

		if (tdb_lock(tdb, tlock->hash, F_WRLCK) == -1)
			return -1;

		/* No previous record?  Start at top of chain. */
		if (!tlock->off) {
			if (ofs_read(tdb, TDB_HASH_TOP(tlock->hash),
				     &tlock->off) == -1)
				goto fail;
		} else {
			/* Otherwise unlock the previous record. */
			if (unlock_record(tdb, tlock->off) != 0)
				goto fail;
		}

		if (want_next) {
			/* We have offset of old record: grab next */
			if (rec_read(tdb, tlock->off, rec) == -1)
				goto fail;
			tlock->off = rec->next;
		}

		/* Iterate through chain */
		while( tlock->off) {
			tdb_off current;
			if (rec_read(tdb, tlock->off, rec) == -1)
				goto fail;

			/* Detect infinite loops. From "Shlomi Yaakobovich" <Shlomi@exanet.com>. */
			if (tlock->off == rec->next) {
				TDB_LOG((tdb, 0, "tdb_next_lock: loop detected.\n"));
				goto fail;
			}

			if (!TDB_DEAD(rec)) {
				/* Woohoo: we found one! */
				if (lock_record(tdb, tlock->off) != 0)
					goto fail;
				return tlock->off;
			}

			/* Try to clean dead ones from old traverses */
			current = tlock->off;
			tlock->off = rec->next;
			if (!tdb->read_only && 
			    do_delete(tdb, current, rec) != 0)
				goto fail;
		}
		tdb_unlock(tdb, tlock->hash, F_WRLCK);
		want_next = 0;
	}
	/* We finished iteration without finding anything */
	return TDB_ERRCODE(TDB_SUCCESS, 0);

 fail:
	tlock->off = 0;
	if (tdb_unlock(tdb, tlock->hash, F_WRLCK) != 0)
		TDB_LOG((tdb, 0, "tdb_next_lock: On error unlock failed!\n"));
	return -1;
}

/* traverse the entire database - calling fn(tdb, key, data) on each element.
   return -1 on error or the record count traversed
   if fn is NULL then it is not called
   a non-zero return value from fn() indicates that the traversal should stop
  */
int tdb_traverse(TDB_CONTEXT *tdb, tdb_traverse_func fn, void *private_val)
{
	TDB_DATA key, dbuf;
	struct list_struct rec;
	struct tdb_traverse_lock tl = { NULL, 0, 0 };
	int ret, count = 0;

	/* This was in the initializaton, above, but the IRIX compiler
	 * did not like it.  crh
	 */
	tl.next = tdb->travlocks.next;

	/* fcntl locks don't stack: beware traverse inside traverse */
	tdb->travlocks.next = &tl;

	/* tdb_next_lock places locks on the record returned, and its chain */
	while ((ret = tdb_next_lock(tdb, &tl, &rec)) > 0) {
		count++;
		/* now read the full record */
		key.dptr = tdb_alloc_read(tdb, tl.off + sizeof(rec), 
					  rec.key_len + rec.data_len);
		if (!key.dptr) {
			ret = -1;
			if (tdb_unlock(tdb, tl.hash, F_WRLCK) != 0)
				goto out;
			if (unlock_record(tdb, tl.off) != 0)
				TDB_LOG((tdb, 0, "tdb_traverse: key.dptr == NULL and unlock_record failed!\n"));
			goto out;
		}
		key.dsize = rec.key_len;
		dbuf.dptr = key.dptr + rec.key_len;
		dbuf.dsize = rec.data_len;

		/* Drop chain lock, call out */
		if (tdb_unlock(tdb, tl.hash, F_WRLCK) != 0) {
			ret = -1;
			SAFE_FREE(key.dptr);
			goto out;
		}
		if (fn && fn(tdb, key, dbuf, private_val)) {
			/* They want us to terminate traversal */
			ret = count;
			if (unlock_record(tdb, tl.off) != 0) {
				TDB_LOG((tdb, 0, "tdb_traverse: unlock_record failed!\n"));;
				ret = -1;
			}
			SAFE_FREE(key.dptr);
			goto out;
		}
		SAFE_FREE(key.dptr);
	}
out:
	tdb->travlocks.next = tl.next;
	if (ret < 0)
		return -1;
	else
		return count;
}

/* find the first entry in the database and return its key */
TDB_DATA tdb_firstkey(TDB_CONTEXT *tdb)
{
	TDB_DATA key;
	struct list_struct rec;

	/* release any old lock */
	if (unlock_record(tdb, tdb->travlocks.off) != 0)
		return tdb_null;
	tdb->travlocks.off = tdb->travlocks.hash = 0;

	if (tdb_next_lock(tdb, &tdb->travlocks, &rec) <= 0)
		return tdb_null;
	/* now read the key */
	key.dsize = rec.key_len;
	key.dptr =tdb_alloc_read(tdb,tdb->travlocks.off+sizeof(rec),key.dsize);
	if (tdb_unlock(tdb, BUCKET(tdb->travlocks.hash), F_WRLCK) != 0)
		TDB_LOG((tdb, 0, "tdb_firstkey: error occurred while tdb_unlocking!\n"));
	return key;
}

/* find the next entry in the database, returning its key */
TDB_DATA tdb_nextkey(TDB_CONTEXT *tdb, TDB_DATA oldkey)
{
	u32 oldhash;
	TDB_DATA key = tdb_null;
	struct list_struct rec;
	char *k = NULL;

	/* Is locked key the old key?  If so, traverse will be reliable. */
	if (tdb->travlocks.off) {
		if (tdb_lock(tdb,tdb->travlocks.hash,F_WRLCK))
			return tdb_null;
		if (rec_read(tdb, tdb->travlocks.off, &rec) == -1
		    || !(k = tdb_alloc_read(tdb,tdb->travlocks.off+sizeof(rec),
					    rec.key_len))
		    || memcmp(k, oldkey.dptr, oldkey.dsize) != 0) {
			/* No, it wasn't: unlock it and start from scratch */
			if (unlock_record(tdb, tdb->travlocks.off) != 0) {
				SAFE_FREE(k);
				return tdb_null;
			}
			if (tdb_unlock(tdb, tdb->travlocks.hash, F_WRLCK) != 0) {
				SAFE_FREE(k);
				return tdb_null;
			}
			tdb->travlocks.off = 0;
		}

		SAFE_FREE(k);
	}

	if (!tdb->travlocks.off) {
		/* No previous element: do normal find, and lock record */
		tdb->travlocks.off = tdb_find_lock_hash(tdb, oldkey, tdb->hash_fn(&oldkey), F_WRLCK, &rec);
		if (!tdb->travlocks.off)
			return tdb_null;
		tdb->travlocks.hash = BUCKET(rec.full_hash);
		if (lock_record(tdb, tdb->travlocks.off) != 0) {
			TDB_LOG((tdb, 0, "tdb_nextkey: lock_record failed (%s)!\n", strerror(errno)));
			return tdb_null;
		}
	}
	oldhash = tdb->travlocks.hash;

	/* Grab next record: locks chain and returned record,
	   unlocks old record */
	if (tdb_next_lock(tdb, &tdb->travlocks, &rec) > 0) {
		key.dsize = rec.key_len;
		key.dptr = tdb_alloc_read(tdb, tdb->travlocks.off+sizeof(rec),
					  key.dsize);
		/* Unlock the chain of this new record */
		if (tdb_unlock(tdb, tdb->travlocks.hash, F_WRLCK) != 0)
			TDB_LOG((tdb, 0, "tdb_nextkey: WARNING tdb_unlock failed!\n"));
	}
	/* Unlock the chain of old record */
	if (tdb_unlock(tdb, BUCKET(oldhash), F_WRLCK) != 0)
		TDB_LOG((tdb, 0, "tdb_nextkey: WARNING tdb_unlock failed!\n"));
	return key;
}

/* delete an entry in the database given a key */
static int tdb_delete_hash(TDB_CONTEXT *tdb, TDB_DATA key, u32 hash)
{
	tdb_off rec_ptr;
	struct list_struct rec;
	int ret;

	if (!(rec_ptr = tdb_find_lock_hash(tdb, key, hash, F_WRLCK, &rec)))
		return -1;
	ret = do_delete(tdb, rec_ptr, &rec);
	if (tdb_unlock(tdb, BUCKET(rec.full_hash), F_WRLCK) != 0)
		TDB_LOG((tdb, 0, "tdb_delete: WARNING tdb_unlock failed!\n"));
	return ret;
}

int tdb_delete(TDB_CONTEXT *tdb, TDB_DATA key)
{
	u32 hash = tdb->hash_fn(&key);
	return tdb_delete_hash(tdb, key, hash);
}

/* store an element in the database, replacing any existing element
   with the same key 

   return 0 on success, -1 on failure
*/
int tdb_store(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA dbuf, int flag)
{
	struct list_struct rec;
	u32 hash;
	tdb_off rec_ptr;
	char *p = NULL;
	int ret = 0;

	/* find which hash bucket it is in */
	hash = tdb->hash_fn(&key);
	if (tdb_lock(tdb, BUCKET(hash), F_WRLCK) == -1)
		return -1;

	/* check for it existing, on insert. */
	if (flag == TDB_INSERT) {
		if (tdb_exists_hash(tdb, key, hash)) {
			tdb->ecode = TDB_ERR_EXISTS;
			goto fail;
		}
	} else {
		/* first try in-place update, on modify or replace. */
		if (tdb_update_hash(tdb, key, hash, dbuf) == 0)
			goto out;
		if (tdb->ecode == TDB_ERR_NOEXIST &&
		    flag == TDB_MODIFY) {
			/* if the record doesn't exist and we are in TDB_MODIFY mode then
			 we should fail the store */
			goto fail;
	}
	}
	/* reset the error code potentially set by the tdb_update() */
	tdb->ecode = TDB_SUCCESS;

	/* delete any existing record - if it doesn't exist we don't
           care.  Doing this first reduces fragmentation, and avoids
           coalescing with `allocated' block before it's updated. */
	if (flag != TDB_INSERT)
		tdb_delete_hash(tdb, key, hash);

	/* Copy key+value *before* allocating free space in case malloc
	   fails and we are left with a dead spot in the tdb. */

	if (!(p = (char *)malloc(key.dsize + dbuf.dsize))) {
		tdb->ecode = TDB_ERR_OOM;
		goto fail;
	}

	memcpy(p, key.dptr, key.dsize);
	if (dbuf.dsize)
		memcpy(p+key.dsize, dbuf.dptr, dbuf.dsize);

	/* we have to allocate some space */
	if (!(rec_ptr = tdb_allocate(tdb, key.dsize + dbuf.dsize, &rec)))
		goto fail;

	/* Read hash top into next ptr */
	if (ofs_read(tdb, TDB_HASH_TOP(hash), &rec.next) == -1)
		goto fail;

	rec.key_len = key.dsize;
	rec.data_len = dbuf.dsize;
	rec.full_hash = hash;
	rec.magic = TDB_MAGIC;

	/* write out and point the top of the hash chain at it */
	if (rec_write(tdb, rec_ptr, &rec) == -1
	    || tdb_write(tdb, rec_ptr+sizeof(rec), p, key.dsize+dbuf.dsize)==-1
	    || ofs_write(tdb, TDB_HASH_TOP(hash), &rec_ptr) == -1) {
		/* Need to tdb_unallocate() here */
		goto fail;
	}
 out:
	SAFE_FREE(p); 
	tdb_unlock(tdb, BUCKET(hash), F_WRLCK);
	return ret;
fail:
	ret = -1;
	goto out;
}

/* Attempt to append data to an entry in place - this only works if the new data size
   is <= the old data size and the key exists.
   on failure return -1. Record must be locked before calling.
*/
static int tdb_append_inplace(TDB_CONTEXT *tdb, TDB_DATA key, u32 hash, TDB_DATA new_dbuf)
{
	struct list_struct rec;
	tdb_off rec_ptr;

	/* find entry */
	if (!(rec_ptr = tdb_find(tdb, key, hash, &rec)))
		return -1;

	/* Append of 0 is always ok. */
	if (new_dbuf.dsize == 0)
		return 0;

	/* must be long enough for key, old data + new data and tailer */
	if (rec.rec_len < key.dsize + rec.data_len + new_dbuf.dsize + sizeof(tdb_off)) {
		/* No room. */
		tdb->ecode = TDB_SUCCESS; /* Not really an error */
		return -1;
	}

	if (tdb_write(tdb, rec_ptr + sizeof(rec) + rec.key_len + rec.data_len,
		      new_dbuf.dptr, new_dbuf.dsize) == -1)
		return -1;

	/* update size */
	rec.data_len += new_dbuf.dsize;
	return rec_write(tdb, rec_ptr, &rec);
}

/* Append to an entry. Create if not exist. */

int tdb_append(TDB_CONTEXT *tdb, TDB_DATA key, TDB_DATA new_dbuf)
{
	struct list_struct rec;
	u32 hash;
	tdb_off rec_ptr;
	char *p = NULL;
	int ret = 0;
	size_t new_data_size = 0;

	/* find which hash bucket it is in */
	hash = tdb->hash_fn(&key);
	if (tdb_lock(tdb, BUCKET(hash), F_WRLCK) == -1)
		return -1;

	/* first try in-place. */
	if (tdb_append_inplace(tdb, key, hash, new_dbuf) == 0)
		goto out;

	/* reset the error code potentially set by the tdb_append_inplace() */
	tdb->ecode = TDB_SUCCESS;

	/* find entry */
	if (!(rec_ptr = tdb_find(tdb, key, hash, &rec))) {
		if (tdb->ecode != TDB_ERR_NOEXIST)
			goto fail;

		/* Not found - create. */

		ret = tdb_store(tdb, key, new_dbuf, TDB_INSERT);
		goto out;
	}

	new_data_size = rec.data_len + new_dbuf.dsize;

	/* Copy key+old_value+value *before* allocating free space in case malloc
	   fails and we are left with a dead spot in the tdb. */

	if (!(p = (char *)malloc(key.dsize + new_data_size))) {
		tdb->ecode = TDB_ERR_OOM;
		goto fail;
	}

	/* Copy the key in place. */
	memcpy(p, key.dptr, key.dsize);

	/* Now read the old data into place. */
	if (rec.data_len &&
		tdb_read(tdb, rec_ptr + sizeof(rec) + rec.key_len, p + key.dsize, rec.data_len, 0) == -1)
			goto fail;

	/* Finally append the new data. */
	if (new_dbuf.dsize)
		memcpy(p+key.dsize+rec.data_len, new_dbuf.dptr, new_dbuf.dsize);

	/* delete any existing record - if it doesn't exist we don't
           care.  Doing this first reduces fragmentation, and avoids
           coalescing with `allocated' block before it's updated. */

	tdb_delete_hash(tdb, key, hash);

	if (!(rec_ptr = tdb_allocate(tdb, key.dsize + new_data_size, &rec)))
		goto fail;

	/* Read hash top into next ptr */
	if (ofs_read(tdb, TDB_HASH_TOP(hash), &rec.next) == -1)
		goto fail;

	rec.key_len = key.dsize;
	rec.data_len = new_data_size;
	rec.full_hash = hash;
	rec.magic = TDB_MAGIC;

	/* write out and point the top of the hash chain at it */
	if (rec_write(tdb, rec_ptr, &rec) == -1
	    || tdb_write(tdb, rec_ptr+sizeof(rec), p, key.dsize+new_data_size)==-1
	    || ofs_write(tdb, TDB_HASH_TOP(hash), &rec_ptr) == -1) {
		/* Need to tdb_unallocate() here */
		goto fail;
	}

 out:
	SAFE_FREE(p); 
	tdb_unlock(tdb, BUCKET(hash), F_WRLCK);
	return ret;

fail:
	ret = -1;
	goto out;
}

static int tdb_already_open(dev_t device,
			    ino_t ino)
{
	TDB_CONTEXT *i;
	
	for (i = tdbs; i; i = i->next) {
		if (i->device == device && i->inode == ino) {
			return 1;
		}
	}

	return 0;
}

/* This is based on the hash algorithm from gdbm */
static u32 default_tdb_hash(TDB_DATA *key)
{
	u32 value;	/* Used to compute the hash value.  */
	u32   i;	/* Used to cycle through random values. */

	/* Set the initial value from the key size. */
	for (value = 0x238F13AF * key->dsize, i=0; i < key->dsize; i++)
		value = (value + (key->dptr[i] << (i*5 % 24)));

	return (1103515243 * value + 12345);  
}

/* open the database, creating it if necessary 

   The open_flags and mode are passed straight to the open call on the
   database file. A flags value of O_WRONLY is invalid. The hash size
   is advisory, use zero for a default value.

   Return is NULL on error, in which case errno is also set.  Don't 
   try to call tdb_error or tdb_errname, just do strerror(errno).

   @param name may be NULL for internal databases. */
TDB_CONTEXT *tdb_open(const char *name, int hash_size, int tdb_flags,
		      int open_flags, mode_t mode)
{
	return tdb_open_ex(name, hash_size, tdb_flags, open_flags, mode, NULL, NULL);
}


TDB_CONTEXT *tdb_open_ex(const char *name, int hash_size, int tdb_flags,
			 int open_flags, mode_t mode,
			 tdb_log_func log_fn,
			 tdb_hash_func hash_fn)
{
	TDB_CONTEXT *tdb;
	struct stat st;
	int rev = 0, locked = 0;
	unsigned char *vp;
	u32 vertest;

	if (!(tdb = calloc(1, sizeof *tdb))) {
		/* Can't log this */
		errno = ENOMEM;
		goto fail;
	}
	tdb->fd = -1;
	tdb->name = NULL;
	tdb->map_ptr = NULL;
	tdb->flags = tdb_flags;
#ifdef _XBOX
	// for the xbox we use an internal database to save all samba stuff
	tdb->flags |= TDB_INTERNAL;
#endif
	tdb->open_flags = open_flags;
	tdb->log_fn = log_fn;
	tdb->hash_fn = hash_fn ? hash_fn : default_tdb_hash;

	if ((open_flags & O_ACCMODE) == O_WRONLY) {
		TDB_LOG((tdb, 0, "tdb_open_ex: can't open tdb %s write-only\n",
			 name));
		errno = EINVAL;
		goto fail;
	}
	
	if (hash_size == 0)
		hash_size = DEFAULT_HASH_SIZE;
	if ((open_flags & O_ACCMODE) == O_RDONLY) {
		tdb->read_only = 1;
		/* read only databases don't do locking or clear if first */
		tdb->flags |= TDB_NOLOCK;
		tdb->flags &= ~TDB_CLEAR_IF_FIRST;
	}

	/* internal databases don't mmap or lock, and start off cleared */
	if (tdb->flags & TDB_INTERNAL) {
		tdb->flags |= (TDB_NOLOCK | TDB_NOMMAP);
		tdb->flags &= ~TDB_CLEAR_IF_FIRST;
		if (tdb_new_database(tdb, hash_size) != 0) {
			TDB_LOG((tdb, 0, "tdb_open_ex: tdb_new_database failed!"));
			goto fail;
		}
		goto internal;
	}

	if ((tdb->fd = open(name, open_flags, mode)) == -1) {
		TDB_LOG((tdb, 5, "tdb_open_ex: could not open file %s: %s\n",
			 name, strerror(errno)));
		goto fail;	/* errno set by open(2) */
	}

	/* ensure there is only one process initialising at once */
	if (tdb_brlock(tdb, GLOBAL_LOCK, F_WRLCK, F_SETLKW, 0) == -1) {
		TDB_LOG((tdb, 0, "tdb_open_ex: failed to get global lock on %s: %s\n",
			 name, strerror(errno)));
		goto fail;	/* errno set by tdb_brlock */
	}

	/* we need to zero database if we are the only one with it open */
	if ((tdb_flags & TDB_CLEAR_IF_FIRST) &&
		(locked = (tdb_brlock(tdb, ACTIVE_LOCK, F_WRLCK, F_SETLK, 0) == 0))) {
		open_flags |= O_CREAT;
		if (ftruncate(tdb->fd, 0) == -1) {
			TDB_LOG((tdb, 0, "tdb_open_ex: "
				 "failed to truncate %s: %s\n",
				 name, strerror(errno)));
			goto fail; /* errno set by ftruncate */
		}
	}

	if (read(tdb->fd, &tdb->header, sizeof(tdb->header)) != sizeof(tdb->header)
	    || strcmp(tdb->header.magic_food, TDB_MAGIC_FOOD) != 0
	    || (tdb->header.version != TDB_VERSION
		&& !(rev = (tdb->header.version==TDB_BYTEREV(TDB_VERSION))))) {
		/* its not a valid database - possibly initialise it */
		if (!(open_flags & O_CREAT) || tdb_new_database(tdb, hash_size) == -1) {
			errno = EIO; /* ie bad format or something */
			goto fail;
		}
		rev = (tdb->flags & TDB_CONVERT);
	}
	vp = (unsigned char *)&tdb->header.version;
	vertest = (((u32)vp[0]) << 24) | (((u32)vp[1]) << 16) |
		  (((u32)vp[2]) << 8) | (u32)vp[3];
	tdb->flags |= (vertest==TDB_VERSION) ? TDB_BIGENDIAN : 0;
	if (!rev)
		tdb->flags &= ~TDB_CONVERT;
	else {
		tdb->flags |= TDB_CONVERT;
		convert(&tdb->header, sizeof(tdb->header));
	}
	if (fstat(tdb->fd, &st) == -1)
		goto fail;

	/* Is it already in the open list?  If so, fail. */
	if (tdb_already_open(st.st_dev, st.st_ino)) {
		TDB_LOG((tdb, 2, "tdb_open_ex: "
			 "%s (%d,%d) is already open in this process\n",
			 name, (int)st.st_dev, (int)st.st_ino));
		errno = EBUSY;
		goto fail;
	}

	if (!(tdb->name = (char *)strdup(name))) {
		errno = ENOMEM;
		goto fail;
	}

	tdb->map_size = st.st_size;
	tdb->device = st.st_dev;
	tdb->inode = st.st_ino;
	tdb->locked = calloc(tdb->header.hash_size+1, sizeof(tdb->locked[0]));
	if (!tdb->locked) {
		TDB_LOG((tdb, 2, "tdb_open_ex: "
			 "failed to allocate lock structure for %s\n",
			 name));
		errno = ENOMEM;
		goto fail;
	}
	tdb_mmap(tdb);
	if (locked) {
		if (!tdb->read_only)
			if (tdb_clear_spinlocks(tdb) != 0) {
				TDB_LOG((tdb, 0, "tdb_open_ex: "
				"failed to clear spinlock\n"));
				goto fail;
			}
		if (tdb_brlock(tdb, ACTIVE_LOCK, F_UNLCK, F_SETLK, 0) == -1) {
			TDB_LOG((tdb, 0, "tdb_open_ex: "
				 "failed to take ACTIVE_LOCK on %s: %s\n",
				 name, strerror(errno)));
			goto fail;
		}

	}

	/* We always need to do this if the CLEAR_IF_FIRST flag is set, even if
	   we didn't get the initial exclusive lock as we need to let all other
	   users know we're using it. */

	if (tdb_flags & TDB_CLEAR_IF_FIRST) {
		/* leave this lock in place to indicate it's in use */
		if (tdb_brlock(tdb, ACTIVE_LOCK, F_RDLCK, F_SETLKW, 0) == -1)
			goto fail;
	}


 internal:
	/* Internal (memory-only) databases skip all the code above to
	 * do with disk files, and resume here by releasing their
	 * global lock and hooking into the active list. */
	if (tdb_brlock(tdb, GLOBAL_LOCK, F_UNLCK, F_SETLKW, 0) == -1)
		goto fail;
	tdb->next = tdbs;
	tdbs = tdb;
	return tdb;

 fail:
	{ int save_errno = errno;

	if (!tdb)
		return NULL;
	
	if (tdb->map_ptr) {
		if (tdb->flags & TDB_INTERNAL)
			SAFE_FREE(tdb->map_ptr);
		else
			tdb_munmap(tdb);
	}
	SAFE_FREE(tdb->name);
	if (tdb->fd != -1)
		if (close(tdb->fd) != 0)
			TDB_LOG((tdb, 5, "tdb_open_ex: failed to close tdb->fd on error!\n"));
	SAFE_FREE(tdb->locked);
	SAFE_FREE(tdb);
	errno = save_errno;
	return NULL;
	}
}

/**
 * Close a database.
 *
 * @returns -1 for error; 0 for success.
 **/
int tdb_close(TDB_CONTEXT *tdb)
{
	TDB_CONTEXT **i;
	int ret = 0;

	if (tdb->map_ptr) {
		if (tdb->flags & TDB_INTERNAL)
			SAFE_FREE(tdb->map_ptr);
		else
			tdb_munmap(tdb);
	}
	SAFE_FREE(tdb->name);
	if (tdb->fd != -1)
		ret = close(tdb->fd);
	SAFE_FREE(tdb->locked);

	/* Remove from contexts list */
	for (i = &tdbs; *i; i = &(*i)->next) {
		if (*i == tdb) {
			*i = tdb->next;
			break;
		}
	}

	memset(tdb, 0, sizeof(*tdb));
	SAFE_FREE(tdb);

	return ret;
}

/* lock/unlock entire database */
int tdb_lockall(TDB_CONTEXT *tdb)
{
	u32 i;

	/* There are no locks on read-only dbs */
	if (tdb->read_only)
		return TDB_ERRCODE(TDB_ERR_LOCK, -1);
	for (i = 0; i < tdb->header.hash_size; i++) 
		if (tdb_lock(tdb, i, F_WRLCK))
			break;

	/* If error, release locks we have... */
	if (i < tdb->header.hash_size) {
		u32 j;

		for ( j = 0; j < i; j++)
			tdb_unlock(tdb, j, F_WRLCK);
		return TDB_ERRCODE(TDB_ERR_NOLOCK, -1);
	}

	return 0;
}
void tdb_unlockall(TDB_CONTEXT *tdb)
{
	u32 i;
	for (i=0; i < tdb->header.hash_size; i++)
		tdb_unlock(tdb, i, F_WRLCK);
}

/* lock/unlock one hash chain. This is meant to be used to reduce
   contention - it cannot guarantee how many records will be locked */
int tdb_chainlock(TDB_CONTEXT *tdb, TDB_DATA key)
{
	return tdb_lock(tdb, BUCKET(tdb->hash_fn(&key)), F_WRLCK);
}

int tdb_chainunlock(TDB_CONTEXT *tdb, TDB_DATA key)
{
	return tdb_unlock(tdb, BUCKET(tdb->hash_fn(&key)), F_WRLCK);
}

int tdb_chainlock_read(TDB_CONTEXT *tdb, TDB_DATA key)
{
	return tdb_lock(tdb, BUCKET(tdb->hash_fn(&key)), F_RDLCK);
}

int tdb_chainunlock_read(TDB_CONTEXT *tdb, TDB_DATA key)
{
	return tdb_unlock(tdb, BUCKET(tdb->hash_fn(&key)), F_RDLCK);
}


/* register a loging function */
void tdb_logging_function(TDB_CONTEXT *tdb, void (*fn)(TDB_CONTEXT *, int , const char *, ...))
{
	tdb->log_fn = fn;
}

/* reopen a tdb - this can be used after a fork to ensure that we have an independent
   seek pointer from our parent and to re-establish locks */
int tdb_reopen(TDB_CONTEXT *tdb)
{
	struct stat st;

	if (tdb->flags & TDB_INTERNAL)
		return 0; /* Nothing to do. */
	if (tdb_munmap(tdb) != 0) {
		TDB_LOG((tdb, 0, "tdb_reopen: munmap failed (%s)\n", strerror(errno)));
		goto fail;
	}
	if (close(tdb->fd) != 0)
		TDB_LOG((tdb, 0, "tdb_reopen: WARNING closing tdb->fd failed!\n"));
	tdb->fd = open(tdb->name, tdb->open_flags & ~(O_CREAT|O_TRUNC), 0);
	if (tdb->fd == -1) {
		TDB_LOG((tdb, 0, "tdb_reopen: open failed (%s)\n", strerror(errno)));
		goto fail;
	}
	if ((tdb->flags & TDB_CLEAR_IF_FIRST) &&
			(tdb_brlock(tdb, ACTIVE_LOCK, F_RDLCK, F_SETLKW, 0) == -1)) {
		TDB_LOG((tdb, 0, "tdb_reopen: failed to obtain active lock\n"));
		goto fail;
	}
	if (fstat(tdb->fd, &st) != 0) {
		TDB_LOG((tdb, 0, "tdb_reopen: fstat failed (%s)\n", strerror(errno)));
		goto fail;
	}
	if (st.st_ino != tdb->inode || st.st_dev != tdb->device) {
		TDB_LOG((tdb, 0, "tdb_reopen: file dev/inode has changed!\n"));
		goto fail;
	}
	tdb_mmap(tdb);

	return 0;

fail:
	tdb_close(tdb);
	return -1;
}

/* reopen all tdb's */
int tdb_reopen_all(int parent_longlived)
{
	TDB_CONTEXT *tdb;

	for (tdb=tdbs; tdb; tdb = tdb->next) {
		/*
		 * If the parent is longlived (ie. a
		 * parent daemon architecture), we know
		 * it will keep it's active lock on a
		 * tdb opened with CLEAR_IF_FIRST. Thus
		 * for child processes we don't have to
		 * add an active lock. This is essential
		 * to improve performance on systems that
		 * keep POSIX locks as a non-scalable data
		 * structure in the kernel.
		 */
		if (parent_longlived) {
			/* Ensure no clear-if-first. */
			tdb->flags &= ~TDB_CLEAR_IF_FIRST;
		}

		if (tdb_reopen(tdb) != 0)
			return -1;
	}

	return 0;
}

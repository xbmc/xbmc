/* 
   Unix SMB/CIFS implementation.
   Locking functions
   Copyright (C) Jeremy Allison 1992-2000
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Revision History:

   POSIX locking support. Jeremy Allison (jeremy@valinux.com), Apr. 2000.
*/

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_LOCKING

/*
 * The POSIX locking database handle.
 */

static TDB_CONTEXT *posix_lock_tdb;

/*
 * The pending close database handle.
 */

static TDB_CONTEXT *posix_pending_close_tdb;

/*
 * The data in POSIX lock records is an unsorted linear array of these
 * records.  It is unnecessary to store the count as tdb provides the
 * size of the record.
 */

struct posix_lock {
	int fd;
	SMB_OFF_T start;
	SMB_OFF_T size;
	int lock_type;
};

/*
 * The data in POSIX pending close records is an unsorted linear array of int
 * records.  It is unnecessary to store the count as tdb provides the
 * size of the record.
 */

/* The key used in both the POSIX databases. */

struct posix_lock_key {
	SMB_DEV_T device;
	SMB_INO_T inode;
}; 

/*******************************************************************
 Form a static locking key for a dev/inode pair.
******************************************************************/

static TDB_DATA locking_key(SMB_DEV_T dev, SMB_INO_T inode)
{
	static struct posix_lock_key key;
	TDB_DATA kbuf;

	memset(&key, '\0', sizeof(key));
	key.device = dev;
	key.inode = inode;
	kbuf.dptr = (char *)&key;
	kbuf.dsize = sizeof(key);
	return kbuf;
}

/*******************************************************************
 Convenience function to get a key from an fsp.
******************************************************************/

static TDB_DATA locking_key_fsp(files_struct *fsp)
{
	return locking_key(fsp->dev, fsp->inode);
}

/****************************************************************************
 Add an fd to the pending close tdb.
****************************************************************************/

static BOOL add_fd_to_close_entry(files_struct *fsp)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);
	TDB_DATA dbuf;

	dbuf.dptr = NULL;
	dbuf.dsize = 0;

	dbuf = tdb_fetch(posix_pending_close_tdb, kbuf);

	dbuf.dptr = SMB_REALLOC(dbuf.dptr, dbuf.dsize + sizeof(int));
	if (!dbuf.dptr) {
		DEBUG(0,("add_fd_to_close_entry: Realloc fail !\n"));
		return False;
	}

	memcpy(dbuf.dptr + dbuf.dsize, &fsp->fh->fd, sizeof(int));
	dbuf.dsize += sizeof(int);

	if (tdb_store(posix_pending_close_tdb, kbuf, dbuf, TDB_REPLACE) == -1) {
		DEBUG(0,("add_fd_to_close_entry: tdb_store fail !\n"));
	}

	SAFE_FREE(dbuf.dptr);
	return True;
}

/****************************************************************************
 Remove all fd entries for a specific dev/inode pair from the tdb.
****************************************************************************/

static void delete_close_entries(files_struct *fsp)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);

	if (tdb_delete(posix_pending_close_tdb, kbuf) == -1)
		DEBUG(0,("delete_close_entries: tdb_delete fail !\n"));
}

/****************************************************************************
 Get the array of POSIX pending close records for an open fsp. Caller must
 free. Returns number of entries.
****************************************************************************/

static size_t get_posix_pending_close_entries(files_struct *fsp, int **entries)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);
	TDB_DATA dbuf;
	size_t count = 0;

	*entries = NULL;
	dbuf.dptr = NULL;

	dbuf = tdb_fetch(posix_pending_close_tdb, kbuf);

	if (!dbuf.dptr) {
		return 0;
	}

	*entries = (int *)dbuf.dptr;
	count = (size_t)(dbuf.dsize / sizeof(int));

	return count;
}

/****************************************************************************
 Get the array of POSIX locks for an fsp. Caller must free. Returns
 number of entries.
****************************************************************************/

static size_t get_posix_lock_entries(files_struct *fsp, struct posix_lock **entries)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);
	TDB_DATA dbuf;
	size_t count = 0;

	*entries = NULL;

	dbuf.dptr = NULL;

	dbuf = tdb_fetch(posix_lock_tdb, kbuf);

	if (!dbuf.dptr) {
		return 0;
	}

	*entries = (struct posix_lock *)dbuf.dptr;
	count = (size_t)(dbuf.dsize / sizeof(struct posix_lock));

	return count;
}

/****************************************************************************
 Deal with pending closes needed by POSIX locking support.
 Note that posix_locking_close_file() is expected to have been called
 to delete all locks on this fsp before this function is called.
****************************************************************************/

int fd_close_posix(struct connection_struct *conn, files_struct *fsp)
{
	int saved_errno = 0;
	int ret;
	size_t count, i;
	struct posix_lock *entries = NULL;
	int *fd_array = NULL;
	BOOL locks_on_other_fds = False;

	if (!lp_posix_locking(SNUM(conn))) {
		/*
		 * No POSIX to worry about, just close.
		 */
		ret = SMB_VFS_CLOSE(fsp,fsp->fh->fd);
		fsp->fh->fd = -1;
		return ret;
	}

	/*
	 * Get the number of outstanding POSIX locks on this dev/inode pair.
	 */

	count = get_posix_lock_entries(fsp, &entries);

	/*
	 * Check if there are any outstanding locks belonging to
	 * other fd's. This should never be the case if posix_locking_close_file()
	 * has been called first, but it never hurts to be *sure*.
	 */

	for (i = 0; i < count; i++) {
		if (entries[i].fd != fsp->fh->fd) {
			locks_on_other_fds = True;
			break;
		}
	}

	if (locks_on_other_fds) {

		/*
		 * There are outstanding locks on this dev/inode pair on other fds.
		 * Add our fd to the pending close tdb and set fsp->fh->fd to -1.
		 */

		if (!add_fd_to_close_entry(fsp)) {
			SAFE_FREE(entries);
			return -1;
		}

		SAFE_FREE(entries);
		fsp->fh->fd = -1;
		return 0;
	}

	SAFE_FREE(entries);

	/*
	 * No outstanding POSIX locks. Get the pending close fd's
	 * from the tdb and close them all.
	 */

	count = get_posix_pending_close_entries(fsp, &fd_array);

	if (count) {
		DEBUG(10,("fd_close_posix: doing close on %u fd's.\n", (unsigned int)count ));

		for(i = 0; i < count; i++) {
			if (SMB_VFS_CLOSE(fsp,fd_array[i]) == -1) {
				saved_errno = errno;
			}
		}

		/*
		 * Delete all fd's stored in the tdb
		 * for this dev/inode pair.
		 */

		delete_close_entries(fsp);
	}

	SAFE_FREE(fd_array);

	/*
	 * Finally close the fd associated with this fsp.
	 */

	ret = SMB_VFS_CLOSE(fsp,fsp->fh->fd);

	if (saved_errno != 0) {
		errno = saved_errno;
		ret = -1;
	} 

	fsp->fh->fd = -1;

	return ret;
}

/****************************************************************************
 Debugging aid :-).
****************************************************************************/

static const char *posix_lock_type_name(int lock_type)
{
	return (lock_type == F_RDLCK) ? "READ" : "WRITE";
}

/****************************************************************************
 Delete a POSIX lock entry by index number. Used if the tdb add succeeds, but
 then the POSIX fcntl lock fails.
****************************************************************************/

static BOOL delete_posix_lock_entry_by_index(files_struct *fsp, size_t entry)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);
	TDB_DATA dbuf;
	struct posix_lock *locks;
	size_t count;

	dbuf.dptr = NULL;
	
	dbuf = tdb_fetch(posix_lock_tdb, kbuf);

	if (!dbuf.dptr) {
		DEBUG(10,("delete_posix_lock_entry_by_index: tdb_fetch failed !\n"));
		goto fail;
	}

	count = (size_t)(dbuf.dsize / sizeof(struct posix_lock));
	locks = (struct posix_lock *)dbuf.dptr;

	if (count == 1) {
		tdb_delete(posix_lock_tdb, kbuf);
	} else {
		if (entry < count-1) {
			memmove(&locks[entry], &locks[entry+1], sizeof(struct posix_lock)*((count-1) - entry));
		}
		dbuf.dsize -= sizeof(struct posix_lock);
		tdb_store(posix_lock_tdb, kbuf, dbuf, TDB_REPLACE);
	}

	SAFE_FREE(dbuf.dptr);

	return True;

 fail:

	SAFE_FREE(dbuf.dptr);
	return False;
}

/****************************************************************************
 Add an entry into the POSIX locking tdb. We return the index number of the
 added lock (used in case we need to delete *exactly* this entry). Returns
 False on fail, True on success.
****************************************************************************/

static BOOL add_posix_lock_entry(files_struct *fsp, SMB_OFF_T start, SMB_OFF_T size, int lock_type, size_t *pentry_num)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);
	TDB_DATA dbuf;
	struct posix_lock pl;

	dbuf.dptr = NULL;
	dbuf.dsize = 0;

	dbuf = tdb_fetch(posix_lock_tdb, kbuf);

	*pentry_num = (size_t)(dbuf.dsize / sizeof(struct posix_lock));

	/*
	 * Add new record.
	 */

	pl.fd = fsp->fh->fd;
	pl.start = start;
	pl.size = size;
	pl.lock_type = lock_type;

	dbuf.dptr = SMB_REALLOC(dbuf.dptr, dbuf.dsize + sizeof(struct posix_lock));
	if (!dbuf.dptr) {
		DEBUG(0,("add_posix_lock_entry: Realloc fail !\n"));
		goto fail;
	}

	memcpy(dbuf.dptr + dbuf.dsize, &pl, sizeof(struct posix_lock));
	dbuf.dsize += sizeof(struct posix_lock);

	if (tdb_store(posix_lock_tdb, kbuf, dbuf, TDB_REPLACE) == -1) {
		DEBUG(0,("add_posix_lock: Failed to add lock entry on file %s\n", fsp->fsp_name));
		goto fail;
	}

	SAFE_FREE(dbuf.dptr);

	DEBUG(10,("add_posix_lock: File %s: type = %s: start=%.0f size=%.0f: dev=%.0f inode=%.0f\n",
			fsp->fsp_name, posix_lock_type_name(lock_type), (double)start, (double)size,
			(double)fsp->dev, (double)fsp->inode ));

	return True;

 fail:

	SAFE_FREE(dbuf.dptr);
	return False;
}

/****************************************************************************
 Calculate if locks have any overlap at all.
****************************************************************************/

static BOOL does_lock_overlap(SMB_OFF_T start1, SMB_OFF_T size1, SMB_OFF_T start2, SMB_OFF_T size2)
{
	if (start1 >= start2 && start1 <= start2 + size2)
		return True;

	if (start1 < start2 && start1 + size1 > start2)
		return True;

	return False;
}

/****************************************************************************
 Delete an entry from the POSIX locking tdb. Returns a copy of the entry being
 deleted and the number of records that are overlapped by this one, or -1 on error.
****************************************************************************/

static int delete_posix_lock_entry(files_struct *fsp, SMB_OFF_T start, SMB_OFF_T size, struct posix_lock *pl)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);
	TDB_DATA dbuf;
	struct posix_lock *locks;
	size_t i, count;
	BOOL found = False;
	int num_overlapping_records = 0;

	dbuf.dptr = NULL;
	
	dbuf = tdb_fetch(posix_lock_tdb, kbuf);

	if (!dbuf.dptr) {
		DEBUG(10,("delete_posix_lock_entry: tdb_fetch failed !\n"));
		goto fail;
	}

	/* There are existing locks - find a match. */
	locks = (struct posix_lock *)dbuf.dptr;
	count = (size_t)(dbuf.dsize / sizeof(struct posix_lock));

	/*
	 * Search for and delete the first record that matches the
	 * unlock criteria.
	 */

	for (i=0; i<count; i++) { 
		struct posix_lock *entry = &locks[i];

		if (entry->fd == fsp->fh->fd &&
			entry->start == start &&
			entry->size == size) {

			/* Make a copy */
			*pl = *entry;

			/* Found it - delete it. */
			if (count == 1) {
				tdb_delete(posix_lock_tdb, kbuf);
			} else {
				if (i < count-1) {
					memmove(&locks[i], &locks[i+1], sizeof(struct posix_lock)*((count-1) - i));
				}
				dbuf.dsize -= sizeof(struct posix_lock);
				tdb_store(posix_lock_tdb, kbuf, dbuf, TDB_REPLACE);
			}
			count--;
			found = True;
			break;
		}
	}

	if (!found)
		goto fail;

	/*
	 * Count the number of entries that are
	 * overlapped by this unlock request.
	 */

	for (i = 0; i < count; i++) {
		struct posix_lock *entry = &locks[i];

		if (fsp->fh->fd == entry->fd &&
			does_lock_overlap( start, size, entry->start, entry->size))
				num_overlapping_records++;
	}

	DEBUG(10,("delete_posix_lock_entry: type = %s: start=%.0f size=%.0f, num_records = %d\n",
			posix_lock_type_name(pl->lock_type), (double)pl->start, (double)pl->size,
				(unsigned int)num_overlapping_records ));

	SAFE_FREE(dbuf.dptr);

	return num_overlapping_records;

 fail:

	SAFE_FREE(dbuf.dptr);
	return -1;
}

/****************************************************************************
 Utility function to map a lock type correctly depending on the open
 mode of a file.
****************************************************************************/

static int map_posix_lock_type( files_struct *fsp, enum brl_type lock_type)
{
	if((lock_type == WRITE_LOCK) && !fsp->can_write) {
		/*
		 * Many UNIX's cannot get a write lock on a file opened read-only.
		 * Win32 locking semantics allow this.
		 * Do the best we can and attempt a read-only lock.
		 */
		DEBUG(10,("map_posix_lock_type: Downgrading write lock to read due to read-only file.\n"));
		return F_RDLCK;
	}
#if 0
	/* We no longer open files write-only. */
	 else if((lock_type == READ_LOCK) && !fsp->can_read) {
		/*
		 * Ditto for read locks on write only files.
		 */
		DEBUG(10,("map_posix_lock_type: Changing read lock to write due to write-only file.\n"));
		return F_WRLCK;
	}
#endif

	/*
	 * This return should be the most normal, as we attempt
	 * to always open files read/write.
	 */

	return (lock_type == READ_LOCK) ? F_RDLCK : F_WRLCK;
}

/****************************************************************************
 Check to see if the given unsigned lock range is within the possible POSIX
 range. Modifies the given args to be in range if possible, just returns
 False if not.
****************************************************************************/

static BOOL posix_lock_in_range(SMB_OFF_T *offset_out, SMB_OFF_T *count_out,
				SMB_BIG_UINT u_offset, SMB_BIG_UINT u_count)
{
	SMB_OFF_T offset = (SMB_OFF_T)u_offset;
	SMB_OFF_T count = (SMB_OFF_T)u_count;

	/*
	 * For the type of system we are, attempt to
	 * find the maximum positive lock offset as an SMB_OFF_T.
	 */

#if defined(MAX_POSITIVE_LOCK_OFFSET) /* Some systems have arbitrary limits. */

	SMB_OFF_T max_positive_lock_offset = (MAX_POSITIVE_LOCK_OFFSET);

#elif defined(LARGE_SMB_OFF_T) && !defined(HAVE_BROKEN_FCNTL64_LOCKS)

	/*
	 * In this case SMB_OFF_T is 64 bits,
	 * and the underlying system can handle 64 bit signed locks.
	 */

	SMB_OFF_T mask2 = ((SMB_OFF_T)0x4) << (SMB_OFF_T_BITS-4);
	SMB_OFF_T mask = (mask2<<1);
	SMB_OFF_T max_positive_lock_offset = ~mask;

#else /* !LARGE_SMB_OFF_T || HAVE_BROKEN_FCNTL64_LOCKS */

	/*
	 * In this case either SMB_OFF_T is 32 bits,
	 * or the underlying system cannot handle 64 bit signed locks.
	 * All offsets & counts must be 2^31 or less.
	 */

	SMB_OFF_T max_positive_lock_offset = 0x7FFFFFFF;

#endif /* !LARGE_SMB_OFF_T || HAVE_BROKEN_FCNTL64_LOCKS */

	/*
	 * POSIX locks of length zero mean lock to end-of-file.
	 * Win32 locks of length zero are point probes. Ignore
	 * any Win32 locks of length zero. JRA.
	 */

	if (count == (SMB_OFF_T)0) {
		DEBUG(10,("posix_lock_in_range: count = 0, ignoring.\n"));
		return False;
	}

	/*
	 * If the given offset was > max_positive_lock_offset then we cannot map this at all
	 * ignore this lock.
	 */

	if (u_offset & ~((SMB_BIG_UINT)max_positive_lock_offset)) {
		DEBUG(10,("posix_lock_in_range: (offset = %.0f) offset > %.0f and we cannot handle this. Ignoring lock.\n",
				(double)u_offset, (double)((SMB_BIG_UINT)max_positive_lock_offset) ));
		return False;
	}

	/*
	 * We must truncate the count to less than max_positive_lock_offset.
	 */

	if (u_count & ~((SMB_BIG_UINT)max_positive_lock_offset))
		count = max_positive_lock_offset;

	/*
	 * Truncate count to end at max lock offset.
	 */

	if (offset + count < 0 || offset + count > max_positive_lock_offset)
		count = max_positive_lock_offset - offset;

	/*
	 * If we ate all the count, ignore this lock.
	 */

	if (count == 0) {
		DEBUG(10,("posix_lock_in_range: Count = 0. Ignoring lock u_offset = %.0f, u_count = %.0f\n",
				(double)u_offset, (double)u_count ));
		return False;
	}

	/*
	 * The mapping was successful.
	 */

	DEBUG(10,("posix_lock_in_range: offset_out = %.0f, count_out = %.0f\n",
			(double)offset, (double)count ));

	*offset_out = offset;
	*count_out = count;
	
	return True;
}

/****************************************************************************
 Actual function that does POSIX locks. Copes with 64 -> 32 bit cruft and
 broken NFS implementations.
****************************************************************************/

static BOOL posix_fcntl_lock(files_struct *fsp, int op, SMB_OFF_T offset, SMB_OFF_T count, int type)
{
	BOOL ret;

	DEBUG(8,("posix_fcntl_lock %d %d %.0f %.0f %d\n",fsp->fh->fd,op,(double)offset,(double)count,type));

	ret = SMB_VFS_LOCK(fsp,fsp->fh->fd,op,offset,count,type);

	if (!ret && ((errno == EFBIG) || (errno == ENOLCK) || (errno ==  EINVAL))) {

		DEBUG(0,("posix_fcntl_lock: WARNING: lock request at offset %.0f, length %.0f returned\n",
					(double)offset,(double)count));
		DEBUG(0,("an %s error. This can happen when using 64 bit lock offsets\n", strerror(errno)));
		DEBUG(0,("on 32 bit NFS mounted file systems.\n"));

		/*
		 * If the offset is > 0x7FFFFFFF then this will cause problems on
		 * 32 bit NFS mounted filesystems. Just ignore it.
		 */

		if (offset & ~((SMB_OFF_T)0x7fffffff)) {
			DEBUG(0,("Offset greater than 31 bits. Returning success.\n"));
			return True;
		}

		if (count & ~((SMB_OFF_T)0x7fffffff)) {
			/* 32 bit NFS file system, retry with smaller offset */
			DEBUG(0,("Count greater than 31 bits - retrying with 31 bit truncated length.\n"));
			errno = 0;
			count &= 0x7fffffff;
			ret = SMB_VFS_LOCK(fsp,fsp->fh->fd,op,offset,count,type);
		}
	}

	DEBUG(8,("posix_fcntl_lock: Lock call %s\n", ret ? "successful" : "failed"));
	return ret;
}

/****************************************************************************
 Actual function that gets POSIX locks. Copes with 64 -> 32 bit cruft and
 broken NFS implementations.
****************************************************************************/

static BOOL posix_fcntl_getlock(files_struct *fsp, SMB_OFF_T *poffset, SMB_OFF_T *pcount, int *ptype)
{
	pid_t pid;
	BOOL ret;

	DEBUG(8,("posix_fcntl_getlock %d %.0f %.0f %d\n",
		fsp->fh->fd,(double)*poffset,(double)*pcount,*ptype));

	ret = SMB_VFS_GETLOCK(fsp,fsp->fh->fd,poffset,pcount,ptype,&pid);

	if (!ret && ((errno == EFBIG) || (errno == ENOLCK) || (errno ==  EINVAL))) {

		DEBUG(0,("posix_fcntl_getlock: WARNING: lock request at offset %.0f, length %.0f returned\n",
					(double)*poffset,(double)*pcount));
		DEBUG(0,("an %s error. This can happen when using 64 bit lock offsets\n", strerror(errno)));
		DEBUG(0,("on 32 bit NFS mounted file systems.\n"));

		/*
		 * If the offset is > 0x7FFFFFFF then this will cause problems on
		 * 32 bit NFS mounted filesystems. Just ignore it.
		 */

		if (*poffset & ~((SMB_OFF_T)0x7fffffff)) {
			DEBUG(0,("Offset greater than 31 bits. Returning success.\n"));
			return True;
		}

		if (*pcount & ~((SMB_OFF_T)0x7fffffff)) {
			/* 32 bit NFS file system, retry with smaller offset */
			DEBUG(0,("Count greater than 31 bits - retrying with 31 bit truncated length.\n"));
			errno = 0;
			*pcount &= 0x7fffffff;
			ret = SMB_VFS_GETLOCK(fsp,fsp->fh->fd,poffset,pcount,ptype,&pid);
		}
	}

	DEBUG(8,("posix_fcntl_getlock: Lock query call %s\n", ret ? "successful" : "failed"));
	return ret;
}


/****************************************************************************
 POSIX function to see if a file region is locked. Returns True if the
 region is locked, False otherwise.
****************************************************************************/

BOOL is_posix_locked(files_struct *fsp,
			SMB_BIG_UINT *pu_offset,
			SMB_BIG_UINT *pu_count,
			enum brl_type *plock_type,
			enum brl_flavour lock_flav)
{
	SMB_OFF_T offset;
	SMB_OFF_T count;
	int posix_lock_type = map_posix_lock_type(fsp,*plock_type);

	DEBUG(10,("is_posix_locked: File %s, offset = %.0f, count = %.0f, type = %s\n",
		fsp->fsp_name, (double)*pu_offset, (double)*pu_count, posix_lock_type_name(*plock_type) ));

	/*
	 * If the requested lock won't fit in the POSIX range, we will
	 * never set it, so presume it is not locked.
	 */

	if(!posix_lock_in_range(&offset, &count, *pu_offset, *pu_count)) {
		return False;
	}

	if (!posix_fcntl_getlock(fsp,&offset,&count,&posix_lock_type)) {
		return False;
	}

	if (posix_lock_type == F_UNLCK) {
		return False;
	}

	if (lock_flav == POSIX_LOCK) {
		/* Only POSIX lock queries need to know the details. */
		*pu_offset = (SMB_BIG_UINT)offset;
		*pu_count = (SMB_BIG_UINT)count;
		*plock_type = (posix_lock_type == F_RDLCK) ? READ_LOCK : WRITE_LOCK;
	}
	return True;
}

/*
 * Structure used when splitting a lock range
 * into a POSIX lock range. Doubly linked list.
 */

struct lock_list {
	struct lock_list *next;
	struct lock_list *prev;
	SMB_OFF_T start;
	SMB_OFF_T size;
};

/****************************************************************************
 Create a list of lock ranges that don't overlap a given range. Used in calculating
 POSIX locks and unlocks. This is a difficult function that requires ASCII art to
 understand it :-).
****************************************************************************/

static struct lock_list *posix_lock_list(TALLOC_CTX *ctx, struct lock_list *lhead, files_struct *fsp)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);
	TDB_DATA dbuf;
	struct posix_lock *locks;
	size_t num_locks, i;

	dbuf.dptr = NULL;

	dbuf = tdb_fetch(posix_lock_tdb, kbuf);

	if (!dbuf.dptr)
		return lhead;
	
	locks = (struct posix_lock *)dbuf.dptr;
	num_locks = (size_t)(dbuf.dsize / sizeof(struct posix_lock));

	/*
	 * Check the current lock list on this dev/inode pair.
	 * Quit if the list is deleted.
	 */

	DEBUG(10,("posix_lock_list: curr: start=%.0f,size=%.0f\n",
		(double)lhead->start, (double)lhead->size ));

	for (i=0; i<num_locks && lhead; i++) {

		struct posix_lock *lock = &locks[i];
		struct lock_list *l_curr;

		/*
		 * Walk the lock list, checking for overlaps. Note that
		 * the lock list can expand within this loop if the current
		 * range being examined needs to be split.
		 */

		for (l_curr = lhead; l_curr;) {

			DEBUG(10,("posix_lock_list: lock: fd=%d: start=%.0f,size=%.0f:type=%s", lock->fd,
				(double)lock->start, (double)lock->size, posix_lock_type_name(lock->lock_type) ));

			if ( (l_curr->start >= (lock->start + lock->size)) ||
				 (lock->start >= (l_curr->start + l_curr->size))) {

				/* No overlap with this lock - leave this range alone. */
/*********************************************
                                             +---------+
                                             | l_curr  |
                                             +---------+
                                +-------+
                                | lock  |
                                +-------+
OR....
             +---------+
             |  l_curr |
             +---------+
**********************************************/

				DEBUG(10,("no overlap case.\n" ));

				l_curr = l_curr->next;

			} else if ( (l_curr->start >= lock->start) &&
						(l_curr->start + l_curr->size <= lock->start + lock->size) ) {

				/*
				 * This unlock is completely overlapped by this existing lock range
				 * and thus should have no effect (not be unlocked). Delete it from the list.
				 */
/*********************************************
                +---------+
                |  l_curr |
                +---------+
        +---------------------------+
        |       lock                |
        +---------------------------+
**********************************************/
				/* Save the next pointer */
				struct lock_list *ul_next = l_curr->next;

				DEBUG(10,("delete case.\n" ));

				DLIST_REMOVE(lhead, l_curr);
				if(lhead == NULL)
					break; /* No more list... */

				l_curr = ul_next;
				
			} else if ( (l_curr->start >= lock->start) &&
						(l_curr->start < lock->start + lock->size) &&
						(l_curr->start + l_curr->size > lock->start + lock->size) ) {

				/*
				 * This unlock overlaps the existing lock range at the high end.
				 * Truncate by moving start to existing range end and reducing size.
				 */
/*********************************************
                +---------------+
                |  l_curr       |
                +---------------+
        +---------------+
        |    lock       |
        +---------------+
BECOMES....
                        +-------+
                        | l_curr|
                        +-------+
**********************************************/

				l_curr->size = (l_curr->start + l_curr->size) - (lock->start + lock->size);
				l_curr->start = lock->start + lock->size;

				DEBUG(10,("truncate high case: start=%.0f,size=%.0f\n",
								(double)l_curr->start, (double)l_curr->size ));

				l_curr = l_curr->next;

			} else if ( (l_curr->start < lock->start) &&
						(l_curr->start + l_curr->size > lock->start) &&
						(l_curr->start + l_curr->size <= lock->start + lock->size) ) {

				/*
				 * This unlock overlaps the existing lock range at the low end.
				 * Truncate by reducing size.
				 */
/*********************************************
   +---------------+
   |  l_curr       |
   +---------------+
           +---------------+
           |    lock       |
           +---------------+
BECOMES....
   +-------+
   | l_curr|
   +-------+
**********************************************/

				l_curr->size = lock->start - l_curr->start;

				DEBUG(10,("truncate low case: start=%.0f,size=%.0f\n",
								(double)l_curr->start, (double)l_curr->size ));

				l_curr = l_curr->next;
		
			} else if ( (l_curr->start < lock->start) &&
						(l_curr->start + l_curr->size > lock->start + lock->size) ) {
				/*
				 * Worst case scenario. Unlock request completely overlaps an existing
				 * lock range. Split the request into two, push the new (upper) request
				 * into the dlink list, and continue with the entry after ul_new (as we
				 * know that ul_new will not overlap with this lock).
				 */
/*********************************************
        +---------------------------+
        |        l_curr             |
        +---------------------------+
                +---------+
                | lock    |
                +---------+
BECOMES.....
        +-------+         +---------+
        | l_curr|         | l_new   |
        +-------+         +---------+
**********************************************/
				struct lock_list *l_new = TALLOC_P(ctx, struct lock_list);

				if(l_new == NULL) {
					DEBUG(0,("posix_lock_list: talloc fail.\n"));
					return NULL; /* The talloc_destroy takes care of cleanup. */
				}

				ZERO_STRUCTP(l_new);
				l_new->start = lock->start + lock->size;
				l_new->size = l_curr->start + l_curr->size - l_new->start;

				/* Truncate the l_curr. */
				l_curr->size = lock->start - l_curr->start;

				DEBUG(10,("split case: curr: start=%.0f,size=%.0f \
new: start=%.0f,size=%.0f\n", (double)l_curr->start, (double)l_curr->size,
								(double)l_new->start, (double)l_new->size ));

				/*
				 * Add into the dlink list after the l_curr point - NOT at lhead. 
				 * Note we can't use DLINK_ADD here as this inserts at the head of the given list.
				 */

				l_new->prev = l_curr;
				l_new->next = l_curr->next;
				l_curr->next = l_new;

				/* And move after the link we added. */
				l_curr = l_new->next;

			} else {

				/*
				 * This logic case should never happen. Ensure this is the
				 * case by forcing an abort.... Remove in production.
				 */
				pstring msg;

				slprintf(msg, sizeof(msg)-1, "logic flaw in cases: l_curr: start = %.0f, size = %.0f : \
lock: start = %.0f, size = %.0f\n", (double)l_curr->start, (double)l_curr->size, (double)lock->start, (double)lock->size );

				smb_panic(msg);
			}
		} /* end for ( l_curr = lhead; l_curr;) */
	} /* end for (i=0; i<num_locks && ul_head; i++) */

	SAFE_FREE(dbuf.dptr);
	
	return lhead;
}

/****************************************************************************
 POSIX function to acquire a lock. Returns True if the
 lock could be granted, False if not.
 TODO -- Fix POSIX lock flavour semantics.
****************************************************************************/

BOOL set_posix_lock(files_struct *fsp,
			SMB_BIG_UINT u_offset,
			SMB_BIG_UINT u_count,
			enum brl_type lock_type,
			enum brl_flavour lock_flav)
{
	SMB_OFF_T offset;
	SMB_OFF_T count;
	BOOL ret = True;
	size_t entry_num = 0;
	size_t lock_count;
	TALLOC_CTX *l_ctx = NULL;
	struct lock_list *llist = NULL;
	struct lock_list *ll = NULL;
	int posix_lock_type = map_posix_lock_type(fsp,lock_type);

	DEBUG(5,("set_posix_lock: File %s, offset = %.0f, count = %.0f, type = %s\n",
			fsp->fsp_name, (double)u_offset, (double)u_count, posix_lock_type_name(lock_type) ));

	/*
	 * If the requested lock won't fit in the POSIX range, we will
	 * pretend it was successful.
	 */

	if(!posix_lock_in_range(&offset, &count, u_offset, u_count))
		return True;

	/*
	 * Windows is very strange. It allows read locks to be overlayed
	 * (even over a write lock), but leaves the write lock in force until the first
	 * unlock. It also reference counts the locks. This means the following sequence :
	 *
	 * process1                                      process2
	 * ------------------------------------------------------------------------
	 * WRITE LOCK : start = 2, len = 10
	 *                                            READ LOCK: start =0, len = 10 - FAIL
	 * READ LOCK : start = 0, len = 14 
	 *                                            READ LOCK: start =0, len = 10 - FAIL
	 * UNLOCK : start = 2, len = 10
	 *                                            READ LOCK: start =0, len = 10 - OK
	 *
	 * Under POSIX, the same sequence in steps 1 and 2 would not be reference counted, but
	 * would leave a single read lock over the 0-14 region. In order to
	 * re-create Windows semantics mapped to POSIX locks, we create multiple TDB
	 * entries, one for each overlayed lock request. We are guarenteed by the brlock
	 * semantics that if a write lock is added, then it will be first in the array.
	 */
	
	if ((l_ctx = talloc_init("set_posix_lock")) == NULL) {
		DEBUG(0,("set_posix_lock: unable to init talloc context.\n"));
		return True; /* Not a fatal error. */
	}

	if ((ll = TALLOC_P(l_ctx, struct lock_list)) == NULL) {
		DEBUG(0,("set_posix_lock: unable to talloc unlock list.\n"));
		talloc_destroy(l_ctx);
		return True; /* Not a fatal error. */
	}

	/*
	 * Create the initial list entry containing the
	 * lock we want to add.
	 */

	ZERO_STRUCTP(ll);
	ll->start = offset;
	ll->size = count;

	DLIST_ADD(llist, ll);

	/*
	 * The following call calculates if there are any
	 * overlapping locks held by this process on
	 * fd's open on the same file and splits this list
	 * into a list of lock ranges that do not overlap with existing
	 * POSIX locks.
	 */

	llist = posix_lock_list(l_ctx, llist, fsp);

	/*
	 * Now we have the list of ranges to lock it is safe to add the
	 * entry into the POSIX lock tdb. We take note of the entry we
	 * added here in case we have to remove it on POSIX lock fail.
	 */

	if (!add_posix_lock_entry(fsp,offset,count,posix_lock_type,&entry_num)) {
		DEBUG(0,("set_posix_lock: Unable to create posix lock entry !\n"));
		talloc_destroy(l_ctx);
		return False;
	}

	/*
	 * Add the POSIX locks on the list of ranges returned.
	 * As the lock is supposed to be added atomically, we need to
	 * back out all the locks if any one of these calls fail.
	 */

	for (lock_count = 0, ll = llist; ll; ll = ll->next, lock_count++) {
		offset = ll->start;
		count = ll->size;

		DEBUG(5,("set_posix_lock: Real lock: Type = %s: offset = %.0f, count = %.0f\n",
			posix_lock_type_name(posix_lock_type), (double)offset, (double)count ));

		if (!posix_fcntl_lock(fsp,SMB_F_SETLK,offset,count,posix_lock_type)) {
			DEBUG(5,("set_posix_lock: Lock fail !: Type = %s: offset = %.0f, count = %.0f. Errno = %s\n",
				posix_lock_type_name(posix_lock_type), (double)offset, (double)count, strerror(errno) ));
			ret = False;
			break;
		}
	}

	if (!ret) {

		/*
		 * Back out all the POSIX locks we have on fail.
		 */

		for (ll = llist; lock_count; ll = ll->next, lock_count--) {
			offset = ll->start;
			count = ll->size;

			DEBUG(5,("set_posix_lock: Backing out locks: Type = %s: offset = %.0f, count = %.0f\n",
				posix_lock_type_name(posix_lock_type), (double)offset, (double)count ));

			posix_fcntl_lock(fsp,SMB_F_SETLK,offset,count,F_UNLCK);
		}

		/*
		 * Remove the tdb entry for this lock.
		 */

		delete_posix_lock_entry_by_index(fsp,entry_num);
	}

	talloc_destroy(l_ctx);
	return ret;
}

/****************************************************************************
 POSIX function to release a lock. Returns True if the
 lock could be released, False if not.
****************************************************************************/

BOOL release_posix_lock(files_struct *fsp, SMB_BIG_UINT u_offset, SMB_BIG_UINT u_count)
{
	SMB_OFF_T offset;
	SMB_OFF_T count;
	BOOL ret = True;
	TALLOC_CTX *ul_ctx = NULL;
	struct lock_list *ulist = NULL;
	struct lock_list *ul = NULL;
	struct posix_lock deleted_lock;
	int num_overlapped_entries;

	DEBUG(5,("release_posix_lock: File %s, offset = %.0f, count = %.0f\n",
		fsp->fsp_name, (double)u_offset, (double)u_count ));

	/*
	 * If the requested lock won't fit in the POSIX range, we will
	 * pretend it was successful.
	 */

	if(!posix_lock_in_range(&offset, &count, u_offset, u_count))
		return True;

	/*
	 * We treat this as one unlock request for POSIX accounting purposes even
	 * if it may later be split into multiple smaller POSIX unlock ranges.
	 * num_overlapped_entries is the number of existing locks that have any
	 * overlap with this unlock request.
	 */ 

	num_overlapped_entries = delete_posix_lock_entry(fsp, offset, count, &deleted_lock);

	if (num_overlapped_entries == -1) {
		smb_panic("release_posix_lock: unable find entry to delete !\n");
	}

	/*
	 * If num_overlapped_entries is > 0, and the lock_type we just deleted from the tdb was
	 * a POSIX write lock, then before doing the unlock we need to downgrade
	 * the POSIX lock to a read lock. This allows any overlapping read locks
	 * to be atomically maintained.
	 */

	if (num_overlapped_entries > 0 && deleted_lock.lock_type == F_WRLCK) {
		if (!posix_fcntl_lock(fsp,SMB_F_SETLK,offset,count,F_RDLCK)) {
			DEBUG(0,("release_posix_lock: downgrade of lock failed with error %s !\n", strerror(errno) ));
			return False;
		}
	}

	if ((ul_ctx = talloc_init("release_posix_lock")) == NULL) {
		DEBUG(0,("release_posix_lock: unable to init talloc context.\n"));
		return True; /* Not a fatal error. */
	}

	if ((ul = TALLOC_P(ul_ctx, struct lock_list)) == NULL) {
		DEBUG(0,("release_posix_lock: unable to talloc unlock list.\n"));
		talloc_destroy(ul_ctx);
		return True; /* Not a fatal error. */
	}

	/*
	 * Create the initial list entry containing the
	 * lock we want to remove.
	 */

	ZERO_STRUCTP(ul);
	ul->start = offset;
	ul->size = count;

	DLIST_ADD(ulist, ul);

	/*
	 * The following call calculates if there are any
	 * overlapping locks held by this process on
	 * fd's open on the same file and creates a
	 * list of unlock ranges that will allow
	 * POSIX lock ranges to remain on the file whilst the
	 * unlocks are performed.
	 */

	ulist = posix_lock_list(ul_ctx, ulist, fsp);

	/*
	 * Release the POSIX locks on the list of ranges returned.
	 */

	for(; ulist; ulist = ulist->next) {
		offset = ulist->start;
		count = ulist->size;

		DEBUG(5,("release_posix_lock: Real unlock: offset = %.0f, count = %.0f\n",
			(double)offset, (double)count ));

		if (!posix_fcntl_lock(fsp,SMB_F_SETLK,offset,count,F_UNLCK))
			ret = False;
	}

	talloc_destroy(ul_ctx);

	return ret;
}

/****************************************************************************
 Remove all lock entries for a specific dev/inode pair from the tdb.
****************************************************************************/

static void delete_posix_lock_entries(files_struct *fsp)
{
	TDB_DATA kbuf = locking_key_fsp(fsp);

	if (tdb_delete(posix_lock_tdb, kbuf) == -1)
		DEBUG(0,("delete_close_entries: tdb_delete fail !\n"));
}

/****************************************************************************
 Debug function.
****************************************************************************/

static void dump_entry(struct posix_lock *pl)
{
	DEBUG(10,("entry: start=%.0f, size=%.0f, type=%d, fd=%i\n",
		(double)pl->start, (double)pl->size, (int)pl->lock_type, pl->fd ));
}

/****************************************************************************
 Remove any locks on this fd. Called from file_close().
****************************************************************************/

void posix_locking_close_file(files_struct *fsp)
{
	struct posix_lock *entries = NULL;
	size_t count, i;

	/*
	 * Optimization for the common case where we are the only
	 * opener of a file. If all fd entries are our own, we don't
	 * need to explicitly release all the locks via the POSIX functions,
	 * we can just remove all the entries in the tdb and allow the
	 * close to remove the real locks.
	 */

	count = get_posix_lock_entries(fsp, &entries);

	if (count == 0) {
		DEBUG(10,("posix_locking_close_file: file %s has no outstanding locks.\n", fsp->fsp_name ));
		return;
	}

	for (i = 0; i < count; i++) {
		if (entries[i].fd != fsp->fh->fd )
			break;

		dump_entry(&entries[i]);
	}

	if (i == count) {
		/* All locks are ours. */
		DEBUG(10,("posix_locking_close_file: file %s has %u outstanding locks, but all on one fd.\n", 
			fsp->fsp_name, (unsigned int)count ));
		SAFE_FREE(entries);
		delete_posix_lock_entries(fsp);
		return;
	}

	/*
	 * Difficult case. We need to delete all our locks, whilst leaving
	 * all other POSIX locks in place.
	 */

	for (i = 0; i < count; i++) {
		struct posix_lock *pl = &entries[i];
		if (pl->fd == fsp->fh->fd)
			release_posix_lock(fsp, (SMB_BIG_UINT)pl->start, (SMB_BIG_UINT)pl->size );
	}
	SAFE_FREE(entries);
}

/*******************************************************************
 Create the in-memory POSIX lock databases.
********************************************************************/

BOOL posix_locking_init(int read_only)
{
	if (posix_lock_tdb && posix_pending_close_tdb)
		return True;
	
	if (!posix_lock_tdb)
		posix_lock_tdb = tdb_open_log(NULL, 0, TDB_INTERNAL,
					  read_only?O_RDONLY:(O_RDWR|O_CREAT), 0644);
	if (!posix_lock_tdb) {
		DEBUG(0,("Failed to open POSIX byte range locking database.\n"));
		return False;
	}
	if (!posix_pending_close_tdb)
		posix_pending_close_tdb = tdb_open_log(NULL, 0, TDB_INTERNAL,
						   read_only?O_RDONLY:(O_RDWR|O_CREAT), 0644);
	if (!posix_pending_close_tdb) {
		DEBUG(0,("Failed to open POSIX pending close database.\n"));
		return False;
	}

	return True;
}

/*******************************************************************
 Delete the in-memory POSIX lock databases.
********************************************************************/

BOOL posix_locking_end(void)
{
    if (posix_lock_tdb && tdb_close(posix_lock_tdb) != 0)
		return False;
    if (posix_pending_close_tdb && tdb_close(posix_pending_close_tdb) != 0)
		return False;
	return True;
}

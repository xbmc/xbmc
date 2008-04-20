/* 
   Unix SMB/CIFS implementation.
   byte range locking code
   Updated to handle range splits/merges.

   Copyright (C) Andrew Tridgell 1992-2000
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
*/

/* This module implements a tdb based byte range locking service,
   replacing the fcntl() based byte range locking previously
   used. This allows us to provide the same semantics as NT */

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_LOCKING

#define ZERO_ZERO 0

/* This contains elements that differentiate locks. The smbpid is a
   client supplied pid, and is essentially the locking context for
   this client */

struct lock_context {
	uint16 smbpid;
	uint16 tid;
	struct process_id pid;
};

/* The data in brlock records is an unsorted linear array of these
   records.  It is unnecessary to store the count as tdb provides the
   size of the record */

struct lock_struct {
	struct lock_context context;
	br_off start;
	br_off size;
	int fnum;
	enum brl_type lock_type;
	enum brl_flavour lock_flav;
};

/* The open brlock.tdb database. */

static TDB_CONTEXT *tdb;

/****************************************************************************
 Debug info at level 10 for lock struct.
****************************************************************************/

static void print_lock_struct(unsigned int i, struct lock_struct *pls)
{
	DEBUG(10,("[%u]: smbpid = %u, tid = %u, pid = %u, ",
			i,
			(unsigned int)pls->context.smbpid,
			(unsigned int)pls->context.tid,
			(unsigned int)procid_to_pid(&pls->context.pid) ));
	
	DEBUG(10,("start = %.0f, size = %.0f, fnum = %d, %s %s\n",
		(double)pls->start,
		(double)pls->size,
		pls->fnum,
		lock_type_name(pls->lock_type),
		lock_flav_name(pls->lock_flav) ));
}

/****************************************************************************
 See if two locking contexts are equal.
****************************************************************************/

static BOOL brl_same_context(const struct lock_context *ctx1, 
			     const struct lock_context *ctx2)
{
	return (procid_equal(&ctx1->pid, &ctx2->pid) &&
		(ctx1->smbpid == ctx2->smbpid) &&
		(ctx1->tid == ctx2->tid));
}

/****************************************************************************
 See if lck1 and lck2 overlap.
****************************************************************************/

static BOOL brl_overlap(const struct lock_struct *lck1,
                        const struct lock_struct *lck2)
{
	/* this extra check is not redundent - it copes with locks
	   that go beyond the end of 64 bit file space */
	if (lck1->size != 0 &&
	    lck1->start == lck2->start &&
	    lck1->size == lck2->size) {
		return True;
	}

	if (lck1->start >= (lck2->start+lck2->size) ||
	    lck2->start >= (lck1->start+lck1->size)) {
		return False;
	}
	return True;
}

/****************************************************************************
 See if lock2 can be added when lock1 is in place.
****************************************************************************/

static BOOL brl_conflict(const struct lock_struct *lck1, 
			 const struct lock_struct *lck2)
{
	/* Ignore PENDING locks. */
	if (lck1->lock_type == PENDING_LOCK || lck2->lock_type == PENDING_LOCK )
		return False;

	/* Read locks never conflict. */
	if (lck1->lock_type == READ_LOCK && lck2->lock_type == READ_LOCK) {
		return False;
	}

	if (brl_same_context(&lck1->context, &lck2->context) &&
	    lck2->lock_type == READ_LOCK && lck1->fnum == lck2->fnum) {
		return False;
	}

	return brl_overlap(lck1, lck2);
} 

/****************************************************************************
 See if lock2 can be added when lock1 is in place - when both locks are POSIX
 flavour. POSIX locks ignore fnum - they only care about dev/ino which we
 know already match.
****************************************************************************/

static BOOL brl_conflict_posix(const struct lock_struct *lck1, 
			 	const struct lock_struct *lck2)
{
#if defined(DEVELOPER)
	SMB_ASSERT(lck1->lock_flav == POSIX_LOCK);
	SMB_ASSERT(lck2->lock_flav == POSIX_LOCK);
#endif

	/* Ignore PENDING locks. */
	if (lck1->lock_type == PENDING_LOCK || lck2->lock_type == PENDING_LOCK )
		return False;

	/* Read locks never conflict. */
	if (lck1->lock_type == READ_LOCK && lck2->lock_type == READ_LOCK) {
		return False;
	}

	/* Locks on the same context con't conflict. Ignore fnum. */
	if (brl_same_context(&lck1->context, &lck2->context)) {
		return False;
	}

	/* One is read, the other write, or the context is different,
	   do they overlap ? */
	return brl_overlap(lck1, lck2);
} 

#if ZERO_ZERO
static BOOL brl_conflict1(const struct lock_struct *lck1, 
			 const struct lock_struct *lck2)
{
	if (lck1->lock_type == PENDING_LOCK || lck2->lock_type == PENDING_LOCK )
		return False;

	if (lck1->lock_type == READ_LOCK && lck2->lock_type == READ_LOCK) {
		return False;
	}

	if (brl_same_context(&lck1->context, &lck2->context) &&
	    lck2->lock_type == READ_LOCK && lck1->fnum == lck2->fnum) {
		return False;
	}

	if (lck2->start == 0 && lck2->size == 0 && lck1->size != 0) {
		return True;
	}

	if (lck1->start >= (lck2->start + lck2->size) ||
	    lck2->start >= (lck1->start + lck1->size)) {
		return False;
	}
	    
	return True;
} 
#endif

/****************************************************************************
 Check to see if this lock conflicts, but ignore our own locks on the
 same fnum only. This is the read/write lock check code path.
 This is never used in the POSIX lock case.
****************************************************************************/

static BOOL brl_conflict_other(const struct lock_struct *lck1, const struct lock_struct *lck2)
{
	if (lck1->lock_type == PENDING_LOCK || lck2->lock_type == PENDING_LOCK )
		return False;

	if (lck1->lock_type == READ_LOCK && lck2->lock_type == READ_LOCK) 
		return False;

	/* POSIX flavour locks never conflict here - this is only called
	   in the read/write path. */

	if (lck1->lock_flav == POSIX_LOCK && lck2->lock_flav == POSIX_LOCK)
		return False;

	/*
	 * Incoming WRITE locks conflict with existing READ locks even
	 * if the context is the same. JRA. See LOCKTEST7 in smbtorture.
	 */

	if (!(lck2->lock_type == WRITE_LOCK && lck1->lock_type == READ_LOCK)) {
		if (brl_same_context(&lck1->context, &lck2->context) &&
					lck1->fnum == lck2->fnum)
			return False;
	}

	return brl_overlap(lck1, lck2);
} 

/****************************************************************************
 Amazingly enough, w2k3 "remembers" whether the last lock failure
 is the same as this one and changes its error code. I wonder if any
 app depends on this ?
****************************************************************************/

static NTSTATUS brl_lock_failed(const struct lock_struct *lock)
{
	static struct lock_struct last_lock_failure;

	if (brl_same_context(&lock->context, &last_lock_failure.context) &&
			lock->fnum == last_lock_failure.fnum &&
			lock->start == last_lock_failure.start &&
			lock->size == last_lock_failure.size) {
		return NT_STATUS_FILE_LOCK_CONFLICT;
	}
	last_lock_failure = *lock;
	if (lock->start >= 0xEF000000 &&
			(lock->start >> 63) == 0) {
		/* amazing the little things you learn with a test
		   suite. Locks beyond this offset (as a 64 bit
		   number!) always generate the conflict error code,
		   unless the top bit is set */
		return NT_STATUS_FILE_LOCK_CONFLICT;
	}
	return NT_STATUS_LOCK_NOT_GRANTED;
}

/****************************************************************************
 Open up the brlock.tdb database.
****************************************************************************/

void brl_init(int read_only)
{
	if (tdb) {
		return;
	}
	tdb = tdb_open_log(lock_path("brlock.tdb"),
			lp_open_files_db_hash_size(),
			TDB_DEFAULT|(read_only?0x0:TDB_CLEAR_IF_FIRST),
			read_only?O_RDONLY:(O_RDWR|O_CREAT), 0644 );
	if (!tdb) {
		DEBUG(0,("Failed to open byte range locking database %s\n",
			lock_path("brlock.tdb")));
		return;
	}
}

/****************************************************************************
 Close down the brlock.tdb database.
****************************************************************************/

void brl_shutdown(int read_only)
{
	if (!tdb) {
		return;
	}
	tdb_close(tdb);
}

#if ZERO_ZERO
/****************************************************************************
 Compare two locks for sorting.
****************************************************************************/

static int lock_compare(const struct lock_struct *lck1, 
			 const struct lock_struct *lck2)
{
	if (lck1->start != lck2->start) {
		return (lck1->start - lck2->start);
	}
	if (lck2->size != lck1->size) {
		return ((int)lck1->size - (int)lck2->size);
	}
	return 0;
}
#endif

/****************************************************************************
 Lock a range of bytes - Windows lock semantics.
****************************************************************************/

static NTSTATUS brl_lock_windows(struct byte_range_lock *br_lck,
			const struct lock_struct *plock,
			BOOL *my_lock_ctx)
{
	unsigned int i;
	files_struct *fsp = br_lck->fsp;
	struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;

	for (i=0; i < br_lck->num_locks; i++) {
		/* Do any Windows or POSIX locks conflict ? */
		if (brl_conflict(&locks[i], plock)) {
			NTSTATUS status = brl_lock_failed(plock);;
			/* Did we block ourselves ? */
			if (brl_same_context(&locks[i].context, &plock->context)) {
				*my_lock_ctx = True;
			}
			return status;
		}
#if ZERO_ZERO
		if (plock->start == 0 && plock->size == 0 && 
				locks[i].size == 0) {
			break;
		}
#endif
	}

	/* We can get the Windows lock, now see if it needs to
	   be mapped into a lower level POSIX one, and if so can
	   we get it ? We tell the lower lock layer about the
	   lock type so it can cope with the difference between
	   Windows "stacking" locks and POSIX "flat" ones. */

	if ((plock->lock_type != PENDING_LOCK) && lp_posix_locking(SNUM(fsp->conn))) {
		if (!set_posix_lock(fsp, plock->start, plock->size, plock->lock_type, WINDOWS_LOCK)) {
			if (errno == EACCES || errno == EAGAIN) {
				return NT_STATUS_FILE_LOCK_CONFLICT;
			} else {
				return map_nt_error_from_unix(errno);
			}
		}
	}

	/* no conflicts - add it to the list of locks */
	locks = (struct lock_struct *)SMB_REALLOC(locks, (br_lck->num_locks + 1) * sizeof(*locks));
	if (!locks) {
		return NT_STATUS_NO_MEMORY;
	}

	memcpy(&locks[br_lck->num_locks], plock, sizeof(struct lock_struct));
	br_lck->num_locks += 1;
	br_lck->lock_data = (void *)locks;
	br_lck->modified = True;

	return NT_STATUS_OK;
}

/****************************************************************************
 Cope with POSIX range splits and merges.
****************************************************************************/

static unsigned int brlock_posix_split_merge(struct lock_struct *lck_arr,
						const struct lock_struct *ex,
						const struct lock_struct *plock,
						BOOL *lock_was_added)
{
	BOOL lock_types_differ = (ex->lock_type != plock->lock_type);

	/* We can't merge non-conflicting locks on different context - ignore fnum. */

	if (!brl_same_context(&ex->context, &plock->context)) {
		/* Just copy. */
		memcpy(&lck_arr[0], ex, sizeof(struct lock_struct));
		return 1;
	}

	/* We now know we have the same context. */

	/* Did we overlap ? */

/*********************************************
                                             +---------+
                                             | ex      |
                                             +---------+
                                +-------+
                                | plock |
                                +-------+
OR....
             +---------+
             |  ex     |
             +---------+
**********************************************/

	if ( (ex->start >= (plock->start + plock->size)) ||
			(plock->start >= (ex->start + ex->size))) {
		/* No overlap with this lock - copy existing. */
		memcpy(&lck_arr[0], ex, sizeof(struct lock_struct));
		return 1;
	}

/*********************************************
                +---------+
                |  ex     |
                +---------+
        +---------------------------+
        |       plock               | -> replace with plock.
        +---------------------------+
**********************************************/

	if ( (ex->start >= plock->start) &&
			(ex->start + ex->size <= plock->start + plock->size) ) {
		memcpy(&lck_arr[0], plock, sizeof(struct lock_struct));
		*lock_was_added = True;
		return 1;
	}

/*********************************************
                +---------------+
                |  ex           |
                +---------------+
        +---------------+
        |   plock       |
        +---------------+
BECOMES....
        +---------------+-------+
        |   plock       | ex    | - different lock types.
        +---------------+-------+
OR....
        +-----------------------+
        |   ex                  | - same lock type.
        +-----------------------+
**********************************************/

	if ( (ex->start >= plock->start) &&
				(ex->start < plock->start + plock->size) &&
				(ex->start + ex->size > plock->start + plock->size) ) {

		*lock_was_added = True;

		/* If the lock types are the same, we merge, if different, we
		   add the new lock before the old. */

		if (lock_types_differ) {
			/* Add new. */
			memcpy(&lck_arr[0], plock, sizeof(struct lock_struct));
			memcpy(&lck_arr[1], ex, sizeof(struct lock_struct));
			/* Adjust existing start and size. */
			lck_arr[1].start = plock->start + plock->size;
			lck_arr[1].size = (ex->start + ex->size) - (plock->start + plock->size);
			return 2;
		} else {
			/* Merge. */
			memcpy(&lck_arr[0], plock, sizeof(struct lock_struct));
			/* Set new start and size. */
			lck_arr[0].start = plock->start;
			lck_arr[0].size = (ex->start + ex->size) - plock->start;
			return 1;
		}
	}

/*********************************************
   +---------------+
   |  ex           |
   +---------------+
           +---------------+
           |   plock       |
           +---------------+
BECOMES....
   +-------+---------------+
   | ex    |   plock       | - different lock types
   +-------+---------------+

OR
   +-----------------------+
   | ex                    | - same lock type.
   +-----------------------+

**********************************************/

	if ( (ex->start < plock->start) &&
			(ex->start + ex->size > plock->start) &&
			(ex->start + ex->size <= plock->start + plock->size) ) {

		*lock_was_added = True;

		/* If the lock types are the same, we merge, if different, we
		   add the new lock after the old. */

		if (lock_types_differ) {
			memcpy(&lck_arr[0], ex, sizeof(struct lock_struct));
			memcpy(&lck_arr[1], plock, sizeof(struct lock_struct));
			/* Adjust existing size. */
			lck_arr[0].size = plock->start - ex->start;
			return 2;
		} else {
			/* Merge. */
			memcpy(&lck_arr[0], ex, sizeof(struct lock_struct));
			/* Adjust existing size. */
			lck_arr[0].size = (plock->start + plock->size) - ex->start;
			return 1;
		}
	}

/*********************************************
        +---------------------------+
        |        ex                 |
        +---------------------------+
                +---------+
                |  plock  |
                +---------+
BECOMES.....
        +-------+---------+---------+
        | ex    |  plock  | ex      | - different lock types.
        +-------+---------+---------+
OR
        +---------------------------+
        |        ex                 | - same lock type.
        +---------------------------+
**********************************************/

	if ( (ex->start < plock->start) && (ex->start + ex->size > plock->start + plock->size) ) {
		*lock_was_added = True;

		if (lock_types_differ) {

			/* We have to split ex into two locks here. */

			memcpy(&lck_arr[0], ex, sizeof(struct lock_struct));
			memcpy(&lck_arr[1], plock, sizeof(struct lock_struct));
			memcpy(&lck_arr[2], ex, sizeof(struct lock_struct));

			/* Adjust first existing size. */
			lck_arr[0].size = plock->start - ex->start;

			/* Adjust second existing start and size. */
			lck_arr[2].start = plock->start + plock->size;
			lck_arr[2].size = (ex->start + ex->size) - (plock->start + plock->size);
			return 3;
		} else {
			/* Just eat plock. */
			memcpy(&lck_arr[0], ex, sizeof(struct lock_struct));
			return 1;
		}
	}

	/* Never get here. */
	smb_panic("brlock_posix_split_merge\n");
	/* Notreached. */
	abort();
	/* Keep some compilers happy. */
	return 0;
}

/****************************************************************************
 Lock a range of bytes - POSIX lock semantics.
 We must cope with range splits and merges.
****************************************************************************/

static NTSTATUS brl_lock_posix(struct byte_range_lock *br_lck,
			const struct lock_struct *plock,
			BOOL *my_lock_ctx)
{
	unsigned int i, count;
	struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;
	struct lock_struct *tp;
	BOOL lock_was_added = False;

	/* No zero-zero locks for POSIX. */
	if (plock->start == 0 && plock->size == 0) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* Don't allow 64-bit lock wrap. */
	if (plock->start + plock->size < plock->start ||
			plock->start + plock->size < plock->size) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	/* The worst case scenario here is we have to split an
	   existing POSIX lock range into two, and add our lock,
	   so we need at most 2 more entries. */

	tp = SMB_MALLOC_ARRAY(struct lock_struct, (br_lck->num_locks + 2));
	if (!tp) {
		return NT_STATUS_NO_MEMORY;
	}
	
	count = 0;
	for (i=0; i < br_lck->num_locks; i++) {
		if (locks[i].lock_flav == WINDOWS_LOCK) {
			/* Do any Windows flavour locks conflict ? */
			if (brl_conflict(&locks[i], plock)) {
				/* Did we block ourselves ? */
				if (brl_same_context(&locks[i].context, &plock->context)) {
					*my_lock_ctx = True;
				}
				/* No games with error messages. */
				SAFE_FREE(tp);
				return NT_STATUS_FILE_LOCK_CONFLICT;
			}
			/* Just copy the Windows lock into the new array. */
			memcpy(&tp[count], &locks[i], sizeof(struct lock_struct));
			count++;
		} else {
			/* POSIX conflict semantics are different. */
			if (brl_conflict_posix(&locks[i], plock)) {
				/* Can't block ourselves with POSIX locks. */
				/* No games with error messages. */
				SAFE_FREE(tp);
				return NT_STATUS_FILE_LOCK_CONFLICT;
			}

			/* Work out overlaps. */
			count += brlock_posix_split_merge(&tp[count], &locks[i], plock, &lock_was_added);
		}
	}

	/* We can get the POSIX lock, now see if it needs to
	   be mapped into a lower level POSIX one, and if so can
	   we get it ? We well the lower lock layer about the
	   lock type so it can cope with the difference between
	   Windows "stacking" locks and POSIX "flat" ones. */

#if 0
	/* FIXME - this call doesn't work correctly yet for POSIX locks... */

	if ((plock->lock_type != PENDING_LOCK) && lp_posix_locking(SNUM(fsp->conn))) {
		files_struct *fsp = br_lck->fsp;

		if (!set_posix_lock(fsp, plock->start, plock->size, plock->lock_type, POSIX_LOCK)) {
			if (errno == EACCES || errno == EAGAIN) {
				SAFE_FREE(tp);
				return NT_STATUS_FILE_LOCK_CONFLICT;
			} else {
				SAFE_FREE(tp);
				return map_nt_error_from_unix(errno);
			}
		}
	}
#endif

	if (!lock_was_added) {
		memcpy(&tp[count], plock, sizeof(struct lock_struct));
		count++;
	}

	/* Realloc so we don't leak entries per lock call. */
	tp = (struct lock_struct *)SMB_REALLOC(tp, count * sizeof(*locks));
	if (!tp) {
		return NT_STATUS_NO_MEMORY;
	}
	br_lck->num_locks = count;
	SAFE_FREE(br_lck->lock_data);
	br_lck->lock_data = (void *)tp;
	br_lck->modified = True;
	return NT_STATUS_OK;
}

/****************************************************************************
 Lock a range of bytes.
****************************************************************************/

NTSTATUS brl_lock(struct byte_range_lock *br_lck,
		uint16 smbpid,
		struct process_id pid,
		br_off start,
		br_off size, 
		enum brl_type lock_type,
		enum brl_flavour lock_flav,
		BOOL *my_lock_ctx)
{
	NTSTATUS ret;
	struct lock_struct lock;

	*my_lock_ctx = False;

#if !ZERO_ZERO
	if (start == 0 && size == 0) {
		DEBUG(0,("client sent 0/0 lock - please report this\n"));
	}
#endif

	lock.context.smbpid = smbpid;
	lock.context.pid = pid;
	lock.context.tid = br_lck->fsp->conn->cnum;
	lock.start = start;
	lock.size = size;
	lock.fnum = br_lck->fsp->fnum;
	lock.lock_type = lock_type;
	lock.lock_flav = lock_flav;

	if (lock_flav == WINDOWS_LOCK) {
		ret = brl_lock_windows(br_lck, &lock, my_lock_ctx);
	} else {
		ret = brl_lock_posix(br_lck, &lock, my_lock_ctx);
	}

#if ZERO_ZERO
	/* sort the lock list */
	qsort(br_lck->lock_data, (size_t)br_lck->num_locks, sizeof(lock), lock_compare);
#endif

	return ret;
}

/****************************************************************************
 Check if an unlock overlaps a pending lock.
****************************************************************************/

static BOOL brl_pending_overlap(const struct lock_struct *lock, const struct lock_struct *pend_lock)
{
	if ((lock->start <= pend_lock->start) && (lock->start + lock->size > pend_lock->start))
		return True;
	if ((lock->start >= pend_lock->start) && (lock->start <= pend_lock->start + pend_lock->size))
		return True;
	return False;
}

/****************************************************************************
 Unlock a range of bytes - Windows semantics.
****************************************************************************/

static BOOL brl_unlock_windows(struct byte_range_lock *br_lck, const struct lock_struct *plock)
{
	unsigned int i, j;
	struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;

#if ZERO_ZERO
	for (i = 0; i < br_lck->num_locks; i++) {
		struct lock_struct *lock = &locks[i];

		if (lock->lock_type == WRITE_LOCK &&
		    brl_same_context(&lock->context, &plock->context) &&
		    lock->fnum == plock->fnum &&
		    lock->lock_flav == WINDOWS_LOCK &&
		    lock->start == plock->start &&
		    lock->size == plock->size) {

			/* found it - delete it */
			if (i < br_lck->num_locks - 1) {
				memmove(&locks[i], &locks[i+1], 
					sizeof(*locks)*((br_lck->num_locks-1) - i));
			}

			br_lck->num_locks -= 1;
			br_lck->modified = True;
			return True;
		}
	}
#endif

	for (i = 0; i < br_lck->num_locks; i++) {
		struct lock_struct *lock = &locks[i];

		/* Only remove our own locks that match in start, size, and flavour. */
		if (brl_same_context(&lock->context, &plock->context) &&
					lock->fnum == plock->fnum &&
					lock->lock_flav == WINDOWS_LOCK &&
					lock->start == plock->start &&
					lock->size == plock->size ) {
			break;
		}
	}

	if (i == br_lck->num_locks) {
		/* we didn't find it */
		return False;
	}

	/* Unlock any POSIX regions. */
	if(lp_posix_locking(br_lck->fsp->conn->cnum)) {
		release_posix_lock(br_lck->fsp, plock->start, plock->size);
	}

	/* Send unlock messages to any pending waiters that overlap. */
	for (j=0; j < br_lck->num_locks; j++) {
		struct lock_struct *pend_lock = &locks[j];

		/* Ignore non-pending locks. */
		if (pend_lock->lock_type != PENDING_LOCK) {
			continue;
		}

		/* We could send specific lock info here... */
		if (brl_pending_overlap(plock, pend_lock)) {
			DEBUG(10,("brl_unlock: sending unlock message to pid %s\n",
				procid_str_static(&pend_lock->context.pid )));

			become_root();
			message_send_pid(pend_lock->context.pid,
					MSG_SMB_UNLOCK,
					NULL, 0, True);
			unbecome_root();
		}
	}

	/* Actually delete the lock. */
	if (i < br_lck->num_locks - 1) {
		memmove(&locks[i], &locks[i+1], 
			sizeof(*locks)*((br_lck->num_locks-1) - i));
	}

	br_lck->num_locks -= 1;
	br_lck->modified = True;
	return True;
}

/****************************************************************************
 Unlock a range of bytes - POSIX semantics.
****************************************************************************/

static BOOL brl_unlock_posix(struct byte_range_lock *br_lck, const struct lock_struct *plock)
{
	unsigned int i, j, count;
	struct lock_struct *tp;
	struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;
	BOOL overlap_found = False;

	/* No zero-zero locks for POSIX. */
	if (plock->start == 0 && plock->size == 0) {
		return False;
	}

	/* Don't allow 64-bit lock wrap. */
	if (plock->start + plock->size < plock->start ||
			plock->start + plock->size < plock->size) {
		DEBUG(10,("brl_unlock_posix: lock wrap\n"));
		return False;
	}

	/* The worst case scenario here is we have to split an
	   existing POSIX lock range into two, so we need at most
	   1 more entry. */

	tp = SMB_MALLOC_ARRAY(struct lock_struct, (br_lck->num_locks + 1));
	if (!tp) {
		DEBUG(10,("brl_unlock_posix: malloc fail\n"));
		return False;
	}

	count = 0;
	for (i = 0; i < br_lck->num_locks; i++) {
		struct lock_struct *lock = &locks[i];
		struct lock_struct tmp_lock[3];
		BOOL lock_was_added = False;
		unsigned int tmp_count;


		/* Only remove our own locks - ignore fnum. */
		if (lock->lock_type == PENDING_LOCK ||
				!brl_same_context(&lock->context, &plock->context)) {
			memcpy(&tp[count], lock, sizeof(struct lock_struct));
			count++;
			continue;
		}

		/* Work out overlaps. */
		tmp_count = brlock_posix_split_merge(&tmp_lock[0], &locks[i], plock, &lock_was_added);

		if (tmp_count == 1) {
			/* Ether the locks didn't overlap, or the unlock completely
			   overlapped this lock. If it didn't overlap, then there's
			   no change in the locks. */
			if (tmp_lock[0].lock_type != UNLOCK_LOCK) {
				SMB_ASSERT(tmp_lock[0].lock_type == locks[i].lock_type);
				/* No change in this lock. */
				memcpy(&tp[count], &tmp_lock[0], sizeof(struct lock_struct));
				count++;
			} else {
				SMB_ASSERT(tmp_lock[0].lock_type == UNLOCK_LOCK);
				overlap_found = True;
			}
			continue;
		} else if (tmp_count == 2) {
			/* The unlock overlapped an existing lock. Copy the truncated
			   lock into the lock array. */
			if (tmp_lock[0].lock_type != UNLOCK_LOCK) {
				SMB_ASSERT(tmp_lock[0].lock_type == locks[i].lock_type);
				SMB_ASSERT(tmp_lock[1].lock_type == UNLOCK_LOCK);
				memcpy(&tp[count], &tmp_lock[0], sizeof(struct lock_struct));
			} else {
				SMB_ASSERT(tmp_lock[0].lock_type == UNLOCK_LOCK);
				SMB_ASSERT(tmp_lock[1].lock_type == locks[i].lock_type);
				memcpy(&tp[count], &tmp_lock[1], sizeof(struct lock_struct));
			}
			count++;
			overlap_found = True;
			continue;
		} else {
			/* tmp_count == 3 - (we split a lock range in two). */
			SMB_ASSERT(tmp_lock[0].lock_type == locks[i].lock_type);
			SMB_ASSERT(tmp_lock[1].lock_type == UNLOCK_LOCK);
			SMB_ASSERT(tmp_lock[2].lock_type == locks[i].lock_type);

			memcpy(&tp[count], &tmp_lock[0], sizeof(struct lock_struct));
			count++;
			memcpy(&tp[count], &tmp_lock[2], sizeof(struct lock_struct));
			count++;
			overlap_found = True;
			/* Optimisation... */
			/* We know we're finished here as we can't overlap any
			   more POSIX locks. Copy the rest of the lock array. */
			if (i < br_lck->num_locks - 1) {
				memcpy(&tp[count], &locks[i+1], 
					sizeof(*locks)*((br_lck->num_locks-1) - i));
				count += ((br_lck->num_locks-1) - i);
			}
			break;
		}
	}

	if (!overlap_found) {
		/* Just ignore - no change. */
		SAFE_FREE(tp);
		DEBUG(10,("brl_unlock_posix: No overlap - unlocked.\n"));
		return True;
	}

#if 0
	/* FIXME - this call doesn't work correctly yet for POSIX locks... */

	/* Unlock any POSIX regions. */
	if(lp_posix_locking(br_lck->fsp->conn->cnum)) {
		release_posix_lock(br_lck->fsp, plock->start, plock->size);
	}
#endif

	/* Realloc so we don't leak entries per unlock call. */
	if (count) {
		tp = (struct lock_struct *)SMB_REALLOC(tp, count * sizeof(*locks));
		if (!tp) {
			DEBUG(10,("brl_unlock_posix: realloc fail\n"));
			return False;
		}
	} else {
		/* We deleted the last lock. */
		SAFE_FREE(tp);
		tp = NULL;
	}

	br_lck->num_locks = count;
	SAFE_FREE(br_lck->lock_data);
	locks = br_lck->lock_data = (void *)tp;
	br_lck->modified = True;

	/* Send unlock messages to any pending waiters that overlap. */

	for (j=0; j < br_lck->num_locks; j++) {
		struct lock_struct *pend_lock = &locks[j];

		/* Ignore non-pending locks. */
		if (pend_lock->lock_type != PENDING_LOCK) {
			continue;
		}

		/* We could send specific lock info here... */
		if (brl_pending_overlap(plock, pend_lock)) {
			DEBUG(10,("brl_unlock: sending unlock message to pid %s\n",
				procid_str_static(&pend_lock->context.pid )));

			become_root();
			message_send_pid(pend_lock->context.pid,
					MSG_SMB_UNLOCK,
					NULL, 0, True);
			unbecome_root();
		}
	}

	return True;
}

/****************************************************************************
 Unlock a range of bytes.
****************************************************************************/

BOOL brl_unlock(struct byte_range_lock *br_lck,
		uint16 smbpid,
		struct process_id pid,
		br_off start,
		br_off size,
		enum brl_flavour lock_flav)
{
	struct lock_struct lock;

	lock.context.smbpid = smbpid;
	lock.context.pid = pid;
	lock.context.tid = br_lck->fsp->conn->cnum;
	lock.start = start;
	lock.size = size;
	lock.fnum = br_lck->fsp->fnum;
	lock.lock_type = UNLOCK_LOCK;
	lock.lock_flav = lock_flav;

	if (lock_flav == WINDOWS_LOCK) {
		return brl_unlock_windows(br_lck, &lock);
	} else {
		return brl_unlock_posix(br_lck, &lock);
	}
}

/****************************************************************************
 Test if we could add a lock if we wanted to.
 Returns True if the region required is currently unlocked, False if locked.
****************************************************************************/

BOOL brl_locktest(struct byte_range_lock *br_lck,
		uint16 smbpid,
		struct process_id pid,
		br_off start,
		br_off size, 
		enum brl_type lock_type,
		enum brl_flavour lock_flav)
{
	BOOL ret = True;
	unsigned int i;
	struct lock_struct lock;
	struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;
	files_struct *fsp = br_lck->fsp;

	lock.context.smbpid = smbpid;
	lock.context.pid = pid;
	lock.context.tid = br_lck->fsp->conn->cnum;
	lock.start = start;
	lock.size = size;
	lock.fnum = fsp->fnum;
	lock.lock_type = lock_type;
	lock.lock_flav = lock_flav;

	/* Make sure existing locks don't conflict */
	for (i=0; i < br_lck->num_locks; i++) {
		/*
		 * Our own locks don't conflict.
		 */
		if (brl_conflict_other(&locks[i], &lock)) {
			return False;
		}
	}

	/*
	 * There is no lock held by an SMB daemon, check to
	 * see if there is a POSIX lock from a UNIX or NFS process.
	 * This only conflicts with Windows locks, not POSIX locks.
	 */

	if(lp_posix_locking(fsp->conn->cnum) && (lock_flav == WINDOWS_LOCK)) {
		ret = is_posix_locked(fsp, &start, &size, &lock_type, WINDOWS_LOCK);

		DEBUG(10,("brl_locktest: posix start=%.0f len=%.0f %s for fnum %d file %s\n",
			(double)start, (double)size, ret ? "locked" : "unlocked",
			fsp->fnum, fsp->fsp_name ));

		/* We need to return the inverse of is_posix_locked. */
		ret = !ret;
        }

	/* no conflicts - we could have added it */
	return ret;
}

/****************************************************************************
 Query for existing locks.
****************************************************************************/

NTSTATUS brl_lockquery(struct byte_range_lock *br_lck,
		uint16 *psmbpid,
		struct process_id pid,
		br_off *pstart,
		br_off *psize, 
		enum brl_type *plock_type,
		enum brl_flavour lock_flav)
{
	unsigned int i;
	struct lock_struct lock;
	struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;
	files_struct *fsp = br_lck->fsp;

	lock.context.smbpid = *psmbpid;
	lock.context.pid = pid;
	lock.context.tid = br_lck->fsp->conn->cnum;
	lock.start = *pstart;
	lock.size = *psize;
	lock.fnum = fsp->fnum;
	lock.lock_type = *plock_type;
	lock.lock_flav = lock_flav;

	/* Make sure existing locks don't conflict */
	for (i=0; i < br_lck->num_locks; i++) {
		struct lock_struct *exlock = &locks[i];
		BOOL conflict = False;

		if (exlock->lock_flav == WINDOWS_LOCK) {
			conflict = brl_conflict(exlock, &lock);
		} else {	
			conflict = brl_conflict_posix(exlock, &lock);
		}

		if (conflict) {
			*psmbpid = exlock->context.smbpid;
        		*pstart = exlock->start;
		        *psize = exlock->size;
        		*plock_type = exlock->lock_type;
			return NT_STATUS_LOCK_NOT_GRANTED;
		}
	}

	/*
	 * There is no lock held by an SMB daemon, check to
	 * see if there is a POSIX lock from a UNIX or NFS process.
	 */

	if(lp_posix_locking(fsp->conn->cnum)) {
		BOOL ret = is_posix_locked(fsp, pstart, psize, plock_type, POSIX_LOCK);

		DEBUG(10,("brl_lockquery: posix start=%.0f len=%.0f %s for fnum %d file %s\n",
			(double)*pstart, (double)*psize, ret ? "locked" : "unlocked",
			fsp->fnum, fsp->fsp_name ));

		if (ret) {
			/* Hmmm. No clue what to set smbpid to - use -1. */
			*psmbpid = 0xFFFF;
			return NT_STATUS_LOCK_NOT_GRANTED;
		}
        }

	return NT_STATUS_OK;
}


/****************************************************************************
 Remove a particular pending lock.
****************************************************************************/

BOOL brl_remove_pending_lock(struct byte_range_lock *br_lck,
		uint16 smbpid,
		struct process_id pid,
		br_off start,
		br_off size,
		enum brl_flavour lock_flav)
{
	unsigned int i;
	struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;
	struct lock_context context;

	context.smbpid = smbpid;
	context.pid = pid;
	context.tid = br_lck->fsp->conn->cnum;

	for (i = 0; i < br_lck->num_locks; i++) {
		struct lock_struct *lock = &locks[i];

		/* For pending locks we *always* care about the fnum. */
		if (brl_same_context(&lock->context, &context) &&
				lock->fnum == br_lck->fsp->fnum &&
				lock->lock_type == PENDING_LOCK &&
				lock->lock_flav == lock_flav &&
				lock->start == start &&
				lock->size == size) {
			break;
		}
	}

	if (i == br_lck->num_locks) {
		/* Didn't find it. */
		return False;
	}

	if (i < br_lck->num_locks - 1) {
		/* Found this particular pending lock - delete it */
		memmove(&locks[i], &locks[i+1], 
			sizeof(*locks)*((br_lck->num_locks-1) - i));
	}

	br_lck->num_locks -= 1;
	br_lck->modified = True;
	return True;
}


/****************************************************************************
 Remove any locks associated with a open file.
****************************************************************************/

void brl_close_fnum(struct byte_range_lock *br_lck, struct process_id pid)
{
	files_struct *fsp = br_lck->fsp;
	uint16 tid = fsp->conn->cnum;
	int fnum = fsp->fnum;
	unsigned int i, j, dcount=0;
	struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;

	/* Remove any existing locks for this fnum (or any fnum if they're POSIX). */

	for (i=0; i < br_lck->num_locks; i++) {
		struct lock_struct *lock = &locks[i];
		BOOL del_this_lock = False;

		if (lock->context.tid == tid && procid_equal(&lock->context.pid, &pid)) {
			if ((lock->lock_flav == WINDOWS_LOCK) && (lock->fnum == fnum)) {
				del_this_lock = True;
			} else if (lock->lock_flav == POSIX_LOCK) {
				del_this_lock = True;
			}
		}

		if (del_this_lock) {
			/* Send unlock messages to any pending waiters that overlap. */
			for (j=0; j < br_lck->num_locks; j++) {
				struct lock_struct *pend_lock = &locks[j];

				/* Ignore our own or non-pending locks. */
				if (pend_lock->lock_type != PENDING_LOCK) {
					continue;
				}

				/* Optimisation - don't send to this fnum as we're
				   closing it. */
				if (pend_lock->context.tid == tid &&
				    procid_equal(&pend_lock->context.pid, &pid) &&
				    pend_lock->fnum == fnum) {
					continue;
				}

				/* We could send specific lock info here... */
				if (brl_pending_overlap(lock, pend_lock)) {
					become_root();
					message_send_pid(pend_lock->context.pid,
							MSG_SMB_UNLOCK,
							NULL, 0, True);
					unbecome_root();
				}
			}

			/* found it - delete it */
			if (br_lck->num_locks > 1 && i < br_lck->num_locks - 1) {
				memmove(&locks[i], &locks[i+1], 
					sizeof(*locks)*((br_lck->num_locks-1) - i));
			}
			br_lck->num_locks--;
			br_lck->modified = True;
			i--;
			dcount++;
		}
	}
}

/****************************************************************************
 Ensure this set of lock entries is valid.
****************************************************************************/

static BOOL validate_lock_entries(unsigned int *pnum_entries, struct lock_struct **pplocks)
{
	unsigned int i;
	unsigned int num_valid_entries = 0;
	struct lock_struct *locks = *pplocks;

	for (i = 0; i < *pnum_entries; i++) {
		struct lock_struct *lock_data = &locks[i];
		if (!process_exists(lock_data->context.pid)) {
			/* This process no longer exists - mark this
			   entry as invalid by zeroing it. */
			ZERO_STRUCTP(lock_data);
		} else {
			num_valid_entries++;
		}
	}

	if (num_valid_entries != *pnum_entries) {
		struct lock_struct *new_lock_data = NULL;

		if (num_valid_entries) {
			new_lock_data = SMB_MALLOC_ARRAY(struct lock_struct, num_valid_entries);
			if (!new_lock_data) {
				DEBUG(3, ("malloc fail\n"));
				return False;
			}

			num_valid_entries = 0;
			for (i = 0; i < *pnum_entries; i++) {
				struct lock_struct *lock_data = &locks[i];
				if (lock_data->context.smbpid &&
						lock_data->context.tid) {
					/* Valid (nonzero) entry - copy it. */
					memcpy(&new_lock_data[num_valid_entries],
						lock_data, sizeof(struct lock_struct));
					num_valid_entries++;
				}
			}
		}

		SAFE_FREE(*pplocks);
		*pplocks = new_lock_data;
		*pnum_entries = num_valid_entries;
	}

	return True;
}

/****************************************************************************
 Traverse the whole database with this function, calling traverse_callback
 on each lock.
****************************************************************************/

static int traverse_fn(TDB_CONTEXT *ttdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	struct lock_struct *locks;
	struct lock_key *key;
	unsigned int i;
	unsigned int num_locks = 0;
	unsigned int orig_num_locks = 0;

	BRLOCK_FN(traverse_callback) = (BRLOCK_FN_CAST())state;

	/* In a traverse function we must make a copy of
	   dbuf before modifying it. */

	locks = (struct lock_struct *)memdup(dbuf.dptr, dbuf.dsize);
	if (!locks) {
		return -1; /* Terminate traversal. */
	}

	key = (struct lock_key *)kbuf.dptr;
	orig_num_locks = num_locks = dbuf.dsize/sizeof(*locks);

	/* Ensure the lock db is clean of entries from invalid processes. */

	if (!validate_lock_entries(&num_locks, &locks)) {
		SAFE_FREE(locks);
		return -1; /* Terminate traversal */
	}

	if (orig_num_locks != num_locks) {
		dbuf.dptr = (void *)locks;
		dbuf.dsize = num_locks * sizeof(*locks);

		if (dbuf.dsize) {
			tdb_store(ttdb, kbuf, dbuf, TDB_REPLACE);
		} else {
			tdb_delete(ttdb, kbuf);
		}
	}

	for ( i=0; i<num_locks; i++) {
		traverse_callback(key->device,
				  key->inode,
				  locks[i].context.pid,
				  locks[i].lock_type,
				  locks[i].lock_flav,
				  locks[i].start,
				  locks[i].size);
	}

	SAFE_FREE(locks);
	return 0;
}

/*******************************************************************
 Call the specified function on each lock in the database.
********************************************************************/

int brl_forall(BRLOCK_FN(fn))
{
	if (!tdb) {
		return 0;
	}
	return tdb_traverse(tdb, traverse_fn, (void *)fn);
}

/*******************************************************************
 Store a potentially modified set of byte range lock data back into
 the database.
 Unlock the record.
********************************************************************/

int byte_range_lock_destructor(struct byte_range_lock *br_lck)
{
	TDB_DATA key;

	key.dptr = (char *)&br_lck->key;
	key.dsize = sizeof(struct lock_key);

	if (!br_lck->modified) {
		goto done;
	}

	if (br_lck->num_locks == 0) {
		/* No locks - delete this entry. */
		if (tdb_delete(tdb, key) == -1) {
			smb_panic("Could not delete byte range lock entry\n");
		}
	} else {
		TDB_DATA data;
		data.dptr = br_lck->lock_data;
		data.dsize = br_lck->num_locks * sizeof(struct lock_struct);

		if (tdb_store(tdb, key, data, TDB_REPLACE) == -1) {
			smb_panic("Could not store byte range mode entry\n");
		}
	}

 done:

	tdb_chainunlock(tdb, key);
	SAFE_FREE(br_lck->lock_data);
	SAFE_FREE(br_lck);
	return 0;
}

/*******************************************************************
 Fetch a set of byte range lock data from the database.
 Leave the record locked.
********************************************************************/

struct byte_range_lock *brl_get_locks(files_struct *fsp)
{
	TDB_DATA key;
	TDB_DATA data;
	struct byte_range_lock *br_lck = SMB_MALLOC_P(struct byte_range_lock);

	if (br_lck == NULL) {
		return NULL;
	}

	br_lck->fsp = fsp;
	br_lck->num_locks = 0;
	br_lck->modified = False;
	memset(&br_lck->key, '\0', sizeof(struct lock_key));
	br_lck->key.device = fsp->dev;
	br_lck->key.inode = fsp->inode;

	key.dptr = (char *)&br_lck->key;
	key.dsize = sizeof(struct lock_key);

	if (tdb_chainlock(tdb, key) != 0) {
		DEBUG(3, ("Could not lock byte range lock entry\n"));
		SAFE_FREE(br_lck);
		return NULL;
	}

	data = tdb_fetch(tdb, key);
	br_lck->lock_data = (void *)data.dptr;
	br_lck->num_locks = data.dsize / sizeof(struct lock_struct);

	if (!fsp->lockdb_clean) {

		/* This is the first time we've accessed this. */
		/* Go through and ensure all entries exist - remove any that don't. */
		/* Makes the lockdb self cleaning at low cost. */

		if (!validate_lock_entries(&br_lck->num_locks, (struct lock_struct **)&br_lck->lock_data)) {
			tdb_chainunlock(tdb, key);
			SAFE_FREE(br_lck->lock_data);
			SAFE_FREE(br_lck);
			return NULL;
		}

		/* Mark the lockdb as "clean" as seen from this open file. */
		fsp->lockdb_clean = True;
	}

	if (DEBUGLEVEL >= 10) {
		unsigned int i;
		struct lock_struct *locks = (struct lock_struct *)br_lck->lock_data;
		DEBUG(10,("brl_get_locks: %u current locks on dev=%.0f, inode=%.0f\n",
			br_lck->num_locks,
			(double)fsp->dev, (double)fsp->inode ));
		for( i = 0; i < br_lck->num_locks; i++) {
			print_lock_struct(i, &locks[i]);
		}
	}
	return br_lck;
}

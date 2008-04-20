/* 
   Unix SMB/CIFS implementation.
   Locking functions
   Copyright (C) Andrew Tridgell 1992-2000
   Copyright (C) Jeremy Allison 1992-2006
   Copyright (C) Volker Lendecke 2005
   
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

   12 aug 96: Erik.Devriendt@te6.siemens.be
   added support for shared memory implementation of share mode locking

   May 1997. Jeremy Allison (jallison@whistle.com). Modified share mode
   locking to deal with multiple share modes per open file.

   September 1997. Jeremy Allison (jallison@whistle.com). Added oplock
   support.

   rewrtten completely to use new tdb code. Tridge, Dec '99

   Added POSIX locking support. Jeremy Allison (jeremy@valinux.com), Apr. 2000.
   Added Unix Extensions POSIX locking support. Jeremy Allison Mar 2006.
*/

#include "includes.h"
uint16 global_smbpid;

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_LOCKING

/* the locking database handle */
static TDB_CONTEXT *tdb;

/****************************************************************************
 Debugging aids :-).
****************************************************************************/

const char *lock_type_name(enum brl_type lock_type)
{
	switch (lock_type) {
		case READ_LOCK:
			return "READ";
		case WRITE_LOCK:
			return "WRITE";
		case PENDING_LOCK:
			return "PENDING";
		default:
			return "other";
	}
}

const char *lock_flav_name(enum brl_flavour lock_flav)
{
	return (lock_flav == WINDOWS_LOCK) ? "WINDOWS_LOCK" : "POSIX_LOCK";
}

/****************************************************************************
 Utility function called to see if a file region is locked.
 Called in the read/write codepath.
****************************************************************************/

BOOL is_locked(files_struct *fsp,
		SMB_BIG_UINT count,
		SMB_BIG_UINT offset, 
		enum brl_type lock_type)
{
	int snum = SNUM(fsp->conn);
	int strict_locking = lp_strict_locking(snum);
	enum brl_flavour lock_flav = lp_posix_cifsu_locktype();
	BOOL ret = True;
	
	if (count == 0) {
		return False;
	}

	if (!lp_locking(snum) || !strict_locking) {
		return False;
	}

	if (strict_locking == Auto) {
		if  (EXCLUSIVE_OPLOCK_TYPE(fsp->oplock_type) && (lock_type == READ_LOCK || lock_type == WRITE_LOCK)) {
			DEBUG(10,("is_locked: optimisation - exclusive oplock on file %s\n", fsp->fsp_name ));
			ret = False;
		} else if ((fsp->oplock_type == LEVEL_II_OPLOCK) &&
			   (lock_type == READ_LOCK)) {
			DEBUG(10,("is_locked: optimisation - level II oplock on file %s\n", fsp->fsp_name ));
			ret = False;
		} else {
			struct byte_range_lock *br_lck = brl_get_locks(fsp);
			if (!br_lck) {
				return False;
			}
			ret = !brl_locktest(br_lck,
					global_smbpid,
					procid_self(),
					offset,
					count,
					lock_type,
					lock_flav);
			byte_range_lock_destructor(br_lck);
		}
	} else {
		struct byte_range_lock *br_lck = brl_get_locks(fsp);
		if (!br_lck) {
			return False;
		}
		ret = !brl_locktest(br_lck,
				global_smbpid,
				procid_self(),
				offset,
				count,
				lock_type,
				lock_flav);
		byte_range_lock_destructor(br_lck);
	}

	DEBUG(10,("is_locked: flavour = %s brl start=%.0f len=%.0f %s for fnum %d file %s\n",
			lock_flav_name(lock_flav),
			(double)offset, (double)count, ret ? "locked" : "unlocked",
			fsp->fnum, fsp->fsp_name ));

	return ret;
}

/****************************************************************************
 Find out if a lock could be granted - return who is blocking us if we can't.
****************************************************************************/

NTSTATUS query_lock(files_struct *fsp,
			uint16 *psmbpid,
			SMB_BIG_UINT *pcount,
			SMB_BIG_UINT *poffset,
			enum brl_type *plock_type,
			enum brl_flavour lock_flav)
{
	struct byte_range_lock *br_lck = NULL;
	NTSTATUS status = NT_STATUS_LOCK_NOT_GRANTED;

	if (!OPEN_FSP(fsp) || !fsp->can_lock) {
		return NT_STATUS_INVALID_HANDLE;
	}

	if (!lp_locking(SNUM(fsp->conn))) {
		return NT_STATUS_OK;
	}

	br_lck = brl_get_locks(fsp);
	if (!br_lck) {
		return NT_STATUS_NO_MEMORY;
	}

	status = brl_lockquery(br_lck,
			psmbpid,
			procid_self(),
			poffset,
			pcount,
			plock_type,
			lock_flav);

	byte_range_lock_destructor(br_lck);
	return status;
}

/****************************************************************************
 Utility function called by locking requests.
****************************************************************************/

NTSTATUS do_lock(files_struct *fsp,
			uint16 lock_pid,
			SMB_BIG_UINT count,
			SMB_BIG_UINT offset,
			enum brl_type lock_type,
			enum brl_flavour lock_flav,
			BOOL *my_lock_ctx)
{
	struct byte_range_lock *br_lck = NULL;
	NTSTATUS status = NT_STATUS_LOCK_NOT_GRANTED;

	if (!OPEN_FSP(fsp) || !fsp->can_lock) {
		return NT_STATUS_INVALID_HANDLE;
	}

	if (!lp_locking(SNUM(fsp->conn))) {
		return NT_STATUS_OK;
	}

	/* NOTE! 0 byte long ranges ARE allowed and should be stored  */

	DEBUG(10,("do_lock: lock flavour %s lock type %s start=%.0f len=%.0f requested for fnum %d file %s\n",
		lock_flav_name(lock_flav), lock_type_name(lock_type),
		(double)offset, (double)count, fsp->fnum, fsp->fsp_name ));

	br_lck = brl_get_locks(fsp);
	if (!br_lck) {
		return NT_STATUS_NO_MEMORY;
	}

	status = brl_lock(br_lck,
			lock_pid,
			procid_self(),
			offset,
			count, 
			lock_type,
			lock_flav,
			my_lock_ctx);

	byte_range_lock_destructor(br_lck);
	return status;
}

/****************************************************************************
 Utility function called by locking requests. This is *DISGUSTING*. It also
 appears to be "What Windows Does" (tm). Andrew, ever wonder why Windows 2000
 is so slow on the locking tests...... ? This is the reason. Much though I hate
 it, we need this. JRA.
****************************************************************************/

NTSTATUS do_lock_spin(files_struct *fsp,
			uint16 lock_pid,
			SMB_BIG_UINT count,
			SMB_BIG_UINT offset,
			enum brl_type lock_type,
			enum brl_flavour lock_flav,
			BOOL *my_lock_ctx)
{
	int j, maxj = lp_lock_spin_count();
	int sleeptime = lp_lock_sleep_time();
	NTSTATUS status, ret;

	if (maxj <= 0) {
		maxj = 1;
	}

	ret = NT_STATUS_OK; /* to keep dumb compilers happy */

	for (j = 0; j < maxj; j++) {
		status = do_lock(fsp,
				lock_pid,
				count,
				offset,
				lock_type,
				lock_flav,
				my_lock_ctx);

		if (!NT_STATUS_EQUAL(status, NT_STATUS_LOCK_NOT_GRANTED) &&
		    !NT_STATUS_EQUAL(status, NT_STATUS_FILE_LOCK_CONFLICT)) {
			return status;
		}
		/* if we do fail then return the first error code we got */
		if (j == 0) {
			ret = status;
			/* Don't spin if we blocked ourselves. */
			if (*my_lock_ctx) {
				return ret;
			}

			/* Only spin for Windows locks. */
			if (lock_flav == POSIX_LOCK) {
				return ret;
			}
		}

		if (sleeptime) {
			sys_usleep(sleeptime);
		}
	}
	return ret;
}

/****************************************************************************
 Utility function called by unlocking requests.
****************************************************************************/

NTSTATUS do_unlock(files_struct *fsp,
			uint16 lock_pid,
			SMB_BIG_UINT count,
			SMB_BIG_UINT offset,
			enum brl_flavour lock_flav)
{
	BOOL ok = False;
	struct byte_range_lock *br_lck = NULL;
	
	if (!lp_locking(SNUM(fsp->conn))) {
		return NT_STATUS_OK;
	}
	
	if (!OPEN_FSP(fsp) || !fsp->can_lock) {
		return NT_STATUS_INVALID_HANDLE;
	}
	
	DEBUG(10,("do_unlock: unlock start=%.0f len=%.0f requested for fnum %d file %s\n",
		  (double)offset, (double)count, fsp->fnum, fsp->fsp_name ));

	br_lck = brl_get_locks(fsp);
	if (!br_lck) {
		return NT_STATUS_NO_MEMORY;
	}

	ok = brl_unlock(br_lck,
			lock_pid,
			procid_self(),
			offset,
			count,
			lock_flav);
   
	byte_range_lock_destructor(br_lck);

	if (!ok) {
		DEBUG(10,("do_unlock: returning ERRlock.\n" ));
		return NT_STATUS_RANGE_NOT_LOCKED;
	}

	return NT_STATUS_OK;
}

/****************************************************************************
 Remove any locks on this fd. Called from file_close().
****************************************************************************/

void locking_close_file(files_struct *fsp)
{
	struct byte_range_lock *br_lck;
	struct process_id pid = procid_self();

	if (!lp_locking(SNUM(fsp->conn)))
		return;

	/*
	 * Just release all the brl locks, no need to release individually.
	 */

	br_lck = brl_get_locks(fsp);
	if (br_lck) {
		brl_close_fnum(br_lck, pid);
		byte_range_lock_destructor(br_lck);
	}

	if(lp_posix_locking(SNUM(fsp->conn))) {
	 	/* Release all the POSIX locks.*/
		posix_locking_close_file(fsp);

	}
}

/****************************************************************************
 Initialise the locking functions.
****************************************************************************/

static int open_read_only;

BOOL locking_init(int read_only)
{
	brl_init(read_only);

	if (tdb)
		return True;

	tdb = tdb_open_log(lock_path("locking.tdb"), 
			lp_open_files_db_hash_size(),
			TDB_DEFAULT|(read_only?0x0:TDB_CLEAR_IF_FIRST), 
			read_only?O_RDONLY:O_RDWR|O_CREAT,
			0644);

	if (!tdb) {
		DEBUG(0,("ERROR: Failed to initialise locking database\n"));
		return False;
	}

	if (!posix_locking_init(read_only))
		return False;

	open_read_only = read_only;

	return True;
}

/*******************************************************************
 Deinitialize the share_mode management.
******************************************************************/

BOOL locking_end(void)
{
	BOOL ret = True;

	brl_shutdown(open_read_only);
	if (tdb) {
		if (tdb_close(tdb) != 0)
			ret = False;
	}

	return ret;
}

/*******************************************************************
 Form a static locking key for a dev/inode pair.
******************************************************************/

/* key and data records in the tdb locking database */
struct locking_key {
	SMB_DEV_T dev;
	SMB_INO_T ino;
};

/*******************************************************************
 Form a static locking key for a dev/inode pair.
******************************************************************/

static TDB_DATA locking_key(SMB_DEV_T dev, SMB_INO_T inode)
{
	static struct locking_key key;
	TDB_DATA kbuf;

	memset(&key, '\0', sizeof(key));
	key.dev = dev;
	key.ino = inode;
	kbuf.dptr = (char *)&key;
	kbuf.dsize = sizeof(key);
	return kbuf;
}

/*******************************************************************
 Print out a share mode.
********************************************************************/

char *share_mode_str(int num, struct share_mode_entry *e)
{
	static pstring share_str;

	slprintf(share_str, sizeof(share_str)-1, "share_mode_entry[%d]: %s "
		 "pid = %s, share_access = 0x%x, private_options = 0x%x, "
		 "access_mask = 0x%x, mid = 0x%x, type= 0x%x, file_id = %lu, "
		 "uid = %u, dev = 0x%x, inode = %.0f",
		 num,
		 e->op_type == UNUSED_SHARE_MODE_ENTRY ? "UNUSED" : "",
		 procid_str_static(&e->pid),
		 e->share_access, e->private_options,
		 e->access_mask, e->op_mid, e->op_type, e->share_file_id,
		 (unsigned int)e->uid, (unsigned int)e->dev, (double)e->inode );

	return share_str;
}

/*******************************************************************
 Print out a share mode table.
********************************************************************/

static void print_share_mode_table(struct locking_data *data)
{
	int num_share_modes = data->u.s.num_share_mode_entries;
	struct share_mode_entry *shares =
		(struct share_mode_entry *)(data + 1);
	int i;

	for (i = 0; i < num_share_modes; i++) {
		struct share_mode_entry entry;

		memcpy(&entry, &shares[i], sizeof(struct share_mode_entry));
		DEBUG(10,("print_share_mode_table: %s\n",
			  share_mode_str(i, &entry)));
	}
}

/*******************************************************************
 Get all share mode entries for a dev/inode pair.
********************************************************************/

static BOOL parse_share_modes(TDB_DATA dbuf, struct share_mode_lock *lck)
{
	struct locking_data *data;
	int i;

	if (dbuf.dsize < sizeof(struct locking_data)) {
		smb_panic("PANIC: parse_share_modes: buffer too short.\n");
	}

	data = (struct locking_data *)dbuf.dptr;

	lck->delete_on_close = data->u.s.delete_on_close;
	lck->initial_delete_on_close = data->u.s.initial_delete_on_close;
	lck->num_share_modes = data->u.s.num_share_mode_entries;

	DEBUG(10, ("parse_share_modes: delete_on_close: %d, "
		   "initial_delete_on_close: %d, "
		   "num_share_modes: %d\n",
		lck->delete_on_close,
		lck->initial_delete_on_close,
		lck->num_share_modes));

	if ((lck->num_share_modes < 0) || (lck->num_share_modes > 1000000)) {
		DEBUG(0, ("invalid number of share modes: %d\n",
			  lck->num_share_modes));
		smb_panic("PANIC: invalid number of share modes");
	}

	lck->share_modes = NULL;
	
	if (lck->num_share_modes != 0) {

		if (dbuf.dsize < (sizeof(struct locking_data) +
				  (lck->num_share_modes *
				   sizeof(struct share_mode_entry)))) {
			smb_panic("PANIC: parse_share_modes: buffer too short.\n");
		}
				  
		lck->share_modes = talloc_memdup(lck, dbuf.dptr+sizeof(*data),
						 lck->num_share_modes *
						 sizeof(struct share_mode_entry));

		if (lck->share_modes == NULL) {
			smb_panic("talloc failed\n");
		}
	}

	/* Get any delete token. */
	if (data->u.s.delete_token_size) {
		char *p = dbuf.dptr + sizeof(*data) +
				(lck->num_share_modes *
				sizeof(struct share_mode_entry));

		if ((data->u.s.delete_token_size < sizeof(uid_t) + sizeof(gid_t)) ||
				((data->u.s.delete_token_size - sizeof(uid_t)) % sizeof(gid_t)) != 0) {
			DEBUG(0, ("parse_share_modes: invalid token size %d\n",
				data->u.s.delete_token_size));
			smb_panic("parse_share_modes: invalid token size\n");
		}

		lck->delete_token = TALLOC_P(lck, UNIX_USER_TOKEN);
		if (!lck->delete_token) {
			smb_panic("talloc failed\n");
		}

		/* Copy out the uid and gid. */
		memcpy(&lck->delete_token->uid, p, sizeof(uid_t));
		p += sizeof(uid_t);
		memcpy(&lck->delete_token->gid, p, sizeof(gid_t));
		p += sizeof(gid_t);

		/* Any supplementary groups ? */
		lck->delete_token->ngroups = (data->u.s.delete_token_size > (sizeof(uid_t) + sizeof(gid_t))) ?
					((data->u.s.delete_token_size -
						(sizeof(uid_t) + sizeof(gid_t)))/sizeof(gid_t)) : 0;

		if (lck->delete_token->ngroups) {
			/* Make this a talloc child of lck->delete_token. */
			lck->delete_token->groups = TALLOC_ARRAY(lck->delete_token, gid_t,
							lck->delete_token->ngroups);
			if (!lck->delete_token) {
				smb_panic("talloc failed\n");
			}

			for (i = 0; i < lck->delete_token->ngroups; i++) {
				memcpy(&lck->delete_token->groups[i], p, sizeof(gid_t));
				p += sizeof(gid_t);
			}
		}

	} else {
		lck->delete_token = NULL;
	}

	/* Save off the associated service path and filename. */
	lck->servicepath = talloc_strdup(lck, dbuf.dptr + sizeof(*data) +
					(lck->num_share_modes *
					sizeof(struct share_mode_entry)) +
					data->u.s.delete_token_size );

	lck->filename = talloc_strdup(lck, dbuf.dptr + sizeof(*data) +
					(lck->num_share_modes *
					sizeof(struct share_mode_entry)) +
					data->u.s.delete_token_size +
					strlen(lck->servicepath) + 1 );

	/*
	 * Ensure that each entry has a real process attached.
	 */

	for (i = 0; i < lck->num_share_modes; i++) {
		struct share_mode_entry *entry_p = &lck->share_modes[i];
		DEBUG(10,("parse_share_modes: %s\n",
			  share_mode_str(i, entry_p) ));
		if (!process_exists(entry_p->pid)) {
			DEBUG(10,("parse_share_modes: deleted %s\n",
				  share_mode_str(i, entry_p) ));
			entry_p->op_type = UNUSED_SHARE_MODE_ENTRY;
			lck->modified = True;
		}
	}

	return True;
}

static TDB_DATA unparse_share_modes(struct share_mode_lock *lck)
{
	TDB_DATA result;
	int num_valid = 0;
	int i;
	struct locking_data *data;
	ssize_t offset;
	ssize_t sp_len;
	uint32 delete_token_size;

	result.dptr = NULL;
	result.dsize = 0;

	for (i=0; i<lck->num_share_modes; i++) {
		if (!is_unused_share_mode_entry(&lck->share_modes[i])) {
			num_valid += 1;
		}
	}

	if (num_valid == 0) {
		return result;
	}

	sp_len = strlen(lck->servicepath);
	delete_token_size = (lck->delete_token ?
			(sizeof(uid_t) + sizeof(gid_t) + (lck->delete_token->ngroups*sizeof(gid_t))) : 0);

	result.dsize = sizeof(*data) +
		lck->num_share_modes * sizeof(struct share_mode_entry) +
		delete_token_size +
		sp_len + 1 +
		strlen(lck->filename) + 1;
	result.dptr = talloc_size(lck, result.dsize);

	if (result.dptr == NULL) {
		smb_panic("talloc failed\n");
	}

	data = (struct locking_data *)result.dptr;
	ZERO_STRUCTP(data);
	data->u.s.num_share_mode_entries = lck->num_share_modes;
	data->u.s.delete_on_close = lck->delete_on_close;
	data->u.s.initial_delete_on_close = lck->initial_delete_on_close;
	data->u.s.delete_token_size = delete_token_size;
	DEBUG(10, ("unparse_share_modes: del: %d, initial del %d, tok = %u, num: %d\n",
		data->u.s.delete_on_close,
		data->u.s.initial_delete_on_close,
		(unsigned int)data->u.s.delete_token_size,
		data->u.s.num_share_mode_entries));
	memcpy(result.dptr + sizeof(*data), lck->share_modes,
	       sizeof(struct share_mode_entry)*lck->num_share_modes);
	offset = sizeof(*data) +
		sizeof(struct share_mode_entry)*lck->num_share_modes;

	/* Store any delete on close token. */
	if (lck->delete_token) {
		char *p = result.dptr + offset;

		memcpy(p, &lck->delete_token->uid, sizeof(uid_t));
		p += sizeof(uid_t);

		memcpy(p, &lck->delete_token->gid, sizeof(gid_t));
		p += sizeof(gid_t);

		for (i = 0; i < lck->delete_token->ngroups; i++) {
			memcpy(p, &lck->delete_token->groups[i], sizeof(gid_t));
			p += sizeof(gid_t);
		}
		offset = p - result.dptr;
	}

	safe_strcpy(result.dptr + offset, lck->servicepath,
		    result.dsize - offset - 1);
	offset += sp_len + 1;
	safe_strcpy(result.dptr + offset, lck->filename,
		    result.dsize - offset - 1);

	if (DEBUGLEVEL >= 10) {
		print_share_mode_table(data);
	}

	return result;
}

static int share_mode_lock_destructor(void *p)
{
	struct share_mode_lock *lck =
		talloc_get_type_abort(p, struct share_mode_lock);
	TDB_DATA key = locking_key(lck->dev, lck->ino);
	TDB_DATA data;

	if (!lck->modified) {
		goto done;
	}

	data = unparse_share_modes(lck);

	if (data.dptr == NULL) {
		if (!lck->fresh) {
			/* There has been an entry before, delete it */
			if (tdb_delete(tdb, key) == -1) {
				smb_panic("Could not delete share entry\n");
			}
		}
		goto done;
	}

	if (tdb_store(tdb, key, data, TDB_REPLACE) == -1) {
		smb_panic("Could not store share mode entry\n");
	}

 done:
	tdb_chainunlock(tdb, key);

	return 0;
}

struct share_mode_lock *get_share_mode_lock(TALLOC_CTX *mem_ctx,
						SMB_DEV_T dev, SMB_INO_T ino,
						const char *servicepath,
						const char *fname)
{
	struct share_mode_lock *lck;
	TDB_DATA key = locking_key(dev, ino);
	TDB_DATA data;

	lck = TALLOC_P(mem_ctx, struct share_mode_lock);
	if (lck == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return NULL;
	}

	/* Ensure we set every field here as the destructor must be
	   valid even if parse_share_modes fails. */

	lck->servicepath = NULL;
	lck->filename = NULL;
	lck->dev = dev;
	lck->ino = ino;
	lck->num_share_modes = 0;
	lck->share_modes = NULL;
	lck->delete_token = NULL;
	lck->delete_on_close = False;
	lck->initial_delete_on_close = False;
	lck->fresh = False;
	lck->modified = False;

	if (tdb_chainlock(tdb, key) != 0) {
		DEBUG(3, ("Could not lock share entry\n"));
		TALLOC_FREE(lck);
		return NULL;
	}

	/* We must set the destructor immediately after the chainlock
	   ensure the lock is cleaned up on any of the error return
	   paths below. */

	talloc_set_destructor(lck, share_mode_lock_destructor);

	data = tdb_fetch(tdb, key);
	lck->fresh = (data.dptr == NULL);

	if (lck->fresh) {

		if (fname == NULL || servicepath == NULL) {
			TALLOC_FREE(lck);
			return NULL;
		}
		lck->filename = talloc_strdup(lck, fname);
		lck->servicepath = talloc_strdup(lck, servicepath);
		if (lck->filename == NULL || lck->servicepath == NULL) {
			DEBUG(0, ("talloc failed\n"));
			TALLOC_FREE(lck);
			return NULL;
		}
	} else {
		if (!parse_share_modes(data, lck)) {
			DEBUG(0, ("Could not parse share modes\n"));
			TALLOC_FREE(lck);
			SAFE_FREE(data.dptr);
			return NULL;
		}
	}

	SAFE_FREE(data.dptr);

	return lck;
}

/*******************************************************************
 Sets the service name and filename for rename.
 At this point we emit "file renamed" messages to all
 process id's that have this file open.
 Based on an initial code idea from SATOH Fumiyasu <fumiya@samba.gr.jp>
********************************************************************/

BOOL rename_share_filename(struct share_mode_lock *lck,
			const char *servicepath,
			const char *newname)
{
	size_t sp_len;
	size_t fn_len;
	size_t msg_len;
	char *frm = NULL;
	int i;

	if (!lck) {
		return False;
	}

	DEBUG(10, ("rename_share_filename: servicepath %s newname %s\n",
		servicepath, newname));

	/*
	 * rename_internal_fsp() and rename_internals() add './' to
	 * head of newname if newname does not contain a '/'.
	 */
	while (newname[0] && newname[1] && newname[0] == '.' && newname[1] == '/') {
		newname += 2;
	}

	lck->servicepath = talloc_strdup(lck, servicepath);
	lck->filename = talloc_strdup(lck, newname);
	if (lck->filename == NULL || lck->servicepath == NULL) {
		DEBUG(0, ("rename_share_filename: talloc failed\n"));
		return False;
	}
	lck->modified = True;

	sp_len = strlen(lck->servicepath);
	fn_len = strlen(lck->filename);

	msg_len = MSG_FILE_RENAMED_MIN_SIZE + sp_len + 1 + fn_len + 1;

	/* Set up the name changed message. */
	frm = TALLOC(lck, msg_len);
	if (!frm) {
		return False;
	}

	SDEV_T_VAL(frm,0,lck->dev);
	SINO_T_VAL(frm,8,lck->ino);

	DEBUG(10,("rename_share_filename: msg_len = %u\n", (unsigned int)msg_len ));

	safe_strcpy(&frm[16], lck->servicepath, sp_len);
	safe_strcpy(&frm[16 + sp_len + 1], lck->filename, fn_len);

	/* Send the messages. */
	for (i=0; i<lck->num_share_modes; i++) {
		struct share_mode_entry *se = &lck->share_modes[i];
		if (!is_valid_share_mode_entry(se)) {
			continue;
		}
		/* But not to ourselves... */
		if (procid_is_me(&se->pid)) {
			continue;
		}

		DEBUG(10,("rename_share_filename: sending rename message to pid %u "
			"dev %x, inode  %.0f sharepath %s newname %s\n",
			(unsigned int)procid_to_pid(&se->pid),
			(unsigned int)lck->dev, (double)lck->ino,
			lck->servicepath, lck->filename ));

		become_root();
		message_send_pid(se->pid, MSG_SMB_FILE_RENAME,
				frm, msg_len, True);
		unbecome_root();
	}

	return True;
}

BOOL get_delete_on_close_flag(SMB_DEV_T dev, SMB_INO_T inode)
{
	BOOL result;
	struct share_mode_lock *lck = get_share_mode_lock(NULL, dev, inode, NULL, NULL);
	if (!lck) {
		return False;
	}
	result = lck->delete_on_close;
	TALLOC_FREE(lck);
	return result;
}

BOOL is_valid_share_mode_entry(const struct share_mode_entry *e)
{
	int num_props = 0;

	num_props += ((e->op_type == NO_OPLOCK) ? 1 : 0);
	num_props += (EXCLUSIVE_OPLOCK_TYPE(e->op_type) ? 1 : 0);
	num_props += (LEVEL_II_OPLOCK_TYPE(e->op_type) ? 1 : 0);

	SMB_ASSERT(num_props <= 1);
	return (num_props != 0);
}

BOOL is_deferred_open_entry(const struct share_mode_entry *e)
{
	return (e->op_type == DEFERRED_OPEN_ENTRY);
}

BOOL is_unused_share_mode_entry(const struct share_mode_entry *e)
{
	return (e->op_type == UNUSED_SHARE_MODE_ENTRY);
}

/*******************************************************************
 Fill a share mode entry.
********************************************************************/

static void fill_share_mode_entry(struct share_mode_entry *e,
				  files_struct *fsp,
				  uid_t uid, uint16 mid, uint16 op_type)
{
	ZERO_STRUCTP(e);
	e->pid = procid_self();
	e->share_access = fsp->share_access;
	e->private_options = fsp->fh->private_options;
	e->access_mask = fsp->access_mask;
	e->op_mid = mid;
	e->op_type = op_type;
	e->time.tv_sec = fsp->open_time.tv_sec;
	e->time.tv_usec = fsp->open_time.tv_usec;
	e->dev = fsp->dev;
	e->inode = fsp->inode;
	e->share_file_id = fsp->fh->file_id;
	e->uid = (uint32)uid;
}

static void fill_deferred_open_entry(struct share_mode_entry *e,
				     const struct timeval request_time,
				     SMB_DEV_T dev, SMB_INO_T ino, uint16 mid)
{
	ZERO_STRUCTP(e);
	e->pid = procid_self();
	e->op_mid = mid;
	e->op_type = DEFERRED_OPEN_ENTRY;
	e->time.tv_sec = request_time.tv_sec;
	e->time.tv_usec = request_time.tv_usec;
	e->dev = dev;
	e->inode = ino;
	e->uid = (uint32)-1;
}

static void add_share_mode_entry(struct share_mode_lock *lck,
				 const struct share_mode_entry *entry)
{
	int i;

	for (i=0; i<lck->num_share_modes; i++) {
		struct share_mode_entry *e = &lck->share_modes[i];
		if (is_unused_share_mode_entry(e)) {
			*e = *entry;
			break;
		}
	}

	if (i == lck->num_share_modes) {
		/* No unused entry found */
		ADD_TO_ARRAY(lck, struct share_mode_entry, *entry,
			     &lck->share_modes, &lck->num_share_modes);
	}
	lck->modified = True;
}

void set_share_mode(struct share_mode_lock *lck, files_struct *fsp,
			uid_t uid, uint16 mid, uint16 op_type)
{
	struct share_mode_entry entry;
	fill_share_mode_entry(&entry, fsp, uid, mid, op_type);
	add_share_mode_entry(lck, &entry);
}

void add_deferred_open(struct share_mode_lock *lck, uint16 mid,
		       struct timeval request_time,
		       SMB_DEV_T dev, SMB_INO_T ino)
{
	struct share_mode_entry entry;
	fill_deferred_open_entry(&entry, request_time, dev, ino, mid);
	add_share_mode_entry(lck, &entry);
}

/*******************************************************************
 Check if two share mode entries are identical, ignoring oplock 
 and mid info and desired_access. (Removed paranoia test - it's
 not automatically a logic error if they are identical. JRA.)
********************************************************************/

static BOOL share_modes_identical(struct share_mode_entry *e1,
				  struct share_mode_entry *e2)
{
	/* We used to check for e1->share_access == e2->share_access here
	   as well as the other fields but 2 different DOS or FCB opens
	   sharing the same share mode entry may validly differ in
	   fsp->share_access field. */

	return (procid_equal(&e1->pid, &e2->pid) &&
		e1->dev == e2->dev &&
		e1->inode == e2->inode &&
		e1->share_file_id == e2->share_file_id );
}

static BOOL deferred_open_identical(struct share_mode_entry *e1,
				    struct share_mode_entry *e2)
{
	return (procid_equal(&e1->pid, &e2->pid) &&
		(e1->op_mid == e2->op_mid) &&
		(e1->dev == e2->dev) &&
		(e1->inode == e2->inode));
}

static struct share_mode_entry *find_share_mode_entry(struct share_mode_lock *lck,
						      struct share_mode_entry *entry)
{
	int i;

	for (i=0; i<lck->num_share_modes; i++) {
		struct share_mode_entry *e = &lck->share_modes[i];
		if (is_valid_share_mode_entry(entry) &&
		    is_valid_share_mode_entry(e) &&
		    share_modes_identical(e, entry)) {
			return e;
		}
		if (is_deferred_open_entry(entry) &&
		    is_deferred_open_entry(e) &&
		    deferred_open_identical(e, entry)) {
			return e;
		}
	}
	return NULL;
}

/*******************************************************************
 Del the share mode of a file for this process. Return the number of
 entries left.
********************************************************************/

BOOL del_share_mode(struct share_mode_lock *lck, files_struct *fsp)
{
	struct share_mode_entry entry, *e;

	/* Don't care about the pid owner being correct here - just a search. */
	fill_share_mode_entry(&entry, fsp, (uid_t)-1, 0, NO_OPLOCK);

	e = find_share_mode_entry(lck, &entry);
	if (e == NULL) {
		return False;
	}

	e->op_type = UNUSED_SHARE_MODE_ENTRY;
	lck->modified = True;
	return True;
}

void del_deferred_open_entry(struct share_mode_lock *lck, uint16 mid)
{
	struct share_mode_entry entry, *e;

	fill_deferred_open_entry(&entry, timeval_zero(),
				 lck->dev, lck->ino, mid);

	e = find_share_mode_entry(lck, &entry);
	if (e == NULL) {
		return;
	}

	e->op_type = UNUSED_SHARE_MODE_ENTRY;
	lck->modified = True;
}

/*******************************************************************
 Remove an oplock mid and mode entry from a share mode.
********************************************************************/

BOOL remove_share_oplock(struct share_mode_lock *lck, files_struct *fsp)
{
	struct share_mode_entry entry, *e;

	/* Don't care about the pid owner being correct here - just a search. */
	fill_share_mode_entry(&entry, fsp, (uid_t)-1, 0, NO_OPLOCK);

	e = find_share_mode_entry(lck, &entry);
	if (e == NULL) {
		return False;
	}

	e->op_mid = 0;
	e->op_type = NO_OPLOCK;
	lck->modified = True;
	return True;
}

/*******************************************************************
 Downgrade a oplock type from exclusive to level II.
********************************************************************/

BOOL downgrade_share_oplock(struct share_mode_lock *lck, files_struct *fsp)
{
	struct share_mode_entry entry, *e;

	/* Don't care about the pid owner being correct here - just a search. */
	fill_share_mode_entry(&entry, fsp, (uid_t)-1, 0, NO_OPLOCK);

	e = find_share_mode_entry(lck, &entry);
	if (e == NULL) {
		return False;
	}

	e->op_type = LEVEL_II_OPLOCK;
	lck->modified = True;
	return True;
}

/****************************************************************************
 Deal with the internal needs of setting the delete on close flag. Note that
 as the tdb locking is recursive, it is safe to call this from within 
 open_file_ntcreate. JRA.
****************************************************************************/

NTSTATUS can_set_delete_on_close(files_struct *fsp, BOOL delete_on_close,
				 uint32 dosmode)
{
	if (!delete_on_close) {
		return NT_STATUS_OK;
	}

	/*
	 * Only allow delete on close for writable files.
	 */

	if ((dosmode & aRONLY) &&
	    !lp_delete_readonly(SNUM(fsp->conn))) {
		DEBUG(10,("can_set_delete_on_close: file %s delete on close "
			  "flag set but file attribute is readonly.\n",
			  fsp->fsp_name ));
		return NT_STATUS_CANNOT_DELETE;
	}

	/*
	 * Only allow delete on close for writable shares.
	 */

	if (!CAN_WRITE(fsp->conn)) {
		DEBUG(10,("can_set_delete_on_close: file %s delete on "
			  "close flag set but write access denied on share.\n",
			  fsp->fsp_name ));
		return NT_STATUS_ACCESS_DENIED;
	}

	/*
	 * Only allow delete on close for files/directories opened with delete
	 * intent.
	 */

	if (!(fsp->access_mask & DELETE_ACCESS)) {
		DEBUG(10,("can_set_delete_on_close: file %s delete on "
			  "close flag set but delete access denied.\n",
			  fsp->fsp_name ));
		return NT_STATUS_ACCESS_DENIED;
	}

	return NT_STATUS_OK;
}

/*************************************************************************
 Return a talloced copy of a UNIX_USER_TOKEN. NULL on fail.
 (Should this be in locking.c.... ?).
*************************************************************************/

static UNIX_USER_TOKEN *copy_unix_token(TALLOC_CTX *ctx, UNIX_USER_TOKEN *tok)
{
	UNIX_USER_TOKEN *cpy;

	if (tok == NULL) {
		return NULL;
	}

	cpy = TALLOC_P(ctx, UNIX_USER_TOKEN);
	if (!cpy) {
		return NULL;
	}

	cpy->uid = tok->uid;
	cpy->gid = tok->gid;
	cpy->ngroups = tok->ngroups;
	if (tok->ngroups) {
		/* Make this a talloc child of cpy. */
		cpy->groups = TALLOC_ARRAY(cpy, gid_t, tok->ngroups);
		if (!cpy->groups) {
			return NULL;
		}
		memcpy(cpy->groups, tok->groups, tok->ngroups * sizeof(gid_t));
	}
	return cpy;
}

/****************************************************************************
 Replace the delete on close token.
****************************************************************************/

void set_delete_on_close_token(struct share_mode_lock *lck, UNIX_USER_TOKEN *tok)
{
	/* Ensure there's no token. */
	if (lck->delete_token) {
		TALLOC_FREE(lck->delete_token); /* Also deletes groups... */
		lck->delete_token = NULL;
	}

	/* Copy the new token (can be NULL). */
	lck->delete_token = copy_unix_token(lck, tok);
	lck->modified = True;
}

/****************************************************************************
 Sets the delete on close flag over all share modes on this file.
 Modify the share mode entry for all files open
 on this device and inode to tell other smbds we have
 changed the delete on close flag. This will be noticed
 in the close code, the last closer will delete the file
 if flag is set.
 Note that setting this to any value clears the initial_delete_on_close flag.
 If delete_on_close is True this makes a copy of any UNIX_USER_TOKEN into the
 lck entry.
****************************************************************************/

BOOL set_delete_on_close(files_struct *fsp, BOOL delete_on_close, UNIX_USER_TOKEN *tok)
{
	struct share_mode_lock *lck;
	
	DEBUG(10,("set_delete_on_close: %s delete on close flag for "
		  "fnum = %d, file %s\n",
		  delete_on_close ? "Adding" : "Removing", fsp->fnum,
		  fsp->fsp_name ));

	if (fsp->is_stat) {
		return True;
	}

	lck = get_share_mode_lock(NULL, fsp->dev, fsp->inode, NULL, NULL);
	if (lck == NULL) {
		return False;
	}

	if (lck->delete_on_close != delete_on_close) {
		set_delete_on_close_token(lck, tok);
		lck->delete_on_close = delete_on_close;
		if (delete_on_close) {
			SMB_ASSERT(lck->delete_token != NULL);
		}
		lck->modified = True;
	}

	if (lck->initial_delete_on_close) {
		lck->initial_delete_on_close = False;
		lck->modified = True;
	}

	TALLOC_FREE(lck);
	return True;
}

static int traverse_fn(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, 
                       void *state)
{
	struct locking_data *data;
	struct share_mode_entry *shares;
	const char *sharepath;
	const char *fname;
	int i;
	LOCKING_FN(traverse_callback) = (LOCKING_FN_CAST())state;

	/* Ensure this is a locking_key record. */
	if (kbuf.dsize != sizeof(struct locking_key))
		return 0;

	data = (struct locking_data *)dbuf.dptr;
	shares = (struct share_mode_entry *)(dbuf.dptr + sizeof(*data));
	sharepath = dbuf.dptr + sizeof(*data) +
		data->u.s.num_share_mode_entries*sizeof(*shares) +
		data->u.s.delete_token_size;
	fname = dbuf.dptr + sizeof(*data) +
		data->u.s.num_share_mode_entries*sizeof(*shares) +
		data->u.s.delete_token_size +
		strlen(sharepath) + 1;

	for (i=0;i<data->u.s.num_share_mode_entries;i++) {
		traverse_callback(&shares[i], sharepath, fname);
	}
	return 0;
}

/*******************************************************************
 Call the specified function on each entry under management by the
 share mode system.
********************************************************************/

int share_mode_forall(void (*fn)(const struct share_mode_entry *, const char *, const char *))
{
	if (tdb == NULL)
		return 0;
	return tdb_traverse(tdb, traverse_fn, fn);
}

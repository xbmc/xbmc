/* 
   Unix SMB/CIFS implementation.
   file opening and share modes
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Jeremy Allison 2001-2004
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
*/

#include "includes.h"

extern struct generic_mapping file_generic_mapping;
extern struct current_user current_user;
extern userdom_struct current_user_info;
extern uint16 global_smbpid;
extern BOOL global_client_failed_oplock_break;

struct deferred_open_record {
	BOOL delayed_for_oplocks;
	SMB_DEV_T dev;
	SMB_INO_T inode;
};

/****************************************************************************
 fd support routines - attempt to do a dos_open.
****************************************************************************/

static int fd_open(struct connection_struct *conn,
			const char *fname, 
			int flags,
			mode_t mode)
{
	int fd;
#ifdef O_NOFOLLOW
	if (!lp_symlinks(SNUM(conn))) {
		flags |= O_NOFOLLOW;
	}
#endif

	fd = SMB_VFS_OPEN(conn,fname,flags,mode);

	DEBUG(10,("fd_open: name %s, flags = 0%o mode = 0%o, fd = %d. %s\n", fname,
		flags, (int)mode, fd, (fd == -1) ? strerror(errno) : "" ));

	return fd;
}

/****************************************************************************
 Close the file associated with a fsp.
****************************************************************************/

int fd_close(struct connection_struct *conn,
		files_struct *fsp)
{
	if (fsp->fh->fd == -1) {
		return 0; /* What we used to call a stat open. */
	}
	if (fsp->fh->ref_count > 1) {
		return 0; /* Shared handle. Only close last reference. */
	}
	return fd_close_posix(conn, fsp);
}

/****************************************************************************
 Change the ownership of a file to that of the parent directory.
 Do this by fd if possible.
****************************************************************************/

void change_owner_to_parent(connection_struct *conn,
				files_struct *fsp,
				const char *fname,
				SMB_STRUCT_STAT *psbuf)
{
	const char *parent_path = parent_dirname(fname);
	SMB_STRUCT_STAT parent_st;
	int ret;

	ret = SMB_VFS_STAT(conn, parent_path, &parent_st);
	if (ret == -1) {
		DEBUG(0,("change_owner_to_parent: failed to stat parent "
			 "directory %s. Error was %s\n",
			 parent_path, strerror(errno) ));
		return;
	}

	if (fsp && fsp->fh->fd != -1) {
		become_root();
		ret = SMB_VFS_FCHOWN(fsp, fsp->fh->fd, parent_st.st_uid, (gid_t)-1);
		unbecome_root();
		if (ret == -1) {
			DEBUG(0,("change_owner_to_parent: failed to fchown "
				 "file %s to parent directory uid %u. Error "
				 "was %s\n", fname,
				 (unsigned int)parent_st.st_uid,
				 strerror(errno) ));
		}

		DEBUG(10,("change_owner_to_parent: changed new file %s to "
			  "parent directory uid %u.\n",	fname,
			  (unsigned int)parent_st.st_uid ));

	} else {
		/* We've already done an lstat into psbuf, and we know it's a
		   directory. If we can cd into the directory and the dev/ino
		   are the same then we can safely chown without races as
		   we're locking the directory in place by being in it.  This
		   should work on any UNIX (thanks tridge :-). JRA.
		*/

		pstring saved_dir;
		SMB_STRUCT_STAT sbuf;

		if (!vfs_GetWd(conn,saved_dir)) {
			DEBUG(0,("change_owner_to_parent: failed to get "
				 "current working directory\n"));
			return;
		}

		/* Chdir into the new path. */
		if (vfs_ChDir(conn, fname) == -1) {
			DEBUG(0,("change_owner_to_parent: failed to change "
				 "current working directory to %s. Error "
				 "was %s\n", fname, strerror(errno) ));
			goto out;
		}

		if (SMB_VFS_STAT(conn,".",&sbuf) == -1) {
			DEBUG(0,("change_owner_to_parent: failed to stat "
				 "directory '.' (%s) Error was %s\n",
				 fname, strerror(errno)));
			goto out;
		}

		/* Ensure we're pointing at the same place. */
		if (sbuf.st_dev != psbuf->st_dev ||
		    sbuf.st_ino != psbuf->st_ino ||
		    sbuf.st_mode != psbuf->st_mode ) {
			DEBUG(0,("change_owner_to_parent: "
				 "device/inode/mode on directory %s changed. "
				 "Refusing to chown !\n", fname ));
			goto out;
		}

		become_root();
		ret = SMB_VFS_CHOWN(conn, ".", parent_st.st_uid, (gid_t)-1);
		unbecome_root();
		if (ret == -1) {
			DEBUG(10,("change_owner_to_parent: failed to chown "
				  "directory %s to parent directory uid %u. "
				  "Error was %s\n", fname,
				  (unsigned int)parent_st.st_uid, strerror(errno) ));
			goto out;
		}

		DEBUG(10,("change_owner_to_parent: changed ownership of new "
			  "directory %s to parent directory uid %u.\n",
			  fname, (unsigned int)parent_st.st_uid ));

  out:

		vfs_ChDir(conn,saved_dir);
	}
}

/****************************************************************************
 Open a file.
****************************************************************************/

static BOOL open_file(files_struct *fsp,
			connection_struct *conn,
			const char *fname,
			SMB_STRUCT_STAT *psbuf,
			int flags,
			mode_t unx_mode,
			uint32 access_mask, /* client requested access mask. */
			uint32 open_access_mask) /* what we're actually using in the open. */
{
	int accmode = (flags & O_ACCMODE);
	int local_flags = flags;
	BOOL file_existed = VALID_STAT(*psbuf);

	fsp->fh->fd = -1;
	errno = EPERM;

	/* Check permissions */

	/*
	 * This code was changed after seeing a client open request 
	 * containing the open mode of (DENY_WRITE/read-only) with
	 * the 'create if not exist' bit set. The previous code
	 * would fail to open the file read only on a read-only share
	 * as it was checking the flags parameter  directly against O_RDONLY,
	 * this was failing as the flags parameter was set to O_RDONLY|O_CREAT.
	 * JRA.
	 */

	if (!CAN_WRITE(conn)) {
		/* It's a read-only share - fail if we wanted to write. */
		if(accmode != O_RDONLY) {
			DEBUG(3,("Permission denied opening %s\n",fname));
			return False;
		} else if(flags & O_CREAT) {
			/* We don't want to write - but we must make sure that
			   O_CREAT doesn't create the file if we have write
			   access into the directory.
			*/
			flags &= ~O_CREAT;
			local_flags &= ~O_CREAT;
		}
	}

	/*
	 * This little piece of insanity is inspired by the
	 * fact that an NT client can open a file for O_RDONLY,
	 * but set the create disposition to FILE_EXISTS_TRUNCATE.
	 * If the client *can* write to the file, then it expects to
	 * truncate the file, even though it is opening for readonly.
	 * Quicken uses this stupid trick in backup file creation...
	 * Thanks *greatly* to "David W. Chapman Jr." <dwcjr@inethouston.net>
	 * for helping track this one down. It didn't bite us in 2.0.x
	 * as we always opened files read-write in that release. JRA.
	 */

	if ((accmode == O_RDONLY) && ((flags & O_TRUNC) == O_TRUNC)) {
		DEBUG(10,("open_file: truncate requested on read-only open "
			  "for file %s\n",fname ));
		local_flags = (flags & ~O_ACCMODE)|O_RDWR;
	}

	if ((open_access_mask & (FILE_READ_DATA|FILE_WRITE_DATA|FILE_APPEND_DATA|FILE_EXECUTE)) ||
	    (!file_existed && (local_flags & O_CREAT)) ||
	    ((local_flags & O_TRUNC) == O_TRUNC) ) {

		/*
		 * We can't actually truncate here as the file may be locked.
		 * open_file_ntcreate will take care of the truncate later. JRA.
		 */

		local_flags &= ~O_TRUNC;

#if defined(O_NONBLOCK) && defined(S_ISFIFO)
		/*
		 * We would block on opening a FIFO with no one else on the
		 * other end. Do what we used to do and add O_NONBLOCK to the
		 * open flags. JRA.
		 */

		if (file_existed && S_ISFIFO(psbuf->st_mode)) {
			local_flags |= O_NONBLOCK;
		}
#endif

		/* Don't create files with Microsoft wildcard characters. */
		if ((local_flags & O_CREAT) && !file_existed &&
		    ms_has_wild(fname))  {
			set_saved_ntstatus(NT_STATUS_OBJECT_NAME_INVALID);
			return False;
		}

		/* Actually do the open */
		fsp->fh->fd = fd_open(conn, fname, local_flags, unx_mode);
		if (fsp->fh->fd == -1)  {
			DEBUG(3,("Error opening file %s (%s) (local_flags=%d) "
				 "(flags=%d)\n",
				 fname,strerror(errno),local_flags,flags));
			return False;
		}

		/* Inherit the ACL if the file was created. */
		if ((local_flags & O_CREAT) && !file_existed) {
			inherit_access_acl(conn, fname, unx_mode);
		}

	} else {
		fsp->fh->fd = -1; /* What we used to call a stat open. */
	}

	if (!file_existed) {
		int ret;

		if (fsp->fh->fd == -1) {
			ret = SMB_VFS_STAT(conn, fname, psbuf);
		} else {
			ret = SMB_VFS_FSTAT(fsp,fsp->fh->fd,psbuf);
			/* If we have an fd, this stat should succeed. */
			if (ret == -1) {
				DEBUG(0,("Error doing fstat on open file %s "
					 "(%s)\n", fname,strerror(errno) ));
			}
		}

		/* For a non-io open, this stat failing means file not found. JRA */
		if (ret == -1) {
			fd_close(conn, fsp);
			return False;
		}
	}

	/*
	 * POSIX allows read-only opens of directories. We don't
	 * want to do this (we use a different code path for this)
	 * so catch a directory open and return an EISDIR. JRA.
	 */

	if(S_ISDIR(psbuf->st_mode)) {
		fd_close(conn, fsp);
		errno = EISDIR;
		return False;
	}

	fsp->mode = psbuf->st_mode;
	fsp->inode = psbuf->st_ino;
	fsp->dev = psbuf->st_dev;
	fsp->vuid = current_user.vuid;
	fsp->file_pid = global_smbpid;
	fsp->can_lock = True;
	fsp->can_read = (access_mask & (FILE_READ_DATA)) ? True : False;
	if (!CAN_WRITE(conn)) {
		fsp->can_write = False;
	} else {
		fsp->can_write = (access_mask & (FILE_WRITE_DATA | FILE_APPEND_DATA)) ? True : False;
	}
	fsp->print_file = False;
	fsp->modified = False;
	fsp->sent_oplock_break = NO_BREAK_SENT;
	fsp->is_directory = False;
	fsp->is_stat = False;
	if (conn->aio_write_behind_list &&
	    is_in_path(fname, conn->aio_write_behind_list, conn->case_sensitive)) {
		fsp->aio_write_behind = True;
	}

	string_set(&fsp->fsp_name,fname);
	fsp->wcp = NULL; /* Write cache pointer. */

	DEBUG(2,("%s opened file %s read=%s write=%s (numopen=%d)\n",
		 *current_user_info.smb_name ? current_user_info.smb_name : conn->user,fsp->fsp_name,
		 BOOLSTR(fsp->can_read), BOOLSTR(fsp->can_write),
		 conn->num_files_open + 1));

	errno = 0;
	return True;
}

/*******************************************************************
 Return True if the filename is one of the special executable types.
********************************************************************/

static BOOL is_executable(const char *fname)
{
	if ((fname = strrchr_m(fname,'.'))) {
		if (strequal(fname,".com") ||
		    strequal(fname,".dll") ||
		    strequal(fname,".exe") ||
		    strequal(fname,".sym")) {
			return True;
		}
	}
	return False;
}

/****************************************************************************
 Check if we can open a file with a share mode.
 Returns True if conflict, False if not.
****************************************************************************/

static BOOL share_conflict(struct share_mode_entry *entry,
			   uint32 access_mask,
			   uint32 share_access)
{
	DEBUG(10,("share_conflict: entry->access_mask = 0x%x, "
		  "entry->share_access = 0x%x, "
		  "entry->private_options = 0x%x\n",
		  (unsigned int)entry->access_mask,
		  (unsigned int)entry->share_access,
		  (unsigned int)entry->private_options));

	DEBUG(10,("share_conflict: access_mask = 0x%x, share_access = 0x%x\n",
		  (unsigned int)access_mask, (unsigned int)share_access));

	if ((entry->access_mask & (FILE_WRITE_DATA|
				   FILE_APPEND_DATA|
				   FILE_READ_DATA|
				   FILE_EXECUTE|
				   DELETE_ACCESS)) == 0) {
		DEBUG(10,("share_conflict: No conflict due to "
			  "entry->access_mask = 0x%x\n",
			  (unsigned int)entry->access_mask ));
		return False;
	}

	if ((access_mask & (FILE_WRITE_DATA|
			    FILE_APPEND_DATA|
			    FILE_READ_DATA|
			    FILE_EXECUTE|
			    DELETE_ACCESS)) == 0) {
		DEBUG(10,("share_conflict: No conflict due to "
			  "access_mask = 0x%x\n",
			  (unsigned int)access_mask ));
		return False;
	}

#if 1 /* JRA TEST - Superdebug. */
#define CHECK_MASK(num, am, right, sa, share) \
	DEBUG(10,("share_conflict: [%d] am (0x%x) & right (0x%x) = 0x%x\n", \
		(unsigned int)(num), (unsigned int)(am), \
		(unsigned int)(right), (unsigned int)(am)&(right) )); \
	DEBUG(10,("share_conflict: [%d] sa (0x%x) & share (0x%x) = 0x%x\n", \
		(unsigned int)(num), (unsigned int)(sa), \
		(unsigned int)(share), (unsigned int)(sa)&(share) )); \
	if (((am) & (right)) && !((sa) & (share))) { \
		DEBUG(10,("share_conflict: check %d conflict am = 0x%x, right = 0x%x, \
sa = 0x%x, share = 0x%x\n", (num), (unsigned int)(am), (unsigned int)(right), (unsigned int)(sa), \
			(unsigned int)(share) )); \
		return True; \
	}
#else
#define CHECK_MASK(num, am, right, sa, share) \
	if (((am) & (right)) && !((sa) & (share))) { \
		DEBUG(10,("share_conflict: check %d conflict am = 0x%x, right = 0x%x, \
sa = 0x%x, share = 0x%x\n", (num), (unsigned int)(am), (unsigned int)(right), (unsigned int)(sa), \
			(unsigned int)(share) )); \
		return True; \
	}
#endif

	CHECK_MASK(1, entry->access_mask, FILE_WRITE_DATA | FILE_APPEND_DATA,
		   share_access, FILE_SHARE_WRITE);
	CHECK_MASK(2, access_mask, FILE_WRITE_DATA | FILE_APPEND_DATA,
		   entry->share_access, FILE_SHARE_WRITE);
	
	CHECK_MASK(3, entry->access_mask, FILE_READ_DATA | FILE_EXECUTE,
		   share_access, FILE_SHARE_READ);
	CHECK_MASK(4, access_mask, FILE_READ_DATA | FILE_EXECUTE,
		   entry->share_access, FILE_SHARE_READ);

	CHECK_MASK(5, entry->access_mask, DELETE_ACCESS,
		   share_access, FILE_SHARE_DELETE);
	CHECK_MASK(6, access_mask, DELETE_ACCESS,
		   entry->share_access, FILE_SHARE_DELETE);

	DEBUG(10,("share_conflict: No conflict.\n"));
	return False;
}

#if defined(DEVELOPER)
static void validate_my_share_entries(int num,
				      struct share_mode_entry *share_entry)
{
	files_struct *fsp;

	if (!procid_is_me(&share_entry->pid)) {
		return;
	}

	if (is_deferred_open_entry(share_entry) &&
	    !open_was_deferred(share_entry->op_mid)) {
		pstring str;
		DEBUG(0, ("Got a deferred entry without a request: "
			  "PANIC: %s\n", share_mode_str(num, share_entry)));
		smb_panic(str);
	}

	if (!is_valid_share_mode_entry(share_entry)) {
		return;
	}

	fsp = file_find_dif(share_entry->dev, share_entry->inode,
			    share_entry->share_file_id);
	if (!fsp) {
		DEBUG(0,("validate_my_share_entries: PANIC : %s\n",
			 share_mode_str(num, share_entry) ));
		smb_panic("validate_my_share_entries: Cannot match a "
			  "share entry with an open file\n");
	}

	if (is_deferred_open_entry(share_entry) ||
	    is_unused_share_mode_entry(share_entry)) {
		goto panic;
	}

	if ((share_entry->op_type == NO_OPLOCK) &&
	    (fsp->oplock_type == FAKE_LEVEL_II_OPLOCK)) {
		/* Someone has already written to it, but I haven't yet
		 * noticed */
		return;
	}

	if (((uint16)fsp->oplock_type) != share_entry->op_type) {
		goto panic;
	}

	return;

 panic:
	{
		pstring str;
		DEBUG(0,("validate_my_share_entries: PANIC : %s\n",
			 share_mode_str(num, share_entry) ));
		slprintf(str, sizeof(str)-1, "validate_my_share_entries: "
			 "file %s, oplock_type = 0x%x, op_type = 0x%x\n",
			 fsp->fsp_name, (unsigned int)fsp->oplock_type,
			 (unsigned int)share_entry->op_type );
		smb_panic(str);
	}
}
#endif

static BOOL is_stat_open(uint32 access_mask)
{
	return (access_mask &&
		((access_mask & ~(SYNCHRONIZE_ACCESS| FILE_READ_ATTRIBUTES|
				  FILE_WRITE_ATTRIBUTES))==0) &&
		((access_mask & (SYNCHRONIZE_ACCESS|FILE_READ_ATTRIBUTES|
				 FILE_WRITE_ATTRIBUTES)) != 0));
}

/****************************************************************************
 Deal with share modes
 Invarient: Share mode must be locked on entry and exit.
 Returns -1 on error, or number of share modes on success (may be zero).
****************************************************************************/

static NTSTATUS open_mode_check(connection_struct *conn,
				const char *fname,
				struct share_mode_lock *lck,
				uint32 access_mask,
				uint32 share_access,
				uint32 create_options,
				BOOL *file_existed)
{
	int i;

	if(lck->num_share_modes == 0) {
		return NT_STATUS_OK;
	}

	*file_existed = True;
	
	if (is_stat_open(access_mask)) {
		/* Stat open that doesn't trigger oplock breaks or share mode
		 * checks... ! JRA. */
		return NT_STATUS_OK;
	}

	/* A delete on close prohibits everything */

	if (lck->delete_on_close) {
		return NT_STATUS_DELETE_PENDING;
	}

	/*
	 * Check if the share modes will give us access.
	 */
	
#if defined(DEVELOPER)
	for(i = 0; i < lck->num_share_modes; i++) {
		validate_my_share_entries(i, &lck->share_modes[i]);
	}
#endif

	if (!lp_share_modes(SNUM(conn))) {
		return NT_STATUS_OK;
	}

	/* Now we check the share modes, after any oplock breaks. */
	for(i = 0; i < lck->num_share_modes; i++) {

		if (!is_valid_share_mode_entry(&lck->share_modes[i])) {
			continue;
		}

		/* someone else has a share lock on it, check to see if we can
		 * too */
		if (share_conflict(&lck->share_modes[i],
				   access_mask, share_access)) {
			return NT_STATUS_SHARING_VIOLATION;
		}
	}
	
	return NT_STATUS_OK;
}

static BOOL is_delete_request(files_struct *fsp) {
	return ((fsp->access_mask == DELETE_ACCESS) &&
		(fsp->oplock_type == NO_OPLOCK));
}

/*
 * 1) No files open at all or internal open: Grant whatever the client wants.
 *
 * 2) Exclusive (or batch) oplock around: If the requested access is a delete
 *    request, break if the oplock around is a batch oplock. If it's another
 *    requested access type, break.
 * 
 * 3) Only level2 around: Grant level2 and do nothing else.
 */

static BOOL delay_for_oplocks(struct share_mode_lock *lck,
				files_struct *fsp,
				int pass_number,
				int oplock_request)
{
	int i;
	struct share_mode_entry *exclusive = NULL;
	BOOL valid_entry = False;
	BOOL delay_it = False;
	BOOL have_level2 = False;

	if (oplock_request & INTERNAL_OPEN_ONLY) {
		fsp->oplock_type = NO_OPLOCK;
	}

	if ((oplock_request & INTERNAL_OPEN_ONLY) || is_stat_open(fsp->access_mask)) {
		return False;
	}

	for (i=0; i<lck->num_share_modes; i++) {

		if (!is_valid_share_mode_entry(&lck->share_modes[i])) {
			continue;
		}

		/* At least one entry is not an invalid or deferred entry. */
		valid_entry = True;

		if (pass_number == 1) {
			if (BATCH_OPLOCK_TYPE(lck->share_modes[i].op_type)) {
				SMB_ASSERT(exclusive == NULL);			
				exclusive = &lck->share_modes[i];
			}
		} else {
			if (EXCLUSIVE_OPLOCK_TYPE(lck->share_modes[i].op_type)) {
				SMB_ASSERT(exclusive == NULL);			
				exclusive = &lck->share_modes[i];
			}
		}

		if (lck->share_modes[i].op_type == LEVEL_II_OPLOCK) {
			SMB_ASSERT(exclusive == NULL);			
			have_level2 = True;
		}
	}

	if (!valid_entry) {
		/* All entries are placeholders or deferred.
		 * Directly grant whatever the client wants. */
		if (fsp->oplock_type == NO_OPLOCK) {
			/* Store a level2 oplock, but don't tell the client */
			fsp->oplock_type = FAKE_LEVEL_II_OPLOCK;
		}
		return False;
	}

	if (exclusive != NULL) { /* Found an exclusive oplock */
		SMB_ASSERT(!have_level2);
		delay_it = is_delete_request(fsp) ?
			BATCH_OPLOCK_TYPE(exclusive->op_type) :	True;
	}

	if (EXCLUSIVE_OPLOCK_TYPE(fsp->oplock_type)) {
		/* We can at most grant level2 as there are other
		 * level2 or NO_OPLOCK entries. */
		fsp->oplock_type = LEVEL_II_OPLOCK;
	}

	if ((fsp->oplock_type == NO_OPLOCK) && have_level2) {
		/* Store a level2 oplock, but don't tell the client */
		fsp->oplock_type = FAKE_LEVEL_II_OPLOCK;
	}

	if (delay_it) {
		BOOL ret;
		char msg[MSG_SMB_SHARE_MODE_ENTRY_SIZE];

		DEBUG(10, ("Sending break request to PID %s\n",
			   procid_str_static(&exclusive->pid)));
		exclusive->op_mid = get_current_mid();

		/* Create the message. */
		share_mode_entry_to_message(msg, exclusive);

		/* Add in the FORCE_OPLOCK_BREAK_TO_NONE bit in the message if set. We don't
		   want this set in the share mode struct pointed to by lck. */

		if (oplock_request & FORCE_OPLOCK_BREAK_TO_NONE) {
			SSVAL(msg,6,exclusive->op_type | FORCE_OPLOCK_BREAK_TO_NONE);
		}

		become_root();
		ret = message_send_pid(exclusive->pid, MSG_SMB_BREAK_REQUEST,
				       msg, MSG_SMB_SHARE_MODE_ENTRY_SIZE, True);
		unbecome_root();
		if (!ret) {
			DEBUG(3, ("Could not send oplock break message\n"));
		}
	}

	return delay_it;
}

static BOOL request_timed_out(struct timeval request_time,
			      struct timeval timeout)
{
	struct timeval now, end_time;
	GetTimeOfDay(&now);
	end_time = timeval_sum(&request_time, &timeout);
	return (timeval_compare(&end_time, &now) < 0);
}

/****************************************************************************
 Handle the 1 second delay in returning a SHARING_VIOLATION error.
****************************************************************************/

static void defer_open(struct share_mode_lock *lck,
		       struct timeval request_time,
		       struct timeval timeout,
		       struct deferred_open_record *state)
{
	uint16 mid = get_current_mid();
	int i;

	/* Paranoia check */

	for (i=0; i<lck->num_share_modes; i++) {
		struct share_mode_entry *e = &lck->share_modes[i];

		if (!is_deferred_open_entry(e)) {
			continue;
		}

		if (procid_is_me(&e->pid) && (e->op_mid == mid)) {
			DEBUG(0, ("Trying to defer an already deferred "
				  "request: mid=%d, exiting\n", mid));
			exit_server("attempt to defer a deferred request");
		}
	}

	/* End paranoia check */

	DEBUG(10,("defer_open_sharing_error: time [%u.%06u] adding deferred "
		  "open entry for mid %u\n",
		  (unsigned int)request_time.tv_sec,
		  (unsigned int)request_time.tv_usec,
		  (unsigned int)mid));

	if (!push_deferred_smb_message(mid, request_time, timeout,
				       (char *)state, sizeof(*state))) {
		exit_server("push_deferred_smb_message failed");
	}
	add_deferred_open(lck, mid, request_time, state->dev, state->inode);

	/*
	 * Push the MID of this packet on the signing queue.
	 * We only do this once, the first time we push the packet
	 * onto the deferred open queue, as this has a side effect
	 * of incrementing the response sequence number.
	 */

	srv_defer_sign_response(mid);
}

/****************************************************************************
 Set a kernel flock on a file for NFS interoperability.
 This requires a patch to Linux.
****************************************************************************/

static void kernel_flock(files_struct *fsp, uint32 share_mode)
{
#if HAVE_KERNEL_SHARE_MODES
	int kernel_mode = 0;
	if (share_mode == FILE_SHARE_WRITE) {
		kernel_mode = LOCK_MAND|LOCK_WRITE;
	} else if (share_mode == FILE_SHARE_READ) {
		kernel_mode = LOCK_MAND|LOCK_READ;
	} else if (share_mode == FILE_SHARE_NONE) {
		kernel_mode = LOCK_MAND;
	}
	if (kernel_mode) {
		flock(fsp->fh->fd, kernel_mode);
	}
#endif
	;
}

/****************************************************************************
 On overwrite open ensure that the attributes match.
****************************************************************************/

static BOOL open_match_attributes(connection_struct *conn,
				const char *path,
				uint32 old_dos_attr,
				uint32 new_dos_attr,
				mode_t existing_unx_mode,
				mode_t new_unx_mode,
				mode_t *returned_unx_mode)
{
	uint32 noarch_old_dos_attr, noarch_new_dos_attr;

	noarch_old_dos_attr = (old_dos_attr & ~FILE_ATTRIBUTE_ARCHIVE);
	noarch_new_dos_attr = (new_dos_attr & ~FILE_ATTRIBUTE_ARCHIVE);

	if((noarch_old_dos_attr == 0 && noarch_new_dos_attr != 0) || 
	   (noarch_old_dos_attr != 0 && ((noarch_old_dos_attr & noarch_new_dos_attr) == noarch_old_dos_attr))) {
		*returned_unx_mode = new_unx_mode;
	} else {
		*returned_unx_mode = (mode_t)0;
	}

	DEBUG(10,("open_match_attributes: file %s old_dos_attr = 0x%x, "
		  "existing_unx_mode = 0%o, new_dos_attr = 0x%x "
		  "returned_unx_mode = 0%o\n",
		  path,
		  (unsigned int)old_dos_attr,
		  (unsigned int)existing_unx_mode,
		  (unsigned int)new_dos_attr,
		  (unsigned int)*returned_unx_mode ));

	/* If we're mapping SYSTEM and HIDDEN ensure they match. */
	if (lp_map_system(SNUM(conn)) || lp_store_dos_attributes(SNUM(conn))) {
		if ((old_dos_attr & FILE_ATTRIBUTE_SYSTEM) &&
		    !(new_dos_attr & FILE_ATTRIBUTE_SYSTEM)) {
			return False;
		}
	}
	if (lp_map_hidden(SNUM(conn)) || lp_store_dos_attributes(SNUM(conn))) {
		if ((old_dos_attr & FILE_ATTRIBUTE_HIDDEN) &&
		    !(new_dos_attr & FILE_ATTRIBUTE_HIDDEN)) {
			return False;
		}
	}
	return True;
}

/****************************************************************************
 Special FCB or DOS processing in the case of a sharing violation.
 Try and find a duplicated file handle.
****************************************************************************/

static files_struct *fcb_or_dos_open(connection_struct *conn,
				     const char *fname, SMB_DEV_T dev,
				     SMB_INO_T inode,
				     uint32 access_mask,
				     uint32 share_access,
				     uint32 create_options)
{
	files_struct *fsp;
	files_struct *dup_fsp;

	DEBUG(5,("fcb_or_dos_open: attempting old open semantics for "
		 "file %s.\n", fname ));

	for(fsp = file_find_di_first(dev, inode); fsp;
	    fsp = file_find_di_next(fsp)) {

		DEBUG(10,("fcb_or_dos_open: checking file %s, fd = %d, "
			  "vuid = %u, file_pid = %u, private_options = 0x%x "
			  "access_mask = 0x%x\n", fsp->fsp_name,
			  fsp->fh->fd, (unsigned int)fsp->vuid,
			  (unsigned int)fsp->file_pid,
			  (unsigned int)fsp->fh->private_options,
			  (unsigned int)fsp->access_mask ));

		if (fsp->fh->fd != -1 &&
		    fsp->vuid == current_user.vuid &&
		    fsp->file_pid == global_smbpid &&
		    (fsp->fh->private_options & (NTCREATEX_OPTIONS_PRIVATE_DENY_DOS |
						 NTCREATEX_OPTIONS_PRIVATE_DENY_FCB)) &&
		    (fsp->access_mask & FILE_WRITE_DATA) &&
		    strequal(fsp->fsp_name, fname)) {
			DEBUG(10,("fcb_or_dos_open: file match\n"));
			break;
		}
	}

	if (!fsp) {
		return NULL;
	}

	/* quite an insane set of semantics ... */
	if (is_executable(fname) &&
	    (fsp->fh->private_options & NTCREATEX_OPTIONS_PRIVATE_DENY_DOS)) {
		DEBUG(10,("fcb_or_dos_open: file fail due to is_executable.\n"));
		return NULL;
	}

	/* We need to duplicate this fsp. */
	dup_fsp = dup_file_fsp(fsp, access_mask, share_access, create_options);
	if (!dup_fsp) {
		return NULL;
	}

	return dup_fsp;
}

/****************************************************************************
 Open a file with a share mode - old openX method - map into NTCreate.
****************************************************************************/

BOOL map_open_params_to_ntcreate(const char *fname, int deny_mode, int open_func,
				uint32 *paccess_mask,
				uint32 *pshare_mode,
				uint32 *pcreate_disposition,
				uint32 *pcreate_options)
{
	uint32 access_mask;
	uint32 share_mode;
	uint32 create_disposition;
	uint32 create_options = 0;

	DEBUG(10,("map_open_params_to_ntcreate: fname = %s, deny_mode = 0x%x, "
		  "open_func = 0x%x\n",
		  fname, (unsigned int)deny_mode, (unsigned int)open_func ));

	/* Create the NT compatible access_mask. */
	switch (GET_OPENX_MODE(deny_mode)) {
		case DOS_OPEN_EXEC: /* Implies read-only - used to be FILE_READ_DATA */
		case DOS_OPEN_RDONLY:
			access_mask = FILE_GENERIC_READ;
			break;
		case DOS_OPEN_WRONLY:
			access_mask = FILE_GENERIC_WRITE;
			break;
		case DOS_OPEN_RDWR:
		case DOS_OPEN_FCB:
			access_mask = FILE_GENERIC_READ|FILE_GENERIC_WRITE;
			break;
		default:
			DEBUG(10,("map_open_params_to_ntcreate: bad open mode = 0x%x\n",
				  (unsigned int)GET_OPENX_MODE(deny_mode)));
			return False;
	}

	/* Create the NT compatible create_disposition. */
	switch (open_func) {
		case OPENX_FILE_EXISTS_FAIL|OPENX_FILE_CREATE_IF_NOT_EXIST:
			create_disposition = FILE_CREATE;
			break;

		case OPENX_FILE_EXISTS_OPEN:
			create_disposition = FILE_OPEN;
			break;

		case OPENX_FILE_EXISTS_OPEN|OPENX_FILE_CREATE_IF_NOT_EXIST:
			create_disposition = FILE_OPEN_IF;
			break;
       
		case OPENX_FILE_EXISTS_TRUNCATE:
			create_disposition = FILE_OVERWRITE;
			break;

		case OPENX_FILE_EXISTS_TRUNCATE|OPENX_FILE_CREATE_IF_NOT_EXIST:
			create_disposition = FILE_OVERWRITE_IF;
			break;

		default:
			/* From samba4 - to be confirmed. */
			if (GET_OPENX_MODE(deny_mode) == DOS_OPEN_EXEC) {
				create_disposition = FILE_CREATE;
				break;
			}
			DEBUG(10,("map_open_params_to_ntcreate: bad "
				  "open_func 0x%x\n", (unsigned int)open_func));
			return False;
	}
 
	/* Create the NT compatible share modes. */
	switch (GET_DENY_MODE(deny_mode)) {
		case DENY_ALL:
			share_mode = FILE_SHARE_NONE;
			break;

		case DENY_WRITE:
			share_mode = FILE_SHARE_READ;
			break;

		case DENY_READ:
			share_mode = FILE_SHARE_WRITE;
			break;

		case DENY_NONE:
			share_mode = FILE_SHARE_READ|FILE_SHARE_WRITE;
			break;

		case DENY_DOS:
			create_options |= NTCREATEX_OPTIONS_PRIVATE_DENY_DOS;
	                if (is_executable(fname)) {
				share_mode = FILE_SHARE_READ|FILE_SHARE_WRITE;
			} else {
				if (GET_OPENX_MODE(deny_mode) == DOS_OPEN_RDONLY) {
					share_mode = FILE_SHARE_READ;
				} else {
					share_mode = FILE_SHARE_NONE;
				}
			}
			break;

		case DENY_FCB:
			create_options |= NTCREATEX_OPTIONS_PRIVATE_DENY_FCB;
			share_mode = FILE_SHARE_NONE;
			break;

		default:
			DEBUG(10,("map_open_params_to_ntcreate: bad deny_mode 0x%x\n",
				(unsigned int)GET_DENY_MODE(deny_mode) ));
			return False;
	}

	DEBUG(10,("map_open_params_to_ntcreate: file %s, access_mask = 0x%x, "
		  "share_mode = 0x%x, create_disposition = 0x%x, "
		  "create_options = 0x%x\n",
		  fname,
		  (unsigned int)access_mask,
		  (unsigned int)share_mode,
		  (unsigned int)create_disposition,
		  (unsigned int)create_options ));

	if (paccess_mask) {
		*paccess_mask = access_mask;
	}
	if (pshare_mode) {
		*pshare_mode = share_mode;
	}
	if (pcreate_disposition) {
		*pcreate_disposition = create_disposition;
	}
	if (pcreate_options) {
		*pcreate_options = create_options;
	}

	return True;

}

static void schedule_defer_open(struct share_mode_lock *lck, struct timeval request_time)
{
	struct deferred_open_record state;

	/* This is a relative time, added to the absolute
	   request_time value to get the absolute timeout time.
	   Note that if this is the second or greater time we enter
	   this codepath for this particular request mid then
	   request_time is left as the absolute time of the *first*
	   time this request mid was processed. This is what allows
	   the request to eventually time out. */

	struct timeval timeout;

	/* Normally the smbd we asked should respond within
	 * OPLOCK_BREAK_TIMEOUT seconds regardless of whether
	 * the client did, give twice the timeout as a safety
	 * measure here in case the other smbd is stuck
	 * somewhere else. */

	timeout = timeval_set(OPLOCK_BREAK_TIMEOUT*2, 0);

	/* Nothing actually uses state.delayed_for_oplocks
	   but it's handy to differentiate in debug messages
	   between a 30 second delay due to oplock break, and
	   a 1 second delay for share mode conflicts. */

	state.delayed_for_oplocks = True;
	state.dev = lck->dev;
	state.inode = lck->ino;

	if (!request_timed_out(request_time, timeout)) {
		defer_open(lck, request_time, timeout, &state);
	}
}

/****************************************************************************
 Open a file with a share mode.
****************************************************************************/

files_struct *open_file_ntcreate(connection_struct *conn,
				 const char *fname,
				 SMB_STRUCT_STAT *psbuf,
				 uint32 access_mask,		/* access bits (FILE_READ_DATA etc.) */
				 uint32 share_access,		/* share constants (FILE_SHARE_READ etc). */
				 uint32 create_disposition,	/* FILE_OPEN_IF etc. */
				 uint32 create_options,		/* options such as delete on close. */
				 uint32 new_dos_attributes,	/* attributes used for new file. */
				 int oplock_request, 		/* internal Samba oplock codes. */
				 				/* Information (FILE_EXISTS etc.) */
				 int *pinfo)
{
	int flags=0;
	int flags2=0;
	BOOL file_existed = VALID_STAT(*psbuf);
	BOOL def_acl = False;
	SMB_DEV_T dev = 0;
	SMB_INO_T inode = 0;
	BOOL fsp_open = False;
	files_struct *fsp = NULL;
	mode_t new_unx_mode = (mode_t)0;
	mode_t unx_mode = (mode_t)0;
	int info;
	uint32 existing_dos_attributes = 0;
	struct pending_message_list *pml = NULL;
	uint16 mid = get_current_mid();
	struct timeval request_time = timeval_zero();
	struct share_mode_lock *lck = NULL;
	uint32 open_access_mask = access_mask;
	NTSTATUS status;

	if (conn->printer) {
		/* 
		 * Printers are handled completely differently.
		 * Most of the passed parameters are ignored.
		 */

		if (pinfo) {
			*pinfo = FILE_WAS_CREATED;
		}

		DEBUG(10, ("open_file_ntcreate: printer open fname=%s\n", fname));

		return print_fsp_open(conn, fname);
	}

	/* We add aARCH to this as this mode is only used if the file is
	 * created new. */
	unx_mode = unix_mode(conn, new_dos_attributes | aARCH,fname, True);

	DEBUG(10, ("open_file_ntcreate: fname=%s, dos_attrs=0x%x "
		   "access_mask=0x%x share_access=0x%x "
		   "create_disposition = 0x%x create_options=0x%x "
		   "unix mode=0%o oplock_request=%d\n",
		   fname, new_dos_attributes, access_mask, share_access,
		   create_disposition, create_options, unx_mode,
		   oplock_request));

	if ((pml = get_open_deferred_message(mid)) != NULL) {
		struct deferred_open_record *state =
			(struct deferred_open_record *)pml->private_data.data;

		/* Remember the absolute time of the original
		   request with this mid. We'll use it later to
		   see if this has timed out. */

		request_time = pml->request_time;

		/* Remove the deferred open entry under lock. */
		lck = get_share_mode_lock(NULL, state->dev, state->inode, NULL, NULL);
		if (lck == NULL) {
			DEBUG(0, ("could not get share mode lock\n"));
		} else {
			del_deferred_open_entry(lck, mid);
			TALLOC_FREE(lck);
		}

		/* Ensure we don't reprocess this message. */
		remove_deferred_open_smb_message(mid);
	}

	if (!check_name(fname,conn)) {
		return NULL;
	} 

	new_dos_attributes &= SAMBA_ATTRIBUTES_MASK;
	if (file_existed) {
		existing_dos_attributes = dos_mode(conn, fname, psbuf);
	}

	/* ignore any oplock requests if oplocks are disabled */
	if (!lp_oplocks(SNUM(conn)) || global_client_failed_oplock_break ||
	    IS_VETO_OPLOCK_PATH(conn, fname)) {
		/* Mask off everything except the private Samba bits. */
		oplock_request &= SAMBA_PRIVATE_OPLOCK_MASK;
	}

	/* this is for OS/2 long file names - say we don't support them */
	if (!lp_posix_pathnames() && strstr(fname,".+,;=[].")) {
		/* OS/2 Workplace shell fix may be main code stream in a later
		 * release. */ 
		set_saved_error_triple(ERRDOS, ERRcannotopen,
				       NT_STATUS_OBJECT_NAME_NOT_FOUND);
		DEBUG(5,("open_file_ntcreate: OS/2 long filenames are not "
			 "supported.\n"));
		return NULL;
	}

	switch( create_disposition ) {
		/*
		 * Currently we're using FILE_SUPERSEDE as the same as
		 * FILE_OVERWRITE_IF but they really are
		 * different. FILE_SUPERSEDE deletes an existing file
		 * (requiring delete access) then recreates it.
		 */
		case FILE_SUPERSEDE:
			/* If file exists replace/overwrite. If file doesn't
			 * exist create. */
			flags2 |= (O_CREAT | O_TRUNC);
			break;

		case FILE_OVERWRITE_IF:
			/* If file exists replace/overwrite. If file doesn't
			 * exist create. */
			flags2 |= (O_CREAT | O_TRUNC);
			break;

		case FILE_OPEN:
			/* If file exists open. If file doesn't exist error. */
			if (!file_existed) {
				DEBUG(5,("open_file_ntcreate: FILE_OPEN "
					 "requested for file %s and file "
					 "doesn't exist.\n", fname ));
				set_saved_ntstatus(NT_STATUS_OBJECT_NAME_NOT_FOUND);
				errno = ENOENT;
				return NULL;
			}
			break;

		case FILE_OVERWRITE:
			/* If file exists overwrite. If file doesn't exist
			 * error. */
			if (!file_existed) {
				DEBUG(5,("open_file_ntcreate: FILE_OVERWRITE "
					 "requested for file %s and file "
					 "doesn't exist.\n", fname ));
				set_saved_ntstatus(NT_STATUS_OBJECT_NAME_NOT_FOUND);
				errno = ENOENT;
				return NULL;
			}
			flags2 |= O_TRUNC;
			break;

		case FILE_CREATE:
			/* If file exists error. If file doesn't exist
			 * create. */
			if (file_existed) {
				DEBUG(5,("open_file_ntcreate: FILE_CREATE "
					 "requested for file %s and file "
					 "already exists.\n", fname ));
				if (S_ISDIR(psbuf->st_mode)) {
					errno = EISDIR;
				} else {
					errno = EEXIST;
				}
				return NULL;
			}
			flags2 |= (O_CREAT|O_EXCL);
			break;

		case FILE_OPEN_IF:
			/* If file exists open. If file doesn't exist
			 * create. */
			flags2 |= O_CREAT;
			break;

		default:
			set_saved_ntstatus(NT_STATUS_INVALID_PARAMETER);
			return NULL;
	}

	/* We only care about matching attributes on file exists and
	 * overwrite. */

	if (file_existed && ((create_disposition == FILE_OVERWRITE) ||
			     (create_disposition == FILE_OVERWRITE_IF))) {
		if (!open_match_attributes(conn, fname,
					   existing_dos_attributes,
					   new_dos_attributes, psbuf->st_mode,
					   unx_mode, &new_unx_mode)) {
			DEBUG(5,("open_file_ntcreate: attributes missmatch "
				 "for file %s (%x %x) (0%o, 0%o)\n",
				 fname, existing_dos_attributes,
				 new_dos_attributes,
				 (unsigned int)psbuf->st_mode,
				 (unsigned int)unx_mode ));
			errno = EACCES;
			return NULL;
		}
	}

	/* This is a nasty hack - must fix... JRA. */
	if (access_mask == MAXIMUM_ALLOWED_ACCESS) {
		open_access_mask = access_mask = FILE_GENERIC_ALL;
	}

	/*
	 * Convert GENERIC bits to specific bits.
	 */

	se_map_generic(&access_mask, &file_generic_mapping);
	open_access_mask = access_mask;

	if (flags2 & O_TRUNC) {
		open_access_mask |= FILE_WRITE_DATA; /* This will cause oplock breaks. */
	}

	DEBUG(10, ("open_file_ntcreate: fname=%s, after mapping "
		   "access_mask=0x%x\n", fname, access_mask ));

	/*
	 * Note that we ignore the append flag as append does not
	 * mean the same thing under DOS and Unix.
	 */

	if (access_mask & (FILE_WRITE_DATA | FILE_APPEND_DATA)) {
		flags = O_RDWR;
	} else {
		flags = O_RDONLY;
	}

	/*
	 * Currently we only look at FILE_WRITE_THROUGH for create options.
	 */

#if defined(O_SYNC)
	if ((create_options & FILE_WRITE_THROUGH) && lp_strict_sync(SNUM(conn))) {
		flags2 |= O_SYNC;
	}
#endif /* O_SYNC */
  
	if (!CAN_WRITE(conn)) {
		/*
		 * We should really return a permission denied error if either
		 * O_CREAT or O_TRUNC are set, but for compatibility with
		 * older versions of Samba we just AND them out.
		 */
		flags2 &= ~(O_CREAT|O_TRUNC);
	}

	/*
	 * Ensure we can't write on a read-only share or file.
	 */

	if (flags != O_RDONLY && file_existed &&
	    (!CAN_WRITE(conn) || IS_DOS_READONLY(existing_dos_attributes))) {
		DEBUG(5,("open_file_ntcreate: write access requested for "
			 "file %s on read only %s\n",
			 fname, !CAN_WRITE(conn) ? "share" : "file" ));
		set_saved_ntstatus(NT_STATUS_ACCESS_DENIED);
		errno = EACCES;
		return NULL;
	}

	fsp = file_new(conn);
	if(!fsp) {
		return NULL;
	}

	fsp->dev = psbuf->st_dev;
	fsp->inode = psbuf->st_ino;
	fsp->share_access = share_access;
	fsp->fh->private_options = create_options;
	fsp->access_mask = open_access_mask; /* We change this to the requested access_mask after the open is done. */
	/* Ensure no SAMBA_PRIVATE bits can be set. */
	fsp->oplock_type = (oplock_request & ~SAMBA_PRIVATE_OPLOCK_MASK);

	if (timeval_is_zero(&request_time)) {
		request_time = fsp->open_time;
	}

	if (file_existed) {
		dev = psbuf->st_dev;
		inode = psbuf->st_ino;

		lck = get_share_mode_lock(NULL, dev, inode,
					conn->connectpath,
					fname);

		if (lck == NULL) {
			file_free(fsp);
			DEBUG(0, ("Could not get share mode lock\n"));
			set_saved_ntstatus(NT_STATUS_SHARING_VIOLATION);
			return NULL;
		}

		/* First pass - send break only on batch oplocks. */
		if (delay_for_oplocks(lck, fsp, 1, oplock_request)) {
			schedule_defer_open(lck, request_time);
			TALLOC_FREE(lck);
			file_free(fsp);
			return NULL;
		}

		/* Use the client requested access mask here, not the one we open with. */
		status = open_mode_check(conn, fname, lck,
					 access_mask, share_access,
					 create_options, &file_existed);

		if (NT_STATUS_IS_OK(status)) {
			/* We might be going to allow this open. Check oplock status again. */
			/* Second pass - send break for both batch or exclusive oplocks. */
			if (delay_for_oplocks(lck, fsp, 2, oplock_request)) {
				schedule_defer_open(lck, request_time);
				TALLOC_FREE(lck);
				file_free(fsp);
				return NULL;
			}
		}

		if (NT_STATUS_EQUAL(status, NT_STATUS_DELETE_PENDING)) {
			/* DELETE_PENDING is not deferred for a second */
			set_saved_ntstatus(status);
			TALLOC_FREE(lck);
			file_free(fsp);
			return NULL;
		}

		if (!NT_STATUS_IS_OK(status)) {
			uint32 can_access_mask;
			BOOL can_access = True;

			SMB_ASSERT(NT_STATUS_EQUAL(status, NT_STATUS_SHARING_VIOLATION));

			/* Check if this can be done with the deny_dos and fcb
			 * calls. */
			if (create_options &
			    (NTCREATEX_OPTIONS_PRIVATE_DENY_DOS|
			     NTCREATEX_OPTIONS_PRIVATE_DENY_FCB)) {
				files_struct *fsp_dup;

				/* Use the client requested access mask here, not the one we open with. */
				fsp_dup = fcb_or_dos_open(conn, fname, dev,
							  inode, access_mask,
							  share_access,
							  create_options);

				if (fsp_dup) {
					TALLOC_FREE(lck);
					file_free(fsp);
					if (pinfo) {
						*pinfo = FILE_WAS_OPENED;
					}
					conn->num_files_open++;
					return fsp_dup;
				}
			}

			/*
			 * This next line is a subtlety we need for
			 * MS-Access. If a file open will fail due to share
			 * permissions and also for security (access) reasons,
			 * we need to return the access failed error, not the
			 * share error. We can't open the file due to kernel
			 * oplock deadlock (it's possible we failed above on
			 * the open_mode_check()) so use a userspace check.
			 */

			if (flags & O_RDWR) {
				can_access_mask = FILE_READ_DATA|FILE_WRITE_DATA;
			} else {
				can_access_mask = FILE_READ_DATA;
			}

			if (((flags & O_RDWR) && !CAN_WRITE(conn)) ||
					!can_access_file(conn,fname,psbuf,can_access_mask)) {
				can_access = False;
			}
 
			/* 
			 * If we're returning a share violation, ensure we
			 * cope with the braindead 1 second delay.
			 */

			if (!(oplock_request & INTERNAL_OPEN_ONLY) &&
			    lp_defer_sharing_violations()) {
				struct timeval timeout;
				struct deferred_open_record state;
				int timeout_usecs;

				/* this is a hack to speed up torture tests
				   in 'make test' */
				timeout_usecs = lp_parm_int(conn->service,
							    "smbd","sharedelay",
							    SHARING_VIOLATION_USEC_WAIT);

				/* This is a relative time, added to the absolute
				   request_time value to get the absolute timeout time.
				   Note that if this is the second or greater time we enter
				   this codepath for this particular request mid then
				   request_time is left as the absolute time of the *first*
				   time this request mid was processed. This is what allows
				   the request to eventually time out. */

				timeout = timeval_set(0, timeout_usecs);

				/* Nothing actually uses state.delayed_for_oplocks
				   but it's handy to differentiate in debug messages
				   between a 30 second delay due to oplock break, and
				   a 1 second delay for share mode conflicts. */

				state.delayed_for_oplocks = False;
				state.dev = dev;
				state.inode = inode;

				if (!request_timed_out(request_time,
						       timeout)) {
					defer_open(lck, request_time, timeout,
						   &state);
				}
			}

			TALLOC_FREE(lck);
			if (!can_access) {
				set_saved_ntstatus(NT_STATUS_ACCESS_DENIED);
			} else {
				/*
				 * We have detected a sharing violation here
				 * so return the correct error code
				 */
				set_saved_ntstatus(NT_STATUS_SHARING_VIOLATION);
			}
			file_free(fsp);
			return NULL;
		}

		/*
		 * We exit this block with the share entry *locked*.....
		 */
	}

	SMB_ASSERT(!file_existed || (lck != NULL));

	/*
	 * Ensure we pay attention to default ACLs on directories if required.
	 */

        if ((flags2 & O_CREAT) && lp_inherit_acls(SNUM(conn)) &&
	    (def_acl = directory_has_default_acl(conn,
						 parent_dirname(fname)))) {
		unx_mode = 0777;
	}

	DEBUG(4,("calling open_file with flags=0x%X flags2=0x%X mode=0%o, "
		"access_mask = 0x%x, open_access_mask = 0x%x\n",
		 (unsigned int)flags, (unsigned int)flags2,
		 (unsigned int)unx_mode, (unsigned int)access_mask,
		 (unsigned int)open_access_mask));

	/*
	 * open_file strips any O_TRUNC flags itself.
	 */

	fsp_open = open_file(fsp,conn,fname,psbuf,flags|flags2,unx_mode,
				access_mask, open_access_mask);

	if (!fsp_open) {
		if (lck != NULL) {
			TALLOC_FREE(lck);
		}
		file_free(fsp);
		return NULL;
	}

	if (!file_existed) {

		/*
		 * Deal with the race condition where two smbd's detect the
		 * file doesn't exist and do the create at the same time. One
		 * of them will win and set a share mode, the other (ie. this
		 * one) should check if the requested share mode for this
		 * create is allowed.
		 */

		/*
		 * Now the file exists and fsp is successfully opened,
		 * fsp->dev and fsp->inode are valid and should replace the
		 * dev=0,inode=0 from a non existent file. Spotted by
		 * Nadav Danieli <nadavd@exanet.com>. JRA.
		 */

		dev = fsp->dev;
		inode = fsp->inode;

		lck = get_share_mode_lock(NULL, dev, inode,
					conn->connectpath,
					fname);

		if (lck == NULL) {
			DEBUG(0, ("open_file_ntcreate: Could not get share mode lock for %s\n", fname));
			fd_close(conn, fsp);
			file_free(fsp);
			set_saved_ntstatus(NT_STATUS_SHARING_VIOLATION);
			return NULL;
		}

		status = open_mode_check(conn, fname, lck,
					 access_mask, share_access,
					 create_options, &file_existed);

		if (!NT_STATUS_IS_OK(status)) {
			struct deferred_open_record state;

			fd_close(conn, fsp);
			file_free(fsp);

			state.delayed_for_oplocks = False;
			state.dev = dev;
			state.inode = inode;

			/* Do it all over again immediately. In the second
			 * round we will find that the file existed and handle
			 * the DELETE_PENDING and FCB cases correctly. No need
			 * to duplicate the code here. Essentially this is a
			 * "goto top of this function", but don't tell
			 * anybody... */

			defer_open(lck, request_time, timeval_zero(),
				   &state);
			TALLOC_FREE(lck);
			return NULL;
		}

		/*
		 * We exit this block with the share entry *locked*.....
		 */

	}

	SMB_ASSERT(lck != NULL);

	/* note that we ignore failure for the following. It is
           basically a hack for NFS, and NFS will never set one of
           these only read them. Nobody but Samba can ever set a deny
           mode and we have already checked our more authoritative
           locking database for permission to set this deny mode. If
           the kernel refuses the operations then the kernel is wrong */

	kernel_flock(fsp, share_access);

	/*
	 * At this point onwards, we can guarentee that the share entry
	 * is locked, whether we created the file or not, and that the
	 * deny mode is compatible with all current opens.
	 */

	/*
	 * If requested, truncate the file.
	 */

	if (flags2&O_TRUNC) {
		/*
		 * We are modifing the file after open - update the stat
		 * struct..
		 */
		if ((SMB_VFS_FTRUNCATE(fsp,fsp->fh->fd,0) == -1) ||
		    (SMB_VFS_FSTAT(fsp,fsp->fh->fd,psbuf)==-1)) {
			TALLOC_FREE(lck);
			fd_close(conn,fsp);
			file_free(fsp);
			return NULL;
		}
	}

	/* Record the options we were opened with. */
	fsp->share_access = share_access;
	fsp->fh->private_options = create_options;
	fsp->access_mask = access_mask;

	if (file_existed) {
		/* stat opens on existing files don't get oplocks. */
		if (is_stat_open(open_access_mask)) {
			fsp->oplock_type = NO_OPLOCK;
		}

		if (!(flags2 & O_TRUNC)) {
			info = FILE_WAS_OPENED;
		} else {
			info = FILE_WAS_OVERWRITTEN;
		}
	} else {
		info = FILE_WAS_CREATED;
		/* Change the owner if required. */
		if (lp_inherit_owner(SNUM(conn))) {
			change_owner_to_parent(conn, fsp, fsp->fsp_name,
					       psbuf);
		}
	}

	if (pinfo) {
		*pinfo = info;
	}

	/* 
	 * Setup the oplock info in both the shared memory and
	 * file structs.
	 */

	if ((fsp->oplock_type != NO_OPLOCK) &&
	    (fsp->oplock_type != FAKE_LEVEL_II_OPLOCK)) {
		if (!set_file_oplock(fsp, fsp->oplock_type)) {
			/* Could not get the kernel oplock */
			fsp->oplock_type = NO_OPLOCK;
		}
	}
	set_share_mode(lck, fsp, current_user.ut.uid, 0, fsp->oplock_type);

	if (info == FILE_WAS_OVERWRITTEN || info == FILE_WAS_CREATED ||
				info == FILE_WAS_SUPERSEDED) {

		/* Handle strange delete on close create semantics. */
		if (create_options & FILE_DELETE_ON_CLOSE) {
			NTSTATUS result = can_set_delete_on_close(fsp, True, new_dos_attributes);

			if (!NT_STATUS_IS_OK(result)) {
				/* Remember to delete the mode we just added. */
				del_share_mode(lck, fsp);
				TALLOC_FREE(lck);
				fd_close(conn,fsp);
				file_free(fsp);
				set_saved_ntstatus(result);
				return NULL;
			}
			/* Note that here we set the *inital* delete on close flag,
			   not the regular one. */
			set_delete_on_close_token(lck, &current_user.ut);
			lck->initial_delete_on_close = True;
			lck->modified = True;
		}
	
		/* Files should be initially set as archive */
		if (lp_map_archive(SNUM(conn)) ||
		    lp_store_dos_attributes(SNUM(conn))) {
			file_set_dosmode(conn, fname,
					 new_dos_attributes | aARCH, NULL,
					 True);
		}
	}

	/*
	 * Take care of inherited ACLs on created files - if default ACL not
	 * selected.
	 */

	if (!file_existed && !def_acl) {

		int saved_errno = errno; /* We might get ENOSYS in the next
					  * call.. */

		if (SMB_VFS_FCHMOD_ACL(fsp, fsp->fh->fd, unx_mode) == -1
		    && errno == ENOSYS) {
			errno = saved_errno; /* Ignore ENOSYS */
		}

	} else if (new_unx_mode) {

		int ret = -1;

		/* Attributes need changing. File already existed. */

		{
			int saved_errno = errno; /* We might get ENOSYS in the
						  * next call.. */
			ret = SMB_VFS_FCHMOD_ACL(fsp, fsp->fh->fd,
						 new_unx_mode);

			if (ret == -1 && errno == ENOSYS) {
				errno = saved_errno; /* Ignore ENOSYS */
			} else {
				DEBUG(5, ("open_file_ntcreate: reset "
					  "attributes of file %s to 0%o\n",
					fname, (unsigned int)new_unx_mode));
				ret = 0; /* Don't do the fchmod below. */
			}
		}

		if ((ret == -1) &&
		    (SMB_VFS_FCHMOD(fsp, fsp->fh->fd, new_unx_mode) == -1))
			DEBUG(5, ("open_file_ntcreate: failed to reset "
				  "attributes of file %s to 0%o\n",
				fname, (unsigned int)new_unx_mode));
	}

	/* If this is a successful open, we must remove any deferred open
	 * records. */
	del_deferred_open_entry(lck, mid);
	TALLOC_FREE(lck);

	conn->num_files_open++;

	return fsp;
}

/****************************************************************************
 Open a file for for write to ensure that we can fchmod it.
****************************************************************************/

files_struct *open_file_fchmod(connection_struct *conn, const char *fname,
			       SMB_STRUCT_STAT *psbuf)
{
	files_struct *fsp = NULL;
	BOOL fsp_open;

	if (!VALID_STAT(*psbuf)) {
		return NULL;
	}

	fsp = file_new(conn);
	if(!fsp) {
		return NULL;
	}

	/* note! we must use a non-zero desired access or we don't get
           a real file descriptor. Oh what a twisted web we weave. */
	fsp_open = open_file(fsp,conn,fname,psbuf,O_WRONLY,0,FILE_WRITE_DATA,FILE_WRITE_DATA);

	/* 
	 * This is not a user visible file open.
	 * Don't set a share mode and don't increment
	 * the conn->num_files_open.
	 */

	if (!fsp_open) {
		file_free(fsp);
		return NULL;
	}

	return fsp;
}

/****************************************************************************
 Close the fchmod file fd - ensure no locks are lost.
****************************************************************************/

int close_file_fchmod(files_struct *fsp)
{
	int ret = fd_close(fsp->conn, fsp);
	file_free(fsp);
	return ret;
}

/****************************************************************************
 Open a directory from an NT SMB call.
****************************************************************************/

files_struct *open_directory(connection_struct *conn,
				const char *fname,
				SMB_STRUCT_STAT *psbuf,
				uint32 access_mask,
				uint32 share_access,
				uint32 create_disposition,
				uint32 create_options,
				int *pinfo)
{
	files_struct *fsp = NULL;
	BOOL dir_existed = VALID_STAT(*psbuf) ? True : False;
	BOOL create_dir = False;
	struct share_mode_lock *lck = NULL;
	NTSTATUS status;
	int info = 0;

	DEBUG(5,("open_directory: opening directory %s, access_mask = 0x%x, "
		 "share_access = 0x%x create_options = 0x%x, "
		 "create_disposition = 0x%x\n",
		 fname,
		 (unsigned int)access_mask,
		 (unsigned int)share_access,
		 (unsigned int)create_options,
		 (unsigned int)create_disposition));

	if (is_ntfs_stream_name(fname)) {
		DEBUG(0,("open_directory: %s is a stream name!\n", fname ));
		set_saved_ntstatus(NT_STATUS_NOT_A_DIRECTORY);
		return NULL;
	}

	switch( create_disposition ) {
		case FILE_OPEN:
			/* If directory exists open. If directory doesn't
			 * exist error. */
			if (!dir_existed) {
				DEBUG(5,("open_directory: FILE_OPEN requested "
					 "for directory %s and it doesn't "
					 "exist.\n", fname ));
				set_saved_ntstatus(NT_STATUS_OBJECT_NAME_NOT_FOUND);
				return NULL;
			}
			info = FILE_WAS_OPENED;
			break;

		case FILE_CREATE:
			/* If directory exists error. If directory doesn't
			 * exist create. */
			if (dir_existed) {
				DEBUG(5,("open_directory: FILE_CREATE "
					 "requested for directory %s and it "
					 "already exists.\n", fname ));
				set_saved_error_triple(ERRDOS, ERRfilexists,
						       NT_STATUS_OBJECT_NAME_COLLISION);
				return NULL;
			}
			create_dir = True;
			info = FILE_WAS_CREATED;
			break;

		case FILE_OPEN_IF:
			/* If directory exists open. If directory doesn't
			 * exist create. */
			if (!dir_existed) {
				create_dir = True;
				info = FILE_WAS_CREATED;
			} else {
				info = FILE_WAS_OPENED;
			}
			break;

		case FILE_SUPERSEDE:
		case FILE_OVERWRITE:
		case FILE_OVERWRITE_IF:
		default:
			DEBUG(5,("open_directory: invalid create_disposition "
				 "0x%x for directory %s\n",
				 (unsigned int)create_disposition, fname));
			set_saved_ntstatus(NT_STATUS_INVALID_PARAMETER);
			return NULL;
	}

	if (create_dir) {
		/*
		 * Try and create the directory.
		 */

		/* We know bad_path is false as it's caught earlier. */

		status = mkdir_internal(conn, fname, False);

		if (!NT_STATUS_IS_OK(status)) {
			DEBUG(2,("open_directory: unable to create %s. "
				 "Error was %s\n", fname, strerror(errno) ));
			/* Ensure we return the correct NT status to the
			 * client. */
			set_saved_error_triple(0, 0, status);
			return NULL;
		}

		/* Ensure we're checking for a symlink here.... */
		/* We don't want to get caught by a symlink racer. */

		if(SMB_VFS_LSTAT(conn,fname, psbuf) != 0) {
			return NULL;
		}

		if(!S_ISDIR(psbuf->st_mode)) {
			DEBUG(0,("open_directory: %s is not a directory !\n",
				 fname ));
			return NULL;
		}
	}

	fsp = file_new(conn);
	if(!fsp) {
		return NULL;
	}

	/*
	 * Setup the files_struct for it.
	 */
	
	fsp->mode = psbuf->st_mode;
	fsp->inode = psbuf->st_ino;
	fsp->dev = psbuf->st_dev;
	fsp->vuid = current_user.vuid;
	fsp->file_pid = global_smbpid;
	fsp->can_lock = True;
	fsp->can_read = False;
	fsp->can_write = False;

	fsp->share_access = share_access;
	fsp->fh->private_options = create_options;
	fsp->access_mask = access_mask;

	fsp->print_file = False;
	fsp->modified = False;
	fsp->oplock_type = NO_OPLOCK;
	fsp->sent_oplock_break = NO_BREAK_SENT;
	fsp->is_directory = True;
	fsp->is_stat = False;
	string_set(&fsp->fsp_name,fname);

	lck = get_share_mode_lock(NULL, fsp->dev, fsp->inode,
				conn->connectpath,
				fname);

	if (lck == NULL) {
		DEBUG(0, ("open_directory: Could not get share mode lock for %s\n", fname));
		file_free(fsp);
		set_saved_ntstatus(NT_STATUS_SHARING_VIOLATION);
		return NULL;
	}

	status = open_mode_check(conn, fname, lck,
				access_mask, share_access,
				create_options, &dir_existed);

	if (!NT_STATUS_IS_OK(status)) {
		set_saved_ntstatus(status);
		TALLOC_FREE(lck);
		file_free(fsp);
		return NULL;
	}

	set_share_mode(lck, fsp, current_user.ut.uid, 0, NO_OPLOCK);

	/* For directories the delete on close bit at open time seems
	   always to be honored on close... See test 19 in Samba4 BASE-DELETE. */
	if (create_options & FILE_DELETE_ON_CLOSE) {
		status = can_set_delete_on_close(fsp, True, 0);
		if (!NT_STATUS_IS_OK(status)) {
			set_saved_ntstatus(status);
			TALLOC_FREE(lck);
			file_free(fsp);
			return NULL;
		}

		set_delete_on_close_token(lck, &current_user.ut);
		lck->initial_delete_on_close = True;
		lck->modified = True;
	}

	TALLOC_FREE(lck);

	/* Change the owner if required. */
	if ((info == FILE_WAS_CREATED) && lp_inherit_owner(SNUM(conn))) {
		change_owner_to_parent(conn, fsp, fsp->fsp_name, psbuf);
	}

	if (pinfo) {
		*pinfo = info;
	}

	conn->num_files_open++;

	return fsp;
}

/****************************************************************************
 Open a pseudo-file (no locking checks - a 'stat' open).
****************************************************************************/

files_struct *open_file_stat(connection_struct *conn, char *fname,
			     SMB_STRUCT_STAT *psbuf)
{
	files_struct *fsp = NULL;

	if (!VALID_STAT(*psbuf))
		return NULL;

	/* Can't 'stat' open directories. */
	if(S_ISDIR(psbuf->st_mode))
		return NULL;

	fsp = file_new(conn);
	if(!fsp)
		return NULL;

	DEBUG(5,("open_file_stat: 'opening' file %s\n", fname));

	/*
	 * Setup the files_struct for it.
	 */
	
	fsp->mode = psbuf->st_mode;
	fsp->inode = psbuf->st_ino;
	fsp->dev = psbuf->st_dev;
	fsp->vuid = current_user.vuid;
	fsp->file_pid = global_smbpid;
	fsp->can_lock = False;
	fsp->can_read = False;
	fsp->can_write = False;
	fsp->print_file = False;
	fsp->modified = False;
	fsp->oplock_type = NO_OPLOCK;
	fsp->sent_oplock_break = NO_BREAK_SENT;
	fsp->is_directory = False;
	fsp->is_stat = True;
	string_set(&fsp->fsp_name,fname);

	conn->num_files_open++;

	return fsp;
}

/****************************************************************************
 Receive notification that one of our open files has been renamed by another
 smbd process.
****************************************************************************/

void msg_file_was_renamed(int msg_type, struct process_id src, void *buf, size_t len)
{
	files_struct *fsp;
	char *frm = (char *)buf;
	SMB_DEV_T dev;
	SMB_INO_T inode;
	const char *sharepath;
	const char *newname;
	size_t sp_len;

	if (buf == NULL || len < MSG_FILE_RENAMED_MIN_SIZE + 2) {
                DEBUG(0, ("msg_file_was_renamed: Got invalid msg len %d\n", (int)len));
                return;
        }

	/* Unpack the message. */
	dev = DEV_T_VAL(frm,0);
	inode = INO_T_VAL(frm,8);
	sharepath = &frm[16];
	newname = sharepath + strlen(sharepath) + 1;
	sp_len = strlen(sharepath);

	DEBUG(10,("msg_file_was_renamed: Got rename message for sharepath %s, new name %s, "
		"dev %x, inode  %.0f\n",
		sharepath, newname, (unsigned int)dev, (double)inode ));

	for(fsp = file_find_di_first(dev, inode); fsp; fsp = file_find_di_next(fsp)) {
		if (memcmp(fsp->conn->connectpath, sharepath, sp_len) == 0) {
	                DEBUG(10,("msg_file_was_renamed: renaming file fnum %d from %s -> %s\n",
				fsp->fnum, fsp->fsp_name, newname ));
			string_set(&fsp->fsp_name, newname);
		} else {
			/* TODO. JRA. */
			/* Now we have the complete path we can work out if this is
			   actually within this share and adjust newname accordingly. */
	                DEBUG(10,("msg_file_was_renamed: share mismatch (sharepath %s "
				"not sharepath %s) "
				"fnum %d from %s -> %s\n",
				fsp->conn->connectpath,
				sharepath,
				fsp->fnum,
				fsp->fsp_name,
				newname ));
		}
        }
}

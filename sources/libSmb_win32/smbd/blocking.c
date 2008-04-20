/* 
   Unix SMB/CIFS implementation.
   Blocking Locking functions
   Copyright (C) Jeremy Allison 1998-2003
   
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

/****************************************************************************
 This is the structure to queue to implement blocking locks.
 notify. It consists of the requesting SMB and the expiry time.
*****************************************************************************/

typedef struct _blocking_lock_record {
	struct _blocking_lock_record *next;
	struct _blocking_lock_record *prev;
	int com_type;
	files_struct *fsp;
	time_t expire_time;
	int lock_num;
	SMB_BIG_UINT offset;
	SMB_BIG_UINT count;
	uint16 lock_pid;
	enum brl_flavour lock_flav;
	enum brl_type lock_type;
	char *inbuf;
	int length;
} blocking_lock_record;

static blocking_lock_record *blocking_lock_queue;

/****************************************************************************
 Destructor for the above structure.
****************************************************************************/

static void free_blocking_lock_record(blocking_lock_record *blr)
{
	DLIST_REMOVE(blocking_lock_queue, blr);
	SAFE_FREE(blr->inbuf);
	SAFE_FREE(blr);
}

/****************************************************************************
 Determine if this is a secondary element of a chained SMB.
  **************************************************************************/

static BOOL in_chained_smb(void)
{
	return (chain_size != 0);
}

static void received_unlock_msg(int msg_type, struct process_id src,
				void *buf, size_t len);

/****************************************************************************
 Function to push a blocking lock request onto the lock queue.
****************************************************************************/

BOOL push_blocking_lock_request( char *inbuf, int length,
		files_struct *fsp,
		int lock_timeout,
		int lock_num,
		uint16 lock_pid,
		enum brl_type lock_type,
		enum brl_flavour lock_flav,
		SMB_BIG_UINT offset, SMB_BIG_UINT count)
{
	static BOOL set_lock_msg;
	blocking_lock_record *blr, *tmp;
	BOOL my_lock_ctx = False;
	struct byte_range_lock *br_lck = NULL;
	NTSTATUS status;

	if(in_chained_smb() ) {
		DEBUG(0,("push_blocking_lock_request: cannot queue a chained request (currently).\n"));
		return False;
	}

	/*
	 * Now queue an entry on the blocking lock queue. We setup
	 * the expiration time here.
	 */

	if((blr = SMB_MALLOC_P(blocking_lock_record)) == NULL) {
		DEBUG(0,("push_blocking_lock_request: Malloc fail !\n" ));
		return False;
	}

	blr->next = NULL;
	blr->prev = NULL;

	if((blr->inbuf = (char *)SMB_MALLOC(length)) == NULL) {
		DEBUG(0,("push_blocking_lock_request: Malloc fail (2)!\n" ));
		SAFE_FREE(blr);
		return False;
	}

	blr->com_type = CVAL(inbuf,smb_com);
	blr->fsp = fsp;
	blr->expire_time = (lock_timeout == -1) ? (time_t)-1 : time(NULL) + (time_t)lock_timeout;
	blr->lock_num = lock_num;
	blr->lock_pid = lock_pid;
	blr->lock_flav = lock_flav;
	blr->lock_type = lock_type;
	blr->offset = offset;
	blr->count = count;
	memcpy(blr->inbuf, inbuf, length);
	blr->length = length;

	br_lck = brl_get_locks(blr->fsp);
	if (!br_lck) {
		free_blocking_lock_record(blr);
		return False;
	}

	/* Add a pending lock record for this. */
	status = brl_lock(br_lck,
			lock_pid,
			procid_self(),
			offset,
			count,
			PENDING_LOCK,
			blr->lock_flav,
			&my_lock_ctx);
	byte_range_lock_destructor(br_lck);

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(0,("push_blocking_lock_request: failed to add PENDING_LOCK record.\n"));
		free_blocking_lock_record(blr);
		return False;
	}

	DLIST_ADD_END(blocking_lock_queue, blr, tmp);

	/* Ensure we'll receive messages when this is unlocked. */
	if (!set_lock_msg) {
		message_register(MSG_SMB_UNLOCK, received_unlock_msg);
		set_lock_msg = True;
	}

	DEBUG(3,("push_blocking_lock_request: lock request length=%d blocked with expiry time %d (+%d) \
for fnum = %d, name = %s\n", length, (int)blr->expire_time, lock_timeout,
		blr->fsp->fnum, blr->fsp->fsp_name ));

	/* Push the MID of this packet on the signing queue. */
	srv_defer_sign_response(SVAL(inbuf,smb_mid));

	return True;
}

/****************************************************************************
 Return a smd with a given size.
*****************************************************************************/

static void send_blocking_reply(char *outbuf, int outsize)
{
	if(outsize > 4)
		smb_setlen(outbuf,outsize - 4);

	if (!send_smb(smbd_server_fd(),outbuf))
		exit_server("send_blocking_reply: send_smb failed.");
}

/****************************************************************************
 Return a lockingX success SMB.
*****************************************************************************/

static void reply_lockingX_success(blocking_lock_record *blr)
{
	char *outbuf = get_OutBuffer();
	int bufsize = BUFFER_SIZE;
	char *inbuf = blr->inbuf;
	int outsize = 0;

	construct_reply_common(inbuf, outbuf);
	set_message(outbuf,2,0,True);

	/*
	 * As this message is a lockingX call we must handle
	 * any following chained message correctly.
	 * This is normally handled in construct_reply(),
	 * but as that calls switch_message, we can't use
	 * that here and must set up the chain info manually.
	 */

	outsize = chain_reply(inbuf,outbuf,blr->length,bufsize);

	outsize += chain_size;

	send_blocking_reply(outbuf,outsize);
}

/****************************************************************************
 Return a generic lock fail error blocking call.
*****************************************************************************/

static void generic_blocking_lock_error(blocking_lock_record *blr, NTSTATUS status)
{
	char *outbuf = get_OutBuffer();
	char *inbuf = blr->inbuf;
	construct_reply_common(inbuf, outbuf);

	/* whenever a timeout is given w2k maps LOCK_NOT_GRANTED to
	   FILE_LOCK_CONFLICT! (tridge) */
	if (NT_STATUS_EQUAL(status, NT_STATUS_LOCK_NOT_GRANTED)) {
		status = NT_STATUS_FILE_LOCK_CONFLICT;
	}

	ERROR_NT(status);
	if (!send_smb(smbd_server_fd(),outbuf))
		exit_server("generic_blocking_lock_error: send_smb failed.");
}

/****************************************************************************
 Return a lock fail error for a lockingX call. Undo all the locks we have 
 obtained first.
*****************************************************************************/

static void reply_lockingX_error(blocking_lock_record *blr, NTSTATUS status)
{
	char *inbuf = blr->inbuf;
	files_struct *fsp = blr->fsp;
	uint16 num_ulocks = SVAL(inbuf,smb_vwv6);
	SMB_BIG_UINT count = (SMB_BIG_UINT)0, offset = (SMB_BIG_UINT) 0;
	uint16 lock_pid;
	unsigned char locktype = CVAL(inbuf,smb_vwv3);
	BOOL large_file_format = (locktype & LOCKING_ANDX_LARGE_FILES);
	char *data;
	int i;

	data = smb_buf(inbuf) + ((large_file_format ? 20 : 10)*num_ulocks);
	
	/* 
	 * Data now points at the beginning of the list
	 * of smb_lkrng structs.
	 */

	/*
	 * Ensure we don't do a remove on the lock that just failed,
	 * as under POSIX rules, if we have a lock already there, we
	 * will delete it (and we shouldn't) .....
	 */
	
	for(i = blr->lock_num - 1; i >= 0; i--) {
		BOOL err;
		
		lock_pid = get_lock_pid( data, i, large_file_format);
		count = get_lock_count( data, i, large_file_format);
		offset = get_lock_offset( data, i, large_file_format, &err);
		
		/*
		 * We know err cannot be set as if it was the lock
		 * request would never have been queued. JRA.
		 */
		
		do_unlock(fsp,
			lock_pid,
			count,
			offset,
			WINDOWS_LOCK);
	}
	
	generic_blocking_lock_error(blr, status);
}

/****************************************************************************
 Return a lock fail error.
*****************************************************************************/

static void blocking_lock_reply_error(blocking_lock_record *blr, NTSTATUS status)
{
	switch(blr->com_type) {
#if 0
	/* We no longer push blocking lock requests for anything but lockingX and trans2. */
	case SMBlock:
	case SMBlockread:
		generic_blocking_lock_error(blr, status);
		break;
#endif
	case SMBlockingX:
		reply_lockingX_error(blr, status);
		break;
	case SMBtrans2:
	case SMBtranss2:
		{
			char *outbuf = get_OutBuffer();
			char *inbuf = blr->inbuf;
			construct_reply_common(inbuf, outbuf);
			/* construct_reply_common has done us the favor to pre-fill the
			 * command field with SMBtranss2 which is wrong :-)
			 */
			SCVAL(outbuf,smb_com,SMBtrans2);
			ERROR_NT(status);
			if (!send_smb(smbd_server_fd(),outbuf)) {
				exit_server("blocking_lock_reply_error: send_smb failed.");
			}
			break;
		}
	default:
		DEBUG(0,("blocking_lock_reply_error: PANIC - unknown type on blocking lock queue - exiting.!\n"));
		exit_server("PANIC - unknown type on blocking lock queue");
	}
}

#if 0
/* We no longer push blocking lock requests for anything but lockingX and trans2. */

/****************************************************************************
 Attempt to finish off getting all pending blocking locks for a lockread call.
 Returns True if we want to be removed from the list.
*****************************************************************************/

static BOOL process_lockread(blocking_lock_record *blr)
{
	char *outbuf = get_OutBuffer();
	char *inbuf = blr->inbuf;
	ssize_t nread = -1;
	char *data, *p;
	int outsize = 0;
	SMB_BIG_UINT startpos;
	size_t numtoread;
	NTSTATUS status;
	files_struct *fsp = blr->fsp;
	BOOL my_lock_ctx = False;

	numtoread = SVAL(inbuf,smb_vwv1);
	startpos = (SMB_BIG_UINT)IVAL(inbuf,smb_vwv2);
	
	numtoread = MIN(BUFFER_SIZE-outsize,numtoread);
	data = smb_buf(outbuf) + 3;
 
	status = do_lock_spin(fsp,
				SVAL(inbuf,smb_pid),
				(SMB_BIG_UINT)numtoread,
				startpos,
				READ_LOCK,
				WINDOWS_LOCK,
				&my_lock_ctx);

	if (NT_STATUS_V(status)) {
		if (!NT_STATUS_EQUAL(status,NT_STATUS_LOCK_NOT_GRANTED) &&
			!NT_STATUS_EQUAL(status,NT_STATUS_FILE_LOCK_CONFLICT)) {
			/*
			 * We have other than a "can't get lock"
			 * error. Send an error.
			 * Return True so we get dequeued.
			 */
			generic_blocking_lock_error(blr, status);
			return True;
		}

		/*
		 * Still waiting for lock....
		 */
		
		DEBUG(10,("process_lockread: failed to get lock for file = %s. Still waiting....\n",
			  fsp->fsp_name));
		return False;
	}

	nread = read_file(fsp,data,startpos,numtoread);

	if (nread < 0) {
		generic_blocking_lock_error(blr,NT_STATUS_ACCESS_DENIED);
		return True;
	}
	
	construct_reply_common(inbuf, outbuf);
	outsize = set_message(outbuf,5,0,True);
	
	outsize += nread;
	SSVAL(outbuf,smb_vwv0,nread);
	SSVAL(outbuf,smb_vwv5,nread+3);
	p = smb_buf(outbuf);
	*p++ = 1;
	SSVAL(p,0,nread); p += 2;
	set_message_end(outbuf, p+nread);
	
	DEBUG(3, ( "process_lockread file = %s, fnum=%d num=%lu nread=%ld\n",
		   fsp->fsp_name, fsp->fnum, (unsigned long)numtoread, (long)nread ) );
	
	send_blocking_reply(outbuf,outsize);
	return True;
}

/****************************************************************************
 Attempt to finish off getting all pending blocking locks for a lock call.
 Returns True if we want to be removed from the list.
*****************************************************************************/

static BOOL process_lock(blocking_lock_record *blr)
{
	char *outbuf = get_OutBuffer();
	char *inbuf = blr->inbuf;
	int outsize;
	SMB_BIG_UINT count = (SMB_BIG_UINT)0, offset = (SMB_BIG_UINT)0;
	NTSTATUS status;
	files_struct *fsp = blr->fsp;
	BOOL my_lock_ctx = False;

	count = IVAL_TO_SMB_OFF_T(inbuf,smb_vwv1);
	offset = IVAL_TO_SMB_OFF_T(inbuf,smb_vwv3);

	errno = 0;
	status = do_lock_spin(fsp,
				SVAL(inbuf,smb_pid),
				count,
				offset,
				WRITE_LOCK,
				WINDOWS_LOCK,
				&my_lock_ctx);

	if (NT_STATUS_IS_ERR(status)) {
		if (!NT_STATUS_EQUAL(status,NT_STATUS_LOCK_NOT_GRANTED) &&
			!NT_STATUS_EQUAL(status,NT_STATUS_FILE_LOCK_CONFLICT)) {
			/*
			 * We have other than a "can't get lock"
			 * error. Send an error.
			 * Return True so we get dequeued.
			 */
			
			blocking_lock_reply_error(blr, status);
			return True;
		}
		/*
		 * Still can't get the lock - keep waiting.
		 */
		DEBUG(10,("process_lock: failed to get lock for file = %s. Still waiting....\n",
			  fsp->fsp_name));
		return False;
	}

	/*
	 * Success - we got the lock.
	 */
	
	DEBUG(3,("process_lock : file=%s fnum=%d offset=%.0f count=%.0f\n",
		 fsp->fsp_name, fsp->fnum, (double)offset, (double)count));
	
	construct_reply_common(inbuf, outbuf);
	outsize = set_message(outbuf,0,0,True);
	send_blocking_reply(outbuf,outsize);
	return True;
}
#endif

/****************************************************************************
 Attempt to finish off getting all pending blocking locks for a lockingX call.
 Returns True if we want to be removed from the list.
*****************************************************************************/

static BOOL process_lockingX(blocking_lock_record *blr)
{
	char *inbuf = blr->inbuf;
	unsigned char locktype = CVAL(inbuf,smb_vwv3);
	files_struct *fsp = blr->fsp;
	uint16 num_ulocks = SVAL(inbuf,smb_vwv6);
	uint16 num_locks = SVAL(inbuf,smb_vwv7);
	SMB_BIG_UINT count = (SMB_BIG_UINT)0, offset = (SMB_BIG_UINT)0;
	uint16 lock_pid;
	BOOL large_file_format = (locktype & LOCKING_ANDX_LARGE_FILES);
	char *data;
	BOOL my_lock_ctx = False;
	NTSTATUS status = NT_STATUS_OK;

	data = smb_buf(inbuf) + ((large_file_format ? 20 : 10)*num_ulocks);

	/* 
	 * Data now points at the beginning of the list
	 * of smb_lkrng structs.
	 */
	
	for(; blr->lock_num < num_locks; blr->lock_num++) {
		BOOL err;

		lock_pid = get_lock_pid( data, blr->lock_num, large_file_format);
		count = get_lock_count( data, blr->lock_num, large_file_format);
		offset = get_lock_offset( data, blr->lock_num, large_file_format, &err);
		
		/*
		 * We know err cannot be set as if it was the lock
		 * request would never have been queued. JRA.
		 */
		errno = 0;
		status = do_lock_spin(fsp,
				lock_pid,
				count,
				offset, 
				((locktype & 1) ? READ_LOCK : WRITE_LOCK),
				WINDOWS_LOCK,
				&my_lock_ctx);

		if (NT_STATUS_IS_ERR(status)) {
			break;
		}
	}

	if(blr->lock_num == num_locks) {
		/*
		 * Success - we got all the locks.
		 */
		
		DEBUG(3,("process_lockingX file = %s, fnum=%d type=%d num_locks=%d\n",
			 fsp->fsp_name, fsp->fnum, (unsigned int)locktype, num_locks) );

		reply_lockingX_success(blr);
		return True;
	} else if (!NT_STATUS_EQUAL(status,NT_STATUS_LOCK_NOT_GRANTED) &&
			!NT_STATUS_EQUAL(status,NT_STATUS_FILE_LOCK_CONFLICT)) {
			/*
			 * We have other than a "can't get lock"
			 * error. Free any locks we had and return an error.
			 * Return True so we get dequeued.
			 */
		
		blocking_lock_reply_error(blr, status);
		return True;
	}

	/*
	 * Still can't get all the locks - keep waiting.
	 */
	
	DEBUG(10,("process_lockingX: only got %d locks of %d needed for file %s, fnum = %d. \
Waiting....\n", 
		  blr->lock_num, num_locks, fsp->fsp_name, fsp->fnum));
	
	return False;
}

/****************************************************************************
 Attempt to get the posix lock request from a SMBtrans2 call.
 Returns True if we want to be removed from the list.
*****************************************************************************/

static BOOL process_trans2(blocking_lock_record *blr)
{
	extern int max_send;
	char *inbuf = blr->inbuf;
	char *outbuf;
	BOOL my_lock_ctx = False;
	char params[2];
	NTSTATUS status;

	status = do_lock(blr->fsp,
			blr->lock_pid,
			blr->count,
			blr->offset,
			blr->lock_type,
			blr->lock_flav,
			&my_lock_ctx);

	if (!NT_STATUS_IS_OK(status)) {
		if (ERROR_WAS_LOCK_DENIED(status)) {
			/* Still can't get the lock, just keep waiting. */
			return False;
		}	
		/*
		 * We have other than a "can't get lock"
		 * error. Send an error and return True so we get dequeued.
		 */
		blocking_lock_reply_error(blr, status);
		return True;
	}

	/* We finally got the lock, return success. */
	outbuf = get_OutBuffer();
	construct_reply_common(inbuf, outbuf);
	SCVAL(outbuf,smb_com,SMBtrans2);
	SSVAL(params,0,0);
	send_trans2_replies(outbuf, max_send, params, 2, NULL, 0);
	return True;
}


/****************************************************************************
 Process a blocking lock SMB.
 Returns True if we want to be removed from the list.
*****************************************************************************/

static BOOL blocking_lock_record_process(blocking_lock_record *blr)
{
	switch(blr->com_type) {
#if 0
		/* We no longer push blocking lock requests for anything but lockingX and trans2. */
		case SMBlock:
			return process_lock(blr);
		case SMBlockread:
			return process_lockread(blr);
#endif
		case SMBlockingX:
			return process_lockingX(blr);
		case SMBtrans2:
		case SMBtranss2:
			return process_trans2(blr);
		default:
			DEBUG(0,("blocking_lock_record_process: PANIC - unknown type on blocking lock queue - exiting.!\n"));
			exit_server("PANIC - unknown type on blocking lock queue");
	}
	return False; /* Keep compiler happy. */
}

/****************************************************************************
 Delete entries by fnum from the blocking lock pending queue.
*****************************************************************************/

void remove_pending_lock_requests_by_fid(files_struct *fsp)
{
	blocking_lock_record *blr, *next = NULL;

	for(blr = blocking_lock_queue; blr; blr = next) {
		next = blr->next;
		if(blr->fsp->fnum == fsp->fnum) {
			struct byte_range_lock *br_lck = brl_get_locks(fsp);

			if (br_lck) {
				DEBUG(10,("remove_pending_lock_requests_by_fid - removing request type %d for \
file %s fnum = %d\n", blr->com_type, fsp->fsp_name, fsp->fnum ));

				brl_remove_pending_lock(br_lck,
					blr->lock_pid,
					procid_self(),
					blr->offset,
					blr->count,
					blr->lock_flav);
				byte_range_lock_destructor(br_lck);

			}

			free_blocking_lock_record(blr);
		}
	}
}

/****************************************************************************
 Delete entries by mid from the blocking lock pending queue. Always send reply.
*****************************************************************************/

void remove_pending_lock_requests_by_mid(int mid)
{
	blocking_lock_record *blr, *next = NULL;

	for(blr = blocking_lock_queue; blr; blr = next) {
		next = blr->next;
		if(SVAL(blr->inbuf,smb_mid) == mid) {
			files_struct *fsp = blr->fsp;
			struct byte_range_lock *br_lck = brl_get_locks(fsp);

			if (br_lck) {
				DEBUG(10,("remove_pending_lock_requests_by_mid - removing request type %d for \
file %s fnum = %d\n", blr->com_type, fsp->fsp_name, fsp->fnum ));

				brl_remove_pending_lock(br_lck,
					blr->lock_pid,
					procid_self(),
					blr->offset,
					blr->count,
					blr->lock_flav);
				byte_range_lock_destructor(br_lck);
			}

			blocking_lock_reply_error(blr,NT_STATUS_FILE_LOCK_CONFLICT);
			free_blocking_lock_record(blr);
		}
	}
}

/****************************************************************************
  Set a flag as an unlock request affects one of our pending locks.
*****************************************************************************/

static void received_unlock_msg(int msg_type, struct process_id src,
				void *buf, size_t len)
{
	DEBUG(10,("received_unlock_msg\n"));
	process_blocking_lock_queue(time(NULL));
}

/****************************************************************************
 Return the number of seconds to the next blocking locks timeout, or default_timeout
*****************************************************************************/

unsigned blocking_locks_timeout(unsigned default_timeout)
{
	unsigned timeout = default_timeout;
	time_t t;
	blocking_lock_record *blr = blocking_lock_queue;

	/* note that we avoid the time() syscall if there are no blocking locks */
	if (!blr)
		return timeout;

	t = time(NULL);

	for (; blr; blr = blr->next) {
		if ((blr->expire_time != (time_t)-1) &&
					(timeout > (blr->expire_time - t))) {
			timeout = blr->expire_time - t;
		}
	}

	if (timeout < 1)
		timeout = 1;

	return timeout;
}

/****************************************************************************
 Process the blocking lock queue. Note that this is only called as root.
*****************************************************************************/

void process_blocking_lock_queue(time_t t)
{
	blocking_lock_record *blr, *next = NULL;

	/*
	 * Go through the queue and see if we can get any of the locks.
	 */

	for (blr = blocking_lock_queue; blr; blr = next) {
		connection_struct *conn = NULL;
		uint16 vuid;
		files_struct *fsp = NULL;

		next = blr->next;

		/*
		 * Ensure we don't have any old chain_fsp values
		 * sitting around....
		 */
		chain_size = 0;
		file_chain_reset();
		fsp = blr->fsp;

		conn = conn_find(SVAL(blr->inbuf,smb_tid));
		vuid = (lp_security() == SEC_SHARE) ? UID_FIELD_INVALID :
				SVAL(blr->inbuf,smb_uid);

		DEBUG(5,("process_blocking_lock_queue: examining pending lock fnum = %d for file %s\n",
			fsp->fnum, fsp->fsp_name ));

		if((blr->expire_time != -1) && (blr->expire_time <= t)) {
			struct byte_range_lock *br_lck = brl_get_locks(fsp);

			/*
			 * Lock expired - throw away all previously
			 * obtained locks and return lock error.
			 */

			if (br_lck) {
				DEBUG(5,("process_blocking_lock_queue: pending lock fnum = %d for file %s timed out.\n",
					fsp->fnum, fsp->fsp_name ));

				brl_remove_pending_lock(br_lck,
					blr->lock_pid,
					procid_self(),
					blr->offset,
					blr->count,
					blr->lock_flav);
				byte_range_lock_destructor(br_lck);
			}

			blocking_lock_reply_error(blr,NT_STATUS_FILE_LOCK_CONFLICT);
			free_blocking_lock_record(blr);
			continue;
		}

		if(!change_to_user(conn,vuid)) {
			struct byte_range_lock *br_lck = brl_get_locks(fsp);

			/*
			 * Remove the entry and return an error to the client.
			 */

			if (br_lck) {
				brl_remove_pending_lock(br_lck,
					blr->lock_pid,
					procid_self(),
					blr->offset,
					blr->count,
					blr->lock_flav);
				byte_range_lock_destructor(br_lck);
			}

			DEBUG(0,("process_blocking_lock_queue: Unable to become user vuid=%d.\n",
				vuid ));
			blocking_lock_reply_error(blr,NT_STATUS_ACCESS_DENIED);
			free_blocking_lock_record(blr);
			continue;
		}

		if(!set_current_service(conn,SVAL(blr->inbuf,smb_flg),True)) {
			struct byte_range_lock *br_lck = brl_get_locks(fsp);

			/*
			 * Remove the entry and return an error to the client.
			 */

			if (br_lck) {
				brl_remove_pending_lock(br_lck,
					blr->lock_pid,
					procid_self(),
					blr->offset,
					blr->count,
					blr->lock_flav);
				byte_range_lock_destructor(br_lck);
			}

			DEBUG(0,("process_blocking_lock_queue: Unable to become service Error was %s.\n", strerror(errno) ));
			blocking_lock_reply_error(blr,NT_STATUS_ACCESS_DENIED);
			free_blocking_lock_record(blr);
			change_to_root_user();
			continue;
		}

		/*
		 * Go through the remaining locks and try and obtain them.
		 * The call returns True if all locks were obtained successfully
		 * and False if we still need to wait.
		 */

		if(blocking_lock_record_process(blr)) {
			struct byte_range_lock *br_lck = brl_get_locks(fsp);

			if (br_lck) {
				brl_remove_pending_lock(br_lck,
					blr->lock_pid,
					procid_self(),
					blr->offset,
					blr->count,
					blr->lock_flav);
				byte_range_lock_destructor(br_lck);
			}

			free_blocking_lock_record(blr);
		}
		change_to_root_user();
	}
}

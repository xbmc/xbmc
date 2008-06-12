/*
   Unix SMB/Netbios implementation.
   Version 3.0
   async_io read handling using POSIX async io.
   Copyright (C) Jeremy Allison 2005.

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

#if defined(WITH_AIO)

/* The signal we'll use to signify aio done. */
#ifndef RT_SIGNAL_AIO
#define RT_SIGNAL_AIO (SIGRTMIN+3)
#endif

/****************************************************************************
 The buffer we keep around whilst an aio request is in process.
*****************************************************************************/

struct aio_extra {
	struct aio_extra *next, *prev;
	SMB_STRUCT_AIOCB acb;
	files_struct *fsp;
	BOOL read_req;
	uint16 mid;
	char *inbuf;
	char *outbuf;
};

static struct aio_extra *aio_list_head;

/****************************************************************************
 Create the extended aio struct we must keep around for the lifetime
 of the aio_read call.
*****************************************************************************/

static struct aio_extra *create_aio_ex_read(files_struct *fsp, size_t buflen, uint16 mid)
{
	struct aio_extra *aio_ex = SMB_MALLOC_P(struct aio_extra);

	if (!aio_ex) {
		return NULL;
	}
	ZERO_STRUCTP(aio_ex);
	/* The output buffer stored in the aio_ex is the start of
	   the smb return buffer. The buffer used in the acb
	   is the start of the reply data portion of that buffer. */
	aio_ex->outbuf = SMB_MALLOC_ARRAY(char, buflen);
	if (!aio_ex->outbuf) {
		SAFE_FREE(aio_ex);
		return NULL;
	}
	DLIST_ADD(aio_list_head, aio_ex);
	aio_ex->fsp = fsp;
	aio_ex->read_req = True;
	aio_ex->mid = mid;
	return aio_ex;
}

/****************************************************************************
 Create the extended aio struct we must keep around for the lifetime
 of the aio_write call.
*****************************************************************************/

static struct aio_extra *create_aio_ex_write(files_struct *fsp, size_t outbuflen, uint16 mid)
{
	struct aio_extra *aio_ex = SMB_MALLOC_P(struct aio_extra);

	if (!aio_ex) {
		return NULL;
	}
	ZERO_STRUCTP(aio_ex);

	/* We need space for an output reply of outbuflen bytes. */
	aio_ex->outbuf = SMB_MALLOC_ARRAY(char, outbuflen);
	if (!aio_ex->outbuf) {
		SAFE_FREE(aio_ex);
		return NULL;
	}
	/* Steal the input buffer containing the write data from the main SMB call. */
	/* We must re-allocate a new one here. */
	if (NewInBuffer(&aio_ex->inbuf) == NULL) {
		SAFE_FREE(aio_ex->outbuf);
		SAFE_FREE(aio_ex);
		return NULL;
	}

	/* aio_ex->inbuf now contains the stolen old InBuf containing the data to write. */

	DLIST_ADD(aio_list_head, aio_ex);
	aio_ex->fsp = fsp;
	aio_ex->read_req = False;
	aio_ex->mid = mid;
	return aio_ex;
}

/****************************************************************************
 Delete the extended aio struct.
*****************************************************************************/

static void delete_aio_ex(struct aio_extra *aio_ex)
{
	DLIST_REMOVE(aio_list_head, aio_ex);
	/* Safe to do as we've removed ourselves from the in use list first. */
	free_InBuffer(aio_ex->inbuf);

	SAFE_FREE(aio_ex->outbuf);
	SAFE_FREE(aio_ex);
}

/****************************************************************************
 Given the aiocb struct find the extended aio struct containing it.
*****************************************************************************/

static struct aio_extra *find_aio_ex(uint16 mid)
{
	struct aio_extra *p;

	for( p = aio_list_head; p; p = p->next) {
		if (mid == p->mid) {
			return p;
		}
	}
	return NULL;
}

/****************************************************************************
 We can have these many aio buffers in flight.
*****************************************************************************/

#define AIO_PENDING_SIZE 10
static sig_atomic_t signals_received;
static int outstanding_aio_calls;
static uint16 aio_pending_array[AIO_PENDING_SIZE];

/****************************************************************************
 Signal handler when an aio request completes.
*****************************************************************************/

static void signal_handler(int sig, siginfo_t *info, void *unused)
{
	if (signals_received < AIO_PENDING_SIZE - 1) {
		aio_pending_array[signals_received] = info->si_value.sival_int;
		signals_received++;
	} /* Else signal is lost. */
	sys_select_signal(RT_SIGNAL_AIO);
}

/****************************************************************************
 Is there a signal waiting ?
*****************************************************************************/

BOOL aio_finished(void)
{
	return (signals_received != 0);
}

/****************************************************************************
 Initialize the signal handler for aio read/write.
*****************************************************************************/

void initialize_async_io_handler(void)
{
	struct sigaction act;

	ZERO_STRUCT(act);
	act.sa_sigaction = signal_handler;
	act.sa_flags = SA_SIGINFO;
	sigemptyset( &act.sa_mask );
	if (sigaction(RT_SIGNAL_AIO, &act, NULL) != 0) {
                DEBUG(0,("Failed to setup RT_SIGNAL_AIO handler\n"));
        }

	/* the signal can start off blocked due to a bug in bash */
	BlockSignals(False, RT_SIGNAL_AIO);
}

/****************************************************************************
 Set up an aio request from a SMBreadX call.
*****************************************************************************/

BOOL schedule_aio_read_and_X(connection_struct *conn,
			     char *inbuf, char *outbuf,
			     int length, int len_outbuf,
			     files_struct *fsp, SMB_OFF_T startpos,
			     size_t smb_maxcnt)
{
	struct aio_extra *aio_ex;
	SMB_STRUCT_AIOCB *a;
	size_t bufsize;
	size_t min_aio_read_size = lp_aio_read_size(SNUM(conn));

	if (!min_aio_read_size || (smb_maxcnt < min_aio_read_size)) {
		/* Too small a read for aio request. */
		DEBUG(10,("schedule_aio_read_and_X: read size (%u) too small "
			  "for minimum aio_read of %u\n",
			  (unsigned int)smb_maxcnt,
			  (unsigned int)min_aio_read_size ));
		return False;
	}

	/* Only do this on non-chained and non-chaining reads not using the write cache. */
        if (chain_size !=0 || (CVAL(inbuf,smb_vwv0) != 0xFF) || (lp_write_cache_size(SNUM(conn)) != 0) ) {
		return False;
	}

	if (outstanding_aio_calls >= AIO_PENDING_SIZE) {
		DEBUG(10,("schedule_aio_read_and_X: Already have %d aio activities outstanding.\n",
			  outstanding_aio_calls ));
		return False;
	}

	/* The following is safe from integer wrap as we've already
	   checked smb_maxcnt is 128k or less. */
	bufsize = PTR_DIFF(smb_buf(outbuf),outbuf) + smb_maxcnt;

	if ((aio_ex = create_aio_ex_read(fsp, bufsize, SVAL(inbuf,smb_mid))) == NULL) {
		DEBUG(10,("schedule_aio_read_and_X: malloc fail.\n"));
		return False;
	}

	/* Copy the SMB header already setup in outbuf. */
	memcpy(aio_ex->outbuf, outbuf, smb_buf(outbuf) - outbuf);
	SCVAL(aio_ex->outbuf,smb_vwv0,0xFF); /* Never a chained reply. */

	a = &aio_ex->acb;

	/* Now set up the aio record for the read call. */
	
	a->aio_fildes = fsp->fh->fd;
	a->aio_buf = smb_buf(aio_ex->outbuf);
	a->aio_nbytes = smb_maxcnt;
	a->aio_offset = startpos;
	a->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	a->aio_sigevent.sigev_signo  = RT_SIGNAL_AIO;
	a->aio_sigevent.sigev_value.sival_int = aio_ex->mid;

	if (SMB_VFS_AIO_READ(fsp,a) == -1) {
		DEBUG(0,("schedule_aio_read_and_X: aio_read failed. Error %s\n",
			strerror(errno) ));
		delete_aio_ex(aio_ex);
		return False;
	}

	DEBUG(10,("schedule_aio_read_and_X: scheduled aio_read for file %s, offset %.0f, len = %u (mid = %u)\n",
		fsp->fsp_name, (double)startpos, (unsigned int)smb_maxcnt, (unsigned int)aio_ex->mid ));

	srv_defer_sign_response(aio_ex->mid);
	outstanding_aio_calls++;
	return True;
}

/****************************************************************************
 Set up an aio request from a SMBwriteX call.
*****************************************************************************/

BOOL schedule_aio_write_and_X(connection_struct *conn,
				char *inbuf, char *outbuf,
				int length, int len_outbuf,
				files_struct *fsp, char *data,
				SMB_OFF_T startpos,
				size_t numtowrite)
{
	struct aio_extra *aio_ex;
	SMB_STRUCT_AIOCB *a;
	size_t outbufsize;
	BOOL write_through = BITSETW(inbuf+smb_vwv7,0);
	size_t min_aio_write_size = lp_aio_write_size(SNUM(conn));

	if (!min_aio_write_size || (numtowrite < min_aio_write_size)) {
		/* Too small a write for aio request. */
		DEBUG(10,("schedule_aio_write_and_X: write size (%u) too small "
			  "for minimum aio_write of %u\n",
			  (unsigned int)numtowrite,
			  (unsigned int)min_aio_write_size ));
		return False;
	}

	/* Only do this on non-chained and non-chaining reads not using the write cache. */
        if (chain_size !=0 || (CVAL(inbuf,smb_vwv0) != 0xFF) || (lp_write_cache_size(SNUM(conn)) != 0) ) {
		return False;
	}

	if (outstanding_aio_calls >= AIO_PENDING_SIZE) {
		DEBUG(3,("schedule_aio_write_and_X: Already have %d aio activities outstanding.\n",
			  outstanding_aio_calls ));
		DEBUG(10,("schedule_aio_write_and_X: failed to schedule aio_write for file %s, offset %.0f, len = %u (mid = %u)\n",
			fsp->fsp_name, (double)startpos, (unsigned int)numtowrite, (unsigned int)SVAL(inbuf,smb_mid) ));
		return False;
	}

	outbufsize = smb_len(outbuf) + 4;
	if ((aio_ex = create_aio_ex_write(fsp, outbufsize, SVAL(inbuf,smb_mid))) == NULL) {
		DEBUG(0,("schedule_aio_write_and_X: malloc fail.\n"));
		return False;
	}

	/* Paranioa.... */
	SMB_ASSERT(aio_ex->inbuf == inbuf);

	/* Copy the SMB header already setup in outbuf. */
	memcpy(aio_ex->outbuf, outbuf, outbufsize);
	SCVAL(aio_ex->outbuf,smb_vwv0,0xFF); /* Never a chained reply. */

	a = &aio_ex->acb;

	/* Now set up the aio record for the write call. */
	
	a->aio_fildes = fsp->fh->fd;
	a->aio_buf = data; /* As we've stolen inbuf this points within inbuf. */
	a->aio_nbytes = numtowrite;
	a->aio_offset = startpos;
	a->aio_sigevent.sigev_notify = SIGEV_SIGNAL;
	a->aio_sigevent.sigev_signo  = RT_SIGNAL_AIO;
	a->aio_sigevent.sigev_value.sival_int = aio_ex->mid;

	if (SMB_VFS_AIO_WRITE(fsp,a) == -1) {
		DEBUG(3,("schedule_aio_wrote_and_X: aio_write failed. Error %s\n",
			strerror(errno) ));
		/* Replace global InBuf as we're going to do a normal write. */
		set_InBuffer(aio_ex->inbuf);
		aio_ex->inbuf = NULL;
		delete_aio_ex(aio_ex);
		return False;
	}

	if (!write_through && !lp_syncalways(SNUM(fsp->conn)) && fsp->aio_write_behind) {
		/* Lie to the client and immediately claim we finished the write. */
	        SSVAL(aio_ex->outbuf,smb_vwv2,numtowrite);
                SSVAL(aio_ex->outbuf,smb_vwv4,(numtowrite>>16)&1);
		show_msg(aio_ex->outbuf);
		if (!send_smb(smbd_server_fd(),aio_ex->outbuf)) {
			exit_server("handle_aio_write: send_smb failed.");
		}
		DEBUG(10,("schedule_aio_write_and_X: scheduled aio_write behind for file %s\n",
			fsp->fsp_name ));
	} else {
		srv_defer_sign_response(aio_ex->mid);
	}
	outstanding_aio_calls++;

	DEBUG(10,("schedule_aio_write_and_X: scheduled aio_write for file %s, \
offset %.0f, len = %u (mid = %u) outstanding_aio_calls = %d\n",
		fsp->fsp_name, (double)startpos, (unsigned int)numtowrite, (unsigned int)aio_ex->mid, outstanding_aio_calls ));

	return True;
}


/****************************************************************************
 Complete the read and return the data or error back to the client.
 Returns errno or zero if all ok.
*****************************************************************************/

static int handle_aio_read_complete(struct aio_extra *aio_ex)
{
	int ret = 0;
	int outsize;
	char *outbuf = aio_ex->outbuf;
	char *data = smb_buf(outbuf);
	ssize_t nread = SMB_VFS_AIO_RETURN(aio_ex->fsp,&aio_ex->acb);

	if (nread < 0) {
		/* We're relying here on the fact that if the fd is
		   closed then the aio will complete and aio_return
		   will return an error. Hopefully this is
		   true.... JRA. */

		/* If errno is ECANCELED then don't return anything to the client. */
		if (errno == ECANCELED) {
			srv_cancel_sign_response(aio_ex->mid);
			return 0;
		}

		DEBUG( 3,( "handle_aio_read_complete: file %s nread == -1. Error = %s\n",
			aio_ex->fsp->fsp_name, strerror(errno) ));

		outsize = (UNIXERROR(ERRDOS,ERRnoaccess));
		ret = errno;
	} else {
		outsize = set_message(outbuf,12,nread,False);
		SSVAL(outbuf,smb_vwv2,0xFFFF); /* Remaining - must be * -1. */
		SSVAL(outbuf,smb_vwv5,nread);
		SSVAL(outbuf,smb_vwv6,smb_offset(data,outbuf));
		SSVAL(outbuf,smb_vwv7,((nread >> 16) & 1));
		SSVAL(smb_buf(outbuf),-2,nread);

		DEBUG( 3, ( "handle_aio_read_complete file %s max=%d nread=%d\n",
			aio_ex->fsp->fsp_name,
			aio_ex->acb.aio_nbytes, (int)nread ) );

	}
	smb_setlen(outbuf,outsize - 4);
	show_msg(outbuf);
	if (!send_smb(smbd_server_fd(),outbuf)) {
		exit_server("handle_aio_read_complete: send_smb failed.");
	}

	DEBUG(10,("handle_aio_read_complete: scheduled aio_read completed for file %s, offset %.0f, len = %u\n",
		aio_ex->fsp->fsp_name, (double)aio_ex->acb.aio_offset, (unsigned int)nread ));

	return ret;
}

/****************************************************************************
 Complete the write and return the data or error back to the client.
 Returns errno or zero if all ok.
*****************************************************************************/

static int handle_aio_write_complete(struct aio_extra *aio_ex)
{
	int ret = 0;
	files_struct *fsp = aio_ex->fsp;
	char *outbuf = aio_ex->outbuf;
	ssize_t numtowrite = aio_ex->acb.aio_nbytes;
	ssize_t nwritten = SMB_VFS_AIO_RETURN(fsp,&aio_ex->acb);

	if (fsp->aio_write_behind) {
		if (nwritten != numtowrite) {
			if (nwritten == -1) {
				DEBUG(5,("handle_aio_write_complete: aio_write_behind failed ! File %s is corrupt ! Error %s\n",
					fsp->fsp_name, strerror(errno) ));
				ret = errno;
			} else {
				DEBUG(0,("handle_aio_write_complete: aio_write_behind failed ! File %s is corrupt ! \
Wanted %u bytes but only wrote %d\n", fsp->fsp_name, (unsigned int)numtowrite, (int)nwritten ));
				ret = EIO;
			}
		} else {
			DEBUG(10,("handle_aio_write_complete: aio_write_behind completed for file %s\n",
				fsp->fsp_name ));
		}
		return 0;
	}

	/* We don't need outsize or set_message here as we've already set the
	   fixed size length when we set up the aio call. */

	if(nwritten == -1) {
		DEBUG( 3,( "handle_aio_write: file %s wanted %u bytes. nwritten == %d. Error = %s\n",
			fsp->fsp_name, (unsigned int)numtowrite,
			(int)nwritten, strerror(errno) ));

		/* If errno is ECANCELED then don't return anything to the client. */
		if (errno == ECANCELED) {
			srv_cancel_sign_response(aio_ex->mid);
			return 0;
		}

		UNIXERROR(ERRHRD,ERRdiskfull);
		ret = errno;
        } else {
		BOOL write_through = BITSETW(aio_ex->inbuf+smb_vwv7,0);

        	SSVAL(outbuf,smb_vwv2,nwritten);
		SSVAL(outbuf,smb_vwv4,(nwritten>>16)&1);
		if (nwritten < (ssize_t)numtowrite) {
			SCVAL(outbuf,smb_rcls,ERRHRD);
			SSVAL(outbuf,smb_err,ERRdiskfull);
		}
                                                                                                                                  
		DEBUG(3,("handle_aio_write: fnum=%d num=%d wrote=%d\n", fsp->fnum, (int)numtowrite, (int)nwritten));
		sync_file(fsp->conn,fsp, write_through);
	}

	show_msg(outbuf);
	if (!send_smb(smbd_server_fd(),outbuf)) {
		exit_server("handle_aio_write: send_smb failed.");
	}

	DEBUG(10,("handle_aio_write_complete: scheduled aio_write completed for file %s, offset %.0f, requested %u, written = %u\n",
		fsp->fsp_name, (double)aio_ex->acb.aio_offset, (unsigned int)numtowrite, (unsigned int)nwritten ));

	return ret;
}

/****************************************************************************
 Handle any aio completion. Returns True if finished (and sets *perr if err was non-zero),
 False if not.
*****************************************************************************/

static BOOL handle_aio_completed(struct aio_extra *aio_ex, int *perr)
{
	int err;

	/* Ensure the operation has really completed. */
	if (SMB_VFS_AIO_ERROR(aio_ex->fsp, &aio_ex->acb) == EINPROGRESS) {
		DEBUG(10,( "handle_aio_completed: operation mid %u still in process for file %s\n",
			aio_ex->mid, aio_ex->fsp->fsp_name ));
		return False;
	}

	if (aio_ex->read_req) {
		err = handle_aio_read_complete(aio_ex);
	} else {
		err = handle_aio_write_complete(aio_ex);
	}

	if (err) {
		*perr = err; /* Only save non-zero errors. */
	}

	return True;
}

/****************************************************************************
 Handle any aio completion inline.
 Returns non-zero errno if fail or zero if all ok.
*****************************************************************************/

int process_aio_queue(void)
{
	int i;
	int ret = 0;

	BlockSignals(True, RT_SIGNAL_AIO);

	DEBUG(10,("process_aio_queue: signals_received = %d\n", (int)signals_received));
	DEBUG(10,("process_aio_queue: outstanding_aio_calls = %d\n", outstanding_aio_calls));

	if (!signals_received) {
		BlockSignals(False, RT_SIGNAL_AIO);
		return 0;
	}

	/* Drain all the complete aio_reads. */
	for (i = 0; i < signals_received; i++) {
		uint16 mid = aio_pending_array[i];
		files_struct *fsp = NULL;
		struct aio_extra *aio_ex = find_aio_ex(mid);

		if (!aio_ex) {
			DEBUG(3,("process_aio_queue: Can't find record to match mid %u.\n",
				(unsigned int)mid));
			srv_cancel_sign_response(mid);
			continue;
		}

		fsp = aio_ex->fsp;
		if (fsp == NULL) {
			/* file was closed whilst I/O was outstanding. Just ignore. */
			DEBUG( 3,( "process_aio_queue: file closed whilst aio outstanding.\n"));
			srv_cancel_sign_response(mid);
			continue;
		}

		if (!handle_aio_completed(aio_ex, &ret)) {
			continue;
		}

		delete_aio_ex(aio_ex);
	}

	outstanding_aio_calls -= signals_received;
	signals_received = 0;
	BlockSignals(False, RT_SIGNAL_AIO);
	return ret;
}

/****************************************************************************
 We're doing write behind and the client closed the file. Wait up to 30 seconds
 (my arbitrary choice) for the aio to complete. Return 0 if all writes completed,
 errno to return if not.
*****************************************************************************/

#define SMB_TIME_FOR_AIO_COMPLETE_WAIT 29

BOOL wait_for_aio_completion(files_struct *fsp)
{
	struct aio_extra *aio_ex;
	const SMB_STRUCT_AIOCB **aiocb_list;
	int aio_completion_count = 0;
	time_t start_time = time(NULL);
	int seconds_left;
	int ret = 0;

	for (seconds_left = SMB_TIME_FOR_AIO_COMPLETE_WAIT; seconds_left >= 0;) {
		int err = 0;
		int i;
		struct timespec ts;

		aio_completion_count = 0;
		for( aio_ex = aio_list_head; aio_ex; aio_ex = aio_ex->next) {
			if (aio_ex->fsp == fsp) {
				aio_completion_count++;
			}
		}

		if (!aio_completion_count) {
			return ret;
		}

		DEBUG(3,("wait_for_aio_completion: waiting for %d aio events to complete.\n",
			aio_completion_count ));

		aiocb_list = SMB_MALLOC_ARRAY(const SMB_STRUCT_AIOCB *, aio_completion_count);
		if (!aiocb_list) {
			return False;
		}

		for( i = 0, aio_ex = aio_list_head; aio_ex; aio_ex = aio_ex->next) {
			if (aio_ex->fsp == fsp) {
				aiocb_list[i++] = &aio_ex->acb;
			}
		}

		/* Now wait up to seconds_left for completion. */
		ts.tv_sec = seconds_left;
		ts.tv_nsec = 0;

		DEBUG(10,("wait_for_aio_completion: %d events, doing a wait of %d seconds.\n",
			aio_completion_count, seconds_left ));

		err = SMB_VFS_AIO_SUSPEND(fsp, aiocb_list, aio_completion_count, &ts);

		DEBUG(10,("wait_for_aio_completion: returned err = %d, errno = %s\n",
			err, strerror(errno) ));
		
		if (err == -1 && errno == EAGAIN) {
			DEBUG(0,("wait_for_aio_completion: aio_suspend timed out waiting for %d events after a wait of %d seconds\n",
					aio_completion_count, seconds_left));
			/* Timeout. */
			cancel_aio_by_fsp(fsp);
			SAFE_FREE(aiocb_list);
			return ret ? ret : EIO;
		}

		/* One or more events might have completed - process them if so. */
		for( i = 0; i < aio_completion_count; i++) {
			uint16 mid = aiocb_list[i]->aio_sigevent.sigev_value.sival_int;

			aio_ex = find_aio_ex(mid);

			if (!aio_ex) {
				DEBUG(0, ("wait_for_aio_completion: mid %u doesn't match an aio record\n",
					(unsigned int)mid ));
				continue;
			}

			if (!handle_aio_completed(aio_ex, &err)) {
				continue;
			}
			delete_aio_ex(aio_ex);
		}

		SAFE_FREE(aiocb_list);
		seconds_left = SMB_TIME_FOR_AIO_COMPLETE_WAIT - (time(NULL) - start_time);
	}

	/* We timed out - we don't know why. Return ret if already an error, else EIO. */
	DEBUG(10,("wait_for_aio_completion: aio_suspend timed out waiting for %d events\n",
			aio_completion_count));

	return ret ? ret : EIO;
}

/****************************************************************************
 Cancel any outstanding aio requests. The client doesn't care about the reply.
*****************************************************************************/

void cancel_aio_by_fsp(files_struct *fsp)
{
	struct aio_extra *aio_ex;

	for( aio_ex = aio_list_head; aio_ex; aio_ex = aio_ex->next) {
		if (aio_ex->fsp == fsp) {
			/* Don't delete the aio_extra record as we may have completed
			   and don't yet know it. Just do the aio_cancel call and return. */
			SMB_VFS_AIO_CANCEL(fsp,fsp->fh->fd, &aio_ex->acb);
			aio_ex->fsp = NULL; /* fsp will be closed when we return. */
		}
	}
}

/****************************************************************************
 Check if a buffer was stolen for aio use.
*****************************************************************************/

BOOL aio_inbuffer_in_use(char *inbuf)
{
	struct aio_extra *aio_ex;

	for( aio_ex = aio_list_head; aio_ex; aio_ex = aio_ex->next) {
		if (aio_ex->inbuf == inbuf) {
			return True;
		}
	}
	return False;
}
#else
BOOL aio_finished(void)
{
	return False;
}

void initialize_async_io_handler(void)
{
}

int process_aio_queue(void)
{
	return False;
}

BOOL schedule_aio_read_and_X(connection_struct *conn,
			     char *inbuf, char *outbuf,
			     int length, int len_outbuf,
			     files_struct *fsp, SMB_OFF_T startpos,
			     size_t smb_maxcnt)
{
	return False;
}

BOOL schedule_aio_write_and_X(connection_struct *conn,
                                char *inbuf, char *outbuf,
                                int length, int len_outbuf,
                                files_struct *fsp, char *data,
                                SMB_OFF_T startpos,
                                size_t numtowrite)
{
	return False;
}

void cancel_aio_by_fsp(files_struct *fsp)
{
}

BOOL wait_for_aio_completion(files_struct *fsp)
{
	return True;
}

BOOL aio_inbuffer_in_use(char *ptr)
{
	return False;
}
#endif

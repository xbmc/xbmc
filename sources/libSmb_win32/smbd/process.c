/* 
   Unix SMB/CIFS implementation.
   process incoming packets - main loop
   Copyright (C) Andrew Tridgell 1992-1998
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

extern uint16 global_smbpid;
extern int keepalive;
extern struct auth_context *negprot_global_auth_context;
extern int smb_echo_count;

static char *InBuffer = NULL;
static char *OutBuffer = NULL;
static char *current_inbuf = NULL;

/* 
 * Size of data we can send to client. Set
 *  by the client for all protocols above CORE.
 *  Set by us for CORE protocol.
 */
int max_send = BUFFER_SIZE;
/*
 * Size of the data we can receive. Set by us.
 * Can be modified by the max xmit parameter.
 */
int max_recv = BUFFER_SIZE;

extern int last_message;
extern int smb_read_error;
SIG_ATOMIC_T reload_after_sighup = 0;
SIG_ATOMIC_T got_sig_term = 0;
extern BOOL global_machine_password_needs_changing;
extern int max_send;

/****************************************************************************
 Function to return the current request mid from Inbuffer.
****************************************************************************/

uint16 get_current_mid(void)
{
	return SVAL(InBuffer,smb_mid);
}

/****************************************************************************
 structure to hold a linked list of queued messages.
 for processing.
****************************************************************************/

static struct pending_message_list *deferred_open_queue;

/****************************************************************************
 Function to push a message onto the tail of a linked list of smb messages ready
 for processing.
****************************************************************************/

static BOOL push_queued_message(char *buf, int msg_len,
				struct timeval request_time,
				struct timeval end_time,
				char *private_data, size_t private_len)
{
	struct pending_message_list *tmp_msg;
	struct pending_message_list *msg;

	msg = TALLOC_ZERO_P(NULL, struct pending_message_list);

	if(msg == NULL) {
		DEBUG(0,("push_message: malloc fail (1)\n"));
		return False;
	}

	msg->buf = data_blob_talloc(msg, buf, msg_len);
	if(msg->buf.data == NULL) {
		DEBUG(0,("push_message: malloc fail (2)\n"));
		TALLOC_FREE(msg);
		return False;
	}

	msg->request_time = request_time;
	msg->end_time = end_time;

	if (private_data) {
		msg->private_data = data_blob_talloc(msg, private_data,
						     private_len);
		if (msg->private_data.data == NULL) {
			DEBUG(0,("push_message: malloc fail (3)\n"));
			TALLOC_FREE(msg);
			return False;
		}
	}

	DLIST_ADD_END(deferred_open_queue, msg, tmp_msg);

	DEBUG(10,("push_message: pushed message length %u on "
		  "deferred_open_queue\n", (unsigned int)msg_len));

	return True;
}

/****************************************************************************
 Function to delete a sharing violation open message by mid.
****************************************************************************/

void remove_deferred_open_smb_message(uint16 mid)
{
	struct pending_message_list *pml;

	for (pml = deferred_open_queue; pml; pml = pml->next) {
		if (mid == SVAL(pml->buf.data,smb_mid)) {
			DEBUG(10,("remove_sharing_violation_open_smb_message: "
				  "deleting mid %u len %u\n",
				  (unsigned int)mid,
				  (unsigned int)pml->buf.length ));
			DLIST_REMOVE(deferred_open_queue, pml);
			TALLOC_FREE(pml);
			return;
		}
	}
}

/****************************************************************************
 Move a sharing violation open retry message to the front of the list and
 schedule it for immediate processing.
****************************************************************************/

void schedule_deferred_open_smb_message(uint16 mid)
{
	struct pending_message_list *pml;
	int i = 0;

	for (pml = deferred_open_queue; pml; pml = pml->next) {
		uint16 msg_mid = SVAL(pml->buf.data,smb_mid);
		DEBUG(10,("schedule_deferred_open_smb_message: [%d] msg_mid = %u\n", i++,
			(unsigned int)msg_mid ));
		if (mid == msg_mid) {
			DEBUG(10,("schedule_deferred_open_smb_message: scheduling mid %u\n",
				mid ));
			pml->end_time.tv_sec = 0;
			pml->end_time.tv_usec = 0;
			DLIST_PROMOTE(deferred_open_queue, pml);
			return;
		}
	}

	DEBUG(10,("schedule_deferred_open_smb_message: failed to find message mid %u\n",
		mid ));
}

/****************************************************************************
 Return true if this mid is on the deferred queue.
****************************************************************************/

BOOL open_was_deferred(uint16 mid)
{
	struct pending_message_list *pml;

	for (pml = deferred_open_queue; pml; pml = pml->next) {
		if (SVAL(pml->buf.data,smb_mid) == mid) {
			set_saved_ntstatus(NT_STATUS_OK);
			return True;
		}
	}
	return False;
}

/****************************************************************************
 Return the message queued by this mid.
****************************************************************************/

struct pending_message_list *get_open_deferred_message(uint16 mid)
{
	struct pending_message_list *pml;

	for (pml = deferred_open_queue; pml; pml = pml->next) {
		if (SVAL(pml->buf.data,smb_mid) == mid) {
			return pml;
		}
	}
	return NULL;
}

/****************************************************************************
 Function to push a deferred open smb message onto a linked list of local smb
 messages ready for processing.
****************************************************************************/

BOOL push_deferred_smb_message(uint16 mid,
			       struct timeval request_time,
			       struct timeval timeout,
			       char *private_data, size_t priv_len)
{
	struct timeval end_time;

	end_time = timeval_sum(&request_time, &timeout);

	DEBUG(10,("push_deferred_open_smb_message: pushing message len %u mid %u "
		  "timeout time [%u.%06u]\n",
		  (unsigned int) smb_len(current_inbuf)+4, (unsigned int)mid,
		  (unsigned int)end_time.tv_sec,
		  (unsigned int)end_time.tv_usec));

	return push_queued_message(current_inbuf, smb_len(current_inbuf)+4,
				   request_time, end_time,
				   private_data, priv_len);
}

struct idle_event {
	struct timed_event *te;
	struct timeval interval;
	BOOL (*handler)(const struct timeval *now, void *private_data);
	void *private_data;
};

static void idle_event_handler(struct timed_event *te,
			       const struct timeval *now,
			       void *private_data)
{
	struct idle_event *event =
		talloc_get_type_abort(private_data, struct idle_event);

	TALLOC_FREE(event->te);

	if (!event->handler(now, event->private_data)) {
		/* Don't repeat, delete ourselves */
		TALLOC_FREE(event);
		return;
	}

	event->te = add_timed_event(event, timeval_sum(now, &event->interval),
				    "idle_event_handler",
				    idle_event_handler, event);

	/* We can't do much but fail here. */
	SMB_ASSERT(event->te != NULL);
}

struct idle_event *add_idle_event(TALLOC_CTX *mem_ctx,
				  struct timeval interval,
				  BOOL (*handler)(const struct timeval *now,
						  void *private_data),
				  void *private_data)
{
	struct idle_event *result;
	struct timeval now = timeval_current();

	result = TALLOC_P(mem_ctx, struct idle_event);
	if (result == NULL) {
		DEBUG(0, ("talloc failed\n"));
		return NULL;
	}

	result->interval = interval;
	result->handler = handler;
	result->private_data = private_data;

	result->te = add_timed_event(result, timeval_sum(&now, &interval),
				     "idle_event_handler",
				     idle_event_handler, result);
	if (result->te == NULL) {
		DEBUG(0, ("add_timed_event failed\n"));
		TALLOC_FREE(result);
		return NULL;
	}

	return result;
}

/****************************************************************************
 Do all async processing in here. This includes kernel oplock messages, change
 notify events etc.
****************************************************************************/

static void async_processing(fd_set *pfds)
{
	DEBUG(10,("async_processing: Doing async processing.\n"));

	process_aio_queue();

	process_kernel_oplocks(pfds);

	/* Do the aio check again after receive_local_message as it does a
	   select and may have eaten our signal. */
	/* Is this till true? -- vl */
	process_aio_queue();

	if (got_sig_term) {
		exit_server_cleanly("termination signal");
	}

	/* check for async change notify events */
	process_pending_change_notify_queue(0);

	/* check for sighup processing */
	if (reload_after_sighup) {
		change_to_root_user();
		DEBUG(1,("Reloading services after SIGHUP\n"));
		reload_services(False);
		reload_after_sighup = 0;
	}
}

/****************************************************************************
 Add a fd to the set we will be select(2)ing on.
****************************************************************************/

static int select_on_fd(int fd, int maxfd, fd_set *fds)
{
	if (fd != -1) {
		FD_SET(fd, fds);
		maxfd = MAX(maxfd, fd);
	}

	return maxfd;
}

/****************************************************************************
  Do a select on an two fd's - with timeout. 

  If a local udp message has been pushed onto the
  queue (this can only happen during oplock break
  processing) call async_processing()

  If a pending smb message has been pushed onto the
  queue (this can only happen during oplock break
  processing) return this next.

  If the first smbfd is ready then read an smb from it.
  if the second (loopback UDP) fd is ready then read a message
  from it and setup the buffer header to identify the length
  and from address.
  Returns False on timeout or error.
  Else returns True.

The timeout is in milliseconds
****************************************************************************/

static BOOL receive_message_or_smb(char *buffer, int buffer_len, int timeout)
{
	fd_set fds;
	int selrtn;
	struct timeval to;
	int maxfd = 0;

	smb_read_error = 0;

 again:

	if (timeout >= 0) {
		to.tv_sec = timeout / 1000;
		to.tv_usec = (timeout % 1000) * 1000;
	} else {
		to.tv_sec = SMBD_SELECT_TIMEOUT;
		to.tv_usec = 0;
	}

	/*
	 * Note that this call must be before processing any SMB
	 * messages as we need to synchronously process any messages
	 * we may have sent to ourselves from the previous SMB.
	 */
	message_dispatch();

	/*
	 * Check to see if we already have a message on the deferred open queue
	 * and it's time to schedule.
	 */
  	if(deferred_open_queue != NULL) {
		BOOL pop_message = False;
		struct pending_message_list *msg = deferred_open_queue;

		if (timeval_is_zero(&msg->end_time)) {
			pop_message = True;
		} else {
			struct timeval tv;
			SMB_BIG_INT tdif;

			GetTimeOfDay(&tv);
			tdif = usec_time_diff(&msg->end_time, &tv);
			if (tdif <= 0) {
				/* Timed out. Schedule...*/
				pop_message = True;
				DEBUG(10,("receive_message_or_smb: queued message timed out.\n"));
			} else {
				/* Make a more accurate select timeout. */
				to.tv_sec = tdif / 1000000;
				to.tv_usec = tdif % 1000000;
				DEBUG(10,("receive_message_or_smb: select with timeout of [%u.%06u]\n",
					(unsigned int)to.tv_sec, (unsigned int)to.tv_usec ));
			}
		}

		if (pop_message) {
			memcpy(buffer, msg->buf.data, MIN(buffer_len, msg->buf.length));
  
			/* We leave this message on the queue so the open code can
			   know this is a retry. */
			DEBUG(5,("receive_message_or_smb: returning deferred open smb message.\n"));
			return True;
		}
	}

	/*
	 * Setup the select read fd set.
	 */

	FD_ZERO(&fds);

	/*
	 * Ensure we process oplock break messages by preference.
	 * We have to do this before the select, after the select
	 * and if the select returns EINTR. This is due to the fact
	 * that the selects called from async_processing can eat an EINTR
	 * caused by a signal (we can't take the break message there).
	 * This is hideously complex - *MUST* be simplified for 3.0 ! JRA.
	 */

	if (oplock_message_waiting(&fds)) {
		DEBUG(10,("receive_message_or_smb: oplock_message is waiting.\n"));
		async_processing(&fds);
		/*
		 * After async processing we must go and do the select again, as
		 * the state of the flag in fds for the server file descriptor is
		 * indeterminate - we may have done I/O on it in the oplock processing. JRA.
		 */
		goto again;
	}

	/*
	 * Are there any timed events waiting ? If so, ensure we don't
	 * select for longer than it would take to wait for them.
	 */

	{
		struct timeval tmp;
		struct timeval *tp = get_timed_events_timeout(&tmp);

		if (tp) {
			to = timeval_min(&to, tp);
			if (timeval_is_zero(&to)) {
				/* Process a timed event now... */
				run_events();
			}
		}
	}
	
	maxfd = select_on_fd(smbd_server_fd(), maxfd, &fds);
	maxfd = select_on_fd(change_notify_fd(), maxfd, &fds);
	maxfd = select_on_fd(oplock_notify_fd(), maxfd, &fds);

	selrtn = sys_select(maxfd+1,&fds,NULL,NULL,&to);

	/* if we get EINTR then maybe we have received an oplock
	   signal - treat this as select returning 1. This is ugly, but
	   is the best we can do until the oplock code knows more about
	   signals */
	if (selrtn == -1 && errno == EINTR) {
		async_processing(&fds);
		/*
		 * After async processing we must go and do the select again, as
		 * the state of the flag in fds for the server file descriptor is
		 * indeterminate - we may have done I/O on it in the oplock processing. JRA.
		 */
		goto again;
	}

	/* Check if error */
	if (selrtn == -1) {
		/* something is wrong. Maybe the socket is dead? */
		smb_read_error = READ_ERROR;
		return False;
	} 
    
	/* Did we timeout ? */
	if (selrtn == 0) {
		smb_read_error = READ_TIMEOUT;
		return False;
	}

	/*
	 * Ensure we process oplock break messages by preference.
	 * This is IMPORTANT ! Otherwise we can starve other processes
	 * sending us an oplock break message. JRA.
	 */

	if (oplock_message_waiting(&fds)) {
		async_processing(&fds);
		/*
		 * After async processing we must go and do the select again, as
		 * the state of the flag in fds for the server file descriptor is
		 * indeterminate - we may have done I/O on it in the oplock processing. JRA.
		 */
		goto again;
	}
	
	return receive_smb(smbd_server_fd(), buffer, 0);
}

/*
 * Only allow 5 outstanding trans requests. We're allocating memory, so
 * prevent a DoS.
 */

NTSTATUS allow_new_trans(struct trans_state *list, int mid)
{
	int count = 0;
	for (; list != NULL; list = list->next) {

		if (list->mid == mid) {
			return NT_STATUS_INVALID_PARAMETER;
		}

		count += 1;
	}
	if (count > 5) {
		return NT_STATUS_INSUFFICIENT_RESOURCES;
	}

	return NT_STATUS_OK;
}

/****************************************************************************
 We're terminating and have closed all our files/connections etc.
 If there are any pending local messages we need to respond to them
 before termination so that other smbds don't think we just died whilst
 holding oplocks.
****************************************************************************/

void respond_to_all_remaining_local_messages(void)
{
	/*
	 * Assert we have no exclusive open oplocks.
	 */

	if(get_number_of_exclusive_open_oplocks()) {
		DEBUG(0,("respond_to_all_remaining_local_messages: PANIC : we have %d exclusive oplocks.\n",
			get_number_of_exclusive_open_oplocks() ));
		return;
	}

	process_kernel_oplocks(NULL);

	return;
}


/*
These flags determine some of the permissions required to do an operation 

Note that I don't set NEED_WRITE on some write operations because they
are used by some brain-dead clients when printing, and I don't want to
force write permissions on print services.
*/
#define AS_USER (1<<0)
#define NEED_WRITE (1<<1) /* Must be paired with AS_USER */
#define TIME_INIT (1<<2)
#define CAN_IPC (1<<3) /* Must be paired with AS_USER */
#define AS_GUEST (1<<5) /* Must *NOT* be paired with AS_USER */
#define DO_CHDIR (1<<6)

/* 
   define a list of possible SMB messages and their corresponding
   functions. Any message that has a NULL function is unimplemented -
   please feel free to contribute implementations!
*/
static const struct smb_message_struct {
	const char *name;
	int (*fn)(connection_struct *conn, char *, char *, int, int);
	int flags;
} smb_messages[256] = {

/* 0x00 */ { "SMBmkdir",reply_mkdir,AS_USER | NEED_WRITE},
/* 0x01 */ { "SMBrmdir",reply_rmdir,AS_USER | NEED_WRITE},
/* 0x02 */ { "SMBopen",reply_open,AS_USER },
/* 0x03 */ { "SMBcreate",reply_mknew,AS_USER},
/* 0x04 */ { "SMBclose",reply_close,AS_USER | CAN_IPC },
/* 0x05 */ { "SMBflush",reply_flush,AS_USER},
/* 0x06 */ { "SMBunlink",reply_unlink,AS_USER | NEED_WRITE }, 
/* 0x07 */ { "SMBmv",reply_mv,AS_USER | NEED_WRITE },
/* 0x08 */ { "SMBgetatr",reply_getatr,AS_USER},
/* 0x09 */ { "SMBsetatr",reply_setatr,AS_USER | NEED_WRITE},
/* 0x0a */ { "SMBread",reply_read,AS_USER},
/* 0x0b */ { "SMBwrite",reply_write,AS_USER | CAN_IPC },
/* 0x0c */ { "SMBlock",reply_lock,AS_USER},
/* 0x0d */ { "SMBunlock",reply_unlock,AS_USER},
/* 0x0e */ { "SMBctemp",reply_ctemp,AS_USER },
/* 0x0f */ { "SMBmknew",reply_mknew,AS_USER}, 
/* 0x10 */ { "SMBchkpth",reply_chkpth,AS_USER},
/* 0x11 */ { "SMBexit",reply_exit,DO_CHDIR},
/* 0x12 */ { "SMBlseek",reply_lseek,AS_USER},
/* 0x13 */ { "SMBlockread",reply_lockread,AS_USER},
/* 0x14 */ { "SMBwriteunlock",reply_writeunlock,AS_USER},
/* 0x15 */ { NULL, NULL, 0 },
/* 0x16 */ { NULL, NULL, 0 },
/* 0x17 */ { NULL, NULL, 0 },
/* 0x18 */ { NULL, NULL, 0 },
/* 0x19 */ { NULL, NULL, 0 },
/* 0x1a */ { "SMBreadbraw",reply_readbraw,AS_USER},
/* 0x1b */ { "SMBreadBmpx",reply_readbmpx,AS_USER},
/* 0x1c */ { "SMBreadBs",NULL,0 },
/* 0x1d */ { "SMBwritebraw",reply_writebraw,AS_USER},
/* 0x1e */ { "SMBwriteBmpx",reply_writebmpx,AS_USER},
/* 0x1f */ { "SMBwriteBs",reply_writebs,AS_USER},
/* 0x20 */ { "SMBwritec",NULL,0},
/* 0x21 */ { NULL, NULL, 0 },
/* 0x22 */ { "SMBsetattrE",reply_setattrE,AS_USER | NEED_WRITE },
/* 0x23 */ { "SMBgetattrE",reply_getattrE,AS_USER },
/* 0x24 */ { "SMBlockingX",reply_lockingX,AS_USER },
/* 0x25 */ { "SMBtrans",reply_trans,AS_USER | CAN_IPC },
/* 0x26 */ { "SMBtranss",reply_transs,AS_USER | CAN_IPC},
/* 0x27 */ { "SMBioctl",reply_ioctl,0},
/* 0x28 */ { "SMBioctls",NULL,AS_USER},
/* 0x29 */ { "SMBcopy",reply_copy,AS_USER | NEED_WRITE },
/* 0x2a */ { "SMBmove",NULL,AS_USER | NEED_WRITE },
/* 0x2b */ { "SMBecho",reply_echo,0},
/* 0x2c */ { "SMBwriteclose",reply_writeclose,AS_USER},
/* 0x2d */ { "SMBopenX",reply_open_and_X,AS_USER | CAN_IPC },
/* 0x2e */ { "SMBreadX",reply_read_and_X,AS_USER | CAN_IPC },
/* 0x2f */ { "SMBwriteX",reply_write_and_X,AS_USER | CAN_IPC },
/* 0x30 */ { NULL, NULL, 0 },
/* 0x31 */ { NULL, NULL, 0 },
/* 0x32 */ { "SMBtrans2", reply_trans2, AS_USER | CAN_IPC },
/* 0x33 */ { "SMBtranss2", reply_transs2, AS_USER},
/* 0x34 */ { "SMBfindclose", reply_findclose,AS_USER},
/* 0x35 */ { "SMBfindnclose", reply_findnclose, AS_USER},
/* 0x36 */ { NULL, NULL, 0 },
/* 0x37 */ { NULL, NULL, 0 },
/* 0x38 */ { NULL, NULL, 0 },
/* 0x39 */ { NULL, NULL, 0 },
/* 0x3a */ { NULL, NULL, 0 },
/* 0x3b */ { NULL, NULL, 0 },
/* 0x3c */ { NULL, NULL, 0 },
/* 0x3d */ { NULL, NULL, 0 },
/* 0x3e */ { NULL, NULL, 0 },
/* 0x3f */ { NULL, NULL, 0 },
/* 0x40 */ { NULL, NULL, 0 },
/* 0x41 */ { NULL, NULL, 0 },
/* 0x42 */ { NULL, NULL, 0 },
/* 0x43 */ { NULL, NULL, 0 },
/* 0x44 */ { NULL, NULL, 0 },
/* 0x45 */ { NULL, NULL, 0 },
/* 0x46 */ { NULL, NULL, 0 },
/* 0x47 */ { NULL, NULL, 0 },
/* 0x48 */ { NULL, NULL, 0 },
/* 0x49 */ { NULL, NULL, 0 },
/* 0x4a */ { NULL, NULL, 0 },
/* 0x4b */ { NULL, NULL, 0 },
/* 0x4c */ { NULL, NULL, 0 },
/* 0x4d */ { NULL, NULL, 0 },
/* 0x4e */ { NULL, NULL, 0 },
/* 0x4f */ { NULL, NULL, 0 },
/* 0x50 */ { NULL, NULL, 0 },
/* 0x51 */ { NULL, NULL, 0 },
/* 0x52 */ { NULL, NULL, 0 },
/* 0x53 */ { NULL, NULL, 0 },
/* 0x54 */ { NULL, NULL, 0 },
/* 0x55 */ { NULL, NULL, 0 },
/* 0x56 */ { NULL, NULL, 0 },
/* 0x57 */ { NULL, NULL, 0 },
/* 0x58 */ { NULL, NULL, 0 },
/* 0x59 */ { NULL, NULL, 0 },
/* 0x5a */ { NULL, NULL, 0 },
/* 0x5b */ { NULL, NULL, 0 },
/* 0x5c */ { NULL, NULL, 0 },
/* 0x5d */ { NULL, NULL, 0 },
/* 0x5e */ { NULL, NULL, 0 },
/* 0x5f */ { NULL, NULL, 0 },
/* 0x60 */ { NULL, NULL, 0 },
/* 0x61 */ { NULL, NULL, 0 },
/* 0x62 */ { NULL, NULL, 0 },
/* 0x63 */ { NULL, NULL, 0 },
/* 0x64 */ { NULL, NULL, 0 },
/* 0x65 */ { NULL, NULL, 0 },
/* 0x66 */ { NULL, NULL, 0 },
/* 0x67 */ { NULL, NULL, 0 },
/* 0x68 */ { NULL, NULL, 0 },
/* 0x69 */ { NULL, NULL, 0 },
/* 0x6a */ { NULL, NULL, 0 },
/* 0x6b */ { NULL, NULL, 0 },
/* 0x6c */ { NULL, NULL, 0 },
/* 0x6d */ { NULL, NULL, 0 },
/* 0x6e */ { NULL, NULL, 0 },
/* 0x6f */ { NULL, NULL, 0 },
/* 0x70 */ { "SMBtcon",reply_tcon,0},
/* 0x71 */ { "SMBtdis",reply_tdis,DO_CHDIR},
/* 0x72 */ { "SMBnegprot",reply_negprot,0},
/* 0x73 */ { "SMBsesssetupX",reply_sesssetup_and_X,0},
/* 0x74 */ { "SMBulogoffX", reply_ulogoffX, 0}, /* ulogoff doesn't give a valid TID */
/* 0x75 */ { "SMBtconX",reply_tcon_and_X,0},
/* 0x76 */ { NULL, NULL, 0 },
/* 0x77 */ { NULL, NULL, 0 },
/* 0x78 */ { NULL, NULL, 0 },
/* 0x79 */ { NULL, NULL, 0 },
/* 0x7a */ { NULL, NULL, 0 },
/* 0x7b */ { NULL, NULL, 0 },
/* 0x7c */ { NULL, NULL, 0 },
/* 0x7d */ { NULL, NULL, 0 },
/* 0x7e */ { NULL, NULL, 0 },
/* 0x7f */ { NULL, NULL, 0 },
/* 0x80 */ { "SMBdskattr",reply_dskattr,AS_USER},
/* 0x81 */ { "SMBsearch",reply_search,AS_USER},
/* 0x82 */ { "SMBffirst",reply_search,AS_USER},
/* 0x83 */ { "SMBfunique",reply_search,AS_USER},
/* 0x84 */ { "SMBfclose",reply_fclose,AS_USER},
/* 0x85 */ { NULL, NULL, 0 },
/* 0x86 */ { NULL, NULL, 0 },
/* 0x87 */ { NULL, NULL, 0 },
/* 0x88 */ { NULL, NULL, 0 },
/* 0x89 */ { NULL, NULL, 0 },
/* 0x8a */ { NULL, NULL, 0 },
/* 0x8b */ { NULL, NULL, 0 },
/* 0x8c */ { NULL, NULL, 0 },
/* 0x8d */ { NULL, NULL, 0 },
/* 0x8e */ { NULL, NULL, 0 },
/* 0x8f */ { NULL, NULL, 0 },
/* 0x90 */ { NULL, NULL, 0 },
/* 0x91 */ { NULL, NULL, 0 },
/* 0x92 */ { NULL, NULL, 0 },
/* 0x93 */ { NULL, NULL, 0 },
/* 0x94 */ { NULL, NULL, 0 },
/* 0x95 */ { NULL, NULL, 0 },
/* 0x96 */ { NULL, NULL, 0 },
/* 0x97 */ { NULL, NULL, 0 },
/* 0x98 */ { NULL, NULL, 0 },
/* 0x99 */ { NULL, NULL, 0 },
/* 0x9a */ { NULL, NULL, 0 },
/* 0x9b */ { NULL, NULL, 0 },
/* 0x9c */ { NULL, NULL, 0 },
/* 0x9d */ { NULL, NULL, 0 },
/* 0x9e */ { NULL, NULL, 0 },
/* 0x9f */ { NULL, NULL, 0 },
/* 0xa0 */ { "SMBnttrans", reply_nttrans, AS_USER | CAN_IPC },
/* 0xa1 */ { "SMBnttranss", reply_nttranss, AS_USER | CAN_IPC },
/* 0xa2 */ { "SMBntcreateX", reply_ntcreate_and_X, AS_USER | CAN_IPC },
/* 0xa3 */ { NULL, NULL, 0 },
/* 0xa4 */ { "SMBntcancel", reply_ntcancel, 0 },
/* 0xa5 */ { "SMBntrename", reply_ntrename, AS_USER | NEED_WRITE },
/* 0xa6 */ { NULL, NULL, 0 },
/* 0xa7 */ { NULL, NULL, 0 },
/* 0xa8 */ { NULL, NULL, 0 },
/* 0xa9 */ { NULL, NULL, 0 },
/* 0xaa */ { NULL, NULL, 0 },
/* 0xab */ { NULL, NULL, 0 },
/* 0xac */ { NULL, NULL, 0 },
/* 0xad */ { NULL, NULL, 0 },
/* 0xae */ { NULL, NULL, 0 },
/* 0xaf */ { NULL, NULL, 0 },
/* 0xb0 */ { NULL, NULL, 0 },
/* 0xb1 */ { NULL, NULL, 0 },
/* 0xb2 */ { NULL, NULL, 0 },
/* 0xb3 */ { NULL, NULL, 0 },
/* 0xb4 */ { NULL, NULL, 0 },
/* 0xb5 */ { NULL, NULL, 0 },
/* 0xb6 */ { NULL, NULL, 0 },
/* 0xb7 */ { NULL, NULL, 0 },
/* 0xb8 */ { NULL, NULL, 0 },
/* 0xb9 */ { NULL, NULL, 0 },
/* 0xba */ { NULL, NULL, 0 },
/* 0xbb */ { NULL, NULL, 0 },
/* 0xbc */ { NULL, NULL, 0 },
/* 0xbd */ { NULL, NULL, 0 },
/* 0xbe */ { NULL, NULL, 0 },
/* 0xbf */ { NULL, NULL, 0 },
/* 0xc0 */ { "SMBsplopen",reply_printopen,AS_USER},
/* 0xc1 */ { "SMBsplwr",reply_printwrite,AS_USER},
/* 0xc2 */ { "SMBsplclose",reply_printclose,AS_USER},
/* 0xc3 */ { "SMBsplretq",reply_printqueue,AS_USER},
/* 0xc4 */ { NULL, NULL, 0 },
/* 0xc5 */ { NULL, NULL, 0 },
/* 0xc6 */ { NULL, NULL, 0 },
/* 0xc7 */ { NULL, NULL, 0 },
/* 0xc8 */ { NULL, NULL, 0 },
/* 0xc9 */ { NULL, NULL, 0 },
/* 0xca */ { NULL, NULL, 0 },
/* 0xcb */ { NULL, NULL, 0 },
/* 0xcc */ { NULL, NULL, 0 },
/* 0xcd */ { NULL, NULL, 0 },
/* 0xce */ { NULL, NULL, 0 },
/* 0xcf */ { NULL, NULL, 0 },
/* 0xd0 */ { "SMBsends",reply_sends,AS_GUEST},
/* 0xd1 */ { "SMBsendb",NULL,AS_GUEST},
/* 0xd2 */ { "SMBfwdname",NULL,AS_GUEST},
/* 0xd3 */ { "SMBcancelf",NULL,AS_GUEST},
/* 0xd4 */ { "SMBgetmac",NULL,AS_GUEST},
/* 0xd5 */ { "SMBsendstrt",reply_sendstrt,AS_GUEST},
/* 0xd6 */ { "SMBsendend",reply_sendend,AS_GUEST},
/* 0xd7 */ { "SMBsendtxt",reply_sendtxt,AS_GUEST},
/* 0xd8 */ { NULL, NULL, 0 },
/* 0xd9 */ { NULL, NULL, 0 },
/* 0xda */ { NULL, NULL, 0 },
/* 0xdb */ { NULL, NULL, 0 },
/* 0xdc */ { NULL, NULL, 0 },
/* 0xdd */ { NULL, NULL, 0 },
/* 0xde */ { NULL, NULL, 0 },
/* 0xdf */ { NULL, NULL, 0 },
/* 0xe0 */ { NULL, NULL, 0 },
/* 0xe1 */ { NULL, NULL, 0 },
/* 0xe2 */ { NULL, NULL, 0 },
/* 0xe3 */ { NULL, NULL, 0 },
/* 0xe4 */ { NULL, NULL, 0 },
/* 0xe5 */ { NULL, NULL, 0 },
/* 0xe6 */ { NULL, NULL, 0 },
/* 0xe7 */ { NULL, NULL, 0 },
/* 0xe8 */ { NULL, NULL, 0 },
/* 0xe9 */ { NULL, NULL, 0 },
/* 0xea */ { NULL, NULL, 0 },
/* 0xeb */ { NULL, NULL, 0 },
/* 0xec */ { NULL, NULL, 0 },
/* 0xed */ { NULL, NULL, 0 },
/* 0xee */ { NULL, NULL, 0 },
/* 0xef */ { NULL, NULL, 0 },
/* 0xf0 */ { NULL, NULL, 0 },
/* 0xf1 */ { NULL, NULL, 0 },
/* 0xf2 */ { NULL, NULL, 0 },
/* 0xf3 */ { NULL, NULL, 0 },
/* 0xf4 */ { NULL, NULL, 0 },
/* 0xf5 */ { NULL, NULL, 0 },
/* 0xf6 */ { NULL, NULL, 0 },
/* 0xf7 */ { NULL, NULL, 0 },
/* 0xf8 */ { NULL, NULL, 0 },
/* 0xf9 */ { NULL, NULL, 0 },
/* 0xfa */ { NULL, NULL, 0 },
/* 0xfb */ { NULL, NULL, 0 },
/* 0xfc */ { NULL, NULL, 0 },
/* 0xfd */ { NULL, NULL, 0 },
/* 0xfe */ { NULL, NULL, 0 },
/* 0xff */ { NULL, NULL, 0 }

};

/*******************************************************************
 Dump a packet to a file.
********************************************************************/

static void smb_dump(const char *name, int type, char *data, ssize_t len)
{
	int fd, i;
	pstring fname;
	if (DEBUGLEVEL < 50) return;

	if (len < 4) len = smb_len(data)+4;
	for (i=1;i<100;i++) {
		slprintf(fname,sizeof(fname)-1, "/tmp/%s.%d.%s", name, i,
				type ? "req" : "resp");
		fd = open(fname, O_WRONLY|O_CREAT|O_EXCL, 0644);
		if (fd != -1 || errno != EEXIST) break;
	}
	if (fd != -1) {
		ssize_t ret = write(fd, data, len);
		if (ret != len)
			DEBUG(0,("smb_dump: problem: write returned %d\n", (int)ret ));
		close(fd);
		DEBUG(0,("created %s len %lu\n", fname, (unsigned long)len));
	}
}


/****************************************************************************
 Do a switch on the message type, and return the response size
****************************************************************************/

static int switch_message(int type,char *inbuf,char *outbuf,int size,int bufsize)
{
	static pid_t pid= (pid_t)-1;
	int outsize = 0;

	type &= 0xff;

	if (pid == (pid_t)-1)
		pid = sys_getpid();

	errno = 0;
	set_saved_ntstatus(NT_STATUS_OK);

	last_message = type;

	/* Make sure this is an SMB packet. smb_size contains NetBIOS header so subtract 4 from it. */
	if ((strncmp(smb_base(inbuf),"\377SMB",4) != 0) || (size < (smb_size - 4))) {
		DEBUG(2,("Non-SMB packet of length %d. Terminating server\n",smb_len(inbuf)));
		exit_server_cleanly("Non-SMB packet");
		return(-1);
	}

	/* yuck! this is an interim measure before we get rid of our
		current inbuf/outbuf system */
	global_smbpid = SVAL(inbuf,smb_pid);

	if (smb_messages[type].fn == NULL) {
		DEBUG(0,("Unknown message type %d!\n",type));
		smb_dump("Unknown", 1, inbuf, size);
		outsize = reply_unknown(inbuf,outbuf);
	} else {
		int flags = smb_messages[type].flags;
		static uint16 last_session_tag = UID_FIELD_INVALID;
		/* In share mode security we must ignore the vuid. */
		uint16 session_tag = (lp_security() == SEC_SHARE) ? UID_FIELD_INVALID : SVAL(inbuf,smb_uid);
		connection_struct *conn = conn_find(SVAL(inbuf,smb_tid));

		DEBUG(3,("switch message %s (pid %d) conn 0x%lx\n",smb_fn_name(type),(int)pid,(unsigned long)conn));

		smb_dump(smb_fn_name(type), 1, inbuf, size);

		/* Ensure this value is replaced in the incoming packet. */
		SSVAL(inbuf,smb_uid,session_tag);

		/*
		 * Ensure the correct username is in current_user_info.
		 * This is a really ugly bugfix for problems with
		 * multiple session_setup_and_X's being done and
		 * allowing %U and %G substitutions to work correctly.
		 * There is a reason this code is done here, don't
		 * move it unless you know what you're doing... :-).
		 * JRA.
		 */

		if (session_tag != last_session_tag) {
			user_struct *vuser = NULL;

			last_session_tag = session_tag;
			if(session_tag != UID_FIELD_INVALID) {
				vuser = get_valid_user_struct(session_tag);           
				if (vuser) {
					set_current_user_info(&vuser->user);
				}
			}
		}

		/* Does this call need to be run as the connected user? */
		if (flags & AS_USER) {

			/* Does this call need a valid tree connection? */
			if (!conn) {
				/* Amazingly, the error code depends on the command (from Samba4). */
				if (type == SMBntcreateX) {
					return ERROR_NT(NT_STATUS_INVALID_HANDLE);
				} else {
					return ERROR_DOS(ERRSRV, ERRinvnid);
				}
			}

			if (!change_to_user(conn,session_tag)) {
				return(ERROR_FORCE_DOS(ERRSRV,ERRbaduid));
			}

			/* All NEED_WRITE and CAN_IPC flags must also have AS_USER. */

			/* Does it need write permission? */
			if ((flags & NEED_WRITE) && !CAN_WRITE(conn)) {
				return(ERROR_DOS(ERRSRV,ERRaccess));
			}

			/* IPC services are limited */
			if (IS_IPC(conn) && !(flags & CAN_IPC)) {
				return(ERROR_DOS(ERRSRV,ERRaccess));
			}
		} else {
			/* This call needs to be run as root */
			change_to_root_user();
		}

		/* load service specific parameters */
		if (conn) {
			if (!set_current_service(conn,SVAL(inbuf,smb_flg),(flags & (AS_USER|DO_CHDIR)?True:False))) {
				return(ERROR_DOS(ERRSRV,ERRaccess));
			}
			conn->num_smb_operations++;
		}

		/* does this protocol need to be run as guest? */
		if ((flags & AS_GUEST) && (!change_to_guest() || 
				!check_access(smbd_server_fd(), lp_hostsallow(-1), lp_hostsdeny(-1)))) {
			return(ERROR_DOS(ERRSRV,ERRaccess));
		}

		current_inbuf = inbuf; /* In case we need to defer this message in open... */
		outsize = smb_messages[type].fn(conn, inbuf,outbuf,size,bufsize);
	}

	smb_dump(smb_fn_name(type), 0, outbuf, outsize);

	return(outsize);
}

/****************************************************************************
 Construct a reply to the incoming packet.
****************************************************************************/

static int construct_reply(char *inbuf,char *outbuf,int size,int bufsize)
{
	int type = CVAL(inbuf,smb_com);
	int outsize = 0;
	int msg_type = CVAL(inbuf,0);

	chain_size = 0;
	file_chain_reset();
	reset_chain_p();

	if (msg_type != 0)
		return(reply_special(inbuf,outbuf));  

	construct_reply_common(inbuf, outbuf);

	outsize = switch_message(type,inbuf,outbuf,size,bufsize);

	outsize += chain_size;

	if(outsize > 4)
		smb_setlen(outbuf,outsize - 4);
	return(outsize);
}

/****************************************************************************
 Keep track of the number of running smbd's. This functionality is used to
 'hard' limit Samba overhead on resource constrained systems. 
****************************************************************************/

static BOOL process_count_update_successful = False;

static int32 increment_smbd_process_count(void)
{
	int32 total_smbds;

	if (lp_max_smbd_processes()) {
		total_smbds = 0;
		if (tdb_change_int32_atomic(conn_tdb_ctx(), "INFO/total_smbds", &total_smbds, 1) == -1)
			return 1;
		process_count_update_successful = True;
		return total_smbds + 1;
	}
	return 1;
}

void decrement_smbd_process_count(void)
{
	int32 total_smbds;

	if (lp_max_smbd_processes() && process_count_update_successful) {
		total_smbds = 1;
		tdb_change_int32_atomic(conn_tdb_ctx(), "INFO/total_smbds", &total_smbds, -1);
	}
}

static BOOL smbd_process_limit(void)
{
	int32  total_smbds;
	
	if (lp_max_smbd_processes()) {

		/* Always add one to the smbd process count, as exit_server() always
		 * subtracts one.
		 */

		if (!conn_tdb_ctx()) {
			DEBUG(0,("smbd_process_limit: max smbd processes parameter set with status parameter not \
set. Ignoring max smbd restriction.\n"));
			return False;
		}

		total_smbds = increment_smbd_process_count();
		return total_smbds > lp_max_smbd_processes();
	}
	else
		return False;
}

/****************************************************************************
 Process an smb from the client
****************************************************************************/

static void process_smb(char *inbuf, char *outbuf)
{
	static int trans_num;
	int msg_type = CVAL(inbuf,0);
	int32 len = smb_len(inbuf);
	int nread = len + 4;

	DO_PROFILE_INC(smb_count);

	if (trans_num == 0) {
		/* on the first packet, check the global hosts allow/ hosts
		deny parameters before doing any parsing of the packet
		passed to us by the client.  This prevents attacks on our
		parsing code from hosts not in the hosts allow list */
		if (smbd_process_limit() ||
				!check_access(smbd_server_fd(), lp_hostsallow(-1), lp_hostsdeny(-1))) {
			/* send a negative session response "not listening on calling name" */
			static unsigned char buf[5] = {0x83, 0, 0, 1, 0x81};
			DEBUG( 1, ( "Connection denied from %s\n", client_addr() ) );
			(void)send_smb(smbd_server_fd(),(char *)buf);
			exit_server_cleanly("connection denied");
		}
	}

	DEBUG( 6, ( "got message type 0x%x of len 0x%x\n", msg_type, len ) );
	DEBUG( 3, ( "Transaction %d of length %d\n", trans_num, nread ) );

	if (msg_type == 0)
		show_msg(inbuf);
	else if(msg_type == SMBkeepalive)
		return; /* Keepalive packet. */

	nread = construct_reply(inbuf,outbuf,nread,max_send);
      
	if(nread > 0) {
		if (CVAL(outbuf,0) == 0)
			show_msg(outbuf);
	
		if (nread != smb_len(outbuf) + 4) {
			DEBUG(0,("ERROR: Invalid message response size! %d %d\n",
				nread, smb_len(outbuf)));
		} else if (!send_smb(smbd_server_fd(),outbuf)) {
			exit_server_cleanly("process_smb: send_smb failed.");
		}
	}
	trans_num++;
}

/****************************************************************************
 Return a string containing the function name of a SMB command.
****************************************************************************/

const char *smb_fn_name(int type)
{
	const char *unknown_name = "SMBunknown";

	if (smb_messages[type].name == NULL)
		return(unknown_name);

	return(smb_messages[type].name);
}

/****************************************************************************
 Helper functions for contruct_reply.
****************************************************************************/

static uint32 common_flags2 = FLAGS2_LONG_PATH_COMPONENTS|FLAGS2_32_BIT_ERROR_CODES;

void add_to_common_flags2(uint32 v)
{
	common_flags2 |= v;
}

void remove_from_common_flags2(uint32 v)
{
	common_flags2 &= ~v;
}

void construct_reply_common(char *inbuf,char *outbuf)
{
	set_message(outbuf,0,0,False);
	
	SCVAL(outbuf,smb_com,CVAL(inbuf,smb_com));
	SIVAL(outbuf,smb_rcls,0);
	SCVAL(outbuf,smb_flg, FLAG_REPLY | (CVAL(inbuf,smb_flg) & FLAG_CASELESS_PATHNAMES)); 
	SSVAL(outbuf,smb_flg2,
		(SVAL(inbuf,smb_flg2) & FLAGS2_UNICODE_STRINGS) |
		common_flags2);
	memset(outbuf+smb_pidhigh,'\0',(smb_tid-smb_pidhigh));

	SSVAL(outbuf,smb_tid,SVAL(inbuf,smb_tid));
	SSVAL(outbuf,smb_pid,SVAL(inbuf,smb_pid));
	SSVAL(outbuf,smb_uid,SVAL(inbuf,smb_uid));
	SSVAL(outbuf,smb_mid,SVAL(inbuf,smb_mid));
}

/****************************************************************************
 Construct a chained reply and add it to the already made reply
****************************************************************************/

int chain_reply(char *inbuf,char *outbuf,int size,int bufsize)
{
	static char *orig_inbuf;
	static char *orig_outbuf;
	int smb_com1, smb_com2 = CVAL(inbuf,smb_vwv0);
	unsigned smb_off2 = SVAL(inbuf,smb_vwv1);
	char *inbuf2, *outbuf2;
	int outsize2;
	char inbuf_saved[smb_wct];
	char outbuf_saved[smb_wct];
	int outsize = smb_len(outbuf) + 4;

	/* maybe its not chained */
	if (smb_com2 == 0xFF) {
		SCVAL(outbuf,smb_vwv0,0xFF);
		return outsize;
	}

	if (chain_size == 0) {
		/* this is the first part of the chain */
		orig_inbuf = inbuf;
		orig_outbuf = outbuf;
	}

	/*
	 * The original Win95 redirector dies on a reply to
	 * a lockingX and read chain unless the chain reply is
	 * 4 byte aligned. JRA.
	 */

	outsize = (outsize + 3) & ~3;

	/* we need to tell the client where the next part of the reply will be */
	SSVAL(outbuf,smb_vwv1,smb_offset(outbuf+outsize,outbuf));
	SCVAL(outbuf,smb_vwv0,smb_com2);

	/* remember how much the caller added to the chain, only counting stuff
		after the parameter words */
	chain_size += outsize - smb_wct;

	/* work out pointers into the original packets. The
		headers on these need to be filled in */
	inbuf2 = orig_inbuf + smb_off2 + 4 - smb_wct;
	outbuf2 = orig_outbuf + SVAL(outbuf,smb_vwv1) + 4 - smb_wct;

	/* remember the original command type */
	smb_com1 = CVAL(orig_inbuf,smb_com);

	/* save the data which will be overwritten by the new headers */
	memcpy(inbuf_saved,inbuf2,smb_wct);
	memcpy(outbuf_saved,outbuf2,smb_wct);

	/* give the new packet the same header as the last part of the SMB */
	memmove(inbuf2,inbuf,smb_wct);

	/* create the in buffer */
	SCVAL(inbuf2,smb_com,smb_com2);

	/* create the out buffer */
	construct_reply_common(inbuf2, outbuf2);

	DEBUG(3,("Chained message\n"));
	show_msg(inbuf2);

	/* process the request */
	outsize2 = switch_message(smb_com2,inbuf2,outbuf2,size-chain_size,
				bufsize-chain_size);

	/* copy the new reply and request headers over the old ones, but
		preserve the smb_com field */
	memmove(orig_outbuf,outbuf2,smb_wct);
	SCVAL(orig_outbuf,smb_com,smb_com1);

	/* restore the saved data, being careful not to overwrite any
		data from the reply header */
	memcpy(inbuf2,inbuf_saved,smb_wct);

	{
		int ofs = smb_wct - PTR_DIFF(outbuf2,orig_outbuf);
		if (ofs < 0) ofs = 0;
			memmove(outbuf2+ofs,outbuf_saved+ofs,smb_wct-ofs);
	}

	return outsize2;
}

/****************************************************************************
 Setup the needed select timeout.
****************************************************************************/

static int setup_select_timeout(void)
{
	int select_timeout;
	int t;

	select_timeout = blocking_locks_timeout(SMBD_SELECT_TIMEOUT);
	select_timeout *= 1000;

	t = change_notify_timeout();
	DEBUG(10, ("change_notify_timeout: %d\n", t));
	if (t != -1)
		select_timeout = MIN(select_timeout, t*1000);

	if (print_notify_messages_pending())
		select_timeout = MIN(select_timeout, 1000);

	return select_timeout;
}

/****************************************************************************
 Check if services need reloading.
****************************************************************************/

void check_reload(time_t t)
{
	static pid_t mypid = 0;
	static time_t last_smb_conf_reload_time = 0;
	static time_t last_printer_reload_time = 0;
	time_t printcap_cache_time = (time_t)lp_printcap_cache_time();

	if(last_smb_conf_reload_time == 0) {
		last_smb_conf_reload_time = t;
		/* Our printing subsystem might not be ready at smbd start up.
		   Then no printer is available till the first printers check
		   is performed.  A lower initial interval circumvents this. */
		if ( printcap_cache_time > 60 )
			last_printer_reload_time = t - printcap_cache_time + 60;
		else
			last_printer_reload_time = t;
	}

	if (mypid != getpid()) { /* First time or fork happened meanwhile */
		/* randomize over 60 second the printcap reload to avoid all
		 * process hitting cupsd at the same time */
		int time_range = 60;

		last_printer_reload_time += random() % time_range;
		mypid = getpid();
	}

	if (reload_after_sighup || (t >= last_smb_conf_reload_time+SMBD_RELOAD_CHECK)) {
		reload_services(True);
		reload_after_sighup = False;
		last_smb_conf_reload_time = t;
	}

	/* 'printcap cache time = 0' disable the feature */
	
	if ( printcap_cache_time != 0 )
	{ 
		/* see if it's time to reload or if the clock has been set back */
		
		if ( (t >= last_printer_reload_time+printcap_cache_time) 
			|| (t-last_printer_reload_time  < 0) ) 
		{
			DEBUG( 3,( "Printcap cache time expired.\n"));
			reload_printers();
			last_printer_reload_time = t;
		}
	}
}

/****************************************************************************
 Process any timeout housekeeping. Return False if the caller should exit.
****************************************************************************/

static BOOL timeout_processing(int deadtime, int *select_timeout, time_t *last_timeout_processing_time)
{
	static time_t last_keepalive_sent_time = 0;
	static time_t last_idle_closed_check = 0;
	time_t t;
	BOOL allidle = True;

	if (smb_read_error == READ_EOF) {
		DEBUG(3,("timeout_processing: End of file from client (client has disconnected).\n"));
		return False;
	}

	if (smb_read_error == READ_ERROR) {
		DEBUG(3,("timeout_processing: receive_smb error (%s) Exiting\n",
			strerror(errno)));
		return False;
	}

	if (smb_read_error == READ_BAD_SIG) {
		DEBUG(3,("timeout_processing: receive_smb error bad smb signature. Exiting\n"));
		return False;
	}

	*last_timeout_processing_time = t = time(NULL);

	if(last_keepalive_sent_time == 0)
		last_keepalive_sent_time = t;

	if(last_idle_closed_check == 0)
		last_idle_closed_check = t;

	/* become root again if waiting */
	change_to_root_user();

	/* run all registered idle events */
	smb_run_idle_events(t);

	/* check if we need to reload services */
	check_reload(t);

	/* automatic timeout if all connections are closed */      
	if (conn_num_open()==0 && (t - last_idle_closed_check) >= IDLE_CLOSED_TIMEOUT) {
		DEBUG( 2, ( "Closing idle connection\n" ) );
		return False;
	} else {
		last_idle_closed_check = t;
	}

	if (keepalive && (t - last_keepalive_sent_time)>keepalive) {
		if (!send_keepalive(smbd_server_fd())) {
			DEBUG( 2, ( "Keepalive failed - exiting.\n" ) );
			return False;
		}

		/* send a keepalive for a password server or the like.
			This is attached to the auth_info created in the
		negprot */
		if (negprot_global_auth_context && negprot_global_auth_context->challenge_set_method 
				&& negprot_global_auth_context->challenge_set_method->send_keepalive) {

			negprot_global_auth_context->challenge_set_method->send_keepalive
			(&negprot_global_auth_context->challenge_set_method->private_data);
		}

		last_keepalive_sent_time = t;
	}

	/* check for connection timeouts */
	allidle = conn_idle_all(t, deadtime);

	if (allidle && conn_num_open()>0) {
		DEBUG(2,("Closing idle connection 2.\n"));
		return False;
	}

	if(global_machine_password_needs_changing && 
			/* for ADS we need to do a regular ADS password change, not a domain
					password change */
			lp_security() == SEC_DOMAIN) {

		unsigned char trust_passwd_hash[16];
		time_t lct;

		/*
		 * We're in domain level security, and the code that
		 * read the machine password flagged that the machine
		 * password needs changing.
		 */

		/*
		 * First, open the machine password file with an exclusive lock.
		 */

		if (secrets_lock_trust_account_password(lp_workgroup(), True) == False) {
			DEBUG(0,("process: unable to lock the machine account password for \
machine %s in domain %s.\n", global_myname(), lp_workgroup() ));
			return True;
		}

		if(!secrets_fetch_trust_account_password(lp_workgroup(), trust_passwd_hash, &lct, NULL)) {
			DEBUG(0,("process: unable to read the machine account password for \
machine %s in domain %s.\n", global_myname(), lp_workgroup()));
			secrets_lock_trust_account_password(lp_workgroup(), False);
			return True;
		}

		/*
		 * Make sure someone else hasn't already done this.
		 */

		if(t < lct + lp_machine_password_timeout()) {
			global_machine_password_needs_changing = False;
			secrets_lock_trust_account_password(lp_workgroup(), False);
			return True;
		}

		/* always just contact the PDC here */
    
		change_trust_account_password( lp_workgroup(), NULL);
		global_machine_password_needs_changing = False;
		secrets_lock_trust_account_password(lp_workgroup(), False);
	}

	/*
	 * Check to see if we have any blocking locks
	 * outstanding on the queue.
	 */
	process_blocking_lock_queue(t);

	/* update printer queue caches if necessary */
  
	update_monitored_printq_cache();
  
	/*
	 * Check to see if we have any change notifies 
	 * outstanding on the queue.
	 */
	process_pending_change_notify_queue(t);

	/*
	 * Now we are root, check if the log files need pruning.
	 * Force a log file check.
	 */
	force_check_log_size();
	check_log_size();

	/* Send any queued printer notify message to interested smbd's. */

	print_notify_send_messages(0);

	/*
	 * Modify the select timeout depending upon
	 * what we have remaining in our queues.
	 */

	*select_timeout = setup_select_timeout();

	return True;
}

/****************************************************************************
 Accessor functions for InBuffer, OutBuffer.
****************************************************************************/

char *get_InBuffer(void)
{
	return InBuffer;
}

void set_InBuffer(char *new_inbuf)
{
	InBuffer = new_inbuf;
	current_inbuf = InBuffer;
}

char *get_OutBuffer(void)
{
	return OutBuffer;
}

void set_OutBuffer(char *new_outbuf)
{
	OutBuffer = new_outbuf;
}

/****************************************************************************
 Free an InBuffer. Checks if not in use by aio system.
 Must have been allocated by NewInBuffer.
****************************************************************************/

void free_InBuffer(char *inbuf)
{
	if (!aio_inbuffer_in_use(inbuf)) {
		if (current_inbuf == inbuf) {
			current_inbuf = NULL;
		}
		SAFE_FREE(inbuf);
	}
}

/****************************************************************************
 Free an OutBuffer. No outbuffers currently stolen by aio system.
 Must have been allocated by NewInBuffer.
****************************************************************************/

void free_OutBuffer(char *outbuf)
{
	SAFE_FREE(outbuf);
}

const int total_buffer_size = (BUFFER_SIZE + LARGE_WRITEX_HDR_SIZE + SAFETY_MARGIN);

/****************************************************************************
 Allocate a new InBuffer. Returns the new and old ones.
****************************************************************************/

char *NewInBuffer(char **old_inbuf)
{
	char *new_inbuf = (char *)SMB_MALLOC(total_buffer_size);
	if (!new_inbuf) {
		return NULL;
	}
	if (old_inbuf) {
		*old_inbuf = InBuffer;
	}
	InBuffer = new_inbuf;
#if defined(DEVELOPER)
	clobber_region(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, InBuffer, total_buffer_size);
#endif
	return InBuffer;
}

/****************************************************************************
 Allocate a new OutBuffer. Returns the new and old ones.
****************************************************************************/

char *NewOutBuffer(char **old_outbuf)
{
	char *new_outbuf = (char *)SMB_MALLOC(total_buffer_size);
	if (!new_outbuf) {
		return NULL;
	}
	if (old_outbuf) {
		*old_outbuf = OutBuffer;
	}
	OutBuffer = new_outbuf;
#if defined(DEVELOPER)
	clobber_region(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, OutBuffer, total_buffer_size);
#endif
	return OutBuffer;
}

/****************************************************************************
 Process commands from the client
****************************************************************************/

void smbd_process(void)
{
	time_t last_timeout_processing_time = time(NULL);
	unsigned int num_smbs = 0;

	/* Allocate the primary Inbut/Output buffers. */

	if ((NewInBuffer(NULL) == NULL) || (NewOutBuffer(NULL) == NULL)) 
		return;

	max_recv = MIN(lp_maxxmit(),BUFFER_SIZE);

	while (True) {
		int deadtime = lp_deadtime()*60;
		int select_timeout = setup_select_timeout();
		int num_echos;

		if (deadtime <= 0)
			deadtime = DEFAULT_SMBD_TIMEOUT;

		errno = 0;      
		
		/* free up temporary memory */
		lp_TALLOC_FREE();
		main_loop_TALLOC_FREE();

		/* Did someone ask for immediate checks on things like blocking locks ? */
		if (select_timeout == 0) {
			if(!timeout_processing( deadtime, &select_timeout, &last_timeout_processing_time))
				return;
			num_smbs = 0; /* Reset smb counter. */
		}

		run_events();

#if defined(DEVELOPER)
		clobber_region(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, InBuffer, total_buffer_size);
#endif

		while (!receive_message_or_smb(InBuffer,BUFFER_SIZE+LARGE_WRITEX_HDR_SIZE,select_timeout)) {
			if(!timeout_processing( deadtime, &select_timeout, &last_timeout_processing_time))
				return;
			num_smbs = 0; /* Reset smb counter. */
		}

		/*
		 * Ensure we do timeout processing if the SMB we just got was
		 * only an echo request. This allows us to set the select
		 * timeout in 'receive_message_or_smb()' to any value we like
		 * without worrying that the client will send echo requests
		 * faster than the select timeout, thus starving out the
		 * essential processing (change notify, blocking locks) that
		 * the timeout code does. JRA.
		 */ 
		num_echos = smb_echo_count;

		clobber_region(SAFE_STRING_FUNCTION_NAME, SAFE_STRING_LINE, OutBuffer, total_buffer_size);

		process_smb(InBuffer, OutBuffer);

		if (smb_echo_count != num_echos) {
			if(!timeout_processing( deadtime, &select_timeout, &last_timeout_processing_time))
				return;
			num_smbs = 0; /* Reset smb counter. */
		}

		num_smbs++;

		/*
		 * If we are getting smb requests in a constant stream
		 * with no echos, make sure we attempt timeout processing
		 * every select_timeout milliseconds - but only check for this
		 * every 200 smb requests.
		 */
		
		if ((num_smbs % 200) == 0) {
			time_t new_check_time = time(NULL);
			if(new_check_time - last_timeout_processing_time >= (select_timeout/1000)) {
				if(!timeout_processing( deadtime, &select_timeout, &last_timeout_processing_time))
					return;
				num_smbs = 0; /* Reset smb counter. */
				last_timeout_processing_time = new_check_time; /* Reset time. */
			}
		}

		/* The timeout_processing function isn't run nearly
		   often enough to implement 'max log size' without
		   overrunning the size of the file by many megabytes.
		   This is especially true if we are running at debug
		   level 10.  Checking every 50 SMBs is a nice
		   tradeoff of performance vs log file size overrun. */

		if ((num_smbs % 50) == 0 && need_to_check_log_size()) {
			change_to_root_user();
			check_log_size();
		}
	}
}

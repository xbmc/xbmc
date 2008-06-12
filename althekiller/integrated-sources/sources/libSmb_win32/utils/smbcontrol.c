/* 
   Unix SMB/CIFS implementation.

   Send messages to other Samba daemons

   Copyright (C) Tim Potter 2003
   Copyright (C) Andrew Tridgell 1994-1998
   Copyright (C) Martin Pool 2001-2002
   Copyright (C) Simo Sorce 2002
   Copyright (C) James Peach 2006
   
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

#if HAVE_LIBUNWIND_H
#include <libunwind.h>
#endif

#if HAVE_LIBUNWIND_PTRACE_H
#include <libunwind-ptrace.h>
#endif

#if HAVE_SYS_PTRACE_H
#include <sys/ptrace.h>
#endif

/* Default timeout value when waiting for replies (in seconds) */

#define DEFAULT_TIMEOUT 10

static int timeout = DEFAULT_TIMEOUT;
static int num_replies;		/* Used by message callback fns */

/* Send a message to a destination pid.  Zero means broadcast smbd. */

static BOOL send_message(struct process_id pid, int msg_type,
			 const void *buf, int len,
			 BOOL duplicates)
{
	TDB_CONTEXT *tdb;
	BOOL ret;
	int n_sent = 0;

	if (!message_init())
		return False;

	if (procid_to_pid(&pid) != 0)
		return message_send_pid(pid, msg_type, buf, len, duplicates);

	tdb = tdb_open_log(lock_path("connections.tdb"), 0, 
			   TDB_DEFAULT, O_RDWR, 0);
	if (!tdb) {
		fprintf(stderr,"Failed to open connections database"
			": %s\n", strerror(errno));
		return False;
	}
	
	ret = message_send_all(tdb,msg_type, buf, len, duplicates,
			       &n_sent);
	DEBUG(10,("smbcontrol/send_message: broadcast message to "
		  "%d processes\n", n_sent));
	
	tdb_close(tdb);
	
	return ret;
}

/* Wait for one or more reply messages */

static void wait_replies(BOOL multiple_replies)
{
	time_t start_time = time(NULL);

	/* Wait around a bit.  This is pretty disgusting - we have to
           busy-wait here as there is no nicer way to do it. */

	do {
		message_dispatch();
		if (num_replies > 0 && !multiple_replies)
			break;
		sleep(1);
	} while (timeout - (time(NULL) - start_time) > 0);
}

/* Message handler callback that displays the PID and a string on stdout */

static void print_pid_string_cb(int msg_type, struct process_id pid, void *buf, size_t len)
{
	printf("PID %u: %.*s", (unsigned int)procid_to_pid(&pid),
	       (int)len, (const char *)buf);
	num_replies++;
}

/* Message handler callback that displays a string on stdout */

static void print_string_cb(int msg_type, struct process_id pid,
			    void *buf, size_t len)
{
	printf("%.*s", (int)len, (const char *)buf);
	num_replies++;
}

/* Send no message.  Useful for testing. */

static BOOL do_noop(const struct process_id pid,
		    const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> noop\n");
		return False;
	}

	/* Move along, nothing to see here */

	return True;
}

/* Send a debug string */

static BOOL do_debug(const struct process_id pid,
		     const int argc, const char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: smbcontrol <dest> debug "
			"<debug-string>\n");
		return False;
	}

	return send_message(
		pid, MSG_DEBUG, argv[1], strlen(argv[1]) + 1, False);
}

#if defined(HAVE_LIBUNWIND_PTRACE) && defined(HAVE_LINUX_PTRACE)

/* Return the name of a process given it's PID. This will only work on Linux,
 * but that's probably moot since this whole stack tracing implementatino is
 * Linux-specific anyway.
 */
static const char * procname(pid_t pid, char * buf, size_t bufsz)
{
	char path[64];
	FILE * fp;

	snprintf(path, sizeof(path), "/proc/%llu/cmdline",
		(unsigned long long)pid);
	if ((fp = fopen(path, "r")) == NULL) {
		return NULL;
	}

	fgets(buf, bufsz, fp);

	fclose(fp);
	return buf;
}

static void print_stack_trace(pid_t pid, int * count)
{
	void *		    pinfo = NULL;
	unw_addr_space_t    aspace = NULL;
	unw_cursor_t	    cursor;
	unw_word_t	    ip, sp;

	char		    nbuf[256];
	unw_word_t	    off;

	int ret;

	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
		fprintf(stderr,
			"Failed to attach to process %llu: %s\n",
			(unsigned long long)pid, strerror(errno));
		return;
	}

	/* Wait until the attach is complete. */
	waitpid(pid, NULL, 0);

	if (((pinfo = _UPT_create(pid)) == NULL) ||
	    ((aspace = unw_create_addr_space(&_UPT_accessors, 0)) == NULL)) {
		/* Probably out of memory. */
		fprintf(stderr,
			"Unable to initialize stack unwind for process %llu\n",
			(unsigned long long)pid);
		goto cleanup;
	}

	if ((ret = unw_init_remote(&cursor, aspace, pinfo))) {
		fprintf(stderr,
			"Unable to unwind stack for process %llu: %s\n",
			(unsigned long long)pid, unw_strerror(ret));
		goto cleanup;
	}

	if (*count > 0) {
		printf("\n");
	}

	if (procname(pid, nbuf, sizeof(nbuf))) {
		printf("Stack trace for process %llu (%s):\n",
			(unsigned long long)pid, nbuf);
	} else {
		printf("Stack trace for process %llu:\n",
			(unsigned long long)pid);
	}

	while (unw_step(&cursor) > 0) {
		ip = sp = off = 0;
		unw_get_reg(&cursor, UNW_REG_IP, &ip);
		unw_get_reg(&cursor, UNW_REG_SP, &sp);

		ret = unw_get_proc_name(&cursor, nbuf, sizeof(nbuf), &off);
		if (ret != 0 && ret != -UNW_ENOMEM) {
			snprintf(nbuf, sizeof(nbuf), "<unknown symbol>");
		}
		printf("    %s + %#llx [ip=%#llx] [sp=%#llx]\n",
			nbuf, (long long)off, (long long)ip,
			(long long)sp);
	}

	(*count)++;

cleanup:
	if (aspace) {
		unw_destroy_addr_space(aspace);
	}

	if (pinfo) {
		_UPT_destroy(pinfo);
	}

	ptrace(PTRACE_DETACH, pid, NULL, NULL);
}

static int stack_trace_connection(TDB_CONTEXT * tdb, TDB_DATA key,
	TDB_DATA data, void * priv)
{
	struct connections_data conn;

	if (data.dsize != sizeof(conn))
		return 0;

	memcpy(&conn, data.dptr, sizeof(conn));
	print_stack_trace(procid_to_pid(&conn.pid), (int *)priv);

	return 0;
}

static BOOL do_daemon_stack_trace(const struct process_id pid,
		       const int argc, const char **argv)
{
	fprintf(stderr,
		"Daemon stack tracing is not supported on this platform\n");
	return False;

	pid_t	dest;
	int	count = 0;

	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> stacktrace\n");
		return False;
	}

	dest = procid_to_pid(&pid);

	if (dest != 0) {
		/* It would be nice to be able to make sure that this PID is
		 * the PID of a smbd/winbind/nmbd process, not some random PID
		 * the user liked the look of. It doesn't seem like it's worth
		 * the effort at the moment, however.
		 */
		print_stack_trace(dest, &count);
	} else {
		TDB_CONTEXT * tdb;

		tdb = tdb_open_log(lock_path("connections.tdb"), 0, 
				   TDB_DEFAULT, O_RDONLY, 0);
		if (!tdb) {
			fprintf(stderr,
				"Failed to open connections database: %s\n",
				strerror(errno));
			return False;
		}

		tdb_traverse(tdb, stack_trace_connection, &count);
		tdb_close(tdb);
	}

	return True;
}

#else /* defined(HAVE_LIBUNWIND_PTRACE) && defined(HAVE_LINUX_PTRACE) */

static BOOL do_daemon_stack_trace(const struct process_id pid,
		       const int argc, const char **argv)
{
	fprintf(stderr,
		"Daemon stack tracing is not supported on this platform\n");
	return False;
}

#endif /* defined(HAVE_LIBUNWIND_PTRACE) && defined(HAVE_LINUX_PTRACE) */

/* Inject a fault (fatal signal) into a running smbd */

static BOOL do_inject_fault(const struct process_id pid,
		       const int argc, const char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: smbcontrol <dest> inject "
			"<bus|hup|term|internal|segv>\n");
		return False;
	}

#ifndef DEVELOPER
	fprintf(stderr, "Fault injection is only available in "
		"developer builds\n");
	return False;
#else /* DEVELOPER */
	{
		int sig = 0;

		if (strcmp(argv[1], "bus") == 0) {
			sig = SIGBUS;
		} else if (strcmp(argv[1], "hup") == 0) {
			sig = SIGHUP;
		} else if (strcmp(argv[1], "term") == 0) {
			sig = SIGTERM;
		} else if (strcmp(argv[1], "segv") == 0) {
			sig = SIGSEGV;
		} else if (strcmp(argv[1], "internal") == 0) {
			/* Force an internal error, ie. an unclean exit. */
			sig = -1;
		} else {
			fprintf(stderr, "Unknown signal name '%s'\n", argv[1]);
			return False;
		}

		return send_message(pid, MSG_SMB_INJECT_FAULT,
				    &sig, sizeof(int), False);
	}
#endif /* DEVELOPER */
}

/* Force a browser election */

static BOOL do_election(const struct process_id pid,
			const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> force-election\n");
		return False;
	}

	return send_message(
		pid, MSG_FORCE_ELECTION, NULL, 0, False);
}

/* Ping a samba daemon process */

static void pong_cb(int msg_type, struct process_id pid, void *buf, size_t len)
{
	char *src_string = procid_str(NULL, &pid);
	printf("PONG from pid %s\n", src_string);
	TALLOC_FREE(src_string);
	num_replies++;
}

static BOOL do_ping(const struct process_id pid, const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> ping\n");
		return False;
	}

	/* Send a message and register our interest in a reply */

	if (!send_message(pid, MSG_PING, NULL, 0, False))
		return False;

	message_register(MSG_PONG, pong_cb);

	wait_replies(procid_to_pid(&pid) == 0);

	/* No replies were received within the timeout period */

	if (num_replies == 0)
		printf("No replies received\n");

	message_deregister(MSG_PONG);

	return num_replies;
}

/* Set profiling options */

static BOOL do_profile(const struct process_id pid,
		       const int argc, const char **argv)
{
	int v;

	if (argc != 2) {
		fprintf(stderr, "Usage: smbcontrol <dest> profile "
			"<off|count|on|flush>\n");
		return False;
	}

	if (strcmp(argv[1], "off") == 0) {
		v = 0;
	} else if (strcmp(argv[1], "count") == 0) {
		v = 1;
	} else if (strcmp(argv[1], "on") == 0) {
		v = 2;
	} else if (strcmp(argv[1], "flush") == 0) {
		v = 3;
	} else {
		fprintf(stderr, "Unknown profile command '%s'\n", argv[1]);
		return False;
	}

	return send_message(pid, MSG_PROFILE, &v, sizeof(int), False);
}

/* Return the profiling level */

static void profilelevel_cb(int msg_type, struct process_id pid, void *buf, size_t len)
{
	int level;
	const char *s;

	num_replies++;

	if (len != sizeof(int)) {
		fprintf(stderr, "invalid message length %ld returned\n", 
			(unsigned long)len);
		return;
	}

	memcpy(&level, buf, sizeof(int));

	switch (level) {
	case 0:
		s = "not enabled";
		break;
	case 1:
		s = "off";
		break;
	case 3:
		s = "count only";
		break;
	case 7:
		s = "count and time";
		break;
	default:
		s = "BOGUS";
		break;
	}
	
	printf("Profiling %s on pid %u\n",s,(unsigned int)procid_to_pid(&pid));
}

static void profilelevel_rqst(int msg_type, struct process_id pid,
			      void *buf, size_t len)
{
	int v = 0;

	/* Send back a dummy reply */

	send_message(pid, MSG_PROFILELEVEL, &v, sizeof(int), False);
}

static BOOL do_profilelevel(const struct process_id pid,
			    const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> profilelevel\n");
		return False;
	}

	/* Send a message and register our interest in a reply */

	if (!send_message(pid, MSG_REQ_PROFILELEVEL, NULL, 0, False))
		return False;

	message_register(MSG_PROFILELEVEL, profilelevel_cb);
	message_register(MSG_REQ_PROFILELEVEL, profilelevel_rqst);

	wait_replies(procid_to_pid(&pid) == 0);

	/* No replies were received within the timeout period */

	if (num_replies == 0)
		printf("No replies received\n");

	message_deregister(MSG_PROFILE);

	return num_replies;
}

/* Display debug level settings */

static BOOL do_debuglevel(const struct process_id pid,
			  const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> debuglevel\n");
		return False;
	}

	/* Send a message and register our interest in a reply */

	if (!send_message(pid, MSG_REQ_DEBUGLEVEL, NULL, 0, False))
		return False;

	message_register(MSG_DEBUGLEVEL, print_pid_string_cb);

	wait_replies(procid_to_pid(&pid) == 0);

	/* No replies were received within the timeout period */

	if (num_replies == 0)
		printf("No replies received\n");

	message_deregister(MSG_DEBUGLEVEL);

	return num_replies;
}

/* Send a print notify message */

static BOOL do_printnotify(const struct process_id pid,
			   const int argc, const char **argv)
{
	const char *cmd;

	/* Check for subcommand */

	if (argc == 1) {
		fprintf(stderr, "Must specify subcommand:\n");
		fprintf(stderr, "\tqueuepause <printername>\n");
		fprintf(stderr, "\tqueueresume <printername>\n");
		fprintf(stderr, "\tjobpause <printername> <unix jobid>\n");
		fprintf(stderr, "\tjobresume <printername> <unix jobid>\n");
		fprintf(stderr, "\tjobdelete <printername> <unix jobid>\n");
		fprintf(stderr, "\tprinter <printername> <comment|port|"
			"driver> <value>\n");
		
		return False;
	}

	cmd = argv[1];

	if (strcmp(cmd, "queuepause") == 0) {

		if (argc != 3) {
			fprintf(stderr, "Usage: smbcontrol <dest> printnotify"
				" queuepause <printername>\n");
			return False;
		}
		
		notify_printer_status_byname(argv[2], PRINTER_STATUS_PAUSED);

		goto send;

	} else if (strcmp(cmd, "queueresume") == 0) {

		if (argc != 3) {
			fprintf(stderr, "Usage: smbcontrol <dest> printnotify"
				" queuereume <printername>\n");
			return False;
		}
		
		notify_printer_status_byname(argv[2], PRINTER_STATUS_OK);

		goto send;

	} else if (strcmp(cmd, "jobpause") == 0) {
		int jobid;

		if (argc != 4) {
			fprintf(stderr, "Usage: smbcontrol <dest> printnotify"
				" jobpause <printername> <unix-jobid>\n");
			return False;
		}

		jobid = atoi(argv[3]);

		notify_job_status_byname(
			argv[2], jobid, JOB_STATUS_PAUSED, 
			SPOOLSS_NOTIFY_MSG_UNIX_JOBID);

		goto send;

	} else if (strcmp(cmd, "jobresume") == 0) {
		int jobid;

		if (argc != 4) {
			fprintf(stderr, "Usage: smbcontrol <dest> printnotify"
				" jobpause <printername> <unix-jobid>\n");
			return False;
		}

		jobid = atoi(argv[3]);

		notify_job_status_byname(
			argv[2], jobid, JOB_STATUS_QUEUED, 
			SPOOLSS_NOTIFY_MSG_UNIX_JOBID);

		goto send;

	} else if (strcmp(cmd, "jobdelete") == 0) {
		int jobid;

		if (argc != 4) {
			fprintf(stderr, "Usage: smbcontrol <dest> printnotify"
				" jobpause <printername> <unix-jobid>\n");
			return False;
		}

		jobid = atoi(argv[3]);

		notify_job_status_byname(
			argv[2], jobid, JOB_STATUS_DELETING,
			SPOOLSS_NOTIFY_MSG_UNIX_JOBID);
		
		notify_job_status_byname(
			argv[2], jobid, JOB_STATUS_DELETING|
			JOB_STATUS_DELETED,
			SPOOLSS_NOTIFY_MSG_UNIX_JOBID);

		goto send;

	} else if (strcmp(cmd, "printer") == 0) {
		uint32 attribute;
		
		if (argc != 5) {
			fprintf(stderr, "Usage: smbcontrol <dest> printnotify "
				"printer <printername> <comment|port|driver> "
				"<value>\n");
			return False;
		}

		if (strcmp(argv[3], "comment") == 0) {
			attribute = PRINTER_NOTIFY_COMMENT;
		} else if (strcmp(argv[3], "port") == 0) {
			attribute = PRINTER_NOTIFY_PORT_NAME;
		} else if (strcmp(argv[3], "driver") == 0) {
			attribute = PRINTER_NOTIFY_DRIVER_NAME;
		} else {
			fprintf(stderr, "Invalid printer command '%s'\n",
				argv[3]);
			return False;
		}

		notify_printer_byname(argv[2], attribute,
				      CONST_DISCARD(char *, argv[4]));

		goto send;
	}

	fprintf(stderr, "Invalid subcommand '%s'\n", cmd);
	return False;

send:
	print_notify_send_messages(0);
	return True;
}

/* Close a share */

static BOOL do_closeshare(const struct process_id pid,
			  const int argc, const char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: smbcontrol <dest> close-share "
			"<sharename>\n");
		return False;
	}

	return send_message(
		pid, MSG_SMB_FORCE_TDIS, argv[1], strlen(argv[1]) + 1, False);
}

/* Force a SAM synchronisation */

static BOOL do_samsync(const struct process_id pid,
		       const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> samsync\n");
		return False;
	}

	return send_message(
		pid, MSG_SMB_SAM_SYNC, NULL, 0, False);
}

/* Force a SAM replication */

static BOOL do_samrepl(const struct process_id pid,
		       const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> samrepl\n");
		return False;
	}

	return send_message(
		pid, MSG_SMB_SAM_REPL, NULL, 0, False);
}

/* Display talloc pool usage */

static BOOL do_poolusage(const struct process_id pid,
			 const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> pool-usage\n");
		return False;
	}

	message_register(MSG_POOL_USAGE, print_string_cb);

	/* Send a message and register our interest in a reply */

	if (!send_message(pid, MSG_REQ_POOL_USAGE, NULL, 0, False))
		return False;

	wait_replies(procid_to_pid(&pid) == 0);

	/* No replies were received within the timeout period */

	if (num_replies == 0)
		printf("No replies received\n");

	message_deregister(MSG_POOL_USAGE);

	return num_replies;
}

/* Perform a dmalloc mark */

static BOOL do_dmalloc_mark(const struct process_id pid,
			    const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> dmalloc-mark\n");
		return False;
	}

	return send_message(
		pid, MSG_REQ_DMALLOC_MARK, NULL, 0, False);
}

/* Perform a dmalloc changed */

static BOOL do_dmalloc_changed(const struct process_id pid,
			       const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> "
			"dmalloc-log-changed\n");
		return False;
	}

	return send_message(
		pid, MSG_REQ_DMALLOC_LOG_CHANGED, NULL, 0, False);
}

/* Shutdown a server process */

static BOOL do_shutdown(const struct process_id pid,
			const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> shutdown\n");
		return False;
	}

	return send_message(pid, MSG_SHUTDOWN, NULL, 0, False);
}

/* Notify a driver upgrade */

static BOOL do_drvupgrade(const struct process_id pid,
			  const int argc, const char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: smbcontrol <dest> drvupgrade "
			"<driver-name>\n");
		return False;
	}

	return send_message(
		pid, MSG_DEBUG, argv[1], strlen(argv[1]) + 1, False);
}

static BOOL do_winbind_online(const struct process_id pid,
			     const int argc, const char **argv)
{
	TDB_CONTEXT *tdb;

	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol winbindd online\n");
		return False;
	}

	if (!lp_winbind_offline_logon()) {
		fprintf(stderr, "The parameter \"winbind offline logon\" must "
			"be set in the [global] section of smb.conf for this "
			"command to be allowed.\n");
		return False;
	}

	/* Remove the entry in the winbindd_cache tdb to tell a later
	   starting winbindd that we're online. */

	tdb = tdb_open_log(lock_path("winbindd_cache.tdb"), 0, TDB_DEFAULT, O_RDWR, 0600);
	if (!tdb) {
		fprintf(stderr, "Cannot open the tdb %s for writing.\n",
			lock_path("winbindd_cache.tdb"));
		return False;
	}

	tdb_delete_bystring(tdb, "WINBINDD_OFFLINE");
	tdb_close(tdb);

	return send_message(pid, MSG_WINBIND_ONLINE, NULL, 0, False);
}

static BOOL do_winbind_offline(const struct process_id pid,
			     const int argc, const char **argv)
{
	TDB_CONTEXT *tdb;
	BOOL ret = False;
	int retry = 0;

	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol winbindd offline\n");
		return False;
	}

	if (!lp_winbind_offline_logon()) {
		fprintf(stderr, "The parameter \"winbind offline logon\" must "
			"be set in the [global] section of smb.conf for this "
			"command to be allowed.\n");
		return False;
	}

	/* Create an entry in the winbindd_cache tdb to tell a later
	   starting winbindd that we're offline. We may actually create
	   it here... */

	tdb = tdb_open_log(lock_path("winbindd_cache.tdb"),
				WINBINDD_CACHE_TDB_DEFAULT_HASH_SIZE,
				TDB_DEFAULT /* TDB_CLEAR_IF_FIRST */, O_RDWR|O_CREAT, 0600);

	if (!tdb) {
		fprintf(stderr, "Cannot open the tdb %s for writing.\n",
			lock_path("winbindd_cache.tdb"));
		return False;
	}

	/* There's a potential race condition that if a child
	   winbindd detects a domain is online at the same time
	   we're trying to tell it to go offline that it might 
	   delete the record we add between us adding it and
	   sending the message. Minimize this by retrying up to
	   5 times. */

	for (retry = 0; retry < 5; retry++) {
		int err;
		TDB_DATA d;
		ZERO_STRUCT(d);
		tdb_store_bystring(tdb, "WINBINDD_OFFLINE", d, TDB_INSERT);

		ret = send_message(pid, MSG_WINBIND_OFFLINE, NULL, 0, False);

		/* Check that the entry "WINBINDD_OFFLINE" still exists. */
		tdb->ecode = TDB_SUCCESS;
		d = tdb_fetch_bystring( tdb, "WINBINDD_OFFLINE" );

		/* As this is a key with no data we don't need to free, we
		   check for existence by looking at tdb_err. */

		err = tdb_error(tdb);

		if (err == TDB_ERR_NOEXIST) {
			DEBUG(10,("do_winbind_offline: offline state not set - retrying.\n"));
		} else {
			break;
		}
	}

	tdb_close(tdb);
	return ret;
}

static BOOL do_winbind_onlinestatus(const struct process_id pid,
				    const int argc, const char **argv)
{
	struct process_id myid;

	myid = pid_to_procid(sys_getpid());

	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol winbindd onlinestatus\n");
		return False;
	}

	message_register(MSG_WINBIND_ONLINESTATUS, print_pid_string_cb);

	if (!send_message(pid, MSG_WINBIND_ONLINESTATUS, &myid, sizeof(myid), False))
		return False;

	wait_replies(procid_to_pid(&pid) == 0);

	/* No replies were received within the timeout period */

	if (num_replies == 0)
		printf("No replies received\n");

	message_deregister(MSG_WINBIND_ONLINESTATUS);

	return num_replies;
}


static BOOL do_reload_config(const struct process_id pid,
			     const int argc, const char **argv)
{
	if (argc != 1) {
		fprintf(stderr, "Usage: smbcontrol <dest> reload-config\n");
		return False;
	}

	return send_message(pid, MSG_SMB_CONF_UPDATED, NULL, 0, False);
}

static void my_make_nmb_name( struct nmb_name *n, const char *name, int type)
{
	fstring unix_name;
	memset( (char *)n, '\0', sizeof(struct nmb_name) );
	fstrcpy(unix_name, name);
	strupper_m(unix_name);
	push_ascii(n->name, unix_name, sizeof(n->name), STR_TERMINATE);
	n->name_type = (unsigned int)type & 0xFF;
	push_ascii(n->scope,  global_scope(), 64, STR_TERMINATE);
}

static BOOL do_nodestatus(const struct process_id pid,
			  const int argc, const char **argv)
{
	struct packet_struct p;

	if (argc != 2) {
		fprintf(stderr, "Usage: smbcontrol nmbd nodestatus <ip>\n");
		return False;
	}

	ZERO_STRUCT(p);

	p.ip = *interpret_addr2(argv[1]);
	p.port = 137;
	p.packet_type = NMB_PACKET;

	p.packet.nmb.header.name_trn_id = 10;
	p.packet.nmb.header.opcode = 0;
	p.packet.nmb.header.response = False;
	p.packet.nmb.header.nm_flags.bcast = False;
	p.packet.nmb.header.nm_flags.recursion_available = False;
	p.packet.nmb.header.nm_flags.recursion_desired = False;
	p.packet.nmb.header.nm_flags.trunc = False;
	p.packet.nmb.header.nm_flags.authoritative = False;
	p.packet.nmb.header.rcode = 0;
	p.packet.nmb.header.qdcount = 1;
	p.packet.nmb.header.ancount = 0;
	p.packet.nmb.header.nscount = 0;
	p.packet.nmb.header.arcount = 0;
	my_make_nmb_name(&p.packet.nmb.question.question_name, "*", 0x00);
	p.packet.nmb.question.question_type = 0x21;
	p.packet.nmb.question.question_class = 0x1;

	return send_message(pid, MSG_SEND_PACKET, &p, sizeof(p), False);
}

/* A list of message type supported */

static const struct {
	const char *name;	/* Option name */
	BOOL (*fn)(const struct process_id pid,
		   const int argc, const char **argv);
	const char *help;	/* Short help text */
} msg_types[] = {
	{ "debug", do_debug, "Set debuglevel"  },
	{ "force-election", do_election,
	  "Force a browse election" },
	{ "ping", do_ping, "Elicit a response" },
	{ "profile", do_profile, "" },
	{ "inject", do_inject_fault,
	    "Inject a fatal signal into a running smbd"},
	{ "stacktrace", do_daemon_stack_trace,
	    "Display a stack trace of a daemon" },
	{ "profilelevel", do_profilelevel, "" },
	{ "debuglevel", do_debuglevel, "Display current debuglevels" },
	{ "printnotify", do_printnotify, "Send a print notify message" },
	{ "close-share", do_closeshare, "Forcibly disconnect a share" },
        { "samsync", do_samsync, "Initiate SAM synchronisation" },
        { "samrepl", do_samrepl, "Initiate SAM replication" },
	{ "pool-usage", do_poolusage, "Display talloc memory usage" },
	{ "dmalloc-mark", do_dmalloc_mark, "" },
	{ "dmalloc-log-changed", do_dmalloc_changed, "" },
	{ "shutdown", do_shutdown, "Shut down daemon" },
	{ "drvupgrade", do_drvupgrade, "Notify a printer driver has changed" },
	{ "reload-config", do_reload_config, "Force smbd or winbindd to reload config file"},
	{ "nodestatus", do_nodestatus, "Ask nmbd to do a node status request"},
	{ "online", do_winbind_online, "Ask winbind to go into online state"},
	{ "offline", do_winbind_offline, "Ask winbind to go into offline state"},
	{ "onlinestatus", do_winbind_onlinestatus, "Request winbind online status"},
	{ "noop", do_noop, "Do nothing" },
	{ NULL }
};

/* Display usage information */

static void usage(poptContext *pc)
{
	int i;

	poptPrintHelp(*pc, stderr, 0);

	fprintf(stderr, "\n");
	fprintf(stderr, "<destination> is one of \"nmbd\", \"smbd\", \"winbindd\" or a "
		"process ID\n");

	fprintf(stderr, "\n");
	fprintf(stderr, "<message-type> is one of:\n");

	for (i = 0; msg_types[i].name; i++) 
	    fprintf(stderr, "\t%-30s%s\n", msg_types[i].name, 
		    msg_types[i].help);

	fprintf(stderr, "\n");

	exit(1);
}

/* Return the pid number for a string destination */

static struct process_id parse_dest(const char *dest)
{
	struct process_id result = {-1};
	pid_t pid;

	/* Zero is a special return value for broadcast smbd */

	if (strequal(dest, "smbd")) {
		return interpret_pid("0");
	}

	/* Try self - useful for testing */

	if (strequal(dest, "self")) {
		return pid_to_procid(sys_getpid());
	}

	/* Fix winbind typo. */
	if (strequal(dest, "winbind")) {
		dest = "winbindd";
	}

	
	if (!(strequal(dest, "winbindd") || strequal(dest, "nmbd"))) {
		/* Check for numeric pid number */

		result = interpret_pid(dest);

		/* Zero isn't valid if not smbd. */
		if (result.pid && procid_valid(&result)) {
			return result;
		}
	}

	/* Look up other destinations in pidfile directory */

	if ((pid = pidfile_pid(dest)) != 0) {
		return pid_to_procid(pid);
	}

	fprintf(stderr,"Can't find pid for destination '%s'\n", dest);

	return result;
}	

/* Execute smbcontrol command */

static BOOL do_command(int argc, const char **argv)
{
	const char *dest = argv[0], *command = argv[1];
	struct process_id pid;
	int i;

	/* Check destination */

	pid = parse_dest(dest);
	if (!procid_valid(&pid)) {
		return False;
	}

	/* Check command */

	for (i = 0; msg_types[i].name; i++) {
		if (strequal(command, msg_types[i].name))
			return msg_types[i].fn(pid, argc - 1, argv + 1);
	}

	fprintf(stderr, "smbcontrol: unknown command '%s'\n", command);

	return False;
}

/* Main program */

int main(int argc, const char **argv)
{
	poptContext pc;
	int opt;

	static struct poptOption wbinfo_options[] = {
		{ "timeout", 't', POPT_ARG_INT, &timeout, 't', 
		  "Set timeout value in seconds", "TIMEOUT" },

		{ "configfile", 's', POPT_ARG_STRING, NULL, 's', 
		  "Use alternative configuration file", "CONFIGFILE" },

		POPT_TABLEEND
	};

	struct poptOption options[] = {
		{ NULL, 0, POPT_ARG_INCLUDE_TABLE, wbinfo_options, 0, 
		  "Options" },

		POPT_AUTOHELP
		POPT_COMMON_VERSION
		POPT_TABLEEND
	};

	load_case_tables();

	setup_logging(argv[0],True);
	
	/* Parse command line arguments using popt */

	pc = poptGetContext(
		"smbcontrol", argc, (const char **)argv, options, 0);

	poptSetOtherOptionHelp(pc, "[OPTION...] <destination> <message-type> "
			       "<parameters>");

	if (argc == 1)
		usage(&pc);

	while ((opt = poptGetNextOpt(pc)) != -1) {
		switch(opt) {
		case 't':	/* --timeout */
			argc -= 2;
			break;
		case 's':	/* --configfile */
			pstrcpy(dyn_CONFIGFILE, poptGetOptArg(pc));
			argc -= 2;
			break;
		default:
			fprintf(stderr, "Invalid option\n");
			poptPrintHelp(pc, stderr, 0);
			break;
		}
	}

	/* We should now have the remaining command line arguments in
           argv.  The argc parameter should have been decremented to the
           correct value in the above switch statement. */

	argv = (const char **)poptGetArgs(pc);
	argc--;			/* Don't forget about argv[0] */

	if (argc == 1)
		usage(&pc);

	lp_load(dyn_CONFIGFILE,False,False,False,True);

	/* Need to invert sense of return code -- samba
         * routines mostly return True==1 for success, but
         * shell needs 0. */ 
	
	return !do_command(argc, argv);
}

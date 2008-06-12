/*
 * FAM file notification support.
 *
 * Copyright (c) James Peach 2005
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include "includes.h"

#ifdef HAVE_FAM_CHANGE_NOTIFY

#include <fam.h>

#if !defined(HAVE_FAM_H_FAMCODES_TYPEDEF)
/* Gamin provides this typedef which means we can't use 'enum FAMCodes' as per
 * every other FAM implementation. Phooey.
 */
typedef enum FAMCodes FAMCodes;
#endif

/* NOTE: There are multiple versions of FAM floating around the net, each with
 * slight differences from the original SGI FAM implementation. In this file,
 * we rely only on the SGI features and do not assume any extensions. For
 * example, we do not look at FAMErrno, because it is not set by the original
 * implementation.
 *
 * Random FAM links:
 *	http://oss.sgi.com/projects/fam/
 *	http://savannah.nongnu.org/projects/fam/
 *	http://sourceforge.net/projects/bsdfam/
 */

struct fam_req_info
{
    FAMRequest	    req;
    int		    generation;
    FAMCodes	    code;
    enum
    {
	/* We are waiting for an event. */
	FAM_REQ_MONITORING,
	/* An event has been receive, but we haven't been able to send it back
	 * to the client yet. It is stashed in the code member.
	 */
	FAM_REQ_FIRED
    } state;
};

/* Don't initialise this until the first register request. We want a single
 * FAM connection for each worker smbd. If we allow the master (parent) smbd to
 * open a FAM connection, multiple processes talking on the same socket will
 * undoubtedly create havoc.
 */
static FAMConnection	global_fc;
static int		global_fc_generation;

#define FAM_TRACE	8
#define FAM_TRACE_LOW	10

#define FAM_EVENT_DRAIN		    ((uint32_t)(-1))

static void * fam_register_notify(connection_struct * conn,
		    char *		path,
		    uint32		flags);

static BOOL fam_check_notify(connection_struct * conn,
		 uint16_t		vuid,
		 char *			path,
		 uint32_t		flags,
		 void *			data,
		 time_t			when);

static void fam_remove_notify(void * data);

static struct cnotify_fns global_fam_notify =
{
    fam_register_notify,
    fam_check_notify,
    fam_remove_notify,
    -1,
    -1
};

/* Turn a FAM event code into a string. Don't rely on specific code values,
 * because that might not work across all flavours of FAM.
 */
static const char *
fam_event_str(FAMCodes code)
{
    static const struct { FAMCodes code; const char * name; } evstr[] =
    {
	{ FAMChanged,		"FAMChanged"},
	{ FAMDeleted,		"FAMDeleted"},
	{ FAMStartExecuting,	"FAMStartExecuting"},
	{ FAMStopExecuting,	"FAMStopExecuting"},
	{ FAMCreated,		"FAMCreated"},
	{ FAMMoved,		"FAMMoved"},
	{ FAMAcknowledge,	"FAMAcknowledge"},
	{ FAMExists,		"FAMExists"},
	{ FAMEndExist,		"FAMEndExist"}
    };

    int i;

    for (i = 0; i < ARRAY_SIZE(evstr); ++i) {
	if (code == evstr[i].code)
	    return(evstr[i].name);
    }

    return("<unknown>");
}

static BOOL
fam_check_reconnect(void)
{
    if (FAMCONNECTION_GETFD(&global_fc) < 0) {
	fstring name;

	global_fc_generation++;
	snprintf(name, sizeof(name), "smbd (%lu)", (unsigned long)sys_getpid());

	if (FAMOpen2(&global_fc, name) < 0) {
	    DEBUG(0, ("failed to connect to FAM service\n"));
	    return(False);
	}
    }

    global_fam_notify.notification_fd = FAMCONNECTION_GETFD(&global_fc);
    return(True);
}

static BOOL
fam_monitor_path(connection_struct *	conn,
	         struct fam_req_info *	info,
		 const char *		path,
		 uint32			flags)
{
    SMB_STRUCT_STAT st;
    pstring	    fullpath;

    DEBUG(FAM_TRACE, ("requesting FAM notifications for '%s'\n", path));

    /* FAM needs an absolute pathname. */

    /* It would be better to use reduce_name() here, but reduce_name does not
     * actually return the reduced result. How utterly un-useful.
     */
    pstrcpy(fullpath, path);
    if (!canonicalize_path(conn, fullpath)) {
	DEBUG(0, ("failed to canonicalize path '%s'\n", path));
	return(False);
    }

    if (*fullpath != '/') {
	DEBUG(0, ("canonicalized path '%s' into `%s`\n", path, fullpath));
	DEBUGADD(0, ("but expected an absolute path\n"));
	return(False);
    }

    if (SMB_VFS_STAT(conn, path, &st) < 0) {
	DEBUG(0, ("stat of '%s' failed: %s\n", path, strerror(errno)));
	return(False);
    }
    /* Start monitoring this file or directory. We hand the state structure to
     * both the caller and the FAM library so we can match up the caller's
     * status requests with FAM notifications.
     */
    if (S_ISDIR(st.st_mode)) {
	FAMMonitorDirectory(&global_fc, fullpath, &(info->req), info);
    } else {
	FAMMonitorFile(&global_fc, fullpath, &(info->req), info);
    }

    /* Grr. On IRIX, neither of the monitor functions return a status. */

    /* We will stay in initialising state until we see the FAMendExist message
     * for this file.
     */
    info->state = FAM_REQ_MONITORING;
    info->generation = global_fc_generation;
    return(True);
}

static BOOL
fam_handle_event(const FAMCodes code, uint32 flags)
{
#define F_CHANGE_MASK (FILE_NOTIFY_CHANGE_FILE | \
			FILE_NOTIFY_CHANGE_ATTRIBUTES | \
			FILE_NOTIFY_CHANGE_SIZE | \
			FILE_NOTIFY_CHANGE_LAST_WRITE | \
			FILE_NOTIFY_CHANGE_LAST_ACCESS | \
			FILE_NOTIFY_CHANGE_CREATION | \
			FILE_NOTIFY_CHANGE_EA | \
			FILE_NOTIFY_CHANGE_SECURITY)

#define F_DELETE_MASK (FILE_NOTIFY_CHANGE_FILE_NAME | \
			FILE_NOTIFY_CHANGE_DIR_NAME)

#define F_CREATE_MASK (FILE_NOTIFY_CHANGE_FILE_NAME | \
			FILE_NOTIFY_CHANGE_DIR_NAME)

    switch (code) {
	case FAMChanged:
	    if (flags & F_CHANGE_MASK)
		return(True);
	    break;
	case FAMDeleted:
	    if (flags & F_DELETE_MASK)
		return(True);
	    break;
	case FAMCreated:
	    if (flags & F_CREATE_MASK)
		return(True);
	    break;
	default:
	    /* Ignore anything else. */
	    break;
    }

    return(False);

#undef F_CHANGE_MASK
#undef F_DELETE_MASK
#undef F_CREATE_MASK
}

static BOOL
fam_pump_events(struct fam_req_info * info, uint32_t flags)
{
    FAMEvent ev;

    for (;;) {

	/* If we are draining the event queue we must keep going until we find
	 * the correct FAMAcknowledge event or the connection drops. Otherwise
	 * we should stop when there are no more events pending.
	 */
	if (flags != FAM_EVENT_DRAIN && !FAMPending(&global_fc)) {
	    break;
	}

	if (FAMNextEvent(&global_fc, &ev) < 0) {
	    DEBUG(0, ("failed to fetch pending FAM event\n"));
	    DEBUGADD(0, ("resetting FAM connection\n"));
	    FAMClose(&global_fc);
	    FAMCONNECTION_GETFD(&global_fc) = -1;
	    return(False);
	}

	DEBUG(FAM_TRACE_LOW, ("FAM event %s on '%s' for request %d\n",
		    fam_event_str(ev.code), ev.filename, ev.fr.reqnum));

	switch (ev.code) {
	    case FAMAcknowledge:
		/* FAM generates an ACK event when we cancel a monitor. We need
		 * this to know when it is safe to free out request state
		 * structure.
		 */
		if (info->generation == global_fc_generation &&
		    info->req.reqnum == ev.fr.reqnum &&
		    flags == FAM_EVENT_DRAIN) {
		    return(True);
		}

	    case FAMEndExist:
	    case FAMExists:
		/* Ignore these. FAM sends these enumeration events when we
		 * start monitoring. If we are monitoring a directory, we will
		 * get a FAMExists event for each directory entry.
		 */

		/* TODO: we might be able to use these to implement recursive
		 * monitoring of entire subtrees.
		 */
	    case FAMMoved:
		/* These events never happen. A move or rename shows up as a
		 * create/delete pair.
		 */
	    case FAMStartExecuting:
	    case FAMStopExecuting:
		/* We might get these, but we just don't care. */
		break;

	    case FAMChanged:
	    case FAMDeleted:
	    case FAMCreated:
		if (info->generation != global_fc_generation) {
		    /* Ignore this; the req number can't be matched. */
		    break;
		}

		if (info->req.reqnum == ev.fr.reqnum) {
		    /* This is the event the caller was interested in. */
		    DEBUG(FAM_TRACE, ("handling FAM %s event on '%s'\n",
				fam_event_str(ev.code), ev.filename));
		    /* Ignore events if we are draining this request. */
		    if (flags != FAM_EVENT_DRAIN) {
			return(fam_handle_event(ev.code, flags));
		    }
		    break;
		} else {
		    /* Caller doesn't want this event. Stash the result so we
		     * can come back to it. Unfortunately, FAM doesn't
		     * guarantee to give us back evinfo.
		     */
		    struct fam_req_info * evinfo =
			(struct fam_req_info *)ev.userdata;

		    if (evinfo) {
			DEBUG(FAM_TRACE, ("storing FAM %s event for winter\n",
				    fam_event_str(ev.code)));
			evinfo->state = FAM_REQ_FIRED;
			evinfo->code = ev.code;
		    } else {
			DEBUG(2, ("received FAM %s notification for %s, "
				  "but userdata was unexpectedly NULL\n",
				  fam_event_str(ev.code), ev.filename));
		    }
		    break;
		}

	    default:
		DEBUG(0, ("ignoring unknown FAM event code %d for `%s`\n",
			    ev.code, ev.filename));
	}
    }

    /* No more notifications pending. */
    return(False);
}

static BOOL
fam_test_connection(void)
{
    FAMConnection fc;

    /* On IRIX FAMOpen2 leaks 960 bytes in 48 blocks. It's a deliberate leak
     * in the library and there's nothing we can do about it here.
     */
    if (FAMOpen2(&fc, "smbd probe") < 0)
	return(False);

    FAMClose(&fc);
    return(True);
}

/* ------------------------------------------------------------------------- */

static void *
fam_register_notify(connection_struct * conn,
		    char *		path,
		    uint32		flags)
{
    struct fam_req_info * info;

    if (!fam_check_reconnect()) {
	return(False);
    }

    if ((info = SMB_MALLOC_P(struct fam_req_info)) == NULL) {
	DEBUG(0, ("malloc of %u bytes failed\n", (unsigned int)sizeof(struct fam_req_info)));
	return(NULL);
    }

    if (fam_monitor_path(conn, info, path, flags)) {
	return(info);
    } else {
	SAFE_FREE(info);
	return(NULL);
    }
}

static BOOL
fam_check_notify(connection_struct *	conn,
		 uint16_t		vuid,
		 char *			path,
		 uint32_t		flags,
		 void *			data,
		 time_t			when)
{
    struct fam_req_info * info;

    info = (struct fam_req_info *)data;
    SMB_ASSERT(info != NULL);

    DEBUG(10, ("checking FAM events for `%s`\n", path));

    if (info->state == FAM_REQ_FIRED) {
	DEBUG(FAM_TRACE, ("handling previously fired FAM %s event\n",
		    fam_event_str(info->code)));
	info->state = FAM_REQ_MONITORING;
	return(fam_handle_event(info->code, flags));
    }

    if (!fam_check_reconnect()) {
	return(False);
    }

    if (info->generation != global_fc_generation) {
	DEBUG(FAM_TRACE, ("reapplying stale FAM monitor to %s\n", path));
	fam_monitor_path(conn, info, path, flags);
	return(False);
    }

    return(fam_pump_events(info, flags));
}

static void
fam_remove_notify(void * data)
{
    struct fam_req_info * info;

    if ((info = (struct fam_req_info *)data) == NULL)
	return;

    /* No need to reconnect. If the FAM connection is gone, there's no need to
     * cancel and we can safely let FAMCancelMonitor fail. If it we
     * reconnected, then the generation check will stop us cancelling the wrong
     * request.
     */

    if (info->generation == global_fc_generation) {
	DEBUG(FAM_TRACE, ("removing FAM notification for request %d\n",
		    info->req.reqnum));
	FAMCancelMonitor(&global_fc, &(info->req));

	/* Soak up all events until the FAMAcknowledge. We can't free
	 * our request state until we are sure there are no more events in
	 * flight.
	 */
	fam_pump_events(info, FAM_EVENT_DRAIN);
    }

    SAFE_FREE(info);
}

struct cnotify_fns * fam_notify_init(void)
{
    FAMCONNECTION_GETFD(&global_fc) = -1;

    if (!fam_test_connection()) {
	DEBUG(0, ("FAM file change notifications not available\n"));
	return(NULL);
    }

    DEBUG(FAM_TRACE, ("enabling FAM change notifications\n"));
    return &global_fam_notify;
}

#endif /* HAVE_FAM_CHANGE_NOTIFY */

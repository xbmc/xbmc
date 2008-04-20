/* 
   Unix SMB/CIFS implementation.
   DMAPI Support routines

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

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_DMAPI

#ifndef USE_DMAPI

int dmapi_init_session(void) { return -1; }
uint32 dmapi_file_flags(const char * const path) { return 0; }
BOOL dmapi_have_session(void) { return False; }

#else /* USE_DMAPI */

#ifdef HAVE_XFS_DMAPI_H
#include <xfs/dmapi.h>
#elif defined(HAVE_SYS_DMI_H)
#include <sys/dmi.h>
#elif defined(HAVE_SYS_JFSDMAPI_H)
#include <sys/jfsdmapi.h>
#elif defined(HAVE_SYS_DMAPI_H)
#include <sys/dmapi.h>
#endif

#define DMAPI_SESSION_NAME "samba"
#define DMAPI_TRACE 10

static dm_sessid_t dmapi_session = DM_NO_SESSION;

/* Initialise the DMAPI interface. Make sure that we only end up initialising
 * once per process to avoid resource leaks across different DMAPI
 * implementations.
 */
static int init_dmapi_service(void)
{
	static pid_t lastpid;

	pid_t mypid;

	mypid = sys_getpid();
	if (mypid != lastpid) {
		char *version;

		lastpid = mypid;
		if (dm_init_service(&version) < 0) {
			return -1;
		}

		DEBUG(0, ("Initializing DMAPI: %s\n", version));
	}

	return 0;
}

BOOL dmapi_have_session(void)
{
	return dmapi_session != DM_NO_SESSION;
}

static dm_sessid_t *realloc_session_list(dm_sessid_t * sessions, int count)
{
	dm_sessid_t *nsessions;

	nsessions = TALLOC_REALLOC_ARRAY(NULL, sessions, dm_sessid_t, count);
	if (nsessions == NULL) {
		TALLOC_FREE(sessions);
		return NULL;
	}

	return nsessions;
}

/* Initialise DMAPI session. The session is persistant kernel state, so it
 * might already exist, in which case we merely want to reconnect to it. This
 * function should be called as root.
 */
int dmapi_init_session(void)
{
	char	buf[DM_SESSION_INFO_LEN];
	size_t	buflen;

	uint	    nsessions = 10;
	dm_sessid_t *sessions = NULL;

	int i, err;

	/* If we aren't root, something in the following will fail due to lack
	 * of privileges. Aborting seems a little extreme.
	 */
	SMB_WARN(getuid() == 0, "dmapi_init_session must be called as root");

	dmapi_session = DM_NO_SESSION;
	if (init_dmapi_service() < 0) {
		return -1;
	}

retry:

	if ((sessions = realloc_session_list(sessions, nsessions)) == NULL) {
		return -1;
	}

	err = dm_getall_sessions(nsessions, sessions, &nsessions);
	if (err < 0) {
		if (errno == E2BIG) {
			nsessions *= 2;
			goto retry;
		}

		DEBUGADD(DMAPI_TRACE,
			("failed to retrieve DMAPI sessions: %s\n",
			strerror(errno)));
		TALLOC_FREE(sessions);
		return -1;
	}

	for (i = 0; i < nsessions; ++i) {
		err = dm_query_session(sessions[i], sizeof(buf), buf, &buflen);
		buf[sizeof(buf) - 1] = '\0';
		if (err == 0 && strcmp(DMAPI_SESSION_NAME, buf) == 0) {
			dmapi_session = sessions[i];
			DEBUGADD(DMAPI_TRACE,
				("attached to existing DMAPI session "
				 "named '%s'\n", buf));
			break;
		}
	}

	TALLOC_FREE(sessions);

	/* No session already defined. */
	if (dmapi_session == DM_NO_SESSION) {
		err = dm_create_session(DM_NO_SESSION, DMAPI_SESSION_NAME,
					&dmapi_session);
		if (err < 0) {
			DEBUGADD(DMAPI_TRACE,
				("failed to create new DMAPI session: %s\n",
				strerror(errno)));
			dmapi_session = DM_NO_SESSION;
			return -1;
		}

		DEBUGADD(DMAPI_TRACE,
			("created new DMAPI session named '%s'\n",
			DMAPI_SESSION_NAME));
	}

	/* Note that we never end the DMAPI session. This enables child
	 * processes to continue to use the session after we exit. It also lets
	 * you run a second Samba server on different ports without any
	 * conflict.
	 */

	return 0;
}

/* Reattach to an existing dmapi session. Called from service processes that
 * might not be running as root.
 */
static int reattach_dmapi_session(void)
{
	char	buf[DM_SESSION_INFO_LEN];
	size_t	buflen;

	if (dmapi_session != DM_NO_SESSION ) {
		become_root();

		/* NOTE: On Linux, this call opens /dev/dmapi, costing us a
		 * file descriptor. Ideally, we would close this when we fork.
		 */
		if (init_dmapi_service() < 0) {
			dmapi_session = DM_NO_SESSION;
			unbecome_root();
			return -1;
		}

		if (dm_query_session(dmapi_session, sizeof(buf),
			    buf, &buflen) < 0) {
			/* Session is stale. Disable DMAPI. */
			dmapi_session = DM_NO_SESSION;
			unbecome_root();
			return -1;
		}

		set_effective_capability(DMAPI_ACCESS_CAPABILITY);

		DEBUG(DMAPI_TRACE, ("reattached DMAPI session\n"));
		unbecome_root();
	}

	return 0;
}

uint32 dmapi_file_flags(const char * const path)
{
	static int attached = 0;

	int		err;
	dm_eventset_t   events = {0};
	uint		nevents;

	void	*dm_handle;
	size_t	dm_handle_len;

	uint32	flags = 0;

	/* If a DMAPI session has been initialised, then we need to make sure
	 * we are attached to it and have the correct privileges. This is
	 * necessary to be able to do DMAPI operations across a fork(2). If
	 * it fails, there is no liklihood of that failure being transient.
	 *
	 * Note that this use of the static attached flag relies on the fact
	 * that dmapi_file_flags() is never called prior to forking the
	 * per-client server process.
	 */
	if (dmapi_have_session() && !attached) {
		attached++;
		if (reattach_dmapi_session() < 0) {
			return 0;
		}
	}

	err = dm_path_to_handle(CONST_DISCARD(char *, path),
		&dm_handle, &dm_handle_len);
	if (err < 0) {
		DEBUG(DMAPI_TRACE, ("dm_path_to_handle(%s): %s\n",
			    path, strerror(errno)));

		if (errno != EPERM) {
			return 0;
		}

		/* Linux capabilities are broken in that changing our
		 * user ID will clobber out effective capabilities irrespective
		 * of whether we have set PR_SET_KEEPCAPS. Fortunately, the
		 * capabilities are not removed from our permitted set, so we
		 * can re-acquire them if necessary.
		 */

		set_effective_capability(DMAPI_ACCESS_CAPABILITY);

		err = dm_path_to_handle(CONST_DISCARD(char *, path),
			&dm_handle, &dm_handle_len);
		if (err < 0) {
			DEBUG(DMAPI_TRACE,
			    ("retrying dm_path_to_handle(%s): %s\n",
			    path, strerror(errno)));
			return 0;
		}
	}

	err = dm_get_eventlist(dmapi_session, dm_handle, dm_handle_len,
		DM_NO_TOKEN, DM_EVENT_MAX, &events, &nevents);
	if (err < 0) {
		DEBUG(DMAPI_TRACE, ("dm_get_eventlist(%s): %s\n",
			    path, strerror(errno)));
		dm_handle_free(dm_handle, dm_handle_len);
		return 0;
	}

	/* We figure that the only reason a DMAPI application would be
	 * interested in trapping read events is that part of the file is
	 * offline.
	 */
	DEBUG(DMAPI_TRACE, ("DMAPI event list for %s is %#llx\n",
		    path, events));
	if (DMEV_ISSET(DM_EVENT_READ, events)) {
		flags = FILE_ATTRIBUTE_OFFLINE;
	}

	dm_handle_free(dm_handle, dm_handle_len);

	if (flags & FILE_ATTRIBUTE_OFFLINE) {
		DEBUG(DMAPI_TRACE, ("%s is OFFLINE\n", path));
	}

	return flags;
}

#endif /* USE_DMAPI */

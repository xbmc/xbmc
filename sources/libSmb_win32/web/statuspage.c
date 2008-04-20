/* 
   Unix SMB/CIFS implementation.
   web status page
   Copyright (C) Andrew Tridgell 1997-1998
   
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
#include "web/swat_proto.h"

#define PIDMAP		struct PidMap

/* how long to wait for start/stops to take effect */
#define SLEEP_TIME 3

PIDMAP {
	PIDMAP	*next, *prev;
	struct process_id pid;
	char	*machine;
};

static PIDMAP	*pidmap;
static int	PID_or_Machine;		/* 0 = show PID, else show Machine name */

static struct process_id smbd_pid;

/* from 2nd call on, remove old list */
static void initPid2Machine (void)
{
	/* show machine name rather PID on table "Open Files"? */
	if (PID_or_Machine) {
		PIDMAP *p;

		for (p = pidmap; p != NULL; ) {
			DLIST_REMOVE(pidmap, p);
			SAFE_FREE(p->machine);
			SAFE_FREE(p);
		}

		pidmap = NULL;
	}
}

/* add new PID <-> Machine name mapping */
static void addPid2Machine (struct process_id pid, char *machine)
{
	/* show machine name rather PID on table "Open Files"? */
	if (PID_or_Machine) {
		PIDMAP *newmap;

		if ((newmap = SMB_MALLOC_P(PIDMAP)) == NULL) {
			/* XXX need error message for this?
			   if malloc fails, PID is always shown */
			return;
		}

		newmap->pid = pid;
		newmap->machine = SMB_STRDUP(machine);

		DLIST_ADD(pidmap, newmap);
	}
}

/* lookup PID <-> Machine name mapping */
static char *mapPid2Machine (struct process_id pid)
{
	static char pidbuf [64];
	PIDMAP *map;

	/* show machine name rather PID on table "Open Files"? */
	if (PID_or_Machine) {
		for (map = pidmap; map != NULL; map = map->next) {
			if (procid_equal(&pid, &map->pid)) {
				if (map->machine == NULL)	/* no machine name */
					break;			/* show PID */

				return map->machine;
			}
		}
	}

	/* PID not in list or machine name NULL? return pid as string */
	snprintf (pidbuf, sizeof (pidbuf) - 1, "%s",
		  procid_str_static(&pid));
	return pidbuf;
}

static char *tstring(time_t t)
{
	static pstring buf;
	pstrcpy(buf, time_to_asc(&t));
	all_string_sub(buf," ","&nbsp;",sizeof(buf));
	return buf;
}

static void print_share_mode(const struct share_mode_entry *e, const char *sharepath, const char *fname)
{
	char           *utf8_fname;
	int deny_mode;

	if (!is_valid_share_mode_entry(e)) {
		return;
	}

	deny_mode = map_share_mode_to_deny_mode(e->share_access,
						    e->private_options);

	printf("<tr><td>%s</td>",_(mapPid2Machine(e->pid)));
	printf("<td>%u</td>",(unsigned int)e->uid);
	printf("<td>");
	switch ((deny_mode>>4)&0xF) {
	case DENY_NONE: printf("DENY_NONE"); break;
	case DENY_ALL:  printf("DENY_ALL   "); break;
	case DENY_DOS:  printf("DENY_DOS   "); break;
	case DENY_FCB:  printf("DENY_FCB   "); break;
	case DENY_READ: printf("DENY_READ  "); break;
	case DENY_WRITE:printf("DENY_WRITE "); break;
	}
	printf("</td>");

	printf("<td>");
	if (e->access_mask & (FILE_READ_DATA|FILE_WRITE_DATA)) {
		printf("%s", _("RDWR       "));
	} else if (e->access_mask & FILE_WRITE_DATA) {
		printf("%s", _("WRONLY     "));
	} else {
		printf("%s", _("RDONLY     "));
	}
	printf("</td>");

	printf("<td>");
	if((e->op_type & 
	    (EXCLUSIVE_OPLOCK|BATCH_OPLOCK)) == 
	   (EXCLUSIVE_OPLOCK|BATCH_OPLOCK))
		printf("EXCLUSIVE+BATCH ");
	else if (e->op_type & EXCLUSIVE_OPLOCK)
		printf("EXCLUSIVE       ");
	else if (e->op_type & BATCH_OPLOCK)
		printf("BATCH           ");
	else if (e->op_type & LEVEL_II_OPLOCK)
		printf("LEVEL_II        ");
	else
		printf("NONE            ");
	printf("</td>");

	push_utf8_allocate(&utf8_fname, fname);
	printf("<td>%s</td><td>%s</td></tr>\n",
	       utf8_fname,tstring(e->time.tv_sec));
	SAFE_FREE(utf8_fname);
}


/* kill off any connections chosen by the user */
static int traverse_fn1(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void* state)
{
	struct connections_data crec;

	if (dbuf.dsize != sizeof(crec))
		return 0;

	memcpy(&crec, dbuf.dptr, sizeof(crec));

	if (crec.cnum == -1 && process_exists(crec.pid)) {
		char buf[30];
		slprintf(buf,sizeof(buf)-1,"kill_%s", procid_str_static(&crec.pid));
		if (cgi_variable(buf)) {
			kill_pid(crec.pid);
			sleep(SLEEP_TIME);
		}
	}
	return 0;
}

/* traversal fn for showing machine connections */
static int traverse_fn2(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void* state)
{
	struct connections_data crec;

	if (dbuf.dsize != sizeof(crec))
		return 0;

	memcpy(&crec, dbuf.dptr, sizeof(crec));
	
	if (crec.cnum == -1 || !process_exists(crec.pid) ||
	    procid_equal(&crec.pid, &smbd_pid))
		return 0;

	addPid2Machine (crec.pid, crec.machine);

	printf("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td>\n",
	       procid_str_static(&crec.pid),
	       crec.machine,crec.addr,
	       tstring(crec.start));
	if (geteuid() == 0) {
		printf("<td><input type=submit value=\"X\" name=\"kill_%s\"></td>\n",
		       procid_str_static(&crec.pid));
	}
	printf("</tr>\n");

	return 0;
}

/* traversal fn for showing share connections */
static int traverse_fn3(TDB_CONTEXT *tdb, TDB_DATA kbuf, TDB_DATA dbuf, void* state)
{
	struct connections_data crec;

	if (dbuf.dsize != sizeof(crec))
		return 0;

	memcpy(&crec, dbuf.dptr, sizeof(crec));

	if (crec.cnum == -1 || !process_exists(crec.pid))
		return 0;

	printf("<tr><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td><td>%s</td></tr>\n",
	       crec.name,uidtoname(crec.uid),
	       gidtoname(crec.gid),procid_str_static(&crec.pid),
	       crec.machine,
	       tstring(crec.start));
	return 0;
}


/* show the current server status */
void status_page(void)
{
	const char *v;
	int autorefresh=0;
	int refresh_interval=30;
	TDB_CONTEXT *tdb;
	int nr_running=0;
	BOOL waitup = False;

	smbd_pid = pid_to_procid(pidfile_pid("smbd"));

	if (cgi_variable("smbd_restart") || cgi_variable("all_restart")) {
		stop_smbd();
		start_smbd();
		waitup=True;
	}

	if (cgi_variable("smbd_start") || cgi_variable("all_start")) {
		start_smbd();
		waitup=True;
	}

	if (cgi_variable("smbd_stop") || cgi_variable("all_stop")) {
		stop_smbd();
		waitup=True;
	}

	if (cgi_variable("nmbd_restart") || cgi_variable("all_restart")) {
		stop_nmbd();
		start_nmbd();
		waitup=True;
	}
	if (cgi_variable("nmbd_start") || cgi_variable("all_start")) {
		start_nmbd();
		waitup=True;
	}

	if (cgi_variable("nmbd_stop")|| cgi_variable("all_stop")) {
		stop_nmbd();
		waitup=True;
	}

#ifdef WITH_WINBIND
	if (cgi_variable("winbindd_restart") || cgi_variable("all_restart")) {
		stop_winbindd();
		start_winbindd();
		waitup=True;
	}

	if (cgi_variable("winbindd_start") || cgi_variable("all_start")) {
		start_winbindd();
		waitup=True;
	}

	if (cgi_variable("winbindd_stop") || cgi_variable("all_stop")) {
		stop_winbindd();
		waitup=True;
	}
#endif
	/* wait for daemons to start/stop */
	if (waitup)
		sleep(SLEEP_TIME);
	
	if (cgi_variable("autorefresh")) {
		autorefresh = 1;
	} else if (cgi_variable("norefresh")) {
		autorefresh = 0;
	} else if (cgi_variable("refresh")) {
		autorefresh = 1;
	}

	if ((v=cgi_variable("refresh_interval"))) {
		refresh_interval = atoi(v);
	}

	if (cgi_variable("show_client_in_col_1")) {
		PID_or_Machine = 1;
	}

	if (cgi_variable("show_pid_in_col_1")) {
		PID_or_Machine = 0;
	}

	tdb = tdb_open_log(lock_path("connections.tdb"), 0, TDB_DEFAULT, O_RDONLY, 0);
	if (tdb) tdb_traverse(tdb, traverse_fn1, NULL);
 
	initPid2Machine ();

	printf("<H2>%s</H2>\n", _("Server Status"));

	printf("<FORM method=post>\n");

	if (!autorefresh) {
		printf("<input type=submit value=\"%s\" name=\"autorefresh\">\n", _("Auto Refresh"));
		printf("<br>%s", _("Refresh Interval: "));
		printf("<input type=text size=2 name=\"refresh_interval\" value=\"%d\">\n", 
		       refresh_interval);
	} else {
		printf("<input type=submit value=\"%s\" name=\"norefresh\">\n", _("Stop Refreshing"));
		printf("<br>%s%d\n", _("Refresh Interval: "), refresh_interval);
		printf("<input type=hidden name=\"refresh\" value=\"1\">\n");
	}

	printf("<p>\n");

	if (!tdb) {
		/* open failure either means no connections have been
                   made */
	}


	printf("<table>\n");

	printf("<tr><td>%s</td><td>%s</td></tr>", _("version:"), SAMBA_VERSION_STRING);

	fflush(stdout);
	printf("<tr><td>%s</td><td>%s</td>\n", _("smbd:"), smbd_running()?_("running"):_("not running"));
	if (geteuid() == 0) {
	    if (smbd_running()) {
		nr_running++;
		printf("<td><input type=submit name=\"smbd_stop\" value=\"%s\"></td>\n", _("Stop smbd"));
	    } else {
		printf("<td><input type=submit name=\"smbd_start\" value=\"%s\"></td>\n", _("Start smbd"));
	    }
	    printf("<td><input type=submit name=\"smbd_restart\" value=\"%s\"></td>\n", _("Restart smbd"));
	}
	printf("</tr>\n");

	fflush(stdout);
	printf("<tr><td>%s</td><td>%s</td>\n", _("nmbd:"), nmbd_running()?_("running"):_("not running"));
	if (geteuid() == 0) {
	    if (nmbd_running()) {
		nr_running++;
		printf("<td><input type=submit name=\"nmbd_stop\" value=\"%s\"></td>\n", _("Stop nmbd"));
	    } else {
		printf("<td><input type=submit name=\"nmbd_start\" value=\"%s\"></td>\n", _("Start nmbd"));
	    }
	    printf("<td><input type=submit name=\"nmbd_restart\" value=\"%s\"></td>\n", _("Restart nmbd"));    
	}
	printf("</tr>\n");

#ifdef WITH_WINBIND
	fflush(stdout);
	printf("<tr><td>%s</td><td>%s</td>\n", _("winbindd:"), winbindd_running()?_("running"):_("not running"));
	if (geteuid() == 0) {
	    if (winbindd_running()) {
		nr_running++;
		printf("<td><input type=submit name=\"winbindd_stop\" value=\"%s\"></td>\n", _("Stop winbindd"));
	    } else {
		printf("<td><input type=submit name=\"winbindd_start\" value=\"%s\"></td>\n", _("Start winbindd"));
	    }
	    printf("<td><input type=submit name=\"winbindd_restart\" value=\"%s\"></td>\n", _("Restart winbindd"));
	}
	printf("</tr>\n");
#endif

	if (geteuid() == 0) {
	    printf("<tr><td></td><td></td>\n");
	    if (nr_running >= 1) {
	        /* stop, restart all */
		printf("<td><input type=submit name=\"all_stop\" value=\"%s\"></td>\n", _("Stop All"));
		printf("<td><input type=submit name=\"all_restart\" value=\"%s\"></td>\n", _("Restart All"));
	    }
	    else if (nr_running == 0) {
	    	/* start all */
		printf("<td><input type=submit name=\"all_start\" value=\"%s\"></td>\n", _("Start All"));
	    }
	    printf("</tr>\n");
	}
	printf("</table>\n");
	fflush(stdout);

	printf("<p><h3>%s</h3>\n", _("Active Connections"));
	printf("<table border=1>\n");
	printf("<tr><th>%s</th><th>%s</th><th>%s</th><th>%s</th>\n", _("PID"), _("Client"), _("IP address"), _("Date"));
	if (geteuid() == 0) {
		printf("<th>%s</th>\n", _("Kill"));
	}
	printf("</tr>\n");

	if (tdb) tdb_traverse(tdb, traverse_fn2, NULL);

	printf("</table><p>\n");

	printf("<p><h3>%s</h3>\n", _("Active Shares"));
	printf("<table border=1>\n");
	printf("<tr><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th></tr>\n\n",
		_("Share"), _("User"), _("Group"), _("PID"), _("Client"), _("Date"));

	if (tdb) tdb_traverse(tdb, traverse_fn3, NULL);

	printf("</table><p>\n");

	printf("<h3>%s</h3>\n", _("Open Files"));
	printf("<table border=1>\n");
	printf("<tr><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th><th>%s</th></tr>\n", _("PID"), _("Sharing"), _("R/W"), _("Oplock"), _("File"), _("Date"));

	locking_init(1);
	share_mode_forall(print_share_mode);
	locking_end();
	printf("</table>\n");

	if (tdb) tdb_close(tdb);

	printf("<br><input type=submit name=\"show_client_in_col_1\" value=\"%s\">\n", _("Show Client in col 1"));
	printf("<input type=submit name=\"show_pid_in_col_1\" value=\"%s\">\n", _("Show PID in col 1"));

	printf("</FORM>\n");

	if (autorefresh) {
		/* this little JavaScript allows for automatic refresh
                   of the page. There are other methods but this seems
                   to be the best alternative */
		printf("<script language=\"JavaScript\">\n");
		printf("<!--\nsetTimeout('window.location.replace(\"%s/status?refresh_interval=%d&refresh=1\")', %d)\n", 
		       cgi_baseurl(),
		       refresh_interval,
		       refresh_interval*1000);
		printf("//-->\n</script>\n");
	}
}

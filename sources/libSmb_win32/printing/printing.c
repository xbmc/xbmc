/* 
   Unix SMB/Netbios implementation.
   Version 3.0
   printing backend routines
   Copyright (C) Andrew Tridgell 1992-2000
   Copyright (C) Jeremy Allison 2002
   
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
#include "printing.h"

extern SIG_ATOMIC_T got_sig_term;
extern SIG_ATOMIC_T reload_after_sighup;

/* Current printer interface */
static BOOL remove_from_jobs_changed(const char* sharename, uint32 jobid);

/* 
   the printing backend revolves around a tdb database that stores the
   SMB view of the print queue 
   
   The key for this database is a jobid - a internally generated number that
   uniquely identifies a print job

   reading the print queue involves two steps:
     - possibly running lpq and updating the internal database from that
     - reading entries from the database

   jobids are assigned when a job starts spooling. 
*/

static TDB_CONTEXT *rap_tdb;
static uint16 next_rap_jobid;
struct rap_jobid_key {
	fstring sharename;
	uint32  jobid;
};

/***************************************************************************
 Nightmare. LANMAN jobid's are 16 bit numbers..... We must map them to 32
 bit RPC jobids.... JRA.
***************************************************************************/

uint16 pjobid_to_rap(const char* sharename, uint32 jobid)
{
	uint16 rap_jobid;
	TDB_DATA data, key;
	struct rap_jobid_key jinfo;
	uint8 buf[2];

	DEBUG(10,("pjobid_to_rap: called.\n"));

	if (!rap_tdb) {
		/* Create the in-memory tdb. */
		rap_tdb = tdb_open_log(NULL, 0, TDB_INTERNAL, (O_RDWR|O_CREAT), 0644);
		if (!rap_tdb)
			return 0;
	}

	ZERO_STRUCT( jinfo );
	fstrcpy( jinfo.sharename, sharename );
	jinfo.jobid = jobid;
	key.dptr = (char*)&jinfo;
	key.dsize = sizeof(jinfo);

	data = tdb_fetch(rap_tdb, key);
	if (data.dptr && data.dsize == sizeof(uint16)) {
		rap_jobid = SVAL(data.dptr, 0);
		SAFE_FREE(data.dptr);
		DEBUG(10,("pjobid_to_rap: jobid %u maps to RAP jobid %u\n",
			(unsigned int)jobid, (unsigned int)rap_jobid));
		return rap_jobid;
	}
	SAFE_FREE(data.dptr);
	/* Not found - create and store mapping. */
	rap_jobid = ++next_rap_jobid;
	if (rap_jobid == 0)
		rap_jobid = ++next_rap_jobid;
	SSVAL(buf,0,rap_jobid);
	data.dptr = (char*)buf;
	data.dsize = sizeof(rap_jobid);
	tdb_store(rap_tdb, key, data, TDB_REPLACE);
	tdb_store(rap_tdb, data, key, TDB_REPLACE);

	DEBUG(10,("pjobid_to_rap: created jobid %u maps to RAP jobid %u\n",
		(unsigned int)jobid, (unsigned int)rap_jobid));
	return rap_jobid;
}

BOOL rap_to_pjobid(uint16 rap_jobid, fstring sharename, uint32 *pjobid)
{
	TDB_DATA data, key;
	uint8 buf[2];

	DEBUG(10,("rap_to_pjobid called.\n"));

	if (!rap_tdb)
		return False;

	SSVAL(buf,0,rap_jobid);
	key.dptr = (char*)buf;
	key.dsize = sizeof(rap_jobid);
	data = tdb_fetch(rap_tdb, key);
	if ( data.dptr && data.dsize == sizeof(struct rap_jobid_key) ) 
	{
		struct rap_jobid_key *jinfo = (struct rap_jobid_key*)data.dptr;
		fstrcpy( sharename, jinfo->sharename );
		*pjobid = jinfo->jobid;
		DEBUG(10,("rap_to_pjobid: jobid %u maps to RAP jobid %u\n",
			(unsigned int)*pjobid, (unsigned int)rap_jobid));
		SAFE_FREE(data.dptr);
		return True;
	}

	DEBUG(10,("rap_to_pjobid: Failed to lookup RAP jobid %u\n",
		(unsigned int)rap_jobid));
	SAFE_FREE(data.dptr);
	return False;
}

static void rap_jobid_delete(const char* sharename, uint32 jobid)
{
	TDB_DATA key, data;
	uint16 rap_jobid;
	struct rap_jobid_key jinfo;
	uint8 buf[2];

	DEBUG(10,("rap_jobid_delete: called.\n"));

	if (!rap_tdb)
		return;

	ZERO_STRUCT( jinfo );
	fstrcpy( jinfo.sharename, sharename );
	jinfo.jobid = jobid;
	key.dptr = (char*)&jinfo;
	key.dsize = sizeof(jinfo);

	data = tdb_fetch(rap_tdb, key);
	if (!data.dptr || (data.dsize != sizeof(uint16))) {
		DEBUG(10,("rap_jobid_delete: cannot find jobid %u\n",
			(unsigned int)jobid ));
		SAFE_FREE(data.dptr);
		return;
	}

	DEBUG(10,("rap_jobid_delete: deleting jobid %u\n",
		(unsigned int)jobid ));

	rap_jobid = SVAL(data.dptr, 0);
	SAFE_FREE(data.dptr);
	SSVAL(buf,0,rap_jobid);
	data.dptr = (char*)buf;
	data.dsize = sizeof(rap_jobid);
	tdb_delete(rap_tdb, key);
	tdb_delete(rap_tdb, data);
}

static int get_queue_status(const char* sharename, print_status_struct *);

/****************************************************************************
 Initialise the printing backend. Called once at startup before the fork().
****************************************************************************/

BOOL print_backend_init(void)
{
	const char *sversion = "INFO/version";
	pstring printing_path;
	int services = lp_numservices();
	int snum;

	unlink(lock_path("printing.tdb"));
	pstrcpy(printing_path,lock_path("printing"));
	mkdir(printing_path,0755);

	/* handle a Samba upgrade */

	for (snum = 0; snum < services; snum++) {
		struct tdb_print_db *pdb;
		if (!lp_print_ok(snum))
			continue;

		pdb = get_print_db_byname(lp_const_servicename(snum));
		if (!pdb)
			continue;
		if (tdb_lock_bystring(pdb->tdb, sversion) == -1) {
			DEBUG(0,("print_backend_init: Failed to open printer %s database\n", lp_const_servicename(snum) ));
			release_print_db(pdb);
			return False;
		}
		if (tdb_fetch_int32(pdb->tdb, sversion) != PRINT_DATABASE_VERSION) {
			tdb_traverse(pdb->tdb, tdb_traverse_delete_fn, NULL);
			tdb_store_int32(pdb->tdb, sversion, PRINT_DATABASE_VERSION);
		}
		tdb_unlock_bystring(pdb->tdb, sversion);
		release_print_db(pdb);
	}

	close_all_print_db(); /* Don't leave any open. */

	/* do NT print initialization... */
	return nt_printing_init();
}

/****************************************************************************
 Shut down printing backend. Called once at shutdown to close the tdb.
****************************************************************************/

void printing_end(void)
{
	close_all_print_db(); /* Don't leave any open. */
}

/****************************************************************************
 Retrieve the set of printing functions for a given service.  This allows 
 us to set the printer function table based on the value of the 'printing'
 service parameter.
 
 Use the generic interface as the default and only use cups interface only
 when asked for (and only when supported)
****************************************************************************/

static struct printif *get_printer_fns_from_type( enum printing_types type )
{
	struct printif *printer_fns = &generic_printif;

#ifdef HAVE_CUPS
	if ( type == PRINT_CUPS ) {
		printer_fns = &cups_printif;
	}
#endif /* HAVE_CUPS */

#ifdef HAVE_IPRINT
	if ( type == PRINT_IPRINT ) {
		printer_fns = &iprint_printif;
	}
#endif /* HAVE_IPRINT */

	printer_fns->type = type;
	
	return printer_fns;
}

static struct printif *get_printer_fns( int snum )
{
	return get_printer_fns_from_type( (enum printing_types)lp_printing(snum) );
}


/****************************************************************************
 Useful function to generate a tdb key.
****************************************************************************/

static TDB_DATA print_key(uint32 jobid)
{
	static uint32 j;
	TDB_DATA ret;

	SIVAL(&j, 0, jobid);
	ret.dptr = (void *)&j;
	ret.dsize = sizeof(j);
	return ret;
}

/***********************************************************************
 unpack a pjob from a tdb buffer 
***********************************************************************/
 
int unpack_pjob( char* buf, int buflen, struct printjob *pjob )
{
	int	len = 0;
	int	used;
	uint32 pjpid, pjsysjob, pjfd, pjstarttime, pjstatus;
	uint32 pjsize, pjpage_count, pjspooled, pjsmbjob;

	if ( !buf || !pjob )
		return -1;
		
	len += tdb_unpack(buf+len, buflen-len, "dddddddddffff",
				&pjpid,
				&pjsysjob,
				&pjfd,
				&pjstarttime,
				&pjstatus,
				&pjsize,
				&pjpage_count,
				&pjspooled,
				&pjsmbjob,
				pjob->filename,
				pjob->jobname,
				pjob->user,
				pjob->queuename);
				
	if ( len == -1 )
		return -1;
		
	if ( (used = unpack_devicemode(&pjob->nt_devmode, buf+len, buflen-len)) == -1 )
		return -1;
	
	len += used;

	pjob->pid = pjpid;
	pjob->sysjob = pjsysjob;
	pjob->fd = pjfd;
	pjob->starttime = pjstarttime;
	pjob->status = pjstatus;
	pjob->size = pjsize;
	pjob->page_count = pjpage_count;
	pjob->spooled = pjspooled;
	pjob->smbjob = pjsmbjob;
	
	return len;

}

/****************************************************************************
 Useful function to find a print job in the database.
****************************************************************************/

static struct printjob *print_job_find(const char *sharename, uint32 jobid)
{
	static struct printjob 	pjob;
	TDB_DATA 		ret;
	struct tdb_print_db 	*pdb = get_print_db_byname(sharename);
	
	DEBUG(10,("print_job_find: looking up job %u for share %s\n",
			(unsigned int)jobid, sharename ));

	if (!pdb) {
		return NULL;
	}

	ret = tdb_fetch(pdb->tdb, print_key(jobid));
	release_print_db(pdb);

	if (!ret.dptr) {
		DEBUG(10,("print_job_find: failed to find jobid %u.\n", (unsigned int)jobid ));
		return NULL;
	}
	
	if ( pjob.nt_devmode ) {
		free_nt_devicemode( &pjob.nt_devmode );
	}
		
	ZERO_STRUCT( pjob );
	
	if ( unpack_pjob( ret.dptr, ret.dsize, &pjob ) == -1 ) {
		DEBUG(10,("print_job_find: failed to unpack jobid %u.\n", (unsigned int)jobid ));
		SAFE_FREE(ret.dptr);
		return NULL;
	}
	
	SAFE_FREE(ret.dptr);

	DEBUG(10,("print_job_find: returning system job %d for jobid %u.\n",
			(int)pjob.sysjob, (unsigned int)jobid ));

	return &pjob;
}

/* Convert a unix jobid to a smb jobid */

static uint32 sysjob_to_jobid_value;

static int unixjob_traverse_fn(TDB_CONTEXT *the_tdb, TDB_DATA key,
			       TDB_DATA data, void *state)
{
	struct printjob *pjob;
	int *sysjob = (int *)state;

	if (!data.dptr || data.dsize == 0)
		return 0;

	pjob = (struct printjob *)data.dptr;
	if (key.dsize != sizeof(uint32))
		return 0;

	if (*sysjob == pjob->sysjob) {
		uint32 jobid = IVAL(key.dptr,0);

		sysjob_to_jobid_value = jobid;
		return 1;
	}

	return 0;
}

/****************************************************************************
 This is a *horribly expensive call as we have to iterate through all the
 current printer tdb's. Don't do this often ! JRA.
****************************************************************************/

uint32 sysjob_to_jobid(int unix_jobid)
{
	int services = lp_numservices();
	int snum;

	sysjob_to_jobid_value = (uint32)-1;

	for (snum = 0; snum < services; snum++) {
		struct tdb_print_db *pdb;
		if (!lp_print_ok(snum))
			continue;
		pdb = get_print_db_byname(lp_const_servicename(snum));
		if (!pdb) {
			continue;
		}
		tdb_traverse(pdb->tdb, unixjob_traverse_fn, &unix_jobid);
		release_print_db(pdb);
		if (sysjob_to_jobid_value != (uint32)-1)
			return sysjob_to_jobid_value;
	}
	return (uint32)-1;
}

/****************************************************************************
 Send notifications based on what has changed after a pjob_store.
****************************************************************************/

static struct {
	uint32 lpq_status;
	uint32 spoolss_status;
} lpq_to_spoolss_status_map[] = {
	{ LPQ_QUEUED, JOB_STATUS_QUEUED },
	{ LPQ_PAUSED, JOB_STATUS_PAUSED },
	{ LPQ_SPOOLING, JOB_STATUS_SPOOLING },
	{ LPQ_PRINTING, JOB_STATUS_PRINTING },
	{ LPQ_DELETING, JOB_STATUS_DELETING },
	{ LPQ_OFFLINE, JOB_STATUS_OFFLINE },
	{ LPQ_PAPEROUT, JOB_STATUS_PAPEROUT },
	{ LPQ_PRINTED, JOB_STATUS_PRINTED },
	{ LPQ_DELETED, JOB_STATUS_DELETED },
	{ LPQ_BLOCKED, JOB_STATUS_BLOCKED },
	{ LPQ_USER_INTERVENTION, JOB_STATUS_USER_INTERVENTION },
	{ -1, 0 }
};

/* Convert a lpq status value stored in printing.tdb into the
   appropriate win32 API constant. */

static uint32 map_to_spoolss_status(uint32 lpq_status)
{
	int i = 0;

	while (lpq_to_spoolss_status_map[i].lpq_status != -1) {
		if (lpq_to_spoolss_status_map[i].lpq_status == lpq_status)
			return lpq_to_spoolss_status_map[i].spoolss_status;
		i++;
	}

	return 0;
}

static void pjob_store_notify(const char* sharename, uint32 jobid, struct printjob *old_data,
			      struct printjob *new_data)
{
	BOOL new_job = False;

	if (!old_data)
		new_job = True;

	/* Job attributes that can't be changed.  We only send
	   notification for these on a new job. */

 	/* ACHTUNG!  Due to a bug in Samba's spoolss parsing of the 
 	   NOTIFY_INFO_DATA buffer, we *have* to send the job submission 
 	   time first or else we'll end up with potential alignment 
 	   errors.  I don't think the systemtime should be spooled as 
 	   a string, but this gets us around that error.   
 	   --jerry (i'll feel dirty for this) */
 
	if (new_job) {
		notify_job_submitted(sharename, jobid, new_data->starttime);
		notify_job_username(sharename, jobid, new_data->user);
	}

	if (new_job || !strequal(old_data->jobname, new_data->jobname))
		notify_job_name(sharename, jobid, new_data->jobname);

	/* Job attributes of a new job or attributes that can be
	   modified. */

	if (new_job || !strequal(old_data->jobname, new_data->jobname))
		notify_job_name(sharename, jobid, new_data->jobname);

	if (new_job || old_data->status != new_data->status)
		notify_job_status(sharename, jobid, map_to_spoolss_status(new_data->status));

	if (new_job || old_data->size != new_data->size)
		notify_job_total_bytes(sharename, jobid, new_data->size);

	if (new_job || old_data->page_count != new_data->page_count)
		notify_job_total_pages(sharename, jobid, new_data->page_count);
}

/****************************************************************************
 Store a job structure back to the database.
****************************************************************************/

static BOOL pjob_store(const char* sharename, uint32 jobid, struct printjob *pjob)
{
	TDB_DATA 		old_data, new_data;
	BOOL 			ret = False;
	struct tdb_print_db 	*pdb = get_print_db_byname(sharename);
	char			*buf = NULL;
	int			len, newlen, buflen;
	

	if (!pdb)
		return False;

	/* Get old data */

	old_data = tdb_fetch(pdb->tdb, print_key(jobid));

	/* Doh!  Now we have to pack/unpack data since the NT_DEVICEMODE was added */

	newlen = 0;
	
	do {
		len = 0;
		buflen = newlen;
		len += tdb_pack(buf+len, buflen-len, "dddddddddffff",
				(uint32)pjob->pid,
				(uint32)pjob->sysjob,
				(uint32)pjob->fd,
				(uint32)pjob->starttime,
				(uint32)pjob->status,
				(uint32)pjob->size,
				(uint32)pjob->page_count,
				(uint32)pjob->spooled,
				(uint32)pjob->smbjob,
				pjob->filename,
				pjob->jobname,
				pjob->user,
				pjob->queuename);

		len += pack_devicemode(pjob->nt_devmode, buf+len, buflen-len);
	
		if (buflen != len) {
			buf = (char *)SMB_REALLOC(buf, len);
			if (!buf) {
				DEBUG(0,("pjob_store: failed to enlarge buffer!\n"));
				goto done;
			}
			newlen = len;
		}
	} while ( buflen != len );
		
	
	/* Store new data */

	new_data.dptr = buf;
	new_data.dsize = len;
	ret = (tdb_store(pdb->tdb, print_key(jobid), new_data, TDB_REPLACE) == 0);

	release_print_db(pdb);

	/* Send notify updates for what has changed */

	if ( ret ) {
		struct printjob old_pjob;

		if ( old_data.dsize )
		{
			if ( unpack_pjob( old_data.dptr, old_data.dsize, &old_pjob ) != -1 )
			{
				pjob_store_notify( sharename, jobid, &old_pjob , pjob );
				free_nt_devicemode( &old_pjob.nt_devmode );
			}
		}
		else {
			/* new job */
			pjob_store_notify( sharename, jobid, NULL, pjob );
		}
	}

done:
	SAFE_FREE( old_data.dptr );
	SAFE_FREE( buf );

	return ret;
}

/****************************************************************************
 Remove a job structure from the database.
****************************************************************************/

void pjob_delete(const char* sharename, uint32 jobid)
{
	struct printjob *pjob;
	uint32 job_status = 0;
	struct tdb_print_db *pdb;

	pdb = get_print_db_byname( sharename );

	if (!pdb)
		return;

	pjob = print_job_find( sharename, jobid );

	if (!pjob) {
		DEBUG(5, ("pjob_delete: we were asked to delete nonexistent job %u\n",
					(unsigned int)jobid));
		release_print_db(pdb);
		return;
	}

	/* We must cycle through JOB_STATUS_DELETING and
           JOB_STATUS_DELETED for the port monitor to delete the job
           properly. */
	
	job_status = JOB_STATUS_DELETING|JOB_STATUS_DELETED;
	notify_job_status(sharename, jobid, job_status);
	
	/* Remove from printing.tdb */

	tdb_delete(pdb->tdb, print_key(jobid));
	remove_from_jobs_changed(sharename, jobid);
	release_print_db( pdb );
	rap_jobid_delete(sharename, jobid);
}

/****************************************************************************
 Parse a file name from the system spooler to generate a jobid.
****************************************************************************/

static uint32 print_parse_jobid(char *fname)
{
	int jobid;

	if (strncmp(fname,PRINT_SPOOL_PREFIX,strlen(PRINT_SPOOL_PREFIX)) != 0)
		return (uint32)-1;
	fname += strlen(PRINT_SPOOL_PREFIX);

	jobid = atoi(fname);
	if (jobid <= 0)
		return (uint32)-1;

	return (uint32)jobid;
}

/****************************************************************************
 List a unix job in the print database.
****************************************************************************/

static void print_unix_job(const char *sharename, print_queue_struct *q, uint32 jobid)
{
	struct printjob pj, *old_pj;

	if (jobid == (uint32)-1) 
		jobid = q->job + UNIX_JOB_START;

	/* Preserve the timestamp on an existing unix print job */

	old_pj = print_job_find(sharename, jobid);

	ZERO_STRUCT(pj);

	pj.pid = (pid_t)-1;
	pj.sysjob = q->job;
	pj.fd = -1;
	pj.starttime = old_pj ? old_pj->starttime : q->time;
	pj.status = q->status;
	pj.size = q->size;
	pj.spooled = True;
	fstrcpy(pj.filename, old_pj ? old_pj->filename : "");
	if (jobid < UNIX_JOB_START) {
		pj.smbjob = True;
		fstrcpy(pj.jobname, old_pj ? old_pj->jobname : "Remote Downlevel Document");
	} else {
		pj.smbjob = False;
		fstrcpy(pj.jobname, old_pj ? old_pj->jobname : q->fs_file);
	}
	fstrcpy(pj.user, old_pj ? old_pj->user : q->fs_user);
	fstrcpy(pj.queuename, old_pj ? old_pj->queuename : sharename );

	pjob_store(sharename, jobid, &pj);
}


struct traverse_struct {
	print_queue_struct *queue;
	int qcount, snum, maxcount, total_jobs;
	const char *sharename;
	time_t lpq_time;
	const char *lprm_command;
	struct printif *print_if;
};

/****************************************************************************
 Utility fn to delete any jobs that are no longer active.
****************************************************************************/

static int traverse_fn_delete(TDB_CONTEXT *t, TDB_DATA key, TDB_DATA data, void *state)
{
	struct traverse_struct *ts = (struct traverse_struct *)state;
	struct printjob pjob;
	uint32 jobid;
	int i = 0;

	if (  key.dsize != sizeof(jobid) )
		return 0;
		
	jobid = IVAL(key.dptr, 0);
	if ( unpack_pjob( data.dptr, data.dsize, &pjob ) == -1 )
		return 0;
	free_nt_devicemode( &pjob.nt_devmode );


	if (!pjob.smbjob) {
		/* remove a unix job if it isn't in the system queue any more */

		for (i=0;i<ts->qcount;i++) {
			uint32 u_jobid = (ts->queue[i].job + UNIX_JOB_START);
			if (jobid == u_jobid)
				break;
		}
		if (i == ts->qcount) {
			DEBUG(10,("traverse_fn_delete: pjob %u deleted due to !smbjob\n",
						(unsigned int)jobid ));
			pjob_delete(ts->sharename, jobid);
			return 0;
		} 

		/* need to continue the the bottom of the function to
		   save the correct attributes */
	}

	/* maybe it hasn't been spooled yet */
	if (!pjob.spooled) {
		/* if a job is not spooled and the process doesn't
                   exist then kill it. This cleans up after smbd
                   deaths */
		if (!process_exists_by_pid(pjob.pid)) {
			DEBUG(10,("traverse_fn_delete: pjob %u deleted due to !process_exists (%u)\n",
						(unsigned int)jobid, (unsigned int)pjob.pid ));
			pjob_delete(ts->sharename, jobid);
		} else
			ts->total_jobs++;
		return 0;
	}

	/* this check only makes sense for jobs submitted from Windows clients */
	
	if ( pjob.smbjob ) {
		for (i=0;i<ts->qcount;i++) {
			uint32 curr_jobid;

			if ( pjob.status == LPQ_DELETED )
				continue;

			curr_jobid = print_parse_jobid(ts->queue[i].fs_file);

			if (jobid == curr_jobid) {

				/* try to clean up any jobs that need to be deleted */

				if ( pjob.status == LPQ_DELETING ) {
					int result;

					result = (*(ts->print_if->job_delete))( 
						ts->sharename, ts->lprm_command, &pjob );

					if ( result != 0 ) {
						/* if we can't delete, then reset the job status */
						pjob.status = LPQ_QUEUED;
						pjob_store(ts->sharename, jobid, &pjob);
					}
					else {
						/* if we deleted the job, the remove the tdb record */
						pjob_delete(ts->sharename, jobid);
						pjob.status = LPQ_DELETED;
					}
						
				}

				break;
			}
		}
	}
	
	/* The job isn't in the system queue - we have to assume it has
	   completed, so delete the database entry. */

	if (i == ts->qcount) {

		/* A race can occur between the time a job is spooled and
		   when it appears in the lpq output.  This happens when
		   the job is added to printing.tdb when another smbd
		   running print_queue_update() has completed a lpq and
		   is currently traversing the printing tdb and deleting jobs.
		   Don't delete the job if it was submitted after the lpq_time. */

		if (pjob.starttime < ts->lpq_time) {
			DEBUG(10,("traverse_fn_delete: pjob %u deleted due to pjob.starttime (%u) < ts->lpq_time (%u)\n",
						(unsigned int)jobid,
						(unsigned int)pjob.starttime,
						(unsigned int)ts->lpq_time ));
			pjob_delete(ts->sharename, jobid);
		} else
			ts->total_jobs++;
		return 0;
	}

	/* Save the pjob attributes we will store. 
	   FIXME!!! This is the only place where queue->job 
	   represents the SMB jobid      --jerry */

	ts->queue[i].job = jobid;		
	ts->queue[i].size = pjob.size;
	ts->queue[i].page_count = pjob.page_count;
	ts->queue[i].status = pjob.status;
	ts->queue[i].priority = 1;
	ts->queue[i].time = pjob.starttime;
	fstrcpy(ts->queue[i].fs_user, pjob.user);
	fstrcpy(ts->queue[i].fs_file, pjob.jobname);

	ts->total_jobs++;

	return 0;
}

/****************************************************************************
 Check if the print queue has been updated recently enough.
****************************************************************************/

static void print_cache_flush(int snum)
{
	fstring key;
	const char *sharename = lp_const_servicename(snum);
	struct tdb_print_db *pdb = get_print_db_byname(sharename);

	if (!pdb)
		return;
	slprintf(key, sizeof(key)-1, "CACHE/%s", sharename);
	tdb_store_int32(pdb->tdb, key, -1);
	release_print_db(pdb);
}

/****************************************************************************
 Check if someone already thinks they are doing the update.
****************************************************************************/

static pid_t get_updating_pid(const char *sharename)
{
	fstring keystr;
	TDB_DATA data, key;
	pid_t updating_pid;
	struct tdb_print_db *pdb = get_print_db_byname(sharename);

	if (!pdb)
		return (pid_t)-1;
	slprintf(keystr, sizeof(keystr)-1, "UPDATING/%s", sharename);
    	key.dptr = keystr;
	key.dsize = strlen(keystr);

	data = tdb_fetch(pdb->tdb, key);
	release_print_db(pdb);
	if (!data.dptr || data.dsize != sizeof(pid_t)) {
		SAFE_FREE(data.dptr);
		return (pid_t)-1;
	}

	updating_pid = IVAL(data.dptr, 0);
	SAFE_FREE(data.dptr);

	if (process_exists_by_pid(updating_pid))
		return updating_pid;

	return (pid_t)-1;
}

/****************************************************************************
 Set the fact that we're doing the update, or have finished doing the update
 in the tdb.
****************************************************************************/

static void set_updating_pid(const fstring sharename, BOOL updating)
{
	fstring keystr;
	TDB_DATA key;
	TDB_DATA data;
	pid_t updating_pid = sys_getpid();
	uint8 buffer[4];
	
	struct tdb_print_db *pdb = get_print_db_byname(sharename);

	if (!pdb)
		return;

	slprintf(keystr, sizeof(keystr)-1, "UPDATING/%s", sharename);
    	key.dptr = keystr;
	key.dsize = strlen(keystr);
	
	DEBUG(5, ("set_updating_pid: %s updating lpq cache for print share %s\n", 
		updating ? "" : "not ",
		sharename ));

	if ( !updating ) {
		tdb_delete(pdb->tdb, key);
		release_print_db(pdb);
		return;
	}
	
	SIVAL( buffer, 0, updating_pid);
	data.dptr = (void *)buffer;
	data.dsize = 4;		/* we always assume this is a 4 byte value */

	tdb_store(pdb->tdb, key, data, TDB_REPLACE);	
	release_print_db(pdb);
}

/****************************************************************************
 Sort print jobs by submittal time.
****************************************************************************/

static int printjob_comp(print_queue_struct *j1, print_queue_struct *j2)
{
	/* Silly cases */

	if (!j1 && !j2)
		return 0;
	if (!j1)
		return -1;
	if (!j2)
		return 1;

	/* Sort on job start time */

	if (j1->time == j2->time)
		return 0;
	return (j1->time > j2->time) ? 1 : -1;
}

/****************************************************************************
 Store the sorted queue representation for later portmon retrieval.
 Skip deleted jobs
****************************************************************************/

static void store_queue_struct(struct tdb_print_db *pdb, struct traverse_struct *pts)
{
	TDB_DATA data;
	int max_reported_jobs = lp_max_reported_jobs(pts->snum);
	print_queue_struct *queue = pts->queue;
	size_t len;
	size_t i;
	uint qcount;

	if (max_reported_jobs && (max_reported_jobs < pts->qcount))
		pts->qcount = max_reported_jobs;
	qcount = 0;

	/* Work out the size. */
	data.dsize = 0;
	data.dsize += tdb_pack(NULL, 0, "d", qcount);

	for (i = 0; i < pts->qcount; i++) {
		if ( queue[i].status == LPQ_DELETED )
			continue;

		qcount++;
		data.dsize += tdb_pack(NULL, 0, "ddddddff",
				(uint32)queue[i].job,
				(uint32)queue[i].size,
				(uint32)queue[i].page_count,
				(uint32)queue[i].status,
				(uint32)queue[i].priority,
				(uint32)queue[i].time,
				queue[i].fs_user,
				queue[i].fs_file);
	}

	if ((data.dptr = SMB_MALLOC(data.dsize)) == NULL)
		return;

        len = 0;
	len += tdb_pack(data.dptr + len, data.dsize - len, "d", qcount);
	for (i = 0; i < pts->qcount; i++) {
		if ( queue[i].status == LPQ_DELETED )
			continue;

		len += tdb_pack(data.dptr + len, data.dsize - len, "ddddddff",
				(uint32)queue[i].job,
				(uint32)queue[i].size,
				(uint32)queue[i].page_count,
				(uint32)queue[i].status,
				(uint32)queue[i].priority,
				(uint32)queue[i].time,
				queue[i].fs_user,
				queue[i].fs_file);
	}

	tdb_store(pdb->tdb, string_tdb_data("INFO/linear_queue_array"), data,
		  TDB_REPLACE);
	SAFE_FREE(data.dptr);
	return;
}

static TDB_DATA get_jobs_changed_data(struct tdb_print_db *pdb)
{
	TDB_DATA data;

	ZERO_STRUCT(data);

	data = tdb_fetch(pdb->tdb, string_tdb_data("INFO/jobs_changed"));
	if (data.dptr == NULL || data.dsize == 0 || (data.dsize % 4 != 0)) {
		SAFE_FREE(data.dptr);
		ZERO_STRUCT(data);
	}

	return data;
}

static void check_job_changed(const char *sharename, TDB_DATA data, uint32 jobid)
{
	unsigned int i;
	unsigned int job_count = data.dsize / 4;

	for (i = 0; i < job_count; i++) {
		uint32 ch_jobid;

		ch_jobid = IVAL(data.dptr, i*4);
		if (ch_jobid == jobid)
			remove_from_jobs_changed(sharename, jobid);
	}
}

/****************************************************************************
 Check if the print queue has been updated recently enough.
****************************************************************************/

static BOOL print_cache_expired(const char *sharename, BOOL check_pending)
{
	fstring key;
	time_t last_qscan_time, time_now = time(NULL);
	struct tdb_print_db *pdb = get_print_db_byname(sharename);
	BOOL result = False;

	if (!pdb)
		return False;

	snprintf(key, sizeof(key), "CACHE/%s", sharename);
	last_qscan_time = (time_t)tdb_fetch_int32(pdb->tdb, key);

	/*
	 * Invalidate the queue for 3 reasons.
	 * (1). last queue scan time == -1.
	 * (2). Current time - last queue scan time > allowed cache time.
	 * (3). last queue scan time > current time + MAX_CACHE_VALID_TIME (1 hour by default).
	 * This last test picks up machines for which the clock has been moved
	 * forward, an lpq scan done and then the clock moved back. Otherwise
	 * that last lpq scan would stay around for a loooong loooong time... :-). JRA.
	 */

	if (last_qscan_time == ((time_t)-1) 
		|| (time_now - last_qscan_time) >= lp_lpqcachetime() 
		|| last_qscan_time > (time_now + MAX_CACHE_VALID_TIME)) 
	{
		uint32 u;
		time_t msg_pending_time;

		DEBUG(4, ("print_cache_expired: cache expired for queue %s " 
			"(last_qscan_time = %d, time now = %d, qcachetime = %d)\n", 
			sharename, (int)last_qscan_time, (int)time_now, 
			(int)lp_lpqcachetime() ));

		/* check if another smbd has already sent a message to update the 
		   queue.  Give the pending message one minute to clear and 
		   then send another message anyways.  Make sure to check for 
		   clocks that have been run forward and then back again. */

		snprintf(key, sizeof(key), "MSG_PENDING/%s", sharename);

		if ( check_pending 
			&& tdb_fetch_uint32( pdb->tdb, key, &u ) 
			&& (msg_pending_time=u) > 0
			&& msg_pending_time <= time_now 
			&& (time_now - msg_pending_time) < 60 ) 
		{
			DEBUG(4,("print_cache_expired: message already pending for %s.  Accepting cache\n",
				sharename));
			goto done;
		}
		
		result = True;
	}

done:
	release_print_db(pdb);
	return result;
}

/****************************************************************************
 main work for updating the lpq cahe for a printer queue
****************************************************************************/

static void print_queue_update_internal( const char *sharename, 
                                         struct printif *current_printif,
                                         char *lpq_command, char *lprm_command )
{
	int i, qcount;
	print_queue_struct *queue = NULL;
	print_status_struct status;
	print_status_struct old_status;
	struct printjob *pjob;
	struct traverse_struct tstruct;
	TDB_DATA data, key;
	TDB_DATA jcdata;
	fstring keystr, cachestr;
	struct tdb_print_db *pdb = get_print_db_byname(sharename);

	if (!pdb) {
		return;
	}
	
	DEBUG(5,("print_queue_update_internal: printer = %s, type = %d, lpq command = [%s]\n",
		sharename, current_printif->type, lpq_command));

	/*
	 * Update the cache time FIRST ! Stops others even
	 * attempting to get the lock and doing this
	 * if the lpq takes a long time.
	 */
	 
	slprintf(cachestr, sizeof(cachestr)-1, "CACHE/%s", sharename);
	tdb_store_int32(pdb->tdb, cachestr, (int)time(NULL));

        /* get the current queue using the appropriate interface */
	ZERO_STRUCT(status);

	qcount = (*(current_printif->queue_get))(sharename, 
		current_printif->type, 
		lpq_command, &queue, &status);

	DEBUG(3, ("print_queue_update_internal: %d job%s in queue for %s\n", 
		qcount, (qcount != 1) ?	"s" : "", sharename));

	/* Sort the queue by submission time otherwise they are displayed
	   in hash order. */

	qsort(queue, qcount, sizeof(print_queue_struct),
		QSORT_CAST(printjob_comp));

	/*
	  any job in the internal database that is marked as spooled
	  and doesn't exist in the system queue is considered finished
	  and removed from the database

	  any job in the system database but not in the internal database 
	  is added as a unix job

	  fill in any system job numbers as we go
	*/

	jcdata = get_jobs_changed_data(pdb);

	for (i=0; i<qcount; i++) {
		uint32 jobid = print_parse_jobid(queue[i].fs_file);

		if (jobid == (uint32)-1) {
			/* assume its a unix print job */
			print_unix_job(sharename, &queue[i], jobid);
			continue;
		}

		/* we have an active SMB print job - update its status */
		pjob = print_job_find(sharename, jobid);
		if (!pjob) {
			/* err, somethings wrong. Probably smbd was restarted
			   with jobs in the queue. All we can do is treat them
			   like unix jobs. Pity. */
			print_unix_job(sharename, &queue[i], jobid);
			continue;
		}

		pjob->sysjob = queue[i].job;

		/* don't reset the status on jobs to be deleted */

		if ( pjob->status != LPQ_DELETING )
			pjob->status = queue[i].status;

		pjob_store(sharename, jobid, pjob);

		check_job_changed(sharename, jcdata, jobid);
	}

	SAFE_FREE(jcdata.dptr);

	/* now delete any queued entries that don't appear in the
           system queue */
	tstruct.queue = queue;
	tstruct.qcount = qcount;
	tstruct.snum = -1;
	tstruct.total_jobs = 0;
	tstruct.lpq_time = time(NULL);
	tstruct.sharename = sharename;
	tstruct.lprm_command = lprm_command;
	tstruct.print_if = current_printif;

	tdb_traverse(pdb->tdb, traverse_fn_delete, (void *)&tstruct);

	/* Store the linearised queue, max jobs only. */
	store_queue_struct(pdb, &tstruct);

	SAFE_FREE(tstruct.queue);

	DEBUG(10,("print_queue_update_internal: printer %s INFO/total_jobs = %d\n",
				sharename, tstruct.total_jobs ));

	tdb_store_int32(pdb->tdb, "INFO/total_jobs", tstruct.total_jobs);

	get_queue_status(sharename, &old_status);
	if (old_status.qcount != qcount)
		DEBUG(10,("print_queue_update_internal: queue status change %d jobs -> %d jobs for printer %s\n",
					old_status.qcount, qcount, sharename));

	/* store the new queue status structure */
	slprintf(keystr, sizeof(keystr)-1, "STATUS/%s", sharename);
	key.dptr = keystr;
	key.dsize = strlen(keystr);

	status.qcount = qcount;
	data.dptr = (void *)&status;
	data.dsize = sizeof(status);
	tdb_store(pdb->tdb, key, data, TDB_REPLACE);	

	/*
	 * Update the cache time again. We want to do this call
	 * as little as possible...
	 */

	slprintf(keystr, sizeof(keystr)-1, "CACHE/%s", sharename);
	tdb_store_int32(pdb->tdb, keystr, (int32)time(NULL));

	/* clear the msg pending record for this queue */

	snprintf(keystr, sizeof(keystr), "MSG_PENDING/%s", sharename);

	if ( !tdb_store_uint32( pdb->tdb, keystr, 0 ) ) {
		/* log a message but continue on */

		DEBUG(0,("print_queue_update: failed to store MSG_PENDING flag for [%s]!\n",
			sharename));
	}

	release_print_db( pdb );

	return;
}

/****************************************************************************
 Update the internal database from the system print queue for a queue.
 obtain a lock on the print queue before proceeding (needed when mutiple
 smbd processes maytry to update the lpq cache concurrently).
****************************************************************************/

static void print_queue_update_with_lock( const char *sharename, 
                                          struct printif *current_printif,
                                          char *lpq_command, char *lprm_command )
{
	fstring keystr;
	struct tdb_print_db *pdb;

	DEBUG(5,("print_queue_update_with_lock: printer share = %s\n", sharename));
	pdb = get_print_db_byname(sharename);
	if (!pdb)
		return;

	if ( !print_cache_expired(sharename, False) ) {
		DEBUG(5,("print_queue_update_with_lock: print cache for %s is still ok\n", sharename));
		release_print_db(pdb);
		return;
	}
	
	/*
	 * Check to see if someone else is doing this update.
	 * This is essentially a mutex on the update.
	 */

	if (get_updating_pid(sharename) != -1) {
		release_print_db(pdb);
		return;
	}

	/* Lock the queue for the database update */

	slprintf(keystr, sizeof(keystr) - 1, "LOCK/%s", sharename);
	/* Only wait 10 seconds for this. */
	if (tdb_lock_bystring_with_timeout(pdb->tdb, keystr, 10) == -1) {
		DEBUG(0,("print_queue_update_with_lock: Failed to lock printer %s database\n", sharename));
		release_print_db(pdb);
		return;
	}

	/*
	 * Ensure that no one else got in here.
	 * If the updating pid is still -1 then we are
	 * the winner.
	 */

	if (get_updating_pid(sharename) != -1) {
		/*
		 * Someone else is doing the update, exit.
		 */
		tdb_unlock_bystring(pdb->tdb, keystr);
		release_print_db(pdb);
		return;
	}

	/*
	 * We're going to do the update ourselves.
	 */

	/* Tell others we're doing the update. */
	set_updating_pid(sharename, True);

	/*
	 * Allow others to enter and notice we're doing
	 * the update.
	 */

	tdb_unlock_bystring(pdb->tdb, keystr);

	/* do the main work now */
	
	print_queue_update_internal( sharename, current_printif, 
		lpq_command, lprm_command );
	
	/* Delete our pid from the db. */
	set_updating_pid(sharename, False);
	release_print_db(pdb);
}

/****************************************************************************
this is the receive function of the background lpq updater
****************************************************************************/
static void print_queue_receive(int msg_type, struct process_id src,
				void *buf, size_t msglen)
{
	fstring sharename;
	pstring lpqcommand, lprmcommand;
	int printing_type;
	size_t len;

	len = tdb_unpack( buf, msglen, "fdPP",
		sharename,
		&printing_type,
		lpqcommand,
		lprmcommand );

	if ( len == -1 ) {
		DEBUG(0,("print_queue_receive: Got invalid print queue update message\n"));
		return;
	}

	print_queue_update_with_lock(sharename, 
		get_printer_fns_from_type((enum printing_types)printing_type),
		lpqcommand, lprmcommand );

	return;
}

static pid_t background_lpq_updater_pid = -1;

/****************************************************************************
main thread of the background lpq updater
****************************************************************************/
void start_background_queue(void)
{
	DEBUG(3,("start_background_queue: Starting background LPQ thread\n"));
	background_lpq_updater_pid = sys_fork();

	if (background_lpq_updater_pid == -1) {
		DEBUG(5,("start_background_queue: background LPQ thread failed to start. %s\n", strerror(errno) ));
		exit(1);
	}

	if(background_lpq_updater_pid == 0) {
		/* Child. */
		DEBUG(5,("start_background_queue: background LPQ thread started\n"));

		claim_connection( NULL, "smbd lpq backend", 0, False, 
			FLAG_MSG_GENERAL|FLAG_MSG_SMBD|FLAG_MSG_PRINT_GENERAL);

		if (!locking_init(0)) {
			exit(1);
		}

		message_register(MSG_PRINTER_UPDATE, print_queue_receive);
		
		DEBUG(5,("start_background_queue: background LPQ thread waiting for messages\n"));
		while (1) {
			pause();
			
			/* check for some essential signals first */
			
                        if (got_sig_term) {
                                exit_server_cleanly(NULL);
                        }

                        if (reload_after_sighup) {
                                change_to_root_user();
                                DEBUG(1,("Reloading services after SIGHUP\n"));
                                reload_services(False);
                                reload_after_sighup = 0;
                        }
			
			/* now check for messages */
			
			DEBUG(10,("start_background_queue: background LPQ thread got a message\n"));
			message_dispatch();

			/* process any pending print change notify messages */

			print_notify_send_messages(0);
		}
	}
}

/****************************************************************************
update the internal database from the system print queue for a queue
****************************************************************************/

static void print_queue_update(int snum, BOOL force)
{
	fstring key;
	fstring sharename;
	pstring lpqcommand, lprmcommand;
	char *buffer = NULL;
	size_t len = 0;
	size_t newlen;
	struct tdb_print_db *pdb;
	int type;
	struct printif *current_printif;

	fstrcpy( sharename, lp_const_servicename(snum));

	/* don't strip out characters like '$' from the printername */
	
	pstrcpy( lpqcommand, lp_lpqcommand(snum));
	string_sub2( lpqcommand, "%p", PRINTERNAME(snum), sizeof(lpqcommand), 
		     False, False, False );
	standard_sub_snum( snum, lpqcommand, sizeof(lpqcommand) );
	
	pstrcpy( lprmcommand, lp_lprmcommand(snum));
	string_sub2( lprmcommand, "%p", PRINTERNAME(snum), sizeof(lprmcommand), 
		     False, False, False );
	standard_sub_snum( snum, lprmcommand, sizeof(lprmcommand) );
	
	/* 
	 * Make sure that the background queue process exists.  
	 * Otherwise just do the update ourselves 
	 */
	
	if ( force || background_lpq_updater_pid == -1 ) {
		DEBUG(4,("print_queue_update: updating queue [%s] myself\n", sharename));
		current_printif = get_printer_fns( snum );
		print_queue_update_with_lock( sharename, current_printif, lpqcommand, lprmcommand );

		return;
	}

	type = lp_printing(snum);
	
	/* get the length */

	len = tdb_pack( NULL, 0, "fdPP",
		sharename,
		type,
		lpqcommand, 
		lprmcommand );

	buffer = SMB_XMALLOC_ARRAY( char, len );

	/* now pack the buffer */
	newlen = tdb_pack( buffer, len, "fdPP",
		sharename,
		type,
		lpqcommand,
		lprmcommand );

	SMB_ASSERT( newlen == len );

	DEBUG(10,("print_queue_update: Sending message -> printer = %s, "
		"type = %d, lpq command = [%s] lprm command = [%s]\n", 
		sharename, type, lpqcommand, lprmcommand ));

	/* here we set a msg pending record for other smbd processes 
	   to throttle the number of duplicate print_queue_update msgs
	   sent.  */

	pdb = get_print_db_byname(sharename);
	if (!pdb) {
		SAFE_FREE(buffer);
		return;
	}

	snprintf(key, sizeof(key), "MSG_PENDING/%s", sharename);

	if ( !tdb_store_uint32( pdb->tdb, key, time(NULL) ) ) {
		/* log a message but continue on */

		DEBUG(0,("print_queue_update: failed to store MSG_PENDING flag for [%s]!\n",
			sharename));
	}

	release_print_db( pdb );

	/* finally send the message */
	
	become_root();
	message_send_pid(pid_to_procid(background_lpq_updater_pid),
		 MSG_PRINTER_UPDATE, buffer, len, False);
	unbecome_root();

	SAFE_FREE( buffer );

	return;
}

/****************************************************************************
 Create/Update an entry in the print tdb that will allow us to send notify
 updates only to interested smbd's. 
****************************************************************************/

BOOL print_notify_register_pid(int snum)
{
	TDB_DATA data;
	struct tdb_print_db *pdb = NULL;
	TDB_CONTEXT *tdb = NULL;
	const char *printername;
	uint32 mypid = (uint32)sys_getpid();
	BOOL ret = False;
	size_t i;

	/* if (snum == -1), then the change notify request was
	   on a print server handle and we need to register on
	   all print queus */
	   
	if (snum == -1) 
	{
		int num_services = lp_numservices();
		int idx;
		
		for ( idx=0; idx<num_services; idx++ ) {
			if (lp_snum_ok(idx) && lp_print_ok(idx) )
				print_notify_register_pid(idx);
		}
		
		return True;
	}
	else /* register for a specific printer */
	{
		printername = lp_const_servicename(snum);
		pdb = get_print_db_byname(printername);
		if (!pdb)
			return False;
		tdb = pdb->tdb;
	}

	if (tdb_lock_bystring_with_timeout(tdb, NOTIFY_PID_LIST_KEY, 10) == -1) {
		DEBUG(0,("print_notify_register_pid: Failed to lock printer %s\n",
					printername));
		if (pdb)
			release_print_db(pdb);
		return False;
	}

	data = get_printer_notify_pid_list( tdb, printername, True );

	/* Add ourselves and increase the refcount. */

	for (i = 0; i < data.dsize; i += 8) {
		if (IVAL(data.dptr,i) == mypid) {
			uint32 new_refcount = IVAL(data.dptr, i+4) + 1;
			SIVAL(data.dptr, i+4, new_refcount);
			break;
		}
	}

	if (i == data.dsize) {
		/* We weren't in the list. Realloc. */
		data.dptr = SMB_REALLOC(data.dptr, data.dsize + 8);
		if (!data.dptr) {
			DEBUG(0,("print_notify_register_pid: Relloc fail for printer %s\n",
						printername));
			goto done;
		}
		data.dsize += 8;
		SIVAL(data.dptr,data.dsize - 8,mypid);
		SIVAL(data.dptr,data.dsize - 4,1); /* Refcount. */
	}

	/* Store back the record. */
	if (tdb_store_bystring(tdb, NOTIFY_PID_LIST_KEY, data, TDB_REPLACE) == -1) {
		DEBUG(0,("print_notify_register_pid: Failed to update pid \
list for printer %s\n", printername));
		goto done;
	}

	ret = True;

 done:

	tdb_unlock_bystring(tdb, NOTIFY_PID_LIST_KEY);
	if (pdb)
		release_print_db(pdb);
	SAFE_FREE(data.dptr);
	return ret;
}

/****************************************************************************
 Update an entry in the print tdb that will allow us to send notify
 updates only to interested smbd's. 
****************************************************************************/

BOOL print_notify_deregister_pid(int snum)
{
	TDB_DATA data;
	struct tdb_print_db *pdb = NULL;
	TDB_CONTEXT *tdb = NULL;
	const char *printername;
	uint32 mypid = (uint32)sys_getpid();
	size_t i;
	BOOL ret = False;

	/* if ( snum == -1 ), we are deregister a print server handle
	   which means to deregister on all print queues */
	   
	if (snum == -1) 
	{
		int num_services = lp_numservices();
		int idx;
		
		for ( idx=0; idx<num_services; idx++ ) {
			if ( lp_snum_ok(idx) && lp_print_ok(idx) )
				print_notify_deregister_pid(idx);
		}
		
		return True;
	}
	else /* deregister a specific printer */
	{
		printername = lp_const_servicename(snum);
		pdb = get_print_db_byname(printername);
		if (!pdb)
			return False;
		tdb = pdb->tdb;
	}

	if (tdb_lock_bystring_with_timeout(tdb, NOTIFY_PID_LIST_KEY, 10) == -1) {
		DEBUG(0,("print_notify_register_pid: Failed to lock \
printer %s database\n", printername));
		if (pdb)
			release_print_db(pdb);
		return False;
	}

	data = get_printer_notify_pid_list( tdb, printername, True );

	/* Reduce refcount. Remove ourselves if zero. */

	for (i = 0; i < data.dsize; ) {
		if (IVAL(data.dptr,i) == mypid) {
			uint32 refcount = IVAL(data.dptr, i+4);

			refcount--;

			if (refcount == 0) {
				if (data.dsize - i > 8)
					memmove( &data.dptr[i], &data.dptr[i+8], data.dsize - i - 8);
				data.dsize -= 8;
				continue;
			}
			SIVAL(data.dptr, i+4, refcount);
		}

		i += 8;
	}

	if (data.dsize == 0)
		SAFE_FREE(data.dptr);

	/* Store back the record. */
	if (tdb_store_bystring(tdb, NOTIFY_PID_LIST_KEY, data, TDB_REPLACE) == -1) {
		DEBUG(0,("print_notify_register_pid: Failed to update pid \
list for printer %s\n", printername));
		goto done;
	}

	ret = True;

  done:

	tdb_unlock_bystring(tdb, NOTIFY_PID_LIST_KEY);
	if (pdb)
		release_print_db(pdb);
	SAFE_FREE(data.dptr);
	return ret;
}

/****************************************************************************
 Check if a jobid is valid. It is valid if it exists in the database.
****************************************************************************/

BOOL print_job_exists(const char* sharename, uint32 jobid)
{
	struct tdb_print_db *pdb = get_print_db_byname(sharename);
	BOOL ret;

	if (!pdb)
		return False;
	ret = tdb_exists(pdb->tdb, print_key(jobid));
	release_print_db(pdb);
	return ret;
}

/****************************************************************************
 Give the fd used for a jobid.
****************************************************************************/

int print_job_fd(const char* sharename, uint32 jobid)
{
	struct printjob *pjob = print_job_find(sharename, jobid);
	if (!pjob)
		return -1;
	/* don't allow another process to get this info - it is meaningless */
	if (pjob->pid != sys_getpid())
		return -1;
	return pjob->fd;
}

/****************************************************************************
 Give the filename used for a jobid.
 Only valid for the process doing the spooling and when the job
 has not been spooled.
****************************************************************************/

char *print_job_fname(const char* sharename, uint32 jobid)
{
	struct printjob *pjob = print_job_find(sharename, jobid);
	if (!pjob || pjob->spooled || pjob->pid != sys_getpid())
		return NULL;
	return pjob->filename;
}


/****************************************************************************
 Give the filename used for a jobid.
 Only valid for the process doing the spooling and when the job
 has not been spooled.
****************************************************************************/

NT_DEVICEMODE *print_job_devmode(const char* sharename, uint32 jobid)
{
	struct printjob *pjob = print_job_find(sharename, jobid);
	
	if ( !pjob )
		return NULL;
		
	return pjob->nt_devmode;
}

/****************************************************************************
 Set the place in the queue for a job.
****************************************************************************/

BOOL print_job_set_place(int snum, uint32 jobid, int place)
{
	DEBUG(2,("print_job_set_place not implemented yet\n"));
	return False;
}

/****************************************************************************
 Set the name of a job. Only possible for owner.
****************************************************************************/

BOOL print_job_set_name(int snum, uint32 jobid, char *name)
{
	const char* sharename = lp_const_servicename(snum);
	struct printjob *pjob;

	pjob = print_job_find(sharename, jobid);
	if (!pjob || pjob->pid != sys_getpid())
		return False;

	fstrcpy(pjob->jobname, name);
	return pjob_store(sharename, jobid, pjob);
}

/***************************************************************************
 Remove a jobid from the 'jobs changed' list.
***************************************************************************/

static BOOL remove_from_jobs_changed(const char* sharename, uint32 jobid)
{
	struct tdb_print_db *pdb = get_print_db_byname(sharename);
	TDB_DATA data, key;
	size_t job_count, i;
	BOOL ret = False;
	BOOL gotlock = False;

	if (!pdb) {
		return False;
	}

	ZERO_STRUCT(data);

	key = string_tdb_data("INFO/jobs_changed");

	if (tdb_chainlock_with_timeout(pdb->tdb, key, 5) == -1)
		goto out;

	gotlock = True;

	data = tdb_fetch(pdb->tdb, key);

	if (data.dptr == NULL || data.dsize == 0 || (data.dsize % 4 != 0))
		goto out;

	job_count = data.dsize / 4;
	for (i = 0; i < job_count; i++) {
		uint32 ch_jobid;

		ch_jobid = IVAL(data.dptr, i*4);
		if (ch_jobid == jobid) {
			if (i < job_count -1 )
				memmove(data.dptr + (i*4), data.dptr + (i*4) + 4, (job_count - i - 1)*4 );
			data.dsize -= 4;
			if (tdb_store(pdb->tdb, key, data, TDB_REPLACE) == -1)
				goto out;
			break;
		}
	}

	ret = True;
  out:

	if (gotlock)
		tdb_chainunlock(pdb->tdb, key);
	SAFE_FREE(data.dptr);
	release_print_db(pdb);
	if (ret)
		DEBUG(10,("remove_from_jobs_changed: removed jobid %u\n", (unsigned int)jobid ));
	else
		DEBUG(10,("remove_from_jobs_changed: Failed to remove jobid %u\n", (unsigned int)jobid ));
	return ret;
}

/****************************************************************************
 Delete a print job - don't update queue.
****************************************************************************/

static BOOL print_job_delete1(int snum, uint32 jobid)
{
	const char* sharename = lp_const_servicename(snum);
	struct printjob *pjob = print_job_find(sharename, jobid);
	int result = 0;
	struct printif *current_printif = get_printer_fns( snum );

	if (!pjob)
		return False;

	/*
	 * If already deleting just return.
	 */

	if (pjob->status == LPQ_DELETING)
		return True;

	/* Hrm - we need to be able to cope with deleting a job before it
	   has reached the spooler.  Just mark it as LPQ_DELETING and 
	   let the print_queue_update() code rmeove the record */
	   

	if (pjob->sysjob == -1) {
		DEBUG(5, ("attempt to delete job %u not seen by lpr\n", (unsigned int)jobid));
	}

	/* Set the tdb entry to be deleting. */

	pjob->status = LPQ_DELETING;
	pjob_store(sharename, jobid, pjob);

	if (pjob->spooled && pjob->sysjob != -1) 
	{
		result = (*(current_printif->job_delete))(
			PRINTERNAME(snum),
			lp_lprmcommand(snum), 
			pjob);

		/* Delete the tdb entry if the delete succeeded or the job hasn't
		   been spooled. */

		if (result == 0) {
			struct tdb_print_db *pdb = get_print_db_byname(sharename);
			int njobs = 1;

			if (!pdb)
				return False;
			pjob_delete(sharename, jobid);
			/* Ensure we keep a rough count of the number of total jobs... */
			tdb_change_int32_atomic(pdb->tdb, "INFO/total_jobs", &njobs, -1);
			release_print_db(pdb);
		}
	}

	remove_from_jobs_changed( sharename, jobid );

	return (result == 0);
}

/****************************************************************************
 Return true if the current user owns the print job.
****************************************************************************/

static BOOL is_owner(struct current_user *user, int snum, uint32 jobid)
{
	struct printjob *pjob = print_job_find(lp_const_servicename(snum), jobid);
	user_struct *vuser;

	if (!pjob || !user)
		return False;

	if ((vuser = get_valid_user_struct(user->vuid)) != NULL) {
		return strequal(pjob->user, vuser->user.smb_name);
	} else {
		return strequal(pjob->user, uidtoname(user->ut.uid));
	}
}

/****************************************************************************
 Delete a print job.
****************************************************************************/

BOOL print_job_delete(struct current_user *user, int snum, uint32 jobid, WERROR *errcode)
{
	const char* sharename = lp_const_servicename( snum );
	struct printjob *pjob;
	BOOL 	owner;
	char 	*fname;

	*errcode = WERR_OK;
		
	owner = is_owner(user, snum, jobid);
	
	/* Check access against security descriptor or whether the user
	   owns their job. */

	if (!owner && 
	    !print_access_check(user, snum, JOB_ACCESS_ADMINISTER)) {
		DEBUG(3, ("delete denied by security descriptor\n"));
		*errcode = WERR_ACCESS_DENIED;

		/* BEGIN_ADMIN_LOG */
		sys_adminlog( LOG_ERR, 
			      "Permission denied-- user not allowed to delete, \
pause, or resume print job. User name: %s. Printer name: %s.",
			      uidtoname(user->ut.uid), PRINTERNAME(snum) );
		/* END_ADMIN_LOG */

		return False;
	}

	/* 
	 * get the spooled filename of the print job
	 * if this works, then the file has not been spooled
	 * to the underlying print system.  Just delete the 
	 * spool file & return.
	 */
	 
	if ( (fname = print_job_fname( sharename, jobid )) != NULL )
	{
		/* remove the spool file */
		DEBUG(10,("print_job_delete: Removing spool file [%s]\n", fname ));
		if ( unlink( fname ) == -1 ) {
			*errcode = map_werror_from_unix(errno);
			return False;
		}
	}
	
	if (!print_job_delete1(snum, jobid)) {
		*errcode = WERR_ACCESS_DENIED;
		return False;
	}

	/* force update the database and say the delete failed if the
           job still exists */

	print_queue_update(snum, True);
	
	pjob = print_job_find(sharename, jobid);
	if ( pjob && (pjob->status != LPQ_DELETING) )
		*errcode = WERR_ACCESS_DENIED;

	return (pjob == NULL );
}

/****************************************************************************
 Pause a job.
****************************************************************************/

BOOL print_job_pause(struct current_user *user, int snum, uint32 jobid, WERROR *errcode)
{
	const char* sharename = lp_const_servicename(snum);
	struct printjob *pjob;
	int ret = -1;
	struct printif *current_printif = get_printer_fns( snum );

	pjob = print_job_find(sharename, jobid);
	
	if (!pjob || !user) {
		DEBUG(10, ("print_job_pause: no pjob or user for jobid %u\n",
			(unsigned int)jobid ));
		return False;
	}

	if (!pjob->spooled || pjob->sysjob == -1) {
		DEBUG(10, ("print_job_pause: not spooled or bad sysjob = %d for jobid %u\n",
			(int)pjob->sysjob, (unsigned int)jobid ));
		return False;
	}

	if (!is_owner(user, snum, jobid) &&
	    !print_access_check(user, snum, JOB_ACCESS_ADMINISTER)) {
		DEBUG(3, ("pause denied by security descriptor\n"));

		/* BEGIN_ADMIN_LOG */
		sys_adminlog( LOG_ERR, 
			"Permission denied-- user not allowed to delete, \
pause, or resume print job. User name: %s. Printer name: %s.",
				uidtoname(user->ut.uid), PRINTERNAME(snum) );
		/* END_ADMIN_LOG */

		*errcode = WERR_ACCESS_DENIED;
		return False;
	}

	/* need to pause the spooled entry */
	ret = (*(current_printif->job_pause))(snum, pjob);

	if (ret != 0) {
		*errcode = WERR_INVALID_PARAM;
		return False;
	}

	/* force update the database */
	print_cache_flush(snum);

	/* Send a printer notify message */

	notify_job_status(sharename, jobid, JOB_STATUS_PAUSED);

	/* how do we tell if this succeeded? */

	return True;
}

/****************************************************************************
 Resume a job.
****************************************************************************/

BOOL print_job_resume(struct current_user *user, int snum, uint32 jobid, WERROR *errcode)
{
	const char *sharename = lp_const_servicename(snum);
	struct printjob *pjob;
	int ret;
	struct printif *current_printif = get_printer_fns( snum );

	pjob = print_job_find(sharename, jobid);
	
	if (!pjob || !user) {
		DEBUG(10, ("print_job_resume: no pjob or user for jobid %u\n",
			(unsigned int)jobid ));
		return False;
	}

	if (!pjob->spooled || pjob->sysjob == -1) {
		DEBUG(10, ("print_job_resume: not spooled or bad sysjob = %d for jobid %u\n",
			(int)pjob->sysjob, (unsigned int)jobid ));
		return False;
	}

	if (!is_owner(user, snum, jobid) &&
	    !print_access_check(user, snum, JOB_ACCESS_ADMINISTER)) {
		DEBUG(3, ("resume denied by security descriptor\n"));
		*errcode = WERR_ACCESS_DENIED;

		/* BEGIN_ADMIN_LOG */
		sys_adminlog( LOG_ERR, 
			 "Permission denied-- user not allowed to delete, \
pause, or resume print job. User name: %s. Printer name: %s.",
			uidtoname(user->ut.uid), PRINTERNAME(snum) );
		/* END_ADMIN_LOG */
		return False;
	}

	ret = (*(current_printif->job_resume))(snum, pjob);

	if (ret != 0) {
		*errcode = WERR_INVALID_PARAM;
		return False;
	}

	/* force update the database */
	print_cache_flush(snum);

	/* Send a printer notify message */

	notify_job_status(sharename, jobid, JOB_STATUS_QUEUED);

	return True;
}

/****************************************************************************
 Write to a print file.
****************************************************************************/

ssize_t print_job_write(int snum, uint32 jobid, const char *buf, SMB_OFF_T pos, size_t size)
{
	const char* sharename = lp_const_servicename(snum);
	int return_code;
	struct printjob *pjob;

	pjob = print_job_find(sharename, jobid);

	if (!pjob)
		return -1;
	/* don't allow another process to get this info - it is meaningless */
	if (pjob->pid != sys_getpid())
		return -1;

	return_code = write_data_at_offset(pjob->fd, buf, size, pos);

	if (return_code>0) {
		pjob->size += size;
		pjob_store(sharename, jobid, pjob);
	}
	return return_code;
}

/****************************************************************************
 Get the queue status - do not update if db is out of date.
****************************************************************************/

static int get_queue_status(const char* sharename, print_status_struct *status)
{
	fstring keystr;
	TDB_DATA data;
	struct tdb_print_db *pdb = get_print_db_byname(sharename);
	int len;

	if (status) {
		ZERO_STRUCTP(status);
	}

	if (!pdb)
		return 0;

	if (status) {
		fstr_sprintf(keystr, "STATUS/%s", sharename);
		data = tdb_fetch(pdb->tdb, string_tdb_data(keystr));
		if (data.dptr) {
			if (data.dsize == sizeof(print_status_struct))
				/* this memcpy is ok since the status struct was 
				   not packed before storing it in the tdb */
				memcpy(status, data.dptr, sizeof(print_status_struct));
			SAFE_FREE(data.dptr);
		}
	}
	len = tdb_fetch_int32(pdb->tdb, "INFO/total_jobs");
	release_print_db(pdb);
	return (len == -1 ? 0 : len);
}

/****************************************************************************
 Determine the number of jobs in a queue.
****************************************************************************/

int print_queue_length(int snum, print_status_struct *pstatus)
{
	const char* sharename = lp_const_servicename( snum );
	print_status_struct status;
	int len;

	ZERO_STRUCT( status );
 
	/* make sure the database is up to date */
	if (print_cache_expired(lp_const_servicename(snum), True))
		print_queue_update(snum, False);
 
	/* also fetch the queue status */
	memset(&status, 0, sizeof(status));
	len = get_queue_status(sharename, &status);

	if (pstatus)
		*pstatus = status;

	return len;
}

/***************************************************************************
 Allocate a jobid. Hold the lock for as short a time as possible.
***************************************************************************/

static BOOL allocate_print_jobid(struct tdb_print_db *pdb, int snum, const char *sharename, uint32 *pjobid)
{
	int i;
	uint32 jobid;

	*pjobid = (uint32)-1;

	for (i = 0; i < 3; i++) {
		/* Lock the database - only wait 20 seconds. */
		if (tdb_lock_bystring_with_timeout(pdb->tdb, "INFO/nextjob", 20) == -1) {
			DEBUG(0,("allocate_print_jobid: failed to lock printing database %s\n", sharename));
			return False;
		}

		if (!tdb_fetch_uint32(pdb->tdb, "INFO/nextjob", &jobid)) {
			if (tdb_error(pdb->tdb) != TDB_ERR_NOEXIST) {
				DEBUG(0, ("allocate_print_jobid: failed to fetch INFO/nextjob for print queue %s\n",
					sharename));
				return False;
			}
			jobid = 0;
		}

		jobid = NEXT_JOBID(jobid);

		if (tdb_store_int32(pdb->tdb, "INFO/nextjob", jobid)==-1) {
			DEBUG(3, ("allocate_print_jobid: failed to store INFO/nextjob.\n"));
			tdb_unlock_bystring(pdb->tdb, "INFO/nextjob");
			return False;
		}

		/* We've finished with the INFO/nextjob lock. */
		tdb_unlock_bystring(pdb->tdb, "INFO/nextjob");
				
		if (!print_job_exists(sharename, jobid))
			break;
	}

	if (i > 2) {
		DEBUG(0, ("allocate_print_jobid: failed to allocate a print job for queue %s\n",
			sharename));
		/* Probably full... */
		errno = ENOSPC;
		return False;
	}

	/* Store a dummy placeholder. */
	{
		TDB_DATA dum;
		dum.dptr = NULL;
		dum.dsize = 0;
		if (tdb_store(pdb->tdb, print_key(jobid), dum, TDB_INSERT) == -1) {
			DEBUG(3, ("allocate_print_jobid: jobid (%d) failed to store placeholder.\n",
				jobid ));
			return False;
		}
	}

	*pjobid = jobid;
	return True;
}

/***************************************************************************
 Append a jobid to the 'jobs changed' list.
***************************************************************************/

static BOOL add_to_jobs_changed(struct tdb_print_db *pdb, uint32 jobid)
{
	TDB_DATA data;
	uint32 store_jobid;

	SIVAL(&store_jobid, 0, jobid);
	data.dptr = (char *)&store_jobid;
	data.dsize = 4;

	DEBUG(10,("add_to_jobs_changed: Added jobid %u\n", (unsigned int)jobid ));

	return (tdb_append(pdb->tdb, string_tdb_data("INFO/jobs_changed"),
			   data) == 0);
}

/***************************************************************************
 Start spooling a job - return the jobid.
***************************************************************************/

uint32 print_job_start(struct current_user *user, int snum, char *jobname, NT_DEVICEMODE *nt_devmode )
{
	uint32 jobid;
	char *path;
	struct printjob pjob;
	user_struct *vuser;
	const char *sharename = lp_const_servicename(snum);
	struct tdb_print_db *pdb = get_print_db_byname(sharename);
	int njobs;

	errno = 0;

	if (!pdb)
		return (uint32)-1;

	if (!print_access_check(user, snum, PRINTER_ACCESS_USE)) {
		DEBUG(3, ("print_job_start: job start denied by security descriptor\n"));
		release_print_db(pdb);
		return (uint32)-1;
	}

	if (!print_time_access_check(snum)) {
		DEBUG(3, ("print_job_start: job start denied by time check\n"));
		release_print_db(pdb);
		return (uint32)-1;
	}

	path = lp_pathname(snum);

	/* see if we have sufficient disk space */
	if (lp_minprintspace(snum)) {
		SMB_BIG_UINT dspace, dsize;
		if (sys_fsusage(path, &dspace, &dsize) == 0 &&
		    dspace < 2*(SMB_BIG_UINT)lp_minprintspace(snum)) {
			DEBUG(3, ("print_job_start: disk space check failed.\n"));
			release_print_db(pdb);
			errno = ENOSPC;
			return (uint32)-1;
		}
	}

	/* for autoloaded printers, check that the printcap entry still exists */
	if (lp_autoloaded(snum) && !pcap_printername_ok(lp_const_servicename(snum))) {
		DEBUG(3, ("print_job_start: printer name %s check failed.\n", lp_const_servicename(snum) ));
		release_print_db(pdb);
		errno = ENOENT;
		return (uint32)-1;
	}

	/* Insure the maximum queue size is not violated */
	if ((njobs = print_queue_length(snum,NULL)) > lp_maxprintjobs(snum)) {
		DEBUG(3, ("print_job_start: Queue %s number of jobs (%d) larger than max printjobs per queue (%d).\n",
			sharename, njobs, lp_maxprintjobs(snum) ));
		release_print_db(pdb);
		errno = ENOSPC;
		return (uint32)-1;
	}

	DEBUG(10,("print_job_start: Queue %s number of jobs (%d), max printjobs = %d\n",
		sharename, njobs, lp_maxprintjobs(snum) ));

	if (!allocate_print_jobid(pdb, snum, sharename, &jobid))
		goto fail;

	/* create the database entry */
	
	ZERO_STRUCT(pjob);
	
	pjob.pid = sys_getpid();
	pjob.sysjob = -1;
	pjob.fd = -1;
	pjob.starttime = time(NULL);
	pjob.status = LPQ_SPOOLING;
	pjob.size = 0;
	pjob.spooled = False;
	pjob.smbjob = True;
	pjob.nt_devmode = nt_devmode;
	
	fstrcpy(pjob.jobname, jobname);

	if ((vuser = get_valid_user_struct(user->vuid)) != NULL) {
		fstrcpy(pjob.user, vuser->user.smb_name);
	} else {
		fstrcpy(pjob.user, uidtoname(user->ut.uid));
	}

	fstrcpy(pjob.queuename, lp_const_servicename(snum));

	/* we have a job entry - now create the spool file */
	slprintf(pjob.filename, sizeof(pjob.filename)-1, "%s/%s%.8u.XXXXXX", 
		 path, PRINT_SPOOL_PREFIX, (unsigned int)jobid);
	pjob.fd = smb_mkstemp(pjob.filename);

	if (pjob.fd == -1) {
		if (errno == EACCES) {
			/* Common setup error, force a report. */
			DEBUG(0, ("print_job_start: insufficient permissions \
to open spool file %s.\n", pjob.filename));
		} else {
			/* Normal case, report at level 3 and above. */
			DEBUG(3, ("print_job_start: can't open spool file %s,\n", pjob.filename));
			DEBUGADD(3, ("errno = %d (%s).\n", errno, strerror(errno)));
		}
		goto fail;
	}

	pjob_store(sharename, jobid, &pjob);

	/* Update the 'jobs changed' entry used by print_queue_status. */
	add_to_jobs_changed(pdb, jobid);

	/* Ensure we keep a rough count of the number of total jobs... */
	tdb_change_int32_atomic(pdb->tdb, "INFO/total_jobs", &njobs, 1);

	release_print_db(pdb);

	return jobid;

 fail:
	if (jobid != -1)
		pjob_delete(sharename, jobid);

	release_print_db(pdb);

	DEBUG(3, ("print_job_start: returning fail. Error = %s\n", strerror(errno) ));
	return (uint32)-1;
}

/****************************************************************************
 Update the number of pages spooled to jobid
****************************************************************************/

void print_job_endpage(int snum, uint32 jobid)
{
	const char* sharename = lp_const_servicename(snum);
	struct printjob *pjob;

	pjob = print_job_find(sharename, jobid);
	if (!pjob)
		return;
	/* don't allow another process to get this info - it is meaningless */
	if (pjob->pid != sys_getpid())
		return;

	pjob->page_count++;
	pjob_store(sharename, jobid, pjob);
}

/****************************************************************************
 Print a file - called on closing the file. This spools the job.
 If normal close is false then we're tearing down the jobs - treat as an
 error.
****************************************************************************/

BOOL print_job_end(int snum, uint32 jobid, enum file_close_type close_type)
{
	const char* sharename = lp_const_servicename(snum);
	struct printjob *pjob;
	int ret;
	SMB_STRUCT_STAT sbuf;
	struct printif *current_printif = get_printer_fns( snum );

	pjob = print_job_find(sharename, jobid);

	if (!pjob)
		return False;

	if (pjob->spooled || pjob->pid != sys_getpid())
		return False;

	if ((close_type == NORMAL_CLOSE || close_type == SHUTDOWN_CLOSE) &&
				(sys_fstat(pjob->fd, &sbuf) == 0)) {
		pjob->size = sbuf.st_size;
		close(pjob->fd);
		pjob->fd = -1;
	} else {

		/* 
		 * Not a normal close or we couldn't stat the job file,
		 * so something has gone wrong. Cleanup.
		 */
		close(pjob->fd);
		pjob->fd = -1;
		DEBUG(3,("print_job_end: failed to stat file for jobid %d\n", jobid ));
		goto fail;
	}

	/* Technically, this is not quite right. If the printer has a separator
	 * page turned on, the NT spooler prints the separator page even if the
	 * print job is 0 bytes. 010215 JRR */
	if (pjob->size == 0 || pjob->status == LPQ_DELETING) {
		/* don't bother spooling empty files or something being deleted. */
		DEBUG(5,("print_job_end: canceling spool of %s (%s)\n",
			pjob->filename, pjob->size ? "deleted" : "zero length" ));
		unlink(pjob->filename);
		pjob_delete(sharename, jobid);
		return True;
	}

	pjob->smbjob = jobid;

	ret = (*(current_printif->job_submit))(snum, pjob);

	if (ret)
		goto fail;

	/* The print job has been sucessfully handed over to the back-end */
	
	pjob->spooled = True;
	pjob->status = LPQ_QUEUED;
	pjob_store(sharename, jobid, pjob);
	
	/* make sure the database is up to date */
	if (print_cache_expired(lp_const_servicename(snum), True))
		print_queue_update(snum, False);
	
	return True;

fail:

	/* The print job was not succesfully started. Cleanup */
	/* Still need to add proper error return propagation! 010122:JRR */
	unlink(pjob->filename);
	pjob_delete(sharename, jobid);
	return False;
}

/****************************************************************************
 Get a snapshot of jobs in the system without traversing.
****************************************************************************/

static BOOL get_stored_queue_info(struct tdb_print_db *pdb, int snum, int *pcount, print_queue_struct **ppqueue)
{
	TDB_DATA data, cgdata;
	print_queue_struct *queue = NULL;
	uint32 qcount = 0;
	uint32 extra_count = 0;
	int total_count = 0;
	size_t len = 0;
	uint32 i;
	int max_reported_jobs = lp_max_reported_jobs(snum);
	BOOL ret = False;
	const char* sharename = lp_servicename(snum);

	/* make sure the database is up to date */
	if (print_cache_expired(lp_const_servicename(snum), True))
		print_queue_update(snum, False);
 
	*pcount = 0;
	*ppqueue = NULL;

	ZERO_STRUCT(data);
	ZERO_STRUCT(cgdata);

	/* Get the stored queue data. */
	data = tdb_fetch(pdb->tdb, string_tdb_data("INFO/linear_queue_array"));
	
	if (data.dptr && data.dsize >= sizeof(qcount))
		len += tdb_unpack(data.dptr + len, data.dsize - len, "d", &qcount);
		
	/* Get the changed jobs list. */
	cgdata = tdb_fetch(pdb->tdb, string_tdb_data("INFO/jobs_changed"));
	if (cgdata.dptr != NULL && (cgdata.dsize % 4 == 0))
		extra_count = cgdata.dsize/4;

	DEBUG(5,("get_stored_queue_info: qcount = %u, extra_count = %u\n", (unsigned int)qcount, (unsigned int)extra_count));

	/* Allocate the queue size. */
	if (qcount == 0 && extra_count == 0)
		goto out;

	if ((queue = SMB_MALLOC_ARRAY(print_queue_struct, qcount + extra_count)) == NULL)
		goto out;

	/* Retrieve the linearised queue data. */

	for( i  = 0; i < qcount; i++) {
		uint32 qjob, qsize, qpage_count, qstatus, qpriority, qtime;
		len += tdb_unpack(data.dptr + len, data.dsize - len, "ddddddff",
				&qjob,
				&qsize,
				&qpage_count,
				&qstatus,
				&qpriority,
				&qtime,
				queue[i].fs_user,
				queue[i].fs_file);
		queue[i].job = qjob;
		queue[i].size = qsize;
		queue[i].page_count = qpage_count;
		queue[i].status = qstatus;
		queue[i].priority = qpriority;
		queue[i].time = qtime;
	}

	total_count = qcount;

	/* Add in the changed jobids. */
	for( i  = 0; i < extra_count; i++) {
		uint32 jobid;
		struct printjob *pjob;

		jobid = IVAL(cgdata.dptr, i*4);
		DEBUG(5,("get_stored_queue_info: changed job = %u\n", (unsigned int)jobid));
		pjob = print_job_find(lp_const_servicename(snum), jobid);
		if (!pjob) {
			DEBUG(5,("get_stored_queue_info: failed to find changed job = %u\n", (unsigned int)jobid));
			remove_from_jobs_changed(sharename, jobid);
			continue;
		}

		queue[total_count].job = jobid;
		queue[total_count].size = pjob->size;
		queue[total_count].page_count = pjob->page_count;
		queue[total_count].status = pjob->status;
		queue[total_count].priority = 1;
		queue[total_count].time = pjob->starttime;
		fstrcpy(queue[total_count].fs_user, pjob->user);
		fstrcpy(queue[total_count].fs_file, pjob->jobname);
		total_count++;
	}

	/* Sort the queue by submission time otherwise they are displayed
	   in hash order. */

	qsort(queue, total_count, sizeof(print_queue_struct), QSORT_CAST(printjob_comp));

	DEBUG(5,("get_stored_queue_info: total_count = %u\n", (unsigned int)total_count));

	if (max_reported_jobs && total_count > max_reported_jobs)
		total_count = max_reported_jobs;

	*ppqueue = queue;
	*pcount = total_count;

	ret = True;

  out:

	SAFE_FREE(data.dptr);
	SAFE_FREE(cgdata.dptr);
	return ret;
}

/****************************************************************************
 Get a printer queue listing.
 set queue = NULL and status = NULL if you just want to update the cache
****************************************************************************/

int print_queue_status(int snum, 
		       print_queue_struct **ppqueue,
		       print_status_struct *status)
{
	fstring keystr;
	TDB_DATA data, key;
	const char *sharename;
	struct tdb_print_db *pdb;
	int count = 0;

	/* make sure the database is up to date */

	if (print_cache_expired(lp_const_servicename(snum), True))
		print_queue_update(snum, False);

	/* return if we are done */
	if ( !ppqueue || !status )
		return 0;

	*ppqueue = NULL;
	sharename = lp_const_servicename(snum);
	pdb = get_print_db_byname(sharename);

	if (!pdb)
		return 0;

	/*
	 * Fetch the queue status.  We must do this first, as there may
	 * be no jobs in the queue.
	 */

	ZERO_STRUCTP(status);
	slprintf(keystr, sizeof(keystr)-1, "STATUS/%s", sharename);
	key.dptr = keystr;
	key.dsize = strlen(keystr);
	data = tdb_fetch(pdb->tdb, key);
	if (data.dptr) {
		if (data.dsize == sizeof(*status)) {
			/* this memcpy is ok since the status struct was 
			   not packed before storing it in the tdb */
			memcpy(status, data.dptr, sizeof(*status));
		}
		SAFE_FREE(data.dptr);
	}

	/*
	 * Now, fetch the print queue information.  We first count the number
	 * of entries, and then only retrieve the queue if necessary.
	 */

	if (!get_stored_queue_info(pdb, snum, &count, ppqueue)) {
		release_print_db(pdb);
		return 0;
	}

	release_print_db(pdb);
	return count;
}

/****************************************************************************
 Pause a queue.
****************************************************************************/

BOOL print_queue_pause(struct current_user *user, int snum, WERROR *errcode)
{
	int ret;
	struct printif *current_printif = get_printer_fns( snum );
	
	if (!print_access_check(user, snum, PRINTER_ACCESS_ADMINISTER)) {
		*errcode = WERR_ACCESS_DENIED;
		return False;
	}
	

	become_root();
		
	ret = (*(current_printif->queue_pause))(snum);

	unbecome_root();
		
	if (ret != 0) {
		*errcode = WERR_INVALID_PARAM;
		return False;
	}

	/* force update the database */
	print_cache_flush(snum);

	/* Send a printer notify message */

	notify_printer_status(snum, PRINTER_STATUS_PAUSED);

	return True;
}

/****************************************************************************
 Resume a queue.
****************************************************************************/

BOOL print_queue_resume(struct current_user *user, int snum, WERROR *errcode)
{
	int ret;
	struct printif *current_printif = get_printer_fns( snum );

	if (!print_access_check(user, snum, PRINTER_ACCESS_ADMINISTER)) {
		*errcode = WERR_ACCESS_DENIED;
		return False;
	}
	
	become_root();
		
	ret = (*(current_printif->queue_resume))(snum);

	unbecome_root();
		
	if (ret != 0) {
		*errcode = WERR_INVALID_PARAM;
		return False;
	}

	/* make sure the database is up to date */
	if (print_cache_expired(lp_const_servicename(snum), True))
		print_queue_update(snum, True);

	/* Send a printer notify message */

	notify_printer_status(snum, PRINTER_STATUS_OK);

	return True;
}

/****************************************************************************
 Purge a queue - implemented by deleting all jobs that we can delete.
****************************************************************************/

BOOL print_queue_purge(struct current_user *user, int snum, WERROR *errcode)
{
	print_queue_struct *queue;
	print_status_struct status;
	int njobs, i;
	BOOL can_job_admin;

	/* Force and update so the count is accurate (i.e. not a cached count) */
	print_queue_update(snum, True);
	
	can_job_admin = print_access_check(user, snum, JOB_ACCESS_ADMINISTER);
	njobs = print_queue_status(snum, &queue, &status);
	
	if ( can_job_admin )
		become_root();

	for (i=0;i<njobs;i++) {
		BOOL owner = is_owner(user, snum, queue[i].job);

		if (owner || can_job_admin) {
			print_job_delete1(snum, queue[i].job);
		}
	}
	
	if ( can_job_admin )
		unbecome_root();

	/* update the cache */
	print_queue_update( snum, True );

	SAFE_FREE(queue);

	return True;
}

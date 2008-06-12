/* 
   Unix SMB/CIFS implementation.
   printing backend routines for smbd - using files_struct rather
   than only snum
   Copyright (C) Andrew Tridgell 1992-2000
   
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

extern struct current_user current_user;

/***************************************************************************
open a print file and setup a fsp for it. This is a wrapper around
print_job_start().
***************************************************************************/

files_struct *print_fsp_open(connection_struct *conn, const char *fname)
{
	int jobid;
	SMB_STRUCT_STAT sbuf;
	files_struct *fsp = file_new(conn);
	fstring name;

	if(!fsp)
		return NULL;

	fstrcpy( name, "Remote Downlevel Document");
	if (fname) {
		const char *p = strrchr(fname, '/');
		fstrcat(name, " ");
		if (!p) {
			p = fname;
		}
		fstrcat(name, p);
	}

	jobid = print_job_start(&current_user, SNUM(conn), name, NULL);
	if (jobid == -1) {
		file_free(fsp);
		return NULL;
	}

	/* Convert to RAP id. */
	fsp->rap_print_jobid = pjobid_to_rap(lp_const_servicename(SNUM(conn)), jobid);
	if (fsp->rap_print_jobid == 0) {
		/* We need to delete the entry in the tdb. */
		pjob_delete(lp_const_servicename(SNUM(conn)), jobid);
		file_free(fsp);
		return NULL;
	}

	/* setup a full fsp */
	fsp->fh->fd = print_job_fd(lp_const_servicename(SNUM(conn)),jobid);
	GetTimeOfDay(&fsp->open_time);
	fsp->vuid = current_user.vuid;
	fsp->fh->pos = -1;
	fsp->can_lock = True;
	fsp->can_read = False;
	fsp->access_mask = FILE_GENERIC_WRITE;
	fsp->can_write = True;
	fsp->print_file = True;
	fsp->modified = False;
	fsp->oplock_type = NO_OPLOCK;
	fsp->sent_oplock_break = NO_BREAK_SENT;
	fsp->is_directory = False;
	string_set(&fsp->fsp_name,print_job_fname(lp_const_servicename(SNUM(conn)),jobid));
	fsp->wbmpx_ptr = NULL;      
	fsp->wcp = NULL; 
	SMB_VFS_FSTAT(fsp,fsp->fh->fd, &sbuf);
	fsp->mode = sbuf.st_mode;
	fsp->inode = sbuf.st_ino;
	fsp->dev = sbuf.st_dev;

	conn->num_files_open++;

	return fsp;
}

/****************************************************************************
 Print a file - called on closing the file.
****************************************************************************/

void print_fsp_end(files_struct *fsp, enum file_close_type close_type)
{
	uint32 jobid;
	fstring sharename;

	if (fsp->fh->private_options & FILE_DELETE_ON_CLOSE) {
		/*
		 * Truncate the job. print_job_end will take
		 * care of deleting it for us. JRA.
		 */
		sys_ftruncate(fsp->fh->fd, 0);
	}

	if (fsp->fsp_name) {
		string_free(&fsp->fsp_name);
	}

	if (!rap_to_pjobid(fsp->rap_print_jobid, sharename, &jobid)) {
		DEBUG(3,("print_fsp_end: Unable to convert RAP jobid %u to print jobid.\n",
			(unsigned int)fsp->rap_print_jobid ));
		return;
	}

	print_job_end(SNUM(fsp->conn),jobid, close_type);
}

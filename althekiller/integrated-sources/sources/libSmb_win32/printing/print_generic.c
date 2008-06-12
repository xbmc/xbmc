/* 
   Unix SMB/CIFS implementation.
   printing command routines
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
#include "printing.h"


/****************************************************************************
run a given print command 
a null terminated list of value/substitute pairs is provided
for local substitution strings
****************************************************************************/
static int print_run_command(int snum, const char* printername, BOOL do_sub,
			     const char *command, int *outfd, ...)
{

	pstring syscmd;
	char *arg;
	int ret;
	va_list ap;
	va_start(ap, outfd);

	/* check for a valid system printername and valid command to run */

	if ( !printername || !*printername ) 
		return -1;

	if (!command || !*command) 
		return -1;

	pstrcpy(syscmd, command);

	while ((arg = va_arg(ap, char *))) {
		char *value = va_arg(ap,char *);
		pstring_sub(syscmd, arg, value);
	}
	va_end(ap);
  
	pstring_sub( syscmd, "%p", printername );

	if ( do_sub && snum != -1 )
		standard_sub_snum(snum,syscmd,sizeof(syscmd));
		
	ret = smbrun(syscmd,outfd);

	DEBUG(3,("Running the command `%s' gave %d\n",syscmd,ret));

	return ret;
}


/****************************************************************************
delete a print job
****************************************************************************/
static int generic_job_delete( const char *sharename, const char *lprm_command, struct printjob *pjob)
{
	fstring jobstr;

	/* need to delete the spooled entry */
	slprintf(jobstr, sizeof(jobstr)-1, "%d", pjob->sysjob);
	return print_run_command( -1, sharename, False, lprm_command, NULL,
		   "%j", jobstr,
		   "%T", http_timestring(pjob->starttime),
		   NULL);
}

/****************************************************************************
pause a job
****************************************************************************/
static int generic_job_pause(int snum, struct printjob *pjob)
{
	fstring jobstr;
	
	/* need to pause the spooled entry */
	slprintf(jobstr, sizeof(jobstr)-1, "%d", pjob->sysjob);
	return print_run_command(snum, PRINTERNAME(snum), True,
				 lp_lppausecommand(snum), NULL,
				 "%j", jobstr,
				 NULL);
}

/****************************************************************************
resume a job
****************************************************************************/
static int generic_job_resume(int snum, struct printjob *pjob)
{
	fstring jobstr;
	
	/* need to pause the spooled entry */
	slprintf(jobstr, sizeof(jobstr)-1, "%d", pjob->sysjob);
	return print_run_command(snum, PRINTERNAME(snum), True,
				 lp_lpresumecommand(snum), NULL,
				 "%j", jobstr,
				 NULL);
}

/****************************************************************************
 Submit a file for printing - called from print_job_end()
****************************************************************************/

static int generic_job_submit(int snum, struct printjob *pjob)
{
	int ret;
	pstring current_directory;
	pstring print_directory;
	char *wd, *p;
	pstring jobname;
	fstring job_page_count, job_size;

	/* we print from the directory path to give the best chance of
           parsing the lpq output */
	wd = sys_getwd(current_directory);
	if (!wd)
		return 0;

	pstrcpy(print_directory, pjob->filename);
	p = strrchr_m(print_directory,'/');
	if (!p)
		return 0;
	*p++ = 0;

	if (chdir(print_directory) != 0)
		return 0;

	pstrcpy(jobname, pjob->jobname);
	pstring_sub(jobname, "'", "_");
	slprintf(job_page_count, sizeof(job_page_count)-1, "%d", pjob->page_count);
	slprintf(job_size, sizeof(job_size)-1, "%lu", (unsigned long)pjob->size);

	/* send it to the system spooler */
	ret = print_run_command(snum, PRINTERNAME(snum), True,
			lp_printcommand(snum), NULL,
			"%s", p,
			"%J", jobname,
			"%f", p,
			"%z", job_size,
			"%c", job_page_count,
			NULL);

	chdir(wd);

        return ret;
}


/****************************************************************************
get the current list of queued jobs
****************************************************************************/
static int generic_queue_get(const char *printer_name, 
                             enum printing_types printing_type,
                             char *lpq_command,
                             print_queue_struct **q, 
                             print_status_struct *status)
{
	char **qlines;
	int fd;
	int numlines, i, qcount;
	print_queue_struct *queue = NULL;
	
	/* never do substitution when running the 'lpq command' since we can't
	   get it rigt when using the background update daemon.  Make the caller 
	   do it before passing off the command string to us here. */

	print_run_command(-1, printer_name, False, lpq_command, &fd, NULL);

	if (fd == -1) {
		DEBUG(5,("generic_queue_get: Can't read print queue status for printer %s\n",
			printer_name ));
		return 0;
	}
	
	numlines = 0;
	qlines = fd_lines_load(fd, &numlines,0);
	close(fd);

	/* turn the lpq output into a series of job structures */
	qcount = 0;
	ZERO_STRUCTP(status);
	if (numlines && qlines) {
		queue = SMB_MALLOC_ARRAY(print_queue_struct, numlines+1);
		if (!queue) {
			file_lines_free(qlines);
			*q = NULL;
			return 0;
		}
		memset(queue, '\0', sizeof(print_queue_struct)*(numlines+1));

		for (i=0; i<numlines; i++) {
			/* parse the line */
			if (parse_lpq_entry(printing_type,qlines[i],
					    &queue[qcount],status,qcount==0)) {
				qcount++;
			}
		}		
	}

	file_lines_free(qlines);
        *q = queue;
	return qcount;
}

/****************************************************************************
 pause a queue
****************************************************************************/
static int generic_queue_pause(int snum)
{
	return print_run_command(snum, PRINTERNAME(snum), True, lp_queuepausecommand(snum), NULL, NULL);
}

/****************************************************************************
 resume a queue
****************************************************************************/
static int generic_queue_resume(int snum)
{
	return print_run_command(snum, PRINTERNAME(snum), True, lp_queueresumecommand(snum), NULL, NULL);
}

/****************************************************************************
 * Generic printing interface definitions...
 ***************************************************************************/

struct printif	generic_printif =
{
	DEFAULT_PRINTING,
	generic_queue_get,
	generic_queue_pause,
	generic_queue_resume,
	generic_job_delete,
	generic_job_pause,
	generic_job_resume,
	generic_job_submit,
};


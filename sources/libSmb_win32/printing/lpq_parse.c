/* 
   Unix SMB/CIFS implementation.
   lpq parsing routines
   Copyright (C) Andrew Tridgell 2000
   
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

static const char *Months[13] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec", "Err"};


/*******************************************************************
 Process time fields
********************************************************************/

static time_t EntryTime(fstring tok[], int ptr, int count, int minimum)
{
	time_t jobtime,jobtime1;

	jobtime = time(NULL);		/* default case: take current time */
	if (count >= minimum) {
		struct tm *t;
		int i, day, hour, min, sec;
		char   *c;

		for (i=0; i<13; i++) {
			if (!strncmp(tok[ptr], Months[i],3)) {
				break; /* Find month */
			}
		}

		if (i<12) {
			t = localtime(&jobtime);
			if (!t) {
				return (time_t)-1;
			}
			day = atoi(tok[ptr+1]);
			c=(char *)(tok[ptr+2]);
			*(c+2)=0;
			hour = atoi(c);
			*(c+5)=0;
			min = atoi(c+3);
			if(*(c+6) != 0) {
				sec = atoi(c+6);
			} else {
				sec=0;
			}

			if ((t->tm_mon < i)|| ((t->tm_mon == i)&&
					((t->tm_mday < day)||
					((t->tm_mday == day)&&
					(t->tm_hour*60+t->tm_min < hour*60+min))))) {
				t->tm_year--;		/* last year's print job */
			}

			t->tm_mon = i;
			t->tm_mday = day;
			t->tm_hour = hour;
			t->tm_min = min;
			t->tm_sec = sec;
			jobtime1 = mktime(t);
			if (jobtime1 != (time_t)-1) {
				jobtime = jobtime1;
			}
		}
	}
	return jobtime;
}

/****************************************************************************
parse a lpq line

here is an example of lpq output under bsd

Warning: no daemon present
Rank   Owner      Job  Files                                 Total Size
1st    tridge     148  README                                8096 bytes

here is an example of lpq output under osf/1

Warning: no daemon present
Rank   Pri Owner      Job  Files                             Total Size
1st    0   tridge     148  README                            8096 bytes


<allan@umich.edu> June 30, 1998.
Modified to handle file names with spaces, like the parse_lpq_lprng code
further below.
****************************************************************************/

static BOOL parse_lpq_bsd(char *line,print_queue_struct *buf,BOOL first)
{
#ifdef	OSF1
#define	RANKTOK	0
#define	PRIOTOK 1
#define	USERTOK 2
#define	JOBTOK	3
#define	FILETOK	4
#define	TOTALTOK (count - 2)
#define	NTOK	6
#define	MAXTOK	128
#else	/* OSF1 */
#define	RANKTOK	0
#define	USERTOK 1
#define	JOBTOK	2
#define	FILETOK	3
#define	TOTALTOK (count - 2)
#define	NTOK	5
#define	MAXTOK	128
#endif	/* OSF1 */

	char *tok[MAXTOK];
	int  count = 0;
	pstring line2;

	pstrcpy(line2,line);

#ifdef	OSF1
	{
		size_t length;
		length = strlen(line2);
		if (line2[length-3] == ':') {
			return False;
		}
	}
#endif	/* OSF1 */

	/* FIXME: Use next_token rather than strtok! */
	tok[0] = strtok(line2," \t");
	count++;

	while ((count < MAXTOK) && ((tok[count] = strtok(NULL," \t")) != NULL)) {
		count++;
	}

	/* we must get at least NTOK tokens */
	if (count < NTOK) {
		return False;
	}

	/* the Job and Total columns must be integer */
	if (!isdigit((int)*tok[JOBTOK]) || !isdigit((int)*tok[TOTALTOK])) {
		return False;
	}

	buf->job = atoi(tok[JOBTOK]);
	buf->size = atoi(tok[TOTALTOK]);
	buf->status = strequal(tok[RANKTOK],"active")?LPQ_PRINTING:LPQ_QUEUED;
	buf->time = time(NULL);
	fstrcpy(buf->fs_user,tok[USERTOK]);
	fstrcpy(buf->fs_file,tok[FILETOK]);

	if ((FILETOK + 1) != TOTALTOK) {
		int i;

		for (i = (FILETOK + 1); i < TOTALTOK; i++) {
			/* FIXME: Using fstrcat rather than other means is a bit
			 * inefficient; this might be a problem for enormous queues with
			 * many fields. */
			fstrcat(buf->fs_file, " ");
			fstrcat(buf->fs_file, tok[i]);
		}
		/* Ensure null termination. */
		fstrterminate(buf->fs_file);
	}

#ifdef PRIOTOK
	buf->priority = atoi(tok[PRIOTOK]);
#else
	buf->priority = 1;
#endif
	return True;
}

/*
<magnus@hum.auc.dk>
LPRng_time modifies the current date by inserting the hour and minute from
the lpq output.  The lpq time looks like "23:15:07"

<allan@umich.edu> June 30, 1998.
Modified to work with the re-written parse_lpq_lprng routine.

<J.P.M.v.Itegem@tue.nl> Dec 17,1999
Modified to work with lprng 3.16
With lprng 3.16 The lpq time looks like
                       "23:15:07"
                       "23:15:07.100"
                       "1999-12-16-23:15:07"
                       "1999-12-16-23:15:07.100"

*/
static time_t LPRng_time(char *time_string)
{
	time_t jobtime;
	struct tm *t;

	jobtime = time(NULL);         /* default case: take current time */
	t = localtime(&jobtime);
	if (!t) {
		return (time_t)-1;
	}

	if ( atoi(time_string) < 24 ){
		t->tm_hour = atoi(time_string);
		t->tm_min = atoi(time_string+3);
		t->tm_sec = atoi(time_string+6);
	} else {
		t->tm_year = atoi(time_string)-1900;
		t->tm_mon = atoi(time_string+5)-1;
		t->tm_mday = atoi(time_string+8);
		t->tm_hour = atoi(time_string+11);
		t->tm_min = atoi(time_string+14);
		t->tm_sec = atoi(time_string+17);
	}    
	jobtime = mktime(t);

	return jobtime;
}

/****************************************************************************
  parse a lprng lpq line
  <allan@umich.edu> June 30, 1998.
  Re-wrote this to handle file names with spaces, multiple file names on one
  lpq line, etc;

****************************************************************************/

static BOOL parse_lpq_lprng(char *line,print_queue_struct *buf,BOOL first)
{
#define	LPRNG_RANKTOK	0
#define	LPRNG_USERTOK	1
#define	LPRNG_PRIOTOK	2
#define	LPRNG_JOBTOK	3
#define	LPRNG_FILETOK	4
#define	LPRNG_TOTALTOK	(num_tok - 2)
#define	LPRNG_TIMETOK	(num_tok - 1)
#define	LPRNG_NTOK	7
#define	LPRNG_MAXTOK	128 /* PFMA just to keep us from running away. */

	fstring tokarr[LPRNG_MAXTOK];
	const char *cptr;
	char *ptr;
	int num_tok = 0;

	cptr = line;
	while((num_tok < LPRNG_MAXTOK) && next_token( &cptr, tokarr[num_tok], " \t", sizeof(fstring))) {
		num_tok++;
	}

	/* We must get at least LPRNG_NTOK tokens. */
	if (num_tok < LPRNG_NTOK) {
		return False;
	}

	if (!isdigit((int)*tokarr[LPRNG_JOBTOK]) || !isdigit((int)*tokarr[LPRNG_TOTALTOK])) {
		return False;
	}

	buf->job  = atoi(tokarr[LPRNG_JOBTOK]);
	buf->size = atoi(tokarr[LPRNG_TOTALTOK]);

	if (strequal(tokarr[LPRNG_RANKTOK],"active")) {
		buf->status = LPQ_PRINTING;
	} else if (strequal(tokarr[LPRNG_RANKTOK],"done")) {
		buf->status = LPQ_PRINTED;
	} else if (isdigit((int)*tokarr[LPRNG_RANKTOK])) {
		buf->status = LPQ_QUEUED;
	} else {
		buf->status = LPQ_PAUSED;
	}

	buf->priority = *tokarr[LPRNG_PRIOTOK] -'A';

	buf->time = LPRng_time(tokarr[LPRNG_TIMETOK]);

	fstrcpy(buf->fs_user,tokarr[LPRNG_USERTOK]);

	/* The '@hostname' prevents windows from displaying the printing icon
	 * for the current user on the taskbar.  Plop in a null.
	 */

	if ((ptr = strchr_m(buf->fs_user,'@')) != NULL) {
		*ptr = '\0';
	}

	fstrcpy(buf->fs_file,tokarr[LPRNG_FILETOK]);

	if ((LPRNG_FILETOK + 1) != LPRNG_TOTALTOK) {
		int i;

		for (i = (LPRNG_FILETOK + 1); i < LPRNG_TOTALTOK; i++) {
			/* FIXME: Using fstrcat rather than other means is a bit
			 * inefficient; this might be a problem for enormous queues with
			 * many fields. */
			fstrcat(buf->fs_file, " ");
			fstrcat(buf->fs_file, tokarr[i]);
		}
		/* Ensure null termination. */
		fstrterminate(buf->fs_file);
	}

	return True;
}

/*******************************************************************
parse lpq on an aix system

Queue   Dev   Status    Job Files              User         PP %   Blks  Cp Rnk
------- ----- --------- --- ------------------ ---------- ---- -- ----- --- ---
lazer   lazer READY
lazer   lazer RUNNING   537 6297doc.A          kvintus@IE    0 10  2445   1   1
              QUEUED    538 C.ps               root@IEDVB           124   1   2
              QUEUED    539 E.ps               root@IEDVB            28   1   3
              QUEUED    540 L.ps               root@IEDVB           172   1   4
              QUEUED    541 P.ps               root@IEDVB            22   1   5
********************************************************************/

static BOOL parse_lpq_aix(char *line,print_queue_struct *buf,BOOL first)
{
	fstring tok[11];
	int count=0;
	const char *cline = line;

	/* handle the case of "(standard input)" as a filename */
	string_sub(line,"standard input","STDIN",0);
	all_string_sub(line,"(","\"",0);
	all_string_sub(line,")","\"",0);

	for (count=0; count<10 && next_token(&cline,tok[count],NULL, sizeof(tok[count])); count++) {
		;
	}

	/* we must get 6 tokens */
	if (count < 10) {
		if ((count == 7) && ((strcmp(tok[0],"QUEUED") == 0) || (strcmp(tok[0],"HELD") == 0))) {
			/* the 2nd and 5th columns must be integer */
			if (!isdigit((int)*tok[1]) || !isdigit((int)*tok[4])) {
				return False;
			}
			buf->size = atoi(tok[4]) * 1024;
			/* if the fname contains a space then use STDIN */
			if (strchr_m(tok[2],' ')) {
				fstrcpy(tok[2],"STDIN");
			}

			/* only take the last part of the filename */
			{
				fstring tmp;
				char *p = strrchr_m(tok[2],'/');
				if (p) {
					fstrcpy(tmp,p+1);
					fstrcpy(tok[2],tmp);
				}
			}

			buf->job = atoi(tok[1]);
			buf->status = strequal(tok[0],"HELD")?LPQ_PAUSED:LPQ_QUEUED;
			buf->priority = 0;
			buf->time = time(NULL);
			fstrcpy(buf->fs_user,tok[3]);
			fstrcpy(buf->fs_file,tok[2]);
		} else {
			DEBUG(6,("parse_lpq_aix count=%d\n", count));
			return False;
		}
	} else {
		/* the 4th and 9th columns must be integer */
		if (!isdigit((int)*tok[3]) || !isdigit((int)*tok[8])) {
			return False;
		}

		buf->size = atoi(tok[8]) * 1024;
		/* if the fname contains a space then use STDIN */
		if (strchr_m(tok[4],' ')) {
			fstrcpy(tok[4],"STDIN");
		}

		/* only take the last part of the filename */
		{
			fstring tmp;
			char *p = strrchr_m(tok[4],'/');
			if (p) {
				fstrcpy(tmp,p+1);
				fstrcpy(tok[4],tmp);
			}
		}

		buf->job = atoi(tok[3]);
		buf->status = strequal(tok[2],"RUNNING")?LPQ_PRINTING:LPQ_QUEUED;
		buf->priority = 0;
		buf->time = time(NULL);
		fstrcpy(buf->fs_user,tok[5]);
		fstrcpy(buf->fs_file,tok[4]);
	}

	return True;
}

/****************************************************************************
parse a lpq line
here is an example of lpq output under hpux; note there's no space after -o !
$> lpstat -oljplus
ljplus-2153         user           priority 0  Jan 19 08:14 on ljplus
      util.c                                  125697 bytes
      server.c				      110712 bytes
ljplus-2154         user           priority 0  Jan 19 08:14 from client
      (standard input)                          7551 bytes
****************************************************************************/

static BOOL parse_lpq_hpux(char *line, print_queue_struct *buf, BOOL first)
{
	/* must read two lines to process, therefore keep some values static */
	static BOOL header_line_ok=False, base_prio_reset=False;
	static fstring jobuser;
	static int jobid;
	static int jobprio;
	static time_t jobtime;
	static int jobstat=LPQ_QUEUED;
	/* to store minimum priority to print, lpstat command should be invoked
		with -p option first, to work */
	static int base_prio;
	int count;
	char htab = '\011';  
	const char *cline = line;
	fstring tok[12];

	/* If a line begins with a horizontal TAB, it is a subline type */
  
	if (line[0] == htab) { /* subline */
		/* check if it contains the base priority */
		if (!strncmp(line,"\tfence priority : ",18)) {
			base_prio=atoi(&line[18]);
			DEBUG(4, ("fence priority set at %d\n", base_prio));
		}

		if (!header_line_ok) {
			return  False; /* incorrect header line */
		}

		/* handle the case of "(standard input)" as a filename */
		string_sub(line,"standard input","STDIN",0);
		all_string_sub(line,"(","\"",0);
		all_string_sub(line,")","\"",0);
    
		for (count=0; count<2 && next_token(&cline,tok[count],NULL,sizeof(tok[count])); count++) {
			;
		}
		/* we must get 2 tokens */
		if (count < 2) {
			return False;
		}
    
		/* the 2nd column must be integer */
		if (!isdigit((int)*tok[1])) {
			return False;
		}
    
		/* if the fname contains a space then use STDIN */
		if (strchr_m(tok[0],' ')) {
			fstrcpy(tok[0],"STDIN");
		}
    
		buf->size = atoi(tok[1]);
		fstrcpy(buf->fs_file,tok[0]);
    
		/* fill things from header line */
		buf->time = jobtime;
		buf->job = jobid;
		buf->status = jobstat;
		buf->priority = jobprio;
		fstrcpy(buf->fs_user,jobuser);
    
		return True;
	} else { /* header line */
		header_line_ok=False; /* reset it */
		if (first) {
			if (!base_prio_reset) {
				base_prio=0; /* reset it */
				base_prio_reset=True;
			}
		} else if (base_prio) {
			base_prio_reset=False;
		}
    
		/* handle the dash in the job id */
		string_sub(line,"-"," ",0);
    
		for (count=0; count<12 && next_token(&cline,tok[count],NULL,sizeof(tok[count])); count++) {
			;
		}
      
		/* we must get 8 tokens */
		if (count < 8) {
			return False;
		}
    
		/* first token must be printer name (cannot check ?) */
		/* the 2nd, 5th & 7th column must be integer */
		if (!isdigit((int)*tok[1]) || !isdigit((int)*tok[4]) || !isdigit((int)*tok[6])) {
			return False;
		}
		jobid = atoi(tok[1]);
		fstrcpy(jobuser,tok[2]);
		jobprio = atoi(tok[4]);
    
		/* process time */
		jobtime=EntryTime(tok, 5, count, 8);
		if (jobprio < base_prio) {
			jobstat = LPQ_PAUSED;
			DEBUG (4, ("job %d is paused: prio %d < %d; jobstat=%d\n",
				jobid, jobprio, base_prio, jobstat));
		} else {
			jobstat = LPQ_QUEUED;
			if ((count >8) && (((strequal(tok[8],"on")) ||
					((strequal(tok[8],"from")) && 
					((count > 10)&&(strequal(tok[10],"on"))))))) {
				jobstat = LPQ_PRINTING;
			}
		}
    
		header_line_ok=True; /* information is correct */
		return False; /* need subline info to include into queuelist */
	}
}

/****************************************************************************
parse a lpstat line

here is an example of "lpstat -o dcslw" output under sysv

dcslw-896               tridge            4712   Dec 20 10:30:30 on dcslw
dcslw-897               tridge            4712   Dec 20 10:30:30 being held

****************************************************************************/

static BOOL parse_lpq_sysv(char *line,print_queue_struct *buf,BOOL first)
{
	fstring tok[9];
	int count=0;
	char *p;
	const char *cline = line;

	/* 
	 * Handle the dash in the job id, but make sure that we skip over
	 * the printer name in case we have a dash in that.
	 * Patch from Dom.Mitchell@palmerharvey.co.uk.
	 */

	/*
	 * Move to the first space.
	 */
	for (p = line ; !isspace(*p) && *p; p++) {
		;
	}

	/*
	 * Back up until the last '-' character or
	 * start of line.
	 */
	for (; (p >= line) && (*p != '-'); p--) {
		;
	}

	if((p >= line) && (*p == '-')) {
		*p = ' ';
	}

	for (count=0; count<9 && next_token(&cline,tok[count],NULL,sizeof(tok[count])); count++) {
		;
	}

	/* we must get 7 tokens */
	if (count < 7) {
		return False;
	}

	/* the 2nd and 4th, 6th columns must be integer */
	if (!isdigit((int)*tok[1]) || !isdigit((int)*tok[3])) {
		return False;
	}
	if (!isdigit((int)*tok[5])) {
		return False;
	}

	/* if the user contains a ! then trim the first part of it */  
	if ((p=strchr_m(tok[2],'!'))) {
		fstring tmp;
		fstrcpy(tmp,p+1);
		fstrcpy(tok[2],tmp);
	}

	buf->job = atoi(tok[1]);
	buf->size = atoi(tok[3]);
	if (count > 7 && strequal(tok[7],"on")) {
		buf->status = LPQ_PRINTING;
	} else if (count > 8 && strequal(tok[7],"being") && strequal(tok[8],"held")) {
		buf->status = LPQ_PAUSED;
	} else {
		buf->status = LPQ_QUEUED;
	}
	buf->priority = 0;
	buf->time = EntryTime(tok, 4, count, 7);
	fstrcpy(buf->fs_user,tok[2]);
	fstrcpy(buf->fs_file,tok[2]);
	return True;
}

/****************************************************************************
parse a lpq line

here is an example of lpq output under qnx
Spooler: /qnx/spooler, on node 1
Printer: txt        (ready) 
0000:     root	[job #1    ]   active 1146 bytes	/etc/profile
0001:     root	[job #2    ]    ready 2378 bytes	/etc/install
0002:     root	[job #3    ]    ready 1146 bytes	-- standard input --
****************************************************************************/

static BOOL parse_lpq_qnx(char *line,print_queue_struct *buf,BOOL first)
{
	fstring tok[7];
	int count=0;
	const char *cline = line;

	DEBUG(4,("antes [%s]\n", line));

	/* handle the case of "-- standard input --" as a filename */
	string_sub(line,"standard input","STDIN",0);
	DEBUG(4,("despues [%s]\n", line));
	all_string_sub(line,"-- ","\"",0);
	all_string_sub(line," --","\"",0);
	DEBUG(4,("despues 1 [%s]\n", line));

	string_sub(line,"[job #","",0);
	string_sub(line,"]","",0);
	DEBUG(4,("despues 2 [%s]\n", line));

	for (count=0; count<7 && next_token(&cline,tok[count],NULL,sizeof(tok[count])); count++) {
		;
	}

	/* we must get 7 tokens */
	if (count < 7) {
		return False;
	}

	/* the 3rd and 5th columns must be integer */
	if (!isdigit((int)*tok[2]) || !isdigit((int)*tok[4])) {
		return False;
	}

	/* only take the last part of the filename */
	{
		fstring tmp;
		char *p = strrchr_m(tok[6],'/');
		if (p) {
			fstrcpy(tmp,p+1);
			fstrcpy(tok[6],tmp);
		}
	}
	
	buf->job = atoi(tok[2]);
	buf->size = atoi(tok[4]);
	buf->status = strequal(tok[3],"active")?LPQ_PRINTING:LPQ_QUEUED;
	buf->priority = 0;
	buf->time = time(NULL);
	fstrcpy(buf->fs_user,tok[1]);
	fstrcpy(buf->fs_file,tok[6]);
	return True;
}

/****************************************************************************
  parse a lpq line for the plp printing system
  Bertrand Wallrich <Bertrand.Wallrich@loria.fr>

redone by tridge. Here is a sample queue:

Local  Printer 'lp2' (fjall):
  Printing (started at Jun 15 13:33:58, attempt 1).
    Rank Owner       Pr Opt  Job Host        Files           Size     Date
  active tridge      X  -    6   fjall       /etc/hosts      739      Jun 15 13:33
     3rd tridge      X  -    7   fjall       /etc/hosts      739      Jun 15 13:33

****************************************************************************/

static BOOL parse_lpq_plp(char *line,print_queue_struct *buf,BOOL first)
{
	fstring tok[11];
	int count=0;
	const char *cline = line;

	/* handle the case of "(standard input)" as a filename */
	string_sub(line,"stdin","STDIN",0);
	all_string_sub(line,"(","\"",0);
	all_string_sub(line,")","\"",0);
  
	for (count=0; count<11 && next_token(&cline,tok[count],NULL,sizeof(tok[count])); count++) {
		;
	}

	/* we must get 11 tokens */
	if (count < 11) {
		return False;
	}

	/* the first must be "active" or begin with an integer */
	if (strcmp(tok[0],"active") && !isdigit((int)tok[0][0])) {
		return False;
	}

	/* the 5th and 8th must be integer */
	if (!isdigit((int)*tok[4]) || !isdigit((int)*tok[7])) {
		return False;
	}

	/* if the fname contains a space then use STDIN */
	if (strchr_m(tok[6],' ')) {
		fstrcpy(tok[6],"STDIN");
	}

	/* only take the last part of the filename */
	{
		fstring tmp;
		char *p = strrchr_m(tok[6],'/');
		if (p) {
			fstrcpy(tmp,p+1);
			fstrcpy(tok[6],tmp);
		}
	}

	buf->job = atoi(tok[4]);

	buf->size = atoi(tok[7]);
	if (strchr_m(tok[7],'K')) {
		buf->size *= 1024;
	}
	if (strchr_m(tok[7],'M')) {
		buf->size *= 1024*1024;
	}

	buf->status = strequal(tok[0],"active")?LPQ_PRINTING:LPQ_QUEUED;
	buf->priority = 0;
	buf->time = time(NULL);
	fstrcpy(buf->fs_user,tok[1]);
	fstrcpy(buf->fs_file,tok[6]);
	return True;
}

/*******************************************************************
parse lpq on an NT system

                         Windows 2000 LPD Server
                              Printer \\10.0.0.2\NP17PCL (Paused)

Owner       Status         Jobname          Job-Id    Size   Pages  Priority
----------------------------------------------------------------------------
root (9.99. Printing  /usr/lib/rhs/rhs-pr      3       625      0      1
root (9.99. Paused    /usr/lib/rhs/rhs-pr      4       625      0      1
jmcd        Waiting   Re: Samba Open Sour     26     32476      1      1

********************************************************************/

static BOOL parse_lpq_nt(char *line,print_queue_struct *buf,BOOL first)
{
#define LPRNT_OWNSIZ 11
#define LPRNT_STATSIZ 9
#define LPRNT_JOBSIZ 19
#define LPRNT_IDSIZ 6
#define LPRNT_SIZSIZ 9
	typedef struct {
		char owner[LPRNT_OWNSIZ];
		char space1;
		char status[LPRNT_STATSIZ];
		char space2;
		char jobname[LPRNT_JOBSIZ];
		char space3;
		char jobid[LPRNT_IDSIZ];
		char space4;
		char size[LPRNT_SIZSIZ];
		char terminator;
	} nt_lpq_line;

	nt_lpq_line parse_line;
#define LPRNT_PRINTING "Printing"
#define LPRNT_WAITING "Waiting"
#define LPRNT_PAUSED "Paused"

	memset(&parse_line, '\0', sizeof(parse_line));
	strncpy((char *) &parse_line, line, sizeof(parse_line) -1);

	if (strlen((char *) &parse_line) != sizeof(parse_line) - 1) {
		return False;
	}

	/* Just want the first word in the owner field - the username */
	if (strchr_m(parse_line.owner, ' ')) {
		*(strchr_m(parse_line.owner, ' ')) = '\0';
	} else {
		parse_line.space1 = '\0';
	}

	/* Make sure we have an owner */
	if (!strlen(parse_line.owner)) {
		return False;
	}

	/* Make sure the status is valid */
	parse_line.space2 = '\0';
	trim_char(parse_line.status, '\0', ' ');
	if (!strequal(parse_line.status, LPRNT_PRINTING) &&
			!strequal(parse_line.status, LPRNT_PAUSED) &&
			!strequal(parse_line.status, LPRNT_WAITING)) {
		return False;
	}
  
	parse_line.space3 = '\0';
	trim_char(parse_line.jobname, '\0', ' ');

	buf->job = atoi(parse_line.jobid);
	buf->priority = 0;
	buf->size = atoi(parse_line.size);
	buf->time = time(NULL);
	fstrcpy(buf->fs_user, parse_line.owner);
	fstrcpy(buf->fs_file, parse_line.jobname);
	if (strequal(parse_line.status, LPRNT_PRINTING)) {
		buf->status = LPQ_PRINTING;
	} else if (strequal(parse_line.status, LPRNT_PAUSED)) {
		buf->status = LPQ_PAUSED;
	} else {
		buf->status = LPQ_QUEUED;
	}

	return True;
}

/*******************************************************************
parse lpq on an OS2 system

JobID  File Name          Rank      Size        Status          Comment       
-----  ---------------    ------    --------    ------------    ------------  
    3  Control                 1          68    Queued          root@psflinu  
    4  /etc/motd               2       11666    Queued          root@psflinu  

********************************************************************/

static BOOL parse_lpq_os2(char *line,print_queue_struct *buf,BOOL first)
{
#define LPROS2_IDSIZ 5
#define LPROS2_JOBSIZ 15
#define LPROS2_SIZSIZ 8
#define LPROS2_STATSIZ 12
#define LPROS2_OWNSIZ 12
	typedef struct {
		char jobid[LPROS2_IDSIZ];
		char space1[2];
		char jobname[LPROS2_JOBSIZ];
		char space2[14];
		char size[LPROS2_SIZSIZ];
		char space3[4];
		char status[LPROS2_STATSIZ];
		char space4[4];
		char owner[LPROS2_OWNSIZ];
		char terminator;
	} os2_lpq_line;

	os2_lpq_line parse_line;
#define LPROS2_PRINTING "Printing"
#define LPROS2_WAITING "Queued"
#define LPROS2_PAUSED "Paused"

	memset(&parse_line, '\0', sizeof(parse_line));
	strncpy((char *) &parse_line, line, sizeof(parse_line) -1);

	if (strlen((char *) &parse_line) != sizeof(parse_line) - 1) {
		return False;
	}

	/* Get the jobid */
	buf->job = atoi(parse_line.jobid);

	/* Get the job name */
	parse_line.space2[0] = '\0';
	trim_char(parse_line.jobname, '\0', ' ');
	fstrcpy(buf->fs_file, parse_line.jobname);

	buf->priority = 0;
	buf->size = atoi(parse_line.size);
	buf->time = time(NULL);

	/* Make sure we have an owner */
	if (!strlen(parse_line.owner)) {
		return False;
	}

	/* Make sure we have a valid status */
	parse_line.space4[0] = '\0';
	trim_char(parse_line.status, '\0', ' ');
	if (!strequal(parse_line.status, LPROS2_PRINTING) &&
			!strequal(parse_line.status, LPROS2_PAUSED) &&
			!strequal(parse_line.status, LPROS2_WAITING)) {
		return False;
	}

	fstrcpy(buf->fs_user, parse_line.owner);
	if (strequal(parse_line.status, LPROS2_PRINTING)) {
		buf->status = LPQ_PRINTING;
	} else if (strequal(parse_line.status, LPROS2_PAUSED)) {
		buf->status = LPQ_PAUSED;
	} else {
		buf->status = LPQ_QUEUED;
	}

	return True;
}

static const char *stat0_strings[] = { "enabled", "online", "idle", "no entries", "free", "ready", NULL };
static const char *stat1_strings[] = { "offline", "disabled", "down", "off", "waiting", "no daemon", NULL };
static const char *stat2_strings[] = { "jam", "paper", "error", "responding", "not accepting", "not running", "turned off", NULL };

#ifdef DEVELOPER

/****************************************************************************
parse a vlp line
****************************************************************************/

static BOOL parse_lpq_vlp(char *line,print_queue_struct *buf,BOOL first)
{
	int toknum = 0;
	fstring tok;
	const char *cline = line;

	/* First line is printer status */

	if (!isdigit(line[0])) return False;

	/* Parse a print job entry */

	while(next_token(&cline, tok, NULL, sizeof(fstring))) {
		switch (toknum) {
		case 0:
			buf->job = atoi(tok);
			break;
		case 1:
			buf->size = atoi(tok);
			break;
		case 2:
			buf->status = atoi(tok);
			break;
		case 3:
			buf->time = atoi(tok);
			break;
		case 4:
			fstrcpy(buf->fs_user, tok);
			break;
		case 5:
			fstrcpy(buf->fs_file, tok);
			break;
		}
		toknum++;
	}

	return True;
}

#endif /* DEVELOPER */

/****************************************************************************
parse a lpq line. Choose printing style
****************************************************************************/

BOOL parse_lpq_entry(enum printing_types printing_type,char *line,
		     print_queue_struct *buf,
		     print_status_struct *status,BOOL first)
{
	BOOL ret;

	switch (printing_type) {
		case PRINT_SYSV:
			ret = parse_lpq_sysv(line,buf,first);
			break;
		case PRINT_AIX:      
			ret = parse_lpq_aix(line,buf,first);
			break;
		case PRINT_HPUX:
			ret = parse_lpq_hpux(line,buf,first);
			break;
		case PRINT_QNX:
			ret = parse_lpq_qnx(line,buf,first);
			break;
		case PRINT_LPRNG:
			ret = parse_lpq_lprng(line,buf,first);
			break;
		case PRINT_PLP:
			ret = parse_lpq_plp(line,buf,first);
			break;
		case PRINT_LPRNT:
			ret = parse_lpq_nt(line,buf,first);
			break;
		case PRINT_LPROS2:
			ret = parse_lpq_os2(line,buf,first);
			break;
#ifdef DEVELOPER
		case PRINT_VLP:
		case PRINT_TEST:
			ret = parse_lpq_vlp(line,buf,first);
			break;
#endif /* DEVELOPER */
		default:
			ret = parse_lpq_bsd(line,buf,first);
			break;
	}

	/* We don't want the newline in the status message. */
	{
		char *p = strchr_m(line,'\n');
		if (p) {
			*p = 0;
		}
	}

	/* in the LPRNG case, we skip lines starting by a space.*/
	if (!ret && (printing_type==PRINT_LPRNG) ) {
		if (line[0]==' ') {
			return ret;
		}
	}

	if (status && !ret) {
		/* a few simple checks to see if the line might be a
			printer status line: 
			handle them so that most severe condition is shown */
		int i;
		strlower_m(line);
      
		switch (status->status) {
			case LPSTAT_OK:
				for (i=0; stat0_strings[i]; i++) {
					if (strstr_m(line,stat0_strings[i])) {
						fstrcpy(status->message,line);
						status->status=LPSTAT_OK;
						return ret;
					}
				}
				/* fallthrough */
			case LPSTAT_STOPPED:
				for (i=0; stat1_strings[i]; i++) {
					if (strstr_m(line,stat1_strings[i])) {
						fstrcpy(status->message,line);
						status->status=LPSTAT_STOPPED;
						return ret;
					}
				}
				/* fallthrough */
			case LPSTAT_ERROR:
				for (i=0; stat2_strings[i]; i++) {
					if (strstr_m(line,stat2_strings[i])) {
						fstrcpy(status->message,line);
						status->status=LPSTAT_ERROR;
						return ret;
					}
				}
				break;
		}
	}

	return ret;
}

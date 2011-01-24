/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    motif_pipe.c: written by Vincent Pagel (pagel@loria.fr) 10/4/95

    pipe communication between motif interface and sound generator

    */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#ifdef SOLARIS
#include <sys/filio.h>
#endif
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <sys/time.h>
#include <sys/types.h>

#include "timidity.h"
#include "controls.h"
#include "motif.h"

static int pipeAppli[2],pipeMotif[2]; /* Pipe for communication with MOTIF process   */
static int fpip_in, fpip_out;	/* in and out depends in which process we are */
static int pid;	               /* Pid for child process */

/* DATA VALIDITY CHECK */
#define INT_CODE 214
#define STRING_CODE 216

#define DEBUGPIPE

/***********************************************************************/
/* PIPE COMUNICATION                                                   */
/***********************************************************************/

static void m_pipe_error(char *st)
{
    fprintf(stderr,"CONNECTION PROBLEM WITH MOTIF PROCESS IN %s BECAUSE:%s"
	    NLS,
	    st, strerror(errno));
    exit(1);
}


/*****************************************
 *              INT                      *
 *****************************************/

void m_pipe_int_write(int c)
{
    int len;
    int code=INT_CODE;

#ifdef DEBUGPIPE
    len = write(fpip_out,&code,sizeof(code));
    if (len!=sizeof(code))
	m_pipe_error("PIPE_INT_WRITE");
#endif

    len = write(fpip_out,&c,sizeof(c));
    if (len!=sizeof(int))
	m_pipe_error("PIPE_INT_WRITE");
}

void m_pipe_int_read(int *c)
{
    int len;

#ifdef DEBUGPIPE
    int code;

    len = read(fpip_in,&code,sizeof(code));
    if (len!=sizeof(code))
	m_pipe_error("PIPE_INT_READ");
    if (code!=INT_CODE)
	fprintf(stderr,"BUG ALERT ON INT PIPE %i" NLS,
		code);
#endif

    len = read(fpip_in,c, sizeof(int));
    if (len!=sizeof(int)) m_pipe_error("PIPE_INT_READ");
}



/*****************************************
 *              STRINGS                  *
 *****************************************/

void m_pipe_string_write(char *str)
{
   int len, slen;

#ifdef DEBUGPIPE
   int code=STRING_CODE;

   len = write(fpip_out,&code,sizeof(code));
   if (len!=sizeof(code))	m_pipe_error("PIPE_STRING_WRITE");
#endif

   slen=strlen(str);
   len = write(fpip_out,&slen,sizeof(slen));
   if (len!=sizeof(slen)) m_pipe_error("PIPE_STRING_WRITE");

   len = write(fpip_out,str,slen);
   if (len!=slen) m_pipe_error("PIPE_STRING_WRITE on string part");
}

void m_pipe_string_read(char *str)
{
    int len, slen;

#ifdef DEBUGPIPE
    int code;

    len = read(fpip_in,&code,sizeof(code));
    if (len!=sizeof(code)) m_pipe_error("PIPE_STRING_READ");
    if (code!=STRING_CODE) fprintf(stderr,"BUG ALERT ON STRING PIPE %i"
				   NLS, code);
#endif

    len = read(fpip_in,&slen,sizeof(slen));
    if (len!=sizeof(slen)) m_pipe_error("PIPE_STRING_READ");

    len = read(fpip_in,str,slen);
    if (len!=slen) m_pipe_error("PIPE_STRING_READ on string part");
    str[slen]='\0';		/* Append a terminal 0 */
}

int m_pipe_read_ready(void)
{
    fd_set fds;
    int cnt;
    struct timeval timeout;

    FD_ZERO(&fds);
    FD_SET(fpip_in, &fds);
    timeout.tv_sec = timeout.tv_usec = 0;
    if((cnt = select(fpip_in + 1, &fds, NULL, NULL, &timeout)) < 0)
    {
	perror("select");
	return -1;
    }

    return cnt > 0 && FD_ISSET(fpip_in, &fds) != 0;
}

void m_pipe_open(void)
{
    int res;

    res=pipe(pipeAppli);
    if (res!=0) m_pipe_error("PIPE_APPLI CREATION");

    res=pipe(pipeMotif);
    if (res!=0) m_pipe_error("PIPE_MOTIF CREATION");

    if ((pid=fork())==0)   /*child*/
	{
	    close(pipeMotif[1]);
	    close(pipeAppli[0]);

	    fpip_in=pipeMotif[0];
	    fpip_out= pipeAppli[1];

	    Launch_Motif_Process(fpip_in);
	    /* Won't come back from here */
	    fprintf(stderr,"WARNING: come back from MOTIF" NLS);
	    exit(0);
	}

    close(pipeMotif[0]);
    close(pipeAppli[1]);

    fpip_in= pipeAppli[0];
    fpip_out= pipeMotif[1];
}

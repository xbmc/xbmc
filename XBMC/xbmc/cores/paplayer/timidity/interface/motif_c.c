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

    motif_ctl.c: written by Vincent Pagel (pagel@loria.fr) 10/4/95

    A motif interface for TIMIDITY : to prevent X redrawings from
    interfering with the audio computation, I don't use the XtAppAddWorkProc

    I create a pipe between the timidity process and a Motif interface
    process forked from the 1st one

    */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "motif.h"
#include "miditrace.h"

static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int secs, int v);
static void ctl_lyric(int lyricid);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_pass_playing_list(int number_of_files, char *list_of_files[]);
static void ctl_event(CtlEvent *e);

static int motif_ready = 0;

/**********************************************/
/* export the interface functions */

#define ctl motif_control_mode

ControlMode ctl=
{
    "motif interface", 'm',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    ctl_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};


/***********************************************************************/
/* Put controls on the pipe                                            */
/***********************************************************************/
static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
    char local[255];

    va_list ap;
    if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
	ctl.verbosity<verbosity_level)
	return 0;

    va_start(ap, fmt);
    if (!motif_ready)
	{
	    vfprintf(stderr, fmt, ap);
	    fprintf(stderr, NLS);
	}
    else
	{
	    vsnprintf(local, sizeof(local), fmt, ap);
	    m_pipe_int_write(CMSG_MESSAGE);
	    m_pipe_int_write(type);
	    m_pipe_string_write(local);
	}
    va_end(ap);
    return 0;
}


static void _ctl_refresh(void)
{
    /* m_pipe_int_write(REFRESH_MESSAGE); */
}

static void ctl_refresh(void)
{
  if (ctl.trace_playing)
    _ctl_refresh();
}

static void ctl_total_time(int tt)
{
  int secs=tt/play_mode->rate;

  m_pipe_int_write(TOTALTIME_MESSAGE);
  m_pipe_int_write(secs);
}

static void ctl_master_volume(int mv)
{
    m_pipe_int_write(MASTERVOL_MESSAGE);
    m_pipe_int_write(mv);
}

static void ctl_file_name(char *name)
{
    m_pipe_int_write(FILENAME_MESSAGE);
    m_pipe_string_write(name);
}

static void ctl_current_time(int secs, int v)
{
    m_pipe_int_write(CURTIME_MESSAGE);
    m_pipe_int_write(secs);
    m_pipe_int_write(v);
}

static void ctl_lyric(int lyricid)
{
    char *lyric;
    static char lyric_buf[300];

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
	if(lyric[0] == ME_KARAOKE_LYRIC)
	{
	    if(!lyric[1])
		return;
	    if(lyric[1] == '/' || lyric[1] == '\\')
	    {
		snprintf(lyric_buf, sizeof(lyric_buf), "\n%s", lyric + 2);
		m_pipe_int_write(LYRIC_MESSAGE);
		m_pipe_string_write(lyric_buf);
	    }
	    else if(lyric[1] == '@')
	    {
		if(lyric[2] == 'L')
		    snprintf(lyric_buf, sizeof(lyric_buf), "Language: %s\n", lyric + 3);
		else if(lyric[2] == 'T')
		    snprintf(lyric_buf, sizeof(lyric_buf), "Title: %s\n", lyric + 3);
		else
		    snprintf(lyric_buf, sizeof(lyric_buf), "%s\n", lyric + 1);
		m_pipe_int_write(LYRIC_MESSAGE);
		m_pipe_string_write(lyric_buf);
	    }
	    else
	    {
		strncpy(lyric_buf, lyric + 1, sizeof(lyric_buf) - 1);
		m_pipe_int_write(LYRIC_MESSAGE);
		m_pipe_string_write(lyric_buf);
	    }
	}
	else
	{
	    strncpy(lyric_buf, lyric + 1, sizeof(lyric_buf) - 1);
	    m_pipe_int_write(LYRIC_MESSAGE);
	    m_pipe_string_write(lyric_buf);
	}
    }
}

static void ctl_event(CtlEvent *e)
{
    switch(e->type)
    {
      case CTLE_NOW_LOADING:
	ctl_file_name((char *)e->v1);
	break;
      case CTLE_PLAY_START:
	ctl_total_time((int)e->v1);
	break;
      case CTLE_CURRENT_TIME:
	ctl_current_time((int)e->v1, (int)e->v2);
	break;
      case CTLE_MASTER_VOLUME:
	ctl_master_volume((int)e->v1);
	break;
      case CTLE_LYRIC:
	ctl_lyric((int)e->v1);
	break;
      case CTLE_REFRESH:
	ctl_refresh();
	break;
    }
}


/***********************************************************************/
/* OPEN THE CONNECTION                                                */
/***********************************************************************/
/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
  ctl.opened=1;
#if 0
  ctl.trace_playing=1;	/* Default mode with Motif interface */
#endif

  /* The child process won't come back from this call  */
  m_pipe_open();

  return 0;
}

/* Tells the window to disapear */
static void ctl_close(void)
{
  if (ctl.opened)
    {
	m_pipe_int_write(CLOSE_MESSAGE);
	ctl.opened=0;
	motif_ready = 0;
    }
}


/*
 * Read information coming from the window in a BLOCKING way
 */
static int ctl_blocking_read(int32 *valp)
{
  int command;
  int new_volume;
  int new_secs;
  int i=0, nfiles;
  char buf[256][256];
  char **ret, *files[256];

  m_pipe_int_read(&command);

  for(;;)    /* Loop after pause sleeping to treat other buttons! */
      {

	  switch(command)
	      {
	      case MOTIF_CHANGE_VOLUME:
		  m_pipe_int_read(&new_volume);
		  *valp= new_volume - amplification ;
		  return RC_CHANGE_VOLUME;

	      case MOTIF_CHANGE_LOCATOR:
		  m_pipe_int_read(&new_secs);
		  *valp= new_secs * play_mode->rate;
		  return RC_JUMP;

	      case MOTIF_QUIT:
		  return RC_QUIT;

	      case MOTIF_PLAY_FILE:
		  return RC_LOAD_FILE;

	      case MOTIF_NEXT:
		  return RC_NEXT;

	      case MOTIF_PREV:
		  return RC_REALLY_PREVIOUS;

	      case MOTIF_RESTART:
		  return RC_RESTART;

	      case MOTIF_FWD:
		  *valp=play_mode->rate;
		  return RC_FORWARD;

	      case MOTIF_RWD:
		  *valp=play_mode->rate;
		  return RC_BACK;

	      case MOTIF_EXPAND:
		  m_pipe_int_read(&nfiles);
		  for (i=0;i<nfiles;i++)
		  {
			m_pipe_string_read(buf[i]);
			files[i] = buf[i];
		  }
		  ret = expand_file_archives(files, &nfiles);
		  m_pipe_int_write(FILE_LIST_MESSAGE);
		  m_pipe_int_write(nfiles);
		  for (i=0;i<nfiles;i++)
			m_pipe_string_write(ret[i]);
		  if(ret != files)
		      free(ret);
		  return RC_NONE;

		case MOTIF_PAUSE:
		  return RC_TOGGLE_PAUSE;

		default:
		  fprintf(stderr,"UNKNOWN RC_MESSAGE %d" NLS, command);
		  return RC_NONE;
	      }
      }
}

/*
 * Read information coming from the window in a non blocking way
 */
static int ctl_read(int32 *valp)
{
  int num;

  /* We don't wan't to lock on reading  */
  num=m_pipe_read_ready();

  if (num==0)
      return RC_NONE;

  return(ctl_blocking_read(valp));
}

static void ctl_pass_playing_list(int number_of_files, char *list_of_files[])
{
    int i=0;
    char file_to_play[1000];
    int command;
    int32 val;

    motif_ready = 1;

    m_pipe_int_write(MASTERVOL_MESSAGE);
    m_pipe_int_write(amplification);

    /* Pass the list to the interface */
    m_pipe_int_write(FILE_LIST_MESSAGE);
    m_pipe_int_write(number_of_files);
    for (i=0;i<number_of_files;i++)
	m_pipe_string_write(list_of_files[i]);

    /* Ask the interface for a filename to play -> begin to play automatically */
    m_pipe_int_write(NEXT_FILE_MESSAGE);

    command = ctl_blocking_read(&val);

    /* Main Loop */
    for (;;)
	{
	    if (command==RC_LOAD_FILE)
		{
		    /* Read a LoadFile command */
		    m_pipe_string_read(file_to_play);
		    command=play_midi_file(file_to_play);
		}
	    else
		{
		    if (command==RC_QUIT)
			return;

		    switch(command)
			{
			case RC_ERROR:
			    m_pipe_int_write(ERROR_MESSAGE);
			    break;
			case RC_NONE:
			    break;
			case RC_NEXT:
			    m_pipe_int_write(NEXT_FILE_MESSAGE);
			    break;
			case RC_REALLY_PREVIOUS:
			    m_pipe_int_write(PREV_FILE_MESSAGE);
			    break;
			case RC_TUNE_END:
			    m_pipe_int_write(TUNE_END_MESSAGE);
			    break;
			case RC_CHANGE_VOLUME:
				amplification += val;
				break;
			default:
			    fprintf(stderr,
				    "PANIC !!! OTHER COMMAND ERROR ?!?! %i"
				    NLS, command);
			}

		    command = ctl_blocking_read(&val);
		}
	}
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_m_loader(void)
{
    return &ctl;
}

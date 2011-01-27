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

    dumb_c.c
    Minimal control mode -- no interaction, just prints out messages.
    */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#ifdef __W32__
#include "wrd.h"
#endif /* __W32__ */

static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_total_time(long tt);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct);
static void ctl_lyric(int lyricid);
static void ctl_event(CtlEvent *e);

/**********************************/
/* export the interface functions */

#define ctl dumb_control_mode

ControlMode ctl=
{
    "dumb interface", 'd',
    1,0,0,
    0,
    ctl_open,
    ctl_close,
    dumb_pass_playing_list,
    ctl_read,
    cmsg,
    ctl_event
};

static FILE *outfp;
int dumb_error_count;

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
  if(using_stdout)
    outfp=stderr;
  else
    outfp=stdout;
  ctl.opened=1;
  return 0;
}

static void ctl_close(void)
{
  fflush(outfp);
  ctl.opened=0;
}

/*ARGSUSED*/
static int ctl_read(int32 *valp)
{
  return RC_NONE;
}

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
  va_list ap;

  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      ctl.verbosity<verbosity_level)
    return 0;
  va_start(ap, fmt);
  if(type == CMSG_WARNING || type == CMSG_ERROR || type == CMSG_FATAL)
      dumb_error_count++;
  if (!ctl.opened)
    {
      vfprintf(stderr, fmt, ap);
      fputs(NLS, stderr);
    }
  else
    {
      vfprintf(outfp, fmt, ap);
      fputs(NLS, outfp);
      fflush(outfp);
    }
  va_end(ap);
  return 0;
}

static void ctl_total_time(long tt)
{
  int mins, secs;
  if (ctl.trace_playing)
    {
      secs=(int)(tt/play_mode->rate);
      mins=secs/60;
      secs-=mins*60;
      cmsg(CMSG_INFO, VERB_NORMAL,
	   "Total playing time: %3d min %02d s", mins, secs);
    }
}

static void ctl_file_name(char *name)
{
  if (ctl.verbosity>=0 || ctl.trace_playing)
      cmsg(CMSG_INFO, VERB_NORMAL, "Playing %s", name);
}

static void ctl_current_time(int secs)
{
  int mins;
  static int prev_secs = -1;

#ifdef __W32__
	  if(wrdt->id == 'w')
	    return;
#endif /* __W32__ */
  if (ctl.trace_playing && secs != prev_secs)
    {
      prev_secs = secs;
      mins=secs/60;
      secs-=mins*60;
      fprintf(outfp, "\r%3d:%02d", mins, secs);
      fflush(outfp);
    }
}

static void ctl_lyric(int lyricid)
{
    char *lyric;

    lyric = event2string(lyricid);
    if(lyric != NULL)
    {
	if(lyric[0] == ME_KARAOKE_LYRIC)
	{
	    if(lyric[1] == '/' || lyric[1] == '\\')
	    {
		fprintf(outfp, "\n%s", lyric + 2);
		fflush(outfp);
	    }
	    else if(lyric[1] == '@')
	    {
		if(lyric[2] == 'L')
		    fprintf(outfp, "\nLanguage: %s\n", lyric + 3);
		else if(lyric[2] == 'T')
		    fprintf(outfp, "Title: %s\n", lyric + 3);
		else
		    fprintf(outfp, "%s\n", lyric + 1);
	    }
	    else
	    {
		fputs(lyric + 1, outfp);
		fflush(outfp);
	    }
	}
	else
	{
	    if(lyric[0] == ME_CHORUS_TEXT || lyric[0] == ME_INSERT_TEXT)
		fprintf(outfp, "\r");
	    fputs(lyric + 1, outfp);
	    fflush(outfp);
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
	ctl_total_time(e->v1);
	break;
      case CTLE_CURRENT_TIME:
	ctl_current_time((int)e->v1);
	break;
      #ifndef CFG_FOR_SF
      case CTLE_LYRIC:
	ctl_lyric((int)e->v1);
	break;
      #endif
    }
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_d_loader(void)
{
    return &ctl;
}

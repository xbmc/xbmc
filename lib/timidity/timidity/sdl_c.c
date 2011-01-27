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

static char buffer_timidity_error[2048];

static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int32 *valp);
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void ctl_event(CtlEvent *e);

int dumb_error_count;

/*ARGSUSED*/
static int ctl_open(int using_stdin, int using_stdout)
{
  return 0;
}

static void ctl_close(void)
{
}

/*ARGSUSED*/
static int ctl_read(int32 *valp)
{
  return RC_NONE;
}

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
  va_list ap;

  if ( type != CMSG_ERROR && type != CMSG_FATAL )
	return 0;

  va_start(ap, fmt);
  vsnprintf(buffer_timidity_error, sizeof(buffer_timidity_error), fmt, ap);
  va_end(ap);
  return 0;
}

static void ctl_event(CtlEvent *e)
{
}

/*
 * interface_<id>_loader();
 */
ControlMode *interface_d_loader(void)
{
    return &ctl;
}


char *Timidity_ErrorMsg()
{
	return buffer_timidity_error;
}


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


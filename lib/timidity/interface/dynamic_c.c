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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
/*
    dynamic_c.c
    Dynamic loading control mode
*/

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
#include "dlutils.h"

static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
extern char *dynamic_interface_module(int id);
static void ctl_event(){} /* Do nothing */
static int cmsg(int type, int verbosity_level, char *fmt, ...);
static void *libhandle;
static void (* ctl_close_hook)(void);

ControlMode dynamic_control_mode =
{
    "Dynamic interface", 0,
    1,0,0,0,
    ctl_open, ctl_close, NULL, NULL, cmsg, ctl_event,
};

static int cmsg(int type, int verbosity_level, char *fmt, ...)
{
  va_list ap;

  if ((type==CMSG_TEXT || type==CMSG_INFO || type==CMSG_WARNING) &&
      dynamic_control_mode.verbosity<verbosity_level)
    return 0;
  va_start(ap, fmt);

  if (!dynamic_control_mode.opened)
    {
      vfprintf(stderr, fmt, ap);
      fprintf(stderr, "\n");
    }
  else
    {
      vfprintf(stdout, fmt, ap);
      fprintf(stdout, "\n");
    }
  va_end(ap);
  return 0;
}

static int ctl_open(int using_stdin, int using_stdout)
{
    ControlMode *(* inferface_loader)(void);
    char *path;
    char buff[256];
    int id;

    if(dynamic_control_mode.opened)
	return 0;
    dynamic_control_mode.opened = 1;

    id = dynamic_control_mode.id_character;
    path = dynamic_interface_module(id);
    if(path == NULL)
    {
	fprintf(stderr, "FATAL ERROR: dynamic_c.c: ctl_open()\n");
	exit(1);
    }

    if((libhandle = dl_load_file(path)) == NULL)
	return -1;

    sprintf(buff, "interface_%c_loader", id);
    if((inferface_loader = (ControlMode *(*)(void))
	dl_find_symbol(libhandle, buff)) == NULL)
	return -1;

    ctl = inferface_loader();

    ctl->verbosity = dynamic_control_mode.verbosity;
    ctl->trace_playing = dynamic_control_mode.trace_playing;
    ctl->flags = dynamic_control_mode.flags;
    ctl_close_hook = ctl->close;
    ctl->close = dynamic_control_mode.close; /* ctl_close() */

    return ctl->open(using_stdin, using_stdout);
}

static void ctl_close(void)
{
    if(ctl_close_hook)
    {
	ctl_close_hook();
	dl_free(libhandle);
	ctl_close_hook = NULL;
    }
}

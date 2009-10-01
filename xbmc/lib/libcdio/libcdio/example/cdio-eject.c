/* -*- c -*-
    $Id: cdio-eject.c,v 1.3 2007/08/09 01:49:09 flameeyes Exp $
  Copyright (C) 2007 Rocky Bernstein <rocky@gnu.org>
  
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
# include "config.h"
#endif

#include <cdio/cdio.h>
#include <stdio.h>
#include <string.h>

static void usage(char * progname)
  {
  fprintf(stderr, "Usage: %s [-t] <device>\n", progname);
  }

int main(int argc, char ** argv)
  {
  driver_return_code_t err;
  int close_tray = 0;
  const char * device = NULL;
  
  if(argc < 2 || argc > 3)
    {
    usage(argv[0]);
    return -1;
    }

  if((argc == 3) && strcmp(argv[1], "-t"))
    {
    usage(argv[0]);
    return -1;
    }

  if(argc == 2)
    device = argv[1];
  else if(argc == 3)
    {
    close_tray = 1;
    device = argv[2];
    }

  if(close_tray)
    {
    err = cdio_close_tray(device, NULL);
    if(err)
      {
      fprintf(stderr, "Closing tray failed for device %s: %s\n",
              device, cdio_driver_errmsg(err));
      return -1;
      }
    }
  else
    {
    err = cdio_eject_media_drive(device);
    if(err)
      {
      fprintf(stderr, "Ejecting failed for device %s: %s\n",
              device, cdio_driver_errmsg(err));
      return -1;
      }
    }

  return 0;
  
  }

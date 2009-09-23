/*
  $Id: drives.cpp,v 1.5 2006/03/28 03:26:16 rocky Exp $

  Copyright (C) 2003, 2004, 2006 Rocky Bernstein <rocky@panix.com>
  
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

/* Simple program to show drivers installed and what the default 
   CD-ROM drive is and what CD drives are available. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <cdio++/cdio.hpp>
#include <cdio/cd_types.h>
#include <cdio/logging.h>

static void 
log_handler (cdio_log_level_t level, const char message[])
{
  switch(level) {
  case CDIO_LOG_DEBUG:
  case CDIO_LOG_INFO:
    return;
  default:
    printf("cdio %d message: %s\n", level, message);
  }
}

static void 
print_drive_class(const char *psz_msg, cdio_fs_anal_t bitmask, 
		  bool b_any=false) {
  char **ppsz_cd_drives=NULL, **c;

  printf("%s...\n", psz_msg);
  ppsz_cd_drives = getDevices(NULL,  bitmask, b_any);
  if (NULL != ppsz_cd_drives) 
    for( c = ppsz_cd_drives; *c != NULL; c++ ) {
      printf("Drive %s\n", *c);
    }

  freeDeviceList(ppsz_cd_drives);
  printf("-----\n");
}

int
main(int argc, const char *argv[])
{
  char **ppsz_cd_drives=NULL, **c;
  
  cdio_log_set_handler (log_handler);

  /* Print out a list of CD-drives */
  printf("All CD-ROM/DVD drives...\n");
  ppsz_cd_drives = getDevices();
  if (NULL != ppsz_cd_drives) 
    for( c = ppsz_cd_drives; *c != NULL; c++ ) {
      printf("Drive %s\n", *c);
    }

  freeDeviceList(ppsz_cd_drives);

  print_drive_class("All CD-ROM drives (again)", CDIO_FS_MATCH_ALL);
  print_drive_class("CD-ROM drives with a CD-DA loaded...", CDIO_FS_AUDIO);
  print_drive_class("CD-ROM drives with some sort of ISO 9660 filesystem...", 
		    CDIO_FS_ANAL_ISO9660_ANY, true);
  print_drive_class("(S)VCD drives...", CDIO_FS_ANAL_VCD_ANY, true);
  return 0;
  
}

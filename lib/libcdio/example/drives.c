/*
  $Id: drives.c,v 1.1 2004/10/10 00:21:08 rocky Exp $

  Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>
  
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
#include <cdio/cdio.h>
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

int
main(int argc, const char *argv[])
{
  char **ppsz_cd_drives=NULL, **c;
  
  cdio_log_set_handler (log_handler);

  /* Print out a list of CD-drives */
  ppsz_cd_drives = cdio_get_devices(DRIVER_DEVICE);
  if (NULL != ppsz_cd_drives) 
    for( c = ppsz_cd_drives; *c != NULL; c++ ) {
      printf("Drive %s\n", *c);
    }

  cdio_free_device_list(ppsz_cd_drives);
  free(ppsz_cd_drives);
  ppsz_cd_drives = NULL;
  
  printf("-----\n");

  /* Print out a list of CD-drives the harder way. */
  ppsz_cd_drives = cdio_get_devices_with_cap(NULL, CDIO_FS_MATCH_ALL, false);

  if (NULL != ppsz_cd_drives) {
    for( c = ppsz_cd_drives; *c != NULL; c++ ) {
      printf("Drive %s\n", *c);
    }
    
  }
  cdio_free_device_list(ppsz_cd_drives);
  free(ppsz_cd_drives);

  printf("-----\n");
  printf("CD-DA drives...\n");
  ppsz_cd_drives = NULL;
  /* Print out a list of CD-drives with CD-DA's in them. */
  ppsz_cd_drives = cdio_get_devices_with_cap(NULL,  CDIO_FS_AUDIO, false);

  if (NULL != ppsz_cd_drives) {
    for( c = ppsz_cd_drives; *c != NULL; c++ ) {
      printf("drive: %s\n", *c);
    }
  }
  cdio_free_device_list(ppsz_cd_drives);
  free(ppsz_cd_drives);
    
  printf("-----\n");
  ppsz_cd_drives = NULL;
  printf("VCD drives...\n");
  /* Print out a list of CD-drives with VCD's in them. */
  ppsz_cd_drives = cdio_get_devices_with_cap(NULL, 
(CDIO_FS_ANAL_SVCD|CDIO_FS_ANAL_CVD|CDIO_FS_ANAL_VIDEOCD|CDIO_FS_UNKNOWN),
					true);
  if (NULL != ppsz_cd_drives) {
    for( c = ppsz_cd_drives; *c != NULL; c++ ) {
      printf("drive: %s\n", *c);
    }
  }

  cdio_free_device_list(ppsz_cd_drives);
  free(ppsz_cd_drives);
  
  return 0;
  
}

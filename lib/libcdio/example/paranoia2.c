/*
 * Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
/* Simple program to show using libcdio's version of the CD-DA
   paranoia library. But in this version, we'll open a cdio object before
   calling paranoia's open. I imagine in many cases such as media
   players this may be what will be done since, one may want to get
   CDDB/CD-Text info beforehand.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/cdda.h>
#include <cdio/cd_types.h>
#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

int
main(int argc, const char *argv[])
{
  cdrom_drive_t *d = NULL; /* Place to store handle given by cd-paranoia. */
  char **ppsz_cd_drives;  /* List of all drives with a loaded CDDA in it. */
  CdIo_t *p_cdio = NULL;

  /* See if we can find a device with a loaded CD-DA in it. */
  ppsz_cd_drives = cdio_get_devices_with_cap(NULL, CDIO_FS_AUDIO, false);

  if (ppsz_cd_drives) {
    /* Found such a CD-ROM with a CD-DA loaded. Use the first drive in
       the list. */
    p_cdio = cdio_open(*ppsz_cd_drives, DRIVER_UNKNOWN);
    d=cdio_cddap_identify_cdio(p_cdio, 1, NULL);
  } else {
    printf("Unable find or access a CD-ROM drive with an audio CD in it.\n");
    exit(1);
  }

  /* Don't need a list of CD's with CD-DA's any more. */
  cdio_free_device_list(ppsz_cd_drives);
  free(ppsz_cd_drives);

  /* We'll set for verbose paranoia messages. */
  cdio_cddap_verbose_set(d, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

  if ( 0 != cdio_cddap_open(d) ) {
    printf("Unable to open disc.\n");
    exit(1);
  }

  /* In the paranoia example was a reading. Here we are going to do
     something trivial (but I think neat any way - get the Endian-ness
     of the drive. */
  {
    const int i_endian = data_bigendianp(d);
    switch (i_endian) {
    case 0:
      printf("Drive returns audio data Little Endian."
	     " Your drive is like most.\n");
      break;
    case 1:
      printf("Drive returns audio data Big Endian.\n");
      break;
    case -1:
      printf("Don't know whether drive is Big or Little Endian.\n");
      break;
    default:
      printf("Whoah - got a return result I'm not expecting %d.\n",
	     i_endian);
      break;
    }
  }

  cdio_cddap_close_no_free_cdio(d);
  cdio_destroy( p_cdio );

  exit(0);
}


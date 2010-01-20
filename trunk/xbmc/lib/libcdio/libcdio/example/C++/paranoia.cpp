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
/* Simple program to show using libcdio's version of the CD-DA paranoia. 
   library. 
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <cdio/paranoia.h>
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

  /* See if we can find a device with a loaded CD-DA in it. */
  ppsz_cd_drives = cdio_get_devices_with_cap(NULL, CDIO_FS_AUDIO, false);

  if (ppsz_cd_drives) {
    /* Found such a CD-ROM with a CD-DA loaded. Use the first drive in
       the list. */
    d=cdda_identify(*ppsz_cd_drives, 1, NULL);
  } else {
    printf("Unable find or access a CD-ROM drive with an audio CD in it.\n");
    exit(1);
  }

  /* Don't need a list of CD's with CD-DA's any more. */
  cdio_free_device_list(ppsz_cd_drives);

  if ( !d ) {
    printf("Unable to identify audio CD disc.\n");
    exit(1);
  }

  /* We'll set for verbose paranoia messages. */
  cdda_verbose_set(d, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

  if ( 0 != cdda_open(d) ) {
    printf("Unable to open disc.\n");
    exit(1);
  }

  /* Okay now set up to read up to the first 300 frames of the first
     audio track of the Audio CD. */
  { 
    cdrom_paranoia_t *p = paranoia_init(d);
    lsn_t i_first_lsn = cdda_disc_firstsector(d);

    if ( -1 == i_first_lsn ) {
      printf("Trouble getting starting LSN\n");
    } else {
      lsn_t   i_cursor;
      track_t i_track    = cdda_sector_gettrack(d, i_first_lsn);
      lsn_t   i_last_lsn = cdda_track_lastsector(d, i_track);

      /* For demo purposes we'll read only 300 frames (about 4
	 seconds).  We don't want this to take too long. On the other
	 hand, I suppose it should be something close to a real test.
       */
      if ( i_last_lsn - i_first_lsn > 300) i_last_lsn = i_first_lsn + 299;

      printf("Reading track %d from LSN %ld to LSN %ld\n", i_track, 
	     (long int) i_first_lsn, (long int) i_last_lsn);

      /* Set reading mode for full paranoia, but allow skipping sectors. */
      paranoia_modeset(p, PARANOIA_MODE_FULL^PARANOIA_MODE_NEVERSKIP);

      paranoia_seek(p, i_first_lsn, SEEK_SET);

      for ( i_cursor = i_first_lsn; i_cursor <= i_last_lsn; i_cursor ++) {
	/* read a sector */
	int16_t *p_readbuf=paranoia_read(p, NULL);
	char *psz_err=cdda_errors(d);
	char *psz_mes=cdda_messages(d);

	if (psz_mes || psz_err)
	  printf("%s%s\n", psz_mes ? psz_mes: "", psz_err ? psz_err: "");

	if (psz_err) free(psz_err);
	if (psz_mes) free(psz_mes);
	if( !p_readbuf ) {
	  printf("paranoia read error. Stopping.\n");
	  break;
	}
      }
    }
    paranoia_free(p);
  }

  cdda_close(d);

  exit(0);
}

/*
  $Id: cdtext.cpp,v 1.3 2005/11/14 01:15:33 rocky Exp $

  Copyright (C) 2005 Rocky Bernstein <rocky@panix.com>
  
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

/* Simple program to list CD-Text info of a Compact Disc using
   libcdio.  An optional drive name can be supplied as an argument.
   See also corresponding C program of a similar name.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <cdio++/cdio.hpp>

/* Set up a CD-DA image to test on which is in the libcdio distribution. */
#define CDDA_IMAGE_PATH  "../../../test/"
#define CDDA_IMAGE CDDA_IMAGE_PATH "cdda.cue"

static void 
print_cdtext_track_info(CdioDevice *device, track_t i_track, 
			const char *psz_msg) {
  cdtext_t *cdtext = device->getCdtext(0);
  if (NULL != cdtext) {
    cdtext_field_t i;
    
    printf("%s\n", psz_msg);
    
    for (i= (cdtext_field_t) MIN_CDTEXT_FIELD; i < MAX_CDTEXT_FIELDS; i++) {
      if (cdtext->field[i]) {
	printf("\t%s: %s\n", cdtext_field2str(i), cdtext->field[i]);
      }
    }
  }
}
    
static void 
print_disc_info(CdioDevice *device, track_t i_tracks, track_t i_first_track) {
  track_t i_last_track = i_first_track+i_tracks;
  discmode_t cd_discmode = device->getDiscmode();

  printf("%s\n", discmode2str[cd_discmode]);
  
  print_cdtext_track_info(device, 0, "\nCD-Text for Disc:");
  for ( ; i_first_track < i_last_track; i_first_track++ ) {
    char psz_msg[50];
    sprintf(psz_msg, "CD-Text for Track %d:", i_first_track);
    print_cdtext_track_info(device, i_first_track, psz_msg);
  }
}

int
main(int argc, const char *argv[])
{
  track_t i_first_track;
  track_t i_tracks;
  CdioDevice *device = new CdioDevice;
  const char *psz_drive = NULL;

  if (!device->open(CDDA_IMAGE, DRIVER_BINCUE)) {
    printf("Couldn't open " CDDA_IMAGE " with BIN/CUE driver.\n");
  } else {
    i_first_track = device->getFirstTrackNum();
    i_tracks      = device->getNumTracks();
    print_disc_info(device, i_tracks, i_first_track);
  }

  if (argc > 1) psz_drive = argv[1];

  if (!device->open(psz_drive, DRIVER_DEVICE)) {
    printf("Couldn't find CD\n");
    return 1;
  } else {
    i_first_track = device->getFirstTrackNum();
    i_tracks      = device->getNumTracks();
    print_disc_info(device, i_tracks, i_first_track);
  }

  delete(device);

  return 0;
}

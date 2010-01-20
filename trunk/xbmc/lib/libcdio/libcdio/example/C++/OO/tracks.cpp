/*
  $Id: tracks.cpp,v 1.1 2005/11/10 11:11:15 rocky Exp $

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

/* Simple program to list track numbers and logical sector numbers of
   a Compact Disc using libcdio. */
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <cdio++/cdio.hpp>
int
main(int argc, const char *argv[])
{
  CdioDevice device;
  track_t i_first_track;
  track_t i_tracks;
  int j, i;
  CdioTrack *track;

  if (!device.open (NULL)) {
    printf("Couldn't find a driver.. leaving.\n");
    return 1;
  }
  
  i_tracks      = device.getNumTracks();
  i_first_track = i = device.getFirstTrackNum();

  printf("CD-ROM Track List (%i - %i)\n", i_first_track, i_tracks);

  printf("  #:  LSN\n");
  
  for (j = 0; j < i_tracks; i++, j++) {
    track = device.getTrackFromNum(i);
    lsn_t lsn = track->getLsn();
    if (CDIO_INVALID_LSN != lsn)
	printf("%3d: %06lu\n", (int) i, (long unsigned int) lsn);
    delete(track);
  }

  track = device.getTrackFromNum(CDIO_CDROM_LEADOUT_TRACK);
  printf("%3X: %06lu  leadout\n", CDIO_CDROM_LEADOUT_TRACK, 
	 (long unsigned int) track->getLsn());
  delete(track);
  return 0;
}

/*
  $Id: mmc2a.c,v 1.4 2006/04/12 09:38:45 rocky Exp $

  Copyright (C) 2006 Rocky Bernstein <rocky@cpan.org>
  
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

/* Sample program to show use of the MMC interface. 
   An optional drive name can be supplied as an argument.
   This basically calls to the libdio mmc_mode_sense_10() and mmc_mode_sense_6 
   routines.
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <cdio/cdio.h>
#include <cdio/mmc.h>

static void 
print_mode_sense (const char *psz_drive, const char *six_or_ten,
		  const uint8_t buf[22])
{
  printf("Mode sense %s information for %s:\n", six_or_ten, psz_drive);
  if (buf[2] & 0x01) {
    printf("\tReads CD-R media.\n");
  }
  if (buf[2] & 0x02) {
    printf("\tReads CD-RW media.\n");
  }
  if (buf[2] & 0x04) {
    printf("\tReads fixed-packet tracks when Addressing type is method 2.\n");
  }
  if (buf[2] & 0x08) {
    printf("\tReads DVD ROM media.\n");
  }
  if (buf[2] & 0x10) {
    printf("\tReads DVD-R media.\n");
  }
  if (buf[2] & 0x20) {
    printf("\tReads DVD-RAM media.\n");
  }
  if (buf[2] & 0x40) {
    printf("\tReads DVD-RAM media.\n");
  }
  if (buf[3] & 0x01) {
    printf("\tWrites CD-R media.\n");
  }
  if (buf[3] & 0x02) {
    printf("\tWrites CD-RW media.\n");
  }
  if (buf[3] & 0x04) {
    printf("\tSupports emulation write.\n");
  }
  if (buf[3] & 0x10) {
    printf("\tWrites DVD-R media.\n");
  }
  if (buf[3] & 0x20) {
    printf("\tWrites DVD-RAM media.\n");
  }
  if (buf[4] & 0x01) {
    printf("\tCan play audio.\n");
  }
  if (buf[4] & 0x02) {
    printf("\tDelivers composition A/V stream.\n");
  }
  if (buf[4] & 0x04) {
    printf("\tSupports digital output on port 2.\n");
  }
  if (buf[4] & 0x08) {
    printf("\tSupports digital output on port 1.\n");
  }
  if (buf[4] & 0x10) {
    printf("\tReads Mode-2 form 1 (e.g. XA) media.\n");
  }
  if (buf[4] & 0x20) {
    printf("\tReads Mode-2 form 2 media.\n");
  }
  if (buf[4] & 0x40) {
    printf("\tReads multi-session CD media.\n");
  }
  if (buf[4] & 0x80) {
    printf("\tSupports Buffer under-run free recording on CD-R/RW media.\n");
  }
  if (buf[4] & 0x01) {
    printf("\tCan read audio data with READ CD.\n");
  }
  if (buf[4] & 0x02) {
    printf("\tREAD CD data stream is accurate.\n");
  }
  if (buf[5] & 0x04) {
    printf("\tReads R-W subchannel information.\n");
  }
  if (buf[5] & 0x08) {
    printf("\tReads de-interleaved R-W subchannel.\n");
  }
  if (buf[5] & 0x10) {
    printf("\tSupports C2 error pointers.\n");
  }
  if (buf[5] & 0x20) {
    printf("\tReads ISRC information.\n");
  }
  if (buf[5] & 0x40) {
    printf("\tReads ISRC informaton.\n");
  }
  if (buf[5] & 0x40) {
    printf("\tReads media catalog number (MCN also known as UPC).\n");
  }
  if (buf[5] & 0x80) {
    printf("\tReads bar codes.\n");
  }
  if (buf[6] & 0x01) {
    printf("\tPREVENT/ALLOW may lock media.\n");
  }
  printf("\tLock state is %slocked.\n", (buf[6] & 0x02) ? "" : "un");
  printf("\tPREVENT/ALLOW jumper is %spresent.\n", (buf[6] & 0x04) ? "": "not ");
  if (buf[6] & 0x08) {
    printf("\tEjects media with START STOP UNIT.\n");
  }
  {
    const unsigned int i_load_type = (buf[6]>>5 & 0x07);
    printf("\tLoading mechanism type  is %d: ", i_load_type);
    switch (buf[6]>>5 & 0x07) {
    case 0: 
      printf("caddy type loading mechanism.\n"); 
      break;
    case 1: 
      printf("tray type loading mechanism.\n"); 
      break;
    case 2: 
      printf("popup type loading mechanism.\n");
      break;
    case 3: 
      printf("reserved\n");
      break;
    case 4: 
      printf("changer with individually changeable discs.\n");
      break;
    case 5: 
      printf("changer using Magazine mechanism.\n");
      break;
    case 6: 
      printf("changer using Magazine mechanism.\n");
      break;
    default:
      printf("Invalid.\n");
      break;
    }
  }
  
  if (buf[7] & 0x01) {
    printf("\tVolume controls each channel separately.\n");
  }
  if (buf[7] & 0x02) {
    printf("\tHas a changer that supports disc present reporting.\n");
  }
  if (buf[7] & 0x04) {
    printf("\tCan load empty slot in changer.\n");
  }
  if (buf[7] & 0x08) {
    printf("\tSide change capable.\n");
  }
  if (buf[7] & 0x10) {
    printf("\tReads raw R-W subchannel information from lead in.\n");
  }
  {
    const unsigned int i_speed_Kbs = CDIO_MMC_GETPOS_LEN16(buf,  8);
    printf("\tMaximum read speed is %d K bytes/sec (about %dX)\n", 
	   i_speed_Kbs, i_speed_Kbs / 176) ;
  }
  printf("\tNumber of Volume levels is %d\n",  CDIO_MMC_GETPOS_LEN16(buf, 10));
  printf("\tBuffers size for data is %d KB\n", CDIO_MMC_GETPOS_LEN16(buf, 12));
  printf("\tCurrent read speed is %d KB\n",    CDIO_MMC_GETPOS_LEN16(buf, 14));
  printf("\tMaximum write speed is %d KB\n",   CDIO_MMC_GETPOS_LEN16(buf, 18));
  printf("\tCurrent write speed is %d KB\n",   CDIO_MMC_GETPOS_LEN16(buf, 28));
}


int
main(int argc, const char *argv[])
{
  CdIo_t *p_cdio;
  const char *psz_drive = NULL;

  if (argc > 1) psz_drive = argv[1];
  p_cdio = cdio_open (psz_drive, DRIVER_UNKNOWN);

  if (!p_cdio) {
    printf("Couldn't find CD\n");
    return 1;
  } else {
    uint8_t buf[22] = { 0, };    /* Place to hold returned data */
    char *psz_cd = cdio_get_default_device(p_cdio);
    if (DRIVER_OP_SUCCESS == mmc_mode_sense_6(p_cdio, buf, sizeof(buf),
					      CDIO_MMC_CAPABILITIES_PAGE) ) {
      print_mode_sense(psz_cd, "6", buf);
    } else {
      printf("Couldn't get MODE_SENSE 6 data.\n");
    }
    if (DRIVER_OP_SUCCESS == mmc_mode_sense_10(p_cdio, buf, sizeof(buf),
					       CDIO_MMC_CAPABILITIES_PAGE) ) {
      print_mode_sense(psz_cd, "10", buf);
    } else {
      printf("Couldn't get MODE_SENSE 10 data.\n");
    }
    free(psz_cd);
  }
  
  cdio_destroy(p_cdio);
  
  return 0;
}

/*
  $Id: mmc3.c,v 1.1 2006/10/11 12:38:18 rocky Exp $

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

/* Simple program to show use of SCSI MMC interface. Is basically the
   the libdio scsi_mmc_get_hwinfo() routine.
*/
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <cdio/cdio.h>
#include <cdio/mmc.h>
#include <string.h>

/* Set how long to wait for MMC commands to complete */
#define DEFAULT_TIMEOUT_MS 10000

int
main(int argc, const char *argv[])
{
  CdIo_t *p_cdio;
  driver_return_code_t ret;
  driver_id_t driver_id = DRIVER_DEVICE;
  char *psz_drive = NULL;
  bool do_eject = false;
  
  if (argc > 1) 
    psz_drive = strdup(argv[1]);

  if (!psz_drive) {
    psz_drive = cdio_get_default_device_driver(&driver_id);
    if (!psz_drive) {
      printf("Can't find a CD-ROM\n");
     exit(1);
    }
  }

  p_cdio = cdio_open (psz_drive, driver_id);
  if (!p_cdio) {
    printf("Can't open %s\n", psz_drive);
    exit(2);
  }
  
  ret = mmc_get_tray_status(p_cdio);
  switch(ret) {
  case 0:
    printf("CD-ROM drive %s is closed.\n", psz_drive);
    do_eject = true;
    break;
  case 1:
    printf("CD-ROM drive %s is open.\n", psz_drive);
    break;
  default:
    printf("Error status for drive %s: %s.\n", psz_drive,
	   cdio_driver_errmsg(ret));
    return 1;
  }
  
  ret = mmc_get_media_changed(p_cdio);
  switch(ret) {
  case 0:
    printf("CD-ROM drive %s media not changed since last test.\n", psz_drive);
    break;
  case 1:
    printf("CD-ROM drive %s media changed since last test.\n", psz_drive);
    break;
  default:
    printf("Error status for drive %s: %s.\n", psz_drive, 
	   cdio_driver_errmsg(ret));
    return 1;
  }

  if (do_eject)
    ret = cdio_eject_media_drive(psz_drive);
  else
    ret = cdio_close_tray(psz_drive, &driver_id);

  ret = mmc_get_tray_status(p_cdio);
  switch(ret) {
  case 0:
    printf("CD-ROM drive %s is closed.\n", psz_drive);
    break;
  case 1:
    printf("CD-ROM drive %s is open.\n", psz_drive);
    break;
  default:
    printf("Error status for drive %s: %s.\n", psz_drive,
	   cdio_driver_errmsg(ret));
    return 1;
  }
  
  ret = mmc_get_media_changed(p_cdio);
  switch(ret) {
  case 0:
    printf("CD-ROM drive %s media not changed since last test.\n", psz_drive);
    break;
  case 1:
    printf("CD-ROM drive %s media changed since last test.\n", psz_drive);
    break;
  default:
    printf("Error status for drive %s: %s.\n", psz_drive,
	   cdio_driver_errmsg(ret));
    return 1;
  }

  free(psz_drive);
  cdio_destroy(p_cdio);

  return 0;
}

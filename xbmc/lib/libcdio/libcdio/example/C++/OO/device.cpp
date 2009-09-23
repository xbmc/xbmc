/*
  $Id: device.cpp,v 1.3 2006/01/25 07:21:52 rocky Exp $

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

/* Simple program to show drivers installed and what the default
   CD-ROM drive is. See also corresponding C program of a similar
   name. */

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

#define _(x) x

/* Prints out drive capabilities */
static void 
print_drive_capabilities(cdio_drive_read_cap_t  i_read_cap,
			 cdio_drive_write_cap_t i_write_cap,
			 cdio_drive_misc_cap_t  i_misc_cap)
{
  if (CDIO_DRIVE_CAP_ERROR == i_misc_cap) {
    printf("Error in getting drive hardware properties\n");
  } else {
    printf(_("Hardware                    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_FILE  
	   ? "Disk Image"  : "CD-ROM or DVD");
    printf(_("Can eject                   : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_EJECT         ? "Yes" : "No");
    printf(_("Can close tray              : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_CLOSE_TRAY    ? "Yes" : "No");
    printf(_("Can disable manual eject    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_LOCK          ? "Yes" : "No");
    printf(_("Can select juke-box disc    : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_DISC   ? "Yes" : "No");

    printf(_("Can set drive speed         : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_SELECT_SPEED  ? "Yes" : "No");
    printf(_("Can detect if CD changed    : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MEDIA_CHANGED ? "Yes" : "No");
    printf(_("Can read multiple sessions  : %s\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_MULTI_SESSION ? "Yes" : "No");
    printf(_("Can hard reset device       : %s\n\n"), 
	   i_misc_cap & CDIO_DRIVE_CAP_MISC_RESET         ? "Yes" : "No");
  }
  
    
  if (CDIO_DRIVE_CAP_ERROR == i_read_cap) {
      printf("Error in getting drive reading properties\n");
  } else {
    printf("Reading....\n");
    printf(_("  Can play audio            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_AUDIO      ? "Yes" : "No");
    printf(_("  Can read  CD-R            : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_R       ? "Yes" : "No");
    printf(_("  Can read  CD-RW           : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_RW      ? "Yes" : "No");
    printf(_("  Can read  DVD-ROM         : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_DVD_ROM    ? "Yes" : "No");
  }
  

  if (CDIO_DRIVE_CAP_ERROR == i_write_cap) {
      printf("Error in getting drive writing properties\n");
  } else {
    printf("\nWriting....\n");
    printf(_("  Can write CD-RW           : %s\n"), 
	   i_read_cap & CDIO_DRIVE_CAP_READ_CD_RW     ? "Yes" : "No");
    printf(_("  Can write DVD-R           : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_READ_DVD_R    ? "Yes" : "No");
    printf(_("  Can write DVD-RAM         : %s\n"), 
	   i_write_cap & CDIO_DRIVE_CAP_READ_DVD_RAM  ? "Yes" : "No");
  }
}

int
main(int argc, const char *argv[])
{
  CdioDevice device;
  
  if (device.open(NULL)) {
    char *default_device = device.getDevice();
    cdio_drive_read_cap_t  i_read_cap;
    cdio_drive_write_cap_t i_write_cap;
    cdio_drive_misc_cap_t  i_misc_cap;
    
    printf("The driver selected is %s\n", device.getDriverName());

    if (default_device)
      printf("The default device for this driver is %s\n", default_device);

    device.getDriveCap(i_read_cap, i_write_cap, i_misc_cap);
    print_drive_capabilities(i_read_cap, i_write_cap, i_misc_cap);
      
    free(default_device);
    printf("\n");

  } else {
    printf("Problem in trying to find a driver.\n\n");
  }
  
  {
    driver_id_t driver_id;
    for (driver_id=CDIO_MIN_DRIVER; driver_id<=CDIO_MAX_DRIVER; driver_id++)
      if (cdio_have_driver(driver_id))
	printf("We have: %s\n", cdio_driver_describe(driver_id));
      else
	printf("We don't have: %s\n", cdio_driver_describe(driver_id));
  }
  
  return 0;

}

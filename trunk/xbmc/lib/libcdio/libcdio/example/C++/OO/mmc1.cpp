/*
  $Id: mmc1.cpp,v 1.1 2005/11/14 01:15:33 rocky Exp $

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

/* Sample program to show use of the MMC interface. 
   An optional drive name can be supplied as an argument.
   This basically the libdio mmc_get_hwinfo() routine.
   See also corresponding C and non OO C++ program.
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
#include <cdio++/cdio.hpp>

/* Set how long to wait for MMC commands to complete */
#define DEFAULT_TIMEOUT_MS 10000

int
main(int argc, const char *argv[])
{
  CdioDevice device;
  const char *psz_drive = NULL;

  if (argc > 1) psz_drive = argv[1];

  if (!device.open(psz_drive)) {
    printf("Couldn't find CD\n");
    return 1;
  } else {
    int i_status;             /* Result of MMC command */
    char buf[36] = { 0, };    /* Place to hold returned data */
    mmc_cdb_t cdb = {{0, }};  /* Command Descriptor Buffer */

    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_INQUIRY);
    cdb.field[4] = sizeof(buf);

    i_status = device.mmcRunCmd(DEFAULT_TIMEOUT_MS, &cdb, 
				SCSI_MMC_DATA_READ, sizeof(buf), &buf);
    if (i_status == 0) {
      char psz_vendor[CDIO_MMC_HW_VENDOR_LEN+1];
      char psz_model[CDIO_MMC_HW_MODEL_LEN+1];
      char psz_rev[CDIO_MMC_HW_REVISION_LEN+1];
      
      memcpy(psz_vendor, buf + 8, sizeof(psz_vendor)-1);
      psz_vendor[sizeof(psz_vendor)-1] = '\0';
      memcpy(psz_model,
	     buf + 8 + CDIO_MMC_HW_VENDOR_LEN, 
	     sizeof(psz_model)-1);
      psz_model[sizeof(psz_model)-1] = '\0';
      memcpy(psz_rev,
	     buf + 8 + CDIO_MMC_HW_VENDOR_LEN +CDIO_MMC_HW_MODEL_LEN,
	     sizeof(psz_rev)-1);
      psz_rev[sizeof(psz_rev)-1] = '\0';

      printf("Vendor: %s\nModel: %s\nRevision: %s\n",
	     psz_vendor, psz_model, psz_rev);
    } else {
      printf("Couldn't get INQUIRY data (vendor, model, and revision).\n");
    }
  }
  
  return 0;
}

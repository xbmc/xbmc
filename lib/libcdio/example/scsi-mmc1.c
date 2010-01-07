/*
  $Id: scsi-mmc1.c,v 1.4 2005/01/29 20:54:20 rocky Exp $

  Copyright (C) 2004 Rocky Bernstein <rocky@panix.com>
  
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
#include <cdio/cdio.h>
#include <cdio/scsi_mmc.h>
#include <string.h>

/* Set how long to wait for MMC commands to complete */
#define DEFAULT_TIMEOUT_MS 10000

int
main(int argc, const char *argv[])
{
  CdIo_t *p_cdio;

  p_cdio = cdio_open (NULL, DRIVER_UNKNOWN);

  if (NULL == p_cdio) {
    printf("Couldn't find CD\n");
    return 1;
  } else {
    int i_status;                  /* Result of MMC command */
    char buf[36] = { 0, };         /* Place to hold returned data */
    scsi_mmc_cdb_t cdb = {{0, }};  /* Command Descriptor Buffer */

    CDIO_MMC_SET_COMMAND(cdb.field, CDIO_MMC_GPCMD_INQUIRY);
    cdb.field[4] = sizeof(buf);

    i_status = scsi_mmc_run_cmd(p_cdio, DEFAULT_TIMEOUT_MS, 
				&cdb, SCSI_MMC_DATA_READ, 
				sizeof(buf), &buf);
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
  
  cdio_destroy(p_cdio);
  
  return 0;
}

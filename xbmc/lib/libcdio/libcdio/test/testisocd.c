/* $Id: testisocd.c,v 1.5 2007/08/12 12:41:10 rocky Exp $

  Copyright (C) 2003, 2004, 2005, 2006, 2007 Rocky Bernstein 
  <rocky@gnu.org>
  
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

/* Tests reading ISO 9660 info from a CD.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "portable.h"

#include <cdio/cdio.h>
#include <cdio/iso9660.h>
#include <cdio/cd_types.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#define SKIP_TEST_RC 77

int
main(int argc, const char *argv[])
{
  char **ppsz_cd_drives;  /* List of all drives with an ISO9660 filesystem. */
  driver_id_t driver_id;  /* Driver ID found */
  char *psz_drive;        /* Name of drive */
  CdIo_t *p_cdio;

  /* See if we can find a device with a loaded CD-DA in it. If successful
     drive_id will be set.  */
  ppsz_cd_drives = 
    cdio_get_devices_with_cap_ret(NULL, CDIO_FS_ANAL_ISO9660_ANY,
				  true, &driver_id);

  if (ppsz_cd_drives && ppsz_cd_drives[0]) {
    /* Found such a CD-ROM with an ISO 9660 filesystem. Use the first drive in
       the list. */
    psz_drive = strdup(ppsz_cd_drives[0]);
    /* Don't need a list of CD's with CD-DA's any more. */
    cdio_free_device_list(ppsz_cd_drives);
  } else {
    printf("Unable find or access a CD-ROM drive with an ISO-9660 "
	   "filesystem.\n");
    exit(SKIP_TEST_RC);
  }

  p_cdio = cdio_open (psz_drive, driver_id);
  if (!p_cdio) {
    fprintf(stderr, "Sorry, couldn't open %s\n", psz_drive);
    return 1;
  } else {
    /* You make get different results looking up "/" versus "/." and the
       latter may give more complete information. "/" will take information
       from the PVD only, whereas "/." will force a directory read of "/" and
       find "." and in that Rock-Ridge information might be found which fills
       in more stat information that iso9660_fs_find_lsn also will find.
       . Ideally iso9660_fs_stat should be fixed. */
       iso9660_stat_t *p_statbuf = iso9660_fs_stat (p_cdio, "/.");

    if (NULL == p_statbuf) {
      fprintf(stderr, 
	      "Could not get ISO-9660 file information for file /.\n");
      cdio_destroy(p_cdio);
      exit(2);
    } else {
      /* Now try getting the statbuf another way */
      char buf[ISO_BLOCKSIZE];
      char *psz_path = NULL;
      const lsn_t i_lsn = p_statbuf->lsn;
      const iso9660_stat_t *p_statbuf2 = iso9660_fs_find_lsn (p_cdio, i_lsn);
      const iso9660_stat_t *p_statbuf3 = 
	iso9660_fs_find_lsn_with_path (p_cdio, i_lsn, &psz_path);

      /* Compare the two statbufs. */
#if 0
      if (0 != memcmp(p_statbuf, p_statbuf2, sizeof(iso9660_stat_t))) {
#else
      if (p_statbuf->lsn != p_statbuf2->lsn || 
	  p_statbuf->size != p_statbuf2->size ||
	  p_statbuf->type != p_statbuf2->type) {
#endif
	  fprintf(stderr, "File stat information between fs_stat and "
		  "fs_find_lsn isn't the same\n");
	  exit(3);
      }

      if (0 != memcmp(p_statbuf3, p_statbuf2, sizeof(iso9660_stat_t))) {
	  fprintf(stderr, "File stat information between fs_find_lsn and "
		  "fs_find_lsn_with_path isn't the same\n");
	  exit(4);
      }

      if (psz_path != NULL) {
	if (0 != strncmp("/./", psz_path, strlen("/./"))) {
	  fprintf(stderr, "Path returned for fs_find_lsn_with_path "
		  "is not correct should be /./, is %s\n", psz_path);
	  exit(5);
	  free(psz_path);
	}
      } else {
	fprintf(stderr, "Path returned for fs_find_lsn_with_path is NULL\n");
	exit(6);
      }
      
      /* Try reading from the directory. */
      memset (buf, 0, ISO_BLOCKSIZE);
      if ( 0 != cdio_read_data_sectors (p_cdio, buf, i_lsn, ISO_BLOCKSIZE, 1) )
	{
	  fprintf(stderr, "Error reading ISO 9660 file at lsn %lu\n",
		  (long unsigned int) p_statbuf->lsn);
	  exit(7);
	}
      exit(0);
    }
  }
  
  exit(0);
}

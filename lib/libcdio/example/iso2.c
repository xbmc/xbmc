/*
  $Id: iso2.c,v 1.5 2005/01/04 04:40:22 rocky Exp $

  Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>
  
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

/* Simple program to show using libiso9660 to extract a file from a
   cue/bin CD-IMAGE.
 */

/* This is the CD-image with an ISO-9660 filesystem */
#define ISO9660_IMAGE_PATH "../"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "test/isofs-m1.cue"

#define ISO9660_FILENAME "/COPYING.;1"
#define LOCAL_FILENAME "copying"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "portable.h"

#include <cdio/cdio.h>
#include <cdio/iso9660.h>

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

#define my_exit(rc)				\
  fclose (p_outfd);				\
  free(p_statbuf);				\
  cdio_destroy(p_cdio);				\
  return rc;					\
  

int
main(int argc, const char *argv[])
{
  iso9660_stat_t *p_statbuf;
  FILE *p_outfd;
  int i;
  
  CdIo_t *p_cdio = cdio_open (ISO9660_IMAGE, DRIVER_BINCUE);
  
  if (NULL == p_cdio) {
    fprintf(stderr, "Sorry, couldn't open BIN/CUE image %s\n", ISO9660_IMAGE);
    return 1;
  }

  p_statbuf = iso9660_fs_stat (p_cdio, ISO9660_FILENAME);

  if (NULL == p_statbuf) 
    {
      fprintf(stderr, 
	      "Could not get ISO-9660 file information for file %s\n",
	      ISO9660_FILENAME);
      cdio_destroy(p_cdio);
      return 2;
    }

  if (!(p_outfd = fopen ("copying", "wb")))
    {
      perror ("fopen()");
      cdio_destroy(p_cdio);
      free(p_statbuf);
      return 3;
    }

  /* Copy the blocks from the ISO-9660 filesystem to the local filesystem. */
  for (i = 0; i < p_statbuf->size; i += ISO_BLOCKSIZE)
    {
      char buf[ISO_BLOCKSIZE];

      memset (buf, 0, ISO_BLOCKSIZE);
      
      if ( 0 != cdio_read_mode1_sector (p_cdio, buf, 
					p_statbuf->lsn + (i / ISO_BLOCKSIZE),
					false) )
      {
	fprintf(stderr, "Error reading ISO 9660 file at lsn %lu\n",
		(long unsigned int) p_statbuf->lsn + (i / ISO_BLOCKSIZE));
	my_exit(4);
      }
      
      
      fwrite (buf, ISO_BLOCKSIZE, 1, p_outfd);
      
      if (ferror (p_outfd))
	{
	  perror ("fwrite()");
	  my_exit(5);
	}
    }
  
  fflush (p_outfd);

  /* Make sure the file size has the exact same byte size. Without the
     truncate below, the file will a multiple of ISO_BLOCKSIZE.
   */
  if (ftruncate (fileno (p_outfd), p_statbuf->size))
    perror ("ftruncate()");

  printf("Extraction of file 'copying' from %s successful.\n", ISO9660_IMAGE);

  my_exit(0);
}

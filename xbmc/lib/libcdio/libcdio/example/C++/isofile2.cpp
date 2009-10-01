/*
  $Id: isofile2.cpp,v 1.1 2006/04/15 16:12:51 rocky Exp $

  Copyright (C) 2004, 2006 Rocky Bernstein <rocky@panix.com>
  
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

/* Simple program to show using libiso9660 to extract a file from an
   ISO-9660 image.

   If a single argument is given, it is used as the ISO 9660 image to
   use in the extraction. Otherwise a compiled in default ISO 9660 image
   name (that comes with the libcdio distribution) will be used.
 */

/* This is the ISO 9660 image. */
#define ISO9660_IMAGE_PATH "../../"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "test/copying.iso"

#define LOCAL_FILENAME "copying"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "portable.h"

#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/iso9660.h>

#include <stdio.h>

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
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

#define CEILING(x, y) ((x+(y-1))/y)

#define my_exit(rc)				\
  fclose (p_outfd);				\
  free(p_statbuf);				\
  iso9660_close(p_iso);				\
  return rc;					\

int
main(int argc, const char *argv[])
{
  iso9660_stat_t *p_statbuf;
  FILE *p_outfd;
  int i;
  char const *psz_image;
  char const *psz_fname;
  iso9660_t *p_iso;

  if (argc > 3) {
    printf("usage %s [ISO9660-image.ISO [filename]]\n", argv[0]);
    printf("Extracts filename from ISO-9660-image.ISO.\n");
    return 1;
  }
  
  if (argc > 1) 
    psz_image = argv[1];
  else 
    psz_image = ISO9660_IMAGE;

  if (argc > 2) 
    psz_fname = argv[2];
  else 
    psz_fname = LOCAL_FILENAME;

  p_iso = iso9660_open (psz_image);
  
  if (NULL == p_iso) {
    fprintf(stderr, "Sorry, couldn't open ISO 9660 image %s\n", psz_image);
    return 1;
  }

  p_statbuf = iso9660_ifs_stat_translate (p_iso, psz_fname);

  if (NULL == p_statbuf) 
    {
      fprintf(stderr, 
	      "Could not get ISO-9660 file information for file %s\n",
	      psz_fname);
      iso9660_close(p_iso);
      return 2;
    }

  if (!(p_outfd = fopen (psz_fname, "wb")))
    {
      perror ("fopen()");
      free(p_statbuf);
      iso9660_close(p_iso);
      return 3;
    }

  /* Copy the blocks from the ISO-9660 filesystem to the local filesystem. */
  {
    const unsigned int i_blocks = CEILING(p_statbuf->size, ISO_BLOCKSIZE);
    for (i = 0; i < i_blocks ; i++) 
    {
      char buf[ISO_BLOCKSIZE];
      const lsn_t lsn = p_statbuf->lsn + i;

      memset (buf, 0, ISO_BLOCKSIZE);
      
      if ( ISO_BLOCKSIZE != iso9660_iso_seek_read (p_iso, buf, lsn, 1) )
      {
	fprintf(stderr, "Error reading ISO 9660 file %s at LSN %lu\n",
		psz_fname, (long unsigned int) lsn);
	my_exit(4);
      }
      
      fwrite (buf, ISO_BLOCKSIZE, 1, p_outfd);
      
      if (ferror (p_outfd))
	{
	  perror ("fwrite()");
	  my_exit(5);
	}
    }
  }
  
  fflush (p_outfd);

  /* Make sure the file size has the exact same byte size. Without the
     truncate below, the file will a multiple of ISO_BLOCKSIZE.
   */
  if (ftruncate (fileno (p_outfd), p_statbuf->size))
    perror ("ftruncate()");

  printf("Extraction of file '%s' from %s successful.\n", 
	 psz_fname, psz_image);

  my_exit(0);
}

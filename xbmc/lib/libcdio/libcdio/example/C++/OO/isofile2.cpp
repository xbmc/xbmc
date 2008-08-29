/*
  $Id: isofile2.cpp,v 1.1 2006/04/15 16:12:51 rocky Exp $

  Copyright (C) 2006 Rocky Bernstein <rocky@panix.com>
  
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
   CUE/BIN CD image.

   If a single argument is given, it is used as the CUE file of a CD image
   to use. Otherwise a compiled-in default image name (that
   comes with the libcdio distribution) will be used.

   This program can be compiled with either a C or C++ compiler. In 
   the distribution we prefer C++ just to make sure we haven't broken
   things on the C++ side.
 */

/* This is the CD-image with an ISO-9660 filesystem */
#define ISO9660_IMAGE_PATH "../../../"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "test/isofs-m1.cue"

#define ISO9660_PATH "/"
#define ISO9660_FILENAME "COPYING"
#define LOCAL_FILENAME "copying"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "portable.h"

#include <cdio++/cdio.hpp>
#include <cdio++/iso9660.hpp>

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

#define CEILING(x, y) ((x+(y-1))/y)

#define my_exit(rc)				\
  fclose (p_outfd);				\
  delete (p_stat);				\
  delete (p_iso);				\
  return rc;					\
  

int
main(int argc, const char *argv[])
{
  ISO9660::Stat *p_stat;
  FILE *p_outfd;
  unsigned int i;
  char const *psz_image;
  char const *psz_fname;
  char translated_name[256];
  char untranslated_name[256] = ISO9660_PATH;
  ISO9660::FS *p_iso = new ISO9660::FS;
  
  if (argc > 3) {
    printf("usage %s [CD-ROM-or-image [filename]]\n", argv[0]);
    printf("Extracts filename from CD-ROM-or-image.\n");
    return 1;
  }
  
  if (argc > 1) 
    psz_image = argv[1];
  else 
    psz_image = ISO9660_IMAGE;

  if (argc > 2) 
    psz_fname = argv[2];
  else 
    psz_fname = ISO9660_FILENAME;

  strcat(untranslated_name, psz_fname);

  if (!p_iso->open(psz_image, DRIVER_UNKNOWN)) {
    fprintf(stderr, "Sorry, couldn't open %s\n", psz_image);
    return 1;
  }

  p_stat = p_iso->stat(psz_fname);

  if (!p_stat) 
    {
      fprintf(stderr, 
	      "Could not get ISO-9660 file information for file %s\n",
	      untranslated_name);
      delete(p_iso);
      return 2;
    }

  iso9660_name_translate(psz_fname, translated_name);
  
  if (!(p_outfd = fopen (translated_name, "wb")))
    {
      perror ("fopen()");
      delete (p_stat);
      delete (p_iso);
      return 3;
    }

  /* Copy the blocks from the ISO-9660 filesystem to the local filesystem. */
  {
    const unsigned int i_blocks = CEILING(p_stat->p_stat->size, ISO_BLOCKSIZE);
    for (i = 0; i < i_blocks; i ++)
      {
	char buf[ISO_BLOCKSIZE];
	const lsn_t lsn = p_stat->p_stat->lsn + i;
	
	memset (buf, 0, ISO_BLOCKSIZE);

	try {
	  p_iso->readDataBlocks(buf, lsn, ISO_BLOCKSIZE);
	}
	catch ( DriverOpException e ) {
	  fprintf(stderr, "Error reading ISO 9660 file at lsn %lu:\n\t%s.\n",
		  (long unsigned int) lsn, e.get_msg());
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
  if (ftruncate (fileno (p_outfd), p_stat->p_stat->size))
    perror ("ftruncate()");

  printf("Extraction of file '%s' from '%s' successful.\n", 
	 translated_name, untranslated_name);

  my_exit(0);
}

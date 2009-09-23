/*
  $Id: udffile.c,v 1.2 2006/04/17 11:45:22 rocky Exp $

  Copyright (C) 2005, 2006 Rocky Bernstein <rocky@cpan.org>
  
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

/* Simple program to show using libudf to extract a file.

   This program can be compiled with either a C or C++ compiler. In 
   the distribution we prefer C++ just to make sure we haven't broken
   things on the C++ side.
 */

/* This is the UDF image. */
#define UDF_IMAGE_PATH "../"
#define UDF_IMAGE "../test/udf102.iso"
#define UDF_FILENAME "/COPYING"
#define LOCAL_FILENAME "copying"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/udf.h>

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

#define udf_PATH_DELIMITERS "/\\"

int
main(int argc, const char *argv[])
{
  udf_t *p_udf;
  FILE *p_outfd;
  char const *psz_udf_image;
  char const *psz_udf_fname;
  char const *psz_local_fname;

  if (argc > 1) 
    psz_udf_image = argv[1];
  else 
    psz_udf_image = UDF_IMAGE;

  if (argc > 2) 
    psz_udf_fname = argv[2];
  else 
    psz_udf_fname = UDF_FILENAME;

  if (argc > 3) 
    psz_local_fname = argv[3];
  else 
    psz_local_fname = LOCAL_FILENAME;


  p_udf = udf_open (psz_udf_image);
  
  if (NULL == p_udf) {
    fprintf(stderr, "Sorry, couldn't open %s as something using UDF\n", 
	    psz_udf_image);
    return 1;
  } else {
    udf_dirent_t *p_udf_root = udf_get_root(p_udf, true, 0);
    udf_dirent_t *p_udf_file = NULL;
    if (NULL == p_udf_root) {
      fprintf(stderr, "Sorry, couldn't find / in %s\n", 
	      psz_udf_image);
      return 1;
    }
    
    p_udf_file = udf_fopen(p_udf_root, psz_udf_fname);
    if (!p_udf_file) {
      fprintf(stderr, "Sorry, couldn't find %s in %s\n", 
	      psz_udf_fname, psz_udf_image);
      return 2;
      
    }

    if (!(p_outfd = fopen (psz_local_fname, "wb")))
      {
	perror ("fopen()");
	return 3;
    }

    {
      long unsigned int i_file_length = udf_get_file_length(p_udf_file);
      const unsigned int i_blocks = CEILING(i_file_length, UDF_BLOCKSIZE);
      unsigned int i;
      for (i = 0; i < i_blocks ; i++) {
	char buf[UDF_BLOCKSIZE] = {'\0',};
	ssize_t i_read = udf_read_block(p_udf_file, buf, 1);

	if ( i_read < 0 ) {
	  fprintf(stderr, "Error reading UDF file %s at block %u\n",
		  psz_local_fname, i);
	  return 4;
	}

	fwrite (buf, i_read, 1, p_outfd);

	if (ferror (p_outfd)) {
	  perror ("fwrite()");
	  return 5;
	}
      }

      fflush (p_outfd);
      udf_dirent_free(p_udf_root);
      udf_close(p_udf);
      /* Make sure the file size has the exact same byte size. Without the
	 truncate below, the file will a multiple of UDF_BLOCKSIZE.
      */
      if (ftruncate (fileno (p_outfd), i_file_length))
	perror ("ftruncate()");
      
      printf("Extraction of file '%s' from %s successful.\n", 
	     psz_local_fname, psz_udf_image);
      
      return 0;
    }
  }
}


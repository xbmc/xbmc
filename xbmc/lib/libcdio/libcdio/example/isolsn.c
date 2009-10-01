/*
  $Id: isolsn.c,v 1.1 2007/08/12 00:56:10 rocky Exp $

  Copyright (C) 2004, 2005, 2006, 2007 Rocky Bernstein <rocky@gnu.org>
  
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

/* Simple program to show using libiso9660 to get a file path
   for a given LSN of an ISO-9660 image.

   If a single argument is given, it is used as the LSN to search for.
   Otherwise we use a built-in default value.

   If a second argument is given, it is ISO 9660 image to use in the
   listing. Otherwise a compiled-in default ISO 9660 image name (that
   comes with the libcdio distribution) will be used.
 */

/* Set up a CD-DA image to test on which is in the libcdio distribution. */
#define ISO9660_IMAGE_PATH "../test/"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "copying.iso"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/iso9660.h>

#include <stdio.h>

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

#define print_vd_info(title, fn)	  \
  if (fn(p_iso, &psz_str)) {		  \
    printf(title ": %s\n", psz_str);	  \
  }					  \
  free(psz_str);			  \
  psz_str = NULL;			  


int
main(int argc, const char *argv[])
{
  char const *psz_fname;
  iso9660_t *p_iso;
  lsn_t lsn = 24;
  char *psz_path = NULL;
  
  if (argc > 1) 
    lsn = strtol(argv[1], (char **)NULL, 10);

  if (argc > 2) 
    psz_fname = argv[2];
  else 
    psz_fname = ISO9660_IMAGE;

  p_iso = iso9660_open (psz_fname);
  
  if (NULL == p_iso) {
    fprintf(stderr, "Sorry, couldn't open %s as an ISO-9660 image\n", 
	    psz_fname);
    return 1;
  }

  iso9660_ifs_find_lsn_with_path (p_iso, lsn, &psz_path);
  if (psz_path != NULL) {
    printf("File at LSN %u is %s\n", lsn, psz_path);
    free(psz_path);
  }

  iso9660_close(p_iso);
  return 0;
}


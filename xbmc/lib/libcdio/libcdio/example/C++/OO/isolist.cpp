/*
  $Id: isolist.cpp,v 1.1 2006/04/15 16:12:51 rocky Exp $

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

/* Simple program to show using libiso9660 to list files in a directory of
   an ISO-9660 image and give some iso9660 information. See the code
   to iso-info for a more complete example.

   If a single argument is given, it is used as the ISO 9660 image to
   use in the listing. Otherwise a compiled-in default ISO 9660 image
   name (that comes with the libcdio distribution) will be used.

   This program can be compiled with either a C or C++ compiler. In 
   the distributuion we perfer C++ just to make sure we haven't broken
   things on the C++ side.
 */

/* Set up a CD-DA image to test on which is in the libcdio distribution. */
#define ISO9660_IMAGE_PATH "../../../"
#define ISO9660_IMAGE ISO9660_IMAGE_PATH "test/copying.iso"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/types.h>
#include <cdio++/iso9660.hpp>

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
  if (p_iso->fn(psz_str)) {		  \
    printf(title ": %s\n", psz_str);	  \
  }					  \
  free(psz_str);			  \
  psz_str = NULL;			  


int
main(int argc, const char *argv[])
{
  stat_vector_t stat_vector;
  ISO9660::IFS *p_iso = new ISO9660::IFS;
  char const *psz_fname;
  const char *psz_path="/";

  if (argc > 1) 
    psz_fname = argv[1];
  else 
    psz_fname = ISO9660_IMAGE;

  if (!p_iso->open(psz_fname)) {
    fprintf(stderr, "Sorry, couldn't open %s as an ISO-9660 image\n", 
	    psz_fname);
    return 1;
  }

  /* Show basic CD info from the Primary Volume Descriptor. */
  {
    char *psz_str = NULL;
    print_vd_info("Application", get_application_id);
    print_vd_info("Preparer   ", get_preparer_id);
    print_vd_info("Publisher  ", get_publisher_id);
    print_vd_info("System     ", get_system_id);
    print_vd_info("Volume     ", get_volume_id);
    print_vd_info("Volume Set ", get_volumeset_id);
  }

  if (p_iso->readdir (psz_path, stat_vector))
  {
    /* Iterate over the list of files.  */
    stat_vector_iterator_t i;
    for(i=stat_vector.begin(); i != stat_vector.end(); ++i)
      {
	char filename[4096];
	ISO9660::Stat *p_s = *i;
	iso9660_name_translate(p_s->p_stat->filename, filename);
	printf ("%s [LSN %6d] %8u %s%s\n", 
		2 == p_s->p_stat->type ? "d" : "-",
		p_s->p_stat->lsn, p_s->p_stat->size, psz_path, filename);
	delete(p_s);
      }
    
    stat_vector.clear();
  }

  delete(p_iso);
  return 0;
}


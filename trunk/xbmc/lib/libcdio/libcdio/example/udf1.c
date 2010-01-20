/*
  $Id: udf1.c,v 1.18 2008/01/09 04:27:16 rocky Exp $

  Copyright (C) 2005, 2008 Rocky Bernstein <rocky@gnu.org>
  
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

/* Simple program to show using libudf to list files in a directory of
   an UDF image.
 */

/* This is the UDF image. */
#define UDF_IMAGE_PATH "../"
#define UDF_IMAGE "/src2/cd-images/udf/UDF102ISO.iso"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <sys/types.h>
#include <cdio/cdio.h>
#include <cdio/udf.h>

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

#define udf_PATH_DELIMITERS "/\\"

static void 
print_file_info(const udf_dirent_t *p_udf_dirent, const char* psz_dirname)
{
  time_t mod_time = udf_get_modification_time(p_udf_dirent);
  char psz_mode[11]="invalid";
  const char *psz_fname= psz_dirname 
    ? psz_dirname : udf_get_filename(p_udf_dirent);

  /* Print directory attributes*/
  printf("%s ", udf_mode_string(udf_get_posix_filemode(p_udf_dirent),
				psz_mode));
  printf("%4d ", udf_get_link_count(p_udf_dirent));
  printf("%lu ", (long unsigned int) udf_get_file_length(p_udf_dirent));
  printf("%s %s",  *psz_fname ? psz_fname : "/", ctime(&mod_time));
}

static udf_dirent_t *
list_files(udf_t *p_udf, udf_dirent_t *p_udf_dirent, const char *psz_path)
{
  if (!p_udf_dirent) return NULL;
  
  print_file_info(p_udf_dirent, psz_path);

  while (udf_readdir(p_udf_dirent)) {
      
    if (udf_is_dir(p_udf_dirent)) {
      
      udf_dirent_t *p_udf_dirent2 = udf_opendir(p_udf_dirent);
      if (p_udf_dirent2) {
	const char *psz_dirname = udf_get_filename(p_udf_dirent);
	const unsigned int i_newlen=2 + strlen(psz_path) + strlen(psz_dirname);
	char *psz_newpath = calloc(1, sizeof(char)*i_newlen);
	
	snprintf(psz_newpath, i_newlen, "%s%s/", psz_path, psz_dirname);
	list_files(p_udf, p_udf_dirent2, psz_newpath);
	free(psz_newpath);
      }
    } else {
      print_file_info(p_udf_dirent, NULL);
    }
  }
  return p_udf_dirent;
}

int
main(int argc, const char *argv[])
{
  udf_t *p_udf;
  char const *psz_udf_image;

  if (argc > 1) 
    psz_udf_image = argv[1];
  else 
    psz_udf_image = UDF_IMAGE;

  p_udf = udf_open (psz_udf_image);
  
  if (NULL == p_udf) {
    fprintf(stderr, "Sorry, couldn't open %s as something using UDF\n", 
	    psz_udf_image);
    return 1;
  } else {
    udf_dirent_t *p_udf_root = udf_get_root(p_udf, true, 0);
    if (NULL == p_udf_root) {
      fprintf(stderr, "Sorry, couldn't find / in %s\n", 
	      psz_udf_image);
      return 1;
    }
    
    {
      char vol_id[UDF_VOLID_SIZE] = "";
      char volset_id[UDF_VOLSET_ID_SIZE+1] = "";
      
      if (0 < udf_get_volume_id(p_udf, vol_id, sizeof(vol_id)) )
	printf("volume id: %s\n", vol_id);

      if (0 < udf_get_volume_id(p_udf, volset_id, sizeof(volset_id)) ) {
	volset_id[UDF_VOLSET_ID_SIZE]='\0';
	printf("volume set id: %s\n", volset_id);
      }

      printf("partition number: %d\n", udf_get_part_number(p_udf));


    }
    
    list_files(p_udf, p_udf_root, "");
  }
  
  udf_close(p_udf);
  return 0;
}


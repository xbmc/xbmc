/*
    $Id: testiso9660.c,v 1.3 2004/10/26 01:21:05 rocky Exp $

    Copyright (C) 2003 Rocky Bernstein <rocky@panix.com>

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
/* Tests ISO9660 library routines. */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <ctype.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#include <cdio/iso9660.h>

int
main (int argc, const char *argv[])
{
  int c;
  int i;
  char dst[100];
  char *dst_p;
  int achars[] = {'!', '"', '%', '&', '(', ')', '*', '+', ',', '-', '.',
                  '/', '?', '<', '=', '>'};
  for (c='A'; c<='Z'; c++ ) {
    if (!iso9660_isdchar(c)) {
      printf("Failed iso9660_isdchar test on %d\n", c);
      return c;
    }
    if (!iso9660_isachar(c)) {
      printf("Failed iso9660_isachar test on %d\n", c);
      return c;
    }
  }
  for (c='0'; c<='9'; c++ ) {
    if (!iso9660_isdchar(c)) {
      printf("Failed iso9660_isdchar test on %d\n", c);
      return c;
    }
    if (!iso9660_isachar(c)) {
      printf("Failed iso9660_isachar test on %d\n", c);
      return c;
    }
  }

  for (i=0; i<=13; i++ ) {
    c=achars[i];
    if (iso9660_isdchar(c)) {
      printf("Should not pass iso9660_isdchar test on %d\n", c);
      return c;
    }
    if (!iso9660_isachar(c)) {
      printf("Failed iso9660_isachar test on symbol %d\n", c);
      return c;
    }
  }

  /* Test iso9660_strncpy_pad */
  dst_p = iso9660_strncpy_pad(dst, "1_3", 5, ISO9660_DCHARS);
  if ( 0 != strncmp(dst, "1_3  ", 5) ) {
    printf("Failed iso9660_strncpy_pad test 1\n");
    return 1;
  }
  dst_p = iso9660_strncpy_pad(dst, "ABC!123", 2, ISO9660_ACHARS);
  if ( 0 != strncmp(dst, "AB", 2) ) {
    printf("Failed iso9660_strncpy_pad test 2\n");
    return 2;
  }

  /* Test iso9660_dirname_valid_p */
  if ( iso9660_dirname_valid_p("/NOGOOD") ) {
    printf("/NOGOOD should fail iso9660_dirname_valid_p\n");
    return 3;
  }
  if ( iso9660_dirname_valid_p("LONGDIRECTORY/NOGOOD") ) {
    printf("LONGDIRECTORY/NOGOOD should fail iso9660_dirname_valid_p\n");
    return 4;
  }
  if ( !iso9660_dirname_valid_p("OKAY/DIR") ) {
    printf("OKAY/DIR should pass iso9660_dirname_valid_p\n");
    return 5;
  }
  if ( iso9660_dirname_valid_p("OKAY/FILE.EXT") ) {
    printf("OKAY/FILENAME.EXT should fail iso9660_dirname_valid_p\n");
    return 6;
  }

  /* Test iso9660_pathname_valid_p */
  if ( !iso9660_pathname_valid_p("OKAY/FILE.EXT") ) {
    printf("OKAY/FILE.EXT should pass iso9660_dirname_valid_p\n");
    return 7;
  }
  if ( iso9660_pathname_valid_p("OKAY/FILENAMETOOLONG.EXT") ) {
    printf("OKAY/FILENAMETOOLONG.EXT should fail iso9660_dirname_valid_p\n");
    return 8;
  }
  if ( iso9660_pathname_valid_p("OKAY/FILE.LONGEXT") ) {
    printf("OKAY/FILE.LONGEXT should fail iso9660_dirname_valid_p\n");
    return 9;
  }

  dst_p = iso9660_pathname_isofy ("this/file.ext", 1);
  if ( 0 != strncmp(dst_p, "this/file.ext;1", 16) ) {
    printf("Failed iso9660_pathname_isofy\n");
    free(dst_p);
    return 10;
  }
  free(dst_p);

  /* Test get/set date */
  {
    struct tm *p_tm, tm;
    iso9660_dtime_t dtime;
    time_t now = time(NULL);

    memset(&dtime, 0, sizeof(dtime));
    p_tm = localtime(&now);
    iso9660_set_dtime(p_tm, &dtime);
    iso9660_get_dtime(&dtime, true, &tm);
    if ( memcmp(p_tm, &tm, sizeof(tm)) != 0 ) {
      printf("Time retrieved with iso0660_get_dtime not same as that set with "
             "iso9660_set_dtime.\n");
      return 11;
    }
    p_tm = gmtime(&now);
    iso9660_set_dtime(p_tm, &dtime);
    iso9660_get_dtime(&dtime, false, &tm);
    if ( memcmp(p_tm, &tm, sizeof(tm)) != 0 ) {
      printf("Time retrieved with iso0660_get_dtime not same as that set with "
             "iso9660_set_dtime.\n");
      return 12;
    }
  }

  return 0;
}

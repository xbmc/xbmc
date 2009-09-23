/*
    $Id: testiso9660.c,v 1.20 2007/11/16 22:44:57 flameeyes Exp $

    Copyright (C) 2003, 2006, 2007  Rocky Bernstein <rocky@gnu.org>

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

static bool 
time_compare(struct tm *p_tm1, struct tm *p_tm2) 
{
  bool okay = true;
  if (!p_tm1) {
    printf("get time is NULL\n");
    return false;
  }
  if (!p_tm2) {
    printf("set time is NULL\n");
    return false;
  }
  if (p_tm1->tm_year != p_tm2->tm_year) {
    printf("Years aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_year, p_tm2->tm_year);
    okay=false;
  }
  if (p_tm1->tm_mon != p_tm2->tm_mon) {
    printf("Months aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_mon, p_tm2->tm_mon);
    okay=false;
  }
  if (p_tm1->tm_mday != p_tm2->tm_mday) {
    printf("Month days aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_mday, p_tm2->tm_mday);
    okay=false;
  }
  if (p_tm1->tm_min != p_tm2->tm_min) {
    printf("minute aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_min, p_tm2->tm_min);
    okay=false;
  }
#if FIXED
  /* Differences in daylight savings time may make these different.*/
  if (p_tm1->tm_hour != p_tm2->tm_hour) {
    printf("hours aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_hour, p_tm2->tm_hour);
    okay=false;
  }
#endif
  if (p_tm1->tm_sec != p_tm2->tm_sec) {
    printf("seconds aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_sec, p_tm2->tm_sec);
    okay=false;
  }
  if (p_tm1->tm_wday != p_tm2->tm_wday) {
    printf("Week days aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_wday, p_tm2->tm_wday);
    okay=false;
  }
  if (p_tm1->tm_yday != p_tm2->tm_yday) {
    printf("Year days aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_yday, p_tm2->tm_yday);
    okay=false;
  }
#if FIXED
  if (p_tm1->tm_isdst != p_tm2->tm_isdst) {
    printf("Is daylight savings times aren't equal. get: %d, set %d\n", 
	   p_tm1->tm_isdst, p_tm2->tm_isdst);
    okay=false;
  }
#ifdef HAVE_TM_GMTOFF
  if (p_tm1->tm_gmtoff != p_tm2->tm_gmtoff) {
    printf("GMT offsets aren't equal. get: %ld, set %ld\n", 
	   p_tm1->tm_gmtoff, p_tm2->tm_gmtoff);
    okay=false;
  }
  if (p_tm1 != p_tm2 && p_tm1 && p_tm2) {
    if (strcmp(p_tm1->tm_zone, p_tm2->tm_zone) != 0) {
      printf("Time Zone values. get: %s, set %s\n", 
	     p_tm1->tm_zone, p_tm2->tm_zone);
      /* Argh... sometimes GMT is converted to UTC. So
	 Let's not call this a failure if everything else was okay.
       */
    }
  }
#endif
#endif
  return okay;
}

int
main (int argc, const char *argv[])
{
  int c;
  int i;
  int i_bad = 0;
  char dst[100];
  char *dst_p;
  int achars[] = {'!', '"', '%', '&', '(', ')', '*', '+', ',', '-', '.',
                  '/', '?', '<', '=', '>'};

  /*********************************************
   * Test ACHAR and DCHAR
   *********************************************/

  for (c='A'; c<='Z'; c++ ) {
    if (!iso9660_is_dchar(c)) {
      printf("Failed iso9660_is_dchar test on %c\n", c);
      i_bad++;
    }
    if (!iso9660_is_achar(c)) {
      printf("Failed iso9660_is_achar test on %c\n", c);
      i_bad++;
    }
  }

  if (i_bad) return i_bad;
  
  for (c='0'; c<='9'; c++ ) {
    if (!iso9660_is_dchar(c)) {
      printf("Failed iso9660_is_dchar test on %c\n", c);
      i_bad++;
    }
    if (!iso9660_is_achar(c)) {
      printf("Failed iso9660_is_achar test on %c\n", c);
      i_bad++;
    }
  }

  if (i_bad) return i_bad;

  for (i=0; i<=13; i++ ) {
    c=achars[i];
    if (iso9660_is_dchar(c)) {
      printf("Should not pass iso9660_is_dchar test on %c\n", c);
      i_bad++;
    }
    if (!iso9660_is_achar(c)) {
      printf("Failed iso9660_is_achar test on symbol %c\n", c);
      i_bad++;
    }
  }

  if (i_bad) return i_bad;

  /*********************************************
   * Test iso9660_strncpy_pad
   *********************************************/

  dst_p = iso9660_strncpy_pad(dst, "1_3", 5, ISO9660_DCHARS);
  if ( 0 != strncmp(dst, "1_3  ", 5) ) {
    printf("Failed iso9660_strncpy_pad DCHARS\n");
    return 31;
  }
  dst_p = iso9660_strncpy_pad(dst, "ABC!123", 2, ISO9660_ACHARS);
  if ( 0 != strncmp(dst, "AB", 2) ) {
    printf("Failed iso9660_strncpy_pad ACHARS truncation\n");
    return 32;
  }

  /*********************************************
   * Test iso9660_dirname_valid_p 
   *********************************************/

  if ( iso9660_dirname_valid_p("/NOGOOD") ) {
    printf("/NOGOOD should fail iso9660_dirname_valid_p\n");
    return 33;
  }
  if ( iso9660_dirname_valid_p("LONGDIRECTORY/NOGOOD") ) {
    printf("LONGDIRECTORY/NOGOOD should fail iso9660_dirname_valid_p\n");
    return 34;
  }
  if ( !iso9660_dirname_valid_p("OKAY/DIR") ) {
    printf("OKAY/DIR should pass iso9660_dirname_valid_p\n");
    return 35;
  }
  if ( iso9660_dirname_valid_p("OKAY/FILE.EXT") ) {
    printf("OKAY/FILENAME.EXT should fail iso9660_dirname_valid_p\n");
    return 36;
  }

  /*********************************************
   * Test iso9660_pathname_valid_p 
   *********************************************/

  if ( !iso9660_pathname_valid_p("OKAY/FILE.EXT") ) {
    printf("OKAY/FILE.EXT should pass iso9660_dirname_valid_p\n");
    return 37;
  }
  if ( iso9660_pathname_valid_p("OKAY/FILENAMETOOLONG.EXT") ) {
    printf("OKAY/FILENAMETOOLONG.EXT should fail iso9660_dirname_valid_p\n");
    return 38;
  }
  if ( iso9660_pathname_valid_p("OKAY/FILE.LONGEXT") ) {
    printf("OKAY/FILE.LONGEXT should fail iso9660_dirname_valid_p\n");
    return 39;
  }

  dst_p = iso9660_pathname_isofy ("this/file.ext", 1);
  if ( 0 != strncmp(dst_p, "this/file.ext;1", 16) ) {
    printf("Failed iso9660_pathname_isofy\n");
    free(dst_p);
    return 40;
  }
  free(dst_p);

  /*********************************************
   * Test get/set date 
   *********************************************/

  {
    struct tm *p_tm, tm;
    iso9660_dtime_t dtime;
    iso9660_ltime_t ltime;
    time_t now = time(NULL);

    memset(&dtime, 0, sizeof(dtime));
    p_tm = localtime(&now);
    iso9660_set_dtime(p_tm, &dtime);
    iso9660_get_dtime(&dtime, true, &tm);
    if ( memcmp(p_tm, &tm, sizeof(tm)) != 0 ) {
      printf("Local time retrieved with iso9660_get_dtime not same as\n");
      printf("that set with iso9660_set_dtime().\n");
      return 41;
    }
    p_tm = gmtime(&now);
    iso9660_set_dtime(p_tm, &dtime);
    if (!iso9660_get_dtime(&dtime, false, &tm)) {
      printf("Error returned by iso9660_get_dtime\n");
      return 42;
    }
    
    if ( memcmp(p_tm, &tm, sizeof(tm)) != 0 ) {
      printf("GMT time retrieved with iso9660_get_dtime() not same as that\n");
      printf("set with iso9660_set_dtime().\n");
      return 43;
    }
    
    {
        time_t t1, t2;
	p_tm = localtime(&now);
	t1 = mktime(p_tm);
	iso9660_set_ltime(p_tm, &ltime);
	  
	if (!iso9660_get_ltime(&ltime, &tm)) {
	  printf("Problem running iso9660_get_ltime\n");
	  return 44;
	}

#ifdef FIXED	
	t2 = mktime(&tm);
	if ( t1 != t2  && ! time_compare(p_tm, &tm) ) {
	  printf("local time retrieved with iso9660_get_ltime() not\n");
	  printf("same as that set with iso9660_set_ltime().\n");
	  return 45;
	}
#endif

	p_tm = gmtime(&now);
	t1 = mktime(p_tm);
	iso9660_set_ltime(p_tm, &ltime);
	iso9660_get_ltime(&ltime, &tm);
	t2 = mktime(&tm);
	if ( t1 != t2 && ! time_compare(p_tm, &tm) ) {
	  printf("GMT time retrieved with iso9660_get_ltime() not\n");
	  printf("same as that set with iso9660_set_ltime().\n");
	  return 46;
	}
    }
  }

  return 0;
}

/* Work around the bug in some systems whereby gettimeofday clobbers the
   static buffer that localtime uses for it's return value.  The gettimeofday
   function from Mac OS X 10.0.4, i.e. Darwin 1.3.7 has this problem.
   The tzset replacement is necessary for at least Solaris 2.5, 2.5.1, and 2.6.
   Copyright (C) 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* written by Jim Meyering */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* Disable the definitions of these functions (from config.h)
   so we can use the library versions here.  */
#undef gettimeofday
#undef gmtime
#undef localtime
#undef tzset

#include <sys/types.h>

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <stdlib.h>

static struct tm *localtime_buffer_addr;

/* This is a wrapper for localtime.  It is used only on systems for which
   gettimeofday clobbers the static buffer used for localtime's result.

   On the first call, record the address of the static buffer that
   localtime uses for its result.  */

struct tm *
rpl_localtime (const time_t *timep)
{
  struct tm *tm = localtime (timep);

  if (! localtime_buffer_addr)
    localtime_buffer_addr = tm;

  return tm;
}

/* Same as above, since gmtime and localtime use the same buffer.  */
struct tm *
rpl_gmtime (const time_t *timep)
{
  struct tm *tm = gmtime (timep);

  if (! localtime_buffer_addr)
    localtime_buffer_addr = tm;

  return tm;
}

/* This is a wrapper for gettimeofday.  It is used only on systems for which
   gettimeofday clobbers the static buffer used for localtime's result.

   Save and restore the contents of the buffer used for localtime's result
   around the call to gettimeofday.  */

int
rpl_gettimeofday (struct timeval *tv, struct timezone *tz)
{
  struct tm save;
  int result;

  if (! localtime_buffer_addr)
    {
      time_t t = 0;
      localtime_buffer_addr = localtime (&t);
    }

  save = *localtime_buffer_addr;
  result = gettimeofday (tv, tz);
  *localtime_buffer_addr = save;

  return result;
}

/* This is a wrapper for tzset. It is used only on systems for which
   tzset may clobber the static buffer used for localtime's result.
   Save and restore the contents of the buffer used for localtime's
   result around the call to tzset.  */
void
rpl_tzset (void)
{
  struct tm save;

  if (! localtime_buffer_addr)
    {
      time_t t = 0;
      localtime_buffer_addr = localtime (&t);
    }

  save = *localtime_buffer_addr;
  tzset ();
  *localtime_buffer_addr = save;
}

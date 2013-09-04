/* Pausing execution of the current thread.
   Copyright (C) 2007, 2009-2011 Free Software Foundation, Inc.
   Written by Bruno Haible <bruno@clisp.org>, 2007.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

/* Specification.  */
#include <unistd.h>

#include <limits.h>

#include "verify.h"

#if (defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__

# define WIN32_LEAN_AND_MEAN  /* avoid including junk */
# include <windows.h>

unsigned int
sleep (unsigned int seconds)
{
  unsigned int remaining;

  /* Sleep for 1 second many times, because
       1. Sleep is not interruptiple by Ctrl-C,
       2. we want to avoid arithmetic overflow while multiplying with 1000.  */
  for (remaining = seconds; remaining > 0; remaining--)
    Sleep (1000);

  return remaining;
}

#elif HAVE_SLEEP

# undef sleep

/* Guarantee unlimited sleep and a reasonable return value.  Cygwin
   1.5.x rejects attempts to sleep more than 49.7 days (2**32
   milliseconds), but uses uninitialized memory which results in a
   garbage answer.  Similarly, Linux 2.6.9 with glibc 2.3.4 has a too
   small return value when asked to sleep more than 24.85 days.  */
unsigned int
rpl_sleep (unsigned int seconds)
{
  /* This requires int larger than 16 bits.  */
  verify (UINT_MAX / 24 / 24 / 60 / 60);
  const unsigned int limit = 24 * 24 * 60 * 60;
  while (limit < seconds)
    {
      unsigned int result;
      seconds -= limit;
      result = sleep (limit);
      if (result)
        return seconds + result;
    }
  return sleep (seconds);
}

#else /* !HAVE_SLEEP */

 #error "Please port gnulib sleep.c to your platform, possibly using usleep() or select(), then report this to bug-gnulib."

#endif

/* Pausing execution of the current thread.
   Copyright (C) 2007 Free Software Foundation, Inc.
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

#else

 #error "Please port gnulib sleep.c to your platform, possibly using usleep() or select(), then report this to bug-gnulib."

#endif

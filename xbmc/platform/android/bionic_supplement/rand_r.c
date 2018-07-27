/*
 *  Reentrant random function frm POSIX.1c.
 *  Copyright (C) 1996, 1999 Free Software Foundation, Inc.
 *  This file is part of the GNU C Library.
 *  Contributed by Ulrich Drepper <drepper@cygnus.com <mailto:drepper@cygnus.com>>, 1996.
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#include <stdlib.h>


/* This algorithm is mentioned in the ISO C standard, here extended
   for 32 bits.  */
int rand_r (unsigned int *seed)
{
    unsigned int next = *seed;
    int result;

    next *= 1103515245;
    next += 12345;
    result = (unsigned int) (next / 65536) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= (unsigned int) (next / 65536) % 1024;

    *seed = next;

    return result;
}

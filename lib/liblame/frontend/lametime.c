/*
 *	Lame time routines source file
 *
 *	Copyright (c) 2000 Mark Taylor
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/* $Id: lametime.c,v 1.18.8.1 2009/01/18 15:44:28 robert Exp $ */

/*
 * name:        GetCPUTime ( void )
 *
 * description: returns CPU time used by the process
 * input:       none
 * output:      time in seconds
 * known bugs:  may not work in SMP and RPC
 * conforming:  ANSI C
 *
 * There is some old difficult to read code at the end of this file.
 * Can someone integrate this into this function (if useful)?
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <time.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#endif

#include "lametime.h"

#if !defined(CLOCKS_PER_SEC)
# warning Your system does not define CLOCKS_PER_SEC, guessing one...
# define CLOCKS_PER_SEC 1000000
#endif


double
GetCPUTime(void)
{
    clock_t t;

#if defined(_MSC_VER)  ||  defined(__BORLANDC__)
    t = clock();
#else
    t = clock();
#endif
    return t / (double) CLOCKS_PER_SEC;
}


/*
 * name:        GetRealTime ( void )
 *
 * description: returns real (human) time elapsed relative to a fixed time (mostly 1970-01-01 00:00:00)
 * input:       none
 * output:      time in seconds
 * known bugs:  bad precision with time()
 */

#if defined(__unix__)  ||  defined(SVR4)  ||  defined(BSD)

# include <sys/time.h>
# include <unistd.h>

double
GetRealTime(void)
{                       /* conforming:  SVr4, BSD 4.3 */
    struct timeval t;

    if (0 != gettimeofday(&t, NULL))
        assert(0);
    return t.tv_sec + 1.e-6 * t.tv_usec;
}

#elif defined(WIN16)  ||  defined(WIN32)

# include <stdio.h>
# include <sys/types.h>
# include <sys/timeb.h>

double
GetRealTime(void)
{                       /* conforming:  Win 95, Win NT */
    struct timeb t;

    ftime(&t);
    return t.time + 1.e-3 * t.millitm;
}

#else

double
GetRealTime(void)
{                       /* conforming:  SVr4, SVID, POSIX, X/OPEN, BSD 4.3 */ /* BUT NOT GUARANTEED BY ANSI */
    time_t  t;

    t = time(NULL);
    return (double) t;
}

#endif


#if defined(_WIN32) || defined(__CYGWIN__)
# include <io.h>
# include <fcntl.h>
#else
# include <unistd.h>
#endif

int
lame_set_stream_binary_mode(FILE * const fp)
{
#if   defined __EMX__
    _fsetmode(fp, "b");
#elif defined __BORLANDC__
    setmode(_fileno(fp), O_BINARY);
#elif defined __CYGWIN__
    setmode(fileno(fp), _O_BINARY);
#elif defined _WIN32
    _setmode(_fileno(fp), _O_BINARY);
#else
    (void) fp;          /* doing nothing here, silencing the compiler only. */
#endif
    return 0;
}


#if defined(__riscos__)
# include <kernel.h>
# include <sys/swis.h>
#elif defined(_WIN32)
# include <sys/types.h>
# include <sys/stat.h>
#else
# include <sys/stat.h>
#endif

off_t
lame_get_file_size(const char *const filename)
{
    struct stat sb;

    if (0 == stat(filename, &sb))
        return sb.st_size;
    return (off_t) - 1;
}

/* End of lametime.c */

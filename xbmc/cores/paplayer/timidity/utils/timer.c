/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#if defined(HAVE_GETTIMEOFDAY)
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "timidity.h"
#include "timer.h"

#if defined(nec_ews)
#ifndef GETTIMEOFDAY_ONE_ARGUMENT
#define GETTIMEOFDAY_ONE_ARGUMENT
#endif /* GETTIMEOFDAY_ONE_ARGUMENT */
#endif

double get_current_calender_time(void)
{
    struct timeval tv;
    struct timezone dmy;

#ifdef GETTIMEOFDAY_ONE_ARGUMENT
    gettimeofday(&tv);
#else
    gettimeofday(&tv, &dmy);
#endif

    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0 ;
}

#elif __MACOS__

#include "timer.h"
double get_current_calender_time(void)
{
    UnsignedWide usec;
    Microseconds(&usec);
    return (double)usec.hi* 4294.967296 + (double)usec.lo / 1000000.0 ;
}

#else /* Windows API */

#include <sys/types.h>
#include <time.h>
#include <windows.h>
#include "timidity.h"
#include "timer.h"
double get_current_calender_time(void)
{
    static DWORD tick_start;
    static int init_flag = 0;

    if(init_flag == 0)
    {
	init_flag = 1;
	tick_start = GetTickCount();
	return 0.0;
    }

    return (GetTickCount() - tick_start) * (1.0/1000.0);
}
#endif

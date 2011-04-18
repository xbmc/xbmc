/*
 *      Copyright (C) 2005-2011 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "os_windows.h"
#include <sys/timeb.h>

int gettimeofday(struct timeval *pcur_time, struct timezone *tz)
{
  if (pcur_time == NULL)
  {
    SetLastError(EFAULT);
    return -1;
  }
  struct _timeb current;

  _ftime(&current);

  pcur_time->tv_sec = current.time;
  pcur_time->tv_usec = current.millitm * 1000L;
  if (tz)
  {
    tz->tz_minuteswest = current.timezone;	/* minutes west of Greenwich  */
    tz->tz_dsttime = current.dstflag;	/* type of dst correction  */
  }
  return 0;
}

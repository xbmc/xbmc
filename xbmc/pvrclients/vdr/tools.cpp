/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "tools.h"

static bool GetAbsTime(struct timespec *Abstime, int MillisecondsFromNow)
{
  struct timeval now;
  if (gettimeofday(&now, NULL) == 0)           // get current time
  {
    now.tv_sec  += MillisecondsFromNow / 1000;  // add full seconds
    now.tv_usec += (MillisecondsFromNow % 1000) * 1000;  // add microseconds
    if (now.tv_usec >= 1000000)               // take care of an overflow
    {
      now.tv_sec++;
      now.tv_usec -= 1000000;
    }
    Abstime->tv_sec = now.tv_sec;          // seconds
    Abstime->tv_nsec = now.tv_usec * 1000; // nano seconds
    return true;
  }
  return false;
}

ssize_t safe_read(int filedes, void *buffer, size_t size)
{
  for (;;)
  {
    ssize_t p = read(filedes, buffer, size);
#if defined(_WIN32) || defined(_WIN64)
		if (p < 0 && WSAGetLastError() == WSAEINTR)
#else
		if (p < 0 && errno == EINTR)
#endif
    {
     XBMC_log(LOG_DEBUG, "EINTR while reading from file handle %d - retrying", filedes);
     continue;
    }
    return p;
  }
}

ssize_t safe_write(int filedes, const void *buffer, size_t size)
{
  ssize_t p = 0;
  ssize_t written = size;
  const unsigned char *ptr = (const unsigned char *)buffer;
  while (size > 0)
  {
    p = write(filedes, ptr, size);
    if (p < 0)
    {
#if defined(_WIN32) || defined(_WIN64)
			if (WSAGetLastError() == WSAEINTR)
#else
			if (errno == EINTR)
#endif
      {
        XBMC_log(LOG_DEBUG, "EINTR while writing to file handle %d - retrying", filedes);
        continue;
      }
      break;
    }
    ptr  += p;
    size -= p;
  }
  return p < 0 ? p : written;
}

// --- cPoller ---------------------------------------------------------------

cPoller::cPoller(int FileHandle, bool Out)
{
  numFileHandles = 0;
  Add(FileHandle, Out);
}

bool cPoller::Add(int FileHandle, bool Out)
{
  if (FileHandle >= 0) {
     for (int i = 0; i < numFileHandles; i++) {
         if (pfd[i].fd == FileHandle && pfd[i].events == (Out ? POLLOUT : POLLIN))
            return true;
         }
     if (numFileHandles < MaxPollFiles) {
        pfd[numFileHandles].fd = FileHandle;
        pfd[numFileHandles].events = Out ? POLLOUT : POLLIN;
        pfd[numFileHandles].revents = 0;
        numFileHandles++;
        return true;
        }
     XBMC_log(LOG_ERROR, "too many file handles in cPoller");
     }
  return false;
}

bool cPoller::Poll(int TimeoutMs)
{
  if (numFileHandles) {
     if (poll(pfd, numFileHandles, TimeoutMs) != 0)
        return true; // returns true even in case of an error, to let the caller
                     // access the file and thus see the error code
     }
  return false;
}

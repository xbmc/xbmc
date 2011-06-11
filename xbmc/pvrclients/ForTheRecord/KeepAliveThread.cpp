/*
 *      Copyright (C) 2010 Marcel Groothuis
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

#if defined TSREADER

#include "client.h" //for XBMC->Log
#include "utils.h"
#include "fortherecordrpc.h"
#include "KeepAliveThread.h"

CKeepAliveThread::CKeepAliveThread()
{
}

CKeepAliveThread::~CKeepAliveThread()
{
  XBMC->Log(LOG_DEBUG, "CKeepAliveThread:: destructor");
}

void CKeepAliveThread::ThreadProc()
{
  bool retval;

  XBMC->Log(LOG_DEBUG, "CKeepAliveThread:: thread started:%d", GetCurrentThreadId());
  while (!ThreadIsStopping(0))
  {
    retval = ForTheRecord::KeepLiveStreamAlive();
    XBMC->Log(LOG_DEBUG, "CKeepAliveThread:: KeepLiveStreamAlive returned %i", (int) retval);
    usleep(20000000); //15 sec
  }
  XBMC->Log(LOG_DEBUG, "CKeepAliveThread:: thread stopped:%d", GetCurrentThreadId());
  return;
}
#endif
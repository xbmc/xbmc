/*
* XBMC Media Center
* Copyright (c) 2002 Frodo
* Portions Copyright (c) by the authors of ffmpeg and xvid
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "Event.h"
#include "utils/TimeUtils.h"
#include "PlatformDefs.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CEvent::Interrupt() 
{ 
  CSingleLock lock(mutex);
  interrupted = true;
  condVar.notifyAll(); 
}

bool CEvent::Wait()
{
  CSingleLock lock(mutex);
  interrupted = false;
  Guard g(interruptible ? this : NULL);

  while (!setState && !interrupted)
    condVar.wait(mutex);

  bool ret = setState;
  if (!manualReset)
    setState = false;

  return ret;
}

bool CEvent::WaitMSec(unsigned int milliSeconds)
{
  CSingleLock lock(mutex);
  interrupted = false;
  Guard g(interruptible ? this : NULL);

  unsigned int startTime = CTimeUtils::GetTimeMS();
  unsigned int remainingTime = milliSeconds;
  while(!setState && !interrupted)
  {
    XbmcThreads::ConditionVariable::TimedWaitResponse resp = condVar.wait(mutex,remainingTime);

    if (setState)
      return true;

    if (resp == XbmcThreads::ConditionVariable::TW_TIMEDOUT)
      return false;

    unsigned int elapsedTimeMillis = CTimeUtils::GetTimeMS() - startTime;
    if (elapsedTimeMillis > milliSeconds)
      return false;

    remainingTime = milliSeconds - elapsedTimeMillis;
  }

  bool ret = setState;
  if (!manualReset)
    setState = false;

  return ret;
}


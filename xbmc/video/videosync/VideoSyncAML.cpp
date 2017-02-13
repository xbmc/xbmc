/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#if defined(HAS_LIBAMCODEC)

#include "video/videosync/VideoSyncAML.h"
#include "guilib/GraphicContext.h"
#include "windowing/WindowingFactory.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "threads/Thread.h"
#include <sys/poll.h>

#include <chrono>
#include <thread>

extern CEvent g_aml_sync_event;

CVideoSyncAML::CVideoSyncAML(CVideoReferenceClock *clock)
: CVideoSync(clock)
, m_abort(false)
{
}

CVideoSyncAML::~CVideoSyncAML()
{
}

bool CVideoSyncAML::Setup(PUPDATECLOCK func)
{
  UpdateClock = func;

  m_abort = false;

  g_Windowing.Register(this);
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: setting up AML");

  return true;
}

void CVideoSyncAML::Run(std::atomic<bool>& stop)
{
  // We use the wall clock for timout handling (no AML h/w, startup)
  std::chrono::time_point<std::chrono::system_clock> now(std::chrono::system_clock::now());
  unsigned int waittime (3000 / m_fps);
  uint64_t numVBlanks (0);

  while (!stop && !m_abort)
  {
    int countVSyncs(1);
    if( !g_aml_sync_event.WaitMSec(waittime))
    {
      std::chrono::milliseconds elapsed(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - now).count());
      uint64_t curVBlanks = (m_fps * elapsed.count()) / 1000;
      int64_t lastVBlankTime((curVBlanks * 1000) / m_fps);
      if (elapsed.count() > lastVBlankTime)
      {
        lastVBlankTime = (++curVBlanks * 1000) / m_fps;
        std::this_thread::sleep_for(std::chrono::milliseconds(lastVBlankTime - elapsed.count()));
      }
      countVSyncs = curVBlanks - numVBlanks;
      numVBlanks = curVBlanks;
    }
    else
      ++numVBlanks;

    uint64_t now = CurrentHostCounter();

    UpdateClock(countVSyncs, now, m_refClock);
  }
}

void CVideoSyncAML::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: cleaning up AML");
  g_Windowing.Unregister(this);
}

float CVideoSyncAML::GetFps()
{
  m_fps = g_graphicsContext.GetFPS();
  CLog::Log(LOGDEBUG, "CVideoReferenceClock: fps: %.3f", m_fps);
  return m_fps;
}

void CVideoSyncAML::OnResetDisplay()
{
  m_abort = true;
}

#endif

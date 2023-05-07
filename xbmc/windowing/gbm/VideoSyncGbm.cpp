/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncGbm.h"

#include "ServiceBroker.h"
#include "cores/VideoPlayer/VideoReferenceClock.h"
#include "threads/Thread.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#include "windowing/gbm/WinSystemGbm.h"
#include "xf86drm.h"
#include "xf86drmMode.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <unistd.h>

CVideoSyncGbm::CVideoSyncGbm(CVideoReferenceClock* clock)
  : CVideoSync(clock), m_winSystem(CServiceBroker::GetWinSystem())
{
  if (!m_winSystem)
    throw std::runtime_error("window system not available");
}

bool CVideoSyncGbm::Setup()
{
  m_abort = false;
  m_winSystem->Register(this);
  CLog::Log(LOGDEBUG, "CVideoSyncGbm::{} setting up", __FUNCTION__);

  auto winSystemGbm = dynamic_cast<KODI::WINDOWING::GBM::CWinSystemGbm*>(m_winSystem);
  if (!winSystemGbm)
  {
    CLog::Log(LOGWARNING, "CVideoSyncGbm::{}: failed to get winSystem", __FUNCTION__);
    return false;
  }

  auto drm = winSystemGbm->GetDrm();
  if (!drm)
  {
    CLog::Log(LOGWARNING, "CVideoSyncGbm::{}: failed to get drm", __FUNCTION__);
    return false;
  }

  auto crtc = drm->GetCrtc();
  if (!crtc)
  {
    CLog::Log(LOGWARNING, "CVideoSyncGbm::{}: failed to get crtc", __FUNCTION__);
    return false;
  }

  uint64_t ns = 0;
  m_crtcId = crtc->GetCrtcId();
  m_fd = drm->GetFileDescriptor();
  int s = drmCrtcGetSequence(m_fd, m_crtcId, &m_sequence, &ns);
  m_offset = CurrentHostCounter() - ns;
  if (s != 0)
  {
    CLog::Log(LOGWARNING, "CVideoSyncGbm::{}: drmCrtcGetSequence failed ({})", __FUNCTION__, s);
    return false;
  }

  CLog::Log(LOGINFO, "CVideoSyncGbm::{}: opened (fd:{} crtc:{} seq:{} ns:{}:{})", __FUNCTION__,
            m_fd, m_crtcId, m_sequence, ns, m_offset + ns);
  return true;
}

void CVideoSyncGbm::Run(CEvent& stopEvent)
{
  /* This shouldn't be very busy and timing is important so increase priority */
  CThread::GetCurrentThread()->SetPriority(ThreadPriority::ABOVE_NORMAL);

  if (m_fd < 0)
  {
    CLog::Log(LOGWARNING, "CVideoSyncGbm::{}: failed to open device ({})", __FUNCTION__, m_fd);
    return;
  }
  CLog::Log(LOGDEBUG, "CVideoSyncGbm::{}: started {}", __FUNCTION__, m_fd);

  while (!stopEvent.Signaled() && !m_abort)
  {
    uint64_t sequence = 0, ns = 0;
    usleep(1000);
    int s = drmCrtcGetSequence(m_fd, m_crtcId, &sequence, &ns);
    if (s != 0)
    {
      CLog::Log(LOGWARNING, "CVideoSyncGbm::{}: drmCrtcGetSequence failed ({})", __FUNCTION__, s);
      break;
    }

    if (sequence == m_sequence)
      continue;

    m_refClock->UpdateClock(sequence - m_sequence, m_offset + ns);
    m_sequence = sequence;
  }
}

void CVideoSyncGbm::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncGbm::{}: cleaning up", __FUNCTION__);
  m_winSystem->Unregister(this);
}

float CVideoSyncGbm::GetFps()
{
  m_fps = m_winSystem->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncGbm::{}: fps:{}", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncGbm::OnResetDisplay()
{
  m_abort = true;
}

void CVideoSyncGbm::RefreshChanged()
{
  if (m_fps != m_winSystem->GetGfxContext().GetFPS())
    m_abort = true;
}

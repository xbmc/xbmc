/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "VideoSyncGbm.h"

#include "ServiceBroker.h"
#include "threads/Thread.h"
#include "utils/TimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WinSystem.h"
#include "xbmc/windowing/gbm/WinSystemGbm.h"
#include "xf86drm.h"
#include "xf86drmMode.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

bool CVideoSyncGbm::Setup(PUPDATECLOCK func)
{
  UpdateClock = func;
  m_abort = false;
  CServiceBroker::GetWinSystem()->Register(this);
  CLog::Log(LOGDEBUG, "CVideoSyncGbm::{} setting up", __FUNCTION__);

  auto winSystem =
      dynamic_cast<KODI::WINDOWING::GBM::CWinSystemGbm*>(CServiceBroker::GetWinSystem());
  if (!winSystem)
  {
    CLog::Log(LOGWARNING, "CVideoSyncGbm::{}: failed to get winSystem", __FUNCTION__);
    return false;
  }

  auto drm = winSystem->GetDrm();
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
  CThread::GetCurrentThread()->SetPriority(CThread::GetCurrentThread()->GetPriority() + 1);

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

    UpdateClock(sequence - m_sequence, m_offset + ns, m_refClock);
    m_sequence = sequence;
  }
}

void CVideoSyncGbm::Cleanup()
{
  CLog::Log(LOGDEBUG, "CVideoSyncGbm::{}: cleaning up", __FUNCTION__);
  CServiceBroker::GetWinSystem()->Unregister(this);
}

float CVideoSyncGbm::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  CLog::Log(LOGDEBUG, "CVideoSyncGbm::{}: fps:{}", __FUNCTION__, m_fps);
  return m_fps;
}

void CVideoSyncGbm::OnResetDisplay()
{
  m_abort = true;
}

void CVideoSyncGbm::RefreshChanged()
{
  if (m_fps != CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS())
    m_abort = true;
}

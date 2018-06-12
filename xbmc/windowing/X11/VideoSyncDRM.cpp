/*
 *      Copyright (C) 2005-2014 Team XBMC
 *      http://kodi.tv
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

#include "VideoSyncDRM.h"
#include "ServiceBroker.h"
#include "xf86drm.h"
#include <sys/poll.h>
#include <sys/time.h>
#include "utils/TimeUtils.h"
#include "utils/MathUtils.h"
#include "windowing/GraphicContext.h"
#include "utils/log.h"
#include "windowing/X11/WinSystemX11GLContext.h"

static drmVBlankSeqType CrtcSel(void)
{
  CWinSystemX11* winSystem = dynamic_cast<CWinSystemX11*>(CServiceBroker::GetWinSystem());
  int crtc = winSystem->GetCrtc();
  int ret = 0;

  if (crtc == 1)
  {
    ret = DRM_VBLANK_SECONDARY;
  }
  else if (crtc > 1)
  {
    ret = (crtc << DRM_VBLANK_HIGH_CRTC_SHIFT) & DRM_VBLANK_HIGH_CRTC_MASK;
  }
  return (drmVBlankSeqType)ret;
}

bool CVideoSyncDRM::Setup(PUPDATECLOCK func)
{
  CLog::Log(LOGDEBUG, "CVideoSyncDRM::%s - setting up DRM", __FUNCTION__);

  UpdateClock = func;

  m_fd = open("/dev/dri/card0", O_RDWR, 0);
  if (m_fd < 0)
  {
    CLog::Log(LOGERROR, "CVideoSyncDRM::%s - can't open /dev/dri/card0", __FUNCTION__);
    return false;
  }

  drmVBlank vbl;
  int ret;
  vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | CrtcSel());
  vbl.request.sequence = 0;
  ret = drmWaitVBlank(m_fd, &vbl);
  if (ret != 0)
  {
    CLog::Log(LOGERROR, "CVideoSyncDRM::%s - drmWaitVBlank returned error", __FUNCTION__);
    return false;
  }

  m_abort = false;
  m_winSystem.Register(this);

  return true;
}

void CVideoSyncDRM::Run(CEvent& stopEvent)
{
  drmVBlank vbl;
  VblInfo info;
  int ret;
  drmVBlankSeqType crtcSel = CrtcSel();

  vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | crtcSel);
  vbl.request.sequence = 0;
  ret = drmWaitVBlank(m_fd, &vbl);
  if (ret != 0)
  {
    CLog::Log(LOGERROR, "CVideoSyncDRM::%s - drmWaitVBlank returned error", __FUNCTION__);
    return;
  }

  info.start = CurrentHostCounter();
  info.videoSync = this;

  vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT | crtcSel);
  vbl.request.sequence = 1;
  vbl.request.signal = (unsigned long)&info;
  ret = drmWaitVBlank(m_fd, &vbl);
  if (ret != 0)
  {
    CLog::Log(LOGERROR, "CVideoSyncDRM::%s - drmWaitVBlank returned error", __FUNCTION__);
    return;
  }

  drmEventContext evctx;
  memset(&evctx, 0, sizeof evctx);
  evctx.version = DRM_EVENT_CONTEXT_VERSION;
  evctx.vblank_handler = EventHandler;
  evctx.page_flip_handler = NULL;

  timeval timeout;
  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(m_fd, &fds);

  while (!stopEvent.Signaled() && !m_abort)
  {
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    ret = select(m_fd + 1, &fds, NULL, NULL, &timeout);

    if (ret <= 0)
    {
      continue;
    }

    ret = drmHandleEvent(m_fd, &evctx);
    if (ret != 0)
    {
      CLog::Log(LOGERROR, "CVideoSyncDRM::%s - drmHandleEvent returned error", __FUNCTION__);
      break;
    }
  }
}

void CVideoSyncDRM::Cleanup()
{
  close(m_fd);
  m_winSystem.Unregister(this);
}

void CVideoSyncDRM::EventHandler(int fd, unsigned int frame, unsigned int sec,
                                 unsigned int usec, void *data)
{
  drmVBlank vbl;
  VblInfo *info = static_cast<VblInfo*>(data);
  drmVBlankSeqType crtcSel = CrtcSel();

  vbl.request.type = (drmVBlankSeqType)(DRM_VBLANK_RELATIVE | DRM_VBLANK_EVENT | crtcSel);
  vbl.request.sequence = 1;
  vbl.request.signal = (unsigned long)data;

  drmWaitVBlank(info->videoSync->m_fd, &vbl);

  uint64_t now = CurrentHostCounter();
  float diff = (float)(now - info->start)/CurrentHostFrequency();
  int vblanks = MathUtils::round_int(diff * info->videoSync->m_fps);
  info->start = now;

  info->videoSync->UpdateClock(vblanks, now, info->videoSync->m_refClock);
}

void CVideoSyncDRM::OnResetDisplay()
{
  m_abort = true;
}

float CVideoSyncDRM::GetFps()
{
  m_fps = CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS();
  return m_fps;
}

void CVideoSyncDRM::RefreshChanged()
{
  if (m_fps != CServiceBroker::GetWinSystem()->GetGfxContext().GetFPS())
    m_abort = true;
}

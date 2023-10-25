/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMLegacy.h"

#include "guilib/gui3d.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <drm_mode.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

using namespace KODI::WINDOWING::GBM;

static int flip_happening = 0;

bool CDRMLegacy::SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo *bo)
{
  struct drm_fb *drm_fb = DrmFbGetFromBo(bo);

  auto ret = drmModeSetCrtc(m_fd, m_crtc->GetCrtcId(), drm_fb->fb_id, 0, 0,
                            m_connector->GetConnectorId(), 1, m_mode);

  if(ret < 0)
  {
    CLog::Log(LOGERROR, "CDRMLegacy::{} - failed to set crtc mode: {}x{}{} @ {} Hz", __FUNCTION__,
              m_mode->hdisplay, m_mode->vdisplay,
              m_mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "", m_mode->vrefresh);

    return false;
  }

  CLog::Log(LOGDEBUG, "CDRMLegacy::{} - set crtc mode: {}x{}{} @ {} Hz", __FUNCTION__,
            m_mode->hdisplay, m_mode->vdisplay, m_mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "",
            m_mode->vrefresh);

  return true;
}

void CDRMLegacy::PageFlipHandler(int fd, unsigned int frame, unsigned int sec,
                                unsigned int usec, void *data)
{
  (void) fd, (void) frame, (void) sec, (void) usec;

  int *flip_happening = static_cast<int *>(data);
  *flip_happening = 0;
}

bool CDRMLegacy::WaitingForFlip()
{
  if (!flip_happening)
    return false;

  struct pollfd drm_fds =
  {
    m_fd,
    POLLIN,
    0,
  };

  drmEventContext drm_evctx{};
  drm_evctx.version = DRM_EVENT_CONTEXT_VERSION;
  drm_evctx.page_flip_handler = PageFlipHandler;

  while(flip_happening)
  {
    auto ret = poll(&drm_fds, 1, -1);

    if(ret < 0)
      return true;

    if(drm_fds.revents & (POLLHUP | POLLERR))
      return true;

    if(drm_fds.revents & POLLIN)
      drmHandleEvent(m_fd, &drm_evctx);
  }

  return false;
}

bool CDRMLegacy::QueueFlip(struct gbm_bo *bo)
{
  struct drm_fb *drm_fb = DrmFbGetFromBo(bo);

  auto ret = drmModePageFlip(m_fd, m_crtc->GetCrtcId(), drm_fb->fb_id, DRM_MODE_PAGE_FLIP_EVENT,
                             &flip_happening);

  if(ret)
  {
    CLog::Log(LOGDEBUG, "CDRMLegacy::{} - failed to queue DRM page flip", __FUNCTION__);
    return false;
  }

  return true;
}

void CDRMLegacy::FlipPage(struct gbm_bo* bo, bool rendered, bool videoLayer, bool async)
{
  if (rendered || videoLayer)
  {
    flip_happening = QueueFlip(bo);
    WaitingForFlip();
  }
}

bool CDRMLegacy::InitDrm()
{
  if (!CDRMUtils::OpenDrm(true))
    return false;

  if (!CDRMUtils::InitDrm())
    return false;

  CLog::Log(LOGDEBUG, "CDRMLegacy::{} - initialized legacy DRM", __FUNCTION__);
  return true;
}

bool CDRMLegacy::SetActive(bool active)
{
  if (!m_connector->SetProperty("DPMS", active ? DRM_MODE_DPMS_ON : DRM_MODE_DPMS_OFF))
  {
    CLog::Log(LOGDEBUG, "CDRMLegacy::{} - failed to set DPMS property", __FUNCTION__);
    return false;
  }

  return true;
}

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drm_mode.h>
#include <EGL/egl.h>
#include <unistd.h>

#include "WinSystemGbmGLESContext.h"
#include "guilib/gui3d.h"
#include "utils/log.h"
#include "settings/Settings.h"

#include "DRMLegacy.h"

static struct drm *m_drm = nullptr;
static struct gbm *m_gbm = nullptr;

static int flip_happening = 0;

static struct pollfd m_drm_fds;
static drmEventContext m_drm_evctx;

bool CDRMLegacy::SetVideoMode(RESOLUTION_INFO res)
{
  m_gbm->next_bo = gbm_surface_lock_front_buffer(m_gbm->surface);
  struct drm_fb *drm_fb = CDRMUtils::DrmFbGetFromBo(m_gbm->next_bo);

  auto ret = drmModeSetCrtc(m_drm->fd,
                            m_drm->crtc->crtc->crtc_id,
                            drm_fb->fb_id,
                            0,
                            0,
                            &m_drm->connector->connector->connector_id,
                            1,
                            m_drm->mode);

  if(ret < 0)
  {
    CLog::Log(LOGERROR,
              "CDRMUtils::%s - failed to set crtc mode: %dx%d%s @ %d Hz",
              __FUNCTION__,
              m_drm->mode->hdisplay,
              m_drm->mode->vdisplay,
              m_drm->mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "",
              m_drm->mode->vrefresh);

    return false;
  }

  CLog::Log(LOGDEBUG, "CDRMUtils::%s - set crtc mode: %dx%d%s @ %d Hz",
            __FUNCTION__,
            m_drm->mode->hdisplay,
            m_drm->mode->vdisplay,
            m_drm->mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "",
            m_drm->mode->vrefresh);

  gbm_surface_release_buffer(m_gbm->surface, m_gbm->bo);
  m_gbm->bo = m_gbm->next_bo;
  m_gbm->next_bo = nullptr;

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
  if(!flip_happening)
  {
    return false;
  }

  m_drm_fds.fd = m_drm->fd;
  m_drm_fds.events = POLLIN;

  m_drm_evctx.version = DRM_EVENT_CONTEXT_VERSION;
  m_drm_evctx.page_flip_handler = PageFlipHandler;

  m_drm_fds.revents = 0;

  while(flip_happening)
  {
    auto ret = poll(&m_drm_fds, 1, -1);

    if(ret < 0)
    {
      return true;
    }

    if(m_drm_fds.revents & (POLLHUP | POLLERR))
    {
      return true;
    }

    if(m_drm_fds.revents & POLLIN)
    {
      drmHandleEvent(m_drm->fd, &m_drm_evctx);
    }
  }

  gbm_surface_release_buffer(m_gbm->surface, m_gbm->bo);
  m_gbm->bo = m_gbm->next_bo;
  m_gbm->next_bo = nullptr;

  return false;
}

bool CDRMLegacy::QueueFlip()
{
  m_gbm->next_bo = gbm_surface_lock_front_buffer(m_gbm->surface);
  struct drm_fb *drm_fb = CDRMUtils::DrmFbGetFromBo(m_gbm->next_bo);

  auto ret = drmModePageFlip(m_drm->fd,
                             m_drm->crtc->crtc->crtc_id,
                             drm_fb->fb_id,
                             DRM_MODE_PAGE_FLIP_EVENT,
                             &flip_happening);

  if(ret)
  {
    CLog::Log(LOGDEBUG, "CDRMLegacy::%s - failed to queue DRM page flip", __FUNCTION__);
    return false;
  }

  return true;
}

void CDRMLegacy::FlipPage()
{
  flip_happening = QueueFlip();
  WaitingForFlip();
}

bool CDRMLegacy::InitDrmLegacy(drm *drm, gbm *gbm)
{
  m_drm = drm;
  m_gbm = gbm;

  if (!CDRMUtils::InitDrm(m_drm))
  {
    return false;
  }

  m_gbm->dev = gbm_create_device(m_drm->fd);

  if (!CGBMUtils::InitGbm(m_gbm, m_drm->mode->hdisplay, m_drm->mode->vdisplay))
  {
    return false;
  }

  return true;
}

void CDRMLegacy::DestroyDrmLegacy()
{
  CDRMUtils::DestroyDrm();

  if(m_gbm->surface)
  {
    gbm_surface_release_buffer(m_gbm->surface, m_gbm->bo);
    m_gbm->bo = m_gbm->next_bo = nullptr;

    gbm_surface_destroy(m_gbm->surface);
    m_gbm->surface = nullptr;
  }

  if(m_gbm->dev)
  {
    gbm_device_destroy(m_gbm->dev);
    m_gbm->dev = nullptr;
  }
}

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <errno.h>
#include <drm_mode.h>
#include <string.h>
#include <unistd.h>

#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "utils/log.h"

#include "DRMAtomic.h"
#include "WinSystemGbmGLESContext.h"

#include <drm_fourcc.h>

void CDRMAtomic::DrmAtomicCommit(int fb_id, int flags, bool rendered, bool videoLayer)
{
  uint32_t blob_id;

  if (flags & DRM_MODE_ATOMIC_ALLOW_MODESET)
  {
    if (!AddProperty(m_connector, "CRTC_ID", m_crtc->crtc->crtc_id))
    {
      return;
    }

    if (drmModeCreatePropertyBlob(m_fd, m_mode, sizeof(*m_mode), &blob_id) != 0)
    {
      return;
    }

    if (!AddProperty(m_crtc, "MODE_ID", blob_id))
    {
      return;
    }

    if (!AddProperty(m_crtc, "ACTIVE", m_active ? 1 : 0))
    {
      return;
    }
  }

  if (rendered)
  {
    AddProperty(m_overlay_plane, "FB_ID", fb_id);
    AddProperty(m_overlay_plane, "CRTC_ID", m_crtc->crtc->crtc_id);
    AddProperty(m_overlay_plane, "SRC_X", 0);
    AddProperty(m_overlay_plane, "SRC_Y", 0);
    AddProperty(m_overlay_plane, "SRC_W", m_width << 16);
    AddProperty(m_overlay_plane, "SRC_H", m_height << 16);
    AddProperty(m_overlay_plane, "CRTC_X", 0);
    AddProperty(m_overlay_plane, "CRTC_Y", 0);
    AddProperty(m_overlay_plane, "CRTC_W", m_mode->hdisplay);
    AddProperty(m_overlay_plane, "CRTC_H", m_mode->vdisplay);
  }
  else if (videoLayer && !CServiceBroker::GetGUI()->GetWindowManager().HasVisibleControls())
  {
    // disable gui plane when video layer is active and gui has no visible controls
    AddProperty(m_overlay_plane, "FB_ID", 0);
    AddProperty(m_overlay_plane, "CRTC_ID", 0);
  }

  auto ret = drmModeAtomicCommit(m_fd, m_req, flags | DRM_MODE_ATOMIC_TEST_ONLY, nullptr);
  if (ret < 0)
  {
    CLog::Log(LOGERROR, "CDRMAtomic::%s - test commit failed: %s", __FUNCTION__, strerror(errno));
  }
  else if (ret == 0)
  {
    ret = drmModeAtomicCommit(m_fd, m_req, flags, nullptr);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "CDRMAtomic::%s - atomic commit failed: %s", __FUNCTION__, strerror(errno));
    }
  }

  if (flags & DRM_MODE_ATOMIC_ALLOW_MODESET)
  {
    if (drmModeDestroyPropertyBlob(m_fd, blob_id) != 0)
      CLog::Log(LOGERROR, "CDRMAtomic::%s - failed to destroy property blob: %s", __FUNCTION__, strerror(errno));
  }

  drmModeAtomicFree(m_req);
  m_req = drmModeAtomicAlloc();
}

void CDRMAtomic::FlipPage(struct gbm_bo *bo, bool rendered, bool videoLayer)
{
  uint32_t flags = 0;

  if(m_need_modeset)
  {
    flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
    m_need_modeset = false;
  }

  struct drm_fb *drm_fb = nullptr;

  if (rendered)
  {
    if (videoLayer)
      m_overlay_plane->format = DRM_FORMAT_ARGB8888;
    else
      m_overlay_plane->format = DRM_FORMAT_XRGB8888;

    drm_fb = CDRMUtils::DrmFbGetFromBo(bo);
    if (!drm_fb)
    {
      CLog::Log(LOGERROR, "CDRMAtomic::%s - Failed to get a new FBO", __FUNCTION__);
      return;
    }
  }

  DrmAtomicCommit(!drm_fb ? 0 : drm_fb->fb_id, flags, rendered, videoLayer);
}

bool CDRMAtomic::InitDrm()
{
  if (!CDRMUtils::OpenDrm())
  {
    return false;
  }

  auto ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_ATOMIC, 1);
  if (ret)
  {
    CLog::Log(LOGERROR, "CDRMAtomic::%s - no atomic modesetting support: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  m_req = drmModeAtomicAlloc();

  if (!CDRMUtils::InitDrm())
  {
    return false;
  }

  CLog::Log(LOGDEBUG, "CDRMAtomic::%s - initialized atomic DRM", __FUNCTION__);
  return true;
}

void CDRMAtomic::DestroyDrm()
{
  CDRMUtils::DestroyDrm();

  drmModeAtomicFree(m_req);
  m_req = nullptr;
}

bool CDRMAtomic::SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo *bo)
{
  m_need_modeset = true;

  return true;
}

bool CDRMAtomic::SetActive(bool active)
{
  m_need_modeset = true;
  m_active = active;

  return true;
}

bool CDRMAtomic::AddProperty(struct drm_object *object, const char *name, uint64_t value)
{
  uint32_t property_id = this->GetPropertyId(object, name);
  if (!property_id)
    return false;

  if (drmModeAtomicAddProperty(m_req, object->id, property_id, value) < 0)
  {
    CLog::Log(LOGERROR, "CDRMAtomic::%s - could not add property %s", __FUNCTION__, name);
    return false;
  }

  return true;
}

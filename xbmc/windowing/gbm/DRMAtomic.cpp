/*
 *      Copyright (C) 2005-present Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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

void CDRMAtomic::DrmAtomicCommit(int fb_id, int flags, bool rendered, bool videoLayer)
{
  uint32_t blob_id;
  struct plane *plane;

  if (flags & DRM_MODE_ATOMIC_ALLOW_MODESET)
  {
    if (!AddProperty(m_req, m_connector, "CRTC_ID", m_crtc->crtc->crtc_id))
    {
      return;
    }

    if (drmModeCreatePropertyBlob(m_fd, m_mode, sizeof(*m_mode), &blob_id) != 0)
    {
      return;
    }

    if (!AddProperty(m_req, m_crtc, "MODE_ID", blob_id))
    {
      return;
    }

    if (!AddProperty(m_req, m_crtc, "ACTIVE", m_active ? 1 : 0))
    {
      return;
    }

    if (!videoLayer)
    {
      // disable overlay plane on modeset
      AddProperty(m_req, m_overlay_plane, "FB_ID", 0);
      AddProperty(m_req, m_overlay_plane, "CRTC_ID", 0);
    }
  }

  if (videoLayer)
    plane = m_overlay_plane;
  else
    plane = m_primary_plane;

  if (rendered)
  {
    AddProperty(m_req, plane, "FB_ID", fb_id);
    AddProperty(m_req, plane, "CRTC_ID", m_crtc->crtc->crtc_id);
    AddProperty(m_req, plane, "SRC_X", 0);
    AddProperty(m_req, plane, "SRC_Y", 0);
    AddProperty(m_req, plane, "SRC_W", m_mode->hdisplay << 16);
    AddProperty(m_req, plane, "SRC_H", m_mode->vdisplay << 16);
    AddProperty(m_req, plane, "CRTC_X", 0);
    AddProperty(m_req, plane, "CRTC_Y", 0);
    AddProperty(m_req, plane, "CRTC_W", m_mode->hdisplay);
    AddProperty(m_req, plane, "CRTC_H", m_mode->vdisplay);
  }
  else if (videoLayer && !CServiceBroker::GetGUI()->GetWindowManager().HasVisibleControls())
  {
    // disable gui plane when video layer is active and gui has no visible controls
    AddProperty(m_req, plane, "FB_ID", 0);
    AddProperty(m_req, plane, "CRTC_ID", 0);
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

bool CDRMAtomic::SetVideoMode(RESOLUTION_INFO res, struct gbm_bo *bo)
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

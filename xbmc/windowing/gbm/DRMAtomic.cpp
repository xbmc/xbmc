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

bool CDRMAtomic::AddConnectorProperty(drmModeAtomicReq *req, int obj_id, const char *name, int value)
{
  struct connector *obj = m_connector;
  int prop_id = 0;

  for (unsigned int i = 0 ; i < obj->props->count_props ; i++)
  {
    if (strcmp(obj->props_info[i]->name, name) == 0)
    {
      prop_id = obj->props_info[i]->prop_id;
      break;
    }
  }

  if (prop_id < 0)
  {
    CLog::Log(LOGERROR, "CDRMAtomic::%s - no connector property: %s", __FUNCTION__, name);
    return false;
  }

  auto ret = drmModeAtomicAddProperty(req, obj_id, prop_id, value);
  if (ret < 0)
  {
    return false;
  }

  return true;
}

bool CDRMAtomic::AddCrtcProperty(drmModeAtomicReq *req, int obj_id, const char *name, int value)
{
  struct crtc *obj = m_crtc;
  int prop_id = -1;

  for (unsigned int i = 0 ; i < obj->props->count_props ; i++)
  {
    if (strcmp(obj->props_info[i]->name, name) == 0)
    {
      prop_id = obj->props_info[i]->prop_id;
      break;
    }
  }

  if (prop_id < 0)
  {
    CLog::Log(LOGERROR, "CDRMAtomic::%s - no crtc property: %s", __FUNCTION__, name);
    return false;
  }

  auto ret = drmModeAtomicAddProperty(req, obj_id, prop_id, value);
  if (ret < 0)
  {
    return false;
  }

  return true;
}

bool CDRMAtomic::AddPlaneProperty(drmModeAtomicReq *req, struct plane *obj, const char *name, int value)
{
  int prop_id = -1;

  for (unsigned int i = 0 ; i < obj->props->count_props ; i++)
  {
    if (strcmp(obj->props_info[i]->name, name) == 0)
    {
      prop_id = obj->props_info[i]->prop_id;
      break;
    }
  }

  if (prop_id < 0)
  {
    CLog::Log(LOGERROR, "CDRMAtomic::%s - no plane property: %s", __FUNCTION__, name);
    return false;
  }

  auto ret = drmModeAtomicAddProperty(req, obj->plane->plane_id, prop_id, value);
  if (ret < 0)
  {
    return false;
  }

  return true;
}

void CDRMAtomic::DrmAtomicCommit(int fb_id, int flags, bool rendered, bool videoLayer)
{
  uint32_t blob_id;
  struct plane *plane;

  if (flags & DRM_MODE_ATOMIC_ALLOW_MODESET)
  {
    if (!AddConnectorProperty(m_req, m_connector->connector->connector_id, "CRTC_ID", m_crtc->crtc->crtc_id))
    {
      return;
    }

    if (drmModeCreatePropertyBlob(m_fd, m_mode, sizeof(*m_mode), &blob_id) != 0)
    {
      return;
    }

    if (!AddCrtcProperty(m_req, m_crtc->crtc->crtc_id, "MODE_ID", blob_id))
    {
      return;
    }

    if (!AddCrtcProperty(m_req, m_crtc->crtc->crtc_id, "ACTIVE", m_active ? 1 : 0))
    {
      return;
    }

    if (!videoLayer)
    {
      // disable overlay plane on modeset
      AddPlaneProperty(m_req, m_overlay_plane, "FB_ID", 0);
      AddPlaneProperty(m_req, m_overlay_plane, "CRTC_ID", 0);
    }
  }

  if (videoLayer)
    plane = m_overlay_plane;
  else
    plane = m_primary_plane;

  if (rendered)
  {
    AddPlaneProperty(m_req, plane, "FB_ID", fb_id);
    AddPlaneProperty(m_req, plane, "CRTC_ID", m_crtc->crtc->crtc_id);
    AddPlaneProperty(m_req, plane, "SRC_X", 0);
    AddPlaneProperty(m_req, plane, "SRC_Y", 0);
    AddPlaneProperty(m_req, plane, "SRC_W", m_mode->hdisplay << 16);
    AddPlaneProperty(m_req, plane, "SRC_H", m_mode->vdisplay << 16);
    AddPlaneProperty(m_req, plane, "CRTC_X", 0);
    AddPlaneProperty(m_req, plane, "CRTC_Y", 0);
    AddPlaneProperty(m_req, plane, "CRTC_W", m_mode->hdisplay);
    AddPlaneProperty(m_req, plane, "CRTC_H", m_mode->vdisplay);
  }
  else if (videoLayer && !CServiceBroker::GetGUI()->GetWindowManager().HasVisibleControls())
  {
    // disable gui plane when video layer is active and gui has no visible controls
    AddPlaneProperty(m_req, plane, "FB_ID", 0);
    AddPlaneProperty(m_req, plane, "CRTC_ID", 0);
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

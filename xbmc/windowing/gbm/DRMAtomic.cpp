/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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

#include <drm_fourcc.h>

using namespace KODI::WINDOWING::GBM;

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
  struct drm_fb *drm_fb = nullptr;

  if (rendered)
  {
    if (videoLayer)
      m_overlay_plane->format = CDRMUtils::FourCCWithAlpha(m_overlay_plane->GetFormat());
    else
      m_overlay_plane->format = CDRMUtils::FourCCWithoutAlpha(m_overlay_plane->GetFormat());

    drm_fb = CDRMUtils::DrmFbGetFromBo(bo);
    if (!drm_fb)
    {
      CLog::Log(LOGERROR, "CDRMAtomic::%s - Failed to get a new FBO", __FUNCTION__);
      return;
    }
  }

  uint32_t flags = 0;

  if (m_need_modeset)
  {
    flags |= DRM_MODE_ATOMIC_ALLOW_MODESET;
    m_need_modeset = false;
  }

  DrmAtomicCommit(!drm_fb ? 0 : drm_fb->fb_id, flags, rendered, videoLayer);
}

bool CDRMAtomic::InitDrm()
{
  if (!CDRMUtils::OpenDrm(true))
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

  if (!CDRMAtomic::ResetPlanes())
  {
    CLog::Log(LOGDEBUG, "CDRMAtomic::%s - failed to reset planes", __FUNCTION__);
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

bool CDRMAtomic::ResetPlanes()
{
  drmModePlaneResPtr plane_resources = drmModeGetPlaneResources(m_fd);
  if (!plane_resources)
  {
    CLog::Log(LOGERROR, "CDRMAtomic::%s - drmModeGetPlaneResources failed: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  for (uint32_t i = 0; i < plane_resources->count_planes; i++)
  {
    drmModePlanePtr plane = drmModeGetPlane(m_fd, plane_resources->planes[i]);
    if (!plane)
      continue;

    drm_object object;

    if (!CDRMUtils::GetProperties(m_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE, &object))
    {
      CLog::Log(LOGERROR, "CDRMAtomic::%s - could not get plane %u properties: %s", __FUNCTION__, plane->plane_id, strerror(errno));\
      drmModeFreePlane(plane);
      continue;
    }

    AddProperty(&object, "FB_ID", 0);
    AddProperty(&object, "CRTC_ID", 0);

    CDRMUtils::FreeProperties(&object);
    drmModeFreePlane(plane);
  }

  drmModeFreePlaneResources(plane_resources);

  return true;
}

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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <drm_fourcc.h>
#include <drm_mode.h>
#include <EGL/egl.h>
#include <unistd.h>

#include "WinSystemGbmGLESContext.h"
#include "guilib/gui3d.h"
#include "utils/log.h"

#include "DRMUtils.h"

static struct drm *m_drm = nullptr;

static drmModeResPtr m_drm_resources = nullptr;
static drmModeCrtcPtr m_orig_crtc = nullptr;

struct drm *CDRMUtils::GetDrm()
{
  return m_drm;
}

void CDRMUtils::WaitVBlank()
{
  drmVBlank vbl;
  vbl.request.type = DRM_VBLANK_RELATIVE;
  vbl.request.sequence = 1;
  drmWaitVBlank(m_drm->fd, &vbl);
}

bool CDRMUtils::SetMode(RESOLUTION_INFO res)
{
  m_drm->mode = &m_drm->connector->connector->modes[atoi(res.strId.c_str())];

  CLog::Log(LOGDEBUG, "CDRMUtils::%s - found crtc mode: %dx%d%s @ %d Hz",
            __FUNCTION__,
            m_drm->mode->hdisplay,
            m_drm->mode->vdisplay,
            m_drm->mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "",
            m_drm->mode->vrefresh);

  return true;
}

void CDRMUtils::DrmFbDestroyCallback(struct gbm_bo *bo, void *data)
{
  struct drm_fb *fb = static_cast<drm_fb *>(data);

  if(fb->fb_id)
  {
    drmModeRmFB(m_drm->fd, fb->fb_id);
  }

  delete (fb);
}

drm_fb * CDRMUtils::DrmFbGetFromBo(struct gbm_bo *bo)
{
  {
    struct drm_fb *fb = static_cast<drm_fb *>(gbm_bo_get_user_data(bo));
    if(fb)
    {
      return fb;
    }
  }

  struct drm_fb *fb = new drm_fb;
  fb->bo = bo;

  uint32_t width,
           height,
           handles[4] = {0},
           strides[4] = {0},
           offsets[4] = {0};

  width = gbm_bo_get_width(bo);
  height = gbm_bo_get_height(bo);

  handles[0] = gbm_bo_get_handle(bo).u32;
  strides[0] = gbm_bo_get_stride(bo);
  memset(offsets, 0, 16);

  auto ret = drmModeAddFB2(m_drm->fd,
                           width,
                           height,
                           DRM_FORMAT_ARGB8888,
                           handles,
                           strides,
                           offsets,
                           &fb->fb_id,
                           0);

  if(ret)
  {
    delete (fb);
    CLog::Log(LOGDEBUG, "CDRMUtils::%s - failed to add framebuffer", __FUNCTION__);
    return nullptr;
  }

  gbm_bo_set_user_data(bo, fb, DrmFbDestroyCallback);

  return fb;
}

bool CDRMUtils::GetResources()
{
  m_drm_resources = drmModeGetResources(m_drm->fd);
  if(!m_drm_resources)
  {
    return false;
  }

  return true;
}

bool CDRMUtils::GetConnector()
{
  for(auto i = 0; i < m_drm_resources->count_connectors; i++)
  {
    m_drm->connector->connector = drmModeGetConnector(m_drm->fd,
                                                      m_drm_resources->connectors[i]);
    if(m_drm->connector->connector->connection == DRM_MODE_CONNECTED)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - found connector: %d", __FUNCTION__,
                                                                 m_drm->connector->connector->connector_id);
      break;
    }
    drmModeFreeConnector(m_drm->connector->connector);
    m_drm->connector->connector = nullptr;
  }

  if(!m_drm->connector->connector)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get connector: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  m_drm->connector->props = drmModeObjectGetProperties(m_drm->fd, m_drm->connector->connector->connector_id, DRM_MODE_OBJECT_CONNECTOR);
  if (!m_drm->connector->props)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get connector %u properties: %s", __FUNCTION__, m_drm->connector->connector->connector_id, strerror(errno));
    return false;
  }

  m_drm->connector->props_info = new drmModePropertyPtr;
  for (uint32_t i = 0; i < m_drm->connector->props->count_props; i++)
  {
    m_drm->connector->props_info[i] = drmModeGetProperty(m_drm->fd, m_drm->connector->props->props[i]);
  }

  return true;
}

bool CDRMUtils::GetEncoder()
{
  for(auto i = 0; i < m_drm_resources->count_encoders; i++)
  {
    m_drm->encoder->encoder = drmModeGetEncoder(m_drm->fd, m_drm_resources->encoders[i]);
    if(m_drm->encoder->encoder->encoder_id == m_drm->connector->connector->encoder_id)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - found encoder: %d", __FUNCTION__,
                                                               m_drm->encoder->encoder->encoder_id);
      break;
    }
    drmModeFreeEncoder(m_drm->encoder->encoder);
    m_drm->encoder->encoder = nullptr;
  }

  if(!m_drm->encoder->encoder)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get encoder: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  return true;
}

bool CDRMUtils::GetCrtc()
{
  for(auto i = 0; i < m_drm_resources->count_crtcs; i++)
  {
    m_drm->crtc->crtc = drmModeGetCrtc(m_drm->fd, m_drm_resources->crtcs[i]);
    if(m_drm->crtc->crtc->crtc_id == m_drm->encoder->encoder->crtc_id)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - found crtc: %d", __FUNCTION__,
                                                            m_drm->crtc->crtc->crtc_id);
      m_drm->crtc_index = i;
      break;
    }
    drmModeFreeCrtc(m_drm->crtc->crtc);
    m_drm->crtc->crtc = nullptr;
  }

  if(!m_drm->crtc->crtc)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get crtc: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  m_drm->crtc->props = drmModeObjectGetProperties(m_drm->fd, m_drm->crtc->crtc->crtc_id, DRM_MODE_OBJECT_CRTC);
  if (!m_drm->crtc->props)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get crtc %u properties: %s", __FUNCTION__, m_drm->crtc->crtc->crtc_id, strerror(errno));
    return false;
  }

  m_drm->crtc->props_info = new drmModePropertyPtr;
  for (uint32_t i = 0; i < m_drm->crtc->props->count_props; i++)
  {
    m_drm->crtc->props_info[i] = drmModeGetProperty(m_drm->fd, m_drm->crtc->props->props[i]);
  }

  return true;
}

bool CDRMUtils::GetPreferredMode()
{
  for(auto i = 0, area = 0; i < m_drm->connector->connector->count_modes; i++)
  {
    drmModeModeInfo *current_mode = &m_drm->connector->connector->modes[i];

    if(current_mode->type & DRM_MODE_TYPE_PREFERRED)
    {
      m_drm->mode = current_mode;
      CLog::Log(LOGDEBUG,
                "CDRMUtils::%s - found preferred mode: %dx%d%s @ %d Hz",
                __FUNCTION__,
                m_drm->mode->hdisplay,
                m_drm->mode->vdisplay,
                m_drm->mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "",
                m_drm->mode->vrefresh);
      break;
    }

    auto current_area = current_mode->hdisplay * current_mode->vdisplay;
    if (current_area > area)
    {
      m_drm->mode = current_mode;
      area = current_area;
    }
  }

  if(!m_drm->mode)
  {
    CLog::Log(LOGDEBUG, "CDRMUtils::%s - failed to find preferred mode", __FUNCTION__);
    return false;
  }

  return true;
}

bool CDRMUtils::GetPlanes()
{
  drmModePlaneResPtr plane_resources;
  uint32_t primary_plane_id = -1;
  uint32_t overlay_plane_id = -1;

  plane_resources = drmModeGetPlaneResources(m_drm->fd);
  if (!plane_resources)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - drmModeGetPlaneResources failed: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  for (uint32_t i = 0; i < plane_resources->count_planes; i++)
  {
    uint32_t id = plane_resources->planes[i];
    drmModePlanePtr plane = drmModeGetPlane(m_drm->fd, id);
    if (!plane)
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - drmModeGetPlane(%u) failed: %s", __FUNCTION__, id, strerror(errno));
      continue;
    }

    if (plane->possible_crtcs & (1 << m_drm->crtc_index))
    {
      drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(m_drm->fd, id, DRM_MODE_OBJECT_PLANE);

      for (uint32_t j = 0; j < props->count_props; j++)
      {
        drmModePropertyPtr p = drmModeGetProperty(m_drm->fd, props->props[j]);

        if ((strcmp(p->name, "type") == 0) && (props->prop_values[j] == DRM_PLANE_TYPE_PRIMARY))
        {
          CLog::Log(LOGDEBUG, "CDRMUtils::%s - found primary plane: %d", __FUNCTION__, id);
          primary_plane_id = id;
        }
        else if ((strcmp(p->name, "type") == 0) && (props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY))
        {
          CLog::Log(LOGDEBUG, "CDRMUtils::%s - found overlay plane: %d", __FUNCTION__, id);
          overlay_plane_id = id;
        }

        drmModeFreeProperty(p);
      }

      drmModeFreeObjectProperties(props);
    }

    drmModeFreePlane(plane);
  }

  drmModeFreePlaneResources(plane_resources);

  // primary plane
  m_drm->primary_plane->plane = drmModeGetPlane(m_drm->fd, primary_plane_id);
  if (!m_drm->primary_plane->plane)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get primary plane %i: %s", __FUNCTION__, primary_plane_id, strerror(errno));
    return false;
  }

  m_drm->primary_plane->props = drmModeObjectGetProperties(m_drm->fd, primary_plane_id, DRM_MODE_OBJECT_PLANE);
  if (!m_drm->primary_plane->props)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get primary plane %u properties: %s", __FUNCTION__, primary_plane_id, strerror(errno));
    return false;
  }

  m_drm->primary_plane->props_info = new drmModePropertyPtr;
  for (uint32_t i = 0; i < m_drm->primary_plane->props->count_props; i++)
  {
    m_drm->primary_plane->props_info[i] = drmModeGetProperty(m_drm->fd, m_drm->primary_plane->props->props[i]);
  }

  // overlay plane
  m_drm->overlay_plane->plane = drmModeGetPlane(m_drm->fd, overlay_plane_id);
  if (!m_drm->overlay_plane->plane)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get overlay plane %i: %s", __FUNCTION__, overlay_plane_id, strerror(errno));
    return false;
  }

  m_drm->overlay_plane->props = drmModeObjectGetProperties(m_drm->fd, overlay_plane_id, DRM_MODE_OBJECT_PLANE);
  if (!m_drm->overlay_plane->props)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get overlay plane %u properties: %s", __FUNCTION__, overlay_plane_id, strerror(errno));
    return false;
  }

  m_drm->overlay_plane->props_info = new drmModePropertyPtr;
  for (uint32_t i = 0; i < m_drm->overlay_plane->props->count_props; i++)
  {
    m_drm->overlay_plane->props_info[i] = drmModeGetProperty(m_drm->fd, m_drm->overlay_plane->props->props[i]);
  }

  return true;
}

int CDRMUtils::Open(const char* device)
{
  int fd = -1;

  std::vector<const char*>modules =
  {
    "i915",
    "amdgpu",
    "radeon",
    "nouveau",
    "vmwgfx",
    "msm",
    "imx-drm",
    "rockchip",
    "vc4",
    "virtio_gpu",
    "sun4i-drm",
  };

  for (auto module : modules)
  {
    fd = drmOpen(module, device);
    if (fd >= 0)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - opened device: %s using module: %s", __FUNCTION__, device, module);
      break;
    }
  }

  return fd;
}

bool CDRMUtils::InitDrm(drm *drm)
{
  m_drm = drm;

  m_drm->connector = new connector;
  m_drm->encoder = new encoder;
  m_drm->crtc = new crtc;
  m_drm->primary_plane = new plane;
  m_drm->overlay_plane = new plane;

  for(int i = 0; i < 10; ++i)
  {
    if (m_drm->fd >= 0)
    {
      drmClose(m_drm->fd);
    }

    std::string device = "/dev/dri/card";
    device.append(std::to_string(i));
    m_drm->fd = CDRMUtils::Open(device.c_str());

    if(m_drm->fd >= 0)
    {
      if(!GetResources())
      {
        continue;
      }

      if(!GetConnector())
      {
        continue;
      }

      if(!GetEncoder())
      {
        continue;
      }

      if(!GetCrtc())
      {
        continue;
      }

      auto ret = drmSetClientCap(m_drm->fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
      if (ret)
      {
        CLog::Log(LOGERROR, "CDRMUtils::%s - failed to set Universal planes capability: %s", __FUNCTION__, strerror(errno));
        return false;
      }

      if(!GetPlanes())
      {
        continue;
      }

      break;
    }
  }

  drmModeFreeResources(m_drm_resources);

  if(m_drm->fd < 0)
  {
    return false;
  }

  if(!GetPreferredMode())
  {
    return false;
  }

  drmSetMaster(m_drm->fd);

  m_orig_crtc = drmModeGetCrtc(m_drm->fd, m_drm->crtc->crtc->crtc_id);

  return true;
}

bool CDRMUtils::RestoreOriginalMode()
{
  if(!m_orig_crtc)
  {
    return false;
  }

  auto ret = drmModeSetCrtc(m_drm->fd,
                            m_orig_crtc->crtc_id,
                            m_orig_crtc->buffer_id,
                            m_orig_crtc->x,
                            m_orig_crtc->y,
                            &m_drm->connector->connector->connector_id,
                            1,
                            &m_orig_crtc->mode);

  if(ret)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - failed to set original crtc mode", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "CDRMUtils::%s - set original crtc mode", __FUNCTION__);

  drmModeFreeCrtc(m_orig_crtc);
  m_orig_crtc = nullptr;

  return true;
}

void CDRMUtils::DestroyDrm()
{
  RestoreOriginalMode();

  if (m_drm_resources)
  {
    drmModeFreeResources(m_drm_resources);
  }

  drmDropMaster(m_drm->fd);
  close(m_drm->fd);

  m_drm_resources = nullptr;

  m_drm->primary_plane = nullptr;
  m_drm->overlay_plane = nullptr;
  m_drm->connector = nullptr;
  m_drm->crtc = nullptr;
  m_drm->fd = -1;
  m_drm->mode = nullptr;
}

bool CDRMUtils::GetModes(std::vector<RESOLUTION_INFO> &resolutions)
{
  for(auto i = 0; i < m_drm->connector->connector->count_modes; i++)
  {
    RESOLUTION_INFO res;
    res.iScreen = 0;
    res.iWidth = m_drm->connector->connector->modes[i].hdisplay;
    res.iHeight = m_drm->connector->connector->modes[i].vdisplay;
    res.iScreenWidth = m_drm->connector->connector->modes[i].hdisplay;
    res.iScreenHeight = m_drm->connector->connector->modes[i].vdisplay;
    if (m_drm->connector->connector->modes[i].clock % 10 != 0)
      res.fRefreshRate = (float)m_drm->connector->connector->modes[i].vrefresh * (1000.0f/1001.0f);
    else
      res.fRefreshRate = m_drm->connector->connector->modes[i].vrefresh;
    res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
    res.fPixelRatio = 1.0f;
    res.bFullScreen = true;
    res.strMode = m_drm->connector->connector->modes[i].name;
    res.strId = std::to_string(i);

    if(m_drm->connector->connector->modes[i].flags & DRM_MODE_FLAG_3D_MASK)
    {
      if(m_drm->connector->connector->modes[i].flags & DRM_MODE_FLAG_3D_TOP_AND_BOTTOM)
      {
        res.dwFlags = D3DPRESENTFLAG_MODE3DTB;
      }
      else if(m_drm->connector->connector->modes[i].flags
          & DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF)
      {
        res.dwFlags = D3DPRESENTFLAG_MODE3DSBS;
      }
    }
    else if(m_drm->connector->connector->modes[i].flags & DRM_MODE_FLAG_INTERLACE)
    {
      res.dwFlags = D3DPRESENTFLAG_INTERLACED;
    }
    else
    {
      res.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    }

    resolutions.push_back(res);
  }

  return resolutions.size() > 0;
}

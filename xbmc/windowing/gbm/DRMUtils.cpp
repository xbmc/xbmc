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
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "windowing/GraphicContext.h"

#include "DRMUtils.h"

CDRMUtils::CDRMUtils()
  : m_connector(new connector)
  , m_encoder(new encoder)
  , m_crtc(new crtc)
  , m_primary_plane(new plane)
  , m_overlay_plane(new plane)
{
}

void CDRMUtils::WaitVBlank()
{
  drmVBlank vbl;
  vbl.request.type = DRM_VBLANK_RELATIVE;
  vbl.request.sequence = 1;
  drmWaitVBlank(m_fd, &vbl);
}

bool CDRMUtils::SetMode(const RESOLUTION_INFO& res)
{
  m_mode = &m_connector->connector->modes[atoi(res.strId.c_str())];

  CLog::Log(LOGDEBUG, "CDRMUtils::%s - found crtc mode: %dx%d%s @ %d Hz",
            __FUNCTION__,
            m_mode->hdisplay,
            m_mode->vdisplay,
            m_mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "",
            m_mode->vrefresh);

  return true;
}

void CDRMUtils::DrmFbDestroyCallback(struct gbm_bo *bo, void *data)
{
  struct drm_fb *fb = static_cast<drm_fb *>(data);

  if(fb->fb_id)
  {
    int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
    drmModeRmFB(drm_fd, fb->fb_id);
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

  auto ret = drmModeAddFB2(m_fd,
                           width,
                           height,
                           m_primary_plane->format,
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

static bool GetProperties(int fd, uint32_t id, uint32_t type, struct drm_object *object)
{
  drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(fd, id, type);
  if (!props)
    return false;

  object->id = id;
  object->type = type;
  object->props = props;
  object->props_info = new drmModePropertyPtr[props->count_props];

  for (uint32_t i = 0; i < props->count_props; i++)
    object->props_info[i] = drmModeGetProperty(fd, props->props[i]);

  return true;
}

static void FreeProperties(struct drm_object *object)
{
  if (object->props_info)
  {
    for (uint32_t i = 0; i < object->props->count_props; i++)
      drmModeFreeProperty(object->props_info[i]);

    delete [] object->props_info;
    object->props_info = nullptr;
  }

  drmModeFreeObjectProperties(object->props);
  object->props = nullptr;
  object->type = 0;
  object->id = 0;
}

uint32_t CDRMUtils::GetPropertyId(struct drm_object *object, const char *name)
{
  for (uint32_t i = 0; i < object->props->count_props; i++)
    if (!strcmp(object->props_info[i]->name, name))
      return object->props_info[i]->prop_id;

  CLog::Log(LOGWARNING, "CDRMUtils::%s - could not find property %s", __FUNCTION__, name);
  return 0;
}

bool CDRMUtils::GetResources()
{
  m_drm_resources = drmModeGetResources(m_fd);
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
    m_connector->connector = drmModeGetConnector(m_fd,
                                                      m_drm_resources->connectors[i]);
    if(m_connector->connector->connection == DRM_MODE_CONNECTED)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - found connector: %d", __FUNCTION__,
                                                                 m_connector->connector->connector_id);
      break;
    }
    drmModeFreeConnector(m_connector->connector);
    m_connector->connector = nullptr;
  }

  if(!m_connector->connector)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get connector: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  if (!GetProperties(m_fd, m_connector->connector->connector_id, DRM_MODE_OBJECT_CONNECTOR, m_connector))
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get connector %u properties: %s", __FUNCTION__, m_connector->connector->connector_id, strerror(errno));
    return false;
  }

  return true;
}

bool CDRMUtils::GetEncoder()
{
  for(auto i = 0; i < m_drm_resources->count_encoders; i++)
  {
    m_encoder->encoder = drmModeGetEncoder(m_fd, m_drm_resources->encoders[i]);
    if(m_encoder->encoder->encoder_id == m_connector->connector->encoder_id)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - found encoder: %d", __FUNCTION__,
                                                               m_encoder->encoder->encoder_id);
      break;
    }
    drmModeFreeEncoder(m_encoder->encoder);
    m_encoder->encoder = nullptr;
  }

  if(!m_encoder->encoder)
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
    m_crtc->crtc = drmModeGetCrtc(m_fd, m_drm_resources->crtcs[i]);
    if(m_crtc->crtc->crtc_id == m_encoder->encoder->crtc_id)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - found crtc: %d", __FUNCTION__,
                                                            m_crtc->crtc->crtc_id);
      m_crtc_index = i;
      break;
    }
    drmModeFreeCrtc(m_crtc->crtc);
    m_crtc->crtc = nullptr;
  }

  if(!m_crtc->crtc)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get crtc: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  if (!GetProperties(m_fd, m_crtc->crtc->crtc_id, DRM_MODE_OBJECT_CRTC, m_crtc))
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get crtc %u properties: %s", __FUNCTION__, m_crtc->crtc->crtc_id, strerror(errno));
    return false;
  }

  return true;
}

bool CDRMUtils::GetPreferredMode()
{
  for(auto i = 0, area = 0; i < m_connector->connector->count_modes; i++)
  {
    drmModeModeInfo *current_mode = &m_connector->connector->modes[i];

    if(current_mode->type & DRM_MODE_TYPE_PREFERRED)
    {
      m_mode = current_mode;
      CLog::Log(LOGDEBUG,
                "CDRMUtils::%s - found preferred mode: %dx%d%s @ %d Hz",
                __FUNCTION__,
                m_mode->hdisplay,
                m_mode->vdisplay,
                m_mode->flags & DRM_MODE_FLAG_INTERLACE ? "i" : "",
                m_mode->vrefresh);
      break;
    }

    auto current_area = current_mode->hdisplay * current_mode->vdisplay;
    if (current_area > area)
    {
      m_mode = current_mode;
      area = current_area;
    }
  }

  if(!m_mode)
  {
    CLog::Log(LOGDEBUG, "CDRMUtils::%s - failed to find preferred mode", __FUNCTION__);
    return false;
  }

  return true;
}

bool CDRMUtils::GetPlanes()
{
  drmModePlaneResPtr plane_resources;
  uint32_t primary_plane_id = 0;
  uint32_t overlay_plane_id = 0;
  uint32_t fourcc = 0;

  plane_resources = drmModeGetPlaneResources(m_fd);
  if (!plane_resources)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - drmModeGetPlaneResources failed: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  for (uint32_t i = 0; i < plane_resources->count_planes; i++)
  {
    uint32_t id = plane_resources->planes[i];
    drmModePlanePtr plane = drmModeGetPlane(m_fd, id);
    if (!plane)
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - drmModeGetPlane(%u) failed: %s", __FUNCTION__, id, strerror(errno));
      continue;
    }

    if (plane->possible_crtcs & (1 << m_crtc_index))
    {
      drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(m_fd, id, DRM_MODE_OBJECT_PLANE);

      for (uint32_t j = 0; j < props->count_props; j++)
      {
        drmModePropertyPtr p = drmModeGetProperty(m_fd, props->props[j]);

        if ((strcmp(p->name, "type") == 0) && (props->prop_values[j] == DRM_PLANE_TYPE_PRIMARY) && (primary_plane_id == 0))
        {
          CLog::Log(LOGDEBUG, "CDRMUtils::%s - found primary plane: %u", __FUNCTION__, id);
          primary_plane_id = id;
        }
        else if ((strcmp(p->name, "type") == 0) && (props->prop_values[j] == DRM_PLANE_TYPE_OVERLAY) && (overlay_plane_id == 0))
        {
          CLog::Log(LOGDEBUG, "CDRMUtils::%s - found overlay plane: %u", __FUNCTION__, id);
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
  m_primary_plane->plane = drmModeGetPlane(m_fd, primary_plane_id);
  if (!m_primary_plane->plane)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get primary plane %u: %s", __FUNCTION__, primary_plane_id, strerror(errno));
    return false;
  }

  if (!GetProperties(m_fd, primary_plane_id, DRM_MODE_OBJECT_PLANE, m_primary_plane))
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get primary plane %u properties: %s", __FUNCTION__, primary_plane_id, strerror(errno));
    return false;
  }

  for (uint32_t i = 0; i < m_primary_plane->plane->count_formats; i++)
  {
    /* we want an alpha layer so break if we find one */
    if (m_primary_plane->plane->formats[i] == DRM_FORMAT_XRGB8888)
    {
      fourcc = DRM_FORMAT_XRGB8888;
      m_primary_plane->format = fourcc;
    }
    else if (m_primary_plane->plane->formats[i] == DRM_FORMAT_ARGB8888)
    {
      fourcc = DRM_FORMAT_ARGB8888;
      m_primary_plane->format = fourcc;
      break;
    }
  }

  if (fourcc == 0)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not find a suitable primary plane format", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "CDRMUtils::%s - primary plane format: %c%c%c%c", __FUNCTION__, fourcc, fourcc >> 8, fourcc >> 16, fourcc >> 24);

  if (overlay_plane_id != 0)
  {
    // overlay plane
    m_overlay_plane->plane = drmModeGetPlane(m_fd, overlay_plane_id);
    if (!m_overlay_plane->plane)
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - could not get overlay plane %u: %s", __FUNCTION__, overlay_plane_id, strerror(errno));
      return false;
    }

    if (!GetProperties(m_fd, overlay_plane_id, DRM_MODE_OBJECT_PLANE, m_overlay_plane))
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - could not get overlay plane %u properties: %s", __FUNCTION__, overlay_plane_id, strerror(errno));
      return false;
    }

    fourcc = 0;

    for (uint32_t i = 0; i < m_overlay_plane->plane->count_formats; i++)
    {
      /* we want an alpha layer so break if we find one */
      if (m_overlay_plane->plane->formats[i] == DRM_FORMAT_XRGB8888)
      {
        fourcc = DRM_FORMAT_XRGB8888;
        m_overlay_plane->format = fourcc;
      }
      else if(m_overlay_plane->plane->formats[i] == DRM_FORMAT_ARGB8888)
      {
        fourcc = DRM_FORMAT_ARGB8888;
        m_overlay_plane->format = fourcc;
        break;
      }
    }

    if (fourcc == 0)
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - could not find a suitable overlay plane format", __FUNCTION__);
      return false;
    }

    CLog::Log(LOGDEBUG, "CDRMUtils::%s - overlay plane format: %c%c%c%c", __FUNCTION__, fourcc, fourcc >> 8, fourcc >> 16, fourcc >> 24);
  }
  else
  {
    delete m_overlay_plane;
    m_overlay_plane = m_primary_plane;
  }
  return true;
}

bool CDRMUtils::OpenDrm()
{
  static constexpr const char *modules[] =
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
    "meson"
  };

  for(int i = 0; i < 10; ++i)
  {
    std::string device = "/dev/dri/card";
    device.append(std::to_string(i));

    for (auto module : modules)
    {
      m_fd = drmOpen(module, device.c_str());
      if (m_fd >= 0)
      {
        if(!GetResources())
        {
          continue;
        }

        if(!GetConnector())
        {
          continue;
        }

        drmModeFreeResources(m_drm_resources);
        m_drm_resources = nullptr;

        drmModeFreeConnector(m_connector->connector);
        m_connector->connector = nullptr;
        FreeProperties(m_connector);

        m_module = module;
        m_device_path = device;

        CLog::Log(LOGDEBUG, "CDRMUtils::%s - opened device: %s using module: %s", __FUNCTION__, device.c_str(), module);
        return true;
      }

      drmClose(m_fd);
      m_fd = -1;
    }
  }

  return false;
}

bool CDRMUtils::InitDrm()
{
  if(m_fd >= 0)
  {
    /* caps need to be set before allocating connectors, encoders, crtcs, and planes */
    auto ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret)
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - failed to set Universal planes capability: %s", __FUNCTION__, strerror(errno));
      return false;
    }

    if(!GetResources())
    {
      return false;
    }

    if(!GetConnector())
    {
      return false;
    }

    if(!GetEncoder())
    {
      return false;
    }

    if(!GetCrtc())
    {
      return false;
    }

    if(!GetPlanes())
    {
      return false;
    }
  }

  drmModeFreeResources(m_drm_resources);
  m_drm_resources = nullptr;

  if(m_fd < 0)
  {
    return false;
  }

  if(!GetPreferredMode())
  {
    return false;
  }

  drmSetMaster(m_fd);

  m_orig_crtc = drmModeGetCrtc(m_fd, m_crtc->crtc->crtc_id);

  return true;
}

bool CDRMUtils::RestoreOriginalMode()
{
  if(!m_orig_crtc)
  {
    return false;
  }

  auto ret = drmModeSetCrtc(m_fd,
                            m_orig_crtc->crtc_id,
                            m_orig_crtc->buffer_id,
                            m_orig_crtc->x,
                            m_orig_crtc->y,
                            &m_connector->connector->connector_id,
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

  drmDropMaster(m_fd);
  close(m_fd);

  m_fd = -1;

  drmModeFreeResources(m_drm_resources);
  m_drm_resources = nullptr;

  drmModeFreeConnector(m_connector->connector);
  FreeProperties(m_connector);
  delete m_connector;
  m_connector = nullptr;

  drmModeFreeEncoder(m_encoder->encoder);
  delete m_encoder;
  m_encoder = nullptr;

  drmModeFreeCrtc(m_crtc->crtc);
  FreeProperties(m_crtc);
  delete m_crtc;
  m_crtc = nullptr;

  drmModeFreePlane(m_primary_plane->plane);
  FreeProperties(m_primary_plane);
  delete m_primary_plane;

  if (m_overlay_plane != m_primary_plane)
  {
    drmModeFreePlane(m_overlay_plane->plane);
    FreeProperties(m_overlay_plane);
    delete m_overlay_plane;
  }
  m_overlay_plane = nullptr;
  m_primary_plane = nullptr;
}

std::vector<RESOLUTION_INFO> CDRMUtils::GetModes()
{
  std::vector<RESOLUTION_INFO> resolutions;
  resolutions.reserve(m_connector->connector->count_modes);

  for(auto i = 0; i < m_connector->connector->count_modes; i++)
  {
    RESOLUTION_INFO res;
    res.iScreen = 0;
    res.iWidth = m_connector->connector->modes[i].hdisplay;
    res.iHeight = m_connector->connector->modes[i].vdisplay;
    res.iScreenWidth = m_connector->connector->modes[i].hdisplay;
    res.iScreenHeight = m_connector->connector->modes[i].vdisplay;
    if (m_connector->connector->modes[i].clock % 5 != 0)
      res.fRefreshRate = (float)m_connector->connector->modes[i].vrefresh * (1000.0f/1001.0f);
    else
      res.fRefreshRate = m_connector->connector->modes[i].vrefresh;
    res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
    res.fPixelRatio = 1.0f;
    res.bFullScreen = true;
    res.strId = std::to_string(i);

    if(m_connector->connector->modes[i].flags & DRM_MODE_FLAG_3D_MASK)
    {
      if(m_connector->connector->modes[i].flags & DRM_MODE_FLAG_3D_TOP_AND_BOTTOM)
      {
        res.dwFlags = D3DPRESENTFLAG_MODE3DTB;
      }
      else if(m_connector->connector->modes[i].flags
          & DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF)
      {
        res.dwFlags = D3DPRESENTFLAG_MODE3DSBS;
      }
    }
    else if(m_connector->connector->modes[i].flags & DRM_MODE_FLAG_INTERLACE)
    {
      res.dwFlags = D3DPRESENTFLAG_INTERLACED;
    }
    else
    {
      res.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;
    }

    res.strMode = StringUtils::Format("%dx%d%s @ %.6f Hz", res.iScreenWidth, res.iScreenHeight,
                                      res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "", res.fRefreshRate);
    resolutions.push_back(res);
  }

  return resolutions;
}

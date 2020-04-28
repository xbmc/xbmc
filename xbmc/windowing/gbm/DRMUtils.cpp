/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DRMUtils.h"

#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/XTimeUtils.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <EGL/egl.h>
#include <drm_mode.h>
#include <fcntl.h>
#include <unistd.h>

using namespace KODI::WINDOWING::GBM;

namespace
{
const std::string SETTING_VIDEOSCREEN_LIMITGUISIZE = "videoscreen.limitguisize";
}

CDRMUtils::CDRMUtils()
  : m_connector(new connector)
  , m_encoder(new encoder)
  , m_video_plane(new plane)
  , m_gui_plane(new plane)
{
}

bool CDRMUtils::SetMode(const RESOLUTION_INFO& res)
{
  if (!CheckConnector(m_connector->connector->connector_id))
    return false;

  m_mode = &m_connector->connector->modes[atoi(res.strId.c_str())];
  m_width = res.iWidth;
  m_height = res.iHeight;

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

  if (fb->fb_id > 0)
  {
    CLog::Log(LOGDEBUG, "CDRMUtils::%s - removing framebuffer: %d", __FUNCTION__, fb->fb_id);
    int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
    drmModeRmFB(drm_fd, fb->fb_id);
  }

  delete fb;
}

drm_fb * CDRMUtils::DrmFbGetFromBo(struct gbm_bo *bo)
{
  {
    struct drm_fb *fb = static_cast<drm_fb *>(gbm_bo_get_user_data(bo));
    if(fb)
    {
      if (m_gui_plane->GetFormat() == fb->format)
        return fb;
      else
        DrmFbDestroyCallback(bo, gbm_bo_get_user_data(bo));
    }
  }

  struct drm_fb *fb = new drm_fb;
  fb->bo = bo;
  fb->format = m_gui_plane->GetFormat();

  uint32_t width,
           height,
           handles[4] = {0},
           strides[4] = {0},
           offsets[4] = {0};

  uint64_t modifiers[4] = {0};

  width = gbm_bo_get_width(bo);
  height = gbm_bo_get_height(bo);

#if defined(HAS_GBM_MODIFIERS)
  for (int i = 0; i < gbm_bo_get_plane_count(bo); i++)
  {
    handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
    strides[i] = gbm_bo_get_stride_for_plane(bo, i);
    offsets[i] = gbm_bo_get_offset(bo, i);
    modifiers[i] = gbm_bo_get_modifier(bo);
  }
#else
  handles[0] = gbm_bo_get_handle(bo).u32;
  strides[0] = gbm_bo_get_stride(bo);
  memset(offsets, 0, 16);
#endif

  uint32_t flags = 0;

  if (modifiers[0] && modifiers[0] != DRM_FORMAT_MOD_INVALID)
  {
    flags |= DRM_MODE_FB_MODIFIERS;
    CLog::Log(LOGDEBUG, "CDRMUtils::{} - using modifier: {:#x}", __FUNCTION__, modifiers[0]);
  }

  int ret = drmModeAddFB2WithModifiers(m_fd,
                                       width,
                                       height,
                                       fb->format,
                                       handles,
                                       strides,
                                       offsets,
                                       modifiers,
                                       &fb->fb_id,
                                       flags);

  if(ret < 0)
  {
    ret = drmModeAddFB2(m_fd,
                        width,
                        height,
                        fb->format,
                        handles,
                        strides,
                        offsets,
                        &fb->fb_id,
                        flags);

    if (ret < 0)
    {
      delete (fb);
      CLog::Log(LOGDEBUG, "CDRMUtils::{} - failed to add framebuffer: {} ({})", __FUNCTION__, strerror(errno), errno);
      return nullptr;
    }
  }

  gbm_bo_set_user_data(bo, fb, DrmFbDestroyCallback);

  return fb;
}

bool CDRMUtils::GetProperties(int fd, uint32_t id, uint32_t type, struct drm_object *object)
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

void CDRMUtils::FreeProperties(struct drm_object *object)
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

bool CDRMUtils::SupportsProperty(struct drm_object *object, const char *name)
{
  for (uint32_t i = 0; i < object->props->count_props; i++)
    if (!strcmp(object->props_info[i]->name, name))
      return true;

  return false;
}

bool CDRMUtils::SupportsPropertyAndValue(struct drm_object* object,
                                         const char* name,
                                         uint64_t value)
{
  for (uint32_t i = 0; i < object->props->count_props; i++)
  {
    if (!StringUtils::EqualsNoCase(object->props_info[i]->name, name))
      continue;

    if (drm_property_type_is(object->props_info[i], DRM_MODE_PROP_ENUM) != 0)
    {
      for (int j = 0; j < object->props_info[i]->count_enums; j++)
      {
        if (object->props_info[i]->enums[j].value == value)
          return true;
      }
    }

    CLog::Log(LOGDEBUG, "CDRMUtils::{} - property '{}' does not support value '{}'", __FUNCTION__,
              name, value);
    break;
  }

  return false;
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

bool CDRMUtils::FindConnector()
{
  for(auto i = 0; i < m_drm_resources->count_connectors; i++)
  {
    m_connector->connector = drmModeGetConnector(m_fd, m_drm_resources->connectors[i]);
    if((m_connector->connector->encoder_id > 0) &&
        m_connector->connector->connection == DRM_MODE_CONNECTED)
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

bool CDRMUtils::FindEncoder()
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

bool CDRMUtils::FindCrtcs()
{
  for (auto i = 0; i < m_drm_resources->count_crtcs; i++)
  {
    struct crtc* object = nullptr;

    if (m_encoder->encoder->possible_crtcs & (1 << i))
    {
      object = new struct crtc;
      object->crtc = drmModeGetCrtc(m_fd, m_drm_resources->crtcs[i]);

      CLog::Log(LOGDEBUG, "CDRMUtils::{} - found possible crtc: {}", __FUNCTION__,
                object->crtc->crtc_id);

      if (!GetProperties(m_fd, object->crtc->crtc_id, DRM_MODE_OBJECT_CRTC, object))
      {
        CLog::Log(LOGERROR, "CDRMUtils::{} - could not get crtc {} properties: {}", __FUNCTION__,
                  object->crtc->crtc_id, strerror(errno));
        drmModeFreeCrtc(object->crtc);
        delete object;
        object = nullptr;
      }

      if (object && object->crtc->crtc_id == m_encoder->encoder->crtc_id)
        m_orig_crtc = object;
    }

    m_crtcs.emplace_back(object);
  }

  if (m_crtcs.empty())
  {
    CLog::Log(LOGERROR, "CDRMUtils::{} - could not get crtc: {}", __FUNCTION__, strerror(errno));
    return false;
  }

  return true;
}

bool CDRMUtils::FindPreferredMode()
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

bool CDRMUtils::SupportsFormat(drmModePlanePtr plane, uint32_t format)
{
  for (uint32_t i = 0; i < plane->count_formats; i++)
    if (plane->formats[i] == format)
      return true;

  return false;
}

drmModePlanePtr CDRMUtils::FindPlane(drmModePlaneResPtr resources, int crtc_index, int type)
{
  for (uint32_t i = 0; i < resources->count_planes; i++)
  {
    drmModePlanePtr plane = drmModeGetPlane(m_fd, resources->planes[i]);

    if (plane && plane->possible_crtcs & (1 << crtc_index))
    {
      drmModeObjectPropertiesPtr props = drmModeObjectGetProperties(m_fd, plane->plane_id, DRM_MODE_OBJECT_PLANE);

      for (uint32_t j = 0; j < props->count_props; j++)
      {
        drmModePropertyPtr p = drmModeGetProperty(m_fd, props->props[j]);

        if ((strcmp(p->name, "type") == 0) && (props->prop_values[j] != DRM_PLANE_TYPE_CURSOR))
        {
          switch (type)
          {
            case KODI_VIDEO_PLANE:
            {
              if (SupportsFormat(plane, DRM_FORMAT_NV12))
              {
                CLog::Log(LOGDEBUG, "CDRMUtils::%s - found video plane %u", __FUNCTION__, plane->plane_id);
                drmModeFreeProperty(p);
                drmModeFreeObjectProperties(props);
                return plane;
              }

              break;
            }
            case KODI_GUI_PLANE:
            {
              uint32_t plane_id = 0;
              if (m_video_plane->plane)
                plane_id = m_video_plane->plane->plane_id;

              if (plane->plane_id != plane_id &&
                  (plane_id == 0 || SupportsFormat(plane, DRM_FORMAT_ARGB8888)) &&
                  SupportsFormat(plane, DRM_FORMAT_XRGB8888))
              {
                CLog::Log(LOGDEBUG, "CDRMUtils::%s - found gui plane %u", __FUNCTION__, plane->plane_id);
                drmModeFreeProperty(p);
                drmModeFreeObjectProperties(props);
                return plane;
              }

              break;
            }
          }
        }

        drmModeFreeProperty(p);
      }

      drmModeFreeObjectProperties(props);
    }

    drmModeFreePlane(plane);
  }

  CLog::Log(LOGWARNING, "CDRMUtils::{} - could not find {} plane for crtc index {}", __FUNCTION__,
            (type == KODI_VIDEO_PLANE) ? "video" : "gui", crtc_index);
  return nullptr;
}

bool CDRMUtils::FindPlanes()
{
  drmModePlaneResPtr plane_resources = drmModeGetPlaneResources(m_fd);
  if (!plane_resources)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - drmModeGetPlaneResources failed: %s", __FUNCTION__,
              strerror(errno));
    return false;
  }

  drmModePlanePtr fallback = nullptr;

  for (size_t i = 0; i < m_crtcs.size(); i++)
  {
    const auto crtc = m_crtcs[i];
    if (!crtc)
      continue;

    m_video_plane->plane = FindPlane(plane_resources, i, KODI_VIDEO_PLANE);
    m_gui_plane->plane = FindPlane(plane_resources, i, KODI_GUI_PLANE);

    if (m_video_plane->plane && m_gui_plane->plane)
    {
      m_crtc = crtc;
      break;
    }

    if (m_gui_plane->plane)
    {
      if (!m_crtc && m_encoder->encoder->crtc_id == crtc->crtc->crtc_id)
      {
        m_crtc = crtc;
        fallback = m_gui_plane->plane;
      }
      else
      {
        drmModeFreePlane(m_gui_plane->plane);
        m_gui_plane->plane = nullptr;
      }
    }

    if (m_video_plane->plane)
    {
      drmModeFreePlane(m_video_plane->plane);
      m_video_plane->plane = nullptr;
    }
  }

  if (!m_gui_plane->plane)
    m_gui_plane->plane = fallback;

  drmModeFreePlaneResources(plane_resources);

  // video plane may not be available
  if (m_video_plane->plane)
  {
    CLog::Log(LOGDEBUG, "CDRMUtils::{} - using video plane {}", __FUNCTION__,
              m_video_plane->plane->plane_id);

    if (!GetProperties(m_fd, m_video_plane->plane->plane_id, DRM_MODE_OBJECT_PLANE, m_video_plane))
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - could not get video plane %u properties: %s", __FUNCTION__, m_video_plane->plane->plane_id, strerror(errno));
      return false;
    }

    if (!FindModifiersForPlane(m_video_plane))
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - no drm modifiers present for the video plane",
                __FUNCTION__);
    }
  }

  CLog::Log(LOGDEBUG, "CDRMUtils::{} - using gui plane {}", __FUNCTION__,
            m_gui_plane->plane->plane_id);

  // gui plane should always be available
  if (!GetProperties(m_fd, m_gui_plane->plane->plane_id, DRM_MODE_OBJECT_PLANE, m_gui_plane))
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - could not get gui plane %u properties: %s", __FUNCTION__, m_gui_plane->plane->plane_id, strerror(errno));
    return false;
  }

  if (!FindModifiersForPlane(m_gui_plane))
  {
    CLog::Log(LOGDEBUG, "CDRMUtils::%s - no drm modifiers present for the gui plane", __FUNCTION__);
    m_gui_plane->modifiers_map.emplace(DRM_FORMAT_ARGB8888, std::vector<uint64_t>{DRM_FORMAT_MOD_LINEAR});
    m_gui_plane->modifiers_map.emplace(DRM_FORMAT_XRGB8888, std::vector<uint64_t>{DRM_FORMAT_MOD_LINEAR});
  }

  return true;
}

bool CDRMUtils::FindModifiersForPlane(struct plane *object)
{
  uint64_t blob_id = 0;

  for (uint32_t i = 0; i < object->props->count_props; i++)
  {
    if (strcmp(object->props_info[i]->name, "IN_FORMATS") == 0)
      blob_id = object->props->prop_values[i];
  }

  if (blob_id == 0)
    return false;

  drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(m_fd, blob_id);
  if (!blob)
    return false;

  drm_format_modifier_blob *header = static_cast<drm_format_modifier_blob*>(blob->data);
  uint32_t *formats = reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(header) + header->formats_offset);
  drm_format_modifier *mod = reinterpret_cast<drm_format_modifier*>(reinterpret_cast<char*>(header) + header->modifiers_offset);

  for (uint32_t i = 0; i < header->count_formats; i++)
  {
    std::vector<uint64_t> modifiers;
    for (uint32_t j = 0; j < header->count_modifiers; j++)
    {
      if (mod[j].formats & 1ULL << i)
        modifiers.emplace_back(mod[j].modifier);
    }

    object->modifiers_map.emplace(formats[i], modifiers);
  }

  if (blob)
    drmModeFreePropertyBlob(blob);

  return true;
}

bool CDRMUtils::OpenDrm(bool needConnector)
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

  for (auto module : modules)
  {
    m_fd.attach(drmOpenWithType(module, nullptr, DRM_NODE_PRIMARY));
    if (m_fd)
    {
      if(!GetResources())
      {
        continue;
      }

      if (needConnector)
      {
        if(!FindConnector())
        {
          continue;
        }

        drmModeFreeConnector(m_connector->connector);
        m_connector->connector = nullptr;
        FreeProperties(m_connector);
      }

      drmModeFreeResources(m_drm_resources);
      m_drm_resources = nullptr;

      m_module = module;

      CLog::Log(LOGDEBUG, "CDRMUtils::%s - opened device: %s using module: %s", __FUNCTION__, drmGetDeviceNameFromFd2(m_fd), module);

      m_renderFd.attach(drmOpenWithType(module, nullptr, DRM_NODE_RENDER));
      if (m_renderFd)
      {
        CLog::Log(LOGDEBUG, "CDRMUtils::%s - opened render node: %s using module: %s", __FUNCTION__, drmGetDeviceNameFromFd2(m_renderFd), module);
      }

      return true;
    }
  }

  m_fd.reset();

  return false;
}

bool CDRMUtils::InitDrm()
{
  if(m_fd)
  {
    /* caps need to be set before allocating connectors, encoders, crtcs, and planes */
    auto ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret)
    {
      CLog::Log(LOGERROR, "CDRMUtils::{} - failed to set universal planes capability: {}", __FUNCTION__, strerror(errno));
      return false;
    }

    ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_STEREO_3D, 1);
    if (ret)
    {
      CLog::Log(LOGERROR, "CDRMUtils::{} - failed to set stereo 3d capability: {}", __FUNCTION__, strerror(errno));
      return false;
    }

#if defined(DRM_CLIENT_CAP_ASPECT_RATIO)
    ret = drmSetClientCap(m_fd, DRM_CLIENT_CAP_ASPECT_RATIO, 0);
    if (ret != 0)
    {
      CLog::Log(LOGERROR, "CDRMUtils::{} - aspect ratio capability is not supported: {}", __FUNCTION__, strerror(errno));
    }
#endif

    if(!GetResources())
    {
      return false;
    }

    if(!FindConnector())
    {
      return false;
    }

    if (!FindEncoder())
    {
      return false;
    }

    if (!FindCrtcs())
    {
      return false;
    }

    if(!FindPlanes())
    {
      return false;
    }
  }

  drmModeFreeResources(m_drm_resources);
  m_drm_resources = nullptr;

  if(!m_fd)
  {
    return false;
  }

  if(!FindPreferredMode())
  {
    return false;
  }

  auto ret = drmSetMaster(m_fd);
  if (ret < 0)
  {
    CLog::Log(LOGWARNING, "CDRMUtils::%s - failed to set drm master, will try to authorize instead: %s", __FUNCTION__, strerror(errno));

    drm_magic_t magic;

    ret = drmGetMagic(m_fd, &magic);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - failed to get drm magic: %s", __FUNCTION__, strerror(errno));
      return false;
    }

    ret = drmAuthMagic(m_fd, magic);
    if (ret < 0)
    {
      CLog::Log(LOGERROR, "CDRMUtils::%s - failed to authorize drm magic: %s", __FUNCTION__, strerror(errno));
      return false;
    }

    CLog::Log(LOGINFO, "CDRMUtils::%s - successfully authorized drm magic", __FUNCTION__);
  }

  return true;
}

bool CDRMUtils::RestoreOriginalMode()
{
  if(!m_orig_crtc)
  {
    return false;
  }

  auto ret = drmModeSetCrtc(m_fd, m_orig_crtc->crtc->crtc_id, m_orig_crtc->crtc->buffer_id,
                            m_orig_crtc->crtc->x, m_orig_crtc->crtc->y,
                            &m_connector->connector->connector_id, 1, &m_orig_crtc->crtc->mode);

  if(ret)
  {
    CLog::Log(LOGERROR, "CDRMUtils::%s - failed to set original crtc mode", __FUNCTION__);
    return false;
  }

  CLog::Log(LOGDEBUG, "CDRMUtils::%s - set original crtc mode", __FUNCTION__);

  return true;
}

void CDRMUtils::DestroyDrm()
{
  RestoreOriginalMode();

  auto ret = drmDropMaster(m_fd);
  if (ret < 0)
  {
    CLog::Log(LOGDEBUG, "CDRMUtils::%s - failed to drop drm master: %s", __FUNCTION__, strerror(errno));
  }

  m_renderFd.reset();
  m_fd.reset();

  drmModeFreeResources(m_drm_resources);
  m_drm_resources = nullptr;

  drmModeFreeConnector(m_connector->connector);
  FreeProperties(m_connector);
  delete m_connector;
  m_connector = nullptr;

  drmModeFreeEncoder(m_encoder->encoder);
  delete m_encoder;
  m_encoder = nullptr;

  for (auto crtc : m_crtcs)
  {
    drmModeFreeCrtc(crtc->crtc);
    FreeProperties(crtc);
    delete crtc;
    crtc = nullptr;
  }

  m_crtc = nullptr;
  m_orig_crtc = nullptr;

  drmModeFreePlane(m_video_plane->plane);
  FreeProperties(m_video_plane);
  delete m_video_plane;
  m_video_plane = nullptr;

  drmModeFreePlane(m_gui_plane->plane);
  FreeProperties(m_gui_plane);
  delete m_gui_plane;
  m_gui_plane = nullptr;
}

RESOLUTION_INFO CDRMUtils::GetResolutionInfo(drmModeModeInfoPtr mode)
{
  RESOLUTION_INFO res;
  res.iScreenWidth = mode->hdisplay;
  res.iScreenHeight = mode->vdisplay;
  res.iWidth = res.iScreenWidth;
  res.iHeight = res.iScreenHeight;

  int limit = CServiceBroker::GetSettingsComponent()->GetSettings()->GetInt(
      SETTING_VIDEOSCREEN_LIMITGUISIZE);
  if (limit > 0 && res.iScreenWidth > 1920 && res.iScreenHeight > 1080)
  {
    switch (limit)
    {
      case 1: // 720p
        res.iWidth = 1280;
        res.iHeight = 720;
        break;
      case 2: // 1080p / 720p (>30hz)
        res.iWidth = mode->vrefresh > 30 ? 1280 : 1920;
        res.iHeight = mode->vrefresh > 30 ? 720 : 1080;
        break;
      case 3: // 1080p
        res.iWidth = 1920;
        res.iHeight = 1080;
        break;
      case 4: // Unlimited / 1080p (>30hz)
        res.iWidth = mode->vrefresh > 30 ? 1920 : res.iScreenWidth;
        res.iHeight = mode->vrefresh > 30 ? 1080 : res.iScreenHeight;
        break;
    }
  }

  if (mode->clock % 5 != 0)
    res.fRefreshRate = static_cast<float>(mode->vrefresh) * (1000.0f/1001.0f);
  else
    res.fRefreshRate = mode->vrefresh;
  res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
  res.fPixelRatio = 1.0f;
  res.bFullScreen = true;

  if (mode->flags & DRM_MODE_FLAG_3D_MASK)
  {
    if (mode->flags & DRM_MODE_FLAG_3D_TOP_AND_BOTTOM)
      res.dwFlags = D3DPRESENTFLAG_MODE3DTB;
    else if (mode->flags & DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF)
      res.dwFlags = D3DPRESENTFLAG_MODE3DSBS;
  }
  else if (mode->flags & DRM_MODE_FLAG_INTERLACE)
    res.dwFlags = D3DPRESENTFLAG_INTERLACED;
  else
    res.dwFlags = D3DPRESENTFLAG_PROGRESSIVE;

  res.strMode = StringUtils::Format("%dx%d%s @ %.6f Hz", res.iScreenWidth, res.iScreenHeight,
                                    res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "", res.fRefreshRate);
  return res;
}

RESOLUTION_INFO CDRMUtils::GetCurrentMode()
{
  return GetResolutionInfo(m_mode);
}

std::vector<RESOLUTION_INFO> CDRMUtils::GetModes()
{
  std::vector<RESOLUTION_INFO> resolutions;
  resolutions.reserve(m_connector->connector->count_modes);

  for(auto i = 0; i < m_connector->connector->count_modes; i++)
  {
    RESOLUTION_INFO res = GetResolutionInfo(&m_connector->connector->modes[i]);
    res.strId = std::to_string(i);
    resolutions.push_back(res);
  }

  return resolutions;
}

uint32_t CDRMUtils::FourCCWithAlpha(uint32_t fourcc)
{
  return (fourcc & 0xFFFFFF00) | static_cast<uint32_t>('A');
}

uint32_t CDRMUtils::FourCCWithoutAlpha(uint32_t fourcc)
{
  return (fourcc & 0xFFFFFF00) | static_cast<uint32_t>('X');
}

bool CDRMUtils::CheckConnector(int connector_id)
{
  struct connector connectorcheck;
  unsigned retryCnt = 7;

  connectorcheck.connector = drmModeGetConnector(m_fd, connector_id);
  while (connectorcheck.connector->connection != DRM_MODE_CONNECTED  && retryCnt > 0)
  {
    CLog::Log(LOGDEBUG, "CDRMUtils::%s - connector is disconnected", __FUNCTION__);
    retryCnt--;
    KODI::TIME::Sleep(1000);
    drmModeFreeConnector(connectorcheck.connector);
    connectorcheck.connector = drmModeGetConnector(m_fd, connector_id);
  }

  int finalConnectionState = connectorcheck.connector->connection;
  drmModeFreeConnector(connectorcheck.connector);

  return finalConnectionState == DRM_MODE_CONNECTED;
}

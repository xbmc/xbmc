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
#include <drm/drm_fourcc.h>
#include <drm/drm_mode.h>
#include <EGL/egl.h>
#include <unistd.h>

#include "WinSystemGbmGLESContext.h"
#include "guilib/gui3d.h"
#include "utils/log.h"

#include "DRMUtils.h"

static struct drm *m_drm = nullptr;

static drmModeResPtr m_drm_resources = nullptr;
static drmModeConnectorPtr m_drm_connector = nullptr;
static drmModeEncoderPtr m_drm_encoder = nullptr;
static drmModeCrtcPtr m_orig_crtc = nullptr;

bool CDRMUtils::GetMode(RESOLUTION_INFO res)
{
  m_drm->mode = &m_drm_connector->modes[atoi(res.strId.c_str())];

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
    m_drm_connector = drmModeGetConnector(m_drm->fd,
                                          m_drm_resources->connectors[i]);
    if(m_drm_connector->connection == DRM_MODE_CONNECTED)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - found connector: %d", __FUNCTION__,
                                                                 m_drm_connector->connector_type);
      break;
    }
    drmModeFreeConnector(m_drm_connector);
    m_drm_connector = nullptr;
  }

  if(!m_drm_connector)
  {
    return false;
  }

  return true;
}

bool CDRMUtils::GetEncoder()
{
  for(auto i = 0; i < m_drm_resources->count_encoders; i++)
  {
    m_drm_encoder = drmModeGetEncoder(m_drm->fd, m_drm_resources->encoders[i]);
    if(m_drm_encoder->encoder_id == m_drm_connector->encoder_id)
    {
      CLog::Log(LOGDEBUG, "CDRMUtils::%s - found encoder: %d", __FUNCTION__,
                                                               m_drm_encoder->encoder_type);
      break;
    }
    drmModeFreeEncoder(m_drm_encoder);
    m_drm_encoder = nullptr;
  }

  if(!m_drm_encoder)
  {
    return false;
  }

  return true;
}

bool CDRMUtils::GetPreferredMode()
{
  for(auto i = 0, area = 0; i < m_drm_connector->count_modes; i++)
  {
    drmModeModeInfo *current_mode = &m_drm_connector->modes[i];

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

bool CDRMUtils::InitDrm(drm *drm)
{
  m_drm = drm;
  const char *device = "/dev/dri/card0";

  m_drm->fd = open(device, O_RDWR);

  if(m_drm->fd < 0)
  {
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
  else
  {
    m_drm->crtc_id = m_drm_encoder->crtc_id;
  }

  if(!GetPreferredMode())
  {
    return false;
  }

  for(auto i = 0; i < m_drm_resources->count_crtcs; i++)
  {
    if(m_drm_resources->crtcs[i] == m_drm->crtc_id)
    {
      m_drm->crtc_index = i;
      break;
    }
  }

  drmModeFreeResources(m_drm_resources);

  drmSetMaster(m_drm->fd);

  m_drm->connector_id = m_drm_connector->connector_id;
  m_orig_crtc = drmModeGetCrtc(m_drm->fd, m_drm->crtc_id);

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
                            &m_drm->connector_id,
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

  if(m_drm_encoder)
  {
    drmModeFreeEncoder(m_drm_encoder);
  }

  if(m_drm_connector)
  {
    drmModeFreeConnector(m_drm_connector);
  }

  if(m_drm_resources)
  {
    drmModeFreeResources(m_drm_resources);
  }

  drmDropMaster(m_drm->fd);
  close(m_drm->fd);

  m_drm_encoder = nullptr;
  m_drm_connector = nullptr;
  m_drm_resources = nullptr;

  m_drm->connector = nullptr;
  m_drm->connector_id = 0;
  m_drm->crtc = nullptr;
  m_drm->crtc_id = 0;
  m_drm->crtc_index = 0;
  m_drm->fd = -1;
  m_drm->mode = nullptr;
}

bool CDRMUtils::GetModes(std::vector<RESOLUTION_INFO> &resolutions)
{
  for(auto i = 0; i < m_drm_connector->count_modes; i++)
  {
    RESOLUTION_INFO res;
    res.iScreen = 0;
    res.iWidth = m_drm_connector->modes[i].hdisplay;
    res.iHeight = m_drm_connector->modes[i].vdisplay;
    res.iScreenWidth = m_drm_connector->modes[i].hdisplay;
    res.iScreenHeight = m_drm_connector->modes[i].vdisplay;
    res.fRefreshRate = m_drm_connector->modes[i].vrefresh;
    res.iSubtitles = static_cast<int>(0.965 * res.iHeight);
    res.fPixelRatio = 1.0f;
    res.bFullScreen = true;
    res.strMode = m_drm_connector->modes[i].name;
    res.strId = std::to_string(i);

    if(m_drm_connector->modes[i].flags & DRM_MODE_FLAG_3D_MASK)
    {
      if(m_drm_connector->modes[i].flags & DRM_MODE_FLAG_3D_TOP_AND_BOTTOM)
      {
        res.dwFlags = D3DPRESENTFLAG_MODE3DTB;
      }
      else if(m_drm_connector->modes[i].flags
          & DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF)
      {
        res.dwFlags = D3DPRESENTFLAG_MODE3DSBS;
      }
    }
    else if(m_drm_connector->modes[i].flags & DRM_MODE_FLAG_INTERLACE)
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

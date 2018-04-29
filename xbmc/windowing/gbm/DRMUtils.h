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

#pragma once

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <vector>

#include "windowing/Resolution.h"
#include "GBMUtils.h"

struct plane
{
  drmModePlane *plane = nullptr;
  drmModeObjectProperties *props = nullptr;
  drmModePropertyRes **props_info = nullptr;
  uint32_t format;
};

struct connector
{
  drmModeConnector *connector = nullptr;
  drmModeObjectProperties *props = nullptr;
  drmModePropertyRes **props_info = nullptr;
};

struct encoder
{
  drmModeEncoder *encoder = nullptr;
};

struct crtc
{
  drmModeCrtc *crtc = nullptr;
  drmModeObjectProperties *props = nullptr;
  drmModePropertyRes **props_info = nullptr;
};

struct drm_fb
{
  struct gbm_bo *bo = nullptr;
  uint32_t fb_id;
};

class CDRMUtils
{
public:
  CDRMUtils();
  virtual ~CDRMUtils() = default;
  virtual void FlipPage(struct gbm_bo *bo, bool rendered, bool videoLayer) {};
  virtual bool SetVideoMode(RESOLUTION_INFO res, struct gbm_bo *bo) { return false; };
  virtual bool SetActive(bool active) { return false; };
  virtual bool InitDrm();
  virtual void DestroyDrm();

  bool GetModes(std::vector<RESOLUTION_INFO> &resolutions);
  bool SetMode(RESOLUTION_INFO res);
  void WaitVBlank();

  int m_fd;

  struct connector *m_connector = nullptr;
  struct encoder *m_encoder = nullptr;
  struct crtc *m_crtc = nullptr;
  struct plane *m_primary_plane = nullptr;
  struct plane *m_overlay_plane = nullptr;
  drmModeModeInfo *m_mode = nullptr;
  drmModeAtomicReq *m_req = nullptr;

protected:
  bool OpenDrm();
  drm_fb * DrmFbGetFromBo(struct gbm_bo *bo);

private:
  bool GetResources();
  bool GetConnector();
  bool GetEncoder();
  bool GetCrtc();
  bool GetPlanes();
  bool GetPreferredMode();
  bool RestoreOriginalMode();
  static void DrmFbDestroyCallback(struct gbm_bo *bo, void *data);

  int m_crtc_index;

  drmModeResPtr m_drm_resources = nullptr;
  drmModeCrtcPtr m_orig_crtc = nullptr;
};

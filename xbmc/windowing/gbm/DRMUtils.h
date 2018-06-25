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

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <vector>

#include "windowing/Resolution.h"
#include "GBMUtils.h"

enum EPLANETYPE
{
  VIDEO_PLANE,
  GUI_PLANE
};

struct drm_object
{
  uint32_t id = 0;
  uint32_t type = 0;
  drmModeObjectPropertiesPtr props = nullptr;
  drmModePropertyRes **props_info = nullptr;
};

struct plane : drm_object
{
  drmModePlanePtr plane = nullptr;
  uint32_t format = DRM_FORMAT_XRGB8888;
};

struct connector : drm_object
{
  drmModeConnectorPtr connector = nullptr;
};

struct encoder
{
  drmModeEncoder *encoder = nullptr;
};

struct crtc : drm_object
{
  drmModeCrtcPtr crtc = nullptr;
};

struct drm_fb
{
  struct gbm_bo *bo = nullptr;
  uint32_t fb_id;
  uint32_t format;
};

class CDRMUtils
{
public:
  CDRMUtils();
  virtual ~CDRMUtils() = default;
  virtual void FlipPage(struct gbm_bo *bo, bool rendered, bool videoLayer) {};
  virtual bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo *bo) { return false; };
  virtual bool SetActive(bool active) { return false; };
  virtual bool InitDrm();
  virtual void DestroyDrm();

  std::string GetModule() const { return m_module; }
  std::string GetDevicePath() const { return m_device_path; }
  int GetFileDescriptor() const { return m_fd; }
  struct plane* GetPrimaryPlane() const { return m_primary_plane; }
  struct plane* GetOverlayPlane() const { return m_overlay_plane; }
  struct crtc* GetCrtc() const { return m_crtc; }

  RESOLUTION_INFO GetCurrentMode();
  std::vector<RESOLUTION_INFO> GetModes();
  bool SetMode(const RESOLUTION_INFO& res);
  void WaitVBlank();

  virtual bool AddProperty(struct drm_object *object, const char *name, uint64_t value) { return false; }
  virtual bool SetProperty(struct drm_object *object, const char *name, uint64_t value) { return false; }

protected:
  bool OpenDrm();
  uint32_t GetPropertyId(struct drm_object *object, const char *name);
  drm_fb* DrmFbGetFromBo(struct gbm_bo *bo);

  int m_fd;
  struct connector *m_connector = nullptr;
  struct encoder *m_encoder = nullptr;
  struct crtc *m_crtc = nullptr;
  struct plane *m_primary_plane = nullptr;
  struct plane *m_overlay_plane = nullptr;
  drmModeModeInfo *m_mode = nullptr;

  int m_width = 0;
  int m_height = 0;

private:
  static bool SupportsFormat(drmModePlanePtr plane, uint32_t format);
  drmModePlanePtr FindPlane(drmModePlaneResPtr resources, int crtc_index, int type);

  bool GetResources();
  bool FindConnector();
  bool FindEncoder();
  bool FindCrtc();
  bool FindPlanes();
  bool FindPreferredMode();
  bool RestoreOriginalMode();
  static void DrmFbDestroyCallback(struct gbm_bo *bo, void *data);
  RESOLUTION_INFO GetResolutionInfo(drmModeModeInfoPtr mode);

  int m_crtc_index;
  std::string m_module;
  std::string m_device_path;

  drmModeResPtr m_drm_resources = nullptr;
  drmModeCrtcPtr m_orig_crtc = nullptr;
};

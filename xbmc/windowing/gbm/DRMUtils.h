/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
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

#pragma once

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <vector>

#include "windowing/Resolution.h"
#include "GBMUtils.h"

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
  uint32_t format;
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

  std::string GetModule() const { return m_module; }
  std::string GetDevicePath() const { return m_device_path; }

  bool GetModes(std::vector<RESOLUTION_INFO> &resolutions);
  bool SetMode(RESOLUTION_INFO res);
  void WaitVBlank();

  bool AddProperty(drmModeAtomicReqPtr req, struct drm_object *object, const char *name, uint64_t value);
  bool SetProperty(struct drm_object *object, const char *name, uint64_t value);

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
  std::string m_module;
  std::string m_device_path;

  drmModeResPtr m_drm_resources = nullptr;
  drmModeCrtcPtr m_orig_crtc = nullptr;
};

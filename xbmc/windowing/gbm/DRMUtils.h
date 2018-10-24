/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <drm_fourcc.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <gbm.h>
#include <map>
#include <vector>

#include "windowing/Resolution.h"
#include "GBMUtils.h"
#include "platform/posix/utils/FileHandle.h"

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

enum EPLANETYPE
{
  KODI_VIDEO_PLANE,
  KODI_GUI_PLANE,
  KODI_GUI_10_PLANE
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
  uint32_t format{0};
  uint32_t fallbackFormat{0};
  bool useFallbackFormat{false};

  uint32_t GetFormat()
  {
    if (useFallbackFormat)
      return fallbackFormat;

    return format;
  }

  std::map<uint32_t, std::vector<uint64_t>> modifiers_map;
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
  std::vector<uint64_t> *GetPrimaryPlaneModifiersForFormat(uint32_t format) { return &m_primary_plane->modifiers_map[format]; }
  std::vector<uint64_t> *GetOverlayPlaneModifiersForFormat(uint32_t format) { return &m_overlay_plane->modifiers_map[format]; }
  struct crtc* GetCrtc() const { return m_crtc; }

  virtual RESOLUTION_INFO GetCurrentMode();
  virtual std::vector<RESOLUTION_INFO> GetModes();
  virtual bool SetMode(const RESOLUTION_INFO& res);

  bool SupportsProperty(struct drm_object *object, const char *name);
  virtual bool AddProperty(struct drm_object *object, const char *name, uint64_t value) { return false; }
  virtual bool SetProperty(struct drm_object *object, const char *name, uint64_t value) { return false; }

  static uint32_t FourCCWithAlpha(uint32_t fourcc);
  static uint32_t FourCCWithoutAlpha(uint32_t fourcc);

protected:
  bool OpenDrm(bool needConnector);
  uint32_t GetPropertyId(struct drm_object *object, const char *name);
  drm_fb* DrmFbGetFromBo(struct gbm_bo *bo);
  static bool GetProperties(int fd, uint32_t id, uint32_t type, struct drm_object *object);
  static void FreeProperties(struct drm_object *object);

  KODI::UTILS::POSIX::CFileHandle m_fd;
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
  bool FindModifiersForPlane(struct plane *object);
  bool FindPreferredMode();
  bool RestoreOriginalMode();
  static void DrmFbDestroyCallback(struct gbm_bo *bo, void *data);
  RESOLUTION_INFO GetResolutionInfo(drmModeModeInfoPtr mode);
  bool CheckConnector(int connectorId);

  int m_crtc_index;
  std::string m_module;
  std::string m_device_path;

  drmModeResPtr m_drm_resources = nullptr;
  drmModeCrtcPtr m_orig_crtc = nullptr;
};

}
}
}

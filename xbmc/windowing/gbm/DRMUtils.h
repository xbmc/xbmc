/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GBMUtils.h"
#include "windowing/Resolution.h"

#include "platform/posix/utils/FileHandle.h"

#include <map>
#include <vector>

#include <drm_fourcc.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

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
  bool useFallbackFormat{false};
  std::map<uint32_t, std::vector<uint64_t>> modifiers_map;

  void SetFormat(uint32_t newFormat) { format = newFormat; }
  uint32_t GetFormat() { return format; }

private:
  uint32_t format{DRM_FORMAT_XRGB8888};
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
  int GetFileDescriptor() const { return m_fd; }
  int GetRenderNodeFileDescriptor() const { return m_renderFd; }
  struct plane* GetVideoPlane() const { return m_video_plane; }
  struct plane* GetGuiPlane() const { return m_gui_plane; }
  std::vector<uint64_t> *GetVideoPlaneModifiersForFormat(uint32_t format) { return &m_video_plane->modifiers_map[format]; }
  std::vector<uint64_t> *GetGuiPlaneModifiersForFormat(uint32_t format) { return &m_gui_plane->modifiers_map[format]; }
  struct crtc* GetCrtc() const { return m_crtc; }
  struct connector* GetConnector() const { return m_connector; }

  virtual RESOLUTION_INFO GetCurrentMode();
  virtual std::vector<RESOLUTION_INFO> GetModes();
  virtual bool SetMode(const RESOLUTION_INFO& res);

  bool SupportsProperty(struct drm_object *object, const char *name);
  bool SupportsPropertyAndValue(struct drm_object* object, const char* name, uint64_t value);
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
  struct crtc* m_orig_crtc = nullptr;
  struct plane *m_video_plane = nullptr;
  struct plane *m_gui_plane = nullptr;
  drmModeModeInfo *m_mode = nullptr;

  int m_width = 0;
  int m_height = 0;

private:
  static bool SupportsFormat(drmModePlanePtr plane, uint32_t format);
  drmModePlanePtr FindPlane(drmModePlaneResPtr resources, int crtc_index, int type);

  bool GetResources();
  bool FindConnector();
  bool FindEncoder();
  bool FindCrtcs();
  bool FindPlanes();
  bool FindModifiersForPlane(struct plane *object);
  bool FindPreferredMode();
  bool RestoreOriginalMode();
  static void DrmFbDestroyCallback(struct gbm_bo *bo, void *data);
  RESOLUTION_INFO GetResolutionInfo(drmModeModeInfoPtr mode);
  bool CheckConnector(int connectorId);

  KODI::UTILS::POSIX::CFileHandle m_renderFd;
  std::string m_module;

  drmModeResPtr m_drm_resources = nullptr;

  std::vector<crtc*> m_crtcs;
};

}
}
}

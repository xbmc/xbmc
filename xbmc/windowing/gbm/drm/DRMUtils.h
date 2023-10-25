/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DRMConnector.h"
#include "DRMCrtc.h"
#include "DRMEncoder.h"
#include "DRMPlane.h"
#include "windowing/Resolution.h"
#include "windowing/gbm/GBMUtils.h"

#include <utility>
#include <vector>

#include <gbm.h>
#include <xf86drm.h>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

struct drm_fb
{
  struct gbm_bo *bo = nullptr;
  uint32_t fb_id;
  uint32_t format;
};

class CDRMUtils
{
public:
  CDRMUtils() = default;
  virtual ~CDRMUtils();
  virtual void FlipPage(struct gbm_bo* bo, bool rendered, bool videoLayer, bool async) {}
  virtual bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo* bo) { return false; }
  virtual bool SetActive(bool active) { return false; }
  virtual bool InitDrm();
  virtual void DestroyDrm();

  int GetFileDescriptor() const { return m_fd; }
  int GetRenderNodeFileDescriptor() const { return m_renderFd; }
  const char* GetRenderDevicePath() const { return m_renderDevicePath; }
  CDRMPlane* GetVideoPlane() const { return m_video_plane; }
  CDRMPlane* GetGuiPlane() const { return m_gui_plane; }
  CDRMCrtc* GetCrtc() const { return m_crtc; }
  CDRMConnector* GetConnector() const { return m_connector; }

  std::vector<std::string> GetConnectedConnectorNames();

  virtual RESOLUTION_INFO GetCurrentMode();
  virtual std::vector<RESOLUTION_INFO> GetModes();
  virtual bool SetMode(const RESOLUTION_INFO& res);

  static uint32_t FourCCWithAlpha(uint32_t fourcc);
  static uint32_t FourCCWithoutAlpha(uint32_t fourcc);

  void SetInFenceFd(int fd) { m_inFenceFd = fd; }
  int TakeOutFenceFd()
  {
    int fd{-1};
    return std::exchange(m_outFenceFd, fd);
  }

protected:
  bool OpenDrm(bool needConnector);
  drm_fb* DrmFbGetFromBo(struct gbm_bo *bo);

  int m_fd;
  CDRMConnector* m_connector{nullptr};
  CDRMEncoder* m_encoder{nullptr};
  CDRMCrtc* m_crtc{nullptr};
  CDRMCrtc* m_orig_crtc{nullptr};
  CDRMPlane* m_video_plane{nullptr};
  CDRMPlane* m_gui_plane{nullptr};
  drmModeModeInfo *m_mode = nullptr;

  int m_width = 0;
  int m_height = 0;

  int m_inFenceFd{-1};
  int m_outFenceFd{-1};

  std::vector<std::unique_ptr<CDRMPlane>> m_planes;

private:
  bool FindConnector();
  bool FindEncoder();
  bool FindCrtc();
  bool FindPlanes();
  bool FindPreferredMode();
  bool RestoreOriginalMode();
  RESOLUTION_INFO GetResolutionInfo(drmModeModeInfoPtr mode);
  void PrintDrmDeviceInfo(drmDevicePtr device);

  int m_renderFd;
  const char* m_renderDevicePath{nullptr};

  std::vector<std::unique_ptr<CDRMConnector>> m_connectors;
  std::vector<std::unique_ptr<CDRMEncoder>> m_encoders;
  std::vector<std::unique_ptr<CDRMCrtc>> m_crtcs;
};

}
}
}

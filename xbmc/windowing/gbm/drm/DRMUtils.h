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

constexpr const char* SETTING_VIDEOSCREEN_LIMITGUISIZE = "videoscreen.limitguisize";
constexpr const char* SETTING_VIDEOSCREEN_COLORRANGE = "videoscreen.colorrange";
constexpr const char* SETTING_VIDEOSCREEN_COLORFORMAT = "videoscreen.colorformat";
constexpr const char* SETTING_VIDEOSCREEN_MAXBPC = "videoscreen.maxbpc";

// https://github.com/torvalds/linux/blob/7d0a66e4bb9081d75c82ec4957c50034cb0ea449/drivers/
// gpu/drm/amd/display/amdgpu_dm/amdgpu_dm_crtc.c#L683-L692
//
// amdgpu drm driver needs at least 1 primary plane to active when doing a modeset
inline constexpr int QUIRK_NEEDSPRIMARY = 1 << 0;

struct drm_fb
{
  struct gbm_bo* bo = nullptr;
  uint32_t fb_id;
  uint32_t format;
};

struct outputformat
{
  uint32_t drm;
  uint8_t bpp;
  uint8_t alpha;
  bool active;
  std::vector<uint64_t> modifiers;
};

class CDRMUtils
{
public:
  CDRMUtils() = default;
  virtual ~CDRMUtils();
  virtual void FlipPage(struct gbm_bo* bo, bool rendered, bool videoLayer, bool async) {}
  virtual bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo* bo) { return false; }
  virtual bool SetActive(bool active) { return false; }
  virtual void RequestModeset() {}
  //! True if the last commit was refused and the previous request was committed instead
  virtual bool LastCommitFellBack() const { return false; }
  //! True if the last commit failed outright, so nothing was committed and the requests it carried
  //! (properties and modeset) are gone. Apart from a fallback, which commits the previous request.
  virtual bool LastCommitFailed() const { return false; }
  //! False if the last flip did not get as far as a commit (no mode, no framebuffer, a request that
  //! could not be built), so it says nothing about the properties it was to carry
  virtual bool LastFlipSubmitted() const { return true; }
  //! True if the last flip took the modeset request, which is gone unless the commit went through
  virtual bool LastFlipCarriedModeset() const { return false; }
  virtual bool InitDrm();
  virtual void DestroyDrm();
  virtual bool SupportsFencing() { return false; }

  int GetFileDescriptor() const { return m_fd; }
  int GetRenderNodeFileDescriptor() const { return m_renderFd; }
  const char* GetRenderDevicePath() const { return m_renderDevicePath; }
  CDRMPlane* GetVideoPlane() const { return m_video_plane; }
  CDRMPlane* GetGuiPlane() const { return m_gui_plane; }
  CDRMCrtc* GetCrtc() const { return m_crtc; }
  CDRMConnector* GetConnector() const { return m_connector; }
  //! Pixel clock of the active mode in kHz, 0 if there is none (m_mode is null then, e.g. after a
  //! mode change that failed because the connector was disconnected)
  uint32_t GetModeClock() const { return m_mode ? m_mode->clock : 0; }
  std::vector<outputformat>& GetOutputFormats() { return m_output_formats; }
  bool FindGuiPlane(uint32_t format, uint64_t modifier);
  bool FindVideoPlane(uint32_t format, uint64_t modifier);
  bool FindVideoAndGuiPlane(uint32_t format, uint64_t modifier, uint64_t width, uint64_t height);
  // FindGuiPlane never releases a D2P video plane reservation; a caller that
  // means "video is over" states it here before revalidating the gui plane.
  void ReleaseVideoPlane() { m_video_plane = nullptr; }
  bool HasQuirk(int quirk) const { return m_drm_quirks & quirk; }

  std::vector<std::string> GetConnectedConnectorNames();

  virtual RESOLUTION_INFO GetCurrentMode();
  //! False when there is no current mode (a mode change failed on a disconnected connector)
  virtual bool HasCurrentMode() const { return m_mode != nullptr; }
  virtual std::vector<RESOLUTION_INFO> GetModes();
  virtual bool SetMode(const RESOLUTION_INFO& res);

  void SetInFenceFd(int fd) { m_inFenceFd = fd; }
  int TakeOutFenceFd()
  {
    int fd{-1};
    return std::exchange(m_outFenceFd, fd);
  }

protected:
  bool OpenDrm(bool needConnector);
  drm_fb* DrmFbGetFromBo(struct gbm_bo* bo);

  int m_fd{-1};
  CDRMConnector* m_connector{nullptr};
  CDRMEncoder* m_encoder{nullptr};
  CDRMCrtc* m_crtc{nullptr};
  CDRMCrtc* m_old_crtc{nullptr};
  CDRMCrtc* m_orig_crtc{nullptr};
  CDRMPlane* m_video_plane{nullptr};
  CDRMPlane* m_gui_plane{nullptr};
  //! The active mode, in the connector's modes or the original crtc. It is null when there is none,
  //! and must not be kept when the connector is reloaded, as that frees the modes.
  drmModeModeInfo* m_mode = nullptr;

  int m_width = 0;
  int m_height = 0;

  int m_inFenceFd{-1};
  int m_outFenceFd{-1};

  std::vector<std::unique_ptr<CDRMPlane>> m_planes;

private:
  bool FindConnector();
  bool FindEncoder();
  bool FindCrtc();
  bool FindPreferredMode();
  bool RestoreOriginalMode();
  RESOLUTION_INFO GetResolutionInfo(drmModeModeInfoPtr mode);
  void PrintDrmDeviceInfo(drmDevicePtr device);

  int m_renderFd{-1};
  const char* m_renderDevicePath{nullptr};

  int m_drm_quirks{0};
  std::vector<std::unique_ptr<CDRMConnector>> m_connectors;
  std::vector<std::unique_ptr<CDRMEncoder>> m_encoders;
  std::vector<std::unique_ptr<CDRMCrtc>> m_crtcs;
  // 8-bit formats first: Direct-to-Plane needs full 8-bit alpha for GUI overlay.
  // 10-bit formats are available for single-plane HDR rendering when requested.
  std::vector<outputformat> m_output_formats = {{DRM_FORMAT_ARGB8888, 8, 8, false, {}},
                                                {DRM_FORMAT_XRGB8888, 8, 0, false, {}},
                                                {DRM_FORMAT_ARGB2101010, 10, 2, false, {}},
                                                {DRM_FORMAT_XRGB2101010, 10, 0, false, {}}};
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI

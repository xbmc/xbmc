/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "VideoLayerBridge.h"
#include "drm/DRMObject.h"
#include "drm/DRMUtils.h"
#include "messaging/ApplicationMessenger.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "threads/SystemClock.h"
#include "utils/DisplayInfo.h"
#include "windowing/WinSystem.h"

#include "platform/linux/input/LibInputHandler.h"

#include <atomic>
#include <utility>

#include <gbm.h>

class IDispResource;

namespace KODI
{
namespace UTILS
{
class CDisplayInfo;
}
namespace WINDOWING
{
namespace GBM
{

enum class ColorRange
{
  AUTO = 0, //!< leave it to the graphics driver
  FULL = 1, //!< output full range (0-255) and signal it
  LIMITED = 2, //!< output limited range (16-235) and signal it
};

class CWinSystemGbm : public CWinSystemBase, public ISettingCallback
{
public:
  CWinSystemGbm();
  ~CWinSystemGbm() override;

  const std::string GetName() override;

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;

  void FlipPage(bool rendered, bool videoLayer, bool async);

  bool CanDoWindowed() override { return false; }
  void UpdateResolutions() override;

  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  bool UseLimitedColor() override;

  bool Hide() override;
  bool Show(bool raise = true) override;
  void Register(IDispResource* resource) override;
  void Unregister(IDispResource* resource) override;

  bool SetVideoOutput(const VideoPicture* videoPicture) override;

  void SetColorimetry(const VideoPicture* videoPicture) override;
  KODI::UTILS::Colorimetry GetColorimetry() const override { return m_colorimetry; }
  KODI::UTILS::Eotf GetEotf() const override { return m_eotf; }
  bool SetHDR(const VideoPicture* videoPicture) override;
  bool IsHDRDisplay() override;
  CHDRCapabilities GetDisplayHDRCapabilities() const override;

  // holding the returned reference defers video plane teardown until it is released
  std::shared_ptr<CVideoLayerBridge> GetVideoLayerBridge() const { return m_videoLayerBridge; }
  void RegisterVideoLayerBridge(std::shared_ptr<CVideoLayerBridge> bridge)
  {
    m_videoLayerBridge = std::move(bridge);
  };

  CGBMUtils::CGBMDevice& GetGBMDevice() const { return m_GBM->GetDevice(); }
  std::shared_ptr<CDRMUtils> GetDrm() const { return m_DRM; }

  std::vector<std::string> GetConnectedOutputs() override;

protected:
  void OnLostDevice();
  void OnResetDisplay();

  std::unique_ptr<CVideoSync> GetVideoSync(CVideoReferenceClock* clock) override;

  std::shared_ptr<CDRMUtils> m_DRM;
  std::unique_ptr<CGBMUtils> m_GBM;
  std::shared_ptr<CVideoLayerBridge> m_videoLayerBridge;

  CCriticalSection m_resourceSection;
  std::vector<IDispResource*> m_resources;

  bool m_dispReset = false;
  XbmcThreads::EndTime<> m_dispResetTimer;
  std::unique_ptr<CLibInputHandler> m_libinput;

private:
  static void ApplyLimitGuiSize(void* userptr);
  static void ApplyColorRangeSetting(void* userptr);

  KODI::MESSAGING::ThreadMessageCallback m_limitGuiSizeCallback{ApplyLimitGuiSize, this};
  KODI::MESSAGING::ThreadMessageCallback m_colorRangeCallback{ApplyColorRangeSetting, this};

  void ApplyBroadcastRange(bool retry = false);
  void RetryBroadcastRange();

  ColorRange GetColorRange() const;

  void MigrateLimitedRangeSetting();

  void UpdateDriverLimitedRange();

  std::atomic<int> m_colorRange{static_cast<int>(ColorRange::AUTO)};

  //! True while "Broadcast RGB" is committed as "Limited 16:235"
  std::atomic<bool> m_driverLimitedRange{false};

  bool m_migratingColorRange{false};

  bool m_broadcastRangeQueued{false};
  uint64_t m_broadcastRangeTarget{0};
  int m_broadcastRangeRetries{0};

  CDRMPropertyBlob m_hdrBlob;
  KODI::UTILS::Eotf m_eotf = KODI::UTILS::Eotf::TRADITIONAL_SDR;
  KODI::UTILS::Colorimetry m_colorimetry = KODI::UTILS::Colorimetry::DEFAULT;

  std::unique_ptr<UTILS::CDisplayInfo> m_info;
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI

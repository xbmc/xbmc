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
#include <optional>
#include <string_view>
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

enum class ColorFormat
{
  AUTO = 0, //!< leave it to the graphics driver
  RGB = 1,
  YUV444 = 2,
  YUV422 = 3,
  YUV420 = 4,
  ADAPTIVE = 5, //!< Auto, with YCbCr for >8 bit video when RGB does not fit the mode
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
  static void ApplyColorFormatSetting(void* userptr);
  static void ApplyMaxBpcSetting(void* userptr);

  KODI::MESSAGING::ThreadMessageCallback m_limitGuiSizeCallback{ApplyLimitGuiSize, this};
  KODI::MESSAGING::ThreadMessageCallback m_colorRangeCallback{ApplyColorRangeSetting, this};
  KODI::MESSAGING::ThreadMessageCallback m_colorFormatCallback{ApplyColorFormatSetting, this};
  KODI::MESSAGING::ThreadMessageCallback m_maxBpcCallback{ApplyMaxBpcSetting, this};

  struct PropertyRequest
  {
    const char* property{nullptr};
    bool queued{false};
    uint64_t target{0};
  };

  void ApplyConnectorProperties(bool retry = false);
  void RetryConnectorProperties();
  void RequeueConnectorProperties();
  void ApplyBroadcastRange();
  void ApplyColorFormat();
  void ApplyMaxBpc();
  void ApplyColorspace();

  bool QueueConnectorProperty(PropertyRequest& request,
                              const char* property,
                              const char* valueName);
  bool QueueConnectorProperty(PropertyRequest& request,
                              const char* property,
                              uint64_t target,
                              std::string_view label);
  //! True if the request's commit was refused. Clears its queued flag
  bool IsRequestRejected(PropertyRequest& request);

  //! True if the connector offers the format, or if that is not known
  bool IsColorFormatOffered(ColorFormat format) const;

  ColorRange GetColorRange() const;
  ColorFormat GetColorFormat() const;
  //! The format to ask the driver for: the choice or Adaptive's pick, if listed and not refused
  ColorFormat GetRequestedColorFormat() const;
  //! The "max bpc" in effect: the choice, or what the connector has. 0 if unknown
  uint64_t GetEffectiveMaxBpc() const;
  //! True if the HDMI link can be driven with FRL, which carries 4K 60 Hz 10 bit RGB
  bool IsHdmiFrlLink() const;
  //! True if >8 bit video in the active HDMI mode does not fit as RGB, but does as YCbCr
  bool ContentNeedsSubsampling() const;

  bool IsYuvRequested() const;
  bool IsYuvFormatCommitted(const CDRMConnector& connector) const;

  void MigrateLimitedRangeSetting();

  void UpdateDriverLimitedRange();

  std::atomic<int> m_colorRange{static_cast<int>(ColorRange::AUTO)};

  //! True while "Broadcast RGB" is committed as "Limited 16:235"
  std::atomic<bool> m_driverLimitedRange{false};

  bool m_migratingColorRange{false};

  PropertyRequest m_rangeRequest;
  PropertyRequest m_formatRequest;
  PropertyRequest m_bpcRequest;
  int m_connectorRetries{0};

  std::atomic<int> m_colorFormat{static_cast<int>(ColorFormat::AUTO)};

  //! Index into the "Maximum bit depth" options, 0 is Auto
  std::atomic<int> m_maxBpc{0};
  //! The driver refused the chosen "max bpc", so the default is used
  bool m_maxBpcFallback{false};
  //! The mode that refused the chosen depth, the fallback only holds there
  std::string m_maxBpcFailedMode;
  //! The default depth is being tried, to see whether the chosen one was refused
  bool m_bpcProbe{false};
  //! The "max bpc" the connector had before Kodi wrote to it
  std::optional<uint64_t> m_defaultMaxBpc;

  //! Bits per component of the video that is playing, 0 when there is none
  std::atomic<int> m_contentBits{0};

  //! How many formats the driver refused for m_colorFormatFailedMode, they are skipped there
  int m_colorFormatSkip{0};
  std::string m_colorFormatFailedMode;
  std::string m_currentModeId;

  //! Whether the "Colorspace" tag was last set for a YCbCr output
  bool m_scanoutYuv{false};

  CDRMPropertyBlob m_hdrBlob;
  KODI::UTILS::Eotf m_eotf = KODI::UTILS::Eotf::TRADITIONAL_SDR;
  KODI::UTILS::Colorimetry m_colorimetry = KODI::UTILS::Colorimetry::DEFAULT;

  std::unique_ptr<UTILS::CDisplayInfo> m_info;
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI

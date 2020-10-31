/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/ISubSettings.h"
#include "settings/lib/ISettingCallback.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"
#include "windowing/Resolution.h"

#include <map>
#include <set>
#include <utility>
#include <vector>

class TiXmlNode;
struct IntegerSettingOption;
struct StringSettingOption;

class CDisplaySettings : public ISettingCallback, public ISubSettings,
                         public Observable
{
public:
  static CDisplaySettings& GetInstance();

  bool Load(const TiXmlNode *settings) override;
  bool Save(TiXmlNode *settings) const override;
  void Clear() override;

  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;
  bool OnSettingChanging(const std::shared_ptr<const CSetting>& setting) override;
  bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode) override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  /*!
   \brief Returns the currently active resolution

   This resolution might differ from the display resolution which is based on
   the user's settings.

   \sa SetCurrentResolution
   \sa GetResolutionInfo
   \sa GetDisplayResolution
   */
  RESOLUTION GetCurrentResolution() const { return m_currentResolution; }
  void SetCurrentResolution(RESOLUTION resolution, bool save = false);
  /*!
   \brief Returns the best-matching resolution of the videoscreen.screenmode setting value

   This resolution might differ from the current resolution which is based on
   the properties of the operating system and the attached displays.

   \sa GetCurrentResolution
   */
  RESOLUTION GetDisplayResolution() const;

  const RESOLUTION_INFO& GetResolutionInfo(size_t index) const;
  const RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution) const;
  RESOLUTION_INFO& GetResolutionInfo(size_t index);
  RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution);
  size_t ResolutionInfoSize() const { return m_resolutions.size(); }
  void AddResolutionInfo(const RESOLUTION_INFO &resolution);

  const RESOLUTION_INFO& GetCurrentResolutionInfo() const { return GetResolutionInfo(m_currentResolution); }
  RESOLUTION_INFO& GetCurrentResolutionInfo() { return GetResolutionInfo(m_currentResolution); }
  RESOLUTION GetResFromString(const std::string &strResolution) { return GetResolutionFromString(strResolution); }
  std::string GetStringFromRes(const RESOLUTION resolution, float refreshrate = 0.0f) { return GetStringFromResolution(resolution, refreshrate); }

  void ApplyCalibrations();
  void UpdateCalibrations();
  void ClearCalibrations();
  void ClearCustomResolutions();

  float GetZoomAmount() const { return m_zoomAmount; }
  void SetZoomAmount(float zoomAmount) { m_zoomAmount = zoomAmount; }
  float GetPixelRatio() const { return m_pixelRatio; }
  void SetPixelRatio(float pixelRatio) { m_pixelRatio = pixelRatio; }
  float GetVerticalShift() const { return m_verticalShift; }
  void SetVerticalShift(float verticalShift) { m_verticalShift = verticalShift; }
  bool IsNonLinearStretched() const { return m_nonLinearStretched; }
  void SetNonLinearStretched(bool nonLinearStretch) { m_nonLinearStretched = nonLinearStretch; }
  void SetMonitor(const std::string& monitor);

  static void SettingOptionsModesFiller(const std::shared_ptr<const CSetting>& setting,
                                        std::vector<StringSettingOption>& list,
                                        std::string& current,
                                        void* data);
  static void SettingOptionsRefreshChangeDelaysFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<IntegerSettingOption>& list,
      int& current,
      void* data);
  static void SettingOptionsRefreshRatesFiller(const std::shared_ptr<const CSetting>& setting,
                                               std::vector<StringSettingOption>& list,
                                               std::string& current,
                                               void* data);
  static void SettingOptionsResolutionsFiller(const std::shared_ptr<const CSetting>& setting,
                                              std::vector<IntegerSettingOption>& list,
                                              int& current,
                                              void* data);
  static void SettingOptionsDispModeFiller(const std::shared_ptr<const CSetting>& setting,
                                           std::vector<IntegerSettingOption>& list,
                                           int& current,
                                           void* data);
  static void SettingOptionsStereoscopicModesFiller(const std::shared_ptr<const CSetting>& setting,
                                                    std::vector<IntegerSettingOption>& list,
                                                    int& current,
                                                    void* data);
  static void SettingOptionsPreferredStereoscopicViewModesFiller(
      const std::shared_ptr<const CSetting>& setting,
      std::vector<IntegerSettingOption>& list,
      int& current,
      void* data);
  static void SettingOptionsMonitorsFiller(const std::shared_ptr<const CSetting>& setting,
                                           std::vector<StringSettingOption>& list,
                                           std::string& current,
                                           void* data);
  static void SettingOptionsCmsModesFiller(const std::shared_ptr<const CSetting>& setting,
                                           std::vector<IntegerSettingOption>& list,
                                           int& current,
                                           void* data);
  static void SettingOptionsCmsWhitepointsFiller(const std::shared_ptr<const CSetting>& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data);
  static void SettingOptionsCmsPrimariesFiller(const std::shared_ptr<const CSetting>& setting,
                                               std::vector<IntegerSettingOption>& list,
                                               int& current,
                                               void* data);
  static void SettingOptionsCmsGammaModesFiller(const std::shared_ptr<const CSetting>& setting,
                                                std::vector<IntegerSettingOption>& list,
                                                int& current,
                                                void* data);


protected:
  CDisplaySettings();
  CDisplaySettings(const CDisplaySettings&) = delete;
  CDisplaySettings& operator=(CDisplaySettings const&) = delete;
  ~CDisplaySettings() override;

  DisplayMode GetCurrentDisplayMode() const;

  static RESOLUTION GetResolutionFromString(const std::string &strResolution);
  static std::string GetStringFromResolution(RESOLUTION resolution, float refreshrate = 0.0f);
  static RESOLUTION GetResolutionForScreen();

  static RESOLUTION FindBestMatchingResolution(const std::map<RESOLUTION, RESOLUTION_INFO> &resolutionInfos, int width, int height, float refreshrate, unsigned int flags);

private:
  // holds the real gui resolution
  RESOLUTION m_currentResolution;

  typedef std::vector<RESOLUTION_INFO> ResolutionInfos;
  ResolutionInfos m_resolutions;
  ResolutionInfos m_calibrations;

  float m_zoomAmount;         // current zoom amount
  float m_pixelRatio;         // current pixel ratio
  float m_verticalShift;      // current vertical shift
  bool  m_nonLinearStretched;   // current non-linear stretch

  bool m_resolutionChangeAborted;
  mutable CCriticalSection m_critical;
};

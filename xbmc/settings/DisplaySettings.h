/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "windowing/Resolution.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISubSettings.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

class TiXmlNode;

class CDisplaySettings : public ISettingCallback, public ISubSettings,
                         public Observable
{
public:
  static CDisplaySettings& GetInstance();

  bool Load(const TiXmlNode *settings) override;
  bool Save(TiXmlNode *settings) const override;
  void Clear() override;

  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;
  bool OnSettingChanging(std::shared_ptr<const CSetting> setting) override;
  bool OnSettingUpdate(std::shared_ptr<CSetting> setting, const char *oldSettingId, const TiXmlNode *oldSettingNode) override;

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
  void ClearCustomResolutions();

  float GetZoomAmount() const { return m_zoomAmount; }
  void SetZoomAmount(float zoomAmount) { m_zoomAmount = zoomAmount; }
  float GetPixelRatio() const { return m_pixelRatio; }
  void SetPixelRatio(float pixelRatio) { m_pixelRatio = pixelRatio; }
  float GetVerticalShift() const { return m_verticalShift; }
  void SetVerticalShift(float verticalShift) { m_verticalShift = verticalShift; }
  bool IsNonLinearStretched() const { return m_nonLinearStretched; }
  void SetNonLinearStretched(bool nonLinearStretch) { m_nonLinearStretched = nonLinearStretch; }
  void SetMonitor(std::string monitor);

  static void SettingOptionsModesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsRefreshChangeDelaysFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsRefreshRatesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsResolutionsFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsDispModeFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsStereoscopicModesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsPreferredStereoscopicViewModesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsMonitorsFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsCmsModesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsCmsWhitepointsFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsCmsPrimariesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsCmsGammaModesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);


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

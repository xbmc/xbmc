#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "guilib/Resolution.h"
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

  static void SettingOptionsRefreshChangeDelaysFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsRefreshRatesFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsResolutionsFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
  static void SettingOptionsScreensFiller(std::shared_ptr<const CSetting> setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);
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

  static RESOLUTION FindBestMatchingResolution(const std::map<RESOLUTION, RESOLUTION_INFO> &resolutionInfos, int screen, int width, int height, float refreshrate, unsigned int flags);

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
  CCriticalSection m_critical;
};

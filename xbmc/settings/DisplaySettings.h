#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://xbmc.org
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
  static CDisplaySettings& Get();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;
  virtual void Clear();

  virtual bool OnSettingChanging(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);

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

  static void SettingOptionsRefreshChangeDelaysFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);
  static void SettingOptionsRefreshRatesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current);
  static void SettingOptionsResolutionsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);
  static void SettingOptionsScreensFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);
  static void SettingOptionsVerticalSyncsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);
  static void SettingOptionsStereoscopicModesFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);
  static void SettingOptionsPreferredStereoscopicViewModesFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current);
  static void SettingOptionsMonitorsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current);

protected:
  CDisplaySettings();
  CDisplaySettings(const CDisplaySettings&);
  CDisplaySettings& operator=(CDisplaySettings const&);
  virtual ~CDisplaySettings();

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

#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
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

#include <vector>

#include "guilib/Resolution.h"
#include "settings/ISubSettings.h"
#include "threads/CriticalSection.h"

class TiXmlNode;

class CDisplaySettings : public ISubSettings
{
public:
  static CDisplaySettings& Get();

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;
  virtual void Clear();

  const RESOLUTION_INFO& GetResolutionInfo(size_t index) const;
  const RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution) const;
  RESOLUTION_INFO& GetResolutionInfo(size_t index);
  RESOLUTION_INFO& GetResolutionInfo(RESOLUTION resolution);
  size_t ResolutionInfoSize() const { return m_resolutions.size(); }
  void AddResolutionInfo(const RESOLUTION_INFO &resolution);

  void ApplyCalibrations();
  void UpdateCalibrations();

protected:
  CDisplaySettings();
  CDisplaySettings(const CDisplaySettings&);
  CDisplaySettings const& operator=(CDisplaySettings const&);
  virtual ~CDisplaySettings();

private:
  typedef std::vector<RESOLUTION_INFO> ResolutionInfos;
  ResolutionInfos m_resolutions;
  ResolutionInfos m_calibrations;
  CCriticalSection m_critical;
};

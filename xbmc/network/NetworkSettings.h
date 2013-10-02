#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "system.h"
#include <string>
#include <vector>

#include "settings/ISettingCallback.h"
#include "utils/Job.h"

class CIPConfig;

class CNetworkSettings : public ISettingCallback, public IJobCallback
{
public:
  static CNetworkSettings& Get();

  // ISettingCallback
  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction( const CSetting *setting);

  // IJobCallback
  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

  void FillInNetworkConnection();

private:
  CNetworkSettings();
  CNetworkSettings(const CNetworkSettings&);
  CNetworkSettings const& operator=(CNetworkSettings const&);
  virtual ~CNetworkSettings();

  void ApplyNetworkConnection(const std::string connection_name, const CIPConfig &ipconfig);

  bool m_doing_connection;
};

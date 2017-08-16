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
#pragma once
#include <string>

#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"

class CUPnPSettings : public ISettingsHandler
{
public:
  static CUPnPSettings& GetInstance();
  
  void OnSettingsUnloaded() override;

  bool Load(const std::string &file);
  bool Save(const std::string &file) const;
  void Clear();

  const std::string& GetServerUUID() const { return m_serverUUID; }
  int GetServerPort() const { return m_serverPort; }
  int GetMaximumReturnedItems() const { return m_maxReturnedItems; }
  const std::string& GetRendererUUID() const { return m_rendererUUID; }
  int GetRendererPort() const { return m_rendererPort; }

  void SetServerUUID(const std::string &uuid) { m_serverUUID = uuid; }
  void SetServerPort(int port) { m_serverPort = port; }
  void SetMaximumReturnedItems(int maximumReturnedItems) { m_maxReturnedItems = maximumReturnedItems; }
  void SetRendererUUID(const std::string &uuid) { m_rendererUUID = uuid; }
  void SetRendererPort(int port) { m_rendererPort = port; }

protected:
  CUPnPSettings();
  CUPnPSettings(const CUPnPSettings&) = delete;
  CUPnPSettings& operator=(CUPnPSettings const&) = delete;
  ~CUPnPSettings() override;

private:
  std::string m_serverUUID;
  int m_serverPort;
  int m_maxReturnedItems;
  std::string m_rendererUUID;
  int m_rendererPort;

  CCriticalSection m_critical;
};

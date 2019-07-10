/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "settings/lib/ISettingsHandler.h"
#include "threads/CriticalSection.h"

#include <string>

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

  mutable CCriticalSection m_critical;
};

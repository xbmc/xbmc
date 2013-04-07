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

#include <set>

#define PRE_SKIN_VERSION_9_10_COMPATIBILITY 1
#define PRE_SKIN_VERSION_11_COMPATIBILITY 1

#include "settings/ISettingsHandler.h"
#include "settings/ISubSettings.h"
#include "threads/CriticalSection.h"
#include "utils/StdString.h"

class CGUISettings;
class TiXmlElement;
class TiXmlNode;

class CSettings : private ISettingsHandler, ISubSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  void RegisterSettingsHandler(ISettingsHandler *settingsHandler);
  void UnregisterSettingsHandler(ISettingsHandler *settingsHandler);
  void RegisterSubSettings(ISubSettings *subSettings);
  void UnregisterSubSettings(ISubSettings *subSettings);

  bool Load();
  void Save() const;
  bool SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings = NULL) const;
  bool Reset();
  void Clear();

protected:
  bool LoadSettings(const CStdString& strSettingsFile);

private:
  // implementation of ISettingsHandler
  virtual bool OnSettingsLoading();
  virtual void OnSettingsLoaded();
  virtual bool OnSettingsSaving() const;
  virtual void OnSettingsSaved() const;
  virtual void OnSettingsCleared();

  // implementation of ISubSettings
  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  CCriticalSection m_critical;
  typedef std::set<ISettingsHandler*> SettingsHandlers;
  SettingsHandlers m_settingsHandlers;
  typedef std::set<ISubSettings*> SubSettings;
  SubSettings m_subSettings;
};

extern class CSettings g_settings;

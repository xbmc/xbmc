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
#include <string>

#include "settings/ISettingCallback.h"
#include "settings/ISettingCreator.h"
#include "settings/Setting.h"
#include "threads/CriticalSection.h"

class CSettingSection;
class CSettingsManager;
class TiXmlElement;
class TiXmlNode;

class CSettings : public ISettingCreator
{
public:
  CSettings();
  virtual ~CSettings();

  static CSettings& Get();

  // implementation of ISettingCreator
  virtual CSetting* CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager = NULL) const;

  bool Initialize();
  bool Load();
  bool Load(const std::string &file);
  bool Load(const TiXmlElement *root, bool hide = false);
  void SetLoaded();
  bool Save();
  bool Save(const std::string &file);
  void Unload();
  void Uninitialize();

  void RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList);
  void UnregisterCallback(ISettingCallback *callback);

  CSetting* GetSetting(const std::string &id) const;
  CSettingSection* GetSection(const std::string &section) const;
  SettingDependencyMap GetDependencies(const std::string &id) const;
  SettingDependencyMap GetDependencies(const CSetting *setting) const;
  void* GetSettingOptionsFiller(const CSetting *setting);
    
  bool GetBool(const std::string &id) const;
  int GetInt(const std::string &id) const;
  double GetNumber(const std::string &id) const;
  std::string GetString(const std::string &id) const;

  bool SetBool(const std::string &id, bool value);
  bool ToggleBool(const std::string &id);
  bool SetInt(const std::string &id, int value);
  bool SetNumber(const std::string &id, double value);
  bool SetString(const std::string &id, const std::string &value);

  bool LoadSetting(const TiXmlNode *node, const std::string &settingId);

private:
  CSettings(const CSettings&);
  CSettings const& operator=(CSettings const&);

  bool Initialize(const std::string &file);
  bool InitializeDefinitions();
  void InitializeSettingTypes();
  void InitializeVisibility();
  void InitializeDefaults();
  void InitializeOptionFillers();
  void InitializeConditions();
  void InitializeISettingsHandlers();
  void InitializeISubSettings();
  void InitializeISettingCallbacks();
  bool Reset();
  
  bool m_initialized;
  CSettingsManager *m_settingsManager;
  CCriticalSection m_critical;
};

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

#include "ISetting.h"
#include "ISettingCallback.h"
#include "ISettingCreator.h"
#include "ISettingsHandler.h"
#include "ISubSettings.h"
#include "SettingConditions.h"
#include "SettingDependency.h"
#include "threads/CriticalSection.h"

class CSettingSection;
class CSettingUpdate;

class TiXmlElement;
class TiXmlNode;

typedef std::pair<std::string, int> IntegerSettingOption;
typedef std::pair<std::string, std::string> StringSettingOption;
typedef std::vector<IntegerSettingOption> IntegerSettingOptions;
typedef std::vector<StringSettingOption> StringSettingOptions;
typedef void (*IntegerSettingOptionsFiller)(const CSetting *setting, IntegerSettingOptions &list, int &current);
typedef void (*StringSettingOptionsFiller)(const CSetting *setting, StringSettingOptions &list, std::string &current);

class CSettingsManager : public ISettingCreator, private ISettingCallback,
                         private ISettingsHandler, private ISubSettings
{
public:
  CSettingsManager();
  virtual ~CSettingsManager();

  // implementation of ISettingCreator
  virtual CSetting* CreateSetting(const std::string &settingType, const std::string &settingId, CSettingsManager *settingsManager = NULL) const;

  bool Initialize(const TiXmlElement *root);
  bool Load(const TiXmlElement *root, bool &updated, bool triggerEvents = true, std::map<std::string, CSetting*> *loadedSettings = NULL);
  virtual bool Save(TiXmlNode *root) const;
  void Unload();
  void Clear();

  bool LoadSetting(const TiXmlNode *node, const std::string &settingId);

  void SetInitialized();
  void SetLoaded() { m_loaded = true; }

  void RegisterCallback(ISettingCallback *callback, const std::set<std::string> &settingList);
  void UnregisterCallback(ISettingCallback *callback);

  void RegisterSettingType(const std::string &settingType, ISettingCreator *settingCreator);

  void RegisterSettingsHandler(ISettingsHandler *settingsHandler);
  void UnregisterSettingsHandler(ISettingsHandler *settingsHandler);

  void RegisterSubSettings(ISubSettings *subSettings);
  void UnregisterSubSettings(ISubSettings *subSettings);

  void RegisterSettingOptionsFiller(const std::string &identifier, IntegerSettingOptionsFiller optionsFiller);
  void RegisterSettingOptionsFiller(const std::string &identifier, StringSettingOptionsFiller optionsFiller);
  void UnregisterSettingOptionsFiller(const std::string &identifier);
  void* GetSettingOptionsFiller(const CSetting *setting);

  CSetting* GetSetting(const std::string &id) const;
  CSettingSection* GetSection(const std::string &section) const;
  SettingDependencyMap GetDependencies(const std::string &id) const;
  SettingDependencyMap GetDependencies(const CSetting *setting) const;
    
  bool GetBool(const std::string &id) const;
  int GetInt(const std::string &id) const;
  double GetNumber(const std::string &id) const;
  std::string GetString(const std::string &id) const;

  bool SetBool(const std::string &id, bool value);
  bool ToggleBool(const std::string &id);
  bool SetInt(const std::string &id, int value);
  bool SetNumber(const std::string &id, double value);
  bool SetString(const std::string &id, const std::string &value);

  const CSettingConditionsManager& GetConditions() const { return m_conditions; }
  void AddCondition(const std::string &condition);
  void AddCondition(const std::string &identifier, SettingConditionCheck condition);

private:
  // implementation of ISettingCallback
  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);

  // implementation of ISettingsHandler
  virtual bool OnSettingsLoading();
  virtual void OnSettingsLoaded();
  virtual bool OnSettingsSaving() const;
  virtual void OnSettingsSaved() const;
  virtual void OnSettingsCleared();

  // implementation of ISubSettings
  virtual bool Load(const TiXmlNode *settings);

  bool Serialize(TiXmlNode *parent) const;
  bool Deserialize(const TiXmlNode *node, std::map<std::string, CSetting*> *loadedSettings = NULL);

  bool LoadSetting(const TiXmlNode *node, CSetting *setting);
  bool UpdateSettings(const TiXmlNode *root);
  bool UpdateSetting(const TiXmlNode *node, CSetting *setting, const CSettingUpdate& update);

  typedef enum {
    SettingOptionsFillerTypeNone = 0,
    SettingOptionsFillerTypeInteger,
    SettingOptionsFillerTypeString
  } SettingOptionsFillerType;

  void RegisterSettingOptionsFiller(const std::string &identifier, void *filler, SettingOptionsFillerType type);

  typedef std::set<ISettingCallback *> CallbackSet;
  typedef struct {
    CSetting *setting;
    SettingDependencyMap dependencies;
    CallbackSet callbacks;
  } Setting;

  bool m_initialized;
  bool m_loaded;
  
  typedef std::map<std::string, Setting> SettingMap;
  SettingMap m_settings;
  typedef std::map<std::string, CSettingSection*> SettingSectionMap;
  SettingSectionMap m_sections;

  typedef std::map<std::string, ISettingCreator*> SettingCreatorMap;
  SettingCreatorMap m_settingCreators;

  std::set<ISubSettings*> m_subSettings;
  typedef std::set<ISettingsHandler*> SettingsHandlers;
  SettingsHandlers m_settingsHandlers;

  CSettingConditionsManager m_conditions;

  typedef struct {
    void *filler;
    SettingOptionsFillerType type;
  } SettingOptionsFiller;
  typedef std::map<std::string, SettingOptionsFiller> SettingOptionsFillerMap;
  SettingOptionsFillerMap m_optionsFillers;

  CCriticalSection m_critical;
};

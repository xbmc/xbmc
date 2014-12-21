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
#include <string>
#include <vector>

#include <memory>

#include "ISetting.h"
#include "ISettingCallback.h"
#include "ISettingControl.h"
#include "SettingDefinitions.h"
#include "SettingDependency.h"
#include "SettingUpdate.h"
#include "threads/SharedSection.h"

/*!
 \ingroup settings
 \brief Basic setting types available in the settings system.
 */
typedef enum {
  SettingTypeNone = 0,
  SettingTypeBool,
  SettingTypeInteger,
  SettingTypeNumber,
  SettingTypeString,
  SettingTypeAction,
  SettingTypeList
} SettingType;

/*!
 \ingroup settings
 \brief Levels which every setting is assigned to.
 */
typedef enum {
  SettingLevelBasic  = 0,
  SettingLevelStandard,
  SettingLevelAdvanced,
  SettingLevelExpert,
  SettingLevelInternal
} SettingLevel;

typedef enum {
  SettingOptionsTypeNone = 0,
  SettingOptionsTypeStatic,
  SettingOptionsTypeDynamic
} SettingOptionsType;

/*!
 \ingroup settings
 \brief Setting base class containing all the properties which are common to
 all settings independent of the setting type.
 */
class CSetting : public ISetting,
                 protected ISettingCallback
{
public:
  CSetting(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSetting(const std::string &id, const CSetting &setting);
  virtual ~CSetting();

  virtual CSetting* Clone(const std::string &id) const = 0;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const = 0;
  virtual bool FromString(const std::string &value) = 0;
  virtual std::string ToString() const = 0;
  virtual bool Equals(const std::string &value) const = 0;
  virtual bool CheckValidity(const std::string &value) const = 0;
  virtual void Reset() = 0;

  int GetLabel() const { return m_label; }
  void SetLabel(int label) { m_label = label; }
  int GetHelp() const { return m_help; }
  void SetHelp(int help) { m_help = help; }
  bool IsEnabled() const;
  void SetEnabled(bool enabled);
  bool IsDefault() const { return !m_changed; }
  const std::string& GetParent() const { return m_parentSetting; }
  void SetParent(const std::string& parentSetting) { m_parentSetting = parentSetting; }
  SettingLevel GetLevel() const { return m_level; }
  void SetLevel(SettingLevel level) { m_level = level; }
  const ISettingControl* GetControl() const { return m_control; }
  ISettingControl* GetControl() { return m_control; }
  void SetControl(ISettingControl* control) { m_control = control; }
  const SettingDependencies& GetDependencies() const { return m_dependencies; }
  void SetDependencies(const SettingDependencies &dependencies) { m_dependencies = dependencies; }
  const std::set<CSettingUpdate>& GetUpdates() const { return m_updates; }

  void SetCallback(ISettingCallback *callback) { m_callback = callback; }

  // overrides of ISetting
  virtual bool IsVisible() const;

protected:
  // implementation of ISettingCallback
  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);
  virtual void OnSettingPropertyChanged(const CSetting *setting, const char *propertyName);

  void Copy(const CSetting &setting);

  ISettingCallback *m_callback;
  int m_label;
  int m_help;
  bool m_enabled;
  std::string m_parentSetting;
  SettingLevel m_level;
  ISettingControl *m_control;
  SettingDependencies m_dependencies;
  std::set<CSettingUpdate> m_updates;
  bool m_changed;
  CSharedSection m_critical;
};

typedef std::shared_ptr<CSetting> SettingPtr;

typedef std::vector<CSetting *> SettingList;
typedef std::vector<SettingPtr> SettingPtrList;

/*!
 \ingroup settings
 \brief List setting implementation
 \sa CSetting
 */
class CSettingList : public CSetting
{
public:
  CSettingList(const std::string &id, CSetting *settingDefinition, CSettingsManager *settingsManager = NULL);
  CSettingList(const std::string &id, CSetting *settingDefinition, int label, CSettingsManager *settingsManager = NULL);
  CSettingList(const std::string &id, const CSettingList &setting);
  virtual ~CSettingList();

  virtual CSetting* Clone(const std::string &id) const;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeList; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset();
  
  int GetElementType() const;
  const CSetting* GetDefinition() const { return m_definition; }
  void SetDefinition(CSetting *definition) { m_definition = definition; }

  const std::string& GetDelimiter() const { return m_delimiter; }
  void SetDelimiter(const std::string &delimiter) { m_delimiter = delimiter; }
  int GetMinimumItems() const { return m_minimumItems; }
  void SetMinimumItems(int minimumItems) { m_minimumItems = minimumItems; }
  int GetMaximumItems() const { return m_maximumItems; }
  void SetMaximumItems(int maximumItems) { m_maximumItems = maximumItems; }
  
  bool FromString(const std::vector<std::string> &value);

  const SettingPtrList& GetValue() const { return m_values; }
  bool SetValue(const SettingPtrList &values);
  const SettingPtrList& GetDefault() const { return m_defaults; }
  void SetDefault(const SettingPtrList &values);

protected:
  void copy(const CSettingList &setting);
  static void copy(const SettingPtrList &srcValues, SettingPtrList &dstValues);
  bool fromString(const std::string &strValue, SettingPtrList &values) const;
  bool fromValues(const std::vector<std::string> &strValues, SettingPtrList &values) const;
  std::string toString(const SettingPtrList &values) const;

  SettingPtrList m_values;
  SettingPtrList m_defaults;
  CSetting *m_definition;
  std::string m_delimiter;
  int m_minimumItems;
  int m_maximumItems;
};

/*!
 \ingroup settings
 \brief Boolean setting implementation.
 \sa CSetting
 */
class CSettingBool : public CSetting
{
public:
  CSettingBool(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingBool(const std::string &id, const CSettingBool &setting);
  CSettingBool(const std::string &id, int label, bool value, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingBool() { }

  virtual CSetting* Clone(const std::string &id) const;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeBool; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset() { SetValue(m_default); }

  bool GetValue() const { CSharedLock lock(m_critical); return m_value; }
  bool SetValue(bool value);
  bool GetDefault() const { return m_default; }
  void SetDefault(bool value);

private:
  void copy(const CSettingBool &setting);
  bool fromString(const std::string &strValue, bool &value) const;

  bool m_value;
  bool m_default;
};

/*!
 \ingroup settings
 \brief Integer setting implementation
 \sa CSetting
 */
class CSettingInt : public CSetting
{
public:
  CSettingInt(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, const CSettingInt &setting);
  CSettingInt(const std::string &id, int label, int value, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, int label, int value, const StaticIntegerSettingOptions &options, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingInt() { }

  virtual CSetting* Clone(const std::string &id) const;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeInteger; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual bool CheckValidity(int value) const;
  virtual void Reset() { SetValue(m_default); }

  int GetValue() const { CSharedLock lock(m_critical); return m_value; }
  bool SetValue(int value);
  int GetDefault() const { return m_default; }
  void SetDefault(int value);

  int GetMinimum() const { return m_min; }
  void SetMinimum(int minimum) { m_min = minimum; }
  int GetStep() const { return m_step; }
  void SetStep(int step) { m_step = step; }
  int GetMaximum() const { return m_max; }
  void SetMaximum(int maximum) { m_max = maximum; }

  SettingOptionsType GetOptionsType() const;
  const StaticIntegerSettingOptions& GetOptions() const { return m_options; }
  void SetOptions(const StaticIntegerSettingOptions &options) { m_options = options; }
  const std::string& GetOptionsFillerName() const { return m_optionsFillerName; }
  void SetOptionsFillerName(const std::string &optionsFillerName, void *data = NULL)
  {
    m_optionsFillerName = optionsFillerName;
    m_optionsFillerData = data;
  }
  void SetOptionsFiller(IntegerSettingOptionsFiller optionsFiller, void *data = NULL)
  {
    m_optionsFiller = optionsFiller;
    m_optionsFillerData = data;
  }
  DynamicIntegerSettingOptions UpdateDynamicOptions();

private:
  void copy(const CSettingInt &setting);
  static bool fromString(const std::string &strValue, int &value);

  int m_value;
  int m_default;
  int m_min;
  int m_step;
  int m_max;
  StaticIntegerSettingOptions m_options;
  std::string m_optionsFillerName;
  IntegerSettingOptionsFiller m_optionsFiller;
  void *m_optionsFillerData;
  DynamicIntegerSettingOptions m_dynamicOptions;
};

/*!
 \ingroup settings
 \brief Real number setting implementation.
 \sa CSetting
 */
class CSettingNumber : public CSetting
{
public:
  CSettingNumber(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingNumber(const std::string &id, const CSettingNumber &setting);
  CSettingNumber(const std::string &id, int label, float value, CSettingsManager *settingsManager = NULL);
  CSettingNumber(const std::string &id, int label, float value, float minimum, float step, float maximum, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingNumber() { }

  virtual CSetting* Clone(const std::string &id) const;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeNumber; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual bool CheckValidity(double value) const;
  virtual void Reset() { SetValue(m_default); }

  double GetValue() const { CSharedLock lock(m_critical); return m_value; }
  bool SetValue(double value);
  double GetDefault() const { return m_default; }
  void SetDefault(double value);

  double GetMinimum() const { return m_min; }
  void SetMinimum(double minimum) { m_min = minimum; }
  double GetStep() const { return m_step; }
  void SetStep(double step) { m_step = step; }
  double GetMaximum() const { return m_max; }
  void SetMaximum(double maximum) { m_max = maximum; }

private:
  virtual void copy(const CSettingNumber &setting);
  static bool fromString(const std::string &strValue, double &value);

  double m_value;
  double m_default;
  double m_min;
  double m_step;
  double m_max;
};

/*!
 \ingroup settings
 \brief String setting implementation.
 \sa CSetting
 */
class CSettingString : public CSetting
{
public:
  CSettingString(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingString(const std::string &id, const CSettingString &setting);
  CSettingString(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingString() { }

  virtual CSetting* Clone(const std::string &id) const;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeString; }
  virtual bool FromString(const std::string &value) { return SetValue(value); }
  virtual std::string ToString() const { return m_value; }
  virtual bool Equals(const std::string &value) const { return m_value == value; }
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset() { SetValue(m_default); }

  virtual const std::string& GetValue() const { CSharedLock lock(m_critical); return m_value; }
  virtual bool SetValue(const std::string &value);
  virtual const std::string& GetDefault() const { return m_default; }
  virtual void SetDefault(const std::string &value);

  virtual bool AllowEmpty() const { return m_allowEmpty; }
  void SetAllowEmpty(bool allowEmpty) { m_allowEmpty = allowEmpty; }

  SettingOptionsType GetOptionsType() const;
  const std::string& GetOptionsFillerName() const { return m_optionsFillerName; }
  void SetOptionsFillerName(const std::string &optionsFillerName, void *data = NULL)
  {
    m_optionsFillerName = optionsFillerName;
    m_optionsFillerData = data;
  }
  void SetOptionsFiller(StringSettingOptionsFiller optionsFiller, void *data = NULL)
  {
    m_optionsFiller = optionsFiller;
    m_optionsFillerData = data;
  }
  DynamicStringSettingOptions UpdateDynamicOptions();

protected:
  virtual void copy(const CSettingString &setting);

  std::string m_value;
  std::string m_default;
  bool m_allowEmpty;
  std::string m_optionsFillerName;
  StringSettingOptionsFiller m_optionsFiller;
  void *m_optionsFillerData;
  DynamicStringSettingOptions m_dynamicOptions;
};

/*!
 \ingroup settings
 \brief Action setting implementation.

 A setting action will trigger a call to the OnSettingAction() callback method
 when activated.

 \sa CSetting
 */
class CSettingAction : public CSetting
{
public:
  CSettingAction(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingAction(const std::string &id, int label, CSettingsManager *settingsManager = NULL);
  CSettingAction(const std::string &id, const CSettingAction &setting);
  virtual ~CSettingAction() { }

  virtual CSetting* Clone(const std::string &id) const;

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeAction; }
  virtual bool FromString(const std::string &value) { return false; }
  virtual std::string ToString() const { return ""; }
  virtual bool Equals(const std::string &value) const { return false; }
  virtual bool CheckValidity(const std::string &value) const { return false; }
  virtual void Reset() { }

  // this needs to be public so it can be triggered when activated
  // by the user in the GUI.
  virtual void OnSettingAction(const CSetting *setting) { return CSetting::OnSettingAction(this); }
};

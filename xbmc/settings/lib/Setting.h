/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ISetting.h"
#include "ISettingCallback.h"
#include "ISettingControl.h"
#include "SettingDefinitions.h"
#include "SettingDependency.h"
#include "SettingLevel.h"
#include "SettingType.h"
#include "SettingUpdate.h"
#include "threads/SharedSection.h"

#include <memory>
#include <set>
#include <string>
#include <vector>

enum class SettingOptionsType {
  Unknown = 0,
  StaticTranslatable,
  Static,
  Dynamic
};

class CSetting;
using SettingPtr = std::shared_ptr<CSetting>;
using SettingConstPtr = std::shared_ptr<const CSetting>;
using SettingList = std::vector<SettingPtr>;

/*!
 \ingroup settings
 \brief Setting base class containing all the properties which are common to
 all settings independent of the setting type.
 */
class CSetting : public ISetting,
                 protected ISettingCallback,
                 public std::enable_shared_from_this<CSetting>
{
public:
  CSetting(const std::string &id, CSettingsManager *settingsManager = nullptr);
  CSetting(const std::string &id, const CSetting &setting);
  ~CSetting() override = default;

  virtual std::shared_ptr<CSetting> Clone(const std::string &id) const = 0;
  void MergeBasics(const CSetting& other);
  virtual void MergeDetails(const CSetting& other) = 0;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  virtual SettingType GetType() const = 0;
  virtual bool FromString(const std::string &value) = 0;
  virtual std::string ToString() const = 0;
  virtual bool Equals(const std::string &value) const = 0;
  virtual bool CheckValidity(const std::string &value) const = 0;
  virtual void Reset() = 0;

  bool IsEnabled() const;
  bool GetEnabled() const { return m_enabled; }
  void SetEnabled(bool enabled);
  bool IsDefault() const { return !m_changed; }
  const std::string& GetParent() const { return m_parentSetting; }
  void SetParent(const std::string& parentSetting) { m_parentSetting = parentSetting; }
  SettingLevel GetLevel() const { return m_level; }
  void SetLevel(SettingLevel level) { m_level = level; }
  std::shared_ptr<const ISettingControl> GetControl() const { return m_control; }
  std::shared_ptr<ISettingControl> GetControl() { return m_control; }
  void SetControl(std::shared_ptr<ISettingControl> control) { m_control = control; }
  const SettingDependencies& GetDependencies() const { return m_dependencies; }
  void SetDependencies(const SettingDependencies &dependencies) { m_dependencies = dependencies; }
  const std::set<CSettingUpdate>& GetUpdates() const { return m_updates; }

  void SetCallback(ISettingCallback *callback) { m_callback = callback; }

  bool IsReference() const { return !m_referencedId.empty(); }
  const std::string& GetReferencedId() const { return m_referencedId; }
  void SetReferencedId(const std::string& referencedId) { m_referencedId = referencedId; }
  void MakeReference(const std::string& referencedId = "");

  bool GetVisible() const { return ISetting::IsVisible(); }
  // overrides of ISetting
  bool IsVisible() const override;

  // implementation of ISettingCallback
  void OnSettingAction(std::shared_ptr<const CSetting> setting) override;

protected:
  // implementation of ISettingCallback
  bool OnSettingChanging(std::shared_ptr<const CSetting> setting) override;
  void OnSettingChanged(std::shared_ptr<const CSetting> setting) override;
  bool OnSettingUpdate(std::shared_ptr<CSetting> setting, const char *oldSettingId, const TiXmlNode *oldSettingNode) override;
  void OnSettingPropertyChanged(std::shared_ptr<const CSetting> setting, const char *propertyName) override;

  void Copy(const CSetting &setting);

  template<class TSetting>
  std::shared_ptr<TSetting> shared_from_base()
  {
    return std::static_pointer_cast<TSetting>(shared_from_this());
  }

  ISettingCallback *m_callback = nullptr;
  bool m_enabled = true;
  std::string m_parentSetting;
  SettingLevel m_level = SettingLevel::Standard;
  std::shared_ptr<ISettingControl> m_control;
  SettingDependencies m_dependencies;
  std::set<CSettingUpdate> m_updates;
  bool m_changed = false;
  mutable CSharedSection m_critical;

  std::string m_referencedId;
};

template<typename TValue, SettingType TSettingType>
class CTraitedSetting : public CSetting
{
public:
  typedef TValue Value;

  // implementation of CSetting
  SettingType GetType() const override { return TSettingType; }

  static SettingType Type() { return TSettingType; }

protected:
  CTraitedSetting(const std::string &id, CSettingsManager *settingsManager = nullptr)
    : CSetting(id, settingsManager)
  { }
  CTraitedSetting(const std::string &id, const CTraitedSetting &setting)
    : CSetting(id, setting)
  { }
  ~CTraitedSetting() override = default;
};

/*!
 \ingroup settings
 \brief List setting implementation
 \sa CSetting
 */
class CSettingList : public CSetting
{
public:
  CSettingList(const std::string &id, std::shared_ptr<CSetting> settingDefinition, CSettingsManager *settingsManager = nullptr);
  CSettingList(const std::string &id, std::shared_ptr<CSetting> settingDefinition, int label, CSettingsManager *settingsManager = nullptr);
  CSettingList(const std::string &id, const CSettingList &setting);
  ~CSettingList() override = default;

  std::shared_ptr<CSetting> Clone(const std::string &id) const override;
  void MergeDetails(const CSetting& other) override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  SettingType GetType() const override { return SettingType::List; }
  bool FromString(const std::string &value) override;
  std::string ToString() const override;
  bool Equals(const std::string &value) const override;
  bool CheckValidity(const std::string &value) const override;
  void Reset() override;

  SettingType GetElementType() const;
  std::shared_ptr<CSetting> GetDefinition() { return m_definition; }
  std::shared_ptr<const CSetting> GetDefinition() const { return m_definition; }
  void SetDefinition(std::shared_ptr<CSetting> definition) { m_definition = definition; }

  const std::string& GetDelimiter() const { return m_delimiter; }
  void SetDelimiter(const std::string &delimiter) { m_delimiter = delimiter; }
  int GetMinimumItems() const { return m_minimumItems; }
  void SetMinimumItems(int minimumItems) { m_minimumItems = minimumItems; }
  int GetMaximumItems() const { return m_maximumItems; }
  void SetMaximumItems(int maximumItems) { m_maximumItems = maximumItems; }

  bool FromString(const std::vector<std::string> &value);

  const SettingList& GetValue() const { return m_values; }
  bool SetValue(const SettingList &values);
  const SettingList& GetDefault() const { return m_defaults; }
  void SetDefault(const SettingList &values);
  bool FindIntInList(int value) const;

protected:
  void copy(const CSettingList &setting);
  static void copy(const SettingList &srcValues, SettingList &dstValues);
  bool fromString(const std::string &strValue, SettingList &values) const;
  bool fromValues(const std::vector<std::string> &strValues, SettingList &values) const;
  std::string toString(const SettingList &values) const;

  SettingList m_values;
  SettingList m_defaults;
  std::shared_ptr<CSetting> m_definition;
  std::string m_delimiter = "|";
  int m_minimumItems = 0;
  int m_maximumItems = -1;
};

/*!
 \ingroup settings
 \brief Boolean setting implementation.
 \sa CSetting
 */
class CSettingBool : public CTraitedSetting<bool, SettingType::Boolean>
{
public:
  CSettingBool(const std::string &id, CSettingsManager *settingsManager = nullptr);
  CSettingBool(const std::string &id, const CSettingBool &setting);
  CSettingBool(const std::string &id, int label, bool value, CSettingsManager *settingsManager = nullptr);
  ~CSettingBool() override = default;

  std::shared_ptr<CSetting> Clone(const std::string &id) const override;
  void MergeDetails(const CSetting& other) override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  bool FromString(const std::string &value) override;
  std::string ToString() const override;
  bool Equals(const std::string &value) const override;
  bool CheckValidity(const std::string &value) const override;
  void Reset() override { SetValue(m_default); }

  bool GetValue() const { CSharedLock lock(m_critical); return m_value; }
  bool SetValue(bool value);
  bool GetDefault() const { return m_default; }
  void SetDefault(bool value);

private:
  void copy(const CSettingBool &setting);
  bool fromString(const std::string &strValue, bool &value) const;

  bool m_value = false;
  bool m_default = false;
};

/*!
 \ingroup settings
 \brief Integer setting implementation
 \sa CSetting
 */
class CSettingInt : public CTraitedSetting<int, SettingType::Integer>
{
public:
  CSettingInt(const std::string &id, CSettingsManager *settingsManager = nullptr);
  CSettingInt(const std::string &id, const CSettingInt &setting);
  CSettingInt(const std::string &id, int label, int value, CSettingsManager *settingsManager = nullptr);
  CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, CSettingsManager *settingsManager = nullptr);
  CSettingInt(const std::string &id, int label, int value, const TranslatableIntegerSettingOptions &options, CSettingsManager *settingsManager = nullptr);
  ~CSettingInt() override = default;

  std::shared_ptr<CSetting> Clone(const std::string &id) const override;
  void MergeDetails(const CSetting& other) override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  bool FromString(const std::string &value) override;
  std::string ToString() const override;
  bool Equals(const std::string &value) const override;
  bool CheckValidity(const std::string &value) const override;
  virtual bool CheckValidity(int value) const;
  void Reset() override { SetValue(m_default); }

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
  const TranslatableIntegerSettingOptions& GetTranslatableOptions() const { return m_translatableOptions; }
  void SetTranslatableOptions(const TranslatableIntegerSettingOptions &options) { m_translatableOptions = options; }
  const IntegerSettingOptions& GetOptions() const { return m_options; }
  void SetOptions(const IntegerSettingOptions &options) { m_options = options; }
  const std::string& GetOptionsFillerName() const { return m_optionsFillerName; }
  void SetOptionsFillerName(const std::string &optionsFillerName, void *data = nullptr)
  {
    m_optionsFillerName = optionsFillerName;
    m_optionsFillerData = data;
  }
  void SetOptionsFiller(IntegerSettingOptionsFiller optionsFiller, void *data = nullptr)
  {
    m_optionsFiller = optionsFiller;
    m_optionsFillerData = data;
  }
  IntegerSettingOptions UpdateDynamicOptions();
  SettingOptionsSort GetOptionsSort() const { return m_optionsSort; }
  void SetOptionsSort(SettingOptionsSort optionsSort) { m_optionsSort = optionsSort; }

private:
  void copy(const CSettingInt &setting);
  static bool fromString(const std::string &strValue, int &value);

  int m_value = 0;
  int m_default = 0;
  int m_min = 0;
  int m_step = 1;
  int m_max = 0;
  TranslatableIntegerSettingOptions m_translatableOptions;
  IntegerSettingOptions m_options;
  std::string m_optionsFillerName;
  IntegerSettingOptionsFiller m_optionsFiller = nullptr;
  void *m_optionsFillerData = nullptr;
  IntegerSettingOptions m_dynamicOptions;
  SettingOptionsSort m_optionsSort = SettingOptionsSort::NoSorting;
};

/*!
 \ingroup settings
 \brief Real number setting implementation.
 \sa CSetting
 */
class CSettingNumber : public CTraitedSetting<double, SettingType::Number>
{
public:
  CSettingNumber(const std::string &id, CSettingsManager *settingsManager = nullptr);
  CSettingNumber(const std::string &id, const CSettingNumber &setting);
  CSettingNumber(const std::string &id, int label, float value, CSettingsManager *settingsManager = nullptr);
  CSettingNumber(const std::string &id, int label, float value, float minimum, float step, float maximum, CSettingsManager *settingsManager = nullptr);
  ~CSettingNumber() override = default;

  std::shared_ptr<CSetting> Clone(const std::string &id) const override;
  void MergeDetails(const CSetting& other) override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  bool FromString(const std::string &value) override;
  std::string ToString() const override;
  bool Equals(const std::string &value) const override;
  bool CheckValidity(const std::string &value) const override;
  virtual bool CheckValidity(double value) const;
  void Reset() override { SetValue(m_default); }

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

  double m_value = 0.0;
  double m_default = 0.0;
  double m_min = 0.0;
  double m_step = 1.0;
  double m_max = 0.0;
};

/*!
 \ingroup settings
 \brief String setting implementation.
 \sa CSetting
 */
class CSettingString : public CTraitedSetting<std::string, SettingType::String>
{
public:
  CSettingString(const std::string &id, CSettingsManager *settingsManager = nullptr);
  CSettingString(const std::string &id, const CSettingString &setting);
  CSettingString(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = nullptr);
  ~CSettingString() override = default;

  std::shared_ptr<CSetting> Clone(const std::string &id) const override;
  void MergeDetails(const CSetting& other) override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  bool FromString(const std::string &value) override { return SetValue(value); }
  std::string ToString() const override { return m_value; }
  bool Equals(const std::string &value) const override { return m_value == value; }
  bool CheckValidity(const std::string &value) const override;
  void Reset() override { SetValue(m_default); }

  virtual const std::string& GetValue() const { CSharedLock lock(m_critical); return m_value; }
  virtual bool SetValue(const std::string &value);
  virtual const std::string& GetDefault() const { return m_default; }
  virtual void SetDefault(const std::string &value);

  virtual bool AllowEmpty() const { return m_allowEmpty; }
  void SetAllowEmpty(bool allowEmpty) { m_allowEmpty = allowEmpty; }

  SettingOptionsType GetOptionsType() const;
  const TranslatableStringSettingOptions& GetTranslatableOptions() const { return m_translatableOptions; }
  void SetTranslatableOptions(const TranslatableStringSettingOptions &options) { m_translatableOptions = options; }
  const StringSettingOptions& GetOptions() const { return m_options; }
  void SetOptions(const StringSettingOptions &options) { m_options = options; }
  const std::string& GetOptionsFillerName() const { return m_optionsFillerName; }
  void SetOptionsFillerName(const std::string &optionsFillerName, void *data = nullptr)
  {
    m_optionsFillerName = optionsFillerName;
    m_optionsFillerData = data;
  }
  void SetOptionsFiller(StringSettingOptionsFiller optionsFiller, void *data = nullptr)
  {
    m_optionsFiller = optionsFiller;
    m_optionsFillerData = data;
  }
  StringSettingOptions UpdateDynamicOptions();
  SettingOptionsSort GetOptionsSort() const { return m_optionsSort; }
  void SetOptionsSort(SettingOptionsSort optionsSort) { m_optionsSort = optionsSort; }

protected:
  virtual void copy(const CSettingString &setting);

  std::string m_value;
  std::string m_default;
  bool m_allowEmpty = false;
  TranslatableStringSettingOptions m_translatableOptions;
  StringSettingOptions m_options;
  std::string m_optionsFillerName;
  StringSettingOptionsFiller m_optionsFiller = nullptr;
  void *m_optionsFillerData = nullptr;
  StringSettingOptions m_dynamicOptions;
  SettingOptionsSort m_optionsSort = SettingOptionsSort::NoSorting;
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
  CSettingAction(const std::string &id, CSettingsManager *settingsManager = nullptr);
  CSettingAction(const std::string &id, int label, CSettingsManager *settingsManager = nullptr);
  CSettingAction(const std::string &id, const CSettingAction &setting);
  ~CSettingAction() override = default;

  std::shared_ptr<CSetting> Clone(const std::string &id) const override;
  void MergeDetails(const CSetting& other) override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  SettingType GetType() const override { return SettingType::Action; }
  bool FromString(const std::string &value) override { return CheckValidity(value); }
  std::string ToString() const override { return ""; }
  bool Equals(const std::string &value) const override { return value.empty(); }
  bool CheckValidity(const std::string &value) const override { return value.empty(); }
  void Reset() override { }

  bool HasData() const { return !m_data.empty(); }
  const std::string& GetData() const { return m_data; }
  void SetData(const std::string& data) { m_data = data; }

protected:
  std::string m_data;
};

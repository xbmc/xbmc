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

#include <map>
#include <set>
#include <string>
#include <vector>

#include "ISetting.h"
#include "ISettingCallback.h"
#include "SettingControl.h"
#include "SettingDependency.h"
#include "SettingLevel.h"
#include "SettingUpdate.h"
#include "threads/CriticalSection.h"

typedef enum {
  SettingTypeNone = 0,
  SettingTypeBool,
  SettingTypeInteger,
  SettingTypeNumber,
  SettingTypeString,
  SettingTypeAction
} SettingType;

typedef std::pair<int, int> SettingOption;
typedef std::vector<SettingOption> SettingOptions;

class CSetting : public ISetting,
                 protected ISettingCallback
{
public:
  CSetting(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSetting(const std::string &id, const CSetting &setting);
  virtual ~CSetting() { }

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const = 0;
  virtual bool FromString(const std::string &value) = 0;
  virtual std::string ToString() const = 0;
  virtual bool Equals(const std::string &value) const = 0;
  virtual bool CheckValidity(const std::string &value) const = 0;
  virtual void Reset() = 0;

  int GetLabel() const { return m_label; }
  int GetHelp() const { return m_help; }
  SettingLevel GetLevel() const { return m_level.GetLevel(); }
  const CSettingControl& GetControl() const { return m_control; }
  const SettingDependencies& GetDependencies() const { return m_dependencies; }
  const std::set<CSettingUpdate>& GetUpdates() const { return m_updates; }

protected:
  friend class CSettingsManager;
    
  virtual bool OnSettingChanging(const CSetting *setting);
  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);

  void Copy(const CSetting &setting);

  ISettingCallback *m_callback;
  int m_label;
  int m_help;
  CSettingLevel m_level;
  CSettingControl m_control;
  SettingDependencies m_dependencies;
  std::set<CSettingUpdate> m_updates;
  bool m_changed;
  CCriticalSection m_critical;
};

typedef std::vector<CSetting *> SettingList;

class CSettingBool : public CSetting
{
public:
  CSettingBool(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingBool(const std::string &id, const CSettingBool &setting);
  CSettingBool(const std::string &id, int label, bool value, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingBool() { }

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeBool; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset() { SetValue(m_default); }

  bool GetValue() const { return m_value; }
  bool SetValue(bool value);
  bool GetDefault() const { return m_default; }
  void SetDefault(bool value);

private:
  void copy(const CSettingBool &setting);
  bool fromString(const std::string &strValue, bool &value) const;

  bool m_value;
  bool m_default;
};

class CSettingInt : public CSetting
{
public:
  CSettingInt(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, const CSettingInt &setting);
  CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, int format, int minimumLabel, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, int label, int value, int minimum, int step, int maximum, const std::string &format, CSettingsManager *settingsManager = NULL);
  CSettingInt(const std::string &id, int label, int value, const SettingOptions &options, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingInt() { }

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeInteger; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual bool CheckValidity(int value) const;
  virtual void Reset() { SetValue(m_default); }

  int GetValue() const { return m_value; }
  bool SetValue(int value);
  int GetDefault() const { return m_default; }
  void SetDefault(int value);

  int GetMinimum() const { return m_min; }
  int GetStep() const { return m_step; }
  int GetMaximum() const { return m_max; }
    
  int GetFormat() const { return m_format; }
  int GetMinimumLabel() const { return m_labelMin; }
  const std::string& GetFormatString() const { return m_strFormat; }
  const SettingOptions& GetOptions() const { return m_options; }
  const std::string& GetOptionsFiller() const { return m_optionsFiller; }

private:
  void copy(const CSettingInt &setting);
  bool fromString(const std::string &strValue, int &value) const;

  int m_value;
  int m_default;
  int m_min;
  int m_step;
  int m_max;
  int m_format;
  int m_labelMin;
  std::string m_strFormat;
  SettingOptions m_options;
  std::string m_optionsFiller;
};

class CSettingNumber : public CSetting
{
public:
  CSettingNumber(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingNumber(const std::string &id, const CSettingNumber &setting);
  CSettingNumber(const std::string &id, int label, float value, float minimum, float step, float maximum, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingNumber() { }

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeNumber; }
  virtual bool FromString(const std::string &value);
  virtual std::string ToString() const;
  virtual bool Equals(const std::string &value) const;
  virtual bool CheckValidity(const std::string &value) const;
  virtual bool CheckValidity(double value) const;
  virtual void Reset() { SetValue(m_default); }

  double GetValue() const { return m_value; }
  bool SetValue(double value);
  double GetDefault() const { return m_default; }
  void SetDefault(double value);
    
  double GetMinimum() const { return m_min; }
  double GetStep() const { return m_step; }
  double GetMaximum() const { return m_max; }

private:
  virtual void copy(const CSettingNumber &setting);
  bool fromString(const std::string &strValue, double &value) const;

  double m_value;
  double m_default;
  double m_min;
  double m_step;
  double m_max;
};

class CSettingString : public CSetting
{
public:
  CSettingString(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingString(const std::string &id, const CSettingString &setting);
  CSettingString(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = NULL);
  virtual ~CSettingString() { }

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  virtual int GetType() const { return SettingTypeString; }
  virtual bool FromString(const std::string &value) { return SetValue(value); }
  virtual std::string ToString() const { return m_value; }
  virtual bool Equals(const std::string &value) const { return m_value == value; }
  virtual bool CheckValidity(const std::string &value) const;
  virtual void Reset() { SetValue(m_default); }

  virtual const std::string& GetValue() const { return m_value; }
  virtual bool SetValue(const std::string &value);
  virtual const std::string& GetDefault() const { return m_default; }
  virtual void SetDefault(const std::string &value);

  virtual bool AllowEmpty() const { return m_allowEmpty; }
  virtual int GetHeading() const { return m_heading; }
  
  const std::string& GetOptionsFiller() const { return m_optionsFiller; }

protected:
  virtual void copy(const CSettingString &setting);

  std::string m_value;
  std::string m_default;
  bool m_allowEmpty;
  int m_heading;
  std::string m_optionsFiller;
};

class CSettingAction : public CSetting
{
public:
  CSettingAction(const std::string &id, CSettingsManager *settingsManager = NULL);
  CSettingAction(const std::string &id, const CSettingAction &setting);
  virtual ~CSettingAction() { }
    
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

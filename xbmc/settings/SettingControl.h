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

#include "settings/lib/ISettingControl.h"
#include "settings/lib/ISettingControlCreator.h"

#define SETTING_XML_ELM_CONTROL_FORMATLABEL  "formatlabel"
#define SETTING_XML_ELM_CONTROL_HIDDEN       "hidden"
#define SETTING_XML_ELM_CONTROL_VERIFYNEW    "verifynew"
#define SETTING_XML_ELM_CONTROL_HEADING      "heading"
#define SETTING_XML_ELM_CONTROL_HIDEVALUE    "hidevalue"
#define SETTING_XML_ELM_CONTROL_MULTISELECT  "multiselect"
#define SETTING_XML_ELM_CONTROL_POPUP        "popup"
#define SETTING_XML_ELM_CONTROL_FORMATVALUE  "value"
#define SETTING_XML_ATTR_SHOW_MORE           "more"
#define SETTING_XML_ATTR_SHOW_DETAILS        "details"
#define SETTING_XML_ATTR_SEPARATOR_POSITION  "separatorposition"
#define SETTING_XML_ATTR_HIDE_SEPARATOR      "hideseparator"

class CVariant;

class CSettingControlCreator : public ISettingControlCreator
{
public:
  CSettingControlCreator() { }
  virtual ~CSettingControlCreator() { }

  // implementation of ISettingControlCreator
  virtual std::shared_ptr<ISettingControl> CreateControl(const std::string &controlType) const override;
};

class CSettingControlCheckmark : public ISettingControl
{
public:
  CSettingControlCheckmark()
  {
    m_format = "boolean";
  }
  virtual ~CSettingControlCheckmark() { }

  // implementation of ISettingControl
  virtual std::string GetType() const override { return "toggle"; }
  virtual bool SetFormat(const std::string &format) override;
};

class CSettingControlFormattedRange : public ISettingControl
{
public:
  virtual ~CSettingControlFormattedRange() { }

  virtual bool Deserialize(const TiXmlNode *node, bool update = false) override;

  int GetFormatLabel() const { return m_formatLabel; }
  void SetFormatLabel(int formatLabel) { m_formatLabel = formatLabel; }
  const std::string& GetFormatString() const { return m_formatString; }
  void SetFormatString(const std::string &formatString) { m_formatString = formatString; }
  int GetMinimumLabel() const { return m_minimumLabel; }
  void SetMinimumLabel(int minimumLabel) { m_minimumLabel = minimumLabel; }

protected:
  CSettingControlFormattedRange()
    : m_formatLabel(-1),
    m_formatString("%i"),
    m_minimumLabel(-1)
  { }

  int m_formatLabel;
  std::string m_formatString;
  int m_minimumLabel;
};

class CSettingControlSpinner : public CSettingControlFormattedRange
{
public:
  CSettingControlSpinner() { }
  virtual ~CSettingControlSpinner() { }

  // implementation of ISettingControl
  virtual std::string GetType() const override { return "spinner"; }

  // specialization of CSettingControlFormattedRange
  virtual bool SetFormat(const std::string &format) override;
};

class CSettingControlEdit : public ISettingControl
{
public:
  CSettingControlEdit()
    : m_hidden(false),
      m_verifyNewValue(false),
      m_heading(-1)
  {
    m_delayed = true;
  }
  virtual ~CSettingControlEdit() { }

  // implementation of ISettingControl
  virtual std::string GetType() const override { return "edit"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false) override;
  virtual bool SetFormat(const std::string &format) override;

  bool IsHidden() const { return m_hidden; }
  void SetHidden(bool hidden) { m_hidden = hidden; }
  bool VerifyNewValue() const { return m_verifyNewValue; }
  void SetVerifyNewValue(bool verifyNewValue) { m_verifyNewValue = verifyNewValue; }
  int GetHeading() const { return m_heading; }
  void SetHeading(int heading) { m_heading = heading; }

protected:
  bool m_hidden;
  bool m_verifyNewValue;
  int m_heading;
};

class CSettingControlButton : public ISettingControl
{
public:
  CSettingControlButton()
    : m_heading(-1),
      m_hideValue(false),
      m_showAddonDetails(true),
      m_showInstalledAddons(true),
      m_showInstallableAddons(false),
      m_showMoreAddons(true),
      m_useImageThumbs(false),
      m_useFileDirectories(false),
      m_actionData()
  { }
  virtual ~CSettingControlButton() { }

  // implementation of ISettingControl
  virtual std::string GetType() const override { return "button"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false) override;
  virtual bool SetFormat(const std::string &format) override;

  int GetHeading() const { return m_heading; }
  void SetHeading(int heading) { m_heading = heading; }
  bool HideValue() const { return m_hideValue; }
  void SetHideValue(bool hideValue) { m_hideValue = hideValue; }

  bool ShowAddonDetails() const { return m_showAddonDetails; }
  void SetShowAddonDetails(bool showAddonDetails) { m_showAddonDetails = showAddonDetails; }
  bool ShowInstalledAddons() const { return m_showInstalledAddons; }
  void SetShowInstalledAddons(bool showInstalledAddons) { m_showInstalledAddons = showInstalledAddons; }
  bool ShowInstallableAddons() const { return m_showInstallableAddons; }
  void SetShowInstallableAddons(bool showInstallableAddons) { m_showInstallableAddons = showInstallableAddons; }
  bool ShowMoreAddons() const { return !m_showInstallableAddons && m_showMoreAddons; }
  void SetShowMoreAddons(bool showMoreAddons) { m_showMoreAddons = showMoreAddons; }

  bool UseImageThumbs() const { return m_useImageThumbs; }
  void SetUseImageThumbs(bool useImageThumbs) { m_useImageThumbs = useImageThumbs; }
  bool UseFileDirectories() const { return m_useFileDirectories; }
  void SetUseFileDirectories(bool useFileDirectories) { m_useFileDirectories = useFileDirectories; }

  bool HasActionData() const { return !m_actionData.empty(); }
  const std::string& GetActionData() const { return m_actionData; }
  void SetActionData(const std::string& actionData) { m_actionData = actionData; }

protected:
  int m_heading;
  bool m_hideValue;

  bool m_showAddonDetails;
  bool m_showInstalledAddons;
  bool m_showInstallableAddons;
  bool m_showMoreAddons;

  bool m_useImageThumbs;
  bool m_useFileDirectories;

  std::string m_actionData;
};

class CSetting;
typedef std::string (*SettingControlListValueFormatter)(std::shared_ptr<const CSetting> setting);

class CSettingControlList : public CSettingControlFormattedRange
{
public:
  CSettingControlList()
    : m_heading(-1),
      m_multiselect(false),
      m_hideValue(false),
      m_formatter(NULL)
  { }
  virtual ~CSettingControlList() { }

  // implementation of ISettingControl
  virtual std::string GetType() const override { return "list"; }

  // specialization of CSettingControlFormattedRange
  virtual bool Deserialize(const TiXmlNode *node, bool update = false) override;
  virtual bool SetFormat(const std::string &format) override;
  
  int GetHeading() const { return m_heading; }
  void SetHeading(int heading) { m_heading = heading; }
  bool CanMultiSelect() const { return m_multiselect; }
  void SetMultiSelect(bool multiselect) { m_multiselect = multiselect; }
  bool HideValue() const { return m_hideValue; }
  void SetHideValue(bool hideValue) { m_hideValue = hideValue; }

  SettingControlListValueFormatter GetFormatter() const { return m_formatter; }
  void SetFormatter(SettingControlListValueFormatter formatter) { m_formatter = formatter; }

protected:
  int m_heading;
  bool m_multiselect;
  bool m_hideValue;
  SettingControlListValueFormatter m_formatter;
};

class CSettingControlSlider;
typedef std::string (*SettingControlSliderFormatter)(std::shared_ptr<const CSettingControlSlider> control, const CVariant &value, const CVariant &minimum, const CVariant &step, const CVariant &maximum);

class CSettingControlSlider : public ISettingControl
{
public:
  CSettingControlSlider()
    : m_heading(-1),
      m_popup(false),
      m_formatLabel(-1),
      m_formatString("%i"),
      m_formatter(NULL)
  { }
  virtual ~CSettingControlSlider() { }

  // implementation of ISettingControl
  virtual std::string GetType() const override { return "slider"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false) override;
  virtual bool SetFormat(const std::string &format) override;

  int GetHeading() const { return m_heading; }
  void SetHeading(int heading) { m_heading = heading; }
  bool UsePopup() const { return m_popup; }
  void SetPopup(bool popup) { m_popup = popup; }
  int GetFormatLabel() const { return m_formatLabel; }
  void SetFormatLabel(int formatLabel) { m_formatLabel = formatLabel; }
  const std::string& GetFormatString() const { return m_formatString; }
  void SetFormatString(const std::string &formatString) { m_formatString = formatString; }

  SettingControlSliderFormatter GetFormatter() const { return m_formatter; }
  void SetFormatter(SettingControlSliderFormatter formatter) { m_formatter = formatter; }

protected:
  int m_heading;
  bool m_popup;
  int m_formatLabel;
  std::string m_formatString;
  SettingControlSliderFormatter m_formatter;
};

class CSettingControlRange : public ISettingControl
{
public:
  CSettingControlRange()
    : m_formatLabel(21469),
      m_valueFormatLabel(-1),
      m_valueFormat("%s")
  { }
  virtual ~CSettingControlRange() { }

  // implementation of ISettingControl
  virtual std::string GetType() const override { return "range"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false) override;
  virtual bool SetFormat(const std::string &format) override;

  int GetFormatLabel() const { return m_formatLabel; }
  void SetFormatLabel(int formatLabel) { m_formatLabel = formatLabel; }
  int GetValueFormatLabel() const { return m_valueFormatLabel; }
  void SetValueFormatLabel(int valueFormatLabel) { m_valueFormatLabel = valueFormatLabel; }
  const std::string& GetValueFormat() const { return m_valueFormat; }
  void SetValueFormat(const std::string &valueFormat) { m_valueFormat = valueFormat; }

protected:
  int m_formatLabel;
  int m_valueFormatLabel;
  std::string m_valueFormat;
};

class CSettingControlTitle : public ISettingControl
{
public:
  CSettingControlTitle()
    : m_separatorHidden(false),
      m_separatorBelowLabel(true)
  { }
  virtual ~CSettingControlTitle() { }

  // implementation of ISettingControl
  virtual std::string GetType() const override { return "title"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false) override;

  bool IsSeparatorHidden() const { return m_separatorHidden; }
  void SetSeparatorHidden(bool hidden) { m_separatorHidden = hidden; }
  bool IsSeparatorBelowLabel() const { return m_separatorBelowLabel; }
  void SetSeparatorBelowLabel(bool below) { m_separatorBelowLabel = below; }

protected:
  bool m_separatorHidden;
  bool m_separatorBelowLabel;
};

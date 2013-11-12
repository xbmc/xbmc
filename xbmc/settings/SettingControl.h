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

#define SETTING_XML_ELM_CONTROL_FORMATLABEL  "formatlabel"
#define SETTING_XML_ELM_CONTROL_HIDDEN       "hidden"
#define SETTING_XML_ELM_CONTROL_VERIFYNEW    "verifynew"
#define SETTING_XML_ELM_CONTROL_HEADING      "heading"
#define SETTING_XML_ELM_CONTROL_HIDEVALUE    "hidevalue"
#define SETTING_XML_ELM_CONTROL_MULTISELECT  "multiselect"

class CSettingControlCheckmark : public ISettingControl
{
public:
  CSettingControlCheckmark()
  {
    m_format = "boolean";
  }
  virtual ~CSettingControlCheckmark() { }

  // implementation of ISettingControl
  virtual std::string GetType() const { return "toggle"; }

protected:
  virtual bool SetFormat(const std::string &format);
};

class CSettingControlSpinner : public ISettingControl
{
public:
  CSettingControlSpinner()
    : m_formatLabel(-1),
      m_formatString("%i"),
      m_minimumLabel(-1)
  { }
  virtual ~CSettingControlSpinner() { }

  // implementation of ISettingControl
  virtual std::string GetType() const { return "spinner"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  int GetFormatLabel() const { return m_formatLabel; }
  const std::string& GetFormatString() const { return m_formatString; }
  int GetMinimumLabel() const { return m_minimumLabel; }

protected:
  virtual bool SetFormat(const std::string &format);

  int m_formatLabel;
  std::string m_formatString;
  int m_minimumLabel;

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
  virtual std::string GetType() const { return "edit"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  bool IsHidden() const { return m_hidden; }
  bool VerifyNewValue() const { return m_verifyNewValue; }
  int GetHeading() const { return m_heading; }

protected:
  virtual bool SetFormat(const std::string &format);

  bool m_hidden;
  bool m_verifyNewValue;
  int m_heading;
};

class CSettingControlButton : public ISettingControl
{
public:
  CSettingControlButton()
    : m_heading(-1),
      m_hideValue(false)
  { }
  virtual ~CSettingControlButton() { }

  // implementation of ISettingControl
  virtual std::string GetType() const { return "button"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  int GetHeading() const { return m_heading; }
  bool HideValue() const { return m_hideValue; }

protected:
  virtual bool SetFormat(const std::string &format);

  int m_heading;
  bool m_hideValue;
};

class CSettingControlList : public ISettingControl
{
public:
  CSettingControlList()
    : m_heading(-1),
      m_multiselect(false)
  { }
  virtual ~CSettingControlList() { }

  // implementation of ISettingControl
  virtual std::string GetType() const { return "list"; }
  virtual bool Deserialize(const TiXmlNode *node, bool update = false);
  
  int GetHeading() const { return m_heading; }
  bool CanMultiSelect() const { return m_multiselect; }

protected:
  virtual bool SetFormat(const std::string &format);
  
  int m_heading;
  bool m_multiselect;
};

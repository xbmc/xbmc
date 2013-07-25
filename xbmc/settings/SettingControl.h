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

#include <string>

class TiXmlNode;

typedef enum {
  SettingControlTypeNone =  0,
  SettingControlTypeCheckmark,
  SettingControlTypeSpinner,
  SettingControlTypeEdit,
  SettingControlTypeList,
  SettingControlTypeButton
} SettingControlType;

typedef enum {
  SettingControlFormatNone = 0,
  SettingControlFormatBoolean,
  SettingControlFormatString,
  SettingControlFormatInteger,
  SettingControlFormatNumber,
  SettingControlFormatIP,
  SettingControlFormatMD5,
  SettingControlFormatPath,
  SettingControlFormatAddon,
  SettingControlFormatAction
} SettingControlFormat;

typedef enum {
  SettingControlAttributeNone       = 0x0,
  SettingControlAttributeHidden     = 0x1,
  SettingControlAttributeVerifyNew  = 0x2
} SettingControlAttribute;

class CSettingControl
{
public:
  CSettingControl(SettingControlType type = SettingControlTypeNone,
                  SettingControlFormat format = SettingControlFormatNone,
                  SettingControlAttribute attribute = SettingControlAttributeNone)
    : m_type(type), m_format(format), m_attributes(attribute),
      m_delayed(false)
  { }
  virtual ~CSettingControl() { }

  bool Deserialize(const TiXmlNode *node, bool update = false);

  SettingControlType GetType() const { return m_type; }
  SettingControlFormat GetFormat() const { return m_format; }
  SettingControlAttribute GetAttributes() const { return m_attributes; }
  bool GetDelayed() const { return m_delayed; }

  void SetType(SettingControlType type) { m_type = type; }
  void SetFormat(SettingControlFormat format) { m_format = format; }
  void SetAttributes(SettingControlAttribute attributes) { m_attributes = attributes; }

protected:
  bool setType(const std::string &strType);
  bool setFormat(const std::string &strFormat);
  bool setAttributes(const std::string &strAttributes);

  SettingControlType m_type;
  SettingControlFormat m_format;
  SettingControlAttribute m_attributes;
  bool m_delayed;
};

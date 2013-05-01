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
#include <string>
#include <vector>

#include "SettingVisibility.h"

#define XML_SETTING     "setting"

#define XML_ATTR_ID     "id"
#define XML_ATTR_LABEL  "label"
#define XML_ATTR_HELP   "help"
#define XML_ATTR_TYPE   "type"

class CSettingsManager;
class TiXmlNode;

class ISetting
{
public:
  ISetting(const std::string &id, CSettingsManager *settingsManager = NULL);
  virtual ~ISetting() { }

  virtual bool Deserialize(const TiXmlNode *node, bool update = false);

  const std::string& GetId() const { return m_id; }
  bool IsVisible() const { return m_visible; }
  void CheckVisible();
  void SetVisible(bool visible) { m_visible = visible; }

  static bool DeserializeIdentification(const TiXmlNode *node, std::string &identification);

protected:
  std::string m_id;
  CSettingsManager *m_settingsManager;

private:
  bool m_visible;
  CSettingVisibility m_visibilityCondition;
};

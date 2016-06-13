#pragma once
/*
 *      Copyright (C) 2012-2014 Team XBMC
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
#include <string>
#include <utility>
#include <vector>

#include "dbwrappers/Database.h"
#include "dbwrappers/DatabaseQuery.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "utils/DatabaseUtils.h"

class CDbUrl;
class CSetting;
class CSmartPlaylist;
class CSmartPlaylistRule;

class CGUIDialogMediaFilter : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogMediaFilter();
  virtual ~CGUIDialogMediaFilter();

  // specializations of CGUIControl
  virtual bool OnMessage(CGUIMessage &message);

  static void ShowAndEditMediaFilter(const std::string &path, CSmartPlaylist &filter);

  typedef struct {
    std::string mediaType;
    Field field;
    uint32_t label;
    int settingType;
    std::string controlType;
    std::string controlFormat;
    CDatabaseQueryRule::SEARCH_OPERATOR ruleOperator;
    CSetting *setting;
    CSmartPlaylistRule *rule;
    void *data;
  } Filter;

protected:
  // specializations of CGUIWindow
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();

  // implementations of ISettingCallback
  virtual void OnSettingChanged(const CSetting *setting);

  // specialization of CGUIDialogSettingsBase
  virtual bool AllowResettingSettings() const { return false; }
  virtual void Save() { }
  virtual unsigned int GetDelayMs() const { return 500; }

  // specialization of CGUIDialogSettingsManualBase
  virtual void SetupView();
  virtual void InitializeSettings();

  bool SetPath(const std::string &path);
  void UpdateControls();
  void TriggerFilter() const;
  void Reset(bool filtersOnly = false);

  int GetItems(const Filter &filter, std::vector<std::string> &items, bool countOnly = false);
  void GetRange(const Filter &filter, int &min, int &interval, int &max);
  void GetRange(const Filter &filter, float &min, float &interval, float &max);
  bool GetMinMax(const std::string &table, const std::string &field, int &min, int &max, const CDatabase::Filter &filter = CDatabase::Filter());

  CSmartPlaylistRule* AddRule(Field field, CDatabaseQueryRule::SEARCH_OPERATOR ruleOperator = CDatabaseQueryRule::OPERATOR_CONTAINS);
  void DeleteRule(Field field);

  static void GetStringListOptions(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);

  CDbUrl* m_dbUrl;
  std::string m_mediaType;
  CSmartPlaylist *m_filter;
  std::map<std::string, Filter> m_filters;
};

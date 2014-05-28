#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#ifndef DIALOGS_DBURL_H_INCLUDED
#define DIALOGS_DBURL_H_INCLUDED
#include "DbUrl.h"
#endif

#ifndef DIALOGS_DBWRAPPERS_DATABASE_H_INCLUDED
#define DIALOGS_DBWRAPPERS_DATABASE_H_INCLUDED
#include "dbwrappers/Database.h"
#endif

#ifndef DIALOGS_PLAYLISTS_SMARTPLAYLIST_H_INCLUDED
#define DIALOGS_PLAYLISTS_SMARTPLAYLIST_H_INCLUDED
#include "playlists/SmartPlayList.h"
#endif

#ifndef DIALOGS_SETTINGS_DIALOGS_GUIDIALOGSETTINGS_H_INCLUDED
#define DIALOGS_SETTINGS_DIALOGS_GUIDIALOGSETTINGS_H_INCLUDED
#include "settings/dialogs/GUIDialogSettings.h"
#endif

#ifndef DIALOGS_THREADS_TIMER_H_INCLUDED
#define DIALOGS_THREADS_TIMER_H_INCLUDED
#include "threads/Timer.h"
#endif

#ifndef DIALOGS_UTILS_DATABASEUTILS_H_INCLUDED
#define DIALOGS_UTILS_DATABASEUTILS_H_INCLUDED
#include "utils/DatabaseUtils.h"
#endif

#ifndef DIALOGS_UTILS_STDSTRING_H_INCLUDED
#define DIALOGS_UTILS_STDSTRING_H_INCLUDED
#include "utils/StdString.h"
#endif


class CFileItemList;

class CGUIDialogMediaFilter : public CGUIDialogSettings, protected ITimerCallback
{
public:
  CGUIDialogMediaFilter();
  virtual ~CGUIDialogMediaFilter();

  virtual bool OnMessage(CGUIMessage& message);

  static void ShowAndEditMediaFilter(const std::string &path, CSmartPlaylist &filter);

  typedef struct {
    std::string mediaType;
    Field field;
    uint32_t label;
    SettingInfo::SETTING_TYPE type;
    CDatabaseQueryRule::SEARCH_OPERATOR ruleOperator;
    void *data;
    CSmartPlaylistRule *rule;
    int controlIndex;
  } Filter;

protected:
  virtual void OnWindowLoaded();

  virtual void CreateSettings();
  virtual void SetupPage();
  virtual void OnSettingChanged(SettingInfo &setting);

  virtual void OnTimeout();

  void Reset();
  bool SetPath(const std::string &path);
  void UpdateControls();
  void TriggerFilter() const;

  void OnBrowse(const Filter &filter, CFileItemList &items, bool countOnly = false);
  CSmartPlaylistRule* AddRule(Field field, CDatabaseQueryRule::SEARCH_OPERATOR ruleOperator = CDatabaseQueryRule::OPERATOR_CONTAINS);
  void DeleteRule(Field field);
  void GetRange(const Filter &filter, float &min, float &interval, float &max, RANGEFORMATFUNCTION &formatFunction);
  bool GetMinMax(const CStdString &table, const CStdString &field, float &min, float &max, const CDatabase::Filter &filter = CDatabase::Filter());

  static CStdString RangeAsFloat(float valueLower, float valueUpper, float minimum);
  static CStdString RangeAsInt(float valueLower, float valueUpper, float minimum);
  static CStdString RangeAsDate(float valueLower, float valueUpper, float minimum);
  static CStdString RangeAsTime(float valueLower, float valueUpper, float minimum);

  CDbUrl* m_dbUrl;
  std::string m_mediaType;
  CSmartPlaylist *m_filter;
  std::map<uint32_t, Filter> m_filters;
  CTimer *m_delayTimer;
};

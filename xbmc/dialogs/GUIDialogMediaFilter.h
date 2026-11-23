/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "dbwrappers/Database.h"
#include "dbwrappers/DatabaseQuery.h"
#include "media/LibraryQueryType.h"
#include "media/MediaType.h"
#include "settings/dialogs/GUIDialogSettingsManualBase.h"
#include "settings/lib/SettingType.h"
#include "utils/DatabaseUtils.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

class CDbUrl;
class CSetting;
namespace KODI::MEDIA
{
class CLibraryQuery;
class CLibraryQueryRule;
} // namespace KODI::MEDIA
struct StringSettingOption;

class CGUIDialogMediaFilter : public CGUIDialogSettingsManualBase
{
public:
  CGUIDialogMediaFilter();
  ~CGUIDialogMediaFilter() override;

  // specializations of CGUIControl
  bool OnMessage(CGUIMessage &message) override;

  static void ShowAndEditMediaFilter(const std::string& path, KODI::MEDIA::CLibraryQuery& filter);

  struct Filter
  {
    KODI::MEDIA::TYPE mediaType;
    Field field;
    uint32_t label;
    SettingType settingType;
    std::string controlType;
    std::string controlFormat;
    CDatabaseQueryRule::SearchOperator ruleOperator;
    std::shared_ptr<CSetting> setting = nullptr;
    KODI::MEDIA::CLibraryQueryRule* rule = nullptr;
    void* data = nullptr;
  };

protected:
  // specializations of CGUIWindow
  void OnWindowLoaded() override;
  void OnInitWindow() override;

  // implementations of ISettingCallback
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;

  // specialization of CGUIDialogSettingsBase
  bool AllowResettingSettings() const override { return false; }
  bool Save() override { return true; }
  std::chrono::milliseconds GetDelayMs() const override { return std::chrono::milliseconds(500); }

  // specialization of CGUIDialogSettingsManualBase
  void SetupView() override;
  void InitializeSettings() override;

  bool SetPath(const std::string &path);
  void UpdateControls();
  void TriggerFilter() const;
  void Reset(bool filtersOnly = false);

  int GetItems(const Filter &filter, std::vector<std::string> &items, bool countOnly = false);
  void GetRange(const Filter &filter, int &min, int &interval, int &max);
  void GetRange(const Filter &filter, float &min, float &interval, float &max);

  KODI::MEDIA::CLibraryQueryRule* AddRule(
      Field field,
      CDatabaseQueryRule::SearchOperator ruleOperator =
          CDatabaseQueryRule::SearchOperator::OPERATOR_CONTAINS);
  void DeleteRule(Field field);

  void GetStringListOptions(const std::shared_ptr<const CSetting>& setting,
                            std::vector<StringSettingOption>& list,
                            std::string& current);

  std::unique_ptr<CDbUrl> m_dbUrl;
  KODI::MEDIA::LIBRARY_QUERY::TYPE m_queryType{KODI::MEDIA::LIBRARY_QUERY::TYPE::SONGS};
  KODI::MEDIA::LIBRARY_QUERY::Source m_source{KODI::MEDIA::MediaSection::VIDEO,
                                              KODI::MEDIA::TYPE::NONE};
  KODI::MEDIA::CLibraryQuery* m_filter{nullptr};
  std::map<std::string, Filter> m_filters;
};

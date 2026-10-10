/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogMediaFilter.h"

#include "DbUrl.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "GUIUserMessages.h"
#include "ServiceBroker.h"
#include "XBDateTime.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbPaths.h"
#include "music/MusicDbUrl.h"
#include "playlists/SmartPlayList.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/ContentNames.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbPaths.h"
#include "video/VideoDbUrl.h"

#include <algorithm>

using namespace KODI;

#define CONTROL_HEADING             2

#define CONTROL_OKAY_BUTTON        28
#define CONTROL_CANCEL_BUTTON      29
#define CONTROL_CLEAR_BUTTON       30

#define CHECK_ALL                  -1
#define CHECK_NO                    0
#define CHECK_YES                   1
#define CHECK_LABEL_ALL           593
#define CHECK_LABEL_NO            106
#define CHECK_LABEL_YES           107

using enum CDatabaseQueryRule::SearchOperator;
using KODI::MEDIA::MediaTypeFromName;

namespace
{
bool IsVideoLibrary(MEDIA::TYPE type)
{
  switch (type)
  {
    case MEDIA::TYPE::MOVIE:
    case MEDIA::TYPE::TV_SHOW:
    case MEDIA::TYPE::EPISODE:
    case MEDIA::TYPE::MUSIC_VIDEO:
      return true;
    default:
      return false;
  }
}

bool IsMusicLibrary(MEDIA::TYPE type)
{
  switch (type)
  {
    case MEDIA::TYPE::ARTIST:
    case MEDIA::TYPE::ALBUM:
    case MEDIA::TYPE::SONG:
      return true;
    default:
      return false;
  }
}

VideoDbContentType VideoContentOf(MEDIA::TYPE type)
{
  switch (type)
  {
    case MEDIA::TYPE::TV_SHOW:
      return VideoDbContentType::TVSHOWS;
    case MEDIA::TYPE::EPISODE:
      return VideoDbContentType::EPISODES;
    case MEDIA::TYPE::MUSIC_VIDEO:
      return VideoDbContentType::MUSICVIDEOS;
    default:
      return VideoDbContentType::MOVIES;
  }
}

uint32_t HeadingOf(MEDIA::TYPE type)
{
  switch (type)
  {
    case MEDIA::TYPE::MOVIE:
      return 20342;
    case MEDIA::TYPE::TV_SHOW:
      return 20343;
    case MEDIA::TYPE::EPISODE:
      return 20360;
    case MEDIA::TYPE::MUSIC_VIDEO:
      return 20389;
    case MEDIA::TYPE::ARTIST:
      return 133;
    case MEDIA::TYPE::ALBUM:
      return 132;
    case MEDIA::TYPE::SONG:
      return 134;
    default:
      return 0;
  }
}
} // namespace

// clang-format off
static const CGUIDialogMediaFilter::Filter filterList[] = {
  {      MEDIA::TYPE::MOVIE,          Field::TITLE,   556,  SettingType::String,   "edit",   "string",   OPERATOR_CONTAINS },
  {      MEDIA::TYPE::MOVIE,         Field::RATING,   563,  SettingType::Number,  "range",  "number",   OPERATOR_BETWEEN },
  {      MEDIA::TYPE::MOVIE,    Field::USER_RATING, 38018, SettingType::Integer,  "range",  "integer",  OPERATOR_BETWEEN },
  {      MEDIA::TYPE::MOVIE,    Field::IN_PROGRESS,   575, SettingType::Integer, "toggle", "",         OPERATOR_FALSE },
  {      MEDIA::TYPE::MOVIE,           Field::YEAR,   562, SettingType::Integer,  "range",  "integer",  OPERATOR_BETWEEN },
  {      MEDIA::TYPE::MOVIE,            Field::TAG, 20459,    SettingType::List,   "list",   "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::MOVIE,          Field::GENRE,   515,    SettingType::List,   "list",   "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::MOVIE,          Field::ACTOR, 20337,    SettingType::List,   "list",   "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::MOVIE,       Field::DIRECTOR, 20339,    SettingType::List,   "list",   "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::MOVIE,         Field::STUDIO,   572,    SettingType::List,   "list",   "string",   OPERATOR_EQUALS },

  {     MEDIA::TYPE::TV_SHOW,          Field::TITLE,   556,  SettingType::String,   "edit",  "string",   OPERATOR_CONTAINS },
  {     MEDIA::TYPE::TV_SHOW,         Field::RATING,   563,  SettingType::Number,  "range",  "number",   OPERATOR_BETWEEN },
  {     MEDIA::TYPE::TV_SHOW,    Field::USER_RATING, 38018, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {     MEDIA::TYPE::TV_SHOW,    Field::IN_PROGRESS,   575, SettingType::Integer, "toggle",        "",         OPERATOR_FALSE },
  {     MEDIA::TYPE::TV_SHOW,           Field::YEAR,   562, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {     MEDIA::TYPE::TV_SHOW,            Field::TAG, 20459,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::TV_SHOW,          Field::GENRE,   515,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::TV_SHOW,          Field::ACTOR, 20337,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::TV_SHOW,       Field::DIRECTOR, 20339,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::TV_SHOW,         Field::STUDIO,   572,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },

  {    MEDIA::TYPE::EPISODE,          Field::TITLE,   556,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {    MEDIA::TYPE::EPISODE,         Field::RATING,   563,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  {    MEDIA::TYPE::EPISODE,    Field::USER_RATING, 38018, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {    MEDIA::TYPE::EPISODE,       Field::AIR_DATE, 20416, SettingType::Integer,  "range",    "date",  OPERATOR_BETWEEN },
  {    MEDIA::TYPE::EPISODE,    Field::IN_PROGRESS,   575, SettingType::Integer, "toggle",        "",    OPERATOR_FALSE },
  {    MEDIA::TYPE::EPISODE,          Field::ACTOR, 20337,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {    MEDIA::TYPE::EPISODE,       Field::DIRECTOR, 20339,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },

  { MEDIA::TYPE::MUSIC_VIDEO,          Field::TITLE,   556,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  { MEDIA::TYPE::MUSIC_VIDEO,         Field::RATING,   563,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  { MEDIA::TYPE::MUSIC_VIDEO,    Field::USER_RATING, 38018, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  { MEDIA::TYPE::MUSIC_VIDEO,         Field::ARTIST,   557,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,          Field::ALBUM,   558,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,           Field::YEAR,   562, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  { MEDIA::TYPE::MUSIC_VIDEO,            Field::TAG, 20459,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,          Field::GENRE,   515,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,       Field::DIRECTOR, 20339,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,         Field::STUDIO,   572,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },

  {     MEDIA::TYPE::ARTIST,         Field::ARTIST,   557,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,         Field::SOURCE, 39030,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::ARTIST,          Field::GENRE,   515,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::ARTIST,          Field::MOODS,   175,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,         Field::STYLES,   176,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,    Field::INSTRUMENTS, 21892,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,    Field::ARTIST_TYPE,   564,  SettingType::String,   "edit",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::ARTIST,         Field::GENDER, 39025,  SettingType::String,   "edit",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::ARTIST, Field::DISAMBIGUATION, 39026,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,      Field::BIOGRAPHY, 21887,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,           Field::BORN, 21893,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,    Field::BAND_FORMED, 21894,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,      Field::DISBANDED, 21896,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::ARTIST,           Field::DIED, 21897,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },

  {      MEDIA::TYPE::ALBUM,          Field::ALBUM,   556,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ALBUM,     Field::DISC_TITLE, 38076,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ALBUM,   Field::ALBUM_ARTIST,   566,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ALBUM,         Field::SOURCE, 39030,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ALBUM,         Field::RATING,   563,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  {      MEDIA::TYPE::ALBUM,    Field::USER_RATING, 38018, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {      MEDIA::TYPE::ALBUM,     Field::ALBUM_TYPE,   564,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ALBUM,           Field::YEAR,   562, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {      MEDIA::TYPE::ALBUM,          Field::GENRE,   515,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ALBUM,    Field::MUSIC_LABEL, 21899,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ALBUM,    Field::COMPILATION,   204, SettingType::Boolean, "toggle",        "",    OPERATOR_FALSE },
  {      MEDIA::TYPE::ALBUM,      Field::IS_BOXSET, 38074, SettingType::Boolean, "toggle",        "",    OPERATOR_FALSE },
  {      MEDIA::TYPE::ALBUM,      Field::ORIG_YEAR, 38078,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },

  {       MEDIA::TYPE::SONG,          Field::TITLE,   556,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {       MEDIA::TYPE::SONG,          Field::ALBUM,   558,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::SONG,     Field::DISC_TITLE, 38076,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {       MEDIA::TYPE::SONG,         Field::ARTIST,   557,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::SONG,           Field::TIME,   180, SettingType::Integer,  "range",    "time",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::SONG,         Field::RATING,   563,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::SONG,    Field::USER_RATING, 38018, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::SONG,           Field::YEAR,   562, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::SONG,          Field::GENRE,   515,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::SONG,      Field::PLAYCOUNT,   567, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::SONG,         Field::SOURCE, 39030,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS }
};
// clang-format on

CGUIDialogMediaFilter::CGUIDialogMediaFilter()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_MEDIA_FILTER, "DialogSettings.xml")
{ }

CGUIDialogMediaFilter::~CGUIDialogMediaFilter()
{
  Reset();
}

bool CGUIDialogMediaFilter::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      if (message.GetSenderId()== CONTROL_CLEAR_BUTTON)
      {
        m_filter->Reset();
        m_filter->SetType(m_mediaType);

        for (auto& filter : m_filters)
        {
          filter.second.rule = nullptr;
          filter.second.setting->Reset();
        }

        TriggerFilter();
        return true;
      }
      break;
    }

    case GUI_MSG_REFRESH_LIST:
    {
      TriggerFilter();
      UpdateControls();
      break;
    }

    case GUI_MSG_WINDOW_DEINIT:
    {
      Reset();
      break;
    }

    default:
      break;
  }

  return CGUIDialogSettingsManualBase::OnMessage(message);
}

void CGUIDialogMediaFilter::ShowAndEditMediaFilter(const std::string& path,
                                                   PLAYLIST::CSmartPlaylist& filter)
{
  CGUIDialogMediaFilter *dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogMediaFilter>(WINDOW_DIALOG_MEDIA_FILTER);
  if (dialog == nullptr)
    return;

  // initialize and show the dialog
  dialog->Initialize();
  dialog->m_filter = &filter;

  // must be called after setting the filter/smartplaylist
  if (!dialog->SetPath(path))
    return;

  dialog->Open();
}

void CGUIDialogMediaFilter::OnWindowLoaded()
{
  CGUIDialogSettingsManualBase::OnWindowLoaded();

  // we don't need the cancel button so let's hide it
  SET_CONTROL_HIDDEN(CONTROL_CANCEL_BUTTON);
}

void CGUIDialogMediaFilter::OnInitWindow()
{
  CGUIDialogSettingsManualBase::OnInitWindow();

  UpdateControls();
}

void CGUIDialogMediaFilter::OnSettingChanged(const std::shared_ptr<const CSetting>& setting)
{
  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  std::map<std::string, Filter>::iterator it = m_filters.find(setting->GetId());
  if (it == m_filters.end())
    return;

  bool remove = false;
  Filter& filter = it->second;

  if (filter.controlType == "edit")
  {
    std::string value = setting->ToString();
    if (!value.empty())
    {
      if (filter.rule == nullptr)
        filter.rule = AddRule(filter.field, filter.ruleOperator);
      filter.rule->m_parameter.clear();
      filter.rule->m_parameter.push_back(value);
    }
    else
      remove = true;
  }
  else if (filter.controlType == "toggle")
  {
    int choice = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    if (choice > CHECK_ALL)
    {
      const CDatabaseQueryRule::SearchOperator ruleOperator =
          choice == CHECK_YES ? OPERATOR_TRUE : OPERATOR_FALSE;
      if (filter.rule == nullptr)
        filter.rule = AddRule(filter.field, ruleOperator);
      else
        filter.rule->m_operator = ruleOperator;
    }
    else
      remove = true;
  }
  else if (filter.controlType == "list")
  {
    std::vector<CVariant> values = CSettingUtils::GetList(std::static_pointer_cast<const CSettingList>(setting));
    if (!values.empty())
    {
      if (filter.rule == nullptr)
        filter.rule = AddRule(filter.field, filter.ruleOperator);

      filter.rule->m_parameter.clear();
      for (const auto& itValue : values)
        filter.rule->m_parameter.push_back(itValue.asString());
    }
    else
      remove = true;
  }
  else if (filter.controlType == "range")
  {
    const std::shared_ptr<const CSettingList> settingList = std::static_pointer_cast<const CSettingList>(setting);
    std::vector<CVariant> values = CSettingUtils::GetList(settingList);
    if (values.size() != 2)
      return;

    std::string strValueLower, strValueUpper;

    SettingConstPtr definition = settingList->GetDefinition();
    if (definition->GetType() == SettingType::Integer)
    {
      const std::shared_ptr<const CSettingInt> definitionInt = std::static_pointer_cast<const CSettingInt>(definition);
      int valueLower = static_cast<int>(values.at(0).asInteger());
      int valueUpper = static_cast<int>(values.at(1).asInteger());

      if (valueLower > definitionInt->GetMinimum() ||
          valueUpper < definitionInt->GetMaximum())
      {
        if (filter.controlFormat == "date")
        {
          strValueLower = CDateTime(static_cast<time_t>(valueLower)).GetAsDBDate();
          strValueUpper = CDateTime(static_cast<time_t>(valueUpper)).GetAsDBDate();
        }
        else
        {
          strValueLower = values.at(0).asString();
          strValueUpper = values.at(1).asString();
        }
      }
    }
    else if (definition->GetType() == SettingType::Number)
    {
      const std::shared_ptr<const CSettingNumber> definitionNumber = std::static_pointer_cast<const CSettingNumber>(definition);
      float valueLower = values.at(0).asFloat();
      float valueUpper = values.at(1).asFloat();

      if (static_cast<double>(valueLower) > definitionNumber->GetMinimum() ||
          static_cast<double>(valueUpper) < definitionNumber->GetMaximum())
      {
        strValueLower = values.at(0).asString();
        strValueUpper = values.at(1).asString();
      }
    }
    else
      return;

    if (!strValueLower.empty() && !strValueUpper.empty())
    {
      // prepare the filter rule
      if (filter.rule == nullptr)
        filter.rule = AddRule(filter.field, filter.ruleOperator);
      filter.rule->m_parameter.clear();

      filter.rule->m_parameter.push_back(strValueLower);
      filter.rule->m_parameter.push_back(strValueUpper);
    }
    else
      remove = true;
  }
  else
    return;

  // we need to remove the existing rule for the title
  if (remove && filter.rule != nullptr)
  {
    DeleteRule(filter.field);
    filter.rule = nullptr;
  }

  CGUIMessage msg(GUI_MSG_REFRESH_LIST, GetID(), 0);
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, WINDOW_DIALOG_MEDIA_FILTER);
}

void CGUIDialogMediaFilter::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  auto& localizeStrings{CServiceBroker::GetResourcesComponent().GetLocalizeStrings()};
  SET_CONTROL_LABEL(CONTROL_HEADING, StringUtils::Format(localizeStrings.Get(1275),
                                                         localizeStrings.Get(HeadingOf(m_type))));

  SET_CONTROL_LABEL(CONTROL_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_CLEAR_BUTTON, 192);
}

void CGUIDialogMediaFilter::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  if (m_filter == nullptr)
    return;

  Reset(true);

  int handledRules = 0;

  const std::shared_ptr<CSettingCategory> category = AddCategory("filter", -1);
  if (category == nullptr)
  {
    CLog::Log(LOGERROR, "CGUIDialogMediaFilter: unable to setup filters");
    return;
  }

  const std::shared_ptr<CSettingGroup> group = AddGroup(category);
  if (group == nullptr)
  {
    CLog::Log(LOGERROR, "CGUIDialogMediaFilter: unable to setup filters");
    return;
  }

  for (const Filter& f : filterList)
  {
    if (f.mediaType != m_type)
      continue;

    Filter filter = f;

    // check the smartplaylist if it contains a matching rule
    for (const auto& rule : m_filter->m_ruleCombination.GetRules())
    {
      if (rule->m_field == static_cast<int>(filter.field))
      {
        filter.rule = static_cast<PLAYLIST::CSmartPlaylistRule*>(rule.get());
        handledRules++;
        break;
      }
    }

    std::string settingId =
        StringUtils::Format("filter.{}.{}", PluralNameOf(filter.mediaType),
                            static_cast<int>(filter.field));
    if (filter.controlType == "edit")
    {
      CVariant data;
      if (filter.rule != nullptr && filter.rule->m_parameter.size() == 1)
        data = filter.rule->m_parameter.at(0);

      if (filter.settingType == SettingType::String)
        filter.setting = AddEdit(group, settingId, filter.label, SettingLevel::Basic, data.asString(), true, false, filter.label, true);
      else if (filter.settingType == SettingType::Integer)
        filter.setting = AddEdit(group, settingId, filter.label, SettingLevel::Basic, static_cast<int>(data.asInteger()), 0, 1, 0, false,  static_cast<int>(filter.label), true);
      else if (filter.settingType == SettingType::Number)
        filter.setting = AddEdit(group, settingId, filter.label, SettingLevel::Basic, data.asFloat(), 0.0f, 1.0f, 0.0f, false, filter.label, true);
    }
    else if (filter.controlType == "toggle")
    {
      int value = CHECK_ALL;
      if (filter.rule != nullptr)
        value = filter.rule->m_operator == OPERATOR_TRUE ? CHECK_YES : CHECK_NO;

      TranslatableIntegerSettingOptions entries;
      entries.emplace_back(CHECK_LABEL_ALL, CHECK_ALL);
      entries.emplace_back(CHECK_LABEL_NO, CHECK_NO);
      entries.emplace_back(CHECK_LABEL_YES, CHECK_YES);

      filter.setting = AddSpinner(group, settingId, filter.label, SettingLevel::Basic, value, entries, true);
    }
    else if (filter.controlType == "list")
    {
      std::vector<std::string> values;
      if (filter.rule != nullptr && !filter.rule->m_parameter.empty())
      {
        values = StringUtils::Split(filter.rule->GetParameter(), DATABASEQUERY_RULE_VALUE_SEPARATOR);
        if (values.size() == 1 && values.at(0).empty())
          values.erase(values.begin());
      }

      filter.setting = AddList(
          group, settingId, filter.label, SettingLevel::Basic, values,
          [this](const std::shared_ptr<const CSetting>& setting,
                 std::vector<StringSettingOption>& list, std::string& current)
          { GetStringListOptions(setting, list, current); },
          filter.label);
    }
    else if (filter.controlType == "range")
    {
      CVariant valueLower, valueUpper;
      if (filter.rule != nullptr)
      {
        if (filter.rule->m_parameter.size() == 2)
        {
          valueLower = filter.rule->m_parameter.at(0);
          valueUpper = filter.rule->m_parameter.at(1);
        }
        else
        {
          DeleteRule(filter.field);
          filter.rule = nullptr;
        }
      }

      if (filter.settingType == SettingType::Integer)
      {
        int min = 0;
        int interval = 0;
        int max = 0;
        GetRange(filter, min, interval, max);

        // don't create the filter if there's no real range
        if (min == max)
          continue;

        int iValueLower = valueLower.isNull() ? min : static_cast<int>(valueLower.asInteger());
        int iValueUpper = valueUpper.isNull() ? max : static_cast<int>(valueUpper.asInteger());

        if (filter.controlFormat == "integer")
          filter.setting = AddRange(group, settingId, filter.label, SettingLevel::Basic, iValueLower, iValueUpper, min, interval, max, -1, 21469, true);
        else if (filter.controlFormat == "percentage")
          filter.setting = AddPercentageRange(group, settingId, filter.label, SettingLevel::Basic, iValueLower, iValueUpper, -1, 1, 21469, true);
        else if (filter.controlFormat == "date")
          filter.setting = AddDateRange(group, settingId, filter.label, SettingLevel::Basic, iValueLower, iValueUpper, min, interval, max, -1, 21469, true);
        else if (filter.controlFormat == "time")
          filter.setting = AddTimeRange(group, settingId, filter.label, SettingLevel::Basic, iValueLower, iValueUpper, min, interval, max, -1, 21469, true);
      }
      else if (filter.settingType == SettingType::Number)
      {
        float min = 0;
        float interval = 0;
        float max = 0;
        GetRange(filter, min, interval, max);

        // don't create the filter if there's no real range
        if (min == max)
          continue;

        float fValueLower = valueLower.isNull() ? min : valueLower.asFloat();
        float fValueUpper = valueUpper.isNull() ? max : valueUpper.asFloat();

        filter.setting = AddRange(group, settingId, filter.label, SettingLevel::Basic, fValueLower, fValueUpper, min, interval, max, -1, 21469, true);
      }
    }
    else
    {
      if (filter.rule != nullptr)
        handledRules--;

      CLog::Log(LOGWARNING,
                "CGUIDialogMediaFilter: filter {} of media type {} with unknown control type '{}'",
                static_cast<int>(filter.field), filter.mediaType, filter.controlType);
      continue;
    }

    if (filter.setting == nullptr)
    {
      if (filter.rule != nullptr)
        handledRules--;

      CLog::Log(LOGWARNING,
                "CGUIDialogMediaFilter: failed to create filter {} of media type {} with control "
                "type '{}'",
                static_cast<int>(filter.field), filter.mediaType, filter.controlType);
      continue;
    }

    m_filters.insert(make_pair(settingId, filter));
  }

  // make sure that no change in capacity size is needed when adding new rules
  // which would copy around the rules and our pointers in the Filter struct
  // wouldn't work anymore
  m_filter->m_ruleCombination.Reserve(
      m_filters.size() + (m_filter->m_ruleCombination.GetRulesAmount() - handledRules));
}

bool CGUIDialogMediaFilter::SetPath(const std::string &path)
{
  if (path.empty() || m_filter == nullptr)
  {
    CLog::Log(LOGWARNING, "CGUIDialogMediaFilter::SetPath({}): invalid path or filter", path);
    return false;
  }

  const bool video{path.starts_with(VIDEO::DB_PATH::ROOT)};
  if (video)
    m_dbUrl = std::make_unique<CVideoDbUrl>();
  else if (path.starts_with(MUSIC::DB_PATH::ROOT))
    m_dbUrl = std::make_unique<CMusicDbUrl>();
  else
  {
    m_dbUrl.reset();
    CLog::Log(
        LOGWARNING,
        "CGUIDialogMediaFilter::SetPath({}): invalid path (neither videodb:// nor musicdb://)",
        path);
    return false;
  }

  if (!m_dbUrl->FromString(path) ||
      !(video ? IsVideoLibrary : IsMusicLibrary)(MediaTypeFromName(m_dbUrl->GetType())))
  {
    CLog::Log(LOGWARNING, "CGUIDialogMediaFilter::SetPath({}): invalid media type", path);
    return false;
  }

  // remove "filter" option
  if (m_dbUrl->HasOption("filter"))
    m_dbUrl->RemoveOption("filter");

  if (video)
    m_mediaType = static_cast<const CVideoDbUrl&>(*m_dbUrl).GetItemType();
  else
    m_mediaType = m_dbUrl->GetType();
  m_type = MediaTypeFromName(m_mediaType);

  m_filter->SetType(m_mediaType);
  return true;
}

void CGUIDialogMediaFilter::UpdateControls()
{
  for (const auto& itFilter : m_filters)
  {
    if (itFilter.second.controlType != "list")
      continue;

    std::vector<std::string> items;
    int size = GetItems(itFilter.second, items, true);

    std::string label =
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(itFilter.second.label);
    BaseSettingControlPtr control = GetSettingControl(itFilter.second.setting->GetId());
    if (control == nullptr)
      continue;

    if (size <= 0 ||
        (size == 1 && itFilter.second.field != Field::SET && itFilter.second.field != Field::TAG))
      CONTROL_DISABLE(control->GetID());
    else
    {
      CONTROL_ENABLE(control->GetID());
      label = StringUtils::Format(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21470), label, size);
    }
    SET_CONTROL_LABEL(control->GetID(), label);
  }
}

void CGUIDialogMediaFilter::TriggerFilter() const
{
  if (m_filter == nullptr)
    return;

  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_FILTER_ITEMS, 10); // 10 for advanced
  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(message);
}

void CGUIDialogMediaFilter::Reset(bool filtersOnly /* = false */)
{
  if (!filtersOnly)
    m_dbUrl.reset();

  m_filters.clear();
}

int CGUIDialogMediaFilter::GetItems(const Filter &filter, std::vector<std::string> &items, bool countOnly /* = false */)
{
  CFileItemList selectItems;

  // remove the rule for the field of the filter we want to retrieve items for
  PLAYLIST::CSmartPlaylist tmpFilter = *m_filter;

  auto it = std::ranges::find_if(tmpFilter.m_ruleCombination.GetRules(), [&filter](const auto& rule)
                                 { return static_cast<int>(filter.field) == rule->m_field; });
  if (it != tmpFilter.m_ruleCombination.GetRules().cend())
    tmpFilter.m_ruleCombination.RemoveRule(*it);

  const std::string baseDir{m_dbUrl->ToString()};
  std::set<std::string, std::less<>> playlists;
  CDatabase::Filter dbfilter;

  if (IsVideoLibrary(m_type))
  {
    CVideoDatabase videodb;
    if (!videodb.Open())
      return -1;

    dbfilter.where = tmpFilter.GetWhereClause(videodb, playlists);
    const VideoDbContentType type{VideoContentOf(m_type)};

    switch (filter.field)
    {
      case Field::GENRE:
        videodb.GetGenresNav(baseDir, selectItems, type, dbfilter, countOnly);
        break;
      case Field::ACTOR:
      case Field::ARTIST:
        videodb.GetActorsNav(baseDir, selectItems, type, dbfilter, countOnly);
        break;
      case Field::DIRECTOR:
        videodb.GetDirectorsNav(baseDir, selectItems, type, dbfilter, countOnly);
        break;
      case Field::STUDIO:
        videodb.GetStudiosNav(baseDir, selectItems, type, dbfilter, countOnly);
        break;
      case Field::ALBUM:
        videodb.GetMusicVideoAlbumsNav(baseDir, selectItems, dbfilter, countOnly);
        break;
      case Field::TAG:
        videodb.GetTagsNav(baseDir, selectItems, type, dbfilter, countOnly);
        break;
      default:
        break;
    }
  }
  else if (IsMusicLibrary(m_type))
  {
    CMusicDatabase musicdb;
    if (!musicdb.Open())
      return -1;

    dbfilter.where = tmpFilter.GetWhereClause(musicdb, playlists);

    switch (filter.field)
    {
      case Field::GENRE:
        musicdb.GetGenresNav(baseDir, selectItems, dbfilter, countOnly);
        break;
      case Field::ARTIST:
      case Field::ALBUM_ARTIST:
        musicdb.GetArtistsNav(baseDir, selectItems, SortDescription(), m_type == MEDIA::TYPE::ALBUM,
                              -1, -1, -1, dbfilter, countOnly);
        break;
      case Field::ALBUM:
        musicdb.GetAlbumsNav(baseDir, selectItems, SortDescription(), -1, -1, dbfilter, countOnly);
        break;
      case Field::ALBUM_TYPE:
        musicdb.GetAlbumTypesNav(baseDir, selectItems, dbfilter, countOnly);
        break;
      case Field::MUSIC_LABEL:
        musicdb.GetMusicLabelsNav(baseDir, selectItems, dbfilter, countOnly);
        break;
      case Field::SOURCE:
        musicdb.GetSourcesNav(baseDir, selectItems, dbfilter, countOnly);
        break;
      default:
        break;
    }
  }

  int size = selectItems.Size();
  if (size <= 0)
    return 0;

  if (countOnly)
  {
    if (size == 1 && selectItems.Get(0)->HasProperty("total"))
      return (int)selectItems.Get(0)->GetProperty("total").asInteger();
    return 0;
  }

  // sort the items
  selectItems.Sort(SortBy::LABEL, SortOrder::ASCENDING);

  for (int index = 0; index < size; ++index)
    items.push_back(selectItems.Get(index)->GetLabel());

  return items.size();
}

PLAYLIST::CSmartPlaylistRule* CGUIDialogMediaFilter::AddRule(
    Field field, CDatabaseQueryRule::SearchOperator ruleOperator /* = OPERATOR_CONTAINS */)
{
  const auto rule{std::make_shared<PLAYLIST::CSmartPlaylistRule>()};
  rule->m_field = static_cast<int>(field);
  rule->m_operator = ruleOperator;

  m_filter->m_ruleCombination.AddRule(rule);
  return static_cast<PLAYLIST::CSmartPlaylistRule*>(
      m_filter->m_ruleCombination.GetRules().back().get());
}

void CGUIDialogMediaFilter::DeleteRule(Field field)
{
  for (const auto& rule : m_filter->m_ruleCombination.GetRules())
  {
    if (rule->m_field == static_cast<int>(field))
    {
      m_filter->m_ruleCombination.RemoveRule(rule);
      break;
    }
  }
}

void CGUIDialogMediaFilter::GetStringListOptions(const SettingConstPtr& setting,
                                                 std::vector<StringSettingOption>& list,
                                                 std::string& /*current*/)
{
  if (!setting)
    return;

  auto itFilter = m_filters.find(setting->GetId());
  if (itFilter == m_filters.end())
    return;

  std::vector<std::string> items;
  if (GetItems(itFilter->second, items, false) <= 0)
    return;

  for (const auto& item : items)
    list.emplace_back(item, item);
}

void CGUIDialogMediaFilter::GetRange(const Filter &filter, int &min, int &interval, int &max)
{
  min = 0;
  interval = 1;
  max = 0;

  switch (filter.field)
  {
    case Field::USER_RATING:
      max = 10;
      break;

    case Field::YEAR:
    {
      std::string table;
      std::string select;
      std::string where;
      switch (m_type)
      {
        case MEDIA::TYPE::MOVIE:
          table = "movie_view";
          select = DatabaseUtils::GetField(Field::YEAR, m_type, DatabaseQueryPart::WHERE);
          break;
        case MEDIA::TYPE::TV_SHOW:
          table = "tvshow_view";
          select = StringUtils::Format(
              "strftime(\"%%Y\", {})",
              DatabaseUtils::GetField(Field::YEAR, m_type, DatabaseQueryPart::WHERE));
          break;
        case MEDIA::TYPE::MUSIC_VIDEO:
          table = "musicvideo_view";
          select = DatabaseUtils::GetField(Field::YEAR, m_type, DatabaseQueryPart::WHERE);
          break;
        case MEDIA::TYPE::ALBUM:
          table = "albumview";
          select = DatabaseUtils::GetField(Field::YEAR, m_type, DatabaseQueryPart::SELECT);
          where = DatabaseUtils::GetField(Field::YEAR, m_type, DatabaseQueryPart::WHERE);
          break;
        case MEDIA::TYPE::SONG:
          table = "songview";
          select = DatabaseUtils::GetField(Field::YEAR, m_type, DatabaseQueryPart::SELECT);
          where = DatabaseUtils::GetField(Field::YEAR, m_type, DatabaseQueryPart::WHERE);
          break;
        default:
          return;
      }

      CDatabase::Filter yearFilter;
      yearFilter.where = (where.empty() ? select : where) + " > 0";
      GetMinMax(table, select, min, max, yearFilter);
      break;
    }

    case Field::AIR_DATE:
      if (m_type == MEDIA::TYPE::EPISODE)
      {
        const std::string name =
            DatabaseUtils::GetField(Field::AIR_DATE, m_type, DatabaseQueryPart::SELECT);
        GetMinMax("episode_view",
                  StringUtils::Format("CAST(strftime(\"%%s\", {}) AS INTEGER)", name), min, max);
        interval = 60 * 60 * 24 * 7; // 1 week
      }
      break;

    case Field::TIME:
      interval = 10;
      if (m_type == MEDIA::TYPE::SONG)
        GetMinMax("songview", "iDuration", min, max);
      break;

    case Field::PLAYCOUNT:
      if (m_type == MEDIA::TYPE::SONG)
        GetMinMax("songview", "iTimesPlayed", min, max);
      break;

    default:
      break;
  }
}

void CGUIDialogMediaFilter::GetRange(const Filter &filter, float &min, float &interval, float &max)
{
  if (filter.field == Field::RATING)
  {
    min = 0.0f;
    interval = 0.1f;
    max = 10.0f;
  }
}

bool CGUIDialogMediaFilter::GetMinMax(const std::string &table, const std::string &field, int &min, int &max, const CDatabase::Filter &filter /* = CDatabase::Filter() */)
{
  if (table.empty() || field.empty())
    return false;

  std::unique_ptr<CDatabase> db;
  std::unique_ptr<CDbUrl> dbUrl;
  if (IsVideoLibrary(m_type))
  {
    auto videodb{std::make_unique<CVideoDatabase>()};
    if (!videodb->Open())
      return false;
    db = std::move(videodb);
    dbUrl = std::make_unique<CVideoDbUrl>();
  }
  else if (IsMusicLibrary(m_type))
  {
    auto musicdb{std::make_unique<CMusicDatabase>()};
    if (!musicdb->Open())
      return false;
    db = std::move(musicdb);
    dbUrl = std::make_unique<CMusicDbUrl>();
  }
  else
    return false;

  CDatabase::Filter extFilter = filter;
  std::string strSQLExtra;
  if (!db->BuildSQL(m_dbUrl->ToString(), strSQLExtra, extFilter, strSQLExtra, *dbUrl))
    return false;

  const std::string prepField = db->PrepareSQL(field);
  const std::string strSQL = "SELECT %s FROM %s ";

  const auto aggregate = [&](const std::string& function)
  {
    const std::string sql =
        db->PrepareSQL(strSQL, (function + "(" + prepField + ")").c_str(), table.c_str()) +
        strSQLExtra;
    return static_cast<int>(strtol(db->GetSingleValue(sql).c_str(), nullptr, 0));
  };
  min = aggregate("MIN");
  max = aggregate("MAX");

  db->Close();
  return true;
}

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
#include "media/LibraryQuery.h"
#include "media/LibraryQueryDatabase.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/lib/SettingDefinitions.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/SortUtils.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

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

namespace
{
//! In the label column, the label the field has everywhere
constexpr uint32_t FIELD_LABEL = 0;
} // namespace

// clang-format off
static const CGUIDialogMediaFilter::Filter filterList[] = {
  {       MEDIA::TYPE::MOVIE,          Field::TITLE, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {       MEDIA::TYPE::MOVIE,         Field::RATING, FIELD_LABEL,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::MOVIE,    Field::USER_RATING, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::MOVIE,    Field::IN_PROGRESS, FIELD_LABEL, SettingType::Integer, "toggle",        "",    OPERATOR_FALSE },
  {       MEDIA::TYPE::MOVIE,           Field::YEAR, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::MOVIE,            Field::TAG, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::MOVIE,          Field::GENRE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::MOVIE,          Field::ACTOR, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::MOVIE,       Field::DIRECTOR, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::MOVIE,         Field::STUDIO, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },

  {     MEDIA::TYPE::TV_SHOW,          Field::TITLE, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::TV_SHOW,         Field::RATING, FIELD_LABEL,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  {     MEDIA::TYPE::TV_SHOW,    Field::USER_RATING, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {     MEDIA::TYPE::TV_SHOW,    Field::IN_PROGRESS, FIELD_LABEL, SettingType::Integer, "toggle",        "",    OPERATOR_FALSE },
  {     MEDIA::TYPE::TV_SHOW,           Field::YEAR, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {     MEDIA::TYPE::TV_SHOW,            Field::TAG, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::TV_SHOW,          Field::GENRE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::TV_SHOW,          Field::ACTOR, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::TV_SHOW,       Field::DIRECTOR, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::TV_SHOW,         Field::STUDIO, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },

  {     MEDIA::TYPE::EPISODE,          Field::TITLE, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {     MEDIA::TYPE::EPISODE,         Field::RATING, FIELD_LABEL,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  {     MEDIA::TYPE::EPISODE,    Field::USER_RATING, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {     MEDIA::TYPE::EPISODE,       Field::AIR_DATE, FIELD_LABEL, SettingType::Integer,  "range",    "date",  OPERATOR_BETWEEN },
  {     MEDIA::TYPE::EPISODE,    Field::IN_PROGRESS, FIELD_LABEL, SettingType::Integer, "toggle",        "",    OPERATOR_FALSE },
  {     MEDIA::TYPE::EPISODE,          Field::ACTOR, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {     MEDIA::TYPE::EPISODE,       Field::DIRECTOR, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },

  { MEDIA::TYPE::MUSIC_VIDEO,          Field::TITLE, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  { MEDIA::TYPE::MUSIC_VIDEO,         Field::RATING, FIELD_LABEL,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  { MEDIA::TYPE::MUSIC_VIDEO,    Field::USER_RATING, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  { MEDIA::TYPE::MUSIC_VIDEO,         Field::ARTIST, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,          Field::ALBUM, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,           Field::YEAR, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  { MEDIA::TYPE::MUSIC_VIDEO,            Field::TAG, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,          Field::GENRE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,       Field::DIRECTOR, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  { MEDIA::TYPE::MUSIC_VIDEO,         Field::STUDIO, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },

  {      MEDIA::TYPE::ARTIST,         Field::ARTIST, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,         Field::SOURCE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ARTIST,          Field::GENRE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ARTIST,          Field::MOODS, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,         Field::STYLES, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,    Field::INSTRUMENTS, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,    Field::ARTIST_TYPE, FIELD_LABEL,  SettingType::String,   "edit",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ARTIST,         Field::GENDER, FIELD_LABEL,  SettingType::String,   "edit",  "string",   OPERATOR_EQUALS },
  {      MEDIA::TYPE::ARTIST, Field::DISAMBIGUATION, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,      Field::BIOGRAPHY, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,           Field::BORN, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,    Field::BAND_FORMED, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,      Field::DISBANDED, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {      MEDIA::TYPE::ARTIST,           Field::DIED, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },

  {       MEDIA::TYPE::ALBUM,          Field::ALBUM,         556,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {       MEDIA::TYPE::ALBUM,     Field::DISC_TITLE, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {       MEDIA::TYPE::ALBUM,   Field::ALBUM_ARTIST, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::ALBUM,         Field::SOURCE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::ALBUM,         Field::RATING, FIELD_LABEL,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::ALBUM,    Field::USER_RATING, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::ALBUM,     Field::ALBUM_TYPE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::ALBUM,           Field::YEAR, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {       MEDIA::TYPE::ALBUM,          Field::GENRE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::ALBUM,    Field::MUSIC_LABEL, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {       MEDIA::TYPE::ALBUM,    Field::COMPILATION, FIELD_LABEL, SettingType::Boolean, "toggle",        "",    OPERATOR_FALSE },
  {       MEDIA::TYPE::ALBUM,      Field::IS_BOXSET, FIELD_LABEL, SettingType::Boolean, "toggle",        "",    OPERATOR_FALSE },
  {       MEDIA::TYPE::ALBUM,      Field::ORIG_YEAR, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },

  {        MEDIA::TYPE::SONG,          Field::TITLE, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {        MEDIA::TYPE::SONG,          Field::ALBUM, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {        MEDIA::TYPE::SONG,     Field::DISC_TITLE, FIELD_LABEL,  SettingType::String,   "edit",  "string", OPERATOR_CONTAINS },
  {        MEDIA::TYPE::SONG,         Field::ARTIST, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {        MEDIA::TYPE::SONG,           Field::TIME, FIELD_LABEL, SettingType::Integer,  "range",    "time",  OPERATOR_BETWEEN },
  {        MEDIA::TYPE::SONG,         Field::RATING, FIELD_LABEL,  SettingType::Number,  "range",  "number",  OPERATOR_BETWEEN },
  {        MEDIA::TYPE::SONG,    Field::USER_RATING, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {        MEDIA::TYPE::SONG,           Field::YEAR, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {        MEDIA::TYPE::SONG,          Field::GENRE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS },
  {        MEDIA::TYPE::SONG,      Field::PLAYCOUNT, FIELD_LABEL, SettingType::Integer,  "range", "integer",  OPERATOR_BETWEEN },
  {        MEDIA::TYPE::SONG,         Field::SOURCE, FIELD_LABEL,    SettingType::List,   "list",  "string",   OPERATOR_EQUALS }
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
        m_filter->SetType(m_queryType);

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
                                                   MEDIA::CLibraryQuery& filter)
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
  SET_CONTROL_LABEL(
      CONTROL_HEADING,
      StringUtils::Format(localizeStrings.Get(1275),
                          localizeStrings.Get(MEDIA::LIBRARY_QUERY::LabelOf(m_queryType))));

  SET_CONTROL_LABEL(CONTROL_OKAY_BUTTON, 186);
  SET_CONTROL_LABEL(CONTROL_CLEAR_BUTTON, 192);
}

void CGUIDialogMediaFilter::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  if (m_filter == nullptr)
    return;

  Reset(true);

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
    if (f.mediaType != m_source.itemType)
      continue;

    Filter filter = f;
    if (filter.label == FIELD_LABEL)
    {
      filter.label = static_cast<uint32_t>(MEDIA::CLibraryQueryRule::GetFieldLabel(filter.field));
    }

    // check the smartplaylist if it contains a matching rule
    for (const auto& rule : m_filter->GetRules())
    {
      if (rule->m_field == filter.field)
      {
        filter.rule = rule.get();
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
        values = StringUtils::Split(filter.rule->GetParameter(),
                                    MEDIA::CLibraryQueryRule::VALUE_SEPARATOR);
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
      CLog::Log(LOGWARNING,
                "CGUIDialogMediaFilter: filter {} of media type {} with unknown control type '{}'",
                static_cast<int>(filter.field), filter.mediaType, filter.controlType);
      continue;
    }

    if (filter.setting == nullptr)
    {
      CLog::Log(LOGWARNING,
                "CGUIDialogMediaFilter: failed to create filter {} of media type {} with control "
                "type '{}'",
                static_cast<int>(filter.field), filter.mediaType, filter.controlType);
      continue;
    }

    m_filters.insert(make_pair(settingId, filter));
  }
}

bool CGUIDialogMediaFilter::SetPath(const std::string &path)
{
  if (path.empty() || m_filter == nullptr)
  {
    CLog::Log(LOGWARNING, "CGUIDialogMediaFilter::SetPath({}): invalid path or filter", path);
    return false;
  }

  const std::optional<MEDIA::MediaSection> library{MEDIA::LIBRARY_QUERY::LibraryOf(path)};
  if (!library)
  {
    m_dbUrl.reset();
    CLog::Log(
        LOGWARNING,
        "CGUIDialogMediaFilter::SetPath({}): invalid path (neither videodb:// nor musicdb://)",
        path);
    return false;
  }

  m_dbUrl = MEDIA::LIBRARY_QUERY::UrlOf(*library);
  if (!m_dbUrl->FromString(path))
  {
    CLog::Log(LOGWARNING, "CGUIDialogMediaFilter::SetPath({}): invalid media type", path);
    return false;
  }

  // remove "filter" option
  if (m_dbUrl->HasOption("filter"))
    m_dbUrl->RemoveOption("filter");

  const std::string content{MEDIA::LIBRARY_QUERY::ListedContentOf(*library, *m_dbUrl)};
  const std::optional<MEDIA::LIBRARY_QUERY::TYPE> type{MEDIA::LIBRARY_QUERY::TypeFromName(content)};
  const std::optional<MEDIA::TYPE> itemType{type ? MEDIA::LIBRARY_QUERY::ItemTypeIn(*type, *library)
                                                 : std::nullopt};
  if (!itemType)
  {
    CLog::Log(LOGWARNING, "CGUIDialogMediaFilter::SetPath({}): no filter for a list of {}", path,
              content);
    return false;
  }
  m_queryType = *type;
  m_source = {*library, *itemType};

  m_filter->SetType(m_queryType);
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
  MEDIA::CLibraryQuery tmpFilter = *m_filter;

  auto it = std::ranges::find_if(tmpFilter.GetRules(), [&filter](const auto& rule)
                                 { return filter.field == rule->m_field; });
  if (it != tmpFilter.GetRules().cend())
    tmpFilter.RemoveRule(*it);

  MEDIA::LIBRARY_QUERY::BrowseValues(m_source, MEDIA::CLibraryQueryRule::BrowseOf(filter.field),
                                     m_dbUrl->ToString(), &tmpFilter, countOnly, selectItems);

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

MEDIA::CLibraryQueryRule* CGUIDialogMediaFilter::AddRule(
    Field field, CDatabaseQueryRule::SearchOperator ruleOperator /* = OPERATOR_CONTAINS */)
{
  const auto rule{std::make_shared<MEDIA::CLibraryQueryRule>()};
  rule->m_field = field;
  rule->m_operator = ruleOperator;

  m_filter->AddRule(rule);
  return m_filter->GetRules().back().get();
}

void CGUIDialogMediaFilter::DeleteRule(Field field)
{
  for (const auto& rule : m_filter->GetRules())
  {
    if (rule->m_field == field)
    {
      m_filter->RemoveRule(rule);
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
      return;

    case Field::AIR_DATE:
      interval = 60 * 60 * 24 * 7; // 1 week
      break;

    case Field::TIME:
      interval = 10;
      break;

    default:
      break;
  }

  if (const std::optional<std::pair<int, int>> range{
          MEDIA::LIBRARY_QUERY::RangeOf(m_source, filter.field, m_dbUrl->ToString())})
  {
    min = range->first;
    max = range->second;
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

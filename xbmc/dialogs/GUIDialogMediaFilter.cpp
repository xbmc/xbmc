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

#include "GUIDialogMediaFilter.h"
#include "DbUrl.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "XBDateTime.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "music/MusicDbUrl.h"
#include "playlists/SmartPlayList.h"
#include "settings/SettingUtils.h"
#include "settings/lib/Setting.h"
#include "settings/windows/GUIControlSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "video/VideoDatabase.h"
#include "video/VideoDbUrl.h"

#define CONTROL_HEADING             2

#define CONTROL_CLEAR_BUTTON       27
#define CONTROL_OKAY_BUTTON        28
#define CONTROL_CANCEL_BUTTON      29

#define CHECK_ALL                  -1
#define CHECK_NO                    0
#define CHECK_YES                   1
#define CHECK_LABEL_ALL           593
#define CHECK_LABEL_NO            106
#define CHECK_LABEL_YES           107

static const CGUIDialogMediaFilter::Filter filterList[] = {
  { "movies",       FieldTitle,         556,    SettingTypeString,  "edit",   "string",   CDatabaseQueryRule::OPERATOR_CONTAINS },
  { "movies",       FieldRating,        563,    SettingTypeNumber,  "range",  "number",   CDatabaseQueryRule::OPERATOR_BETWEEN },
  //{ "movies",       FieldTime,          180,    SettingTypeInteger, "range",  "time",     CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "movies",       FieldInProgress,    575,    SettingTypeInteger, "toggle", "",         CDatabaseQueryRule::OPERATOR_FALSE },
  { "movies",       FieldYear,          562,    SettingTypeInteger, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "movies",       FieldTag,           20459,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "movies",       FieldGenre,         515,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "movies",       FieldActor,         20337,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "movies",       FieldDirector,      20339,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "movies",       FieldStudio,        572,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },

  { "tvshows",      FieldTitle,         556,    SettingTypeString,  "edit",   "string",   CDatabaseQueryRule::OPERATOR_CONTAINS },
  //{ "tvshows",      FieldTvShowStatus,  126,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "tvshows",      FieldRating,        563,    SettingTypeNumber,  "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "tvshows",      FieldInProgress,    575,    SettingTypeInteger, "toggle", "",         CDatabaseQueryRule::OPERATOR_FALSE },
  { "tvshows",      FieldYear,          562,    SettingTypeInteger, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "tvshows",      FieldTag,           20459,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "tvshows",      FieldGenre,         515,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "tvshows",      FieldActor,         20337,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "tvshows",      FieldDirector,      20339,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "tvshows",      FieldStudio,        572,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },

  { "episodes",     FieldTitle,         556,    SettingTypeString,  "edit",   "string",   CDatabaseQueryRule::OPERATOR_CONTAINS },
  { "episodes",     FieldRating,        563,    SettingTypeNumber,  "range",  "number",   CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "episodes",     FieldAirDate,       20416,  SettingTypeInteger, "range",  "date",     CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "episodes",     FieldInProgress,    575,    SettingTypeInteger, "toggle", "",         CDatabaseQueryRule::OPERATOR_FALSE },
  { "episodes",     FieldActor,         20337,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "episodes",     FieldDirector,      20339,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },

  { "musicvideos",  FieldTitle,         556,    SettingTypeString,  "edit",   "string",   CDatabaseQueryRule::OPERATOR_CONTAINS },
  { "musicvideos",  FieldArtist,        557,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "musicvideos",  FieldAlbum,         558,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  //{ "musicvideos",  FieldTime,          180,    SettingTypeInteger, "range",  "time",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "musicvideos",  FieldYear,          562,    SettingTypeInteger, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "musicvideos",  FieldTag,           20459,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "musicvideos",  FieldGenre,         515,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "musicvideos",  FieldDirector,      20339,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "musicvideos",  FieldStudio,        572,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },

  { "artists",      FieldArtist,        557,    SettingTypeString,  "edit",   "string",   CDatabaseQueryRule::OPERATOR_CONTAINS },
  { "artists",      FieldGenre,         515,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },

  { "albums",       FieldAlbum,         556,    SettingTypeString,  "edit",   "string",   CDatabaseQueryRule::OPERATOR_CONTAINS },
  { "albums",       FieldArtist,        557,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "albums",       FieldRating,        563,    SettingTypeInteger, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "albums",       FieldAlbumType,     564,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "albums",       FieldYear,          562,    SettingTypeInteger, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "albums",       FieldGenre,         515,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "albums",       FieldMusicLabel,    21899,  SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },

  { "songs",        FieldTitle,         556,    SettingTypeString,  "edit",   "string",   CDatabaseQueryRule::OPERATOR_CONTAINS },
  { "songs",        FieldAlbum,         558,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "songs",        FieldArtist,        557,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "songs",        FieldTime,          180,    SettingTypeInteger, "range",  "time",     CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "songs",        FieldRating,        563,    SettingTypeInteger, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "songs",        FieldYear,          562,    SettingTypeInteger, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
  { "songs",        FieldGenre,         515,    SettingTypeList,    "list",   "string",   CDatabaseQueryRule::OPERATOR_EQUALS },
  { "songs",        FieldPlaycount,     567,    SettingTypeInteger, "range",  "integer",  CDatabaseQueryRule::OPERATOR_BETWEEN },
};

#define NUM_FILTERS sizeof(filterList) / sizeof(CGUIDialogMediaFilter::Filter)

using namespace std;

CGUIDialogMediaFilter::CGUIDialogMediaFilter()
  : CGUIDialogSettingsManualBase(WINDOW_DIALOG_MEDIA_FILTER, "DialogMediaFilter.xml"),
    m_dbUrl(NULL),
    m_filter(NULL)
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

        for (map<std::string, Filter>::iterator filter = m_filters.begin(); filter != m_filters.end(); filter++)
        {
          filter->second.rule = NULL;
          filter->second.setting->Reset();
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

void CGUIDialogMediaFilter::ShowAndEditMediaFilter(const std::string &path, CSmartPlaylist &filter)
{
  CGUIDialogMediaFilter *dialog = (CGUIDialogMediaFilter *)g_windowManager.GetWindow(WINDOW_DIALOG_MEDIA_FILTER);
  if (dialog == NULL)
    return;

  // initialize and show the dialog
  dialog->Initialize();
  dialog->m_filter = &filter;

  // must be called after setting the filter/smartplaylist
  if (!dialog->SetPath(path))
    return;

  dialog->DoModal();
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

void CGUIDialogMediaFilter::OnSettingChanged(const CSetting *setting)
{
  CGUIDialogSettingsManualBase::OnSettingChanged(setting);

  map<std::string, Filter>::iterator it = m_filters.find(setting->GetId());
  if (it == m_filters.end())
    return;
  
  bool remove = false;
  Filter& filter = it->second;
 
  if (filter.controlType == "edit")
  {
    std::string value = setting->ToString();
    if (!value.empty())
    {
      if (filter.rule == NULL)
        filter.rule = AddRule(filter.field, filter.ruleOperator);
      filter.rule->m_parameter.clear();
      filter.rule->m_parameter.push_back(value);
    }
    else
      remove = true;
  }
  else if (filter.controlType == "toggle")
  {
    int choice = static_cast<const CSettingInt*>(setting)->GetValue();
    if (choice > CHECK_ALL)
    {
      CDatabaseQueryRule::SEARCH_OPERATOR ruleOperator = choice == CHECK_YES ? CDatabaseQueryRule::OPERATOR_TRUE : CDatabaseQueryRule::OPERATOR_FALSE;
      if (filter.rule == NULL)
        filter.rule = AddRule(filter.field, ruleOperator);
      else
        filter.rule->m_operator = ruleOperator;
    }
    else
      remove = true;
  }
  else if (filter.controlType == "list")
  {
    std::vector<CVariant> values = CSettingUtils::GetList(static_cast<const CSettingList*>(setting));
    if (!values.empty())
    {
      if (filter.rule == NULL)
        filter.rule = AddRule(filter.field, filter.ruleOperator);

      filter.rule->m_parameter.clear();
      for (std::vector<CVariant>::const_iterator itValue = values.begin(); itValue != values.end(); ++itValue)
        filter.rule->m_parameter.push_back(itValue->asString());
    }
    else
      remove = true;
  }
  else if (filter.controlType == "range")
  {
    const CSettingList *settingList = static_cast<const CSettingList*>(setting);
    std::vector<CVariant> values = CSettingUtils::GetList(settingList);
    if (values.size() != 2)
      return;

    std::string strValueLower, strValueUpper;

    const CSetting *definition = settingList->GetDefinition();
    if (definition->GetType() == SettingTypeInteger)
    {
      const CSettingInt *definitionInt = static_cast<const CSettingInt*>(definition);
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
        else if (filter.controlFormat == "time")
        {
          strValueLower = CDateTime(static_cast<time_t>(valueLower)).GetAsLocalizedTime("mm:ss");
          strValueUpper = CDateTime(static_cast<time_t>(valueUpper)).GetAsLocalizedTime("mm:ss");
        }
        else
        {
          strValueLower = values.at(0).asString();
          strValueUpper = values.at(1).asString();
        }
      }
    }
    else if (definition->GetType() == SettingTypeNumber)
    {
      const CSettingNumber *definitionNumber = static_cast<const CSettingNumber*>(definition);
      float valueLower = values.at(0).asFloat();
      float valueUpper = values.at(1).asFloat();

      if (valueLower > definitionNumber->GetMinimum() ||
          valueUpper < definitionNumber->GetMaximum())
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
      if (filter.rule == NULL)
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
  if (remove && filter.rule != NULL)
  {
    DeleteRule(filter.field);
    filter.rule = NULL;
  }

  CGUIMessage msg(GUI_MSG_REFRESH_LIST, GetID(), 0);
  g_windowManager.SendThreadMessage(msg, WINDOW_DIALOG_MEDIA_FILTER);
}

void CGUIDialogMediaFilter::SetupView()
{
  CGUIDialogSettingsManualBase::SetupView();

  // set the heading label based on the media type
  uint32_t localizedMediaId = 0; 
  if (m_mediaType == "movies")
    localizedMediaId = 20342;
  else if (m_mediaType == "tvshows")
    localizedMediaId = 20343;
  else if (m_mediaType == "episodes")
    localizedMediaId = 20360;
  else if (m_mediaType == "musicvideos")
    localizedMediaId = 20389;
  else if (m_mediaType == "artists")
    localizedMediaId = 133;
  else if (m_mediaType == "albums")
    localizedMediaId = 132;
  else if (m_mediaType == "songs")
    localizedMediaId = 134;

  // set the heading
  SET_CONTROL_LABEL(CONTROL_HEADING, StringUtils::Format(g_localizeStrings.Get(1275).c_str(), g_localizeStrings.Get(localizedMediaId).c_str()));
}

void CGUIDialogMediaFilter::InitializeSettings()
{
  CGUIDialogSettingsManualBase::InitializeSettings();

  if (m_filter == NULL)
    return;

  Reset(true);

  int handledRules = 0;

  CSettingCategory *category = AddCategory("filter", -1);
  if (category == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMediaFilter: unable to setup filters");
    return;
  }

  CSettingGroup *group = AddGroup(category);
  if (group == NULL)
  {
    CLog::Log(LOGERROR, "CGUIDialogMediaFilter: unable to setup filters");
    return;
  }

  for (unsigned int index = 0; index < NUM_FILTERS; index++)
  {
    if (filterList[index].mediaType != m_mediaType)
      continue;

    Filter filter = filterList[index];

    // check the smartplaylist if it contains a matching rule
    for (CDatabaseQueryRules::iterator rule = m_filter->m_ruleCombination.m_rules.begin(); rule != m_filter->m_ruleCombination.m_rules.end(); rule++)
    {
      if ((*rule)->m_field == filter.field)
      {
        filter.rule = (CSmartPlaylistRule *)rule->get();
        handledRules++;
        break;
      }
    }

    std::string settingId = StringUtils::Format("filter.%s.%d", filter.mediaType.c_str(), filter.field); 
    if (filter.controlType == "edit")
    {
      CVariant data;
      if (filter.rule != NULL && filter.rule->m_parameter.size() == 1)
        data = filter.rule->m_parameter.at(0);

      if (filter.settingType == SettingTypeString)
        filter.setting = AddEdit(group, settingId, filter.label, 0, data.asString(), true, false, filter.label, true);
      else if (filter.settingType == SettingTypeInteger)
        filter.setting = AddEdit(group, settingId, filter.label, 0, static_cast<int>(data.asInteger()), 0, 1, 0, false, filter.label, true);
      else if (filter.settingType == SettingTypeNumber)
        filter.setting = AddEdit(group, settingId, filter.label, 0, data.asFloat(), 0.0f, 1.0f, 0.0f, false, filter.label, true);
    }
    else if (filter.controlType == "toggle")
    {
      int value = CHECK_ALL;
      if (filter.rule != NULL)
        value = filter.rule->m_operator == CDatabaseQueryRule::OPERATOR_TRUE ? CHECK_YES : CHECK_NO;

      StaticIntegerSettingOptions entries;
      entries.push_back(pair<int, int>(CHECK_LABEL_ALL, CHECK_ALL));
      entries.push_back(pair<int, int>(CHECK_LABEL_NO,  CHECK_NO));
      entries.push_back(pair<int, int>(CHECK_LABEL_YES, CHECK_YES));

      filter.setting = AddSpinner(group, settingId, filter.label, 0, value, entries, true);
    }
    else if (filter.controlType == "list")
    {
      std::vector<std::string> values;
      if (filter.rule != NULL && filter.rule->m_parameter.size() > 0)
      {
        values = StringUtils::Split(filter.rule->GetParameter(), DATABASEQUERY_RULE_VALUE_SEPARATOR);
        if (values.size() == 1 && values.at(0).empty())
          values.erase(values.begin());
      }

      filter.setting = AddList(group, settingId, filter.label, 0, values, GetStringListOptions, filter.label);
    }
    else if (filter.controlType == "range")
    {
      CVariant valueLower, valueUpper;
      if (filter.rule != NULL)
      {
        if (filter.rule->m_parameter.size() == 2)
        {
          valueLower = filter.rule->m_parameter.at(0);
          valueUpper = filter.rule->m_parameter.at(1);
        }
        else
        {
          DeleteRule(filter.field);
          filter.rule = NULL;
        }
      }

      if (filter.settingType == SettingTypeInteger)
      {
        int min, interval, max;
        GetRange(filter, min, interval, max);

        // don't create the filter if there's no real range
        if (min == max)
          break;

        int iValueLower = valueLower.isNull() ? min : static_cast<int>(valueLower.asInteger());
        int iValueUpper = valueUpper.isNull() ? max : static_cast<int>(valueUpper.asInteger());

        if (filter.controlFormat == "integer")
          filter.setting = AddRange(group, settingId, filter.label, 0, iValueLower, iValueUpper, min, interval, max, -1, 21469, true);
        else if (filter.controlFormat == "percentage")
          filter.setting = AddPercentageRange(group, settingId, filter.label, 0, iValueLower, iValueUpper, -1, 1, 21469, true);
        else if (filter.controlFormat == "date")
          filter.setting = AddDateRange(group, settingId, filter.label, 0, iValueLower, iValueUpper, min, interval, max, -1, 21469, true);
        else if (filter.controlFormat == "time")
          filter.setting = AddTimeRange(group, settingId, filter.label, 0, iValueLower, iValueUpper, min, interval, max, -1, 21469, true);
      }
      else if (filter.settingType == SettingTypeNumber)
      {
        float min, interval, max;
        GetRange(filter, min, interval, max);

        // don't create the filter if there's no real range
        if (min == max)
          break;

        float fValueLower = valueLower.isNull() ? min : valueLower.asFloat();
        float fValueUpper = valueUpper.isNull() ? max : valueUpper.asFloat();

        filter.setting = AddRange(group, settingId, filter.label, 0, fValueLower, fValueUpper, min, interval, max, -1, 21469, true);
      }
    }
    else
    {
      if (filter.rule != NULL)
        handledRules--;

      CLog::Log(LOGWARNING, "CGUIDialogMediaFilter: filter %d of media type %s with unknown control type '%s'",
                filter.field, filter.mediaType.c_str(), filter.controlType.c_str());
      continue;
    }

    if (filter.setting == NULL)
    {
      if (filter.rule != NULL)
        handledRules--;

      CLog::Log(LOGWARNING, "CGUIDialogMediaFilter: failed to create filter %d of media type %s with control type '%s'",
                filter.field, filter.mediaType.c_str(), filter.controlType.c_str());
      continue;
    }

    m_filters.insert(make_pair(settingId, filter));
  }

  // make sure that no change in capacity size is needed when adding new rules
  // which would copy around the rules and our pointers in the Filter struct
  // wouldn't work anymore
  m_filter->m_ruleCombination.m_rules.reserve(m_filters.size() + (m_filter->m_ruleCombination.m_rules.size() - handledRules));
}

bool CGUIDialogMediaFilter::SetPath(const std::string &path)
{
  if (path.empty() || m_filter == NULL)
  {
    CLog::Log(LOGWARNING, "CGUIDialogMediaFilter::SetPath(%s): invalid path or filter", path.c_str());
    return false;
  }

  delete m_dbUrl;
  bool video = false;
  if (path.find("videodb://") == 0)
  {
    m_dbUrl = new CVideoDbUrl();
    video = true;
  }
  else if (path.find("musicdb://") == 0)
    m_dbUrl = new CMusicDbUrl();
  else
  {
    CLog::Log(LOGWARNING, "CGUIDialogMediaFilter::SetPath(%s): invalid path (neither videodb:// nor musicdb://)", path.c_str());
    return false;
  }

  if (!m_dbUrl->FromString(path) ||
     (video && m_dbUrl->GetType() != "movies" && m_dbUrl->GetType() != "tvshows" && m_dbUrl->GetType() != "episodes" && m_dbUrl->GetType() != "musicvideos") ||
     (!video && m_dbUrl->GetType() != "artists" && m_dbUrl->GetType() != "albums" && m_dbUrl->GetType() != "songs"))
  {
    CLog::Log(LOGWARNING, "CGUIDialogMediaFilter::SetPath(%s): invalid media type", path.c_str());
    return false;
  }

  // remove "filter" option
  if (m_dbUrl->HasOption("filter"))
    m_dbUrl->RemoveOption("filter");

  if (video)
    m_mediaType = ((CVideoDbUrl*)m_dbUrl)->GetItemType();
  else
    m_mediaType = m_dbUrl->GetType();

  m_filter->SetType(m_mediaType);
  return true;
}

void CGUIDialogMediaFilter::UpdateControls()
{
  for (map<std::string, Filter>::iterator itFilter = m_filters.begin(); itFilter != m_filters.end(); itFilter++)
  {
    if (itFilter->second.controlType != "list")
      continue;

    std::vector<std::string> items;
    int size = GetItems(itFilter->second, items, true);

    std::string label = g_localizeStrings.Get(itFilter->second.label);
    BaseSettingControlPtr control = GetSettingControl(itFilter->second.setting->GetId());
    if (control == NULL)
      continue;

    if (size <= 0 ||
       (size == 1 && itFilter->second.field != FieldSet && itFilter->second.field != FieldTag))
       CONTROL_DISABLE(control->GetID());
    else
    {
      CONTROL_ENABLE(control->GetID());
      label = StringUtils::Format(g_localizeStrings.Get(21470).c_str(), label.c_str(), size);
    }
    SET_CONTROL_LABEL(control->GetID(), label);
  }
}

void CGUIDialogMediaFilter::TriggerFilter() const
{
  if (m_filter == NULL)
    return;

  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_FILTER_ITEMS, 10); // 10 for advanced
  g_windowManager.SendThreadMessage(message);
}

void CGUIDialogMediaFilter::Reset(bool filtersOnly /* = false */)
{
  if (!filtersOnly)
  {
    delete m_dbUrl;
    m_dbUrl = NULL;
  }

  m_filters.clear();
}

int CGUIDialogMediaFilter::GetItems(const Filter &filter, std::vector<std::string> &items, bool countOnly /* = false */)
{
  CFileItemList selectItems;

  // remove the rule for the field of the filter we want to retrieve items for
  CSmartPlaylist tmpFilter = *m_filter;
  for (CDatabaseQueryRules::iterator rule = tmpFilter.m_ruleCombination.m_rules.begin(); rule != tmpFilter.m_ruleCombination.m_rules.end(); rule++)
  {
    if ((*rule)->m_field == filter.field)
    {
      tmpFilter.m_ruleCombination.m_rules.erase(rule);
      break;
    }
  }

  if (m_mediaType == "movies" || m_mediaType == "tvshows" || m_mediaType == "episodes" || m_mediaType == "musicvideos")
  {
    CVideoDatabase videodb;
    if (!videodb.Open())
      return -1;

    std::set<std::string> playlists;
    CDatabase::Filter dbfilter;
    dbfilter.where = tmpFilter.GetWhereClause(videodb, playlists);

    VIDEODB_CONTENT_TYPE type = VIDEODB_CONTENT_MOVIES;    
    if (m_mediaType == "tvshows")
      type = VIDEODB_CONTENT_TVSHOWS;
    else if (m_mediaType == "episodes")
      type = VIDEODB_CONTENT_EPISODES;
    else if (m_mediaType == "musicvideos")
      type = VIDEODB_CONTENT_MUSICVIDEOS;

    if (filter.field == FieldGenre)
      videodb.GetGenresNav(m_dbUrl->ToString(), selectItems, type, dbfilter, countOnly);
    else if (filter.field == FieldActor || filter.field == FieldArtist)
      videodb.GetActorsNav(m_dbUrl->ToString(), selectItems, type, dbfilter, countOnly);
    else if (filter.field == FieldDirector)
      videodb.GetDirectorsNav(m_dbUrl->ToString(), selectItems, type, dbfilter, countOnly);
    else if (filter.field == FieldStudio)
      videodb.GetStudiosNav(m_dbUrl->ToString(), selectItems, type, dbfilter, countOnly);
    else if (filter.field == FieldAlbum)
      videodb.GetMusicVideoAlbumsNav(m_dbUrl->ToString(), selectItems, -1, dbfilter, countOnly);
    else if (filter.field == FieldTag)
      videodb.GetTagsNav(m_dbUrl->ToString(), selectItems, type, dbfilter, countOnly);
  }
  else if (m_mediaType == "artists" || m_mediaType == "albums" || m_mediaType == "songs")
  {
    CMusicDatabase musicdb;
    if (!musicdb.Open())
      return -1;

    std::set<std::string> playlists;
    CDatabase::Filter dbfilter;
    dbfilter.where = tmpFilter.GetWhereClause(musicdb, playlists);
    
    if (filter.field == FieldGenre)
      musicdb.GetGenresNav(m_dbUrl->ToString(), selectItems, dbfilter, countOnly);
    else if (filter.field == FieldArtist)
      musicdb.GetArtistsNav(m_dbUrl->ToString(), selectItems, m_mediaType == "albums", -1, -1, -1, dbfilter, SortDescription(), countOnly);
    else if (filter.field == FieldAlbum)
      musicdb.GetAlbumsNav(m_dbUrl->ToString(), selectItems, -1, -1, dbfilter, SortDescription(), countOnly);
    else if (filter.field == FieldAlbumType)
      musicdb.GetAlbumTypesNav(m_dbUrl->ToString(), selectItems, dbfilter, countOnly);
    else if (filter.field == FieldMusicLabel)
      musicdb.GetMusicLabelsNav(m_dbUrl->ToString(), selectItems, dbfilter, countOnly);
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
  selectItems.Sort(SortByLabel, SortOrderAscending);

  for (int index = 0; index < size; ++index)
    items.push_back(selectItems.Get(index)->GetLabel());

  return items.size();
}

CSmartPlaylistRule* CGUIDialogMediaFilter::AddRule(Field field, CDatabaseQueryRule::SEARCH_OPERATOR ruleOperator /* = CDatabaseQueryRule::OPERATOR_CONTAINS */)
{
  CSmartPlaylistRule rule;
  rule.m_field = field;
  rule.m_operator = ruleOperator;

  m_filter->m_ruleCombination.AddRule(rule);
  return (CSmartPlaylistRule *)m_filter->m_ruleCombination.m_rules.back().get();
}

void CGUIDialogMediaFilter::DeleteRule(Field field)
{
  for (CDatabaseQueryRules::iterator rule = m_filter->m_ruleCombination.m_rules.begin(); rule != m_filter->m_ruleCombination.m_rules.end(); rule++)
  {
    if ((*rule)->m_field == field)
    {
      m_filter->m_ruleCombination.m_rules.erase(rule);
      break;
    }
  }
}

void CGUIDialogMediaFilter::GetStringListOptions(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data)
{
  if (setting == NULL || data == NULL)
    return;

  CGUIDialogMediaFilter *mediaFilter = static_cast<CGUIDialogMediaFilter*>(data);

  std::map<std::string, Filter>::const_iterator itFilter = mediaFilter->m_filters.find(setting->GetId());
  if (itFilter == mediaFilter->m_filters.end())
    return;

  std::vector<std::string> items;
  if (mediaFilter->GetItems(itFilter->second, items, false) <= 0)
    return;

  for (std::vector<std::string>::const_iterator item = items.begin(); item != items.end(); ++item)
    list.push_back(make_pair(*item, *item));
}

void CGUIDialogMediaFilter::GetRange(const Filter &filter, int &min, int &interval, int &max)
{
  if (filter.field == FieldRating &&
     (m_mediaType == "albums" || m_mediaType == "songs"))
  {
    min = 0;
    interval = 1;
    max = 5;
  }
  else if (filter.field == FieldYear)
  {
    min = 0;
    interval = 1;
    max = 0;

    if (m_mediaType == "movies" || m_mediaType == "tvshows" || m_mediaType == "musicvideos")
    {
      std::string table;
      std::string year;
      if (m_mediaType == "movies")
      {
        table = "movie_view";
        year = DatabaseUtils::GetField(FieldYear, MediaTypeMovie, DatabaseQueryPartWhere);
      }
      else if (m_mediaType == "tvshows")
      {
        table = "tvshow_view";
        year = StringUtils::Format("strftime(\"%%Y\", %s)", DatabaseUtils::GetField(FieldYear, MediaTypeTvShow, DatabaseQueryPartWhere).c_str());
      }
      else if (m_mediaType == "musicvideos")
      {
        table = "musicvideo_view";
        year = DatabaseUtils::GetField(FieldYear, MediaTypeMusicVideo, DatabaseQueryPartWhere);
      }

      CDatabase::Filter filter;
      filter.where = year + " > 0";
      GetMinMax(table, year, min, max, filter);
    }
    else if (m_mediaType == "albums" || m_mediaType == "songs")
    {
      std::string table;
      if (m_mediaType == "albums")
        table = "albumview";
      else if (m_mediaType == "songs")
        table = "songview";
      else
        return;

      CDatabase::Filter filter;
      filter.where = DatabaseUtils::GetField(FieldYear, m_mediaType, DatabaseQueryPartWhere) + " > 0";
      GetMinMax(table, DatabaseUtils::GetField(FieldYear, m_mediaType, DatabaseQueryPartSelect), min, max, filter);
    }
  }
  else if (filter.field == FieldAirDate)
  {
    min = 0;
    interval = 1;
    max = 0;

    if (m_mediaType == "episodes")
    {
      std::string field = StringUtils::Format("CAST(strftime(\"%%s\", c%02d) AS INTEGER)", VIDEODB_ID_EPISODE_AIRED);
      
      GetMinMax("episode_view", field, min, max);
      interval = 60 * 60 * 24 * 7; // 1 week
    }
  }
  else if (filter.field == FieldTime)
  {
    min = 0;
    interval = 10;
    max = 0;

    if (m_mediaType == "songs")
      GetMinMax("songview", "iDuration", min, max);
  }
  else if (filter.field == FieldPlaycount)
  {
    min = 0;
    interval = 1;
    max = 0;

    if (m_mediaType == "songs")
      GetMinMax("songview", "iTimesPlayed", min, max);
  }
}

void CGUIDialogMediaFilter::GetRange(const Filter &filter, float &min, float &interval, float &max)
{
  if (filter.field == FieldRating &&
     (m_mediaType == "movies" || m_mediaType == "tvshows" || m_mediaType == "episodes"))
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

  CDatabase *db = NULL;
  CDbUrl *dbUrl = NULL;
  if (m_mediaType == "movies" || m_mediaType == "tvshows" || m_mediaType == "episodes" || m_mediaType == "musicvideos")
  {
    CVideoDatabase *videodb = new CVideoDatabase();
    if (!videodb->Open())
    {
      delete videodb;
      return false;
    }

    db = videodb;
    dbUrl = new CVideoDbUrl();
  }
  else if (m_mediaType == "artists" || m_mediaType == "albums" || m_mediaType == "songs")
  {
    CMusicDatabase *musicdb = new CMusicDatabase();
    if (!musicdb->Open())
    {
      delete musicdb;
      return false;
    }

    db = musicdb;
    dbUrl = new CMusicDbUrl();
  }

  if (db == NULL || !db->IsOpen() || dbUrl == NULL)
  {
    delete db;
    delete dbUrl;
    return false;
  }

  CDatabase::Filter extFilter = filter;
  std::string strSQLExtra;
  if (!db->BuildSQL(m_dbUrl->ToString(), strSQLExtra, extFilter, strSQLExtra, *dbUrl))
  {
    delete db;
    delete dbUrl;
    return false;
  }

  std::string strSQL = "SELECT %s FROM %s ";

  min = static_cast<int>(strtol(db->GetSingleValue(db->PrepareSQL(strSQL, std::string("MIN(" + field + ")").c_str(), table.c_str()) + strSQLExtra).c_str(), NULL, 0));
  max = static_cast<int>(strtol(db->GetSingleValue(db->PrepareSQL(strSQL, std::string("MAX(" + field + ")").c_str(), table.c_str()) + strSQLExtra).c_str(), NULL, 0));

  db->Close();
  delete db;
  delete dbUrl;

  return true;
}

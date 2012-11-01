/*
 *      Copyright (C) 2012 Team XBMC
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

#include "GUIDialogMediaFilter.h"
#include "FileItem.h"
#include "GUIUserMessages.h"
#include "XBDateTime.h"
#include "dialogs/GUIDialogSelect.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "music/MusicDatabase.h"
#include "playlists/SmartPlayList.h"
#include "utils/log.h"
#include "utils/MathUtils.h"
#include "video/VideoDatabase.h"

#define TIMEOUT_DELAY             500

// list of controls
#define CONTROL_HEADING             2
// list of controls from CGUIDialogSettings
#define CONTROL_GROUP_LIST          5
#define CONTROL_DEFAULT_BUTTON      7
#define CONTROL_DEFAULT_RADIOBUTTON 8
#define CONTROL_DEFAULT_SPIN        9
#define CONTROL_DEFAULT_SLIDER     10

#define CONTROL_CLEAR_BUTTON       27
#define CONTROL_OKAY_BUTTON        28
#define CONTROL_CANCEL_BUTTON      29
#define CONTROL_START              30

#define CHECK_ALL                  -1
#define CHECK_NO                    0
#define CHECK_YES                   1
#define CHECK_LABEL_ALL           593
#define CHECK_LABEL_NO            106
#define CHECK_LABEL_YES           107

using namespace std;

static const CGUIDialogMediaFilter::Filter filterList[] = {
  { "movies",       FieldTitle,         556,    SettingInfo::EDIT,        CSmartPlaylistRule::OPERATOR_CONTAINS },
  { "movies",       FieldRating,        563,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  //{ "movies",       FieldTime,          180,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },
  { "movies",       FieldInProgress,    575,    SettingInfo::CHECK,       CSmartPlaylistRule::OPERATOR_FALSE },
  { "movies",       FieldYear,          562,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "movies",       FieldTag,           20459,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "movies",       FieldGenre,         515,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "movies",       FieldActor,         20337,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "movies",       FieldDirector,      20339,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "movies",       FieldStudio,        572,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  //{ "movies",       FieldLastPlayed,    568,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },
  //{ "movies",       FieldDateAdded,     570,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },

  { "tvshows",      FieldTitle,         556,    SettingInfo::EDIT,        CSmartPlaylistRule::OPERATOR_CONTAINS },
  //{ "tvshows",      FieldTvShowStatus,  126,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },
  { "tvshows",      FieldRating,        563,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "tvshows",      FieldInProgress,    575,    SettingInfo::CHECK,       CSmartPlaylistRule::OPERATOR_FALSE },
  { "tvshows",      FieldYear,          562,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "tvshows",      FieldGenre,         515,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "tvshows",      FieldActor,         20337,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "tvshows",      FieldDirector,      20339,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "tvshows",      FieldStudio,        572,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  //{ "tvshows",      FieldDateAdded,     570,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },

  { "episodes",     FieldTitle,         556,    SettingInfo::EDIT,        CSmartPlaylistRule::OPERATOR_CONTAINS },
  { "episodes",     FieldRating,        563,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "episodes",     FieldAirDate,       20416,  SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "episodes",     FieldInProgress,    575,    SettingInfo::CHECK,       CSmartPlaylistRule::OPERATOR_FALSE },
  { "episodes",     FieldActor,         20337,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "episodes",     FieldDirector,      20339,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  //{ "episodes",     FieldLastPlayed,    568,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },
  //{ "episodes",     FieldDateAdded,     570,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },

  { "musicvideos",  FieldTitle,         556,    SettingInfo::EDIT,        CSmartPlaylistRule::OPERATOR_CONTAINS },
  { "musicvideos",  FieldArtist,        557,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "musicvideos",  FieldAlbum,         558,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  //{ "musicvideos",  FieldTime,          180,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },
  { "musicvideos",  FieldYear,          562,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "musicvideos",  FieldGenre,         515,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "musicvideos",  FieldDirector,      20339,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "musicvideos",  FieldStudio,        572,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  //{ "musicvideos",  FieldLastPlayed,    568,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },
  //{ "musicvideos",  FieldDateAdded,     570,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },

  { "artists",      FieldArtist,        557,    SettingInfo::EDIT,        CSmartPlaylistRule::OPERATOR_CONTAINS },
  { "artists",      FieldGenre,         515,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },

  { "albums",       FieldAlbum,         556,    SettingInfo::EDIT,        CSmartPlaylistRule::OPERATOR_CONTAINS },
  { "albums",       FieldArtist,        557,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "albums",       FieldRating,        563,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "albums",       FieldAlbumType,     564,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "albums",       FieldYear,          562,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "albums",       FieldGenre,         515,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "albums",       FieldMusicLabel,    21899,  SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },

  { "songs",        FieldTitle,         556,    SettingInfo::EDIT,        CSmartPlaylistRule::OPERATOR_CONTAINS },
  { "songs",        FieldAlbum,         558,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "songs",        FieldArtist,        557,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "songs",        FieldTime,          180,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "songs",        FieldRating,        563,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "songs",        FieldYear,          562,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  { "songs",        FieldGenre,         515,    SettingInfo::BUTTON,      CSmartPlaylistRule::OPERATOR_EQUALS },
  { "songs",        FieldPlaycount,     567,    SettingInfo::RANGE,       CSmartPlaylistRule::OPERATOR_BETWEEN },
  //{ "songs",        FieldLastPlayed,    568,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },
  //{ "songs",        FieldDateAdded,     570,    SettingInfo::TODO,        CSmartPlaylistRule::TODO },
};

#define NUM_FILTERS sizeof(filterList) / sizeof(CGUIDialogMediaFilter::Filter)

CGUIDialogMediaFilter::CGUIDialogMediaFilter()
    : CGUIDialogSettings(WINDOW_DIALOG_MEDIA_FILTER, "DialogMediaFilter.xml"),
      m_dbUrl(NULL),
      m_filter(NULL)
{
  m_delayTimer = new CTimer(this);
}

CGUIDialogMediaFilter::~CGUIDialogMediaFilter()
{
  Reset();
  delete m_delayTimer;
}

bool CGUIDialogMediaFilter::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int control = message.GetSenderId();

      if (control == CONTROL_CLEAR_BUTTON)
      {
        m_filter->Reset();
        m_filter->SetType(m_mediaType);
        if (m_delayTimer && m_delayTimer->IsRunning())
          m_delayTimer->Stop();

        for (map<uint32_t, Filter>::iterator filter = m_filters.begin(); filter != m_filters.end(); filter++)
        {
          filter->second.rule = NULL;
          
          switch (filter->second.type)
          {
            case SettingInfo::STRING:
            case SettingInfo::EDIT:
              ((CStdString *)filter->second.data)->clear();
              break;

            case SettingInfo::CHECK:
              *(int *)filter->second.data = CHECK_ALL;
              break;

            case SettingInfo::BUTTON:
              ((CStdString *)filter->second.data)->clear();
              SET_CONTROL_LABEL2(filter->second.controlIndex, *(CStdString *)filter->second.data);
              break;

            case SettingInfo::RANGE:
              *(((float **)filter->second.data)[0]) = m_settings[filter->second.controlIndex - CONTROL_START].min;
              *(((float **)filter->second.data)[1]) = m_settings[filter->second.controlIndex - CONTROL_START].max;
              break;

            default:
              continue;
          }

          UpdateSetting(filter->first);
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

  return CGUIDialogSettings::OnMessage(message);
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
  CGUIDialogSettings::OnWindowLoaded();
  // we don't need the cancel button so let's hide it
  SET_CONTROL_HIDDEN(CONTROL_CANCEL_BUTTON);
}

void CGUIDialogMediaFilter::CreateSettings()
{
  if (m_filter == NULL)
    return;

  m_settings.clear();
  int handledRules = 0;
  for (unsigned int index = 0; index < NUM_FILTERS; index++)
  {
    if (filterList[index].mediaType != m_mediaType)
      continue;

    Filter filter = filterList[index];
    filter.controlIndex = CONTROL_START + m_settings.size();

    // check the smartplaylist if it contains a matching rule
    for (vector<CSmartPlaylistRule>::iterator rule = m_filter->m_ruleCombination.m_rules.begin(); rule != m_filter->m_ruleCombination.m_rules.end(); rule++)
    {
      if (rule->m_field == filter.field)
      {
        filter.rule = &(*rule);
        handledRules++;
        break;
      }
    }

    switch (filter.type)
    {
      case SettingInfo::STRING:
      case SettingInfo::EDIT:
      {
        if (filter.rule != NULL && filter.rule->m_parameter.size() == 1)
          filter.data = new CStdString(filter.rule->m_parameter.at(0));
        else
          filter.data = new CStdString();

        if (filter.type == SettingInfo::STRING)
          AddString(filter.field, filter.label, (CStdString *)filter.data);
        else
          AddEdit(filter.field, filter.label, (CStdString *)filter.data);
        break;
      }
      
      case SettingInfo::CHECK:
      {
        if (filter.rule == NULL)
          filter.data = new int(CHECK_ALL);
        else
          filter.data = new int(filter.rule->m_operator == CSmartPlaylistRule::OPERATOR_TRUE ? CHECK_YES : CHECK_NO);

        vector<pair<int, int> > entries;
        entries.push_back(pair<int, int>(CHECK_ALL, CHECK_LABEL_ALL));
        entries.push_back(pair<int, int>(CHECK_NO,  CHECK_LABEL_NO));
        entries.push_back(pair<int, int>(CHECK_YES, CHECK_LABEL_YES));
        AddSpin(filter.field, filter.label, (int *)filter.data, entries);
        break;
      }

      case SettingInfo::BUTTON:
      {
        CStdString *values = new CStdString();
        if (filter.rule != NULL && filter.rule->m_parameter.size() > 0)
          *values = filter.rule->GetParameter();
        filter.data = values;

        AddButton(filter.field, filter.label);
        break;
      }

      case SettingInfo::RANGE:
      {
        float min, interval, max;
        RANGEFORMATFUNCTION format;
        GetRange(filter, min, interval, max, format);

        // don't create the filter if there's no real range
        if (min == max)
          break;

        float *valueLower = new float();
        float *valueUpper = new float();
        if (filter.rule != NULL && filter.rule->m_parameter.size() == 2)
        {
          *valueLower = (float)strtod(filter.rule->m_parameter.at(0), NULL);
          *valueUpper = (float)strtod(filter.rule->m_parameter.at(1), NULL);
        }
        else
        {
          *valueLower = min;
          *valueUpper = max;

          if (filter.rule != NULL)
          {
            DeleteRule(filter.field);
            filter.rule = NULL;
          }
        }

        AddRangeSlider(filter.field, filter.label, valueLower, valueUpper, min, interval, max, format);
        filter.data = m_settings[filter.controlIndex - CONTROL_START].data;
        break;
      }

      default:
        filter.controlIndex = -1;
        if (filter.rule != NULL)
          handledRules--;
        continue;
    }

    m_filters[filter.field] = filter;
  }

  // make sure that no change in capacity size is needed when adding new rules
  // which would copy around the rules and our pointers in the Filter struct
  // wouldn't work anymore
  m_filter->m_ruleCombination.m_rules.reserve(m_filters.size() + (m_filter->m_ruleCombination.m_rules.size() - handledRules));
}

void CGUIDialogMediaFilter::SetupPage()
{
  CGUIDialogSettings::SetupPage();

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

  CStdString format;
  format.Format(g_localizeStrings.Get(1275).c_str(), g_localizeStrings.Get(localizedMediaId).c_str());
  SET_CONTROL_LABEL(CONTROL_HEADING, format);

  // now we can finally set the label/values of the button settings (genre, actors etc)
  for (map<uint32_t, Filter>::const_iterator filter = m_filters.begin(); filter != m_filters.end(); filter++)
  {
    if (filter->second.type == SettingInfo::BUTTON &&
        filter->second.controlIndex >= 0 && filter->second.data != NULL)
      SET_CONTROL_LABEL2(filter->second.controlIndex, *(CStdString *)filter->second.data);
  }

  UpdateControls();
}

void CGUIDialogMediaFilter::OnTimeout()
{
  CGUIMessage msg(GUI_MSG_REFRESH_LIST, GetID(), 0);
  g_windowManager.SendThreadMessage(msg, WINDOW_DIALOG_MEDIA_FILTER);
}

void CGUIDialogMediaFilter::OnSettingChanged(SettingInfo &setting)
{
  map<uint32_t, Filter>::iterator it = m_filters.find(setting.id);
  if (it == m_filters.end())
    return;

  bool changed = true;
  bool delay = false;
  bool remove = false;
  Filter& filter = it->second;

  switch (filter.type)
  {
    case SettingInfo::STRING:
    case SettingInfo::EDIT:
    {
      CStdString *str = static_cast<CStdString*>(filter.data);
      if (!str->empty())
      {
        if (filter.rule == NULL)
          filter.rule = AddRule(filter.field, filter.ruleOperator);
        filter.rule->m_parameter.clear();
        filter.rule->m_parameter.push_back(*str);
        // trigger the live filtering with a delay in case the user
        // types several characters in a short time
        delay = true;
      }
      else
        remove = true;
        
      break;
    }
    
    case SettingInfo::CHECK:
    {
      int choice = *(int *)setting.data;
      if (choice > CHECK_ALL)
      {
        CSmartPlaylistRule::SEARCH_OPERATOR ruleOperator = choice == CHECK_YES ? CSmartPlaylistRule::OPERATOR_TRUE : CSmartPlaylistRule::OPERATOR_FALSE;
        if (filter.rule == NULL)
          filter.rule = AddRule(filter.field, ruleOperator);
        else
          filter.rule->m_operator = ruleOperator;
      }
      else
        remove = true;

      break;
    }

    case SettingInfo::BUTTON:
    {
      CFileItemList items;
      OnBrowse(filter, items);
      
      if (items.Size() > 0)
      {
        if (filter.rule == NULL)
          filter.rule = AddRule(filter.field, filter.ruleOperator);

        filter.rule->m_parameter.clear();
        for (int index = 0; index < items.Size(); index++)
          filter.rule->m_parameter.push_back(items[index]->GetLabel());

        *(CStdString *)filter.data = filter.rule->GetParameter();
      }
      else
      {
        remove = true;
        *(CStdString *)filter.data = "";
      }

      SET_CONTROL_LABEL2(filter.controlIndex, *(CStdString *)filter.data);
      break;
    }

    case SettingInfo::RANGE:
    {
      SettingInfo &setting = m_settings[filter.controlIndex - CONTROL_START];
      float *valueLower = ((float **)filter.data)[0];
      float *valueUpper = ((float **)filter.data)[1];

      if (*valueLower > setting.min || *valueUpper < setting.max)
      {
        if (filter.rule == NULL)
          filter.rule = AddRule(filter.field, filter.ruleOperator);

        filter.rule->m_parameter.clear();
        if (filter.field == FieldAirDate)
        {
          CDateTime lower = (time_t)*valueLower;
          CDateTime upper = (time_t)*valueUpper;
          filter.rule->m_parameter.push_back(lower.GetAsDBDate());
          filter.rule->m_parameter.push_back(upper.GetAsDBDate());
        }
        else
        {
          CStdString tmp;
          tmp.Format("%.1f", *valueLower);
          filter.rule->m_parameter.push_back(tmp);
          tmp.clear();
          tmp.Format("%.1f", *valueUpper);
          filter.rule->m_parameter.push_back(tmp);
        }
      }
      else
      {
        remove = true;
        *((float **)filter.data)[0] = setting.min;
        *((float **)filter.data)[1] = setting.max;
      }

      // trigger the live filtering with a delay in case the user
      // moves the slider several steps in a short time
      delay = true;
      break;
    }

    default:
      changed = false;
      break;
  }

  // we need to remove the existing rule for the title
  if (remove && filter.rule != NULL)
  {
    DeleteRule(filter.field);
    filter.rule = NULL;
  }

  if (changed)
  {
    if (!delay)
    {
      CGUIMessage msg(GUI_MSG_REFRESH_LIST, GetID(), 0);
      OnMessage(msg);
    }
    else if (m_delayTimer)
    {
      if (m_delayTimer->IsRunning())
        m_delayTimer->Restart();
      else
        m_delayTimer->Start(TIMEOUT_DELAY, false);
    }
  }
}

void CGUIDialogMediaFilter::Reset()
{
  if (m_delayTimer && m_delayTimer->IsRunning())
    m_delayTimer->Stop();

  delete m_dbUrl;
  m_dbUrl = NULL;

  // delete all the setting's data
  for (map<uint32_t, Filter>::iterator filter = m_filters.begin(); filter != m_filters.end(); filter++)
  {
    switch (filter->second.type)
    {
      case SettingInfo::STRING:
      case SettingInfo::EDIT:
      case SettingInfo::BUTTON:
        delete (CStdString *)filter->second.data;
        break;

      case SettingInfo::CHECK:
        delete (int *)filter->second.data;
        break;

      case SettingInfo::RANGE:
        if (filter->second.data != NULL)
        {
          delete ((float **)filter->second.data)[0];
          delete ((float **)filter->second.data)[1];
        }
        delete (float *)filter->second.data;
        break;

      default:
        continue;
    }
  }

  m_filters.clear();
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
    m_dbUrl->AddOption("filter", "");

  if (video)
    m_mediaType = ((CVideoDbUrl*)m_dbUrl)->GetItemType();
  else
    m_mediaType = m_dbUrl->GetType();

  m_filter->SetType(m_mediaType);
  return true;
}

void CGUIDialogMediaFilter::UpdateControls()
{
  for (map<uint32_t, Filter>::iterator itFilter = m_filters.begin(); itFilter != m_filters.end(); itFilter++)
  {
    if (itFilter->second.type == SettingInfo::BUTTON)
    {
      CFileItemList items;
      OnBrowse(itFilter->second, items, true);

      int size = items.Size();
      if (items.Size() == 1 && items[0]->HasProperty("total"))
        size = (int)items[0]->GetProperty("total").asInteger();

      CStdString label = g_localizeStrings.Get(itFilter->second.label);
      if (size <= 1)
        CONTROL_DISABLE(itFilter->second.controlIndex);
      else
      {
        CONTROL_ENABLE(itFilter->second.controlIndex);
        label.Format("%s [%d]", label, size);
      }
      SET_CONTROL_LABEL(itFilter->second.controlIndex, label);
    }
  }
}

void CGUIDialogMediaFilter::TriggerFilter() const
{
  if (m_filter == NULL)
    return;

  CGUIMessage message(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_FILTER_ITEMS, 10); // 10 for advanced
  g_windowManager.SendThreadMessage(message);
}

void CGUIDialogMediaFilter::OnBrowse(const Filter &filter, CFileItemList &items, bool countOnly /* = false */)
{
  CFileItemList selectItems;
  if (m_mediaType == "movies" || m_mediaType == "tvshows" || m_mediaType == "episodes" || m_mediaType == "musicvideos")
  {
    CVideoDatabase videodb;
    if (!videodb.Open())
      return;

    CSmartPlaylist tmpFilter = *m_filter;
    for (vector<CSmartPlaylistRule>::iterator rule = tmpFilter.m_ruleCombination.m_rules.begin(); rule != tmpFilter.m_ruleCombination.m_rules.end(); rule++)
    {
      if (rule->m_field == filter.field)
      {
        tmpFilter.m_ruleCombination.m_rules.erase(rule);
        break;
      }
    }

    std::set<CStdString> playlists;
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
      return;

    CSmartPlaylist tmpFilter = *m_filter;
    for (vector<CSmartPlaylistRule>::iterator rule = tmpFilter.m_ruleCombination.m_rules.begin(); rule != tmpFilter.m_ruleCombination.m_rules.end(); rule++)
    {
      if (rule->m_field == filter.field)
      {
        tmpFilter.m_ruleCombination.m_rules.erase(rule);
        break;
      }
    }

    std::set<CStdString> playlists;
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

  if (selectItems.Size() <= 0)
    return;

  if (countOnly)
  {
    items.Copy(selectItems);
    return;
  }

  // sort the items
  selectItems.Sort(SORT_METHOD_LABEL, SortOrderAscending);

  CGUIDialogSelect* pDialog = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  pDialog->Reset();
  pDialog->SetItems(&selectItems);
  CStdString strHeading;
  strHeading.Format(g_localizeStrings.Get(13401), g_localizeStrings.Get(filter.label));
  pDialog->SetHeading(strHeading);
  pDialog->SetMultiSelection(true);

  if (filter.rule != NULL && !filter.rule->m_parameter.empty())
    pDialog->SetSelected(filter.rule->m_parameter);

  pDialog->DoModal();
  if (pDialog->IsConfirmed())
    items.Copy(pDialog->GetSelectedItems());
  else
    items.Clear();
  pDialog->Reset();
}

CSmartPlaylistRule* CGUIDialogMediaFilter::AddRule(Field field, CSmartPlaylistRule::SEARCH_OPERATOR ruleOperator /* = CSmartPlaylistRule::OPERATOR_CONTAINS */)
{
  CSmartPlaylistRule rule;
  rule.m_field = field;
  rule.m_operator = ruleOperator;

  m_filter->m_ruleCombination.m_rules.push_back(rule);
  return &m_filter->m_ruleCombination.m_rules.at(m_filter->m_ruleCombination.m_rules.size() - 1);
}

void CGUIDialogMediaFilter::DeleteRule(Field field)
{
  for (vector<CSmartPlaylistRule>::iterator rule = m_filter->m_ruleCombination.m_rules.begin(); rule != m_filter->m_ruleCombination.m_rules.end(); rule++)
  {
    if (rule->m_field == field)
    {
      m_filter->m_ruleCombination.m_rules.erase(rule);
      break;
    }
  }
}

void CGUIDialogMediaFilter::GetRange(const Filter &filter, float &min, float &interval, float &max, RANGEFORMATFUNCTION &formatFunction)
{
  if (filter.field == FieldRating)
  {
    if (m_mediaType == "movies" || m_mediaType == "tvshows" || m_mediaType == "episodes")
    {
      min = 0.0f;
      interval = 0.1f;
      max = 10.0f;
      formatFunction = RangeAsFloat;
    }
    else if (m_mediaType == "albums" || m_mediaType == "songs")
    {
      min = 0.0f;
      interval = 1.0f;
      max = 5.0f;
      formatFunction = RangeAsInt;
    }
  }
  else if (filter.field == FieldYear)
  {
    formatFunction = RangeAsInt;
    min = 0.0f;
    interval = 1.0f;
    max = 0.0f;

    if (m_mediaType == "movies" || m_mediaType == "tvshows" || m_mediaType == "musicvideos")
    {
      CStdString table;
      CStdString year;
      if (m_mediaType == "movies")
      {
        table = "movieview";
        year = DatabaseUtils::GetField(FieldYear, MediaTypeMovie, DatabaseQueryPartWhere);
      }
      else if (m_mediaType == "tvshows")
      {
        table = "tvshowview";
        year.Format("strftime(\"%%Y\", %s)", DatabaseUtils::GetField(FieldYear, MediaTypeTvShow, DatabaseQueryPartWhere));
      }
      else if (m_mediaType == "musicvideos")
      {
        table = "musicvideoview";
        year = DatabaseUtils::GetField(FieldYear, MediaTypeMusicVideo, DatabaseQueryPartWhere);
      }

      CDatabase::Filter filter;
      filter.where = year + " > 0";
      GetMinMax(table, year, min, max, filter);
    }
    else if (m_mediaType == "albums" || m_mediaType == "songs")
    {
      CStdString table;
      MediaType mediaType;
      if (m_mediaType == "albums")
      {
        table = "albumview";
        mediaType = MediaTypeAlbum;
      }
      else if (m_mediaType == "songs")
      {
        table = "songview";
        mediaType = MediaTypeSong;
      }
      else
        return;

      CDatabase::Filter filter;
      filter.where = DatabaseUtils::GetField(FieldYear, mediaType, DatabaseQueryPartWhere) + " > 0";
      GetMinMax(table, DatabaseUtils::GetField(FieldYear, mediaType, DatabaseQueryPartSelect), min, max, filter);
    }
  }
  else if (filter.field == FieldAirDate)
  {
    formatFunction = RangeAsDate;
    min = 0.0f;
    interval = 1.0f;
    max = 0.0f;

    if (m_mediaType == "episodes")
    {
      CStdString field; field.Format("CAST(strftime(\"%%s\", c%02d) AS INTEGER)", VIDEODB_ID_EPISODE_AIRED);
      
      GetMinMax("episodeview", field, min, max);
      interval = 60 * 60 * 24 * 7; // 1 week
    }
  }
  else if (filter.field == FieldTime)
  {
    formatFunction = RangeAsTime;
    min = 0.0f;
    interval = 10.0f;
    max = 0.0f;

    if (m_mediaType == "songs")
      GetMinMax("songview", "iDuration", min, max);
  }
  else if (filter.field == FieldPlaycount)
  {
    formatFunction = RangeAsInt;
    min = 0.0f;
    interval = 1.0f;
    max = 0.0f;

    if (m_mediaType == "songs")
      GetMinMax("songview", "iTimesPlayed", min, max);
  }
}

bool CGUIDialogMediaFilter::GetMinMax(const CStdString &table, const CStdString &field, float &min, float &max, const CDatabase::Filter &filter /* = CDatabase::Filter() */)
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
  CStdString strSQLExtra;
  if (!db->BuildSQL(m_dbUrl->ToString(), strSQLExtra, extFilter, strSQLExtra, *dbUrl))
  {
    delete db;
    delete dbUrl;
    return false;
  }

  CStdString strSQL = "SELECT %s FROM %s ";

  min = (float)strtod(db->GetSingleValue(db->PrepareSQL(strSQL, CStdString("MIN(" + field + ")").c_str(), table.c_str()) + strSQLExtra).c_str(), NULL);
  max = (float)strtod(db->GetSingleValue(db->PrepareSQL(strSQL, CStdString("MAX(" + field + ")").c_str(), table.c_str()) + strSQLExtra).c_str(), NULL);

  db->Close();
  delete db;
  delete dbUrl;

  return true;
}

CStdString CGUIDialogMediaFilter::RangeAsFloat(float valueLower, float valueUpper, float minimum)
{
  CStdString text;
  if (valueLower != valueUpper)
    text.Format(g_localizeStrings.Get(21467).c_str(), valueLower, valueUpper);
  else
    text.Format("%.1f", valueLower);
  return text;
}

CStdString CGUIDialogMediaFilter::RangeAsInt(float valueLower, float valueUpper, float minimum)
{
  CStdString text;
  if (valueLower != valueUpper)
    text.Format(g_localizeStrings.Get(21468).c_str(), MathUtils::round_int((double)valueLower), MathUtils::round_int((double)valueUpper));
  else
    text.Format("%d", MathUtils::round_int((double)valueLower));
  return text;
}

CStdString CGUIDialogMediaFilter::RangeAsDate(float valueLower, float valueUpper, float minimum)
{
  CDateTime from = (time_t)valueLower;
  CDateTime to = (time_t)valueUpper;
  CStdString text;
  if (valueLower != valueUpper)
    text.Format(g_localizeStrings.Get(21469).c_str(), from.GetAsLocalizedDate(), to.GetAsLocalizedDate());
  else
    text.Format("%s", from.GetAsLocalizedDate());
  return text;
}

CStdString CGUIDialogMediaFilter::RangeAsTime(float valueLower, float valueUpper, float minimum)
{
  CDateTime from = (time_t)valueLower;
  CDateTime to = (time_t)valueUpper;
  CStdString text;
  if (valueLower != valueUpper)
    text.Format(g_localizeStrings.Get(21469).c_str(), from.GetAsLocalizedTime("mm:ss"), to.GetAsLocalizedTime("mm:ss"));
  else
    text.Format("%s", from.GetAsLocalizedTime("mm:ss"));
  return text;
}

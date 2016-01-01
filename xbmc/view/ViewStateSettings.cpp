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

#include "ViewStateSettings.h"

#include <cstring>
#include <utility>

#include "threads/SingleLock.h"
#include "utils/log.h"
#include "utils/SortUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"

#define XML_VIEWSTATESETTINGS       "viewstates"
#define XML_VIEWMODE                "viewmode"
#define XML_SORTMETHOD              "sortmethod"
#define XML_SORTORDER               "sortorder"
#define XML_SORTATTRIBUTES          "sortattributes"
#define XML_GENERAL                 "general"
#define XML_SETTINGLEVEL            "settinglevel"
#define XML_EVENTLOG                "eventlog"
#define XML_EVENTLOG_LEVEL          "level"
#define XML_EVENTLOG_LEVEL_HIGHER   "showhigherlevels"

CViewStateSettings::CViewStateSettings()
  : m_settingLevel(SettingLevelStandard),
    m_eventLevel(EventLevel::Basic),
    m_eventShowHigherLevels(true)
{
  AddViewState("musicnavartists");
  AddViewState("musicnavalbums");
  AddViewState("musicnavsongs");
  AddViewState("musiclastfm");
  AddViewState("videonavactors");
  AddViewState("videonavyears");
  AddViewState("videonavgenres");
  AddViewState("videonavtitles");
  AddViewState("videonavepisodes", DEFAULT_VIEW_AUTO, SortByEpisodeNumber);
  AddViewState("videonavtvshows");
  AddViewState("videonavseasons");
  AddViewState("videonavmusicvideos");

  AddViewState("programs", DEFAULT_VIEW_AUTO);
  AddViewState("pictures", DEFAULT_VIEW_AUTO);
  AddViewState("videofiles", DEFAULT_VIEW_AUTO);
  AddViewState("musicfiles", DEFAULT_VIEW_AUTO);

  Clear();
}

CViewStateSettings::~CViewStateSettings()
{
  for (std::map<std::string, CViewState*>::const_iterator viewState = m_viewStates.begin(); viewState != m_viewStates.end(); ++viewState)
    delete viewState->second;
  m_viewStates.clear();
}

CViewStateSettings& CViewStateSettings::GetInstance()
{
  static CViewStateSettings sViewStateSettings;
  return sViewStateSettings;
}

bool CViewStateSettings::Load(const TiXmlNode *settings)
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  const TiXmlNode *pElement = settings->FirstChildElement(XML_VIEWSTATESETTINGS);
  if (pElement == NULL)
  {
    CLog::Log(LOGWARNING, "CViewStateSettings: no <viewstates> tag found");
    return false;
  }

  for (std::map<std::string, CViewState*>::iterator viewState = m_viewStates.begin(); viewState != m_viewStates.end(); ++viewState)
  {
    const TiXmlNode* pViewState = pElement->FirstChildElement(viewState->first);
    if (pViewState == NULL)
      continue;

    XMLUtils::GetInt(pViewState, XML_VIEWMODE, viewState->second->m_viewMode, DEFAULT_VIEW_LIST, DEFAULT_VIEW_MAX);

    // keep backwards compatibility to the old sorting methods
    if (pViewState->FirstChild(XML_SORTATTRIBUTES) == NULL)
    {
      int sortMethod;
      if (XMLUtils::GetInt(pViewState, XML_SORTMETHOD, sortMethod, SORT_METHOD_NONE, SORT_METHOD_MAX))
        viewState->second->m_sortDescription = SortUtils::TranslateOldSortMethod((SORT_METHOD)sortMethod);
    }
    else
    {
      int sortMethod;
      if (XMLUtils::GetInt(pViewState, XML_SORTMETHOD, sortMethod, SortByNone, SortByRandom))
        viewState->second->m_sortDescription.sortBy = (SortBy)sortMethod;
      if (XMLUtils::GetInt(pViewState, XML_SORTATTRIBUTES, sortMethod, SortAttributeNone, SortAttributeIgnoreFolders))
        viewState->second->m_sortDescription.sortAttributes = (SortAttribute)sortMethod;
    }

    int sortOrder;
    if (XMLUtils::GetInt(pViewState, XML_SORTORDER, sortOrder, SortOrderNone, SortOrderDescending))
      viewState->second->m_sortDescription.sortOrder = (SortOrder)sortOrder;
  }

  pElement = settings->FirstChild(XML_GENERAL);
  if (pElement != NULL)
  {
    int settingLevel;
    if (XMLUtils::GetInt(pElement, XML_SETTINGLEVEL, settingLevel, (const int)SettingLevelBasic, (const int)SettingLevelExpert))
      m_settingLevel = (SettingLevel)settingLevel;
    else
      m_settingLevel = SettingLevelStandard;

    const TiXmlNode* pEventLogNode = pElement->FirstChild(XML_EVENTLOG);
    if (pEventLogNode != NULL)
    {
      int eventLevel;
      if (XMLUtils::GetInt(pEventLogNode, XML_EVENTLOG_LEVEL, eventLevel, (const int)EventLevel::Basic, (const int)EventLevel::Error))
        m_eventLevel = (EventLevel)eventLevel;
      else
        m_eventLevel = EventLevel::Basic;

      if (!XMLUtils::GetBoolean(pEventLogNode, XML_EVENTLOG_LEVEL_HIGHER, m_eventShowHigherLevels))
        m_eventShowHigherLevels = true;
    }
  }

  return true;
}

bool CViewStateSettings::Save(TiXmlNode *settings) const
{
  if (settings == NULL)
    return false;

  CSingleLock lock(m_critical);
  // add the <viewstates> tag
  TiXmlElement xmlViewStateElement(XML_VIEWSTATESETTINGS);
  TiXmlNode *pViewStateNode = settings->InsertEndChild(xmlViewStateElement);
  if (pViewStateNode == NULL)
  {
    CLog::Log(LOGWARNING, "CViewStateSettings: could not create <viewstates> tag");
    return false;
  }

  for (std::map<std::string, CViewState*>::const_iterator viewState = m_viewStates.begin(); viewState != m_viewStates.end(); ++viewState)
  {
    TiXmlElement newElement(viewState->first);
    TiXmlNode *pNewNode = pViewStateNode->InsertEndChild(newElement);
    if (pNewNode == NULL)
      continue;

    XMLUtils::SetInt(pNewNode, XML_VIEWMODE, viewState->second->m_viewMode);
    XMLUtils::SetInt(pNewNode, XML_SORTMETHOD, (int)viewState->second->m_sortDescription.sortBy);
    XMLUtils::SetInt(pNewNode, XML_SORTORDER, (int)viewState->second->m_sortDescription.sortOrder);
    XMLUtils::SetInt(pNewNode, XML_SORTATTRIBUTES, (int)viewState->second->m_sortDescription.sortAttributes);
  }

  TiXmlNode *generalNode = settings->FirstChild(XML_GENERAL);
  if (generalNode == NULL)
  {
    TiXmlElement generalElement(XML_GENERAL);
    generalNode = settings->InsertEndChild(generalElement);
    if (generalNode == NULL)
      return false;
  }

  XMLUtils::SetInt(generalNode, XML_SETTINGLEVEL, (int)m_settingLevel);

  TiXmlNode *eventLogNode = generalNode->FirstChild(XML_EVENTLOG);
  if (eventLogNode == NULL)
  {
    TiXmlElement eventLogElement(XML_EVENTLOG);
    eventLogNode = generalNode->InsertEndChild(eventLogElement);
    if (eventLogNode == NULL)
      return false;
  }

  XMLUtils::SetInt(eventLogNode, XML_EVENTLOG_LEVEL, (int)m_eventLevel);
  XMLUtils::SetBoolean(eventLogNode, XML_EVENTLOG_LEVEL_HIGHER, (int)m_eventShowHigherLevels);

  return true;
}

void CViewStateSettings::Clear()
{
  m_settingLevel = SettingLevelStandard;
}

const CViewState* CViewStateSettings::Get(const std::string &viewState) const
{
  CSingleLock lock(m_critical);
  std::map<std::string, CViewState*>::const_iterator view = m_viewStates.find(viewState);
  if (view != m_viewStates.end())
    return view->second;

  return NULL;
}

CViewState* CViewStateSettings::Get(const std::string &viewState)
{
  CSingleLock lock(m_critical);
  std::map<std::string, CViewState*>::iterator view = m_viewStates.find(viewState);
  if (view != m_viewStates.end())
    return view->second;

  return NULL;
}

void CViewStateSettings::SetSettingLevel(SettingLevel settingLevel)
{
  if (settingLevel < SettingLevelBasic)
    m_settingLevel = SettingLevelBasic;
  if (settingLevel > SettingLevelExpert)
    m_settingLevel = SettingLevelExpert;
  else
    m_settingLevel = settingLevel;
}

void CViewStateSettings::CycleSettingLevel()
{
  m_settingLevel = GetNextSettingLevel();
}

SettingLevel CViewStateSettings::GetNextSettingLevel() const
{
  SettingLevel level = (SettingLevel)((int)m_settingLevel + 1);
  if (level > SettingLevelExpert)
    level = SettingLevelBasic;
  return level;
}

void CViewStateSettings::SetEventLevel(EventLevel eventLevel)
{
  if (eventLevel < EventLevel::Basic)
    m_eventLevel = EventLevel::Basic;
  if (eventLevel > EventLevel::Error)
    m_eventLevel = EventLevel::Error;
  else
    m_eventLevel = eventLevel;
}

void CViewStateSettings::CycleEventLevel()
{
  m_eventLevel = GetNextEventLevel();
}

EventLevel CViewStateSettings::GetNextEventLevel() const
{
  EventLevel level = (EventLevel)((int)m_eventLevel + 1);
  if (level > EventLevel::Error)
    level = EventLevel::Basic;
  return level;
}

void CViewStateSettings::AddViewState(const std::string& strTagName, int defaultView /* = DEFAULT_VIEW_LIST */, SortBy defaultSort /* = SortByLabel */)
{
  if (strTagName.empty() || m_viewStates.find(strTagName) != m_viewStates.end())
    return;

  CViewState *viewState = new CViewState(defaultView, defaultSort, SortOrderAscending);
  if (viewState == NULL)
    return;

  m_viewStates.insert(make_pair(strTagName, viewState));
}

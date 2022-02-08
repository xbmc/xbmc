/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ViewStateSettings.h"

#include "utils/SortUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <cstring>
#include <mutex>
#include <utility>

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
{
  AddViewState("musicnavartists");
  AddViewState("musicnavalbums");
  AddViewState("musicnavsongs", DEFAULT_VIEW_LIST, SortByTrackNumber);
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
  AddViewState("games", DEFAULT_VIEW_AUTO);

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

  std::unique_lock<CCriticalSection> lock(m_critical);
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
      if (XMLUtils::GetInt(pViewState, XML_SORTMETHOD, sortMethod, SortByNone, SortByLastUsed))
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
    if (XMLUtils::GetInt(pElement, XML_SETTINGLEVEL, settingLevel, static_cast<int>(SettingLevel::Basic), static_cast<int>(SettingLevel::Expert)))
      m_settingLevel = (SettingLevel)settingLevel;
    else
      m_settingLevel = SettingLevel::Standard;

    const TiXmlNode* pEventLogNode = pElement->FirstChild(XML_EVENTLOG);
    if (pEventLogNode != NULL)
    {
      int eventLevel;
      if (XMLUtils::GetInt(pEventLogNode, XML_EVENTLOG_LEVEL, eventLevel, static_cast<int>(EventLevel::Basic), static_cast<int>(EventLevel::Error)))
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

  std::unique_lock<CCriticalSection> lock(m_critical);
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
  m_settingLevel = SettingLevel::Standard;
}

const CViewState* CViewStateSettings::Get(const std::string &viewState) const
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  std::map<std::string, CViewState*>::const_iterator view = m_viewStates.find(viewState);
  if (view != m_viewStates.end())
    return view->second;

  return NULL;
}

CViewState* CViewStateSettings::Get(const std::string &viewState)
{
  std::unique_lock<CCriticalSection> lock(m_critical);
  std::map<std::string, CViewState*>::iterator view = m_viewStates.find(viewState);
  if (view != m_viewStates.end())
    return view->second;

  return NULL;
}

void CViewStateSettings::SetSettingLevel(SettingLevel settingLevel)
{
  if (settingLevel < SettingLevel::Basic)
    m_settingLevel = SettingLevel::Basic;
  if (settingLevel > SettingLevel::Expert)
    m_settingLevel = SettingLevel::Expert;
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
  if (level > SettingLevel::Expert)
    level = SettingLevel::Basic;
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

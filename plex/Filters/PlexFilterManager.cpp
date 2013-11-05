#include "PlexFilterManager.h"
#include "PlexDirectory.h"
#include "FileItem.h"
#include "PlexUtils.h"
#include "GUIMessage.h"
#include "PlexTypes.h"
#include "GUIWindowManager.h"
#include "Key.h"
#include "PlexSectionFilter.h"
#include "File.h"
#include "SpecialProtocol.h"

#include "XBMCTinyXML.h"

#define PLEX_FILTER_MANAGER_XML_PATH "special://profile/plexfiltermanager.xml"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexFilterManager::CPlexFilterManager()
{
  /* myplex playlist filters */
  m_myPlexPlaylistFilter = CPlexSectionFilterPtr(new CPlexMyPlexPlaylistFilter(CURL("plexserver://myplex/pms/playlists")));
  m_filtersMap["plexserver://myplex/pms/playlists"] = m_myPlexPlaylistFilter;

  loadFiltersFromDisk();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexFilterManager::loadFiltersFromDisk()
{
  CLog::Log(LOGDEBUG, "CPlexFilterManager::loadFiltersFromDisk loading from %s", CSpecialProtocol::TranslatePath(PLEX_FILTER_MANAGER_XML_PATH).c_str());
  if (!XFILE::CFile::Exists(PLEX_FILTER_MANAGER_XML_PATH))
    return;

  CXBMCTinyXML doc;
  doc.LoadFile(PLEX_FILTER_MANAGER_XML_PATH);
  if (doc.RootElement())
  {
    TiXmlElement *root = doc.RootElement();
    TiXmlElement *section = root->FirstChildElement();

    while(section)
    {
      std::string url;
      if (section->QueryStringAttribute("url", &url) == TIXML_SUCCESS)
      {
        CLog::Log(LOGDEBUG, "CPlexFilterManager::loadFiltersFromDisk loading filters for section %s", url.c_str());

        CPlexSectionFilterPtr filter;
        if ("plexserver://myplex/pms/playlists" == url)
          filter = m_myPlexPlaylistFilter;
        else
          filter = CPlexSectionFilterPtr(new CPlexSectionFilter(CURL(url)));

        std::string primaryFilter;
        if(section->QueryStringAttribute("primaryFilter", &primaryFilter) == TIXML_SUCCESS)
          filter->setPrimaryFilter(primaryFilter);

        std::string sortOrder;
        if (section->QueryStringAttribute("sortOrder", &sortOrder) == TIXML_SUCCESS)
          filter->setSortOrder(sortOrder);

        bool sortOrderAsc;
        if (section->QueryBoolAttribute("sortOrderAscending", &sortOrderAsc) == TIXML_SUCCESS)
          filter->setSortOrderAscending(sortOrderAsc);


        TiXmlElement *secondaryFilter = section->FirstChildElement();
        while (secondaryFilter)
        {
          std::string name, key, values, title;
          int type;

          if (secondaryFilter->QueryIntAttribute("type", &type) == TIXML_SUCCESS &&
              secondaryFilter->QueryStringAttribute("name", &name) == TIXML_SUCCESS &&
              secondaryFilter->QueryStringAttribute("values", &values) == TIXML_SUCCESS &&
              secondaryFilter->QueryStringAttribute("key", &key) == TIXML_SUCCESS &&
              secondaryFilter->QueryStringAttribute("title", &title) == TIXML_SUCCESS)
          {
            CLog::Log(LOGDEBUG, "CPlexFilterManager::loadFiltersFromDisk added secondary filter %s %d => %s", title.c_str(), type, values.c_str());
            CPlexSecondaryFilterPtr secFilter = CPlexSecondaryFilterPtr(new CPlexSecondaryFilter(name, key, title, (CPlexSecondaryFilter::SecondaryFilterType)type));
            if (type == CPlexSecondaryFilter::FILTER_TYPE_BOOLEAN)
              secFilter->setSelected(true);
            else
              secFilter->setSelected(values);
            filter->addSecondaryFilter(secFilter);
          }

          secondaryFilter = secondaryFilter->NextSiblingElement();
        }

        m_filtersMap[url] = filter;
      }
      section = section->NextSiblingElement();
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexFilterManager::saveFiltersToDisk()
{
  CSingleLock lk(m_filterSection);
  CXBMCTinyXML doc;

  TiXmlDeclaration decl("1.0", "utf-8", "");
  doc.InsertEndChild(decl);

  TiXmlElement root("FilterManager");

  std::pair<std::string, CPlexSectionFilterPtr> p;
  BOOST_FOREACH(p, m_filtersMap)
  {
    TiXmlElement section("Section");
    section.SetAttribute("url", p.first);
    section.SetAttribute("primaryFilter", p.second->currentPrimaryFilter());
    section.SetAttribute("sortOrder", p.second->currentSortOrder());
    section.SetAttribute("sortOrderAscending", p.second->currentSortOrderAscending());

    std::vector<CPlexSecondaryFilterPtr> secFilter = p.second->currentSecondaryFilters();
    BOOST_FOREACH(CPlexSecondaryFilterPtr filter, secFilter)
    {
      TiXmlElement filterEl("SecondaryFilter");
      std::pair<std::string, std::string> fp = filter->getFilterKeyValue();
      filterEl.SetAttribute("key", filter->getFilterKey());
      filterEl.SetAttribute("values", fp.second);
      filterEl.SetAttribute("type", filter->getFilterType());
      filterEl.SetAttribute("name", filter->getFilterName());
      filterEl.SetAttribute("title", filter->getFilterTitle());
      section.InsertEndChild(filterEl);
    }

    root.InsertEndChild(section);
  }

  doc.InsertEndChild(root);
  doc.SaveFile(PLEX_FILTER_MANAGER_XML_PATH);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexSectionFilterPtr CPlexFilterManager::getFilterForSection(const std::string &sectionUrl)
{
  if (sectionUrl == m_myPlexPlaylistFilter->getFilterUrl())
    return m_myPlexPlaylistFilter;

  CSingleLock lk(m_filterSection);
  if (m_filtersMap.find(sectionUrl) != m_filtersMap.end())
    return m_filtersMap[sectionUrl];

  return CPlexSectionFilterPtr();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexFilterManager::loadFilterForSection(const std::string &sectionUrl, bool forceReload)
{
  CSingleLock lk(m_filterSection);

  CPlexSectionFilterPtr filter;
  if (m_filtersMap.find(sectionUrl) != m_filtersMap.end())
    filter = m_filtersMap[sectionUrl];

  if (filter && filter->isLoaded() && !forceReload)
  {
    onFilterLoaded(sectionUrl);
    return;
  }
  else if (!filter)
    filter = CPlexSectionFilterPtr(new CPlexSectionFilter(CURL(sectionUrl)));

  CJobManager::GetInstance().AddJob(new CPlexSectionFilterLoadJob(filter), this, CJob::PRIORITY_LOW);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexFilterManager::onFilterLoaded(const std::string &sectionUrl)
{
  CGUIMessage msg(GUI_MSG_FILTER_LOADED, PLEX_FILTER_MANAGER, 0, 0, 0);
  msg.SetStringParam(sectionUrl);
  g_windowManager.SendThreadMessage(msg);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexFilterManager::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  CPlexSectionFilterLoadJob *ljob = static_cast<CPlexSectionFilterLoadJob*>(job);
  if (ljob && success)
  {
    CPlexSectionFilterPtr filter = ljob->m_sectionFilter;
    CSingleLock lk(m_filterSection);
    if (m_filtersMap.find(filter->getFilterUrl()) == m_filtersMap.end())
      m_filtersMap[filter->getFilterUrl()] = filter;
    onFilterLoaded(filter->getFilterUrl());
  }
}


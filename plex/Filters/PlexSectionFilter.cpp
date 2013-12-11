#include "PlexSectionFilter.h"
#include "PlexUtils.h"

#include "PlexDirectory.h"
#include "FileItem.h"
#include "URL.h"

#include "PlexApplication.h"
#include "Client/PlexServerDataLoader.h"

#include "LocalizeStrings.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexSectionFilter::CPlexSectionFilter(const CURL &sectionUrl) : m_sectionUrl(sectionUrl)
{
  m_currentPrimaryFilter = "all";
  m_currentSortOrder = "titleSort";
  m_currentSortOrderAscending = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexSectionFilter::loadFilters()
{
  XFILE::CPlexDirectory dir;
  CFileItemList list;

  CLog::Log(LOGDEBUG, "CPlexSectionFilter::loadFilters loading filters from section %s", m_sectionUrl.Get().c_str());

  bool advancedFilters = g_plexApplication.dataLoader->SectionHasFilters(m_sectionUrl);
  EPlexDirectoryType type = g_plexApplication.dataLoader->GetSectionType(m_sectionUrl);

  /* get primary filters */
  CURL fURL(m_sectionUrl);
  if (dir.GetDirectory(fURL.Get(), list))
  {
    for (int i = 0; i < list.Size(); i ++)
    {
      CFileItemPtr primaryFilter = list.Get(i);

      if (advancedFilters && primaryFilter->GetProperty("unprocessed_key").asString() == "folder" &&
          type != PLEX_DIR_TYPE_HOME_MOVIES)
        continue;

      if (advancedFilters && (type == PLEX_DIR_TYPE_MOVIE))
      {
        if (primaryFilter->GetProperty("unprocessed_key").asString() == "all" ||
            primaryFilter->GetProperty("unprocessed_key").asString() == "onDeck" ||
            primaryFilter->GetProperty("unprocessed_key").asString() == "folder")
          m_primaryFilters[primaryFilter->GetProperty("unprocessed_key").asString()] = primaryFilter->GetLabel();
      }
      else
      {
        if (!primaryFilter->GetProperty("secondary").asBoolean() &&
            !primaryFilter->GetProperty("search").asBoolean() &&
            primaryFilter->GetProperty("unprocessed_key").asString() != "unwatched")
          m_primaryFilters[primaryFilter->GetProperty("unprocessed_key").asString()] = primaryFilter->GetLabel();
      }
    }
  }

  if (!advancedFilters)
  {
    CLog::Log(LOGDEBUG, "CPlexSectionFilter::loadFilters section %s doesn't have filters...", m_sectionUrl.Get().c_str());
    return true;
  }

  list.Clear();

  /* and now the secondaries */
  if (type != PLEX_DIR_TYPE_HOME_MOVIES)
  {
    PlexUtils::AppendPathToURL(fURL, "filters");
    if (dir.GetDirectory(fURL.Get(), list))
    {
      for (int i = 0; i < list.Size(); i ++)
      {
        CFileItemPtr filter = list.Get(i);
        CPlexSecondaryFilterPtr secondaryFilter = CPlexSecondaryFilter::secondaryFilterFromItem(filter);
        if (secondaryFilter)
        {
          /* we might already have this filter in our list because it was used last time
         * and saved to the state file */
          BOOST_FOREACH(CPlexSecondaryFilterPtr filter, m_currentSecondaryFilters)
          {
            if (filter->getFilterKey() == secondaryFilter->getFilterKey())
            {
              secondaryFilter = filter;
              break;
            }
          }

          m_secondaryFilters[secondaryFilter->getFilterKey()] = secondaryFilter;

          /* if this is a selected filter it probably comes from the
         * XML file at this point, so we need to load it values to
         * have something nice to show in the UI */
          if (secondaryFilter->isSelected())
            secondaryFilter->loadValues();
        }
      }
    }

    /* and now sorts */
    list.Clear();
    fURL = m_sectionUrl;
    PlexUtils::AppendPathToURL(fURL, "sorts");
    if (dir.GetDirectory(fURL.Get(), list))
    {
      for (int i = 0; i < list.Size(); i ++)
      {
        CFileItemPtr sort = list.Get(i);
        m_sortOrders[sort->GetProperty("unprocessed_key").asString()] = sort->GetProperty("title").asString();
        if (sort->HasProperty("default") && m_currentSortOrder.empty())
        {
          m_currentSortOrder = sort->GetProperty("unprocessed_key").asString();
          m_currentSortOrderAscending = sort->GetProperty("default").asString() == "asc" ? true : false;
        }
      }
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CUrlOptions CPlexSectionFilter::getFilterOptions()
{
  CUrlOptions options;
  BOOST_FOREACH(CPlexSecondaryFilterPtr filter, m_currentSecondaryFilters)
  {
    std::pair<std::string, std::string> kv = filter->getFilterKeyValue();
    options.AddOption(kv.first, kv.second);
  }
  return options;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexSectionFilter::hasActiveSecondaryFilters() const
{
  return m_currentSecondaryFilters.size() > 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CPlexSectionFilter::addFiltersToUrl(const CURL &baseUrl)
{
  CURL nu(baseUrl);

  PlexUtils::AppendPathToURL(nu, m_currentPrimaryFilter);
  nu.AddOptions(getFilterOptions());
  nu.SetOption("sort", m_currentSortOrder + ":" + (m_currentSortOrderAscending ? "asc" : "desc"));

  return nu;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexSectionFilter::addSecondaryFilter(CPlexSecondaryFilterPtr secFilter)
{
  if (secFilter->isSelected())
  {
    if (std::find(m_currentSecondaryFilters.begin(), m_currentSecondaryFilters.end(), secFilter) == m_currentSecondaryFilters.end())
      m_currentSecondaryFilters.push_back(secFilter);
  }
  else
  {
    if (std::find(m_currentSecondaryFilters.begin(), m_currentSecondaryFilters.end(), secFilter) != m_currentSecondaryFilters.end())
      m_currentSecondaryFilters.erase(std::find(m_currentSecondaryFilters.begin(), m_currentSecondaryFilters.end(), secFilter), m_currentSecondaryFilters.end());
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexSecondaryFilterPtr CPlexSectionFilter::addSecondaryFilter(const std::string &filterKey)
{
  if (m_secondaryFilters.find(filterKey) == m_secondaryFilters.end())
  {
    CLog::Log(LOGDEBUG, "CPlexSectionFilter::addSecondaryFilter asked to add %s but it was not loaded!?", filterKey.c_str());
    return CPlexSecondaryFilterPtr();
  }

  CPlexSecondaryFilterPtr secFilter = m_secondaryFilters[filterKey];
  addSecondaryFilter(secFilter);

  return secFilter;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexSectionFilter::loadFilterValues(CPlexSecondaryFilterPtr secFilter)
{
  secFilter->loadValues(getFilterOptions());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexMyPlexPlaylistFilter::CPlexMyPlexPlaylistFilter(const CURL &sectionUrl) : CPlexSectionFilter(sectionUrl)
{
  m_currentPrimaryFilter = "queue";
  m_currentSortOrder = "";

  m_primaryFilters["queue"] = g_localizeStrings.Get(44021);
  m_primaryFilters["recommendations"] = g_localizeStrings.Get(44022);

  CPlexSecondaryFilterPtr unwatchedFilter = CPlexSecondaryFilterPtr(new CPlexSecondaryFilter("Unwatched", "unwatched", "Unwatched", CPlexSecondaryFilter::FILTER_TYPE_BOOLEAN));
  m_secondaryFilters["unwatched"] = unwatchedFilter;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CURL CPlexMyPlexPlaylistFilter::addFiltersToUrl(const CURL &baseUrl)
{
  CURL ret(baseUrl);
  PlexUtils::AppendPathToURL(ret, m_currentPrimaryFilter);

  bool unwatched = false;
  BOOST_FOREACH(CPlexSecondaryFilterPtr filter, m_currentSecondaryFilters)
  {
    if (filter->getFilterKey() == "unwatched" && filter->isSelected())
    {
      PlexUtils::AppendPathToURL(ret, filter->getFilterKey());
      unwatched = true;
    }
  }

  if (!unwatched)
    PlexUtils::AppendPathToURL(ret, "all");

  return ret;
}

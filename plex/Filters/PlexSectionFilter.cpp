#include "PlexSectionFilter.h"
#include "PlexUtils.h"

#include "PlexDirectory.h"
#include "FileItem.h"
#include "URL.h"

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


  /* get primary filters */
  CURL fURL(m_sectionUrl);
  if (dir.GetDirectory(fURL.Get(), list))
  {
    for (int i = 0; i < list.Size(); i ++)
    {
      CFileItemPtr primaryFilter = list.Get(i);
      if (!primaryFilter->GetProperty("secondary").asBoolean() &&
          !primaryFilter->GetProperty("search").asBoolean() &&
          primaryFilter->GetProperty("unprocessed_key").asString() != "unwatched")
        m_primaryFilters[primaryFilter->GetProperty("unprocessed_key").asString()] = primaryFilter->GetLabel();
    }
  }

  list.Clear();

  /* and now the secondaries */
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

  return false;
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

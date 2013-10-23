#ifndef PLEXSECTIONFILTER_H
#define PLEXSECTIONFILTER_H

#include <vector>
#include "PlexSecondaryFilter.h"
#include "URL.h"
#include "UrlOptions.h"

#include <boost/foreach.hpp>

class CPlexSectionFilter;
typedef boost::shared_ptr<CPlexSectionFilter> CPlexSectionFilterPtr;

class CPlexSectionFilter
{
  public:
    CPlexSectionFilter(const CURL& sectionUrl);
    bool loadFilters();

    CUrlOptions getFilterOptions();
    CURL addFiltersToUrl(const CURL& baseUrl);

    PlexStringPairVector getPrimaryFilters() const
    {
      PlexStringPairVector filters;
      BOOST_FOREACH(PlexStringPair p, m_primaryFilters)
        filters.push_back(p);
      return filters;
    }

    std::vector<CPlexSecondaryFilterPtr> getSecondaryFilters() const
    {
      std::vector<CPlexSecondaryFilterPtr> filters;
      std::pair<std::string, CPlexSecondaryFilterPtr> f;
      BOOST_FOREACH(f, m_secondaryFilters)
        filters.push_back(f.second);
      return filters;
    }

    PlexStringPairVector getSortOrders() const
    {
      PlexStringPairVector sorts;
      BOOST_FOREACH(PlexStringPair p, m_sortOrders)
        sorts.push_back(p);
      return sorts;
    }

    void setPrimaryFilter(const std::string& primaryFilter) { m_currentPrimaryFilter = primaryFilter; }
    std::string currentPrimaryFilter() const { return m_currentPrimaryFilter; }

    std::string currentSortOrder() const { return m_currentSortOrder; }
    bool currentSortOrderAscending() const { return m_currentSortOrderAscending; }

    void setSortOrder(const std::string& sortorder) { m_currentSortOrder = sortorder; }
    void setSortOrderAscending(bool asc) { m_currentSortOrderAscending = asc; }

    std::vector<CPlexSecondaryFilterPtr> currentSecondaryFilters() const { return m_currentSecondaryFilters; }

    std::string getFilterUrl() const { return m_sectionUrl.Get(); }
    bool isLoaded() const { return m_secondaryFilters.size() > 0; }

    void addSecondaryFilter(CPlexSecondaryFilterPtr secFilter);
    CPlexSecondaryFilterPtr addSecondaryFilter(const std::string& filterKey);

    void loadFilterValues(CPlexSecondaryFilterPtr secFilter);

    void clearFilters()
    {
      BOOST_FOREACH(CPlexSecondaryFilterPtr filter, m_currentSecondaryFilters)
        filter->clearFilters();

      m_currentSecondaryFilters.clear();
    }

  private:
    CURL m_sectionUrl;
    PlexStringMap m_primaryFilters;
    std::map<std::string, CPlexSecondaryFilterPtr> m_secondaryFilters;
    PlexStringMap m_sortOrders;

    std::string m_currentSortOrder;
    bool m_currentSortOrderAscending;

    std::string m_currentPrimaryFilter;
    std::vector<CPlexSecondaryFilterPtr> m_currentSecondaryFilters;
};

#endif // PLEXSECTIONFILTER_H

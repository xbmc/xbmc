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
    virtual bool loadFilters();
    virtual CURL addFiltersToUrl(const CURL& baseUrl);
    virtual bool hasActiveSecondaryFilters() const;

    CUrlOptions getFilterOptions();

    virtual PlexStringPairVector getPrimaryFilters() const
    {
      PlexStringPairVector filters;
      BOOST_FOREACH(PlexStringPair p, m_primaryFilters)
        filters.push_back(p);
      return filters;
    }

    virtual std::vector<CPlexSecondaryFilterPtr> getSecondaryFilters() const
    {
      std::vector<CPlexSecondaryFilterPtr> filters;
      std::pair<std::string, CPlexSecondaryFilterPtr> f;
      BOOST_FOREACH(f, m_secondaryFilters)
        filters.push_back(f.second);
      return filters;
    }

    virtual PlexStringPairVector getSortOrders() const
    {
      PlexStringPairVector sorts;
      BOOST_FOREACH(PlexStringPair p, m_sortOrders)
        sorts.push_back(p);
      return sorts;
    }

    virtual void setPrimaryFilter(const std::string& primaryFilter) { m_currentPrimaryFilter = primaryFilter; }
    virtual std::string currentPrimaryFilter() const { return m_currentPrimaryFilter; }

    virtual std::string currentSortOrder() const { return m_currentSortOrder; }
    virtual bool currentSortOrderAscending() const { return m_currentSortOrderAscending; }

    virtual void setSortOrder(const std::string& sortorder) { m_currentSortOrder = sortorder; }
    virtual void setSortOrderAscending(bool asc) { m_currentSortOrderAscending = asc; }

    virtual std::vector<CPlexSecondaryFilterPtr> currentSecondaryFilters() const { return m_currentSecondaryFilters; }

    virtual std::string getFilterUrl() const { return m_sectionUrl.Get(); }
    virtual bool isLoaded() const { return m_primaryFilters.size() > 0; }
    virtual bool hasAdvancedFilters() const { return (m_secondaryFilters.size() > 0 || m_sortOrders.size() > 0); }

    virtual void addSecondaryFilter(CPlexSecondaryFilterPtr secFilter);
    virtual CPlexSecondaryFilterPtr addSecondaryFilter(const std::string& filterKey);

    virtual void loadFilterValues(CPlexSecondaryFilterPtr secFilter);

    CPlexSecondaryFilterPtr getSecondaryFilterOfName(const std::string& name)
    {
      BOOST_FOREACH(CPlexSecondaryFilterPtr filter, getSecondaryFilters())
      {
        if (filter->getFilterName() == name)
          return filter;
      }
      return CPlexSecondaryFilterPtr();
    }

    bool needRefreshOnStateChange()
    {
      CPlexSecondaryFilterPtr unwatched = getSecondaryFilterOfName("unwatched");

      if (unwatched && unwatched->isSelected())
        return true;

      if (currentPrimaryFilter() == "onDeck")
        return true;

      return false;
    }

    virtual void clearFilters()
    {
      BOOST_FOREACH(CPlexSecondaryFilterPtr filter, m_currentSecondaryFilters)
        filter->clearFilters();

      m_currentSecondaryFilters.clear();
    }

    virtual bool secondaryFiltersActivated() const
    {
      if (m_currentPrimaryFilter != "all")
        return false;
      return true;
    }

  protected:
    CURL m_sectionUrl;
    PlexStringMap m_primaryFilters;
    std::map<std::string, CPlexSecondaryFilterPtr> m_secondaryFilters;
    PlexStringMap m_sortOrders;

    std::string m_currentSortOrder;
    bool m_currentSortOrderAscending;

    std::string m_currentPrimaryFilter;
    std::vector<CPlexSecondaryFilterPtr> m_currentSecondaryFilters;
};

class CPlexMyPlexPlaylistFilter : public CPlexSectionFilter
{
  public:
    CPlexMyPlexPlaylistFilter(const CURL& sectionUrl);

    virtual CURL addFiltersToUrl(const CURL& baseUrl);
    virtual void loadFilterValues(CPlexSecondaryFilterPtr secFilter) { };
    virtual bool hasAdvancedFilters() const { return true; }
    virtual bool isLoaded() const { return true; }
    virtual bool loadFilters() { return true; }
    virtual bool secondaryFiltersActivated() const { return true; }
};

#endif // PLEXSECTIONFILTER_H

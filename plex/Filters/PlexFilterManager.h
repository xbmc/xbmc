#ifndef PLEXFILTERMANAGER_H
#define PLEXFILTERMANAGER_H

#include <boost/shared_ptr.hpp>
#include <vector>
#include "URL.h"
#include "threads/CriticalSection.h"
#include "JobManager.h"
#include "PlexSectionFilter.h"

class CPlexSectionFilterLoadJob : public CJob
{
  public:
    CPlexSectionFilterLoadJob(CPlexSectionFilterPtr filter) : m_sectionFilter(filter) {}
    bool DoWork()
    {
      m_sectionFilter->loadFilters();
      return m_sectionFilter->isLoaded();
    }

    CPlexSectionFilterPtr m_sectionFilter;
};

class CPlexFilterManager : public IJobCallback
{
  public:
    CPlexFilterManager();
    void loadFilterForSection(const std::string& sectionUrl, bool forceReload = false);
    CPlexSectionFilterPtr getFilterForSection(const std::string& sectionUrl);

    void saveFiltersToDisk();
  private:
    void onFilterLoaded(const std::string &sectionUrl);

    virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job);

    CCriticalSection m_filterSection;
    std::map<std::string, CPlexSectionFilterPtr> m_filtersMap;
    void loadFiltersFromDisk();

    CPlexSectionFilterPtr m_myPlexPlaylistFilter;
};

typedef boost::shared_ptr<CPlexFilterManager> CPlexFilterManagerPtr;

#endif // PLEXFILTERMANAGER_H

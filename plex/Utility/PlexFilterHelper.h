//
//  PlexFilterHelper.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-05.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXFILTERHELPER_H
#define PLEXFILTERHELPER_H

#include "PlexFilter.h"
#include "windows/GUIMediaWindow.h"

#define FILTER_LIST     19000
#define FILTER_BUTTON   19001
#define FILTER_RADIO_BUTTON 19002
#define FILTER_ACTIVE_LABEL 19003

#define SORT_LIST       19010
#define SORT_RADIO_BUTTON 19011

#define FILTER_BUTTONS_START -100
#define SORT_BUTTONS_START -200

#define FILTER_LABEL 19009
#define SORT_LABEL 19019

class CPlexFilterHelper
{
  public:
    CPlexFilterHelper(CGUIMediaWindow* mediaWindow) : m_appliedSort(""), m_sortDirectionAsc(true)
    {
      m_mediaWindow = mediaWindow;
    }

    CStdString GetRealDirectoryUrl(const CStdString& strDirectory, bool& secondary);
    void ClearFilters();
    bool FetchFilterSortList(const CStdString& url, const CStdString& filterSort, int type, CFileItemList& list);
    void BuildFilters(const CStdString& baseUrl, EPlexDirectoryType = PLEX_DIR_TYPE_UNKNOWN);

    bool ApplyFilter(int ctrlId);
    bool ApplySort(int ctrlId);

    void ApplyFilterFromDialog(CPlexFilterPtr filter);

    CStdString GetFilterUrl(const CStdString& exclude="", const CStdString& baseUrl="") const;

    int GetWindowID() const { return m_mediaWindow->GetID(); }

    void SetSectionUrl(const CStdString& url) { m_sectionUrl = url; }
    CStdString GetSectionUrl() const { return m_sectionUrl; }

  private:
    std::map<CStdString, CPlexFilterPtr> m_filters;
    std::map<CStdString, CPlexFilterPtr> m_sorts;
    std::map<CStdString, std::string> m_appliedFilters;
    CStdString m_appliedSort;
    bool m_sortDirectionAsc;
    CPlexFilterPtr m_openFilter;
    EPlexDirectoryType m_filterType;

    CStdString m_sectionUrl;
    CStdString m_mapToSection;

    CGUIMediaWindow* m_mediaWindow;
};

#endif // PLEXFILTERHELPER_H

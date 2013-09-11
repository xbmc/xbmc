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

    CURL GetRealDirectoryUrl(const CStdString& strDirectory, bool& secondary);
    void ClearFilters();
    void BuildFilters(const CURL& baseUrl, EPlexDirectoryType = PLEX_DIR_TYPE_UNKNOWN);

    bool ApplyFilter(int ctrlId);
    bool ApplySort(int ctrlId);
    bool SkinHasFilters() const;

    void ApplyFilterFromDialog(CPlexFilterPtr filter);

    CURL GetFilterUrl(const CStdString& exclude="", const CURL& baseUrl=CURL()) const;

    int GetWindowID() const { return m_mediaWindow->GetID(); }

    void SetSectionUrl(const CStdString& url) { m_sectionUrl = url; }
    CURL GetSectionUrl() const { return m_sectionUrl; }

  private:
    std::map<CStdString, CPlexFilterPtr> m_filters;
    std::map<CStdString, CPlexFilterPtr> m_sorts;
  
    /* name = value */
    std::map<CStdString, CStdString> m_appliedFilters;
  
    CStdString m_appliedSort;
    bool m_sortDirectionAsc;
    CPlexFilterPtr m_openFilter;
    EPlexDirectoryType m_filterType;

    CURL m_sectionUrl;
    CURL m_mapToSection;

    CGUIMediaWindow* m_mediaWindow;
};

#endif // PLEXFILTERHELPER_H

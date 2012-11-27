//
//  GUIWindowMediaFilterView.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-19.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef GUIWINDOWMEDIAFILTERVIEW_H
#define GUIWINDOWMEDIAFILTERVIEW_H

#include "video/windows/GUIWindowVideoNav.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "StringUtils.h"
#include "GUI/PlexFilter.h"

#define FILTER_LIST     19000
#define FILTER_BUTTON   19001
#define FILTER_RADIO_BUTTON 19002
#define FILTER_SPIN_CONTROL 19003

#define SORT_LIST       19010
#define SORT_RADIO_BUTTON 19011

#define FILTER_BUTTONS_START -100
#define SORT_BUTTONS_START -200

#define FILTER_LABEL 19009
#define SORT_LABEL 19019

class CGUIWindowMediaFilterView : public CGUIWindowVideoNav
{    
  public:
    CGUIWindowMediaFilterView() : CGUIWindowVideoNav(), m_appliedSort(""), m_sortDirectionAsc(true), m_returningFromSkinLoad(false) {};
    bool OnMessage(CGUIMessage &message);
  protected:
    bool Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters);
    bool Update(const CStdString &strDirectory, bool updateFilterPath);
    void BuildFilters(const CStdString &url, int type);
    bool FetchFilterSortList(const CStdString& url, const CStdString& filterSort, int type, CFileItemList& list);
    void PopulateSublist(CPlexFilterPtr filter);
    CStdString GetFilterUrl(const CStdString& exclude="", const CStdString& baseUrl="") const;

  private:
    std::map<CStdString, CPlexFilterPtr> m_filters;
    std::map<CStdString, CPlexFilterPtr> m_sorts;
    CStdString m_baseUrl;
    std::map<CStdString, std::string> m_appliedFilters;
    CStdString m_appliedSort;
    bool m_sortDirectionAsc;
    bool m_returningFromSkinLoad;
    CPlexFilterPtr m_openFilter;
};

#endif // GUIWINDOWMEDIAFILTERVIEW_H

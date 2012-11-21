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

#define FILTER_LIST     19000
#define FILTER_BUTTON   19001
#define FILTER_RADIO_BUTTON 19002
#define FILTER_SPIN_CONTROL 19003

#define SORT_LIST       19010

class CGUIWindowMediaFilterView : public CGUIWindowVideoNav
{
  protected:
    bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
    void BuildFilters(const CStdString &url, int type);
    bool FetchFilterSortList(const CStdString& url, const CStdString& filterSort, int type, CFileItemList& list);
};

#endif // GUIWINDOWMEDIAFILTERVIEW_H

//
//  GUIDialogFilterSort.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-26.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef GUIDIALOGFILTERSORT_H
#define GUIDIALOGFILTERSORT_H

#define FILTER_SUBLIST 19020
#define FILTER_SUBLIST_BUTTON 19021
#define FILTER_SUBLIST_LABEL 19029
#define FILTER_SUBLIST_BUTTONS_START -300

#include "guilib/GUIDialog.h"
#include "Utility/PlexFilter.h"
#include "FileItem.h"
#include "Utility/PlexFilterHelper.h"
#include <map>

class CGUIDialogFilterSort : public CGUIDialog
{
  public:
    CGUIDialogFilterSort();

    void SetFilter(CPlexFilterPtr filter);
    void SetFilterHelper(CPlexFilterHelper* helper) { m_helper = helper; }
    void DoModal(int iWindowID = WINDOW_INVALID, const CStdString &param = "");
    bool OnMessage(CGUIMessage &message);

  private:
    CPlexFilterPtr m_filter;
    std::map<int, CGUIRadioButtonControl*> m_filterIdMap;
    std::map<int, CFileItemPtr> m_itemIdMap;
    CPlexFilterHelper* m_helper;
};

#endif // GUIDIALOGFILTERSORT_H

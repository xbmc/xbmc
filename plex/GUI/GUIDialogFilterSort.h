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
#define FILTER_SUBLIST_RADIO_BUTTON 19021
#define FILTER_SUBLIST_BUTTON 19022
#define FILTER_SUBLIST_LABEL 19029
#define FILTER_SUBLIST_BUTTONS_START -300
#define FILTER_SUBLIST_CLEAR_FILTERS -99

#include "guilib/GUIDialog.h"
#include "FileItem.h"
#include <map>
#include "Filters/PlexSecondaryFilter.h"
#include "guilib/GUIRadioButtonControl.h"

typedef std::pair<PlexStringPair, CGUIRadioButtonControl*> filterControl;

class CGUIDialogFilterSort : public CGUIDialog
{
  public:
    CGUIDialogFilterSort();
    void SetFilter(CPlexSecondaryFilterPtr filter, int filterButtonId);
    bool OnMessage(CGUIMessage &message);
    bool OnAction(const CAction &action);

  private:
    CGUIButtonControl *m_clearFilters;
    CPlexSecondaryFilterPtr m_filter;
    std::map<int, filterControl> m_filterMap;
    int m_filterButtonId;
};

#endif // GUIDIALOGFILTERSORT_H

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

class CGUIWindowMediaFilterView : public CGUIWindowVideoNav
{
  public:
    void OnInitWindow();
};

#endif // GUIWINDOWMEDIAFILTERVIEW_H

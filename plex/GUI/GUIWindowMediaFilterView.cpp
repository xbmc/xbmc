//
//  GUIWindowMediaFilterView.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-19.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "GUIWindowMediaFilterView.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIButtonControl.h"

void CGUIWindowMediaFilterView::OnInitWindow()
{
  CGUIWindowVideoNav::OnInitWindow();

  CLog::Log(LOGDEBUG, "Hello from FilterView");

  CGUIControlGroupList *group = (CGUIControlGroupList *)GetControl(FILTER_LIST);
  if (!group)
    return;

  CGUIButtonControl* originalButton = (CGUIButtonControl *)GetControl(FILTER_BUTTON);
  originalButton->SetVisible(false);

  CGUIButtonControl* filter1 = new CGUIButtonControl(*originalButton);
  filter1->SetLabel("Filter 1");
  filter1->SetVisible(true);
  filter1->SetID(-100);
  filter1->AllocResources();

  group->AddControl(filter1);
}

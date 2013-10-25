//
//  GUIDialogFilterSort.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-26.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "GUIDialogFilterSort.h"
#include "plex/PlexTypes.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUILabelControl.h"
#include "GUIWindowManager.h"

CGUIDialogFilterSort::CGUIDialogFilterSort()
  : CGUIDialog(WINDOW_DIALOG_FILTER_SORT, "DialogFilters.xml")
{
  m_loadType = LOAD_ON_GUI_INIT;
}

void CGUIDialogFilterSort::SetFilter(CPlexSecondaryFilterPtr filter, int filterButtonId)
{
  m_filter = filter;
  m_filterButtonId = filterButtonId;

  if (!m_filter->hasValues())
  {
    /* We should always have values at this point! */
    return;
  }

  CGUIControlGroupList* list = (CGUIControlGroupList*)GetControl(FILTER_SUBLIST);
  if (!list)
    return;

  list->ClearAll();

  CGUIRadioButtonControl* radioButton = (CGUIRadioButtonControl*)GetControl(FILTER_SUBLIST_BUTTON);
  if (!radioButton)
    return;

  radioButton->SetVisible(false);

  CGUILabelControl* headerLabel = (CGUILabelControl*)GetControl(FILTER_SUBLIST_LABEL);
  if (headerLabel)
    headerLabel->SetLabel(m_filter->getFilterTitle());

  PlexStringPairVector values = m_filter->getFilterValues();

  int id = FILTER_SUBLIST_BUTTONS_START;
  BOOST_FOREACH(PlexStringPair p, values)
  {
    CGUIRadioButtonControl* sublistItem = new CGUIRadioButtonControl(*radioButton);
    sublistItem->SetLabel(p.second);
    sublistItem->SetVisible(true);
    sublistItem->AllocResources();
    sublistItem->SetID(id);
    sublistItem->SetSelected(m_filter->isSelected(p.first));

    filterControl fc;
    fc.first = p;
    fc.second = sublistItem;
    m_filterMap[id] = fc;

    list->AddControl(sublistItem);

    id++;
  }
}

bool CGUIDialogFilterSort::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_CLEAR_FILTERS)
  {
    m_filter->clearFilters();

    for (int i = FILTER_SUBLIST_BUTTONS_START; i < 0; i++)
    {
      CGUIRadioButtonControl* button = (CGUIRadioButtonControl*)GetControl(i);
      if (!button)
        break;

      button->SetSelected(false);
    }

    CGUIMessage msg(GUI_MSG_FILTER_SELECTED, WINDOW_DIALOG_FILTER_SORT, 0, m_filterButtonId, 0);
    msg.SetStringParam(m_filter->getFilterKey());
    g_windowManager.SendThreadMessage(msg, g_windowManager.GetActiveWindow());

    SetInvalid();

    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogFilterSort::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int senderId = message.GetSenderId();
      if (m_filterMap.find(senderId) != m_filterMap.end())
      {
        CGUIRadioButtonControl *filterCtrl = m_filterMap[senderId].second;
        PlexStringPair filterKv = m_filterMap[senderId].first;
        m_filter->setSelected(filterKv.first, filterCtrl->IsSelected());

        CGUIMessage msg(GUI_MSG_FILTER_SELECTED, WINDOW_DIALOG_FILTER_SORT, 0, m_filterButtonId, 0);
        msg.SetStringParam(m_filter->getFilterKey());
        g_windowManager.SendThreadMessage(msg, g_windowManager.GetActiveWindow());

        SetInvalid();
        return true;
      }
    }

  }
  return CGUIDialog::OnMessage(message);
}

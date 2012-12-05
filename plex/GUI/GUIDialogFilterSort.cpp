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

void CGUIDialogFilterSort::SetFilter(CPlexFilterPtr filter)
{
  m_filter = filter;
  m_filterIdMap.clear();
  m_itemIdMap.clear();

  CFileItemList sublist;
  if (!filter->GetSublist(sublist))
    return;

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
    headerLabel->SetLabel(filter->GetFilterString());

  for (int i = 0; i < sublist.Size(); i++)
  {
    CFileItemPtr item = sublist.Get(i);
    CGUIRadioButtonControl* sublistItem = new CGUIRadioButtonControl(*radioButton);
    sublistItem->SetLabel(item->GetLabel());
    sublistItem->SetVisible(true);
    sublistItem->AllocResources();
    sublistItem->SetID(FILTER_SUBLIST_BUTTONS_START + i);

    if (filter->HasCurrentValue(item->GetProperty("unprocessedKey").asString()))
      sublistItem->SetSelected(true);

    m_filterIdMap[FILTER_SUBLIST_BUTTONS_START + i] = sublistItem;
    m_itemIdMap[FILTER_SUBLIST_BUTTONS_START + i] = item;
    list->AddControl(sublistItem);
  }

}

bool CGUIDialogFilterSort::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      int senderId = message.GetSenderId();
      if (m_filterIdMap.find(senderId) != m_filterIdMap.end())
      {
        CGUIRadioButtonControl *filterCtrl = m_filterIdMap[senderId];
        CFileItemPtr item = m_itemIdMap[senderId];
        if (filterCtrl->IsSelected())
        {
          m_filter->AddCurrentValue(item->GetProperty("unprocessedKey").asString());
        }
        else
        {
          m_filter->RemoveCurrentValue(item->GetProperty("unprocessedKey").asString());
        }

        if (m_helper)
          m_helper->ApplyFilterFromDialog(m_filter);

        CGUIMessage msg(GUI_MSG_UPDATE_FILTERS, GetID(), WINDOW_VIDEO_NAV);
        g_windowManager.SendThreadMessage(msg);
      }
    }

  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogFilterSort::DoModal(int iWindowID, const CStdString &param)
{
  CGUIDialog::DoModal(iWindowID, param);
}

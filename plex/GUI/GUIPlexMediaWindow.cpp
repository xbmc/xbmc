//
//  GUIWindowMediaFilterView.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-11-19.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "GUIPlexMediaWindow.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIRadioButtonControl.h"
#include "guilib/GUISpinControlEx.h"
#include "plex/PlexUtils.h"
#include "plex/FileSystem/PlexDirectory.h"
#include "GUIUserMessages.h"
#include "AdvancedSettings.h"
#include "guilib/GUILabelControl.h"
#include "GUI/GUIDialogFilterSort.h"
#include "GUIWindowManager.h"

#include "LocalizeStrings.h"

bool CGUIPlexMediaWindow::OnMessage(CGUIMessage &message)
{
  bool ret = CGUIWindowVideoNav::OnMessage(message);

  switch(message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      bool update = false;
      int ctrlId = message.GetSenderId();
      dprintf("Clicked with CtrlID = %d", ctrlId);
      if (ctrlId < 0 && ctrlId >= FILTER_BUTTONS_START)
      {
        update = m_filterHelper.ApplyFilter(ctrlId);
      }
      else if (ctrlId < FILTER_BUTTONS_START && ctrlId >= SORT_BUTTONS_START)
      {
        update = m_filterHelper.ApplySort(ctrlId);
      }

      if (update)
        Update(m_baseUrl, true, false);
    }
      break;

    case GUI_MSG_LOAD_SKIN:
    {
      /* This is called BEFORE the skin is reloaded, so let's save this event to be handled
       * in WINDOW_INIT instead */
      if (IsActive())
        m_returningFromSkinLoad = true;
    }
      break;

    case GUI_MSG_WINDOW_INIT:
    {
      /* If this is a reload event we must make sure to get the filters back */
      if (m_returningFromSkinLoad)
        Update(m_baseUrl, true);
      m_returningFromSkinLoad = false;

      CGUILabelControl *lbl = (CGUILabelControl*)GetControl(FILTER_LABEL);
      if (lbl)
        lbl->SetLabel(g_localizeStrings.Get(44030));

      lbl = (CGUILabelControl*)GetControl(SORT_LABEL);
      if (lbl)
        lbl->SetLabel(g_localizeStrings.Get(44031));
    }
      break;

    case GUI_MSG_UPDATE_FILTERS:
    {
      Update(m_baseUrl, true, false);
      break;
    }

  }

  return ret;
}

bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  return Update(strDirectory, updateFilterPath, true);
}

bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters)
{

  /* If we changed view we need to make sure that we clear filters before we ask for the new URL */
  if (strDirectory != m_baseUrl)
  {
    m_baseUrl = strDirectory;
    m_filterHelper.ClearFilters();
  }

  bool changedUrl;
  CStdString newUrl = m_filterHelper.GetRealDirectoryUrl(strDirectory, changedUrl);

  bool ret = CGUIWindowVideoNav::Update(newUrl, updateFilterPath);

  if (changedUrl)
  {
    /* Kill the history */
    m_history.ClearPathHistory();
    m_startDirectory = newUrl;

    if (updateFilters)
    {
      int type = 0;
      if (m_vecItems->HasProperty("typeNumber"))
        type = m_vecItems->GetProperty("typeNumber").asInteger();
      m_filterHelper.BuildFilters(m_baseUrl, type);
    }
  }

  return ret;
}

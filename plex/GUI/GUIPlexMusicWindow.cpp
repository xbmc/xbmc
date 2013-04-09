//
//  GUIPlexMusicWindow.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-05.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "GUIPlexMusicWindow.h"
#include "GUIMessage.h"
#include "guilib/GUILabelControl.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"

bool CGUIPlexMusicWindow::OnMessage(CGUIMessage &message)
{
  bool ret = CGUIWindowMusicSongs::OnMessage(message);

  switch(message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      bool update = false;
      int ctrlId = message.GetSenderId();
      if (ctrlId < 0 && ctrlId >= FILTER_BUTTONS_START)
      {
        update = m_filterHelper.ApplyFilter(ctrlId);
      }
      else if (ctrlId < FILTER_BUTTONS_START && ctrlId >= SORT_BUTTONS_START)
      {
        update = m_filterHelper.ApplySort(ctrlId);
      }

      if (update)
        Update(m_filterHelper.GetSectionUrl(), true, false);
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
        Update(m_filterHelper.GetSectionUrl(), true);
      else
        BuildFilters(m_filterHelper.GetSectionUrl());
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
      Update(m_filterHelper.GetSectionUrl(), true, false);
      break;
    }

  }

  return ret;
}

void CGUIPlexMusicWindow::BuildFilters(const CStdString& strDirectory)
{
  if (strDirectory.empty())
    return;
  m_filterHelper.BuildFilters(strDirectory, m_vecItems->GetPlexDirectoryType());
}

bool CGUIPlexMusicWindow::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  return Update(strDirectory, updateFilterPath, true);
}

bool CGUIPlexMusicWindow::Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters)
{
  bool isSecondary;
  CStdString newUrl = m_filterHelper.GetRealDirectoryUrl(strDirectory, isSecondary);

  bool ret = CGUIWindowMusicSongs::Update(newUrl, updateFilterPath);

  if (isSecondary)
  {
    /* Kill the history */
    m_history.ClearPathHistory();
    m_history.AddPath(newUrl);
    m_startDirectory = newUrl;

    if (updateFilters)
      BuildFilters(strDirectory);
  }
  else
  {
    m_history.AddPath(newUrl);
  }

  return ret;
}

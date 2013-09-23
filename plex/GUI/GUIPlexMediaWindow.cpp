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
#include "PlexContentPlayerMixin.h"

#include "LocalizeStrings.h"

#define DEFAULT_PAGE_SIZE 50
#define XMIN(a,b) ((a)<(b)?(a):(b))

bool CGUIPlexMediaWindow::OnMessage(CGUIMessage &message)
{
  bool ret = CGUIWindowVideoNav::OnMessage(message);

  switch(message.GetMessage())
  {
    case GUI_MSG_CLICKED:
    {
      bool update = false;
      int ctrlId = message.GetSenderId();
      if (ctrlId < 0 && ctrlId >= FILTER_BUTTONS_START)
        update = m_filterHelper.ApplyFilter(ctrlId);
      else if (ctrlId < FILTER_BUTTONS_START && ctrlId >= SORT_BUTTONS_START)
        update = m_filterHelper.ApplySort(ctrlId);

      if (update)
        Update(m_filterHelper.GetSectionUrl().Get(), true, false);
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
        Update(m_filterHelper.GetSectionUrl().Get(), true);
      else
        BuildFilter(m_filterHelper.GetSectionUrl());
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
      Update(m_filterHelper.GetSectionUrl().Get(), true, false);
      break;
    }

    case GUI_MSG_ITEM_SELECT:
    {
      int currentIdx = m_viewControl.GetSelectedItem();
      if (currentIdx > m_pagingOffset && m_currentJobId == -1)
      {
        /* the user selected something in the middle of where we loaded, let's just cheat and fill in everything */
        LoadPage(m_pagingOffset, currentIdx + DEFAULT_PAGE_SIZE);
      }
      break;
    }
      
  }

  return ret;
}

bool CGUIPlexMediaWindow::OnAction(const CAction &action)
{
  bool ret = CGUIWindowVideoNav::OnAction(action);

  if ((action.GetID() > ACTION_NONE &&
      action.GetID() <= ACTION_PAGE_DOWN) ||
      action.GetID() >= KEY_ASCII) // KEY_ASCII means that we letterjumped.
  {
    if (m_viewControl.GetSelectedItem() >= m_pagingOffset)
      LoadPage(m_pagingOffset, m_viewControl.GetSelectedItem() + DEFAULT_PAGE_SIZE);
    else if (m_viewControl.GetSelectedItem() >= (m_pagingOffset - (DEFAULT_PAGE_SIZE/2)))
      LoadNextPage();
  }

  return ret;
}

bool CGUIPlexMediaWindow::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  CURL u(strDirectory);
  u.SetProtocolOption("containerStart", "0");
  u.SetProtocolOption("containerSize", boost::lexical_cast<std::string>(DEFAULT_PAGE_SIZE));
  m_pagingOffset = DEFAULT_PAGE_SIZE - 1;
  
  bool ret = CGUIWindowVideoNav::GetDirectory(u.Get(), items);
  
  if (items.HasProperty("totalSize"))
  {
    if (items.GetProperty("totalSize").asInteger() > DEFAULT_PAGE_SIZE)
    {
     
      std::map<int, std::string> charMap;
      if (boost::ends_with(u.GetFileName(), "/all"))
      {
        /* we need the first characters, this is blocking this thread, which is not optimal :( */
        u.SetProtocolOptions("");
        
        /* cut off the all in the end */
        u.SetFileName(u.GetFileName().substr(0, u.GetFileName().size()-3));
        
        PlexUtils::AppendPathToURL(u, "firstCharacter");
        XFILE::CPlexDirectory dir;
        CFileItemList characters;

        if (dir.GetDirectory(u, characters))
        {
          int total = 0;
          for (int i = 0; i < characters.Size(); i++)
          {
            CFileItemPtr charDir = characters.Get(i);
            int num = charDir->GetProperty("size").asInteger();
            for (int j = 0; j < num; j ++)
              charMap[total ++] = charDir->GetProperty("title").asString();
          }
        }
      }
      
      for (int i=0; i < (items.GetProperty("totalSize").asInteger()) - DEFAULT_PAGE_SIZE; i++)
      {
        CFileItemPtr item = CFileItemPtr(new CFileItem);
        item->SetPath(boost::lexical_cast<std::string>(i));
        if (charMap.find(DEFAULT_PAGE_SIZE + i) != charMap.end())
          item->SetSortLabel(CStdString(charMap[DEFAULT_PAGE_SIZE + i]));
        items.Add(item);
      }
    }
  }
  return ret;
}

void CGUIPlexMediaWindow::LoadPage(int start, int numberOfItems)
{
  if (start >= m_vecItems->GetProperty("totalSize").asInteger())
    return;
  if (m_currentJobId != -1)
  {
    CJobManager::GetInstance().CancelJob(m_currentJobId);
    m_currentJobId = -1;
  }
  
  CURL u(m_vecItems->GetPath());
  
  int pageSize = XMIN(numberOfItems, m_vecItems->GetProperty("totalSize").asInteger() - start);
  
  u.SetProtocolOption("containerStart", boost::lexical_cast<std::string>(start));
  u.SetProtocolOption("containerSize", boost::lexical_cast<std::string>(pageSize));
  
  CLog::Log(LOGDEBUG, "CGUIPlexMediaWindow::LoadPage loading %d to %d", start, start+pageSize);
  
  m_currentJobId = CJobManager::GetInstance().AddJob(new CPlexDirectoryFetchJob(u), this);
}

void CGUIPlexMediaWindow::LoadNextPage()
{
  if (m_vecItems->HasProperty("totalSize"))
  {
    if (m_vecItems->GetProperty("totalSize").asInteger() > m_pagingOffset)
    {
      LoadPage(m_pagingOffset, DEFAULT_PAGE_SIZE);
    }
  }
}

void CGUIPlexMediaWindow::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (success)
  {
    CPlexDirectoryFetchJob *fjob = static_cast<CPlexDirectoryFetchJob*>(job);
    
    int nItem = m_viewControl.GetSelectedItem();
    CStdString strSelected;
    if (nItem >= 0)
      strSelected = m_vecItems->Get(nItem)->GetPath();
    
    int itemsToRemove = fjob->m_items.Size();
    for (int i = 0; i < itemsToRemove; i ++)
      m_vecItems->Remove(m_pagingOffset);
    
    for (int i = 0; i < fjob->m_items.Size(); i ++)
      m_vecItems->Insert(m_pagingOffset + i, fjob->m_items.Get(i));
    
    m_pagingOffset += fjob->m_items.Size();
    m_viewControl.SetItems(*m_vecItems);
    m_viewControl.SetSelectedItem(strSelected);
  }
  
  m_currentJobId = -1;
}

void CGUIPlexMediaWindow::PlayMovie(const CFileItem *item)
{
  CFileItemPtr file = CFileItemPtr(new CFileItem(*item));
  PlexContentPlayerMixin::PlayPlexItem(file);
}

void CGUIPlexMediaWindow::BuildFilter(const CURL& strDirectory)
{
  if (strDirectory.Get().empty())
    return;

  m_filterHelper.BuildFilters(strDirectory, m_vecItems->GetPlexDirectoryType());
}

bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath)
{
  return Update(strDirectory, updateFilterPath, true);
}

bool CGUIPlexMediaWindow::Update(const CStdString &strDirectory, bool updateFilterPath, bool updateFilters)
{
  bool isSecondary = false;
  CURL newUrl = m_filterHelper.GetRealDirectoryUrl(strDirectory, isSecondary);

  bool ret = CGUIWindowVideoNav::Update(newUrl.Get(), updateFilterPath);

  if (isSecondary)
  {
    /* Kill the history */
    m_history.ClearPathHistory();
    m_history.AddPath(newUrl.Get());
    m_startDirectory = newUrl.Get();

    if (updateFilters)
      BuildFilter(m_filterHelper.GetSectionUrl());
  }
  else
  {
    newUrl.SetProtocolOptions("");
    m_history.AddPath(newUrl.GetUrlWithoutOptions());
  }

  return ret;
}

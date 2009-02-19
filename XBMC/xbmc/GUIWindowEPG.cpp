/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "GUIWindowEPG.h"
#include "GUIWindowEPGProgInfo.h"
#include "PVRManager.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"

#define CONTROL_LABELEMPTY        10
#define CONTROL_EPGGRID           20
#define LABEL_CHANNELNAME         30  /* 70 available */
#define LABEL_RULER               110

/* Need to allocate enough IDs for extreme numbers of channels & shows */

CGUIWindowEPG::CGUIWindowEPG(void) 
    : CGUIWindow(WINDOW_EPG, "MyTVGuide.xml")
{
  m_curDaysOffset = 0; // offset of zero to grab current schedule
  m_bDisplayEmptyDatabaseMessage = false;
  m_channels = new VECFILEITEMS; ///TOOO move somewhere else
}

CGUIWindowEPG::~CGUIWindowEPG(void)
{
  delete m_channels;
}

void CGUIWindowEPG::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
}

void CGUIWindowEPG::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
}

bool CGUIWindowEPG::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowEPG::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIWindow::OnMessage(message);
      m_database.Close();
      return true;
    }
    break;
  case GUI_MSG_LABEL_RESET:
    {
      /*m_bDisplayEmptyDatabaseMessage = !m_bDisplayEmptyDatabaseMessage;*/ // flip the visibility
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      //start threaded grid update
      return CGUIWindow::OnMessage(message);
    }
    break;
  case GUI_MSG_CLICKED:
    {
      unsigned iControl = message.GetSenderId();

      if (m_guideGrid && iControl == m_guideGrid->GetID())  // msg came from grid control
      {
        // get selected item
        CFileItemPtr item = m_guideGrid->GetSelectedItemPtr();
        if (!item) return false;

        int iAction = message.GetParam1();

        if (iAction == ACTION_RECORD)
        {
          // bring up recording dialog
          OnInfo(item.get());
          return true;
        }
        else if (iAction == ACTION_PLAY)
        {
          ///TODO either switch channel, or set autoview
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO || iAction == ACTION_SELECT_ITEM)
        {
          OnInfo(item.get());
          return true;
        }
      }
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowEPG::OnInitWindow()
{
  UpdateGrid();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowEPG::GetGridData()
{
  ///////
  m_daysToDisplay = 2; /// from settings
  ///////

  CDateTimeSpan offset;
  CDateTime now;

  now = CDateTime::GetCurrentDateTime();
  offset.SetDateTimeSpan(0, 0, 0, (now.GetMinute() % 30) * 60 + now.GetSecond()); // set start to previous half-hour
  m_gridStart = now - offset;

  offset.SetDateTimeSpan(m_curDaysOffset, 0, 0, 0);
  m_gridStart += offset;

  offset.SetDateTimeSpan(m_daysToDisplay, 0, 0, 0);
  m_gridEnd = m_gridStart + offset;
  
  // check that m_gridEnd date exists in schedules
  // otherwise reduce the range
  DWORD tick(timeGetTime());
  m_dataEnd = m_database.GetDataEnd(0);

  CLog::Log(LOGDEBUG, "TV: m_database.GetDataEnd() took %u ms to return", timeGetTime()-tick); /// slow to return

  if (m_dataEnd < m_gridEnd)
  {
    m_gridEnd = m_dataEnd;
  }

  // grab the channel list
  tick = timeGetTime();

  if (m_numChannels > 0)
  {
    // start with an empty data store
    m_gridData = new EPGData;

    DWORD tick(timeGetTime());
    m_database.Open();
    m_database.BeginTransaction();

    int items = 0;
    for (int i = 0; i < m_numChannels; i++)
    {
      /*VECFILEITEMS programmes;
      CFileItemPtr prog = m_channels->at(i);
      if(!m_database.GetProgrammesByChannelName(prog->GetLabel(), programmes, m_gridStart, m_gridEnd))
        continue;
      
      items += (int)programmes.size();
      m_gridData->push_back(programmes);
    */}
    m_database.CommitTransaction();
    m_database.Close();

    if (items > 0)
    {
      CLog::Log(LOGDEBUG, "%s returned %u items in %u ms", __FUNCTION__, timeGetTime()-tick, items);
      m_bDisplayEmptyDatabaseMessage = false;
    }
    else
    {
      CLog::Log(LOGERROR, "%s failed, (%s)", __FUNCTION__, "No EPG Data found!");
      m_bDisplayEmptyDatabaseMessage = true;
    }
  }
  else
  {
    CLog::Log(LOGERROR, "%s failed, (%s)", __FUNCTION__, "No Channels found!");
    m_bDisplayEmptyDatabaseMessage = true;
  }
}

void CGUIWindowEPG::UpdateGrid()
{
  if (!m_guideGrid || !m_gridData) return; /// ?? what if GridContainer hasnt binded it's items

  // need to check grid exists
  //CGUIListItemPtr currentItem = m_guideGrid->GetSelectedItemPtr();

  CEPG* epg = CPVRManager::GetInstance()->GetEPG();
  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_EPGGRID, 0, 0, epg);
  if (g_graphicsContext.SendMessage(msg))
  {
    // get a pointer to the grid container
    m_guideGrid = (CGUIEPGGridContainer*)GetControl(CONTROL_EPGGRID);
  }
  else m_guideGrid = NULL;
}

void CGUIWindowEPG::OnInfo(CFileItem* pItem)
{
  if ( !pItem ) return ;
  /// ShowInfo can kill the item as this window can be closed while we do it, can it?
  /// so take a copy of the item now
  CFileItem item(*pItem);
  if (!item.IsTVDb()) //TODO
    return;
  
  ShowEPGInfo(&item);
}

void CGUIWindowEPG::ShowEPGInfo(CFileItem *item)
{
  CGUIWindowEPGProgInfo* pDlgInfo = (CGUIWindowEPGProgInfo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_EPG_INFO);
  if (!pDlgInfo)
    return;

  pDlgInfo->SetProgramme(item);
  pDlgInfo->DoModal();

}

void CGUIWindowEPG::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowEPG::Render()
{
  if (m_bDisplayEmptyDatabaseMessage)
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(775));
    SET_CONTROL_HIDDEN(CONTROL_EPGGRID);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"");
    SET_CONTROL_VISIBLE(CONTROL_EPGGRID);
  }

  CGUIWindow::Render();
}

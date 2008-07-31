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
#include "PVRManager.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"

#define CONTROL_LABELEMPTY        10
#define CONTROL_EPGGRID           20
#define LABEL_CHANNELNAME         30  /* 70 available */
#define LABEL_RULER			          110

/* Need to allocate enough IDs for extreme numbers of channels & shows */

CGUIWindowEPG::CGUIWindowEPG(void) 
    : CGUIWindow(WINDOW_EPG, "MyTVGuide.xml")
{
  m_curDaysOffset = 0; // offset of zero to grab current schedule
}

CGUIWindowEPG::~CGUIWindowEPG(void)
{
  m_gridData.clear();      
}

void CGUIWindowEPG::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  GetGridData();
  UpdateGridItems();
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
      m_database.Close();
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      m_database.Open();
      return CGUIWindow::OnMessage(message);
    }
    break;
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowEPG::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
}

void CGUIWindowEPG::GetGridData()
{
  ///////
  m_daysToDisplay = 1; /// from settings
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
  m_dataEnd = m_database.GetDataEnd();
  if (m_dataEnd < m_gridEnd)
  {
    m_gridEnd = m_dataEnd;
  }

  // grab the list of channels
  VECFILEITEMS channels;
  m_database.Open();
  m_database.GetChannels(false, channels);
  m_database.Close();
  m_numChannels = (int)channels.size();

  if (m_numChannels > 0)
  {
    // start with an empty data store
    iEPGRow it = m_gridData.begin();
    for ( ; it != m_gridData.end(); it++)
    {
      if (it->size())
        it->empty();
    }
    m_gridData.clear();

    DWORD tick(timeGetTime());
    m_database.Open();
    m_database.BeginTransaction();

    int items = 0;
    for (int i = 0; i < m_numChannels; i++)
    {
      VECFILEITEMS programmes;
      if(!m_database.GetProgrammesByChannel(channels[i]->GetLabel(), programmes, m_gridStart, m_gridEnd))
        continue;
      
      items += (int)programmes.size();
      m_gridData.push_back(programmes);
    }
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

void CGUIWindowEPG::UpdateGridItems()
{
  m_guideGrid = (CGUIEPGGridContainer*)GetControl(CONTROL_EPGGRID);
  m_guideGrid->UpdateItems(m_gridData, m_gridStart, m_gridEnd);
}

void CGUIWindowEPG::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowEPG::Render()
{
  if (m_bDisplayEmptyDatabaseMessage)
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(775))
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"")
  }

  CGUIWindow::Render();
}

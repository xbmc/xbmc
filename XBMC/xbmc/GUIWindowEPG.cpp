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
#define CONTROL_GRIDITEMS         100 /* ?? available */
/* Need to allocate enough IDs for extreme numbers of channels & shows */

CGUIWindowEPG::CGUIWindowEPG(void) 
    : CGUIWindow(WINDOW_EPG, "MyTVGuide.xml")
{
  m_curDaysOffset = 0; // offset of zero to grab current schedule
}

CGUIWindowEPG::~CGUIWindowEPG(void)
{
  
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
  UpdateGridData();
  UpdateGridItems();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowEPG::UpdateGridData()
{
  GetEPG(0);
}

void CGUIWindowEPG::UpdateGridItems()
{
  m_gridItems = (CGUIEPGGridContainer*)GetControl(CONTROL_EPGGRID);
  m_gridItems->UpdateItems(m_gridData);
}
void CGUIWindowEPG::GetEPG(int offset)
{
  VECTVCHANNELS channels;

  m_database.Open();
  m_database.GetAllChannels(false, channels);
  m_database.Close();

  m_curDaysOffset = 0;
  m_daysToDisplay = 1;

  m_numChannels = (int)channels.size();
  if (m_numChannels > 0)
  {
    // start with an empty datastore
    itEPGRow it = m_gridData.begin();
    for ( ; it != m_gridData.end(); it++)
    {
      if (it->shows.size())
        it->shows.empty();
    }
    DWORD tick(timeGetTime());
    m_database.Open();
    m_database.BeginTransaction();

    int items = 0;
    for (int i = 0; i < m_numChannels; i++)
    {
      EPGRow curRow;
      curRow.channelName = channels[i]->GetLabel();
      curRow.channelNum  = channels[i]->GetPropertyInt("ChannelNum");

      if(!m_database.GetShowsByChannel(channels[i]->GetLabel(), curRow.shows, m_curDaysOffset, m_daysToDisplay))
        return; /* debug log: couldn't grab data */
      items += (int)curRow.shows.size();
      m_gridData.push_back(curRow);
    }
    m_database.CommitTransaction();
    m_database.Close();
    CLog::Log(LOGDEBUG, "%s completed successfully in %u ms, returning %u items", __FUNCTION__, timeGetTime()-tick, items);
    m_bDisplayEmptyDatabaseMessage = false;
  }
  else
  {
    CLog::Log(LOGERROR, "%s (%s) failed", __FUNCTION__, "No Channels found!");
    m_bDisplayEmptyDatabaseMessage = true;
  }
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
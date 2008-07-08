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
#include "GUIImage.h"
#include "PVRManager.h"
#include "GUISettings.h"
#include "GUIWindowManager.h"

#define CONTROL_EPGGRID           2
#define CONTROL_LABELEMPTY        10

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
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowEPG::OnInitWindow()
{
  UpdateGridData();
  //UpdateGridItems();
  CGUIWindow::OnInitWindow();
}

void CGUIWindowEPG::UpdateGridData()
{
  GetEPG(0);
}

void CGUIWindowEPG::GetEPG(int offset)
{
  VECTVCHANNELS channels;

  m_tvDB.Open();
  m_tvDB.GetAllChannels(false, channels);
  m_tvDB.Close();

  m_curDaysOffset = 0;
  m_daysToDisplay = 2;

  m_numChannels = (int)channels.size();
  if (m_numChannels > 0)
  {
    m_bDisplayEmptyDatabaseMessage = false;
    DWORD tick(timeGetTime());
    m_tvDB.Open();
    m_tvDB.BeginTransaction();

    int items = 0;
    for (int i = 0; i < m_numChannels; i++)
    {
      EPGRow curRow;
      curRow.channelName = channels[i]->GetLabel();
      curRow.channelNum  = channels[i]->GetPropertyInt("ChannelNum");

      if(!m_tvDB.GetShowsByChannel(channels[i]->GetLabel(), curRow.shows, m_curDaysOffset, m_daysToDisplay))
        return; /* couldn't grab data */
      items += (int)curRow.shows.size();
      m_gridData.push_back(curRow);
    }
    m_tvDB.CommitTransaction();
    m_tvDB.Close();
    CLog::Log(LOGINFO, "%s completed successfully in %u ms, returning %u items", __FUNCTION__, timeGetTime()-tick, items);
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
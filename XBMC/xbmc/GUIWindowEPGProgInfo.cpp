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
#include "GUIWindow.h"
#include "GUIWindowEPGProgInfo.h"
#include "Util.h"
#include "PVRManager.h"
#include "Picture.h"
#include "GUIImage.h"
#include "GUIWindowEPG.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"

#define CONTROL_PROG_TITLE              20 // from EPG
#define CONTROL_PROG_SUBTITLE           21
#define CONTROL_PROG_STARTTIME          23
#define CONTROL_PROG_DATE               24
#define CONTROL_PROG_DURATION           25
#define CONTROL_PROG_GENRE              26
#define CONTROL_PROG_CHANNEL            27
#define CONTROL_PROG_CHANNELCALLSIGN    28
#define CONTROL_PROG_CHANNELNUM         29

#define CONTROL_PROG_RECSTATUS          30 // from pvrmanager
#define CONTROL_PROG_JOBSTATUS          31

#define CONTROL_TEXTAREA                22
#define CONTROL_LIKELIST                50

#define CONTROL_BTN_LIKELIST            5
#define CONTROL_BTN_RECORD              6

CGUIWindowEPGProgInfo::CGUIWindowEPGProgInfo(void)
    : CGUIDialog(WINDOW_DIALOG_EPG_INFO, "DialogEPGProgInfo.xml")
    , m_progItem(new CFileItem)
{
  m_likeList = new CFileItemList;
}

CGUIWindowEPGProgInfo::~CGUIWindowEPGProgInfo(void)
{
  delete m_likeList;
}

bool CGUIWindowEPGProgInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    {
      ClearLists();
      m_database.Close();
    }
    break;
  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();
      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      
      CGUIDialog::OnMessage(message);
      m_viewDescription = true;
      Update();

      return true;
    }
    break;
  case GUI_MSG_CLICKED:
    int iControl = message.GetSenderId();
    {
      if (iControl == CONTROL_BTN_LIKELIST)
      {
        m_viewDescription = !m_viewDescription;
        Update();
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowEPGProgInfo::SetProgramme(const CFileItem *item)
{
  *m_progItem = *item;
 
  // get list of like items
  ClearLists();
  m_database.Open();
  m_database.GetProgrammesByEpisodeID(m_progItem->GetEPGInfoTag()->m_episodeID, m_likeList, true);    // episodeID matches rank higher == appear first in the list
  /*m_database.GetProgrammesBySubtitle(m_progItem->GetEPGInfoTag()->m_strPlotOutline, m_likeList, true);*/

  if (!m_likeList->IsEmpty())
  {
    /// remove the search item from results
    for (int i = 0; i < m_likeList->GetFileCount(); i++)
    {
      if (m_likeList->Get(i)->GetEPGInfoTag()->GetDbID() == m_progItem->GetEPGInfoTag()->GetDbID())
      {
        m_likeList->Remove(i);
        break; // only one match can ever be found
      }
    }
  }
}

void CGUIWindowEPGProgInfo::Update()
{
  CStdString strTemp;
  strTemp = m_progItem->GetEPGInfoTag()->m_strTitle; strTemp.Trim();
  SetLabel(CONTROL_PROG_TITLE, strTemp);

  strTemp = m_progItem->GetEPGInfoTag()->m_startTime.GetAsLocalizedDate(true); strTemp.Trim();
  SetLabel(CONTROL_PROG_DATE, strTemp);

  strTemp = m_progItem->GetEPGInfoTag()->m_startTime.GetAsLocalizedTime("", false); strTemp.Trim();
  SetLabel(CONTROL_PROG_STARTTIME, strTemp);

  strTemp = m_progItem->GetEPGInfoTag()->m_duration.GetMinutes(); strTemp.Trim();
  SetLabel(CONTROL_PROG_DURATION, strTemp);

  strTemp = m_progItem->GetEPGInfoTag()->m_strGenre; strTemp.Trim();
  SetLabel(CONTROL_PROG_GENRE, strTemp);

  strTemp = m_progItem->GetEPGInfoTag()->m_strChannel; strTemp.Trim();
  SetLabel(CONTROL_PROG_CHANNEL, strTemp);

  strTemp.Format("%u", m_progItem->GetEPGInfoTag()->m_channelNum);
  SetLabel(CONTROL_PROG_CHANNELNUM, strTemp);

  // programme subtitle
  strTemp = m_progItem->GetEPGInfoTag()->m_strPlotOutline; strTemp.Trim();
  if (strTemp.IsEmpty())
  {
    SET_CONTROL_HIDDEN(CONTROL_PROG_SUBTITLE);
  }
  else
  {
    SetLabel(CONTROL_PROG_SUBTITLE, strTemp);
    SET_CONTROL_VISIBLE(CONTROL_PROG_SUBTITLE);
  }

  // programme description
  strTemp = m_progItem->GetEPGInfoTag()->m_strPlot; strTemp.Trim();
  SetLabel(CONTROL_TEXTAREA, strTemp);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIKELIST, 0, 0, m_likeList);
  OnMessage(msg);

  if (m_viewDescription)
  {
    SET_CONTROL_LABEL(CONTROL_BTN_LIKELIST, 204);

    SET_CONTROL_HIDDEN(CONTROL_LIKELIST);
    SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_BTN_LIKELIST, 207);

    SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
    SET_CONTROL_VISIBLE(CONTROL_LIKELIST);
  }

  if (m_likeList->IsEmpty())
  {
    SET_CONTROL_HIDDEN(CONTROL_BTN_LIKELIST);
  }

  SET_CONTROL_VISIBLE(CONTROL_BTN_RECORD);
}

void CGUIWindowEPGProgInfo::ClearLists()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIKELIST);
  OnMessage(msg);
  m_likeList->Clear();
}

void CGUIWindowEPGProgInfo::SetLabel(int iControl, const CStdString &strLabel)
{
  if (strLabel.IsEmpty())
  {
    SET_CONTROL_LABEL(iControl, 416); /// disable instead? // "Not available"
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

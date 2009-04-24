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


/* Standart includes */
#include "stdafx.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUISettings.h"
#include "GUIDialogOK.h"
#include "Util.h"

/* self include */
#include "GUIDialogTVEPGProgInfo.h"

/* TV control */
#include "PVRManager.h"

/* TV information tags */
#include "utils/TVEPGInfoTag.h"
#include "utils/TVChannelInfoTag.h"
#include "utils/TVRecordInfoTag.h"
#include "utils/TVTimerInfoTag.h"

/* Dialog windows includes */
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogNumeric.h"
#include "GUIDialogKeyboard.h"

using namespace std;

#define CONTROL_PROG_TITLE              20 // from db
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

#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7


CGUIDialogTVEPGProgInfo::CGUIDialogTVEPGProgInfo(void)
    : CGUIDialog(WINDOW_DIALOG_TV_GUIDE_INFO, "DialogEPGProgInfo.xml")
    , m_progItem(new CFileItem)
{
}

CGUIDialogTVEPGProgInfo::~CGUIDialogTVEPGProgInfo(void)
{
}

bool CGUIDialogTVEPGProgInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_DEINIT:
  {
  }
  break;
  case GUI_MSG_WINDOW_INIT:
  {
    CGUIDialog::OnMessage(message);
    m_viewDescription = true;
    Update();
    return true;
  }

  break;
  case GUI_MSG_CLICKED:
    int iControl = message.GetSenderId();
    {
      if (iControl == CONTROL_BTN_OK)
      {
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_RECORD)
      {
        int iChannel = m_progItem->GetTVEPGInfoTag()->m_channelNum;

        if (iChannel != -1)
        {
          if (m_progItem->GetTVEPGInfoTag()->m_isRecording == false)
          {
            // prompt user for confirmation of channel record
            CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);

            if (pDialog)
            {
              pDialog->SetHeading(264);
              pDialog->SetLine(0, "");
              pDialog->SetLine(1, m_progItem->GetTVEPGInfoTag()->m_strTitle);
              pDialog->SetLine(2, "");
              pDialog->DoModal();

              if (pDialog->IsConfirmed())
              {
                CTVTimerInfoTag newtimer(*m_progItem.get());
                CFileItem *item = new CFileItem(newtimer);

                if (CPVRManager::GetInstance()->AddTimer(*item))
                {
                  m_progItem->GetTVEPGInfoTag()->m_isRecording = true;
                }
              }
            }
          }
          else
          {
            CGUIDialogOK::ShowAndGetInput(18100,18107,0,0);
          }
        }

        Close();

        return true;
      }
      else if (iControl == CONTROL_BTN_SWITCH)
      {
        Close();
        {
          CFileItemList channelslist;
          int ret_channels;

      /*    if (!m_progItem->GetTVEPGInfoTag()->m_isRadio)*/
     /*       ret_channels = CPVRManager::GetInstance()->GetTVChannels(&channelslist, -1, false, !g_guiSettings.GetBool("pvrmenu.ftaonly"));*/
        /*  else*/
     /*       ret_channels = CPVRManager::GetInstance()->GetRadioChannels(&channelslist, -1, false, !g_guiSettings.GetBool("pvrmenu.ftaonly"));*/

          if (ret_channels > 0)
          {
            if (!g_application.PlayFile(*channelslist[m_progItem->GetTVEPGInfoTag()->m_channelNum-1]))
            {
              CGUIDialogOK::ShowAndGetInput(18100,0,18134,0);
              return false;
            }
            return true;
          }
        }
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogTVEPGProgInfo::SetProgInfo(const CFileItem *item)
{
  *m_progItem = *item;
}

void CGUIDialogTVEPGProgInfo::Update()
{
  CStdString strTemp;
  int minutes;

  strTemp = m_progItem->GetTVEPGInfoTag()->m_strTitle; strTemp.Trim();
  SetLabel(CONTROL_PROG_TITLE, strTemp);

  strTemp = m_progItem->GetTVEPGInfoTag()->m_startTime.GetAsLocalizedDate(true); strTemp.Trim();
  SetLabel(CONTROL_PROG_DATE, strTemp);

  strTemp = m_progItem->GetTVEPGInfoTag()->m_startTime.GetAsLocalizedTime("", false); strTemp.Trim();
  SetLabel(CONTROL_PROG_STARTTIME, strTemp);

  minutes = m_progItem->GetTVEPGInfoTag()->m_duration.GetMinutes();
  minutes += m_progItem->GetTVEPGInfoTag()->m_duration.GetHours()*60;
  strTemp.Format("%i", minutes);
  strTemp.Trim();
  SetLabel(CONTROL_PROG_DURATION, strTemp);

  strTemp = m_progItem->GetTVEPGInfoTag()->m_strGenre; strTemp.Trim();
  SetLabel(CONTROL_PROG_GENRE, strTemp);

  strTemp = m_progItem->GetTVEPGInfoTag()->m_strChannel;
  strTemp.Trim();
  SetLabel(CONTROL_PROG_CHANNEL, strTemp);

  int i = m_progItem->GetTVEPGInfoTag()->m_channelNum;
  strTemp.Format("%u", m_progItem->GetTVEPGInfoTag()->m_channelNum); // int value
  SetLabel(CONTROL_PROG_CHANNELNUM, strTemp);

  // programme subtitle
  strTemp = m_progItem->GetTVEPGInfoTag()->m_strPlotOutline; strTemp.Trim();

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
  strTemp = m_progItem->GetTVEPGInfoTag()->m_strPlot; strTemp.Trim();

  SetLabel(CONTROL_TEXTAREA, strTemp);

  SET_CONTROL_VISIBLE(CONTROL_BTN_RECORD);
}

void CGUIDialogTVEPGProgInfo::SetLabel(int iControl, const CStdString &strLabel)
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

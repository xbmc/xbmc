/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "Application.h"
#include "GUIWindowManager.h"
#include "GUIDialogPVRGuideInfo.h"
#include "utils/PVREpg.h"
#include "utils/PVRChannels.h"
#include "utils/PVRTimers.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"

using namespace std;

#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7

CGUIDialogPVRGuideInfo::CGUIDialogPVRGuideInfo(void)
    : CGUIDialog(WINDOW_DIALOG_TV_GUIDE_INFO, "DialogPVRGuideInfo.xml")
    , m_progItem(new CFileItem)
{
}

CGUIDialogPVRGuideInfo::~CGUIDialogPVRGuideInfo(void)
{
}

bool CGUIDialogPVRGuideInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      Update();
      break;
    }
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_OK)
      {
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_RECORD)
      {
        if (m_progItem->GetEPGInfoTag()->ChannelNumber() > 0)
        {
          if (m_progItem->GetEPGInfoTag()->Timer() == NULL)
          {
            // prompt user for confirmation of channel record
            CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

            if (pDialog)
            {
              pDialog->SetHeading(264);
              pDialog->SetLine(0, "");
              pDialog->SetLine(1, m_progItem->GetEPGInfoTag()->Title());
              pDialog->SetLine(2, "");
              pDialog->DoModal();

              if (pDialog->IsConfirmed())
              {
                cPVRTimerInfoTag newtimer(*m_progItem.get());
                CFileItem *item = new CFileItem(newtimer);
                cPVRTimers::AddTimer(*item);
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

          if (!m_progItem->GetEPGInfoTag()->IsRadio())
            ret_channels = PVRChannelsTV.GetChannels(&channelslist, -1);
          else
            ret_channels = PVRChannelsRadio.GetChannels(&channelslist, -1);

          if (ret_channels > 0)
          {
            if (!g_application.PlayFile(*channelslist[m_progItem->GetEPGInfoTag()->ChannelNumber()-1]))
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

void CGUIDialogPVRGuideInfo::SetProgInfo(const CFileItem *item)
{
  *m_progItem = *item;
}

CFileItemPtr CGUIDialogPVRGuideInfo::GetCurrentListItem(int offset)
{
  return m_progItem;
}

void CGUIDialogPVRGuideInfo::Update()
{
  // set recording button label
  cPVREPGInfoTag* tag = m_progItem->GetEPGInfoTag();
  if (tag->End() > CDateTime::GetCurrentDateTime())
  {
    if (tag->Timer() == NULL)
    {
      if (tag->Start() < CDateTime::GetCurrentDateTime())
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 264);
      else
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 18416);
    }
    else
    {
      if (tag->Start() < CDateTime::GetCurrentDateTime())
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 18414);
      else
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 18415);
    }
  }
  else
    SET_CONTROL_HIDDEN(CONTROL_BTN_RECORD);
}

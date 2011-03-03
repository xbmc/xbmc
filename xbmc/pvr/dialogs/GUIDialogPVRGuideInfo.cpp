/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "GUIDialogPVRGuideInfo.h"
#include "Application.h"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogYesNo.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "pvr/epg/PVREpgInfoTag.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/timers/PVRTimerInfoTag.h"

using namespace std;

#define CONTROL_BTN_SWITCH              5
#define CONTROL_BTN_RECORD              6
#define CONTROL_BTN_OK                  7

CGUIDialogPVRGuideInfo::CGUIDialogPVRGuideInfo(void)
    : CGUIDialog(WINDOW_DIALOG_PVR_GUIDE_INFO, "DialogPVRGuideInfo.xml")
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
        if (((CPVREpgInfoTag *)m_progItem->GetEPGInfoTag())->ChannelTag()->ChannelNumber() > 0)
        {
          if (((CPVREpgInfoTag *) m_progItem->GetEPGInfoTag())->Timer() == NULL)
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
                CPVREpgInfoTag *tag = (CPVREpgInfoTag *) m_progItem->GetEPGInfoTag();
                CPVRTimerInfoTag *newtimer = CPVRTimerInfoTag::CreateFromEpg(*tag);
                CFileItem *item = new CFileItem(*newtimer);
                CPVRTimers::AddTimer(*item);
              }
            }
          }
          else
          {
            CGUIDialogOK::ShowAndGetInput(19033,19067,0,0);
          }
        }
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_SWITCH)
      {
        Close();

        if (!g_application.PlayFile(CFileItem(*((CPVREpgInfoTag *)m_progItem->GetEPGInfoTag())->ChannelTag())))
        {
          CGUIDialogOK::ShowAndGetInput(19033,0,19035,0);
          return false;
        }
        return true;
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
  CPVREpgInfoTag* tag = (CPVREpgInfoTag *) m_progItem->GetEPGInfoTag();
  if (tag->End() > CDateTime::GetCurrentDateTime())
  {
    if (tag->Timer() == NULL)
    {
      if (tag->Start() < CDateTime::GetCurrentDateTime())
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 264);
      else
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19061);
    }
    else
    {
      if (tag->Start() < CDateTime::GetCurrentDateTime())
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19059);
      else
        SET_CONTROL_LABEL(CONTROL_BTN_RECORD, 19060);
    }
  }
  else
    SET_CONTROL_HIDDEN(CONTROL_BTN_RECORD);
}

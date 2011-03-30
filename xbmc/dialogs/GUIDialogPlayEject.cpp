/*
 *      Copyright (C) 2005-2011 Team XBMC
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

#include "Autorun.h"
#include "GUIDialogPlayEject.h"
#include "guilib/GUIWindowManager.h"
#include "storage/MediaManager.h"
#include "storage/IoSupport.h"

#define ID_BUTTON_PLAY      11
#define ID_BUTTON_EJECT     10

CGUIDialogPlayEject::CGUIDialogPlayEject()
    : CGUIDialogYesNo(WINDOW_DIALOG_PLAY_EJECT)
{
}

CGUIDialogPlayEject::~CGUIDialogPlayEject()
{
}

bool CGUIDialogPlayEject::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();
    if (iControl == ID_BUTTON_PLAY)
    {
      if (g_mediaManager.IsDiscInDrive())
      {
        m_bConfirmed = true;
        Close();
      }

      return true;
    }
    if (iControl == ID_BUTTON_EJECT)
    {
      CIoSupport::ToggleTray();
      return true;
    }
  }

  return CGUIDialogYesNo::OnMessage(message);
}

void CGUIDialogPlayEject::FrameMove()
{
  CONTROL_ENABLE_ON_CONDITION(ID_BUTTON_PLAY, g_mediaManager.IsDiscInDrive());

  CGUIDialogYesNo::FrameMove();
}

void CGUIDialogPlayEject::OnInitWindow()
{
  if (g_mediaManager.IsDiscInDrive())
  {
    m_defaultControl = ID_BUTTON_PLAY;
  }
  else
  {
    CONTROL_DISABLE(ID_BUTTON_PLAY);
    m_defaultControl = ID_BUTTON_EJECT;
  }

  CGUIDialogYesNo::OnInitWindow();
}

bool CGUIDialogPlayEject::ShowAndGetInput(CVariant vHeading, CVariant vLine0,
  CVariant vLine1, CVariant vLine2, unsigned int uiAutoCloseTime /* = 0 */)
{
  CGUIDialogPlayEject * pDialog = (CGUIDialogPlayEject *)g_windowManager.
    GetWindow(WINDOW_DIALOG_PLAY_EJECT);

  if (!pDialog)
    return false;

  pDialog->SetHeading(vHeading);
  pDialog->SetLine(0, vLine0);
  pDialog->SetLine(1, vLine1);
  pDialog->SetLine(2, vLine2);

  pDialog->SetChoice(ID_BUTTON_PLAY - 10, 208);
  pDialog->SetChoice(ID_BUTTON_EJECT - 10, 13391);

  if (uiAutoCloseTime)
    pDialog->SetAutoClose(uiAutoCloseTime);

  pDialog->DoModal();

  return pDialog->IsConfirmed();
}

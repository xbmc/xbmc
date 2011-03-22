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
#include "GUIDialogPlayEjectCancel.h"
#include "GUIWindowManager.h"
#include "storage/MediaManager.h"
#include "storage/IoSupport.h"

#define ID_BUTTON_PLAY      10
#define ID_BUTTON_EJECT     11
#define ID_BUTTON_CANCEL    12

CGUIDialogPlayEjectCancel::CGUIDialogPlayEjectCancel(void)
    : CGUIDialogBoxBase(WINDOW_DIALOG_PLAY_EJECT_CANCEL, "DialogPlayEjectCancel.xml")
{
}

CGUIDialogPlayEjectCancel::~CGUIDialogPlayEjectCancel()
{
}

bool CGUIDialogPlayEjectCancel::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();
    if (iControl == ID_BUTTON_PLAY)
    {
      if (g_mediaManager.IsDiscInDrive())
      {
        MEDIA_DETECT::CAutorun::PlayDisc();
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
    if (iControl == ID_BUTTON_CANCEL)
    {
      m_bConfirmed = false;
      Close();
      return true;
    }
  }

  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogPlayEjectCancel::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_PREVIOUS_MENU)
  {
    m_bConfirmed = false;
    Close();
    return true;
  }

  return CGUIDialogBoxBase::OnAction(action);
}

void CGUIDialogPlayEjectCancel::FrameMove()
{
  CONTROL_ENABLE_ON_CONDITION(ID_BUTTON_PLAY, g_mediaManager.IsDiscInDrive());
}

void CGUIDialogPlayEjectCancel::OnInitWindow()
{
  if (!g_mediaManager.IsDiscInDrive())
  {
    CONTROL_DISABLE(ID_BUTTON_PLAY);
    m_defaultControl = ID_BUTTON_EJECT;
  }

  CGUIDialog::OnInitWindow();
}

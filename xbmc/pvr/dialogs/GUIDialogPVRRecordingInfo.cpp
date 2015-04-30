/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIDialogPVRRecordingInfo.h"
#include "FileItem.h"

using namespace PVR;

#define CONTROL_BTN_OK  10

CGUIDialogPVRRecordingInfo::CGUIDialogPVRRecordingInfo(void)
  : CGUIDialog(WINDOW_DIALOG_PVR_RECORDING_INFO, "DialogPVRRecordingInfo.xml")
  , m_recordItem(new CFileItem)
{
}

bool CGUIDialogPVRRecordingInfo::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    int iControl = message.GetSenderId();

    if (iControl == CONTROL_BTN_OK)
    {
      Close();
      return true;
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRRecordingInfo::SetRecording(const CFileItem *item)
{
  *m_recordItem = *item;
}

CFileItemPtr CGUIDialogPVRRecordingInfo::GetCurrentListItem(int offset)
{
  return m_recordItem;
}

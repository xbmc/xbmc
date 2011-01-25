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

#include "GUIDialogPVRCutterOSD.h"
#include "utils/log.h"
#include "Application.h"

using namespace std;

CGUIDialogPVRCutterOSD::CGUIDialogPVRCutterOSD()
    : CGUIDialog(WINDOW_DIALOG_PVR_OSD_CUTTER, "DialogPVRCutterOSD.xml")
{
}

CGUIDialogPVRCutterOSD::~CGUIDialogPVRCutterOSD()
{
}

bool CGUIDialogPVRCutterOSD::OnAction(const CAction& action)
{
  if (action.GetID() == ACTION_PREVIOUS_MENU || action.GetID() == ACTION_CLOSE_DIALOG)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogPVRCutterOSD::OnMessage(CGUIMessage& message)
{
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogPVRCutterOSD::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
}

void CGUIDialogPVRCutterOSD::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

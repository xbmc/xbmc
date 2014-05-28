/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#ifndef DIALOGS_GUIDIALOGVIDEOOSD_H_INCLUDED
#define DIALOGS_GUIDIALOGVIDEOOSD_H_INCLUDED
#include "GUIDialogVideoOSD.h"
#endif

#ifndef DIALOGS_APPLICATION_H_INCLUDED
#define DIALOGS_APPLICATION_H_INCLUDED
#include "Application.h"
#endif

#ifndef DIALOGS_FILEITEM_H_INCLUDED
#define DIALOGS_FILEITEM_H_INCLUDED
#include "FileItem.h"
#endif

#ifndef DIALOGS_GUIUSERMESSAGES_H_INCLUDED
#define DIALOGS_GUIUSERMESSAGES_H_INCLUDED
#include "GUIUserMessages.h"
#endif

#ifndef DIALOGS_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#define DIALOGS_GUILIB_GUIWINDOWMANAGER_H_INCLUDED
#include "guilib/GUIWindowManager.h"
#endif

#ifndef DIALOGS_GUILIB_KEY_H_INCLUDED
#define DIALOGS_GUILIB_KEY_H_INCLUDED
#include "guilib/Key.h"
#endif

#ifndef DIALOGS_INPUT_MOUSESTAT_H_INCLUDED
#define DIALOGS_INPUT_MOUSESTAT_H_INCLUDED
#include "input/MouseStat.h"
#endif

#ifndef DIALOGS_CORES_IPLAYER_H_INCLUDED
#define DIALOGS_CORES_IPLAYER_H_INCLUDED
#include "cores/IPlayer.h"
#endif


#ifndef DIALOGS_PVR_PVRMANAGER_H_INCLUDED
#define DIALOGS_PVR_PVRMANAGER_H_INCLUDED
#include "pvr/PVRManager.h"
#endif

#ifndef DIALOGS_PVR_CHANNELS_PVRCHANNELGROUPSCONTAINER_H_INCLUDED
#define DIALOGS_PVR_CHANNELS_PVRCHANNELGROUPSCONTAINER_H_INCLUDED
#include "pvr/channels/PVRChannelGroupsContainer.h"
#endif


using namespace PVR;

CGUIDialogVideoOSD::CGUIDialogVideoOSD(void)
    : CGUIDialog(WINDOW_DIALOG_VIDEO_OSD, "VideoOSD.xml")
{
  m_loadType = KEEP_IN_MEMORY;
}

CGUIDialogVideoOSD::~CGUIDialogVideoOSD(void)
{
}

void CGUIDialogVideoOSD::FrameMove()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (g_Mouse.IsActive() || g_windowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_CHANNELS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_GUIDE)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_DIRECTOR)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_PVR_OSD_CUTTER)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_OSD_TELETEXT))
      // extend show time by original value
      SetAutoClose(m_showDuration);
  }
  CGUIDialog::FrameMove();
}

bool CGUIDialogVideoOSD::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SHOW_OSD)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

EVENT_RESULT CGUIDialogVideoOSD::OnMouseEvent(const CPoint &point, const CMouseEvent &event)
{
  if (event.m_id == ACTION_MOUSE_WHEEL_UP)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_FORWARD, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }
  if (event.m_id == ACTION_MOUSE_WHEEL_DOWN)
  {
    return g_application.OnAction(CAction(ACTION_ANALOG_SEEK_BACK, 0.5f)) ? EVENT_RESULT_HANDLED : EVENT_RESULT_UNHANDLED;
  }

  return CGUIDialog::OnMouseEvent(point, event);
}

bool CGUIDialogVideoOSD::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_VIDEO_MENU_STARTED:
    {
      // We have gone to the DVD menu, so close the OSD.
      Close();
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:  // fired when OSD is hidden
    {
      // Remove our subdialogs if visible
      CGUIDialog *pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning())
        pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_CHANNELS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_GUIDE);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_DIRECTOR);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_PVR_OSD_CUTTER);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_OSD_TELETEXT);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}


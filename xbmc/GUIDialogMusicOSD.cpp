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
#include "GUIDialogMusicOSD.h"
#include "GUIWindowSettingsCategory.h"
#include "Application.h"
#include "GUIWindowManager.h"


#define CONTROL_VIS_BUTTON       500
#define CONTROL_LOCK_BUTTON      501
#define CONTROL_VIS_CHOOSER      503

CGUIDialogMusicOSD::CGUIDialogMusicOSD(void)
    : CGUIDialog(WINDOW_DIALOG_MUSIC_OSD, "MusicOSD.xml")
{
  m_pVisualisation = NULL;
  LoadOnDemand(false);    // we are loaded by the vis window.
}

CGUIDialogMusicOSD::~CGUIDialogMusicOSD(void)
{
}

bool CGUIDialogMusicOSD::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == CONTROL_VIS_CHOOSER)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
        OnMessage(msg);
        CStdString strLabel = msg.GetLabel();
        if (msg.GetParam1() == 0)
          g_guiSettings.SetString("mymusic.visualisation", "None");
        else
          g_guiSettings.SetString("mymusic.visualisation",
                                  CVisualisation::GetCombinedName( strLabel ));
        // hide the control and reset focus
        SET_CONTROL_HIDDEN(CONTROL_VIS_CHOOSER);
        SET_CONTROL_FOCUS(CONTROL_VIS_BUTTON, 0);
        return true;
      }
      else if (iControl == CONTROL_VIS_BUTTON)
      {
        SET_CONTROL_VISIBLE(CONTROL_VIS_CHOOSER);
        SET_CONTROL_FOCUS(CONTROL_VIS_CHOOSER, 0);
        // fire off an event that we've pressed this button...
        CAction action;
        action.wID = ACTION_SELECT_ITEM;
        OnAction(action);
      }
      else if (iControl == CONTROL_LOCK_BUTTON)
      {
        CGUIMessage msg(GUI_MSG_VISUALISATION_ACTION, 0, 0, ACTION_VIS_PRESET_LOCK);
        g_graphicsContext.SendMessage(msg);
      }
      return true;
    }
    break;
  case GUI_MSG_WINDOW_DEINIT:
  case GUI_MSG_VISUALISATION_UNLOADING:
    {
      m_pVisualisation = NULL;
    }
    break;
  case GUI_MSG_VISUALISATION_LOADED:
    {
      if (message.GetLPVOID())
        m_pVisualisation = (CVisualisation *)message.GetLPVOID();
    }
  }
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogMusicOSD::Render()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (g_Mouse.HasMoved() || m_gWindowManager.IsWindowActive(WINDOW_DIALOG_VIS_SETTINGS)
                           || m_gWindowManager.IsWindowActive(WINDOW_DIALOG_VIS_PRESET_LIST))
      SetAutoClose(3000);
  }
  CGUIDialog::Render();
}

void CGUIDialogMusicOSD::OnInitWindow()
{
  CSetting *pSetting = g_guiSettings.GetSetting("mymusic.visualisation");
  CGUIWindowSettingsCategory::FillInVisualisations(pSetting, CONTROL_VIS_CHOOSER);

  ResetControlStates();
  CGUIDialog::OnInitWindow();
}

bool CGUIDialogMusicOSD::OnAction(const CAction &action)
{
  // keyboard or controller movement should prevent autoclosing
  if (action.wID != ACTION_MOUSE && m_autoClosing)
    SetAutoClose(3000);
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogMusicOSD::OnMouse(const CPoint &point)
{
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // pause
    CAction action;
    action.wID = ACTION_PAUSE;
    return g_application.OnAction(action);
  }
  return CGUIDialog::OnMouse(point);
}


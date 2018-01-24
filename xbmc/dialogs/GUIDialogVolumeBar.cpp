/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
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

#include "GUIDialogVolumeBar.h"
#include "IGUIVolumeBarCallback.h"
#include "Application.h"
#include "input/Key.h"
#include "threads/SingleLock.h"

#define VOLUME_BAR_DISPLAY_TIME 1000L

CGUIDialogVolumeBar::CGUIDialogVolumeBar(void)
  : CGUIDialog(WINDOW_DIALOG_VOLUME_BAR, "DialogVolumeBar.xml", DialogModalityType::MODELESS)
{
  m_loadType = LOAD_ON_GUI_INIT;
}

CGUIDialogVolumeBar::~CGUIDialogVolumeBar(void) = default;

bool CGUIDialogVolumeBar::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_VOLUME_UP || action.GetID() == ACTION_VOLUME_DOWN || action.GetID() == ACTION_VOLUME_SET || action.GetID() == ACTION_MUTE)
  {
    if (g_application.IsMuted() || g_application.GetVolume(false) <= VOLUME_MINIMUM)
    { // cancel the timer, dialog needs to stay visible
      CancelAutoClose();
      return true;
    }
    else
    { // reset the timer, as we've changed the volume level
      SetAutoClose(VOLUME_BAR_DISPLAY_TIME);
      return true;
    }
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogVolumeBar::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  case GUI_MSG_WINDOW_DEINIT:
    return CGUIDialog::OnMessage(message);
  }
  return false; // don't process anything other than what we need!
}

void CGUIDialogVolumeBar::RegisterCallback(IGUIVolumeBarCallback *callback)
{
  CSingleLock lock(m_callbackMutex);

  m_callbacks.insert(callback);
}

void CGUIDialogVolumeBar::UnregisterCallback(IGUIVolumeBarCallback *callback)
{
  CSingleLock lock(m_callbackMutex);

  m_callbacks.erase(callback);
}

bool CGUIDialogVolumeBar::IsVolumeBarEnabled() const
{
  CSingleLock lock(m_callbackMutex);

  // Hide volume bar if any callbacks are shown
  for (const auto &callback : m_callbacks)
  {
    if (callback->IsShown())
      return false;
  }

  return true;
}

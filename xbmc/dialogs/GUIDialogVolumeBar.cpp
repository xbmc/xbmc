/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIDialogVolumeBar.h"

#include "Application.h"
#include "IGUIVolumeBarCallback.h"
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
    if (g_application.IsMuted() || g_application.GetVolumeRatio() <= VOLUME_MINIMUM)
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

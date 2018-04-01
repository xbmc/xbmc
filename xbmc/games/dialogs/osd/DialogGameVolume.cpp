/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "DialogGameVolume.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "Application.h"
#include "ServiceBroker.h"

#include <cmath>

using namespace KODI;
using namespace GAME;

#define CONTROL_LABEL   12 //! @todo Remove me

CDialogGameVolume::CDialogGameVolume()
{
  // Initialize CGUIWindow
  SetID(WINDOW_DIALOG_GAME_VOLUME);
  m_loadType = KEEP_IN_MEMORY;
}

bool CDialogGameVolume::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_STATE_CHANGED:
    {
      int controlId = message.GetControlId();
      if (controlId == GetID())
      {
        OnStateChanged();
        return true;
      }

      break;
    }
    default:
      break;
  }

  return CGUIDialogSlider::OnMessage(message);
}

void CDialogGameVolume::OnInitWindow()
{
  m_volumePercent = m_oldVolumePercent = GetVolumePercent();

  CGUIDialogSlider::OnInitWindow();

  // Set slider parameters
  SetModalityType(DialogModalityType::MODAL);
  SetSlider(GetLabel(), GetVolumePercent(), VOLUME_MIN, VOLUME_DELTA, VOLUME_MAX, this, nullptr);

  SET_CONTROL_HIDDEN(CONTROL_LABEL);

  CGUIDialogVolumeBar *dialogVolumeBar = dynamic_cast<CGUIDialogVolumeBar*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_DIALOG_VOLUME_BAR));
  if (dialogVolumeBar != nullptr)
    dialogVolumeBar->RegisterCallback(this);

  ANNOUNCEMENT::CAnnouncementManager::GetInstance().AddAnnouncer(this);
}

void CDialogGameVolume::OnDeinitWindow(int nextWindowID)
{
  ANNOUNCEMENT::CAnnouncementManager::GetInstance().RemoveAnnouncer(this);

  CGUIDialogVolumeBar *dialogVolumeBar = dynamic_cast<CGUIDialogVolumeBar*>(CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_DIALOG_VOLUME_BAR));
  if (dialogVolumeBar != nullptr)
    dialogVolumeBar->UnregisterCallback(this);

  CGUIDialogSlider::OnDeinitWindow(nextWindowID);
}

void CDialogGameVolume::OnSliderChange(void *data, CGUISliderControl *slider)
{
  const float volumePercent = slider->GetFloatValue();

  if (std::fabs(volumePercent - m_volumePercent) > 0.1f)
  {
    m_volumePercent = volumePercent;
    g_application.SetVolume(volumePercent, true);
  }
}

bool CDialogGameVolume::IsShown() const
{
  return m_active;
}

void CDialogGameVolume::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  if (flag == ANNOUNCEMENT::Application && strcmp(message, "OnVolumeChanged") == 0)
  {
    const float volumePercent = static_cast<float>(data["volume"].asDouble());

    if (std::fabs(volumePercent - m_volumePercent) > 0.1f)
    {
      m_volumePercent = volumePercent;

      CGUIMessage msg(GUI_MSG_STATE_CHANGED, GetID(), GetID());
      CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg);
    }
  }
}

void CDialogGameVolume::OnStateChanged()
{
  if (m_volumePercent != m_oldVolumePercent)
  {
    m_oldVolumePercent = m_volumePercent;
    SetSlider(GetLabel(), m_volumePercent, VOLUME_MIN, VOLUME_DELTA, VOLUME_MAX, this, nullptr);
  }
}

float CDialogGameVolume::GetVolumePercent() const
{
  return g_application.GetVolume(true);
}

std::string CDialogGameVolume::GetLabel()
{
  return g_localizeStrings.Get(13376); // "Volume"
}

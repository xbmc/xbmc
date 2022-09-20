/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameVolume.h"

#include "ServiceBroker.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationVolumeHandling.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIDialog.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUISliderControl.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "utils/Variant.h"

#include <cmath>

using namespace KODI;
using namespace GAME;

#define CONTROL_LABEL 12 //! @todo Remove me

CDialogGameVolume::CDialogGameVolume()
{
  // Initialize CGUIWindow
  SetID(WINDOW_DIALOG_GAME_VOLUME);
  m_loadType = KEEP_IN_MEMORY;
}

bool CDialogGameVolume::OnMessage(CGUIMessage& message)
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

  CGUIDialogVolumeBar* dialogVolumeBar = dynamic_cast<CGUIDialogVolumeBar*>(
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_DIALOG_VOLUME_BAR));
  if (dialogVolumeBar != nullptr)
    dialogVolumeBar->RegisterCallback(this);

  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(this);
}

void CDialogGameVolume::OnDeinitWindow(int nextWindowID)
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(this);

  CGUIDialogVolumeBar* dialogVolumeBar = dynamic_cast<CGUIDialogVolumeBar*>(
      CServiceBroker::GetGUI()->GetWindowManager().GetWindow(WINDOW_DIALOG_VOLUME_BAR));
  if (dialogVolumeBar != nullptr)
    dialogVolumeBar->UnregisterCallback(this);

  CGUIDialogSlider::OnDeinitWindow(nextWindowID);
}

void CDialogGameVolume::OnSliderChange(void* data, CGUISliderControl* slider)
{
  const float volumePercent = slider->GetFloatValue();

  if (std::fabs(volumePercent - m_volumePercent) > 0.1f)
  {
    m_volumePercent = volumePercent;
    auto& components = CServiceBroker::GetAppComponents();
    const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
    appVolume->SetVolume(volumePercent, true);
  }
}

bool CDialogGameVolume::IsShown() const
{
  return m_active;
}

void CDialogGameVolume::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                 const std::string& sender,
                                 const std::string& message,
                                 const CVariant& data)
{
  if (flag == ANNOUNCEMENT::Application && message == "OnVolumeChanged")
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
  const auto& components = CServiceBroker::GetAppComponents();
  const auto appVolume = components.GetComponent<CApplicationVolumeHandling>();
  return appVolume->GetVolumePercent();
}

std::string CDialogGameVolume::GetLabel()
{
  return g_localizeStrings.Get(13376); // "Volume"
}

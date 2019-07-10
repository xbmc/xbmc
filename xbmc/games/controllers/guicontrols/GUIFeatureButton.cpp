/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIFeatureButton.h"

#include "games/controllers/windows/GUIControllerDefines.h"
#include "guilib/GUIMessage.h"
#include "guilib/WindowIDs.h"
#include "messaging/ApplicationMessenger.h"
#include "threads/Event.h"
#include "utils/StringUtils.h"

using namespace KODI;
using namespace GAME;

CGUIFeatureButton::CGUIFeatureButton(const CGUIButtonControl& buttonTemplate,
                                     IConfigurationWizard* wizard,
                                     const CControllerFeature& feature,
                                     unsigned int index) :
  CGUIButtonControl(buttonTemplate),
  m_feature(feature),
  m_wizard(wizard)
{
  // Initialize CGUIButtonControl
  SetLabel(m_feature.Label());
  SetID(CONTROL_FEATURE_BUTTONS_START + index);
  SetVisible(true);
  AllocResources();
}

void CGUIFeatureButton::OnUnFocus(void)
{
  CGUIButtonControl::OnUnFocus();
  m_wizard->OnUnfocus(this);
}

bool CGUIFeatureButton::DoPrompt(const std::string& strPrompt, const std::string& strWarn, const std::string& strFeature, CEvent& waitEvent)
{
  using namespace MESSAGING;

  bool bInterrupted = false;

  if (!HasFocus())
  {
    CGUIMessage msgFocus(GUI_MSG_SETFOCUS, GetID(), GetID());
    CApplicationMessenger::GetInstance().SendGUIMessage(msgFocus, WINDOW_INVALID, false);
  }

  CGUIMessage msgLabel(GUI_MSG_LABEL_SET, GetID(), GetID());

  for (unsigned int i = 0; i < COUNTDOWN_DURATION_SEC; i++)
  {
    const unsigned int secondsElapsed = i;
    const unsigned int secondsRemaining = COUNTDOWN_DURATION_SEC - i;

    const bool bWarn = secondsElapsed >= WAIT_TO_WARN_SEC;

    std::string strLabel;

    if (bWarn)
      strLabel = StringUtils::Format(strWarn.c_str(), strFeature.c_str(), secondsRemaining);
    else
      strLabel = StringUtils::Format(strPrompt.c_str(), strFeature.c_str(), secondsRemaining);

    msgLabel.SetLabel(strLabel);
    CApplicationMessenger::GetInstance().SendGUIMessage(msgLabel, WINDOW_INVALID, false);

    waitEvent.Reset();
    bInterrupted = waitEvent.WaitMSec(1000); // Wait 1 second

    if (bInterrupted)
      break;
  }

  // Reset label
  msgLabel.SetLabel(m_feature.Label());
  CApplicationMessenger::GetInstance().SendGUIMessage(msgLabel, WINDOW_INVALID, false);

  return bInterrupted;
}

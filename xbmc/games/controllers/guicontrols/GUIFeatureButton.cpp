/*
 *      Copyright (C) 2016-2017 Team Kodi
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

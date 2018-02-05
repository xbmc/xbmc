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

#include "GUIThrottleButton.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <string>

using namespace KODI;
using namespace GAME;

CGUIThrottleButton::CGUIThrottleButton(const CGUIButtonControl& buttonTemplate,
                                       IConfigurationWizard* wizard,
                                       const CControllerFeature& feature,
                                       unsigned int index) :
  CGUIFeatureButton(buttonTemplate, wizard, feature, index)
{
  Reset();
}

bool CGUIThrottleButton::PromptForInput(CEvent& waitEvent)
{
  using namespace JOYSTICK;

  bool bInterrupted = false;

  // Get the prompt for the current analog stick direction
  std::string strPrompt;
  std::string strWarn;
  switch (m_state)
  {
    case STATE::THROTTLE_UP:
      strPrompt = g_localizeStrings.Get(35092); // "Move %s up"
      strWarn   = g_localizeStrings.Get(35093); // "Move %s up (%d)"
      break;
    case STATE::THROTTLE_DOWN:
      strPrompt = g_localizeStrings.Get(35094); // "Move %s down"
      strWarn   = g_localizeStrings.Get(35095); // "Move %s down (%d)"
      break;
    default:
      break;
  }

  if (!strPrompt.empty())
  {
    bInterrupted = DoPrompt(strPrompt, strWarn, m_feature.Label(), waitEvent);

    if (!bInterrupted)
      m_state = STATE::FINISHED; // Not interrupted, must have timed out
    else
      m_state = GetNextState(m_state); // Interrupted by input, proceed
  }

  return bInterrupted;
}

bool CGUIThrottleButton::IsFinished(void) const
{
  return m_state >= STATE::FINISHED;
}

JOYSTICK::THROTTLE_DIRECTION CGUIThrottleButton::GetThrottleDirection(void) const
{
  using namespace JOYSTICK;

  switch (m_state)
  {
    case STATE::THROTTLE_UP:    return THROTTLE_DIRECTION::UP;
    case STATE::THROTTLE_DOWN:  return THROTTLE_DIRECTION::DOWN;
    default:
      break;
  }

  return THROTTLE_DIRECTION::NONE;
}

void CGUIThrottleButton::Reset(void)
{
  m_state = STATE::THROTTLE_UP;
}

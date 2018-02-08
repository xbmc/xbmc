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

#include "GUIWheelButton.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <string>

using namespace KODI;
using namespace GAME;

CGUIWheelButton::CGUIWheelButton(const CGUIButtonControl& buttonTemplate,
                                 IConfigurationWizard* wizard,
                                 const CControllerFeature& feature,
                                 unsigned int index) :
  CGUIFeatureButton(buttonTemplate, wizard, feature, index)
{
  Reset();
}

bool CGUIWheelButton::PromptForInput(CEvent& waitEvent)
{
  using namespace JOYSTICK;

  bool bInterrupted = false;

  // Get the prompt for the current analog stick direction
  std::string strPrompt;
  std::string strWarn;
  switch (m_state)
  {
    case STATE::WHEEL_LEFT:
      strPrompt = g_localizeStrings.Get(35098); // "Move %s left"
      strWarn   = g_localizeStrings.Get(35099); // "Move %s left (%d)"
      break;
    case STATE::WHEEL_RIGHT:
      strPrompt = g_localizeStrings.Get(35096); // "Move %s right"
      strWarn   = g_localizeStrings.Get(35097); // "Move %s right (%d)"
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

bool CGUIWheelButton::IsFinished(void) const
{
  return m_state >= STATE::FINISHED;
}

JOYSTICK::WHEEL_DIRECTION CGUIWheelButton::GetWheelDirection(void) const
{
  using namespace JOYSTICK;

  switch (m_state)
  {
    case STATE::WHEEL_LEFT:  return WHEEL_DIRECTION::LEFT;
    case STATE::WHEEL_RIGHT: return WHEEL_DIRECTION::RIGHT;
    default:
      break;
  }

  return WHEEL_DIRECTION::NONE;
}

void CGUIWheelButton::Reset(void)
{
  m_state = STATE::WHEEL_LEFT;
}

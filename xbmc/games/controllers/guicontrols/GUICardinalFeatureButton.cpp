/*
 *      Copyright (C) 2016-2017 Team Kodi
 *      http://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "GUICardinalFeatureButton.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"

#include <string>

using namespace KODI;
using namespace GAME;

CGUICardinalFeatureButton::CGUICardinalFeatureButton(const CGUIButtonControl& buttonTemplate,
                                                     IConfigurationWizard* wizard,
                                                     const CControllerFeature& feature,
                                                     unsigned int index) :
  CGUIFeatureButton(buttonTemplate, wizard, feature, index)
{
  Reset();
}

bool CGUICardinalFeatureButton::PromptForInput(CEvent& waitEvent)
{
  using namespace JOYSTICK;

  bool bInterrupted = false;

  // Get the prompt for the current analog stick direction
  std::string strPrompt;
  std::string strWarn;
  switch (m_state)
  {
    case STATE::CARDINAL_DIRECTION_UP:
      strPrompt = g_localizeStrings.Get(35092); // "Move %s up"
      strWarn   = g_localizeStrings.Get(35093); // "Move %s up (%d)"
      break;
    case STATE::CARDINAL_DIRECTION_RIGHT:
      strPrompt = g_localizeStrings.Get(35096); // "Move %s right"
      strWarn   = g_localizeStrings.Get(35097); // "Move %s right (%d)"
      break;
    case STATE::CARDINAL_DIRECTION_DOWN:
      strPrompt = g_localizeStrings.Get(35094); // "Move %s down"
      strWarn   = g_localizeStrings.Get(35095); // "Move %s down (%d)"
      break;
    case STATE::CARDINAL_DIRECTION_LEFT:
      strPrompt = g_localizeStrings.Get(35098); // "Move %s left"
      strWarn   = g_localizeStrings.Get(35099); // "Move %s left (%d)"
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

bool CGUICardinalFeatureButton::IsFinished(void) const
{
  return m_state >= STATE::FINISHED;
}

KODI::INPUT::CARDINAL_DIRECTION CGUICardinalFeatureButton::GetCardinalDirection(void) const
{
  using namespace INPUT;

  switch (m_state)
  {
    case STATE::CARDINAL_DIRECTION_UP:    return CARDINAL_DIRECTION::UP;
    case STATE::CARDINAL_DIRECTION_RIGHT: return CARDINAL_DIRECTION::RIGHT;
    case STATE::CARDINAL_DIRECTION_DOWN:  return CARDINAL_DIRECTION::DOWN;
    case STATE::CARDINAL_DIRECTION_LEFT:  return CARDINAL_DIRECTION::LEFT;
    default:
      break;
  }

  return CARDINAL_DIRECTION::NONE;
}

void CGUICardinalFeatureButton::Reset(void)
{
  m_state = STATE::CARDINAL_DIRECTION_UP;
}

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

#include "JoystickEasterEgg.h"
#include "JoystickIDs.h"
#include "ServiceBroker.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/WindowIDs.h"
#include "settings/Settings.h"

using namespace KODI;
using namespace JOYSTICK;

const std::map<std::string, std::vector<FeatureName>> CJoystickEasterEgg::m_sequence =
{
  {
    DEFAULT_CONTROLLER_ID,
    {
      "up",
      "up",
      "down",
      "down",
      "left",
      "right",
      "left",
      "right",
      "b",
      "a",
    },
  },
};

CJoystickEasterEgg::CJoystickEasterEgg(const std::string& controllerId) :
  m_controllerId(controllerId),
  m_state(0)
{
}

bool CJoystickEasterEgg::OnButtonPress(const FeatureName& feature)
{
  bool bHandled = false;

  auto it = m_sequence.find(m_controllerId);
  if (it != m_sequence.end())
  {
    const auto& sequence = it->second;

    // Update state
    if (feature == sequence[m_state])
      m_state++;
    else
      m_state = 0;

    // Capture input when finished with arrows (2 x up/down/left/right)
    if (m_state > 8)
    {
      bHandled = true;

      if (m_state >= sequence.size())
      {
        OnFinish();
        m_state = 0;
      }
    }
  }

  return bHandled;
}

void CJoystickEasterEgg::OnFinish(void)
{
  CServiceBroker::GetSettings().ToggleBool(CSettings::SETTING_GAMES_ENABLE);

  WINDOW_SOUND sound = CServiceBroker::GetSettings().GetBool(CSettings::SETTING_GAMES_ENABLE) ? SOUND_INIT : SOUND_DEINIT;
  g_audioManager.PlayWindowSound(WINDOW_DIALOG_KAI_TOAST, sound);

  //! @todo Shake screen
}

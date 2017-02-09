/*
 *      Copyright (C) 2016 Team Kodi
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
#include "ServiceBroker.h"
#include "guilib/GUIAudioManager.h"
#include "guilib/WindowIDs.h"
#include "settings/Settings.h"

using namespace KODI;
using namespace JOYSTICK;

std::vector<FeatureName> CJoystickEasterEgg::m_sequence = {
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
};

CJoystickEasterEgg::CJoystickEasterEgg(void) :
  m_state(0)
{
}

bool CJoystickEasterEgg::OnButtonPress(const FeatureName& feature)
{
  bool bHandled = false;

  // Update state
  if (feature == m_sequence[m_state])
    m_state++;
  else
    m_state = 0;

  // Capture input when finished with arrows (2 x up/down/left/right)
  if (m_state > 8)
  {
    bHandled = true;

    if (m_state >= m_sequence.size())
    {
      OnFinish();
      m_state = 0;
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

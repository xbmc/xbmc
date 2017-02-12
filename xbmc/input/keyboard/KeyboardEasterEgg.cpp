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

#include "input/keyboard/KeyboardEasterEgg.h"
#include "input/joysticks/JoystickEasterEgg.h"
#include "input/Key.h"

using namespace KODI;
using namespace KEYBOARD;

std::vector<XBMCVKey> CKeyboardEasterEgg::m_sequence = {
  XBMCVK_UP,
  XBMCVK_UP,
  XBMCVK_DOWN,
  XBMCVK_DOWN,
  XBMCVK_LEFT,
  XBMCVK_RIGHT,
  XBMCVK_LEFT,
  XBMCVK_RIGHT,
  XBMCVK_B,
  XBMCVK_A,
};

CKeyboardEasterEgg::CKeyboardEasterEgg(void) :
  m_state(0)
{
}

bool CKeyboardEasterEgg::OnKeyPress(const CKey& key)
{
  bool bHandled = false;

  // Update state
  if (key.GetVKey() == m_sequence[m_state])
    m_state++;
  else
    m_state = 0;

  // Capture input when finished with arrows (2 x up/down/left/right)
  if (m_state > 8)
  {
    bHandled = true;

    if (m_state >= m_sequence.size())
    {
      JOYSTICK::CJoystickEasterEgg::OnFinish();
      m_state = 0;
    }
  }

  return bHandled;
}

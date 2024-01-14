/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "input/keyboard/KeyboardEasterEgg.h"

#include "input/joysticks/JoystickEasterEgg.h"
#include "input/keyboard/Key.h"

using namespace KODI;
using namespace KEYBOARD;

std::vector<XBMCVKey> CKeyboardEasterEgg::m_sequence = {
    XBMCVK_UP,    XBMCVK_UP,   XBMCVK_DOWN,  XBMCVK_DOWN, XBMCVK_LEFT,
    XBMCVK_RIGHT, XBMCVK_LEFT, XBMCVK_RIGHT, XBMCVK_B,    XBMCVK_A,
};

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

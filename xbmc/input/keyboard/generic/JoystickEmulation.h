/*
 *      Copyright (C) 2015-2017 Team Kodi
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
#pragma once

#include "input/keyboard/IKeyboardHandler.h"

#include <vector>

namespace KODI
{
namespace JOYSTICK
{
  class IDriverHandler;
}

namespace KEYBOARD
{
  /*!
   * \ingroup keyboard
   * \brief Generic implementation of a handler for joysticks that use keyboard
   *        drivers. It basically emulates a joystick with many buttons.
   */
  class CJoystickEmulation : public IKeyboardHandler
  {
  public:
    CJoystickEmulation(JOYSTICK::IDriverHandler* handler);

    ~CJoystickEmulation(void) override = default;

    // implementation of IKeyboardHandler
    bool OnKeyPress(const CKey& key) override;
    void OnKeyRelease(const CKey& key) override;

  private:
    struct KeyEvent
    {
      unsigned int buttonIndex;
      bool         bHandled;
    };

    bool OnPress(unsigned int buttonIndex);
    void OnRelease(unsigned int buttonIndex);
    bool GetEvent(unsigned int buttonIndex, KeyEvent& event) const;

    static unsigned int GetButtonIndex(const CKey& key);

    JOYSTICK::IDriverHandler* const m_handler;
    std::vector<KeyEvent>           m_pressedKeys;
  };
}
}

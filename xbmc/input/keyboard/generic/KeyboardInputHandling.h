/*
 *      Copyright (C) 2017 Team Kodi
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

#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"

namespace KODI
{
  namespace JOYSTICK
  {
    class IButtonMap;
  }

  namespace KEYBOARD
  {
    class IKeyboardInputHandler;

    /*!
     * \ingroup keyboard
     * \brief Class to translate input from Kodi keycodes to key names defined
     *        by the keyboard's controller profile
     */
    class CKeyboardInputHandling : public IKeyboardDriverHandler
    {
    public:
      CKeyboardInputHandling(IKeyboardInputHandler* handler, JOYSTICK::IButtonMap* buttonMap);

      ~CKeyboardInputHandling(void) override = default;

      // implementation of IKeyboardDriverHandler
      bool OnKeyPress(const CKey& key) override;
      void OnKeyRelease(const CKey& key) override;

    private:
      // Construction parameters
      IKeyboardInputHandler* const m_handler;
      JOYSTICK::IButtonMap* const m_buttonMap;
    };
  }
}

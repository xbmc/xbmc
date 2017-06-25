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

#include "input/joysticks/DefaultController.h"

#include <string>

namespace KODI
{
namespace GAME
{
  class CGameClient;

  /*!
   * \ingroup games
   * \brief Monitors for keymap actions in the background during gameplay
   *
   * This class inherits from the default controller that Kodi uses for
   * normal input. However, it overrides some functions of CDefaultJoystick
   * to customize the input handling behavior:
   *
   *   - GetWindowID():
   *         This forces the keymap handler to use the FullscreenGame window
   *
   *   - GetFallthrough():
   *         This prevents the keymap handler from falling through to
   *         FullscreenVideo or the global keymap when looking up keys.
   *
   * This class is registered as a promiscuous input handler, so it receives
   * all input but doesn't block any handled input from reaching the game
   * client.
   */
  class CPortInput : public JOYSTICK::CDefaultController
  {
  public:
    CPortInput(CGameClient &gameClient);

    virtual ~CPortInput() = default;

    // Implementation of IInputHandler via CDefaultController
    virtual bool AcceptsInput(const std::string &feature) const override;

    // Implementation of CDefaultJoystick via CDefaultController
    virtual int GetWindowID() const;
    virtual bool GetFallthrough() const { return false; }

  private:
    const CGameClient &m_gameClient;
  };
}
}
